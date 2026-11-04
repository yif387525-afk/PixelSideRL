// Host-side check for combat P0 formulas. No Unreal headers.
#include "Combat/PixelSideCombatMath.h"

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
	Expect(Near(KnockbackInitialSpeed(40.f, KnockbackSlideDuration), 80.f / KnockbackSlideDuration), "ease-out speed");

	Expect(Near(HitFlashDuration, 2.f / PixelAnimFps), "hit flash is 2 frames at 12 fps");
	Expect(PeaFireInterval == 0.14f && PeaDamage == 8.f && PeaProjectileSpeed == 520.f, "peashooter §4.1");
	Expect(MaxWalkSpeed == 220.f && CoyoteTime == 0.08f && JumpBuffer == 0.10f, "move §2");
	Expect(PlayerIFrameDuration == 0.60f && PlayerHitKnockback == 120.f, "player hurt §6");

	if (gFails != 0)
	{
		std::printf("%d failed\n", gFails);
		return 1;
	}
	std::printf("ok\n");
	return 0;
}
