#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace PixelSideMesh
{
inline void Tint(UStaticMeshComponent* Mesh, const TCHAR* MeshPath, const FLinearColor& Color, bool bBlockAll)
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
	if (bBlockAll)
	{
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	}
	else
	{
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
