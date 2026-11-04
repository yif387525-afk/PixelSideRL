#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/ProjectilePea.h"
#include "Combat/DA_Weapon.generated.h"

/**
 * Weapon row. UDA_Weapon_Peashooter seeds CombatFeel §4.1.
 * Create Content/Data/Weapons/DA_Weapon_Peashooter from this class in the editor;
 * until that asset exists, the weapon component uses the peashooter CDO values.
 */
UCLASS(BlueprintType)
class PIXELSIDERL_API UDA_Weapon : public UDataAsset
{
	GENERATED_BODY()

public:
	UDA_Weapon();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.01", Units = "s"))
	float FireInterval = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0", Units = "deg"))
	float Spread = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.1"))
	float ProjectileRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float Damage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float KnockbackToEnemy = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0", Units = "s"))
	float HitStop_Player = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0", Units = "s"))
	float HitStop_Enemy = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float CamShakeScale = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.05", Units = "s"))
	float ProjectileLife = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AProjectilePea> ProjectileClass;
};

/** C++ stand-in for Content/Data/Weapons/DA_Weapon_Peashooter. Defaults are §4.1. */
UCLASS(BlueprintType)
class PIXELSIDERL_API UDA_Weapon_Peashooter : public UDA_Weapon
{
	GENERATED_BODY()

public:
	UDA_Weapon_Peashooter();
};
