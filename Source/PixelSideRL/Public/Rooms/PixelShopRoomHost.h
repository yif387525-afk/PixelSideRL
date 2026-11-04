#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PixelShopRoomHost.generated.h"

/**
 * Room_Shop_01 entry. Not the P0.7 combat clock.
 * On play it reads the shop CSV, leaves both doors unlocked, and calls
 * GetShopPresenter()->OpenShop(Offers, Scrap, true). OpenShop reads RunScrap.
 * It does not call OpenShopDemo. F11 still opens the Lab demo widget itself.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "ShopRoomHost"))
struct FPixelShopRoomHostPrivate;

class PIXELSIDERL_API APixelShopRoomHost : public AActor
{
	GENERATED_BODY()

public:
	APixelShopRoomHost();
	virtual ~APixelShopRoomHost() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Default Room_Shop_01. Any meta whose type is Shop also opens. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	FName RoomId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bAutoOpen = true;

	/** Off once a shop umap carries the geometry. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bBuildGreyboxFromCsv = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bPlacePlayerAtAnchor = true;

	UFUNCTION(BlueprintPure, Category = "PixelSide|Room")
	bool HasOpenedShop() const { return bOpenedShop; }

private:
	bool LoadAndEnter();
	bool TryOpenShop();

	bool bEntered = false;
	bool bOpenedShop = false;
	bool bPlayerPlaced = false;
	int32 OpenAttempts = 0;
	FPixelShopRoomHostPrivate* Priv = nullptr;
};
