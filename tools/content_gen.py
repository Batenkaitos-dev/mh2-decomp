#!/usr/bin/env python3
"""Write a C file's content from the user's copy of the game (CONTRIBUTING, "Content").

    python3 tools/content_gen.py src/MODULE/FILE.c ... [--stamp OUT] [--jobs N]

The repository holds no content of the game: no values of its data tables, no
text. A C file keeps each data object's declaration (type, name, size) and
takes the initializer from a file this tool writes at build time:

    s16 gSelHeaderLayout[16][2] = {
    #include "select/006CAD00.inc"
    };

and a text the code uses (not data) is a macro, `TXT(0x0046A100)`, which
build/gen/MODULE/FILE.h defines as the string literal at that address.

For each such line, build/gen/MODULE/ADDR.inc gets the initializer of the
object at ADDR: the bytes of build/orig (the YAML's target_path), read as the
object's C type. The type comes from clang's parse of the file (the layout
is the EE's: every scalar aligned to its size, a struct to its largest
member). Values are written so that MWCC compiles them to the same bytes:
- integers as numbers, floats in a form that reads back to the same float;
- an array of 1-byte integers as a string literal (octal escapes);
- a word (a pointer, or a u32/s32) that holds the address of a name the
  file declares becomes that name (`(T)&NAME`, `(T)((char *)&NAME + 4)`);
  the default names (D_008DB7C0, func_0090A9C0) hold their address;
- a pointer into the file's own .rodata, outside every object the file
  declares, is a string literal there: MWCC emits it in the file's .rodata
  in first-use order, as the original source's literal did;
- any other word is a number.
Every generated file is ASCII: the game's Shift-JIS text is octal escapes,
which MWCC (and clang, for the port) copy as bytes.

The include directory is build/gen (MWCC's `-i build/gen`, clang's
`-I build/gen`). A file is rewritten only when its text changes.
"""

from __future__ import annotations

import argparse
import bisect
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GEN = ROOT / "build" / "gen"
sys.path.insert(0, str(ROOT / "tools"))

INC_LINE = re.compile(r'^[ \t]*#[ \t]*include[ \t]+"(\w+)/([0-9A-F]{8})\.inc"[ \t]*$', re.M)
TXT_USE = re.compile(r"\bTXT\(\s*0x([0-9A-Fa-f]{1,8})\s*\)")
TXT_HDR = re.compile(r'^[ \t]*#[ \t]*include[ \t]+"(\w+)/(\w+)\.h"[ \t]*$', re.M)
CLANG = os.environ.get("CLANG", "clang")
# The EE's data layout: mipsel's (o32) sizes and alignments are the same for
# every type the game's data uses (64-bit integers and doubles aligned to 8).
CLANG_TARGET = os.environ.get("CONTENT_GEN_TARGET", "--target=mipsel-linux-gnu")
INCLUDES = ["-nostdinc", "-I", "include"]
DEFS = ["-D__builtin_args_info(n)=0", "-D__builtin_next_arg(x)=((char *)0)"]


class GenError(Exception):
    pass


def write_if_changed(path: Path, text: str) -> None:
    if path.exists() and path.read_text(encoding="ascii", errors="replace") == text:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_text(text, encoding="ascii")
    tmp.replace(path)


def cstr(data: bytes) -> str:
    """A C string literal of exactly these bytes (no NUL added), in ASCII."""
    out = []
    for b in data:
        if b == 0x22:
            out.append('\\"')
        elif b == 0x5C:
            out.append("\\\\")
        elif 0x20 <= b < 0x7F and b != 0x3F:          # '?' escaped: no trigraphs
            out.append(chr(b))
        else:
            out.append("\\%03o" % b)
    return '"' + "".join(out) + '"'


