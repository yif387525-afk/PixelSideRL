#pragma once

#include "CoreMinimal.h"
#include "Combat/DamageSpec.generated.h"

class AActor;
class AController;

/** E_TeamId. Friendly fire is off when both sides share a non-neutral team. */
UENUM(BlueprintType)
enum class ETeamId : uint8
{
	Player UMETA(DisplayName = "Player"),
	Enemy UMETA(DisplayName = "Enemy"),
	Neutral UMETA(DisplayName = "Neutral")
};

/** S_DamageSpec / FDamageSpec. P0 minimum set from Combat_P0_Checklist §2.2. */
USTRUCT(BlueprintType)
struct PIXELSIDERL_API FDamageSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Damage = 0.f;

	/** Horizontal travel in uu, along the hit. Receiver may replace this (player profile). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Knockback = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float HitStopVictim = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float HitStopInstigator = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TObjectPtr<AController> InstigatorController = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FName DamageTypeTag = NAME_None;

	FDamageSpec()
	{
		DamageTypeTag = FName(TEXT("Default"));
	}
};
