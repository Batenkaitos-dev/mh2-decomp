#!/usr/bin/env python3
"""Take the game's content out of a C file (CONTRIBUTING, "Content").

    python3 tools/content_conv.py src/MODULE/FILE.c ... [--dry] [--keep NAME,...] [--text ADDR,...]
                                  [--list-ascii]

For a C file whose data initializers hold values, it rewrites:
- each data object with values (numbers, floats, texts, or values mixed
  with pointers) as its declaration and an include of the content that
  tools/content_gen.py writes from the user's copy at build time:

      s16 gSelHeaderLayout[16][2] = {
      #include "select/006CAD00.inc"
      };

  (an array declared `[]` gets its size). Objects made only of references
  (functions, data, zeros) keep their initializer, and so do the objects
  --keep names (reviewed exceptions, config/content_exceptions.txt);
- each string literal in a function body that holds non-ASCII text (and
  the ASCII ones --text ADDR,... lists: what the player sees) as TXT(0xADDR), the
  address of the literal in the original, with `#include "MODULE/FILE.h"`
  (the texts' header) after the file's last include. Write a short English
  comment for each: what the text is, not the text. --list-ascii lists the
  ASCII literals left in the functions (formats, names, messages), to review.

It needs the matching build (build/MODULE.elf for the objects' addresses)
and clang. Comments inside a removed initializer go above its declaration;
check them for quoted text. Then run content_gen.py (configure.py does),
ninja, and check that the targets still match.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import content_gen as cg  # noqa: E402

READELF = ROOT / "tools/deps/linux-x64/binutils/mips-ps2-decompals-readelf"


def elf_symbols(module: str) -> dict[str, list[tuple[int, int]]]:
    """name -> [(address, size)] of the objects of build/MODULE.elf."""
    out: dict[str, list[tuple[int, int]]] = {}
    p = subprocess.run([str(READELF), "-sW", str(ROOT / "build" / f"{module}.elf")],
                       capture_output=True, text=True, check=True)
    for line in p.stdout.splitlines():
        f = line.split()
        if len(f) >= 8 and f[3] == "OBJECT":
            out.setdefault(f[7], []).append((int(f[1], 16), int(f[2], 0)))
    return out


def ast_of(rel: str) -> dict:
    p = subprocess.run([cg.CLANG, cg.CLANG_TARGET, "-fsyntax-only", *cg.INCLUDES, "-I", "build/gen", *cg.DEFS, "-w",
                        "-ferror-limit=0", "-Xclang", "-ast-dump=json", "-x", "c", rel],
                       cwd=ROOT, capture_output=True)
    if not p.stdout:
        sys.exit(f"clang failed on {rel}: {p.stderr.decode(errors='replace')[:2000]}")
    return json.loads(p.stdout)


def offs(rng: dict) -> tuple[int, int]:
    """Byte offsets [begin, end) of a node's range in its file (macros: where expanded)."""
    def pick(l):
        return l.get("expansionLoc", l)
    b, e = pick(rng["begin"]), pick(rng["end"])
    return b["offset"], e["offset"] + e.get("tokLen", 0)


def leaves(n: dict, out: list) -> None:
    """The kinds of the values of an initializer: sym, zero, value, string."""
    k = n.get("kind")
    if k in ("IntegerLiteral", "CharacterLiteral", "FloatingLiteral"):
        v = n.get("value", "1")
        out.append("zero" if v in ("0", "0.0") or (k == "FloatingLiteral" and float(v) == 0) else "value")
        return
    if k == "StringLiteral":
        out.append("string")
        return
    if k == "ImplicitValueInitExpr":
        out.append("zero")
        return
    if k == "DeclRefExpr":
        rd = n.get("referencedDecl", {})
        out.append("value" if rd.get("kind") == "EnumConstantDecl" else "sym")
        return
    if k == "InitListExpr":
        # (with an array_filler, clang lists the filler, then the elements, there)
        for c in n.get("inner", []) + n.get("array_filler", []):
            leaves(c, out)
        return
    if k == "UnaryExprOrTypeTraitExpr":
        out.append("value")
        return
    sub: list = []
    for c in n.get("inner", []):
        leaves(c, sub)
    # an expression on a name (a cast, &, an index or an offset) is a reference
    if "sym" in sub and "string" not in sub:
        out.append("sym")
    else:
        out.extend(sub)


