#include "UI/PixelShopWidget.h"

#include "Affix/AffixSubsystemDefault.h"
#include "Event/EventSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Styling/SlateTypes.h"
#include "UI/PixelAffixFont.h"
#include "UI/PixelShopSlotWidget.h"
#include "Containers/Ticker.h"
#include "Widgets/SWidget.h"

#include <cmath>
#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogPixelShopUI, Log, All);

namespace
{
	constexpr int32 ShopSlotCount = 3;
	constexpr double ShopDemoScrap = 128.0;
	constexpr double ShopRerollCost = 25.0;
	constexpr int32 ShopViewportZ = 80;
	const FText ShopTitle = NSLOCTEXT("PixelShop", "Title", "商店");
	const FText ShopHint = NSLOCTEXT("PixelShop", "Hint", "← → 选格 · Enter 买 · R 刷新 · Esc 离开");

	FString JsonString(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Key)
	{
		if (!Obj.IsValid())
		{
			return FString();
		}
		const TSharedPtr<FJsonValue> Field = Obj->TryGetField(Key);
		if (!Field.IsValid() || Field->Type != EJson::String)
		{
			return FString();
		}
		return Field->AsString();
	}

	bool JsonNumber(const TSharedPtr<FJsonObject>& Obj, const TCHAR* Key, double& Out)
	{
		if (!Obj.IsValid())
		{
			return false;
		}
		const TSharedPtr<FJsonValue> Field = Obj->TryGetField(Key);
		if (!Field.IsValid() || Field->Type != EJson::Number)
		{
			return false;
		}
		Out = Field->AsNumber();
		return true;
	}

	int32 SlotIndexFromId(const FString& SlotId)
	{
		if (SlotId == TEXT("Slot_1"))
		{
			return 0;
		}
		if (SlotId == TEXT("Slot_2"))
		{
			return 1;
		}
		if (SlotId == TEXT("Slot_3"))
		{
			return 2;
		}
		return INDEX_NONE;
	}

	EPixelShopItemKind KindFromString(const FString& Kind)
	{
		if (Kind == TEXT("Heal"))
		{
			return EPixelShopItemKind::Heal;
		}
		if (Kind == TEXT("AffixVoucher"))
		{
			return EPixelShopItemKind::AffixVoucher;
		}
		return EPixelShopItemKind::WeaponContract;
	}

	EPixelShopOnPurchase PurchaseFromString(const FString& Signal)
	{
		if (Signal == TEXT("GrantRunWeapon"))
		{
			return EPixelShopOnPurchase::GrantRunWeapon;
		}
		if (Signal == TEXT("HealFractionOfMaxHp"))
		{
			return EPixelShopOnPurchase::HealFractionOfMaxHp;
		}
		if (Signal == TEXT("OpenAffixChoiceImmediate"))
		{
			return EPixelShopOnPurchase::OpenAffixChoiceImmediate;
		}
		return EPixelShopOnPurchase::None;
	}

	struct FPriceBook
	{
		TMap<FName, double> BaseScrap;
		double Multiplier = 1.0;
		bool bLayerFound = false;
	};

	double ResolvePrice(const FPriceBook& Book, FName Key, bool& bFound)
	{
		const double* Base = Book.BaseScrap.Find(Key);
		if (!Base)
		{
			bFound = false;
			return -1.0;
		}
		bFound = true;
		return (*Base) * Book.Multiplier;
	}

