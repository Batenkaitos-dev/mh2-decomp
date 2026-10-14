#!/usr/bin/env python3
"""Check that the source holds none of the game's content (CONTRIBUTING, "Content").

    python3 tools/content_scan.py [FILE ...] [-v]

The repository holds the code and the structure (types, names, sizes); the
game's data values and texts come from the user's copy at build time
(tools/content_gen.py). This scan fails on:
- any non-ASCII character in src/ or include/: in a string literal, a
  comment or anywhere else (the game's text, or a comment quoting it), and
  in the other files the repository publishes: config/, tools/,
  CONTRIBUTING.md, README.md, docs/mwcc-matching.md, docs/notes/nm/ and
  docs/ai-workflow/;
- a data initializer in src/ with values: a definition at file scope (or a
  `static` one in a function) whose initializer holds a number other than 0
  (outside a `[...]` index or an offset from a name), a float, a character
  or a string literal. An initializer is either the generated include

      T NAME[N] = {
      #include "MODULE/ADDR.inc"
      };

  or references only: names, `&`, casts, indexes, offsets, `0`/NULL (a
  table of functions, a list of pointers);
- unless the object is a reviewed exception: config/content_exceptions.txt,
  `src/MODULE/FILE.c NAME  # why` (what is not the game's: zlib's tables,
  a file format's constants).

It can't tell an enum constant from a name, nor English game text in a
function's ASCII literal or a comment from a functional string: review
those. Without FILE, it scans all of those files (git's list of them, in a
clone). Exit status 1 on any finding.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXCEPTIONS = ROOT / "config" / "content_exceptions.txt"
# the files the repository publishes besides src/ and include/: checked for non-ASCII text
PUBLISHED = ["config", "tools", "CONTRIBUTING.md", "README.md", "docs/mwcc-matching.md",
             "docs/notes/nm", "docs/ai-workflow"]

TOKEN = re.compile(r"""
    (?P<comment>/\*.*?\*/|//[^\n]*)
  | (?P<pp>^[ \t]*\#(?:[^\n\\]|\\.)*)
  | (?P<string>"(?:\\.|[^"\\\n])*")
  | (?P<char>'(?:\\.|[^'\\\n])*')
  | (?P<number>\.?[0-9](?:[eEpP][+-]|[0-9A-Za-z_.])*)
  | (?P<ident>[A-Za-z_]\w*)
  | (?P<op>[=!<>]=|<<=|>>=|[-+*/%&|^]=)
  | (?P<punct>\S)
