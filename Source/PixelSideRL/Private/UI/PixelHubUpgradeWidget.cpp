#include "UI/PixelHubUpgradeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
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
#include "Containers/Ticker.h"
#include "InputCoreTypes.h"
#include "Meta/MetaProgressSubsystem.h"
#include "Styling/SlateTypes.h"
#include "UI/PixelAffixFont.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelHubUpgrade, Log, All);

namespace
{
	const FLinearColor UpgradeInk(FColor(0xF2, 0xF2, 0xF2));
	const FLinearColor UpgradeCoreColor(FColor(0x7E, 0xC8, 0xFF));
	const FText UpgradeTitle = NSLOCTEXT("PixelHub", "UpgradeTitle", "永久成长");
	const FText CloseLabel = NSLOCTEXT("PixelHub", "Close", "关闭");
	const FText UpgradeHint = NSLOCTEXT("PixelHub", "UpgradeHint", "↑ ↓ 选择 · Enter 升级 · Esc 关闭");

	// Wireframe §2.3. "Scrap" in an effect line is the in-run grant. The price line is Core.
	struct FDemoNode
	{
		const TCHAR* Id;
		const TCHAR* Name;
		const TCHAR* Summary;
		const TCHAR* Prefix;
		const TCHAR* S0;
		const TCHAR* S1;
		const TCHAR* S2;
		int32 StepCount;
		int32 MaxRank;
		int32 C0;
		int32 C1;
		int32 C2;
	};

	const FDemoNode GDemoNodes[] = {
		{TEXT("meta_vitality"), TEXT("体质"), TEXT("开局 HP +10/20/30"), TEXT("开局 HP "), TEXT("+10"), TEXT("+20"), TEXT("+30"), 3, 3, 20, 40, 70},
		{TEXT("meta_stockpile"), TEXT("囤积"), TEXT("开局 Scrap +15/30/50"), TEXT("开局 Scrap "), TEXT("+15"), TEXT("+30"), TEXT("+50"), 3, 3, 15, 35, 60},
		{TEXT("meta_pea_familiar"), TEXT("豆射手熟悉"), TEXT("豆枪伤害 +4/8/12%"), TEXT("豆枪伤害 "), TEXT("+4%"), TEXT("+8%"), TEXT("+12%"), 3, 3, 25, 50, 90},
		{TEXT("meta_scrap_sense"), TEXT("废料感知"), TEXT("击杀 Scrap +5/10/15%"), TEXT("击杀 Scrap "), TEXT("+5%"), TEXT("+10%"), TEXT("+15%"), 3, 3, 20, 45, 80},
		{TEXT("meta_second_gun"), TEXT("第二枪缘分"), TEXT("层1 房2 后保底武器房"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), 1, 1, 40, 0, 0},
	};

	constexpr int32 DemoNodeCount = UE_ARRAY_COUNT(GDemoNodes);

	void StyleUpgradeLine(UTextBlock* Text, const FSlateFontInfo& Font, const FLinearColor& Color, ETextJustify::Type Align)
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

	void StyleUpgradeButton(UButton* Button)
	{
		if (!Button)
		{
			return;
		}
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
		const FSlateColor ButtonInk(UpgradeInk);
		Style.NormalForeground = ButtonInk;
		Style.HoveredForeground = ButtonInk;
		Style.PressedForeground = ButtonInk;
		Style.DisabledForeground = FSlateColor(FLinearColor(FColor(0x88, 0x88, 0x88)));
		Button->SetStyle(Style);
	}

	void ApplyCoreText(UTextBlock* Text, int32 Core)
	{
		if (!Text)
		{
			return;
		}
		Text->SetText(FText::FromString(FString::Printf(TEXT("Core %d"), Core)));
		Text->SetColorAndOpacity(FSlateColor(UpgradeCoreColor));
	}

	FString DemoDescription(const FDemoNode& Node, int32 Rank)
	{
		if (Node.StepCount <= 1 || Rank <= 0)
		{
			return FString(Node.Summary);
		}
		const TCHAR* Steps[] = {Node.S0, Node.S1, Node.S2};
		const int32 Current = FMath::Clamp(Rank - 1, 0, Node.StepCount - 1);
		if (Rank >= Node.MaxRank)
		{
			return FString(Node.Prefix) + Steps[Current];
		}
		const int32 Next = FMath::Clamp(Rank, 0, Node.StepCount - 1);
		return FString(Node.Prefix) + Steps[Current] + TEXT(" → ") + Steps[Next];
	}