	bool LoadJsonObject(const FString& Path, TSharedPtr<FJsonObject>& Out, FString& Error)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			Error = FString::Printf(TEXT("missing %s"), *Path);
			return false;
		}
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
		if (!FJsonSerializer::Deserialize(Reader, Out) || !Out.IsValid())
		{
			Error = FString::Printf(TEXT("bad JSON %s"), *Path);
			return false;
		}
		return true;
	}

	bool ReadPriceBook(const TSharedPtr<FJsonObject>& Root, int32 Layer, FPriceBook& Book, FString& Error)
	{
		if (JsonString(Root, TEXT("currencyId")) != TEXT("Scrap"))
		{
			Error = TEXT("DA_ShopPriceCurve currencyId is not Scrap");
			return false;
		}
		const TArray<TSharedPtr<FJsonValue>>* Layers = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Prices = nullptr;
		if (!Root->TryGetArrayField(TEXT("layers"), Layers) || !Layers
			|| !Root->TryGetArrayField(TEXT("prices"), Prices) || !Prices)
		{
			Error = TEXT("DA_ShopPriceCurve is missing layers or prices");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Layers)
		{
			const TSharedPtr<FJsonObject> Row = Value.IsValid() ? Value->AsObject() : nullptr;
			double LayerNum = 0.0;
			double Multiplier = 0.0;
			if (!JsonNumber(Row, TEXT("layer"), LayerNum) || !JsonNumber(Row, TEXT("multiplier"), Multiplier))
			{
				continue;
			}
			if (FMath::RoundToInt(LayerNum) == Layer)
			{
				Book.Multiplier = Multiplier;
				Book.bLayerFound = true;
			}
		}
		if (!Book.bLayerFound)
		{
			Error = FString::Printf(TEXT("DA_ShopPriceCurve has no layer %d"), Layer);
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Prices)
		{
			const TSharedPtr<FJsonObject> Row = Value.IsValid() ? Value->AsObject() : nullptr;
			const FString Key = JsonString(Row, TEXT("priceKey"));
			double Base = 0.0;
			if (Key.IsEmpty() || !JsonNumber(Row, TEXT("baseScrap"), Base))
			{
				continue;
			}
			Book.BaseScrap.Add(FName(*Key), Base);
		}
		if (Book.BaseScrap.Num() == 0)
		{
			Error = TEXT("DA_ShopPriceCurve has no prices");
			return false;
		}
		return true;
	}

	bool FillShelfFromJson(int32 Layer, TArray<FPixelShopSlotOffer>& Slots, TArray<FPixelShopSlotOffer>& Pool, FString& Error)
	{
		const FString InvPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Shop/DA_ShopInventory.json"));
		const FString PricePath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Shop/DA_ShopPriceCurve.json"));
		TSharedPtr<FJsonObject> InvRoot;
		TSharedPtr<FJsonObject> PriceRoot;
		if (!LoadJsonObject(InvPath, InvRoot, Error) || !LoadJsonObject(PricePath, PriceRoot, Error))
		{
			return false;
		}
		if (JsonString(InvRoot, TEXT("currencyId")) != TEXT("Scrap"))
		{
			Error = TEXT("DA_ShopInventory currencyId is not Scrap");
			return false;
		}
		const FString Insufficient = JsonString(InvRoot, TEXT("insufficientScrap"));
		if (!Insufficient.IsEmpty() && Insufficient != TEXT("GreyOut"))
		{
			UE_LOG(LogPixelShopUI, Warning, TEXT("Shop insufficientScrap is '%s'. This shell still greys the slot."), *Insufficient);
		}
		bool bSkip = true;
		const TSharedPtr<FJsonValue> SkipField = InvRoot->TryGetField(TEXT("skipAllowed"));
		if (SkipField.IsValid() && SkipField->Type == EJson::Boolean)
		{
			bSkip = SkipField->AsBool();
		}
		if (!bSkip)
		{
			UE_LOG(LogPixelShopUI, Warning, TEXT("Shop table skipAllowed is false. Leave still closes without a purchase."));
		}

		FPriceBook Book;
		if (!ReadPriceBook(PriceRoot, Layer, Book, Error))
		{
			return false;
		}

		Slots.Reset();
		Slots.SetNum(ShopSlotCount);
		TArray<bool> bHaveSlot;
		bHaveSlot.Init(false, ShopSlotCount);
		int32 WeaponSlot = INDEX_NONE;
		FText WeaponDescription;
		EPixelShopOnPurchase WeaponPurchase = EPixelShopOnPurchase::GrantRunWeapon;

		const TArray<TSharedPtr<FJsonValue>>* SlotValues = nullptr;
		if (!InvRoot->TryGetArrayField(TEXT("slots"), SlotValues) || !SlotValues)
		{
			Error = TEXT("DA_ShopInventory is missing slots");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *SlotValues)
		{
			const TSharedPtr<FJsonObject> Row = Value.IsValid() ? Value->AsObject() : nullptr;
			const int32 Index = SlotIndexFromId(JsonString(Row, TEXT("slotId")));
			const FString KindText = JsonString(Row, TEXT("kind"));
			if (!Row.IsValid() || Index == INDEX_NONE || KindText.IsEmpty())
			{
				continue;
			}
			FPixelShopSlotOffer Offer;
			Offer.Kind = KindFromString(KindText);
			Offer.Name = FText::FromString(JsonString(Row, TEXT("displayName")));
			Offer.Description = FText::FromString(JsonString(Row, TEXT("description")));
			Offer.ItemId = FName(*JsonString(Row, TEXT("offerId")));
			Offer.PriceKey = FName(*JsonString(Row, TEXT("priceKey")));
			Offer.OnPurchase = PurchaseFromString(JsonString(Row, TEXT("onPurchase")));
			double HealFraction = 0.0;
			if (JsonNumber(Row, TEXT("healFractionOfMaxHp"), HealFraction))
			{
				Offer.HealFractionOfMaxHp = HealFraction;
			}
			if (Offer.Kind == EPixelShopItemKind::WeaponContract)
			{
				WeaponSlot = Index;
				WeaponDescription = Offer.Description;
				WeaponPurchase = Offer.OnPurchase;
				bHaveSlot[Index] = true;
				Slots[Index] = Offer;
				continue;
			}
			bool bPriced = false;
			Offer.Price = ResolvePrice(Book, Offer.PriceKey, bPriced);
			if (!bPriced)
			{
				UE_LOG(LogPixelShopUI, Warning, TEXT("Shop priceKey '%s' is missing from DA_ShopPriceCurve."), *Offer.PriceKey.ToString());
				Offer.Price = -1.0;
			}
			Slots[Index] = Offer;
			bHaveSlot[Index] = true;
		}

		Pool.Reset();
		const TArray<TSharedPtr<FJsonValue>>* PoolValues = nullptr;
		if (InvRoot->TryGetArrayField(TEXT("weaponContractPool"), PoolValues) && PoolValues)
		{
			for (const TSharedPtr<FJsonValue>& Value : *PoolValues)
			{
				const TSharedPtr<FJsonObject> Row = Value.IsValid() ? Value->AsObject() : nullptr;
				const FString OfferId = JsonString(Row, TEXT("offerId"));
				const FString PriceKey = JsonString(Row, TEXT("priceKey"));
				if (OfferId.IsEmpty() || PriceKey.IsEmpty())
				{
					continue;
				}
				FPixelShopSlotOffer Offer;
				Offer.Kind = EPixelShopItemKind::WeaponContract;
				Offer.ItemId = FName(*OfferId);
				Offer.Name = FText::FromString(JsonString(Row, TEXT("displayName")));
				Offer.Description = WeaponDescription;
				Offer.PriceKey = FName(*PriceKey);
				Offer.DataAssetName = JsonString(Row, TEXT("dataAssetName"));
				Offer.OnPurchase = WeaponPurchase;
				bool bPriced = false;
				Offer.Price = ResolvePrice(Book, Offer.PriceKey, bPriced);
				if (!bPriced)
				{
					UE_LOG(LogPixelShopUI, Warning, TEXT("Shop weapon priceKey '%s' is missing from DA_ShopPriceCurve."), *PriceKey);
					Offer.Price = -1.0;
				}
				Pool.Add(Offer);
			}
		}

		if (WeaponSlot != INDEX_NONE)
		{
			if (Pool.Num() == 0)
			{
				Error = TEXT("WeaponContract pool is empty");
				return false;
			}
			Slots[WeaponSlot] = Pool[0];
		}
		for (int32 Index = 0; Index < ShopSlotCount; ++Index)
		{
			if (!bHaveSlot[Index])
			{
				Error = FString::Printf(TEXT("DA_ShopInventory is missing Slot_%d"), Index + 1);
				return false;
			}
		}
		return true;
	}

	TArray<FString> SplitCsv(const FString& Line)
	{
		TArray<FString> Cols;
		Line.ParseIntoArray(Cols, TEXT(","), false);
		for (FString& Col : Cols)
		{
			Col.TrimStartAndEndInline();
		}
		return Cols;
	}

	bool FillShelfFromCsv(int32 Layer, TArray<FPixelShopSlotOffer>& Slots, TArray<FPixelShopSlotOffer>& Pool, FString& Error)
	{
		const FString InvPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Shop/DA_ShopInventory.csv"));
		const FString PricePath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data/Shop/DA_ShopPriceCurve.csv"));
		FString InvText;
		FString PriceText;
		if (!FFileHelper::LoadFileToString(InvText, *InvPath) || !FFileHelper::LoadFileToString(PriceText, *PricePath))
		{
			Error = TEXT("shop CSV missing");
			return false;
		}
		FPriceBook Book;
		TArray<FString> PriceLines;
		PriceText.ParseIntoArrayLines(PriceLines);
		for (const FString& Raw : PriceLines)
		{
			const TArray<FString> Cols = SplitCsv(Raw);
			if (Cols.Num() < 11 || Cols[0] == TEXT("contract_version"))
			{
				continue;
			}
			if (Cols[1] != TEXT("Scrap"))
			{
				Error = TEXT("DA_ShopPriceCurve.csv currency is not Scrap");
				return false;
			}
			const int32 RowLayer = FCString::Atoi(*Cols[9]);
			if (RowLayer == Layer)
			{
				Book.Multiplier = FCString::Atod(*Cols[10]);
				Book.bLayerFound = true;
			}
			if (!Cols[3].IsEmpty() && !Book.BaseScrap.Contains(FName(*Cols[3])))
			{
				Book.BaseScrap.Add(FName(*Cols[3]), FCString::Atod(*Cols[8]));
			}
		}
		if (!Book.bLayerFound)
		{
			Error = FString::Printf(TEXT("DA_ShopPriceCurve.csv has no layer %d"), Layer);
			return false;
		}

		Slots.Reset();
		Slots.SetNum(ShopSlotCount);
		TArray<bool> bHaveSlot;
		bHaveSlot.Init(false, ShopSlotCount);
		int32 WeaponSlot = INDEX_NONE;
		FText WeaponDescription;
		EPixelShopOnPurchase WeaponPurchase = EPixelShopOnPurchase::GrantRunWeapon;
		Pool.Reset();

		TArray<FString> InvLines;
		InvText.ParseIntoArrayLines(InvLines);
		for (const FString& Raw : InvLines)
		{
			const TArray<FString> Cols = SplitCsv(Raw);
			if (Cols.Num() < 14 || Cols[0] == TEXT("contract_version"))
			{
				continue;
			}
			if (Cols[1] != TEXT("Scrap"))
			{
				Error = TEXT("DA_ShopInventory.csv currency is not Scrap");
				return false;
			}
			const int32 Index = SlotIndexFromId(Cols[3]);
			if (Index == INDEX_NONE)
			{
				continue;
			}
			const EPixelShopItemKind Kind = KindFromString(Cols[4]);
			if (Kind == EPixelShopItemKind::WeaponContract)
			{
				if (WeaponSlot == INDEX_NONE)
				{
					WeaponSlot = Index;
					WeaponDescription = FText::FromString(Cols[5]);
					WeaponPurchase = PurchaseFromString(Cols[13]);
					bHaveSlot[Index] = true;
				}
				FPixelShopSlotOffer Offer;
				Offer.Kind = EPixelShopItemKind::WeaponContract;
				Offer.ItemId = FName(*Cols[6]);
				Offer.Name = FText::FromString(Cols[9]);
				Offer.Description = WeaponDescription;
				Offer.PriceKey = FName(*Cols[10]);
				Offer.DataAssetName = Cols[8];
				Offer.OnPurchase = PurchaseFromString(Cols[13]);
				bool bPriced = false;
				Offer.Price = ResolvePrice(Book, Offer.PriceKey, bPriced);
				if (!bPriced)
				{
					Offer.Price = -1.0;
				}
				Pool.Add(Offer);
				continue;
			}
			if (bHaveSlot[Index])
			{
				continue;
			}
			FPixelShopSlotOffer Offer;
			Offer.Kind = Kind;
			Offer.ItemId = FName(*Cols[6]);
			Offer.Name = FText::FromString(Cols[5]);
			Offer.Description = FText::FromString(Cols[9].IsEmpty() ? Cols[5] : Cols[9]);
			Offer.PriceKey = FName(*Cols[10]);
			Offer.OnPurchase = PurchaseFromString(Cols[13]);
			if (!Cols[11].IsEmpty())
			{
				Offer.HealFractionOfMaxHp = FCString::Atod(*Cols[11]);
			}
			bool bPriced = false;
			Offer.Price = ResolvePrice(Book, Offer.PriceKey, bPriced);
			if (!bPriced)
			{
				Offer.Price = -1.0;
			}
			Slots[Index] = Offer;
			bHaveSlot[Index] = true;
		}
		if (WeaponSlot != INDEX_NONE && Pool.Num() > 0)
		{
			for (FPixelShopSlotOffer& Offer : Pool)
			{
				Offer.Description = WeaponDescription;
				Offer.OnPurchase = WeaponPurchase;
			}
			Slots[WeaponSlot] = Pool[0];
		}
		for (int32 Index = 0; Index < ShopSlotCount; ++Index)
		{
			if (!bHaveSlot[Index])
			{
				Error = TEXT("shop CSV is missing a slot");
				return false;
			}
		}
		return Pool.Num() > 0 || WeaponSlot == INDEX_NONE;
	}

	void FillLockedShelf(int32 Layer, TArray<FPixelShopSlotOffer>& Slots, TArray<FPixelShopSlotOffer>& Pool)
	{
		const double Multiplier = (Layer == 2) ? 1.15 : 1.0;
		auto Weapon = [Multiplier](const TCHAR* OfferId, const TCHAR* Name, const TCHAR* Asset, const TCHAR* Key, double Base)
		{
			FPixelShopSlotOffer Offer;
			Offer.Kind = EPixelShopItemKind::WeaponContract;
			Offer.ItemId = FName(OfferId);
			Offer.Name = FText::FromString(Name);
			Offer.Description = FText::FromString(TEXT("本局临时武器。货位从武器合约池展示一把；三把均有独立标价，本表不写权重。"));
			Offer.PriceKey = FName(Key);
			Offer.DataAssetName = Asset;
			Offer.OnPurchase = EPixelShopOnPurchase::GrantRunWeapon;
			Offer.Price = Base * Multiplier;
			return Offer;
		};
		Pool.Reset();
		Pool.Add(Weapon(TEXT("WeaponContract_Peashooter"), TEXT("豆射手"), TEXT("DA_Weapon_Peashooter"), TEXT("Weapon_Peashooter"), 45.0));
		Pool.Add(Weapon(TEXT("WeaponContract_Scattergun"), TEXT("散射"), TEXT("DA_Weapon_Scattergun"), TEXT("Weapon_Scattergun"), 60.0));
		Pool.Add(Weapon(TEXT("WeaponContract_RailPierce"), TEXT("穿透轨"), TEXT("DA_Weapon_RailPierce"), TEXT("Weapon_RailPierce"), 80.0));

		Slots.Reset();
		Slots.SetNum(ShopSlotCount);
		Slots[0] = Pool[0];

		FPixelShopSlotOffer Heal;
		Heal.Kind = EPixelShopItemKind::Heal;
		Heal.ItemId = FName(TEXT("HealMaxHp"));
		Heal.Name = FText::FromString(TEXT("回复"));
		Heal.Description = FText::FromString(TEXT("回复约 40% MaxHP"));
		Heal.PriceKey = FName(TEXT("Heal"));
		Heal.HealFractionOfMaxHp = 0.4;
		Heal.OnPurchase = EPixelShopOnPurchase::HealFractionOfMaxHp;
		Heal.Price = 30.0 * Multiplier;
		Slots[1] = Heal;

		FPixelShopSlotOffer Voucher;
		Voucher.Kind = EPixelShopItemKind::AffixVoucher;
		Voucher.ItemId = FName(TEXT("AffixChoiceVoucher"));
		Voucher.Name = FText::FromString(TEXT("三选一券"));
		Voucher.Description = FText::FromString(TEXT("购买确认后立即三选一"));
		Voucher.PriceKey = FName(TEXT("AffixVoucher"));
		Voucher.OnPurchase = EPixelShopOnPurchase::OpenAffixChoiceImmediate;
		Voucher.Price = 60.0 * Multiplier;
		Slots[2] = Voucher;
	}

	bool ReadShopShelf(int32 Layer, TArray<FPixelShopSlotOffer>& Slots, TArray<FPixelShopSlotOffer>& Pool, FString& Error)
	{
		FString JsonError;
		if (FillShelfFromJson(Layer, Slots, Pool, JsonError))
		{
			return true;
		}
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop JSON unused: %s"), *JsonError);
		FString CsvError;
		if (FillShelfFromCsv(Layer, Slots, Pool, CsvError))
		{
			UE_LOG(LogPixelShopUI, Log, TEXT("Shop shelf from DA_Shop CSV."));
			return true;
		}
		Error = JsonError + TEXT(" | ") + CsvError;
		return false;
	}

	void StyleShopLine(UTextBlock* Text, const FSlateFontInfo& Font, const FLinearColor& Color, ETextJustify::Type Align)
	{
		if (!Text)
		{
			return;
		}
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(Align);
		Text->SetShadowOffset(FVector2D::ZeroVector);
		Text->SetAutoWrapText(false);
		Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	void StyleShopButton(UButton* Button)
	{
		if (!Button)
		{
			return;
		}
		Button->SetIsFocusable(false);
		FButtonStyle Style = Button->GetStyle();
		auto Face = [](FSlateBrush& Brush, const FLinearColor& Color)
		{
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.TintColor = FSlateColor(Color);
		};
		Face(Style.Normal, FLinearColor(FColor(0x2C, 0x2C, 0x44)));
		Face(Style.Hovered, FLinearColor(FColor(0x3E, 0x3E, 0x5C)));
		Face(Style.Pressed, FLinearColor(FColor(0x22, 0x22, 0x34)));
		Face(Style.Disabled, FLinearColor(FColor(0x2A, 0x2A, 0x2A)));
		Style.NormalPadding = FMargin(8.f, 4.f);
		Style.PressedPadding = FMargin(8.f, 5.f, 8.f, 3.f);
		const FSlateColor Ink(FLinearColor(FColor(0xF2, 0xF2, 0xF2)));
		Style.NormalForeground = Ink;
		Style.HoveredForeground = Ink;
		Style.PressedForeground = Ink;
		Style.DisabledForeground = FSlateColor(FLinearColor(FColor(0x88, 0x88, 0x88)));
		Button->SetStyle(Style);
	}

	/** int32 debit for AddRunScrap. Same half-away llround as GrantScrap. Does not write the price curve. */
	bool TryRunDebit(double Price, int32& OutDebit)
	{
		if (!std::isfinite(Price) || Price < 0.0
			|| Price > static_cast<double>(std::numeric_limits<int32>::max()))
		{
			return false;
		}
		const long long Rounded = std::llround(Price);
		if (Rounded < 0 || Rounded > static_cast<long long>(std::numeric_limits<int32>::max()))
		{
			return false;
		}
		OutDebit = static_cast<int32>(Rounded);
		return true;
	}

	FText FormatScrapAmount(double Value)
	{
		const double Cents = FMath::RoundToDouble(Value * 100.0) / 100.0;
		if (FMath::IsNearlyEqual(Cents, FMath::RoundToDouble(Cents), 0.001))
		{
			return FText::AsNumber(FMath::RoundToInt(Cents));
		}
		FString Text = FString::Printf(TEXT("%.2f"), Cents);
		while (Text.Len() > 0 && Text.EndsWith(TEXT("0")))
		{
			Text.LeftChopInline(1);
		}
		if (Text.EndsWith(TEXT(".")))
		{
			Text.LeftChopInline(1);
		}
		return FText::FromString(Text);
	}
}

