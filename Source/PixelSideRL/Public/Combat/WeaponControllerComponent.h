#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Affix/AffixTypes.h"
#include "Combat/WeaponControllerComponent.generated.h"

class UDA_Weapon;
class UStatModifiersComponent;
class AProjectilePea;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPixelWeaponFired);

/**
 * AC_WeaponController. Full-auto peashooter along facing ±X.
 * Cooldown = FireInterval / GetStat(FireRate). Does not roll affixes.
 */
UCLASS(ClassGroup = (PixelSide), meta = (BlueprintSpawnableComponent, DisplayName = "AC_WeaponController"))
class PIXELSIDERL_API UWeaponControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponControllerComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWantsToFire(bool bNewWantsToFire);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool IsTriggerHeld() const { return bWantsToFire; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetFacingSign(float Sign);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool TryGetFireDir(FVector& OutDir) const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool TryFire();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetFireCooldownRemaining() const { return CooldownRemaining; }

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetFireInterval() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetResolvedFireCooldown() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	FName GetWeaponId() const;

	UFUNCTION(BlueprintPure, Category = "Weapon")
	UDA_Weapon* GetWeaponData() const { return WeaponData; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponData(UDA_Weapon* NewWeapon);

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnPixelWeaponFired OnWeaponFired;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UDA_Weapon> WeaponData = nullptr;

protected:
	void EnsureWeaponData();
	void PushBasesToStats();
	UStatModifiersComponent* FindStats() const;
	float ResolveStat(EStatId StatId, float BaseValue) const;
	FVector MuzzleLocation() const;

	float FacingSign = 1.f;
	float CooldownRemaining = 0.f;
	bool bWantsToFire = false;
};
