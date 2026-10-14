#!/usr/bin/env python3
"""Verify that every tool installed by tools/setup.sh runs.

Run it from the project environment (tools/setup.sh runs it at the end):

    source tools/env.sh
    python tools/check_env.py [--markdown]

Exits non-zero if a required tool is missing or fails its check.
"""

from __future__ import annotations

import argparse
import importlib.metadata
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IS_WINDOWS = os.name == "nt"
PLATFORM = "windows-x64" if IS_WINDOWS else "linux-x64"
DEPS = ROOT / "tools" / "deps" / PLATFORM
EXE = ".exe" if IS_WINDOWS else ""


@dataclass
class Result:
    name: str
    ok: bool
    version: str
    detail: str = ""
    required: bool = True


def run(cmd: list[str], timeout: float = 120, **kw) -> subprocess.CompletedProcess:
    return subprocess.run(
        cmd, capture_output=True, text=True, errors="replace", timeout=timeout,
        stdin=subprocess.DEVNULL, **kw
    )


def first_line(text: str) -> str:
    for line in text.splitlines():
        if line.strip():
            return line.strip()
    return ""


def tool_version(name: str, exe: str, args: list[str], pattern: str | None = None,
                 required: bool = True) -> Result:
    path = shutil.which(exe)
    if not path:
        return Result(name, False, "", f"{exe} not found on PATH", required)
    try:
        p = run([path, *args])
    except (OSError, subprocess.TimeoutExpired) as e:
        return Result(name, False, "", f"failed to run: {e}", required)
    out = (p.stdout or "") + (p.stderr or "")
    if pattern:
        m = re.search(pattern, out)
        ver = m.group(1) if m else first_line(out)
    else:
        ver = first_line(out)
    ok = p.returncode == 0 or bool(pattern and re.search(pattern, out))
    return Result(name, ok, ver, "" if ok else f"exit {p.returncode}: {first_line(out)}", required)


def python_packages() -> list[Result]:
    results = []
    for dist in ["splat64", "spimdisasm", "rabbitizer", "pyelftools", "pycdlib",
                 "ninja"]:
        try:
            ver = importlib.metadata.version(dist)
        except importlib.metadata.PackageNotFoundError:
            results.append(Result(f"py:{dist}", False, "", "not installed"))
            continue
        # git-installed packages: report the pinned commit instead of the
        # placeholder version in their pyproject.
        direct = importlib.metadata.distribution(dist).read_text("direct_url.json")
        if direct:
            info = json.loads(direct).get("vcs_info", {})
            if info.get("commit_id"):
                ver = f"git {info['commit_id'][:12]}"
        results.append(Result(f"py:{dist}", True, ver))
    return results


# One instruction from each R5900 class the game code will contain.
R5900_TEST = """\
.set noreorder
.set noat
mult1   $a0, $a1
madd    $v0, $a0, $a1
lq      $t0, 0x10($sp)
sq      $t0, 0x20($sp)
adda.s  $f0, $f1
paddw   $t0, $t1, $t2
pextlw  $t0, $t1, $t2
pcpyld  $t0, $t1, $t2
qfsrv   $t0, $t1, $t2
lqc2    $vf1, 0x0($a0)
qmfc2   $t0, $vf1
vmulax.xyzw $ACC, $vf1, $vf2x
vmaddx.xyz  $vf4, $vf5, $vf6x
vcallms 0x100
"""
R5900_EXPECT = ["mult1", "madd", "lq", "sq", "adda.s", "paddw", "pextlw", "pcpyld",
                "qfsrv", "lqc2", "qmfc2", "vmulax.xyzw", "vmaddx.xyz", "vcallms"]


def binutils_r5900() -> Result:
    name = "binutils R5900 round-trip"
    as_ = shutil.which("mips-ps2-decompals-as")
    objdump = shutil.which("mips-ps2-decompals-objdump")
    if not as_ or not objdump:
        return Result(name, False, "", "mips-ps2-decompals-as/objdump not found")
    with tempfile.TemporaryDirectory() as td:
        src, obj = Path(td, "t.s"), Path(td, "t.o")
        src.write_text(R5900_TEST)
        p = run([as_, "-march=r5900", "-EL", "-mabi=eabi", "-o", str(obj), str(src)])
        if p.returncode != 0:
            return Result(name, False, "", f"as failed: {first_line(p.stderr)}")
        p = run([objdump, "-d", "-m", "mips:5900", str(obj)])
        mnemonics = re.findall(r"^\s+[0-9a-f]+:\s+[0-9a-f]{8}\s+(\S+)", p.stdout, re.M)
    if mnemonics != R5900_EXPECT:
        return Result(name, False, "", f"decoded {mnemonics}")
    return Result(name, True, f"{len(R5900_EXPECT)} insns (EE core, COP1, MMI, VU0 macro)")


MWCC_FLAGS = ["-c", "-nostdinc", "-O3,p", "-sdatathreshold", "0"]
MWCC_EXPECT = [0x03E00008, 0x00851021]   # jr ra; addu v0,a0,a1


