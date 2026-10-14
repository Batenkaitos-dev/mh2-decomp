# CardTask_SelectFile (sub_main, src/sub_main/unk_3DDBA0.c)

## Status
Matched since round 17, tagged FAKEMATCH (`s32 file = file_in;` declared first). Round 30, s5
(fakematch review): **no natural form found; the tagged match stays.** Every natural form tried
compiles to 12-17 words different (scores below).

## Handoff
Nothing in progress. Reopen only with a new idea for a value numbered between the two
parameters and `t` (see "Why a copy" below), or if the lead rules one of the forms in "For the
lead" acceptable.

## For the lead
- No prototype change helps: the parameter is an int index (callers pass `slot + 5`,
  `m->charNo + 5`, 4 and 8, all s32 expressions), so there is no `void *obj`-style type to give
  it. A u32 parameter (with the file's prototype) is 17 words; u8/u16/s8/s16 were 27/55 in
  round 25. No declaration was changed (the four `(s32 a, s32 b)` prototypes: unk_3DDBA0.c:134,
  unk_3D54B0.c:31, 1760, 3063).
- The only other match found is the same trick in another type: `u32 f2 = file;` declared
  first and used only in the loop's `f2 != i` (0 words). Not better than the tagged form.

## Why a copy (the allocator model, docs/mwcc-matching.md, "How MWCC picks saved registers")
The game has clear in s2 and the loop's file in s1; slti and the `file * 16` index read a0
directly (a same-type copy is propagated into uses outside loops, not into the loop). With
the parameter used directly, file is value 1 and clear value 2; both are params, live from the
entry, and clear's range contains file's, so they go in the same pass in number order and file
(numbered first) takes the higher register (s2). The game needs the loop's file value numbered
after clear and before `t`: a copy declared after `t` or `i` is 12 words, a created value
(narrow parameter's extended copy) comes after every local too. Only a local declared first
holding file gives that, and nothing in the function gives such a local a purpose.

## Round 30 s5 tries (full words; the tagged form is 0)
| form | words |
|---|---|
| parameter used directly | 17 |
| static inline helper for the mask loop, s32 / u32 / s16 parameter | 17 / 17 / 23 |
| static inline helper for the whole if body (u32 file) | 17 |
| extern wrapper around a static inline with the same body, s32 / u32 (+ `(s32)` test) | 17 / 14 |
| copy declared after `i` / between `t` and `op` | 12 / 12 |
| u32 copy used only in the loop, declared after `i` / first | 12 / 0 (same fakematch) |
| `const s32 file` / `register s32 file` parameter | 17 / 17 |
| no `t` local (gCardTask. everywhere) | 60 |
| early `return` for file < 4 | 17 |
| u32 parameter (file's prototype too) | 17 |
| K&R definition declaring clear before file | 17 |
The inliner substitutes a plain variable argument (no copy), so no inline form helps.
