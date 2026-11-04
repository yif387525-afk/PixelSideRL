#include "Affix/AffixSubsystem.h"

float UAffixSubsystem::GetStatForWeapon(EStatId Stat, FName WeaponId) const
{
	(void)WeaponId;
	return GetStat(Stat);
}

void UAffixSubsystem::ApplyChoicePause(bool bPaused)
{
	OnAffixPauseRequested.Broadcast(bPaused);
}

void UAffixSubsystem::SetAutoPauseDuringChoice(bool bEnabled)
{
	bAutoPauseDuringChoice = bEnabled;
}
