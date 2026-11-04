#include "Combat/DamageableComponent.h"

#include "Combat/PixelSideCharacterMovement.h"
#include "Combat/PixelSideCombatLog.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/PixelSidePalette.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PaperFlipbookComponent.h"
#include "TimerManager.h"

UDamageableComponent::UDamageableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(false);
	MaxHealth = PixelSideCombat::PlayerMaxHealth;
	Health = MaxHealth;
	IFrameDuration = PixelSideCombat::PlayerIFrameDuration;
	PlayerKnockback = PixelSideCombat::PlayerHitKnockback;
	PlayerHitStopDuration = PixelSideCombat::PlayerHitStop;
	HitFlashTime = PixelSideCombat::HitFlashDuration;
	MeshRestColor = FLinearColor::White;
}

void UDamageableComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	CacheFlashTargets();
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UDamageableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitStopTimer);
	}
	EndHitStop();
	RestoreFlash();
	Super::EndPlay(EndPlayReason);
}

void UDamageableComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float WorldDt = GetWorld() ? GetWorld()->GetDeltaSeconds() : DeltaTime;
	if (IFrameRemaining > 0.f)
	{
		IFrameRemaining = FMath::Max(0.f, IFrameRemaining - WorldDt);
	}
	if (FlashRemaining > 0.f)
	{
		FlashRemaining = FMath::Max(0.f, FlashRemaining - WorldDt);
		if (FlashRemaining <= 0.f)
		{
			RestoreFlash();
		}
	}
}

bool UDamageableComponent::ApplyDamage_Implementation(const FDamageSpec& Spec)
{
	if (!bAcceptsDamage || bDead || !GetOwner())
	{
		return Health > 0.f && !bDead;
	}

	if (bUsePlayerHurtProfile && IFrameRemaining > 0.f)
	{
		return true;
	}

	const ETeamId SourceTeam = ResolveActorTeam(Spec.SourceActor);
	if (SourceTeam == Team && Team != ETeamId::Neutral)
	{
		return Health > 0.f;
	}

	const float Applied = FMath::Max(0.f, Spec.Damage);
	Health = FMath::Max(0.f, Health - Applied);
	LastDamageTaken = Applied;
	LastDamageWorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	const float KnockbackDistance = bUsePlayerHurtProfile ? PlayerKnockback : Spec.Knockback;
	const float HitStop = bUsePlayerHurtProfile ? PlayerHitStopDuration : Spec.HitStopVictim;

	float Direction = 1.f;
	if (Spec.SourceActor)
	{
		const float DeltaX = GetOwner()->GetActorLocation().X - Spec.SourceActor->GetActorLocation().X;
		if (!FMath::IsNearlyZero(DeltaX))
		{
			Direction = FMath::Sign(DeltaX);
		}
	}

	if (bUsePlayerHurtProfile)
	{
		IFrameRemaining = IFrameDuration;
	}

	StartFlash();
	ApplyKnockback(Direction * KnockbackDistance);
	BeginHitStop(HitStop);

	if (Health <= 0.f)
	{
		bDead = true;
		Health = 0.f;
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
			{
				Move->DisableMovement();
			}
		}
		UE_LOG(LogPixelSideCombat, Log, TEXT("%s died"), *GetOwner()->GetName());
	}

	OnHealthChanged.Broadcast(Health, MaxHealth);
	return Health > 0.f;
}

void UDamageableComponent::GetHealth_Implementation(float& CurrentHealth, float& MaxHealthOut)
{
	CurrentHealth = Health;
	MaxHealthOut = MaxHealth;
}

bool UDamageableComponent::IsAlive_Implementation()
{
	return Health > 0.f && !bDead;
}

ETeamId UDamageableComponent::GetTeamId_Implementation()
{
	return Team;
}

bool UDamageableComponent::IsBlinkHidden() const
{
	if (!bUsePlayerHurtProfile || IFrameRemaining <= 0.f || !GetWorld())
	{
		return false;
	}
	const float Phase = FMath::Fmod(GetWorld()->GetTimeSeconds() * PixelSideCombat::IFrameBlinkHz, 1.f);
	return Phase >= 0.5f;
}

bool UDamageableComponent::ApplyEventHealth(float NewCurrentHealth)
{
	if (bDead || Health <= 0.f)
	{
		return false;
	}
	const float Upper = FMath::Max(1.f, MaxHealth);
	const float Clamped = FMath::Clamp(NewCurrentHealth, 1.f, Upper);
	if (FMath::IsNearlyEqual(Health, Clamped))
	{
		return true;
	}
	Health = Clamped;
	OnHealthChanged.Broadcast(Health, MaxHealth);
	return true;
}

