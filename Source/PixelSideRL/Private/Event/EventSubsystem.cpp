#include "Event/EventSubsystem.h"

#include "Combat/DamageableComponent.h"
#include "Event/EventOps.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelSideEvent, Log, All);

struct FEventCatalogState
{
	PixelSideEvent::FCatalog Catalog;
};

namespace
{
std::string ToUtf8(const FString& Text)
{
	const FTCHARToUTF8 Convert(*Text);
	return std::string(Convert.Get(), static_cast<size_t>(FMath::Max(0, Convert.Length())));
}

FString FromUtf8(std::string_view Text)
{
	const FUTF8ToTCHAR Convert(Text.data(), static_cast<int32>(Text.size()));
	return FString(Convert.Length(), Convert.Get());
}

FName IdName(std::string_view Id)
{
	return FName(FromUtf8(Id));
}

UDamageableComponent* FindPlayerDamageable(const UObject* WorldContext)
{
	if (!WorldContext)
	{
		return nullptr;
	}
	UWorld* World = WorldContext->GetWorld();
	if (!World)
	{
		return nullptr;
	}
	APlayerController* PC = World->GetFirstPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	return Pawn ? Pawn->FindComponentByClass<UDamageableComponent>() : nullptr;
}

FEventResolveView ToView(const PixelSideEvent::FResolveResult& Result)
{
	FEventResolveView View;
	View.bOk = Result.bOk;
	View.EventId = IdName(Result.EventId);
	View.ChoiceId = IdName(Result.ChoiceId);
	View.ResultToast = FText::FromString(FromUtf8(Result.ResultToast));
	View.RunScrap = Result.Scrap;
	View.CurrentHP = Result.CurrentHP;
	View.MaxHP = Result.MaxHP;
	View.bAffixOnResolve = Result.bAffixOnResolve;
	View.bAffixChoiceRequested = Result.bAffixChoiceRequested;
	View.bCombatAllowed = Result.bCombatAllowed;
	View.bCombatStarted = Result.bCombatStarted;
	View.Error = FromUtf8(Result.Error);
	for (const std::string& Note : Result.Notes)
	{
		View.Notes.Add(FromUtf8(Note));
	}
	for (const PixelSideEvent::FOpStep& Step : Result.Steps)
	{
		FEventOpStepView Row;
		Row.Op = FromUtf8(Step.Op);
		Row.ScrapBefore = Step.ScrapBefore;
		Row.ScrapAfter = Step.ScrapAfter;
		Row.HpBefore = Step.HpBefore;
		Row.HpAfter = Step.HpAfter;
		Row.bApplied = Step.bApplied;
		Row.bUnknown = Step.bUnknown;
		Row.Message = FromUtf8(Step.Message);
		View.Steps.Add(Row);
	}
	return View;
}

void LogResolve(const FEventResolveView& View)
{
	if (!View.bOk)
	{
		UE_LOG(LogPixelSideEvent, Warning, TEXT("Resolve %s/%s failed: %s"),
			*View.EventId.ToString(), *View.ChoiceId.ToString(), *View.Error);
		return;
	}
	for (const FString& Note : View.Notes)
	{
		UE_LOG(LogPixelSideEvent, Warning, TEXT("Resolve %s/%s note: %s"),
			*View.EventId.ToString(), *View.ChoiceId.ToString(), *Note);
	}
	const int32 Count = View.Steps.Num();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FEventOpStepView& Step = View.Steps[Index];
		if (Step.bUnknown)
		{
			UE_LOG(LogPixelSideEvent, Warning, TEXT("Resolve %s/%s step %d/%d %s HP %.1f -> %.1f Scrap %d -> %d | %s"),
				*View.EventId.ToString(),
				*View.ChoiceId.ToString(),
				Index + 1,
				Count,
				*Step.Op,
				Step.HpBefore,
				Step.HpAfter,
				Step.ScrapBefore,
				Step.ScrapAfter,
				*Step.Message);
		}
		else
		{
			UE_LOG(LogPixelSideEvent, Log, TEXT("Resolve %s/%s step %d/%d %s HP %.1f -> %.1f Scrap %d -> %d | %s"),
				*View.EventId.ToString(),
				*View.ChoiceId.ToString(),
				Index + 1,
				Count,
				*Step.Op,
				Step.HpBefore,
				Step.HpAfter,
				Step.ScrapBefore,
				Step.ScrapAfter,
				*Step.Message);
		}
	}
	UE_LOG(LogPixelSideEvent, Log, TEXT("Resolve %s/%s done Scrap=%d HP=%.1f/%.1f affixRequested=%d combatStarted=%d"),
		*View.EventId.ToString(),
		*View.ChoiceId.ToString(),
		View.RunScrap,
		View.CurrentHP,
		View.MaxHP,
		View.bAffixChoiceRequested ? 1 : 0,
		View.bCombatStarted ? 1 : 0);
}
}

