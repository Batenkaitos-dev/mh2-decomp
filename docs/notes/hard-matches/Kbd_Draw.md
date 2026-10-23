# Kbd_Draw (sub_main, src/sub_main/kbd.c)

## The match

Four forms together make the match. The word counts give the miss left after each step,
starting from 19 words.

1. **The glyph width as an s32 holding the (s16) of its byte**: `s32 gw;` and
   `gw = (s16)D_00416F20[g].w;`, read like the scale (`(s16)D_00416F20[g].scale`). With `s16 gw`
   the value is a narrow local: MWCC numbers it as created, and `#pragma schedule off` shows the w
   load created after the h load, at the sw statement (the game creates it first: w byte t1, h
   byte t0). The s32 keeps its own number at its declaration, and the game's code uses it with no
   re-extension (an s16 variable is re-extended at each use, as the i == 7 branch's `w` is).
   19 -> 16. Without the cast the extension is gone (size -4); `short` is the same as s16.
2. **The glyph blocks' declaration orders** with it (all 720 orders of each block were tried):
   loop 1 needs gw before sw and pad < gh < code (pad, gh and code are spilled there: the slots
   0x100, 0xF0, 0xE0 go in reverse declaration order); loop 2 needs sw < gw < gh and code < pad
   (slots 0xD0, 0xC0). 60 orders of each give 0 in the end; the file has gw, sw, pad, gh, code, g
   and code, g, sw, gw, gh, pad. 16 -> 5.
3. **The texel column as `((code & 0xF) << 4)`** in u0 and u1 of all three Kbd_DrawSprite calls.
   For `X - 1 + gw`, MWCC emits `addu v0,gw,temp` when X is `(code & 0xF) * 16` and `addu
   v0,temp,gw` (the game's order) when X is `(code & 0xF) << 4` (micro test: ud against u4).
   The same fixed the i == 7 branch's `+ w`. 5 -> 2. u0 needs no change, but is written the
   same way for consistency (same code).
4. **`for (i = 0, p = D_00418750; ...)`** in the page loop: i = 0 is then created before p's
   addiu, and the second scheduling pass (ALU values in creation order) puts it first, as the game
   does (`i = 0; p = ...; for (; ...)` also matches). 2 -> 0.

## Evidence

Rejected: u1 written as `gw + ..`, `(.. + gw) - 1` and four other orders (16-118 words); a
contrived first web for gw to make the load a later web (54, size +8); the types of gw, gh,
sw, g one at a time (u8, u16, u32, s8, s32 without cast: 18-173 or another size).

## Method and form

- Operand order of a commutative add is a lever: `(c & 0xF) * 16 - 1 + v` and
  `((c & 0xF) << 4) - 1 + v` give the same instructions with the addu operands the other way
  round (`gC - 1 + w` gives temp first too, in a micro test). Where only an addu's rs/rt are
  swapped, try `<< n` for `* 2^n` in the operand (a candidate line for "Temporary registers
  swapped" or the compare forms in docs/mwcc-matching.md).
- `s32 gw = (s16)D_00416F20[g].w;` is a cast that changes no value (u8 to s16 to s32). It is
  how the function already reads the scale (`(s16)D_00416F20[g].scale`, needed for the game's
  extension), so it is left untagged; read as a cast with no purpose, it would take a
  FAKEMATCH tag. No other form found gives the extension at the definition and no
  re-extension at the uses.
