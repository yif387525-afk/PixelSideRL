#include "Combat/PixelDamageDummy.h"

#include "Combat/DamageableComponent.h"
#include "Combat/PixelSideCharacterMovement.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/PixelSideMeshUtil.h"
#include "Combat/PixelSidePalette.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
void ApplyDummyPawnPass(UCapsuleComponent* Capsule)
{
	if (!Capsule)
	{
		return;
	}
	// Profile first: object type stays Pawn, WorldStatic stays Block (the post stands),
	// and a pea still overlaps this capsule. Then drop only Pawn-vs-Pawn. Profile last
	// would put the block back and soft-lock the route.
	Capsule->SetCollisionProfileName(TEXT("Pawn"));
	PixelSideMesh::LetPawnsPass(Capsule);
}
}

APixelDamageDummy::APixelDamageDummy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPixelSideCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	AutoPossessAI = EAutoPossessAI::Disabled;
	bUseControllerRotationYaw = false;
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(PixelSideCombat::CapsuleRadius, PixelSideCombat::CapsuleHalfHeight);
	// Capsule stays up (~38 uu). The live jump is ~32, so blocking Pawns soft-locks the route.
	// Body mesh stays NoCollision; do not disable the capsule.
	ApplyDummyPawnPass(Capsule);

	Damageable = CreateDefaultSubobject<UDamageableComponent>(TEXT("AC_Damageable"));
	Damageable->Team = ETeamId::Enemy;
	Damageable->MaxHealth = PixelSideCombat::DamageDummyMaxHealth;
	Damageable->bUsePlayerHurtProfile = false;
	Damageable->bAcceptsDamage = true;
	Damageable->HitFlashTime = PixelSideCombat::HitFlashDuration;
	Damageable->MeshRestColor = PixelSidePalette::EnemyWarning();

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->ComponentTags.Add(TEXT("HitFlash"));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetCastShadow(false);
	BodyMesh->SetRelativeScale3D(FVector(0.28f, 0.12f, 0.36f));
	BodyMesh->SetRelativeLocation(FVector(0.f, PixelSideCombat::VisualPlaneY, -PixelSideCombat::CapsuleHalfHeight + 18.f));

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = 0.f;
	}

	Tags.Add(TEXT("DamageDummy"));
}

bool APixelDamageDummy::ApplyDamage_Implementation(const FDamageSpec& Spec)
{
	return Damageable ? IDamageable::Execute_ApplyDamage(Damageable, Spec) : false;
}

void APixelDamageDummy::GetHealth_Implementation(float& CurrentHealth, float& MaxHealth)
{
	if (Damageable)
	{
		IDamageable::Execute_GetHealth(Damageable, CurrentHealth, MaxHealth);
		return;
	}
	CurrentHealth = 0.f;
	MaxHealth = 0.f;
}

bool APixelDamageDummy::IsAlive_Implementation()
{
	return Damageable ? IDamageable::Execute_IsAlive(Damageable) : false;
}

ETeamId APixelDamageDummy::GetTeamId_Implementation()
{
	return Damageable ? Damageable->GetTeam() : ETeamId::Enemy;
}

void APixelDamageDummy::BeginPlay()
{
	Super::BeginPlay();
	// Registration can reload the Pawn profile and put the block back.
	ApplyDummyPawnPass(GetCapsuleComponent());
	PixelSideMesh::Tint(BodyMesh, TEXT("/Engine/BasicShapes/Cube.Cube"), PixelSidePalette::EnemyWarning(), PixelSideMesh::EMeshCollision::NoCollision);
}
