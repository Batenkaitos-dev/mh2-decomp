# EquipWindow_DrawDetails (sub_main, src/sub_main/menu_windows.c)

## Status
MATCHED in round 30, session 4 (663 -> 0 words; 1128 at the start of round 30). One tagged
FAKEMATCH: `x + 0` in the gun case's cls 10 `EquipMenu_DrawElement` call (see For the lead).

## Handoff
Nothing left. Scratch of session 4: s4 (vr2.py prints the parameter, ext,
cls, col and kind registers per variant; dl.py makes deletion variants from line ranges;
sim3.py/rp2.py replay with the real jump tables). The final body is s4/m0.c.

## How the match was found (session 4)
- **Case 3 reads the raw flags.** With `flags = flags | 8` our case 3 read the flags' extended
  copy (so that copy was live across the dispatch); the game reads the raw parameter: `flags |= 8`.
  On its own it changes no register (kind still spilled).
- **kind followed p.** The game's registers: other s0, y s1, x s2, e s3, p s4, raw flags s5,
  flags copy and kind s6, col s7, cls s8; ours had p s6, raw s4, copy s5, and the bowgun case's
  `has` took s5, the one register kind could have had. Deleting cases (dl.py) showed p's order
  was the cause, and probes showed that any one more reference to p, anywhere, gives the game's
  registers, kind included. The natural form: case 4/5 does `p = Player_PreviewWith(p, e);`
  before the inline, and the inline uses p (the game keeps the preview player in p's s4). With
  c2's od and flags |= 8: 1409 -> 427.
- Then: the inline's kind as u8 (no andi before `kind == 4`) and its locals in the order extra,
  has, plus, d (105); the gun case's chain `cAtk = cAff = cLvl = cSil = cOpt = cRng = 0`, read
  from the game's chain code (the second value of the chain is the one kept in s1: cOpt), which
  gives the game's colour slots (adiff 15, 2 words short); `x + 0` (10); the gun case's
  `buf[0], buf[1], buf[2]` store order and case 3's `ty` declared and set before `tx` (0).
- sim.py's replay over the final code matched the spill sets only with its coarse jump-table
  edges (every branch target); with the real tables (sim3.py) it fails on the game (30 vs 27):
  it is not a model of round 1, so the fix came from experiments, not from the replay.

## How the variables were found (round 30, s3)
- Spill slots are numbered top-down in MWCC's value order (docs/mwcc-matching.md, rule 6 of "How MWCC
  numbers the values later passes create"): declared variables' first webs, then created
  values, then later webs. The game's d, atk, def and names slots sit above details: so they
  are function-scope variables declared first, and the case 7 and armour-case uses are later
  webs of the same variables.
- **Later webs are numbered per variable, in the order of each variable's first definition in
  the code (dead definitions count).** Tests on this function: swapping `cAtk = 0` and `atk =`
  in case 6 swaps the later webs of cAtk and atk in case 7; the order of the case 7 chain sets
  the order of the armour case's colour webs; the armour case's own statement order and the
  declaration order change nothing. The game's order (atk, cSil, cLvl, cAff, cOpt, cAtk) comes
  only with the full chain `cAtk = cOpt = cAff = cLvl = cSil = cRng = 0;` in case 6, where
  only cAtk is used: the dead stores are removed but still order the webs. So each case began
  with the same colour reset (the blade case also keeps a dead d[9] compare: its `else if`
  form keeps the game's lone slt/bnez/b, every other form drops it).
- buf: the game's buf is below both EquipItem copies, so it is in the 16-byte-or-more group,
  and 32 bytes below t2: 17 to 32 bytes (32 chosen). t is declared before t2.
- `grey` is not a variable in the game: `flags & 8` is a CSE (slot among the temporaries).

## Scores (func_diff -D NON_MATCHING words / adiff full)
- start 1128/259. Blade case `if (od[9] < d[9]) .. else if (d[9] < od[9]) ..`: 1099.
- d, atk, def, names function-scope, first: 1093. grey removed, colours shared: 1085.
- + the case 7 chain as `cAtk = cAff = cLvl = cSil = cOpt = cRng = 0` and the blade case's
  reset `cAtk = cOpt = cAff = cLvl = cSil = cRng = 0` after atk: 879; both chains the
  latter form: 879-880. buf[32] with t first: 880 (frame grows until the spills are fixed).
- No change: chain form of the armour reset; which colour the blade d[9] compare uses;
  `p = Player_PreviewWith(p, e)` in the ammo inline (the game's preview player takes p's s4;
  ours still e's s3); the type of kind (s8..u32); single declaration moves of cls, col, kind,
  lines, cOpt (878 best, cOpt anywhere before cAtk).
- od as a function-scope variable shared by cases 6 and 7 (declared last): 1298; oatk shared: 880.
- Worse: cls as u8 (936, but frame 0x270 and 1466 instructions), col/rank/base/rankCol widened.
- Session 3 (vr.py, bodies in s3): two-arm blade compare (no else) 750 (frame
  0x2B0: od 412 and the multiplier address 176 spill); + od function-scope before cAtk, shared
  with the gun case 1119, blade only (c2.c) 1409 (frame 0x2A0, the game's slots plus kind's);
  kind or cls given one or two more uses (probes, not code): see Handoff.

## Session 3's spill rule (black-box, scratch s3/bb)
When no value can be removed, MWCC pushes the value with the fewest references (definitions and
uses), ties going to the last numbered; degree plays no part; a pushed value spills only if its
coloured neighbours hold all of s0-s8. The blade case's od/cAtk spill choice is a tie broken by
numbering (od function-scope, declared before cAtk, gives the game's).

## For the lead
- Matched with one FAKEMATCH: `EquipMenu_DrawElement(e, x + 0, y + 0x3C, other)` in the gun
  case's cls 10 branch. The game passes an int x there, re-extended (dsll32/dsra32) from case 0's
  (s32)x copy, where every other call passes the s16 x as it is. `x + 0` (a zero layout offset
  beside the other branch's `x + 0xC6`) is the only form found; plain x is 10 words off and
  `(s32)x` 15. Tagged by the ruling on a cast with no purpose; your call whether it needs the tag.
- The other changes are ordinary C: the bowgun case reassigns the parameter p to the preview
  player after its last use as the real player (the ruling on a parameter reused after its last
  use); `flags |= 8`; the inline's u8 kind; declaration and store orders; the chained resets
  (dead in the blade case, as before); a function-scope od for the blade case beside the gun
  case's block-scope od (ruled natural this round).
- The two block-scope declarations (case 2's EquipMenu_DrawSkills with an s16 y, case 3's
  Player_DrawSkillsPlain with an s16 x) are now marked `unit's own declaration:` with the word
  counts the definitions give (470 and 420).
- For CONTRIBUTING (the method): when a spilled value's neighbours fill the registers, check the
  colour order of the long-lived values (here p after the flags copy) before the spilled value
  itself; one-reference probes on each long-lived value (an extra use in any case) find the value
  whose order matters in a few compiles.
