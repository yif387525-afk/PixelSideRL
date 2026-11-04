#pragma once

// Host-side meta progress. No Unreal headers.
// JSON in Content/Data/Meta is the source of truth. Do not invent a stand-in uasset.
//
// Core is the out-of-run wallet. Run Scrap is not stored here.
// BuildRunStartContext reads purchased ranks. SeedRunScrap is what BeginRunFromMeta
// passes to UEventSubsystem::ResetRunScrap.
// NodeViews is the read-only list Hub draws. It does not spend Core and does not save.

#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace PixelSideMeta
{

inline constexpr const char* kNamingRule = "DA_MetaNode_<NodeId>";
inline constexpr const char* kMvpAffixPoolPolicy = "AllTenUnlockedAtRunStart";
inline constexpr int kSaveVersion = 1;

inline constexpr std::array<const char*, 10> kMvpAffixIds = {
	"RapidFire",
	"HollowPoint",
	"ThickSkin",
	"Magnet",
	"Piercer",
	"TwinShot",
	"CritOptics",
	"Adrenaline",
	"GlassCannon",
	"Scavenger"
};

inline std::string FrozenDataAssetName(const std::string& NodeId)
{
	return std::string("DA_MetaNode_") + NodeId;
}

/** Same clamp as ResetRunScrap + AddRunScrap: negative becomes 0. */
inline int SeedRunScrap(int StartingScrap)
{
	return StartingScrap > 0 ? StartingScrap : 0;
}

struct FModifier
{
	std::string Stat;
	std::string Op;
	double Value = 0.0;
	std::string WeaponTag;
};

struct FNode
{
	std::string NodeId;
	std::string DataAssetName;
	std::string DisplayName;
	std::string Description;
	std::vector<int> CostCoreByRank;
	std::vector<std::string> Prerequisites;
	/** Parsed and ignored. MVP does not gate the affix pool on this list. */
	std::vector<std::string> UnlockAffixIds;
	int MaxPurchases = 0;
	std::vector<std::vector<FModifier>> PermanentModifiersByRank;
	std::vector<int> StartingScrapByRank;
	std::vector<double> EconomyDeltaByRank;
	std::string EconomyKey;
	std::string WeaponId;
	std::vector<std::vector<std::string>> FlagsByRank;
};

struct FEconomy
{
	std::string RunCurrencyId;
	std::string MetaCurrencyId;
	int BaseClearBonus = 0;
	double DeathMult = 0.0;
	int PerRoom = 0;
	int BossBonus = 0;
	double CoinToScrap = 0.0;
	double ScrapToCore = 0.0;
	double MetaCoreMult = 0.0;
	int SoftCapPerRun = 0;
	int ContractVersion = 0;
	bool bLocked = false;
};

struct FRunStart
{
	std::vector<FModifier> PermanentModifiers;
	std::vector<std::string> UnlockedAffixIds;
	/** Always empty. PeaFamiliar is a Damage modifier, not a weapon grant. */
	std::string StartWeaponId;
	int StartingScrap = 0;
	std::vector<std::string> Flags;
	double MetaCoreMult = 0.0;
	bool bUnlockAffixIgnored = true;
	bool bAffixPoolReady = false;
};

/**
 * One row for the Hub upgrade list. Read-only.
 * NextCostCore is CostCoreByRank[OwnedRank], or 0 when the next rank does not exist.
 * bLocked is an unmet prerequisite. bCanPurchase is the same check as CanPurchase.
 */
struct FNodeView
{
	std::string NodeId;
	std::string DisplayName;
	std::string Description;
	int OwnedRank = 0;
	int MaxPurchases = 0;
	int NextCostCore = 0;
	bool bMaxed = false;
	bool bLocked = false;
	bool bCanPurchase = false;
};

/** RoguelikeMeta §6 preview. Does not credit Core. */
struct FCorePreview
{
	bool bReady = false;
	bool bAppliedToCore = false;
	int Core = 0;
	double MetaCoreMult = 0.0;
};

class FProgress
{
public:
	bool LoadEconomyJson(std::string_view Json, std::string* Error);
	bool LoadNodesJson(std::string_view Json, std::string* Error);
	/** affixes[] only. postMvpCandidates are not part of the pool. */
	bool LoadAffixPoolJson(std::string_view Json, std::string* Error);

	bool HasEconomy() const { return bEconomy; }
	bool HasNodes() const { return bNodes; }
	bool HasAffixPool() const { return bPool; }

	const FEconomy& Economy() const { return Econ; }
	const std::vector<FNode>& Nodes() const { return NodeRows; }
	const FNode* FindNode(const std::string& NodeId) const;
	const std::vector<std::string>& AffixPool() const { return Pool; }
	const std::string& NamingRuleText() const { return Naming; }
	const std::string& PoolPolicy() const { return Policy; }

	int GetCore() const { return Core; }
	/** Delta may be negative. Result never goes below 0 or above INT_MAX. */
	void AddCore(int Delta, const std::string& Reason);
	const std::string& LastCoreReason() const { return CoreReason; }

	/** 0 when the node is missing or unpurchased. */
	int GetPurchasedRank(const std::string& NodeId) const;

	bool CanPurchase(const std::string& NodeId, std::string* Reason) const;
	bool Purchase(const std::string& NodeId, std::string* Reason);
	void ResetSlot();

	FRunStart BuildRunStartContext() const;
	double GetMetaCoreMult() const;
	FCorePreview PreviewCoreGain(bool bCleared, int RoomsCleared, int BossKilled, int RunScrapRemain) const;

	/** NodeId:AffixId for every UnlockAffixIds entry. Those ids are not a pool gate. */
	std::vector<std::string> IgnoredUnlockAffixIds() const;

	/** Loaded nodes in table order, including rank 0. Does not spend Core. */
	std::vector<FNodeView> NodeViews() const;

	std::string Describe() const;

private:
	int EffectiveRank(const FNode& Node) const;

	FEconomy Econ;
	std::vector<FNode> NodeRows;
	std::vector<std::string> Pool;
	std::vector<std::pair<std::string, int>> Purchased;
	std::string Naming;
	std::string Policy;
	std::string CoreReason;
	int Core = 0;
	bool bEconomy = false;
	bool bNodes = false;
	bool bPool = false;
};

}
