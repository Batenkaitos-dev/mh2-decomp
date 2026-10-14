#!/usr/bin/env python3
"""Measure how much of each module's initialized data C files own.

    python tools/data_progress.py                  # one line per module
    python tools/data_progress.py --markdown       # the same as a table
    python tools/data_progress.py --files game     # game's pieces, one per line

It reads the splat configs (config/main.yaml, config/overlays/*.yaml): a
subsegment typed `.data`, `.rodata`, `.sdata` or `.bss` is a C file's own
range ("Functions with switch tables or strings" in CONTRIBUTING.md);
a `data`/`rodata`/`bss` piece is still assembly. A piece runs to the next
piece's start (the last to the segment's end, or for .bss to bss_size).

Only Capcom's data counts. The libraries' data stays assembly (the owner's
rule), so it is left out:
- nethttp, dnas_ins and dnas_net are Sony's libhttps and DNAS: library, but
  for nethttp's Capcom front end (text_8FB130, unk_8FB130.c): its .rodata
  (0x954D50-0x954E60), its .bss (0x95AA00-0x95B130) and its .data
  (0x94FF98-0x950008) count as Capcom's;
- yn's first .data and .rodata pieces are Sony's netcnfif (yn.yaml);
- main's .data below 0x1BA110 and .rodata below 0x1C7B10 are the SDK's and
  CRI's: the lowest object that Capcom's code (0x179590 up, from dma) loads
  is at 0x1BA110 in .data (the libraries' highest is 0x1BA090) and
  0x1C7B70 in .rodata (the libraries' highest 0x1C7AD8; dma.c owns the
  .rodata from 0x1C7B10).
- sub_main's .data 0x403170-0x405178 (but for Capcom's decoration table,
  0x403DF0-0x4045C0, deco_table.c) and .rodata 0x46A3C0-0x46A450 are
  ACCESS's kana-kanji engine's (the pieces named access/);
- sub_main's .data 0x4054D0-0x4154E0 is the DNAS authentication data and
  its passphrase (INCLUDE_DATA objects of unk_308BC0.c).
lbguild is Capcom's data with no code, test an all-zero stub: they are
listed but not in the total.

The zero padding pieces (`text_end`, `rodata_end`) are not data. .bss is shown apart:
it has no bytes in the files, but a C file that owns it defines it.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parent.parent

MODULES = ["main", "sub_main", "game", "lobby", "select", "plsel", "gm_sub", "yn",
           "lbguild", "test", "nethttp", "dnas_ins", "dnas_net"]
LIBRARY_MODULES = {"nethttp", "dnas_ins", "dnas_net"}
UNCOUNTED = {"lbguild", "test"}
# (section, lo, hi) vram ranges of library data inside a Capcom module
LIBRARY_RANGES = {
    "main": [(".data", 0x195C00, 0x1BA110), (".rodata", 0x1BCE80, 0x1C7B10)],
    "yn": [(".data", 0xAD1180, 0xAD11A0), (".rodata", 0xAD2280, 0xAD2770)],
    # the DNAS authentication data (64K) and its passphrase, which unk_308BC0.c passes to the
    # DNAS library: INCLUDE_DATA objects, taken from the user's original at build time
    # ACCESS's kana-kanji engine (pieces named access/)
    "sub_main": [(".data", 0x403170, 0x403DF0), (".data", 0x4045C0, 0x405178),
                 (".rodata", 0x46A3C0, 0x46A450),
                 (".data", 0x4054D0, 0x4154E0)],
}
# (section, lo, hi) vram ranges of Capcom's data inside a library module
CAPCOM_RANGES = {
    "nethttp": [(".data", 0x94FF98, 0x950008), (".rodata", 0x954D50, 0x954E60),
                (".bss", 0x95AA00, 0x95B130)],
}
SECTIONS = [".data", ".rodata", ".sdata", ".bss"]


def config_path(module: str) -> Path:
    return ROOT / ("config/main.yaml" if module == "main" else f"config/overlays/{module}.yaml")


def piece_fields(sub) -> dict:
    if isinstance(sub, dict):
        return dict(sub)
    d = {"start": sub[0]}
    if len(sub) > 1:
        d["type"] = sub[1]
    if len(sub) > 2:
        d["name"] = sub[2]
    return d


def pieces(module: str) -> list[dict]:
    """Every data piece of the module's code segment: section, vram lo/hi, owner."""
    cfg = yaml.safe_load(config_path(module).read_text(encoding="utf-8"))
    segs = cfg["segments"]
    out: list[dict] = []
    for i, seg in enumerate(segs):
        if not isinstance(seg, dict) or seg.get("type") != "code":
            continue
        seg_end = segs[i + 1][0] if isinstance(segs[i + 1], list) else segs[i + 1]["start"]
        base = seg["vram"] - seg["start"]
        subs = [piece_fields(s) for s in seg["subsegments"]]
        bss_lo = None
        rows = []
        for j, s in enumerate(subs):
            typ = s.get("type", "")
            name = s.get("name", "")
            if typ in ("bss", ".bss"):
                vram = s["vram"]
                bss_lo = vram if bss_lo is None else bss_lo
                rows.append({"section": ".bss", "lo": vram, "c": typ == ".bss", "name": name})
                continue
            if typ in ("c", "asm", "hasm", "bin"):
                section = ".text"
            elif typ.startswith("."):
                section = typ
            elif typ in ("data", "rodata", "sdata"):
                section = s.get("linker_section_order", ".data" if typ == "data" else "." + typ)
            else:
                section = ".text"
            rows.append({"section": section, "lo": base + s["start"], "c": typ.startswith("."),
                         "name": name, "file_end": base + seg_end, "base": base})
        # ends: the next piece's start; the last file piece ends at the segment end,
        # the last .bss piece at bss_size
        file_rows = [r for r in rows if r["section"] != ".bss"]
        for a, b in zip(file_rows, file_rows[1:]):
            a["hi"] = b["lo"]
        if file_rows:
            file_rows[-1]["hi"] = file_rows[-1]["file_end"]
        bss_rows = [r for r in rows if r["section"] == ".bss"]
        for a, b in zip(bss_rows, bss_rows[1:]):
            a["hi"] = b["lo"]
        if bss_rows:
            bss_rows[-1]["hi"] = bss_lo + seg.get("bss_size", 0)
        image = None
        for r in rows:
            if r["section"] not in SECTIONS:
                continue
            if r["name"].endswith("_end") and r["section"] != ".bss":
                # the zero padding after a section's last object: not data
                if image is None:
                    image = (ROOT / "build/orig" / ("SLPM_662.80" if module == "main" else module + ".bin")).read_bytes()
                if not any(image[r["lo"] - r["base"]:r["hi"] - r["base"]]):
                    continue
            out.append(r)
    return out


