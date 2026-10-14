#!/usr/bin/env python3
"""Verify the user's original disc files (read-only).

Hashes the disc image and every file on it, checks that the extracted files
(if present) match the disc image byte for byte, and checks both against the
known-good manifest in config/disc.sha1.

    python tools/verify_originals.py                   # verify
    python tools/verify_originals.py --write-manifest  # (re)write config/disc.sha1
"""

from __future__ import annotations

import argparse
import hashlib
import os
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import orig  # noqa: E402

MANIFEST = orig.ROOT / "config" / "disc.sha1"
IMAGE_KEY = "<disc image>"
CHUNK = 8 << 20


def hash_stream(f, want_all: bool = False) -> dict[str, str]:
    sha1 = hashlib.sha1()
    md5 = hashlib.md5() if want_all else None
    crc = 0
    while chunk := f.read(CHUNK):
        sha1.update(chunk)
        if want_all:
            md5.update(chunk)
            crc = zlib.crc32(chunk, crc)
    out = {"sha1": sha1.hexdigest()}
    if want_all:
        out |= {"md5": md5.hexdigest(), "crc32": f"{crc:08x}"}
    return out


def _walk_files(top: Path, skip: set[Path]) -> list[Path]:
    """The files under top, not descending into the directories in skip
    (this checkout, an .iso's folder) or into any other git checkout kept
    next to the disc files."""
    files = []
    for dirpath, dirnames, filenames in os.walk(top):
        d = Path(dirpath)
        dirnames[:] = [n for n in dirnames
                       if (d / n).resolve() not in skip and not (d / n / ".git").exists()]
        files += [d / n for n in filenames if (d / n).is_file()]
    return files


def load_manifest() -> dict[str, tuple[str, int]]:
    entries = {}
    if MANIFEST.exists():
        for line in MANIFEST.read_text().splitlines():
            if line.strip() and not line.startswith("#"):
                sha1, size, path = line.split(maxsplit=2)
                entries[path] = (sha1, int(size))
    return entries


def parse_system_cnf(data: bytes) -> dict[str, str]:
    fields = {}
    for line in data.decode("ascii", "replace").splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            fields[k.strip()] = v.strip()
    return fields


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--write-manifest", action="store_true", help=f"write {MANIFEST.relative_to(orig.ROOT)}")
    args = ap.parse_args()

    base = orig.orig_dir()
    isos = orig.find_isos(base)
    extracted = orig.find_extracted(base)
    print(f"originals: {base}")
    print(f"  disc image(s): {', '.join(str(p.relative_to(base)) for p in isos) or 'none'}")
    print(f"  extracted files: {'yes' if extracted else 'no'}")
    if len(isos) > 1:
        print("error: more than one .iso found; set MH2_ORIG_DIR to a folder with one", file=sys.stderr)
        return 2
    if not isos and not extracted:
        print("error: no originals found (set MH2_ORIG_DIR)", file=sys.stderr)
        return 2

    manifest = load_manifest()
    problems: list[str] = []
    result: dict[str, tuple[str, int]] = {}
    disc_files: list[orig.DiscFile] = []

    if isos:
        iso_path = isos[0]
        size = iso_path.stat().st_size
        print(f"\nhashing {iso_path.name} ({size:,} bytes, {size // orig.SECTOR:,} sectors)...")
        with open(iso_path, "rb") as f:
            h = hash_stream(f, want_all=True)
        print(f"  SHA-1  {h['sha1']}\n  MD5    {h['md5']}\n  CRC32  {h['crc32']}")
        result[IMAGE_KEY] = (h["sha1"], size)
        with orig.Iso(iso_path) as iso:
            pvd = iso.pvd
            def s(b: bytes) -> str:
                return b.decode("ascii", "replace").strip()
            print(f"  ISO 9660 PVD: system={s(pvd.system_identifier)!r} volume={s(pvd.volume_identifier)!r} "
                  f"blocks={pvd.space_size:,} created={s(pvd.volume_creation_date.record())[:14]}")
            disc_files = sorted(iso.files(), key=lambda d: d.lba)
            print(f"\n{'LBA':>9} {'size':>13}  {'SHA-1':40}  path   [extracted copy]")
            for df in disc_files:
                with iso.open(df.path) as f:
                    sha1 = hash_stream(f)["sha1"]
                result[df.path] = (sha1, df.size)
                status = ""
                if extracted:
                    local = extracted.joinpath(*df.path.split("/"))
                    if not local.is_file():
                        status = "MISSING"
                    else:
                        with open(local, "rb") as f:
                            lsha1 = hash_stream(f)["sha1"]
                        status = "match" if lsha1 == sha1 and local.stat().st_size == df.size else f"MISMATCH ({lsha1})"
                    if status != "match":
                        problems.append(f"extracted {df.path}: {status}")
                print(f"{df.lba:9d} {df.size:13,d}  {sha1}  {df.path}   [{status or 'n/a'}]")

    if extracted:
        on_disc = {d.path.upper() for d in disc_files}
        skip = {orig.ROOT.resolve()} | {p.parent.resolve() for p in isos if p.parent != base}
        extras = []
        for p in sorted(_walk_files(extracted, skip)):
            if p.suffix.lower() != ".iso":
                rel = p.relative_to(extracted).as_posix()
                if disc_files and rel.upper() not in on_disc:
                    extras.append(rel)
                if not disc_files:  # no image: hash the extracted files themselves
                    with open(p, "rb") as f:
                        result[rel] = (hash_stream(f)["sha1"], p.stat().st_size)
        for e in extras:
            problems.append(f"extracted file not on disc: {e}")
        if disc_files:
            print(f"\nextracted files: {len(disc_files) - sum('MISSING' in p for p in problems)} of "
                  f"{len(disc_files)} present, {sum('MISMATCH' in p for p in problems)} mismatched, "
                  f"{len(extras)} extra")

    cnf_key = next((k for k in result if k.upper() == "SYSTEM.CNF"), None)
    if cnf_key:
        data = orig.read_disc_file(cnf_key)
        cnf = parse_system_cnf(data)
        print(f"\nSYSTEM.CNF: {cnf}")
        if cnf.get("VER") != "1.04":
            problems.append(f"SYSTEM.CNF VER is {cnf.get('VER')!r}, expected '1.04'")

    if args.write_manifest:
        MANIFEST.parent.mkdir(exist_ok=True)
        lines = ["# Monster Hunter 2 (Dos), NTSC-J, version 1.04: known-good SHA-1s.",
                 "# Format: sha1  size  path   (written by tools/verify_originals.py)"]
        lines += [f"{sha1}  {size}  {path}" for path, (sha1, size) in result.items()]
        MANIFEST.write_text("\n".join(lines) + "\n", newline="\n")
        print(f"\nwrote {MANIFEST.relative_to(orig.ROOT)} ({len(result)} entries)")
    elif manifest:
        for path, (sha1, size) in result.items():
            want = manifest.get(path)
            if want is None:
                problems.append(f"{path}: not in manifest")
            elif want != (sha1, size):
                problems.append(f"{path}: SHA-1/size differs from manifest")
        for path in manifest.keys() - result.keys():
            if not (path == IMAGE_KEY and not isos):
                problems.append(f"{path}: in manifest but not found")
        print(f"\nmanifest: checked {len(result)} entries against {MANIFEST.relative_to(orig.ROOT)}")

    if problems:
        print(f"\n{len(problems)} problem(s):")
        for p in problems:
            print(f"  - {p}")
        return 1
    print("\nOK: originals verified")
    return 0


if __name__ == "__main__":
    sys.exit(main())
