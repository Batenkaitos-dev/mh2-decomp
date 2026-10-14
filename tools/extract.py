#!/usr/bin/env python3
"""Extract the matching-build targets from the user's original disc files.

Reads the boot ELF and the 12 code overlays (DATA.BIN entry 1) from the
extracted disc files or the ISO (see tools/orig.py), verifies each against
config/targets.sha1, and writes them to build/orig/. The originals are only
read. Everything written is derived from the user's copy and is gitignored.

    python tools/extract.py [--out build/orig]
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import afs  # noqa: E402
import orig  # noqa: E402

TARGETS = orig.ROOT / "config" / "targets.sha1"


def load_targets() -> list[tuple[str, int, str, str]]:
    rows = []
    for line in TARGETS.read_text().splitlines():
        if line.strip() and not line.startswith("#"):
            sha1, size, name, source = line.split()
            rows.append((sha1, int(size), name, source))
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", type=Path, default=orig.ROOT / "build" / "orig")
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    failed = 0
    for sha1, size, name, source in load_targets():
        dest = args.out / name
        if dest.exists() and dest.stat().st_size == size and \
                hashlib.sha1(dest.read_bytes()).hexdigest() == sha1:
            print(f"[ok]   {name}")
            continue
        with afs.open_disc_file(source) as f:
            data = f.read()
        got = hashlib.sha1(data).hexdigest()
        if got != sha1 or len(data) != size:
            print(f"[FAIL] {name} from {source}: SHA-1 {got}, expected {sha1} "
                  f"(is this version 1.04? run tools/verify_originals.py)", file=sys.stderr)
            failed += 1
            continue
        tmp = dest.with_suffix(dest.suffix + ".part")
        tmp.write_bytes(data)
        tmp.replace(dest)
        print(f"[done] {name} ({size:,} bytes)")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
