#!/usr/bin/env python3
"""Erzeugt native/enums_1_5_27.inc aus der Il2CppDumper-Ausgabe dump.cs.

  gen_enums.py <dump.cs> <out.inc>

Je Enum-Wert eine Zeile COTL_ENUM(Kurzname, Wert, "Bezeichner"). Die Bezeichner sind Code-Namen
des Spiels (für I2-Begriffe wie "Traits/{0}"), keine Spieltexte.
"""
import re
import sys

# dump.cs-Name → Kurzname im C++-Code
ENUMS = {
    "FollowerTrait.TraitType": "TraitType",
    "InventoryItem.ITEM_TYPE": "ItemType",
    "TarotCards.Card": "TarotCard",
    "EquipmentType": "EquipmentType",
    "FollowerRole": "FollowerRole",
    "Thought": "Thought",
    "RelicType": "RelicType",
    "Objectives.TYPES": "ObjectiveType",
    "Objectives.CustomQuestTypes": "CustomQuest",
    "StructureBrain.TYPES": "StructureType",
    "UpgradeSystem.Type": "UpgradeType",
    "Enemy": "Enemy",
    "FollowerClothingType": "ClothingType",
    "FollowerLocation": "FollowerLocation",
}


def parse(dump_path):
    found = {short: [] for short in ENUMS.values()}
    current = None
    head = re.compile(r"^public enum (\S+) // TypeDefIndex")
    const = re.compile(r"^\tpublic const (\S+) (\w+) = (-?\d+);")
    with open(dump_path, encoding="utf-8") as f:
        for line in f:
            m = head.match(line)
            if m:
                current = ENUMS.get(m.group(1))
                continue
            if current is None:
                continue
            if line.startswith("}"):
                current = None
                continue
            m = const.match(line)
            if m:
                found[current].append((int(m.group(3)), m.group(2)))
    missing = [k for k, v in found.items() if not v]
    if missing:
        sys.exit(f"Enums nicht gefunden: {missing}")
    return found


def main():
    found = parse(sys.argv[1])
    with open(sys.argv[2], "w", encoding="utf-8", newline="\n") as out:
        out.write("// Generiert von tools/gen_enums.py aus dump.cs (Cult of the Lamb 1.5.27). Nicht bearbeiten.\n")
        for short in ENUMS.values():
            for value, ident in found[short]:
                out.write(f'COTL_ENUM({short}, {value}, "{ident}")\n')
    print({k: len(v) for k, v in found.items()})


if __name__ == "__main__":
    main()
