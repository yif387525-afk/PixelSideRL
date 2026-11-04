#include "Affix/AffixCatalog.h"

#include "Affix/AffixRowApply.h"
#include "Affix/AffixStatMath.h"

void UAffixCatalog::PopulateP05Defaults()
{
	PixelSideAffix::FRunModel Model;
	Model.UseBuiltinP05();
	Definitions.Reset();
	PoolId = TEXT("P05");
	for (const PixelSideAffix::FAffixRow& Row : Model.Pool())
	{
		const FName ObjName = MakeUniqueObjectName(this, UAffixDefinition::StaticClass(), PixelSideAffixUE::IdName(Row.DataAssetName));
		UAffixDefinition* Def = NewObject<UAffixDefinition>(this, ObjName, RF_Transient);
		PixelSideAffixUE::ApplyRow(Def, Row);
		Definitions.Add(Def);
	}
}
