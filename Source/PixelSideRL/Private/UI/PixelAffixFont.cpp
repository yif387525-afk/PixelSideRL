#include "UI/PixelAffixFont.h"

#include "Fonts/CompositeFont.h"
#include "Fonts/SlateFontInfo.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"

DEFINE_LOG_CATEGORY(LogPixelAffixUI);
DEFINE_LOG_CATEGORY(LogPixelHUD);

FSlateFontInfo PixelAffixMakeFont(int32 Size)
{
	static TSharedPtr<FCompositeFont> Composite;
	static bool bTried = false;
	if (!bTried)
	{
		bTried = true;
		const FString Regular = FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf");
		const FString Fallback = FPaths::EngineContentDir() / TEXT("Slate/Fonts/DroidSansFallback.ttf");
		if (FPaths::FileExists(Regular))
		{
			Composite = MakeShared<FCompositeFont>();
			Composite->DefaultTypeface.AppendFont(TEXT("Regular"), Regular, EFontHinting::Default);
			if (FPaths::FileExists(Fallback))
			{
				FCompositeFallbackFont& Row = Composite->FallbackTypeface.AddDefaulted_GetRef();
				Row.Typeface.AppendFont(TEXT("Regular"), Fallback, EFontHinting::Default);
				Row.ScalingFactor = 1.f;
			}
			else
			{
				UE_LOG(LogPixelAffixUI, Warning, TEXT("Affix UI: DroidSansFallback.ttf missing. Chinese card text may not draw."));
			}
		}
	}

	if (Composite.IsValid())
	{
		return FSlateFontInfo(Composite, static_cast<float>(Size), FName(TEXT("Regular")));
	}
	return FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size);
}
