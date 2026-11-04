#include "UI/PixelHUDWidget.h"

#include "UI/PixelAffixFont.h"
#include "UI/PixelHUDAffixIconWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "TimerManager.h"
#include "Widgets/SWidget.h"

namespace PixelHUD
{
	const FLinearColor TextColor(FColor(0xF2, 0xF2, 0xF2));
	const FLinearColor ScrapColor(FColor(0xFF, 0xD5, 0x4A));
	const FLinearColor HealthFill(FColor(0x2E, 0xE6, 0xA6));
	const FLinearColor HealthLow(FColor(0xFF, 0x3B, 0x3B));
	const FLinearColor HealthBack(FColor(0x3A, 0x3A, 0x3A));
	const FLinearColor WeaponChip(FColor(0xF2, 0xF2, 0xF2));

	constexpr int32 MaxAffixIcons = 8;
	constexpr float LowHealthRatio = 0.25f;
	constexpr float LowHealthFlashHz = 2.f;
	constexpr float HealthWidth = 96.f;
	constexpr float HealthHeight = 8.f;
	constexpr float WeaponSize = 16.f;
	constexpr float IconSpacing = 2.f;
	constexpr float SafeMargin = 8.f;
	constexpr float TopBarHeight = 28.f;
	constexpr float RefreshSeconds = 0.05f;

	void StyleText(UTextBlock* Text, const FSlateFontInfo& Font, const FLinearColor& Color)
	{
		if (!Text)
		{
			return;
		}
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(ETextJustify::Left);
		Text->SetShadowOffset(FVector2D::ZeroVector);
		Text->SetAutoWrapText(false);
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

	void PassThrough(UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}
		const ESlateVisibility Visibility = Widget->GetVisibility();
		if (Visibility == ESlateVisibility::Collapsed || Visibility == ESlateVisibility::Hidden)
		{
			return;
		}
		Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

UPixelHUDWidget::UPixelHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = false;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UPixelHUDWidget::SetHealthRatio(float Ratio)
{
	DemoHealth = FMath::Clamp(Ratio, 0.f, 1.f);
	RefreshHealthPresentation();
}

void UPixelHUDWidget::SetAmmo(int32 Ammo)
{
	DemoAmmo = Ammo;
	RefreshAmmoAndScrap();
}

void UPixelHUDWidget::SetScrap(int32 Scrap)
{
	DemoScrap = Scrap;
	RefreshAmmoAndScrap();
}

void UPixelHUDWidget::SetWeaponIcon(UTexture2D* Icon)
{
	DemoWeaponIcon = Icon;
	bHasWeaponOverride = true;
	bAppliedWeaponMissing = false;
	AppliedWeaponIcon = nullptr;
	RefreshWeaponIcon();
}

void UPixelHUDWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
	RefreshFromDemo();
}

TSharedRef<SWidget> UPixelHUDWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureLayout();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	RefreshFromDemo();
	if (IsDesignTime())
	{
		return;
	}
	TryBindSubsystem();
	RefreshAffixIcons();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimer);
		World->GetTimerManager().SetTimer(
			RefreshTimer,
			this,
			&UPixelHUDWidget::RefreshFromDemo,
			PixelHUD::RefreshSeconds,
			true);
	}
}

void UPixelHUDWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimer);
	}
	UnbindSubsystem();
	Super::NativeDestruct();
}

void UPixelHUDWidget::HandleAffixChanged(FAffixChangeEvent Event)
{
	UE_LOG(LogPixelHUD, Verbose, TEXT("WBP_HUD affix changed: %s stacks=%d removed=%s"),
		*Event.AffixId.ToString(),
		Event.Stacks,
		Event.bRemoved ? TEXT("true") : TEXT("false"));
	RefreshAffixIcons();
}

void UPixelHUDWidget::EnsureLayout()
{
	const bool bHaveShell = Bar_Health && Img_Weapon && Txt_Ammo && Txt_Coin && HB_AffixIcons;
	if (!bHaveShell)
	{
		if (WidgetTree && !WidgetTree->RootWidget)
		{
			BuildFallbackShell();
		}
		else if (!bWarnedMissingWidgets)
		{
			bWarnedMissingWidgets = true;
			UE_LOG(LogPixelHUD, Warning,
				TEXT("WBP_HUD is missing one of Bar_Health, Img_Weapon, Txt_Ammo, Txt_Coin, HB_AffixIcons."));
		}
	}
	EnsureAffixSlots();
	PixelHUD::PassThrough(Bar_Health);
	PixelHUD::PassThrough(Img_Weapon);
	PixelHUD::PassThrough(Txt_Ammo);
	PixelHUD::PassThrough(Txt_Coin);
	PixelHUD::PassThrough(HB_AffixIcons);
}

