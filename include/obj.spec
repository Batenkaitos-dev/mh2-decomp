# Fields of Obj (include/obj.h), an object of sub_main's first object pool (0x80 of 0x40 bytes at 0x5B4FB0,
# Obj_Alloc/Obj_Free; sub_main's unk_363550.c, unk_37B570.c and unk_386160.c read it too), which the game overlay uses for its
# short-lived objects (effects and the like): an update function run every frame steps a state machine in
# +0x4 (0 set up, 1 run, 2 end, 3 free). Regenerate with:
#   python tools/gen_struct.py include/obj.spec include/obj.h
name Obj
size 0x40
0x000 u8 active // nonzero while the object is in use
0x001 u8 visible
0x002 u8 kind // set by the creator
0x003 u8 type // the variant within the kind (the creator's argument; the updates switch on it)
0x004 u8 state // 0 set up, 1 run, 2 end, 3 free (the update functions switch on it)
0x005 u8 var0 // per-kind scratch byte (counters, flags; += var1 in some)
0x006 u8 var1 // per-kind scratch byte
0x007 u8 var2 // per-kind scratch byte (a random lifetime: Rand_Next % 10 + 55 ...)
0x008 s16 timer // a frame counter in many kinds (--timer <= 0; & 0x1FF / 512.0f)
0x00A s16 var3 // per-kind s16: a second counter (to unk01E[0]) or an angle (/ 65536.0f)
0x00C struct Obj * prev
0x010 struct Obj * next
0x014 ObjFn{4} draw // drawn every frame while it is visible (Obj_DrawAll); NULL: none
0x018 void * buf // its blocks of the object buffer (the kind's data: a struct of its own per kind)
0x01C u8 blk
0x01D u8 nblk
0x01E u8 var4 // per-kind scratch byte: a player index, a position-table index (0xFF none), a random lifetime
0x01F u8 keep // not freed by Obj_FreeUnkept
0x020 f32 pos[3]
0x02C ObjFn{4} update // run every frame (Obj_UpdateAll)
0x030 f32 fvar // per-kind f32: a scale, an alpha, a speed or a velocity, a spacing
0x034 void * arg // per-kind pointer: the creator's argument (the thrown item's player; cleared by K14)
0x038 struct DrawSlot * ds // its draw slot, DrawSlot_Get(slot) (a DrawSlot; OT_AddByDepth adds it)
0x03C s16 slot // its draw slot (DrawSlot_Alloc; -1: none was free), in the kinds that draw through one
0x03E s16 var5 // per-kind s16: a frame counter (K13 to 1200, the season banner), the notice banner's message
