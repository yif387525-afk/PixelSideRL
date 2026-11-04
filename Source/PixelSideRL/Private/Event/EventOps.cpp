#include "Event/EventOps.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace PixelSideEvent
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
	std::string Text;
	std::vector<Json> Array;
	std::vector<std::pair<std::string, Json>> Fields;

	const Json* Find(std::string_view Key) const
	{
		if (Kind != Type::Object)
		{
			return nullptr;
		}
		for (const std::pair<std::string, Json>& Field : Fields)
		{
			if (Field.first == Key)
			{
				return &Field.second;
			}
		}
		return nullptr;
	}
};

class FParser
{
public:
	FParser(std::string_view InText, std::string& InError)
		: Text(InText)
		, Error(InError)
	{
	}

	bool Parse(Json& Out)
	{
		if (Text.size() >= 3
			&& static_cast<unsigned char>(Text[0]) == 0xEF
			&& static_cast<unsigned char>(Text[1]) == 0xBB
			&& static_cast<unsigned char>(Text[2]) == 0xBF)
		{
			Index = 3;
		}
		if (!ParseValue(Out))
		{
			return false;
		}
		Skip();
		if (Index != Text.size())
		{
			return Fail("trailing data");
		}
		return true;
	}

private:
	std::string_view Text;
	std::string& Error;
	size_t Index = 0;
	int Depth = 0;

	bool Fail(const char* Message)
	{
		Error = Message;
		return false;
	}

	void Skip()
	{
		while (Index < Text.size())
		{
			const unsigned char C = static_cast<unsigned char>(Text[Index]);
			if (C != ' ' && C != '\t' && C != '\n' && C != '\r')
			{
				break;
			}
			++Index;
		}
	}

	bool Consume(char Expected)
	{
		Skip();
		if (Index >= Text.size() || Text[Index] != Expected)
		{
			return false;
		}
		++Index;
		return true;
	}

	void AppendUtf8(std::string& Out, unsigned int Codepoint)
	{
		if (Codepoint <= 0x7Fu)
		{
			Out.push_back(static_cast<char>(Codepoint));
		}
		else if (Codepoint <= 0x7FFu)
		{
			Out.push_back(static_cast<char>(0xC0u | (Codepoint >> 6)));
			Out.push_back(static_cast<char>(0x80u | (Codepoint & 0x3Fu)));
		}
		else if (Codepoint <= 0xFFFFu)
		{
			Out.push_back(static_cast<char>(0xE0u | (Codepoint >> 12)));
			Out.push_back(static_cast<char>(0x80u | ((Codepoint >> 6) & 0x3Fu)));
			Out.push_back(static_cast<char>(0x80u | (Codepoint & 0x3Fu)));
		}
		else
		{
			Out.push_back(static_cast<char>(0xF0u | (Codepoint >> 18)));
			Out.push_back(static_cast<char>(0x80u | ((Codepoint >> 12) & 0x3Fu)));
			Out.push_back(static_cast<char>(0x80u | ((Codepoint >> 6) & 0x3Fu)));
			Out.push_back(static_cast<char>(0x80u | (Codepoint & 0x3Fu)));
		}
	}

	bool ReadHex4(unsigned int& Codepoint)
	{
		if (Index + 4 > Text.size())
		{
			return false;
		}
		Codepoint = 0;
		for (int Place = 0; Place < 4; ++Place)
		{
			const unsigned char Hex = static_cast<unsigned char>(Text[Index++]);
			Codepoint <<= 4;
			if (Hex >= '0' && Hex <= '9')
			{
				Codepoint += static_cast<unsigned int>(Hex - '0');
			}
			else if (Hex >= 'a' && Hex <= 'f')
			{
				Codepoint += static_cast<unsigned int>(Hex - 'a' + 10);
			}
			else if (Hex >= 'A' && Hex <= 'F')
			{
				Codepoint += static_cast<unsigned int>(Hex - 'A' + 10);
			}
			else
			{
				return false;
			}
		}
		return true;
	}

