#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PixelDebugHurtTurret.generated.h"

class UDamageableComponent;
class UStaticMeshComponent;

/**
 * Lab turret. Fires a slow enemy pea (Grunt damage / interval) so the player i-frame can be seen.
 * Not an enemy AI.
 */
UCLASS(Blueprintable, BlueprintType)
class PIXELSIDERL_API APixelDebugHurtTurret : public AActor
{
	GENERATED_BODY()

public:
	APixelDebugHurtTurret();

	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void FireAtPlayer();

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<USceneComponent> Root = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UStaticMeshComponent> BodyMesh = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Turret")
	TObjectPtr<UDamageableComponent> Damageable = nullptr;

	FTimerHandle FireTimer;
};
