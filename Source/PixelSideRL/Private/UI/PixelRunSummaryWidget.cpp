#include "UI/PixelRunSummaryWidget.h"

#include "Affix/AffixSubsystemDefault.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/SlateTypes.h"
#include "UI/PixelAffixFont.h"
#include "Widgets/SWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelRunSummaryUI, Log, All);

namespace
{
	constexpr int32 SummaryViewportZ = 90;
	constexpr int32 MaxBuildChips = 8;
	constexpr int32 MaxNameChars = 8;

	const FText OutcomeWin = NSLOCTEXT("PixelRunSummary", "OutcomeWin", "胜利");
	const FText OutcomeFail = NSLOCTEXT("PixelRunSummary", "OutcomeFail", "失败");
	const FText ContinueLabel = NSLOCTEXT("PixelRunSummary", "Continue", "回据点");
	const FText HintLabel = NSLOCTEXT("PixelRunSummary", "Hint", "Enter / Space / 点击回据点 · Esc 同确认");
	const FText EmptyBuild = NSLOCTEXT("PixelRunSummary", "EmptyBuild", "（无词条）");
	const FText MoreBuild = NSLOCTEXT("PixelRunSummary", "MoreBuild", "…");
	const FText ClearTip = NSLOCTEXT("PixelRunSummary", "ClearTip", "局内 Scrap / 词条 / 临时枪已清空");
	const FText BuildCaption = NSLOCTEXT("PixelRunSummary", "Build", "构筑");
	const FText DemoWinLabel = NSLOCTEXT("PixelRunSummary", "DemoWin", "样例·胜");
	const FText DemoFailLabel = NSLOCTEXT("PixelRunSummary", "DemoFail", "样例·败");

	const FLinearColor Ink(FColor(0xF2, 0xF2, 0xF2));
	const FLinearColor WinColor(FColor(0x2E, 0xE6, 0xA6));
	const FLinearColor FailColor(FColor(0xFF, 0x3B, 0x3B));
	const FLinearColor ScrapColor(FColor(0xFF, 0xD5, 0x4A));
	const FLinearColor CoreColor(FColor(0xFF, 0xFF, 0xFF));
	const FLinearColor TipColor(FColor(0xB0, 0xB0, 0xB0));

	void StyleLine(UTextBlock* Text, const FSlateFontInfo& Font, const FLinearColor& Color, ETextJustify::Type Align)
	{
		if (!Text)
		{
			return;
		}
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Align);
		Text->SetShadowOffset(FVector2D::ZeroVector);
		Text->SetAutoWrapText(false);
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	void StyleSummaryButton(UButton* Button)
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
		Face(Style.Normal, FLinearColor(FColor(0x2C, 0x2C, 0x44)));
		Face(Style.Hovered, FLinearColor(FColor(0x3E, 0x3E, 0x5C)));
		Face(Style.Pressed, FLinearColor(FColor(0x22, 0x22, 0x34)));
		Face(Style.Disabled, FLinearColor(FColor(0x2A, 0x2A, 0x2A)));
		Style.NormalPadding = FMargin(8.f, 4.f);
		Style.PressedPadding = FMargin(8.f, 5.f, 8.f, 3.f);
		const FSlateColor ButtonInk(Ink);
		Style.NormalForeground = ButtonInk;
		Style.HoveredForeground = ButtonInk;
		Style.PressedForeground = ButtonInk;
		Style.DisabledForeground = FSlateColor(FLinearColor(FColor(0x88, 0x88, 0x88)));
		Button->SetStyle(Style);
	}

	FText FormatClock(int32 DurationSec)
	{
		const int32 AbsSec = FMath::Abs(DurationSec);
		const int32 Minutes = AbsSec / 60;
		const int32 Seconds = AbsSec % 60;
		const FString Clock = FString::Printf(TEXT("%s%02d:%02d"), DurationSec < 0 ? TEXT("-") : TEXT(""), Minutes, Seconds);
		return FText::FromString(Clock);
	}

	FText FormatSigned(int32 Value)
	{
		return FText::FromString(FString::Printf(TEXT("%+d"), Value));
	}

	FString ShortenName(const FString& Name)
	{
		if (Name.Len() <= MaxNameChars)
		{
			return Name;
		}
		return Name.Left(MaxNameChars - 1) + TEXT("…");
	}

	void CollectLabels(const FRunSummaryView& InView, TArray<FText>& OutLabels, TArray<FString>& OutTips)
	{
		const int32 Count = FMath::Max(InView.AffixIds.Num(), InView.AffixShortNames.Num());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FName Id = InView.AffixIds.IsValidIndex(Index) ? InView.AffixIds[Index] : NAME_None;
			FText Label;
			if (InView.AffixShortNames.IsValidIndex(Index) && !InView.AffixShortNames[Index].IsEmpty())
			{
				Label = InView.AffixShortNames[Index];
			}
			else if (!Id.IsNone())
			{
				Label = FText::FromName(Id);
			}
			if (Label.IsEmpty())
			{
				continue;
			}
			OutLabels.Add(Label);
			OutTips.Add(Id.IsNone() ? Label.ToString() : FString::Printf(TEXT("%s (%s)"), *Label.ToString(), *Id.ToString()));
		}
	}
}

