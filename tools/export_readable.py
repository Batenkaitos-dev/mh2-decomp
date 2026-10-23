#!/usr/bin/env python3
"""Export a readable source tree, with the game's data and text written out in C.

    python3 tools/export_readable.py [OUT] [--jobs N] [--deps link|none] [--force]

OUT (default build/readable) gets a copy of every file git tracks, with the
C files of src/ that take content from the user's copy (CONTRIBUTING,
"Content") written out in full:
- each data initializer `#include "MODULE/ADDR.inc"` becomes the values
  themselves, read from the user's copy as tools/content_gen.py reads them:
  one array element per line, each struct member labelled by its field name
  (designated initializers: `.bone = 14`, `[3] = {...}`; MWCC and clang
  both build them to the same bytes as the plain list);
- integers are decimal, but a value above 0x7FFFFFFF and a value of a
  field or table whose name says it holds flags, a mask, bits or a colour
  (as far as a tool can tell);
- pointers, floats and string literals are written as content_gen writes
  them (the bytes depend on it), a literal with a comment that gives its
  text decoded (the game's Shift-JIS, as UTF-8);
- a `char` array that holds a text is its string literal, with the same
  comment; any other byte array is a list of numbers;
- each `TXT(0xADDR)` becomes its string literal and that comment, and the
  file's texts header (`#include "MODULE/FILE.h"`) goes.
Every other file is copied unchanged, so OUT configures and builds with the
repository's own commands (it has no content left for content_gen to
write), and unedited it builds the 13 targets byte-identical. OUT gets a
README_EXPORT.md saying what it is.

OUT holds the game's data and text, from the user's copy: it is for the
user's own research and modifications, never committed and never shared.
It must be outside the repository or under its build/ (git-ignored).

Run it after configure (it reads build/orig, the user's copy, as content_gen
does) in a clone (it copies git's list of files, from the working tree).
--deps link (the default) makes OUT/tools/deps a link to this clone's
toolchain; with --deps none, run tools/setup.sh in OUT. An existing export
is replaced only with --force (edits made in it are lost).
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import content_gen as cg  # noqa: E402

ROOT = cg.ROOT
README = "README_EXPORT.md"
WIDTH = 110             # an element longer than this is broken, one member per line

# plain `char` (not `signed char`, `unsigned char`, u8, s8) marks a text
# array: give it a type of its own (content_gen's layout reads only
# t[0], t[1], t[2])
cg.BUILTIN["char"] = ("int", 1, True, "char")

# a field or table whose values read better in hex (point 3: masks, flag
# sets, colours)
HEXISH = re.compile(r"flag|mask|bits|colou?r|rgba|argb|abgr", re.I)
# comments and literals (skipped), and TXT() uses
SCAN = re.compile(r"""/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*'"""
                  r"""|\bTXT\(\s*0x([0-9A-Fa-f]{1,8})\s*\)""", re.S)
DECL = re.compile(r"([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*(?:__attribute__\s*\(\([^;{]*?\)\)\s*)*=\s*\{\s*\Z")
BANNER = ("/* Exported by tools/export_readable.py from the user's copy of the game: this file\n"
          " * holds the game's data and text. For your own research and modifications;\n"
          " * never commit or share it. */\n")


def decoded(data: bytes) -> str | None:
    """The text of these bytes (the game's Shift-JIS), safe in a comment; None if it
    doesn't decode. Characters MWCC's Shift-JIS can't hold are written \\uXXXX."""
    for codec in ("shift_jis", "cp932"):
        try:
            s = data.decode(codec)
            break
        except UnicodeDecodeError:
            s = None
    if s is None:
        return None
    out = []
    for ch in s:
        o = ord(ch)
        if ch == "\n":
            out.append("\\n")
        elif ch == "\t":
            out.append("\\t")
        elif o < 0x20 or o == 0x7F:
            out.append("\\x%02x" % o)
        else:
            try:
                ch.encode("shift_jis")      # tools/mwcc_cc.py hands MWCC the file in Shift-JIS
                out.append(ch)
            except UnicodeEncodeError:
                out.append("\\u%04x" % o)
    return "".join(out).replace("*/", "*\\/")


