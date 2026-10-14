#!/usr/bin/env python3
"""VU1 microcode assembler: src/sub_main/vu1_microcode.vsm -> GNU as source.

    python3 tools/vu1as.py SRC.vsm OUT.s [--bin OUT.bin]

Writes OUT.s, the block of DMA packets as `.word`s (the build assembles it
with the usual `as` rule), and with --bin the raw bytes. Our own tool,
written from the VU instruction set: no Sony toolchain is involved.

The source (the .vsm file's header has the full conventions):
- `;` starts a comment. One instruction pair per line, the upper (FMAC)
  instruction first, then the lower, then any flags of the upper word:
  `[E]` end, `[M]`, `[D]`, `[T]`. The lower `loi VALUE` sets the I bit and
  stores VALUE (a float, or 0x... bits) as the lower word.
- `name:` labels the next pair; labels are local to their program. Branch
  targets are labels.
- `.program NAME` starts a program, `.endprogram` ends it. NAME is a global
  symbol at the start of the program's packet.
- `.base N`, `.offset N`, `.stcycl CL, WL` and `.unpack ADDR, W0, W1, W2, W3`
  (an UNPACK V4-32 of one qword per four words) are VIF codes sent after the
  code; they are emitted in the order written.

The packing (the rules the 60 packets follow): a DMA RET tag (its qwc
computed), a VIF NOP, the code as MPG transfers of up to 256 pairs loaded
from micro address 0 (a NOP before an MPG whose data would not start on 8
bytes), the program's VIF codes, and NOPs to a quadword.
"""
from __future__ import annotations

import argparse
import re
import struct
import sys

BC = "xyzw"
FLAG_BITS = {"I": 31, "E": 30, "M": 29, "D": 28, "T": 27}

# ---- the instruction set: opcode tables shared with tools/vu1dis.py ----
# Upper, bits 0-5 below 0x3C: name -> opcode (bc variants: x y z w in order).
UPPER_OPS = {}
for _i, _n in enumerate(("add", "sub", "madd", "msub", "max", "mini", "mul")):
    for _b in range(4):
        UPPER_OPS[_n + BC[_b]] = _i * 4 + _b
UPPER_OPS.update({"mulq": 0x1C, "maxi": 0x1D, "muli": 0x1E, "minii": 0x1F, "addq": 0x20, "maddq": 0x21,
                  "addi": 0x22, "maddi": 0x23, "subq": 0x24, "msubq": 0x25, "subi": 0x26, "msubi": 0x27,
                  "add": 0x28, "madd": 0x29, "mul": 0x2A, "max": 0x2B, "sub": 0x2C, "msub": 0x2D,
                  "opmsub": 0x2E, "mini": 0x2F})
# Upper, bits 0-5 0x3C-0x3F: name -> op2 (bits 6-10 and 0-1).
UPPER_OPS2 = {}
for _i, _n in enumerate(("adda", "suba", "madda", "msuba")):
    for _b in range(4):
        UPPER_OPS2[_n + BC[_b]] = _i * 4 + _b
for _b in range(4):
    UPPER_OPS2["mula" + BC[_b]] = 0x18 + _b
UPPER_OPS2.update({"itof0": 0x10, "itof4": 0x11, "itof12": 0x12, "itof15": 0x13, "ftoi0": 0x14,
                   "ftoi4": 0x15, "ftoi12": 0x16, "ftoi15": 0x17, "mulaq": 0x1C, "abs": 0x1D,
                   "mulai": 0x1E, "clipw": 0x1F, "addaq": 0x20, "maddaq": 0x21, "addai": 0x22,
                   "maddai": 0x23, "subaq": 0x24, "msubaq": 0x25, "subai": 0x26, "msubai": 0x27,
                   "adda": 0x28, "madda": 0x29, "mula": 0x2A, "suba": 0x2C, "msuba": 0x2D,
                   "opmula": 0x2E, "nop": 0x2F})
