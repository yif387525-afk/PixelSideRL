#include "Meta/MetaProgress.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <utility>

namespace PixelSideMeta
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
	std::vector<Json> Items;
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
		const std::string Token(Text.substr(Start, Index - Start));
		char* End = nullptr;
		Out.Number = std::strtod(Token.c_str(), &End);
		if (!End || End != Token.c_str() + Token.size() || !std::isfinite(Out.Number))
		{
			return Fail("number");
		}
		Out.Kind = Json::Type::Number;
		return true;
	}

	bool ParseArray(Json& Out);
	bool ParseObject(Json& Out);

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
};

bool FParser::ParseArray(Json& Out)
{
	if (!Consume('['))
	{
		return Fail("array");
	}
	Out = Json();
	Out.Kind = Json::Type::Array;
	++Depth;
	Skip();
	if (Index < Text.size() && Text[Index] == ']')
	{
		++Index;
		--Depth;
		return true;
	}
	while (true)
	{
		Json Value;
		if (!ParseValue(Value))
		{
			return false;
		}
		Out.Items.push_back(std::move(Value));
		Skip();
		if (Index < Text.size() && Text[Index] == ']')
		{
			++Index;
			--Depth;
			return true;
		}
		if (!Consume(','))
		{
			return Fail("array");
		}
	}
}

