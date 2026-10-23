# Page_Click (lobby, src/lobby/page_input.c)

## The match

The game colours the loop head's values gBrowserWork t1, gBrowserState t0, gBrowserSub a3,
the object a2, `a` a1: numbered Work < State < Sub < o < a. With the globals read directly,
o is numbered first (a declared local's first web, before every created value) and the three
global loads after it (common subexpressions, numbered at their first occurrence here), 21
words off.

The three globals held in declared locals, declared before o:

```c
    const BrowserWork *w;
    const BrowserState *st;
    const BrowserSub *sb;
    BrowserObj *o;
    ...
    for (i = 0; i < 500; i++) {
        w = gBrowserWork;
        o = w->obj[i];
        ...                       /* null and active tests */
        st = gBrowserState;
        sb = gBrowserSub;
        x0 = o->x - (f32)st->x;
        ...                       /* hit test, listObj branch, styleWait/fade: st, sb, w */
                gBrowserSub->listObj = Select_ListClick(sb->listObj);
```

(In the source the three are named work, state and subState.)

## Why

- The `const` is what keeps them: MWCC propagates a plain same-type copy of a global (96
  variants of w/st/sb copies, all 21 words), but not a copy between pointer types, and
  `const T *` from `T *` counts as one. As declared locals they are numbered at their
  declarations, before o.
- The rule, for "Which locals are values of their own": a `const T *` copy of a `T *` global
  is a local of its own (numbered at its declaration), where a plain copy is propagated
  away. The source treats the const views as natural (no FAKEMATCH tag); the same reading is
  used for a parameter copy in Player_DrawHeldProp.md.

## Evidence

- `(T *)(void *)gX` or `(T *)(u8 *)gX` casts match too (worse: casts with no purpose).
- Needed: all three (subsets 13-21 words); w, st, sb in that order before o (other orders of
  those four 8-21; the other locals' places are free: 144 of 188 single declaration moves
  match); st/sb assigned after the active test (all three at the loop top: 14). The store
  after Select_ListClick goes through gBrowserSub, as the game reloads it after the call (sb
  is const anyway).
- An unused `BrowserObj **objs` has no effect.
- A later web of o gives the game's loop head too: a contrived real web of o before the loop
  (`o = obj[0]; if (o == NULL || o->kind == 99) return;`) makes everything from the loop's
  init on match (tail 0), and so does the object load in a while or for condition (the
  condition is after the body in the IR, so its web is a later one), but those change the
  code (750+ words). A real web after the loop or a two-definition web inside it changes
  nothing: the first web is the first in code order.
- No change (21): inline accessors for the object, redundant `o = gBrowserWork->obj[i]`
  reloads after each test (copies of the CSE), o assigned after CSE'd obj[i] reads, do-while
  and goto loops, the unused objs.

## Method

Compile all variants in one file (helpers inserted before the function where needed) and
score each; for a loop, compare from the loop's init on, so a head that matches shows even
when the code before it differs.
