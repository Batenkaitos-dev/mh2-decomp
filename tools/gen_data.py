#!/usr/bin/env python3
"""Write C definitions for a range of a module's .data (or .bss), so that a C
file can own it (CONTRIBUTING.md, "Functions with switch tables or strings").

    python tools/gen_data.py MODULE data LO HI     # .data objects in [LO, HI)
    python tools/gen_data.py MODULE bss LO HI      # .bss objects, highest first

It needs a current split (asm/MODULE/data/*.s) and build/orig/MODULE.bin.

.data: each object becomes `u32 D_XXXXXXXX[N] = {...};` (or `u8`/`char`),
in address order, with pointers written as C: a pointer to a string of the
module's .rodata becomes the string literal (MWCC emits a literal at its
first use, so the file's strings come out in the order the tables use
them), a pointer to a data object `(u32)D_...` (`&D_...[i]` inside one), a
pointer to a function `(u32)func_...`. The objects are splat's labels,
merged where MWCC could not have placed a separate object: MWCC aligns an
object of 16 bytes or more to 16 and a smaller one to 8 (4 raised to 8 by
mwcc_cc.py, as the game's linker did), so a label at an address that isn't
8-aligned, or at 8 mod 16 with 16 bytes or more of content, is part of the
object before it. The declared size runs to the next object, except that
an object at 8 mod 16 is cut to its content (under 16 bytes): the next
object's alignment supplies the padding.

A string that is the target of pointers but comes out of first-use order
(a literal used later in the file, emitted earlier in the game) is printed
as a named `const char` array instead (see --named); the listing says
which. Strings used by code as well as data are left to the code (the
function's .s brings them while it is assembly).

.bss: `u8 D_XXXXXXXX[size];` from the highest address down (MWCC emits
uninitialized variables last-declared first).

The output is plain; give the objects real types when their functions are
worked on (the bytes and relocations are what must stay).
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from module_symbols import symbol_files  # noqa: E402  (a module's symbol files, from its YAML)

ROOT = Path(__file__).resolve().parent.parent

DLABEL = re.compile(r"^dlabel (\S+)")
GLABEL = re.compile(r"^glabel (\S+)")
ADDR = re.compile(r"/\*\s+[0-9A-F]+\s+([0-9A-F]{8})\b")
WORDSYM = re.compile(r"/\*\s+[0-9A-F]+\s+([0-9A-F]{8})\s+[0-9A-F]{8}\s+\*/\s+\.word\s+([A-Za-z_.$][\w.$]*)(?:\s*\+\s*(0x[0-9A-Fa-f]+|\d+))?\s*$")
BSSLINE = re.compile(r"/\*\s+([0-9A-F]{8})\s+\*/\s+\.space\s+(0x[0-9A-Fa-f]+|\d+)")
SECTION = re.compile(r'^\s*\.section\s+([^\s,"]+)')
INSN_REF = re.compile(r"%(?:hi|lo)\(([A-Za-z_.$][\w.$]*)")
SYM_LINE = re.compile(r"^\s*([A-Za-z_.$][\w.$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;")


def image(module: str) -> tuple[bytes, int]:
    if module == "main":
        # the executable: its code segment starts at file offset `start`, at `vram` (config/main.yaml)
        import yaml
        seg = next(s for s in yaml.safe_load((ROOT / "config/main.yaml").read_text(encoding="utf-8"))["segments"]
                   if isinstance(s, dict) and s.get("type") == "code")
        return (ROOT / "build/orig/SLPM_662.80").read_bytes()[seg["start"]:], seg["vram"]
    data = (ROOT / f"build/orig/{module}.bin").read_bytes()
    return data, struct.unpack_from("<I", data, 8)[0]


class Module:
    def __init__(self, module: str):
        self.module = module
        self.data, self.load = image(module)
        self.labels: dict[int, str] = {}      # data labels: address -> name
        self.kind: dict[int, str] = {}        # address -> '.data' / '.rodata' / '.bss'
        self.funcs: dict[str, int] = {}
        self.ptrs: dict[int, tuple[str, int]] = {}   # word address -> (symbol, addend)
        self.bss: list[tuple[int, int, str]] = []
        self.code_refs: set[str] = set()
        seen: set[str] = set()
        for path in sorted((ROOT / "asm" / module).glob("*.s")) + sorted((ROOT / "asm" / module / "data").glob("*.s")):
            sec = None
            kind = ".rodata" if "rodata" in path.name else ".bss" if path.name.endswith(".bss.s") else ".data"
            cur = None
            for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
                m = SECTION.match(line)
                if m:
                    sec = m.group(1)
                m = GLABEL.match(line)
                if m:
                    cur = ("f", m.group(1))
                    continue
                m = DLABEL.match(line)
                if m:
                    cur = ("d", m.group(1))
                    continue
                if cur is None:
                    continue
                if cur[0] == "f":
                    m = ADDR.search(line)
                    if m and cur[1] not in self.funcs:
                        self.funcs[cur[1]] = int(m.group(1), 16)
                    for r in INSN_REF.findall(line):
                        self.code_refs.add(r)
                    continue
                if path.parent.name != "data":
                    # data in a C file's full disassembly (a claimed range)
                    kind = {".data": ".data", ".bss": ".bss"}.get(sec, ".rodata")
                    m = BSSLINE.search(line)
                    if m:
                        a = int(m.group(1), 16)
                        if cur[1] not in seen:
                            seen.add(cur[1])
                            self.labels[a] = cur[1]
                            self.kind[a] = ".bss"
                            self.bss.append((a, int(m.group(2), 0), cur[1]))
                        continue
                m = BSSLINE.search(line)
                if m:
                    a = int(m.group(1), 16)
                    self.labels.setdefault(a, cur[1])
                    self.kind[a] = ".bss"
                    self.bss.append((a, int(m.group(2), 0), cur[1]))
                    continue
                m = ADDR.search(line)
                if m:
                    a = int(m.group(1), 16)
                    if cur[1] not in seen:
                        seen.add(cur[1])
                        self.labels[a] = cur[1]
                        self.kind[a] = kind
                m = WORDSYM.search(line)
                if m:
                    self.ptrs[int(m.group(1), 16)] = (m.group(2), int(m.group(3), 0) if m.group(3) else 0)
        self.addr_of = {n: a for a, n in self.labels.items()}
        self.addr_of.update(self.funcs)
        # every symbol file of main, sub_main and the module (their YAMLs'
        # lists: the library names are in sdk_*.txt, gcc_*.txt, cri_*.txt, net_*.txt)
        for m in ("main", "sub_main", module):
            for p in symbol_files(m):
                if p.exists():
                    for line in p.read_text(encoding="utf-8").splitlines():
                        mm = SYM_LINE.match(line)
                        if mm:
                            self.addr_of.setdefault(mm.group(1), int(mm.group(2), 16))

    def byte(self, a: int) -> int:
        return self.data[a - self.load]

    def word(self, a: int) -> int:
        return struct.unpack_from("<I", self.data, a - self.load)[0]

    def cstring(self, a: int, limit: int) -> str | None:
        """The string at a, if the object there is one NUL-terminated string
        followed by zeros only (up to limit), else None."""
        raw = self.data[a - self.load:a - self.load + limit]
        if not raw or raw[0] == 0 and any(raw):
            return None
        end = raw.find(b"\0")
        if end < 0 or any(raw[end:]):
            return None
        s = raw[:end]
        try:
            text = s.decode("shift_jis")
        except UnicodeDecodeError:
            return None
        if any(ord(c) < 0x20 and c not in "\n\t\r" for c in text):
            return None
        return text


def c_string(mod: Module, a: int, n: int) -> str:
    """C literal for the n bytes at a (without the NUL): UTF-8 where the
    Shift-JIS round trip is exact and the second byte isn't a backslash,
    octal escapes otherwise."""
    raw = mod.data[a - mod.load:a - mod.load + n]
    out = []
    i = 0
    while i < len(raw):
        b = raw[i]
        if b < 0x80:
            ch = chr(b)
            out.append({"\n": "\\n", "\t": "\\t", "\r": "\\r", '"': '\\"', "\\": "\\\\"}.get(ch, ch))
            i += 1
            continue
        if 0xA1 <= b <= 0xDF:                     # half-width katakana
            seq = raw[i:i + 1]
        else:
            seq = raw[i:i + 2]
        try:
            ch = seq.decode("shift_jis")
            ok = ch.encode("shift_jis") == seq and b"\\" not in seq[1:]
        except UnicodeDecodeError:
            ok = False
        out.append(ch if ok else "".join(f"\\{x:03o}" for x in seq))
        i += len(seq)
    return '"' + "".join(out) + '"'


def objects(mod: Module, lo: int, hi: int) -> list[tuple[int, int, int, str]]:
    """[(start, declared size, span, name)] for the .data objects in [lo, hi)."""
    labs = sorted(a for a in mod.labels if lo <= a < hi and mod.kind.get(a) != ".bss")
    if not labs or labs[0] != lo:
        labs = [lo] + labs
    bounds = labs + [hi]

    def content(a: int, b: int) -> int:
        """Bytes from a to the last nonzero byte or pointer before b."""
        n = 0
        for x in range(a, b):
            if mod.byte(x) or (x - x % 4) in mod.ptrs:
                n = x - a + 1
        return n

    def need(a: int, b: int) -> int:
        """The object's bytes: a string's with its NUL (`char X[]`: lbguild's
        15-byte texts at 8 mod 16), else the content in words."""
        text = mod.cstring(a, b - a)
        if text:
            return len(text.encode("shift_jis")) + 1
        return (content(a, b) + 3) & ~3

    objs = []
    for i, a in enumerate(labs):
        b = bounds[i + 1]
        if objs:
            s0, _, _, _ = objs[-1]
            # An object at 8 mod 16 is under 16 bytes, and the next object
            # starts at most 24 bytes after it (8 + padding to 16).
            # (content in words: the object is declared as u32 words, so 13-15
            # bytes of content make a 16-byte object, which MWCC 16-aligns)
            must_merge = a % 8 != 0 or (a % 16 == 8 and (need(a, b) > 15 or b - a > 24))
            if must_merge:
                objs[-1] = (s0, b - s0, b - s0, objs[-1][3])
                continue
        objs.append((a, b - a, b - a, mod.labels.get(a, f"D_{a:08X}")))
    # A merge can make an object at 8 mod 16 reach 16 bytes of content (a
    # chain of short lists, gm_sub 0x92BDC8): MWCC would 16-align it, so it
    # is part of the object before it too. Repeat until none is left.
    i = 1
    while i < len(objs):
        s = objs[i][0]
        e = objs[i + 1][0] if i + 1 < len(objs) else hi
        if s % 16 == 8 and need(s, e) > 15:
            s0 = objs[i - 1][0]
            objs[i - 1] = (s0, e - s0, e - s0, objs[i - 1][3])
            del objs[i]
            i = max(i - 1, 1)
            continue
        i += 1
    # the previous object's span ends at the next start
    out = []
    for i, (s, _, _, name) in enumerate(objs):
        e = objs[i + 1][0] if i + 1 < len(objs) else hi
        span = e - s
        size = span
        if s % 16 == 8 and span >= 16:
            # 9..15 bytes: the next object (8- or 16-aligned) then starts
            # 16 or 24 bytes on
            size = max((content(s, e) + 3) & ~3, 12)
        out.append((s, size, span, name))
    return out


def gen_data(mod: Module, lo: int, hi: int, named: set[int], ro_lo: int, ro_hi: int) -> str:
    objs = objects(mod, lo, hi)
    obj_at = {s: (size, name) for s, size, _, name in objs}
    starts = sorted(obj_at)

    def owner(a: int) -> tuple[int, str] | None:
        import bisect
        i = bisect.bisect_right(starts, a) - 1
        if i >= 0 and a < starts[i] + obj_at[starts[i]][0]:
            return starts[i], obj_at[starts[i]][1]
        return None

    # which rodata targets are strings, and their first use (in data order)
    lines = []
    decls = []
    emitted_named = set()
    elem = {}
    for s, size, span, name in objs:
        has_ptr = any(s <= p < s + size for p in mod.ptrs)
        elem[s] = "u32" if size % 4 == 0 and (has_ptr or size >= 4) else "u8"
    for s, size, span, name in objs:
        body = []
        et = elem[s]
        if et == "u32":
            for w in range(s, s + size, 4):
                if w in mod.ptrs:
                    sym, add = mod.ptrs[w]
                    body.append(pointer(mod, sym, add, owner, elem, obj_at, named, emitted_named, lines, ro_lo, ro_hi))
                else:
                    body.append(f"0x{mod.word(w):08X}")
            per = 4
        else:
            text = mod.cstring(s, size)
            if text is not None and len(text.encode("shift_jis")) + 1 <= size:
                n = len(mod.data[s - mod.load:].split(b"\0", 1)[0])
                lines.append(f"char {name}[0x{size:X}] = {c_string(mod, s, n)};")
                continue
            body = [f"0x{mod.byte(x):02X}" for x in range(s, s + size)]
            per = 12
        rows = [", ".join(body[i:i + per]) for i in range(0, len(body), per)]
        lines.append(f"{et} {name}[{size // (4 if et == 'u32' else 1)}] = {{\n    " + ",\n    ".join(rows) + ",\n};")
    return "\n".join(lines) + "\n"


def pointer(mod, sym, add, owner, elem, obj_at, named, emitted_named, lines, ro_lo, ro_hi) -> str:
    a = mod.addr_of.get(sym)
    if a is None:
        m = re.match(r"(?:D|func|jtbl)_([0-9A-Fa-f]{6,8})$", sym)
        a = int(m.group(1), 16) if m else None
    if a is not None and add == 0 and mod.kind.get(a) == ".rodata" and ro_lo <= a < ro_hi:
        # a string of the file's .rodata
        nxt = min([x for x in mod.labels if x > a] + [a + 0x1000])
        text = mod.cstring(a, nxt - a)
        if text is not None and sym not in mod.code_refs:
            if a in named:
                if a not in emitted_named:
                    emitted_named.add(a)
                    n = len(mod.data[a - mod.load:].split(b"\0", 1)[0])
                    lines.append(f"const char {sym}[] = {c_string(mod, a, n)};")
                return f"(u32){sym}"
            n = len(mod.data[a - mod.load:].split(b"\0", 1)[0])
            return f"(u32){c_string(mod, a, n)}"
    if a is not None:
        o = owner(a + add)
        if o is not None:
            start, oname = o
            off = a + add - start
            if off == 0:
                return f"(u32){oname}"
            if elem[start] == "u32" and off % 4 == 0:
                return f"(u32)&{oname}[{off // 4}]"
            return f"(u32)((u8 *){oname} + 0x{off:X})"
    if add:
        return f"(u32)((u8 *){sym} + 0x{add:X})"
    return f"(u32){sym}"


def gen_bss(mod: Module, lo: int, hi: int) -> str:
    labs = sorted(a for a, k in mod.kind.items() if k == ".bss" and lo <= a < hi)
    labs = [a for a in labs]
    out = []
    bounds = labs + [hi]
    objs = []
    for i, a in enumerate(labs):
        b = bounds[i + 1]
        if objs and a % 8:
            objs[-1] = (objs[-1][0], b - objs[-1][0], objs[-1][2])
            continue
        objs.append((a, b - a, mod.labels[a]))
    for a, size, name in reversed(objs):
        if a % 16 == 8 and size >= 16:
            size = 8
        out.append(f"u8 {name}[0x{size:X}];")
    return "\n".join(out) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("module")
    ap.add_argument("what", choices=("data", "bss"))
    ap.add_argument("lo", type=lambda s: int(s, 16))
    ap.add_argument("hi", type=lambda s: int(s, 16))
    ap.add_argument("--rodata", nargs=2, type=lambda s: int(s, 16), metavar=("LO", "HI"),
                    help="the file's .rodata range: pointers to strings there become literals")
    ap.add_argument("--named", nargs="*", default=[], type=lambda s: int(s, 16),
                    help="string addresses to define as named const arrays (out of first-use order)")
    ap.add_argument("--symbols", action="store_true",
                    help="print config/symbols lines (with size:) for the objects that other labels "
                         "fall inside, so that splat writes references to them as sym + offset")
    args = ap.parse_args()
    mod = Module(args.module)
    if args.symbols:
        if args.what == "bss":
            labs = sorted(a for a, k in mod.kind.items() if k == ".bss" and args.lo <= a < args.hi)
            objs, bounds = [], labs + [args.hi]
            for i, a in enumerate(labs):
                if objs and a % 8:
                    objs[-1] = (objs[-1][0], bounds[i + 1] - objs[-1][0], objs[-1][2])
                    continue
                objs.append((a, bounds[i + 1] - a, mod.labels[a]))
            spans = [(a, size, name) for a, size, name in objs]
        else:
            spans = [(s, span, name) for s, _, span, name in objects(mod, args.lo, args.hi)]
        for s, span, name in spans:
            inner = [a for a in mod.labels if s < a < s + span]
            if inner:
                print(f"{name} = 0x{s:08X}; // size:0x{span:X}")
        return 0
    if args.what == "bss":
        sys.stdout.write(gen_bss(mod, args.lo, args.hi))
    else:
        ro = args.rodata or (0, 0)
        sys.stdout.write(gen_data(mod, args.lo, args.hi, set(args.named), ro[0], ro[1]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
