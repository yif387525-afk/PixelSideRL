// Host-side event op checks. No Unreal headers.
// g++ -std=c++17 -Wall -Wextra -Werror -I Source/PixelSideRL/Public -o /tmp/test_event_ops Scripts/test_event_ops.cpp Source/PixelSideRL/Private/Event/EventOps.cpp
// /tmp/test_event_ops .
#include "Event/EventOps.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

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

bool FileExists(const std::string& Path)
{
	std::ifstream In(Path, std::ios::binary);
	return static_cast<bool>(In);
}

std::string EventJson(const std::string& Id, const std::string& Effects, bool bAffix, bool bCombat)
{
	return std::string("{\"EventId\":\"") + Id
		+ "\",\"DisplayName\":\"T\",\"PromptBody\":\"P\",\"SkipHint\":\"S\","
		+ "\"bAllowDoorSkip\":true,\"bCombatAllowed\":" + (bCombat ? "true" : "false")
		+ ",\"bAffixOnResolve\":" + (bAffix ? "true" : "false")
		+ ",\"Choices\":[{\"ChoiceId\":\"Only\",\"Label\":\"L\",\"Effects\":[" + Effects + "]}],"
		+ "\"DataAssetName\":\"DA_Event_" + Id + "\",\"contractVersion\":3}";
}

PixelSideEvent::FResolveResult ResolveOnly(
	PixelSideEvent::FCatalog& Catalog,
	const std::string& EventId,
	PixelSideEvent::FRunScrapWallet& Wallet,
	PixelSideEvent::FEventVitals& Vitals)
{
	return Catalog.Resolve(EventId, "Only", Wallet, Vitals);
}

