#!/usr/bin/env python3
"""Validate shop DataAsset JSON/CSV against RoguelikeMeta §5 and Room_Shop_01.

JSON under Content/Data/Shop is the design entry. No uasset. No jsonschema dependency.
contractVersion 3 matches the Affix / Economy formal lock. There is no shopContractVersion.
"""
from __future__ import annotations

import csv
import json
import re
import sys
from decimal import Decimal
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SHOP = ROOT / "Content/Data/Shop"
GEOM = ROOT / "Content/Data/Rooms/Room_Shop_01_geometry.csv"

EXPECTED_FILES = {
    "SCHEMA_ShopInventory.json",
    "SCHEMA_ShopPriceCurve.json",
    "DA_ShopInventory.json",
    "DA_ShopInventory.csv",
    "DA_ShopPriceCurve.json",
    "DA_ShopPriceCurve.csv",
}

ANCHORS = {
    "ShopKeep": (400, 0, "NPCAnchor"),
    "Slot_1": (720, 0, "ShopSlot"),
    "Slot_2": (840, 0, "ShopSlot"),
    "Slot_3": (960, 0, "ShopSlot"),
    "LeaveHint": (1100, 0, "UIHint"),
}

SLOT_LOCK = {
    "Slot_1": {
        "kind": "WeaponContract",
        "displayName": "武器临时合约",
        "effectScope": "Run",
        "offerMode": "PickOneFromPool",
        "poolId": "WeaponContractPool",
        "offerId": None,
        "priceKey": None,
        "onPurchase": "GrantRunWeapon",
        "healFractionOfMaxHp": None,
        "choiceCount": None,
        "anchor": (720, 0),
        "description_has": ("本局", "不写权重"),
    },
    "Slot_2": {
        "kind": "Heal",
        "displayName": "回复",
        "effectScope": "Immediate",
        "offerMode": "Fixed",
        "poolId": None,
        "offerId": "HealMaxHp",
        "priceKey": "Heal",
        "onPurchase": "HealFractionOfMaxHp",
        "healFractionOfMaxHp": Decimal("0.4"),
        "choiceCount": None,
        "anchor": (840, 0),
        "description_has": ("40%",),
    },
    "Slot_3": {
        "kind": "AffixVoucher",
        "displayName": "三选一券",
        "effectScope": "Immediate",
        "offerMode": "Fixed",
        "poolId": None,
        "offerId": "AffixChoiceVoucher",
        "priceKey": "AffixVoucher",
        "onPurchase": "OpenAffixChoiceImmediate",
        "healFractionOfMaxHp": None,
        "choiceCount": 3,
        "anchor": (960, 0),
        "description_has": ("三选一",),
    },
}

WEAPONS = [
    ("Peashooter", "DA_Weapon_Peashooter", "豆射手", "Weapon_Peashooter", "WeaponContract_Peashooter", None),
    ("Scattergun", "DA_Weapon_Scattergun", "散射", "Weapon_Scattergun", "WeaponContract_Scattergun", None),
    ("RailPierce", "DA_Weapon_RailPierce", "穿透轨", "Weapon_RailPierce", "WeaponContract_RailPierce", "Rail"),
]

# priceKey, offerId, slotId, weaponId, docAlias, baseScrap
PRICES = [
    ("Heal", "HealMaxHp", "Slot_2", None, None, 30),
    ("AffixVoucher", "AffixChoiceVoucher", "Slot_3", None, None, 60),
    ("Weapon_Peashooter", "WeaponContract_Peashooter", "Slot_1", "Peashooter", None, 45),
    ("Weapon_Scattergun", "WeaponContract_Scattergun", "Slot_1", "Scattergun", None, 60),
    ("Weapon_RailPierce", "WeaponContract_RailPierce", "Slot_1", "RailPierce", "Rail", 80),
]

LAYERS = [(1, Decimal("1")), (2, Decimal("1.15"))]
LEGAL_WEAPON_IDS = {"Peashooter", "Scattergun", "RailPierce"}


