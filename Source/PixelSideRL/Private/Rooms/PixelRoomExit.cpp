#include "Rooms/PixelRoomExit.h"

#include "Rooms/PixelRoomDirector.h"
#include "Combat/PixelSideMeshUtil.h"
#include "Combat/PixelSidePalette.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

APixelRoomExit::APixelRoomExit()
{
	PrimaryActorTick.bCanEverTick = false;
	ExitId = FName(TEXT("Exit.Right"));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Passage = CreateDefaultSubobject<UBoxComponent>(TEXT("Passage"));
	Passage->SetupAttachment(Root);
	Passage->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Passage->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Passage->SetGenerateOverlapEvents(true);
	Passage->SetBoxExtent(FVector(10.f, 24.f, 40.f));

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	DoorMesh->SetupAttachment(Root);
	DoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DoorMesh->SetCastShadow(false);
}

void APixelRoomExit::RefreshVolume()
{
	FVector Offset = VolumeRelativeLocation;
	if (Offset.IsNearlyZero())
	{
		const bool bLeft = ExitId == FName(TEXT("Exit.Left"));
		// Left socket sits in WALL_L. Shift the trigger onto the room side of that wall,
		// short of the player anchor at x=40 (capsule radius 12), so spawn does not count as a backtrack.
		Offset = bLeft ? FVector(50.f, 0.f, 40.f) : FVector(0.f, 0.f, 48.f);
	}
	if (Passage)
	{
		Passage->SetRelativeLocation(Offset);
	}
	if (DoorMesh)
	{
		DoorMesh->SetRelativeLocation(FVector(0.f, 0.f, 48.f));
		DoorMesh->SetRelativeScale3D(FVector(0.28f, 0.40f, 0.96f));
	}
}

void APixelRoomExit::BeginPlay()
{
	Super::BeginPlay();
	Tags.Add(TEXT("PixelSideRoom"));
	Tags.Add(ExitId);
	RefreshVolume();
	PixelSideMesh::Tint(DoorMesh, TEXT("/Engine/BasicShapes/Cube.Cube"), PixelSidePalette::Wall(), PixelSideMesh::EMeshCollision::NoCollision);
	ApplyLock(!bRequireClear);
	if (Passage)
	{
		Passage->OnComponentBeginOverlap.AddDynamic(this, &APixelRoomExit::HandleBeginOverlap);
	}
}

void APixelRoomExit::ApplyLock(bool bFlowOpen)
{
	const bool bBlock = bRequireClear && !bFlowOpen;
	bPassageOpen = !bBlock;
	if (!DoorMesh)
	{
		return;
	}
	DoorMesh->SetVisibility(bBlock);
	DoorMesh->SetHiddenInGame(!bBlock);
	if (bBlock)
	{
		DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		DoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		DoorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void APixelRoomExit::HandleBeginOverlap(
	UPrimitiveComponent* Overlapped,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	(void)Overlapped;
	(void)OtherComp;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;

	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled() || !GetWorld())
	{
		return;
	}

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APixelRoomDirector::StaticClass(), Found);
	if (Found.Num() == 0)
	{
		return;
	}
	if (APixelRoomDirector* Director = Cast<APixelRoomDirector>(Found[0]))
	{
		Director->NotifyExitTouched(ExitId);
	}
}
