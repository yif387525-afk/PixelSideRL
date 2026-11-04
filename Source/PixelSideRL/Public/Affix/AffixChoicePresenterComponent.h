#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "Affix/AffixSubsystem.h"
#include "AffixChoicePresenterComponent.generated.h"

class UAffixChoiceDebugWidget;

/**
 * Listens for OnAffixChoiceOffered / OnAffixChoiceClosed and accepts 1/2/3 while paused.
 * AffixChoiceWidgetClass null + bEnableNativeChoiceUI: text stand-in UAffixChoiceDebugWidget (#29).
 * AffixChoiceWidgetClass set: that widget only. Native text UI is forced off so the two never stack.
 */
UCLASS(ClassGroup = (PixelSide), meta = (BlueprintSpawnableComponent, DisplayName = "AC_AffixChoicePresenter"))
class PIXELSIDERL_API UAffixChoicePresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAffixChoicePresenterComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Text stand-in. Forced off when AffixChoiceWidgetClass is set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	bool bEnableNativeChoiceUI = true;

	/**
	 * Formal choice widget. UPixelAffixChoiceWidget or a child (DisplayName WBP_AffixChoice).
	 * Null keeps the #29 text stand-in. Non-null ignores UAffixChoiceDebugWidget.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	TSubclassOf<UUserWidget> AffixChoiceWidgetClass = nullptr;

private:
	UFUNCTION()
	void HandleOffered(const TArray<FAffixChoiceEntry>& Choices);

	UFUNCTION()
	void HandleClosed(bool bConfirmed);

	void ConfirmSlot(int32 Index);
	void BindPauseKeys();
	void UnbindSubsystem();
	void ShowFormalChoice(const TArray<FAffixChoiceEntry>& Choices);
	UAffixSubsystem* Resolve() const;

	UFUNCTION()
	void ConfirmKey0();

	UFUNCTION()
	void ConfirmKey1();

	UFUNCTION()
	void ConfirmKey2();

	UPROPERTY()
	TObjectPtr<UAffixSubsystem> Bound = nullptr;

	UPROPERTY()
	TObjectPtr<UAffixChoiceDebugWidget> Widget = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> FormalChoiceWidget = nullptr;

	bool bPauseKeysBound = false;
};
