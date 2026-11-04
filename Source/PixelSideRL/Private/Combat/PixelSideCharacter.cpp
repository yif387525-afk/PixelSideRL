#include "Combat/PixelSideCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/CombatDebugReadComponent.h"
#include "Combat/DamageableComponent.h"
#include "Combat/PixelSideCharacterMovement.h"
#include "Combat/PixelSideCombatLog.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/PixelSideMeshUtil.h"
#include "Combat/PixelSidePalette.h"
#include "Affix/AffixChoicePresenterComponent.h"
#include "Affix/AffixInventoryComponent.h"
#include "Affix/AffixSubsystemDefault.h"
#include "Combat/StatModifiersComponent.h"
#include "Combat/WeaponControllerComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "UI/PixelAffixChoiceWidget.h"
#include "UI/PixelDebugCombatHUD.h"
#include "UI/PixelHUDWidget.h"

APixelSideCharacter::APixelSideCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPixelSideCharacterMovement>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(PixelSideCombat::CapsuleRadius, PixelSideCombat::CapsuleHalfHeight);

	CoyoteTime = PixelSideCombat::CoyoteTime;
	JumpBuffer = PixelSideCombat::JumpBuffer;
	FacingSign = 1.f;
	ArtFacingMultiplier = 1.f;

	WeaponController = CreateDefaultSubobject<UWeaponControllerComponent>(TEXT("AC_WeaponController"));
	Damageable = CreateDefaultSubobject<UDamageableComponent>(TEXT("AC_Damageable"));
	Stats = CreateDefaultSubobject<UStatModifiersComponent>(TEXT("AC_StatModifiers"));
	AffixInventory = CreateDefaultSubobject<UAffixInventoryComponent>(TEXT("AC_AffixInventory"));
	AffixChoicePresenter = CreateDefaultSubobject<UAffixChoicePresenterComponent>(TEXT("AC_AffixChoicePresenter"));
	// Formal three-card shell. Text stand-in stays off so Lab does not draw both.
	// Clear AffixChoiceWidgetClass and set bEnableNativeChoiceUI to bring UAffixChoiceDebugWidget back.
	AffixChoicePresenter->bEnableNativeChoiceUI = false;
	AffixChoicePresenter->AffixChoiceWidgetClass = UPixelAffixChoiceWidget::StaticClass();
	HUDPresenter = CreateDefaultSubobject<UPixelHUDPresenterComponent>(TEXT("AC_HUDPresenter"));
	HUDPresenter->HUDWidgetClass = UPixelHUDWidget::StaticClass();
	CombatDebugRead = CreateDefaultSubobject<UCombatDebugReadComponent>(TEXT("AC_CombatDebugRead"));
#if UE_BUILD_SHIPPING
	bEnableCombatDebugHUD = false;
#endif

	Damageable->Team = ETeamId::Player;
	Damageable->MaxHealth = PixelSideCombat::PlayerMaxHealth;
	Damageable->bUsePlayerHurtProfile = true;
	Damageable->bAcceptsDamage = true;
	Damageable->IFrameDuration = PixelSideCombat::PlayerIFrameDuration;
	Damageable->PlayerKnockback = PixelSideCombat::PlayerHitKnockback;
	Damageable->PlayerHitStopDuration = PixelSideCombat::PlayerHitStop;
	Damageable->HitFlashTime = PixelSideCombat::HitFlashDuration;
	Damageable->MeshRestColor = PixelSidePalette::PlayerCloth();

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Placeholder"));
	PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
	PlaceholderMesh->ComponentTags.Add(TEXT("HitFlash"));
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlaceholderMesh->SetCastShadow(false);
	PlaceholderMesh->SetRelativeScale3D(FVector(0.20f, 0.10f, 0.32f));
	BasePlaceholderRelativeLocation = FVector(0.f, PixelSideCombat::VisualPlaneY, -PixelSideCombat::CapsuleHalfHeight + 16.f);
	PlaceholderMesh->SetRelativeLocation(BasePlaceholderRelativeLocation);

	if (UPaperFlipbookComponent* Sprite = GetSprite())
	{
		Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Sprite->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		BaseSpriteRelativeLocation = FVector(
			0.f,
			PixelSideCombat::VisualPlaneY,
			-PixelSideCombat::CapsuleHalfHeight + PixelSideCombat::SpriteFeetOffsetZ);
		Sprite->SetRelativeLocation(BaseSpriteRelativeLocation);
	}

	CameraAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("CameraAnchor"));
	CameraAnchor->SetupAttachment(GetCapsuleComponent());
	CameraAnchor->SetUsingAbsoluteLocation(true);
	CameraAnchor->SetUsingAbsoluteRotation(true);

	SideCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SideCamera"));
	SideCamera->SetupAttachment(CameraAnchor);
	SideCamera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	SideCamera->SetOrthoWidth(PixelSideCombat::OrthoWidth);
	SideCamera->bConstrainAspectRatio = true;
	SideCamera->AspectRatio = PixelSideCombat::OrthoWidth / PixelSideCombat::OrthoHeight;
	SideCamera->OrthoNearClipPlane = 0.f;
	SideCamera->OrthoFarClipPlane = 5000.f;
	SideCamera->SetRelativeLocation(FVector(0.f, PixelSideCombat::CameraArmLength, 0.f));
	SideCamera->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	SideCamera->bUsePawnControlRotation = false;
}

