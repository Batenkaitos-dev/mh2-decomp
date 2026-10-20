# DlgInput_Draw (lobby, src/lobby/browser_dialogs.c)

## Status

Matched in round 30 (s4), from 17 words (44 in round 29's w_after state). Natural
C, no tag. Every function of the file MATCHes; ninja (13 OK).

## What matched it

No `w`, `p` or `pp` local after the copy loop: the field test and the switch
index read `gBrowserWork->obj[gBrowserSub->listObj]` in full, and the cases
read `gBrowserWork->kbdText`, as the rest of the function does:

    gBrowserWork->obj[gBrowserSub->listObj]->s2 -= n;
    if (gBrowserWork->obj[gBrowserSub->listObj]->field == 4) {
        switch ((((u8 *)gBrowserWork->obj[gBrowserSub->listObj]->s1)[1] - '0') + ...) {
        case 0:
            memcpy(D_0089534D, gBrowserWork->kbdText, n + 1);

## Why (round 30)

- The game's gBrowserWork load after the `-=` store (v1, live into the cases)
  and the constant 4 of the field test (a0) both have 23 or more neighbours
  (38 and 29 in the game's graph; scratch nb.py), so both wait in the first
  colouring pass and the later numbered one is coloured first. The game needs
  the load numbered after the 4.
- A global load used again is a common subexpression numbered at its second
  occurrence (Gs_ClearFrame's rule). With the chain written out in the
  switch index, the second occurrence is the index's `s1[1]` read: after the
  4, before the jump table's values. In round 29's w_after (`w =
  gBrowserWork; pp = gBrowserWork->obj;`) the second occurrence is pp's,
  before the 4 (both wait; the 4 is coloured first). With pp alone (no w)
  it is case 0's memcpy, after the jump table: the load is then removed in
  the first pass (its neighbours are gone by its turn) and coloured after
  the 4, which waits.
- Replay on the game's graph (scratch rep.py, tempcolor's simulate): the
  load numbered anywhere from just after the 4 to just before the index's
  `sll` for the table gives w v1 and the 4 a0; numbered first or after the
  table, w t4 and the 4 v1.
- The `(u8 *)` casts on `s1` (a `char *` field) give the game's lbu.

## Tried this session (func_diff words; scratch v1-v6.py)

From w_after (44; ours 4 v1, w a3 in every one): w's case-1/2 reassignments
dropped (44); no w (44); `p = gBrowserWork->obj[...]` or `&gBrowserWork->obj[...]`
(46); a first web for w in the copy loop (L1-L3) or at the kbdText test and
max/strlen (C1, C2): 44; the tail as a static inline helper taking w or not
(44); cast copies `(char *)gBrowserWork` into a char * local or into `text`
(the Browser_Keyboard result) as a later web (X1-X6: 44-45); `text =
gBrowserWork->kbdText` (197). Then the chain in full in the test and the
switch (S1a): MATCH; with pp kept for the test or p for one of the two
index reads (S1b-S1e): 165-170.

## For the lead

- Nothing outside the function changed.
- Rule worth a line in docs/mwcc-matching.md ("Values with 23 or more neighbours"):
  two waiting values are ordered by their numbers, and a reused global load
  (a CSE) is numbered at its second occurrence, so where the second
  occurrence falls (here: the switch index or the first case) decides which
  of the two is coloured first. tempcolor's fitted order (w after the switch
  index) reads the colouring without the waiting rule; replaying with
  tempcolor's simulate on the game's graph showed the window.
- tempcolor cannot read our w when it is in a3: the memcpy calls count a3 as
  an argument, so the web is merged with the call webs (223 neighbours,
  precoloured).
