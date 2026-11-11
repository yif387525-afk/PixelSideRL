#include "Flow/PixelHubRunFlowSubsystem.h"

#include "Affix/AffixSubsystemDefault.h"
#include "Combat/DamageableComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Event/EventSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Meta/MetaProgressSubsystem.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Rooms/PixelSideRoomFlow.h"
#include "UI/PixelHubWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelHubRun, Log, All);

namespace PixelHubRunPrivate
{
bool FStringMatches(const FString& Name, bool (*Matches)(std::string_view))
{
	if (Name.IsEmpty() || Matches == nullptr)
	{
		return false;
	}
	const std::string Utf8(TCHAR_TO_UTF8(*Name));
	return Matches(Utf8);
}

bool WorldMatches(const UObject* WorldContext, const FString& StartupMapName, bool (*Matches)(std::string_view))
{
	if (FStringMatches(StartupMapName, Matches))
	{
		return true;
	}
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}
	if (FStringMatches(World->GetName(), Matches))
	{
		return true;
	}
	if (const ULevel* Level = World->PersistentLevel)
	{
		if (FStringMatches(World->GetMapName(), Matches))
		{
			return true;
		}
		if (const UObject* Package = Level->GetOutermost())
		{
			return FStringMatches(Package->GetName(), Matches);
		}
	}
	return false;
}

bool MapPackageExists(const char* PackageUtf8)
{
	if (PackageUtf8 == nullptr || PackageUtf8[0] == '\0')
	{
		return false;
	}
	const FString Package(UTF8_TO_TCHAR(PackageUtf8));
	if (FPackageName::DoesPackageExist(Package))
	{
		return true;
	}
	if (!FPackageName::IsValidLongPackageName(Package))
	{
		return false;
	}
	const FString Filename = FPackageName::LongPackageNameToFilename(Package, FPackageName::GetMapPackageExtension());
	return FPaths::FileExists(Filename);
}

void UnpauseForTravel(UWorld* World)
{
	if (!World || !World->IsPaused())
	{
		return;
	}
	UE_LOG(LogPixelHubRun, Log, TEXT("Unpausing before level travel."));
	UGameplayStatics::SetGamePaused(World, false);
}

void HoldWorldPaused(UWorld* World)
{
	if (!World || World->bIsTearingDown || World->IsPaused())
	{
		return;
	}
	UGameplayStatics::SetGamePaused(World, true);
}

// Core ticker, not the world timer, so this still elapses while the hub or summary pause is held.
// Non-zero so a ticker added from FinishStart or Confirm cannot travel in that same callback.
constexpr float TravelDelaySec = 0.05f;
}

UPixelHubRunFlowSubsystem* UPixelHubRunFlowSubsystem::Get(const UObject* WorldContext)
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
	return World->GetGameInstance()->GetSubsystem<UPixelHubRunFlowSubsystem>();
}

bool UPixelHubRunFlowSubsystem::IsHubMap(const UObject* WorldContext, const FString& StartupMapName)
{
	return PixelHubRunPrivate::WorldMatches(WorldContext, StartupMapName, &PixelSideRoom::MapNameNamesHub);
}

bool UPixelHubRunFlowSubsystem::IsRoomCombat01Map(const UObject* WorldContext, const FString& StartupMapName)
{
	return PixelHubRunPrivate::WorldMatches(WorldContext, StartupMapName, &PixelSideRoom::MapNameNamesRoomCombat01);
}

void UPixelHubRunFlowSubsystem::Deinitialize()
{
	CancelHubOpenRetry();
	CancelHealthBindRetry();
	CancelTravel();
	UnbindHealth();
	if (UPixelHubWidget* Hub = HubWidget.Get())
	{
		if (IsValid(Hub))
		{
			Hub->OnStartRun.RemoveDynamic(this, &UPixelHubRunFlowSubsystem::HandleHubStartRun);
		}
	}
	if (UPixelRunSummaryWidget* Summary = SummaryWidget.Get())
	{
		if (IsValid(Summary))
		{
			Summary->OnContinueToHub.RemoveDynamic(this, &UPixelHubRunFlowSubsystem::HandleSummaryContinue);
		}
	}
	HubWidget = nullptr;
	SummaryWidget = nullptr;
	Super::Deinitialize();
}

