// Copyright PixelSideRL. Affix / modifier contracts for Buff pipeline.
// UE module not yet generated; keep as source-of-truth for BP/C++ parity.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AffixTypes.generated.h"

class UDA_Weapon;
class UTexture2D;

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

/**
 * DA_Affix_* row. Field names match SCHEMA_AffixDefinition.json (contractVersion 3).
 * Stub only — no uassets. 词条官 owns the real data.
 */
UCLASS(BlueprintType)
class PIXELSIDERL_API UAffixDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	FName AffixId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	EAffixRarity Rarity = EAffixRarity::Common;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	EAffixStackRule StackRule = EAffixStackRule::Unique;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix", meta = (ClampMin = "1"))
	int32 MaxStacks = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix", meta = (ClampMin = "0.0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	TArray<FName> Tags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	TArray<FName> ExclusiveTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	TArray<EAffixTrigger> Triggers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	TArray<FAffixModifier> Modifiers;

	/** Soft reference to a weapon data asset. Unset if this affix grants none. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	TSoftObjectPtr<UDA_Weapon> GrantWeapon;

	/** Contract name (not a bool). None if the affix is not limited to a weapon tag. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	FName bRequiresWeaponTag;
};
