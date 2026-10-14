#!/usr/bin/env python3
"""Write a described asset's C from the user's copy (CONTRIBUTING, "Described assets").

    python3 tools/asset_c.py config/assets/MODULE/FILE.txt OUT.c

An asset (a data file of the disc: lbguild.bin) is not in the repository. Its
description is: config/assets/MODULE/FILE.txt (the layout: the range the C
owns, the roots, what each pointer type points to, names) and the header the
description includes (the types). This tool reads the module's original
(the YAML's target_path, build/orig/MODULE.bin), finds every object by
following the pointers of the types from the roots, checks that the objects
and MWCC's alignment padding cover the range exactly, and writes the C:
the definitions, in address order, with the original's values. configure.py
runs it for a C piece of the YAML that has a description and no src/ file;
the object then links like any other C file's.
"""
from __future__ import annotations

import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import gen_data  # noqa: E402
from type_data import f32_text  # noqa: E402

SCALARS = {"u8": (1, "B"), "s8": (1, "b"), "char": (1, "b"), "u16": (2, "H"), "s16": (2, "h"),
           "u32": (4, "I"), "s32": (4, "i"), "f32": (4, "f")}
CNAMES = {"int": "s32", "short": "s16", "float": "f32", "unsigned int": "u32",
          "unsigned short": "u16", "unsigned char": "u8", "signed char": "s8"}


def fail(msg: str) -> None:
    sys.exit(f"asset_c.py: {msg}")


class Types:
    """The typedef structs of a header and the headers it includes (from include/)."""

    def __init__(self) -> None:
        self.structs: dict[str, list[tuple[str, str, int]]] = {}   # name -> [(field, type, count)]
        self.seen: set[str] = set()

    def load(self, header: str) -> None:
        path = ROOT / "include" / header
        if header in self.seen or not path.exists():
            return
        self.seen.add(header)
        text = path.read_text(encoding="utf-8")
        for inc in re.findall(r'^\s*#\s*include\s+"([^"]+)"', text, re.M):
            self.load(inc)
        text = re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)
        for m in re.finditer(r"typedef\s+struct\s*\w*\s*\{([^{}]*)\}\s*(\w+)\s*;", text):
            fields = []
            try:
                for decl in m.group(1).split(";"):
                    decl = " ".join(decl.split())
                    if not decl:
                        continue
                    if "(" in decl:
                        raise ValueError(decl)                  # function pointers: not described
                    mm = re.match(r"^((?:unsigned |signed )?\w+)\s*(\**)\s*(\w+)\s*((?:\[[^\]]+\])*)$", decl)
                    if not mm:
                        raise ValueError(decl)
                    base = CNAMES.get(mm.group(1), mm.group(1))
                    t = base + "*" * len(mm.group(2))
                    n = 1
                    for d in re.findall(r"\[([^\]]+)\]", mm.group(4)):
                        if not re.match(r"^[0-9A-Fa-fx+\-* ()]+$", d):
                            raise ValueError(decl)
                        n *= int(eval(d, {"__builtins__": {}}))
                    if not t.endswith("*") and t not in SCALARS and t not in self.structs:
                        raise ValueError(decl)
                    fields.append((mm.group(3), t, n))
            except ValueError:
                continue
            self.structs[m.group(2)] = fields

    def check(self, t: str) -> None:
        if not (t.endswith("*") or t in SCALARS or t in self.structs):
            fail(f"unknown type {t} (not a scalar, a pointer or a struct of the header)")

    def size(self, t: str) -> int:
        if t.endswith("*"):
            return 4
        if t in SCALARS:
            return SCALARS[t][0]
        o = 0
        for _, ft, n in self.structs[t]:
            al = self.align(ft)
            o = (o + al - 1) // al * al + self.size(ft) * n
        al = self.align(t)
        return (o + al - 1) // al * al

    def align(self, t: str) -> int:
        if t.endswith("*") or t in SCALARS:
            return self.size(t)
        return max(self.align(ft) for _, ft, _ in self.structs[t])

    def scalars(self, t: str, a: int):
        """(address, scalar or pointer type) of each value of a T at a, in order."""
        if t.endswith("*") or t in SCALARS:
            yield a, t
            return
        o = a
        for _, ft, n in self.structs[t]:
            al = self.align(ft)
            o = (o + al - 1) // al * al
            for i in range(n):
                yield from self.scalars(ft, o + i * self.size(ft))
            o += self.size(ft) * n


