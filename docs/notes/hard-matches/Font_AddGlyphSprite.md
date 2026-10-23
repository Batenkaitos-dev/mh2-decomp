# Font_AddGlyphSprite (sub_main, src/sub_main/sys_main.c)

## The match

Font_AddGlyphSprite is static (its only caller is Font_DrawGlyph, in the same file). The far
texel corner is computed in place just before the stores,

    tw = u + tw - 1;
    v += th;
    ((u64 *)p)[8] = (u32)(tw * 16 + 8) | ((u64)(u32)(v * 16 + 8) << 16);

with tw declared before th (th first: 24 words).

## Why it works
- `v += th;` keeps v one node, so the sum stays in v's t5 (CONTRIBUTING, "One node or two").
- tw is a dead variable reused (an accepted form), so `u + tw - 1` is a value of its own (a
  later web), created before `v += th` while th still holds a1: it stays in a3 as in the game,
  and `v << 4` takes a1 once th is dead (an inferred reading; `v += th;` first gives 21). With
  `u + tw - 1` inside the expression it took a1 whatever the operand order (19 words).
- The two statements can go before the in-place adds, after them, or between the word 7 and
  word 8 stores (all 0); `v += th;` before the tw line or at its earlier place: 21.
- Mini test (parameters as u, v, tw, th): with `(u64)(u32)(v * 16 + 8)` MWCC creates the V
  part first; with `(u64)(v * 16 + 8)` or `* 0x10000` the U part first; the colouring follows
  that creation order (the model holds). In the whole function the U-first forms gave the V
  chain a1 but left `u + tw - 1` in a1 too (presumably no overlap with `v << 4` in the first
  schedule).

## Evidence: word 8 forms
- 176 word 8 spellings on the v += th state (U forms x V forms x operand order): best 19.
- Later webs for one part with v += th first: th = v * 16 + 8 (21: V chain a1, u + tw - 1
  still a1), slot, u, tw = (u + tw - 1) * 16 + 8 (17-21); u64 local in two statements with
  v += th between (35).
- (v += th) inside word 8, 176 forms: 91 (v coloured a0 throughout).
- Permuter from the v += th state, 9 minutes: 18 at best.

## Evidence: the rest of the function
Method: many variants of the function compiled in one copy of the file (0.15 s for one, about
1 s for 180), scored like func_diff (positional) and aligned; declaration orders climbed by
one-line moves, or every order of named declarations tried.
1. Tail adds in place (`x0 += ox; x1 += ox; y0 += oy; y1 += oy;` before the stores): the game's
   tail order (word 8's parts, then y0/y1 + oy, then x1/x0 + ox) and p/word2/word3 in t7/t6/t5,
   but 2 nops (first branch slot, bc1t slot). Climb: aligned 41. Only the first add's place
   matters (24 orders).
2. y forms on that: `y0 = y; y1 = (s16)y + h; y0 *= 16; y1 *= 16;` (y0 a copy scaled in place)
   plus a climb: 2 words, 194 instructions. `y0 = gFontCache->y; y1 = (s16)y0 + h;` (no y) also
   2. s16 y without the cast 8; `y0 = y * 16; y1 = (y + h) * 16;` (s16) 62, (s32) 73.
3. oy in place (`oy = (...) / 2; oy *= 16;`): MWCC sinks the *16 to the tail (worse).
4. On the 2-word state: v + th forms (th + v, << 4, operand orders of |, u + tw - 1 spellings,
   1440 combinations): no change; u1/v1 or u64 locals for word 8: substituted away (2); clut
   copied to a u64 local: no change or worse; tw/th/sprites/p statement orders (24): best 2.
5. Declaration orders: every order of 8 (x0 x1 oy ox y1 y0 tw th) on an earlier state, of 7
   on the v += th state (5040): no fix for the tw/th swap there except tw before th.
