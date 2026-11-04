#include "Combat/PixelDebugHurtTurret.h"

#include "Combat/DamageableComponent.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/PixelSideMeshUtil.h"
#include "Combat/PixelSidePalette.h"
#include "Combat/ProjectilePea.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

APixelDebugHurtTurret::APixelDebugHurtTurret()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(Root);
	BodyMesh->SetRelativeScale3D(FVector(0.28f, 0.16f, 0.36f));

	Damageable = CreateDefaultSubobject<UDamageableComponent>(TEXT("AC_Damageable"));
	Damageable->Team = ETeamId::Enemy;
	Damageable->MaxHealth = PixelSideCombat::DamageDummyMaxHealth;
	Damageable->bAcceptsDamage = false;
	Damageable->bUsePlayerHurtProfile = false;
	Damageable->MeshRestColor = PixelSidePalette::EnemyWarning();

	Tags.Add(TEXT("DebugHurtTurret"));
}

void APixelDebugHurtTurret::BeginPlay()
{
	Super::BeginPlay();
	PixelSideMesh::Tint(BodyMesh, TEXT("/Engine/BasicShapes/Cube.Cube"), PixelSidePalette::EnemyWarning(), true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FireTimer,
			this,
			&APixelDebugHurtTurret::FireAtPlayer,
			PixelSideCombat::GruntFireInterval,
			true,
			PixelSideCombat::DebugTurretFirstShotDelay);
	}
}

void APixelDebugHurtTurret::FireAtPlayer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	float Direction = 1.f;
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		const float DeltaX = Player->GetActorLocation().X - GetActorLocation().X;
		if (!FMath::IsNearlyZero(DeltaX))
		{
			Direction = FMath::Sign(DeltaX);
		}
	}

	const FVector Muzzle = GetActorLocation() + FVector(Direction * 28.f, PixelSideCombat::VisualPlaneY, 8.f);
	const FVector ShotDir(Direction, 0.f, 0.f);

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = nullptr;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectilePea* Pea = World->SpawnActor<AProjectilePea>(AProjectilePea::StaticClass(), Muzzle, ShotDir.Rotation(), Params);
	if (!Pea)
	{
		return;
	}

	Pea->LaunchShot(
		ShotDir,
		PixelSideCombat::GruntProjectileSpeed,
		PixelSideCombat::PeaProjectileRadius,
		PixelSideCombat::GruntProjectileLife,
		PixelSideCombat::GruntDamage,
		PixelSideCombat::PlayerHitKnockback,
		PixelSideCombat::PlayerHitStop,
		0.f,
		this,
		ETeamId::Enemy);
}