UPixelRunSummaryWidget::UPixelRunSummaryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = true;
	SetVisibility(ESlateVisibility::Collapsed);
}

FRunSummaryView UPixelRunSummaryWidget::MakeDemoWinView()
{
	FRunSummaryView Demo;
	Demo.bVictory = true;
	Demo.DurationSec = 16 * 60 + 20;
	Demo.KillCount = 86;
	Demo.FloorReached = 2;
	Demo.FloorMax = 2;
	Demo.BossKills = 2;
	Demo.ScrapRemain = 40;
	Demo.CoreGained = 42;
	Demo.AffixIds = {FName(TEXT("Demo_PeaVolley")), FName(TEXT("Demo_BurstCore")), FName(TEXT("Demo_LeechCoat"))};
	Demo.AffixShortNames = {
		NSLOCTEXT("PixelRunSummary", "DemoPea", "豌豆连射"),
		NSLOCTEXT("PixelRunSummary", "DemoBurst", "爆裂弹芯"),
		NSLOCTEXT("PixelRunSummary", "DemoLeech", "吸血涂层")
	};
	return Demo;
}

FRunSummaryView UPixelRunSummaryWidget::MakeDemoFailView()
{
	FRunSummaryView Demo;
	Demo.bVictory = false;
	Demo.DurationSec = 9 * 60 + 5;
	Demo.KillCount = 31;
	Demo.FloorReached = 1;
	Demo.FloorMax = 2;
	Demo.BossKills = 0;
	Demo.ScrapRemain = 12;
	Demo.CoreGained = 8;
	Demo.AffixIds = {FName(TEXT("Demo_PeaVolley"))};
	Demo.AffixShortNames = {NSLOCTEXT("PixelRunSummary", "DemoPea", "豌豆连射")};
	return Demo;
}

void UPixelRunSummaryWidget::OpenWin()
{
	Present(MakeDemoWinView(), true);
}

void UPixelRunSummaryWidget::OpenFail()
{
	Present(MakeDemoFailView(), true);
}

