#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "PixelHUDPresenterComponent.generated.h"

/**
 * Adds WBP_HUD for the local player and leaves it on screen.
 * Does not pause, does not change the input mode, and does not touch AC_AffixChoicePresenter.
 * HUDWidgetClass null falls back to UPixelHUDWidget.
 */
UCLASS(ClassGroup = (PixelSide), meta = (BlueprintSpawnableComponent, DisplayName = "AC_HUDPresenter"))
class PIXELSIDERL_API UPixelHUDPresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPixelHUDPresenterComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Create the formal HUD on the owning local player if it is not already in the viewport.
	 * Safe to call from BeginPlay, PossessedBy, and PawnClientRestart.
	 * GameMode / PlayerController on a Run map can call this too; one instance is enough.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void EnsureMounted();

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	UUserWidget* GetHUDWidget() const { return HUDWidget; }

	/** Formal shell. UPixelHUDWidget or a child (DisplayName WBP_HUD). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	/** Turn off to keep the component and skip the viewport add. Default on, including Shipping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|UI")
	bool bEnableHUD = true;

private:
	UPROPERTY()
	TObjectPtr<UUserWidget> HUDWidget;
};