""", re.S | re.M | re.X)
GEN_INCLUDE = re.compile(r'^[ \t]*#[ \t]*include[ \t]+"\w+/[0-9A-F]{8}\.inc"[ \t]*$')


def load_exceptions() -> set[tuple[str, str]]:
    out = set()
    if EXCEPTIONS.exists():
        for line in EXCEPTIONS.read_text(encoding="utf-8").splitlines():
            line = line.split("#", 1)[0].strip()
            if line:
                f, name = line.split()[:2]
                out.add((f, name))
    return out


def non_ascii(rel: str, text: str) -> list[str]:
    out = []
    for i, line in enumerate(text.splitlines(), 1):
        if any(ord(c) > 0x7F for c in line):
            out.append(f"{rel}:{i}: non-ASCII text")
    return out


def initializers(text: str):
    """(line, name, [tokens]) of each file-scope or static data initializer."""
    toks = [(m.lastgroup, m.group(), m.start()) for m in TOKEN.finditer(text)]
    depth, i, n = 0, 0, len(toks)
    stmt_start = 0              # index of the first token of the current statement
    fn_depth = None             # brace depth of the function body we are in
    while i < n:
        kind, t, pos = toks[i]
        if kind in ("comment", "pp"):
            i += 1
            continue
        if t == "{":
            # a function body: `)` then `{` at file scope
            if depth == 0 and fn_depth is None:
                prev = [x for x in toks[stmt_start:i] if x[0] not in ("comment", "pp")]
                if prev and prev[-1][1] == ")" and not any(x[1] == "=" for x in prev):
                    fn_depth = 1
            depth += 1
            if fn_depth is not None:
                stmt_start = i + 1      # a block in a function starts a statement
        elif t == "}":
            depth -= 1
            if fn_depth is not None:
                stmt_start = i + 1      # the end of a block (or of the function)
                if depth == 0:
                    fn_depth = None
        elif t == ";" and (depth == 0 or fn_depth is not None):
            stmt_start = i + 1
        elif t == "=" and (depth == 0 or fn_depth is not None) and (fn_depth is None or True):
            stmt = [x for x in toks[stmt_start:i] if x[0] not in ("comment", "pp")]
            words = [x[1] for x in stmt]
            if fn_depth is not None and "static" not in words:
                i += 1
                continue
            if "typedef" in words or not stmt:
                i += 1
                continue
            name = next((x[1] for x in reversed(stmt) if x[0] == "ident"), "?")
            # the initializer: up to `;` (or `,` at its own depth) outside braces
            j, d, init = i + 1, 0, []
            while j < n:
                k2, t2, p2 = toks[j]
                if t2 in "({[" and k2 == "punct":
                    d += 1
                elif t2 in ")}]" and k2 == "punct":
                    d -= 1
                elif d == 0 and t2 in ";,":
                    break
                init.append(toks[j])
                j += 1
            yield text.count("\n", 0, pos) + 1, name, init
            i = j
            continue
        i += 1


def check_init(init) -> str | None:
    """Why an initializer holds content, or None."""
    real = [x for x in init if x[0] != "comment"]
    pps = [x for x in real if x[0] == "pp"]
    if pps:
        if len(pps) == 1 and GEN_INCLUDE.match(pps[0][1]) and \
                [x[1] for x in real if x[0] != "pp"] == ["{", "}"]:
            return None
        return "a preprocessor line in an initializer (other than the generated include)"
    index = 0
    for i, (kind, t, _) in enumerate(real):
        if t == "[":
            index += 1
        elif t == "]":
            index -= 1
        elif kind == "string":
            return f"a string literal {t[:40]}"
        elif kind == "char":
            return f"a character literal {t}"
        elif kind == "number" and not index:
            if re.fullmatch(r"0+[uUlL]*|0[xX]0+[uUlL]*", t):
                continue
            # an offset from a name, `(u8 *)NAME + 0x10`: a reference
            if i >= 2 and real[i - 1][1] in "+-" and (real[i - 2][0] == "ident" or real[i - 2][1] == ")"):
                continue
            return f"a value {t}"
    return None


def scan(files: list[Path], exceptions: set[tuple[str, str]]) -> list[str]:
    out = []
    for p in files:
        rel = p.relative_to(ROOT).as_posix()
        raw = p.read_bytes()
        try:
            text = raw.decode("utf-8")
        except UnicodeDecodeError:
            out.append(f"{rel}: not UTF-8")
            continue
        out += non_ascii(rel, text)
        if p.suffix != ".c" or not rel.startswith("src/"):
            continue
        for line, name, init in initializers(text):
            why = check_init(init)
            if why and (rel, name) not in exceptions:
                out.append(f"{rel}:{line}: {name}: {why}")
    return out


def published_files() -> list[Path]:
    """The files of PUBLISHED: git's list in a clone (not build products), else all."""
    import subprocess
    try:
        out = subprocess.run(["git", "ls-files", "-z", "--", *PUBLISHED], cwd=ROOT,
                             capture_output=True, check=True).stdout.decode()
        return [ROOT / f for f in out.split("\0") if f and (ROOT / f).is_file()]
    except (OSError, subprocess.CalledProcessError):
        return sorted(p for d in PUBLISHED for p in ([ROOT / d] if (ROOT / d).is_file() else (ROOT / d).rglob("*"))
                      if p.is_file())


def main() -> int:
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    verbose = "-v" in sys.argv
    if args:
        files = [Path(a).resolve() for a in args]
    else:
        files = sorted(p for d in ("src", "include") for p in (ROOT / d).rglob("*")
                       if p.is_file() and p.suffix in (".c", ".h", ".s", ".spec", ".vsm", ".inc"))
        files += published_files()
    found = scan(files, load_exceptions())
    for f in found[: None if verbose else 50]:
        print(f)
    if len(found) > 50 and not verbose:
        print(f"... {len(found) - 50} more (-v)")
    print(f"content_scan: {len(files)} files scanned, {len(found)} findings")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
