#include "UI/PixelEventChoiceButton.h"

#include "UI/PixelAffixFont.h"
#include "UI/PixelEventChoiceWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

namespace
{
	const FLinearColor LabelColor(FColor(0xF2, 0xF2, 0xF2));
	const FLinearColor FillColor(FColor(0x1A, 0x1A, 0x2E));
	const FLinearColor HoverColor(FColor(0x2C, 0x2C, 0x48));
	const FLinearColor PressedColor(FColor(0x3D, 0x7E, 0xFF));

	void StyleEventChoiceLine(UTextBlock* Text, const FSlateFontInfo& Font)
	{
		if (!Text)
		{
			return;
		}
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(LabelColor));
		Text->SetJustification(ETextJustify::Left);
		Text->SetShadowOffset(FVector2D::ZeroVector);
		Text->SetAutoWrapText(false);
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	void PaintButton(UButton* Button)
	{
		if (!Button)
		{
			return;
		}
		FButtonStyle Style = Button->GetStyle();
		auto Solid = [](FSlateBrush& Brush, const FLinearColor& Color)
		{
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.TintColor = FSlateColor(Color);
			Brush.ImageSize = FVector2D(280.f, 28.f);
		};
		Solid(Style.Normal, FillColor);
		Solid(Style.Hovered, HoverColor);
		Solid(Style.Pressed, PressedColor);
		Solid(Style.Disabled, FillColor);
		Style.NormalPadding = FMargin(10.f, 4.f);
		Style.PressedPadding = FMargin(10.f, 5.f);
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::White);
		Button->SetIsFocusable(false);
	}
}

UPixelEventChoiceButton::UPixelEventChoiceButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = false;
	SetVisibility(ESlateVisibility::Visible);
}

void UPixelEventChoiceButton::SetOption(FName InChoiceId, const FText& InLabel, int32 InHotkey)
{
	ChoiceId = InChoiceId;
	HotkeyNumber = FMath::Max(1, InHotkey);
	EnsureLayout();
	if (Txt_Label)
	{
		Txt_Label->SetText(InLabel);
	}
	if (Txt_Hotkey)
	{
		Txt_Hotkey->SetText(FText::AsNumber(HotkeyNumber));
	}
}

void UPixelEventChoiceButton::SetFocused(bool bInFocused)
{
	bFocused = bInFocused;
	if (FocusRing)
	{
		FocusRing->SetPadding(FMargin(bFocused ? 1.f : 0.f));
		FocusRing->SetBrushColor(bFocused ? FLinearColor::White : FLinearColor::Transparent);
	}
}

void UPixelEventChoiceButton::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
}

TSharedRef<SWidget> UPixelEventChoiceButton::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelEventChoiceButton::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureLayout();
	BindSelectButton();
}

void UPixelEventChoiceButton::HandleSelectClicked()
{
	if (ChoiceId.IsNone())
	{
		return;
	}
	OnChoiceClicked.Broadcast(ChoiceId);
}

void UPixelEventChoiceButton::EnsureLayout()
{
	const bool bRequired = Btn_Select && Txt_Hotkey && Txt_Label;
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
		UE_LOG(LogPixelEventUI, Warning,
			TEXT("WBP_EventChoiceButton is missing one of Btn_Select, Txt_Hotkey, Txt_Label."));
	}
}

void UPixelEventChoiceButton::BuildFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo Font = PixelAffixMakeFont(12);

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Choice"));
	Size->SetHeightOverride(32.f);

	FocusRing = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Focus"));
	FocusRing->SetBrushColor(FLinearColor::Transparent);
	FocusRing->SetPadding(FMargin(0.f));
	Size->SetContent(FocusRing);

	UOverlay* Face = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_Choice"));
	FocusRing->SetContent(Face);

	Btn_Select = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_Select"));
	PaintButton(Btn_Select);
	if (UOverlaySlot* ButtonSlot = Face->AddChildToOverlay(Btn_Select))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		ButtonSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Label"));
	Row->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* RowSlot = Face->AddChildToOverlay(Row))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
		RowSlot->SetVerticalAlignment(VAlign_Center);
		RowSlot->SetPadding(FMargin(10.f, 0.f, 10.f, 0.f));
	}

	Txt_Hotkey = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Hotkey"));
	Txt_Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Label"));
	StyleEventChoiceLine(Txt_Hotkey, Font);
	StyleEventChoiceLine(Txt_Label, Font);
	Txt_Hotkey->SetText(FText::AsNumber(HotkeyNumber));

	if (UHorizontalBoxSlot* HotkeySlot = Row->AddChildToHorizontalBox(Txt_Hotkey))
	{
		HotkeySlot->SetVerticalAlignment(VAlign_Center);
		HotkeySlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}
	if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Txt_Label))
	{
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	WidgetTree->RootWidget = Size;
	BindSelectButton();
}

void UPixelEventChoiceButton::BindSelectButton()
{
	if (!Btn_Select)
	{
		return;
	}
	Btn_Select->OnClicked.RemoveDynamic(this, &UPixelEventChoiceButton::HandleSelectClicked);
	Btn_Select->OnClicked.AddDynamic(this, &UPixelEventChoiceButton::HandleSelectClicked);
}
