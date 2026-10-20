# NgWord_Check (select, src/select/select_ngword.c)

## Status

Matched in round 30 (s5, branch decomp-b30-s5): 6 words at the start, 0 now.
Every function of select_ngword.c matches; ninja (13 OK).

## What matched it (round 30)

Round 29 left 6 words: the game numbers k, j, the buf walker, the word
pointer, wc, nc (t7..t2); ours numbered nc and r before the walker.

1. `word[k]` in place of the `r` pointer (`r = word; ... r++`): MWCC makes a
   second walker for it, created after buf's, so walker t5 < word walker t4
   as in the game. 14 words (the walkers right, wc/nc still numbered at their
   declarations, before both walkers).
2. wc and nc made later webs (docs/mwcc-matching.md, "How MWCC numbers the values later
   passes create", rule 1). A contrived test first (scratch c3/c4/c5/c6):
   - nc alone with an earlier web: nc t2 right (11 words);
   - both: right only when wc's earlier web comes first in the code (c5);
     nc's first gives wc/nc swapped (c4, c6, and p1 below: 9). So later webs
     are ordered by where each variable's first web is, not by where the later
     webs are defined (both are in the same loop test here), and not by the
     declaration order.
   - single-use first webs are substituted away and change nothing:
     `c = buf[0]; if (c)`, `(c = *w) != 0` in the for's test, dead stores,
     an initializer (g1-g6, h1-h3: 14).
3. The natural first webs, in the code the game already has:
   - nc: the record loop `for (w = list, nc = *w; nc != 0; w += 0x10, nc = *w)`
     (two definitions reach the test, so the web is real): 11 (k2);
   - wc: the outer loop `for (wc = *p; wc != 0; wc = *p)`: MATCH (u6). The
     increment's `*p` is a CSE of the scan's last load, so the bottom test
     re-extends it (`dsll32/dsra32 v0,a0`), as the game does: the old
     `while ((s8)c != 0)` cast is gone. `while ((s8)wc != 0)` with `wc = *p`
     before it matched too (m4, n0), but the cast has no purpose;
     `while (wc != 0)` there gives 17 (no extension), `while (*p != 0)` gives
     the extension but no first web for wc (14).
4. Clean-ups, each still MATCH: the scan as the lobby twin writes it
   (`while (*p != 0) { if (found) break; p++; if (!ctype(name[i++]) &&
   ctype(*p)) break; }`), `gCtypeTable[x + 1]` as in WordFilter_Scan,
   `if (n == -1) continue;`, the found block's two tests joined with &&,
   buf[j] read directly (no c local), wc/nc renamed c1/c2, declared c1, c2.

Other scores this session: an explicit buf walker
q (91: MWCC then keeps &buf in a saved register, frame +16); the lobby twin's
whole form (t1: 59; its j loop with CSE temporaries in this function, A1: 39,
every j-loop register reversed); nc/wc/c retyped in the word[k] state (a1-a7:
14-101); nc or wc declared at block scope (14).

## For the lead

- docs/mwcc-matching.md, "How MWCC numbers the values later passes create", rule 1
  says later webs are numbered "in code order of their first definition".
  Here both later webs are defined in one loop test (wc first) and they were
  numbered in the order of the variables' first webs (c4/c5/c6, p1). Worth a
  line in the rule: the order of later webs follows the order of the
  variables' first webs.
- A first web must be a real one: a definition used once is substituted away
  (no web); two definitions reaching one use (a for's init and increment)
  make one.
- The `(s8)c` cast pattern (`while ((s8)s[i] != 0)` also in
  sub_main/kbd_input.c:1468) may be the same CSE-of-a-load effect: the
  extension comes from a byte load CSE'd across blocks.