CONVERT = ("itof0", "itof4", "itof12", "itof15", "ftoi0", "ftoi4", "ftoi12", "ftoi15", "abs")
# Lower, the 0x40 group with bits 0-5 0x3C-0x3F: name -> op2.
LOWER_OPS2 = {"move": 0x30, "mr32": 0x31, "lqi": 0x34, "sqi": 0x35, "lqd": 0x36, "sqd": 0x37, "div": 0x38,
              "sqrt": 0x39, "rsqrt": 0x3A, "waitq": 0x3B, "mtir": 0x3C, "mfir": 0x3D, "ilwr": 0x3E,
              "iswr": 0x3F, "rnext": 0x40, "rget": 0x41, "rinit": 0x42, "rxor": 0x43, "mfp": 0x64,
              "xtop": 0x68, "xitop": 0x69, "xgkick": 0x6C, "esadd": 0x70, "ersadd": 0x71, "eleng": 0x72,
              "erleng": 0x73, "eatanxy": 0x74, "eatanxz": 0x75, "esum": 0x76, "esqrt": 0x78,
              "ersqrt": 0x79, "ercpr": 0x7A, "waitp": 0x7B, "esin": 0x7C, "eatan": 0x7D, "eexp": 0x7E}
LOWER_ARITH = {"iadd": 0x30, "isub": 0x31, "iaddi": 0x32, "iand": 0x34, "ior": 0x35}
# Lower, other primary opcodes (bits 25-31).
LOWER_PRIMARY = {"lq": 0x00, "sq": 0x01, "ilw": 0x04, "isw": 0x05, "iaddiu": 0x08, "isubiu": 0x09,
                 "fceq": 0x10, "fcset": 0x11, "fcand": 0x12, "fcor": 0x13, "fseq": 0x14, "fsset": 0x15,
                 "fsand": 0x16, "fsor": 0x17, "fmeq": 0x18, "fmand": 0x1A, "fmor": 0x1B, "fcget": 0x1C,
                 "b": 0x20, "bal": 0x21, "jr": 0x24, "jalr": 0x25, "ibeq": 0x28, "ibne": 0x29,
                 "ibltz": 0x2C, "ibgtz": 0x2D, "iblez": 0x2E, "ibgez": 0x2F}
EFU_VECTOR = ("esadd", "ersadd", "eleng", "erleng", "eatanxy", "eatanxz", "esum")
UPPER_NOP = 0x000002FF
LOWER_NOP = 0x8000033C   # move.(none) vf00, vf00


class AsmError(Exception):
    pass


def dest_bits(d: str) -> int:
    if any(c not in BC for c in d) or list(d) != sorted(d, key=BC.index):
        raise AsmError(f"bad dest .{d}")
    return sum(1 << (24 - BC.index(c)) for c in d)


def vf(s: str, field: bool = False):
    """vfNN, or with field=True vfNNx (returns (NN, index of x))."""
    m = re.fullmatch(r"vf(\d\d)([xyzw]?)", s)
    if not m or int(m.group(1)) > 31 or bool(m.group(2)) != field:
        raise AsmError(f"bad VF operand {s}")
    return (int(m.group(1)), BC.index(m.group(2))) if field else int(m.group(1))


def vi(s: str) -> int:
    m = re.fullmatch(r"vi(\d\d)", s)
    if not m or int(m.group(1)) > 15:
        raise AsmError(f"bad VI operand {s}")
    return int(m.group(1))


def num(s: str) -> int:
    return int(s, 0)


def mem(s: str, pattern: str) -> int:
    """The vi register of (viNN), (viNN++) or (--viNN)."""
    m = re.fullmatch(pattern.replace("R", r"(vi\d\d)"), s)
    if not m:
        raise AsmError(f"bad address operand {s}")
    return vi(m.group(1))


def offset(s: str) -> tuple[int, int]:
    """imm(viNN) -> (imm, NN), imm a signed 11-bit offset."""
    m = re.fullmatch(r"(-?\d+)\((vi\d\d)\)", s)
    if not m or not -1024 <= int(m.group(1)) < 1024:
        raise AsmError(f"bad offset operand {s}")
    return int(m.group(1)) & 0x7FF, vi(m.group(2))


