#include "Combat/CombatDebugReadComponent.h"

#include "Combat/DamageableComponent.h"
#include "Combat/PixelSideCharacter.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/StatModifiersComponent.h"
#include "Combat/WeaponControllerComponent.h"

UCombatDebugReadComponent::UCombatDebugReadComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UDamageableComponent* UCombatDebugReadComponent::FindDamageable(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	if (APixelSideCharacter* Player = Cast<APixelSideCharacter>(Actor))
	{
		if (UDamageableComponent* Damageable = Player->GetDamageable())
		{
			return Damageable;
		}
	}
	return Actor->FindComponentByClass<UDamageableComponent>();
}

const UWeaponControllerComponent* UCombatDebugReadComponent::FindWeapon(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	if (const APixelSideCharacter* Player = Cast<APixelSideCharacter>(Actor))
	{
		if (const UWeaponControllerComponent* Weapon = Player->GetWeaponController())
		{
			return Weapon;
		}
	}
	return Actor->FindComponentByClass<UWeaponControllerComponent>();
}

float UCombatDebugReadComponent::ReadFireRate(AActor* Actor)
{
	const UStatModifiersComponent* Stats = nullptr;
	if (const APixelSideCharacter* Player = Cast<APixelSideCharacter>(Actor))
	{
		Stats = Player->GetStatModifiers();
	}
	if (!Stats && Actor)
	{
		Stats = Actor->FindComponentByClass<UStatModifiersComponent>();
	}
	return Stats ? Stats->GetStat(EStatId::FireRate) : PixelSideCombat::FireRateBase;
}

FPixelCombatDebugView UCombatDebugReadComponent::ReadActor(AActor* Actor)
{
	FPixelCombatDebugView View;
	View.FireRate = ReadFireRate(Actor);
	View.ResolvedFireCooldown = 1.f / FMath::Max(PixelSideCombat::FireRateBase, PixelSideCombat::MinFireRate);

	UDamageableComponent* Damageable = FindDamageable(Actor);
	if (Damageable)
	{
		View.bHasDamageable = true;
		View.Health = Damageable->GetCurrentHealth();
		View.MaxHealth = Damageable->GetMaxHealth();
		View.bAlive = IDamageable::Execute_IsAlive(Damageable);
		View.IFrameRemaining = FMath::Max(0.f, Damageable->GetIFrameRemaining());
		View.Team = Damageable->GetTeam();
		View.LastDamageTaken = FMath::Max(0.f, Damageable->GetLastDamageTaken());
		View.LastDamageTime = Damageable->GetLastDamageWorldTime();
	}

	const UWeaponControllerComponent* Weapon = FindWeapon(Actor);
	if (Weapon)
	{
		View.bHasWeapon = true;
		const FName WeaponId = Weapon->GetWeaponId();
		View.WeaponName = WeaponId.IsNone() ? FString() : WeaponId.ToString();
		View.FireCooldownRemaining = FMath::Max(0.f, Weapon->GetFireCooldownRemaining());
		View.FireInterval = FMath::Max(0.f, Weapon->GetFireInterval());
		View.ResolvedFireCooldown = Weapon->GetResolvedFireCooldown();
	}

	if (View.ResolvedFireCooldown <= KINDA_SMALL_NUMBER)
	{
		View.ResolvedFireCooldown = (View.FireInterval > KINDA_SMALL_NUMBER)
			? View.FireInterval
			: (1.f / FMath::Max(PixelSideCombat::FireRateBase, PixelSideCombat::MinFireRate));
	}

	return View;
}

void UCombatDebugReadComponent::GetHealth_Implementation(float& Health, float& MaxHealth)
{
	const FPixelCombatDebugView View = ReadActor(GetOwner());
	Health = View.Health;
	MaxHealth = View.MaxHealth;
}

bool UCombatDebugReadComponent::IsAlive_Implementation()
{
	return ReadActor(GetOwner()).bAlive;
}

float UCombatDebugReadComponent::GetIFrameRemaining_Implementation()
{
	return ReadActor(GetOwner()).IFrameRemaining;
}

ETeamId UCombatDebugReadComponent::GetTeamId_Implementation()
{
	return ReadActor(GetOwner()).Team;
}

FText UCombatDebugReadComponent::GetWeaponName_Implementation()
{
	const FString Name = ReadActor(GetOwner()).WeaponName;
	return Name.IsEmpty() ? FText::GetEmpty() : FText::FromString(Name);
}

float UCombatDebugReadComponent::GetFireCooldownRemaining_Implementation()
{
	return ReadActor(GetOwner()).FireCooldownRemaining;
}

float UCombatDebugReadComponent::GetFireInterval_Implementation()
{
	return ReadActor(GetOwner()).FireInterval;
}

float UCombatDebugReadComponent::GetFireRate_Implementation()
{
	return ReadActor(GetOwner()).FireRate;
}

float UCombatDebugReadComponent::GetResolvedFireCooldown_Implementation()
{
	return ReadActor(GetOwner()).ResolvedFireCooldown;
}

float UCombatDebugReadComponent::GetLastDamageTaken_Implementation()
{
	return ReadActor(GetOwner()).LastDamageTaken;
}

float UCombatDebugReadComponent::GetLastDamageTime_Implementation()
{
	return ReadActor(GetOwner()).LastDamageTime;
}