void TestAbandoned(const std::string& Root)
{
	using namespace PixelSideEvent;
	const std::string Dir = Root + "/Content/Data/Events/";
	Expect(!FileExists(Dir + "DA_Event_AbandonedCache.uasset"), "no abandoned cache uasset");

	const std::string Schema = ReadFile(Dir + "SCHEMA_EventDefinition.json");
	Expect(Schema.find("GrantScrap") != std::string::npos, "schema GrantScrap");
	Expect(Schema.find("DamageSelf") != std::string::npos, "schema DamageSelf");
	Expect(Schema.find("HealSelf") != std::string::npos, "schema HealSelf");
	Expect(Schema.find("None") != std::string::npos, "schema None");
	Expect(Schema.find("CurrentHP") != std::string::npos, "schema hp floor note");

	FCatalog Catalog;
	std::string Error;
	Expect(Catalog.LoadEventJson(ReadFile(Dir + "DA_Event_AbandonedCache.json"), &Error), "load abandoned");
	if (!Error.empty())
	{
		std::printf("FAIL load detail %s\n", Error.c_str());
		++gFails;
	}
	Expect(Catalog.LoadPoolJson(ReadFile(Dir + "DT_Event_MVP.json"), &Error), "load pool");
	Expect(Catalog.ValidatePool(&Error), "pool matches DA_Event_<EventId>");
	Expect(Catalog.PoolNamingRule() == "DA_Event_<EventId>", "naming rule frozen");

	const FEventDefinition* Def = Catalog.Find("AbandonedCache");
	Expect(Def != nullptr, "find abandoned");
	if (!Def)
	{
		return;
	}
	Expect(Def->DataAssetName == "DA_Event_AbandonedCache", "da freeze");
	Expect(Def->DataAssetName == FrozenDataAssetName(Def->EventId), "da helper");
	Expect(!Def->bAffixOnResolve, "no affix on resolve");
	Expect(!Def->bCombatAllowed, "no combat");
	Expect(Def->Choices.size() == 2, "two choices");
	Expect(Def->Choices[0].ChoiceId == "CarefulLoot", "careful id");
	Expect(Def->Choices[0].Effects.size() == 1 && Def->Choices[0].Effects[0].Kind == EOp::GrantScrap, "careful op");
	Expect(Near(static_cast<float>(Def->Choices[0].Effects[0].Value), 25.f), "careful 25");
	Expect(Def->Choices[1].ChoiceId == "ForceLoot", "force id");
	Expect(Def->Choices[1].Effects.size() == 2, "force two effects");
	Expect(Def->Choices[1].Effects[0].Kind == EOp::DamageSelf, "force damage first");
	Expect(Near(static_cast<float>(Def->Choices[1].Effects[0].Value), 15.f), "force 15");
	Expect(Def->Choices[1].Effects[1].Kind == EOp::GrantScrap, "force scrap second");
	Expect(Near(static_cast<float>(Def->Choices[1].Effects[1].Value), 50.f), "force 50");

	const int Core = 7;
	FRunScrapWallet Wallet;
	FEventVitals Vitals;
	Vitals.CurrentHP = 30.f;
	Vitals.MaxHP = 30.f;
	const FResolveResult Careful = Catalog.Resolve("AbandonedCache", "CarefulLoot", Wallet, Vitals);
	Expect(Careful.bOk, "careful ok");
	Expect(Careful.Steps.size() == 1, "careful one step");
	Expect(Wallet.Scrap == 25 && Careful.Scrap == 25, "careful +25");
	Expect(Near(Vitals.CurrentHP, 30.f), "careful hp unchanged");
	Expect(!Careful.bAffixChoiceRequested && !Careful.bAffixOnResolve, "careful no affix");
	Expect(!Careful.bCombatStarted && !Careful.bCombatAllowed, "careful no combat");
	Expect(Careful.Notes.empty(), "careful no notes");
	Expect(Core == 7, "core untouched by careful");

	Wallet.Scrap = 10;
	Vitals.CurrentHP = 30.f;
	const FResolveResult CarefulAdd = Catalog.Resolve("AbandonedCache", "CarefulLoot", Wallet, Vitals);
	Expect(CarefulAdd.bOk && Wallet.Scrap == 35, "careful adds to wallet");

	Wallet.Scrap = 0;
	Vitals.CurrentHP = 30.f;
	Vitals.MaxHP = 30.f;
	const FResolveResult Force = Catalog.Resolve("AbandonedCache", "ForceLoot", Wallet, Vitals);
	Expect(Force.bOk && Force.Steps.size() == 2, "force two steps one call");
	Expect(Force.Steps[0].Kind == EOp::DamageSelf, "force step0 damage");
	Expect(Force.Steps[0].ScrapBefore == 0 && Force.Steps[0].ScrapAfter == 0, "damage does not grant scrap");
	Expect(Near(Force.Steps[0].HpBefore, 30.f) && Near(Force.Steps[0].HpAfter, 15.f), "force -15 hp");
	Expect(Force.Steps[1].Kind == EOp::GrantScrap, "force step1 scrap");
	Expect(Near(Force.Steps[1].HpBefore, 15.f) && Near(Force.Steps[1].HpAfter, 15.f), "scrap keeps damaged hp");
	Expect(Force.Steps[1].ScrapBefore == 0 && Force.Steps[1].ScrapAfter == 50, "force +50 after damage");
	Expect(Wallet.Scrap == 50 && Near(Vitals.CurrentHP, 15.f), "force end state");
	Expect(Vitals.CurrentHP >= 1.f, "force alive");
	Expect(!Force.bAffixChoiceRequested && !Force.bCombatStarted, "force no affix or combat");
	Expect(Core == 7, "core untouched by force");

	Wallet.Scrap = 0;
	Vitals.CurrentHP = 10.f;
	Vitals.MaxHP = 30.f;
	const FResolveResult Floor = Catalog.Resolve("AbandonedCache", "ForceLoot", Wallet, Vitals);
	Expect(Floor.bOk && Floor.Steps.size() == 2, "floor steps");
	Expect(Near(Floor.Steps[0].HpBefore, 10.f) && Near(Floor.Steps[0].HpAfter, 1.f), "10-15 floors at 1");
	Expect(Floor.Steps[0].Message.find("floored") != std::string::npos, "floor message");
	Expect(Floor.Steps[0].ScrapAfter == 0 && Floor.Steps[1].ScrapAfter == 50, "scrap after floor");
	Expect(Near(Floor.Steps[1].HpAfter, 1.f) && Near(Vitals.CurrentHP, 1.f), "hp stays 1");
	Expect(Vitals.CurrentHP >= 1.f, "not dead");

	Wallet.Scrap = 3;
	Vitals.CurrentHP = 1.f;
	const FResolveResult Stay = Catalog.Resolve("AbandonedCache", "ForceLoot", Wallet, Vitals);
	Expect(Near(Stay.Steps[0].HpAfter, 1.f) && Stay.Scrap == 53, "hp 1 stays 1 and still pays scrap");

	Wallet.Scrap = 4;
	Vitals.CurrentHP = 0.f;
	const int ScrapBeforeDead = Wallet.Scrap;
	const FResolveResult Dead = Catalog.Resolve("AbandonedCache", "ForceLoot", Wallet, Vitals);
	Expect(Near(Dead.Steps[0].HpAfter, 0.f), "dead stays dead");
	Expect(Dead.Steps[0].Message.find("already dead") != std::string::npos, "no revive");
	Expect(Wallet.Scrap == ScrapBeforeDead + 50, "dead still receives scrap");

	Wallet.Scrap = 1;
	Vitals.CurrentHP = 22.f;
	const FResolveResult Missing = Catalog.Resolve("AbandonedCache", "Nope", Wallet, Vitals);
	Expect(!Missing.bOk && Wallet.Scrap == 1 && Near(Vitals.CurrentHP, 22.f), "unknown choice is a no-op");
	const FResolveResult MissingEvent = Catalog.Resolve("NoSuch", "CarefulLoot", Wallet, Vitals);
	Expect(!MissingEvent.bOk && Wallet.Scrap == 1 && Near(Vitals.CurrentHP, 22.f), "unknown event is a no-op");

	std::string DupError;
	Expect(!Catalog.LoadEventJson(ReadFile(Dir + "DA_Event_AbandonedCache.json"), &DupError), "duplicate rejected");
	Expect(Catalog.Events().size() == 1, "duplicate did not append");
	Expect(Core == 7, "core still untouched");
}

