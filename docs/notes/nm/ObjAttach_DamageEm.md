# ObjAttach_DamageEm (sub_main, src/sub_main/hit_detection.c)

## Status

- **Matched** in round 30 (slot s1, second try): 0 words, CHECK OK. The fix is statement order:
  `thunder`, `water`, `ice` and `dragon = 0.0f` go after the two hz statements (before
  `id = o->hitId`); `mv`, `eatk`, `atk`, `fire = 0.0f` and `crit = 0` stay before them.
- Why it works: the first scheduling pass (before colouring) sees fewer independent values
  before the hz chain, so the `elist[2] * 10` chain's last `sll` comes before the table's
  `sll` and the colourer gives weaponType*4 and e*5 the same v0, as in the game. The final
  (second-pass) listing is the same either way, which is why no form of the hz statements
  could fix it. "What is left" below is the first session's analysis, kept as history.

## How it was found (round 30, s1)

- A probe (scratch de2/pv.py) prints func_diff words and whether wt4 shares
  e5's register (the game's colouring, "G"). Single statement moves (225), head deletions and
  every hz form: never G.
- 300 random head orders (store before the loads, hz pair kept together; rperm.py): 26 G, all
  with several statements after the hz pair; best G at 8 words (only float registers off).
- Split sweep (fl.py): the base float inits with the first k before hz and the rest after:
  k = 4 gives 0 words for every order of bow/nostun/weak/store/crit tried; k = 5-6: 2;
  k = 0-3: 37-52; k = 7-8: 9. With the rest after `id`: 9-52.
- Other finds on the way (CSV): `hz += X + X`, X = (elist[2] << 2) + elist[2], or an s32 f5 =
  elist[2] * 5 after the table with `hz += f5 + f5`: 2 words (addu for the last step); a
  CSE'd e*5 is coloured after wt4, a numbering change, not the game's schedule.

## What was left (first session, history)

Game: `lb a1,0x989(a2); sll v1,v0,1 (E4); sll v0,a3,2 (A2); lui a2; addiu; addu v0; sll a2,a1,2;
lw a1,0(v0); lbu v0,0xB5(a0); addu a1,a1,a2`. Ours: A2 in v1 right after the lb, E4 after the
`lw a1`, then `addu a1` before the hitId load. Names used below: A1 lbu weaponType, A2 its sll,
A3/A4 lui/addiu D_90CBD0, A5 addu, B1 lb hitzoneSet, B2 its sll, A6 lw row, A7 addu, A8 lw hz,
E1 lh elist[2], E2 sll 2, E3 addu, E4 sll 1, H `addu s0,s0,v1` (in the bnez delay slot).

- The difference is the colouring, and the colouring comes from the first pass. In the game,
  weaponType*4 and &D[wt] share v0 with elist[2]*4 and *5, so the game's first pass had E4 before
  A2; in ours A2 comes first and gets v1.
- Colouring check (scratch csolve.py, csolve3.py): a colourer that takes the
  values in reverse creation order, each getting the first free register of v0, v1, a0, ...,
  reproduces the `#pragma schedule off` colouring of this block exactly. The script then searched
  for first-pass orders that give each final colouring, with the spilled `id` numbered as a
  declared local. Ours needs E3 < A2 < A5 < B2 < A6 < E4. The game needs E1..E4 before A2, with A1,
  B1 and A3 before E4: one decision differs (E4 or A2 first), as the lead read it.
- Creation order (`#pragma schedule off`): A1..A8, then E1..E4, H, in every spelling tried. A
  local `off = elist[2] * 10` before the table statement is forward-substituted, so it is created
  at its use. A one-expression sum, `&tbl[off]`, `&T[..][..][elist[2] * 10]`, a 10-byte struct
  pointer, and static inline helpers (whose parameters are substituted too) all put the table
  first. Only `#pragma opt_propagation off` keeps an `off` local computed first. Then E4 does go
  before the table's sll, but the loads come E1, B1, A1 (the game has E1, A1, B1), and the rest of
  the function breaks (1207).
- Moving `o->unk0D0[0] = 0;` after the two hz statements also puts E4 first (25), but the store
  then comes after the chain, and the game's store comes before every load.
- Scheduler model (scratch sim.py, simval.py): two issue slots, FPU ops in their own pipe, load
  latency 3, priority = register pressure then creation order, no heights. It reproduces our
  colouring exactly but not three other variants' (no_crit0, m3_15, m12_14), so it is not reliable.
  It agrees that an E chain created after A2 loses the E4/A2 tie (both end one value and create
  one), whatever the fillers are.

## Tried in round 30 (s4); func_diff words, 9 unless noted

- Moving each of the 16 head statements (bow = 0 .. id = o->hitId) to every other head position,
  225 moves (stmv.py): best 9; E4 first only in m3_15 (the unk0D0 store after hz, 25).
- `off` / `e` / `k` / `w` locals (s32, s16) before the table statement: 9. `e = elist[2]` at the top
  (before the store): 36. `off` at the top: 35.
- An empty `if` on off, part or hz between the statements (to stop the substitution): 9 (MWCC
  removes it first).
- Table spellings `*(D[w] + k)`, `k[D[w]]`, `*(*(w + D) + k)`: 9. A 10-byte struct type for hz
  (`hz += elist[2]`; `&((struct HZ *)row)[elist[2]]`): 9 and 10.
- Inline helpers Hz_Get(t, part) in 7 forms: 9-10.
- Chained initialisers (`mv = eatk = atk = 1.0f`, the 0.0f chain, `bow = nostun = weak = 0`, crit
  in a chain): 9-51.
- `id` sharing the later `old` local (a dead u8 reused): 9; `old` used for id: 14.
- No `id` statement (diagnostic only): the same E4 placement (so the hitId load does not decide).
- Permuter (perm_nu, now with ObjAttach_HitFlags' body): 17 minutes on the whole function, 17k
  iterations, nothing below base 350. Then 45 minutes with PERM_RANDOMIZE limited to the
  declarations and the head statements (scratch perm_nu_pr.py): stopped after 10 minutes
  (16k iterations), nothing below 350.
- An extra value at the chain's end: `hz += (u32)((u8 *)0 + elist[2] * 10);` (or a `u8 *base`
  local holding `(u8 *)0 + elist[2] * 10`, before or after the table statement, declared
  anywhere) puts E4 in the game's place, right after the lb and before the table's sll (31 words).
  But the pointer-to-int conversion leaves a `move v1,v0` before H, and e10 lands in v0, not v1.
  So the E4/A2 decision moves when e10 is not H's direct operand, even with the E chain created
  after the table. A copy that the colourer coalesced would leave no move (the game's code has 96
  self-moves `daddu x,x,zero`, so MWCC sometimes keeps one). No form tried gets that.

## For the lead

- Matched by statement order alone; no change outside the function.

- A per-function `#pragma opt_propagation off` is not a fix (1207 words).
- Scratch tools: vt.py (variants with `--probe`: E4's index against the
  table lui), csolve*.py (first-pass orders behind a colouring), sim*.py (the model),
  perm_nu_pr.py (perm_nu with PERM_RANDOMIZE inserted after import around a region).
