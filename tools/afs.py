#!/usr/bin/env python3
"""Read CRI AFS archives (AFS00-02.AFS, DATA.BIN) from the user's disc files.

AFS layout (little endian):
    "AFS\\0", u32 count
    count x { u32 offset, u32 size }      table of contents
    { u32 offset, u32 size }               name table (may be 0/0, and on some
                                           discs it sits just before the first
                                           file instead of after the TOC)
    name table: count x 0x30 bytes { char name[32]; u16 year, month, day,
                                     hour, minute, second; u32 size }

    python tools/afs.py AFS01.AFS            # list entries (disc path)
    python tools/afs.py AFS01.AFS --kinds    # also guess each entry's type
"""

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO

sys.path.insert(0, str(Path(__file__).resolve().parent))
import orig  # noqa: E402


@dataclass
class AfsEntry:
    index: int
    offset: int
    size: int
    name: str = ""
    date: str = ""


def read_toc(f: BinaryIO) -> list[AfsEntry]:
    f.seek(0)
    magic, count = struct.unpack("<4sI", f.read(8))
    if magic != b"AFS\0":
        raise ValueError("not an AFS archive")
    raw = f.read(8 * count + 8)
    entries = [AfsEntry(i, *struct.unpack_from("<II", raw, 8 * i)) for i in range(count)]
    nt_off, nt_size = struct.unpack_from("<II", raw, 8 * count)
    if not nt_off and entries:
        # Some tools store the name-table pointer in the last 8 bytes before
        # the first file.
        f.seek(entries[0].offset - 8)
        nt_off, nt_size = struct.unpack("<II", f.read(8))
    end = f.seek(0, 2)
    names = b""
    if nt_off and nt_size >= 0x30 * count and nt_off + 0x30 * count <= end:
        f.seek(nt_off)
        names = f.read(0x30 * count)
    if len(names) == 0x30 * count:
        for e in entries:
            rec = names[0x30 * e.index: 0x30 * (e.index + 1)]
            e.name = rec[:32].split(b"\0")[0].decode("ascii", "replace")
            y, mo, d, h, mi, s = struct.unpack_from("<6H", rec, 32)
            e.date = f"{y:04d}-{mo:02d}-{d:02d} {h:02d}:{mi:02d}:{s:02d}"
    return entries


class SubFile:
    """Read-only window [offset, offset+size) of another file object."""

    def __init__(self, f: BinaryIO, offset: int, size: int):
        self._f, self._off, self._size, self._pos = f, offset, size, 0

    def seek(self, pos: int, whence: int = 0) -> int:
        base = {0: 0, 1: self._pos, 2: self._size}[whence]
        self._pos = max(0, base + pos)
        return self._pos

    def tell(self) -> int:
        return self._pos

    def read(self, n: int = -1) -> bytes:
        n = self._size - self._pos if n < 0 else min(n, self._size - self._pos)
        if n <= 0:
            return b""
        self._f.seek(self._off + self._pos)
        data = self._f.read(n)
        self._pos += len(data)
        return data

    def close(self) -> None:
        self._f.close()

    def __enter__(self) -> SubFile:
        return self

    def __exit__(self, *exc) -> None:
        self.close()


def open_disc_file(path: str) -> BinaryIO:
    """Open a disc file for reading: extracted copy if present, else from the ISO.

    "DATA.BIN:0:12" opens entry 12 of the AFS archive that is entry 0 of
    DATA.BIN (nested AFS archives).
    """
    disc_path, *indices = path.split(":")
    f = orig.open_disc_file(disc_path)
    for idx in indices:
        e = read_toc(f)[int(idx)]
        f = SubFile(f, e.offset, e.size)
    return f


def guess_kind(head: bytes, size: int) -> str:
    """Best-effort content type from the first bytes of an entry."""
    if head[:4] == b"\x7fELF":
        return "ELF"
    if head[:4] == b"AFS\0":
        return "AFS"
    if head[:4] == b"TIM2":
        return "TIM2 texture"
    if head[:2] == b"\x80\x00" and b"(c)CRI" in head[:0x200]:
        return "ADX audio"
    if head[:4] == b"\x00\x00\x01\xba":
        return "MPEG-PS (Sofdec SFD)"
    if head[:4] in (b"RIFF",):
        return "RIFF"
    if head[:4].isalnum() and head[:4].isascii():
        return f"magic {head[:4].decode()!r}"
    # Heuristic for raw R5900 code: count common instruction patterns.
    words = struct.unpack_from(f"<{len(head) // 4}I", head)
    jr_ra = sum(w == 0x03E00008 for w in words)
    sp_adj = sum((w & 0xFFFF0000) == 0x27BD0000 for w in words)
    if words and (jr_ra + sp_adj) * 64 >= len(words):
        return f"code? (jr ra={jr_ra}, addiu sp={sp_adj} in {len(words)} words)"
    return f"data {head[:8].hex()}"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("path", help="disc path, e.g. AFS01.AFS or DATA.BIN")
    ap.add_argument("--kinds", action="store_true", help="read each entry's header and guess its type")
    args = ap.parse_args()
    with open_disc_file(args.path) as f:
        entries = read_toc(f)
        print(f"{args.path}: {len(entries)} entries")
        for e in entries:
            line = f"{e.index:4d}  off={e.offset:#011x}  size={e.size:#010x} ({e.size:>11,d})  {e.name:24} {e.date}"
            if args.kinds:
                f.seek(e.offset)
                line += "  " + guess_kind(f.read(min(e.size, 0x4000)), e.size)
            print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
