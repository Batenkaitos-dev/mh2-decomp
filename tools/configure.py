#!/usr/bin/env python3
"""Generate build.ninja for the matching build of Monster Hunter 2 (Dos) v1.04.

The matching build runs on Linux (on Windows: inside WSL), because the
original compiler is a Windows program run through wibo. Steps:

  1. extract the boot ELF and the 12 overlays from the user's copy
     (tools/extract.py -> build/orig/)
  2. split each target with splat (config/*.yaml -> asm/, assets/, build/ld/)
  3. write build.ninja: assemble, compile, link, objcopy, SHA-1 check

Usage (after `source tools/env.sh`):
    python tools/configure.py            # extract + split what changed + write build.ninja
    python tools/configure.py --split    # re-run splat for every target
    ninja                                # build all 13 targets and check them

`--split-only` stops after step 2 (asm/, build/orig/, build/gen/); it also
runs on Windows.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TOOLS = ROOT / "tools"
DEPS = TOOLS / "deps" / "linux-x64"

# Matching compiler flags (docs/mwcc-matching.md, "The compiler").
# `-str readonly`: string literals go to .rodata, next to the jump tables, as
# in the game. `-i build/gen`: the content tools/content_gen.py writes from
# the user's copy (CONTRIBUTING, "Content").
CFLAGS = "-c -nostdinc -O3,p -sdatathreshold 0 -str readonly -i include -i build/gen"
ASFLAGS = "-EL -march=r5900 -mabi=eabi -G0 -no-pad-sections -I build/include -I include"

# Modules whose names other targets may reference (their code is always
# resident): every config/symbols/ file the module's YAML lists (main: main.txt
# and the library files, sdk_*.txt, gcc_*.txt, cri_*.txt) goes into one "extern"
# copy with absolute:True for splat and a linker-script fragment defining the
# names for the link.
EXTERN_SYMBOLS = {"main": "config/main.yaml", "sub_main": "config/overlays/sub_main.yaml"}
# Link-only names of the modules loaded together. A module's code and data may
# point into an overlay that is in memory with it (sub_main's area tables into
# gm_sub's data, its pause menu into game's functions, its scene steps start
# plsel's tasks; select's title screen into gm_sub). Each module listed here
# links build/symbols/<module>.links.ld:
# a PROVIDE() line for every name in the symbol files and config/ld aliases of
# the modules it is loaded with. PROVIDE defines a name only where the code
# references it and nothing else defines it, so nothing is exported to splat
# and no module's output changes. A name two of the modules give different
# addresses (an address-derived name of overlays loaded at the same address)
# is left out; so are names main or sub_main already export. main's own code
# reads sub_main's objects too (sub_main is always loaded), and a flag of the
# lobby's while the lobby is loaded (file_queue.c): main links their names the
# same way.
LOADED_WITH = {
    "main": ["sub_main", "lobby"],
    "sub_main": ["gm_sub", "game", "lobby", "plsel"],
    "game": ["gm_sub"],
    "gm_sub": ["game", "select"],
    "select": ["gm_sub", "plsel"],
    "plsel": ["game", "select", "gm_sub", "lobby"],
    "lobby": ["lbguild", "nethttp"],
    "lbguild": ["lobby"],
    "nethttp": ["lobby"],
    "yn": ["lobby", "select"],
}
ALIAS_LINE = re.compile(r"^\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*)\s*\+\s*(0x[0-9A-Fa-f]+|\d+)\s*;")
SYM_LINE = re.compile(r"^\s*([A-Za-z_.$][\w.$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;(.*)$")
# Progress categories by origin, besides one per module:
# - game: Capcom's code, the headline figure;
# - sdk: Sony's EE libraries, the toolchain's libc/libm/libgcc, CodeWarrior's
#   crt0 and runtime objects, nethttp's libraries and DNAS (asm pieces named
#   sdk/, gcc/, mw/, net/);
# - middleware: CRI's Sofdec and ADX (cri/);
# - access: ACCESS's kana-kanji engine in sub_main, 0x2F9D80-0x305E60
#   (asm, data and rodata pieces named access/);
# - vu1: the VU1 microcode (sub_main's vu1_microcode);
# - zlib: zlib 1.1.4, third-party, decompiled: the lobby's inflate and
#   adler32 (ZLIB_UNITS), whose C stays under zlib's licence (README,
#   "Legal").
# Their ids are origin/<name> (the game overlay's module id is 'game').
KIND_CATEGORIES = [("origin/" + k, k) for k in ("game", "sdk", "middleware", "access", "vu1", "zlib")]
# Padding is not code: the 0x40 zero bytes every overlay's text starts with
# (an asm piece 'text' before the first unit) and lbguild's and test's empty
# text (a textbin 'text'). These units get only the 'padding' category, so no
# module or origin row counts them (tools/progress.py takes them out of TOTAL).
PADDING = "padding"
PADDING_UNITS = {"text", "data/text"}
ZLIB_UNITS = {f"lobby/{n}" for n in ("adler32", "infblock", "inflate", "infutil")}


def unit_kind(unit_name: str) -> str:
    """The origin category of an objdiff unit (module/path)."""
    module, _, rest = unit_name.partition("/")
    if rest in PADDING_UNITS:
        return PADDING
    rest = rest.removeprefix("data/")
    if rest.startswith(("sdk/", "gcc/", "mw/", "net/")):
        return "origin/sdk"
    if rest.startswith("cri/"):
        return "origin/middleware"
    if rest == "vu1_microcode":
        return "origin/vu1"
    if rest.startswith("access/"):
        return "origin/access"
    if f"{module}/{rest}" in ZLIB_UNITS:
        return "origin/zlib"
    return "origin/game"


def unit_categories(module: str, unit_name: str) -> list[str]:
    """A unit's objdiff progress categories: its module and its origin, or
    only 'padding'."""
    kind = unit_kind(unit_name)
    return [kind] if kind == PADDING else [module, kind]




class Target:
    def __init__(self, name: str, sha1: str, yaml: Path, module: str, externs: list[str]):
        self.name = name              # output file name, e.g. SLPM_662.80 or game.bin
        self.sha1 = sha1
        self.yaml = yaml
        self.module = module          # splat basename / build dir, e.g. main or game
        self.externs = externs        # extern symbol sets this target links against

    @property
    def ld(self) -> Path:
        return Path("build/ld") / f"{self.module}.ld"


def load_targets() -> list[Target]:
    targets = []
    for line in (ROOT / "config" / "targets.sha1").read_text().splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        sha1, _size, name, _source = line.split()
        if name == "SLPM_662.80":
            targets.append(Target(name, sha1, Path("config/main.yaml"), "main", []))
        else:
            module = name.removesuffix(".bin")
            externs = ["main"] if module == "sub_main" else ["main", "sub_main"]
            targets.append(Target(name, sha1, Path(f"config/overlays/{module}.yaml"), module, externs))
    return targets


def write_extern_symbols() -> None:
    """build/symbols/<m>.extern.txt (splat) and <m>.extern.ld (linker)."""
    out_dir = ROOT / "build" / "symbols"
    out_dir.mkdir(parents=True, exist_ok=True)
    for module, yaml_path in EXTERN_SYMBOLS.items():
        txt, ld = [], []
        yaml_text = (ROOT / yaml_path).read_text(encoding="utf-8")
        srcs = re.findall(r"^\s*-\s*(config/symbols/\S+\.txt)\s*$", yaml_text, re.M)
        lines = [l for src in srcs for l in (ROOT / src).read_text(encoding="utf-8").splitlines()]
        for line in lines:
            m = SYM_LINE.match(line)
            if not m:
                txt.append(line)
                continue
            name, addr, rest = m.groups()
            attrs = rest.strip()
            attrs = (attrs + " absolute:True") if attrs.startswith("//") else "// absolute:True"
            txt.append(f"{name} = {addr}; {attrs}")
            ld.append(f"{name} = {addr};")
        _write_if_changed(out_dir / f"{module}.extern.txt", "\n".join(txt) + "\n")
        _write_if_changed(out_dir / f"{module}.extern.ld", "\n".join(ld) + "\n")


def _module_symbols(module: str) -> dict[str, int]:
    """name -> address for a module's own names: its YAML's config/ symbol files
    and its config/ld aliases (evaluated against those names, or D_ADDR)."""
    yaml_path = EXTERN_SYMBOLS.get(module, f"config/overlays/{module}.yaml")
    yaml_text = (ROOT / yaml_path).read_text(encoding="utf-8")
    srcs = re.findall(r"^\s*-\s*(config/\S+\.txt)\s*$", yaml_text, re.M)
    names: dict[str, int] = {}
    for src in srcs:
        for line in (ROOT / src).read_text(encoding="utf-8").splitlines():
            m = SYM_LINE.match(line)
            if m:
                names.setdefault(m.group(1), int(m.group(2), 16))
    aliases = ROOT / "config" / "ld" / f"{module}_aliases.ld"
    if aliases.exists():
        for line in aliases.read_text(encoding="utf-8").splitlines():
            m = ALIAS_LINE.match(line)
            if not m:
                continue
            name, base, off = m.groups()
            addr = names.get(base)
            if addr is None and re.fullmatch(r"D_[0-9A-Fa-f]{6,8}", base):
                addr = int(base[2:], 16)
            if addr is not None:
                names.setdefault(name, addr + int(off, 0))
    return names


def write_link_symbols() -> None:
    """build/symbols/<m>.links.ld: PROVIDE lines for the names of the modules
    loaded with <m> (LOADED_WITH)."""
    out_dir = ROOT / "build" / "symbols"
    out_dir.mkdir(parents=True, exist_ok=True)
    cache: dict[str, dict[str, int]] = {}
    sym = lambda m: cache.setdefault(m, _module_symbols(m))
    for module, others in LOADED_WITH.items():
        # the module's own names, and those main and sub_main already export to it
        exported = {"main": [], "sub_main": ["main"]}.get(module, ["main", "sub_main"])
        own = set(sym(module)).union(*(set(sym(e)) for e in exported))
        seen: dict[str, int] = {}
        clash: set[str] = set()
        for other in others:
            for name, addr in sym(other).items():
                if name in own:
                    continue
                if seen.setdefault(name, addr) != addr:
                    clash.add(name)
        lines = [f"/* Link-only: names of {', '.join(others)} (loaded with {module}). */",
                 "/* tools/configure.py writes this file (LOADED_WITH). */"]
        lines += [f"PROVIDE({n} = 0x{a:08X});" for n, a in sorted(seen.items(), key=lambda x: (x[1], x[0]))
                  if n not in clash]
        _write_if_changed(out_dir / f"{module}.links.ld", "\n".join(lines) + "\n")


def _write_if_changed(path: Path, text: str) -> None:
    if not path.exists() or path.read_text(encoding="utf-8") != text:
        path.write_text(text, encoding="utf-8", newline="\n")


def splat_fingerprint(t: Target) -> str:
    h = hashlib.sha1()
    h.update(t.sha1.encode())
    import splat
    h.update(getattr(splat, "__version__", "?").encode())
    h.update((ROOT / t.yaml).read_bytes())
    for path in _symbol_files(t):
        h.update(path.read_bytes())
    return h.hexdigest()


def _symbol_files(t: Target) -> list[Path]:
    text = (ROOT / t.yaml).read_text(encoding="utf-8")
    return [ROOT / p for p in re.findall(r"^\s*-\s*((?:config|build)/symbols/\S+\.txt)\s*$", text, re.M)]


def run_splat(t: Target, force: bool) -> None:
    stamp = ROOT / "build" / "splat" / f"{t.module}.stamp"
    fp = splat_fingerprint(t)
    if not force and stamp.exists() and stamp.read_text() == fp and (ROOT / t.ld).exists():
        return
    print(f"[..]   splat {t.yaml}")
    shutil.rmtree(ROOT / "asm" / t.module, ignore_errors=True)
    p = subprocess.run([sys.executable, "-m", "splat", "split", str(t.yaml)], cwd=ROOT,
                       capture_output=True, text=True, errors="replace")
    log = ROOT / "build" / "splat" / f"{t.module}.log"
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text(p.stdout + p.stderr, encoding="utf-8")
    if p.returncode != 0 or not (ROOT / t.ld).exists():
        sys.exit(f"splat failed for {t.yaml}; see {log.relative_to(ROOT)}")
    stamp.write_text(fp)
    print(f"[done] splat {t.yaml}")


def objects_from_ld(t: Target) -> list[str]:
    seen, objs = set(), []
    for m in re.finditer(r"(build/\S+?\.o)\(", (ROOT / t.ld).read_text()):
        if m.group(1) not in seen:
            seen.add(m.group(1))
            objs.append(m.group(1))
    return objs


def source_for(t: Target, obj: str) -> tuple[str, str]:
    """Map a splat object path to (rule, source path).

    An asm piece with a committed source, src/<module>/<piece>.s (Capcom's
    hand-written assembly), is built from it; splat's disassembly of the
    piece (asm/<module>/<piece>.s) stays what objdiff compares it with.
    Likewise a textbin piece with a
    src/<module>/<piece>.vsm (the VU1 microcode, tools/vu1as.py)."""
    rel = obj.removeprefix(f"build/{t.module}/").removesuffix(".o")
    rel = re.sub(r"\.(s|c|bin)$", "", rel)
    if rel.startswith("asm/") and (ROOT / ("src/" + rel.removeprefix("asm/").replace("/data/", "/") + ".vsm")).exists():
        return "vu1as", "src/" + rel.removeprefix("asm/").replace("/data/", "/") + ".vsm"
    if rel.startswith("asm/") and (ROOT / ("src/" + rel.removeprefix("asm/") + ".s")).exists():
        return "as", "src/" + rel.removeprefix("asm/") + ".s"
    for ext, rule in ((".c", "cc"), (".s", "as"), (".bin", "incbin")):
        if (ROOT / (rel + ext)).exists():
            return rule, rel + ext
    desc = asset_description(rel)
    if desc:
        # a described asset: tools/asset_c.py writes its C from the user's original
        return "cc", "build/assets/" + rel.removeprefix("src/") + ".c"
    sys.exit(f"no source found for {obj} (looked for {rel}.c/.s/.bin)")


def asset_description(rel: str) -> str | None:
    """config/assets/MODULE/FILE.txt for a C piece src/MODULE/FILE with no src/ file: a
    described asset (CONTRIBUTING, "Described assets")."""
    desc = "config/assets/" + rel.removeprefix("src/") + ".txt"
    return desc if rel.startswith("src/") and (ROOT / desc).exists() else None


def included_asm(c_src: str) -> list[str]:
    """Data assembly files a C file splices in via INCLUDE_DATA."""
    deps = []
    for line in (ROOT / c_src).read_text(encoding="utf-8").splitlines():
        m = re.match(r'\s*INCLUDE_DATA\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)', line)
        if m:
            deps.append(f"{m.group(1)}/{m.group(2)}.s")
    return deps


def included_data(c_src: str) -> list[tuple[str, str]]:
    """(.s path, name) of a C file's INCLUDE_DATA lines: objects that must stay out of
    the repository, whose .s tools/data_blob.py writes from the user's original."""
    out = []
    for line in (ROOT / c_src).read_text(encoding="utf-8").splitlines():
        m = re.match(r'\s*INCLUDE_DATA\(\s*"([^"]+)"\s*,\s*(\w+)\s*\)', line)
        if m:
            if not m.group(1).startswith("build/"):
                sys.exit(f"{c_src}: INCLUDE_DATA's directory must be under build/ (git-ignored): {m.group(1)}")
            out.append((f"{m.group(1)}/{m.group(2)}.s", m.group(2)))
    return out


