# WindowFrame_Draw (sub_main, src/sub_main/unk_356380.c)

## Status

**Matched** (round 30 s2): 482 -> 110 -> 0 words. Unwrapped; the level 4 pragma pair stays
(at the file's level 3 the function is 934 of 990 words off).

## Handoff

Nothing left. How the last 110 words went (from the committed 110 state):
1. `if (h % 17 > 8)` (as the `w % 17 > 8` beside it): 108 (fixes `slti at` against `slti v1`;
   a permuter find).
2. The last loop's two end values: the row loop's y end is the first web of its own s32 local
   declared after dh (`endy`, the game's slot 0x140), the column loop's x end the first web of
   another (`endx`), and `end` is used only by the edge loops of styles 0-3. With the y end a
   later web of `end` instead (S5) the saved registers were already the game's (55: the
   s/&a.y/&a.h/&a.w rotation gone) but its spill went to a temporary slot; with its own local
   declared after dh (endy before endx): 8.
3. The row loop reuses `step` (a later web) and the column loop has `stepx` (a first web): the
   game's f20/f22. With `r.x1 = x + w` and `l.x1 = x + w - dx` (the game loads x's extension
   first): MATCH.

## Round 30 (s2) log

Scratch: wfd (vr.py variant runner: `python3 vr.py VARFILE BASE.c`;
full.sh NAME: full listing + slot table; slots.py; opdiff.py: structural diff with registers
normalised; regpairs.py: register pairs per range; dm.py: declaration moves).

From 482 (s32 style, one Rect, i = t->u):
- u8 style: 514 (temporaries' slots right, i still a local slot).
- A second Rect: the game's lines use a Rect at 0x200 and the fill one at 0x210 (two Rects:
  `Rect r; Rect l;`, r for Prim_FillRect). With u8 style: 516 (frame 0x230 with i).
- No i (t->u read for both a.u0) + u8 + two Rects: 916 (h kept in a register, &a.u1 spilled,
  frame 0x210, size 988).
- dw, dh before a.w, a.h (`dw = ...; dh = ...; a.w = t->w; a.h = t->h;`) and `end = (f32)x +
  (f32)w;` between `n = h / 17;` and the n++ test: with i 616 (size 984: only i's sw/lw and
  reload differ structurally), without i 922 (S1, the permuter's start).
- 650 declaration moves from S1: all keep frame 0x210 (best 920).
- perm_nu from S1: in 4 minutes, finds that give i an earlier first web (`i = s->cv + s->ch;
  a.v1 = i;` in the corners: 133 in the build; `i = t->h; a.h = i; v1 = t->v + i`;
  `i = h - (t->h - t->iy)` for dh). Read as: the last loop's inner counter i must be a later
  web (rule 1 of "How MWCC numbers the values later passes create").
- Natural forms of that: both edge loops of styles 0-3 counted with i instead of n: **110**
  (applied); only loop 1: 923, only loop 2: 910; `i = t->u; u1 = i + t->w; a.u0 = i` (first
  a.u0 only, last re-reads t->u): 127; i/n swapped in the last loop: 933 or worse.
- Holding the corner's s->cv + s->ch in v1/u1/n/m/end/endy/dw/dh/dx/dy instead of i: 920+.

## For the lead

Nothing outside the function. All forms are ordinary C (no FAKEMATCH, no compiled-out check):
the loop counters and end/step locals are only named and shared differently; u8 style and
the two Rects are plain declarations.

### After the 110 commit
- Declaration moves from 110 (650): none better. Single type changes of every scalar local
  (80): none better (u32 style 194). `s` as block-scope, `gFrameStyles + style`, const, in place
  (`s = gFrameStyles; s += style`): no change. Statement moves of up to 6 lines (1020): best 107.
- `end = s->tw; if (end > 2)` (end given an earlier first web): 101 (the last loop's end/i
  registers right); a dedicated `s32 tw` local at every declaration place: 108. Superseded by
  the endx/endy split below, which needs nothing of the kind.
- end/endy swapped between the loops: both edge loops on endy, or the last loop's names
  swapped: 55 (registers right, the row end's spill in a temporary slot); then the row end on a
  new local after dh: 8; then step/stepx and x + w: MATCH.
