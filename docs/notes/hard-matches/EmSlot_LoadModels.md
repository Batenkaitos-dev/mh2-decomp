# EmSlot_LoadModels (sub_main, src/sub_main/scene_models.c)

## The match

A FAKEMATCH (tagged in the source; `s` below is the `scene` local in src): the byte offset o
set in both paths, kind first set to the array base, the integer-sum address. Each is "a cast
or statement with no purpose" in the rulings' sense. No natural form found: kind's slot pointer
must be a later web (or a value numbered after slot*20), and no natural C gives a u8 pointer a
dead first value. Without these forms the function misses by 24 words.

How each tagged form reads:
- The byte offset is written `o = slot * sizeof(s16)` (MATCH, as `slot * 2` and `slot << 1`),
  the form of the Kbd_DrawSpecialKey ruling (`ofs = mode * sizeof(KeyPos)`); each path computes
  its own offset as the game does (one `o` at the top: 79 words).
- The integer sum `(u8 *)(slot + (s32)s) + 0x2A` is the catalogue's form, untagged in
  Font_ClearPage (`(u8 *)(ofs + (s32)gFontCache) + 0x64`).
- What stays contrived: `kind = s->emKind;` for the test only, so that the slot pointer is a
  later web.

## What the game's registers need

The 24 words were one swap: slot s1 / kind s5 (game s5 / s1), plus its schedule and the
operand order of kind's address. Replaying the allocator ("How MWCC picks saved registers",
limit 7) on the 10 saved values (P slot, S s, K kind ptr, W work, M model, O slot*2, F slot*4,
T slot*20, W2/M2 the second work/model) with the interference read off the game (a script
builds the graph and simulates; another, given a colouring, lists the number orders that give
it): the 24-word form's colouring is its declaration order exactly. The game needs, in every
one of its 120 orders:

  P < S < F < T < K < O,  M < W,  W2 < M2,  T < M2

i.e. kind and slot*2 are numbered after the common subexpressions slot*4 and slot*20, in
code order (kind block 2, slot*2 block 3), and the second model after slot*20. A declared
local is numbered before every created value, so kind and o as plain locals can never give
it. Later webs (a variable set again after its first value died) are numbered after the
created values, in code order: that is what the match uses:
- `o = slot * 2` set in the early-return block too, so block 3's o is a later web;
- `kind = s->emKind;` for the test (`kind[slot] == 0`), then the slot pointer (later web);
- work and model reused for the second set (natural; W2, M2 later webs).
That gives every register (1 word: `addu v0,s4,s5` for the game's `addu v0,s5,s4`).
- Operand order of kind's address: a pointer expression (&s->emKind[slot], s->emKind + slot,
  slot + s->emKind, (u8 *)s + slot + 0x2A, casts, u32 index) always gives (s, slot); the
  game's (slot, s) comes from MWCC's CSE of the load `s->emKind[slot]` (seen in the natural
  form) or from integer addition in source order: `(u8 *)(slot + (s32)s) + 0x2A` (used).

## Why natural C doesn't get there

- All-natural form (s->X[slot] everywhere, s->emKind[slot] reads, work/model reused):
  117 words. slot*2 recomputed in every block, kind's CSE recomputed in block 3 (3 reads),
  block 2's kind CSE carried only to the success block.
- Variations: gScene bases in the early block (142), block orders, cnt/first order, nesting
  `if (model >= 0)`, `s` initialised, goto/label splits between the two stores, inline
  empty functions: no change to the carry (114-142).
- Synthetic tests (test copies c4-c7, counting `sll n,1`): block 3 keeps n*2 for the later
  blocks only in odd shapes: `first[n]; models[n] = Get(m); cnt[n]` (a call AND an n*4 access
  before the second n*2), `first[n]; models[n] = 0; Free(); cnt[n]`,
  `first[n]; Free(models[n]); cnt[n]`, `first[n]; Free(); gV = n * 3; cnt[n]`. Not: a call
  alone (Nop(), Get() unused), a store alone, a global store, n*4 after the call. The game's
  block 3 stores first/cnt adjacent before Model_Get, so none of these is its source.
- CSE numbering (test copies c1-c3): a common subexpression is numbered at its **second
  occurrence** (first reuse) in code order (f8, f10 contradict "last occurrence").
- Declaration-type changes of o, work, model; a u32 copy of slot (moves P, 31-50); kind
  forms: none better than 24 without the later-web form.

## Natural forms tried on the matched function

Scores in words (1 = every register right, a %lo addend difference on kind); D is the number
of `sll n,1`.
- Natural base (s->X[slot]; `u8 *kind = &s->emKind[slot]` after the test): 93 (D5). Load-CSE
  kind (s->emKind[slot] at each read): 117.
- Compiled-out check lever (EmSlot_FreeModels.md): a two-test empty `if` after modelFirst's
  store in block 3 AND after partFirst's store in the second success block (or after the
  failure block's) carries slot*2 as the game does (D2), but 101-107: slot*2 is then a common
  subexpression numbered too early (s3/s4, not s0) and the colouring takes s7. The check's
  variable (slot, model, *kind) changes nothing; one check alone does nothing.
- 216 combinations (4 declarations' orders x none/two checks x kind after the test / at the top
  / `kind = s->emKind; ... kind += slot`): best 86 (D5).
- static inline helpers (a kind pointer, a kind value, the first/count store pair) compile to
  the same code as without them: the inliner creates nothing numbered late.
- kind's address (on the fake form, 1 word for the operand order): &s->emKind[slot],
  s->emKind + slot, slot + s->emKind, &slot[s->emKind], &gScene.emKind[slot], (u32) index,
  `<< 0`, a static inline &gScene accessor: all (s, slot). Load before the pointer
  (`Load_EmModelPack(s->emKind[slot]); kind = &s->emKind[slot];`): 113. Only the integer sum
  gives (slot, s).
- kind once, block-scope (declared at its main-path set): 21 (K s3; function scope 31).
  `kind += slot` / `&kind[slot]` after `kind = s->emKind`: 90 (an extra lui for gScene+0x2A).
- o computed once at the top: 79. `o = slot * sizeof(s16)` (and `slot << 1`): MATCH.
- The allocator replay with MWCC's push rule (fewest references, ties last-numbered) finds the
  same 120 orders, none with a push: kind and slot*2 must be numbered after slot*20. A CSE is
  numbered at its first reuse, and kind's first reuse (Setup1's argument) comes before
  slot*20's (Setup2's); slot*2's first reuse is in block 3.

## What a natural form would need

1. A new rule: something that numbers a value after MWCC's common subexpressions other than a
   later web (a loop optimizer value? none here), or a natural dead u8 pointer.
2. A natural first value for kind: the early-return path has no u8 pointer in the game.

## Applying the model to EmSlot_FreeModels

- Same model: replay it with the graph changed for its values.
- In the game it reads `gScene.modelFirst[n]` / `modelCount[n]` with constant bases in the
  first block (D_006ADD98, D_006ADD90): two n*2 uses not sharing an address, which is the
  shape that lets n*2 carry; the after-loop block then shares `s + n*2` (pf, pc in s3, s2)
  and the last stores recompute n*2 and n*4.
- Later webs are the lever when a value must be numbered after created ones (the j rotate,
  n, n*2).

## Rules

Candidate CONTRIBUTING lines: "A value that must be numbered after MWCC's common
subexpressions (slot*4, slot*20) can't be a declared first web; replay the allocator on the
game's colouring to see which values those are" and "a CSE is numbered at its second
occurrence" (test copies c2 f8, c3 f10).