class Desc:
    """A description: config/assets/MODULE/FILE.txt."""

    def __init__(self, path: Path) -> None:
        self.module = path.parent.name
        self.header = None
        self.lo = self.hi = None
        self.roots: list[tuple[int, str, object]] = []
        self.targets: dict[str, object] = {}
        self.names: dict[int, str] = {}
        for num, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            parts = line.split("#", 1)[0].split()
            if not parts:
                continue
            k = parts[0]
            try:
                if k == "include":
                    self.header = parts[1]
                elif k == "range":
                    self.lo, self.hi = int(parts[1], 0), int(parts[2], 0)
                elif k == "root":
                    self.roots.append((int(parts[1], 0), parts[2], spec(parts[3])))
                    for p in parts[4:]:
                        if p.startswith("name="):
                            self.names[int(parts[1], 0)] = p[5:]
                elif k == "target":
                    self.targets[parts[1]] = spec(parts[2])
                elif k == "name":
                    self.names[int(parts[1], 0)] = parts[2]
                else:
                    raise ValueError(k)
            except (ValueError, IndexError):
                fail(f"{path}:{num}: cannot read {line.strip()!r}")
        if self.header is None or self.lo is None or not self.roots:
            fail(f"{path}: needs include, range and at least one root")


def spec(s: str) -> object:
    """N (an array of N), "[]" (up to the next object) or 1 (one value, not an array)."""
    return "[]" if s == "[]" else int(s, 0)


def load_orig(module: str) -> tuple[bytes, int]:
    """(the module's original, its load address) from config/overlays/MODULE.yaml."""
    import yaml
    cfg = yaml.safe_load((ROOT / "config" / "overlays" / f"{module}.yaml").read_text(encoding="utf-8"))
    orig = ROOT / cfg["options"]["target_path"]
    if not orig.exists():
        fail(f"{orig} is missing: run tools/configure.py (it copies the user's original)")
    for seg in cfg["segments"]:
        if isinstance(seg, dict) and "vram" in seg:
            return orig.read_bytes(), seg["vram"] - seg["start"]
    fail(f"{module}.yaml: no segment with a vram")


def obj_align(size: int) -> int:
    """MWCC: an object of 16 bytes or more on 16, a smaller one on 8."""
    return 16 if size >= 16 else 8