UPixelShopWidget::UPixelShopWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsFocusable = true;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPixelShopWidget::CollectShelfOffers(int32 Layer, TArray<FPixelShopSlotOffer>& OutOffers, TArray<FPixelShopSlotOffer>& OutPool)
{
	OutOffers.Reset();
	OutPool.Reset();
	FString Error;
	const int32 UseLayer = Layer > 0 ? Layer : 1;
	if (!ReadShopShelf(UseLayer, OutOffers, OutPool, Error))
	{
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop tables unread (%s). Using the locked #35 rows."), *Error);
		FillLockedShelf(UseLayer, OutOffers, OutPool);
	}
}

void UPixelShopWidget::OpenShop(const TArray<FPixelShopSlotOffer>& InOffers, double Scrap, bool bDemoPause)
{
	if (IsDesignTime())
	{
		return;
	}
	UEventSubsystem* Events = UEventSubsystem::Get(this);
	if (!Events)
	{
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop run wallet did not open: UEventSubsystem is missing."));
		return;
	}
	const int32 RunScrap = Events->GetRunScrap();
	UE_LOG(LogPixelShopUI, Log,
		TEXT("Shop OpenShop reads GetRunScrap=%d. Passed Scrap %.2f is not the wallet."),
		RunScrap, Scrap);
	ApplyOpen(InOffers, static_cast<double>(RunScrap), bDemoPause, false, true);
}

