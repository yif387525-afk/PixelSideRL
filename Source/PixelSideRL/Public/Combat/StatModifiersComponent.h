#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Affix/AffixTypes.h"
#include "Affix/AffixSubsystem.h"
#include "StatModifiersComponent.generated.h"

/**
 * AC_StatModifiers. Base layer stays on this component (weapon DA / move feel).
 * When UAffixSubsystemDefault is alive, GetStat returns that subsystem's aggregate
 * (Base → Meta → Affix passive → temp), so it matches UAffixSubsystem::GetStat.
 * OnAffixChanged / OnStatsRebuilt are forwarded from the subsystem for weapon cache refresh.
 */
UCLASS(ClassGroup = (PixelSide), meta = (BlueprintSpawnableComponent, DisplayName = "AC_StatModifiers"))
class PIXELSIDERL_API UStatModifiersComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatModifiersComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;

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
	UFUNCTION()
	void HandleSubsystemAffixChanged(FAffixChangeEvent Event);

	UFUNCTION()
	void HandleSubsystemStatsRebuilt();

	void EnsureAffixBound();
	void UnbindAffix();
	UAffixSubsystem* ResolveAffixSubsystem() const;

	UPROPERTY()
	TMap<EStatId, float> BaseStats;

	UPROPERTY()
	TObjectPtr<UAffixSubsystem> BoundAffix = nullptr;
};