#if !UE_BUILD_SHIPPING
static void PixelSideDebugGrantAffixCmd(const TArray<FString>& Args)
{
	const FName AffixId(*(Args.Num() > 0 ? Args[0] : FString(TEXT("RapidFire"))));
	int32 Granted = 0;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || !World->IsGameWorld())
			{
				continue;
			}
			APlayerController* PC = World->GetFirstPlayerController();
			APixelSideCharacter* Player = PC ? Cast<APixelSideCharacter>(PC->GetPawn()) : nullptr;
			if (!Player)
			{
				continue;
			}
			if (UAffixSubsystemDefault* Affix = UAffixSubsystemDefault::Get(Player))
			{
				Affix->DebugGrantAffix(AffixId);
				++Granted;
			}
		}
	}
	if (Granted == 0)
	{
		UE_LOG(LogPixelSideCombat, Warning, TEXT("PixelSide.DebugGrantAffix: no player with UAffixSubsystemDefault. Lab F6 grants RapidFire."));
	}
}

static FAutoConsoleCommand GPixelSideDebugGrantAffix(
	TEXT("PixelSide.DebugGrantAffix"),
	TEXT("PixelSide.DebugGrantAffix <AffixId=RapidFire>. Calls UAffixSubsystemDefault::DebugGrantAffix. Lab F6 is the same grant for RapidFire."),
	FConsoleCommandWithArgsDelegate::CreateStatic(&PixelSideDebugGrantAffixCmd));
#endif

void APixelSideCharacter::BeginPlay()
{
	// Before Super so a missing component is registered before AC_WeaponController::BeginPlay binds.
	EnsureStatModifiers();
	Super::BeginPlay();

	PixelSideMesh::Tint(PlaceholderMesh, TEXT("/Engine/BasicShapes/Cube.Cube"), PixelSidePalette::PlayerCloth(), PixelSideMesh::EMeshCollision::NoCollision);

	if (Stats && !bStatsBound)
	{
		// MoveSpeed / MaxHP only. Fire cooldown is the weapon's listener on the same component delegates.
		// Do not also bind UAffixSubsystem; 词条官 forwards those onto AC_StatModifiers.
		Stats->OnAffixChanged.AddDynamic(this, &APixelSideCharacter::HandleAffixChanged);
		Stats->OnStatsRebuilt.AddDynamic(this, &APixelSideCharacter::HandleStatsRebuilt);
		bStatsBound = true;
	}
	if (WeaponController)
	{
		WeaponController->BindStatRefresh();
		WeaponController->OnWeaponFired.RemoveDynamic(this, &APixelSideCharacter::HandleWeaponFired);
		WeaponController->OnWeaponFired.AddDynamic(this, &APixelSideCharacter::HandleWeaponFired);
	}

	BeginAffixRun();
	CacheStatBases();
	UpdateCamera();
	MountFormalHUD();
	MountCombatDebugHUD();
}

void APixelSideCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bAffixChoiceBound)
	{
		if (UAffixSubsystemDefault* Affix = UAffixSubsystemDefault::Get(this))
		{
			Affix->OnAffixChoiceOffered.RemoveDynamic(this, &APixelSideCharacter::HandleAffixChoiceOffered);
			Affix->OnAffixChoiceClosed.RemoveDynamic(this, &APixelSideCharacter::HandleAffixChoiceClosed);
		}
		bAffixChoiceBound = false;
	}
	if (CombatDebugHUD)
	{
		CombatDebugHUD->RemoveFromParent();
		CombatDebugHUD = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void APixelSideCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCoyoteAndJump(DeltaSeconds);
	UpdateFacingVisual(DeltaSeconds);
	UpdateAnimation();
	UpdateCamera();
}

void APixelSideCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		PC->SetViewTarget(this);
	}
	MountFormalHUD();
	MountCombatDebugHUD();
}

void APixelSideCharacter::PawnClientRestart()
{
	EnsureCombatInput();
	Super::PawnClientRestart();
	AddMappingContext();
	MountFormalHUD();
	MountCombatDebugHUD();
}

void APixelSideCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	EnsureCombatInput();

	UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Enhanced)
	{
		UE_LOG(LogPixelSideCombat, Error, TEXT("Player input is not Enhanced Input. DefaultInput.ini must set EnhancedPlayerInput."));
		return;
	}

	Enhanced->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APixelSideCharacter::HandleMove);
	Enhanced->BindAction(MoveAction, ETriggerEvent::Completed, this, &APixelSideCharacter::HandleMove);
	Enhanced->BindAction(JumpAction, ETriggerEvent::Started, this, &APixelSideCharacter::HandleJumpStarted);
	Enhanced->BindAction(JumpAction, ETriggerEvent::Completed, this, &APixelSideCharacter::HandleJumpCompleted);
	Enhanced->BindAction(FireAction, ETriggerEvent::Started, this, &APixelSideCharacter::HandleFireStarted);
	Enhanced->BindAction(FireAction, ETriggerEvent::Triggered, this, &APixelSideCharacter::HandleFireStarted);
	Enhanced->BindAction(FireAction, ETriggerEvent::Completed, this, &APixelSideCharacter::HandleFireCompleted);
#if !UE_BUILD_SHIPPING
	Enhanced->BindAction(DebugHurtAction, ETriggerEvent::Started, this, &APixelSideCharacter::HandleDebugHurt);
	Enhanced->BindAction(DebugGrantRapidFireAction, ETriggerEvent::Started, this, &APixelSideCharacter::HandleDebugGrantRapidFire);
	Enhanced->BindAction(DebugAffixChoiceAction, ETriggerEvent::Started, this, &APixelSideCharacter::HandleDebugAffixChoice);
#endif
}

bool APixelSideCharacter::ApplyDamage_Implementation(const FDamageSpec& Spec)
{
	return Damageable ? IDamageable::Execute_ApplyDamage(Damageable, Spec) : false;
}

void APixelSideCharacter::GetHealth_Implementation(float& CurrentHealth, float& MaxHealth)
{
	if (Damageable)
	{
		IDamageable::Execute_GetHealth(Damageable, CurrentHealth, MaxHealth);
		return;
	}
	CurrentHealth = 0.f;
	MaxHealth = 0.f;
}

bool APixelSideCharacter::IsAlive_Implementation()
{
	return Damageable ? IDamageable::Execute_IsAlive(Damageable) : false;
}

ETeamId APixelSideCharacter::GetTeamId_Implementation()
{
	return Damageable ? Damageable->GetTeam() : ETeamId::Player;
}

bool APixelSideCharacter::CanJumpInternal_Implementation() const
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move || bIsCrouched || JumpCurrentCount >= JumpMaxCount)
	{
		return false;
	}
	if (Move->IsMovingOnGround())
	{
		return true;
	}
	return bCoyoteArmed && CoyoteTimeRemaining > 0.f;
}

void APixelSideCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();
	bCoyoteArmed = false;
	CoyoteTimeRemaining = 0.f;
	bJumpConsumed = true;
	JumpBufferRemaining = 0.f;
}

