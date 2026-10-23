# Browser_ReadPad (lobby, src/lobby/browser_util.c)

## The match

The function matches only when a function earlier in the same file compiles a two-argument
runtime call. That function is not in the game's binary (nothing in lobby calls such a
helper): the game's linker dead-stripped it. The build stands it in with a function named
`Stripped_...`, which tools/mwcc_cc.py compiles (so MWCC's state changes) and then drops:

    static f64 Stripped_Sub(f64 a, f64 b) { return a - b; }

placed anywhere before Browser_ReadPad (it compiles to a tail call of dpsub; no data, no
relocations outside its section). With it, Browser_ReadPad and the file's other functions
MATCH; without it the block misses by 11 words, and nothing in C inside the function can do it
(evidence below). Evidence that the game's linker dead-stripped functions:
strings whose first user is gone (docs/mwcc-matching.md, "A string before the jump table of a
function that uses it later").

Inside the function:

- The double conversions are the compiler's, written implicitly (`a = 101.25 - angle;`).
- The join is read as `*(u8 *)((char *)w + o + k)` (in src: `work`, `tiltOfs`, `outOfs`).
- `o` stays contrived (the game's 0x1A650 loaded in each branch); no natural form was found (a
  natural one scored 112).

## Why: the hidden a1 holder is MWCC's own litodp call

Black-box tests: a function `p = (s32 *)(s32)gA; gW[0] = p[0]; gW[1] = p[1];` then a loop with
a conditional `a = 101.25 - gSA[i]` gives p a1 when it is the file's first helper user, a2 when
a function before it in the file compiled any two-argument runtime call: dpsub, dpadd, dpmul,
dpdiv, _dpflt, _dpfeq, __divdi3, __muldi3, __umoddi3 (each tried). Then even a lone `(f64)s16`
(litodp) blocks a1 from the function entry.

These do not have the effect: one-argument helpers before (litodp, dptofp, fptodp, dptoli,
__fixdfdi, _d_ulltod), declarations (`double dpsub(double, double);`), explicit calls of a
declared dpsub, unprototyped/variadic/K&R calls, user functions with f64/s64 parameters, a
struct copy, unused `static inline` helpers, and dead or folded double ops (removed before code
generation). A function after does not. Inside the same function nothing changes it (dpsub
emitted before litodp: still a1): the state is per file.

- An unused `static` (not inline) function with `a - b` on doubles is compiled and emitted by
  MWCC (8 bytes) and triggers it.
- The explicit `dptofp(101.25 - litodp(x))` calls (user declarations) never get the effect:
  the conversions must be the compiler's (`a = 101.25 - angle;`, same code otherwise).

## Evidence

- In that state the only word left was the join's operand order (`addu v1,s0,a2`, w + o):
  `*(u8 *)((char *)w + o + k)`, `*(u8 *)((u8 *)w + o + k)`, `*(u8 *)((s8 *)w + o + k)` and
  `*(u8 *)(&((u8 *)w)[o] + k)` give it; `((u8 *)w + o)[k]` and 13 other spellings give o + w
  or swap the k add.
- pad: `pad = &gPadState[i]` (i = 0) or `(PadState *)((u8 *)gPadState + off)` match as well as
  the `(s32)` cast; `gPadState` / `&gPadState[0]` fold the address into the loads (144).
- tempcolor.py does not flag the original's skip: it models the litodp call as reading every
  argument register with a reaching definition (CALL_RD), which for the original happens to put
  an a1 web from the entry; for the same function without the stripped stand-in the model
  gives a2. The rule above is the real cause.
- Permuter (21 minutes from the 11-word block): best score 85.
