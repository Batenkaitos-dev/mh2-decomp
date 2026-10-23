# Plsel_BuildResultText (plsel, src/plsel/quest_results.c)

## The match

A FAKEMATCH. The match needs `static f64 Stripped_Sub(f64 a, f64 b) { return a - b; }`
before the function (the Browser_ReadPad rule, CONTRIBUTING "Fakematches": a stand-in for a
function the game's linker stripped, compiled by tools/mwcc_cc.py and dropped), the floats
passed to sprintf promoted by the compiler (`sprintf(buf2, t[54], f)`, all six sites, no
explicit fptodp), and both size tests as `(sz = w->oldX) > 0` (not `0 <` for the second).

The stand-in sits just before Plsel_BuildResultText; nothing after it in the file changes.
The stray "%s" before 0xD6ABB0's jump table is the other
sign of a stripped function in quest_results.c. The file's `char *fptodp(f32);` is still used
by two later functions (explicit calls).

## Why

Once a function earlier in the file has compiled a two-argument runtime call, MWCC models
its own fptodp calls (implicit float-to-double promotion) as reading a1 too (Browser_ReadPad
shows the same for litodp). In case 5 the last a1 set before each size test is the format of
the sprintf before it, so a1 is live from there, over the test (through the else path, which
sets no a1), to the fptodp after it. So the size can't coalesce into a1 (it stays in v0,
`move a1,v0` after the test), and the move can't fill the blez slot (it writes a1, live at
the target). Nothing in plsel calls a two-argument helper (asm scan: only fptodp, 14 calls),
so the state must come from a stripped function.

## Evidence

### The stand-in

- Small tests: implicit promotion alone: ours (lhu a1). With `Stripped_Sub` before: the
  game's `lhu v0; blez v0; nop; daddu a1,v0; jal; andi a0`. The same with a user
  `char *fptodp(f32);` declared: same (the declaration doesn't matter). Stripped_Sub with
  explicit `fptodp(f)` calls: ours (explicit calls never get the effect). The game's form
  with a call (`h0()`) in the else branch, which sets a1 on that path: ours again at that
  site (lhu a1, andi a0 in the slot), which confirms the a1-live mechanism.
- Real file: implicit promotion without the stand-in: 24 words; with it and the second test
  `> 0`: MATCH; the other four fptodp sites of the function made implicit too: MATCH.
  Without Stripped_Sub the matching C is 667 (2 words short); explicit fptodp and a `0 <`
  second test: 24 words.

### Forms without a stand-in

A small test reproduces both shapes: u16 sz gives ours, s32 sz a 6-word shape (as does a
prototype `f32 Monster_SizeFromPercent(u8 kind, s32 x)`).

- Delay-slot filler (fall-through block of a conditional branch):
  - takes the first instruction: `andi a1,v0` (s32 sz), `dsll32 a1,v0,0` (s64 prototype),
    `daddu a0,s1,zero`, `daddu a0,s0,zero` where s0 is the just-loaded branch operand;
  - does not take a load (volatile, second loads: then the game's exact branch shape
    `blez v0; nop; lhu a1,...; jal; andi a0`), or an in-place op on the branch operand
    (`andi v0,v0,0xFFFF`, `dsll32 v0,v0,16`);
  - a switch's compare branch is never filled (`beq a1,zero; nop`), but it is beq, not blez.
- Coalescing into a1: every form coalesces the u16 size into a1 (lhu a1), or converts it
  (andi a1 / dsll32 a1) in the slot:
  - conversions: sz s16/u32/s64/u64/s8/u8; prototypes s32/u32/s64 x sz u16/s16/s32;
    (u16)(u32), (u32)(u8 *), (u16)(s16), (u16)(u64), sz + 0;
  - typed local copies in the then-block or before the test (u32/s32/pointer);
  - the test on the field and the arg in a typed copy, or the reverse;
  - inline wrappers (s32/u8 kind, u16/s32 x);
  - `register`;
  - uninitialised reads of sz (webs merged), also next to a1 (Sjis_Widen(T[sz], buf));
  - ternary, while/break, for/goto, do/while(0), goto;
  - switch;
  - #pragma optimization_level 3/2, schedule off.
- Scans of the game:
  - `l* vX; branch on vX; nop; daddu aN,vX,zero`: only these two sites;
  - the same with the move in the slot: 0 sites;
  - a load followed by a move into an argument register before a jal: 8 sites, all spill
    reloads from the stack or a call result precoloured v0 (Map_DrawMonsterIcon: a variable
    set from a call in one arm and a load in the other);
  - the 79 self-moves `daddu x,x,zero` follow stack reloads (spills: SkillList_DrawPage's
    `mark`).
  So a kept move into an argument register comes from a call result or a spill everywhere
  else in the game. This function has no spills: s0-s6 are saved, s7 is free, and nothing
  else is on the stack.
- The ObjAttach "move" (ObjAttach_DamageEm.md) is `addu v1,v0,zero`, from `(u8 *)0 + x`. It
  is not the daddu move: small tests give `addu rd,rs,zero` for a null-based pointer add.
