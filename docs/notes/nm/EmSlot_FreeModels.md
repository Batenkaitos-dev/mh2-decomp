# EmSlot_FreeModels (sub_main, src/sub_main/unk_33B080.c)

## Status

MATCHED as a FAKEMATCH (round 30, slot s1, session 2), from 30 words. func_diff: all 22
functions of the file MATCH; ninja (13 OK).

## Handoff (next agent: start here)

Nothing left unless a natural form turns up for the contrived cast (below, "For the lead").

## What the match needed (the findings)

1. **The empty delay slot** (`li v0,-1; beq cnt,v0; nop; dsll32 s0,a0`). MWCC does not move
   the fall-through's first instruction into the slot when its destination register is read
   earlier in the branch's block: in the game J2 (s0) is D's register, and D is read by the
   block's `addu v0,s0,s6`. Once the registers were right the slot emptied by itself (tests:
   scratch s2/t/t3-t7.c; the earlier "filled in every form" came from J2 in another register).
2. **D (n*2) numbered after X1**: a CSE is numbered at its reuse. The first block's two reads
   must not share n*2, so the reuse is in the block after the loop: `gScene.modelCount[(s16)n]`
   (also `(s32)n`, `(s16)i`, or the cast on the other read). Not: `n + 0`, `n | 0`, `n * 1`,
   `+n`, `*(gScene.modelCount + n)`, `(&gScene)->`, an empty check between the reads.
3. **N (n) numbered after PC**: `s32 n = (s32)i;` (or `(int)`; a perm_nu find) with the last
   store `s->emKind[i] = 0;`: n's extension then behaves as a CSE whose reuse is that store.
   `s32 n = i;` (and u32, s16, block-scope, any i/n mix: 256-variant sweeps) numbers n at its
   declaration (s5). A contrived later web of n (n first used for a test) gave every register
   too, with extra code: the numbering model holds.
4. **The block after the loop split between its two reads** (each read then uses n*2 once and
   keeps the first block's value, `s + n*2` shared): `if (first < 0 || first >= 0x80) {}` (a
   compiled-out range check of the model slot; gModelUsed has 0x80 entries). One-test empty
   ifs (`if (c) {}`, `if (c) ;`, `(void)0`) don't split; two-test ones, `if (c) {} else {}`,
   `c ? 0 : 0;`, `while (c) break;`, `switch` with one case do.

Scratch: s2 (gen.py with `--slot`; mk.py + sp*.py variant specs; t/ test
files with cc.py and slot.py).

## For the lead

- FAKEMATCH tag on the function: the `(s16)n` cast only keeps two equal index expressions
  apart (the rulings' Em_TurnToward case). `s32 n = (s32)i;` with an explicit cast and
  `emKind[i]` (the parameter at the last store) are ordinary C but needed; the check is
  marked `/* compiled-out check */`.
- Suggested lines for docs/mwcc-matching.md: (a) "Delay slots filled or empty": MWCC leaves a conditional
  branch's slot empty when the fall-through's first instruction writes a register that is
  read earlier in the branch's block (EmSlot_FreeModels: fix the registers first); (b) the
  numbering: an explicit cast in a local's initialiser (`s32 n = (s32)i;`) can make it a
  common subexpression numbered at its reuse instead of a declared local; an explicit cast
  in one of two equal index expressions keeps a CSE from being created in that block.
- EmSlot_LoadModels (FAKEMATCH, same file) may come out more naturally with these levers.
