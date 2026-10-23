# Player_DrawHeldProp (sub_main, src/sub_main/player_draw.c)

Formerly named Player_DrawWeaponPart; its comment in the source cites this note.
Size 1172 instructions, frame 416 bytes, no spills. Below, `p` is the Player pointer local
(named `pl` in the source) and `obj` the parameter.

## The match

FAKEMATCH, for the oy/oz pointers in the 0x323 block (the spit roast's last motion; offY
and offZ in the source). The forms that make the match, with the words left after each
step (func_diff), from 562:

- 562 -> 139: in the 0x323 block, `oy = &off[1]; oz = &off[2];` and every off[1]/off[2]
  there written `*oy`/`*oz` (the tagged form, see "The oy/oz pointers"); S3 in the order x,
  y, z (&pos[1] then gets s8 as in the game): 167; a hill climb over moves of call-free
  constant assignments within their runs, with a dependency check (cases 3/4/5,
  0x39..0x3E, 0x324.., 0x3F6..): 141; `sx = sy = 1.0f;` in the 0x3F1.. case (a permuter
  find; it stores the same values, 1.0 in both): 139. The frame and every stack slot
  matched from here.
- The 0x1A0 / useItem 0x39..0x3E case is `sx = 0.75f; sy = sx;` (the game copies f27 = 0.75
  into f26 and f25; `sy = sx; sx = 0.75f;` would set sy to 1.0): 140. The off stores in
  order in that case: 127.
- 127 -> 77 (every saved register the game's): three changes, all needed together; each
  alone did nothing or worse:
  1. the 0x320 case's standing-part state in its own `s16 stand` (mode keeps one web, so it
     is numbered at its declaration: mode/idx s4 as in the game);
  2. set read as a Model (`set = (Model *)gCommonModels[0]`, `set->active`,
     `(u8 *)set->mats`, `(u8 *)set->matSets + K`, as the file's other Model readers): a
     plain `u8 *` copy of the extern is propagated away (its load is a created value
     numbered early); the cast keeps set a variable;
  3. the player as a local copy of the parameter that MWCC keeps, declared right after
     `Model *set;`: set then sits between p and the &off CSEs in the colouring. The source
     has `void Player_DrawHeldProp(void *obj)` with `Player *p = obj;` (as the file's
     shared-header helpers take the player; the prototype says `void *obj` too, and the
     caller's `(Player *)p` converts). Also 77, and also a full match: `Player *pl`
     parameter with `Player *const p = pl;` (the const keeps MWCC from propagating the copy
     into the parameter, as Page_Click.md's const view of a global); a same-type cast
     `Player *p = (Player *)pl;` (a cast with no purpose). A plain `Player *p = pl;` is
     propagated: 162 words. With a Player * parameter read directly, the compiler swaps the
     two registers that hold the player and the model set.
- 77 -> 22, each checked by hand (independent constant statements reordered, no read moved
  past its write): 0x324.. `part = ...HeldPartForMotion...; bone; rz; off[0]; rx; off[1];
  ry = rx; off[2]` (bone after the call: the game fills the call's delay slot with
  `move a0,s0`): 53; `(u32)(motionId - 0x3F4) <= 1` for `< 2` (the game's `sltiu at`): -2;
  0x323 block `off[0] = 12.0f` before `*oy`/`*oz`: -4; 0x3F1.. and 0x3F6.. `bone; part; rx;
  ry; rz; off[0]; off[1]; off[2]` (+ `sz = 0.9f; sy = sz; sx = sz;`): 34; 0xEB.. and 0x1A0
  default `part; bone; rx; ry; rz; mode; off[0]; off[1]; off[2]`: 22.
- 22 -> 0: the sheathed case 3/4/5 in the order `part; sx *= 2.5f; sy *= 3.0f; sz *= 2.0f;
  bone; rx; ry; rz; off[0]; off[1]; off[2];` (found by a hill climb over the case's
  statements, then the most natural of the matching orders).

## The oy/oz pointers

No natural form found; without them 448 words (4 more instructions). Why (black-box tests,
all at -O3,p): when the block before an `if` uses a common subexpression (here the &off[1]
address MWCC keeps in s2) before a call, and the if-block uses it two or more times with a
call between, MWCC's common-subexpression pass (it goes with `#pragma opt_common_subs off`,
not with opt_lifetimes) gives the if-block its own copy (`daddu s7,s2,zero`), which the
allocator never merges. The game has no copy, so its source differs there.

- No copy when: the block before the if has no use before its last call, a real `if` sits
  between the call and the block, the if-block uses it once or with no call between, the if
  is removed.
- Still a copy with: do {} while (0), goto/label, if (0), switch (0), empty if, a
  single-case switch for the if, the block as a static inline, set-and-rotate helpers,
  `*(off + 1)`, a plain `f32 *v = off`, loops, x/y/z computed in both branches or in a comma
  condition, every order of the onBone == 0 path's statements (648).
- Pointer locals set inside the block (`oy = &off[1]`) are declared variables, so no copy:
  1172 instructions. Set at the function's top: 1146 (worse); at the else block's top: 246
  words.

## Evidence for the set/mode registers

At 139 words, set/part were in s4 and mode/idx in s1 where the game has s1/s4 (the only
saved-register difference regalloc_predict reported; it found no order, 11 saved webs).
No gain from: set/mode declaration moves, initialisers, mode u8/u16/s8/s32, idx s16/u8/u32,
onBone s16, bone s32, set read as gCommonModels[0] at each use, `u8 **models`, lights from
the global, part initialised, mode's 0x320 web as its own `stand` alone (225, worse).
`u8 *const set = gCommonModels[0];` declared before bone moves set to s0 (it is then a
declared local's first web; the plain set is substituted away), after bone nothing; with
mode and bone at every position (504): nothing better.

Permuter from 139 (7040 -> 6640): in the build only `sx = sy = 1.0f` (0x3F1..) helped;
`sx = sy = 1.0f; sz = sx;` at the top, `sy = sz = 0.9f` (0x3F6..): worse; others were a
compiled-out `if ((sy && sy) && sy) {}` and an extra `off[1] = (rx = 0.0f)` store (a change
of meaning). perm_nu wants BEST.c to be the function's text only; given the whole file it
drops the function.

## Method

A statement-order hill climb (moves and swaps within one block, or every order of a case's
statements) with a dependency check that rejects moves across a statement that reads or
writes the moved one's variables. Check its result by hand anyway: one version moved
`sy = sx` past `sx = 0.75f`.
