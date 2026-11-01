#!/usr/bin/env python3
"""Disassembliert IL2CPP-Methoden aus der entpackten main-NSO.

  disasm.py <exefs/main> <script.json> <Methodenname|0xRVA> [anzahl]

Methodenname wie in script.json, z. B. "HungerBar$$get_MAX_HUNGER" (Teilstring genügt, alle
Treffer werden ausgegeben). ADRP+ADD/LDR-Paare und BL-Ziele werden mit Symbolnamen annotiert.
Benötigt capstone und lz4 (siehe docs/re-notes.md, ohne sudo per apt-get download).
"""
import json
import os
import pickle
import struct
import sys

import capstone
import lz4.block


def load_nso(path):
    d = open(path, "rb").read()
    assert d[:4] == b"NSO0"
    flags = struct.unpack_from("<I", d, 0xC)[0]
    image = bytearray()
    for i, (hdr, csize_off) in enumerate([(0x10, 0x60), (0x20, 0x64), (0x30, 0x68)]):
        file_off, mem_off, size = struct.unpack_from("<III", d, hdr)
        csize = struct.unpack_from("<I", d, csize_off)[0]
        raw = d[file_off:file_off + csize]
        seg = lz4.block.decompress(raw, uncompressed_size=size) if flags & (1 << i) else raw[:size]
        if len(image) < mem_off + size:
            image.extend(b"\0" * (mem_off + size - len(image)))
        image[mem_off:mem_off + size] = seg
    return bytes(image)


def load_symbols(script_path):
    cache = script_path + ".symcache"
    if os.path.exists(cache) and os.path.getmtime(cache) > os.path.getmtime(script_path):
        return pickle.load(open(cache, "rb"))
    j = json.load(open(script_path))
    syms = {}
    for m in j.get("ScriptMethod", []):
        syms[m["Address"]] = m["Name"]
    for m in j.get("ScriptMetadata", []):
        syms.setdefault(m["Address"], m["Name"])
    for m in j.get("ScriptMetadataMethod", []):
        syms.setdefault(m["Address"], m["Name"])
    # String-Literale (StringLiteral-Slots), z. B. "Traits/"
    for m in j.get("ScriptString", []):
        syms.setdefault(m["Address"], '"' + m["Value"][:60] + '"')
    pickle.dump(syms, open(cache, "wb"))
    return syms


def disasm(image, syms, rva, count):
    md = capstone.Cs(capstone.CS_ARCH_ARM64, capstone.CS_MODE_ARM)
    md.detail = True
    adrp = {}
    for ins in md.disasm(image[rva:rva + count * 4], rva):
        note = ""
        ops = ins.operands
        if ins.mnemonic == "adrp":
            adrp[ops[0].reg] = ops[1].imm
        elif ins.mnemonic in ("add", "ldr", "str", "ldrb", "ldrsw", "ldp") and len(ops) >= 2:
            if ins.mnemonic == "add" and len(ops) == 3 and ops[1].reg in adrp and ops[2].type == capstone.arm64.ARM64_OP_IMM:
                tgt = adrp[ops[1].reg] + ops[2].imm
                note = f"-> 0x{tgt:X} {syms.get(tgt, '')}"
            elif ops[-1].type == capstone.arm64.ARM64_OP_MEM and ops[-1].mem.base in adrp:
                tgt = adrp[ops[-1].mem.base] + ops[-1].mem.disp
                val = struct.unpack_from("<Q", image, tgt)[0] if tgt + 8 <= len(image) else 0
                note = f"-> [0x{tgt:X}] {syms.get(tgt, '')} (=0x{val:X} {syms.get(val, '')})"
        elif ins.mnemonic in ("bl", "b") and ops and ops[0].type == capstone.arm64.ARM64_OP_IMM:
            note = f"-> {syms.get(ops[0].imm, '')}"
        # Ein überschriebenes Register ist keine ADRP-Basis mehr
        if ins.mnemonic != "adrp" and ops and ops[0].type == capstone.arm64.ARM64_OP_REG:
            adrp.pop(ops[0].reg, None)
        print(f"  0x{ins.address:08X}: {ins.mnemonic:8} {ins.op_str:40} {note}")
        if ins.mnemonic == "ret":
            break


def main():
    image = load_nso(sys.argv[1])
    syms = load_symbols(sys.argv[2])
    target = sys.argv[3]
    count = int(sys.argv[4]) if len(sys.argv) > 4 else 80
    if target.startswith("0x"):
        hits = [(int(target, 16), syms.get(int(target, 16), "?"))]
    else:
        hits = sorted((a, n) for a, n in syms.items() if target in n and "$$" in n)
    for addr, name in hits[:12]:
        print(f"{name} @ 0x{addr:X}")
        disasm(image, syms, addr, count)


if __name__ == "__main__":
    main()
