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
