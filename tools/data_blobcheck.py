#!/usr/bin/env python3
"""List data definitions in src/ that look like binary blobs, not tables.

    python tools/data_blobcheck.py [BASE]     (BASE: only files changed since that commit)

Compressed or encrypted data, embedded images or sounds, and anything a
proprietary library uses must not be committed as C: the owner's rules
allow no assets and no library data in the repository. Moving data into
C writes whatever the range holds, so run this before committing moved
data. It prints every initialized array
- of 1 KB or more whose bytes have an entropy of 6.5 bits per byte or more
  (tables and strings of the game stay well below: the highest is 5.9), or
- of 256 bytes or more whose bytes, and the differences of neighbouring
  bytes, both have an entropy of at least 0.85 of the most their size
  allows (min(8, log2 n) bits per byte). Random bytes of 256 to 1024 bytes
  measure 0.88 to 0.98 (the DNAS data: 448 bytes at 0.94, 544 at 0.96);
  the game's tables stay below 0.78 (the highest: the lobby's HTML
  colours, 0.77). The differences keep
  permutation-like tables out: the lobby's to-upper table measures 0.97,
  but its differences 0.03.
Arrays of a basic type are measured on their bytes; arrays of structs on
the low byte of each integer in the initializer.
Exit status 1 if it finds one. Such an object becomes an INCLUDE_DATA line
(CONTRIBUTING.md, "Data in C"): the build takes its bytes from the user's
original (tools/data_blob.py). It also checks that INCLUDE_DATA lines name a
directory under build/ and that nothing under build/ or asm/ is tracked.
"""
import math
import re
import struct
import subprocess
import sys
from collections import Counter
from pathlib import Path

ES = {"u8": 1, "s8": 1, "char": 1, "u16": 2, "s16": 2, "u32": 4, "s32": 4, "f32": 4,
      "u64": 8, "s64": 8}
DEFN = re.compile(r"^(?:static )?(?:const )?(\w+)\s+(\w+)((?:\[[^\]]*\])+)\s*=\s*\{(.*?)^\};", re.S | re.M)
# numbers, not the digits of a name (D_00410040) or of a float's fraction
NUM = re.compile(r"(?<![\w.])(?:-?0x[0-9A-Fa-f]+|-?\d+(?:\.\d+)?f?)")
COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)


def blob_bytes(t, body):
    b = bytearray()
    for v in NUM.findall(body):
        hexa = "0x" in v.lower()
        if not hexa:
            v = v.rstrip("f")
        if t == "f32" and not hexa:
            b += struct.pack("<f", float(v))
        elif "." not in v:
            x = int(v, 16) if "0x" in v.lower() else int(v, 10)
            b += (x & ((1 << (8 * ES[t])) - 1)).to_bytes(ES[t], "little")
    return b


def low_bytes(body):
    """The low byte of each integer of a struct array's initializer."""
    b = bytearray()
    for v in NUM.findall(body):
        if "." in v:
            continue
        b.append((int(v, 16) if "0x" in v.lower() else int(v.rstrip("f"), 10)) & 0xFF)
    return b
def bits(b):
    n = len(b)
    return -sum(c / n * math.log2(c / n) for c in Counter(b).values())
def entropy(b):
    """(size, bits per byte, relative entropy) if the bytes look like a blob, else None."""
    n = len(b)
    if n < 256:
        return None
    h = bits(b)
    d = bytes((b[i + 1] - b[i]) & 0xFF for i in range(n - 1))
    rel = min(h / min(8, math.log2(n)), bits(d) / min(8, math.log2(n - 1)))
    if (n >= 1024 and h >= 6.5) or rel >= 0.85:
        return (n, h, rel)
    return None


INCLUDE_DATA = re.compile(r'^\s*INCLUDE_DATA\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)', re.M)


def main():
    if len(sys.argv) > 1:
        out = subprocess.run(["git", "diff", "--name-only", sys.argv[1], "--", "src"],
                             capture_output=True, text=True, check=True).stdout.split()
        files = [Path(f) for f in out if f.endswith(".c") and Path(f).exists()]
    else:
        files = sorted(Path("src").rglob("*.c"))
    found = 0
    for f in files:
        text = f.read_text(encoding="utf-8", errors="replace")
        for m in DEFN.finditer(text):
            t, name, _dims, body = m.groups()
            body = COMMENT.sub(" ", body)
            e = entropy(blob_bytes(t, body) if t in ES else low_bytes(body))
            if e:
                found += 1
                kind = "bytes" if t in ES else "integers (low bytes)"
                print(f"{f}: {name}: {e[0]} {kind}, {e[1]:.2f} bits/byte, {e[2]:.2f} of the most")
        for m in INCLUDE_DATA.finditer(text):
            # the generated .s must stay in a git-ignored directory
            if not m.group(1).startswith("build/"):
                found += 1
                print(f"{f}: INCLUDE_DATA {m.group(2)}: its directory {m.group(1)} is not under build/")
    # and nothing generated from the originals is tracked or staged
    tracked = subprocess.run(["git", "ls-files", "--cached", "build", "asm"],
                             capture_output=True, text=True).stdout.split()
    for p in tracked:
        found += 1
        print(f"{p}: a generated file is tracked")
    print(f"{found} blob-like definitions")
    sys.exit(1 if found else 0)


if __name__ == "__main__":
    main()
