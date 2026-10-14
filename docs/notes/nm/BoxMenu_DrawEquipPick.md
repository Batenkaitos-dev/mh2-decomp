# BoxMenu_DrawEquipPick (lobby, src/lobby/unk_7683D0.c, level 4)

## Status

Matched (round 30, s5): 23 -> 0 words. Unwrapped; every function of the file matches (38);
ninja (13 OK). Natural C, no FAKEMATCH tag.

## What matched

- The empty-list text colour (13 or 2, read only by the `n == 0` case's Text_SetColor) has its
  own `s16 ecol` local. Since round 22 it had been held in y, the list's y position (y's first
  web, with `y = 0x2F` its later web).
- The declarations end `u16 n; s16 y; u16 *p; u8 alpha;` (alpha moved after y and p). ecol can
  go anywhere: all five places in the block of five match.

## Why it works (for the models)

- With y holding both values, the two webs of y were coloured apart: over the 24 orders of
  n, alpha, y, p, y's first web always shared p's register and y's later web (`y = 0x2F`, the
  loop's) always shared the `flags & 1` CSE's (s0 in every order), 23-40 words. The game pairs
  them the other way round: flags & 1 with p (s1), the colour with the loop's y (s2).
- With the colour in its own local, y's only web is the loop's (a first web, numbered at its
  declaration, not a later web numbered after the created values). Then the order n, y, p, alpha
  gives the game's colouring: alpha s0, p and flags & 1 s1, y and ecol s2, n s3.
- So round 29's reading (alpha must be numbered after the created flags & 1) was not the fix:
  the later web of y, numbered last and coloured first, was the value in the wrong place.
  Where a reused variable's later web takes a register too early (s0), give one of its values
  its own variable so the other is a first web.

## Tries (round 30, s5; gen.py scores, words)

- A declared sel for flags & 1 (u32, s32, u8; `sel = flags & 1` or `sel = flags; sel &= 1;`, at
  the top or in the if) x 6 orders of sel, y, alpha: 23 (substituted away; the u8 forms 28/382).
- u32 alpha with `(alpha % 256) << 24` (file prototype, or (u8, u32, s32) in the block): 23
  with addiu for the daddiu, the same registers; u8 alpha unmasked: 23; u16 alpha: 294.
- The 24 orders of n, alpha, y, p (y still holding the colour): 23-40.
- The 120 orders of n, alpha, y, p, ecol: MATCH for the five with n, y, p, alpha in order.

## For the lead

- Nothing outside BoxMenu_DrawEquipPick changed. The comment above BoxMenu_DrawItemList
  (line ~158) still says this function fits (u8, s32, u8) with an s32 alpha (round
  21); it now declares the definition's prototype in its block (u8 alpha). Not edited (outside
  this function).
- Scratch: ep (gen.py with --regs, mkperm.py for declaration orders). No job
  left running.
