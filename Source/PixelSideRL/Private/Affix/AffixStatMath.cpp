// Copyright PixelSideRL. P0.5 affix math, JSON/CSV load, and choice roll.
#include "Affix/AffixStatMath.h"
#include "Combat/PixelSideCombatMath.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <utility>

namespace PixelSideAffix
{
namespace
{

struct Json
{
	enum class Type
	{
		Null,
		Bool,
		Number,
		String,
		Array,
		Object
	};

	Type Kind = Type::Null;
	bool Bool = false;
	double Number = 0.0;
	std::string String;
	std::vector<Json> Array;
	std::vector<std::pair<std::string, Json>> Object;

	const Json* Find(std::string_view Key) const
	{
		for (const auto& Pair : Object)
		{
			if (Pair.first == Key)
			{
				return &Pair.second;
			}
		}
		return nullptr;
	}
};

class JsonParser
{
public:
	explicit JsonParser(std::string_view In)
		: Text(In)
	{
		if (Text.size() >= 3
			&& static_cast<unsigned char>(Text[0]) == 0xEF
			&& static_cast<unsigned char>(Text[1]) == 0xBB
			&& static_cast<unsigned char>(Text[2]) == 0xBF)
		{
			Index = 3;
		}
	}

	bool Parse(Json& Out, std::string& Error)
	{
		Skip();
		Out = ParseValue();
		if (!Ok)
		{
			Error = Err;
			return false;
		}
		Skip();
		if (Index != Text.size())
		{
			Error = "trailing junk at " + Loc();
			return false;
		}
		return true;
	}

private:
	std::string_view Text;
	size_t Index = 0;
	int Depth = 0;
	bool Ok = true;
	std::string Err;

	static bool IsWs(char C)
	{
		return C == ' ' || C == '\t' || C == '\n' || C == '\r';
	}

	std::string Loc() const
	{
		int Line = 1;
		int Col = 1;
		for (size_t K = 0; K < Index && K < Text.size(); ++K)
		{
			if (Text[K] == '\n')
			{
				++Line;
				Col = 1;
			}
			else
			{
				++Col;
			}
		}
		return "line " + std::to_string(Line) + " col " + std::to_string(Col);
	}

	void Fail(const std::string& Message)
	{
		if (Ok)
		{
			Ok = false;
			Err = Message + " at " + Loc();
		}
	}

	void Skip()
	{
		while (Index < Text.size() && IsWs(Text[Index]))
		{
			++Index;
		}
	}

	bool Consume(char C)
	{
		Skip();
		if (Index < Text.size() && Text[Index] == C)
		{
			++Index;
			return true;
		}
		return false;
	}

	void AppendUtf8(std::string& Out, uint32_t Cp)
	{
		if (Cp <= 0x7F)
		{
			Out.push_back(static_cast<char>(Cp));
		}
		else if (Cp <= 0x7FF)
		{
			Out.push_back(static_cast<char>(0xC0 | (Cp >> 6)));
			Out.push_back(static_cast<char>(0x80 | (Cp & 0x3F)));
		}
		else if (Cp <= 0xFFFF)
		{
			Out.push_back(static_cast<char>(0xE0 | (Cp >> 12)));
			Out.push_back(static_cast<char>(0x80 | ((Cp >> 6) & 0x3F)));
			Out.push_back(static_cast<char>(0x80 | (Cp & 0x3F)));
		}
		else
		{
			Out.push_back(static_cast<char>(0xF0 | (Cp >> 18)));
			Out.push_back(static_cast<char>(0x80 | ((Cp >> 12) & 0x3F)));
			Out.push_back(static_cast<char>(0x80 | ((Cp >> 6) & 0x3F)));
			Out.push_back(static_cast<char>(0x80 | (Cp & 0x3F)));
		}
	}

	std::string ParseString()
	{
		std::string Out;
		if (Index >= Text.size() || Text[Index] != '"')
		{
			Fail("expected string");
			return Out;
		}
		++Index;
		while (Index < Text.size())
		{
			const char C = Text[Index++];
			if (C == '"')
			{
				return Out;
			}
			if (C != '\\')
			{
				Out.push_back(C);
				continue;
			}
			if (Index >= Text.size())
			{
				Fail("bad escape");
				return Out;
			}
			const char E = Text[Index++];
			switch (E)
			{
			case '"':
			case '\\':
			case '/':
				Out.push_back(E);
				break;
			case 'b':
				Out.push_back('\b');
				break;
			case 'f':
				Out.push_back('\f');
				break;
			case 'n':
				Out.push_back('\n');
				break;
			case 'r':
				Out.push_back('\r');
				break;
			case 't':
				Out.push_back('\t');
				break;
			case 'u':
			{
				if (Index + 4 > Text.size())
				{
					Fail("bad unicode escape");
					return Out;
				}
				uint32_t Cp = 0;
				for (int K = 0; K < 4; ++K)
				{
					const char H = Text[Index++];
					Cp <<= 4;
					if (H >= '0' && H <= '9')
					{
						Cp += static_cast<uint32_t>(H - '0');
					}
					else if (H >= 'a' && H <= 'f')
					{
						Cp += static_cast<uint32_t>(H - 'a' + 10);
					}
					else if (H >= 'A' && H <= 'F')
					{
						Cp += static_cast<uint32_t>(H - 'A' + 10);
					}
					else
					{
						Fail("bad unicode escape");
						return Out;
					}
				}
				AppendUtf8(Out, Cp);
				break;
			}
			default:
				Fail("bad escape");
				return Out;
			}
		}
		Fail("unterminated string");
		return Out;
	}

