#include "UI/PixelDebugCombatHUD.h"

#include "Blueprint/WidgetTree.h"
#include "Combat/CombatDebugReadComponent.h"
#include "Combat/PixelSideCharacter.h"
#include "Combat/PixelSideCombatLog.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "Widgets/SWidget.h"

namespace PixelDebugHud
{
	const FLinearColor TextColor = FLinearColor(FColor(0xF2, 0xF2, 0xF2));
	const FLinearColor RecentHitColor = FLinearColor(FColor(0xFF, 0x3B, 0x3B));
	const FLinearColor BarFill = FLinearColor(FColor(0x8F, 0xE0, 0x6A));
	const FLinearColor BarBack = FLinearColor(FColor(0x22, 0x22, 0x22));
	constexpr float RefreshInterval = 0.05f;
	constexpr float IFrameBlinkHz = 2.f;
	constexpr float RecentHitSeconds = 0.5f;
	constexpr float AnchorX = 8.f;
	constexpr float AnchorY = 40.f;

	FString FormatNumber(float Value)
	{
		const float Rounded = FMath::RoundToFloat(Value);
		if (FMath::IsNearlyEqual(Value, Rounded, 0.05f))
		{
			return FString::FromInt(FMath::RoundToInt(Rounded));
		}
		return FString::Printf(TEXT("%.1f"), Value);
	}

	void SetLine(UTextBlock* Text, const FString& Line, const FLinearColor& Color)
	{
		if (!Text)
		{
			return;
		}
		Text->SetText(FText::FromString(Line));
		Text->SetColorAndOpacity(FSlateColor(Color));
	}

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
	}
}

UPixelDebugCombatHUD::UPixelDebugCombatHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = false;
#if UE_BUILD_SHIPPING
	bEnableCombatDebugHUD = false;
	SetVisibility(ESlateVisibility::Collapsed);
#else
	SetVisibility(ESlateVisibility::HitTestInvisible);
#endif
}

UPixelDebugCombatHUD* UPixelDebugCombatHUD::MountForPlayer(
	APlayerController* OwningPlayer,
	AActor* InObservedActor,
	TSubclassOf<UPixelDebugCombatHUD> WidgetClass)
{
#if UE_BUILD_SHIPPING
	(void)OwningPlayer;
	(void)InObservedActor;
	(void)WidgetClass;
	return nullptr;
#else
	if (!OwningPlayer || !OwningPlayer->IsLocalPlayerController())
	{
		return nullptr;
	}
	if (const APixelSideCharacter* Player = Cast<APixelSideCharacter>(InObservedActor))
	{
		if (!Player->IsCombatDebugHUDEnabled())
		{
			return nullptr;
		}
	}
	if (!WidgetClass)
	{
		WidgetClass = StaticClass();
	}

	UPixelDebugCombatHUD* Widget = CreateWidget<UPixelDebugCombatHUD>(OwningPlayer, WidgetClass);
	if (!Widget)
	{
		return nullptr;
	}
	Widget->SetObservedActor(InObservedActor);
	Widget->AddToViewport(50);
	return Widget;
#endif
}

void UPixelDebugCombatHUD::SetObservedActor(AActor* Actor)
{
	ObservedActor = Actor;
}

void UPixelDebugCombatHUD::NativePreConstruct()
{
	Super::NativePreConstruct();
#if !UE_BUILD_SHIPPING
	EnsureLayout();
#endif
}

TSharedRef<SWidget> UPixelDebugCombatHUD::RebuildWidget()
{
#if !UE_BUILD_SHIPPING
	EnsureLayout();
#endif
	return Super::RebuildWidget();
}

void UPixelDebugCombatHUD::NativeConstruct()
{
	Super::NativeConstruct();

#if UE_BUILD_SHIPPING
	SetVisibility(ESlateVisibility::Collapsed);
	return;
#else
	EnsureLayout();
	const bool bRequiredBound = Txt_HP && Bar_HP && Txt_IFrame && Txt_Weapon && Txt_CD && Txt_LastDmg;
	if (!bRequiredBound && !bWarnedMissingWidgets)
	{
		bWarnedMissingWidgets = true;
		UE_LOG(LogPixelSideCombat, Warning,
			TEXT("WBP_DebugCombatHUD is missing one of Txt_HP, Bar_HP, Txt_IFrame, Txt_Weapon, Txt_CD, Txt_LastDmg."));
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshHandle);
		World->GetTimerManager().SetTimer(
			RefreshHandle,
			FTimerDelegate::CreateUObject(this, &UPixelDebugCombatHUD::RefreshFromSource),
			PixelDebugHud::RefreshInterval,
			true);
	}
	RefreshFromSource();
