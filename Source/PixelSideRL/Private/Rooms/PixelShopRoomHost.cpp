#include "Rooms/PixelShopRoomHost.h"

#include "Combat/PixelSideCharacter.h"
#include "Combat/PixelSideCombatLog.h"
#include "Combat/PixelSideCombatMath.h"
#include "Combat/PixelSideLabSandboxLayout.h"
#include "Combat/PixelSideMeshUtil.h"
#include "Combat/PixelSidePalette.h"
#include "Rooms/PixelRoomExit.h"
#include "Rooms/PixelSideRoomFlow.h"
#include "UI/PixelShopPresenterComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Meta/MetaStringConv.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

struct FPixelShopRoomHostPrivate
{
	std::vector<PixelSideRoom::FGeometryRow> Geometry;
	PixelSideRoom::FShopEntry Entry;
};

APixelShopRoomHost::APixelShopRoomHost()
{
	PrimaryActorTick.bCanEverTick = true;
	RoomId = FName(PixelSideRoom::RoomShop01Id);
	Priv = new FPixelShopRoomHostPrivate();
}

APixelShopRoomHost::~APixelShopRoomHost()
{
	delete Priv;
	Priv = nullptr;
}

void APixelShopRoomHost::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoOpen)
	{
		LoadAndEnter();
	}
}

void APixelShopRoomHost::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bOpenedShop)
	{
		if (UWorld* World = GetWorld())
		{
			if (APixelSideCharacter* Player = Cast<APixelSideCharacter>(UGameplayStatics::GetPlayerPawn(World, 0)))
			{
				if (UPixelShopPresenterComponent* Presenter = Player->GetShopPresenter())
				{
					Presenter->CloseShop();
				}
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

void APixelShopRoomHost::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	(void)DeltaSeconds;
	if (!bEntered || bOpenedShop || TryOpenShop())
	{
		SetActorTickEnabled(false);
	}
}

bool APixelShopRoomHost::LoadAndEnter()
{
	if (!Priv || bEntered)
	{
		return bEntered;
	}

	const FString Stem = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Rooms"), RoomId.ToString());
	FString MetaText;
	if (!FFileHelper::LoadFileToString(MetaText, *(Stem + TEXT("_meta.csv"))))
	{
		UE_LOG(LogPixelSideCombat, Error, TEXT("Shop room CSV missing under %s"), *Stem);
		return false;
	}

	PixelSideRoom::FRoomMeta Meta;
	std::string Error;
	if (!PixelSideRoom::LoadRoomMetaCsv(PixelSideMetaUE::ToUtf8(MetaText), Meta, Error))
	{
		UE_LOG(LogPixelSideCombat, Error, TEXT("Shop room meta: %s"), *PixelSideMetaUE::Utf8ToString(Error));
		return false;
	}

	Priv->Entry = PixelSideRoom::MakeShopEntry(Meta);
	if (!Priv->Entry.bOpenRunShop)
	{
		UE_LOG(LogPixelSideCombat, Warning, TEXT("ShopRoomHost refuses %s (type %s). Combat director is unchanged."),
			*PixelSideMetaUE::Utf8ToString(Meta.RoomId), *PixelSideMetaUE::Utf8ToString(Meta.Type));
		return false;
	}

	if (bBuildGreyboxFromCsv)
	{
		FString GeoText;
		if (!FFileHelper::LoadFileToString(GeoText, *(Stem + TEXT("_geometry.csv"))))
		{
			UE_LOG(LogPixelSideCombat, Error, TEXT("Shop room geometry CSV missing."));
		}
		else if (!PixelSideRoom::LoadGeometryCsv(PixelSideMetaUE::ToUtf8(GeoText), Priv->Geometry, Error))
		{
			UE_LOG(LogPixelSideCombat, Error, TEXT("Shop room geometry: %s"), *PixelSideMetaUE::Utf8ToString(Error));
			Priv->Geometry.clear();
		}
		else
		{
			UWorld* World = GetWorld();
			if (World)
			{
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
					Block->Tags.Add(TEXT("PixelSideShop"));
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
						float X0 = 0.f;
						float X1 = 0.f;
						PixelSideRoom::ResolveHorizontalSpan(Row, X0, X1);
						const float SizeX = FMath::Max(1.f, FMath::Abs(X1 - X0));
						const float CenterX = (X0 + X1) * 0.5f;
						SpawnBlock(Origin + FVector(CenterX, 0.f, Row.TopY - GroundThickness * 0.5f), SizeX, GroundThickness, FloorColor, *Id, false);
					}
					else if (Row.Kind == "Wall")
					{
						float X0 = 0.f;
						float X1 = 0.f;
						PixelSideRoom::ResolveHorizontalSpan(Row, X0, X1);
						const float SizeX = FMath::Max(1.f, FMath::Abs(X1 - X0));
						const float CenterX = (X0 + X1) * 0.5f;
						const float WallHeight = FMath::Max(Row.TopY, 1.f);
						const float Total = WallHeight + GroundThickness;
						const float CenterZ = (WallHeight - GroundThickness) * 0.5f;
						SpawnBlock(Origin + FVector(CenterX, 0.f, CenterZ), SizeX, Total, WallColor, *Id, false);
					}
					else if (Row.Kind == "Platform")
					{
						float X0 = 0.f;
						float X1 = 0.f;
						PixelSideRoom::ResolveHorizontalSpan(Row, X0, X1);
						const float SizeX = FMath::Max(1.f, FMath::Abs(X1 - X0));
						const float CenterX = (X0 + X1) * 0.5f;
						SpawnBlock(Origin + FVector(CenterX, 0.f, Row.TopY - PlatformThickness * 0.5f), SizeX, PlatformThickness, FloorColor, *Id, false);
					}
					else if (Row.Kind == "Cover")
					{
						float X0 = 0.f;
						float X1 = 0.f;
						PixelSideRoom::ResolveHorizontalSpan(Row, X0, X1);
						const float SizeX = FMath::Max(1.f, FMath::Abs(X1 - X0));
						const float CenterX = (X0 + X1) * 0.5f;
						const float Height = FMath::Max(Row.Height, 1.f);
						SpawnBlock(Origin + FVector(CenterX, 0.f, Row.TopY + Height * 0.5f), SizeX, Height, WallColor, *Id, true);
					}
					else if (Row.Kind == "Exit")
					{
						const std::string Tag = PixelSideRoom::ExitTagOf(Row);
						if (Tag.empty())
						{
							continue;
						}
						const FVector Socket = Origin + FVector(Row.X0, 0.f, Row.X1);
						const bool bLeft = Tag == PixelSideRoom::ExitLeftId;
						const FVector Offset = bLeft ? FVector(50.f, 0.f, 40.f) : FVector(0.f, 0.f, 48.f);
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
						Exit->bRequireClear = Priv->Entry.bLockDoors;
						Exit->VolumeRelativeLocation = Offset;
						Exit->FinishSpawning(FTransform(FRotator::ZeroRotator, Socket));
						Exit->Tags.Add(TEXT("PixelSideShop"));
#if WITH_EDITOR
						Exit->SetActorLabel(PixelSideMetaUE::Utf8ToString(Tag));
#endif
					}
				}
			}
		}
	}

	bEntered = true;
	if (!TryOpenShop())
	{
		UE_LOG(LogPixelSideCombat, Log, TEXT("Room_Shop_01 is in. Shop shell waits for the player pawn."));
	}
	return true;
}