bool FParser::ParseObject(Json& Out)
{
	if (!Consume('{'))
	{
		return Fail("object");
	}
	Out = Json();
	Out.Kind = Json::Type::Object;
	++Depth;
	Skip();
	if (Index < Text.size() && Text[Index] == '}')
	{
		++Index;
		--Depth;
		return true;
	}
	while (true)
	{
		Skip();
		std::string Key;
		if (!ParseString(Key))
		{
			return false;
		}
		if (Out.Find(Key))
		{
			return Fail("duplicate key");
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
		if (Index < Text.size() && Text[Index] == '}')
		{
			++Index;
			--Depth;
			return true;
		}
		if (!Consume(','))
		{
			return Fail("object");
		}
	}
}

bool ParseJson(std::string_view Text, Json& Out, std::string* Error)
{
	std::string Local;
	std::string& Sink = Error ? *Error : Local;
	Sink.clear();
	FParser Parser(Text, Sink);
	if (!Parser.Parse(Out))
	{
		if (Sink.empty())
		{
			Sink = "json";
		}
		return false;
	}
	return true;
}

void SetError(std::string* Error, const std::string& Message)
{
	if (Error)
	{
		*Error = Message;
	}
}

bool RequireString(const Json& Obj, const char* Key, std::string& Out, std::string* Error)
{
	const Json* Field = Obj.Find(Key);
	if (!Field || Field->Kind != Json::Type::String)
	{
		SetError(Error, std::string("missing ") + Key);
		return false;
	}
	Out = Field->Text;
	return true;
}

bool RequireBool(const Json& Obj, const char* Key, bool& Out, std::string* Error)
{
	const Json* Field = Obj.Find(Key);
	if (!Field || Field->Kind != Json::Type::Bool)
	{
		SetError(Error, std::string("missing ") + Key);
		return false;
	}
	Out = Field->Bool;
	return true;
}

bool RequireNumber(const Json& Obj, const char* Key, double& Out, std::string* Error)
{
	const Json* Field = Obj.Find(Key);
	if (!Field || Field->Kind != Json::Type::Number || !std::isfinite(Field->Number))
	{
		SetError(Error, std::string("missing ") + Key);
		return false;
	}
	Out = Field->Number;
	return true;
}

bool AsInt(double Number, int& Out)
{
	if (!std::isfinite(Number))
	{
		return false;
	}
	const double Rounded = std::round(Number);
	if (std::fabs(Number - Rounded) > 1e-6)
	{
		return false;
	}
	if (Rounded > static_cast<double>(std::numeric_limits<int>::max())
		|| Rounded < static_cast<double>(std::numeric_limits<int>::min()))
	{
		return false;
	}
	Out = static_cast<int>(Rounded);
	return true;
}

bool RequireInt(const Json& Obj, const char* Key, int& Out, std::string* Error)
{
	double Number = 0.0;
	if (!RequireNumber(Obj, Key, Number, Error) || !AsInt(Number, Out))
	{
		SetError(Error, std::string("bad int ") + Key);
		return false;
	}
	return true;
}

bool OptionalStringArray(const Json& Obj, const char* Key, std::vector<std::string>& Out, std::string* Error)
{
	Out.clear();
	const Json* Field = Obj.Find(Key);
	if (!Field)
	{
		return true;
	}
	if (Field->Kind != Json::Type::Array)
	{
		SetError(Error, std::string("bad array ") + Key);
		return false;
	}
	for (const Json& Item : Field->Items)
	{
		if (Item.Kind != Json::Type::String || Item.Text.empty())
		{
			SetError(Error, std::string("bad string in ") + Key);
			return false;
		}
		Out.push_back(Item.Text);
	}
	return true;
}

bool IsKnownStat(const std::string& Stat)
{
	return Stat == "MaxHP" || Stat == "MoveSpeed" || Stat == "FireRate" || Stat == "Damage"
		|| Stat == "CritChance" || Stat == "CritMult" || Stat == "MagSize" || Stat == "ReloadSpeed"
		|| Stat == "ProjectileCount" || Stat == "Pierce" || Stat == "Knockback" || Stat == "PickupRadius"
		|| Stat == "CoinGain" || Stat == "ScrapGain";
}

bool IsForbiddenMetaStat(const std::string& Stat)
{
	return Stat == "CritChance" || Stat == "Pierce" || Stat == "ProjectileCount";
}

bool IsKnownOp(const std::string& Op)
{
	return Op == "Add" || Op == "Mul" || Op == "Override";
}

bool ParseModifier(const Json& Obj, FModifier& Out, std::string* Error)
{
	if (Obj.Kind != Json::Type::Object)
	{
		SetError(Error, "modifier");
		return false;
	}
	if (!RequireString(Obj, "Stat", Out.Stat, Error) || !RequireString(Obj, "Op", Out.Op, Error))
	{
		return false;
	}
	if (!IsKnownStat(Out.Stat) || !IsKnownOp(Out.Op))
	{
		SetError(Error, "bad modifier " + Out.Stat + " " + Out.Op);
		return false;
	}
	if (IsForbiddenMetaStat(Out.Stat))
	{
		SetError(Error, "forbidden stat " + Out.Stat);
		return false;
	}
	double Value = 0.0;
	if (!RequireNumber(Obj, "Value", Value, Error))
	{
		return false;
	}
	Out.Value = Value;
	Out.WeaponTag.clear();
	if (const Json* Tag = Obj.Find("WeaponTag"))
	{
		if (Tag->Kind != Json::Type::String || Tag->Text.empty())
		{
			SetError(Error, "bad WeaponTag");
			return false;
		}
		Out.WeaponTag = Tag->Text;
	}
	return true;
}

bool ParseModifierList(const Json& Arr, std::vector<FModifier>& Out, std::string* Error)
{
	Out.clear();
	if (Arr.Kind != Json::Type::Array)
	{
		SetError(Error, "modifier list");
		return false;
	}
	for (const Json& Item : Arr.Items)
	{
		FModifier Mod;
		if (!ParseModifier(Item, Mod, Error))
		{
			return false;
		}
		Out.push_back(std::move(Mod));
	}
	return true;
}

std::string FormatNum(double Value)
{
	char Buf[64];
	std::snprintf(Buf, sizeof(Buf), "%.4f", Value);
	std::string Text(Buf);
	const size_t Dot = Text.find('.');
	if (Dot != std::string::npos)
	{
		while (!Text.empty() && Text.back() == '0')
		{
			Text.pop_back();
		}
		if (!Text.empty() && Text.back() == '.')
		{
			Text.pop_back();
		}
	}
	return Text;
}

const int* FindRank(const std::vector<std::pair<std::string, int>>& Purchased, const std::string& NodeId)
{
	for (const std::pair<std::string, int>& Row : Purchased)
	{
		if (Row.first == NodeId)
		{
			return &Row.second;
		}
	}
	return nullptr;
}

int* FindRankMut(std::vector<std::pair<std::string, int>>& Purchased, const std::string& NodeId)
{
	for (std::pair<std::string, int>& Row : Purchased)
	{
		if (Row.first == NodeId)
		{
			return &Row.second;
		}
	}
	return nullptr;
}

} // namespace

