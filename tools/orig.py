"""Locate and read the user's original Monster Hunter 2 (Dos) disc files.

The originals are read-only: nothing here writes to them. They are looked up
in $MH2_ORIG_DIR, or by default in the parent directory of this repository,
as either:

- extracted disc files (SYSTEM.CNF at the top level), and/or
- a disc image (*.iso) at the top level or one directory below.
"""

from __future__ import annotations

import os
from collections.abc import Iterator
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO

import pycdlib

ROOT = Path(__file__).resolve().parent.parent
SECTOR = 2048


def orig_dir() -> Path:
    env = os.environ.get("MH2_ORIG_DIR")
    return Path(env).resolve() if env else ROOT.parent


def find_isos(base: Path | None = None) -> list[Path]:
    base = base or orig_dir()
    found = []
    for p in sorted(base.iterdir()):
        if p.is_file() and p.suffix.lower() == ".iso":
            found.append(p)
        elif p.is_dir() and p.resolve() != ROOT:
            found += [c for c in sorted(p.iterdir()) if c.is_file() and c.suffix.lower() == ".iso"]
    return found


def find_extracted(base: Path | None = None) -> Path | None:
    base = base or orig_dir()
    if any(p.name.upper() == "SYSTEM.CNF" for p in base.iterdir()):
        return base
    return None


@dataclass(frozen=True)
class DiscFile:
    path: str   # disc path, "/" separated, no leading slash or ";1" (e.g. "IOP/MODULES/SMAP.IRX")
    size: int
    lba: int    # first sector on the disc


class Iso:
    """Read-only view of the ISO 9660 filesystem of a disc image."""

    def __init__(self, path: Path):
        self.path = path
        self._iso = pycdlib.PyCdlib()
        self._fp = open(path, "rb")
        self._iso.open_fp(self._fp)

    def close(self) -> None:
        self._iso.close()
        self._fp.close()

    def __enter__(self) -> Iso:
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    @property
    def pvd(self):
        return self._iso.pvd

    def files(self) -> Iterator[DiscFile]:
        for dirname, _dirs, names in self._iso.walk(iso_path="/"):
            for name in names:
                iso_path = f"{dirname.rstrip('/')}/{name}"
                rec = self._iso.get_record(iso_path=iso_path)
                yield DiscFile(_clean(iso_path), rec.data_length, rec.extent_location())

    def open(self, path: str) -> BinaryIO:
        """Open a disc file by its clean path (case-insensitive)."""
        return self._iso.open_file_from_iso(iso_path=self._iso_path(path))

    def read(self, path: str) -> bytes:
        with self.open(path) as f:
            return f.read()

    def _iso_path(self, path: str) -> str:
        want = path.strip("/").upper()
        for f in self.files():
            if f.path.upper() == want:
                name = "/" + f.path
                return name if name.endswith(";1") else name + ";1"
        raise FileNotFoundError(path)


def _clean(iso_path: str) -> str:
    p = iso_path.lstrip("/")
    return p[:-2] if p.endswith(";1") else p


class _IsoMember:
    """A disc file opened from an ISO that closes the ISO when it is closed."""

    def __init__(self, iso: Iso, path: str):
        self._iso = iso
        # pycdlib's file object only becomes readable once entered.
        self._cm = iso.open(path)
        self._f = self._cm.__enter__()

    def __getattr__(self, name):
        return getattr(self._f, name)

    def close(self) -> None:
        self._cm.__exit__(None, None, None)
        self._iso.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc) -> None:
        self.close()


def open_disc_file(path: str) -> BinaryIO:
    """Open one disc file for reading: the extracted copy if present, else from the ISO."""
    base = orig_dir()
    ext = find_extracted(base)
    if ext:
        candidate = ext.joinpath(*path.split("/"))
        if candidate.is_file():
            return open(candidate, "rb")
    isos = find_isos(base)
    if isos:
        return _IsoMember(Iso(isos[0]), path)
    raise FileNotFoundError(f"{path} not found in extracted files or ISO under {base}")


def read_disc_file(path: str) -> bytes:
    """Read one disc file from the extracted files if present, else from the ISO."""
    with open_disc_file(path) as f:
        return f.read()
