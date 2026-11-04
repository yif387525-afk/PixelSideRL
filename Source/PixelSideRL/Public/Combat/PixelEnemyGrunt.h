#pragma once

#include "CoreMinimal.h"
#include "PaperCharacter.h"
#include "Combat/Damageable.h"
#include "PixelEnemyGrunt.generated.h"

class UDamageableComponent;
class UPaperFlipbook;
class UStaticMeshComponent;

/**
 * BP_AI_Grunt. CombatFeel §7 P0.5: aggro 280, chase at 140, horizontal shot 0.8 s / damage 6.
 * Not the lab debug turret (7.5 / 1.0 s).
 * Capsule object type stays Pawn so peas overlap it. Response to Pawn is Ignore,
 * so a chase cannot soft-lock the route the way D1 did.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "BP_AI_Grunt"))
class PIXELSIDERL_API APixelEnemyGrunt : public APaperCharacter, public IDamageable
{
	GENERATED_BODY()

public:
	APixelEnemyGrunt(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual bool ApplyDamage_Implementation(const FDamageSpec& Spec) override;
	virtual void GetHealth_Implementation(float& CurrentHealth, float& MaxHealth) override;
	virtual bool IsAlive_Implementation() override;
	virtual ETeamId GetTeamId_Implementation() override;

	UFUNCTION(BlueprintPure, Category = "PixelSide|Grunt")
	bool IsAlert() const { return bAlert; }

	UFUNCTION(BlueprintPure, Category = "PixelSide|Grunt")
	float GetFacingSign() const { return FacingSign; }

	/** Spawn facing from the room CSV. Left is -1 (toward the entrance). Aggro still turns the body when it chases. */
	UFUNCTION(BlueprintCallable, Category = "PixelSide|Grunt")
	void SetInitialFacingSign(float Sign);

	UFUNCTION(BlueprintPure, Category = "Damageable")
	UDamageableComponent* GetDamageable() const { return Damageable; }

protected:
	void EnsureFlipbooks();
	void UpdateCombat(float DeltaSeconds);
	void UpdateFacingVisual(float DeltaSeconds);
	void UpdateAnimation();
	void FireHorizontal();
	bool IsLiving() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damageable")
	TObjectPtr<UDamageableComponent> Damageable = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PixelSide|Grunt")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh = nullptr;

	/** Assign on a Blueprint to skip the runtime PNG slice. Empty uses Content/Art/Enemies/EnemyGrunt. */
	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Anim")
	TObjectPtr<UPaperFlipbook> IdleFlipbook = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Anim")
	TObjectPtr<UPaperFlipbook> RunFlipbook = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Anim")
	TObjectPtr<UPaperFlipbook> FireFlipbook = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Anim")
	TObjectPtr<UPaperFlipbook> HurtFlipbook = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "PixelSide|Anim")
	float ArtFacingMultiplier = 1.f;

	float FacingSign = -1.f;
	float TimeUntilFire = 0.f;
	float FireAnimRemaining = 0.f;
	float RecoilRemaining = 0.f;
	bool bAlert = false;
	FVector BaseSpriteRelativeLocation = FVector::ZeroVector;
	FVector BasePlaceholderRelativeLocation = FVector::ZeroVector;
};