bool FProgress::LoadEconomyJson(std::string_view JsonText, std::string* Error)
{
	Json Root;
	if (!ParseJson(JsonText, Root, Error))
	{
		return false;
	}
	FEconomy Next;
	if (!RequireInt(Root, "contractVersion", Next.ContractVersion, Error) || Next.ContractVersion != 3)
	{
		SetError(Error, "economy contractVersion");
		return false;
	}
	if (!RequireBool(Root, "locked", Next.bLocked, Error) || !Next.bLocked)
	{
		SetError(Error, "economy locked");
		return false;
	}
	if (!RequireString(Root, "RunCurrencyId", Next.RunCurrencyId, Error)
		|| !RequireString(Root, "MetaCurrencyId", Next.MetaCurrencyId, Error)
		|| Next.RunCurrencyId != "Scrap"
		|| Next.MetaCurrencyId != "Core")
	{
		SetError(Error, "economy currency");
		return false;
	}
	double Base = 0.0;
	double PerRoom = 0.0;
	double Boss = 0.0;
	double Cap = 0.0;
	if (!RequireNumber(Root, "BaseClearBonus", Base, Error)
		|| !RequireNumber(Root, "DeathMult", Next.DeathMult, Error)
		|| !RequireNumber(Root, "PerRoom", PerRoom, Error)
		|| !RequireNumber(Root, "BossBonus", Boss, Error)
		|| !RequireNumber(Root, "CoinToScrap", Next.CoinToScrap, Error)
		|| !RequireNumber(Root, "ScrapToCore", Next.ScrapToCore, Error)
		|| !RequireNumber(Root, "MetaCoreMult", Next.MetaCoreMult, Error)
		|| !RequireNumber(Root, "SoftCapPerRun", Cap, Error))
	{
		return false;
	}
	if (!AsInt(Base, Next.BaseClearBonus) || !AsInt(PerRoom, Next.PerRoom) || !AsInt(Boss, Next.BossBonus) || !AsInt(Cap, Next.SoftCapPerRun)
		|| Next.BaseClearBonus < 0 || Next.PerRoom < 0 || Next.BossBonus < 0 || Next.SoftCapPerRun <= 0
		|| Next.DeathMult < 0.0 || Next.CoinToScrap < 0.0 || Next.ScrapToCore < 0.0 || Next.MetaCoreMult < 0.0)
	{
		SetError(Error, "economy range");
		return false;
	}
	Econ = Next;
	bEconomy = true;
	SetError(Error, "");
	return true;
}

