// Host-side P0.5 affix checks. No Unreal headers.
// g++ -std=c++17 -Wall -Wextra -Werror -I Source/PixelSideRL/Public -o /tmp/test_affix_p05 Scripts/test_affix_p05.cpp Source/PixelSideRL/Private/Affix/AffixStatMath.cpp
// /tmp/test_affix_p05 .
#include "Affix/AffixStatMath.h"
#include "Combat/PixelSideCombatMath.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
int gFails = 0;

void Expect(bool Cond, const char* Msg)
{
	if (!Cond)
	{
		std::printf("FAIL %s\n", Msg);
		++gFails;
	}
}

bool Near(float A, float B)
{
	const float D = (A > B) ? (A - B) : (B - A);
	return D < 0.0001f;
}

std::string ReadFile(const std::string& Path)
{
	std::ifstream In(Path, std::ios::binary);
	if (!In)
	{
		std::printf("FAIL missing %s\n", Path.c_str());
		++gFails;
		return {};
	}
	std::ostringstream Buffer;
	Buffer << In.rdbuf();
	return Buffer.str();
}

bool SameList(const std::vector<std::string>& A, const std::vector<std::string>& B)
{
	return A == B;
}

bool SameRow(const PixelSideAffix::FAffixRow& A, const PixelSideAffix::FAffixRow& B, const char* Label)
{
	using namespace PixelSideAffix;
	bool Ok = true;
	auto Check = [&](bool Cond, const char* What) {
		if (!Cond)
		{
			std::printf("FAIL %s %s\n", Label, What);
			++gFails;
			Ok = false;
		}
	};
	Check(A.AffixId == B.AffixId, "id");
	Check(A.DisplayName == B.DisplayName, "name");
	Check(A.Description == B.Description, "desc");
	Check(A.Rarity == B.Rarity, "rarity");
	Check(A.StackRule == B.StackRule, "stack");
	Check(A.MaxStacks == B.MaxStacks, "max");
	Check(Near(A.Weight, B.Weight), "weight");
	Check(SameList(A.Tags, B.Tags), "tags");
	Check(SameList(A.ExclusiveTags, B.ExclusiveTags), "exclusive");
	Check(A.Triggers == B.Triggers, "triggers");
	Check(A.DataAssetName == B.DataAssetName, "da");
	Check(A.DataAssetName == std::string("DA_Affix_") + A.AffixId, "da freeze");
	Check(A.Modifiers.size() == B.Modifiers.size(), "mod count");
	const size_t N = A.Modifiers.size() < B.Modifiers.size() ? A.Modifiers.size() : B.Modifiers.size();
	for (size_t I = 0; I < N; ++I)
	{
		Check(A.Modifiers[I].Stat == B.Modifiers[I].Stat, "mod stat");
		Check(A.Modifiers[I].Op == B.Modifiers[I].Op, "mod op");
		Check(Near(A.Modifiers[I].Value, B.Modifiers[I].Value), "mod value");
		Check(Near(A.Modifiers[I].DurationSec, B.Modifiers[I].DurationSec), "mod duration");
	}
	return Ok;
}

const PixelSideAffix::FAffixRow* FindId(const std::vector<PixelSideAffix::FAffixRow>& Rows, const std::string& Id)
{
	for (const auto& Row : Rows)
	{
		if (Row.AffixId == Id)
		{
			return &Row;
		}
	}
	return nullptr;
}

bool OfferHasRare(const PixelSideAffix::FRunModel& Model)
{
	for (const auto& Choice : Model.Pending())
	{
		const auto* Row = Model.Find(Choice.AffixId);
		if (Row && static_cast<int>(Row->Rarity) >= static_cast<int>(PixelSideAffix::ERarity::Rare))
		{
			return true;
		}
	}
	return false;
}
}