def cstr_lines(data: bytes, indent: str = "", width: int = 96) -> str:
    """cstr(data), cut into adjacent literals of at most `width` columns."""
    s = cstr(data)[1:-1]
    pieces, cur = [], ""
    i = 0
    while i < len(s):
        tok = s[i:i + 4] if s[i] == "\\" and s[i + 1] in "01234567" else s[i:i + 2] if s[i] == "\\" else s[i]
        if len(cur) + len(tok) > width:
            pieces.append(cur)
            cur = ""
        cur += tok
        i += len(tok)
    pieces.append(cur)
    return ("\n" + indent).join('"%s"' % p for p in pieces)


# --- the module: its original, its sections, its names ----------------------

class Module:
    def __init__(self, name: str) -> None:
        import yaml
        import configure
        self.name = name
        self.yaml_path = configure.EXTERN_SYMBOLS.get(name, f"config/overlays/{name}.yaml")
        y = yaml.safe_load((ROOT / self.yaml_path).read_text(encoding="utf-8"))
        self.data = (ROOT / y["options"]["target_path"]).read_bytes()
        # (vram start, vram end, file offset) of each code segment; the
        # subsegments' (offset, type, name)
        self.ranges: list[tuple[int, int, int]] = []
        self.subs: list[tuple[int, str, str]] = []
        segs = y["segments"]
        starts = [s["start"] if isinstance(s, dict) else s[0] for s in segs]
        for i, s in enumerate(segs):
            if not isinstance(s, dict) or "vram" not in s or "start" not in s:
                continue
            end = next((st for st in starts[i + 1:] if st is not None and st > s["start"]), len(self.data))
            self.ranges.append((s["vram"], s["vram"] + end - s["start"], s["start"]))
            for ss in s.get("subsegments", []):
                if isinstance(ss, list) and len(ss) >= 2:
                    self.subs.append((ss[0], str(ss[1]), str(ss[2]) if len(ss) > 2 else ""))
                elif isinstance(ss, dict) and "start" in ss:
                    self.subs.append((ss["start"], str(ss.get("type")), str(ss.get("name", ""))))
            self.subs.append((end, "end", ""))
        self.subs.sort()
        # names: the module's own first, then those of the modules loaded with it
        self.names: dict[str, int] = {}
        mods = [name] + configure.LOADED_WITH.get(name, []) + [m for m in ("main", "sub_main") if m != name]
        for m in mods:
            try:
                for k, v in configure._module_symbols(m).items():
                    self.names.setdefault(k, v)
            except FileNotFoundError:
                pass
        for line in self._extra_symbol_lines(y):
            m = configure.SYM_LINE.match(line)
            if m:
                self.names.setdefault(m.group(1), int(m.group(2), 16))

    @staticmethod
    def _extra_symbol_lines(y):
        for p in y["options"].get("symbol_addrs_path", []):
            path = ROOT / p
            if path.exists():
                yield from path.read_text(encoding="utf-8").splitlines()

    def offset(self, addr: int) -> int:
        for lo, hi, off in self.ranges:
            if lo <= addr < hi:
                return off + addr - lo
        raise GenError(f"{self.name}: address {addr:#x} is outside the module's data")

    def read(self, addr: int, n: int) -> bytes:
        o = self.offset(addr)
        return self.data[o:o + n]

    def cstring(self, addr: int) -> bytes:
        o = self.offset(addr)
        e = self.data.index(b"\0", o)
        return self.data[o:e]

    def vram(self, off: int) -> int | None:
        for lo, hi, base in self.ranges:
            if base <= off < base + hi - lo:
                return lo + off - base
        return None

    def section_range(self, unit: str, kind: str) -> tuple[int, int] | None:
        """[lo, hi) vram of a C file's subsegment of a section (".rodata", ".data")."""
        for i, (off, t, n) in enumerate(self.subs):
            if t == kind and n == unit:
                lo = self.vram(off)
                hi = self.vram(self.subs[i + 1][0] - 1)
                return (lo, hi + 1) if lo is not None and hi is not None else None
        return None


# --- types: from clang's parse of the file -----------------------------------
# A type is ("int", size, signed) | ("float", size) | ("ptr", qualtype) |
# ("array", elem, n) | ("rec", record id)