def text_comment(data: bytes) -> str:
    """` /* "text" */` after a literal of these bytes, when the literal doesn't show
    it (it has escapes: the game's Shift-JIS, control characters, `?`); else ""."""
    if not any(b < 0x20 or b >= 0x7F or b == 0x3F for b in data):
        return ""
    t = decoded(data)
    return f' /* "{t}" */' if t is not None else ""


def is_text(data: bytes) -> bool:
    """Whether a char array's bytes are a text (a NUL-ended string, or empty)."""
    s = data.rstrip(b"\0")
    return b"\0" not in s and (not s or decoded(s) is not None) and \
        not any(b < 0x20 and b not in (0x09, 0x0A) for b in s)


# --- values as nodes: a string (one value), ("list", [node]), ("rec", [(field, node)])

class Readable(cg.Emitter):
    def word(self, v: int, cast: str, literal: bool = True) -> str | None:
        w = super().word(v, cast, literal)
        if w and w.endswith('"'):
            w += text_comment(self.mod.cstring(v))
        return w

    def number(self, t, a: int, label: str) -> str:
        size, signed = t[1], t[2]
        v = int.from_bytes(self.mod.read(a, size), "little")
        if size == 4:
            w = self.word(v, cg.int_type(4, signed))
            if w:
                return w
        if signed and v >= 1 << (8 * size - 1):
            return str(v - (1 << (8 * size)))
        if v > 0x7FFFFFFF or (v >= 10 and HEXISH.search(label)):
            return f"{v:#x}"
        return str(v)

    def node(self, t, a: int, label: str):
        ty = self.types
        if t[0] == "int":
            return self.number(t, a, label)
        if t[0] in ("float", "ptr"):
            return self.scalar(t, a)
        if t[0] == "array":
            elem, n = t[1], t[2]
            es = ty.size(elem)
            if elem[0] == "int" and elem[1] == 1 and len(elem) > 3:
                data = self.mod.read(a, n)
                if is_text(data):
                    s = data.rstrip(b"\0")
                    return cg.cstr(s) + text_comment(s)
            return ("list", [self.node(elem, a + i * es, label) for i in range(n)])
        fields, size, _ = ty.layout(t[1])
        if ty.records[t[1]][0] == "union":
            name, ft, off = fields[0]
            fs = ty.size(ft)
            if any(self.mod.read(a + fs, size - fs)):
                raise cg.GenError(f"{a:#x}: a union whose first member doesn't hold all its bytes")
            return ("rec", [(name, self.node(ft, a, name))])
        items, end = [], 0
        for name, ft, off in fields:
            if any(self.mod.read(a + end, off - end)):
                raise cg.GenError(f"{a:#x}: non-zero padding before .{name}")
            items.append((name, self.node(ft, a + off, name)))
            end = off + ty.size(ft)
        if any(self.mod.read(a + end, size - end)):
            raise cg.GenError(f"{a:#x}: non-zero padding at the end")
        return ("rec", items)


def has_rec(node) -> bool:
    return isinstance(node, tuple) and (node[0] == "rec" or any(has_rec(n) for n in node[1]))


def inline(node) -> str:
    if isinstance(node, str):
        return node
    if node[0] == "list":
        return "{ " + ", ".join(inline(n) for n in node[1]) + " }"
    return "{ " + ", ".join((f".{f} = " if f else "") + inline(n) for f, n in node[1]) + " }"


def block(node, indent: str) -> list[str]:
    """The lines of a node's members, one per line (its braces are the caller's)."""
    if isinstance(node, str):
        return [indent + node]
    if node[0] == "list":
        members = [(f"[{i}] = ", n) for i, n in enumerate(node[1])]
    else:
        members = [((f".{f} = " if f else ""), n) for f, n in node[1]]
    lines = []
    for lead, n in members:
        one = indent + lead + inline(n) + ","
        if isinstance(n, str) or (len(one) <= WIDTH and "\n" not in one):
            lines.append(one)
        else:
            lines.append(indent + lead + "{")
            lines += block(n, indent + "    ")
            lines.append(indent + "},")
    return lines


# --- a file -----------------------------------------------------------------

