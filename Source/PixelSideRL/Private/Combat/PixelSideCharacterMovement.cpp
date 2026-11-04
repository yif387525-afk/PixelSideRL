#include "Combat/PixelSideCharacterMovement.h"
#include "Combat/PixelSideCombatMath.h"

UPixelSideCharacterMovement::UPixelSideCharacterMovement()
{
	ApplyAuthoringDefaults();
}

void UPixelSideCharacterMovement::ApplyAuthoringDefaults()
{
	MaxWalkSpeed = PixelSideCombat::MaxWalkSpeed;
	MaxAcceleration = PixelSideCombat::GroundAcceleration;
	BrakingDecelerationWalking = PixelSideCombat::GroundDeceleration;
	// Friction is folded into GroundDeceleration so the §2 number is the stop curve.
	GroundFriction = 0.f;
	BrakingFrictionFactor = 1.f;
	JumpZVelocity = PixelSideCombat::JumpZVelocity;
	AirControl = PixelSideCombat::AirControl;
	GravityScale = PixelSideCombat::GravityScale;
	BaseGravityScale = PixelSideCombat::GravityScale;
	FallGravityMultiplier = PixelSideCombat::FallGravityMultiplier;
	MaxFallSpeed = PixelSideCombat::MaxFallSpeed;
	MaxStepHeight = PixelSideCombat::MaxStepHeight;
	bConstrainToPlane = true;
	PlaneConstraintNormal = FVector::YAxisVector;
	bSnapToPlaneAtStart = true;
	bOrientRotationToMovement = false;
	bUseControllerDesiredRotation = false;
	NavAgentProps.bCanCrouch = false;
}

void UPixelSideCharacterMovement::BeginPlay()
{
	Super::BeginPlay();
	BaseGravityScale = GravityScale;
}

void UPixelSideCharacterMovement::ApplyJumpCut()
{
	// LaunchCharacter only queues PendingLaunchVelocity until the next movement
	// tick. StopJumping clears bPressedJump and never sees that rising impulse,
	// so a tap would still leave the ground at full JumpZVelocity.
	PendingLaunchVelocity.Z = PixelSideCombat::ApplyJumpCutRisingZ(PendingLaunchVelocity.Z);
	Velocity.Z = PixelSideCombat::ApplyJumpCutRisingZ(Velocity.Z);
}

void UPixelSideCharacterMovement::ClearKnockback()
{
	bKnockbackActive = false;
	KnockbackTimeLeft = 0.f;
	KnockbackDuration = 0.f;
	KnockbackInitialX = 0.f;
	PendingKnockbackX = 0.f;
}

void UPixelSideCharacterMovement::StartKnockback(float VelocityX, float Duration)
{
	KnockbackInitialX = VelocityX;
	KnockbackDuration = FMath::Max(Duration, 0.01f);
	KnockbackTimeLeft = KnockbackDuration;
	PendingKnockbackX = VelocityX;
	bKnockbackActive = true;
	Velocity.X = VelocityX;
	Velocity.Y = 0.f;
}

void UPixelSideCharacterMovement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	bKnockbackActive = KnockbackTimeLeft > 0.f;
	if (bKnockbackActive)
	{
		const float Alpha = FMath::Clamp(KnockbackTimeLeft / FMath::Max(KnockbackDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
		PendingKnockbackX = KnockbackInitialX * Alpha;
		Velocity.X = PendingKnockbackX;
		KnockbackTimeLeft = FMath::Max(0.f, KnockbackTimeLeft - DeltaTime);
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPixelSideCharacterMovement::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	Velocity.Y = 0.f;
	if (bKnockbackActive)
	{
		Velocity.X = PendingKnockbackX;
	}
}

void UPixelSideCharacterMovement::PhysFalling(float deltaTime, int32 Iterations)
{
	GravityScale = (Velocity.Z < 0.f) ? (BaseGravityScale * FallGravityMultiplier) : BaseGravityScale;
	Super::PhysFalling(deltaTime, Iterations);
	if (Velocity.Z < -MaxFallSpeed)
	{
		Velocity.Z = -MaxFallSpeed;
	}
	GravityScale = BaseGravityScale;
}
