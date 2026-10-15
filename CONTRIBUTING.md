# Contributing

Every one of Capcom's functions already matches, so the work left is
readability: names, types and fields, and the few places where the C
matches through a contrived form. This guide covers what goes in the
repository, the checks every change must pass, how to work on a function,
the conventions, and how the code and data are laid out. Which C gives
which instructions with the game's compiler is in the MWCC matching
reference, [docs/mwcc-matching.md](docs/mwcc-matching.md).

What is left, at the last count:
- about 1,200 struct fields in `include/` named by their offset (`unk1C`),
  and more in structs local to one file;
- data objects still named by their address (`D_00XXXXXX`);
- small groups of field accesses through raw byte offsets;
- 33 fakematches ("Fakematches").

## Ground rules

- **Never commit game data:** no executable, disc image, extracted file,
  asset or build output. `asm/`, `assets/` and `build/` are generated from
  your own copy and are gitignored. What the repository may hold is in
  "What goes in the repository" below.
- Your original disc files are read-only: the tools only read them.
- **Every change keeps the build matching:** `ninja` must print `OK` for
  all 13 targets ("Checking a change").
- **No game content in the source:** no values of the game's data and
  none of its text ("Content"); `python tools/content_scan.py` must pass.
- The licensed code (Sony's SDK and the toolchain's runtime, CRI's
  middleware, the network libraries and DNAS, ACCESS's kana-kanji engine)
  stays assembly generated from your copy (DNAS: a binary copy of its
  bytes). It is not decompiled or rewritten as C.

## What goes in the repository

- **Committed:** the decompiled C, with the structure of the data it
  defines (types, names, sizes, and the tables made only of references,
  such as state and callback tables); Capcom's hand-written assembly
  ("Hand-written assembly"); the headers; the splat configs, symbol files
  and hashes in `config/`.
- **Not committed: content.** The values of the game's data and its text
  come from your copy at build time ("Content").
- **Not committed: assets.** Data files loaded from the disc (lbguild, the
  AFS archives), embedded images, sounds, fonts or models, compressed or
  encrypted blobs, and data a library uses (DNAS's authentication data).
  Their descriptions (types, layouts, names) may be committed; their bytes
  come from your copy at build time, through an `INCLUDE_DATA` line ("Data
  in C") or a described asset (below).
- `python tools/data_blobcheck.py` flags blob-like definitions and any
  tracked file under `build/` or `asm/`.

### Described assets

A data file the game loads is described, not committed: lbguild.bin (328
guild quests and their 2,624 texts) is the one so far.
- The types are in a header (include/lbguild.h: `GuildQuest`) and the
  layout in config/assets/MODULE/FILE.txt: the header, the range the C
  owns, the roots (objects at known addresses), and what each pointer type
  points to (`target char* 8`: a `char **` points to eight texts). No
  values, and no counts the data alone gives.
- The YAML has a C piece for it (`[0x100, .data, lbguild]`) and no src/
  file. configure.py sees the description and has tools/asset_c.py write
  build/assets/MODULE/FILE.c from your copy, which is then compiled like
  any C file.
- asset_c.py checks that the objects and MWCC's alignment padding cover
  the range exactly and that every pointer lands on an object's start, and
  stops at anything the description doesn't explain.

### Content

The repository holds no values of the game's data and none of its text:
the build writes them from your copy.
- **Data:** an object keeps its declaration; its initializer is an include
  of a file that tools/content_gen.py writes from your copy at build time,
  build/gen/MODULE/ADDR.inc (ADDR: the object's address; build/ is never
  committed):

      s16 gSelHeaderLayout[16][2] = {
      #include "select/006CAD00.inc"
      };

  The generator reads the type from clang's parse of the file and the
  bytes from your copy. A word that holds the address of a name the file
  declares becomes the name; a pointer to a string of the file's `.rodata`
  becomes the literal. Tables made only of references (functions, data,
  zeros, offsets from names) keep their C: they are the structure, not
  content.
- **Text in code:** `TXT(0xADDR)` (include/common.h) for a literal of the
  game's text (Japanese, or English the player sees: a menu label, the
  copyright line), with a short English comment saying what it is, never
  the text; the file includes "MODULE/FILE.h", which defines it. Short
  functional ASCII strings (formats, file and device names, debug and
  assert messages) stay literals.
- **Comments** say what a text is, in English; they never quote the game's
  text.
- **Converting a file** (new data in C, or a new literal of the game's
  text): `python tools/content_conv.py src/MODULE/FILE.c` after a build
  (it reads the objects' addresses and sizes in build/MODULE.elf), then
  `python tools/configure.py` and `ninja`. `--text ADDR,...` makes the
  listed ASCII literals `TXT()`.
- **The check:** `python tools/content_scan.py` fails on non-ASCII text in
  src/ and include/ and in the other committed text files (config/,
  tools/, the guides, docs/), and on data initializers with values.
  config/content_exceptions.txt lists the reviewed exceptions, each with
  its reason: what is not the game's (zlib's tables, the PNG format's
  constants).

## Setting up

Follow the README's "Building" section: `tools/setup.sh` once, `source
tools/env.sh` in every shell, then `python tools/configure.py` and
`ninja`. Re-run configure.py after editing anything in `config/` (ninja
also re-runs it when those files change); `python tools/configure.py
--split` forces a fresh split of every target.

## Checking a change

Before you commit:

1. `ninja` prints `OK` for all 13 targets: the build is byte-identical to
   the game (config/targets.sha1).
2. `python tools/proto_check.py` reports 0 disagreements: every function
   declaration agrees with its definition, apart from the ones marked as
   the game's own ("Fakematches").
3. `python tools/data_blobcheck.py` reports 0 blob-like definitions.
4. `python tools/content_scan.py` reports 0 findings ("Content").
5. `ninja progress` still shows the `game` row at 12,551/12,551.
6. `git status` lists no generated file.

## Working on a function

- `python tools/func_diff.py MODULE FILE.c [FUNC ...]` compiles a C file
  and prints MATCH or DIFF for each function against the original, in
  about a second, with no build change. It also checks the jump tables
  and literals a function loads.
- `python tools/adiff.py MODULE FILE.c FUNC` aligns the two disassemblies
  before comparing them, so one extra instruction counts once. Its summary
  (`mn=` differing mnemonics, `full=` differing instructions) ranks drafts
  by how close they are.
- configure.py writes `objdiff.json`: open the repository in the objdiff
  GUI (`objdiff`) to diff a unit side by side; `ninja` rebuilds on save.
- `python tools/asm_funcs.py MODULE show FUNC` prints splat's disassembly
  of a function.
- **`static` matters.** MWCC tracks the registers a `static` function
  defined earlier in the same file uses, and after calling it doesn't
  reload what the callee leaves alone. Such a callee must be in the same
  file, before its callers.
- C sources are UTF-8; tools/mwcc_cc.py converts them to Shift-JIS, the
  game's encoding, before compiling.
- A rename or retype can change the code (a narrower type, another struct
  layout). When it does, the MWCC pattern catalogue
  (docs/mwcc-matching.md) and the permuter ("decomp-permuter") help find
  the form that matches.

## Names and types

- Names live in `config/symbols/<module>.txt`. Names in `main.txt` and
  `sub_main.txt` are visible to every module, and each module also links
  the names of the modules loaded with it (`LOADED_WITH` in
  tools/configure.py). Renaming a function renames it for its callers in
  other modules: their C must use the new name, or the link fails.
- Functions are `Module_Name` (`Movie_Play`, `Player_CopyPadInput`). Game
  data is `gName` (`gPlayers`). An object reached only through entry N of
  a named table is `TABLE_N` until its own use is known.
- Types that more than one C file needs go in `include/`. Some structs
  are generated: edit `include/NAME.spec`, then `python
  tools/gen_struct.py include/NAME.spec include/NAME.h` writes the padded
  struct into the header.
- A field whose use isn't known yet is named by its offset (`unk1C`).
  Naming one changes the header (or its .spec) and the field's users.
- Label networking code where you meet it, in the `NETWORK` sections of
  the symbol files.

## Fakematches

A match that relies on a form the original source very likely didn't have
(a test that checks nothing, a statement written twice, a cast with no
purpose, a copy that only mirrors the game's register allocation) is a
*fakematch*. Mark it with a comment that starts `FAKEMATCH:` and says what
is contrived and why it matches, so that it can be replaced when a natural
form turns up. `git grep FAKEMATCH` lists them.

Forms that are plausible for 2006 C need no tag. Two of them carry a
marker so that they can be counted:
- **A compiled-out check:** an empty `if (cond) ;` whose condition is a
  plausible validity check (an assert or a debug print compiled out).
  Mark it `/* compiled-out check */` on its line.
- **A unit's own declaration:** a declaration whose integer width or
  return type differs from the definition in another file, where the
  calling code matches only with it, is how the game's unit declared it.
  Mark it `/* unit's own declaration: <evidence> */`; proto_check counts
  these apart (431 at the last count). Where a cast at the call gives the
  same code, declare the callee as it is defined and cast the call
  instead.

Ordinary, with no marker: a block-scope declaration, `volatile` on a
global that other code changes, a dead variable reused for a later value,
a `#pragma optimization_level` pair around a function built at another
level, a local view of a global, and per-kind views of a shared object (a
struct with one kind's own field names over a generic object).

`NON_MATCHING` blocks (C under `#ifdef NON_MATCHING`, with the assembly
kept for the build) are something else: C that doesn't match yet. None is
left.

## Hand-written assembly

Capcom's own assembly is source: main's 12 memory copy and fill loops
(`Mem_*`, src/main/mem_loops.s) and sub_main's VU1 microcode (60
programs, src/sub_main/vu1_microcode.vsm). sub_main's 42 VU0 routines
are CodeWarrior `asm` functions inside their C files
(src/sub_main/unk_2E3940.c and unk_2F17C0.c): MWCC never emits VU0 macro
instructions from C (docs/mwcc-matching.md, "What can't come from C").
- The YAML keeps an `asm` piece for the range, and configure.py builds it
  from src/<module>/<piece>.s when that file exists. splat still writes
  its own disassembly to asm/, which objdiff compares the object with.
- The VU1 microcode is VU1 assembly, not MIPS: tools/vu1as.py, the
  project's own assembler, turns the .vsm file into GNU as source (the
  DMA packets as `.word`s). The file's header has the syntax.
  `python tools/vu1dis.py build/orig/sub_main.bin` prints the block from
  your copy in the same syntax, and its output assembles back to the same
  bytes.

## Functions with switch tables or strings

MWCC puts a `switch`'s jump table and every string literal in the C file's
`.rodata`, and initialized variables in its `.data`. In the game, each
translation unit's objects sit together, in its functions' order. So each
C file owns its data: its range of `.rodata` and `.data` is a subsegment
of its own in the YAML (`[0x186B50, .rodata, net_tcp]`).
- Strings are written as literals (the game's texts as `TXT()`,
  "Content"); MWCC emits a literal at its first use and reuses it for the
  same text later in the file.
- `.bss` variables are defined from the highest address down: MWCC emits
  them last-declared first.
- The game aligned every data object to at least 8 bytes, MWCC 3.0b52 to
  4; tools/mwcc_cc.py raises the C object's data sections to 8.
- A `switch` table spans the lowest to the highest case label, so cases
  that only `break` are listed when the game's table covers them.
- A string two of the game's units use has one copy per unit, while MWCC
  makes one per file: a file that spans two units names the second copy
  (`extern`).

## Data in C

All of Capcom's initialized data, 1.75 MB, is declared in C, with its
values generated from your copy ("Content"; `python
tools/data_progress.py` shows it per module); the libraries' data, and
ACCESS's engine's, stays assembly.
- **Aliases.** A name inside a C object (a label splat made at a field, a
  table's second record) is defined for the linker relative to its
  object, in config/ld/MODULE_aliases.ld: `D_008C9BCC = D_008C9BB0 +
  0x1C;`. The code keeps its names, and they move with the object.
- **Alignment decides where objects start.** MWCC puts an object of 16
  bytes or more on a 16-byte boundary, a smaller one on 8. Retyping an
  object can move the ones after it: check the addresses after it in
  build/MODULE.map.
- **False pointers:** numbers splat took for addresses stay `(u32)SYM`
  where they couldn't be told apart; the bytes are the same.
- **`.rodata` that no code loads** (tables that `.data` points to) is
  defined between `#pragma rodata on` and `#pragma rodata off`:
  tools/mwcc_cc.py moves those variables to `.rodata`, in source order,
  without `const`.
- **Objects that stay out of the repository** (an asset inside a C
  file's data) are an `INCLUDE_DATA("build/blobs/MODULE", D_X);` line in
  their place among the definitions. The object's size is on its symbol
  line (`// size:0x10000`), and tools/data_blob.py writes its bytes from
  your copy at build time.

## Progress

`ninja progress` runs `objdiff-cli report` and prints matched code per
module, then by origin (tools/configure.py, `unit_kind`): `game`
(Capcom's code), `sdk` (Sony's libraries, the toolchain's runtime, the
network libraries), `middleware` (CRI), `access` (ACCESS's kana-kanji
engine in sub_main, assembly from your copy like the libraries) and `vu1`
(the microcode). The `named` column counts matched
functions with a real name. objdiff counts a function as matched only if
its relocations match too. Padding (the overlays' leading zeros) is not
counted.

## decomp-permuter

[decomp-permuter](https://github.com/simonlindholm/decomp-permuter) finds
C variants that match. Its settings for this project are
tools/permuter_settings.toml and tools/permuter_prelude.inc (it looks for
them in `tools/`); how to use it here, and the false fixes it finds, are
in docs/mwcc-matching.md, "decomp-permuter".