def export_file(rel: str) -> tuple[str, str, dict]:
    """(rel, the file's readable text, counts) of a C file with content."""
    text = (ROOT / rel).read_text(encoding="utf-8")
    parts = Path(rel).parts
    modname, unit = parts[1], "/".join(parts[2:])[:-2]
    incs = [(m.group(1), m.group(2), m.start(), m.end()) for m in cg.INC_LINE.finditer(text)]
    names = sorted(set(cg.TXT_USE.findall(text)))
    counts = {"files": 1, "fields": 0, "numbers": 0, "texts": 0, "txt": 0}
    mod = cg.module(modname)
    with tempfile.TemporaryDirectory() as td:
        ast = cg.parse_file(rel, [(m, a, p) for m, a, p, _ in incs],
                            [(f"{modname}/{unit}.h", names)] if names else [], Path(td))
    types = cg.Types(ast)
    order: dict[str, int] = {}
    for i, n in enumerate(ast.get("inner", [])):
        if n.get("kind") in ("VarDecl", "FunctionDecl") and n.get("name"):
            order.setdefault(n["name"], i)
    types.order = order
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
    edits = []          # (start, end, new text)
    for m_, addr, pos, end in incs:
        if m_ != modname:
            raise cg.GenError(f"{rel}: content of module {m_} included in module {modname}")
        decl = DECL.search(text[max(0, pos - 2000):pos].rstrip(" \t\n"))
        if not decl:
            raise cg.GenError(f"{rel}: no declaration `NAME ... = {{` before the include of {addr}.inc")
        name = decl.group(1)
        cands = [v for v in types.vars.get(name, []) if v.get("init")]
        if not cands:
            raise cg.GenError(f"{rel}: {name}: no definition in clang's parse")
        qt = cands[0]["type"]["qualType"]
        try:
            t = types.parse(qt)
            em = Readable(mod, types, unit, top.get(cands[0]["id"], order.get(name, 1 << 30)))
            node = em.node(t, int(addr, 16), name)
        except cg.GenError as e:
            raise cg.GenError(f"{rel}: {name} ({qt}) at {addr}: {e}") from None
        indent = re.match(r"[ \t]*", text[text.rfind("\n", 0, pos) + 1:]).group(0) + "    "
        if isinstance(node, str):
            body = indent + node
            counts["texts" if node.startswith('"') else "numbers"] += 1
        else:
            lines = block(node, indent)
            if node[0] == "list" and len(node[1]) == 1 and len(lines) == 1:
                lines = [indent + inline(node[1][0])]       # T x[1] = { v }
            body = "\n".join(lines)
            counts["fields" if has_rec(node) else "numbers"] += 1
        edits.append((pos, end, body))
    out, last = [], 0
    for s, e, new in edits:
        out.append(text[last:s])
        out.append(new)
        last = e
    out.append(text[last:])
    text = "".join(out)

    def txt(m):
        if m.group(1) is None:
            return m.group(0)
        counts["txt"] += 1
        data = mod.cstring(int(m.group(1), 16))
        return cg.cstr(data) + text_comment(data)
    text = SCAN.sub(txt, text)
    text = re.sub(rf'^[ \t]*#[ \t]*include[ \t]+"{re.escape(modname)}/{re.escape(unit)}\.h".*\n', "",
                  text, flags=re.M)
    return rel, BANNER + text, counts


def export_one(rel: str):
    try:
        return export_file(rel)
    except cg.GenError as e:
        return rel, None, str(e)


def tracked() -> list[str]:
    p = subprocess.run(["git", "ls-files", "-z"], cwd=ROOT, capture_output=True)
    if p.returncode != 0:
        sys.exit("export_readable: not a git clone (it copies git's list of files)")
    return [f for f in p.stdout.decode("utf-8").split("\0") if f and (ROOT / f).is_file()]


