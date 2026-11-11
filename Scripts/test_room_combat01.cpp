// Host-side P0.7 Room_Combat_01 checks. No Unreal headers.
// g++ -std=c++17 -Wall -Wextra -Werror -I Source/PixelSideRL/Public -o /tmp/test_room_combat01 Scripts/test_room_combat01.cpp Source/PixelSideRL/Private/Rooms/PixelSideRoomFlow.cpp
// /tmp/test_room_combat01 .
#include "Rooms/PixelSideRoomFlow.h"

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

bool Contains(const std::string& Text, const char* Needle)
{
	return Text.find(Needle) != std::string::npos;
}

const PixelSideRoom::FSpawnOrder* FindOrder(const std::vector<PixelSideRoom::FSpawnOrder>& Orders, const char* Id)
{
	for (const auto& Order : Orders)
	{
		if (Order.SpawnId == Id)
		{
			return &Order;
		}
	}
	return nullptr;
}

void Ack(PixelSideRoom::FRoomFlow& Flow, const std::vector<PixelSideRoom::FSpawnOrder>& Orders)
{
	for (const auto& Order : Orders)
	{
		Flow.NotifySpawned(Order.SpawnId);
	}
}

std::vector<PixelSideRoom::FSpawnOrder> ArmAndTake(PixelSideRoom::FRoomFlow& Flow)
{
	Flow.NotifyEntered();
	Flow.Tick(Flow.FirstWaveDelay());
	return Flow.ConsumeSpawnOrders();
}
}

