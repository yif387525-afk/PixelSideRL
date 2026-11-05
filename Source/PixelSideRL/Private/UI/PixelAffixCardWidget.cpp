#include "UI/PixelAffixCardWidget.h"

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

namespace PixelAffixCardStyle
{
	const FLinearColor TextColor(FColor(0xF2, 0xF2, 0xF2));
	const FLinearColor CardFill(FColor(0x1A, 0x1A, 0x2E));
	const FLinearColor IconEmpty(FColor(0x4A, 0x4A, 0x4A));
	constexpr float CardWidth = 120.f;
	constexpr float CardHeight = 168.f;
	constexpr float IconSize = 32.f;

	void StyleText(UTextBlock* Text, const FSlateFontInfo& Font, const FLinearColor& Color, ETextJustify::Type Align, bool bWrap)
	{
		if (!Text)
		{
			return;
		}
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Align);
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
}

UPixelAffixCardWidget::UPixelAffixCardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = false;
	SetVisibility(ESlateVisibility::Visible);
}

void UPixelAffixCardWidget::SetChoiceIndex(int32 Index)
{
	ChoiceIndex = Index;
	if (Txt_Hotkey)
	{
		Txt_Hotkey->SetText(FText::AsNumber(Index + 1));
	}
}

void UPixelAffixCardWidget::ApplyCardView(const FAffixCardView& View)
{
	EnsureLayout();
	if (Txt_Name)
	{
		Txt_Name->SetText(View.Name);
	}
	if (Txt_Desc)
	{
		Txt_Desc->SetText(View.Desc);
		Txt_Desc->SetWrapTextAt(96.f);
	}
	if (Txt_Rarity)
	{
		Txt_Rarity->SetText(StaticEnum<EAffixRarity>()->GetDisplayNameTextByValue(static_cast<int64>(View.Rarity)));
		Txt_Rarity->SetColorAndOpacity(FSlateColor(RarityColor(View.Rarity)));
	}
	ApplyRarityChrome(View.Rarity);

	if (Img_Icon)
	{
		UTexture2D* Texture = nullptr;
		if (!View.Icon.IsNull())
		{
			Texture = View.Icon.Get();
			if (!Texture)
			{
				Texture = View.Icon.LoadSynchronous();
			}
		}
		if (Texture)
		{
			Img_Icon->SetBrushFromTexture(Texture, false);
			Img_Icon->SetDesiredSizeOverride(FVector2D(PixelAffixCardStyle::IconSize, PixelAffixCardStyle::IconSize));
			Img_Icon->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			// No T_UI_AffixPlaceholder asset in repo. Flat chip until 点阵 delivers one.
			Img_Icon->SetBrush(PixelAffixCardStyle::Solid(
				PixelAffixCardStyle::IconEmpty,
				FVector2D(PixelAffixCardStyle::IconSize, PixelAffixCardStyle::IconSize)));
			Img_Icon->SetColorAndOpacity(FLinearColor::White);
		}
	}

	if (Txt_Stacks)
	{
		if (View.bOwned && View.Stacks > 0)
		{
			Txt_Stacks->SetText(FText::Format(
				NSLOCTEXT("PixelAffixCard", "Stacks", "x{0}"),
				FText::AsNumber(View.Stacks)));
			Txt_Stacks->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Txt_Stacks->SetText(FText::GetEmpty());
			Txt_Stacks->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UPixelAffixCardWidget::SetFocused(bool bInFocused)
{
	bFocused = bInFocused;
	SetRenderTranslation(FVector2D(0.f, bFocused ? -2.f : 0.f));
	if (FocusRing)
	{
		FocusRing->SetPadding(FMargin(bFocused ? 1.f : 0.f));
		FocusRing->SetBrushColor(bFocused ? FLinearColor::White : FLinearColor::Transparent);
	}
	else if (Border_Rarity)
	{
		Border_Rarity->SetPadding(FMargin(bFocused ? 3.f : 2.f));
	}
}

void UPixelAffixCardWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
}

TSharedRef<SWidget> UPixelAffixCardWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelAffixCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureLayout();
	BindSelectButton();
}

void UPixelAffixCardWidget::HandleSelectClicked()
{
	OnCardSelected.Broadcast(ChoiceIndex);
}

void UPixelAffixCardWidget::EnsureLayout()
{
	const bool bRequired = Border_Rarity && Btn_Select && Txt_Hotkey && Img_Icon && Txt_Name && Txt_Desc && Txt_Rarity;
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
			TEXT("WBP_AffixCard is missing one of Border_Rarity, Btn_Select, Txt_Hotkey, Img_Icon, Txt_Name, Txt_Desc, Txt_Rarity."));
	}
}

