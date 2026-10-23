# GroupBoard_DrawRecruitForm (lobby, src/lobby/group_board.c)

## The match
Natural C, no tag. Three forms (from a 15-word form that copied ty into an s32 `ry`):
- **No `ry` local.** The 15-word form copied ty into an s32 `ry` before each row block and
  bumped it (`ry += 0x16`). Written out as `ty + 0x18` and `ty + 0x16`, MWCC makes a common
  subexpression of ty's s32 extension (`dsll32/dsra32` of the already extended ty) in a saved
  register and computes `ty + 0x16` in place in it: the game's copy in s6 (row 1 block) and
  s5 (row 2 block). This put the first string, the first copy and the third string in s6 at
  once, and swapped x's extension/bx (game s4) with r, x + 0x1E and the second copy (game
  s5): 27 words. Every single declaration move and the local types left it at 27 or worse.
- **x + 0x1E in a declared `s32 lx`** (`lx = x + 0x1E;` before its first use, declared 6th
  or later): 2 words. Declared 1st to 5th it pushes r/lx down (38-91); `s16 lx` 279-310. In
  the ry form the same local changes nothing at any place.
- **`Gfx_DrawRectFrame(bx = x + 0x32, ty + 0x16, ...)`** in both branches of the row 1
  test. With `bx = x + 0x32;` as its own statement before the call, the row 1 block created
  bx before `ty + 0x16` and the final schedule put `addiu s4,s4,0x32` first; the game has the
  copy's bump first (2 words). The assignment in the argument creates `ty + 0x16` first.
  (Only the if branch needs it; both are written alike.)

## Evidence
- Learning variants: with the first string not a saved value, x + 0x1E stayed s6 and ry1 s5,
  so ry (a declared variable with many references) was the value coloured before x + 0x1E,
  not the string.
- In the game the nine values live across the x + 0x1E window take s0..s8 in colouring order
  (cx, ty, w, x, x's extension, x + 0x1E, string/copy, y, x + 0x9C): a 9-clique, so the
  register order there is the colouring order.
- Method: variants of the block scored by func_diff, with the key values' registers on one
  line, and every single declaration move tried.
