#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Affix/AffixSubsystem.h"
#include "WeaponControllerComponent.generated.h"

class UDA_Weapon;
class UStatModifiersComponent;
class AProjectilePea;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPixelWeaponFired);

/**
 * AC_WeaponController. Full-auto peashooter along facing ±X.
 * Cooldown = FireInterval / GetStat(FireRate) via ResolveStat. No AffixId branch, no Roll.
 *
 * 转发约定：只绑 AC_StatModifiers 的 OnAffixChanged / OnStatsRebuilt，不绑 GameInstance
 * UAffixSubsystem 的同名委托。词条官把 Subsystem 事件转到组件委托；两路都听会双刷。
 * 射速与伤害每枪 live GetStat，无缓存。事件里只重算进行中的冷却，不 PushBasesToStats
 * （避免把词条官写入的 FireRate 乘区盖回 Base 1）。换枪与 BeginPlay 仍 PushBasesToStats。
 * OnAffixChoiceClosed 不订阅：暂停/解暂停归界面与流程，枪公式不变。
 */
UCLASS(ClassGroup = (PixelSide), meta = (BlueprintSpawnableComponent, DisplayName = "AC_WeaponController"))
class PIXELSIDERL_API UWeaponControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponControllerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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

	/** Idempotent. Binds the owner's AC_StatModifiers only. */
	void BindStatRefresh();

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnPixelWeaponFired OnWeaponFired;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UDA_Weapon> WeaponData = nullptr;

protected:
	void EnsureWeaponData();
	void PushBasesToStats();
	void UnbindStatRefresh();
	void RecomputeInFlightCooldown();
	UStatModifiersComponent* FindStats() const;
	float ResolveStat(EStatId StatId, float BaseValue) const;
	FVector MuzzleLocation() const;

	UFUNCTION()
	void HandleAffixChanged(FAffixChangeEvent Event);

	UFUNCTION()
	void HandleStatsRebuilt();

	float FacingSign = 1.f;
	float CooldownRemaining = 0.f;
	/** Full cooldown applied to the current cycle. Used so a second delegate does not scale twice. */
	float AppliedCooldown = 0.f;
	bool bWantsToFire = false;
	bool bStatRefreshBound = false;
};
