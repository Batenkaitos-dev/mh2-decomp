# Kbd_DrawSpecialKey (sub_main, src/sub_main/unk_314E00.c, level 4)

## Status

- **Matched** (round 30 session 4, slot s2). The head as the game has it:
  `kc = gKbd->col; kr = gKbd->row; cur = (kc << 2) + kr; ofs = gKbd->mode * sizeof(KeyPos);
  row = (&gKbdSpecialKeyPos->row)[ofs]; col = (&gKbdSpecialKeyPos->col)[ofs];` with `s32 ofs`
  declared before cur (and the unused `u8 mode` gone).
- Why it works: the game's mode << 1 (t1) is a declared local's first web (the byte offset), so it
  is numbered before the index and leaves the colouring first. The index then has 22 neighbours
  at its turn instead of 23 and does not wait, and is coloured after gKbd, kc, mode (t5). The
  two `(&tbl->field)[ofs]` reads keep two bases (sym, sym+1), and with the reads after the index
  the first pass does kc << 2 and the index first (CF), as the game does. `ofs` declared after
  cur: 71 adiff (the index waits again); the reads before the index: 10; `* 2` and `<< 1`, u32 ofs:
  the same match.
- It came from the analysis in "session 4" below (item 2): the index's 23rd neighbour had to leave
  first, and the only candidates not alone in their register were the t0/t1/a2/a3 groups.

## What round 30 found (scratch k: gen.py compiles many variants of the
## function in one file and scores each with adiff; `--cf` flags "kc << 2 scheduled before
## mode << 1" (CF/mf), the number of table bases (b1/b2) and whether the two shifts share a
## register (SAME/diff); mk.py writes a variant into a copy of the real file)

1. The game's colouring follows from a first pass that does `addu cur` before `sll mode2`
   (the two shifts then share a register, and the second pass orders kc, kr, mode loads by
   height through that reuse: exactly the game's listing). Rule 0 (heights) puts mode first;
   creation order alone does not move it (cur created first, ABCRL: still mode first).
2. **The number of table bases decides it.** Every form whose two table reads share one base
   (`KeyPos *kp = &tbl[m]; kp->row; kp->col`, `((u8 *)tbl)[m * 2]` / `[m * 2 + 1]`, an inline
   returning `&tbl[m]`, a byte-offset local) gives cur first with the game's registers: 13
   adiff words, the only difference the address (one lui/addiu base and `lbu a2,1(a2)` where
   the game has two bases, sym and sym+1, each `addu base,base,t1`). Every two-base form
   (`tbl[m].row` / `.col`, `(&tbl[m])->col`, `(&tbl->col)[o]`, mixed pointer/array: MWCC CSEs a
   mixed pair into one base) gives mode first, in every head order but three.
3. Two bases with the head order kc, kr, row, col, cur (ABRLC; also ABLCR, BARLC): the game's
   schedule exactly (lbu kc, kr, mode; sll kc4; addu cur; sll mode2 in one register; ...), but
   every register shifted (gKbd t5, cur t0, li 13 t1, row base t2; game t4, t5, t0, a3): 71
   adiff words, 118 func_diff. With schedule off the same source gets the game's registers, so
   the numbering is right and the first pass differs. cur behaves as numbered late (coloured
   before li 13); in the game it is coloured after gKbd, kc, mode (as a declared local's first
   web). Declaration order of cur and c (896 variants), a separate tail variable, in-place cur
   forms, kc/kr/cur types: no change (scratch v14-v18).
4. Reusing a variable (`o = kc << 2; cur = o + kr; o = mode << 1;`) is not what moved the
   order in the first tests: it was their one-base byte-offset address (v10: the same with no
   reuse gives the same listing).
5. Unused declared locals change nothing (the old `u8 mode;` declaration).

## Tried this round (adiff words; func_diff 9 = adiff 10 for the current source)

- 400 head orders x mode local (none/s32/u8) x cur form (v12): best 10 (the current order).
- Inline helpers for the index or the table reads (v3, v20): 10-92; one-base helpers 11-18.
- kc/kr/cur types (v2): 10-45. Struct copy `KeyPos p = tbl[m]` (v6): stack copies, 102-119.
- One-base form plus an extra live base in the head (`bs = gKbdBoxSizes`, v23): still cur
  first, so register pressure is not what flips it. `rp = &tbl[m].row; cp = &tbl[m].col`
  and kp/array mixes are CSEd into one base (13); `(&tbl[0].row)[m * 2]` keeps two (67, mode first).
- Permuter 35 min (-j2) from the ABRLC block (scratch k/abrlc_blk.c), stopped at about 20 min
  (budget): base 770 (118 words) -> 30 with kr reused for cur (`kr = (kc << 2) + kr;`, so
  `col == kr` compares the index: invalid, as in round 23); 75 with a second `cur = ...` in
  case 5, 80 with a copy of cur in case 13 (contrived). Outputs in nonmatchings/ (git-ignored).
  The 30 hints that cur wants another numbering there (a later web of a variable): a dead s32
  variable the C allows, holding the index, is the next thing to try.

## Round 30 session 2 (s3): what it found

1. **The register shift of the ABRLC heads is the index's degree.** ABRLC (kc, kr, row, col,
   cur) with case 13's `== 6` changed to `== 0` (one value less, li 6, live beside the index):
   gKbd t4, kc t3, mode t2, li 13 t0, cur t5, the game's (17 words, the rest sym/sym1/kc4).
   Removing case 13 or case 5 does the same. So with the CF first pass the index has 23
   neighbours and waits ("Values with 23 or more neighbours"), coloured early (t0), shifting
   li 13, sym, mode, kc, gKbd up one. The game colours it last (t5) with the same first pass
   and the same case code, so in the game it has 22 neighbours when its turn comes.
