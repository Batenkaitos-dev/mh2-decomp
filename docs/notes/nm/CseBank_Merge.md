# CseBank_Merge (main, src/main/cse_bank.c)

## Status
Matched in round 30 (s6), from 31 words; unwrapped (the `#pragma optimization_level 4` pair
stays, with its comment). Every function in cse_bank.c MATCHes; the file has no NON_MATCHING
block left. History: CSV rows, and round 29's nm-wait notes (rules 9, 10).

## What matched it
Two changes, both natural C:
- `u32 max` (the program-count search): the search reads `if (max < src->h.max)` (the game's
  sltu, now without casts), the -1 fill loop `for (i = 0; i < (s32)max + 1; i++)` (the game's
  slt), and the size `size += (max + 1) * sizeof(s32)`: 31 -> 8. This fixes the Prog fill loop
  (unrolled x8): the game keeps the first guard's max + 1 for the leftover guard and computes
  it again for the leftover loop; with a plain s32 `max + 1` in both the loop and the size, ours
  did the reverse (i, limit and walker in other registers).
- The three pad loops as `for (; size & 0xF; size++) ((u8 *)chunk)[size] = 0xFF;` (the `pad`
  walker local is gone): 8 -> 0. The game sets the walker in the loop's preheader, after the
  `size & 0xF` guard (the loop optimizer's walker); `pad = chunk + size` in the for's init put
  it before the guard (the Smpl and Vagi `max--` lines scheduled around it).

## Seen on the way (scratch cse: gen.py compiles many variants in one file)
- A same-type copy of max as the bound (`n = max; i < n + 1`, any place, block scope): 31,
  the copy is propagated. Bound read as `prog->h.max + 1` or `<=`: 336-337 (reloaded).
- With s32 max: loop `(s32)max + 1` (a no-op cast) and size `((u32)max + 1) * 4`: 8; plain
  loop with that size: 31; `(s32)((u32)max + 1)` 29, `(s32)(u32)max + 1` 28.
- With u32 max: `(s32)(max + 1)` 29, `i <= (s32)max` 40; the size as `((s32)max + 1) * 4`: 23.
  So the loop bound's `max + 1` must not be the same expression as the size's.
- The same C at level 3: 59 words (registers only).

## For the lead
Nothing outside the function changed. The `(s32)` in the fill loop's bound is what makes the
compare signed with an unsigned max (without it: sltu); I judged it natural (it has a purpose).
