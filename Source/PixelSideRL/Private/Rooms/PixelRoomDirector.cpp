#include "Rooms/PixelRoomDirector.h"

#include "Affix/AffixSubsystem.h"
#include "Affix/AffixSubsystemDefault.h"
#include "Combat/Damageable.h"
#include "Combat/PixelSideCombatLog.h"
#include "Combat/PixelSideGameMode.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/PixelSideLabSandboxLayout.h"
#include "Combat/PixelSideMeshUtil.h"
#include "Combat/PixelSidePalette.h"
#include "Rooms/PixelRoomExit.h"
#include "Rooms/PixelSideRoomFlow.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Meta/MetaStringConv.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

struct FPixelRoomDirectorPrivate
{
	struct FLiveEnemy
	{
		std::string SpawnId;
		TWeakObjectPtr<APixelEnemyGrunt> Actor;
		bool bReportedDead = false;
	};

	PixelSideRoom::FRoomFlow Flow;
	std::vector<FLiveEnemy> Lives;
	std::vector<PixelSideRoom::FGeometryRow> Geometry;
	bool bUnlockBroadcast = false;
	bool bWavesBroadcast = false;
	bool bLoggedMissingAffix = false;
};

namespace
{
EPixelRoomPhase ToPhase(PixelSideRoom::EPhase Phase)
{
	switch (Phase)
	{
	case PixelSideRoom::EPhase::Arming:
		return EPixelRoomPhase::Arming;
	case PixelSideRoom::EPhase::Fighting:
		return EPixelRoomPhase::Fighting;
	case PixelSideRoom::EPhase::AwaitingAffix:
		return EPixelRoomPhase::AwaitingAffix;
	case PixelSideRoom::EPhase::Cleared:
		return EPixelRoomPhase::Cleared;
	default:
		return EPixelRoomPhase::Idle;
	}
}
}

APixelRoomDirector::APixelRoomDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	RoomId = FName(PixelSideRoom::RoomCombat01Id);
	FirstWaveDelay = PixelSideRoom::FirstWaveDelayDefaultSec;
	GruntEarlyClass = APixelEnemyGrunt::StaticClass();
	Priv = new FPixelRoomDirectorPrivate();
}

APixelRoomDirector::~APixelRoomDirector()
{
	delete Priv;
	Priv = nullptr;
}

void APixelRoomDirector::BeginPlay()
{
	Super::BeginPlay();
	BindAffix();
	if (bAutoStart)
	{
		StartRoom();
	}
}

void APixelRoomDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindAffix();
	Super::EndPlay(EndPlayReason);
}

void APixelRoomDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Priv || !bRoomStarted)
	{
		return;
	}
	if (bPlacePlayerAtAnchor && !bPlayerPlaced)
	{
		PlacePlayer();
	}
	Priv->Flow.Tick(DeltaSeconds);
	PollDeaths();
	SpawnQueued();
	OfferAffix();
	Publish();
}

