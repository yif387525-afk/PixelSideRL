#include "Combat/PixelSideGameMode.h"

#include "Combat/PixelDamageDummy.h"
#include "Combat/PixelDebugHurtTurret.h"
#include "Combat/PixelEnemyGrunt.h"
#include "Combat/PixelSideCharacter.h"
#include "Combat/PixelSideCharacterMovement.h"
#include "Combat/PixelSideCombatLog.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/PixelSideLabSandboxLayout.h"
#include "Combat/PixelSideMeshUtil.h"
#include "Combat/PixelSidePalette.h"
#include "Flow/PixelHubRunFlowSubsystem.h"
#include "Rooms/PixelRoomDirector.h"
#include "Rooms/PixelShopRoomHost.h"
#include "Rooms/PixelSideRoomFlow.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Level.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

#if !UE_BUILD_SHIPPING
static int32 GPixelSideDriveRoomCombat01 = 0;
static FAutoConsoleVariableRef CVarPixelSideDriveRoomCombat01(
	TEXT("PixelSide.DriveRoomCombat01"),
	GPixelSideDriveRoomCombat01,
	TEXT("Empty-map fallback. Set to 1 before Play to drive Room_Combat_01 from CSV instead of the Lab sandbox. The official Room_Combat_01 map does not need this."),
	ECVF_Default);

static int32 GPixelSideDriveRoomShop01 = 0;
static FAutoConsoleVariableRef CVarPixelSideDriveRoomShop01(
	TEXT("PixelSide.DriveRoomShop01"),
	GPixelSideDriveRoomShop01,
	TEXT("Set to 1 before Play to open Room_Shop_01 through AC_ShopPresenter (run wallet). Does not change F11."),
	ECVF_Default);
#endif

namespace
{
FVector ResolveRoomCombat01PlayerStart()
{
	float X = PixelSideRoom::RoomCombat01PlayerStartX;
	float Y = PixelSideRoom::RoomCombat01PlayerStartY;
	const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Rooms/Room_Combat_01_geometry.csv"));
	FString Text;
	if (FFileHelper::LoadFileToString(Text, *Path))
	{
		std::vector<PixelSideRoom::FGeometryRow> Rows;
		std::string Error;
		if (PixelSideRoom::LoadGeometryCsv(std::string(TCHAR_TO_UTF8(*Text)), Rows, Error))
		{
			PixelSideRoom::FindAnchor(Rows, "PlayerStart", X, Y);
		}
		else
		{
			UE_LOG(LogPixelSideCombat, Warning, TEXT("Room_Combat_01 geometry: %s"), *FString(UTF8_TO_TCHAR(Error.c_str())));
		}
	}
	return FVector(X, 0.f, PixelSideCombat::CapsuleHalfHeight + static_cast<float>(PixelSideLabLayout::SpawnClearance) + Y);
}

FVector ResolveRoomShop01PlayerStart()
{
	float X = PixelSideRoom::RoomShop01PlayerStartX;
	float Y = PixelSideRoom::RoomShop01PlayerStartY;
	const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Rooms/Room_Shop_01_geometry.csv"));
	FString Text;
	if (FFileHelper::LoadFileToString(Text, *Path))
	{
		std::vector<PixelSideRoom::FGeometryRow> Rows;
		std::string Error;
		if (PixelSideRoom::LoadGeometryCsv(std::string(TCHAR_TO_UTF8(*Text)), Rows, Error))
		{
			PixelSideRoom::FindAnchor(Rows, "PlayerStart", X, Y);
		}
		else
		{
			UE_LOG(LogPixelSideCombat, Warning, TEXT("Room_Shop_01 geometry: %s"), *FString(UTF8_TO_TCHAR(Error.c_str())));
		}
	}
	return FVector(X, 0.f, PixelSideCombat::CapsuleHalfHeight + static_cast<float>(PixelSideLabLayout::SpawnClearance) + Y);
}

bool FStringNamesOfficialRoom(const FString& Name)
{
	if (Name.IsEmpty())
	{
		return false;
	}
	const std::string Utf8(TCHAR_TO_UTF8(*Name));
	return PixelSideRoom::MapNameNamesRoomCombat01(Utf8);
}

bool MapSignalsOfficialRoomCombat01(const UWorld* World, const FString& MapName)
{
	if (FStringNamesOfficialRoom(MapName))
	{
		return true;
	}
	if (!World)
	{
		return false;
	}
	if (FStringNamesOfficialRoom(World->GetName()))
	{
		return true;
	}
	// GetMapName walks the loaded package. Skip it until the persistent level exists.
	if (const ULevel* Level = World->PersistentLevel)
	{
		if (FStringNamesOfficialRoom(World->GetMapName()))
		{
			return true;
		}
		if (const UObject* Package = Level->GetOutermost())
		{
			return FStringNamesOfficialRoom(Package->GetName());
		}
	}
	return false;
}

bool HasPlacedRoomDirector(UWorld* World)
{
	if (!World)
	{
		return false;
	}
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, APixelRoomDirector::StaticClass(), Found);
	return Found.Num() > 0;
}