void UPixelShopWidget::OpenShopDemo(int32 Layer)
{
	TArray<FPixelShopSlotOffer> Built;
	TArray<FPixelShopSlotOffer> Pool;
	FString Error;
	const int32 UseLayer = Layer > 0 ? Layer : 1;
	if (!ReadShopShelf(UseLayer, Built, Pool, Error))
	{
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop tables unread (%s). Using the locked #35 rows."), *Error);
		FillLockedShelf(UseLayer, Built, Pool);
	}
	WeaponPool = Pool;
	ShownWeapon = 0;
	PriceLayer = UseLayer;
	if (Built.IsValidIndex(0))
	{
		UE_LOG(LogPixelShopUI, Log, TEXT("Shop shelf layer %d: %s %.2f, %s %.2f, %s %.2f"),
			UseLayer,
			*Built[0].ItemId.ToString(), Built[0].Price,
			Built.IsValidIndex(1) ? *Built[1].ItemId.ToString() : TEXT("?"),
			Built.IsValidIndex(1) ? Built[1].Price : -1.0,
			Built.IsValidIndex(2) ? *Built[2].ItemId.ToString() : TEXT("?"),
			Built.IsValidIndex(2) ? Built[2].Price : -1.0);
	}
	ApplyOpen(Built, ShopDemoScrap, true, true, false);
}

void UPixelShopWidget::CloseShop()
{
	if (!bOpen && !bOwnsPause)
	{
		return;
	}
	const bool bWasOpen = bOpen;
	bOpen = false;
	EndNudge();
	ReleaseInputLock();
	ReleaseInput();
	if (!IsDesignTime())
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
	EndDemoPause();
	if (bWasOpen)
	{
		if (bRunWallet)
		{
			UE_LOG(LogPixelShopUI, Log, TEXT("Shop closed. runScrap=%d"), RunScrapShown);
		}
		else
		{
			UE_LOG(LogPixelShopUI, Log, TEXT("Shop closed. demoScrap=%.2f"), DemoScrap);
		}
		OnShopClosed.Broadcast();
	}
}