#endif
}

void UPixelDebugCombatHUD::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshHandle);
	}
	Super::NativeDestruct();
}

bool UPixelDebugCombatHUD::IsDebugHUDAllowed()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	if (!bEnableCombatDebugHUD)
	{
		return false;
	}
	if (const APixelSideCharacter* Player = Cast<APixelSideCharacter>(ResolveObservedActor()))
	{
		return Player->IsCombatDebugHUDEnabled();
	}
	return true;
#endif
}

AActor* UPixelDebugCombatHUD::ResolveObservedActor()
{
	if (AActor* Pinned = ObservedActor.Get())
	{
		return Pinned;
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		return PC->GetPawn();
	}
	return nullptr;
}

void UPixelDebugCombatHUD::EnsureLayout()
{
	const bool bRequiredBound = Txt_HP && Bar_HP && Txt_IFrame && Txt_Weapon && Txt_CD && Txt_LastDmg;
	if (bRequiredBound)
	{
		return;
	}

	// Native class has no designer tree. Build the named controls before slate takes the root.
	// A child WBP already has a root; do not replace it.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildFallbackLayout();
	}
}

void UPixelDebugCombatHUD::BuildFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), 12);
	const FLinearColor DimBack(0.f, 0.f, 0.f, 0.55f);

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas_Debug"));
	WidgetTree->RootWidget = Canvas;

	UBorder* Back = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Back"));
	Back->SetBrushColor(DimBack);
	Back->SetPadding(FMargin(6.f, 4.f));

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox_Lines"));
	USizeBox* ColumnSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Lines"));
	ColumnSize->SetMinDesiredWidth(168.f);
	ColumnSize->AddChild(Column);
	Back->AddChild(ColumnSize);

	UHorizontalBox* HpRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HBox_HP"));
	Txt_HP = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_HP"));
	PixelDebugHud::StyleText(Txt_HP, Font, PixelDebugHud::TextColor);
	if (UHorizontalBoxSlot* HpTextSlot = HpRow->AddChildToHorizontalBox(Txt_HP))
	{
		HpTextSlot->SetVerticalAlignment(VAlign_Center);
		HpTextSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
	}

	Bar_HP = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("Bar_HP"));
	Bar_HP->SetPercent(1.f);
	Bar_HP->SetFillColorAndOpacity(PixelDebugHud::BarFill);
	FProgressBarStyle BarStyle = Bar_HP->GetWidgetStyle();
	BarStyle.BackgroundImage.TintColor = FSlateColor(PixelDebugHud::BarBack);
	BarStyle.FillImage.TintColor = FSlateColor(PixelDebugHud::BarFill);
	Bar_HP->SetWidgetStyle(BarStyle);

	USizeBox* BarSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Bar_HP"));
	BarSize->SetWidthOverride(72.f);
	BarSize->SetHeightOverride(8.f);
	BarSize->AddChild(Bar_HP);
	if (UHorizontalBoxSlot* BarSlot = HpRow->AddChildToHorizontalBox(BarSize))
	{
		BarSlot->SetVerticalAlignment(VAlign_Center);
	}

	auto AddRow = [Column](UWidget* Row)
	{
		if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 1.f));
			RowSlot->SetHorizontalAlignment(HAlign_Left);
		}
	};

	AddRow(HpRow);

	Txt_IFrame = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_IFrame"));
	Txt_Weapon = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Weapon"));
	Txt_CD = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_CD"));
	Txt_LastDmg = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_LastDmg"));
	Txt_Alive = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Alive"));
	PixelDebugHud::StyleText(Txt_IFrame, Font, PixelDebugHud::TextColor);
	PixelDebugHud::StyleText(Txt_Weapon, Font, PixelDebugHud::TextColor);
	PixelDebugHud::StyleText(Txt_CD, Font, PixelDebugHud::TextColor);
	PixelDebugHud::StyleText(Txt_LastDmg, Font, PixelDebugHud::TextColor);
	PixelDebugHud::StyleText(Txt_Alive, Font, PixelDebugHud::RecentHitColor);
	AddRow(Txt_IFrame);
	AddRow(Txt_Weapon);
	AddRow(Txt_CD);
	AddRow(Txt_LastDmg);
	AddRow(Txt_Alive);

	if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Back))
	{
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
		PanelSlot->SetPosition(FVector2D(PixelDebugHud::AnchorX, PixelDebugHud::AnchorY));
		PanelSlot->SetAutoSize(true);
	}
}