void EnsureHubPlayerStart(UWorld* World)
{
	if (!World)
	{
		return;
	}
	TArray<AActor*> Starts;
	UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), Starts);
	if (Starts.Num() > 0)
	{
		return;
	}
	const FVector HubSpawn(
		0.f,
		0.f,
		PixelSideCombat::CapsuleHalfHeight + static_cast<float>(PixelSideLabLayout::SpawnClearance));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), HubSpawn, FRotator::ZeroRotator, Params);
}
}

APixelSideGameMode::APixelSideGameMode()
{
	DefaultPawnClass = APixelSideCharacter::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
#if UE_BUILD_SHIPPING
	bSpawnLabSandbox = false;
#endif
}

void APixelSideGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	StartupMapName = MapName;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Hub is not a combat room. Leave its PlayerStart. Do not move it onto the Lab spawn.
	// Official Room_Combat_01 still wins below when the map actually names that room.
	if (UPixelHubRunFlowSubsystem::IsHubMap(this, MapName))
	{
		UE_LOG(LogPixelSideCombat, Log, TEXT("Hub map. PlayerStart stays. Lab sandbox stays out. No director."));
		EnsureHubPlayerStart(World);
		return;
	}

	// Official Room_Combat_01 already has geometry, a PlayerStart, and a director.
	// InitGame runs before BeginPlay, so the map / package name is the early signal.
	// A director already in the level is the same signal when the actor list is up.
	// Do not move that PlayerStart onto the Lab spawn or the empty-map CSV anchor.
	const PixelSideRoom::ERoomCombat01Startup Startup = PixelSideRoom::ResolveRoomCombat01Startup(
		MapSignalsOfficialRoomCombat01(World, MapName),
		HasPlacedRoomDirector(World),
		ShouldDriveRoomCombat01());
	if (Startup == PixelSideRoom::ERoomCombat01Startup::OfficialPlay)
	{
		UE_LOG(LogPixelSideCombat, Log, TEXT("Official Room_Combat_01 play (map name or placed RoomDirector). PlayerStart stays. Lab sandbox stays out."));
		// The placed DirectionalLight_Room often did not keep intensity or rotation.
		// With auto exposure off, that light does not show in Lit. Same sun as Lab.
		EnsureSideSun(World, TEXT("PixelSideRoom"));
		return;
	}

	// Entry ships a PlayerStart (often the BADsize one). Place the pawn for this run.
	// The 5-zone lab uses design feet on G_A; otherwise keep the legacy air spawn.
	// Empty-map P0.7 uses the CSV PlayerStart and skips the lab.
	// Room_Shop_01 does the same with its own anchor. Combat wins if both drives are on.
	const bool bRoom = Startup == PixelSideRoom::ERoomCombat01Startup::EmptyMapDrive;
	const bool bShop = !bRoom && ShouldDriveRoomShop01();
	const bool bBuildLab = !bRoom && !bShop && bSpawnLabSandbox && !HasDamageDummy();
	const FVector LabSpawn = bRoom
		? ResolveRoomCombat01PlayerStart()
		: (bShop
			? ResolveRoomShop01PlayerStart()
			: (bBuildLab
			? FVector(
				static_cast<float>(PixelSideLabLayout::PlayerSpawnFeetX),
				static_cast<float>(PixelSideLabLayout::SlabCenterY),
				PixelSideCombat::CapsuleHalfHeight + static_cast<float>(PixelSideLabLayout::SpawnClearance))
			: FVector(PixelSideCombat::LabPlayerSpawnX, 0.f, PixelSideCombat::LabPlayerSpawnZ)));

	TArray<AActor*> Starts;
	UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), Starts);
	if (Starts.Num() > 0)
	{
		if (APlayerStart* Start = Cast<APlayerStart>(Starts[0]))
		{
			Start->SetActorLocation(LabSpawn);
		}
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), LabSpawn, FRotator::ZeroRotator, Params);
}