void UPixelShopWidget::SetSlotOffer(int32 SlotIndex, const FPixelShopSlotOffer& Offer)
{
	if (SlotIndex < 0 || SlotIndex >= ShopSlotCount || !Offers.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop SetSlotOffer bad index %d."), SlotIndex);
		return;
	}
	if (bSlotSold[SlotIndex])
	{
		return;
	}
	bDemoShelf = false;
	Offers[SlotIndex] = Offer;
	if (bOpen || IsDesignTime())
	{
		RefreshAll();
	}
}

void UPixelShopWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureLayout();
	if (IsDesignTime())
	{
		ShowDesignShelf();
	}
}

TSharedRef<SWidget> UPixelShopWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UPixelShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureLayout();
	BindInteractions();
	if (IsDesignTime())
	{
		ShowDesignShelf();
		return;
	}
	if (!bOpen)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPixelShopWidget::NativeDestruct()
{
	EndNudge();
	ReleaseInputLock();
	if (Btn_Reroll)
	{
		Btn_Reroll->OnClicked.RemoveDynamic(this, &UPixelShopWidget::HandleRerollClicked);
	}
	if (Btn_Leave)
	{
		Btn_Leave->OnClicked.RemoveDynamic(this, &UPixelShopWidget::HandleLeaveClicked);
	}
	if (bOpen || bOwnsPause)
	{
		bOpen = false;
		EndDemoPause();
		UE_LOG(LogPixelShopUI, Log, TEXT("Shop destroyed while open. Demo pause released."));
	}
	ReleaseInput();
	Super::NativeDestruct();
}

FReply UPixelShopWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bOpen)
	{
		return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
	}
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape)
	{
		CloseShop();
		return FReply::Handled();
	}
	if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Up)
	{
		MoveFocus(-1);
		return FReply::Handled();
	}
	if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Down)
	{
		MoveFocus(1);
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		TryBuy(FocusedIndex);
		return FReply::Handled();
	}
	if (Key == EKeys::R)
	{
		TryReroll();
		return FReply::Handled();
	}
	return FReply::Handled();
}

void UPixelShopWidget::ApplyOpen(const TArray<FPixelShopSlotOffer>& InOffers, double InScrap, bool bInDemoPause, bool bInDemoShelf, bool bInRunWallet)
{
	if (IsDesignTime())
	{
		return;
	}
	if (IsAffixChoicePending())
	{
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop did not open: an affix choice is pending."));
		return;
	}
	EnsureLayout();
	BindInteractions();
	bRunWallet = bInRunWallet;
	bDemoShelf = bInDemoShelf;
	LoadOffers(InOffers, InScrap);
	EndNudge();
	ReleaseInputLock();
	const bool bWasOpen = bOpen;
	bOpen = true;
	RefreshAll();
	SetVisibility(ESlateVisibility::Visible);
	if (!IsInViewport())
	{
		AddToViewport(ShopViewportZ);
	}
	AcquireInput();
	if (!bWasOpen && bInDemoPause)
	{
		BeginDemoPause();
	}
	if (bRunWallet)
	{
		UE_LOG(LogPixelShopUI, Log, TEXT("Shop open. slots=3 scrap=%d demoShelf=%s ownsPause=%s wallet=run"),
			RunScrapShown,
			bDemoShelf ? TEXT("true") : TEXT("false"),
			bOwnsPause ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogPixelShopUI, Log, TEXT("Shop open. slots=3 scrap=%.2f demoShelf=%s ownsPause=%s"),
			DemoScrap,
			bDemoShelf ? TEXT("true") : TEXT("false"),
			bOwnsPause ? TEXT("true") : TEXT("false"));
	}
}

void UPixelShopWidget::LoadOffers(const TArray<FPixelShopSlotOffer>& InOffers, double InScrap)
{
	if (bRunWallet)
	{
		const double Clamped = FMath::Clamp(InScrap, 0.0, static_cast<double>(std::numeric_limits<int32>::max()));
		RunScrapShown = static_cast<int32>(Clamped);
	}
	else
	{
		DemoScrap = FMath::Max(0.0, InScrap);
	}
	if (InOffers.Num() != ShopSlotCount)
	{
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop expected 3 offers, got %d."), InOffers.Num());
	}
	Offers.SetNum(ShopSlotCount);
	for (int32 Index = 0; Index < ShopSlotCount; ++Index)
	{
		Offers[Index] = InOffers.IsValidIndex(Index) ? InOffers[Index] : FPixelShopSlotOffer();
		bSlotSold[Index] = false;
	}
	FocusedIndex = 0;
}