def dec_of(value: object) -> Decimal:
    if isinstance(value, bool) or not isinstance(value, (int, float, str)):
        raise TypeError(type(value).__name__)
    return Decimal(str(value))


def load_json(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def resolve(schema: dict, root: dict) -> dict:
    if "$ref" in schema:
        ref = schema["$ref"]
        if not ref.startswith("#/$defs/"):
            raise ValueError(ref)
        return root["$defs"][ref.split("/")[-1]]
    return schema


def type_ok(value: object, type_name: str) -> bool:
    if type_name == "null":
        return value is None
    if type_name == "string":
        return isinstance(value, str)
    if type_name == "boolean":
        return isinstance(value, bool)
    if type_name == "integer":
        return isinstance(value, int) and not isinstance(value, bool)
    if type_name == "number":
        return isinstance(value, (int, float)) and not isinstance(value, bool)
    if type_name == "array":
        return isinstance(value, list)
    if type_name == "object":
        return isinstance(value, dict)
    return False


def check_schema(value: object, schema: dict, root: dict, path: str, errors: list[str]) -> None:
    schema = resolve(schema, root)
    declared = schema.get("type")
    if isinstance(declared, list):
        if not any(type_ok(value, name) for name in declared):
            errors.append(f"{path}: type {type(value).__name__} not in {declared}")
            return
    elif isinstance(declared, str) and not type_ok(value, declared):
        errors.append(f"{path}: type {type(value).__name__} != {declared}")
        return

    if "enum" in schema and value not in schema["enum"]:
        errors.append(f"{path}: {value!r} not in {schema['enum']}")
    if "const" in schema and value != schema["const"]:
        errors.append(f"{path}: {value!r} != const {schema['const']!r}")
    if value is None:
        return

    if isinstance(value, str):
        if "minLength" in schema and len(value) < schema["minLength"]:
            errors.append(f"{path}: shorter than {schema['minLength']}")
        if "pattern" in schema and re.fullmatch(schema["pattern"], value) is None:
            errors.append(f"{path}: {value!r} fails {schema['pattern']}")

    if isinstance(value, (int, float)) and not isinstance(value, bool):
        if "minimum" in schema and value < schema["minimum"]:
            errors.append(f"{path}: {value} < {schema['minimum']}")
        if "maximum" in schema and value > schema["maximum"]:
            errors.append(f"{path}: {value} > {schema['maximum']}")
        if "exclusiveMinimum" in schema and value <= schema["exclusiveMinimum"]:
            errors.append(f"{path}: {value} <= {schema['exclusiveMinimum']}")

    if isinstance(value, list):
        if "minItems" in schema and len(value) < schema["minItems"]:
            errors.append(f"{path}: len {len(value)} < {schema['minItems']}")
        if "maxItems" in schema and len(value) > schema["maxItems"]:
            errors.append(f"{path}: len {len(value)} > {schema['maxItems']}")
        item_schema = schema.get("items")
        if isinstance(item_schema, dict):
            for index, item in enumerate(value):
                check_schema(item, item_schema, root, f"{path}[{index}]", errors)

    if isinstance(value, dict):
        props = schema.get("properties", {})
        if schema.get("additionalProperties") is False:
            for key in value:
                if key not in props:
                    errors.append(f"{path}: unknown field {key}")
        for key in schema.get("required", []):
            if key not in value:
                errors.append(f"{path}: missing {key}")
        for key, item in value.items():
            if key in props:
                check_schema(item, props[key], root, f"{path}.{key}", errors)


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle))


def check_files(errors: list[str]) -> None:
    found = {path.name for path in SHOP.iterdir() if path.is_file()}
    if found != EXPECTED_FILES:
        errors.append(f"Content/Data/Shop files {sorted(found)} != {sorted(EXPECTED_FILES)}")
    for path in SHOP.rglob("*"):
        if path.suffix.lower() == ".uasset":
            errors.append(f"placeholder uasset forbidden: {path}")