void UPixelDebugCombatHUD::RefreshFromSource()
{
	if (!IsDebugHUDAllowed())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	const FPixelCombatDebugView View = UCombatDebugReadComponent::ReadActor(ResolveObservedActor());
	const FLinearColor Idle = PixelDebugHud::TextColor;
	FLinearColor Dim = Idle;
	Dim.A = 0.4f;

	if (!View.bHasDamageable)
	{
		PixelDebugHud::SetLine(Txt_HP, TEXT("HP  — / —"), Idle);
		PixelDebugHud::SetLine(Txt_IFrame, TEXT("IFR —"), Idle);
		PixelDebugHud::SetLine(Txt_LastDmg, TEXT("DMG —"), Dim);
		if (Bar_HP)
		{
			Bar_HP->SetPercent(0.f);
		}
		if (Txt_IFrame)
		{
			Txt_IFrame->SetRenderOpacity(1.f);
		}
		if (Txt_Alive)
		{
			Txt_Alive->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		PixelDebugHud::SetLine(
			Txt_HP,
			FString::Printf(TEXT("HP  %s / %s"), *PixelDebugHud::FormatNumber(View.Health), *PixelDebugHud::FormatNumber(View.MaxHealth)),
			Idle);
		PixelDebugHud::SetLine(
			Txt_IFrame,
			FString::Printf(TEXT("IFR %.2fs"), View.IFrameRemaining),
			Idle);
		if (Bar_HP)
		{
			const float Ratio = (View.MaxHealth > KINDA_SMALL_NUMBER)
				? FMath::Clamp(View.Health / View.MaxHealth, 0.f, 1.f)
				: 0.f;
			Bar_HP->SetPercent(Ratio);
		}
		if (Txt_IFrame)
		{
			const bool bBlink = View.IFrameRemaining > 0.f;
			float Opacity = 1.f;
			if (bBlink)
			{
				const UWorld* World = GetWorld();
				const float Now = World ? World->GetTimeSeconds() : 0.f;
				const float Period = 1.f / PixelDebugHud::IFrameBlinkHz;
				Opacity = (FMath::Fmod(Now, Period) < Period * 0.5f) ? 1.f : 0.15f;
			}
			Txt_IFrame->SetRenderOpacity(Opacity);
		}

		const bool bRecent = View.LastDamageTime >= 0.f
			&& View.LastDamageTaken > 0.f
			&& GetWorld()
			&& (GetWorld()->GetTimeSeconds() - View.LastDamageTime) <= PixelDebugHud::RecentHitSeconds;
		if (View.LastDamageTime < 0.f)
		{
			PixelDebugHud::SetLine(Txt_LastDmg, TEXT("DMG —"), Dim);
		}
		else
		{
			PixelDebugHud::SetLine(
				Txt_LastDmg,
				FString::Printf(TEXT("DMG -%s @ %.1fs"), *PixelDebugHud::FormatNumber(View.LastDamageTaken), View.LastDamageTime),
				bRecent ? PixelDebugHud::RecentHitColor : Dim);
		}

		if (Txt_Alive)
		{
			Txt_Alive->SetText(FText::FromString(TEXT("DEAD")));
			Txt_Alive->SetColorAndOpacity(FSlateColor(PixelDebugHud::RecentHitColor));
			Txt_Alive->SetVisibility(View.bAlive ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		}
	}

	if (!View.bHasWeapon)
	{
		PixelDebugHud::SetLine(Txt_Weapon, TEXT("WPN —"), Idle);
		PixelDebugHud::SetLine(Txt_CD, TEXT("CD  — / —"), Idle);
		return;
	}

	const FString WeaponLabel = View.WeaponName.IsEmpty() ? FString(TEXT("—")) : View.WeaponName;
	PixelDebugHud::SetLine(Txt_Weapon, FString::Printf(TEXT("WPN %s"), *WeaponLabel), Idle);
	PixelDebugHud::SetLine(
		Txt_CD,
		FString::Printf(TEXT("CD  %.2f / %.2f"), View.FireCooldownRemaining, View.ResolvedFireCooldown),
		Idle);
}
