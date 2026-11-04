// Copyright PixelSideRL. P0.7 Room_Combat_01 wave clock. No Unreal headers.
#include "Rooms/PixelSideRoomFlow.h"

#include <cstdlib>
#include <utility>

namespace PixelSideRoom
{
namespace
{

std::string Trim(std::string_view Text)
{
	size_t Begin = 0;
	while (Begin < Text.size() && (Text[Begin] == ' ' || Text[Begin] == '\t' || Text[Begin] == '\r'))
	{
		++Begin;
	}
	size_t End = Text.size();
	while (End > Begin && (Text[End - 1] == ' ' || Text[End - 1] == '\t' || Text[End - 1] == '\r'))
	{
		--End;
	}
	return std::string(Text.substr(Begin, End - Begin));
}

bool ParseCsv(std::string_view Text, std::vector<std::vector<std::string>>& Rows, std::string& Error)
{
	if (Text.size() >= 3 && static_cast<unsigned char>(Text[0]) == 0xEF && static_cast<unsigned char>(Text[1]) == 0xBB && static_cast<unsigned char>(Text[2]) == 0xBF)
	{
		Text.remove_prefix(3);
	}

	std::string Line;
	bool bQuote = false;
	auto Flush = [&](bool bEof) -> bool {
		if (!bEof && Line.empty() && !bQuote)
		{
			return true;
		}
		if (bQuote)
		{
			Error = "unterminated csv quote";
			return false;
		}
		const std::string Trimmed = Trim(Line);
		Line.clear();
		if (Trimmed.empty())
		{
			return true;
		}

		std::vector<std::string> Fields;
		std::string Cur;
		bool bInQuote = false;
		for (size_t I = 0; I < Trimmed.size(); ++I)
		{
			const char C = Trimmed[I];
			if (bInQuote)
			{
				if (C == '"')
				{
					if (I + 1 < Trimmed.size() && Trimmed[I + 1] == '"')
					{
						Cur.push_back('"');
						++I;
					}
					else
					{
						bInQuote = false;
					}
				}
				else
				{
					Cur.push_back(C);
				}
			}
			else if (C == '"')
			{
				bInQuote = true;
			}
			else if (C == ',')
			{
				Fields.push_back(Trim(Cur));
				Cur.clear();
			}
			else
			{
				Cur.push_back(C);
			}
		}
		if (bInQuote)
		{
			Error = "unterminated csv quote";
			return false;
		}
		Fields.push_back(Trim(Cur));
		Rows.push_back(std::move(Fields));
		return true;
	};

	for (char C : Text)
	{
		if (C == '\n' && !bQuote)
		{
			if (!Flush(false))
			{
				return false;
			}
			continue;
		}
		if (C == '"')
		{
			bQuote = !bQuote;
		}
		Line.push_back(C);
	}
	return Flush(true);
}

int FindCol(const std::vector<std::string>& Header, const char* Name, std::string& Error)
{
	for (size_t I = 0; I < Header.size(); ++I)
	{
		if (Header[I] == Name)
		{
			return static_cast<int>(I);
		}
	}
	Error = std::string("missing column ") + Name;
	return -1;
}

bool Field(const std::vector<std::string>& Row, int Col, std::string& Out, std::string& Error, const char* What)
{
	if (Col < 0 || Col >= static_cast<int>(Row.size()))
	{
		Error = std::string("short row for ") + What;
		return false;
	}
	Out = Row[static_cast<size_t>(Col)];
	return true;
}

bool ParseFloat(const std::string& Text, float& Out)
{
	if (Text.empty())
	{
		return false;
	}
	char* End = nullptr;
	const float Value = std::strtof(Text.c_str(), &End);
	if (!End || End == Text.c_str() || *End != '\0')
	{
		return false;
	}
	Out = Value;
	return true;
}

bool ParseInt(const std::string& Text, int& Out)
{
	if (Text.empty())
	{
		return false;
	}
	char* End = nullptr;
	const long Value = std::strtol(Text.c_str(), &End, 10);
	if (!End || End == Text.c_str() || *End != '\0')
	{
		return false;
	}
	Out = static_cast<int>(Value);
	return true;
}

bool ParseBool(const std::string& Text, bool& Out)
{
	std::string Lower;
	Lower.reserve(Text.size());
	for (char C : Text)
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
	if (Lower == "true" || Lower == "1")
	{
		Out = true;
		return true;
	}
	if (Lower == "false" || Lower == "0")
	{
		Out = false;
		return true;
	}
	return false;
}

bool RequireCols(const std::vector<std::string>& Header, const char* const* Names, int Count, std::vector<int>& Cols, std::string& Error)
{
	Cols.resize(static_cast<size_t>(Count));
	for (int I = 0; I < Count; ++I)
	{
		const int Col = FindCol(Header, Names[I], Error);
		if (Col < 0)
		{
			return false;
		}
		Cols[static_cast<size_t>(I)] = Col;
	}
	return true;
}

}

FRoomFlow::FRoomFlow()
	: Delay(FirstWaveDelayDefaultSec)
{
}

void FRoomFlow::ResetRuntime()
{
	Slots.clear();
	WaveGroups.clear();
	PendingOrders.clear();
	PhaseKind = EPhase::Idle;
	ArmRemaining = 0.f;
	Living = 0;
	Reserved = 0;
	PeakAliveCount = 0;
	PeakCommittedCount = 0;
	WaveCursor = 0;
	Entries = 0;
	OrdersIssuedCount = 0;
	bWantsOffer = false;
	bClearedSticky = false;
	bBacktrackKeptClear = false;
	bRefusedReset = false;
	bStalled = false;
	NoteText.clear();
}

void FRoomFlow::BumpPeak()
{
	if (Living > PeakAliveCount)
	{
		PeakAliveCount = Living;
	}
	const int Committed = Living + Reserved;
	if (Committed > PeakCommittedCount)
	{
		PeakCommittedCount = Committed;
	}
}

void FRoomFlow::SetFirstWaveDelay(float Seconds)
{
	Delay = ClampFirstWaveDelay(Seconds);
}

bool LoadRoomTables(
	std::string_view MetaCsv,
	std::string_view SpawnCsv,
	std::string_view ExpectedRoomId,
	FRoomMeta& Meta,
	std::vector<FSpawnRow>& Spawns,
	std::string& Error)
{
	Meta = FRoomMeta();
	Spawns.clear();
	Error.clear();

	std::vector<std::vector<std::string>> MetaRows;
	if (!ParseCsv(MetaCsv, MetaRows, Error))
	{
		return false;
	}
	if (MetaRows.size() < 2)
	{
		Error = "meta csv needs a header and one room";
		return false;
	}

	const char* MetaNames[] = {
		"room_id", "type", "tag", "design_width", "design_height", "max_alive",
		"clear_rule", "affix_on_clear", "require_clear_exit", "notes"
	};
	std::vector<int> MetaCols;
	if (!RequireCols(MetaRows[0], MetaNames, 10, MetaCols, Error))
	{
		return false;
	}
	const std::vector<std::string>& MetaLine = MetaRows[1];
	if (!Field(MetaLine, MetaCols[0], Meta.RoomId, Error, "room_id")
		|| !Field(MetaLine, MetaCols[1], Meta.Type, Error, "type")
		|| !Field(MetaLine, MetaCols[2], Meta.Tag, Error, "tag")
		|| !Field(MetaLine, MetaCols[6], Meta.ClearRule, Error, "clear_rule")
		|| !Field(MetaLine, MetaCols[8], Meta.RequireClearExit, Error, "require_clear_exit")
		|| !Field(MetaLine, MetaCols[9], Meta.Notes, Error, "notes"))
	{
		return false;
	}
	std::string Width;
	std::string Height;
	std::string MaxAlive;
	std::string Affix;
	if (!Field(MetaLine, MetaCols[3], Width, Error, "design_width")
		|| !Field(MetaLine, MetaCols[4], Height, Error, "design_height")
		|| !Field(MetaLine, MetaCols[5], MaxAlive, Error, "max_alive")
		|| !Field(MetaLine, MetaCols[7], Affix, Error, "affix_on_clear"))
	{
		return false;
	}
	if (!ParseFloat(Width, Meta.DesignWidth) || !ParseFloat(Height, Meta.DesignHeight) || !ParseInt(MaxAlive, Meta.MaxAlive) || !ParseBool(Affix, Meta.bAffixOnClear))
	{
		Error = "meta numeric or bool field";
		return false;
	}
	if (Meta.RoomId.empty() || Meta.ClearRule.empty() || Meta.RequireClearExit.empty() || Meta.MaxAlive < 1)
	{
		Error = "meta room_id, clear_rule, require_clear_exit, or max_alive";
		return false;
	}
	if (!ExpectedRoomId.empty() && Meta.RoomId != ExpectedRoomId)
	{
		Error = "room_id does not match the P0.7 acceptance room";
		return false;
	}

	std::vector<std::vector<std::string>> SpawnTable;
	if (!ParseCsv(SpawnCsv, SpawnTable, Error))
	{
		return false;
	}
	if (SpawnTable.empty())
	{
		Error = "spawn csv is empty";
		return false;
	}
	const char* SpawnNames[] = {
		"spawn_id", "spawn_tag", "wave_group", "x", "y", "facing", "optional", "enemy_id_placeholder", "notes"
	};
	std::vector<int> SpawnCols;
	if (!RequireCols(SpawnTable[0], SpawnNames, 9, SpawnCols, Error))
	{
		return false;
	}
	for (size_t R = 1; R < SpawnTable.size(); ++R)
	{
		const std::vector<std::string>& Line = SpawnTable[R];
		FSpawnRow Row;
		std::string Wave;
		std::string X;
		std::string Y;
		std::string Optional;
		if (!Field(Line, SpawnCols[0], Row.SpawnId, Error, "spawn_id")
			|| !Field(Line, SpawnCols[1], Row.SpawnTag, Error, "spawn_tag")
			|| !Field(Line, SpawnCols[2], Wave, Error, "wave_group")
			|| !Field(Line, SpawnCols[3], X, Error, "x")
			|| !Field(Line, SpawnCols[4], Y, Error, "y")
			|| !Field(Line, SpawnCols[5], Row.Facing, Error, "facing")
			|| !Field(Line, SpawnCols[6], Optional, Error, "optional")
			|| !Field(Line, SpawnCols[7], Row.EnemyId, Error, "enemy_id_placeholder")
			|| !Field(Line, SpawnCols[8], Row.Notes, Error, "notes"))
		{
			return false;
		}
		if (Row.SpawnId.empty() || !ParseInt(Wave, Row.WaveGroup) || Row.WaveGroup < 1 || !ParseFloat(X, Row.X) || !ParseFloat(Y, Row.Y) || !ParseBool(Optional, Row.bOptional))
		{
			Error = "spawn row " + Row.SpawnId;
			return false;
		}
		if (!Row.bOptional && Row.EnemyId.empty())
		{
			Error = "required spawn missing enemy id " + Row.SpawnId;
			return false;
		}
		for (const FSpawnRow& Prev : Spawns)
		{
			if (Prev.SpawnId == Row.SpawnId)
			{
				Error = "duplicate spawn id " + Row.SpawnId;
				return false;
			}
		}
		Spawns.push_back(std::move(Row));
	}
	return true;
}

bool LoadGeometryCsv(std::string_view Text, std::vector<FGeometryRow>& Rows, std::string& Error)
{
	Rows.clear();
	Error.clear();
	std::vector<std::vector<std::string>> Table;
	if (!ParseCsv(Text, Table, Error))
	{
		return false;
	}
	if (Table.empty())
	{
		Error = "geometry csv is empty";
		return false;
	}
	const char* Names[] = {
		"id", "kind", "x0", "x1", "x_center", "width", "top_y", "height", "facing", "rule", "notes"
	};
	std::vector<int> Cols;
	if (!RequireCols(Table[0], Names, 11, Cols, Error))
	{
		return false;
	}
	auto OptionalFloat = [](const std::string& Text, float& Out) {
		if (Text.empty())
		{
			Out = 0.f;
			return true;
		}
		return ParseFloat(Text, Out);
	};
	for (size_t R = 1; R < Table.size(); ++R)
	{
		const std::vector<std::string>& Line = Table[R];
		FGeometryRow Row;
		std::string X0;
		std::string X1;
		std::string XCenter;
		std::string Width;
		std::string TopY;
		std::string Height;
		if (!Field(Line, Cols[0], Row.Id, Error, "id")
			|| !Field(Line, Cols[1], Row.Kind, Error, "kind")
			|| !Field(Line, Cols[2], X0, Error, "x0")
			|| !Field(Line, Cols[3], X1, Error, "x1")
			|| !Field(Line, Cols[4], XCenter, Error, "x_center")
			|| !Field(Line, Cols[5], Width, Error, "width")
			|| !Field(Line, Cols[6], TopY, Error, "top_y")
			|| !Field(Line, Cols[7], Height, Error, "height")
			|| !Field(Line, Cols[8], Row.Facing, Error, "facing")
			|| !Field(Line, Cols[9], Row.Rule, Error, "rule")
			|| !Field(Line, Cols[10], Row.Notes, Error, "notes"))
		{
			return false;
		}
		if (!OptionalFloat(X0, Row.X0) || !OptionalFloat(X1, Row.X1) || !OptionalFloat(XCenter, Row.XCenter)
			|| !OptionalFloat(Width, Row.Width) || !OptionalFloat(TopY, Row.TopY) || !OptionalFloat(Height, Row.Height))
		{
			Error = "geometry numeric field " + Row.Id;
			return false;
		}
		if (Row.Id.empty() || Row.Kind.empty())
		{
			Error = "geometry id or kind";
			return false;
		}
		Rows.push_back(std::move(Row));
	}
	return true;
}

bool FindAnchor(const std::vector<FGeometryRow>& Rows, std::string_view Id, float& X, float& Y)
{
	for (const FGeometryRow& Row : Rows)
	{
		if (Row.Kind == "Anchor" && Row.Id == Id)
		{
			X = Row.X0;
			Y = Row.X1;
			return true;
		}
	}
	return false;
}

bool FRoomFlow::Load(std::string_view MetaCsv, std::string_view SpawnCsv, std::string_view ExpectedRoomId, std::string& Error)
{
	const float KeptDelay = Delay;
	FRoomMeta LoadedMeta;
	std::vector<FSpawnRow> LoadedSpawns;
	if (!LoadRoomTables(MetaCsv, SpawnCsv, ExpectedRoomId, LoadedMeta, LoadedSpawns, Error))
	{
		return false;
	}
	ResetRuntime();
	Delay = KeptDelay;
	MetaRow = std::move(LoadedMeta);
	SpawnRows = std::move(LoadedSpawns);
	for (const FSpawnRow& Row : SpawnRows)
	{
		if (Row.bOptional)
		{
			continue;
		}
		FSlot Slot;
		Slot.Row = Row;
		Slot.State = ESlotState::Pending;
		Slots.push_back(std::move(Slot));
		bool bSeen = false;
		for (int Group : WaveGroups)
		{
			if (Group == Row.WaveGroup)
			{
				bSeen = true;
				break;
			}
		}
		if (!bSeen)
		{
			WaveGroups.push_back(Row.WaveGroup);
		}
	}
	for (size_t I = 1; I < WaveGroups.size(); ++I)
	{
		const int Key = WaveGroups[I];
		size_t J = I;
		while (J > 0 && WaveGroups[J - 1] > Key)
		{
			WaveGroups[J] = WaveGroups[J - 1];
			--J;
		}
		WaveGroups[J] = Key;
	}
	return true;
}

FRoomFlow::FSlot* FRoomFlow::FindSlot(std::string_view SpawnId)
{
	for (FSlot& Slot : Slots)
	{
		if (Slot.Row.SpawnId == SpawnId)
		{
			return &Slot;
		}
	}
	return nullptr;
}

FSpawnOrder FRoomFlow::MakeOrder(const FSpawnRow& Row) const
{
	FSpawnOrder Order;
	Order.SpawnId = Row.SpawnId;
	Order.EnemyId = Row.EnemyId;
	Order.Kind = ResolveEnemyKind(Row.EnemyId);
	Order.WaveGroup = Row.WaveGroup;
	Order.X = Row.X;
	Order.Y = Row.Y;
	Order.Facing = Row.Facing.empty() ? std::string("Left") : Row.Facing;
	Order.FacingSign = FacingSignFromName(Order.Facing);
	return Order;
}

void FRoomFlow::FillCap()
{
	if (WaveCursor < 0 || WaveCursor >= static_cast<int>(WaveGroups.size()))
	{
		return;
	}
	const int Group = WaveGroups[static_cast<size_t>(WaveCursor)];
	for (FSlot& Slot : Slots)
	{
		if (Slot.Row.WaveGroup != Group || Slot.State != ESlotState::Pending)
		{
			continue;
		}
		if (Living + Reserved >= MetaRow.MaxAlive)
		{
			break;
		}
		PendingOrders.push_back(MakeOrder(Slot.Row));
		Slot.State = ESlotState::Queued;
		++Reserved;
		++OrdersIssuedCount;
		BumpPeak();
	}
}

bool FRoomFlow::CurrentWaveFullyDead() const
{
	if (WaveCursor < 0 || WaveCursor >= static_cast<int>(WaveGroups.size()))
	{
		return false;
	}
	const int Group = WaveGroups[static_cast<size_t>(WaveCursor)];
	bool bAny = false;
	for (const FSlot& Slot : Slots)
	{
		if (Slot.Row.WaveGroup != Group)
		{
			continue;
		}
		bAny = true;
		if (Slot.State != ESlotState::Dead)
		{
			return false;
		}
	}
	return bAny;
}

void FRoomFlow::MarkCleared()
{
	PhaseKind = EPhase::Cleared;
	bClearedSticky = true;
	bWantsOffer = false;
}

void FRoomFlow::FinishWaves()
{
	if (MetaRow.ClearRule != ClearRuleWaves)
	{
		bStalled = true;
		NoteText = "unsupported clear_rule";
		bWantsOffer = false;
		return;
	}
	if (MetaRow.bAffixOnClear)
	{
		PhaseKind = EPhase::AwaitingAffix;
		bWantsOffer = true;
		return;
	}
	MarkCleared();
}

void FRoomFlow::StartCurrent()
{
	while (true)
	{
		if (WaveCursor >= static_cast<int>(WaveGroups.size()))
		{
			FinishWaves();
			return;
		}
		PhaseKind = EPhase::Fighting;
		FillCap();
		if (!CurrentWaveFullyDead())
		{
			return;
		}
		++WaveCursor;
	}
}

void FRoomFlow::NotifyEntered()
{
	++Entries;
	if (bClearedSticky || PhaseKind == EPhase::Cleared)
	{
		bRefusedReset = true;
		return;
	}
	if (PhaseKind != EPhase::Idle)
	{
		return;
	}
	PhaseKind = EPhase::Arming;
	ArmRemaining = Delay;
}

void FRoomFlow::Tick(float DeltaSeconds)
{
	if (PhaseKind != EPhase::Arming)
	{
		return;
	}
	if (DeltaSeconds < 0.f)
	{
		DeltaSeconds = 0.f;
	}
	ArmRemaining -= DeltaSeconds;
	if (ArmRemaining <= 0.f)
	{
		WaveCursor = 0;
		StartCurrent();
	}
}

void FRoomFlow::NotifySpawned(std::string_view SpawnId)
{
	FSlot* Slot = FindSlot(SpawnId);
	if (!Slot || Slot->State != ESlotState::Queued)
	{
		return;
	}
	Slot->State = ESlotState::Alive;
	if (Reserved > 0)
	{
		--Reserved;
	}
	++Living;
	BumpPeak();
}

void FRoomFlow::NotifySpawnFailed(std::string_view SpawnId)
{
	FSlot* Slot = FindSlot(SpawnId);
	if (!Slot || Slot->State != ESlotState::Queued)
	{
		return;
	}
	Slot->State = ESlotState::Failed;
	if (Reserved > 0)
	{
		--Reserved;
	}
	NoteText = "spawn failed " + Slot->Row.SpawnId;
	BumpPeak();
}

void FRoomFlow::NotifyEnemyDied(std::string_view SpawnId)
{
	FSlot* Slot = FindSlot(SpawnId);
	if (!Slot || Slot->State != ESlotState::Alive)
	{
		return;
	}
	Slot->State = ESlotState::Dead;
	if (Living > 0)
	{
		--Living;
	}
	BumpPeak();
	if (PhaseKind != EPhase::Fighting)
	{
		return;
	}
	if (CurrentWaveFullyDead())
	{
		++WaveCursor;
		StartCurrent();
		return;
	}
	FillCap();
}

EExitTouch FRoomFlow::NotifyExitUsed(std::string_view ExitId)
{
	if (ExitId.empty())
	{
		return EExitTouch::Ignored;
	}
	if (ExitId == ExitLeftId && (bClearedSticky || PhaseKind == EPhase::Cleared))
	{
		bBacktrackKeptClear = true;
		return EExitTouch::BacktrackKeptClear;
	}
	if (!IsExitOpen(ExitId))
	{
		return EExitTouch::DeniedLocked;
	}
	return EExitTouch::Passed;
}

void FRoomFlow::NotifyChoiceClosed(bool bConfirmed)
{
	if (PhaseKind != EPhase::AwaitingAffix)
	{
		return;
	}
	if (bConfirmed)
	{
		MarkCleared();
		return;
	}
	bWantsOffer = true;
}

void FRoomFlow::MarkAffixRequested()
{
	bWantsOffer = false;
}

std::vector<FSpawnOrder> FRoomFlow::ConsumeSpawnOrders()
{
	std::vector<FSpawnOrder> Out;
	Out.swap(PendingOrders);
	return Out;
}

bool FRoomFlow::WantsAffixOffer() const
{
	return PhaseKind == EPhase::AwaitingAffix && bWantsOffer;
}

bool FRoomFlow::IsExitOpen(std::string_view ExitId) const
{
	if (ExitId.empty())
	{
		return false;
	}
	if (ExitId == MetaRow.RequireClearExit)
	{
		return PhaseKind == EPhase::Cleared;
	}
	return true;
}

int FRoomFlow::CurrentWaveGroup() const
{
	if (WaveGroups.empty())
	{
		return 0;
	}
	if (PhaseKind == EPhase::Idle || PhaseKind == EPhase::Arming)
	{
		return 0;
	}
	int Index = WaveCursor;
	if (Index < 0)
	{
		Index = 0;
	}
	if (Index >= static_cast<int>(WaveGroups.size()))
	{
		Index = static_cast<int>(WaveGroups.size()) - 1;
	}
	return WaveGroups[static_cast<size_t>(Index)];
}

}