void APixelSideGameMode::BeginPlay()
{
	Super::BeginPlay();
	UWorld* World = GetWorld();
	// Hub before the combat startup so this map does not lay out the Lab or spawn a director.
	// It does not call EnsureSideSun. Official Room_Combat_01 below still does.
	if (UPixelHubRunFlowSubsystem::IsHubMap(this, StartupMapName))
	{
		UE_LOG(LogPixelSideCombat, Log, TEXT("Hub map. OpenHub. Lab sandbox stays out. No director."));
		if (UPixelHubRunFlowSubsystem* HubFlow = UPixelHubRunFlowSubsystem::Get(this))
		{
			HubFlow->HandleHubMapBegan();
		}
		else
		{
			UE_LOG(LogPixelSideCombat, Warning, TEXT("Hub map has no UPixelHubRunFlowSubsystem. OpenHub was not called."));
		}
		return;
	}
	// BeginPlay sees a director placed in the level even if InitGame only had the map name.
	// Official play must not SpawnLabSandbox and must not spawn a second director
	// (a spawned director would build CSV greybox on top of the official geometry).
	const PixelSideRoom::ERoomCombat01Startup Startup = PixelSideRoom::ResolveRoomCombat01Startup(
		MapSignalsOfficialRoomCombat01(World, StartupMapName),
		HasPlacedRoomDirector(World),
		ShouldDriveRoomCombat01());
	if (Startup == PixelSideRoom::ERoomCombat01Startup::OfficialPlay)
	{
		if (ShouldDriveRoomShop01())
		{
			UE_LOG(LogPixelSideCombat, Log, TEXT("Room_Shop_01 drive is ignored while Room_Combat_01 official play is active."));
		}
		UE_LOG(LogPixelSideCombat, Log, TEXT("Official Room_Combat_01 play. Lab sandbox stays out. No second director."));
		// BeginPlay sees a director already placed on the official room. Do not skip the sun.
		EnsureSideSun(World, TEXT("PixelSideRoom"));
		if (MapSignalsOfficialRoomCombat01(World, StartupMapName))
		{
			if (UPixelHubRunFlowSubsystem* CombatFlow = UPixelHubRunFlowSubsystem::Get(this))
			{
				CombatFlow->HandleCombatMapBegan();
			}
			else
			{
				UE_LOG(LogPixelSideCombat, Warning, TEXT("Room_Combat_01 has no UPixelHubRunFlowSubsystem. Run summary will not open."));
			}
		}
		return;
	}
	if (Startup == PixelSideRoom::ERoomCombat01Startup::EmptyMapDrive)
	{
		if (ShouldDriveRoomShop01())
		{
			UE_LOG(LogPixelSideCombat, Log, TEXT("Room_Shop_01 drive is ignored while Room_Combat_01 drive is on."));
		}
		UE_LOG(LogPixelSideCombat, Log, TEXT("P0.7 Room_Combat_01 drive is on. Lab sandbox stays out of this play."));
		SpawnRoomCombat01();
		return;
	}
	if (ShouldDriveRoomShop01())
	{
		UE_LOG(LogPixelSideCombat, Log, TEXT("Room_Shop_01 drive is on. OpenShop uses RunScrap. Lab sandbox and F11 stay out of this play."));
		SpawnRoomShop01();
		return;
	}
	if (bSpawnLabSandbox && !HasDamageDummy())
	{
		SpawnLabSandbox();
	}
}

bool APixelSideGameMode::ShouldDriveRoomCombat01() const
{
	if (bDriveRoomCombat01)
	{
		return true;
	}
#if !UE_BUILD_SHIPPING
	return GPixelSideDriveRoomCombat01 != 0;
#else
	return false;
#endif
}

void APixelSideGameMode::SpawnRoomCombat01()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, APixelRoomDirector::StaticClass(), Found);
	if (Found.Num() > 0)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<APixelRoomDirector>(APixelRoomDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
}

bool APixelSideGameMode::ShouldDriveRoomShop01() const
{
	if (bDriveRoomShop01)
	{
		return true;
	}
#if !UE_BUILD_SHIPPING
	return GPixelSideDriveRoomShop01 != 0;
#else
	return false;
#endif
}