int main(int argc, char** argv)
{
	using namespace PixelSideAffix;
	const std::string Root = argc > 1 ? argv[1] : ".";
	const std::string JsonText = ReadFile(Root + "/Content/Data/Affixes/DT_Affix_P05.json");
	const std::string CsvText = ReadFile(Root + "/Content/Data/Affixes/DT_Affix_P05.csv");
	const std::string MvpText = ReadFile(Root + "/Content/Data/Affixes/DT_Affix_MVP.json");
	const std::string BiasText = ReadFile(Root + "/Content/Data/Affixes/DT_AffixRarityBias.json");
	if (gFails != 0)
	{
		std::printf("%d failed\n", gFails);
		return 1;
	}

	Expect(Near(DefaultBase(EStat::Damage), PixelSideCombat::PeaDamage), "damage base is peashooter");
	Expect(Near(DefaultBase(EStat::FireRate), PixelSideCombat::FireRateBase), "firerate base is 1");
	Expect(Near(DefaultBase(EStat::MaxHP), PixelSideCombat::PlayerMaxHealth), "max hp base");
	Expect(Near(DefaultBase(EStat::MoveSpeed), PixelSideCombat::MaxWalkSpeed), "move base");
	Expect(Near(DefaultBase(EStat::ScrapGain), 1.f), "scrap gain mul identity");
	Expect(BuiltinP05Rows().size() == 8, "builtin eight");

	FTable FromJson;
	Expect(LoadTableFromJson(JsonText, FromJson), "parse p05 json");
	Expect(FromJson.ContractVersion == 3, "json contract 3");
	Expect(FromJson.PoolId == "P05", "pool id");
	Expect(FromJson.Affixes.size() == 8, "json eight");
	const char* Ids[] = {
		"RapidFire", "HollowPoint", "ThickSkin", "Magnet", "Piercer", "TwinShot", "CritOptics", "Scavenger"
	};
	for (int I = 0; I < 8; ++I)
	{
		Expect(FromJson.Affixes[static_cast<size_t>(I)].AffixId == Ids[I], "json order");
		const FAffixRow* Builtin = FindId(BuiltinP05Rows(), Ids[I]);
		const FAffixRow* Loaded = FindId(FromJson.Affixes, Ids[I]);
		Expect(Builtin && Loaded, "row present");
		if (Builtin && Loaded)
		{
			SameRow(*Builtin, *Loaded, Ids[I]);
		}
	}

	FTable FromCsv;
	Expect(LoadTableFromCsv(CsvText, FromCsv), "parse p05 csv");
	Expect(FromCsv.Affixes.size() == 8, "csv eight");
	for (const char* Id : Ids)
	{
		const FAffixRow* Builtin = FindId(BuiltinP05Rows(), Id);
		const FAffixRow* Loaded = FindId(FromCsv.Affixes, Id);
		Expect(Builtin && Loaded, "csv row");
		if (Builtin && Loaded)
		{
			SameRow(*Builtin, *Loaded, Id);
		}
	}

	FTable Mvp;
	Expect(LoadTableFromJson(MvpText, Mvp), "parse locked mvp");
	Expect(Mvp.Affixes.size() == 10, "mvp still 10");
	for (const char* Id : Ids)
	{
		const FAffixRow* Builtin = FindId(BuiltinP05Rows(), Id);
		const FAffixRow* Locked = FindId(Mvp.Affixes, Id);
		Expect(Builtin && Locked, "mvp has p05 id");
		if (Builtin && Locked)
		{
			SameRow(*Builtin, *Locked, Id);
		}
	}
	Expect(FindId(Mvp.Affixes, "Adrenaline") && FindId(Mvp.Affixes, "GlassCannon"), "excluded stay on the lock table");
	Expect(FindId(FromJson.Affixes, "Adrenaline") == nullptr, "p05 does not invent or include Adrenaline");

	FBiasTable Bias;
	std::string BiasError;
	Expect(LoadBiasFromJson(BiasText, Bias, &BiasError), "parse rarity bias");
	const FBiasTable Builtin = BuiltinBias();
	Expect(Near(Bias.NormalRoom.Common, Builtin.NormalRoom.Common) && Near(Bias.NormalRoom.Rare, 0.25f), "normal bias");
	Expect(Near(Bias.EliteRoom.Rare, 0.35f) && Near(Bias.PostBoss.Epic, 0.25f), "elite and boss bias");
	Expect(Near(Bias.TagWeightBonus, 0.1f) && Bias.PityStreak == 5 && Bias.PityAtLeast == ERarity::Rare, "pity and tag bonus");

	FTable Junk;
	Expect(!LoadTableFromJson("{", Junk), "reject truncated json");
	Expect(!LoadTableFromJson("{\"contractVersion\":2,\"affixes\":[{\"AffixId\":\"X\",\"DisplayName\":\"n\",\"Description\":\"d\",\"Rarity\":\"Common\",\"StackRule\":\"Unique\",\"Modifiers\":[{\"Stat\":\"Damage\",\"Op\":\"Add\",\"Value\":1}]}]}", Junk), "reject other contract");

	FRunModel Model;
	Expect(Model.TableSource() == ETableSource::Builtin, "starts on builtin");
	std::string LoadError;
	Expect(Model.LoadJson(JsonText, &LoadError), "model loads json");
	Expect(Model.TableSource() == ETableSource::Json, "source json");
	Expect(Model.LoadBiasJson(BiasText, &LoadError), "model loads bias");

	// HollowPoint ×1: Damage = Base * (1+0.1) = 8 * 1.1 = 8.8
	const float DamageBase = Model.GetBase(EStat::Damage);
	const float FireBase = Model.GetBase(EStat::FireRate);
	Expect(Near(Model.GetStat(EStat::Damage), DamageBase), "no affix damage is base");
	Expect(Near(Model.GetStat(EStat::FireRate), FireBase), "no affix firerate is 1");

	FRunModel::FStart Start;
	Start.Seed = 1;
	Model.BeginRun(Start);
	Expect(Model.IsInRun(), "run started");
	FChange Change;
	Expect(Model.DebugGrant("HollowPoint", &Change) && Change.Stacks == 1, "grant hollow point");
	const float HollowOnce = DamageBase * (1.f + 0.1f);
	Expect(Near(HollowOnce, 8.8f), "hand calc 8 * 1.1 = 8.8");
	Expect(Near(Model.GetStat(EStat::Damage), HollowOnce), "HollowPoint x1 damage");
	Expect(Near(Model.GetStat(EStat::FireRate), FireBase), "hollow point does not touch firerate");

	Expect(Model.DebugGrant("HollowPoint", &Change) && Change.Stacks == 2, "stack hollow point");
	const float HollowTwice = DamageBase * (1.f + 0.1f) * (1.f + 0.1f);
	Expect(Near(HollowTwice, 9.68f), "hand calc two factors 8 * 1.1 * 1.1");
	Expect(Near(Model.GetStat(EStat::Damage), HollowTwice), "HollowPoint x2 product");

	Expect(Model.DebugGrant("RapidFire", &Change), "grant rapid fire");
	const float RapidOnce = FireBase * (1.f + 0.12f);
	Expect(Near(RapidOnce, 1.12f), "hand calc firerate 1.12");
	Expect(Near(Model.GetStat(EStat::FireRate), RapidOnce), "RapidFire x1");
	const float Interval = PixelSideCombat::ComputeFireCooldown(PixelSideCombat::PeaFireInterval, Model.GetStat(EStat::FireRate));
	Expect(Near(Interval, PixelSideCombat::PeaFireInterval / 1.12f), "cooldown is FireInterval / GetStat(FireRate)");

	Expect(Model.DebugGrant("RapidFire", &Change) && Model.DebugGrant("RapidFire", &Change) && Change.Stacks == 3, "rapid fire cap");
	Expect(!Model.DebugGrant("RapidFire", &Change), "fourth rapid fire rejected");
	float RapidThree = FireBase;
	for (int I = 0; I < 3; ++I)
	{
		RapidThree *= (1.f + 0.12f);
	}
	Expect(Near(Model.GetStat(EStat::FireRate), RapidThree), "RapidFire x3 product");
	Expect(Near(Model.PreviewChoiceWeight("RapidFire", EOfferSource::NormalRoom), 0.f), "capped affix leaves the pool");

	Expect(Model.DebugGrant("ThickSkin", &Change), "thick skin");
	Expect(Near(Model.GetStat(EStat::MaxHP), PixelSideCombat::PlayerMaxHealth + 10.f), "max hp add 10");
	Expect(Model.DebugGrant("Magnet", &Change), "magnet");
	Expect(Near(Model.GetStat(EStat::PickupRadius), 48.f), "pickup radius add 48");
	Expect(!Model.DebugGrant("Magnet", &Change), "unique magnet");
	Expect(Model.DebugGrant("Piercer", &Change), "piercer");
	Expect(Near(Model.GetStat(EStat::Pierce), 1.f), "pierce add 1");
	Expect(Model.DebugGrant("TwinShot", &Change), "twin shot");
	Expect(Near(Model.GetStat(EStat::ProjectileCount), 2.f), "projectile count 1+1");
	Expect(Model.DebugGrant("CritOptics", &Change), "crit optics");
	Expect(Near(Model.GetStat(EStat::CritChance), 0.08f), "crit chance 0.08");
	Expect(Model.DebugGrant("Scavenger", &Change), "scavenger");
	Expect(Near(Model.GetStat(EStat::ScrapGain), 1.f * (1.f + 0.15f)), "scrap gain mul 1.15");
	Expect(Model.HasTag("Offense") && Model.HasTag("Economy") && Model.HasTag("Defense"), "tags from inventory");

	const FCard Card = Model.GetCard("HollowPoint", 1);
	Expect(Card.Name == "空尖弹" && Card.Rarity == "Common" && Card.bOwned && Card.Stacks == 2, "card view");
	Expect(Card.Desc == "伤害 +0.1", "format {Value}");
	Expect(Model.GetCard("RapidFire", 2).Desc == "射速 +0.24", "preview stacks scale {Value}");
	Expect(Model.GetCard("Piercer", 1).Desc == "穿透 +1", "desc without token stays");
	Expect(Card.AffixId == "HollowPoint" && Model.Find("HollowPoint")->DataAssetName == "DA_Affix_HollowPoint", "frozen da name");

	Model.EndRun();
	Expect(!Model.IsInRun() && Model.GetStacks("HollowPoint") == 0, "end run clears inventory");
	Expect(Near(Model.GetStat(EStat::Damage), DamageBase), "end run drops back to base");

	// Meta Add then passive Mul. (8+2) * (1+0.1) = 11
	FRunModel::FStart MetaStart;
	MetaStart.Seed = 7;
	MetaStart.PermanentModifiers.push_back(FMod{EStat::Damage, EOp::Add, 2.f, 0.f});
	MetaStart.StartWeaponId = "Peashooter";
	MetaStart.StartingScrap = 15;
	Model.BeginRun(MetaStart);
	Expect(Model.StartWeaponId() == "Peashooter" && Model.StartingScrap() == 15, "run context stored");
	Expect(Near(Model.GetStat(EStat::Damage), DamageBase + 2.f), "meta add before affix");
	Model.DebugGrant("HollowPoint", nullptr);
	Expect(Near(Model.GetStat(EStat::Damage), (DamageBase + 2.f) * (1.f + 0.1f)), "meta then hollow point");

	// Same-layer Add then Mul, and Override replacing both.
	const char* Synthetic =
		"{\"contractVersion\":3,\"poolId\":\"Synth\",\"affixes\":["
		"{\"AffixId\":\"Bulk\",\"DisplayName\":\"b\",\"Description\":\"d\",\"Rarity\":\"Common\",\"StackRule\":\"Unique\",\"MaxStacks\":1,\"Weight\":1,"
		"\"Tags\":[],\"ExclusiveTags\":[],\"Triggers\":[\"Passive\"],\"Modifiers\":["
		"{\"Stat\":\"MaxHP\",\"Op\":\"Add\",\"Value\":10,\"DurationSec\":0},"
		"{\"Stat\":\"MaxHP\",\"Op\":\"Mul\",\"Value\":-0.25,\"DurationSec\":0}]},"
		"{\"AffixId\":\"LockDamage\",\"DisplayName\":\"l\",\"Description\":\"d\",\"Rarity\":\"Common\",\"StackRule\":\"Unique\",\"MaxStacks\":1,\"Weight\":1,"
		"\"Tags\":[],\"ExclusiveTags\":[],\"Triggers\":[\"Passive\"],\"Modifiers\":["
		"{\"Stat\":\"Damage\",\"Op\":\"Add\",\"Value\":4,\"DurationSec\":0},"
		"{\"Stat\":\"Damage\",\"Op\":\"Mul\",\"Value\":1,\"DurationSec\":0},"
		"{\"Stat\":\"Damage\",\"Op\":\"Override\",\"Value\":3,\"DurationSec\":0}]},"
		"{\"AffixId\":\"TriShot\",\"DisplayName\":\"t\",\"Description\":\"d\",\"Rarity\":\"Rare\",\"StackRule\":\"Unique\",\"MaxStacks\":1,\"Weight\":4,"
		"\"Tags\":[\"Offense\"],\"ExclusiveTags\":[\"ProjectileShape\"],\"Triggers\":[\"Passive\"],\"Modifiers\":["
		"{\"Stat\":\"ProjectileCount\",\"Op\":\"Add\",\"Value\":2,\"DurationSec\":0}]},"
		"{\"AffixId\":\"Adrenaline\",\"DisplayName\":\"肾上腺素\",\"Description\":\"x\",\"Rarity\":\"Epic\",\"StackRule\":\"RefreshDuration\",\"MaxStacks\":1,\"Weight\":3,"
		"\"Tags\":[\"Mobility\"],\"ExclusiveTags\":[],\"Triggers\":[\"OnKill\"],\"Modifiers\":["
		"{\"Stat\":\"MoveSpeed\",\"Op\":\"Mul\",\"Value\":0.25,\"DurationSec\":2.5}]}"
		"]}";
	FRunModel Synth;
	Expect(Synth.LoadJson(Synthetic, &LoadError), LoadError.c_str());
	FRunModel::FStart SynthStart;
	SynthStart.Seed = 3;
	Synth.BeginRun(SynthStart);
	Synth.DebugGrant("Bulk", nullptr);
	Expect(Near(Synth.GetStat(EStat::MaxHP), (PixelSideCombat::PlayerMaxHealth + 10.f) * (1.f - 0.25f)), "add then mul in one layer");
	Synth.DebugGrant("LockDamage", nullptr);
	Expect(Near(Synth.GetStat(EStat::Damage), 3.f), "override replaces add and mul");
	const float MoveBase = Synth.GetBase(EStat::MoveSpeed);
	Synth.DebugGrant("Adrenaline", nullptr);
	Expect(Near(Synth.GetStat(EStat::MoveSpeed), MoveBase), "duration mod is not passive");
	const std::vector<std::string> Fired = Synth.NotifyTrigger(ETrigger::OnKill);
	Expect(Fired.size() == 1 && Fired[0] == "Adrenaline", "on kill fires adrenaline");
	Expect(Near(Synth.GetStat(EStat::MoveSpeed), MoveBase * (1.f + 0.25f)), "temp layer after passive");
	Synth.NotifyTrigger(ETrigger::OnKill);
	Expect(Near(Synth.GetStat(EStat::MoveSpeed), MoveBase * (1.f + 0.25f)), "refresh duration does not stack the temp mul");

	// Choice weights from the locked table.
	FRunModel Pool;
	Expect(Pool.LoadJson(JsonText, &LoadError), "reload p05");
	Expect(Pool.LoadBiasJson(BiasText, &LoadError), "reload bias");
	FRunModel::FStart PoolStart;
	PoolStart.Seed = 1;
	Pool.BeginRun(PoolStart);
	Expect(Near(Pool.PreviewChoiceWeight("HollowPoint", EOfferSource::NormalRoom), 10.f * 0.7f), "hollow point weight");
	Expect(Near(Pool.PreviewChoiceWeight("Piercer", EOfferSource::EliteRoom), 5.f * 0.35f), "piercer elite weight");
	Expect(Near(Pool.PreviewChoiceWeight("Piercer", EOfferSource::PostBoss), 5.f * 0.45f), "piercer boss weight");
	Expect(Pool.RollChoices(3, 42, EOfferSource::NormalRoom), "roll 3");
	Expect(Pool.Pending().size() == 3, "three cards");
	Expect(Pool.Pending()[0].AffixId != Pool.Pending()[1].AffixId
		&& Pool.Pending()[1].AffixId != Pool.Pending()[2].AffixId
		&& Pool.Pending()[0].AffixId != Pool.Pending()[2].AffixId, "distinct ids");
	const std::string A = Pool.Pending()[0].AffixId;
	const std::string B = Pool.Pending()[1].AffixId;
	const std::string C = Pool.Pending()[2].AffixId;
	Expect(!Pool.RollChoices(3, 99, EOfferSource::NormalRoom), "second roll waits for confirm");
	Expect(Pool.SkipAffixChoice(), "skip");
	Expect(!Pool.IsChoicePending(), "skip closes");
	Expect(Pool.RollChoices(3, 42, EOfferSource::NormalRoom), "reroll same seed");
	Expect(Pool.Pending()[0].AffixId == A && Pool.Pending()[1].AffixId == B && Pool.Pending()[2].AffixId == C, "seed is stable");
	Expect(Pool.ConfirmAffixChoice(0, &Change) && Change.AffixId == A && Change.Stacks == 1, "confirm index 0");
	Expect(!Pool.IsChoicePending() && Pool.GetStacks(A) == 1, "confirm grants");
	Expect(!Pool.ConfirmAffixChoice(0, &Change), "confirm without an offer fails");

	Pool.DebugGrant("Magnet", nullptr);
	Expect(Pool.RollChoices(3, 5, EOfferSource::NormalRoom), "roll after magnet");
	for (const FChoice& Choice : Pool.Pending())
	{
		Expect(Choice.AffixId != "Magnet", "unique stays out of the offer");
	}
	Pool.SkipAffixChoice();
	Expect(Near(Pool.PreviewChoiceWeight("HollowPoint", EOfferSource::NormalRoom), 10.f * 0.7f * 1.1f), "offense tag +10%");
	Expect(Near(Pool.PreviewChoiceWeight("TwinShot", EOfferSource::NormalRoom), 4.f * 0.25f * (1.f + 0.1f * 2.f)), "two-tag boost");

	// Unlock filter of three still fills a hand.
	FRunModel Narrow;
	Narrow.LoadJson(JsonText, nullptr);
	FRunModel::FStart NarrowStart;
	NarrowStart.Seed = 2;
	NarrowStart.UnlockedAffixIds = {"RapidFire", "HollowPoint", "ThickSkin"};
	Narrow.BeginRun(NarrowStart);
	Expect(Narrow.RollChoices(3, 1, EOfferSource::NormalRoom), "unlocked trio");
	for (const FChoice& Choice : Narrow.Pending())
	{
		Expect(Choice.AffixId == "RapidFire" || Choice.AffixId == "HollowPoint" || Choice.AffixId == "ThickSkin", "pool filter");
	}

	// Pool of one AdditiveStack Common repeats to fill 3.
	const char* OnlyCommon =
		"{\"contractVersion\":3,\"affixes\":[{\"AffixId\":\"ThickSkin\",\"DisplayName\":\"厚皮\",\"Description\":\"最大生命 +{Value}\","
		"\"Rarity\":\"Common\",\"StackRule\":\"AdditiveStack\",\"MaxStacks\":5,\"Weight\":10,\"Tags\":[\"Defense\"],\"ExclusiveTags\":[],"
		"\"Triggers\":[\"Passive\"],\"Modifiers\":[{\"Stat\":\"MaxHP\",\"Op\":\"Add\",\"Value\":10,\"DurationSec\":0}],"
		"\"DataAssetName\":\"DA_Affix_ThickSkin\"}]}";
	FRunModel Short;
	Expect(Short.LoadJson(OnlyCommon, &LoadError), "short pool");
	FRunModel::FStart ShortStart;
	ShortStart.Seed = 1;
	Short.BeginRun(ShortStart);
	Expect(Short.RollChoices(3, 1, EOfferSource::NormalRoom), "fallback fills 3");
	Expect(Short.Pending().size() == 3, "fallback count");
	Expect(Short.Pending()[0].AffixId == "ThickSkin" && Short.Pending()[2].AffixId == "ThickSkin", "repeated common");
	Expect(Short.LastNote().find("pool short") != std::string::npos, "fallback note");

	// Five offers with rare weight zero, sixth must contain a Rare+.
	FRunModel Pity;
	Pity.LoadJson(JsonText, nullptr);
	FBiasTable CommonOnly = BuiltinBias();
	CommonOnly.NormalRoom = {1.f, 0.f, 0.f, 0.f};
	Pity.SetBias(CommonOnly);
	FRunModel::FStart PityStart;
	PityStart.Seed = 11;
	Pity.BeginRun(PityStart);
	for (int Offer = 0; Offer < 5; ++Offer)
	{
		Expect(Pity.RollChoices(3, -1, EOfferSource::NormalRoom), "common offer");
		Expect(!OfferHasRare(Pity), "bias hid rares");
		Expect(Pity.ConfirmAffixChoice(0, nullptr), "take common");
	}
	Expect(Pity.PityStreak() == 5, "streak hits 5");
	Expect(Pity.RollChoices(3, -1, EOfferSource::NormalRoom), "sixth offer");
	Expect(OfferHasRare(Pity), "pity injects Rare+");

	// Exclusive tag: TwinShot blocks another ProjectileShape affix.
	FRunModel Block;
	Block.LoadJson(JsonText, nullptr);
	// Graft by granting TwinShot on a model that also has TriShot: rebuild json is heavier.
	// Weight of TriShot drops once TwinShot is owned inside the synthetic table if we add TwinShot.
	const char* Shape =
		"{\"contractVersion\":3,\"affixes\":["
		"{\"AffixId\":\"TwinShot\",\"DisplayName\":\"双生弹\",\"Description\":\"弹数 +1\",\"Rarity\":\"Rare\",\"StackRule\":\"Unique\",\"MaxStacks\":1,\"Weight\":4,"
		"\"Tags\":[\"Offense\",\"Weapon\"],\"ExclusiveTags\":[\"ProjectileShape\"],\"Triggers\":[\"Passive\"],"
		"\"Modifiers\":[{\"Stat\":\"ProjectileCount\",\"Op\":\"Add\",\"Value\":1,\"DurationSec\":0}]},"
		"{\"AffixId\":\"TriShot\",\"DisplayName\":\"三生\",\"Description\":\"d\",\"Rarity\":\"Rare\",\"StackRule\":\"Unique\",\"MaxStacks\":1,\"Weight\":4,"
		"\"Tags\":[\"Offense\"],\"ExclusiveTags\":[\"ProjectileShape\"],\"Triggers\":[\"Passive\"],"
		"\"Modifiers\":[{\"Stat\":\"ProjectileCount\",\"Op\":\"Add\",\"Value\":2,\"DurationSec\":0}]}"
		"]}";
	Expect(Block.LoadJson(Shape, &LoadError), "shape table");
	FRunModel::FStart BlockStart;
	BlockStart.Seed = 1;
	Block.BeginRun(BlockStart);
	Expect(Block.PreviewChoiceWeight("TriShot", EOfferSource::NormalRoom) > 0.f, "trishot available");
	Block.DebugGrant("TwinShot", nullptr);
	Expect(Near(Block.PreviewChoiceWeight("TriShot", EOfferSource::NormalRoom), 0.f), "exclusive tag blocks TriShot");

	// Temp mul applies after the passive product.
	FRunModel Order;
	Order.UseBuiltinP05();
	FRunModel::FStart OrderStart;
	OrderStart.Seed = 1;
	OrderStart.PermanentModifiers.push_back(FMod{EStat::Damage, EOp::Override, 10.f, 0.f});
	Order.BeginRun(OrderStart);
	Expect(Near(Order.GetStat(EStat::Damage), 10.f), "meta override");
	Order.DebugGrant("HollowPoint", nullptr);
	Expect(Near(Order.GetStat(EStat::Damage), 10.f * 1.1f), "passive mul after meta override");

	Order.EndRun();
	Expect(!Order.IsInRun(), "ended");
	Expect(!Order.RollChoices(3, 1, EOfferSource::NormalRoom), "roll requires BeginRun");

	if (gFails != 0)
	{
		std::printf("%d failed\n", gFails);
		return 1;
	}
	std::printf("ok affix p05\n");
	return 0;
}
