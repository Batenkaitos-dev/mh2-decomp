# NgWord_Check (select, src/select/select_ngword.c)

The game numbers k, j, the buf walker, the word pointer, wc, nc (t7..t2). Below, wc and nc
are the outer and record loop values (named c1 and c2 in the source, declared c1, c2).
Word counts give the miss left at each step, from 6 words with an `r` pointer and wc/nc as
first webs (nc and r numbered before the walker).

## The match

1. `word[k]` in place of the `r` pointer (`r = word; ... r++`): MWCC makes a second walker
   for it, created after buf's, so walker t5 < word walker t4 as in the game. 14 words (the
   walkers right, wc/nc still numbered at their declarations, before both walkers).
2. wc and nc made later webs (CONTRIBUTING, "How MWCC numbers the values later passes
   create", rule 1), with natural first webs in the code the game already has:
   - nc: the record loop `for (w = list, nc = *w; nc != 0; w += 0x10, nc = *w)` (two
     definitions reach the test, so the web is real): 11;
   - wc: the outer loop `for (wc = *p; wc != 0; wc = *p)`: MATCH. The increment's `*p` is a
     CSE of the scan's last load, so the bottom test re-extends it (`dsll32/dsra32 v0,a0`),
     as the game does, with no `(s8)` cast. `while ((s8)wc != 0)` with `wc = *p` before it
     matched too, but the cast has no purpose; `while (wc != 0)` there gives 17 (no
     extension), `while (*p != 0)` gives the extension but no first web for wc (14).
3. Forms that each still MATCH: the scan as the lobby twin writes it (`while (*p != 0) { if
   (found) break; p++; if (!ctype(name[i++]) && ctype(*p)) break; }`), `gCtypeTable[x + 1]`
   as in WordFilter_Scan, `if (n == -1) continue;`, the found block's two tests joined with
   &&, buf[j] read directly (no c local).

## Why

A contrived test of the later webs:
- nc alone with an earlier web: nc t2 right (11 words);
- both: right only when wc's earlier web comes first in the code; nc's first gives wc/nc
  swapped (9). So later webs are ordered by where each variable's first web is, not by where
  the later webs are defined (both are in the same loop test here), and not by the
  declaration order.
- single-use first webs are substituted away and change nothing: `c = buf[0]; if (c)`,
  `(c = *w) != 0` in the for's test, dead stores, an initializer (14).

For the models:
- CONTRIBUTING, "How MWCC numbers the values later passes create", rule 1 says later webs
  are numbered "in code order of their first definition". Here both later webs are defined
  in one loop test (wc first) and they were numbered in the order of the variables' first
  webs. A line for the rule: the order of later webs follows the order of the variables'
  first webs.
- A first web must be a real one: a definition used once is substituted away (no web); two
  definitions reaching one use (a for's init and increment) make one.
- The `(s8)c` cast pattern (`while ((s8)s[i] != 0)` also in sub_main/kbd_input.c:1468) may
  be the same CSE-of-a-load effect: the extension comes from a byte load CSE'd across blocks.

## Evidence

An explicit buf walker q: 91 words (MWCC then keeps &buf in a saved register, frame +16);
the lobby twin's whole form: 59; its j loop with CSE temporaries in this function: 39, every
j-loop register reversed; nc/wc/c retyped in the word[k] state: 14-101; nc or wc declared at
block scope: 14.
