# Browser_ReadPad (lobby, src/lobby/browser_util.c)

## Status

- Round 30 (s1): still 11 words in the build (the block is committed in a more natural form: the
  double conversions written implicitly, the join read as `*(u8 *)((char *)w + o + k)`).
- **Cause of the 11 words found; the block matches (func_diff MATCH, every other function of the
  file MATCH) once a function earlier in the same file compiles a two-argument runtime call.**
  That function is not in the game's binary (nothing in lobby calls such a helper), so it needs
  a build rule for a linker-stripped function: the lead's decision ("For the lead").

## Handoff

- If the lead accepts the stripped-function rule: add the function and the mwcc_cc.py rule
  below, unwrap the block, run the checks (CONTRIBUTING.md, "Checking a change"). Nothing else is needed (scratch base_q.c is the
  matching file: rp/base_q.c, with `Test_Prior`).
- If not: the block stays at 11; nothing in C inside the function can do it (tests below).
  `o` stays contrived (the game's 0x1A650 per branch); no natural form found (n1: 112).

## For the lead

1. **Rule needed: a function the game's linker dead-stripped.** Exact edit proposed (not applied):
   in tools/mwcc_cc.py, in the loop that names the C object's sections, before
   `if name == ".text" and i in func_in:` handle `func_in[i].startswith("Stripped_")` as a
   marker (`renames[i] = f".text.{func_in[i]}"; order.append(("marker", i, None))`), so the
   function is compiled (MWCC's state changes) and its section discarded, as INCLUDE_ASM's
   markers are. Then in browser_util.c, anywhere before Browser_ReadPad:
   `static f64 Stripped_Sub(f64 a, f64 b) { return a - b; }` with a comment (it compiles to a
   tail call of dpsub; no data, no relocations outside its section). Score: Browser_ReadPad
   MATCH, the file's other 8 functions MATCH (func_diff with the function present).
   Evidence the game's linker dead-stripped: strings whose first user is gone (round 4's plsel notes).
2. The file's `double litodp(s32 x); float dptofp(double x);` (lines 13-14) are now unused
   (only this block called them); they can go.

## Findings (round 30)

- **The hidden a1 holder is MWCC's own litodp call.** Black-box (scratch t4.c-t17.c, gen_t*.py):
  a function `p = (s32 *)(s32)gA; gW[0] = p[0]; gW[1] = p[1];` then a loop with a conditional
  `a = 101.25 - gSA[i]` gives p a1 when it is the file's first helper user, a2 when a function
  before it in the file compiled any two-argument runtime call: dpsub, dpadd, dpmul, dpdiv,
  _dpflt, _dpfeq, __divdi3, __muldi3, __umoddi3 (each tried). Then even a lone `(f64)s16`
  (litodp) blocks a1 from the function entry. One-argument helpers before (litodp, dptofp,
  fptodp, dptoli, __fixdfdi, _d_ulltod), declarations (`double dpsub(double, double);`),
  explicit calls of a declared dpsub, unprototyped/variadic/K&R calls, user functions with
  f64/s64 parameters, a struct copy, unused `static inline` helpers, and dead or folded double
  ops (removed before code generation) do not. A function after does not. Inside the same
  function nothing changes it (dpsub emitted before litodp: still a1): the state is per file.
- An unused `static` (not inline) function with `a - b` on doubles is compiled and emitted by
  MWCC (8 bytes) and triggers it.
- The explicit `dptofp(101.25 - litodp(x))` calls (user declarations) never get the effect:
  the conversions must be the compiler's (`a = 101.25 - angle;`, same code otherwise).
- In that state the only word left was the join's operand order (`addu v1,s0,a2`, w + o):
  `*(u8 *)((char *)w + o + k)`, `*(u8 *)((u8 *)w + o + k)`, `*(u8 *)((s8 *)w + o + k)` and
  `*(u8 *)(&((u8 *)w)[o] + k)` give it; `((u8 *)w + o)[k]` and 13 other spellings give o + w
  or swap the k add (scratch s14.py, s15.py).
- pad: `pad = &gPadState[i]` (i = 0) or `(PadState *)((u8 *)gPadState + off)` match as well as
  the `(s32)` cast; `gPadState` / `&gPadState[0]` fold the address into the loads (144).
- tempcolor.py does not flag the original's skip: it models the litodp call as reading every
  argument register with a reaching definition (CALL_RD), which for the original happens to put
  an a1 web from the entry; for ours the same model gives a2. The rule above is the real cause.
- Permuter (perm_nu, 21 min from the old block, first state): best permuter score 85, stopped
  once the cause was known.
