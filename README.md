# Monster Hunter 2 (Dos) decompilation

A research and preservation project: a matching decompilation of
**Monster Hunter 2 (Dos)** for the PlayStation 2 (NTSC-J, version 1.04,
`SLPM_662.80`; Capcom, 2006). Capcom's code is reconstructed as C that the
game's original compiler turns back into the original's exact bytes, and
the build reproduces the boot executable and all 12 code overlays byte for
byte.

The code matches, but much of it doesn't read well yet. Readability work
was deliberately left until the whole build matched, so that every cleanup
can be checked against it. That work is underway, and any assistance is welcome
(see [Contributing](#contributing)).

The project exists to preserve how the game works, in a form people can
read, and to show how a large PS2 title was made: how a 2006 CodeWarrior
codebase of over 12,500 functions was written, compiled and laid out.

**This repository contains no game data.** It holds the code and the
structure of the game's data (types, names, sizes). Everything else comes
from your own copy of the game when you build.

## Status

The rows of `ninja progress`:

| Part | What it is | Functions | Code | Matched | Built from |
|---|---|---|---|---|---|
| `game` | Capcom's code | 12,551 | 4.57 MB | 100% | the repository's C and assembly |
| `vu1` | Capcom's VU1 microcode | 60 programs | 107 KB | 100% | the repository's VU1 assembly |
| `access` | ACCESS's kana-kanji engine | 262 | 48 KB | not decompiled | assembly from your copy |
| `sdk` | Sony's SDK, the toolchain's runtime, the network libraries | 2,530 | 1.17 MB | not decompiled | assembly from your copy; DNAS as a binary copy |
| `middleware` | CRI's Sofdec and ADX | 1,982 | 288 KB | not decompiled | assembly from your copy |

- **Complete.** All 12,551 of Capcom's functions are source and match; no
  `NON_MATCHING` block is left.
  - 12,497 are C.
  - 42 are CodeWarrior `asm` functions in their C files
    (`src/sub_main/unk_2E3940.c` and `unk_2F17C0.c`): VU0 code, which MWCC
    never emits from C ([docs/mwcc-matching.md](docs/mwcc-matching.md),
    "What can't come from C").
  - 12 are main's hand-written copy and fill loops
    ([src/main/mem_loops.s](src/main/mem_loops.s)), which no C produces.
  - The VU1 microcode (60 programs) is VU1 assembly
    ([src/sub_main/vu1_microcode.vsm](src/sub_main/vu1_microcode.vsm)),
    built by the project's own assembler, `tools/vu1as.py`.
- **Byte for byte.** The build reproduces the boot executable and all 12
  overlays; `ninja` checks all 13 against
  [config/targets.sha1](config/targets.sha1). Four of the overlays hold no
  Capcom code: `dnas_net` and `dnas_ins` (Sony's DNAS2 library, copied from
  your disc as is), `lbguild` (data only) and `test` (a 256-byte all-zero
  stub).
- **Counted apart.** Code that Capcom licensed rather than wrote has rows
  of its own and is not decompiled: Sony's EE libraries (with the
  toolchain's C library, libm, libgcc and CodeWarrior's runtime), CRI's
  middleware, the network libraries (nethttp's HTTPS and OpenSSL-derived
  code, and DNAS) and ACCESS's kana-kanji engine in sub_main. The build
  takes it from your copy: splat turns it into assembly, except DNAS,
  whose bytes are copied as they are. Its names are in
  [config/symbols/](config/symbols/). Over the whole program, 12,552 of
  17,326 functions (75.6% of 6.19 MB of code) are source.
- **Data and text** are described in C, not stored in it: each object's
  type, name and size are in the source, and its values and the game's
  texts come from your copy at build time
  ([CONTRIBUTING.md](CONTRIBUTING.md), "Content").
- **Readability.** 12,400 of the 12,551 functions have real names (the
  `named` column of `ninja progress`). What is left to name and tidy is
  under [Contributing](#contributing).

## Your copy of the game

The build reads your own copy of the disc: either the `.iso` or the
extracted disc files. Only version 1.04 is supported; other releases and
reprints may have different executables.

By default, the tools look for your files in the **parent directory of the
repository**: the disc files directly there (`SYSTEM.CNF`,
`SLPM_662.80`, ...), or the `.iso` there or one folder below. To use
another place, set `MH2_ORIG_DIR`:

```sh
export MH2_ORIG_DIR=/path/to/mh2
```

The tools treat your files as read-only: they never modify, move or delete
them. `python tools/verify_originals.py` checks your copy against the
known-good hashes of version 1.04 (the Redump dump) in
[config/disc.sha1](config/disc.sha1).

`asm/`, `assets/` and everything under `build/` (including `build/gen/`,
the data values and texts) are generated from your copy and gitignored.
They are for your own use: don't share them.

## Building

The matching build runs on **Linux** x86-64 (tested on Ubuntu 24.04); on
Windows, it runs inside **WSL 2**. Everything is installed into the
repository's `tools/deps/` (gitignored) at pinned versions:
[tools/versions.txt](tools/versions.txt), whose downloads are checked
against their SHA-256, and [uv.lock](uv.lock) for Python. That takes about
400 MB, and a build about 210 MB more.

### Linux

You need `git`, and `sudo` on Debian or Ubuntu, where the setup script
installs `ca-certificates`, `curl`, `git`, `tar` and `clang` with apt
(clang reads the C files' types for `tools/content_gen.py`). On another
distribution, install those yourself and run `tools/setup.sh --no-apt`.

```sh
git clone <repository URL> mh2-decomp
cd mh2-decomp
tools/setup.sh                   # once: the pinned toolchain, into tools/deps/
source tools/env.sh              # in every new shell
export MH2_ORIG_DIR=/path/to/mh2 # unless your copy is in the parent directory
python tools/verify_originals.py # optional: check your copy
python tools/configure.py        # extract the targets from your copy, split them, write build.ninja
ninja                            # generate the content, compile, assemble, link, check the 13 targets
ninja progress                   # matched code per module and by origin
```

`ninja` ends with one line per target:

```
OK    SLPM_662.80
OK    select.bin
...
OK    sub_main.bin
```

**`OK` for all 13** means that each built file's SHA-1 equals the game's
([config/targets.sha1](config/targets.sha1)): your build is
byte-identical to the original. The first `configure.py` takes about a
minute and a half, and a full build a little over a minute with 8 jobs.

The setup script keeps its downloads in `tools/deps/downloads/` and
fetches only what is missing there, so a second checkout can reuse a first
one's: `mkdir -p tools/deps && ln -s /path/to/first/tools/deps/downloads
tools/deps/downloads` before `tools/setup.sh`.

### Windows

1. Install WSL 2 with Ubuntu 24.04 (Microsoft's WSL documentation
   describes how), then open an Ubuntu shell.
2. In Ubuntu, clone the repository into the Linux file system (for
   example `~/mh2-decomp`): the build is much faster there than under
   `/mnt/c`.
3. Follow the Linux steps. Your disc files can stay on Windows: for a copy
   in `D:\games\mh2`, `export MH2_ORIG_DIR=/mnt/d/games/mh2`.

The game's compiler is a Windows program; wibo runs it on Linux, and
there is no native Windows build.

## Project layout

| Path | What it is |
|---|---|
| `src/<module>/` | Capcom's code: C, one file per translation unit; main's hand-written assembly; sub_main's VU1 microcode |
| `include/` | headers: the game's types, and the `*.spec` files some structs are generated from |
| `config/` | splat configs (`main.yaml`, `overlays/<module>.yaml`), names (`symbols/`), function starts (`functions/`), linker aliases (`ld/`), described assets (`assets/`), the reviewed exceptions of the content check (`content_exceptions.txt`), the known-good hashes (`disc.sha1`, `targets.sha1`) |
| `tools/` | setup, build, verification and diffing scripts ([tools/README.md](tools/README.md)) |
| `docs/` | the MWCC matching reference ([docs/mwcc-matching.md](docs/mwcc-matching.md)), notes on hard matches (`docs/notes/nm/`), how the project was made ([docs/ai-workflow/](docs/ai-workflow/README.md)) |
| `asm/`, `assets/`, `build/` | generated from your copy (gitignored) |

`main` is the boot executable, `SLPM_662.80`; the other modules are the
code overlays stored in the disc's `DATA.BIN`.

## How it matches

- **The compiler** is Metrowerks CodeWarrior for PS2, compiler build
  `3.0b52-030722`, with the flags `-O3,p -sdatathreshold 0 -str readonly`
  (and `#pragma optimization_level` where the game built a file or a
  function at another level). It was chosen by compiling candidates and
  comparing their output with the game's code. The setup script downloads
  it from decomp.me's compiler archive, and the build runs it through
  wibo.
- **splat** splits the boot executable and the overlays by the YAML files
  in `config/`, with spimdisasm and rabbitizer disassembling the R5900
  code. That is where the libraries' assembly comes from.
- **`tools/content_gen.py`** writes the data values and texts the C
  includes, from your copy, with the types clang reads from the C.
- **`tools/mwcc_cc.py`** compiles each C file, converted from UTF-8 to
  Shift-JIS (the game's encoding). binutils, built for the PS2's R5900,
  assembles and links, and `tools/check_targets.py` compares the results
  with the game's hashes.
- **objdiff** diffs each unit against the original, function by function,
  and makes the progress report; `tools/func_diff.py` checks one file's
  functions in about a second.
- [docs/mwcc-matching.md](docs/mwcc-matching.md) is the project's
  reference on which C gives which instructions with this compiler: its
  register allocator, its scheduler, and a catalogue of patterns.

The project stands on these tools:

| Tool | By | Used for |
|---|---|---|
| [splat](https://github.com/ethteck/splat) | ethteck | splitting the executables |
| [spimdisasm](https://github.com/Decompollaborate/spimdisasm), [rabbitizer](https://github.com/Decompollaborate/rabbitizer) | Decompollaborate | disassembly |
| [binutils-mips-ps2-decompals](https://github.com/decompals/binutils-mips-ps2-decompals) | decompals | assembling and linking for the R5900 |
| [wibo](https://github.com/decompals/wibo) | decompals | running the Windows compiler on Linux |
| [decomp.me's compiler archive](https://github.com/decompme/compilers) | decompme | the MWCC download |
| [objdiff](https://github.com/encounter/objdiff) | encounter | diffs and progress reports |
| [decomp-permuter](https://github.com/simonlindholm/decomp-permuter) | simonlindholm | finding C variants that match (settings in `tools/`) |
| [m2c](https://github.com/matt-kempster/m2c) | matt-kempster | the first drafts of the C |
| [uv](https://github.com/astral-sh/uv) | astral-sh | the pinned Python environment |
| [ninja](https://github.com/ninja-build/ninja) | ninja-build | running the build |
| [clang](https://github.com/llvm/llvm-project) | llvm | reading the C's types for the content generator |
| [pyelftools](https://github.com/eliben/pyelftools), [pycdlib](https://github.com/clalancette/pycdlib) | eliben; clalancette | reading ELF files and the disc image |

## Contributing

The code matches; what's open is making it read well, and anyone can
help:
- name the `unk` fields in `include/` (and those in structs local to one
  file) and the data objects still named by address;
- give the small groups of raw byte-offset accesses their struct fields;
- replace a fakematch (`git grep FAKEMATCH`) with a natural form that
  matches.

Every change must keep `ninja` at `OK` for all 13 targets and
`tools/proto_check.py` at 0 disagreements, and add no game content
(`python tools/content_scan.py`: 0 findings).
[CONTRIBUTING.md](CONTRIBUTING.md) has the checks, the conventions, the
tools and the counts of what is left.

## Made with AI

This project was completed almost entirely autonomously by Claude,
Anthropic's AI model (Claude Opus 5.5), working in Claude Code from
instructions on how to build, compile and verify the byte matching. The
project's owner, Batenkaitos, orchestrated the work, tested it, and made
the decisions when important questions came up.

- **What the build guarantees:** every matched function compiles to the
  original's exact bytes ([Status](#status)).
- **What it doesn't:** names, types, comments and explanations are the
  AI's reading of the code, and can be wrong. Corrections are welcome.

More in [docs/ai-workflow/](docs/ai-workflow/README.md).

## Legal

- This is an unofficial project, not affiliated with or endorsed by Capcom
  or Sony Interactive Entertainment. Monster Hunter is a trademark of
  Capcom Co., Ltd., and PlayStation is a trademark of Sony Interactive
  Entertainment Inc.; all trademarks belong to their owners.
- The repository contains no game data, executables, assets, text or SDK
  files. Building it requires your own legally obtained copy of the game.
- The game links zlib 1.1.4. Its code here, in `src/lobby/`
  (`unk_7513C0.c`, `unk_753B50.c`, `unk_753C10.c` and `unk_756FD0.c`) with
  its header `include/zlib.h`, keeps zlib's own licence notice.