BUILTIN = {
    "char": ("int", 1, True), "signed char": ("int", 1, True), "unsigned char": ("int", 1, False),
    "short": ("int", 2, True), "unsigned short": ("int", 2, False),
    "int": ("int", 4, True), "unsigned int": ("int", 4, False),
    "long": ("int", 4, True), "unsigned long": ("int", 4, False),
    "long long": ("int", 8, True), "unsigned long long": ("int", 8, False),
    "_Bool": ("int", 1, False), "float": ("float", 4), "double": ("float", 8), "long double": ("float", 8),
}
# clang's names of an anonymous record: `struct (unnamed struct at F:L:C)`, and for one
# nested in a named record `union TalkGift::(unnamed at F:L:C)`
ANON = re.compile(r"(?:struct|union|enum) (?:\w+::)*\((?:unnamed|anonymous)(?: (?:struct|union|enum))? at ([^)]+)\)")
QUALS = re.compile(r"\b(?:const|volatile|__restrict|restrict)\b")
# a default name (splat's), which holds the object's or function's address
DEFAULT_NAME = re.compile(r"^(?:D|func|jtbl)_([0-9A-Fa-f]{6,8})$")


class Types:
    """The records, typedefs and declarations of a file, from clang's JSON AST."""

    def __init__(self, ast: dict) -> None:
        self.records: dict[str, tuple[str, list]] = {}    # id -> (struct|union, [(name, qualtype, bits)])
        self.rec_name: dict[str, str] = {}                  # "struct X" -> id
        self.rec_loc: dict[str, str] = {}                   # "file:line:col" -> id
        self.typedefs: dict[str, object] = {}              # name -> qualtype or ("rec", id)
        self.enums: set[str] = set()
        self.vars: dict[str, list[dict]] = {}              # name -> VarDecl nodes
        self.funcs: set[str] = set()
        self._lay: dict[str, tuple] = {}
        self._walk(ast, {"file": "", "line": 0})

    @staticmethod
    def _loc(loc: dict, cur: dict) -> None:
        # clang's JSON leaves out a location's file and line when they are the
        # last ones it printed: follow them in the order it printed them
        for l in (loc.get("spellingLoc"), loc.get("expansionLoc"), loc):
            if isinstance(l, dict):
                if "file" in l:
                    cur["file"] = l["file"]
                if "line" in l:
                    cur["line"] = l["line"]

    def _walk(self, n: dict, cur: dict) -> None:
        here = None
        if "loc" in n:
            loc = n["loc"]
            self._loc(loc, cur)
            col = loc.get("col", (loc.get("spellingLoc") or {}).get("col"))
            here = f"{cur['file']}:{cur['line']}:{col}"
        if "range" in n:
            for k in ("begin", "end"):
                if k in n["range"]:
                    self._loc(n["range"][k], cur)
        k = n.get("kind")
        if k == "RecordDecl" and n.get("completeDefinition"):
            fields = []
            for c in n.get("inner", []):
                if c.get("kind") == "FieldDecl":
                    bits = None
                    if c.get("isBitfield"):
                        bits = int(c["inner"][0].get("value", "0")) if c.get("inner") else 0
                    fields.append((c.get("name", ""), c["type"].get("desugaredQualType", c["type"]["qualType"]), bits))
            tag = n.get("tagUsed", "struct")
            self.records[n["id"]] = (tag, fields)
            if n.get("name"):
                self.rec_name[tag + " " + n["name"]] = n["id"]
            if here:
                self.rec_loc[here] = n["id"]
        elif k == "TypedefDecl":
            inner = (n.get("inner") or [{}])[0]
            while inner.get("kind") == "ElaboratedType" and inner.get("inner"):
                inner = inner["inner"][0]
            if inner.get("kind") == "RecordType":
                self.typedefs[n["name"]] = ("rec", inner["decl"]["id"])
            else:
                self.typedefs[n["name"]] = n["type"].get("desugaredQualType", n["type"]["qualType"])
        elif k == "EnumDecl" and n.get("name"):
            self.enums.add("enum " + n["name"])
        elif k == "VarDecl" and n.get("name"):
            self.vars.setdefault(n["name"], []).append(n)
        elif k == "FunctionDecl" and n.get("name"):
            self.funcs.add(n["name"])
        for c in n.get("inner", []):
            if isinstance(c, dict):
                self._walk(c, cur)

    def parse(self, qt: str):
        s = " ".join(QUALS.sub("", qt).split())
        anon = None
        m = ANON.search(s)
        if m:
            anon = m.group(1)
            s = s[:m.start()] + "@ANON" + s[m.end():]
        if "(" in s:
            # a pointer to a function or to an array (or an array of them)
            m = re.search(r"\(([^()]*)\)", s)
            inner = m.group(1).replace(" ", "")
            if not inner.startswith("*"):
                raise GenError(f"type not understood: {qt}")
            t = ("ptr", None)
            for d in reversed(re.findall(r"\[(\d+)\]", inner)):
                t = ("array", t, int(d))
            return t
        dims = []
        while s.endswith("]"):
            m = re.match(r"^(.*?)\s*\[(\d*)\]$", s)
            if not m or not m.group(2):
                raise GenError(f"array without a size: {qt}")
            s = m.group(1)
            dims.insert(0, int(m.group(2)))
        s = s.strip()
        if s.endswith("*"):
            t = ("ptr", None if "@ANON" in s else s)
        elif s == "@ANON":
            t = ("rec", self._anon(anon, qt))
        elif s in BUILTIN:
            t = BUILTIN[s]
        elif s.startswith("enum ") or s in self.enums:
            t = ("int", 4, True)
        elif s.startswith(("struct ", "union ")):
            rid = self.rec_name.get(s)
            if rid is None:
                raise GenError(f"no definition of {s}")
            t = ("rec", rid)
        elif s in self.typedefs:
            td = self.typedefs[s]
            t = td if isinstance(td, tuple) else self.parse(td)
            if t[0] == "ptr":
                t = ("ptr", s)          # a typedef'd pointer: its name casts
        else:
            raise GenError(f"type not understood: {qt}")
        for d in reversed(dims):
            t = ("array", t, d)
        return t

    def _anon(self, loc: str, qt: str) -> str:
        # clang on Windows spells the paths with backslashes (in the AST's
        # locations, and so in this table's keys): compare them with slashes
        want = re.sub(r"^.*?((?:include|src)/)", r"\1", loc.replace("\\", "/"))
        for k, v in self.rec_loc.items():
            if re.sub(r"^.*?((?:include|src)/)", r"\1", k.replace("\\", "/")) == want:
                return v
        raise GenError(f"no definition of {qt}")

    def size(self, t) -> int:
        if t[0] in ("int", "float"):
            return t[1]
        if t[0] == "ptr":
            return 4
        if t[0] == "array":
            return self.size(t[1]) * t[2]
        return self.layout(t[1])[1]

    def align(self, t) -> int:
        if t[0] in ("int", "float"):
            return t[1]
        if t[0] == "ptr":
            return 4
        if t[0] == "array":
            return self.align(t[1])
        return self.layout(t[1])[2]

    def layout(self, rid: str):
        """([(name, type, offset)], size, align) of a record: the EE's rules."""
        if rid in self._lay:
            return self._lay[rid]
        tag, fields = self.records[rid]
        out, off, al, size = [], 0, 1, 0
        for name, qt, bits in fields:
            if bits is not None:
                raise GenError(f"bit-field {name}: not supported")
            ft = self.parse(qt)
            fa, fs = self.align(ft), self.size(ft)
            al = max(al, fa)
            if tag == "union":
                out.append((name, ft, 0))
                size = max(size, fs)
            else:
                off = (off + fa - 1) // fa * fa
                out.append((name, ft, off))
                off += fs
                size = off
        size = (size + al - 1) // al * al
        self._lay[rid] = (out, size, al)
        return self._lay[rid]


