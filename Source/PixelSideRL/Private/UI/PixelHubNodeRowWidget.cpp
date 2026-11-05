#include "UI/PixelHubNodeRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateTypes.h"
#include "UI/PixelAffixFont.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelHubRow, Log, All);

namespace
{
	const FLinearColor RowInk(FColor(0xF2, 0xF2, 0xF2));
	const FLinearColor RowCoreColor(FColor(0x7E, 0xC8, 0xFF));
	const FLinearColor CostRed(FColor(0xFF, 0x3B, 0x3B));
	const FLinearColor DimText(FColor(0x9A, 0xA0, 0xA6));
	const FLinearColor FrameIdle(FColor(0x3A, 0x3A, 0x4E));
	const FLinearColor FrameFocus(FColor(0xF2, 0xF2, 0xF2));
	const FLinearColor FrameLocked(FColor(0x4A, 0x4A, 0x4A));
	const FText BuyLabel = NSLOCTEXT("PixelHub", "Buy", "升级");
	const FText MaxedLabel = NSLOCTEXT("PixelHub", "Maxed", "已满级");

	void StyleLine(UTextBlock* Text, const FSlateFontInfo& Font, const FLinearColor& Color)
	{
		if (!Text)
		{
			return;
		}
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(ETextJustify::Left);
		Text->SetShadowOffset(FVector2D::ZeroVector);
	}

	void StyleBuyButton(UButton* Button)
	{
		if (!Button)
		{
			return;
		}
		Button->SetIsFocusable(false);
		FButtonStyle Style = Button->GetStyle();
		auto Face = [](FSlateBrush& Brush, const FLinearColor& Color)
		{
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.TintColor = FSlateColor(Color);
		};
		Face(Style.Normal, FLinearColor(FColor(0x1A, 0x4F, 0x73)));
		Face(Style.Hovered, FLinearColor(FColor(0x2A, 0x6E, 0x9C)));
		Face(Style.Pressed, FLinearColor(FColor(0x14, 0x3C, 0x58)));
		Face(Style.Disabled, FLinearColor(FColor(0x2A, 0x2A, 0x2A)));
		Style.NormalPadding = FMargin(6.f, 2.f);
		Style.PressedPadding = FMargin(6.f, 3.f, 6.f, 1.f);
		const FSlateColor ButtonInk(RowInk);
		Style.NormalForeground = ButtonInk;
		Style.HoveredForeground = ButtonInk;
		Style.PressedForeground = ButtonInk;
		Style.DisabledForeground = FSlateColor(DimText);
		Button->SetStyle(Style);
	}

	void PaintBorder(UBorder* Border, const FLinearColor& Color)
	{
		if (!Border)
		{
			return;
		}
		Border->SetBrushColor(Color);
	}
}

UPixelHubNodeRowWidget::UPixelHubNodeRowWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

void UPixelHubNodeRowWidget::ApplyRow(const FPixelHubNodeRowView& View, bool bFocused, bool bInputLocked)
{
	EnsureLayout();
	NodeId = View.NodeId;
	if (Txt_Name)
	{
		Txt_Name->SetText(View.DisplayName);
		Txt_Name->SetColorAndOpacity(FSlateColor(RowInk));
	}
	if (Txt_Level)
	{
		const int32 MaxRank = FMath::Max(1, View.MaxPurchases);
		Txt_Level->SetText(FText::FromString(FString::Printf(TEXT("Lv %d/%d"), View.OwnedRank, MaxRank)));
		Txt_Level->SetColorAndOpacity(FSlateColor(RowInk));
	}
	if (Txt_Desc)
	{
		Txt_Desc->SetText(View.Description);
		Txt_Desc->SetColorAndOpacity(FSlateColor(DimText));
	}
	if (Txt_Cost)
	{
		if (View.bMaxed)
		{
			Txt_Cost->SetText(MaxedLabel);
			Txt_Cost->SetColorAndOpacity(FSlateColor(DimText));
		}
		else
		{
			Txt_Cost->SetText(FText::FromString(FString::Printf(TEXT("下一阶 %d Core"), View.NextCostCore)));
			const bool bShort = !View.bLocked && !View.bCanPurchase;
			Txt_Cost->SetColorAndOpacity(FSlateColor(bShort ? CostRed : RowCoreColor));
		}
	}
	if (Btn_Buy)
	{
		Btn_Buy->SetVisibility(View.bMaxed ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
		Btn_Buy->SetIsEnabled(!View.bMaxed && View.bCanPurchase && !bInputLocked);
	}
	if (Txt_Buy)
	{
		Txt_Buy->SetText(BuyLabel);
	}
	if (Border_Locked)
	{
		Border_Locked->SetVisibility(View.bLocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	const FLinearColor Frame = View.bLocked ? FrameLocked : (bFocused ? FrameFocus : FrameIdle);
	PaintBorder(Border_Frame, Frame);
}

void UPixelHubNodeRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
}

TSharedRef<SWidget> UPixelHubNodeRowWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelHubNodeRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureLayout();
	BindBuy();
}

void UPixelHubNodeRowWidget::EnsureLayout()
{
	const bool bHave = Border_Frame && Txt_Name && Txt_Level && Txt_Desc && Txt_Cost && Btn_Buy && Border_Locked;
	if (bHave)
	{
		BindBuy();
		return;
	}
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFallbackLayout();
	}
	else if (!bWarnedMissingWidgets)
	{
		bWarnedMissingWidgets = true;
		UE_LOG(LogPixelHubRow, Warning, TEXT("WBP_Hub_NodeRow is missing one of Border_Frame, Txt_Name, Txt_Level, Txt_Desc, Txt_Cost, Btn_Buy, Border_Locked."));
	}
	BindBuy();
}