void APixelSideGameMode::SpawnRoomShop01()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, APixelShopRoomHost::StaticClass(), Found);
	if (Found.Num() > 0)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<APixelShopRoomHost>(APixelShopRoomHost::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
}

void APixelSideGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bLabPitRecoveryActive)
	{
		RecoverLabPitFall();
	}
}

bool APixelSideGameMode::HasDamageDummy() const
{
	if (!GetWorld())
	{
		return false;
	}
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APixelDamageDummy::StaticClass(), Found);
	return Found.Num() > 0;
}

void APixelSideGameMode::EnsureSideSun(UWorld* World, const TCHAR* NewLightTag)
{
	if (!World)
	{
		return;
	}

	// Camera sits on +Y looking down -Y. Yaw -90 puts the sun on that side so the faces
	// in view, and the floor, catch it. Numbers are PixelSideRoom::SideSun* so Lab,
	// official Play, and the empty-map director cannot drift apart.
	// No skylight. Lab is already visible with this sun while auto exposure stays off.
	// A fill light would lift shadows past that look.
	const FRotator SunRotation(
		PixelSideRoom::SideSunPitchDeg,
		PixelSideRoom::SideSunYawDeg,
		PixelSideRoom::SideSunRollDeg);

	auto Apply = [SunRotation](ADirectionalLight* Sun)
	{
		if (!Sun)
		{
			return;
		}
		// Mobility first. A static placed light ignores rotation until it can move.
		// The light component is the directional actor's root, so this is the actor mobility too.
		UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
		if (Light)
		{
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetIntensity(PixelSideRoom::SideSunIntensityLux);
			Light->SetAtmosphereSunLight(false);
		}
		Sun->SetActorRotation(SunRotation);
		if (Light)
		{
			Light->SetWorldRotation(SunRotation);
		}
	};

	TArray<AActor*> Lights;
	UGameplayStatics::GetAllActorsOfClass(World, ADirectionalLight::StaticClass(), Lights);
	if (Lights.Num() == 0)
	{
		FActorSpawnParameters LightParams;
		LightParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
			ADirectionalLight::StaticClass(),
			FVector::ZeroVector,
			SunRotation,
			LightParams);
		if (!Sun)
		{
			UE_LOG(LogPixelSideCombat, Warning, TEXT("Side sun spawn failed."));
			return;
		}
		if (NewLightTag && *NewLightTag)
		{
			Sun->Tags.Add(NewLightTag);
		}
		Apply(Sun);
		UE_LOG(LogPixelSideCombat, Log, TEXT("Side sun spawned (%.0f lux, rotation %.0f, %.0f, %.0f)."),
			PixelSideRoom::SideSunIntensityLux,
			PixelSideRoom::SideSunPitchDeg,
			PixelSideRoom::SideSunYawDeg,
			PixelSideRoom::SideSunRollDeg);
		return;
	}

	for (AActor* Actor : Lights)
	{
		Apply(Cast<ADirectionalLight>(Actor));
	}
	UE_LOG(LogPixelSideCombat, Log, TEXT("Side sun retargeted %d directional light(s) (%.0f lux, rotation %.0f, %.0f, %.0f)."),
		Lights.Num(),
		PixelSideRoom::SideSunIntensityLux,
		PixelSideRoom::SideSunPitchDeg,
		PixelSideRoom::SideSunYawDeg,
		PixelSideRoom::SideSunRollDeg);
}

