# Cam_LineHitDist (sub_main, src/sub_main/cam_script.c)

## Status
Round 30 (s4, FAKEMATCH review): matched with the FAKEMATCH tag (`D_0041CE40[0, 0]` on a[0]'s
read); no natural form found, code left unchanged. Plain spelling: 9 of 70 words (the three
offset loads take f3, f2, f1 for x, y, z; the game has f1, f3, f2).

## Handoff
Stuck at "no natural form". What would end it: an original whose two reads of the x offset are
different expressions to the IR common-subexpression pass (see Mechanism), written naturally. No
idea left that the tests below have not covered.

## Mechanism (round 30)
- The three offsets are common subexpressions, numbered where the CSE pass finds them, at the
  second occurrence (the b[] reads): plain numbers x < y < z, coloured in reverse, so z f1, y f2,
  x f3. The game needs y < z < x (tempcolor --whatif: number x after z, 15/15).
- Proof: statement order a0 a1 a2 b1 b2 b0 gives exactly the game's registers (9 words left, only
  b's store order); a1 a2 a0 b0 b1 b2 keeps the plain colouring (first occurrences don't count).
- The comma (and `s32 k = 0; D[k]`, and a pointer cast `*(f32 *)D_0041CE40`, on EITHER of the two
  x reads) makes the two x reads different expressions to the IR CSE: y and z stay CSE values
  (numbered early), x becomes a code-generator temporary (numbered after them) and a back-end pass
  merges the two loads later. Index or cast on all six reads, or on y/z reads, gives 9 again.

## Tried this session (scores in words; plain = 9)
- 64 operand-order combinations of the six statements (`D + to` vs `to + D`): all 9.
- All 720 orders of the six statements (exhaustive): only source order and a0a1a2b1b2b0 give 9
  (the latter with the game's colouring), the rest 11-22.
- x read spelled 14 ways on a[0] or on b[0]: `*(f32 *)D`, `((f32 *)D)[0]`, `*(f32 *)&D`,
  `*(f32 *)((u8 *)D + 0)` MATCH (all casts); `*D`, `(&D[0])[0]`, `*&D[0]`, `*(D + 0)`,
  `(D + 1)[-1]`, `D[(s32)0]`, `D[0u]`, `D[(u8)0]`, `D[0 & 0]` 9; `D_0041CE44[-1]` 51.
- k index: x's b[0] read only MATCH; a[1] only 6; a[2] only 9; a[0..2] 9; all six 9.
- f32 *o = D_0041CE40: initialiser before the call 67-71 (o kept in s0); assigned after the check
  (propagated) 9, for all reads, a only, b only, with o[3] for the radius or not; `(f32 *)D` 67.
- Inline helper Cam_AddOfs(d, s, o) for a only or b only 53; both 9.
- A local ox for x only (statement, or assignment in the expression) 9; initialiser 46.
- for loops over i for a only 60, b only 58.

## For the lead
- The game's table is a sphere list (`{x, y, z, r}`, ended by r = -1, as Em_CollideField reads
  with `f32 *sph; sph += 4`); Cam_PushOutOfEms reads it through `o = D_0041CE40`. Nothing in this
  function's asm points to an index or a cast; keep the tag. Of the matching forms, the cast
  `*(f32 *)D_0041CE40` on one read is the least odd spelling, but it is no more natural than the
  comma.
