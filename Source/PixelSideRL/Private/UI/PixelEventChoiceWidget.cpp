#include "UI/PixelEventChoiceWidget.h"

#include "UI/PixelAffixFont.h"
#include "Blueprint/WidgetTree.h"
#include "Combat/PixelSideCharacter.h"
#include "Event/EventSubsystem.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogPixelEventUI);

namespace
{
	constexpr int32 MaxChoices = 3;

	void StyleLine(UTextBlock* Text, const FSlateFontInfo& Font, ETextJustify::Type Align, bool bWrap)
	{
		if (!Text)
		{
			return;
		}
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(FColor(0xF2, 0xF2, 0xF2))));
		Text->SetJustification(Align);
		Text->SetShadowOffset(FVector2D::ZeroVector);
		Text->SetAutoWrapText(bWrap);
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	void PaintSkip(UButton* Button)
	{
		if (!Button)
		{
			return;
		}
		FButtonStyle Style = Button->GetStyle();
		const FLinearColor Fill(FColor(0x12, 0x12, 0x1C));
		const FLinearColor Hover(FColor(0x2A, 0x2A, 0x3A));
		auto Solid = [](FSlateBrush& Brush, const FLinearColor& Color)
		{
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.TintColor = FSlateColor(Color);
			Brush.ImageSize = FVector2D(280.f, 28.f);
		};
		Solid(Style.Normal, Fill);
		Solid(Style.Hovered, Hover);
		Solid(Style.Pressed, Hover);
		Solid(Style.Disabled, Fill);
		Style.NormalPadding = FMargin(10.f, 4.f);
		Style.PressedPadding = FMargin(10.f, 5.f);
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::White);
	}

	void ShowLabLine(const FString& Line)
	{
#if !UE_BUILD_SHIPPING
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(0xEC01, 2.5f, FColor::White, Line);
		}
#else
		(void)Line;
#endif
	}

	/** Locked copy of Content/Data/Events/DA_Event_AbandonedCache.json. Not a DataAsset. */
	FPixelEventChoiceView MakeAbandonedCacheDemoView()
	{
		FPixelEventChoiceView View;
		View.EventId = FName(TEXT("AbandonedCache"));
		View.DisplayName = FText::FromString(TEXT("废弃补给箱"));
		View.PromptBody = FText::FromString(TEXT("封条已经裂开。小心撬开能摸到 Scrap；硬拽更快，但箱缘会割手。也可以直接走开。"));
		View.SkipHint = FText::FromString(TEXT("走开（无奖励）"));
		View.bAllowDoorSkip = true;
		View.bCombatAllowed = false;
		View.bAffixOnResolve = false;

		FPixelEventChoiceOption Careful;
		Careful.ChoiceId = FName(TEXT("CarefulLoot"));
		Careful.Label = FText::FromString(TEXT("小心撬开"));
		FPixelEventChoiceOption Force;
		Force.ChoiceId = FName(TEXT("ForceLoot"));
		Force.Label = FText::FromString(TEXT("硬拽出来"));
		View.Choices.Add(Careful);
		View.Choices.Add(Force);
		return View;
	}
}

