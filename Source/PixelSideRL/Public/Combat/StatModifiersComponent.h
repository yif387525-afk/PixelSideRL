#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Affix/AffixTypes.h"
#include "Affix/AffixSubsystem.h"
#include "Combat/StatModifiersComponent.generated.h"

/**
 * AC_StatModifiers placeholder. GetStat returns the Base layer (weapon DA / move feel).
 * No Roll, no inventory. A future affix component can replace this class and keep GetStat.
 * Missing component: callers fall back to the same bases (see UWeaponControllerComponent).
 */
UCLASS(ClassGroup = (PixelSide), meta = (BlueprintSpawnableComponent, DisplayName = "AC_StatModifiers"))
class PIXELSIDERL_API UStatModifiersComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatModifiersComponent();

	UFUNCTION(BlueprintCallable, Category = "Affix|Stats")
	void SetBaseStat(EStatId Stat, float Value);

	UFUNCTION(BlueprintPure, Category = "Affix|Stats")
	float GetStat(EStatId Stat) const;

	UFUNCTION(BlueprintCallable, Category = "Affix|Stats")
	void NotifyStatsRebuilt();

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnAffixChanged OnAffixChanged;

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnStatsRebuilt OnStatsRebuilt;

	static float FallbackBase(EStatId Stat);

private:
	UPROPERTY()
	TMap<EStatId, float> BaseStats;
};
