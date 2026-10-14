#!/usr/bin/env python3
"""Write typed C definitions for a range of a module's .data (gen_data.py with types).

    python tools/type_data.py MODULE LO HI TYPES [--rodata LO HI] [--c FILE.c ...]

Like `tools/gen_data.py MODULE data LO HI` (same objects, same pointers),
except that the objects named in the TYPES file get their C type:

    # a struct: its name, then its fields' types in order (TYPE or TYPE*N)
    struct EmCurveKey f32 f32
    struct EmFlyParam s32*15 u16*6 s32*4 f32*2
    # an object: its address (or name), its type; an array when the object
    # is bigger than the type (or `count=N`); `name=` gives it a real name,
    # `scalar` makes a single value a variable rather than an array of one
    # (a single struct is a variable unless `array` is given)
    008C58F0 EmCurveKey
    008C5AA0 EmCurveKey*
    008C57B0 EmFlyParam name=EmKutku_FlyParam
    # words splat took for addresses that are numbers (colors, packed bytes)
    number 00442480 0043AB24

Scalar types: u8 s8 u16 s16 u32 s32 f32, `ptr` (void *), `fn` (a function
pointer: written as the function's name) and `T*` (a pointer to T). A
pointer field whose word the split has no relocation for is written as a
cast number (another overlay's address); a number field whose word the
split took for a pointer is written as the number. Floats are written in
the shortest form that gives the same bits. Objects not in TYPES come out
as gen_data.py writes them (u32 arrays).

It needs a split in which the range is still assembly (or a checkout of
one, with --root).
The struct layouts must match the C file's (the tool doesn't read headers).
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
import unicodedata
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_data  # noqa: E402

SCALARS = {"u8": (1, "B"), "s8": (1, "b"), "char": (1, "b"), "u16": (2, "H"), "s16": (2, "h"),
           "u32": (4, "I"), "s32": (4, "i"), "f32": (4, "f"), "u64": (8, "Q"), "s64": (8, "q"), "ptr": (4, "P"), "fn": (4, "P")}


def f32_text(bits: int) -> str:
    v = struct.unpack("<f", struct.pack("<I", bits))[0]
    if v != v or v in (float("inf"), float("-inf")):
        raise ValueError(f"float bits {bits:08X}")
    for digits in range(1, 18):
        s = f"{v:.{digits}g}"
        if struct.pack("<f", float(s)) == struct.pack("<I", bits):
            break
    s = repr(float(s))          # positional where Python writes it so (120.0, not 1.2e+02)
    if s.endswith(".0") and "e" in s:
        s = s[:-2]
    if bits == 0x80000000:
        s = "-0.0"
    return s + "f"


def sig(ret: str, params: str) -> str:
    """A function's signature without the parameter names: `void|s8*,s16*`."""
    out = []
    for prm in params.split(","):
        prm = " ".join(prm.replace("*", " * ").split())
        toks = prm.split()
        if len(toks) > 1 and re.match(r"[A-Za-z_]\w*$", toks[-1]) and toks[-2] != "struct" and not (
                len(toks) == 2 and toks[0] in ("unsigned", "signed")):
            toks = toks[:-1]
        out.append("".join(toks))
    r = "".join(ret.replace("*", " * ").split())
    return r + "|" + ",".join(x for x in out if x and x != "void")


