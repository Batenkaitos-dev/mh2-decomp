# StageObj_K29_Draw (game, src/game/player_effects.c)

## The match

Natural C, no FAKEMATCH tag. The entry's temporaries were a numbering question in three
parts (words left after each, from 20):

1. **The constant 1** (`li t0,1`, for `p->type == 1`) is MWCC's common subexpression of
   kind's initialiser. With `u8 kind = 1;` as the first declaration it is created at the top,
   has 25 neighbours, waits and takes v1. `kind = 1;` as a statement after p's computation
   (kind still declared first, so still s3) leaves it fewer than 23: t0, as the game. That
   also put visible (v1) and buf (a0) right (47 words, more elsewhere).
2. **owner and the object type**: `StageObj *o = node->owner;` and a `u32 type` local set in
   the test (`type = o->type, tbl = StageObj_K29_DrawTbl[type], ...`, `switch (type)`), as
   StageObj_K0F_Draw and StageObj_Gun_Draw write them: both are declared values, numbered
   first, so o a2, type a1 (34). type declared before o, or u8: 38-51.
3. **set and p->type**: MWCC numbers a common subexpression at its **last occurrence** (below).
   set's last use is `set->mats` at the join; p->type's CSE runs into the part switches of
   types 1, 13 and 14, so without a local p->type is numbered after set (p->type v1, set a0).
   The game numbers p->type first: a declared `u8 pt` (after type), set in the test
   (`(pt = p->type) == 1`) and in the else (`pt = p->type;`), and read by the part switch of
   type 1, 13 or 14 (any one is enough; the three are written with pt). Then set's CSE is
   numbered after every declared local: v1. `Model *set` (assigned in the test) must be declared
   after pt (after mode or tbl: 38; first: 47); gStageModelSet at every use matches too.

The match's one unusual form: pt is set inside the test and again in the else, and only the
part switches before any call read it (the others read p->type again, as the game reloads it
after calls). It is the ruling's "a local that caches a field for some of its uses"; the
comment above the function says which uses read the field.

## Why

- **A common subexpression is numbered at its last occurrence** (in code order), a rule for
  CONTRIBUTING with this function as the example. Evidence on the 34-word state:
  `(node->owner)->var1` in case 16 (a permuter find) makes owner a CSE whose last use is case
  16: it is then numbered last and coloured first (v1; 25 words). Reloading p->type (a
  volatile probe) in cases 1, 13 and 14 together, or in the test or the else, ends p->type's
  CSE before the join: set then takes v1. Gs_ClearFrame's "second occurrence" is this rule
  for a value used twice. Where a CSE must be numbered earlier, a declared local for it (a
  value of its own) is numbered with the locals.

## Evidence (words)

- kind = 1 placement: after p 47 (constant t0); in the if body 690.
- set: gStageModelSet everywhere 38; set statement before the if 64 (load moves to the entry);
  tbl before the if 73-76; set copies/casts in the test, body or mats: 38 (all propagated).
- o + type local: 34; with set declared at five places as a (Model *) cast copy: 34-47.
- The permuter from 38 and from 34: only an uninitialised u8 local for case 14's switch (UB,
  rejected) and the case-16 owner read (25, explained above).
- pt in every part switch: 616 (it then lives across calls); pt assigned but unused: 34.
