# Fields of StageObj (include/stageobj.h), an object of sub_main's third object list (StageObj_Alloc(nblk)
# allocates one from the free list gStageObjFreeTop onto the active list gStageObjList; StageObj_UpdateAll runs
# each active one's update every frame, StageObj_Free frees one). The game overlay's stage objects use it: their
# updates switch on the state, their creators set the kind. The header up to 0x1F is laid out like Obj's.
# Regenerate with:
#   python tools/gen_struct.py include/stageobj.spec include/stageobj.h
name StageObj
size 0x40
0x000 u8 active // nonzero while the object is in use
0x001 u8 visible
0x002 u8 kind // set by the creator (enum StageObjKind)
0x003 u8 type // the variant within the kind (the updates switch on it)
0x004 u8 state // the update's state
0x005 u8 var0 // per-kind scratch byte (counters, flags)
0x006 u8 var1 // per-kind scratch byte (a bone index for Em_GetBonePos, an angle << 8 ...)
0x007 u8 var2 // per-kind scratch byte (0xFF: none in many kinds; an angle << 8 ...)
0x008 s16 timer // a frame counter
0x00A s16 angle // a 16-bit angle in most kinds (Vec_RotateY, Mtx_RotateY, MonsterFx_CreateAtYaw; yaw from Vec_DirToAngles); a bone index in some
0x00C struct StageObj * prev
0x010 struct StageObj * next
0x014 void * draw
0x018 u8 * buf // its blocks of the object buffer (the kind's data)
0x01C u8 blk
0x01D u8 nblk // the block count given to StageObj_Alloc
0x01E u8 ownerIndex // the owner's index (Player.index, Em.index; compared with gScene.localPlayer)
0x01F u8 keep // not freed by StageObj_FreeUnkept
0x020 StageObjFn{4} update // run every frame (StageObj_UpdateAll)
0x024 f32 pos[3] // the position
0x030 f32 scale // the size the creator gives (1.0f by default)
0x034 void * owner // the monster, player or object it follows (NULL: none), in some kinds
0x038 struct DrawSlot * node // its draw slot, DrawSlot_Get(slot), when the kind draws one (DrawSlot: include/sprite.h)
0x03C s16 slot // its draw slot (DrawSlot_Alloc; -1: none was free), freed by the end state (DrawSlot_Free)
0x03E u16 angle2 // a second 16-bit angle (pitch from Vec_DirToAngles) in some kinds; an area in others
