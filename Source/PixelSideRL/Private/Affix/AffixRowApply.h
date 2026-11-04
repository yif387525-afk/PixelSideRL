#pragma once

#include "Affix/AffixTypes.h"
#include "Affix/AffixStatMath.h"
#include "UObject/SoftObjectPath.h"

namespace PixelSideAffixUE
{

inline EStatId ToStatId(PixelSideAffix::EStat Stat)
{
	switch (Stat)
	{
	case PixelSideAffix::EStat::MaxHP: return EStatId::MaxHP;
	case PixelSideAffix::EStat::MoveSpeed: return EStatId::MoveSpeed;
	case PixelSideAffix::EStat::FireRate: return EStatId::FireRate;
	case PixelSideAffix::EStat::Damage: return EStatId::Damage;
	case PixelSideAffix::EStat::CritChance: return EStatId::CritChance;
	case PixelSideAffix::EStat::CritMult: return EStatId::CritMult;
	case PixelSideAffix::EStat::MagSize: return EStatId::MagSize;
	case PixelSideAffix::EStat::ReloadSpeed: return EStatId::ReloadSpeed;
	case PixelSideAffix::EStat::ProjectileCount: return EStatId::ProjectileCount;
	case PixelSideAffix::EStat::Pierce: return EStatId::Pierce;
	case PixelSideAffix::EStat::Knockback: return EStatId::Knockback;
	case PixelSideAffix::EStat::PickupRadius: return EStatId::PickupRadius;
	case PixelSideAffix::EStat::CoinGain: return EStatId::CoinGain;
	case PixelSideAffix::EStat::ScrapGain: return EStatId::ScrapGain;
	default: return EStatId::Damage;
	}
}

inline PixelSideAffix::EStat ToLiteStat(EStatId Stat)
{
	switch (Stat)
	{
	case EStatId::MaxHP: return PixelSideAffix::EStat::MaxHP;
	case EStatId::MoveSpeed: return PixelSideAffix::EStat::MoveSpeed;
	case EStatId::FireRate: return PixelSideAffix::EStat::FireRate;
	case EStatId::Damage: return PixelSideAffix::EStat::Damage;
	case EStatId::CritChance: return PixelSideAffix::EStat::CritChance;
	case EStatId::CritMult: return PixelSideAffix::EStat::CritMult;
	case EStatId::MagSize: return PixelSideAffix::EStat::MagSize;
	case EStatId::ReloadSpeed: return PixelSideAffix::EStat::ReloadSpeed;
	case EStatId::ProjectileCount: return PixelSideAffix::EStat::ProjectileCount;
	case EStatId::Pierce: return PixelSideAffix::EStat::Pierce;
	case EStatId::Knockback: return PixelSideAffix::EStat::Knockback;
	case EStatId::PickupRadius: return PixelSideAffix::EStat::PickupRadius;
	case EStatId::CoinGain: return PixelSideAffix::EStat::CoinGain;
	case EStatId::ScrapGain: return PixelSideAffix::EStat::ScrapGain;
	default: return PixelSideAffix::EStat::Damage;
	}
}

inline EModifierOp ToOp(PixelSideAffix::EOp Op)
{
	switch (Op)
	{
	case PixelSideAffix::EOp::Add: return EModifierOp::Add;
	case PixelSideAffix::EOp::Mul: return EModifierOp::Mul;
	case PixelSideAffix::EOp::Override: return EModifierOp::Override;
	default: return EModifierOp::Add;
	}
}

inline PixelSideAffix::EOp ToLiteOp(EModifierOp Op)
{
	switch (Op)
	{
	case EModifierOp::Add: return PixelSideAffix::EOp::Add;
	case EModifierOp::Mul: return PixelSideAffix::EOp::Mul;
	case EModifierOp::Override: return PixelSideAffix::EOp::Override;
	default: return PixelSideAffix::EOp::Add;
	}
}

inline EAffixRarity ToRarity(PixelSideAffix::ERarity Rarity)
{
	switch (Rarity)
	{
	case PixelSideAffix::ERarity::Common: return EAffixRarity::Common;
	case PixelSideAffix::ERarity::Rare: return EAffixRarity::Rare;
	case PixelSideAffix::ERarity::Epic: return EAffixRarity::Epic;
	case PixelSideAffix::ERarity::Legendary: return EAffixRarity::Legendary;
	default: return EAffixRarity::Common;
	}
}

inline EAffixStackRule ToStack(PixelSideAffix::EStack Stack)
{
	switch (Stack)
	{
	case PixelSideAffix::EStack::Unique: return EAffixStackRule::Unique;
	case PixelSideAffix::EStack::AdditiveStack: return EAffixStackRule::AdditiveStack;
	case PixelSideAffix::EStack::RefreshDuration: return EAffixStackRule::RefreshDuration;
	default: return EAffixStackRule::Unique;
	}
}

inline EAffixTrigger ToTrigger(PixelSideAffix::ETrigger Trigger)
{
	switch (Trigger)
	{
	case PixelSideAffix::ETrigger::OnAcquire: return EAffixTrigger::OnAcquire;
	case PixelSideAffix::ETrigger::OnHit: return EAffixTrigger::OnHit;
	case PixelSideAffix::ETrigger::OnKill: return EAffixTrigger::OnKill;
	case PixelSideAffix::ETrigger::OnRoomClear: return EAffixTrigger::OnRoomClear;
	case PixelSideAffix::ETrigger::OnTakeDamage: return EAffixTrigger::OnTakeDamage;
	case PixelSideAffix::ETrigger::Passive: return EAffixTrigger::Passive;
	default: return EAffixTrigger::Passive;
	}
}

inline PixelSideAffix::EOfferSource ToLiteSource(EAffixOfferSource Source)
{
	switch (Source)
	{
	case EAffixOfferSource::EliteRoom: return PixelSideAffix::EOfferSource::EliteRoom;
	case EAffixOfferSource::PostBoss: return PixelSideAffix::EOfferSource::PostBoss;
	default: return PixelSideAffix::EOfferSource::NormalRoom;
	}
}

inline PixelSideAffix::ETrigger ToLiteTrigger(EAffixTrigger Trigger)
{
	switch (Trigger)
	{
	case EAffixTrigger::OnAcquire: return PixelSideAffix::ETrigger::OnAcquire;
	case EAffixTrigger::OnHit: return PixelSideAffix::ETrigger::OnHit;
	case EAffixTrigger::OnKill: return PixelSideAffix::ETrigger::OnKill;
	case EAffixTrigger::OnRoomClear: return PixelSideAffix::ETrigger::OnRoomClear;
	case EAffixTrigger::OnTakeDamage: return PixelSideAffix::ETrigger::OnTakeDamage;
	default: return PixelSideAffix::ETrigger::Passive;
	}
}

inline std::string ToUtf8(const FString& Text)
{
	const FTCHARToUTF8 Convert(*Text);
	return std::string(Convert.Get(), static_cast<size_t>(FMath::Max(0, Convert.Length())));
}

inline FString FromUtf8(std::string_view Text)
{
	const FUTF8ToTCHAR Convert(Text.data(), static_cast<int32>(Text.size()));
	return FString(Convert.Length(), Convert.Get());
}

inline FName IdName(std::string_view Id)
{
	return FName(FromUtf8(Id));
}

inline void ApplyRow(UAffixDefinition* Dst, const PixelSideAffix::FAffixRow& Row)
{
	if (!Dst)
	{
		return;
	}
	Dst->AffixId = IdName(Row.AffixId);
	Dst->DisplayName = FText::FromString(FromUtf8(Row.DisplayName));
	Dst->Description = FText::FromString(FromUtf8(Row.Description));
	Dst->Rarity = ToRarity(Row.Rarity);
	Dst->StackRule = ToStack(Row.StackRule);
	Dst->MaxStacks = Row.MaxStacks;
	Dst->Weight = Row.Weight;
	Dst->Tags.Reset();
	for (const std::string& Tag : Row.Tags)
	{
		Dst->Tags.Add(IdName(Tag));
	}
	Dst->ExclusiveTags.Reset();
	for (const std::string& Tag : Row.ExclusiveTags)
	{
		Dst->ExclusiveTags.Add(IdName(Tag));
	}
	Dst->Triggers.Reset();
	for (PixelSideAffix::ETrigger Trigger : Row.Triggers)
	{
		Dst->Triggers.Add(ToTrigger(Trigger));
	}
	Dst->Modifiers.Reset();
	for (const PixelSideAffix::FMod& Mod : Row.Modifiers)
	{
		FAffixModifier Out;
		Out.Stat = ToStatId(Mod.Stat);
		Out.Op = ToOp(Mod.Op);
		Out.Value = Mod.Value;
		Out.DurationSec = Mod.DurationSec;
		if (!Mod.WeaponTag.empty())
		{
			Out.WeaponTag = IdName(Mod.WeaponTag);
		}
		Dst->Modifiers.Add(Out);
	}
	Dst->bRequiresWeaponTag = NAME_None;
	Dst->GrantWeapon.Reset();
	Dst->Icon.Reset();
	if (!Row.Icon.empty())
	{
		Dst->Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(FromUtf8(Row.Icon)));
	}
}

inline void CheckStatOrdinals()
{
	static_assert(static_cast<uint8>(EStatId::MaxHP) == static_cast<uint8>(PixelSideAffix::EStat::MaxHP), "MaxHP");
	static_assert(static_cast<uint8>(EStatId::MoveSpeed) == static_cast<uint8>(PixelSideAffix::EStat::MoveSpeed), "MoveSpeed");
	static_assert(static_cast<uint8>(EStatId::FireRate) == static_cast<uint8>(PixelSideAffix::EStat::FireRate), "FireRate");
	static_assert(static_cast<uint8>(EStatId::Damage) == static_cast<uint8>(PixelSideAffix::EStat::Damage), "Damage");
	static_assert(static_cast<uint8>(EStatId::ScrapGain) == static_cast<uint8>(PixelSideAffix::EStat::ScrapGain), "ScrapGain");
	static_assert(static_cast<uint8>(EAffixOfferSource::NormalRoom) == static_cast<uint8>(PixelSideAffix::EOfferSource::NormalRoom), "NormalRoom");
	static_assert(static_cast<uint8>(EAffixOfferSource::PostBoss) == static_cast<uint8>(PixelSideAffix::EOfferSource::PostBoss), "PostBoss");
}

}
