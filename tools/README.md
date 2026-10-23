# tools/

Run the tools from the repository's root, in a shell prepared by
`source tools/env.sh`. Each script's docstring (or `-h`) has its usage.

## Setting up

| Tool | What it does |
|---|---|
| `setup.sh`, `versions.txt` | install the pinned toolchain into `tools/deps/` (gitignored) |
| `env.sh` | put it on PATH, in every new shell |
| `check_env.py` | run every installed tool once |

## Building and verifying with your own copy

| Tool | What it does |
|---|---|
| `verify_originals.py` | check your copy against `config/disc.sha1` |
| `orig.py`, `afs.py` | find and read your disc files; read the AFS archives |
| `extract.py` | pull the boot ELF and the 12 code overlays from your copy |
| `configure.py` | extract, split with splat, write `build.ninja` and `objdiff.json` |
| `mwcc_cc.py`, `sjis_as.py` | the build's compiler and assembler steps |
| `vu1as.py` | the VU1 assembler: src/sub_main/vu1_microcode.vsm to GNU as source |
| `asset_c.py`, `gen_data.py`, `type_data.py`, `data_blob.py` | the C of a described asset, and the data that stays in your copy, written at build time |
| `content_gen.py` | the values of the C's data and the game's texts, written from your copy into `build/gen/` at build time |
| `export_readable.py` | a copy of the tree with your copy's data and text written out in C (default `build/readable/`; CONTRIBUTING.md, "The readable export"). It holds the game's content: keep it on your machine |
| `check_targets.py` | compare the 13 built files with `config/targets.sha1` |
| `progress.py` | `ninja progress`: matched code per module and origin |

## Checking and working on the code

| Tool | What it does |
|---|---|
| `proto_check.py` | function declarations that disagree with their definitions |
| `content_scan.py` | game content in the source: non-ASCII text, data initializers with values |
| `content_conv.py` | take a C file's data values and game texts out into generated includes |
| `data_blobcheck.py` | blob-like data and tracked build outputs |
| `data_progress.py` | the data C owns, per module |
| `func_diff.py`, `adiff.py` | compile a C file and compare each function with the original; an aligned diff of one function |
| `asm_funcs.py` | browse splat's assembly by function |
| `gen_struct.py` | write a header's padded struct from its `include/*.spec` |
| `vu1dis.py` | print the VU1 microcode from your copy in vu1as's syntax |
| `permuter_settings.toml`, `permuter_prelude.inc` | decomp-permuter's settings (it looks for them in `tools/`) |
