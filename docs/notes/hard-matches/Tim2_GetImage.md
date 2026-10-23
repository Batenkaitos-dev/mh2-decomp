# Tim2_GetImage (main, src/main/pad_port.c)

## The match

No FAKEMATCH tag: p is a byte cursor, ph the picture's start, both `u8 *`; mipMaps is read as
the header byte `ph[0x11]`.

The form: `p = call; ph = p;`, then inside the first `if`, before the loop,
`p = ph + sizeof(Tim2PictureHeader);` (the mip header; `levelSize` read from
`(Tim2MipHeader *)p`), and after it `p = ph + Tim2_Align(tim2, hsize);`.

The `(u32)i` index is kept (the same unsigned-index form is in matched code without a tag,
e.g. weapon_forge.c's `m->list[(u32)cur]`).

## Why

With a same-type copy `ph = p`, a later redefinition of p from ph before a loop
(`p = ph + k`) lets the unroller's leftover guard read through p while the loop's own test
reads through ph and the read after the loop shares their CSE value (black-box).

How it was found: in the form with ph `u8 *`, `ph = p;` and every read `ph[0x11]` (18 words),
`p = ph;` inserted at every statement matched right after `ph = p`, after `hsize = ...`, or
inside the if before the loop; 18-94 elsewhere. A redefinition of p from ph before the loop is
what the guard needed. The natural spelling of that is the cursor above. (Method: insert one
candidate statement at every line and score each.)

## Evidence (words)

- Alternatives to the matching form: ph as `Tim2PictureHeader *` with a cast copy 86-90; ph
  from the call (no `p = call`) 90; p and ph both `Tim2PictureHeader *` (`p = ph + 1`) 44; the
  mipMaps reads cast `((Tim2PictureHeader *)ph)->mipMaps` 89-94 (the address ph + 17 is
  hoisted), except the last read alone (MATCH); a signed index 78, `(u8)i` 119, i u32 81,
  `(u32 *)(p + 0x10) + (u32)i` 80.
- Without the redefinition: `n` assigned in the loop condition (`i < (n = ph[0x11]) - 1`,
  `n <= 4`): 91 (no unrolling); three variables (pic = call; ph/p copies of pic or of each
  other): 20; `ph = call; p = ph; ph = p;` 3; chained `ph = p = call` 2, `p = ph = call` 3.

## Findings about the compiler

- **MWCC's IR pass order** (dump labels in mwccps2.exe, strings 0x1663cc-0x16670a; dumps are
  disabled in this build, `#pragma dumpir` prints nothing): BuildflowGraph,
  SerializeAllAssignments, EvaluateConditionals, RemoveUnreachable, RemoveRedundantJumps,
  DoJumpChaining, RemoveLabels; "IRO pass loop": ScalarizeClassDataMembers,
  CopyAndConstantPropagation, ExpressionPropagation, RangePropagateInFNodes, UseDef, DeadStore,
  PropagateSelfAssignments, ConstantFolding, SimplifyConditionals, SimplifyIfThenElse; then
  PullOutInvariantAssignments, RemoveAllEmptyLoops, LoopReversal, LoopUnroller,
  DoCommonLoopTransformations; "Second pass": CopyAndConstantPropagation, ConstantFolding,
  EvaluateConditionals; then CommonSubs, DoTransformations, RemoveLabels, SplitLifetimes,
  RewriteBitFieldTemps, RegenerateExpressions, UnpromoteTypesLinear, Delinearize.
- **Unroller** (IroUnrollLoop.c strings): a for loop must be `init; goto TEST` ("preheader does
  not end with a goto"); for a variable count it keeps the original loop for the leftovers
  ("NeedOrigLoop", "LeftOver", "NeedUnrollBodyTest", "ResetUnrolledFinalValue"). Pragmas that
  exist and were tried here: opt_propagateequivalentscalartypes on/off, opt_unrollpostloop on/off,
  opt_findoptimalunrollfactor on, opt_optimizecontrolflow off (all 1);
  opt_addgotounrolledlooptest on (84).
- **Copy propagation, black-box:** `ph = (Tim2PictureHeader *)p` (the pointer types differ) is
  never propagated, wherever it stands (diagnostic: a store before the loop makes the top guard
  reload through ph). A same-type copy (`u8 *ph = p`) is propagated into every use outside loops
  (the first if, the unroller's guards, the remainder guard, the `<= 4` test) but not into the
  remainder loop's own test, nor into uses after p changes. A copy inside the loop body (or the
  for-increment) stops the unrolling (93); a copy in the loop condition (`ph = p, i < ...`) is
  pulled out of the loop, but the test was already rewritten to p (guard and test via p, 1).
- **IR CSE is not copy-aware:** a load through p in a join block after the same load through
  ph is reloaded (small tests: `p[0x11]`, `((T *)p)->mipMaps`); through ph twice it is shared.
- **Backend:** copy propagation is block-local (the first if's load reads v0, the call result,
  because the copies are in the same block); redundant loads are removed across an extended
  block (a ph-load in the if-block that repeats a p-load in the entry block becomes
  `andi a0,v0,0xff`).
- **Register order:** declaring ph before p swaps the registers (p a2, ph a3): 12.
- **Other types for ph** in the 18-word form: `const u8 *` (not propagated, an extra load: 12);
  `void *` or `u32` with cast reads (the address ph + 17 is hoisted out of the loops: 21).
- **Read forms** (p and ph as `u8 *` or `Tim2PictureHeader *`, each of the four mipMaps reads
  through p or ph): with p `u8 *` the loop-test read alone decides the guard and the test:
  through p both read p (1, the test wrong), through a `T *` ph both read ph (1, the guard
  wrong), through a `u8 *` ph guard p / test ph with the `<= 4` reload (18). p as
  `Tim2PictureHeader *` (with casts for the arithmetic): 44-90.
