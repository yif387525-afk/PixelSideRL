#include "UI/PixelShopPresenterComponent.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogPixelShopPresenter, Log, All);

UPixelShopPresenterComponent::UPixelShopPresenterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ShopWidgetClass = UPixelShopWidget::StaticClass();
}

void UPixelShopPresenterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ShopWidget)
	{
		if (ShopWidget->IsShopOpen())
		{
			ShopWidget->CloseShop();
		}
		ShopWidget->RemoveFromParent();
		ShopWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

UPixelShopWidget* UPixelShopPresenterComponent::OpenShop(
	const TArray<FPixelShopSlotOffer>& Offers, double Scrap, bool bDemoPause)
{
	UPixelShopWidget* Widget = EnsureWidget();
	if (!Widget)
	{
		UE_LOG(LogPixelShopPresenter, Warning, TEXT("AC_ShopPresenter could not create WBP_Shop."));
		return nullptr;
	}
	Widget->OpenShop(Offers, Scrap, bDemoPause);
	if (!Widget->IsShopOpen())
	{
		UE_LOG(LogPixelShopPresenter, Warning, TEXT("AC_ShopPresenter OpenShop did not open."));
		return nullptr;
	}
	UE_LOG(LogPixelShopPresenter, Log,
		TEXT("AC_ShopPresenter OpenShop. scrap=%.2f demoPause=%s. Shell viewport only. No RunScrap write."),
		Scrap,
		bDemoPause ? TEXT("true") : TEXT("false"));
	return Widget;
}

UPixelShopWidget* UPixelShopPresenterComponent::OpenShopDemo(int32 Layer)
{
	UPixelShopWidget* Widget = EnsureWidget();
	if (!Widget)
	{
		UE_LOG(LogPixelShopPresenter, Warning, TEXT("AC_ShopPresenter could not create WBP_Shop."));
		return nullptr;
	}
	Widget->OpenShopDemo(Layer);
	if (!Widget->IsShopOpen())
	{
		UE_LOG(LogPixelShopPresenter, Warning, TEXT("AC_ShopPresenter OpenShopDemo did not open."));
		return nullptr;
	}
	UE_LOG(LogPixelShopPresenter, Log,
		TEXT("AC_ShopPresenter OpenShopDemo layer=%d. Shell viewport only. No RunScrap write."),
		Layer);
	return Widget;
}

void UPixelShopPresenterComponent::CloseShop()
{
	if (!ShopWidget)
	{
		return;
	}
	ShopWidget->CloseShop();
}

bool UPixelShopPresenterComponent::IsShopOpen() const
{
	return ShopWidget && ShopWidget->IsShopOpen();
}

UPixelShopWidget* UPixelShopPresenterComponent::EnsureWidget()
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return nullptr;
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
		return nullptr;
	}

	TSubclassOf<UPixelShopWidget> WidgetClass = ShopWidgetClass;
	if (!WidgetClass)
	{
		WidgetClass = UPixelShopWidget::StaticClass();
	}
	if (ShopWidget && ShopWidget->GetClass() != WidgetClass)
	{
		if (ShopWidget->IsShopOpen())
		{
			ShopWidget->CloseShop();
		}
		ShopWidget->RemoveFromParent();
		ShopWidget = nullptr;
	}
	if (!ShopWidget)
	{
		ShopWidget = CreateWidget<UPixelShopWidget>(PC, WidgetClass);
	}
	return ShopWidget;
}
