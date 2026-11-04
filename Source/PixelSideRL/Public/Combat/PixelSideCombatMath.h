#pragma once

// Single default table for combat P0. Runtime reads component / DA properties
// seeded from these constants. Do not copy the literals into gameplay code.
// Feel source: Docs/CombatFeel.md. Jump frames: player v3 T_Player_Jump (5).

namespace PixelSideCombat
{
// --- Art / presentation (ArtBible) ---
constexpr int FrameWidth = 32;
constexpr int FrameHeight = 40;
constexpr int JumpFrameCount = 5;
constexpr int IdleFrameCount = 4;
constexpr int RunFrameCount = 6;
constexpr int FireFrameCount = 3;
constexpr int HurtFrameCount = 2;
constexpr float PixelAnimFps = 12.f;
constexpr float OrthoWidth = 640.f;
constexpr float OrthoHeight = 360.f;
constexpr float CameraArmLength = 800.f;
constexpr float FixedCameraZ = 150.f;
// Camera sits on +Y looking toward -Y. Sprites live on this plane, in front of the floor slab.
constexpr float VisualPlaneY = 12.f;

// T_Player_Jump v3 is five frames: rise, rise, apex, fall, fall. Never assume 4.
constexpr int JumpRiseEarlyFrame = 0;
constexpr int JumpRiseLateFrame = 1;
constexpr int JumpApexFrame = 2;
constexpr int JumpFallEarlyFrame = 3;
constexpr int JumpFallLateFrame = 4;
constexpr float JumpApexSpeed = 40.f;

// --- Move feel (CombatFeel §2). Dead Cells pass: snappier stop, stronger air redirect. ---
constexpr float MaxWalkSpeed = 245.f;
constexpr float GroundAcceleration = 3200.f;
constexpr float GroundDeceleration = 4000.f;
constexpr float JumpZVelocity = 400.f;
constexpr float AirControl = 0.70f;
constexpr float CoyoteTime = 0.08f;
constexpr float JumpBuffer = 0.10f;
constexpr float GravityScale = 2.55f;
constexpr float FallGravityMultiplier = 1.55f;
constexpr float MaxFallSpeed = 1000.f;
// Release jump while rising: Velocity.Z *= this. Held jumps keep JumpZVelocity.
constexpr float JumpCutRisingZScale = 0.35f;
constexpr float CapsuleRadius = 12.f;
constexpr float CapsuleHalfHeight = 19.f; // diameter 24, height 38
constexpr float SpriteFeetOffsetZ = -1.f;
constexpr float MaxStepHeight = 16.f; // one ArtBible floor tile
constexpr float FacingInputDeadzone = 0.1f;
constexpr float RunSpeedThreshold = 10.f;

// --- Muzzle (CombatFeel §3) ---
constexpr float MuzzleOffsetX = 10.f;
constexpr float MuzzleOffsetZ = 6.f;
constexpr float FireRecoilPixels = 2.f;
constexpr float FireRecoilDuration = 0.06f;

// --- Peashooter (CombatFeel §4.1) ---
constexpr float PeaFireInterval = 0.12f;
constexpr float PeaSpreadDegrees = 1.0f;
constexpr float PeaProjectileSpeed = 560.f;
constexpr float PeaProjectileRadius = 3.f;
constexpr float PeaDamage = 8.f;
constexpr float PeaKnockback = 70.f;
constexpr float PeaHitStopPlayer = 0.03f;
constexpr float PeaHitStopEnemy = 0.08f;
constexpr float PeaCamShakeScale = 1.1f;
constexpr float PeaProjectileLife = 0.9f;

// FireRate is a multiplier. Default base is 1. Cooldown = FireInterval / GetStat(FireRate).
constexpr float FireRateBase = 1.f;
constexpr float MinFireRate = 0.01f;

// --- Projectile pipeline (CombatFeel §5) ---
constexpr float ProjectileIgnoreOwnerSeconds = 0.05f;
constexpr float SameTargetHitInterval = 0.10f;

// --- Player hurt (CombatFeel §6) ---
constexpr float PlayerIFrameDuration = 0.60f;
constexpr float IFrameBlinkHz = 9.f; // inside the 8–10 Hz band
constexpr float PlayerHitKnockback = 145.f;
constexpr float PlayerHitStop = 0.07f;
constexpr int HitFlashFrames = 2;
constexpr float HitFlashDuration = static_cast<float>(HitFlashFrames) / PixelAnimFps;
constexpr float HitStopDilation = 0.05f;

// Knockback values are horizontal travel in uu (1 uu = 1 px). They are played as a
// short ease-out so GroundDeceleration (4000) does not erase a 70/145 shove in under a pixel.
// Air keeps 60% of that distance (CombatFeel §6).
constexpr float KnockbackSlideDuration = 0.11f;
constexpr float KnockbackAirRetain = 0.60f;

// Published player max in the debug HUD wireframe (HP x / 30). Not a CombatFeel row.
constexpr float PlayerMaxHealth = 30.f;
// Five peashooter hits (5 * 8).
constexpr float DamageDummyMaxHealth = 40.f;

// Debug turret stands in for Grunt's slow pea (CombatFeel §7: 0.8 s, damage 6).
// Speed is intentionally under the player's 560 so the bolt stays readable.
constexpr float GruntFireInterval = 0.80f;
constexpr float GruntDamage = 6.f;
constexpr float GruntProjectileSpeed = 280.f;
constexpr float GruntProjectileLife = 3.5f;
constexpr float DebugTurretFirstShotDelay = 0.60f;

// Greybox spawned when a map has no damage dummy (see APixelSideGameMode).
constexpr float LabFloorCenterX = 400.f;
constexpr float LabFloorScaleX = 24.f;
constexpr float LabFloorScaleY = 0.16f;
constexpr float LabFloorScaleZ = 0.5f;
constexpr float LabFloorCenterZ = -25.f;
constexpr float LabPlayerSpawnX = 0.f;
constexpr float LabPlayerSpawnZ = 120.f;
constexpr float LabDummyX = 420.f;
constexpr float LabDummyZ = 80.f;
constexpr float LabTurretX = -260.f;
constexpr float LabTurretZ = 18.f;
constexpr float LabWallScaleX = 0.5f;
constexpr float LabWallScaleY = 0.16f;
constexpr float LabWallScaleZ = 2.4f;
constexpr float LabWallCenterZ = 95.f;

inline float ComputeFireCooldown(float FireInterval, float FireRateMultiplier)
{
	const float Rate = (FireRateMultiplier > MinFireRate) ? FireRateMultiplier : MinFireRate;
	return FireInterval / Rate;
}

// Jump cut applies only on the way up. Apex and falling arcs stay intact.
inline float ApplyJumpCutRisingZ(float VerticalZ)
{
	return (VerticalZ > 0.f) ? (VerticalZ * JumpCutRisingZScale) : VerticalZ;
}

inline int SelectJumpFrameIndex(float VerticalVelocity, float InJumpZVelocity, float InMaxFallSpeed, float InApexSpeed)
{
	if (VerticalVelocity > InApexSpeed)
	{
		const float Denom = (InJumpZVelocity > 0.f) ? InJumpZVelocity : InApexSpeed;
		const float Alpha = VerticalVelocity / Denom;
		return (Alpha > 0.5f) ? JumpRiseEarlyFrame : JumpRiseLateFrame;
	}
	if (VerticalVelocity < -InApexSpeed)
	{
		const float Denom = (InMaxFallSpeed > 0.f) ? InMaxFallSpeed : InApexSpeed;
		const float Alpha = (-VerticalVelocity) / Denom;
		return (Alpha < 0.5f) ? JumpFallEarlyFrame : JumpFallLateFrame;
	}
	return JumpApexFrame;
}

inline float KnockbackDistance(float BaseDistance, bool bAirborne)
{
	return bAirborne ? (BaseDistance * KnockbackAirRetain) : BaseDistance;
}

inline float KnockbackInitialSpeed(float DistanceUu, float DurationSeconds)
{
	if (DurationSeconds <= 0.0001f)
	{
		return 0.f;
	}
	return (2.f * DistanceUu) / DurationSeconds;
}

static_assert(JumpFrameCount == 5, "player v3 T_Player_Jump is 5 frames");
static_assert(JumpFallLateFrame == JumpFrameCount - 1, "jump frames are 0..4");
static_assert(IdleFrameCount == 4, "T_Player_Idle is 4 frames");
static_assert(RunFrameCount == 6, "T_Player_Run is 6 frames");
static_assert(FireFrameCount == 3, "T_Player_Fire is 3 frames");
static_assert(HurtFrameCount == 2, "T_Player_Hurt is 2 frames");
}
