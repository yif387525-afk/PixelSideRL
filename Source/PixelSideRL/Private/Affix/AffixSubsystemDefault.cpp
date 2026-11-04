#include "Affix/AffixSubsystemDefault.h"

#include "Affix/AffixRowApply.h"
#include "Affix/AffixStatMath.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelSideAffix, Log, All);

struct FAffixP05State
{
	PixelSideAffix::FRunModel Model;
};

namespace
{
void EmitChange(UAffixSubsystemDefault* Subsystem, const PixelSideAffix::FChange& Change)
{
	if (!Subsystem)
	{
		return;
	}
	FAffixChangeEvent Event;
	Event.AffixId = PixelSideAffixUE::IdName(Change.AffixId);
	Event.Stacks = Change.Stacks;
	Event.bRemoved = Change.bRemoved;
	Subsystem->OnAffixChanged.Broadcast(Event);
	Subsystem->OnStatsRebuilt.Broadcast();
}
}

UAffixSubsystemDefault* UAffixSubsystemDefault::Get(const UObject* WorldContext)
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
	return World->GetGameInstance()->GetSubsystem<UAffixSubsystemDefault>();
}

void UAffixSubsystemDefault::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PixelSideAffixUE::CheckStatOrdinals();
	EnsureState();
	ReloadTableFromContent();
	RebuildDefinitions();
}

void UAffixSubsystemDefault::Deinitialize()
{
	DiscardState();
	Super::Deinitialize();
}

void UAffixSubsystemDefault::BeginDestroy()
{
	DiscardState();
	Super::BeginDestroy();
}

void UAffixSubsystemDefault::EnsureState()
{
	if (!State)
	{
		State = new FAffixP05State();
	}
}

void UAffixSubsystemDefault::DiscardState()
{
	RuntimeAffixes.Reset();
	delete State;
	State = nullptr;
}

void UAffixSubsystemDefault::EnsureRun()
{
	EnsureState();
	if (State && !State->Model.IsInRun())
	{
		PixelSideAffix::FRunModel::FStart Start;
		Start.Seed = 1;
		State->Model.BeginRun(Start);
		UE_LOG(LogPixelSideAffix, Log, TEXT("BeginRun auto-started (P0.6 lab, empty meta)"));
		OnStatsRebuilt.Broadcast();
	}
}

void UAffixSubsystemDefault::ReloadTableFromContent()
{
	EnsureState();
	const FString JsonPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Affixes/DT_Affix_P05.json"));
	const FString CsvPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Affixes/DT_Affix_P05.csv"));
	const FString BiasPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Affixes/DT_AffixRarityBias.json"));
	FString Text;
	bool bLoaded = false;
	if (FFileHelper::LoadFileToString(Text, *JsonPath))
	{
		std::string Error;
		if (State->Model.LoadJson(PixelSideAffixUE::ToUtf8(Text), &Error) && State->Model.Pool().size() <= 8)
		{
			UE_LOG(LogPixelSideAffix, Log, TEXT("P0.6 affix pool from %s (%d)"), *JsonPath, static_cast<int32>(State->Model.Pool().size()));
			bLoaded = true;
		}
		else
		{
			UE_LOG(LogPixelSideAffix, Warning, TEXT("P0.6 JSON unused (%s): %s"), *JsonPath, *PixelSideAffixUE::FromUtf8(Error));
		}
	}
	if (!bLoaded && FFileHelper::LoadFileToString(Text, *CsvPath))
	{
		std::string Error;
		if (State->Model.LoadCsv(PixelSideAffixUE::ToUtf8(Text), &Error) && State->Model.Pool().size() <= 8)
		{
			UE_LOG(LogPixelSideAffix, Log, TEXT("P0.6 affix pool from %s"), *CsvPath);
			bLoaded = true;
		}
	}
	if (!bLoaded)
	{
		State->Model.UseBuiltinP05();
		UE_LOG(LogPixelSideAffix, Log, TEXT("P0.6 affix pool from C++ BuiltinP05Rows (no uasset)"));
	}
	if (FFileHelper::LoadFileToString(Text, *BiasPath))
	{
		std::string Error;
		if (!State->Model.LoadBiasJson(PixelSideAffixUE::ToUtf8(Text), &Error))
		{
			UE_LOG(LogPixelSideAffix, Warning, TEXT("Rarity bias left at builtin defaults: %s"), *PixelSideAffixUE::FromUtf8(Error));
		}
	}
}

void UAffixSubsystemDefault::RebuildDefinitions()
{
	RuntimeAffixes.Reset();
	if (!State)
	{
		return;
	}
	for (const PixelSideAffix::FAffixRow& Row : State->Model.Pool())
	{
		const FName ObjName = MakeUniqueObjectName(this, UAffixDefinition::StaticClass(), PixelSideAffixUE::IdName(Row.DataAssetName));
		UAffixDefinition* Def = NewObject<UAffixDefinition>(this, ObjName, RF_Transient);
		PixelSideAffixUE::ApplyRow(Def, Row);
		RuntimeAffixes.Add(Def);
	}
}