	bool ParseString(std::string& Out)
	{
		if (!Consume('"'))
		{
			return Fail("string");
		}
		Out.clear();
		while (Index < Text.size())
		{
			const unsigned char C = static_cast<unsigned char>(Text[Index]);
			if (C == '"')
			{
				++Index;
				return true;
			}
			if (C == '\\')
			{
				++Index;
				if (Index >= Text.size())
				{
					return Fail("escape");
				}
				const char Escaped = Text[Index++];
				switch (Escaped)
				{
				case '"':
				case '\\':
				case '/':
					Out.push_back(Escaped);
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
					unsigned int Codepoint = 0;
					if (!ReadHex4(Codepoint))
					{
						return Fail("unicode");
					}
					if (Codepoint >= 0xD800u && Codepoint <= 0xDBFFu)
					{
						if (Index + 2 > Text.size() || Text[Index] != '\\' || Text[Index + 1] != 'u')
						{
							return Fail("surrogate");
						}
						Index += 2;
						unsigned int Low = 0;
						if (!ReadHex4(Low) || Low < 0xDC00u || Low > 0xDFFFu)
						{
							return Fail("surrogate");
						}
						Codepoint = 0x10000u + ((Codepoint - 0xD800u) << 10) + (Low - 0xDC00u);
					}
					else if (Codepoint >= 0xDC00u && Codepoint <= 0xDFFFu)
					{
						return Fail("surrogate");
					}
					AppendUtf8(Out, Codepoint);
					break;
				}
				default:
					return Fail("escape");
				}
				continue;
			}
			if (C < 0x20u)
			{
				return Fail("control");
			}
			Out.push_back(static_cast<char>(C));
			++Index;
		}
		return Fail("string");
	}

	bool ParseNumber(Json& Out)
	{
		const size_t Start = Index;
		if (Index < Text.size() && Text[Index] == '-')
		{
			++Index;
		}
		if (Index >= Text.size() || Text[Index] < '0' || Text[Index] > '9')
		{
			return Fail("number");
		}
		if (Text[Index] == '0')
		{
			++Index;
		}
		else
		{
			while (Index < Text.size() && Text[Index] >= '0' && Text[Index] <= '9')
			{
				++Index;
			}
		}
		if (Index < Text.size() && Text[Index] == '.')
		{
			++Index;
			if (Index >= Text.size() || Text[Index] < '0' || Text[Index] > '9')
			{
				return Fail("number");
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
				return Fail("number");
			}
			while (Index < Text.size() && Text[Index] >= '0' && Text[Index] <= '9')
			{
				++Index;
			}
		}
		const std::string Token(Text.data() + Start, Index - Start);
		char* End = nullptr;
		Out.Number = std::strtod(Token.c_str(), &End);
		if (!End || End != Token.c_str() + Token.size() || !std::isfinite(Out.Number))
		{
			return Fail("number");
		}
		Out.Kind = Json::Type::Number;
		return true;
	}

	bool ParseValue(Json& Out)
	{
		if (Depth > 32)
		{
			return Fail("depth");
		}
		Skip();
		if (Index >= Text.size())
		{
			return Fail("eof");
		}
		const char C = Text[Index];
		if (C == '"')
		{
			Out.Kind = Json::Type::String;
			return ParseString(Out.Text);
		}
		if (C == '{')
		{
			return ParseObject(Out);
		}
		if (C == '[')
		{
			return ParseArray(Out);
		}
		if (C == 't')
		{
			if (Text.substr(Index, 4) != "true")
			{
				return Fail("bool");
			}
			Index += 4;
			Out.Kind = Json::Type::Bool;
			Out.Bool = true;
			return true;
		}
		if (C == 'f')
		{
			if (Text.substr(Index, 5) != "false")
			{
				return Fail("bool");
			}
			Index += 5;
			Out.Kind = Json::Type::Bool;
			Out.Bool = false;
			return true;
		}
		if (C == 'n')
		{
			if (Text.substr(Index, 4) != "null")
			{
				return Fail("null");
			}
			Index += 4;
			Out.Kind = Json::Type::Null;
			return true;
		}
		if (C == '-' || (C >= '0' && C <= '9'))
		{
			return ParseNumber(Out);
		}
		return Fail("value");
	}

	bool ParseObject(Json& Out)
	{
		if (!Consume('{'))
		{
			return Fail("object");
		}
		++Depth;
		Out.Kind = Json::Type::Object;
		Skip();
		if (Consume('}'))
		{
			--Depth;
			return true;
		}
		for (;;)
		{
			std::string Key;
			if (!ParseString(Key))
			{
				return false;
			}
			for (const std::pair<std::string, Json>& Field : Out.Fields)
			{
				if (Field.first == Key)
				{
					return Fail("duplicate key");
				}
			}
			if (!Consume(':'))
			{
				return Fail("colon");
			}
			Json Value;
			if (!ParseValue(Value))
			{
				return false;
			}
			Out.Fields.push_back(std::make_pair(std::move(Key), std::move(Value)));
			Skip();
			if (Consume('}'))
			{
				--Depth;
				return true;
			}
			if (!Consume(','))
			{
				return Fail("comma");
			}
		}
	}

	bool ParseArray(Json& Out)
	{
		if (!Consume('['))
		{
			return Fail("array");
		}
		++Depth;
		Out.Kind = Json::Type::Array;
		Skip();
		if (Consume(']'))
		{
			--Depth;
			return true;
		}
		for (;;)
		{
			Json Value;
			if (!ParseValue(Value))
			{
				return false;
			}
			Out.Array.push_back(std::move(Value));
			Skip();
			if (Consume(']'))
			{
				--Depth;
				return true;
			}
			if (!Consume(','))
			{
				return Fail("comma");
			}
		}
	}
};

