#include "Meta/MetaProgressSubsystem.h"

#include "Affix/AffixSubsystemDefault.h"
#include "Event/EventSubsystem.h"
#include "Meta/MetaProgress.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Templates/Function.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelSideMeta, Log, All);

struct FMetaProgressState
{
	PixelSideMeta::FProgress Progress;
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
	if (Id.empty())
	{
		return NAME_None;
	}
	return FName(FromUtf8(Id));
}

bool ToStat(const std::string& Name, EStatId& Out)
{
	if (Name == "MaxHP") { Out = EStatId::MaxHP; return true; }
	if (Name == "MoveSpeed") { Out = EStatId::MoveSpeed; return true; }
	if (Name == "FireRate") { Out = EStatId::FireRate; return true; }
	if (Name == "Damage") { Out = EStatId::Damage; return true; }
	if (Name == "CritChance") { Out = EStatId::CritChance; return true; }
	if (Name == "CritMult") { Out = EStatId::CritMult; return true; }
	if (Name == "MagSize") { Out = EStatId::MagSize; return true; }
	if (Name == "ReloadSpeed") { Out = EStatId::ReloadSpeed; return true; }
	if (Name == "ProjectileCount") { Out = EStatId::ProjectileCount; return true; }
	if (Name == "Pierce") { Out = EStatId::Pierce; return true; }
	if (Name == "Knockback") { Out = EStatId::Knockback; return true; }
	if (Name == "PickupRadius") { Out = EStatId::PickupRadius; return true; }
	if (Name == "CoinGain") { Out = EStatId::CoinGain; return true; }
	if (Name == "ScrapGain") { Out = EStatId::ScrapGain; return true; }
	return false;
}

bool ToOp(const std::string& Name, EModifierOp& Out)
{
	if (Name == "Add") { Out = EModifierOp::Add; return true; }
	if (Name == "Mul") { Out = EModifierOp::Mul; return true; }
	if (Name == "Override") { Out = EModifierOp::Override; return true; }
	return false;
}

FRunStartContext ToContext(const PixelSideMeta::FRunStart& Start)
{
	FRunStartContext Context;
	Context.StartingScrap = Start.StartingScrap;
	Context.MetaCoreMult = static_cast<float>(Start.MetaCoreMult);
	Context.Seed = 1;
	for (const PixelSideMeta::FModifier& Mod : Start.PermanentModifiers)
	{
		FAffixModifier Row;
		if (!ToStat(Mod.Stat, Row.Stat) || !ToOp(Mod.Op, Row.Op))
		{
			UE_LOG(LogPixelSideMeta, Warning, TEXT("Dropped meta modifier %s %s"), *FromUtf8(Mod.Stat), *FromUtf8(Mod.Op));
			continue;
		}
		Row.Value = static_cast<float>(Mod.Value);
		Row.DurationSec = 0.f;
		Row.WeaponTag = IdName(Mod.WeaponTag);
		Context.PermanentModifiers.Add(Row);
	}
	for (const std::string& Id : Start.UnlockedAffixIds)
	{
		Context.UnlockedAffixIds.Add(IdName(Id));
	}
	for (const std::string& Flag : Start.Flags)
	{
		Context.MetaFlags.Add(IdName(Flag));
	}
	if (!Start.bAffixPoolReady)
	{
		UE_LOG(LogPixelSideMeta, Warning, TEXT("Affix pool missing. UnlockedAffixIds left empty (affix BeginRun treats empty as the loaded pool)."));
	}
	return Context;
}
}

UMetaProgressSubsystem* UMetaProgressSubsystem::Get(const UObject* WorldContext)
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
	return World->GetGameInstance()->GetSubsystem<UMetaProgressSubsystem>();
}

void UMetaProgressSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	State = new FMetaProgressState();
	ReloadFromContent();
}

void UMetaProgressSubsystem::Deinitialize()
{
	DiscardState();
	Super::Deinitialize();
}

void UMetaProgressSubsystem::BeginDestroy()
{
	DiscardState();
	Super::BeginDestroy();
}

void UMetaProgressSubsystem::DiscardState()
{
	delete State;
	State = nullptr;
}