def uses_content(c_src: str) -> bool:
    """Whether a C file includes content tools/content_gen.py writes (a data initializer's
    `#include "MODULE/ADDR.inc"`, or texts: TXT(0xADDR))."""
    text = (ROOT / c_src).read_text(encoding="utf-8")
    return bool(re.search(r'^\s*#\s*include\s+"\w+/[0-9A-F]{8}\.inc"', text, re.M) or re.search(r"\bTXT\(", text))


def tool(name: str) -> str:
    path = shutil.which(name)
    if not path:
        sys.exit(f"{name} not found on PATH; run `source tools/env.sh` (after tools/setup.sh)")
    return path


def write_ninja(targets: list[Target]) -> None:
    lines = [
        "# Generated by tools/configure.py. Do not edit.",
        "ninja_required_version = 1.10",
        "",
        f"python = {sys.executable}",
        f"as = {tool('mips-ps2-decompals-as')}",
        f"ld = {tool('mips-ps2-decompals-ld')}",
        f"objcopy = {tool('mips-ps2-decompals-objcopy')}",
        f"wibo = {tool('wibo')}",
        f"mwcc = {DEPS / 'mwcc' / 'mwccps2.exe'}",
        f"asflags = {ASFLAGS}",
        f"cflags = {CFLAGS}",
        "",
        "rule as",
        "  command = $python tools/sjis_as.py $as $in $out $asflags",
        "  description = AS $in",
        "rule vu1as",
        "  command = $python tools/vu1as.py $in $out --symbol $symbol",
        "  description = VU1AS $in",
        "rule incbin",
        "  command = printf '.section .data\\n.incbin \"%s\"\\n' $in | $as $asflags -o $out -",
        "  description = BIN $in",
        "rule cc",
        "  command = $python tools/mwcc_cc.py --out $out --src $in --wibo $wibo --mwcc $mwcc"
        " --cflags \"$cflags\" --as $as --asflags \"$asflags\" --ld $ld",
        "  description = CC $in",
        "rule asset",
        "  command = $python tools/asset_c.py $in $out",
        "  description = ASSET $out",
        "rule blob",
        "  command = $python tools/data_blob.py $module $name $out",
        "  description = BLOB $name",
        "rule content",
        "  command = $python tools/content_gen.py $in --jobs 1 --stamp $out",
        "  description = CONTENT $in",
        "rule ld",
        "  command = $ld -EL $ldflags -Map $map --no-check-sections -o $out",
        "  description = LD $out",
        "rule objcopy",
        "  command = $objcopy -O binary $in $out",
        "  description = OBJCOPY $out",
        "rule check",
        "  command = $python tools/check_targets.py $in && touch $out",
        "  description = CHECK $in",
        "rule provcheck",
        "  command = $python tools/provenance.py --check --quiet && touch $out",
        "  description = PROVENANCE (every byte of code from src/ or a labelled library piece)",
        "rule provenance",
        "  command = $python tools/provenance.py",
        "  description = PROVENANCE",
        "rule plaincheck",
        "  command = $python tools/plain_check.py && touch $out",
        "  description = PLAIN MWCC (every function as the wrapper compiles it)",
        "rule configure",
        "  command = $python tools/configure.py",
        "  generator = 1",
        "  description = CONFIGURE",
        "",
    ]
    checks, expected = [], []
    units = []
    headers = sorted(str(p.relative_to(ROOT)).replace("\\", "/") for p in (ROOT / "include").rglob("*.h"))
    for t in targets:
        objs = objects_from_ld(t)
        for obj in objs:
            rule, src = source_for(t, obj)
            if rule == "as":
                implicit = " | tools/sjis_as.py"
            elif rule == "cc" and src.startswith("build/assets/"):
                # a described asset: its C is written from the description and the user's
                # original (which configure.py copied to the YAML's target_path)
                import yaml
                orig = yaml.safe_load((ROOT / t.yaml).read_text(encoding="utf-8"))["options"]["target_path"]
                desc = asset_description("src/" + src.removeprefix("build/assets/").removesuffix(".c"))
                lines.append(f"build {src}: asset {desc} | tools/asset_c.py tools/gen_data.py "
                             f"tools/type_data.py {t.yaml.as_posix()} {orig} {' '.join(headers)}")
                implicit = " | " + " ".join(["tools/mwcc_cc.py", "tools/sjis_as.py", *headers])
            elif rule == "cc":
                implicit = " | " + " ".join(["tools/mwcc_cc.py", "tools/sjis_as.py", *headers, *included_asm(src)])
                if uses_content(src):
                    # the file's content (data initializers, texts), written from the user's
                    # original by tools/content_gen.py (CONTRIBUTING, "Content")
                    import yaml
                    orig = yaml.safe_load((ROOT / t.yaml).read_text(encoding="utf-8"))["options"]["target_path"]
                    stamp = "build/gen/" + src.removeprefix("src/").removesuffix(".c") + ".stamp"
                    # (and the VU1 programs' source: content_gen takes their names from it)
                    deps = ["tools/content_gen.py", t.yaml.as_posix(), orig, *headers,
                            *(p.relative_to(ROOT).as_posix() for p in _symbol_files(t) if p.exists()),
                            "tools/vu1as.py",
                            *(p.relative_to(ROOT).as_posix() for p in sorted(ROOT.glob("src/*/*.vsm")))]
                    lines.append(f"build {stamp}: content {src} | {' '.join(deps)}")
                    implicit += f" {stamp}"
                blobs = included_data(src)
                if blobs:
                    # INCLUDE_DATA: the .s is written here, from the user's original; the
                    # object reads that original too (.incbin)
                    import yaml
                    orig = yaml.safe_load((ROOT / t.yaml).read_text(encoding="utf-8"))["options"]["target_path"]
                    implicit += f" {orig}"
                    for spath, name in blobs:
                        deps = " ".join(["tools/data_blob.py", t.yaml.as_posix(), orig,
                                         *(p.relative_to(ROOT).as_posix() for p in _symbol_files(t))])
                        lines.append(f"build {spath}: blob | {deps}")
                        lines.append(f"  module = {t.module}")
                        lines.append(f"  name = {name}")
            elif rule == "vu1as":
                # VU1 microcode source: tools/vu1as.py writes GNU as source (the block's
                # .words under the textbin's symbol), assembled as usual; objdiff's target
                # is splat's .incbin of the same range from the user's copy
                piece = Path(src).stem
                gen = f"build/{t.module}/{piece}.vsm.s"
                lines.append(f"build {gen}: vu1as {src} | tools/vu1as.py")
                lines.append(f"  symbol = __{piece}_textbin")
                lines.append(f"build {obj}: as {gen} | tools/sjis_as.py")
                exp = f"build/expected/{t.module}/{piece}.o"
                target_asm = obj.removeprefix(f"build/{t.module}/").removesuffix(".o") + ".s"
                lines.append(f"build {exp}: as {target_asm} | tools/sjis_as.py")
                expected.append(exp)
                unit_name = f"{t.module}/{piece}"
                units.append({"name": unit_name, "target_path": exp, "base_path": obj,
                              "metadata": {"source_path": src, "progress_categories": unit_categories(t.module, unit_name)}})
                continue
            else:
                implicit = ""
            lines.append(f"build {obj}: {rule} {src}{implicit}")
            unit_name = f"{t.module}/{Path(src).with_suffix('').as_posix().split('/', 2)[-1]}"
            if src.startswith("build/assets/"):
                unit_name = f"{t.module}/{Path(src).stem}"
            if rule == "cc":
                # objdiff target: splat's full disassembly of this file's range.
                full_asm = "asm/" + src.removeprefix("src/").removesuffix(".c") + ".s"
                exp = f"build/expected/{t.module}/" + src.removeprefix(f"src/{t.module}/").removesuffix(".c") + ".o"
                if not (ROOT / full_asm).exists():
                    # (a file of data only, lbguild.c: splat writes no disassembly)
                    units.append({"name": unit_name, "base_path": obj,
                                  "metadata": {"source_path": src, "progress_categories": unit_categories(t.module, unit_name)}})
                    continue
                lines.append(f"build {exp}: as {full_asm} | tools/sjis_as.py")
                expected.append(exp)
                units.append({"name": unit_name, "target_path": exp, "base_path": obj,
                              "metadata": {"source_path": src, "progress_categories": unit_categories(t.module, unit_name)}})
            elif rule == "as" and src.startswith("src/"):
                # hand-written assembly: a diffable unit like a C file (target = splat's
                # disassembly of the same range)
                exp = f"build/expected/{t.module}/" + src.removeprefix(f"src/{t.module}/").removesuffix(".s") + ".o"
                lines.append(f"build {exp}: as asm/{src.removeprefix('src/')} | tools/sjis_as.py")
                expected.append(exp)
                units.append({"name": unit_name, "target_path": exp, "base_path": obj,
                              "metadata": {"source_path": src, "progress_categories": unit_categories(t.module, unit_name)}})
            elif rule == "as":
                units.append({"name": unit_name, "target_path": obj,
                              "metadata": {"progress_categories": unit_categories(t.module, unit_name)}})
        base = Path("build/ld") / t.ld.stem
        ld_inputs = [str(t.ld), f"{base}.undefined_funcs_auto.txt", f"{base}.undefined_syms_auto.txt"]
        ld_inputs += [f"build/symbols/{e}.extern.ld" for e in t.externs]
        # Optional: names inside C-owned data objects, each defined relative to
        # the object that contains it (D_X = D_OBJ + 0xOFF;). See CONTRIBUTING.md, "Data in C".
        aliases = Path("config/ld") / f"{t.module}_aliases.ld"
        if (ROOT / aliases).exists():
            ld_inputs.append(aliases.as_posix())
        if t.module in LOADED_WITH:
            ld_inputs.append(f"build/symbols/{t.module}.links.ld")
        elf, mapfile, out = f"build/{t.module}.elf", f"build/{t.module}.map", f"build/out/{t.name}"
        lines.append(f"build {elf} | {mapfile}: ld {' '.join(objs)} | {' '.join(ld_inputs)}")
        lines.append("  ldflags = " + " ".join(f"-T {p}" for p in ld_inputs))
        lines.append(f"  map = {mapfile}")
        lines.append(f"build {out}: objcopy {elf}")
        lines.append(f"build {out}.ok: check {out} | tools/check_targets.py config/targets.sha1")
        checks.append(f"{out}.ok")
        lines.append("")
    regen = ["tools/configure.py", "config/targets.sha1"] + sorted(
        str(p.relative_to(ROOT)).replace("\\", "/")
        for p in list((ROOT / "config").rglob("*.yaml")) + list((ROOT / "config" / "symbols").glob("*.txt"))
        + list((ROOT / "config" / "functions").glob("*.txt")) + list((ROOT / "config" / "ld").glob("*.ld")))
    lines.append(f"build build.ninja: configure | {' '.join(regen)}")
    maps = [f"build/{t.module}.map" for t in targets]
    lines.append(f"build build/provenance.ok: provcheck {' '.join(maps)} | tools/provenance.py")
    c_objs = sorted(f"build/{p.parent.name}/src/{p.parent.name}/{p.stem}.o" for p in (ROOT / "src").glob("*/*.c"))
    lines.append(f"build build/plaincheck.ok: plaincheck {' '.join(c_objs)} | tools/plain_check.py")
    lines.append("build plaincheck: phony build/plaincheck.ok")
    lines.append("build provenance: provenance | build/provenance.ok")
    lines.append(f"build all: phony {' '.join(checks + expected)} build/provenance.ok build/plaincheck.ok")
    lines.append("rule progress")
    lines.append(f"  command = {tool('objdiff-cli')} report generate -p . -o $out --format json"
                 " && $python tools/progress.py $out")
    lines.append("  description = PROGRESS")
    lines.append("build build/report.json: progress | all objdiff.json")
    lines.append("build progress: phony build/report.json")
    lines.append("default all")
    ninja_file = ROOT / "build.ninja"
    _write_if_changed(ninja_file, "\n".join(lines) + "\n")
    # Always leave build.ninja newer than everything it was generated from,
    # or ninja keeps regenerating it (also guards against clock skew between
    # files edited on Windows and a WSL clock).
    newest = max([ninja_file.stat().st_mtime] + [(ROOT / p).stat().st_mtime for p in regen])
    stamp = max(newest, __import__("time").time()) + 1
    os.utime(ninja_file, (stamp, stamp))
    write_objdiff(targets, units)


