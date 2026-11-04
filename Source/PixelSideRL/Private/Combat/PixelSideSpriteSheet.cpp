#include "Combat/PixelSideSpriteSheet.h"

#include "Combat/PixelSideCombatLog.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "SpriteEditorOnlyTypes.h"

namespace
{
UMaterialInterface* PixelSpriteMaterial()
{
	// Masked unlit keeps hard pixel alpha. Translucent is the fallback if that asset moves.
	const TCHAR* Paths[] = {
		TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial"),
		TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"),
		TEXT("/Paper2D/OpaqueUnlitSpriteMaterial.OpaqueUnlitSpriteMaterial"),
	};
	for (const TCHAR* Path : Paths)
	{
		if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, Path))
		{
			return Material;
		}
	}
	return nullptr;
}

UTexture2D* TextureFromBGRA(const TArray64<uint8>& BGRA, int32 Width, int32 Height, const TCHAR* DebugName)
{
	const int64 Expected = static_cast<int64>(Width) * static_cast<int64>(Height) * 4;
	if (Width <= 0 || Height <= 0 || BGRA.Num() < Expected)
	{
		return nullptr;
	}

	const FName TexName = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), DebugName);
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8, TexName);
	if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
	{
		return nullptr;
	}

	Texture->SRGB = true;
	Texture->Filter = TF_Nearest;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	Texture->NeverStream = true;
	Texture->MipGenSettings = TMGS_NoMipmaps;
	Texture->LODGroup = TEXTUREGROUP_Pixels2D;

	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	if (Mip.BulkData.GetBulkDataSize() < Expected)
	{
		Data = Mip.BulkData.Realloc(Expected);
	}
	if (!Data)
	{
		Mip.BulkData.Unlock();
		return nullptr;
	}
	FMemory::Memcpy(Data, BGRA.GetData(), static_cast<SIZE_T>(Expected));
	Mip.BulkData.Unlock();
	Texture->UpdateResource();
	return Texture;
}

UPaperSprite* MakeFrame(UObject* Outer, UTexture2D* Texture, int32 OffsetX, int32 FrameWidth, int32 FrameHeight, UMaterialInterface* Material, const TCHAR* DebugName)
{
	const FName SpriteName = MakeUniqueObjectName(Outer, UPaperSprite::StaticClass(), DebugName);
	UPaperSprite* Sprite = NewObject<UPaperSprite>(Outer, SpriteName);
	if (!Sprite)
	{
		return nullptr;
	}

	FSpriteAssetInitParameters Init;
	Init.Texture = Texture;
	Init.Offset = FIntPoint(OffsetX, 0);
	Init.Dimension = FIntPoint(FrameWidth, FrameHeight);
	Init.bOverridePixelsPerUnrealUnit = true;
	Init.PixelsPerUnrealUnit = 1.f;
	Init.DefaultMaterialOverride = Material;
	Sprite->InitializeSprite(Init, false);
	Sprite->SetPivotMode(ESpritePivotMode::Bottom_Center, FVector2D::ZeroVector, true);
	return Sprite;
}
}

UPaperFlipbook* PixelSideSprites::LoadHorizontalFlipbook(
	UObject* Outer,
	const TCHAR* ContentRelativePath,
	int32 FrameWidth,
	int32 FrameHeight,
	int32 FrameCount,
	float FramesPerSecond,
	const TCHAR* DebugName)
{
	if (!Outer || !ContentRelativePath || FrameWidth <= 0 || FrameHeight <= 0 || FrameCount <= 0 || FramesPerSecond <= 0.f)
	{
		return nullptr;
	}

	const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), ContentRelativePath);
	TArray<uint8> Compressed;
	if (!FFileHelper::LoadFileToArray(Compressed, *FullPath))
	{
		UE_LOG(LogPixelSideCombat, Warning, TEXT("Sprite sheet missing: %s"), *FullPath);
		return nullptr;
	}

	IImageWrapperModule& Images = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	const TSharedPtr<IImageWrapper> Wrapper = Images.CreateImageWrapper(EImageFormat::PNG);
	if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Compressed.GetData(), Compressed.Num()))
	{
		UE_LOG(LogPixelSideCombat, Warning, TEXT("Sprite sheet is not a PNG: %s"), *FullPath);
		return nullptr;
	}

	TArray64<uint8> Raw;
	if (!Wrapper->GetRaw(ERGBFormat::BGRA, 8, Raw))
	{
		UE_LOG(LogPixelSideCombat, Warning, TEXT("Sprite sheet decode failed: %s"), *FullPath);
		return nullptr;
	}

	const int32 Width = Wrapper->GetWidth();
	const int32 Height = Wrapper->GetHeight();
	if (Width != FrameWidth * FrameCount || Height != FrameHeight)
	{
		UE_LOG(
			LogPixelSideCombat,
			Warning,
			TEXT("Sprite sheet %s is %dx%d; expected %d frames of %dx%d."),
			*FullPath,
			Width,
			Height,
			FrameCount,
			FrameWidth,
			FrameHeight);
		return nullptr;
	}

	UMaterialInterface* Material = PixelSpriteMaterial();
	if (!Material)
	{
		// Without a sprite-sampler material the sheet would not show. Caller keeps the placeholder.
		UE_LOG(LogPixelSideCombat, Warning, TEXT("Paper2D unlit sprite material is missing. %s stays on the placeholder."), DebugName ? DebugName : TEXT("Flipbook"));
		return nullptr;
	}

	const FName BookName = MakeUniqueObjectName(Outer, UPaperFlipbook::StaticClass(), DebugName ? DebugName : TEXT("PF_Runtime"));
	UPaperFlipbook* Flipbook = NewObject<UPaperFlipbook>(Outer, BookName);
	UTexture2D* Texture = TextureFromBGRA(Raw, Width, Height, DebugName ? DebugName : TEXT("T_Runtime"));
	if (!Flipbook || !Texture)
	{
		return nullptr;
	}

	TArray<UPaperSprite*> Frames;
	Frames.Reserve(FrameCount);
	for (int32 Index = 0; Index < FrameCount; ++Index)
	{
		const FString FrameLabel = FString::Printf(TEXT("%s_%d"), DebugName ? DebugName : TEXT("Frame"), Index);
		UPaperSprite* Frame = MakeFrame(Flipbook, Texture, Index * FrameWidth, FrameWidth, FrameHeight, Material, *FrameLabel);
		if (!Frame)
		{
			return nullptr;
		}
		Frames.Add(Frame);
	}

	{
		FScopedFlipbookMutator Mutator(Flipbook);
		Mutator.FramesPerSecond = FramesPerSecond;
		Mutator.KeyFrames.Reset();
		for (UPaperSprite* Frame : Frames)
		{
			FPaperFlipbookKeyFrame& Key = Mutator.KeyFrames.AddDefaulted_GetRef();
			Key.Sprite = Frame;
			Key.FrameRun = 1;
		}
	}

	if (Flipbook->GetNumFrames() != FrameCount)
	{
		UE_LOG(LogPixelSideCombat, Warning, TEXT("%s flipbook built %d frames, expected %d."), DebugName ? DebugName : TEXT("Flipbook"), Flipbook->GetNumFrames(), FrameCount);
		return nullptr;
	}
	return Flipbook;
}