void UMetaProgressSubsystem::ReloadFromContent()
{
	if (!State)
	{
		State = new FMetaProgressState();
	}
	const FString EconomyPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Meta/DA_Economy.json"));
	const FString NodesPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Meta/DT_MetaNodes_MVP.json"));
	const FString AffixPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Affixes/DT_Affix_MVP.json"));

	FString Text;
	std::string Error;
	if (!FFileHelper::LoadFileToString(Text, *EconomyPath) || !State->Progress.LoadEconomyJson(ToUtf8(Text), &Error))
	{
		UE_LOG(LogPixelSideMeta, Warning, TEXT("Economy JSON rejected %s: %s"), *EconomyPath, *FromUtf8(Error));
	}
	else
	{
		UE_LOG(LogPixelSideMeta, Log, TEXT("Loaded %s"), *EconomyPath);
	}
	if (!FFileHelper::LoadFileToString(Text, *NodesPath) || !State->Progress.LoadNodesJson(ToUtf8(Text), &Error))
	{
		UE_LOG(LogPixelSideMeta, Warning, TEXT("Meta nodes JSON rejected %s: %s"), *NodesPath, *FromUtf8(Error));
	}
	else
	{
		UE_LOG(LogPixelSideMeta, Log, TEXT("Loaded %s (%d, rule %s)"),
			*NodesPath,
			static_cast<int32>(State->Progress.Nodes().size()),
			*FromUtf8(State->Progress.NamingRuleText()));
		const std::vector<std::string> Ignored = State->Progress.IgnoredUnlockAffixIds();
		for (const std::string& Row : Ignored)
		{
			UE_LOG(LogPixelSideMeta, Warning, TEXT("UnlockAffix ignored (MVP pool is the ten, no Piercer/TwinShot gate): %s"), *FromUtf8(Row));
		}
	}
	if (!FFileHelper::LoadFileToString(Text, *AffixPath) || !State->Progress.LoadAffixPoolJson(ToUtf8(Text), &Error))
	{
		UE_LOG(LogPixelSideMeta, Warning, TEXT("Affix pool JSON rejected %s: %s"), *AffixPath, *FromUtf8(Error));
	}
	else
	{
		UE_LOG(LogPixelSideMeta, Log, TEXT("Meta affix pool %s (%d)"), *AffixPath, static_cast<int32>(State->Progress.AffixPool().size()));
	}
}

int32 UMetaProgressSubsystem::GetCore() const
{
	return State ? State->Progress.GetCore() : 0;
}

void UMetaProgressSubsystem::AddCore(int32 Delta, const FString& Reason)
{
	if (!State)
	{
		return;
	}
	State->Progress.AddCore(Delta, ToUtf8(Reason));
	UE_LOG(LogPixelSideMeta, Log, TEXT("AddCore %+d (%s) -> Core=%d"), Delta, *Reason, State->Progress.GetCore());
}

int32 UMetaProgressSubsystem::GetPurchasedRank(FName NodeId) const
{
	if (!State || NodeId.IsNone())
	{
		return 0;
	}
	return State->Progress.GetPurchasedRank(ToUtf8(NodeId.ToString()));
}

TMap<FName, int32> UMetaProgressSubsystem::GetPurchasedNodes() const
{
	TMap<FName, int32> Out;
	if (!State)
	{
		return Out;
	}
	for (const PixelSideMeta::FNode& Node : State->Progress.Nodes())
	{
		Out.Add(IdName(Node.NodeId), State->Progress.GetPurchasedRank(Node.NodeId));
	}
	return Out;
}

bool UMetaProgressSubsystem::CanPurchase(FName NodeId) const
{
	if (!State || NodeId.IsNone())
	{
		return false;
	}
	std::string Reason;
	return State->Progress.CanPurchase(ToUtf8(NodeId.ToString()), &Reason);
}

