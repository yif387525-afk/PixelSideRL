// Contract header — concrete type is UAffixSubsystemDefault.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Affix/AffixTypes.h"
#include "AffixSubsystem.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct PIXELSIDERL_API FAffixCardView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FText Name;
	UPROPERTY(BlueprintReadOnly) FText Desc;
	UPROPERTY(BlueprintReadOnly) EAffixRarity Rarity = EAffixRarity::Common;
	UPROPERTY(BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(BlueprintReadOnly) FName AffixId;
	UPROPERTY(BlueprintReadOnly) int32 Stacks = 0;
	UPROPERTY(BlueprintReadOnly) bool bOwned = false;
};

USTRUCT(BlueprintType)
struct PIXELSIDERL_API FAffixChoiceEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) TObjectPtr<UAffixDefinition> Def = nullptr;
	UPROPERTY(BlueprintReadOnly) float WeightUsed = 0.f;
	UPROPERTY(BlueprintReadOnly) bool bWouldBeNew = true;
};

USTRUCT(BlueprintType)
struct PIXELSIDERL_API FAffixRuntimeEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName AffixId;
	UPROPERTY(BlueprintReadOnly) int32 Stacks = 0;
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UAffixDefinition> Def = nullptr;
};

USTRUCT(BlueprintType)
struct PIXELSIDERL_API FAffixChangeEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName AffixId;
	UPROPERTY(BlueprintReadOnly) int32 Stacks = 0;
	UPROPERTY(BlueprintReadOnly) bool bRemoved = false;
};

UENUM(BlueprintType)
enum class EAffixOfferSource : uint8
{
	NormalRoom,
	EliteRoom,
	PostBoss
};