void UPixelHUDWidget::BuildFallbackShell()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo Font = PixelAffixMakeFont(11);

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas_HUD"));
	PixelHUD::PassThrough(Canvas);
	WidgetTree->RootWidget = Canvas;

	UHorizontalBox* TopBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Top"));
	PixelHUD::PassThrough(TopBar);

	Bar_Health = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("Bar_Health"));
	Bar_Health->SetPercent(1.f);
	FProgressBarStyle BarStyle = Bar_Health->GetWidgetStyle();
	BarStyle.BackgroundImage.TintColor = FSlateColor(PixelHUD::HealthBack);
	BarStyle.FillImage.TintColor = FSlateColor(FLinearColor::White);
	Bar_Health->SetWidgetStyle(BarStyle);
	Bar_Health->SetFillColorAndOpacity(PixelHUD::HealthFill);
	PixelHUD::PassThrough(Bar_Health);

	USizeBox* HealthSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Bar_Health"));
	HealthSize->SetWidthOverride(PixelHUD::HealthWidth);
	HealthSize->SetHeightOverride(PixelHUD::HealthHeight);
	PixelHUD::PassThrough(HealthSize);
	HealthSize->SetContent(Bar_Health);
	if (UHorizontalBoxSlot* HealthSlot = TopBar->AddChildToHorizontalBox(HealthSize))
	{
		HealthSlot->SetVerticalAlignment(VAlign_Top);
		HealthSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}

	Img_Weapon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Weapon"));
	Img_Weapon->SetBrush(PixelHUD::Solid(PixelHUD::WeaponChip, FVector2D(PixelHUD::WeaponSize, PixelHUD::WeaponSize)));
	PixelHUD::PassThrough(Img_Weapon);
	USizeBox* WeaponSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Img_Weapon"));
	WeaponSize->SetWidthOverride(PixelHUD::WeaponSize);
	WeaponSize->SetHeightOverride(PixelHUD::WeaponSize);
	PixelHUD::PassThrough(WeaponSize);
	WeaponSize->SetContent(Img_Weapon);
	if (UHorizontalBoxSlot* WeaponSlot = TopBar->AddChildToHorizontalBox(WeaponSize))
	{
		WeaponSlot->SetVerticalAlignment(VAlign_Top);
		WeaponSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
	}

	Txt_Ammo = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Ammo"));
	PixelHUD::StyleText(Txt_Ammo, Font, PixelHUD::TextColor);
	if (UHorizontalBoxSlot* AmmoSlot = TopBar->AddChildToHorizontalBox(Txt_Ammo))
	{
		AmmoSlot->SetVerticalAlignment(VAlign_Top);
		AmmoSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	}

	USpacer* Mid = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Spacer_Mid"));
	PixelHUD::PassThrough(Mid);
	if (UHorizontalBoxSlot* MidSlot = TopBar->AddChildToHorizontalBox(Mid))
	{
		MidSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	Txt_Coin = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Coin"));
	PixelHUD::StyleText(Txt_Coin, Font, PixelHUD::ScrapColor);
	if (UHorizontalBoxSlot* CoinSlot = TopBar->AddChildToHorizontalBox(Txt_Coin))
	{
		CoinSlot->SetVerticalAlignment(VAlign_Top);
		CoinSlot->SetPadding(FMargin(8.f, 2.f, 8.f, 0.f));
	}

	HB_AffixIcons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_AffixIcons"));
	PixelHUD::PassThrough(HB_AffixIcons);
	if (UHorizontalBoxSlot* IconSlot = TopBar->AddChildToHorizontalBox(HB_AffixIcons))
	{
		IconSlot->SetVerticalAlignment(VAlign_Top);
		IconSlot->SetHorizontalAlignment(HAlign_Right);
	}

	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(TopBar))
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
		PanelSlot->SetOffsets(FMargin(PixelHUD::SafeMargin, PixelHUD::SafeMargin, PixelHUD::SafeMargin, PixelHUD::TopBarHeight));
		PanelSlot->SetAutoSize(false);
	}

	bBuiltFallbackShell = true;
}

