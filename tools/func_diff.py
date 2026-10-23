#!/usr/bin/env python3
"""Compile a C file with MWCC and compare each function with the original.

    python tools/func_diff.py MODULE FILE.c [FUNC ...] [-D NAME] [-C N]

MODULE is main, sub_main, game, ... Every function MWCC emits for FILE.c
(INCLUDE_DATA lines are ignored) is compared word by word with the original
bytes at its address, which comes from the name (func_XXXXXXXX) or from
config/symbols/*.txt. Relocated fields are masked, and their targets are
checked against the symbol addresses (a '?' after a symbol means its address
is unknown, so only the opcode was compared). Differences are listed side by
side: original on the left, compiled on the right, '!' marks a difference.

The function's own jump tables and string literals (MWCC's local `@N`
objects) are checked too: each is compared with the original bytes at the
address the original code loads there, entries pointing into functions
included. A function only MATCHes when they match as well.

This needs no splat split or build.ninja change, so C can be tried before a
file is set up; `ninja` stays the final check. Exit status 0 when every
function listed (or compiled) matches.

    -D NAME   define NAME before compiling (e.g. -D NON_MATCHING)
    -C N      context lines around differences (default 3; -1 = whole function)
"""

from __future__ import annotations

import argparse
import bisect
import os
import re
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pathlib import Path

from module_symbols import symbol_files  # noqa: E402  (a module's symbol files, from its YAML)

ROOT = Path(__file__).resolve().parent.parent
DEPS = ROOT / "tools" / "deps" / "linux-x64"
WIBO = DEPS / "wibo" / "wibo"
MWCC = DEPS / "mwcc" / "mwccps2.exe"
OBJDUMP = DEPS / "binutils" / "mips-ps2-decompals-objdump"
CFLAGS = ["-c", "-nostdinc", "-O3,p", "-sdatathreshold", "0", "-str", "readonly", "-i", "include",
          "-i", "build/gen"]  # as tools/configure.py
SPLICE_LINE = re.compile(r"^\s*INCLUDE_DATA\(.*$")
SYM_LINE = re.compile(r"^\s*([A-Za-z_.$][\w.$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;")
AUTO_NAME = re.compile(r"^(?:func|D|jtbl)_([0-9A-Fa-f]{6,8})$")
R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16 = 2, 4, 5, 6


def module_image(module: str) -> tuple[bytes, int]:
    """The original file and the address of its offset 0."""
    if module == "main":
        data = (ROOT / "build/orig/SLPM_662.80").read_bytes()
        phoff, = struct.unpack_from("<I", data, 0x1C)
        phnum, = struct.unpack_from("<H", data, 0x2C)
        for i in range(phnum):
            p_type, p_off, p_vaddr, _pa, p_filesz = struct.unpack_from("<5I", data, phoff + 32 * i)
            if p_type == 1 and p_filesz:
                return data, p_vaddr - p_off
        sys.exit("no PT_LOAD segment in the boot ELF")
    data = (ROOT / f"build/orig/{module}.bin").read_bytes()
    return data, struct.unpack_from("<I", data, 8)[0]          # MWo3 header: load address


def load_symbols(module: str) -> dict[str, int]:
    syms = {}
    for path in [p for m in ("main", "sub_main") for p in symbol_files(m)] + symbol_files(module):
        if path.exists():
            for line in path.read_text(encoding="utf-8").splitlines():
                mm = SYM_LINE.match(line)
                if mm:
                    syms[mm.group(1)] = int(mm.group(2), 16)
    return syms


def function_starts(module: str) -> list[int]:
    """Where the module's functions start, if its starts are pinned
    (config/functions/<module>.txt plus the named ones), else []."""
    pinned = ROOT / "config" / "functions" / f"{module}.txt"
    if not pinned.exists():
        return []
    starts = set()
    for path in [pinned] + symbol_files(module):
        if path.exists():
            for line in path.read_text(encoding="utf-8").splitlines():
                mm = SYM_LINE.match(line)
                if mm:
                    starts.add(int(mm.group(2), 16))
    return sorted(starts)