def main() -> int:
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    dpath, out = Path(sys.argv[1]), Path(sys.argv[2])
    d = Desc(dpath)
    ty = Types()
    ty.load(d.header)
    if d.header not in ty.seen:
        fail(f"include/{d.header} not found")
    data, load = load_orig(d.module)

    def word(a: int, t: str) -> int:
        n, code = (4, "I") if t.endswith("*") else SCALARS[t]
        if not d.lo <= a and a + n <= d.hi:
            fail(f"a value at {a:08X} is outside the range")
        return struct.unpack("<" + code, data[a - load:a - load + n])[0]

    objs: dict[int, list] = {}       # address -> [type, spec, count]
    order: list[int] = []

    def add(a: int, t: str, sp: object, why: str) -> None:
        ty.check(t)
        if not d.lo <= a < d.hi:
            fail(f"{why}: {a:08X} is outside the range {d.lo:08X}-{d.hi:08X}")
        if a in objs:
            if objs[a][0] != t:
                fail(f"{a:08X} is a {objs[a][0]} and a {t} ({why})")
            return
        objs[a] = [t, sp, None]
        order.append(a)

    for a, t, sp in d.roots:
        add(a, t, sp, "root")

    def count(a: int) -> int:
        t, sp, _ = objs[a]
        if sp != "[]":
            return sp
        nxt = min([x for x in objs if x > a] + [d.hi])
        es = ty.size(t)
        k = (nxt - a) // es
        while k and a % obj_align(k * es):
            k -= 1
        if not k:
            fail(f"{a:08X}: no {t} fits before {nxt:08X}")
        return k

    def walk(a: int) -> None:
        t, sp, _ = objs[a]
        n = count(a)
        objs[a][2] = n
        for i in range(n):
            for va, vt in ty.scalars(t, a + i * ty.size(t)):
                if vt.endswith("*"):
                    v = word(va, vt)
                    if v:
                        tt = vt[:-1]
                        if tt not in d.targets:
                            fail(f"a {vt} at {va:08X}: no `target {tt}` line")
                        add(v, tt, d.targets[tt], f"the {vt} at {va:08X}")

    i = 0
    while i < len(order):
        walk(order[i])
        i += 1
    # a count up to the next object must not change once every object is known
    for a in order:
        if objs[a][1] == "[]" and count(a) != objs[a][2]:
            fail(f"{a:08X}: its extent changed as objects were found; give a count")
    starts = sorted(objs)
    # the objects and MWCC's padding cover the range exactly
    cur = d.lo
    for a in starts:
        t, sp, n = objs[a]
        size = ty.size(t) * n
        at = (cur + obj_align(size) - 1) // obj_align(size) * obj_align(size)
        if at != a:
            fail(f"{a:08X} ({t}): MWCC would place it at {at:08X} (after {cur:08X}): "
                 "an object no pointer reaches, or a wrong count")
        cur = a + size
        nxt = min([x for x in starts if x > a] + [d.hi])
        if any(data[cur - load:nxt - load]):
            fail(f"{cur:08X}-{nxt:08X} (after {a:08X}): bytes no object holds")
    if cur > d.hi:
        fail(f"the objects run past {d.hi:08X}")

    names = {a: d.names.get(a, f"D_{a:08X}") for a in starts}

    class Mod:                       # what gen_data.c_string reads
        pass
    mod = Mod()
    mod.data, mod.load = data, load

    def value(a: int, t: str) -> str:
        v = word(a, t)
        if t.endswith("*"):
            if not v:
                return "NULL"
            if v not in objs:
                fail(f"the {t} at {a:08X} points inside an object ({v:08X})")
            # an array decays to its address; one value needs `&`
            return names[v] if objs[v][1] != 1 else "&" + names[v]
        if t == "f32":
            return f32_text(v)
        if t in ("s8", "s16", "s32", "char"):
            return str(v) if abs(v) < 0x10000 else (f"0x{v:X}" if v > 0 else f"-0x{-v:X}")
        return str(v) if v < 0x10000 else f"0x{v:08X}"

    def ctype(t: str) -> tuple[str, str]:
        """(the base type, the stars before the name)."""
        base = t.rstrip("*")
        return base, "*" * (len(t) - len(base))

    lines, defined = [], set()
    for a in starts:
        t, sp, n = objs[a]
        base, stars = ctype(t)
        name = names[a]
        for va, vt in ((x, y) for i in range(n) for x, y in ty.scalars(t, a + i * ty.size(t))):
            v = word(va, vt) if vt.endswith("*") else 0
            if v and v not in defined:
                # a forward reference: the C declares it first
                tb, ts = ctype(objs[v][0])
                ext = f"extern {tb} {ts}{names[v]}" + ("" if objs[v][1] == 1 else "[]") + ";"
                if ext not in lines:
                    lines.append(ext)
        if t == "char":
            size = n
            raw = data[a - load:a - load + size]
            end = len(raw.rstrip(b"\0"))
            if end == size:
                fail(f"{name}: a text without its NUL")
            lit, o = [], a
            for p in raw[:end].split(b"\0"):
                lit.append(gen_data.c_string(mod, o, len(p))[1:-1])
                o += len(p) + 1
            # (at 8 mod 16, a text in a 16-byte slot is cut to 15 bytes: under 16, MWCC 8-aligns it)
            decl = f"[0x{size:X}]"
            lines.append(f'char {name}{decl} = "' + "\\000".join(lit) + '";')
        elif sp == 1 and not t.endswith("*") and t in ty.structs:
            items = [value(x, y) for x, y in ty.scalars(t, a)]
            lines.append(f"{base} {stars}{name} = {{" + ", ".join(items) + "};")
        else:
            items = [value(x, y) for i in range(n) for x, y in ty.scalars(t, a + i * ty.size(t))]
            dim = "" if sp == 1 else f"[{n}]"
            lines.append(f"{base} {stars}{name}{dim} = {{")
            per = max(1, 64 // max(len(x) for x in items))
            for i in range(0, len(items), per):
                lines.append("    " + ", ".join(items[i:i + per]) + ",")
            lines.append("};")
        defined.add(a)

    rel = dpath.resolve().relative_to(ROOT).as_posix()
    head = [f"/* Written by tools/asset_c.py from {rel} and the user's original: not in the repository. */",
            f'#include "{d.header}"', ""]
    out.parent.mkdir(parents=True, exist_ok=True)
    text = "\n".join(head + lines) + "\n"
    if not out.exists() or out.read_text(encoding="utf-8") != text:
        out.write_text(text, encoding="utf-8")
    print(f"{out}: {len(starts)} objects, {d.lo:08X}-{d.hi:08X}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
