# ModeSel_DrawButton (select, src/select/select_title.c, level 4)

## Status

Matched (round 30, s5): 38 -> 0 words. Unwrapped; every function of the file matches (34);
ninja (13 OK). Natural C, no FAKEMATCH tag.

## What matched

- `sx` (the saved spr.x of the shadow pass) is an `s32`, declared after m, i, ip and before
  pc, py. The declarations are `s16 k; ModeWork *m; s32 i; ModeIcon *ip; s32 sx; s32 sy;
  u32 sc; u32 *pc; s16 *py;` (after spr, icon, r, a, b). No other change.
- Also matching (generator, scratch mds/v10.txt, v11.txt): k anywhere; sy s16 or s32; sx as
  `int`; the sx, sy, sc group anywhere between ip and pc as long as sc is not before sx; and,
  with sx s16, `s32 k; k = (s16)*item;` in sx's place (a cast: not used).

## Why it works (for the models)

- The value that decides it is sx, not k. k has only six saved neighbours (item, pc, py, m,
  i, ip), so it is removed early and coloured late: it takes whatever register those six leave,
  and sx (not a neighbour of k) takes the same one. In the game the six leave s3 because sx is
  coloured s3 between m (s2) and py (s4).
- An `s16 sx` holding the loaded halfword is numbered like a created value, after every
  declared local ("Values with 23 or more neighbours", rule 3): moving its declaration does
  nothing (round 20 and 24's 1440 orders moved an s16 sx, so they never moved it).
- Observed with an s32 sx at each place among pc, py, m, i, ip (scratch mds/v9.txt, 18
  variants, s32/u32/int alike): the saved registers rotate around sx. The values are removed
  (coloured in reverse) as: item, then the values declared after sx in declaration order, then
  sx, then the values declared before sx. Base (s16 sx, numbered last): item, sx, py, pc, m, i,
  ip, i.e. ip s0 ... py s4, sx/k s5, item s6. The game's ip s0, i s1, m s2, sx s3, py s4, pc
  s5 therefore needs "pc, py after sx; m, i, ip before it": the declaration order above.
- This rotation is not what regalloc_predict's simulate() replays (it removes in plain number
  order after the spills of sy and sc, which predicts item, pc, py, sx, m, i, ip for the
  declaration pc, py, sx, m, i, ip; the build gives item, m, i, ip, sx, pc, py there). The
  mechanism (where MWCC's removal scan resumes after the stall) is not worked out.

## Tries (round 30, s5; scratch mds, gen.py variant files v1-v11)

- Declaration moves of k, sx, sy, sc, r, a, b, spr/icon (s16 sx): 38 everywhere, k/sx s5.
- Types: k s32/u16/u8 (code changes, 48), sx u16 48, sy s32 38; s32 sx after m (rotation
  found there: 47); `spr.y`/`spr.color` written directly (hoisted, 72).
- s32 sx at each of the six places among pc, py, m, i, ip, and u32/int: the rotation above;
  m, i, ip, sx, pc, py: MATCH.

## For the lead

- A rule for the models ("How MWCC picks saved registers"): with a stall resolved by spills
  (here sy, sc), the final removal went [declared after X], X, [declared before X] around the
  short-lived local X whose neighbours the spills freed, not plain number order. Worth a
  black-box test before it goes in CONTRIBUTING; regalloc_predict reproduces our builds only
  by fitting other numberings (it put sx before every declared local).
