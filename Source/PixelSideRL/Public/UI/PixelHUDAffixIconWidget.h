#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PixelHUDAffixIconWidget.generated.h"

class UImage;
class UTexture2D;

/**
 * C++ parent for WBP_HUD_AffixIcon.
 * One 12x12 HUD slot. Empty is a gray chip. Occupied draws Icon, or a light chip when Icon is unset.
 * Does not load a placeholder texture. Hover text is reserved: AffixId is stored, and the slot
 * stays non-hit-testable so the bar does not steal clicks.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_HUD_AffixIcon"))
class PIXELSIDERL_API UPixelHUDAffixIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelHUDAffixIconWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Occupied slot. Icon may be null; the chip still reads as owned. AffixId is the only label kept. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetOccupied(FName InAffixId, UTexture2D* Icon);

	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetEmpty();

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	FName GetAffixId() const { return AffixId; }

protected:
	virtual void NativePreConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Icon;

private:
	void EnsureLayout();
	void BuildFallbackLayout();
	void ApplyVisual();

	UPROPERTY()
	TObjectPtr<UTexture2D> IconTexture;

	FName AffixId = NAME_None;
	bool bOccupied = false;
	bool bWarnedMissingWidgets = false;
};
