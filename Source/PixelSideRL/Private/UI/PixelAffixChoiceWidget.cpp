#include "UI/PixelAffixChoiceWidget.h"

#include "UI/PixelAffixCardWidget.h"
#include "UI/PixelAffixFont.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SWidget.h"

namespace
{
	const FText TitleText = NSLOCTEXT("PixelAffixChoice", "Title", "选择一项强化");
	const FText HintText = NSLOCTEXT("PixelAffixChoice", "Hint", "1 / 2 / 3 选择 · Enter 确认");

	void StyleLine(UTextBlock* Text, const FSlateFontInfo& Font)
	{
		if (!Text)
		{
			return;
		}
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(FColor(0xF2, 0xF2, 0xF2))));
		Text->SetJustification(ETextJustify::Center);
		Text->SetShadowOffset(FVector2D::ZeroVector);
		Text->SetAutoWrapText(false);
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

UPixelAffixChoiceWidget::UPixelAffixChoiceWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = true;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPixelAffixChoiceWidget::PresentOfferedChoice(const TArray<FAffixChoiceEntry>& Choices)
{
	HandleAffixChoiceOffered(Choices);
}

void UPixelAffixChoiceWidget::CloseFromPresenter(bool bConfirmed)
{
	CloseInternal(bConfirmed);
}

void UPixelAffixChoiceWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
}

TSharedRef<SWidget> UPixelAffixChoiceWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelAffixChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bInConstruct = true;
	EnsureLayout();
	BindCards();
	if (IsDesignTime())
	{
		bInConstruct = false;
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return;
	}
	SetVisibility(ESlateVisibility::Collapsed);
	TryBindSubsystem();
	bInConstruct = false;
}

void UPixelAffixChoiceWidget::NativeDestruct()
{
	UnbindSubsystem();
	ReleaseInput();
	bOpen = false;
	Super::NativeDestruct();
}

FReply UPixelAffixChoiceWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bOpen)
	{
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}

	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape)
	{
		return FReply::Handled();
	}
	if (Key == EKeys::One || Key == EKeys::NumPadOne)
	{
		ConfirmIndex(0);
		return FReply::Handled();
	}
	if (Key == EKeys::Two || Key == EKeys::NumPadTwo)
	{
		ConfirmIndex(1);
		return FReply::Handled();
	}
	if (Key == EKeys::Three || Key == EKeys::NumPadThree)
	{
		ConfirmIndex(2);
		return FReply::Handled();
	}
	if (Key == EKeys::Left || Key == EKeys::A)
	{
		MoveFocus(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::Right || Key == EKeys::D)
	{
		MoveFocus(1);
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		ConfirmIndex(FocusedIndex);
		return FReply::Handled();
	}
	return FReply::Handled();
}

void UPixelAffixChoiceWidget::HandleAffixChoiceOffered(const TArray<FAffixChoiceEntry>& Choices)
{
	UAffixSubsystem* Subsystem = FindConcreteSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogPixelAffixUI, Warning, TEXT("AffixChoice ignored an offer: UAffixSubsystem is missing or still abstract."));
		return;
	}
	if (Choices.Num() != 3)
	{
		UE_LOG(LogPixelAffixUI, Warning, TEXT("AffixChoice expected 3 entries, got %d."), Choices.Num());
	}

	TArray<FAffixCardView> Views;
	Views.Reserve(3);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const UAffixDefinition* Def = Choices.IsValidIndex(Index) ? Choices[Index].Def.Get() : nullptr;
		if (Def)
		{
			Views.Add(Subsystem->GetAffixCardView(Def, 1));
		}
		else
		{
			UE_LOG(LogPixelAffixUI, Warning, TEXT("AffixChoice slot %d has no definition."), Index);
			Views.Add(FAffixCardView());
		}
	}
	OpenWithViews(Views);
}

void UPixelAffixChoiceWidget::HandleAffixChoiceClosed(bool bConfirmed)
{
	CloseInternal(bConfirmed);
}

void UPixelAffixChoiceWidget::HandleCardSelected(int32 Index)
{
	ConfirmIndex(Index);
}

