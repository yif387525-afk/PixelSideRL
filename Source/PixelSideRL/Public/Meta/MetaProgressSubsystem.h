#pragma once

#include "CoreMinimal.h"
#include "Affix/AffixSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MetaProgressSubsystem.generated.h"

class FMetaProgressState;

/**
 * Out-of-run Core and the five meta nodes.
 * In-run Scrap stays on UEventSubsystem. This type does not buy shop goods and does not write a save file.
 * See Docs/MetaProgress_API.md.
 */
UCLASS()
class PIXELSIDERL_API UMetaProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Meta", meta = (WorldContext = "WorldContext"))
	static UMetaProgressSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintPure, Category = "Meta")
	int32 GetCore() const;

	/** Out-of-run Core only. Does not change RunScrap. Result is clamped at 0. */
	UFUNCTION(BlueprintCallable, Category = "Meta")
	void AddCore(int32 Delta, const FString& Reason);

	/** 0 when NodeId is unknown or not purchased. A new slot is all zeros. */
	UFUNCTION(BlueprintPure, Category = "Meta")
	int32 GetPurchasedRank(FName NodeId) const;

	/** Every loaded node, including ranks that are still 0. */
	UFUNCTION(BlueprintPure, Category = "Meta")
	TMap<FName, int32> GetPurchasedNodes() const;

	UFUNCTION(BlueprintPure, Category = "Meta")
	bool CanPurchase(FName NodeId) const;

	/** Spends Core and raises the rank by one. Fails at MaxPurchases or when Core is short. */
	UFUNCTION(BlueprintCallable, Category = "Meta")
	bool Purchase(FName NodeId);

	/**
	 * PermanentModifiers, StartingScrap (Stash), the ten MVP affix ids, MetaFlags, MetaCoreMult.
	 * UnlockAffixIds on nodes are not a pool gate. Piercer and TwinShot are already in the ten.
	 */
	UFUNCTION(BlueprintPure, Category = "Meta")
	FRunStartContext BuildRunStartContext() const;

	/**
	 * BuildRunStartContext, then UAffixSubsystemDefault::BeginRun, then
	 * UEventSubsystem::ResetRunScrap(StartingScrap).
	 */
	UFUNCTION(BlueprintCallable, Category = "Meta")
	FRunStartContext BeginRunFromMeta();

	/** DA_Economy.MetaCoreMult plus the purchased SalvageSense delta. 0 if the economy table did not load. */
	UFUNCTION(BlueprintPure, Category = "Meta")
	float GetMetaCoreMult() const;

	/**
	 * RoguelikeMeta §6 preview. Does not call AddCore and does not end the run.
	 * Floor is applied to the whole product, then SoftCapPerRun.
	 */
	UFUNCTION(BlueprintPure, Category = "Meta")
	int32 PreviewCoreGain(bool bCleared, int32 RoomsCleared, int32 BossKilled, int32 RunScrapRemain) const;

	UFUNCTION(BlueprintPure, Category = "Meta")
	bool HasMetaFlag(FName Flag) const;

	UFUNCTION(BlueprintCallable, Category = "Meta")
	void ResetSlot();

	/** Memory only this pass. Returns false and does not write Saved/SaveGames. */
	UFUNCTION(BlueprintCallable, Category = "Meta")
	bool Save();

	/** Memory only this pass. Returns false and leaves the current wallet untouched. */
	UFUNCTION(BlueprintCallable, Category = "Meta")
	bool Load();

	UFUNCTION(BlueprintPure, Category = "Meta|Debug")
	FString DescribeRunStartContext() const;

private:
	void DiscardState();
	void ReloadFromContent();
	bool PurchaseInternal(FName NodeId, FString* Reason);

	FMetaProgressState* State = nullptr;
};
