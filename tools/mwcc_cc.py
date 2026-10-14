#!/usr/bin/env python3
"""Compile a C file with MWCC (via wibo), splicing in functions and data that
are still assembly.

A C file may mix decompiled functions with `INCLUDE_ASM("dir", name);` (or
`#pragma GLOBAL_ASM("path.s")`) lines for functions not yet decompiled, and
`INCLUDE_RODATA("dir", name);` lines for data objects not yet in C, and
`INCLUDE_DATA("dir", name);` lines for .data objects that must stay out of the
repository (their .s file, written at build time by tools/data_blob.py, takes
the bytes from the user's original). MWCC has no inline way to include
GNU-syntax assembly, so:

  1. each INCLUDE_ASM line is replaced by an empty marker function, and each
     INCLUDE_RODATA line by a marker variable (INCLUDE_DATA: one in .data). MWCC emits every function and
     every variable, string literal and jump table in its own section, in
     source order: a function's literals and tables come right before its
     code;
  2. the C file is converted to Shift-JIS (the game's encoding) and compiled;
  3. each included .s file is assembled with its sections renamed after its
     marker. A function's .s file can hold its jump tables and strings too
     (`.section .rodata`, which splat writes there when the C file owns its
     .rodata: the file has a `.rodata` subsegment in the YAML);
  4. `ld -r` puts everything back together in source order: the functions
     into one .text section, and each data section of the C object in turn,
     with the assembled data of a marker taking the marker's place. A marker
     function's data goes where the function's own would have been, right
     before its code.

A function named `Stripped_...` stands in for one the game's linker
dead-stripped: MWCC compiles it, since what it compiles can change the code
of the functions after it in the file (CONTRIBUTING, "Fakematches"), and its
code is then dropped like a marker's. It must have no data of its own.

Data sections are not merged: each string, table and variable stays a
section of its own, with MWCC's alignment (or, for assembled data, the
alignment of its original address), all named .data, .rodata or .bss. The
final link lays them out one by one exactly as the original linker did,
whatever the file's start address. (Merged here, they would be laid out
from a 16-byte-aligned start, which the original file need not have had.)

    mwcc_cc.py --out OUT.o --src IN.c --wibo WIBO --mwcc MWCCPS2.EXE --cflags "..."
               --as AS --asflags "..." --ld LD
"""

from __future__ import annotations

import argparse
import os
import re
import shlex
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
INCLUDE_ASM = re.compile(r'^\s*INCLUDE_ASM\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)\s*;?\s*$')
INCLUDE_RODATA = re.compile(r'^\s*INCLUDE_RODATA\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)\s*;?\s*$')
INCLUDE_DATA = re.compile(r'^\s*INCLUDE_DATA\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)\s*;?\s*$')
GLOBAL_ASM = re.compile(r'^\s*#\s*pragma\s+GLOBAL_ASM\(\s*"([^"]+)"\s*\)\s*$')
RODATA_PRAGMA = re.compile(r"^\s*#\s*pragma\s+rodata\s+(on|off)\s*$")
RO_MARKER = "__mwcc_ro_"
FUNC_MARKER = "__mwcc_gasm_"
DATA_MARKER = "__mwcc_gdata_"
STRIPPED = "Stripped_"   # a stand-in for a function the game's linker stripped: compiled, then dropped
DATA_KINDS = (".data", ".rodata", ".bss")
# The game's toolchain aligned every data object to at least 8 bytes: in all
# modules, 2- to 4-byte strings and 1- to 4-byte variables sit at multiples
# of 8 ("Jan", "Feb"... 8 bytes apart; gCseRpcSema, gCseRpcReqSema...).
# MWCC 3.0b52 gives such objects 4 (a char: 1), so their sections are raised
# to 8. Larger objects already get 8 or 16 from MWCC, as in the game.
MIN_DATA_ALIGN = 8
SECTION = re.compile(r"^\s*\.section\s+([^\s,]+)")
VRAM_COMMENT = re.compile(r"/\*\s*[0-9A-F]+\s+([0-9A-F]{8})\s")
# splat writes the VU0 macro instructions' special registers without the `$`
# in per-function files (`vmulax.xyzw ACC, $vf4, $vf8x`, `vdiv Q, ...`), which
# gas rejects; its full disassembly has `$ACC`, `$Q`, `$I`.
VU0_INSN = re.compile(r"^(\s*/\*[^*]*\*/\s+v[a-z0-9]+(?:\.[xyzw]+)?\s+)(.*?)(\s*)$")
VU0_SPECIAL = re.compile(r"(?<![\w$])(ACC|Q|I|R)(?!\w)")
DISCARD = "*(.mwcats) *(.rel.mwcats) *(.comment) *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.gnu.attributes) *(.mdebug*)"


