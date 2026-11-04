#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PixelHubNodeRowWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHubNodeBuyClicked, FName, NodeId);

/**
 * One permanent-growth row. The parent fills every field.
 * NextCostCore is the Core price of the next rank. This row does not spend anything.
 * Description may mention an in-run Scrap grant. The price line stays Core.
 */
USTRUCT(BlueprintType)
struct PIXELSIDERL_API FPixelHubNodeRowView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FName NodeId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	int32 OwnedRank = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	int32 MaxPurchases = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	int32 NextCostCore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	bool bMaxed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	bool bLocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PixelSide|UI")
	bool bCanPurchase = false;
};

/**
 * C++ parent for WBP_Hub_NodeRow.
 * Name, Lv Owned/Max, effect line, next Core cost, and the upgrade button.
 * Builds the named controls when no designer tree exists. Does not call Purchase.
 */
UCLASS(Blueprintable, meta = (DisplayName = "WBP_Hub_NodeRow"))
class PIXELSIDERL_API UPixelHubNodeRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPixelHubNodeRowWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "PixelSide|UI")
	void ApplyRow(const FPixelHubNodeRowView& View, bool bFocused, bool bInputLocked);

	UFUNCTION(BlueprintPure, Category = "PixelSide|UI")
	FName GetNodeId() const { return NodeId; }

	UPROPERTY(BlueprintAssignable, Category = "PixelSide|UI")
	FOnHubNodeBuyClicked OnBuyClicked;

protected:
	virtual void NativePreConstruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Frame;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Level;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Desc;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Cost;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Buy;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Buy;

	/** Shown only when a prerequisite is unmet. MVP rows leave it collapsed. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_Locked;

private:
	void EnsureLayout();
	void BuildFallbackLayout();
	void BindBuy();

	UFUNCTION()
	void HandleBuyClicked();

	FName NodeId;
	bool bBuyBound = false;
	bool bWarnedMissingWidgets = false;
};
