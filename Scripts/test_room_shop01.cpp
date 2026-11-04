// Host-side Room_Shop_01 entry checks. No Unreal headers.
// g++ -std=c++17 -Wall -Wextra -Werror -I Source/PixelSideRL/Public -o /tmp/test_room_shop01 Scripts/test_room_shop01.cpp Source/PixelSideRL/Private/Rooms/PixelSideRoomFlow.cpp
// /tmp/test_room_shop01 .
#include "Rooms/PixelSideRoomFlow.h"

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

bool Contains(const std::string& Text, const char* Needle)
{
	return Text.find(Needle) != std::string::npos;
}

int NonEmptyLines(const std::string& Text)
{
	int Count = 0;
	std::string Line;
	for (char C : Text)
	{
		if (C == '\n')
		{
			if (!Line.empty() && Line != "\r")
			{
				++Count;
			}
			Line.clear();
		}
		else
		{
			Line.push_back(C);
		}
	}
	if (!Line.empty() && Line != "\r")
	{
		++Count;
	}
	return Count;
}
}

int main(int argc, char** argv)
{
	using namespace PixelSideRoom;
	const std::string Root = argc > 1 ? argv[1] : ".";

	Expect(IsShopRoom(RoomShop01Id, RoomTypeShop), "shop id and type");
	Expect(IsShopRoom(RoomShop01Id, "Branch"), "shop id alone");
	Expect(IsShopRoom("Room_Other", RoomTypeShop), "shop type alone");
	Expect(!IsShopRoom(RoomCombat01Id, "Combat"), "combat is not a shop");
	Expect(IsP07AcceptanceRoom(RoomCombat01Id), "combat 01 still the only p07 room");
	Expect(!IsP07AcceptanceRoom(RoomShop01Id), "shop stays off the combat clock");

	const std::string MetaText = ReadFile(Root + "/Content/Data/Rooms/Room_Shop_01_meta.csv");
	const std::string SpawnText = ReadFile(Root + "/Content/Data/Rooms/Room_Shop_01_spawns.csv");
	const std::string GeoText = ReadFile(Root + "/Content/Data/Rooms/Room_Shop_01_geometry.csv");
	const std::string Host = ReadFile(Root + "/Source/PixelSideRL/Private/Rooms/PixelShopRoomHost.cpp");
	const std::string Director = ReadFile(Root + "/Source/PixelSideRL/Private/Rooms/PixelRoomDirector.cpp");
	const std::string Character = ReadFile(Root + "/Source/PixelSideRL/Private/Combat/PixelSideCharacter.cpp");
	const std::string Mode = ReadFile(Root + "/Source/PixelSideRL/Private/Combat/PixelSideGameMode.cpp");
	if (gFails != 0)
	{
		std::printf("%d failed\n", gFails);
		return 1;
	}

	Expect(NonEmptyLines(SpawnText) == 1, "spawn csv is header only");

	FRoomMeta Meta;
	std::string Error;
	Expect(LoadRoomMetaCsv(MetaText, Meta, Error), "shop meta loads");
	Expect(Meta.RoomId == RoomShop01Id, "room id");
	Expect(Meta.Type == RoomTypeShop, "type shop");
	Expect(Meta.MaxAlive == 0, "no alive budget");
	Expect(!Meta.bAffixOnClear, "no affix");
	Expect(Meta.RequireClearExit.empty(), "no require-clear exit");
	Expect(Meta.ClearRule == "None", "clear rule none");
	Expect(Meta.DesignWidth > 1200.f && Meta.DesignWidth < 1300.f, "width 1280");

	const FShopEntry Entry = MakeShopEntry(Meta);
	Expect(Entry.bOpenRunShop, "entry opens the run shop");
	Expect(Entry.ShelfLayer == ShopShelfLayer && Entry.ShelfLayer == 1, "shelf layer 1");
	Expect(!Entry.bLockDoors, "doors stay unlocked");
	Expect(!Entry.bAffixOnClear, "entry does not ask for affix");
	Expect(!Entry.bForcePurchase, "no forced buy");

	FRoomMeta CombatMeta;
	std::vector<FSpawnRow> CombatSpawns;
	Expect(!LoadRoomTables(MetaText, SpawnText, "", CombatMeta, CombatSpawns, Error), "combat loader rejects shop");
	Expect(Error.find("max_alive") != std::string::npos || Error.find("require_clear") != std::string::npos, "rejection is the combat gate");
	Expect(!LoadRoomTables(MetaText, SpawnText, RoomCombat01Id, CombatMeta, CombatSpawns, Error), "shop is not the p07 room");

	FRoomMeta Other;
	Other.RoomId = RoomCombat01Id;
	Other.Type = "Combat";
	Expect(!MakeShopEntry(Other).bOpenRunShop, "combat entry does not open shop");

	std::vector<FGeometryRow> Geo;
	Expect(LoadGeometryCsv(GeoText, Geo, Error), "geometry");
	bool bSawShelf = false;
	bool bSawCounter = false;
	bool bLockedExit = false;
	for (const FGeometryRow& Row : Geo)
	{
		if (Row.Id == "P_shelf")
		{
			float X0 = 0.f;
			float X1 = 0.f;
			ResolveHorizontalSpan(Row, X0, X1);
			bSawShelf = X0 > 700.f && X0 < 740.f && X1 > 1000.f && X1 < 1060.f && Row.TopY > 27.f && Row.TopY < 29.f;
		}
		if (Row.Id == "CV_counter")
		{
			bSawCounter = Row.XCenter > 390.f && Row.XCenter < 410.f;
		}
		if (Row.Kind == "Exit" && Row.Rule == "RequireClear")
		{
			bLockedExit = true;
		}
	}
	Expect(bSawShelf, "shelf span top 28");
	Expect(bSawCounter, "counter anchor");
	Expect(!bLockedExit, "geometry does not require clear");

	Expect(Contains(Host, "->OpenShop("), "host calls OpenShop");
	Expect(Contains(Host, "GetShopPresenter"), "host uses the presenter");
	Expect(Contains(Host, "CollectShelfOffers"), "host reads the DA shelf");
	Expect(!Contains(Host, "OpenShopDemo"), "host does not open the lab demo");
	Expect(!Contains(Director, "OpenShop"), "combat director does not open the shop");
	Expect(Contains(Director, "Room_Combat_01"), "director still names combat 01");
	Expect(Contains(Character, "DebugShopWidget->OpenShopDemo()"), "F11 still opens the lab widget");
	Expect(Contains(Mode, "ShouldDriveRoomCombat01()"), "combat drive remains");
	Expect(Contains(Mode, "Room_Shop_01 drive is ignored while Room_Combat_01"), "combat wins if both drives are on");

	if (gFails != 0)
	{
		std::printf("%d failed\n", gFails);
		return 1;
	}
	std::printf("room shop 01 ok\n");
	return 0;
}
