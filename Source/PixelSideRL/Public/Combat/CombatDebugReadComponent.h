#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/CombatDebugRead.h"
#include "CombatDebugReadComponent.generated.h"

class AActor;
class UDamageableComponent;
class UWeaponControllerComponent;

/**
 * One read of the pawn's existing combat components.
 * No health or weapon state of its own.
 */
struct PIXELSIDERL_API FPixelCombatDebugView
{
	float Health = 0.f;
	float MaxHealth = 0.f;
	bool bAlive = false;
	float IFrameRemaining = 0.f;
	ETeamId Team = ETeamId::Neutral;
	FString WeaponName;
	float FireCooldownRemaining = 0.f;
	float FireInterval = 0.f;
	float FireRate = 1.f;
	float ResolvedFireCooldown = 1.f;
	float LastDamageTaken = 0.f;
	float LastDamageTime = -1.f;
	bool bHasDamageable = false;
	bool bHasWeapon = false;
};

/**
 * AC_CombatDebugRead. Forwards BI_CombatDebugRead to the owner's damageable and weapon.
 * No tick. Does not apply damage or fire.
 */
UCLASS(ClassGroup = (PixelSide), meta = (BlueprintSpawnableComponent, DisplayName = "AC_CombatDebugRead"))
class PIXELSIDERL_API UCombatDebugReadComponent : public UActorComponent, public ICombatDebugRead
{
	GENERATED_BODY()

public:
	UCombatDebugReadComponent();

	/** Read AC_Damageable / AC_WeaponController on Actor. Prefer the character accessors. */
	static FPixelCombatDebugView ReadActor(AActor* Actor);

	virtual void GetHealth_Implementation(float& Health, float& MaxHealth) override;
	virtual bool IsAlive_Implementation() override;
	virtual float GetIFrameRemaining_Implementation() override;
	virtual ETeamId GetTeamId_Implementation() override;
	virtual FText GetWeaponName_Implementation() override;
	virtual float GetFireCooldownRemaining_Implementation() override;
	virtual float GetFireInterval_Implementation() override;
	virtual float GetFireRate_Implementation() override;
	virtual float GetResolvedFireCooldown_Implementation() override;
	virtual float GetLastDamageTaken_Implementation() override;
	virtual float GetLastDamageTime_Implementation() override;

private:
	static UDamageableComponent* FindDamageable(AActor* Actor);
	static const UWeaponControllerComponent* FindWeapon(AActor* Actor);
	static float ReadFireRate(AActor* Actor);
};
