# Player_DrawWeaponPart (sub_main, src/sub_main/player_draw.c)

## Status

- **Matched** (round 30, session 3), as a FAKEMATCH with two contrived forms (both in "For the
  lead"): `Player *const p = pl;` (a const local copy of the parameter) and the oy/oz pointers
  in the 0x323 block. 127 -> 77 (every saved register the game's) -> 22 (statement orders) -> 0.
- 22 -> 0: the sheathed case 3/4/5 in the order `part; sx *= 2.5f; sy *= 3.0f; sz *= 2.0f;
  bone; rx; ry; rz; off[0]; off[1]; off[2];` (found by a hill climb over the case's statements,
  scratch r3/blkclimb.py; then the most natural of the matching orders).
- 77 -> 22, each checked by hand (independent constant statements reordered, no read moved past
  its write): 0x324.. `part = ...HeldPartForMotion...; bone; rz; off[0]; rx; off[1]; ry = rx;
  off[2]` (bone after the call: the game fills the call's delay slot with `move a0,s0`): 53;
  `(u32)(motionId - 0x3F4) <= 1` for `< 2` (the game's `sltiu at`): -2; 0x323 block `off[0] =
  12.0f` before `*oy`/`*oz`: -4; 0x3F1.. and 0x3F6.. `bone; part; rx; ry; rz; off[0]; off[1];
  off[2]` (+ `sz = 0.9f; sy = sz; sx = sz;`): 34; 0xEB.. and 0x1A0 default `part; bone; rx; ry;
  rz; mode; off[0]; off[1]; off[2]`: 22.
- Session 2 (804aa58d): the off stores in order in the 0x39..0x3E case: 140 -> 127; 560ae46c the
  lead's meaning fix there (`sx = 0.75f; sy = sx;`). Session 1: 562 -> 139 (see the CSV rows).
- What gave 127 -> 77 (three changes, all needed together; each alone did nothing or worse):
  1. the 0x320 case's standing-part state in its own `s16 stand` (mode keeps one web, so it is
     numbered at its declaration: mode/idx s4 as in the game);
  2. set read as a Model (`set = (Model *)gCommonModels[0]`, `set->active`, `(u8 *)set->mats`,
     `(u8 *)set->matSets + K`, as the file's other Model readers): a plain `u8 *` copy of the
     extern is propagated away (its load is a created value numbered early); the cast keeps set
     a variable;
  3. `p` a const local copy of the parameter (`void Player_DrawWeaponPart(Player *pl)`,
     `Player *const p = pl;` declared right after set): set then sits between p and the
     &off CSEs in the colouring. A plain `Player *p = pl;` is propagated (162 words); `void *obj`
     or `u8 *obj` parameters with `Player *p = obj;` give the same 77 (need line 2202 changed).

## Handoff

- None: matched. Scratch r3 has the tools used (gen.py, chk.py, apply.py,
  permrun.py: every order of a case's statements with a dependency check; blkclimb.py: a hill
  climb over moves and swaps within one block).

## For the lead

- FAKEMATCH (tagged at the match, per your ruling): pointer locals for &off[1]/&off[2] in the
  0x323 block. No natural form found; without them 448 words (4 more instructions).
  Why (black-box, dw/t/t1.c..t11.c, all at -O3,p): when the block before an `if` uses a
  common subexpression (here the &off[1] address MWCC keeps in s2) before a call, and the
  if-block uses it two or more times with a call between, MWCC's common-subexpression pass
  (it goes with `#pragma opt_common_subs off`, not with opt_lifetimes) gives the if-block its
  own copy (`daddu s7,s2,zero`), which the allocator never merges. The game has no copy, so its
  source differs there. No copy when: the block before the if has no use before its last call
  (t11), a real `if` sits between the call and the block (t22, t23, t24), the if-block uses it
  once or with no call between (t4, t5), the if is removed (S_noif). Still a copy with: do {}
  while (0), goto/label, if (0), switch (0), empty if, a single-case switch for the if (w1),
  the block as a static inline (i1), set-and-rotate helpers (j1, j2), `*(off + 1)`, a plain
  `f32 *v = off` (i2), loops, x/y/z computed in both branches or in a comma condition (c1-c3),
  every order of the onBone == 0 path's statements (648). Pointer locals set inside the block
  (`oy = &off[1]`) are declared variables, so no copy: 1172 instructions. Set at the function's
  top: 1146 (worse); at the else block's top: 246 words.
- (session 3) `p` as a const local copy of the parameter: `void Player_DrawWeaponPart(Player
  *pl) { ... Player *const p = pl;` (declared right after `Model *set;`). The const keeps
  MWCC from propagating the copy into the parameter (the ruling on Page_Click's const view of a
  global, here for a parameter); without it 162 words. The form with no const: the parameter as
  `void *obj` (as the shared-header helpers take it) and `Player *p = obj;`: also 77 words, but
  it needs line 2202's prototype changed to `void Player_DrawWeaponPart(void *obj);` (the call
  at 2343 passes `(Player *)p`, which converts). A same-type cast `Player *p = (Player *)pl;`
  also gives 77 (a cast with no purpose). At the match, `void *obj` + `Player *p = obj;` also
  matches in full (checked with scratch r3/gen.py, where the prototype doesn't apply). Committed:
  the const copy, tagged FAKEMATCH. If you rule the const copy natural (as Page_Click's const
  view), drop its tag; or take the `void *obj` form with the one-line prototype edit at
  line ~2205: `void Player_DrawWeaponPart(void *obj);`.

## Earlier sessions (round 30, sessions 1-2)

### Status then

- Round 30, session 2: meaning fix from the lead's audit: in 0x1A0 / useItem 0x39..0x3E the
  draft had `sy = sx; sx = 0.75f;` (sy = 1.0; the game copies f27 = 0.75 into f26 and f25);
  now `sx = 0.75f; sy = sx;`: 139 -> 140 words. The 0x3F1 case's `sx = sy = 1.0f;` (permuter
  find) stores the same values as before (1.0 in both): checked.
- Round 30 (slot s5, session 1): 562 -> 139 words (func_diff), NON_MATCHING. Size now 1172
  instructions, the game's. Frame (416) and every stack slot already matched; no spills.
- Applied: (1) in the 0x323 block, `oy = &off[1]; oz = &off[2];` and every off[1]/off[2] there
  written `*oy`/`*oz` (FAKEMATCH candidate, see "For the lead"); (2) S3 in the order x, y, z
  (&pos[1] then gets s8 as in the game): 167; (3) a hill climb over moves of call-free
  constant assignments within their runs, with a dependency check (cases 3/4/5, 0x39..0x3E,
  0x324.., 0x3F6..): 141; (4) `sx = sy = 1.0f;` in the 0x3F1.. case (from a permuter output): 139.
- Left (139): set/part in s4 and mode/idx in s1 where the game has s1/s4 (the only saved-register
  difference regalloc_predict reports now), the scheduling of the sheathed cases (listing lines
  131-185) and of the cases that follow from it, a few float registers.

### Handoff then

- Generator: dw/gen.py (copied from efx/; `=== name` blocks of OLD/---/NEW,
  `+++` between replacements, `@@OLD` = replace all; SRC=FILE reads another copy of the source)
  and dw/cnt.sh VARIANTS (one line per variant: positional words, aligned diff, s-to-s copies,
  s8 = &pos[1], length). dw/climb.py N: the move hill climb on dw/cur.c (a copy of the source;
  it rejects moves across a statement that reads or writes the moved one's variables; check
  its result by hand anyway: the first version moved `sy = sx` past `sx = 0.75f`). Mini tests
  of the copies: dw/t/*.c with dw/t/run.sh FILE.c.
- Next ideas, in order:
  1. The set/mode swap. regalloc_predict finds no order (11 saved webs). No gain from: set/mode
     declaration moves, initialisers, mode u8/u16/s8/s32, idx s16/u8/u32, onBone s16, bone s32,
     set read as gCommonModels[0] at each use, `u8 **models`, lights from the global, part
     initialised, mode's 0x320 web as its own `stand` (225, worse). `u8 *const set =
     gCommonModels[0];` declared before bone moves set to s0 (it is then a declared local's first
     web; the plain set is substituted away), after bone nothing; with mode and bone at every
     position (504): nothing better. Untested: set as a later web of a dead u8 * variable,
     "short webs declared first", the order of the `mode = K` stores in each case together with
     the case's other statements (the climb kept each run's statements apart from calls).
  2. perm_nu from this state: run 17 min (-j2) in session 1, 7040 -> 6640; in the build only
     `sx = sy = 1.0f` (0x3F1..) helped (applied); `sx = sy = 1.0f; sz = sx;` at the top,
     `sy = sz = 0.9f` (0x3F6..): worse; others were a compiled-out `if ((sy && sy) && sy) {}`
     and an extra `off[1] = (rx = 0.0f)` store (a change of meaning). perm_nu wants BEST.c to
     be the function's text only (dw/best_fn.c); given the whole file it drops the function.
     Worth a longer run (45-60 min) from 139.
  3. A natural form for oy/oz.

## The lead's ruling (round 30)
The const local copy of the parameter is replaced by the `void *obj` form this session found
(`Player *p = obj;`, the prototype changed to match): the file's shared-header helpers take
`void *obj` the same way, and the caller already casts a u8 *. The FAKEMATCH tag keeps only the
oy/oz pointers.
