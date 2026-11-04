#pragma once

// Host-side event op rules. No Unreal headers.
// JSON in Content/Data/Events is the source of truth. Do not invent a stand-in uasset.

#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace PixelSideEvent
{

enum class EOp : unsigned char
{
	GrantScrap,
	DamageSelf,
	HealSelf,
	None,
	Unknown
};

struct FEffect
{
	EOp Kind = EOp::Unknown;
	std::string Op;
	double Value = 0.0;
	std::string Stat;
};

struct FChoice
{
	std::string ChoiceId;
	std::string Label;
	std::string ResultToast;
	std::vector<FEffect> Effects;
};

struct FEventDefinition
{
	std::string EventId;
	std::string DisplayName;
	std::string PromptBody;
	std::string SkipHint;
	bool bAllowDoorSkip = true;
	bool bCombatAllowed = false;
	bool bAffixOnResolve = false;
	std::string BindRoomId;
	std::string InteractAnchor;
	std::string DataAssetName;
	int ContractVersion = 0;
	std::vector<FChoice> Choices;
};

/** In-run Scrap only. Not Core. Not FMetaSaveData. */
struct FRunScrapWallet
{
	int Scrap = 0;
};

struct FEventVitals
{
	float CurrentHP = 0.f;
	/** <= 0 means HealSelf has no MaxHP cap. */
	float MaxHP = 0.f;
};

struct FOpStep
{
	std::string Op;
	EOp Kind = EOp::Unknown;
	int ScrapBefore = 0;
	int ScrapAfter = 0;
	float HpBefore = 0.f;
	float HpAfter = 0.f;
	bool bApplied = false;
	bool bUnknown = false;
	std::string Message;
};

struct FResolveResult
{
	bool bOk = false;
	std::string Error;
	std::string EventId;
	std::string ChoiceId;
	std::string ResultToast;
	bool bAffixOnResolve = false;
	/** Always false. MVP does not open AffixChoice. */
	bool bAffixChoiceRequested = false;
	bool bCombatAllowed = false;
	/** Always false. MVP does not start combat. */
	bool bCombatStarted = false;
	std::vector<std::string> Notes;
	std::vector<FOpStep> Steps;
	int Scrap = 0;
	float CurrentHP = 0.f;
	float MaxHP = 0.f;
};

struct FPoolEntry
{
	std::string EventId;
	std::string DataAssetName;
	std::string BindRoomId;
	double Weight = 0.0;
};

inline void AddScrap(FRunScrapWallet& Wallet, int Delta)
{
	const long long Next = static_cast<long long>(Wallet.Scrap) + static_cast<long long>(Delta);
	if (Next <= 0)
	{
		Wallet.Scrap = 0;
		return;
	}
	if (Next >= static_cast<long long>(std::numeric_limits<int>::max()))
	{
		Wallet.Scrap = std::numeric_limits<int>::max();
		return;
	}
	Wallet.Scrap = static_cast<int>(Next);
}

std::string FrozenDataAssetName(const std::string& EventId);
const char* OpName(EOp Kind);

class FCatalog
{
public:
	bool LoadEventJson(std::string_view Json, std::string* Error);
	bool LoadPoolJson(std::string_view Json, std::string* Error);
	/** Each pool row is loaded and named DA_Event_<EventId>. */
	bool ValidatePool(std::string* Error) const;

	const FEventDefinition* Find(const std::string& EventId) const;
	const std::vector<FEventDefinition>& Events() const { return Definitions; }
	const std::vector<FPoolEntry>& Pool() const { return PoolEntries; }
	const std::string& PoolNamingRule() const { return NamingRule; }

	/**
	 * Apply Choice.Effects in order, synchronously.
	 * DamageSelf on a living target floors CurrentHP at 1 (events cannot kill).
	 * Already-dead vitals (CurrentHP <= 0) are left dead.
	 * bAffixOnResolve false does not request an affix choice. A true flag is noted and still does not.
	 * bCombatAllowed true is noted and still does not start combat.
	 */
	FResolveResult Resolve(const std::string& EventId, const std::string& ChoiceId, FRunScrapWallet& Wallet, FEventVitals& Vitals) const;

private:
	std::vector<FEventDefinition> Definitions;
	std::vector<FPoolEntry> PoolEntries;
	std::string NamingRule;
};

}