void APixelSideCharacter::EnsureCombatInput()
{
	if (CombatMappingContext)
	{
		return;
	}

	MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move"));
	MoveAction->ValueType = EInputActionValueType::Axis1D;
	JumpAction = NewObject<UInputAction>(this, TEXT("IA_Jump"));
	JumpAction->ValueType = EInputActionValueType::Boolean;
	FireAction = NewObject<UInputAction>(this, TEXT("IA_Fire"));
	FireAction->ValueType = EInputActionValueType::Boolean;
	DebugHurtAction = NewObject<UInputAction>(this, TEXT("IA_DebugHurt"));
	DebugHurtAction->ValueType = EInputActionValueType::Boolean;

	CombatMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Combat"));
	CombatMappingContext->MapKey(MoveAction, EKeys::D);

	FEnhancedActionKeyMapping& MoveLeft = CombatMappingContext->MapKey(MoveAction, EKeys::A);
	UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(CombatMappingContext);
	MoveLeft.Modifiers.Add(Negate);
	CombatMappingContext->MapKey(MoveAction, EKeys::Gamepad_LeftX);

	CombatMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	CombatMappingContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
	CombatMappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
	CombatMappingContext->MapKey(FireAction, EKeys::Gamepad_RightTrigger);
#if !UE_BUILD_SHIPPING
	DebugGrantRapidFireAction = NewObject<UInputAction>(this, TEXT("IA_DebugGrantRapidFire"));
	DebugGrantRapidFireAction->ValueType = EInputActionValueType::Boolean;
	DebugAffixChoiceAction = NewObject<UInputAction>(this, TEXT("IA_DebugAffixChoice"));
	DebugAffixChoiceAction->ValueType = EInputActionValueType::Boolean;
	CombatMappingContext->MapKey(DebugHurtAction, EKeys::H);
	CombatMappingContext->MapKey(DebugGrantRapidFireAction, EKeys::F6);
	CombatMappingContext->MapKey(DebugAffixChoiceAction, EKeys::F7);
#endif
}

void APixelSideCharacter::MountFormalHUD()
{
	if (HUDPresenter)
	{
		HUDPresenter->EnsureMounted();
	}
}

void APixelSideCharacter::MountCombatDebugHUD()
{
#if UE_BUILD_SHIPPING
	return;
#else
	if (!bEnableCombatDebugHUD || CombatDebugHUD)
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	CombatDebugHUD = UPixelDebugCombatHUD::MountForPlayer(PC, this, CombatDebugHUDClass);
#endif
}

void APixelSideCharacter::AddMappingContext()
{
	EnsureCombatInput();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->GetLocalPlayer() || !CombatMappingContext)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(CombatMappingContext);
		Subsystem->AddMappingContext(CombatMappingContext, 0);
	}
}

void APixelSideCharacter::EnsureStatModifiers()
{
	if (!Stats)
	{
		Stats = FindComponentByClass<UStatModifiersComponent>();
	}
	if (Stats)
	{
		return;
	}

	Stats = NewObject<UStatModifiersComponent>(this, UStatModifiersComponent::StaticClass(), TEXT("AC_StatModifiers"));
	if (!Stats)
	{
		UE_LOG(LogPixelSideCombat, Warning, TEXT("Player %s has no AC_StatModifiers and one could not be created."), *GetName());
		return;
	}
	AddInstanceComponent(Stats);
	Stats->RegisterComponent();
	UE_LOG(LogPixelSideCombat, Log, TEXT("Created AC_StatModifiers on %s. Weapon BeginPlay pushes FireRate base 1."), *GetName());
}

void APixelSideCharacter::CacheStatBases()
{
	if (!Stats)
	{
		return;
	}

	const float AuthoredSpeed = GetCharacterMovement() ? GetCharacterMovement()->MaxWalkSpeed : PixelSideCombat::MaxWalkSpeed;
	Stats->SetBaseStat(EStatId::MoveSpeed, AuthoredSpeed);
	Stats->SetBaseStat(EStatId::MaxHP, Damageable ? Damageable->GetMaxHealth() : PixelSideCombat::PlayerMaxHealth);
	Stats->SetBaseStat(EStatId::FireRate, PixelSideCombat::FireRateBase);
	Stats->SetBaseStat(EStatId::ProjectileCount, 1.f);
	Stats->SetBaseStat(EStatId::Pierce, 0.f);
	RefreshFromStats();
}

void APixelSideCharacter::RefreshFromStats()
{
	if (!Stats)
	{
		return;
	}
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = Stats->GetStat(EStatId::MoveSpeed);
	}
	if (Damageable)
	{
		Damageable->SetMaxHealth(Stats->GetStat(EStatId::MaxHP), true);
	}
}