void UPixelHUDWidget::EnsureAffixSlots()
{
	if (!HB_AffixIcons)
	{
		return;
	}

	AffixSlots.Reset();
	int32 ForeignChildren = 0;
	const int32 ChildCount = HB_AffixIcons->GetChildrenCount();
	for (int32 Index = 0; Index < ChildCount; ++Index)
	{
		UWidget* Child = HB_AffixIcons->GetChildAt(Index);
		if (UPixelHUDAffixIconWidget* Icon = Cast<UPixelHUDAffixIconWidget>(Child))
		{
			if (AffixSlots.Num() < PixelHUD::MaxAffixIcons)
			{
				AffixSlots.Add(Icon);
			}
		}
		else
		{
			++ForeignChildren;
		}
	}

	if (ForeignChildren > 0 && AffixSlots.Num() == 0)
	{
		if (!bWarnedForeignAffixSlots)
		{
			bWarnedForeignAffixSlots = true;
			UE_LOG(LogPixelHUD, Warning,
				TEXT("WBP_HUD HB_AffixIcons has no WBP_HUD_AffixIcon children. Affix icons were not injected."));
		}
		return;
	}

	while (AffixSlots.Num() < PixelHUD::MaxAffixIcons && ForeignChildren == 0)
	{
		const int32 SlotIndex = AffixSlots.Num();
		const FName IconName(*FString::Printf(TEXT("AffixIcon_%d"), SlotIndex));
		UPixelHUDAffixIconWidget* Icon = CreateWidget<UPixelHUDAffixIconWidget>(
			this,
			UPixelHUDAffixIconWidget::StaticClass(),
			IconName);
		if (!Icon)
		{
			UE_LOG(LogPixelHUD, Warning, TEXT("WBP_HUD failed to create affix slot %d."), SlotIndex);
			break;
		}
		Icon->SetEmpty();
		if (UHorizontalBoxSlot* Slot = HB_AffixIcons->AddChildToHorizontalBox(Icon))
		{
			const float Right = (SlotIndex < PixelHUD::MaxAffixIcons - 1) ? PixelHUD::IconSpacing : 0.f;
			Slot->SetPadding(FMargin(0.f, 0.f, Right, 0.f));
			Slot->SetVerticalAlignment(VAlign_Top);
			Slot->SetHorizontalAlignment(HAlign_Center);
		}
		AffixSlots.Add(Icon);
	}

	for (UPixelHUDAffixIconWidget* Icon : AffixSlots)
	{
		PixelHUD::PassThrough(Icon);
	}
}

void UPixelHUDWidget::RefreshFromDemo()
{
	EnsureLayout();
	RefreshHealthPresentation();
	RefreshAmmoAndScrap();
	RefreshWeaponIcon();
}

void UPixelHUDWidget::RefreshHealthPresentation()
{
	if (!Bar_Health)
	{
		return;
	}

	const float Ratio = FMath::Clamp(DemoHealth, 0.f, 1.f);
	const bool bDead = Ratio <= 0.f;
	const bool bLow = !bDead && Ratio <= PixelHUD::LowHealthRatio;
	Bar_Health->SetPercent(bDead ? 0.f : Ratio);

	FLinearColor Fill = bLow ? PixelHUD::HealthLow : PixelHUD::HealthFill;
	if (bLow && PixelHUD::LowHealthFlashHz > 0.f)
	{
		const float Period = 1.f / PixelHUD::LowHealthFlashHz;
		const float Now = static_cast<float>(FPlatformTime::Seconds());
		const float Phase = FMath::Fmod(Now, Period);
		if (Phase >= Period * 0.5f)
		{
			Fill.A = 0.25f;
		}
	}
	Bar_Health->SetFillColorAndOpacity(Fill);
}

void UPixelHUDWidget::RefreshAmmoAndScrap()
{
	if (Txt_Ammo)
	{
		Txt_Ammo->SetText(FText::AsNumber(DemoAmmo));
	}
	if (Txt_Coin)
	{
		const FString Body = (DemoScrap >= 0)
			? FString::Printf(TEXT("Scrap %03d"), DemoScrap)
			: FString::Printf(TEXT("Scrap %d"), DemoScrap);
		Txt_Coin->SetText(FText::FromString(Body));
		Txt_Coin->SetColorAndOpacity(FSlateColor(PixelHUD::ScrapColor));
	}
}

