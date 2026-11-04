#include "Affix/AffixChoicePresenterComponent.h"

#include "Affix/AffixSubsystemDefault.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/AffixChoiceDebugWidget.h"
#include "UI/PixelAffixChoiceWidget.h"

UAffixChoicePresenterComponent::UAffixChoicePresenterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAffixChoicePresenterComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Bound)
	{
		return;
	}
	if (AffixChoiceWidgetClass && bEnableNativeChoiceUI)
	{
		// Formal shell and the text stand-in must not both be on screen.
		bEnableNativeChoiceUI = false;
	}
	UAffixSubsystem* Affix = Resolve();
	if (!Affix)
	{
		return;
	}
	Affix->OnAffixChoiceOffered.AddDynamic(this, &UAffixChoicePresenterComponent::HandleOffered);
	Affix->OnAffixChoiceClosed.AddDynamic(this, &UAffixChoicePresenterComponent::HandleClosed);
	Bound = Affix;
	BindPauseKeys();
}

void UAffixChoicePresenterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
	if (FormalChoiceWidget)
	{
		FormalChoiceWidget->RemoveFromParent();
		FormalChoiceWidget = nullptr;
	}
	UnbindSubsystem();
	Super::EndPlay(EndPlayReason);
}

void UAffixChoicePresenterComponent::HandleOffered(const TArray<FAffixChoiceEntry>& Choices)
{
	BindPauseKeys();
	if (AffixChoiceWidgetClass)
	{
		ShowFormalChoice(Choices);
		return;
	}
	if (!bEnableNativeChoiceUI)
	{
		return;
	}
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}
	if (!Widget)
	{
		Widget = CreateWidget<UAffixChoiceDebugWidget>(PC, UAffixChoiceDebugWidget::StaticClass());
	}
	if (!Widget)
	{
		return;
	}
	Widget->ShowChoices(Choices);
	if (!Widget->IsInViewport())
	{
		Widget->AddToViewport(50);
	}
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(Widget->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
	Widget->SetKeyboardFocus();
}

void UAffixChoicePresenterComponent::HandleClosed(bool bConfirmed)
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (Widget)
	{
		Widget->RemoveFromParent();
	}
	if (UPixelAffixChoiceWidget* Choice = Cast<UPixelAffixChoiceWidget>(FormalChoiceWidget))
	{
		Choice->CloseFromPresenter(bConfirmed);
	}
	else if (FormalChoiceWidget)
	{
		FormalChoiceWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (PC)
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

void UAffixChoicePresenterComponent::ShowFormalChoice(const TArray<FAffixChoiceEntry>& Choices)
{
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC || !AffixChoiceWidgetClass)
	{
		return;
	}
	if (FormalChoiceWidget && FormalChoiceWidget->GetClass() != AffixChoiceWidgetClass)
	{
		FormalChoiceWidget->RemoveFromParent();
		FormalChoiceWidget = nullptr;
	}
	if (!FormalChoiceWidget)
	{
		FormalChoiceWidget = CreateWidget<UUserWidget>(PC, AffixChoiceWidgetClass);
	}
	if (!FormalChoiceWidget)
	{
		return;
	}
	if (!FormalChoiceWidget->IsInViewport())
	{
		FormalChoiceWidget->AddToViewport(100);
	}
	if (UPixelAffixChoiceWidget* Choice = Cast<UPixelAffixChoiceWidget>(FormalChoiceWidget))
	{
		Choice->PresentOfferedChoice(Choices);
	}
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(FormalChoiceWidget->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
	FormalChoiceWidget->SetKeyboardFocus();
}

void UAffixChoicePresenterComponent::ConfirmSlot(int32 Index)
{
	if (Bound && Bound->IsChoicePending())
	{
		Bound->ConfirmAffixChoice(Index);
	}
}

void UAffixChoicePresenterComponent::BindPauseKeys()
{
	if (bPauseKeysBound)
	{
		return;
	}
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC || !PC->InputComponent)
	{
		return;
	}
	FInputKeyBinding& Key0 = PC->InputComponent->BindKey(EKeys::One, IE_Pressed, this, &UAffixChoicePresenterComponent::ConfirmKey0);
	FInputKeyBinding& Key1 = PC->InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &UAffixChoicePresenterComponent::ConfirmKey1);
	FInputKeyBinding& Key2 = PC->InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &UAffixChoicePresenterComponent::ConfirmKey2);
	Key0.bExecuteWhenPaused = true;
	Key1.bExecuteWhenPaused = true;
	Key2.bExecuteWhenPaused = true;
	bPauseKeysBound = true;
}

void UAffixChoicePresenterComponent::UnbindSubsystem()
{
	if (!Bound)
	{
		return;
	}
	Bound->OnAffixChoiceOffered.RemoveDynamic(this, &UAffixChoicePresenterComponent::HandleOffered);
	Bound->OnAffixChoiceClosed.RemoveDynamic(this, &UAffixChoicePresenterComponent::HandleClosed);
	Bound = nullptr;
}

UAffixSubsystem* UAffixChoicePresenterComponent::Resolve() const
{
	return UAffixSubsystemDefault::Get(this);
}

void UAffixChoicePresenterComponent::ConfirmKey0()
{
	ConfirmSlot(0);
}

void UAffixChoicePresenterComponent::ConfirmKey1()
{
	ConfirmSlot(1);
}

void UAffixChoicePresenterComponent::ConfirmKey2()
{
	ConfirmSlot(2);
}
