// Host-side meta progress checks. No Unreal headers.
// g++ -std=c++17 -Wall -Wextra -Werror -I Source/PixelSideRL/Public -o /tmp/test_meta_progress Scripts/test_meta_progress.cpp Source/PixelSideRL/Private/Meta/MetaProgress.cpp
// /tmp/test_meta_progress .
#include "Meta/MetaProgress.h"

#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <limits>
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

bool Near(double A, double B)
{
	const double D = (A > B) ? (A - B) : (B - A);
	return D < 0.0001;
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

bool FileExists(const std::string& Path)
{
	std::ifstream In(Path, std::ios::binary);
	return static_cast<bool>(In);
}

bool DirHas(const std::string& Dir, bool (*Pred)(const std::string&))
{
	DIR* Handle = opendir(Dir.c_str());
	if (!Handle)
	{
		std::printf("FAIL opendir %s\n", Dir.c_str());
		++gFails;
		return false;
	}
	bool Found = false;
	while (const dirent* Entry = readdir(Handle))
	{
		const std::string Name = Entry->d_name;
		if (Name == "." || Name == "..")
		{
			continue;
		}
		if (Pred(Name))
		{
			std::printf("FAIL unexpected meta file %s\n", Name.c_str());
			++gFails;
			Found = true;
		}
	}
	closedir(Handle);
	return Found;
}

bool EndsWith(const std::string& Name, const char* Suffix)
{
	const size_t N = std::char_traits<char>::length(Suffix);
	return Name.size() >= N && Name.compare(Name.size() - N, N, Suffix) == 0;
}

bool IsUasset(const std::string& Name)
{
	return EndsWith(Name, ".uasset");
}

bool IsPerNodeAsset(const std::string& Name)
{
	return Name.rfind("DA_MetaNode_", 0) == 0;
}

bool ReplaceNth(std::string& Text, const std::string& From, const std::string& To, int Nth)
{
	size_t Pos = 0;
	for (int Index = 0; Index <= Nth; ++Index)
	{
		Pos = Text.find(From, Pos);
		if (Pos == std::string::npos)
		{
			return false;
		}
		if (Index == Nth)
		{
			Text.replace(Pos, From.size(), To);
			return true;
		}
		Pos += From.size();
	}
	return false;
}

const PixelSideMeta::FNode* Need(const PixelSideMeta::FProgress& Progress, const char* Id)
{
	const PixelSideMeta::FNode* Node = Progress.FindNode(Id);
	Expect(Node != nullptr, Id);
	return Node;
}

bool LoadLocked(PixelSideMeta::FProgress& Progress, const std::string& Root)
{
	std::string Error;
	const bool bEcon = Progress.LoadEconomyJson(ReadFile(Root + "/Content/Data/Meta/DA_Economy.json"), &Error);
	Expect(bEcon, "load economy");
	if (!bEcon)
	{
		std::printf("FAIL economy %s\n", Error.c_str());
	}
	const bool bNodes = Progress.LoadNodesJson(ReadFile(Root + "/Content/Data/Meta/DT_MetaNodes_MVP.json"), &Error);
	Expect(bNodes, "load nodes");
	if (!bNodes)
	{
		std::printf("FAIL nodes %s\n", Error.c_str());
	}
	const bool bPool = Progress.LoadAffixPoolJson(ReadFile(Root + "/Content/Data/Affixes/DT_Affix_MVP.json"), &Error);
	Expect(bPool, "load affix pool");
	if (!bPool)
	{
		std::printf("FAIL pool %s\n", Error.c_str());
	}
	return bEcon && bNodes && bPool;
}

void TestTables(const PixelSideMeta::FProgress& Progress)
{
	using namespace PixelSideMeta;
	Expect(Progress.HasEconomy() && Progress.HasNodes() && Progress.HasAffixPool(), "tables ready");
	Expect(Progress.Economy().ContractVersion == 3 && Progress.Economy().bLocked, "economy v3 locked");
	Expect(Progress.Economy().RunCurrencyId == "Scrap" && Progress.Economy().MetaCurrencyId == "Core", "currency ids");
	Expect(Progress.Economy().BaseClearBonus == 40, "base clear");
	Expect(Near(Progress.Economy().DeathMult, 0.5), "death mult");
	Expect(Progress.Economy().PerRoom == 8, "per room");
	Expect(Progress.Economy().BossBonus == 55, "boss");
	Expect(Near(Progress.Economy().CoinToScrap, 0.1), "coin to scrap");
	Expect(Near(Progress.Economy().ScrapToCore, 0.1), "scrap to core");
	Expect(Near(Progress.Economy().MetaCoreMult, 1.0), "base meta mult");
	Expect(Progress.Economy().SoftCapPerRun == 180, "soft cap");
	Expect(Progress.NamingRuleText() == kNamingRule, "naming rule");
	Expect(Progress.PoolPolicy() == kMvpAffixPoolPolicy, "pool policy");
	Expect(Progress.Nodes().size() == 5, "five nodes");
	const std::vector<FNodeView> Views = Progress.NodeViews();
	Expect(Views.size() == 5, "node views");
	Expect(Views[0].NodeId == "Vitality" && Views[0].DisplayName == "体质", "vitality view name");
	Expect(Views[0].OwnedRank == 0 && Views[0].MaxPurchases == 3 && Views[0].NextCostCore == 25, "vitality next");
	Expect(!Views[0].bMaxed && !Views[0].bLocked && !Views[0].bCanPurchase, "vitality needs core");
	Expect(Views[4].NodeId == "SidearmLuck" && Views[4].MaxPurchases == 1 && Views[4].NextCostCore == 45 && !Views[4].bLocked, "sidearm view");
	Expect(Progress.GetCore() == 0, "new core");
	Expect(Progress.AffixPool().size() == kMvpAffixIds.size(), "ten affixes");

	const char* Ids[] = {"Vitality", "Stash", "PeaFamiliar", "SalvageSense", "SidearmLuck"};
	for (size_t Index = 0; Index < 5; ++Index)
	{
		Expect(Progress.Nodes()[Index].NodeId == Ids[Index], "node order");
		Expect(Progress.Nodes()[Index].DataAssetName == FrozenDataAssetName(Ids[Index]), "da freeze");
		Expect(Progress.GetPurchasedRank(Ids[Index]) == 0, "rank zero");
		Expect(Progress.Nodes()[Index].UnlockAffixIds.empty(), "unlock list empty");
		Expect(Progress.Nodes()[Index].Prerequisites.empty(), "no prereq");
	}
	for (size_t Index = 0; Index < kMvpAffixIds.size(); ++Index)
	{
		Expect(Progress.AffixPool()[Index] == kMvpAffixIds[Index], "pool order");
	}
	for (const std::string& Id : Progress.AffixPool())
	{
		Expect(Id != "ExtendedMag" && Id != "ReflectShield" && Id != "QuakeLanding", "post-mvp stays out");
	}
	Expect(Progress.IgnoredUnlockAffixIds().empty(), "nothing ignored on the lock table");

	const FNode* Vitality = Need(Progress, "Vitality");
	const FNode* Stash = Need(Progress, "Stash");
	const FNode* Pea = Need(Progress, "PeaFamiliar");
	const FNode* Salvage = Need(Progress, "SalvageSense");
	const FNode* Sidearm = Need(Progress, "SidearmLuck");
	if (!Vitality || !Stash || !Pea || !Salvage || !Sidearm)
	{
		return;
	}
	Expect(Vitality->MaxPurchases == 3 && Vitality->CostCoreByRank.size() == 3, "vitality ranks");
	Expect(Vitality->CostCoreByRank[0] == 25 && Vitality->CostCoreByRank[1] == 50 && Vitality->CostCoreByRank[2] == 90, "vitality cost");
	Expect(Stash->CostCoreByRank[0] == 20 && Stash->CostCoreByRank[1] == 40 && Stash->CostCoreByRank[2] == 75, "stash cost");
	Expect(Stash->StartingScrapByRank.size() == 3, "stash scrap ranks");
	Expect(Stash->StartingScrapByRank[0] == 15 && Stash->StartingScrapByRank[1] == 30 && Stash->StartingScrapByRank[2] == 50, "stash scrap");
	Expect(Pea->CostCoreByRank[0] == 30 && Pea->CostCoreByRank[1] == 60 && Pea->CostCoreByRank[2] == 100, "pea cost");
	Expect(Pea->WeaponId == "Peashooter", "pea weapon id");
	Expect(Salvage->EconomyKey == "MetaCoreMult", "salvage key");
	Expect(Salvage->EconomyDeltaByRank.size() == 3, "salvage deltas");
	Expect(Near(Salvage->EconomyDeltaByRank[0], 0.05) && Near(Salvage->EconomyDeltaByRank[1], 0.10) && Near(Salvage->EconomyDeltaByRank[2], 0.15), "salvage values");
	Expect(Salvage->CostCoreByRank[0] == 35 && Salvage->CostCoreByRank[1] == 70 && Salvage->CostCoreByRank[2] == 110, "salvage cost");
	Expect(Sidearm->MaxPurchases == 1 && Sidearm->CostCoreByRank.size() == 1 && Sidearm->CostCoreByRank[0] == 45, "sidearm cost");
	Expect(Sidearm->FlagsByRank.size() == 1 && Sidearm->FlagsByRank[0].size() == 1, "sidearm flag rank");
	Expect(Sidearm->FlagsByRank[0][0] == "GuaranteeWeaponRoomAfterRoom2_Act1", "sidearm flag");
}

void TestPurchase(PixelSideMeta::FProgress& Progress)
{
	using namespace PixelSideMeta;
	std::string Reason;
	Expect(!Progress.CanPurchase("Stash", &Reason) && Reason == "core", "stash needs core");
	Expect(!Progress.Purchase("NoSuch", &Reason) && Reason == "unknown", "unknown node");
	Expect(Progress.GetPurchasedRank("NoSuch") == 0, "unknown rank");

	Progress.AddCore(19, "short");
	Expect(Progress.GetCore() == 19 && Progress.LastCoreReason() == "short", "add 19");
	Expect(!Progress.Purchase("Stash", &Reason) && Reason == "core" && Progress.GetCore() == 19, "short does not spend");
	Expect(Progress.GetPurchasedRank("Stash") == 0, "stash still 0");

	Progress.AddCore(1, "topup");
	Expect(Progress.Purchase("Stash", &Reason) && Reason.empty(), "buy stash 1");
	Expect(Progress.GetCore() == 0 && Progress.GetPurchasedRank("Stash") == 1, "stash rank 1");
	FRunStart Start = Progress.BuildRunStartContext();
	Expect(Start.StartingScrap == 15, "stash scrap 15");
	Expect(Start.StartWeaponId.empty(), "no weapon grant");
	Expect(Start.PermanentModifiers.empty(), "stash has no modifier");
	Expect(Start.Flags.empty(), "no flag yet");
	Expect(Near(Start.MetaCoreMult, 1.0), "mult still 1");
	Expect(Start.bUnlockAffixIgnored && Start.bAffixPoolReady, "pool flags");
	Expect(Start.UnlockedAffixIds.size() == 10, "ten open");
	Expect(SeedRunScrap(Start.StartingScrap) == 15, "seed run scrap");
	int RunScrap = 0;
	Expect(RunScrap == 0, "run scrap stays 0 until reset");
	RunScrap = SeedRunScrap(Start.StartingScrap);
	Expect(RunScrap == 15, "reset writes starting scrap");
	Expect(Progress.Describe().find("StartingScrap=15") != std::string::npos, "describe scrap");
	Expect(Progress.Describe().find("Stash=1") != std::string::npos, "describe rank");

	Progress.AddCore(40 + 75, "finish stash");
	Expect(Progress.Purchase("Stash", &Reason), "stash 2");
	Expect(Progress.Purchase("Stash", &Reason), "stash 3");
	Expect(Progress.GetPurchasedRank("Stash") == 3 && Progress.GetCore() == 0, "stash max");
	Start = Progress.BuildRunStartContext();
	Expect(Start.StartingScrap == 50, "stash scrap is the rank value, not a sum");
	Expect(!Progress.Purchase("Stash", &Reason) && Reason == "max" && Progress.GetCore() == 0, "no fourth stash");
	{
		const std::vector<FNodeView> Maxed = Progress.NodeViews();
		Expect(Maxed[1].NodeId == "Stash" && Maxed[1].bMaxed && Maxed[1].OwnedRank == 3 && Maxed[1].NextCostCore == 0 && !Maxed[1].bCanPurchase, "stash view maxed");
	}

	Progress.ResetSlot();
	Expect(Progress.GetCore() == 0 && Progress.GetPurchasedRank("Stash") == 0, "reset slot");
	Expect(Progress.BuildRunStartContext().StartingScrap == 0, "no stash is 0 scrap");

	Progress.AddCore(25 + 50 + 30, "vita pea");
	{
		const std::vector<FNodeView> Ready = Progress.NodeViews();
		Expect(Ready[0].bCanPurchase && Ready[0].NextCostCore == 25 && Ready[0].OwnedRank == 0, "vitality view affordable");
	}
	Expect(Progress.Purchase("Vitality", &Reason) && Progress.Purchase("Vitality", &Reason), "vitality 2");
	Expect(Progress.Purchase("PeaFamiliar", &Reason), "pea 1");
	{
		const std::vector<FNodeView> After = Progress.NodeViews();
		Expect(After[0].OwnedRank == 2 && After[0].NextCostCore == 90 && !After[0].bMaxed && !After[0].bCanPurchase, "vitality view rank 2");
		Expect(After[2].NodeId == "PeaFamiliar" && After[2].OwnedRank == 1 && After[2].NextCostCore == 60, "pea view rank 1");
	}
	Start = Progress.BuildRunStartContext();
	Expect(Start.StartingScrap == 0, "vitality does not grant scrap");
	Expect(Start.PermanentModifiers.size() == 2, "two modifiers");
	if (Start.PermanentModifiers.size() == 2)
	{
		Expect(Start.PermanentModifiers[0].Stat == "MaxHP" && Start.PermanentModifiers[0].Op == "Add", "hp add");
		Expect(Near(Start.PermanentModifiers[0].Value, 20.0) && Start.PermanentModifiers[0].WeaponTag.empty(), "hp +20 not stacked");
		Expect(Start.PermanentModifiers[1].Stat == "Damage" && Start.PermanentModifiers[1].Op == "Mul", "pea mul");
		Expect(Near(Start.PermanentModifiers[1].Value, 0.04) && Start.PermanentModifiers[1].WeaponTag == "Peashooter", "pea tag");
	}
	Expect(Start.StartWeaponId.empty(), "pea is not StartWeaponId");

	Progress.ResetSlot();
	Progress.AddCore(35 + 70 + 45, "sense");
	Expect(Progress.Purchase("SalvageSense", &Reason) && Progress.Purchase("SalvageSense", &Reason), "salvage 2");
	Expect(Near(Progress.GetMetaCoreMult(), 1.10), "mult 1.10");
	Expect(Progress.Purchase("SidearmLuck", &Reason), "sidearm");
	Start = Progress.BuildRunStartContext();
	Expect(Start.Flags.size() == 1 && Start.Flags[0] == "GuaranteeWeaponRoomAfterRoom2_Act1", "flag on");
	Expect(!Progress.Purchase("SidearmLuck", &Reason) && Reason == "max", "sidearm once");
	Progress.ResetSlot();
	Expect(Progress.BuildRunStartContext().Flags.empty(), "flag off at rank 0");
	Expect(Near(Progress.GetMetaCoreMult(), 1.0), "mult back to base");

	Progress.AddCore(35 + 70 + 110, "salvage 3");
	Expect(Progress.Purchase("SalvageSense", &Reason), "s1");
	Expect(Progress.Purchase("SalvageSense", &Reason), "s2");
	Expect(Progress.Purchase("SalvageSense", &Reason), "s3");
	Expect(Near(Progress.GetMetaCoreMult(), 1.15), "mult 1.15");

	const int CoreBefore = Progress.GetCore();
	const FCorePreview Clear = Progress.PreviewCoreGain(true, 0, 0, 0);
	Expect(Clear.bReady && !Clear.bAppliedToCore && Clear.Core == 46, "clear * 1.15");
	Expect(Progress.GetCore() == CoreBefore, "preview does not credit core");
	Progress.ResetSlot();
	Expect(Progress.PreviewCoreGain(true, 0, 0, 0).Core == 40, "clear base");
	Expect(Progress.PreviewCoreGain(false, 0, 0, 0).Core == 20, "death base");
	Expect(Progress.PreviewCoreGain(true, 2, 0, 0).Core == 56, "two rooms");
	Expect(Progress.PreviewCoreGain(true, 0, 1, 0).Core == 95, "boss");
	Expect(Progress.PreviewCoreGain(true, 0, 0, 25).Core == 42, "scrap remain");
	Expect(Progress.PreviewCoreGain(true, 100, 1, 999).Core == 180, "soft cap");
	Expect(Progress.GetCore() == 0, "preview still does not pay");

	Progress.AddCore(-5, "neg");
	Expect(Progress.GetCore() == 0, "core floor");
	Progress.AddCore(10, "ten");
	Progress.AddCore(-3, "refund");
	Expect(Progress.GetCore() == 7, "partial refund");
	Progress.AddCore(std::numeric_limits<int>::max(), "cap");
	Expect(Progress.GetCore() == std::numeric_limits<int>::max(), "core ceiling");
	Expect(SeedRunScrap(0) == 0 && SeedRunScrap(-4) == 0, "seed clamps");
}

void TestMutations(const std::string& Nodes, const std::string& Economy, const std::string& Affix)
{
	using namespace PixelSideMeta;
	std::string Pierce = Nodes;
	Expect(ReplaceNth(Pierce, "\"Stat\": \"MaxHP\"", "\"Stat\": \"Pierce\"", 0), "splice pierce");
	FProgress Blocked;
	std::string Error;
	Expect(!Blocked.LoadNodesJson(Pierce, &Error), "pierce rejected");
	Expect(Error.find("forbidden stat Pierce") != std::string::npos, "pierce reason");

	std::string Policy = Nodes;
	Expect(ReplaceNth(Policy, kMvpAffixPoolPolicy, "UnlockAffix", 0), "splice policy");
	Expect(!Blocked.LoadNodesJson(Policy, &Error), "policy rejected");
	Expect(Error.find("mvpAffixPoolPolicy") != std::string::npos, "policy reason");

	std::string Name = Nodes;
	Expect(ReplaceNth(Name, kNamingRule, "DA_MetaNode_X", 0), "splice rule");
	Expect(!Blocked.LoadNodesJson(Name, &Error), "rule rejected");

	std::string BadEcon = Economy;
	Expect(ReplaceNth(BadEcon, "\"contractVersion\": 3", "\"contractVersion\": 2", 0), "splice econ");
	Expect(!Blocked.LoadEconomyJson(BadEcon, &Error), "econ version rejected");

	std::string Unlock = Nodes;
	Expect(ReplaceNth(Unlock, "\"UnlockAffixIds\": []", "\"UnlockAffixIds\": [\"Piercer\", \"TwinShot\"]", 0), "splice unlock");
	FProgress Ignore;
	Expect(Ignore.LoadNodesJson(Unlock, &Error), "unlock list still loads");
	Expect(Ignore.LoadAffixPoolJson(Affix, &Error), "pool still loads");
	const std::vector<std::string> Ignored = Ignore.IgnoredUnlockAffixIds();
	Expect(Ignored.size() == 2, "two ignored ids");
	if (Ignored.size() == 2)
	{
		Expect(Ignored[0] == "Vitality:Piercer" && Ignored[1] == "Vitality:TwinShot", "ignored names");
	}
	const FRunStart Start = Ignore.BuildRunStartContext();
	Expect(Start.UnlockedAffixIds.size() == 10, "unlock does not shrink the pool");
	Expect(Start.bUnlockAffixIgnored, "flag stays ignored");
	int Piercer = 0;
	int Twin = 0;
	for (const std::string& Id : Start.UnlockedAffixIds)
	{
		Piercer += Id == "Piercer" ? 1 : 0;
		Twin += Id == "TwinShot" ? 1 : 0;
	}
	Expect(Piercer == 1 && Twin == 1, "piercer and twinshot are in the ten once");

	std::string Prereq = Nodes;
	Expect(ReplaceNth(Prereq, "\"Prerequisites\": []", "\"Prerequisites\": [\"Vitality\"]", 1), "splice prereq");
	FProgress Gated;
	Expect(Gated.LoadNodesJson(Prereq, &Error), "prereq loads");
	Gated.AddCore(1000, "lab");
	std::string Reason;
	Expect(!Gated.CanPurchase("Stash", &Reason) && Reason == "prereq:Vitality", "stash waits");
	{
		bool bLocked = false;
		for (const FNodeView& View : Gated.NodeViews())
		{
			if (View.NodeId == "Stash")
			{
				bLocked = View.bLocked && !View.bCanPurchase && !View.bMaxed;
			}
		}
		Expect(bLocked, "stash view locked");
	}
	Expect(Gated.Purchase("Vitality", &Reason), "buy vitality");
	Expect(Gated.Purchase("Stash", &Reason), "stash after prereq");
	Expect(Gated.BuildRunStartContext().StartingScrap == 15, "gated stash still pays scrap");
}

} // namespace

