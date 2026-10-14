# Menu_DrawQuestSheet (lobby, src/lobby/unk_760C70.c)

## Status

- MATCHED in round 30 (slot s6, second session), from 5 words. FAKEMATCH tag (a statement
  written twice). Whole file func_diff MATCH (40 functions), ninja (13 OK).
  Level 4 (pragma pair), as before.

## What matched it

- A stamp colour local, `u32 col`, set after `j = (sel + 2) % 12;` and again as the first
  statement of the first `if (offs != NULL)` block, used by the two `b != -1` stamp calls; the
  else call keeps the constant.
- Why: with two definitions, MWCC's copy propagation (the IRO loop) keeps col a value of its
  own; a later pass drops the repeated definition (no call between the two: a call in between
  keeps both, and col is then spilled to the stack, 166-205 words). The back end sees one
  constant definition live over most of the function; the colouring spills it and
  rematerialises `lui t0` at each use, so the frame and every other register stay as before.
  In the first scheduling pass the second call's t0 is then the move that ends col (rule 5:
  +0, created before t1/t3), so it goes second: t2, t0, t1, t3, a2, a3, as the game.
- Placement matters: of 16 statement positions for the pair, only the first offs block's
  (inside before the call, or right after the block) matches; elsewhere 61-220 (col coloured
  s6/s8 when set inside the stamp branch; b and h swapped in the spill set, or a 0xF0 frame,
  when set earlier or later). Declaration position and type (u32, s32, char *) don't matter.
- col in the else call too: 3 words (its t0 moves before t1/t3, as the old model said).

## The rule found (for CONTRIBUTING)

- **MWCC rematerialises a spilled constant.** A local set to a constant that copy
  propagation keeps (two definitions in its web at that time, one dropped later) and that the
  colouring spills is not stored: `lui`/`li` again at each use, straight into the argument
  register. Its uses then act as moves that end a value in the first scheduling pass, which
  moves them earlier. A game call that sets a constant argument earlier than every
  creation order allows (a +0 instruction where the constant would be +1) can be this.
- **A repeated constant assignment is dropped after copy propagation** when nothing between
  the two (no call) can change the variable: `c = K; if (v) c = K;` or `c = K; if (v) { c = K;
  F(); }` keep c a value with one definition. With a call between, both definitions stay.
- MWCC never keeps a constant across a call by itself (scratch qs2/t/d1.c, d2.c: extern,
  static and earlier-defined callees, switch-chain constants, assignment in an argument): a
  single-definition constant local is always propagated, whatever its type (u32, s32, u64,
  s64, char *; a union goes to memory, a float is converted at each use).

## Earlier sessions (12 -> 5)

- `(y + h / 2) - 0x30` written at both stamp calls, as `x + w / 2` already is, with no py
  local: h's extension is numbered before the D_007FE934 load (12 -> 5).
- x + w / 2 written out at each use (162 -> 18), k = b; k *= 2 (permuter find, 18 -> 12).
- The first-pass model reading of the last 5 words (t0 must end a value) was right; every
  single-definition form of the colour was propagated (CSV rows of round 30).

## Log (round 30, s6)

- Session 1: gen.py (scratch qs/gen.py: every variant in one file, scored), perm_nu 31 min from
  the 5-word state (nothing below base).
- Session 2: scratch qs2/ (gen.py copy, cls.py: score plus parameter homes and t0 setters per
  variant; mk1.py: the placement sweep; mk2.py/mk3.py: the candidate forms; t/d1.c, d2.c: the
  small tests above). No job left running.

## For the lead

- The match is a tagged fakematch ("a statement written twice"). No natural form found: a
  single definition is propagated; a default plus an override in both offs branches keeps two
  definitions (stack spill). If a natural reason for the repeated assignment turns up (a
  macro, or an override colour equal to the default), the tag can go.
- Nothing outside the function changed. The block's stray `int abs(int);` (inside the old
  NON_MATCHING branch only) was dropped: `s32 abs(s32);` before its users stays.