void UPixelRunSummaryWidget::Present(const FRunSummaryView& InView, bool bPause)
{
	if (IsDesignTime())
	{
		return;
	}
	if (State == EPixelRunSummaryState::Leaving)
	{
		UE_LOG(LogPixelRunSummaryUI, Warning, TEXT("RunSummary ignored Present while leaving."));
		return;
	}
	if (IsAffixChoicePending())
	{
		UE_LOG(LogPixelRunSummaryUI, Warning, TEXT("RunSummary did not open: an affix choice is pending."));
		return;
	}
	if (!GetOwningPlayer())
	{
		UE_LOG(LogPixelRunSummaryUI, Warning, TEXT("RunSummary has no owning player. Create it with a PlayerController."));
		return;
	}

	EnsureLayout();
	BindInteractions();
	EnsureBuildChips();
	View = InView;
	State = InView.bVictory ? EPixelRunSummaryState::Open_Win : EPixelRunSummaryState::Open_Fail;
	Refresh();
	SetVisibility(ESlateVisibility::Visible);
	if (!IsInViewport())
	{
		AddToViewport(SummaryViewportZ);
	}
	AcquireInput();
	if (bPause)
	{
		BeginDemoPause();
	}
	UE_LOG(LogPixelRunSummaryUI, Log,
		TEXT("RunSummary open %s time=%d kills=%d floor=%d/%d boss=%d scrapRemain=%d coreGained=%d affixes=%d ownsPause=%s. Display only. No AddCore, no save."),
		InView.bVictory ? TEXT("win") : TEXT("fail"),
		InView.DurationSec,
		InView.KillCount,
		InView.FloorReached,
		InView.FloorMax,
		InView.BossKills,
		InView.ScrapRemain,
		InView.CoreGained,
		FMath::Max(InView.AffixIds.Num(), InView.AffixShortNames.Num()),
		bOwnsPause ? TEXT("true") : TEXT("false"));
}

void UPixelRunSummaryWidget::Confirm()
{
	if (State != EPixelRunSummaryState::Open_Win && State != EPixelRunSummaryState::Open_Fail)
	{
		return;
	}
	State = EPixelRunSummaryState::Leaving;
	if (Btn_Continue)
	{
		Btn_Continue->SetIsEnabled(false);
	}
	if (!IsDesignTime())
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
	ReleaseInput();
	EndDemoPause();
	State = EPixelRunSummaryState::Closed;
	UE_LOG(LogPixelRunSummaryUI, Log, TEXT("RunSummary continue. OnContinueToHub only. Core was not written."));
	OnContinueToHub.Broadcast();
}

bool UPixelRunSummaryWidget::IsOpen() const
{
	return State == EPixelRunSummaryState::Open_Win || State == EPixelRunSummaryState::Open_Fail;
}

bool UPixelRunSummaryWidget::IsReady() const
{
	return IsOpen();
}

void UPixelRunSummaryWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
	if (IsDesignTime())
	{
		ShowDesignWin();
	}
}

TSharedRef<SWidget> UPixelRunSummaryWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelRunSummaryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureLayout();
	BindInteractions();
	if (IsDesignTime())
	{
		ShowDesignWin();
		return;
	}
	if (!IsOpen())
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPixelRunSummaryWidget::NativeDestruct()
{
	if (Btn_Continue)
	{
		Btn_Continue->OnClicked.RemoveDynamic(this, &UPixelRunSummaryWidget::HandleContinueClicked);
	}
	if (Btn_DemoWin)
	{
		Btn_DemoWin->OnClicked.RemoveDynamic(this, &UPixelRunSummaryWidget::HandleDemoWinClicked);
	}
	if (Btn_DemoFail)
	{
		Btn_DemoFail->OnClicked.RemoveDynamic(this, &UPixelRunSummaryWidget::HandleDemoFailClicked);
	}
	if (IsOpen() || State == EPixelRunSummaryState::Leaving || bOwnsPause)
	{
		State = EPixelRunSummaryState::Closed;
		EndDemoPause();
		UE_LOG(LogPixelRunSummaryUI, Log, TEXT("RunSummary destroyed while open. Demo pause released if this shell owned it."));
	}
	ReleaseInput();
	Super::NativeDestruct();
}

FReply UPixelRunSummaryWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!IsOpen())
	{
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Escape)
	{
		Confirm();
		return FReply::Handled();
	}
	return FReply::Handled();
}

void UPixelRunSummaryWidget::HandleContinueClicked()
{
	Confirm();
}

void UPixelRunSummaryWidget::HandleDemoWinClicked()
{
	OpenWin();
}

void UPixelRunSummaryWidget::HandleDemoFailClicked()
{
	OpenFail();
}

