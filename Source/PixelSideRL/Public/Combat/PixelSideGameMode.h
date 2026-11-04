#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Combat/PixelSideGameMode.generated.h"

/**
 * Default mode for the combat slice. On an empty map, spawns a greybox lane,
 * a damage dummy, and a debug turret when bSpawnLabSandbox is set.
 */
UCLASS(Blueprintable)
class PIXELSIDERL_API APixelSideGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APixelSideGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Lab")
	bool bSpawnLabSandbox = true;

protected:
	bool HasDamageDummy() const;
	void SpawnLabSandbox();
	void SpawnBlock(const FVector& Location, const FVector& Scale, const FLinearColor& Color) const;
};