void UPixelAffixChoiceWidget::EnsureLayout()
{
	const bool bHaveShell = Overlay_Root && Img_Dim && Txt_Title && Txt_Hint && HB_Cards;
	if (!bHaveShell)
	{
		if (WidgetTree && !WidgetTree->RootWidget)
		{
			BuildFallbackShell();
		}
		else if (!bWarnedMissingWidgets)
		{
			bWarnedMissingWidgets = true;
			UE_LOG(LogPixelAffixUI, Warning,
				TEXT("WBP_AffixChoice is missing one of Overlay_Root, Img_Dim, Txt_Title, Txt_Hint, HB_Cards, Card_0, Card_1, Card_2."));
		}
	}
	EnsureFallbackCards();
}

void UPixelAffixChoiceWidget::BuildFallbackShell()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo TitleFont = PixelAffixMakeFont(16);
	const FSlateFontInfo HintFont = PixelAffixMakeFont(11);

	Overlay_Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_Root"));
	Overlay_Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Overlay_Root;

	Img_Dim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Dim"));
	FSlateBrush DimBrush;
	DimBrush.DrawAs = ESlateBrushDrawType::Box;
	DimBrush.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Img_Dim->SetBrush(DimBrush);
	Img_Dim->SetColorAndOpacity(FLinearColor::White);
	if (UOverlaySlot* DimSlot = Overlay_Root->AddChildToOverlay(Img_Dim))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Choice"));
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* ColumnSlot = Overlay_Root->AddChildToOverlay(Column))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Center);
		ColumnSlot->SetVerticalAlignment(VAlign_Center);
		ColumnSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 48.f));
	}

	Txt_Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Title"));
	Txt_Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Hint"));
	StyleLine(Txt_Title, TitleFont);
	StyleLine(Txt_Hint, HintFont);
	Txt_Title->SetText(TitleText);
	Txt_Hint->SetText(HintText);

	HB_Cards = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Cards"));
	HB_Cards->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	auto AddRow = [Column](UWidget* Widget, float Top, float Bottom)
	{
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Widget))
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetPadding(FMargin(0.f, Top, 0.f, Bottom));
		}
	};
	AddRow(Txt_Title, 0.f, 12.f);
	AddRow(HB_Cards, 0.f, 14.f);
	AddRow(Txt_Hint, 0.f, 0.f);

	bBuiltFallbackShell = true;
}