void UPixelHubRunFlowSubsystem::HandleHubMapBegan()
{
	bStartTravelInFlight = false;
	bReturnTravelInFlight = false;
	bReplaySummary = false;
	bSummaryShown = false;
	HubOpenAttempts = 0;
	CancelHealthBindRetry();
	UnbindHealth();
	CancelTravel();
	UE_LOG(LogPixelHubRun, Log, TEXT("Hub map. OpenHub. Lab sandbox stays out. No director."));
	TryOpenHub();
}

void UPixelHubRunFlowSubsystem::HandleCombatMapBegan()
{
	const bool bArrivedFromHub = bStartTravelInFlight;
	bStartTravelInFlight = false;
	bReturnTravelInFlight = false;
	bReplaySummary = false;
	bSummaryShown = false;
	CancelHubOpenRetry();
	CancelTravel();
	if (bArrivedFromHub)
	{
		DismissHubWidget();
		UE_LOG(LogPixelHubRun, Log, TEXT("Entered Room_Combat_01 from Hub. Hub UI is closed."));
	}
	else
	{
		HubWidget = nullptr;
		UE_LOG(LogPixelHubRun, Log, TEXT("Room_Combat_01 Play did not come from Hub. Hub UI is not opened."));
	}
	HealthBindAttempts = 0;
	BindCombatHealth();
}

void UPixelHubRunFlowSubsystem::RequestEndRun(bool bVictory)
{
	if (bSummaryShown)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}
	if (!IsRoomCombat01Map(this, FString()))
	{
		UE_LOG(LogPixelHubRun, Warning, TEXT("Run end ignored. Current map is not Room_Combat_01. Hub was not opened."));
		return;
	}
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		UE_LOG(LogPixelHubRun, Warning, TEXT("Run end has no player controller. Summary was not opened."));
		return;
	}

	UPixelRunSummaryWidget* Summary = SummaryWidget.Get();
	if (!IsValid(Summary) || Summary->GetWorld() != World)
	{
		Summary = CreateWidget<UPixelRunSummaryWidget>(PC, UPixelRunSummaryWidget::StaticClass());
		SummaryWidget = Summary;
	}
	if (!Summary)
	{
		UE_LOG(LogPixelHubRun, Warning, TEXT("Run end could not create WBP_RunSummary."));
		return;
	}

	bSummaryShown = true;
	Summary->OnContinueToHub.RemoveDynamic(this, &UPixelHubRunFlowSubsystem::HandleSummaryContinue);
	Summary->OnContinueToHub.AddDynamic(this, &UPixelHubRunFlowSubsystem::HandleSummaryContinue);

	FRunSummaryView View;
	View.bVictory = bVictory;
	View.DurationSec = FMath::Max(0, FMath::FloorToInt(World->GetTimeSeconds()));
	View.KillCount = 0;
	View.FloorReached = 1;
	View.FloorMax = 1;
	View.BossKills = 0;
	View.CoreGained = 0;
	View.ScrapRemain = 0;
	if (const UEventSubsystem* Events = UEventSubsystem::Get(World))
	{
		View.ScrapRemain = Events->GetRunScrap();
	}
	if (const UAffixSubsystemDefault* Affix = UAffixSubsystemDefault::Get(World))
	{
		const TArray<FAffixRuntimeEntry> Owned = Affix->GetOwnedAffixes();
		for (const FAffixRuntimeEntry& OwnedEntry : Owned)
		{
			if (!OwnedEntry.AffixId.IsNone())
			{
				View.AffixIds.Add(OwnedEntry.AffixId);
			}
		}
	}
	LastSummary = View;
	bHaveLastSummary = true;
	Summary->Present(View, true);
	if (!Summary->IsOpen())
	{
		bSummaryShown = false;
		Summary->OnContinueToHub.RemoveDynamic(this, &UPixelHubRunFlowSubsystem::HandleSummaryContinue);
		UE_LOG(LogPixelHubRun, Warning, TEXT("WBP_RunSummary did not open. An affix choice may still be up. Core was not written."));
		return;
	}
	UE_LOG(LogPixelHubRun, Log, TEXT("Run end %s. CoreGained=0. AddCore was not called. Confirm travels to Hub."),
		bVictory ? TEXT("victory") : TEXT("defeat"));
}