def check_geometry(errors: list[str]) -> None:
    rows = {row["id"]: row for row in read_csv(GEOM)}
    for anchor_id, (x, y, kind) in ANCHORS.items():
        row = rows.get(anchor_id)
        if row is None:
            errors.append(f"geometry missing {anchor_id}")
            continue
        if (row.get("x0"), row.get("x1"), row.get("kind")) != (str(x), str(y), kind):
            errors.append(
                f"geometry {anchor_id} is ({row.get('x0')},{row.get('x1')},{row.get('kind')})"
                f" != ({x},{y},{kind})"
            )


def check_inventory(data: dict, errors: list[str]) -> None:
    if data.get("contractVersion") != 3:
        errors.append("inventory contractVersion must be 3")
    for token in ("禁止", "uasset", "Scrap", "Core"):
        if token not in data.get("note", ""):
            errors.append(f"inventory note missing {token}")
    if "Core" in (data.get("currencyId"), data.get("currencyScope")):
        errors.append("inventory currency must be run Scrap, not Core")

    anchors = data.get("roomAnchors", [])
    if [row.get("id") for row in anchors] != list(ANCHORS):
        errors.append(f"roomAnchors order {[row.get('id') for row in anchors]}")
    for row in anchors:
        expected = ANCHORS.get(row.get("id"))
        if expected and (row.get("x"), row.get("y"), row.get("kind")) != expected:
            errors.append(f"anchor {row.get('id')} drifted from Room_Shop_01")

    slots = data.get("slots", [])
    if [row.get("slotId") for row in slots] != ["Slot_1", "Slot_2", "Slot_3"]:
        errors.append("slots must be Slot_1, Slot_2, Slot_3 in that order")
    for row in slots:
        lock = SLOT_LOCK.get(row.get("slotId"))
        if lock is None:
            continue
        for key, expected in lock.items():
            if key == "description_has":
                text = row.get("description", "")
                for piece in expected:
                    if piece not in text:
                        errors.append(f"{row.get('slotId')} description missing {piece}")
                continue
            if key == "anchor":
                if (row.get("anchorX"), row.get("anchorY")) != expected:
                    errors.append(f"{row.get('slotId')} anchor != {expected}")
                continue
            if key == "healFractionOfMaxHp":
                actual = None if row.get(key) is None else dec_of(row.get(key))
                if actual != expected:
                    errors.append(f"{row.get('slotId')} healFraction {actual} != {expected}")
                continue
            if row.get(key) != expected:
                errors.append(f"{row.get('slotId')} {key}={row.get(key)!r} != {expected!r}")

    pool = data.get("weaponContractPool", [])
    if [row.get("weaponId") for row in pool] != [item[0] for item in WEAPONS]:
        errors.append("weapon pool must be Peashooter, Scattergun, RailPierce")
    seen: set[str] = set()
    for row, expected in zip(pool, WEAPONS):
        weapon_id, asset, display, price_key, offer_id, alias = expected
        seen.add(weapon_id)
        if weapon_id not in LEGAL_WEAPON_IDS:
            errors.append(f"illegal weaponId {weapon_id}")
        if row.get("dataAssetName") != asset:
            errors.append(f"{weapon_id} dataAssetName must be {asset}")
        if row.get("displayName") != display:
            errors.append(f"{weapon_id} displayName must be {display}")
        if row.get("priceKey") != price_key or row.get("offerId") != offer_id:
            errors.append(f"{weapon_id} price/offer id drifted")
        if row.get("effectScope") != "Run" or row.get("docAlias") != alias:
            errors.append(f"{weapon_id} scope/alias drifted")
    if seen != LEGAL_WEAPON_IDS:
        errors.append(f"weapon ids {seen} != {LEGAL_WEAPON_IDS}")


