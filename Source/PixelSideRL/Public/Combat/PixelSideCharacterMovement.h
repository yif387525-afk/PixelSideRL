#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PixelSideCharacterMovement.generated.h"

/** Side-view movement: documented accel/stop, fall gravity, and a short knockback slide. */
UCLASS()
class PIXELSIDERL_API UPixelSideCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UPixelSideCharacterMovement();

	/** Horizontal ease-out. VelocityX is the initial speed (uu/s), signed. */
	void StartKnockback(float VelocityX, float Duration);

	/**
	 * Variable jump height. Scales rising Z (and a LaunchCharacter impulse that
	 * has not been consumed yet) by JumpCutRisingZScale. StopJumping does not.
	 */
	void ApplyJumpCut();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void PhysFalling(float deltaTime, int32 Iterations) override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Move")
	float FallGravityMultiplier = 1.55f;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Move")
	float MaxFallSpeed = 1000.f;

private:
	void ApplyAuthoringDefaults();

	float BaseGravityScale = 1.f;
	float KnockbackInitialX = 0.f;
	float KnockbackDuration = 0.f;
	float KnockbackTimeLeft = 0.f;
	float PendingKnockbackX = 0.f;
	bool bKnockbackActive = false;
};