def split_pair(text: str) -> tuple[str, list[str], str, list[str]]:
    """'mul.xyzw vf24, vf29, vf24  lq.xyzw vf28, 0(vi00)' -> the two instructions:
    an instruction's operands run while the previous operand ends with a comma."""
    toks = text.split()

    def take(i):
        mn, ops = toks[i], []
        i += 1
        if mn.split(".")[0] not in ("nop", "waitq", "waitp"):
            while i < len(toks):
                ops.append(toks[i].rstrip(","))
                i += 1
                if not toks[i - 1].endswith(","):
                    break
        return mn, ops, i

    up, uops, i = take(0)
    if i >= len(toks):
        raise AsmError("a pair needs an upper and a lower instruction")
    lo, lops, j = take(i)
    if j != len(toks):
        raise AsmError(f"unexpected text after the lower instruction: {' '.join(toks[j:])}")
    return up, uops, lo, lops


def encode_upper(mn: str, ops: list[str]) -> int:
    name, _, d = mn.partition(".")
    if name == "nop":
        return UPPER_NOP
    db = dest_bits(d)

    def src2(s):   # ft with its broadcast field, or Q / I (ft field 0)
        return 0 if s in ("Q", "I") else (vf(s, field=True)[0] if s[-1] in BC else vf(s))
    if name in UPPER_OPS:
        return db | (src2(ops[2]) << 16) | (vf(ops[1]) << 11) | (vf(ops[0]) << 6) | UPPER_OPS[name]
    if name not in UPPER_OPS2:
        raise AsmError(f"unknown upper instruction {mn}")
    op2 = UPPER_OPS2[name]
    low = ((op2 >> 2) << 6) | 0x3C | (op2 & 3)
    if name == "clipw":                          # clipw.xyz fs, ftw
        return dest_bits("xyz") | (vf(ops[1], field=True)[0] << 16) | (vf(ops[0]) << 11) | low
    if name in CONVERT:                          # itof0.xyzw ft, fs
        return db | (vf(ops[0]) << 16) | (vf(ops[1]) << 11) | low
    if ops[0] != "ACC":
        raise AsmError(f"{mn} writes ACC")
    return db | (src2(ops[2]) << 16) | (vf(ops[1]) << 11) | low


