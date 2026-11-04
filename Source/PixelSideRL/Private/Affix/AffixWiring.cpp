#include "Affix/AffixSubsystem.h"
#include "Affix/AffixSubsystemDefault.h"
#include "Affix/AffixCatalog.h"
#include "Affix/AffixInventoryComponent.h"

// Concrete P0.6 subsystem, catalog defaults, and AC_AffixInventory.
// Combat reads AC_StatModifiers::GetStat and does not roll affixes.
static_assert(sizeof(EStatId) > 0, "affix stat contract");
static_assert(sizeof(UAffixSubsystemDefault) > 0, "affix subsystem default");
