#!/usr/bin/env python3
"""Summarise decompilation progress from an objdiff report.

    ninja progress                        # builds, runs objdiff-cli report, prints this
    python tools/progress.py build/report.json [--markdown]

Counts come from objdiff: every assembly file is a target-only unit and
every C file is a unit diffed against the original assembly, so "matched"
means byte-identical code produced by the matching compiler. "Named"
counts the matched functions with a real name rather than splat's
func_XXXXXXXX.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path

AUTO_NAME = re.compile(r"(func|sub)_[0-9A-Fa-f]+$")
KINDS = {"origin/" + k for k in ("game", "sdk", "middleware", "access", "vu1")}   # origin categories (tools/configure.py)
PADDING = "padding"   # the overlays' leading zeros and empty text pieces: not counted (tools/configure.py)
MEASURES = ("total_code", "matched_code", "total_functions", "matched_functions")


def measure(m: dict, key: str) -> int:
    return int(m.get(key, 0) or 0)


def row(name: str, m: dict) -> tuple:
    total, matched = measure(m, "total_code"), measure(m, "matched_code")
    funcs, mfuncs = measure(m, "total_functions"), measure(m, "matched_functions")
    pct = 100.0 * matched / total if total else 0.0
    return name, matched, total, pct, mfuncs, funcs


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("report", type=Path)
    ap.add_argument("--markdown", action="store_true")
    args = ap.parse_args()
    report = json.loads(args.report.read_text())
    named = Counter()   # category -> matched functions with a real name
    for u in report.get("units", []):
        cats = u.get("metadata", {}).get("progress_categories") or ["?"]
        for f in u.get("functions", []):
            if float(f.get("fuzzy_match_percent") or 0) == 100.0 and not AUTO_NAME.match(f["name"]):
                for cat in cats:
                    named[cat] += 1
                named["TOTAL"] += 1
    cats = report.get("categories", [])
    rows = [row(c.get("name") or c["id"], c.get("measures", {})) + (named[c["id"]],)
            for c in cats if c["id"] not in KINDS and c["id"] != PADDING]
    # objdiff's overall measures include the padding units: take them out
    total = dict(report.get("measures", {}))
    pad = next((c.get("measures", {}) for c in cats if c["id"] == PADDING), {})
    for k in MEASURES:
        total[k] = measure(total, k) - measure(pad, k)
    rows.append(row("TOTAL", total) + (named["TOTAL"] - named[PADDING],))
    # by origin (tools/configure.py, KIND_CATEGORIES): game is Capcom's code
    kinds = [row(c.get("name") or c["id"], c.get("measures", {})) + (named[c["id"]],) for c in cats if c["id"] in KINDS]
    if kinds:
        rows.append(None)
        rows += kinds
    if args.markdown:
        print("| Module | Matched code | Total code | % | Functions | Named |")
        print("|---|---:|---:|---:|---:|---:|")
        for r in rows:
            if r is None:
                print()
                print("| Origin | Matched code | Total code | % | Functions | Named |")
                print("|---|---:|---:|---:|---:|---:|")
                continue
            n, mc, tc, pct, mf, tf, nm = r
            print(f"| {n} | {mc:,} | {tc:,} | {pct:.3f}% | {mf:,} / {tf:,} | {nm:,} |")
    else:
        print(f"{'module':14} {'matched code':>14} {'total code':>12} {'%':>9} {'functions':>17} {'named':>6}")
        for r in rows:
            if r is None:
                print()
                print(f"{'origin':14} {'matched code':>14} {'total code':>12} {'%':>9} {'functions':>17} {'named':>6}")
                continue
            n, mc, tc, pct, mf, tf, nm = r
            print(f"{n:14} {mc:>14,} {tc:>12,} {pct:>8.3f}% {mf:>8,}/{tf:<8,} {nm:>6,}")
    if pad:
        print()
        print(f"(not counted: {measure(pad, 'total_code'):,} bytes of padding, the overlays' leading zeros"
              " and lbguild's and test's empty text)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