bool FProgress::LoadNodesJson(std::string_view JsonText, std::string* Error)
{
	Json Root;
	if (!ParseJson(JsonText, Root, Error))
	{
		return false;
	}
	int Version = 0;
	bool bLocked = false;
	std::string Rule;
	std::string NextPolicy;
	if (!RequireInt(Root, "contractVersion", Version, Error) || Version != 3)
	{
		SetError(Error, "nodes contractVersion");
		return false;
	}
	if (!RequireBool(Root, "locked", bLocked, Error) || !bLocked)
	{
		SetError(Error, "nodes locked");
		return false;
	}
	if (!RequireString(Root, "daNamingRule", Rule, Error) || Rule != kNamingRule)
	{
		SetError(Error, "daNamingRule");
		return false;
	}
	if (!RequireString(Root, "mvpAffixPoolPolicy", NextPolicy, Error) || NextPolicy != kMvpAffixPoolPolicy)
	{
		SetError(Error, "mvpAffixPoolPolicy");
		return false;
	}
	const Json* Nodes = Root.Find("nodes");
	if (!Nodes || Nodes->Kind != Json::Type::Array || Nodes->Items.empty())
	{
		SetError(Error, "nodes");
		return false;
	}
	std::vector<FNode> Parsed;
	for (const Json& Item : Nodes->Items)
	{
		if (Item.Kind != Json::Type::Object)
		{
			SetError(Error, "node");
			return false;
		}
		FNode Node;
		if (!RequireString(Item, "NodeId", Node.NodeId, Error)
			|| !RequireString(Item, "DataAssetName", Node.DataAssetName, Error)
			|| !RequireInt(Item, "MaxPurchases", Node.MaxPurchases, Error))
		{
			return false;
		}
		if (Node.DataAssetName != FrozenDataAssetName(Node.NodeId))
		{
			SetError(Error, "DataAssetName " + Node.NodeId);
			return false;
		}
		if (Node.MaxPurchases < 1 || Node.MaxPurchases > 32)
		{
			SetError(Error, "MaxPurchases " + Node.NodeId);
			return false;
		}
		for (const FNode& Existing : Parsed)
		{
			if (Existing.NodeId == Node.NodeId)
			{
				SetError(Error, "duplicate " + Node.NodeId);
				return false;
			}
		}
		const Json* Display = Item.Find("DisplayName");
		const Json* Desc = Item.Find("Description");
		if (Display)
		{
			if (Display->Kind != Json::Type::String)
			{
				SetError(Error, "DisplayName");
				return false;
			}
			Node.DisplayName = Display->Text;
		}
		if (Desc)
		{
			if (Desc->Kind != Json::Type::String)
			{
				SetError(Error, "Description");
				return false;
			}
			Node.Description = Desc->Text;
		}
		const Json* Weapon = Item.Find("WeaponId");
		if (Weapon)
		{
			if (Weapon->Kind != Json::Type::String || Weapon->Text.empty())
			{
				SetError(Error, "WeaponId");
				return false;
			}
			Node.WeaponId = Weapon->Text;
		}
		if (!OptionalStringArray(Item, "Prerequisites", Node.Prerequisites, Error)
			|| !OptionalStringArray(Item, "UnlockAffixIds", Node.UnlockAffixIds, Error))
		{
			return false;
		}
		const Json* Costs = Item.Find("CostCoreByRank");
		if (!Costs || Costs->Kind != Json::Type::Array || static_cast<int>(Costs->Items.size()) != Node.MaxPurchases)
		{
			SetError(Error, "CostCoreByRank " + Node.NodeId);
			return false;
		}
		for (const Json& Cost : Costs->Items)
		{
			int Value = 0;
			if (Cost.Kind != Json::Type::Number || !AsInt(Cost.Number, Value) || Value < 0)
			{
				SetError(Error, "CostCoreByRank " + Node.NodeId);
				return false;
			}
			Node.CostCoreByRank.push_back(Value);
		}
		if (const Json* Mods = Item.Find("PermanentModifiersByRank"))
		{
			if (Mods->Kind != Json::Type::Array || static_cast<int>(Mods->Items.size()) != Node.MaxPurchases)
			{
				SetError(Error, "PermanentModifiersByRank " + Node.NodeId);
				return false;
			}
			for (const Json& RankMods : Mods->Items)
			{
				std::vector<FModifier> List;
				if (!ParseModifierList(RankMods, List, Error))
				{
					SetError(Error, Error && !Error->empty() ? *Error : "PermanentModifiersByRank " + Node.NodeId);
					return false;
				}
				Node.PermanentModifiersByRank.push_back(std::move(List));
			}
		}
		if (const Json* Scrap = Item.Find("StartingScrapByRank"))
		{
			if (Scrap->Kind != Json::Type::Array || static_cast<int>(Scrap->Items.size()) != Node.MaxPurchases)
			{
				SetError(Error, "StartingScrapByRank " + Node.NodeId);
				return false;
			}
			for (const Json& RankScrap : Scrap->Items)
			{
				int Value = 0;
				if (RankScrap.Kind != Json::Type::Number || !AsInt(RankScrap.Number, Value) || Value < 0)
				{
					SetError(Error, "StartingScrapByRank " + Node.NodeId);
					return false;
				}
				Node.StartingScrapByRank.push_back(Value);
			}
		}
		if (const Json* Key = Item.Find("EconomyKey"))
		{
			if (Key->Kind != Json::Type::String || Key->Text.empty())
			{
				SetError(Error, "EconomyKey");
				return false;
			}
			if (Key->Text != "MetaCoreMult")
			{
				SetError(Error, "EconomyKey " + Key->Text);
				return false;
			}
			Node.EconomyKey = Key->Text;
		}
		if (const Json* Delta = Item.Find("EconomyDeltaByRank"))
		{
			if (Node.EconomyKey.empty())
			{
				SetError(Error, "EconomyDelta without key");
				return false;
			}
			if (Delta->Kind != Json::Type::Array || static_cast<int>(Delta->Items.size()) != Node.MaxPurchases)
			{
				SetError(Error, "EconomyDeltaByRank " + Node.NodeId);
				return false;
			}
			for (const Json& RankDelta : Delta->Items)
			{
				if (RankDelta.Kind != Json::Type::Number || !std::isfinite(RankDelta.Number))
				{
					SetError(Error, "EconomyDeltaByRank " + Node.NodeId);
					return false;
				}
				Node.EconomyDeltaByRank.push_back(RankDelta.Number);
			}
		}
		else if (!Node.EconomyKey.empty())
		{
			SetError(Error, "EconomyDeltaByRank " + Node.NodeId);
			return false;
		}
		if (const Json* Flags = Item.Find("FlagsByRank"))
		{
			if (Flags->Kind != Json::Type::Array || static_cast<int>(Flags->Items.size()) != Node.MaxPurchases)
			{
				SetError(Error, "FlagsByRank " + Node.NodeId);
				return false;
			}
			for (const Json& RankFlags : Flags->Items)
			{
				if (RankFlags.Kind != Json::Type::Array)
				{
					SetError(Error, "FlagsByRank " + Node.NodeId);
					return false;
				}
				std::vector<std::string> List;
				for (const Json& Flag : RankFlags.Items)
				{
					if (Flag.Kind != Json::Type::String || Flag.Text.empty())
					{
						SetError(Error, "FlagsByRank " + Node.NodeId);
						return false;
					}
					List.push_back(Flag.Text);
				}
				Node.FlagsByRank.push_back(std::move(List));
			}
		}
		Parsed.push_back(std::move(Node));
	}
	NodeRows = std::move(Parsed);
	Naming = Rule;
	Policy = NextPolicy;
	bNodes = true;
	SetError(Error, "");
	return true;
}

