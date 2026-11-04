#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PixelSideGameMode.generated.h"

/**
 * Default mode for the combat slice. When bSpawnLabSandbox is set and the map
 * has no damage dummy, spawns the Lab_CombatP0 5-zone greybox (platforms, covers,
 * dummies, one debug turret, a directional light) and soft-recovers pit falls.
 */
UCLASS(Blueprintable)
class PIXELSIDERL_API APixelSideGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APixelSideGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Lab")
	bool bSpawnLabSandbox = true;

	/** Debug only. Locked default is island jump; BRIDGE is not a resident slab. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Lab")
	bool bSpawnLabBridge = false;

protected:
	bool HasDamageDummy() const;
	void SpawnLabSandbox();
	void SpawnBlock(const FVector& Location, const FVector& Scale, const FLinearColor& Color, const TCHAR* LayoutId) const;
	void SpawnLabBox(const FVector& Center, float SizeX, float SizeZ, const FLinearColor& Color, const TCHAR* LayoutId) const;
	void RecoverLabPitFall();

	bool bLabPitRecoveryActive = false;
};
