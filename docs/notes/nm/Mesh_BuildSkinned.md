# Mesh_BuildSkinned (sub_main, src/sub_main/unk_2F6540.c)

## Status
Matched in round 30 (s6), from 23 words; unwrapped. The file's functions all MATCH; it has no
NON_MATCHING block left.

## What matched it
Three changes together, all natural C (none matches alone):
- One write cursor `u8 *p` for both buffers: the index list as `*(u16 *)p = ...; p += 2;`,
  then `p = a->verts;` and the vertices through it, `p += 0x2C` (the old `u8 *v` is gone).
- The weight loops counted with `j`, dead after the index loop (they used `k`).
- The ListCount result in a fresh `s32 nw` (it reused `m`), declared anywhere before `best`.

Why: the 16 values live across ModelChunkC_Rec (which with ModelChunk_Get overwrites v0-t2)
are coloured in the reverse of their removal order, the first seven taking t3..t9 and the rest
s registers. Read off the two listings, the removal order was
  ours:  &b[2] &b[1] &w[3] v  best &w[1] &w[2] walkers &v[10] &v[9] i m k
  game:  &b[2] &b[1] &w[3] nw best &w[1] &w[2] walkers &v[10] &v[9] i j p
that is, the declared first webs (v, best) by declaration, then created values, then later
webs in the order of their variables' first webs (i, m, k in ours). So the game's count is a
first web declared before best, and the vertex pointer a later web of a variable first set
after the one counting the weight loop: p (first set at the index list) after j.

## What was seen on the way
- Every single declaration move: 146 of 240 at 23, none moves v out of the s registers.
- nw alone (any place): 48-51 (v stays in s4/s5); p with the weight loops still on k: 51,
  with nw 20 (p t4, k t3);
  p + j without nw: 35. `p = (u16 *)a->verts; ... p += 0x16` with `u16 *p` also matches
  (0); the u8 cursor is the plainer form. The unused `u8 *v` kept or removed: no difference.
- Learning variants (code removed: the k loop, the zero stores, the bone/weight words, best)
  all kept v in s5 except removing the k loop: the k loop's values decide it.
- tempcolor.py and regalloc_predict.py did not see this one: v is an s register in ours (out
  of tempcolor's scope), and the replay found no order for the original.
- Tools (scratch mb): gen.py compiles many variants of the function in one
  file and prints per variant the words off, the registers of v, m, k, i and the register
  pairs that differ; dm.py (every declaration move), vp.py/vp2.py (the cursor, counter and
  count combinations).

## For the lead
- Nothing outside Mesh_BuildSkinned was changed.
- The twin Mesh_BuildRigid (matched) has `u16 *p` and a separate `u8 *v`; here the game's
  allocation needs the vertex pointer to be the index cursor's later web. Written as one u8
  byte cursor (no tag): a dead variable reused for a later value of its type.