void UPixelShopWidget::ShowDesignShelf()
{
	TArray<FPixelShopSlotOffer> Built;
	TArray<FPixelShopSlotOffer> Pool;
	FString Error;
	if (!ReadShopShelf(1, Built, Pool, Error))
	{
		FillLockedShelf(1, Built, Pool);
	}
	WeaponPool = Pool;
	ShownWeapon = 0;
	PriceLayer = 1;
	bRunWallet = false;
	bDemoShelf = true;
	LoadOffers(Built, ShopDemoScrap);
	bOpen = false;
	RefreshAll();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UPixelShopWidget::HandleSlotBuyClicked(int32 SlotIndex)
{
	TryBuy(SlotIndex);
}

void UPixelShopWidget::HandleRerollClicked()
{
	TryReroll();
}

void UPixelShopWidget::HandleLeaveClicked()
{
	CloseShop();
}

void UPixelShopWidget::EnsureLayout()
{
	const bool bHaveShell = Overlay_Root && Img_Dim && Txt_Title && Txt_Scrap && HB_Slots && Btn_Reroll && Btn_Leave && Txt_Hint;
	if (!bHaveShell)
	{
		if (WidgetTree && !WidgetTree->RootWidget)
		{
			BuildFallbackShell();
		}
		else if (!bWarnedMissingWidgets)
		{
			bWarnedMissingWidgets = true;
			UE_LOG(LogPixelShopUI, Warning,
				TEXT("WBP_Shop is missing one of Overlay_Root, Img_Dim, Txt_Title, Txt_Scrap, HB_Slots, Slot_1, Slot_2, Slot_3, Btn_Reroll, Btn_Leave, Txt_Hint."));
		}
	}
	EnsureFallbackSlots();
}

void UPixelShopWidget::BuildFallbackShell()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	const FSlateFontInfo TitleFont = PixelAffixMakeFont(16);
	const FSlateFontInfo ScrapFont = PixelAffixMakeFont(14);
	const FSlateFontInfo HintFont = PixelAffixMakeFont(11);
	const FSlateFontInfo ButtonFont = PixelAffixMakeFont(12);
	const FLinearColor Ink(FColor(0xF2, 0xF2, 0xF2));
	const FLinearColor ScrapColor(FColor(0xFF, 0xD5, 0x4A));

	Overlay_Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay_Root"));
	Overlay_Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	WidgetTree->RootWidget = Overlay_Root;

	Img_Dim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Img_Dim"));
	FSlateBrush DimBrush;
	DimBrush.DrawAs = ESlateBrushDrawType::Box;
	DimBrush.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	Img_Dim->SetBrush(DimBrush);
	Img_Dim->SetColorAndOpacity(FLinearColor::White);
	if (UOverlaySlot* DimSlot = Overlay_Root->AddChildToOverlay(Img_Dim))
	{
		DimSlot->SetHorizontalAlignment(HAlign_Fill);
		DimSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VB_Shop"));
	Column->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UOverlaySlot* ColumnSlot = Overlay_Root->AddChildToOverlay(Column))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
		ColumnSlot->SetVerticalAlignment(VAlign_Fill);
		ColumnSlot->SetPadding(FMargin(16.f, 12.f, 16.f, 12.f));
	}

	UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Top"));
	Txt_Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Title"));
	Txt_Scrap = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Scrap"));
	StyleShopLine(Txt_Title, TitleFont, Ink, ETextJustify::Left);
	StyleShopLine(Txt_Scrap, ScrapFont, ScrapColor, ETextJustify::Right);
	Txt_Title->SetText(ShopTitle);
	if (UHorizontalBoxSlot* TitleSlot = Top->AddChildToHorizontalBox(Txt_Title))
	{
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}
	USpacer* TopGap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Spacer_Top"));
	if (UHorizontalBoxSlot* GapSlot = Top->AddChildToHorizontalBox(TopGap))
	{
		GapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	if (UHorizontalBoxSlot* ScrapSlot = Top->AddChildToHorizontalBox(Txt_Scrap))
	{
		ScrapSlot->SetVerticalAlignment(VAlign_Center);
	}

	HB_Slots = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Slots"));
	HB_Slots->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	auto MakeButton = [this, ButtonFont, Ink](const TCHAR* ButtonName, const TCHAR* LabelName, const FText& Label, UButton*& OutButton, UTextBlock*& OutLabel, float Width)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("Size_%s"), ButtonName));
		Size->SetWidthOverride(Width);
		Size->SetHeightOverride(28.f);
		OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		StyleShopButton(OutButton);
		OutLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		StyleShopLine(OutLabel, ButtonFont, Ink, ETextJustify::Center);
		OutLabel->SetText(Label);
		OutButton->AddChild(OutLabel);
		Size->SetContent(OutButton);
		return Size;
	};

	USizeBox* RerollSize = MakeButton(TEXT("Btn_Reroll"), TEXT("Txt_Reroll"),
		FText::Format(NSLOCTEXT("PixelShop", "Reroll", "刷新 {0}"), FormatScrapAmount(ShopRerollCost)),
		Btn_Reroll, Txt_Reroll, 112.f);
	USizeBox* LeaveSize = MakeButton(TEXT("Btn_Leave"), TEXT("Txt_Leave"),
		NSLOCTEXT("PixelShop", "Leave", "离开"),
		Btn_Leave, Txt_Leave, 72.f);

	UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HB_Bottom"));
	if (UHorizontalBoxSlot* RerollSlot = Bottom->AddChildToHorizontalBox(RerollSize))
	{
		RerollSlot->SetVerticalAlignment(VAlign_Center);
	}
	USpacer* BottomGap = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Spacer_Bottom"));
	if (UHorizontalBoxSlot* GapSlot = Bottom->AddChildToHorizontalBox(BottomGap))
	{
		GapSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	if (UHorizontalBoxSlot* LeaveSlot = Bottom->AddChildToHorizontalBox(LeaveSize))
	{
		LeaveSlot->SetVerticalAlignment(VAlign_Center);
	}

	Txt_Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Txt_Hint"));
	StyleShopLine(Txt_Hint, HintFont, Ink, ETextJustify::Center);
	Txt_Hint->SetText(ShopHint);

	auto AddRow = [Column](UWidget* Widget, float TopPad, float BottomPad, ESlateSizeRule::Type SizeRule)
	{
		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Widget))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetPadding(FMargin(0.f, TopPad, 0.f, BottomPad));
			if (SizeRule == ESlateSizeRule::Fill)
			{
				Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
	};
	AddRow(Top, 0.f, 8.f, ESlateSizeRule::Automatic);
	AddRow(HB_Slots, 12.f, 0.f, ESlateSizeRule::Automatic);
	USpacer* Mid = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), TEXT("Spacer_Mid"));
	AddRow(Mid, 0.f, 0.f, ESlateSizeRule::Fill);
	AddRow(Bottom, 0.f, 6.f, ESlateSizeRule::Automatic);
	AddRow(Txt_Hint, 0.f, 0.f, ESlateSizeRule::Automatic);
	if (UVerticalBoxSlot* SlotRow = Cast<UVerticalBoxSlot>(HB_Slots->Slot))
	{
		SlotRow->SetHorizontalAlignment(HAlign_Center);
	}

	bBuiltFallbackShell = true;
	BindInteractions();
}

