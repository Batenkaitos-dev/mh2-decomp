# QuestDesk_DrawFieldGuide (lobby, src/lobby/quest_desk_draw.c)

## Status

- **Matched** (round 30, session 4, slot s4). The title line is written
  `lx = Text_CenterX(buf, j, 0x12); tx = lx - 0x12; Font_SetPos(tx, cy);`: tx's first web is the
  title's x and lx's the centred x (both dead variables reused before their later values, the
  label x and the list x), with session 3's 15-word state otherwise (block `lx = tx; p = ...;
  y = cy;`, tx declared first). Meaning checked: half is i's later web, the loop counts with j,
  the drops use q, top is kept in y.
- Found by perm_nu from session 3's 15-word state (q5/s15.c) in about 10 minutes (its form was
  `lx = centre; tx = lx; Font_SetPos(tx, cy);`, a copy); the subtraction moved into tx is the
  natural form of the same thing and matches too.

## Handoff

- Nothing left: QuestDesk_DrawFieldGuide is matched and unwrapped (file all MATCH, CHECK OK).
- Scratch q6 (session 4): vr.py (variants), hcs.py, s9.c/s9n.c (9-word states),
  match80/ (the permuter's match).

## Session 4 (func_diff; scratch q6)

- perm_nu from s15 (p1): 135 = `lx = m[0x74]; idx = lx;` (9 in the build: x/y chains right, only
  tx s6 / lx s4 swapped), then 80 = MATCH (above). Stopped once matched.
- The 9 family: the season index in a reused s16 idx that later holds the list x (no lx;
  `p = ...; idx = tx; y = cy;`) 9 at every idx place; hill climb (decls, block orders, c2, Lb)
  from 9: none below in the lx-first orders (PYI/YPI give the old top-first 4). tx/lx swap
  untouched by: lx or tx at any place, local types (u32/int/s16), the drops' uses (probe),
  tx as sx's later web (196), tx as a common subexpression (19-196), the title centre inline
  or in cy2 (9).
- From 15: the centre in its own s32 cx at every place 15-76; `sx = x;` moved earlier 37-116.
- Title-line forms from 15: `lx = Text_CenterX(...); tx = lx - 0x12; Font_SetPos(tx, cy)` MATCH,
  `lx = ... - 0x12; tx = lx; Font_SetPos(tx, cy)` MATCH, `tx = ... - 0x12` alone 20,
  `tx = lx = ...` 20, `Font_SetPos(tx = lx, cy)` 15, `tx = lx` with `Font_SetPos(lx, cy)` 15.

## Tried in session 3 (func_diff; scratch q5)

- perm_nu from the 4-word state (q4/nm6, run by the cut-off session, 44 min, 147k iterations):
  nothing below its base (120 = the base score).
- Which extension goes first, measured on every variant (q5/ord.py): with `lx = tx` (and casts
  `(s16)tx`, `(s32)tx`, `(s16)(s32)tx`, any cast on `y = cy`: all the same code) the final order
  is the source order of the two statements, and lx first always gives other registers: LPT/PLT
  94, LTP 123, TLP 124 (9 saved registers), TPL/PTL 4 (q5/v4.py, 168 variants).
- `lx = sx + 0x28` (tx's CSE; also `(s16)(sx + 0x28)`): top goes first whenever `y = cy` is
  before p's statement, whatever lx's place (LTP: lx first in `#pragma schedule off`, top first
  in the build), lx first otherwise: TPL/TLP/LTP 11 (tx s6, lx s4), LPT 93, PLT/PTL 96. So the
  final order is not always the source order, but lx first still goes with the 93-96 colouring.
- lx as x's later web (`x = tx;`, x in the loop and the drops, no lx): LPT/PLT 20 with the game's
  schedule (lx first); TPL/PTL 11. The parameter's webs share one register: x and lx both s3
  (game x s4, lx s6), y and top s4, tx s6. Hill climb (decls, block orders) from 20: none below.
  On the 27/31 states: 27, 81.
- A separate loop variable `my = cy;` (s16 or s32; the copy is coalesced, no code) in all 24
  orders of lx/my/top/p: 76-106; lx first only with 78+ (q5/v5.py).
