#!/usr/bin/env python3
"""Browse splat's assembly by function: show, list, and find candidates.

    python tools/asm_funcs.py MODULE show FUNC [FUNC ...]   # FUNC: name or hex address
    python tools/asm_funcs.py MODULE list LO HI             # functions in [LO, HI)
    python tools/asm_funcs.py MODULE candidates [MAXSIZE] [LO HI]
    python tools/asm_funcs.py MODULE runs [MAXSIZE] [LO HI]      # runs of candidates

`candidates` lists functions still in assembly, up to MAXSIZE bytes (default
0x80), with flags: J uses a jump table (its C file must own its .rodata), F uses the FPU (float constants may go to rodata),
C calls other functions, L has a loop. `list` and `candidates` also print how
many `jal`s in all modules call each function.

Dense runs of small functions are good places for a new C file: `runs` lists
the longest runs of at least 5 adjacent candidates without J or F (MAXSIZE
default 0x60).
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
NONMATCHING = re.compile(r"^nonmatching (\S+), (0x[0-9A-F]+)")
INSN = re.compile(r"/\* [0-9A-F]+ ([0-9A-F]{8}) [0-9A-F]{8} \*/\s+(\S+)\s*(.*)$")
VRAM = re.compile(r"/\* [0-9A-F]+ ([0-9A-F]{8}) [0-9A-F]{8} \*/")
FPU_OPS = {"mtc1", "mfc1", "lwc1", "swc1", "ctc1", "cfc1"}


def asm_files(module: str, only_asm: bool) -> list[Path]:
    d = ROOT / "asm" / module
    files = []
    for p in sorted(d.glob("*.s")):
        # a C file's range also has a full disassembly next to the others
        if only_asm and (ROOT / "src" / module / f"{p.stem}.c").exists():
            continue
        files.append(p)
    return files


def functions(module: str, only_asm: bool = False):
    """Yield dicts: name, addr, size, lines, flags."""
    for p in asm_files(module, only_asm):
        yield from functions_in(p)


def functions_in(p: Path):
    """The functions of one assembly file, as functions() yields them."""
    cur = None
    for line in p.read_text(encoding="utf-8").splitlines():
        m = NONMATCHING.match(line)
        if m:
            if cur:
                yield cur
            cur = {"name": m.group(1), "size": int(m.group(2), 16), "addr": None,
                   "lines": [], "flags": set()}
            continue
        if cur is None:
            continue
        if line.startswith("endlabel"):
            yield cur
            cur = None
            continue
        i = INSN.search(line)
        if i:
            addr, op, args = int(i.group(1), 16), i.group(2), i.group(3)
            if cur["addr"] is None:
                cur["addr"] = addr
            if "jtbl_" in args:
                cur["flags"].add("J")
            if op.endswith((".s", ".w")) or op in FPU_OPS:
                cur["flags"].add("F")
            if op in ("jal", "jalr"):
                cur["flags"].add("C")
            t = re.search(r"\.L([0-9A-F]{8})", args)
            if op.startswith("b") and t and int(t.group(1), 16) < addr:
                cur["flags"].add("L")
        if line.strip() and not line.startswith("glabel"):
            cur["lines"].append(VRAM.sub(lambda mm: mm.group(1), line.rstrip()))
    if cur:
        yield cur


def callers() -> dict[str, int]:
    """Number of `jal`s to each name, over every module's assembly."""
    count: dict[str, int] = {}
    for p in (ROOT / "asm").rglob("*.s"):
        if "nonmatchings" in p.parts:
            continue
        for m in re.finditer(r"\bjal\s+(\w+)", p.read_text(encoding="utf-8")):
            count[m.group(1)] = count.get(m.group(1), 0) + 1
    return count


def runs(module: str, maxsize: int, lo: int, hi: int) -> list[list[dict]]:
    """Runs of at least 5 adjacent small functions without jump tables or FPU."""
    fs = sorted((f for f in functions(module, only_asm=True)
                 if f["addr"] is not None and lo <= f["addr"] < hi), key=lambda f: f["addr"])
    out, cur = [], []
    for f in fs:
        ok = f["size"] <= maxsize and not f["flags"] & {"J", "F"}
        # adjacent: at most one alignment gap after the previous function
        near = cur and f["addr"] <= cur[-1]["addr"] + ((cur[-1]["size"] + 15) & ~15) + 0x10
        if ok and (not cur or near):
            cur.append(f)
            continue
        if len(cur) >= 5:
            out.append(cur)
        cur = [f] if ok else []
    if len(cur) >= 5:
        out.append(cur)
    return sorted(out, key=lambda r: -len(r))


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    module, cmd, rest = sys.argv[1], sys.argv[2], sys.argv[3:]
    if cmd == "show":
        want = set(rest) | {int(w, 16) for w in rest if re.fullmatch(r"(0x)?[0-9A-Fa-f]{5,8}", w)}
        for f in functions(module):
            if f["name"] in want or f["addr"] in want:
                print(f"== {f['name']} @ {f['addr']:08X}, {f['size']:#x} bytes")
                print("\n".join(f["lines"]))
        return 0
    if cmd == "list":
        lo, hi = int(rest[0], 16), int(rest[1], 16)
        n = callers()
        for f in functions(module):
            if f["addr"] is not None and lo <= f["addr"] < hi:
                print(f"{f['addr']:08X} {f['size']:#7x} {''.join(sorted(f['flags'])):4} "
                      f"{n.get(f['name'], 0):4} {f['name']}")
        return 0
    if cmd == "candidates":
        maxsize = int(rest[0], 0) if rest else 0x80
        lo, hi = (int(rest[1], 16), int(rest[2], 16)) if len(rest) >= 3 else (0, 1 << 32)
        n = callers()
        for f in functions(module, only_asm=True):
            if f["addr"] is not None and lo <= f["addr"] < hi and f["size"] <= maxsize:
                print(f"{f['addr']:08X} {f['size']:#7x} {''.join(sorted(f['flags'])):4} "
                      f"{n.get(f['name'], 0):4} {f['name']}")
        return 0
    if cmd == "runs":
        maxsize = int(rest[0], 0) if rest else 0x60
        lo, hi = (int(rest[1], 16), int(rest[2], 16)) if len(rest) >= 3 else (0, 1 << 32)
        for r in runs(module, maxsize, lo, hi)[:25]:
            end = r[-1]["addr"] + r[-1]["size"]
            print(f"{len(r):3} functions {r[0]['addr']:08X}-{end:08X}  sizes "
                  + " ".join(f"{f['size']:x}" for f in r[:16]) + (" ..." if len(r) > 16 else ""))
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
