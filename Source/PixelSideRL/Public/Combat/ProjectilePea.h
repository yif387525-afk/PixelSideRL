#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Combat/DamageSpec.h"
#include "Combat/ProjectilePea.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/** BP_Projectile_Pea. Overlap damages BI_Damageable; a blocking static hit destroys the shot. */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "BP_Projectile_Pea"))
class PIXELSIDERL_API AProjectilePea : public AActor
{
	GENERATED_BODY()

public:
	AProjectilePea();

	void LaunchShot(
		const FVector& Direction,
		float Speed,
		float Radius,
		float LifeSeconds,
		float InDamage,
		float InKnockback,
		float InHitStopVictim,
		float InHitStopInstigator,
		AActor* InSource,
		ETeamId InTeam);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	void HandleActor(AActor* OtherActor, const FHitResult* Hit, bool bBlockingHit);
	bool IsOwnerIgnored(const AActor* OtherActor) const;
	bool IsFriendly(const AActor* OtherActor) const;
	bool PassHitGate(AActor* OtherActor);
	FDamageSpec BuildSpec() const;
	void ApplyRadius(float Radius);
	void ApplyTeamColor();

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> Collision = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Visual = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement = nullptr;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> VisualMid = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	ETeamId Team = ETeamId::Player;

	UPROPERTY()
	TObjectPtr<AActor> DamageSource = nullptr;

	FVector ShotDirection = FVector::XAxisVector;
	float ShotSpeed = 0.f;
	float ShotDamage = 0.f;
	float ShotKnockback = 0.f;
	float ShotHitStopVictim = 0.f;
	float ShotHitStopInstigator = 0.f;
	float IgnoreOwnerUntilTime = 0.f;
	float LastImpactTime = -1000.f;
	bool bImpacted = false;
};