def split_library(module: str, rows: list[dict]) -> list[dict]:
    """Cut the rows at the library ranges and mark the library parts."""
    out = []
    if module in LIBRARY_MODULES:
        # all library, except Capcom's ranges
        ranges, default, mark = CAPCOM_RANGES.get(module, []), True, False
    else:
        ranges, default, mark = LIBRARY_RANGES.get(module, []), False, True
    for r in rows:
        cuts = [(r["lo"], r["hi"], default)]
        for sec, lo, hi in ranges:
            if sec != r["section"]:
                continue
            new = []
            for a, b, lib in cuts:
                for x, y, inside in ((a, min(b, lo), False), (max(a, lo), min(b, hi), True),
                                     (max(a, hi), b, False)):
                    if x < y:
                        new.append((x, y, mark if inside else lib))
            cuts = new
        for a, b, lib in cuts:
            out.append(dict(r, lo=a, hi=b, library=lib))
    return out


def measure(module: str) -> dict:
    rows = split_library(module, pieces(module))
    m = {sec: {"c": 0, "asm": 0, "lib": 0} for sec in SECTIONS}
    for r in rows:
        size = r["hi"] - r["lo"]
        key = "lib" if r["library"] else ("c" if r["c"] else "asm")
        m[r["section"]][key] += size
    return m


def kb(n: int) -> str:
    return f"{n / 1024:.1f}K"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--markdown", action="store_true")
    ap.add_argument("--files", metavar="MODULE", help="list the module's data pieces")
    args = ap.parse_args()

    if args.files:
        for r in split_library(args.files, pieces(args.files)):
            who = "library" if r["library"] else ("C" if r["c"] else "asm")
            print(f"{r['section']:8} {r['lo']:08X}-{r['hi']:08X} {r['hi'] - r['lo']:8} {who:8} {r['name']}")
        return 0

    head = ("Module", ".data C", ".data asm", ".rodata C", ".rodata asm", "Capcom data in C", ".bss C / asm", "Library")
    lines = []
    tot = {"c": 0, "asm": 0}
    for mod in MODULES:
        if not config_path(mod).exists():
            continue
        m = measure(mod)
        c = sum(m[s]["c"] for s in (".data", ".rodata", ".sdata"))
        a = sum(m[s]["asm"] for s in (".data", ".rodata", ".sdata"))
        lib = sum(m[s]["lib"] for s in (".data", ".rodata", ".sdata"))
        if mod not in UNCOUNTED:
            tot["c"] += c
            tot["asm"] += a
        pct = f"{100 * c / (c + a):.1f}%" if c + a else "-"
        name = mod + ((" (library, Capcom front end)" if mod in CAPCOM_RANGES else " (library)")
                      if mod in LIBRARY_MODULES else " (not counted)" if mod in UNCOUNTED else "")
        lines.append((name,
                      kb(m[".data"]["c"] + m[".sdata"]["c"]), kb(m[".data"]["asm"] + m[".sdata"]["asm"]),
                      kb(m[".rodata"]["c"]), kb(m[".rodata"]["asm"]),
                      f"{kb(c)} of {kb(c + a)} ({pct})",
                      f"{kb(m['.bss']['c'])} / {kb(m['.bss']['asm'])}", kb(lib)))
    tc, ta = tot["c"], tot["asm"]
    lines.append(("Capcom total", "", "", "", "", f"{kb(tc)} of {kb(tc + ta)} ({100 * tc / (tc + ta):.1f}%)", "", ""))
    if args.markdown:
        print("| " + " | ".join(head) + " |")
        print("|" + "---|" * len(head))
        for ln in lines:
            print("| " + " | ".join(ln) + " |")
    else:
        widths = [max(len(str(x[i])) for x in lines + [head]) for i in range(len(head))]
        for ln in [head] + lines:
            print("  ".join(str(v).ljust(w) for v, w in zip(ln, widths)).rstrip())
    return 0


if __name__ == "__main__":
    sys.exit(main())
