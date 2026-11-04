#pragma once

#include "CoreMinimal.h"
#include "Affix/AffixSubsystem.h"
#include "AffixSubsystemDefault.generated.h"

class FAffixP05State;

/**
 * P0.6 affix subsystem. Loads DT_Affix_P05.json (else CSV, else the C++ table).
 * No uasset is required for GetStat or the three-choice roll.
 */
UCLASS()
class PIXELSIDERL_API UAffixSubsystemDefault : public UAffixSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Affix", meta = (WorldContext = "WorldContext"))
	static UAffixSubsystemDefault* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void BeginDestroy() override;

	virtual void BeginRun(FRunStartContext Context) override;
	virtual void EndRun() override;
	virtual bool IsInRun() const override;
	virtual void RequestAffixChoice(EAffixOfferSource Source) override;
	virtual bool ConfirmAffixChoice(int32 Index) override;
	virtual void SkipAffixChoice() override;
	virtual TArray<FAffixChoiceEntry> RollChoices(int32 Count, int32 Seed) override;
	virtual bool IsChoicePending() const override;
	virtual float GetStat(EStatId Stat) const override;
	virtual float GetStatForWeapon(EStatId Stat, FName WeaponId) const override;
	virtual float GetStatBase(EStatId Stat) const override;
	virtual void SetBaseStat(EStatId Stat, float Value) override;
	virtual int32 GetAffixStacks(FName AffixId) const override;
	virtual bool HasTag(FName Tag) const override;
	virtual TArray<FAffixRuntimeEntry> GetOwnedAffixes() const override;
	virtual FAffixCardView GetAffixCardView(const UAffixDefinition* Def, int32 PreviewStacks) const override;
	virtual FAffixCardView GetAffixCardViewFromId(FName AffixId) const override;
	virtual bool GrantAffix(FName AffixId) override;
	virtual bool DebugGrantAffix(FName AffixId) override;
	virtual void NotifyAffixTrigger(EAffixTrigger Trigger) override;
	virtual void ApplyChoicePause(bool bPaused) override;

private:
	void EnsureState();
	void EnsureRun();
	void ReloadTableFromContent();
	void RebuildDefinitions();
	void DiscardState();
	TArray<FAffixChoiceEntry> BuildEntries() const;
	UAffixDefinition* FindDefinition(FName AffixId) const;
	UWorld* ResolveWorld() const;

	FAffixP05State* State = nullptr;
	EAffixOfferSource LastOfferSource = EAffixOfferSource::NormalRoom;

	UPROPERTY()
	TArray<TObjectPtr<UAffixDefinition>> RuntimeAffixes;
};