def main_file_decls(ast: dict, rel: str):
    """(node, in a function) of the VarDecls and FunctionDecls of the main file, in order."""
    cur = {"file": "", "line": 0}
    found = []

    def walk(n, in_fn):
        if "loc" in n:
            cg.Types._loc(n["loc"], cur)
        here = cur["file"]
        if "range" in n:
            for k in ("begin", "end"):
                if k in n["range"]:
                    cg.Types._loc(n["range"][k], cur)
        k = n.get("kind")
        if k in ("VarDecl", "FunctionDecl", "StringLiteral") and here == rel:
            found.append((n, in_fn))
        for c in n.get("inner", []):
            if isinstance(c, dict):
                walk(c, in_fn or (k == "FunctionDecl"))
    walk(ast, False)
    return found


def c_literal_bytes(src: str) -> bytes:
    """The bytes of C string literal(s) as MWCC compiles them (the file is Shift-JIS)."""
    out = bytearray()
    for m in re.finditer(r'"((?:\\.|[^"\\])*)"', src, re.S):
        s, i = m.group(1), 0
        while i < len(s):
            c = s[i]
            if c != "\\":
                out += c.encode("shift_jis")
                i += 1
                continue
            n = s[i + 1]
            if n in "01234567":
                j = i + 1
                while j < i + 4 and j < len(s) and s[j] in "01234567":
                    j += 1
                out.append(int(s[i + 1:j], 8) & 0xFF)
                i = j
            elif n == "x":
                j = i + 2
                while j < len(s) and s[j] in "0123456789abcdefABCDEF":
                    j += 1
                out.append(int(s[i + 2:j], 16) & 0xFF)
                i = j
            else:
                out += {"n": b"\n", "t": b"\t", "r": b"\r", "a": b"\a", "b": b"\b", "f": b"\f",
                        "v": b"\v"}.get(n, n.encode("shift_jis"))
                i += 2
    return bytes(out)


