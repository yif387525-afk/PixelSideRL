#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PixelDebugHurtTurret.generated.h"

class UDamageableComponent;
class UStaticMeshComponent;

/**
 * Lab debug turret. Fires PixelSideCombat::GruntDamage on GruntFireInterval
 * (≤25% player MaxHP, interval ≥ 1 s) so the player i-frame can be seen.
 * Not future Grunt AI. CombatFeel §7 still lists Grunt as 0.8 s / damage 6.
 * Body stays WorldStatic (shots and visibility still block) but ignores Pawns,
 * because the mesh sits on the 2D route. Damage acceptance is unchanged.
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