int main(int argc, char** argv)
{
	using namespace PixelSideRoom;
	const std::string Root = argc > 1 ? argv[1] : ".";

	Expect(FirstWaveDelayDefaultSec >= FirstWaveDelayMinSec && FirstWaveDelayDefaultSec <= FirstWaveDelayMaxSec, "delay window");
	Expect(Near(ClampFirstWaveDelay(0.1f), 0.5f), "clamp low");
	Expect(Near(ClampFirstWaveDelay(2.f), 1.f), "clamp high");
	Expect(Near(ClampFirstWaveDelay(0.75f), 0.75f), "clamp mid");
	Expect(IsP07AcceptanceRoom(RoomCombat01Id), "acceptance id");
	Expect(!IsP07AcceptanceRoom("Room_Combat_02"), "combat 02 stays out");
	Expect(!IsP07AcceptanceRoom("Room_Elite_01"), "elite stays out");
	Expect(!IsP07AcceptanceRoom("Room_Boss_01"), "boss stays out");
	Expect(ResolveEnemyKind(EnemyGruntEarlyId) == EEnemyKind::GruntEarly, "grunt early kind");
	Expect(ResolveEnemyKind("Enemy.Elite.Mid") == EEnemyKind::None, "no elite class");
	Expect(Near(FacingSignFromName("Left"), -1.f), "face left");
	Expect(Near(FacingSignFromName("Right"), 1.f), "face right");
	Expect(static_cast<int>(EOfferSource::NormalRoom) == 0, "offer source is the normal-room pool");

	Expect(MapNameNamesRoomCombat01("Room_Combat_01"), "short map name");
	Expect(MapNameNamesRoomCombat01("UEDPIE_0_Room_Combat_01"), "pie world name");
	Expect(MapNameNamesRoomCombat01("/Game/Maps/Rooms/Room_Combat_01"), "package path");
	Expect(MapNameNamesRoomCombat01("Room_Combat_01.umap"), "umap filename");
	Expect(MapNameNamesRoomCombat01("/Game/Maps/Rooms/UEDPIE_1_Room_Combat_01"), "pie package");
	Expect(MapNameNamesRoomCombat01("Room_Combat_01?listen"), "map plus options");
	Expect(!MapNameNamesRoomCombat01(""), "empty map name");
	Expect(!MapNameNamesRoomCombat01("Entry"), "entry is not the official room");
	Expect(!MapNameNamesRoomCombat01("/Engine/Maps/Entry"), "engine entry");
	Expect(!MapNameNamesRoomCombat01("Lab_CombatP0"), "lab map stays on the sandbox");
	Expect(!MapNameNamesRoomCombat01("Room_Combat_02"), "combat 02 is not combat 01");
	Expect(!MapNameNamesRoomCombat01("UEDPIE_0_Room_Combat_02"), "pie combat 02");
	Expect(!MapNameNamesRoomCombat01("Room_Shop_01"), "shop map");
	Expect(!MapNameNamesRoomCombat01("Room_Combat_010"), "longer token is not combat 01");

	Expect(ResolveRoomCombat01Startup(true, false, false) == ERoomCombat01Startup::OfficialPlay, "map name alone is official play");
	Expect(ResolveRoomCombat01Startup(true, false, true) == ERoomCombat01Startup::OfficialPlay, "official map beats the cvar");
	Expect(ResolveRoomCombat01Startup(true, true, true) == ERoomCombat01Startup::OfficialPlay, "map and director beat the cvar");
	Expect(ResolveRoomCombat01Startup(false, true, false) == ERoomCombat01Startup::OfficialPlay, "placed director alone is official play");
	Expect(ResolveRoomCombat01Startup(false, true, true) == ERoomCombat01Startup::OfficialPlay, "placed director beats the cvar");
	Expect(ResolveRoomCombat01Startup(false, false, true) == ERoomCombat01Startup::EmptyMapDrive, "cvar remains the empty-map drive");
	Expect(ResolveRoomCombat01Startup(false, false, false) == ERoomCombat01Startup::Default, "lab and entry stay on the sandbox path");
	Expect(Near(SideSunIntensityLux, 10.f), "side sun is lab 10 lux");
	Expect(Near(SideSunPitchDeg, -40.f) && Near(SideSunYawDeg, -90.f) && Near(SideSunRollDeg, 0.f), "side sun rotation is -40,-90,0");

	const std::string MetaText = ReadFile(Root + "/Content/Data/Rooms/Room_Combat_01_meta.csv");
	const std::string SpawnText = ReadFile(Root + "/Content/Data/Rooms/Room_Combat_01_spawns.csv");
	const std::string GeoText = ReadFile(Root + "/Content/Data/Rooms/Room_Combat_01_geometry.csv");
	const std::string Feel = ReadFile(Root + "/Source/PixelSideRL/Public/Combat/PixelSideCombatMath.h");
	const std::string Director = ReadFile(Root + "/Source/PixelSideRL/Private/Rooms/PixelRoomDirector.cpp");
	const std::string AffixCsv = ReadFile(Root + "/Content/Data/Affixes/DT_Affix_P05.csv");
	const std::string Mode = ReadFile(Root + "/Source/PixelSideRL/Private/Combat/PixelSideGameMode.cpp");
	const std::string EngineIni = ReadFile(Root + "/Config/DefaultEngine.ini");
	if (gFails != 0)
	{
		std::printf("%d failed\n", gFails);
		return 1;
	}

	const std::size_t InitAt = Mode.find("APixelSideGameMode::InitGame");
	const std::size_t BeginAt = Mode.find("APixelSideGameMode::BeginPlay");
	const std::size_t TickAt = Mode.find("APixelSideGameMode::Tick");
	Expect(InitAt != std::string::npos && BeginAt != std::string::npos && TickAt != std::string::npos && InitAt < BeginAt && BeginAt < TickAt, "gamemode startup functions");
	if (InitAt != std::string::npos && BeginAt != std::string::npos && TickAt != std::string::npos)
	{
		const std::string InitBody = Mode.substr(InitAt, BeginAt - InitAt);
		const std::string BeginBody = Mode.substr(BeginAt, TickAt - BeginAt);
		const std::size_t InitOfficial = InitBody.find("ERoomCombat01Startup::OfficialPlay");
		const std::size_t InitMove = InitBody.find("SetActorLocation");
		Expect(InitBody.find("MapSignalsOfficialRoomCombat01") != std::string::npos, "initgame reads the map name");
		Expect(InitBody.find("HasPlacedRoomDirector") != std::string::npos, "initgame sees a placed director");
		Expect(InitOfficial != std::string::npos && InitMove != std::string::npos && InitOfficial < InitMove, "official play leaves PlayerStart");
		Expect(InitBody.find("ResolveRoomCombat01PlayerStart") != std::string::npos, "empty-map drive still places the csv start");
		if (InitOfficial != std::string::npos)
		{
			const std::size_t InitSun = InitBody.find("EnsureSideSun", InitOfficial);
			const std::size_t InitOfficialReturn = InitBody.find("return;", InitOfficial);
			Expect(InitSun != std::string::npos && InitOfficialReturn != std::string::npos && InitSun < InitOfficialReturn, "initgame official play ensures the side sun");
			Expect(InitBody.substr(InitOfficial, InitOfficialReturn - InitOfficial).find("PixelSideRoom") != std::string::npos, "initgame room sun tag");
		}
		const std::size_t BeginOfficial = BeginBody.find("ERoomCombat01Startup::OfficialPlay");
		const std::size_t BeginDrive = BeginBody.find("ERoomCombat01Startup::EmptyMapDrive");
		const std::size_t BeginSpawnDirector = BeginBody.find("SpawnRoomCombat01()");
		const std::size_t BeginLab = BeginBody.find("SpawnLabSandbox()");
		Expect(BeginBody.find("HasPlacedRoomDirector") != std::string::npos, "beginplay sees a placed director");
		Expect(BeginOfficial != std::string::npos && BeginDrive != std::string::npos && BeginSpawnDirector != std::string::npos && BeginLab != std::string::npos, "beginplay branches");
		Expect(BeginOfficial < BeginDrive && BeginDrive < BeginSpawnDirector && BeginSpawnDirector < BeginLab, "official play before empty-map drive before lab");
		Expect(Contains(BeginBody, "Room_Shop_01 drive is ignored while Room_Combat_01"), "combat still wins over shop");
		Expect(Contains(Mode, "ShouldDriveRoomCombat01()"), "empty-map cvar path remains");
		if (BeginOfficial != std::string::npos)
		{
			const std::size_t BeginSun = BeginBody.find("EnsureSideSun", BeginOfficial);
			const std::size_t BeginOfficialReturn = BeginBody.find("return;", BeginOfficial);
			Expect(BeginSun != std::string::npos && BeginOfficialReturn != std::string::npos && BeginSun < BeginOfficialReturn, "beginplay official play ensures the side sun");
			Expect(BeginBody.substr(BeginOfficial, BeginOfficialReturn - BeginOfficial).find("PixelSideRoom") != std::string::npos, "beginplay room sun tag");
		}
	}

	const std::size_t SunAt = Mode.find("APixelSideGameMode::EnsureSideSun");
	const std::size_t SunEnd = Mode.find("APixelSideGameMode::SpawnLabSandbox");
	const std::size_t LabEnd = Mode.find("APixelSideGameMode::RecoverLabPitFall");
	Expect(SunAt != std::string::npos && SunEnd != std::string::npos && LabEnd != std::string::npos && SunAt < SunEnd && SunEnd < LabEnd, "side sun helper sits with lab");
	if (SunAt != std::string::npos && SunEnd != std::string::npos && LabEnd != std::string::npos)
	{
		const std::string SunBody = Mode.substr(SunAt, SunEnd - SunAt);
		const std::string LabBody = Mode.substr(SunEnd, LabEnd - SunEnd);
		Expect(Contains(SunBody, "SideSunIntensityLux"), "intensity is the shared lab value");
		Expect(Contains(SunBody, "SideSunPitchDeg"), "pitch is the shared lab value");
		Expect(Contains(SunBody, "SideSunYawDeg"), "yaw is the shared lab value");
		Expect(Contains(SunBody, "SideSunRollDeg"), "roll is the shared lab value");
		Expect(Contains(SunBody, "EComponentMobility::Movable"), "side sun is movable");
		Expect(Contains(SunBody, "SetAtmosphereSunLight(false)"), "side sun is not an atmosphere sun");
		Expect(Contains(SunBody, "SetActorRotation"), "actor rotation is forced");
		Expect(Contains(SunBody, "SetWorldRotation"), "light component rotation is forced");
		const std::size_t Mobility = SunBody.find("SetMobility");
		const std::size_t ActorRot = SunBody.find("SetActorRotation");
		Expect(Mobility != std::string::npos && ActorRot != std::string::npos && Mobility < ActorRot, "mobility is set before rotation");
		const std::size_t None = SunBody.find("Lights.Num() == 0");
		const std::size_t Spawn = SunBody.find("SpawnActor<ADirectionalLight>");
		const std::size_t Retarget = SunBody.find("for (AActor* Actor : Lights)");
		Expect(None != std::string::npos && Spawn != std::string::npos && Retarget != std::string::npos && None < Spawn && Spawn < Retarget, "spawn when absent, retarget when present");
		Expect(Contains(LabBody, "EnsureSideSun(World, TEXT(\"PixelSideLab\"))"), "lab uses the shared side sun");
		Expect(LabBody.find("SpawnActor<ADirectionalLight>") == std::string::npos, "lab does not fork a sun spawn");
	}
	Expect(Contains(Director, "APixelSideGameMode::EnsureSideSun"), "empty-map greybox uses the shared side sun");
	Expect(Director.find("SetAtmosphereSunLight") == std::string::npos, "director does not fork atmosphere sun");
	Expect(Director.find("FRotator(-40") == std::string::npos, "director does not fork sun rotation");
	Expect(Contains(EngineIni, "r.DefaultFeature.AutoExposure=0"), "auto exposure cvar stays off");
	Expect(Contains(EngineIni, "r.DefaultFeature.AutoExposure=False"), "renderer auto exposure stays off");
	Expect(!Contains(Mode, "r.DefaultFeature.AutoExposure"), "game mode does not toggle auto exposure");

	Expect(Contains(Feel, "JumpZVelocity = 400.f"), "jump untouched");
	Expect(Contains(Feel, "GravityScale = 2.55f"), "gravity untouched");
	Expect(Contains(Feel, "PeaDamage = 8.f"), "pea untouched");
	Expect(Contains(Feel, "EnemyGruntDamage = 6.f"), "grunt damage untouched");
	Expect(Contains(Feel, "EnemyGruntFireInterval = 0.8f"), "grunt interval untouched");
	Expect(Contains(Feel, "GruntDamage = 7.5f"), "turret untouched");
	Expect(Contains(Director, "RequestAffixChoice"), "director asks the subsystem");
	Expect(!Contains(Director, "RollChoices"), "director does not roll");
	Expect(!Contains(Director, "ConfirmAffixChoice"), "presenter confirms");
	Expect(Contains(Director, "APixelEnemyGrunt"), "spawns the grunt class");
	Expect(Contains(Director, "Enemy.Grunt.Early") || Contains(std::string(EnemyGruntEarlyId), "Enemy.Grunt.Early"), "early grunt id");
	int AffixRows = 0;
	for (const char* Id : {"RapidFire", "HollowPoint", "ThickSkin", "Magnet", "Piercer", "TwinShot", "CritOptics", "Scavenger"})
	{
		if (Contains(AffixCsv, Id))
		{
			++AffixRows;
		}
	}
	Expect(AffixRows == 8, "p0.6 eight still the pool file");

	FRoomMeta Meta;
	std::vector<FSpawnRow> Spawns;
	std::string Error;
	Expect(LoadRoomTables(MetaText, SpawnText, RoomCombat01Id, Meta, Spawns, Error), "load combat 01");
	if (!Error.empty())
	{
		std::printf("  %s\n", Error.c_str());
	}
	Expect(Meta.RoomId == RoomCombat01Id, "room id");
	Expect(Meta.Type == "Combat", "type");
	Expect(Meta.Tag == "Early", "tag");
	Expect(Meta.MaxAlive == 4, "max alive 4");
	Expect(Meta.ClearRule == ClearRuleWaves, "waves cleared");
	Expect(Meta.bAffixOnClear, "affix on clear");
	Expect(Meta.RequireClearExit == ExitRightId, "require right");
	Expect(Spawns.size() == 4, "four spawns");

	FRoomFlow Flow;
	Expect(Flow.Load(MetaText, SpawnText, RoomCombat01Id, Error), "flow load");
	Expect(Near(Flow.FirstWaveDelay(), 0.75f), "default delay");
	Flow.NotifyEntered();
	Flow.Tick(0.5f);
	Expect(Flow.ConsumeSpawnOrders().empty(), "silent through 0.5s");
	Expect(Flow.Phase() == EPhase::Arming, "still arming");
	Flow.Tick(0.25f);
	const std::vector<FSpawnOrder> Wave1 = Flow.ConsumeSpawnOrders();
	Expect(Wave1.size() == 2, "w1 count");
	const FSpawnOrder* G1 = FindOrder(Wave1, "SP_G1");
	const FSpawnOrder* G2 = FindOrder(Wave1, "SP_G2");
	Expect(G1 && G2, "w1 ids");
	Expect(FindOrder(Wave1, "SP_G3") == nullptr && FindOrder(Wave1, "SP_G4") == nullptr, "w2 waits");
	if (G1 && G2)
	{
		Expect(Near(G1->X, 420.f) && Near(G1->Y, 0.f), "g1 pos");
		Expect(Near(G2->X, 560.f) && Near(G2->Y, 0.f), "g2 pos");
		Expect(G1->EnemyId == EnemyGruntEarlyId && G2->EnemyId == EnemyGruntEarlyId, "w1 enemy");
		Expect(G1->Kind == EEnemyKind::GruntEarly && G2->Kind == EEnemyKind::GruntEarly, "w1 kind");
		Expect(G1->Facing == "Left" && Near(G1->FacingSign, -1.f), "g1 face");
		Expect(G2->Facing == "Left" && Near(G2->FacingSign, -1.f), "g2 face");
		Expect(G1->WaveGroup == 1 && G2->WaveGroup == 1, "w1 group");
	}
	Ack(Flow, Wave1);
	Expect(Flow.Alive() == 2, "two alive");
	Expect(Flow.PeakAlive() <= Flow.MaxAlive(), "peak under cap");
	Expect(Flow.PeakCommitted() <= Flow.MaxAlive(), "committed under cap");
	Expect(!Flow.IsExitOpen(ExitRightId), "right shut during w1");
	Expect(Flow.IsExitOpen(ExitLeftId), "left is the way back");
	Expect(Flow.NotifyExitUsed(ExitRightId) == EExitTouch::DeniedLocked, "right touch denied");
	Expect(Flow.NotifyExitUsed(ExitLeftId) == EExitTouch::Passed, "left during the fight");
	Expect(Flow.Phase() == EPhase::Fighting && Flow.Alive() == 2, "left touch keeps the wave");
	Expect(Flow.ConsumeSpawnOrders().empty(), "left touch does not spawn");
	Expect(!Flow.IsCleared(), "not clear yet");

	Flow.NotifyEnemyDied("SP_G1");
	Expect(Flow.ConsumeSpawnOrders().empty(), "one kill does not start w2");
	Expect(Flow.Alive() == 1, "one still up");
	Expect(Flow.Phase() == EPhase::Fighting, "still fighting");
	Expect(!Flow.WantsAffixOffer(), "no offer yet");

	Flow.NotifyEnemyDied("SP_G2");
	const std::vector<FSpawnOrder> Wave2 = Flow.ConsumeSpawnOrders();
	Expect(Wave2.size() == 2, "w2 count");
	const FSpawnOrder* G3 = FindOrder(Wave2, "SP_G3");
	const FSpawnOrder* G4 = FindOrder(Wave2, "SP_G4");
	Expect(G3 && G4, "w2 ids");
	if (G3 && G4)
	{
		Expect(Near(G3->X, 720.f) && Near(G3->Y, 0.f), "g3 pos");
		Expect(Near(G4->X, 900.f) && Near(G4->Y, 0.f), "g4 pos");
		Expect(G3->EnemyId == EnemyGruntEarlyId && G4->Kind == EEnemyKind::GruntEarly, "w2 grunt");
		Expect(G3->Facing == "Left" && G4->Facing == "Left", "w2 face");
		Expect(G3->WaveGroup == 2 && G4->WaveGroup == 2, "w2 group");
	}
	Ack(Flow, Wave2);
	Expect(Flow.Alive() == 2, "w2 alive");
	Expect(Flow.PeakAlive() <= 4, "never five");
	Expect(!Flow.WantsAffixOffer(), "w2 still up");
	Expect(!Flow.IsExitOpen(ExitRightId), "right shut during w2");

	Flow.NotifyEnemyDied("SP_G3");
	Expect(Flow.ConsumeSpawnOrders().empty(), "last grunt still holds the room");
	Flow.NotifyEnemyDied("SP_G4");
	Expect(Flow.ConsumeSpawnOrders().empty(), "no third wave");
	Expect(Flow.Alive() == 0, "room empty");
	Expect(Flow.IsAwaitingAffix(), "waiting on the card");
	Expect(Flow.WantsAffixOffer(), "wants the subsystem offer");
	Expect(Flow.OfferSource() == EOfferSource::NormalRoom, "normal room source");
	Expect(!Flow.IsExitOpen(ExitRightId), "right shut until the card");
	Expect(Flow.NotifyExitUsed(ExitRightId) == EExitTouch::DeniedLocked, "skipping the card keeps the door");

	Flow.MarkAffixRequested();
	Expect(!Flow.WantsAffixOffer(), "offer handed off");
	Flow.NotifyChoiceClosed(false);
	Expect(Flow.IsAwaitingAffix(), "still waiting");
	Expect(Flow.WantsAffixOffer(), "ask again");
	Expect(!Flow.IsCleared(), "a dismiss is not a clear");
	Expect(!Flow.IsExitOpen(ExitRightId), "right still shut");

	Flow.MarkAffixRequested();
	Flow.NotifyChoiceClosed(true);
	Expect(Flow.IsCleared(), "confirmed clear");
	Expect(Flow.IsExitOpen(ExitRightId), "right opens");
	Expect(Flow.NotifyExitUsed(ExitRightId) == EExitTouch::Passed, "right passage");
	Expect(!Flow.WantsAffixOffer(), "offer done");
	const int Issued = Flow.OrdersIssued();
	Expect(Flow.NotifyExitUsed(ExitLeftId) == EExitTouch::BacktrackKeptClear, "left keeps the clear");
	Expect(Flow.KeptClearAcrossBacktrack(), "backtrack flag");
	Expect(Flow.IsCleared() && Flow.IsExitOpen(ExitRightId), "clear survived the left door");
	Expect(Flow.Alive() == 0, "no respawn on the left door");
	Flow.Tick(5.f);
	Expect(Flow.ConsumeSpawnOrders().empty(), "no wave after the left door");
	Expect(Flow.OrdersIssued() == Issued, "order count frozen");
	Flow.NotifyEntered();
	Expect(Flow.RefusedResetOnReentry(), "re-entry does not rewind");
	Expect(Flow.IsCleared(), "still clear");
	Flow.Tick(1.f);
	Expect(Flow.ConsumeSpawnOrders().empty(), "re-entry does not respawn");

	std::vector<FGeometryRow> Geo;
	Expect(LoadGeometryCsv(GeoText, Geo, Error), "geometry");
	float StartX = -1.f;
	float StartY = -1.f;
	Expect(FindAnchor(Geo, "PlayerStart", StartX, StartY), "player start row");
	Expect(Near(StartX, 40.f) && Near(StartY, 0.f), "player start coords");
	bool bSawRight = false;
	bool bSawLeft = false;
	for (const FGeometryRow& Row : Geo)
	{
		if (Row.Kind != "Exit")
		{
			continue;
		}
		const std::string Tag = ExitTagOf(Row);
		if (Tag == ExitRightId)
		{
			bSawRight = true;
			Expect(Row.Rule == "RequireClear", "right rule");
			Expect(Near(Row.X0, 1200.f) && Near(Row.X1, 0.f), "right socket");
		}
		if (Tag == ExitLeftId)
		{
			bSawLeft = true;
			Expect(Row.Rule.empty(), "left rule empty");
			Expect(Near(Row.X0, -40.f) && Near(Row.X1, 0.f), "left socket");
		}
	}
	Expect(bSawRight && bSawLeft, "both exits");

	const char* CapMeta =
		"room_id,type,tag,design_width,design_height,max_alive,clear_rule,affix_on_clear,require_clear_exit,notes\n"
		"Room_Combat_01,Combat,Early,1280,360,2,WavesCleared,true,Exit.Right,cap\n";
	const char* CapSpawns =
		"spawn_id,spawn_tag,wave_group,x,y,facing,optional,enemy_id_placeholder,notes\n"
		"A,Ground,1,1,0,Left,false,Enemy.Grunt.Early,a\n"
		"B,Ground,1,2,0,Left,false,Enemy.Grunt.Early,b\n"
		"C,Ground,1,3,0,Right,false,Enemy.Grunt.Early,c\n"
		"D,Ground,1,4,0,Left,true,Enemy.Grunt.Early,optional\n";
	FRoomFlow Cap;
	Expect(Cap.Load(CapMeta, CapSpawns, RoomCombat01Id, Error), "cap load");
	auto First = ArmAndTake(Cap);
	Expect(First.size() == 2, "cap emits two");
	Expect(FindOrder(First, "D") == nullptr, "optional stays out");
	Ack(Cap, First);
	Expect(Cap.Alive() == 2, "cap full");
	Expect(Cap.ConsumeSpawnOrders().empty(), "third waits");
	Cap.NotifyEnemyDied(First[0].SpawnId);
	auto Rest = Cap.ConsumeSpawnOrders();
	Expect(Rest.size() == 1 && Rest[0].SpawnId == "C", "third fills a slot");
	Expect(Near(Rest[0].FacingSign, 1.f), "right facing");
	Ack(Cap, Rest);
	Expect(Cap.Alive() == 2, "still at the cap");
	Expect(Cap.PeakAlive() <= 2 && Cap.PeakCommitted() <= 2, "cap peak");
	Cap.NotifyEnemyDied(First[1].SpawnId);
	Cap.NotifyEnemyDied("C");
	Expect(Cap.WantsAffixOffer(), "cap room still offers");

	const char* OtherMeta =
		"room_id,type,tag,design_width,design_height,max_alive,clear_rule,affix_on_clear,require_clear_exit,notes\n"
		"Room_Combat_02,Combat,Early,1600,360,5,WavesCleared,true,Exit.Right,no\n";
	FRoomMeta Other;
	std::vector<FSpawnRow> OtherSpawns;
	Expect(!LoadRoomTables(OtherMeta, CapSpawns, RoomCombat01Id, Other, OtherSpawns, Error), "reject other room");
	Expect(Error.find("acceptance") != std::string::npos, "rejection reason");

	const char* StallMeta =
		"room_id,type,tag,design_width,design_height,max_alive,clear_rule,affix_on_clear,require_clear_exit,notes\n"
		"Room_Combat_01,Combat,Early,1280,360,4,BossDead,true,Exit.Right,no\n";
	const char* OneSpawn =
		"spawn_id,spawn_tag,wave_group,x,y,facing,optional,enemy_id_placeholder,notes\n"
		"A,Ground,1,1,0,Left,false,Enemy.Grunt.Early,a\n";
	FRoomFlow Stall;
	Expect(Stall.Load(StallMeta, OneSpawn, RoomCombat01Id, Error), "stall load");
	auto Only = ArmAndTake(Stall);
	Ack(Stall, Only);
	Stall.NotifyEnemyDied("A");
	Expect(Stall.IsStalled(), "unknown clear rule stalls");
	Expect(!Stall.WantsAffixOffer() && !Stall.IsExitOpen(ExitRightId), "stall keeps the door");

	const char* NoAffixMeta =
		"room_id,type,tag,design_width,design_height,max_alive,clear_rule,affix_on_clear,require_clear_exit,notes\n"
		"Room_Combat_01,Combat,Early,1280,360,4,WavesCleared,false,Exit.Right,no\n";
	FRoomFlow Bare;
	Expect(Bare.Load(NoAffixMeta, OneSpawn, RoomCombat01Id, Error), "no affix load");
	auto BareWave = ArmAndTake(Bare);
	Ack(Bare, BareWave);
	Bare.NotifyEnemyDied("A");
	Expect(Bare.IsCleared() && Bare.IsExitOpen(ExitRightId), "clear without an offer still opens");
	Expect(!Bare.WantsAffixOffer(), "flag off means no offer");

	FRoomFlow Clamped;
	Clamped.SetFirstWaveDelay(0.1f);
	Expect(Near(Clamped.FirstWaveDelay(), 0.5f), "runtime clamp");
	Expect(Clamped.Load(MetaText, SpawnText, RoomCombat01Id, Error), "reload keeps delay");
	Expect(Near(Clamped.FirstWaveDelay(), 0.5f), "load keeps the clamped delay");

	FRoomFlow Early;
	Expect(Early.Load(MetaText, SpawnText, RoomCombat01Id, Error), "early load");
	Early.NotifyEntered();
	Early.Tick(0.4f);
	Early.NotifyEntered();
	Early.Tick(0.4f);
	Expect(Early.ConsumeSpawnOrders().size() == 2, "a second enter does not restart the arm");

	FRoomFlow FailSpawn;
	Expect(FailSpawn.Load(MetaText, SpawnText, RoomCombat01Id, Error), "failspawn load");
	auto Open = ArmAndTake(FailSpawn);
	Expect(Open.size() == 2, "failspawn wave");
	FailSpawn.NotifySpawnFailed(Open[0].SpawnId);
	FailSpawn.NotifySpawned(Open[1].SpawnId);
	FailSpawn.NotifyEnemyDied(Open[1].SpawnId);
	Expect(FailSpawn.ConsumeSpawnOrders().empty(), "a failed spawn blocks the next wave");
	Expect(!FailSpawn.WantsAffixOffer(), "a failed spawn blocks the offer");

	if (gFails != 0)
	{
		std::printf("%d failed\n", gFails);
		return 1;
	}
	std::printf("room combat 01 ok\n");
	return 0;
}