bool Fail(std::string* Error, const std::string& Message)
{
	if (Error)
	{
		*Error = Message;
	}
	return false;
}

bool ParseRoot(std::string_view Text, Json& Out, std::string* Error)
{
	std::string Local;
	FParser Parser(Text, Local);
	if (!Parser.Parse(Out) || Out.Kind != Json::Type::Object)
	{
		return Fail(Error, Local.empty() ? "root" : Local);
	}
	return true;
}

bool ReadString(const Json& Obj, const char* Key, std::string& Out, bool bRequired, std::string& Error)
{
	const Json* Field = Obj.Find(Key);
	if (!Field)
	{
		if (!bRequired)
		{
			return true;
		}
		Error = std::string(Key) + " missing";
		return false;
	}
	if (Field->Kind != Json::Type::String)
	{
		Error = std::string(Key) + " not a string";
		return false;
	}
	Out = Field->Text;
	return true;
}

bool ReadBool(const Json& Obj, const char* Key, bool& Out, bool bRequired, std::string& Error)
{
	const Json* Field = Obj.Find(Key);
	if (!Field)
	{
		if (!bRequired)
		{
			return true;
		}
		Error = std::string(Key) + " missing";
		return false;
	}
	if (Field->Kind != Json::Type::Bool)
	{
		Error = std::string(Key) + " not a bool";
		return false;
	}
	Out = Field->Bool;
	return true;
}

bool ReadNumber(const Json& Obj, const char* Key, double& Out, bool bRequired, std::string& Error)
{
	const Json* Field = Obj.Find(Key);
	if (!Field)
	{
		if (!bRequired)
		{
			return true;
		}
		Error = std::string(Key) + " missing";
		return false;
	}
	if (Field->Kind != Json::Type::Number)
	{
		Error = std::string(Key) + " not a number";
		return false;
	}
	Out = Field->Number;
	return true;
}

bool IsEventId(const std::string& Id)
{
	if (Id.empty() || !std::isupper(static_cast<unsigned char>(Id[0])))
	{
		return false;
	}
	for (char C : Id)
	{
		if (!std::isalnum(static_cast<unsigned char>(C)))
		{
			return false;
		}
	}
	return true;
}

EOp KindFromName(const std::string& Name)
{
	if (Name == "GrantScrap")
	{
		return EOp::GrantScrap;
	}
	if (Name == "DamageSelf")
	{
		return EOp::DamageSelf;
	}
	if (Name == "HealSelf")
	{
		return EOp::HealSelf;
	}
	if (Name == "None")
	{
		return EOp::None;
	}
	return EOp::Unknown;
}

bool ReadEffect(const Json& Obj, FEffect& Effect, std::string& Error)
{
	if (Obj.Kind != Json::Type::Object)
	{
		Error = "effect";
		return false;
	}
	if (!ReadString(Obj, "Op", Effect.Op, true, Error) || Effect.Op.empty())
	{
		Error = Error.empty() ? "Op missing" : Error;
		return false;
	}
	Effect.Kind = KindFromName(Effect.Op);
	const Json* Value = Obj.Find("Value");
	if (Value)
	{
		if (Value->Kind != Json::Type::Number)
		{
			Error = "Value not a number";
			return false;
		}
		Effect.Value = Value->Number;
	}
	if (!ReadString(Obj, "Stat", Effect.Stat, false, Error))
	{
		return false;
	}
	return true;
}

