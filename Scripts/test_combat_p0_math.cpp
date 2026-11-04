// Host-side check for combat P0 formulas. No Unreal headers.
#include "Combat/PixelSideCombatMath.h"
#include "Combat/PixelSideLabSandboxLayout.h"

#include <cstdio>

namespace
{
int gFails = 0;

void Expect(bool Cond, const char* Msg)
{
	if (!Cond)
	{
		std::printf("FAIL %s\n", Msg);
		++gFails;
	}
}

bool Near(float A, float B)
{
	const float D = (A > B) ? (A - B) : (B - A);
	return D < 0.0001f;
}
}

int main()
{
	using namespace PixelSideCombat;

	Expect(JumpFrameCount == 5, "jump sheet is 5 frames");
	Expect(JumpRiseEarlyFrame == 0 && JumpApexFrame == 2 && JumpFallLateFrame == 4, "rise/apex/fall indices");
	Expect(SelectJumpFrameIndex(400.f, JumpZVelocity, MaxFallSpeed, JumpApexSpeed) == 0, "rise early");
	Expect(SelectJumpFrameIndex(100.f, JumpZVelocity, MaxFallSpeed, JumpApexSpeed) == 1, "rise late");
	Expect(SelectJumpFrameIndex(0.f, JumpZVelocity, MaxFallSpeed, JumpApexSpeed) == 2, "apex");
	Expect(SelectJumpFrameIndex(-10.f, JumpZVelocity, MaxFallSpeed, JumpApexSpeed) == 2, "apex band");
	Expect(SelectJumpFrameIndex(-100.f, JumpZVelocity, MaxFallSpeed, JumpApexSpeed) == 3, "fall early");
	Expect(SelectJumpFrameIndex(-800.f, JumpZVelocity, MaxFallSpeed, JumpApexSpeed) == 4, "fall late");

	Expect(FireRateBase == 1.f, "FireRate base is 1");
	Expect(Near(ComputeFireCooldown(PeaFireInterval, FireRateBase), PeaFireInterval), "cooldown at FireRate 1");
	const float Rapid = ComputeFireCooldown(PeaFireInterval, 1.12f);
	Expect(Rapid < PeaFireInterval && Near(Rapid, PeaFireInterval / 1.12f), "RapidFire mul 0.12 shortens interval");
	Expect(ComputeFireCooldown(PeaFireInterval, 0.f) > 1.f, "zero FireRate does not divide by zero");
	Expect(Near(RescaleInFlightCooldown(PeaFireInterval, PeaFireInterval, Rapid), Rapid), "in-flight cooldown tracks FireRate 1.12");
	Expect(Near(RescaleInFlightCooldown(Rapid, Rapid, Rapid), Rapid), "second rebuild does not scale twice");
	Expect(Near(RescaleInFlightCooldown(0.f, PeaFireInterval, Rapid), 0.f), "idle cooldown stays idle");

	Expect(Near(KnockbackDistance(PeaKnockback, false), PeaKnockback), "ground knockback");
	Expect(Near(KnockbackDistance(PlayerHitKnockback, true), PlayerHitKnockback * KnockbackAirRetain), "air keeps 60%");
	Expect(Near(KnockbackInitialSpeed(70.f, KnockbackSlideDuration), 140.f / KnockbackSlideDuration), "ease-out speed");

	Expect(Near(ApplyJumpCutRisingZ(400.f), 140.f), "jump cut scales rising Z");
	Expect(Near(ApplyJumpCutRisingZ(0.f), 0.f), "jump cut ignores apex");
	Expect(Near(ApplyJumpCutRisingZ(-80.f), -80.f), "jump cut ignores falling");

	Expect(Near(HitFlashDuration, 2.f / PixelAnimFps), "hit flash is 2 frames at 12 fps");
	Expect(PeaFireInterval == 0.12f && PeaDamage == 8.f && PeaProjectileSpeed == 560.f, "peashooter §4.1");
	Expect(PeaSpreadDegrees == 1.f && PeaKnockback == 70.f && PeaHitStopEnemy == 0.08f && PeaCamShakeScale == 1.1f, "peashooter hit feel");
	Expect(MaxWalkSpeed == 245.f && GroundDeceleration == 4000.f && AirControl == 0.70f, "move §2");
	Expect(JumpZVelocity == 400.f && JumpCutRisingZScale == 0.35f && GravityScale == 2.55f, "jump and gravity");
	Expect(CoyoteTime == 0.08f && JumpBuffer == 0.10f, "coyote and buffer unchanged");
	Expect(Near(GruntDamage, 7.5f) && GruntDamage <= PlayerMaxHealth * 0.25f, "lab turret shot is 25% max HP");
	Expect(GruntFireInterval >= 1.f && Near(GruntFireInterval, 1.f), "lab turret interval is 1s");
	Expect(Near(GruntProjectileSpeed, 280.f), "lab turret bolt speed unchanged");
	Expect(EnemyGruntMoveSpeed == 140.f && EnemyGruntAggroRange == 280.f && EnemyGruntLoseAggroRange == 360.f, "grunt move and aggro");
	Expect(Near(EnemyGruntFireInterval, 0.8f) && Near(EnemyGruntDamage, 6.f), "grunt shot is 0.8s / 6");
	Expect(EnemyGruntDamage != GruntDamage && EnemyGruntFireInterval != GruntFireInterval, "grunt row is not the turret row");
	Expect(Near(EnemyGruntProjectileSpeed, 320.f) && EnemyGruntProjectileSpeed != GruntProjectileSpeed, "grunt bolt is not the turret bolt");
	Expect(Near(EnemyGruntMuzzleLocalX(), 8.5f) && Near(EnemyGruntMuzzleLocalZ(), 18.5f), "grunt muzzle is the fire tip");
	Expect(!EnemyGruntWantsAlert(false, 281.f) && EnemyGruntWantsAlert(false, 280.f), "aggro edge is 280");
	Expect(EnemyGruntWantsAlert(true, 360.f) && !EnemyGruntWantsAlert(true, 361.f), "lose aggro edge is 360");
	Expect(EnemyGruntChaseSign(10.f, EnemyGruntChaseSlack) == 1.f && EnemyGruntChaseSign(-10.f, EnemyGruntChaseSlack) == -1.f, "chase sign");
	Expect(EnemyGruntChaseSign(2.f, EnemyGruntChaseSlack) == 0.f, "chase slack");
	Expect(EnemyGruntFrameWidth == 28 && EnemyGruntFrameHeight == 32 && EnemyGruntMuzzlePixelY == 13, "grunt sheet");
	Expect(PlayerIFrameDuration == 0.60f && PlayerHitKnockback == 145.f && PlayerHitStop == 0.07f, "player hurt §6");
	Expect(KnockbackSlideDuration == 0.11f, "knockback slide");

	using namespace PixelSideLabLayout;
	Expect(!bSpawnBridgeByDefault, "BRIDGE is not a resident slab");
	Expect(PitSideGap == 60 && IslandX0 - PitX0 == PitSideGap && PitX1 - IslandX1 == PitSideGap, "pit side gaps are 60");
	Expect((IslandX0 + IslandX1) == (PitX0 + PitX1), "island is centered in the pit");
	Expect(IslandX1 - IslandX0 == 80 && IslandX0 == 1400 && IslandX1 == 1480, "island width 80, X unchanged");
	Expect(IslandTopY == 24 && P1TopY == 28 && P2TopY == 56 && P3TopY == 84 && P_E_TopY == 56, "hotfix tops");
	Expect(P2TopY - P1TopY == 28 && P3TopY - P2TopY == 28, "platform rise is 28");
	Expect(P1TopY < 32 && IslandTopY < 32, "P1 and island sit under the live ~32 jump");
	Expect(JumpZVelocity == 400.f && GravityScale == 2.55f, "hotfix does not retune jump");
	Expect(G_A_X1 == PitX0 && G_C_Right_X0 == PitX1, "floors meet the pit lips");
	Expect(D2_FeetX == P2CenterX && D2_FeetY == P2TopY, "D2 stands on P2");
	Expect(D4_FeetX == P_E_CenterX && D4_FeetY == P_E_TopY, "D4 stands on P_E_high");
	Expect(SafeFeetX > G_A_X0 && SafeFeetX < PitX0 && SafeFeetX + 12 < PitCatchX0, "safe point is on G_A left of the pit");
	Expect(PitDeathY == -80 && PitCatchX0 < G_A_X1 && PitCatchX1 > PitX1, "pit catch includes the lip and edge");
	Expect(T1_FeetX >= 2000 && CV2_X + CV2_Width / 2 < T1_FeetX, "T1 sits behind CV2");
	Expect(CV1_Height == 44 && CV2_Height == 48 && CV3_Height == 40, "cover heights unchanged");
	Expect(CV1_Height > 32 && CV2_Height > 32 && CV3_Height > 32, "covers stay above the live jump");
	Expect(D1_FeetX > PlayerSpawnFeetX && D1_FeetY == G_A_TopY, "D1 stands on the opening route");
	Expect(G1_FeetX == 480 && G1_FeetY == G_A_TopY && G1_FeetX < D1_FeetX, "G1 stands on G_A before D1");
	Expect(G1_FeetX - PlayerSpawnFeetX > 360, "G1 starts outside lose-aggro");
	Expect(SlabDepth / 2 >= 15, "slab reaches the projectile plane");
	Expect(WallLeftX + WallThickness / 2 == G_A_X0, "left wall seals X=0");
	Expect(WallRightX - WallThickness / 2 == PlayWidth, "right wall seals X=3200");

	if (gFails != 0)
	{
		std::printf("%d failed\n", gFails);
		return 1;
	}
	std::printf("ok\n");
	return 0;
}
