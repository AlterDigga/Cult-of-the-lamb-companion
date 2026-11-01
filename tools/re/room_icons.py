#!/usr/bin/env python3
"""Erzeugt native/room_icons_1_5_27.inc: Raum-Prefab (BiomeRoom.GameObjectPath) → MapIcon-Sprite.

  room_icons.py <roomicons.txt> <pidscan.txt> > native/room_icons_1_5_27.inc

roomicons.txt: "<Prefab-Pfad>\t<FileID>\t<PathID>\t…" (GenerateRoom.MapIcon je Raum-Prefab),
pidscan.txt: "<Bundle>\t<PathID>\t<Sprite-Name>" (Sprites im MapIcons-Bundle). Siehe docs/re-notes.md.
"""
import sys

names = {}
for line in open(sys.argv[2], encoding="utf-8"):
    b, pid, name = line.rstrip("\n").split("\t")
    names[pid] = name
rows = {}
for line in open(sys.argv[1], encoding="utf-8"):
    path, fid, pid, _ = line.rstrip("\n").split("\t")
    if pid in names:
        rows[path] = names[pid]
print("// Generiert von tools/re/room_icons.py (Cult of the Lamb 1.5.27). Nicht bearbeiten.")
for path in sorted(rows):
    print(f'COTL_ROOMICON("{path}", "{rows[path]}")')