void APixelRoomDirector::StartRoom()
{
	if (!Priv)
	{
		return;
	}
	if (bRoomStarted)
	{
		Priv->Flow.NotifyEntered();
		Publish();
		return;
	}

	const std::string RoomName = PixelSideMetaUE::ToUtf8(RoomId.ToString());
	if (!PixelSideRoom::IsP07AcceptanceRoom(RoomName))
	{
		UE_LOG(LogPixelSideCombat, Warning, TEXT("P0.7 director drives Room_Combat_01 only (got %s)."), *RoomId.ToString());
		bRoomStarted = true;
		return;
	}

	const FString Stem = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Rooms"), RoomId.ToString());
	FString MetaText;
	FString SpawnText;
	if (!FFileHelper::LoadFileToString(MetaText, *(Stem + TEXT("_meta.csv")))
		|| !FFileHelper::LoadFileToString(SpawnText, *(Stem + TEXT("_spawns.csv"))))
	{
		UE_LOG(LogPixelSideCombat, Error, TEXT("Room_Combat_01 CSV missing under %s"), *Stem);
		bRoomStarted = true;
		return;
	}

	std::string Error;
	if (!Priv->Flow.Load(PixelSideMetaUE::ToUtf8(MetaText), PixelSideMetaUE::ToUtf8(SpawnText), PixelSideRoom::RoomCombat01Id, Error))
	{
		UE_LOG(LogPixelSideCombat, Error, TEXT("Room_Combat_01 CSV: %s"), *PixelSideMetaUE::Utf8ToString(Error));
		bRoomStarted = true;
		return;
	}

	Priv->Flow.SetFirstWaveDelay(FirstWaveDelay);
	FirstWaveDelay = Priv->Flow.FirstWaveDelay();

	if (bBuildGreyboxFromCsv)
	{
		FString GeoText;
		if (!FFileHelper::LoadFileToString(GeoText, *(Stem + TEXT("_geometry.csv"))))
		{
			UE_LOG(LogPixelSideCombat, Error, TEXT("Room_Combat_01 geometry CSV missing."));
		}
		else if (!PixelSideRoom::LoadGeometryCsv(PixelSideMetaUE::ToUtf8(GeoText), Priv->Geometry, Error))
		{
			UE_LOG(LogPixelSideCombat, Error, TEXT("Room_Combat_01 geometry: %s"), *PixelSideMetaUE::Utf8ToString(Error));
		}
		else
		{
			BuildGreybox();
		}
	}

	BindAffix();
	bRoomStarted = true;
	if (bPlacePlayerAtAnchor)
	{
		PlacePlayer();
	}
	Priv->Flow.NotifyEntered();
	Publish();
	UE_LOG(
		LogPixelSideCombat,
		Log,
		TEXT("Room_Combat_01 armed. First wave in %.2fs. Clear calls RequestAffixChoice(NormalRoom); Exit.Right opens after one confirm."),
		FirstWaveDelay);
}

