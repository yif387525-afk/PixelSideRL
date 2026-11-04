#include "UI/PixelHubWidget.h"

#include "Affix/AffixSubsystemDefault.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Meta/MetaProgressSubsystem.h"
#include "Styling/SlateTypes.h"
#include "UI/PixelAffixFont.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelHubUI, Log, All);

namespace
{
	constexpr int32 HubViewportZ = 85;
	const FLinearColor Ink(FColor(0xF2, 0xF2, 0xF2));
	const FLinearColor CoreColor(FColor(0x7E, 0xC8, 0xFF));
	const FLinearColor MarkColor(FColor(0xC8, 0xC8, 0xD8));
	const FText HubTitle = NSLOCTEXT("PixelHub", "Title", "据点");
	const FText StartLabel = NSLOCTEXT("PixelHub", "Start", "开始新一局");
	const FText UpgradesLabel = NSLOCTEXT("PixelHub", "Upgrades", "永久成长");
	const FText SummaryLabel = NSLOCTEXT("PixelHub", "LastSummary", "上次结算");
	const FText MenuLabel = NSLOCTEXT("PixelHub", "Menu", "返回标题");
	const FText MarkLabel = NSLOCTEXT("PixelHub", "Mark", "PixelSideRL");

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
	}

	void StyleHubButton(UButton* Button)
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

	USizeBox* MakeButton(UWidgetTree* Tree, const TCHAR* SizeName, const TCHAR* ButtonName, const TCHAR* TextName, const FText& Label, const FSlateFontInfo& Font, UButton*& OutButton, UTextBlock*& OutText)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), SizeName);
		Size->SetWidthOverride(168.f);
		Size->SetHeightOverride(28.f);
		OutButton = Tree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		StyleHubButton(OutButton);
		OutText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
		StyleLine(OutText, Font, Ink, ETextJustify::Center);
		OutText->SetText(Label);
		OutButton->AddChild(OutText);
		Size->SetContent(OutButton);
		return Size;
	}
}

UPixelHubWidget::UPixelHubWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPixelHubWidget::OpenHub(bool bPause)
{
	OpenInternal(false, bPause);
}

void UPixelHubWidget::OpenHubDemo(bool bPause)
{
	OpenInternal(true, bPause);
}

void UPixelHubWidget::CloseHub()
{
	if (State == EPixelHubState::Closed)
	{
		return;
	}
	CancelStartLock();
	State = EPixelHubState::Closed;
	if (Panel_Upgrade && Panel_Upgrade->IsUpgradeOpen())
	{
		Panel_Upgrade->CloseUpgrade();
	}
	ReleaseInput();
	EndPause();
	SetVisibility(ESlateVisibility::Collapsed);
}

UPixelHubWidget* UPixelHubWidget::OpenForPlayer(
	APlayerController* OwningPlayer,
	bool bDemo,
	TSubclassOf<UPixelHubWidget> WidgetClass)
{
	if (!OwningPlayer || !OwningPlayer->IsLocalPlayerController())
	{
		return nullptr;
	}
	if (!WidgetClass)
	{
		WidgetClass = StaticClass();
	}
	UPixelHubWidget* Widget = CreateWidget<UPixelHubWidget>(OwningPlayer, WidgetClass);
	if (!Widget)
	{
		return nullptr;
	}
	if (bDemo)
	{
		Widget->OpenHubDemo();
	}
	else
	{
		Widget->OpenHub();
	}
	return Widget->IsHubOpen() ? Widget : nullptr;
}

void UPixelHubWidget::SetLastRunSummary(const FRunSummaryView& InView)
{
	LastSummary = InView;
	bHasLastSummary = true;
	RefreshChrome();
}

void UPixelHubWidget::ClearLastRunSummary()
{
	bHasLastSummary = false;
	RefreshChrome();
}

bool UPixelHubWidget::IsHubOpen() const
{
	return State != EPixelHubState::Closed;
}

bool UPixelHubWidget::IsUpgradeOpen() const
{
	return Panel_Upgrade && Panel_Upgrade->IsUpgradeOpen();
}

