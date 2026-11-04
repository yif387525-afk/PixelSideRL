#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Affix/AffixSubsystem.h"
#include "AffixChoiceDebugWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/**
 * Native stand-in for WBP_AffixChoice. Text only, no uasset.
 * Shows the three GetAffixCardView rows. Keys 1/2/3 or the buttons call ConfirmAffixChoice.
 * Replace this with WBP_AffixChoice by listening to the same subsystem events and disabling the presenter.
 */
UCLASS()
class PIXELSIDERL_API UAffixChoiceDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UAffixChoiceDebugWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void ShowChoices(const TArray<FAffixChoiceEntry>& Choices);

	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void EnsureLayout();
	void ConfirmIndex(int32 Index);

	UFUNCTION()
	void HandleCard0();

	UFUNCTION()
	void HandleCard1();

	UFUNCTION()
	void HandleCard2();

	UPROPERTY()
	TObjectPtr<UVerticalBox> RootBox = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> Lines;
};
