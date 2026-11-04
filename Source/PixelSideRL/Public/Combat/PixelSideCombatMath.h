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

// Lab debug turret and the H debug-hurt key (APixelDebugHurtTurret / HandleDebugHurt).
// Supervisor rule: one shot ≤ 25% of PlayerMaxHealth, and the turret waits ≥ 1 s.
// This row is the turret only. P0.5 Grunt uses EnemyGrunt* below. Do not alias them.
// Speed is intentionally under the player's 560 so the bolt stays readable.
constexpr float GruntFireInterval = 1.0f;
constexpr float GruntDamage = 7.5f; // 25% of PlayerMaxHealth (30)
constexpr float GruntProjectileSpeed = 280.f;
constexpr float GruntProjectileLife = 3.5f;
constexpr float DebugTurretFirstShotDelay = 0.60f;

// CombatFeel §7 Grunt (P0.5). Not the lab turret row above.
constexpr float EnemyGruntMoveSpeed = 140.f;
constexpr float EnemyGruntAggroRange = 280.f;
constexpr float EnemyGruntLoseAggroRange = 360.f;
constexpr float EnemyGruntFireInterval = 0.8f;
constexpr float EnemyGruntDamage = 6.f;
// §7 does not list bolt speed. 320 is slower than the player pea (560) and faster
// than walk (245), so a jump clears it and holding run does not. Not the turret's 280.
constexpr float EnemyGruntProjectileSpeed = 320.f;
constexpr float EnemyGruntProjectileLife = 2.0f;
constexpr float EnemyGruntProjectileRadius = 3.f;
// Four peashooter hits (4 * 8). §7 does not list HP.
constexpr float EnemyGruntMaxHealth = 32.f;
// Capsule reaches the shot plane (VisualPlaneY 12 + pea radius 3) the way the player capsule does.
// Half-height matches the 32px frame. Half-height must stay ≥ radius.
constexpr float EnemyGruntCapsuleRadius = 12.f;
constexpr float EnemyGruntCapsuleHalfHeight = 16.f;
// Ignore tiny X error so the body does not flip on top of the player. Not an attack leash.
constexpr float EnemyGruntChaseSlack = 4.f;

// T_EnemyGrunt_* sheets. 28×32, horizontal strip, pivot is feet center. Fire tip row is Y=13.
constexpr int EnemyGruntFrameWidth = 28;
constexpr int EnemyGruntFrameHeight = 32;
constexpr int EnemyGruntIdleFrameCount = 4;
constexpr int EnemyGruntRunFrameCount = 6;
constexpr int EnemyGruntFireFrameCount = 3;
constexpr int EnemyGruntHurtFrameCount = 2;
// Resting fire frames (0 and 2). Recoil frame 1 moves the tip to x=20; the socket stays on the rest tip.
constexpr int EnemyGruntMuzzlePixelX = 22;
constexpr int EnemyGruntMuzzlePixelY = 13;

// Entry PlayerStart when the procedural lab is not built (shipping, or the
// level already has a damage dummy). The 5-zone sandbox lives in
// PixelSideLabSandboxLayout.h and is spawned by APixelSideGameMode.
constexpr float LabPlayerSpawnX = 0.f;
constexpr float LabPlayerSpawnZ = 120.f;

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

// Local offset from the feet-center pivot to the fire-sheet tip, in uu (1 px = 1 uu).
inline float EnemyGruntMuzzleLocalX()
{
	return (static_cast<float>(EnemyGruntMuzzlePixelX) + 0.5f) - (static_cast<float>(EnemyGruntFrameWidth) * 0.5f);
}

inline float EnemyGruntMuzzleLocalZ()
{
	return static_cast<float>(EnemyGruntFrameHeight) - (static_cast<float>(EnemyGruntMuzzlePixelY) + 0.5f);
}

// Horizontal distance. Lose range is wider so the body does not flicker on the aggro line.
inline bool EnemyGruntWantsAlert(bool bAlreadyAlert, float HorizontalDistance)
{
	if (HorizontalDistance < 0.f)
	{
		HorizontalDistance = -HorizontalDistance;
	}
	if (bAlreadyAlert)
	{
		return HorizontalDistance <= EnemyGruntLoseAggroRange;
	}
	return HorizontalDistance <= EnemyGruntAggroRange;
}

// +1 chase toward +X, -1 toward -X, 0 inside the slack.
inline float EnemyGruntChaseSign(float DeltaX, float Slack)
{
	if (DeltaX > Slack)
	{
		return 1.f;
	}
	if (DeltaX < -Slack)
	{
		return -1.f;
	}
	return 0.f;
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
static_assert(GruntDamage <= PlayerMaxHealth * 0.25f, "lab turret shot is at most 25% player max HP");
static_assert(GruntFireInterval >= 1.f, "lab turret interval is at least 1 s");
static_assert(GruntDamage == 7.5f, "lab turret damage stays 7.5");
static_assert(GruntFireInterval == 1.0f, "lab turret interval stays 1 s");
static_assert(EnemyGruntMoveSpeed == 140.f, "CombatFeel §7 grunt speed");
static_assert(EnemyGruntAggroRange == 280.f, "CombatFeel §7 aggro");
static_assert(EnemyGruntLoseAggroRange == 360.f, "CombatFeel §7 lose aggro");
static_assert(EnemyGruntFireInterval == 0.8f, "CombatFeel §7 grunt interval");
static_assert(EnemyGruntDamage == 6.f, "CombatFeel §7 grunt damage");
static_assert(EnemyGruntDamage != GruntDamage, "grunt damage is not the lab turret shot");
static_assert(EnemyGruntFireInterval < GruntFireInterval, "grunt interval is not the lab turret interval");
static_assert(EnemyGruntProjectileSpeed != GruntProjectileSpeed, "grunt bolt speed is not the turret bolt");
static_assert(EnemyGruntFrameWidth == 28 && EnemyGruntFrameHeight == 32, "EnemyGrunt sheets are 28x32");
static_assert(EnemyGruntIdleFrameCount == 4 && EnemyGruntRunFrameCount == 6, "grunt idle/run frame counts");
static_assert(EnemyGruntFireFrameCount == 3 && EnemyGruntHurtFrameCount == 2, "grunt fire/hurt frame counts");
static_assert(EnemyGruntMuzzlePixelY == 13, "fire tip row is Y=13");
static_assert(EnemyGruntCapsuleHalfHeight >= EnemyGruntCapsuleRadius, "grunt capsule half-height covers the radius");
}
