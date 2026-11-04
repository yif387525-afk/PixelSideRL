#include "Affix/AffixSubsystem.h"

// Pulls the affix contract headers into the game module. The subsystem stays abstract.
// Combat reads AC_StatModifiers only and does not roll affixes.
static_assert(sizeof(EStatId) > 0, "affix stat contract");
