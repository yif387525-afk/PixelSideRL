#pragma once

#include "CoreMinimal.h"
#include "PaperCharacter.h"
#include "InputActionValue.h"
#include "Affix/AffixSubsystem.h"
#include "Combat/Damageable.h"
#include "PixelSideCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UPaperFlipbook;
class UStaticMeshComponent;
class UCameraComponent;
class USceneComponent;
class UWeaponControllerComponent;
class UDamageableComponent;
class UStatModifiersComponent;

/**
 * Side-view player. Parent BP_PlayerSideScroller to this class.
 * Jump flipbook is scrubbed across 5 v3 frames (rise / apex / fall).
 */
UCLASS(Blueprintable, BlueprintType)
class PIXELSIDERL_API APixelSideCharacter : public APaperCharacter, public IDamageable
{
	GENERATED_BODY()

public:
	APixelSideCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void PossessedBy(AController* NewController) override;

	virtual bool ApplyDamage_Implementation(const FDamageSpec& Spec) override;
	virtual void GetHealth_Implementation(float& CurrentHealth, float& MaxHealth) override;
	virtual bool IsAlive_Implementation() override;
	virtual ETeamId GetTeamId_Implementation() override;

	UFUNCTION(BlueprintPure, Category = "PixelSide")
	float GetFacingSign() const { return FacingSign; }

	UFUNCTION(BlueprintPure, Category = "PixelSide")
	UWeaponControllerComponent* GetWeaponController() const { return WeaponController; }

	UFUNCTION(BlueprintPure, Category = "PixelSide")
	UDamageableComponent* GetDamageable() const { return Damageable; }

	UFUNCTION(BlueprintPure, Category = "PixelSide")
	UStatModifiersComponent* GetStatModifiers() const { return Stats; }

protected:
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;

	void EnsureCombatInput();
	void AddMappingContext();
	void CacheStatBases();
	void RefreshFromStats();

	UFUNCTION()
	void HandleMove(const FInputActionValue& Value);

	UFUNCTION()
	void HandleJumpStarted(const FInputActionValue& Value);

	UFUNCTION()
	void HandleJumpCompleted(const FInputActionValue& Value);

	UFUNCTION()
	void HandleFireStarted(const FInputActionValue& Value);

	UFUNCTION()
	void HandleFireCompleted(const FInputActionValue& Value);

	UFUNCTION()
	void HandleDebugHurt(const FInputActionValue& Value);

	UFUNCTION()
	void HandleAffixChanged(FAffixChangeEvent Event);

	UFUNCTION()
	void HandleStatsRebuilt();

	UFUNCTION()
	void HandleWeaponFired();

	void UpdateCoyoteAndJump(float DeltaSeconds);
	void UpdateFacingVisual(float DeltaSeconds);
	void UpdateAnimation();
	void UpdateCamera();
	void WarnFlipbookOnce(const TCHAR* Label, const UPaperFlipbook* Flipbook, int32 ExpectedFrames, bool& bAlreadyWarned) const;
	bool CanRequestFeelJump() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide")
	TObjectPtr<UWeaponControllerComponent> WeaponController = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide")
	TObjectPtr<UDamageableComponent> Damageable = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide")
	TObjectPtr<UStatModifiersComponent> Stats = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Camera")
	TObjectPtr<USceneComponent> CameraAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Camera")
	TObjectPtr<UCameraComponent> SideCamera = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Move", meta = (ClampMin = "0.0", Units = "s"))
	float CoyoteTime = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Move", meta = (ClampMin = "0.0", Units = "s"))
	float JumpBuffer = 0.f;

	/** Multiply sprite facing if the imported sheet faces the wrong way. */
	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Anim")
	float ArtFacingMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Anim")
	TObjectPtr<UPaperFlipbook> IdleFlipbook = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Anim")
	TObjectPtr<UPaperFlipbook> RunFlipbook = nullptr;

	/** Player v3 sheet: exactly 5 frames, 32x40, rise / apex / fall. Not the legacy 4-frame clip. */
	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Anim")
	TObjectPtr<UPaperFlipbook> JumpFlipbook = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Anim")
	TObjectPtr<UPaperFlipbook> FireFlipbook = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Anim")
	TObjectPtr<UPaperFlipbook> HurtFlipbook = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> FireAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DebugHurtAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> CombatMappingContext = nullptr;

	float FacingSign = 1.f;
	float CoyoteTimeRemaining = 0.f;
	float JumpBufferRemaining = 0.f;
	float RecoilRemaining = 0.f;
	bool bCoyoteArmed = false;
	bool bJumpHeld = false;
	bool bJumpConsumed = false;
	/** Grounded frames to wait after a jump impulse so coyote does not re-arm before the pawn leaves the floor. */
	int32 JumpLockFrames = 0;
	bool bStatsBound = false;
	FVector BaseSpriteRelativeLocation = FVector::ZeroVector;
	FVector BasePlaceholderRelativeLocation = FVector::ZeroVector;

	mutable bool bWarnedJumpFrames = false;
	mutable bool bWarnedIdleFrames = false;
	mutable bool bWarnedRunFrames = false;
	mutable bool bWarnedFireFrames = false;
	mutable bool bWarnedHurtFrames = false;
};