def piece_starts(module: str, base: int) -> list[int]:
    """Where each piece of the module's YAML starts (code, data, padding pieces),
    as addresses: a function's original tail ends at the next one at the latest."""
    y = ROOT / "config" / ("main.yaml" if module == "main" else f"overlays/{module}.yaml")
    if not y.exists():
        return []
    offs = re.findall(r"^\s*-\s*(?:\[\s*|\{\s*start:\s*)(0x[0-9A-Fa-f]+)", y.read_text(encoding="utf-8"), re.M)
    return sorted({base + int(o, 16) for o in offs})


def sym_addr(name: str, syms: dict[str, int]) -> int | None:
    if name in syms:
        return syms[name]
    m = AUTO_NAME.match(name)
    return int(m.group(1), 16) if m else None


def read_object(path: Path):
    """[(function, bytes, [(offset, type, symbol)])] per .text section, other
    non-empty sections, and {name: (bytes, relocs)} for each data object."""
    data = path.read_bytes()
    e_shoff, = struct.unpack_from("<I", data, 0x20)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", data, 0x2E)
    shdrs = [struct.unpack_from("<10I", data, e_shoff + i * e_shentsize) for i in range(e_shnum)]
    shstr = data[shdrs[e_shstrndx][4]:][:shdrs[e_shstrndx][5]]

    def cstr(tab, off):
        return tab[off:tab.index(b"\0", off)].decode()

    symtab = next(i for i, h in enumerate(shdrs) if h[1] == 2)
    strtab = data[shdrs[shdrs[symtab][6]][4]:][:shdrs[shdrs[symtab][6]][5]]
    syms, sizes = [], []
    for k in range(shdrs[symtab][5] // 16):
        st_name, st_value, st_size, st_info, _o, st_shndx = struct.unpack_from("<IIIBBH", data, shdrs[symtab][4] + 16 * k)
        syms.append((cstr(strtab, st_name), st_value, st_info, st_shndx))
        sizes.append(st_size)
    funcs = {shndx: name for name, value, info, shndx in syms if (info & 0xF) == 2 and value == 0}
    relocs: dict[int, list] = {}
    for h in shdrs:
        if h[1] == 9:                                          # SHT_REL
            for k in range(h[5] // 8):
                r_off, r_info = struct.unpack_from("<II", data, h[4] + 8 * k)
                relocs.setdefault(h[7], []).append((r_off, r_info & 0xFF, syms[r_info >> 8][0]))
        elif h[1] == 4:
            sys.exit("RELA relocations are not handled")
    out = []
    for i, h in enumerate(shdrs):
        if cstr(shstr, h[0]) == ".text" and i in funcs:
            out.append((funcs[i], data[h[4]:h[4] + h[5]], sorted(relocs.get(i, []))))
    other = [(cstr(shstr, h[0]), h[5]) for h in shdrs
             if cstr(shstr, h[0]) in (".rodata", ".data", ".sdata", ".sbss", ".bss") and h[5]]
    objects = {}
    for (name, value, info, shndx), size in zip(syms, sizes):
        if (info & 0xF) == 1 and 0 < shndx < e_shnum and shdrs[shndx][1] == 1:     # STT_OBJECT, PROGBITS
            h = shdrs[shndx]
            objects[name] = (data[h[4] + value:h[4] + value + size],
                             [(o - value, t, s) for o, t, s in relocs.get(shndx, []) if value <= o < value + size])
    return out, other, objects


def check_object(name: str, obj, orig_addr: int, image: bytes, base: int, syms) -> str | None:
    """Compare a data object of the C object (a jump table, a literal) with the
    original bytes at orig_addr. Returns a description of the difference."""
    body, relocs = obj
    orig = image[orig_addr - base:orig_addr - base + len(body)]
    if len(orig) != len(body):
        return "past the end of the file"
    want = bytearray(body)
    for off, rtype, sname in relocs:
        if rtype != R_MIPS_32:
            return f"relocation type {rtype} at +{off:#x}"
        target = sym_addr(sname, syms)
        if target is None:
            return f"address of {sname} unknown"
        addend, = struct.unpack_from("<I", body, off)
        struct.pack_into("<I", want, off, (target + addend) & 0xFFFFFFFF)
    if bytes(want) != orig:
        k = next(i for i in range(len(want)) if want[i] != orig[i])
        return f"differs from +{k:#x}"
    return None


def disasm(blob: bytes, vma: int) -> list[str]:
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        f.write(blob)
    try:
        p = subprocess.run([str(OBJDUMP), "-D", "-z", "-b", "binary", "-m", "mips:5900", "-EL",
                            f"--adjust-vma={vma:#x}", f.name], capture_output=True, text=True)
    finally:
        os.unlink(f.name)
    return [m.group(1).replace("\t", " ") for m in
            re.finditer(r"^\s*[0-9a-f]+:\s+[0-9a-f]{8}\s+(.*)$", p.stdout, re.M)]


def compare(body: bytes, relocs, orig: bytes, addr: int, syms) -> tuple[set, dict, dict]:
    """Differing words, notes per word, and the original address of each
    symbol of unknown address that a %hi/%lo pair loads (MWCC's @N)."""
    n = len(body) // 4
    new = struct.unpack_from(f"<{n}I", body)
    old = struct.unpack_from(f"<{n}I", orig)
    at: dict[int, list] = {}
    for off, rtype, sname in relocs:
        at.setdefault(off // 4, []).append((rtype, sname))
    bad, notes, pending_hi, loaded = set(), {}, {}, {}
    for i in range(n):
        a, b = old[i], new[i]
        if i not in at:
            if a != b:
                bad.add(i)
            continue
        for rtype, sname in at[i]:
            target = sym_addr(sname, syms)
            unknown = "" if target is not None else "?"
            if rtype == R_MIPS_26:
                ok = (a >> 26) == (b >> 26)
                dest = ((addr + 4 * i) & 0xF0000000) | ((a & 0x3FFFFFF) << 2)
                if target is not None and dest != target + ((b & 0x3FFFFFF) << 2):
                    ok = False
                notes[i] = sname + unknown
            elif rtype in (R_MIPS_HI16, R_MIPS_LO16):
                ok = (a >> 16) == (b >> 16)
                if rtype == R_MIPS_HI16:
                    # paired with its %lo by symbol and register: MWCC can schedule two
                    # `lui`s of one symbol (different addends) before their %lo's
                    pending_hi[sname] = pending_hi[(sname, (b >> 16) & 0x1F)] = (a & 0xFFFF, b & 0xFFFF)
                    notes[i] = f"%hi({sname}){unknown}"
                else:
                    key = (sname, (b >> 21) & 0x1F)
                    if key in pending_hi or sname in pending_hi:
                        hi_old, hi_new = pending_hi[key] if key in pending_hi else pending_hi[sname]
                        lo_old = (a & 0xFFFF) - ((a & 0x8000) << 1)
                        lo_new = (b & 0xFFFF) - ((b & 0x8000) << 1)
                        addend = ((hi_new << 16) + lo_new) & 0xFFFFFFFF
                        was = ((hi_old << 16) + lo_old) & 0xFFFFFFFF
                        if target is not None and was != (target + addend) & 0xFFFFFFFF:
                            ok = False
                        if target is None and addend == 0:
                            loaded[sname] = was
                    notes[i] = f"%lo({sname}){unknown}"
            else:
                ok = False
                notes[i] = f"relocation type {rtype} ({sname})"
            if not ok:
                bad.add(i)
    return bad, notes, loaded


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("module")
    ap.add_argument("cfile", type=Path)
    ap.add_argument("funcs", nargs="*")
    ap.add_argument("-D", dest="defines", action="append", default=[])
    ap.add_argument("-C", dest="context", type=int, default=3)
    args = ap.parse_args()
    image, base = module_image(args.module)
    syms = load_symbols(args.module)
    starts = function_starts(args.module)
    pieces = piece_starts(args.module, base)
    lines = args.cfile.resolve().read_text(encoding="utf-8").splitlines()
    text = "".join(f"#define {d}\n" for d in args.defines)
    text += "\n".join("" if SPLICE_LINE.match(l) else l for l in lines) + "\n"
    (ROOT / "build").mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=ROOT / "build") as td:
        src = Path(td) / args.cfile.name
        src.write_bytes(text.encode("shift_jis"))
        obj = Path(td) / "out.o"
        p = subprocess.run([str(WIBO), str(MWCC), *CFLAGS, "-o", str(obj), str(src)], cwd=ROOT,
                           env=dict(os.environ, MWCIncludes="."), capture_output=True, text=True)
        msg = (p.stdout + p.stderr).strip()
        if msg:
            print(msg)
        if p.returncode != 0:
            return 1
        funcs, other, objects = read_object(obj)
    ok = True
    names = [f[0] for f in funcs]
    for want in args.funcs:
        if want not in names:
            print(f"??    {want}: not in the compiled object")
            ok = False
    checked_data = 0
    for name, body, relocs in funcs:
        if args.funcs and name not in args.funcs:
            continue
        addr = sym_addr(name, syms)
        if addr is None:
            print(f"??    {name}: address unknown (name it func_XXXXXXXX or add it to config/symbols)")
            ok = False
            continue
        orig = image[addr - base:addr - base + len(body)]
        bad, notes, loaded = compare(body, relocs, orig, addr, syms)
        # The original may go on past the compiled code: dead code the
        # compiler kept (an epilogue after an endless loop). Up to the next
        # function there may only be padding. The file's own piece of the YAML
        # ends the tail too: what follows the file's last function may be data or
        # another file's code (Letterbox_Draw, Yn_LoadModule).
        k = bisect.bisect_right(starts, addr)
        j = bisect.bisect_right(pieces, addr)
        end = min(starts[k] if k < len(starts) else 1 << 32, pieces[j] if j < len(pieces) else 1 << 32)
        if k < len(starts) and addr + len(body) < end:
            tail = image[addr - base + len(body):end - base]
            if any(tail):
                # count the original's extra words (up to its last nonzero one) as differing
                extra = max(i // 4 + 1 for i in range(0, len(tail), 4) if any(tail[i:i + 4]))
                print(f"DIFF  {name}: {len(bad) + extra} of {len(body) // 4 + extra} words differ; the original"
                      f" has {len(tail):#x} more bytes before the next function or piece (at {end:08X}) that"
                      " aren't padding")
                ok = False
                if args.context < 0:
                    # the listing of the part both have
                    do, dn = disasm(orig, addr), disasm(body, addr)
                    for i in range(len(body) // 4):
                        mark = "!" if i in bad else ("~" if i in notes else " ")
                        print(f"  {addr + 4 * i:08X} {mark} {do[i] if i < len(do) else '':40.40} | "
                              f"{dn[i] if i < len(dn) else ''}  {notes.get(i, '')}")
                continue
        data_bad = []
        for sname, was in sorted(loaded.items(), key=lambda kv: kv[1]):
            if sname in objects:
                checked_data += 1
                why = check_object(sname, objects[sname], was, image, base, syms)
                if why:
                    data_bad.append(f"{sname} at {was:08X} ({len(objects[sname][0]):#x} bytes): {why}")
        if not bad and not data_bad:
            print(f"MATCH {name} ({len(body):#x} bytes)")
            continue
        ok = False
        if not bad:
            print(f"DIFF  {name}: code matches, data differs")
            for line in data_bad:
                print(f"  data {line}")
            continue
        print(f"DIFF  {name}: {len(bad)} of {len(body) // 4} words differ")
        for line in data_bad:
            print(f"  data {line}")
        do, dn = disasm(orig, addr), disasm(body, addr)
        show = set(range(len(body) // 4)) if args.context < 0 else \
            {j for i in bad for j in range(i - args.context, i + args.context + 1)}
        last = -2
        for i in range(len(body) // 4):
            if i not in show:
                continue
            if i != last + 1:
                print("  ...")
            last = i
            mark = "!" if i in bad else ("~" if i in notes else " ")
            left = do[i] if i < len(do) else ""
            right = dn[i] if i < len(dn) else ""
            print(f"  {addr + 4 * i:08X} {mark} {left:40.40} | {right}  {notes.get(i, '')}")
    sizes: dict[str, int] = {}
    for sec, size in other:
        sizes[sec] = sizes.get(sec, 0) + size
    if sizes:
        print("note: the object has " + ", ".join(f"{sec} ({size:#x} bytes)" for sec, size in sizes.items())
              + f"; {checked_data} jump tables or literals were checked above. For the build, the file must"
              " own that data: a `.rodata` (or `.data`) subsegment in the YAML (CONTRIBUTING.md,"
              " \"Functions with switch tables or strings\").")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
