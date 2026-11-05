#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Delegates/IDelegateInstance.h"
#include "UI/PixelHubNodeRowWidget.h"
#include "PixelHubUpgradeWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UOverlay;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHubUpgradeClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHubWalletChanged, int32, Core);

/**
 * C++ parent for WBP_Hub_Upgrade.
 * Five rows. Demo wallet starts at Core 120 and never calls Meta.
 * A live open reads GetNodeViews / GetCore and buys with Purchase(NodeId).
 * Esc and the close button return to the Hub. This widget does not write a save,
 * does not call AddCore, and does not bind a lab key.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_Hub_Upgrade"))
class PIXELSIDERL_API UPixelHubUpgradeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelHubUpgradeWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Local Core 120 and the wireframe §2.3 nodes. Does not call GetCore or Purchase. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void UseDemoWallet(int32 StartingCore = 120);

	/** Live Meta rows. Does not write a save. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void UseMetaWallet();

	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void OpenUpgrade();

	/** Esc / 关闭. Idempotent. Does not finish a purchase that is still locked. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void CloseUpgrade();

	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void MoveFocus(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void BuyFocused();

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsUpgradeOpen() const { return bOpen; }

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsDemoWallet() const { return bDemoWallet; }

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	int32 GetShownCore() const;

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	bool IsPurchasing() const { return bPurchasing; }

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnHubUpgradeClosed OnClosed;

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnHubWalletChanged OnWalletChanged;

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
	TObjectPtr<UTextBlock> Txt_Core;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Close;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Close;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> VBox_Nodes;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Hint;

private:
	void EnsureLayout();
	void BuildFallbackShell();
	void BindClose();
	void EnsureRowCount(int32 Count);
	void RefreshRows();
	TArray<FPixelHubNodeRowView> BuildViews() const;
	void TryPurchase(FName NodeId);
	void CommitPurchase();
	void CancelPurchaseLock();
	void BroadcastCore();
	bool FinishPurchase(float DeltaSeconds);

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleRowBuy(FName NodeId);

	UPROPERTY()
	TArray<TObjectPtr<UPixelHubNodeRowWidget>> Rows;

	UPROPERTY()
	TArray<int32> DemoRanks;

	FName PendingNodeId;
	FDelegateHandle PurchaseTicker;
	int32 DemoCore = 0;
	int32 FocusIndex = 0;
	int32 ShownCount = 0;
	bool bDemoWallet = false;
	bool bOpen = false;
	bool bPurchasing = false;
	bool bCloseBound = false;
	bool bBuiltFallbackShell = false;
	bool bWarnedMissingWidgets = false;
};
