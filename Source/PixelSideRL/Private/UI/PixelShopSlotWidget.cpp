#include "UI/PixelShopSlotWidget.h"

#include "UI/PixelAffixFont.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SWidget.h"

namespace PixelShopSlotStyle
{
	const FLinearColor TextColor(FColor(0xF2, 0xF2, 0xF2));
	const FLinearColor KindColor(FColor(0xC8, 0xC8, 0xC8));
	const FLinearColor PriceColor(FColor(0xFF, 0xD5, 0x4A));
	const FLinearColor GreyText(FColor(0x8A, 0x8A, 0x8A));
	const FLinearColor GreyFrame(FColor(0x5C, 0x5C, 0x5C));
	const FLinearColor FrameColor(FColor(0xB0, 0xB0, 0xB0));
	const FLinearColor CardFill(FColor(0x1A, 0x1A, 0x2E));
	const FLinearColor IconEmpty(FColor(0x4A, 0x4A, 0x4A));
	const FLinearColor SoldSlash(FColor(0xE0, 0xE0, 0xE0));
	constexpr float SlotWidth = 128.f;
	constexpr float SlotHeight = 100.f;
	constexpr float IconSize = 24.f;

	void StyleText(UTextBlock* Text, const FSlateFontInfo& Font, const FLinearColor& Color, bool bWrap)
	{
		if (!Text)
		{
			return;
		}
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(ETextJustify::Center);
		Text->SetShadowOffset(FVector2D::ZeroVector);
		Text->SetAutoWrapText(bWrap);
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	FSlateBrush Solid(const FLinearColor& Color, const FVector2D& Size)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(Color);
		Brush.ImageSize = Size;
		return Brush;
	}

	FText KindLabel(const FPixelShopSlotOffer& Offer)
	{
		switch (Offer.Kind)
		{
		case EPixelShopItemKind::Heal:
			if (Offer.HealFractionOfMaxHp >= 0.0)
			{
				const int32 Percent = FMath::RoundToInt(Offer.HealFractionOfMaxHp * 100.0);
				return FText::Format(
					NSLOCTEXT("PixelShopSlot", "KindHeal", "Heal · {0}% MaxHP"),
					FText::AsNumber(Percent));
			}
			return NSLOCTEXT("PixelShopSlot", "KindHealPlain", "Heal");
		case EPixelShopItemKind::AffixVoucher:
			return NSLOCTEXT("PixelShopSlot", "KindVoucher", "AffixVoucher");
		case EPixelShopItemKind::WeaponContract:
		default:
			return NSLOCTEXT("PixelShopSlot", "KindWeapon", "WeaponContract");
		}
	}
}

UPixelShopSlotWidget::UPixelShopSlotWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = false;
	SetVisibility(ESlateVisibility::Visible);
}

void UPixelShopSlotWidget::SetSlotIndex(int32 Index)
{
	SlotIndex = Index;
}

