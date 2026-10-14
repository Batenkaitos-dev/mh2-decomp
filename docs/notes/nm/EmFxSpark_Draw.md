# EmFxSpark_Draw (game, src/game/em_bulldrome.c, level 4)

## Status

- Matched in round 30 (s3), from 6 words. The whole file is MATCH (113 functions, no
  NON_MATCHING block left in it); ninja (13 OK).
- The fix (case 3, the fade when fx->timer < 20): the alpha in a u32 local set twice,
  `u32 a = (color >> 24) & 0xFF; a = (u32) (a * (fx->timer / 20.0f)) & 0xFF;`, then `(a << 24)`
  in the same `B | (G | (A | R))` tree as before. No fakematch: a local set twice.

## What it was

The game's join after the float-to-u32 conversion starts with the red channel's `srl` (copied
into both paths), then the alpha's `andi`; its `sll` of the alpha is in place (v1) and the red
channel's sll goes to a0. Ours had the andi first, the alpha's sll in a0 and the red one in v1.

## Why it works (each step checked with test files in t)

1. Colouring, not the scheduler, decides it. A replay of rule 4 ("How MWCC colours temporaries")
   over every first-pass order of the join (scratch sim.py, sim1.py; 9240 orders): with our
   creation order (andi, sll A, red srl/andi/sll, or, ...) no order gives the game's registers.
   With the alpha's sll created after the red chain (andi, red chain, sll A, or, ... or red
   chain first) 48 orders do. With the game's colouring, the second pass puts the srl first by
   height (g0's srl v0 waits for the red andi's v0), so the head of the join follows.
2. Operand evaluation order (t/o1-o7.c; level 4 too): for `L | R` MWCC evaluates R first when R
   has at least two operations more than L (`(t << 24) | R3`: R first; `((t & 0xFF) << 24) | R3`:
   L first; `G3 | ((t << 24) | R3)`: right first). A float-to-u32 conversion counts as none,
   but the float expression inside it counts. The `or` keeps the source's operand order.
   So the alpha operand must be `a << 24` with `a` an already-computed value.
3. At level 4 MWCC substitutes a local assigned once and used once, and also in-place chains
   (`x = conv; x &= 0xFF;`, `x = conv & 0xFF; x <<= 24;`): every such form puts the conversion
   back into the operand (A first, 7). A local whose second assignment reads the first
   (`a = (color >> 24) & 0xFF; a = (u32)(a * f) & 0xFF;`) stays a value (test t/g3c.py D5): the
   andi is created at its statement, `a << 24` after the red chain.
   (At the default level, `x = conv & 0xFF; x <<= 24;` with color a parameter also works: t/gen1.py
   W6. The function is at level 4 and color is in memory, so it does not.)

Declaring a at the top of the function (after color, or after alpha) matches too.

## For the lead

- A possible line for "How MWCC schedules" or the catalogue's temporaries entry: in `L | R`
  MWCC evaluates the right operand first when it has two or more operations more than the left
  (count the ALU operations; a float-to-int conversion counts none). A light operand such as
  `a << 24` therefore comes after a three-operation channel, and the colouring follows that
  creation order. To get it, the light operand's value must not be substituted: at level 4 a
  local assigned twice where the second assignment reads the first keeps its value; single
  assignments and in-place `&=`/`<<=` chains are substituted.