void UPixelRunSummaryWidget::ShowDesignWin()
{
	EnsureBuildChips();
	View = MakeDemoWinView();
	State = EPixelRunSummaryState::Closed;
	Refresh();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UPixelRunSummaryWidget::EnsureLayout()
{
	const bool bHaveShell = Overlay_Root && Img_Dim && Txt_Outcome && Txt_Time && Txt_Kills && WB_Build
		&& Txt_Floor && Txt_BossKills && Txt_ScrapLeft && Txt_CoreGain && Txt_CoreTip && Btn_Continue;
	if (!bHaveShell)
	{
		if (WidgetTree && !WidgetTree->RootWidget)
		{
			BuildFallbackShell();
		}
		else if (!bWarnedMissingWidgets)
		{
			bWarnedMissingWidgets = true;
			UE_LOG(LogPixelRunSummaryUI, Warning,
				TEXT("WBP_RunSummary is missing one of Overlay_Root, Img_Dim, Txt_Outcome, Txt_Time, Txt_Kills, WB_Build, Txt_Floor, Txt_BossKills, Txt_ScrapLeft, Txt_CoreGain, Txt_CoreTip, Btn_Continue."));
		}
	}
}

void UPixelRunSummaryWidget::BuildFallbackShell()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo OutcomeFont = PixelAffixMakeFont(22);
	const FSlateFontInfo CoreFont = PixelAffixMakeFont(28);
	const FSlateFontInfo BodyFont = PixelAffixMakeFont(13);
	const FSlateFontInfo SmallFont = PixelAffixMakeFont(12);
	const FSlateFontInfo TipFont = PixelAffixMakeFont(10);
	const FSlateFontInfo ButtonFont = PixelAffixMakeFont(12);
	const FSlateFontInfo HintFont = PixelAffixMakeFont(11);

	Overlay_Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_Root"));
	Overlay_Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Overlay_Root;

	Img_Dim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Dim"));
	FSlateBrush DimBrush;
	DimBrush.DrawAs = ESlateBrushDrawType::Box;
	DimBrush.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.94f));
	Img_Dim->SetBrush(DimBrush);
	Img_Dim->SetColorAndOpacity(FLinearColor::White);
	Img_Dim->SetVisibility(ESlateVisibility::Visible);
	if (UOverlaySlot* DimSlot = Overlay_Root->AddChildToOverlay(Img_Dim))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* ColumnSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Column"));
	ColumnSize->SetWidthOverride(520.f);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Summary"));
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ColumnSize->SetContent(Column);
	if (UOverlaySlot* ColumnSlot = Overlay_Root->AddChildToOverlay(ColumnSize))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Center);
		ColumnSlot->SetVerticalAlignment(VAlign_Center);
		ColumnSlot->SetPadding(FMargin(8.f));
	}

	Txt_Outcome = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Outcome"));
	StyleLine(Txt_Outcome, OutcomeFont, WinColor, ETextJustify::Center);

	UHorizontalBox* Sub = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Sub"));
	Sub->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Txt_Time = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Time"));
	Txt_Kills = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Kills"));
	UTextBlock* Dot = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_SubDot"));
	StyleLine(Txt_Time, BodyFont, Ink, ETextJustify::Center);
	StyleLine(Dot, BodyFont, Ink, ETextJustify::Center);
	StyleLine(Txt_Kills, BodyFont, Ink, ETextJustify::Center);
	Dot->SetText(NSLOCTEXT("PixelRunSummary", "Dot", "·"));
	if (UHorizontalBoxSlot* TimeSlot = Sub->AddChildToHorizontalBox(Txt_Time))
	{
		TimeSlot->SetVerticalAlignment(VAlign_Center);
		TimeSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
	}
	if (UHorizontalBoxSlot* DotSlot = Sub->AddChildToHorizontalBox(Dot))
	{
		DotSlot->SetVerticalAlignment(VAlign_Center);
		DotSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
	}
	if (UHorizontalBoxSlot* KillsSlot = Sub->AddChildToHorizontalBox(Txt_Kills))
	{
		KillsSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* BuildFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border_Build"));
	FSlateBrush FrameBrush;
	FrameBrush.DrawAs = ESlateBrushDrawType::Box;
	FrameBrush.TintColor = FSlateColor(FLinearColor(FColor(0x24, 0x24, 0x36)));
	BuildFrame->SetBrush(FrameBrush);
	BuildFrame->SetPadding(FMargin(8.f, 6.f));
	BuildFrame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UVerticalBox* BuildColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Build"));
	BuildColumn->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UTextBlock* BuildLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Build"));
	StyleLine(BuildLabel, SmallFont, TipColor, ETextJustify::Left);
	BuildLabel->SetText(BuildCaption);
	WB_Build = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("WB_Build"));
	WB_Build->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WB_Build->SetInnerSlotPadding(FVector2D(6.f, 4.f));
	if (UVerticalBoxSlot* CaptionSlot = BuildColumn->AddChildToVerticalBox(BuildLabel))
	{
		CaptionSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}
	BuildColumn->AddChildToVerticalBox(WB_Build);
	BuildFrame->AddChild(BuildColumn);

	UHorizontalBox* Stats = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Stats"));
	Stats->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Txt_Floor = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Floor"));
	Txt_BossKills = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_BossKills"));
	StyleLine(Txt_Floor, SmallFont, Ink, ETextJustify::Center);
	StyleLine(Txt_BossKills, SmallFont, Ink, ETextJustify::Center);
	if (UHorizontalBoxSlot* FloorSlot = Stats->AddChildToHorizontalBox(Txt_Floor))
	{
		FloorSlot->SetPadding(FMargin(0.f, 0.f, 28.f, 0.f));
		FloorSlot->SetVerticalAlignment(VAlign_Center);
	}
	if (UHorizontalBoxSlot* BossSlot = Stats->AddChildToHorizontalBox(Txt_BossKills))
	{
		BossSlot->SetVerticalAlignment(VAlign_Center);
	}

	Txt_ScrapLeft = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_ScrapLeft"));
	StyleLine(Txt_ScrapLeft, SmallFont, ScrapColor, ETextJustify::Center);
	Txt_CoreTip = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_CoreTip"));
	StyleLine(Txt_CoreTip, TipFont, TipColor, ETextJustify::Center);
	Txt_CoreTip->SetAutoWrapText(true);
	Txt_CoreGain = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_CoreGain"));
	StyleLine(Txt_CoreGain, CoreFont, CoreColor, ETextJustify::Center);

	USizeBox* ContinueSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Btn_Continue"));
	ContinueSize->SetWidthOverride(128.f);
	ContinueSize->SetHeightOverride(32.f);
	Btn_Continue = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_Continue"));
	StyleSummaryButton(Btn_Continue);
	Txt_Continue = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Continue"));
	StyleLine(Txt_Continue, ButtonFont, Ink, ETextJustify::Center);
	Txt_Continue->SetText(ContinueLabel);
	Btn_Continue->AddChild(Txt_Continue);
	ContinueSize->SetContent(Btn_Continue);

	Txt_Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Hint"));
	StyleLine(Txt_Hint, HintFont, Ink, ETextJustify::Center);

	auto AddRow = [Column](UWidget* Widget, float TopPad, float BottomPad, EHorizontalAlignment Align)
	{
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Widget))
		{
			Slot->SetHorizontalAlignment(Align);
			Slot->SetPadding(FMargin(0.f, TopPad, 0.f, BottomPad));
		}
	};
	AddRow(Txt_Outcome, 0.f, 4.f, HAlign_Center);
	AddRow(Sub, 0.f, 12.f, HAlign_Center);
	AddRow(BuildFrame, 0.f, 10.f, HAlign_Fill);
	AddRow(Stats, 0.f, 4.f, HAlign_Center);
	AddRow(Txt_ScrapLeft, 0.f, 2.f, HAlign_Center);
	AddRow(Txt_CoreTip, 0.f, 6.f, HAlign_Fill);
	AddRow(Txt_CoreGain, 4.f, 14.f, HAlign_Center);
	AddRow(ContinueSize, 0.f, 6.f, HAlign_Center);
	AddRow(Txt_Hint, 0.f, 0.f, HAlign_Center);