void UPixelShopSlotWidget::ApplySlot(const FPixelShopSlotOffer& Offer, const FAffixCardView* AffixView, bool bInSold, bool bInCanAfford)
{
	EnsureLayout();
	bSold = bInSold;
	const bool bGrey = bInSold || !bInCanAfford;
	const FLinearColor NameColor = bGrey
		? PixelShopSlotStyle::GreyText
		: (AffixView ? RarityColor(AffixView->Rarity) : PixelShopSlotStyle::TextColor);
	const FLinearColor BodyColor = bGrey ? PixelShopSlotStyle::GreyText : PixelShopSlotStyle::KindColor;
	const FLinearColor CostColor = bGrey ? PixelShopSlotStyle::GreyText : PixelShopSlotStyle::PriceColor;

	if (Txt_Name)
	{
		const FText Name = (AffixView && !AffixView->Name.IsEmpty()) ? AffixView->Name : Offer.Name;
		Txt_Name->SetText(Name);
		Txt_Name->SetColorAndOpacity(FSlateColor(NameColor));
	}
	if (Txt_Kind)
	{
		Txt_Kind->SetText(PixelShopSlotStyle::KindLabel(Offer));
		Txt_Kind->SetColorAndOpacity(FSlateColor(BodyColor));
	}
	if (Txt_Price)
	{
		if (Offer.Price < 0.0)
		{
			Txt_Price->SetText(NSLOCTEXT("PixelShopSlot", "NoPrice", "—"));
		}
		else
		{
			Txt_Price->SetText(FText::Format(
				NSLOCTEXT("PixelShopSlot", "Price", "{0} Scrap"),
				FormatScrapAmount(Offer.Price)));
		}
		Txt_Price->SetColorAndOpacity(FSlateColor(CostColor));
	}

	FText Tip = Offer.Description;
	if (AffixView && !AffixView->Desc.IsEmpty())
	{
		Tip = AffixView->Desc;
	}
	SetToolTipText(Tip);
	if (Btn_Buy)
	{
		Btn_Buy->SetToolTipText(Tip);
		Btn_Buy->SetIsEnabled(!bInSold);
	}

	if (Border_Frame)
	{
		Border_Frame->SetBrushColor(bGrey ? PixelShopSlotStyle::GreyFrame : PixelShopSlotStyle::FrameColor);
	}

	if (Img_Icon)
	{
		TSoftObjectPtr<UTexture2D> Icon = Offer.Icon;
		if (AffixView)
		{
			Icon = AffixView->Icon;
		}
		UTexture2D* Texture = nullptr;
		if (!Icon.IsNull())
		{
			Texture = Icon.Get();
			if (!Texture)
			{
				Texture = Icon.LoadSynchronous();
			}
		}
		if (Texture)
		{
			Img_Icon->SetBrushFromTexture(Texture, false);
			Img_Icon->SetDesiredSizeOverride(FVector2D(PixelShopSlotStyle::IconSize, PixelShopSlotStyle::IconSize));
		}
		else
		{
			Img_Icon->SetBrush(PixelShopSlotStyle::Solid(
				PixelShopSlotStyle::IconEmpty,
				FVector2D(PixelShopSlotStyle::IconSize, PixelShopSlotStyle::IconSize)));
		}
		Img_Icon->SetColorAndOpacity(bGrey ? FLinearColor(1.f, 1.f, 1.f, 0.35f) : FLinearColor::White);
	}

	if (Img_SoldOut)
	{
		Img_SoldOut->SetVisibility(bInSold ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPixelShopSlotWidget::SetFocused(bool bInFocused)
{
	bFocused = bInFocused;
	if (FocusRing)
	{
		FocusRing->SetPadding(FMargin(bFocused ? 1.f : 0.f));
		FocusRing->SetBrushColor(bFocused ? FLinearColor::White : FLinearColor::Transparent);
	}
	else if (Border_Frame)
	{
		Border_Frame->SetPadding(FMargin(bFocused ? 3.f : 2.f));
	}
	ApplyTranslation();
}

void UPixelShopSlotWidget::SetNudgeX(float InNudgeX)
{
	NudgeX = InNudgeX;
	ApplyTranslation();
}

void UPixelShopSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
}

TSharedRef<SWidget> UPixelShopSlotWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelShopSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureLayout();
	BindBuyButton();
}

void UPixelShopSlotWidget::NativeDestruct()
{
	if (Btn_Buy)
	{
		Btn_Buy->OnClicked.RemoveDynamic(this, &UPixelShopSlotWidget::HandleBuyClicked);
	}
	Super::NativeDestruct();
}

void UPixelShopSlotWidget::HandleBuyClicked()
{
	if (!bSold)
	{
		OnBuyClicked.Broadcast(SlotIndex);
	}
}

void UPixelShopSlotWidget::EnsureLayout()
{
	const bool bRequired = Border_Frame && Btn_Buy && Img_Icon && Txt_Name && Txt_Kind && Txt_Price && Img_SoldOut;
	if (bRequired)
	{
		return;
	}
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFallbackLayout();
		return;
	}
	if (!bWarnedMissingWidgets)
	{
		bWarnedMissingWidgets = true;
		UE_LOG(LogPixelAffixUI, Warning,
			TEXT("WBP_ShopSlot is missing one of Border_Frame, Btn_Buy, Img_Icon, Txt_Name, Txt_Kind, Txt_Price, Img_SoldOut."));
	}
}

