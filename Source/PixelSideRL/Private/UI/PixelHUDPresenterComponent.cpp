#include "UI/PixelHUDPresenterComponent.h"

#include "UI/PixelHUDWidget.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	constexpr int32 HUDZOrder = 10;
}

UPixelHUDPresenterComponent::UPixelHUDPresenterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	HUDWidgetClass = UPixelHUDWidget::StaticClass();
}

void UPixelHUDPresenterComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureMounted();
}

void UPixelHUDPresenterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HUDWidget)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UPixelHUDPresenterComponent::EnsureMounted()
{
	if (!bEnableHUD)
	{
		if (HUDWidget)
		{
			HUDWidget->RemoveFromParent();
			HUDWidget = nullptr;
		}
		return;
	}

	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}

	APlayerController* PC = nullptr;
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		PC = Cast<APlayerController>(Pawn->GetController());
	}
	if (!PC)
	{
		PC = World->GetFirstPlayerController();
	}
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	TSubclassOf<UUserWidget> WidgetClass = HUDWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UPixelHUDWidget::StaticClass();
	}
	if (HUDWidget && HUDWidget->GetClass() != WidgetClass)
	{
		HUDWidget->RemoveFromParent();
		HUDWidget = nullptr;
	}
	if (HUDWidget && HUDWidget->IsInViewport())
	{
		return;
	}
	if (!HUDWidget)
	{
		HUDWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
	}
	if (!HUDWidget)
	{
		return;
	}
	if (!HUDWidget->IsInViewport())
	{
		HUDWidget->AddToViewport(HUDZOrder);
	}
	HUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}
