#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PixelRoomExit.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * BP_RoomExit. Socket for Exit.Left / Exit.Right.
 * bRequireClear stays blocked until the room director opens it.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "BP_RoomExit"))
class PIXELSIDERL_API APixelRoomExit : public AActor
{
	GENERATED_BODY()

public:
	APixelRoomExit();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "PixelSide|Room")
	void ApplyLock(bool bFlowOpen);

	UFUNCTION(BlueprintPure, Category = "PixelSide|Room")
	bool IsPassageOpen() const { return bPassageOpen; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	FName ExitId;

	/** Combat forward door. The director keeps this shut until the clear and the affix confirm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bRequireClear = false;

	/** Zero uses the built-in offset (left socket steps inside WALL_L so the player spawn does not touch it). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	FVector VolumeRelativeLocation = FVector::ZeroVector;

protected:
	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* Overlapped,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void RefreshVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	TObjectPtr<USceneComponent> Root = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	TObjectPtr<UBoxComponent> Passage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	TObjectPtr<UStaticMeshComponent> DoorMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bPassageOpen = false;
};