void APixelRoomDirector::NotifyExitTouched(FName InExitId)
{
	if (!Priv || !bRoomStarted || InExitId.IsNone() || !GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (InExitId == LastTouchId && Now - LastTouchTime < 0.5f)
	{
		return;
	}
	LastTouchId = InExitId;
	LastTouchTime = Now;

	const PixelSideRoom::EExitTouch Touch = Priv->Flow.NotifyExitUsed(PixelSideMetaUE::ToUtf8(InExitId.ToString()));
	switch (Touch)
	{
	case PixelSideRoom::EExitTouch::DeniedLocked:
		UE_LOG(LogPixelSideCombat, Log, TEXT("Exit %s stays shut until the room is clear and the affix card is confirmed."), *InExitId.ToString());
		break;
	case PixelSideRoom::EExitTouch::BacktrackKeptClear:
		UE_LOG(LogPixelSideCombat, Log, TEXT("Exit.Left while Room_Combat_01 is clear. Waves stay down. Exit.Right stays as it is."));
		break;
	case PixelSideRoom::EExitTouch::Passed:
		UE_LOG(LogPixelSideCombat, Log, TEXT("Exit %s used."), *InExitId.ToString());
		break;
	default:
		break;
	}
	Publish();
}

void APixelRoomDirector::HandleAffixChoiceClosed(bool bConfirmed)
{
	if (!Priv || !Priv->Flow.IsAwaitingAffix())
	{
		return;
	}
	Priv->Flow.NotifyChoiceClosed(bConfirmed);
	if (!bConfirmed)
	{
		UE_LOG(LogPixelSideCombat, Log, TEXT("Room_Combat_01 affix closed with no card. Exit.Right stays shut until a card is confirmed."));
	}
	Publish();
}

void APixelRoomDirector::BindAffix()
{
	if (bAffixBound)
	{
		return;
	}
	UAffixSubsystem* Affix = ResolveAffix();
	if (!Affix)
	{
		return;
	}
	Affix->OnAffixChoiceClosed.AddDynamic(this, &APixelRoomDirector::HandleAffixChoiceClosed);
	BoundAffix = Affix;
	bAffixBound = true;
}

void APixelRoomDirector::UnbindAffix()
{
	if (!bAffixBound || !BoundAffix)
	{
		bAffixBound = false;
		BoundAffix = nullptr;
		return;
	}
	BoundAffix->OnAffixChoiceClosed.RemoveDynamic(this, &APixelRoomDirector::HandleAffixChoiceClosed);
	BoundAffix = nullptr;
	bAffixBound = false;
}

UAffixSubsystem* APixelRoomDirector::ResolveAffix() const
{
	return UAffixSubsystemDefault::Get(this);
}

void APixelRoomDirector::PollDeaths()
{
	for (FPixelRoomDirectorPrivate::FLiveEnemy& Live : Priv->Lives)
	{
		if (Live.bReportedDead)
		{
			continue;
		}
		APixelEnemyGrunt* Grunt = Live.Actor.Get();
		const bool bDead = !Grunt || !IDamageable::Execute_IsAlive(Grunt);
		if (!bDead)
		{
			continue;
		}
		Live.bReportedDead = true;
		Priv->Flow.NotifyEnemyDied(Live.SpawnId);
	}
}

void APixelRoomDirector::SpawnQueued()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const std::vector<PixelSideRoom::FSpawnOrder> Orders = Priv->Flow.ConsumeSpawnOrders();
	for (const PixelSideRoom::FSpawnOrder& Order : Orders)
	{
		if (Order.Kind != PixelSideRoom::EEnemyKind::GruntEarly)
		{
			UE_LOG(LogPixelSideCombat, Warning, TEXT("Room_Combat_01 has no class for %s"), *PixelSideMetaUE::Utf8ToString(Order.EnemyId));
			Priv->Flow.NotifySpawnFailed(Order.SpawnId);
			continue;
		}

		UClass* Class = GruntEarlyClass ? *GruntEarlyClass : APixelEnemyGrunt::StaticClass();
		const FVector Location = GetActorLocation() + FVector(
			Order.X,
			0.f,
			Order.Y + PixelSideCombat::EnemyGruntCapsuleHalfHeight + static_cast<float>(PixelSideLabLayout::SpawnClearance));

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		APixelEnemyGrunt* Grunt = World->SpawnActor<APixelEnemyGrunt>(Class, Location, FRotator::ZeroRotator, Params);
		if (!Grunt)
		{
			UE_LOG(LogPixelSideCombat, Warning, TEXT("Room_Combat_01 failed to spawn %s"), *PixelSideMetaUE::Utf8ToString(Order.SpawnId));
			Priv->Flow.NotifySpawnFailed(Order.SpawnId);
			continue;
		}

		Grunt->SetInitialFacingSign(Order.FacingSign);
		Grunt->Tags.Add(TEXT("PixelSideRoom"));
		Grunt->Tags.Add(TEXT("Room_Combat_01"));
		Grunt->Tags.Add(FName(*PixelSideMetaUE::Utf8ToString(Order.SpawnId)));
		Grunt->Tags.Add(FName(*PixelSideMetaUE::Utf8ToString(Order.EnemyId)));

		FPixelRoomDirectorPrivate::FLiveEnemy Live;
		Live.SpawnId = Order.SpawnId;
		Live.Actor = Grunt;
		Priv->Lives.push_back(std::move(Live));
		Priv->Flow.NotifySpawned(Order.SpawnId);
		UE_LOG(
			LogPixelSideCombat,
			Log,
			TEXT("Room_Combat_01 W%d %s at (%.0f, %.0f) facing %s"),
			Order.WaveGroup,
			*PixelSideMetaUE::Utf8ToString(Order.SpawnId),
			Order.X,
			Order.Y,
			*PixelSideMetaUE::Utf8ToString(Order.Facing));
	}
}

void APixelRoomDirector::OfferAffix()
{
	if (!Priv->Flow.WantsAffixOffer())
	{
		return;
	}
	BindAffix();
	UAffixSubsystem* Affix = BoundAffix ? BoundAffix : ResolveAffix();
	if (!Affix)
	{
		if (!Priv->bLoggedMissingAffix)
		{
			Priv->bLoggedMissingAffix = true;
			UE_LOG(LogPixelSideCombat, Warning, TEXT("Room_Combat_01 is clear and UAffixSubsystem is missing. Exit.Right stays shut."));
		}
		return;
	}
	Affix->RequestAffixChoice(EAffixOfferSource::NormalRoom);
	if (!Affix->IsChoicePending())
	{
		UE_LOG(LogPixelSideCombat, Warning, TEXT("Room_Combat_01 clear did not open a choice. Exit.Right stays shut."));
		return;
	}
	Priv->Flow.MarkAffixRequested();
	UE_LOG(LogPixelSideCombat, Log, TEXT("Room_Combat_01 waves cleared. RequestAffixChoice(NormalRoom). Exit.Right stays shut until confirm."));
}