void UPixelHubWidget::OpenInternal(bool bForceDemo, bool bPause)
{
	if (IsDesignTime())
	{
		return;
	}
	if (IsAffixChoicePending())
	{
		UE_LOG(LogPixelHubUI, Warning, TEXT("Hub did not open: an affix choice is pending."));
		return;
	}
	if (!GetOwningPlayer())
	{
		UE_LOG(LogPixelHubUI, Warning, TEXT("Hub has no owning player. Create it with a PlayerController."));
		return;
	}

	EnsureLayout();
	if (!Panel_Upgrade)
	{
		UE_LOG(LogPixelHubUI, Warning, TEXT("Hub has no upgrade panel."));
		return;
	}

	CancelStartLock();
	if (Panel_Upgrade->IsUpgradeOpen())
	{
		Panel_Upgrade->CloseUpgrade();
	}

	UMetaProgressSubsystem* Meta = nullptr;
	if (!bForceDemo)
	{
		Meta = UMetaProgressSubsystem::Get(this);
	}
	bDemoWallet = bForceDemo || Meta == nullptr;
	if (bDemoWallet)
	{
		Panel_Upgrade->UseDemoWallet(120);
		if (bForceDemo)
		{
			UE_LOG(LogPixelHubUI, Log, TEXT("Hub demo. Local Core 120. Does not call GetCore, Purchase, AddCore, or Save."));
		}
		else
		{
			UE_LOG(LogPixelHubUI, Log, TEXT("Hub has no UMetaProgressSubsystem. Stub Core 120. No save file."));
		}
	}
	else
	{
		Panel_Upgrade->UseMetaWallet();
	}

	ShownCore = Panel_Upgrade->GetShownCore();
	if (bDemoWallet)
	{
		State = EPixelHubState::NoSave;
	}
	else
	{
		State = IsLiveEmpty(Meta) ? EPixelHubState::NoSave : EPixelHubState::Idle;
	}
	RefreshChrome();
	SetVisibility(ESlateVisibility::Visible);
	if (!IsInViewport())
	{
		AddToViewport(HubViewportZ);
	}
	AcquireInput();
	if (bPause)
	{
		BeginPause();
	}
	UE_LOG(LogPixelHubUI, Log, TEXT("Hub open wallet=%s core=%d state=%s ownsPause=%s. F6-F11 are not bound."),
		bDemoWallet ? TEXT("demo") : TEXT("meta"),
		ShownCore,
		State == EPixelHubState::NoSave ? TEXT("NoSave") : TEXT("Idle"),
		bOwnsPause ? TEXT("true") : TEXT("false"));
}

void UPixelHubWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
	if (IsDesignTime())
	{
		ShowDesign();
	}
}

TSharedRef<SWidget> UPixelHubWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelHubWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureLayout();
	BindButtons();
	BindUpgrade();
}

void UPixelHubWidget::NativeDestruct()
{
	CancelStartLock();
	if (SummaryWidget)
	{
		SummaryWidget->OnContinueToHub.RemoveDynamic(this, &UPixelHubWidget::HandleSummaryContinue);
	}
	ReleaseInput();
	EndPause();
	Super::NativeDestruct();
}

FReply UPixelHubWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!IsHubOpen())
	{
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}
	const FKey Key = InKeyEvent.GetKey();
	if (Panel_Upgrade && Panel_Upgrade->IsUpgradeOpen())
	{
		if (Key == EKeys::Escape)
		{
			Panel_Upgrade->CloseUpgrade();
			return FReply::Handled();
		}
		if (Panel_Upgrade->IsPurchasing())
		{
			return FReply::Handled();
		}
		if (Key == EKeys::Up || Key == EKeys::W)
		{
			Panel_Upgrade->MoveFocus(-1);
			return FReply::Handled();
		}
		if (Key == EKeys::Down || Key == EKeys::S)
		{
			Panel_Upgrade->MoveFocus(1);
			return FReply::Handled();
		}
		if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
		{
			Panel_Upgrade->BuyFocused();
			return FReply::Handled();
		}
	}
	return FReply::Handled();
}