def convert(rel: str, keep: set[str], text_ascii: set[int], dry: bool, list_ascii: bool = False) -> list[str]:
    raw = (ROOT / rel).read_bytes()
    parts = Path(rel).parts
    modname, unit = parts[1], "/".join(parts[2:])[:-2]
    mod = cg.module(modname)
    syms = elf_symbols(modname)
    named = {a for name, lst in syms.items() if not name.startswith("@") for a, _ in lst}
    ast = ast_of(rel)
    types = cg.Types(ast)
    edits: list[tuple[int, int, bytes]] = []
    listed: set[str] = set()
    report = []
    rodata = mod.section_range(unit, ".rodata")
    for n, in_fn in main_file_decls(ast, rel):
        k = n["kind"]
        if k == "VarDecl" and n.get("init") and n.get("storageClass") != "extern" \
                and (not in_fn or n.get("storageClass") == "static"):
            # the initializer: the last child that isn't an attribute (a definition
            # lists the attributes it inherits from an earlier declaration too)
            exprs = [c for c in n.get("inner", []) if not c.get("kind", "").endswith("Attr")]
            init = exprs[-1] if exprs else None
            if init is None:
                continue
            kinds: list = []
            leaves(init, kinds)
            name = n["name"]
            if set(kinds) <= {"sym", "zero"} or name in keep:
                report.append(f"keep {name}: " + ("kept by --keep" if name in keep else "references only"))
                continue
            addrs = syms.get(name, [])
            if len(addrs) != 1:
                sys.exit(f"{rel}: {name}: {len(addrs)} addresses in build/{modname}.elf")
            addr, elf_size = addrs[0]
            b, e = offs(init["range"])
            if cg.INC_LINE.search(raw[b:e].decode("utf-8", "replace")):
                continue                    # done already
            # the comments inside the initializer go above the declaration
            # (string and character literals skipped: "mmbb://..." holds no comment)
            comments = [c for c in re.findall(rb'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*',
                                              raw[b:e], re.S) if c[:1] == b"/"]
            new = b'{\n#include "%s/%08X.inc"\n}' % (modname.encode(), addr)
            edits.append((b, e, new))
            vb, _ = offs(n["range"])
            line_start = raw.rfind(b"\n", 0, vb) + 1
            decl = raw[vb:b]
            if b"[]" in decl:
                # the size MWCC gave it (clang reads the file as UTF-8, so its
                # count of a Japanese string's bytes is not the game's)
                qt = n["type"]["qualType"]
                try:
                    t = types.parse(qt)
                    count = elf_size // types.size(t[1])
                    if t[0] != "array" or count * types.size(t[1]) != elf_size:
                        raise cg.GenError("size")
                except cg.GenError:
                    sys.exit(f"{rel}: {name}: no size for {qt} ({elf_size} bytes in the ELF)")
                i = decl.index(b"[]")
                edits.append((vb + i, vb + i + 2, b"[%d]" % count))
            if comments:
                indent = re.match(rb"[ \t]*", raw[line_start:]).group(0)
                block = b"".join(indent + c.strip() + b"\n" for c in comments)
                edits.append((line_start, line_start, block))
            report.append(f"data {name} -> {modname}/{addr:08X}.inc" + (f" ({len(comments)} comments moved)" if comments else ""))
        elif k == "StringLiteral" and in_fn:
            b, e = offs(n["range"])
            src = raw[b:e].decode("utf-8")
            if not src.startswith('"'):
                continue                    # TXT() already, or a macro's literal
            data = c_literal_bytes(src)
            ascii = all(c < 0x80 for c in data)
            if ascii and not text_ascii and not list_ascii:
                continue
            if not rodata:
                sys.exit(f"{rel}: no .rodata range for a literal")
            lo, hi = rodata
            o_lo, o_hi = mod.offset(lo), mod.offset(hi - 1) + 1
            # (a named object with the same bytes is data, not the literal; a literal
            # starts after a NUL, not inside a longer one that ends with the same bytes)
            hits = [mod.vram(o) for o in range(o_lo, o_hi - len(data))
                    if mod.data[o:o + len(data) + 1] == data + b"\0" and (mod.vram(o) % 8 == 0)
                    and (o == o_lo or mod.data[o - 1] == 0) and mod.vram(o) not in named]
            if ascii and list_ascii:
                if src not in listed:
                    listed.add(src)
                    report.append(f"ascii {','.join('%08X' % h for h in hits) or '?'} {src}")
                continue
            if ascii and not (set(hits) & text_ascii):
                continue
            if len(hits) != 1:
                sys.exit(f"{rel}: literal {src[:60]}: {len(hits)} places in the file's .rodata")
            edits.append((b, e, b"TXT(0x%08X)" % hits[0]))
            report.append(f"text {hits[0]:08X}")
    if any(r.startswith("text") for r in report) and \
            b'#include "%s/%s.h"' % (modname.encode(), unit.encode()) not in raw:
        incs = list(re.finditer(rb'^#include [<"][^\n]*\.h[">][^\n]*\n', raw, re.M))
        at = incs[-1].end() if incs else 0
        edits.append((at, at, b'#include "%s/%s.h"   /* the texts this file uses (tools/content_gen.py) */\n'
                      % (modname.encode(), unit.encode())))
    edits.sort(key=lambda x: (x[0], x[1]))
    out, pos = bytearray(), 0
    for b, e, new in edits:
        if b < pos:
            sys.exit(f"{rel}: overlapping edits at {b}")
        out += raw[pos:b] + new
        pos = e
    out += raw[pos:]
    if not dry:
        (ROOT / rel).write_bytes(bytes(out))
    return report


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("files", nargs="+")
    ap.add_argument("--dry", action="store_true")
    ap.add_argument("--keep", default="", help="names of objects to leave as they are")
    ap.add_argument("--text", default="", help="addresses (hex) of ASCII literals that are game text")
    ap.add_argument("--list-ascii", action="store_true",
                    help="list the ASCII literals of the functions (with their addresses) and change nothing")
    args = ap.parse_args()
    keep = {k for k in args.keep.split(",") if k}
    text_ascii = {int(a, 16) for a in args.text.split(",") if a}
    for f in args.files:
        rel = str(Path(f).resolve().relative_to(ROOT))
        for line in convert(rel, keep, text_ascii, args.dry or args.list_ascii, args.list_ascii):
            if not args.list_ascii or line.startswith("ascii"):
                print(f"{rel}: {line}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
