# Monster Hunter 2 (Dos) decompilation

A research and preservation project: a matching decompilation of
**Monster Hunter 2 (Dos)** for the PlayStation 2 (NTSC-J, version 1.04,
`SLPM_662.80`; Capcom, 2006). Capcom's code is reconstructed as C that the
game's original compiler turns back into the original's exact bytes: the
build reproduces the boot executable and all 12 code overlays byte for
byte.

The project exists to preserve how the game works, in a form people can
read and study, and to study the code of a large PS2 title: how a 2006
CodeWarrior codebase of over 12,500 functions was written, compiled and
laid out.

**This repository contains no game data.** It holds the code and the
structure of the game's data (types, names, sizes). Everything else comes
from your own copy of the game when you build.

## Status

| `ninja progress` row | What it is | Functions | Code | Status |
|---|---|---|---|---|
| `game` | Capcom's code | 12,551 | 4.57 MB | 100% matched |
| `vu1` | Capcom's VU1 microcode | 60 programs | 107 KB | 100% matched, as VU1 assembly |
| `access` | ACCESS's kana-kanji engine | 262 | 48 KB | assembly from your copy |
| `sdk` | Sony's SDK, the toolchain's runtime, the network libraries | 2,530 | 1.17 MB | assembly from your copy |
| `middleware` | CRI's Sofdec and ADX | 1,982 | 288 KB | assembly from your copy |

- **Complete.** All 12,551 of Capcom's functions are source and match:
  12,539 as C, and main's 12 memory copy and fill loops as Capcom's
  hand-written assembly ([src/main/mem_loops.s](src/main/mem_loops.s)),
  which no C produces. No `NON_MATCHING` block is left. Capcom's VU1
  microcode is source as VU1 assembly
  ([src/sub_main/vu1_microcode.vsm](src/sub_main/vu1_microcode.vsm)),
  built by the project's own assembler (tools/vu1as.py). The boot
  executable and the 12 code overlays build byte for byte: `ninja` checks
  all 13 against [config/targets.sha1](config/targets.sha1).
- **Counted apart.** Code that Capcom licensed rather than wrote has rows
  of its own and stays assembly, which splat generates from your copy at
  build time: Sony's EE libraries (with the toolchain's C library, libm,
  libgcc and CodeWarrior's runtime), CRI's middleware, the network
  libraries (nethttp's HTTPS and OpenSSL-derived code, DNAS), and ACCESS's
  kana-kanji conversion engine in sub_main. It is not decompiled; its
  names are in [config/symbols/](config/symbols/). Over the whole
  program, 12,552 of 17,326 functions, 75.6% of 6.19 MB of code, are
  source.
- **Data and text.** Capcom's data and the game's text are described in
  C, not stored in it. Every data object has its type, name and size in
  the C; its values come from your copy at build time
  (tools/content_gen.py writes them into `build/gen/`). A text the game
  shows is a `TXT(0xADDR)` reference with a comment saying what it is. All
  1,706.7 KiB of Capcom's initialized data is declared in C this way
  (`python tools/data_progress.py`), and `python tools/content_scan.py`
  checks that the source holds none of the game's data values or text.
- **Readability.** 12,400 of the 12,551 game functions have real names
  (the `named` column of `ninja progress`). Every function declaration
  agrees with its definition (`tools/proto_check.py`: 0 disagreements),
  except 431 that are marked as the game's own declarations, which the
  matching code needs. Still to do: about 1,200 struct fields in
  `include/` named by their offset (`unk1C`), and more in structs local
  to one file; data objects still named by their address
  (`D_00XXXXXX`); small groups of field accesses through raw byte
  offsets; and 33 fakematches, marked `FAKEMATCH`
  ([CONTRIBUTING.md](CONTRIBUTING.md)).

## You need your own copy of the game

The repository holds no executable, disc image, extracted file, asset,
data value or game text, and nothing derived from them. The build reads
the files of your own copy of the disc, either the `.iso` or the
extracted disc files. Only version 1.04 is supported: other releases and
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

You need `git`, and on Debian or Ubuntu `sudo`: the setup script installs
`ca-certificates`, `curl`, `git`, `tar` and `clang` with apt (clang reads
the C files' types for tools/content_gen.py). On another distribution,
install those yourself and run `tools/setup.sh --no-apt`.

```sh
git clone https://github.com/Batenkaitos-dev/mh2-decomp.git
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

**`OK` for all 13** (the boot executable and the 12 overlays: `select`,
`lobby`, `nethttp`, `dnas_net`, `gm_sub`, `dnas_ins`, `yn`, `lbguild`,
`test`, `plsel`, `game`, `sub_main`) means that the SHA-1 of every built
file equals the game's, in [config/targets.sha1](config/targets.sha1): your
build is byte-identical to the original. The first `configure.py` takes
about a minute and a half, and a full build a little over a minute with
8 jobs.

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

The game's compiler is a Windows program, but the build runs it through
wibo on Linux, so the build doesn't run natively on Windows.

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
  `3.0b52-030722`, chosen by compiling candidates and comparing their
  output with the game's code, with the flags `-O3,p -sdatathreshold 0
  -str readonly` (and `#pragma optimization_level` where the game built a
  file or a function at another level). It is a Windows program: the build
  runs it on Linux through wibo, and the setup script downloads it from
  decomp.me's compiler archive.
- **splat** splits the boot executable and the overlays by the YAML files
  in `config/`, with spimdisasm and rabbitizer disassembling the R5900
  code; that is where the libraries' assembly comes from.
  tools/content_gen.py writes the data values and texts the C includes,
  from your copy, with the types clang reads from the C. tools/mwcc_cc.py
  compiles each C file (converted from UTF-8 to Shift-JIS, the game's
  encoding), and binutils, built for the PS2's R5900, assembles and links.
  tools/check_targets.py compares the results with the game's hashes.
- **objdiff** diffs each unit against the original, function by function,
  and makes the progress report; tools/func_diff.py checks one file's
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
`tools/proto_check.py` at 0 disagreements, and must add no game content:
no game data, data values or game text in the source.
`python tools/content_scan.py` must report 0 findings (CONTRIBUTING.md,
"Content"). [CONTRIBUTING.md](CONTRIBUTING.md) has the checks, the
conventions and the tools.

## A PC port

A native PC port of the game, built from this decompilation's C, is in
development as a separate project, not yet published. Mods for PC will be
made through it: a mod is a change to this C, compiled natively by the
port, with this byte-matching build as the reference every change is
checked against. The port takes the game's content from your own copy
too; nothing of the game ships with it.

## Made with AI

This project was completed almost entirely autonomously by Claude,
Anthropic's AI model (Claude Opus 5.5), working in Claude Code from
instructions on how to build, compile and verify the byte matching. The
project's owner, Batenkaitos, orchestrated the work, tested it, and made
the decisions when important questions came up.

- **What the build guarantees:** every matched function compiles to the
  original's exact bytes, and the build reproduces the game's executables
  byte for byte.
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
