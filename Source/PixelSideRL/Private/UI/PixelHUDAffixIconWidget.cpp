#include "UI/PixelHUDAffixIconWidget.h"

#include "UI/PixelAffixFont.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Widgets/SWidget.h"

namespace PixelHUDAffixIcon
{
	const FLinearColor EmptyColor(FColor(0x4A, 0x4A, 0x4A));
	const FLinearColor OwnedColor(FColor(0xF2, 0xF2, 0xF2));
	constexpr float SlotSize = 12.f;

	FSlateBrush Solid(const FLinearColor& Color)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(Color);
		Brush.ImageSize = FVector2D(SlotSize, SlotSize);
		return Brush;
	}
}

UPixelHUDAffixIconWidget::UPixelHUDAffixIconWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = false;
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPixelHUDAffixIconWidget::SetOccupied(FName InAffixId, UTexture2D* Icon)
{
	if (InAffixId.IsNone())
	{
		SetEmpty();
		return;
	}
	bOccupied = true;
	AffixId = InAffixId;
	IconTexture = Icon;
	ApplyVisual();
}

void UPixelHUDAffixIconWidget::SetEmpty()
{
	bOccupied = false;
	AffixId = NAME_None;
	IconTexture = nullptr;
	ApplyVisual();
}

void UPixelHUDAffixIconWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
	ApplyVisual();
}

TSharedRef<SWidget> UPixelHUDAffixIconWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelHUDAffixIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureLayout();
	ApplyVisual();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPixelHUDAffixIconWidget::EnsureLayout()
{
	if (Img_Icon)
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
		UE_LOG(LogPixelHUD, Warning, TEXT("WBP_HUD_AffixIcon is missing Img_Icon."));
	}
}

void UPixelHUDAffixIconWidget::BuildFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_AffixIcon"));
	Size->SetWidthOverride(PixelHUDAffixIcon::SlotSize);
	Size->SetHeightOverride(PixelHUDAffixIcon::SlotSize);
	Size->SetVisibility(ESlateVisibility::HitTestInvisible);

	Img_Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Icon"));
	Img_Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	Img_Icon->SetBrush(PixelHUDAffixIcon::Solid(PixelHUDAffixIcon::EmptyColor));
	Size->SetContent(Img_Icon);

	WidgetTree->RootWidget = Size;
}

void UPixelHUDAffixIconWidget::ApplyVisual()
{
	EnsureLayout();
	if (!Img_Icon)
	{
		return;
	}

	if (bOccupied && IconTexture)
	{
		Img_Icon->SetBrushFromTexture(IconTexture, false);
		Img_Icon->SetDesiredSizeOverride(FVector2D(PixelHUDAffixIcon::SlotSize, PixelHUDAffixIcon::SlotSize));
		Img_Icon->SetColorAndOpacity(FLinearColor::White);
		return;
	}

	const FLinearColor Color = bOccupied ? PixelHUDAffixIcon::OwnedColor : PixelHUDAffixIcon::EmptyColor;
	Img_Icon->SetBrush(PixelHUDAffixIcon::Solid(Color));
	Img_Icon->SetColorAndOpacity(FLinearColor::White);
}
