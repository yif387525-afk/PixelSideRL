#include "Combat/PixelEnemyGrunt.h"

#include "Combat/DamageableComponent.h"
#include "Combat/PixelSideCharacterMovement.h"
#include "Combat/PixelSideCombatLog.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/PixelSideMeshUtil.h"
#include "Combat/PixelSidePalette.h"
#include "Combat/PixelSideSpriteSheet.h"
#include "Combat/ProjectilePea.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"

namespace
{
void ApplyGruntPawnPass(UCapsuleComponent* Capsule)
{
	if (!Capsule)
	{
		return;
	}
	// Same order as the damage dummy. Profile last would put the Pawn block back
	// and a chasing grunt would soft-lock the route.
	Capsule->SetCollisionProfileName(TEXT("Pawn"));
	PixelSideMesh::LetPawnsPass(Capsule);
}

UPaperFlipbook* LoadGruntSheet(UObject* Outer, const TCHAR* FileName, int32 FrameCount, const TCHAR* DebugName)
{
	const FString Relative = FString(TEXT("Art/Enemies/EnemyGrunt/")) + FileName;
	return PixelSideSprites::LoadHorizontalFlipbook(
		Outer,
		*Relative,
		PixelSideCombat::EnemyGruntFrameWidth,
		PixelSideCombat::EnemyGruntFrameHeight,
		FrameCount,
		PixelSideCombat::PixelAnimFps,
		DebugName);
}
}

APixelEnemyGrunt::APixelEnemyGrunt(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPixelSideCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;
	bUseControllerRotationYaw = false;
	JumpMaxCount = 0;
	FacingSign = -1.f;
	ArtFacingMultiplier = 1.f;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(PixelSideCombat::EnemyGruntCapsuleRadius, PixelSideCombat::EnemyGruntCapsuleHalfHeight);
	ApplyGruntPawnPass(Capsule);

	Damageable = CreateDefaultSubobject<UDamageableComponent>(TEXT("AC_Damageable"));
	Damageable->Team = ETeamId::Enemy;
	Damageable->MaxHealth = PixelSideCombat::EnemyGruntMaxHealth;
	Damageable->bUsePlayerHurtProfile = false;
	Damageable->bAcceptsDamage = true;
	Damageable->HitFlashTime = PixelSideCombat::HitFlashDuration;
	Damageable->MeshRestColor = PixelSidePalette::EnemyWarning();

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder"));
	PlaceholderMesh->SetupAttachment(Capsule);
	PlaceholderMesh->ComponentTags.Add(TEXT("HitFlash"));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetCastShadow(false);
	PlaceholderMesh->SetRelativeScale3D(FVector(0.18f, 0.10f, 0.28f));
	BasePlaceholderRelativeLocation = FVector(0.f, PixelSideCombat::VisualPlaneY, -PixelSideCombat::EnemyGruntCapsuleHalfHeight + 14.f);
	PlaceholderMesh->SetRelativeLocation(BasePlaceholderRelativeLocation);

	if (UPaperFlipbookComponent* Sprite = GetSprite())
	{
		Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Sprite->SetCastShadow(false);
		Sprite->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		BaseSpriteRelativeLocation = FVector(0.f, PixelSideCombat::VisualPlaneY, -PixelSideCombat::EnemyGruntCapsuleHalfHeight);
		Sprite->SetRelativeLocation(BaseSpriteRelativeLocation);
	}

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = PixelSideCombat::EnemyGruntMoveSpeed;
	}

	Tags.Add(TEXT("EnemyGrunt"));
}

void APixelEnemyGrunt::SetInitialFacingSign(float Sign)
{
	if (FMath::IsNearlyZero(Sign))
	{
		return;
	}
	FacingSign = (Sign < 0.f) ? -1.f : 1.f;
}

void APixelEnemyGrunt::BeginPlay()
{
	Super::BeginPlay();
	ApplyGruntPawnPass(GetCapsuleComponent());
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = PixelSideCombat::EnemyGruntMoveSpeed;
	}
	PixelSideMesh::Tint(PlaceholderMesh, TEXT("/Engine/BasicShapes/Cube.Cube"), PixelSidePalette::EnemyWarning(), PixelSideMesh::EMeshCollision::NoCollision);
	EnsureFlipbooks();
	UE_LOG(
		LogPixelSideCombat,
		Log,
		TEXT("Grunt ready. Speed %.0f, aggro %.0f, shot %.1fs / %.0f. Lab turret is a different row."),
		PixelSideCombat::EnemyGruntMoveSpeed,
		PixelSideCombat::EnemyGruntAggroRange,
		PixelSideCombat::EnemyGruntFireInterval,
		PixelSideCombat::EnemyGruntDamage);
}

