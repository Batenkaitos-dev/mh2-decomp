# Monster Hunter 2 (Dos) decompilation

A research and preservation project: a matching decompilation of
**Monster Hunter 2 (Dos)** for the PlayStation 2 (NTSC-J, version 1.04,
`SLPM_662.80`; Capcom, 2006).

- **Capcom's code is decompiled and matches.** All 12,532 of its functions
  are built from this repository's source to the original's exact bytes:
  12,520 as C, compiled by the game's original compiler, and 12 as
  Capcom's own hand-written assembly. The build reproduces the boot
  executable and all 12 code overlays byte for byte.
- **The libraries Capcom licensed are not decompiled:** Sony's SDK, CRI's
  middleware, the network libraries and ACCESS's kana-kanji engine. The
  build takes them from your copy of the game.
- **DNAS, and the values of the game's data and texts, come from your
  copy** at build time.

The table under [Status](#status) gives the bytes of each.

The code matches, and it has had a full readability pass. That work was
deliberately left until the whole build matched, so that every cleanup
could be checked against it: the functions, data and fields are named and
typed, each C file has its header, and the data's tables have their own
types. Corrections and further cleanup are welcome (see
[Contributing](#contributing)).

The project exists to preserve how the game works, in a form people can
read, and to show how a large PS2 title was made: how a 2006 CodeWarrior
codebase of over 12,500 functions was written, compiled and laid out.

**This repository contains no game data.** It holds the code and the
structure of the game's data (types, names, sizes). Everything else comes
from your own copy of the game when you build.

## Status

Where each byte of the 13 built files comes from (`ninja provenance`, from
the link maps):

| Origin | Bytes | Share | From your copy? |
|---|---:|---:|---|
| Capcom's code, compiled from C | 4,633,096 | 53.7% | no |
| zlib 1.1.4 (third-party), compiled from its decompiled C | 20,032 | 0.2% | no |
| Data the compiler makes from the C: tables written in C, literals, jump tables | 203,980 | 2.4% | no |
| Assembly from the repository's source: main's copy loops, the VU1 microcode | 107,568 | 1.2% | no |
| Capcom's data, described in C, with its values from your copy | 1,654,752 | 19.2% | by policy: the game's content |
| Third-party libraries, code and data | 1,134,560 | 13.2% | yes: not decompiled |
| DNAS: its two overlays and its authentication data | 862,352 | 10.0% | yes |
| Headers and zero padding | 3,868 | 0.0% | no: the link could write them |
| **Placed by the link** (`.bss` aside) | **8,620,208** | 100% | |

The first four rows, 4,964,676 bytes (57.6%), are built from the
repository's source. The rest of each file is the linker's alignment fill.

- **Complete.** All 12,532 of Capcom's functions are source and match (the
  `game` row of `ninja progress`); no `NON_MATCHING` block is left.
  - 12,478 are C.
  - 42 are CodeWarrior `asm` functions in their C files
    (`src/sub_main/engine_math.c`, `anim_curve.c` and `vu1_packets.c`): VU0
    code, which MWCC never emits from C
    ([docs/mwcc-matching.md](docs/mwcc-matching.md), "What can't come from
    C").
  - 12 are main's hand-written copy and fill loops
    ([src/main/mem_loops.s](src/main/mem_loops.s)), which no C produces.
  - The VU1 microcode (60 programs) is VU1 assembly
    ([src/sub_main/vu1_microcode.vsm](src/sub_main/vu1_microcode.vsm)),
    built by the project's own assembler, `tools/vu1as.py`.
    `config/overlays/sub_main.yaml` lists the block as `textbin`: that is
    splat's copy from your disc, which objdiff compares against; the build
    assembles the source in its place.
- **zlib 1.1.4**, which the lobby links (inflate and adler32), is
  decompiled too and counted apart (the `zlib` row): its 19 functions
  match, in `src/lobby/adler32.c`, `infblock.c`, `inflate.c` and
  `infutil.c`.
- **Byte for byte.** The build reproduces the boot executable and all 12
  overlays; `ninja` checks all 13 against
  [config/targets.sha1](config/targets.sha1). Four of the overlays hold no
  Capcom code: `dnas_net` and `dnas_ins` (Sony's DNAS2 library, copied from
  your disc as is), `lbguild` (data only) and `test` (a 256-byte all-zero
  stub).
- **Checked by every build.** `ninja` also compiles each C file a second
  time with plain MWCC, with none of the build's wrapper, and compares
  every function with the build's (`tools/plain_check.py`: 12,539 of 12,539
  identical, Capcom's and zlib's), and fails if any code comes from
  anything but `src/`, the repository's assembly or a labelled library
  piece (`tools/provenance.py --check`).
- **Counted apart.** Code that Capcom licensed rather than wrote has rows
  of its own in `ninja progress` and is not decompiled: Sony's EE libraries
  (with the toolchain's C library, libm, libgcc and CodeWarrior's runtime),
  CRI's middleware, the network libraries (nethttp's HTTPS and
  OpenSSL-derived code, and DNAS) and ACCESS's kana-kanji engine in
  sub_main. The build takes it from your copy: splat turns it into
  assembly, except DNAS, whose bytes are copied as they are. Its names are
  in [config/symbols/](config/symbols/). Over the whole program, 12,552 of
  17,326 functions (75.6% of 6.19 MB of code) are source: Capcom's 12,532,
  zlib's 19, and the VU1 microcode, which `ninja progress` counts as one.
- **Data and text** are described in C, not stored in it: each object's
  type, name and size are in the source, and its values and the game's
  texts come from your copy at build time
  ([CONTRIBUTING.md](CONTRIBUTING.md), "Content"). The one object kept as
  a copy of its bytes is DNAS's authentication data (64 KB in sub_main, an
  `INCLUDE_DATA` line in `src/sub_main/dnas_check.c`).