- `m = (u8 *)(s32)gMenuWork;` (the sibling QuestDesk_DrawRecruitForm's form), at the statement
  or as the initialiser, or `(u32)`: identical code (same 4/11/93-124 table).
- lx as a block-scope local with its initialiser (`{ s16 lx = tx; ...` to the end): LPT/PLT 99,
  LTP/TLP 128, TPL/PTL 4; with `sx + 0x28` the Lb table.
- Black-box tests (q5/t1.c, t2.c, q4/t1.c; sc.py): two s16 extensions before a loop: the one that
  is its source's last use goes first, otherwise creation order (A1-A8). A coalesced s32 copy
  (`w = b; v = b;` with w changed in the loop) does not make `v = b` a last use (B1-B3): creation
  order. An s16 local copied from an s16 parameter gets an extension (C1, C2); one copied from an
  s16 local (`my = cy`) is coalesced with no code.
- lx first web = the title centre x (`lx = Text_CenterX(buf, j, 0x12) - 0x12; Font_SetPos(lx, cy);`):
  LPT/PLT 82, TPL/PTL 34, LTP 128; also the second centre (c2) 173-236. Hill climb from LPT 82
  -> 15 (tx declared first; q5/h4): see Status. From 15: c2, lx = sx + 0x28, PLT, every single
  declaration move: none below (h5); casts `(s32)x`, `(s32)y + 0x28`, `(s32)sx + 0xA0`, operand
  orders, `y = (s16)cy`: all 15; `tx = x + 0x28` 203.
- From 31 (q3/s31.c): hill climb with lx = sx + 0x28, m after memset, PLT: none below 31 (h3).

## Tried in session 2 (func_diff)

- From 4: `lx = sx + 0x28` (CSE with tx) 11 in TPL/TLP/LTP, 93-96 otherwise; `y = cy` or `lx =
  tx` in the for's init: lx after top 4-5, lx before top 94; hill climbs of the declarations with
  the block fixed as LTP 123 -> 78, LPT/PLT 94 -> 31.
- From 78 (y = cy) with the block as TPL or PTL: hill climb 87 -> 37 -> 4 (applied); TLP start
  80 -> 78, PLT start 86 -> 31 (= LPT). With the 4-word declarations: LTP 123, TLP 124, LPT/PLT
  94; `lx = (s16)tx`, `y = (s16)cy` 4.

- Block orders (lx/top/p) from 62: LTP 62, TLP 64, others 98 (8 saved registers); from 78 (y = cy):
  LTP 78, TLP 80, LPT/PLT 86, TPL/PTL 87. Swapping the half and idx loads: +10 (code changes).
- Hill climb from LPT: 98 -> 27 (no y = cy; committed); with y = cy 86 -> 31. From LTP (62, 78)
  every single move: none below.
- From 31: every integer local in 6 types: none below 31 (idx s32/u8/u16/u32, kind u32, sx/tx u32
  31); m after memset 32, `u8 *m = gMenuWork;` at its declaration 31, declared first 69, last 56;
  gMenuWork with no m 330 (absolute addressing).
- From 31: the centre x as its own `s32 cx` (no j first web) 74, hill climb 74 -> 63 -> 31 (cx
  declared after sx, before j: the same registers as the 31 state, a more natural form); cx as
  tx's first web 81; `sx + 0xA0` written twice (CSE) 199. m in an inner block 56-58; a second
  `m = gMenuWork;` anywhere 191-318 (re-materialised); m as char/s8/u16/s16/u32 pointer or s32
  325/254 (code changes); `register` / `const` m 31; `cy += 0x1E;` before or after tx 31.
- perm_nu from 78 (nm4, 25 min, stopped): only meaning changes (i reused for tx or the compare);
  from 31 (nm5, about 30 min, stopped): best 410 = an alias of m set in one branch only (junk).

## Tried in session 1 (func_diff)

- half as its own local, s8 or s32, 15 places: 128-132; with `y = cy` 131; m[0x75] read twice
  109 (a reload: not the game's code).
- half as i's later web + loop counter j: 105-122 (by j's place); + drops through q: 82.
- From 82: every single declaration move and `y = cy`: none below (moves that matter: kind early,
  q before cy2, m and lx before j, j before cy before tx, cx late); every local's integer type
  none below; top s32/int/u16 166-168 (code changes); loop counter as a later web of cx/sx/kind
  122; first loop on j and the flag in i's first web 122; cy2 as cy's or top's later web 82;
  `top = top + 0x8C` 82, `top += 0x8C` 184 (code changes).
- perm_nu from 82 (nm2, 15 min, stopped): `j = sx + 0xA0` for the centre x: 62 (applied).
- From 62: every single declaration move and `y = cy` none below (122 ties); p for the drops 85;
  kind u32 62, s16/u16 282; idx u8/s32 63; j u32 63; sx/tx u32 62.
- From 62 with `y = cy` fixed: hill climb 83 -> 78 (kind after sx), nothing below 78.
- From 62: no sx (x + 0xA0, x + 0x28) 122; s16 sx 122; int sx 62; top as kind's later web 82
  (code changes), as cy2's first web 62.
- perm_nu from 62 (nm3, 17 min, stopped): only meaning changes (centre x or loop on i), a reload of
  m[0x75], or y copied in one branch only; none kept.

## Next ideas

- None: matched (session 4).

## For the lead

- New lever (round 30, session 2): the order of independent statements in one block can change
  the registers while the final code stays the same, because MWCC colours on its first
  scheduling pass's order. Here `lx = tx; p = gFieldGuideTables[kind][idx]; top = cy;` (p before
  top) instead of `lx = tx; top = cy; p = ...;` took the build from nine saved registers to eight
  and, with declaration hill climbs, 62 -> 27; with top kept in y and the order y, p, lx: 4
  (every register right). Worth a catalogue line under "Saved registers in
  another order" (try the orders of the statements between two calls before the loop or call
  where the registers go wrong).
- regalloc_predict.py treats fp (s8) webs as float registers (`r[0] == "f"` in classify and
  the adjacency); the fix is `(r[0] == "f" and r != "fp")` (scratch qdfg/rpfix/).
- Round 29's 44-word block changed what the function does; round 30's blocks restore the meaning.
- Session 3: the extension MWCC schedules first in the build is the one its first pass takes
  first (the one that is its source's last use, otherwise the earlier created; `lx = sx + 0x28`
  is an exception to the source order, scratch q5/ord.py), and that order decides the
  colouring; a statement order that only moves the final schedule was not found here.
- Session 4: the match came from tx and lx holding the title line's x values first (dead
  variables reused before their later values); in the colouring that moved tx and lx's first
  webs to the title, which fixed the x/y chains and the tx/lx pair together. Worth a catalogue
  line beside "A dead variable reused for an intermediate value" (Font_AddGlyphSprite).