def check_curve(inventory: dict, curve: dict, errors: list[str]) -> None:
    if curve.get("contractVersion") != 3:
        errors.append("price curve contractVersion must be 3")
    if curve.get("currencyId") != "Scrap" or curve.get("currencyScope") != "Run":
        errors.append("price curve currency must be run Scrap")
    note = curve.get("note", "")
    for token in ("1.15", "Scrap", "Core"):
        if token not in note:
            errors.append(f"price curve note missing {token}")

    layers = curve.get("layers", [])
    parsed = [(row.get("layer"), dec_of(row.get("multiplier"))) for row in layers]
    if parsed != LAYERS:
        errors.append(f"layers {parsed} != {LAYERS}")

    prices = curve.get("prices", [])
    if [row.get("priceKey") for row in prices] != [item[0] for item in PRICES]:
        errors.append("price key order drifted")
    by_key = {}
    for row, expected in zip(prices, PRICES):
        price_key, offer_id, slot_id, weapon_id, alias, base = expected
        by_key[price_key] = row
        if row.get("offerId") != offer_id or row.get("slotId") != slot_id:
            errors.append(f"{price_key} offer/slot drifted")
        if row.get("weaponId") != weapon_id or row.get("docAlias") != alias:
            errors.append(f"{price_key} weapon/alias drifted")
        if row.get("baseScrap") != base:
            errors.append(f"{price_key} baseScrap {row.get('baseScrap')} != {base}")
        if weapon_id is not None and weapon_id not in LEGAL_WEAPON_IDS:
            errors.append(f"{price_key} illegal weaponId")

    referenced = set()
    for slot in inventory.get("slots", []):
        if slot.get("priceKey"):
            referenced.add(slot["priceKey"])
    for weapon in inventory.get("weaponContractPool", []):
        referenced.add(weapon.get("priceKey"))
    if referenced != set(by_key):
        errors.append(f"inventory price keys {sorted(referenced)} != curve {sorted(by_key)}")
    for weapon in inventory.get("weaponContractPool", []):
        price = by_key.get(weapon.get("priceKey"))
        if price and price.get("offerId") != weapon.get("offerId"):
            errors.append(f"{weapon.get('weaponId')} price offerId mismatch")
        if price and price.get("weaponId") != weapon.get("weaponId"):
            errors.append(f"{weapon.get('weaponId')} curve weaponId mismatch")


def fmt_decimal(value: Decimal) -> str:
    text = format(value, "f")
    if "." in text:
        text = text.rstrip("0").rstrip(".")
    return text


