#!/usr/bin/env python3
"""Show that the compiler wrapper never touches code: every function compiled
with plain MWCC equals the wrapper's.

    python3 tools/plain_check.py [-j N] [FILE.c ...]

Every build runs it (`ninja`, or `ninja plaincheck` alone). Each C file of src/ (or the
ones listed) is compiled with mwccps2.exe directly, with the build's flags
(tools/configure.py CFLAGS) and nothing else: no tools/mwcc_cc.py, so no data
splice, no renamed sections, no dropped functions. The file is converted to
Shift-JIS first, as the build does (the game's encoding; an ASCII file is
unchanged), and INCLUDE_DATA lines expand to nothing
(include/common.h). Every function of the plain object is then compared with
the same function in the build's object (build/MODULE/src/MODULE/FILE.o), word
by word, with the relocated fields masked on both sides, and their relocations
must be the same (offset and type). `Stripped_` functions, which the wrapper
drops by design, are skipped. Prints a count per module and every difference;
exit status 1 on any.
"""
from __future__ import annotations

import argparse
import collections
import os
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import configure  # noqa: E402

DEPS = ROOT / "tools" / "deps" / "linux-x64"
WIBO = DEPS / "wibo" / "wibo"
MWCC = DEPS / "mwcc" / "mwccps2.exe"
STRIPPED = "Stripped_"
# MIPS relocation types (REL): the bits each one fills in
FIELD = {2: 0xFFFFFFFF, 4: 0x03FFFFFF, 5: 0xFFFF, 6: 0xFFFF, 7: 0xFFFF}


def elf_functions(path: Path) -> dict[str, tuple[bytes, list[tuple[int, int]]]]:
    """{function: (code, [(offset, reloc type)])} of an ELF32 little-endian object."""
    d = path.read_bytes()
    shoff, = struct.unpack_from("<I", d, 0x20)
    ent, num, stri = struct.unpack_from("<HHH", d, 0x2E)
    sh = [struct.unpack_from("<IIIIIIIIII", d, shoff + i * ent) for i in range(num)]
    so = sh[stri][4]
    name = lambda off, base: d[base + off:d.index(b"\x00", base + off)].decode("latin-1")
    symtab = next(i for i, s in enumerate(sh) if s[1] == 2)
    strtab = sh[sh[symtab][6]][4]
    syms = []
    for k in range(sh[symtab][5] // 16):
        nm, val, size, info, _o, shndx = struct.unpack_from("<IIIBBH", d, sh[symtab][4] + k * 16)
        syms.append((name(nm, strtab), val, size, info, shndx))
    rels = collections.defaultdict(list)            # section index -> [(offset, type)]
    for s in sh:
        if s[1] == 9:                                # SHT_REL
            for k in range(s[5] // 8):
                off, info = struct.unpack_from("<II", d, s[4] + k * 8)
                rels[s[7]].append((off, info & 0xFF))
    out = {}
    for nm, val, size, info, shndx in syms:
        if (info & 0xF) != 2 or not nm or shndx == 0 or shndx >= num:
            continue
        s = sh[shndx]
        if name(s[0], so) != ".text" and not name(s[0], so).startswith(".text"):
            continue
        size = size or s[5] - val
        code = d[s[4] + val:s[4] + val + size]
        rl = sorted((o - val, t) for o, t in rels[shndx] if val <= o < val + size)
        out[nm] = (code, rl)
    return out


def masked(code: bytes, rl: list[tuple[int, int]]) -> bytes:
    b = bytearray(code)
    for off, t in rl:
        if off + 4 <= len(b):
            w, = struct.unpack_from("<I", b, off)
            struct.pack_into("<I", b, off, w & ~FIELD.get(t, 0xFFFF) & 0xFFFFFFFF)
    return bytes(b)


def check(src: str) -> tuple[str, int, int, list[str]]:
    """(module, functions compared, identical, differences) for one C file."""
    rel = Path(src)
    module = rel.parts[1]
    built = ROOT / "build" / module / rel.with_suffix(".o")
    if not built.exists():
        return module, 0, 0, [f"{src}: no build object {built.relative_to(ROOT)} (build first)"]
    with tempfile.TemporaryDirectory(dir=ROOT / "build") as td:
        c = Path(td) / rel.name
        c.write_bytes((ROOT / rel).read_text(encoding="utf-8").encode("shift_jis"))
        o = Path(td) / "plain.o"
        p = subprocess.run([str(WIBO), str(MWCC), *configure.CFLAGS.split(), "-o", str(o), str(c)],
                           cwd=ROOT, capture_output=True, env=dict(os.environ, MWCIncludes="."))
        if p.returncode != 0 or not o.exists():
            return module, 0, 0, [f"{src}: plain MWCC failed: {(p.stdout + p.stderr).decode(errors='replace')[-400:]}"]
        plain = elf_functions(o)
    wrapped = elf_functions(built)
    diffs = []
    n = same = 0
    for fn, (code, rl) in sorted(plain.items()):
        if fn.startswith(STRIPPED):
            continue
        n += 1
        if fn not in wrapped:
            diffs.append(f"{src}: {fn} is not in the build's object")
            continue
        wcode, wrl = wrapped[fn]
        if len(code) != len(wcode):
            diffs.append(f"{src}: {fn}: {len(code)} bytes plain, {len(wcode)} in the build")
        elif [t for _o, t in rl] != [t for _o, t in wrl] or [o for o, _t in rl] != [o for o, _t in wrl]:
            diffs.append(f"{src}: {fn}: relocations differ")
        elif masked(code, rl) != masked(wcode, wrl):
            diffs.append(f"{src}: {fn}: code differs")
        else:
            same += 1
    for fn in sorted(set(wrapped) - set(plain)):
        diffs.append(f"{src}: {fn} is in the build's object only")
    return module, n, same, diffs


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("files", nargs="*")
    ap.add_argument("-j", type=int, default=min(os.cpu_count() or 1, 8))
    a = ap.parse_args()
    files = a.files or sorted(str(p.relative_to(ROOT)) for p in (ROOT / "src").glob("*/*.c"))
    per = collections.defaultdict(lambda: [0, 0])
    diffs = []
    with ProcessPoolExecutor(a.j) as ex:
        for module, n, same, d in ex.map(check, files):
            per[module][0] += n
            per[module][1] += same
            diffs += d
    for module, (n, same) in sorted(per.items()):
        print(f"  {module:<9} {same:>6,} of {n:>6,} functions identical")
    total = sum(v[0] for v in per.values())
    ok = sum(v[1] for v in per.values())
    print(f"plain MWCC: {ok:,} of {total:,} functions identical to the build's ({len(files)} files)")
    for x in diffs:
        print("  " + x)
    return 1 if diffs else 0


if __name__ == "__main__":
    sys.exit(main())