UPixelEventChoiceWidget::UPixelEventChoiceWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = true;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPixelEventChoiceWidget::Present(const FPixelEventChoiceView& View)
{
	EnsureLayout();
	ActiveView = View;
	if (ActiveView.Choices.Num() > MaxChoices)
	{
		UE_LOG(LogPixelEventUI, Warning, TEXT("EventChoice keeps the first %d choices of %d."), MaxChoices, ActiveView.Choices.Num());
		ActiveView.Choices.SetNum(MaxChoices);
	}
	if (ActiveView.bAffixOnResolve)
	{
		UE_LOG(LogPixelEventUI, Warning, TEXT("EventChoice ignored bAffixOnResolve. WBP_AffixChoice stays closed."));
	}
	if (ActiveView.bCombatAllowed)
	{
		UE_LOG(LogPixelEventUI, Warning, TEXT("EventChoice ignored bCombatAllowed. This UI does not start combat."));
	}

	bResolving = false;
	FocusedIndex = 0;
	bOpen = true;
	ApplyView();
	RefreshFocus();
	SetVisibility(ESlateVisibility::Visible);
	if (!IsDesignTime() && !bInConstruct && !IsInViewport())
	{
		AddToViewport(90);
	}
	if (!IsDesignTime())
	{
		AcquireInput();
		SetPawnControlBlocked(true);
	}

	UE_LOG(LogPixelEventUI, Log,
		TEXT("EventChoice open EventId=%s choices=%d doorSkip=%s affixOnResolve=%s. No SetGamePaused. Open does not apply effects."),
		*ActiveView.EventId.ToString(),
		ChoiceCount(),
		ActiveView.bAllowDoorSkip ? TEXT("true") : TEXT("false"),
		ActiveView.bAffixOnResolve ? TEXT("true") : TEXT("false"));
}

void UPixelEventChoiceWidget::PresentAbandonedCacheDemo()
{
	Present(MakeAbandonedCacheDemoView());
}

void UPixelEventChoiceWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
	if (IsDesignTime())
	{
		ActiveView = MakeAbandonedCacheDemoView();
		ApplyView();
	}
}

TSharedRef<SWidget> UPixelEventChoiceWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelEventChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bInConstruct = true;
	EnsureLayout();
	BindChoices();
	BindSkip();
	if (IsDesignTime())
	{
		bInConstruct = false;
		if (ActiveView.EventId.IsNone())
		{
			ActiveView = MakeAbandonedCacheDemoView();
			ApplyView();
		}
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return;
	}
	SetVisibility(ESlateVisibility::Collapsed);
	bInConstruct = false;
}

void UPixelEventChoiceWidget::NativeDestruct()
{
	if (bOpen)
	{
		CloseInternal();
	}
	else
	{
		ReleaseInput();
		SetPawnControlBlocked(false);
	}
	Super::NativeDestruct();
}

FReply UPixelEventChoiceWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bOpen)
	{
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}

	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape)
	{
		if (ActiveView.bAllowDoorSkip)
		{
			Skip();
		}
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
		if (ActiveView.bAllowDoorSkip && FocusedIndex == ChoiceCount())
		{
			Skip();
		}
		else
		{
			ConfirmIndex(FocusedIndex);
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void UPixelEventChoiceWidget::HandleChoiceButtonClicked(FName ChoiceId)
{
	const int32 Limit = ChoiceCount();
	for (int32 Index = 0; Index < Limit; ++Index)
	{
		if (ActiveView.Choices[Index].ChoiceId == ChoiceId)
		{
			ConfirmIndex(Index);
			return;
		}
	}
	UE_LOG(LogPixelEventUI, Warning, TEXT("EventChoice click ignored. Unknown ChoiceId %s."), *ChoiceId.ToString());
}

void UPixelEventChoiceWidget::HandleSkipClicked()
{
	Skip();
}

void UPixelEventChoiceWidget::EnsureLayout()
{
	const bool bHaveShell = Overlay_Root && Img_Dim && Txt_Title && Txt_Body && Btn_Skip && Txt_Skip;
	if (!bHaveShell)
	{
		if (WidgetTree && !WidgetTree->RootWidget)
		{
			BuildFallbackShell();
		}
		else if (!bWarnedMissingWidgets)
		{
			bWarnedMissingWidgets = true;
			UE_LOG(LogPixelEventUI, Warning,
				TEXT("WBP_EventChoice is missing one of Overlay_Root, Img_Dim, Txt_Title, Txt_Body, Btn_Choice_1, Btn_Choice_2, Btn_Choice_3, Btn_Skip, Txt_Skip."));
		}
	}
	EnsureFallbackChoices();
}

void UPixelEventChoiceWidget::BuildFallbackShell()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo TitleFont = PixelAffixMakeFont(16);
	const FSlateFontInfo BodyFont = PixelAffixMakeFont(12);

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

	USizeBox* Panel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Size_Panel"));
	Panel->SetWidthOverride(420.f);
	if (UOverlaySlot* PanelSlot = Overlay_Root->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Event"));
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Panel->SetContent(Column);

	Txt_Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Title"));
	Txt_Body = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Body"));
	StyleLine(Txt_Title, TitleFont, ETextJustify::Center, false);
	StyleLine(Txt_Body, BodyFont, ETextJustify::Center, true);
	Txt_Body->SetWrapTextAt(400.f);

	ChoiceColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Choices"));
	ChoiceColumn->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	Btn_Skip = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Btn_Skip"));
	PaintSkip(Btn_Skip);
	Txt_Skip = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Skip"));
	StyleLine(Txt_Skip, BodyFont, ETextJustify::Center, false);
	if (UButtonSlot* SkipSlot = Cast<UButtonSlot>(Btn_Skip->SetContent(Txt_Skip)))
	{
		SkipSlot->SetHorizontalAlignment(HAlign_Fill);
		SkipSlot->SetVerticalAlignment(VAlign_Center);
	}

	auto AddRow = [Column](UWidget* Widget, float Bottom)
	{
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Widget))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, Bottom));
		}
	};
	AddRow(Txt_Title, 8.f);
	AddRow(Txt_Body, 16.f);
	AddRow(ChoiceColumn, 12.f);
	AddRow(Btn_Skip, 0.f);

	bBuiltFallbackShell = true;
	BindSkip();
}

