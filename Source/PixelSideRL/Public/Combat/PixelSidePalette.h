#pragma once

#include "CoreMinimal.h"

// ArtBible §3.1 hex values, one place. Gameplay code calls these instead of embedding hex.
namespace PixelSidePalette
{
inline FLinearColor FromHex(const TCHAR* Hex)
{
	return FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
}

inline FLinearColor ProjectilePlayer() { return FromHex(TEXT("F0C040")); }
inline FLinearColor ProjectileEnemy() { return FromHex(TEXT("E060A0")); }
inline FLinearColor HitFlash() { return FromHex(TEXT("F5F0E8")); }
inline FLinearColor PlayerCloth() { return FromHex(TEXT("3D6B8C")); }
inline FLinearColor EnemyWarning() { return FromHex(TEXT("C43C3C")); }
inline FLinearColor Floor() { return FromHex(TEXT("2E3D52")); }
inline FLinearColor Wall() { return FromHex(TEXT("3A4555")); }
}