void APixelSideGameMode::SpawnLabSandbox()
{
	using namespace PixelSideLabLayout;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FLinearColor FloorColor = PixelSidePalette::Floor();
	const FLinearColor WallColor = PixelSidePalette::Wall();
	const float CenterY = static_cast<float>(SlabCenterY);
	const float PlatformT = static_cast<float>(PlatformThickness);

	auto Slab = [this, CenterY](float X0, float X1, float TopY, float Thickness, const FLinearColor& Color, const TCHAR* Id)
	{
		const FVector Center((X0 + X1) * 0.5f, CenterY, TopY - Thickness * 0.5f);
		SpawnLabBox(Center, X1 - X0, Thickness, Color, Id);
	};
	auto Platform = [&Slab, FloorColor, PlatformT](float CenterX, float Width, float TopY, const TCHAR* Id)
	{
		Slab(CenterX - Width * 0.5f, CenterX + Width * 0.5f, TopY, PlatformT, FloorColor, Id);
	};
	auto Cover = [this, WallColor, CenterY](float BottomCenterX, float BottomY, float Height, float Width, const TCHAR* Id, bool bLetPawnsPass)
	{
		const FVector Center(BottomCenterX, CenterY, BottomY + Height * 0.5f);
		SpawnLabBox(Center, Width, Height, WallColor, Id, bLetPawnsPass);
	};

	// §3.1 floors. PIT_C is the gap. Island uses ground thickness. No BRIDGE unless asked.
	Slab(G_A_X0, G_A_X1, G_A_TopY, GroundThickness, FloorColor, TEXT("G_A"));
	Slab(IslandX0, IslandX1, IslandTopY, GroundThickness, FloorColor, TEXT("G_C_island"));
	Slab(G_C_Right_X0, G_C_Right_X1, G_C_Right_TopY, GroundThickness, FloorColor, TEXT("G_C_right"));
	if (bSpawnLabBridge)
	{
		Slab(BridgeX0, BridgeX1, BridgeTopY, PlatformThickness, FloorColor, TEXT("BRIDGE"));
	}

	// §3.2
	Platform(P1CenterX, P1Width, P1TopY, TEXT("P1"));
	Platform(P2CenterX, P2Width, P2TopY, TEXT("P2"));
	Platform(P3CenterX, P3Width, P3TopY, TEXT("P3"));
	Platform(P_E_CenterX, P_E_Width, P_E_TopY, TEXT("P_E_high"));

	// §3.3 Taller than the jump, and the slab covers the shot plane, so there is no
	// Y side to walk around and no top to land on. Pawns pass through. Shots and
	// visibility still hit. Peek is standing on the far side along X.
	Cover(CV1_X, CV1_BottomY, CV1_Height, CV1_Width, TEXT("CV1"), true);
	Cover(CV2_X, CV2_BottomY, CV2_Height, CV2_Width, TEXT("CV2"), true);
	Cover(CV3_X, CV3_BottomY, CV3_Height, CV3_Width, TEXT("CV3"), true);

	// Boundary walls. Height is above the walkable plane; sink through the floor so the pawn cannot slip under.
	const float WallTotalHeight = static_cast<float>(WallHeight + GroundThickness);
	const float WallCenterZ = (static_cast<float>(WallHeight) - static_cast<float>(GroundThickness)) * 0.5f;
	SpawnLabBox(FVector(WallLeftX, CenterY, WallCenterZ), WallThickness, WallTotalHeight, WallColor, TEXT("Wall_L"));
	SpawnLabBox(FVector(WallRightX, CenterY, WallCenterZ), WallThickness, WallTotalHeight, WallColor, TEXT("Wall_R"));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FRotator FaceLeft(0.f, 180.f, 0.f);
	const float DummyLift = PixelSideCombat::CapsuleHalfHeight + static_cast<float>(SpawnClearance);

	const struct FLabDummy
	{
		const TCHAR* Id;
		float FeetX;
		float FeetY;
	} Dummies[] = {
		{TEXT("D1"), static_cast<float>(D1_FeetX), static_cast<float>(D1_FeetY)},
		{TEXT("D2"), static_cast<float>(D2_FeetX), static_cast<float>(D2_FeetY)},
		{TEXT("D3"), static_cast<float>(D3_FeetX), static_cast<float>(D3_FeetY)},
		{TEXT("D4"), static_cast<float>(D4_FeetX), static_cast<float>(D4_FeetY)},
		{TEXT("D5"), static_cast<float>(D5_FeetX), static_cast<float>(D5_FeetY)},
	};
	for (const FLabDummy& Mark : Dummies)
	{
		APixelDamageDummy* Dummy = World->SpawnActor<APixelDamageDummy>(
			APixelDamageDummy::StaticClass(),
			FVector(Mark.FeetX, CenterY, Mark.FeetY + DummyLift),
			FaceLeft,
			Params);
		if (Dummy)
		{
			Dummy->Tags.Add(TEXT("PixelSideLab"));
			Dummy->Tags.Add(Mark.Id);
		}
	}

	// G1. Stats live on APixelEnemyGrunt (CombatFeel §7). Pawn-ignore is on that class.
	const float GruntLift = PixelSideCombat::EnemyGruntCapsuleHalfHeight + static_cast<float>(SpawnClearance);
	APixelEnemyGrunt* Grunt = World->SpawnActor<APixelEnemyGrunt>(
		APixelEnemyGrunt::StaticClass(),
		FVector(static_cast<float>(G1_FeetX), CenterY, static_cast<float>(G1_FeetY) + GruntLift),
		FRotator::ZeroRotator,
		Params);
	if (Grunt)
	{
		Grunt->Tags.Add(TEXT("PixelSideLab"));
		Grunt->Tags.Add(TEXT("G1"));
	}

	// T1 position only. Damage per shot is not set here — 弹芯 owns the Lab default
	// (single hit <= 25% MaxHP) on APixelDebugHurtTurret / GruntDamage.
	// That row is not the grunt above.
	APixelDebugHurtTurret* Turret = World->SpawnActor<APixelDebugHurtTurret>(
		APixelDebugHurtTurret::StaticClass(),
		FVector(static_cast<float>(T1_FeetX), CenterY, static_cast<float>(T1_FeetY + TurretBodyHalfHeight)),
		FaceLeft,
		Params);
	if (Turret)
	{
		Turret->Tags.Add(TEXT("PixelSideLab"));
		Turret->Tags.Add(TEXT("T1"));
	}

	// Entry has no lights. BasicShapeMaterial is lit, so a map with no sun draws solid black.
	EnsureSideSun(World, TEXT("PixelSideLab"));

	bLabPitRecoveryActive = true;
	SetActorTickEnabled(true);
}

