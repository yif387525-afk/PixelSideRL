#pragma once

#include "CoreMinimal.h"
#include "PaperCharacter.h"
#include "InputActionValue.h"
#include "Affix/AffixChoicePresenterComponent.h"
#include "UI/PixelHUDPresenterComponent.h"
#include "Affix/AffixInventoryComponent.h"
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
class UCombatDebugReadComponent;
class UPixelDebugCombatHUD;

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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

	/** Dev overlay switch. Shipping builds force this off. Formal WBP_HUD must not reference the debug widget. */
	UFUNCTION(BlueprintPure, Category = "PixelSide|Debug")
	bool IsCombatDebugHUDEnabled() const { return bEnableCombatDebugHUD; }

protected:
	virtual bool CanJumpInternal_Implementation() const override;
	virtual void OnJumped_Implementation() override;

	void EnsureCombatInput();
	void EnsureStatModifiers();
	void AddMappingContext();
	void MountFormalHUD();
	void MountCombatDebugHUD();
	void CacheStatBases();
	void RefreshFromStats();
	void BeginAffixRun();
	bool IsAffixChoiceBlockingControl() const;

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
	void HandleDebugGrantRapidFire(const FInputActionValue& Value);

	UFUNCTION()
	void HandleDebugAffixChoice(const FInputActionValue& Value);

	UFUNCTION()
	void HandleDebugEventCarefulLoot(const FInputActionValue& Value);

	UFUNCTION()
	void HandleDebugEventForceLoot(const FInputActionValue& Value);

	UFUNCTION()
	void HandleAffixChoiceOffered(const TArray<FAffixChoiceEntry>& Choices);

	UFUNCTION()
	void HandleAffixChoiceClosed(bool bConfirmed);

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
	TObjectPtr<UAffixInventoryComponent> AffixInventory = nullptr;

	/** Choice mount. Native text UI is off; AffixChoiceWidgetClass is the WBP_AffixChoice parent. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide")
	TObjectPtr<UAffixChoicePresenterComponent> AffixChoicePresenter = nullptr;

	/** In-run WBP_HUD. Does not pause and does not touch the choice presenter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide")
	TObjectPtr<UPixelHUDPresenterComponent> HUDPresenter = nullptr;

	/** Read-only BI_CombatDebugRead facade. No tick, no damage, no fire. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Debug")
	TObjectPtr<UCombatDebugReadComponent> CombatDebugRead = nullptr;

	/** Non-shipping Lab overlay. Off in shipping. Not referenced by WBP_HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Debug")
	bool bEnableCombatDebugHUD = true;

	/** Child of UPixelDebugCombatHUD. Null uses the C++ widget, so no uasset is required. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Debug", meta = (EditCondition = "bEnableCombatDebugHUD"))
	TSubclassOf<UPixelDebugCombatHUD> CombatDebugHUDClass;

	UPROPERTY(Transient)
	TObjectPtr<UPixelDebugCombatHUD> CombatDebugHUD = nullptr;

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
	TObjectPtr<UInputAction> DebugGrantRapidFireAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DebugAffixChoiceAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DebugEventCarefulLootAction = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DebugEventForceLootAction = nullptr;

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
	bool bAffixChoiceBound = false;
	FVector BaseSpriteRelativeLocation = FVector::ZeroVector;
	FVector BasePlaceholderRelativeLocation = FVector::ZeroVector;

	mutable bool bWarnedJumpFrames = false;
	mutable bool bWarnedIdleFrames = false;
	mutable bool bWarnedRunFrames = false;
	mutable bool bWarnedFireFrames = false;
	mutable bool bWarnedHurtFrames = false;
};
