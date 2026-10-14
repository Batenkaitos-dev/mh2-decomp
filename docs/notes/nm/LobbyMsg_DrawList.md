# LobbyMsg_DrawList (lobby, src/lobby/unk_7D2C80.c)

## Status

MATCHED in round 30 (slot s2), from 23 words. Natural C, no FAKEMATCH tag. The whole file
func_diffs MATCH, ninja (13 OK).

## What matched it

The declaration order `y, y2, top, i, flags, msgs` (after `buf` and `st`; the rest as before).
y is the selected line's position, y2 the other lines' (round 29's split, which put both y
webs in the game's s5). Round 29 had found that `top, i, flags, msgs` gives the game's order
one register low, with the hoisted `0x6E - w` (created by the loop optimizer, so numbered after
every declared local) removed too early and coloured late (s4 for s0). Declaring y and y2 before
the four fixes it: the game's registers are top s4, i s3, flags s2, msgs s1, 0x6E - w s0, y s5.

## The reading (for CONTRIBUTING)

- **Short webs declared before the long ones let a created value be coloured first.** The y
  webs (live only between their definition and the icon call, inside the loop) have fewer
  neighbours than the four loop-long locals and the hoisted invariant. In a removal pass in
  number order, they are removed and lower everyone's count. Numbered after the four (declared
  after them), they are removed after the pass has already passed over the four, so only the
  invariant (numbered last) goes in that pass, before the four: coloured after them (s4).
  Numbered before the four, they go first, the four become removable in the same pass, and the
  invariant follows them: coloured first (s0). So where a created value is coloured too late
  among long-lived locals, look for short webs that interfere with all of them and number them
  earlier (declare them first).
- Tries (all in the CSV): y, y2 first with the four as top, i, flags, msgs: MATCH (either order
  of y and y2); with round 29's order of the four: 50; y, y2 after top: 70; between the four:
  55; split around the four: 68; one y for both kinds of line, declared first: 73.

## For the lead

- Nothing outside the function changed. Scratch: lm (vr.py variant runner,
  t2.py the order variants). No job left running.
