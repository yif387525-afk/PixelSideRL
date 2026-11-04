#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/EngineTypes.h"
#include "PixelDebugCombatHUD.generated.h"

class AActor;
class APlayerController;
class UProgressBar;
class UTextBlock;

/**
 * C++ parent for WBP_DebugCombatHUD.
 * Lab / _Dev only: HP, i-frame, weapon, fire cooldown, last hit.
 * Builds the named controls when no designer tree exists. A child WBP binds the same names.
 * 只读调试 HUD。不调用 ApplyDamage / Fire，不进正式 WBP_HUD。
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_DebugCombatHUD"))
class PIXELSIDERL_API UPixelDebugCombatHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelDebugCombatHUD(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Create the overlay on a local player. Shipping builds return null.
	 * ObservedActor is usually the possessed APixelSideCharacter.
	 * WidgetClass null uses this native class (no WBP asset required).
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|Debug")
	static UPixelDebugCombatHUD* MountForPlayer(
		APlayerController* OwningPlayer,
		AActor* ObservedActor,
		TSubclassOf<UPixelDebugCombatHUD> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "PixelSide|Debug")
	void SetObservedActor(AActor* Actor);

	/** Dev switch. Shipping forces this off in the constructor and never shows the overlay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Debug")
	bool bEnableCombatDebugHUD = true;

protected:
	virtual void NativePreConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_HP;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_HP;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_IFrame;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Weapon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_CD;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_LastDmg;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Alive;

private:
	void EnsureLayout();
	void BuildFallbackLayout();
	void RefreshFromSource();
	bool IsDebugHUDAllowed();
	AActor* ResolveObservedActor();

	TWeakObjectPtr<AActor> ObservedActor;
	FTimerHandle RefreshHandle;
	bool bWarnedMissingWidgets = false;
};