bool FProgress::LoadAffixPoolJson(std::string_view JsonText, std::string* Error)
{
	Json Root;
	if (!ParseJson(JsonText, Root, Error))
	{
		return false;
	}
	int Version = 0;
	if (!RequireInt(Root, "contractVersion", Version, Error) || Version != 3)
	{
		SetError(Error, "affix contractVersion");
		return false;
	}
	const Json* Affixes = Root.Find("affixes");
	if (!Affixes || Affixes->Kind != Json::Type::Array || Affixes->Items.size() != kMvpAffixIds.size())
	{
		SetError(Error, "affix pool size");
		return false;
	}
	std::vector<std::string> Next;
	Next.reserve(kMvpAffixIds.size());
	for (size_t Index = 0; Index < Affixes->Items.size(); ++Index)
	{
		const Json& Item = Affixes->Items[Index];
		if (Item.Kind != Json::Type::Object)
		{
			SetError(Error, "affix row");
			return false;
		}
		std::string Id;
		if (!RequireString(Item, "AffixId", Id, Error))
		{
			return false;
		}
		if (Id != kMvpAffixIds[Index])
		{
			SetError(Error, "affix id " + Id);
			return false;
		}
		if (const Json* Asset = Item.Find("DataAssetName"))
		{
			if (Asset->Kind != Json::Type::String || Asset->Text != "DA_Affix_" + Id)
			{
				SetError(Error, "DA_Affix_" + Id);
				return false;
			}
		}
		Next.push_back(Id);
	}
	Pool = std::move(Next);
	bPool = true;
	SetError(Error, "");
	return true;
}

const FNode* FProgress::FindNode(const std::string& NodeId) const
{
	for (const FNode& Node : NodeRows)
	{
		if (Node.NodeId == NodeId)
		{
			return &Node;
		}
	}
	return nullptr;
}

void FProgress::AddCore(int Delta, const std::string& Reason)
{
	CoreReason = Reason;
	const long long Next = static_cast<long long>(Core) + static_cast<long long>(Delta);
	if (Next <= 0)
	{
		Core = 0;
		return;
	}
	if (Next >= static_cast<long long>(std::numeric_limits<int>::max()))
	{
		Core = std::numeric_limits<int>::max();
		return;
	}
	Core = static_cast<int>(Next);
}

