#include "Combat/DA_Weapon.h"
#include "Combat/PixelSideCombatMath.h"

UDA_Weapon::UDA_Weapon()
{
	WeaponId = FName(TEXT("Peashooter"));
	FireInterval = PixelSideCombat::PeaFireInterval;
	Spread = PixelSideCombat::PeaSpreadDegrees;
	ProjectileSpeed = PixelSideCombat::PeaProjectileSpeed;
	ProjectileRadius = PixelSideCombat::PeaProjectileRadius;
	Damage = PixelSideCombat::PeaDamage;
	KnockbackToEnemy = PixelSideCombat::PeaKnockback;
	HitStop_Player = PixelSideCombat::PeaHitStopPlayer;
	HitStop_Enemy = PixelSideCombat::PeaHitStopEnemy;
	CamShakeScale = PixelSideCombat::PeaCamShakeScale;
	ProjectileLife = PixelSideCombat::PeaProjectileLife;
}

UDA_Weapon_Peashooter::UDA_Weapon_Peashooter()
{
	WeaponId = FName(TEXT("Peashooter"));
	FireInterval = PixelSideCombat::PeaFireInterval;
	Spread = PixelSideCombat::PeaSpreadDegrees;
	ProjectileSpeed = PixelSideCombat::PeaProjectileSpeed;
	ProjectileRadius = PixelSideCombat::PeaProjectileRadius;
	Damage = PixelSideCombat::PeaDamage;
	KnockbackToEnemy = PixelSideCombat::PeaKnockback;
	HitStop_Player = PixelSideCombat::PeaHitStopPlayer;
	HitStop_Enemy = PixelSideCombat::PeaHitStopEnemy;
	CamShakeScale = PixelSideCombat::PeaCamShakeScale;
	ProjectileLife = PixelSideCombat::PeaProjectileLife;
}