UAffixDefinition* UAffixSubsystemDefault::FindDefinition(FName AffixId) const
{
	for (const TObjectPtr<UAffixDefinition>& Def : RuntimeAffixes)
	{
		if (Def && Def->AffixId == AffixId)
		{
			return Def.Get();
		}
	}
	return nullptr;
}

TArray<FAffixChoiceEntry> UAffixSubsystemDefault::BuildEntries() const
{
	TArray<FAffixChoiceEntry> Out;
	if (!State)
	{
		return Out;
	}
	for (const PixelSideAffix::FChoice& Choice : State->Model.Pending())
	{
		FAffixChoiceEntry Entry;
		Entry.Def = FindDefinition(PixelSideAffixUE::IdName(Choice.AffixId));
		Entry.WeightUsed = Choice.WeightUsed;
		Entry.bWouldBeNew = Choice.bWouldBeNew;
		Out.Add(Entry);
	}
	return Out;
}

UWorld* UAffixSubsystemDefault::ResolveWorld() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		return GI->GetWorld();
	}
	return GetWorld();
}

void UAffixSubsystemDefault::BeginRun(FRunStartContext Context)
{
	EnsureState();
	PixelSideAffix::FRunModel::FStart Start;
	for (const FAffixModifier& Mod : Context.PermanentModifiers)
	{
		PixelSideAffix::FMod Lite;
		Lite.Stat = PixelSideAffixUE::ToLiteStat(Mod.Stat);
		Lite.Op = PixelSideAffixUE::ToLiteOp(Mod.Op);
		Lite.Value = Mod.Value;
		Lite.DurationSec = Mod.DurationSec;
		if (!Mod.WeaponTag.IsNone())
		{
			Lite.WeaponTag = PixelSideAffixUE::ToUtf8(Mod.WeaponTag.ToString());
		}
		Start.PermanentModifiers.push_back(Lite);
	}
	for (const FName& Id : Context.UnlockedAffixIds)
	{
		if (!Id.IsNone())
		{
			Start.UnlockedAffixIds.push_back(PixelSideAffixUE::ToUtf8(Id.ToString()));
		}
	}
	if (!Context.StartWeaponId.IsNone())
	{
		Start.StartWeaponId = PixelSideAffixUE::ToUtf8(Context.StartWeaponId.ToString());
	}
	Start.StartingScrap = Context.StartingScrap;
	Start.Seed = Context.Seed > 0 ? static_cast<uint32_t>(Context.Seed) : 1u;
	State->Model.BeginRun(Start);
	OnStatsRebuilt.Broadcast();
}

void UAffixSubsystemDefault::EndRun()
{
	if (State)
	{
		State->Model.EndRun();
	}
	OnStatsRebuilt.Broadcast();
}

bool UAffixSubsystemDefault::IsInRun() const
{
	return State && State->Model.IsInRun();
}

void UAffixSubsystemDefault::RequestAffixChoice(EAffixOfferSource Source)
{
	EnsureRun();
	LastOfferSource = Source;
	if (!State->Model.IsChoicePending())
	{
		const bool bOk = State->Model.RollChoices(3, -1, PixelSideAffixUE::ToLiteSource(Source));
		if (!bOk)
		{
			UE_LOG(LogPixelSideAffix, Warning, TEXT("Affix roll incomplete: %s"), *PixelSideAffixUE::FromUtf8(State->Model.LastNote()));
		}
	}
	ApplyChoicePause(true);
	OnAffixChoiceOffered.Broadcast(BuildEntries());
}

bool UAffixSubsystemDefault::ConfirmAffixChoice(int32 Index)
{
	if (!State)
	{
		return false;
	}
	PixelSideAffix::FChange Change;
	if (!State->Model.ConfirmAffixChoice(Index, &Change))
	{
		return false;
	}
	ApplyChoicePause(false);
	EmitChange(this, Change);
	OnAffixChoiceClosed.Broadcast(true);
	return true;
}

void UAffixSubsystemDefault::SkipAffixChoice()
{
	if (!State || !State->Model.SkipAffixChoice())
	{
		return;
	}
	ApplyChoicePause(false);
	OnAffixChoiceClosed.Broadcast(false);
}

TArray<FAffixChoiceEntry> UAffixSubsystemDefault::RollChoices(int32 Count, int32 Seed)
{
	EnsureRun();
	if (!State->Model.IsChoicePending())
	{
		State->Model.RollChoices(Count, Seed, PixelSideAffixUE::ToLiteSource(LastOfferSource));
	}
	return BuildEntries();
}

bool UAffixSubsystemDefault::IsChoicePending() const
{
	return State && State->Model.IsChoicePending();
}

float UAffixSubsystemDefault::GetStat(EStatId Stat) const
{
	return GetStatForWeapon(Stat, NAME_None);
}

