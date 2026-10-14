#!/usr/bin/env python3
"""Aligned diff of one compiled function against the original.

    python tools/adiff.py MODULE FILE.c FUNC [-q] [-c N] [-D NAME]

func_diff.py compares word by word, so one extra or missing instruction
makes every later word differ and "N of M words differ" says little about
how close a draft is. This lines the two disassemblies up first (difflib),
so an inserted instruction shows as one line and the rest match again.

Marks: '!' differs, '~' the same except a relocated operand, '>' the same
branch except its target (shifted by an insertion elsewhere). The summary
line is `FUNC mn=M full=F orig=O ours=N`: M counts differences in the
mnemonic sequence alone (registers and constants ignored), F the differing
aligned instructions, O and N the lengths. Rank near drafts by F, then M.
Exit status 0 only on a full match.

-q prints only the summary; -c N shows N lines of context around each
difference; -D defines a macro (-D NON_MATCHING for a function under
#ifdef NON_MATCHING). The file's `#pragma optimization_level` applies,
as in func_diff.py. Needs MWCC, so it runs where the matching build does.
Written by the round 6 game core worker (its scratch triage tool).
"""

import argparse
import bisect
import difflib
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import func_diff as fd  # noqa: E402

BRANCH = re.compile(r"^(b|beq|bne|beqz|bnez|bgez|bgtz|blez|bltz|bc1t|bc1f|bltzal|bgezal|beql|bnel|bc1tl|bc1fl|j|jal)$")


def compile_obj(cfile, defines=()):
    lines = Path(cfile).read_text(encoding="utf-8").splitlines()
    text = "".join(f"#define {d}\n" for d in defines)
    text += "\n".join("" if fd.INCLUDE_ASM.match(l) else l for l in lines) + "\n"
    (ROOT / "build").mkdir(exist_ok=True)
    td = Path(tempfile.mkdtemp(prefix="adiff-", dir=ROOT / "build"))
    try:
        src = td / Path(cfile).name
        src.write_bytes(text.encode("shift_jis"))
        obj = td / "out.o"
        p = subprocess.run([str(fd.WIBO), str(fd.MWCC), *fd.CFLAGS, "-o", str(obj), str(src)], cwd=ROOT,
                           env=dict(os.environ, MWCIncludes="."), capture_output=True, text=True)
        msg = (p.stdout + p.stderr).strip()
        res = fd.read_object(obj) if p.returncode == 0 else None
    finally:
        for f in td.iterdir():
            f.unlink()
        td.rmdir()
    return res, msg


def norm(ins):
    return re.sub(r"\s+", " ", ins.strip())


def mnem(ins):
    return ins.split(" ")[0] if ins else ""


def loose(ins):
    """The instruction with its numbers blanked, for aligning."""
    m = mnem(ins)
    ops = ins[len(m):].strip()
    ops = re.sub(r"(?<![\w$])(-?0x[0-9a-f]+|-?\d+)(?![\w])", "#", ops)
    return m + " " + ops


def orig_body(module, func):
    image, base = fd.module_image(module)
    syms = fd.load_symbols(module)
    starts = fd.function_starts(module)
    addr = fd.sym_addr(func, syms)
    if addr is None:
        sys.exit(f"{func}: no address (not in the symbol files, not func_XXXXXXXX)")
    k = bisect.bisect_right(starts, addr)
    end = starts[k] if k < len(starts) else addr + 0x10000
    orig = image[addr - base:end - base]
    n = len(orig) // 4
    full = n
    while n > 0 and not any(orig[4 * (n - 1):4 * n]):     # trailing padding
        n -= 1
    if 0 < n < full:
        # keep the delay slot of a final jump or branch
        w = int.from_bytes(orig[4 * (n - 1):4 * n], "little")
        op = w >> 26
        if op in (1, 2, 3, 4, 5, 6, 7) or (op == 0 and (w & 0x3F) in (8, 9)):
            n += 1
    return addr, orig[:4 * n]


def diff_lists(do, dn, rel_idx):
    ko = [loose(x) for x in do]
    kn = [loose(x) for x in dn]
    sm = difflib.SequenceMatcher(None, ko, kn, autojunk=False)
    lines, full = [], 0
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op == "equal":
            for t in range(i2 - i1):
                o, n, j = do[i1 + t], dn[j1 + t], j1 + t
                if o == n:
                    mark = " "
                elif j in rel_idx:
                    mark = "~"
                elif BRANCH.match(mnem(o)):
                    mark = ">"
                else:
                    mark = "!"
                    full += 1
                lines.append((mark, o, n))
            continue
        for t in range(max(i2 - i1, j2 - j1)):
            o = do[i1 + t] if i1 + t < i2 else ""
            n = dn[j1 + t] if j1 + t < j2 else ""
            full += 1
            lines.append(("!", o, n))
    smm = difflib.SequenceMatcher(None, [mnem(x) for x in do], [mnem(x) for x in dn], autojunk=False)
    mdiff = sum(max(i2 - i1, j2 - j1) for op, i1, i2, j1, j2 in smm.get_opcodes() if op != "equal")
    return lines, full, mdiff


def run(module, cfile, func, defines=()):
    """((lines, full, mdiff, orig words, our words), compiler messages), or
    (None, reason) when the file doesn't compile or lacks the function."""
    res, msg = compile_obj(cfile, defines)
    if res is None:
        return None, msg
    funcs, _other, _objects = res
    for name, body, relocs in funcs:
        if name == func:
            break
    else:
        return None, f"{func} is not in the compiled object"
    addr, orig = orig_body(module, func)
    do = [norm(x) for x in fd.disasm(orig, addr)]
    dn = [norm(x) for x in fd.disasm(body, addr)]
    rel_idx = {off // 4 for off, _t, _s in relocs}
    lines, full, mdiff = diff_lists(do, dn, rel_idx)
    return (lines, full, mdiff, len(do), len(dn)), msg


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("module")
    ap.add_argument("cfile")
    ap.add_argument("func")
    ap.add_argument("-q", action="store_true", help="only the summary line")
    ap.add_argument("-c", type=int, default=-1, help="lines of context around differences")
    ap.add_argument("-D", dest="defines", action="append", default=[])
    a = ap.parse_args()
    r, msg = run(a.module, a.cfile, a.func, a.defines)
    if r is None:
        print(msg)
        print(f"{a.func} COMPILE-ERROR")
        return 2
    lines, full, mdiff, no, nn = r
    if not a.q:
        show = set(range(len(lines)))
        if a.c >= 0:
            show = {j for i, l in enumerate(lines) if l[0] == "!" for j in range(i - a.c, i + a.c + 1)}
        last = -2
        for i, (mark, o, n) in enumerate(lines):
            if i not in show:
                continue
            if i != last + 1:
                print("  ...")
            last = i
            print(f"  {mark} {o:44.44} | {n}")
    print(f"{a.func} mn={mdiff} full={full} orig={no} ours={nn}")
    return 0 if full == 0 and no == nn else 1


if __name__ == "__main__":
    sys.exit(main())