void APixelRoomDirector::Publish()
{
	if (!Priv)
	{
		return;
	}

	Phase = ToPhase(Priv->Flow.Phase());
	AliveCount = Priv->Flow.Alive();
	CurrentWave = Priv->Flow.CurrentWaveGroup();
	bRoomCleared = Priv->Flow.IsCleared();
	bRequireClearExitOpen = Priv->Flow.IsExitOpen(PixelSideRoom::ExitRightId);

	if (UWorld* World = GetWorld())
	{
		TArray<AActor*> Exits;
		UGameplayStatics::GetAllActorsOfClass(World, APixelRoomExit::StaticClass(), Exits);
		for (AActor* Actor : Exits)
		{
			APixelRoomExit* Exit = Cast<APixelRoomExit>(Actor);
			if (!Exit)
			{
				continue;
			}
			Exit->ApplyLock(Priv->Flow.IsExitOpen(PixelSideMetaUE::ToUtf8(Exit->ExitId.ToString())));
		}
	}

	const bool bWavesDone = Priv->Flow.IsAwaitingAffix() || Priv->Flow.IsCleared();
	if (bWavesDone && !Priv->bWavesBroadcast)
	{
		Priv->bWavesBroadcast = true;
		OnRoomWavesCleared.Broadcast();
	}
	if (Priv->Flow.IsCleared() && !Priv->bUnlockBroadcast)
	{
		Priv->bUnlockBroadcast = true;
		OnRequireClearExitUnlocked.Broadcast();
		UE_LOG(LogPixelSideCombat, Log, TEXT("Room_Combat_01 Exit.Right is open."));
	}
}

void APixelRoomDirector::PlacePlayer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	float X = PixelSideRoom::RoomCombat01PlayerStartX;
	float Y = PixelSideRoom::RoomCombat01PlayerStartY;
	PixelSideRoom::FindAnchor(Priv->Geometry, "PlayerStart", X, Y);

	ACharacter* Player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	if (!Player)
	{
		return;
	}
	const UCapsuleComponent* Capsule = Player->GetCapsuleComponent();
	const float Half = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : PixelSideCombat::CapsuleHalfHeight;
	const FVector Dest = GetActorLocation() + FVector(X, 0.f, Y + Half + static_cast<float>(PixelSideLabLayout::SpawnClearance));

	TArray<AActor*> Starts;
	UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), Starts);
	if (Starts.Num() > 0)
	{
		Starts[0]->SetActorLocation(Dest);
	}

	Player->SetActorLocation(Dest, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Move = Player->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetMovementMode(MOVE_Walking);
	}
	bPlayerPlaced = true;
}