bool APixelShopRoomHost::TryOpenShop()
{
	if (!Priv || !bEntered || bOpenedShop)
	{
		return bOpenedShop;
	}
	if (++OpenAttempts > 120)
	{
		UE_LOG(LogPixelSideCombat, Warning, TEXT("Room_Shop_01 did not reach AC_ShopPresenter. Doors stay unlocked."));
		bOpenedShop = true;
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	APixelSideCharacter* Player = Cast<APixelSideCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	if (!Player)
	{
		return false;
	}

	if (bPlacePlayerAtAnchor && !bPlayerPlaced)
	{
		float X = PixelSideRoom::RoomShop01PlayerStartX;
		float Y = PixelSideRoom::RoomShop01PlayerStartY;
		PixelSideRoom::FindAnchor(Priv->Geometry, "PlayerStart", X, Y);
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

	UPixelShopPresenterComponent* Presenter = Player->GetShopPresenter();
	if (!Presenter)
	{
		return false;
	}
	TArray<FPixelShopSlotOffer> Offers;
	TArray<FPixelShopSlotOffer> Pool;
	UPixelShopWidget::CollectShelfOffers(Priv->Entry.ShelfLayer, Offers, Pool);
	// Scrap is not the balance. OpenShop reads GetRunScrap and spends with AddRunScrap.
	if (!Presenter->OpenShop(Offers, 0.0, true))
	{
		return false;
	}

	bOpenedShop = true;
	UE_LOG(LogPixelSideCombat, Log,
		TEXT("Room_Shop_01 OpenShop layer=%d via AC_ShopPresenter. Run wallet. Doors unlocked. No forced buy."),
		Priv->Entry.ShelfLayer);
	return true;
}