bool UMetaProgressSubsystem::PurchaseInternal(FName NodeId, FString* Reason)
{
	if (!State || NodeId.IsNone())
	{
		if (Reason)
		{
			*Reason = TEXT("unknown");
		}
		return false;
	}
	std::string HostReason;
	const bool bOk = State->Progress.Purchase(ToUtf8(NodeId.ToString()), &HostReason);
	if (Reason)
	{
		*Reason = FromUtf8(HostReason);
	}
	if (!bOk)
	{
		UE_LOG(LogPixelSideMeta, Warning, TEXT("Purchase %s failed: %s (Core=%d rank=%d)"),
			*NodeId.ToString(),
			*FromUtf8(HostReason),
			State->Progress.GetCore(),
			State->Progress.GetPurchasedRank(ToUtf8(NodeId.ToString())));
		return false;
	}
	UE_LOG(LogPixelSideMeta, Log, TEXT("Purchase %s -> rank %d Core=%d"),
		*NodeId.ToString(),
		State->Progress.GetPurchasedRank(ToUtf8(NodeId.ToString())),
		State->Progress.GetCore());
	return true;
}

bool UMetaProgressSubsystem::Purchase(FName NodeId)
{
	return PurchaseInternal(NodeId, nullptr);
}

FRunStartContext UMetaProgressSubsystem::BuildRunStartContext() const
{
	if (!State)
	{
		return FRunStartContext();
	}
	return ToContext(State->Progress.BuildRunStartContext());
}

FRunStartContext UMetaProgressSubsystem::BeginRunFromMeta()
{
	const FRunStartContext Context = BuildRunStartContext();
	UGameInstance* GameInstance = GetGameInstance();
	UEventSubsystem* Events = nullptr;
	if (GameInstance)
	{
		if (UAffixSubsystem* Affix = GameInstance->GetSubsystem<UAffixSubsystemDefault>())
		{
			Affix->BeginRun(Context);
		}
		else
		{
			UE_LOG(LogPixelSideMeta, Warning, TEXT("BeginRunFromMeta: no UAffixSubsystemDefault"));
		}
		Events = GameInstance->GetSubsystem<UEventSubsystem>();
		if (Events)
		{
			Events->ResetRunScrap(Context.StartingScrap);
		}
		else
		{
			UE_LOG(LogPixelSideMeta, Warning, TEXT("BeginRunFromMeta: no UEventSubsystem"));
		}
	}
	UE_LOG(LogPixelSideMeta, Log, TEXT("BeginRunFromMeta StartingScrap=%d RunScrap=%d Core=%d MetaCoreMult=%.4f Flags=%d Mods=%d Pool=%d"),
		Context.StartingScrap,
		Events ? Events->GetRunScrap() : -1,
		GetCore(),
		GetMetaCoreMult(),
		Context.MetaFlags.Num(),
		Context.PermanentModifiers.Num(),
		Context.UnlockedAffixIds.Num());
	return Context;
}

float UMetaProgressSubsystem::GetMetaCoreMult() const
{
	return State ? static_cast<float>(State->Progress.GetMetaCoreMult()) : 0.f;
}

int32 UMetaProgressSubsystem::PreviewCoreGain(bool bCleared, int32 RoomsCleared, int32 BossKilled, int32 RunScrapRemain) const
{
	if (!State)
	{
		return 0;
	}
	const PixelSideMeta::FCorePreview Preview = State->Progress.PreviewCoreGain(bCleared, RoomsCleared, BossKilled, RunScrapRemain);
	if (!Preview.bReady)
	{
		UE_LOG(LogPixelSideMeta, Warning, TEXT("PreviewCoreGain: economy table is not loaded"));
		return 0;
	}
	return Preview.Core;
}

bool UMetaProgressSubsystem::HasMetaFlag(FName Flag) const
{
	if (Flag.IsNone())
	{
		return false;
	}
	const FRunStartContext Context = BuildRunStartContext();
	return Context.MetaFlags.Contains(Flag);
}

void UMetaProgressSubsystem::ResetSlot()
{
	if (State)
	{
		State->Progress.ResetSlot();
	}
	UE_LOG(LogPixelSideMeta, Log, TEXT("ResetSlot Core=0 ranks cleared (memory only)"));
}

bool UMetaProgressSubsystem::Save()
{
	UE_LOG(LogPixelSideMeta, Log, TEXT("Save skipped: memory only, no Saved/SaveGames/PixelSideRL_Meta.sav"));
	return false;
}