# --- values -------------------------------------------------------------------

def int_type(size: int, signed: bool) -> str:
    return {1: "char", 2: "short", 4: "int", 8: "long long"}[size] if signed else \
        "unsigned " + {1: "char", 2: "short", 4: "int", 8: "long long"}[size]


class Emitter:
    """The initializer of an object of a file, from the module's bytes."""

    def __init__(self, mod: Module, types: Types, unit: str, before: int) -> None:
        self.mod, self.types = mod, types
        self.rodata = mod.section_range(unit, ".rodata")
        if not hasattr(types, "declared"):
            # once per file: (order, address, name, size or 0) of the names the file
            # declares: those of the symbol files, and the default names that hold
            # their address (D_008DB7C0, func_0090A9C0)
            types.declared = []
            names = dict(mod.names)
            for name in list(types.vars) + sorted(types.funcs):
                m = DEFAULT_NAME.match(name)
                if m and name not in names:
                    names[name] = int(m.group(1), 16)
            for name, addr in names.items():
                if name in types.funcs or name in types.vars:
                    size = 0
                    for v in types.vars.get(name, []):
                        try:
                            size = types.size(types.parse(v["type"]["qualType"]))
                            break
                        except (GenError, KeyError):
                            pass
                    types.declared.append((types.order.get(name, 1 << 30), addr, name, size))
            # every object the file declares (before or after), to tell a pointer
            # into one of them from a literal's address
            types.all_spans = sorted((addr, size or 1, name) for _, addr, name, size in types.declared)
        self.by_addr: dict[int, str] = {}
        self.spans: list[tuple[int, int, str]] = []
        for order, addr, name, size in types.declared:
            if order <= before:
                self.by_addr.setdefault(addr, name)
                if size:
                    self.spans.append((addr, size, name))
        self.spans.sort()

    def ref(self, name: str) -> str:
        return name if name in self.types.funcs and name not in self.types.vars else "&" + name

    def word(self, v: int, cast: str, literal: bool = True, inside: bool = True) -> str | None:
        """A 4-byte value as a name or (a pointer's) a literal, or None (a number).
        inside=False: only an object's or a function's own address is a name."""
        if v < 0x100000:
            return None
        name = self.by_addr.get(v)
        if name:
            return f"({cast}){self.ref(name)}"
        if not inside:
            return None
        i = bisect.bisect_right(self.spans, (v, 1 << 62, ""))
        for lo, size, name in reversed(self.spans[max(0, i - 64):i]):
            if lo <= v < lo + size:
                return f"({cast})((char *){self.ref(name)} + {v - lo:#x})"
        if literal and self.rodata and self.rodata[0] <= v < self.rodata[1]:
            # in an object the file declares later: a number (the source can't
            # name it yet); anywhere else, a literal's
            i = bisect.bisect_right(self.types.all_spans, (v, 1 << 62, ""))
            for lo, size, name in reversed(self.types.all_spans[max(0, i - 64):i]):
                if lo <= v < lo + size:
                    return None
            # a literal starts aligned, after a NUL (another's end or padding)
            if v % 4 or (v > self.rodata[0] and self.mod.read(v - 1, 1) != b"\0"):
                return None
            return f"({cast}){cstr(self.mod.cstring(v))}"
        return None

    def scalar(self, t, a: int) -> str:
        b = self.mod.read(a, self.types.size(t))
        if t[0] == "float":
            f = struct.unpack("<f" if t[1] == 4 else "<d", b)[0]
            if f != f or f in (float("inf"), float("-inf")):
                raise GenError(f"{a:#x}: not a finite float ({b.hex()})")
            return repr(f)
        v = int.from_bytes(b, "little")
        if t[0] == "ptr":
            if v == 0:
                return "0"
            cast = t[1] or "void *"
            return self.word(v, cast) or f"({cast}){v:#010x}"
        size, signed = t[1], t[2]
        if size == 4:
            # an integer field: an object's or a function's own address is a name (a
            # pointer the C keeps as a number, a stage header's offsets relocated on
            # load); a value inside an object, or a literal's, is a number that only
            # equals an address (camera coordinates, colours)
            w = self.word(v, int_type(4, signed), literal=False, inside=False)
            if w:
                return w
        if signed and v >= 1 << (8 * size - 1):
            return str(v - (1 << (8 * size)))
        return str(v) if v < 10 else f"{v:#x}"

    def value(self, t, a: int, indent: str = "") -> str:
        if t[0] in ("int", "float", "ptr"):
            return self.scalar(t, a)
        return "{" + self.inner(t, a, indent) + "}"

    def inner(self, t, a: int, indent: str = "") -> str:
        """The initializer of a T at a without its outer braces."""
        ty = self.types
        if t[0] in ("int", "float", "ptr"):
            return self.scalar(t, a)
        if t[0] == "array":
            elem, n = t[1], t[2]
            if elem[0] == "int" and elem[1] == 1:
                data = self.mod.read(a, n).rstrip(b"\0")
                return cstr_lines(data, indent + "    ")
            es = ty.size(elem)
            items = [self.value(elem, a + i * es, indent + "    ") for i in range(n)]
            return wrap(items, indent)
        fields, size, _ = ty.layout(t[1])
        tag = ty.records[t[1]][0]
        if tag == "union":
            name, ft, off = fields[0]
            fs = ty.size(ft)
            if any(self.mod.read(a + fs, size - fs)):
                raise GenError(f"{a:#x}: a union whose first member doesn't hold all its bytes")
            return self.value(ft, a, indent)
        items, end = [], 0
        for name, ft, off in fields:
            if any(self.mod.read(a + end, off - end)):
                raise GenError(f"{a:#x}: non-zero padding before .{name}")
            items.append(self.value(ft, a + off, indent + "    "))
            end = off + ty.size(ft)
        if any(self.mod.read(a + end, size - end)):
            raise GenError(f"{a:#x}: non-zero padding at the end")
        return wrap(items, indent)