README_TEXT = """\
# A readable export of the Monster Hunter 2 (Dos) decompilation

This directory was generated by tools/export_readable.py from {root}
and from **your copy of the game**: its C files hold the game's data and
text, written out in full. It is for your own research and modifications.
**Never commit it, publish it or share it**: the repository holds none of
the game's content, and this directory does.

What differs from the repository: in src/, each data object's initializer
is written inline (one array element per line, each struct member by its
field name: `.bone = 14`), and each `TXT(0xADDR)` is its string literal,
with a comment giving the text decoded. Every other file is the
repository's own, unchanged.

It builds with the repository's usual commands, from this directory:

    source tools/env.sh
    export MH2_ORIG_DIR=<the folder with your copy of the game>
    python3 tools/configure.py --split
    ninja

Unedited, it builds the 13 targets byte-identical (ninja prints "OK" for
each). Edited, it builds the modified game in build/out/; ninja then
reports the targets you changed as differing from the original, which is
expected. MH2_ORIG_DIR is needed because the default (the folder above the
tree) is not your game folder here.

tools/deps is {deps}. Running tools/export_readable.py again with --force
replaces this directory, and your edits with it.
"""


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("out", nargs="?", default=str(ROOT / "build" / "readable"))
    ap.add_argument("--jobs", type=int, default=min(os.cpu_count() or 1, 4))
    ap.add_argument("--deps", choices=("link", "none"), default="link")
    ap.add_argument("--force", action="store_true", help="replace an existing export")
    args = ap.parse_args()
    out = Path(args.out).resolve()
    if out == ROOT or (ROOT in out.parents and (ROOT / "build") not in out.parents):
        sys.exit(f"export_readable: {out} is in the repository; use a directory outside it or under build/")
    # the export holds the game's content: never inside a git work tree, where
    # it could be committed (this repository's build/ is git-ignored)
    if (ROOT / "build") not in out.parents:
        for d in [out] + list(out.parents):
            if (d / ".git").exists():
                sys.exit(f"export_readable: {out} is inside the git work tree {d}; "
                         "the export holds your copy's data and text: put it outside any repository")
    if not (ROOT / "build" / "orig").is_dir():
        sys.exit("export_readable: no build/orig: run tools/configure.py (--split) first")
    if out.exists() and any(out.iterdir()):
        if not (out / README).exists():
            sys.exit(f"export_readable: {out} exists and is not an export; choose another directory")
        if not args.force:
            sys.exit(f"export_readable: {out} holds an export; --force replaces it (and any edits in it)")
        shutil.rmtree(out)
    t0 = time.time()
    files = tracked()
    content = [f for f in files if f.startswith("src/") and f.endswith(".c")
               and (cg.INC_LINE.search((ROOT / f).read_text(encoding="utf-8"))
                    or cg.TXT_USE.search((ROOT / f).read_text(encoding="utf-8")))]
    content.sort(key=lambda f: -(ROOT / f).stat().st_size)          # the largest first
    results = {}
    errors = []
    totals = {"files": 0, "fields": 0, "numbers": 0, "texts": 0, "txt": 0}
    with ProcessPoolExecutor(max(1, args.jobs)) as ex:
        for rel, text, info in ex.map(export_one, content):
            if text is None:
                errors.append(info)
                continue
            results[rel] = text
            for k, v in info.items():
                totals[k] += v
    for e in errors:
        print(f"export_readable: {e}", file=sys.stderr)
    if errors:
        return 1
    for f in files:
        dst = out / f
        dst.parent.mkdir(parents=True, exist_ok=True)
        if f in results:
            dst.write_text(results[f], encoding="utf-8")
            shutil.copymode(ROOT / f, dst)
        else:
            shutil.copy2(ROOT / f, dst)
    deps = "not included: run tools/setup.sh here"
    if args.deps == "link" and (ROOT / "tools" / "deps").is_dir():
        try:
            os.symlink(ROOT / "tools" / "deps", out / "tools" / "deps", target_is_directory=True)
            deps = f"a link to {ROOT / 'tools' / 'deps'}"
        except OSError as e:
            print(f"export_readable: no link for tools/deps ({e}); run tools/setup.sh in {out}", file=sys.stderr)
    (out / README).write_text(README_TEXT.format(root=ROOT, deps=deps), encoding="utf-8")
    print(f"export_readable: {out}: {len(files)} files, {totals['files']} C files written out "
          f"({totals['fields'] + totals['numbers'] + totals['texts']} objects: {totals['fields']} with field "
          f"names, {totals['numbers']} as plain numbers, {totals['texts']} texts; {totals['txt']} TXT() uses) "
          f"in {time.time() - t0:.0f} s")
    return 0


if __name__ == "__main__":
    sys.exit(main())