	int32 DemoNextCost(const FDemoNode& Node, int32 Rank)
	{
		if (Rank < 0 || Rank >= Node.MaxRank)
		{
			return 0;
		}
		const int32 Costs[] = {Node.C0, Node.C1, Node.C2};
		return Costs[Rank];
	}

	int32 FindDemoIndex(FName NodeId)
	{
		const FString Id = NodeId.ToString();
		for (int32 Index = 0; Index < DemoNodeCount; ++Index)
		{
			if (Id == GDemoNodes[Index].Id)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}

UPixelHubUpgradeWidget::UPixelHubUpgradeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPixelHubUpgradeWidget::UseDemoWallet(int32 StartingCore)
{
	EnsureLayout();
	CancelPurchaseLock();
	bDemoWallet = true;
	DemoCore = FMath::Max(0, StartingCore);
	DemoRanks.Init(0, DemoNodeCount);
	FocusIndex = 0;
	RefreshRows();
	BroadcastCore();
	UE_LOG(LogPixelHubUpgrade, Log, TEXT("Hub upgrade demo wallet Core=%d. Wireframe nodes. No GetCore, no Purchase, no save."), DemoCore);
}

void UPixelHubUpgradeWidget::UseMetaWallet()
{
	EnsureLayout();
	CancelPurchaseLock();
	bDemoWallet = false;
	FocusIndex = 0;
	if (!UMetaProgressSubsystem::Get(this))
	{
		UE_LOG(LogPixelHubUpgrade, Warning, TEXT("Hub upgrade meta path has no UMetaProgressSubsystem. Rows stay empty. No demo spend."));
	}
	RefreshRows();
	BroadcastCore();
	UE_LOG(LogPixelHubUpgrade, Log, TEXT("Hub upgrade meta wallet Core=%d rows=%d. Purchase only. No save."), GetShownCore(), ShownCount);
}

void UPixelHubUpgradeWidget::OpenUpgrade()
{
	EnsureLayout();
	BindClose();
	bOpen = true;
	RefreshRows();
	SetVisibility(ESlateVisibility::Visible);
	BroadcastCore();
}

void UPixelHubUpgradeWidget::CloseUpgrade()
{
	if (!bOpen)
	{
		return;
	}
	CancelPurchaseLock();
	bOpen = false;
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.Broadcast();
}

void UPixelHubUpgradeWidget::MoveFocus(int32 Delta)
{
	if (!bOpen || bPurchasing || ShownCount <= 0)
	{
		return;
	}
	FocusIndex = FMath::Clamp(FocusIndex + Delta, 0, ShownCount - 1);
	RefreshRows();
}

void UPixelHubUpgradeWidget::BuyFocused()
{
	if (!bOpen || bPurchasing)
	{
		return;
	}
	const TArray<FPixelHubNodeRowView> Views = BuildViews();
	if (!Views.IsValidIndex(FocusIndex))
	{
		return;
	}
	TryPurchase(Views[FocusIndex].NodeId);
}

int32 UPixelHubUpgradeWidget::GetShownCore() const
{
	if (!bDemoWallet)
	{
		if (const UMetaProgressSubsystem* Meta = UMetaProgressSubsystem::Get(this))
		{
			return Meta->GetCore();
		}
		return 0;
	}
	return DemoCore;
}

void UPixelHubUpgradeWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
}

TSharedRef<SWidget> UPixelHubUpgradeWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelHubUpgradeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureLayout();
	BindClose();
}

void UPixelHubUpgradeWidget::NativeDestruct()
{
	CancelPurchaseLock();
	Super::NativeDestruct();
}

FReply UPixelHubUpgradeWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bOpen)
	{
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape)
	{
		CloseUpgrade();
		return FReply::Handled();
	}
	if (bPurchasing)
	{
		return FReply::Handled();
	}
	if (Key == EKeys::Up || Key == EKeys::W)
	{
		MoveFocus(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::Down || Key == EKeys::S)
	{
		MoveFocus(1);
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		BuyFocused();
		return FReply::Handled();
	}
	return FReply::Handled();
}