def check_csvs(inventory: dict, curve: dict, errors: list[str]) -> None:
    inv_rows = read_csv(SHOP / "DA_ShopInventory.csv")
    expected_inv = []
    slot_by_id = {row["slotId"]: row for row in inventory["slots"]}
    for weapon in inventory["weaponContractPool"]:
        slot = slot_by_id["Slot_1"]
        expected_inv.append(
            {
                "contract_version": "3",
                "currency_id": "Scrap",
                "currency_scope": "Run",
                "slot_id": "Slot_1",
                "slot_kind": "WeaponContract",
                "slot_display_name": slot["displayName"],
                "offer_id": weapon["offerId"],
                "weapon_id": weapon["weaponId"],
                "data_asset_name": weapon["dataAssetName"],
                "offer_display_name": weapon["displayName"],
                "price_key": weapon["priceKey"],
                "heal_fraction_of_max_hp": "",
                "effect_scope": "Run",
                "on_purchase": "GrantRunWeapon",
                "choice_count": "",
                "anchor_x": "720",
                "anchor_y": "0",
                "doc_alias": weapon["docAlias"] or "",
            }
        )
    for slot_id in ("Slot_2", "Slot_3"):
        slot = slot_by_id[slot_id]
        heal = slot["healFractionOfMaxHp"]
        choice = slot["choiceCount"]
        expected_inv.append(
            {
                "contract_version": "3",
                "currency_id": "Scrap",
                "currency_scope": "Run",
                "slot_id": slot_id,
                "slot_kind": slot["kind"],
                "slot_display_name": slot["displayName"],
                "offer_id": slot["offerId"] or "",
                "weapon_id": "",
                "data_asset_name": "",
                "offer_display_name": slot["displayName"],
                "price_key": slot["priceKey"] or "",
                "heal_fraction_of_max_hp": "" if heal is None else fmt_decimal(dec_of(heal)),
                "effect_scope": slot["effectScope"],
                "on_purchase": slot["onPurchase"],
                "choice_count": "" if choice is None else str(choice),
                "anchor_x": str(slot["anchorX"]),
                "anchor_y": str(slot["anchorY"]),
                "doc_alias": "",
            }
        )
    if inv_rows != expected_inv:
        errors.append("DA_ShopInventory.csv is not a mirror of DA_ShopInventory.json")

    price_rows = read_csv(SHOP / "DA_ShopPriceCurve.csv")
    expected_prices = []
    for price in curve["prices"]:
        for layer in curve["layers"]:
            resolved = dec_of(price["baseScrap"]) * dec_of(layer["multiplier"])
            expected_prices.append(
                {
                    "contract_version": "3",
                    "currency_id": "Scrap",
                    "currency_scope": "Run",
                    "price_key": price["priceKey"],
                    "offer_id": price["offerId"],
                    "slot_id": price["slotId"],
                    "weapon_id": price["weaponId"] or "",
                    "doc_alias": price["docAlias"] or "",
                    "base_scrap": str(price["baseScrap"]),
                    "layer": str(layer["layer"]),
                    "multiplier": fmt_decimal(dec_of(layer["multiplier"])),
                    "resolved_scrap": fmt_decimal(resolved),
                }
            )
    if price_rows != expected_prices:
        errors.append("DA_ShopPriceCurve.csv is not baseScrap * layerMultiplier")
    for row in price_rows:
        product = dec_of(row["base_scrap"]) * dec_of(row["multiplier"])
        if dec_of(row["resolved_scrap"]) != product:
            errors.append(f"{row['price_key']} layer {row['layer']} resolved != base * multiplier")


def layer2_summary(curve: dict) -> str:
    parts = []
    for price in curve["prices"]:
        resolved = dec_of(price["baseScrap"]) * Decimal("1.15")
        parts.append(f"{price['priceKey']}:{fmt_decimal(resolved)}")
    return ",".join(parts)


def main() -> int:
    errors: list[str] = []
    check_files(errors)
    check_geometry(errors)

    inv_schema = load_json(SHOP / "SCHEMA_ShopInventory.json")
    curve_schema = load_json(SHOP / "SCHEMA_ShopPriceCurve.json")
    inventory = load_json(SHOP / "DA_ShopInventory.json")
    curve = load_json(SHOP / "DA_ShopPriceCurve.json")

    for schema, title in (
        (inv_schema, "DA_ShopInventory"),
        (curve_schema, "DA_ShopPriceCurve"),
    ):
        if schema.get("title") != title or schema.get("contractVersion") != 3:
            errors.append(f"{title} schema title/contractVersion drifted")
        policy = schema.get("contractVersionPolicy", "")
        if "shopContractVersion" not in policy or "contractVersion=3" not in policy:
            errors.append(f"{title} schema must document contractVersion=3 and no shopContractVersion")

    if isinstance(inventory, dict) and isinstance(inv_schema, dict):
        check_schema(inventory, inv_schema, inv_schema, "DA_ShopInventory", errors)
        check_inventory(inventory, errors)
    else:
        errors.append("inventory JSON/schema must be objects")

    if isinstance(curve, dict) and isinstance(curve_schema, dict) and isinstance(inventory, dict):
        check_schema(curve, curve_schema, curve_schema, "DA_ShopPriceCurve", errors)
        check_curve(inventory, curve, errors)
        check_csvs(inventory, curve, errors)
    else:
        errors.append("price curve JSON/schema must be objects")

    if errors:
        print("FAIL")
        for err in errors:
            print(" -", err)
        return 1

    print(
        "OK: slots=3 weapons=Peashooter/Scattergun/RailPierce"
        f" base=30/60/45/60/80 layer2={layer2_summary(curve)} contractVersion=3"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
