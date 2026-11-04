#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Affix/AffixSubsystem.h"
#include "AffixChoicePresenterComponent.generated.h"

class UAffixChoiceDebugWidget;

/**
 * Shows the native three-choice stand-in and accepts 1/2/3 while the game is paused.
 * WBP_AffixChoice should bind OnAffixChoiceOffered / OnAffixChoiceClosed and set bEnableNativeChoiceUI false.
 */
UCLASS(ClassGroup = (PixelSide), meta = (BlueprintSpawnableComponent, DisplayName = "AC_AffixChoicePresenter"))
class PIXELSIDERL_API UAffixChoicePresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAffixChoicePresenterComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	bool bEnableNativeChoiceUI = true;

private:
	UFUNCTION()
	void HandleOffered(const TArray<FAffixChoiceEntry>& Choices);

	UFUNCTION()
	void HandleClosed(bool bConfirmed);

	void ConfirmSlot(int32 Index);
	void BindPauseKeys();
	void UnbindSubsystem();
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

	bool bPauseKeysBound = false;
};
