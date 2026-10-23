# Em_TurnToward (game, src/game/em_common.c)

## The match

One FAKEMATCH tag: an `(s32)` cast on `max` in the first of the max / 2 branch's two tests,
`if (e->speed < (s32) max / 2)` beside `else if (e->speed > max / 2)`: a cast with no purpose
in C, needed only to keep the two max / 2 apart as common subexpressions. No natural form found
that does it; (u16) on max works too.

The forms, in the order they close the gap (func_diff words after each, from 45):

1. 45 -> 33: `(u16) tgt` in both of case 2's heading tests (`(u16) tgt < th`,
   `0x10000 - th < (u16) tgt`). The game compares the masked copy of tgt it made for the side
   tests (a3) there; a plain `tgt < th` reads the raw a2, so the masked copy dies early and
   diff's mask takes t2 (the game: t3).
2. 33 -> 13: hd declared before flag (anywhere between diff and flag; a full single-move sweep
   of the declarations, 306 variants). flag t1, hold t0, hd t2 as in the game.
3. 13 -> 4: after the switch, `(u16) cur >= 0x8000` (the game's `li v1,0x8000; slt v1`; `> 32767`
   gives `li at`), and `(u16) cur` at every later use (case 1/2's two tests, `cur + spd`,
   `cur - spd`). The game uses the second masked copy everywhere, so the raw reload of
   e->heading dies at the masks and doesn't share v1 with v's raw value.
4. 4 -> 0: the max / 2 branch. The game has no mask on acc in either branch, so both are
   compound (`+=`, `-=`), but `+=` there swapped the speed load and max / 2 (speed v1, max / 2
   a0; 7 words). The permuter found `max / ((0, 2))` in the second test in two minutes. By
   hand: `(s32) max / 2` or `(u16) max / 2` in either test, not both, MATCH.

## Why

- With the same `max / 2` in both tests, MWCC's common-subexpression pass shares it: a value
  numbered with the code generator's temporaries, before the speed load (which a later pass
  merges from the two tests' loads, as both branches store the field): the speed load is
  coloured first and takes v1. With two different trees (a cast in one), the pass leaves them
  apart and a later pass merges the two `srl` too, into a value numbered after the speed load:
  max / 2 is coloured first and takes v1, the speed a0, as in the game.
- The same register swap comes from the speed load's side: a plain read of e->speed in either
  branch (`e->speed = e->speed + acc`, or `- acc` in the else) makes the speed load a common
  subexpression of its own and gives the game's registers, but `x = x + y` masks the u16 acc
  (docs/mwcc-matching.md's compound-assignment entry). With compound operators in both branches, or
  constant stores in both, the speed load is merged later and numbered after max / 2 (probes
  q1-q9, b0000-b2111).

## Evidence (func_diff words)

- max / 2 held in a dead variable (th, hd, d, side1, hold, spd: u16, extra masks, 12-237; v,
  cur, tgt, s32/u32 fresh local: substituted, 4-7).
- Compare operand orders, `max >> 1` (sra: another size), `max / 2u`, `(u32) max / 2` in one
  test (1: unsigned compare), casts on the quotient `(s32) (max / 2)` (7).
- A speed local `sp = e->speed` (u16/s32/u32, every declaration position, `sp += acc;
  e->speed = sp;` forms): 7-232, propagated away (no position effect).
- s32/u32 acc (masks at every `+=`: 239), s32 acc with plain adds everywhere (221, another
  layout), s32 max (signed division: another size).
- `em->turnSpeed` for e->speed in some of the branch's accesses: no change.

## Rule

Evidence for "How MWCC numbers the values later passes create": two equal expressions in two
tests of an if / else-if chain are one common subexpression; written as different trees, a
later pass merges them into a value numbered after the code generator's temporaries. And a
field load shared by the two tests is merged the same way (numbered late) when both branches
store to the field with compound operators, but is a common subexpression numbered early when
either branch reads it in a plain expression.
