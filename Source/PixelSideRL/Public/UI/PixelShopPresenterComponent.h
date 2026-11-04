#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UI/PixelShopWidget.h"
#include "PixelShopPresenterComponent.generated.h"

/**
 * Room_Shop_01 open/close entry (DisplayName AC_ShopPresenter).
 * Creates WBP_Shop and calls UPixelShopWidget::OpenShop / OpenShopDemo / CloseShop.
 * Those opens add the shell at the existing shop viewport Z (80). This component
 * does not call AddToViewport itself, does not read or write RunScrap, and does not
 * call AddRunScrap. TryBuy / TryReroll stay on the shell.
 * Pass Scrap into OpenShop only to seed the shell's DemoScrap, or call OpenShopDemo
 * for the table shelf (Scrap 128).
 * No key is bound. F6–F11 are unchanged. F11 still opens the Lab demo on the character.
 */
UCLASS(ClassGroup = (PixelSide), meta = (BlueprintSpawnableComponent, DisplayName = "AC_ShopPresenter"))
class PIXELSIDERL_API UPixelShopPresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPixelShopPresenterComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * Show three offers. Scrap is copied into the shell only.
	 * bDemoPause matches UPixelShopWidget::OpenShop (default on).
	 * Returns the widget when it opened. Null if there is no local player or the shell refused.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	UPixelShopWidget* OpenShop(const TArray<FPixelShopSlotOffer>& Offers, double Scrap, bool bDemoPause = true);

	/**
	 * Read DA_Shop* and open the demo shelf. Layer 1 uses baseScrap. Scrap 128.
	 * Returns the widget when it opened. Null if there is no local player or the shell refused.
	 */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	UPixelShopWidget* OpenShopDemo(int32 Layer = 1);

	/** Same as Leave / Esc on the shell. Idempotent. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void CloseShop();

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsShopOpen() const;

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	UPixelShopWidget* GetShopWidget() const { return ShopWidget; }

	/** Null uses UPixelShopWidget. No uasset required. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|UI")
	TSubclassOf<UPixelShopWidget> ShopWidgetClass;

private:
	UPixelShopWidget* EnsureWidget();

	UPROPERTY(Transient)
	TObjectPtr<UPixelShopWidget> ShopWidget;
};