void UPixelHubUpgradeWidget::EnsureLayout()
{
	const bool bHave = Overlay_Root && Txt_Title && Txt_Core && Btn_Close && VBox_Nodes && Txt_Hint;
	if (!bHave)
	{
		if (WidgetTree && !WidgetTree->RootWidget)
		{
			BuildFallbackShell();
		}
		else if (!bWarnedMissingWidgets)
		{
			bWarnedMissingWidgets = true;
			UE_LOG(LogPixelHubUpgrade, Warning,
				TEXT("WBP_Hub_Upgrade is missing one of Overlay_Root, Txt_Title, Txt_Core, Btn_Close, VBox_Nodes, Txt_Hint."));
		}
	}
	BindClose();
}

void UPixelHubUpgradeWidget::BuildFallbackShell()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo TitleFont = PixelAffixMakeFont(16);
	const FSlateFontInfo BodyFont = PixelAffixMakeFont(13);
	const FSlateFontInfo HintFont = PixelAffixMakeFont(11);

	Overlay_Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_Root"));
	Overlay_Root->SetVisibility(ESlateVisibility::Visible);
	WidgetTree->RootWidget = Overlay_Root;

	Img_Dim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Dim"));
	FSlateBrush DimBrush;
	DimBrush.DrawAs = ESlateBrushDrawType::Box;
	DimBrush.TintColor = FSlateColor(FLinearColor(FColor(0x12, 0x12, 0x1C)));
	Img_Dim->SetBrush(DimBrush);
	if (UOverlaySlot* DimSlot = Overlay_Root->AddChildToOverlay(Img_Dim))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Upgrade"));
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* ColumnSlot = Overlay_Root->AddChildToOverlay(Column))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
		ColumnSlot->SetVerticalAlignment(VAlign_Fill);
		ColumnSlot->SetPadding(FMargin(8.f));
	}

	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_UpgradeHeader"));
	Header->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Txt_Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Title"));
	StyleUpgradeLine(Txt_Title, TitleFont, UpgradeInk, ETextJustify::Left);
	Txt_Title->SetText(UpgradeTitle);
	Txt_Core = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Core"));
	StyleUpgradeLine(Txt_Core, BodyFont, UpgradeCoreColor, ETextJustify::Right);
	ApplyCoreText(Txt_Core, 0);
	USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Btn_Close"));
	CloseSize->SetWidthOverride(64.f);
	CloseSize->SetHeightOverride(22.f);
	Btn_Close = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_Close"));
	StyleUpgradeButton(Btn_Close);
	Txt_Close = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Close"));
	StyleUpgradeLine(Txt_Close, HintFont, UpgradeInk, ETextJustify::Center);
	Txt_Close->SetText(CloseLabel);
	Btn_Close->AddChild(Txt_Close);
	CloseSize->SetContent(Btn_Close);
	if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(Txt_Title))
	{
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}
	USpacer* Gap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Spacer_UpgradeHeader"));
	if (UHorizontalBoxSlot* GapSlot = Header->AddChildToHorizontalBox(Gap))
	{
		GapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	if (UHorizontalBoxSlot* CoreSlot = Header->AddChildToHorizontalBox(Txt_Core))
	{
		CoreSlot->SetVerticalAlignment(VAlign_Center);
		CoreSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}
	if (UHorizontalBoxSlot* CloseSlot = Header->AddChildToHorizontalBox(CloseSize))
	{
		CloseSlot->SetVerticalAlignment(VAlign_Center);
	}

	VBox_Nodes = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox_Nodes"));
	VBox_Nodes->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Txt_Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Hint"));
	StyleUpgradeLine(Txt_Hint, HintFont, UpgradeInk, ETextJustify::Left);
	Txt_Hint->SetText(UpgradeHint);

	if (UVerticalBoxSlot* HeaderSlot = Column->AddChildToVerticalBox(Header))
	{
		HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
	if (UVerticalBoxSlot* ListSlot = Column->AddChildToVerticalBox(VBox_Nodes))
	{
		ListSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ListSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}
	Column->AddChildToVerticalBox(Txt_Hint);

	bBuiltFallbackShell = true;
	BindClose();
}

void UPixelHubUpgradeWidget::BindClose()
{
	if (bCloseBound || !Btn_Close)
	{
		return;
	}
	Btn_Close->OnClicked.AddDynamic(this, &UPixelHubUpgradeWidget::HandleCloseClicked);
	bCloseBound = true;
}

void UPixelHubUpgradeWidget::EnsureRowCount(int32 Count)
{
	if (!VBox_Nodes)
	{
		return;
	}
	while (Rows.Num() < Count)
	{
		const int32 Index = Rows.Num();
		UPixelHubNodeRowWidget* Row = CreateWidget<UPixelHubNodeRowWidget>(
			this,
			UPixelHubNodeRowWidget::StaticClass(),
			*FString::Printf(TEXT("Row_%d"), Index));
		if (!Row)
		{
			UE_LOG(LogPixelHubUpgrade, Warning, TEXT("Hub upgrade failed to create row %d."), Index);
			break;
		}
		Row->OnBuyClicked.AddDynamic(this, &UPixelHubUpgradeWidget::HandleRowBuy);
		if (UVerticalBoxSlot* Slot = VBox_Nodes->AddChildToVerticalBox(Row))
		{
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}
		Rows.Add(Row);
	}
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		if (Rows[Index])
		{
			Rows[Index]->SetVisibility(Index < Count ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}

void UPixelHubUpgradeWidget::RefreshRows()
{
	EnsureLayout();
	const TArray<FPixelHubNodeRowView> Views = BuildViews();
	ShownCount = Views.Num();
	if (ShownCount <= 0)
	{
		FocusIndex = 0;
	}
	else
	{
		FocusIndex = FMath::Clamp(FocusIndex, 0, ShownCount - 1);
	}
	EnsureRowCount(ShownCount);
	for (int32 Index = 0; Index < ShownCount; ++Index)
	{
		if (Rows.IsValidIndex(Index) && Rows[Index])
		{
			Rows[Index]->ApplyRow(Views[Index], Index == FocusIndex, bPurchasing);
		}
	}
	if (Txt_Title)
	{
		Txt_Title->SetText(UpgradeTitle);
	}
	if (Txt_Close)
	{
		Txt_Close->SetText(CloseLabel);
	}
	if (Txt_Hint)
	{
		Txt_Hint->SetText(UpgradeHint);
	}
	ApplyCoreText(Txt_Core, GetShownCore());
	if (Btn_Close)
	{
		Btn_Close->SetIsEnabled(!bPurchasing);
	}
}

TArray<FPixelHubNodeRowView> UPixelHubUpgradeWidget::BuildViews() const
{
	TArray<FPixelHubNodeRowView> Out;
	if (!bDemoWallet)
	{
		const UMetaProgressSubsystem* Meta = UMetaProgressSubsystem::Get(this);
		if (!Meta)
		{
			return Out;
		}
		for (const FMetaNodeView& Src : Meta->GetNodeViews())
		{
			FPixelHubNodeRowView Row;
			Row.NodeId = Src.NodeId;
			Row.DisplayName = Src.DisplayName;
			Row.Description = Src.Description;
			Row.OwnedRank = Src.OwnedRank;
			Row.MaxPurchases = Src.MaxPurchases;
			Row.NextCostCore = Src.NextCostCore;
			Row.bMaxed = Src.bMaxed;
			Row.bLocked = Src.bLocked;
			Row.bCanPurchase = Src.bCanPurchase;
			Out.Add(Row);
		}
		return Out;
	}

	Out.Reserve(DemoNodeCount);
	for (int32 Index = 0; Index < DemoNodeCount; ++Index)
	{
		const FDemoNode& Node = GDemoNodes[Index];
		const int32 Rank = DemoRanks.IsValidIndex(Index) ? DemoRanks[Index] : 0;
		const bool bMaxed = Rank >= Node.MaxRank;
		const int32 Cost = bMaxed ? 0 : DemoNextCost(Node, Rank);
		FPixelHubNodeRowView Row;
		Row.NodeId = FName(Node.Id);
		Row.DisplayName = FText::FromString(Node.Name);
		Row.Description = FText::FromString(DemoDescription(Node, Rank));
		Row.OwnedRank = Rank;
		Row.MaxPurchases = Node.MaxRank;
		Row.NextCostCore = Cost;
		Row.bMaxed = bMaxed;
		Row.bLocked = false;
		Row.bCanPurchase = !bMaxed && Cost > 0 && DemoCore >= Cost;
		Out.Add(Row);
	}
	return Out;
}

void UPixelHubUpgradeWidget::TryPurchase(FName NodeId)
{
	if (!bOpen || bPurchasing || NodeId.IsNone())
	{
		return;
	}
	const TArray<FPixelHubNodeRowView> Views = BuildViews();
	const FPixelHubNodeRowView* Row = Views.FindByPredicate([NodeId](const FPixelHubNodeRowView& Item)
	{
		return Item.NodeId == NodeId;
	});
	if (!Row || !Row->bCanPurchase)
	{
		return;
	}
	PendingNodeId = NodeId;
	bPurchasing = true;
	RefreshRows();
	if (PurchaseTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PurchaseTicker);
		PurchaseTicker.Reset();
	}
	PurchaseTicker = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPixelHubUpgradeWidget::FinishPurchase), 0.1f);
}

