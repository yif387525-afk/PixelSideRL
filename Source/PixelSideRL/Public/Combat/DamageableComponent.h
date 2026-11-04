#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Damageable.h"
#include "DamageableComponent.generated.h"

class UPaperFlipbookComponent;
class UMaterialInstanceDynamic;
class UPixelSideCharacterMovement;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPixelHealthChanged, float, CurrentHealth, float, MaxHealth);

/**
 * AC_Damageable. Health, team, hit flash, knockback, victim hit-stop, player i-frames.
 */
UCLASS(ClassGroup = (PixelSide), meta = (BlueprintSpawnableComponent, DisplayName = "AC_Damageable"))
class PIXELSIDERL_API UDamageableComponent : public UActorComponent, public IDamageable
{
	GENERATED_BODY()

public:
	UDamageableComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual bool ApplyDamage_Implementation(const FDamageSpec& Spec) override;
	virtual void GetHealth_Implementation(float& CurrentHealth, float& MaxHealth) override;
	virtual bool IsAlive_Implementation() override;
	virtual ETeamId GetTeamId_Implementation() override;

	UFUNCTION(BlueprintPure, Category = "Damageable")
	float GetCurrentHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Damageable")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Damageable")
	float GetIFrameRemaining() const { return IFrameRemaining; }

	UFUNCTION(BlueprintPure, Category = "Damageable")
	float GetLastDamageTaken() const { return LastDamageTaken; }

	UFUNCTION(BlueprintPure, Category = "Damageable")
	float GetLastDamageWorldTime() const { return LastDamageWorldTime; }

	UFUNCTION(BlueprintPure, Category = "Damageable")
	ETeamId GetTeam() const { return Team; }

	UFUNCTION(BlueprintPure, Category = "Damageable")
	bool IsBlinkHidden() const;

	void SetMaxHealth(float NewMax, bool bKeepRatio);

	static ETeamId ResolveActorTeam(const AActor* Actor);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damageable")
	ETeamId Team = ETeamId::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damageable", meta = (ClampMin = "1.0"))
	float MaxHealth = 1.f;

	/** Player receivers use CombatFeel §6 instead of the incoming knockback/hit-stop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damageable")
	bool bUsePlayerHurtProfile = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damageable")
	bool bAcceptsDamage = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damageable|Hurt")
	float IFrameDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damageable|Hurt")
	float PlayerKnockback = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damageable|Hurt")
	float PlayerHitStopDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damageable|Hurt")
	float HitFlashTime = 0.f;

	/** Rest color for tagged HitFlash meshes after the white flash. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damageable|Hurt")
	FLinearColor MeshRestColor = FLinearColor::White;

	UPROPERTY(BlueprintAssignable, Category = "Damageable")
	FOnPixelHealthChanged OnHealthChanged;

protected:
	void ApplyKnockback(float SignedDistance);
	void BeginHitStop(float Seconds);
	void EndHitStop();
	void StartFlash();
	void RestoreFlash();
	void CacheFlashTargets();
	bool IsOwnerGrounded() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damageable")
	float Health = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damageable")
	float IFrameRemaining = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damageable")
	float LastDamageTaken = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damageable")
	float LastDamageWorldTime = -1.f;

	bool bDead = false;
	float FlashRemaining = 0.f;
	FLinearColor BaseSpriteColor = FLinearColor::White;
	TWeakObjectPtr<UPaperFlipbookComponent> SpriteVisual;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> FlashMids;
	FTimerHandle HitStopTimer;
	float SavedTimeDilation = 1.f;
};
