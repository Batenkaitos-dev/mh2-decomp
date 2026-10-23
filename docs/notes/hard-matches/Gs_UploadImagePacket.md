# Gs_UploadImagePacket (sub_main, src/sub_main/loading_screen.c)

## The match

```c
fmt = dpsm & 7;
bits = 8;
if (fmt == 0) {
    bits = 8;
}
if (fmt == 1) {
    bits = 6;
}
...
```

Also MATCH: `s32 bits = 8;` at the declaration with the same tests; `bits = 8;` before
`fmt = dpsm & 7;`.

## Why

The format 0 test repeats the default. MWCC drops its store (and the test) but the block
boundary stays: block A is `andi v1,t0,7; li t4,8` (in creation order), block B is
`li v0,1; bne v1,v0`. The delay-slot filler looks only inside B, where the last
instruction is the compare constant the branch needs, so the slot stays a nop: the game's
head. Without the boundary, the scheduler puts li t4,8 (height 1, live out) after li v0,1
(height 2) and the filler moves it into the slot.

The method: an empty slot in the game after `x = K; li vR,C; bne` (x set before the
compare constant, in the same block) comes from a block boundary between them; a redundant
`if (cond) x = K;` that repeats x's current value gives that boundary with no code. This is
a candidate for the catalogue entry "Delay slots filled or empty" in docs/mwcc-matching.md.

The form is not tagged FAKEMATCH: `if (fmt == 0) bits = 8;` is read as the ruling's "store
that every other branch also writes, though the value is already set" (each format case
writes bits; the chain is a full table of formats 0 to 4). The function's comment explains
it. If it is ruled a fakematch, `FAKEMATCH:` goes on the comment's last sentence.

## Evidence

- Not matching: `if (fmt == 5) bits = 8;` as the extra test (a real compare stays, 7 lines
  off); `if (fmt == 0) bits = 8; else if (fmt == 1) ...` (9).
- Scans of the game's asm: an independent ALU instruction right before a compare `li`, with
  the branch slot left as a nop, occurs once in the whole game: here. The opposite (an
  independent `li` in the slot after a compare-li branch) occurs 6360 times. So the
  scheduler and the filler behave as in our build everywhere, and the original source had
  to give the head another block structure, not another scheduling rule.
- `#pragma schedule off` gives the game's head (creation order, and no slot filling: the
  filler belongs to the scheduling step) but 52 lines off elsewhere.
- Probes that changed nothing: optimize_for_size, peephole, the opt_* pragmas, `#pragma
  processor`, -g / -sym, levels 3 and 4 (levels 1-2 keep the head but change the rest),
  register pressure (8 live parameters plus globals), a stack frame, the chain in a
  non-entry block, static inline (it is inlined into Loading_Open).
- Side finding: `if (c) x = A; else x = B;` (u32 operands) is made a triangle by the
  delay-slot filler (it fills the bne slot from the else block and retargets the branch);
  with `#pragma schedule off` it stays a diamond.