def wrap(items: list[str], indent: str, width: int = 100) -> str:
    if sum(len(i) + 2 for i in items) <= width and not any("\n" in i for i in items):
        return ", ".join(items)
    lines, cur = [], ""
    for it in items:
        if "\n" in it or len(it) > width // 2:
            if cur:
                lines.append(cur)
                cur = ""
            lines.append(it + ",")
            continue
        if cur and len(cur) + len(it) + 2 > width:
            lines.append(cur)
            cur = ""
        cur += it + ", "
    if cur:
        lines.append(cur.rstrip())
    pad = "\n" + indent + "    "
    return pad + pad.join(l.rstrip() for l in lines) + "\n" + indent


# --- a file -----------------------------------------------------------------

_modules: dict[str, Module] = {}


def module(name: str) -> Module:
    if name not in _modules:
        _modules[name] = Module(name)
    return _modules[name]




def parse_file(rel: str, incs, txts: list[str], tmp: Path) -> dict:
    """clang's AST of the file, with placeholder content (the types don't need it)."""
    for mod, addr, _ in incs:
        p = tmp / mod / f"{addr}.inc"
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text("0\n")
    for hdr, names in txts:
        p = tmp / hdr
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text("".join(f'#define TXT_0x{n} ""\n' for n in names))
    p = subprocess.run([CLANG, CLANG_TARGET, "-fsyntax-only", *INCLUDES, "-I", str(tmp), *DEFS, "-w",
                        "-ferror-limit=0", "-Xclang", "-ast-dump=json", "-x", "c", rel],
                       cwd=ROOT, capture_output=True)
    if not p.stdout:
        raise GenError(f"clang failed on {rel}: {p.stderr.decode(errors='replace')[:2000]}")
    return json.loads(p.stdout)