void UPixelAffixChoiceWidget::EnsureFallbackCards()
{
	if (Card_0 && Card_1 && Card_2)
	{
		return;
	}
	if (!bBuiltFallbackShell || !HB_Cards)
	{
		return;
	}

	UPixelAffixCardWidget** Slots[3] = {&Card_0, &Card_1, &Card_2};
	const TCHAR* Names[3] = {TEXT("Card_0"), TEXT("Card_1"), TEXT("Card_2")};
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (*Slots[Index])
		{
			continue;
		}
		UPixelAffixCardWidget* Card = CreateWidget<UPixelAffixCardWidget>(this, UPixelAffixCardWidget::StaticClass(), Names[Index]);
		if (!Card)
		{
			UE_LOG(LogPixelAffixUI, Warning, TEXT("AffixChoice failed to create %s."), Names[Index]);
			continue;
		}
		Card->SetChoiceIndex(Index);
		*Slots[Index] = Card;
		if (UHorizontalBoxSlot* Slot = HB_Cards->AddChildToHorizontalBox(Card))
		{
			const float Right = (Index < 2) ? 16.f : 0.f;
			Slot->SetPadding(FMargin(0.f, 0.f, Right, 0.f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}
	BindCards();
}

void UPixelAffixChoiceWidget::BindCards()
{
	UPixelAffixCardWidget* Cards[3] = {Card_0, Card_1, Card_2};
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UPixelAffixCardWidget* Card = Cards[Index];
		if (!Card)
		{
			continue;
		}
		Card->SetChoiceIndex(Index);
		Card->OnCardSelected.RemoveDynamic(this, &UPixelAffixChoiceWidget::HandleCardSelected);
		Card->OnCardSelected.AddDynamic(this, &UPixelAffixChoiceWidget::HandleCardSelected);
	}
}

void UPixelAffixChoiceWidget::TryBindSubsystem()
{
	UnbindSubsystem();
	UAffixSubsystem* Subsystem = FindConcreteSubsystem();
	if (!Subsystem)
	{
		if (!bLoggedMissingSubsystem)
		{
			bLoggedMissingSubsystem = true;
			UE_LOG(LogPixelAffixUI, Log, TEXT("AffixChoice: no concrete UAffixSubsystem yet. Cards wait for OnAffixChoiceOffered."));
		}
		return;
	}
	Subsystem->OnAffixChoiceOffered.AddDynamic(this, &UPixelAffixChoiceWidget::HandleAffixChoiceOffered);
	Subsystem->OnAffixChoiceClosed.AddDynamic(this, &UPixelAffixChoiceWidget::HandleAffixChoiceClosed);
	BoundSubsystem = Subsystem;
}

void UPixelAffixChoiceWidget::UnbindSubsystem()
{
	if (UAffixSubsystem* Subsystem = BoundSubsystem.Get())
	{
		Subsystem->OnAffixChoiceOffered.RemoveDynamic(this, &UPixelAffixChoiceWidget::HandleAffixChoiceOffered);
		Subsystem->OnAffixChoiceClosed.RemoveDynamic(this, &UPixelAffixChoiceWidget::HandleAffixChoiceClosed);
	}
	BoundSubsystem = nullptr;
}

UAffixSubsystem* UPixelAffixChoiceWidget::FindConcreteSubsystem() const
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

void UPixelAffixChoiceWidget::OpenWithViews(const TArray<FAffixCardView>& Views)
{
	EnsureLayout();
	BindCards();
	bConfirming = false;
	FocusedIndex = 0;
	ApplyViews(Views);
	RefreshFocus();
	bOpen = true;
	SetVisibility(ESlateVisibility::Visible);
	if (!bInConstruct && !IsInViewport())
	{
		AddToViewport(100);
	}
	AcquireInput();
}

void UPixelAffixChoiceWidget::ApplyViews(const TArray<FAffixCardView>& Views)
{
	if (Txt_Title)
	{
		Txt_Title->SetText(TitleText);
	}
	if (Txt_Hint)
	{
		Txt_Hint->SetText(HintText);
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UPixelAffixCardWidget* Card = CardAt(Index);
		if (!Card)
		{
			continue;
		}
		Card->SetChoiceIndex(Index);
		Card->ApplyCardView(Views.IsValidIndex(Index) ? Views[Index] : FAffixCardView());
	}
}

void UPixelAffixChoiceWidget::ConfirmIndex(int32 Index)
{
	if (!bOpen || bConfirming || Index < 0 || Index > 2)
	{
		return;
	}
	bConfirming = true;

	UAffixSubsystem* Subsystem = FindConcreteSubsystem();
	if (!Subsystem)
	{
		bConfirming = false;
		UE_LOG(LogPixelAffixUI, Warning, TEXT("AffixChoice did not confirm %d: no concrete UAffixSubsystem."), Index);
		return;
	}

	const bool bAccepted = Subsystem->ConfirmAffixChoice(Index);
	if (!bAccepted)
	{
		bConfirming = false;
		UE_LOG(LogPixelAffixUI, Warning, TEXT("AffixChoice ConfirmAffixChoice(%d) rejected."), Index);
		return;
	}
	if (bOpen)
	{
		CloseInternal(true);
	}
}

void UPixelAffixChoiceWidget::CloseInternal(bool bConfirmed)
{
	if (!bOpen && !bCapturedInput && GetVisibility() == ESlateVisibility::Collapsed)
	{
		return;
	}
	bOpen = false;
	bConfirming = false;
	FocusedIndex = 0;
	RefreshFocus();
	if (!IsDesignTime())
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
	ReleaseInput();
	UE_LOG(LogPixelAffixUI, Log, TEXT("AffixChoice closed. confirmed=%s"), bConfirmed ? TEXT("true") : TEXT("false"));
}

void UPixelAffixChoiceWidget::MoveFocus(int32 Delta)
{
	FocusedIndex = (FocusedIndex + Delta) % 3;
	if (FocusedIndex < 0)
	{
		FocusedIndex += 3;
	}
	RefreshFocus();
}

void UPixelAffixChoiceWidget::RefreshFocus()
{
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (UPixelAffixCardWidget* Card = CardAt(Index))
		{
			Card->SetFocused(Index == FocusedIndex);
		}
	}
}

void UPixelAffixChoiceWidget::AcquireInput()
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

void UPixelAffixChoiceWidget::ReleaseInput()
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

UPixelAffixCardWidget* UPixelAffixChoiceWidget::CardAt(int32 Index) const
{
	switch (Index)
	{
	case 0: return Card_0;
	case 1: return Card_1;
	case 2: return Card_2;
	default: return nullptr;
	}
}