/** Meta injection for BeginRun. Empty UnlockedAffixIds means the whole loaded pool. */
USTRUCT(BlueprintType)
struct PIXELSIDERL_API FRunStartContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	TArray<FAffixModifier> PermanentModifiers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	TArray<FName> UnlockedAffixIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	FName StartWeaponId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	int32 StartingScrap = 0;

	/** <= 0 is treated as seed 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	int32 Seed = 1;

	/** Filled by UMetaProgressSubsystem. Affix BeginRun does not read these. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meta")
	TArray<FName> MetaFlags;

	/** Economy base plus SalvageSense. Affix BeginRun does not read this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meta")
	float MetaCoreMult = 1.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAffixChanged, FAffixChangeEvent, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatsRebuilt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAffixChoiceOffered, const TArray<FAffixChoiceEntry>&, Choices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAffixChoiceClosed, bool, bConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAffixTrigger, EAffixTrigger, Trigger, FName, AffixId);
/** Flow / GameMode / PlayerController listens and may SetPause. Default subclass also pauses when bAutoPauseDuringChoice is set. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAffixPauseRequested, bool, bPaused);

/** See Docs/AffixSubsystem_API.md. UAffixSubsystemDefault is the P0.6 implementation. */
UCLASS(Abstract)
class PIXELSIDERL_API UAffixSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Affix|Run")
	virtual void BeginRun(FRunStartContext Context) PURE_VIRTUAL(UAffixSubsystem::BeginRun, );

	UFUNCTION(BlueprintCallable, Category = "Affix|Run")
	virtual void EndRun() PURE_VIRTUAL(UAffixSubsystem::EndRun, );

	UFUNCTION(BlueprintPure, Category = "Affix|Run")
	virtual bool IsInRun() const PURE_VIRTUAL(UAffixSubsystem::IsInRun, return false;);

	UFUNCTION(BlueprintCallable, Category = "Affix|Choice")
	virtual void RequestAffixChoice(EAffixOfferSource Source = EAffixOfferSource::NormalRoom) PURE_VIRTUAL(UAffixSubsystem::RequestAffixChoice, );

	UFUNCTION(BlueprintCallable, Category = "Affix|Choice")
	virtual bool ConfirmAffixChoice(int32 Index) PURE_VIRTUAL(UAffixSubsystem::ConfirmAffixChoice, return false;);

	UFUNCTION(BlueprintCallable, Category = "Affix|Choice")
	virtual void SkipAffixChoice() PURE_VIRTUAL(UAffixSubsystem::SkipAffixChoice, );

	UFUNCTION(BlueprintCallable, Category = "Affix|Choice")
	virtual TArray<FAffixChoiceEntry> RollChoices(int32 Count = 3, int32 Seed = -1) PURE_VIRTUAL(UAffixSubsystem::RollChoices, return TArray<FAffixChoiceEntry>(););

	UFUNCTION(BlueprintPure, Category = "Affix|Choice")
	virtual bool IsChoicePending() const PURE_VIRTUAL(UAffixSubsystem::IsChoicePending, return false;);

	UFUNCTION(BlueprintPure, Category = "Affix|Stats")
	virtual float GetStat(EStatId Stat) const PURE_VIRTUAL(UAffixSubsystem::GetStat, return 0.f;);

	/**
	 * Same fold as GetStat. WeaponId None is the no-weapon query (global mods only).
	 * Modifier WeaponTag None applies on every query. Any other tag applies only when
	 * it equals WeaponId. AC_WeaponController passes GetWeaponId().
	 */
	UFUNCTION(BlueprintPure, Category = "Affix|Stats")
	virtual float GetStatForWeapon(EStatId Stat, FName WeaponId) const;

	UFUNCTION(BlueprintPure, Category = "Affix|Stats")
	virtual float GetStatBase(EStatId Stat) const PURE_VIRTUAL(UAffixSubsystem::GetStatBase, return 0.f;);

	UFUNCTION(BlueprintCallable, Category = "Affix|Stats")
	virtual void SetBaseStat(EStatId Stat, float Value) PURE_VIRTUAL(UAffixSubsystem::SetBaseStat, );

	UFUNCTION(BlueprintPure, Category = "Affix|Stats")
	virtual int32 GetAffixStacks(FName AffixId) const PURE_VIRTUAL(UAffixSubsystem::GetAffixStacks, return 0;);

	UFUNCTION(BlueprintPure, Category = "Affix|Stats")
	virtual bool HasTag(FName Tag) const PURE_VIRTUAL(UAffixSubsystem::HasTag, return false;);

	UFUNCTION(BlueprintPure, Category = "Affix|Stats")
	virtual TArray<FAffixRuntimeEntry> GetOwnedAffixes() const PURE_VIRTUAL(UAffixSubsystem::GetOwnedAffixes, return TArray<FAffixRuntimeEntry>(););

	UFUNCTION(BlueprintPure, Category = "Affix|UI")
	virtual FAffixCardView GetAffixCardView(const UAffixDefinition* Def, int32 PreviewStacks = 1) const
		PURE_VIRTUAL(UAffixSubsystem::GetAffixCardView, return FAffixCardView(););

	UFUNCTION(BlueprintPure, Category = "Affix|UI")
	virtual FAffixCardView GetAffixCardViewFromId(FName AffixId) const
		PURE_VIRTUAL(UAffixSubsystem::GetAffixCardViewFromId, return FAffixCardView(););

	/** Runtime grant used by AC_AffixInventory. Starts a run if BeginRun was not called. */
	UFUNCTION(BlueprintCallable, Category = "Affix|Run")
	virtual bool GrantAffix(FName AffixId) PURE_VIRTUAL(UAffixSubsystem::GrantAffix, return false;);

	UFUNCTION(BlueprintCallable, Category = "Affix|Debug")
	virtual bool DebugGrantAffix(FName AffixId) PURE_VIRTUAL(UAffixSubsystem::DebugGrantAffix, return false;);

	UFUNCTION(BlueprintCallable, Category = "Affix|Trigger")
	virtual void NotifyAffixTrigger(EAffixTrigger Trigger) PURE_VIRTUAL(UAffixSubsystem::NotifyAffixTrigger, );

	/**
	 * Pause hook. Default implementation broadcasts OnAffixPauseRequested and, when auto-pause is on,
	 * calls UGameplayStatics::SetGamePaused. GameMode / PlayerController may listen and pause themselves;
	 * set SetAutoPauseDuringChoice(false) if the flow owns the pause.
	 */
	UFUNCTION(BlueprintCallable, Category = "Affix|Choice")
	virtual void ApplyChoicePause(bool bPaused);

	UFUNCTION(BlueprintCallable, Category = "Affix|Choice")
	virtual void SetAutoPauseDuringChoice(bool bEnabled);

protected:
	bool bAutoPauseDuringChoice = true;

public:
	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnAffixChanged OnAffixChanged;

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnStatsRebuilt OnStatsRebuilt;

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnAffixChoiceOffered OnAffixChoiceOffered;

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnAffixChoiceClosed OnAffixChoiceClosed;

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnAffixTrigger OnAffixTrigger;

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnAffixPauseRequested OnAffixPauseRequested;
};
