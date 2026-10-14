# MovieSfd_FramePacket (sub_main, src/sub_main/movie.c)

## Status

Matched in round 30 (s5, branch decomp-b30-s5): 7 words at the start, 0 now.
Every function of movie.c matches; ninja (13 OK).

## What matched it (round 30)

```c
    i = 0;
    v = gMovieTexWidth;
    while (v > 1) { r = v % 2; v = v / 2; i++; }
    if (r != 0) { i++; }
    tw = i;
    for (i = 0, v = 1; i < tw; i++) { v <<= 1; }
    if (v < gMovieTexWidth) { tw++; }
```

The game numbers the unrolled bound `tw - 8` (t5), then the width (t4),
then the counter (t2), then the shifted value (v1) (tempcolor.py).

1. The width as a CSE of `gMovieTexWidth` (no wt local): a reused global
   load is numbered after the unroller's bound (11 words alone: the
   counter and shifted value are then still numbered first).
2. The counter and the shifted value as later webs, the counter's variable
   first in the code: `i` counts the halvings (`i = 0` is the first
   statement, before `v = gMovieTexWidth`), `tw = i` after, then the power
   loop reuses i and v. Later webs are numbered after the bound and the CSE,
   in the code order of their variables' first webs: i, then v. MATCH.

## Why the earlier tries failed

- r is read unset when the width is below 2 (the game's own read of t6).
  A variable read before it is set on some path is one web: MWCC does not
  split it (scratch t/e.c: with `r = 0` first, r's power-loop use is a
  separate web in another register; without, it shares r's register). So
  round 29's counter "as a later web of r" made no later web.
- th, th2, ofs have no web before the power loop: the counter is their first
  web, numbered at the declaration.
- Block-scope counters are numbered after the function's locals but still
  before the unrolled bound (11).

## Tries this session (scores)

- No wt (CSE): 11. Counter/shifted over i, v, r, th, th2, ofs with the CSE:
  11-137 (gen2). Halving variable, remainder, counter, shifted over
  v/r/i/wt/th/th2/ofs, width as CSE or wt (gen3, 1944 of 2664 run): 7 best.
- `u32 wt = gMovieTexWidth; v = wt;` then wt as the counter with
  `(s32)wt < tw`: 6 (counter right; the width numbered at wt's declaration,
  before the bound). A u32 copy of an s32 value is not substituted away (a
  real first web); the s32 copy is (11).
- The x1 form above: 0.

## For the lead

- docs/mwcc-matching.md, "How MWCC numbers the values later passes create", rule 1:
  two additions from this block.
  - A variable read before it is set on some path (an uninitialised read
    the game itself has) is not split into webs: all its definitions are
    one value, so it cannot give a later web (scratch t/e.c, E0 vs E1).
  - Later webs are ordered by the code order of their variables' first webs,
    not the declaration order (scratch t/d.c: 8 combinations of declaration
    order, first-web order and roles; only the first-web order decides).
  - The unroller's bound is numbered after every declared local (block scope
    too) and before a reused global load (CSE) and later webs.
- "Which locals are values of their own": a u32 local copying an s32 value
  (`u32 wt = gW; v = wt;`) keeps a value of its own where the s32 copy is
  substituted (6 against 11 here).
