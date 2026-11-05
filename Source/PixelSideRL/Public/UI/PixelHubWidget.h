#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Delegates/IDelegateInstance.h"
#include "UI/PixelHubUpgradeWidget.h"
#include "UI/PixelRunSummaryWidget.h"
#include "PixelHubWidget.generated.h"

class UButton;
class UImage;
class UMetaProgressSubsystem;
class UOverlay;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHubStartRun);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHubOpenUpgrades);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHubBackToMenu);

/**
 * Wireframe states. NoSave still accepts every button: Core may be 0 and the run can start.
 * Starting locks input briefly so a double click cannot fire OnStartRun twice.
 */
UENUM(BlueprintType)
enum class EPixelHubState : uint8
{
	Closed,
	Idle,
	Starting,
	NoSave
};

/**
 * C++ parent for WBP_Hub.
 * Title 据点, Core label in #7EC8FF, and three buttons: 开始新一局 / 永久成长 / 返回标题.
 * 上次结算 is hidden until SetLastRunSummary. Opening it only presents WBP_RunSummary.
 *
 * OpenHub uses UMetaProgressSubsystem when it exists: GetCore, and upgrades call Purchase.
 * OpenHubDemo, and OpenHub with no subsystem, keep a local DemoCore of 120. That path does not
 * call GetCore, Purchase, AddCore, Save, or Load, and it does not write a save file.
 *
 * There is no hub pause API. Open defaults to SetGamePaused, same habit as OpenShopDemo.
 * Pass bPause false when the caller already paused. A world that is already paused is not
 * taken over, and close will not unpause it. An open affix choice refuses this shell.
 *
 * No lab key. F6–F11 stay as they are. Non-shipping console: PixelSide.Hub.Demo and PixelSide.Hub.Open.
 * OnStartRun does not call BeginRunFromMeta. OnBackToMenu does not travel.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_Hub"))
class PIXELSIDERL_API UPixelHubWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelHubWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Live wallet when UMetaProgressSubsystem is present. Otherwise the local DemoCore 120 stub.
	 * bPause defaults on. Refuses while an affix choice is pending.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void OpenHub(bool bPause = true);

	/**
	 * Local DemoCore 120 and wireframe §2.3 nodes even if Meta exists.
	 * Does not call GetCore, Purchase, AddCore, or Save.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void OpenHubDemo(bool bPause = true);

	/** Hide. Unpauses only a pause this open took. Idempotent. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void CloseHub();

	/**
	 * Create the shell on a local player and open it.
	 * bDemo false calls OpenHub. bDemo true calls OpenHubDemo.
	 * WidgetClass null uses this native class. No lab key.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	static UPixelHubWidget* OpenForPlayer(
		APlayerController* OwningPlayer,
		bool bDemo,
		TSubclassOf<UPixelHubWidget> WidgetClass);

	/** Shows 上次结算. The view is displayed later by WBP_RunSummary. Core is not granted here. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetLastRunSummary(const FRunSummaryView& InView);

	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void ClearLastRunSummary();

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsHubOpen() const;

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsUpgradeOpen() const;

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsDemoWallet() const { return bDemoWallet; }

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	int32 GetShownCore() const { return ShownCore; }

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	EPixelHubState GetHubState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool HasLastRunSummary() const { return bHasLastSummary; }

	/** True only when this open called SetGamePaused. Close unpauses that case only. */
	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool DoesOwnPause() const { return bOwnsPause; }

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnHubStartRun OnStartRun;

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnHubOpenUpgrades OnOpenUpgrades;

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnHubBackToMenu OnBackToMenu;

protected:
	virtual void NativePreConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UOverlay> Overlay_Root;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Dim;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Title;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Core;

	/** Optional mark from the wireframe ASCII. Not a required control. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Mark;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_StartRun;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_StartRun;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Upgrades;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Upgrades;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_LastSummary;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_LastSummary;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_MainMenu;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_MainMenu;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPixelHubUpgradeWidget> Panel_Upgrade;

private:
	void OpenInternal(bool bForceDemo, bool bPause);
	void EnsureLayout();
	void BuildFallbackShell();
	void BindButtons();
	void BindUpgrade();
	void RefreshChrome();
	void ShowDesign();
	bool CanClickHub() const;
	bool IsLiveEmpty(const UMetaProgressSubsystem* Meta) const;
	void ApplyCoreLabel();
	void CancelStartLock();
	bool FinishStart(float DeltaSeconds);
	void BeginPause();
	void EndPause();
	void AcquireInput();
	void ReleaseInput();
	bool IsAffixChoicePending() const;

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleUpgradesClicked();

	UFUNCTION()
	void HandleLastSummaryClicked();

	UFUNCTION()
	void HandleMenuClicked();

	UFUNCTION()
	void HandleUpgradeClosed();

	UFUNCTION()
	void HandleWalletChanged(int32 Core);

	UFUNCTION()
	void HandleSummaryContinue();

	UPROPERTY()
	TObjectPtr<UPixelRunSummaryWidget> SummaryWidget;

	UPROPERTY()
	FRunSummaryView LastSummary;

	FDelegateHandle StartTicker;
	EPixelHubState State = EPixelHubState::Closed;
	int32 ShownCore = 0;
	bool bDemoWallet = false;
	bool bHasLastSummary = false;
	bool bOwnsPause = false;
	bool bCapturedInput = false;
	bool bButtonsBound = false;
	bool bUpgradeBound = false;
	bool bBuiltFallbackShell = false;
	bool bWarnedMissingWidgets = false;
};
