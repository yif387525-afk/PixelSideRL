#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Affix/AffixSubsystem.h"
#include "PixelAffixCardWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAffixCardSelected, int32, Index);

/**
 * C++ parent for WBP_AffixCard.
 * Shows one offered card: Name, Desc, Rarity, Icon. Hotkey digit is UI-local.
 * Builds the named controls when no designer tree exists.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_AffixCard"))
class PIXELSIDERL_API UPixelAffixCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelAffixCardWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 0..2. Also writes Txt_Hotkey as "1".."3". */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetChoiceIndex(int32 Index);

	/** Display fields only. Does not read UAffixDefinition. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void ApplyCardView(const FAffixCardView& View);

	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetFocused(bool bInFocused);

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	int32 GetChoiceIndex() const { return ChoiceIndex; }

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnAffixCardSelected OnCardSelected;

protected:
	virtual void NativePreConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Rarity;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Select;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Hotkey;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Desc;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Rarity;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Stacks;

private:
	UFUNCTION()
	void HandleSelectClicked();

	void EnsureLayout();
	void BuildFallbackLayout();
	void BindSelectButton();
	void ApplyRarityChrome(EAffixRarity Rarity);

	static FLinearColor RarityColor(EAffixRarity Rarity);

	int32 ChoiceIndex = 0;
	bool bFocused = false;
	bool bWarnedMissingWidgets = false;

	/** Fallback-only 1px focus ring. Not a required WBP name. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> FocusRing;
};