UEventSubsystem* UEventSubsystem::Get(const UObject* WorldContext)
{
	if (!WorldContext)
	{
		return nullptr;
	}
	const UWorld* World = WorldContext->GetWorld();
	if (!World || !World->GetGameInstance())
	{
		return nullptr;
	}
	return World->GetGameInstance()->GetSubsystem<UEventSubsystem>();
}

void UEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	State = new FEventCatalogState();
	ReloadFromContent();
}

void UEventSubsystem::Deinitialize()
{
	DiscardState();
	Super::Deinitialize();
}

void UEventSubsystem::BeginDestroy()
{
	DiscardState();
	Super::BeginDestroy();
}

void UEventSubsystem::DiscardState()
{
	delete State;
	State = nullptr;
}

void UEventSubsystem::ReloadFromContent()
{
	if (!State)
	{
		State = new FEventCatalogState();
	}
	const FString Dir = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Events"));
	TArray<FString> Names;
	IFileManager::Get().FindFiles(Names, *Dir, TEXT("json"));
	Names.Sort();
	int32 Loaded = 0;
	for (const FString& Name : Names)
	{
		if (!Name.StartsWith(TEXT("DA_Event_")) || !Name.EndsWith(TEXT(".json")))
		{
			continue;
		}
		const FString Path = Dir / Name;
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			UE_LOG(LogPixelSideEvent, Warning, TEXT("Event JSON unreadable: %s"), *Path);
			continue;
		}
		std::string Error;
		if (!State->Catalog.LoadEventJson(ToUtf8(Text), &Error))
		{
			UE_LOG(LogPixelSideEvent, Warning, TEXT("Event JSON rejected %s: %s"), *Path, *FromUtf8(Error));
			continue;
		}
		++Loaded;
		UE_LOG(LogPixelSideEvent, Log, TEXT("Loaded %s"), *Path);
	}
	const FString PoolPath = Dir / TEXT("DT_Event_MVP.json");
	FString PoolText;
	if (FFileHelper::LoadFileToString(PoolText, *PoolPath))
	{
		std::string Error;
		if (!State->Catalog.LoadPoolJson(ToUtf8(PoolText), &Error) || !State->Catalog.ValidatePool(&Error))
		{
			UE_LOG(LogPixelSideEvent, Warning, TEXT("Event pool check failed: %s"), *FromUtf8(Error));
		}
		else
		{
			UE_LOG(LogPixelSideEvent, Log, TEXT("Event pool %s (%d DA, rule %s)"),
				*PoolPath,
				Loaded,
				*FromUtf8(State->Catalog.PoolNamingRule()));
		}
	}
	else
	{
		UE_LOG(LogPixelSideEvent, Warning, TEXT("Missing event pool %s"), *PoolPath);
	}
	if (!State->Catalog.Find("AbandonedCache"))
	{
		UE_LOG(LogPixelSideEvent, Warning, TEXT("DA_Event_AbandonedCache did not load"));
	}
}

void UEventSubsystem::ResetRunScrap(int32 NewScrap)
{
	RunScrap = 0;
	AddRunScrap(NewScrap);
}

void UEventSubsystem::AddRunScrap(int32 Delta)
{
	PixelSideEvent::FRunScrapWallet Wallet;
	Wallet.Scrap = RunScrap;
	PixelSideEvent::AddScrap(Wallet, Delta);
	RunScrap = Wallet.Scrap;
}

bool UEventSubsystem::GetEventDefinition(FName EventId, FEventDefinitionView& Out) const
{
	Out = FEventDefinitionView();
	if (!State)
	{
		return false;
	}
	const PixelSideEvent::FEventDefinition* Def = State->Catalog.Find(ToUtf8(EventId.ToString()));
	if (!Def)
	{
		return false;
	}
	Out.EventId = IdName(Def->EventId);
	Out.DataAssetName = IdName(Def->DataAssetName);
	Out.DisplayName = FText::FromString(FromUtf8(Def->DisplayName));
	Out.PromptBody = FText::FromString(FromUtf8(Def->PromptBody));
	Out.SkipHint = FText::FromString(FromUtf8(Def->SkipHint));
	Out.bAllowDoorSkip = Def->bAllowDoorSkip;
	Out.bCombatAllowed = Def->bCombatAllowed;
	Out.bAffixOnResolve = Def->bAffixOnResolve;
	Out.BindRoomId = IdName(Def->BindRoomId);
	for (const PixelSideEvent::FChoice& Choice : Def->Choices)
	{
		FEventChoiceView Row;
		Row.ChoiceId = IdName(Choice.ChoiceId);
		Row.Label = FText::FromString(FromUtf8(Choice.Label));
		Row.ResultToast = FText::FromString(FromUtf8(Choice.ResultToast));
		Row.EffectCount = static_cast<int32>(Choice.Effects.size());
		Out.Choices.Add(Row);
	}
	return true;
}

