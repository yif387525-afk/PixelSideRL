#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace PixelSideMesh
{
/** How a tinted basic-shape should collide. On a pair, Ignore wins over Block. */
enum class EMeshCollision : uint8
{
	/** Visual only. */
	NoCollision,
	/** WorldStatic BlockAll. Floors, walls, platforms. */
	BlockAll,
	/**
	 * WorldStatic. WorldDynamic and Visibility stay Block, so shots and LOS stop.
	 * Pawn and Camera are Ignore, so the player walks through.
	 */
	PassThroughPawn,
};

/** Drop Pawn blocking and leave every other channel on this body as it already is. */
inline void LetPawnsPass(UPrimitiveComponent* Prim)
{
	if (!Prim)
	{
		return;
	}
	Prim->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
}

inline void Tint(UStaticMeshComponent* Mesh, const TCHAR* MeshPath, const FLinearColor& Color, EMeshCollision Collision)
{
	if (!Mesh)
	{
		return;
	}

	if (UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, MeshPath))
	{
		Mesh->SetStaticMesh(Asset);
	}

	Mesh->SetCastShadow(false);
	if (Collision == EMeshCollision::NoCollision)
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else if (Collision == EMeshCollision::BlockAll)
	{
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		// Do not assign the BlockAll profile: that profile blocks Pawns.
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionObjectType(ECC_WorldStatic);
		Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		LetPawnsPass(Mesh);
	}

	if (UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		if (UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(BaseMat, Mesh))
		{
			Mid->SetVectorParameterValue(TEXT("Color"), Color);
			Mesh->SetMaterial(0, Mid);
		}
	}
}
}
