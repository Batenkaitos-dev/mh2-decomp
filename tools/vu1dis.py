#!/usr/bin/env python3
"""VU1 microcode disassembler: the DMA packets of a binary -> tools/vu1as.py's syntax.

    python3 tools/vu1dis.py build/orig/sub_main.bin [--start 0x104680] [--end 0x11E8C0] [--program N]

A maintenance check for src/sub_main/vu1_microcode.vsm: it prints each
program of the block (sub_main 0x3E7F80-0x4021C0 by default) with labels
named after their pair index (Lxxx), in the syntax tools/vu1as.py reads,
so `vu1dis | vu1as` gives the same bytes back. The committed source has
names, headers and comments instead; this shows what the bytes say.
"""
from __future__ import annotations

import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from vu1as import (BC, CONVERT, EFU_VECTOR, FLAG_BITS, LOWER_ARITH, LOWER_NOP, LOWER_OPS2,  # noqa: E402
                   LOWER_PRIMARY, UPPER_NOP, UPPER_OPS, UPPER_OPS2)

UP1 = {v: k for k, v in UPPER_OPS.items()}
UP2 = {v: k for k, v in UPPER_OPS2.items()}
LO2 = {v: k for k, v in LOWER_OPS2.items()}
LOA = {v: k for k, v in LOWER_ARITH.items()}
LOP = {v: k for k, v in LOWER_PRIMARY.items()}
BRANCHES = ("b", "bal", "ibeq", "ibne", "ibltz", "ibgtz", "iblez", "ibgez")


def dest(w: int) -> str:
    return "".join(c for i, c in enumerate(BC) if (w >> (24 - i)) & 1)


def vf(r: int, f: str = "") -> str:
    return f"vf{r:02d}{f}"


def vi(r: int) -> str:
    return f"vi{r:02d}"


def imm11(w: int) -> int:
    v = w & 0x7FF
    return v - 0x800 if v & 0x400 else v