void UPixelHubNodeRowWidget::BuildFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo NameFont = PixelAffixMakeFont(13);
	const FSlateFontInfo BodyFont = PixelAffixMakeFont(11);

	Border_Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Frame"));
	FSlateBrush FrameBrush;
	FrameBrush.DrawAs = ESlateBrushDrawType::Box;
	FrameBrush.TintColor = FSlateColor(FrameIdle);
	Border_Frame->SetBrush(FrameBrush);
	Border_Frame->SetPadding(FMargin(8.f, 6.f));
	WidgetTree->RootWidget = Border_Frame;

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_Row"));
	Overlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Border_Frame->SetContent(Overlay);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Row"));
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* ColumnSlot = Overlay->AddChildToOverlay(Column))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
		ColumnSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Row"));
	Top->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Txt_Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Name"));
	Txt_Level = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Level"));
	Txt_Cost = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Cost"));
	StyleLine(Txt_Name, NameFont, RowInk);
	StyleLine(Txt_Level, BodyFont, RowInk);
	StyleLine(Txt_Cost, BodyFont, RowCoreColor);
	if (UHorizontalBoxSlot* NameSlot = Top->AddChildToHorizontalBox(Txt_Name))
	{
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);
		NameSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}
	if (UHorizontalBoxSlot* LevelSlot = Top->AddChildToHorizontalBox(Txt_Level))
	{
		LevelSlot->SetVerticalAlignment(VAlign_Center);
		LevelSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}
	if (UHorizontalBoxSlot* CostSlot = Top->AddChildToHorizontalBox(Txt_Cost))
	{
		CostSlot->SetVerticalAlignment(VAlign_Center);
		CostSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}

	USizeBox* BuySize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Btn_Buy"));
	BuySize->SetWidthOverride(72.f);
	BuySize->SetHeightOverride(22.f);
	Btn_Buy = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_Buy"));
	StyleBuyButton(Btn_Buy);
	Txt_Buy = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Buy"));
	StyleLine(Txt_Buy, BodyFont, RowInk);
	Txt_Buy->SetJustification(ETextJustify::Center);
	Txt_Buy->SetText(BuyLabel);
	Btn_Buy->AddChild(Txt_Buy);
	BuySize->SetContent(Btn_Buy);
	if (UHorizontalBoxSlot* BuySlot = Top->AddChildToHorizontalBox(BuySize))
	{
		BuySlot->SetVerticalAlignment(VAlign_Center);
	}

	Txt_Desc = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Desc"));
	StyleLine(Txt_Desc, BodyFont, DimText);
	Txt_Desc->SetAutoWrapText(true);

	if (UVerticalBoxSlot* TopSlot = Column->AddChildToVerticalBox(Top))
	{
		TopSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
	}
	Column->AddChildToVerticalBox(Txt_Desc);

	Border_Locked = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Locked"));
	FSlateBrush LockBrush;
	LockBrush.DrawAs = ESlateBrushDrawType::Box;
	LockBrush.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.45f));
	Border_Locked->SetBrush(LockBrush);
	Border_Locked->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* LockSlot = Overlay->AddChildToOverlay(Border_Locked))
	{
		LockSlot->SetHorizontalAlignment(HAlign_Fill);
		LockSlot->SetVerticalAlignment(VAlign_Fill);
	}

	BindBuy();
}

void UPixelHubNodeRowWidget::BindBuy()
{
	if (bBuyBound || !Btn_Buy)
	{
		return;
	}
	Btn_Buy->OnClicked.AddDynamic(this, &UPixelHubNodeRowWidget::HandleBuyClicked);
	bBuyBound = true;
}

void UPixelHubNodeRowWidget::HandleBuyClicked()
{
	if (!NodeId.IsNone())
	{
		OnBuyClicked.Broadcast(NodeId);
	}
}
