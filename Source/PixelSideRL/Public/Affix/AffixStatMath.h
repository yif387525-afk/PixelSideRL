// Copyright PixelSideRL. P0.5 affix table, stat fold, and three-choice roll.
// No Unreal types — host tests compile this with the game module.
//
// GetStat layers (Docs/AffixBuffPipeline.md, Docs/AffixSubsystem_API.md):
//   Base → Meta permanent → Affix passive → temporary Duration buffs.
// Inside one layer, per stat: (current + ΣAdd) * Π(1+Mul). Mul Value is +x%
// (factor = 1+Value). The last Override in that layer replaces the result.
//
// Hand calc, peashooter bases, no meta:
//   HollowPoint ×1  Damage   = 8 * (1+0.1) = 8.8
//   RapidFire ×1    FireRate = 1 * (1+0.12) = 1.12
//   cooldown = FireInterval / GetStat(FireRate)
// Two AdditiveStack copies are two factors, not one summed percent:
//   HollowPoint ×2  Damage   = 8 * 1.1 * 1.1 = 9.68
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace PixelSideAffix
{

enum class EStat : uint8_t
{
	MaxHP,
	MoveSpeed,
	FireRate,
	Damage,
	CritChance,
	CritMult,
	MagSize,
	ReloadSpeed,
	ProjectileCount,
	Pierce,
	Knockback,
	PickupRadius,
	CoinGain,
	ScrapGain,
	Count
};

enum class EOp : uint8_t
{
	Add,
	Mul,
	Override
};

enum class EStack : uint8_t
{
	Unique,
	AdditiveStack,
	RefreshDuration
};

enum class ERarity : uint8_t
{
	Common,
	Rare,
	Epic,
	Legendary
};

enum class ETrigger : uint8_t
{
	OnAcquire,
	OnHit,
	OnKill,
	OnRoomClear,
	OnTakeDamage,
	Passive
};

enum class EOfferSource : uint8_t
{
	NormalRoom,
	EliteRoom,
	PostBoss
};

enum class ETableSource : uint8_t
{
	Builtin,
	Json,
	Csv
};

struct FMod
{
	EStat Stat = EStat::Damage;
	EOp Op = EOp::Add;
	float Value = 0.f;
	float DurationSec = 0.f;
};

struct FAffixRow
{
	std::string AffixId;
	std::string DisplayName;
	std::string Description;
	ERarity Rarity = ERarity::Common;
	std::string Icon;
	EStack StackRule = EStack::Unique;
	int MaxStacks = 1;
	float Weight = 1.f;
	std::vector<std::string> Tags;
	std::vector<std::string> ExclusiveTags;
	std::vector<ETrigger> Triggers;
	std::vector<FMod> Modifiers;
	std::string DataAssetName;
};

struct FTable
{
	int ContractVersion = 0;
	std::string PoolId;
	std::string Error;
	std::vector<FAffixRow> Affixes;
};

struct FRarityWeights
{
	float Common = 0.7f;
	float Rare = 0.25f;
	float Epic = 0.05f;
	float Legendary = 0.f;
};

struct FBiasTable
{
	FRarityWeights NormalRoom{0.7f, 0.25f, 0.05f, 0.f};
	FRarityWeights EliteRoom{0.5f, 0.35f, 0.15f, 0.f};
	FRarityWeights PostBoss{0.3f, 0.45f, 0.25f, 0.f};
	float TagWeightBonus = 0.1f;
	int PityStreak = 5;
	ERarity PityAtLeast = ERarity::Rare;
};

struct FOwned
{
	std::string AffixId;
	int Stacks = 0;
};

struct FCard
{
	std::string Name;
	std::string Desc;
	std::string Rarity;
	std::string Icon;
	std::string AffixId;
	int Stacks = 0;
	bool bOwned = false;
};

struct FChoice
{
	std::string AffixId;
	float WeightUsed = 0.f;
	bool bWouldBeNew = true;
};

struct FChange
{
	std::string AffixId;
	int Stacks = 0;
	bool bRemoved = false;
};

const char* StatName(EStat Stat);
const char* OpName(EOp Op);
const char* StackName(EStack Stack);
const char* RarityName(ERarity Rarity);
const char* TriggerName(ETrigger Trigger);

bool ParseStat(std::string_view Text, EStat& Out);
bool ParseOp(std::string_view Text, EOp& Out);
bool ParseStack(std::string_view Text, EStack& Out);
bool ParseRarity(std::string_view Text, ERarity& Out);
bool ParseTrigger(std::string_view Text, ETrigger& Out);

float DefaultBase(EStat Stat);
float ApplyLayer(float Current, const std::vector<FMod>& Mods, EStat Stat);
std::string FormatDescription(std::string_view Description, float FirstModifierValue, int PreviewStacks);

const std::vector<FAffixRow>& BuiltinP05Rows();
FBiasTable BuiltinBias();

bool LoadTableFromJson(std::string_view Json, FTable& Out);
bool LoadTableFromCsv(std::string_view Csv, FTable& Out);
bool LoadBiasFromJson(std::string_view Json, FBiasTable& Out, std::string* Error);

class FRunModel
{
public:
	struct FStart
	{
		std::vector<FMod> PermanentModifiers;
		std::vector<std::string> UnlockedAffixIds;
		std::string StartWeaponId;
		int StartingScrap = 0;
		uint32_t Seed = 1;
	};

	FRunModel();

	void UseBuiltinP05();
	bool LoadJson(std::string_view Json, std::string* Error);
	bool LoadCsv(std::string_view Csv, std::string* Error);
	bool LoadBiasJson(std::string_view Json, std::string* Error);
	void SetBias(const FBiasTable& InBias);

	void SetBase(EStat Stat, float Value);
	float GetBase(EStat Stat) const;
	void ResetBases();

	void BeginRun(const FStart& Start);
	void EndRun();
	bool IsInRun() const { return bInRun; }

	bool RollChoices(int Count, int SeedOrNeg1, EOfferSource Source);
	bool IsChoicePending() const { return !PendingChoices.empty(); }
	const std::vector<FChoice>& Pending() const { return PendingChoices; }
	bool ConfirmAffixChoice(int Index, FChange* Out);
	bool SkipAffixChoice();
	bool DebugGrant(const std::string& AffixId, FChange* Out);
	std::vector<std::string> NotifyTrigger(ETrigger Trigger);

	float GetStat(EStat Stat) const;
	int GetStacks(const std::string& AffixId) const;
	bool HasTag(const std::string& Tag) const;
	FCard GetCard(const std::string& AffixId, int PreviewStacks) const;
	const FAffixRow* Find(const std::string& AffixId) const;
	float PreviewChoiceWeight(const std::string& AffixId, EOfferSource Source) const;

	const std::vector<FAffixRow>& Pool() const { return Table.Affixes; }
	const std::vector<FOwned>& Owned() const { return Inventory; }
	int ContractVersion() const { return Table.ContractVersion; }
	const std::string& PoolId() const { return Table.PoolId; }
	ETableSource TableSource() const { return Source; }
	const std::string& LastNote() const { return Note; }
	int PityStreak() const { return StreakNoRarePlus; }
	const std::string& StartWeaponId() const { return WeaponId; }
	int StartingScrap() const { return Scrap; }
	const FBiasTable& Bias() const { return BiasTable; }

private:
	struct FTemp
	{
		std::string AffixId;
		FMod Mod;
	};

	float EffectiveWeight(const FAffixRow& Row, EOfferSource Source, bool bIgnoreRarityBias) const;
	bool IsEligible(const FAffixRow& Row) const;
	bool ExclusiveConflict(const FAffixRow& Row) const;
	int TagOverlap(const FAffixRow& Row) const;
	bool TryAdd(const FAffixRow& Row, FChange* Out);
	void NoteOfferResolved();
	void PushDurationOnAcquire(const FAffixRow& Row);
	void FillCommonFallback(int Count);
	void ApplyPity();
	const FAffixRow* PickOne(const std::vector<std::pair<const FAffixRow*, float>>& Cands);
	float RarityFactor(ERarity Rarity, EOfferSource Source) const;
	float Rand01();
	uint32_t NextU32();
	FOwned* FindOwned(const std::string& AffixId);
	const FOwned* FindOwned(const std::string& AffixId) const;

	FTable Table;
	FBiasTable BiasTable;
	ETableSource Source = ETableSource::Builtin;
	float Bases[static_cast<int>(EStat::Count)]{};
	std::vector<FMod> Meta;
	std::vector<FOwned> Inventory;
	std::vector<FTemp> Temps;
	std::vector<FChoice> PendingChoices;
	std::vector<std::string> Unlocked;
	std::string WeaponId;
	std::string Note;
	int Scrap = 0;
	int StreakNoRarePlus = 0;
	uint32_t Rng = 1;
	bool bInRun = false;
	EOfferSource LastSource = EOfferSource::NormalRoom;
};

}
