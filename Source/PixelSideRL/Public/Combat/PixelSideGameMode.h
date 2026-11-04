#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PixelSideGameMode.generated.h"

/**
 * Default mode for the combat slice. When bSpawnLabSandbox is set and the map
 * has no damage dummy, spawns the Lab_CombatP0 5-zone greybox (platforms, covers,
 * dummies, one grunt, one debug turret, a directional light) and soft-recovers pit falls.
 * bDriveRoomCombat01 (or the PixelSide.DriveRoomCombat01 cvar) skips that sandbox
 * and runs the P0.7 Room_Combat_01 director instead.
 * bDriveRoomShop01 (or PixelSide.DriveRoomShop01) skips the sandbox and opens
 * Room_Shop_01 through AC_ShopPresenter. Combat drive wins if both are set.
 * Neither flag changes the F11 Lab shop widget.
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

	bool ShouldDriveRoomCombat01() const;
	bool ShouldDriveRoomShop01() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Lab")
	bool bSpawnLabSandbox = true;

	/** P0.7. Skips the Lab sandbox and drives Room_Combat_01 from CSV. Default off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bDriveRoomCombat01 = false;

	/** Room_Shop_01. Skips the Lab sandbox and opens AC_ShopPresenter. Default off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Room")
	bool bDriveRoomShop01 = false;

	/** Debug only. Locked default is island jump; BRIDGE is not a resident slab. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Lab")
	bool bSpawnLabBridge = false;

protected:
	bool HasDamageDummy() const;
	void SpawnLabSandbox();
	void SpawnRoomCombat01();
	void SpawnRoomShop01();
	void SpawnBlock(const FVector& Location, const FVector& Scale, const FLinearColor& Color, const TCHAR* LayoutId, bool bLetPawnsPass = false) const;
	void SpawnLabBox(const FVector& Center, float SizeX, float SizeZ, const FLinearColor& Color, const TCHAR* LayoutId, bool bLetPawnsPass = false) const;
	void RecoverLabPitFall();

	bool bLabPitRecoveryActive = false;
};