void UPixelHubUpgradeWidget::CommitPurchase()
{
	const FName NodeId = PendingNodeId;
	PendingNodeId = NAME_None;
	if (NodeId.IsNone())
	{
		return;
	}
	if (bDemoWallet)
	{
		const int32 Index = FindDemoIndex(NodeId);
		if (!DemoRanks.IsValidIndex(Index))
		{
			return;
		}
		const FDemoNode& Node = GDemoNodes[Index];
		const int32 Rank = DemoRanks[Index];
		const int32 Cost = DemoNextCost(Node, Rank);
		if (Rank >= Node.MaxRank || Cost <= 0 || DemoCore < Cost)
		{
			return;
		}
		DemoCore -= Cost;
		DemoRanks[Index] = Rank + 1;
		UE_LOG(LogPixelHubUpgrade, Log, TEXT("Hub demo purchase %s -> Lv %d Core=%d. Local only. No save."),
			*NodeId.ToString(), DemoRanks[Index], DemoCore);
		return;
	}

	UMetaProgressSubsystem* Meta = UMetaProgressSubsystem::Get(this);
	if (!Meta)
	{
		UE_LOG(LogPixelHubUpgrade, Warning, TEXT("Hub Purchase skipped: no UMetaProgressSubsystem."));
		return;
	}
	if (!Meta->CanPurchase(NodeId) || !Meta->Purchase(NodeId))
	{
		UE_LOG(LogPixelHubUpgrade, Warning, TEXT("Hub Purchase %s failed. Core=%d. No save."), *NodeId.ToString(), Meta->GetCore());
		return;
	}
	UE_LOG(LogPixelHubUpgrade, Log, TEXT("Hub Purchase %s -> rank %d Core=%d. Save was not called."),
		*NodeId.ToString(), Meta->GetPurchasedRank(NodeId), Meta->GetCore());
}

void UPixelHubUpgradeWidget::CancelPurchaseLock()
{
	bPurchasing = false;
	PendingNodeId = NAME_None;
	if (PurchaseTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PurchaseTicker);
		PurchaseTicker.Reset();
	}
}

void UPixelHubUpgradeWidget::BroadcastCore()
{
	OnWalletChanged.Broadcast(GetShownCore());
}

bool UPixelHubUpgradeWidget::FinishPurchase(float DeltaSeconds)
{
	(void)DeltaSeconds;
	PurchaseTicker.Reset();
	bPurchasing = false;
	if (bOpen)
	{
		CommitPurchase();
		RefreshRows();
		BroadcastCore();
	}
	else
	{
		PendingNodeId = NAME_None;
	}
	return false;
}

void UPixelHubUpgradeWidget::HandleCloseClicked()
{
	CloseUpgrade();
}

void UPixelHubUpgradeWidget::HandleRowBuy(FName NodeId)
{
	const TArray<FPixelHubNodeRowView> Views = BuildViews();
	for (int32 Index = 0; Index < Views.Num(); ++Index)
	{
		if (Views[Index].NodeId == NodeId)
		{
			FocusIndex = Index;
			break;
		}
	}
	TryPurchase(NodeId);
}