def gen_file(rel: str) -> list[str]:
    """Write the content of one C file; returns the paths written or checked."""
    text = (ROOT / rel).read_text(encoding="utf-8")
    parts = Path(rel).parts
    modname, unit = parts[1], "/".join(parts[2:])[:-2]
    incs = [(m.group(1), m.group(2), m.start()) for m in INC_LINE.finditer(text)]
    names = sorted(set(TXT_USE.findall(text)))
    txts = [(f"{modname}/{unit}.h", names)] if names else []
    if not incs and not txts:
        return []
    for mod, _, _ in incs:
        if mod != modname:
            raise GenError(f"{rel}: content of module {mod} included in module {modname}")
    mod = module(modname)
    with tempfile.TemporaryDirectory() as td:
        ast = parse_file(rel, incs, txts, Path(td))
    types = Types(ast)
    order: dict[str, int] = {}
    for i, n in enumerate(ast.get("inner", [])):
        if n.get("kind") in ("VarDecl", "FunctionDecl") and n.get("name"):
            order.setdefault(n["name"], i)
    types.order = order
    # the top-level declaration each definition is in (a function's, for its statics)
    top: dict[str, int] = {}

    def ids(n, i):
        if n.get("kind") == "VarDecl":
            top[n["id"]] = i
        for c in n.get("inner", []):
            if isinstance(c, dict):
                ids(c, i)
    for i, n in enumerate(ast.get("inner", [])):
        if n.get("kind") == "VarDecl":
            top[n["id"]] = i
        elif n.get("kind") == "FunctionDecl":
            ids(n, i)
    out = []
    for m_, addr, pos in incs:
        decl = re.search(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*(?:__attribute__\s*\(\([^;{]*?\)\)\s*)*=\s*\{\s*\Z",
                         text[max(0, pos - 2000):pos].rstrip(" \t\n"))     # (a window: files are long)
        if not decl:
            raise GenError(f"{rel}: no declaration `NAME ... = {{` before the include of {addr}.inc")
        name = decl.group(1)
        cands = [v for v in types.vars.get(name, []) if v.get("init")]
        if not cands:
            raise GenError(f"{rel}: {name}: no definition in clang's parse")
        qt = cands[0]["type"]["qualType"]
        try:
            t = types.parse(qt)
            em = Emitter(mod, types, unit, top.get(cands[0]["id"], order.get(name, 1 << 30)))
            body = em.inner(t, int(addr, 16))
        except GenError as e:
            raise GenError(f"{rel}: {name} ({qt}) at {addr}: {e}") from None
        path = GEN / modname / f"{addr}.inc"
        write_if_changed(path, f"/* {name}: generated by tools/content_gen.py from the user's copy */\n{body}\n")
        out.append(str(path))
    for hdr, names in txts:
        lines = [f"/* {rel}'s texts: generated by tools/content_gen.py from the user's copy */\n"]
        for n in names:
            lines.append(f"#define TXT_0x{n} {cstr(mod.cstring(int(n, 16)))}\n")
        path = GEN / hdr
        write_if_changed(path, "".join(lines))
        out.append(str(path))
    return out


def gen_one(rel: str) -> str | None:
    """gen_file(rel); the error's text, or None."""
    try:
        gen_file(rel)
        return None
    except GenError as e:
        return str(e)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("files", nargs="*", help="src/MODULE/FILE.c (default: every C file)")
    ap.add_argument("--stamp", help="touch this file when done (ninja)")
    # (processes: the work is Python's; a large file takes about 400 MB, so not too many)
    ap.add_argument("--jobs", type=int, default=min(os.cpu_count() or 1, 8))
    args = ap.parse_args()
    files = [str(Path(f).resolve().relative_to(ROOT)) for f in args.files] if args.files else \
        sorted(str(p.relative_to(ROOT)) for p in (ROOT / "src").rglob("*.c"))
    if len(files) > 1 and args.jobs > 1:
        files.sort(key=lambda f: -(ROOT / f).stat().st_size)      # the largest first
        with ProcessPoolExecutor(args.jobs) as ex:
            errors = [e for e in ex.map(gen_one, files) if e]
    else:
        errors = [e for e in map(gen_one, files) if e]
    for e in errors:
        print(f"content_gen: {e}", file=sys.stderr)
    if errors:
        return 1
    if args.stamp:
        Path(args.stamp).parent.mkdir(parents=True, exist_ok=True)
        Path(args.stamp).write_text("")
    return 0


if __name__ == "__main__":
    sys.exit(main())