void TestSyntheticOps()
{
	using namespace PixelSideEvent;
	FCatalog Catalog;
	std::string Error;
	Expect(Catalog.LoadEventJson(EventJson("TestNone", "{\"Op\":\"None\",\"Value\":25}", false, false), &Error), "load none");
	Expect(Catalog.LoadEventJson(EventJson("TestUnknown", "{\"Op\":\"GrantCore\",\"Value\":99}", false, false), &Error), "load unknown");
	Expect(Catalog.LoadEventJson(
		EventJson("TestOrder", "{\"Op\":\"DamageSelf\",\"Value\":5},{\"Op\":\"GrantCore\",\"Value\":1},{\"Op\":\"GrantScrap\",\"Value\":7}", false, false),
		&Error), "load order");
	Expect(Catalog.LoadEventJson(EventJson("TestHeal", "{\"Op\":\"HealSelf\",\"Value\":15}", false, false), &Error), "load heal");
	Expect(Catalog.LoadEventJson(EventJson("TestFlags", "{\"Op\":\"None\"}", true, true), &Error), "load flags");
	Expect(!Catalog.LoadEventJson("{\"EventId\":\"Bad\"}", &Error), "malformed rejected");

	const int Core = 11;
	FRunScrapWallet Wallet;
	Wallet.Scrap = 4;
	FEventVitals Vitals;
	Vitals.CurrentHP = 12.f;
	Vitals.MaxHP = 30.f;
	const FResolveResult None = ResolveOnly(Catalog, "TestNone", Wallet, Vitals);
	Expect(None.bOk && None.Steps.size() == 1 && None.Steps[0].Kind == EOp::None, "none step");
	Expect(None.Steps[0].bApplied && !None.Steps[0].bUnknown, "none applied");
	Expect(Wallet.Scrap == 4 && Near(Vitals.CurrentHP, 12.f), "none no side effects");
	Expect(!None.bAffixChoiceRequested && !None.bCombatStarted, "none no extra systems");

	const FResolveResult Unknown = ResolveOnly(Catalog, "TestUnknown", Wallet, Vitals);
	Expect(Unknown.bOk && Unknown.Steps.size() == 1 && Unknown.Steps[0].bUnknown, "unknown flagged");
	Expect(!Unknown.Steps[0].bApplied, "unknown not applied");
	Expect(Unknown.Steps[0].Message.find("GrantCore") != std::string::npos, "unknown names the op");
	Expect(Unknown.Steps[0].Message.find("unknown") != std::string::npos, "unknown logged");
	Expect(Wallet.Scrap == 4 && Near(Vitals.CurrentHP, 12.f) && Core == 11, "unknown no side effects");

	Wallet.Scrap = 0;
	Vitals.CurrentHP = 20.f;
	const FResolveResult Order = ResolveOnly(Catalog, "TestOrder", Wallet, Vitals);
	Expect(Order.bOk && Order.Steps.size() == 3, "unknown does not abort the list");
	Expect(Order.Steps[0].Kind == EOp::DamageSelf && Near(Order.Steps[0].HpAfter, 15.f) && Order.Steps[0].ScrapAfter == 0, "order damage");
	Expect(Order.Steps[1].bUnknown && Order.Steps[1].ScrapAfter == 0 && Near(Order.Steps[1].HpAfter, 15.f), "order unknown");
	Expect(Order.Steps[2].Kind == EOp::GrantScrap && Order.Steps[2].ScrapAfter == 7 && Near(Order.Steps[2].HpAfter, 15.f), "order scrap last");
	Expect(Core == 11, "order did not mint core");

	Wallet.Scrap = 0;
	Vitals.CurrentHP = 20.f;
	Vitals.MaxHP = 30.f;
	const FResolveResult Heal = ResolveOnly(Catalog, "TestHeal", Wallet, Vitals);
	Expect(Heal.bOk && Near(Vitals.CurrentHP, 30.f), "heal clamps to max");
	Expect(Heal.Steps[0].Message.find("MaxHP") != std::string::npos, "heal clamp message");
	Expect(Wallet.Scrap == 0, "heal does not pay scrap");

	Vitals.CurrentHP = 10.f;
	Vitals.MaxHP = 0.f;
	const FResolveResult HealOpen = ResolveOnly(Catalog, "TestHeal", Wallet, Vitals);
	Expect(Near(Vitals.CurrentHP, 25.f), "heal without max is uncapped");

	Vitals.CurrentHP = 10.f;
	Vitals.MaxHP = 30.f;
	const FResolveResult HealRoom = ResolveOnly(Catalog, "TestHeal", Wallet, Vitals);
	Expect(Near(Vitals.CurrentHP, 25.f), "heal under max");

	Vitals.CurrentHP = 0.f;
	const FResolveResult HealDead = ResolveOnly(Catalog, "TestHeal", Wallet, Vitals);
	Expect(Near(Vitals.CurrentHP, 0.f), "heal does not revive");

	Wallet.Scrap = 2;
	Vitals.CurrentHP = 9.f;
	Vitals.MaxHP = 30.f;
	const FResolveResult Flags = ResolveOnly(Catalog, "TestFlags", Wallet, Vitals);
	Expect(Flags.bOk && Flags.bAffixOnResolve && Flags.bCombatAllowed, "flags are read");
	Expect(!Flags.bAffixChoiceRequested && !Flags.bCombatStarted, "mvp does not act on those flags");
	Expect(Flags.Notes.size() == 2, "both flags noted");
	Expect(Wallet.Scrap == 2 && Near(Vitals.CurrentHP, 9.f), "flagged none still empty");
	Expect(Core == 11, "flags did not touch core");
}
}

int main(int argc, char** argv)
{
	const std::string Root = (argc > 1) ? argv[1] : ".";
	TestAbandoned(Root);
	TestSyntheticOps();
	if (gFails != 0)
	{
		std::printf("%d failed\n", gFails);
		return 1;
	}
	std::printf("ok event ops\n");
	return 0;
}
