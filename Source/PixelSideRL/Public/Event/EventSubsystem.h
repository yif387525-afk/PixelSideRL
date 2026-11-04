#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EventSubsystem.generated.h"

class FEventCatalogState;

/** One applied effect. Steps stay in JSON order. */
USTRUCT(BlueprintType)
struct PIXELSIDERL_API FEventOpStepView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FString Op;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	int32 ScrapBefore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	int32 ScrapAfter = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	float HpBefore = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	float HpAfter = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bApplied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bUnknown = false;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FString Message;
};

/** Result of ResolveEventChoice. UI may bind OnEventChoiceResolved or read this struct. */
USTRUCT(BlueprintType)
struct PIXELSIDERL_API FEventResolveView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName EventId;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName ChoiceId;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FText ResultToast;

	/** In-run Scrap after the choice. Not Core. Not FMetaSaveData. */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	int32 RunScrap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	float CurrentHP = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	float MaxHP = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bAffixOnResolve = false;

	/** MVP always false. bAffixOnResolve false is honored by not opening AffixChoice. */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bAffixChoiceRequested = false;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bCombatAllowed = false;

	/** MVP always false. */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bCombatStarted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FString Error;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TArray<FString> Notes;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TArray<FEventOpStepView> Steps;
};

USTRUCT(BlueprintType)
struct PIXELSIDERL_API FEventChoiceView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName ChoiceId;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FText Label;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FText ResultToast;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	int32 EffectCount = 0;
};

/** Read model for a future WBP_EventChoice. This PR does not build that widget. */
USTRUCT(BlueprintType)
struct PIXELSIDERL_API FEventDefinitionView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName EventId;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName DataAssetName;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FText PromptBody;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FText SkipHint;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bAllowDoorSkip = true;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bCombatAllowed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	bool bAffixOnResolve = false;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName BindRoomId;

	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TArray<FEventChoiceView> Choices;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEventChoiceResolved, FEventResolveView, Result);

/**
 * Loads Content/Data/Events/DA_Event_*.json (no uasset) and applies the four MVP ops.
 * RunScrap is the in-run wallet placeholder. It is not Core and it is not written to FMetaSaveData.
 * Shop prices in DA_ShopInventory spend this same Scrap later; this subsystem does not buy anything.
 */
UCLASS()
class PIXELSIDERL_API UEventSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Event", meta = (WorldContext = "WorldContext"))
	static UEventSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void BeginDestroy() override;

	/**
	 * Apply Choice.Effects in order in this call.
	 * GrantScrap adds RunScrap. DamageSelf / HealSelf write the player AC_Damageable
	 * through ApplyEventHealth (floor 1, no hurt feel, no death).
	 * Does not call RequestAffixChoice and does not change Core.
	 */
	UFUNCTION(BlueprintCallable, Category = "Event")
	bool ResolveEventChoice(FName EventId, FName ChoiceId, FEventResolveView& OutResult);

	UFUNCTION(BlueprintPure, Category = "Event")
	bool GetEventDefinition(FName EventId, FEventDefinitionView& Out) const;

	/** In-run Scrap. currencyScope=Run. Not FMetaSaveData.Core. */
	UFUNCTION(BlueprintPure, Category = "Event|Run")
	int32 GetRunScrap() const { return RunScrap; }

	/** Run start. Pass StartingScrap from the run context. Does not touch Core. */
	UFUNCTION(BlueprintCallable, Category = "Event|Run")
	void ResetRunScrap(int32 NewScrap = 0);

	/** Other in-run systems (a later shop purchase) should add or spend here, not via Core. */
	UFUNCTION(BlueprintCallable, Category = "Event|Run")
	void AddRunScrap(int32 Delta);

	UPROPERTY(BlueprintAssignable, Category = "Event")
	FOnEventChoiceResolved OnEventChoiceResolved;

private:
	void DiscardState();
	void ReloadFromContent();

	FEventCatalogState* State = nullptr;

	/** In-run Scrap wallet. Field name is RunScrap. Not serialized with meta save. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Event|Run", meta = (AllowPrivateAccess = "true"))
	int32 RunScrap = 0;
};
