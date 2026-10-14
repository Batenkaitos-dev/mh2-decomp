#!/usr/bin/env python3
"""Assemble a splat/spimdisasm .s file whose strings contain Shift-JIS text.

splat writes .s files in UTF-8, with Japanese strings decoded from the
game's Shift-JIS bytes. gas would emit those characters as UTF-8. This
wrapper rewrites every non-ASCII character inside string literals as
octal-escaped Shift-JIS bytes (using the same Python codec spimdisasm
decoded with), then pipes the result to gas. Escaping every byte also
avoids the classic Shift-JIS problem of 0x5C trail bytes being read as a
backslash. Line numbers are preserved, so gas errors still point at the
right line.

It also lets every piece be linked at its original address without the
linker script's SUBALIGN (which the per-file data split removed: a C
object's strings and tables keep their own 4-, 8- or 16-byte alignment):
- each section gets the alignment of the address of its first line in the
  original (from splat's `/* offset vram ... */` comments, up to 16
  bytes), so the linker puts it right where it was after the previous
  piece;
- inside data sections, `.align` directives above 4 bytes are dropped:
  gas would count them from the section start, which need not be 16-byte
  aligned, and the disassembly already holds the padding they stand for.

    sjis_as.py <as> <input.s> <output.o> [as flags...]
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ENCODING = "shift_jis"
SECTION = re.compile(r"^\s*\.section\s+([^\s,]+)")
ALIGN = re.compile(r"^\s*\.align\s+(\d+)\s*$")
# A line of splat output that emits something at a known address:
# /* [offset] vram [word] */ followed by an instruction or a directive.
ADDRESSED = re.compile(r"^\s*/\*\s+(?:[0-9A-F]+\s+)?([0-9A-F]{8})(?:\s+[0-9A-F]{8})?\s+\*/\s*\S")
MAX_ALIGN = 16


def align_sections(lines: list[str]) -> list[str]:
    """See the module docstring. Keeps the number of lines."""
    out = list(lines)
    pending = None          # index of a .section line still waiting for its alignment
    is_text = True
    for i, line in enumerate(lines):
        m = SECTION.match(line)
        if m:
            pending, is_text = i, m.group(1).startswith(".text")
            continue
        if not is_text:
            a = ALIGN.match(line)
            if a and int(a.group(1)) > 2:
                out[i] = "\n"
                continue
        if pending is not None:
            v = ADDRESSED.match(line)
            if v:
                addr = int(v.group(1), 16)
                align = min(addr & -addr, MAX_ALIGN) if addr else MAX_ALIGN
                out[pending] = out[pending].rstrip("\n") + f"; .balign {align}\n"
                pending = None
    return out


def escape_non_ascii(line: str) -> str:
    if line.isascii():
        return line
    out = []
    in_str = False
    escaped = False
    for ch in line:
        if in_str:
            if escaped:
                escaped = False
                out.append(ch)
            elif ch == "\\":
                escaped = True
                out.append(ch)
            elif ch == '"':
                in_str = False
                out.append(ch)
            elif ord(ch) < 0x80:
                out.append(ch)
            else:
                out.extend(f"\\{b:03o}" for b in ch.encode(ENCODING))
        else:
            if ch == '"':
                in_str = True
            out.append(ch if ord(ch) < 0x80 else " ")  # non-ASCII outside strings: comments only
    return "".join(out)


def main() -> int:
    if len(sys.argv) < 4:
        print(__doc__, file=sys.stderr)
        return 2
    assembler, src, obj, *flags = sys.argv[1:]
    text = Path(src).read_text(encoding="utf-8")
    lines = align_sections(text.splitlines(keepends=True))
    converted = "".join(escape_non_ascii(line) for line in lines)
    src_dir = str(Path(src).parent)
    p = subprocess.run([assembler, *flags, "-I", src_dir, "-o", obj, "-"],
                       input=converted.encode("ascii"))
    return p.returncode


if __name__ == "__main__":
    sys.exit(main())