def run(cmd: list[str], **kw) -> None:
    p = subprocess.run(cmd, **kw)
    if p.returncode != 0:
        sys.exit(p.returncode)


class Elf:
    """Just enough of a 32-bit little-endian relocatable ELF: section names
    and symbols, and renaming sections."""

    def __init__(self, path: Path):
        self.path = path
        self.data = bytearray(path.read_bytes())
        self.shoff, = struct.unpack_from("<I", self.data, 0x20)
        self.shentsize, self.shnum, self.shstrndx = struct.unpack_from("<HHH", self.data, 0x2E)
        self.shdrs = [list(struct.unpack_from("<10I", self.data, self.shoff + i * self.shentsize))
                      for i in range(self.shnum)]
        off, size = self.shdrs[self.shstrndx][4], self.shdrs[self.shstrndx][5]
        self.shstrtab = bytes(self.data[off:off + size])

    @staticmethod
    def _cstr(table: bytes, off: int) -> str:
        return table[off:table.index(b"\0", off)].decode()

    def name(self, i: int) -> str:
        return self._cstr(self.shstrtab, self.shdrs[i][0])

    def symbols(self) -> list[tuple[str, int, int, int]]:
        """(name, value, info, shndx) of every symbol."""
        symtab = next(i for i, h in enumerate(self.shdrs) if h[1] == 2)        # SHT_SYMTAB
        strh = self.shdrs[self.shdrs[symtab][6]]
        strtab = bytes(self.data[strh[4]:strh[4] + strh[5]])
        out = []
        off, size = self.shdrs[symtab][4], self.shdrs[symtab][5]
        for k in range(size // 16):
            st_name, st_value, _size, st_info, _other, st_shndx = struct.unpack_from("<IIIBBH", self.data, off + 16 * k)
            out.append((self._cstr(strtab, st_name), st_value, st_info, st_shndx))
        return out

    def min_align(self, i: int, align: int) -> None:
        """Raise section i's alignment to at least `align` (saved by rename())."""
        if self.shdrs[i][8] < align:
            self.shdrs[i][8] = align
            struct.pack_into("<I", self.data, self.shoff + i * self.shentsize + 32, align)

    def rename(self, names: dict[int, str]) -> None:
        """Give sections new names (a new .shstrtab is appended to the file)."""
        table = bytearray(self.shstrtab)
        offsets: dict[str, int] = {}
        for i, new in names.items():
            if new not in offsets:
                offsets[new] = len(table)
                table += new.encode() + b"\0"
            self.shdrs[i][0] = offsets[new]
            struct.pack_into("<I", self.data, self.shoff + i * self.shentsize, offsets[new])
        new_off = (len(self.data) + 3) & ~3
        self.data += b"\0" * (new_off - len(self.data)) + table
        struct.pack_into("<II", self.data, self.shoff + self.shstrndx * self.shentsize + 16, new_off, len(table))
        self.shstrtab = bytes(table)
        self.path.write_bytes(self.data)


def alignment_for(asm_text: str) -> int:
    """Alignment (log2) of a spliced function, from its original address."""
    m = VRAM_COMMENT.search(asm_text)
    if not m:
        return 4
    addr = int(m.group(1), 16)
    for p in (4, 3, 2):
        if addr % (1 << p) == 0:
            return p
    return 2


def section_kind(name: str) -> str:
    """.text, .data, .rodata or .bss for a section name in a spliced file."""
    for kind in (".text",) + DATA_KINDS:
        if name == kind or name.startswith(kind + "."):
            return kind
    if name in (".rdata", ".late_rodata"):
        return ".rodata"
    sys.exit(f"mwcc_cc.py: unexpected section {name} in spliced assembly")


def splice_source(marker: str, text: str, default_kind: str) -> tuple[str, list[tuple[str, str]]]:
    """Rewrite a spliced .s file so that each of its sections is named after
    the marker: .text.<marker>, and .rodata.<marker>.<n> etc. for its data.
    Returns the wrapper source and [(kind, section name)] in file order."""
    out = ['.include "macro.inc"\n', ".set noat\n", ".set noreorder\n"]
    sections: list[tuple[str, str]] = []

    def begin(kind: str) -> None:
        if kind == ".text":
            name = f".text.{marker}"
            if (kind, name) not in sections:
                sections.append((kind, name))
            out.append(f'.section {name}, "ax"\n.align {alignment_for(text)}\n')
            return
        name = f"{kind}.{marker}.{len(sections)}"
        sections.append((kind, name))
        flags = '"a"' if kind == ".rodata" else '"wa", @nobits' if kind == ".bss" else '"wa"'
        out.append(f".section {name}, {flags}\n")

    begin(default_kind)
    for line in text.splitlines(keepends=True):
        m = SECTION.match(line)
        if m:
            begin(section_kind(m.group(1)))
            out.append("\n")        # keeps the line count close for error messages
            continue
        m = VU0_INSN.match(line)
        if m:
            line = m.group(1) + VU0_SPECIAL.sub(r"$\1", m.group(2)) + m.group(3)
        out.append(line)
    out.append("\n")
    return "".join(out), sections


def main() -> int:
    ap = argparse.ArgumentParser()
    for a in ("--out", "--src", "--wibo", "--mwcc", "--cflags", "--as", "--asflags", "--ld"):
        ap.add_argument(a, required=True)
    args = ap.parse_args()
    out, src = Path(args.out), Path(args.src)
    out.parent.mkdir(parents=True, exist_ok=True)

    lines = src.read_text(encoding="utf-8").splitlines(keepends=True)
    splices: list[tuple[str, Path, str]] = []          # (marker, .s path, default section kind)
    for i, line in enumerate(lines):
        m = INCLUDE_ASM.match(line) or GLOBAL_ASM.match(line)
        if m:
            path = Path(m.group(1)) / f"{m.group(2)}.s" if m.re is INCLUDE_ASM else Path(m.group(1))
            marker = f"{FUNC_MARKER}{len(splices)}"
            splices.append((marker, path, ".text"))
            lines[i] = f"void {marker}(void) {{}}\n"
            continue
        m = INCLUDE_RODATA.match(line)
        if m:
            marker = f"{DATA_MARKER}{len(splices)}"
            splices.append((marker, Path(m.group(1)) / f"{m.group(2)}.s", ".rodata"))
            lines[i] = f"const int {marker} = 1;\n"
            continue
        m = INCLUDE_DATA.match(line)
        if m:
            # a .data object from the user's original (tools/data_blob.py writes its .s)
            marker = f"{DATA_MARKER}{len(splices)}"
            splices.append((marker, Path(m.group(1)) / f"{m.group(2)}.s", ".data"))
            lines[i] = f"int {marker} = 1;\n"
            continue
        m = RODATA_PRAGMA.match(line)
        if m:
            # variables of .rodata that the code declares without `const` (CONTRIBUTING.md,
            # "Data in C"): their sections go to .rodata, in place
            lines[i] = f"int {RO_MARKER}{m.group(1)}_{i} = 1;\n"

    with tempfile.TemporaryDirectory(dir=out.parent) as td:
        td = Path(td)
        csrc = td / src.name
        # MWCC reads the source bytes as-is; the game's text is Shift-JIS.
        csrc.write_bytes("".join(lines).encode("shift_jis"))
        cobj = td / "c.o"
        env = dict(os.environ, MWCIncludes=".")
        run([args.wibo, args.mwcc, *shlex.split(args.cflags), "-o", str(cobj), str(csrc)], env=env)

        # `static` functions: MWCC gives their callers in the file code that relies on the
        # registers the callee leaves alone, which is how the game's static functions show.
        # Their symbols are local, so the assembly still spliced in (and the data assembly)
        # could not call them: make them global. The code doesn't change.
        local_funcs = [name for name, _v, info, shndx in Elf(cobj).symbols()
                       if (info & 0xF) == 2 and (info >> 4) == 0 and shndx and re.fullmatch(r"[A-Za-z_]\w*", name)
                       and not name.startswith(STRIPPED)]
        if local_funcs:
            objcopy = re.sub(r"ld$", "objcopy", args.ld)
            run([objcopy, *[f"--globalize-symbol={n}" for n in local_funcs], str(cobj)])

        # Name every section of the C object uniquely, remembering the order
        # MWCC emitted them in: .text.<function>, and .<kind>.s<index> for data.
        elf = Elf(cobj)
        syms = elf.symbols()
        func_in = {shndx: name for name, value, info, shndx in syms if (info & 0xF) == 2 and value == 0}
        marker_in = {shndx: name for name, _v, _i, shndx in syms if name.startswith(DATA_MARKER)}
        ro_in = {shndx: name for name, _v, _i, shndx in syms if name.startswith(RO_MARKER)}
        ro = False
        renames: dict[int, str] = {}
        order: list[tuple[str, int, str | None]] = []   # (kind, section index, function or marker)
        for i in range(elf.shnum):
            name = elf.name(i)
            if name == ".text" and i in func_in and func_in[i].startswith(STRIPPED):
                # compiled for its effect on the functions after it; the game's linker dropped it
                renames[i] = f".text.{func_in[i]}"
                order.append(("marker", i, None))
            elif name == ".text" and i in func_in:
                renames[i] = f".text.{func_in[i]}"
                order.append((".text", i, func_in[i]))
            elif name in DATA_KINDS and i in ro_in:
                # `#pragma rodata on/off`: its marker goes, and switches the variables after it
                ro = ro_in[i].startswith(RO_MARKER + "on")
                renames[i] = f"{name}.s{i}"
                order.append(("marker", i, None))
            elif name in DATA_KINDS:
                kind = ".rodata" if ro and name == ".data" else name
                renames[i] = f"{kind}.s{i}"
                order.append((kind, i, marker_in.get(i)))
                elf.min_align(i, MIN_DATA_ALIGN)
        elf.rename(renames)

        # Assemble the spliced files.
        spliced: dict[str, tuple[Path, list[tuple[str, str]]]] = {}
        for marker, path, kind in splices:
            if not (ROOT / path).exists():
                sys.exit(f"mwcc_cc.py: {path} not found. splat writes a function's or a data object's .s"
                         " file when it splits (python tools/configure.py; --split to force it)")
            wrapper = td / f"{marker}.s"
            source, sections = splice_source(marker, (ROOT / path).read_text(encoding="utf-8"), kind)
            wrapper.write_text(source, encoding="utf-8")
            o = td / f"{marker}.o"
            run([sys.executable, str(HERE / "sjis_as.py"), args.__dict__["as"], str(wrapper), str(o),
                 *shlex.split(args.asflags)])
            spliced[marker] = (o, sections)
        emitted = {key for kind, _i, key in order if key}
        missing = [m for m, _p, _k in splices if m not in emitted]
        if missing:
            sys.exit(f"mwcc_cc.py: MWCC did not emit the markers {missing}")

        # A `static` function that spliced assembly still calls: MWCC has compiled
        # its C callers knowing which registers it leaves alone (docs/mwcc-matching.md);
        # the assembly only needs the symbol, so it is made global in the object.
        undef = {name for o, _s in spliced.values() for name, _v, _i, shndx in Elf(o).symbols() if name and shndx == 0}
        static = sorted({name for name, _v, info, shndx in syms
                         if (info >> 4) == 0 and (info & 0xF) == 2 and shndx != 0 and name in undef})
        if static:
            objcopy = Path(args.ld).with_name(Path(args.ld).name[:-2] + "objcopy")
            run([str(objcopy)] + [a for s in static for a in ("--globalize-symbol", s)] + [str(cobj)])

        # Lay everything out in source order.
        text_items: list[str] = []
        data_items: list[tuple[str, str]] = []            # (kind, input section spec)
        discard: list[str] = []

        def add_spliced(marker: str) -> None:
            o, sections = spliced[marker]
            for kind, name in sections:
                if kind == ".text":
                    text_items.append(f"{o}({name})")
                else:
                    data_items.append((kind, f"{o}({name})"))

        for kind, i, key in order:
            if kind == "marker":
                discard.append(f"{cobj}({renames[i]})")
            elif key in spliced:
                # A marker: its assembly, data first (a function's literals and
                # tables precede its code, as MWCC emits them), replaces it.
                discard.append(f"{cobj}({renames[i]})")
                add_spliced(key)
            elif kind == ".text":
                text_items.append(f"{cobj}({renames[i]})")
            else:
                data_items.append((kind, f"{cobj}({renames[i]})"))

        # One output section per data section (merged later by name only).
        count = {k: 0 for k in DATA_KINDS}
        data_out = []
        for kind, spec in data_items:
            data_out.append(f"  {kind}.{count[kind]} 0 : {{ {spec} }}\n")
            count[kind] += 1
        script = td / "splice.ld"
        script.write_text(
            "SECTIONS {\n"
            f"  .text 0 : {{ {' '.join(text_items)} }}\n"
            + "".join(data_out) +
            f"  /DISCARD/ : {{ {' '.join(discard)} {DISCARD} }}\n"
            "}\n")
        run([args.ld, "-EL", "-r", "-T", str(script), "-o", str(out), cobj] +
            [str(o) for o, _s in spliced.values()])

    # .data.3 -> .data, .rel.rodata.1 -> .rel.rodata: the final link script
    # takes each file's `.data` etc., which now match every piece in order.
    res = Elf(out)
    back = {}
    for i in range(res.shnum):
        name = res.name(i)
        for kind in DATA_KINDS:
            for prefix in (kind, ".rel" + kind):
                if re.fullmatch(re.escape(prefix) + r"\.\d+", name):
                    back[i] = prefix
    if back:
        res.rename(back)
    return 0


if __name__ == "__main__":
    sys.exit(main())
