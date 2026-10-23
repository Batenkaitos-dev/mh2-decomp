#!/usr/bin/env python3
"""Where each byte of the 13 built files comes from, from the link maps.

    python3 tools/provenance.py [--check [--quiet]] [--list ORIGIN]

Run after the build (`ninja provenance` runs it). Every input section the link
places into a module (build/MODULE.map) is counted under its origin:

- code compiled from C: the .text of src/'s C files (Capcom's; zlib apart);
- data the compiler makes from the C: the data of src/'s objects whose values
  are written in C (pointer and function tables, TXT() texts, small constants)
  and MWCC's own (string literals, float constants, jump tables);
- data described in C, values from your copy: the objects whose initializer
  includes the content tools/content_gen.py writes from your copy
  (`#include "MODULE/ADDR.inc"`), and lbguild's described asset;
- assembly from the repository's source: src/main/mem_loops.s and the VU1
  microcode, src/sub_main/vu1_microcode.vsm;
- third-party libraries (Sony's SDK, the C and GCC runtimes, Metrowerks', CRI,
  the network libraries, ACCESS's engine), code and data, from your copy;
- DNAS's overlays and its authentication data (INCLUDE_DATA), from your copy;
- headers and padding: the ELF header and tail, the overlay headers, and the
  zero padding before each overlay's code, from your copy.

.bss takes no bytes in the files and is not counted. The linker's alignment
fill is the rest of each file. --check fails if any code (.text) comes from
anything but src/'s C files, the repository's own assembly or a labelled
library piece (its YAML name under sdk/, gcc/, mw/, net/, cri/ or access/, or
a DNAS overlay): every build runs it after the links (`--check --quiet`, which
prints nothing when it passes).
"""
from __future__ import annotations

import argparse
import collections
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MODULES = ("main", "sub_main", "game", "lobby", "select", "plsel", "gm_sub", "yn", "nethttp", "lbguild",
           "dnas_ins", "dnas_net", "test")
INPUT = re.compile(r"^ (\.\w+)\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)$")
SYMBOL = re.compile(r"^\s+0x[0-9a-f]+\s+([A-Za-z_@.$][\w@.$]*)\s*$")
CONTENT_DEF = re.compile(r'\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*=\s*\{\s*\n\s*#include\s+"\w+/[0-9A-F]+\.inc"')
SPLICE = re.compile(r'^\s*INCLUDE_DATA\(\s*"[^"]+"\s*,\s*(\w+)\s*\)', re.M)
LIBRARY = ("sdk/", "gcc/", "mw/", "net/", "cri/", "access/")
ZLIB_HINT = re.compile(r"zlib", re.I)

ORIGINS = [
    ("code", "Capcom's code compiled from C", "no"),
    ("zlib", "zlib 1.1.4 (third-party), compiled from its decompiled C", "no"),
    ("access", "ACCESS's kana-kanji engine (third-party), compiled from its decompiled C", "no"),
    ("cdata", "Data the compiler makes from the C (tables written in C, literals, jump tables)", "no"),
    ("asm", "Assembly from the repository's source (mem_loops.s, the VU1 microcode)", "no"),
    ("content", "Capcom's data described in C, values from your copy", "by policy: the game's content"),
    ("lib", "Third-party libraries, code and data, from your copy", "yes: not decompiled"),
    ("dnas", "DNAS: its overlays and its authentication data, from your copy", "yes"),
    ("hdr", "Headers and padding, from your copy", "no: the link could write them"),
    ("other", "Other assembly pieces, from your copy", "?"),
]


# The modules whose data outside C is all the libraries' (every object of
# Capcom's there is C): main (Sony's SDK, the C runtime, CRI), yn (netcnfif),
# nethttp (libhttps and its SSL). A piece elsewhere is listed as "other".
LIB_DATA_MODULES = ("main", "yn", "nethttp")


def third_party_units() -> dict[str, str]:
    """src/ units that are third-party C: {unit: zlib or access}, from
    tools/configure.py's own rule (unit_kind)."""
    sys.path.insert(0, str(ROOT / "tools"))
    import configure                                                 # noqa: E402
    out = {}
    for f in (ROOT / "src").glob("*/*.c"):
        kind = configure.unit_kind(f"{f.parent.name}/{f.stem}")
        if kind in ("origin/zlib", "origin/access"):
            out[f"{f.parent.name}/{f.stem}"] = kind.split("/")[1]
    return out


