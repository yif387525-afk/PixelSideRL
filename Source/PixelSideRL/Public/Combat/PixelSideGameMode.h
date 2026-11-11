#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PixelSideGameMode.generated.h"

class UWorld;

/**
 * Default mode for the combat slice. When bSpawnLabSandbox is set and the map
 * has no damage dummy, spawns the Lab_CombatP0 5-zone greybox (platforms, covers,
 * dummies, one grunt, one debug turret, a directional light) and soft-recovers pit falls.
 * Official Room_Combat_01 (map / package name names that room, or the level already
 * has APixelRoomDirector) skips the sandbox, leaves the in-map PlayerStart, and does
 * not spawn a second director. It still runs EnsureSideSun so Lit Play matches Lab.
 * The placed director runs the P0.7 loop.
 * bDriveRoomCombat01 (or the PixelSide.DriveRoomCombat01 cvar) is the empty-map
 * fallback: it skips the sandbox and spawns a director that builds greybox from CSV.
 * bDriveRoomShop01 (or PixelSide.DriveRoomShop01) skips the sandbox and opens
 * Room_Shop_01 through AC_ShopPresenter. Combat drive wins if both drives are set.
 * Official Room_Combat_01 play wins over either drive. Neither flag changes the F11 Lab shop widget.
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

	/**
	 * Lab's side sun (PixelSideRoom::SideSun*).
	 * If any directional light is already in the world, force each one: Mobility Movable,
	 * intensity SideSunIntensityLux, rotation (pitch, yaw, roll) on the actor and the light
	 * component, atmosphere sun off. If none exists, spawn one with those settings and tag it.
	 * Official Room_Combat_01 Play calls this so the early return does not leave Lit black.
	 */
	static void EnsureSideSun(UWorld* World, const TCHAR* NewLightTag);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PixelSide|Lab")
	bool bSpawnLabSandbox = true;

	/** Empty-map P0.7 fallback. Skips Lab and spawns a Room_Combat_01 director. Official Room_Combat_01 does not need this. Default off. */
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

	/** Map name passed to InitGame. BeginPlay also checks the world package and any placed director. */
	FString StartupMapName;
};
