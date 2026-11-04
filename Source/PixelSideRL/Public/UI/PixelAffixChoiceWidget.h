#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Affix/AffixSubsystem.h"
#include "PixelAffixChoiceWidget.generated.h"

class APlayerController;
class UHorizontalBox;
class UImage;
class UOverlay;
class UPixelAffixCardWidget;
class UTextBlock;

/**
 * C++ parent for WBP_AffixChoice.
 * Fills Card_0..2 from GetAffixCardView (Name / Desc / Rarity / Icon) and confirms
 * with ConfirmAffixChoice(Index). Closes on OnAffixChoiceClosed.
 * Does not RollChoices, RequestAffixChoice, or SetGamePaused.
 * Pause stays on UAffixSubsystem::ApplyChoicePause (Request / Confirm).
 *
 * Mount: AC_AffixChoicePresenter.AffixChoiceWidgetClass, with bEnableNativeChoiceUI false
 * so UAffixChoiceDebugWidget is not also shown.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_AffixChoice"))
class PIXELSIDERL_API UPixelAffixChoiceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelAffixChoiceWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Fill three cards and show. Does not roll and does not pause. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void PresentOfferedChoice(const TArray<FAffixChoiceEntry>& Choices);

	/** Collapse and release UI input. Idempotent. Does not unpause. */
	void CloseFromPresenter(bool bConfirmed);

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
	TObjectPtr<UTextBlock> Txt_Hint;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HB_Cards;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPixelAffixCardWidget> Card_0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPixelAffixCardWidget> Card_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPixelAffixCardWidget> Card_2;

private:
	UFUNCTION()
	void HandleAffixChoiceOffered(const TArray<FAffixChoiceEntry>& Choices);

	UFUNCTION()
	void HandleAffixChoiceClosed(bool bConfirmed);

	UFUNCTION()
	void HandleCardSelected(int32 Index);

	void EnsureLayout();
	void BuildFallbackShell();
	void EnsureFallbackCards();
	void BindCards();
	void TryBindSubsystem();
	void UnbindSubsystem();
	UAffixSubsystem* FindConcreteSubsystem() const;

	void OpenWithViews(const TArray<FAffixCardView>& Views);
	void ApplyViews(const TArray<FAffixCardView>& Views);
	void ConfirmIndex(int32 Index);
	void CloseInternal(bool bConfirmed);
	void MoveFocus(int32 Delta);
	void RefreshFocus();
	void AcquireInput();
	void ReleaseInput();

	UPixelAffixCardWidget* CardAt(int32 Index) const;

	TWeakObjectPtr<UAffixSubsystem> BoundSubsystem;
	int32 FocusedIndex = 0;
	bool bOpen = false;
	bool bConfirming = false;
	bool bCapturedInput = false;
	bool bBuiltFallbackShell = false;
	bool bWarnedMissingWidgets = false;
	bool bLoggedMissingSubsystem = false;
	bool bInConstruct = false;
};