void UPixelHubRunFlowSubsystem::TryOpenHub()
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || !IsHubMap(this, FString()))
	{
		return;
	}
	if (const UAffixSubsystemDefault* Affix = UAffixSubsystemDefault::Get(this))
	{
		if (Affix->IsChoicePending())
		{
			UE_LOG(LogPixelHubRun, Warning, TEXT("Hub map did not open WBP_Hub: an affix choice is pending."));
			return;
		}
	}
	if (IsValid(HubWidget) && HubWidget->IsHubOpen() && HubWidget->GetWorld() == World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->IsLocalPlayerController())
	{
		ScheduleHubOpenRetry();
		return;
	}

	UPixelHubWidget* Opened = UPixelHubWidget::OpenForPlayer(PC, false, nullptr);
	if (!Opened)
	{
		UE_LOG(LogPixelHubRun, Warning, TEXT("Hub map OpenHub returned no widget."));
		return;
	}
	HubWidget = Opened;
	Opened->OnStartRun.RemoveDynamic(this, &UPixelHubRunFlowSubsystem::HandleHubStartRun);
	Opened->OnStartRun.AddDynamic(this, &UPixelHubRunFlowSubsystem::HandleHubStartRun);
	if (bHaveLastSummary)
	{
		Opened->SetLastRunSummary(LastSummary);
	}
	UE_LOG(LogPixelHubRun, Log, TEXT("Hub OpenHub on map enter. Start broadcasts OnStartRun; this flow calls BeginRunFromMeta once, then travels."));
}

void UPixelHubRunFlowSubsystem::ScheduleHubOpenRetry()
{
	if (HubOpenRetryTicker.IsValid())
	{
		return;
	}
	if (HubOpenAttempts >= 30)
	{
		UE_LOG(LogPixelHubRun, Warning, TEXT("Hub map gave up waiting for a local player controller. OpenHub was not called."));
		return;
	}
	++HubOpenAttempts;
	HubOpenRetryTicker = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPixelHubRunFlowSubsystem::FinishHubOpenRetry), 0.f);
}

bool UPixelHubRunFlowSubsystem::FinishHubOpenRetry(float DeltaSeconds)
{
	(void)DeltaSeconds;
	HubOpenRetryTicker.Reset();
	TryOpenHub();
	return false;
}

void UPixelHubRunFlowSubsystem::CancelHubOpenRetry()
{
	if (HubOpenRetryTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HubOpenRetryTicker);
		HubOpenRetryTicker.Reset();
	}
}

void UPixelHubRunFlowSubsystem::BindCombatHealth()
{
	UnbindHealth();
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || !IsRoomCombat01Map(this, FString()))
	{
		return;
	}
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
	UDamageableComponent* Damageable = Pawn ? Pawn->FindComponentByClass<UDamageableComponent>() : nullptr;
	if (!Damageable)
	{
		ScheduleHealthBindRetry();
		return;
	}
	Damageable->OnHealthChanged.RemoveDynamic(this, &UPixelHubRunFlowSubsystem::HandlePlayerHealthChanged);
	Damageable->OnHealthChanged.AddDynamic(this, &UPixelHubRunFlowSubsystem::HandlePlayerHealthChanged);
	BoundHealth = Damageable;
	UE_LOG(LogPixelHubRun, Log, TEXT("Room_Combat_01 is listening for player HP <= 0."));
	if (Damageable->GetCurrentHealth() <= 0.f)
	{
		RequestEndRun(false);
	}
}