def write_objdiff(targets: list[Target], units: list[dict]) -> None:
    """objdiff.json: C files are diffable units (target = original asm);
    assembly-only files are target-only units so totals cover all code."""
    import json
    config = {
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "custom_make": "ninja",
        "build_target": False,
        "build_base": True,
        "watch_patterns": ["*.c", "*.h", "*.s", "*.yaml", "*.txt"],
        "units": units,
        "progress_categories": [{"id": t.module, "name": t.name} for t in targets]
        + [{"id": k, "name": n} for k, n in KIND_CATEGORIES] + [{"id": PADDING, "name": PADDING}],
    }
    _write_if_changed(ROOT / "objdiff.json", json.dumps(config, indent=2) + "\n")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--split", action="store_true", help="re-run splat for every target")
    ap.add_argument("--split-only", action="store_true",
                    help="extract and split only (no build.ninja); works on Windows")
    args = ap.parse_args()
    if os.name == "nt" and not args.split_only:
        sys.exit("The matching build runs under Linux. On Windows, run it inside WSL (see README),\n"
                 "or use --split-only to extract and split only.")
    os.chdir(ROOT)
    targets = load_targets()

    if subprocess.run([sys.executable, str(TOOLS / "extract.py")]).returncode != 0:
        return 1
    write_extern_symbols()
    for t in targets:
        run_splat(t, args.split)
    if args.split_only:
        # the content too, for the tools that parse src/ with clang
        if subprocess.run([sys.executable, str(TOOLS / "content_gen.py")]).returncode != 0:
            return 1
        print("split done (asm/, build/orig/, build/gen/)")
        return 0
    write_link_symbols()
    write_ninja(targets)
    print("wrote build.ninja; now run: ninja")
    return 0


if __name__ == "__main__":
    sys.exit(main())
