# GmEm_SetMeshAlphas (gm_sub, src/gm_sub/gm_em.c)

## The match

Background: docs/mwcc-matching.md, "How MWCC colours temporaries" and "Values with 23 or more
neighbours".

Without the two forms below, the function misses by 35 words, all in kind 0x24's 32-mesh loop
(mode 0): the game gives its counter and its two walkers (the table and mesh pointers MWCC
makes for `gGmEmKind24MeshTex[j]` and `meshes[j]`) v1, a0 and a1, coloured before the short
temporaries; the plain form colours them after the hoisted constants 1 and 14 (t1-t3). The
other loop values (slti, address, gTexHandles, lbu, 14, 1, n) are coloured in the game's order
either way. So the walkers wait in the game (23 or more neighbours left) and not in the plain
form, while the constants 1 and 14 must not wait. Two natural changes add the two neighbours
that the walkers have and the constants don't:
- **The loop's n a later web of n.** Case 1's (and the second, same) texture index
  `n = em->parts[7].state >= 10 ? 2 : 1;` as `if (em->parts[7].state >= 10) n = 2; else
  n = 1;` (the form case 0x24's case 3 has): two definitions reaching the use make it n's real
  first web, so the 0x24 loop's n becomes a later web, numbered after the walkers and before
  the constant 1 (left for the walkers when they are visited, removed before the constants).
  Alone: 37 (n coloured before the walkers; no wait yet).
- **`g = 0.7f * r;` inside the `j == 1 || j == 14` branch** (or `0.7f * r` written at the
  three stores): the hoisted product's 0x3F333333 constant is then created after the walkers'
  inits and overlaps them in the first schedule (the game's a2 skipping v1 and a1). With the
  first change: MATCH.

## Data

The zeros after jtbl_00933230 (0x933350-0x933380, padding to .bss) are a `rodata_end` data
piece in config/overlays/gm_sub.yaml (`{ start: 0x38550, type: data, name: rodata_end,
linker_section_order: .rodata }`), since MWCC emits the table without them: the table's 72 entries are the
same, and the switch's sltiu 0x48 never reads the rest.

## Evidence (variants compiled into one file; the loop's registers printed; schedule off as a check)
- tempcolor.py can't read this function: the jump-table cases make every t-register
  live-in, so the loop's walkers merge into function-wide webs (the "500-1174 neighbours").
- Small tests (t2-t7): walkers are numbered after the declared locals, before the loop
  body's temporaries, in every loop form (for, while, do, goto, pointer or byte arithmetic,
  u32/s16 counter, explicit walkers). A later-web counter is numbered after the walkers; a
  later-web n between the walkers and the first body constant. Extra integer values live over
  the loop make the counter wait first, then all walkers and the constants; floats don't count.
- In the function: values kept live over the loop by diagnostics (`i + m`, `+ (s32)b` in the
  main loop's call) made the counter wait at +2, all five at +3; with n as a later web, +2 gave
  the game's loop registers exactly (except the 0.7f constant, which showed the overlap).
- No effect alone (35): `r * 0.7f`, `j == 14 || j == 1`, explicit p for meshes[j], a do-while,
  `j = 0` before the for, g before the r if/else, g in the for's init (37 with n later web).