bool UEventSubsystem::ResolveEventChoice(FName EventId, FName ChoiceId, FEventResolveView& OutResult)
{
	OutResult = FEventResolveView();
	OutResult.EventId = EventId;
	OutResult.ChoiceId = ChoiceId;
	if (!State)
	{
		OutResult.Error = TEXT("event catalog missing");
		UE_LOG(LogPixelSideEvent, Warning, TEXT("%s"), *OutResult.Error);
		OnEventChoiceResolved.Broadcast(OutResult);
		return false;
	}

	UDamageableComponent* Damageable = FindPlayerDamageable(this);
	PixelSideEvent::FEventVitals Vitals;
	if (Damageable)
	{
		Vitals.CurrentHP = Damageable->GetCurrentHealth();
		Vitals.MaxHP = Damageable->GetMaxHealth();
	}
	else
	{
		UE_LOG(LogPixelSideEvent, Warning, TEXT("Resolve %s/%s: no player AC_Damageable; HP is not written"),
			*EventId.ToString(), *ChoiceId.ToString());
	}

	PixelSideEvent::FRunScrapWallet Wallet;
	Wallet.Scrap = RunScrap;
	const PixelSideEvent::FResolveResult Result = State->Catalog.Resolve(
		ToUtf8(EventId.ToString()),
		ToUtf8(ChoiceId.ToString()),
		Wallet,
		Vitals);
	OutResult = ToView(Result);

	if (!Result.bOk)
	{
		LogResolve(OutResult);
		OnEventChoiceResolved.Broadcast(OutResult);
		return false;
	}

	// Same call, JSON order. Do not route through ApplyDamage (that can kill and plays hurt feel).
	// Do not call RequestAffixChoice. bAffixChoiceRequested stays false.
	for (const PixelSideEvent::FOpStep& Step : Result.Steps)
	{
		RunScrap = Step.ScrapAfter;
		if (Damageable)
		{
			const bool bWrote = Damageable->ApplyEventHealth(Step.HpAfter);
			if (!bWrote && !FMath::IsNearlyEqual(Step.HpBefore, Step.HpAfter))
			{
				UE_LOG(LogPixelSideEvent, Warning, TEXT("Resolve %s/%s HP write skipped (already dead)"),
					*EventId.ToString(), *ChoiceId.ToString());
			}
		}
	}
	OutResult.RunScrap = RunScrap;
	if (Damageable)
	{
		OutResult.CurrentHP = Damageable->GetCurrentHealth();
		OutResult.MaxHP = Damageable->GetMaxHealth();
	}
	LogResolve(OutResult);
	if (Damageable)
	{
		UE_LOG(LogPixelSideEvent, Log, TEXT("Resolve %s/%s pawn HP %.1f / %.1f alive=%d RunScrap=%d"),
			*EventId.ToString(),
			*ChoiceId.ToString(),
			Damageable->GetCurrentHealth(),
			Damageable->GetMaxHealth(),
			Damageable->IsAlive_Implementation() ? 1 : 0,
			RunScrap);
	}
	OnEventChoiceResolved.Broadcast(OutResult);
	return true;
}

#if !UE_BUILD_SHIPPING
static void PixelSideResolveEventCmd(const TArray<FString>& Args)
{
	const FName EventId(*(Args.Num() > 0 ? Args[0] : FString(TEXT("AbandonedCache"))));
	const FName ChoiceId(*(Args.Num() > 1 ? Args[1] : FString(TEXT("CarefulLoot"))));
	int32 Ran = 0;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || !World->IsGameWorld() || !World->GetGameInstance())
			{
				continue;
			}
			if (UEventSubsystem* Events = World->GetGameInstance()->GetSubsystem<UEventSubsystem>())
			{
				FEventResolveView View;
				Events->ResolveEventChoice(EventId, ChoiceId, View);
				++Ran;
			}
		}
	}
	if (Ran == 0)
	{
		UE_LOG(LogPixelSideEvent, Warning, TEXT("PixelSide.ResolveEvent: no game world. Lab F8=CarefulLoot F9=ForceLoot."));
	}
}

static FAutoConsoleCommand GPixelSideResolveEvent(
	TEXT("PixelSide.ResolveEvent"),
	TEXT("PixelSide.ResolveEvent <EventId=AbandonedCache> <ChoiceId=CarefulLoot|ForceLoot>. Non-shipping. Logs Scrap and HP. Does not grant affixes or change Core."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&PixelSideResolveEventCmd));
#endif