def encode_lower(mn: str, ops: list[str], pc: int, labels: dict[str, int]) -> int:
    name, _, d = mn.partition(".")
    if name == "nop":
        return LOWER_NOP
    db = dest_bits(d)
    group = 0x40 << 25
    if name in LOWER_ARITH:
        if name == "iaddi":                      # iaddi it, is, imm5
            imm = num(ops[2])
            if not -16 <= imm < 16:
                raise AsmError(f"iaddi immediate {imm} out of range")
            return group | (vi(ops[0]) << 16) | (vi(ops[1]) << 11) | ((imm & 31) << 6) | 0x32
        return group | (vi(ops[2]) << 16) | (vi(ops[1]) << 11) | (vi(ops[0]) << 6) | LOWER_ARITH[name]
    if name in LOWER_OPS2:
        op2 = LOWER_OPS2[name]
        ft = fs = extra = 0
        if name in ("move", "mr32"):
            ft, fs = vf(ops[0]), vf(ops[1])
        elif name in ("lqi", "lqd"):
            ft, fs = vf(ops[0]), mem(ops[1], r"\(R\+\+\)" if name == "lqi" else r"\(--R\)")
        elif name in ("sqi", "sqd"):
            fs, ft = vf(ops[0]), mem(ops[1], r"\(R\+\+\)" if name == "sqi" else r"\(--R\)")
        elif name in ("div", "rsqrt"):           # div Q, fsf, ftf
            (fs, fsf), (ft, ftf) = vf(ops[1], True), vf(ops[2], True)
            extra = (fsf << 21) | (ftf << 23)
        elif name == "sqrt":                     # sqrt Q, ftf
            ft, ftf = vf(ops[1], True)
            extra = ftf << 23
        elif name == "mtir":                     # mtir it, fsf
            ft, (fs, fsf) = vi(ops[0]), vf(ops[1], True)
            extra = fsf << 21
        elif name == "mfir":                     # mfir.dest ft, is
            ft, fs = vf(ops[0]), vi(ops[1])
        elif name in ("ilwr", "iswr"):           # ilwr.f it, (is)
            ft, fs = vi(ops[0]), mem(ops[1], r"\(R\)")
        elif name in ("rnext", "rget", "mfp"):   # rnext.dest ft, R
            ft = vf(ops[0])
        elif name in ("rinit", "rxor"):          # rinit R, fsf
            fs, fsf = vf(ops[1], True)
            extra = fsf << 21
        elif name in ("xtop", "xitop"):
            ft = vi(ops[0])
        elif name == "xgkick":
            fs = vi(ops[0])
        elif name in EFU_VECTOR:                 # esadd P, fs
            fs = vf(ops[1])
        elif name not in ("waitq", "waitp"):     # esqrt P, fsf
            fs, fsf = vf(ops[1], True)
            extra = fsf << 21
        return group | db | extra | (ft << 16) | (fs << 11) | ((op2 >> 2) << 6) | 0x3C | (op2 & 3)
    if name not in LOWER_PRIMARY:
        raise AsmError(f"unknown lower instruction {mn}")
    op = LOWER_PRIMARY[name] << 25
    if name in ("lq", "ilw", "isw"):             # lq.dest ft, imm(is)
        imm, base = offset(ops[1])
        return op | db | ((vf(ops[0]) if name == "lq" else vi(ops[0])) << 16) | (base << 11) | imm
    if name == "sq":                             # sq.dest fs, imm(it)
        imm, base = offset(ops[1])
        return op | db | (base << 16) | (vf(ops[0]) << 11) | imm
    if name in ("iaddiu", "isubiu"):             # iaddiu it, is, imm15
        imm = num(ops[2])
        if not 0 <= imm < 0x8000:
            raise AsmError(f"{name} immediate {imm} out of range")
        return op | ((imm >> 11) << 21) | (vi(ops[0]) << 16) | (vi(ops[1]) << 11) | (imm & 0x7FF)
    if name in ("fceq", "fcand", "fcor", "fcset"):   # fcand vi01, imm24
        if name != "fcset" and ops[0] != "vi01":
            raise AsmError(f"{name} writes vi01")
        return op | (num(ops[-1]) & 0xFFFFFF)
    if name in ("fseq", "fsand", "fsor", "fsset"):   # fsand it, imm12
        imm = num(ops[-1])
        it = 0 if name == "fsset" else vi(ops[0])
        return op | (((imm >> 11) & 1) << 21) | (it << 16) | (imm & 0x7FF)
    if name in ("fmeq", "fmand", "fmor"):        # fmand it, is
        return op | (vi(ops[0]) << 16) | (vi(ops[1]) << 11)
    if name == "fcget":
        return op | (vi(ops[0]) << 16)
    if name == "jr":
        return op | (vi(ops[0]) << 11)
    if name == "jalr":                           # jalr it, is
        return op | (vi(ops[0]) << 16) | (vi(ops[1]) << 11)
    # branches: the target is a label; the offset counts pairs from the next pair
    if ops[-1] not in labels:
        raise AsmError(f"unknown label {ops[-1]}")
    off = labels[ops[-1]] - pc - 1
    if not -1024 <= off < 1024:
        raise AsmError(f"branch to {ops[-1]} out of range")
    off &= 0x7FF
    if name == "b":
        return op | off
    if name == "bal":                            # bal it, label
        return op | (vi(ops[0]) << 16) | off
    if name in ("ibeq", "ibne"):                 # ibeq it, is, label
        return op | (vi(ops[0]) << 16) | (vi(ops[1]) << 11) | off
    return op | (vi(ops[0]) << 11) | off         # ibltz is, label


def float_bits(s: str) -> int:
    if s.lower().startswith("0x"):
        return int(s, 16)
    return struct.unpack("<I", struct.pack("<f", float(s)))[0]


