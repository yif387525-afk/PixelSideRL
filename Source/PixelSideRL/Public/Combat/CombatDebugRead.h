#pragma once

#include "CoreMinimal.h"
#include "Combat/DamageSpec.h"
#include "UObject/Interface.h"
#include "CombatDebugRead.generated.h"

/**
 * BI_CombatDebugRead.
 * Thin read-only facade for the Lab / _Dev combat HUD.
 * Implementations forward to UDamageableComponent and UWeaponControllerComponent.
 * 只读：不要调用 ApplyDamage、TryFire，也不要改武器 DA。
 */
UINTERFACE(BlueprintType, meta = (DisplayName = "BI_CombatDebugRead"))
class PIXELSIDERL_API UCombatDebugRead : public UInterface
{
	GENERATED_BODY()
};

class PIXELSIDERL_API ICombatDebugRead
{
	GENERATED_BODY()

public:
	/** Current and max health from AC_Damageable. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Debug")
	void GetHealth(float& Health, float& MaxHealth);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Debug")
	bool IsAlive();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Debug")
	float GetIFrameRemaining();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Debug")
	ETeamId GetTeamId();

	/** WeaponId from AC_WeaponController (DA row). Empty if the pawn has no weapon. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Debug")
	FText GetWeaponName();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Debug")
	float GetFireCooldownRemaining();

	/**
	 * Absolute DA FireInterval. 0 when the pawn has no weapon.
	 * CD line denominator prefers GetResolvedFireCooldown (interval / FireRate).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Debug")
	float GetFireInterval();

	/** AC_StatModifiers::GetStat(FireRate). Base 1 with no affix; RapidFire Mul 0.12 → about 1.12. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Debug")
	float GetFireRate();

	/**
	 * Seconds the cooldown remaining counts through: FireInterval / GetStat(FireRate).
	 * When that is unavailable, 1/FireRateBase.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Debug")
	float GetResolvedFireCooldown();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Debug")
	float GetLastDamageTaken();

	/** World time of the last damage. Negative if this pawn has not been hit. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat|Debug")
	float GetLastDamageTime();
};