bool ReadChoice(const Json& Obj, FChoice& Choice, std::string& Error)
{
	if (Obj.Kind != Json::Type::Object)
	{
		Error = "choice";
		return false;
	}
	if (!ReadString(Obj, "ChoiceId", Choice.ChoiceId, true, Error) || !IsEventId(Choice.ChoiceId))
	{
		Error = "ChoiceId";
		return false;
	}
	if (!ReadString(Obj, "Label", Choice.Label, true, Error) || Choice.Label.empty())
	{
		Error = "Label";
		return false;
	}
	if (!ReadString(Obj, "ResultToast", Choice.ResultToast, false, Error))
	{
		return false;
	}
	const Json* Effects = Obj.Find("Effects");
	if (!Effects || Effects->Kind != Json::Type::Array)
	{
		Error = "Effects";
		return false;
	}
	for (const Json& Item : Effects->Array)
	{
		FEffect Effect;
		if (!ReadEffect(Item, Effect, Error))
		{
			return false;
		}
		Choice.Effects.push_back(std::move(Effect));
	}
	return true;
}

bool ReadEvent(const Json& Root, FEventDefinition& Def, std::string& Error)
{
	if (!ReadString(Root, "EventId", Def.EventId, true, Error) || !IsEventId(Def.EventId))
	{
		Error = "EventId";
		return false;
	}
	if (!ReadString(Root, "DisplayName", Def.DisplayName, true, Error) || Def.DisplayName.empty()
		|| !ReadString(Root, "PromptBody", Def.PromptBody, true, Error) || Def.PromptBody.empty()
		|| !ReadString(Root, "SkipHint", Def.SkipHint, true, Error) || Def.SkipHint.empty())
	{
		if (Error.empty())
		{
			Error = "text";
		}
		return false;
	}
	if (!ReadBool(Root, "bAllowDoorSkip", Def.bAllowDoorSkip, false, Error)
		|| !ReadBool(Root, "bCombatAllowed", Def.bCombatAllowed, false, Error)
		|| !ReadBool(Root, "bAffixOnResolve", Def.bAffixOnResolve, false, Error))
	{
		return false;
	}
	if (!ReadString(Root, "BindRoomId", Def.BindRoomId, false, Error)
		|| !ReadString(Root, "InteractAnchor", Def.InteractAnchor, false, Error)
		|| !ReadString(Root, "DataAssetName", Def.DataAssetName, false, Error))
	{
		return false;
	}
	const std::string Frozen = FrozenDataAssetName(Def.EventId);
	if (Def.DataAssetName.empty())
	{
		Def.DataAssetName = Frozen;
	}
	else if (Def.DataAssetName != Frozen)
	{
		Error = "DataAssetName must be " + Frozen;
		return false;
	}
	double Version = 0.0;
	if (!ReadNumber(Root, "contractVersion", Version, true, Error) || std::fabs(Version - 3.0) > 1e-6)
	{
		Error = "contractVersion must be 3";
		return false;
	}
	Def.ContractVersion = 3;
	const Json* Choices = Root.Find("Choices");
	if (!Choices || Choices->Kind != Json::Type::Array || Choices->Array.empty() || Choices->Array.size() > 3)
	{
		Error = "Choices";
		return false;
	}
	for (const Json& Item : Choices->Array)
	{
		FChoice Choice;
		if (!ReadChoice(Item, Choice, Error))
		{
			return false;
		}
		for (const FChoice& Existing : Def.Choices)
		{
			if (Existing.ChoiceId == Choice.ChoiceId)
			{
				Error = "duplicate ChoiceId " + Choice.ChoiceId;
				return false;
			}
		}
		Def.Choices.push_back(std::move(Choice));
	}
	return true;
}

float PositiveAmount(double Value)
{
	if (!std::isfinite(Value) || Value <= 0.0)
	{
		return 0.f;
	}
	return static_cast<float>(Value);
}

int RoundedScrap(double Value)
{
	if (!std::isfinite(Value))
	{
		return 0;
	}
	const long long Rounded = std::llround(Value);
	if (Rounded >= static_cast<long long>(std::numeric_limits<int>::max()))
	{
		return std::numeric_limits<int>::max();
	}
	if (Rounded <= static_cast<long long>(std::numeric_limits<int>::min()))
	{
		return std::numeric_limits<int>::min();
	}
	return static_cast<int>(Rounded);
}

