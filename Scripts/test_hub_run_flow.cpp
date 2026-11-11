// Host-side P0.8 Hub start checks. No Unreal headers.
// g++ -std=c++17 -Wall -Wextra -Werror -I Source/PixelSideRL/Public -o /tmp/test_hub_run_flow Scripts/test_hub_run_flow.cpp
// /tmp/test_hub_run_flow .
#include "Rooms/PixelSideRoomFlow.h"

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

bool NearlySideSunUnchanged()
{
	const auto Near = [](float A, float B) -> bool
	{
		const float D = (A > B) ? (A - B) : (B - A);
		return D < 0.0001f;
	};
	return Near(PixelSideRoom::SideSunIntensityLux, 10.f)
		&& Near(PixelSideRoom::SideSunPitchDeg, -40.f)
		&& Near(PixelSideRoom::SideSunYawDeg, -90.f)
		&& Near(PixelSideRoom::SideSunRollDeg, 0.f);
}
}

int main(int argc, char** argv)
{
	const std::string Root = (argc > 1) ? argv[1] : ".";
	using namespace PixelSideRoom;

	Expect(std::string(HubMapPackage) == "/Game/Maps/Hub", "hub package path");
	Expect(std::string(RoomCombat01MapPackage) == "/Game/Maps/Rooms/Room_Combat_01", "combat package path");
	Expect(MapNameNamesHub("Hub"), "short hub name");
	Expect(MapNameNamesHub("UEDPIE_0_Hub"), "pie hub name");
	Expect(MapNameNamesHub("/Game/Maps/Hub"), "hub package");
	Expect(MapNameNamesHub("Hub.umap"), "hub filename");
	Expect(MapNameNamesHub("Hub?listen"), "hub plus options");
	Expect(MapNameNamesHub("/Game/Maps/UEDPIE_0_Hub"), "pie hub package");
	Expect(!MapNameNamesHub(""), "empty is not hub");
	Expect(!MapNameNamesHub("Entry"), "entry is not hub");
	Expect(!MapNameNamesHub("/Engine/Maps/Entry"), "engine entry is not hub");
	Expect(!MapNameNamesHub("HubWorld"), "longer token is not hub");
	Expect(!MapNameNamesHub("MyHub"), "suffix is not hub");
	Expect(!MapNameNamesHub("Room_Combat_01"), "combat room is not hub");
	Expect(!MapNameNamesHub("UEDPIE_0_Room_Combat_01"), "pie combat is not hub");
	Expect(!MapNameNamesHub("/Game/Maps/Rooms/Room_Combat_01"), "combat package is not hub");
	Expect(!MapNameNamesHub("Room_Combat_01.umap"), "combat file is not hub");
	Expect(!MapNameNamesRoomCombat01("/Game/Maps/Hub"), "hub package is not combat 01");
	Expect(!MapNameNamesRoomCombat01("UEDPIE_0_Hub"), "pie hub is not combat 01");
	Expect(MapNameNamesRoomCombat01("/Game/Maps/Rooms/Room_Combat_01"), "combat package still matches");
	Expect(NearlySideSunUnchanged(), "side sun constants stay the lab values");

	const std::string Mode = ReadFile(Root + "/Source/PixelSideRL/Private/Combat/PixelSideGameMode.cpp");
	const std::string Flow = ReadFile(Root + "/Source/PixelSideRL/Private/Flow/PixelHubRunFlowSubsystem.cpp");
	const std::string HubWidget = ReadFile(Root + "/Source/PixelSideRL/Private/UI/PixelHubWidget.cpp");
	const std::string Summary = ReadFile(Root + "/Source/PixelSideRL/Private/UI/PixelRunSummaryWidget.cpp");
	const std::string EngineIni = ReadFile(Root + "/Config/DefaultEngine.ini");
	const std::string Feel = ReadFile(Root + "/Source/PixelSideRL/Public/Combat/PixelSideCombatMath.h");
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
		const std::size_t InitHub = InitBody.find("IsHubMap");
		const std::size_t InitMove = InitBody.find("SetActorLocation");
		Expect(InitHub != std::string::npos && InitOfficial != std::string::npos && InitHub < InitOfficial, "hub init does not steal the official block");
		Expect(InitOfficial != std::string::npos && InitMove != std::string::npos && InitOfficial < InitMove, "official play still leaves PlayerStart");
		if (InitOfficial != std::string::npos)
		{
			const std::size_t InitSun = InitBody.find("EnsureSideSun", InitOfficial);
			const std::size_t InitOfficialReturn = InitBody.find("return;", InitOfficial);
			Expect(InitSun != std::string::npos && InitOfficialReturn != std::string::npos && InitSun < InitOfficialReturn, "initgame official play still ensures the side sun");
			Expect(InitBody.substr(InitOfficial, InitOfficialReturn - InitOfficial).find("PixelSideRoom") != std::string::npos, "initgame room sun tag");
		}

		const std::size_t BeginHub = BeginBody.find("HandleHubMapBegan");
		const std::size_t BeginOfficial = BeginBody.find("ERoomCombat01Startup::OfficialPlay");
		const std::size_t BeginDrive = BeginBody.find("ERoomCombat01Startup::EmptyMapDrive");
		const std::size_t BeginSpawnDirector = BeginBody.find("SpawnRoomCombat01()");
		const std::size_t BeginLab = BeginBody.find("SpawnLabSandbox()");
		Expect(BeginHub != std::string::npos && BeginOfficial != std::string::npos && BeginHub < BeginOfficial, "hub beginplay returns before official combat");
		Expect(BeginOfficial < BeginDrive && BeginDrive < BeginSpawnDirector && BeginSpawnDirector < BeginLab, "official play before empty-map drive before lab");
		if (BeginOfficial != std::string::npos)
		{
			const std::size_t BeginSun = BeginBody.find("EnsureSideSun", BeginOfficial);
			const std::size_t BeginFlow = BeginBody.find("HandleCombatMapBegan", BeginOfficial);
			const std::size_t BeginOfficialReturn = BeginBody.find("return;", BeginOfficial);
			Expect(BeginSun != std::string::npos && BeginFlow != std::string::npos && BeginOfficialReturn != std::string::npos, "beginplay official block");
			Expect(BeginSun < BeginFlow && BeginFlow < BeginOfficialReturn, "combat flow is after the side sun and before the official return");
			Expect(BeginBody.substr(BeginOfficial, BeginOfficialReturn - BeginOfficial).find("PixelSideRoom") != std::string::npos, "beginplay room sun tag");
		}
	}

	Expect(Contains(Mode, "EnsureSideSun(World, TEXT(\"PixelSideRoom\"))"), "official play still calls the shared sun");
	Expect(Contains(Mode, "EnsureSideSun(World, TEXT(\"PixelSideLab\"))"), "lab still uses the shared sun");
	Expect(!Contains(Flow, "EnsureSideSun"), "hub flow does not add a sun");
	Expect(!Contains(Flow, "AutoExposure"), "hub flow does not touch auto exposure");
	Expect(!Contains(Mode, "r.DefaultFeature.AutoExposure"), "game mode does not toggle auto exposure");
	Expect(Contains(EngineIni, "r.DefaultFeature.AutoExposure=0"), "auto exposure cvar stays off");
	Expect(Contains(EngineIni, "r.DefaultFeature.AutoExposure=False"), "renderer auto exposure stays off");
	Expect(Contains(EngineIni, "GameDefaultMap=/Engine/Maps/Entry"), "GameDefaultMap stays Entry");

	Expect(Contains(Flow, "BeginRunFromMeta"), "flow calls BeginRunFromMeta");
	Expect(Contains(Flow, "RoomCombat01MapPackage"), "flow travels with the combat package constant");
	Expect(Contains(Flow, "HubMapPackage"), "flow returns with the hub package constant");
	Expect(Contains(Flow, "OpenLevel"), "flow travels");
	Expect(Contains(Flow, "CoreGained = 0"), "summary core stays a placeholder");
	Expect(!Contains(Flow, "AddCore("), "flow does not grant core");
	Expect(!Contains(Flow, "PreviewCoreGain"), "flow does not run the core formula");
	Expect(!Contains(Flow, "Save("), "flow does not save");
	Expect(Contains(Flow, "PixelSide.Run.End"), "explicit end is a console command");
	Expect(Contains(Flow, "OnHealthChanged"), "death uses the existing health delegate");
	Expect(Contains(Flow, "Tools/BakeHub.py"), "missing hub map names the bake script");
	Expect(!Contains(Flow, "JumpZVelocity"), "flow does not touch the feel table");
	Expect(!Contains(Flow, "RequestAffixChoice"), "flow does not roll an affix choice");

	Expect(Contains(HubWidget, "OnStartRun.Broadcast()"), "hub widget still broadcasts");
	Expect(!Contains(HubWidget, "->BeginRunFromMeta"), "hub widget does not call BeginRunFromMeta");
	Expect(!Contains(HubWidget, "::BeginRunFromMeta"), "hub widget does not call BeginRunFromMeta");
	Expect(!Contains(HubWidget, "OpenLevel"), "hub widget does not travel");
	Expect(Contains(Summary, "OnContinueToHub.Broadcast()"), "summary still broadcasts");
	Expect(!Contains(Summary, "OpenLevel"), "summary widget does not travel");
	Expect(!Contains(Summary, "BeginRunFromMeta"), "summary widget does not start a run");

	Expect(Contains(Feel, "JumpZVelocity = 400.f"), "jump untouched");
	Expect(Contains(Feel, "GravityScale = 2.55f"), "gravity untouched");
	Expect(Contains(Feel, "PeaDamage = 8.f"), "pea untouched");

	const std::string Bake = ReadFile(Root + "/Tools/BakeHub.py");
	Expect(Contains(Bake, "ASSET_PATH = \"/Game/Maps/Hub\""), "bake writes the hub package");
	Expect(Contains(Bake, "spawn_actor(unreal.PlayerStart"), "bake places a player start");
	Expect(Contains(Bake, "No floor, no director, no light"), "bake does not lay out a lab");
	Expect(Contains(Bake, "\"PixelRoomDirector\""), "bake rejects a director");
	Expect(Contains(Bake, "\"DirectionalLight\""), "bake rejects a sun");
	Expect(Contains(Bake, "\"StaticMeshActor\""), "bake rejects lab meshes");
	Expect(!Contains(Bake, "spawn_actor(unreal.DirectionalLight"), "bake does not spawn a sun");
	Expect(!Contains(Bake, "spawn_actor(unreal.StaticMeshActor"), "bake does not spawn geometry");

	if (gFails != 0)
	{
		std::printf("%d failed\n", gFails);
		return 1;
	}
	std::printf("hub run flow ok\n");
	return 0;
}