2. **ABKCRL** (above, 13 words): the index as cur's later web, kc a copy of cur's first web
   (the col load), computed before the table reads. Its first pass is the game's (kc4 and m2
   share t1) and sym/sym1/kc4/m2 colour as in the game (in ABRLC they don't). Only the
   index/gKbd pair is swapped. Removing li 6 there changes nothing (so it is numbering, not
   degree). Its statement order matters: ABKCRL, AKBCRL, BAKCRL all 13; the table reads before
   the index: 21-27; `cur <<= 2; cur += kr;` (in place), `cur = kc; cur = (cur << 2) + kr`
   (copy the other way), a separate s32 i for the index, `kc = gKbd->col` CSE'd instead of
   the copy: all mf (67-92). Declaration orders (120), cur/kc types, `cur * 4`, operand order,
   a `KbdEntry *k = gKbd` local: no change (13 or worse; scratch w12, w13, w16, w18).
3. **The permuter's kr reuse** (`kr = (kc << 2) + kr;`, invalid) measures 6 adiff words in the
   ABRLC head (scratch w4): every register right but the index and kr (index v1, kr's load t1).
4. **Declarations do nothing in these heads.** 120 declaration orders of cur/row/col/kc/kr in
   ABRLC; kc, kr, row or col also given a later web in the tail (so they stay variables) with
   kc declared before cur (640 + 240 variants, scratch w15, w17, w19): all 71 or worse. The
   "a neighbour numbered before the index leaves first" route (which would let the index go at
   22 neighbours with the game's registers) did not show.
5. Other forms with no gain: re-reading the table in the cases or tail (no cross-block CSE of
   the loads: 42-146), the mode load held in cur before the index (`cur = gKbd->mode;` then
   the reads, then `cur = (kc << 2) + kr`: same as without), index held in a copy of the kr
   load (x1-x4: 71), `cur = gKbd->col; ... kc = cur; cur = (cur << 2) + kr` with the reads
   before the index (x5: 22, gKbd/index swapped as ABKCRL plus sym/kc4).
6. In ABKCRL the swap is numbering, not degree: removing li 6, or case 5, or li 6 and li 4
   leaves gKbd t5 / index t4 (scratch w20, w24). A block-scope cur for the head and switch with
   its own s32 for the tail index (natural C), a separate tail variable, `register`: all 13.
7. Permuter 30 min from ABKCRL (base 65, -j2): its only finds below 65 change what the code means:
   `cur = gKbd->col;` inserted in a case before `grid[cur]` (5, three times: the head index is then
   not live into case 5 and every head register is the game's; only case 5 differs), `color`
   reused for kc (35). So a change to the index's webs elsewhere renumbers it.

## Round 30 session 4 (slot s2): what it found (scratch k3, gen.py variant files a1-a9, b1)

1. **ABKCRL and ABCRL differ only in the index's web, yet their first passes differ.** With
   `#pragma schedule off` the two compile to the same instruction order (lw gKbd; lbu col, row;
   sll; addu; lbu mode; ...), only gKbd/index registers swapped. Scheduled, ABKCRL (index =
   cur's later web) is CF and ABCRL (index = cur's first web) is mf. So the first pass depends on
   the index's numbering, not only on creation order; ABRLC (kc4/index created after the table
   reads) is CF too. In every form found, CF comes with the index (or kc4) created or numbered
   after the mode chain, and the game needs the index numbered first.
2. **The game's colouring order** (tempcolor --no-call-rd, final listing): kr waits (v1); then
   numbering index < gKbd < kc < mode < kc4 < li13 < sym/sym1/row/col chains. The index's 23
   neighbours there: sym, sym1, gKbd, li13, kc, kr, mode, m2, sym+m2, sym1+m2, row, col, a0, a1,
   li5, li4, case 4 (lw, lw), case 5 (lw, lw), case 13 (lw, li 6, lw). With a CF first pass every
   head overlap is forced, so in the game either a chain/case value does not overlap the index in
   the first pass, or a neighbour is numbered before the index and leaves first. That neighbour
   can't be kc, mode, gKbd (sole holders of t3, t2, t4); it must be one of li13/case 4/case 5/li 6
   (t0), m2/li5/li4/case 13 lw (t1), sym/sym+m2/row (a3), sym1/sym1+m2/col (a2).
3. Tried, no gain (adiff; CF/mf as gen.py --cf):
   - Casts or copies on the shift's operand (`((u32)kc << 2)`, s32/u16/u8 casts, `t = kc` of
     u32/s32/u8): all propagated, the same code as ABCRL (67 mf). Initialiser casts (s32, u32) on
     kc, kr, cur in the current, ABRLC and ABKCRL heads, 81 variants: 10 / 71 / 13, unchanged.
   - A reused variable with the index in another: `k = col; kc = k; cur = (k << 2) + kr; k = mode`
     (switch/table on k or gKbd->mode, k declared first/last), `i = (cur << 2) + kr` with cur
     then redefined (mode, i): 67-92 mf. CF needs exactly `cur = <expr of cur> + kr` on the col
     load with a live copy: `cur * 4 + kr`, `kr + (cur << 2)`, kr read first: all 13 CF (ABKCRL).
   - No kc/kr locals (gKbd->col/row at each use, CSE'd): index before the reads 67 mf, after 13
     mf (mode t3, kc t2); ABKCRL with `+ gKbd->row` or `+ (kr = gKbd->row)`: 13 CF.
   - A mode local (u8, s32, u32) loaded before the index in 4 head orders, switch on it or on
     gKbd->mode: the same code as without it (propagated): 13 mf (mode first) or 67 mf.
   - kc4 as cur's later web, index a first web (`cur = col; kc = cur; cur <<= 2; i = cur + kr`,
     i declared first or last): 67-92 mf; with the reads before `cur << 2` it is ABRLC (71).
   - s32/u32 row or col declared before cur in ABRLC (a neighbour that leaves first): 56-86.
4. Permuter from the one-base state (scratch k3/ob_blk.c): 460 -> 450 in 8 min (the index before
   the kp reads); stopped at the match.
5. **The match** (scratch k3/b2.py): the t1 group's m2 as a declared first web: `s32 ofs` before
   cur, `ofs = gKbd->mode * 2` (or `<< 1`, or `* sizeof(KeyPos)`), `(&tbl->row)[ofs]`,
   `(&tbl->col)[ofs]` (or `((u8 *)tbl)[ofs]` for row) after the index: 0. The same reads before the
   index: 10 (CF, sym t1 and kc4 a2); ofs declared after cur: 71 (the ABRLC shift); ofs = mode
   (a declared mode, a sole holder of t2): 67-71.

## For the lead

- Model finding for docs/mwcc-matching.md ("Values with 23 or more neighbours"): a value at the threshold
  (here the cursor index, 23 neighbours, numbered first as a declared local) stops waiting when one
  of its neighbours is made a declared local's first web numbered before it: that neighbour leaves
  the colouring first and the value's count at its turn drops to 22. Pick a neighbour that is not
  the only holder of its register in the game (here mode << 1, which shares t1 with the chain's
  li 5 / li 4): it is coloured after the value and still finds its register. Here that neighbour
  was a byte offset `ofs = mode * sizeof(KeyPos)` used for two field reads, which also keeps the
  game's two table bases.

- Nothing outside the function. ABKCRL (session 2, 13 words, scratch k/abk_blk.c) is the
  nearest structure so far: every value but two in the game's register. Model finding for
  docs/mwcc-matching.md ("Values with 23 or more neighbours"): a value at the threshold can be pushed
  over or under it by one compare constant in another block (case 13's `li 6` here), and the
  wait then shifts every register coloured after it by one.
