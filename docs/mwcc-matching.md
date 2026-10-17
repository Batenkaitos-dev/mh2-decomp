# The MWCC matching reference

How Metrowerks CodeWarrior for PS2 (build 3.0b52-030722, the compiler of
the game's code) turns C into the game's instructions: what to know about
the compiler, the pattern catalogue (which C gives which code, with the
models of its register allocation and scheduling), and the permuter.
The contributor guide (the repository's rules, setting up, checking a
change, working on a function, the fakematch rulings) is
[CONTRIBUTING.md](../CONTRIBUTING.md). Some entries name the project's own
research tools (near_misses.py, func_variants.py, the autodecomp tools):
they are not part of this repository; func_diff.py, adiff.py and the
permuter do the same work by hand.

## The compiler

The game code was built with **Metrowerks CodeWarrior for PS2**. We use
compiler build `3.0b52-030722` with `-O3,p -sdatathreshold 0 -str readonly`
(`tools/configure.py`; `-str readonly` puts string literals in `.rodata`).
Things to know about it:

- Each function, and each variable, string literal and jump table, gets its
  own section, in source order (uninitialized variables come last,
  last-declared first).
- It pads loop heads and short loops with `nop`s, never uses branch-likely
  instructions, and emits tail calls as `j`.
- It saves callee-saved registers with 128-bit `sq`/`lq`.
- Globals are addressed absolutely (`lui` + offset), never `gp`-relative.
- Sony SDK and CRI middleware code in the binaries was built with GCC and
  is not ours to match; it stays assembly.
- **Some files were compiled at optimization level 4**, not 3. Such a file
  says so with `#pragma optimization_level 4` after its includes (23 files
  so far: `grep -rl "optimization_level 4" src`); func_diff.py,
  near_misses.py, func_variants.py and the permuter respect it. Try it
  when a file's near misses resist. A file qualifies only if every function
  that already matches still matches with it. A file with one function of
  each kind is probably two of the game's units.
- **Empty branch delay slots:** MWCC normally fills them in optimized C,
  but some C forms leave them empty: a one-case `switch` (its compare
  branches are never scheduled), a `switch` with `default:` first (the
  whole function), a two-step assignment, a redundant test. Try those
  before concluding the original was function-level `asm` (which also
  leaves them empty, and is then kept as assembly).

## MWCC pattern catalogue

Which C gives which instructions with MWCC 3.0b52, sorted by what the diff
shows. It gathers the "MWCC patterns" of the project's history and of the workers'
session notes (rounds 1 to 25). Where the notes disagreed, the entry
gives the reading that holds now. "(tested)" marks rules checked by
compiling small test functions with the project's compiler. Read the
index below first, then open the one entry you need.

Each entry gives the symptom in bold, then the cause, the fix (the C form)
and where it was seen (function and module). Try
one fix at a time with `func_variants.py`, and keep a variant when the
func_diff score goes down, even if registers still differ.

`near_misses.py` tags each near miss by the kind of difference. The
tags lead here: registers to the two register sections; order to "An
instruction or constant load in another place" and "Arguments loaded in
another order"; delay-slot to "Delay slots filled or empty"; masks to "An
extra or missing mask or sign extension"; size to the sections on
switches, branch layout, loops and prototypes.

**Checking a match.** A score of 0 isn't a match until the build agrees:
- adiff ignores branch targets and relocations: `full=0` can hide a
  missing `default:` (Bowgun_State01) or another symbol (func_007B0050),
  or `if (a || b) return;` where the game has two ifs (Quest_OnRemoteKill,
  round 9: mn=0 and full=0, branch targets different). Confirm with
  `func_diff.py -C 0 MODULE FILE.c FUNC`.
- func_diff compares which symbols the relocations name, not where the
  data lands: a `.rodata` object that moves every later string by 8 bytes
  shows only in the build's SHA-1.
- A diff helper once showed an `lb`/`lbu` difference as one differing
  `jal` word (DojoMenu_DrawQuestPage, in a worker's t.py and in
  an agent's diff helper); the target check found it. Build and check every
  target (`ninja`; CONTRIBUTING.md, "Checking a change") before calling it a match.
- The permuter's start score can be far from adiff's (so a run finds
  nothing), and it scores stack-slot differences as 0 (func_0070F580:
  base 0). Rank drafts by adiff, not by the permuter's score.
- A near miss whose only difference is which register holds a constant
  may be a wrong constant: func_007E4470's case set {2, 3, 4} was really
  {1, 3, 4} (the chain's `li v1, 1` fed the third compare). Check which
  constant each compare uses before renaming registers.

### Index

One line per entry: the symptom, the usual fix, and the section by a short
name (Saved = "Saved registers in another order", Temporaries, Placement =
"An instruction or constant load in another place", Argument order, Masks,
daddiu, Compares, Switches, Branches, Delay slots, Static callees,
Prototypes, Addresses, Loops, Struct copies, Float, Stack, Data, Level 4,
Not from C). Open the entry itself for the forms and examples.

- A score of 0 that may not be a match: confirm with func_diff and the build (Checking a match, above)
- Saved registers no declaration order explains: replay the allocator with regalloc_predict.py (Saved, "How MWCC picks saved registers")
- Saved registers swapped or rotated between locals: reorder the declarations (Saved)
- A created address in s0 that pushes a declared one up: declare it as a local at the top (Saved)
- A copy the game keeps and yours folds: another type, and a cast in the assignment (Saved)
- A loop's registers rotated while its twin matches: a fresh local, not a reused one (Saved)
- Saved registers off after every single declaration move: swap pairs of declarations (Saved)
- regalloc_predict wants a value numbered last: a created value, the expression at each use (Saved)
- A loop's index and counter swapped: swap which variable plays which role (Saved)
- A second value in a lower saved register in the original: merge two locals, sometimes two pairs at once (Saved)
- One pointer in two saved registers: an int address parameter and a typed copy (Saved)
- A parameter in another saved register: a typed or same-type local copy of it, or dropping one (Saved)
- Saved registers rotated: merge two locals with separate lifetimes, or split one (Saved)
- A value MWCC makes takes the lowest s-register: declare it, or write it at each use (Saved)
- A later loop's or case's variables in other s-registers: own or reused variables, a pointer per loop (Saved)
- One s-register for two values: reuse one variable for both (Saved)
- Parameters in each other's s-registers: switch on a cast, change how the narrow one is used (Saved)
- Field read through a0 before `move s0, a0`: first initialised local, or a volatile read (Saved)
- Narrow parameter's extended copy kept, raw one passed on: `s16 ty = y;` or `xs = x + 0;` (Saved)
- Address, product or loop invariant kept across calls: a declared local for it (Saved)
- Local loaded at a loop's top, or live from the entry: move its declaration or assignment (Saved)
- No declaration order fixes the s-registers: statement order, variable reuse, level 4 (Saved)
- v0/v1 or a0-a3 swapped around a switch: reuse the chain's constants; `case 0: default:` (Temporaries)
- A value masked again per case in other registers: a macro that copies its argument once (Temporaries)
- A long-lived value in a high temporary: an earlier use of the variable, so yours is a later web (Temporaries)
- Counts or loop bounds coloured after a hoisted invariant: make them later webs (Temporaries)
- t registers in reverse: the earlier-declared value gets the higher register (Temporaries)
- Every temporary rotated, every mnemonic right: split the expression into statements (Temporaries)
- Two temporaries swapped against every declaration order: a fresh or a reused dead variable (Temporaries)
- A load folded into its use in yours: a local of another type holds it (Temporaries)
- Every temporary one register up: the function returns a value (Temporaries)
- Every temporary one register off in a void function: same-file callees static, or not (Temporaries)
- Result in a parameter's register: assign it back to the parameter (Temporaries)
- A value kept in an unused parameter's register: reuse the dead parameter as a local (Temporaries)
- Temporaries or float registers in another order: permute declarations, per-block scopes (Temporaries)
- A value stays in its first register, or yours coalesces it: add or remove a typed copy (Temporaries)
- Switch value in a2 where yours has a1: take the passed-through parameter and pass it on (Temporaries)
- Two registers swapped, the code otherwise right: vary one cast or operand at a time (Temporaries)
- `movz` for `movn` with the constants swapped: flip the ternary, or an if/else (keeps s-registers) (Temporaries)
- A repeated constant or extended zero in another register: reorder the chained assignment (Temporaries)
- Caller passes a copy in a0, not the variable: the callee's parameter is s16 (Temporaries)
- A constant in an argument register for no reason: it is a call's argument (Temporaries)
- A global's load above or below a store: named objects move, struct fields keep order (Placement)
- A switched-on field loaded in other places: a local read once, plus the game's one re-read (Placement)
- A raw-address copy of a matched function scheduled differently: the named fields (Placement)
- Stores in another order, the code otherwise right: single moves within each run of stores (Placement)
- A packed GS word's parts in another order: write them in the game's order, y first for GS_XYZ (Placement)
- A constant argument one slot early next to a global's load: the global as an array read through a cast (Placement)
- An address loaded before another load: a chained store (Placement)
- A search loop's first test with v0 taken: the not-found result read before the loops (Placement)
- An address in a loop's preheader among the compare constants: assign it inside every case's conditional block (Placement)
- A global reloaded after stores, or loaded once: struct member or volatile vs extern scalar (Placement)
- A value reloaded after a call, or kept: the call as its own statement, or nested (Placement)
- A loop test's load reused in the body, or reloaded: temporaries, or read it twice (Placement)
- `sw; lw` of the same word: two statements (`x--; if (x == 0)`); test the assignment for the reverse (Placement)
- Independent loads or stores in another order: the source order; try every order (Placement)
- A load no statement order moves before a store or call: read it into a local first (Placement)
- Per-kind table loads up front: read the field in every initialiser (Placement)
- A constant in the `jal`'s slot, not the next `b`'s: move the assignment across the call (Placement)
- A store or pointer step in the `jal`'s slot: a static leaf callee, or locals then the call (Placement)
- A loop invariant in other registers, or after the loop: write it where the game computes it (Placement)
- A store or argument moved after a rewrite: try both spellings of the expression (Placement)
- Schedule off as if a check were still there: an empty compound `if` (a compiled-out check) (Placement)
- A load before a branch in the game, after it in yours: a local of another type than the field (Placement)
- A late load and an alignment nop missing: subtractions as statements of their own (Placement)
- Bytes ORed together, loaded in another order: mask every byte, the plain one too (Placement)
- A loop's hoisted invariants in other temporaries: the expression in the loop, as its twin has it (Placement)
- A value recomputed after a call, or reused: stop the CSE, or hold it in a local (Placement)
- An address, offset or mask where no plain C puts it: a `static inline` helper (Placement)
- a1 loaded before a0, or the third before the second: real parameter types and order (Argument order)
- Arguments in another order: the callee's prototype, K&R or another parameter type (Argument order)
- An int argument's `li` in the jal slot in the original: the int parameter is last (Argument order)
- A float argument set up last in yours, first in the original: its real place in the prototype (Argument order)
- An expression argument evaluated before plain loads: `+ 0`, or a local for the result (Argument order)
- An address argument first, or last in the slot: its base or parameter int vs pointer (Argument order)
- A call's result extended or masked, or not: bare test vs compare; the return type (`andi 0xFF`: u8) (Masks)
- A u8 parameter masked once and that copy used: an s32 local copy (Masks)
- Masks and extensions that say a local's type: an int holding (u16)x, an inline with s32 parameters (Masks)
- A narrow local extended again before an add in a call argument: step it inside the argument, `f(x, y += K)` (Masks)
- `andi`/`dsll32` before a call's argument, or none: the variable's and the caller's prototype's widths (Masks)
- A narrow parameter or local extended at each use: an int copy, or an in-place s16 (Masks)
- s16 coordinate steps folded, or added at each call: steps inside or outside the arguments (Masks)
- The extension on the other operand of `op=`: `x op= y` vs `x = x op y` (Masks)
- `andi rX, zero, 0xFF`, an extended zero: a chained zero through narrow types (Masks)
- `andi` on a u8/u16 tested as a condition, or none: bare test vs `!= 0`/`== 0` (Masks)
- An s8 re-extended before a shift, index or compare: an int, cast at the use (Masks)
- The extension or mask after a select or join: if/else vs ternary, where it is set (Masks)
- `sra` for `srl`, `slti` for `sltiu`: signedness, `U` suffixes, casts (Masks)
- 64-bit instructions (`dsllv`, `lwu`, `pcpyld`): `long`, `1LL << n`, s64 arithmetic (Masks)
- Bitfield stores: storing 0 isn't folded, storing 1 is (Masks)
- A load or store of another width or signedness: the field's type, or `*(u16 *)&field` (Masks)
- `daddiu reg, zero, N` where yours has `li`: the variable is u8 or u16 (daddiu)
- `slti at` vs `slti v0`: `>=` and a loop-bottom `<` use a register; rewrite `>=` as `>` (Compares)
- `sltu at; bnez at`: the assembler's bltu macro, an unsigned `<`, `<=` or `>` in an if (Compares)
- A `movn` where yours branches: `x == 0 ? A : B` (Compares)
- `bgez`/`bltz` vs `slt at, x, zero`: `x >= 0` against `0 <= x` (Compares)
- A computed `!x` (`sltu`/`xori`) before a branch: `!(x) != 0` and similar forms (Compares)
- One range test vs separate compares: an s32 local merges; `U` or a switch keeps them (Compares)
- A comparison's operands loaded in another order: swap the operands and the operator (Compares)
- A compare the game keeps and yours folds: an s32 local for the result, a tested assignment (Compares)
- The compare chain in another order: write the cases in reverse of the chain's order (Switches)
- Separate compares vs one range check: a switch, its cases in reverse order (Switches)
- An extra compare and branch to the switch's end: add the empty `case N: break;` (Switches)
- Chain vs jump table, or a table of another size: list the empty cases it covers (Switches)
- Default body right after the compares, or reached by `b`: move `default:` (Switches)
- `beq x, K` and a `b` where yours has one `bne`: a one-case switch (Switches)
- `beq` to one block and `b` to another: a two-way switch, `case 0: default:` (Switches)
- `beq; beq; b else` against `beq; bne`: a switch vs an `||` if (Switches)
- A chain of `!=` tests returning 0 or 1: a switch whose cases share one body (Switches)
- A dead `li`, or a `beq` to the next instruction, before the default: label order and place (Switches)
- One exit block per case, or one shared exit: `return` vs `break` in the cases (Switches)
- One block reached from two cases or paths: a `goto`; MWCC doesn't merge tails (Switches)
- The switch value compared in another register: a u8 local, a pointer to the field (Switches)
- Boolean function with branches, yours straight-line: explicit `return 1; return 0;` (Branches)
- Far off with a different size: compare branch targets and size first, fix the control flow (Branches)
- The else block first in the original: invert the test (Branches)
- A branch to a `b` before the epilogue in yours: an early return (Branches)
- An early return at the end or before the tail: arrange if/else and returns like the blocks (Branches)
- An if-body after the last block, jumped back to: a `goto` past an explicit `return;` (Branches)
- A loop's inner exits straight to the end: `goto end;` inside, `return;` after the loop (Branches)
- Early exits share one return block, or have one each: nested ifs, one return, `goto` (Branches)
- An alignment nop at a join: `if` clamp vs ternary, switch labels (Branches)
- A `b` to the next instruction at an if-chain's end, in the original: a store of the value already set, in an else (Branches)
- The two blocks of an if/else swapped: `!x` vs `x == 0`, the condition's polarity (Branches)
- A `b` to the end after an if-body: an explicit `return;` there (Branches)
- The value or polarity of a select's delay slot: the ternary's form and operand order (Branches)
- A call result's copy in the branch's slot: `if ((x = f()) != 0)` (Branches)
- A store in each branch, or one after the join: store in both branches (Branches)
- A `j` tail call vs `jal` and the epilogue: `return f();` vs `f();` (Branches)
- A branch to the next instruction, or `b` over an empty else: write the redundant code (Branches)
- A `b` to another `b`: no jump threading; nest the source like the blocks (Branches)
- Empty delay slots where yours are filled: a default-first switch (anywhere), or the other forms (Delay slots)
- Empty delay slots that your default-first switch doesn't give: move it to another test (Delay slots)
- A compare chain's slots empty, the default a bare `b`: a default with code that stays (Delay slots)
- A filled slot where yours has a nop: an `if` instead of the switch, `default:` last (Delay slots)
- The same store or constant in two slots: it is written twice in the source (Delay slots)
- `lui at` in a `b`'s slot vs a `nop`: raw address vs named object (Delay slots)
- A register used after a call without being set: make the same-file callee static (Static callees)
- A caller that matches only with a callee of another file static: the files are one unit (Static callees)
- `move a0, s0` before a call, or a0 passed on: pass the parameter or its copy (Static callees)
- Which callees were static: single-file callees; join files whose callers keep registers (Static callees)
- A call missing from your compile: a `static inline` callee, or the draft lost the call (Static callees)
- Arguments not narrowed, a float promoted to double: a prototype before the first call (Prototypes)
- Declarations that disagree on float/int parameter order: the callers' load order is the real one (Prototypes)
- A split that would separate two uses of one string literal: none, it's one unit (Prototypes)
- A match that needs a callee's narrower parameter: that prototype in every file (Prototypes)
- A local array's address kept across calls in yours: the callee's parameter a pointer, not an int (Prototypes)
- A callee's integer parameter narrower in a header than in its definition: the definition's type (Prototypes)
- A coordinate converted once, not at each call: the callee's s16 parameters (Prototypes)
- A pointer argument masked or loaded out of order: the real pointer types (Prototypes)
- A float argument missing (f12/f13 set before `jal`): declare the float parameters (Prototypes)
- Callers pass different argument counts, or none: `void f();` and a K&R definition, or no declaration (Prototypes)
- A parameter the code never uses: keep it, the callers set it (Prototypes)
- An argument in t0: a fifth parameter (Prototypes)
- `sd a0` in the callee, `ld a0` before the call: an 8-byte struct by value (Prototypes)
- A large struct by value passed on with no copy: the callee's prototype takes a pointer (Prototypes)
- Variadic functions: variadic prototypes and MWCC's `va_start` (Prototypes)
- Matches alone but not in its file: fit that file's own declaration (Prototypes)
- `addu` operands reversed: pointer vs subscript vs an int byte offset in a local (Addresses)
- The game adds the index first (`addu v, idx, base`): a static inline accessor for the base (Addresses)
- A raw address a name schedules differently: `*(T *)&Object.field` schedules like it (Addresses)
- A raw address read that won't move: use the named global (Addresses)
- A named address that changes the code: `lui; ori` constants and shared `lui`s stay raw (Addresses)
- Other operand orders in integer arithmetic: split the expression, a local per operand (Addresses)
- An `or` of a constant colour in another operand order: an Rgba-style inline (Addresses)
- Sums and compares on a parameter's fields in another operand order: a `u8 *` parameter read through casts (Addresses)
- A run-time `negu` of a constant: `d *= -1;` (Addresses)
- `sll` vs `addu x, x, x` or a multiply: the scaling's spelling (Addresses)
- A global's address in a register vs `%lo` per access: a pointer local through a cast, `(u8 *)(s32)D` (Addresses)
- A field offset in the load vs in the relocation: element pointer vs indexed struct array (Addresses)
- A union member's fields addressed from the symbol where the game keeps the base in a register: a cast of the union's address (Addresses)
- A field's offset folded into the address (a label at base + offset) where yours puts it in the load: index from the field as a pointer (Addresses)
- A field's address computed once vs from the base: typed fields vs casts, view structs (Addresses)
- An address kept for a load and a later store: a pointer local to the field (Addresses)
- An array base in a register (`addiu reg, base, 0x5EC`): the access written out, no local (Addresses)
- A constant of 0x8000 or more through `at` vs a register: `u8 *` vs int arithmetic (Addresses)
- A loop unrolled 8 times, or not: the bound, the init expression or the loop kind (Loops)
- A preheader out of line with a hoisted load: an empty-then `if` around a do-while (Loops)
- `bnez; b exit; b loop` in the game's loop: an unsigned compare in the search loop (Loops)
- An index computed each time in the original, a pointer in yours: an unsigned index (Loops)
- Walkers set up before the loop's guard in yours: index the arrays, MWCC makes the walkers (Loops)
- Walkers bumped in another order: MWCC's own first, then the for header's (Loops)
- A loop bound recomputed: a local set in the for initialiser (Loops)
- A loop's walker set before the pre-test: index the array instead (Loops)
- A loop the original unrolls (by 7 for 21) and yours doesn't: shrink the body under MWCC's limit, casts count (Loops)
- A loop rotated, or not: `while`/`for` with init vs `for (;;)` with a break (Loops)
- A later loop's counter or pointer in other registers: reuse the counter, give the loop its own pointer (Loops)
- `bnez` vs `bgtz`/`blez` as the loop branch: the count-down form, the counter's sign (Loops)
- A pointer step as its own `addiu`, or folded: post-increments and step forms (Loops)
- A strength-reduced pointer vs an index: switch between index and pointer; read the base through memory (Loops)
- A table's first entry loaded before the loop: a typed table, the first entry in a local (Loops)
- The first test before the pointer set-up, duplicated, or on a call's result: an `if` and a `do-while` (Loops)
- Seven or eight `nop`s in a loop, or a wait loop's branches off: count the asm nops (Loops)
- An inline byte-clearing loop with a NULL test: `T x = {0};` (Struct copies)
- A struct copy's instructions differ: the types' alignment and size (Struct copies)
- An unrolled copy vs a loop, or all loads before the stores: struct copy vs fields (Struct copies)
- A byte copy copying the source pointer before each `*src++`: `if (c = *s++) {}` (Struct copies)
- A record copied to the stack, read through a register: `*(rp = &r) = table[id];` (Struct copies)
- `sq`/`lq` of a 128-bit value: a `mode(TI)` u128 typedef (Struct copies)
- Float constants (`lui`/`mtc1`, `sw zero`): no data split needed, a local per constant (Float)
- `mul.s`/`add.s` operands reversed: in-place forms, a float local for the constant (Float)
- Float registers rotated: fields at each use, in-place steps, one reused temporary (Float)
- A float added to an s16 field in another order: `+=` vs the plain form (Float)
- The unsigned int-to-float sequence, or not: `(s32)` for a signed conversion (Float)
- A float-to-unsigned sequence (0x4F000000): the float assigned to a u32 (Float)
- `trunc.w.s` vs `cvt.w.s`: the same instruction (Float)
- A float constant converted at run time in a loop: level 4 and a float local (Float)
- One conversion or negation vs several: repeat `x / (f32)n`; where the local is (Float)
- `mula.s` and `madd.s`: plain `a * t + b * s` (Float)
- `c.eq.s`/`c.lt.s` operands swapped: `if (x)` vs `!= 0.0f`, `K < x` vs `x > K` (Float)
- A min or max search branching like the game: the listed if/else forms (Float)
- `sqrt.s` with no call: declare `f32 sqrtf(f32);` (Float)
- Locals at other stack offsets: size groups, declaration order within a group (Stack)
- An extra spilled copy of a narrow parameter: the expression at each use, not a partial local (Stack)
- A 16-byte spill slot where the game packs 4 bytes: no pointer-to-integer cast at the call (Stack)
- Two spill slots swapped: a declared s32 copy of the narrow value (Stack)
- A 9th or later parameter's stack copy missing: type it u32 (Stack)
- A spill 12 bytes lower, in a 16-byte slot: the value comes from a static inline helper (Stack)
- A stack slot 12 bytes off: a spilled variable (4 bytes) vs a compiler temporary (16) (Stack)
- A frame 0x30 bigger, or a lone f32 elsewhere: `f32 m[16]`, `f32 v[3]`, real sizes (Stack)
- A parameter stored to its home slot: its address is taken (Stack)
- A small array on the stack, or in registers: a loop fill vs assignments (Stack)
- A frame that fixes a buffer's size: size the buffer from the frame (Stack)
- Two locals sharing one stack slot: few forms known (Stack)
- Another value spilled: change which value lives long (Stack)
- A stack struct's fields through saved registers: pointer locals to its fields (Stack)
- A link error after claiming `.rodata`: INCLUDE_RODATA, `force_not_migration` (Data)
- A module 8 or 16 bytes long or short: shared literals, padding objects (Data)
- A string symbol that won't link in a `.rodata`-owning file: write the literal (Data)
- A string before the jump table of a later user: INCLUDE_RODATA it in place (Data)
- Jump tables in another order: emitted last-created first (Data)
- A relocation `sym + offset` vs another label: give the symbol a `size:` (Data)
- A link error for another module's data: an address macro or a local symbol (Data)
- A `%lo` paired with the wrong `%hi`: declare a second symbol (Data)
- The last function of `.text`: a `text_end` piece for the padding after it (Data)
- One more round of copy propagation than yours: `#pragma optimization_level 4`; a mid-file region is a unit (Level 4)
- One file's loads not folded or values not propagated: a file-wide opt_* pragma, if all its functions still match (Level 4)
- A near miss that no form moves: score it at levels 3 and 4 with a pragma pair first (Level 4)
- A level-4 function matching only while a neighbour is C: test and match them together (Level 4)
- VU0 macro instructions: CodeWarrior `asm` functions (Not from C)
- Loops branching to their own 8-aligned entry: hand-written, stays assembly (Not from C)
- 8-aligned functions, `sd` saves, `beql`: GCC libraries, stay assembly (Not from C)
- Busy waits, `sync.l; ei`, gp stores: `__asm__ volatile` statements (Not from C)
- Empty delay slots alone: not proof of assembly (Not from C)
- Proving that no C gives it: builds, pragmas, the compiler's pass (Not from C)
- A permuter result: rewrite it as natural C; don't commit its false fixes (Not from C)
- Solved and still open cases: the lists at the end of the catalogue (Not from C)

### Saved registers in another order

**How MWCC picks saved registers** (round 12, from experiments: black-box
test functions). The allocator is a
Chaitin-style graph colourer, and every rule below is one of its steps:

1. **Numbering.** Each value gets a number: the parameters first (in
   argument order), then the locals in declaration order, then the values
   MWCC creates, in the order it creates them: temporaries, a
   strength-reduced walker, a hoisted loop invariant (a bound `first + n`
   written in the loop test), a common subexpression, a narrow parameter's
   extended copy, and a variable's second and later webs (the variable
   reassigned after its old value died; the first web keeps the
   declaration's place). A local that only copies a parameter (`u8 *b =
   p;`) merges with the parameter and takes its number.
2. **Removal passes.** MWCC goes over the values in number order, again and
   again, and removes each one that has fewer neighbours (values live at
   the same time somewhere) than a limit. For a value live across a call
   the limit is **8**: fewer than 8 other values still in the graph.
   **The zero register and sp count as neighbours** of every value
   defined before their last read in the function: zero is read by a
   small constant (`li`, `ori` from zero, `move x,zero`) and a compare
   with zero (`beqz`, `bnez`, `xor x,y,zero`), sp by a local array's
   address (`addiu x,sp,N`). Each counts once, is never removed and takes
   no register; the parameters' own registers don't count. Constants are
   read up to the end of most functions, so the effective limit is
   usually 7, the figure round 12 measured and the one the rules below
   use. sp's last read is the last use of a stack buffer, so values
   defined before it have one neighbour more than values defined after it
   (round 30, ShortMail_DrawForm: black-box cliques of 8 and 9 values,
   and a replay with this rule that reproduces all five of that
   function's builds; tests in docs/notes/nm/ShortMail_DrawForm.md).
   Round 12's first calibration said 6: its test call had 9 arguments,
   and a call with more than 8 arguments adds one neighbour to the values
   live into it (probably sp: the 9th argument is stored through it).
   Temporaries count: loaded values, constants (stored or
   compared, an address `lui/addiu` pair counts once), an `andi` feeding a
   branch, the loops' tests (the `slti` of a backward branch), arithmetic;
   they need not be live at the same time (4 load-and-store pairs before a
   call are 4 neighbours). Argument values loaded or moved straight into
   a0-a3, call results and a compare that only feeds a forward branch don't
   count. Liveness is taken after MWCC's first scheduling pass: when the
   scheduler hoists a value's last use (typically its argument move
   `a0 = x`) above a run of stores, those temporaries are not its
   neighbours (t33-t37 in the notes: temporaries between a value's last
   call and its use as an argument never counted). A value that fails waits
   for the next pass; the temporaries (numbered last, few neighbours) go in
   the first pass, so the waiting values usually go in the second, in
   number order.
3. **When a pass removes nothing** (7 or more values all live together
   across calls): the value with the fewest references (definitions plus
   uses; a use in a loop counts once) is removed, the last numbered of
   equals.
4. **Colouring**, in the reverse of the removal order: each value takes
   the lowest register its already-coloured neighbours leave free (s0
   first for values live across calls; v1, a0, a1, ... t0 ... for
   temporaries, v0 first when it is free; f1 ... f19, then f20 up, for
   floats).

What follows: when every saved-register value goes in one pass, the last
numbered is coloured first and gets s0 (the "reverse declaration order",
parameters highest, created values lowest). A value with 6 or more
neighbours in the first pass (a long-lived pointer over a busy region, a
counter kept across two loops) is coloured before all of those and gets
s0 whatever its declaration (from 7 neighbours: three values live across
calls plus four temporaries in their range is enough); among the waiting values, the number order
holds again. Removing a neighbour earlier in the same pass can let a value
go in the first pass after all (in func_0033CB20, a walker numbered before
the bound lowers the bound's count below the limit). Seven or more values live
together wrap: the six with the most references take s5..s0 by
declaration, the others s6, s7, s8 (fewest references first). Values that
never meet share a register. Float values have a higher limit (8 floats
across calls stay in order, f27..f20).

`python3 regalloc_predict.py MODULE FUNC --c FILE.c -D NON_MATCHING`
replays this on the original's and on our assembly: it splits the
registers into webs (named `s2@20`: register and first instruction),
counts each saved-register web's neighbours (round 31: with the limit 8
and zero and sp as neighbours, rule 2; a parameter's copy counted at the
entry and a delay slot's definition before its branch, as MWCC had them
before scheduling), searches the orders of the saved-register webs (all
orders up to 7 webs, a sampling local search beyond) for those that give
the original's registers, and prints what they share (`declared before:
s3@26 < s2@20`, parameters fixed first). With `--c`, it then pairs our
webs with the original's by first instruction and prints, per value, both
registers, the pass each is removed in, and the pairs our build numbers
the other way round (`ours numbers these the other way round (the
original needs A before B): s0@128 < s5@127`). `--opt NAME=VALUE`
switches the counting rules (OPTS in the script); `--opt r12=1` is round
12's model (limit 7, no zero/sp). `python3 rabench.py` scores the
model on matched functions: it numbers each function's saved webs the
C's own way (declaration order from MWCC's DWARF, `-g`) and compares the
replay with the build. Round 31: functions fully right, round 12's model
against the new: black-box tests 222 -> 262 of 305, sub_main 799 -> 822 of
1177, lobby 943 -> 951 of 1368, game 1182 -> 1237 of 1509.
Read
the result as: which values must be
numbered before which. When the original needs a created value before a
declared one, or a local before a parameter, no declaration order can do
it: change what is a declared variable and what MWCC creates. Seen in:
func_0033CB20 (sub_main: the original has no `end` local; `for (j =
first; j < first + n; j++)` makes the bound a hoisted value, numbered after
the walker `&gModelUsed[j]`); func_0034DA40 (sub_main: the model asked
for the created `item * 24` offset before the declared `icon`; declaring
`icon` as `u8`, the type of the byte it holds, instead of `s32` gave exactly
that, so a narrow local holding a loaded byte seems to be numbered like a
created value; the same narrowing did nothing for the cases below).
A local's integer type moves it in the numbering even where the code stays
the same: NetDev_Refresh (sub_main) matched with `u32 count` for `s32 count`
(the saved registers had rotated, count in s0 instead of s5). So when the
model asks for a value numbered elsewhere, try each integer local as u8,
s8, u16, s16, s32 and u32, one at a time and then combined:
`python3 regalloc_typesweep.py 0 60 OUT.log` does it for every
unapplied near miss in the near-miss sweep's results (a greedy three rounds, one
line per kept change, about 10 minutes in 4 shards: add `SHARD 4`). In
round 12 it matched these two and func_00348190 (`s16 full` for `s32
full`), and lowered about 20 others (their CSV notes say which type). Open
cases of the same kind: func_00D67C30
(plsel: the original numbers the strength-reduced `&recs[n]` walker before
the declared loop counter `kind`), Combine_CanUsePair (sub_main: the walker
r2 after the hoisted `(s16)b` of the second loop nest); the
sweep's results say what was tried. The tool works from the final schedule and
guesses which webs are MWCC temporaries: on 80 matched functions of
sub_main, main and game with 3 or more saved webs, some order reproduces
the registers for 62 (78%; 51 of the 57 with at most 3 free webs); on 80
of select, lobby, plsel and gm_sub, for 53 (66%; 33 of 39). Round 31's
benchmark (rabench.py) measures it another way: with the C's own numbering
the replay is fully right on 3,010 of 4,054 matched functions with two or
more saved webs in sub_main, lobby and game. Most misses
are large functions (7 or more free webs, where the fewest-references pick
decides). Small misses show the temporaries estimate is off by one or two
either way (func_00770410, lobby: the params only go in order if the `lui`
of a constant argument and the address chain count less; switching that
rule on fixes it and breaks as many others), and one pattern looked outside
the model: func_003D6CF0 (sub_main, now Patch_CopyFile) keeps `buf` in s0
below `hdr = buf + 0x40` in s1. It matched in round 17 with a typed local
copy of an s32 parameter (`u8 *buf = (u8 *)addr;`): a copy is numbered as
a local ("A parameter's register decided by a copy of it"). Treat the
tool as a guide. Round 18 checked it against eight 2-word register misses:
none needed a new numbering rule; their causes were prototype argument
types, the order loop walkers are bumped, placement around a loop's guard,
branch layout, `at` against v0 in a compare, and a stack slot. `--test FILE.c all` runs it on a
test file's functions.

**How MWCC colours temporaries** (round 28, from black-box tests compiled with the build's flags; the tests named here, t1 to
t11 and q1 to q6, are described in round 28's notes. Read with "How MWCC
picks saved registers": it is the same allocator.)

1. **The pipeline.** Code generation numbers the values (parameters, then locals in declaration
   order, then each temporary as it is created, then values later passes create); a first
   scheduling pass reorders each block; the colourer runs on that order; a second pass
   schedules again with the registers fixed. `#pragma schedule off` (put it after any
   `#pragma optimization_level`, which resets it) turns both passes off, so the listing shows
   the values in creation order with the same allocator: the way to read a draft's numbering
   (t3: three products, the loads hoisted with scheduling, in source order without;
   `#pragma scheduling off` is "illegal" for this compiler).
2. **Register order** v0, v1, a0, a1, a2, a3, t0..t7, t8, t9 (t4 q1: 14 locals live together
   take t9 down to v0, skipping the parameters' a0/a1); floats f0, f1, f2 ... (t8: four floats
   take f3..f0 in a void function; a float return value is coalesced into f0).
3. **v0 in a void function** is blocked from the entry, or from the last call that clobbers
   it, to the end; a float-returning function has v0 free (t8 ff). A call to a static function
   defined earlier in the file clobbers only what that function writes (Font_SetColorIndex:
   Font_SetColorRaw leaves v0 alone, so the temporaries after it skip v0).
4. **Simplify and select.** Passes over the values in number order remove each temporary with
   fewer than **23** neighbours left; the removed values are coloured in reverse, each taking
   the first register of the order that no coloured or precoloured neighbour holds.
   - t6: a pointer p live over n stored constants takes the highest register up to n = 22 and
     v1 from n = 23 (it waits, and is coloured before all of them); t7: with four parameters
     live through it, from n = 19: precoloured parameters count. The blocked v0 of a void
     function does not count (the same limit with `int` and `void`).
   - So in a region where fewer than 23 values meet it, the last created value is coloured first
     and takes the lowest register, and the first declared local takes the highest ("reverse
     declaration order"); a value with 23 or more neighbours (a long-lived local, pointer or
     count over a big block) waits and is coloured before all of them: MovieSfd_FramePacket's th
     (39 neighbours) holds v0 for 120 instructions; Font_AddGlyphSprite's tw, th, p and
     gFontCache have 71-90. round 12's regalloc_predict.py used 22 for this limit (tthresh).
5. **Creation order.** An expression's operands before its result; call arguments left to right
   (q6); a relational compare is not reordered as + and * are: `a[j] > a[g]` creates a[j] first,
   `a[g] < a[j]` a[g] (Plsel_UpdateSizeRecords: three compares turned round match it); so are `==` and `+` of two
   loads (t11: gA[i] + gA[j] and gA[j] + gA[i] number the two loads in source order, so their
   registers differ; q3's same code was a constant operand); a compare's left operand before its constant (q4: the load, then `li 5`, though the
   `li` is emitted first); commutative operands reordered: `(x + 1) * g[i]` and
   `g[i] * (x + 1)` give the same code (q3); an array read numbers the index before the base:
   every spelling of `((s32 *)gQuest.unk058)[e->arg]` (t9: pointer arithmetic, a local, a
   shifted byte offset, operand order) gives the same order. Later webs, hoisted and
   strength-reduced values and extended narrow parameters come after the code generator's
   temporaries (the andi of a u8 parameter numbered after later constants: EmScript_Op53).
6. **Interference is that of the first schedule.** The final listing separates values the
   colourer saw together, so a register can be skipped with no neighbour holding it in the
   final code (Font_AddGlyphSprite: 15 such webs). Over matched functions this is rare
   (below).

**Checks on matched functions** (tempcolor.py's analysis; round 28 nm's scripts):
- corpus.py, 150 random matched functions of sub_main, main and game, final code: a numbering
  that explains every colouring constraint exists for 150 of 150; in 132 every skipped
  register is held by a neighbour (the 18 others: values the final schedule separated).
- corpus_s.py, the same files compiled with `#pragma schedule off`: 139 of 150 fully explained;
  the order of first definitions keeps every constraint in 130.
- val.py, 121 functions, replaying the colouring (rule 4) on the final code: with the
  numbering read from the schedule-off build, 1320 of 1534 temporaries (86%) get their
  register, all of them in 92 functions (76%); with the order of the final listing 69% (50
  functions); with a numbering fitted to the registers 92% (102). The misses are mostly declared
  locals (numbered by declaration, not where the code first sets them) and webs the listing
  splits otherwise than MWCC. val2.py (200 functions, lobby too, seed 5): 86% (143 functions);
  numbering the locals first in declaration order, found from MWCC's own DWARF (`-sym on`
  writes each local's register; scratch dw.py parses it), adds almost nothing (86.4%), so the
  misses are not mostly about locals.

**Using it on a near miss:** `python3 tempcolor.py MODULE FUNC --c FILE.c -D
NON_MATCHING [-v] [--whatif]` prints, for the original and ours, the constraints each colouring
puts on the numbering ("A < B": A numbered before B), the registers that differ, and the pairs
ours numbers the other way round; `--whatif` replays the colouring with our numbering on the
original's graph and tries moving each value. Read a flip as "this value must be created
earlier/later": a local declared at another place, a value held in a local (numbered as
declared) or computed inline (numbered as created), or a later web of a variable (created after
the code generator's temporaries). `--test FILE.c [FUNC] --order --pragma "schedule off"`
prints a test function's numbering.

**How MWCC numbers the values later passes create** (round 29, nm-sub)

Rules found this round, each with the function it was checked on. "Numbered later" = coloured
earlier = the lower saved register when the values go in the same pass.

1. **A variable's later web is numbered after the loop optimizer's values** (strength-reduced
   walkers, hoisted invariant addresses), and later webs among themselves in code order of their
   first definition. The first web of a variable keeps the declaration's place, before all
   created values. Quest_RollRewards (matched): the drop loop's counter must be numbered after
   the supply walkers (`&gScene`, `&gScene.supply[n]` and `[28 + nb]`, s1-s3) and the drop table
   pointer before them. Declared `u32 j` (its first web): j s4; reusing `lim` for the drop table
   after the range cases (a later web): lim s0. With the range cases on their own pointer (lim's
   first web is now the drop table) and the counter a later web of an s32 that holds the
   goal count first: j s0, walkers s1-s3, lim s4, the game's registers. Where a value must be
   numbered after created values, make it a later web of a dead variable of its type; where it
   must come before them, make it a variable's first web.
   - Test t/web2.c: a loop with a strength-reduced `&gSup[n]` walker and a kept table pointer;
     the counter k declared and used only there gets s2 (walker s0); the same loop after `k =
     Use(m); if (k > 3) return;` (so the counter is k's later web) gets s0, the walker s1.
   - Kbd_Draw (33 -> 19): the page loop's counter and cursor walker (game s1, s2) had to be
     numbered after the loop's block-scope locals (cur s3, p s5); the box loop before it had its
     own block-scope `n` and `c`. Counting the box loop with `i` and walking the function-scope
     `c` makes the page loop's i and c later webs: right registers at once. Then the order of
     the two first webs (box loop: count s2, c s1) is the declaration order (c after i).
   - Which locals have a number of their own (from nm-temps, round 29): MWCC substitutes away
     a value that is only a plain copy or an expression assigned once, so it is numbered where
     MWCC creates it, not at its declaration; a local changed in place (`p += i`) or a pointer
     derived from another (`&h[2]`) keeps its own number, at its declaration. With rule 1: a
     declared variable's first web is numbered at its declaration only if it is not
     substituted away; a reuse (later web) or an in-place change keeps it a variable.
   - nm-lobby matched Page_DrawDropList with this lever (round 29).

2. **A common subexpression carried across a loop or branch** (EmSlot_FreeModels, EmSlot_LoadModels:
   the game keeps `n * 2` in s0 from before a loop to the block after it). Black-box tests
   (scratch t/cse*.c, t/gen_cse.py; the stores `s->a[n] = ..` with n an s32 parameter):
   - Within one basic block (calls do not end it) every repeat of `n * 2` is shared.
   - Across a block boundary (a loop, an if), a later block reuses the earlier value only if
     `n * 2` occurs **once** in that block (one array access with index n, or one address
     `&s->a[n]`). A later block with two accesses (`s->a[n] = 0; s->b[n] = -1;`, or two
     pointers `f = &s->a[n]; c = &s->b[n];`) computes `n * 2` again, whatever the first block
     has (gen_cse.py: 48 of 48 combinations of 1, 2, 1-call-1 occurrences, gS./s-> bases,
     loop/if/if-else separators follow this).
   - Exception (t/cse12.c): when the first block has its two accesses separated by a call
     (`s->a[n] = m; Free(4); s->b[n] = 1;`) and two later blocks each have two accesses, all
     reuse it (x6, y1-y3, y7, y8); with the call removed (y4) or one more call-separated pair
     in a later block (y5), each block recomputes. Not yet understood.
   - Session 2 (t/cse16.c, t/cse18.c; count of `sll x,n,1` over the function): what decides
     it is the first block's **first stretch between calls** that holds `n * 2`. One
     occurrence there (`s->a[n] = m; Free(4); s->b[n] = 1;`, or a bare `Use(n * 2);` before
     two accesses) and every later block reuses the value, even blocks with two accesses.
     Two occurrences in that first stretch (`a[n]; b[n];` adjacent; or `a[n]; b[n]; Free();
     Use(b[n])`) and later blocks with two accesses recompute. A store to a global, an `if`
     or a loop between the two accesses does not count as a separator; only a call does.
     Different bases (`gS2.b[n]`, `gS.a[n]`) change nothing. `a[n]; Free(4); b[n]; c[n];
     Free(3)` (one, then two) still recomputes: unexplained.
   - t/cse15.c, t/cse17.c (LoadModels shape): slot * 4 is kept (used once per block: rule
     2), slot * 2 recomputed per block whatever the call placement in the main block; the
     game keeps both. Pointers taken once (`f = &s->first[slot]`) keep `s + slot * 2` in a
     saved register instead (not the game's shape).
   - A goto to the next line (`if (*f == 99) goto L; L:`) between the two pointers splits the
     block: each half has one occurrence, so both reuse `n * 2`, and `s + n * 2` is shared too
     (the game's shape in EmSlot_FreeModels, but contrived). An empty `if (c) {}` or
     `do {} while (0)` does not split it.
   - So the game's source had, in the after-loop block, `n * 2` once: not found yet in a
     natural form (`count = first - 4` keeps the CSE but computes count from first).
   - t/cse14.c: EmSlot_LoadModels' natural shape (the early-return block, then blocks of
     `s->first[slot] = m; s->cnt[slot] = 1; s->models[slot] = Get(m);`) recomputes slot * 2
     and slot * 4 in every block (L1: 4 and 3 times; L2 without the early block: 3 and 2), where
     the game computes each once and keeps them (s0, s3). The draft's byte offset `o = slot * 2`
     gives the game's size; the game's own form is still unknown.

3. **Spill slots of parameter copies** (EquipFx_ChainUpdate, session 2). A local that is a
   plain copy of a parameter and never changed (`EmBone *b1 = pb1;`) is propagated away: if it
   spills, it spills as the parameter, in a packed 4-byte slot. A copy that is changed (`a +=
   0x20`) or an extension (`s32 n = pn` of an s16) stays a local and spills to a 16-byte slot
   (local slots are given in reverse declaration order). Casting the parameter's type (int or
   void * parameters), `register`, an inline wrapper, dead stores at the end, or moving the
   copy do not stop the propagation; a second definition of the local inside the loop does
   (b1, b2 got the game's 16-byte slots and frame) but changes the code. Taking the address
   puts it in memory (another slot area). The game's EquipFx frame has b1, b2, m as 16-byte
   locals: the game's form is still unknown.

4. **Where a pointer's increment goes beside the created walkers** (EquipMenu_DrawSkills,
   matched). In a loop whose arrays are indexed by i (MWCC creates the walkers), a `p++;` at
   the end of the body is emitted before the created walkers' increments; `p++` in the for's
   increment expression (`for (i = 0; i < 9; i++, p++)`) is emitted after them, as the game
   has. A walker of our own set in the for's init (`for (i = 0, cp = cols; ...)`) puts its
   set-up after `i = 0`; set before the for (`cp = cols; for (i = 0; ...)`) it comes first.
   The saved registers came from a full sweep of the six scalar declarations (720 orders:
   30 at 5 words, all with p, i, col, ty first in that order).

5. **An s16 parameter changed in place: `x += c` and `x = x + c` differ** (nm-lobby's find in
   DojoMenu_DrawMenu; confirmed by t/padd.c). With s16 x and s32 c, `x += c` narrows c
   (`dsll32/dsra32` on c), adds it to the raw x and extends the sum back into x's register;
   `x = x + c` extends x first, then adds c. The saved registers were the same in the test.
   Applied: ShortMail_DrawForm 30 -> 26 with `Font_SetPos(..., y += 0xC)` on the parameter in
   place of a copy `cy = y; ... cy += 0xC` (the game extends y in the prologue). The same
   function's `sel` (the game adds 1 to the raw sel, then extends) is the `sel += 1` form, but
   that moves four parameters to other registers (59 at best). regalloc_predict.py does not
   model a parameter's later web (nm-lobby): no order reproduces the game there.

6. **A narrow local's value lives in MWCC's extension, a compiler temporary** (WindowFrame_Draw,
   session 3). Spill slots are given from the top down: declared locals first (reverse
   declaration order: the first declared is highest), then compiler temporaries (extensions,
   CSEs, hoisted addresses) in their own order. `s32 style = f->style & 7;` spilled is a
   declared local (the highest slot); `u8 style` (or u16) makes the uses read MWCC's s32
   extension of it, which is spilled as a temporary: the game's slot 0x100 between the CSE
   `x + w` and the hoisted `&b.color`, with every other temporary slot then right. s8/s16/char
   give the signed extension's code (worse). So a spilled value in the temporaries' area that
   the C needs as a variable: try the variable one size narrower (unsigned when the value is
   masked).

**Which locals are values of their own** (round 29, nm-temps)

- **A temporary in another register: which values are locals of their own.** MWCC numbers a
  declared local's first web where it is declared (before every value it creates), and colours
  from the highest number down. A local that is only a name for a value is not a value of its
  own: a plain copy (`y = c0;`, `lbl = e->arg;`) or an expression assigned once and used once is
  substituted into its use, and moving or retyping it changes nothing. A local stays a value of
  its own, numbered as declared, when it is (a) changed in place (`p += i;`, `s = c0; s >>= 8;`),
  (b) a pointer computed from another (`u16 *sum = &h[2];`, `s32 *ofs = tbl; ofs += i;`), or
  (c) an expression assigned once and used twice or more (`sx = ox + x;` for the near corner
  and `sx + w` for the far one), where writing the expression out twice gives a CSE temporary
  that is numbered where it is created. So when tempcolor.py says a value must be numbered
  earlier, give it a local of one of these three kinds; when it must be numbered later, write
  it out (or use a later web of a dead variable, nm-sub's rule 1). Matched by it: Quest_StepScript
  (b), SaveFile_Encrypt (a, b, and a separate u16 key instead of a later web of the version),
  Color_Lerp (a: `u32 s` shifted in place), Loading_WriteSprite (c, with GS_XYZ's
  `(x | y << 16) | z << 32` order). Seen in: round 29, nm-temps.
- **A variable whose webs each have one definition is substituted** (round 30,
  ShortMail_DrawForm). `ly = sy + 0x60;` in one block and `ly = sy + 0x8C;` in another are each
  substituted into their uses: ly's first web is numbered after the locals, wherever ly is
  declared, and two separate s16 locals assigned once each are substituted the same way. An
  `s32 ly` assigned `ly = (s16)(sy + 0x60);` (an int holding the narrowed value) is a value of its
  own, numbered at its declaration, with the same code.

**Moving an s16 parameter in place** (round 29, nm-lobby)

- **`x = x + c` and `x += c` on an s16 parameter are different code.** `x = x + c` extends x,
  adds, and extends the sum back into x's own register (the sum is a later web of the
  parameter, kept in the parameter's register). `x += c` extends x in place once and then
  recomputes `x + c` (addiu, dsll32/dsra32) at every use: the sum is never stored. The same
  holds for an int copy (`s32 sx = x; sx += c;` or `sx = sx + c;`): the sum is substituted into
  each use. A separate local `tx = sx + c` (assigned once, used several times) is computed once
  and kept, and coalesces into sx's register when sx dies there.
- **Registers.** A parameter moved with `x = x + c` keeps the parameter's register; the same
  value in a separate local `lx = x + c` is numbered at lx's declaration, so its register then
  follows the declaration order (in the test a local declared before the other local gave the
  parameter form's registers, one declared after swapped them).
- Tests (scratch t/ip1.c, t/ip2.c, compiled with t/cc.sh at -O3,p): t1 `x = x + 0xAA` (extend,
  add, extend into s1, x's register), t2 `x += 0xAA` (x extended in place, `addiu v0,s0,170`
  at each use), t3 `lx = x + 0xAA` (lx in y's dead register), t4-t6 (parameter form vs a local
  declared before/after another local), t7-t9 (`sx += 0x28`, `sx = sx + 0x28`, `x += 0x28`: sum
  recomputed per use), t10 (`tx = sx + 0x28`: one `addiu s0,s0,40` in place).
- Matched by it: DojoMenu_DrawMenu (round 29: the cases' row positions as `x = x + 0xAA`,
  `y = y + c` on the parameters). Explains QuestDesk_DrawFieldGuide's game `addiu s4,s4,0x28`
  (a separate tx, not sx += 0x28). regalloc_predict does not model a parameter's later web as
  part of the parameter's node (for nm-sub).
- Where it does not help: a parameter still read after the move (GroupBoard: x and y are passed
  unchanged to Menu_DrawStamp at the end; CombShop: x/y as ly/tx push the params to s6/s4, 110-134).

**Values with 23 or more neighbours** (round 29, nm-wait)

(Round 29, from black-box tests compiled with the build's flags: scratch t2.py to t8.py, each
case a void function with two to five pointers, each loaded or computed once and storing 20 to
75 constants, so that every pointer has 23 or more neighbours. Read with "How
MWCC colours temporaries", rule 4.)

1. **Order among waiting values: the reverse of their numbering**, as for the others. The
   values with 23 or more neighbours wait in the first pass; once the short temporaries are
   gone they go in the next pass in number order, and are coloured in reverse: the last numbered
   takes the lowest register (v1 in a void function), the first declared the highest. Pointers
   computed into locals (`u32 *a = &ga_a[gi];`, t3.py "idx", t4.py "I"): the registers follow
   the declaration order only, whatever the order of the assignments (abc/cba/bac) and of the
   uses (abc.../cba...), scheduled or with `#pragma schedule off`. tempcolor.py's simulate()
   already replays this (its pass 2 runs in number order).
2. **A local copied from a global and used before any store that may alias it is numbered
   after the other locals** (as its load, a created value): `u32 *x = g_x;` whose first use
   comes before any store through a pointer takes v1 (coloured first) wherever it is declared
   (t2.py: in every two- and three-pointer case the first used took v1; t5.py: with three
   computed locals, x takes v1 only when its use comes first; t7.py: a store to another global
   `gk = 1` first keeps the effect, a store `*gip = 1` first cancels it, and a conditional
   return between two such uses leaves the second at its declaration). Only that first value
   moves; the others keep the declaration order. A field load (`u32 *x = gs->p;`, `*gpp`)
   keeps its declaration's place.
3. **Narrow locals**: a u16 local holding a loaded halfword is numbered like a created value
   wherever it is declared; as u32/s32 it takes its declaration's place (Browser_ListCursor:
   every declaration order of `u16 pressed` gave the same registers; `u32 pressed` declared first
   matched). Round 12 saw the same for a u8 (func_0034DA40).
4. **A later web** (a dead variable reused) is numbered after the code generator's
   temporaries (nm-sub's notes, rule 1): AreaSky_Draw matched by holding the choice count in the
   model loop's dead counter `k`, so that it is numbered after lvl's andi and coloured first.
5. **Many "wait" rows were not waiting values.** tempcolor.py runs with `rp.CALL_RD = True`:
   a call reads each argument register (a0-a3, t0-t3) that any path sets since the entry, so a
   value that only reaches a call through some path is precoloured and live up to it, with
   dozens of neighbours. Browser_ListCursor's pressed/tilt ("33 neighbours") and AreaSky_Draw's
   lvl/cnt ("45/64") had fewer than 23 with it off (scratch tcx.py), and the flips tcx.py named
   led to both matches. On matched functions CALL_RD is still the better default (scratch
   valw.py, 145 functions of lobby and sub_main, seed 5: on 1519/1870 temporaries and 118
   functions fully right; off 1555/1954 and 93), so read a block both ways.
6. **Checks on matched functions** (valw.py): only 24 of 1870 temporaries have 23 or more
   neighbours, and the replay colours 8 of them right; the misses are in unk_2F6540.c's
   ModelChunk* readers, unrolled loops (x8) where the listing's webs of the walker are not
   MWCC's, so the graph, not the order rule, is off there. No rule beyond 1-4 was found.
7. **A field address shared by a load and stores is numbered after its base; a pointer local
   numbers it first** (session 2). MWCC keeps `&gLobbyMenu->step` in a register when the field is
   read and stored several times in one block (the load folds the offset, `lbu a1,47(a2)`, the
   stores use `addiu a3,a2,47` / `sb ..,0(a3)`); that address is created after the gLobbyMenu
   load. IdMenu_Choose (matched) numbers it before: `u8 *sp = &gLobbyMenu->step;` as a declared
   local (its first web, so its declaration's place), `switch (*sp)`, `*sp += 1`, `*sp = 2`.
   Test: 216 variants (the declaration orders of pad, sp, b and the types of pad and b): the 16
   that match all have pad declared before sp; sp without the s32 copy b 17, b without sp 11.
   The same block shows rule 3's other side: a 32-bit copy `s32 b = pad;` of a u16 local is a
   first web of its own (numbered at its declaration) and gives the game's second `andi` of the
   buttons, where the old `pad = (u16)pad` no-op kept pad's single number.
   Checks that failed (session 2): the same move (a value made a declared 32-bit local to number
   it first) put hd right in Em_TurnToward but moved diff and hold (59), and gives 83+ for an
   explicit walker in NgWord_Check (the walker changes the code there).
8. **A value the game numbers after a rule-2 local must be a declared local too** (session 2,
   Page_DrawObjBack, matched). `st = gBrowserState` (rule 2: numbered as its load) is numbered
   after every declared local's first web, and before the code generator's later values (the
   CSE of e->color in case 16). The game numbers st after the colour load: so the colour is a
   declared s32 local (`color = e->color;` for the test and the first rectangle), as are kind
   (both switches) and sx (= st->x), in the order sx < kind (rule 1's declaration order).
   Test: the 24 declaration orders of sx, kind, color, st: the 12 with sx before kind match;
   without the color local every order gives 15 (session 1's 6-order search), the color local
   alone 10, color in all four rectangles 510 (kept across calls).
9. **Block-scoped locals are numbered after every function-scope local** (session 3,
   CseBank_Merge 101 -> 31). A u16 `vbase` declared at function scope (any of 20 places) was
   numbered before the Smpl loop's block-scoped ss, d, vs; as the reused `base` (a later web)
   it was numbered after the loop's in[] walker. The game numbers it between (after vs, before
   the walker): so ss, d, vs and vbase are all function-scope first webs, declared in that
   order (sdvb 31, sdbv 44, bsdv 62). Where in the function-scope list the four go changes
   nothing (6 places, same score): only their order among themselves counts, since they do not
   interfere with the other sections' values. A value the game numbers after a loop's locals
   but before the loop optimizer's walkers is a function-scope first web declared after them.
10. **Two CSE facts behind code differences in the same block** (CseBank_Merge):
   - `(x->max + 1) * sizeof(T)` after a loop whose condition is `j < x->max + 1` reuses the
     condition's max + 1; the game recomputes it from the loaded max (`addiu` again). Writing
     `((u32)x->max + 1) * sizeof(T)` gives the game's code (the conversion makes it another
     expression); `1 + max`, `sizeof(T) *`, `* 16`, `(u32)(max + 1)` are canonicalised to ours.
   - Records after a 16-byte header: `((T *)(c + 1))[j]` makes the walker start at c + 16;
     `((Chunk *)c)->recs[j]` (a struct with the header and the record array) starts it at c,
     with the 16 folded into the loads, as the game has.
   - A bank pointer `b = in[i]` used for two fields keeps in[i] in a register across the
     inner loop (the game's `lw t3,16(t5)` for bodySize); `in[i]->` twice reloads it after the
     loop's stores.

Using it: run tcx.py (or tempcolor.py) on a block; for each flip "A < B" make A numbered
earlier: a 32-bit declared local (first web) before created values, a narrow local or a copy
of a global used at once (rule 2) as created, a later web of a dead variable after them.

**How MWCC schedules** (round 29, nm-sched)

Black-box tests in scratch t/ (compiled with the build's flags through sc.py). What is known
from round 28: a first pass orders each block before colouring, a second one after, and `#pragma
schedule off` turns both off and shows the creation order.

0. **Heights first.** The scheduler places first the instruction with the longest path to the
   block's end; the branch that ends the block is an ordinary consumer, and a value live out of
   the block counts its chain inside the block only (t/h1.c: `cur = col * 4 + row` (lbu, sll,
   addu) goes before a `mode` load that feeds the branch (lbu, bne); `row` and `mode`, both
   lbu then one consumer, keep their creation order).

1. **A block that starts at a label is scheduled as if its first issue slot were taken.**
   A block reached by a branch (a join after an `if`, the target of a taken branch, the exit
   of a `for` with a zero-trip test, the code after a `switch`) issues its first instruction
   alone: the instruction that depends on it follows at once. A block reached only by falling
   through (after a `do`-`while`'s back branch, after a call, the then-block of an `if`) pairs
   its first instruction with an independent one, which ends up between the two.
   - t/a4.c, a8.c, g1.c: `F(D1, 0)` gives `lui a0; addiu a0; move a1,zero` after `if (x) g();`
     and `lui a0; move a1,zero; addiu a0` after `do {} while (g() == 5);`; two stores
     `gw = y; gx = 2;` give `lui at; sw s0; li v1,2` and `lui at; li v1,2; sw s0`; `F3(gw, gx,
     D1)` gives `lui at; lw a0; lui a2` and `lui at; lui a2; lw a0`.
   - Checked on the game's code (scratch chk1.py, every C file's original asm/<module>/<file>.s):
     blocks that set up a call as `lui rX; addiu rX,%lo; li rY` then `jal`: after a label 38 of
     38 keep lui and addiu together; after a branch's fall-through 8 of 9 split them, the ninth
     being func_00183C80 (labelled in the source, see below). Blocks of other shapes, where more
     instructions compete, are mixed (label: 78 together, 61 split; fall-through: 14, 66): rule 1
     decides only between otherwise tied instructions.
   - Dead control flow does not make a label (an empty `if`, `goto L; L:`, `switch (r) {
     default: }`, `for (;;) { if (...) break; }`: t/a6.c, a9.c, b2.c): MWCC removes it before
     scheduling. A `switch` with a case does: `for (;;) { switch (f()) { case K: continue; }
     break; }` (or `retry: switch (f()) { case K: goto retry; }`) compiles to the same
     `jal; li; beq` loop as `do {} while (f() == K)`, with the block after it labelled
     (t/b2.c P3, P4). That matched func_00183C80.
2. **An in-place update goes after an independent new value.** Of two independent
   instructions of the same height (the same path length to the block's end), one that updates
   a variable in place (`w -= off`, `t -= k0`: it reads the variable it writes) is scheduled
   after one that makes a new value (a temporary, or an assignment to a variable from other
   values); two in-place updates, or two new values, keep their creation order (`#pragma
   schedule off` shows it; a local used once is created at its use).
   - t/g2.c, g3.c, g5.c (`t -= k0; c = k1 - k0; *o = (f32)t / (f32)c;` and 20 variants): the
     in-place `t -= k0` goes second in every statement order; `d = t - k0` with `c = k1 - k0`
     follows creation order; `t -= k0; k1 -= k0;` too.
   - The order shows in the final listing even where the registers make both in place (`subu
     a0,a0,a1` for a `d = t - k0` that the allocator put in t's register goes first): what counts
     is the variable, before allocation.
   - Kbd_DrawLine matched by it: the game computes `w - off` before the strncpy's `&conv[skip]`,
     ours after; `n = w - off;` (a new variable, as the function's candidate branch writes it)
     keeps creation order.
   - Exception seen (rule 1): a copy from a saved register (`move a1,s0`) still goes between `lui` and
     its `addiu` in a labelled block (t/g1.c J_c2): a tie-break, below.

3. **Stores and loads keep their order; constants and free values fill around them.** Two
   stores through pointers (the same base or not, any types) and a load after a store (any
   type, even a float load after a byte store) never change order: to move a store, move its
   statement. The value a store needs (a `li`, a `lui`/`ori` float constant) is scheduled ahead
   of it, into the first free issue slot. Two stores to different global variables are
   independent: one whose value is ready (a parameter, an earlier value) goes first, then
   those that wait for a constant (`gx = 0` counts as waiting: `sw zero` is after `gy = x`),
   otherwise creation order.
   - t/k1.c S1-S5, t/k3.c R1-R9 (`p->a = 5; return q->b + 1;`: the load stays after the store
     for s32/s16/u8/f32 loads, globals, the same base), t/k4.c W1-WE (`ga = 0; gb = x;` gives gb
     first; `ga = 1; gb = x;` too, the li hoisted; `ga = x; gb = y;`, `ga = 1; gb = 2;`, `ga = x +
     1; gb = y + 2;` keep source order).
   - Matched by it: Chara_SetupNewPlayer and Chara_SetupSavedPlayer (session 2), whose stores
     the game makes in another order than ours (motionLayerCount after alpha, rot[1] = 0 after the
     start position): the statements in the game's store order matched both at once.
4. **The model: top-down, two instructions per cycle.** The tests fit a list scheduler that
   fills cycles from the top, two issue slots per cycle, a load's result usable two cycles
   later: short independent instructions fill the slots beside a load chain (t/k6.c: `G3(p->n->
   n->v, k + 1, m + 2)` gives `lw; addiu a1; [addiu a2]; lw; lw`), two ALU updates issue in one
   cycle before the compare that uses one of them (t/k5.c B2: `j += 2; i++;` then `slt j`: the
   in-place `i++` stays between, `addiu s0; addiu s1; slt v1,s0`). Rule 1 is this model's first
   slot. Between equal candidates a load goes before an ALU value (t/k1.c A1, A2), and creation
   order decides between equals of one kind (L1, U1). Arguments are created in argument order
   except that a simple ALU argument (`k + 3`) is created before a load argument (A1-A4, off).
   - The delay-slot filler is a separate step: it takes the last instruction the branch does not
     need, skipping loads (t/k6.c D1: `lw; addiu a1; lw; lw a0; j; addiu a2`: the load `lw a0`
     stays, the earlier `addiu a2` moves into the slot).
   - What the final listing shows is the second pass, with the registers of the colouring: a
     register reused after the first pass ties the second pass to the first one's order (every
     `lui v0` chain of AreaObj_K11_Draw's case 2 follows the first pass's order).
   - Corpus check of rule 2 on the game's asm (scratch chk2.py, chk2s.py): adjacent independent
     ALU pairs with one in place, by register: 11534 in-place second, 11421 first; on saved
     registers (variables), 267 against 196, the 196 mostly loop steps created before the loop
     test. Registers do not show which value was a variable before allocation, so the corpus
     says nothing either way; rule 2 rests on the tests and Kbd_DrawLine.

5. **Register pressure decides between equal heights (rule 2 is a case of it).** Of two
   ready instructions of the same height, the scheduler first takes the one that leaves fewer
   values live: a new value whose operands all stay live adds one (+1); a new value that is the
   last use of an operand (+0), or an in-place update (`t -= k0`: the same variable before and
   after, +0), add none; a last use that defines nothing new lowers it. At equal counts, one that
   ends an operand's life goes first, then creation order. An in-place update ends no variable's
   life (the variable lives on), so beside a new value that is some operand's last use it goes
   second (rule 2); beside a new value whose operands all stay live (+1) it goes first.
   - t/m1.c K0-K9 (the rule 2 pair with uses added after the division): `t -= k0; c = k1 - k0;`
     gives c first (K0), t first when k1 is used later (K1: c is then +1), and still t first with
     c created first (K3); `d = t - k0; c = k1 - k0;` with t used later gives c first (K5: d +1,
     c +0), with t and k1 both used later creation order (K6); two in-place updates keep creation
     order (K8, K9).
   - Matched by it: Color_EvalKeys (session 3). The game's `t - k0` before `k1 - k0` is a new
     value that is t's last use (+0, ends t) created first, beside the denominator (+0, ends k1):
     creation order. Written `c0 = t - k0;` (c0, the u32 local later loaded with k[1]) it also
     takes the game's registers; a new `s32 d` gave the order but not the registers (6).
   - Reading a near miss: when the game puts first what ours puts second, count for each of the
     two what it ends and what it creates; to move one earlier, make it a last use (a new value
     instead of an in-place update), or keep the other's operand live.

6. **A loop-invariant cast is hoisted out of a loop; a plain `k - 3` is not.** In a loop with
   calls, `(u32)(k - 3) <= 1` on an s32 k computes `k - 3` once before the loop, in a saved
   register (with a nop before the loop label); `k - 3 < 2` (s32) and `k - 3 <= 1` on a u32 k
   compute it into v0/v1 on every pass. The cast (s32 to u32) is a value of its own that the
   loop optimizer hoists; `k == 3 || k == 4` on a u32 k gives the same unhoisted range check
   (`addiu; sltiu at,v,2`). So a range check that the game recomputes in a loop is on an
   unsigned variable, with no cast.
   - t/n1.c-n6.c (T1-T8, U1-U9, W1-WC, X1-X3; the same at level 4): every `(u32)` form
     hoisted, every form on a u32 variable (a parameter, a copy of a u8, a load) not.
   - Matched by it: EquipPreview_Draw (session 3): `u32 k;` and `(k == 3 || k == 4 || k == 6)`.
   - Trap met on the way: vtry.py replaces the first occurrence in the file; a declaration
     like `    s32 k;
` must be anchored on its neighbours, or the change lands in another
     function (four functions of unk_36E1D0.c declare `s32 k`).

**What round 30's single-function agents found** (round 30; each function's notes, docs/notes/nm/FUNC.md)

- **The first scheduling pass decides which values can share a register** (AreaObj_K11_Draw,
  BoxMenu_ArrangeMenu). Before colouring, the scheduler fills two issue slots a cycle, a load's
  result usable two cycles later, one load a cycle; in each slot it takes an instruction that
  ends a value, otherwise the earliest-created ready one. The second pass (the final listing)
  takes loads first, then ALU values in creation order, so most statement orders look the same
  there. When the final listing already has the game's shape but a register differs (tempcolor
  reports a base register "skipped with no neighbour"), change the creation order for the first
  pass. BoxMenu_ArrangeMenu: the swap block's loads held in locals used twice (`h = *held;
  k = *k2;`) and created as held, `*held`, k2, `*k2`, slots make slots the gBoxWork base's last
  use, so the two share v1 as in the game. Every order of the five statements compiled at once
  (a generator that builds all the variants into one file and scores each): this one 0, one
  other 2, the other 28 7. A local used once is moved to its use and doesn't fix an order.
- **Colouring before scheduling** (EmFxSpark_Draw). Replay the colouring over every possible
  first-pass order (9,240 there) before reading a difference as scheduling: there the colouring
  decided it and the schedule followed. In `L | R` MWCC evaluates the right operand first when
  it has two or more operations more than the left (a float-to-int conversion counts as none).
  At level 4 a local assigned once and used once is substituted into its use; one whose second
  assignment reads the first (`a = (u32) (a * (fx->timer / 20.0f)) & 0xFF;`) stays a value of
  its own.
- **A global load used twice is numbered at its second occurrence** (Gs_ClearFrame); a field
  load held in a local keeps the local's declaration number, where a plain copy of an extern is
  propagated away. `oy = gSysWork.offsetY; ox = gSysWork.offsetX;` numbers the loads in the
  game's order.
- **Later webs, refined** (NgWord_Check; rule 1 of "How MWCC numbers the values later passes
  create"): later webs are numbered in the order of the variables' first webs, and a first web
  must be real (two or more definitions reaching a use). The scan written like its lobby twin
  WordFilter_Scan matched, with no casts.
- **A pointer local for a sub-packet** (Model_Draw): the END tag written through its own `u32
  *last = q + 8;` makes its stores wait for the pointer in the first schedule, giving the game's
  store order.
- **The optimizer's passes, in order** (Tim2_GetImage, from the pass names in mwccps2.exe; IR
  dumps are disabled): the IRO pass loop (copy propagation, expression propagation, dead stores,
  folding), PullOutInvariantAssignments, LoopUnroller, DoCommonLoopTransformations, a second
  copy propagation, CommonSubs, then SplitLifetimes, RegenerateExpressions and Delinearize. The
  tests: a copy between pointer types (`ph = (T *)p`) is never propagated; a same-type copy is
  propagated into every use outside loops, but not into a loop's own test and not after the
  source changes; the common-subexpression pass treats loads through two pointers as different
  even when one is a copy of the other; the back end's copy propagation works within one basic
  block. The unroller needs a `for` loop (`init; goto TEST`) and keeps the original loop for a
  variable count's leftover iterations.
- **Where a common subexpression is numbered: not settled.** Gs_ClearFrame's block and
  EmSlot_LoadModels' test functions (scratch f8, f10) put it at its second occurrence, its first
  reuse; StageObj_K29_Draw's results fit its last occurrence. Test the block's own case. A permuter
  find that made
  `node->owner` a common subexpression whose last use is a late case numbered it last; reloading
  `p->type` in the cases ended its subexpression early and moved another value's register. A
  constant initialiser in a declaration (`u8 kind = 1;`) is created at the top, where it can
  gather 23 or more neighbours and wait; written as a statement later, it is numbered there.
- **The order values live across a call are removed in** (Mesh_BuildSkinned), and so coloured in
  reverse: the declared locals' first values in declaration order, then the values MWCC creates,
  then the reused variables' later values in the order of the variables' first uses. A count kept
  in a fresh local declared early, and a pointer made the later web of a cursor first set after
  the loop counter, put two values in the game's registers there.
- **Short webs declared first** (LobbyMsg_DrawList): where a value MWCC creates (a hoisted `0x6E -
  w`) is coloured too late among long-lived locals, declare earlier the short webs that overlap
  them all. They are removed first and lower every neighbour count, so the created value, numbered
  last, is coloured first.
- **One node or two** (CombShop_DrawAmount): the statement `v = (s16)(v + c)` splits an s16 local
  into two nodes; `v += c` inside a call's argument (`Font_SetPos(tx, ly += 0x18)`) keeps one node
  and stores the extended sum in it. When a later web keeps its first web's register although a
  lower one is free, the game's variable is one node: look for the update inside an expression.
- **Reused variables and copies** (MovieSfd_FramePacket, Loading_Open): a variable read unset on
  some path is never split into webs, so it can't give a later web; the unroller's bound is
  numbered after every declared local, block-scope ones too, and before a reused global load and
  later webs; a u32 copy of an s32 value keeps a value of its own, where an s32 copy is substituted
  away; an unused declared local still shifts the numbering. One loop counter reused for three
  loops separated two walkers that every declaration order coupled. Registers that look free in
  the final listing can be skipped because of the colouring order.
- **Two waiting values** (DlgInput_Draw): of two values with 23 or more neighbours, the one
  numbered later is coloured first, so where a reused global load's second occurrence falls
  decides it. Writing `gBrowserWork->obj[gBrowserSub->listObj]` out at each use moved it past the
  constant 4.
- **An unsigned `% 256`** (BoxMenu_DrawItemList): MWCC seems to turn it into an `andi` only after
  the first scheduling pass, so the colouring sees a longer live range than a plain mask gives. An
  `andi` the game keeps live longer than any first-pass order allows can be an unsigned `%` by a
  power of two in the source.
- **A const view of a global** (Page_Click): a `const T *` copy of a `T *` global is a local of its
  own, numbered at its declaration; a plain copy is propagated away (round 29's 96 plain copies all
  stayed at 21 words).
- **State left by an earlier function in the file** (Browser_ReadPad): once a function earlier in
  the same .c file has compiled a two-argument runtime call (dpsub, dpadd, _dpflt, __divdi3...),
  MWCC models its own `litodp` call as reading a1 too, so a1 stays busy from the entry. If a
  register is avoided for no reason the block explains, look at what the functions before it
  compile. Nothing in lobby makes such a call any more: see the stand-in ruling (CONTRIBUTING.md, "Fakematches").
- **Later webs come after the values MWCC creates** (EmSlot_LoadModels,
  GmEm_SetMeshAlphas). A value the game numbers after MWCC's common subexpressions and walkers is a
  later web, a variable set again after its first value died; a declared local's first web is
  always numbered before them. Reusing one pointer for both walks of the same
  array makes the second walker a later web; GmEm_SetMeshAlphas wrote an earlier case's
  ternary as if/else, which made the loop's count a later web. The walkers MWCC creates for a loop
  are numbered after the declared locals and before the loop body's temporaries.
- **Splitting a reused variable** (BoxMenu_DrawEquipPick): when a reused variable's later web
  takes a register too early, give one of its values its own local, so the other becomes a first
  web numbered at its declaration.
- **A narrow local holding a load is numbered like a created value** (ModeSel_DrawButton,
  Kbd_Draw): an s16 local holding a loaded halfword (or a byte) is numbered after every declared
  local wherever it is declared, so moving it changes nothing; declared s32 it keeps its
  declaration's number. An observation not yet tested on its own (ModeSel_DrawButton): there the
  values were removed in the order item, the locals declared after sx, sx, then the locals
  declared before it, which regalloc_predict's simulate doesn't reproduce.
- **`x << n` against `x * 2^n`** (Kbd_Draw): the two give an addu's operands in opposite orders.
- **Masks at each use** (Em_TurnToward): where the game compares the masked copy `(u16) x` it has
  already made, writing the mask at each use keeps that copy live as in the game. Two equal
  expressions in the two tests of an if/else-if chain become one common subexpression.
- **Signed and unsigned compares, and where walkers start** (CseBank_Merge): a `u32` count gives
  the game's `sltu`, and `(s32)max + 1` in a loop bound its `slt`. A walker started in a `for`'s
  init comes before the loop's guard; indexing the array lets the loop optimizer start its walker
  after the guard, where the game has it.
- **tempcolor and jump tables** (GmEm_SetMeshAlphas): with jump-table cases every t-register looks
  live from the entry, so tempcolor.py can't read such a function (round 28's 500-1174
  neighbours came from that). Float values don't count as integer values' neighbours.
- **Loop variables decide spills and rotations** (WindowFrame_Draw, 482 words to 0): which loops
  share a counter, end or step variable, and which use is a variable's first web, decided both
  which value spills (h spilled as in the game only once an inner counter was a later web) and
  the saved-register rotation (separate s32 `endx`/`endy`, a reused `step` and its own `stepx`).
  Declaration order and types did nothing there.
- **Casts that make or break a common subexpression** (EmSlot_FreeModels): an explicit cast in a
  local's initialiser (`s32 n = (s32)i;`) can make the local a common subexpression numbered
  where it is reused; an explicit cast in one of two equal index expressions keeps them from
  forming one in that block (tagged there: it only keeps them apart).
- **Which half of an OR is built first** (Font_AddGlyphSprite): `(u64)(u32)(v * 16 + 8) << 16`
  makes MWCC build that operand first, `(u64)(v * 16 + 8)` the other; the colouring follows that
  order. A dead variable reused for an intermediate value (`tw = u + tw - 1;`) created it before
  a later update and fixed its register.
- **A parameter's copy and its stack slot** (EquipFx_ChainUpdate): a plain copy of a parameter is
  propagated away and spills as the parameter, in a 4-byte slot; a copy that a second definition
  reaches stays a variable with a 16-byte local slot. Arrays declared in another order move to
  other offsets.
- **Check a block's meaning** (QuestDesk_DrawFieldGuide): round 29's 44-word block reused a
  variable across a loop that overwrites it, so its final compare tested the loop counter instead
  of the flag; with the flag restored the block was 128 words. A NON_MATCHING block must do what
  the game does: check a find's meaning before keeping it, above all a reuse of a variable that
  a loop between its uses overwrites.
- **A value at the 23-neighbour limit** (Kbd_DrawSpecialKey): it stops waiting when one of its
  neighbours is made a declared local's first value, numbered before it, so that the neighbour
  leaves the colouring first. Choose one that shares its register with other values in the game,
  so it still finds that register when coloured later: there `ofs = gKbd->mode * sizeof(KeyPos)`
  declared before cur, with the reads `(&gKbdSpecialKeyPos->row)[ofs]` keeping the game's two
  table bases.
- **Fewer values before a chain** (ObjAttach_DamageEm): moving four independent float
  initialisations after the two statements of an address chain changed the first scheduling pass,
  and with it the colouring, while the final listing stayed the same. A probe that tests one
  register property of the game's colouring (two values sharing a register), run over 300 random
  statement orders, found the state that 225 single moves and every expression form had missed.
- **How MWCC chooses what to spill** (EquipWindow_DrawDetails, black-box tests with 11 values live
  across one call): when no value can be removed, it pushes the value with the fewest references
  (definitions and uses); ties go to the last-numbered value, and the degree plays no part. A
  pushed value spills only if its coloured neighbours already hold all of s0-s8.
- **A spilled constant is rematerialised** (Menu_DrawQuestSheet): MWCC emits the constant again at
  each use, straight into the argument register, rather than storing it. A constant local set once
  is always propagated; with two definitions it stays a value of its own, and a repeat with
  nothing between the two is dropped.
- **A cursor set again from its copy** (Tim2_GetImage): with `ph = p`, setting p again from ph
  before an unrolled loop (`p = ph + sizeof(Tim2PictureHeader)`) makes the leftover loop's guard
  read through p, while the loop's own test keeps reading ph.
- **The stripped-function effect through implicit conversions** (Plsel_BuildResultText): the
  litodp effect of Browser_ReadPad also holds for the fptodp calls MWCC makes for a float passed to
  a variadic call. There it kept a copy (`move a1,v0`) after a size test and left the branch's
  delay slot empty; an explicit `fptodp(f)` call doesn't have the effect.
- **One more reference to a long-lived value** (EquipWindow_DrawDetails, 1,128 words to 0): when
  a spilled value's neighbours fill the registers, check the colour order of the long-lived values
  first. One extra use of a parameter (`p = Player_PreviewWith(p, e);`, which the game also keeps
  in p's register) moved p in the colour order and took kind out of the spill set; adding one use
  of each long-lived value finds the one that matters in a few compiles.
- **Dead variables reused to place first webs** (QuestDesk_DrawFieldGuide, Page_DrawDropList):
  giving two locals their first values at an earlier line (the title's x in tx and lx before
  their list values), or swapping which loop variable counts what, fixed register chains that
  declaration and statement orders could not.
- **One value's reads made different** (Cam_LineHitDist): three offsets each read twice are
  shared values numbered at their second read; making only one value's two reads look different
  to the optimizer (a comma, an index fixed at 0, a cast) renumbers it after the others. No
  natural form gives that; it stays tagged.
- **Zero and sp as neighbours; single-definition webs** (ShortMail_DrawForm, 22 words to 0, the
  last NON_MATCHING block): the removal limit for values live across calls is 8, with the zero
  register and sp counted as neighbours ("How MWCC picks saved registers", rule 2), and an s32
  holding a narrowed value keeps its declaration's number where an s16 assigned once is
  substituted ("Which locals are values of their own").


- **Saved registers swapped or rotated between locals.** Cause: MWCC gives out
  s-registers in reverse declaration order: the last declared local gets s0,
  the one before it s1, and so on. Parameters kept in saved registers come
  after all the locals, in the highest ones (the last parameter gets the
  lowest of those; a narrow one converted at the entry is an exception, see
  "Parameters in each other's saved registers"). With more than six the order
  can wrap (one function of unk_363550.c gave its first six locals s5..s0 and
  the last two s6, s7). Fix: reorder the declarations. When the order can't be
  read off, try every order: 120 orders take 20 to 60 seconds with
  `func_variants.py`, and the autodecomp tools try the order the registers
  suggest (`t_regorder`). Seen in: func_002F99F0 (sub_main),
  Plsel_BuildAwardText (plsel), LobbyMsg_DrawPage (lobby).
- **A value that isn't a variable in your C takes the lowest saved register
  and pushes the declared ones up.** Cause: values MWCC creates (a hoisted
  loop invariant such as `(u8)sel`, a strength-reduced `&table[i]`, an
  address written out at each use) are allocated before every declared
  variable. Fix: declare the value where its register says (`cur = (u8)sel;`
  declared between two others, an explicit `s32 *p` before `i`), or the
  reverse: write the expression at each use so that MWCC makes it
  (`w->items[k].kind`, `i ^ 1`, `(alpha << 24)` at each use). The
  reverse too: a declared variable takes a higher register than the one
  MWCC makes (`k += sizeof(NetUser)` as a local byte offset sits between
  the pointers, where `p->players[i]` makes the offset in the lowest; an
  explicit walker `sc` takes a declared variable's register where the
  game indexes `D_006ADCB0.unk224[i]`). Seen in: Parts_DrawTypes,
  ModeSel_Draw (select), Plsel_DrawItemBox (plsel), func_007C1360
  (lobby), HuntingHorn_PlaySong (game), Player_BuildPartIds (sub_main: s0
  and s1 swapped until `u8 *q;`, declared at the top, held `&ids[n]` in
  `if (n != 1) { q = &ids[n]; if (*q >= K) *q = 0; }`; declared in the
  loop's block, it changed nothing).
- **A later loop's or a switch case's variables in other saved registers.**
  Cause: a variable keeps one register within a live range; MWCC can give
  one variable's separate live ranges different registers (one u16 `i`
  for two object loops: t0 in the first, s0 in the second, where a second
  variable `j` swapped with the walker). Fix: give the second
  loop its own counter, walker or temporaries (a counter shared by three loops
  got the lowest register in all of them), or reuse the first loop's, or an
  earlier local of the same type for a new running sum (both occur); give a
  case its own temporary or pointer when the original keeps the value
  elsewhere (`b` and `b2` for the same sum in two cases; a pointer set in the
  case that uses it, `w = (s8 *)work;`, not at its declaration; `s32 k = kind
  & 0xF` for `kind &= 0xF` in one case). Four copies of one loop with their
  own registers are four sets of variables, declared in the order of their
  registers. When two values swap and no declaration order helps, move one
  into a local of its own and permute again. A callee's result kept across
  a call only in some cases gets a saved register there alone when each
  case has its own variable (`k` for cases 3 and 9, `t` for the others);
  a loop's second index can live in the first loop's result variable
  (`for (i = found, found++; i < 4; i++, found++)`). Seen in: func_00D676E0
  and Plsel_BuildItemText (plsel), GmEm_Setup and QSync_OnPlayer (gm_sub),
  ArenaFeed_LoadPage, Html_ParseColor, Html_AttrAreaCoords, Layout_DrawImage
  and Layout_SpreadColumns (lobby), func_0080E490 and EmKushala_State3_0F
  (game); in round 8 Portal_PageReady, WeaponForge_OpenUpgrades (the box
  index in the first loop's counter), func_007CA930 and func_007C2750
  (lobby). Round 9: at level 4, a pointer local per loop (`c = l->unk01C;`
  before the loop, then `c[i]` or `*c++`) gives the loop an induction register
  starting at `base + offset`, and a second loop over the same array needs its
  own pointer variable (func_00D6AC30, plsel); loop 2's index needs its own
  s32 copy of the u16 index (func_007697D0, lobby).
- **One saved register for two values (a pointer that is NULL or a table,
  and a retry count).** Cause: the source reused one variable for both
  (`p++; } while ((s32)p < 6);`); two C variables get two registers. Fix:
  reuse the variable, with a comment. MWCC also shares one saved register
  between variables whose lives don't overlap; declaration order still
  decides which (permute with the initialisations moved into statements).
  Reuse forms that matched: a switch's result variable reused for a byte
  stored three times, `found` and `method` reused by a later loop, one s16
  local for `HasSkill ? 60 : 30` and a later call's result (v0/v1 of a
  `%`), a running total also used as the first loop's temporary. Seen in:
  func_007209A0, LobbyText_UpdateByte2 and Form_Submit (lobby),
  func_0070D360 and EmBlangonga_PickCarried (game); func_00702830
  (game, a count then a found flag) is still open.
- **Parameters in each other's saved registers.** Cause: three parameters kept
  across calls get s2, s1, s0 in parameter order, but a switch on one of them,
  or the order of independent statements, can swap them. A narrow parameter
  that MWCC masks or extends once at the entry (`andi s0, a0, 0xFF`) is a
  value MWCC creates: its copy takes the lowest register, like the created
  values above, below the other parameters; kept raw and converted at each
  use, it follows the parameter order (tested: `(u8 from, u8 *msg)` gives from
  s0 in one function and s1 in another, by how from is used; two round 7 notes
  disagreed on this). Fix: switch on a cast (`switch ((u8)kind)`), or move a
  store after another; change how the narrow parameter is used. Seen in:
  QSync_SendPlayer (gm_sub).
- **The original reads a field through a0 before `move s0, a0`; yours saves
  the parameter first.** Fix: make the field read the first initialised
  local (`s32 msg = m->msg;`); read inside a call's arguments, it comes
  after the save. A `volatile` read keeps the load on the argument
  register: `((volatile SndFile *)f)->headSize` reads through a0 before
  the copy to s0. The reverse, the first field read through the saved
  copy (`lbu a1, 3(s6)`) and later ones through a0: a plain statement after
  the initialised declarations (`n = D[o->type];`); as an initialiser it
  reads through a0. Seen in: five network menu states (sub_main
  unk_3D54B0.c), Sound_StageBank (main), StageObj_K1C_SetupChain (game).
- **A narrow parameter's extended copy in a saved register while the raw
  parameter is passed on.** Fix: `s16 ty = y;` as the first declaration.
  When the copy is extended after the first calls (s0 raw, s1 extended),
  `xs = x + 0;`: MWCC copy-propagates a plain `xs = x`. Seen in: the
  records' draw functions (lobby unk_6B1D70.c), TownGuide_DrawHeading,
  func_0070F0B0 and func_0070F580 (lobby).
- **An address, a product or a loop invariant kept in a saved register
  across calls in the original, recomputed in yours.** Cause: MWCC keeps
  such a value across calls only in a declared variable. Fix: a local for
  it: `ns = &D_5D46A0;`, `tbl = D_447750;`, `e = D_6ADB50;` at the top,
  `fl = &hdr[1];` before the loop, `k = n * sizeof(T);` used as
  `((T *)((u8 *)a + k))->f`, `s16 *bone = &e->bone;`,
  `d = (T *)&D_008F5A84;` before the first call. A loop's invariants
  (`x + 10`, `x + 0x3C`) are locals assigned before the loop, or MWCC
  hoists them itself when the loop uses the s16 parameter (`x + 0x14`) or
  `(s16)tx - K` of an int `tx`; with an int copy (`lx + K`) it doesn't.
  Seen in: Yn_Update (yn), QSync_QueueEmState and QuestNet_Recv (gm_sub),
  func_00709220 and func_0070CE90 (game), func_006C48E0 and the list
  draws ArmorForge_DrawMakeList and GroupBoard_DrawRooms (lobby).
- **A local loaded at a loop's top, or live from the function's entry.**
  Cause: a local declared inside a loop body is loaded at the loop top into
  a saved register; a variable set only inside a loop that may not run is
  live from the entry. Fix: move its declaration or assignment into or out
  of the loop. `end = (base = G) + K;` gives `base` the higher register.
  Seen in: unk_333D70.c (sub_main).
- **Saved registers that no declaration order fixes.** Try the order of
  the statements before a loop (`k = 0; g = 0; first = ...;`, found by the
  permuter), a variable reused for a later value (above), or optimization
  level 4: Ending_DrawStaffRoll (plsel) kept the same wrong registers in all
  120 orders at level 3 and matched at level 4. Seen in: func_002E4B60
  (sub_main), StageObj_K27_Draw (game: the permuter moved two
  declarations to the top, then three more were permuted).

- **A parameter's register decided by a copy of it.** Cause: MWCC numbers
  parameters before the locals, but a copy of a parameter in a local is
  numbered as a local. Fix: a typed copy (`u8 *buf = (u8 *)addr;` of an
  s32 parameter) or a same-type copy (`s32 file = file_in;`) moves it by
  its declaration position. The reverse: drop a copy the draft has (`s32 p
  = page;`) and use the parameter; the extended copy MWCC makes is numbered
  last and takes s0. Seen in: Patch_CopyFile and func_003D6C50 (sub_main),
  CardTask_SelectFile (sub_main, FAKEMATCH), CombShop_PageReady (lobby).
- **Saved registers rotated by the number of locals.** Cause: two locals
  whose lifetimes don't overlap are one local in the original, or the
  reverse. Fix: merge them, even across integer types, and try again after
  each merge (func_00381D90 matched only after an earlier merge;
  func_003A9FF0 went from 127 words to 58); or give a later loop or case
  its own local (Sprite_Queue's `q2`: 35 to 3). A local's integer type alone
  moves saved registers too (StatusPage_DrawStats: `s32 ty`). Dropping
  locals so a compiler-made walker is numbered first matched func_00D67C30
  (plsel; regalloc_predict.py shows the order). Check that merged
  lifetimes really don't overlap (Seg_ClipPlane's do).

- **A pointer in two saved registers in the original, one in yours.**
  Fix: an int parameter holding the address and a typed copy (`s32 data`,
  `SndFile *f = (SndFile *)data;`, sums written `(u8 *)(data + f->off)`);
  a pointer-typed parameter's copy is merged with it. An untyped `void *`
  parameter with its typed copy declared last is numbered as a local too.
  Seen in: Sound_LoadBankFile (main), MovieSfd_Create (sub_main).
- **A local numbered later in yours.** A declared local is numbered before
  a constant MWCC reuses across a switch (`pad = st->card[1]; switch
  (pad)`: LobbyCard_Select, lobby); a loop local reused for an earlier value
  gets an earlier web, which renumbers its loop.

- **A value in a lower saved register in the original, its own local in
  yours.** Cause: the original reuses one local for two values. MWCC numbers
  the reused local's second value after the values it makes itself (loop
  walkers, hoisted values), so it gets a lower saved register. Fix: merge
  two locals whose lifetimes don't overlap; check that by hand, because a
  sweep that merges locals blindly finds invalid merges too. Some merges help
  only together: Plsel_BuildRewardLines (plsel) matched only once case 2
  used both of case 1's `tbl` and `n1`, and each alone scored worse. Seen
  in: Offer_Generate, Npc_Init (lobby), Chara_SelectTask (select),
  Player_DrawWeapon (sub_main: `b` and `b2` merged, which removed a spill).

- **A loop's index and counter in each other's saved registers.** Cause: a
  variable reused in a later loop gets a second web, numbered with that
  variable, so which declared variable plays the index and which the counter
  decides the order (the initialisation order doesn't). Fix: swap the two
  variables' roles in the loop. regalloc_predict.py's "declared
  before" list says which value must be numbered before which. Seen in:
  Chat_DrawSpeakers (lobby: i and n swapped in both loops), ItemList_DrawPage
  (sub_main: num as the row's y, y as the dead counter).
- **regalloc_predict says a value must be numbered last.** Then it is a
  value MWCC creates, not a declared local: write the expression at each use
  and let MWCC make the common subexpression. Gfx_DrawRectFrame (sub_main)
  matched with `y + h` written twice. A u8 local is numbered like a created
  value; an s32 local reused in a later loop is not (Rate_UpdatePrices,
  lobby: the count and the index as s32 locals, 20 words to 4).
- **Saved registers still off after every single declaration move.** Fix:
  swap every pair of top-level declarations, keep each swap that lowers the
  score, and repeat. StatusPage_DrawStats (sub_main) went from 15 words to a
  match, WindowFrame_Draw from 310 to 246.
- **A loop's counter and walkers get the game's registers only with more
  values live across the loop.** func_00906210 (gm_sub): two more ints live
  across its 32-mesh loop give the game's registers (a test, not a fix).
  MWCC leaves a temporary with many neighbours to a later removal pass,
  which puts it first in the colouring. The real source keeps more values
  live there; which ones is open.

- **A loop's registers rotated while its twin loop matches.** Cause: the
  loop reuses a variable an earlier loop used. MWCC numbers that later use
  like a value it creates, so it takes registers first; a fresh local
  doesn't. Fix: a fresh local for the loop's counter. Seen in:
  Psp_ConnectTask (select, 959 words: a fresh `i` for the column in the two
  loops after a left move, not the reused `k`; 12 words to a match).
- **The game keeps a separate copy of a value that yours folds away.** Fix:
  a cast in the assignment: type the local differently and cast where it
  is assigned. ModelChunk5_TotalItems (sub_main) matched with a u32 `n`
  and `if ((s32)(n = ModelChunk5_Count(h)) == -1)`.

### Temporary registers swapped

Temporaries are coloured by the same allocator (see "How MWCC picks
saved registers" above): numbered in creation order after the locals, and
coloured from the highest number down, each taking the lowest free
register (v0 when free, v1, a0-a3, t0-t9; floats f0 or f1 up). So the
temporary made last in a region takes the lowest register, and a
temporary still live where another is made pushes it up.

- **v0 and v1 (or a0-a3) swapped around a switch.** Cause: MWCC reuses the
  constants its compare chain left in registers: as a call's argument in a
  case (`f(2)` takes the chain's 2), as the return value (`return 2` after
  the switch), in the next chain, in a switch inside a case (its compares
  use the outer chain's constants), as a variable's new value (`mode = 2;`
  takes the saved register the chain loaded with 2), as a stored value
  (`D_008BFAB8[0x39] = 1;`); `s16 v; ... case 1: v = 1;` extends the reused
  constant, and a constant the chain loaded stays in its register across
  calls to a static callee (`*(u8 *)0x6ADCB1 = 2;` from the chain's v1).
  Fix: write the natural C (`func_0082D8E0(em, 1)` after `unk390 == 1`).
  To stop the reuse in the default body (the original loads constants and
  argument registers again), give the default a case label: `case 0:
  default:` (on an inner switch it also stops a reload of the field the
  outer switch loaded). Seen in: func_006C5270, func_006EF6F0,
  ArmorForge_MakeInput, LobbyLogin_SendSummary and func_007C9D80 (lobby),
  Player_StartIdleMotion (sub_main), Gs_SetState and Gs_SetupDisplay (main),
  EmDiablos_State4_0E, func_0080E9B0, GreatSword_SetChargeAttack,
  Player_State0_15 and EmKushala_Update (game).
- **Every temporary one register higher (v1 for v0) in a void function, or
  the reverse; v1 where yours has v0 before a tail call.** Cause: MWCC
  treats v0 as live to the end of a void function unless a call clobbers it,
  and a call to a static function that it knows leaves v0 alone doesn't; it
  also builds a tail call's address in v1 when the callee is static. Fix:
  make the same-file callees static, or not, to match (see "Arguments kept
  or reloaded across calls"); this includes an empty function called from
  a switch's default (static, the chain keeps v1 throughout; global, MWCC
  moves to v0 after the first compare). Seen in: ModeSel_Start (select),
  func_003BD8C0 (sub_main), func_006C3440, EmChameleos_MotionEffects,
  func_00848FE0, func_0083FB80 and Bow_UpdateAim (game), Plsel_DebugMenu
  (plsel: its empty callee func_00D606D0 made static).
- **The result in a parameter's register.** Fix: reuse the parameter:
  `arg1 = f(...);`, `x = abs(x);`, `cur = j; return cur;`,
  `if (flag == 0xFF) flag = 3; else flag = 1;` (a ternary into the
  parameter or a local gets another register), `d = room - least;
  room = d;` (`room -= least` moves it); `return op->blocks = ...;` frees
  v0; a converted value assigned back to the parameter stays in its
  register (`x = (f32)x;`, a float-to-u16 result). An s16 result extended
  into the variable's register and then copied to a0 is a callee returning
  s16 whose result is stored in the s16 parameter. Seen in: EmScript_Call
  (game), func_006C6740, Layout_SpreadColumns, LockOn_SetSector and
  func_00711C90 (lobby), Plsel_AddChatLog (plsel), func_0036C660
  (sub_main).
- **A value kept in an unused parameter's register (a0 live through every
  case, `sll a0, a2, 1` at the top), where yours uses a t-register and
  recomputes it after joins.** Cause: the source reused the dead parameter
  as a variable. Fix: `static void f(s32 j, EmWork *work, s32 i) { j = i *
  2; ...`, with every s16 array read at a byte offset (`*(s16 *)((u8 *)base
  + j)`, a FIN_S16 macro) so that j stays one live value; a dead first
  parameter as the loop pointer (`u16 f(EmBPick *p, u16 r)`, the callers
  cast em). Seen in: EmPlesioth_UpdateFinWave and
  EmBlangonga_PickCarried (game).
- **The temporaries or float registers of a function in another order.**
  Cause: they follow the declaration order too: the last declared local gets
  the lowest free register (`f32 ax, ay, by, bx` gives bx f3 up to ax f6),
  and declarations inside blocks count. Fix: permute the declarations; read
  a loop's bound into a local before the loop (`n = s->n;`) instead of the
  field in the test. For one block of a big function (or two copies of one
  block with different temporaries), declare its locals inside a new `{ }`
  block, per case, and permute each block's order alone. Seen in:
  func_0090A9D0 (gm_sub), func_00322A50 (sub_main), LobbyBgm_NearestEntry
  and func_00763070 (lobby), the monsters' turn-toward-target blocks
  (game).
- **A value stays in its first register where yours coalesces the copy (or
  the reverse).** Cause: MWCC doesn't coalesce a copy into a variable of
  another type. Fix: add or remove a typed copy (`u32 n = r;`,
  `QNetHdr *h = (QNetHdr *)msg;`, `s32 i = from;`, `p = (u8 *)text;` for a
  `char[]` buffer). A call's result that stays live in v0 after a join
  and is copied to a saved register in both arms: assign it in each arm
  (`if (c) { gs = buf = f(); ... } else { gs = buf = g(); ... }`) and use
  `buf` where v0 is read (a ternary sinks the copy). Seen in:
  QuestNet_ProcessQueue, QuestNet_OnSysMsg (gm_sub), LobbyText_UpdatePart
  (lobby), Tex_Unlock (main).
- **A switch value in a2 where yours has a1.** Cause: the dispatcher passes
  its second parameter on untouched (`(em, work)`), which keeps a1 live; a
  register left alone through a compare chain is an argument of the case's
  callee. A chain whose last constants go into a2 means those cases'
  callees take `(em, work)`; a constant in a2 set per case in the `jal`'s
  slot is a third parameter of a state function shared by several states
  (`(Em *em, EmWork *work, s32 kind)`). Fix: take the parameter and pass it
  on (`F(em, work)`, `f(t)`); m2c drops it. Seen in: the families' state
  dispatchers (game: all are C since round 7), the lobby's task
  dispatchers.
- **Two registers swapped where the code is otherwise right.** Fix: vary one
  cast or operand at a time (an index, a modulo, the stored value, operand
  order, `|=`): `supplyTaken[(u16)id / 32]` instead of `id / 32` put a0 and
  a1 back in place; `(x & K) != 0` for a bare `x & K`, `x = x - K` for
  `x -= K`; a value computed in two statements (`d = 0x10000 - h;
  d = (d - em->unkA0) & 0xFFFF;`); a no-op cast (`(u32)h->start` on a u32
  field) changes the load order. Round 8: `m->list[(u32)cur]` on one of
  two accesses (or a `static inline` accessor with a u32 index: probably
  the game's macro), `e.id = (u16)id;` storing an int into an s16 field,
  a u16 built in steps (`n = hdr[4] << 8; n += hdr[5]; n -= used;`), the
  operand order of an addition of two globals. Seen in: QSync_OnSupply,
  QSync_OnState (gm_sub), Quest_DrawDetails, WeaponForge_MakeInput,
  func_007B3DC0 and LobbySrvReq_6105 (lobby), func_00787EA0 (game),
  LinearHeap_Init and Gs_ClearFrame (main).
- **`movz` where the original has `movn`, with the constants swapped.**
  Cause: the ternary's form decides: `c ? 2 : 1` gives `li 2` and `movz`,
  `c == 0 ? 1 : 2` gives `li 1` and `movn`, and the operands keep their
  source order. Fix: flip the condition and the operands; when the
  registers still differ, an if/else instead of the ternary, or the
  target's type (`s32 color = c != 0 ? A : B;` matched where a u32 didn't).
  Early returns of constants are if-converted too (`if (c == 0) return K;
  return 0;` gives a `movz`). Seen in: EmBulldrome_State1_01 and
  func_00798240 (game), Plsel_SelectTask (plsel), Tim2_Align (main),
  func_0076CAA0, GroupBoard_DrawResults and func_00773D40 (lobby). Round 9: in
  a large function the ternary that gives the original's `li 3; li 9; movn`
  (`last = D_006ADCF1 != 0 ? 9 : 3`) rotated every saved register and added 4
  words, where the if/else form keeps them: weigh the whole function before
  keeping a ternary (func_003437A0, sub_main, still open). `x = c ? a : b`
  puts the then-value in the default register: flip the condition to swap them
  (func_00345DB0, sub_main). An s16 `if (c) d = 4; else d = 10;` gave the
  original's `movz` and extension where the ternary put d straight into a
  saved register (PointShop_Draw, lobby). `(k & 2) ? 0x8000 : 0` gives `li a0,
  0x8000; movz a0, zero, v0` where the original has the condition in a0 and
  the value in v0; no form found yet (func_007120F0, lobby).
- **A repeated constant or an extended zero in another register.** Cause: a
  chained assignment stores from its last target back
  (`d->unk157 = d->unk156 = x;` stores +0x156 first), and its order decides
  which register gets the value and which get copies. Fix: reorder the chain
  (`buf[0x11F] = buf[0x11E] = buf[0x11D] = 6`, `a = b = c = 0`;
  `s.h = s.w = size;` stores w first; `scale[0] = scale[1] = scale[2] = x`
  stores [2] first; `al = b = 0xFF;` for two register copies). A constant
  shared by both paths of an if and a store after it (`ori a3` in each
  path, `sw a3` at the join) is assigned in both paths: `d[0].color = c =
  0xFFFFFF;` in one, `... | (c = 0xFFFFFF)` in the other, then `d[1].color
  = c;`. Seen in: func_0079ECF0, func_006FA980 and Layout_CellColSpan
  (lobby), Psp_DrawActor (select), EmFxTri_Draw (game). func_007D1150
  (lobby) used this form until round 16 matched it with an Rgba-style
  inline (Addresses).
- **The caller passes a copy in a0 where the original passes the variable
  itself.** Cause: the callee's parameter was s16, not an int with a cast
  inside. Fix: declare the parameter s16, in the callee and in its
  callers' prototypes (the draw helpers Icon_Draw and func_2E66F0 take s16
  coordinates). Callees extend their s16 parameters, so a raw pass means
  an s16 parameter. Seen in: Parts_DrawSwatches and its caller
  func_006BD050 (select), the lobby screens' draws and func_00792740's
  fourth parameter (lobby), `Player_AddSharpness(void *, s16)` in
  Player_State0_61 (game).
- **A constant kept in an argument register for no visible reason (`li
  a0, 1; bne v0, a0`, `li a2, 1` before a two-argument call).** Cause: a
  call takes it as an argument: another branch's call (its second
  argument), or the call itself has a parameter its prototype lacks
  (`Quest_UpdateTimers(1)` declared `(void)`; `func_332780(lines, item,
  1)`). An argument register set in a branch's delay slot before a call
  that takes nothing is the other branch's argument, scheduled into the
  slot. Fix: read the callee and the other branches first. Seen in: round 1,
  CombShop_BuildList and SrvMenu_Run (lobby), Load_Idle (main).

- **Every temporary one register up.** Cause: the function returns a value
  and the draft is `void`, so v0 is free in yours and busy in the
  original. Fix: return the value. Seen in: NetReq_TakeFixed (lobby,
  returns s32 1).

- **A load first in a temporary in the original, folded into its use in
  yours.** Fix: hold it in a local of another type than the value (`s32 rgb;
  rgb = s->rgb;` for a u32 field): MWCC doesn't forward-substitute it.
  Seen in: Sprite_Queue (sub_main).

- **Two temporaries swapped, and no declaration order fixes it.** MWCC
  colours temporaries in the reverse of their numbering: the lowest-numbered
  value is coloured last. A fresh variable, or a dead one reused, moves a
  value in that order (LobbyWorld_NextFadeKey, lobby: the counter in its own variable
  and a pointer held in a dead parameter).

- **Every temporary rotated, every mnemonic right.** Cause: one expression
  where the game's source has two statements; the split changes the
  temporaries' numbering. Font_Init (sub_main) matched with `x = a + 0x3F;
  x &= ~0x3F;`, where the one-expression form scored 104 words.

- **Temporaries in reverse declaration order.** Values that never reach a
  saved register take t registers in declaration order, and when they are
  all live together the earlier-declared one gets the higher register.
  Fix: reorder the declarations by the game's t registers, highest first.
  Seen in: Layout_MeasureColumns (lobby, nmlobby-h1).
- **A long-lived value in a low temporary in the game (v1), a high one in
  yours: the variable's later web.** MWCC numbers a variable's first web
  (its first live range) as declared, and its later webs like values it
  creates, which it colours first. Fix: give the variable an earlier,
  unrelated use, so that the use you need becomes a later web. Seen in:
  Em_TurnToward (game: `cur` holds Math_Heading's result first, then
  `cur = e->heading` lands in v1; 93 words to 52). adiff scored the first
  step worse (92 to 103) and func_diff better: score such sweeps with
  func_diff. The same holds for counts and loop bounds: func_00707060
  (game) matched when the remote branch's count and the main loop's bound
  were later webs of one variable, and the other count and bound of
  another (a temporary has to be coloured before a hoisted invariant).
- **The same value masked again in each switch case, in another register in
  each case.** With the parameter itself, each case's mask is a created
  value and takes a0 where the game has a1; one copy assigned in the tests
  fixes only the first case (its first web). One block-local copy per case
  (`s32 k = r;`) matched func_00703D60 (game), tagged FAKEMATCH: nothing
  showed the original had those copies. A macro for the cases that copies
  its argument once, `EM_AGGRO_ADD(cond, rank, row) { s32 k = (rank); if
  (cond) ... }` with the conditions on the parameter, matches too and is
  natural (round 24).

### An instruction or constant load in another place

- **A load of a global above a store in the original, below it in yours (or
  the reverse).** Cause: MWCC moves a load above a store only when it knows
  the two don't alias: two named objects can be reordered, an absolute
  address (`*(T *)0x5BB0AE`) or a cast pointer may alias every global, and
  the fields of one struct keep their source order. Two absolute stores
  keep their order; stores to two different named objects are reordered
  even when they overlap. A named global's load also moves above a store to
  the stack; loads through pointers still wait for named stores (tested).
  `extern T x : 0xADDR;` compiles but aliases like an absolute address.
  Fix: named objects (an extern, a member of a struct global such as
  `D_006ADCB0`, a named array cast, `gPadState[0].held`, a per-file view
  struct of a global such as sub_main's gQuest) to let the load move up;
  fields of one struct to keep the order; otherwise write the load first.
  func_diff and the build accept a raw address where the original names a
  symbol (the bytes are the same), so only the schedule tells them apart;
  a sub_main variable without a symbol yet can be reached as a named object
  through its neighbour (`gRandGaussSum[2]` for 0x6AE398, `gSave.pad2728[0]`
  for the save's clock bytes, `((u8 *)&gChatRecipients)[0x14]`), or named
  in the module's own symbol file. A neighbour-relative access can still
  compile differently: Sound_PlaySePos (main) matched only with `extern
  SndVec gCameraPos;` (through `((SndVec *)D_6A9DF0)[-1]` MWCC kept the
  address in `at`). Named stores also let loads of address-taken stack
  locals move above them. The same fix spreads the constants of a run of
  stores (with an absolute-address load first, MWCC hoisted every constant
  into its own register; named, it loads them one at a time before each
  store). Seen in: QNetIf_OnPlayerLeft (gm_sub), func_00701990 (lobby,
  `gLobbyNet.dlBuf`), Plsel_StateChoose and Plsel_DrawArrow (plsel),
  func_00823010 and func_00844850 (game), LobbyWorld_Skip, func_007B9B10
  and func_007C2C80 (lobby), Chara_DrawEquipment (select, 22 to 14 words);
  func_002EC7C0, func_002EC950 and func_002ECCC0 (sub_main) wait for names
  of main's render-state globals.
- **A global reloaded after every store or loop iteration in the original,
  loaded once in yours.** Cause: a plain `extern` scalar is assumed
  unchanged by stores through pointers; a struct member isn't. Fix: read it
  as a struct member (`D_006ADCB0.playerCount`, `((T *)0x5BAC70)->cam`), or
  through `volatile` for a reload right after its store
  (`*(volatile s32 *)&g`, `c = *(volatile u32 *)&D_002190F0;`,
  `(*(volatile u8 *)&em->step)++` where the cases would reuse the value
  the outer switch loaded). A global pointer is reloaded after every store
  through it (`gW->a = 1; gW->b = 2;` reloads gW; tested); a function that
  loads it once per group of stores uses an element pointer. For the
  reverse, a bound in a u16 extern reloaded every iteration, read it into
  a local first (`cnt = D_008EA8FA;`); a state byte kept in a register
  across a switch is a local (`s8 state = m[0x45]; ... m[0x45] = state +
  1;`), reloaded it is `m[0x45]++` on an `s8 *m`. Seen in: unk_7077A0.c
  and GreatSword_State19 (game), cse_spumap.c, dma.c, Gs_ShowFrame and
  Gs_SetState (main), func_006C4A00 (select), func_007BFAE0 (lobby). A
  float computed into a local before stores through `o->unk038` makes each
  store reload the pointer (func_006EFDE0, game).
- **A value reloaded after a call in the original, kept in a saved register
  in yours (or the reverse).** Cause: an expression around the call lets
  MWCC reuse a load made before it. Fix: make the call a statement of its
  own (`x = f(tbl[1]); g(x, tbl[1]);`, `k = f(i); a[k] += g[i];`,
  `v = f(); if (v < g[i])`), or nest it for the reverse. A call nested in
  another call's arguments makes MWCC load the plain global arguments
  first (into saved registers); a local for the inner result gives the
  reload after it. Written inline, `(u16)(f() + s->half - s->dir)` loads
  `s->half` before the call; `a = f();` first doesn't. Seen in:
  Chara_DrawQuestion (select), Plsel_CountHunts (plsel), func_0075F820
  (lobby), func_00701140 (game).
- **A value loaded by a loop's test reused in the body in yours, loaded
  again in the original (or the reverse).** Fix: temporaries
  (`dx = ...; dz = ...;`) let MWCC reuse it; reading it twice in the body
  stops that. Seen in: gather.c (sub_main), Text_DisplayLength (sub_main).
- **A store, then a reload of the same word (`sw; lw`).** Cause: two
  statements: `p->a = v; x = p->a;` (`x = p->a = v;` doesn't reload). Likewise
  `x--; if (x <= 0)` gives `sw; lw; bgtz`, where `if (--x <= 0)` tests the
  register and puts the store in the delay slot; `x = ++x % 256` stores before
  the modulo; `x = x * 1` keeps a load and a store; `*out = f(); if (*out !=
  0)` reloads. A global stored and read at once reloads when it is a member of
  a struct global (`D_008BDB20.set = set; f(D_008BDB20.set);`); a plain extern
  passes the register on. For the reverse, test the assignment: `if
  ((em->unk89C &= 1) == 0)` keeps the masked value. A global decremented
  and read again in an else (`sh v0, g; ... bgtz; ... lh v0, g`) is `if
  (--g <= 0) {...} else if (g < K)`; m2c's `t = g - 1; g = t; if (t <= 0)`
  reuses the register. A field decremented as s32 and tested by its low
  byte through a pointer: `p->f--; t = &p->f; if (*(s8 *)t <= 0) { *t =
  30; ...`. Seen in: QSync_OnPlayer (gm_sub), func_006BD050 (select),
  LobbyActor_StartScript, func_007D77A0 and func_007C2FD0 (lobby), the
  families' Init functions (game), unk_308BC0.c (sub_main). Round 9: `--x ==
  0` in the game (the counter stored, then loaded again) is `x--;` then `if (x
  == 0 ...)` (Plsel_ResultsUpdate, plsel); a value stored twice in a row and
  reloaded between is `*d++ = c[i]; *p++ = c[i];`, where `*p++ = *d++ = c[i]`
  loads once (func_002E65C0, sub_main).
- **Independent loads or stores in another order.** Cause: MWCC keeps the
  source order of independent statements, and of initialised declarations
  (`s8 i = 0;` before a pointer set to an array puts the zero first); of the
  loads before a branch, the one moved into the delay slot comes last. Fix:
  order the statements as the original schedules them, including those
  before and after a loop (`cur = (u8)sel; i = 0; src = T1;` against
  `for (i = 0, act = T1, ...)`; `for (p = T, i = 0; ...)` puts the
  `lui`/`addiu` before the `move`, `for (i = 0, p = T; ...)` after it;
  `cell = &s->cells[s->curCell]; s->paragraph = 0;` schedules the store
  after the index computation). A value computed early but declared late
  keeps its register (`need` computed before a call, declared after `i`).
  The statements that compute locals count too, not only the stores: `u =
  ...; spr.u0 = u + 1; v = ...; spr.v0 = v + 1;` gives the game's pairwise
  schedule where u and v computed together leave MWCC a free order. MWCC
  still moves constant stores: for a run of stores to named globals, try
  every order (120 for five) and read the relocations with func_diff. When
  one statement is out of place, move it through every position, or
  hill-climb the order of the simple stores. Seen in: Parts_DrawTypes,
  Psp_MarkIcons, ModeSel_SetLayout, ModeSel_DrawButtons and
  Chara_DrawEquipIcon (select), func_003D6320 (sub_main), Heap_Alloc
  (main), Html_TagTableEnd, func_007B0050, func_007D6F30, VPart_Draw and
  ArmorForge_DrawMake (lobby), Player_State3_00 and func_00796FA0 (game).
- **A load that no statement order moves before a store or a call.** Fix:
  read it into a local first (`t = *(s8 *)0x5D87A0;` before the stores,
  `s32 vram = t->vram;` before a call), or set a pointer local before the
  other loads (`u8 *s = D_008EA908;`, `Em *m = &D_5EC580[o->idx];` as the
  first initialised declaration); `d = o->data;` before an `if` lets
  the load fill the branch's delay slot, and a pointer set after the calls
  stays out of the saved registers. A call's argument read before an
  absolute store (`t = G; *(u8 *)X = 4; f(t);`), and a read written before
  stores through a pointer that may alias it, stay above them. Other
  forms: the first initialised local read before a memset (`s32 cur =
  m->cursor;`), `em = s->owner;` as the first statement inside the `if`,
  table addresses as pointer locals assigned at the case's start. Seen in:
  Plsel_DrawSaveRecordMenu (plsel), func_0079FF20, func_007667F0,
  func_007CA060, Html_TagTdEnd, ArmorForge_DrawMake and VFx_Run (lobby),
  ScreenTex_Send (main), func_006C3910 and the ObjAttach creators (game).
- **Per-kind table loads up front in the original, each moved down to its
  use in yours.** Fix: read the field in every initialiser
  (`p1 = tblA[em->type]; p2 = tblB[em->type];`) instead of a `u8 type`
  local. Seen in: the second batch's game files (unk_705AC0.c to
  unk_708850.c).
- **A constant in a `jal`'s delay slot where yours has it in the next `b`'s
  (2 words longer, or the reverse).** Cause: a constant assigned before a
  call goes into the call's slot; assigned after it, into the next branch's.
  The same for other assignments (`last = id; f(...);` puts the `andi` of
  `last` in the slot). Fix: move the assignment across the call. Seen in:
  unk_39C290.c (sub_main), unk_6D5330.c (lobby).
- **A store or a pointer step in a `jal`'s delay slot in the original,
  earlier in yours.** Fix: make the callee a static leaf defined earlier, or
  read the arguments into locals, step the pointer, then call. Seen in:
  QuestNet_Update calling QuestNet_ResetTotals (gm_sub), func_00D63310
  (plsel).
- **A loop invariant in other registers, or in a block after the loop.**
  Cause: MWCC hoists invariants itself, into other registers than locals set
  before the loop get; an invariant written inside a loop (`~(align - 1)`)
  becomes a block at the end of the function that jumps back;
  `if (0x200000 < x)` keeps the constant in `at` inside the loop where
  `x >= 0x200001` hoists it; an invariant assigned to a variable inside the
  loop (`bx = x + 0x73;`) puts the loop's preheader after the epilogue,
  assigned before the loop it stays inline. A bound written in the test
  (`n <= (s16)(first + 10)`) is hoisted after the loop's pointer set-up,
  a local set before the loop before it. A loop-invariant store that the
  original repeats in the loop needs a `volatile` pointer. A value that
  MWCC would fold into the loop's use stays in place in a variable of its
  own (`x2 = x + 0x134;` after the call, where `x += 0x134` is folded).
  Fix: write the expression where the original computes it. Seen in:
  func_007069E0 (game), VramSlot_FindGap and ColorEdit_Draw (main),
  func_006EFE80 (lobby), Key_LerpVec2 (sub_main), Gallery_DrawEntryPage
  (select). EmScript_Op0C and EmScript_Op0D (game) are still open: the game
  hoists `&em->unk438` among the loop's constants (in the reverse case
  order of the `bne` constants), a pointer set before the loop comes first.
- **A store or an argument moved after an expression was rewritten.** Cause:
  the form changes the schedule: a ternary instead of an `if` (the 16-byte
  rounding `ALIGN16`), `p = G; p += i;` (the pointer loaded first) against
  `G[i]` (the index first), `seq = f(); return seq;` against `return f();`,
  `r = f(); r = g(r);` against `g(f())`, nested `if`s against `&&` with an
  assignment, `t = c / s; w = hw * t;` before other statements. Fix: try
  both spellings. Seen in: ave_wrap.c, func_003D6190, func_00320A10 and
  Frustum_Build (sub_main), Chara_LoadSlot (select, `p = base;
  p += i * 0x37F8;`).
- **Statements scheduled as if a check were still there: loads, registers
  or a whole draw's last calls off by the schedule, with nothing missing.**
  Cause: an empty `if` with a compound condition is a scheduling barrier:
  its short-circuit blocks are removed late, so it splits the block and
  changes the load order and registers around it; a single-condition empty
  `if` is removed too early to do that. Any compound test works (`if (a &&
  a) {}` too). Fix: an empty compound `if` where the schedule changes,
  written as the check the game probably compiled out: `if (a >= 10 || b >
  1) {}`, `if (kind <= 0 || kind >= 0x60) {}`, `if (align == 0 || (align &
  (align - 1)) != 0) {}` (a power-of-two check), `if (j < 0 || j >= 0x20)
  {}` in a loop. The permuter finds it in minutes, as `if ((s && s) && s)
  {}`: rewrite that as such a check. When a draw function is off by the
  schedule of its last calls or of a loop body, try one before them. Where
  it goes matters: in a loop's break block it keeps the game's `bnez body;
  b exit` (NameList_DropFirst: `if (n == NULL) { if (list == NULL ||
  list->next == NULL) {} break; }`); as a switch's `default:` body it keeps
  a compare chain's delay slots empty (LobbyStep_DownloadQuests); between a
  read and its use it moves the read. One-test empty ifs do nothing. Mark
  each with `/* compiled-out check */`, and write a condition the code
  plausibly checked: the lead's rulings (CONTRIBUTING.md, "Fakematches") say which pass.
  Seen in: Plsel_LogChatFrom and func_00D61400 (plsel), Rate_ListAdd,
  func_0018D770 and Packet_Alloc (main), func_0075A2C0, CombShop_DrawList,
  Npc_Draw, ServerList_Sort, MoveMenu_EntryInput and Rate_UpdatePrices
  (lobby), Light_InitAll, CamMode1_Update and Key3_EvalByTimer (sub_main).
- **A value recomputed after a call in the original, reused in yours (or the
  reverse).** Fix: an operation that stops the common subexpression
  (`switch ((s32)hr + 1)`); for the reverse, a local that holds the value
  (`modId = m->id`). Seen in: unk_3B4C80.c (sub_main), Iop_Init (main).
- **An address, an offset or a mask computed where no plain C puts it (a
  field's address in a branch's delay slot for the store alone, an offset
  kept out of a relocation, a mask after a shift).** Cause: a
  `static inline` helper in the source; MWCC inlines it but schedules its
  parameter like a separate value. Fix: `static inline void
  Html_MaxU16(u16 *p, s32 v) { if (*p < v) *p = v; }` called as
  `Html_MaxU16(&s->unk182, n)`; `static inline Request *Save_Request(u8
  *save, s32 idx) { return (Request *)(save + idx * 0x14 + 0x299C); }`
  called with `(u8 *)&gSave` keeps 0x299C out of gSave's relocation; a
  one-line helper for `jis & 0xFF`; `static inline HeapBlock
  *Heap_Blocks(Heap *h) { return h->blocks; }` for a first read. A call's
  arguments evaluated before calls that come first in the source are a
  helper taking those arguments. A permuter result with an identity
  function around an argument points here. At level 4 a compound
  assignment (`D_00895258->y += ...`) also computes the field's address
  once for the store, which looks like a helper. Seen in: Html_TagEndTr,
  Html_TagEndTable, OfferMenu_AddRequest, Portal_PageReady
  (`Page_ClampScroll`) and Page_PointTo (lobby), Font_GlyphIndex
  (sub_main), Heap_Unlink and Sound_SetSe (main).

- **A load the game makes before a branch, yours after it.** Cause: MWCC
  copy-propagates a local that only copies a field of the same type
  (`s16 y = p->y;`) to its uses, so the load lands after an intervening
  branch (here a float-to-u32 conversion's). Fix: a local of another type
  (`s32 y = p->y;`) keeps the load where it is assigned. Seen in:
  Font_AddGlyphSprite (sub_main).
- **A late load and an alignment nop in the game, both missing in yours
  (a size mismatch of one instruction or two).** Fix: subtractions as
  statements of their own (`t -= k0; k1 -= k0;`) instead of inside the
  division. Seen in: Color_EvalKeys (sub_main, 218 words to 37).
- **Bytes assembled with ORs, loaded in another order.** Cause: MWCC loads
  the costliest operands first, and an unmasked byte is cheap. Fix: mask
  every byte, the plain one too: `(b0 & 0xFF) | ((b1 << 8) & 0xFF00) |
  ...`. Seen in: LobbyNet_GetPartyAddr (lobby, with Swap16's one-parameter
  prototype).
- **A loop's hoisted invariants in other temporaries.** Cause: a local
  computed before the loop and used in it changes the order in which MWCC
  creates the loop's invariants. Fix: write the expression in the loop,
  where a twin function has it. Seen in: BoxMenu_DrawEquipGrid (lobby:
  `(s16)(117.0f + fx - 116.0f + 25.0f * (i % 10)) + 1` as in
  BoxMenu_DrawItemList, and `x0 = 117.0f + fx` moved to just before the
  second loop).

- **An address the original computes in a loop's preheader, among the
  compare constants, and yours before the loop.** Cause: MWCC hoists an
  assignment out of conditional code only when every path to the loop has
  it. Fix: assign the pointer inside the conditional block of each case
  that uses it (`w = &em->work;` inside the type check of both case 6 and
  case 7), not once before. Seen in: EmScript_Op0C and EmScript_Op0D
  (game).

- **An address loaded before another load in the original, after it in
  yours.** Fix: a chained store (`gScene.area = *P54 = *P50 = v;`). Seen
  in: LobbyEntry_Login (lobby).
- **A search loop's first test with v0 taken.** Fix: read the not-found
  result before the loops (`def = D_007E7AD0[0];`) and return it at the
  end. Seen in: Kitchen_FindIngredient (lobby; the permuter found it from a
  deliberately worse starting state).

- **A constant argument scheduled one slot early in yours, next to the
  load of a pointer-sized global.** Cause: the global declared as a pointer
  (`extern char *D_x;`) and read `(s32)D_x`. Fix: declare it as an array
  and read the word through a cast (`extern u8 D_x[];`, `*(s32 *)D_x`),
  as the file does for its neighbours. Seen in: ArenaCup_DrawRecord
  (lobby).
- **The parts of a packed GS word computed in another order.** Fix: write
  the parts in the order the game computes them: a GS_XYZ word whose y
  comes first is `(u64)y << 16 | (u64)x | (u64)z << 32`. Seen in:
  Prim_DrawSpriteF (sub_main: 12 words to 2; the `or` itself still takes
  y first where the game's takes x).

- **Stores in another order, the code otherwise right.** Fix: move one
  statement at a time within its run of simple assignments, keep each move
  that lowers the score, and repeat until none does. Float constants are
  loaded in source order, so first put each case's constant assignments in
  the game's order. Seen in: Gs_SetupDisplay (main), Player_DrawWeaponPart
  (sub_main: 410 words to 233).
- **A copy of a matched function, written with raw addresses, scheduled
  differently.** Cause: `*(T *)0xADDR` stores may alias the loads around
  them. Fix: write it with the named fields the matched function uses.
  Seen in: Galex_SetupQuest (plsel, a copy of sub_main's Quest_Init tail).

- **A switch on a field, with the game's loads of that field in other places
  than yours.** Fix: read the field into a local once, for the switch and
  the tests, and keep a second read where the game loads it again.
  StageObj_K0F_Draw (game, 473 words) matched with `u32 type = o->type;` and
  the one re-read in case 1/6 (the permuter's idea; 67 words to a match).

### Arguments loaded in another order

- **a1 loaded before a0, or the third argument before the second.** Cause:
  an argument that needs a conversion for its parameter (an extension, a
  mask, a pointer passed to an int parameter or the reverse) is evaluated at
  another point. Fix: give the callee its real parameter types in that file:
  pointers for pointer arguments (`(Em *em, EmWork *path)`, not `s32` with
  `(s32)` casts), the real widths (u32, s16, u8); to compute an address
  first, a `u32` parameter and `(u32)&m`. The order of int and float
  parameters in the prototype, and a mix of s16 and s32 ones
  (`(void *, s16, s16, s16)` against `(void *, s16, s32, s16)`), set the
  order too; a `u32` size parameter is evaluated first. MWCC evaluates
  arguments in parameter order, so the load order shows the parameter
  order: a float parameter before a pointer (`(pos, k, f32 scale, void
  *owner)`), a colour after the floats (a0, f12, a1 for `(u8 kind, f32 x,
  ..., s32 color)`). Other forms: `(Player *, s16 id)` where the callee
  compares `(s16)id`; the byte read itself (`f(w[7], w[0x14])`), not a u16
  narrowed by the prototype; a first argument cast from a local of another
  pointer type reorders the set-up (`DmaTag_End((u128 *)p, 3, irq, 0)`).
  Seen in: Em_StartTurnToTarget's callers, StageObj_K0E_RunParts,
  func_006ED8A0 and func_00796FA0 (game), Prim_DrawAll and Bgm_EnterArea
  (sub_main), func_006C4CB0, LobbyActor_RunScript, Player_MoveToSpawn and
  Page_DrawWidget (lobby), Dma_BuildEndPacket (main).
- **An expression argument evaluated before plain loads of the others.**
  Cause: an int expression comes first. Fix: `f(v, &g, p[1], p[2] + 0)`.
  For the reverse, a call's result that the original computes before the
  other arguments goes into a local first (`key = f(...);
  Draw_ImageMap(..., key)`), and a load the original makes first can be a
  value it also keeps in a local (`i = p->unk83E;`, the loop counter;
  `n = p->unk83E; f(em, m->item, n, 0)` right before the call). An int
  argument computed before the float ones goes into a local first (`c =
  (fade * 255 / 16) << 24; Draw_RectClipped(0, 0, 640, 448, c)`); a
  product computed in two steps (`fee = price; fee *= m->quantity;`) is
  moved into a1 in the `jal`'s slot, where inside the arguments it is
  computed straight into a1. A table pointer loaded straight into the
  argument register is `src = T[i]; src += off; f(p, src);` (`f(p, T[i] +
  off)` loads it into v0). Seen in: Plsel_CheckSizeAwards (plsel),
  Layout_DrawImage and Deco_DrawMake (lobby), Player_State0_53,
  Player_State0_55 and func_006F2360 (game).
- **An address argument computed first, or last in the call's delay slot.**
  Cause: an int base (`(T *)(arg0 + 0x30)`) is computed before the other
  arguments and a pointer base (`o + 0x30` with `u8 *o`) last; `(s32)m->ptr`
  to an `s32` parameter loads before a later `u8` argument, to a `u8 *`
  parameter after it; `(s32)&D_...` to an int parameter adds the `%lo` one
  slot earlier than to a pointer parameter; `(s32)m` to an int parameter
  in a loop made MWCC hoist the address into a saved register, a pointer
  parameter didn't. Fix: switch the base, or the parameter, between int
  and pointer (`(char *)(u32)D_00895258->field1` puts a string argument
  first; `(s32)h + 4` in one case of a switch where the others use `h +
  4`). Seen in: func_006CAE40, Em_StartTurnAway's callers, func_0085DDC0
  and func_0070BCE0 (game), SaveMenu_LoadCharacter (sub_main),
  Portal_HandleData and Link_Follow (lobby).

- **A float argument set up last in yours and first in the original.**
  Cause: under the EABI a float argument goes to f12 wherever it stands in
  the call, but MWCC sets up the arguments in parameter order. Fix: give
  the float parameter its real place in the prototype and the calls
  (func_00395D80 is `(void *obj, f32 frame, s32 a, s16 kind)`; its 254
  calls were reordered). Seen in: Player_MotionEffects (sub_main).

- **An int argument's `li` in the jal's delay slot in the original,
  earlier in yours.** Cause: MWCC sets up arguments in source order, ints
  and floats alike, and the original's int parameter is the last one. Fix:
  the callee's real parameter order (Fx_SpawnSplash is `(pos, y, scale,
  k)`). Seen in: Lure_Update (sub_main).

- **Arguments evaluated in another order, and the temporaries with them.**
  Cause: the callee's prototype: its parameter types decide the order. A
  K&R prototype (no parameter types) or a wider type (s32 for a u32 colour)
  can give the game's order. Seen in: LobbyCard_RunNetSave (a compare
  chain's constant 1 stays in a0) and Layout_DrawRun (lobby).

### An extra or missing mask or sign extension

- **An extension (`dsll32`/`dsra32`) or a mask (`andi`) of a call's result
  in yours but not in the original, or the reverse.** (tested) Cause: MWCC
  extends a narrow result (s8, s16, u8, u16) at every use, whatever the
  prototype: compared (`!= 0`, `== 3`, `< 5`), stored in an int, passed on.
  Only a bare truth test (`if (f())`, `if (!f())`) uses v0 as it is, and a
  cast (`(s16)f()`) always converts. In the callee, returning an s16
  variable as s16 needs no extension; as an int it does. Fix: to drop it,
  test bare or declare the callee as returning int in that file; to get it,
  compare or cast. This settles the older notes: round 2's "a result
  declared u8 isn't masked" holds for bare tests only, and round 4's "an s16
  prototype makes MWCC trust the callee" too. Callers that use a narrow
  result unconverted mean the callee returns int: `s32 f(...)` with
  `return (s16)(...);` or `return *(u8 *)X;` (still `lbu`) compiles the
  same callee. `(u16)Rand_Next(1) & 1` masks with 0xFFFF for the cast.
  The reverse: callers that extend an s32 callee's result as s8 see an
  `s8` prototype in their file; a u8 getter tested `== 0` without a mask
  returns s32 (`s32 f(void) { return *(u8 *)0x6A9E61; }`); a u16 result
  that the game doesn't mask again is assigned in an if/else, not a
  ternary. Seen in: the curve updates' callers, Hammer_CheckChargeInput and
  EmAnteka_Init (game), Plsel_StateReady (plsel), LobbyMsg_DrawPart,
  func_007CA110, func_007CF4F0, func_007CA130, Page_Load and Url_GetScheme
  (lobby), Sound_PlaySePos (main). Round 9: an `andi
  0xFF` of v0 right after a call means the callee returns u8 in the caller's
  file (sub_main; Notice_Push, found by a helper).
- **An `andi` or `dsll32` before a call's argument, or none.** Cause: a value
  is converted at a call only when its type differs from the parameter's in
  the prototype the caller sees: an int into an s8 or s16 parameter is
  sign-extended, into a u8 or u16 one masked; a u8 into a u8 parameter (or an
  int into an int) passes as it is. Fix: match the types to what the original
  shows: a conversion means an int variable and a narrow parameter; none means
  the same width on both sides, or an int parameter (a callee without a
  prototype takes an int). A u16 passed through an `s32 (*)(s32)` pointer is
  masked. Clues: a narrow parameter passed on unmasked while the function's
  own tests mask it means the callee's parameter has the same narrow type; an
  int held in a saved register and masked into a0 (`lhu s4; andi a0, s4,
  0xFFFF`) is an `s32` local passed to a u16 parameter; a narrow value passed
  on unconverted after `lhu` or `lb` is an int local or parameter. A u16
  constant for a u16 parameter goes straight into the argument register; for
  an s32 parameter it goes through v0 and is masked at the call. An s16
  counter passed to an s32 parameter is extended again. The callee's
  parameter type decides in every caller of the file: a u16 pad passed on
  unmasked through a chain means every function of the chain takes `u16
  pad` (one `s32` adds an `andi`); a caller that extends only `(s8)-d` and
  passes an s8 local raw calls a callee taking s8; `(u16)em->unkA4` to a
  u16 parameter loads with `lhu`. An int copy of the u16 pad (`b = pad`)
  in the block after a call gives the pad's mask in a0. An int argument
  passed unextended right after `lh; addiu -1` is an s32 local holding the
  decremented value, stored back after the other loads (`k = t.unk8 - 1;
  ...; t.unk8 = k; f(0, k)`); a narrow copy masked into a saved register
  in the `jal`'s slot while the raw value is passed is assigned from an
  int before the call (`alpha = x; f(.., x)`). Seen in: Plsel_ItemBoxMenu
  (plsel), StageObj_K12_CreateOnEm, EmBlangonga_DigThrow, Bow_UpdateAim,
  func_006C6E40 and func_00720340 (game), QuestNet_OnGameMsg (gm_sub),
  Layout_Input, Html_ParseLength, Deco_DrawRemovePanel, LobbyStep_6310,
  func_0071F8A0, MailMenu_Update, func_007A1A50 and func_00766A50 (lobby).
  Round 9: a `dsll32`/`dsra32` of an argument before the `jal` means that the
  prototype the caller's file sees takes s16 there, even where the callee's
  definition in its own file takes s32 (sub_main; each file keeps its own
  declaration, see "A function that matches alone but not in its file"); an
  s16 value passed to an s32 parameter without a new extension was an s32
  local holding `(s16)table[...]` (func_00341D20, sub_main).
- **A narrow parameter or local extended again at each use.** Cause: u8, u16
  and s16 parameters are masked or extended at every use, an s16 counter
  every iteration, an s16 local after every join; an s16 parameter changed
  in place stays unextended in its register and is extended at each later
  use; `x + K` passed inline to an s16 parameter is hoisted unextended and
  extended at each call. Fix: an int copy or an int type (`s32 x = v;` masks
  a u16 once, `top = (s16)(y + 0x12)`, `s32 n = f();` extends once at the
  call). A u16 parameter used once gives `andi 0xFFFF`, then `andi 0x40`; an
  int copy masked with 0xFFFF keeps the mask only when tested twice.
  `dsll32`/`dsra32 16` before each test of a parameter means it is s16. An
  s16 parameter is extended where it is first used in arithmetic; an s32
  parameter copied into an s16 local (`s16 ty = y;`) is extended at the
  function's start. For one extension kept across joins, an int copy
  (`s32 lx = x;`, `s32 area = D_008EA8F8;` for a u8 global read twice),
  then `lx + 14` at each use and `(s16)(lx + 14) + 144` where the original
  extends twice; an s16 parameter changed in place (`x -= 130;`, `by = y;
  y += 14;`) or an s16 copy is extended again after every join, and
  `(s16)(x - 0x96) + 8` written at each use extends there. Round 8:
  `s32 wy = y;` once after the first calls gave one extension for all the
  `y + K`; an s32 local holding `x & mask` with `(s16)` casts at the uses
  re-extends at each use, an s16 local shares one extension; an s32
  parameter tested as `(s16)fwd` is extended at the test only (declared
  s16 it is extended at the entry, into another register); `x = x + 14;`
  then `f(x, ...)` keeps the extended sum in the parameter's register for
  later calls where `x += 14` doesn't; `s = (s16)s;` after `if (cfg) s =
  -s;` re-extends at the merge; a u8 parameter narrowed again at a
  compare is compared as ints (`(s32)cur != (s32)idx`). Seen in:
  LobbyPkt_PutU16, Records_DrawRows, HunterInfo_DrawStatus,
  HunterInfo_DrawEquipment, HunterInfo_DrawGroup, Deco_DrawRemovePanel,
  Group_Join, func_006FE370, func_00770250, CombShop_DrawLineWindow and
  IdMenu_DrawText (lobby), NetMenu_ChooseSlot and func_00315B80
  (sub_main), func_0070E520, Player_StepBack and Player_BowgunAim (game).
- **Several additions to an s16 coordinate folded into one, or a constant
  start value propagated into the calls, where the original adds and
  extends at each step (or the reverse).** Cause: steps written as
  statements (`y += 10; ... y += 0x18;`) are folded; a step inside a call's
  arguments (`func_2E66F0(x, y += 0x18)`) is not propagated and keeps its
  own add and extension. Fix: move the steps into or out of the arguments;
  `f(lx += 0x46, ...)` keeps the step in the register. A draw y passed as
  `y + yoff` in a loop where y steps is strength-reduced into an int
  temporary. Seen in: func_00775A70, func_00770250, EquipShop_DrawLine,
  func_0076F720, WeaponForge_DrawUpgradeBox, Deco_DrawRecipeDetail and
  GroupBoard_DrawRooms (lobby). LevelList_DrawEntries and
  SrvMenu_DrawList (lobby), whose original keeps `x + 0x14` unextended and
  extends it at each use, may need an s16 parameter changed in place.
- **The extension on the other operand of a compound assignment.** Cause:
  `x op= y` on an s16, u16 or u8 `x` converts y to x's type first (and loads
  y first); `x = x op y` extends x instead and adds an int y as it is. Fix:
  switch between the two forms (`x += (s32)n` keeps n's mask and loads the
  field first; `cell->unk1E += (u16)(e - b)` doesn't load the field again).
  `x |= 1 << n` on a u16 field masks the shifted value, `x = x | (1 << n)`
  doesn't; `room -= n` (s16 room, u8 n) masks n at the subtraction and
  keeps room's 0xFF in a saved register; `px -= half` into an s16 local
  keeps it in its saved register where `px = px - half` is folded into the
  argument registers; `x += ...` kept a u16 x in one register from its
  first value (level 4); `base += ofs;` on two u16 locals gives one `andi`
  after the add. Seen in: Em_AddStatus5, func_006E2180 and
  EmGravios_State0_24 (game), Layout_RowSpanCell, TownMsg_OnActor,
  Form_Submit and Layout_SelectEnd (lobby). Round 9: `*p += f()` and `*p -=
  f()` on a u16 mask f's s32 result first (`andi 0xFFFF`); `*p = *p + f()`
  doesn't (Kbd_CursorRight and Kbd_Backspace, sub_main).
- **`andi rX, zero, 0xFF` or `dsll32 rX, zero, 16`: an extended zero.**
  Cause: a chained assignment of 0 through narrow types (`a16 = b8 = 0;`,
  `px = py = 0;` with an s16 `px`, `used = given = 0;` with u16s, `mx = my =
  0` with s8s gives `dsll32 a3, zero, 24`). Seen in: Psp_DrawActor
  (select), func_00D64580 (plsel), Layout_SpreadColumns and
  Page_ScrollInput (lobby). Round 9: `btn = pad = HW->unkAE = 0;` (`andi s1,
  zero, 0xffff; move s2, s1`, func_0033E730, sub_main).
- **An `andi` on a u8 or u16 value tested as a condition, or none.** Cause:
  a u8 tested bare (`if (flag && ...)`, `!flag`) isn't masked; `flag != 0`
  and `flag == 0` mask it; `if (mask & 0xFF)` gives an extra `andi`;
  `u8 s = size; f(s, s);` masks once and copies. After a bare test the value
  stays unmasked, so a later use masks it after the branch (and the
  branch's slot may stay empty); after `!= 0` the masked value is reused.
  Flags tested after a loop, and a u8 flag re-masked as the game does
  (`andi; bnez; nop`), need `!= 0`. `s32 f = em->unk86B; if (f == 0) ...;
  if ((u8)f & 0x40)` gives `andi 0xFF` then `andi 0x40`; a u8 local for a
  byte field used twice in one expression reads it once. This choice can
  move every register of a function: `== 0`/`!= 0` for bare tests of a u8
  flag gave StageObj_K29_CreateOnPlayer its whole allocation (60 to 0). A
  field just tested and compared again is re-masked as `(em->unk006 &
  0xFF) == 2`; an s8 local passed on as a u8 is tested bare (`if (k)`) to
  avoid a second extension; a local for a field used in a loop guard adds
  an `andi` the game doesn't have (write the field out). Seen in:
  EmKirin_State1_0D, EmAnteka_Update, EmApceros_Update, EmScript_Op23,
  func_00733EA0, Player_State0_0B, Player_State0_1C, func_006F2B70 and
  func_006F0440 (game), func_00355210 (sub_main), Html_AttrInputMaxlength,
  Html_ParseColor, Layout_Option, Png_StartRows, Page_DrawFocus,
  Page_DragScroll and Select_ListClick (lobby).
- **An s8 re-extended before a shift, an index or a compare.** Fix: hold it
  in an int and cast at the use (`1 << (s8)v`; an `s32 id` compared as
  `(s8)id` and used unextended as an index); update an s8 in place
  (`k++; k %= n;`, `k--; if (k < 0) k += n;`), not with
  `k = (s8)(k + 1) % n`; an index `s32 i = m->cursor;` rather than an s16
  local, which is extended again before the shift. A byte assigned to an
  s8 local in a loop's condition (`(c2 = str[j]) != 0`) is re-extended in
  the body; read directly (`!word[k] == 0`) it isn't. A u8 masked again
  before its sign extension (`andi v1, a1, 0xFF; dsll32; dsra32`) is `u8 v`
  compared as `(s16)(v & 0xFF)`. Seen in: func_00316E90 and func_003368A0
  (sub_main), WeaponForge_MakeDetail and WordFilter_Scan (lobby),
  Plsel_CheckRankAwards (plsel).
- **The extension or mask after a select or a join, or none.** Cause: a
  ternary into an s16 local extends after the select; an `if` doesn't. An
  int tested as `(s16)changed` is extended before the test. A value set in
  both branches (`if (c) blend = 12; else blend = 8;`, or `a = c ? f : 0xFF;`)
  is selected in v0 and extended or masked at the join in the same
  register; `blend = 8; if (c) blend = 12;` gives it a new register.
  `if (s->size) w = s->size; else w = 16;` puts the mask in the `beqz`'s
  slot, `if ((w = s->size) == 0) w = 16;` doesn't. A boolean made in v0
  and then masked is `u8 t; if (c) t = 0; else t = 1; near = t;`. A clamp
  to an s16 margin compares with the int expression and stores its cast:
  `if (x1 > 0x280 - s->right) x1 = (s16)(0x280 - s->right);`. Seen in:
  func_003D6320 and unk_37B570.c (sub_main), Player_State0_3C and
  func_006FF450 (game), Layout_InputEnd and Page_DrawFrame (lobby).
  EmFxSpark_Draw (game, the alpha's `andi` sunk into both branches) is
  still open.
- **`sra` where the original has `srl`, `slti` for `sltiu`, or the
  reverse.** Cause: signedness: `x / 8` of a u8 is `srl`, `(u8)x >> 3` is
  `sra`; `sizeof` arithmetic is unsigned (`len < sizeof(t) - 1` gives
  `sltiu`, `(n + sizeof(t)) * -1` a `mult`); `(f() & 0x7FU) < *tbl` compares
  unsigned; `T[u8 - 1U]` keeps the -1 before the shift where `T[u8 - 1]`
  folds it into the offset. A halving is `srl` as `x / 2` of a u8 or
  `n / 2U` of a u16, `sra` as `x >> 1` (the value is promoted to int);
  `(u32)*p >> 1` shifts unsigned; `(u32)n > 1` gives `sltiu n, 2`;
  `(u32)s->season >= 3` compares a signed field unsigned; `(s32)` on an
  unsigned bitfield compares it signed; `d < 0x8000U` on a u16; a u8 field
  compared `> 0` is promoted to int (`blez`). Seen in: QuestNet_Parse
  (gm_sub), NetMenu_DrawMessage (sub_main), Pad_ConvertData (main),
  Player_State0_47, func_008465C0, func_006FE4B0 and func_006C4410 (game),
  Png_FilterRow and LobbyWorld_NextDay (lobby).
- **64-bit instructions (`dsllv`, `lwu`, `dsll`, `daddiu`, `pcpyld`) in the
  original.** Cause: 64-bit C: `long` is 64 bits (memcpy's size sign-extends
  an int argument with `dsll32`/`dsra32 0`); `1LL << n` gives `dsllv` and
  `lwu`; `((s64)id << 10) + 0x3400` gives `dsll` and `daddiu`; a u64 set to
  -1 is `li -1; pcpyld`; `x & 0xFFFFFF` on a u32 and a `u32 x : 24` bitfield
  give `dsll32 8; dsrl32 8`; a `char` passed to a `long` parameter is
  extended with `dsll32`/`dsra32 24`; `%` on a 64-bit unsigned long calls
  `__umoddi3` (main 0x110F38). Seen in: func_00336320 (sub_main),
  Ending_Scroll (plsel), Png_ReadRow and adler32 (lobby).
- **Bitfield stores.** Storing 0 into a bitfield isn't folded, storing 1 is;
  the bitfields of a DMA channel's CHCR give `lbu`/`and`/`or`/`sb`. Seen in:
  dma.c (main).
- **A load or store of another width or signedness (`lhu` for `lh`, `lwu` for
  `lw`, `sb` for `sw`).** Cause: the field's type. Fix: retype the field, or
  `*(u16 *)&field` for `lhu` of an s16 field; `x = (u16)x` on an s32 field
  gives `lhu`, then `sw`; `s16 k = *item;` loads an `s32 *` with `lh`; a u8
  parameter given a u16 field reads its low byte with `lbu`; a u32 field used
  in 64-bit arithmetic loads with `lwu`. `(s8)m.flag` of a u8 field loads with
  `lbu` and extends, so an `lb` needs the field (or a per-file view) declared
  s8; `(u8)buf[0]` of a `char` buffer loads with `lbu`, `((char *)row)[0]` of
  a u8 one with `lb`. `*(u16 *)&o->unk0CC` copies an s16 field with `lhu`,
  `*(u16 *)&v->pitch += dp;` gives `lhu`/`andi`; a u16 field passed to an
  s32 parameter loads with `lhu`, an s16 one with `lh`; `(s16)*(s32 *)(node
  + 0x1C)` as an s16 argument gives `lh`. Seen in: EmScript_Op19,
  Eff06_Scatter, func_006F7550, func_00861B70 and the SpitMeat items
  (game), ModeSel_DrawButton (select), texture.c (main),
  LobbyLogin_OnProgress, Png_FilterRow and DojoMenu_DrawQuestPage (lobby,
  an s8 field).

- **A narrow local extended again before an add inside a call's
  argument.** Cause: `y = y + 0x16;` as its own statement extends the s16
  again before the call uses it; stepped inside the argument, the call
  takes the raw register. Fix: `Font_SetPos(x + 0x12, y += 0x16)`. Seen
  in: Gallery_DrawCourseCard and ElderForecast_DrawQuest (lobby).

- **Local types read off the game's masks.** An `andi 0xFFFF`, then a sign
  extension at each use: an int holding `(u16)x`. A parameter sign-extended
  in place, then again before each call that takes an s16: a static inline
  with s32 parameters. Seen in: StatusPage_DrawStats (80 words to 54) and
  EquipWindow_DrawDetails (323 to 269), both sub_main.
- **A u8 parameter masked once, the masked copy used everywhere.** Fix: an
  s32 local copy (`s32 mode = mode0;`); the parameter used directly keeps
  its raw register as well. Seen in: PriceInfo_DrawDetails (lobby).

### `daddiu reg, zero, N` instead of `li`

- **`daddiu reg, zero, N` where yours has `li` (`addiu`).** Cause: a
  constant assigned to an unsigned narrow variable: a u8 or u16 local, or a
  u8 or u16 parameter reassigned in place (`state = 5;`); int, char, short
  and u32 locals get `li` (tested in round 4). Fix: declare the variable u8
  or u16, and the callee it is passed to with the same type (the families'
  `Em_SetStateSync(Em *, u16 group, u16 state, u8 mode)`). A u8 result
  variable returned as u8 (`u8 ret`, a u8 return type) gives `daddiu v0,
  zero, 2` and no `andi` at the end. Rounds 3 and 4 first listed this as
  unknown. The `daddiu` loads mark the 16-bit and 8-bit locals when you
  retype a draft. Seen in: func_006D4520 and LobbyStep_670F (lobby),
  EmChameleos_SetGroup0State, EmAnteka_Update (a u8 flag set to 1 in a
  loop), func_008465C0, EmGravios_State0_24, Player_State0_0B and
  Player_State3_00 (game).

### Compare forms

- **`slti at, x, K` and `bnez at` in the original, `slti v0` in yours (or
  the reverse).** (tested in round 8) Cause: MWCC computes a compare into a
  real register only when it branches on `x < y` as written being true
  (`bnez`): `if (x >= K)` (it skips the body when x < K), `if (x < K ||
  ...)`, `if (x >= K && ...)`, and a loop's bottom test `while (i < n)`.
  Every other form uses `at`: `<` in a plain `if` (`slti at; beqz at`),
  `>` and `<=` everywhere (also `K < x` and `K <= x`), `>=` as a loop's
  bottom test, and a loop's entry test. The same holds between two
  variables (`if (h <= y)` gives `slt at, y, h`; `y >= h` a `slt v1`) and
  for unsigned compares. Fix: `x > K - 1` for `x >= K` (the usual one: `a
  > 1` for `a >= 2`, `k <= 10` for `k < 11` in a loop), and back;
  `(u32)(x - N) <= 1` for m2c's `< 2U`, `(u32)(x - 0xC) > 1` for `addiu;
  sltiu at, 2; bnez at`; `diff <= 0x2000U` where `diff < 0x2001U` computes
  into v0 (in a test that branches on it being true). The round 7 reading
  "`<` computes into a normal register" holds only in those positions.
  Seen in: Sjis_CharBytes, func_003D42B0 (sub_main), Parts_Animate
  (select), func_006C6600 and EmKirin_State1_0C (game), Png_ReadImage,
  Layout_OpenCell, VFx_Init, Select_ListClick and func_007D85D0 (lobby),
  Plsel_DrawEquipPage (plsel).
- **`bgez` or `bltz` against `slt at, x, zero`.** Cause: `x >= 0` and
  `x < 0` give `bgez`/`bltz`; `0 <= x` gives a compare into `at` (and
  `0 < f()` gives `slt at, zero, v0`). MWCC keeps a `u8 < 0` test
  (`lbu; bgez`). Seen in: Yn_DrawEntryDetails (yn), the families' "target
  in front" tests (game), unk_2E6F40.c (sub_main).
- **`sltu`/`xori` (a computed `!x`) before a branch.** Cause: MWCC computes
  a negation only when the source makes it a value. Fix: `!(x) != 0` as the
  condition, `if (!(f() != 0) == 0)`, `!!(x)` inside `&&`,
  `while (!(c = *p) == 0 && (tbl[c] & 2))`; `ok = !(x & 0xF);` gives `sltu`
  and `xori`, `ok = (x & 0xF) == 0;` gives `xor` and `sltiu`; a u8 counter's
  `sltu; xori; bnez` is `!(n != 0) == 0 && ...`. Seen in: Yn_ScanCards
  (yn), EmHermitaur_State2_07 and FinFindKey (game), Html_SkipSpaces
  (lobby), unk_2F7420.c (sub_main).
- **One range test (`sltiu at, t, 3`) where yours has separate compares, or
  the reverse.** Cause: `t == 0 || t == 1 || t == 2` on an s32 local, and
  `x == K || x == K + 1` on an s32 global, merge into a range test; an
  unsigned constant (`0x37U`) or a cast on one side keeps the compares. Fix:
  read the field into an s32 local first to merge them, or add the `U` to
  keep them apart. On a byte field, `andi 0xFF` then `sltiu at, x, 2` is a
  u8 local compared as `fmt <= 1U` (`== 0 || == 1` gives two compares,
  `< 2U` no `andi`). `part == 4 || part == 3 || part == 2` on an s32
  parameter merges too; the game's separate `beq`s are then a switch
  (`case 2: case 3: case 4:`), which keeps them. Seen in:
  Plsel_SumRecordRewards (plsel), the lobby's first village range,
  Npc_HidePart (lobby), Tim2_CheckHeader (main), func_006BDA00 (select).
- **The operands of a comparison loaded in another order.** (tested) Cause:
  with a call on one side, the left operand is evaluated first
  (`f(i) > s->x` calls first; `s->x < f(i)` loads first into a saved
  register); between two plain loads, the right operand is loaded first; in
  a loop MWCC may still load before the call. Fix: swap the operands and the
  operator; for a load after the call, `v = f(); if (v < g[i])`; an operand
  that contains a call goes first (`from + strlen(from) == hit + len`). An
  operand computed into a local before the test is evaluated first
  (`n = expr; if (--em->timer <= n)`), and a difference computed into a
  local loads its operands in source order (`dy = a - b; if (x > dy)` loads
  a first, `x > a - b` loads b first). The operand order also picks which
  value gets a0 or a1 while the compare form stays (`cy < top + 0x10`, not
  `top + 0x10 > cy`); `if (x % 100 < -v)` puts the `negu` in the `bgez`
  slot where `-v > x % 100` doesn't. Seen in: func_00D676E0 (plsel),
  unk_361570.c (sub_main), EmRathalos_AirDive, EmYamaTsukami_State3_01
  and Eff00_Setup (game), Page_PointerScroll and Page_PointAt (lobby).
- **A compare the original keeps and yours folds.** Cause: `(f() == 0) == 1`
  is folded, and so is the test of a local just assigned a constant. Fix:
  store `f() == 0` in an s32 local first; `(w->f = f()) == 0` tests the
  stored narrow value, extended; `if ((D_0024BE10[port].vibProfile = 3) !=
  0)` keeps `li v1, 3; beqz v1`. A compare of a value with itself survives
  when one side is an int local (`s32 v = *p; old = v - 0x80; ... if (old
  != v - 0x80)`; a `u8 v` folds it). Seen in: the lobby's second village
  range, Yn_ScanCardsForConnect (yn), Pad_SetVibration (main),
  Option_BrightnessInput (select).

- **A `movn` where yours branches.** Fix: `x == 0 ? A : B`
  (`last = D_006ADCF1 == 0 ? 3 : 9;`). Seen in: PouchMenu_Update (sub_main).

- **`sltu at, a, b` and `bnez at` in the original.** That pair is the
  assembler's `bltu a, b` macro: MWCC emits it for an unsigned `<`, `<=`
  or `>` in an if (the `at` forms above); `>=` in an if computes into v0.
  The operand written first takes the first register (`gw <= gh` gives
  the macro with gw in a0). Seen in: func_003294A0 (sub_main), which then
  matched at level 3 with the compare on the globals themselves
  (`if (D_005D9FF8 <= D_005D9FFC)`).

### Switches: compare chains, jump tables, default placement

- **The compare chain in another order.** Cause: a switch without a jump
  table tests its cases in reverse source order, within a group of labels
  too (`case 0x309: case 3: case 1:` compares 1, 3, 0x309); the case bodies
  stay in source order. Fix: write the cases in the reverse of the chain's
  order. The chain 1, 2, 0 of `case 0: case 2: case 1:` shows that it is
  source order, not value order (one note's "from the highest value down" is
  the same thing for ascending cases). For generated C: the blocks in
  address order, each block's labels in reverse compare order. Round 8
  confirmed it everywhere: `case B: case A:` gives `beq A; beq B`; `case 1:
  case 5: ... case 0x5B:` compares 0x5B first; a chain 6, 4, 5 is `case 5:
  case 4: case 6:`; `case 0: break; case 1: ...` gives `beq 1` first. m2c
  lists the cases in source order, so reverse both the labels and the
  bodies from the asm. An unsorted chain in the game is a switch (an `||`
  chain leaves a `bne` last). Seen in: Item_IsUsable (sub_main, 0x614 bytes
  on the first try), func_007AC5C0 (game), func_00780970, func_00780A80,
  func_00780100 (34 kinds), TownMsg_SendState, func_00761790,
  func_007BBF70, func_007D9410, Form_Submit and BrowserDlg_DrawStatus
  (lobby), Pad_ReadPort (main). Round 9 confirmed it again: Kbd_SetLayout and
  func_00317270 (sub_main), QuestBoard_Draw (cases 12, 9, 10, 11 compare 11,
  10, 9, 12), PoogiePet_Update, Npc_IsPresent and the townsperson motion
  tables func_0075B8A0 and func_0075C8C0 (lobby), func_006FFE50 (game),
  Plsel_BuildResultText (`case 0: case 1: case 2:` gives `beq 2; beq 1; beqz`,
  plsel) and every kind group of func_00906210 (gm_sub). A default sharing a
  case's body takes that case's place in the chain: PoogiePet_Update's chain
  3, 0, 1, 2 is `case 2: ... case 1: ... case 0: default: ... case 3: break;`,
  with `case 0: default:` after the other cases; written first, the group
  drops the compare of 0 (see "A dead `li v1, K`").
- **A compare and a branch to the switch's end that your C doesn't have.**
  Cause: m2c drops `case N: break;`, but MWCC still compares N. Fix: add the
  empty case back from the chain. Seen in: func_0079F1A0 (game, 0x24A8
  bytes).
- **A compare chain where the original has a jump table (or the reverse), or
  a table of another size or start.** Cause: about six consecutive cases
  make a table, which spans the lowest to the highest label, and labels that
  only `break` count. Fix: list the empty cases the table covers
  (`case 2: case 3: break;`); `case 5:` next to `default:` keeps the full
  size; `case -1:` sharing the default's body starts the table at -1; with
  only the working cases MWCC uses a chain. With a table, write every
  label it covers, the default's too (`case 0: case 4: case 5: case 6:
  case 9: case 10: default:`), and put the bodies in the table's address
  order. Seen in: Countdown_Draw (sub_main), CharaSel_Update (select),
  Gs_SetState (main), the families' Update functions and the
  motion-effects giants (game), DlgBox_Update and Page_Load (lobby).
- **The default body right after the compares, or reached by a `b`.** Cause:
  the bodies follow source order, so `default:` written first falls through
  after the chain and written last is reached by a `b`. m2c's `if (t != A) {
  if (t != B) { P; return; } Q; return; } R;` is `switch (t) { default: P;
  break; case B: Q; break; case A: R; break; }`. Fix: move `default:`. A
  default written first also changes the delay slots (see "Delay slots
  filled or empty"). Seen in: StageObj_K12_Setup (game), Yn_ConfigToDev
  (yn), func_0070CD40 (lobby).
- **`beq x, K, L` and a `b` past it where yours has one `bne`.** Cause: a
  switch with one case, `switch (x) { case K: ...; break; }` (with a
  default: `beq x, K, case; b default`); an `if` gives one branch. Nested in
  a case: `beqz v0, L; nop; b end` after a call is
  `switch (f()) { case 0: ...; break; }`. m2c often prints it as
  `if (x != K) {} else { ... }`; `if (f() == 0)` gives `bnez v0, end`
  instead. A call's result tested `beq 3; beqz; b end` is a two-case
  `switch (f()) { case 0: ...; case 3: ...; }`. Fix: the switch (with a
  cast where needed, `switch ((u16)em->unk296) { case 0: ...whole body...;
  break; }`). Seen in: func_006C8350 (lobby, from 139 words to a match),
  func_003E6F20 and func_003C4880 (sub_main), func_006BE8E0,
  func_007580B0, func_00720310 (five sites), CombShop_Steps and the dojo
  menu states (lobby), func_006ECE20, func_006C6600, func_006F3170 and
  func_00796FA0 (game). Round 9: steps whose code is `beqz v0; b end` are
  one-case `switch (f()) { case 0: ...; break; }`, and `if (r[0x16] != 4)` is
  `switch (r[0x16]) { case 4: ...; default: ... }` (QuestCounter_Step, lobby).
- **`beq` to one block and `b` to another (a two-way switch).** Cause:
  `switch (x) { case K: A; break; default: B; break; }`, which m2c writes as
  `if (x != K) { B; return; } A;`. `li v0, 1; beq v1, v0, A; nop; beqz v1,
  A; nop; b B` is `switch (x) { case 0: case 1: A; break; default: B; }`,
  not `||`. A flag set per case (`case 8: v = 3; break; default: v = 0;`)
  puts the `li` in the branch slots. m2c's `(arg & 0xFF) != 1` is often
  `switch (arg & 0xFF) { case 0: default: A; break; case 1: B; break; }`:
  the A block reloads a0 from its saved register and follows an alignment
  nop (`default:` alone left 2 words). `beq x, 1, L; nop; b join; move v1,
  zero` then `L:` setting 1 is `switch (x) { default: f = 0; break; case
  1: f = 1; break; }` (an if/else or a ternary folds into `xori`/`sltiu`).
  Seen in: StageObj_K11_Draw, EmBlangonga_Turn, EmDrome_State3_06,
  Player_State2_21, Player_State5_05 and Player_State3_00 (game),
  QSync_OnPlayer (gm_sub), LobbyStep_670F and HunterInfo_DrawActions
  (lobby). Round 9: `beqz s1, A; b B` with A before B is `switch (k) { case 0:
  A; break; default: B; }` (func_00341D20, sub_main).
- **`beq A, L; beq B, L; b else`, or `beq A; beq B; bne C, else`.** Cause:
  `if (t == 1 || t == 0x2A || t == 0x25) {...} else {...}` gives `beq`,
  `beq`, then `bne` to the else block (m2c prints it as a switch with an
  empty case); a switch gives `beq; nop; b`. A switch whose cases fall into
  `default:`, with a `break` inside the case's `if` (`switch (x) { case B:
  case A: if (c) { ...; break; } default: ... }`), gives `beq; beq; b`.
  More `if` forms: `beqz; bne 4` with an alignment nop before the body is
  `if (a == 0 || a == 4)`; `beq A; bne B, end` is `if (r == 3 || (r == 2
  && pad & 0x8000))`; `beq x, 1, L; bnez x, end; <test>; L: body` is `if (x
  == 1 || (x == 0 && test))`; `beq 8; bne 9` with the third block last is
  `if (x != 8) { if (x == 9) A else B } else C`. `x == 2 || x == 4`
  compiled `beq; beq; b` is a switch. Seen in: EmRathalos_VersusPoseC,
  EmChameleos_Update, EmBulldrome_Update, Player_State2_0F and
  Player_State0_01 (game), func_0070C800, func_007C90C0 and Form_Submit
  (lobby). Round 9: a guard `if (t != 6 && t != 7 && t != 4) return;` gives
  `bne`; the game's `beq` chain and `b end` is `switch (t) { case 4: case 7:
  case 6: <the rest of the function> }` (func_006FFE50, game); a trailing `if
  (a == 5 || a == 4 || a == 13)` whose last test is a `beq` and a `b` is a
  switch (func_00317270, sub_main); so are `||` chains of equality tests
  compiled `beq; beq; b` (func_00782A60, lobby).
- **A chain of `!=` tests returning 0 or 1.** Cause: a switch whose cases
  share one body. Fix: `switch (x) { case A: case B: ... return 1; } return
  0;` with the cases reversed. Seen in: func_00723BE0 (game, 52 cases).
- **A dead `li v1, K`, or a `beq` to the next instruction, before the
  default's body.** Cause: case labels sharing the default's body; their
  order decides which compares stay. `case K: default:` leaves a dead
  `li` (the dropped compare's constant); `case 1: default:` and `default:
  case 1:` drop the compare of 1; `case 0: default: case K:` keeps K's
  (`beq v1, K, L; nop; L:`, a branch to the next instruction) and drops
  0's; `case 1: default: case 2:` gives `beq v1, 2, next` with no dead `li`;
  `default: case 4: case 0x10:` (or `case 4: default: case 0x10:`) leaves
  `li v1, 4` dead where `case 0x10: case 4: default:` misses by two; a chain
  ending in a bare `li v0, 0x33` is `case 0x33: default: case 0x3D:`. Seen
  in: func_00889030, EmDiablos_State4_05, EmBlangonga_KnockDown,
  func_0078A2E0, DualSwords_State04 (`case 0: default: case 2:`),
  Player_State2_08, EmGravios_SetGroup1State, func_006C4FF0,
  HuntingHorn_PlaySong, func_006EFDE0 and EmBlangonga_MotionEffects
  (game), BrowserDlg_DrawStatus (lobby). Round 9: the group's place decides
  too: `case 0: default:` written before the other cases drops the compare of
  0, written after them keeps it (PoogiePet_Update, lobby).
- **One exit block per case, or one shared exit.** Cause: `return` in a case
  gets its own exit block and `break` jumps to the switch's end
  (`if (c) { ...; break; } break;` equals `if (c) { ... } break;`); a
  dispatcher whose cases all return keeps a `b` after the last one, and a
  last case ending in `return` jumps to the epilogue where `break` falls
  through (a `b epilogue` right before the epilogue). m2c's gotos to a
  shared `return 1` after a switch are `break`s with the return after the
  switch. A `default:` that computes a result into a local and returns it,
  with the cases after it returning the argument, gives a `b` over the case
  block (with `break` and one return after the switch, MWCC coalesces the
  two values). A `b` to a lone `b end` block after the cases is
  `default: return;`; a `b epilogue` with `li v0, 0` in its slot right after
  the chain is `default: return 0;` written first. Fix: return or break as
  the original's blocks show. Seen in: Player_RunWeaponState,
  EmScript_Op0F, func_0079EFE0 and EmMelynx_UpdateFace (game),
  func_00780A80, Monster_IsAvailableOnline, Img_TexSize and func_007DA6A0
  (lobby).
- **One block reached from two cases.** Cause: MWCC doesn't merge duplicate
  code (tested with repeated returns, calls and case tails), so a shared
  block is a `goto` in the source: a label inside another case (`state = 6;`
  then case 6's body; `f = 100.0f; goto move;`), or `goto fail;` into an
  earlier case. Exceptions seen: m2c's goto to a one-statement tail was that
  statement written twice (func_00798BF0, game), and two
  `return D_006AEF83;` shared one exit (sub_main). Two cases where the
  first jumps over the second's call to a shared tail are
  `case A: o = f(B); goto near; case B: o = f(A); near: ...`; a condition
  tested before a switch can jump into a case (`goto skip;`), and a case
  into the default (`if (o->state == 0) break; goto draw;`; a stage switch
  with `goto normal;` into its default). Seen in: EmGaruga_SetGroup1State,
  EmLaoshan_Update, EmScript_Op03 and the Rathalos, Diablos and Kushala
  Updates (game), Plsel_SaveRecordMenu (plsel), func_00781150,
  AreaMenu_Update and DlgMsg_Draw (lobby). Round 9 confirmed it: the cases of
  a switch inside a loop that end with the same code jump into one copy, so
  the earlier cases `goto widen;` and the label sits inside the last one's
  `if` (Plsel_BuildRewardLines, plsel); two identical tails, one reached by a
  jump back up, are a label in the first branch's body; case 0x43's shared tails in func_00906210 (gm_sub) are gotos into
  the next cases; failure paths that each write their own `unk30[1] = 0` have
  no shared label (BoxMenu_EquipPickInput, lobby). The reverse: MWCC does
  merge two identical if/else arms (`e += r % 100` in both); writing one as `e
  = (void **)((s32 *)e + r % 100)` keeps them apart with the same code
  (Player_SetPartAnim, sub_main).
- **The switch value compared in another register.** See the chain's
  constants under "Temporary registers swapped". NetMenu_Draw (sub_main, a0
  where MWCC used v1) matched with the value in a `u8` local
  (`k = m->unk2C[i]; switch (k)`), which takes a0. Passing the parameter's
  saved copy to a call (`f((Em *)o)`) frees a0 for the switch value
  (func_006EFB00, game). A pointer to the switched field lets a case
  update it through the switch's own register: `seq = &p->unk3939; switch
  (*seq) { ... (*seq)++;` (lobby), `sub = &D_008F5208[1]; switch (*sub) {
  ... *sub = 0;` (func_006ED8A0, game).

- **Separate compares in the original where yours tests a range, or the
  reverse.** MWCC folds `j == 2 || j == 1` into one range check; a `switch`
  keeps separate branches, and compares its cases in the reverse of their
  source order. Seen in: func_0032C400 (sub_main: `default:` written before
  `case 1:`).

### Branch layout: returns, shared blocks, alignment nops

- **A `b` to the very next instruction at the end of an if-chain or a
  switch arm, in the original only.** Cause: the original stored a value
  there that was already set, and MWCC emptied that block late, after the
  branch to it was laid out. Fix: write the store (the same value) in the
  else of the if whose test leaves the block that set it. Natural when
  every other branch writes it too (Npc_UpdateState's `flag = 1`); a
  FAKEMATCH when it is the only such store (Player_UpdateHunger's
  `rate = 0`). Seen in: Npc_UpdateState (lobby), Player_UpdateHunger
  (sub_main), round 26.

- **Branches, alignment nops and a `sltu` or `xori`/`sltiu` tail in the
  original's boolean function, straight-line code in yours.** Cause:
  `return c != 0;`, a result variable or a ternary give straight-line code;
  explicit returns keep the branch layout, even where MWCC folds them into a
  `sltu`. Fix: `if (c) return 1; return 0;`, `if (!a) return 0; if (b)
  return 1; return 0;`, `if (f()) return 1; if (g(x) == 0) return 1; return
  0;`; the same for other constants (`if (c) return 1; return 3;`). With a
  call, `if (f(em, 0, 30.0f) != 0) return 1; return 0;` puts the float
  constant's `lui` in the `bne`'s slot and gives each `return 0` a block,
  where `return f() != 0;` puts `move v0, zero` there. A switch whose cases
  each `if (x == K) return 1;` reuses the chain's 1 in v0 and shares one
  `return 0`. Seen in: NetMenu_CardChanged (sub_main), func_00723F20 and
  func_006C0920 (game), Lobby_NetMenuAllowed (lobby).
- **An early return placed at the end, or before the tail.** Cause: the
  blocks follow the source: `if (mode == 0) { ... } else { return 100; }`
  puts the return at the end, `if (ok) { ... } else { return -1; } tail;`
  puts the -1 before the tail, `if (!active) { ...; return 0; } return -1;`
  after the body. An if/else around the rest of the function instead of an
  early `return;` moves a store into a `b`'s slot and sends the false branch
  straight to the epilogue, or shares a `return 0` block. Fix: arrange the
  if/else and returns like the original's blocks. Seen in: func_00321A60
  (sub_main), Cond_IsLatest and Dialog_MenuStep (lobby), func_00711030
  and Player_GatherAtSpot (game).
- **An if-body placed after the function's last block and jumped back
  to.** Cause: a `goto` to a block written after an explicit `return;`,
  with every early exit its own `return;`. Seen in: func_0030EB20
  (sub_main). Round 9: `if (A == 0 || B == 0) {X} else {Y}` places B's test
  after X, where the game has A's test branch to B's test after the block: `if
  (A != 0) goto check; stop: X; return; check: if (B == 0) goto stop; Y;`
  (Chara_CreateTask, select).
- **A loop's inner exits branch straight to the end while its own exit
  goes through a `b` over an alignment nop.** Fix: `goto end;` for the
  exits inside the loop, `return;` after the loop, `end:;` at the end
  (`break` sends the inner exits to the `b` too). In a goto-structured
  function, `return` (not `break`) in a loop body keeps the game's separate
  `b` block. Seen in: TagText_ReadTags and Page_Navigate (lobby).
- **All early exits branch to one shared return block in the original; yours
  has a `b epilogue` block each (or the reverse).** Cause: each `return x;`
  gets its own block. Fix: nested `if`s and one `return 0;` at the end, or
  `goto end;` to it; `if (c) return 1; break;` in the cases shares one
  `return 0`; a final `return -1` reuses a comparison's `li v0, -1`; a body
  that is one `if (x) { ... }` before `return 0;` branches to the shared
  return, where `if (!x) return 0;` first gets a block of its own. Seen
  in: func_00D71BD0 (plsel), func_002F7000 (sub_main), Layout_CellColSpan
  and Layout_DrawCellBox (lobby). Round 9: `if (a) return; if (b) return;`
  branches straight to the epilogue, where `if (a || b) return;` and nested
  ifs go through a `b` stub (Quest_OnRemoteKill, sub_main; adiff's `full=0`
  hid it); a case's failure branch straight to the epilogue is `goto end;`
  with `end: ;` at the function's end, where a `return;` gets its own `b`
  block (sub_main).
- **An alignment nop at a join in the original (or in yours).** Cause: MWCC
  aligns switch case labels (and a default written first) and the label of an
  `if (b) return 1; return 0;` it folded; an `if` clamp gets an aligned join,
  a ternary doesn't. Fix: `n = (n > K) ? K : n;` or `x = (x == 0) ? a : x;` to
  avoid it; a switch to get it: `beqz v0, L; nop; b L2; nop; nop; L: b end; li
  v0, 0; L2: li v0, 1` is `switch (x) { case 0: return 0; default: return 1;
  }`, and a fall-through block that starts after an alignment nop is a default
  written first. A `static inline` helper also leaves aligned joins. Alignment
  nops at a plain if/else's labels can come from a difference elsewhere (the
  order of the stores after it): fix the rest first. `x = -1; if (c) x =
  K;` leaves a nop at the join where `x = c ? K : -1;` doesn't. `if (a) t
  = 60; else if (b) t = 15; else t = 30;` branches past the alignment nop
  where a nested ternary branches to it (only func_diff shows it). Seen in:
  func_006BEC30, StageObj_K1C_GrowFactor, func_007E0950, func_00858880 and
  func_0070D360 (game), QuestNet_Recv (gm_sub), Equip_FitsPlayer,
  LobbyLogin_DrawIcon, DlgCursor_Draw and func_007728D0 (lobby).
- **The two blocks of an if/else swapped.** Cause: `if (!x) A else B`
  compiles as `if (x) B else A`, while `if (x == 0) A else B` keeps A first
  (the same for `!(x & 0x80)` against `(x & 0x80) == 0`). Fix: switch
  between `!` and `== 0`, or between the positive and negative form of the
  condition (`if (t->active == 0 || (a != x && a != y)) return 0; return
  1;`); m2c's gotos between two returns can be `a == 0 || b == K`, and its
  goto triangles are `a || b || c` (`if (area == 0xB || area == 0x15)`).
  Seen in: StageObj_K11_OwnerInMotion, StageObj_K1C_OwnerActive and the
  motion-effects giants (game).
- **A `b` to the end (with alignment nops) after an if-body, or to the
  epilogue just before it.** Cause: an explicit `return;` there; without it
  the body falls through (also `return;` at the end of an if/else, or of
  the last default's `if`, not `break;`). Seen in: Plsel_DrawResultPage
  (plsel), Link_Follow and DlgBox_Draw (lobby), Player_UpdateCarryInput
  (game), Heap_Move (main).
- **The value in a select's delay slot, or its polarity, differs.** Cause:
  the ternary's form: `a != K ? x : y` against `a == K ? y : x`;
  `id = c ? A : B;` puts both `li` in delay slots; `c ? v : -1` returns an
  int -1 with the flag in v0; two `if (...) return -1;` became a `negu`;
  `x < 0 ? f() : x` gives `bgez s0, L; move v0, s0` over the call where
  `x >= 0 ? x : f()` gives a `bltz` and a `b`; `if (c == '=') state = 1;
  else { state = 1; goto end; }` puts the copy in the `bne`'s slot, where
  `state = 1; if (c != '=') goto end;` doesn't; `c ? (s16)3 : (s16)8` puts
  the result in a2 rather than a3; `x = (x > 0.0f) ? m : -m;` stores in
  both arms. Seen in: func_0033FD70 and BladeData_StatusValue
  (sub_main), LobbyMsg_DrawPage and Html_ReadAttrValue (lobby),
  func_006EDFF0 and EmKushala_ClampVelocity (game).
- **The copy of a call's result in the branch's delay slot
  (`beqz v0, L; move s0, v0`); yours copies first and tests the copy.**
  Cause: `if ((x = f()) != 0)` tests v0 with the copy in the slot;
  `x = f(); if (x != 0)` copies first. The same for loops: `if ((n = f())
  < 10) do { ... } while (n < 10);`, `do { ... } while ((p = f(x)) !=
  NULL);` (a `while` with that test is rotated, the test at both ends);
  `cell = f(); if (*cell != -1) { list = cell; ...` gives `lw a0, 0(v0);
  beq; move s0, v0`. A float assigned in a condition puts the `mov.s` in the
  slot. Fix: the matching form. Seen in: func_006E3C50 (game),
  NetMenu_DrawMessage and unk_328CD0.c (sub_main), Layout_RowSpanCells and
  Layout_CellSpanNext (lobby).
- **A store in each branch in the original, one after the join in yours (or
  the reverse).** Cause: `if (c) G = a; else G = b;` keeps both stores; a
  local assigned in both branches and stored once gives one; m2c merges
  stores the original has in both branches. Seen in: func_003D7010
  (sub_main), func_0070B070 (game).
- **A `j` tail call against a `jal` and the epilogue.** Cause: a call as a
  function's last statement becomes `j`, but a switch whose last case calls
  keeps `jal`; a function that is only `j target` is `return target(args);`;
  `return f();` and `f();` in a wrapper put a store in or out of the `j`'s
  delay slot; a dispatcher that returns its callee's result (`return A(m);`)
  gets `b epilogue` with `ld ra` in the slot, a void one doesn't. m2c drops
  what the `j`'s delay slot sets (an argument, `em->step++`). Seen in: the
  families' "enter a state" wrappers (game), the lobby's reply stubs.
- **A branch to the next instruction, a `b` over an empty else, or compares
  whose branches all go to one place.** Cause: a store or a test MWCC deletes
  while keeping its branches. An `else` (or a then-branch) that re-assigns the
  value the variable already holds: `s8 r = 6; if (a) { if (b) r = ...; } else
  { r = 6; }`, `if (c) r = 1; else r = 0;` with r already 0, `else { img = p;
  }` (the then-block keeps its `b` to the epilogue right after it), `if (x ==
  0) p = *list; else p = *list + base;`. A redundant test: an empty `else if
  (x == 2) { return; }` keeps its compare and branch (an empty block is
  removed), as do `if (flag == 11) return; return;`, a second `else if` with
  the same test, and `if (a && b) { if (b) ...`; `else if (a && b && c && d) {
  return 0; } else { return 0; }` keeps the compares of a, b and c and drops
  d's. More: `flag = 1; break;` with flag already 1; `if (x < K) { r = r; }
  else { r = 20000.0f - r; }` keeps `bc1f; nop; b join` (an empty
  then-block or `!(x < K)` folds); a loop test repeated, `if (p->frame >= j
  && p->frame >= j) break;`; a dead `i = 0;` that keeps a `bnez at, wait;
  b end`; `if (item == 0xFFFF) {} else { while (...) ... }` for a loop
  entered by a jump to its test in a block after the epilogue (`if (item
  != 0xFFFF)` inverts the loop). Fix: write the redundant code the
  branches show. Seen in: func_006F89D0, LobbyNotify_6121,
  LobbyStep_Task5Done, func_00770250, ArmorForge_BoxInput, func_00780100
  and func_007D9190 (lobby), Gs_GetState, Tim2_GetImage and
  Tim2_CheckHeader (main), func_00D75920 (plsel), EmScript_Op3C,
  func_006F83F0 and func_00796FA0 (game), Yn_LoadModule (yn). Not yet
  solved: Player_UpdateHunger (sub_main). DualSwords_State04's extra `beq
  variant, 2` was `case 0: default: case 2:` (see "Switches").
- **A `b` to another `b` (no jump threading).** Cause: MWCC doesn't thread
  a jump to a jump the source makes: `if (A || B) { if (C) f(0.8f);
  return; } f(...);` keeps the inner `if`'s exit as a `b end` of its own,
  where `if (A || B) { if (C) f(); } else { g(); }` threads it. Seen in:
  LobbyWorld_CatchUp (lobby).

- **A branch to the epilogue in the original, to a `b` just before it in
  yours.** Fix: an early `return;` on that path. Seen in: func_006C6BD0
  (select).
- **The else block placed first in the original.** Fix: invert the test:
  `if (a == X || b == Y) B else A` for `if (a != X && b != Y) A else B`.

- **A block far off with sizes that differ: wrong control flow behind the
  register noise.** Compare the block's branch targets and size with the
  game's before anything else: a missing `return` (Page_DragScroll, lobby:
  387 words to 82), two tests that are one if/else (BoxMenu_DrawEquipList,
  96 to 23), a test that returns on its own (Page_Click). func_diff checks
  branch targets and is the measure here; adiff ignores them and can score
  the right control flow worse.

### Delay slots filled or empty

- **An empty delay slot in the original where yours is filled (usually from
  the fall-through block).** Cause: MWCC fills slots in optimized C, but
  some forms leave them empty (round 5 first concluded, from reading the
  compiler's delay-slot pass, that none does; these matches corrected it).
  First check that MWCC would fill it at all: it fills a conditional
  branch's slot only with an instruction from before the branch that the
  test doesn't depend on, or with a harmless one from the fall-through
  block, never a load, a store, a call or a branch. So a small function
  whose fall-through blocks start with a load, a call or a `b` has every
  slot empty as plain C (Arena_FreeBlock, func_003DAAC0 and
  others, sub_main). Otherwise try these forms in this order:
  - a switch with one case and a default: the compare branches of a switch
    are never scheduled (`beq x, K, case; nop; b default; nop`;
    func_007861C0, lobby);
  - a switch with a non-empty `default:` body written first (and at least one
    case; not a jump-table switch): MWCC then fills no conditional branch's
    slot from the fall-through or the target in the whole function, before the
    switch too. An empty default, a switch with only `default:`, or default
    last don't do it. A switch whose other cases only `break` counts, since
    its default body comes first (`return;` in those cases fills the slots
    again; tested); so does one nested in an `if` (tested) or in another
    switch's case. Slots that can take an instruction from before the branch,
    and `jal`, `jr` and `b` slots, are still filled, so only part of the
    function may show it. Clues: m2c shows the switch as an `if`; the original
    has an alignment nop just before the default body, a chain of `beq x, K`
    to one block whose fall-through starts on an 8-aligned address, or a `b
    join` with the default's only store in its slot; constants loaded just
    before each store, in one register. Forms: `default: return;` or
    `default: return 0;` before the cases; an outer `switch (flag) {
    default: <inner switch> break; case 0: break; }` for a test that
    leaves its slot empty; an `if (x != 1)` or `if (sg != 5)` elsewhere in
    the function written as the default-first switch. When a default-first
    switch gives the schedule but a wrong alignment nop, look for another
    if/else in the function that can be it (EmHermitaur_Update). A
    jump-table switch doesn't do it in test functions (round 8, with and
    without table entries that reach the default), but EmFxTri_Draw's
    draft (game, a table switch with `default:` first) had its slots empty
    until the default body got case labels (`case 0: case 5: default:`):
    when a table switch's default comes first, try labelling it. Limit:
    ActSpot_IsUsable (lobby,
    5 words) gets its empty slots from `default: return 1;` first, but its
    `return 1`s become `b epilogue; move v0, a0` instead of a jump to the
    shared `li v0, 1`. Seen in EmBlangonga_ChangeState, Lance_State01,
    Player_State0_47, EmGravios_State5_04 and EmChameleos_State0_0A (game),
    QuestNet_Send (gm_sub), Player_UpdateStatus and func_00387870
    (sub_main): round 5 had written the last three off as function-level
    asm; func_00786B30, WeaponForge_DrawUpgradeBox and Lobby_ReceiveAll
    (lobby); in round 8 func_006E8C70, func_006F8B10, func_006FC350,
    func_006FE4B0, Gunlance_State02, Player_State0_06, Player_State2_21,
    func_0079EFE0, EmMelynx_UpdateFace and EmHermitaur_Update (game); in
    round 9 func_00906210 (gm_sub, 4,134 words: one `switch (n16)` with
    `default:` first, in case 9 of the 0x48-case kind switch, empties the
    conditional slots of the whole function), func_00782A60 (lobby: only
    a first `default: return;` in the inner switch did it, a first empty
    `default: break;` changed nothing, and the outer switch default-first
    put its default body before the case body), func_0077BE00 (lobby) and
    func_00383200 (sub_main, the inner switch);
  - a two-step assignment, `r = D_x; r = r != 0; return r;` after an early
    `return 1`, where `return D_x != 0;` fills the slot (func_0078E540,
    lobby);
  - a redundant test, `if (x) { if (y) return 0; return 0; }`
    (func_007A4B80, lobby);
  - a parameter copied into a local inside the `if` (`p = e;`,
    NetList_ReadValues, lobby);
  - a call's result returned through an expression:
    `i = f(); return (i << 6) + 0x10;`, `r = f(); return -(r != K);`
    (sub_main), `ph = Tim2_GetPicture(tim2, 0); return ph->x - 1;` after
    an early `return 0` (Tim2_LastLevel, main; the call nested in the
    return fills the slot), `if (a == 1) return 1; if (b == 1) return 1;
    return 0;` (func_008FC080, gm_sub);
  - a bare truth test of a u16 field or a u8 local, `if (s->maxLength)`,
    where `!= 0` fills the slot (Html_AttrInputMaxlength, lobby);
  - a function ending in `if (x != 0) return pad; return 0;` (if-converted
    to `movz`): every conditional slot empty, as with a default-first
    switch (func_00773D40, lobby).
- **A filled slot in the original where yours has a nop.** Cause: the
  reverse of the above: a one-case or `default:`-first switch where the
  original has an `if`, or a value still live at the branch's target (MWCC
  fills a `return 0` from the fall-through only when v0 is dead at the
  target). Fix: an `if` or `if` chain instead of the switch, `default:`
  last, or case labels on the default body (`case 0: case 5: default:`).
  Seen in: EmKushala_ChangeState and EmFxTri_Draw (game).
- **The same store or constant in two delay slots.** Cause: it is written
  twice in the source: a store before an inner switch and in its case; a
  local set in both arms and passed after the join (`n = -1;` in each arm,
  then `Pouch_Add(p, item, n)`, gives `b join; li a2, -1` twice). Seen in:
  EmShakalaka_ChangeState and EmChameleos_StealItem (game).
- **`lui at` of a global in a `b`'s slot in the original, a `nop` in yours
  (or the reverse).** Cause: MWCC fills an unconditional `b`'s slot from
  its target with `lui at, 0x6B` of an absolute address
  (`*(u32 *)0x6AE398`), but not with `lui at, %hi(sym)` of an extern.
  Conditional branches take either. Fix: a named object for the `nop`, the
  raw address for the filled slot. Seen in: LobbyWorld_Skip (lobby).
- **Stores and constants in a `jal`'s slot:** see "An instruction or
  constant load in another place".

- **Empty delay slots, but a default-first switch in yours doesn't give
  them.** Cause: the switch is on another test than the original's. Fix:
  write the switch plainly, then let tsweep's `defsw` rewrite move it to
  each test in turn; then early returns (`if (x == K) return;
  call; return;`) and one-case switches. Seen in: Npc_State2C and
  ActSpot_IsUsable (lobby).
- **A compare chain's delay slots empty in the game, filled in yours, the
  default a bare `b` to a shared return.** Cause: MWCC fills the chain's
  slots once the default's code vanishes (a `break`, a `goto`, `;` or dead
  stores), and `default: return 2;` merges with the last return and gets
  the head scheduled. Fix: a default with code of its own that stays: a
  compiled-out check as its body (2 words left: the default label's
  padding). Seen in: LobbyStep_DownloadQuests (lobby, 299 words to 2).

- **An empty slot where the fall-through's first instruction writes a register
  read earlier in the branch's block** (round 30, EmSlot_FreeModels): MWCC
  doesn't move such an instruction into the slot. Fix the registers first;
  the slot then follows.
- **An empty slot after `x = K; li vR,C; bne`** (x set before the compare
  constant, in the same block; round 30, Gs_UploadImagePacket). Cause: a
  block boundary between them. A redundant `if (cond) x = K;` that repeats
  x's current value makes that boundary with no code (`if (fmt == 0) bits =
  8;` in a full table of formats, every case writing bits).

### Arguments kept or reloaded across calls (static callees)

- **The original uses an argument register, a t-register or v0/v1 after a call
  without setting it again; yours reloads it or keeps the value in a saved
  register.** Cause: the callee is `static` and defined earlier in the same
  file: MWCC knows which registers it writes and keeps values in the others
  across the call (m2c's drafts then read "unset registers"); it can also load
  an argument's field before the call. This holds for static `asm` functions
  and empty functions too (an empty function called from a state machine or a
  switch's default must be static). Fix: make the callee `static` and define
  it before its callers. A static callee defined after its caller gets a
  forward `static` declaration; MWCC knows the registers of those defined
  earlier (`f1(em); f2(em);` with f1 defined before and f2 after: a0 isn't set
  again for f2). `tools/mwcc_cc.py` makes static functions global in the
  object, so assembly still spliced into the file can call them and the
  callers can become C one at a time (older notes said every caller had to be
  C first). Seen in: func_0090AB00 keeping t5 across func_0090A9D0 (gm_sub),
  NetMenu_SetState and Math_GetSinCos (sub_main), Chara_FilterParts (select),
  EmYamaTsukami_State3_01 and Player_State2_15 (game), func_00780970,
  Bmp_CreateTexture, LobbyBgm_Restart and LobbyLogin_StateConnect (lobby).
  Round 8 confirmed it many times: a0 or a3 kept across the call
  (func_00711030, func_00712090, func_0070D320's caller), t0 (func_006F2B30's
  caller, `andi t0, v0, 0xFF` in the `jal`'s slot before Plsel_SlotCount),
  t3-t6 (func_007D9190's callers match only once it is C), v1 (a chain's
  constant across func_007C8FC0), a1-a3 holding pointers to a stack
  struct's bytes (func_007C15E0), url and end in a2/a3 across Url_PathEnd
  and Url_LastDot, the motion-effects' empty default callees (the chain's
  constants stay in a0, the value in v1). A global leaf sends the value to
  a saved register instead. Also seen in: sound.c's buffer getters
  (main), Ending_InitObject and Ending_AnimateObject (plsel),
  Light_SetDefault and func_0030F7C0 (sub_main), HunterInfo_Update and
  ChatLog_Lines (lobby).
- **`move a0, s0` before a call where yours passes a0 on (or the
  reverse).** Cause: passing a typed copy of the parameter (`Player *p =
  (Player *)em; f(p, ...)`) reloads a0 from the saved copy; passing the
  parameter itself reuses a0. MWCC also passes a0 on unchanged in a block
  with one predecessor but copies it again from its saved register at a
  join. Fix: pass the copy or the parameter, as the original does. Seen in:
  the CheckHit functions and func_006EFB00 (game), func_00774D10 (lobby).
- **Finding the static callees.** A callee called only from one file (one
  address range) whose callers don't reload registers around it was static.
  `staticregs.py` (autodecomp) rewrites such drafts and `static.py`
  tries callees static in their file. When callers in two files keep
  registers across the same callee, the two files are one of the game's
  units (unk_2F6540.c and unk_2F7420.c, sub_main; round 7 found eight more
  such pairs; round 8 joined more,
  such as unk_7949D0.c and unk_7952C0.c, lobby). To check before the
  files are joined, put a static copy of the callee at the top of a scratch
  copy of the caller's file: func_007960D0 and Png_ReadRow (lobby) match
  that way. The permuter can't help here: it replaces the other functions
  with declarations.
- **A call missing from your compile.** Cause: a `static inline` callee
  (the game had some: Equip_FitsPlayer, lobby), or a draft that lost the
  call. Static and global functions are not inlined at -O3,p (tested).
  sub_main's Save_GetEquipFlags was listed here as inlined whatever the
  pragmas: it is not. Its callers' drafts had lost the call; made static,
  its three callers match (round 17; IPA keeps a0-a3 across the call).

- **A caller that matches only with its callee `static`, the callee being
  in another file.** A static function can only be called from its own
  unit, so the two files are one of the game's units: merge them. Round 18
  proved four this way (merged in rounds 19 and 20):
  Player_Update/Player_PickStatusState (game, unk_70A290.c to
  unk_722400.c), Npc_StateA1/Npc_StartTurnBack (lobby, unk_780580.c to
  unk_7873F0.c), Deco_FitsEquip/Armor_GetData (unk_3B4C80.c to
  unk_3BD5E0.c) and Quest_HandleNetMsg/Quest_SetCleared (unk_3A1070.c to
  unk_3AAD20.c, sub_main). A sweep making each same-file callee static
  matched Font_SetColorIndex, Quest_AddReward and Player_CalcResists
  (sub_main).
  - Round 19 merged three: unk_780580.c (lobby, five files), unk_3B4C80.c
    (three) and unk_3A6130.c (with unk_3AAD20.c). A merge joins only
    adjacent files (a C file's text is contiguous).
  - Round 20 merged the other two: unk_70A290.c (game, with unk_716360.c
    and unk_722400.c: Player_Update 47 words off to 24, then matched with
    `f32 buf[3]` and a FAKEMATCH mask) and unk_3A1070.c
    (sub_main, with mine_point.c and unk_3A6130.c: unk_3A1070.c calls
    Quest_DeliveredCount, static in unk_3A6130.c, so mine_point.c between
    them is in the unit too).
  - **Merging files:** one prototype per function for the whole file
    (MWCC rejects a second, different one). Pointer types can change
    freely; integer widths can't: a caller that saw `u8`, `s8` or `s16`
    where the definition says `u16` or `s32` keeps its code with a cast
    at the call (`(s8)Versus_Team(...)` as the argument,
    `(s16)QuestGoal_DeliverCountFor(...)` on the result), and a parameter
    an old prototype narrowed must stay narrow (Actor_SetState's u16
    state: with s32, `*(u16 *)` arguments load with `lw`). Data that two
    files typed differently gets the user's type with the same bytes
    (u32 arrays as u16 arrays, pointer pairs as one pointer table).
    Round 20 added: a caller that saw a narrower return keeps its `andi`
    with a cast on the result (`(u8)Obj_InCurrentArea(em)`,
    `(u16)QuestGoal_DeliverItem(i)`); a caller that saw `s32` where the
    definition returns `u16` needs the definition widened if it still
    matches (SpecialMine_Gather); a caller that saw `s32` parameters
    where the real ones are `u16`/`s16` passes its registers unextended,
    which no cast undoes: the merged file declares that callee without a
    prototype (`s32 Pouch_Add();`, `f32 *Stage_GetAreaRecord();`), or with
    the wider parameter where the narrow callers pass constants
    (Effect06_SpawnAt's bone as `s32`). A callee that some callers call
    with fewer arguments than others (Em_StartScript, EmScript_NextCmd,
    Player_InSceneEvent) stays or becomes a K&R declaration or definition.
  - A sweep of every NON_MATCHING block with each callee from another
    file of its module copied in as `static` before it (round 19)
    matched nothing new; 297 of 816 copies
    didn't compile (the callee needs its own file's types).
    Round 21 ran it again (377 copies, 223 didn't compile): nothing. An
    assembly-level check needs no compile: after each `jal` to a function
    of another file, does the caller read a0-a3, t0-t9, v1 or an f-register
    before setting it? Over every module: none (149 such reads, all to
    callees already static in their file).
- **Prototype conflicts are weak evidence of units** (round 21).
  The idea: one C file declares a callee one
  way, so callers that need two declarations are two units. In practice:
  - most conflicts are a caller's wrong local or parameter type. Equip_Color
    in unk_7683D0.c (lobby) fits `(u8, s32, u8)` everywhere once
    BoxMenu_DrawEquipPick's `alpha` is s32; StageObj_K14_FindGround
    (game) fits one prototyped definition once StageObj_K14_CreateOnEm takes
    `s16 k, s16 c` (MWCC still re-extends them where it uses them). Three
    block-scope FAKEMATCH declarations went that way: try the callers'
    types before splitting.
  - the game's own units had them: in unk_7A1120.c (lobby) Shop_CanBuy and
    Shop_DrawSell need `SavePouch_Count(u16)` while ItemInfo_Draw, between
    them, calls it with no argument (no prototype in scope), yet
    Shop_IsUnlockItem is static (Shop_CanBuy needs it) and ItemInfo_Draw
    calls it: one unit, two declarations (block-scope ones, presumably).
  So split only on a conflict that survives retyping and has other
  evidence (a static callee, data, level), never on the conflict alone.
- **Pooled string literals prove one unit.** MWCC pools identical literals
  only inside a unit, so one literal used on both sides of a candidate cut
  means no cut there (round 23, nmsub): "%4d" and "%3d" in unk_356380.c
  (sub_main) are stored once and used by StatusPage_DrawStats and by
  EquipWindow_DrawDetails, so no split lies between them. The file's .data
  order said the same. In that unit the game's calls pass s16 to
  Player_DrawSkillsPlain, whose definition needs s32. MWCC refuses a second
  file-scope declaration ("redeclared"), and K&R definitions leave the
  calls unprototyped; a block-scope declaration in the caller gives the
  game's calls. With the unit proven, the lead ruled that declaration the
  original's form, not a FAKEMATCH (unk_7A1120.c above declares
  SavePouch_Count inside functions too). Without such evidence, a
  block-scope declaration that differs from the definition stays a
  FAKEMATCH: try the callers' types first.
- **Float and integer parameters in either order.** The PS2 passes floats and
  integers in separate registers, so a definition matches with its float
  and integer parameters in any relative order, and so do callers whose
  declaration has another order. The order the callers load their
  arguments in tells the real one. Make every declaration agree with the
  definition: a C-built port passes arguments by position. Round 23
  aligned the lobby's Draw_RectClipped (colour first), Page_DrawTextLine,
  Page_DrawIcon and about 20 others this way, with every function
  unchanged; the disagreements left are in
  round 23's nm-lobby notes ("Prototype disagreements").

### Prototypes

- **Arguments not narrowed, a float argument converted to double (a call to
  a conversion helper), or registers shuffled in the callers of a function
  without a prototype.** Cause: MWCC compiles a call to an undeclared
  function as `int f()`: no conversion to the parameter types, `float`
  promoted to `double`. This also hits a function defined only under
  `#ifdef NON_MATCHING`, and a same-file function defined after its callers.
  Fix: a prototype for every callee, main's and sub_main's included, before
  the first call. When checking for one with grep, match the whole name: a
  test for `Motion_ReachedFrame(` also finds `EmMotion_ReachedFrame(`.
  Scratch files that hold one function lack the prototypes of functions
  defined later in the real file: compile drafts in the real file before
  trusting a mismatch. Seen in: Ending_DrawStaffRoll (plsel: 84 words off
  instead of 15 until Ending_DrawTitle had a prototype), func_007082E0,
  Vec_RotateY, the callers of Motion_ReachedFrame and the motion-effects
  giants (game), village 2's drafts (lobby).
- **A coordinate converted once and kept in a saved register in yours,
  converted again at each call in the original.** Cause: the prototype's
  parameter width: with s32 parameters MWCC converts `y` once; the real
  callee (Prim_DrawSprite in the browser) takes s16, and then the
  conversion at each call comes back. Seen in: Page_DrawWidget (lobby,
  0xC60 bytes, from 248 words to 4).
- **A pointer argument masked (`andi 0xFFFF`) or loaded in another order.**
  Cause: a wrong parameter type in the caller's file (a pointer declared
  `u16` or `s32`). Fix: the real types (`Jingle_Play(Player *, s32)`, not
  `(u16, s32)`). Seen in: Player_State0_54 (game).
- **A float argument missing from your call (the original sets f12 or f13
  before the `jal`).** Cause: the prototype has fewer float parameters than
  the callee takes; a wrapper that is only `j target; nop` takes its target's
  parameters. Fix: count the float registers set before the calls and declare
  them (`Motion_InFrameRange(void *, s32, f32, f32)`);
  autodecomp's `extra_protos.h` lists such prototypes. Generally, an
  argument register that holds a value at a call for no visible reason is a
  parameter nobody declared: Area_LineBlocked's "f12 for the list entry" is
  func_003C07E0's radius (its prologue saves f12). The order of the int
  and float registers set shows the prototype the caller saw:
  DlgBack_Draw loads a1, f12, a2, f13 for func_00726390, declared `(tex, u,
  x, v, y, w, h)` in that file. Seen in: Lance_State06 (game), sub_main's
  EmMotion_ReachedFrame wrapper, Area_LineBlocked (sub_main), DlgBack_Draw
  (lobby).
- **Callers of one function pass different numbers of arguments, or pass
  none and leave a0 from earlier code.** Cause: the original's prototypes
  differed between files or were missing. Fix: a declaration without
  parameters (`void f();`, only for callees without float parameters) and a
  K&R definition after it (`void f(em) Em *em; { ... }`), which compiles
  like a prototyped one for int and pointer parameters (tested in round 4).
  The same fits a file whose prototype has int parameters where the
  function wants pointers, and a callee called both as `(em)` and
  `(em, p)` in one file (it reads a1 as the caller left it). A callee that
  reads a0 although the call passes nothing is declared without parameters
  and called with none; MWCC then drops the `move a0, s0` it would add
  (`func_8FB280(3)` or passing the live value is a false fix). An int
  passed to such a declaration isn't masked. The same K&R definition
  fits a callee whose callers pass an int unconverted to an s16 or u8
  parameter: `s32 f();` before the callers, then `s32 f(id) s16 id; {`
  (the callee masks or extends it itself). A caller that passes an
  argument to a function defined without parameters declares it with
  them in its file (`(s32 color)`, `(s32, s32)`). Seen in: func_007E06E0,
  EmScript_NextCmd and func_008625F0 (game), func_003D6F50, func_00338830,
  HitId_Next and Pad_StickToDpad (sub_main), Offer_IndexOf, func_0076CAA0,
  LobbyLevel2_Refresh, func_00711520, func_007CF630, and the callers
  of func_8FB280, LobbyLogin_DrawWaveTitle and Page_Margins (lobby). Round 9:
  a call with no argument in a file whose other calls pass one has no
  declaration before it (MWCC's implicit `int f()`); MWCC takes a `()`
  declaration as `(...)` and rejects a later prototype, so the file's other
  callers declare `s32 SavePouch_Count(u16 item);` in their own blocks
  (SavePouch_Count's callers in unk_7A1120.c, lobby).
- **A parameter the code never uses.** Cause: a parameter that a caller sets
  is real even when unused, and a wrapper called with more integer arguments
  than it uses takes them (`f(p) { g(p, 1.0f, 1.0f, 1.0f); }`: int and float
  registers are numbered separately). m2c's phantom parameters (argument
  registers merely passed through) can go when no caller sets them, and m2c
  names parameters after their registers (a function that only uses a1 still
  needs its first parameter). A register set before a call for no
  visible reason (t0, a1) is such an extra parameter (`func_00D61AB0`'s
  fifth, set to 0). Route handlers take `(NetMsg m, NetMsg *pm)` even
  when they read neither: the by-value NetMsg adds 16 bytes below the
  locals. Unused parameters can also be reused as locals (see "A value
  kept in an unused parameter's register"). Seen in: func_003E2170
  (sub_main), YnNetcnf_LoadModules (yn), Parts_DrawFacePanel and its two
  twins (select, an unused `sel` the caller passes as 0), Ending_InitObject,
  Ending_AnimateObject and func_00D61AB0 (plsel), func_007C17C0,
  LobbyRoute_6191, func_007D4EC0 and func_00720710 (lobby).
- **An argument in t0.** Cause: the EE ABI passes the 5th to 8th arguments
  in t0-t3 (floats in f12 up, counted separately): a wrapper
  `f(a, b, c, d) { g(a, b, c, d, K); }`; a t0 left from a compare can be the
  fifth argument. Seen in: func_003D8B30 (sub_main).
- **`sd a0` in the callee and `ld a0` before the call.** Cause: an 8-byte
  struct passed by value in one 64-bit register (`NetMsg`, `NetBlob`). Seen
  in: lobby_net.c (lobby).
- **A large struct received by value and passed on with no copy.** Cause:
  the callee's prototype in that file takes a pointer. Seen in:
  LobbyLevel1_Search (lobby).
- **Varargs.** Declare variadic callees as such (Text_Printf). A variadic C
  function needs MWCC's `va_start`: `#define va_start(ap, last) ((ap) =
  (char *)__builtin_next_arg(last) - (__builtin_args_info(2) >= 8 ? 0 : (8 -
  __builtin_args_info(2)) * 8))`. Since round 31 it is in one header,
  include/ee_stdarg.h (with the host compiler's builtins under MH2_NATIVE):
  include that, don't copy the define. Seen in: unk_3DDBA0.c (sub_main),
  unk_773A90.c (lobby, Text_Printf).
- **A function that matches alone but not in its file.** Cause: a
  conflicting declaration in the file (prototypes may differ between files,
  each file sees its own). Fix: fit the declaration; `recover.py` merges
  differing pointer parameters to `void *`. The game's own declarations
  differed too: keep each file's as its code needs (`Mmbb_SetField(s8, s8,
  char *)` in unk_741900.c, whose callers extend the index, against the
  definition's `u8 idx`; func_007A2D50's item s32 in unk_7766D0.c with
  `(u16)` casts at two callers). Seen in: the pipeline's drafts (game,
  lobby), unk_741900.c, unk_7303D0.c and unk_7766D0.c (lobby).

- **A callee declared with another integer type than its definition, and
  the argument registers set in another order.** Cause: the caller's
  header narrows a parameter the definition keeps wide (lobby_net.h had
  Net_LockrockObfuscate's key as u16, its definition in unk_308BC0.c has
  s32: the u16 key's register was set before n's `move a3`). Fix: the
  definition's type. Check the definition before trusting a header. Seen
  in: HttpLbs_Send (nethttp); GroupBoard_DrawMemberMenu and
  GroupBoard_DrawMemberList (lobby) came closer the same way.
- **A local array's address kept in a saved register in yours, recomputed
  at each call in the original.** Cause: the callee's parameter declared
  as an int, the address passed as `(s32)mtx`. Fix: a pointer parameter
  (`f32 *a`): MWCC then recomputes the address at each call, like the
  other array's. Seen in: Gallery_UpdateShots (lobby).

- **A block that matches only with a callee's narrower parameter.** Change
  the callee's prototype in every file that declares it, and check that each
  file's other functions still match. PriceInfo_DrawDetails (lobby) needed
  Item_DrawIcon's size as s16 (one file still had s32) and Text_CenterX as
  `(char *, s16, s16)`.

- **Declarations that disagree with their definitions** (round 24).
  `python3 tools/proto_check.py [REPO]` checks every declaration (file
  scope and block scope, headers included) against the definition it links
  to (the same file's static function, the module's own, or main's and
  sub_main's), and prints one line per kind: `count` (another number of
  parameters), `unspec` (declared `()`), `order` (int and float parameters
  in another order), `return` (void against a value, int against float or
  pointer), `ptrint` (a pointer declared as an integer, or the reverse),
  `width` (another integer width or sign), `retw`, `ptr` and `retptr`
  (another pointer type), `other`. `--list KIND` lists them, `--by-module`
  counts them by module, `--callee NAME` and `--file` narrow the list.
  The PS2 hides all of them; a C-built port passes arguments by position
  and type. Make a new declaration agree with its definition: copy the
  definition's line. When the definition's type breaks a matched caller,
  the game's own units declared it otherwise: try the declaration's type
  in the definition (MWCC often compiles a `u8` and an `s32` parameter
  stored with `sb` the same way), and keep the caller's form only when no
  single type fits every file (Kbd_IsOpen: s8 in sub_main's units, s32 in
  the lobby's). That declaration is the unit's own: give it the comment
  `/* unit's own declaration: ... */` (on its line or the line before),
  with the function the definition's type changes and by how much;
  proto_check counts these apart (`unit`), so the other kinds are the ones
  still to fix. A narrower integer declaration of a wider
  definition (or the reverse) is harmless on the usual ABIs, which pass
  every integer in a full register; a pointer held in an integer, and
  parameters in another order or number, are not.

### Address arithmetic and base registers

- **`addu` operands the other way round (the index first, or the base
  first).** (tested) Cause: pointer arithmetic (`p + i`, `(u8 *)p + off`,
  `row += idx`) and a plain subscript (`p[i]`, or a member array at
  offset 0) put the pointer first; a subscript whose address has a
  constant offset (a field of the element, `route[i].f`; a member array
  not at offset 0, `p->arr[i]`; `p[i + 1]`) puts the scaled index first;
  integer arithmetic keeps the source order when the offset is a variable
  (`off + (u32)base`, `i + (u32)em + 0x6D4`, `(u8 *)(idx + (s32)ptr)`), but
  a product in the same expression (`i * 4 + (u32)q`, `b[6] * 0x118 +
  (s32)mdl`) still puts the base first: compute the byte offset into an
  int local of its own, then add it (`off = i * 4; v = (s32 *)(off +
  (s32)em + 0x86C);`). When the game adds `off + em` again for each field,
  spell one of them with `(u32)em` so that MWCC doesn't share the sum; for
  a table of s16 read by byte offset, `*(s16 *)((u8 *)base + j)` (FIN_S16).
  The reverse, base first after a call: `*(u8 *)((u32)D_008EA908 + off +
  0xA34)`; an index computed before the base load, `((T *)((u8 *)p + i *
  4))->f`. A pointer stepped by hand puts the pointer first
  where the subscript didn't: `talks = T[kind]; talks += talk;
  x = (*talks)[k];` for `T[kind][talk][k]`; pointer arithmetic on a typed
  record (`models + D[k] + e[0]`) against byte offsets (`* 0x8C`). A global
  indexed in full at each use (`D_5B1DD0[k][i].flags`) gives `addu base,
  off`, a pointer local to the element `addu off, base`. `(u8)s[len - 1]`
  on a `char *` adds the index first, `((u8 *)s)[len - 1]` computes
  `len - 1` apart; `*(u8 *)(s1 + k)` and `((u8 *)s1)[k]` swap the operands;
  `&save->pouch[i]` gives the index first as `(BoxStack *)((u8
  *)save->pouch + i * 4)`; `HtmlCell *cell = &D->cells[D->curCell];` then
  `cell->bold`. Fix: switch between the spellings. Seen in: func_0078B130,
  func_00788E60, Quest_GetText, Talk_FirstByte, Html_ParseLength,
  TownMsg_SendState, LockOn_Update, func_00763070, func_00774240,
  Link_Follow, Layout_TextFull, Layout_DrawRun and func_0070F910 (lobby),
  func_006EF770,
  func_0070F6E0, func_006FF450, Em_SetLayerMotion and (round 7's two open
  cases) func_007038F0 and EmPlesioth_UpdateFinFold (game),
  DmaQueue_Add and DmaQueue_AddChain (main).
- **Other operand orders in integer arithmetic.** Fix: split the expression:
  `k = (u8)f(); k = k * 4 + v * 2;`, a local per operand (`u16 yaw = f(); u16
  r = a[i]; return r + yaw;`, `u16 w = f(); n = a + (w + b * 2);`), `c = hi <<
  8; c |= lo;` (also `c <<= 8; c |= *s++;` against `c = (c << 8) | *s++;`), a
  left-accumulated sum (`t = a; t += b; t += c;`), `p->bpp * p->width` against
  `p->width * p->bpp` (the left operand is loaded first), the grouping of an
  or-chain (`b | ((g << 8) | ((a << 24) | (r << 16)))` for a colour, probably
  a macro), `y = 0x58; y += (size + 4) * 10;` for `li` and `addu` where one
  expression folds to `addiu`; `n = x % 3; n = 3 - n;` keeps the 3 in one
  register; `x * -1` stays where `-x` used twice is hoisted; `q = n / 6; n -=
  q * 6;` gives the original's multiply-and-subtract where `n % 6` divides.
  MWCC keeps the or-order of a colour as written (`b | ((g << 8) | ((a <<
  24) | 0xFF0000))`, `c | (((c << 16) | 0xFF000000) | (c << 8))`). Seen in:
  Bgm_PlayAreaMusic, func_00314800 and unk_2E6F40.c (sub_main),
  func_00795430, Html_TagEndTable, unk_751DA0.c and DlgBox_Draw (lobby),
  Plsel_ComputeReward (plsel), texture.c (main, `IMAGE_SET_PITCH`),
  func_00865F20 (game).
- **A run-time `negu` of a constant in the original; yours folds it (`li
  4`).** Cause: `d = -4; if (o->flip) d = -d;` folds, so do `0 - d` and an
  inline helper. Fix: `d *= -1;` keeps the `negu` (`d *= -2;` gives `negu;
  sll`), also on an s8 local. Seen in: func_006C4B70 (select),
  func_006C0BE0's caller and Player_BowgunAim (game).
- **`sll` against `addu x, x, x` or a multiply, or the index computed before
  or after the table load.** Cause: the scaling: `[a + a]` and `p += b + b`
  give `addu`, `[a * 2]` or a 2-byte element `sll`; `* 4` and `<< 2` order
  differently; `* sizeof(T)` (unsigned) loads the table entry before
  computing the index where `* 12` computes the index first;
  `&T[a*6 + b*36]` keeps the terms' order; `tbl[i * 3 + k]` scales the sum
  where `tbl[i][k]` gives separate induction variables. Seen in:
  EmScript_Op90 (game), Bone_SetMatrices (sub_main).
- **A global's address in a register (`lui`/`addiu` once, then offsets) in
  the original, `lui` and `%lo(sym+off)` per access in yours (or the
  reverse).** Cause: MWCC propagates `&global`, and an array used as a
  pointer, into every access; a pointer made through a cast stays in a
  register. Fix: `u8 *c = (u8 *)D_x;` (even a no-op cast),
  `T *p = (T *)arr;`; where it is assigned matters (at the top, inside an
  `if`, after the calls); `f(buf = &G)` keeps it where `buf = &G; f(buf);`
  rematerialises it; with a pointer local to a struct global, the first
  field read keeps its `%lo` (even where the original names that byte with
  a label of its own) and the others go through the register. A raw
  address used twice is folded into each use; the original's `lui`/`addiu`
  into a register is a symbol cast to a pointer local
  (`pt = (u8 *)D_5CA920`). At level 4 too, a pointer local cast from
  `&global` keeps the address in a saved register where the name used once
  gets its own `%lo`. Seen in: func_0031BC10 (sub_main), the village menus
  (lobby, `(u8 *)D_008BDC20`), func_0070CE10 (game), Pad_EndLibs (main),
  func_007D90A0 and the network screen's steps (`st = &D_5D46A0; switch
  (st->unk1)`, func_007CDBE0; lobby). Round 9: where `p = D_008C219D` (an
  array) still propagated into `%lo(D+off)` accesses, `p = (u8
  *)(s32)D_008C219D;` kept it (lobby village 1); also `t = (f32 *)D_0043CB58;`
  (func_00374340, sub_main) and `u8 *w = (u8 *)&D_008BDC20;` with the cast
  (QuestCounter_Step, PoogieMenu_Update, lobby). For `lui v1, %hi(gPlayers)`
  then `172(v1)`, assign the pointer in a nested `if` after the other
  conditions (`if (step >= 4 && ...) { pl = gPlayers; if (...) ... }`);
  assigned before the `if`, it is hoisted too early (Chara_CreateTask,
  select). A state machine's fields through a saved register while its switch
  reads the raw address: `sc = &D_006ADCB0` for the fields, `switch (*(u8
  *)0x6ADCB1)` (Plsel_ResultsUpdate, plsel).
- **A field offset in the load (`lhu 0x29C(a0)`) against one folded into the
  relocation (`%lo(sym+0x29C)`).** Cause: `Em *m = &D_5EC580[i]; m->f` and
  `((Player *)&gPlayers[i])->f` put the offset in the load; `D_5EC580[i].f`
  folds it into the relocation (also `list1[id - 1].f`, whose -1 folds into
  the address); struct-typed global arrays and typed 2D tables give one
  `%lo(D_x + 2)` per field, a cast of a u32 array one base register plus
  offsets; `k = a*2 + 4; t[k]` keeps the +4 in the index where `t[a*2 + 4]`
  folds it; `pl = &gPlayers[t]; pl->unk680` keeps 0x680 in the load. A
  table of records that splat split into several symbols
  (D_008CA320/24/28) is indexed through the symbol of the field
  (`D_008CA324[k].x`), or func_diff reports a relocation difference; the
  same symbol gives `lhu 0(v0)` where `D_008EA200 + 4 + ...` folds the 4
  into the load. An offset folded into a global's relocation needs a named
  struct array (`gPlayers[idx].angle2` with `extern PlMsg gPlayers[];`;
  with `u8 gPlayers[]` and byte arithmetic the offset stays in the store);
  a struct-typed vector member (`gPlayers[me].pos = v`) gives one `%lo` per
  member where `f32 pos[3]` with a cast adds the offsets to one base.
  Byte offsets as declared locals (`o8 = k * 8;`, then `D->unk26[0][o8]`)
  keep a table row's address computation in the game's registers. Seen in:
  func_006C6D40, EmScript_Op14, EmKushala_State3_0F and
  EmVelociprey_Reappear (game), Ending_DrawPictures (plsel),
  Pad_SetVibration (main), LobbyLevel2_EnterFree, TownMsg_Recv,
  LobbyScreen_EnterArea and func_007C33B0 (lobby). Round 9: a table read at
  each use with the byte offset in a local, `k = item * 0x18;` then
  `(&gItemData[0].max)[k]` and `(&gItemData[0].unk02)[k]`, gives the game's
  `%lo(gItemData + 3)` form (ItemInfo_Draw, lobby); `((u8
  *)&D_008C0140[k])[0x2A]` keeps 0x2A in the load where
  `D_008C0140[k].text[0xE]` put it in the `%lo` (lobby village 1).
- **A field's address computed once and kept in a register in yours,
  addressed from the base each time in the original (or the reverse).**
  Cause: a field reached through a cast (`*(s16 *)((u8 *)o + K)`,
  `((T *)o)->f`, m2c's `M2C_FIELD`) has its address computed once; a field
  of a typed pointer local, a struct member, or a padding element
  (`*(s16 *)&em->unk706[0]`) is addressed from the base each time. Fix: add
  the field to the struct (the `.spec` files), or use a typed local pointer
  or a small per-file typedef; or the reverse. This is round 7's most
  common fix: a draft reading Player fields through `em` casts matched as
  `Player *p = (Player *)em; p->input.pressed`; a work area as a view
  struct (EmChameleosFxWork, the bulldrome effect objects' EmFx); a
  parameter declared `NpcWalk *` instead of `u8 *` with casts; D_008BFAB8
  as `LobbyMenu *`. It changes register allocation and scheduling too, not
  only the addressing. A block-local `HtmlState *s = D_008B2748;` in one
  branch is addressed from the base where `D_008B2748->center` read and
  written kept `&center` in a register. Round 8: a work area's field
  written through a cast next to `&work->unk01C`, or a view struct cast
  inside the function (`((View *)p)->f` in many statements added three
  saved registers), makes MWCC keep the address; `*(s32 *)&work->unk01C =
  0;` (a field of the struct itself) doesn't. For the motion effects the
  parameter itself had to be the view type (`EmKhezuFxWork *work`); every
  cast form kept `work + 2` in a saved register. Two spellings of one
  address (`((T *)w)->snd.pos` and `(u8 *)w + 0x16C`) aren't merged. A
  typed view of a callback's node gave the game's load order. Seen in:
  GmEm_SetupKind (gm_sub), Em_AddPoison, func_006B5490, func_006C5A40,
  func_006FF7C0, EmPlesioth_State6_01, EmCephadrome_State6_00,
  EmShenGaoren_MotionEffects, func_00796FA0, StageObj_K0E_RunParts and
  func_006C5C20 (game), func_0079D6C0, SrvMenu_Choose, IdMenu_Redraw,
  Html_TagCenter and LockOn_Update (lobby, a padding element).
- **An address kept in a register for a load and a later store, or for the
  first accesses of a block.** Fix: a pointer local: `u8 *s` declared before
  the base pointer and set (`s = p + 0x3945`) inside the first `if`; `u8 *w =
  &em->unk438;` declared first for a work area inside the monster; `Em *m =
  &arr[i];` computes the element's address once where `arr[i].a`, `arr[i].b`
  build each field's; a field tested twice in one condition makes MWCC compute
  the array base, which a local (`kind = c[i].kind`) avoids; `sel = pts[k];`
  then `sel[0..2]` for a selected row. A pointer local to a field that is
  tested and then stored (`u8 *c = &p->unk38; if (*c != 1) *c = 1;`, `seq =
  &p->unk3934; switch (*seq)`, `w = &s->hrWidth;` set in several branches)
  gives the original's `addiu aN, p, K` in the branch's delay slot. `pos[1]`
  (or `text[2]`, `v[2]`) read twice after the array's address was passed keeps
  `&pos[1]` in a register MWCC makes; read into a local first, it is loaded
  from the stack slot; a declared pointer to it is a variable of its own (see
  "Stack slots and frame size"). `f32 p[2][3]` keeps `&p[1]` in a saved
  register while p[0] is addressed from sp. A local for a sum used twice (`y =
  pen.h + pen.runY`) stops MWCC recomputing it. A float read several times
  from a stack array is read into a local (`d = v[2];`); `v[2]` at each use
  keeps `&v[2]` in a register. For the lobby menus, a pointer local to the
  field (`cur = &m[0xA]`) declared before the base pointer local, with the
  base reloaded from the global after the call. Seen in: EmHermitaur_Setup,
  EmYamaTsukami_State3_07, EmKutku_SetEscapePoint, EmKhezu_NearestCeilingPoint,
  func_006ED8A0 and func_0070B4F0 (game), SrvMenu_Quit, LobbyStep_6504,
  Layout_Rule, func_007B5DE0, func_00771A60 and func_00774D10 (lobby).
- **An array base in a register (`addiu a0, w, 0x5EC; addu a0, a0, i*4; lw
  0(a0)`) where yours has `lw 0x5EC(v0)`, or the reverse.** Cause: MWCC
  CSEs `D_00895270->obj` when one statement or block uses
  `D_00895270->obj[...]` several times with no local; a local `o` for the
  element gives `lw 0x5EC(...)`. Fix: write the access out at each use,
  with no local. Other spellings of it: `*(T **)((u32)D_00895270 + 0x5EC +
  idx * 4)`, `pp = D_00895270->obj; pp += k; p = *pp;`. If the base then
  lands in the wrong register, read the element again at a later use
  instead of keeping it in a local. The same for a base pointer loaded
  first and kept in t0: no local, `&h->blocks[i]` at each use. Seen in:
  Page_Click, Select_ListClick, DlgBack_Update, Page_DrawFocus and
  DlgInput_Draw (lobby), Heap_Link (main).
- **A constant of 0x8000 or more added through `at`
  (`ori at, zero, K; addu`) against a register or two `addiu` (or the
  reverse).** Cause: a pointer plus a large constant goes through `at`; an
  int plus it loads the constant into a register (or derives it from one
  holding 0x10000: `ori v0, a2, 0x80`) or splits it into two `addiu` (0x7FFF
  and the rest). Fix: `u8 *` for the `at` form; `s32 buf` or `(u32)p + K`
  for the register (`(u8 *)((s32)gLoadBuf + 0x20000)` schedules
  `lui v0, 0x2` before the load); `(s32)(p + 0x6EF4) + 0x20` keeps two
  `addiu`; `(u8 *)(p + 4) + 0x11C4` folds where `(u8 *)((s32)(p + 4) +
  0x11C4)` doesn't. For a work-area field above 0x8000, `li at, K; addu;
  lw 0(r)` comes from one statement reading the field twice, `lui at, 1;
  addu; lw -N(at)` from a pointer local read per statement. Seen in:
  func_003D6BD0 and func_00336A90 (sub_main), func_0079EA30,
  LobbyCard_SaveNetFile, func_007BC680, func_007B50E0 and BrowserCache_Init
  (lobby).

- **An `or` of a constant colour with an alpha in another operand order,
  or other registers compared with the folded value.** Fix: an inline that
  builds the colour from its parts, `static inline u32 Rgba(u32 r, u32 g,
  u32 b, u32 a)`, called as `Rgba(0xFF, 0xFF, 0xFF, Alpha8(...))`, in place
  of a chained assignment of the folded constant. Seen in: func_007D1150
  (lobby; it also let func_007D16A0 match).
- **Sums and compares on a parameter's fields in another operand order.**
  Cause: a `u8 *` parameter read through casts (`*(f32 *)pos`) orders the
  operands differently from a typed pointer (`f32 *pos`). Fix: try the
  `u8 *` form. Seen in: HitShape_Place (sub_main; its caller
  PlayerEm_PushApart passes `(u8 *)b->pos`).

- **An address that has to stay raw.** An address taken as a value with a
  low half under 0x8000 compiles to `lui; ori` as a constant but `lui;
  addiu` as a symbol, and raw addresses in one 64 KB page share one `lui`
  where symbols get one each. Those stay raw, or become macros
  (include/hw_regs.h, include/ram_areas.h). Six functions keep raw casts
  for this (DojoMenu_OnlineStart, GroupBoard_DrawSearchForm,
  func_0075E9E0, LobbyText_UpdateByte2, QuestRecord_Fill,
  Ending_DrawSpriteF). The reverse also happens: a raw sub_main address
  read below a stack store matched only as a named object
  (StageObj_K05_Draw, StageObj_K0E_Draw, game).

- **A raw address read kept in source order in yours, moved in the
  original.** Cause: `*(T *)0xADDR` is a pointer read that may alias a local
  whose address is taken, so it stays in place around that local's stores;
  a named global moves freely. Fix: look the address up in
  config/symbols/*.txt and use the object. Seen in: Chara_DrawEquipment and
  func_006BDA00 (select); CardGame_AskQuit (sub_main, round 25), whose
  `*(u8 **)0x8B43F0` was the block's whole difference: gSubMenuWork
  matched it.

- **A function copied from another unit with the game's callee inlined.**
  Galex_SetupQuest (plsel) inlines Quest_LoadTargets: a static inline copy
  of the callee in the file gives the game's code.

- **A raw address that a name schedules differently.** A cast of the named
  object's field address (`*(s32 *)&gQuestRecord.unk08`) schedules like
  the raw address, so the name can stay. Round 22 named 196 of the C's 224
  raw RAM addresses this way or plainly; four of the six functions listed
  as "must stay raw" now use names.

- **`addu v, idx, base` in the game (the index added first), the base first
  in yours.** A `static inline` accessor that returns the base (`static
  inline PageObjTbl *Page_ObjTbl(void)`) makes MWCC load the base before
  the index and add the index first, and loops keep their folded walkers.
  A cast pointer gives the same order outside loops but breaks the walkers.
  Seen in: Page_DrawDropList (lobby, 870 words: 23 words to 16). A scan of
  the lobby's blocks for swapped `addu` operands found one more
  (DlgInput_Draw, 1 word).

- **A union member's fields addressed from the symbol (`lw v, %lo(gMenuWork+0x24)(at)`)
  where the game keeps the base in a register.** With a shared work area
  declared as a union (MenuWork: win, prize, shop, hair, contest, raw),
  `gMenuWork.hair.field` makes MWCC fold each field into the symbol's
  relocation, while the game loads the base once. Keep a pointer to the
  member made by a cast of the union's address
  (`HairMenuWork *w = (HairMenuWork *)&gMenuWork;`), with a comment.
  Seen in: HairMenu_MovePlayer and HairMenu_Update (lobby unk_6F2610.c,
  round 31).

- **A field's offset folded into the address (`lbu v, %lo(gSave+0x1234)(t)`)
  where yours loads from the struct with the offset in the load.** An
  array field written `gSave.recipeBits[i]` or `&s.arr[i]` puts the field's
  offset in the load's immediate; the game folded it into the symbol (the
  old D_ label at base + offset). Index from the field as a pointer:
  `((u8 *)gSave.recipeBits)[i]`, `((u8 *)&gPlayers[0].area)[i *
  sizeof(Player)]`. In a one-line function a local view pointer changes the
  code: write the cast at the use. Seen in: the save's recipe and monster
  note bits, EmScript_Op06's player area (round 31, the types lane).

### Loops and pointer bumps

- **A loop unrolled 8 times with a remainder loop in yours, not in the
  original (or the reverse).** Cause: MWCC unrolls simple counted loops 8
  times at -O3,p (a zeroing loop `used[j] = 0` too) and fully unrolls small
  constant counts (2, 3: `k = 0; for (i = 0; i < 3; i++) k += a[i];` starts
  from its 0 with `addu rX, zero, rX`); a bound in a plain
  `extern` scalar allows it and a struct member reloaded every iteration
  doesn't; `for (j = sum = 0; j < n; j++)` keeps it unrolled once where
  `j = 0, sum = 0` unrolls it; `do { } while` isn't unrolled; `s += *buf++`
  unrolls with two loads per pointer step where `buf[i]` gives an indexed
  body. Fix: change the bound, the init expression or the loop kind. A loop
  whose 4 or 8 copies each step the pointer (not folded into offsets) was
  unrolled in the source. A word copy with both pointers stepped by 8
  (pairs of `lw`/`sw`, the first pair folded into the destination's
  offsets) is `for (k = 0; k < 6; k++) *d++ = *src++;` on s32 pointers,
  unrolled by two; a copy written `*obj->s2 = c; obj->s2++;` unrolls 8
  times like the game's. Seen in: func_003D6990, func_003B09C0 and
  func_003B0AF0 (sub_main), ScreenTex_Reserve (main), func_00860E20
  (game), LobbyWorld_GetTime and DlgInput_Draw (lobby). Round 9: a 4-pass fill
  loop `for (i = 0; i < 4; i++)` was unrolled with a u32 i and not with an s32
  one (func_006F6DF0, lobby).
- **A loop unrolled in the original (by 7 for 21 passes) that stays a loop
  in yours.** Cause: MWCC unrolls only a body under a size limit
  (mwccps2's message: "IsLoopUnrollable:No because loop size greater than
  threshold"), and the limit counts the body's intermediate code, casts
  included. Fix: shrink the body: chained stores (`rec->scale[k] =
  b->scale[k] = a->scale[k] = 1.0f`), pointers stepped with `a++; b++`
  instead of indexed, reads without casts. The pragmas `opt_unroll_count`,
  `opt_unroll_instr_count` and `opt_full_unroll_limit`, and level 4, don't
  change it. Seen in: Player_InitRecords (sub_main, was func_00337D30,
  parked from round 9 to round 16).
- **A later loop's counter or pointer in other registers.** Cause: the
  locals the loop shares with the rest of the function. Fix: try the
  function's counter reused for the loop with a pointer local of the
  loop's own. Seen in: func_00328CD0 (sub_main; its last loop: `i` reused,
  its own `AreaTri *tri`).
- **A loop rotated (the test at the bottom) in yours, not in the original
  (or the reverse).** Cause: `while (c)` and a `for` with an init expression
  are rotated; `for (;;) { if (c) break; ... }` and `for (; i < N; ...)`
  keep the test at the top; `for (i = i; ...)` gives a `for` an init
  expression; an `__asm__` block in the loop stops the rotation.
  `#pragma opt_rotateloops off` fixed AveTcp_Send's outer loop
  but, set for the whole file, broke AveWrap_Bind; switching it back with
  `reset` after the one function is untried. An out-of-line preheader (after
  the function's return) with a `b` to the test is an index loop whose
  pointer MWCC strength-reduces (`while (up[i] != 0)`); the pointer form
  `p = &up[i]; while (*p)` duplicates the test instead. Seen in:
  Sel_DrawHeader (select), unk_339EC0.c and unk_2F17C0.c (sub_main),
  WordFilter_Scan (lobby).
- **Another loop branch (`bnez` against `bgtz` or `blez`).** Cause:
  `do { } while (--n);` gives `bnez`, `n--; } while (n != 0);` gives `bgtz`
  with a signed n; `for (i = 10; i != 0; i--)` gives `bnez` with a `u32 i`
  and `bgtz` with an s32; `if (--n > 0) goto retry;` gives `blez`. Other
  count-down forms: `for (i = n; i != 0; i--, p++)`,
  `for (; n > 0; n--, p++)`, `while (n-- != 0)`. Seen in: unk_3549F0.c and
  unk_3DDBA0.c (sub_main).
- **A pointer step as its own `addiu` in the original, folded into the load
  offsets in yours (or the reverse).** Cause: MWCC folds `p[2]` and
  `p += 2; *p` into the offsets; post-increments (`a = *p++; b = *p++;`) and
  steps inside an expression (`*(p += 2)`, `(++p)->x`) keep their `addiu`,
  but which one does varies: try each. Forms that kept it in round 7:
  `x = *(p += 2);` when p is used after; `s.u1 = (++uv)->u; s.v1 = uv->v;`;
  `t += i; x = *t;` (`t[i]` folds); `*p++ = d[0]; *p++ = d[1]; *p = 0;`
  (with `p += 2` after the stores it folds); `p[2] = *src++; p[1] =
  *src++; p[0] = *src++; src++;` for a walking byte copy.
  `for (...; i++, src++)` orders the increments differently from `src++`
  at the end of the body; a parameter can be the walker
  (`f = (u8 *)f + 0xC;`, `tim2 += Tim2_Align(...)`), which keeps the
  parameter's register where a separate walker swaps them. Round 8:
  `pos[0] = *f++; pos[1] = *f++; pos[2] = *f;` gives `addiu a1, a1, 8`
  before the third load; `p++; p++;` before a test is shared by two returns
  where `p += 2;` is folded into each; `for (...; p++, i++)` against `i++,
  p++` orders the increments; `st->tex[i++] = a; st->tex[i++] = b;
  st->tex[i] = c;` computes each address (`tex[i + 1]` folds them); `ks +=
  i; k = *ks;` puts the sum in ks's register, `k = *(ks += i);` in the
  index's. Seen in: Ending_DrawObjects and Ending_DrawPicture (plsel),
  Chara_FilterParts (select), func_003E55E0 (sub_main), EmScript_Op0F,
  func_006FCDB0, EmScript_Op17, EmFxSpark_Setup, StageObj_K0E_SetupParts
  and StageObj_K1C_NextStepD (game), LobbyLogin_DrawIcon,
  LobbyBgm_SelectEntry, LobbyMsg_DrawCountdown, Bmp_CreatePalette,
  Html_ParseLength and func_007CA280 (lobby), Tim2_GetPicture (main).
  func_007D44C0 and func_007D45E0 (lobby: a layout pointer stepped by 4
  twice between loads) fold in every form tried.
- **A strength-reduced pointer in yours, an index in the original (or the
  reverse).** Cause: MWCC strength-reduces `a[i]` in a loop to a pointer (for
  a member array, starting at the struct with the member's offset in the
  loads); a pointer made by casting an extern array isn't reduced; tables
  walked by index keep their reloads where pointer walkers share them; `for (j
  = 0xA3, i = 0; ...; i++, j++)` keeps j an index; indexing (`quad[j][0]`)
  instead of a walking pointer gives MWCC's pointer the low saved register;
  indexed stores `a[n++] = i` become a pointer that is stepped while n is kept
  too. A loop that recomputes `&gPlayers[i]` with a `mult` every pass has its
  counter increased by hand in each path (`i++; continue;`, `goto next` to a
  shared last increment), which keeps MWCC from treating it as an induction
  variable. Fix: switch between index and pointer (write the index form for
  `a[n++]`), or step a pointer to `&t->need[0]` by hand; where the struct type
  lacks the member array, a view struct with padding up to it gives the
  member's offset in the loads. A loop-invariant pointer init (`r =
  &up[i]` before an inner loop) is hoisted out of the enclosing loop;
  indexing (`up[j]`) gives a pointer set up in the inner loop's
  preheader. Strength-reduced pointers take the lowest free temporaries.
  To keep the base rematerialised each pass, index by a byte offset
  (`note = D_00896FA0 + (u32)(i * 4);` with `u8 D_00896FA0[]`); every
  subscript spelling became a reduced pointer. A dead counter kept next to
  a reduced pointer is a variable also used by an earlier switch in the
  loop (`j = w->unk06; switch (j)`). Seen in: func_006DABF0,
  QuestList_Build, func_00797040 and WordFilter_Scan (lobby),
  Plsel_CheckAwardsOffline (plsel), EmChameleos_StealItem,
  func_007037F0, HuntingHorn_CheckSongs and StageObj_K0E_RunParts (game),
  unk_3E0EC0.c (sub_main). Round 9: MWCC strength-reduces `r[i]` of a pointer
  held in a register into a walker; the game's base-plus-offset walk (an
  `addu` of base and offset at each use) came from reading the route pointer
  through memory at each use, `(*(NpcRoutePt **)(w + 0xC))[i]`, which MWCC
  hoists itself (func_0077DA10, lobby); `refs[i]` indexing, not a walker set
  in the `for` init, gave the prologue's loop guard.
- **The first entry of a table loaded by absolute address before a loop,
  then a pointer in a saved register.** Fix: a typed table (a u16 or struct
  array, not a cast of a u32 array) and the first entry read into a local
  before the loop: `v = *p; while (v != END) { ...; p++; v = *p; }`; the
  order of the initialisers (`u16 i = 0; u16 *p = T; u16 key = T[0];`) picks
  the delay-slot filler. For a table of records, `r = T; if (r->type !=
  0xFF) { do { ...; r++; i++; } while (r->type != 0xFF); }` (a `while` or
  `for` loads the first entry through the pointer). A pre-test that reads
  the table by its symbol while the body reuses the value: `if ((v =
  T[0].weight) != 0xFFFF) do { ...; tbl++; } while ((v = tbl->weight) !=
  0xFFFF);`. Seen in: ItemList_IndexOf (sub_main), Plsel_RollBonus and
  Plsel_CheckRankAwards (plsel), Monster_AllHunted, ArmorForge_LoadMakePage
  and Form_Submit (lobby), func_00796FA0 (game).
- **The loop's first test before the pointer set-up, or a duplicated test.**
  Fix: `i = 0; if (i < n) { p = T; do { ...; p++; i++; } while (i < n); }`;
  an `if` and a `do-while`; `goto` retry loops; records walked by index
  (`i++; p = base + i * 8;`) rather than by a byte offset; a second counter
  incremented at the top (`i++; if (i > n) break;`); `i++; continue;` in
  `while (1)` gets its own `b` to the top. A walker MWCC makes
  (`G[n++] = v`, or a table walked by index, `p = T + i * 0x14A8;` in the
  body) is set up after the loop's first test; a pointer local set before
  the loop, or in the `for`, before it: write the index form (`c[n]`) for
  the first. The `if` and `do-while` form also puts `i = 0` in the test's
  delay slot and the preheader's set-up (`tx = ...`, `m = 1`) after the
  zero-trip test, where a `for` puts it before. A loop from 1 with the
  first iteration written out (`D[0][1] = 0; D[0][0] = 0; for (i = 1; ...)`)
  gives a first page through `%lo` and the rest through registers; a loop
  that tests `i < i + count` first and then reloads the bounds through a
  register is `i = G.first; a = &G; if (i < i + G.count) do { ... } while
  (i < a->first + a->count);`. Seen in: Psp_LoadFiles (select), Adx_Init
  (main), Talk_FindScript, func_00770250, Npc_Draw and
  ArmorForge_DrawMakeList (lobby), func_00330570 and
  func_0033CDA0 (sub_main), Eff06_Scatter (game). Round 9: a loop whose first
  test is on a call's result (`slti at, v0, K; beqz` with `move s4, v0` in the
  slot) is `if ((n = f()) < K) { do {...} while (++n < K); }`
  (Plsel_BuildResultText, plsel); an entry test that reads the table through
  `%lo` before the walker is set is `p = D; if ((c = *p) != 0) { do {...}
  while ((c = *p) != 0); }`; pointers set before the
  zero-count test: `if (n != 0) { l = ...; do {...} while (n != 0); }`
  (LayerCallbacks_Run, sub_main); `j = 0` in the entry test's slot and the
  walker's `move` after the branch: `if (0 < n) { j = 0; e = part; do ...
  while (j < n); }`, or MWCC's own walker (`part[j + 2]`) (func_00373D30 and
  func_0036F460, sub_main).
- **Eight (or seven) `nop`s as a loop's body, or a branch-layout
  difference around a wait loop.** Cause: inline asm in the game:
  `__asm__ volatile ("nop\n nop\n ...")` in the loop gives the branch to
  the test, then the nops. A wait of the wrong length looks like a
  branch-layout problem. Fix: count the nops between the label and the
  next C statement in each loop; the alignment nop before an 8-aligned
  label is not part of the asm. Seen in: Yn_UnloadModule (yn, eight),
  Yn_LoadModule (yn, seven in its retry loops), and in round 9 Net_LoadIrx
  (sub_main, was func_00309D00: Yn_LoadModule's form, seven in both retry
  loops, with a dead `i = 0` on success).

- **A loop bound recomputed, or in another register.** Fix: the bound as a
  local set in the for initialiser (`for (k = ..., end = k + 8; k < end;
  k++)`, `s32 end` declared before k). Seen in: QuestGoal_Complete
  (sub_main).
- **A loop's walker set before the pre-test in yours, after it in the
  original.** Fix: index the array (`r[i]`) instead of stepping a declared
  pointer; MWCC then makes its own walker after the test. Seen in:
  func_0077DA10 (lobby, 4 words to 2).

- **Walkers bumped in another order.** MWCC bumps the walkers it makes
  first, then the for header's explicit steps in source order. Fix: step
  the others explicitly and index the one the original bumps first
  (`for (i = 0, st = D_00D78C60; i < 17; i++, tb++, st++)` with `v[i]`).
  Seen in: Plsel_ComputeRewards (plsel).
- **A loop's base in the lowest register in the original.** Fix: read it
  through memory at each use (a macro over the struct field): it becomes a
  value MWCC makes, numbered after the walkers. Seen in: func_0077DA10
  (lobby).
- **A scaled index live across an if or a loop in the original.** Fix: an
  explicit byte offset (`s32 o = i * 2`) used for one access. Seen in:
  Rand_NextSaved (sub_main, FAKEMATCH).

- **Walkers set up after the loop's guard in the original, before it in
  yours.** Cause: a hand-written walker or pointer local, often from an
  earlier draft. Fix: index the arrays (`dur = tbl[i]`, `o->unkA + i *
  0x5555`) and let MWCC create the walkers, after the guard. Seen in:
  ModelObj_Run and Offer_Generate's records (lobby), AreaGrid_TestSegment,
  AreaGrid_SweepCells, Hud_DrawGaugeFrame and Hud_DrawClock (sub_main).

- **An index computed each time in the original, a strength-reduced pointer
  in yours.** Fix: an unsigned index (`levelSize[(u32)i]`); MWCC doesn't
  strength-reduce it. Tim2_GetImage (sub_main) went from 42 words to 1.

- **`bnez x, body; b exit; body: b loop` in the game's loop, a tight loop
  in yours.** One source: an unsigned compare in a search loop (`(u32)id ==
  *p`, PriceInfo_Open, lobby). It doesn't explain NameList_DropFirst's loop
  (lobby, 4 words): open.
- **A search loop's preheader out of line, with a load hoisted into it.**
  Write it as `if (col == K) { } else { do { ... } while (col != K); }`.
  With a `while` after the empty test, the load stays inside the loop; a
  plain `while` hoists it, but the preheader is in line. Seen in:
  Psp_ConnectTask's column searches (select).

### Struct copies and clears

- **An inline byte-clearing loop with a NULL test on the address and no
  loop-head alignment.** Cause: `T x = {0};` on a local struct or array; a
  hand-written loop is aligned and unrolled instead. Seen in: the CSE
  requests (main), Model_BuildVifPackets (sub_main).
- **The copy instructions of a struct assignment differ.** Cause: MWCC picks
  them by alignment and size: `lq`/`sq` when 16-aligned (a global it knows is
  16-aligned on both sides, or `extern T x __attribute__((aligned(16)));`, or
  a struct type declared with the attribute), an `ld`/`sd` loop when 8-aligned
  (an s64 member), unrolled `lwc1`/`swc1` groups when 4-aligned up to 0x30
  bytes whatever the field types and an `lw`/`sw` loop above that (tested;
  round 6 had written 0x20, but 0x24 and 0x30 give `lwc1`/`swc1` too), an
  `lh`/`sh` loop when 2-aligned (s16 and u8 members only), a byte loop when
  1-aligned; a 64-byte matrix is an 8 x 8-byte loop, and a struct-typed global
  copied into a local mixes them by size (0x1C bytes: `lq`, `ld`, `lwc1`). An
  `lw`/`sw` copy of 8 to 0x30 bytes in the original is the fields assigned one
  by one. m2c writes struct copies out field by field: `r[i - 1] = r[i];` with
  a record type of the right size and alignment gives them back. Fix: give the
  global the struct type (a copy through a cast pointer doesn't do this), add
  or remove the aligned attribute, copy through `*(T *)((u8 *)table + 0)` to
  lose a 16-alignment. MWCC must know the source is 16-aligned: a single
  struct object or a defined struct array gives `lq`/`sq`; an extern array
  of unknown size, a cast of a u32 array, or a cast to a typedef with
  `aligned(16)` doesn't (a global declared with that type does). A struct
  global (not an array) of 12 bytes copied into a local gives `ld` and
  `lwc1`. Seen in: func_0036C560 (sub_main), Plsel_DrawPlayerList (plsel),
  Yn_DrawProxy (yn), Pad_ReadPort and Gs_SetState (main), func_00757CB0,
  func_00757EC0, func_007C4F80, Page_ScrollInput and Dialog_MenuStep
  (lobby, 16-aligned stick tables), Gallery_DrawScrollBar (select).
- **A fully unrolled copy against a loop, or all loads before the stores.**
  Cause: a 12-byte copy unrolls a loop (`for (i = 0; i < 6; i++) dst[i + 1]
  = src[i];`); a Vec3 assignment loads all three floats before storing where
  three float assignments interleave; a 4-byte struct of bytes loads the
  four bytes first; three `lwc1` then three `swc1` are a struct copy
  (`*(Vec3f *)p->pos = *(Vec3f *)p->dest;`). Seen in: EquipView_FromPlayer
  (sub_main), func_00705860 and TownMsg_OnMove (lobby).
- **A byte copy that copies the source pointer before each `*src++`
  (`move a3, a1; lb t1, 0(a3); addiu a1, a3, 1`), while the destination's
  steps fold into offsets.** Cause: the byte read is an assignment inside an
  empty `if`'s condition. A post-increment evaluated in a condition (an
  `if`, a switch's controlling expression) is materialised as `tmp = p; p
  = tmp + 1; *tmp`, and the copy survives unrolling; in a plain statement
  MWCC folds it. Fix:
  `for (i = 0; i < n; i++) { if (c = *s++) {} *d++ = c; }` with `s8 *s`
  (the byte type picks `lb` or `lbu`; `if ((c = *s++) != 0) {}` is the
  same). `(void)(c = *s++)`, a comma expression, `*d++ = (c = *s++)`,
  `register`, casts and an `__asm__` in the body don't do it; `if (*d++ =
  *s++) {}` copies both pointers. The script commands' `switch (*p++)`
  (game) is the same shape; a backward copy uses `s--`. Seen in: Mem_Copy
  (was func_0017FEB0) and Heap_Move (main), the 12 script commands (game,
  round 7).
- **A record copied to the stack and read through a register holding the
  copy's address.** Fix: `*(rp = &r) = table[id];`, then `rp->f`
  (`r = table[id]; r.f` addresses the copy from sp). Seen in:
  Gallery_DrawPicture and CharaSel_DrawEntry (select).
- **`sq`/`lq` of a 128-bit value.** Fix: `typedef unsigned int u128
  __attribute__((mode(TI)));`. Seen in: dma.c (main).

### Float code

- **Float constants.** MWCC builds them with `lui`/`ori`/`mtc1`, never from
  `.rodata`, so float code needs no data split. A constant stored without
  arithmetic goes through an integer register (`lui v0, 0x4120; sw v0, ...`),
  so `*(f32 *)&x = 10.0f` and an int store compile the same. A float zero
  store is `sw zero`, like an int's; it is `swc1` only from an FPR that
  already holds 0.0 (after a compare with 0.0f) (tested; earlier notes said
  `swc1`, round 7's matches showed `sw zero`). An `f32[3]` set field by field
  gives `sw zero` and `lui`/`sw`; an initializer (`= {0.0f, 20.0f, 0.0f}`)
  gives other code. A constant used in both arms of an if and after the
  join stays in one FPR when a local is set to it in each arm (`turn =
  65536.0f;`). Seen in: the families' state setters, func_0085E400,
  Player_State0_34 and func_006F2360 (game), unk_2E6F40.c (sub_main). Round 9:
  constants one bit above the nearest float (0x3C23D70B for 0.01, 0x39D1B718
  for 0.0004) are folded quotients, `0.3f / 30.0f` and `0.4f / 1000.0f`
  (func_003C8EA0, sub_main).
- **`mul.s` or `add.s` operands the other way round.** Cause: in a plain
  product of a float value, a literal is the first operand of `mul.s`
  (`0.15f * x` and `x * 0.15f` alike). The in-place forms decide
  otherwise: `x *= K` keeps x first (`x *= K;` then `(s16)x`, where
  `(s16)(x * K)` puts K first), and with an int converted to float, `w =
  len; w *= 9.0f;` gives (len, K) where `w = len * 9.0f` put the literal
  first, while `pan = x - 320; pan = K * pan;` gives (K, pan) where `(x -
  320) * K` and float locals didn't. Otherwise the source order and the
  form decide (`x += K` keeps x first,
  `t = x + K; x = t;` puts K first; `d + b` with `d` a local holding a
  call's result differs from `f() + b`; `f = a / b; f += c;` gives
  `add.s f, f, c` where `a / b + c` and `c + a / b` put c first). Fix: a
  float local holding the constant (`rate = 0.15f; ... hpMax * rate`,
  `f = 2.0f; f = f * D_008BDC08;`; the local also keeps the constant in its
  register for later compares, and a later value assigned to it takes the
  same register; `f32 d = 155.0f; ... v.x * d`), swapped operands, the
  compound or the plain form. Seen in: EmChameleos_SetGroup1State (game),
  func_003B0DF0 (sub_main), func_006F6870, LobbyWorld_AddDays,
  LobbyBgm_EndsToday and VFx_SpawnShot (lobby), func_00D6CCC0 (plsel),
  func_006C4B70 and Sel_DrawHeader (select).
- **Float registers rotated between temporaries.** Cause: m2c's temps
  (`temp_f2 = em->unk574;`) fix the order of the loads, while fields written
  at each use give the order of the uses; a local changed in place
  (`speed *= K`, `f = *p; f *= q;`, `c = len; c *= 9.0f; c = 640.0f - c;`,
  `f = (f32)t / (f32)n; f *= 86400.0f; return f;`) keeps its register and
  converts straight into it; one float local reused for two values puts the
  second in the first's register. Fix: write the fields directly, split
  into in-place steps, reuse one temp (`h = em->unk574; h = em->posY -
  (1000.0f + h);`; `a *= unk6E4; a *= -1 + 2 * unk30;` keeps a in f20), or
  reorder the float statements (y before x). Seen in: GmEm_Update
  (gm_sub), EmGaruga_SetGroup3State, the four State2_01 landings and
  EmFxTri_Draw (game), Plsel_DrawAwardText (plsel),
  LobbyWorld_TicksToSeconds, DlgPage_Draw and Select_ListClick (lobby).
- **A float added to an s16 field in another order.** Cause: `x += (s16)f`
  gives `lh; cvt.w.s; mfc1; addu; sh` with the loaded value first;
  `x = x + (s32)f` and `(s32)f + x` order the registers or operands
  differently, and `x += (s32)f` extends the converted value. Seen in: the
  monster families (game, round 4).
- **The unsigned int-to-float sequence in yours where the original converts
  signed (or the reverse).** Cause: any unsigned value converted to float
  (u32, u16, a u8 field or a u8 local) gives the unsigned sequence
  (`bltz; srl; andi; or; cvt.s.w; add.s`); the game has both. Fix: `(s32)x`
  for a signed conversion (`1000.0f * (s32)x`), an explicit `(s32)` when
  storing a float into u8 fields; write m2c's spelled-out sequence back as
  a cast. `t *= f;` on a u32 t converts in source order, where
  `t = t * f; t += ...` converted the second value first. A u16 call result
  converts unsigned too: the game's signed `cvt.s.w` alone is `(s32)` of it
  (`6.2831855f * (s32)Rand_Next(1)`); a u16 that the game converts as an
  unsigned int is a `u32` parameter. Seen in: VObj_Spawn, LobbyWorld_GetTime
  and LockOn_SetSector (lobby), movie.c and func_0031D7E0 (sub_main),
  ColorEdit_Draw (main), StageObj_K1B_Setup, func_006C4410 and
  func_00865F20 (game).
- **A float-to-unsigned conversion (`c.le.s` against 2^31, `cvt.w.s`, then
  `or 0x80000000`, the constant 0x4F000000).** Cause: a float assigned to a
  `u32` local (or cast `(u16)(u32)`); `(u16)(s32)x` gives `cvt.w.s` only,
  and `u16 r = (s16)(u16)(s32)(x);` gives `cvt.w.s; andi; sext; andi`. Seen
  in: func_0070B900, the em files and unk_6EC680.c (game),
  LockOn_SetSector (lobby).
- **`trunc.w.s` in our objdump listing, `cvt.w.s` in splat's.** It is the
  same instruction (0x46000024; the EE's `cvt.w.s` truncates), and MWCC
  emits it: not a compiler difference (round 4's `mwcc_fixup.py` changed
  nothing and is gone). `(u16)(s32)(expr)` gives `cvt.w.s` and `andi`;
  `u32 a = 255.0f * f;` then `(u8)a` at each use gives a conversion once and
  masks.
- **A float constant converted to int at run time (`lui; mtc1; cvt.w.s; mfc1`)
  inside a loop.** Cause: MWCC folds `(s32)14.0f`, and hoists a float
  variable's conversion out of the loop. Fix: in a level-4 file, a float local
  converted at each use to a narrow type (`x += (s16)space;`). Round 4 took
  this for another compiler. Seen in: Ending_DrawTitle (plsel).
- **One conversion or negation where the original has several (or the
  reverse).** Cause: `x / (f32)n` written three times converts once, and an
  `f32 fn = n;` local converts earlier and in another register; `-hw` of a
  local set to 320.0f is a run-time `neg.s` where `-320.0f` folds. Seen in:
  func_00795620 (lobby), Frustum_Build (sub_main).
- **`mula.s` and `madd.s` in the original.** Cause: MWCC emits them for
  `a * t + b * s`: plain C (m2c can't read them, and round 3 took them for
  VU0 code). Indexed 2D weight tables (`D_008CDA90[k][0]`) give the game's
  `madd` operand order where pointer walkers don't. Seen in: Vec_Lerp
  (sub_main), func_00861B70 (game). Real VU0 code (`vadd`, `lqc2`...) is
  asm (last section).
- **`c.eq.s`, `c.lt.s` or `c.le.s` operands swapped, or the branch
  inverted.** Cause: `if (x)` on a float gives `c.eq.s f1(x), f0(zero)`;
  `x != 0.0f` swaps them. `K < x` gives `c.lt.s K, x` where `x > K` gives
  `c.le.s x, K` with the branch inverted; `if (!(0.0f <= y)) A else B`
  gives `c.le.s 0, y; bc1t B`. Seen in: qnet.c (gm_sub, the ping test),
  the flying states 3_08 and 3_15 of Kut-Ku and Rathalos, func_006F5740
  (game).
- **A minimum or maximum search that branches and loads like the
  original.** Fix: `if (best == 0.0f) best = d; else if (d <= best) best =
  d; else continue;`; over a few values, a temporary per compare (`t =
  dist[1]; if (best < t) { best = t; k = 1; }`), since `if (best <
  dist[1])` loads the value twice. Seen in: unk_3DDBA0.c (sub_main),
  EmKutku_SetEscapePoint (game).
- **`sqrt.s` (objdump: `c1 0xc0044`) between `nop`s, with no call.**
  Cause: `f32 sqrtf(f32);` is an intrinsic: declared so, MWCC emits
  `sqrt.s` itself (tested). Seen in: unk_2F7420.c (sub_main).

### Stack slots and frame size

- **Locals at other stack offsets.** Cause: locals in memory are grouped by
  size, the smallest at the top of the frame: objects of 16 bytes or more at
  the bottom, then those of 8 to 15, then 4-byte ones, then 2-byte ones
  (tested with arrays, structs and scalars of those sizes). Within a group
  they take their slots from the top down in declaration order: the first
  declared gets the highest address. So a large array is below the small
  locals wherever it is declared (round 7's "the largest array takes the
  lowest slot" and "two same-size arrays in reverse order" are both this).
  Fix: reorder the declarations within the group; change a size (`f32 v[3]`
  against `f32 v[4]`) to move a local between groups. Two locals of one
  kind that the game places next to each other with no gap are one array
  (`PspTri t[2]`; two locals got an 8-aligned gap). Seen in:
  Bone_SetMatricesRel (sub_main), func_0070AFE0, EmKutku_SetEscapePoint and
  Player_State2_02 (game), func_0079C360 and Layout_SpreadColumns (lobby),
  func_006C5ED0 (select). Round 9: stack arrays and spilled locals follow the
  same rule, the last declared at the lowest address (Npc_DrawProps, lobby).
- **A frame 0x30 bytes bigger than yours, or a lone f32 at another offset.**
  Cause: a 4x4 matrix (`f32 m[16]`) where m2c declared `f32[4]`; a local
  whose address is passed as a position is a vector (`f32 v[3]`; a lone f32
  lands at 0x3C instead of 0x30); a library struct passed by address is
  bigger than declared (scePad2SocketParam is 0x20 bytes); a 16-byte gap
  below the locals is a `NetMsg` received by value, even unused. Seen in:
  func_006F9BF0 (game), the landing states of Garuga, Gravios and Kut-Ku
  (game), unk_18AFE0.c (main), func_007C17C0 and LobbyRoute_6191 (lobby).
- **A parameter stored to its home slot above the frame (`sw a1,
  0x3C(sp)`).** Cause: the parameter's address is taken
  (`memcpy(buf + 0x1C, &x, 4)`). Seen in: func_0079F210 (lobby).
- **A small local array kept on the stack with a pointer per element, or
  kept in registers.** Cause: `u8 c[4]` filled by a loop (fully unrolled)
  stays in memory; four assignments or a struct of four bytes keep the
  bytes in registers. `volatile` on an uninitialised local gives a read
  from its stack slot. Seen in: LobbyWorld_GetFadeColor (lobby),
  unk_308BC0.c (sub_main).
- **A frame that fixes a buffer's size.** A frame of 0x120 around one buffer
  at sp+0x10 needs a buffer of 0x101 to 0x110 bytes (`u8 buf[0x100]` gives
  0x110). Seen in: the HTML tag handlers (lobby).
- **Two locals share one stack slot in the original.** Cause: MWCC shares a
  slot only in some cases: clearing the first bytes of a `NetMsg` with
  `memset(&m, 0, 6)` gave the original's shared slot; a reply callback that
  writes a small result into the message's spare bytes
  (`func_00717C50(id, &a, (u16 *)&m + 3)`) uses the NetMsg's slot where a
  separate local gets its own. Block scopes and declaration orders didn't
  help LobbyReply_6912, LobbyStep_6884Done and LobbyLogin_OnSummaryReply
  (still unmatched). Seen in: LobbyNotify_68B0 and LobbyLevel2_On6412Reply
  (lobby).
- **Another value spilled (`sw`, then `lh`) than in yours.** Fix: change
  which value lives long: Yn_DrawHelpBar matched at level 4 with its height
  an s32 holding `(s16)f()`, passed back as `(s16)h`. `if (c) d = 4; else d
  = 10;` let an s16 d be spilled where the ternary didn't; a value set right
  after an if-chain (`mesh = ...`) so that another dies earlier freed a
  spill (frame 0x170 to 0x160); `al = a * al` instead of a new `alpha`
  gave a web spilled alone. A spilled s32 passed to s16 parameters is `sw`
  then `lh`. A value used before and after a loop written as the same
  expression (`xs + 0x14`), not a local, gets MWCC's temporary the spill
  slot below the loop's hoisted ones. Seen in: Yn_DrawHelpBar (yn),
  func_006D9090, func_0070F580 and func_0070F910 (lobby), EmFxTri_Draw
  (game). StageObj_K1C_RunChain and EffEm33_RunChain (game: a pointer
  spilled 12 bytes off) are still open.
- **Fields of a stack struct addressed through saved registers
  (`addiu s6, sp, 0xAC`, then `sh v0, 0(s6)`).** Cause: a plain local struct
  with many stores and free registers; pointer variables to its fields
  (`px = &part.x`, `u8 *type = &tmp.type;`, `f32 *dy = &d[1];`) stay in
  saved registers across calls. Where the pointer is set matters: inside
  the block that uses it, or after the first call (at the top, its `addiu`
  comes first); `v[0] = a; vy = &v[1]; *vy = b;` loads the constants one
  at a time. A local struct `{f32 x, y, z;}` instead of the array doesn't
  do it. A struct filled by a copy call (`func_17FEB0(&msg, data, n)`) is
  read through `q = &msg;` assigned after the copy (at its declaration,
  or with `msg.x`, the fields are addressed from sp). Seen in:
  Plsel_DrawFrame (plsel), Title_DrawLogo (select), ArmorForge_MakeDetail,
  WeaponForge_BoxInput, func_006AF950, func_006AFB10, TownMsg_OnMove and
  TownMsg_OnActor (lobby), Slab_Contains, Seg_NearestPoint and
  unk_3E0EC0.c (sub_main), func_006E9B90 (game).

- **A stack slot 12 bytes off.** Cause: a spilled variable gets a packed
  4-byte slot, a compiler temporary a 16-byte one. Seen in:
  StageObj_K1C_RunChain and EffEm33_RunChain (game, NON_MATCHING; the
  permuter doesn't see stack offsets).

- **A spill in a 16-byte slot 12 bytes lower in the original.** Cause: the
  value is an inlined function's return value, which MWCC keeps in a
  16-byte compiler-temporary slot. Fix: a `static inline` helper that
  returns it (`return Em_Bone(em, 0x25);`). An inlined helper's return value
  also explains a sign extension that a plain expression doesn't produce.
  Seen in: EffEm33_RunChain, StageObj_K1C_RunChain (game), Kitchen_EatDish
  (lobby, Kitchen_DishRank).
- **A stack-passed parameter (the 9th or later) copied to the stack in the
  original, not in yours.** Fix: type it u32 rather than s32.

- **Two spill slots swapped, the code otherwise right.** Cause: MWCC's
  created extension of a narrow value takes its slot in the wrong order.
  Fix: a declared s32 copy (`s32 cnt = n;`), declared just before the other
  spilled variable and used in every compare. Seen in: EffEm33_Run (game:
  the parts loop's s16 bound).

- **A spill in a 16-byte slot in yours, a packed 4-byte slot in the game.**
  Cause: a pointer cast to an integer at a call argument makes MWCC treat
  the variable as a compiler temporary. Fix: pass the pointer itself (and
  give the callee a pointer parameter). Seen in: Em_CarryRiders (sub_main,
  `(s32)em` at Em_PosInFrame's call).
- **An extra spilled copy of a narrow parameter.** Cause: a local for part
  of an expression (`w / 2`) makes MWCC keep an extended copy of the
  parameter. Fix: write the whole expression at each use. Seen in:
  Menu_DrawQuestSheet (lobby: `x + w / 2`, 162 words to 18).

### Data and .rodata placement

The workflow is in CONTRIBUTING.md ("Functions with switch tables or
strings", "Data in C"); these are the symptoms.

- **A link error for an object after a file claimed its `.rodata`
  ("undefined reference").** Cause: splat migrates into a function's `.s`
  only the run of objects after that function's previous migrated object, so
  an object after a shared one lands in no `.s`, and an object no
  instruction names (16 zero bytes after some strings) isn't migrated
  either. Fix: an `INCLUDE_RODATA` line right after the function that emits
  the object before it; `force_not_migration` in the symbol file for a
  single-user object whose owner comes earlier in the text than the previous
  owner. A string right after the jump tables of C functions lands in no
  `.s` either: `INCLUDE_RODATA` it before its user. Seen in: unk_6EC680.c
  (game, D_008EC0A0), select_option.c (select), unk_74A2C0.c (lobby, a "").
- **A module 8 or 16 bytes too long or too short.** Cause: a literal of your
  own where the original shares one (while one user is still assembly, use
  the `INCLUDE_RODATA`'d name; with every user in C the literal is shared);
  a string that two of the game's units in your file both use (the game has
  one copy per unit: give the second by name, or as an array of its own,
  `const char D_00825EE8[] = "%d";`, where splat won't migrate it); the zero
  padding at a section's end or after a string (a data object of its own
  such as `const u8 D_00D7BD40[0x40] = {0}` after the function, or a
  `text_end` or `rodata_end` piece in the YAML); small objects aligned or
  merged differently. Examples: a string symbol that covers dropped strings
  ("%s%s" then two "", 16 bytes: the literal, then `const u8
  D_008DD028[8] = {0};` after the function); the zeros after an overlay's
  last jump table (`const u32 D_00849F60[8] = {0};` after its function);
  a literal followed by padding (INCLUDE_RODATA plus
  `force_not_migration`). The reverse: 4 zero bytes after a string that are
  only the 8-byte alignment mwcc_cc.py gives the next literal; defining
  them moves every later string by 8. Seen in: QNetIf_OnPlayerLeft
  (gm_sub), unk_6B9160.c, func_00775240, func_00770250, func_006FCEF0 and
  the village 2 files (lobby), unk_D74450.c (plsel),
  HuntingHorn_ShowSongMessage (game), Gallery_DrawEntryPage (select).
- **A string symbol in a function of a file that owns its `.rodata` doesn't
  link.** Fix: write the literal; an extern of splat's label fails once the
  function is C. File-scope const arrays used before any literal come
  first in the file (the PNG chunk names of unk_7513C0.c). Seen in:
  func_00729AC0 (lobby), SeasonBanner_Draw (sub_main: `"%s"`, and two
  strings joined by a full-width space).
- **A string before the jump table of a function that uses it later.**
  Cause: its first user was probably dropped by the linker. Fix:
  `INCLUDE_RODATA` it where it is. Seen in: unk_D67530.c (plsel, "%s"),
  yn_main.c (" " and "").
- **A function's jump tables in another order.** Cause: MWCC emits them
  last-created first. Seen in: func_00308450 (sub_main).
- **A relocation against `sym + offset` in the original, another label in
  yours.** Fix: give the data symbol a `size:` in
  `config/symbols/<module>.txt`, so that splat writes `sym + offset` (as a C
  struct access does); labels inside it then need no definition. splat reads
  the `// size:` only on the symbol's own line; a comment line above it has no
  effect. Seen in: config/symbols/yn.txt (yn), D_00589CC0 (sub_main).
- **A link error for another module's data.** Cause: an `extern` of a name
  that splat never saw in this module's assembly isn't defined anywhere.
  Fix: an address (`#define D_6AE024 (*(u32 *)0x6AE024)`) until the owner
  exports a name. Where the schedule needs a named object (see "An
  instruction or constant load in another place"), a name in the module's
  own symbol file (`D_006ADFD7` in lobby_browser.txt) or the neighbouring
  symbol with an index does it until then. Seen in: the lobby (sub_main's
  data), PortalEnd_Step0, LobbyWorld_Skip and func_007C2C80 (lobby:
  D_005BC4E8 and four more in lobby_online.txt).
- **A `%lo` paired with the wrong `%hi`.** Cause: GNU ld pairs a HI16 with
  the wrong LO16 when one symbol has two interleaved `%hi` values that
  differ by a carry. Fix: declare a second symbol for the other address.
  Seen in: unk_363550.c (sub_main, D_005BA4FC).
- **The last function of `.text` or of a file, and what follows it.** For
  the build, the zero padding after the last function of `.text` needs a
  `text_end` data piece in the YAML once the function is C (0x80 bytes
  after LobbyActor_Draw; 32 nops before sub_main's vu1_microcode; Yn_LoadModule,
  0x913C). Until round 25, func_diff read whatever followed a file's last
  function, up to the next function start it knew, as that function's tail,
  and reported a DIFF where the binary matched (Sound_ReadBank,
  LobbyActor_Draw, func_003E7E00, Yn_LoadModule, StageObj_BowArc_Create,
  Letterbox_Draw; a worker's script reverted good renames on it). Since
  round 26 the tail also ends at the next piece of the module's YAML.

### Optimization level 4 files

- **Symptom:** a near miss that looks like one more round of copy
  propagation: the first test of a rotated loop reads `buf[0]` through sp, a
  product is computed once where yours computes it twice (or the reverse),
  address temporaries and saved registers that no declaration order fixes, a
  float constant converted at run time, an index product (`i * 4`) hoisted
  into a second saved register at level 3. Compound assignments behave
  differently at level 4: `x += ...` kept a u16 in one register from its
  first value (Layout_SelectEnd), and `D->y += ...` computes the field's
  address once for the store (Page_PointTo). Round 9: spilled, CSE'd field
  addresses in saved registers or hoisted invariants (func_006FF990: 180
  words off at level 3, 16 at 4: try level 4 early for these), a u8 loop
  bound copied with `andi` in yours and kept in its register in the
  original (round 8's near misses in unk_722400.c), parameters extended
  late in place (func_0076F0E0).
- **Cause:** the game compiled some files at optimization level 4. `-O4,p`,
  `-O4,p -opt nointrinsics` and `-O3,p` with the pragma give the same code.
- **Fix:** `#pragma optimization_level 4` after the includes, with a comment;
  then check with `func_diff.py` that every function of the file that matched
  still matches. A file that needs both levels is often two of the game's units
  (but not always: "The level can change inside one of Capcom's units"
  below): split it there when other evidence agrees (round 7 split unk_333D70.c at func_00334260, now
  unk_334260.c at level 3); `#pragma optimization_level reset` before the
  level-3 part works in the meantime (Layout_TextFull's NON_MATCHING block
  is closer at level 3 and carries the pair around it). A pragma region in
  the middle of a file marks a unit of its own: in round 9 unk_6F8A10.c's
  level-4 functions 0x6FF630-0x701810 became unk_6FF630.c (with a
  `.rodata` split), the rest unk_701810.c at level 3. A pragma pair around
  one NON_MATCHING block keeps a draft's best level until its file is
  decided (func_0076F0E0 in unk_76DB40.c, lobby). `func_diff.py`,
  `near_misses.py`, `func_variants.py` and the permuter read the source, so
  they see the pragma.
- **Where to try it first:** the lobby's HTML parser and layout
  (0x741900-0x7513C0) look like one unit built at level 4, and the
  browser's portal and dialog code (0x72C7C0, 0x733AB0-0x7409A0 or more)
  another: for any near miss there, try the pragma before anything else.
  Neighbouring files of a level-4 file are candidates too (unk_18AFE0.c
  joined unk_189F90.c). A file whose functions all match at both levels
  goes to level 4 when one of them needs it (unk_733AB0.c,
  unk_734710.c and unk_73C310.c: Portal_ParseStep, Dialog_PageStep and
  Link_Follow match only at 4; in round 9 unk_73ACF0.c for
  Page_Navigate). In the game, unk_70A290.c's 0x722400 part and
  unk_6FF630.c are at level 4; unk_6AF280.c, unk_6EC680.c, unk_733EA0.c and unk_88E760.c match whole
  at both levels.
- **At level 4 now** (75 files, after round 20):
  dma.c, texconv.c, unk_1864E0.c, unk_189F90.c, unk_18AFE0.c (main);
  unk_2E3940.c, unk_2EF020.c, unk_308BC0.c, unk_314AD0.c, unk_314E00.c,
  unk_333D70.c, unk_33E620.c, unk_356380.c, unk_361570.c, unk_36BE70.c,
  unk_37B570.c, unk_39C290.c, unk_3A1070.c, unk_3AC9C0.c, unk_3B4C80.c,
  unk_3BE060.c, unk_3CD9C0.c, unk_3D4100.c, unk_3E0EC0.c (sub_main); ending.c,
  plsel.c, unk_D67530.c (plsel); select_gallery.c, select_title.c (select);
  yn_main.c (yn); em_bulldrome.c, unk_6CADF0.c, unk_6E3B00.c,
  unk_6FF630.c, unk_70A290.c (from 0x722400 on: one unit at both levels,
  below), unk_880D50.c (game); unk_6B0570.c, unk_6B5300.c, unk_6B5E70.c, unk_6B7830.c, unk_6B8AC0.c,
  unk_6B9160.c, unk_6BCA50.c, unk_6BF430.c, unk_6C8300.c, unk_6C9640.c,
  unk_6D5330.c, unk_6E56C0.c, unk_6E7100.c, unk_6E9310.c, unk_6ED140.c,
  unk_6ED9B0.c, unk_6EE870.c, unk_6F16A0.c, unk_6F4B30.c, unk_6F62D0.c,
  unk_6F8CC0.c, unk_6FB0B0.c, unk_6FF940.c, unk_7115D0.c,
  unk_71F8A0.c, unk_72C7C0.c, unk_733AB0.c, unk_734710.c, unk_73ACF0.c, unk_73C310.c,
  unk_741900.c, unk_748A90.c, unk_74A2C0.c, unk_74C620.c, unk_74D3F0.c,
  unk_753C10.c (zlib's inflate), unk_7683D0.c, unk_7949D0.c and unk_7BF560.c
  (lobby).
  Level-3 pairs inside level-4 files, kept as Capcom's own pragmas (next
  bullet): unk_33E620.c (Map_DrawZoomed, the zoomed twin of Map_DrawArea,
  between level-4 functions of the same map code) (sub_main); unk_880D50.c
  (StageObj_K1C_NextStepD, one of the K1C family's step helpers, called by
  the level-4 StageObj_K1C_RunSeq) (game). unk_74A2C0.c (around
  Layout_TextFull's NON_MATCHING draft) (lobby).
- **The level can change inside one of Capcom's units** (round 18), so a
  level change alone doesn't prove a unit boundary. The proof:
  Player_PickStatusState (0x70DFD0) has one caller, Player_Update
  (0x7227F0), which keeps a0 across the call.
  With the callee defined `static` before it, Player_Update goes from 47
  words to 24 and the a0 reload is gone; defined non-static it stays at 47
  (see "Arguments kept or reloaded across calls (static callees)"). A static callee is in its caller's unit, so
  0x70DFD0-0x7227F0 is one unit, and it holds level-3-only functions
  (Player_State0_53 and Player_State0_55) as well as level-4-only ones
  (Player_Update: 452 words off at 3; Player_CheckTeamCombo). Capcom's
  source switched levels inside the unit, with `#pragma
  optimization_level` or the like. Round 20 merged it: unk_70A290.c
  (game, 0x70A290-0x72E450, the old unk_716360.c and unk_722400.c) is at 3
  up to 0x722400 and at 4 from there. Round 19's merges add one: the Deco
  unit (unk_3B4C80.c, sub_main) is at 4 up to 0x3BD380 and at 3 from there
  (`reset`; Equip_CompactDecorations needs 3). The Quest unit
  (unk_3A1070.c, sub_main, 0x3A1070-0x3AC3A0, merged in round 20 with the
  old mine_point.c and unk_3A6130.c) is at 4 throughout.
  So:
  - a one-function island of the other level inside one function family is
    a pragma: keep its pair (Map_DrawZoomed, StageObj_K1C_NextStepD);
  - split a pair file only where something besides the level places the
    cut: a closed function family with its own .data (EffEm33 in round 18,
    the round 17 splits below), static callees, data a family points back
    at. Round 18's level-only cuts were reverted at the merge (below).
- **Level-4 pairs in level-3 files** (kept: nothing but the level places a
  cut there): unk_328CD0.c (sub_main), unk_6F8A10.c (game: around
  AreaObj_K13_Draw), unk_70E2A0.c and unk_722360.c (lobby), around matched
  functions and drafts; unk_760C70.c and unk_76DB40.c (lobby), inside
  NON_MATCHING blocks only.
- **Split for this in round 18:** unk_6E0650.c (game): EffEm33_*
  (0x6E3B00-0x6EA220, its .data 0x8AB600-0x8AD3A0 and jump tables closed)
  is unk_6E3B00.c at level 4; EffShakalaka34_* to EffEm02_* are
  unk_6EA220.c. The units batch also cut unk_6F8A10.c, unk_70E2A0.c,
  unk_722360.c and unk_328CD0.c on the level alone; the lead reverted those
  four at the merge, since the level can change inside a unit (above). Their
  level scans and cut points are in round 18's notes.
- **Split for this in rounds 11-12:** unk_30EA30.c (Kbd_DrawCandidateBox is now
  unk_314AD0.c), unk_3E0EC0.c (the fishing code, level 3, is now
  unk_3E3D00.c), unk_2E3940.c (the bones are now unk_2EF020.c), unk_185460.c
  (its level-4 tail is now unk_1864E0.c), unk_71F8A0.c (round 8's join took in
  the browser's page code, now unk_722360.c, level 3), unk_76DB40.c (from
  func_00770410 on, now unk_770410.c: its caller passes a y that
  func_0076FA10's s16 parameter would re-extend).
- **Split for this in round 17** (the two pair files with no NON_MATCHING
  block left): unk_3CA790.c (sub_main) is now unk_3CA790.c (Effect06_*),
  unk_3CD9C0.c (Effect0D_*, level 4: Effect0D_Run) and unk_3D2230.c (the
  ground effects and Effect1A_*); unk_6C7AE0.c (game) is now unk_6C7AE0.c
  (Eff06_* and EffEm07_*; Eff06_InitSkills needs level 3), unk_6CADF0.c
  (EffEm08_*, level 4: EffEm08_Run and func_006CF020) and unk_6D29F0.c
  (Eff09_* and Eff10_*). The cuts follow the function families and each
  unit's .data (a family's pointer table points back at its own animation
  tables, so the data cuts are exact); a function that matches at both
  levels between two families can't be placed by its level. Each new file
  repeats the declarations it uses from the parts before it.
- **Level 3 needed:** in round 5's scan, level 4 broke one matched function
  in each of unk_6C7AE0.c, unk_6D3B50.c, unk_7077A0.c (game), unk_715CA0.c
  (lobby), unk_31E190.c, unk_335040.c, unk_3BD5E0.c and unk_3E3D00.c
  (sub_main; split from unk_3E0EC0.c in round 11).
- **Seen in:** Ending_DrawStaffRoll (plsel), Yn_Connect (yn), Tex_VramBlocks
  (main); in round 7 Html_ParseSome, Html_PageKeyword, Lbs_Sort, Html_Csv
  and Page_SetKind (lobby), Vec_RotateX (sub_main), the pad helpers
  (main) and func_00D67C30 (plsel, 65 to 10 words); in round 8
  Sel_DrawHeader (select), Plsel_DrawItemBox (plsel), Portal_ParseStep,
  Dialog_PageStep, Link_Follow, Page_PointTo and Layout_SelectEnd (lobby);
  in round 9 func_006FF990, func_006FFE50, Em_SeePlayers and EmScript_Op5F
  (game), Page_Navigate (lobby, 86 to 52 words), func_0076F0E0 (lobby, 40
  to 20) and func_00712370 (below).
- **A function that matches only when a neighbour is C too.** At level 4,
  MWCC's allocation in one function can depend on the other functions of
  the file compiled from C: func_00712370 (lobby, unk_7115D0.c) matches
  under `-D NON_MATCHING` with func_007120F0's C in the file, and is 95
  words off with func_007120F0 as `INCLUDE_ASM` (at level 3 no order fixed
  its saved registers). Fix: test a NON_MATCHING function with its
  neighbours' drafts compiled too, not alone, and match them together;
  func_00712370 waits for func_007120F0 (15 words, one `movz`). Seen in:
  func_00712370 (lobby).

- **Scan the level first.** A block tuned at the wrong level looks like a
  hopeless register problem. In round 16 a scan of every NON_MATCHING
  block at levels 2, 3 and 4 (a pragma pair around the block alone)
  matched Gallery_DrawPrizeList (lobby) and Effect0D_Run (sub_main) at
  level 4, took StageObj_K0A_Run (game) from 26 words to 2 at level 3, and
  moved about a dozen more blocks; EffEm08_Run and func_006CF020 (game)
  matched at level 4 inside a level-3 file. Each keeps its level in a
  pragma pair until its file's units are decided. Try this before anything
  else on a near miss.
- **Scan again after the code changes.** A block's own level pair can be
  the last obstacle: func_003294A0 (sub_main) carried a level-4 pair, and
  once its compare was right it matched only at 3 (round 19). A rescan of
  13 sub_main files' blocks found five closer at the other level.

- **Other CodeWarrior settings for a whole file.** `#pragma opt_propagation
  off` (unk_185460.c, main) and `#pragma opt_lifetimes off` (unk_334260.c,
  sub_main: Tex_LoadFile, a pointer stepped N times in an unrolled loop
  whose loads the game doesn't fold into offsets). Such a pragma is fair
  only when every other function of the file still matches; each is an
  assumption about how that unit was built, and its comment says so. A
  pragma pair around one function, other than the optimization level, is a
  FAKEMATCH.

### What can't come from C, and the permuter's false fixes

- **VU0 macro instructions (`lqc2`, `sqc2`, `vadd` or `vmul` with broadcast
  fields, `qmtc2`).** Cause: MWCC never emits them from C; the originals were
  CodeWarrior `asm` functions. Fix: `asm void f(...) { .set noreorder ... }`
  with the splat listing (registers without `$`, `la reg, sym` for a
  `lui`/`addiu` pair, labels without the dot); a `j` to a label inside the
  function stays a raw `.word` (objdiff then shows 99.9%; the bytes match).
  They count as matched. Seen in: Math_SinCos and the VU0 math library
  (sub_main).
- **Loops that branch back to their own entry label, with 8-byte aligned
  entries.** Hand-written assembly: main 0x182450-0x182640 (copy and fill
  loops), committed as assembly source (src/main/mem_loops.s; "Hand-written
  assembly").
- **8-byte aligned functions, `sd` saves, branch-likely instructions
  (`beql`).** GCC code: Sony's and CRI's libraries (and yn's netcnfif) stay
  assembly. CRI Sofdec (main 0x1313C0-0x158930) was built with CodeWarrior
  but isn't Capcom's, so it stays assembly too.
- **Inline asm the game used.** The eight-nop busy waits (above); interrupt
  handlers that end with `__asm__ volatile ("sync.l\n\tei");` (dma.c and
  display.c, main); MWCC accepts GCC-style `__asm__ volatile`, with operands
  too, and schedules it with the code around it unless it starts with `.set
  noreorder`. MWCC's `asm` functions reject `%hi`/`%lo`: a function that needs
  them is a static C function around an `__asm__` block (Vu1_PutLightColors;
  Vu1_CopyMatrixSlots around an inline `lq`/`sq` block, sub_main).
  The gp register as a C value: `__asm__ volatile ("move %0, $gp" :
  "=r"(g)); tp.gpReg = g;` compiles to a plain `sw gp` that the scheduler
  moves, and the statement's position steers it: written between the
  stackSize and initPriority stores it lands in CreateThread's delay slot
  (Psp_StartThread, select). A CodeWarrior `asm { move g, gp }` block and
  `&_gp` stay 6 words off, and MWCC rejects `register ... asm("gp")`.
- **Empty delay slots don't prove asm.** Round 5 showed that function-level
  asm with `.set reorder` gives the pattern (conditional-branch slots empty,
  `jal` and `jr` slots filled from before) but can't align labels, and the
  originals have the alignment nops of compiled code. QuestNet_Send and
  QSync_OnPlayer (gm_sub), Player_UpdateStatus and func_00387870 (sub_main)
  were classed as asm before the C forms of "Delay slots filled or empty"
  were found; all but QSync_OnPlayer (s0 and s1 swapped) match now.
- **Proving that no C gives it.** What round 5 did for the delay slots: all
  21 MWCC builds of Phase 2's harness at -O2 to -O4, every MIPS back-end
  pragma and about 60 front-end `opt_*` pragmas on and off, the command-line
  options, then the compiler's own code for the pass in question. MWCC
  ignores unknown pragmas silently (`strings mwccps2.exe | grep opt_` lists
  the real ones); `#pragma schedule off` empties every slot, `jal` and `b`
  included; `#pragma peephole off` gave func_003862D0 (sub_main) the
  original's branch structure but not its registers.
- **The permuter's false fixes: don't commit them.** A meaningless
  condition as it stands (`if ((cur && cur) && cur) {}`): an empty compound
  `if` is a real scheduling barrier, so rewrite it as a plausible
  compiled-out check (Packet_Alloc matched with `if (align == 0 || (align &
  (align - 1)) != 0) {}`; see "Statements scheduled as if a check were
  still there"). Dead code the assembly doesn't show (Sound_StageBank,
  which matched with a volatile read), a changed constant
  (GreatSword_SetChargeAttack, which later matched with `case 0:
  default:`), logic moved into another branch (Yn_Connect's
  `unk25 |= 0x80`), a dead store that fixes one branch and misaligns
  another (Yn_LoadModule, which later matched: its wait was 7 asm nops, not
  8), `fx++; fx--;` (func_0085DC60), `new_var = 0`
  temporaries (Chara_SetupSavedPlayer), `m->list[cur & 0xFFFFFFFFu]`
  (func_006E05B0: the natural form is `m->list[(u32)cur]`, which matched
  WeaponForge_MakeInput), an uninitialised pointer (CombShop_PageReady),
  casts that change behaviour (Layout_CellInput), several of these at once
  (func_00770770), a comma operator (`(u32)(0, k)`, func_006BB390), an
  argument passed to a callee that takes none (`func_8FB280(3)`), a
  `volatile short` return type (func_00711C90: the callee returns s16).
  Its real finds read as natural C once rewritten: an
  inline identity call became `p[2] + 0` (Plsel_CheckSizeAwards), a
  pointer to the pointer became `gLobbyNet.dlBuf` (func_00701990),
  `* 12U` became `* sizeof(f32[3])` (EmScript_Op90), an identity function
  around `(s32)&gSave` became a `static inline` accessor
  (OfferMenu_AddRequest), `(tbl = T)[kind]` became a pointer step
  (Talk_FirstByte), an empty `if` became a compiled-out range check
  (Rate_ListAdd), a duplicated assignment pointed at the element indexed in
  full (func_0070F6E0). Round 8's real finds: a pointer declared last and
  set inside a hit test (DlgCursor_Draw), a `u8` local reused for two tests
  (Page_PointAt), `t += id * 14` right after t's load (func_00D61400), a
  reused s16 or float local (func_0070D360, func_006F2360), a statement
  order before a loop (func_002E4B60), an element read again instead of a
  local (Page_DrawFocus). Dead code is fine when the assembly shows it: a
  redundant test or `else`, an empty compound `if`.
- **Solved in round 7**, from this list: the constants loaded just before
  each store (Plsel_DrawArrow: named objects; a default-first switch also
  gives it), the switch value in a0 (NetMenu_Draw: a u8 local), an s16
  parameter added to unextended (EquipShop_DrawLine, func_0076F720: steps
  inside the arguments), the `nop` before a `jal` (Lobby_ReceiveAll: a
  default-first switch), a case's empty then-branch jump (func_006BE8E0: a
  nested one-case switch); `or`/`and` operand order on a u16
  (Lobby_SetReadyBit) matches too.
- **Solved in round 8**, from this list: the byte copy (Mem_Copy and
  Heap_Move: the read inside an empty `if`, see "Struct copies and
  clears"); the `lw`/`sw` copy with 8-byte steps (func_003B0AF0: a word
  loop unrolled by two); the if-body after the last block (func_0030EB20:
  a `goto` past an explicit `return;`); the constants loaded before each
  store (Chara_DrawEquipIcon, the old func_006B34B0: statement order;
  Chara_DrawEquipment down to 14 words with a named object);
  Plsel_CheckRankAwards (a u8 loop value and the table walks); the
  command pointer's step (EmScript_Op17: `p++; p++;`); func_007E4360
  (EmMelynx_UpdateFace: `default: return;` first); Psp_StartThread's gp
  store (above); DualSwords_State04 (`case 0: default: case 2:`); the
  `addu` order of func_007038F0 and func_00794930 (a byte offset in an int
  local); func_007D9190's search loop (a repeated test; 9 words left, its
  callers wait for it).
- **Not explained yet** (worth a fresh look): two locals sharing a stack
  slot (LobbyReply_6912, LobbyStep_6884Done and LobbyLogin_OnSummaryReply,
  lobby); parameters in the lowest saved registers (QSync_OnPlayer, gm_sub:
  a parameter masked or extended once at the entry would give it, see
  "Parameters in each other's saved registers"; untried); every `bc1t`
  slot empty (Color_Lerp, the old func_0037D890, sub_main); a pointer stepped
  by 4 twice between loads (func_007D44C0 and func_007D45E0, lobby); two
  loop counters where MWCC keeps one (func_007C1900, lobby); a pointer
  spilled 12 bytes off (StageObj_K1C_RunChain, EffEm33_RunChain, game; see
  "A stack slot 12 bytes off"). Explained since: func_007D2030 (round 16),
  EmScript_Op0C and Op0D (round 16, Placement), ActSpot_IsUsable (round 17,
  Delay slots), Save_GetEquipFlags (round 17: not inlined, Static callees).

## decomp-permuter

For functions that are close but won't match, use
[decomp-permuter](https://github.com/simonlindholm/decomp-permuter) (tested
at commit `059609d4aec73eb0650726772954e1ad575825f8`). Its settings are in
`tools/permuter_settings.toml`: it compiles with MWCC through wibo, using the
project's flags and the `mwcc` randomization weights.

One-time setup (the permuter has its own Python dependencies):

```sh
git clone https://github.com/simonlindholm/decomp-permuter tools/deps/decomp-permuter
git -C tools/deps/decomp-permuter checkout 059609d4aec73eb0650726772954e1ad575825f8
python3 -m venv tools/deps/permuter-venv
tools/deps/permuter-venv/bin/pip install pycparser toml
```

Then, from the repository root, give it a C file containing just the
function (plus the declarations it needs; it must live inside the repo,
e.g. under `build/`) and the function's assembly:

```sh
PERM=tools/deps/permuter-venv/bin/python
$PERM tools/deps/decomp-permuter/import.py build/perm/func.c \
    asm/sub_main/nonmatchings/unk_2E3940/func_002E6E20.s
$PERM tools/deps/decomp-permuter/permuter.py nonmatchings/func_002E6E20
```

`import.py` creates `nonmatchings/<function>/` (gitignored). A score of 0
means a match.

**Before the permuter**, when only registers differ, run a
declaration-order search: every order of the declaration lines (up to six
or so), or a hill climb over swaps of two lines. In round 9 it fixed
register-only drafts in minutes where the permuter found nothing
(func_006C67E0 245 to 4 words; three sub_main drafts matched): scratch
tools such as sub_main's declperm.py, game's hc.sh and lobby's dperm2.py,
all a few dozen lines around `itertools.permutations` and adiff.

Limits:
- A function already in C has no `.s` under `asm/.../nonmatchings/` (the
  build deletes it once the function is C): import the `.s` from a split
  of the base commit (a clean clone after `configure.py`).
- The permuter replaces other functions with bare declarations, so it
  cannot help when a `static` callee's register usage matters.
- It does not pass MWCC's `-i include` to the preprocessor: inline what the
  function needs from `common.h`.
- If `python3 -m venv` fails in WSL (no python3-venv package), `uv venv`
  works.
- For a function with a jump table, a matching candidate scores 10, not 0
  (tested with func_006EB060): the table's relocations name `jtbl_...` in
  the target and MWCC's local `@N` in the candidate. Don't rely on
  `--stop-on-zero` there; check candidates with `func_diff.py`.
