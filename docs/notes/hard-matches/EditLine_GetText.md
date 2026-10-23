# EditLine_GetText (sub_main, src/sub_main/unk_2FB0C0.c)

## The match

The game's registers: dst s3, mode s2; part 1 a s4, b s1, s s0; part 2 the walker s5, end s1,
c s4, prev s0, c's extension s6.

- Part 2 walks with the first part's `EditEnt *p`:

      p = (EditEnt *)gConvEditLine + start; end = p + n;
      for (; p < end; p++) { c = p->code; ...

  with end an `EditEnt *`. This makes the walker a later web of p (rule 1 of
  docs/mwcc-matching.md, "How MWCC numbers the values later passes create"), numbered after c: c s4, walker s5.
- dst and mode are plain parameters (no dst copy, no mode local). Declaration order of the
  locals: p, end, i, a, b, s, c, prev (31 of 101 sampled declaration orders of p, i, a, b, end,
  prev, c, s match).
- gConvEditLine is declared `u8[]` and read here as `EditEnt *` through a cast; typing the
  global as `EditEnt gConvEditLine[]` would change its other readers (e.g. EditLine_ToCells)
  and was not tried.

## How it was found (56 words -> 31 -> 0)

1. Probe (56 -> 31): mode as a local of its own (`u32 mode_in` parameter, `s32 mode =
   mode_in;`, or an s32 parameter with `const s32 mode = mode_in;`) declared after a and b
   gave part 1 exactly (mode s2, dst s3, a s4). A plain `s32 mode = mode_in` copy of an s32
   parameter is not the same (9 words with the final part 2). This showed that the reading "a
   local numbered before both parameters" was a symptom, not the lever: see step 3.
2. Part 2 (31 -> 0): with a `u8 *e` walker, c (a u16 holding a loaded halfword, numbered as
   created) got s5 and e s4 in all 450 sampled declaration orders. Walking with the first
   part's `EditEnt *p` (above) gives c s4, walker s5 at once. A separate `EditEnt *q` (a first
   web) stays at 31.
3. With that part 2, dst and mode are plain parameters again: a dst copy had only compensated
   for the wrong part 2.

## Evidence (part 1 / part 2 words)

Mode local, `u8 *e` walker, best of 450 orders 31 (0/31); mode the parameter, `u8 *e` walker,
200 orders 28 at best (13/15); dst parameter + u32/s32 mode local + p walker 0 at mode
positions 0-4; const s32 copy 0 at positions 1-5.

The variants were compiled together in one file after the file's head, each scored by
func_diff words for part 1 / part 2 and the registers of dst, mode, a, b, s, e, end, c, prev
and c's extension, with sampled declaration-order sweeps.

## Method

For a walker over a struct array that the function also walks in an earlier part with a typed
pointer: try the same pointer variable (a later web) before reaching for numbering levers on
the parameters. Here the "local numbered before the parameters" was the wrong part-2 walker
shifting every colour.