void UPixelHubWidget::EnsureLayout()
{
	const bool bHave = Overlay_Root && Img_Dim && Txt_Title && Txt_Core && Btn_StartRun && Btn_Upgrades && Btn_MainMenu;
	if (!bHave)
	{
		if (WidgetTree && !WidgetTree->RootWidget)
		{
			BuildFallbackShell();
		}
		else if (!bWarnedMissingWidgets)
		{
			bWarnedMissingWidgets = true;
			UE_LOG(LogPixelHubUI, Warning,
				TEXT("WBP_Hub is missing one of Overlay_Root, Img_Dim, Txt_Title, Txt_Core, Btn_StartRun, Btn_Upgrades, Btn_MainMenu."));
		}
	}
	BindButtons();
	BindUpgrade();
}

void UPixelHubWidget::BuildFallbackShell()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo TitleFont = PixelAffixMakeFont(18);
	const FSlateFontInfo BodyFont = PixelAffixMakeFont(13);
	const FSlateFontInfo MarkFont = PixelAffixMakeFont(14);

	Overlay_Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_Root"));
	Overlay_Root->SetVisibility(ESlateVisibility::Visible);
	WidgetTree->RootWidget = Overlay_Root;

	Img_Dim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Dim"));
	FSlateBrush DimBrush;
	DimBrush.DrawAs = ESlateBrushDrawType::Box;
	DimBrush.TintColor = FSlateColor(FLinearColor(FColor(0x14, 0x14, 0x1C)));
	Img_Dim->SetBrush(DimBrush);
	if (UOverlaySlot* DimSlot = Overlay_Root->AddChildToOverlay(Img_Dim))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Hub"));
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* ColumnSlot = Overlay_Root->AddChildToOverlay(Column))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
		ColumnSlot->SetVerticalAlignment(VAlign_Fill);
		ColumnSlot->SetPadding(FMargin(8.f));
	}

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Header"));
	Header->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Txt_Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Title"));
	StyleLine(Txt_Title, TitleFont, Ink, ETextJustify::Left);
	Txt_Title->SetText(HubTitle);
	Txt_Core = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Core"));
	StyleLine(Txt_Core, BodyFont, CoreColor, ETextJustify::Right);
	Txt_Core->SetText(FText::FromString(TEXT("Core 0")));
	if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(Txt_Title))
	{
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}
	USpacer* HeaderGap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Spacer_Header"));
	if (UHorizontalBoxSlot* GapSlot = Header->AddChildToHorizontalBox(HeaderGap))
	{
		GapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	if (UHorizontalBoxSlot* CoreSlot = Header->AddChildToHorizontalBox(Txt_Core))
	{
		CoreSlot->SetVerticalAlignment(VAlign_Center);
	}

	Txt_Mark = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Mark"));
	StyleLine(Txt_Mark, MarkFont, MarkColor, ETextJustify::Center);
	Txt_Mark->SetText(MarkLabel);

	auto AddCentered = [Column](UWidget* Widget, float Bottom)
	{
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Widget))
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, Bottom));
		}
	};

	if (UVerticalBoxSlot* HeaderSlot = Column->AddChildToVerticalBox(Header))
	{
		HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
	USpacer* TopGap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Spacer_Top"));
	if (UVerticalBoxSlot* TopSlot = Column->AddChildToVerticalBox(TopGap))
	{
		TopSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	AddCentered(Txt_Mark, 16.f);
	UButton* StartButton = nullptr;
	UTextBlock* StartText = nullptr;
	UButton* UpgradeButton = nullptr;
	UTextBlock* UpgradeText = nullptr;
	UButton* SummaryButton = nullptr;
	UTextBlock* SummaryText = nullptr;
	UButton* MenuButton = nullptr;
	UTextBlock* MenuText = nullptr;
	AddCentered(MakeButton(WidgetTree, TEXT("Size_Btn_StartRun"), TEXT("Btn_StartRun"), TEXT("Txt_StartRun"), StartLabel, BodyFont, StartButton, StartText), 6.f);
	AddCentered(MakeButton(WidgetTree, TEXT("Size_Btn_Upgrades"), TEXT("Btn_Upgrades"), TEXT("Txt_Upgrades"), UpgradesLabel, BodyFont, UpgradeButton, UpgradeText), 6.f);
	AddCentered(MakeButton(WidgetTree, TEXT("Size_Btn_LastSummary"), TEXT("Btn_LastSummary"), TEXT("Txt_LastSummary"), SummaryLabel, BodyFont, SummaryButton, SummaryText), 16.f);
	Btn_StartRun = StartButton;
	Txt_StartRun = StartText;
	Btn_Upgrades = UpgradeButton;
	Txt_Upgrades = UpgradeText;
	Btn_LastSummary = SummaryButton;
	Txt_LastSummary = SummaryText;
	if (Btn_LastSummary)
	{
		Btn_LastSummary->SetVisibility(ESlateVisibility::Collapsed);
		if (UWidget* SummarySize = Btn_LastSummary->GetParent())
		{
			SummarySize->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	AddCentered(MakeButton(WidgetTree, TEXT("Size_Btn_MainMenu"), TEXT("Btn_MainMenu"), TEXT("Txt_MainMenu"), MenuLabel, BodyFont, MenuButton, MenuText), 0.f);
	Btn_MainMenu = MenuButton;
	Txt_MainMenu = MenuText;
	USpacer* BottomGap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Spacer_Bottom"));
	if (UVerticalBoxSlot* BottomSlot = Column->AddChildToVerticalBox(BottomGap))
	{
		BottomSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	bBuiltFallbackShell = true;
	BindButtons();
}

void UPixelHubWidget::BindButtons()
{
	if (bButtonsBound || !Btn_StartRun || !Btn_Upgrades || !Btn_MainMenu)
	{
		return;
	}
	Btn_StartRun->OnClicked.AddDynamic(this, &UPixelHubWidget::HandleStartClicked);
	Btn_Upgrades->OnClicked.AddDynamic(this, &UPixelHubWidget::HandleUpgradesClicked);
	Btn_MainMenu->OnClicked.AddDynamic(this, &UPixelHubWidget::HandleMenuClicked);
	if (Btn_LastSummary)
	{
		Btn_LastSummary->OnClicked.AddDynamic(this, &UPixelHubWidget::HandleLastSummaryClicked);
	}
	bButtonsBound = true;
}

void UPixelHubWidget::BindUpgrade()
{
	if (!Panel_Upgrade && Overlay_Root)
	{
		Panel_Upgrade = CreateWidget<UPixelHubUpgradeWidget>(this, UPixelHubUpgradeWidget::StaticClass(), TEXT("Panel_Upgrade"));
		if (Panel_Upgrade)
		{
			if (UOverlaySlot* Slot = Overlay_Root->AddChildToOverlay(Panel_Upgrade))
			{
				Slot->SetHorizontalAlignment(HAlign_Fill);
				Slot->SetVerticalAlignment(VAlign_Fill);
			}
			Panel_Upgrade->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (bUpgradeBound || !Panel_Upgrade)
	{
		return;
	}
	Panel_Upgrade->OnClosed.RemoveDynamic(this, &UPixelHubWidget::HandleUpgradeClosed);
	Panel_Upgrade->OnClosed.AddDynamic(this, &UPixelHubWidget::HandleUpgradeClosed);
	Panel_Upgrade->OnWalletChanged.RemoveDynamic(this, &UPixelHubWidget::HandleWalletChanged);
	Panel_Upgrade->OnWalletChanged.AddDynamic(this, &UPixelHubWidget::HandleWalletChanged);
	bUpgradeBound = true;
}

void UPixelHubWidget::RefreshChrome()
{
	if (Txt_Title)
	{
		Txt_Title->SetText(HubTitle);
	}
	if (Txt_Mark)
	{
		Txt_Mark->SetText(MarkLabel);
	}
	if (Txt_StartRun)
	{
		Txt_StartRun->SetText(StartLabel);
	}
	if (Txt_Upgrades)
	{
		Txt_Upgrades->SetText(UpgradesLabel);
	}
	if (Txt_LastSummary)
	{
		Txt_LastSummary->SetText(SummaryLabel);
	}
	if (Txt_MainMenu)
	{
		Txt_MainMenu->SetText(MenuLabel);
	}
	ApplyCoreLabel();
	const bool bEnable = CanClickHub();
	if (Btn_StartRun)
	{
		Btn_StartRun->SetIsEnabled(bEnable);
	}
	if (Btn_Upgrades)
	{
		Btn_Upgrades->SetIsEnabled(bEnable);
	}
	if (Btn_MainMenu)
	{
		Btn_MainMenu->SetIsEnabled(bEnable);
	}
	if (Btn_LastSummary)
	{
		const ESlateVisibility SummaryVis = bHasLastSummary ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
		Btn_LastSummary->SetVisibility(SummaryVis);
		if (UWidget* SummarySize = Btn_LastSummary->GetParent())
		{
			SummarySize->SetVisibility(SummaryVis);
		}
		Btn_LastSummary->SetIsEnabled(bEnable && bHasLastSummary);
	}
}

void UPixelHubWidget::ShowDesign()
{
	if (Txt_Title)
	{
		Txt_Title->SetText(HubTitle);
	}
	if (Txt_Mark)
	{
		Txt_Mark->SetText(MarkLabel);
	}
	if (Txt_Core)
	{
		Txt_Core->SetText(FText::FromString(TEXT("Core 120")));
		Txt_Core->SetColorAndOpacity(FSlateColor(CoreColor));
	}
	if (Btn_LastSummary)
	{
		Btn_LastSummary->SetVisibility(ESlateVisibility::Collapsed);
		if (UWidget* SummarySize = Btn_LastSummary->GetParent())
		{
			SummarySize->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

bool UPixelHubWidget::CanClickHub() const
{
	return IsHubOpen() && State != EPixelHubState::Starting && !IsUpgradeOpen();
}

bool UPixelHubWidget::IsLiveEmpty(const UMetaProgressSubsystem* Meta) const
{
	if (!Meta || Meta->GetCore() != 0)
	{
		return Meta == nullptr;
	}
	for (const FMetaNodeView& View : Meta->GetNodeViews())
	{
		if (View.OwnedRank != 0)
		{
			return false;
		}
	}
	return true;
}

void UPixelHubWidget::ApplyCoreLabel()
{
	if (!Txt_Core)
	{
		return;
	}
	Txt_Core->SetText(FText::FromString(FString::Printf(TEXT("Core %d"), ShownCore)));
	Txt_Core->SetColorAndOpacity(FSlateColor(CoreColor));
}

void UPixelHubWidget::CancelStartLock()
{
	if (StartTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(StartTicker);
		StartTicker.Reset();
	}
}

bool UPixelHubWidget::FinishStart(float DeltaSeconds)
{
	(void)DeltaSeconds;
	StartTicker.Reset();
	if (State != EPixelHubState::Starting)
	{
		return false;
	}
	if (bDemoWallet)
	{
		State = EPixelHubState::NoSave;
	}
	else
	{
		State = IsLiveEmpty(UMetaProgressSubsystem::Get(this)) ? EPixelHubState::NoSave : EPixelHubState::Idle;
	}
	RefreshChrome();
	UE_LOG(LogPixelHubUI, Log, TEXT("Hub OnStartRun. BeginRunFromMeta was not called."));
	OnStartRun.Broadcast();
	return false;
}

void UPixelHubWidget::BeginPause()
{
	if (bOwnsPause || IsAffixChoicePending())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || World->IsPaused())
	{
		if (World && World->IsPaused())
		{
			UE_LOG(LogPixelHubUI, Warning, TEXT("Hub opened while the world was already paused. Close will not unpause it."));
		}
		return;
	}
	UGameplayStatics::SetGamePaused(World, true);
	bOwnsPause = true;
}

void UPixelHubWidget::EndPause()
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

void UPixelHubWidget::AcquireInput()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
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

void UPixelHubWidget::ReleaseInput()
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

bool UPixelHubWidget::IsAffixChoicePending() const
{
	if (const UAffixSubsystem* Affix = UAffixSubsystemDefault::Get(this))
	{
		return Affix->IsChoicePending();
	}
	return false;
}

void UPixelHubWidget::HandleStartClicked()
{
	if (!CanClickHub())
	{
		return;
	}
	State = EPixelHubState::Starting;
	RefreshChrome();
	CancelStartLock();
	StartTicker = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPixelHubWidget::FinishStart), 0.12f);
}

void UPixelHubWidget::HandleUpgradesClicked()
{
	if (!CanClickHub() || !Panel_Upgrade)
	{
		return;
	}
	OnOpenUpgrades.Broadcast();
	if (!bDemoWallet)
	{
		Panel_Upgrade->UseMetaWallet();
	}
	Panel_Upgrade->OpenUpgrade();
	ShownCore = Panel_Upgrade->GetShownCore();
	RefreshChrome();
}

void UPixelHubWidget::HandleLastSummaryClicked()
{
	if (!CanClickHub() || !bHasLastSummary)
	{
		return;
	}
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	if (!SummaryWidget || SummaryWidget->GetWorld() != GetWorld())
	{
		SummaryWidget = CreateWidget<UPixelRunSummaryWidget>(PC, UPixelRunSummaryWidget::StaticClass());
	}
	if (!SummaryWidget)
	{
		return;
	}
	SummaryWidget->OnContinueToHub.RemoveDynamic(this, &UPixelHubWidget::HandleSummaryContinue);
	SummaryWidget->OnContinueToHub.AddDynamic(this, &UPixelHubWidget::HandleSummaryContinue);
	SummaryWidget->Present(LastSummary, false);
}

void UPixelHubWidget::HandleMenuClicked()
{
	if (!CanClickHub())
	{
		return;
	}
	CloseHub();
	OnBackToMenu.Broadcast();
}

void UPixelHubWidget::HandleUpgradeClosed()
{
	if (Panel_Upgrade)
	{
		ShownCore = Panel_Upgrade->GetShownCore();
	}
	RefreshChrome();
	if (IsHubOpen())
	{
		AcquireInput();
	}
}

void UPixelHubWidget::HandleWalletChanged(int32 Core)
{
	ShownCore = Core;
	ApplyCoreLabel();
}

void UPixelHubWidget::HandleSummaryContinue()
{
	if (IsHubOpen())
	{
		AcquireInput();
	}
}

#if !UE_BUILD_SHIPPING
static TWeakObjectPtr<UPixelHubWidget> GLabHub;

static void PixelSideHubOpenCmd(const TArray<FString>& Args, bool bDemo)
{
	(void)Args;
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
			if (!GLabHub.IsValid() || GLabHub->GetWorld() != World)
			{
				GLabHub = CreateWidget<UPixelHubWidget>(PC, UPixelHubWidget::StaticClass());
			}
			if (!GLabHub.IsValid())
			{
				continue;
			}
			if (bDemo)
			{
				GLabHub->OpenHubDemo();
			}
			else
			{
				GLabHub->OpenHub();
			}
			if (GLabHub->IsHubOpen())
			{
				++Opened;
			}
			break;
		}
	}
	if (Opened == 0)
	{
		UE_LOG(LogPixelHubUI, Warning, TEXT("PixelSide.Hub: no player controller, or an affix choice is pending. No key is bound."));
	}
}

static void PixelSideHubDemoCmd(const TArray<FString>& Args)
{
	PixelSideHubOpenCmd(Args, true);
}

static void PixelSideHubLiveCmd(const TArray<FString>& Args)
{
	PixelSideHubOpenCmd(Args, false);
}

static FAutoConsoleCommand GPixelSideHubDemo(
	TEXT("PixelSide.Hub.Demo"),
	TEXT("PixelSide.Hub.Demo. Opens WBP_Hub on local Core 120. Does not call GetCore, Purchase, AddCore, or Save. No key. F6-F11 are unchanged."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&PixelSideHubDemoCmd));

static FAutoConsoleCommand GPixelSideHubOpen(
	TEXT("PixelSide.Hub.Open"),
	TEXT("PixelSide.Hub.Open. Opens WBP_Hub on UMetaProgressSubsystem when it exists. Purchase spends Core. Does not write a save. No key. F6-F11 are unchanged."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&PixelSideHubLiveCmd));
#endif
