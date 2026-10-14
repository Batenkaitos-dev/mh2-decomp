#!/usr/bin/env python3
"""Compare built targets against config/targets.sha1.

    python tools/check_targets.py build/out/SLPM_662.80 [more files...]

Prints OK or the first differing offset for each file; exits non-zero if any
file does not match.
"""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def load_targets() -> dict[str, tuple[str, int]]:
    out = {}
    for line in (ROOT / "config" / "targets.sha1").read_text().splitlines():
        if line.strip() and not line.startswith("#"):
            sha1, size, name, _source = line.split()
            out[name] = (sha1, int(size))
    return out


def main() -> int:
    targets = load_targets()
    bad = 0
    for arg in sys.argv[1:]:
        path = Path(arg)
        want_sha1, want_size = targets[path.name]
        data = path.read_bytes()
        got = hashlib.sha1(data).hexdigest()
        if got == want_sha1:
            print(f"OK    {path.name}")
            continue
        bad += 1
        orig = ROOT / "build" / "orig" / path.name
        detail = f"size {len(data)} (want {want_size})"
        if orig.exists():
            ref = orig.read_bytes()
            first = next((i for i in range(min(len(ref), len(data))) if ref[i] != data[i]), None)
            if first is not None:
                detail += f", first difference at file offset {first:#x}"
        print(f"FAIL  {path.name}: SHA-1 {got} != {want_sha1}; {detail}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
