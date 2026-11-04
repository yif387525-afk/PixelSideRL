// Contract header — module may not compile until UE project exists.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Affix/AffixTypes.h"
#include "AffixSubsystem.generated.h"

class UAffixDefinition;
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
struct PIXELSIDERL_API FAffixChangeEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName AffixId;
	UPROPERTY(BlueprintReadOnly) int32 Stacks = 0;
	UPROPERTY(BlueprintReadOnly) bool bRemoved = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAffixChanged, FAffixChangeEvent, Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStatsRebuilt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAffixChoiceOffered, const TArray<FAffixChoiceEntry>&, Choices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAffixChoiceClosed, bool, bConfirmed);

/** See Docs/AffixSubsystem_API.md — BP surface for 弹芯 / 界面. */
UCLASS(Abstract)
class PIXELSIDERL_API UAffixSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Affix|Run")
	virtual void BeginRun(/* FRunStartContext */) PURE_VIRTUAL(UAffixSubsystem::BeginRun,);

	UFUNCTION(BlueprintCallable, Category = "Affix|Run")
	virtual void EndRun() PURE_VIRTUAL(UAffixSubsystem::EndRun,);

	UFUNCTION(BlueprintCallable, Category = "Affix|Choice")
	virtual void RequestAffixChoice() PURE_VIRTUAL(UAffixSubsystem::RequestAffixChoice,);

	UFUNCTION(BlueprintCallable, Category = "Affix|Choice")
	virtual bool ConfirmAffixChoice(int32 Index) PURE_VIRTUAL(UAffixSubsystem::ConfirmAffixChoice, return false;);

	UFUNCTION(BlueprintPure, Category = "Affix|Stats")
	virtual float GetStat(EStatId Stat) const PURE_VIRTUAL(UAffixSubsystem::GetStat, return 0.f;);

	UFUNCTION(BlueprintPure, Category = "Affix|Stats")
	virtual int32 GetAffixStacks(FName AffixId) const PURE_VIRTUAL(UAffixSubsystem::GetAffixStacks, return 0;);

	UFUNCTION(BlueprintPure, Category = "Affix|UI")
	virtual FAffixCardView GetAffixCardView(const UAffixDefinition* Def, int32 PreviewStacks = 1) const
		PURE_VIRTUAL(UAffixSubsystem::GetAffixCardView, return FAffixCardView(););

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnAffixChanged OnAffixChanged;

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnStatsRebuilt OnStatsRebuilt;

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnAffixChoiceOffered OnAffixChoiceOffered;

	UPROPERTY(BlueprintAssignable, Category = "Affix|Events")
	FOnAffixChoiceClosed OnAffixChoiceClosed;
};
