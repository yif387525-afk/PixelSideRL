// Copyright PixelSideRL. Affix / modifier contracts for Buff pipeline.
// UE module not yet generated; keep as source-of-truth for BP/C++ parity.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Affix/AffixTypes.generated.h"

UENUM(BlueprintType)
enum class EAffixRarity : uint8
{
	Common UMETA(DisplayName = "Common"),
	Rare UMETA(DisplayName = "Rare"),
	Epic UMETA(DisplayName = "Epic"),
	Legendary UMETA(DisplayName = "Legendary")
};

UENUM(BlueprintType)
enum class EAffixStackRule : uint8
{
	Unique UMETA(DisplayName = "Unique"),
	AdditiveStack UMETA(DisplayName = "AdditiveStack"),
	RefreshDuration UMETA(DisplayName = "RefreshDuration")
};

UENUM(BlueprintType)
enum class EModifierOp : uint8
{
	Add UMETA(DisplayName = "Add"),
	Mul UMETA(DisplayName = "Mul"),
	Override UMETA(DisplayName = "Override")
};

UENUM(BlueprintType)
enum class EStatId : uint8
{
	MaxHP,
	MoveSpeed,
	FireRate,
	Damage,
	CritChance,
	CritMult,
	MagSize,
	ReloadSpeed,
	ProjectileCount,
	Pierce,
	Knockback,
	PickupRadius,
	CoinGain,   // legacy alias; prefer ScrapGain for economy
	ScrapGain
};

UENUM(BlueprintType)
enum class EAffixTrigger : uint8
{
	OnAcquire,
	OnHit,
	OnKill,
	OnRoomClear,
	OnTakeDamage,
	Passive
};

USTRUCT(BlueprintType)
struct PIXELSIDERL_API FAffixModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EStatId Stat = EStatId::Damage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EModifierOp Op = EModifierOp::Add;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Value = 0.f;

	/** <= 0: permanent for the run (or meta). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DurationSec = 0.f;
};