void UPixelShopWidget::EnsureFallbackSlots()
{
	if (Slot_1 && Slot_2 && Slot_3)
	{
		return;
	}
	if (!bBuiltFallbackShell || !HB_Slots)
	{
		return;
	}
	UPixelShopSlotWidget** Slots[ShopSlotCount] = {&Slot_1, &Slot_2, &Slot_3};
	const TCHAR* Names[ShopSlotCount] = {TEXT("Slot_1"), TEXT("Slot_2"), TEXT("Slot_3")};
	for (int32 Index = 0; Index < ShopSlotCount; ++Index)
	{
		if (*Slots[Index])
		{
			continue;
		}
		UPixelShopSlotWidget* Slot = CreateWidget<UPixelShopSlotWidget>(this, UPixelShopSlotWidget::StaticClass(), Names[Index]);
		if (!Slot)
		{
			UE_LOG(LogPixelShopUI, Warning, TEXT("Shop failed to create %s."), Names[Index]);
			continue;
		}
		Slot->SetSlotIndex(Index);
		*Slots[Index] = Slot;
		if (UHorizontalBoxSlot* BoxSlot = HB_Slots->AddChildToHorizontalBox(Slot))
		{
			const float Right = (Index < ShopSlotCount - 1) ? 12.f : 0.f;
			BoxSlot->SetPadding(FMargin(0.f, 0.f, Right, 0.f));
			BoxSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	BindInteractions();
}

void UPixelShopWidget::BindInteractions()
{
	UPixelShopSlotWidget* Slots[ShopSlotCount] = {Slot_1, Slot_2, Slot_3};
	for (int32 Index = 0; Index < ShopSlotCount; ++Index)
	{
		if (!Slots[Index])
		{
			continue;
		}
		Slots[Index]->SetSlotIndex(Index);
		Slots[Index]->OnBuyClicked.RemoveDynamic(this, &UPixelShopWidget::HandleSlotBuyClicked);
		Slots[Index]->OnBuyClicked.AddDynamic(this, &UPixelShopWidget::HandleSlotBuyClicked);
	}
	if (Btn_Reroll)
	{
		Btn_Reroll->OnClicked.RemoveDynamic(this, &UPixelShopWidget::HandleRerollClicked);
		Btn_Reroll->OnClicked.AddDynamic(this, &UPixelShopWidget::HandleRerollClicked);
	}
	if (Btn_Leave)
	{
		Btn_Leave->OnClicked.RemoveDynamic(this, &UPixelShopWidget::HandleLeaveClicked);
		Btn_Leave->OnClicked.AddDynamic(this, &UPixelShopWidget::HandleLeaveClicked);
	}
}

void UPixelShopWidget::RefreshAll()
{
	if (Txt_Title)
	{
		Txt_Title->SetText(ShopTitle);
	}
	if (Txt_Scrap)
	{
		const double Shown = bRunWallet ? static_cast<double>(RunScrapShown) : DemoScrap;
		Txt_Scrap->SetText(FText::Format(
			NSLOCTEXT("PixelShop", "Scrap", "Scrap {0}"),
			FormatScrapAmount(Shown)));
	}
	if (Txt_Hint)
	{
		Txt_Hint->SetText(ShopHint);
	}
	if (Txt_Reroll)
	{
		Txt_Reroll->SetText(FText::Format(
			NSLOCTEXT("PixelShop", "Reroll", "刷新 {0}"),
			FormatScrapAmount(ShopRerollCost)));
	}
	if (Txt_Leave)
	{
		Txt_Leave->SetText(NSLOCTEXT("PixelShop", "Leave", "离开"));
	}
	if (Btn_Reroll)
	{
		Btn_Reroll->SetIsEnabled(CanReroll());
	}

	UAffixSubsystem* Affix = FindAffix();
	for (int32 Index = 0; Index < ShopSlotCount; ++Index)
	{
		UPixelShopSlotWidget* Slot = SlotAt(Index);
		if (!Slot || !Offers.IsValidIndex(Index))
		{
			continue;
		}
		FAffixCardView View;
		const FAffixCardView* ViewPtr = nullptr;
		if (Offers[Index].AffixDef)
		{
			if (Affix)
			{
				View = Affix->GetAffixCardView(Offers[Index].AffixDef, 1);
				ViewPtr = &View;
			}
			else if (!bLoggedMissingAffix)
			{
				bLoggedMissingAffix = true;
				UE_LOG(LogPixelShopUI, Warning, TEXT("Shop AffixDef set but UAffixSubsystemDefault is missing. Showing the table name."));
			}
		}
		const bool bSold = bSlotSold[Index];
		Slot->SetSlotIndex(Index);
		Slot->ApplySlot(Offers[Index], ViewPtr, bSold, !bSold && CanAfford(Offers[Index].Price));
		Slot->SetFocused(bOpen && Index == FocusedIndex);
	}
}

void UPixelShopWidget::RefreshFocus()
{
	for (int32 Index = 0; Index < ShopSlotCount; ++Index)
	{
		if (UPixelShopSlotWidget* Slot = SlotAt(Index))
		{
			Slot->SetFocused(bOpen && Index == FocusedIndex);
		}
	}
}

void UPixelShopWidget::TryBuy(int32 Index)
{
	if (bRunWallet)
	{
		TryBuyFromRun(Index);
		return;
	}
	if (!bOpen || bInputLocked || Index < 0 || Index >= ShopSlotCount || !Offers.IsValidIndex(Index))
	{
		return;
	}
	if (bSlotSold[Index])
	{
		return;
	}
	if (!CanAfford(Offers[Index].Price))
	{
		BeginNudge(Index);
		UE_LOG(LogPixelShopUI, Log, TEXT("Shop buy greyed out. slot=%d price=%.2f scrap=%.2f"), Index, Offers[Index].Price, DemoScrap);
		return;
	}

	bInputLocked = true;
	const FPixelShopSlotOffer Bought = Offers[Index];
	DemoScrap -= Bought.Price;
	bSlotSold[Index] = true;
	if (Index == FocusedIndex)
	{
		MoveFocus(1);
	}
	RefreshAll();
	OnBuyItem.Broadcast(Index, Bought.ItemId);
	UE_LOG(LogPixelShopUI, Log, TEXT("Shop demo buy slot=%d id=%s scrap=%.2f. Local DemoScrap only. Does not call AddRunScrap."),
		Index, *Bought.ItemId.ToString(), DemoScrap);
	ArmInputLock();
	if (Bought.OnPurchase == EPixelShopOnPurchase::OpenAffixChoiceImmediate)
	{
		CloseShop();
		OnRequestAffixChoiceFromVoucher.Broadcast();
	}
}

void UPixelShopWidget::TryReroll()
{
	if (bRunWallet)
	{
		TryRerollFromRun();
		return;
	}
	if (!bOpen || bInputLocked || !CanReroll())
	{
		return;
	}
	bInputLocked = true;
	DemoScrap -= ShopRerollCost;
	if (bDemoShelf && WeaponPool.Num() > 0 && !bSlotSold[0]
		&& Offers.IsValidIndex(0) && Offers[0].Kind == EPixelShopItemKind::WeaponContract)
	{
		ShownWeapon = (ShownWeapon + 1) % WeaponPool.Num();
		Offers[0] = WeaponPool[ShownWeapon];
	}
	RefreshAll();
	OnReroll.Broadcast();
	UE_LOG(LogPixelShopUI, Log, TEXT("Shop demo reroll. scrap=%.2f weapon=%d. Price curve was not written."), DemoScrap, ShownWeapon);
	ArmInputLock();
}

void UPixelShopWidget::MoveFocus(int32 Delta)
{
	if (Delta == 0)
	{
		return;
	}
	for (int32 Step = 0; Step < ShopSlotCount; ++Step)
	{
		FocusedIndex = (FocusedIndex + Delta + ShopSlotCount) % ShopSlotCount;
		if (!bSlotSold[FocusedIndex])
		{
			break;
		}
	}
	RefreshFocus();
}

void UPixelShopWidget::TryBuyFromRun(int32 Index)
{
	if (!bOpen || bInputLocked || Index < 0 || Index >= ShopSlotCount || !Offers.IsValidIndex(Index))
	{
		return;
	}
	if (bSlotSold[Index])
	{
		return;
	}
	int32 Debit = 0;
	const bool bPriced = TryRunDebit(Offers[Index].Price, Debit);
	if (!PullRunScrap() || !bPriced || RunScrapShown < Debit)
	{
		RefreshAll();
		BeginNudge(Index);
		UE_LOG(LogPixelShopUI, Log, TEXT("Shop buy greyed out. slot=%d price=%.2f debit=%d runScrap=%d"),
			Index, Offers[Index].Price, bPriced ? Debit : -1, RunScrapShown);
		return;
	}
	UEventSubsystem* Events = UEventSubsystem::Get(this);
	if (!Events)
	{
		BeginNudge(Index);
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop run buy skipped. slot=%d UEventSubsystem is missing."), Index);
		return;
	}
	bInputLocked = true;
	const FPixelShopSlotOffer Bought = Offers[Index];
	Events->AddRunScrap(-Debit);
	RunScrapShown = Events->GetRunScrap();
	bSlotSold[Index] = true;
	if (Index == FocusedIndex)
	{
		MoveFocus(1);
	}
	RefreshAll();
	OnBuyItem.Broadcast(Index, Bought.ItemId);
	UE_LOG(LogPixelShopUI, Log,
		TEXT("Shop run buy slot=%d id=%s debit=%d runScrap=%d. AddRunScrap then GetRunScrap. No grant, no heal, no DA write."),
		Index, *Bought.ItemId.ToString(), Debit, RunScrapShown);
	ArmInputLock();
	if (Bought.OnPurchase == EPixelShopOnPurchase::OpenAffixChoiceImmediate)
	{
		CloseShop();
		OnRequestAffixChoiceFromVoucher.Broadcast();
	}
}

void UPixelShopWidget::TryRerollFromRun()
{
	if (!bOpen || bInputLocked)
	{
		return;
	}
	if (!PullRunScrap())
	{
		RefreshAll();
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop run reroll skipped. UEventSubsystem is missing."));
		return;
	}
	RefreshAll();
	if (!CanReroll())
	{
		return;
	}
	int32 Debit = 0;
	if (!TryRunDebit(ShopRerollCost, Debit))
	{
		return;
	}
	UEventSubsystem* Events = UEventSubsystem::Get(this);
	if (!Events)
	{
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop run reroll skipped. UEventSubsystem is missing."));
		return;
	}
	bInputLocked = true;
	Events->AddRunScrap(-Debit);
	RunScrapShown = Events->GetRunScrap();
	RefreshAll();
	OnReroll.Broadcast();
	UE_LOG(LogPixelShopUI, Log, TEXT("Shop run reroll. debit=%d runScrap=%d. Price curve was not written."), Debit, RunScrapShown);
	ArmInputLock();
}

bool UPixelShopWidget::PullRunScrap()
{
	UEventSubsystem* Events = UEventSubsystem::Get(this);
	if (!Events)
	{
		return false;
	}
	RunScrapShown = Events->GetRunScrap();
	return true;
}

bool UPixelShopWidget::CanReroll() const
{
	if (!CanAfford(ShopRerollCost))
	{
		return false;
	}
	for (int32 Index = 0; Index < ShopSlotCount; ++Index)
	{
		if (!bSlotSold[Index])
		{
			return true;
		}
	}
	return false;
}

bool UPixelShopWidget::CanAfford(double Price) const
{
	if (bRunWallet)
	{
		int32 Debit = 0;
		if (!TryRunDebit(Price, Debit))
		{
			return false;
		}
		return RunScrapShown >= Debit;
	}
	return Price >= 0.0 && DemoScrap + KINDA_SMALL_NUMBER >= Price;
}

bool UPixelShopWidget::IsAffixChoicePending() const
{
	if (const UAffixSubsystem* Affix = FindAffix())
	{
		return Affix->IsChoicePending();
	}
	return false;
}

UAffixSubsystem* UPixelShopWidget::FindAffix() const
{
	return UAffixSubsystemDefault::Get(this);
}

void UPixelShopWidget::BeginDemoPause()
{
	if (bOwnsPause || IsAffixChoicePending())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (World->IsPaused())
	{
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop opened while the world was already paused. Close will not unpause it."));
		return;
	}
	UGameplayStatics::SetGamePaused(World, true);
	bOwnsPause = true;
}

void UPixelShopWidget::EndDemoPause()
{
	if (!bOwnsPause)
	{
		return;
	}
	bOwnsPause = false;
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || IsAffixChoicePending())
	{
		return;
	}
	UGameplayStatics::SetGamePaused(World, false);
}

void UPixelShopWidget::AcquireInput()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		UE_LOG(LogPixelShopUI, Warning, TEXT("Shop has no owning player. Create it with a PlayerController."));
		return;
	}
	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(TakeWidget());
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
	bCapturedInput = true;
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetKeyboardFocus(TakeWidget());
	}
}

