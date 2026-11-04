#include "Affix/AffixTypes.h"

#include "Affix/AffixRowApply.h"

bool UAffixDefinition::ApplyBuiltinP05(FName InAffixId)
{
	if (InAffixId.IsNone())
	{
		return false;
	}
	const std::string Id = PixelSideAffixUE::ToUtf8(InAffixId.ToString());
	for (const PixelSideAffix::FAffixRow& Row : PixelSideAffix::BuiltinP05Rows())
	{
		if (Row.AffixId == Id)
		{
			PixelSideAffixUE::ApplyRow(this, Row);
			return true;
		}
	}
	return false;
}
