#include "Combat/WeaponControllerComponent.h"

#include "Combat/DA_Weapon.h"
#include "Combat/DamageableComponent.h"
#include "Combat/PixelSideCombatLog.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/ProjectilePea.h"
#include "Combat/StatModifiersComponent.h"
#include "Engine/World.h"

UWeaponControllerComponent::UWeaponControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	FacingSign = 1.f;
}

void UWeaponControllerComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureWeaponData();
	PushBasesToStats();
}

void UWeaponControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	CooldownRemaining = FMath::Max(0.f, CooldownRemaining - DeltaTime);
	if (bWantsToFire)
	{
		TryFire();
	}
}

void UWeaponControllerComponent::SetWantsToFire(bool bNewWantsToFire)
{
	bWantsToFire = bNewWantsToFire;
	if (bWantsToFire)
	{
		TryFire();
	}
}

void UWeaponControllerComponent::SetFacingSign(float Sign)
{
	if (FMath::Abs(Sign) > KINDA_SMALL_NUMBER)
	{
		FacingSign = FMath::Sign(Sign);
	}
}

bool UWeaponControllerComponent::TryGetFireDir(FVector& OutDir) const
{
	OutDir = FVector(FacingSign, 0.f, 0.f);
	return GetOwner() != nullptr;
}

bool UWeaponControllerComponent::TryFire()
{
	if (CooldownRemaining > 0.f || !GetOwner() || !GetWorld())
	{
		return false;
	}

	EnsureWeaponData();
	if (!WeaponData)
	{
		return false;
	}

	const float FireRate = ResolveStat(EStatId::FireRate, PixelSideCombat::FireRateBase);
	const float Damage = ResolveStat(EStatId::Damage, WeaponData->Damage);
	const float Knockback = ResolveStat(EStatId::Knockback, WeaponData->KnockbackToEnemy);
	CooldownRemaining = PixelSideCombat::ComputeFireCooldown(WeaponData->FireInterval, FireRate);

	FVector FireDir;
	TryGetFireDir(FireDir);
	const float SpreadRad = FMath::DegreesToRadians(FMath::FRandRange(-WeaponData->Spread, WeaponData->Spread));
	const float Facing = FireDir.X >= 0.f ? 1.f : -1.f;
	const FVector ShotDir(Facing * FMath::Cos(SpreadRad), 0.f, FMath::Sin(SpreadRad));

	const TSubclassOf<AProjectilePea> ProjectileClass = WeaponData->ProjectileClass
		? WeaponData->ProjectileClass
		: TSubclassOf<AProjectilePea>(AProjectilePea::StaticClass());

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = Cast<APawn>(GetOwner());
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector Muzzle = MuzzleLocation();
	const FRotator Rot = ShotDir.Rotation();
	AProjectilePea* Pea = GetWorld()->SpawnActor<AProjectilePea>(ProjectileClass, Muzzle, Rot, Params);
	if (!Pea)
	{
		UE_LOG(LogPixelSideCombat, Warning, TEXT("Peashooter failed to spawn a projectile"));
		return false;
	}

	Pea->LaunchShot(
		ShotDir,
		WeaponData->ProjectileSpeed,
		WeaponData->ProjectileRadius,
		WeaponData->ProjectileLife,
		Damage,
		Knockback,
		WeaponData->HitStop_Enemy,
		WeaponData->HitStop_Player,
		GetOwner(),
		UDamageableComponent::ResolveActorTeam(GetOwner()));

	OnWeaponFired.Broadcast();
	return true;
}

float UWeaponControllerComponent::GetFireInterval() const
{
	return WeaponData ? WeaponData->FireInterval : PixelSideCombat::PeaFireInterval;
}

float UWeaponControllerComponent::GetResolvedFireCooldown() const
{
	const float Interval = GetFireInterval();
	const float Rate = ResolveStat(EStatId::FireRate, PixelSideCombat::FireRateBase);
	return PixelSideCombat::ComputeFireCooldown(Interval, Rate);
}

FName UWeaponControllerComponent::GetWeaponId() const
{
	return WeaponData ? WeaponData->WeaponId : FName(TEXT("Peashooter"));
}

void UWeaponControllerComponent::SetWeaponData(UDA_Weapon* NewWeapon)
{
	WeaponData = NewWeapon;
	EnsureWeaponData();
	PushBasesToStats();
}

void UWeaponControllerComponent::EnsureWeaponData()
{
	if (!WeaponData)
	{
		WeaponData = NewObject<UDA_Weapon_Peashooter>(this, TEXT("DA_Weapon_Peashooter_Runtime"));
	}
}

void UWeaponControllerComponent::PushBasesToStats()
{
	UStatModifiersComponent* Stats = FindStats();
	if (!Stats || !WeaponData)
	{
		return;
	}
	Stats->SetBaseStat(EStatId::Damage, WeaponData->Damage);
	Stats->SetBaseStat(EStatId::Knockback, WeaponData->KnockbackToEnemy);
	Stats->SetBaseStat(EStatId::FireRate, PixelSideCombat::FireRateBase);
}

UStatModifiersComponent* UWeaponControllerComponent::FindStats() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UStatModifiersComponent>() : nullptr;
}

float UWeaponControllerComponent::ResolveStat(EStatId StatId, float BaseValue) const
{
	if (const UStatModifiersComponent* Stats = FindStats())
	{
		return Stats->GetStat(StatId);
	}
	if (StatId == EStatId::FireRate)
	{
		return PixelSideCombat::FireRateBase;
	}
	return BaseValue;
}

FVector UWeaponControllerComponent::MuzzleLocation() const
{
	const FVector Origin = GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
	const float Sign = (FacingSign >= 0.f) ? 1.f : -1.f;
	return Origin + FVector(Sign * PixelSideCombat::MuzzleOffsetX, PixelSideCombat::VisualPlaneY, PixelSideCombat::MuzzleOffsetZ);
}