int FProgress::GetPurchasedRank(const std::string& NodeId) const
{
	const int* Rank = FindRank(Purchased, NodeId);
	return Rank ? *Rank : 0;
}

int FProgress::EffectiveRank(const FNode& Node) const
{
	int Rank = GetPurchasedRank(Node.NodeId);
	if (Rank <= 0)
	{
		return 0;
	}
	if (Node.MaxPurchases > 0 && Rank > Node.MaxPurchases)
	{
		Rank = Node.MaxPurchases;
	}
	return Rank;
}

bool FProgress::CanPurchase(const std::string& NodeId, std::string* Reason) const
{
	if (!bNodes)
	{
		SetError(Reason, "catalog");
		return false;
	}
	const FNode* Node = FindNode(NodeId);
	if (!Node)
	{
		SetError(Reason, "unknown");
		return false;
	}
	const int Rank = GetPurchasedRank(NodeId);
	if (Rank >= Node->MaxPurchases || Rank < 0 || Rank >= static_cast<int>(Node->CostCoreByRank.size()))
	{
		SetError(Reason, "max");
		return false;
	}
	for (const std::string& Prereq : Node->Prerequisites)
	{
		if (GetPurchasedRank(Prereq) < 1)
		{
			SetError(Reason, "prereq:" + Prereq);
			return false;
		}
	}
	if (Core < Node->CostCoreByRank[static_cast<size_t>(Rank)])
	{
		SetError(Reason, "core");
		return false;
	}
	SetError(Reason, "");
	return true;
}

bool FProgress::Purchase(const std::string& NodeId, std::string* Reason)
{
	if (!CanPurchase(NodeId, Reason))
	{
		return false;
	}
	const FNode* Node = FindNode(NodeId);
	if (!Node)
	{
		SetError(Reason, "unknown");
		return false;
	}
	const int Rank = GetPurchasedRank(NodeId);
	const int Cost = Node->CostCoreByRank[static_cast<size_t>(Rank)];
	Core -= Cost;
	if (int* Slot = FindRankMut(Purchased, NodeId))
	{
		*Slot = Rank + 1;
	}
	else
	{
		Purchased.push_back(std::make_pair(NodeId, Rank + 1));
	}
	SetError(Reason, "");
	return true;
}

void FProgress::ResetSlot()
{
	Core = 0;
	CoreReason.clear();
	Purchased.clear();
}

double FProgress::GetMetaCoreMult() const
{
	if (!bEconomy)
	{
		return 0.0;
	}
	double Mult = Econ.MetaCoreMult;
	for (const FNode& Node : NodeRows)
	{
		if (Node.EconomyKey != "MetaCoreMult")
		{
			continue;
		}
		const int Rank = EffectiveRank(Node);
		if (Rank <= 0 || Rank > static_cast<int>(Node.EconomyDeltaByRank.size()))
		{
			continue;
		}
		Mult += Node.EconomyDeltaByRank[static_cast<size_t>(Rank - 1)];
	}
	return Mult;
}

FRunStart FProgress::BuildRunStartContext() const
{
	FRunStart Start;
	Start.bUnlockAffixIgnored = true;
	Start.bAffixPoolReady = bPool;
	if (bPool)
	{
		Start.UnlockedAffixIds = Pool;
	}
	Start.MetaCoreMult = GetMetaCoreMult();
	long long Scrap = 0;
	for (const FNode& Node : NodeRows)
	{
		const int Rank = EffectiveRank(Node);
		if (Rank <= 0)
		{
			continue;
		}
		const size_t Index = static_cast<size_t>(Rank - 1);
		if (Index < Node.PermanentModifiersByRank.size())
		{
			const std::vector<FModifier>& Mods = Node.PermanentModifiersByRank[Index];
			Start.PermanentModifiers.insert(Start.PermanentModifiers.end(), Mods.begin(), Mods.end());
		}
		if (Index < Node.StartingScrapByRank.size())
		{
			Scrap += Node.StartingScrapByRank[Index];
		}
		if (Index < Node.FlagsByRank.size())
		{
			const std::vector<std::string>& Flags = Node.FlagsByRank[Index];
			Start.Flags.insert(Start.Flags.end(), Flags.begin(), Flags.end());
		}
	}
	if (Scrap < 0)
	{
		Scrap = 0;
	}
	if (Scrap > static_cast<long long>(std::numeric_limits<int>::max()))
	{
		Scrap = std::numeric_limits<int>::max();
	}
	Start.StartingScrap = static_cast<int>(Scrap);
	return Start;
}

