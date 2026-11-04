#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Affix/AffixSubsystem.h"
#include "AffixInventoryComponent.generated.h"

/**
 * AC_AffixInventory. Actor-facing copy of the run inventory.
 * ConfirmAffixChoice / GrantAffix write here, then AC_StatModifiers::GetStat matches the subsystem.
 */
UCLASS(ClassGroup = (PixelSide), meta = (BlueprintSpawnableComponent, DisplayName = "AC_AffixInventory"))
class PIXELSIDERL_API UAffixInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAffixInventoryComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Affix|Inventory")
	bool AddAffix(UAffixDefinition* Definition);

	UFUNCTION(BlueprintPure, Category = "Affix|Inventory")
	int32 GetStacks(FName AffixId) const;

	UFUNCTION(BlueprintPure, Category = "Affix|Inventory")
	TArray<FAffixRuntimeEntry> GetOwned() const { return Owned; }

	UFUNCTION(BlueprintCallable, Category = "Affix|Inventory")
	void RefreshOwned();

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnAffixChanged OnAffixChanged;

private:
	UFUNCTION()
	void HandleAffixChanged(FAffixChangeEvent Event);

	UFUNCTION()
	void HandleStatsRebuilt();

	void BindSubsystem();
	void UnbindSubsystem();
	UAffixSubsystem* Resolve() const;

	UPROPERTY()
	TArray<FAffixRuntimeEntry> Owned;

	UPROPERTY()
	TObjectPtr<UAffixSubsystem> Bound = nullptr;
};