class Program:
    def __init__(self, name: str, line: int):
        self.name, self.line = name, line
        self.rows: list[tuple[int, str]] = []    # (source line, pair text)
        self.labels: dict[str, int] = {}
        self.vif: list[int] = []                 # VIF codes and data after the code

    def assemble(self) -> list[int]:
        """The program's DMA packet as 32-bit words."""
        code = []
        for pc, (line, text) in enumerate(self.rows):
            try:
                flags = re.findall(r"\[([IEMDT])\]", text)
                text = re.sub(r"\[[IEMDT]\]", "", text)
                up, uops, lo, lops = split_pair(text)
                upper = encode_upper(up, uops)
                if lo == "loi":
                    flags.append("I")
                    lower = float_bits(lops[0])
                else:
                    lower = encode_lower(lo, lops, pc, self.labels)
                for f in flags:
                    upper |= 1 << FLAG_BITS[f]
            except (AsmError, IndexError, ValueError, KeyError) as e:
                raise AsmError(f"line {line}: {e}: {text.strip()}") from None
            code.append((lower, upper))
        words = [0]                              # VIF NOP after the tag
        for start in range(0, len(code), 256):
            chunk = code[start:start + 256]
            if len(words) % 2 == 0:              # MPG data starts on 8 bytes
                words.append(0)
            words.append(0x4A000000 | ((len(chunk) & 0xFF) << 16) | start)
            for lower, upper in chunk:
                words += [lower, upper]
        words += self.vif
        while (len(words) + 2) % 4:
            words.append(0)
        qwc = (len(words) + 2) // 4 - 1
        return [0x60000000 | qwc, 0] + words    # DMA RET tag (id 6), qwc after the tag


def parse(text: str) -> list[Program]:
    progs: list[Program] = []
    cur = None
    for n, raw in enumerate(text.splitlines(), 1):
        line = raw.split(";", 1)[0].strip()
        if not line:
            continue
        try:
            m = re.match(r"([A-Za-z_][\w.]*):(.*)$", line)
            if m and not line.startswith("."):
                if cur is None:
                    raise AsmError("label outside a program")
                if m.group(1) in cur.labels:
                    raise AsmError(f"label {m.group(1)} defined twice")
                cur.labels[m.group(1)] = len(cur.rows)
                line = m.group(2).strip()
                if not line:
                    continue
            if line.startswith("."):
                d, _, args = line.partition(" ")
                a = [x.strip() for x in args.split(",")] if args.strip() else []
                if d == ".program":
                    if cur is not None:
                        raise AsmError(".program inside a program")
                    cur = Program(a[0], n)
                elif cur is None:
                    raise AsmError(f"{d} outside a program")
                elif d == ".endprogram":
                    progs.append(cur)
                    cur = None
                elif d == ".base":
                    cur.vif.append(0x03000000 | num(a[0]))
                elif d == ".offset":
                    cur.vif.append(0x02000000 | num(a[0]))
                elif d == ".stcycl":
                    cur.vif.append(0x01000000 | (num(a[1]) << 8) | num(a[0]))
                elif d == ".unpack":                     # V4-32, one qword per 4 words
                    data = [num(x) for x in a[1:]]
                    if not data or len(data) % 4:
                        raise AsmError(".unpack needs quadwords (4 words each)")
                    cur.vif.append(0x6C000000 | ((len(data) // 4) << 16) | num(a[0]))
                    cur.vif += data
                else:
                    raise AsmError(f"unknown directive {d}")
                continue
            if cur is None:
                raise AsmError("instruction outside a program")
            cur.rows.append((n, line))
        except (AsmError, IndexError, ValueError) as e:
            raise AsmError(f"line {n}: {e}") from None
    if cur is not None:
        raise AsmError(f".program {cur.name} (line {cur.line}) has no .endprogram")
    return progs


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("src")
    ap.add_argument("out", help="GNU as source to write")
    ap.add_argument("--symbol", help="a function symbol over the whole block (objdiff pairs it by name)")
    ap.add_argument("--bin", help="also write the raw bytes")
    args = ap.parse_args()
    try:
        progs = parse(open(args.src, encoding="utf-8").read())
        packets = [(p.name, p.assemble()) for p in progs]
    except AsmError as e:
        sys.exit(f"{args.src}: {e}")
    out = [f"# Generated by tools/vu1as.py from {args.src}. Do not edit.",
           '.include "macro.inc"', "", '.section .text, "ax"', ""]
    if args.symbol:
        out.append(f"glabel {args.symbol}")
    for name, words in packets:
        out.append(f".global {name}\n{name}:")
        for i in range(0, len(words), 4):
            out.append("    .word " + ", ".join(f"0x{w:08X}" for w in words[i:i + 4]))
    if args.symbol:
        out.append(f"endlabel {args.symbol}")
    with open(args.out, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")
    if args.bin:
        with open(args.bin, "wb") as f:
            for _, words in packets:
                f.write(struct.pack(f"<{len(words)}I", *words))
    return 0


if __name__ == "__main__":
    sys.exit(main())
