#!/usr/bin/env python3
"""Validate Affix MVP stub against SCHEMA (lightweight, no jsonschema dep)."""
from __future__ import annotations
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STUB = ROOT / "Content/Data/Affixes/DT_Affix_MVP.json"
_legacy = ROOT / "Content/Data/Affixes/DT_Affix_MVP_stub.json"  # deprecated
RARITIES = {"Common", "Rare", "Epic", "Legendary"}
STACKS = {"Unique", "AdditiveStack", "RefreshDuration"}
OPS = {"Add", "Mul", "Override"}

def main() -> int:
    data = json.loads(STUB.read_text(encoding="utf-8"))
    affixes = data.get("affixes", [])
    ids = set()
    errors = []
    for a in affixes:
        aid = a.get("AffixId")
        if not aid:
            errors.append("missing AffixId")
            continue
        if aid in ids:
            errors.append(f"duplicate AffixId: {aid}")
        ids.add(aid)
        if a.get("Rarity") not in RARITIES:
            errors.append(f"{aid}: bad Rarity")
        if a.get("StackRule") not in STACKS:
            errors.append(f"{aid}: bad StackRule")
        for m in a.get("Modifiers", []):
            if m.get("Op") not in OPS:
                errors.append(f"{aid}: bad Op {m.get('Op')}")
            if "Stat" not in m or "Value" not in m:
                errors.append(f"{aid}: modifier missing Stat/Value")
    if len(affixes) < 8:
        errors.append(f"MVP expects 8–12 affixes, got {len(affixes)}")

    stub = json.loads(_legacy.read_text(encoding="utf-8"))
    if stub.get("deprecated") is not True:
        errors.append("DT_Affix_MVP_stub.json must stay deprecated")
    if stub.get("contractVersion") != 3:
        errors.append("stub contractVersion drifted")

    p05_path = ROOT / "Content/Data/Affixes/DT_Affix_P05.json"
    p05 = json.loads(p05_path.read_text(encoding="utf-8"))
    if p05.get("contractVersion") != 3:
        errors.append("P05 contractVersion must be 3")
    p05_affixes = p05.get("affixes", [])
    if not (1 <= len(p05_affixes) <= 8):
        errors.append(f"P0.6 pool must be 1..8 affixes, got {len(p05_affixes)}")
    expected = [
        "RapidFire",
        "HollowPoint",
        "ThickSkin",
        "Magnet",
        "Piercer",
        "TwinShot",
        "CritOptics",
        "Scavenger",
    ]
    by_mvp = {a.get("AffixId"): a for a in affixes}
    seen = []
    for row in p05_affixes:
        aid = row.get("AffixId")
        seen.append(aid)
        if row.get("DataAssetName") != f"DA_Affix_{aid}":
            errors.append(f"{aid}: DataAssetName must be DA_Affix_{aid}")
        locked = by_mvp.get(aid)
        if locked is None:
            errors.append(f"{aid}: not in the contractVersion=3 lock table")
            continue
        for key in ("Rarity", "StackRule", "MaxStacks", "Weight", "DisplayName", "Description"):
            if row.get(key) != locked.get(key):
                errors.append(f"{aid}: {key} drifted from DT_Affix_MVP.json")
        if row.get("Modifiers") != locked.get("Modifiers"):
            errors.append(f"{aid}: Modifiers drifted from DT_Affix_MVP.json")
    if seen != expected:
        errors.append(f"P0.6 ids {seen} != {expected}")
    mul_stats = {
        (m.get("Stat"), m.get("Op"))
        for row in p05_affixes
        for m in row.get("Modifiers", [])
    }
    if ("FireRate", "Mul") not in mul_stats or ("Damage", "Mul") not in mul_stats:
        errors.append("P0.6 pool must keep FireRate and Damage Mul rows")

    if errors:
        print("FAIL")
        for e in errors:
            print(" -", e)
        return 1
    print(f"OK: mvp={len(affixes)} p05={len(p05_affixes)} ids={seen}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
