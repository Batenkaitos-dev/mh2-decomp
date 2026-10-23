#!/usr/bin/env python3
"""A module's symbol files, from its YAML (used by the build's data tools and
by the diff tools)."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def symbol_files(module: str) -> list[Path]:
    """config/symbols/<module>.txt and the module's other symbol files, <module>_*.txt
    (the game overlay's monster families have theirs in game_em.txt)."""
    d = ROOT / "config" / "symbols"
    files = [d / f"{module}.txt"] + sorted(d.glob(f"{module}_*.txt"))
    # and the library files its YAML lists (main: sdk_*.txt, gcc_*.txt, cri_*.txt)
    y = ROOT / "config" / ("main.yaml" if module == "main" else f"overlays/{module}.yaml")
    if y.exists():
        for p in re.findall(r"^\s*-\s*config/symbols/(\S+\.txt)\s*$", y.read_text(encoding="utf-8"), re.M):
            if d / p not in files:
                files.append(d / p)
    return files