void APixelEnemyGrunt::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCombat(DeltaSeconds);
	UpdateFacingVisual(DeltaSeconds);
	UpdateAnimation();
}

bool APixelEnemyGrunt::ApplyDamage_Implementation(const FDamageSpec& Spec)
{
	return Damageable ? IDamageable::Execute_ApplyDamage(Damageable, Spec) : false;
}

void APixelEnemyGrunt::GetHealth_Implementation(float& CurrentHealth, float& MaxHealth)
{
	if (Damageable)
	{
		IDamageable::Execute_GetHealth(Damageable, CurrentHealth, MaxHealth);
		return;
	}
	CurrentHealth = 0.f;
	MaxHealth = 0.f;
}

bool APixelEnemyGrunt::IsAlive_Implementation()
{
	return Damageable ? IDamageable::Execute_IsAlive(Damageable) : false;
}

ETeamId APixelEnemyGrunt::GetTeamId_Implementation()
{
	return Damageable ? Damageable->GetTeam() : ETeamId::Enemy;
}

bool APixelEnemyGrunt::IsLiving() const
{
	return Damageable && Damageable->GetCurrentHealth() > 0.f;
}

void APixelEnemyGrunt::EnsureFlipbooks()
{
	if (!IdleFlipbook)
	{
		IdleFlipbook = LoadGruntSheet(this, TEXT("T_EnemyGrunt_Idle.png"), PixelSideCombat::EnemyGruntIdleFrameCount, TEXT("PF_EnemyGrunt_Idle"));
	}
	if (!RunFlipbook)
	{
		RunFlipbook = LoadGruntSheet(this, TEXT("T_EnemyGrunt_Run.png"), PixelSideCombat::EnemyGruntRunFrameCount, TEXT("PF_EnemyGrunt_Run"));
	}
	if (!FireFlipbook)
	{
		FireFlipbook = LoadGruntSheet(this, TEXT("T_EnemyGrunt_Fire.png"), PixelSideCombat::EnemyGruntFireFrameCount, TEXT("PF_EnemyGrunt_Fire"));
	}
	if (!HurtFlipbook)
	{
		HurtFlipbook = LoadGruntSheet(this, TEXT("T_EnemyGrunt_Hurt.png"), PixelSideCombat::EnemyGruntHurtFrameCount, TEXT("PF_EnemyGrunt_Hurt"));
	}

	if (UPaperFlipbookComponent* Sprite = GetSprite())
	{
		if (IdleFlipbook)
		{
			Sprite->SetFlipbook(IdleFlipbook);
			Sprite->SetLooping(true);
			Sprite->PlayFromStart();
		}
		// The capsule is the hit volume. A flipbook with baked collision must not block the route.
		Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void APixelEnemyGrunt::UpdateCombat(float DeltaSeconds)
{
	if (FireAnimRemaining > 0.f)
	{
		FireAnimRemaining = FMath::Max(0.f, FireAnimRemaining - DeltaSeconds);
	}
	if (!IsLiving())
	{
		bAlert = false;
		return;
	}

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		bAlert = false;
		return;
	}

	const float DeltaX = Player->GetActorLocation().X - GetActorLocation().X;
	const bool bWasAlert = bAlert;
	bAlert = PixelSideCombat::EnemyGruntWantsAlert(bAlert, DeltaX);
	if (bAlert && !bWasAlert)
	{
		// Interval is the gap between shots. Crossing aggro fires the first one.
		TimeUntilFire = 0.f;
	}
	if (!bAlert)
	{
		return;
	}

	const float Chase = PixelSideCombat::EnemyGruntChaseSign(DeltaX, PixelSideCombat::EnemyGruntChaseSlack);
	if (Chase != 0.f)
	{
		FacingSign = Chase;
		AddMovementInput(FVector(Chase, 0.f, 0.f), 1.f);
	}

	TimeUntilFire -= DeltaSeconds;
	if (TimeUntilFire <= 0.f)
	{
		FireHorizontal();
		TimeUntilFire = PixelSideCombat::EnemyGruntFireInterval;
	}
}

void APixelEnemyGrunt::FireHorizontal()
{
	UWorld* World = GetWorld();
	if (!World || !IsLiving())
	{
		return;
	}

	const float Direction = (FacingSign < 0.f) ? -1.f : 1.f;
	const FVector Muzzle = GetActorLocation() + FVector(
		Direction * PixelSideCombat::EnemyGruntMuzzleLocalX(),
		PixelSideCombat::VisualPlaneY,
		-PixelSideCombat::EnemyGruntCapsuleHalfHeight + PixelSideCombat::EnemyGruntMuzzleLocalZ());
	const FVector ShotDir(Direction, 0.f, 0.f);

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectilePea* Pea = World->SpawnActor<AProjectilePea>(AProjectilePea::StaticClass(), Muzzle, ShotDir.Rotation(), Params);
	if (!Pea)
	{
		return;
	}

	Pea->LaunchShot(
		ShotDir,
		PixelSideCombat::EnemyGruntProjectileSpeed,
		PixelSideCombat::EnemyGruntProjectileRadius,
		PixelSideCombat::EnemyGruntProjectileLife,
		PixelSideCombat::EnemyGruntDamage,
		PixelSideCombat::PlayerHitKnockback,
		PixelSideCombat::PlayerHitStop,
		0.f,
		this,
		ETeamId::Enemy);

	FireAnimRemaining = static_cast<float>(PixelSideCombat::EnemyGruntFireFrameCount) / PixelSideCombat::PixelAnimFps;
	RecoilRemaining = PixelSideCombat::FireRecoilDuration;
}

void APixelEnemyGrunt::UpdateFacingVisual(float DeltaSeconds)
{
	float RecoilAlpha = 0.f;
	if (RecoilRemaining > 0.f)
	{
		RecoilAlpha = RecoilRemaining / FMath::Max(PixelSideCombat::FireRecoilDuration, KINDA_SMALL_NUMBER);
		RecoilRemaining = FMath::Max(0.f, RecoilRemaining - DeltaSeconds);
	}
	const float RecoilX = -FacingSign * PixelSideCombat::FireRecoilPixels * RecoilAlpha;
	const float Flip = FacingSign * ArtFacingMultiplier;

	if (UPaperFlipbookComponent* Sprite = GetSprite())
	{
		Sprite->SetRelativeScale3D(FVector(1.f, Flip, 1.f));
		Sprite->SetRelativeLocation(BaseSpriteRelativeLocation + FVector(RecoilX, 0.f, 0.f));
	}
	if (PlaceholderMesh)
	{
		PlaceholderMesh->SetRelativeLocation(BasePlaceholderRelativeLocation + FVector(RecoilX, 0.f, 0.f));
	}
}

void APixelEnemyGrunt::UpdateAnimation()
{
	UPaperFlipbookComponent* Sprite = GetSprite();
	if (!Sprite)
	{
		return;
	}

	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bAlive = IsLiving();
	bool bShowHurt = !bAlive;
	if (!bShowHurt && HurtFlipbook && Damageable && GetWorld() && Damageable->GetLastDamageWorldTime() >= 0.f)
	{
		const float SinceHit = GetWorld()->GetTimeSeconds() - Damageable->GetLastDamageWorldTime();
		const float HurtWindow = static_cast<float>(PixelSideCombat::EnemyGruntHurtFrameCount) / PixelSideCombat::PixelAnimFps;
		bShowHurt = SinceHit >= 0.f && SinceHit < HurtWindow;
	}

	UPaperFlipbook* Active = nullptr;
	if (!bAlive)
	{
		Active = HurtFlipbook;
	}
	else if (bShowHurt && HurtFlipbook)
	{
		Active = HurtFlipbook;
	}
	else if (FireAnimRemaining > 0.f && FireFlipbook)
	{
		Active = FireFlipbook;
	}
	else if (bAlive && Move && FMath::Abs(Move->Velocity.X) > PixelSideCombat::RunSpeedThreshold && RunFlipbook)
	{
		Active = RunFlipbook;
	}
	else if (IdleFlipbook)
	{
		Active = IdleFlipbook;
	}

	if (Active && Sprite->GetFlipbook() != Active)
	{
		Sprite->SetFlipbook(Active);
		Sprite->SetLooping(bAlive);
		Sprite->PlayFromStart();
		Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else if (!bAlive)
	{
		Sprite->SetLooping(false);
	}

	const bool bHasArt = Active != nullptr;
	Sprite->SetVisibility(bHasArt);
	if (PlaceholderMesh)
	{
		PlaceholderMesh->SetVisibility(!bHasArt);
	}
}