void UPixelShopSlotWidget::BuildFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo NameFont = PixelAffixMakeFont(11);
	const FSlateFontInfo KindFont = PixelAffixMakeFont(9);
	const FSlateFontInfo PriceFont = PixelAffixMakeFont(11);

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Slot"));
	Size->SetWidthOverride(PixelShopSlotStyle::SlotWidth);
	Size->SetHeightOverride(PixelShopSlotStyle::SlotHeight);

	FocusRing = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Focus"));
	FocusRing->SetBrushColor(FLinearColor::Transparent);
	FocusRing->SetPadding(FMargin(0.f));
	Size->SetContent(FocusRing);

	Border_Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Frame"));
	Border_Frame->SetBrushColor(PixelShopSlotStyle::FrameColor);
	Border_Frame->SetPadding(FMargin(2.f));
	FocusRing->SetContent(Border_Frame);

	UBorder* Body = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Body"));
	Body->SetBrushColor(PixelShopSlotStyle::CardFill);
	Body->SetPadding(FMargin(6.f, 4.f, 6.f, 4.f));
	Border_Frame->SetContent(Body);

	UOverlay* Face = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_Face"));
	Body->SetContent(Face);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Slot"));
	if (UOverlaySlot* ColumnSlot = Face->AddChildToOverlay(Column))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
		ColumnSlot->SetVerticalAlignment(VAlign_Fill);
	}

	Img_Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Icon"));
	Img_Icon->SetBrush(PixelShopSlotStyle::Solid(
		PixelShopSlotStyle::IconEmpty,
		FVector2D(PixelShopSlotStyle::IconSize, PixelShopSlotStyle::IconSize)));
	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Icon"));
	IconBox->SetWidthOverride(PixelShopSlotStyle::IconSize);
	IconBox->SetHeightOverride(PixelShopSlotStyle::IconSize);
	IconBox->SetContent(Img_Icon);
	if (UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconBox))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
	}

	Txt_Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Name"));
	Txt_Kind = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Kind"));
	Txt_Price = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Price"));
	PixelShopSlotStyle::StyleText(Txt_Name, NameFont, PixelShopSlotStyle::TextColor, false);
	PixelShopSlotStyle::StyleText(Txt_Kind, KindFont, PixelShopSlotStyle::KindColor, false);
	PixelShopSlotStyle::StyleText(Txt_Price, PriceFont, PixelShopSlotStyle::PriceColor, false);

	auto AddText = [Column](UTextBlock* Text, float Bottom)
	{
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Text))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, Bottom));
		}
	};
	AddText(Txt_Name, 1.f);
	AddText(Txt_Kind, 1.f);
	AddText(Txt_Price, 0.f);

	Btn_Buy = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_Buy"));
	FButtonStyle Style = Btn_Buy->GetStyle();
	auto ClearBrush = [](FSlateBrush& Brush)
	{
		Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
		Brush.TintColor = FSlateColor(FLinearColor::Transparent);
	};
	ClearBrush(Style.Normal);
	ClearBrush(Style.Hovered);
	ClearBrush(Style.Pressed);
	ClearBrush(Style.Disabled);
	Style.NormalPadding = FMargin(0.f);
	Style.PressedPadding = FMargin(0.f);
	Btn_Buy->SetStyle(Style);
	Btn_Buy->SetBackgroundColor(FLinearColor::Transparent);
	if (UOverlaySlot* ButtonSlot = Face->AddChildToOverlay(Btn_Buy))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		ButtonSlot->SetVerticalAlignment(VAlign_Fill);
	}

	Img_SoldOut = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_SoldOut"));
	Img_SoldOut->SetBrush(PixelShopSlotStyle::Solid(PixelShopSlotStyle::SoldSlash, FVector2D(140.f, 3.f)));
	Img_SoldOut->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Img_SoldOut->SetRenderTransformAngle(-24.f);
	Img_SoldOut->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* SoldSlot = Face->AddChildToOverlay(Img_SoldOut))
	{
		SoldSlot->SetHorizontalAlignment(HAlign_Center);
		SoldSlot->SetVerticalAlignment(VAlign_Center);
	}

	WidgetTree->RootWidget = Size;
	BindBuyButton();
}

void UPixelShopSlotWidget::BindBuyButton()
{
	if (!Btn_Buy)
	{
		return;
	}
	Btn_Buy->OnClicked.RemoveDynamic(this, &UPixelShopSlotWidget::HandleBuyClicked);
	Btn_Buy->OnClicked.AddDynamic(this, &UPixelShopSlotWidget::HandleBuyClicked);
}

void UPixelShopSlotWidget::ApplyTranslation()
{
	SetRenderTranslation(FVector2D(NudgeX, bFocused ? -2.f : 0.f));
}

FLinearColor UPixelShopSlotWidget::RarityColor(EAffixRarity Rarity)
{
	switch (Rarity)
	{
	case EAffixRarity::Rare:
		return FLinearColor(FColor(0x3D, 0x7E, 0xFF));
	case EAffixRarity::Epic:
		return FLinearColor(FColor(0xB4, 0x4D, 0xFF));
	case EAffixRarity::Legendary:
		return FLinearColor(FColor(0xFF, 0xC1, 0x07));
	case EAffixRarity::Common:
	default:
		return FLinearColor(FColor(0xB0, 0xB0, 0xB0));
	}
}

FText UPixelShopSlotWidget::FormatScrapAmount(double Value)
{
	const double Cents = FMath::RoundToDouble(Value * 100.0) / 100.0;
	if (FMath::IsNearlyEqual(Cents, FMath::RoundToDouble(Cents), 0.001))
	{
		return FText::AsNumber(FMath::RoundToInt(Cents));
	}
	FString Text = FString::Printf(TEXT("%.2f"), Cents);
	while (Text.Len() > 0 && Text.EndsWith(TEXT("0")))
	{
		Text.LeftChopInline(1);
	}
	if (Text.EndsWith(TEXT(".")))
	{
		Text.LeftChopInline(1);
	}
	return FText::FromString(Text);
}