def zero_only(path: Path, kind: str) -> bool:
    """True when the object's sections of that kind hold only zero bytes (counted, not read out)."""
    import struct
    d = path.read_bytes()
    shoff, = struct.unpack_from("<I", d, 0x20)
    ent, num, stri = struct.unpack_from("<HHH", d, 0x2E)
    sh = [struct.unpack_from("<IIIIIIIIII", d, shoff + i * ent) for i in range(num)]
    so = sh[stri][4]
    for s in sh:
        name = d[so + s[0]:d.index(b"\0", so + s[0])].decode()
        if name == kind and s[1] != 8 and any(d[s[4]:s[4] + s[5]]):
            return False
    return True


def source_objects(module: str, stem: str, cache: dict) -> tuple[set, dict]:
    """(content objects, INCLUDE_DATA objects) of src/MODULE/STEM.c"""
    key = (module, stem)
    if key not in cache:
        p = ROOT / "src" / module / f"{stem}.c"
        text = p.read_text(encoding="utf-8", errors="replace") if p.exists() else ""
        cache[key] = (set(CONTENT_DEF.findall(text)), set(SPLICE.findall(text)))
    return cache[key]


def classify(module: str, kind: str, path: str, names: list[str], cache: dict, third: dict) -> str:
    rel = path.split(f"build/{module}/", 1)[-1]
    if module in ("dnas_ins", "dnas_net"):
        return "dnas"
    if rel.startswith("assets/") and module != "lbguild":
        return "hdr"
    if (module == "lbguild" and not rel.startswith("asm/")) or rel.startswith("assets/lbguild"):
        return "content"
    if rel.startswith("src/"):
        m = re.match(r"src/(\w+)/(.+)\.o$", rel)
        unit = f"{m.group(1)}/{m.group(2)}" if m else ""
        if kind == ".text":
            return third.get(unit, "code")
        content, spliced = source_objects(m.group(1), m.group(2), cache) if m else (set(), set())
        for n in names:
            if n in spliced:
                return "dnas"
            if n in content:
                return "content"
        return third.get(unit, "cdata")
    if rel.startswith("asm/"):
        piece = rel.split("/", 2)[-1]
        if piece.removeprefix("data/").startswith(LIBRARY):
            return "lib"
        if piece in ("mem_loops.o",) or "vu1_microcode" in piece:
            return "asm"
        if piece in ("text.o", "data/text.o") or zero_only(ROOT / path, kind):
            return "hdr"
        if piece.startswith("data/") and module in LIB_DATA_MODULES:
            return "lib"
        return "other"
    return "other"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--list", metavar="ORIGIN")
    ap.add_argument("--quiet", action="store_true", help="with --check: print only a failure")
    a = ap.parse_args()
    third = third_party_units()
    cache: dict = {}
    total = collections.Counter()
    bad = []
    listing = []
    for module in MODULES:
        mp = ROOT / "build" / f"{module}.map"
        if not mp.exists():
            sys.exit(f"provenance: {mp} missing: build first")
        cur = None
        for line in mp.read_text(encoding="utf-8", errors="replace").splitlines():
            m = INPUT.match(line)
            if m:
                kind, size, path = m.group(1), int(m.group(3), 16), m.group(4)
                cur = [kind, size, path, []] if size and kind in (".text", ".data", ".rodata") else None
                if cur:
                    listing.append((module, cur))
                continue
            if cur is not None:
                s = SYMBOL.match(line)
                if s:
                    cur[3].append(s.group(1))
        for mod, cur in [x for x in listing if x[0] == module]:
            kind, size, path, names = cur
            o = classify(module, kind, path, names, cache, third)
            cur.append(o)
            total[o] += size
            if kind == ".text" and o not in ("code", "zlib", "access", "asm", "lib", "dnas", "hdr"):
                bad.append(f"{module}: {path} ({kind}, {size} bytes)")
    placed = sum(total.values())
    if a.check and a.quiet:
        if bad:
            print("provenance: code from an unlabelled source:", *bad, sep="\n  ")
            return 1
        return 0
    print(f"{'Origin':<84} {'Bytes':>11} {'Share':>7}  From your copy?")
    for key, label, needed in ORIGINS:
        if total[key]:
            print(f"{label:<84} {total[key]:>11,} {100 * total[key] / placed:6.1f}%  {needed}")
    print(f"{'Placed by the link (13 files, .bss aside)':<84} {placed:>11,} {100.0:6.1f}%")
    if a.list:
        for module, cur in listing:
            if cur[-1] == a.list:
                print(f"  {module:<9} {cur[0]:<8} {cur[1]:>9,}  {cur[2]}  {' '.join(cur[3][:3])}")
    if a.check:
        if bad:
            print("provenance: code from an unlabelled source:", *bad, sep="\n  ")
            return 1
        print("provenance: every byte of code comes from src/, the repository's assembly or a labelled library piece")
    return 0


if __name__ == "__main__":
    sys.exit(main())