bool UMetaProgressSubsystem::Load()
{
	UE_LOG(LogPixelSideMeta, Log, TEXT("Load skipped: memory only, current Core and ranks stay"));
	return false;
}

FString UMetaProgressSubsystem::DescribeRunStartContext() const
{
	if (!State)
	{
		return FString();
	}
	return FromUtf8(State->Progress.Describe());
}

#if !UE_BUILD_SHIPPING
static int32 VisitMeta(TFunctionRef<void(UMetaProgressSubsystem*)> Visit)
{
	int32 Ran = 0;
	if (!GEngine)
	{
		return 0;
	}
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* World = Context.World();
		if (!World || !World->IsGameWorld() || !World->GetGameInstance())
		{
			continue;
		}
		if (UMetaProgressSubsystem* Meta = World->GetGameInstance()->GetSubsystem<UMetaProgressSubsystem>())
		{
			Visit(Meta);
			++Ran;
		}
	}
	return Ran;
}

static void PixelSideMetaAddCoreCmd(const TArray<FString>& Args)
{
	const int32 Delta = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 100;
	const int32 Ran = VisitMeta([Delta](UMetaProgressSubsystem* Meta)
	{
		Meta->AddCore(Delta, TEXT("Debug"));
	});
	if (Ran == 0)
	{
		UE_LOG(LogPixelSideMeta, Warning, TEXT("PixelSide.Meta.AddCore: no game world."));
	}
}

static void PixelSideMetaPurchaseCmd(const TArray<FString>& Args)
{
	const FName NodeId(*(Args.Num() > 0 ? Args[0] : FString(TEXT("Stash"))));
	const int32 Ran = VisitMeta([NodeId](UMetaProgressSubsystem* Meta)
	{
		Meta->Purchase(NodeId);
	});
	if (Ran == 0)
	{
		UE_LOG(LogPixelSideMeta, Warning, TEXT("PixelSide.Meta.Purchase: no game world."));
	}
}

static void PixelSideMetaContextCmd(const TArray<FString>& Args)
{
	(void)Args;
	const int32 Ran = VisitMeta([](UMetaProgressSubsystem* Meta)
	{
		UE_LOG(LogPixelSideMeta, Log, TEXT("%s"), *Meta->DescribeRunStartContext());
	});
	if (Ran == 0)
	{
		UE_LOG(LogPixelSideMeta, Warning, TEXT("PixelSide.Meta.Context: no game world."));
	}
}

static void PixelSideMetaBeginRunCmd(const TArray<FString>& Args)
{
	(void)Args;
	const int32 Ran = VisitMeta([](UMetaProgressSubsystem* Meta)
	{
		Meta->BeginRunFromMeta();
	});
	if (Ran == 0)
	{
		UE_LOG(LogPixelSideMeta, Warning, TEXT("PixelSide.Meta.BeginRun: no game world."));
	}
}

static FAutoConsoleCommand GPixelSideMetaAddCore(
	TEXT("PixelSide.Meta.AddCore"),
	TEXT("PixelSide.Meta.AddCore <Delta=100>. Non-shipping. Adds out-of-run Core. Does not change RunScrap."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&PixelSideMetaAddCoreCmd));

static FAutoConsoleCommand GPixelSideMetaPurchase(
	TEXT("PixelSide.Meta.Purchase"),
	TEXT("PixelSide.Meta.Purchase <NodeId=Stash>. Non-shipping. Spends Core and raises that node one rank."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&PixelSideMetaPurchaseCmd));

static FAutoConsoleCommand GPixelSideMetaContext(
	TEXT("PixelSide.Meta.Context"),
	TEXT("PixelSide.Meta.Context. Non-shipping. Prints BuildRunStartContext (Core, ranks, StartingScrap, modifiers, pool)."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&PixelSideMetaContextCmd));

static FAutoConsoleCommand GPixelSideMetaBeginRun(
	TEXT("PixelSide.Meta.BeginRun"),
	TEXT("PixelSide.Meta.BeginRun. Non-shipping. BeginRunFromMeta and logs StartingScrap plus RunScrap. Does not rebind F6-F11."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&PixelSideMetaBeginRunCmd));
#endif
