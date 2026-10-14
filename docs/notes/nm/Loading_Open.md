# Loading_Open (sub_main, src/sub_main/unk_3D4100.c)

## Status
Matched in round 30 (s2), from 18 words; unwrapped. The file's 9 functions MATCH; it has no
NON_MATCHING block left.

## What matched it
Two changes, both natural C:
- The declaration order: `buf` declared first and `t` (the sprite table pointer) after
  `base`. Found by a sweep of every single declaration move, then a second move from the
  best states. It put t in t8 and the outer select walker in s1, as the game (17 words).
- One counter for the sequential loops: `i` counts the images, the palettes and the select
  packets, and then holds the random index (`l` and `j` are gone). Its later webs are
  numbered after the walkers MWCC creates (docs/mwcc-matching.md, "How MWCC numbers the values later
  passes create", rule 1), so each loop's counter is coloured before its walker: the select
  counter s2 below its walker s3 (9 words), then the palette counter s1 and its cbp/clut
  walker t8 (0).

## What was seen on the way
- The palette walker and the outer select walker took the same register in 802 of 870
  declaration variants (both t8, or both s1): with `l` as the palette counter no order gave
  the game's pair (t8, s1). The counter's later web broke the coupling.
- Registers that look free in the final code were skipped (t9 in the k loop at 18 words, t8
  in the palette loop at 9): the colouring order, not a visible neighbour, decides there.
- An unused declared local can still shift the numbering (removing an unused `s32 j` changed
  the registers at 9 words); the unused `l` of the match made no difference, and is removed.
- Isolated tests (scratch tgen.py, test functions appended to the file so
  the static callees' bodies are known): values crossing only Tex_SelectPacket (it clobbers
  v0-t5) take t6..t9 first-fit in reverse numbering, then s registers; Loading_WriteSprite
  clobbers v0-t4.
- Tools: scratch lo.py compiles many variants of the function in one file and prints the
  registers of the k loop and the palette loop for each (one line per variant); vr.py scores
  variants with func_diff. Both worked well for a block whose words were all registers.

## For the lead
Nothing outside Loading_Open was changed.
