# BoxMenu_DrawItemList (lobby, src/lobby/box_menu_draw.c, level 4)

## The match

- `u32 alpha;` and loops 1 and 1b write the alpha byte as `(alpha % 256) << 24`:
  `r.color = ((alpha % 256) << 24) | D_007F1A30[page];` and `s.color |= (alpha % 256) << 24;`
  (the two must be the same expression: it is the common subexpression that shares s1). With
  an s32 alpha, `((u32)alpha % 256) << 24` (or `% 0x100`) matches too; the u32 declaration
  needs no cast. Loop 2 keeps the `a` form (`for (i = 0, a = alpha; ...)`).
- The price read in loop 1b is `gItemData[stk->item].value` (the s32 at 0x10; the game's
  `%hi(gItemData + 0x10)`). A form `*(s32 *)&((ItemData *)D_45F120)[stk->item]` names a symbol
  no linker script defines and does not link.

## Why it works

- p735 (the permuter's find: unmasked `alpha << 24`, no andi at all) gives every instruction
  and register of the block but the andi: the game's block is that plus an andi live from
  cycle 1 to the sll.
- The first-pass model (two slots, load latency 1 here, an instruction that ends a value
  first, else the earliest-created ready one; the float constants numbered in source order
  153, 146, 28.8), simulated in a script, reproduces a_init (`a = alpha` in the for, andi v0)
  and p735, and says no creation order of a plain andi gives a0: the sll that ends it is always
  taken the next cycle. So the game's mask is likely not an andi during the first pass: MWCC
  apparently strength-reduces an unsigned `% 256` to `andi` after it (a divu then, long
  latency, so the shift waits), and the colouring sees the long live range.
- Rule for the catalogue: an andi that the game keeps live longer than any first-pass order
  allows can be an unsigned `% 256` (or another power of two) in the source. (The mechanism is
  inferred, not traced: the modulo form gives the game's colouring and schedule; the pass that
  rewrites it was not identified.)

## Evidence (scores are words in func_diff)

- Colour expression forms in loop 1 (20): 13-14; forms that differ from loop 1b's break the
  CSE (734); a variable for (u8)alpha (11 variables, before/in the for): 15-17 (andi v0); loop
  2 with `a`: 11.
- `a = alpha` with do/while/for-without-init/`!=` loops: 15; an earlier web of `a`: 15-18;
  `i = 0` before the if: 19 (andi v1, c2 v0); `* 0x1000000`: 11-15; split colour stores:
  765-770; u8 alpha: 120; a 15-minute permuter run from the `a = alpha` state: only unmasked
  alpha (13).
- `(u32)alpha % 256 << 24` in loops 1 and 1b: MATCH; then u32 alpha and `alpha % 256`: MATCH.
