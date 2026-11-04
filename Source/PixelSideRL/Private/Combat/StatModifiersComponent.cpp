#include "Combat/StatModifiersComponent.h"
#include "Combat/PixelSideCombatMath.h"

UStatModifiersComponent::UStatModifiersComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatModifiersComponent::SetBaseStat(EStatId Stat, float Value)
{
	if (const float* Found = BaseStats.Find(Stat))
	{
		if (FMath::IsNearlyEqual(*Found, Value))
		{
			return;
		}
	}
	BaseStats.Add(Stat, Value);
	NotifyStatsRebuilt();
}

float UStatModifiersComponent::GetStat(EStatId Stat) const
{
	if (const float* Found = BaseStats.Find(Stat))
	{
		return *Found;
	}
	return FallbackBase(Stat);
}

void UStatModifiersComponent::NotifyStatsRebuilt()
{
	OnStatsRebuilt.Broadcast();
}

float UStatModifiersComponent::FallbackBase(EStatId Stat)
{
	switch (Stat)
	{
	case EStatId::MaxHP:
		return PixelSideCombat::PlayerMaxHealth;
	case EStatId::MoveSpeed:
		return PixelSideCombat::MaxWalkSpeed;
	case EStatId::FireRate:
		return PixelSideCombat::FireRateBase;
	case EStatId::Damage:
		return PixelSideCombat::PeaDamage;
	case EStatId::Knockback:
		return PixelSideCombat::PeaKnockback;
	case EStatId::ProjectileCount:
		return 1.f;
	case EStatId::Pierce:
		return 0.f;
	default:
		return 0.f;
	}
}
