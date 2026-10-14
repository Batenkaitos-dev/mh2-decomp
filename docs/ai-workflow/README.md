# How this project was made

This project was completed almost entirely autonomously by Claude Opus 5.5,
Anthropic's AI model, working in
[Claude Code](https://www.anthropic.com/claude-code). It worked from
instructions on how to build the project, compile with the original
compiler, and verify the byte matching. The decompiled C, the tools and the
documentation are its work.

The project's owner, Batenkaitos, orchestrated the work, tested the results,
and steered the decisions when important questions came up: the compiler,
the porting strategy, the decomp's scope, and what goes in the repository.

## What the checks guarantee, and what they don't

**Guaranteed by the build:**
- The boot executable and the 12 code overlays built from the repository are
  byte-identical to the originals (SHA-1, `config/targets.sha1`). A function
  counted as matched compiles to exactly the original's bytes with the
  original compiler.
- None of the game's data values, text or assets, and nothing generated
  from the game, is committed (`tools/content_scan.py`,
  `tools/data_blobcheck.py`; CONTRIBUTING.md, "What goes in the
  repository").

**Not guaranteed. These are the AI's interpretation and can be wrong:**
- function, variable and field names, comments, and the explanations in the
  notes and docs;
- fakematches: matches through a contrived form, marked `FAKEMATCH`.

Corrections are welcome.

## What it didn't do

- **Copy code** from other decompilation projects or from leaked sources.
  The C is reconstructed from the owner's own copy of the game.
- **Work in projects that ask AI to stay away.** 2Tie/mh1j (Monster
  Hunter 1) was read no further than its README;
  dreamingmoths/memory-of-alessa (Silent Hill 2), whose policy asks LLMs not
  to work in it, isn't used; decomp.me, which asks that no LLM be hooked up
  to it, was never automated.
- **Submit work** to other projects. Many decomp communities don't accept
  AI-generated contributions, and this project respects that.
- **Download** game data, SDKs or BIOS files, or connect to game servers.

## The record

The full development record (the notes of every round of work, the
decisions, and the tools that ran the work) is kept in the project's
private repository. This repository starts from a clean history and holds
the decompilation only.
