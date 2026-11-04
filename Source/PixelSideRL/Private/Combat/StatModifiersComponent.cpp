#include "Combat/StatModifiersComponent.h"

#include "Affix/AffixSubsystemDefault.h"
#include "Combat/PixelSideCombatMath.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UStatModifiersComponent::UStatModifiersComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatModifiersComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureAffixBound();
}

void UStatModifiersComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindAffix();
	Super::EndPlay(EndPlayReason);
}

void UStatModifiersComponent::BeginDestroy()
{
	UnbindAffix();
	Super::BeginDestroy();
}

void UStatModifiersComponent::SetBaseStat(EStatId Stat, float Value)
{
	const float* Found = BaseStats.Find(Stat);
	const bool bChanged = !Found || !FMath::IsNearlyEqual(*Found, Value);
	if (bChanged)
	{
		BaseStats.Add(Stat, Value);
	}
	EnsureAffixBound();
	if (BoundAffix)
	{
		BoundAffix->SetBaseStat(Stat, Value);
		return;
	}
	if (bChanged)
	{
		NotifyStatsRebuilt();
	}
}

float UStatModifiersComponent::GetStat(EStatId Stat) const
{
	return GetStatForWeapon(Stat, NAME_None);
}

float UStatModifiersComponent::GetStatForWeapon(EStatId Stat, FName WeaponId) const
{
	UStatModifiersComponent* Self = const_cast<UStatModifiersComponent*>(this);
	Self->EnsureAffixBound();
	if (UAffixSubsystem* Affix = Self->BoundAffix)
	{
		if (const float* Local = BaseStats.Find(Stat))
		{
			if (!FMath::IsNearlyEqual(Affix->GetStatBase(Stat), *Local))
			{
				Affix->SetBaseStat(Stat, *Local);
			}
		}
		return Affix->GetStatForWeapon(Stat, WeaponId);
	}
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

void UStatModifiersComponent::HandleSubsystemAffixChanged(FAffixChangeEvent Event)
{
	OnAffixChanged.Broadcast(Event);
}

void UStatModifiersComponent::HandleSubsystemStatsRebuilt()
{
	OnStatsRebuilt.Broadcast();
}

void UStatModifiersComponent::EnsureAffixBound()
{
	if (BoundAffix)
	{
		return;
	}
	UAffixSubsystem* Affix = ResolveAffixSubsystem();
	if (!Affix)
	{
		return;
	}
	Affix->OnAffixChanged.AddDynamic(this, &UStatModifiersComponent::HandleSubsystemAffixChanged);
	Affix->OnStatsRebuilt.AddDynamic(this, &UStatModifiersComponent::HandleSubsystemStatsRebuilt);
	BoundAffix = Affix;
	for (const TPair<EStatId, float>& Pair : BaseStats)
	{
		Affix->SetBaseStat(Pair.Key, Pair.Value);
	}
}

void UStatModifiersComponent::UnbindAffix()
{
	if (!BoundAffix)
	{
		return;
	}
	BoundAffix->OnAffixChanged.RemoveDynamic(this, &UStatModifiersComponent::HandleSubsystemAffixChanged);
	BoundAffix->OnStatsRebuilt.RemoveDynamic(this, &UStatModifiersComponent::HandleSubsystemStatsRebuilt);
	BoundAffix = nullptr;
}

UAffixSubsystem* UStatModifiersComponent::ResolveAffixSubsystem() const
{
	return UAffixSubsystemDefault::Get(this);
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
