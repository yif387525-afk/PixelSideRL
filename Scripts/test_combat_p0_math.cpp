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

	Expect(Near(ComputeFireCooldown(PeaFireInterval, FireRateBase), PeaFireInterval), "cooldown at FireRate 1");
	const float Rapid = ComputeFireCooldown(PeaFireInterval, 1.12f);
	Expect(Rapid < PeaFireInterval && Near(Rapid, PeaFireInterval / 1.12f), "RapidFire mul 0.12 shortens interval");
	Expect(ComputeFireCooldown(PeaFireInterval, 0.f) > 1.f, "zero FireRate does not divide by zero");

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
	Expect(PlayerIFrameDuration == 0.60f && PlayerHitKnockback == 145.f && PlayerHitStop == 0.07f, "player hurt §6");
	Expect(KnockbackSlideDuration == 0.11f, "knockback slide");

	using namespace PixelSideLabLayout;
	Expect(!bSpawnBridgeByDefault, "BRIDGE is not a resident slab");
	Expect(PitX1 - PitX0 == 240, "PIT_C is 240 wide");
	Expect(IslandX0 > PitX0 && IslandX1 < PitX1 && IslandTopY == 40, "island sits in the pit");
	Expect(P2TopY - P1TopY == 48 && P3TopY - P2TopY == 48, "platform rise is 48");
	Expect(D2_FeetX == P2CenterX && D2_FeetY == P2TopY, "D2 stands on P2");
	Expect(D4_FeetX == P_E_CenterX && D4_FeetY == P_E_TopY, "D4 stands on P_E_high");
	Expect(SafeFeetX > G_A_X0 && SafeFeetX < G_A_X1, "safe point is on G_A");
	Expect(PitDeathY == -80 && PitCatchX0 < G_A_X1 && PitCatchX1 > PitX1, "pit catch includes the lip and edge");
	Expect(T1_FeetX >= 2000 && CV2_X + CV2_Width / 2 < T1_FeetX, "T1 sits behind CV2");
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