#if !UE_BUILD_SHIPPING
	UHorizontalBox* DemoRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Demo"));
	DemoRow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	auto MakeDemo = [this, ButtonFont](const TCHAR* ButtonName, const TCHAR* LabelName, const FText& Label, UButton*& OutButton)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("Size_%s"), ButtonName));
		Size->SetWidthOverride(96.f);
		Size->SetHeightOverride(24.f);
		OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		StyleSummaryButton(OutButton);
		UTextBlock* Caption = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		StyleLine(Caption, ButtonFont, Ink, ETextJustify::Center);
		Caption->SetText(Label);
		OutButton->AddChild(Caption);
		Size->SetContent(OutButton);
		return Size;
	};
	USizeBox* WinSize = MakeDemo(TEXT("Btn_DemoWin"), TEXT("Txt_DemoWin"), DemoWinLabel, Btn_DemoWin);
	USizeBox* FailSize = MakeDemo(TEXT("Btn_DemoFail"), TEXT("Txt_DemoFail"), DemoFailLabel, Btn_DemoFail);
	if (UHorizontalBoxSlot* WinSlot = DemoRow->AddChildToHorizontalBox(WinSize))
	{
		WinSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}
	DemoRow->AddChildToHorizontalBox(FailSize);
	AddRow(DemoRow, 10.f, 0.f, HAlign_Center);