void APixelSideCharacter::BeginAffixRun()
{
	UAffixSubsystemDefault* Affix = UAffixSubsystemDefault::Get(this);
	if (!Affix)
	{
		return;
	}
	if (!bAffixChoiceBound)
	{
		Affix->OnAffixChoiceOffered.AddDynamic(this, &APixelSideCharacter::HandleAffixChoiceOffered);
		Affix->OnAffixChoiceClosed.AddDynamic(this, &APixelSideCharacter::HandleAffixChoiceClosed);
		bAffixChoiceBound = true;
	}
	if (!Affix->IsInRun())
	{
		Affix->BeginRun(FRunStartContext());
	}
}

bool APixelSideCharacter::IsAffixChoiceBlockingControl() const
{
	if (const UAffixSubsystemDefault* Affix = UAffixSubsystemDefault::Get(this))
	{
		return Affix->IsChoicePending();
	}
	return false;
}

void APixelSideCharacter::HandleMove(const FInputActionValue& Value)
{
	if (IsAffixChoiceBlockingControl())
	{
		return;
	}
	const float Axis = Value.Get<float>();
	AddMovementInput(FVector::XAxisVector, Axis);
	if (FMath::Abs(Axis) > PixelSideCombat::FacingInputDeadzone)
	{
		FacingSign = FMath::Sign(Axis);
	}
}

void APixelSideCharacter::HandleJumpStarted(const FInputActionValue& Value)
{
	if (IsAffixChoiceBlockingControl())
	{
		return;
	}
	bJumpHeld = true;
	bJumpConsumed = false;
	JumpBufferRemaining = JumpBuffer;
	UpdateCoyoteAndJump(0.f);
}

void APixelSideCharacter::HandleJumpCompleted(const FInputActionValue& Value)
{
	bJumpHeld = false;
	StopJumping();
	if (UPixelSideCharacterMovement* Move = Cast<UPixelSideCharacterMovement>(GetCharacterMovement()))
	{
		Move->ApplyJumpCut();
	}
}

void APixelSideCharacter::HandleFireStarted(const FInputActionValue& Value)
{
	if (IsAffixChoiceBlockingControl())
	{
		return;
	}
	if (WeaponController)
	{
		WeaponController->SetWantsToFire(true);
	}
}

void APixelSideCharacter::HandleFireCompleted(const FInputActionValue& Value)
{
	if (WeaponController)
	{
		WeaponController->SetWantsToFire(false);
	}
}

void APixelSideCharacter::HandleDebugHurt(const FInputActionValue& Value)
{
#if !UE_BUILD_SHIPPING
	if (!Damageable || !GetWorld())
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	const FVector MarkerLocation(Origin.X + FacingSign * 64.f, Origin.Y, Origin.Z);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATargetPoint* Marker = GetWorld()->SpawnActor<ATargetPoint>(ATargetPoint::StaticClass(), MarkerLocation, FRotator::ZeroRotator, Params);

	FDamageSpec Spec;
	Spec.Damage = PixelSideCombat::GruntDamage;
	Spec.Knockback = PixelSideCombat::PlayerHitKnockback;
	Spec.HitStopVictim = PixelSideCombat::PlayerHitStop;
	Spec.SourceActor = Marker;
	Spec.DamageTypeTag = FName(TEXT("Default"));
	IDamageable::Execute_ApplyDamage(Damageable, Spec);
	if (Marker)
	{
		Marker->SetLifeSpan(0.25f);
	}
#else
	(void)Value;
#endif
}

void APixelSideCharacter::HandleDebugGrantRapidFire(const FInputActionValue& Value)
{
#if !UE_BUILD_SHIPPING
	(void)Value;
	if (UAffixSubsystemDefault* Affix = UAffixSubsystemDefault::Get(this))
	{
		Affix->DebugGrantAffix(FName(TEXT("RapidFire")));
	}
#else
	(void)Value;
#endif
}

void APixelSideCharacter::HandleDebugAffixChoice(const FInputActionValue& Value)
{
#if !UE_BUILD_SHIPPING
	(void)Value;
	if (UAffixSubsystemDefault* Affix = UAffixSubsystemDefault::Get(this))
	{
		Affix->RequestAffixChoice(EAffixOfferSource::NormalRoom);
	}
#else
	(void)Value;
#endif
}

