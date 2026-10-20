# AreaObj_K11_Draw (game, src/game/area_scenery.c)

## Status

- Matched in round 30 (s3), from 5 words. The whole file is MATCH (72 functions); ninja (13 OK).
- The fix: case 2's statements in the order z, x0, zstep, sph[4], xstep, rows, cols, tbl
  (the call last, as before). No fakematch: a plain statement order.

## What it was

The game's case 2 stores the radius (sph[4], lui/ori/sw 224) before xstep (mov.s f23,f22) and
tbl (lui/addiu, its spill store sw 192 in the jal slot), every chain in v0. Ours had tbl first
(source order), and every order of x0/zstep/tbl/xstep/sph[4] with rows and cols kept second and
third gave 5 to 10: with the radius created before tbl, tbl's lui went between the radius's ori
and sw (the ori then in v1).

## Why the order works (a model that predicted all three listings before they were compiled)

The first pass (before colouring) as a cycle scheduler: two issue slots per cycle, the first
slot of cycle 0 taken (the block starts at the case label; rule 1), result latency 1 for ALU
ops and 2 from mtc1 to an FPU op (the `nop` between mtc1 f22 and mov.s f23 with schedule off).
In each slot it took an instruction that ends a value (z.m, r.o, r.sw, t.a) first, otherwise the
earliest-created ready one. Heights did not decide anything in this block.

- Base order (z, rows, cols, x0, zs, tbl, xs, r): chains z, x0, zs, tbl, r one after the other.
- Radius before tbl with rows and cols early: in the cycle of r.ori, r.sw is not yet ready (it
  waits for ori), so the free slot takes the earliest-created ready instruction: tbl's lui. The
  two values overlap, the ori goes to v1, and pass 2 keeps the interleave (6).
- Rows and cols created after the radius (or between zstep and the radius's ori): `li s1` /
  `move s0` are ready and created before tbl, so they take those free slots, tbl's lui comes after
  r.sw, every chain gets v0, and pass 2 moves the s-register and FPU moves up to the game's places.

Matching orders found (scratch c2.py/vtry.py): zxsrRCXt, zxsrRXCt, zxsrXRCt
(committed), zxsRrCXt (z x0 zstep R=rows C=cols r=sph[4] X=xstep t=tbl). zxsrXtRC: 6;
zRxsrCXt: 7.

## For the lead

- A possible line for "How MWCC schedules" (rule 4): in the first pass, when a chain's next
  instruction waits a cycle (a store after its ori), the free slot takes the earliest-created
  ready instruction; to keep another chain out of it, create a short independent instruction
  (a `li` of a counter) between the two chains in the source. Found by simulating the model above;
  it predicted the base listing, the interleaved one and the match.
