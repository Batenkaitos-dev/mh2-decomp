# Model_Draw (sub_main, src/sub_main/sys_main.c)

## Status
Matched in round 30 (s6), from 7 words. Unwrapped; the file's 198 functions MATCH;
Font_AddGlyphSprite (77) and Font_DrawGlyph (7) stay NON_MATCHING and still compile.

## What matched it
The program CALL packet in the game's store order (q[0], q[1], q[2..3]) with the END tag
written through its own pointer, the one AddChain gets as `last`:

    last = q + 8;
    last[0] = 0xF0000001;
    last[1] = 0;
    *(u64 *)&last[2] = 0;
    ...
    DmaQueue_AddChain(0, (u32)q & 0x0FFFFFFF, last, &gDmaQueue);

Natural C (a pointer to the tag that is then passed on); no tag. Setting `last` before the
q[0..3] stores matches too (on the test copy).

Why (as far as observed): with q[8] stored through q, the first schedule hoists the
0xF0000001 lui into the free slot beside 0x50000001's lui, so the two interfere and
0x50000001's lui and ori both take a0 (9 words). Through `last`, the lui of 0xF000 comes after
0x50000001's ori in the first schedule, as in the game: lui v1 / ori a0,v1, then lui v1,0xF000
after the q[0] store.

## How it was found
A 20-line copy of the block (`if (gProg != e->prog) { ... }` and the second AddChain, e as a
parameter kept in s2) reproduces the build's code for both the committed order (7) and the
game's order (9). Scratch gen.py builds one test function per variant and
compares the block with the game's 21 instructions (one compile for all variants). Tried there:
the q[8] store last (luiF000 still hoisted: constants fill the first free slot whatever their
store's place), no 0xF0000001 (one register for 0x50000001), inline DMA tag setters (call
packet, end tag, both, tag/addr parameters: 8-14), then `last` (MATCH).

## For the lead
Nothing outside Model_Draw was changed. The method may help other store-order blocks: a
pointer local for a sub-packet makes its stores wait for the pointer in the first schedule.