void APixelSideCharacter::HandleAffixChoiceOffered(const TArray<FAffixChoiceEntry>& Choices)
{
	(void)Choices;
	if (WeaponController)
	{
		WeaponController->SetWantsToFire(false);
	}
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
	}
}

void APixelSideCharacter::HandleAffixChoiceClosed(bool bConfirmed)
{
	(void)bConfirmed;
}

void APixelSideCharacter::HandleAffixChanged(FAffixChangeEvent Event)
{
	(void)Event;
	// Move / HP only. The weapon rescales fire cooldown on this same component event.
	RefreshFromStats();
}

void APixelSideCharacter::HandleStatsRebuilt()
{
	RefreshFromStats();
}

void APixelSideCharacter::HandleWeaponFired()
{
	RecoilRemaining = PixelSideCombat::FireRecoilDuration;
}

void APixelSideCharacter::UpdateCoyoteAndJump(float DeltaSeconds)
{
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bGrounded = Move && Move->IsMovingOnGround();
	if (bGrounded)
	{
		if (JumpLockFrames > 0)
		{
			--JumpLockFrames;
		}
		else
		{
			CoyoteTimeRemaining = CoyoteTime;
			bCoyoteArmed = true;
		}
	}
	else
	{
		JumpLockFrames = 0;
		if (bCoyoteArmed)
		{
			CoyoteTimeRemaining = FMath::Max(0.f, CoyoteTimeRemaining - DeltaSeconds);
			if (CoyoteTimeRemaining <= 0.f)
			{
				bCoyoteArmed = false;
			}
		}
	}

	if (JumpBufferRemaining > 0.f)
	{
		JumpBufferRemaining = FMath::Max(0.f, JumpBufferRemaining - DeltaSeconds);
	}

	const bool bIntent = bJumpHeld || JumpBufferRemaining > 0.f;
	if (!bJumpConsumed && bIntent && CanRequestFeelJump())
	{
		// Impulse now. A same-frame release calls StopJumping and would eat bPressedJump
		// before CharacterMovement sampled it. Height is cut in HandleJumpCompleted.
		// A buffered jump whose button is already up has no later release while rising,
		// so it launches at the cut speed. JumpBuffer itself stays 0.10s.
		if (UCharacterMovementComponent* MutableMove = GetCharacterMovement())
		{
			float LaunchZ = MutableMove->JumpZVelocity;
			if (!bJumpHeld)
			{
				LaunchZ = PixelSideCombat::ApplyJumpCutRisingZ(LaunchZ);
			}
			LaunchCharacter(FVector(0.f, 0.f, LaunchZ), false, true);
		}
		bJumpConsumed = true;
		JumpLockFrames = 3;
		JumpBufferRemaining = 0.f;
		OnJumped();
	}
}

bool APixelSideCharacter::CanRequestFeelJump() const
{
	if (const UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		if (Move->IsMovingOnGround())
		{
			return true;
		}
	}
	return bCoyoteArmed && CoyoteTimeRemaining > 0.f;
}

