// P0.7 room flow for Room_Combat_01. No Unreal types — host tests compile this
// with the game module. The director reads the corridor CSV and asks
// UAffixSubsystem::RequestAffixChoice(NormalRoom). This file does not roll a pool.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace PixelSideRoom
{

inline constexpr float FirstWaveDelayMinSec = 0.5f;
inline constexpr float FirstWaveDelayMaxSec = 1.0f;
inline constexpr float FirstWaveDelayDefaultSec = 0.75f;
inline constexpr char RoomCombat01Id[] = "Room_Combat_01";
inline constexpr char RoomShop01Id[] = "Room_Shop_01";
inline constexpr char RoomTypeShop[] = "Shop";
inline constexpr int ShopShelfLayer = 1;
inline constexpr float RoomShop01PlayerStartX = 48.f;
inline constexpr float RoomShop01PlayerStartY = 0.f;
inline constexpr char EnemyGruntEarlyId[] = "Enemy.Grunt.Early";
inline constexpr char ExitLeftId[] = "Exit.Left";
inline constexpr char ExitRightId[] = "Exit.Right";
inline constexpr char ClearRuleWaves[] = "WavesCleared";
inline constexpr float RoomCombat01PlayerStartX = 40.f;
inline constexpr float RoomCombat01PlayerStartY = 0.f;

static_assert(FirstWaveDelayDefaultSec >= FirstWaveDelayMinSec, "default first-wave delay");
static_assert(FirstWaveDelayDefaultSec <= FirstWaveDelayMaxSec, "default first-wave delay");

enum class EPhase : uint8_t
{
	Idle,
	Arming,
	Fighting,
	AwaitingAffix,
	Cleared
};

enum class EEnemyKind : uint8_t
{
	None,
	GruntEarly
};

/** Matches UAffixSubsystem EAffixOfferSource::NormalRoom (the P0.6 eight). */
enum class EOfferSource : uint8_t
{
	NormalRoom
};

enum class EExitTouch : uint8_t
{
	Ignored,
	DeniedLocked,
	Passed,
	BacktrackKeptClear
};

struct FSpawnRow
{
	std::string SpawnId;
	std::string SpawnTag;
	int WaveGroup = 0;
	float X = 0.f;
	float Y = 0.f;
	std::string Facing;
	bool bOptional = false;
	std::string EnemyId;
	std::string Notes;
};

struct FRoomMeta
{
	std::string RoomId;
	std::string Type;
	std::string Tag;
	float DesignWidth = 0.f;
	float DesignHeight = 0.f;
	int MaxAlive = 0;
	std::string ClearRule;
	bool bAffixOnClear = false;
	std::string RequireClearExit;
	std::string Notes;
};

struct FGeometryRow
{
	std::string Id;
	std::string Kind;
	float X0 = 0.f;
	float X1 = 0.f;
	float XCenter = 0.f;
	float Width = 0.f;
	float TopY = 0.f;
	float Height = 0.f;
	std::string Facing;
	std::string Rule;
	std::string Notes;
};

struct FSpawnOrder
{
	std::string SpawnId;
	std::string EnemyId;
	EEnemyKind Kind = EEnemyKind::None;
	int WaveGroup = 0;
	float X = 0.f;
	float Y = 0.f;
	float FacingSign = -1.f;
	std::string Facing;
};

inline bool IsP07AcceptanceRoom(std::string_view RoomId)
{
	return RoomId == RoomCombat01Id;
}

/**
 * True when a map name, PIE world name, or package path is official Room_Combat_01.
 * `UEDPIE_0_Room_Combat_01` and `/Game/Maps/Rooms/Room_Combat_01` match.
 * A longer token such as Room_Combat_02 or Room_Combat_010 does not.
 */
inline bool MapNameNamesRoomCombat01(std::string_view MapName)
{
	const std::string_view Id(RoomCombat01Id);
	std::size_t From = 0;
	while (From < MapName.size())
	{
		const std::size_t At = MapName.find(Id, From);
		if (At == std::string_view::npos)
		{
			return false;
		}
		const std::size_t After = At + Id.size();
		if (After >= MapName.size())
		{
			return true;
		}
		const unsigned char Next = static_cast<unsigned char>(MapName[After]);
		const bool bContinuesToken = (Next >= '0' && Next <= '9') || (Next >= 'A' && Next <= 'Z') || (Next >= 'a' && Next <= 'z');
		if (!bContinuesToken)
		{
			return true;
		}
		From = After;
	}
	return false;
}

/**
 * Side sun Lab already uses, and official Room_Combat_01 Play applies again.
 * FRotator order is pitch, yaw, roll. The camera sits on +Y looking toward -Y, so yaw -90
 * lights the faces in view and the floor. 10 lux is the UE5 directional default.
 * AutoExposure stays off. These values are the Lit look; do not fork a second set.
 */
inline constexpr float SideSunIntensityLux = 10.f;
inline constexpr float SideSunPitchDeg = -40.f;
inline constexpr float SideSunYawDeg = -90.f;
inline constexpr float SideSunRollDeg = 0.f;

/**
 * How Play starts Room_Combat_01.
 * OfficialPlay: the loaded map names Room_Combat_01, or a director is already placed.
 * Leave the in-map PlayerStart, skip the Lab sandbox, and do not spawn another director.
 * Still apply the side sun above. The placed directional often did not keep intensity or rotation.
 * EmptyMapDrive: PixelSide.DriveRoomCombat01 on a map that is not official and has no director.
 * Default: Lab sandbox when that map is actually a Lab / empty entry.
 * OfficialPlay wins when the cvar is also set.
 */
enum class ERoomCombat01Startup : uint8_t
{
	Default,
	EmptyMapDrive,
	OfficialPlay
};

inline ERoomCombat01Startup ResolveRoomCombat01Startup(bool bMapNamesOfficialRoom, bool bPlacedDirector, bool bEmptyMapDrive)
{
	if (bMapNamesOfficialRoom || bPlacedDirector)
	{
		return ERoomCombat01Startup::OfficialPlay;
	}
	if (bEmptyMapDrive)
	{
		return ERoomCombat01Startup::EmptyMapDrive;
	}
	return ERoomCombat01Startup::Default;
}

/** Room id or meta type. Combat_01 is neither. */
inline bool IsShopRoom(std::string_view RoomId, std::string_view Type)
{
	return RoomId == RoomShop01Id || Type == RoomTypeShop;
}

/**
 * What entering a shop room is allowed to do.
 * OpenShop on the run wallet, price-curve layer 1. Doors stay unlocked.
 * No affix. No forced buy. A non-shop room leaves bOpenRunShop false.
 */
struct FShopEntry
{
	bool bOpenRunShop = false;
	int ShelfLayer = ShopShelfLayer;
	bool bLockDoors = false;
	bool bAffixOnClear = false;
	bool bForcePurchase = false;
};

inline FShopEntry MakeShopEntry(const FRoomMeta& Meta)
{
	FShopEntry Entry;
	if (!IsShopRoom(Meta.RoomId, Meta.Type))
	{
		return Entry;
	}
	Entry.bOpenRunShop = true;
	Entry.ShelfLayer = ShopShelfLayer;
	Entry.bLockDoors = false;
	Entry.bAffixOnClear = false;
	Entry.bForcePurchase = false;
	return Entry;
}

/** Platform/cover row: width>0 is center+width; otherwise x0–x1. */
inline void ResolveHorizontalSpan(const FGeometryRow& Row, float& OutX0, float& OutX1)
{
	if (Row.Width > 0.f)
	{
		OutX0 = Row.XCenter - Row.Width * 0.5f;
		OutX1 = Row.XCenter + Row.Width * 0.5f;
		return;
	}
	OutX0 = Row.X0;
	OutX1 = Row.X1;
}

inline EEnemyKind ResolveEnemyKind(std::string_view EnemyId)
{
	if (EnemyId == EnemyGruntEarlyId)
	{
		return EEnemyKind::GruntEarly;
	}
	return EEnemyKind::None;
}

inline float ClampFirstWaveDelay(float Seconds)
{
	if (Seconds < FirstWaveDelayMinSec)
	{
		return FirstWaveDelayMinSec;
	}
	if (Seconds > FirstWaveDelayMaxSec)
	{
		return FirstWaveDelayMaxSec;
	}
	return Seconds;
}

inline float FacingSignFromName(std::string_view Facing)
{
	std::string Lower;
	Lower.reserve(Facing.size());
	for (char C : Facing)
	{
		if (C >= 'A' && C <= 'Z')
		{
			Lower.push_back(static_cast<char>(C - 'A' + 'a'));
		}
		else
		{
			Lower.push_back(C);
		}
	}
	if (Lower == "right")
	{
		return 1.f;
	}
	return -1.f;
}

inline std::string ExitTagOf(const FGeometryRow& Row)
{
	constexpr std::string_view Prefix("Exit.");
	if (Row.Facing.size() >= Prefix.size() && std::string_view(Row.Facing).substr(0, Prefix.size()) == Prefix)
	{
		return Row.Facing;
	}
	return {};
}

/** Meta row only. Allows max_alive 0 and an empty require_clear_exit (shop / event). */
bool LoadRoomMetaCsv(std::string_view MetaCsv, FRoomMeta& Meta, std::string& Error);

bool LoadRoomTables(
	std::string_view MetaCsv,
	std::string_view SpawnCsv,
	std::string_view ExpectedRoomId,
	FRoomMeta& Meta,
	std::vector<FSpawnRow>& Spawns,
	std::string& Error);

bool LoadGeometryCsv(std::string_view Text, std::vector<FGeometryRow>& Rows, std::string& Error);

bool FindAnchor(const std::vector<FGeometryRow>& Rows, std::string_view Id, float& X, float& Y);

/**
 * Wave clock for one acceptance room.
 * Enter → arm 0.5–1.0s → W1 → each later wave after the previous wave is dead.
 * Alive count stays ≤ max_alive. WavesCleared + affix_on_clear asks for a NormalRoom
 * offer; the require-clear exit opens only after that offer is confirmed.
 * Touching Exit.Left after a clear does not rewind the clear or queue spawns.
 */
class FRoomFlow
{
public:
	FRoomFlow();

	bool Load(std::string_view MetaCsv, std::string_view SpawnCsv, std::string_view ExpectedRoomId, std::string& Error);
	void SetFirstWaveDelay(float Seconds);
	float FirstWaveDelay() const { return Delay; }

	void NotifyEntered();
	void Tick(float DeltaSeconds);
	void NotifySpawned(std::string_view SpawnId);
	void NotifySpawnFailed(std::string_view SpawnId);
	void NotifyEnemyDied(std::string_view SpawnId);
	EExitTouch NotifyExitUsed(std::string_view ExitId);
	void NotifyChoiceClosed(bool bConfirmed);
	void MarkAffixRequested();

	std::vector<FSpawnOrder> ConsumeSpawnOrders();

	bool WantsAffixOffer() const;
	EOfferSource OfferSource() const { return EOfferSource::NormalRoom; }
	bool IsExitOpen(std::string_view ExitId) const;
	bool IsCleared() const { return PhaseKind == EPhase::Cleared; }
	bool IsAwaitingAffix() const { return PhaseKind == EPhase::AwaitingAffix; }
	EPhase Phase() const { return PhaseKind; }
	int Alive() const { return Living; }
	int MaxAlive() const { return MetaRow.MaxAlive; }
	int PeakAlive() const { return PeakAliveCount; }
	int PeakCommitted() const { return PeakCommittedCount; }
	int CurrentWaveGroup() const;
	int EntryCount() const { return Entries; }
	bool KeptClearAcrossBacktrack() const { return bBacktrackKeptClear; }
	bool RefusedResetOnReentry() const { return bRefusedReset; }
	int OrdersIssued() const { return OrdersIssuedCount; }
	bool IsStalled() const { return bStalled; }
	const std::string& Note() const { return NoteText; }
	const FRoomMeta& Meta() const { return MetaRow; }
	const std::vector<FSpawnRow>& Spawns() const { return SpawnRows; }

private:
	enum class ESlotState : uint8_t
	{
		Pending,
		Queued,
		Alive,
		Dead,
		Failed
	};

	struct FSlot
	{
		FSpawnRow Row;
		ESlotState State = ESlotState::Pending;
	};

	void ResetRuntime();
	void BumpPeak();
	void StartCurrent();
	void FillCap();
	bool CurrentWaveFullyDead() const;
	void FinishWaves();
	void MarkCleared();
	FSlot* FindSlot(std::string_view SpawnId);
	FSpawnOrder MakeOrder(const FSpawnRow& Row) const;

	FRoomMeta MetaRow;
	std::vector<FSpawnRow> SpawnRows;
	std::vector<FSlot> Slots;
	std::vector<int> WaveGroups;
	std::vector<FSpawnOrder> PendingOrders;
	EPhase PhaseKind = EPhase::Idle;
	float Delay = FirstWaveDelayDefaultSec;
	float ArmRemaining = 0.f;
	int Living = 0;
	int Reserved = 0;
	int PeakAliveCount = 0;
	int PeakCommittedCount = 0;
	int WaveCursor = 0;
	int Entries = 0;
	int OrdersIssuedCount = 0;
	bool bWantsOffer = false;
	bool bClearedSticky = false;
	bool bBacktrackKeptClear = false;
	bool bRefusedReset = false;
	bool bStalled = false;
	std::string NoteText;
};

}