#endif

	bBuiltFallbackShell = true;
	BindInteractions();
}

void UPixelRunSummaryWidget::EnsureBuildChips()
{
	if (bBuildPoolReady || !WB_Build || !WidgetTree)
	{
		return;
	}
	WB_Build->ClearChildren();

	const FSlateFontInfo ChipFont = PixelAffixMakeFont(11);
	for (int32 Index = 0; Index < MaxBuildChips; ++Index)
	{
		UBorder* Chip = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("BuildChip_%d"), Index));
		FSlateBrush ChipBrush;
		ChipBrush.DrawAs = ESlateBrushDrawType::Box;
		ChipBrush.TintColor = FSlateColor(FLinearColor(FColor(0x2C, 0x2C, 0x44)));
		Chip->SetBrush(ChipBrush);
		Chip->SetPadding(FMargin(4.f, 2.f));
		Chip->SetVisibility(ESlateVisibility::Collapsed);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		USizeBox* IconSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconSize->SetWidthOverride(12.f);
		IconSize->SetHeightOverride(12.f);
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush IconBrush;
		IconBrush.DrawAs = ESlateBrushDrawType::Box;
		IconBrush.ImageSize = FVector2D(12.f, 12.f);
		IconBrush.TintColor = FSlateColor(FLinearColor(FColor(0xF2, 0xF2, 0xF2)));
		Icon->SetBrush(IconBrush);
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconSize->SetContent(Icon);
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		StyleLine(Label, ChipFont, Ink, ETextJustify::Left);
		if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconSize))
		{
			IconSlot->SetVerticalAlignment(VAlign_Center);
			IconSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
		}
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label))
		{
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
		Chip->AddChild(Row);
		if (UWrapBoxSlot* WrapSlot = WB_Build->AddChildToWrapBox(Chip))
		{
			WrapSlot->SetVerticalAlignment(VAlign_Center);
		}
		BuildChipRoots.Add(Chip);
		BuildChipLabels.Add(Label);
	}

	Txt_BuildMore = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_BuildMore"));
	StyleLine(Txt_BuildMore, ChipFont, Ink, ETextJustify::Left);
	Txt_BuildMore->SetText(MoreBuild);
	Txt_BuildMore->SetVisibility(ESlateVisibility::Collapsed);
	WB_Build->AddChildToWrapBox(Txt_BuildMore);

	Txt_BuildEmpty = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_BuildEmpty"));
	StyleLine(Txt_BuildEmpty, ChipFont, TipColor, ETextJustify::Left);
	Txt_BuildEmpty->SetText(EmptyBuild);
	Txt_BuildEmpty->SetVisibility(ESlateVisibility::Collapsed);
	WB_Build->AddChildToWrapBox(Txt_BuildEmpty);

	bBuildPoolReady = true;
}