void UDamageableComponent::SetMaxHealth(float NewMax, bool bKeepRatio)
{
	NewMax = FMath::Max(1.f, NewMax);
	if (bKeepRatio && MaxHealth > KINDA_SMALL_NUMBER)
	{
		const float Ratio = Health / MaxHealth;
		MaxHealth = NewMax;
		Health = Ratio * MaxHealth;
	}
	else
	{
		MaxHealth = NewMax;
		Health = FMath::Clamp(Health, 0.f, MaxHealth);
	}
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

ETeamId UDamageableComponent::ResolveActorTeam(const AActor* Actor)
{
	if (!Actor)
	{
		return ETeamId::Neutral;
	}
	if (const UDamageableComponent* Comp = Actor->FindComponentByClass<UDamageableComponent>())
	{
		return Comp->Team;
	}
	if (Actor->GetClass()->ImplementsInterface(UDamageable::StaticClass()))
	{
		return IDamageable::Execute_GetTeamId(const_cast<AActor*>(Actor));
	}
	if (const AActor* InstigatorActor = Actor->GetInstigator())
	{
		if (InstigatorActor != Actor)
		{
			return ResolveActorTeam(InstigatorActor);
		}
	}
	return ETeamId::Neutral;
}

void UDamageableComponent::ApplyKnockback(float SignedDistance)
{
	if (FMath::IsNearlyZero(SignedDistance) || !GetOwner())
	{
		return;
	}

	const bool bAir = !IsOwnerGrounded();
	const float Distance = PixelSideCombat::KnockbackDistance(FMath::Abs(SignedDistance), bAir);
	const float Speed = PixelSideCombat::KnockbackInitialSpeed(Distance, PixelSideCombat::KnockbackSlideDuration);
	const float SignedSpeed = FMath::Sign(SignedDistance) * Speed;

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UPixelSideCharacterMovement* Move = Cast<UPixelSideCharacterMovement>(Character->GetCharacterMovement()))
		{
			Move->StartKnockback(SignedSpeed, PixelSideCombat::KnockbackSlideDuration);
			return;
		}
	}
}

bool UDamageableComponent::IsOwnerGrounded() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr;
	return Move && Move->IsMovingOnGround();
}

void UDamageableComponent::BeginHitStop(float Seconds)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World || Seconds <= 0.f)
	{
		return;
	}

	FTimerManager& Timers = World->GetTimerManager();
	if (!Timers.IsTimerActive(HitStopTimer))
	{
		SavedTimeDilation = Owner->CustomTimeDilation;
	}
	Owner->CustomTimeDilation = PixelSideCombat::HitStopDilation;
	Timers.SetTimer(HitStopTimer, this, &UDamageableComponent::EndHitStop, Seconds, false);
}

void UDamageableComponent::EndHitStop()
{
	if (AActor* Owner = GetOwner())
	{
		Owner->CustomTimeDilation = SavedTimeDilation;
	}
}

void UDamageableComponent::StartFlash()
{
	CacheFlashTargets();
	FlashRemaining = HitFlashTime;
	const FLinearColor Flash = PixelSidePalette::HitFlash();
	if (UPaperFlipbookComponent* Sprite = SpriteVisual.Get())
	{
		Sprite->SetSpriteColor(Flash);
	}
	for (const TWeakObjectPtr<UMaterialInstanceDynamic>& MidPtr : FlashMids)
	{
		if (UMaterialInstanceDynamic* Mid = MidPtr.Get())
		{
			Mid->SetVectorParameterValue(TEXT("Color"), Flash);
		}
	}
}

void UDamageableComponent::RestoreFlash()
{
	if (UPaperFlipbookComponent* Sprite = SpriteVisual.Get())
	{
		Sprite->SetSpriteColor(BaseSpriteColor);
	}
	for (const TWeakObjectPtr<UMaterialInstanceDynamic>& MidPtr : FlashMids)
	{
		if (UMaterialInstanceDynamic* Mid = MidPtr.Get())
		{
			Mid->SetVectorParameterValue(TEXT("Color"), MeshRestColor);
		}
	}
}

void UDamageableComponent::CacheFlashTargets()
{
	SpriteVisual = nullptr;
	FlashMids.Reset();
	if (!GetOwner())
	{
		return;
	}

	TInlineComponentArray<UMeshComponent*> Meshes;
	GetOwner()->GetComponents(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (UPaperFlipbookComponent* Sprite = Cast<UPaperFlipbookComponent>(Mesh))
		{
			SpriteVisual = Sprite;
			if (FlashRemaining <= 0.f)
			{
				BaseSpriteColor = Sprite->GetSpriteColor();
			}
		}
		if (!Mesh->ComponentHasTag(TEXT("HitFlash")))
		{
			continue;
		}
		if (UMaterialInstanceDynamic* Mid = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)))
		{
			FlashMids.Add(Mid);
		}
	}
}