void UPixelAffixCardWidget::BuildFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo HotkeyFont = PixelAffixMakeFont(11);
	const FSlateFontInfo NameFont = PixelAffixMakeFont(12);
	const FSlateFontInfo BodyFont = PixelAffixMakeFont(10);

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Card"));
	Size->SetWidthOverride(PixelAffixCardStyle::CardWidth);
	Size->SetHeightOverride(PixelAffixCardStyle::CardHeight);

	FocusRing = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Focus"));
	FocusRing->SetBrushColor(FLinearColor::Transparent);
	FocusRing->SetPadding(FMargin(0.f));
	Size->SetContent(FocusRing);

	Border_Rarity = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Rarity"));
	Border_Rarity->SetBrushColor(RarityColor(EAffixRarity::Common));
	Border_Rarity->SetPadding(FMargin(2.f));
	FocusRing->SetContent(Border_Rarity);

	UBorder* Body = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Body"));
	Body->SetBrushColor(PixelAffixCardStyle::CardFill);
	Body->SetPadding(FMargin(6.f, 16.f, 6.f, 6.f));
	Border_Rarity->SetContent(Body);

	UOverlay* Face = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_Face"));
	Body->SetContent(Face);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Card"));
	if (UOverlaySlot* ColumnSlot = Face->AddChildToOverlay(Column))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
		ColumnSlot->SetVerticalAlignment(VAlign_Fill);
	}

	Img_Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Icon"));
	Img_Icon->SetBrush(PixelAffixCardStyle::Solid(
		PixelAffixCardStyle::IconEmpty,
		FVector2D(PixelAffixCardStyle::IconSize, PixelAffixCardStyle::IconSize)));
	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Icon"));
	IconBox->SetWidthOverride(PixelAffixCardStyle::IconSize);
	IconBox->SetHeightOverride(PixelAffixCardStyle::IconSize);
	IconBox->SetContent(Img_Icon);
	if (UVerticalBoxSlot* IconSlot = Column->AddChildToVerticalBox(IconBox))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 6.f));
	}

	Txt_Name = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Name"));
	Txt_Desc = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Desc"));
	Txt_Rarity = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Rarity"));
	PixelAffixCardStyle::StyleText(Txt_Name, NameFont, PixelAffixCardStyle::TextColor, ETextJustify::Center, false);
	PixelAffixCardStyle::StyleText(Txt_Desc, BodyFont, PixelAffixCardStyle::TextColor, ETextJustify::Center, true);
	PixelAffixCardStyle::StyleText(Txt_Rarity, BodyFont, RarityColor(EAffixRarity::Common), ETextJustify::Center, false);
	Txt_Desc->SetWrapTextAt(96.f);

	auto AddText = [Column](UTextBlock* Text, float Bottom)
	{
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Text))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, Bottom));
		}
	};
	AddText(Txt_Name, 4.f);
	if (UVerticalBoxSlot* DescSlot = Column->AddChildToVerticalBox(Txt_Desc))
	{
		DescSlot->SetHorizontalAlignment(HAlign_Fill);
		DescSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	AddText(Txt_Rarity, 0.f);

	Btn_Select = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_Select"));
	FButtonStyle Style = Btn_Select->GetStyle();
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
	Btn_Select->SetStyle(Style);
	Btn_Select->SetBackgroundColor(FLinearColor::Transparent);
	if (UOverlaySlot* ButtonSlot = Face->AddChildToOverlay(Btn_Select))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		ButtonSlot->SetVerticalAlignment(VAlign_Fill);
	}

	Txt_Hotkey = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Hotkey"));
	Txt_Stacks = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Stacks"));
	PixelAffixCardStyle::StyleText(Txt_Hotkey, HotkeyFont, PixelAffixCardStyle::TextColor, ETextJustify::Left, false);
	PixelAffixCardStyle::StyleText(Txt_Stacks, HotkeyFont, PixelAffixCardStyle::TextColor, ETextJustify::Right, false);
	Txt_Hotkey->SetText(FText::AsNumber(ChoiceIndex + 1));
	Txt_Stacks->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* HotkeySlot = Face->AddChildToOverlay(Txt_Hotkey))
	{
		HotkeySlot->SetHorizontalAlignment(HAlign_Left);
		HotkeySlot->SetVerticalAlignment(VAlign_Top);
		HotkeySlot->SetPadding(FMargin(4.f, 2.f, 0.f, 0.f));
	}
	if (UOverlaySlot* StackSlot = Face->AddChildToOverlay(Txt_Stacks))
	{
		StackSlot->SetHorizontalAlignment(HAlign_Right);
		StackSlot->SetVerticalAlignment(VAlign_Top);
		StackSlot->SetPadding(FMargin(0.f, 2.f, 4.f, 0.f));
	}

	WidgetTree->RootWidget = Size;
	BindSelectButton();
}

void UPixelAffixCardWidget::BindSelectButton()
{
	if (!Btn_Select)
	{
		return;
	}
	Btn_Select->OnClicked.RemoveDynamic(this, &UPixelAffixCardWidget::HandleSelectClicked);
	Btn_Select->OnClicked.AddDynamic(this, &UPixelAffixCardWidget::HandleSelectClicked);
}

void UPixelAffixCardWidget::ApplyRarityChrome(EAffixRarity Rarity)
{
	if (Border_Rarity)
	{
		Border_Rarity->SetBrushColor(RarityColor(Rarity));
		if (!bFocused || FocusRing)
		{
			Border_Rarity->SetPadding(FMargin(2.f));
		}
	}
}

FLinearColor UPixelAffixCardWidget::RarityColor(EAffixRarity Rarity)
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