void UPixelHubRunFlowSubsystem::ScheduleHealthBindRetry()
{
	if (HealthBindTicker.IsValid())
	{
		return;
	}
	if (HealthBindAttempts >= 30)
	{
		UE_LOG(LogPixelHubRun, Warning, TEXT("Room_Combat_01 gave up waiting for the player damageable. Death will not open the summary."));
		return;
	}
	++HealthBindAttempts;
	HealthBindTicker = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPixelHubRunFlowSubsystem::FinishHealthBindRetry), 0.f);
}

bool UPixelHubRunFlowSubsystem::FinishHealthBindRetry(float DeltaSeconds)
{
	(void)DeltaSeconds;
	HealthBindTicker.Reset();
	BindCombatHealth();
	return false;
}

void UPixelHubRunFlowSubsystem::CancelHealthBindRetry()
{
	if (HealthBindTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HealthBindTicker);
		HealthBindTicker.Reset();
	}
}

void UPixelHubRunFlowSubsystem::UnbindHealth()
{
	if (UDamageableComponent* Damageable = BoundHealth.Get())
	{
		if (IsValid(Damageable))
		{
			Damageable->OnHealthChanged.RemoveDynamic(this, &UPixelHubRunFlowSubsystem::HandlePlayerHealthChanged);
		}
	}
	BoundHealth = nullptr;
}

void UPixelHubRunFlowSubsystem::DismissHubWidget()
{
	if (UPixelHubWidget* Hub = HubWidget.Get())
	{
		if (IsValid(Hub))
		{
			Hub->OnStartRun.RemoveDynamic(this, &UPixelHubRunFlowSubsystem::HandleHubStartRun);
			if (Hub->IsHubOpen())
			{
				Hub->CloseHub();
			}
		}
	}
	HubWidget = nullptr;
}

void UPixelHubRunFlowSubsystem::HandleHubStartRun()
{
	if (bStartTravelInFlight || bReturnTravelInFlight)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || !IsHubMap(this, FString()))
	{
		UE_LOG(LogPixelHubRun, Warning, TEXT("OnStartRun ignored. This flow only travels from the Hub map."));
		return;
	}
	if (!PixelHubRunPrivate::MapPackageExists(PixelSideRoom::RoomCombat01MapPackage))
	{
		UE_LOG(LogPixelHubRun, Error, TEXT("Room_Combat_01 package is missing (%s). Hub stays open. BeginRunFromMeta was not called."),
			UTF8_TO_TCHAR(PixelSideRoom::RoomCombat01MapPackage));
		return;
	}
	bStartTravelInFlight = true;
	ArmTravel();
}

void UPixelHubRunFlowSubsystem::HandleSummaryContinue()
{
	if (bReturnTravelInFlight || bStartTravelInFlight)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}
	if (!PixelHubRunPrivate::MapPackageExists(PixelSideRoom::HubMapPackage))
	{
		UE_LOG(LogPixelHubRun, Error, TEXT("Hub package is missing (%s). Run Tools/BakeHub.py in the editor. Summary stays up. Core was not written."),
			UTF8_TO_TCHAR(PixelSideRoom::HubMapPackage));
		// Outside this confirm click, so the summary can open again. Hold pause so the pawn cannot run off.
		bSummaryShown = false;
		bReplaySummary = true;
		PixelHubRunPrivate::HoldWorldPaused(World);
		ArmTravel();
		return;
	}
	bReturnTravelInFlight = true;
	PixelHubRunPrivate::HoldWorldPaused(World);
	ArmTravel();
}

void UPixelHubRunFlowSubsystem::HandlePlayerHealthChanged(float CurrentHealth, float MaxHealth)
{
	(void)MaxHealth;
	if (CurrentHealth > 0.f)
	{
		return;
	}
	RequestEndRun(false);
}

