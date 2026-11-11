#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/PixelRunSummaryWidget.h"
#include "PixelHubRunFlowSubsystem.generated.h"

class UDamageableComponent;
class UPixelHubWidget;

/**
 * P0.8 Hub one-click loop. Lives on the game instance so the in-run flag set by
 * BeginRunFromMeta survives the travel into Room_Combat_01.
 *
 * Hub map enter calls OpenHub. After WBP_Hub's short start lock, OnStartRun calls
 * UMetaProgressSubsystem::BeginRunFromMeta once and travels to Room_Combat_01.
 * The hub widget itself still does not call BeginRunFromMeta and does not travel.
 * Player BeginPlay still calls BeginAffixRun, which skips when IsInRun() is already true.
 *
 * Direct Play of Room_Combat_01 does not open Hub. HP <= 0, or RequestEndRun, presents
 * WBP_RunSummary. Confirm travels back to Hub and the next Hub map enter calls OpenHub.
 * CoreGained stays 0. This type does not call AddCore, PreviewCoreGain, or Save.
 *
 * One room only. No second floor, no wave changes, no feel-table edits, no new meta API.
 * Does not bind a key. F6-F11 stay as they are. Does not call EnsureSideSun and does not
 * change auto exposure. Official Room_Combat_01 lighting stays on APixelSideGameMode.
 */
UCLASS()
class PIXELSIDERL_API UPixelHubRunFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "PixelSide|Run", meta = (WorldContext = "WorldContext"))
	static UPixelHubRunFlowSubsystem* Get(const UObject* WorldContext);

	/** Map name, PIE name, or package path names /Game/Maps/Hub. */
	static bool IsHubMap(const UObject* WorldContext, const FString& StartupMapName);

	/** Map name, PIE name, or package path names official Room_Combat_01. */
	static bool IsRoomCombat01Map(const UObject* WorldContext, const FString& StartupMapName);

	virtual void Deinitialize() override;

	/** Hub map BeginPlay. Opens WBP_Hub. Does not spawn a director and does not lay out the Lab. */
	void HandleHubMapBegan();

	/**
	 * Official Room_Combat_01 map BeginPlay. Does not open Hub.
	 * Closes a hub widget only when this travel was the Hub start button.
	 */
	void HandleCombatMapBegan();

	/**
	 * Explicit end while Room_Combat_01 is loaded. Presents WBP_RunSummary.
	 * bVictory false is the defeat copy; true is the other end path.
	 * Does not grant Core. No key.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|Run")
	void RequestEndRun(bool bVictory);

private:
	void TryOpenHub();
	void ScheduleHubOpenRetry();
	bool FinishHubOpenRetry(float DeltaSeconds);
	void CancelHubOpenRetry();

	void BindCombatHealth();
	void ScheduleHealthBindRetry();
	bool FinishHealthBindRetry(float DeltaSeconds);
	void CancelHealthBindRetry();
	void UnbindHealth();

	void DismissHubWidget();
	void TravelToCombat();
	void TravelToHub();
	void ArmTravel();

	UFUNCTION()
	void HandleHubStartRun();

	UFUNCTION()
	void HandleSummaryContinue();

	UFUNCTION()
	void HandlePlayerHealthChanged(float CurrentHealth, float MaxHealth);

	bool FinishTravel(float DeltaSeconds);
	void CancelTravel();

	UPROPERTY()
	TObjectPtr<UPixelHubWidget> HubWidget;

	UPROPERTY()
	TObjectPtr<UPixelRunSummaryWidget> SummaryWidget;

	UPROPERTY()
	FRunSummaryView LastSummary;

	TWeakObjectPtr<UDamageableComponent> BoundHealth;
	FTSTicker::FDelegateHandle HubOpenRetryTicker;
	FTSTicker::FDelegateHandle HealthBindTicker;
	FTSTicker::FDelegateHandle TravelTicker;

	int32 HubOpenAttempts = 0;
	int32 HealthBindAttempts = 0;
	bool bHaveLastSummary = false;
	bool bSummaryShown = false;
	bool bStartTravelInFlight = false;
	bool bReturnTravelInFlight = false;
	bool bReplaySummary = false;
};
