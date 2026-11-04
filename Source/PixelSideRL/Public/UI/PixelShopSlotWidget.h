#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Affix/AffixSubsystem.h"
#include "PixelShopSlotWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopSlotBuyClicked, int32, SlotIndex);

/** Matches DA_ShopInventory slot kind. Scrap only. */
UENUM(BlueprintType)
enum class EPixelShopItemKind : uint8
{
	WeaponContract,
	Heal,
	AffixVoucher
};

/**
 * JSON onPurchase string. The shop shell records it and does not run it.
 * GrantRunWeapon does not equip a weapon. HealFractionOfMaxHp does not change HP.
 * OpenAffixChoiceImmediate only causes OnRequestAffixChoiceFromVoucher.
 */
UENUM(BlueprintType)
enum class EPixelShopOnPurchase : uint8
{
	None,
	GrantRunWeapon,
	HealFractionOfMaxHp,
	OpenAffixChoiceImmediate
};

/**
 * One Room_Shop_01 row.
 * Price is resolved Scrap from DA_ShopPriceCurve (baseScrap * layerMultiplier, not rounded to an int).
 * Negative Price means the curve had no row; the slot greys out.
 * AffixDef is optional and unused by the locked three rows. When set, the slot shows
 * GetAffixCardView Name / Desc / Rarity / Icon only.
 * DataAssetName is the table string. This shell never loads that asset.
 */
USTRUCT(BlueprintType)
struct PIXELSIDERL_API FPixelShopSlotOffer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	EPixelShopItemKind Kind = EPixelShopItemKind::WeaponContract;

	/** offerId from the inventory table. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	double Price = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FName PriceKey;

	/** Heal rows only. Negative means the table left it null. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	double HealFractionOfMaxHp = -1.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	EPixelShopOnPurchase OnPurchase = EPixelShopOnPurchase::None;

	/** Table name only, for example DA_Weapon_Peashooter. Not loaded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FString DataAssetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	TObjectPtr<UAffixDefinition> AffixDef = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	TSoftObjectPtr<UTexture2D> Icon;
};

/**
 * C++ parent for WBP_ShopSlot.
 * One Room_Shop_01 cell (128×100). Builds the named controls when no designer tree exists.
 * Does not spend Scrap and does not grant, heal, or open an affix choice.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_ShopSlot"))
class PIXELSIDERL_API UPixelShopSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelShopSlotWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 0..2. Widget names are Slot_1..Slot_3. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetSlotIndex(int32 Index);

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	int32 GetSlotIndex() const { return SlotIndex; }

	/**
	 * AffixView is null unless the offer carries an AffixDef.
	 * Only Name, Desc, Rarity, and Icon are read from the view.
	 * bInCanAfford false greys the cell (DA insufficientScrap = GreyOut). Sold draws the slash.
	 */
	void ApplySlot(const FPixelShopSlotOffer& Offer, const FAffixCardView* AffixView, bool bInSold, bool bInCanAfford);

	void SetFocused(bool bInFocused);
	void SetNudgeX(float InNudgeX);

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnShopSlotBuyClicked OnBuyClicked;

protected:
	virtual void NativePreConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Frame;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Buy;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Kind;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Price;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_SoldOut;

private:
	UFUNCTION()
	void HandleBuyClicked();

	void EnsureLayout();
	void BuildFallbackLayout();
	void BindBuyButton();
	void ApplyTranslation();

	static FLinearColor RarityColor(EAffixRarity Rarity);
	static FText FormatScrapAmount(double Value);

	int32 SlotIndex = 0;
	bool bFocused = false;
	bool bSold = false;
	float NudgeX = 0.f;
	bool bWarnedMissingWidgets = false;

	/** Fallback-only 1px focus ring. Not a required WBP name. */
	UPROPERTY(Transient)
	TObjectPtr<UBorder> FocusRing;
};
