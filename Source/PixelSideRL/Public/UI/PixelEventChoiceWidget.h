#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/PixelEventChoiceButton.h"
#include "PixelEventChoiceWidget.generated.h"

class UButton;
class UImage;
class UOverlay;
class UTextBlock;
class UVerticalBox;

DECLARE_LOG_CATEGORY_EXTERN(LogPixelEventUI, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPixelEventChoiceSelected, FName, ChoiceId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPixelEventSkipped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPixelEventClosed);

/**
 * Display row for one Choices[] entry. Label only.
 * Effects (GrantScrap / DamageSelf / HealSelf / None) are not stored and not executed.
 * Room / flow reads the DA JSON and applies the Op whitelist later.
 */
USTRUCT(BlueprintType)
struct FPixelEventChoiceOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FName ChoiceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FText Label;
};

/**
 * What WBP_EventChoice shows. Demo fills this from DA_Event_AbandonedCache strings.
 * This is not a DataAsset uasset.
 */
USTRUCT(BlueprintType)
struct FPixelEventChoiceView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FName EventId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FText PromptBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FText SkipHint;

	/** GDD: doors stay usable. This widget does not lock exits and does not SetGamePaused. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	bool bAllowDoorSkip = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	bool bCombatAllowed = false;

	/** MVP stays false. Confirming a choice never opens WBP_AffixChoice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	bool bAffixOnResolve = false;

	/** 1–3. Extra entries are ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	TArray<FPixelEventChoiceOption> Choices;
};

/**
 * C++ parent for WBP_EventChoice.
 * Shows DisplayName, PromptBody, up to three Choices[].Label buttons, and Btn_Skip (SkipHint).
 * Broadcasts OnEventChoiceSelected / OnEventSkipped / OnEventClosed.
 * Does not GrantScrap, DamageSelf, HealSelf, or open AffixChoice.
 *
 * Open / Close: calls APixelSideCharacter::SetEventChoiceBlocksControl.
 * That ignores move, jump, and fire only. The world is not paused, so door overlaps keep ticking.
 * bAllowDoorSkip stays on the view (AbandonedCache is true). Real Room_Event_01 integration
 * still leaves exits unlocked — GDD §2.8 「门仍可走」. Lab has no door actor; the modal blocks move/fire.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_EventChoice"))
class PIXELSIDERL_API UPixelEventChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelEventChoiceWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Fill the shell and show it. Does not apply effects and does not SetGamePaused. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void Present(const FPixelEventChoiceView& View);

	/**
	 * Lab / Demo view. Strings are the locked DA_Event_AbandonedCache copy
	 * (AbandonedCache, CarefulLoot, ForceLoot, SkipHint). Not a DataAsset uasset.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void PresentAbandonedCacheDemo();

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsOpen() const { return bOpen; }

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnPixelEventChoiceSelected OnEventChoiceSelected;

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnPixelEventSkipped OnEventSkipped;

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnPixelEventClosed OnEventClosed;

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
	TObjectPtr<UTextBlock> Txt_Body;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPixelEventChoiceButton> Btn_Choice_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPixelEventChoiceButton> Btn_Choice_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPixelEventChoiceButton> Btn_Choice_3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Skip;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Skip;

private:
	UFUNCTION()
	void HandleChoiceButtonClicked(FName ChoiceId);

	UFUNCTION()
	void HandleSkipClicked();

	void EnsureLayout();
	void BuildFallbackShell();
	void EnsureFallbackChoices();
	void BindChoices();
	void BindSkip();
	void ApplyView();
	void ConfirmIndex(int32 Index);
	void Skip();
	void CloseInternal();
	void MoveFocus(int32 Delta);
	void RefreshFocus();
	int32 ChoiceCount() const;
	int32 FocusSlotCount() const;
	void AcquireInput();
	void ReleaseInput();
	void SetPawnControlBlocked(bool bBlocked);
	UPixelEventChoiceButton* ButtonAt(int32 Index) const;

	FPixelEventChoiceView ActiveView;
	int32 FocusedIndex = 0;
	bool bOpen = false;
	bool bResolving = false;
	bool bCapturedInput = false;
	bool bPawnControlBlocked = false;
	bool bBuiltFallbackShell = false;
	bool bWarnedMissingWidgets = false;
	bool bWarnedMissingPawn = false;
	bool bInConstruct = false;

	/** Fallback-only column. Not a required WBP name. */
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ChoiceColumn;
};