void UPixelHUDWidget::RefreshWeaponIcon()
{
	if (!Img_Weapon)
	{
		return;
	}

	if (!bHasWeaponOverride && !bBuiltFallbackShell)
	{
		return;
	}

	const bool bMissing = DemoWeaponIcon == nullptr;
	if (bMissing && bAppliedWeaponMissing)
	{
		return;
	}
	if (!bMissing && AppliedWeaponIcon.Get() == DemoWeaponIcon)
	{
		return;
	}

	if (!bMissing)
	{
		Img_Weapon->SetBrushFromTexture(DemoWeaponIcon, false);
		Img_Weapon->SetDesiredSizeOverride(FVector2D(PixelHUD::WeaponSize, PixelHUD::WeaponSize));
		Img_Weapon->SetColorAndOpacity(FLinearColor::White);
		AppliedWeaponIcon = DemoWeaponIcon;
		bAppliedWeaponMissing = false;
		return;
	}

	Img_Weapon->SetBrush(PixelHUD::Solid(PixelHUD::WeaponChip, FVector2D(PixelHUD::WeaponSize, PixelHUD::WeaponSize)));
	Img_Weapon->SetColorAndOpacity(FLinearColor::White);
	AppliedWeaponIcon = nullptr;
	bAppliedWeaponMissing = true;
}

void UPixelHUDWidget::RefreshAffixIcons()
{
	EnsureAffixSlots();
	if (AffixSlots.Num() == 0)
	{
		return;
	}

	TArray<FAffixRuntimeEntry> Owned;
	UAffixSubsystem* Subsystem = FindConcreteSubsystem();
	if (Subsystem)
	{
		Owned = Subsystem->GetOwnedAffixes();
	}

	int32 Shown = 0;
	int32 Eligible = 0;
	for (const FAffixRuntimeEntry& Entry : Owned)
	{
		if (Entry.AffixId.IsNone())
		{
			continue;
		}
		++Eligible;
		if (Shown >= AffixSlots.Num())
		{
			continue;
		}

		UTexture2D* Icon = nullptr;
		if (Subsystem)
		{
			// Card view is only a way to read Icon. Name / Desc / Rarity / stacks stay off the strip.
			const FAffixCardView View = Subsystem->GetAffixCardViewFromId(Entry.AffixId);
			if (!View.Icon.IsNull())
			{
				Icon = View.Icon.Get();
				if (!Icon)
				{
					Icon = View.Icon.LoadSynchronous();
				}
			}
		}
		if (UPixelHUDAffixIconWidget* Slot = AffixSlots[Shown])
		{
			Slot->SetOccupied(Entry.AffixId, Icon);
		}
		++Shown;
	}

	if (Eligible > PixelHUD::MaxAffixIcons && !bLoggedAffixTruncation)
	{
		bLoggedAffixTruncation = true;
		UE_LOG(LogPixelHUD, Log, TEXT("WBP_HUD affix strip full (%d owned, showing %d)."), Eligible, PixelHUD::MaxAffixIcons);
	}

	for (int32 Index = Shown; Index < AffixSlots.Num(); ++Index)
	{
		if (UPixelHUDAffixIconWidget* Slot = AffixSlots[Index])
		{
			Slot->SetEmpty();
		}
	}
}

void UPixelHUDWidget::TryBindSubsystem()
{
	UnbindSubsystem();
	UAffixSubsystem* Subsystem = FindConcreteSubsystem();
	if (!Subsystem)
	{
		if (!bLoggedMissingSubsystem)
		{
			bLoggedMissingSubsystem = true;
			UE_LOG(LogPixelHUD, Log, TEXT("WBP_HUD: no concrete UAffixSubsystem yet. Affix icons stay empty until one exists."));
		}
		return;
	}
	Subsystem->OnAffixChanged.AddDynamic(this, &UPixelHUDWidget::HandleAffixChanged);
	BoundSubsystem = Subsystem;
}

void UPixelHUDWidget::UnbindSubsystem()
{
	if (UAffixSubsystem* Subsystem = BoundSubsystem.Get())
	{
		Subsystem->OnAffixChanged.RemoveDynamic(this, &UPixelHUDWidget::HandleAffixChanged);
	}
	BoundSubsystem = nullptr;
}

UAffixSubsystem* UPixelHUDWidget::FindConcreteSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}
	UAffixSubsystem* Subsystem = GameInstance->GetSubsystem<UAffixSubsystem>();
	if (!Subsystem || !Subsystem->GetClass() || Subsystem->GetClass()->HasAnyClassFlags(CLASS_Abstract))
	{
		return nullptr;
	}
	return Subsystem;
}
