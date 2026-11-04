#include "Combat/PixelSideGameMode.h"

#include "Combat/PixelDamageDummy.h"
#include "Combat/PixelDebugHurtTurret.h"
#include "Combat/PixelSideCharacter.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/PixelSideMeshUtil.h"
#include "Combat/PixelSidePalette.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

APixelSideGameMode::APixelSideGameMode()
{
	DefaultPawnClass = APixelSideCharacter::StaticClass();
#if UE_BUILD_SHIPPING
	bSpawnLabSandbox = false;
#endif
}

void APixelSideGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UWorld* World = GetWorld();
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

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<APlayerStart>(
		APlayerStart::StaticClass(),
		FVector(PixelSideCombat::LabPlayerSpawnX, 0.f, PixelSideCombat::LabPlayerSpawnZ),
		FRotator::ZeroRotator,
		Params);
}

void APixelSideGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (bSpawnLabSandbox && !HasDamageDummy())
	{
		SpawnLabSandbox();
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

void APixelSideGameMode::SpawnLabSandbox()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SpawnBlock(
		FVector(PixelSideCombat::LabFloorCenterX, 0.f, PixelSideCombat::LabFloorCenterZ),
		FVector(PixelSideCombat::LabFloorScaleX, PixelSideCombat::LabFloorScaleY, PixelSideCombat::LabFloorScaleZ),
		PixelSidePalette::Floor());

	const float HalfWidth = 50.f * PixelSideCombat::LabFloorScaleX;
	const float Left = PixelSideCombat::LabFloorCenterX - HalfWidth;
	const float Right = PixelSideCombat::LabFloorCenterX + HalfWidth;
	const float WallHalf = 50.f * PixelSideCombat::LabWallScaleX;
	const FVector WallScale(PixelSideCombat::LabWallScaleX, PixelSideCombat::LabWallScaleY, PixelSideCombat::LabWallScaleZ);
	SpawnBlock(FVector(Left - WallHalf, 0.f, PixelSideCombat::LabWallCenterZ), WallScale, PixelSidePalette::Wall());
	SpawnBlock(FVector(Right + WallHalf, 0.f, PixelSideCombat::LabWallCenterZ), WallScale, PixelSidePalette::Wall());

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	World->SpawnActor<APixelDamageDummy>(
		APixelDamageDummy::StaticClass(),
		FVector(PixelSideCombat::LabDummyX, 0.f, PixelSideCombat::LabDummyZ),
		FRotator::ZeroRotator,
		Params);

	World->SpawnActor<APixelDebugHurtTurret>(
		APixelDebugHurtTurret::StaticClass(),
		FVector(PixelSideCombat::LabTurretX, 0.f, PixelSideCombat::LabTurretZ),
		FRotator::ZeroRotator,
		Params);
}

void APixelSideGameMode::SpawnBlock(const FVector& Location, const FVector& Scale, const FLinearColor& Color) const
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
	UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Block->SetActorScale3D(Scale);
	PixelSideMesh::Tint(Mesh, TEXT("/Engine/BasicShapes/Cube.Cube"), Color, true);
	Block->SetActorLocation(Location);
}