FOpStep ApplyEffect(const FEffect& Effect, FRunScrapWallet& Wallet, FEventVitals& Vitals)
{
	FOpStep Step;
	Step.Op = Effect.Op;
	Step.Kind = Effect.Kind;
	Step.ScrapBefore = Wallet.Scrap;
	Step.HpBefore = Vitals.CurrentHP;

	switch (Effect.Kind)
	{
	case EOp::GrantScrap:
		AddScrap(Wallet, RoundedScrap(Effect.Value));
		Step.bApplied = true;
		Step.Message = "GrantScrap";
		break;
	case EOp::DamageSelf:
		if (Vitals.CurrentHP <= 0.f)
		{
			Step.bApplied = true;
			Step.Message = "DamageSelf skipped (already dead)";
			break;
		}
		{
			const float Next = Vitals.CurrentHP - PositiveAmount(Effect.Value);
			if (Next < 1.f)
			{
				Vitals.CurrentHP = 1.f;
				Step.Message = "DamageSelf floored at 1";
			}
			else
			{
				Vitals.CurrentHP = Next;
				Step.Message = "DamageSelf";
			}
			Step.bApplied = true;
		}
		break;
	case EOp::HealSelf:
		if (Vitals.CurrentHP <= 0.f)
		{
			Step.bApplied = true;
			Step.Message = "HealSelf skipped (already dead)";
			break;
		}
		{
			float Next = Vitals.CurrentHP + PositiveAmount(Effect.Value);
			if (Vitals.MaxHP > 0.f && Next > Vitals.MaxHP)
			{
				Next = Vitals.MaxHP;
				Step.Message = "HealSelf clamped to MaxHP";
			}
			else
			{
				Step.Message = "HealSelf";
			}
			Vitals.CurrentHP = Next;
			Step.bApplied = true;
		}
		break;
	case EOp::None:
		Step.bApplied = true;
		Step.Message = "None";
		break;
	case EOp::Unknown:
		Step.bUnknown = true;
		Step.bApplied = false;
		Step.Message = "unknown Op '" + Effect.Op + "' ignored";
		break;
	}

	Step.ScrapAfter = Wallet.Scrap;
	Step.HpAfter = Vitals.CurrentHP;
	return Step;
}

}

std::string FrozenDataAssetName(const std::string& EventId)
{
	return std::string("DA_Event_") + EventId;
}

const char* OpName(EOp Kind)
{
	switch (Kind)
	{
	case EOp::GrantScrap: return "GrantScrap";
	case EOp::DamageSelf: return "DamageSelf";
	case EOp::HealSelf: return "HealSelf";
	case EOp::None: return "None";
	case EOp::Unknown: return "Unknown";
	}
	return "Unknown";
}

bool FCatalog::LoadEventJson(std::string_view JsonText, std::string* Error)
{
	Json Root;
	if (!ParseRoot(JsonText, Root, Error))
	{
		return false;
	}
	FEventDefinition Def;
	std::string Local;
	if (!ReadEvent(Root, Def, Local))
	{
		return Fail(Error, Local);
	}
	if (Find(Def.EventId))
	{
		return Fail(Error, "duplicate EventId " + Def.EventId);
	}
	Definitions.push_back(std::move(Def));
	return true;
}

