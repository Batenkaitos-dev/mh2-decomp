# BoxMenu_ArrangeMenu (lobby, src/lobby/menu_sheets.c)

## The match

The swap block (step 1, sub 1, pad & 0x20):

    held = &gBoxWork->held;
    h = *held;
    k2 = &gBoxWork->kind;
    k = *k2;
    slots = gBoxWork->slots;
    c = slots[k];
    if (c != h) {
        save = &gSave;
        if (k == 0) {
            ... save->itemBox[h] ... save->itemBox[gBoxWork->held] = st;
        } else {
            ... save->box[h] ...; save->box[*held] = e;
            Equip_SwapSlots(*held, slots[*k2]);

u8 k and u16 h are the function's own locals. Natural C (values cached in locals, re-read
through the pointers after the stores); no tag.

## Why (the model that predicted it)

- The miss without this order (7 words): gBoxWork's load in a1 (game v1, shared with slots =
  base + 28). The final listings of the other forms and the game have the same cycle structure;
  the difference is the first (pre-colouring) scheduling pass, where slots and k2 were created
  before gBoxWork's last use (tempcolor: "a1@125 skips v1, no neighbour holds it").
- First pass (as in AreaObj_K11_Draw.md): two issue slots per cycle, one load per cycle, load
  latency 2; in each slot an instruction that ends a value first, otherwise the earliest-created
  ready one. Created H, K, S, KV, I, A, C, HV: H K | S KV | HV, so the base lives past S.
  Created H, KV, K, HV, S (the statement order above, h and k used twice so they are values of
  their own, created at their statements): H KV | K HV | S, so S ends the base and shares v1.
- The second pass (final listing) looks different: loads first, then ALU values in creation
  order (KV H | HV K | S I in the 7-word form, I S in the game), which is why every final listing
  looked the same.
- A load read into a local used once is substituted to its use (`k = *k2` used only in the
  index gave 7): k is used for the index and the kind test, h for the compare and the copies.

## Evidence (variants and scores)

- All 30 valid orders of the five statements (H, h, K, k, S): HhKkS 0, KkHhS 2, the others 7.
- k/h read after the three pointers, u8/u32 k, u16/u32 h (7, 77, 108).
- `(slots = gBoxWork->slots)[*k2]` inline, with and without the compare turned round (7, 115).
- Single forms (slots from held, held from slots, k2 from slots, casts, &slots[0], m local, u8 view,
  register, c as s32/u32, *(slots + *k2), (u32) index, a p = &slots[*k2] local, save first): 7-118.

The variants were scored by generating one file with every variant of the function and
comparing each against the expected object.

## Method

For blocks whose final listing matches in shape but a base register is "skipped with no
neighbour": the first pass is earliest-created with one load per cycle, so interleave the loads
with the pointer values in creation order (a value read into a local used twice is created at
its statement).