void UPixelRunSummaryWidget::BindInteractions()
{
	if (Btn_Continue)
	{
		Btn_Continue->OnClicked.RemoveDynamic(this, &UPixelRunSummaryWidget::HandleContinueClicked);
		Btn_Continue->OnClicked.AddDynamic(this, &UPixelRunSummaryWidget::HandleContinueClicked);
	}
	if (Btn_DemoWin)
	{
		Btn_DemoWin->OnClicked.RemoveDynamic(this, &UPixelRunSummaryWidget::HandleDemoWinClicked);
		Btn_DemoWin->OnClicked.AddDynamic(this, &UPixelRunSummaryWidget::HandleDemoWinClicked);
#if UE_BUILD_SHIPPING
		Btn_DemoWin->SetVisibility(ESlateVisibility::Collapsed);
#endif
	}
	if (Btn_DemoFail)
	{
		Btn_DemoFail->OnClicked.RemoveDynamic(this, &UPixelRunSummaryWidget::HandleDemoFailClicked);
		Btn_DemoFail->OnClicked.AddDynamic(this, &UPixelRunSummaryWidget::HandleDemoFailClicked);
#if UE_BUILD_SHIPPING
		Btn_DemoFail->SetVisibility(ESlateVisibility::Collapsed);
#endif
	}
}

