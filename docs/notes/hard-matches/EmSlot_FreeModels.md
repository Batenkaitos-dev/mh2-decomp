# EmSlot_FreeModels (sub_main, src/sub_main/scene_models.c)

## The match

A FAKEMATCH. The tag is for the `(s16)n` cast, which only keeps two equal index expressions
apart (the rulings' Em_TurnToward case, Em_TurnToward.md). `s32 n = (s32)i;` with an explicit
cast and `emKind[i]` (the parameter at the last store) are ordinary C but needed; the empty
check is marked `/* compiled-out check */`. Without these forms the function misses by 30
words. No natural form found for the cast.

## What the match needs

1. **The empty delay slot** (`li v0,-1; beq cnt,v0; nop; dsll32 s0,a0`). MWCC does not move
   the fall-through's first instruction into the slot when its destination register is read
   earlier in the branch's block: in the game J2 (s0) is D's register, and D is read by the
   block's `addu v0,s0,s6`. Once the registers are right the slot empties by itself (test
   copies t3-t7; the slot looked "filled in every form" only while J2 was in another register).
2. **D (n*2) numbered after X1**: a CSE is numbered at its reuse. The first block's two reads
   must not share n*2, so the reuse is in the block after the loop: `gScene.modelCount[(s16)n]`
   (also `(s32)n`, `(s16)i`, or the cast on the other read). Not: `n + 0`, `n | 0`, `n * 1`,
   `+n`, `*(gScene.modelCount + n)`, `(&gScene)->`, an empty check between the reads.
3. **N (n) numbered after PC**: `s32 n = (s32)i;` (or `(int)`; a permuter find) with the last
   store `s->emKind[i] = 0;`: n's extension then behaves as a CSE whose reuse is that store.
   `s32 n = i;` (and u32, s16, block-scope, any i/n mix: 256-variant sweeps) numbers n at its
   declaration (s5). A contrived later web of n (n first used for a test) gave every register
   too, with extra code: the numbering model holds.
4. **The block after the loop split between its two reads** (each read then uses n*2 once and
   keeps the first block's value, `s + n*2` shared): `if (first < 0 || first >= 0x80) {}` (a
   compiled-out range check of the model slot; gModelUsed has 0x80 entries). One-test empty
   ifs (`if (c) {}`, `if (c) ;`, `(void)0`) don't split; two-test ones, `if (c) {} else {}`,
   `c ? 0 : 0;`, `while (c) break;`, `switch` with one case do.

## Rules

Candidate CONTRIBUTING lines:
- "Delay slots filled or empty": MWCC leaves a conditional branch's slot empty when the
  fall-through's first instruction writes a register that is read earlier in the branch's
  block (fix the registers first).
- The numbering: an explicit cast in a local's initialiser (`s32 n = (s32)i;`) can make it a
  common subexpression numbered at its reuse instead of a declared local; an explicit cast in
  one of two equal index expressions keeps a CSE from being created in that block.

The same model applied to EmSlot_LoadModels (same file): see EmSlot_LoadModels.md.