class Types:
    def __init__(self, path: Path):
        self.structs: dict[str, list[tuple[str, int]]] = {}
        self.objs: dict[str, tuple[str, str | None]] = {}   # address or name -> (type, new name)
        self.counts: dict[str, int] = {}                     # address -> array length
        self.scalars: set[str] = set()                       # addresses of single variables
        self.fntypes: set[str] = set()                       # function pointer typedefs
        self.fnsig: dict[str, str] = {}                      # typedef or function name -> signature
        self.arrays: set[str] = set()                        # addresses of arrays of one
        self.tdefs: dict[str, str] = {}                      # scalar typedef -> basic type
        self.numbers: set[int] = set()                       # words the split took for pointers
        for line in path.read_text(encoding="utf-8").splitlines():
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            parts = line.split()
            if parts[0] == "number":
                # : words splat took for addresses (colors, packed bytes)
                self.numbers |= {int(p, 16) for p in parts[1:]}
                continue
            if parts[0] == "struct":
                fields = []
                for f in parts[2:]:
                    t, _, n = f.partition("*") if re.match(r"^\w+\*\d+$", f) else (f, "", "")
                    fields.append((t, int(n) if n else 0))
                self.structs[parts[1]] = fields
            else:
                name = None
                for p in parts[2:]:
                    if p.startswith("name="):
                        name = p[5:]
                    if p.startswith("count="):
                        self.counts[parts[0].upper()] = int(p[6:], 0)
                    if p == "scalar":
                        self.scalars.add(parts[0].upper())
                    if p == "array":
                        self.arrays.add(parts[0].upper())
                self.objs[parts[0].upper() if re.match(r"^[0-9A-Fa-f]{8}$", parts[0]) else parts[0]] = (parts[1], name)
                inner = re.match(r"^(\w+)\*(\d+)$", parts[1])
                if inner:
                    # an array of arrays: T*N is one row of N
                    self.structs[parts[1]] = [(inner.group(1), 0)] * int(inner.group(2))

    CTYPES = {"u8": "u8", "s8": "s8", "u16": "u16", "s16": "s16", "u32": "u32", "s32": "s32",
              "f32": "f32", "int": "s32", "char": "s8", "short": "s16", "float": "f32"}

    def load_c(self, path: Path) -> None:
        """Read the layouts of the simple `typedef struct {...} Name;` in a C file or header."""
        text = re.sub(r"/\*.*?\*/|//[^\n]*", "", path.read_text(encoding="utf-8"), flags=re.S)
        self.fntypes |= set(re.findall(r"typedef\s+[\w\s\*]+?\(\s*\*\s*(\w+)\s*\)\s*\(", text))
        # scalar typedefs (zlib's uInt, Byte): their basic type
        basic = {"unsigned int": "u32", "unsigned long": "u32", "unsigned": "u32", "int": "s32",
                 "long": "s32", "signed int": "s32", "unsigned char": "u8", "char": "s8",
                 "signed char": "s8", "unsigned short": "u16", "short": "s16", "float": "f32"}
        for m in re.finditer(r"typedef\s+((?:unsigned\s+|signed\s+)?\w+)\s+(\w+)\s*;", text):
            b = " ".join(m.group(1).split())
            b = basic.get(b, self.tdefs.get(b, b))
            if b in SCALARS:
                self.tdefs[m.group(2)] = b
        # signatures (the return type and the parameter types, without names): a function a
        # table of a typedef's type points to is cast when its own signature differs
        for m in re.finditer(r"typedef\s+([\w\s\*]+?)\(\s*\*\s*(\w+)\s*\)\s*\(([^;{)]*)\)\s*;", text):
            self.fnsig[m.group(2)] = sig(m.group(1), m.group(3))
        for m in re.finditer(r"^(?:static\s+|extern\s+)?([A-Za-z_][\w\s\*]*?)\b(\w+)\s*\(([^;{)]*)\)\s*[;{]", text, re.M):
            if m.group(2) not in ("if", "while", "for", "switch", "return", "sizeof"):
                self.fnsig.setdefault(m.group(2), sig(m.group(1), m.group(3)))
        for m in re.finditer(r"typedef\s+struct\s*\w*\s*\{([^{}]*)\}\s*(\w+)\s*(?:__attribute__\s*\(\([^;]*?\)\)\s*)?;", text):
            fields = []
            try:
                for decl in m.group(1).split(";"):
                    decl = " ".join(decl.split())
                    decl = re.sub(r"^(?:(?:const|volatile|struct) )+", "", decl)     # struct Seg *next, const char *name
                    if not decl:
                        continue
                    if "(*" in decl:
                        fields.append(("fn", 0))
                        continue
                    base, _, rest = decl.partition(" ")
                    if base in ("unsigned", "signed", "const", "volatile"):
                        raise ValueError(decl)
                    for var in rest.split(","):
                        var = var.strip()
                        ptr = var.startswith("*")
                        # (a dimension may be an expression of numbers: pad98[0x9C - 0x98])
                        dims = [int(x, 0) if re.match(r"\s*(0x[0-9A-Fa-f]+|\d+)\s*$", x) else
                                int(eval(x, {"__builtins__": {}})) if re.match(r"^[0-9A-Fa-fx+\-* ()]+$", x) else int(x, 0)
                                for x in re.findall(r"\[([^\]]+)\]", var)]
                        t = "ptr" if ptr else self.CTYPES.get(base, base)
                        if not ptr and t not in SCALARS and t not in self.structs:
                            raise ValueError(decl)
                        n = 1
                        for d in dims:
                            n *= d
                        fields.append((t, n if dims else 0))
            except ValueError:
                continue
            self.structs.setdefault(m.group(2), fields)

    def size(self, t: str) -> int:
        if t in self.fntypes:
            return 4
        if t.endswith("*") or t in SCALARS:
            return 4 if t.endswith("*") else SCALARS[t][0]
        o = 0
        for ft, n in self.structs[t]:
            al = self.align(ft)
            o = (o + al - 1) // al * al + self.size(ft) * (n or 1)
        al = self.align(t)
        return (o + al - 1) // al * al

    def align(self, t: str) -> int:
        if t in self.fntypes:
            return 4
        if t.endswith("*") or t in SCALARS:
            return self.size(t)
        return max(self.align(ft) for ft, _ in self.structs[t])


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("module")
    ap.add_argument("lo", type=lambda s: int(s, 16))
    ap.add_argument("hi", type=lambda s: int(s, 16))
    ap.add_argument("types", type=Path)
    ap.add_argument("--rodata", nargs=2, type=lambda s: int(s, 16), metavar=("LO", "HI"), default=(0, 0))
    ap.add_argument("--root", type=Path, help="read the split from this checkout")
    ap.add_argument("--c", type=Path, action="append", default=[],
                    help="a C file or header whose simple typedef structs TYPES may use")
    args = ap.parse_args()
    if args.root:
        gen_data.ROOT = args.root.resolve()
    ty = Types(args.types)
    for c in args.c:
        ty.load_c(c)
    # scalar typedefs as their basic types, in objects and struct fields
    ty.objs = {k: (ty.tdefs.get(t, t), n) for k, (t, n) in ty.objs.items()}
    ty.structs = {k: [(ty.tdefs.get(ft, ft), n) for ft, n in v] for k, v in ty.structs.items()}
    mod = gen_data.Module(args.module)
    objs = gen_data.objects(mod, args.lo, args.hi)
    # a typed address inside an object starts an object of its own
    for key in ty.objs:
        if not re.match(r"^[0-9A-F]{8}$", key):
            continue
        a = int(key, 16)
        for i, (s, size, span, name) in enumerate(objs):
            if s < a < s + size:
                objs[i:i + 1] = [(s, a - s, a - s, name), (a, s + size - a, s + span - a, f"D_{a:08X}")]
                break
    # a word the split left a number that is the address of one of the module's functions is
    # a pointer to it (the split marks only some tables' words as pointers)
    func_at = {a: n for n, a in mod.funcs.items()}

    def is_code(w: int) -> bool:
        return w not in mod.ptrs and mod.word(w) in func_at

    # a number array ends before the first pointer in it (a table of its own: the split's labels
    # can miss one), at its last nonzero value
    for s, size, span, name in objs:
        key = f"{s:08X}"
        spec = ty.objs.get(key) or ty.objs.get(name)
        if not spec or spec[0] not in SCALARS or spec[0] in ("ptr", "fn") or key in ty.counts:
            continue
        p = next((w for w in range(s, s + size, 4) if w in mod.ptrs or is_code(w)), None)
        if p is not None and SCALARS[spec[0]][0] < 4:
            # bytes or halfwords splat took for addresses among more plain numbers (text codes:
            # .word D_004D004C): no table after them, the array goes on
            rest = range(p, s + size - size % 4, 4)
            if any(w not in mod.ptrs and not is_code(w) and mod.word(w) for w in rest):   # (a plain number among them)
                continue
        if p is None or (spec[0] in ("s32", "u32") and (s in mod.ptrs or key in ty.arrays)):
            continue    # (a table of pointers the code reads as numbers: they stay pointers)
        last = max([x for x in range(s, p) if mod.byte(x)] + [s])
        sz = ty.size(spec[0])
        ty.counts[key] = max(1, (last + 1 - s + sz - 1) // sz)
    # a typed object takes in the labels inside its type (the fields of a struct, the rows of an array)
    i = 0
    while i < len(objs):
        s, size, span, name = objs[i]
        spec = ty.objs.get(f"{s:08X}") or ty.objs.get(name)
        if spec:
            need = ty.size(spec[0]) * ty.counts.get(f"{s:08X}", 1)
            # (a typed object that starts inside the type is a field of it: its name becomes an
            # alias, as for D_00432248, the color of MenuDef D_00432230)
            while size < need and i + 1 < len(objs) and (
                    f"{objs[i + 1][0]:08X}" not in ty.objs or objs[i + 1][0] < s + need):
                ns, nsize, nspan, nname = objs.pop(i + 1)
                if f"{ns:08X}" in ty.objs:
                    print(f"{nname}: inside {name} ({spec[0]}): an alias", file=sys.stderr)
                size, span = ns + nsize - s, ns + nspan - s
            # (an array whose last row a label cuts: the row goes on into the next object)
            esz = ty.size(spec[0])
            while not ty.counts.get(f"{s:08X}") and size % esz and i + 1 < len(objs) and                     f"{objs[i + 1][0]:08X}" not in ty.objs and any(mod.byte(x) for x in range(s + size - size % esz, s + size)):
                ns, nsize, nspan, _ = objs.pop(i + 1)
                size, span = ns + nsize - s, ns + nspan - s
            objs[i] = (s, size, span, name)
            count = ty.counts.get(f"{s:08X}")
            end = s + need
            if count and end < s + size and any(mod.byte(x) for x in range(end, s + size)):
                # more after the counted rows: an object of its own at the next aligned address
                # (MWCC aligns an object of 16 bytes or more to 16, a smaller one to 8)
                first = next(x for x in range(end, s + size) if mod.byte(x))
                nxt = max((end + 7) & ~7, first & ~7)
                if any(mod.byte(x) for x in range(end, nxt)):
                    raise SystemExit(f"{name}: nonzero bytes between its {count} rows and {nxt:08X}")
                objs[i] = (s, nxt - s, nxt - s, name)
                objs.insert(i + 1, (nxt, s + size - nxt, s + span - nxt, f"D_{nxt:08X}"))
        i += 1
    obj_at = {s: (size, name) for s, size, _, name in objs}
    # a number splat took for an address: in an object of mostly plain numbers, a word
    # with a symbol whose low 12 bits a plain number of the object shares (the 0x...000
    # coordinates and the 0x..0124 commands of a script: ColorEdit_Draw = 0x0018E000)
    fake: set[int] = set()
    for s, size, _, name in objs:
        ws = range(s, s + size - size % 4, 4)
        syms = [w for w in ws if w in mod.ptrs]
        plain = [mod.word(w) for w in ws if w not in mod.ptrs and not is_code(w) and mod.word(w)]
        if syms and len(plain) > 2 * len(syms):
            low = {v & 0xFFF for v in plain}
            fake |= {w for w in syms if mod.word(w) & 0xFFF in low or mod.word(w) & 3}   # (or not word-aligned: text codes)
            # (or a neighbour's low halfword within 0x10 of its own: key codes 0x005A0470, 0x00000471)
            more = True
            while more:
                more = False
                for w in syms:
                    if w in fake:
                        continue
                    for nb in (w - 4, w + 4):
                        if s <= nb < s + size - 3 and (nb not in mod.ptrs or nb in fake) and \
                                abs((mod.word(nb) & 0xFFFF) - (mod.word(w) & 0xFFFF)) <= 0x10 and mod.word(nb):
                            fake.add(w)
                            more = True
                            break
    fake |= ty.numbers
    starts = sorted(obj_at)
    elem = {s: "u32" for s in starts}
    newname = {}
    typed = {}
    for s, size, _, name in objs:
        spec = ty.objs.get(f"{s:08X}") or ty.objs.get(name)
        if spec:
            typed[s] = spec[0]
            if spec[1]:
                newname[s] = spec[1]
                obj_at[s] = (size, spec[1])

    def owner(a: int):
        import bisect
        i = bisect.bisect_right(starts, a) - 1
        if i >= 0 and a < starts[i] + obj_at[starts[i]][0]:
            return starts[i], obj_at[starts[i]][1]
        return None

    lines: list[str] = []
    emitted: set[int] = set()
    forward: list[str] = []

    def ptr_text(w: int, ctype: str | None) -> str:
        """The pointer at word address w as C, cast to ctype when a cast is needed."""
        if w not in mod.ptrs and is_code(w):
            fname = func_at[mod.word(w)]
            return fname if ctype == "fn" else (f"({ctype}){fname}" if ctype else f"(u32){fname}")
        if w not in mod.ptrs:
            v = mod.word(w)
            if v == 0:
                return "NULL" if ctype else "0"
            return f"({ctype})0x{v:08X}" if ctype else f"0x{v:08X}"
        sym, add = mod.ptrs[w]
        o = None
        a = mod.addr_of.get(sym)
        if a is None:
            m = re.match(r"(?:D|func|jtbl)_([0-9A-Fa-f]{6,8})$", sym)
            a = int(m.group(1), 16) if m else None
        if a is not None:
            o = owner(a + add)
        if o is not None:
            start, oname = o
            off = a + add - start
            if start not in emitted and oname not in forward:
                forward.append(oname)
            if start in typed:
                sz = ty.size(typed[start])
                if off == 0:
                    if ctype and ctype not in ("void *", "fn") and                             ctype.replace(" ", "")[:-1] != typed[start].replace(" ", ""):
                        return f"({ctype}){oname}"      # a table of mixed objects: the code's type
                    key, tt = f"{start:08X}", typed[start]
                    if obj_at[start][0] // sz == 1 and key not in ty.arrays and (
                            key in ty.scalars or (tt not in SCALARS and not tt.endswith("*"))):
                        return f"&{oname}" if ctype else f"(u32)&{oname}"   # a single struct
                    return oname if ctype else f"(u32){oname}"
                if off % sz == 0:
                    return f"&{oname}[{off // sz}]" if ctype else f"(u32)&{oname}[{off // sz}]"
                return f"({ctype or 'u32'})((u8 *){oname} + 0x{off:X})"
            if off == 0:
                return f"({ctype}){oname}" if ctype else f"(u32){oname}"
            if off % 4 == 0:
                return f"({ctype})&{oname}[{off // 4}]" if ctype else f"(u32)&{oname}[{off // 4}]"
            return f"({ctype or 'u32'})((u8 *){oname} + 0x{off:X})"
        text = gen_data.pointer(mod, sym, add, owner, elem, obj_at, set(), set(), lines,
                                args.rodata[0], args.rodata[1])
        if ctype and text.startswith("(u32)"):
            body = text[5:]
            return body if ctype in ("fn",) else f"({ctype}){body}"
        return text

    def value(a: int, t: str) -> tuple[str, int]:
        """C text for one value of type t at address a, and its size."""
        if t.endswith("*") or t in ("ptr", "fn") or t in ty.fntypes:
            ctype = "fn" if t == "fn" or t in ty.fntypes else ("void *" if t == "ptr" else t[:-1] + " *")
            txt = ptr_text(a, ctype)
            if t in ty.fntypes and txt in ty.fnsig and t in ty.fnsig and ty.fnsig[txt] != ty.fnsig[t]:
                txt = f"({t}){txt}"      # another prototype than the table's type: cast
            if t == "fn" and txt.startswith("(fn)"):
                txt = "(void *)" + txt[4:]
            return txt, 4
        if t in SCALARS:
            n, code = SCALARS[t]
            if n == 4 and t in ("s32", "u32") and a in mod.ptrs and a not in fake:
                # a pointer the code reads as a number: the symbol, cast
                txt = ptr_text(a, None)
                return (txt.replace("(u32)", "(s32)", 1) if t == "s32" else txt), 4
            if n == 4:
                v = mod.word(a)
                if t == "f32":
                    try:
                        return f32_text(v), 4
                    except ValueError:
                        o = max(x for x in obj_at if x <= a)
                        raise SystemExit(f"{obj_at[o][1]}: float bits {v:08X} at {a:08X}: not a float")
                if t == "s32":
                    v = struct.unpack("<i", struct.pack("<I", v))[0]
                    return (str(v) if abs(v) < 0x10000 else (f"0x{v:X}" if v > 0 else f"-0x{-v:X}")), 4
                return (str(v) if v < 0x10000 else f"0x{v:08X}"), 4
            w = a - a % 4
            if n < 4 and w in mod.ptrs and w not in fake:
                # a word the split relocates (a pointer) inside a field narrower than a word:
                # the struct does not fit here (a pointer field, or the object ends earlier)
                o = max(x for x in obj_at if x <= a)
                raise SystemExit(f"{obj_at[o][1]}: a pointer at {w:08X} lands in a {t} field at {a:08X}: "
                                 "fix the struct (or the object's end) in the types file")
            raw = bytes(mod.byte(a + i) for i in range(n))
            v = struct.unpack("<" + code, raw)[0]
            return (str(v) if abs(v) < 0x100 or code in "bh" else f"0x{v:X}"), n
        parts = []
        o = a
        for ft, cnt in ty.structs[t]:
            al = ty.align(ft)
            o = (o + al - 1) // al * al
            if cnt:
                sub = []
                for _ in range(cnt):
                    txt, n = value(o, ft)
                    sub.append(txt)
                    o += n
                parts.append("{" + ", ".join(sub) + "}")
            else:
                txt, n = value(o, ft)
                parts.append(txt)
                o += n
        return "{" + ", ".join(parts) + "}", ty.size(t)

    def text_ok(b):
        """Text: CP932 without control characters other than \\n, \\t, \\r."""
        try:
            t = b.decode("cp932")
        except UnicodeDecodeError:
            return False
        return all(c in "\n\t\r" or unicodedata.category(c)[0] != "C" for c in t)

    for s, size, span, name in objs:
        name = obj_at[s][1]
        if typed.get(s) == "char":
            # text: a string literal when the object is one NUL-terminated string (and padding)
            n = len(mod.data[s - mod.load:s - mod.load + size].split(b"\0", 1)[0])
            if n < size and mod.cstring(s, size) is not None and not any(mod.byte(x) for x in range(s + n, s + size)):
                # (at 8 mod 16, a string under 16 bytes in a 16-byte slot: `char X[]`, which
                # MWCC 8-aligns; the words' size would 16-align it: lbguild's 15-byte texts)
                decl = "[]" if s % 16 == 8 and n + 1 < 16 <= size else f"[0x{size:X}]"
                lines.append(f"char {name}{decl} = {gen_data.c_string(mod, s, n)};")
                emitted.add(s)
                continue
            if n == size and span > size and mod.byte(s + n) == 0 and mod.cstring(s, n + 1) is not None:
                # the size was cut to keep a 16-byte array off a 16-byte boundary: the string
                # alone (its NUL in the padding) keeps the object under 16 bytes
                lines.append(f"char {name}[] = {gen_data.c_string(mod, s, n)};")
                emitted.add(s)
                continue
            # several strings (the later ones without labels), or text in Shift-JIS
            # extensions (CP932: circled digits): one literal, "\000" between the strings
            raw = mod.data[s - mod.load:s - mod.load + size]
            end = len(raw.rstrip(b"\0"))
            parts = raw[:end].split(b"\0")
            if end and all(text_ok(p) for p in parts) and (end < size or (span > size and mod.byte(s + size) == 0)):
                lit, o = [], s
                for p in parts:
                    lit.append(gen_data.c_string(mod, o, len(p))[1:-1])
                    o += len(p) + 1
                decl = f"[0x{size:X}]" if end < size else "[]"
                lines.append(f'char {name}{decl} = "' + "\\000".join(lit) + '";')
                emitted.add(s)
                continue
        if s in typed:
            t = typed[s]
            sz = ty.size(t)
            count = ty.counts.get(f"{s:08X}")
            if size % sz or (count and count * sz < size):
                # the declared size covers padding up to the next object
                content = count * sz if count else size - size % sz
                if any(mod.byte(x) for x in range(s + content, s + size)):
                    raise SystemExit(f"{name}: {size:#x} bytes is not a whole number of {t} ({sz:#x})")
                size = content
            n = size // sz
            items = [value(s + i * sz, t)[0] for i in range(n)]
            inner = re.match(r"^(\w+)\*(\d+)$", t)
            ctype = "void *" if t == "ptr" else ("void *" if t == "fn" else t if t in ty.fntypes else (t[:-1] + " *" if t.endswith("*") else t))
            if inner:
                ctype = inner.group(1)
            decl = f"{ctype} {name}" if not ctype.endswith("*") else f"{ctype}{name}"
            if inner:
                rows = items
                dims = f"[{n}][{inner.group(2)}]"
                lines.append(f"{decl}{dims} = " + "{\n    " + ",\n    ".join(rows) + ",\n};")
            elif n == 1 and f"{s:08X}" not in ty.arrays and (f"{s:08X}" in ty.scalars or (t not in SCALARS and not t.endswith("*"))):
                lines.append(f"{decl} = {items[0]};")
            else:
                per = 1 if t in ty.structs else (4 if t in ("f32", "ptr", "fn") or t.endswith("*") or sz == 4 else 8)
                rows = [", ".join(items[i:i + per]) for i in range(0, n, per)]
                lines.append(f"{decl}[{n}] = {{\n    " + ",\n    ".join(rows) + ",\n};")
            emitted.add(s)
            continue
        # untyped: a table of functions (every word 0 or a function) as `void *`, else as
        # gen_data.py writes it
        words = range(s, s + size - size % 4, 4)
        if not size % 4 and any(w in mod.ptrs or is_code(w) for w in words) and all(
                (w in mod.ptrs and mod.ptrs[w][1] == 0 and mod.ptrs[w][0] in mod.funcs) or is_code(w) or
                (w not in mod.ptrs and mod.word(w) == 0) for w in words):
            items = [ptr_text(w, "fn") for w in words]
            rows = [", ".join(items[i:i + 4]) for i in range(0, len(items), 4)]
            lines.append(f"void *{name}[{len(items)}] = {{\n    " + ",\n    ".join(rows) + ",\n};")
            emitted.add(s)
            continue
        body = []
        for w in range(s, s + size - size % 4, 4):
            body.append(f"0x{mod.word(w):08X}" if w in fake else ptr_text(w, None))
        rows = [", ".join(body[i:i + 4]) for i in range(0, len(body), 4)]
        if size % 4:
            raise SystemExit(f"{name}: {size:#x} bytes, not words: give it a type")
        lines.append(f"u32 {name}[{size // 4}] = {{\n    " + ",\n    ".join(rows) + ",\n};")
        emitted.add(s)
    head = []
    for n in forward:
        s = next(x for x in starts if obj_at[x][1] == n)
        t = typed.get(s)
        # a pointer to an object defined further down: declare it first
        ct = "u32" if not t else ("void *" if t in ("ptr", "fn") else (t[:-1] + " *" if t.endswith("*") else t))
        # (as it is defined: `void *X[8]` for a table of functions, `T X` for one struct)
        d = next((m for m in (re.match(r"([^=\n]*?)\b%s((?:\[\d*\])*) = " % re.escape(n), x) for x in lines) if m), None)
        if d:
            head.append(f"extern {d.group(1)}{n}{re.sub(r'^[[]\d*[]]', '[]', d.group(2))};")
            continue
        head.append(f"extern {ct}{'' if ct.endswith('*') else ' '}{n}[];")
    lines = head + lines
    sys.stdout.write("\n".join(lines) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