void APixelSideCharacter::UpdateFacingVisual(float DeltaSeconds)
{
	if (WeaponController)
	{
		WeaponController->SetFacingSign(FacingSign);
	}

	const float Flip = FacingSign * ArtFacingMultiplier;
	float RecoilAlpha = 0.f;
	if (RecoilRemaining > 0.f)
	{
		RecoilAlpha = RecoilRemaining / FMath::Max(PixelSideCombat::FireRecoilDuration, KINDA_SMALL_NUMBER);
		RecoilRemaining = FMath::Max(0.f, RecoilRemaining - DeltaSeconds);
	}
	const float RecoilX = -FacingSign * PixelSideCombat::FireRecoilPixels * RecoilAlpha;

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

void APixelSideCharacter::UpdateAnimation()
{
	UPaperFlipbookComponent* Sprite = GetSprite();
	if (!Sprite)
	{
		return;
	}

	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bGrounded = Move && Move->IsMovingOnGround();
	const bool bFiring = WeaponController && WeaponController->IsTriggerHeld();

	bool bShowHurt = false;
	if (HurtFlipbook && Damageable && GetWorld() && Damageable->GetLastDamageWorldTime() >= 0.f)
	{
		const float SinceHit = GetWorld()->GetTimeSeconds() - Damageable->GetLastDamageWorldTime();
		bShowHurt = SinceHit >= 0.f && SinceHit < Damageable->HitFlashTime;
	}

	UPaperFlipbook* Active = nullptr;
	bool bScrubJump = false;
	if (bShowHurt)
	{
		Active = HurtFlipbook;
		WarnFlipbookOnce(TEXT("Hurt"), HurtFlipbook, PixelSideCombat::HurtFrameCount, bWarnedHurtFrames);
	}
	else if (!bGrounded && JumpFlipbook)
	{
		Active = JumpFlipbook;
		bScrubJump = true;
		WarnFlipbookOnce(TEXT("Jump"), JumpFlipbook, PixelSideCombat::JumpFrameCount, bWarnedJumpFrames);
	}
	else if (bFiring && FireFlipbook)
	{
		Active = FireFlipbook;
		WarnFlipbookOnce(TEXT("Fire"), FireFlipbook, PixelSideCombat::FireFrameCount, bWarnedFireFrames);
	}
	else if (bGrounded && Move && FMath::Abs(Move->Velocity.X) > PixelSideCombat::RunSpeedThreshold && RunFlipbook)
	{
		Active = RunFlipbook;
		WarnFlipbookOnce(TEXT("Run"), RunFlipbook, PixelSideCombat::RunFrameCount, bWarnedRunFrames);
	}
	else if (IdleFlipbook)
	{
		Active = IdleFlipbook;
		WarnFlipbookOnce(TEXT("Idle"), IdleFlipbook, PixelSideCombat::IdleFrameCount, bWarnedIdleFrames);
	}

	if (bScrubJump && JumpFlipbook)
	{
		if (Sprite->GetFlipbook() != JumpFlipbook)
		{
			Sprite->SetFlipbook(JumpFlipbook);
			Sprite->SetLooping(false);
			Sprite->Stop();
		}
		const int32 Available = FMath::Max(JumpFlipbook->GetNumFrames(), 1);
		const float JumpZ = Move ? Move->JumpZVelocity : PixelSideCombat::JumpZVelocity;
		const int Frame = PixelSideCombat::SelectJumpFrameIndex(
			GetVelocity().Z,
			JumpZ,
			PixelSideCombat::MaxFallSpeed,
			PixelSideCombat::JumpApexSpeed);
		Sprite->SetPlaybackPositionInFrames(FMath::Clamp(Frame, 0, Available - 1), false);
	}
	else if (Active && Sprite->GetFlipbook() != Active)
	{
		Sprite->SetFlipbook(Active);
		Sprite->SetLooping(true);
		Sprite->PlayFromStart();
	}

	const bool bBlinkOff = Damageable && Damageable->IsBlinkHidden();
	Sprite->SetVisibility(Active != nullptr && !bBlinkOff);
	if (PlaceholderMesh)
	{
		PlaceholderMesh->SetVisibility(Active == nullptr && !bBlinkOff);
	}
}

void APixelSideCharacter::UpdateCamera()
{
	if (!CameraAnchor)
	{
		return;
	}
	const FVector Loc = GetActorLocation();
	CameraAnchor->SetWorldLocation(FVector(Loc.X, 0.f, PixelSideCombat::FixedCameraZ));
	CameraAnchor->SetWorldRotation(FRotator::ZeroRotator);
}

void APixelSideCharacter::WarnFlipbookOnce(const TCHAR* Label, const UPaperFlipbook* Flipbook, int32 ExpectedFrames, bool& bAlreadyWarned) const
{
	if (!Flipbook || bAlreadyWarned || Flipbook->GetNumFrames() == ExpectedFrames)
	{
		return;
	}
	bAlreadyWarned = true;
	UE_LOG(
		LogPixelSideCombat,
		Warning,
		TEXT("%s flipbook '%s' has %d frames; player v3 expects %d x %dx%d. Jump is 5 frames (rise/apex/fall), not the legacy 4-frame sheet."),
		Label,
		*Flipbook->GetName(),
		Flipbook->GetNumFrames(),
		ExpectedFrames,
		PixelSideCombat::FrameWidth,
		PixelSideCombat::FrameHeight);
}
