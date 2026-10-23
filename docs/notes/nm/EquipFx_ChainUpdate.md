# EquipFx_ChainUpdate (sub_main, src/sub_main/player_looks.c)

## Status

- Round 30 (slot s5): MATCHED (54 -> 29 -> 0 words), tagged FAKEMATCH for two forms:
  1. the copies `b1 = pb1, b2 = pb2, m = pm` made again in the outer loop's step (the game
     spills b1, b2, m as locals in 16-byte slots; a plain copy of a parameter is propagated away
     and spills as the parameter, 4-byte slot; two definitions keep the copies variables and the
     step copies give no code);
  2. `f32 *dy = &dir[1], *dz = &dir[2]` (declared before bone and st, set before the outer
     loop, after `a = pa`) read and written by the clamp only: the game numbers the two hoisted
     addresses before bone and st. Declared after bone/st, set inside the loops, block-scoped or
     unused: no (30). Used in the clamp only or in every dir[1]/dir[2] use: both match.
- Also part of the match: vec and wpos declared before tip; `a = pa` after `l = list`.

## Handoff

- Nothing left. A natural form for either FAKEMATCH form would replace the tag (next ideas
  below). Generator: efx/gen.py (variants file; `FULL d3_clamp.c` = the match).

## Round 30 tries (positional words from gen.py; func_diff gives about one less)

- Frame (b1, b2, m in 16-byte local slots): const copies, pointer-type copies (f32 * params),
  volatile/const params, K&R definitions (copies or params direct), a as the parameter changed
  in place (4-byte slot), dead redefinitions after the loop, `b1->unk000` forms, the whole body
  or the per-chain body as a static inline: all keep 4-byte parameter slots (55-129).
  Diagnostics: with `return (s32)pb1` the copy and the parameter are still one value (copies of
  parameters are propagated whatever their type; Page_Click's const lever is for globals).
- Works: a second definition of the copies. In the loop step: game slots, no code (35 with vec
  and wpos first). At the top of the body (or block-scope copies in the body): b1, b2 right
  but m (one use per iteration) is substituted into its use (36).
- `a = pa` after `l = list` (the game stores a0 after l's move, in the preheader): 35 -> 30.
  a as an index walker (`pa + i * 0x20`): a temporary slot below n (41). list[i]: 399.
- Registers (all 30 or worse): st/bone declaration order (swaps them only), block scope (in the
  outer body or the inner if), inner loop as for/while (46), st or bone written as index
  expressions (st `a + j * 0xC + 4`: an SR walker, numbered after bone, before dir1; bone
  `&bone[j]`: same), bone `m + k * 0x190` at each use (311), st/bone/j as later webs of dead
  variables (contrived first webs: later webs are numbered after jext, s0), declared pointers to
  dir[1], dir[2] (372: a stops spilling).
- Permuter (perm_nu from d2, base 155): about 9,700 iterations, nothing under 155 (stopped).
- Also no change:  (st numbered as before, 352: slots move), u32 walkers (311).

## Next ideas (for a natural form)

1. The step copies: a source shape with two definitions reaching m's single use per iteration.
2. The dir pointers: the registers needed bone and st numbered after the hoisted &dir[1],
   &dir[2]. Only declared pointers set before the loop gave it; SR walkers (index forms) and
   later webs did not (see tries).

## For the lead

- Matched with a FAKEMATCH comment naming both forms: the step copies (a copy that only
  mirrors the game's allocation) and the dy/dz pointer locals (pointer locals aimed at the
  elements the game keeps in saved registers). Ruling wanted if either should count as natural.
- Lever for CONTRIBUTING: a parameter copy spills as the parameter (4-byte slot) unless a second
  definition reaches its uses; a copy made again in the loop step costs no code.