def mwcc_check() -> Result:
    """Compile a one-line function with the matching compiler (via wibo) and
    check the exact output words."""
    name = "MWCC (matching compiler)"
    if IS_WINDOWS:
        return Result(name, False, "", "runs under Linux/WSL only (wibo)", False)
    wibo = shutil.which("wibo")
    exe = DEPS / "mwcc" / "mwccps2.exe"
    if not wibo or not exe.exists():
        return Result(name, False, "", "wibo or mwcc not installed")
    from elftools.elf.elffile import ELFFile
    with tempfile.TemporaryDirectory() as td:
        src, obj = Path(td, "t.c"), Path(td, "t.o")
        src.write_text("int mh2_check(int a, int b) { return a + b; }\n")
        p = run([wibo, str(exe), *MWCC_FLAGS, "-o", str(obj), str(src)],
                env=dict(os.environ, MWCIncludes="."))
        if p.returncode != 0 or not obj.exists():
            return Result(name, False, "", f"compile failed: {first_line(p.stdout + p.stderr)}")
        with open(obj, "rb") as f:
            text = next(s for s in ELFFile(f).iter_sections() if s.name == ".text").data()
    words = [int.from_bytes(text[i:i + 4], "little") for i in range(0, len(text), 4)]
    if words[:2] != MWCC_EXPECT:
        return Result(name, False, "", f"unexpected code {[hex(w) for w in words]}")
    stamp = DEPS / "mwcc" / ".mh2-installed"
    ver = stamp.read_text().strip() if stamp.exists() else "?"
    return Result(name, True, ver, "compiled test function matches")


def orig_files() -> Result:
    env = os.environ.get("MH2_ORIG_DIR")
    base = Path(env) if env else ROOT.parent
    if not base.is_dir():
        return Result("original disc files", False, str(base), "directory does not exist", False)
    top = list(base.iterdir())
    cnf = next((p for p in top if p.name.upper() == "SYSTEM.CNF"), None)
    # The ISO may sit next to the extracted files or one directory below.
    candidates = top + [c for d in top if d.is_dir() and d.name != ROOT.name for c in d.iterdir()]
    isos = [p.name for p in candidates if p.is_file() and p.suffix.lower() == ".iso"]
    src = "MH2_ORIG_DIR" if env else "default: parent of repo"
    if cnf or isos:
        what = ", ".join(filter(None, ["extracted files (SYSTEM.CNF)" if cnf else "",
                                        f"ISO: {', '.join(isos)}" if isos else ""]))
        return Result("original disc files", True, str(base), f"{src}; found {what}", False)
    return Result("original disc files", False, str(base),
                  f"{src}; no SYSTEM.CNF or .iso found (set MH2_ORIG_DIR)", False)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--markdown", action="store_true", help="print a Markdown table")
    args = ap.parse_args()

    r: list[Result] = []
    r.append(Result("host", True, f"{platform.system()} {platform.release()} {platform.machine()}"))
    r.append(Result("python", sys.prefix != sys.base_prefix, platform.python_version(),
                    sys.executable if sys.prefix != sys.base_prefix else "not running in the project venv"))
    r.append(tool_version("uv", "uv", ["--version"], r"uv (\S+)"))
    r.append(tool_version("git", "git", ["--version"], r"git version (\S+)"))
    r += python_packages()
    r.append(tool_version("splat", "splat", ["--version"], r"(\d+\.\d+\.\d+)"))
    r.append(tool_version("spimdisasm", "spimdisasm", ["--version"], r"(\d+\.\d+\.\d+)"))
    r.append(tool_version("ninja", "ninja", ["--version"], r"(\d+\.\d+\.\d+)"))
    r.append(tool_version("binutils as", "mips-ps2-decompals-as", ["--version"], r"GNU assembler.*?(\d+\.\d+\S*)"))
    r.append(tool_version("binutils ld", "mips-ps2-decompals-ld", ["--version"], r"GNU ld.*?(\d+\.\d+\S*)"))
    r.append(binutils_r5900())
    r.append(mwcc_check())
    r.append(tool_version("objdiff-cli", "objdiff-cli", ["--version"], r"(\d+\.\d+\.\d+)"))
    gui = shutil.which("objdiff")
    r.append(Result("objdiff (GUI)", bool(gui), "present" if gui else "", "" if gui else "not found"))
    # clang: tools/content_gen.py reads the C files' types with it
    r.append(tool_version("clang", "clang", ["--version"], r"clang version (\S+)", True))
    r.append(orig_files())

    failed = [x for x in r if not x.ok and x.required]
    if args.markdown:
        print("| Tool | Version | Status | Detail |")
        print("|---|---|---|---|")
        for x in r:
            status = "ok" if x.ok else ("FAIL" if x.required else "skipped/optional")
            print(f"| {x.name} | {x.version} | {status} | {x.detail} |")
    else:
        for x in r:
            status = "ok  " if x.ok else ("FAIL" if x.required else "--  ")
            line = f"[{status}] {x.name:28} {x.version}"
            if x.detail:
                line += f"   ({x.detail})"
            print(line)
    if failed:
        print(f"\n{len(failed)} required check(s) failed: {', '.join(x.name for x in failed)}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
