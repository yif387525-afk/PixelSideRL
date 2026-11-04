#include "Affix/AffixSubsystem.h"

void UAffixSubsystem::ApplyChoicePause(bool bPaused)
{
	OnAffixPauseRequested.Broadcast(bPaused);
}

void UAffixSubsystem::SetAutoPauseDuringChoice(bool bEnabled)
{
	bAutoPauseDuringChoice = bEnabled;
}
