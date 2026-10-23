# CombShop_DrawAmount (lobby, src/lobby/combine_shop.c)

## The match

FAKEMATCH tag: the per-branch `id = item` (see "The FAKEMATCH").

    u16 item = m->item[m->page];
    s16 ly;
    s32 wy;
    s32 wx;
    s32 id;
    s32 tx;
    s32 n, price, tot;          /* before or after the others: same */
    ...
    ly = wy + 0xD2;
    ...
    Font_SetPos(tx, ly += 0x18);
    ...
    Font_SetPos(tx, ly + 0x18);

(A 22-word form had an s32 ly with `ly = (s16)(ly + 0x18);` as a statement and
`(s16)ly + 0x18` at the end, and the declarations wx, tx, wy, item, n, id, ly.)

## Why

- The game's ly holds s5 in both webs (the first from 0x778798, the second
  from 0x778820). The second web's only saved neighbours are m, item and tx
  (s0, s6, s1): as a node of its own it would take s2. So in the game ly is
  one node.
- regalloc_predict's simulate on the game's graph: with ly's webs separate, no
  order of the 7 locals gives the game (0 of 5040); with them as one node,
  exactly one of 720: item, ly, wy, wx, id, tx after the parameters. With the
  22-word form's order the same replay gives that form's registers exactly.
- A statement `ly = (s16)(ly + 0x18)` (s32 or s16) splits ly: in the game's
  order the second web is a separate node, so item has 7 neighbours left at
  its turn, waits a pass and takes s0 (61-117 words). `ly = ly + 0x18` and
  `ly += 0x18` on an s32 ly keep one node but recompute the sum at each use
  (46, every saved register right). Only s16 `ly += 0x18` as the call's
  argument stores the extended sum in ly's own node: MATCH.
- In the same order: `Font_SetPos(tx, ly = ly + 0x18)` 117; s32 ly with
  `ly += 0x18` in the argument 104; `ly += 0x18;` as a statement 117;
  `ly = (s16)(ly + 0x18)` in the argument 140; `register` 61.

## Evidence (func_diff words)

- The game's order with the 22-word form's ly forms: 61; with s16 ly and the cast or
  `ly = ly + 0x18`: 117 (s16 `ly = ly + 0x18` also re-extends ly first).
- No id (item passed, MWCC's CSE of the conversion): 17 (created, so numbered
  after tx). id once before the if, at its declaration or before Win_Draw:
  175-206 (the andi before the branch). `id = item` inside each call's
  argument: MATCH too.
- No wx/wy/id/tx locals: 138-156; tx written out (wx + 0x28 at each use): not
  kept across calls (106-143). No wx: 28. item declared last (the sibling
  CombShop_DrawLineWindow's place): 46; ly before item: 20.
- ly as x's later web (x = wy + 0xD2, as a parameter: s16 re-extends, x's
  node waits and takes s0, 110-119) or a local copy `s16 lx = x` (propagated
  into the parameter, ly still split: 117).
- A test copy shows a local's later web is a node of its own; an earlier test
  fits a parameter's later web being part of its node (or numbered next to it).

## The FAKEMATCH

`id = item;` in each branch of the Item_IsDeco test. Without a declared id, MWCC's own copy of
the conversion is numbered after tx (17 words); id assigned once before the if computes it
before the branch (175). `id = item` inside each ItemInfo_Draw call's argument matches too.
Under the rulings it is a per-branch copy that mirrors the allocation, so it is tagged; it
would not be if the in-argument form or the statements read as the unit's style.

## Rule

A candidate for CONTRIBUTING, "How MWCC numbers the values later passes create": a local
updated by `v = (s16)(v + c)` as a statement is split into two nodes; an s16 `v += c` used as
a call's argument keeps one node and stores the extended sum in it. When a later web holds its
first web's register though a lower register is free for it, the game's variable is one node:
look for the update inside an expression.