float UAffixSubsystemDefault::GetStatForWeapon(EStatId Stat, FName WeaponId) const
{
	if (!State)
	{
		return PixelSideAffix::DefaultBase(PixelSideAffixUE::ToLiteStat(Stat));
	}
	const std::string Tag = WeaponId.IsNone()
		? std::string()
		: PixelSideAffixUE::ToUtf8(WeaponId.ToString());
	return State->Model.GetStat(PixelSideAffixUE::ToLiteStat(Stat), Tag);
}

float UAffixSubsystemDefault::GetStatBase(EStatId Stat) const
{
	if (!State)
	{
		return PixelSideAffix::DefaultBase(PixelSideAffixUE::ToLiteStat(Stat));
	}
	return State->Model.GetBase(PixelSideAffixUE::ToLiteStat(Stat));
}

void UAffixSubsystemDefault::SetBaseStat(EStatId Stat, float Value)
{
	EnsureState();
	const PixelSideAffix::EStat Lite = PixelSideAffixUE::ToLiteStat(Stat);
	if (FMath::IsNearlyEqual(State->Model.GetBase(Lite), Value))
	{
		return;
	}
	State->Model.SetBase(Lite, Value);
	OnStatsRebuilt.Broadcast();
}

int32 UAffixSubsystemDefault::GetAffixStacks(FName AffixId) const
{
	if (!State || AffixId.IsNone())
	{
		return 0;
	}
	return State->Model.GetStacks(PixelSideAffixUE::ToUtf8(AffixId.ToString()));
}

bool UAffixSubsystemDefault::HasTag(FName Tag) const
{
	if (!State || Tag.IsNone())
	{
		return false;
	}
	return State->Model.HasTag(PixelSideAffixUE::ToUtf8(Tag.ToString()));
}

TArray<FAffixRuntimeEntry> UAffixSubsystemDefault::GetOwnedAffixes() const
{
	TArray<FAffixRuntimeEntry> Out;
	if (!State)
	{
		return Out;
	}
	for (const PixelSideAffix::FOwned& Owned : State->Model.Owned())
	{
		FAffixRuntimeEntry Entry;
		Entry.AffixId = PixelSideAffixUE::IdName(Owned.AffixId);
		Entry.Stacks = Owned.Stacks;
		Entry.Def = FindDefinition(Entry.AffixId);
		Out.Add(Entry);
	}
	return Out;
}

FAffixCardView UAffixSubsystemDefault::GetAffixCardView(const UAffixDefinition* Def, int32 PreviewStacks) const
{
	FAffixCardView View;
	if (!Def)
	{
		return View;
	}
	View.Name = Def->DisplayName;
	const float First = Def->Modifiers.Num() > 0 ? Def->Modifiers[0].Value : 0.f;
	const std::string Desc = PixelSideAffix::FormatDescription(
		PixelSideAffixUE::ToUtf8(Def->Description.ToString()), First, PreviewStacks);
	View.Desc = FText::FromString(PixelSideAffixUE::FromUtf8(Desc));
	View.Rarity = Def->Rarity;
	View.Icon = Def->Icon;
	View.AffixId = Def->AffixId;
	View.Stacks = GetAffixStacks(Def->AffixId);
	View.bOwned = View.Stacks > 0;
	return View;
}

FAffixCardView UAffixSubsystemDefault::GetAffixCardViewFromId(FName AffixId) const
{
	if (const UAffixDefinition* Def = FindDefinition(AffixId))
	{
		return GetAffixCardView(Def, 1);
	}
	return FAffixCardView();
}

bool UAffixSubsystemDefault::GrantAffix(FName AffixId)
{
	if (AffixId.IsNone())
	{
		return false;
	}
	EnsureRun();
	PixelSideAffix::FChange Change;
	if (!State->Model.DebugGrant(PixelSideAffixUE::ToUtf8(AffixId.ToString()), &Change))
	{
		return false;
	}
	EmitChange(this, Change);
	return true;
}

bool UAffixSubsystemDefault::DebugGrantAffix(FName AffixId)
{
#if UE_BUILD_SHIPPING
	(void)AffixId;
	return false;
#else
	return GrantAffix(AffixId);
#endif
}

void UAffixSubsystemDefault::NotifyAffixTrigger(EAffixTrigger Trigger)
{
	if (!State)
	{
		return;
	}
	const std::vector<std::string> Fired = State->Model.NotifyTrigger(PixelSideAffixUE::ToLiteTrigger(Trigger));
	for (const std::string& Id : Fired)
	{
		OnAffixTrigger.Broadcast(Trigger, PixelSideAffixUE::IdName(Id));
	}
	if (!Fired.empty())
	{
		OnStatsRebuilt.Broadcast();
	}
}

void UAffixSubsystemDefault::ApplyChoicePause(bool bPaused)
{
	Super::ApplyChoicePause(bPaused);
	if (!bAutoPauseDuringChoice)
	{
		return;
	}
	if (UWorld* World = ResolveWorld())
	{
		UGameplayStatics::SetGamePaused(World, bPaused);
	}
}
