#!/usr/bin/env python3
"""Generate a padded C struct from a list of fields, or update one in a header.

    python tools/gen_struct.py SPEC              # print the struct
    python tools/gen_struct.py SPEC HEADER.h     # replace it in HEADER.h

SPEC lines (`#` starts a comment line):
    name Em                       struct name
    size 0x9D0                    total size (padded at the end)
    0x2C2 s16 hp // hit points    OFFSET TYPE NAME[DIMS] [// comment]
    0x868 u8 seen // COMPAT: unk868[1]; ...  also declare the old name (a union)

TYPE is a basic type (u8 s8 u16 s16 u32 s32 f32 u64 s64), a pointer
(`struct Em *`), or `Name{SIZE}` for a struct of SIZE bytes (decimal).
Fields may come in any order; gaps become `u8 unkXXX[...]` padding and
overlaps are errors. Large game structs (include/em.h's Em) grow a few
fields per function, and this keeps every field at its offset.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

SIZES = {"char": 1, "u8": 1, "s8": 1, "u16": 2, "s16": 2, "u32": 4, "s32": 4, "f32": 4, "u64": 8, "s64": 8}


def type_size(t: str) -> int:
    t = t.strip()
    if t.endswith("*"):
        return 4
    if t in SIZES:
        return SIZES[t]
    m = re.match(r"\w+\s*\{(\d+)\}$", t)
    if m:
        return int(m.group(1))
    sys.exit(f"unknown type size: {t}")


def generate(spec: str) -> str:
    name, total, fields = "Unk", None, []
    for line in spec.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("name "):
            name = line.split()[1]
            continue
        if line.startswith("size "):
            total = int(line.split()[1], 16)
            continue
        code, _, comment = line.partition("//")
        m = re.match(r"(0x[0-9A-Fa-f]+|[0-9A-Fa-f]+)\s+(.+?)\s*\b(\w+)((?:\[[^\]]+\])*)\s*$", code.strip())
        if not m:
            sys.exit(f"bad line: {line}")
        off, typ, fname, dims = int(m.group(1), 16), m.group(2).strip(), m.group(3), m.group(4)
        count = 1
        for d in re.findall(r"\[([^\]]+)\]", dims):
            count *= int(d, 0)
        fields.append((off, typ, fname, dims, type_size(typ) * count, comment.strip()))
    fields.sort()
    out = [f"typedef struct {name} {{"]
    pos = 0
    for off, typ, fname, dims, size, comment in fields:
        if off < pos:
            sys.exit(f"overlap at {fname} ({off:#x} < {pos:#x})")
        if off > pos:
            out.append(f"    u8 unk{pos:03X}[0x{off:03X} - 0x{pos:03X}];")
        ptyp = re.sub(r"\s*\{\d+\}$", "", typ)
        sp = '' if ptyp.endswith('*') else ' '
        decl = f"    {ptyp}{sp}{fname}{dims};"
        cm = re.match(r"COMPAT: (\w+)((?:\[[^\]]+\])*)[;:]?\s*(.*)$", comment)
        if cm:  # keep the old name too: union { T new; T old; }; /* COMPAT: old */
            otyp = "u8" if cm.group(2) else ptyp
            osp = '' if otyp.endswith('*') else ' '
            decl = f"    union {{ {ptyp}{sp}{fname}{dims}; {otyp}{osp}{cm.group(1)}{cm.group(2) or dims}; }}; /* COMPAT: {cm.group(1)} */ "
            comment = cm.group(3)
        note = f"/* 0x{off:03X}" + (f": {comment}" if comment else "") + " */"
        out.append(f"{decl:<32}{note}")
        pos = off + size
    if total is not None:
        if total < pos:
            sys.exit(f"size {total:#x} is smaller than the fields ({pos:#x})")
        if total > pos:
            out.append(f"    u8 unk{pos:03X}[0x{total:03X} - 0x{pos:03X}];")
    out.append(f"}} {name};")
    return "\n".join(out)


def main() -> int:
    if len(sys.argv) not in (2, 3):
        print(__doc__)
        return 2
    struct = generate(Path(sys.argv[1]).read_text(encoding="utf-8"))
    if len(sys.argv) == 2:
        print(struct)
        return 0
    header = Path(sys.argv[2])
    name = re.match(r"typedef struct (\w+)", struct).group(1)
    text = header.read_text(encoding="utf-8")
    pat = re.compile(r"typedef struct " + name + r" \{.*?\n\} " + name + ";", re.S)
    if not pat.search(text):
        sys.exit(f"struct {name} not found in {header}")
    header.write_text(pat.sub(lambda _m: struct, text, count=1), encoding="utf-8")
    print(f"updated {name} in {header}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
