#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Ticker.h"
#include "UI/PixelShopSlotWidget.h"
#include "PixelShopWidget.generated.h"

class UAffixSubsystem;
class UButton;
class UHorizontalBox;
class UImage;
class UOverlay;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShopBuyItem, int32, SlotIndex, FName, ItemId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopReroll);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopRequestAffixChoiceFromVoucher);

/**
 * C++ parent for WBP_Shop.
 * Room_Shop_01 shelf: Slot_1..Slot_3 in one row (the P1 wireframe ASCII is 4 cells; that fourth
 * cell is not a widget). Demo wallet starts at Scrap 128. Reroll costs 25 Scrap.
 *
 * Two wallets. Do not mix them, and do not add another open/close path.
 *
 * Demo — OpenShopDemo and Lab F11. Local DemoScrap starts at 128. Buy and reroll
 * change only that number and the sold flags. They do not call GetRunScrap or
 * AddRunScrap. Heal is 30, so 128 becomes 98, then Leave closes.
 *
 * Run — OpenShop, the call AC_ShopPresenter::OpenShop already forwards.
 * The Scrap argument stays so that forward still compiles. It is not copied into
 * DemoScrap. The label is UEventSubsystem::GetRunScrap().
 * A successful buy calls AddRunScrap(-Price), then the label is GetRunScrap() again.
 * A successful reroll calls AddRunScrap(-RerollCost), then GetRunScrap().
 * RunScrap is int32. The slot still shows the offer double. The debit is llround
 * of that price, same rounding as GrantScrap. The price curve file is not rewritten.
 * Not enough Scrap greys the cell and does not spend.
 *
 * Neither path grants a weapon, heals, writes DA_Shop*, calls RequestAffixChoice,
 * or writes WBP_HUD. This shell does not create or drive AC_ShopPresenter.
 *
 * There is no shop-subsystem pause API. OpenShopDemo and OpenShop(..., bDemoPause=true)
 * call UGameplayStatics::SetGamePaused. That is the Lab / open path. Pass bDemoPause false
 * when the room already owns pause. Close does not unpause a pending affix choice.
 * This is not a second AC_AffixChoicePresenter.
 * Room flow opens and closes through UPixelShopPresenterComponent (AC_ShopPresenter).
 * That component only calls OpenShop, OpenShopDemo, and CloseShop. It does not
 * itself call AddRunScrap. Lab F11 does not use it.
 *
 * OpenShopDemo reads Content/Data/Shop/DA_ShopInventory.json and DA_ShopPriceCurve.json
 * (CSV if JSON is missing). It does not write those files and does not load weapon uassets.
 * Slot_1 shows one weapon from weaponContractPool. Slot_2 is Heal. Slot_3 is AffixVoucher.
 * Layer 1 prices are the curve baseScrap. Insufficient Scrap greys the cell.
 *
 * The 128 Scrap wallet and the 25 Scrap reroll are Lab demo state on this widget.
 * They are not fields in the shop tables, and they are not pushed into WBP_HUD.
 *
 * AffixVoucher order: OnBuyItem, CloseShop (unpause + OnShopClosed), then
 * OnRequestAffixChoiceFromVoucher. This widget does not call RequestAffixChoice.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_Shop"))
class PIXELSIDERL_API UPixelShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelShopWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * Read DA_Shop* into offers. Does not open the shell and does not touch DemoScrap or RunScrap.
	 * Same rows OpenShopDemo would show. The room then passes them to OpenShop.
	 */
	static void CollectShelfOffers(int32 Layer, TArray<FPixelShopSlotOffer>& OutOffers, TArray<FPixelShopSlotOffer>& OutPool);

	/**
	 * Run wallet. Same signature AC_ShopPresenter::OpenShop already calls.
	 * Scrap is not the balance and is not written to DemoScrap. The label is GetRunScrap().
	 * Buy success: AddRunScrap(-Price), then GetRunScrap. Reroll success: AddRunScrap(-RerollCost), then GetRunScrap.
	 * bDemoPause defaults on. Refuses to open while an affix choice is pending, or when UEventSubsystem is missing.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void OpenShop(const TArray<FPixelShopSlotOffer>& Offers, double Scrap, bool bDemoPause = true);

	/**
	 * Read the shop tables, show 3 slots, Scrap 128, demo pause.
	 * Layer selects DA_ShopPriceCurve.layers (1 = baseScrap). Restocks on every call.
	 * Local DemoScrap only. Does not call GetRunScrap or AddRunScrap. Lab F11.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void OpenShopDemo(int32 Layer = 1);

	/** Esc / Leave. Skip-buy is allowed. Idempotent. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void CloseShop();

	/**
	 * Replace one unsold row after OnReroll when this open was not OpenShopDemo.
	 * Turns off demo cycling so the flow's row is kept.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void SetSlotOffer(int32 SlotIndex, const FPixelShopSlotOffer& Offer);

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsShopOpen() const { return bOpen; }

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	int32 GetShopSlotCount() const { return 3; }

	/** Local Lab wallet. OpenShop does not write it. */
	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	double GetDemoScrap() const { return DemoScrap; }

	/** True while the open on screen is OpenShop (run wallet), not OpenShopDemo. */
	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsRunWallet() const { return bOpen && bRunWallet; }

	/** Last GetRunScrap() read for the run open. 0 on the demo path. */
	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	int32 GetShownRunScrap() const { return bRunWallet ? RunScrapShown : 0; }

	/** Wireframe Lab cost. Not a priceKey in DA_ShopPriceCurve. */
	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	double GetDemoRerollCost() const { return 25.0; }

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnShopBuyItem OnBuyItem;

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnShopReroll OnReroll;

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnShopClosed OnShopClosed;

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnShopRequestAffixChoiceFromVoucher OnRequestAffixChoiceFromVoucher;

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
	TObjectPtr<UTextBlock> Txt_Scrap;

	/** 1×3 row. The wireframe name UG_Slots was the 2×2 four-cell demo. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> HB_Slots;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPixelShopSlotWidget> Slot_1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPixelShopSlotWidget> Slot_2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPixelShopSlotWidget> Slot_3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Reroll;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Leave;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Hint;

	/** Caption inside Btn_Reroll. Fallback name; a WBP may bind it. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Reroll;

	/** Caption inside Btn_Leave. Fallback name; a WBP may bind it. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Leave;

private:
	void ApplyOpen(const TArray<FPixelShopSlotOffer>& InOffers, double InScrap, bool bInDemoPause, bool bInDemoShelf, bool bInRunWallet);
	void LoadOffers(const TArray<FPixelShopSlotOffer>& InOffers, double InScrap);
	void ShowDesignShelf();

	UFUNCTION()
	void HandleSlotBuyClicked(int32 SlotIndex);

	UFUNCTION()
	void HandleRerollClicked();

	UFUNCTION()
	void HandleLeaveClicked();

	void EnsureLayout();
	void BuildFallbackShell();
	void EnsureFallbackSlots();
	void BindInteractions();
	void RefreshAll();
	void RefreshFocus();
	void TryBuy(int32 Index);
	void TryReroll();
	void TryBuyFromRun(int32 Index);
	void TryRerollFromRun();
	bool PullRunScrap();
	void MoveFocus(int32 Delta);
	bool CanReroll() const;
	bool IsAffixChoicePending() const;
	UAffixSubsystem* FindAffix() const;

	void BeginDemoPause();
	void EndDemoPause();
	void AcquireInput();
	void ReleaseInput();

	void BeginNudge(int32 Index);
	void EndNudge();
	bool AdvanceNudge(float DeltaSeconds);
	void ArmInputLock();
	void ReleaseInputLock();
	bool FinishInputLock(float DeltaSeconds);

	UPixelShopSlotWidget* SlotAt(int32 Index) const;
	bool CanAfford(double Price) const;

	UPROPERTY()
	TArray<FPixelShopSlotOffer> Offers;

	/** WeaponContract pool for the open demo shelf. Reroll advances ShownWeapon. */
	UPROPERTY()
	TArray<FPixelShopSlotOffer> WeaponPool;

	double DemoScrap = 0.0;
	/** Mirror of the last GetRunScrap() while bRunWallet. Not a second wallet. */
	int32 RunScrapShown = 0;
	int32 ShownWeapon = 0;
	int32 PriceLayer = 1;
	int32 FocusedIndex = 0;
	bool bSlotSold[3] = {false, false, false};
	bool bDemoShelf = false;
	bool bRunWallet = false;
	bool bOpen = false;
	bool bOwnsPause = false;
	bool bCapturedInput = false;
	bool bInputLocked = false;
	bool bBuiltFallbackShell = false;
	bool bWarnedMissingWidgets = false;
	bool bLoggedMissingAffix = false;

	int32 NudgeSlot = INDEX_NONE;
	float NudgeAge = 0.f;
	FTSTicker::FDelegateHandle NudgeTicker;
	FTSTicker::FDelegateHandle InputLockTicker;
};
