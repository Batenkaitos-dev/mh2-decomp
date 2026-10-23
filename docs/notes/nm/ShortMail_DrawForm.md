# ShortMail_DrawForm (lobby, src/lobby/short_mail.c)

## Status

Matched in round 30's fifth session (slot s6): 0 words, the NON_MATCHING block removed. The
form: `s32 ly = (s16)(sy + 0x60);` for block 2's row (a value of its own, numbered at its
declaration), the body box's row in a second local `s16 by`, `s32 f1 = flags & 1;` declared,
declarations c, t, cy, f1, ly, f2, sy, sx, m, buf, tx, s, by; the title string through
`void *t` (no casts; `const char *t` with its two casts also matches); the format strings as
literals ("%s%s", "%s"), since the file owns its .rodata.

## Handoff

Nothing left. The rule found (below, "Fifth session") is proposed for CONTRIBUTING in "For the
lead".

## Fifth session (s6): the colourer's rule and the match

- Black-box tests (scratch sm5: mt.py compiles a test file with the build's
  flags; run.py SPEC prints each test's colour order; specs s1.py to s12.py). Cliques of n values
  live across calls, each passed to a final call:
  - 8 values: no wait, all go in one pass (reverse numbering, last numbered s0); 9 values: the
    two last numbered (fewest references) are pushed and take s7, s8. So the limit is **fewer
    than 8 neighbours**, not 7.
  - CONTRIBUTING's 7 came from a hidden neighbour: **the zero register and sp count as
    neighbours** of every value defined before their last read. A small constant (`li`, `ori`
    from zero, `move x,zero` for 0), a compare with zero (`beqz`/`bnez`, `xor x,y,zero`) or a local
    array's address (`addiu x,sp,N`) at the end of an 8-clique makes it wait (one push); the same
    read before the values are defined does nothing; a `lui` constant, `blez`, `negu`, `andi`,
    `slti` do nothing. zero and sp count once each (7 values + both: one push). They are never
    pushed and take no register.
  - The parameters' registers (a0 live until a late read) do not count (s12.py Q1-Q6).
  - Temporaries count (the `lui at` of each global access: s11.py P5-P8), as CONTRIBUTING says.
- Replay (sm5/eval6.py on the fourth session's schedule-off listings and rp4.py graphs; nat.py
  scores the natural numbering): with zero (live to the end) and sp (live to the last `buf`
  print) as permanent neighbours, the limit 8, cy0 and cy1 one node (`cy += 0xC` inside the call
  argument), and ly's first web numbered after the locals (before x + 0xAA), the natural
  numbering reproduces all five builds: 19/19 each (committed, g25, s36, s32, e25). Without sp:
  28/95. Checked on the compiler: ly's declaration at every place in the committed state and in
  s36 changes nothing (its webs have one definition each, so `ly` is substituted).
- The game's numbering (sm5/gsearch.py, hits.py): with params first, the game's registers need
  c < ly < f2 < sy < sx < m, t < cy < sx, f1 < sx, so ly must be numbered at a declaration. Then
  sel waits alone for the last pass (coloured first, s0) while the others go in one cascade.
- Levers tried on the compiler: splitting ly into two locals (s16): no change (each assigned
  once, still substituted); `(ly = sy + 0x60) - 1` inside the call: every register right but the
  second use re-extends ly (171); `s32 ly` with that assignment: 9 (only the argument order of
  that call left); `s32 ly; ly = (s16)(sy + 0x60);` as a statement: MATCH. f1 as a CSE again: 64;
  plain `char *t`: 85; `void *t` without casts: MATCH; t declared first, by before s: MATCH.

## Fourth session's handoff (kept for its details)

- Probes (one more use of a value, in scratch): an added use of sel, m/p, sx, sy, t, cy or f2
  changes no register on any of the four states (committed, g25, s32, s36). Only f1, flags,
  x + 0xAA and x move registers: they are the values the colourer pushes. On g25 one more use of
  f1 (or of x + 0xAA) in block 1 gives sel s0, but f1 s1 (x + 0xAA: s1). The game's code reads
  f1 twice and x + 0xAA twice, as ours does, so no natural extra use exists (the brief's ruling).
- `#pragma schedule off` gives the same title registers as the scheduled build on all five states
  (committed, g25, s32, s36, e25), so the colouring can be studied on creation-order listings
  (sm4/odis.py FILE.c OFF SIG).
- Replay on those listings (sm4/rp4.py; the argument values built straight in a0-a3 by an
  extension pair are not counted as temporaries): the graph admits both colourings. A forced
  removal order reproduces our build (x c2 c3 tx s sel3 ly2 cy1 c1 ly1 xAA flags f1 sel t sy sx
  m f2 cy0) and one gives the game's (... xAA flags f1 t f2 cy1 sy cy0 sx m sel). sm4/cons.py
  prints the colour-order constraints. So the game needs **sel removed last** (coloured first),
  then m, sx, cy, t; ours removes sel right after the three pushed values.
- But no rule set with natural numbering reproduces our builds: params first, locals in
  declaration order, created and later webs in any order. Tried: push by fewest refs, most
  neighbours or refs/degree; pass forward or backward; restart after each removal; pushes that
  keep neighbours' degrees; limit 6/7/8; cy0 numbered as a created value. Best 32 of 100
  registers over the five builds (sm4/eval5.py). A free numbering fits each build (srch5.py), but
  the fits put c1 and ly1 before the parameters, which no declaration does.
- Next, in order: (1) black-box micro tests for the core question: in a clique of 8 values live
  across calls, which of a parameter, a local and a CSE (`flags & 1`) is removed last, and what
  makes a parameter outlast the locals (in the p-direct states p and sel are coloured first);
  (2) with that rule, replay eval5.py's five builds, then ask for the game's numbering;
  (3) only then sweeps. The probe hint: what the game needs is g25 with sel and f1 swapped in
  the colour order (sel removed last, f1 third).

## Third session's handoff (kept for its details)

- From the second session: the first string is held in a const view `const char *t` (casts at
  its two uses; `void *t` gives the same registers without casts); a plain `char *` copy is
  numbered as its load (24). p used directly gives sel s0, p s1 in every order tried; `s32 f1`
  declared after f2 and cy gives f1 s5.
- Not the stripped-function effect: the block has no runtime calls (0 FPU ops), and compiled
  alone (lines 1-10 and 283-390 of the file, without ShortMail_Step/Input) it gives the same
  listing as in the whole file.
- The states: committed (22: sel s4); e25 (25: sel, x/sx, cy right; m s4, f1 s1, t s5); s32/s36
  (p used directly and `s32 f1` declared: sel s0, p s1; s36 has t s2, cy s3, sx s4, s32 has f1
  s2, sx s5). Files in sm3 (e25.c, s32.c, s36.c: whole unit files).
- New state g25 (25 words; sm3/g25.c): m kept, `s32 f1 = flags & 1;` declared,
  order f2, sy, c, sx, m, f1, t, cy (then ly, buf, tx, s). m s1, x/sx s2, cy/sy s3, t s4, f2 s4,
  flags, xAA, c1 as the game: the whole title in the game's order, except **sel s5 and f1 s0**
  (swapped; ly1 s0, ly2 s1, c2 s0 follow from sel). f1 declared last (after cy, "g7", 65) gives
  f1 s5 but sel s4 under m s0, sx s1, cy s2, t s3: the game's order with sel coloured last
  instead of first. With m, sel is never coloured first; with p used directly (s32/s36, the
  5040 orders of the second session) sel s0 and p s1 but the title goes wrong. What is left:
  sel coloured first while m (or p) stays second.
- New facts this session: `sx = (s32)x` (explicit cast) makes sx a created value: its
  declaration position stops mattering (s36 65, s32 60, committed 47 at every position; 720
  orders of c, f2, cy, f1, t, sy on s36 with the cast: best 48). Without the cast sx is
  numbered at its declaration (s36: sx at 0-3 86 with sx s0, 4-5 36, 6-10 65).
- The replay: sm3/sim.py rebuilds the game's webs (regalloc_predict's Func) and our build's
  (build_ours, webs named after the game's), merges webs and replays the passes with a choice
  of spill pick. Fewest references (CONTRIBUTING's rule) reproduces at most 13 of 20 game
  registers under any merge; "most neighbours left" reproduces all 19 when c10+cy0, tx0+tx and
  cy1+sy are merged (orders with f1, f2, t < cy1 < sx). But neither rule reproduces our own
  build: with our declaration order 4/20 (refs) and 10/20 (most neighbours), and the best order
  any search finds 15-17/20. So the graph itself (liveness from the final listing, the
  temporaries) is off, not only the spill rule; the replay can't be trusted for this block yet.
- Next ideas, in order: (1) make the replay reproduce our own builds first (5-10 variants with
  known declaration orders; chk.py and srch_ours.py), e.g. liveness from `#pragma schedule off`
  listings (creation order) instead of the final schedule; then ask it for the game's order;
  (2) a form where cy1 and sy are one value (the "most neighbours" replay needs that), with the
  game's in-place re-extension kept (cy used directly changes the shape: 154-174).

## Second session (s2) tried (func_diff words)

- t as const view, each of 11 places on the committed order: 22 before cy, 24 after; the whole
  of s (both strings) as const char *: 28-30. Types of t: void * (no casts), const void *, u8 *,
  s8 *, u32, s32, `(char *)D_0080C0DC`: 22 as const; plain char * 24.
- 5040 orders of t, sy, sx, m, f2, c, cy (ly, buf, tx, s after) on 22: best 22; sel s0 in 120
  orders, all with c < f2 < sy, sx, m; best of those 25 (e25: c, t, f2, cy, m, sy, sx: sel, x/sx,
  cy, flags, xAA right; f1 s1, p s4, t s5). 23892 of the 40320 orders with s moved too: best 22.
- On e25: f1 declared at each place 25-47 (33 after f2: f1 s5 but p s4, cy s1); single moves of
  every declaration 25-80; `cx = x + 0xAA` (s32, any place): 25 (substituted); statement moves
  (t/sx swap, m first, sy earlier, f2 at the top or early): 25-149.
- p direct, f1 declared (cpf.c): 5040 orders, best 32 (s32.c), pattern 36 (s36.c); f1 as CSE
  (cp.c, 5040 orders with ly): best 37. Block-scoped t (and s in block 3): 35, sel s4.
- On s32/s36: separate c2/c3/ly2 locals (4 places each) same; types of sx, sy, f1, f2, c, cy,
  ly, tx (u32/s32/u16/s16) same or other shape; s16 sx 235; x used directly 189 (x raw kept
  live, 9 saved registers); `x = (s16)x; sx = x;` 86 (x/sx one value s0, sel s1), its 5040
  orders: sel never s0; first string as s's first web (`s = (char *)D_0080C0DC`): same as t.
- Permuter (perm_nu): 15 min from e25 (base 630) and 16 min from s36 (base 475): nothing better.

## Third session (s6) tried (func_diff words)

- Compiled alone (no ShortMail_Step/Input before it): same listing. No runtime calls in the block.
- `sx = (s32)x`: sx's declaration stops mattering (s36 65, s32 60, committed 47; with the cast, 720 orders of c, f2, cy, f1, t, sy on s36: best 48). `sy = (s32)cy`, `cy = (s16)y`, both:
  no change on s36/s32/committed/e25. `sx + 0xAA` in either Text_CenterX: 113-295 (shape).
- Without sy (cy used): 154-174; without cy and sy (y += 0xC on the parameter): 168-180;
  `y += 0xC` with `sy = y`: 301 (shape); x used directly: 221.
- On s36, single moves: sx at 0-3 86, 4-5 36, 6-10 65; t at 0-2 37 (t s5, sx s4, f1 s2), 3 49;
  cy at 3-10 64-76; sy at 0-5 65-86.
- e25 with p direct: 60; plus f1 declared at each of 11 places: 57-83.
- m and f1 declared: 336 orders with f1 < t < cy < sx < m (c, f2, sy anywhere): best 34; 1500
  random orders of c, t, f2, cy, m, sy, sx, f1: best 22 (committed pattern), 24, 25 (g25 and
  four others); around g25 single moves of each of the 8 (57 variants): 25 best (g_f1@7 65).
- g25/g7 with `m = (char *)p;` placed later (after the if, Icon_Draw, Font_SetColor, the title,
  the first row, before Sjis_Widen): 72-160.
- Two definitions (`t = ...` or `sx = x;` twice, adjacent or the first at the top): no change.
- Permuter (perm_nu): 13 min from the committed state (base 600): its finds change the shape
  (a `long long` f1, 308 words; f1 held in tx, 308). 25 min from g25 (base 640):
  480 (c declared first, 71 words), 495 (that and p in the two Sjis_Widen, 74). Nothing usable.
- The replay (sm3/sim.py, sim_run.py, chk.py, srch_ours.py, seeds.py): see Handoff.

## Fourth session (s6) tried (func_diff words)

- Probes, one more use of each value (as Icon_Draw's last argument, Menu_DrawSideArrows' flip
  or Text_CenterX's width): on the committed state sel, m, f1 (as `flags & 1`), cy, sx, sy, f2
  and t give no register change (scores 23-304 only from the code added); p used once (70),
  flags (60, 217), f1 in block 1 (229), x + 0xAA (229), x (236-307) move everything. Same on
  g25 (m_f1 224: sel s0, f1 s1; m_xAA 228: sel s0, xAA s1), s36 and s32 (m_f1 on s32 218: sel,
  p, f1, flags, x + 0xAA right; x s3, sx s4, t s3, cy s2).
- Step 3 levers: tx given a first value at the title (`tx = Text_CenterX(...)`, then
  `Font_SetPos(tx, ...)`): 281-285 on all five states (the shape changes). `c = 0x80301010;
  Font_SetColor(c);` at the first colour: no change (constant propagated). Short webs (ly, tx, c,
  s) declared first, all 24 orders, with and without the tx form, on the five states: base 80,
  the others unchanged (g25 25, s36 36, s32 32, e25 25).
- A copy `s16 cur = sel` (cur += 1, cur compared), declared at each place, before or after
  `cy = y`, on the committed state and on g25: 28-69; the copy is extended at the entry
  (dsll32/dsra32 from a1, where the game moves a1 raw), and sel takes s0 only when cur is
  declared after m (58, 69: cy s1, m s2, sx s3).
- Scratch sm4: odis.py (our listing, scheduled or OFF, SIG = key registers
  against the game's), rp4.py (graph from an OFF listing; RP4=argext=1 and other options),
  forced.py and cons.py (colourings from a removal order; colour-order constraints), fit.py,
  hyp.py, srch4.py, srch5.py, eval5.py (rule searches), show.sh VARFILE BASE (variants, one line
  each), p1*.py probes, gen_lev.py, gen_cur.py; *_off.lst listings; g25/s32/s36/e25.c grafted
  onto the current file.

## For the lead

- Fifth session: matched. Changes are inside the function and its own declarations (the two
  format-string externs D_00838450/D_00838458, used only here, are now literals, as the file
  owns its .rodata). `void *t` follows the ruling (fewer casts); `const char *t` with casts also
  matches if the lead prefers the const view.
- Proposed for docs/mwcc-matching.md, "How MWCC picks saved registers", rule 2 (replacing "the limit is
  **7**"): "For a value live across a call the limit is **8**: fewer than 8 other values still
  in the graph. **The zero register and sp count as neighbours** of every value defined before
  their last read in the function: a small constant (`li`, `ori` from zero, `move x,zero`), a
  compare with zero (`beqz`, `bnez`, `xor x,y,zero`) or a local array's address (`addiu x,sp,N`;
  probably also round 12's "more than 8 arguments": the 9th is stored through sp). Constants appear in most functions up to the
  end, so the effective limit is usually 7 (round 12's calibration); sp's read ends at the last
  use of a stack buffer, so values defined before it have one neighbour more than values defined
  after it. They count once each, are never pushed and take no register. The parameters' own
  registers do not count. (ShortMail_DrawForm; tests in docs/notes/nm/ShortMail_DrawForm.md.)"
- Also for docs/mwcc-matching.md, "Which locals are values of their own": "A variable whose webs each have
  one definition (`ly = sy + 0x60;` in one block, `ly = sy + 0x8C;` in another) is substituted:
  its first web is numbered after the locals, wherever `ly` is declared; two separate s16 locals
  are still substituted. `s32 ly = (s16)(sy + 0x60);` (an int holding the narrowed value) is a
  value of its own, numbered at its declaration, with the same code (ShortMail_DrawForm)."
- Fourth session: nothing outside the function changed; no form for a ruling. The replay's
  open question (what makes a parameter outlast the locals in the colourer) needs black-box
  tests, which may be worth a round of their own.
- Nothing outside the function changed. The committed form uses a const view with casts at the
  uses (the callees take char *); `void *t` gives the same registers without casts.
- Scratch (third session): sm3 (vr.py and sig.py copied from sm2; vh.py
  helpers to build variant bodies; short.sh VARFILE prints one line per variant; g25.c; sim.py).
- Scratch: sm2 (vr.py variant runner, SHOW=2 prints every value's register
  with the game's in brackets via sig.py; apply.py VARFILE NAME BASE OUT; e25.c, cpf.c, cp.c,
  s32.c, s36.c the states above; p7/p8/p9/p10/p12/p13 logs of the sweeps; mg.py the replay).