- **Readability.** All 12,551 functions, Capcom's and zlib's, have real
  names (the `named` column of `ninja progress`). What is left to name and
  tidy (fields and data objects) is under [Contributing](#contributing).

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
ninja provenance                 # where each byte of the 13 files comes from
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

### A readable copy, for your own use

After `configure.py`, `python tools/export_readable.py` writes
`build/readable/`, a copy of the source with your copy's data tables and
texts written out in C (each struct member by its field name). It builds
the same way, byte-identical until you edit it. It holds the game's
content: keep it on your machine, and never commit or share it
([CONTRIBUTING.md](CONTRIBUTING.md), "The readable export").

## Project layout

| Path | What it is |
|---|---|
| `src/<module>/` | Capcom's code: C, one file per translation unit; main's hand-written assembly; sub_main's VU1 microcode |
| `include/` | headers: `common.h`, one for each C file (or its subject), one for each library's functions, and the `*.spec` files some structs are generated from |
| `config/` | splat configs (`main.yaml`, `overlays/<module>.yaml`), names (`symbols/`), function starts (`functions/`), linker aliases (`ld/`), described assets (`assets/`), the reviewed exceptions of the content check (`content_exceptions.txt`), the known-good hashes (`disc.sha1`, `targets.sha1`) |
| `tools/` | setup, build, verification and diffing scripts ([tools/README.md](tools/README.md)) |
| `docs/` | the MWCC matching reference ([docs/mwcc-matching.md](docs/mwcc-matching.md)), notes on hard matches (`docs/notes/hard-matches/`), how the project was made ([docs/ai-workflow/](docs/ai-workflow/README.md)) |
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
  Shift-JIS (the game's encoding). It changes how MWCC's output is laid
  out, never its code: it removes its own marker variables, makes static
  functions' symbols global, keeps every data object in a section of its
  own with its alignment raised to at least 8 (the game's), moves the
  variables between `#pragma rodata on` and `off` to `.rodata`, splices in
  the one copied object (`INCLUDE_DATA`), and drops the `Stripped_`
  functions, stand-ins for code the game's linker removed.
  `tools/plain_check.py` shows that no code byte changes. binutils, built
  for the PS2's R5900, assembles and links, and `tools/check_targets.py`
  compares the results with the game's hashes.
- **The boot executable's `.comment`** reads `MW MIPS C Compiler
  (2.4.1.01)`. That label comes from CodeWarrior's linker: the string is in
  the `mwldps2.exe` that comes with the compiler, whose own `mwccps2.exe`
  says 3.0.0. It doesn't point to another compiler.
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

Claude, Anthropic's AI model (Claude Opus 5.5), did the work in Claude
Code; the project's owner, Batenkaitos, directed it and made the
decisions.

- **What the build guarantees:** the 13 files are byte-identical to the
  game's, every function of Capcom's and zlib's compiles to the original's
  exact bytes, and none of Capcom's code comes from your copy
  ([Status](#status)).
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
  (`adler32.c`, `infblock.c`, `inflate.c` and `infutil.c`) with its header
  `include/zlib.h`, keeps zlib's own licence notice.