void UPixelShopWidget::ReleaseInput()
{
	if (!bCapturedInput)
	{
		return;
	}
	bCapturedInput = false;
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	FInputModeGameOnly GameOnly;
	PC->SetInputMode(GameOnly);
	PC->SetShowMouseCursor(false);
}

void UPixelShopWidget::BeginNudge(int32 Index)
{
	EndNudge();
	NudgeSlot = Index;
	NudgeAge = 0.f;
	if (UPixelShopSlotWidget* Slot = SlotAt(Index))
	{
		Slot->SetNudgeX(2.f);
	}
	NudgeTicker = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPixelShopWidget::AdvanceNudge), 0.f);
}

void UPixelShopWidget::EndNudge()
{
	if (NudgeTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(NudgeTicker);
		NudgeTicker.Reset();
	}
	if (UPixelShopSlotWidget* Slot = SlotAt(NudgeSlot))
	{
		Slot->SetNudgeX(0.f);
	}
	NudgeSlot = INDEX_NONE;
	NudgeAge = 0.f;
}

bool UPixelShopWidget::AdvanceNudge(float DeltaSeconds)
{
	NudgeAge += DeltaSeconds;
	if (NudgeAge >= 0.12f || NudgeSlot == INDEX_NONE)
	{
		if (UPixelShopSlotWidget* Slot = SlotAt(NudgeSlot))
		{
			Slot->SetNudgeX(0.f);
		}
		NudgeSlot = INDEX_NONE;
		NudgeTicker.Reset();
		return false;
	}
	const float X = (NudgeAge < 0.04f || NudgeAge >= 0.08f) ? 2.f : -2.f;
	if (UPixelShopSlotWidget* Slot = SlotAt(NudgeSlot))
	{
		Slot->SetNudgeX(X);
	}
	return true;
}

void UPixelShopWidget::ArmInputLock()
{
	bInputLocked = true;
	if (InputLockTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(InputLockTicker);
		InputLockTicker.Reset();
	}
	InputLockTicker = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UPixelShopWidget::FinishInputLock), 0.1f);
}

void UPixelShopWidget::ReleaseInputLock()
{
	bInputLocked = false;
	if (InputLockTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(InputLockTicker);
		InputLockTicker.Reset();
	}
}

bool UPixelShopWidget::FinishInputLock(float DeltaSeconds)
{
	(void)DeltaSeconds;
	bInputLocked = false;
	InputLockTicker.Reset();
	return false;
}

UPixelShopSlotWidget* UPixelShopWidget::SlotAt(int32 Index) const
{
	switch (Index)
	{
	case 0: return Slot_1;
	case 1: return Slot_2;
	case 2: return Slot_3;
	default: return nullptr;
	}
}