void APixelSideGameMode::RecoverLabPitFall()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ACharacter* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	if (!Player)
	{
		return;
	}

	const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
	const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : PixelSideCombat::CapsuleHalfHeight;
	const FVector Loc = Player->GetActorLocation();
	const float FeetZ = Loc.Z - HalfHeight;
	if (FeetZ >= static_cast<float>(PixelSideLabLayout::PitDeathY))
	{
		return;
	}
	if (Loc.X < static_cast<float>(PixelSideLabLayout::PitCatchX0) || Loc.X > static_cast<float>(PixelSideLabLayout::PitCatchX1))
	{
		return;
	}

	// Lab-only. Teleport back onto G_A, left of the island. Do not apply damage.
	// Design SafeFeetY is up (Unreal Z). Unreal Y stays on the depth plane.
	const FVector Safe(
		static_cast<float>(PixelSideLabLayout::SafeFeetX),
		static_cast<float>(PixelSideLabLayout::SlabCenterY),
		HalfHeight + static_cast<float>(PixelSideLabLayout::SpawnClearance));
	Player->SetActorLocation(Safe, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Move = Player->GetCharacterMovement())
	{
		if (UPixelSideCharacterMovement* PixelMove = Cast<UPixelSideCharacterMovement>(Move))
		{
			PixelMove->ClearKnockback();
		}
		Move->StopMovementImmediately();
		Move->SetMovementMode(MOVE_Walking);
	}
	UE_LOG(LogPixelSideCombat, Log, TEXT("Lab pit soft recovery to G_A safe point (0 damage)."));
}

void APixelSideGameMode::SpawnLabBox(const FVector& Center, float SizeX, float SizeZ, const FLinearColor& Color, const TCHAR* LayoutId, bool bLetPawnsPass) const
{
	const float Inv = 1.f / static_cast<float>(PixelSideLabLayout::CubeSize);
	const FVector Scale(SizeX * Inv, static_cast<float>(PixelSideLabLayout::SlabDepth) * Inv, SizeZ * Inv);
	SpawnBlock(Center, Scale, Color, LayoutId, bLetPawnsPass);
}

void APixelSideGameMode::SpawnBlock(const FVector& Location, const FVector& Scale, const FLinearColor& Color, const TCHAR* LayoutId, bool bLetPawnsPass) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Block = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (!Block)
	{
		return;
	}

	Block->Tags.Add(TEXT("PixelSideLab"));
	if (LayoutId)
	{
		Block->Tags.Add(LayoutId);
#if WITH_EDITOR
		Block->SetActorLabel(FString(LayoutId));
#endif
	}
	UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Block->SetActorScale3D(Scale);
	const PixelSideMesh::EMeshCollision Collision = bLetPawnsPass
		? PixelSideMesh::EMeshCollision::PassThroughPawn
		: PixelSideMesh::EMeshCollision::BlockAll;
	PixelSideMesh::Tint(Mesh, TEXT("/Engine/BasicShapes/Cube.Cube"), Color, Collision);
	Block->SetActorLocation(Location);
}
