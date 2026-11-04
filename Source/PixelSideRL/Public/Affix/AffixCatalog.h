#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Affix/AffixTypes.h"
#include "AffixCatalog.generated.h"

/** Optional catalog. PopulateP05Defaults builds transient DA_Affix_* rows. Do not save a stand-in uasset. */
UCLASS(BlueprintType)
class PIXELSIDERL_API UAffixCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	FName PoolId = TEXT("P05");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
	TArray<TObjectPtr<UAffixDefinition>> Definitions;

	UFUNCTION(BlueprintCallable, Category = "Affix")
	void PopulateP05Defaults();
};