void UPixelRunSummaryWidget::Refresh()
{
	const bool bWin = View.bVictory;
	if (Txt_Outcome)
	{
		Txt_Outcome->SetText(bWin ? OutcomeWin : OutcomeFail);
		Txt_Outcome->SetColorAndOpacity(FSlateColor(bWin ? WinColor : FailColor));
	}
	if (Txt_Time)
	{
		Txt_Time->SetText(FormatClock(View.DurationSec));
	}
	if (Txt_Kills)
	{
		Txt_Kills->SetText(FText::Format(
			NSLOCTEXT("PixelRunSummary", "Kills", "击杀 {0}"),
			FText::AsNumber(View.KillCount)));
	}
	if (Txt_Floor)
	{
		Txt_Floor->SetText(FText::Format(
			NSLOCTEXT("PixelRunSummary", "Floor", "层进度 {0}/{1}"),
			FText::AsNumber(View.FloorReached),
			FText::AsNumber(View.FloorMax)));
	}
	if (Txt_BossKills)
	{
		Txt_BossKills->SetText(FText::Format(
			NSLOCTEXT("PixelRunSummary", "Boss", "Boss {0}"),
			FText::AsNumber(View.BossKills)));
	}
	if (Txt_ScrapLeft)
	{
		Txt_ScrapLeft->SetText(FText::Format(
			NSLOCTEXT("PixelRunSummary", "ScrapLeft", "Scrap 结余 {0}"),
			FText::AsNumber(View.ScrapRemain)));
		Txt_ScrapLeft->SetColorAndOpacity(FSlateColor(ScrapColor));
	}
	if (Txt_CoreGain)
	{
		Txt_CoreGain->SetText(FText::Format(
			NSLOCTEXT("PixelRunSummary", "CoreGain", "获得 Core {0}"),
			FormatSigned(View.CoreGained)));
		Txt_CoreGain->SetColorAndOpacity(FSlateColor(CoreColor));
	}
	if (Txt_CoreTip)
	{
		Txt_CoreTip->SetText(ClearTip);
		Txt_CoreTip->SetVisibility(bWin ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (Txt_Continue)
	{
		Txt_Continue->SetText(ContinueLabel);
	}
	if (Txt_Hint)
	{
		Txt_Hint->SetText(HintLabel);
	}
	if (Btn_Continue)
	{
		Btn_Continue->SetIsEnabled(IsReady() || IsDesignTime());
	}
	RefreshBuild();
}

void UPixelRunSummaryWidget::RefreshBuild()
{
	EnsureBuildChips();
	TArray<FText> Labels;
	TArray<FString> Tips;
	CollectLabels(View, Labels, Tips);
	const int32 Shown = FMath::Min(Labels.Num(), BuildChipLabels.Num());
	for (int32 Index = 0; Index < BuildChipRoots.Num() && Index < BuildChipLabels.Num(); ++Index)
	{
		const bool bShow = Index < Shown;
		if (BuildChipRoots[Index])
		{
			BuildChipRoots[Index]->SetVisibility(bShow ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
			if (bShow)
			{
				BuildChipRoots[Index]->SetToolTipText(FText::FromString(Tips.IsValidIndex(Index) ? Tips[Index] : Labels[Index].ToString()));
			}
		}
		if (bShow && BuildChipLabels[Index])
		{
			BuildChipLabels[Index]->SetText(FText::FromString(ShortenName(Labels[Index].ToString())));
		}
	}
	if (Txt_BuildMore)
	{
		Txt_BuildMore->SetVisibility(Labels.Num() > MaxBuildChips ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (Txt_BuildEmpty)
	{
		Txt_BuildEmpty->SetText(EmptyBuild);
		Txt_BuildEmpty->SetVisibility(Labels.Num() == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UPixelRunSummaryWidget::BeginDemoPause()
{
	if (bOwnsPause || IsAffixChoicePending())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (World->IsPaused())
	{
		UE_LOG(LogPixelRunSummaryUI, Warning, TEXT("RunSummary opened while the world was already paused. Confirm will not unpause it."));
		return;
	}
	UGameplayStatics::SetGamePaused(World, true);
	bOwnsPause = true;
}

void UPixelRunSummaryWidget::EndDemoPause()
{
	if (!bOwnsPause)
	{
		return;
	}
	bOwnsPause = false;
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || IsAffixChoicePending())
	{
		return;
	}
	UGameplayStatics::SetGamePaused(World, false);
}

void UPixelRunSummaryWidget::AcquireInput()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		UE_LOG(LogPixelRunSummaryUI, Warning, TEXT("RunSummary has no owning player. Create it with a PlayerController."));
		return;
	}
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(TakeWidget());
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
	bCapturedInput = true;
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetKeyboardFocus(TakeWidget());
	}
}

void UPixelRunSummaryWidget::ReleaseInput()
{
	if (!bCapturedInput)
	{
		return;
	}
	bCapturedInput = false;
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	FInputModeGameOnly GameOnly;
	PC->SetInputMode(GameOnly);
	PC->SetShowMouseCursor(false);
}

bool UPixelRunSummaryWidget::IsAffixChoicePending() const
{
	if (const UAffixSubsystem* Affix = UAffixSubsystemDefault::Get(this))
	{
		return Affix->IsChoicePending();
	}
	return false;
}

#if !UE_BUILD_SHIPPING
static TWeakObjectPtr<UPixelRunSummaryWidget> GLabRunSummary;

static void PixelSideRunSummaryDemoCmd(const TArray<FString>& Args)
{
	const bool bFail = Args.Num() > 0 && Args[0].Equals(TEXT("Fail"), ESearchCase::IgnoreCase);
	int32 Opened = 0;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || !World->IsGameWorld())
			{
				continue;
			}
			APlayerController* PC = World->GetFirstPlayerController();
			if (!PC)
			{
				continue;
			}
			if (!GLabRunSummary.IsValid() || GLabRunSummary->GetWorld() != World)
			{
				GLabRunSummary = CreateWidget<UPixelRunSummaryWidget>(PC, UPixelRunSummaryWidget::StaticClass());
			}
			if (!GLabRunSummary.IsValid())
			{
				continue;
			}
			if (bFail)
			{
				GLabRunSummary->OpenFail();
			}
			else
			{
				GLabRunSummary->OpenWin();
			}
			++Opened;
			break;
		}
	}
	if (Opened == 0)
	{
		UE_LOG(LogPixelRunSummaryUI, Warning, TEXT("PixelSide.RunSummary.Demo: no player controller. Pass Win or Fail. No key is bound."));
	}
}

static FAutoConsoleCommand GPixelSideRunSummaryDemo(
	TEXT("PixelSide.RunSummary.Demo"),
	TEXT("PixelSide.RunSummary.Demo Win|Fail. Calls OpenWin or OpenFail on WBP_RunSummary. Does not bind a key. F6-F11 are unchanged. Does not write Core."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&PixelSideRunSummaryDemoCmd));
#endif
