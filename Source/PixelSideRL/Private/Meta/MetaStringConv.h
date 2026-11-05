#pragma once

#include "CoreMinimal.h"

#include <string>
#include <string_view>

// One inline copy for the module unity TU. Anonymous-namespace helpers with
// these names in several .cpp files are the same function under clang and
// fail -Werror (redefinition) inside Module.PixelSideRL.*.cpp.
namespace PixelSideMetaUE
{

inline std::string ToUtf8(const FString& Text)
{
	const FTCHARToUTF8 Convert(*Text);
	return std::string(Convert.Get(), static_cast<size_t>(FMath::Max(0, Convert.Length())));
}

inline FString FromUtf8(std::string_view Text)
{
	const FUTF8ToTCHAR Convert(Text.data(), static_cast<int32>(Text.size()));
	return FString(Convert.Length(), Convert.Get());
}

inline FName IdName(std::string_view Id)
{
	if (Id.empty())
	{
		return NAME_None;
	}
	return FName(FromUtf8(Id));
}

inline FString Utf8ToString(const std::string& Text)
{
	return FromUtf8(Text);
}

}
