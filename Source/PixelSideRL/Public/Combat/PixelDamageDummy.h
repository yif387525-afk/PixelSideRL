#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Combat/Damageable.h"
#include "Combat/PixelDamageDummy.generated.h"

class UDamageableComponent;
class UStaticMeshComponent;

/** BP_DamageDummy. Enemy-team post for the peashooter acceptance pass. */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "BP_DamageDummy"))
class PIXELSIDERL_API APixelDamageDummy : public ACharacter, public IDamageable
{
	GENERATED_BODY()

public:
	APixelDamageDummy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;

	virtual bool ApplyDamage_Implementation(const FDamageSpec& Spec) override;
	virtual void GetHealth_Implementation(float& CurrentHealth, float& MaxHealth) override;
	virtual bool IsAlive_Implementation() override;
	virtual ETeamId GetTeamId_Implementation() override;

	UFUNCTION(BlueprintPure, Category = "Damageable")
	UDamageableComponent* GetDamageable() const { return Damageable; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damageable")
	TObjectPtr<UDamageableComponent> Damageable = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damageable")
	TObjectPtr<UStaticMeshComponent> BodyMesh = nullptr;
};