void UPixelEventChoiceWidget::EnsureFallbackChoices()
{
	if (Btn_Choice_1 && Btn_Choice_2 && Btn_Choice_3)
	{
		return;
	}
	if (!bBuiltFallbackShell || !ChoiceColumn)
	{
		return;
	}

	TObjectPtr<UPixelEventChoiceButton>* Slots[MaxChoices] = {&Btn_Choice_1, &Btn_Choice_2, &Btn_Choice_3};
	const TCHAR* Names[MaxChoices] = {TEXT("Btn_Choice_1"), TEXT("Btn_Choice_2"), TEXT("Btn_Choice_3")};
	for (int32 Index = 0; Index < MaxChoices; ++Index)
	{
		if (*Slots[Index])
		{
			continue;
		}
		UPixelEventChoiceButton* Button = CreateWidget<UPixelEventChoiceButton>(this, UPixelEventChoiceButton::StaticClass(), Names[Index]);
		if (!Button)
		{
			UE_LOG(LogPixelEventUI, Warning, TEXT("EventChoice failed to create %s."), Names[Index]);
			continue;
		}
		*Slots[Index] = Button;
		Button->SetVisibility(ESlateVisibility::Collapsed);
		if (UVerticalBoxSlot* Slot = ChoiceColumn->AddChildToVerticalBox(Button))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetPadding(FMargin(0.f, 0.f, 0.f, Index < MaxChoices - 1 ? 8.f : 0.f));
		}
	}
	BindChoices();
}

void UPixelEventChoiceWidget::BindChoices()
{
	UPixelEventChoiceButton* Buttons[MaxChoices] = {Btn_Choice_1, Btn_Choice_2, Btn_Choice_3};
	for (int32 Index = 0; Index < MaxChoices; ++Index)
	{
		UPixelEventChoiceButton* Button = Buttons[Index];
		if (!Button)
		{
			continue;
		}
		Button->OnChoiceClicked.RemoveDynamic(this, &UPixelEventChoiceWidget::HandleChoiceButtonClicked);
		Button->OnChoiceClicked.AddDynamic(this, &UPixelEventChoiceWidget::HandleChoiceButtonClicked);
	}
}

