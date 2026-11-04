#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PixelRunSummaryWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UOverlay;
class UTextBlock;
class UWrapBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunSummaryContinueToHub);

/**
 * Wireframe states. Ready is not a separate screen: Open_Win and Open_Fail already have data,
 * and Btn_Continue is enabled. Leaving locks input, then OnContinueToHub fires.
 */
UENUM(BlueprintType)
enum class EPixelRunSummaryState : uint8
{
	Closed,
	Open_Win,
	Open_Fail,
	Leaving
};

/**
 * What WBP_RunSummary shows. The shell does not compute Core and does not write a save.
 * CoreGained is printed as given. ScrapRemain is the in-run remainder line, not Core.
 * AffixShortNames is optional and parallel to AffixIds. An empty label falls back to the id text.
 * This shell does not call GetAffixCardView, PreviewCoreGain, or AddCore.
 */
USTRUCT(BlueprintType)
struct PIXELSIDERL_API FRunSummaryView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	bool bVictory = false;

	/** Seconds. Displayed as MM:SS. Not a Core input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	int32 DurationSec = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	int32 KillCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	int32 FloorReached = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	int32 FloorMax = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	int32 BossKills = 0;

	/** In-run Scrap left on screen. The shell does not spend it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	int32 ScrapRemain = 0;

	/** Out-of-run Core amount to display. The shell does not grant it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	int32 CoreGained = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	TArray<FName> AffixIds;

	/** Short names for WB_Build. Demo uses the P1 wireframe copy. Not looked up from a table. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	TArray<FText> AffixShortNames;
};

/**
 * C++ parent for WBP_RunSummary.
 * Full-screen win / fail summary. Present fills FRunSummaryView. OpenWin and OpenFail fill the
 * P1 wireframe §2.2 placeholder numbers. Confirm (button, Enter, Space, Esc) broadcasts
 * OnContinueToHub once. Esc is the same confirm so the screen cannot trap the player.
 *
 * There is no run-summary pause API. Present / OpenWin / OpenFail default to
 * UGameplayStatics::SetGamePaused, same habit as OpenShopDemo. Pass bPause false when the
 * flow already paused or has already left the run. A world that is already paused is not
 * taken over, and confirm will not unpause it. An open affix choice refuses this shell so
 * the choice pause stays owned by the affix subsystem.
 *
 * This widget does not write Core, does not call PreviewCoreGain or AddCore, does not save,
 * does not change WBP_HUD, and does not spend RunScrap. No lab key: F6–F11 stay as they are,
 * and F12 is not bound. Non-shipping console: PixelSide.RunSummary.Demo Win|Fail.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_RunSummary"))
class PIXELSIDERL_API UPixelRunSummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelRunSummaryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Show the view. bPause defaults on until a summary pause API exists.
	 * Refuses while an affix choice is pending. Does not grant Core.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void Present(const FRunSummaryView& InView, bool bPause = true);

	/** P1 §2.2 win placeholder. Pauses like Present. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void OpenWin();

	/** P1 §2.2 fail placeholder, including the clear-out tip. Pauses like Present. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void OpenFail();

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	static FRunSummaryView MakeDemoWinView();

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	static FRunSummaryView MakeDemoFailView();

	/** Enter / Space / click / Esc. Idempotent. Broadcasts OnContinueToHub and does not travel. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void Confirm();

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsOpen() const;

	/** True while Open_Win or Open_Fail: data is in and Btn_Continue accepts input. */
	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsReady() const;

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	EPixelRunSummaryState GetSummaryState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	FRunSummaryView GetView() const { return View; }

	/** True only when this open called SetGamePaused. Confirm unpauses that case only. */
	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool DoesOwnPause() const { return bOwnsPause; }

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnRunSummaryContinueToHub OnContinueToHub;

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
	TObjectPtr<UTextBlock> Txt_Outcome;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Time;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Kills;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> WB_Build;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Floor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_BossKills;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_ScrapLeft;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_CoreGain;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_CoreTip;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Continue;

	/** Caption inside Btn_Continue. Fallback name; a WBP may bind it. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Continue;

	/** Key line. Not required by the wireframe control table. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Hint;

	/** _Dev preview only. Shipping hides them. Not a lab key. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_DemoWin;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_DemoFail;

private:
	void EnsureLayout();
	void BuildFallbackShell();
	void EnsureBuildChips();
	void BindInteractions();
	void ShowDesignWin();
	void Refresh();
	void RefreshBuild();

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleDemoWinClicked();

	UFUNCTION()
	void HandleDemoFailClicked();

	void BeginDemoPause();
	void EndDemoPause();
	void AcquireInput();
	void ReleaseInput();
	bool IsAffixChoicePending() const;

	UPROPERTY()
	FRunSummaryView View;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> BuildChipRoots;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> BuildChipLabels;

	UPROPERTY()
	TObjectPtr<UTextBlock> Txt_BuildEmpty;

	UPROPERTY()
	TObjectPtr<UTextBlock> Txt_BuildMore;

	EPixelRunSummaryState State = EPixelRunSummaryState::Closed;
	bool bOwnsPause = false;
	bool bCapturedInput = false;
	bool bBuiltFallbackShell = false;
	bool bWarnedMissingWidgets = false;
	bool bBuildPoolReady = false;
};
