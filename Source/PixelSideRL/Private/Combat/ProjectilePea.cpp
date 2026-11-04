#include "Combat/ProjectilePea.h"

#include "Combat/DamageableComponent.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/PixelSidePalette.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AProjectilePea::AProjectilePea()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(PixelSideCombat::PeaProjectileRadius);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AProjectilePea::OnSphereOverlap);
	Collision->OnComponentHit.AddDynamic(this, &AProjectilePea::OnSphereHit);

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetCastShadow(false);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 0.f;
	Movement->MaxSpeed = PixelSideCombat::PeaProjectileSpeed;
	Movement->ProjectileGravityScale = 0.f;
	Movement->bRotationFollowsVelocity = false;
	Movement->bShouldBounce = false;
	Movement->bSweepCollision = true;
	Movement->bForceSubStepping = true;
	Movement->MaxSimulationTimeStep = 0.008f;

	InitialLifeSpan = PixelSideCombat::PeaProjectileLife;
	ShotDamage = PixelSideCombat::PeaDamage;
	ShotKnockback = PixelSideCombat::PeaKnockback;
	ShotHitStopVictim = PixelSideCombat::PeaHitStopEnemy;
}

void AProjectilePea::BeginPlay()
{
	Super::BeginPlay();

	if (UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
	{
		Visual->SetStaticMesh(Sphere);
	}
	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		VisualMid = UMaterialInstanceDynamic::Create(BaseMat, this);
		Visual->SetMaterial(0, VisualMid);
	}
	ApplyRadius(PixelSideCombat::PeaProjectileRadius);
	ApplyTeamColor();
}

void AProjectilePea::LaunchShot(
	const FVector& Direction,
	float Speed,
	float Radius,
	float LifeSeconds,
	float InDamage,
	float InKnockback,
	float InHitStopVictim,
	float InHitStopInstigator,
	AActor* InSource,
	ETeamId InTeam)
{
	const FVector Dir = Direction.GetSafeNormal();
	const FVector SafeDir = Dir.IsNearlyZero() ? FVector::XAxisVector : Dir;
	Team = InTeam;
	DamageSource = InSource;
	ShotDirection = SafeDir;
	ShotSpeed = Speed;
	ShotDamage = InDamage;
	ShotKnockback = InKnockback;
	ShotHitStopVictim = InHitStopVictim;
	ShotHitStopInstigator = InHitStopInstigator;

	ApplyRadius(Radius);
	ApplyTeamColor();

	if (AActor* OwnerActor = GetOwner())
	{
		Collision->IgnoreActorWhenMoving(OwnerActor, true);
	}
	if (APawn* InstigatorPawn = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(InstigatorPawn, true);
	}
	IgnoreOwnerUntilTime = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f) + PixelSideCombat::ProjectileIgnoreOwnerSeconds;

	Movement->InitialSpeed = Speed;
	Movement->MaxSpeed = Speed;
	Movement->Velocity = SafeDir * Speed;
	Movement->UpdateComponentVelocity();
	Movement->Activate(true);
	SetLifeSpan(FMath::Max(LifeSeconds, 0.05f));
	SetActorRotation(SafeDir.Rotation());
}

void AProjectilePea::OnSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	HandleActor(OtherActor, &SweepResult, false);
}

void AProjectilePea::OnSphereHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	HandleActor(OtherActor, &Hit, true);
}

void AProjectilePea::HandleActor(AActor* OtherActor, const FHitResult* Hit, bool bBlockingHit)
{
	if (!OtherActor || OtherActor == this || bImpacted)
	{
		return;
	}
	if (IsOwnerIgnored(OtherActor))
	{
		return;
	}

	UDamageableComponent* Target = OtherActor->FindComponentByClass<UDamageableComponent>();
	const bool bInterface = OtherActor->GetClass()->ImplementsInterface(UDamageable::StaticClass());
	if (Target || bInterface)
	{
		if (IsFriendly(OtherActor))
		{
			// Friendly fire is off: do not stop the bolt on a teammate.
			Collision->IgnoreActorWhenMoving(OtherActor, true);
			if (Movement)
			{
				Movement->Velocity = ShotDirection * ShotSpeed;
				Movement->Activate(true);
			}
			return;
		}
		if (!PassHitGate(OtherActor))
		{
			return;
		}

		const FDamageSpec Spec = BuildSpec();
		IDamageable::Execute_ApplyDamage(Target ? static_cast<UObject*>(Target) : OtherActor, Spec);
		Destroy();
		return;
	}

	if (bBlockingHit)
	{
		bImpacted = true;
		Destroy();
	}
}

bool AProjectilePea::IsOwnerIgnored(const AActor* OtherActor) const
{
	if (!OtherActor)
	{
		return false;
	}
	const bool bIsOwner = OtherActor == GetOwner() || OtherActor == GetInstigator() || OtherActor == DamageSource;
	if (!bIsOwner)
	{
		return false;
	}
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	return Now < IgnoreOwnerUntilTime;
}

bool AProjectilePea::IsFriendly(const AActor* OtherActor) const
{
	const ETeamId OtherTeam = UDamageableComponent::ResolveActorTeam(OtherActor);
	return OtherTeam == Team && Team != ETeamId::Neutral;
}

bool AProjectilePea::PassHitGate(AActor* OtherActor)
{
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (bImpacted && (Now - LastImpactTime) < PixelSideCombat::SameTargetHitInterval)
	{
		return false;
	}
	bImpacted = true;
	LastImpactTime = Now;
	return OtherActor != nullptr;
}

FDamageSpec AProjectilePea::BuildSpec() const
{
	FDamageSpec Spec;
	Spec.Damage = ShotDamage;
	Spec.Knockback = ShotKnockback;
	Spec.HitStopVictim = ShotHitStopVictim;
	Spec.HitStopInstigator = ShotHitStopInstigator;
	Spec.SourceActor = DamageSource ? DamageSource.Get() : GetInstigator();
	Spec.InstigatorController = GetInstigatorController();
	Spec.DamageTypeTag = FName(TEXT("Default"));
	return Spec;
}

void AProjectilePea::ApplyRadius(float Radius)
{
	const float SafeRadius = FMath::Max(Radius, 0.5f);
	Collision->SetSphereRadius(SafeRadius);
	// Engine sphere mesh radius is 50 uu.
	const float Scale = SafeRadius / 50.f;
	Visual->SetRelativeScale3D(FVector(Scale));
}

void AProjectilePea::ApplyTeamColor()
{
	if (!VisualMid)
	{
		return;
	}
	const FLinearColor Color = (Team == ETeamId::Enemy)
		? PixelSidePalette::ProjectileEnemy()
		: PixelSidePalette::ProjectilePlayer();
	VisualMid->SetVectorParameterValue(TEXT("Color"), Color);
}