FCorePreview FProgress::PreviewCoreGain(bool bCleared, int RoomsCleared, int BossKilled, int RunScrapRemain) const
{
	FCorePreview Preview;
	Preview.bAppliedToCore = false;
	if (!bEconomy)
	{
		return Preview;
	}
	Preview.bReady = true;
	Preview.MetaCoreMult = GetMetaCoreMult();
	const double Clear = bCleared ? 1.0 : Econ.DeathMult;
	const double Rooms = RoomsCleared > 0 ? static_cast<double>(RoomsCleared) : 0.0;
	const double Bosses = BossKilled > 0 ? static_cast<double>(BossKilled) : 0.0;
	const double Scrap = RunScrapRemain > 0 ? static_cast<double>(RunScrapRemain) : 0.0;
	const double Sum = static_cast<double>(Econ.BaseClearBonus) * Clear
		+ Rooms * static_cast<double>(Econ.PerRoom)
		+ Bosses * static_cast<double>(Econ.BossBonus)
		+ Scrap * Econ.ScrapToCore;
	const double Scaled = Sum * Preview.MetaCoreMult;
	double Floored = std::floor(Scaled + 1e-9);
	if (Floored < 0.0)
	{
		Floored = 0.0;
	}
	if (Floored > static_cast<double>(Econ.SoftCapPerRun))
	{
		Floored = static_cast<double>(Econ.SoftCapPerRun);
	}
	Preview.Core = static_cast<int>(Floored);
	return Preview;
}

std::vector<std::string> FProgress::IgnoredUnlockAffixIds() const
{
	std::vector<std::string> Out;
	for (const FNode& Node : NodeRows)
	{
		for (const std::string& Id : Node.UnlockAffixIds)
		{
			Out.push_back(Node.NodeId + ":" + Id);
		}
	}
	return Out;
}

std::string FProgress::Describe() const
{
	const FRunStart Start = BuildRunStartContext();
	std::string Text = "Core=" + std::to_string(Core) + "\n";
	for (const FNode& Node : NodeRows)
	{
		Text += Node.NodeId + "=" + std::to_string(GetPurchasedRank(Node.NodeId)) + "\n";
	}
	Text += "StartingScrap=" + std::to_string(Start.StartingScrap) + "\n";
	Text += "MetaCoreMult=" + FormatNum(Start.MetaCoreMult) + "\n";
	Text += "Flags=";
	for (size_t Index = 0; Index < Start.Flags.size(); ++Index)
	{
		if (Index > 0)
		{
			Text += ",";
		}
		Text += Start.Flags[Index];
	}
	Text += "\nMods=";
	for (size_t Index = 0; Index < Start.PermanentModifiers.size(); ++Index)
	{
		if (Index > 0)
		{
			Text += ";";
		}
		const FModifier& Mod = Start.PermanentModifiers[Index];
		Text += Mod.Stat + " " + Mod.Op + " " + FormatNum(Mod.Value);
		if (!Mod.WeaponTag.empty())
		{
			Text += " WeaponTag=" + Mod.WeaponTag;
		}
	}
	Text += "\nPool=";
	for (size_t Index = 0; Index < Start.UnlockedAffixIds.size(); ++Index)
	{
		if (Index > 0)
		{
			Text += ",";
		}
		Text += Start.UnlockedAffixIds[Index];
	}
	Text += "\nUnlockAffixIgnored=1\n";
	const std::vector<std::string> Ignored = IgnoredUnlockAffixIds();
	Text += "IgnoredUnlock=";
	for (size_t Index = 0; Index < Ignored.size(); ++Index)
	{
		if (Index > 0)
		{
			Text += ",";
		}
		Text += Ignored[Index];
	}
	Text += "\n";
	return Text;
}

} // namespace PixelSideMeta
