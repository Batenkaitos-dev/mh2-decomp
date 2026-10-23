# Mesh_BuildSkinned (sub_main, src/sub_main/model_file.c)

## The match

Three changes together, all natural C (none matches alone):
- One write cursor `u8 *p` for both buffers: the index list as `*(u16 *)p = ...; p += 2;`,
  then `p = a->verts;` and the vertices through it, `p += 0x2C` (no separate `u8 *v`).
- The weight loops counted with `j`, dead after the index loop (not `k`).
- The ListCount result in a fresh `s32 nw` (not a reuse of `m`), declared anywhere before
  `best`.

The twin Mesh_BuildRigid has `u16 *p` and a separate `u8 *v`; here the game's allocation
needs the vertex pointer to be the index cursor's later web. Written as one u8 byte cursor
(no tag): a dead variable reused for a later value of its type.

## Why

The 16 values live across ModelChunkC_Rec (which with ModelChunk_Get overwrites v0-t2) are
coloured in the reverse of their removal order, the first seven taking t3..t9 and the rest s
registers. Read off the two listings (ours before the change, and the game's), the removal
order was

    ours:  &b[2] &b[1] &w[3] v  best &w[1] &w[2] walkers &v[10] &v[9] i m k
    game:  &b[2] &b[1] &w[3] nw best &w[1] &w[2] walkers &v[10] &v[9] i j p

that is, the declared first webs (v, best) by declaration, then created values, then later
webs in the order of their variables' first webs (i, m, k in ours). So the game's count is a
first web declared before best, and the vertex pointer a later web of a variable first set
after the one counting the weight loop: p (first set at the index list) after j.

## Evidence

- Every single declaration move: 146 of 240 at 23 words, none moves v out of the s registers.
- nw alone (any place): 48-51 (v stays in s4/s5); p with the weight loops still on k: 51,
  with nw 20 (p t4, k t3); p + j without nw: 35. `p = (u16 *)a->verts; ... p += 0x16` with
  `u16 *p` also matches (0); the u8 cursor is the plainer form. An unused `u8 *v` kept or
  removed: no difference.
- Learning variants (code removed: the k loop, the zero stores, the bone/weight words, best)
  all kept v in s5 except removing the k loop: the k loop's values decide it.
- tempcolor.py and regalloc_predict.py did not see this one: v is an s register in ours (out
  of tempcolor's scope), and the replay found no order for the original.

## Method

Compile many variants of the function in one file and print per variant the words off, the
registers of the values in question and the register pairs that differ; sweep every single
declaration move, then the combinations of the candidate changes (cursor, counter, count).