void APixelRoomDirector::BuildGreybox()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Same cube / slab depth as the lab greybox so shots meet the floor. Thickness is presentation.
	// JumpZ and gravity stay on the feel table.
	constexpr float CubeSize = static_cast<float>(PixelSideLabLayout::CubeSize);
	constexpr float SlabDepth = static_cast<float>(PixelSideLabLayout::SlabDepth);
	constexpr float GroundThickness = static_cast<float>(PixelSideLabLayout::GroundThickness);
	constexpr float PlatformThickness = static_cast<float>(PixelSideLabLayout::PlatformThickness);
	const FVector Origin = GetActorLocation();
	const FLinearColor FloorColor = PixelSidePalette::Floor();
	const FLinearColor WallColor = PixelSidePalette::Wall();

	auto SpawnBlock = [&](const FVector& Center, float SizeX, float SizeZ, const FLinearColor& Color, const TCHAR* LayoutId, bool bLetPawnsPass)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Block = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Center, FRotator::ZeroRotator, Params);
		if (!Block)
		{
			return;
		}
		Block->Tags.Add(TEXT("PixelSideRoom"));
		if (LayoutId)
		{
			Block->Tags.Add(LayoutId);
#if WITH_EDITOR
			Block->SetActorLabel(FString(LayoutId));
#endif
		}
		UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Block->SetActorScale3D(FVector(SizeX / CubeSize, SlabDepth / CubeSize, SizeZ / CubeSize));
		const PixelSideMesh::EMeshCollision Collision = bLetPawnsPass
			? PixelSideMesh::EMeshCollision::PassThroughPawn
			: PixelSideMesh::EMeshCollision::BlockAll;
		PixelSideMesh::Tint(Mesh, TEXT("/Engine/BasicShapes/Cube.Cube"), Color, Collision);
		Block->SetActorLocation(Center);
	};

	for (const PixelSideRoom::FGeometryRow& Row : Priv->Geometry)
	{
		const FString Id = PixelSideMetaUE::Utf8ToString(Row.Id);
		if (Row.Kind == "Floor")
		{
			const float SizeX = FMath::Abs(Row.X1 - Row.X0);
			const float CenterX = (Row.X0 + Row.X1) * 0.5f;
			const FVector Center = Origin + FVector(CenterX, 0.f, Row.TopY - GroundThickness * 0.5f);
			SpawnBlock(Center, SizeX, GroundThickness, FloorColor, *Id, false);
		}
		else if (Row.Kind == "Wall")
		{
			const float SizeX = FMath::Max(1.f, FMath::Abs(Row.X1 - Row.X0));
			const float CenterX = (Row.X0 + Row.X1) * 0.5f;
			const float WallHeight = FMath::Max(Row.TopY, 1.f);
			const float Total = WallHeight + GroundThickness;
			const float CenterZ = (WallHeight - GroundThickness) * 0.5f;
			SpawnBlock(Origin + FVector(CenterX, 0.f, CenterZ), SizeX, Total, WallColor, *Id, false);
		}
		else if (Row.Kind == "Platform")
		{
			const float Width = FMath::Max(Row.Width, 1.f);
			const FVector Center = Origin + FVector(Row.XCenter, 0.f, Row.TopY - PlatformThickness * 0.5f);
			SpawnBlock(Center, Width, PlatformThickness, FloorColor, *Id, false);
		}
		else if (Row.Kind == "Cover")
		{
			const float Width = FMath::Max(Row.Width, 1.f);
			const float Height = FMath::Max(Row.Height, 1.f);
			const FVector Center = Origin + FVector(Row.XCenter, 0.f, Row.TopY + Height * 0.5f);
			SpawnBlock(Center, Width, Height, WallColor, *Id, true);
		}
		else if (Row.Kind == "Exit")
		{
			const std::string Tag = PixelSideRoom::ExitTagOf(Row);
			if (Tag.empty())
			{
				continue;
			}
			const bool bRequire = Tag == Priv->Flow.Meta().RequireClearExit;
			const FVector Socket = Origin + FVector(Row.X0, 0.f, Row.X1);
			const bool bLeft = Tag == PixelSideRoom::ExitLeftId;
			const FVector Offset = bLeft ? FVector(50.f, 0.f, 40.f) : FVector(0.f, 0.f, 48.f);

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			APixelRoomExit* Exit = World->SpawnActorDeferred<APixelRoomExit>(
				APixelRoomExit::StaticClass(),
				FTransform(FRotator::ZeroRotator, Socket),
				this,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Exit)
			{
				continue;
			}
			Exit->ExitId = FName(*PixelSideMetaUE::Utf8ToString(Tag));
			Exit->bRequireClear = bRequire;
			Exit->VolumeRelativeLocation = Offset;
			Exit->FinishSpawning(FTransform(FRotator::ZeroRotator, Socket));
#if WITH_EDITOR
			Exit->SetActorLabel(PixelSideMetaUE::Utf8ToString(Tag));
#endif
		}
	}

	// Empty-map drive still gets Lab's side sun. A directional already in the level is retargeted.
	APixelSideGameMode::EnsureSideSun(World, TEXT("PixelSideRoom"));
}
