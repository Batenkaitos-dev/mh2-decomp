# Gs_ClearFrame (main, src/main/gs_setup.c)

## Status

Matched in round 30 (nm s4): 6 words to 0, unwrapped. Every function in the
file matches; ninja (13 OK).

## What matched

```c
    s32 oy;
    s32 ox;
    ...  /* scissor, RGBAQ */
    oy = gSysWork.offsetY;
    ox = gSysWork.offsetX;
    D_00218A60 = GS_XYZ(ox << 4, oy << 4, 0);
    D_00218A70 = GS_XYZ((ox + gDispWidth) << 4, (oy + gDispHeight) << 4, 0);
```

## Why (round 30)

- MWCC numbers a global load used twice (a common subexpression) at its
  second occurrence, in the code generator's evaluation order (the right
  operand of `|` and `+` first). With the scissor, RGBAQ and the first XYZ
  before the far corner, all four second loads are in the far corner, in
  the order oy, h, ox, w: ours numbered the height before offsetX.
  Statement orders that give the game's order (XYZ statements before the
  scissor) change the schedule (37-55). Read from the registers of the four
  loads in each variant.
- The game numbers oy, ox before the display size: as declared locals. A
  plain extern copy (`x = gSysOffsetX`) is propagated (10); a struct field
  load into a local (`gSysWork.offsetX`, or through `SysWork *sw`) keeps its
  declaration's number (docs/mwcc-matching.md, "Values with 23 or more neighbours",
  rule 2: a field load keeps its declaration's place). Declaration order
  y, x (x, y: 6); assignments y then x after the RGBAQ (other places 33-61);
  then the sums as ox + w, oy + h (the addu operand order follows the
  source): 0.
- The file's display setup already reads gSysWork.offsetX/offsetY.

## Tries this session (scores)

- gSysWork fields in the expressions (no locals), any subset: 6.
- Statement orders XYSR/XYRS 40, XRYS 55, XSYR 39, RXYS 62, SYXR 20, SRYX 23,
  YXSR 37, YSXR 36.
- In-place x/y (`x += gDispWidth`): 57. Sum locals in any place: 6
  (substituted).
- `s32 *o = &gSysOffsetX; o[0]/o[1]`: 6. `SysWork *sw` with x/y locals: 2 to
  61 by place and order. The match holds without sw, and with every
  gSysWork field of the function (bgColor, gsScissor, gsTest, gsZbuf) read
  through gSysWork too; the minimal form is the one committed.

## For the lead

Nothing outside the function changed. The rule found (a reused global load
is numbered at its second occurrence; a field load held in a local keeps
its declaration's number, where a plain extern copy is propagated) may help
other blocks where tempcolor names a reused load numbered in the wrong
order.