def upper(w: int) -> str:
    ft, fs, fd, d = (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31, dest(w)
    if w & 0x07FFFFFF == UPPER_NOP:
        return "nop"
    if (w & 0x3C) != 0x3C:
        op, acc = w & 0x3F, False
        name = UP1.get(op)
    else:
        op, acc = (((w >> 6) & 0x1F) << 2) | (w & 3), True
        name = UP2.get(op)
    if name is None or name == "nop":
        raise ValueError(f"undefined upper word 0x{w:08x}")
    if name == "clipw":
        return f"clipw.xyz {vf(fs)}, {vf(ft, 'w')}"
    if name in CONVERT:
        return f"{name}.{d} {vf(ft)}, {vf(fs)}"
    if 0x1C <= op <= 0x27:                        # Q / I forms
        src2 = "Q" if name.endswith("q") else "I"
    elif op < 0x1C:                               # broadcast forms
        src2 = vf(ft, name[-1])
    else:
        src2 = vf(ft)
    return f"{name}.{d} {'ACC' if acc else vf(fd)}, {vf(fs)}, {src2}"


def lower(w: int, pc: int, label=lambda t: f"L{t:03x}") -> str:
    """pc: the pair's index (for branch targets); label(index) names a target."""
    op, d = w >> 25, dest(w)
    ft, fs, fd = (w >> 16) & 31, (w >> 11) & 31, (w >> 6) & 31
    fsf, ftf = BC[(w >> 21) & 3], BC[(w >> 23) & 3]
    sfx = f".{d}" if d else ""
    if op == 0x40:
        if w == LOWER_NOP:
            return "nop"
        if (w & 0x3C) != 0x3C:
            name = LOA.get(w & 0x3F)
            if name == "iaddi":
                return f"iaddi {vi(ft)}, {vi(fs)}, {fd - 32 if fd & 16 else fd}"
            if name:
                return f"{name} {vi(fd)}, {vi(fs)}, {vi(ft)}"
            raise ValueError(f"undefined lower word 0x{w:08x}")
        name = LO2.get((((w >> 6) & 0x1F) << 2) | (w & 3))
        forms = {"move": f"{vf(ft)}, {vf(fs)}", "mr32": f"{vf(ft)}, {vf(fs)}",
                 "lqi": f"{vf(ft)}, ({vi(fs)}++)", "sqi": f"{vf(fs)}, ({vi(ft)}++)",
                 "lqd": f"{vf(ft)}, (--{vi(fs)})", "sqd": f"{vf(fs)}, (--{vi(ft)})",
                 "div": f"Q, {vf(fs, fsf)}, {vf(ft, ftf)}", "rsqrt": f"Q, {vf(fs, fsf)}, {vf(ft, ftf)}",
                 "sqrt": f"Q, {vf(ft, ftf)}", "mtir": f"{vi(ft)}, {vf(fs, fsf)}",
                 "mfir": f"{vf(ft)}, {vi(fs)}", "ilwr": f"{vi(ft)}, ({vi(fs)})", "iswr": f"{vi(ft)}, ({vi(fs)})",
                 "rnext": f"{vf(ft)}, R", "rget": f"{vf(ft)}, R", "mfp": f"{vf(ft)}, P",
                 "rinit": f"R, {vf(fs, fsf)}", "rxor": f"R, {vf(fs, fsf)}",
                 "xtop": vi(ft), "xitop": vi(ft), "xgkick": vi(fs)}
        if name is None:
            raise ValueError(f"undefined lower word 0x{w:08x}")
        if name in ("waitq", "waitp"):
            return name
        if name in forms:
            return f"{name}{sfx if name not in ('div', 'rsqrt', 'sqrt', 'mtir', 'rinit', 'rxor', 'xtop', 'xitop', 'xgkick') else ''} {forms[name]}"
        return f"{name}{sfx} P, {vf(fs)}" if name in EFU_VECTOR else f"{name} P, {vf(fs, fsf)}"
    name = LOP.get(op)
    if name is None:
        raise ValueError(f"undefined lower word 0x{w:08x}")
    if name in ("lq", "ilw", "isw"):
        return f"{name}{sfx} {vf(ft) if name == 'lq' else vi(ft)}, {imm11(w)}({vi(fs)})"
    if name == "sq":
        return f"sq{sfx} {vf(fs)}, {imm11(w)}({vi(ft)})"
    if name in ("iaddiu", "isubiu"):
        return f"{name} {vi(ft)}, {vi(fs)}, {((w >> 10) & 0x7800) | (w & 0x7FF)}"
    if name in ("fceq", "fcand", "fcor"):
        return f"{name} vi01, 0x{w & 0xFFFFFF:06x}"
    if name == "fcset":
        return f"fcset 0x{w & 0xFFFFFF:06x}"
    imm12 = ((w >> 10) & 0x800) | (w & 0x7FF)
    if name in ("fseq", "fsand", "fsor"):
        return f"{name} {vi(ft)}, 0x{imm12:03x}"
    if name == "fsset":
        return f"fsset 0x{imm12:03x}"
    if name in ("fmeq", "fmand", "fmor", "jalr"):
        return f"{name} {vi(ft)}, {vi(fs)}"
    if name == "fcget":
        return f"fcget {vi(ft)}"
    if name == "jr":
        return f"jr {vi(fs)}"
    t = label(pc + 1 + imm11(w))
    if name == "b":
        return f"b {t}"
    if name == "bal":
        return f"bal {vi(ft)}, {t}"
    if name in ("ibeq", "ibne"):
        return f"{name} {vi(ft)}, {vi(fs)}, {t}"
    return f"{name} {vi(fs)}, {t}"


def flags(up: int) -> str:
    return "".join(f"[{c}]" for c, b in FLAG_BITS.items() if c != "I" and (up >> b) & 1)


def loi_text(lo: int) -> str:
    f = struct.unpack("<f", struct.pack("<I", lo))[0]
    return repr(f) if struct.unpack("<I", struct.pack("<f", float(repr(f))))[0] == lo else f"0x{lo:08x}"


def branch_target(lo: int, pc: int) -> int | None:
    return pc + 1 + imm11(lo) if LOP.get(lo >> 25) in BRANCHES else None


def packets(data: bytes, start: int, end: int):
    """Yield (offset, [(lower, upper)...], [VIF codes after the code, with their data])."""
    w = lambda o: struct.unpack_from("<I", data, o)[0]   # noqa: E731
    o = start
    while o < end:
        tag = w(o)
        if tag >> 28 != 6:
            raise ValueError(f"0x{o:x}: not a DMA RET tag")
        stop, p, code, vif = o + 16 + (tag & 0xFFFF) * 16, o + 8, {}, []
        while p < stop:
            c = w(p)
            cmd, n = (c >> 24) & 0x7F, (c >> 16) & 0xFF
            p += 4
            if cmd == 0x4A:                                  # MPG
                for k in range(n or 256):
                    code[(c & 0xFFFF) + k] = (w(p + 8 * k), w(p + 8 * k + 4))
                p += (n or 256) * 8
            elif cmd >= 0x60:                                # UNPACK (V4-32 here)
                vif += [c] + [w(p + 4 * k) for k in range((n or 256) * 4)]
                p += (n or 256) * 16
            elif cmd:
                vif.append(c)
        yield o, [code[k] for k in range(len(code))], vif
        o = stop


def program_lines(code, vif, name: str) -> list[str]:
    targets = {t for pc, (lo, up) in enumerate(code) if not up >> 31 for t in [branch_target(lo, pc)] if t is not None}
    out = [f".program {name}"]
    i = 0
    while i < len(vif):
        c = vif[i]
        cmd = (c >> 24) & 0x7F
        if cmd == 3:
            out.append(f".base 0x{c & 0xFFFF:x}")
        elif cmd == 2:
            out.append(f".offset 0x{c & 0xFFFF:x}")
        elif cmd == 1:
            out.append(f".stcycl {c & 0xFF}, {(c >> 8) & 0xFF}")
        elif cmd == 0x6C:
            n = ((c >> 16) & 0xFF) * 4
            out.append(f".unpack 0x{c & 0xFFFF:x}, " + ", ".join(f"0x{x:08x}" for x in vif[i + 1:i + 1 + n]))
            i += n
        else:
            raise ValueError(f"VIF code 0x{c:08x} not in the source syntax")
        i += 1
    for pc, (lo, up) in enumerate(code):
        if pc in targets:
            out.append(f"L{pc:03x}:")
        lt = f"loi {loi_text(lo)}" if up >> 31 else lower(lo, pc)
        out.append(f"    {upper(up):34} {lt:34} {flags(up)}".rstrip())
    out.append(".endprogram")
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("binary")
    ap.add_argument("--start", type=lambda s: int(s, 0), default=0x104680)
    ap.add_argument("--end", type=lambda s: int(s, 0), default=0x11E8C0)
    ap.add_argument("--program", type=int, help="only the Nth packet (from 0)")
    args = ap.parse_args()
    data = open(args.binary, "rb").read()
    for n, (o, code, vif) in enumerate(packets(data, args.start, args.end)):
        if args.program is None or args.program == n:
            print("\n".join(program_lines(code, vif, f"Vu1Packet{n:02d}")) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
