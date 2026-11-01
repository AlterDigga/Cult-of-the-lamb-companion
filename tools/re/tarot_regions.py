#!/usr/bin/env python3
"""Erzeugt native/tarot_regions_1_5_27.inc: TarotCards.Card → Region der Kartenvorderseite.

  tarot_regions.py <skintable.txt> <card_skins.txt> > native/tarot_regions_1_5_27.inc

skintable.txt: Zeilen "<Card> <Name> \"<Skin>\"" (Sprungtabelle von TarotCards.Skin, siehe
docs/re-notes.md); card_skins.txt: Ausgabe von spine_skins.py für WeaponCard.skel.
"""
import sys

skins = {}
for line in open(sys.argv[2], encoding="utf-8"):
    parts = line.rstrip("\n").split("\t")
    for p in parts[1:]:
        slot, rest = p.split(":", 1)
        if slot == "Card Front":
            skins[parts[0]] = rest.split("=", 1)[1]

print("// Generiert von tools/re/tarot_regions.py (Cult of the Lamb 1.5.27). Nicht bearbeiten.")
for line in open(sys.argv[1], encoding="utf-8"):
    card, name, skin = line.split(None, 2)
    skin = skin.strip().strip('"')
    if skin in skins:
        print(f'COTL_TAROT({card}, "{skins[skin]}") // {name}')