int main(int argc, char** argv)
{
	const std::string Root = argc > 1 ? argv[1] : ".";
	const std::string MetaDir = Root + "/Content/Data/Meta";
	Expect(!FileExists(MetaDir + "/DA_MetaNode_Vitality.uasset"), "no vitality uasset");
	Expect(!FileExists(MetaDir + "/DA_MetaNode_Stash.uasset"), "no stash uasset");
	Expect(!FileExists(MetaDir + "/DA_Economy.uasset"), "no economy uasset");
	DirHas(MetaDir, IsUasset);
	DirHas(MetaDir, IsPerNodeAsset);

	const std::string Economy = ReadFile(MetaDir + "/DA_Economy.json");
	const std::string Nodes = ReadFile(MetaDir + "/DT_MetaNodes_MVP.json");
	const std::string Affix = ReadFile(Root + "/Content/Data/Affixes/DT_Affix_MVP.json");
	Expect(Economy.find("\"contractVersion\": 3") != std::string::npos, "economy text v3");
	Expect(Nodes.find("\"locked\": true") != std::string::npos, "nodes locked text");

	PixelSideMeta::FProgress Progress;
	if (LoadLocked(Progress, Root))
	{
		TestTables(Progress);
		TestPurchase(Progress);
	}
	TestMutations(Nodes, Economy, Affix);

	if (gFails == 0)
	{
		std::printf("ok meta progress\n");
		return 0;
	}
	std::printf("%d failed\n", gFails);
	return 1;
}
