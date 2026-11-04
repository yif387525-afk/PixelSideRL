#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPixelAffixUI, Log, All);

/** Runtime font for the affix shell. Prefers engine Roboto + CJK fallback when those files exist. */
FSlateFontInfo PixelAffixMakeFont(int32 Size);