void UPixelHubRunFlowSubsystem::ArmTravel()
{
	if (TravelTicker.IsValid())
	{
		return;
	}
	// After the hub lock or the summary confirm returns. Core ticker, so a pause does not block it.
	TravelTicker = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPixelHubRunFlowSubsystem::FinishTravel), PixelHubRunPrivate::TravelDelaySec);
}

bool UPixelHubRunFlowSubsystem::FinishTravel(float DeltaSeconds)
{
	(void)DeltaSeconds;
	TravelTicker.Reset();
	if (bReplaySummary)
	{
		bReplaySummary = false;
		RequestEndRun(LastSummary.bVictory);
		return false;
	}
	if (bReturnTravelInFlight)
	{
		TravelToHub();
		return false;
	}
	if (bStartTravelInFlight)
	{
		TravelToCombat();
		return false;
	}
	return false;
}

void UPixelHubRunFlowSubsystem::CancelTravel()
{
	if (TravelTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TravelTicker);
		TravelTicker.Reset();
	}
}

void UPixelHubRunFlowSubsystem::TravelToCombat()
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || !IsHubMap(this, FString()))
	{
		bStartTravelInFlight = false;
		return;
	}
	DismissHubWidget();
	PixelHubRunPrivate::UnpauseForTravel(World);
	if (UMetaProgressSubsystem* Meta = UMetaProgressSubsystem::Get(this))
	{
		const FRunStartContext Started = Meta->BeginRunFromMeta();
		UE_LOG(LogPixelHubRun, Log, TEXT("Hub start called BeginRunFromMeta once. StartingScrap=%d. Travel %s."),
			Started.StartingScrap,
			UTF8_TO_TCHAR(PixelSideRoom::RoomCombat01MapPackage));
	}
	else
	{
		UE_LOG(LogPixelHubRun, Warning, TEXT("Hub start has no UMetaProgressSubsystem. Travel %s. The player BeginAffixRun starts an empty run."),
			UTF8_TO_TCHAR(PixelSideRoom::RoomCombat01MapPackage));
	}
	UGameplayStatics::OpenLevel(World, FName(UTF8_TO_TCHAR(PixelSideRoom::RoomCombat01MapPackage)));
}

void UPixelHubRunFlowSubsystem::TravelToHub()
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		bReturnTravelInFlight = false;
		return;
	}
	PixelHubRunPrivate::UnpauseForTravel(World);
	UE_LOG(LogPixelHubRun, Log, TEXT("RunSummary confirm. Travel %s and OpenHub. Core was not written."),
		UTF8_TO_TCHAR(PixelSideRoom::HubMapPackage));
	UGameplayStatics::OpenLevel(World, FName(UTF8_TO_TCHAR(PixelSideRoom::HubMapPackage)));
}

#if !UE_BUILD_SHIPPING
static void PixelSideRunEndCmd(const TArray<FString>& Args)
{
	const bool bVictory = !(Args.Num() > 0 && Args[0].Equals(TEXT("Fail"), ESearchCase::IgnoreCase));
	int32 Found = 0;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || !World->IsGameWorld())
			{
				continue;
			}
			if (!UPixelHubRunFlowSubsystem::IsRoomCombat01Map(World, FString()))
			{
				continue;
			}
			if (UPixelHubRunFlowSubsystem* Flow = UPixelHubRunFlowSubsystem::Get(World))
			{
				Flow->RequestEndRun(bVictory);
				++Found;
			}
			break;
		}
	}
	if (Found == 0)
	{
		UE_LOG(LogPixelHubRun, Warning, TEXT("PixelSide.Run.End: not on Room_Combat_01. No key is bound."));
	}
}

static FAutoConsoleCommand GPixelSideRunEnd(
	TEXT("PixelSide.Run.End"),
	TEXT("PixelSide.Run.End [Fail]. Non-shipping. On Room_Combat_01, presents WBP_RunSummary (victory, or Fail for defeat). Confirm travels to Hub. Does not write Core. No key. F6-F11 are unchanged."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&PixelSideRunEndCmd));
#endif
