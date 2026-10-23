# PriceInfo_Open (lobby, src/lobby/market.c)

## Status

Round 30 (s3, branch decomp-b30-s3): the block is unchanged (99 words by func_diff, the
defined `cnt >= 1` folds to one instruction fewer), but **a full match exists** and waits
for two rulings (below, "For the lead"). With the edit below the file is 11 of 11 MATCH
unwrapped, and the checks pass (13/13 targets) on that state (scratch
pi/cand_unk_7106D0.c, check_cand.log).

## Handoff

- If the lead accepts both points: apply "The matching edit", unwrap the block, func_diff the
  file, run the checks, commit "(+1 matched, -1 NON_MATCHING)".
- If the flag is refused: nothing defined on every path keeps the game's `li 1; sltu` (ten
  rounds of forms, CSV). The pointer part alone, with `cnt >= 1`, gives 92 (the 1 folds).
- If only the FAKEMATCH part is refused: a natural form would need two u16 pointer variables
  whose webs come before and after the counter's (see "What the numbering needs").

## The matching edit (against the block as it is)

```c
    u32 cnt;
    u32 one;                            /* 1 once the list has a monster; unset with k */
...
    if (id != 0) {
        a = w;
        b = w;
        one = 1;
        do {
...
    /* FAKEMATCH: list and p, dead after the first loop, hold the counts'
     * addresses, and area (not yet used) holds n * 2: as later webs of list
     * and p around the counter's, and area's first web, they take the game's
     * registers (captured a2, i a1, hunted a0, n * 2 a0). */
    i = 0;
    area = n * 2;
    c = w + area;
    list = (s16 *)&gSave.captured[k];
    p = (s16 *)&gSave.hunted[k];
    for (; i < 5; i++) {
        cnt = (u16)*p + (u16)*list;
        if (cnt >= one) {
            *(s16 *)(c + 0xCA) = *tbl;
        }
        tbl++;
        c += 2;
        n++;
    }
```

Also matching: `one = 1;` right after `k = id;`, after `found = 0;`, or after
`PriceInfo_IsListed(id);` (every path through the loop body); `s32 one`; `found == one` in
the first loop; the loop-2 set-up as a for-init `for (i = 0, area = n * 2, c = w + area,
list = ..., p = ...; i < 5; i++)`; `*(u16 *)p + *(u16 *)list`. Not matching: `one = 1` at the
loop's end (95), `i = 0` after `area = n * 2` (5), `(s16 *)gSave.hunted + k` (5).

## For the lead

1. **The flag.** The game itself reads s7 (k) unset when the monster list is empty: k is set
   only in the first loop's body, and the second loop reads `gSave.hunted[k]` regardless
   (0x711368 `dsll32 v0,s7,16`). `one = 1` beside `a = w; b = w;` (or beside `k = id;`) is
   unset on exactly that path and no other. There the game's result is already garbage (the
   counts of a garbage monster index), so the unset `one` adds no undefined behaviour on a
   path where the game's behaviour is defined. By the letter of "Not accepted" the original
   does set the value there (`li v0,1` at 0x7113A4), so this is a new case for a ruling.
   Without an unset path MWCC folds the 1 (`beqz`), one instruction short: 99 words.
2. **The FAKEMATCH part:** `list` and `p` (s16 *, dead after the first loop) reused with casts
   for the u16 count addresses, `area` (s32, used only by the final switch) for `n * 2`.

## What the numbering needs (round 30 s3; tempcolor.py and variant sweeps)

Loop 2's set-up in the game: n * 2 a0, i a1, hunted address a0, captured address a2.
- i must be a later web (the loop-1 counter reused): a first web (`j`) is coloured after both
  hoisted addresses (a2). With i a later web and both addresses hoisted, i is coloured first
  (a0): 9 words.
- The hunted address must be numbered after i's later web: a later web of a variable whose
  first web follows i's (p, a, b work; list, whose first web precedes i's, gives 8).
- The captured address must be numbered before i's later web and created before the hunted
  one (the final schedule puts its `addiu` first). Hoisted it is numbered right but created
  after a pointer set before the loop (2 words, only the two `addiu`s swapped); as list's later
  web (first web before i's) both hold.
- n * 2 must be a first web (area's: numbered at its declaration) created after `i = 0`;
  a plain temporary (`c = w + n * 2`) gives 5, n * 2 in found or id (later webs) 9.

## Tries this session (func_diff words; scratch pi)

- The flag set beside k (`k = id; one = 1;`, u32 or s32, `found == one` too): 9, as round
  22's flag (the flag's place on the loop-body paths does not matter).
- Defined thresholds: `one` u8/s8/u16/s16/u32/s32/`register`/const at its declaration or
  before loop 2: 99; u64/s64: 13 (the 64-bit compare folds after a zero-extension); `for
  (one = 0; one < 1; one++);` and a for-init `one = 1`: 99.
- Loop-2 counter as j (16 positions of declaration), found, id, area: 9 to 17; loop-2
  statement orders x for/do/while (72): 9 best; c forms (indexed, tbl[i], c before the tbl if):
  9 to 129; n * 2 in found/id/area (16 combinations, before or in the for-init): area in the
  for-init after i = 0: 7, others 9.
- Pointer webs: p/a/b for the hunted address with area: 2; list for captured as well: MATCH.
- perm_nu.py from the flag state (15 min, stopped once the hand match was found): best was
  round 22's invalid `n = n * 2` (7).

## Matched (the lead, round 30)
The lead applied this session's candidate (cand_unk_7106D0.c). The unset
`one` is the game's own unset read: the matched code is the game's byte for
byte, so the game's register for the 1 is set only when the list has a
monster, and it compares against it unset when the list is empty, as it
indexes with k unset there (CONTRIBUTING, "Fakematches": a read of a
register the game itself never set on that path). Loop 2's set-up (list, p
and area reused for the game's registers) is tagged FAKEMATCH.
