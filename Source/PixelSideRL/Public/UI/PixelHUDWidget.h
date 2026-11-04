#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Affix/AffixSubsystem.h"
#include "Engine/EngineTypes.h"
#include "PixelHUDWidget.generated.h"

class UHorizontalBox;
class UImage;
class UPixelHUDAffixIconWidget;
class UProgressBar;
class UTextBlock;
class UTexture2D;

/**
 * C++ parent for WBP_HUD.
 * In-run top bar: health, weapon chip, ammo, Scrap, up to 8 affix icons.
 * Builds the named controls when no designer tree exists.
 *
 * Affix strip listens to OnAffixChanged and reads GetOwnedAffixes (Icon + AffixId only).
 * Combat numbers are Demo* until a formal read API exists. Call the setters; this widget
 * does not ApplyDamage, Fire, or read the debug combat HUD.
 *
 * Reserved for 弹芯 (not called here): GetHealthRatio(), current weapon icon, Scrap balance.
 * Mount: AC_HUDPresenter on the player. Visibility stays SelfHitTestInvisible / HitTestInvisible.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_HUD"))
class PIXELSIDERL_API UPixelHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 0..1. <=0 empty bar (Dead). <=0.25 red fill flashing at 2Hz (LowHealth). */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetHealthRatio(float Ratio);

	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetAmmo(int32 Ammo);

	/** In-run currency. Label is Scrap. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetScrap(int32 Scrap);

	/** Null restores the flat weapon chip. Does not load a placeholder texture. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetWeaponIcon(UTexture2D* Icon);

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	float GetDemoHealthRatio() const { return DemoHealth; }

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	int32 GetDemoAmmo() const { return DemoAmmo; }

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	int32 GetDemoScrap() const { return DemoScrap; }

protected:
	virtual void NativePreConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> Bar_Health;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Weapon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Ammo;

	/** Wireframe name stays Txt_Coin. Shown text is Scrap, not 币. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Coin;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HB_AffixIcons;

	/** Placeholder until GetHealthRatio(). 1 = full. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|HUD")
	float DemoHealth = 1.f;

	/** Placeholder until an ammo / energy read exists. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|HUD")
	int32 DemoAmmo = 99;

	/** Placeholder until a Scrap balance read exists. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|HUD")
	int32 DemoScrap = 0;

private:
	UFUNCTION()
	void HandleAffixChanged(FAffixChangeEvent Event);

	void EnsureLayout();
	void BuildFallbackShell();
	void EnsureAffixSlots();
	void RefreshFromDemo();
	void RefreshHealthPresentation();
	void RefreshAmmoAndScrap();
	void RefreshWeaponIcon();
	void RefreshAffixIcons();
	void TryBindSubsystem();
	void UnbindSubsystem();
	UAffixSubsystem* FindConcreteSubsystem() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPixelHUDAffixIconWidget>> AffixSlots;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> DemoWeaponIcon;

	TWeakObjectPtr<UAffixSubsystem> BoundSubsystem;
	FTimerHandle RefreshTimer;
	TWeakObjectPtr<UTexture2D> AppliedWeaponIcon;
	bool bAppliedWeaponMissing = false;
	bool bHasWeaponOverride = false;
	bool bBuiltFallbackShell = false;
	bool bWarnedMissingWidgets = false;
	bool bWarnedForeignAffixSlots = false;
	bool bLoggedMissingSubsystem = false;
	bool bLoggedAffixTruncation = false;
};
