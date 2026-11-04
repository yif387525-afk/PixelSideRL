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
    if errors:
        print("FAIL")
        for e in errors:
            print(" -", e)
        return 1
    print(f"OK: {len(affixes)} affixes, ids={sorted(ids)}")
    return 0

if __name__ == "__main__":
    sys.exit(main())