	Json ParseValue()
	{
		Json Out;
		if (!Ok)
		{
			return Out;
		}
		if (Depth > 32)
		{
			Fail("json too deep");
			return Out;
		}
		Skip();
		if (Index >= Text.size())
		{
			Fail("unexpected end");
			return Out;
		}
		const char C = Text[Index];
		if (C == '"')
		{
			Out.Kind = Json::Type::String;
			Out.String = ParseString();
			return Out;
		}
		if (C == '{')
		{
			++Index;
			++Depth;
			Out.Kind = Json::Type::Object;
			Skip();
			if (Consume('}'))
			{
				--Depth;
				return Out;
			}
			for (;;)
			{
				Skip();
				const std::string Key = ParseString();
				if (!Consume(':'))
				{
					Fail("expected ':'");
					return Out;
				}
				Json Child = ParseValue();
				Out.Object.emplace_back(Key, std::move(Child));
				Skip();
				if (Consume('}'))
				{
					--Depth;
					return Out;
				}
				if (!Consume(','))
				{
					Fail("expected ',' or '}'");
					return Out;
				}
			}
		}
		if (C == '[')
		{
			++Index;
			++Depth;
			Out.Kind = Json::Type::Array;
			Skip();
			if (Consume(']'))
			{
				--Depth;
				return Out;
			}
			for (;;)
			{
				Out.Array.push_back(ParseValue());
				Skip();
				if (Consume(']'))
				{
					--Depth;
					return Out;
				}
				if (!Consume(','))
				{
					Fail("expected ',' or ']'");
					return Out;
				}
			}
		}
		if (C == 't' && Text.substr(Index, 4) == "true")
		{
			Index += 4;
			Out.Kind = Json::Type::Bool;
			Out.Bool = true;
			return Out;
		}
		if (C == 'f' && Text.substr(Index, 5) == "false")
		{
			Index += 5;
			Out.Kind = Json::Type::Bool;
			Out.Bool = false;
			return Out;
		}
		if (C == 'n' && Text.substr(Index, 4) == "null")
		{
			Index += 4;
			Out.Kind = Json::Type::Null;
			return Out;
		}
		if (C == '-' || (C >= '0' && C <= '9'))
		{
			const size_t Start = Index;
			if (Text[Index] == '-')
			{
				++Index;
			}
			if (Index >= Text.size() || Text[Index] < '0' || Text[Index] > '9')
			{
				Fail("bad number");
				return Out;
			}
			while (Index < Text.size() && Text[Index] >= '0' && Text[Index] <= '9')
			{
				++Index;
			}
			if (Index < Text.size() && Text[Index] == '.')
			{
				++Index;
				if (Index >= Text.size() || Text[Index] < '0' || Text[Index] > '9')
				{
					Fail("bad number");
					return Out;
				}
				while (Index < Text.size() && Text[Index] >= '0' && Text[Index] <= '9')
				{
					++Index;
				}
			}
			if (Index < Text.size() && (Text[Index] == 'e' || Text[Index] == 'E'))
			{
				++Index;
				if (Index < Text.size() && (Text[Index] == '+' || Text[Index] == '-'))
				{
					++Index;
				}
				if (Index >= Text.size() || Text[Index] < '0' || Text[Index] > '9')
				{
					Fail("bad number");
					return Out;
				}
				while (Index < Text.size() && Text[Index] >= '0' && Text[Index] <= '9')
				{
					++Index;
				}
			}
			const std::string Slice(Text.substr(Start, Index - Start));
			char* End = nullptr;
			const double Value = std::strtod(Slice.c_str(), &End);
			if (!End || *End != '\0' || !std::isfinite(Value))
			{
				Fail("bad number");
				return Out;
			}
			Out.Kind = Json::Type::Number;
			Out.Number = Value;
			return Out;
		}
		Fail("unexpected token");
		return Out;
	}
};

bool ParseJson(std::string_view Text, Json& Out, std::string& Error)
{
	JsonParser Parser(Text);
	return Parser.Parse(Out, Error);
}

const char* kStatNames[] = {
	"MaxHP",
	"MoveSpeed",
	"FireRate",
	"Damage",
	"CritChance",
	"CritMult",
	"MagSize",
	"ReloadSpeed",
	"ProjectileCount",
	"Pierce",
	"Knockback",
	"PickupRadius",
	"CoinGain",
	"ScrapGain"
};

const char* kOpNames[] = {"Add", "Mul", "Override"};
const char* kStackNames[] = {"Unique", "AdditiveStack", "RefreshDuration"};
const char* kRarityNames[] = {"Common", "Rare", "Epic", "Legendary"};
const char* kTriggerNames[] = {"OnAcquire", "OnHit", "OnKill", "OnRoomClear", "OnTakeDamage", "Passive"};

static_assert(sizeof(kStatNames) / sizeof(kStatNames[0]) == static_cast<size_t>(EStat::Count), "stat names");

bool ParseEnum(std::string_view Text, const char* const* Names, int Count, int& Out)
{
	for (int I = 0; I < Count; ++I)
	{
		if (Text == Names[I])
		{
			Out = I;
			return true;
		}
	}
	return false;
}

bool ReadString(const Json& Obj, const char* Key, std::string& Out, bool bRequired, std::string& Error)
{
	const Json* Value = Obj.Find(Key);
	if (!Value || Value->Kind == Json::Type::Null)
	{
		if (bRequired)
		{
			Error = std::string(Key) + " missing";
			return false;
		}
		return true;
	}
	if (Value->Kind != Json::Type::String)
	{
		Error = std::string(Key) + " not a string";
		return false;
	}
	Out = Value->String;
	return true;
}

bool ReadNumber(const Json& Obj, const char* Key, double& Out, bool bRequired, std::string& Error)
{
	const Json* Value = Obj.Find(Key);
	if (!Value || Value->Kind == Json::Type::Null)
	{
		if (bRequired)
		{
			Error = std::string(Key) + " missing";
			return false;
		}
		return true;
	}
	if (Value->Kind != Json::Type::Number || !std::isfinite(Value->Number))
	{
		Error = std::string(Key) + " not a number";
		return false;
	}
	Out = Value->Number;
	return true;
}

bool ReadStringList(const Json& Obj, const char* Key, std::vector<std::string>& Out, std::string& Error)
{
	const Json* Value = Obj.Find(Key);
	if (!Value || Value->Kind == Json::Type::Null)
	{
		return true;
	}
	if (Value->Kind != Json::Type::Array)
	{
		Error = std::string(Key) + " not an array";
		return false;
	}
	for (const Json& Item : Value->Array)
	{
		if (Item.Kind != Json::Type::String)
		{
			Error = std::string(Key) + " item not a string";
			return false;
		}
		Out.push_back(Item.String);
	}
	return true;
}

bool ReadAffixObject(const Json& Obj, FAffixRow& Row, std::string& Error)
{
	if (Obj.Kind != Json::Type::Object)
	{
		Error = "affix is not an object";
		return false;
	}
	if (!ReadString(Obj, "AffixId", Row.AffixId, true, Error))
	{
		return false;
	}
	if (!ReadString(Obj, "DisplayName", Row.DisplayName, true, Error))
	{
		return false;
	}
	if (!ReadString(Obj, "Description", Row.Description, true, Error))
	{
		return false;
	}
	std::string Rarity;
	std::string Stack;
	if (!ReadString(Obj, "Rarity", Rarity, true, Error))
	{
		return false;
	}
	if (!ReadString(Obj, "StackRule", Stack, true, Error))
	{
		return false;
	}
	if (!ParseRarity(Rarity, Row.Rarity) || !ParseStack(Stack, Row.StackRule))
	{
		Error = Row.AffixId + " bad Rarity or StackRule";
		return false;
	}
	if (!ReadString(Obj, "Icon", Row.Icon, false, Error))
	{
		return false;
	}
	double MaxStacks = static_cast<double>(Row.MaxStacks);
	double Weight = static_cast<double>(Row.Weight);
	if (!ReadNumber(Obj, "MaxStacks", MaxStacks, false, Error))
	{
		return false;
	}
	if (!ReadNumber(Obj, "Weight", Weight, false, Error))
	{
		return false;
	}
	Row.MaxStacks = static_cast<int>(std::lround(MaxStacks));
	if (Row.MaxStacks < 1)
	{
		Row.MaxStacks = 1;
	}
	Row.Weight = static_cast<float>(Weight);
	if (!ReadStringList(Obj, "Tags", Row.Tags, Error))
	{
		return false;
	}
	if (!ReadStringList(Obj, "ExclusiveTags", Row.ExclusiveTags, Error))
	{
		return false;
	}
	std::vector<std::string> Triggers;
	if (!ReadStringList(Obj, "Triggers", Triggers, Error))
	{
		return false;
	}
	for (const std::string& Name : Triggers)
	{
		ETrigger Trigger = ETrigger::Passive;
		if (!ParseTrigger(Name, Trigger))
		{
			Error = Row.AffixId + " bad trigger " + Name;
			return false;
		}
		Row.Triggers.push_back(Trigger);
	}
	const Json* Mods = Obj.Find("Modifiers");
	if (!Mods || Mods->Kind != Json::Type::Array)
	{
		Error = Row.AffixId + " Modifiers missing";
		return false;
	}
	for (const Json& ModObj : Mods->Array)
	{
		if (ModObj.Kind != Json::Type::Object)
		{
			Error = Row.AffixId + " modifier not an object";
			return false;
		}
		FMod Mod;
		std::string Stat;
		std::string Op;
		if (!ReadString(ModObj, "Stat", Stat, true, Error))
		{
			return false;
		}
		if (!ReadString(ModObj, "Op", Op, true, Error))
		{
			return false;
		}
		if (!ParseStat(Stat, Mod.Stat) || !ParseOp(Op, Mod.Op))
		{
			Error = Row.AffixId + " bad Stat or Op";
			return false;
		}
		double Value = 0.0;
		double Duration = 0.0;
		if (!ReadNumber(ModObj, "Value", Value, true, Error))
		{
			return false;
		}
		if (!ReadNumber(ModObj, "DurationSec", Duration, false, Error))
		{
			return false;
		}
		Mod.Value = static_cast<float>(Value);
		Mod.DurationSec = static_cast<float>(Duration);
		Row.Modifiers.push_back(Mod);
	}
	if (!ReadString(Obj, "DataAssetName", Row.DataAssetName, false, Error))
	{
		return false;
	}
	if (Row.DataAssetName.empty())
	{
		Row.DataAssetName = std::string("DA_Affix_") + Row.AffixId;
	}
	return true;
}

bool SplitList(std::string_view Text, char Sep, std::vector<std::string>& Out)
{
	Out.clear();
	std::string Cur;
	for (char C : Text)
	{
		if (C == Sep)
		{
			if (!Cur.empty())
			{
				Out.push_back(Cur);
				Cur.clear();
			}
		}
		else
		{
			Cur.push_back(C);
		}
	}
	if (!Cur.empty())
	{
		Out.push_back(Cur);
	}
	return true;
}

std::vector<std::string> SplitCsvLine(std::string_view Line, std::string& Error)
{
	std::vector<std::string> Fields;
	std::string Cur;
	bool bQuote = false;
	for (size_t I = 0; I < Line.size(); ++I)
	{
		const char C = Line[I];
		if (bQuote)
		{
			if (C == '"')
			{
				if (I + 1 < Line.size() && Line[I + 1] == '"')
				{
					Cur.push_back('"');
					++I;
				}
				else
				{
					bQuote = false;
				}
			}
			else
			{
				Cur.push_back(C);
			}
		}
		else if (C == '"')
		{
			bQuote = true;
		}
		else if (C == ',')
		{
			Fields.push_back(Cur);
			Cur.clear();
		}
		else
		{
			Cur.push_back(C);
		}
	}
	if (bQuote)
	{
		Error = "unterminated csv quote";
		return {};
	}
	Fields.push_back(Cur);
	return Fields;
}

bool HasPassiveTrigger(const FAffixRow& Row)
{
	if (Row.Triggers.empty())
	{
		return true;
	}
	for (ETrigger Trigger : Row.Triggers)
	{
		if (Trigger == ETrigger::Passive)
		{
			return true;
		}
	}
	return false;
}

bool TriggersOnAcquire(const FAffixRow& Row)
{
	if (Row.Triggers.empty())
	{
		return true;
	}
	for (ETrigger Trigger : Row.Triggers)
	{
		if (Trigger == ETrigger::Passive || Trigger == ETrigger::OnAcquire)
		{
			return true;
		}
	}
	return false;
}

FAffixRow MakeRow(
	const char* Id,
	const char* Name,
	const char* Desc,
	ERarity Rarity,
	EStack Stack,
	int MaxStacks,
	float Weight,
	std::initializer_list<const char*> Tags,
	std::initializer_list<const char*> Exclusive,
	std::initializer_list<ETrigger> Triggers,
	std::initializer_list<FMod> Mods)
{
	FAffixRow Row;
	Row.AffixId = Id;
	Row.DisplayName = Name;
	Row.Description = Desc;
	Row.Rarity = Rarity;
	Row.StackRule = Stack;
	Row.MaxStacks = MaxStacks;
	Row.Weight = Weight;
	for (const char* Tag : Tags)
	{
		Row.Tags.emplace_back(Tag);
	}
	for (const char* Tag : Exclusive)
	{
		Row.ExclusiveTags.emplace_back(Tag);
	}
	Row.Triggers.assign(Triggers.begin(), Triggers.end());
	Row.Modifiers.assign(Mods.begin(), Mods.end());
	Row.DataAssetName = std::string("DA_Affix_") + Id;
	return Row;
}

bool ReadBiasRoom(const Json& Rooms, const char* Name, FRarityWeights& Out, std::string& Error)
{
	const Json* Room = Rooms.Find(Name);
	if (!Room || Room->Kind != Json::Type::Object)
	{
		Error = std::string(Name) + " missing";
		return false;
	}
	double Common = 0;
	double Rare = 0;
	double Epic = 0;
	double Legendary = Out.Legendary;
	if (!ReadNumber(*Room, "Common", Common, true, Error))
	{
		return false;
	}
	if (!ReadNumber(*Room, "Rare", Rare, true, Error))
	{
		return false;
	}
	if (!ReadNumber(*Room, "Epic", Epic, true, Error))
	{
		return false;
	}
	if (!ReadNumber(*Room, "Legendary", Legendary, false, Error))
	{
		return false;
	}
	Out.Common = static_cast<float>(Common);
	Out.Rare = static_cast<float>(Rare);
	Out.Epic = static_cast<float>(Epic);
	Out.Legendary = static_cast<float>(Legendary);
	return true;
}

} // namespace

const char* StatName(EStat Stat)
{
	const int Index = static_cast<int>(Stat);
	if (Index < 0 || Index >= static_cast<int>(EStat::Count))
	{
		return "";
	}
	return kStatNames[Index];
}

const char* OpName(EOp Op)
{
	const int Index = static_cast<int>(Op);
	if (Index < 0 || Index >= 3)
	{
		return "";
	}
	return kOpNames[Index];
}

const char* StackName(EStack Stack)
{
	const int Index = static_cast<int>(Stack);
	if (Index < 0 || Index >= 3)
	{
		return "";
	}
	return kStackNames[Index];
}

const char* RarityName(ERarity Rarity)
{
	const int Index = static_cast<int>(Rarity);
	if (Index < 0 || Index >= 4)
	{
		return "";
	}
	return kRarityNames[Index];
}

const char* TriggerName(ETrigger Trigger)
{
	const int Index = static_cast<int>(Trigger);
	if (Index < 0 || Index >= 6)
	{
		return "";
	}
	return kTriggerNames[Index];
}

bool ParseStat(std::string_view Text, EStat& Out)
{
	int Index = 0;
	if (!ParseEnum(Text, kStatNames, static_cast<int>(EStat::Count), Index))
	{
		return false;
	}
	Out = static_cast<EStat>(Index);
	return true;
}

bool ParseOp(std::string_view Text, EOp& Out)
{
	int Index = 0;
	if (!ParseEnum(Text, kOpNames, 3, Index))
	{
		return false;
	}
	Out = static_cast<EOp>(Index);
	return true;
}

bool ParseStack(std::string_view Text, EStack& Out)
{
	int Index = 0;
	if (!ParseEnum(Text, kStackNames, 3, Index))
	{
		return false;
	}
	Out = static_cast<EStack>(Index);
	return true;
}

bool ParseRarity(std::string_view Text, ERarity& Out)
{
	int Index = 0;
	if (!ParseEnum(Text, kRarityNames, 4, Index))
	{
		return false;
	}
	Out = static_cast<ERarity>(Index);
	return true;
}

bool ParseTrigger(std::string_view Text, ETrigger& Out)
{
	int Index = 0;
	if (!ParseEnum(Text, kTriggerNames, 6, Index))
	{
		return false;
	}
	Out = static_cast<ETrigger>(Index);
	return true;
}

float DefaultBase(EStat Stat)
{
	switch (Stat)
	{
	case EStat::MaxHP:
		return PixelSideCombat::PlayerMaxHealth;
	case EStat::MoveSpeed:
		return PixelSideCombat::MaxWalkSpeed;
	case EStat::FireRate:
		return PixelSideCombat::FireRateBase;
	case EStat::Damage:
		return PixelSideCombat::PeaDamage;
	case EStat::Knockback:
		return PixelSideCombat::PeaKnockback;
	case EStat::ProjectileCount:
		return 1.f;
	case EStat::Pierce:
		return 0.f;
	case EStat::CritChance:
		return 0.f;
	case EStat::CritMult:
		return 1.f;
	case EStat::ReloadSpeed:
		return 1.f;
	case EStat::CoinGain:
		return 1.f;
	case EStat::ScrapGain:
		return 1.f;
	case EStat::MagSize:
	case EStat::PickupRadius:
		return 0.f;
	default:
		return 0.f;
	}
}

float ApplyLayer(float Current, const std::vector<FMod>& Mods, EStat Stat)
{
	float Add = 0.f;
	float Mul = 1.f;
	bool bOverride = false;
	float OverrideValue = 0.f;
	for (const FMod& Mod : Mods)
	{
		if (Mod.Stat != Stat)
		{
			continue;
		}
		switch (Mod.Op)
		{
		case EOp::Add:
			Add += Mod.Value;
			break;
		case EOp::Mul:
			Mul *= (1.f + Mod.Value);
			break;
		case EOp::Override:
			bOverride = true;
			OverrideValue = Mod.Value;
			break;
		}
	}
	if (bOverride)
	{
		return OverrideValue;
	}
	return (Current + Add) * Mul;
}

std::string FormatDescription(std::string_view Description, float FirstModifierValue, int PreviewStacks)
{
	const int Stacks = PreviewStacks < 1 ? 1 : PreviewStacks;
	const float Shown = FirstModifierValue * static_cast<float>(Stacks);
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%.4g", static_cast<double>(Shown));
	std::string Out(Description);
	const std::string Token = "{Value}";
	for (;;)
	{
		const size_t At = Out.find(Token);
		if (At == std::string::npos)
		{
			break;
		}
		Out.replace(At, Token.size(), Buf);
	}
	return Out;
}

const std::vector<FAffixRow>& BuiltinP05Rows()
{
	static const std::vector<FAffixRow> Rows = {
		MakeRow("RapidFire", "速射", "射速 +{Value}", ERarity::Common, EStack::AdditiveStack, 3, 10.f,
			{"Offense", "Weapon"}, {}, {ETrigger::Passive},
			{FMod{EStat::FireRate, EOp::Mul, 0.12f, 0.f}}),
		MakeRow("HollowPoint", "空尖弹", "伤害 +{Value}", ERarity::Common, EStack::AdditiveStack, 3, 10.f,
			{"Offense"}, {}, {ETrigger::Passive},
			{FMod{EStat::Damage, EOp::Mul, 0.1f, 0.f}}),
		MakeRow("ThickSkin", "厚皮", "最大生命 +{Value}", ERarity::Common, EStack::AdditiveStack, 5, 10.f,
			{"Defense"}, {}, {ETrigger::Passive},
			{FMod{EStat::MaxHP, EOp::Add, 10.f, 0.f}}),
		MakeRow("Magnet", "磁吸", "拾取半径 +{Value}", ERarity::Common, EStack::Unique, 1, 8.f,
			{"Economy"}, {}, {ETrigger::Passive},
			{FMod{EStat::PickupRadius, EOp::Add, 48.f, 0.f}}),
		MakeRow("Piercer", "穿甲", "穿透 +1", ERarity::Rare, EStack::AdditiveStack, 2, 5.f,
			{"Offense", "Weapon"}, {}, {ETrigger::Passive},
			{FMod{EStat::Pierce, EOp::Add, 1.f, 0.f}}),
		MakeRow("TwinShot", "双生弹", "弹数 +1", ERarity::Rare, EStack::Unique, 1, 4.f,
			{"Offense", "Weapon"}, {"ProjectileShape"}, {ETrigger::Passive},
			{FMod{EStat::ProjectileCount, EOp::Add, 1.f, 0.f}}),
		MakeRow("CritOptics", "暴击瞄具", "暴击率 +{Value}", ERarity::Rare, EStack::AdditiveStack, 3, 5.f,
			{"Offense"}, {}, {ETrigger::Passive},
			{FMod{EStat::CritChance, EOp::Add, 0.08f, 0.f}}),
		MakeRow("Scavenger", "拾荒者", "局内 Scrap 收益 +{Value}", ERarity::Common, EStack::AdditiveStack, 3, 8.f,
			{"Economy"}, {}, {ETrigger::Passive},
			{FMod{EStat::ScrapGain, EOp::Mul, 0.15f, 0.f}}),
	};
	return Rows;
}

FBiasTable BuiltinBias()
{
	FBiasTable Bias;
	Bias.NormalRoom = {0.7f, 0.25f, 0.05f, 0.f};
	Bias.EliteRoom = {0.5f, 0.35f, 0.15f, 0.f};
	Bias.PostBoss = {0.3f, 0.45f, 0.25f, 0.f};
	Bias.TagWeightBonus = 0.1f;
	Bias.PityStreak = 5;
	Bias.PityAtLeast = ERarity::Rare;
	return Bias;
}

bool LoadTableFromJson(std::string_view Text, FTable& Out)
{
	FTable Local;
	Json Root;
	if (!ParseJson(Text, Root, Local.Error))
	{
		Out = std::move(Local);
		return false;
	}
	if (Root.Kind != Json::Type::Object)
	{
		Local.Error = "root is not an object";
		Out = std::move(Local);
		return false;
	}
	if (const Json* Version = Root.Find("contractVersion"))
	{
		if (Version->Kind != Json::Type::Number || std::fabs(Version->Number - 3.0) > 1e-6)
		{
			Local.Error = "contractVersion must be 3";
			Out = std::move(Local);
			return false;
		}
		Local.ContractVersion = 3;
	}
	if (!ReadString(Root, "poolId", Local.PoolId, false, Local.Error))
	{
		Out = std::move(Local);
		return false;
	}
	const Json* Affixes = Root.Find("affixes");
	if (!Affixes || Affixes->Kind != Json::Type::Array || Affixes->Array.empty())
	{
		Local.Error = "affixes empty";
		Out = std::move(Local);
		return false;
	}
	for (const Json& Item : Affixes->Array)
	{
		FAffixRow Row;
		if (!ReadAffixObject(Item, Row, Local.Error))
		{
			Out = std::move(Local);
			return false;
		}
		for (const FAffixRow& Existing : Local.Affixes)
		{
			if (Existing.AffixId == Row.AffixId)
			{
				Local.Error = "duplicate AffixId " + Row.AffixId;
				Out = std::move(Local);
				return false;
			}
		}
		Local.Affixes.push_back(std::move(Row));
	}
	Out = std::move(Local);
	return true;
}

bool LoadTableFromCsv(std::string_view Text, FTable& Out)
{
	FTable Local;
	Local.ContractVersion = 3;
	Local.PoolId = "P05";
	size_t Cursor = 0;
	if (Text.size() >= 3
		&& static_cast<unsigned char>(Text[0]) == 0xEF
		&& static_cast<unsigned char>(Text[1]) == 0xBB
		&& static_cast<unsigned char>(Text[2]) == 0xBF)
	{
		Cursor = 3;
	}
	bool bHeader = true;
	while (Cursor <= Text.size())
	{
		size_t End = Text.find('\n', Cursor);
		if (End == std::string_view::npos)
		{
			End = Text.size();
		}
		std::string_view Line = Text.substr(Cursor, End - Cursor);
		if (!Line.empty() && Line.back() == '\r')
		{
			Line.remove_suffix(1);
		}
		Cursor = End + 1;
		if (Line.empty())
		{
			if (End == Text.size())
			{
				break;
			}
			continue;
		}
		std::string SplitError;
		const std::vector<std::string> Fields = SplitCsvLine(Line, SplitError);
		if (!SplitError.empty())
		{
			Local.Error = SplitError;
			Out = std::move(Local);
			return false;
		}
		if (bHeader)
		{
			bHeader = false;
			const char* Expect[] = {
				"AffixId", "DisplayName", "Description", "Rarity", "StackRule", "MaxStacks", "Weight",
				"Tags", "ExclusiveTags", "Triggers", "Stat", "Op", "Value", "DurationSec", "DataAssetName"
			};
			if (Fields.size() != 15)
			{
				Local.Error = "csv header width";
				Out = std::move(Local);
				return false;
			}
			for (int I = 0; I < 15; ++I)
			{
				if (Fields[static_cast<size_t>(I)] != Expect[I])
				{
					Local.Error = "csv header mismatch";
					Out = std::move(Local);
					return false;
				}
			}
			if (End == Text.size())
			{
				break;
			}
			continue;
		}
		if (Fields.size() != 15)
		{
			Local.Error = "csv row width";
			Out = std::move(Local);
			return false;
		}
		FAffixRow* Row = nullptr;
		for (FAffixRow& Existing : Local.Affixes)
		{
			if (Existing.AffixId == Fields[0])
			{
				Row = &Existing;
				break;
			}
		}
		if (!Row)
		{
			FAffixRow Fresh;
			Fresh.AffixId = Fields[0];
			Fresh.DisplayName = Fields[1];
			Fresh.Description = Fields[2];
			if (!ParseRarity(Fields[3], Fresh.Rarity) || !ParseStack(Fields[4], Fresh.StackRule))
			{
				Local.Error = Fresh.AffixId + " bad rarity or stack";
				Out = std::move(Local);
				return false;
			}
			Fresh.MaxStacks = std::atoi(Fields[5].c_str());
			if (Fresh.MaxStacks < 1)
			{
				Fresh.MaxStacks = 1;
			}
			Fresh.Weight = static_cast<float>(std::strtod(Fields[6].c_str(), nullptr));
			SplitList(Fields[7], '|', Fresh.Tags);
			SplitList(Fields[8], '|', Fresh.ExclusiveTags);
			std::vector<std::string> TriggerNames;
			SplitList(Fields[9], '|', TriggerNames);
			for (const std::string& Name : TriggerNames)
			{
				ETrigger Trigger = ETrigger::Passive;
				if (!ParseTrigger(Name, Trigger))
				{
					Local.Error = Fresh.AffixId + " bad trigger";
					Out = std::move(Local);
					return false;
				}
				Fresh.Triggers.push_back(Trigger);
			}
			Fresh.DataAssetName = Fields[14].empty() ? (std::string("DA_Affix_") + Fresh.AffixId) : Fields[14];
			Local.Affixes.push_back(std::move(Fresh));
			Row = &Local.Affixes.back();
		}
		FMod Mod;
		if (!ParseStat(Fields[10], Mod.Stat) || !ParseOp(Fields[11], Mod.Op))
		{
			Local.Error = Fields[0] + " bad modifier";
			Out = std::move(Local);
			return false;
		}
		Mod.Value = static_cast<float>(std::strtod(Fields[12].c_str(), nullptr));
		Mod.DurationSec = static_cast<float>(std::strtod(Fields[13].c_str(), nullptr));
		Row->Modifiers.push_back(Mod);
		if (End == Text.size())
		{
			break;
		}
	}
	if (Local.Affixes.empty())
	{
		Local.Error = "csv has no affixes";
		Out = std::move(Local);
		return false;
	}
	Out = std::move(Local);
	return true;
}

bool LoadBiasFromJson(std::string_view Text, FBiasTable& Out, std::string* Error)
{
	Json Root;
	std::string LocalError;
	if (!ParseJson(Text, Root, LocalError))
	{
		if (Error)
		{
			*Error = LocalError;
		}
		return false;
	}
	if (Root.Kind != Json::Type::Object)
	{
		if (Error)
		{
			*Error = "bias root is not an object";
		}
		return false;
	}
	const Json* Rooms = Root.Find("AffixRarityBias");
	if (!Rooms || Rooms->Kind != Json::Type::Object)
	{
		if (Error)
		{
			*Error = "AffixRarityBias missing";
		}
		return false;
	}
	FBiasTable Bias = BuiltinBias();
	if (!ReadBiasRoom(*Rooms, "NormalRoom", Bias.NormalRoom, LocalError)
		|| !ReadBiasRoom(*Rooms, "EliteRoom", Bias.EliteRoom, LocalError)
		|| !ReadBiasRoom(*Rooms, "PostBoss", Bias.PostBoss, LocalError))
	{
		if (Error)
		{
			*Error = LocalError;
		}
		return false;
	}
	double Bonus = Bias.TagWeightBonus;
	if (!ReadNumber(Root, "TagWeightBonus", Bonus, false, LocalError))
	{
		if (Error)
		{
			*Error = LocalError;
		}
		return false;
	}
	Bias.TagWeightBonus = static_cast<float>(Bonus);
	if (const Json* Pity = Root.Find("Pity"))
	{
		if (Pity->Kind != Json::Type::Object)
		{
			if (Error)
			{
				*Error = "Pity not an object";
			}
			return false;
		}
		double Streak = Bias.PityStreak;
		if (!ReadNumber(*Pity, "StreakNoRarePlus", Streak, false, LocalError))
		{
			if (Error)
			{
				*Error = LocalError;
			}
			return false;
		}
		Bias.PityStreak = static_cast<int>(std::lround(Streak));
		std::string AtLeast;
		if (!ReadString(*Pity, "EnsureRarityAtLeast", AtLeast, false, LocalError))
		{
			if (Error)
			{
				*Error = LocalError;
			}
			return false;
		}
		if (!AtLeast.empty() && !ParseRarity(AtLeast, Bias.PityAtLeast))
		{
			if (Error)
			{
				*Error = "EnsureRarityAtLeast";
			}
			return false;
		}
	}
	if (const Json* Version = Root.Find("contractVersion"))
	{
		if (Version->Kind != Json::Type::Number || std::fabs(Version->Number - 3.0) > 1e-6)
		{
			if (Error)
			{
				*Error = "bias contractVersion must be 3";
			}
			return false;
		}
	}
	Out = Bias;
	return true;
}

FRunModel::FRunModel()
{
	UseBuiltinP05();
	BiasTable = BuiltinBias();
	ResetBases();
}

void FRunModel::UseBuiltinP05()
{
	Table.Affixes = BuiltinP05Rows();
	Table.ContractVersion = 3;
	Table.PoolId = "P05";
	Table.Error.clear();
	Source = ETableSource::Builtin;
}

bool FRunModel::LoadJson(std::string_view JsonText, std::string* Error)
{
	FTable Parsed;
	if (!LoadTableFromJson(JsonText, Parsed))
	{
		if (Error)
		{
			*Error = Parsed.Error;
		}
		return false;
	}
	Table = std::move(Parsed);
	Source = ETableSource::Json;
	return true;
}

bool FRunModel::LoadCsv(std::string_view Csv, std::string* Error)
{
	FTable Parsed;
	if (!LoadTableFromCsv(Csv, Parsed))
	{
		if (Error)
		{
			*Error = Parsed.Error;
		}
		return false;
	}
	Table = std::move(Parsed);
	Source = ETableSource::Csv;
	return true;
}

bool FRunModel::LoadBiasJson(std::string_view JsonText, std::string* Error)
{
	FBiasTable Parsed;
	if (!LoadBiasFromJson(JsonText, Parsed, Error))
	{
		return false;
	}
	BiasTable = Parsed;
	return true;
}

void FRunModel::SetBias(const FBiasTable& InBias)
{
	BiasTable = InBias;
}

void FRunModel::SetBase(EStat Stat, float Value)
{
	const int Index = static_cast<int>(Stat);
	if (Index < 0 || Index >= static_cast<int>(EStat::Count))
	{
		return;
	}
	Bases[Index] = Value;
}

float FRunModel::GetBase(EStat Stat) const
{
	const int Index = static_cast<int>(Stat);
	if (Index < 0 || Index >= static_cast<int>(EStat::Count))
	{
		return 0.f;
	}
	return Bases[Index];
}

void FRunModel::ResetBases()
{
	for (int Index = 0; Index < static_cast<int>(EStat::Count); ++Index)
	{
		Bases[Index] = DefaultBase(static_cast<EStat>(Index));
	}
}

void FRunModel::BeginRun(const FStart& Start)
{
	Inventory.clear();
	Temps.clear();
	PendingChoices.clear();
	Meta = Start.PermanentModifiers;
	Unlocked = Start.UnlockedAffixIds;
	WeaponId = Start.StartWeaponId;
	Scrap = Start.StartingScrap;
	StreakNoRarePlus = 0;
	Note.clear();
	Rng = Start.Seed == 0 ? 1u : Start.Seed;
	bInRun = true;
	LastSource = EOfferSource::NormalRoom;
}

void FRunModel::EndRun()
{
	Inventory.clear();
	Temps.clear();
	PendingChoices.clear();
	Meta.clear();
	Unlocked.clear();
	WeaponId.clear();
	Scrap = 0;
	StreakNoRarePlus = 0;
	Note.clear();
	bInRun = false;
}

bool FRunModel::RollChoices(int Count, int SeedOrNeg1, EOfferSource SourceKind)
{
	Note.clear();
	if (!bInRun)
	{
		Note = "BeginRun required";
		return false;
	}
	if (!PendingChoices.empty())
	{
		Note = "choice already pending";
		return false;
	}
	if (Count < 1)
	{
		Count = 1;
	}
	if (Count > 12)
	{
		Count = 12;
	}
	if (SeedOrNeg1 >= 0)
	{
		Rng = SeedOrNeg1 == 0 ? 1u : static_cast<uint32_t>(SeedOrNeg1);
	}
	LastSource = SourceKind;
	std::vector<std::string> Picked;
	while (static_cast<int>(PendingChoices.size()) < Count)
	{
		std::vector<std::pair<const FAffixRow*, float>> Cands;
		for (const FAffixRow& Row : Table.Affixes)
		{
			if (std::find(Picked.begin(), Picked.end(), Row.AffixId) != Picked.end())
			{
				continue;
			}
			const float Weight = EffectiveWeight(Row, SourceKind, false);
			if (Weight > 0.f)
			{
				Cands.emplace_back(&Row, Weight);
			}
		}
		const FAffixRow* Row = PickOne(Cands);
		if (!Row)
		{
			break;
		}
		FChoice Choice;
		Choice.AffixId = Row->AffixId;
		Choice.WeightUsed = EffectiveWeight(*Row, SourceKind, false);
		Choice.bWouldBeNew = GetStacks(Row->AffixId) == 0;
		Picked.push_back(Row->AffixId);
		PendingChoices.push_back(std::move(Choice));
	}
	if (static_cast<int>(PendingChoices.size()) < Count)
	{
		FillCommonFallback(Count);
	}
	ApplyPity();
	return static_cast<int>(PendingChoices.size()) == Count;
}

bool FRunModel::ConfirmAffixChoice(int Index, FChange* Out)
{
	if (!bInRun || PendingChoices.empty() || Index < 0 || Index >= static_cast<int>(PendingChoices.size()))
	{
		return false;
	}
	const FAffixRow* Row = Find(PendingChoices[static_cast<size_t>(Index)].AffixId);
	if (!Row)
	{
		return false;
	}
	FChange Change;
	if (!TryAdd(*Row, &Change))
	{
		return false;
	}
	NoteOfferResolved();
	PendingChoices.clear();
	if (Out)
	{
		*Out = Change;
	}
	return true;
}

bool FRunModel::SkipAffixChoice()
{
	if (!bInRun || PendingChoices.empty())
	{
		return false;
	}
	NoteOfferResolved();
	PendingChoices.clear();
	return true;
}

bool FRunModel::DebugGrant(const std::string& AffixId, FChange* Out)
{
	if (!bInRun)
	{
		return false;
	}
	const FAffixRow* Row = Find(AffixId);
	if (!Row)
	{
		return false;
	}
	return TryAdd(*Row, Out);
}

std::vector<std::string> FRunModel::NotifyTrigger(ETrigger Trigger)
{
	std::vector<std::string> Fired;
	if (!bInRun)
	{
		return Fired;
	}
	for (const FOwned& Owned : Inventory)
	{
		const FAffixRow* Row = Find(Owned.AffixId);
		if (!Row)
		{
			continue;
		}
		bool bListens = false;
		for (ETrigger Listed : Row->Triggers)
		{
			if (Listed == Trigger)
			{
				bListens = true;
				break;
			}
		}
		if (!bListens)
		{
			continue;
		}
		Fired.push_back(Row->AffixId);
		if (Row->StackRule == EStack::RefreshDuration)
		{
			Temps.erase(
				std::remove_if(Temps.begin(), Temps.end(), [&](const FTemp& Temp) { return Temp.AffixId == Row->AffixId; }),
				Temps.end());
		}
		for (const FMod& Mod : Row->Modifiers)
		{
			if (Mod.DurationSec > 0.f)
			{
				Temps.push_back(FTemp{Row->AffixId, Mod});
			}
		}
	}
	return Fired;
}

float FRunModel::GetStat(EStat Stat) const
{
	float Value = GetBase(Stat);
	Value = ApplyLayer(Value, Meta, Stat);
	std::vector<FMod> Passive;
	for (const FOwned& Owned : Inventory)
	{
		const FAffixRow* Row = Find(Owned.AffixId);
		if (!Row || !HasPassiveTrigger(*Row))
		{
			continue;
		}
		const int Stacks = Owned.Stacks > 0 ? Owned.Stacks : 0;
		for (int Stack = 0; Stack < Stacks; ++Stack)
		{
			for (const FMod& Mod : Row->Modifiers)
			{
				if (Mod.DurationSec <= 0.f)
				{
					Passive.push_back(Mod);
				}
			}
		}
	}
	Value = ApplyLayer(Value, Passive, Stat);
	std::vector<FMod> Timed;
	Timed.reserve(Temps.size());
	for (const FTemp& Temp : Temps)
	{
		Timed.push_back(Temp.Mod);
	}
	return ApplyLayer(Value, Timed, Stat);
}

int FRunModel::GetStacks(const std::string& AffixId) const
{
	if (const FOwned* Owned = FindOwned(AffixId))
	{
		return Owned->Stacks;
	}
	return 0;
}

bool FRunModel::HasTag(const std::string& Tag) const
{
	for (const FOwned& Owned : Inventory)
	{
		const FAffixRow* Row = Find(Owned.AffixId);
		if (!Row)
		{
			continue;
		}
		for (const std::string& OwnedTag : Row->Tags)
		{
			if (OwnedTag == Tag)
			{
				return true;
			}
		}
	}
	return false;
}

FCard FRunModel::GetCard(const std::string& AffixId, int PreviewStacks) const
{
	FCard Card;
	const FAffixRow* Row = Find(AffixId);
	if (!Row)
	{
		return Card;
	}
	Card.Name = Row->DisplayName;
	const float First = Row->Modifiers.empty() ? 0.f : Row->Modifiers[0].Value;
	Card.Desc = FormatDescription(Row->Description, First, PreviewStacks);
	Card.Rarity = RarityName(Row->Rarity);
	Card.Icon = Row->Icon;
	Card.AffixId = Row->AffixId;
	Card.Stacks = GetStacks(AffixId);
	Card.bOwned = Card.Stacks > 0;
	return Card;
}

const FAffixRow* FRunModel::Find(const std::string& AffixId) const
{
	for (const FAffixRow& Row : Table.Affixes)
	{
		if (Row.AffixId == AffixId)
		{
			return &Row;
		}
	}
	return nullptr;
}

float FRunModel::PreviewChoiceWeight(const std::string& AffixId, EOfferSource SourceKind) const
{
	const FAffixRow* Row = Find(AffixId);
	if (!Row)
	{
		return 0.f;
	}
	return EffectiveWeight(*Row, SourceKind, false);
}

float FRunModel::EffectiveWeight(const FAffixRow& Row, EOfferSource SourceKind, bool bIgnoreRarityBias) const
{
	if (!IsEligible(Row))
	{
		return 0.f;
	}
	const float Rarity = bIgnoreRarityBias ? 1.f : RarityFactor(Row.Rarity, SourceKind);
	if (!bIgnoreRarityBias && Rarity <= 0.f)
	{
		return 0.f;
	}
	const float Boost = 1.f + BiasTable.TagWeightBonus * static_cast<float>(TagOverlap(Row));
	return Row.Weight * Rarity * Boost;
}

bool FRunModel::IsEligible(const FAffixRow& Row) const
{
	if (Row.Weight <= 0.f || Row.AffixId.empty())
	{
		return false;
	}
	if (!Unlocked.empty())
	{
		bool bOpen = false;
		for (const std::string& Id : Unlocked)
		{
			if (Id == Row.AffixId)
			{
				bOpen = true;
				break;
			}
		}
		if (!bOpen)
		{
			return false;
		}
	}
	const int Stacks = GetStacks(Row.AffixId);
	if (Stacks >= Row.MaxStacks)
	{
		return false;
	}
	if (Row.StackRule == EStack::Unique && Stacks > 0)
	{
		return false;
	}
	if (ExclusiveConflict(Row))
	{
		return false;
	}
	return true;
}

bool FRunModel::ExclusiveConflict(const FAffixRow& Row) const
{
	if (Row.ExclusiveTags.empty())
	{
		return false;
	}
	for (const FOwned& Owned : Inventory)
	{
		const FAffixRow* Other = Find(Owned.AffixId);
		if (!Other || Other->AffixId == Row.AffixId)
		{
			continue;
		}
		for (const std::string& Tag : Row.ExclusiveTags)
		{
			for (const std::string& OtherTag : Other->ExclusiveTags)
			{
				if (Tag == OtherTag)
				{
					return true;
				}
			}
		}
	}
	return false;
}

int FRunModel::TagOverlap(const FAffixRow& Row) const
{
	std::vector<std::string> OwnedTags;
	for (const FOwned& Owned : Inventory)
	{
		const FAffixRow* Other = Find(Owned.AffixId);
		if (!Other)
		{
			continue;
		}
		for (const std::string& Tag : Other->Tags)
		{
			if (std::find(OwnedTags.begin(), OwnedTags.end(), Tag) == OwnedTags.end())
			{
				OwnedTags.push_back(Tag);
			}
		}
	}
	int Count = 0;
	std::vector<std::string> Seen;
	for (const std::string& Tag : Row.Tags)
	{
		if (Tag.empty() || std::find(Seen.begin(), Seen.end(), Tag) != Seen.end())
		{
			continue;
		}
		Seen.push_back(Tag);
		if (std::find(OwnedTags.begin(), OwnedTags.end(), Tag) != OwnedTags.end())
		{
			++Count;
		}
	}
	return Count;
}

bool FRunModel::TryAdd(const FAffixRow& Row, FChange* Out)
{
	FOwned* Existing = FindOwned(Row.AffixId);
	int Stacks = 1;
	if (Existing)
	{
		if (Row.StackRule == EStack::Unique || Existing->Stacks >= Row.MaxStacks)
		{
			return false;
		}
		if (Row.StackRule == EStack::AdditiveStack)
		{
			Existing->Stacks += 1;
		}
		else
		{
			Existing->Stacks = 1;
		}
		Stacks = Existing->Stacks;
	}
	else
	{
		Inventory.push_back(FOwned{Row.AffixId, 1});
	}
	PushDurationOnAcquire(Row);
	if (Out)
	{
		Out->AffixId = Row.AffixId;
		Out->Stacks = Stacks;
		Out->bRemoved = false;
	}
	return true;
}

void FRunModel::NoteOfferResolved()
{
	bool bRare = false;
	for (const FChoice& Choice : PendingChoices)
	{
		const FAffixRow* Row = Find(Choice.AffixId);
		if (Row && static_cast<int>(Row->Rarity) >= static_cast<int>(BiasTable.PityAtLeast))
		{
			bRare = true;
			break;
		}
	}
	StreakNoRarePlus = bRare ? 0 : (StreakNoRarePlus + 1);
}

void FRunModel::PushDurationOnAcquire(const FAffixRow& Row)
{
	if (!TriggersOnAcquire(Row))
	{
		return;
	}
	if (Row.StackRule == EStack::RefreshDuration)
	{
		Temps.erase(
			std::remove_if(Temps.begin(), Temps.end(), [&](const FTemp& Temp) { return Temp.AffixId == Row.AffixId; }),
			Temps.end());
	}
	for (const FMod& Mod : Row.Modifiers)
	{
		if (Mod.DurationSec > 0.f)
		{
			Temps.push_back(FTemp{Row.AffixId, Mod});
		}
	}
}

void FRunModel::FillCommonFallback(int Count)
{
	if (static_cast<int>(PendingChoices.size()) >= Count)
	{
		return;
	}
	Note = "pool short; repeated AdditiveStack Common";
	int Guard = 0;
	while (static_cast<int>(PendingChoices.size()) < Count && Guard++ < 16)
	{
		std::vector<std::pair<const FAffixRow*, float>> Cands;
		for (const FAffixRow& Row : Table.Affixes)
		{
			if (Row.StackRule != EStack::AdditiveStack || Row.Rarity != ERarity::Common)
			{
				continue;
			}
			if (!IsEligible(Row))
			{
				continue;
			}
			const float Boost = 1.f + BiasTable.TagWeightBonus * static_cast<float>(TagOverlap(Row));
			const float Weight = Row.Weight * Boost;
			if (Weight > 0.f)
			{
				Cands.emplace_back(&Row, Weight);
			}
		}
		const FAffixRow* Row = PickOne(Cands);
		if (!Row)
		{
			break;
		}
		FChoice Choice;
		Choice.AffixId = Row->AffixId;
		Choice.WeightUsed = Row->Weight;
		Choice.bWouldBeNew = GetStacks(Row->AffixId) == 0;
		PendingChoices.push_back(std::move(Choice));
	}
}

void FRunModel::ApplyPity()
{
	if (StreakNoRarePlus < BiasTable.PityStreak)
	{
		return;
	}
	for (const FChoice& Choice : PendingChoices)
	{
		const FAffixRow* Row = Find(Choice.AffixId);
		if (Row && static_cast<int>(Row->Rarity) >= static_cast<int>(BiasTable.PityAtLeast))
		{
			return;
		}
	}
	std::vector<std::string> Picked;
	for (const FChoice& Choice : PendingChoices)
	{
		Picked.push_back(Choice.AffixId);
	}
	std::vector<std::pair<const FAffixRow*, float>> Cands;
	for (const FAffixRow& Row : Table.Affixes)
	{
		if (static_cast<int>(Row.Rarity) < static_cast<int>(BiasTable.PityAtLeast))
		{
			continue;
		}
		if (std::find(Picked.begin(), Picked.end(), Row.AffixId) != Picked.end())
		{
			continue;
		}
		if (!IsEligible(Row))
		{
			continue;
		}
		const float Boost = 1.f + BiasTable.TagWeightBonus * static_cast<float>(TagOverlap(Row));
		const float Weight = Row.Weight * Boost;
		if (Weight > 0.f)
		{
			Cands.emplace_back(&Row, Weight);
		}
	}
	const FAffixRow* Row = PickOne(Cands);
	if (!Row)
	{
		Note = "pity could not find Rare+";
		return;
	}
	FChoice Injected;
	Injected.AffixId = Row->AffixId;
	Injected.WeightUsed = Row->Weight;
	Injected.bWouldBeNew = GetStacks(Row->AffixId) == 0;
	if (PendingChoices.empty())
	{
		PendingChoices.push_back(std::move(Injected));
	}
	else
	{
		PendingChoices.back() = std::move(Injected);
	}
	if (Note.empty())
	{
		Note = "pity injected Rare+";
	}
}

const FAffixRow* FRunModel::PickOne(const std::vector<std::pair<const FAffixRow*, float>>& Cands)
{
	float Total = 0.f;
	for (const auto& Cand : Cands)
	{
		Total += Cand.second;
	}
	if (Cands.empty() || Total <= 0.f)
	{
		return nullptr;
	}
	float Cursor = Rand01() * Total;
	for (const auto& Cand : Cands)
	{
		Cursor -= Cand.second;
		if (Cursor < 0.f)
		{
			return Cand.first;
		}
	}
	return Cands.back().first;
}

float FRunModel::RarityFactor(ERarity Rarity, EOfferSource SourceKind) const
{
	const FRarityWeights* Weights = &BiasTable.NormalRoom;
	if (SourceKind == EOfferSource::EliteRoom)
	{
		Weights = &BiasTable.EliteRoom;
	}
	else if (SourceKind == EOfferSource::PostBoss)
	{
		Weights = &BiasTable.PostBoss;
	}
	switch (Rarity)
	{
	case ERarity::Common:
		return Weights->Common;
	case ERarity::Rare:
		return Weights->Rare;
	case ERarity::Epic:
		return Weights->Epic;
	case ERarity::Legendary:
		return Weights->Legendary;
	}
	return 0.f;
}

float FRunModel::Rand01()
{
	return static_cast<float>(static_cast<double>(NextU32()) / 4294967296.0);
}

uint32_t FRunModel::NextU32()
{
	uint32_t X = Rng;
	X ^= X << 13;
	X ^= X >> 17;
	X ^= X << 5;
	if (X == 0)
	{
		X = 0xA3C59AC3u;
	}
	Rng = X;
	return X;
}

FOwned* FRunModel::FindOwned(const std::string& AffixId)
{
	for (FOwned& Owned : Inventory)
	{
		if (Owned.AffixId == AffixId)
		{
			return &Owned;
		}
	}
	return nullptr;
}

const FOwned* FRunModel::FindOwned(const std::string& AffixId) const
{
	for (const FOwned& Owned : Inventory)
	{
		if (Owned.AffixId == AffixId)
		{
			return &Owned;
		}
	}
	return nullptr;
}

} // namespace PixelSideAffix