bool FCatalog::LoadPoolJson(std::string_view JsonText, std::string* Error)
{
	Json Root;
	if (!ParseRoot(JsonText, Root, Error))
	{
		return false;
	}
	double Version = 0.0;
	std::string Local;
	if (!ReadNumber(Root, "contractVersion", Version, true, Local) || std::fabs(Version - 3.0) > 1e-6)
	{
		return Fail(Error, "contractVersion must be 3");
	}
	std::string Rule;
	if (!ReadString(Root, "daNamingRule", Rule, true, Local) || Rule != "DA_Event_<EventId>")
	{
		return Fail(Error, "daNamingRule must be DA_Event_<EventId>");
	}
	const Json* Events = Root.Find("events");
	if (!Events || Events->Kind != Json::Type::Array || Events->Array.empty())
	{
		return Fail(Error, "events empty");
	}
	std::vector<FPoolEntry> Loaded;
	for (const Json& Item : Events->Array)
	{
		if (Item.Kind != Json::Type::Object)
		{
			return Fail(Error, "pool event");
		}
		FPoolEntry Entry;
		if (!ReadString(Item, "EventId", Entry.EventId, true, Local) || !IsEventId(Entry.EventId))
		{
			return Fail(Error, "pool EventId");
		}
		if (!ReadString(Item, "DataAssetName", Entry.DataAssetName, true, Local))
		{
			return Fail(Error, Local);
		}
		if (Entry.DataAssetName != FrozenDataAssetName(Entry.EventId))
		{
			return Fail(Error, "pool DataAssetName must be " + FrozenDataAssetName(Entry.EventId));
		}
		if (!ReadString(Item, "BindRoomId", Entry.BindRoomId, false, Local))
		{
			return Fail(Error, Local);
		}
		if (!ReadNumber(Item, "Weight", Entry.Weight, false, Local))
		{
			return Fail(Error, Local);
		}
		for (const FPoolEntry& Existing : Loaded)
		{
			if (Existing.EventId == Entry.EventId)
			{
				return Fail(Error, "duplicate pool EventId " + Entry.EventId);
			}
		}
		Loaded.push_back(std::move(Entry));
	}
	PoolEntries = std::move(Loaded);
	NamingRule = std::move(Rule);
	return true;
}

bool FCatalog::ValidatePool(std::string* Error) const
{
	if (NamingRule != "DA_Event_<EventId>")
	{
		return Fail(Error, "daNamingRule");
	}
	if (PoolEntries.empty())
	{
		return Fail(Error, "event pool empty");
	}
	for (const FPoolEntry& Entry : PoolEntries)
	{
		const std::string Frozen = FrozenDataAssetName(Entry.EventId);
		if (Entry.DataAssetName != Frozen)
		{
			return Fail(Error, "pool DataAssetName " + Entry.DataAssetName);
		}
		const FEventDefinition* Def = Find(Entry.EventId);
		if (!Def)
		{
			return Fail(Error, "pool event not loaded " + Entry.EventId);
		}
		if (Def->DataAssetName != Frozen)
		{
			return Fail(Error, "loaded DataAssetName " + Def->DataAssetName);
		}
	}
	return true;
}

const FEventDefinition* FCatalog::Find(const std::string& EventId) const
{
	for (const FEventDefinition& Def : Definitions)
	{
		if (Def.EventId == EventId)
		{
			return &Def;
		}
	}
	return nullptr;
}

FResolveResult FCatalog::Resolve(const std::string& EventId, const std::string& ChoiceId, FRunScrapWallet& Wallet, FEventVitals& Vitals) const
{
	FResolveResult Result;
	Result.EventId = EventId;
	Result.ChoiceId = ChoiceId;
	Result.Scrap = Wallet.Scrap;
	Result.CurrentHP = Vitals.CurrentHP;
	Result.MaxHP = Vitals.MaxHP;
	Result.bAffixChoiceRequested = false;
	Result.bCombatStarted = false;

	const FEventDefinition* Def = Find(EventId);
	if (!Def)
	{
		Result.Error = "unknown EventId " + EventId;
		return Result;
	}
	Result.bAffixOnResolve = Def->bAffixOnResolve;
	Result.bCombatAllowed = Def->bCombatAllowed;
	if (Def->bCombatAllowed)
	{
		Result.Notes.push_back("bCombatAllowed is true; MVP does not start combat");
	}
	if (Def->bAffixOnResolve)
	{
		Result.Notes.push_back("bAffixOnResolve is true; MVP does not request AffixChoice");
	}

	const FChoice* Choice = nullptr;
	for (const FChoice& Item : Def->Choices)
	{
		if (Item.ChoiceId == ChoiceId)
		{
			Choice = &Item;
			break;
		}
	}
	if (!Choice)
	{
		Result.Error = "unknown ChoiceId " + ChoiceId;
		return Result;
	}

	Result.ResultToast = Choice->ResultToast;
	for (const FEffect& Effect : Choice->Effects)
	{
		Result.Steps.push_back(ApplyEffect(Effect, Wallet, Vitals));
	}
	Result.bOk = true;
	Result.Scrap = Wallet.Scrap;
	Result.CurrentHP = Vitals.CurrentHP;
	Result.MaxHP = Vitals.MaxHP;
	return Result;
}

}
