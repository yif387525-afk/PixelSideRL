#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Combat/DamageSpec.h"
#include "Combat/Damageable.generated.h"

/**
 * BI_Damageable. Implemented by UDamageableComponent.
 * Call the component (or the actor, which forwards) — do not look up a Blueprint-only asset.
 */
UINTERFACE(BlueprintType, meta = (DisplayName = "BI_Damageable"))
class PIXELSIDERL_API UDamageable : public UInterface
{
	GENERATED_BODY()
};

class PIXELSIDERL_API IDamageable
{
	GENERATED_BODY()

public:
	/** Apply the spec. Returns whether the target is still alive. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damageable")
	bool ApplyDamage(const FDamageSpec& Spec);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damageable")
	void GetHealth(float& CurrentHealth, float& MaxHealth);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damageable")
	bool IsAlive();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Damageable")
	ETeamId GetTeamId();
};