void UPixelEventChoiceWidget::BindSkip()
{
	if (!Btn_Skip)
	{
		return;
	}
	Btn_Skip->OnClicked.RemoveDynamic(this, &UPixelEventChoiceWidget::HandleSkipClicked);
	Btn_Skip->OnClicked.AddDynamic(this, &UPixelEventChoiceWidget::HandleSkipClicked);
}

void UPixelEventChoiceWidget::ApplyView()
{
	if (Txt_Title)
	{
		Txt_Title->SetText(ActiveView.DisplayName);
	}
	if (Txt_Body)
	{
		Txt_Body->SetText(ActiveView.PromptBody);
		Txt_Body->SetWrapTextAt(400.f);
		Txt_Body->SetAutoWrapText(true);
	}
	if (Txt_Skip)
	{
		Txt_Skip->SetText(ActiveView.SkipHint);
	}

	const int32 Count = ChoiceCount();
	for (int32 Index = 0; Index < MaxChoices; ++Index)
	{
		UPixelEventChoiceButton* Button = ButtonAt(Index);
		if (!Button)
		{
			continue;
		}
		if (Index < Count)
		{
			const FPixelEventChoiceOption& Option = ActiveView.Choices[Index];
			Button->SetOption(Option.ChoiceId, Option.Label, Index + 1);
			Button->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Button->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Btn_Skip)
	{
		Btn_Skip->SetVisibility(ActiveView.bAllowDoorSkip ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPixelEventChoiceWidget::ResolveConfirmedChoice(FName ChoiceId)
{
	if (IsDesignTime())
	{
		return;
	}

	UEventSubsystem* Events = UEventSubsystem::Get(this);
	if (!Events)
	{
		UE_LOG(LogPixelEventUI, Warning,
			TEXT("EventChoice Resolve skipped: no EventSubsystem. EventId=%s ChoiceId=%s. UI still closes."),
			*ActiveView.EventId.ToString(),
			*ChoiceId.ToString());
		return;
	}

	FEventResolveView Result;
	const bool bOk = Events->ResolveEventChoice(ActiveView.EventId, ChoiceId, Result);
	if (!bOk)
	{
		UE_LOG(LogPixelEventUI, Warning,
			TEXT("EventChoice Resolve failed EventId=%s ChoiceId=%s: %s. UI still closes. Effects are not applied again."),
			*ActiveView.EventId.ToString(),
			*ChoiceId.ToString(),
			Result.Error.IsEmpty() ? TEXT("unknown") : *Result.Error);
		return;
	}

	UE_LOG(LogPixelEventUI, Log,
		TEXT("EventChoice Resolve ok EventId=%s ChoiceId=%s RunScrap=%d HP=%.1f/%.1f."),
		*ActiveView.EventId.ToString(),
		*ChoiceId.ToString(),
		Result.RunScrap,
		Result.CurrentHP,
		Result.MaxHP);
}

void UPixelEventChoiceWidget::ConfirmIndex(int32 Index)
{
	if (!bOpen || bResolving || Index < 0 || Index >= ChoiceCount())
	{
		return;
	}
	const FName ChoiceId = ActiveView.Choices[Index].ChoiceId;
	if (ChoiceId.IsNone())
	{
		UE_LOG(LogPixelEventUI, Warning, TEXT("EventChoice slot %d has no ChoiceId."), Index);
		return;
	}

	bResolving = true;
	ResolveConfirmedChoice(ChoiceId);
	UE_LOG(LogPixelEventUI, Log, TEXT("OnEventChoiceSelected ChoiceId=%s."), *ChoiceId.ToString());
	ShowLabLine(FString::Printf(TEXT("OnEventChoiceSelected %s"), *ChoiceId.ToString()));
	OnEventChoiceSelected.Broadcast(ChoiceId);
	if (bOpen)
	{
		CloseInternal();
	}
}

void UPixelEventChoiceWidget::Skip()
{
	if (!bOpen || bResolving || !ActiveView.bAllowDoorSkip)
	{
		return;
	}
	bResolving = true;
	UE_LOG(LogPixelEventUI, Log, TEXT("OnEventSkipped. No reward."));
	ShowLabLine(TEXT("OnEventSkipped"));
	OnEventSkipped.Broadcast();
	if (bOpen)
	{
		CloseInternal();
	}
}

void UPixelEventChoiceWidget::CloseInternal()
{
	if (!bOpen && !bCapturedInput && !bPawnControlBlocked)
	{
		return;
	}
	const bool bWasOpen = bOpen;
	bOpen = false;
	bResolving = false;
	FocusedIndex = 0;
	RefreshFocus();
	if (!IsDesignTime())
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
	ReleaseInput();
	SetPawnControlBlocked(false);
	if (bWasOpen)
	{
		UE_LOG(LogPixelEventUI, Log, TEXT("OnEventClosed."));
		OnEventClosed.Broadcast();
	}
}

void UPixelEventChoiceWidget::MoveFocus(int32 Delta)
{
	const int32 Count = FocusSlotCount();
	if (Count <= 0)
	{
		return;
	}
	FocusedIndex = (FocusedIndex + Delta) % Count;
	if (FocusedIndex < 0)
	{
		FocusedIndex += Count;
	}
	RefreshFocus();
}

void UPixelEventChoiceWidget::RefreshFocus()
{
	const int32 Count = ChoiceCount();
	for (int32 Index = 0; Index < MaxChoices; ++Index)
	{
		if (UPixelEventChoiceButton* Button = ButtonAt(Index))
		{
			Button->SetFocused(bOpen && Index == FocusedIndex);
		}
	}
	if (Btn_Skip)
	{
		const bool bSkipFocused = bOpen && ActiveView.bAllowDoorSkip && FocusedIndex == Count;
		Btn_Skip->SetRenderTranslation(FVector2D(0.f, bSkipFocused ? -1.f : 0.f));
	}
}

int32 UPixelEventChoiceWidget::ChoiceCount() const
{
	return FMath::Clamp(ActiveView.Choices.Num(), 0, MaxChoices);
}

int32 UPixelEventChoiceWidget::FocusSlotCount() const
{
	return ChoiceCount() + (ActiveView.bAllowDoorSkip ? 1 : 0);
}

void UPixelEventChoiceWidget::AcquireInput()
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

void UPixelEventChoiceWidget::ReleaseInput()
{
	if (!bCapturedInput)
	{
		return;
	}
	bCapturedInput = false;
	if (!GetWorld() || GetWorld()->bIsTearingDown)
	{
		return;
	}
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	FInputModeGameOnly GameOnly;
	PC->SetInputMode(GameOnly);
	PC->SetShowMouseCursor(false);
}

void UPixelEventChoiceWidget::SetPawnControlBlocked(bool bBlocked)
{
	if (bPawnControlBlocked == bBlocked)
	{
		return;
	}
	APlayerController* PC = GetOwningPlayer();
	APixelSideCharacter* Character = PC ? Cast<APixelSideCharacter>(PC->GetPawn()) : nullptr;
	if (!Character)
	{
		if (bBlocked && !bWarnedMissingPawn)
		{
			bWarnedMissingPawn = true;
			UE_LOG(LogPixelEventUI, Warning,
				TEXT("EventChoice could not block move/fire: pawn is not APixelSideCharacter. World stays unpaused."));
		}
		if (!bBlocked)
		{
			bPawnControlBlocked = false;
		}
		return;
	}
	Character->SetEventChoiceBlocksControl(bBlocked);
	bPawnControlBlocked = bBlocked;
}

UPixelEventChoiceButton* UPixelEventChoiceWidget::ButtonAt(int32 Index) const
{
	switch (Index)
	{
	case 0: return Btn_Choice_1;
	case 1: return Btn_Choice_2;
	case 2: return Btn_Choice_3;
	default: return nullptr;
	}
}
