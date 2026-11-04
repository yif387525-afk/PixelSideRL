#pragma once

#include "CoreMinimal.h"

class UObject;
class UPaperFlipbook;

/**
 * Build a Paper2D flipbook by slicing a horizontal PNG strip at runtime.
 * Content stays the authored PNG. This does not write a .uasset.
 * Pivot of every frame is bottom center. 1 px = 1 uu.
 */
namespace PixelSideSprites
{
UPaperFlipbook* LoadHorizontalFlipbook(
	UObject* Outer,
	const TCHAR* ContentRelativePath,
	int32 FrameWidth,
	int32 FrameHeight,
	int32 FrameCount,
	float FramesPerSecond,
	const TCHAR* DebugName);
}
