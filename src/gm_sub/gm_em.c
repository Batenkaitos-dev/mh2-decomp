/*
 * The monsters in the quest overlay (gm_sub): each monster's task
 * (GmEm_Task: set-up, update, finish, free), the set-up and update every
 * kind shares (GmEm_Setup, GmEm_ResetState, GmEm_Update) and the dispatch to
 * the families' own Start, Init and Update functions by kind (Em.type,
 * EM_KIND_*); the sizes (GmEm_ResetSize, GmEm_ApplySize); the sounds of the
 * shared motions (GmEm_MotionSounds); and what Em_DrawParts asks before it
 * draws a mesh set: the texture scrolls of some kinds (GmEm_SetMeshSetUv)
 * and each set's materials (GmEm_PrepareMeshSet: alpha, break states,
 * colours, textures).
 *
 * The rest of the file is its .data: the per-kind monster tables the game
 * overlay's monster code reads (the families' callback tables and data),
 * generated from the game as u32 words with pointers written as C.
 */
#include "common.h"
#include "sprite.h"
#include "quest.h"
extern u16 gFrameTick; /* sub_main: the frame counter */
#include "scene.h"
#include "m2c_macros.h"
#include "em.h"
#include "stageobj.h"
#include "player.h"
#include "area.h"
#include "render_state.h"

/* A texture coordinate matrix (4 x 4 f32): Mtx_Identity fills it, the
 * scroll functions set its offsets, and Gs_SetState(RS_UV_MTX) hands it to
 * the VU1 programs, which map a mesh's texture coordinates through rows 0, 1
 * and 3. Only the offsets are named: nothing here sets the rest. */
typedef struct UvMtx {
    f32 rows[12];               /* 0x00: rows 0 to 2 */
    f32 uOffset;                /* 0x30: row 3: the U offset */
    f32 vOffset;                /* 0x34: the V offset */
    f32 row3zw[2];              /* 0x38 */
} UvMtx;

/* Applies a render state: (id, value). The value is an s32 (a matrix's or a
 * material's address here): with a pointer parameter the compiler loads the
 * two arguments in the other order. */
s32 Gs_SetState(u8 reg, u32 v);
void Mtx_Identity(f32 *m);
f32 Math_Sin(f32);
void EmMine_RemoveTimed(Em *em);
s32 EmChameleos_IsInvisible(Em *);
s32 ScEvent_CurId(void);
s32 ScEvent_CurFlag1(void);
StageObj *MonsterFx_CreateOnEm(Em *em, u8 k, f32 scale);
void Em_ClearNeed1State(Em *em);
void Em_ClearNeed2State(Em *em);
void Em_RefillNeed3(Em *em);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed2(Em *em, s32 n);
void Em_AddNeed3(Em *em, s32 n);
void GmEm_KhezuScrollUv(Em *, s32);
void GmEm_PlesiothScrollUv(Em *, s32);
void GmEm_YamaTsukamiScrollUv(Em *, s32);
s32 GmEm_ChameleosMeshUv(Em *, s32);
void GmEm_FatalisCPulseUv(Em *, s32);

struct DrawSlot;
s32 OT_AddByDepth(u32 *table, struct DrawSlot *node, u32 n, s32 clamp);
void Player_UpdateAreaPos(void *);
void GmEm_Setup(Em *);
void EmSlot_SetupModel(Em *);
void Versus_AssignEmTeam(Em *);
void GmEm_ResetState(Em *);
void Em_InitCollMask(Em *em);
void Em_SelectSightRow(Em *em, u8 idx);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_InitStatusThresholds(Em *em);
void Em_InitRage(Em *em);
void Em_InitKindParams(Em *em);
void Em_PlaySeOnFrame(Em *em, s32 frame, s32 se);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
void Em_MigrateHost(Em *em);
void QSync_UpdateEmState(Em *);
void Player_TickTimers(Player *);
void Player_UpdateHitStop(void *);
struct TriHdr;
s32 Em_GetFloorInfoEx(Em *em, f32 *pos, struct TriHdr *info, f32 *out, f32 *depth);
void Em_SensePlayers(Em *em);
void Em_CheckPartBuildup(Em *em);
s8 Em_SeePlayers(Em *em);
s32 Em_FindNearAreaObj(Em *);
void Em_StartRage2(Em *em);
void Em_PickRequest(Em *em);
u8 Em_UpdateNoticed(Em *em);
extern s16 gEmCombatInit[];
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Vec_Set(f32 *v, f32 x, f32 y, f32 z);
extern EmRageRow *gEmRageTbl[];
void Em_EndRage(Em *em);
u16 Map_DayId(u16 area);
s32 Em_InBlockArea2(Em *em);
void Em_ForceState19(Em *em);
void Em_UpdateStatus(Em *em);
u16 Side_HornItemPlaying(u8 a);
void EmMine_UpdateTimed(Em *);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Em_UpdateBendTarget(Em *em);
void Em_UpdateReactions(Em *em);
void Em_DecayNotice(Em *em);
void Player_SetFlag(void *, u32 flag);
s32 Player_UpdateMotion(Player *);
void Player_ClearFlag(void *, u32 flag);
void Em_CollideField(Em *em, u8 keep);
void Em_UpdateLightColors(Em *em);
s32 Game_IsOnline(void);
void EmScript_Init(Em *em);
void Em_RestartScript(Em *em);
f32 Em_LevelAttackFactor(Em *em);
f32 Em_LevelDamageFactor(Em *em);
void Em_ApplyAreaValues(Em *em);
extern f32 gEmBodyScales[];
extern u8 gEmPushPriorities[];
extern s32 gEmKindTables[];
extern s32 *gEmRateTbl[];
extern s32 *gEmDecay7ECRates[];
void EmRathalos_Start(Em *);
void EmFatalis_Init(Em *);
void EmAnteka_Start(Em *);
void EmMosswine_Start(Em *);
void EmKutku_Start(Em *);
void EmLaoshan_Init(Em *);
void EmCephadrome_Start(Em *);
void EmMelynx_Start(Em *);
void EmTrader_Init(Em *);
void EmApceros_Start(Em *);
void EmVelociprey_Start(Em *);
void EmDiablos_Start(Em *);
void EmKhezu_Start(Em *);
void EmGravios_Start(Em *);
void EmFelyneCart_Init(Em *);
void EmVespoid_Start(Em *);
void EmPlesioth_Start(Em *);
void EmDrome_Start(Em *);
void EmKind1D_Init(Em *);
void EmKirin_Init(Em *);
void EmGaruga_Start(Em *);
void EmHermitaur_Start(Em *em);
void EmShenGaoren_Init(Em *);
void EmBlangonga_Start(Em *);
void EmKushala_Start(Em *);
void EmKind38_Start(Em *);
void EmShakalaka_Start(Em *);
void EmYamaTsukami_Start(Em *);
void EmChameleos_Start(Em *);
void EmRemobra_Start(Em *);
void EmBulldrome_Start(Em *);
s16 DrawSlot_Alloc(void);
struct DrawSlot *DrawSlot_Get(s32 i);
void Em_DrawParts(DrawSlot *a);
extern u32 gGmEmCallbackTbl[76];
void EmRathalos_Update(Em *em);
void EmGravios_Update(Em *em);
void EmKutku_Update(Em *em);
void EmFatalis_Update(Em *em);
void EmAnteka_Update(Em *em);
void EmMosswine_Update(Em *em);
void EmLaoshan_Update(Em *em);
void EmCephadrome_Update(Em *em);
void EmMelynx_Update(Em *em);
void EmTrader_Update(Em *);
void EmApceros_Update(Em *em);
void EmKhezu_Update(Em *em);
void EmDiablos_Update(Em *em);
void EmVelociprey_Update(Em *em);
void EmDrome_Update(Em *em);
void EmFelyneCart_Update(Em *);
void EmPlesioth_Update(Em *em);
void EmVespoid_Update(Em *em);
void EmKind1D_Update(Em *);
void EmKirin_Update(Em *em);
void EmGaruga_Update(Em *em);
void EmShakalaka_Update(Em *em);
void EmHermitaur_Update(Em *em);
void EmBlangonga_Update(Em *em);
void EmKushala_Update(Em *em);
void EmShenGaoren_Update(Em *em);
void EmKind38_Update(Em *);
void EmYamaTsukami_Update(Em *em);
void EmChameleos_Update(Em *em);
void EmRemobra_Update(Em *em);
void EmBulldrome_Update(Em *em);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: that keeps the game's masking instruction */
u8 EmKind_MotionGroupCount(s16 i);
void GmEm_SetupKind(Em *);
void Em_InitParts(Em *);
s32 Quest_IsVersus(void);
s32 Side_Match(u8 a, u8 b, u8 none);
void Em_UpdateMatrix(Em *);
void GmEm_Update(Em *);
void GmEm_CallFunc3(Em *);


/* .bss: kind 29's base hp row, filled at run time (gEmBaseHp's entry for
 * kind 29 points to it). */
u8 gEmBaseHp_29[0x70];

void EmSlot_Free(Em *e); /* sub_main: e is an EmSlot * */
void QuestEm_Hunted(Em *em);
void EmMine_Remove(Em *em);

/* Plays the monster's sounds on frames of its current motion
 * (Em_PlaySeOnFrame(em, frame, sound)), some picked at random with
 * Sound_RandomSe; the sounds are the monster bank's. work->fxMotion keeps
 * the motion id switched on. Kinds 0x20, Mosswine, Bullfango and Bulldrome
 * differ in three motions. The case blocks follow the order of the game's
 * code, and each block's labels the reverse of the order it compares them. */
void GmEm_MotionSounds(Em *em, EmWork *work)
{
    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, 1):
        Em_PlaySeOnFrame(em, 0x10, Sound_RandomSe(0, 2, 1, 2));
        Em_PlaySeOnFrame(em, 0x4C, Sound_RandomSe(1, 2, 2, 2));
        break;
    case EM_MOTION_ID(0, 2):
        switch (em->type) {
        case 0x20:
            Em_PlaySeOnFrame(em, 0xC, Sound_RandomSe(0, 2, 1, 2));
            Em_PlaySeOnFrame(em, 0x2A, Sound_RandomSe(1, 2, 2, 2));
            Em_PlaySeOnFrame(em, 6, 0x13);
            Em_PlaySeOnFrame(em, 0x10, 0x12);
            Em_PlaySeOnFrame(em, 0x24, 0x13);
            break;
        case EM_KIND_MOSSWINE:
        case EM_KIND_BULLFANGO:
            Em_PlaySeOnFrame(em, 0xC, Sound_RandomSe(0, 2, 1, 2));
            Em_PlaySeOnFrame(em, 0x2A, Sound_RandomSe(1, 2, 2, 2));
            Em_PlaySeOnFrame(em, 6, 0x13);
            Em_PlaySeOnFrame(em, 0x10, 0x12);
            Em_PlaySeOnFrame(em, 0x24, 0x13);
            break;
        case EM_KIND_BULLDROME:
            Em_PlaySeOnFrame(em, 0xC, Sound_RandomSe(0, 1, 1, 1));
            Em_PlaySeOnFrame(em, 0x2A, Sound_RandomSe(1, 1, 2, 1));
            Em_PlaySeOnFrame(em, 2, 0x13);
            Em_PlaySeOnFrame(em, 0x10, 0x12);
            Em_PlaySeOnFrame(em, 0x24, 0x13);
            Em_PlaySeOnFrame(em, 0x2A, 0x12);
            break;
        }
        break;
    case EM_MOTION_ID(0, 3):
        Em_PlaySeOnFrame(em, 0xC, Sound_RandomSe(0, 2, 1, 2));
        Em_PlaySeOnFrame(em, 0x26, Sound_RandomSe(1, 2, 2, 2));
        Em_PlaySeOnFrame(em, 6, 0x12);
        Em_PlaySeOnFrame(em, 0x1E, 0x13);
        Em_PlaySeOnFrame(em, 0x2C, 0x13);
        break;
    case EM_MOTION_ID(0, 4):
    case EM_MOTION_ID(0, 5):
        Em_PlaySeOnFrame(em, 6, Sound_RandomSe(0, 2, 1, 2));
        Em_PlaySeOnFrame(em, 0x46, Sound_RandomSe(1, 2, 2, 2));
        Em_PlaySeOnFrame(em, 0x16, 0x13);
        Em_PlaySeOnFrame(em, 0x38, 0x12);
        Em_PlaySeOnFrame(em, 0x64, 0x13);
        break;
    case EM_MOTION_ID(0, 6):
        Em_PlaySeOnFrame(em, 0x28, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 0x3C, Sound_RandomSe(1, 4, 2, 4));
        Em_PlaySeOnFrame(em, 0x50, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 0x64, Sound_RandomSe(1, 4, 2, 4));
        Em_PlaySeOnFrame(em, 0x9A, 0xF);
        Em_PlaySeOnFrame(em, 0x14, 0x12);
        Em_PlaySeOnFrame(em, 0x28, 0x12);
        Em_PlaySeOnFrame(em, 0xA4, 0x12);
        break;
    case EM_MOTION_ID(0, 7):
        Em_PlaySeOnFrame(em, 0x26, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 0x3A, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0x5C, Sound_RandomSe(4, 4, 5, 4));
        Em_PlaySeOnFrame(em, 0x72, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0xA4, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 0xB4, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0xD2, Sound_RandomSe(4, 4, 5, 4));
        Em_PlaySeOnFrame(em, 0xE8, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0x18, 0x12);
        Em_PlaySeOnFrame(em, 0x28, 0x12);
        Em_PlaySeOnFrame(em, 0x82, 0x12);
        Em_PlaySeOnFrame(em, 0xA4, 0x12);
        Em_PlaySeOnFrame(em, 0xF6, 0x12);
        break;
    case EM_MOTION_ID(0, 8):
        Em_PlaySeOnFrame(em, 4, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 0x24, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 4, 0x13);
        Em_PlaySeOnFrame(em, 0x2A, 0x13);
        Em_PlaySeOnFrame(em, 0x34, 0x12);
        break;
    case EM_MOTION_ID(0, 9):
        Em_PlaySeOnFrame(em, 6, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 0x46, Sound_RandomSe(0, 2, 1, 2));
        Em_PlaySeOnFrame(em, 0x26, 0x13);
        Em_PlaySeOnFrame(em, 0x4C, 0x13);
        break;
    case EM_MOTION_ID(0, 10):
        Em_PlaySeOnFrame(em, 0x24, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0x3A, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0x80, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0x96, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0x16, 0x12);
        Em_PlaySeOnFrame(em, 0x74, 0x13);
        break;
    case EM_MOTION_ID(0, 11):
        Em_PlaySeOnFrame(em, 0x16, 0xF);
        Em_PlaySeOnFrame(em, 0xA, 0x12);
        Em_PlaySeOnFrame(em, 0x52, 0x12);
        break;
    case EM_MOTION_ID(0, 12):
        Em_PlaySeOnFrame(em, 4, 0xE);
        Em_PlaySeOnFrame(em, 0x44, 0xF);
        Em_PlaySeOnFrame(em, 0xC, 0x12);
        Em_PlaySeOnFrame(em, 0x46, 0x13);
        Em_PlaySeOnFrame(em, 0x5E, 0x13);
        break;
    case EM_MOTION_ID(0, 13):
        Em_PlaySeOnFrame(em, 0x18, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 0x28, 0x12);
        break;
    case EM_MOTION_ID(0, 14):
        Em_PlaySeOnFrame(em, 0x44, 0xD);
        Em_PlaySeOnFrame(em, 0x24, 0x14);
        Em_PlaySeOnFrame(em, 0x4A, 0x10);
        break;
    case EM_MOTION_ID(0, 15):
        switch (em->type) {
        case 0x20:
            Em_PlaySeOnFrame(em, 0x14, 8);
            Em_PlaySeOnFrame(em, 0x18, 0x13);
            Em_PlaySeOnFrame(em, 0x24, 0x13);
            break;
        case EM_KIND_MOSSWINE:
        case EM_KIND_BULLFANGO:
            Em_PlaySeOnFrame(em, 0xA, 0xE);
            Em_PlaySeOnFrame(em, 0x18, 0x13);
            Em_PlaySeOnFrame(em, 0x24, 0x13);
            break;
        case EM_KIND_BULLDROME:
            Em_PlaySeOnFrame(em, 0x22, Sound_RandomSe(3, 2, 4, 2));
            Em_PlaySeOnFrame(em, 0xA, 0xE);
            Em_PlaySeOnFrame(em, 0x20, 0x19);
            Em_PlaySeOnFrame(em, 0x24, 0x13);
            break;
        }
        break;
    case EM_MOTION_ID(0, 16):
        Em_PlaySeOnFrame(em, 0x14, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 8, 0x13);
        Em_PlaySeOnFrame(em, 0xE, 0x13);
        Em_PlaySeOnFrame(em, 0xC, 0xD);
        Em_PlaySeOnFrame(em, 0x62, 0x12);
        break;
    case EM_MOTION_ID(0, 17):
        Em_PlaySeOnFrame(em, 0x20, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0x46, 0xF);
        Em_PlaySeOnFrame(em, 0xC8, Sound_RandomSe(4, 4, 5, 4));
        Em_PlaySeOnFrame(em, 0x1C, 0x14);
        Em_PlaySeOnFrame(em, 0x2E, 0x13);
        break;
    case EM_MOTION_ID(0, 18):
    case EM_MOTION_ID(0, 19):
        Em_PlaySeOnFrame(em, 0xE, 0x16);
        Em_PlaySeOnFrame(em, 0x64, Sound_RandomSe(1, 4, 2, 4));
        Em_PlaySeOnFrame(em, 0x74, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 0x40, 0xD);
        Em_PlaySeOnFrame(em, 0x86, 0xE);
        Em_PlaySeOnFrame(em, 0xE6, 0xF);
        Em_PlaySeOnFrame(em, 0x32, 0x19);
        Em_PlaySeOnFrame(em, 0x36, 0x14);
        Em_PlaySeOnFrame(em, 0xA2, 0x19);
        Em_PlaySeOnFrame(em, 0xA8, 0x14);
        Em_PlaySeOnFrame(em, 0x11C, 0x13);
        Em_PlaySeOnFrame(em, 0x13E, 0x12);
        break;
    case EM_MOTION_ID(0, 21):
        Em_PlaySeOnFrame(em, 0x1E, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 0x68, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0x14, 0x12);
        Em_PlaySeOnFrame(em, 0x1E, 0x12);
        Em_PlaySeOnFrame(em, 0x32, 0x12);
        Em_PlaySeOnFrame(em, 0x48, 0x12);
        Em_PlaySeOnFrame(em, 0x5C, 0x12);
        Em_PlaySeOnFrame(em, 0x68, 0x12);
        break;
    case EM_MOTION_ID(0, 22):
        Em_PlaySeOnFrame(em, 0x1C, Sound_RandomSe(0, 4, 1, 4));
        Em_PlaySeOnFrame(em, 0x52, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 8, 0x10);
        Em_PlaySeOnFrame(em, 6, 0x15);
        Em_PlaySeOnFrame(em, 0x36, 0x10);
        Em_PlaySeOnFrame(em, 0x34, 0x15);
        Em_PlaySeOnFrame(em, 0x54, 0x10);
        Em_PlaySeOnFrame(em, 0x52, 0x15);
        Em_PlaySeOnFrame(em, 0x72, 0x10);
        Em_PlaySeOnFrame(em, 0x70, 0x15);
        Em_PlaySeOnFrame(em, 0x8C, 0x10);
        Em_PlaySeOnFrame(em, 0xAA, 0x10);
        Em_PlaySeOnFrame(em, 0xC8, 0x18);
        break;
    case EM_MOTION_ID(0, 23):
        Em_PlaySeOnFrame(em, 4, 0x17);
        Em_PlaySeOnFrame(em, 8, 0x18);
        Em_PlaySeOnFrame(em, 0xC, 0x16);
        Em_PlaySeOnFrame(em, 0x2C, 0x14);
        Em_PlaySeOnFrame(em, 0x34, 0x14);
        Em_PlaySeOnFrame(em, 0x56, 0x13);
        Em_PlaySeOnFrame(em, 0x64, 0x13);
        Em_PlaySeOnFrame(em, 0x66, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0x80, 0x12);
        Em_PlaySeOnFrame(em, 0x86, 0x13);
        Em_PlaySeOnFrame(em, 0x94, 0x13);
        Em_PlaySeOnFrame(em, 0x9C, 0x13);
        break;
    case EM_MOTION_ID(0, 20):
        Em_PlaySeOnFrame(em, 8, Sound_RandomSe(7, 4, 8, 4));
        Em_PlaySeOnFrame(em, 0x1A, 0x15);
        Em_PlaySeOnFrame(em, 0x3C, 0x12);
        break;
    case EM_MOTION_ID(0, 60):
    case EM_MOTION_ID(0, 61):
        Em_PlaySeOnFrame(em, 0xC, Sound_RandomSe(7, 4, 8, 4));
        Em_PlaySeOnFrame(em, 0x60, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0x26, 0x14);
        Em_PlaySeOnFrame(em, 0x2A, 0x14);
        Em_PlaySeOnFrame(em, 0x54, 0x12);
        Em_PlaySeOnFrame(em, 0x6C, 0x12);
        break;
    case EM_MOTION_ID(0, 62):
    case EM_MOTION_ID(0, 67):
        Em_PlaySeOnFrame(em, 0xC, Sound_RandomSe(9, 4, 0xA, 4));
        break;
    case EM_MOTION_ID(0, 63):
        switch (em->type) {
        case 0x20:
            Em_PlaySeOnFrame(em, 0xE, 0x11);
            Em_PlaySeOnFrame(em, 0x40, 0x10);
            Em_PlaySeOnFrame(em, 0x46, Sound_RandomSe(7, 4, 8, 4));
            break;
        case EM_KIND_MOSSWINE:
        case EM_KIND_BULLFANGO:
            Em_PlaySeOnFrame(em, 0xE, 0x11);
            Em_PlaySeOnFrame(em, 0x40, 0x10);
            Em_PlaySeOnFrame(em, 0x46, Sound_RandomSe(7, 4, 8, 4));
            break;
        case EM_KIND_BULLDROME:
            Em_PlaySeOnFrame(em, 0x10, 9);
            Em_PlaySeOnFrame(em, 0xE, 0x11);
            Em_PlaySeOnFrame(em, 0x40, 0x10);
            Em_PlaySeOnFrame(em, 0x46, Sound_RandomSe(7, 4, 8, 4));
            break;
        }
        break;
    case EM_MOTION_ID(0, 64):
        Em_PlaySeOnFrame(em, 8, 0xB);
        Em_PlaySeOnFrame(em, 8, 0x10);
        Em_PlaySeOnFrame(em, 0x1E, 0x10);
        break;
    case EM_MOTION_ID(0, 66):
    case EM_MOTION_ID(0, 73):
        Em_PlaySeOnFrame(em, 0x12, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 0x50, 0x10);
        Em_PlaySeOnFrame(em, 0x1E, 0x12);
        Em_PlaySeOnFrame(em, 0x3C, 0x13);
        Em_PlaySeOnFrame(em, 0x58, 0x12);
        Em_PlaySeOnFrame(em, 0x68, 0x12);
        Em_PlaySeOnFrame(em, 0x80, 0xD);
        break;
    case EM_MOTION_ID(0, 72):
        Em_PlaySeOnFrame(em, 0x12, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 0x16, 0x10);
        Em_PlaySeOnFrame(em, 0x1E, 0x12);
        Em_PlaySeOnFrame(em, 0x3C, 0x13);
        Em_PlaySeOnFrame(em, 0x58, 0x12);
        Em_PlaySeOnFrame(em, 0x68, 0x12);
        Em_PlaySeOnFrame(em, 0x80, 0xD);
        break;
    case EM_MOTION_ID(0, 71):
        Em_PlaySeOnFrame(em, 6, Sound_RandomSe(9, 4, 0xA, 4));
        Em_PlaySeOnFrame(em, 0x24, 0x11);
        Em_PlaySeOnFrame(em, 0x42, Sound_RandomSe(3, 4, 4, 4));
        Em_PlaySeOnFrame(em, 0x60, Sound_RandomSe(3, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0x7C, Sound_RandomSe(5, 4, 6, 4));
        Em_PlaySeOnFrame(em, 0xB4, Sound_RandomSe(0, 4, 1, 4));
        break;
    case EM_MOTION_ID(0, 70):
        Em_PlaySeOnFrame(em, 4, 0xC);
        Em_PlaySeOnFrame(em, 0x14, 0x12);
        Em_PlaySeOnFrame(em, 0x34, 0x12);
        Em_PlaySeOnFrame(em, 0x3E, 0x12);
        Em_PlaySeOnFrame(em, 0x78, 0x10);
        Em_PlaySeOnFrame(em, 0x118, Sound_RandomSe(1, 4, 2, 4));
        break;
    }
}

/* Sets up the monster for its kind: hooks the kind's handler table
 * (gGmEmCallbackTbl) and runs its first entry, takes a draw slot when the
 * monster is visible, then calls its family's Start or Init function. */
void GmEm_SetupKind(Em *em)
{
    em->handlers = (EmVt *)gGmEmCallbackTbl[em->type];
    em->handlers->init(em);
    if (em->visible != 0) {
        em->drawSlotId = DrawSlot_Alloc();
        if (em->drawSlotId != -1) {
            em->drawSlot = DrawSlot_Get(em->drawSlotId);
            em->drawSlot->owner = em;
            em->drawSlot->draw = Em_DrawParts;
        } else {
            em->drawSlot = NULL;
        }
    }
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
        EmRathalos_Start(em);
        break;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        EmFatalis_Init(em);
        break;
    case EM_KIND_KELBI:
    case EM_KIND_ANTEKA:
        EmAnteka_Start(em);
        break;
    case EM_KIND_MOSSWINE:
        EmMosswine_Start(em);
        break;
    case EM_KIND_BULLFANGO:
        EmMosswine_Start(em);
        break;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
        EmKutku_Start(em);
        break;
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        EmLaoshan_Init(em);
        break;
    case EM_KIND_CEPHADROME:
        EmCephadrome_Start(em);
        break;
    case 9:
        EmMelynx_Start(em);
        break;
    case EM_KIND_TRADER:
        EmTrader_Init(em);
        break;
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
        EmRathalos_Start(em);
        break;
    case 12:
    case EM_KIND_POPO:
        EmApceros_Start(em);
        break;
    case EM_KIND_GENPREY:
        EmVelociprey_Start(em);
        break;
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
        EmDiablos_Start(em);
        break;
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        EmKhezu_Start(em);
        break;
    case EM_KIND_VELOCIPREY:
    case EM_KIND_GIAPREY:
        EmVelociprey_Start(em);
        break;
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
        EmGravios_Start(em);
        break;
    case EM_KIND_FELYNE_CART:
        EmFelyneCart_Init(em);
        break;
    case EM_KIND_VESPOID:
        EmVespoid_Start(em);
        break;
    case EM_KIND_GYPCEROS:
    case EM_KIND_PURPLE_GYPCEROS:
        EmKutku_Start(em);
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        EmPlesioth_Start(em);
        break;
    case EM_KIND_BASARIOS:
        EmGravios_Start(em);
        break;
    case EM_KIND_MELYNX:
        EmMelynx_Start(em);
        break;
    case EM_KIND_HORNETAUR:
        EmVespoid_Start(em);
        break;
    case EM_KIND_APCEROS:
        EmApceros_Start(em);
        break;
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
        EmDiablos_Start(em);
        break;
    case EM_KIND_VELOCIDROME:
        EmDrome_Start(em);
        break;
    case EM_KIND_GENDROME:
        EmDrome_Start(em);
        break;
    case 29:
        EmKind1D_Init(em);
        break;
    case EM_KIND_IOPREY:
        EmVelociprey_Start(em);
        break;
    case EM_KIND_IODROME:
        EmDrome_Start(em);
        break;
    case 32:
        EmMosswine_Start(em);
        break;
    case EM_KIND_KIRIN:
        EmKirin_Init(em);
        break;
    case EM_KIND_CEPHALOS:
        EmCephadrome_Start(em);
        break;
    case EM_KIND_YIAN_GARUGA:
        EmGaruga_Start(em);
        break;
    case EM_KIND_DAIMYO_HERMITAUR:
        EmHermitaur_Start(em);
        break;
    case EM_KIND_SHEN_GAOREN:
        EmShenGaoren_Init(em);
        break;
    case EM_KIND_BLANGONGA:
        EmBlangonga_Start(em);
        break;
    case EM_KIND_CONGALALA:
        EmBlangonga_Start(em);
        break;
    case EM_KIND_RAJANG:
        EmBlangonga_Start(em);
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
        EmKushala_Start(em);
        break;
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        EmKushala_Start(em);
        break;
    case 56:
        EmKind38_Start(em);
        break;
    case EM_KIND_SHAKALAKA:
        EmShakalaka_Start(em);
        break;
    case EM_KIND_YAMA_TSUKAMI:
        EmYamaTsukami_Start(em);
        break;
    case EM_KIND_CHAMELEOS:
        EmChameleos_Start(em);
        break;
    case EM_KIND_BLANGO:
        EmBlangonga_Start(em);
        break;
    case EM_KIND_CONGA:
        EmBlangonga_Start(em);
        break;
    case EM_KIND_REMOBRA:
        EmRemobra_Start(em);
        break;
    case EM_KIND_HERMITAUR:
    case EM_KIND_CEANATAUR:
        EmHermitaur_Start(em);
        break;
    case EM_KIND_SHOGUN_CEANATAUR:
        EmHermitaur_Start(em);
        break;
    case EM_KIND_BULLDROME:
        EmBulldrome_Start(em);
        break;
    case EM_KIND_YAMA_TSUKAMI_B:
        EmYamaTsukami_Start(em);
        break;
    case 0:
        break;
    }
}

/* Resets the monster to its normal size (sizePct 100): the scale from
 * bodyScale, and the three need gauges cleared and refilled to their
 * maxima. */
void GmEm_ResetSize(Em *em)
{
    f32 scale;

    em->sizePct = 100;
    scale = em->sizePct / 100.0f;
    em->scaleX = em->bodyScale * scale;
    em->scaleY = em->bodyScale * scale;
    em->scaleZ = em->bodyScale * scale;
    Em_ClearNeed2State(em);
    Em_ClearNeed1State(em);
    Em_RefillNeed3(em);
    Em_AddNeed2(em, em->need2Max);
    Em_AddNeed1(em, em->need1Max);
    Em_AddNeed3(em, em->need3Max);
}

/* Sets the monster's scale from bodyScale and sizePct. */
void GmEm_ApplySize(Em *em)
{
    f32 scale = em->sizePct / 100.0f;

    em->scaleX = em->bodyScale * scale;
    em->scaleY = em->bodyScale * scale;
    em->scaleZ = em->bodyScale * scale;
}

/* Returns value scaled by the monster's size (sizePct). */
f32 GmEm_ScaleBySize(Em *em, f32 value)
{
    return value * (em->sizePct / 100.0f);
}

/* Resets the monster's per-quest state: clears its two spawn requests,
 * restarts its script, sets its three need gauges' maxima and starting
 * values from the per-kind rows, and clears the other fields it tracks. */
void GmEm_ResetState(Em *em)
{
    s8 i;
    s32 *need1Row;
    EmRate *need3Row;
    s32 *need2Row;

    em->hostChanged = 0;
    for (i = 0; i < 2; i++) {
        em->spawns[i].pending = 0;
        em->spawns[i].pos[0] = 0.0f;
        em->spawns[i].pos[1] = 0.0f;
        em->spawns[i].pos[2] = 0.0f;
        em->spawns[i].rot[0] = 0;
        em->spawns[i].rot[1] = 0;
        em->spawns[i].rot[2] = 0;
        em->spawns[i].stateGroup = 0;
        em->spawns[i].state = 0;
        em->spawns[i].goalKind = 0;
        em->spawns[i].goalArg = 0;
        em->spawns[i].goalNo = 0;
        em->spawns[i].area = 0;
        em->spawns[i].timer = 0;
        em->spawns[i].rand = 0;
    }
    EmScript_Init(em);
    Em_RestartScript(em);
    em->tailObj = 0;
    em->floorAttr = 1;
    em->hitPhase = 0;
    em->visible = 1;
    em->motion[0].step = 2.0f;
    em->motion[1].step = 2.0f;
    em->motion[2].step = 2.0f;
    em->motion[3].step = 2.0f;
    em->bodyScale = gEmBodyScales[em->type];
    em->scaleX = gEmBodyScales[em->type];
    em->scaleY = gEmBodyScales[em->type];
    em->scaleZ = gEmBodyScales[em->type];
    if (em->type != EM_KIND_FELYNE_CART && em->type != EM_KIND_TRADER) {
        GmEm_ApplySize(em);
    }
    if (gQuest.stage == 0) {
        em->rotX = 0.0f;
        em->rotY = 0.0f;
        em->rotZ = 0.0f;
    }
    em->alpha = 1.0f;
    em->painted = 0;
    em->motionTime = 0;
    EmMine_Remove(em);
    em->pushPriority = gEmPushPriorities[em->type];
    em->bendAngle[0] = 0;
    em->bendAngle[1] = 0;
    em->bendAngle[2] = 0;
    em->bendAngle[3] = 0;
    em->motionSpeed = 1.0f;
    em->speedBonus = 0;
    em->quakeTimer = 0;
    em->hostNo = 0;
    em->fxFlags = 0;
    em->syncPulse[0] = 0;
    em->syncPulse[1] = 0;
    em->combatTimer = 0;
    em->combat = 0;
    em->requested = 0;
    em->requestId = 0;
    em->seen = 0;
    em->noticed = 0;
    em->noticeTimer[0] = 0;
    em->noticeTimer[1] = 0;
    em->noticeTimer[2] = 0;
    em->noticeTimer[3] = 0;
    need1Row = gEmRateTbl[em->type];
    need3Row = gEmRateStatus.rate[em->type];
    need2Row = gEmDecay7ECRates[em->type];
    em->need2Max = need2Row[0];
    em->need1Max = need1Row[0];
    em->need3Max = need3Row->max;
    if (em->need2Gauge <= 0) {
        em->need2Gauge = need2Row[1];
    }
    if (em->need1Gauge <= 0) {
        em->need1Gauge = need1Row[1];
    }
    if (em->need3Gauge <= 0) {
        em->need3Gauge = need3Row->start;
    }
    em->rage = 0;
    em->rageTimer = 0;
    em->rageBuildup = 0;
    em->rageStep = 0;
    em->ragePending = 0;
    em->hooked = 0;
    em->guardTimer = 0;
    em->flashed = 0;
    em->noReact = 0;
    em->routeMissCount = 0;
    em->need2State = 0;
    em->need1State = 0;
    em->need3State = 0;
    em->noAlert = 0;
    em->alertBuildup[0] = 0;
    em->alertBuildup[1] = 0;
    em->alertBuildup[2] = 0;
    em->alertBuildup[3] = 0;
    em->sensedPlayers = 0;
    em->alertedPlayers = 0;
    em->alertLimitRow = 1;
    em->triggers = 0;
    em->noticeCount[0] = 0;
    em->noticeCount[1] = 0;
    em->noticeCount[2] = 0;
    em->noticeCount[3] = 0;
    em->tables = (void **) gEmKindTables[em->type];
    em->blindTimer = 0;
    em->part8Down = 0;
    em->partBreakCount = 0;
    em->trapHit = 0;
    em->trapTimer = 0;
    em->hideShadowHold = 0;
    em->senseDelay = 0;
    em->requestPrio = 0;
    em->lowHpNeed = 0;
    em->need4 = 0;
    em->need2 = 0;
    em->need1 = 0;
    em->need3 = 0;
    em->holdTimer = 0;
    em->holdTimer2 = 0;
    em->ceilingTimer = 0;
    em->hermitTimer = 0;
    em->windTimer = 0;
    em->moveFrozen = 0;
    Em_InitStatusThresholds(em);
    em->status2Buildup = 0;
    em->status2Decay = 0;
    em->status2Count = 0;
    em->status1Buildup = 0;
    em->status1Decay = 0;
    em->status1Count = 0;
    em->status4Buildup = 0;
    em->status4Decay = 0;
    em->status4Count = 0;
    em->status5Count = 0;
    em->poisonBuildup = 0;
    em->poisonTimer = 0;
    em->poisonCount = 0;
    em->poisonDamage = 0;
    em->status = 0;
    em->statusFlags = 0;
    em->hitReactMode = 0;
    em->blind = 0;
    em->hideShadow = 0;
    em->attackFactor = Em_LevelAttackFactor(em);
    em->damageFactor = Em_LevelDamageFactor(em);
    em->fieldCollide = 1;
    em->link = 0;
    em->enterDelay = 0;
    em->inSceneArea = 0;
    em->hitzoneSet = 0;
    em->itemUseKind = 0;
    em->versusGauge = 0;
    em->rageStarted = 0;
    em->scriptCallEntryHi = 0;
    em->scriptDelay = 0;
    em->stateArmor = 0;
    em->mineSlot2 = -1;
    em->mineTimer2 = 0;
    em->mineCount2 = 1;
    Em_ApplyAreaValues(em);
}

/* The task's set-up phase: sets up the model, resets the per-quest state,
 * motions, sight, reactions, statuses, rage and kind parameters, shows all
 * 32 meshes and runs the kind's set-up. For a large monster, the first
 * player in the quest in its area (on another side in versus) becomes its
 * host. */
void GmEm_Setup(Em *em)
{
    s32 i;

    EmSlot_SetupModel(em);
    em->taskPhase++;
    em->isEm = 1;
    em->ownTask = 0;
    em->hostNo = gScene.localPlayer;
    Versus_AssignEmTeam(em);
    GmEm_ResetState(em);
    em->motionId[0] = 0;
    em->motionId[1] = 0;
    em->motionId[2] = 0;
    em->motionId[3] = 0;
    Em_InitCollMask(em);
    Em_SelectSightRow(em, 0);
    Em_LoadReactionSet(em, 0);
    Em_InitStatusThresholds(em);
    Em_InitRage(em);
    Em_InitKindParams(em);
    for (i = 0; i < 0x20; i++) {
        em->meshShown[i] = 1;
    }
    em->motionLayerCount = (u8)(s32)EmKind_MotionGroupCount(em->type);
    GmEm_SetupKind(em);
    Em_InitParts(em);
    if (em->large != 0) {
        for (i = 0; i < gScene.playerCount; i++) {
            if (em->area == gPlayers[i].area && gScene.inQuest[i] == 1) {
                if (Quest_IsVersus() == 1) {
                    if (gPlayers[i].side >= 3 || Side_Match(gPlayers[i].side, *(u8 *)&em->side, 1) == 1) {
                        continue;
                    }
                    if (gScene.localPlayer == i) {
                        em->remote = 0;
                    } else {
                        em->remote = 1;
                    }
                } else {
                    if (gScene.localPlayer == i) {
                        em->remote = 0;
                    } else {
                        em->remote = 1;
                    }
                }
                em->hostNo = i;
                break;
            }
        }
    } else {
        em->remote = 0;
    }
    Player_UpdateAreaPos(em);
    Em_UpdateMatrix(em);
}

/* Runs the update function of the monster's family, by kind. */
void GmEm_UpdateKind(Em *em)
{
    switch (em->type) {
    case 0:
    case EM_KIND_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_AZURE_RATHALOS:
        EmRathalos_Update(em);
        break;
    case EM_KIND_GRAVIOS:
    case EM_KIND_BASARIOS:
    case EM_KIND_BLACK_GRAVIOS:
        EmGravios_Update(em);
        break;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_GYPCEROS:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_PURPLE_GYPCEROS:
        EmKutku_Update(em);
        break;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        EmFatalis_Update(em);
        break;
    case EM_KIND_KELBI:
    case EM_KIND_ANTEKA:
        EmAnteka_Update(em);
        break;
    case EM_KIND_MOSSWINE:
    case EM_KIND_BULLFANGO:
    case 32:
        EmMosswine_Update(em);
        break;
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        EmLaoshan_Update(em);
        break;
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        EmCephadrome_Update(em);
        break;
    case 9:
    case EM_KIND_MELYNX:
        EmMelynx_Update(em);
        break;
    case EM_KIND_TRADER:
        EmTrader_Update(em);
        break;
    case 12:
    case EM_KIND_APCEROS:
    case EM_KIND_POPO:
        EmApceros_Update(em);
        break;
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        EmKhezu_Update(em);
        break;
    case EM_KIND_DIABLOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_WHITE_MONOBLOS:
        EmDiablos_Update(em);
        break;
    case EM_KIND_GENPREY:
    case EM_KIND_VELOCIPREY:
    case EM_KIND_IOPREY:
    case EM_KIND_GIAPREY:
        EmVelociprey_Update(em);
        break;
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IODROME:
        EmDrome_Update(em);
        break;
    case EM_KIND_FELYNE_CART:
        EmFelyneCart_Update(em);
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        EmPlesioth_Update(em);
        break;
    case EM_KIND_VESPOID:
    case EM_KIND_HORNETAUR:
        EmVespoid_Update(em);
        break;
    case 29:
        EmKind1D_Update(em);
        break;
    case EM_KIND_KIRIN:
        EmKirin_Update(em);
        break;
    case EM_KIND_YIAN_GARUGA:
        EmGaruga_Update(em);
        break;
    case EM_KIND_SHAKALAKA:
        EmShakalaka_Update(em);
        break;
    case EM_KIND_DAIMYO_HERMITAUR:
    case EM_KIND_HERMITAUR:
    case EM_KIND_SHOGUN_CEANATAUR:
    case EM_KIND_CEANATAUR:
        EmHermitaur_Update(em);
        break;
    case EM_KIND_BLANGONGA:
    case EM_KIND_CONGALALA:
    case EM_KIND_RAJANG:
    case EM_KIND_BLANGO:
    case EM_KIND_CONGA:
        EmBlangonga_Update(em);
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        EmKushala_Update(em);
        break;
    case EM_KIND_SHEN_GAOREN:
        EmShenGaoren_Update(em);
        break;
    case 56:
        EmKind38_Update(em);
        break;
    case EM_KIND_YAMA_TSUKAMI:
        EmYamaTsukami_Update(em);
        break;
    case EM_KIND_CHAMELEOS:
        EmChameleos_Update(em);
        break;
    case EM_KIND_REMOBRA:
        EmRemobra_Update(em);
        break;
    case EM_KIND_BULLDROME:
        EmBulldrome_Update(em);
        break;
    case EM_KIND_YAMA_TSUKAMI_B:
        EmYamaTsukami_Update(em);
        break;
    }
}

/* The monster update step shared by all kinds (timers, flags, the kind's
 * update through GmEm_UpdateKind, the render scale). The scale's float
 * registers need the speed in a local of its own, scaled in place
 * (speed *= ...): as one expression they come out rotated. */
void GmEm_Update(Em *em)
{
    s32 noticed;
    u8 set;
    u8 nearAreaObj;
    u8 result;
    u16 dayMap;
    f32 motionStep;
    f32 speed;
    EmRageRow *rageRow;
    f32 pos[3];

    set = 0;
    Em_MigrateHost(em);
    if (Game_IsOnline() == 1 && em->large != 0) {
        QSync_UpdateEmState(em);
    }
    if (em->stateGroup == EM_GROUP_VERSUS) {
        em->noHitTimer = 2;
    }
    Player_TickTimers((Player *) em);
    Player_UpdateHitStop(em);
    if (em->inSceneArea == 0) {
        if (gScene.area == em->area) {
            em->inSceneArea = 1;
            Em_GetFloorInfoEx(em, &em->posX, (struct TriHdr *)&em->floorKind, &em->floorY, &em->surfaceY);
            if (em->stance == EM_STANCE_GROUND) {
                em->posY = em->floorY;
            }
        }
    } else if (gScene.area != em->area) {
        em->inSceneArea = 0;
    }
    if (em->moveFrozen == 0) {
        em->prevPosX = em->posX;
        em->prevPosY = em->posY;
        em->prevPosZ = em->posZ;
    }
    if (gScene.area != em->area) {
        em->enterDelay = 2;
    }
    if (em->stateGroup != EM_GROUP_DEFEAT && em->stateGroup != EM_GROUP_EVENT && em->stateGroup != EM_GROUP_EVENT2 && em->stateGroup != EM_GROUP_VERSUS) {
        if (em->combat == 1) {
            em->senseDelay = 0;
        }
        if (em->senseDelay != 0) {
            em->senseDelay--;
        } else {
            Em_SensePlayers(em);
            if (em->noAlert != 1) {
                Em_CheckPartBuildup(em);
            }
            Em_SeePlayers(em);
            switch (em->type) {
            case EM_KIND_RATHIAN:
            case EM_KIND_GOLD_RATHIAN:
            case EM_KIND_PINK_RATHIAN:
            case EM_KIND_CEPHADROME:
            case EM_KIND_RATHALOS:
            case EM_KIND_SILVER_RATHALOS:
            case EM_KIND_AZURE_RATHALOS:
            case EM_KIND_KHEZU:
            case EM_KIND_RED_KHEZU:
            case EM_KIND_GYPCEROS:
            case EM_KIND_PURPLE_GYPCEROS:
            case EM_KIND_DAIMYO_HERMITAUR:
            case EM_KIND_BLANGONGA:
            case EM_KIND_CONGALALA:
                if (Em_ScriptFlagsFrom(em, EM_TRIGGER_AREA_OBJ) == 0 && em->remote == 0 && em->requested == 0) {
                    nearAreaObj = Em_FindNearAreaObj(em);
                } else {
                    nearAreaObj = 0;
                }
                if (em->combat == 0 || em->type == EM_KIND_CONGALALA) {
                    if (nearAreaObj == 1 && Em_ScriptFlagsFrom(em, EM_TRIGGER_AREA_OBJ) == 0 && em->noReact == 0) {
                        em->triggers |= EM_TRIGGER_AREA_OBJ;
                    }
                }
                break;
            }
            if (em->ragePending != 0 && em->stateGroup != EM_GROUP_REACT && em->noReact == 0 && Em_ScriptFlagsFrom(em, EM_TRIGGER_RAGE) == 0) {
                Em_StartRage2(em);
            }
            Em_PickRequest(em);
            noticed = (u8)Em_UpdateNoticed(em);
            if ((em->type == 9 || em->type == EM_KIND_MOSSWINE) && em->combat == 0) {
                noticed = 0;
            }
            if (em->combat == 0 && noticed != 0 && Em_ScriptFlagsFrom(em, EM_TRIGGER_NOTICED) == 0 && em->noAlert != 1 && em->noReact == 0) {
                em->scriptPending = 1;
                em->triggers |= EM_TRIGGER_NOTICED;
                em->triggers &= ~EM_TRIGGER_AREA_OBJ;
            }
            if (em->combat == 1 && noticed != 0) {
                em->combatTimer = gEmCombatInit[em->type];
            }
            if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                if (em->targetNo != -1) {
                    Em_GetTargetPos(em, em->targetNo, pos);
                    Vec_Set(em->goal, pos[0], pos[1], pos[2]);
                }
            }
        }
    }
    if (*(s8 *)&em->syncPulse[0] > 0) {
        (*(s8 *)&em->syncPulse[0])--;
    }
    if (*(s8 *)&em->syncPulse[1] > 0) {
        (*(s8 *)&em->syncPulse[1])--;
    }
    if (*(s8 *)&em->holdTimer > 0) {
        (*(s8 *)&em->holdTimer)--;
    }
    if (*(s8 *)&em->holdTimer2 > 0) {
        (*(s8 *)&em->holdTimer2)--;
    }
    if (*(s8 *)&em->ceilingTimer > 0) {
        (*(s8 *)&em->ceilingTimer)--;
    }
    if (*(s8 *)&em->hermitTimer > 0) {
        (*(s8 *)&em->hermitTimer)--;
    }
    if (em->windTimer > 0) {
        em->windTimer--;
    }
    if (*(s8 *)&em->guardTimer != 0) {
        if (--*(s8 *)&em->guardTimer <= 0) {
            em->guardTimer = 0;
        }
    }
    if (em->rage != 0) {
        rageRow = gEmRageTbl[em->type];
        if (rageRow != NULL) {
            em->motionSpeed = rageRow->motionSpeed;
        }
        if (--em->rageTimer <= 0) {
            Em_EndRage(em);
        }
    }
    dayMap = Map_DayId(gScene.map);
    switch (em->type) {
    case EM_KIND_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PURPLE_GYPCEROS:
    case EM_KIND_YIAN_GARUGA:
        if (dayMap != MAP_BATTLEFIELD && dayMap != MAP_GREAT_ARENA && dayMap != MAP_ARENA) {
            result = Em_InBlockArea2(em);
            if (em->combat == 0 && result == 1 && Em_ScriptFlagsFrom(em, EM_TRIGGER_BLOCK_AREA) == 0 && em->noReact == 0) {
                em->scriptPending = 1;
                em->triggers |= EM_TRIGGER_BLOCK_AREA;
            }
        }
        break;
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_BASARIOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_CEPHADROME:
    case EM_KIND_CEPHALOS:
        break;
    }
    if (em->areaTimer1 > 0) {
        em->areaTimer1--;
    }
    if (em->combat == 1) {
        if (em->areaTimer2 > 0) {
            em->areaTimer2--;
        }
    }
    if (em->stateGroup != EM_GROUP_DEFEAT) {
        if (em->blindTimer != 0) {
            if (--em->blindTimer <= 0) {
                em->blindTimer = 0;
                if (em->status != 2 && em->status != 1) {
                    em->blind = 0;
                    if (em->remote == 0 && em->stateGroup != EM_GROUP_REACT && em->stance == EM_STANCE_GROUND) {
                        Em_ForceState19(em);
                    }
                }
            }
        } else if (em->blind != 0) {
            em->blind--;
        }
        Em_UpdateStatus(em);
    }
    if (em->wallHitDirs != 0) {
        if (em->stateGroup == EM_GROUP_FLY && em->velY >= 0.0f && em->velZ > 0.0f) {
            em->posY += 30.0f;
        }
        em->wallTouch = 2;
    } else {
        em->wallTouch = 0;
    }
    if (*(u8 *)&em->side != 0) {
        if (em->rageStarted != 0) {
            em->itemUseKind = 0x88;
        } else {
            result = Side_HornItemPlaying(*(u8 *)&em->side);
            if (result == 0) {
                em->itemUseKind = 0;
            } else if (!(em->itemUseKind & 0x80)) {
                em->itemUseKind = result | 0x80;
            }
        }
    }
    GmEm_UpdateKind(em);
    EmMine_UpdateTimed(em);
    em->rotX = *(u16 *)&em->rotX;
    em->rotY = *(u16 *)&em->rotY;
    em->rotZ = *(u16 *)&em->rotZ;
    Mtx_FromAnglesYXZ(&em->rotX, (f32 *) em->rotMtx);
    Em_UpdateBendTarget(em);
    Em_UpdateReactions(em);
    Em_DecayNotice(em);
    speed = em->motionSpeed;
    speed *= (f32)(em->speedBonus + 100) / 100.0f;
    motionStep = 2.0f * speed;
    em->motion[0].step = motionStep;
    em->motion[1].step = motionStep;
    em->motion[2].step = motionStep;
    em->motion[3].step = motionStep;
    if (em->moveFrozen != 0) {
        if (em->flags & EM_FLAG_NO_ROOT_MOTION) {
            set = 1;
        } else {
            Player_SetFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
    }
    Player_UpdateMotion((Player *) em);
    if (em->moveFrozen != 0 && !set) {
        Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
    }
    Em_CollideField(em, 0);
    Em_GetFloorInfoEx(em, &em->posX, (struct TriHdr *)&em->floorKind, &em->floorY, &em->surfaceY);
    if (*(s8 *)&em->hideShadowHold == 0) {
        if (em->shadowOff != 0 || em->stance == EM_STANCE_UNDER) {
            em->hideShadow = 1;
        } else {
            em->hideShadow = 0;
        }
    } else if (--*(s8 *)&em->hideShadowHold <= 0) {
        em->hideShadowHold = 0;
    }
    if (em->stance != EM_STANCE_AIR && em->stance != EM_STANCE_UNDER) {
        if (em->surfaceKind != 0) {
            if (em->type != EM_KIND_CEPHADROME && em->type != EM_KIND_DIABLOS && em->type != EM_KIND_MONOBLOS && (u32)(em->type - EM_KIND_BLACK_DIABLOS) > 1 && em->type != EM_KIND_CEPHALOS) {
                em->floorY = em->surfaceY;
            }
        }
        if (em->remote == 0) {
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
        } else if (gScene.area == em->area) {
            em->posY = em->floorY;
            em->rotX = 0;
            em->rotZ = 0;
        }
    }
    Em_UpdateLightColors(em);
}

/* The task's finish phase: moves on to the free phase, hides the monster
 * and removes its timed mines. */
void GmEm_Finish(Em *em)
{
    em->taskPhase++;
    em->visible = 0;
    EmMine_RemoveTimed(em);
}

/* The task's last phase: reports the monster hunted (QuestEm_Hunted),
 * removes its mines and frees its slot. */
void GmEm_Free(Em *em)
{
    QuestEm_Hunted(em);
    EmMine_Remove(em);
    EmSlot_Free(em);
}

/* Returns 0, hiding the monster's shadow, while a scene event that runs
 * only some small monsters plays: event 0xAB keeps kind 0xC and Popo of
 * variant 0, event 0xA4 keeps Blango; any other event keeps none. Else 1.
 * The gotos are the game's control flow. */
s32 GmEm_CanUpdate(Em *em)
{
    s32 eventId;
    u8 kind;

    if ((gScene.eventRunning != 0) && (em->large == 0) && (ScEvent_CurFlag1() == 1)) {
        eventId = ScEvent_CurId();
        switch (eventId) {
        case 0xAB:
            kind = em->type;
            if ((kind == 0xC) || (kind == EM_KIND_POPO)) {
                if (em->variant != 0) {
                    goto block_10;
                }
                goto block_15;
            }
block_10:
            em->hideShadow = 1;
            return 0;
        case 0xA4:
            if (em->type != EM_KIND_BLANGO) {
                em->hideShadow = 1;
                return 0;
            }
            goto block_15;
        default:
            em->hideShadow = 1;
            return 0;
        }
    } else {
block_15:
        return 1;
    }
}

/* The monster's task, each frame, by Em.taskPhase: set-up, update, finish
 * and free. Then it adds the monster's draw slot to an ordering table:
 * the Fatalis, Lao-Shan Lung, Shen Gaoren and Yama Tsukami kinds and an
 * invisible Chameleos go to the sorted one. Returns 1 once freed. */
s32 GmEm_Task(Em *em)
{
    switch (em->taskPhase) {
    case 0:
        GmEm_Setup(em);
        GmEm_CallFunc3(em);
        Player_UpdateAreaPos(em);
        return 0;
    case 1:
        if (GmEm_CanUpdate(em) == 0) {
            return 0;
        }
        GmEm_Update(em);
        break;
    case 2:
        if (GmEm_CanUpdate(em) == 0) {
            return 0;
        }
        GmEm_Finish(em);
        break;
    case 3:
        GmEm_Free(em);
        return 1;
    }
    GmEm_CallFunc3(em);
    if (em->visible != 0 && em->area == gScene.area) {
        em->drawSlot->pos[0] = em->posX;
        em->drawSlot->pos[1] = em->posY;
        em->drawSlot->pos[2] = em->posZ;
        if (em->drawSlot != NULL) {
            if (em->alpha == 1.0f) {
                if (em->type == EM_KIND_FATALIS || em->type == EM_KIND_FATALIS_B || em->type == EM_KIND_FATALIS_C || em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG ||
                    em->type == EM_KIND_SHEN_GAOREN || em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
                    OT_AddByDepth(gDrawOt32, em->drawSlot, 0x20, 1);
                } else if (em->type == EM_KIND_CHAMELEOS && EmChameleos_IsInvisible(em) == 1) {
                    OT_AddByDepth(gDrawOtSorted, em->drawSlot, 0x40, 0);
                } else {
                    OT_AddByDepth(gDrawOt32, em->drawSlot, 0x20, 0);
                }
            } else {
                if (em->type == EM_KIND_FATALIS || em->type == EM_KIND_FATALIS_B || em->type == EM_KIND_FATALIS_C || em->type == EM_KIND_LAO_SHAN_LUNG || em->type == EM_KIND_ASHEN_LAO_SHAN_LUNG ||
                    em->type == EM_KIND_SHEN_GAOREN || em->type == EM_KIND_YAMA_TSUKAMI || em->type == EM_KIND_YAMA_TSUKAMI_B) {
                    OT_AddByDepth(gDrawOtSorted, em->drawSlot, 0x40, 1);
                } else {
                    OT_AddByDepth(gDrawOtSorted, em->drawSlot, 0x40, 0);
                }
            }
        }
    }
    Player_UpdateAreaPos(em);
    return 0;
}

/* The Khezu kinds: mesh set 2's texture scrolls along U (a 64-frame
 * cycle), except while the monster is defeated. */
void GmEm_KhezuScrollUv(Em *em, s32 meshSet)
{
    UvMtx uvMtx;

    Mtx_Identity((f32 *) &uvMtx);
    if (em->stateGroup != EM_GROUP_DEFEAT) {
        switch (meshSet) {
        case 2:
            uvMtx.uOffset = (gScene.frame & 0x3F) / 64.0f;
            break;
        }
    }
    Gs_SetState(RS_UV_MTX, (s32)&uvMtx);
}

/* The Plesioth kinds: mesh set 4's texture scrolls along V (a 64-frame
 * cycle). */
void GmEm_PlesiothScrollUv(Em *em, s32 meshSet)
{
    UvMtx uvMtx;

    Mtx_Identity((f32 *) &uvMtx);
    switch (meshSet) {
    case 4:
        uvMtx.vOffset = 1.0f - (gScene.frame & 0x3F) / 64.0f;
        break;
    }
    Gs_SetState(RS_UV_MTX, (s32)&uvMtx);
}

/* The Yama Tsukami kinds: mesh sets 1 and 4 scroll along U (a 128-frame
 * cycle), except while defeated. */
void GmEm_YamaTsukamiScrollUv(Em *em, s32 meshSet)
{
    UvMtx uvMtx;

    Mtx_Identity((f32 *) &uvMtx);
    if (em->stateGroup != EM_GROUP_DEFEAT) {
        switch (meshSet) {
        case 1:
        case 4:
            uvMtx.uOffset = (gScene.frame & 0x7F) / 128.0f;
            break;
        }
    }
    Gs_SetState(RS_UV_MTX, (s32)&uvMtx);
}

/* Chameleos: mesh sets 5 to 7 offset their texture by the invisibility
 * fade (none: 0.75 for set 7, else 0); set 7 is skipped once fxFlags bit 0
 * is set. The other sets are drawn only while it is visible. Returns 0 to
 * skip the set. */
s32 GmEm_ChameleosMeshUv(Em *em, s32 meshSet)
{
    UvMtx uvMtx;
    /* work + 0x4C is Em.invisFade, Chameleos' invisibility fade (0xFFFF: none),
     * read through a pointer to the work area as the game's code does: as
     * em->invisFade the registers come out in another order */
    u8 *work = &em->work;

    switch (meshSet) {
    default:
        return EmChameleos_IsInvisible(em) != 1;
    case 7:
        if (em->fxFlags & 1) {
            return 0;
        }
    case 5:
    case 6:
        Mtx_Identity((f32 *) &uvMtx);
        if (*(u16 *)(work + 0x4C) != 0xFFFF) {
            uvMtx.uOffset = 1.0f - *(u16 *)(work + 0x4C) / 32.0f;
        } else {
            switch (meshSet) {
            case 7:
                uvMtx.uOffset = 0.75f;
                break;
            case 5:
            case 6:
            default:
                uvMtx.uOffset = 0.0f;
                break;
            }
        }
        Gs_SetState(RS_UV_MTX, (s32)&uvMtx);
        return 1;
    }
}

/* Fatalis C: mesh set 0's texture offset pulses along U (a 128-frame
 * sine). */
void GmEm_FatalisCPulseUv(Em *em, s32 meshSet)
{
    UvMtx uvMtx;

    switch (meshSet) {
    case 0:
        Mtx_Identity((f32 *) &uvMtx);
        uvMtx.uOffset = 0.15f * (1.0f + Math_Sin(6.2831855f * ((gScene.frame & 0x7F) << 9) / 65536.0f));
        Gs_SetState(RS_UV_MTX, (s32)&uvMtx);
        break;
    }
}

/* Sets the texture matrix of the monster's mesh set meshSet, for the kinds
 * that move their textures (Em_DrawParts calls it per set). Returns 0 when
 * the set must not be drawn (Chameleos only), else 1. */
s32 GmEm_SetMeshSetUv(Em *em, s32 meshSet)
{
    switch (em->type) {
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
        GmEm_KhezuScrollUv(em, meshSet);
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        GmEm_PlesiothScrollUv(em, meshSet);
        break;
    case EM_KIND_CHAMELEOS:
        return GmEm_ChameleosMeshUv(em, meshSet);
    case EM_KIND_YAMA_TSUKAMI:
    case EM_KIND_YAMA_TSUKAMI_B:
        GmEm_YamaTsukamiScrollUv(em, meshSet);
        break;
    case EM_KIND_FATALIS_C:
        GmEm_FatalisCPulseUv(em, meshSet);
        break;
    }
    return 1;
}

/* Hands material to the renderer as material slot `slot`
 * (RS_MATERIAL). owner, the monster, is unused. */
void GmEm_UploadMaterial(s32 owner, ModelMat *material, s32 slot)
{
    Gs_SetState(RS_MATERIAL(slot), (s32)material);
}

/* Shen Gaoren: colours the material by the flinch level of part `part`
 * (its work area's partFlinch, at Em +0x45E): dark red, pulsing red after a
 * first flinch, bright pulsing red after a second. */
void GmEm_ShenGaorenFlinchColor(Em *em, ModelMat *material, u16 part)
{
    f32 red;
    s8 flinch = *(s8 *)(part + (u32)em + 0x45E);

    if (flinch == 0) {
        red = 25.0f;
    } else if (flinch == 1) {
        red = 100.0f + 30.0f * (0.5f * (1.0f + Math_Sin(6.2831855f * (gScene.frame << 14) / 65536.0f)));
    } else {
        red = 220.0f + 35.0f * (0.5f * (1.0f + Math_Sin(6.2831855f * (gScene.frame << 14) / 65536.0f)));
    }
    material->rgb[0] = red / 255.0f;
    material->rgb[1] = 40.0f / 255.0f;
    material->rgb[2] = 55.0f / 255.0f;
}

extern s32 gTexHandles[];
extern s16 gGmEmFatalisBMatTex[];
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
/* The fields of the monster's work area (Em.work) GmEm_PrepareMeshSet reads,
 * as the family views name them: EmBlangongaWork's rageTimer and tint,
 * EmFatalisWork's rageTimer, EmWork's hunted and huntedB. One view of its
 * own, because reading them through those structs (casts of one pointer)
 * compiles the function differently from the game's code. */
typedef struct EmMeshWork {
    u8 unk00[0x38];
    s16 blangRageTimer;         /* 0x38: EmBlangongaWork.rageTimer (Rajang) */
    u8 unk3A[0x41 - 0x3A];
    u8 blangTint[3];            /* 0x41: EmBlangongaWork.tint, the body colour (r, g, b) */
    u8 unk44[0x57 - 0x44];
    s8 fatalisRageTimer;        /* 0x57: EmFatalisWork.rageTimer */
    u8 unk58[0x5E - 0x58];
    u8 hunted;                  /* 0x5E: EmWork.hunted (the dromes) */
    u8 unk5F[0x6E - 0x5F];
    u8 huntedB;                 /* 0x6E: EmWork.huntedB (the preys) */
} EmMeshWork;
void Color_RgbToFloat(f32 *rgb, u32 c);
s32 Area_IsDay(u16 area);
s32 Em_PartBroken(Em *em, s32 arg1);
s32 EmBlangonga_EatPropMode(Em *em);
s32 EmBlangonga_CarriedShown(Em *em, u16 part);
u8 EmChameleos_CanTurnInvisible(Em *em); /* unit's own declaration: the definition (game) returns s32 (0, 1 or 2); declared u8 here because GmEm_PrepareMeshSet's code masks the result to a byte, as a u8 return compiles */

/* Prepares mesh set `set` of the monster's model (sets: the model's
 * material sets) before it is drawn: every material of the set gets
 * Em.alpha, then per kind some are hidden (alpha 0) by the parts' break
 * states, partBreakCount, the rage or a blink cycle, or are recoloured or
 * retextured; each is handed to the renderer with GmEm_UploadMaterial.
 * Em_DrawParts calls it per shown set, the Bulldrome's draw for set 1.
 * Two forms are kept because they give the game's code: the Felyne case's
 * switch on animFrame lists `default:` first, which leaves its branches'
 * delay slots empty as in the game; and in Fatalis B's 32-material loop,
 * texIdx is set by an if/else and shadeDark is computed in the branch for
 * materials 1 and 14, which keeps the loop's two pointers and its counter
 * in the registers the game's code uses. */
void GmEm_PrepareMeshSet(Em *em, s32 set, ModelMatSet *sets)
{
    ModelMat *modelMats = em->model->mats;
    ModelMat *material;
    s32 i;
    s32 texIdx;
    s32 kind20Tex;
    s32 cond;
    s16 animFrame;
    EmMeshWork *work;
    f32 shade;
    f32 shadeDark;
    s32 j;
    u8 rageTex;

    switch (em->type) {
    /* The Rathian kinds: meshes hidden as parts 1, 2 and 6 break, and the
     * meshes shown only after the part-break reaction (partBreakCount). */
    case EM_KIND_RATHIAN: case EM_KIND_PINK_RATHIAN: case EM_KIND_GOLD_RATHIAN:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 0:
                    if (em->parts[1].state > 0) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[2].state > 0) material->alpha = 0.0f;
                    break;
                case 5:
                    material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 0:
                case 1:
                case 2:
                    break;
                case 3:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 0:
                case 1:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[2].state == 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[1].state == 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 1:
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Fatalis: the head's meshes by part 5's break stage (set 1, and their
     * broken counterparts in set 4); set 3's textures by parts[7].state. */
    case EM_KIND_FATALIS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 0:
                case 1:
                    if (em->parts[5].state >= 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state > 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 1:
                case 5:
                    if (em->parts[7].state >= 10) texIdx = 2; else texIdx = 1;
                    material->tex = gTexHandles[em->modelSlot * 20 + 0xA3 + texIdx];
                    break;
                case 3:
                case 8:
                    if (em->parts[0].state > 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 1:
                    if (em->parts[5].state < 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state >= 2 || em->parts[5].state <= 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state < 2) material->alpha = 0.0f;
                    break;
                case 0:
                    if (em->parts[0].state <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Yian Kut-Ku kinds: whole and broken meshes by part 6 and the
     * part-break reaction. */
    case EM_KIND_YIAN_KUT_KU: case EM_KIND_BLUE_YIAN_KUT_KU:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 5:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 6:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 0:
                    break;
                case 1:
                    material->alpha = 0.0f;
                    break;
                case 2:
                case 3:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 0:
                    material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Lao-Shan Lung kinds: whole meshes (sets 2 and 4) and their broken
     * counterparts (set 5) by parts 0, 4, 5 and 6. */
    case EM_KIND_LAO_SHAN_LUNG: case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 2:
                switch (i) {
                case 0:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[0].state >= 2) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[0].state > 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->parts[4].state >= 3) material->alpha = 0.0f;
                    break;
                }
                break;
            case 5:
                switch (i) {
                case 0:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[5].state < 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[0].state < 2) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[0].state <= 0) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[4].state < 3) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Cephadrome kinds: Cephadrome tints every material sand-brown;
     * meshes by part 0 and the part-break reaction. */
    case EM_KIND_CEPHADROME: case EM_KIND_CEPHALOS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            if (em->type == EM_KIND_CEPHADROME) {
                material->rgb[0] = 101.0f / 255.0f;
                material->rgb[1] = 96.0f / 255.0f;
                material->rgb[2] = 65.0f / 255.0f;
            }
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 2:
                    material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 5:
                case 6:
                    if (em->type == EM_KIND_CEPHADROME && em->parts[0].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 2:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->parts[0].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Felyne kinds (9, the cart, Melynx): a 10-step cycle picks the face
     * mesh (a blink). Em_DrawParts draws these kinds with their own function. */
    case 9: case EM_KIND_FELYNE_CART: case EM_KIND_MELYNX:
        animFrame = (gFrameTick >> 1) % 10;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
            case 1:
                switch (animFrame) {
                default:
                    switch (i) {
                    case 2:
                    case 3:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 8:
                    switch (i) {
                    case 1:
                    case 3:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 9:
                    switch (i) {
                    case 1:
                    case 2:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Rathalos kinds: as the Rathian kinds, by parts 1, 2 and 6 and the
     * part-break reaction. */
    case EM_KIND_RATHALOS: case EM_KIND_SILVER_RATHALOS: case EM_KIND_AZURE_RATHALOS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 3:
                    if (em->parts[1].state > 0) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[2].state > 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 0:
                case 4:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 1:
                    material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->parts[1].state == 0) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[2].state == 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 4:
                    material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 1:
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The raptors and their leaders: the eyes blink on a 98-frame cycle (one
     * of materials 0-3 shown), closed once hunted; material 9 only in two motions. */
    case EM_KIND_GENPREY: case EM_KIND_VELOCIPREY: case EM_KIND_VELOCIDROME: case EM_KIND_GENDROME: case EM_KIND_IOPREY: case EM_KIND_IODROME: case EM_KIND_GIAPREY:
        work = (EmMeshWork *)&em->work;
        switch (em->type) {
        case 0x1B:
        case 0x1C:
        case 0x1F:
            cond = work->hunted;
            break;
        default:
            cond = work->huntedB;
            break;
        }
        if (cond != 0) animFrame = 0; else animFrame = gScene.frame % 98;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            if (em->type == EM_KIND_VELOCIPREY || em->type == EM_KIND_GIAPREY || em->type == EM_KIND_GENPREY || em->type == EM_KIND_IOPREY) {
                switch (i) {
                case 5:
                    material->alpha = 0.0f;
                    break;
                }
            } else {
                switch (i) {
                case 4:
                    material->alpha = 0.0f;
                    break;
                }
            }
            switch (animFrame) {
            case 0:
            case 1:
            case 2:
            case 3:
                switch (i) {
                case 0:
                case 1:
                case 3:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
            case 5:
                switch (i) {
                case 0:
                case 2:
                case 3:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 6:
            case 7:
                switch (i) {
                case 0:
                case 1:
                case 2:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            default:
                switch (i) {
                case 1:
                case 2:
                case 3:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            }
            switch (i) {
            case 9:
                if (em->motionId[0] != EM_MOTION_ID(0, 40) && em->motionId[0] != EM_MOTION_ID(0, 45)) material->alpha = 0.0f;
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Diablos kinds: meshes by part 6 and the part-break reaction. */
    case EM_KIND_DIABLOS: case EM_KIND_BLACK_DIABLOS:
        if (em->motionId[0] == EM_MOTION_ID(0, 113)) EmMotion_ReachedFrame(em, 0, 10.0f);
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 3:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[6].state > 0) material->alpha = 0.0f;
                    break;
                case 6:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 4:
                    material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 7:
                    if (em->parts[6].state <= 0) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Khezu kinds: meshes by parts 0 and 6 and the part-break reaction. */
    case EM_KIND_KHEZU: case EM_KIND_RED_KHEZU:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 1:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 3:
                    material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->parts[0].state < 2) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Gravios kinds: meshes by part 6 and the part-break reaction. */
    case EM_KIND_GRAVIOS: case EM_KIND_BLACK_GRAVIOS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 3:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 4:
                    if (em->parts[6].state > 1) material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->parts[6].state > 0) material->alpha = 0.0f;
                    break;
                case 6:
                    material->alpha = 0.0f;
                    break;
                case 7:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 0:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[6].state == 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Vespoid and Hornetaur: the wings, one of materials 2-4 on a 9-frame
     * cycle; materials 1 and 5-7 flicker in the flying motions. */
    case EM_KIND_VESPOID: case EM_KIND_HORNETAUR:
        animFrame = gScene.frame % 9;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (i) {
            case 0:
                break;
            case 2:
            case 3:
            case 4:
                switch (animFrame) {
                case 0:
                case 1:
                    switch (i) {
                    case 3:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 3:
                case 4:
                case 5:
                case 6:
                    switch (i) {
                    case 2:
                    case 3:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                default:
                    switch (i) {
                    case 2:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                }
                break;
            case 1:
            case 5:
            case 6:
            case 7:
                switch (em->motionId[0]) {
                case 0x3E9:
                case 0x3EA:
                case 0x3EB:
                case 0x3EE:
                case 0x3EF:
                case 0x3F2:
                case 0x3F3:
                case 0x3F4:
                    if (em->motionId[0] == EM_MOTION_ID(0, 12) && EmMotion_ReachedFrame(em, 0, 72.0f) == 0) {
                        switch (i) {
                        case 6:
                        case 7:
                            material->alpha = 0.0f;
                            break;
                        }
                    } else if (gScene.frame & 1) {
                        switch (i) {
                        case 7:
                            material->alpha = 0.0f;
                            break;
                        }
                    } else {
                        switch (i) {
                        case 1:
                        case 5:
                        case 6:
                            material->alpha = 0.0f;
                            break;
                        }
                    }
                    break;
                default:
                    switch (i) {
                    case 6:
                    case 7:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Plesioth kinds: meshes by part 0 and the part-break reaction. */
    case EM_KIND_PLESIOTH: case EM_KIND_GREEN_PLESIOTH:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 2:
                    material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 5:
                case 6:
                    if (em->parts[0].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 2:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 5:
                switch (i) {
                case 0:
                case 1:
                    if (em->parts[0].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Basarios: meshes by part 6 and the part-break reaction. */
    case EM_KIND_BASARIOS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 2:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 4:
                    if (em->parts[6].state > 0) material->alpha = 0.0f;
                    break;
                case 5:
                    material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 0:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[6].state <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Shakalaka: the mask meshes (picked by variant) hidden from frame 222
     * of its death motion. */
    case EM_KIND_SHAKALAKA:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 0:
                case 2:
                case 3:
                    if (em->motionId[0] == EM_MOTION_ID(0, 68) && EmMotion_ReachedFrame(em, 0, 222.0f) != 0) material->alpha = 0.0f;
                    if (em->variant == 3) {
                        if (i == 2) material->alpha = 0.0f;
                    } else {
                        if (i == 3) material->alpha = 0.0f;
                    }
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Monoblos kinds: material 0 tinted red by the rage fade (Em.rageFade,
     * counted to 60 while enraged); meshes by part 6 and the part-break reaction. */
    case EM_KIND_MONOBLOS: case EM_KIND_WHITE_MONOBLOS:
        shade = 1.0f - 0.172549009f * ((s8) em->rageFade / 60.0f);
        shadeDark = 1.0f - 205.0f / 255.0f * ((s8) em->rageFade / 60.0f);
        if (em->motionId[0] == EM_MOTION_ID(0, 113)) EmMotion_ReachedFrame(em, 0, 10.0f);
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 0:
                    material->rgb[0] = shade;
                    material->rgb[1] = shadeDark;
                    material->rgb[2] = shadeDark;
                    break;
                case 4:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 7:
                    material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 4:
                    material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Fatalis B: set 0 first recolours its 32 materials (dimmed, then lit
     * again) and switches them to the rage textures (+8) as the rage comes and
     * goes; then the meshes as Fatalis'. */
    case EM_KIND_FATALIS_B:
        cond = em->rage;
        work = (EmMeshWork *)&em->work;
        if (cond != 0) {
            if (em->rageStep == 0) {
                if (work->fatalisRageTimer < 30) {
                    rageTex = 0;
                } else {
                    rageTex = 1;
                }
            } else {
                rageTex = 1;
            }
        } else if (work->fatalisRageTimer == -1) {
            rageTex = 0;
        } else if (work->fatalisRageTimer < 30) {
            rageTex = 1;
        } else {
            rageTex = 0;
        }
        if (set == 0) {
            if (work->fatalisRageTimer > 0) {
                if (cond != 0) {
                    if (work->fatalisRageTimer < 30) {
                        shade = 1.1f - 0.9f * (work->fatalisRageTimer / 30.0f);
                    } else {
                        shade = 0.1f + 0.9f * ((work->fatalisRageTimer - 30) / 10.0f);
                    }
                } else {
                    if (work->fatalisRageTimer < 30) {
                        shade = 1.1f - 0.9f * (work->fatalisRageTimer / 30.0f);
                    } else {
                        shade = 0.1f + 0.9f * ((work->fatalisRageTimer - 30) / 20.0f);
                    }
                }
            } else {
                shade = 1.0f;
            }
            for (j = 0; j < 32; j++) {
                texIdx = gGmEmFatalisBMatTex[j];
                if (rageTex && texIdx < 8) texIdx += 8;
                if (j == 1 || j == 14) {
                    shadeDark = 0.7f * shade;
                    modelMats[j].rgb[0] = shadeDark;
                    modelMats[j].rgb[1] = shadeDark;
                    modelMats[j].rgb[2] = shadeDark;
                } else {
                    modelMats[j].rgb[0] = shade;
                    modelMats[j].rgb[1] = shade;
                    modelMats[j].rgb[2] = shade;
                }
                modelMats[j].tex = gTexHandles[em->modelSlot * 20 + 0xA3 + texIdx];
            }
        }
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 0:
                case 1:
                    if (em->parts[5].state >= 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state > 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 1:
                case 5:
                    if (em->parts[7].state >= 10) texIdx = 2; else texIdx = 1;
                    if (rageTex) texIdx += 8;
                    material->tex = gTexHandles[em->modelSlot * 20 + 0xA3 + texIdx];
                    break;
                case 3:
                case 8:
                    if (em->parts[0].state > 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 1:
                    if (em->parts[5].state < 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state >= 2 || em->parts[5].state <= 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state < 2) material->alpha = 0.0f;
                    break;
                case 0:
                    if (em->parts[0].state <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Fatalis C: as Fatalis, and set 6 (the rage glow) lit by the rage timer. */
    case EM_KIND_FATALIS_C:
        work = (EmMeshWork *)&em->work;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 0:
                case 1:
                    if (em->parts[5].state >= 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state > 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 1:
                case 5:
                    if (em->parts[7].state >= 10) texIdx = 2; else texIdx = 1;
                    material->tex = gTexHandles[em->modelSlot * 20 + 0xA3 + texIdx];
                    break;
                case 3:
                case 8:
                    if (em->parts[0].state > 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 1:
                    if (em->parts[5].state < 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state >= 2 || em->parts[5].state <= 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state < 2) material->alpha = 0.0f;
                    break;
                case 0:
                    if (em->parts[0].state <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 6:
                if (em->rage != 0) {
                    switch (em->rageStep) {
                    case 0:
                        if (work->fatalisRageTimer < 0) {
                            shade = 0.0f;
                        } else {
                            shade = work->fatalisRageTimer / 60.0f;
                        }
                        break;
                    case 1:
                    default:
                        shade = 1.0f;
                        break;
                    }
                } else {
                    switch (em->rageStep) {
                    case 0:
                        if (work->fatalisRageTimer < 0) {
                            shade = 0.0f;
                        } else {
                            shade = 1.0f - work->fatalisRageTimer / 60.0f;
                        }
                        break;
                    case 1:
                    default:
                        shade = 1.0f;
                        break;
                    }
                }
                material->rgb[0] = shade;
                material->rgb[1] = shade;
                material->rgb[2] = shade;
                switch (i) {
                case 1:
                    if (em->parts[5].state >= 3) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state < 3) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Kind 0x20: every material gets the slot's texture 1. */
    case 0x20:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            if (i == 0) {
                kind20Tex = 1;
                material->tex = gTexHandles[em->modelSlot * 20 + 0xA3 + kind20Tex];
            }
            material->alpha = em->alpha;
            material->tex = gTexHandles[em->modelSlot * 20 + 0xA3 + kind20Tex];
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Yian Garuga: whole meshes and their broken counterparts by parts 0, 1,
     * 2 and 6, and the part-break reaction. */
    case EM_KIND_YIAN_GARUGA:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 5:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 6:
                    material->alpha = 0.0f;
                    break;
                case 7:
                    if (em->parts[1].state >= 2) material->alpha = 0.0f;
                    break;
                case 8:
                    if (em->parts[2].state >= 2) material->alpha = 0.0f;
                    break;
                case 9:
                    if (em->parts[0].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 0:
                    material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[6].state >= 3) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 0:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->parts[1].state < 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[6].state < 3) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[2].state < 2) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[0].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Blangonga kinds: material 0 coloured by the work area's tint;
     * meshes by part 2 and the part-break reaction; a day mesh pair while
     * defeated. */
    case EM_KIND_BLANGONGA: case EM_KIND_BLANGO:
        work = (EmMeshWork *)&em->work;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            if (em->type == EM_KIND_BLANGONGA) {
                switch (set) {
                case 0:
                    switch (i) {
                    case 0:
                        Color_RgbToFloat(material->rgb, 0xFF000000 | (work->blangTint[0] << 16) | (work->blangTint[1] << 8) | work->blangTint[2]);
                        break;
                    case 4:
                        if (em->partBreakCount > 0) material->alpha = 0.0f;
                        break;
                    case 5:
                        if (Em_PartBroken(em, 2) == 1) material->alpha = 0.0f;
                        break;
                    }
                    break;
                }
            }
            switch (set) {
            case 0:
                switch (i) {
                case 0:
                    Color_RgbToFloat(material->rgb, 0xFF000000 | (work->blangTint[0] << 16) | (work->blangTint[1] << 8) | work->blangTint[2]);
                    break;
                case 1:
                    if (Area_IsDay(gScene.area) == 0 || em->stateGroup != EM_GROUP_DEFEAT) material->alpha = 0.0f;
                    break;
                }
                break;
            case 1:
                switch (i) {
                case 0:
                    if (Area_IsDay(gScene.area) == 1 || em->stateGroup == EM_GROUP_DEFEAT) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Congalala kinds: meshes by parts 0, 2 and 3; the carried item's and
     * the food's meshes (sets 6-8); the tint and the day pair as Blangonga. */
    case EM_KIND_CONGALALA: case EM_KIND_CONGA:
        work = (EmMeshWork *)&em->work;
        rageTex = EmBlangonga_EatPropMode(em);
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            if (em->type == EM_KIND_CONGALALA) {
                switch (set) {
                case 0:
                    switch (i) {
                    case 0:
                        if (em->parts[0].state >= 2) material->alpha = 0.0f;
                        break;
                    case 3:
                        if (em->parts[2].state >= 2) material->alpha = 0.0f;
                        break;
                    case 4:
                        if (em->parts[3].state >= 2) material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 1:
                    switch (i) {
                    case 0:
                    case 1:
                        if (em->parts[0].state < 2) material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 2:
                    material->alpha = 0.0f;
                    break;
                case 4:
                    material->alpha = 0.0f;
                    break;
                case 6:
                    if (EmBlangonga_CarriedShown(em, 0) == 0 && EmBlangonga_CarriedShown(em, 1) == 0 && EmBlangonga_CarriedShown(em, 2) == 0 && rageTex != 1) material->alpha = 0.0f;
                    break;
                case 7:
                    if (EmBlangonga_CarriedShown(em, 0) == 1 || EmBlangonga_CarriedShown(em, 1) == 1 || EmBlangonga_CarriedShown(em, 2) == 1 || rageTex == 1) material->alpha = 0.0f;
                    break;
                case 8:
                    switch (i) {
                    case 0:
                        if (EmBlangonga_CarriedShown(em, 0) == 0 || rageTex != 0) material->alpha = 0.0f;
                        break;
                    case 1:
                        if (EmBlangonga_CarriedShown(em, 1) == 0 && rageTex != 1) material->alpha = 0.0f;
                        break;
                    case 2:
                        if (EmBlangonga_CarriedShown(em, 2) == 0 || rageTex != 0) material->alpha = 0.0f;
                        break;
                    }
                    break;
                }
            } else {
                switch (set) {
                case 0:
                    switch (i) {
                    case 0:
                    case 2:
                    case 3:
                    case 4:
                        material->alpha = 0.0f;
                        break;
                    }
                    break;
                case 5:
                case 6:
                case 7:
                case 8:
                    material->alpha = 0.0f;
                    break;
                }
            }
            switch (set) {
            case 0:
                switch (i) {
                case 1:
                    Color_RgbToFloat(material->rgb, 0xFF000000 | (work->blangTint[0] << 16) | (work->blangTint[1] << 8) | work->blangTint[2]);
                    break;
                case 7:
                    if (Area_IsDay(gScene.area) == 0 || em->stateGroup != EM_GROUP_DEFEAT) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                if (Area_IsDay(gScene.area) == 1 || em->stateGroup == EM_GROUP_DEFEAT) material->alpha = 0.0f;
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Rajang: the rage glow (sets 3-5) faded in by the rage timer (/ 15);
     * meshes by part 0 and the part-break reaction. */
    case EM_KIND_RAJANG:
        work = (EmMeshWork *)&em->work;
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 5:
                    if (em->parts[0].state > 0) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[0].state >= 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                if (em->partBreakCount > 0) material->alpha = 0.0f;
                break;
            case 3:
                switch (i) {
                case 0:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 1:
                    if (em->partBreakCount <= 0) {
                        material->alpha = 0.0f;
                    } else {
                        material->alpha *= work->blangRageTimer / 15.0f;
                    }
                    break;
                }
                break;
            case 4:
                material->alpha *= work->blangRageTimer / 15.0f;
                break;
            case 5:
                switch (i) {
                case 0:
                    material->alpha *= work->blangRageTimer / 15.0f;
                    break;
                case 1:
                    if (EmBlangonga_CarriedShown(em, 6) == 1) {
                        material->alpha *= work->blangRageTimer / 15.0f;
                    } else {
                        material->alpha = 0.0f;
                    }
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Shen Gaoren: set 0's materials coloured by the flinch level of parts 5,
     * 6, 3 and 4 (GmEm_ShenGaorenFlinchColor); set 4 by part 2's stage. */
    case EM_KIND_SHEN_GAOREN:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 0:
                    GmEm_ShenGaorenFlinchColor(em, material, 5);
                    break;
                case 1:
                    GmEm_ShenGaorenFlinchColor(em, material, 6);
                    break;
                case 2:
                    GmEm_ShenGaorenFlinchColor(em, material, 3);
                    break;
                case 3:
                    GmEm_ShenGaorenFlinchColor(em, material, 4);
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    break;
                case 1:
                    if (em->parts[2].state >= 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[2].state < 2) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[2].state >= 4) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[2].state < 4) material->alpha = 0.0f;
                    break;
                case 5:
                    if (em->parts[2].state >= 4) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[2].state < 4) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Daimyo Hermitaur: whole and broken pairs by parts 5 and 6; the
     * shell (set 5) by the part-break count. */
    case EM_KIND_DAIMYO_HERMITAUR:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 1:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->parts[5].state >= 2) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->parts[6].state < 2) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->parts[5].state < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            case 5:
                switch (i) {
                case 1:
                    if (em->partBreakCount > 0) material->alpha = 0.0f;
                    break;
                case 2:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 3:
                    if (em->partBreakCount >= 2) material->alpha = 0.0f;
                    break;
                case 4:
                    if (em->partBreakCount < 2) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Shogun Ceanataur: calm and enraged meshes, broken meshes by parts 5
     * and 6; the shell (set 3) picked by variant, hidden once part 2 breaks. */
    case EM_KIND_SHOGUN_CEANATAUR:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 0:
                switch (i) {
                case 1:
                case 4:
                    if (em->rage == 1) material->alpha = 0.0f;
                    if (i == 1) goto part6;
                    goto part5;
                case 0:
                case 3:
                    if (em->rage == 0) material->alpha = 0.0f;
                    if (i != 0) goto part5;
                case 2:
                part6:
                    if (em->parts[6].state > 0) material->alpha = 0.0f;
                    break;
                case 5:
                part5:
                    if (em->parts[5].state > 0) material->alpha = 0.0f;
                    break;
                case 6:
                    if (em->parts[6].state <= 0) material->alpha = 0.0f;
                    break;
                case 7:
                    if (em->parts[5].state <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                if (i != em->variant) {
                    material->alpha = 0.0f;
                } else if (em->parts[2].state >= 2) {
                    material->alpha = 0.0f;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Lunastra and Teostra: whole and broken meshes by Em_PartBroken(0) and
     * (1), and the part-break reaction. */
    case EM_KIND_LUNASTRA: case EM_KIND_TEOSTRA:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 3:
                    if (Em_PartBroken(em, 0) == 1) material->alpha = 0.0f;
                    break;
                case 4:
                    if (Em_PartBroken(em, 0) != 1) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 4:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                case 5:
                case 6:
                    if (Em_PartBroken(em, 1) == 1) material->alpha = 0.0f;
                    break;
                case 7:
                    if (Em_PartBroken(em, 1) != 1) material->alpha = 0.0f;
                    break;
                case 8:
                case 9:
                    if (Em_PartBroken(em, 1) == 1) material->alpha = 0.0f;
                    break;
                case 10:
                    if (Em_PartBroken(em, 1) != 1) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The Kushala Daora kinds: whole and broken meshes by Em_PartBroken(0)
     * and (1), and the part-break reaction. */
    case EM_KIND_KUSHALA_DAORA: case EM_KIND_RUSTED_KUSHALA_DAORA:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 2:
                    if (Em_PartBroken(em, 0) == 1) material->alpha = 0.0f;
                    break;
                case 3:
                    if (Em_PartBroken(em, 0) != 1) material->alpha = 0.0f;
                    break;
                }
                break;
            case 3:
                switch (i) {
                case 4:
                    if (Em_PartBroken(em, 1) == 1) material->alpha = 0.0f;
                    break;
                case 5:
                    if (Em_PartBroken(em, 1) != 1) material->alpha = 0.0f;
                    break;
                case 6:
                    if (Em_PartBroken(em, 1) == 1) material->alpha = 0.0f;
                    break;
                case 7:
                    if (Em_PartBroken(em, 1) != 1) material->alpha = 0.0f;
                    break;
                case 8:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Chameleos: meshes by part 0 and the part-break reaction; set 8's
     * material 1 only while it can turn invisible. */
    case EM_KIND_CHAMELEOS:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 1:
                switch (i) {
                case 1:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 2:
                switch (i) {
                case 1:
                    if (Em_PartBroken(em, 0) == 1) material->alpha = 0.0f;
                    break;
                case 2:
                    if (Em_PartBroken(em, 0) == 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 4:
                switch (i) {
                case 0:
                    if (em->partBreakCount <= 0) material->alpha = 0.0f;
                    break;
                }
                break;
            case 8:
                switch (i) {
                case 0:
                    if (Em_PartBroken(em, 0) == 1) material->alpha = 0.0f;
                    break;
                case 1:
                    if (EmChameleos_CanTurnInvisible(em) == 0) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* Yama Tsukami: set 10's meshes dropped as part 6's state grows. */
    case EM_KIND_YAMA_TSUKAMI:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            switch (set) {
            case 10:
                switch (i) {
                case 0:
                    if (em->parts[6].state >= 2) material->alpha = 0.0f;
                    break;
                case 1:
                case 2:
                    if (em->parts[6].state >= 6) material->alpha = 0.0f;
                    break;
                case 3:
                case 4:
                    if (em->parts[6].state >= 4) material->alpha = 0.0f;
                    break;
                case 5:
                case 6:
                    if (em->parts[6].state >= 8) material->alpha = 0.0f;
                    break;
                }
                break;
            }
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    /* The other kinds: the alpha only. */
    default:
        for (i = 0; i < sets[set].count; i++) {
            material = &modelMats[sets[set].mat[i]];
            material->alpha = em->alpha;
            GmEm_UploadMaterial((s32)em, material, i);
        }
        break;
    }
}

/* Bulldrome, with its current motion: while it is enraged, every 45 frames
 * of its idle and moving motions puts a rage aura (monster effect 0xD,
 * 0.4 times its scale) on it. */
void GmEm_SpawnBulldromeRageAura(Em *em, s16 motion)
{
    switch (motion) {
    case EM_MOTION_ID(0, 1):
    case EM_MOTION_ID(0, 2):
    case EM_MOTION_ID(0, 3):
    case EM_MOTION_ID(0, 4):
    case EM_MOTION_ID(0, 5):
    case EM_MOTION_ID(0, 7):
    case EM_MOTION_ID(0, 10):
    case EM_MOTION_ID(0, 11):
    case EM_MOTION_ID(0, 14):
    case EM_MOTION_ID(0, 17):
        if (em->rage != 0 && gScene.frame % 45 == 0) {
            MonsterFx_CreateOnEm(em, 0xD, 0.4f * em->scaleX);
        }
        break;
    }
}

/* The file's .data: the per-kind monster tables, read by the game
 * overlay (gEmLowHpRatios, gEmScaleRows, gEmBaseHp, gEmVersusHpFactors, gEmSizeLevelFactors there) and by
 * GmEm_SetupKind (gGmEmCallbackTbl), GmEm_PrepareMeshSet (gGmEmFatalisBMatTex) and gm_load.c
 * (the lists of gGmKindFileLists). Generated by tools/gen_data.py: u32 words,
 * pointers as C; give them types when their users are worked on. */
void GmEm_Nop(void);
void GmEm_SpawnEffect01(Em *em);
s32 GmEm_ReturnZero(void);
extern u32 gGmEmCallbackTbl_0[];

u32 D_0090ACC0[8] = {
#include "gm_sub/0090ACC0.inc"
};
u32 D_0090ACE0[8] = {
#include "gm_sub/0090ACE0.inc"
};
u32 D_0090AD00[8] = {
#include "gm_sub/0090AD00.inc"
};
u32 D_0090AD20[8] = {
#include "gm_sub/0090AD20.inc"
};
u32 D_0090AD40[8] = {
#include "gm_sub/0090AD40.inc"
};
u32 D_0090AD60[8] = {
#include "gm_sub/0090AD60.inc"
};
u32 D_0090AD80[8] = {
#include "gm_sub/0090AD80.inc"
};
u32 D_0090ADA0[8] = {
#include "gm_sub/0090ADA0.inc"
};
u32 D_0090ADC0[8] = {
#include "gm_sub/0090ADC0.inc"
};
u32 D_0090ADE0[8] = {
#include "gm_sub/0090ADE0.inc"
};
u32 D_0090AE00[8] = {
#include "gm_sub/0090AE00.inc"
};
u32 D_0090AE20[8] = {
#include "gm_sub/0090AE20.inc"
};
u32 D_0090AE40[8] = {
#include "gm_sub/0090AE40.inc"
};
u32 D_0090AE60[8] = {
#include "gm_sub/0090AE60.inc"
};
u32 D_0090AE80[8] = {
#include "gm_sub/0090AE80.inc"
};
u32 D_0090AEA0[8] = {
#include "gm_sub/0090AEA0.inc"
};
u32 D_0090AEC0[8] = {
#include "gm_sub/0090AEC0.inc"
};
u32 D_0090AEE0[8] = {
#include "gm_sub/0090AEE0.inc"
};
u32 D_0090AF00[8] = {
#include "gm_sub/0090AF00.inc"
};
u32 D_0090AF20[8] = {
#include "gm_sub/0090AF20.inc"
};
u32 D_0090AF40[8] = {
#include "gm_sub/0090AF40.inc"
};
u32 D_0090AF60[8] = {
#include "gm_sub/0090AF60.inc"
};
u32 D_0090AF80[8] = {
#include "gm_sub/0090AF80.inc"
};
u32 D_0090AFA0[8] = {
#include "gm_sub/0090AFA0.inc"
};
u32 D_0090AFC0[8] = {
#include "gm_sub/0090AFC0.inc"
};
u32 D_0090AFE0[8] = {
#include "gm_sub/0090AFE0.inc"
};
u32 D_0090B000[8] = {
#include "gm_sub/0090B000.inc"
};
u32 D_0090B020[8] = {
#include "gm_sub/0090B020.inc"
};
u32 D_0090B040[8] = {
#include "gm_sub/0090B040.inc"
};
u32 D_0090B060[8] = {
#include "gm_sub/0090B060.inc"
};
u32 D_0090B080[8] = {
#include "gm_sub/0090B080.inc"
};
u32 D_0090B0A0[8] = {
#include "gm_sub/0090B0A0.inc"
};
u32 D_0090B0C0[8] = {
#include "gm_sub/0090B0C0.inc"
};
u32 D_0090B0E0[8] = {
#include "gm_sub/0090B0E0.inc"
};
u32 D_0090B100[8] = {
#include "gm_sub/0090B100.inc"
};
u32 D_0090B120[8] = {
#include "gm_sub/0090B120.inc"
};
u32 D_0090B140[8] = {
#include "gm_sub/0090B140.inc"
};
u32 D_0090B160[8] = {
#include "gm_sub/0090B160.inc"
};
u32 D_0090B180[8] = {
#include "gm_sub/0090B180.inc"
};
u32 D_0090B1A0[8] = {
#include "gm_sub/0090B1A0.inc"
};
u32 D_0090B1C0[8] = {
#include "gm_sub/0090B1C0.inc"
};
u32 D_0090B1E0[8] = {
#include "gm_sub/0090B1E0.inc"
};
u32 D_0090B200[8] = {
#include "gm_sub/0090B200.inc"
};
u32 D_0090B220[8] = {
#include "gm_sub/0090B220.inc"
};
u32 D_0090B240[8] = {
#include "gm_sub/0090B240.inc"
};
u32 D_0090B260[8] = {
#include "gm_sub/0090B260.inc"
};
u32 D_0090B280[8] = {
#include "gm_sub/0090B280.inc"
};
u32 D_0090B2A0[8] = {
#include "gm_sub/0090B2A0.inc"
};
u32 D_0090B2C0[8] = {
#include "gm_sub/0090B2C0.inc"
};
u32 D_0090B2E0[8] = {
#include "gm_sub/0090B2E0.inc"
};
u32 D_0090B300[8] = {
#include "gm_sub/0090B300.inc"
};
u32 D_0090B320[8] = {
#include "gm_sub/0090B320.inc"
};
u32 D_0090B340[8] = {
#include "gm_sub/0090B340.inc"
};
u32 D_0090B360[8] = {
#include "gm_sub/0090B360.inc"
};
u32 D_0090B380[8] = {
#include "gm_sub/0090B380.inc"
};
u32 D_0090B3A0[84] = {
#include "gm_sub/0090B3A0.inc"
};
u32 D_0090B4F0[18] = {
#include "gm_sub/0090B4F0.inc"
};
u32 D_0090B538[2] = {
    (u32)D_0090B4F0, 0x00000000,
};
u32 D_0090B540[18] = {
#include "gm_sub/0090B540.inc"
};
u32 D_0090B588[2] = {
    (u32)D_0090B540, 0x00000000,
};
u32 D_0090B590[18] = {
#include "gm_sub/0090B590.inc"
};
u32 D_0090B5D8[2] = {
    (u32)D_0090B590, 0x00000000,
};
u32 D_0090B5E0[18] = {
#include "gm_sub/0090B5E0.inc"
};
u32 D_0090B628[2] = {
    (u32)D_0090B5E0, 0x00000000,
};
u32 D_0090B630[18] = {
#include "gm_sub/0090B630.inc"
};
u32 D_0090B678[2] = {
    (u32)D_0090B630, 0x00000000,
};
u32 D_0090B680[18] = {
#include "gm_sub/0090B680.inc"
};
u32 D_0090B6C8[2] = {
    (u32)D_0090B680, 0x00000000,
};
u32 D_0090B6D0[18] = {
#include "gm_sub/0090B6D0.inc"
};
u32 D_0090B718[2] = {
    (u32)D_0090B6D0, 0x00000000,
};
u32 D_0090B720[18] = {
#include "gm_sub/0090B720.inc"
};
u32 D_0090B768[2] = {
    (u32)D_0090B720, 0x00000000,
};
u32 D_0090B770[18] = {
#include "gm_sub/0090B770.inc"
};
u32 D_0090B7B8[2] = {
    (u32)D_0090B770, 0x00000000,
};
u32 D_0090B7C0[18] = {
#include "gm_sub/0090B7C0.inc"
};
u32 D_0090B808[2] = {
    (u32)D_0090B7C0, 0x00000000,
};
u32 D_0090B810[18] = {
#include "gm_sub/0090B810.inc"
};
u32 D_0090B858[2] = {
    (u32)D_0090B810, 0x00000000,
};
u32 D_0090B860[18] = {
#include "gm_sub/0090B860.inc"
};
u32 D_0090B8A8[2] = {
    (u32)D_0090B860, 0x00000000,
};
u32 D_0090B8B0[18] = {
#include "gm_sub/0090B8B0.inc"
};
u32 D_0090B8F8[2] = {
    (u32)D_0090B8B0, 0x00000000,
};
u32 D_0090B900[18] = {
#include "gm_sub/0090B900.inc"
};
u32 D_0090B948[2] = {
    (u32)D_0090B900, 0x00000000,
};
u32 D_0090B950[18] = {
#include "gm_sub/0090B950.inc"
};
u32 D_0090B998[2] = {
    (u32)D_0090B950, 0x00000000,
};
u32 D_0090B9A0[18] = {
#include "gm_sub/0090B9A0.inc"
};
u32 D_0090B9E8[2] = {
    (u32)D_0090B9A0, 0x00000000,
};
u32 D_0090B9F0[20] = {
#include "gm_sub/0090B9F0.inc"
};
u32 D_0090BA40[18] = {
#include "gm_sub/0090BA40.inc"
};
u32 D_0090BA88[2] = {
    (u32)D_0090B9F0, (u32)D_0090BA40,
};
u32 D_0090BA90[18] = {
#include "gm_sub/0090BA90.inc"
};
u32 D_0090BAD8[2] = {
    (u32)D_0090BA90, 0x00000000,
};
u32 D_0090BAE0[18] = {
#include "gm_sub/0090BAE0.inc"
};
u32 D_0090BB28[2] = {
    (u32)D_0090BAE0, 0x00000000,
};
u32 D_0090BB30[18] = {
#include "gm_sub/0090BB30.inc"
};
u32 D_0090BB78[2] = {
    (u32)D_0090BB30, 0x00000000,
};
u32 D_0090BB80[18] = {
#include "gm_sub/0090BB80.inc"
};
u32 D_0090BBC8[2] = {
    (u32)D_0090BB80, 0x00000000,
};
u32 D_0090BBD0[18] = {
#include "gm_sub/0090BBD0.inc"
};
u32 D_0090BC18[2] = {
    (u32)D_0090BBD0, 0x00000000,
};
u32 D_0090BC20[18] = {
#include "gm_sub/0090BC20.inc"
};
u32 D_0090BC68[2] = {
    (u32)D_0090BC20, 0x00000000,
};
u32 D_0090BC70[18] = {
#include "gm_sub/0090BC70.inc"
};
u32 D_0090BCB8[2] = {
    (u32)D_0090BC70, 0x00000000,
};
u32 D_0090BCC0[18] = {
#include "gm_sub/0090BCC0.inc"
};
u32 D_0090BD08[2] = {
    (u32)D_0090BCC0, 0x00000000,
};
u32 D_0090BD10[18] = {
#include "gm_sub/0090BD10.inc"
};
u32 D_0090BD58[2] = {
    (u32)D_0090BD10, 0x00000000,
};
u32 D_0090BD60[18] = {
#include "gm_sub/0090BD60.inc"
};
u32 D_0090BDA8[2] = {
    (u32)D_0090BD60, 0x00000000,
};
u32 D_0090BDB0[18] = {
#include "gm_sub/0090BDB0.inc"
};
u32 D_0090BDF8[2] = {
    (u32)D_0090BDB0, 0x00000000,
};
u32 D_0090BE00[18] = {
#include "gm_sub/0090BE00.inc"
};
u32 D_0090BE48[2] = {
    (u32)D_0090BE00, 0x00000000,
};
u32 D_0090BE50[18] = {
#include "gm_sub/0090BE50.inc"
};
u32 D_0090BE98[2] = {
    (u32)D_0090BE50, 0x00000000,
};
u32 D_0090BEA0[18] = {
#include "gm_sub/0090BEA0.inc"
};
u32 D_0090BEE8[2] = {
    (u32)D_0090BEA0, 0x00000000,
};
u32 D_0090BEF0[18] = {
#include "gm_sub/0090BEF0.inc"
};
u32 D_0090BF38[2] = {
    (u32)D_0090BEF0, 0x00000000,
};
u32 D_0090BF40[18] = {
#include "gm_sub/0090BF40.inc"
};
u32 D_0090BF88[2] = {
    (u32)D_0090BF40, 0x00000000,
};
u32 D_0090BF90[20] = {
#include "gm_sub/0090BF90.inc"
};
u32 D_0090BFE0[18] = {
#include "gm_sub/0090BFE0.inc"
};
u32 D_0090C028[2] = {
    (u32)D_0090BF90, (u32)D_0090BFE0,
};
u32 D_0090C030[18] = {
#include "gm_sub/0090C030.inc"
};
u32 D_0090C078[2] = {
    (u32)D_0090C030, 0x00000000,
};
u32 D_0090C080[18] = {
#include "gm_sub/0090C080.inc"
};
u32 D_0090C0C8[2] = {
    (u32)D_0090C080, 0x00000000,
};
u32 D_0090C0D0[18] = {
#include "gm_sub/0090C0D0.inc"
};
u32 D_0090C118[2] = {
    (u32)D_0090C0D0, 0x00000000,
};
u32 D_0090C120[18] = {
#include "gm_sub/0090C120.inc"
};
u32 D_0090C168[2] = {
    (u32)D_0090C120, 0x00000000,
};
u32 D_0090C170[18] = {
#include "gm_sub/0090C170.inc"
};
u32 D_0090C1B8[2] = {
    (u32)D_0090C170, 0x00000000,
};
u32 D_0090C1C0[20] = {
#include "gm_sub/0090C1C0.inc"
};
u32 D_0090C210[18] = {
#include "gm_sub/0090C210.inc"
};
u32 D_0090C258[2] = {
    (u32)D_0090C1C0, (u32)D_0090C210,
};
u32 D_0090C260[20] = {
#include "gm_sub/0090C260.inc"
};
u32 D_0090C2B0[20] = {
#include "gm_sub/0090C2B0.inc"
};
u32 D_0090C300[18] = {
#include "gm_sub/0090C300.inc"
};
u32 D_0090C348[3] = {
    (u32)D_0090C260, (u32)D_0090C2B0, (u32)D_0090C300,
};
u32 D_0090C360[18] = {
#include "gm_sub/0090C360.inc"
};
u32 D_0090C3A8[2] = {
    (u32)D_0090C360, 0x00000000,
};
u32 D_0090C3B0[18] = {
#include "gm_sub/0090C3B0.inc"
};
u32 D_0090C3F8[2] = {
    (u32)D_0090C3B0, 0x00000000,
};
u32 D_0090C400[20] = {
#include "gm_sub/0090C400.inc"
};
u32 D_0090C450[18] = {
#include "gm_sub/0090C450.inc"
};
u32 D_0090C498[2] = {
    (u32)D_0090C400, (u32)D_0090C450,
};
u32 D_0090C4A0[18] = {
#include "gm_sub/0090C4A0.inc"
};
u32 D_0090C4E8[2] = {
    (u32)D_0090C4A0, 0x00000000,
};
u32 D_0090C4F0[20] = {
#include "gm_sub/0090C4F0.inc"
};
u32 D_0090C540[18] = {
#include "gm_sub/0090C540.inc"
};
u32 D_0090C588[2] = {
    (u32)D_0090C4F0, (u32)D_0090C540,
};
u32 D_0090C590[18] = {
#include "gm_sub/0090C590.inc"
};
u32 D_0090C5D8[2] = {
    (u32)D_0090C590, 0x00000000,
};
u32 D_0090C5E0[18] = {
#include "gm_sub/0090C5E0.inc"
};
u32 D_0090C628[2] = {
    (u32)D_0090C5E0, 0x00000000,
};
u32 D_0090C630[18] = {
#include "gm_sub/0090C630.inc"
};
u32 D_0090C678[2] = {
    (u32)D_0090C630, 0x00000000,
};
u32 D_0090C680[18] = {
#include "gm_sub/0090C680.inc"
};
u32 D_0090C6C8[2] = {
    (u32)D_0090C680, 0x00000000,
};
u32 D_0090C6D0[20] = {
#include "gm_sub/0090C6D0.inc"
};
u32 D_0090C720[18] = {
#include "gm_sub/0090C720.inc"
};
u32 D_0090C768[2] = {
    (u32)D_0090C6D0, (u32)D_0090C720,
};
u32 D_0090C770[18] = {
#include "gm_sub/0090C770.inc"
};
u32 D_0090C7B8[2] = {
    (u32)D_0090C770, 0x00000000,
};
u32 D_0090C7C0[18] = {
#include "gm_sub/0090C7C0.inc"
};
u32 D_0090C808[2] = {
    (u32)D_0090C7C0, 0x00000000,
};
u32 D_0090C810[18] = {
#include "gm_sub/0090C810.inc"
};
u32 D_0090C858[2] = {
    (u32)D_0090C810, 0x00000000,
};
u32 D_0090C860[20] = {
#include "gm_sub/0090C860.inc"
};
u32 D_0090C8B0[18] = {
#include "gm_sub/0090C8B0.inc"
};
u32 D_0090C8F8[2] = {
    (u32)D_0090C860, (u32)D_0090C8B0,
};
u32 D_0090C900[18] = {
#include "gm_sub/0090C900.inc"
};
u32 D_0090C948[2] = {
    (u32)D_0090C900, 0x00000000,
};
u32 D_0090C950[20] = {
#include "gm_sub/0090C950.inc"
};
u32 D_0090C9A0[18] = {
#include "gm_sub/0090C9A0.inc"
};
u32 D_0090C9E8[2] = {
    (u32)D_0090C950, (u32)D_0090C9A0,
};
u32 D_0090C9F0[18] = {
#include "gm_sub/0090C9F0.inc"
};
u32 D_0090CA38[2] = {
    (u32)D_0090C9F0, 0x00000000,
};
u32 D_0090CA40[18] = {
#include "gm_sub/0090CA40.inc"
};
u32 D_0090CA88[2] = {
    (u32)D_0090CA40, 0x00000000,
};
u32 D_0090CA90[18] = {
#include "gm_sub/0090CA90.inc"
};
u32 D_0090CAD8[2] = {
    (u32)D_0090CA90, 0x00000000,
};
u32 D_0090CAE0[20] = {
#include "gm_sub/0090CAE0.inc"
};
u32 D_0090CB30[18] = {
#include "gm_sub/0090CB30.inc"
};
u32 D_0090CB78[2] = {
    (u32)D_0090CAE0, (u32)D_0090CB30,
};
u32 D_0090CB80[172] = {
#include "gm_sub/0090CB80.inc"
};
u32 D_0090CE30[80] = {
#include "gm_sub/0090CE30.inc"
};
u32 D_0090CF70[80] = {
#include "gm_sub/0090CF70.inc"
};
u32 D_0090D0B0[80] = {
#include "gm_sub/0090D0B0.inc"
};
u32 D_0090D1F0[80] = {
#include "gm_sub/0090D1F0.inc"
};
u32 D_0090D330[154] = {
#include "gm_sub/0090D330.inc"
};
u32 D_0090D598[2] = {
#include "gm_sub/0090D598.inc"
};
u32 D_0090D5A0[4] = {
#include "gm_sub/0090D5A0.inc"
};
u32 D_0090D5B0[216] = {
#include "gm_sub/0090D5B0.inc"
};
u32 D_0090D910[2] = {
#include "gm_sub/0090D910.inc"
};
u32 D_0090D918[2] = {
#include "gm_sub/0090D918.inc"
};
u32 D_0090D920[2] = {
#include "gm_sub/0090D920.inc"
};
u32 D_0090D928[2] = {
#include "gm_sub/0090D928.inc"
};
u32 D_0090D930[2] = {
#include "gm_sub/0090D930.inc"
};
u32 D_0090D938[2] = {
#include "gm_sub/0090D938.inc"
};
u32 D_0090D940[2] = {
#include "gm_sub/0090D940.inc"
};
u32 D_0090D948[2] = {
#include "gm_sub/0090D948.inc"
};
u32 D_0090D950[2] = {
#include "gm_sub/0090D950.inc"
};
u32 D_0090D958[2] = {
#include "gm_sub/0090D958.inc"
};
u32 D_0090D960[2] = {
#include "gm_sub/0090D960.inc"
};
u32 D_0090D968[2] = {
#include "gm_sub/0090D968.inc"
};
u32 D_0090D970[2] = {
#include "gm_sub/0090D970.inc"
};
u32 D_0090D978[2] = {
#include "gm_sub/0090D978.inc"
};
u32 D_0090D980[2] = {
#include "gm_sub/0090D980.inc"
};
u32 D_0090D988[2] = {
#include "gm_sub/0090D988.inc"
};
u32 D_0090D990[2] = {
#include "gm_sub/0090D990.inc"
};
u32 D_0090D998[2] = {
#include "gm_sub/0090D998.inc"
};
u32 D_0090D9A0[2] = {
#include "gm_sub/0090D9A0.inc"
};
u32 D_0090D9A8[2] = {
#include "gm_sub/0090D9A8.inc"
};
u32 D_0090D9B0[2] = {
#include "gm_sub/0090D9B0.inc"
};
u32 D_0090D9B8[2] = {
#include "gm_sub/0090D9B8.inc"
};
u32 D_0090D9C0[2] = {
#include "gm_sub/0090D9C0.inc"
};
u32 D_0090D9C8[2] = {
#include "gm_sub/0090D9C8.inc"
};
u32 D_0090D9D0[2] = {
#include "gm_sub/0090D9D0.inc"
};
u32 D_0090D9D8[2] = {
#include "gm_sub/0090D9D8.inc"
};
u32 D_0090D9E0[2] = {
#include "gm_sub/0090D9E0.inc"
};
u32 D_0090D9E8[2] = {
#include "gm_sub/0090D9E8.inc"
};
u32 D_0090D9F0[2] = {
#include "gm_sub/0090D9F0.inc"
};
u32 D_0090D9F8[2] = {
#include "gm_sub/0090D9F8.inc"
};
u32 D_0090DA00[2] = {
#include "gm_sub/0090DA00.inc"
};
u32 D_0090DA08[2] = {
#include "gm_sub/0090DA08.inc"
};
u32 D_0090DA10[2] = {
#include "gm_sub/0090DA10.inc"
};
u32 D_0090DA18[2] = {
#include "gm_sub/0090DA18.inc"
};
u32 D_0090DA20[2] = {
#include "gm_sub/0090DA20.inc"
};
u32 D_0090DA28[2] = {
#include "gm_sub/0090DA28.inc"
};
u32 D_0090DA30[2] = {
#include "gm_sub/0090DA30.inc"
};
u32 D_0090DA38[2] = {
#include "gm_sub/0090DA38.inc"
};
u32 D_0090DA40[2] = {
#include "gm_sub/0090DA40.inc"
};
u32 D_0090DA48[2] = {
#include "gm_sub/0090DA48.inc"
};
u32 D_0090DA50[2] = {
#include "gm_sub/0090DA50.inc"
};
u32 D_0090DA58[2] = {
#include "gm_sub/0090DA58.inc"
};
u32 D_0090DA60[2] = {
#include "gm_sub/0090DA60.inc"
};
u32 D_0090DA68[2] = {
#include "gm_sub/0090DA68.inc"
};
u32 D_0090DA70[2] = {
#include "gm_sub/0090DA70.inc"
};
u32 D_0090DA78[2] = {
#include "gm_sub/0090DA78.inc"
};
u32 D_0090DA80[2] = {
#include "gm_sub/0090DA80.inc"
};
u32 D_0090DA88[2] = {
#include "gm_sub/0090DA88.inc"
};
u32 D_0090DA90[2] = {
#include "gm_sub/0090DA90.inc"
};
u32 D_0090DA98[2] = {
#include "gm_sub/0090DA98.inc"
};
u32 D_0090DAA0[2] = {
#include "gm_sub/0090DAA0.inc"
};
u32 D_0090DAA8[2] = {
#include "gm_sub/0090DAA8.inc"
};
u32 D_0090DAB0[2] = {
#include "gm_sub/0090DAB0.inc"
};
u32 D_0090DAB8[2] = {
#include "gm_sub/0090DAB8.inc"
};
u32 D_0090DAC0[2] = {
#include "gm_sub/0090DAC0.inc"
};
u32 D_0090DAC8[2] = {
#include "gm_sub/0090DAC8.inc"
};
u32 D_0090DAD0[2] = {
#include "gm_sub/0090DAD0.inc"
};
u32 D_0090DAD8[2] = {
#include "gm_sub/0090DAD8.inc"
};
u32 D_0090DAE0[2] = {
#include "gm_sub/0090DAE0.inc"
};
u32 D_0090DAE8[2] = {
#include "gm_sub/0090DAE8.inc"
};
u32 D_0090DAF0[2] = {
#include "gm_sub/0090DAF0.inc"
};
u32 D_0090DAF8[2] = {
#include "gm_sub/0090DAF8.inc"
};
u32 D_0090DB00[2] = {
#include "gm_sub/0090DB00.inc"
};
u32 D_0090DB08[2] = {
#include "gm_sub/0090DB08.inc"
};
u32 D_0090DB10[2] = {
#include "gm_sub/0090DB10.inc"
};
u32 D_0090DB18[2] = {
#include "gm_sub/0090DB18.inc"
};
u32 D_0090DB20[78] = {
#include "gm_sub/0090DB20.inc"
};
u32 D_0090DC58[2] = {
#include "gm_sub/0090DC58.inc"
};
u32 D_0090DC60[2] = {
#include "gm_sub/0090DC60.inc"
};
u32 D_0090DC68[2] = {
#include "gm_sub/0090DC68.inc"
};
u32 D_0090DC70[2] = {
#include "gm_sub/0090DC70.inc"
};
u32 D_0090DC78[2] = {
#include "gm_sub/0090DC78.inc"
};
u32 D_0090DC80[2] = {
#include "gm_sub/0090DC80.inc"
};
u32 D_0090DC88[2] = {
#include "gm_sub/0090DC88.inc"
};
u32 D_0090DC90[2] = {
#include "gm_sub/0090DC90.inc"
};
u32 D_0090DC98[2] = {
#include "gm_sub/0090DC98.inc"
};
u32 D_0090DCA0[2] = {
#include "gm_sub/0090DCA0.inc"
};
u32 D_0090DCA8[2] = {
#include "gm_sub/0090DCA8.inc"
};
u32 D_0090DCB0[2] = {
#include "gm_sub/0090DCB0.inc"
};
u32 D_0090DCB8[2] = {
#include "gm_sub/0090DCB8.inc"
};
u32 D_0090DCC0[2] = {
#include "gm_sub/0090DCC0.inc"
};
u32 D_0090DCC8[2] = {
#include "gm_sub/0090DCC8.inc"
};
u32 D_0090DCD0[80] = {
#include "gm_sub/0090DCD0.inc"
};
extern void *EmAnteka_Callbacks[];
extern void *EmApceros_Callbacks[];
extern void *EmBlangonga_Callbacks[];
extern void *EmBulldrome_Callbacks[];
extern void *EmCephadrome_Callbacks[];
extern void *EmChameleos_Callbacks[];
extern void *EmDiablos_Callbacks[];
extern void *EmDrome_Callbacks[];
extern void *EmFatalis_Callbacks[];
extern void *EmGaruga_Callbacks[];
extern void *EmGravios_Callbacks[];
extern void *EmHermitaur_Callbacks[];
extern void *EmKhezu_Callbacks[];
extern void *EmTrader_Callbacks[];
extern void *EmFelyneCart_Callbacks[];
extern void *EmKind1D_Callbacks[];
extern void *EmKind38_Callbacks[];
extern void *EmKirin_Callbacks[];
extern void *EmKushala_Callbacks[];
extern void *EmKutku_Callbacks[];
extern void *EmLaoshan_Callbacks[];
extern void *EmMelynx_Callbacks[];
extern void *EmMosswine_Callbacks[];
extern void *EmPlesioth_Callbacks[];
extern void *EmRathalos_Callbacks[];
extern void *EmRemobra_Callbacks[];
extern void *EmShakalaka_Callbacks[];
extern void *EmShenGaoren_Callbacks[];
extern void *EmVelociprey_Callbacks[];
extern void *EmVespoid_Callbacks[];
extern void *EmYamaTsukami_Callbacks[];
u32 gGmEmCallbackTbl[76] = {
    (u32)gGmEmCallbackTbl_0, (u32)EmRathalos_Callbacks, (u32)EmFatalis_Callbacks, (u32)EmAnteka_Callbacks,
    (u32)EmMosswine_Callbacks, (u32)EmMosswine_Callbacks, (u32)EmKutku_Callbacks, (u32)EmLaoshan_Callbacks,
    (u32)EmCephadrome_Callbacks, (u32)EmMelynx_Callbacks, (u32)EmTrader_Callbacks, (u32)EmRathalos_Callbacks,
    (u32)EmApceros_Callbacks, (u32)EmVelociprey_Callbacks, (u32)EmDiablos_Callbacks, (u32)EmKhezu_Callbacks,
    (u32)EmVelociprey_Callbacks, (u32)EmGravios_Callbacks, (u32)EmFelyneCart_Callbacks, (u32)EmVespoid_Callbacks,
    (u32)EmKutku_Callbacks, (u32)EmPlesioth_Callbacks, (u32)EmGravios_Callbacks, (u32)EmMelynx_Callbacks,
    (u32)EmVespoid_Callbacks, (u32)EmApceros_Callbacks, (u32)EmDiablos_Callbacks, (u32)EmDrome_Callbacks,
    (u32)EmDrome_Callbacks, (u32)EmKind1D_Callbacks, (u32)EmVelociprey_Callbacks, (u32)EmDrome_Callbacks,
    (u32)EmMosswine_Callbacks, (u32)EmKirin_Callbacks, (u32)EmCephadrome_Callbacks, (u32)EmVelociprey_Callbacks,
    (u32)EmFatalis_Callbacks, (u32)EmRathalos_Callbacks, (u32)EmKutku_Callbacks, (u32)EmKutku_Callbacks,
    (u32)EmGaruga_Callbacks, (u32)EmRathalos_Callbacks, (u32)EmRathalos_Callbacks, (u32)EmDiablos_Callbacks,
    (u32)EmDiablos_Callbacks, (u32)EmKhezu_Callbacks, (u32)EmPlesioth_Callbacks, (u32)EmGravios_Callbacks,
    (u32)EmHermitaur_Callbacks, (u32)EmRathalos_Callbacks, (u32)EmLaoshan_Callbacks, (u32)EmBlangonga_Callbacks,
    (u32)EmBlangonga_Callbacks, (u32)EmBlangonga_Callbacks, (u32)EmKushala_Callbacks, (u32)EmShenGaoren_Callbacks,
    (u32)EmKind38_Callbacks, (u32)EmShakalaka_Callbacks, (u32)EmYamaTsukami_Callbacks, (u32)EmChameleos_Callbacks,
    (u32)EmKushala_Callbacks, (u32)EmBlangonga_Callbacks, (u32)EmBlangonga_Callbacks, (u32)EmRemobra_Callbacks,
    (u32)EmKushala_Callbacks, (u32)EmKushala_Callbacks, (u32)EmHermitaur_Callbacks, (u32)EmHermitaur_Callbacks,
    (u32)EmBulldrome_Callbacks, (u32)EmAnteka_Callbacks, (u32)EmApceros_Callbacks, (u32)EmFatalis_Callbacks,
    (u32)EmYamaTsukami_Callbacks, (u32)EmHermitaur_Callbacks, 0x00000000, 0x00000000,
};
/* The texture offsets of the 32 meshes of kind 0x24 (GmEm_PrepareMeshSet). */
s16 gGmEmFatalisBMatTex[40] = {
#include "gm_sub/0090DF40.inc"
};
u32 gGmEmCallbackTbl_0[8] = {
    (u32)GmEm_SpawnEffect01, (u32)GmEm_SpawnEffect01, (u32)GmEm_Nop, (u32)GmEm_Nop,
    (u32)GmEm_ReturnZero, 0x00000000, 0x00000000, 0x00000000,
};
u32 D_0090DFB0[32] = {
#include "gm_sub/0090DFB0.inc"
};
u32 D_0090E030[12] = {
#include "gm_sub/0090E030.inc"
};
u32 D_0090E060[16] = {
#include "gm_sub/0090E060.inc"
};
u32 D_0090E0A0[28] = {
#include "gm_sub/0090E0A0.inc"
};
u32 D_0090E110[48] = {
#include "gm_sub/0090E110.inc"
};
u32 D_0090E1D0[24] = {
#include "gm_sub/0090E1D0.inc"
};
u32 D_0090E230[20] = {
#include "gm_sub/0090E230.inc"
};
u32 D_0090E280[12] = {
#include "gm_sub/0090E280.inc"
};
u32 D_0090E2B0[16] = {
#include "gm_sub/0090E2B0.inc"
};
u32 D_0090E2F0[12] = {
#include "gm_sub/0090E2F0.inc"
};
u32 D_0090E320[8] = {
#include "gm_sub/0090E320.inc"
};
u32 D_0090E340[8] = {
#include "gm_sub/0090E340.inc"
};
u32 D_0090E360[8] = {
#include "gm_sub/0090E360.inc"
};
u32 D_0090E380[8] = {
#include "gm_sub/0090E380.inc"
};
u32 D_0090E3A0[8] = {
#include "gm_sub/0090E3A0.inc"
};
u32 D_0090E3C0[8] = {
#include "gm_sub/0090E3C0.inc"
};
u32 D_0090E3E0[8] = {
#include "gm_sub/0090E3E0.inc"
};
u32 D_0090E400[8] = {
#include "gm_sub/0090E400.inc"
};
u32 D_0090E420[12] = {
#include "gm_sub/0090E420.inc"
};
u32 D_0090E450[12] = {
#include "gm_sub/0090E450.inc"
};
u32 D_0090E480[12] = {
#include "gm_sub/0090E480.inc"
};
u32 D_0090E4B0[24] = {
#include "gm_sub/0090E4B0.inc"
};
u32 D_0090E510[12] = {
#include "gm_sub/0090E510.inc"
};
u32 D_0090E540[8] = {
#include "gm_sub/0090E540.inc"
};
u32 D_0090E560[20] = {
#include "gm_sub/0090E560.inc"
};
u32 D_0090E5B0[12] = {
#include "gm_sub/0090E5B0.inc"
};
u32 D_0090E5E0[20] = {
#include "gm_sub/0090E5E0.inc"
};
u32 D_0090E630[12] = {
#include "gm_sub/0090E630.inc"
};
u32 D_0090E660[8] = {
#include "gm_sub/0090E660.inc"
};
u32 D_0090E680[12] = {
#include "gm_sub/0090E680.inc"
};
u32 D_0090E6B0[12] = {
#include "gm_sub/0090E6B0.inc"
};
u32 D_0090E6E0[12] = {
#include "gm_sub/0090E6E0.inc"
};
u32 D_0090E710[12] = {
#include "gm_sub/0090E710.inc"
};
u32 D_0090E740[12] = {
#include "gm_sub/0090E740.inc"
};
u32 D_0090E770[12] = {
#include "gm_sub/0090E770.inc"
};
u32 D_0090E7A0[20] = {
#include "gm_sub/0090E7A0.inc"
};
u32 D_0090E7F0[16] = {
#include "gm_sub/0090E7F0.inc"
};
u32 D_0090E830[16] = {
#include "gm_sub/0090E830.inc"
};
u32 D_0090E870[12] = {
#include "gm_sub/0090E870.inc"
};
u32 D_0090E8A0[80] = {
#include "gm_sub/0090E8A0.inc"
};
u32 D_0090E9E0[4] = {
#include "gm_sub/0090E9E0.inc"
};
u32 D_0090E9F0[4] = {
#include "gm_sub/0090E9F0.inc"
};
u32 D_0090EA00[4] = {
#include "gm_sub/0090EA00.inc"
};
u32 D_0090EA10[4] = {
#include "gm_sub/0090EA10.inc"
};
u32 D_0090EA20[4] = {
#include "gm_sub/0090EA20.inc"
};
u32 D_0090EA30[4] = {
#include "gm_sub/0090EA30.inc"
};
u32 D_0090EA40[4] = {
#include "gm_sub/0090EA40.inc"
};
u32 D_0090EA50[4] = {
#include "gm_sub/0090EA50.inc"
};
u32 D_0090EA60[4] = {
#include "gm_sub/0090EA60.inc"
};
u32 D_0090EA70[4] = {
#include "gm_sub/0090EA70.inc"
};
u32 D_0090EA80[4] = {
#include "gm_sub/0090EA80.inc"
};
u32 D_0090EA90[4] = {
#include "gm_sub/0090EA90.inc"
};
u32 D_0090EAA0[4] = {
#include "gm_sub/0090EAA0.inc"
};
u32 D_0090EAB0[4] = {
#include "gm_sub/0090EAB0.inc"
};
u32 D_0090EAC0[4] = {
#include "gm_sub/0090EAC0.inc"
};
u32 D_0090EAD0[4] = {
#include "gm_sub/0090EAD0.inc"
};
u32 D_0090EAE0[4] = {
#include "gm_sub/0090EAE0.inc"
};
u32 D_0090EAF0[4] = {
#include "gm_sub/0090EAF0.inc"
};
u32 D_0090EB00[4] = {
#include "gm_sub/0090EB00.inc"
};
u32 D_0090EB10[4] = {
#include "gm_sub/0090EB10.inc"
};
u32 D_0090EB20[4] = {
#include "gm_sub/0090EB20.inc"
};
u32 D_0090EB30[4] = {
#include "gm_sub/0090EB30.inc"
};
u32 D_0090EB40[4] = {
#include "gm_sub/0090EB40.inc"
};
u32 D_0090EB50[4] = {
#include "gm_sub/0090EB50.inc"
};
u32 D_0090EB60[4] = {
#include "gm_sub/0090EB60.inc"
};
u32 D_0090EB70[4] = {
#include "gm_sub/0090EB70.inc"
};
u32 D_0090EB80[4] = {
#include "gm_sub/0090EB80.inc"
};
u32 D_0090EB90[4] = {
#include "gm_sub/0090EB90.inc"
};
u32 D_0090EBA0[4] = {
#include "gm_sub/0090EBA0.inc"
};
u32 D_0090EBB0[4] = {
#include "gm_sub/0090EBB0.inc"
};
u32 D_0090EBC0[4] = {
#include "gm_sub/0090EBC0.inc"
};
u32 D_0090EBD0[4] = {
#include "gm_sub/0090EBD0.inc"
};
u32 D_0090EBE0[4] = {
#include "gm_sub/0090EBE0.inc"
};
u32 D_0090EBF0[4] = {
#include "gm_sub/0090EBF0.inc"
};
u32 D_0090EC00[4] = {
#include "gm_sub/0090EC00.inc"
};
u32 D_0090EC10[4] = {
#include "gm_sub/0090EC10.inc"
};
u32 D_0090EC20[4] = {
#include "gm_sub/0090EC20.inc"
};
u32 D_0090EC30[76] = {
#include "gm_sub/0090EC30.inc"
};
u32 D_0090ED60[4] = {
#include "gm_sub/0090ED60.inc"
};
u32 D_0090ED70[4] = {
#include "gm_sub/0090ED70.inc"
};
u32 D_0090ED80[4] = {
#include "gm_sub/0090ED80.inc"
};
u32 D_0090ED90[4] = {
#include "gm_sub/0090ED90.inc"
};
u32 D_0090EDA0[4] = {
#include "gm_sub/0090EDA0.inc"
};
u32 D_0090EDB0[4] = {
#include "gm_sub/0090EDB0.inc"
};
u32 D_0090EDC0[4] = {
#include "gm_sub/0090EDC0.inc"
};
u32 D_0090EDD0[4] = {
#include "gm_sub/0090EDD0.inc"
};
u32 D_0090EDE0[4] = {
#include "gm_sub/0090EDE0.inc"
};
u32 D_0090EDF0[4] = {
#include "gm_sub/0090EDF0.inc"
};
u32 D_0090EE00[4] = {
#include "gm_sub/0090EE00.inc"
};
u32 D_0090EE10[4] = {
#include "gm_sub/0090EE10.inc"
};
u32 D_0090EE20[4] = {
#include "gm_sub/0090EE20.inc"
};
u32 D_0090EE30[4] = {
#include "gm_sub/0090EE30.inc"
};
u32 D_0090EE40[4] = {
#include "gm_sub/0090EE40.inc"
};
u32 D_0090EE50[4] = {
#include "gm_sub/0090EE50.inc"
};
u32 D_0090EE60[4] = {
#include "gm_sub/0090EE60.inc"
};
u32 D_0090EE70[4] = {
#include "gm_sub/0090EE70.inc"
};
u32 D_0090EE80[4] = {
#include "gm_sub/0090EE80.inc"
};
u32 D_0090EE90[4] = {
#include "gm_sub/0090EE90.inc"
};
u32 D_0090EEA0[4] = {
#include "gm_sub/0090EEA0.inc"
};
u32 D_0090EEB0[4] = {
#include "gm_sub/0090EEB0.inc"
};
u32 D_0090EEC0[4] = {
#include "gm_sub/0090EEC0.inc"
};
u32 D_0090EED0[4] = {
#include "gm_sub/0090EED0.inc"
};
u32 D_0090EEE0[4] = {
#include "gm_sub/0090EEE0.inc"
};
u32 D_0090EEF0[4] = {
#include "gm_sub/0090EEF0.inc"
};
u32 D_0090EF00[4] = {
#include "gm_sub/0090EF00.inc"
};
u32 D_0090EF10[4] = {
#include "gm_sub/0090EF10.inc"
};
u32 D_0090EF20[58] = {
#include "gm_sub/0090EF20.inc"
};
u32 D_0090F008[3] = {
#include "gm_sub/0090F008.inc"
};
u32 D_0090F018[3] = {
#include "gm_sub/0090F018.inc"
};
u32 D_0090F028[3] = {
#include "gm_sub/0090F028.inc"
};
u32 D_0090F038[3] = {
#include "gm_sub/0090F038.inc"
};
u32 D_0090F048[3] = {
#include "gm_sub/0090F048.inc"
};
u32 D_0090F058[3] = {
#include "gm_sub/0090F058.inc"
};
u32 D_0090F068[3] = {
#include "gm_sub/0090F068.inc"
};
u32 D_0090F078[3] = {
#include "gm_sub/0090F078.inc"
};
u32 D_0090F088[3] = {
#include "gm_sub/0090F088.inc"
};
u32 D_0090F098[3] = {
#include "gm_sub/0090F098.inc"
};
u32 D_0090F0A8[3] = {
#include "gm_sub/0090F0A8.inc"
};
u32 D_0090F0B8[3] = {
#include "gm_sub/0090F0B8.inc"
};
extern u32 gEmKindTables_68_0[];
extern u32 gEmKindTables_68_2[];
extern u32 gEmRouteSets[];
u32 D_0090F0D0[60] = {
#include "gm_sub/0090F0D0.inc"
};
u32 D_0090F1C0[4] = {
#include "gm_sub/0090F1C0.inc"
};
u32 D_0090F1D0[4] = {
#include "gm_sub/0090F1D0.inc"
};
u32 D_0090F1E0[4] = {
#include "gm_sub/0090F1E0.inc"
};
u32 D_0090F1F0[4] = {
#include "gm_sub/0090F1F0.inc"
};
u32 D_0090F200[4] = {
#include "gm_sub/0090F200.inc"
};
u32 D_0090F210[4] = {
#include "gm_sub/0090F210.inc"
};
u32 D_0090F220[4] = {
#include "gm_sub/0090F220.inc"
};
u32 D_0090F230[16] = {
#include "gm_sub/0090F230.inc"
};
u32 D_0090F270[4] = {
#include "gm_sub/0090F270.inc"
};
u32 D_0090F280[4] = {
#include "gm_sub/0090F280.inc"
};
u32 D_0090F290[4] = {
#include "gm_sub/0090F290.inc"
};
u32 D_0090F2A0[8] = {
#include "gm_sub/0090F2A0.inc"
};
u32 D_0090F2C0[4] = {
#include "gm_sub/0090F2C0.inc"
};
u32 D_0090F2D0[4] = {
#include "gm_sub/0090F2D0.inc"
};
u32 D_0090F2E0[4] = {
#include "gm_sub/0090F2E0.inc"
};
u32 D_0090F2F0[4] = {
#include "gm_sub/0090F2F0.inc"
};
u32 D_0090F300[4] = {
#include "gm_sub/0090F300.inc"
};
u32 D_0090F310[4] = {
#include "gm_sub/0090F310.inc"
};
u32 D_0090F320[4] = {
#include "gm_sub/0090F320.inc"
};
u32 D_0090F330[4] = {
#include "gm_sub/0090F330.inc"
};
u32 D_0090F340[16] = {
#include "gm_sub/0090F340.inc"
};
u32 D_0090F380[4] = {
#include "gm_sub/0090F380.inc"
};
u32 D_0090F390[4] = {
#include "gm_sub/0090F390.inc"
};
u32 D_0090F3A0[4] = {
#include "gm_sub/0090F3A0.inc"
};
u32 D_0090F3B0[4] = {
#include "gm_sub/0090F3B0.inc"
};
u32 D_0090F3C0[4] = {
#include "gm_sub/0090F3C0.inc"
};
u32 D_0090F3D0[4] = {
#include "gm_sub/0090F3D0.inc"
};
u32 D_0090F3E0[4] = {
#include "gm_sub/0090F3E0.inc"
};
u32 D_0090F3F0[4] = {
#include "gm_sub/0090F3F0.inc"
};
u32 D_0090F400[16] = {
#include "gm_sub/0090F400.inc"
};
u32 D_0090F440[4] = {
#include "gm_sub/0090F440.inc"
};
u32 D_0090F450[4] = {
#include "gm_sub/0090F450.inc"
};
u32 D_0090F460[4] = {
#include "gm_sub/0090F460.inc"
};
u32 D_0090F470[4] = {
#include "gm_sub/0090F470.inc"
};
u32 D_0090F480[4] = {
#include "gm_sub/0090F480.inc"
};
u32 D_0090F490[4] = {
#include "gm_sub/0090F490.inc"
};
u32 D_0090F4A0[4] = {
#include "gm_sub/0090F4A0.inc"
};
u32 D_0090F4B0[4] = {
#include "gm_sub/0090F4B0.inc"
};
u32 D_0090F4C0[4] = {
#include "gm_sub/0090F4C0.inc"
};
u32 D_0090F4D0[4] = {
#include "gm_sub/0090F4D0.inc"
};
u32 D_0090F4E0[4] = {
#include "gm_sub/0090F4E0.inc"
};
u32 D_0090F4F0[22] = {
#include "gm_sub/0090F4F0.inc"
};
u32 D_0090F548[2] = {
#include "gm_sub/0090F548.inc"
};
u32 D_0090F550[4] = {
#include "gm_sub/0090F550.inc"
};
u32 D_0090F560[4] = {
#include "gm_sub/0090F560.inc"
};
u32 D_0090F570[4] = {
#include "gm_sub/0090F570.inc"
};
u32 D_0090F580[4] = {
#include "gm_sub/0090F580.inc"
};
u32 D_0090F590[4] = {
#include "gm_sub/0090F590.inc"
};
u32 D_0090F5A0[4] = {
#include "gm_sub/0090F5A0.inc"
};
u32 D_0090F5B0[14] = {
#include "gm_sub/0090F5B0.inc"
};
u32 D_0090F5E8[2] = {
#include "gm_sub/0090F5E8.inc"
};
u32 D_0090F5F0[4] = {
#include "gm_sub/0090F5F0.inc"
};
u32 D_0090F600[4] = {
#include "gm_sub/0090F600.inc"
};
u32 D_0090F610[4] = {
#include "gm_sub/0090F610.inc"
};
u32 D_0090F620[4] = {
#include "gm_sub/0090F620.inc"
};
u32 D_0090F630[4] = {
#include "gm_sub/0090F630.inc"
};
u32 D_0090F640[4] = {
#include "gm_sub/0090F640.inc"
};
u32 D_0090F650[44] = {
#include "gm_sub/0090F650.inc"
};
u32 D_0090F700[4] = {
#include "gm_sub/0090F700.inc"
};
u32 D_0090F710[4] = {
#include "gm_sub/0090F710.inc"
};
u32 D_0090F720[4] = {
#include "gm_sub/0090F720.inc"
};
u32 D_0090F730[4] = {
#include "gm_sub/0090F730.inc"
};
u32 D_0090F740[4] = {
#include "gm_sub/0090F740.inc"
};
u32 D_0090F750[4] = {
#include "gm_sub/0090F750.inc"
};
u32 D_0090F760[4] = {
#include "gm_sub/0090F760.inc"
};
u32 D_0090F770[16] = {
#include "gm_sub/0090F770.inc"
};
u32 D_0090F7B0[4] = {
#include "gm_sub/0090F7B0.inc"
};
u32 D_0090F7C0[4] = {
#include "gm_sub/0090F7C0.inc"
};
u32 D_0090F7D0[4] = {
#include "gm_sub/0090F7D0.inc"
};
u32 D_0090F7E0[4] = {
#include "gm_sub/0090F7E0.inc"
};
u32 D_0090F7F0[4] = {
#include "gm_sub/0090F7F0.inc"
};
u32 D_0090F800[12] = {
#include "gm_sub/0090F800.inc"
};
u32 D_0090F830[4] = {
#include "gm_sub/0090F830.inc"
};
u32 D_0090F840[4] = {
#include "gm_sub/0090F840.inc"
};
u32 D_0090F850[4] = {
#include "gm_sub/0090F850.inc"
};
u32 D_0090F860[4] = {
#include "gm_sub/0090F860.inc"
};
u32 D_0090F870[4] = {
#include "gm_sub/0090F870.inc"
};
u32 D_0090F880[10] = {
#include "gm_sub/0090F880.inc"
};
u32 D_0090F8A8[2] = {
#include "gm_sub/0090F8A8.inc"
};
u32 D_0090F8B0[4] = {
#include "gm_sub/0090F8B0.inc"
};
u32 D_0090F8C0[4] = {
#include "gm_sub/0090F8C0.inc"
};
u32 D_0090F8D0[4] = {
#include "gm_sub/0090F8D0.inc"
};
u32 D_0090F8E0[4] = {
#include "gm_sub/0090F8E0.inc"
};
u32 D_0090F8F0[344] = {
#include "gm_sub/0090F8F0.inc"
};
u32 D_0090FE50[18] = {
#include "gm_sub/0090FE50.inc"
};
u32 D_0090FE98[3] = {
#include "gm_sub/0090FE98.inc"
};
u32 D_0090FEB0[16] = {
#include "gm_sub/0090FEB0.inc"
};
u32 D_0090FEF0[10] = {
#include "gm_sub/0090FEF0.inc"
};
u32 D_0090FF18[3] = {
#include "gm_sub/0090FF18.inc"
};
u32 D_0090FF28[3] = {
#include "gm_sub/0090FF28.inc"
};
u32 D_0090FF40[6] = {
#include "gm_sub/0090FF40.inc"
};
u32 D_0090FF58[3] = {
#include "gm_sub/0090FF58.inc"
};
u32 D_0090FF68[3] = {
#include "gm_sub/0090FF68.inc"
};
u32 D_0090FF80[6] = {
#include "gm_sub/0090FF80.inc"
};
u32 D_0090FF98[3] = {
#include "gm_sub/0090FF98.inc"
};
u32 D_0090FFA8[3] = {
#include "gm_sub/0090FFA8.inc"
};
u32 D_0090FFC0[324] = {
#include "gm_sub/0090FFC0.inc"
};
u32 D_009104D0[20] = {
#include "gm_sub/009104D0.inc"
};
u32 D_00910520[24] = {
#include "gm_sub/00910520.inc"
};
u32 D_00910580[16] = {
#include "gm_sub/00910580.inc"
};
u32 D_009105C0[16] = {
#include "gm_sub/009105C0.inc"
};
u32 D_00910600[24] = {
#include "gm_sub/00910600.inc"
};
u32 D_00910660[20] = {
#include "gm_sub/00910660.inc"
};
u32 D_009106B0[24] = {
#include "gm_sub/009106B0.inc"
};
u32 D_00910710[16] = {
#include "gm_sub/00910710.inc"
};
u32 D_00910750[24] = {
#include "gm_sub/00910750.inc"
};
u32 D_009107B0[20] = {
#include "gm_sub/009107B0.inc"
};
u32 D_00910800[24] = {
#include "gm_sub/00910800.inc"
};
u32 D_00910860[24] = {
#include "gm_sub/00910860.inc"
};
u32 D_009108C0[20] = {
#include "gm_sub/009108C0.inc"
};
u32 D_00910910[28] = {
#include "gm_sub/00910910.inc"
};
u32 D_00910980[4] = {
#include "gm_sub/00910980.inc"
};
u32 D_00910990[4] = {
#include "gm_sub/00910990.inc"
};
u32 D_009109A0[4] = {
#include "gm_sub/009109A0.inc"
};
u32 D_009109B0[4] = {
#include "gm_sub/009109B0.inc"
};
u32 D_009109C0[4] = {
#include "gm_sub/009109C0.inc"
};
u32 D_009109D0[316] = {
#include "gm_sub/009109D0.inc"
};
u32 D_00910EC0[12] = {
#include "gm_sub/00910EC0.inc"
};
u32 D_00910EF0[12] = {
#include "gm_sub/00910EF0.inc"
};
u32 D_00910F20[24] = {
#include "gm_sub/00910F20.inc"
};
u32 D_00910F80[56] = {
#include "gm_sub/00910F80.inc"
};
u32 D_00911060[20] = {
#include "gm_sub/00911060.inc"
};
u32 D_009110B0[24] = {
#include "gm_sub/009110B0.inc"
};
u32 D_00911110[40] = {
#include "gm_sub/00911110.inc"
};
u32 D_009111B0[24] = {
#include "gm_sub/009111B0.inc"
};
u32 D_00911210[24] = {
#include "gm_sub/00911210.inc"
};
u32 D_00911270[24] = {
#include "gm_sub/00911270.inc"
};
u32 D_009112D0[22] = {
#include "gm_sub/009112D0.inc"
};
u32 D_00911328[2] = {
#include "gm_sub/00911328.inc"
};
u32 D_00911330[2] = {
#include "gm_sub/00911330.inc"
};
u32 D_00911338[2] = {
#include "gm_sub/00911338.inc"
};
u32 D_00911340[32] = {
#include "gm_sub/00911340.inc"
};
u32 D_009113C0[8] = {
#include "gm_sub/009113C0.inc"
};
u32 D_009113E0[664] = {
#include "gm_sub/009113E0.inc"
};
u32 D_00911E40[16] = {
#include "gm_sub/00911E40.inc"
};
u32 D_00911E80[24] = {
#include "gm_sub/00911E80.inc"
};
u32 D_00911EE0[12] = {
#include "gm_sub/00911EE0.inc"
};
u32 D_00911F10[8] = {
#include "gm_sub/00911F10.inc"
};
u32 D_00911F30[16] = {
#include "gm_sub/00911F30.inc"
};
u32 D_00911F70[12] = {
#include "gm_sub/00911F70.inc"
};
u32 D_00911FA0[4] = {
#include "gm_sub/00911FA0.inc"
};
u32 D_00911FB0[4] = {
#include "gm_sub/00911FB0.inc"
};
u32 D_00911FC0[4] = {
#include "gm_sub/00911FC0.inc"
};
u32 D_00911FD0[4] = {
#include "gm_sub/00911FD0.inc"
};
u32 D_00911FE0[4] = {
#include "gm_sub/00911FE0.inc"
};
u32 D_00911FF0[4] = {
#include "gm_sub/00911FF0.inc"
};
u32 D_00912000[8] = {
#include "gm_sub/00912000.inc"
};
u32 D_00912020[8] = {
#include "gm_sub/00912020.inc"
};
u32 D_00912040[8] = {
#include "gm_sub/00912040.inc"
};
u32 D_00912060[8] = {
#include "gm_sub/00912060.inc"
};
u32 D_00912080[8] = {
#include "gm_sub/00912080.inc"
};
u32 D_009120A0[8] = {
#include "gm_sub/009120A0.inc"
};
u32 D_009120C0[8] = {
#include "gm_sub/009120C0.inc"
};
u32 D_009120E0[8] = {
#include "gm_sub/009120E0.inc"
};
u32 D_00912100[8] = {
#include "gm_sub/00912100.inc"
};
u32 D_00912120[8] = {
#include "gm_sub/00912120.inc"
};
u32 D_00912140[368] = {
#include "gm_sub/00912140.inc"
};
u32 D_00912700[12] = {
#include "gm_sub/00912700.inc"
};
u32 D_00912730[12] = {
#include "gm_sub/00912730.inc"
};
u32 D_00912760[12] = {
#include "gm_sub/00912760.inc"
};
u32 D_00912790[8] = {
#include "gm_sub/00912790.inc"
};
u32 D_009127B0[20] = {
#include "gm_sub/009127B0.inc"
};
u32 D_00912800[12] = {
#include "gm_sub/00912800.inc"
};
u32 D_00912830[12] = {
#include "gm_sub/00912830.inc"
};
u32 D_00912860[20] = {
#include "gm_sub/00912860.inc"
};
u32 D_009128B0[12] = {
#include "gm_sub/009128B0.inc"
};
u32 D_009128E0[12] = {
#include "gm_sub/009128E0.inc"
};
u32 D_00912910[12] = {
#include "gm_sub/00912910.inc"
};
u32 D_00912940[10] = {
#include "gm_sub/00912940.inc"
};
u32 D_00912968[2] = {
#include "gm_sub/00912968.inc"
};
u32 D_00912970[2] = {
#include "gm_sub/00912970.inc"
};
u32 D_00912978[2] = {
#include "gm_sub/00912978.inc"
};
u32 D_00912980[12] = {
#include "gm_sub/00912980.inc"
};
u32 D_009129B0[8] = {
#include "gm_sub/009129B0.inc"
};
u32 D_009129D0[16] = {
#include "gm_sub/009129D0.inc"
};
u32 D_00912A10[8] = {
#include "gm_sub/00912A10.inc"
};
u32 D_00912A30[12] = {
#include "gm_sub/00912A30.inc"
};
u32 D_00912A60[12] = {
#include "gm_sub/00912A60.inc"
};
u32 D_00912A90[12] = {
#include "gm_sub/00912A90.inc"
};
u32 D_00912AC0[8] = {
#include "gm_sub/00912AC0.inc"
};
u32 D_00912AE0[8] = {
#include "gm_sub/00912AE0.inc"
};
u32 D_00912B00[12] = {
#include "gm_sub/00912B00.inc"
};
u32 D_00912B30[8] = {
#include "gm_sub/00912B30.inc"
};
u32 D_00912B50[8] = {
#include "gm_sub/00912B50.inc"
};
u32 D_00912B70[12] = {
#include "gm_sub/00912B70.inc"
};
u32 D_00912BA0[460] = {
#include "gm_sub/00912BA0.inc"
};
u32 D_009132D0[22] = {
#include "gm_sub/009132D0.inc"
};
u32 D_00913328[3] = {
#include "gm_sub/00913328.inc"
};
u32 D_00913338[3] = {
#include "gm_sub/00913338.inc"
};
u32 D_00913350[8] = {
#include "gm_sub/00913350.inc"
};
u32 D_00913370[8] = {
#include "gm_sub/00913370.inc"
};
u32 D_00913390[8] = {
#include "gm_sub/00913390.inc"
};
u32 D_009133B0[16] = {
#include "gm_sub/009133B0.inc"
};
u32 D_009133F0[12] = {
#include "gm_sub/009133F0.inc"
};
u32 D_00913420[4] = {
#include "gm_sub/00913420.inc"
};
u32 D_00913430[8] = {
#include "gm_sub/00913430.inc"
};
u32 D_00913450[20] = {
#include "gm_sub/00913450.inc"
};
u32 D_009134A0[24] = {
#include "gm_sub/009134A0.inc"
};
u32 D_00913500[4] = {
#include "gm_sub/00913500.inc"
};
u32 D_00913510[4] = {
#include "gm_sub/00913510.inc"
};
u32 D_00913520[4] = {
#include "gm_sub/00913520.inc"
};
u32 D_00913530[8] = {
#include "gm_sub/00913530.inc"
};
u32 D_00913550[4] = {
#include "gm_sub/00913550.inc"
};
u32 D_00913560[4] = {
#include "gm_sub/00913560.inc"
};
u32 D_00913570[4] = {
#include "gm_sub/00913570.inc"
};
u32 D_00913580[4] = {
#include "gm_sub/00913580.inc"
};
u32 D_00913590[4] = {
#include "gm_sub/00913590.inc"
};
u32 D_009135A0[4] = {
#include "gm_sub/009135A0.inc"
};
u32 D_009135B0[4] = {
#include "gm_sub/009135B0.inc"
};
u32 D_009135C0[16] = {
#include "gm_sub/009135C0.inc"
};
u32 D_00913600[4] = {
#include "gm_sub/00913600.inc"
};
u32 D_00913610[4] = {
#include "gm_sub/00913610.inc"
};
u32 D_00913620[4] = {
#include "gm_sub/00913620.inc"
};
u32 D_00913630[4] = {
#include "gm_sub/00913630.inc"
};
u32 D_00913640[4] = {
#include "gm_sub/00913640.inc"
};
u32 D_00913650[4] = {
#include "gm_sub/00913650.inc"
};
u32 D_00913660[4] = {
#include "gm_sub/00913660.inc"
};
u32 D_00913670[640] = {
#include "gm_sub/00913670.inc"
};
u32 D_00914070[16] = {
#include "gm_sub/00914070.inc"
};
u32 D_009140B0[16] = {
#include "gm_sub/009140B0.inc"
};
u32 D_009140F0[6] = {
#include "gm_sub/009140F0.inc"
};
u32 D_00914108[2] = {
#include "gm_sub/00914108.inc"
};
u32 D_00914110[408] = {
#include "gm_sub/00914110.inc"
};
u32 D_00914770[12] = {
#include "gm_sub/00914770.inc"
};
u32 D_009147A0[12] = {
#include "gm_sub/009147A0.inc"
};
u32 D_009147D0[8] = {
#include "gm_sub/009147D0.inc"
};
u32 D_009147F0[12] = {
#include "gm_sub/009147F0.inc"
};
u32 D_00914820[12] = {
#include "gm_sub/00914820.inc"
};
u32 D_00914850[12] = {
#include "gm_sub/00914850.inc"
};
u32 D_00914880[12] = {
#include "gm_sub/00914880.inc"
};
u32 D_009148B0[16] = {
#include "gm_sub/009148B0.inc"
};
u32 D_009148F0[2] = {
#include "gm_sub/009148F0.inc"
};
u32 D_009148F8[2] = {
#include "gm_sub/009148F8.inc"
};
u32 D_00914900[4] = {
#include "gm_sub/00914900.inc"
};
u32 D_00914910[12] = {
#include "gm_sub/00914910.inc"
};
u32 D_00914940[12] = {
#include "gm_sub/00914940.inc"
};
u32 D_00914970[12] = {
#include "gm_sub/00914970.inc"
};
u32 D_009149A0[16] = {
#include "gm_sub/009149A0.inc"
};
u32 D_009149E0[16] = {
#include "gm_sub/009149E0.inc"
};
u32 D_00914A20[16] = {
#include "gm_sub/00914A20.inc"
};
u32 D_00914A60[16] = {
#include "gm_sub/00914A60.inc"
};
u32 D_00914AA0[24] = {
#include "gm_sub/00914AA0.inc"
};
u32 D_00914B00[12] = {
#include "gm_sub/00914B00.inc"
};
u32 D_00914B30[12] = {
#include "gm_sub/00914B30.inc"
};
u32 D_00914B60[12] = {
#include "gm_sub/00914B60.inc"
};
u32 D_00914B90[12] = {
#include "gm_sub/00914B90.inc"
};
u32 D_00914BC0[12] = {
#include "gm_sub/00914BC0.inc"
};
u32 D_00914BF0[12] = {
#include "gm_sub/00914BF0.inc"
};
u32 D_00914C20[12] = {
#include "gm_sub/00914C20.inc"
};
u32 D_00914C50[24] = {
#include "gm_sub/00914C50.inc"
};
u32 D_00914CB0[12] = {
#include "gm_sub/00914CB0.inc"
};
u32 D_00914CE0[12] = {
#include "gm_sub/00914CE0.inc"
};
u32 D_00914D10[12] = {
#include "gm_sub/00914D10.inc"
};
u32 D_00914D40[12] = {
#include "gm_sub/00914D40.inc"
};
u32 D_00914D70[12] = {
#include "gm_sub/00914D70.inc"
};
u32 D_00914DA0[12] = {
#include "gm_sub/00914DA0.inc"
};
u32 D_00914DD0[472] = {
#include "gm_sub/00914DD0.inc"
};
u32 D_00915530[16] = {
#include "gm_sub/00915530.inc"
};
u32 D_00915570[12] = {
#include "gm_sub/00915570.inc"
};
u32 D_009155A0[12] = {
#include "gm_sub/009155A0.inc"
};
u32 D_009155D0[12] = {
#include "gm_sub/009155D0.inc"
};
u32 D_00915600[12] = {
#include "gm_sub/00915600.inc"
};
u32 D_00915630[16] = {
#include "gm_sub/00915630.inc"
};
u32 D_00915670[20] = {
#include "gm_sub/00915670.inc"
};
u32 D_009156C0[20] = {
#include "gm_sub/009156C0.inc"
};
u32 D_00915710[16] = {
#include "gm_sub/00915710.inc"
};
u32 D_00915750[12] = {
#include "gm_sub/00915750.inc"
};
u32 D_00915780[24] = {
#include "gm_sub/00915780.inc"
};
u32 D_009157E0[8] = {
#include "gm_sub/009157E0.inc"
};
u32 D_00915800[26] = {
#include "gm_sub/00915800.inc"
};
u32 D_00915868[3] = {
#include "gm_sub/00915868.inc"
};
u32 D_00915878[3] = {
#include "gm_sub/00915878.inc"
};
u32 D_00915888[3] = {
#include "gm_sub/00915888.inc"
};
u32 D_00915898[3] = {
#include "gm_sub/00915898.inc"
};
u32 D_009158A8[3] = {
#include "gm_sub/009158A8.inc"
};
u32 D_009158B8[3] = {
#include "gm_sub/009158B8.inc"
};
u32 D_009158D0[14] = {
#include "gm_sub/009158D0.inc"
};
u32 D_00915908[3] = {
#include "gm_sub/00915908.inc"
};
u32 D_00915920[4] = {
#include "gm_sub/00915920.inc"
};
u32 D_00915930[4] = {
#include "gm_sub/00915930.inc"
};
u32 D_00915940[16] = {
#include "gm_sub/00915940.inc"
};
u32 D_00915980[8] = {
#include "gm_sub/00915980.inc"
};
u32 D_009159A0[12] = {
#include "gm_sub/009159A0.inc"
};
u32 D_009159D0[8] = {
#include "gm_sub/009159D0.inc"
};
u32 D_009159F0[16] = {
#include "gm_sub/009159F0.inc"
};
u32 D_00915A30[8] = {
#include "gm_sub/00915A30.inc"
};
u32 D_00915A50[32] = {
#include "gm_sub/00915A50.inc"
};
u32 D_00915AD0[4] = {
#include "gm_sub/00915AD0.inc"
};
u32 D_00915AE0[4] = {
#include "gm_sub/00915AE0.inc"
};
u32 D_00915AF0[4] = {
#include "gm_sub/00915AF0.inc"
};
u32 D_00915B00[4] = {
#include "gm_sub/00915B00.inc"
};
u32 D_00915B10[4] = {
#include "gm_sub/00915B10.inc"
};
u32 D_00915B20[4] = {
#include "gm_sub/00915B20.inc"
};
u32 D_00915B30[14] = {
#include "gm_sub/00915B30.inc"
};
u32 D_00915B68[2] = {
#include "gm_sub/00915B68.inc"
};
u32 D_00915B70[4] = {
#include "gm_sub/00915B70.inc"
};
u32 D_00915B80[4] = {
#include "gm_sub/00915B80.inc"
};
u32 D_00915B90[4] = {
#include "gm_sub/00915B90.inc"
};
u32 D_00915BA0[4] = {
#include "gm_sub/00915BA0.inc"
};
u32 D_00915BB0[4] = {
#include "gm_sub/00915BB0.inc"
};
u32 D_00915BC0[4] = {
#include "gm_sub/00915BC0.inc"
};
u32 D_00915BD0[4] = {
#include "gm_sub/00915BD0.inc"
};
u32 D_00915BE0[16] = {
#include "gm_sub/00915BE0.inc"
};
u32 D_00915C20[760] = {
#include "gm_sub/00915C20.inc"
};
u32 D_00916800[24] = {
#include "gm_sub/00916800.inc"
};
u32 D_00916860[16] = {
#include "gm_sub/00916860.inc"
};
u32 D_009168A0[12] = {
#include "gm_sub/009168A0.inc"
};
u32 D_009168D0[16] = {
#include "gm_sub/009168D0.inc"
};
u32 D_00916910[16] = {
#include "gm_sub/00916910.inc"
};
u32 D_00916950[12] = {
#include "gm_sub/00916950.inc"
};
u32 D_00916980[8] = {
#include "gm_sub/00916980.inc"
};
u32 D_009169A0[12] = {
#include "gm_sub/009169A0.inc"
};
u32 D_009169D0[24] = {
#include "gm_sub/009169D0.inc"
};
u32 D_00916A30[12] = {
#include "gm_sub/00916A30.inc"
};
u32 D_00916A60[22] = {
#include "gm_sub/00916A60.inc"
};
u32 D_00916AB8[2] = {
#include "gm_sub/00916AB8.inc"
};
u32 D_00916AC0[2] = {
#include "gm_sub/00916AC0.inc"
};
u32 D_00916AC8[2] = {
#include "gm_sub/00916AC8.inc"
};
u32 D_00916AD0[16] = {
#include "gm_sub/00916AD0.inc"
};
u32 D_00916B10[16] = {
#include "gm_sub/00916B10.inc"
};
u32 D_00916B50[12] = {
#include "gm_sub/00916B50.inc"
};
u32 D_00916B80[16] = {
#include "gm_sub/00916B80.inc"
};
u32 D_00916BC0[16] = {
#include "gm_sub/00916BC0.inc"
};
u32 D_00916C00[12] = {
#include "gm_sub/00916C00.inc"
};
u32 D_00916C30[12] = {
#include "gm_sub/00916C30.inc"
};
u32 D_00916C60[12] = {
#include "gm_sub/00916C60.inc"
};
u32 D_00916C90[12] = {
#include "gm_sub/00916C90.inc"
};
u32 D_00916CC0[12] = {
#include "gm_sub/00916CC0.inc"
};
u32 D_00916CF0[12] = {
#include "gm_sub/00916CF0.inc"
};
u32 D_00916D20[12] = {
#include "gm_sub/00916D20.inc"
};
u32 D_00916D50[16] = {
#include "gm_sub/00916D50.inc"
};
u32 D_00916D90[16] = {
#include "gm_sub/00916D90.inc"
};
u32 D_00916DD0[12] = {
#include "gm_sub/00916DD0.inc"
};
u32 D_00916E00[16] = {
#include "gm_sub/00916E00.inc"
};
u32 D_00916E40[16] = {
#include "gm_sub/00916E40.inc"
};
u32 D_00916E80[12] = {
#include "gm_sub/00916E80.inc"
};
u32 D_00916EB0[12] = {
#include "gm_sub/00916EB0.inc"
};
u32 D_00916EE0[12] = {
#include "gm_sub/00916EE0.inc"
};
u32 D_00916F10[12] = {
#include "gm_sub/00916F10.inc"
};
u32 D_00916F40[20] = {
#include "gm_sub/00916F40.inc"
};
u32 D_00916F90[20] = {
#include "gm_sub/00916F90.inc"
};
u32 D_00916FE0[12] = {
#include "gm_sub/00916FE0.inc"
};
u32 D_00917010[12] = {
#include "gm_sub/00917010.inc"
};
u32 D_00917040[12] = {
#include "gm_sub/00917040.inc"
};
u32 D_00917070[12] = {
#include "gm_sub/00917070.inc"
};
u32 D_009170A0[1072] = {
#include "gm_sub/009170A0.inc"
};
u32 D_00918160[12] = {
#include "gm_sub/00918160.inc"
};
u32 D_00918190[12] = {
#include "gm_sub/00918190.inc"
};
u32 D_009181C0[20] = {
#include "gm_sub/009181C0.inc"
};
u32 D_00918210[8] = {
#include "gm_sub/00918210.inc"
};
u32 D_00918230[24] = {
#include "gm_sub/00918230.inc"
};
u32 D_00918290[8] = {
#include "gm_sub/00918290.inc"
};
u32 D_009182B0[20] = {
#include "gm_sub/009182B0.inc"
};
u32 D_00918300[12] = {
#include "gm_sub/00918300.inc"
};
u32 D_00918330[12] = {
#include "gm_sub/00918330.inc"
};
u32 D_00918360[12] = {
#include "gm_sub/00918360.inc"
};
u32 D_00918390[8] = {
#include "gm_sub/00918390.inc"
};
u32 D_009183B0[8] = {
#include "gm_sub/009183B0.inc"
};
u32 D_009183D0[24] = {
#include "gm_sub/009183D0.inc"
};
u32 D_00918430[8] = {
#include "gm_sub/00918430.inc"
};
u32 D_00918450[32] = {
#include "gm_sub/00918450.inc"
};
u32 D_009184D0[8] = {
#include "gm_sub/009184D0.inc"
};
u32 D_009184F0[8] = {
#include "gm_sub/009184F0.inc"
};
u32 D_00918510[8] = {
#include "gm_sub/00918510.inc"
};
u32 D_00918530[8] = {
#include "gm_sub/00918530.inc"
};
u32 D_00918550[8] = {
#include "gm_sub/00918550.inc"
};
u32 D_00918570[8] = {
#include "gm_sub/00918570.inc"
};
u32 D_00918590[8] = {
#include "gm_sub/00918590.inc"
};
u32 D_009185B0[8] = {
#include "gm_sub/009185B0.inc"
};
u32 D_009185D0[8] = {
#include "gm_sub/009185D0.inc"
};
u32 D_009185F0[8] = {
#include "gm_sub/009185F0.inc"
};
u32 D_00918610[8] = {
#include "gm_sub/00918610.inc"
};
u32 D_00918630[8] = {
#include "gm_sub/00918630.inc"
};
u32 D_00918650[8] = {
#include "gm_sub/00918650.inc"
};
u32 D_00918670[8] = {
#include "gm_sub/00918670.inc"
};
u32 D_00918690[8] = {
#include "gm_sub/00918690.inc"
};
u32 D_009186B0[8] = {
#include "gm_sub/009186B0.inc"
};
u32 D_009186D0[8] = {
#include "gm_sub/009186D0.inc"
};
u32 D_009186F0[8] = {
#include "gm_sub/009186F0.inc"
};
u32 D_00918710[64] = {
#include "gm_sub/00918710.inc"
};
u32 D_00918810[4] = {
#include "gm_sub/00918810.inc"
};
u32 D_00918820[4] = {
#include "gm_sub/00918820.inc"
};
u32 D_00918830[4] = {
#include "gm_sub/00918830.inc"
};
u32 D_00918840[4] = {
#include "gm_sub/00918840.inc"
};
u32 D_00918850[4] = {
#include "gm_sub/00918850.inc"
};
u32 D_00918860[4] = {
#include "gm_sub/00918860.inc"
};
u32 D_00918870[4] = {
#include "gm_sub/00918870.inc"
};
u32 D_00918880[4] = {
#include "gm_sub/00918880.inc"
};
u32 D_00918890[4] = {
#include "gm_sub/00918890.inc"
};
u32 D_009188A0[4] = {
#include "gm_sub/009188A0.inc"
};
u32 D_009188B0[4] = {
#include "gm_sub/009188B0.inc"
};
u32 D_009188C0[4] = {
#include "gm_sub/009188C0.inc"
};
u32 D_009188D0[26] = {
#include "gm_sub/009188D0.inc"
};
u32 D_00918938[3] = {
#include "gm_sub/00918938.inc"
};
u32 D_00918948[3] = {
#include "gm_sub/00918948.inc"
};
u32 D_00918958[3] = {
#include "gm_sub/00918958.inc"
};
u32 D_00918968[3] = {
#include "gm_sub/00918968.inc"
};
u32 D_00918978[3] = {
#include "gm_sub/00918978.inc"
};
u32 D_00918990[20] = {
#include "gm_sub/00918990.inc"
};
u32 D_009189E0[4] = {
#include "gm_sub/009189E0.inc"
};
u32 D_009189F0[4] = {
#include "gm_sub/009189F0.inc"
};
u32 D_00918A00[4] = {
#include "gm_sub/00918A00.inc"
};
u32 D_00918A10[4] = {
#include "gm_sub/00918A10.inc"
};
u32 D_00918A20[4] = {
#include "gm_sub/00918A20.inc"
};
u32 D_00918A30[4] = {
#include "gm_sub/00918A30.inc"
};
u32 D_00918A40[14] = {
#include "gm_sub/00918A40.inc"
};
u32 D_00918A78[2] = {
#include "gm_sub/00918A78.inc"
};
u32 D_00918A80[4] = {
#include "gm_sub/00918A80.inc"
};
u32 D_00918A90[4] = {
#include "gm_sub/00918A90.inc"
};
u32 D_00918AA0[4] = {
#include "gm_sub/00918AA0.inc"
};
u32 D_00918AB0[4] = {
#include "gm_sub/00918AB0.inc"
};
u32 D_00918AC0[4] = {
#include "gm_sub/00918AC0.inc"
};
u32 D_00918AD0[4] = {
#include "gm_sub/00918AD0.inc"
};
u32 D_00918AE0[4] = {
#include "gm_sub/00918AE0.inc"
};
u32 D_00918AF0[16] = {
#include "gm_sub/00918AF0.inc"
};
u32 D_00918B30[4] = {
#include "gm_sub/00918B30.inc"
};
u32 D_00918B40[4] = {
#include "gm_sub/00918B40.inc"
};
u32 D_00918B50[4] = {
#include "gm_sub/00918B50.inc"
};
u32 D_00918B60[4] = {
#include "gm_sub/00918B60.inc"
};
u32 D_00918B70[4] = {
#include "gm_sub/00918B70.inc"
};
u32 D_00918B80[4] = {
#include "gm_sub/00918B80.inc"
};
u32 D_00918B90[4] = {
#include "gm_sub/00918B90.inc"
};
u32 D_00918BA0[4] = {
#include "gm_sub/00918BA0.inc"
};
u32 D_00918BB0[4] = {
#include "gm_sub/00918BB0.inc"
};
u32 D_00918BC0[4] = {
#include "gm_sub/00918BC0.inc"
};
u32 D_00918BD0[4] = {
#include "gm_sub/00918BD0.inc"
};
u32 D_00918BE0[428] = {
#include "gm_sub/00918BE0.inc"
};
u32 D_00919290[28] = {
#include "gm_sub/00919290.inc"
};
u32 D_00919300[28] = {
#include "gm_sub/00919300.inc"
};
u32 D_00919370[32] = {
#include "gm_sub/00919370.inc"
};
u32 D_009193F0[32] = {
#include "gm_sub/009193F0.inc"
};
u32 D_00919470[12] = {
#include "gm_sub/00919470.inc"
};
u32 D_009194A0[12] = {
#include "gm_sub/009194A0.inc"
};
u32 D_009194D0[12] = {
#include "gm_sub/009194D0.inc"
};
u32 D_00919500[12] = {
#include "gm_sub/00919500.inc"
};
u32 D_00919530[12] = {
#include "gm_sub/00919530.inc"
};
u32 D_00919560[10] = {
#include "gm_sub/00919560.inc"
};
u32 D_00919588[3] = {
#include "gm_sub/00919588.inc"
};
u32 D_009195A0[4] = {
#include "gm_sub/009195A0.inc"
};
u32 D_009195B0[4] = {
#include "gm_sub/009195B0.inc"
};
u32 D_009195C0[4] = {
#include "gm_sub/009195C0.inc"
};
u32 D_009195D0[4] = {
#include "gm_sub/009195D0.inc"
};
u32 D_009195E0[4] = {
#include "gm_sub/009195E0.inc"
};
u32 D_009195F0[8] = {
#include "gm_sub/009195F0.inc"
};
u32 D_00919610[8] = {
#include "gm_sub/00919610.inc"
};
u32 D_00919630[8] = {
#include "gm_sub/00919630.inc"
};
u32 D_00919650[8] = {
#include "gm_sub/00919650.inc"
};
u32 D_00919670[16] = {
#include "gm_sub/00919670.inc"
};
u32 D_009196B0[8] = {
#include "gm_sub/009196B0.inc"
};
u32 D_009196D0[16] = {
#include "gm_sub/009196D0.inc"
};
u32 D_00919710[8] = {
#include "gm_sub/00919710.inc"
};
u32 D_00919730[456] = {
#include "gm_sub/00919730.inc"
};
u32 D_00919E50[12] = {
#include "gm_sub/00919E50.inc"
};
u32 D_00919E80[16] = {
#include "gm_sub/00919E80.inc"
};
u32 D_00919EC0[12] = {
#include "gm_sub/00919EC0.inc"
};
u32 D_00919EF0[16] = {
#include "gm_sub/00919EF0.inc"
};
u32 D_00919F30[12] = {
#include "gm_sub/00919F30.inc"
};
u32 D_00919F60[20] = {
#include "gm_sub/00919F60.inc"
};
u32 D_00919FB0[16] = {
#include "gm_sub/00919FB0.inc"
};
u32 D_00919FF0[20] = {
#include "gm_sub/00919FF0.inc"
};
u32 D_0091A040[16] = {
#include "gm_sub/0091A040.inc"
};
u32 D_0091A080[12] = {
#include "gm_sub/0091A080.inc"
};
u32 D_0091A0B0[20] = {
#include "gm_sub/0091A0B0.inc"
};
u32 D_0091A100[12] = {
#include "gm_sub/0091A100.inc"
};
u32 D_0091A130[8] = {
#include "gm_sub/0091A130.inc"
};
u32 D_0091A150[24] = {
#include "gm_sub/0091A150.inc"
};
u32 D_0091A1B0[8] = {
#include "gm_sub/0091A1B0.inc"
};
u32 D_0091A1D0[8] = {
#include "gm_sub/0091A1D0.inc"
};
u32 D_0091A1F0[12] = {
#include "gm_sub/0091A1F0.inc"
};
u32 D_0091A220[16] = {
#include "gm_sub/0091A220.inc"
};
u32 D_0091A260[20] = {
#include "gm_sub/0091A260.inc"
};
u32 D_0091A2B0[16] = {
#include "gm_sub/0091A2B0.inc"
};
u32 D_0091A2F0[16] = {
#include "gm_sub/0091A2F0.inc"
};
u32 D_0091A330[20] = {
#include "gm_sub/0091A330.inc"
};
u32 D_0091A380[20] = {
#include "gm_sub/0091A380.inc"
};
u32 D_0091A3D0[20] = {
#include "gm_sub/0091A3D0.inc"
};
u32 D_0091A420[12] = {
#include "gm_sub/0091A420.inc"
};
u32 D_0091A450[16] = {
#include "gm_sub/0091A450.inc"
};
u32 D_0091A490[16] = {
#include "gm_sub/0091A490.inc"
};
u32 D_0091A4D0[24] = {
#include "gm_sub/0091A4D0.inc"
};
u32 D_0091A530[24] = {
#include "gm_sub/0091A530.inc"
};
u32 D_0091A590[20] = {
#include "gm_sub/0091A590.inc"
};
u32 D_0091A5E0[62] = {
#include "gm_sub/0091A5E0.inc"
};
u32 D_0091A6D8[2] = {
#include "gm_sub/0091A6D8.inc"
};
u32 D_0091A6E0[2] = {
#include "gm_sub/0091A6E0.inc"
};
u32 D_0091A6E8[2] = {
#include "gm_sub/0091A6E8.inc"
};
u32 D_0091A6F0[52] = {
#include "gm_sub/0091A6F0.inc"
};
u32 D_0091A7C0[32] = {
#include "gm_sub/0091A7C0.inc"
};
u32 D_0091A840[12] = {
#include "gm_sub/0091A840.inc"
};
u32 D_0091A870[28] = {
#include "gm_sub/0091A870.inc"
};
u32 D_0091A8E0[8] = {
#include "gm_sub/0091A8E0.inc"
};
u32 D_0091A900[36] = {
#include "gm_sub/0091A900.inc"
};
u32 D_0091A990[8] = {
#include "gm_sub/0091A990.inc"
};
u32 D_0091A9B0[28] = {
#include "gm_sub/0091A9B0.inc"
};
u32 D_0091AA20[20] = {
#include "gm_sub/0091AA20.inc"
};
u32 D_0091AA70[12] = {
#include "gm_sub/0091AA70.inc"
};
u32 D_0091AAA0[20] = {
#include "gm_sub/0091AAA0.inc"
};
u32 D_0091AAF0[16] = {
#include "gm_sub/0091AAF0.inc"
};
u32 D_0091AB30[12] = {
#include "gm_sub/0091AB30.inc"
};
u32 D_0091AB60[20] = {
#include "gm_sub/0091AB60.inc"
};
u32 D_0091ABB0[8] = {
#include "gm_sub/0091ABB0.inc"
};
u32 D_0091ABD0[28] = {
#include "gm_sub/0091ABD0.inc"
};
u32 D_0091AC40[28] = {
#include "gm_sub/0091AC40.inc"
};
u32 D_0091ACB0[12] = {
#include "gm_sub/0091ACB0.inc"
};
u32 D_0091ACE0[20] = {
#include "gm_sub/0091ACE0.inc"
};
u32 D_0091AD30[20] = {
#include "gm_sub/0091AD30.inc"
};
u32 D_0091AD80[12] = {
#include "gm_sub/0091AD80.inc"
};
u32 D_0091ADB0[20] = {
#include "gm_sub/0091ADB0.inc"
};
u32 D_0091AE00[20] = {
#include "gm_sub/0091AE00.inc"
};
u32 D_0091AE50[12] = {
#include "gm_sub/0091AE50.inc"
};
u32 D_0091AE80[20] = {
#include "gm_sub/0091AE80.inc"
};
u32 D_0091AED0[8] = {
#include "gm_sub/0091AED0.inc"
};
u32 D_0091AEF0[12] = {
#include "gm_sub/0091AEF0.inc"
};
u32 D_0091AF20[8] = {
#include "gm_sub/0091AF20.inc"
};
u32 D_0091AF40[48] = {
#include "gm_sub/0091AF40.inc"
};
u32 D_0091B000[8] = {
#include "gm_sub/0091B000.inc"
};
u32 D_0091B020[20] = {
#include "gm_sub/0091B020.inc"
};
u32 D_0091B070[8] = {
#include "gm_sub/0091B070.inc"
};
u32 D_0091B090[16] = {
#include "gm_sub/0091B090.inc"
};
u32 D_0091B0D0[8] = {
#include "gm_sub/0091B0D0.inc"
};
u32 D_0091B0F0[20] = {
#include "gm_sub/0091B0F0.inc"
};
u32 D_0091B140[8] = {
#include "gm_sub/0091B140.inc"
};
u32 D_0091B160[20] = {
#include "gm_sub/0091B160.inc"
};
u32 D_0091B1B0[20] = {
#include "gm_sub/0091B1B0.inc"
};
u32 D_0091B200[12] = {
#include "gm_sub/0091B200.inc"
};
u32 D_0091B230[20] = {
#include "gm_sub/0091B230.inc"
};
u32 D_0091B280[8] = {
#include "gm_sub/0091B280.inc"
};
u32 D_0091B2A0[16] = {
#include "gm_sub/0091B2A0.inc"
};
u32 D_0091B2E0[16] = {
#include "gm_sub/0091B2E0.inc"
};
u32 D_0091B320[12] = {
#include "gm_sub/0091B320.inc"
};
u32 D_0091B350[28] = {
#include "gm_sub/0091B350.inc"
};
u32 D_0091B3C0[8] = {
#include "gm_sub/0091B3C0.inc"
};
u32 D_0091B3E0[16] = {
#include "gm_sub/0091B3E0.inc"
};
u32 D_0091B420[16] = {
#include "gm_sub/0091B420.inc"
};
u32 D_0091B460[12] = {
#include "gm_sub/0091B460.inc"
};
u32 D_0091B490[20] = {
#include "gm_sub/0091B490.inc"
};
u32 D_0091B4E0[20] = {
#include "gm_sub/0091B4E0.inc"
};
u32 D_0091B530[12] = {
#include "gm_sub/0091B530.inc"
};
u32 D_0091B560[20] = {
#include "gm_sub/0091B560.inc"
};
u32 D_0091B5B0[20] = {
#include "gm_sub/0091B5B0.inc"
};
u32 D_0091B600[12] = {
#include "gm_sub/0091B600.inc"
};
u32 D_0091B630[12] = {
#include "gm_sub/0091B630.inc"
};
u32 D_0091B660[12] = {
#include "gm_sub/0091B660.inc"
};
u32 D_0091B690[12] = {
#include "gm_sub/0091B690.inc"
};
u32 D_0091B6C0[16] = {
#include "gm_sub/0091B6C0.inc"
};
u32 D_0091B700[20] = {
#include "gm_sub/0091B700.inc"
};
u32 D_0091B750[12] = {
#include "gm_sub/0091B750.inc"
};
u32 D_0091B780[16] = {
#include "gm_sub/0091B780.inc"
};
u32 D_0091B7C0[20] = {
#include "gm_sub/0091B7C0.inc"
};
u32 D_0091B810[12] = {
#include "gm_sub/0091B810.inc"
};
u32 D_0091B840[28] = {
#include "gm_sub/0091B840.inc"
};
u32 D_0091B8B0[20] = {
#include "gm_sub/0091B8B0.inc"
};
u32 D_0091B900[12] = {
#include "gm_sub/0091B900.inc"
};
u32 D_0091B930[20] = {
#include "gm_sub/0091B930.inc"
};
u32 D_0091B980[20] = {
#include "gm_sub/0091B980.inc"
};
u32 D_0091B9D0[12] = {
#include "gm_sub/0091B9D0.inc"
};
u32 D_0091BA00[20] = {
#include "gm_sub/0091BA00.inc"
};
u32 D_0091BA50[20] = {
#include "gm_sub/0091BA50.inc"
};
u32 D_0091BAA0[12] = {
#include "gm_sub/0091BAA0.inc"
};
u32 D_0091BAD0[90] = {
#include "gm_sub/0091BAD0.inc"
};
u32 D_0091BC38[3] = {
#include "gm_sub/0091BC38.inc"
};
u32 D_0091BC48[3] = {
#include "gm_sub/0091BC48.inc"
};
u32 D_0091BC58[3] = {
#include "gm_sub/0091BC58.inc"
};
u32 D_0091BC68[3] = {
#include "gm_sub/0091BC68.inc"
};
u32 D_0091BC78[3] = {
#include "gm_sub/0091BC78.inc"
};
u32 D_0091BC88[3] = {
#include "gm_sub/0091BC88.inc"
};
u32 D_0091BC98[3] = {
#include "gm_sub/0091BC98.inc"
};
u32 D_0091BCA8[3] = {
#include "gm_sub/0091BCA8.inc"
};
u32 D_0091BCB8[3] = {
#include "gm_sub/0091BCB8.inc"
};
u32 D_0091BCC8[3] = {
#include "gm_sub/0091BCC8.inc"
};
u32 D_0091BCD8[3] = {
#include "gm_sub/0091BCD8.inc"
};
u32 D_0091BCE8[3] = {
#include "gm_sub/0091BCE8.inc"
};
u32 D_0091BCF8[3] = {
#include "gm_sub/0091BCF8.inc"
};
u32 D_0091BD08[3] = {
#include "gm_sub/0091BD08.inc"
};
u32 D_0091BD18[3] = {
#include "gm_sub/0091BD18.inc"
};
u32 D_0091BD30[32] = {
#include "gm_sub/0091BD30.inc"
};
u32 D_0091BDB0[22] = {
#include "gm_sub/0091BDB0.inc"
};
u32 D_0091BE08[3] = {
#include "gm_sub/0091BE08.inc"
};
u32 D_0091BE18[3] = {
#include "gm_sub/0091BE18.inc"
};
u32 D_0091BE30[8] = {
#include "gm_sub/0091BE30.inc"
};
u32 D_0091BE50[8] = {
#include "gm_sub/0091BE50.inc"
};
u32 D_0091BE70[8] = {
#include "gm_sub/0091BE70.inc"
};
u32 D_0091BE90[12] = {
#include "gm_sub/0091BE90.inc"
};
u32 D_0091BEC0[4] = {
#include "gm_sub/0091BEC0.inc"
};
u32 D_0091BED0[12] = {
#include "gm_sub/0091BED0.inc"
};
u32 D_0091BF00[20] = {
#include "gm_sub/0091BF00.inc"
};
u32 D_0091BF50[8] = {
#include "gm_sub/0091BF50.inc"
};
u32 D_0091BF70[40] = {
#include "gm_sub/0091BF70.inc"
};
u32 D_0091C010[12] = {
#include "gm_sub/0091C010.inc"
};
u32 D_0091C040[12] = {
#include "gm_sub/0091C040.inc"
};
u32 D_0091C070[12] = {
#include "gm_sub/0091C070.inc"
};
u32 D_0091C0A0[12] = {
#include "gm_sub/0091C0A0.inc"
};
u32 D_0091C0D0[12] = {
#include "gm_sub/0091C0D0.inc"
};
u32 D_0091C100[12] = {
#include "gm_sub/0091C100.inc"
};
u32 D_0091C130[34] = {
#include "gm_sub/0091C130.inc"
};
u32 D_0091C1B8[3] = {
#include "gm_sub/0091C1B8.inc"
};
u32 D_0091C1C8[3] = {
#include "gm_sub/0091C1C8.inc"
};
u32 D_0091C1D8[3] = {
#include "gm_sub/0091C1D8.inc"
};
u32 D_0091C1F0[8] = {
#include "gm_sub/0091C1F0.inc"
};
u32 D_0091C210[4] = {
#include "gm_sub/0091C210.inc"
};
u32 D_0091C220[4] = {
#include "gm_sub/0091C220.inc"
};
u32 D_0091C230[4] = {
#include "gm_sub/0091C230.inc"
};
u32 D_0091C240[4] = {
#include "gm_sub/0091C240.inc"
};
u32 D_0091C250[4] = {
#include "gm_sub/0091C250.inc"
};
u32 D_0091C260[4] = {
#include "gm_sub/0091C260.inc"
};
u32 D_0091C270[4] = {
#include "gm_sub/0091C270.inc"
};
u32 D_0091C280[16] = {
#include "gm_sub/0091C280.inc"
};
u32 D_0091C2C0[4] = {
#include "gm_sub/0091C2C0.inc"
};
u32 D_0091C2D0[4] = {
#include "gm_sub/0091C2D0.inc"
};
u32 D_0091C2E0[4] = {
#include "gm_sub/0091C2E0.inc"
};
u32 D_0091C2F0[4] = {
#include "gm_sub/0091C2F0.inc"
};
u32 D_0091C300[4] = {
#include "gm_sub/0091C300.inc"
};
u32 D_0091C310[4] = {
#include "gm_sub/0091C310.inc"
};
u32 D_0091C320[4] = {
#include "gm_sub/0091C320.inc"
};
u32 D_0091C330[16] = {
#include "gm_sub/0091C330.inc"
};
u32 D_0091C370[32] = {
#include "gm_sub/0091C370.inc"
};
u32 D_0091C3F0[8] = {
#include "gm_sub/0091C3F0.inc"
};
u32 D_0091C410[332] = {
#include "gm_sub/0091C410.inc"
};
u32 D_0091C940[12] = {
#include "gm_sub/0091C940.inc"
};
u32 D_0091C970[20] = {
#include "gm_sub/0091C970.inc"
};
u32 D_0091C9C0[24] = {
#include "gm_sub/0091C9C0.inc"
};
u32 D_0091CA20[24] = {
#include "gm_sub/0091CA20.inc"
};
u32 D_0091CA80[12] = {
#include "gm_sub/0091CA80.inc"
};
u32 D_0091CAB0[12] = {
#include "gm_sub/0091CAB0.inc"
};
u32 D_0091CAE0[20] = {
#include "gm_sub/0091CAE0.inc"
};
u32 D_0091CB30[20] = {
#include "gm_sub/0091CB30.inc"
};
u32 D_0091CB80[12] = {
#include "gm_sub/0091CB80.inc"
};
u32 D_0091CBB0[20] = {
#include "gm_sub/0091CBB0.inc"
};
u32 D_0091CC00[4] = {
#include "gm_sub/0091CC00.inc"
};
u32 D_0091CC10[4] = {
#include "gm_sub/0091CC10.inc"
};
u32 D_0091CC20[4] = {
#include "gm_sub/0091CC20.inc"
};
u32 D_0091CC30[4] = {
#include "gm_sub/0091CC30.inc"
};
u32 D_0091CC40[4] = {
#include "gm_sub/0091CC40.inc"
};
u32 D_0091CC50[4] = {
#include "gm_sub/0091CC50.inc"
};
u32 D_0091CC60[16] = {
#include "gm_sub/0091CC60.inc"
};
u32 D_0091CCA0[16] = {
#include "gm_sub/0091CCA0.inc"
};
u32 D_0091CCE0[12] = {
#include "gm_sub/0091CCE0.inc"
};
u32 D_0091CD10[16] = {
#include "gm_sub/0091CD10.inc"
};
u32 D_0091CD50[16] = {
#include "gm_sub/0091CD50.inc"
};
u32 D_0091CD90[12] = {
#include "gm_sub/0091CD90.inc"
};
u32 D_0091CDC0[20] = {
#include "gm_sub/0091CDC0.inc"
};
u32 D_0091CE10[20] = {
#include "gm_sub/0091CE10.inc"
};
u32 D_0091CE60[12] = {
#include "gm_sub/0091CE60.inc"
};
u32 D_0091CE90[12] = {
#include "gm_sub/0091CE90.inc"
};
u32 D_0091CEC0[8] = {
#include "gm_sub/0091CEC0.inc"
};
u32 D_0091CEE0[12] = {
#include "gm_sub/0091CEE0.inc"
};
u32 D_0091CF10[12] = {
#include "gm_sub/0091CF10.inc"
};
u32 D_0091CF40[12] = {
#include "gm_sub/0091CF40.inc"
};
u32 D_0091CF70[12] = {
#include "gm_sub/0091CF70.inc"
};
u32 D_0091CFA0[12] = {
#include "gm_sub/0091CFA0.inc"
};
u32 D_0091CFD0[12] = {
#include "gm_sub/0091CFD0.inc"
};
u32 D_0091D000[12] = {
#include "gm_sub/0091D000.inc"
};
u32 D_0091D030[12] = {
#include "gm_sub/0091D030.inc"
};
u32 D_0091D060[12] = {
#include "gm_sub/0091D060.inc"
};
u32 D_0091D090[340] = {
#include "gm_sub/0091D090.inc"
};
u32 D_0091D5E0[36] = {
#include "gm_sub/0091D5E0.inc"
};
u32 D_0091D670[24] = {
#include "gm_sub/0091D670.inc"
};
u32 D_0091D6D0[48] = {
#include "gm_sub/0091D6D0.inc"
};
u32 D_0091D790[48] = {
#include "gm_sub/0091D790.inc"
};
u32 D_0091D850[44] = {
#include "gm_sub/0091D850.inc"
};
u32 D_0091D900[20] = {
#include "gm_sub/0091D900.inc"
};
u32 D_0091D950[12] = {
#include "gm_sub/0091D950.inc"
};
u32 D_0091D980[12] = {
#include "gm_sub/0091D980.inc"
};
u32 D_0091D9B0[12] = {
#include "gm_sub/0091D9B0.inc"
};
u32 D_0091D9E0[24] = {
#include "gm_sub/0091D9E0.inc"
};
u32 D_0091DA40[12] = {
#include "gm_sub/0091DA40.inc"
};
u32 D_0091DA70[8] = {
#include "gm_sub/0091DA70.inc"
};
u32 D_0091DA90[20] = {
#include "gm_sub/0091DA90.inc"
};
u32 D_0091DAE0[12] = {
#include "gm_sub/0091DAE0.inc"
};
u32 D_0091DB10[16] = {
#include "gm_sub/0091DB10.inc"
};
u32 D_0091DB50[12] = {
#include "gm_sub/0091DB50.inc"
};
u32 D_0091DB80[4] = {
#include "gm_sub/0091DB80.inc"
};
u32 D_0091DB90[56] = {
#include "gm_sub/0091DB90.inc"
};
u32 D_0091DC70[8] = {
#include "gm_sub/0091DC70.inc"
};
u32 D_0091DC90[8] = {
#include "gm_sub/0091DC90.inc"
};
u32 D_0091DCB0[8] = {
#include "gm_sub/0091DCB0.inc"
};
u32 D_0091DCD0[8] = {
#include "gm_sub/0091DCD0.inc"
};
u32 D_0091DCF0[6] = {
#include "gm_sub/0091DCF0.inc"
};
u32 D_0091DD08[3] = {
#include "gm_sub/0091DD08.inc"
};
u32 D_0091DD18[3] = {
#include "gm_sub/0091DD18.inc"
};
u32 D_0091DD28[3] = {
#include "gm_sub/0091DD28.inc"
};
u32 D_0091DD38[3] = {
#include "gm_sub/0091DD38.inc"
};
u32 D_0091DD48[3] = {
#include "gm_sub/0091DD48.inc"
};
u32 D_0091DD58[3] = {
#include "gm_sub/0091DD58.inc"
};
u32 D_0091DD68[3] = {
#include "gm_sub/0091DD68.inc"
};
u32 D_0091DD78[3] = {
#include "gm_sub/0091DD78.inc"
};
u32 D_0091DD88[3] = {
#include "gm_sub/0091DD88.inc"
};
u32 D_0091DD98[3] = {
#include "gm_sub/0091DD98.inc"
};
u32 D_0091DDB0[50] = {
#include "gm_sub/0091DDB0.inc"
};
u32 D_0091DE78[3] = {
#include "gm_sub/0091DE78.inc"
};
u32 D_0091DE88[3] = {
#include "gm_sub/0091DE88.inc"
};
u32 D_0091DE98[3] = {
#include "gm_sub/0091DE98.inc"
};
u32 D_0091DEA8[3] = {
#include "gm_sub/0091DEA8.inc"
};
u32 D_0091DEB8[3] = {
#include "gm_sub/0091DEB8.inc"
};
u32 D_0091DED0[12] = {
#include "gm_sub/0091DED0.inc"
};
u32 D_0091DF00[4] = {
#include "gm_sub/0091DF00.inc"
};
u32 D_0091DF10[4] = {
#include "gm_sub/0091DF10.inc"
};
u32 D_0091DF20[4] = {
#include "gm_sub/0091DF20.inc"
};
u32 D_0091DF30[4] = {
#include "gm_sub/0091DF30.inc"
};
u32 D_0091DF40[4] = {
#include "gm_sub/0091DF40.inc"
};
u32 D_0091DF50[20] = {
#include "gm_sub/0091DF50.inc"
};
u32 D_0091DFA0[4] = {
#include "gm_sub/0091DFA0.inc"
};
u32 D_0091DFB0[4] = {
#include "gm_sub/0091DFB0.inc"
};
u32 D_0091DFC0[4] = {
#include "gm_sub/0091DFC0.inc"
};
u32 D_0091DFD0[4] = {
#include "gm_sub/0091DFD0.inc"
};
u32 D_0091DFE0[4] = {
#include "gm_sub/0091DFE0.inc"
};
u32 D_0091DFF0[4] = {
#include "gm_sub/0091DFF0.inc"
};
u32 D_0091E000[14] = {
#include "gm_sub/0091E000.inc"
};
u32 D_0091E038[2] = {
#include "gm_sub/0091E038.inc"
};
u32 D_0091E040[4] = {
#include "gm_sub/0091E040.inc"
};
u32 D_0091E050[4] = {
#include "gm_sub/0091E050.inc"
};
u32 D_0091E060[4] = {
#include "gm_sub/0091E060.inc"
};
u32 D_0091E070[4] = {
#include "gm_sub/0091E070.inc"
};
u32 D_0091E080[4] = {
#include "gm_sub/0091E080.inc"
};
u32 D_0091E090[4] = {
#include "gm_sub/0091E090.inc"
};
u32 D_0091E0A0[4] = {
#include "gm_sub/0091E0A0.inc"
};
u32 D_0091E0B0[4] = {
#include "gm_sub/0091E0B0.inc"
};
u32 D_0091E0C0[4] = {
#include "gm_sub/0091E0C0.inc"
};
u32 D_0091E0D0[4] = {
#include "gm_sub/0091E0D0.inc"
};
u32 D_0091E0E0[816] = {
#include "gm_sub/0091E0E0.inc"
};
u32 D_0091EDA0[12] = {
#include "gm_sub/0091EDA0.inc"
};
u32 D_0091EDD0[12] = {
#include "gm_sub/0091EDD0.inc"
};
u32 D_0091EE00[24] = {
#include "gm_sub/0091EE00.inc"
};
u32 D_0091EE60[12] = {
#include "gm_sub/0091EE60.inc"
};
u32 D_0091EE90[12] = {
#include "gm_sub/0091EE90.inc"
};
u32 D_0091EEC0[12] = {
#include "gm_sub/0091EEC0.inc"
};
u32 D_0091EEF0[12] = {
#include "gm_sub/0091EEF0.inc"
};
u32 D_0091EF20[12] = {
#include "gm_sub/0091EF20.inc"
};
u32 D_0091EF50[12] = {
#include "gm_sub/0091EF50.inc"
};
u32 D_0091EF80[12] = {
#include "gm_sub/0091EF80.inc"
};
u32 D_0091EFB0[16] = {
#include "gm_sub/0091EFB0.inc"
};
u32 D_0091EFF0[16] = {
#include "gm_sub/0091EFF0.inc"
};
u32 D_0091F030[12] = {
#include "gm_sub/0091F030.inc"
};
u32 D_0091F060[12] = {
#include "gm_sub/0091F060.inc"
};
u32 D_0091F090[16] = {
#include "gm_sub/0091F090.inc"
};
u32 D_0091F0D0[16] = {
#include "gm_sub/0091F0D0.inc"
};
u32 D_0091F110[12] = {
#include "gm_sub/0091F110.inc"
};
u32 D_0091F140[12] = {
#include "gm_sub/0091F140.inc"
};
u32 D_0091F170[16] = {
#include "gm_sub/0091F170.inc"
};
u32 D_0091F1B0[12] = {
#include "gm_sub/0091F1B0.inc"
};
u32 D_0091F1E0[12] = {
#include "gm_sub/0091F1E0.inc"
};
u32 D_0091F210[12] = {
#include "gm_sub/0091F210.inc"
};
u32 D_0091F240[20] = {
#include "gm_sub/0091F240.inc"
};
u32 D_0091F290[20] = {
#include "gm_sub/0091F290.inc"
};
u32 D_0091F2E0[12] = {
#include "gm_sub/0091F2E0.inc"
};
u32 D_0091F310[52] = {
#include "gm_sub/0091F310.inc"
};
u32 D_0091F3E0[4] = {
#include "gm_sub/0091F3E0.inc"
};
u32 D_0091F3F0[4] = {
#include "gm_sub/0091F3F0.inc"
};
u32 D_0091F400[4] = {
#include "gm_sub/0091F400.inc"
};
u32 D_0091F410[4] = {
#include "gm_sub/0091F410.inc"
};
u32 D_0091F420[4] = {
#include "gm_sub/0091F420.inc"
};
u32 D_0091F430[4] = {
#include "gm_sub/0091F430.inc"
};
u32 D_0091F440[16] = {
#include "gm_sub/0091F440.inc"
};
u32 D_0091F480[8] = {
#include "gm_sub/0091F480.inc"
};
u32 D_0091F4A0[20] = {
#include "gm_sub/0091F4A0.inc"
};
u32 D_0091F4F0[20] = {
#include "gm_sub/0091F4F0.inc"
};
u32 D_0091F540[16] = {
#include "gm_sub/0091F540.inc"
};
u32 D_0091F580[20] = {
#include "gm_sub/0091F580.inc"
};
u32 D_0091F5D0[28] = {
#include "gm_sub/0091F5D0.inc"
};
u32 D_0091F640[8] = {
#include "gm_sub/0091F640.inc"
};
u32 D_0091F660[28] = {
#include "gm_sub/0091F660.inc"
};
u32 D_0091F6D0[28] = {
#include "gm_sub/0091F6D0.inc"
};
u32 D_0091F740[12] = {
#include "gm_sub/0091F740.inc"
};
u32 D_0091F770[20] = {
#include "gm_sub/0091F770.inc"
};
u32 D_0091F7C0[20] = {
#include "gm_sub/0091F7C0.inc"
};
u32 D_0091F810[12] = {
#include "gm_sub/0091F810.inc"
};
u32 D_0091F840[20] = {
#include "gm_sub/0091F840.inc"
};
u32 D_0091F890[8] = {
#include "gm_sub/0091F890.inc"
};
u32 D_0091F8B0[12] = {
#include "gm_sub/0091F8B0.inc"
};
u32 D_0091F8E0[8] = {
#include "gm_sub/0091F8E0.inc"
};
u32 D_0091F900[16] = {
#include "gm_sub/0091F900.inc"
};
u32 D_0091F940[16] = {
#include "gm_sub/0091F940.inc"
};
u32 D_0091F980[16] = {
#include "gm_sub/0091F980.inc"
};
u32 D_0091F9C0[20] = {
#include "gm_sub/0091F9C0.inc"
};
u32 D_0091FA10[28] = {
#include "gm_sub/0091FA10.inc"
};
u32 D_0091FA80[28] = {
#include "gm_sub/0091FA80.inc"
};
u32 D_0091FAF0[12] = {
#include "gm_sub/0091FAF0.inc"
};
u32 D_0091FB20[20] = {
#include "gm_sub/0091FB20.inc"
};
u32 D_0091FB70[8] = {
#include "gm_sub/0091FB70.inc"
};
u32 D_0091FB90[16] = {
#include "gm_sub/0091FB90.inc"
};
u32 D_0091FBD0[16] = {
#include "gm_sub/0091FBD0.inc"
};
u32 D_0091FC10[12] = {
#include "gm_sub/0091FC10.inc"
};
u32 D_0091FC40[20] = {
#include "gm_sub/0091FC40.inc"
};
u32 D_0091FC90[8] = {
#include "gm_sub/0091FC90.inc"
};
u32 D_0091FCB0[16] = {
#include "gm_sub/0091FCB0.inc"
};
u32 D_0091FCF0[8] = {
#include "gm_sub/0091FCF0.inc"
};
u32 D_0091FD10[16] = {
#include "gm_sub/0091FD10.inc"
};
u32 D_0091FD50[16] = {
#include "gm_sub/0091FD50.inc"
};
u32 D_0091FD90[12] = {
#include "gm_sub/0091FD90.inc"
};
u32 D_0091FDC0[20] = {
#include "gm_sub/0091FDC0.inc"
};
u32 D_0091FE10[20] = {
#include "gm_sub/0091FE10.inc"
};
u32 D_0091FE60[12] = {
#include "gm_sub/0091FE60.inc"
};
u32 D_0091FE90[20] = {
#include "gm_sub/0091FE90.inc"
};
u32 D_0091FEE0[20] = {
#include "gm_sub/0091FEE0.inc"
};
u32 D_0091FF30[12] = {
#include "gm_sub/0091FF30.inc"
};
u32 D_0091FF60[16] = {
#include "gm_sub/0091FF60.inc"
};
u32 D_0091FFA0[16] = {
#include "gm_sub/0091FFA0.inc"
};
u32 D_0091FFE0[12] = {
#include "gm_sub/0091FFE0.inc"
};
u32 D_00920010[16] = {
#include "gm_sub/00920010.inc"
};
u32 D_00920050[16] = {
#include "gm_sub/00920050.inc"
};
u32 D_00920090[12] = {
#include "gm_sub/00920090.inc"
};
u32 D_009200C0[16] = {
#include "gm_sub/009200C0.inc"
};
u32 D_00920100[16] = {
#include "gm_sub/00920100.inc"
};
u32 D_00920140[12] = {
#include "gm_sub/00920140.inc"
};
u32 D_00920170[20] = {
#include "gm_sub/00920170.inc"
};
u32 D_009201C0[8] = {
#include "gm_sub/009201C0.inc"
};
u32 D_009201E0[16] = {
#include "gm_sub/009201E0.inc"
};
u32 D_00920220[20] = {
#include "gm_sub/00920220.inc"
};
u32 D_00920270[12] = {
#include "gm_sub/00920270.inc"
};
u32 D_009202A0[20] = {
#include "gm_sub/009202A0.inc"
};
u32 D_009202F0[8] = {
#include "gm_sub/009202F0.inc"
};
u32 D_00920310[32] = {
#include "gm_sub/00920310.inc"
};
u32 D_00920390[28] = {
#include "gm_sub/00920390.inc"
};
u32 D_00920400[12] = {
#include "gm_sub/00920400.inc"
};
u32 D_00920430[20] = {
#include "gm_sub/00920430.inc"
};
u32 D_00920480[20] = {
#include "gm_sub/00920480.inc"
};
u32 D_009204D0[12] = {
#include "gm_sub/009204D0.inc"
};
u32 D_00920500[380] = {
#include "gm_sub/00920500.inc"
};
u32 D_00920AF0[12] = {
#include "gm_sub/00920AF0.inc"
};
u32 D_00920B20[16] = {
#include "gm_sub/00920B20.inc"
};
u32 D_00920B60[20] = {
#include "gm_sub/00920B60.inc"
};
u32 D_00920BB0[24] = {
#include "gm_sub/00920BB0.inc"
};
u32 D_00920C10[16] = {
#include "gm_sub/00920C10.inc"
};
u32 D_00920C50[16] = {
#include "gm_sub/00920C50.inc"
};
u32 D_00920C90[20] = {
#include "gm_sub/00920C90.inc"
};
u32 D_00920CE0[16] = {
#include "gm_sub/00920CE0.inc"
};
u32 D_00920D20[16] = {
#include "gm_sub/00920D20.inc"
};
u32 D_00920D60[32] = {
#include "gm_sub/00920D60.inc"
};
u32 D_00920DE0[12] = {
#include "gm_sub/00920DE0.inc"
};
u32 D_00920E10[16] = {
#include "gm_sub/00920E10.inc"
};
u32 D_00920E50[12] = {
#include "gm_sub/00920E50.inc"
};
u32 D_00920E80[20] = {
#include "gm_sub/00920E80.inc"
};
u32 D_00920ED0[24] = {
#include "gm_sub/00920ED0.inc"
};
u32 D_00920F30[12] = {
#include "gm_sub/00920F30.inc"
};
u32 D_00920F60[12] = {
#include "gm_sub/00920F60.inc"
};
u32 D_00920F90[24] = {
#include "gm_sub/00920F90.inc"
};
u32 D_00920FF0[12] = {
#include "gm_sub/00920FF0.inc"
};
u32 D_00921020[12] = {
#include "gm_sub/00921020.inc"
};
u32 D_00921050[12] = {
#include "gm_sub/00921050.inc"
};
u32 D_00921080[16] = {
#include "gm_sub/00921080.inc"
};
u32 D_009210C0[12] = {
#include "gm_sub/009210C0.inc"
};
u32 D_009210F0[48] = {
#include "gm_sub/009210F0.inc"
};
u32 D_009211B0[2] = {
#include "gm_sub/009211B0.inc"
};
u32 D_009211B8[2] = {
#include "gm_sub/009211B8.inc"
};
u32 D_009211C0[4] = {
#include "gm_sub/009211C0.inc"
};
u32 D_009211D0[16] = {
#include "gm_sub/009211D0.inc"
};
u32 D_00921210[8] = {
#include "gm_sub/00921210.inc"
};
u32 D_00921230[12] = {
#include "gm_sub/00921230.inc"
};
u32 D_00921260[16] = {
#include "gm_sub/00921260.inc"
};
u32 D_009212A0[20] = {
#include "gm_sub/009212A0.inc"
};
u32 D_009212F0[16] = {
#include "gm_sub/009212F0.inc"
};
u32 D_00921330[16] = {
#include "gm_sub/00921330.inc"
};
u32 D_00921370[12] = {
#include "gm_sub/00921370.inc"
};
u32 D_009213A0[16] = {
#include "gm_sub/009213A0.inc"
};
u32 D_009213E0[16] = {
#include "gm_sub/009213E0.inc"
};
u32 D_00921420[20] = {
#include "gm_sub/00921420.inc"
};
u32 D_00921470[16] = {
#include "gm_sub/00921470.inc"
};
u32 D_009214B0[16] = {
#include "gm_sub/009214B0.inc"
};
u32 D_009214F0[20] = {
#include "gm_sub/009214F0.inc"
};
u32 D_00921540[20] = {
#include "gm_sub/00921540.inc"
};
u32 D_00921590[12] = {
#include "gm_sub/00921590.inc"
};
u32 D_009215C0[12] = {
#include "gm_sub/009215C0.inc"
};
u32 D_009215F0[8] = {
#include "gm_sub/009215F0.inc"
};
u32 D_00921610[12] = {
#include "gm_sub/00921610.inc"
};
u32 D_00921640[8] = {
#include "gm_sub/00921640.inc"
};
u32 D_00921660[12] = {
#include "gm_sub/00921660.inc"
};
u32 D_00921690[12] = {
#include "gm_sub/00921690.inc"
};
u32 D_009216C0[12] = {
#include "gm_sub/009216C0.inc"
};
u32 D_009216F0[12] = {
#include "gm_sub/009216F0.inc"
};
u32 D_00921720[12] = {
#include "gm_sub/00921720.inc"
};
u32 D_00921750[12] = {
#include "gm_sub/00921750.inc"
};
u32 D_00921780[12] = {
#include "gm_sub/00921780.inc"
};
u32 D_009217B0[12] = {
#include "gm_sub/009217B0.inc"
};
u32 D_009217E0[12] = {
#include "gm_sub/009217E0.inc"
};
u32 D_00921810[20] = {
#include "gm_sub/00921810.inc"
};
u32 D_00921860[12] = {
#include "gm_sub/00921860.inc"
};
u32 D_00921890[8] = {
#include "gm_sub/00921890.inc"
};
u32 D_009218B0[12] = {
#include "gm_sub/009218B0.inc"
};
u32 D_009218E0[12] = {
#include "gm_sub/009218E0.inc"
};
u32 D_00921910[20] = {
#include "gm_sub/00921910.inc"
};
u32 D_00921960[16] = {
#include "gm_sub/00921960.inc"
};
u32 D_009219A0[8] = {
#include "gm_sub/009219A0.inc"
};
u32 D_009219C0[16] = {
#include "gm_sub/009219C0.inc"
};
u32 D_00921A00[8] = {
#include "gm_sub/00921A00.inc"
};
u32 D_00921A20[64] = {
#include "gm_sub/00921A20.inc"
};
u32 D_00921B20[4] = {
#include "gm_sub/00921B20.inc"
};
u32 D_00921B30[4] = {
#include "gm_sub/00921B30.inc"
};
u32 D_00921B40[4] = {
#include "gm_sub/00921B40.inc"
};
u32 D_00921B50[4] = {
#include "gm_sub/00921B50.inc"
};
u32 D_00921B60[4] = {
#include "gm_sub/00921B60.inc"
};
u32 D_00921B70[4] = {
#include "gm_sub/00921B70.inc"
};
u32 D_00921B80[14] = {
#include "gm_sub/00921B80.inc"
};
u32 D_00921BB8[2] = {
#include "gm_sub/00921BB8.inc"
};
u32 D_00921BC0[4] = {
#include "gm_sub/00921BC0.inc"
};
u32 D_00921BD0[4] = {
#include "gm_sub/00921BD0.inc"
};
u32 D_00921BE0[4] = {
#include "gm_sub/00921BE0.inc"
};
u32 D_00921BF0[4] = {
#include "gm_sub/00921BF0.inc"
};
u32 D_00921C00[4] = {
#include "gm_sub/00921C00.inc"
};
u32 D_00921C10[12] = {
#include "gm_sub/00921C10.inc"
};
u32 D_00921C40[4] = {
#include "gm_sub/00921C40.inc"
};
u32 D_00921C50[4] = {
#include "gm_sub/00921C50.inc"
};
u32 D_00921C60[4] = {
#include "gm_sub/00921C60.inc"
};
u32 D_00921C70[4] = {
#include "gm_sub/00921C70.inc"
};
u32 D_00921C80[4] = {
#include "gm_sub/00921C80.inc"
};
u32 D_00921C90[4] = {
#include "gm_sub/00921C90.inc"
};
u32 D_00921CA0[4] = {
#include "gm_sub/00921CA0.inc"
};
u32 D_00921CB0[14] = {
#include "gm_sub/00921CB0.inc"
};
u32 D_00921CE8[2] = {
#include "gm_sub/00921CE8.inc"
};
u32 D_00921CF0[4] = {
#include "gm_sub/00921CF0.inc"
};
u32 D_00921D00[4] = {
#include "gm_sub/00921D00.inc"
};
u32 D_00921D10[4] = {
#include "gm_sub/00921D10.inc"
};
u32 D_00921D20[4] = {
#include "gm_sub/00921D20.inc"
};
u32 D_00921D30[4] = {
#include "gm_sub/00921D30.inc"
};
u32 D_00921D40[4] = {
#include "gm_sub/00921D40.inc"
};
u32 D_00921D50[4] = {
#include "gm_sub/00921D50.inc"
};
u32 D_00921D60[16] = {
#include "gm_sub/00921D60.inc"
};
u32 D_00921DA0[640] = {
#include "gm_sub/00921DA0.inc"
};
u32 D_009227A0[12] = {
#include "gm_sub/009227A0.inc"
};
u32 D_009227D0[4] = {
#include "gm_sub/009227D0.inc"
};
u32 D_009227E0[12] = {
#include "gm_sub/009227E0.inc"
};
u32 D_00922810[12] = {
#include "gm_sub/00922810.inc"
};
u32 D_00922840[8] = {
#include "gm_sub/00922840.inc"
};
u32 D_00922860[20] = {
#include "gm_sub/00922860.inc"
};
u32 D_009228B0[8] = {
#include "gm_sub/009228B0.inc"
};
u32 D_009228D0[16] = {
#include "gm_sub/009228D0.inc"
};
u32 D_00922910[6] = {
#include "gm_sub/00922910.inc"
};
u32 D_00922928[3] = {
#include "gm_sub/00922928.inc"
};
u32 D_00922940[12] = {
#include "gm_sub/00922940.inc"
};
u32 D_00922970[12] = {
#include "gm_sub/00922970.inc"
};
u32 D_009229A0[12] = {
#include "gm_sub/009229A0.inc"
};
u32 D_009229D0[20] = {
#include "gm_sub/009229D0.inc"
};
u32 D_00922A20[8] = {
#include "gm_sub/00922A20.inc"
};
u32 D_00922A40[12] = {
#include "gm_sub/00922A40.inc"
};
u32 D_00922A70[24] = {
#include "gm_sub/00922A70.inc"
};
u32 D_00922AD0[20] = {
#include "gm_sub/00922AD0.inc"
};
u32 D_00922B20[12] = {
#include "gm_sub/00922B20.inc"
};
u32 D_00922B50[12] = {
#include "gm_sub/00922B50.inc"
};
u32 D_00922B80[12] = {
#include "gm_sub/00922B80.inc"
};
u32 D_00922BB0[8] = {
#include "gm_sub/00922BB0.inc"
};
u32 D_00922BD0[12] = {
#include "gm_sub/00922BD0.inc"
};
u32 D_00922C00[12] = {
#include "gm_sub/00922C00.inc"
};
u32 D_00922C30[12] = {
#include "gm_sub/00922C30.inc"
};
u32 D_00922C60[12] = {
#include "gm_sub/00922C60.inc"
};
u32 D_00922C90[54] = {
#include "gm_sub/00922C90.inc"
};
u32 D_00922D68[3] = {
#include "gm_sub/00922D68.inc"
};
u32 D_00922D78[3] = {
#include "gm_sub/00922D78.inc"
};
u32 D_00922D88[3] = {
#include "gm_sub/00922D88.inc"
};
u32 D_00922D98[3] = {
#include "gm_sub/00922D98.inc"
};
u32 D_00922DA8[3] = {
#include "gm_sub/00922DA8.inc"
};
u32 D_00922DB8[3] = {
#include "gm_sub/00922DB8.inc"
};
u32 D_00922DC8[3] = {
#include "gm_sub/00922DC8.inc"
};
u32 D_00922DD8[3] = {
#include "gm_sub/00922DD8.inc"
};
u32 D_00922DE8[3] = {
#include "gm_sub/00922DE8.inc"
};
u32 D_00922DF8[3] = {
#include "gm_sub/00922DF8.inc"
};
u32 D_00922E08[3] = {
#include "gm_sub/00922E08.inc"
};
u32 D_00922E18[3] = {
#include "gm_sub/00922E18.inc"
};
u32 D_00922E28[3] = {
#include "gm_sub/00922E28.inc"
};
u32 D_00922E38[3] = {
#include "gm_sub/00922E38.inc"
};
u32 D_00922E48[3] = {
#include "gm_sub/00922E48.inc"
};
u32 D_00922E58[3] = {
#include "gm_sub/00922E58.inc"
};
u32 D_00922E68[3] = {
#include "gm_sub/00922E68.inc"
};
u32 D_00922E78[3] = {
#include "gm_sub/00922E78.inc"
};
u32 D_00922E88[3] = {
#include "gm_sub/00922E88.inc"
};
u32 D_00922E98[3] = {
#include "gm_sub/00922E98.inc"
};
u32 D_00922EA8[3] = {
#include "gm_sub/00922EA8.inc"
};
u32 D_00922EB8[3] = {
#include "gm_sub/00922EB8.inc"
};
u32 D_00922EC8[3] = {
#include "gm_sub/00922EC8.inc"
};
u32 D_00922ED8[3] = {
#include "gm_sub/00922ED8.inc"
};
u32 D_00922EE8[3] = {
#include "gm_sub/00922EE8.inc"
};
u32 D_00922EF8[3] = {
#include "gm_sub/00922EF8.inc"
};
u32 D_00922F10[84] = {
#include "gm_sub/00922F10.inc"
};
u32 D_00923060[4] = {
#include "gm_sub/00923060.inc"
};
u32 D_00923070[4] = {
#include "gm_sub/00923070.inc"
};
u32 D_00923080[4] = {
#include "gm_sub/00923080.inc"
};
u32 D_00923090[4] = {
#include "gm_sub/00923090.inc"
};
u32 D_009230A0[4] = {
#include "gm_sub/009230A0.inc"
};
u32 D_009230B0[4] = {
#include "gm_sub/009230B0.inc"
};
u32 D_009230C0[4] = {
#include "gm_sub/009230C0.inc"
};
u32 D_009230D0[4] = {
#include "gm_sub/009230D0.inc"
};
u32 D_009230E0[4] = {
#include "gm_sub/009230E0.inc"
};
u32 D_009230F0[4] = {
#include "gm_sub/009230F0.inc"
};
u32 D_00923100[4] = {
#include "gm_sub/00923100.inc"
};
u32 D_00923110[4] = {
#include "gm_sub/00923110.inc"
};
u32 D_00923120[4] = {
#include "gm_sub/00923120.inc"
};
u32 D_00923130[4] = {
#include "gm_sub/00923130.inc"
};
u32 D_00923140[4] = {
#include "gm_sub/00923140.inc"
};
u32 D_00923150[4] = {
#include "gm_sub/00923150.inc"
};
u32 D_00923160[34] = {
#include "gm_sub/00923160.inc"
};
u32 D_009231E8[3] = {
#include "gm_sub/009231E8.inc"
};
u32 D_009231F8[3] = {
#include "gm_sub/009231F8.inc"
};
u32 D_00923208[3] = {
#include "gm_sub/00923208.inc"
};
u32 D_00923218[3] = {
#include "gm_sub/00923218.inc"
};
u32 D_00923228[3] = {
#include "gm_sub/00923228.inc"
};
u32 D_00923238[3] = {
#include "gm_sub/00923238.inc"
};
u32 D_00923250[40] = {
#include "gm_sub/00923250.inc"
};
u32 D_009232F0[4] = {
#include "gm_sub/009232F0.inc"
};
u32 D_00923300[4] = {
#include "gm_sub/00923300.inc"
};
u32 D_00923310[4] = {
#include "gm_sub/00923310.inc"
};
u32 D_00923320[4] = {
#include "gm_sub/00923320.inc"
};
u32 D_00923330[4] = {
#include "gm_sub/00923330.inc"
};
u32 D_00923340[4] = {
#include "gm_sub/00923340.inc"
};
u32 D_00923350[4] = {
#include "gm_sub/00923350.inc"
};
u32 D_00923360[4] = {
#include "gm_sub/00923360.inc"
};
u32 D_00923370[4] = {
#include "gm_sub/00923370.inc"
};
u32 D_00923380[4] = {
#include "gm_sub/00923380.inc"
};
u32 D_00923390[4] = {
#include "gm_sub/00923390.inc"
};
u32 D_009233A0[4] = {
#include "gm_sub/009233A0.inc"
};
u32 D_009233B0[4] = {
#include "gm_sub/009233B0.inc"
};
u32 D_009233C0[28] = {
#include "gm_sub/009233C0.inc"
};
u32 D_00923430[4] = {
#include "gm_sub/00923430.inc"
};
u32 D_00923440[4] = {
#include "gm_sub/00923440.inc"
};
u32 D_00923450[4] = {
#include "gm_sub/00923450.inc"
};
u32 D_00923460[4] = {
#include "gm_sub/00923460.inc"
};
u32 D_00923470[4] = {
#include "gm_sub/00923470.inc"
};
u32 D_00923480[4] = {
#include "gm_sub/00923480.inc"
};
u32 D_00923490[4] = {
#include "gm_sub/00923490.inc"
};
u32 D_009234A0[4] = {
#include "gm_sub/009234A0.inc"
};
u32 D_009234B0[16] = {
#include "gm_sub/009234B0.inc"
};
u32 D_009234F0[4] = {
#include "gm_sub/009234F0.inc"
};
u32 D_00923500[4] = {
#include "gm_sub/00923500.inc"
};
u32 D_00923510[4] = {
#include "gm_sub/00923510.inc"
};
u32 D_00923520[8] = {
#include "gm_sub/00923520.inc"
};
u32 D_00923540[4] = {
#include "gm_sub/00923540.inc"
};
u32 D_00923550[4] = {
#include "gm_sub/00923550.inc"
};
u32 D_00923560[4] = {
#include "gm_sub/00923560.inc"
};
u32 D_00923570[4] = {
#include "gm_sub/00923570.inc"
};
u32 D_00923580[4] = {
#include "gm_sub/00923580.inc"
};
u32 D_00923590[4] = {
#include "gm_sub/00923590.inc"
};
u32 D_009235A0[4] = {
#include "gm_sub/009235A0.inc"
};
u32 D_009235B0[4] = {
#include "gm_sub/009235B0.inc"
};
u32 D_009235C0[4] = {
#include "gm_sub/009235C0.inc"
};
u32 D_009235D0[18] = {
#include "gm_sub/009235D0.inc"
};
u32 D_00923618[2] = {
#include "gm_sub/00923618.inc"
};
u32 D_00923620[4] = {
#include "gm_sub/00923620.inc"
};
u32 D_00923630[4] = {
#include "gm_sub/00923630.inc"
};
u32 D_00923640[4] = {
#include "gm_sub/00923640.inc"
};
u32 D_00923650[4] = {
#include "gm_sub/00923650.inc"
};
u32 D_00923660[12] = {
#include "gm_sub/00923660.inc"
};
u32 D_00923690[4] = {
#include "gm_sub/00923690.inc"
};
u32 D_009236A0[4] = {
#include "gm_sub/009236A0.inc"
};
u32 D_009236B0[4] = {
#include "gm_sub/009236B0.inc"
};
u32 D_009236C0[4] = {
#include "gm_sub/009236C0.inc"
};
u32 D_009236D0[4] = {
#include "gm_sub/009236D0.inc"
};
u32 D_009236E0[4] = {
#include "gm_sub/009236E0.inc"
};
u32 D_009236F0[4] = {
#include "gm_sub/009236F0.inc"
};
u32 D_00923700[16] = {
#include "gm_sub/00923700.inc"
};
u32 D_00923740[4] = {
#include "gm_sub/00923740.inc"
};
u32 D_00923750[4] = {
#include "gm_sub/00923750.inc"
};
u32 D_00923760[4] = {
#include "gm_sub/00923760.inc"
};
u32 D_00923770[4] = {
#include "gm_sub/00923770.inc"
};
u32 D_00923780[4] = {
#include "gm_sub/00923780.inc"
};
u32 D_00923790[4] = {
#include "gm_sub/00923790.inc"
};
u32 D_009237A0[4] = {
#include "gm_sub/009237A0.inc"
};
u32 D_009237B0[4] = {
#include "gm_sub/009237B0.inc"
};
u32 D_009237C0[4] = {
#include "gm_sub/009237C0.inc"
};
u32 D_009237D0[4] = {
#include "gm_sub/009237D0.inc"
};
u32 D_009237E0[4] = {
#include "gm_sub/009237E0.inc"
};
u32 D_009237F0[22] = {
#include "gm_sub/009237F0.inc"
};
u32 D_00923848[2] = {
#include "gm_sub/00923848.inc"
};
u32 D_00923850[4] = {
#include "gm_sub/00923850.inc"
};
u32 D_00923860[4] = {
#include "gm_sub/00923860.inc"
};
u32 D_00923870[4] = {
#include "gm_sub/00923870.inc"
};
u32 D_00923880[4] = {
#include "gm_sub/00923880.inc"
};
u32 D_00923890[4] = {
#include "gm_sub/00923890.inc"
};
u32 D_009238A0[4] = {
#include "gm_sub/009238A0.inc"
};
u32 D_009238B0[14] = {
#include "gm_sub/009238B0.inc"
};
u32 D_009238E8[2] = {
#include "gm_sub/009238E8.inc"
};
u32 D_009238F0[4] = {
#include "gm_sub/009238F0.inc"
};
u32 D_00923900[4] = {
#include "gm_sub/00923900.inc"
};
u32 D_00923910[340] = {
#include "gm_sub/00923910.inc"
};
u32 D_00923E60[24] = {
#include "gm_sub/00923E60.inc"
};
u32 D_00923EC0[24] = {
#include "gm_sub/00923EC0.inc"
};
u32 D_00923F20[20] = {
#include "gm_sub/00923F20.inc"
};
u32 D_00923F70[24] = {
#include "gm_sub/00923F70.inc"
};
u32 D_00923FD0[28] = {
#include "gm_sub/00923FD0.inc"
};
u32 D_00924040[28] = {
#include "gm_sub/00924040.inc"
};
u32 D_009240B0[14] = {
#include "gm_sub/009240B0.inc"
};
u32 D_009240E8[3] = {
#include "gm_sub/009240E8.inc"
};
u32 D_00924100[4] = {
#include "gm_sub/00924100.inc"
};
u32 D_00924110[4] = {
#include "gm_sub/00924110.inc"
};
u32 D_00924120[4] = {
#include "gm_sub/00924120.inc"
};
u32 D_00924130[4] = {
#include "gm_sub/00924130.inc"
};
u32 D_00924140[4] = {
#include "gm_sub/00924140.inc"
};
u32 D_00924150[8] = {
#include "gm_sub/00924150.inc"
};
u32 D_00924170[8] = {
#include "gm_sub/00924170.inc"
};
u32 D_00924190[8] = {
#include "gm_sub/00924190.inc"
};
u32 D_009241B0[8] = {
#include "gm_sub/009241B0.inc"
};
u32 D_009241D0[8] = {
#include "gm_sub/009241D0.inc"
};
u32 D_009241F0[8] = {
#include "gm_sub/009241F0.inc"
};
u32 D_00924210[8] = {
#include "gm_sub/00924210.inc"
};
u32 D_00924230[8] = {
#include "gm_sub/00924230.inc"
};
u32 D_00924250[8] = {
#include "gm_sub/00924250.inc"
};
u32 D_00924270[8] = {
#include "gm_sub/00924270.inc"
};
u32 D_00924290[628] = {
#include "gm_sub/00924290.inc"
};
u32 D_00924C60[48] = {
#include "gm_sub/00924C60.inc"
};
u32 D_00924D20[8] = {
#include "gm_sub/00924D20.inc"
};
u32 D_00924D40[40] = {
#include "gm_sub/00924D40.inc"
};
u32 D_00924DE0[8] = {
#include "gm_sub/00924DE0.inc"
};
u32 D_00924E00[8] = {
#include "gm_sub/00924E00.inc"
};
u32 D_00924E20[44] = {
#include "gm_sub/00924E20.inc"
};
u32 D_00924ED0[20] = {
#include "gm_sub/00924ED0.inc"
};
u32 D_00924F20[16] = {
#include "gm_sub/00924F20.inc"
};
u32 D_00924F60[4] = {
#include "gm_sub/00924F60.inc"
};
u32 D_00924F70[4] = {
#include "gm_sub/00924F70.inc"
};
u32 D_00924F80[4] = {
#include "gm_sub/00924F80.inc"
};
u32 D_00924F90[4] = {
#include "gm_sub/00924F90.inc"
};
u32 D_00924FA0[4] = {
#include "gm_sub/00924FA0.inc"
};
u32 D_00924FB0[4] = {
#include "gm_sub/00924FB0.inc"
};
u32 D_00924FC0[8] = {
#include "gm_sub/00924FC0.inc"
};
u32 D_00924FE0[8] = {
#include "gm_sub/00924FE0.inc"
};
u32 D_00925000[8] = {
#include "gm_sub/00925000.inc"
};
u32 D_00925020[8] = {
#include "gm_sub/00925020.inc"
};
u32 D_00925040[8] = {
#include "gm_sub/00925040.inc"
};
u32 D_00925060[8] = {
#include "gm_sub/00925060.inc"
};
u32 D_00925080[8] = {
#include "gm_sub/00925080.inc"
};
u32 D_009250A0[8] = {
#include "gm_sub/009250A0.inc"
};
u32 D_009250C0[8] = {
#include "gm_sub/009250C0.inc"
};
u32 D_009250E0[8] = {
#include "gm_sub/009250E0.inc"
};
u32 D_00925100[324] = {
#include "gm_sub/00925100.inc"
};
u32 D_00925610[12] = {
#include "gm_sub/00925610.inc"
};
u32 D_00925640[12] = {
#include "gm_sub/00925640.inc"
};
u32 D_00925670[16] = {
#include "gm_sub/00925670.inc"
};
u32 D_009256B0[20] = {
#include "gm_sub/009256B0.inc"
};
u32 D_00925700[12] = {
#include "gm_sub/00925700.inc"
};
u32 D_00925730[12] = {
#include "gm_sub/00925730.inc"
};
u32 D_00925760[8] = {
#include "gm_sub/00925760.inc"
};
u32 D_00925780[12] = {
#include "gm_sub/00925780.inc"
};
u32 D_009257B0[12] = {
#include "gm_sub/009257B0.inc"
};
u32 D_009257E0[12] = {
#include "gm_sub/009257E0.inc"
};
u32 D_00925810[12] = {
#include "gm_sub/00925810.inc"
};
u32 D_00925840[12] = {
#include "gm_sub/00925840.inc"
};
u32 D_00925870[24] = {
#include "gm_sub/00925870.inc"
};
u32 D_009258D0[12] = {
#include "gm_sub/009258D0.inc"
};
u32 D_00925900[12] = {
#include "gm_sub/00925900.inc"
};
u32 D_00925930[12] = {
#include "gm_sub/00925930.inc"
};
u32 D_00925960[12] = {
#include "gm_sub/00925960.inc"
};
u32 D_00925990[12] = {
#include "gm_sub/00925990.inc"
};
u32 D_009259C0[12] = {
#include "gm_sub/009259C0.inc"
};
u32 D_009259F0[40] = {
#include "gm_sub/009259F0.inc"
};
u32 D_00925A90[4] = {
#include "gm_sub/00925A90.inc"
};
u32 D_00925AA0[4] = {
#include "gm_sub/00925AA0.inc"
};
u32 D_00925AB0[4] = {
#include "gm_sub/00925AB0.inc"
};
u32 D_00925AC0[4] = {
#include "gm_sub/00925AC0.inc"
};
u32 D_00925AD0[4] = {
#include "gm_sub/00925AD0.inc"
};
u32 D_00925AE0[4] = {
#include "gm_sub/00925AE0.inc"
};
u32 D_00925AF0[4] = {
#include "gm_sub/00925AF0.inc"
};
u32 D_00925B00[4] = {
#include "gm_sub/00925B00.inc"
};
u32 D_00925B10[4] = {
#include "gm_sub/00925B10.inc"
};
u32 D_00925B20[4] = {
#include "gm_sub/00925B20.inc"
};
u32 D_00925B30[4] = {
#include "gm_sub/00925B30.inc"
};
u32 D_00925B40[4] = {
#include "gm_sub/00925B40.inc"
};
u32 D_00925B50[4] = {
#include "gm_sub/00925B50.inc"
};
u32 D_00925B60[4] = {
#include "gm_sub/00925B60.inc"
};
u32 D_00925B70[4] = {
#include "gm_sub/00925B70.inc"
};
u32 D_00925B80[4] = {
#include "gm_sub/00925B80.inc"
};
u32 D_00925B90[4] = {
#include "gm_sub/00925B90.inc"
};
u32 D_00925BA0[4] = {
#include "gm_sub/00925BA0.inc"
};
u32 D_00925BB0[4] = {
#include "gm_sub/00925BB0.inc"
};
u32 D_00925BC0[40] = {
#include "gm_sub/00925BC0.inc"
};
u32 D_00925C60[4] = {
#include "gm_sub/00925C60.inc"
};
u32 D_00925C70[4] = {
#include "gm_sub/00925C70.inc"
};
u32 D_00925C80[4] = {
#include "gm_sub/00925C80.inc"
};
u32 D_00925C90[4] = {
#include "gm_sub/00925C90.inc"
};
u32 D_00925CA0[4] = {
#include "gm_sub/00925CA0.inc"
};
u32 D_00925CB0[4] = {
#include "gm_sub/00925CB0.inc"
};
u32 D_00925CC0[14] = {
#include "gm_sub/00925CC0.inc"
};
u32 D_00925CF8[3] = {
#include "gm_sub/00925CF8.inc"
};
u32 D_00925D08[3] = {
#include "gm_sub/00925D08.inc"
};
u32 D_00925D18[3] = {
#include "gm_sub/00925D18.inc"
};
u32 D_00925D28[3] = {
#include "gm_sub/00925D28.inc"
};
u32 D_00925D38[3] = {
#include "gm_sub/00925D38.inc"
};
u32 D_00925D48[3] = {
#include "gm_sub/00925D48.inc"
};
u32 D_00925D58[3] = {
#include "gm_sub/00925D58.inc"
};
u32 D_00925D68[3] = {
#include "gm_sub/00925D68.inc"
};
u32 D_00925D78[3] = {
#include "gm_sub/00925D78.inc"
};
u32 D_00925D88[3] = {
#include "gm_sub/00925D88.inc"
};
u32 D_00925D98[3] = {
#include "gm_sub/00925D98.inc"
};
u32 D_00925DA8[3] = {
#include "gm_sub/00925DA8.inc"
};
u32 D_00925DB8[3] = {
#include "gm_sub/00925DB8.inc"
};
u32 D_00925DC8[3] = {
#include "gm_sub/00925DC8.inc"
};
u32 D_00925DD8[3] = {
#include "gm_sub/00925DD8.inc"
};
u32 D_00925DE8[3] = {
#include "gm_sub/00925DE8.inc"
};
u32 D_00925DF8[3] = {
#include "gm_sub/00925DF8.inc"
};
u32 D_00925E08[3] = {
#include "gm_sub/00925E08.inc"
};
u32 D_00925E20[64] = {
#include "gm_sub/00925E20.inc"
};
u32 D_00925F20[4] = {
#include "gm_sub/00925F20.inc"
};
u32 D_00925F30[4] = {
#include "gm_sub/00925F30.inc"
};
u32 D_00925F40[4] = {
#include "gm_sub/00925F40.inc"
};
u32 D_00925F50[4] = {
#include "gm_sub/00925F50.inc"
};
u32 D_00925F60[4] = {
#include "gm_sub/00925F60.inc"
};
u32 D_00925F70[4] = {
#include "gm_sub/00925F70.inc"
};
u32 D_00925F80[4] = {
#include "gm_sub/00925F80.inc"
};
u32 D_00925F90[16] = {
#include "gm_sub/00925F90.inc"
};
u32 D_00925FD0[4] = {
#include "gm_sub/00925FD0.inc"
};
u32 D_00925FE0[4] = {
#include "gm_sub/00925FE0.inc"
};
u32 D_00925FF0[4] = {
#include "gm_sub/00925FF0.inc"
};
u32 D_00926000[4] = {
#include "gm_sub/00926000.inc"
};
u32 D_00926010[4] = {
#include "gm_sub/00926010.inc"
};
u32 D_00926020[4] = {
#include "gm_sub/00926020.inc"
};
u32 D_00926030[4] = {
#include "gm_sub/00926030.inc"
};
u32 D_00926040[4] = {
#include "gm_sub/00926040.inc"
};
u32 D_00926050[4] = {
#include "gm_sub/00926050.inc"
};
u32 D_00926060[18] = {
#include "gm_sub/00926060.inc"
};
u32 D_009260A8[2] = {
#include "gm_sub/009260A8.inc"
};
u32 D_009260B0[4] = {
#include "gm_sub/009260B0.inc"
};
u32 D_009260C0[4] = {
#include "gm_sub/009260C0.inc"
};
u32 D_009260D0[4] = {
#include "gm_sub/009260D0.inc"
};
u32 D_009260E0[4] = {
#include "gm_sub/009260E0.inc"
};
u32 D_009260F0[4] = {
#include "gm_sub/009260F0.inc"
};
u32 D_00926100[4] = {
#include "gm_sub/00926100.inc"
};
u32 D_00926110[4] = {
#include "gm_sub/00926110.inc"
};
u32 D_00926120[4] = {
#include "gm_sub/00926120.inc"
};
u32 D_00926130[4] = {
#include "gm_sub/00926130.inc"
};
u32 D_00926140[20] = {
#include "gm_sub/00926140.inc"
};
u32 D_00926190[336] = {
#include "gm_sub/00926190.inc"
};
u32 D_009266D0[16] = {
#include "gm_sub/009266D0.inc"
};
u32 D_00926710[12] = {
#include "gm_sub/00926710.inc"
};
u32 D_00926740[12] = {
#include "gm_sub/00926740.inc"
};
u32 D_00926770[16] = {
#include "gm_sub/00926770.inc"
};
u32 D_009267B0[12] = {
#include "gm_sub/009267B0.inc"
};
u32 D_009267E0[24] = {
#include "gm_sub/009267E0.inc"
};
u32 D_00926840[16] = {
#include "gm_sub/00926840.inc"
};
u32 D_00926880[12] = {
#include "gm_sub/00926880.inc"
};
u32 D_009268B0[12] = {
#include "gm_sub/009268B0.inc"
};
u32 D_009268E0[12] = {
#include "gm_sub/009268E0.inc"
};
u32 D_00926910[8] = {
#include "gm_sub/00926910.inc"
};
u32 D_00926930[12] = {
#include "gm_sub/00926930.inc"
};
u32 D_00926960[12] = {
#include "gm_sub/00926960.inc"
};
u32 D_00926990[12] = {
#include "gm_sub/00926990.inc"
};
u32 D_009269C0[12] = {
#include "gm_sub/009269C0.inc"
};
u32 D_009269F0[12] = {
#include "gm_sub/009269F0.inc"
};
u32 D_00926A20[12] = {
#include "gm_sub/00926A20.inc"
};
u32 D_00926A50[12] = {
#include "gm_sub/00926A50.inc"
};
u32 D_00926A80[12] = {
#include "gm_sub/00926A80.inc"
};
u32 D_00926AB0[12] = {
#include "gm_sub/00926AB0.inc"
};
u32 D_00926AE0[8] = {
#include "gm_sub/00926AE0.inc"
};
u32 D_00926B00[24] = {
#include "gm_sub/00926B00.inc"
};
u32 D_00926B60[46] = {
#include "gm_sub/00926B60.inc"
};
u32 D_00926C18[3] = {
#include "gm_sub/00926C18.inc"
};
u32 D_00926C28[3] = {
#include "gm_sub/00926C28.inc"
};
u32 D_00926C38[3] = {
#include "gm_sub/00926C38.inc"
};
u32 D_00926C48[3] = {
#include "gm_sub/00926C48.inc"
};
u32 D_00926C58[3] = {
#include "gm_sub/00926C58.inc"
};
u32 D_00926C68[3] = {
#include "gm_sub/00926C68.inc"
};
u32 D_00926C78[3] = {
#include "gm_sub/00926C78.inc"
};
u32 D_00926C88[3] = {
#include "gm_sub/00926C88.inc"
};
u32 D_00926C98[3] = {
#include "gm_sub/00926C98.inc"
};
u32 D_00926CA8[3] = {
#include "gm_sub/00926CA8.inc"
};
u32 D_00926CB8[3] = {
#include "gm_sub/00926CB8.inc"
};
u32 D_00926CC8[3] = {
#include "gm_sub/00926CC8.inc"
};
u32 D_00926CD8[3] = {
#include "gm_sub/00926CD8.inc"
};
u32 D_00926CE8[3] = {
#include "gm_sub/00926CE8.inc"
};
u32 D_00926CF8[3] = {
#include "gm_sub/00926CF8.inc"
};
u32 D_00926D08[3] = {
#include "gm_sub/00926D08.inc"
};
u32 D_00926D18[3] = {
#include "gm_sub/00926D18.inc"
};
u32 D_00926D28[3] = {
#include "gm_sub/00926D28.inc"
};
u32 D_00926D38[3] = {
#include "gm_sub/00926D38.inc"
};
u32 D_00926D48[3] = {
#include "gm_sub/00926D48.inc"
};
u32 D_00926D58[3] = {
#include "gm_sub/00926D58.inc"
};
u32 D_00926D68[3] = {
#include "gm_sub/00926D68.inc"
};
u32 D_00926D80[46] = {
#include "gm_sub/00926D80.inc"
};
u32 D_00926E38[3] = {
#include "gm_sub/00926E38.inc"
};
u32 D_00926E48[3] = {
#include "gm_sub/00926E48.inc"
};
u32 D_00926E58[3] = {
#include "gm_sub/00926E58.inc"
};
u32 D_00926E68[3] = {
#include "gm_sub/00926E68.inc"
};
u32 D_00926E78[3] = {
#include "gm_sub/00926E78.inc"
};
u32 D_00926E88[3] = {
#include "gm_sub/00926E88.inc"
};
u32 D_00926EA0[14] = {
#include "gm_sub/00926EA0.inc"
};
u32 D_00926ED8[3] = {
#include "gm_sub/00926ED8.inc"
};
u32 D_00926EE8[3] = {
#include "gm_sub/00926EE8.inc"
};
u32 D_00926EF8[3] = {
#include "gm_sub/00926EF8.inc"
};
u32 D_00926F08[3] = {
#include "gm_sub/00926F08.inc"
};
u32 D_00926F18[3] = {
#include "gm_sub/00926F18.inc"
};
u32 D_00926F28[3] = {
#include "gm_sub/00926F28.inc"
};
u32 D_00926F40[36] = {
#include "gm_sub/00926F40.inc"
};
u32 D_00926FD0[4] = {
#include "gm_sub/00926FD0.inc"
};
u32 D_00926FE0[4] = {
#include "gm_sub/00926FE0.inc"
};
u32 D_00926FF0[4] = {
#include "gm_sub/00926FF0.inc"
};
u32 D_00927000[4] = {
#include "gm_sub/00927000.inc"
};
u32 D_00927010[4] = {
#include "gm_sub/00927010.inc"
};
u32 D_00927020[4] = {
#include "gm_sub/00927020.inc"
};
u32 D_00927030[4] = {
#include "gm_sub/00927030.inc"
};
u32 D_00927040[4] = {
#include "gm_sub/00927040.inc"
};
u32 D_00927050[18] = {
#include "gm_sub/00927050.inc"
};
u32 D_00927098[2] = {
#include "gm_sub/00927098.inc"
};
u32 D_009270A0[4] = {
#include "gm_sub/009270A0.inc"
};
u32 D_009270B0[4] = {
#include "gm_sub/009270B0.inc"
};
u32 D_009270C0[4] = {
#include "gm_sub/009270C0.inc"
};
u32 D_009270D0[4] = {
#include "gm_sub/009270D0.inc"
};
u32 D_009270E0[4] = {
#include "gm_sub/009270E0.inc"
};
u32 D_009270F0[4] = {
#include "gm_sub/009270F0.inc"
};
u32 D_00927100[14] = {
#include "gm_sub/00927100.inc"
};
u32 D_00927138[2] = {
#include "gm_sub/00927138.inc"
};
u32 D_00927140[4] = {
#include "gm_sub/00927140.inc"
};
u32 D_00927150[4] = {
#include "gm_sub/00927150.inc"
};
u32 D_00927160[4] = {
#include "gm_sub/00927160.inc"
};
u32 D_00927170[4] = {
#include "gm_sub/00927170.inc"
};
u32 D_00927180[4] = {
#include "gm_sub/00927180.inc"
};
u32 D_00927190[4] = {
#include "gm_sub/00927190.inc"
};
u32 D_009271A0[4] = {
#include "gm_sub/009271A0.inc"
};
u32 D_009271B0[16] = {
#include "gm_sub/009271B0.inc"
};
u32 D_009271F0[4] = {
#include "gm_sub/009271F0.inc"
};
u32 D_00927200[4] = {
#include "gm_sub/00927200.inc"
};
u32 D_00927210[4] = {
#include "gm_sub/00927210.inc"
};
u32 D_00927220[6] = {
#include "gm_sub/00927220.inc"
};
u32 D_00927238[2] = {
#include "gm_sub/00927238.inc"
};
u32 D_00927240[4] = {
#include "gm_sub/00927240.inc"
};
u32 D_00927250[4] = {
#include "gm_sub/00927250.inc"
};
u32 D_00927260[4] = {
#include "gm_sub/00927260.inc"
};
u32 D_00927270[4] = {
#include "gm_sub/00927270.inc"
};
u32 D_00927280[4] = {
#include "gm_sub/00927280.inc"
};
u32 D_00927290[4] = {
#include "gm_sub/00927290.inc"
};
u32 D_009272A0[44] = {
#include "gm_sub/009272A0.inc"
};
u32 D_00927350[20] = {
#include "gm_sub/00927350.inc"
};
u32 D_009273A0[6] = {
#include "gm_sub/009273A0.inc"
};
u32 D_009273B8[2] = {
#include "gm_sub/009273B8.inc"
};
u32 D_009273C0[2] = {
#include "gm_sub/009273C0.inc"
};
u32 D_009273C8[2] = {
#include "gm_sub/009273C8.inc"
};
u32 D_009273D0[4636] = {
#include "gm_sub/009273D0.inc"
};
u32 D_0092BC40[8] = {
#include "gm_sub/0092BC40.inc"
};
u32 D_0092BC60[8] = {
#include "gm_sub/0092BC60.inc"
};
u32 D_0092BC80[4] = {
#include "gm_sub/0092BC80.inc"
};
u32 D_0092BC90[4] = {
#include "gm_sub/0092BC90.inc"
};
u32 D_0092BCA0[8] = {
#include "gm_sub/0092BCA0.inc"
};
u32 D_0092BCC0[12] = {
#include "gm_sub/0092BCC0.inc"
};
u32 D_0092BCF0[16] = {
#include "gm_sub/0092BCF0.inc"
};
u32 D_0092BD30[8] = {
#include "gm_sub/0092BD30.inc"
};
u32 D_0092BD50[12] = {
#include "gm_sub/0092BD50.inc"
};
u32 D_0092BD80[8] = {
#include "gm_sub/0092BD80.inc"
};
u32 D_0092BDA0[40] = {
#include "gm_sub/0092BDA0.inc"
};
u32 D_0092BE40[8] = {
#include "gm_sub/0092BE40.inc"
};
u32 D_0092BE60[10] = {
#include "gm_sub/0092BE60.inc"
};
u32 D_0092BE88[3] = {
#include "gm_sub/0092BE88.inc"
};
u32 D_0092BEA0[8] = {
#include "gm_sub/0092BEA0.inc"
};
u32 D_0092BEC0[12] = {
#include "gm_sub/0092BEC0.inc"
};
u32 D_0092BEF0[12] = {
#include "gm_sub/0092BEF0.inc"
};
u32 D_0092BF20[6] = {
#include "gm_sub/0092BF20.inc"
};
u32 D_0092BF38[3] = {
#include "gm_sub/0092BF38.inc"
};
u32 D_0092BF48[3] = {
#include "gm_sub/0092BF48.inc"
};
u32 D_0092BF60[8] = {
#include "gm_sub/0092BF60.inc"
};
u32 D_0092BF80[12] = {
#include "gm_sub/0092BF80.inc"
};
u32 D_0092BFB0[6] = {
#include "gm_sub/0092BFB0.inc"
};
u32 D_0092BFC8[3] = {
#include "gm_sub/0092BFC8.inc"
};
u32 D_0092BFD8[3] = {
#include "gm_sub/0092BFD8.inc"
};
u32 D_0092BFF0[8] = {
#include "gm_sub/0092BFF0.inc"
};
u32 D_0092C010[12] = {
#include "gm_sub/0092C010.inc"
};
u32 D_0092C040[8] = {
#include "gm_sub/0092C040.inc"
};
u32 D_0092C060[8] = {
#include "gm_sub/0092C060.inc"
};
u32 D_0092C080[12] = {
#include "gm_sub/0092C080.inc"
};
u32 D_0092C0B0[8] = {
#include "gm_sub/0092C0B0.inc"
};
u32 D_0092C0D0[8] = {
#include "gm_sub/0092C0D0.inc"
};
u32 D_0092C0F0[8] = {
#include "gm_sub/0092C0F0.inc"
};
u32 D_0092C110[4] = {
#include "gm_sub/0092C110.inc"
};
u32 D_0092C120[12] = {
#include "gm_sub/0092C120.inc"
};
u32 D_0092C150[12] = {
#include "gm_sub/0092C150.inc"
};
u32 D_0092C180[16] = {
#include "gm_sub/0092C180.inc"
};
u32 D_0092C1C0[8] = {
#include "gm_sub/0092C1C0.inc"
};
u32 D_0092C1E0[6] = {
#include "gm_sub/0092C1E0.inc"
};
u32 D_0092C1F8[2] = {
#include "gm_sub/0092C1F8.inc"
};
u32 D_0092C200[8] = {
#include "gm_sub/0092C200.inc"
};
u32 D_0092C220[6] = {
#include "gm_sub/0092C220.inc"
};
u32 D_0092C238[2] = {
#include "gm_sub/0092C238.inc"
};
u32 D_0092C240[4] = {
#include "gm_sub/0092C240.inc"
};
u32 D_0092C250[4] = {
#include "gm_sub/0092C250.inc"
};
u32 D_0092C260[4] = {
#include "gm_sub/0092C260.inc"
};
u32 D_0092C270[6] = {
#include "gm_sub/0092C270.inc"
};
u32 D_0092C288[3] = {
#include "gm_sub/0092C288.inc"
};
u32 D_0092C298[3] = {
#include "gm_sub/0092C298.inc"
};
u32 D_0092C2B0[12] = {
#include "gm_sub/0092C2B0.inc"
};
u32 D_0092C2E0[12] = {
#include "gm_sub/0092C2E0.inc"
};
u32 D_0092C310[10] = {
#include "gm_sub/0092C310.inc"
};
u32 D_0092C338[3] = {
#include "gm_sub/0092C338.inc"
};
u32 D_0092C350[6] = {
#include "gm_sub/0092C350.inc"
};
u32 D_0092C368[3] = {
#include "gm_sub/0092C368.inc"
};
u32 D_0092C380[8] = {
#include "gm_sub/0092C380.inc"
};
u32 D_0092C3A0[12] = {
#include "gm_sub/0092C3A0.inc"
};
u32 D_0092C3D0[8] = {
#include "gm_sub/0092C3D0.inc"
};
u32 D_0092C3F0[8] = {
#include "gm_sub/0092C3F0.inc"
};
u32 D_0092C410[28] = {
#include "gm_sub/0092C410.inc"
};
u32 D_0092C480[8] = {
#include "gm_sub/0092C480.inc"
};
u32 D_0092C4A0[28] = {
#include "gm_sub/0092C4A0.inc"
};
u32 D_0092C510[16] = {
#include "gm_sub/0092C510.inc"
};
u32 D_0092C550[6] = {
#include "gm_sub/0092C550.inc"
};
u32 D_0092C568[3] = {
#include "gm_sub/0092C568.inc"
};
u32 D_0092C578[3] = {
#include "gm_sub/0092C578.inc"
};
u32 D_0092C590[24] = {
#include "gm_sub/0092C590.inc"
};
u32 D_0092C5F0[8] = {
#include "gm_sub/0092C5F0.inc"
};
u32 D_0092C610[10] = {
#include "gm_sub/0092C610.inc"
};
u32 D_0092C638[3] = {
#include "gm_sub/0092C638.inc"
};
u32 D_0092C650[20] = {
#include "gm_sub/0092C650.inc"
};
u32 D_0092C6A0[6] = {
#include "gm_sub/0092C6A0.inc"
};
u32 D_0092C6B8[3] = {
#include "gm_sub/0092C6B8.inc"
};
u32 D_0092C6D0[8] = {
#include "gm_sub/0092C6D0.inc"
};
u32 D_0092C6F0[12] = {
#include "gm_sub/0092C6F0.inc"
};
u32 D_0092C720[8] = {
#include "gm_sub/0092C720.inc"
};
u32 D_0092C740[8] = {
#include "gm_sub/0092C740.inc"
};
u32 D_0092C760[8] = {
#include "gm_sub/0092C760.inc"
};
u32 D_0092C780[8] = {
#include "gm_sub/0092C780.inc"
};
u32 D_0092C7A0[4] = {
#include "gm_sub/0092C7A0.inc"
};
u32 D_0092C7B0[4] = {
#include "gm_sub/0092C7B0.inc"
};
u32 D_0092C7C0[8] = {
#include "gm_sub/0092C7C0.inc"
};
u32 D_0092C7E0[12] = {
#include "gm_sub/0092C7E0.inc"
};
u32 D_0092C810[8] = {
#include "gm_sub/0092C810.inc"
};
u32 D_0092C830[4] = {
#include "gm_sub/0092C830.inc"
};
u32 D_0092C840[8] = {
#include "gm_sub/0092C840.inc"
};
u32 D_0092C860[8] = {
#include "gm_sub/0092C860.inc"
};
u32 D_0092C880[8] = {
#include "gm_sub/0092C880.inc"
};
u32 D_0092C8A0[4] = {
#include "gm_sub/0092C8A0.inc"
};
u32 D_0092C8B0[8] = {
#include "gm_sub/0092C8B0.inc"
};
u32 D_0092C8D0[8] = {
#include "gm_sub/0092C8D0.inc"
};
u32 D_0092C8F0[4] = {
#include "gm_sub/0092C8F0.inc"
};
u32 D_0092C900[4] = {
#include "gm_sub/0092C900.inc"
};
u32 D_0092C910[8] = {
#include "gm_sub/0092C910.inc"
};
u32 D_0092C930[10] = {
#include "gm_sub/0092C930.inc"
};
u32 D_0092C958[3] = {
#include "gm_sub/0092C958.inc"
};
u32 D_0092C970[12] = {
#include "gm_sub/0092C970.inc"
};
u32 D_0092C9A0[8] = {
#include "gm_sub/0092C9A0.inc"
};
u32 D_0092C9C0[8] = {
#include "gm_sub/0092C9C0.inc"
};
u32 D_0092C9E0[6] = {
#include "gm_sub/0092C9E0.inc"
};
u32 D_0092C9F8[3] = {
#include "gm_sub/0092C9F8.inc"
};
u32 D_0092CA10[8] = {
#include "gm_sub/0092CA10.inc"
};
u32 D_0092CA30[8] = {
#include "gm_sub/0092CA30.inc"
};
u32 D_0092CA50[4] = {
#include "gm_sub/0092CA50.inc"
};
u32 D_0092CA60[12] = {
#include "gm_sub/0092CA60.inc"
};
u32 D_0092CA90[8] = {
#include "gm_sub/0092CA90.inc"
};
u32 D_0092CAB0[6] = {
#include "gm_sub/0092CAB0.inc"
};
u32 D_0092CAC8[3] = {
#include "gm_sub/0092CAC8.inc"
};
u32 D_0092CAE0[8] = {
#include "gm_sub/0092CAE0.inc"
};
u32 D_0092CB00[12] = {
#include "gm_sub/0092CB00.inc"
};
u32 D_0092CB30[6] = {
#include "gm_sub/0092CB30.inc"
};
u32 D_0092CB48[3] = {
#include "gm_sub/0092CB48.inc"
};
u32 D_0092CB58[2] = {
#include "gm_sub/0092CB58.inc"
};
u32 D_0092CB60[8] = {
#include "gm_sub/0092CB60.inc"
};
u32 D_0092CB80[8] = {
#include "gm_sub/0092CB80.inc"
};
u32 D_0092CBA0[8] = {
#include "gm_sub/0092CBA0.inc"
};
u32 D_0092CBC0[8] = {
#include "gm_sub/0092CBC0.inc"
};
u32 D_0092CBE0[8] = {
#include "gm_sub/0092CBE0.inc"
};
u32 D_0092CC00[12] = {
#include "gm_sub/0092CC00.inc"
};
u32 D_0092CC30[8] = {
#include "gm_sub/0092CC30.inc"
};
u32 D_0092CC50[8] = {
#include "gm_sub/0092CC50.inc"
};
u32 D_0092CC70[8] = {
#include "gm_sub/0092CC70.inc"
};
u32 D_0092CC90[4] = {
#include "gm_sub/0092CC90.inc"
};
u32 D_0092CCA0[8] = {
#include "gm_sub/0092CCA0.inc"
};
u32 D_0092CCC0[4] = {
#include "gm_sub/0092CCC0.inc"
};
u32 D_0092CCD0[8] = {
#include "gm_sub/0092CCD0.inc"
};
u32 D_0092CCF0[4] = {
#include "gm_sub/0092CCF0.inc"
};
u32 D_0092CD00[6] = {
#include "gm_sub/0092CD00.inc"
};
u32 D_0092CD18[3] = {
#include "gm_sub/0092CD18.inc"
};
u32 D_0092CD28[2] = {
#include "gm_sub/0092CD28.inc"
};
u32 D_0092CD30[8] = {
#include "gm_sub/0092CD30.inc"
};
u32 D_0092CD50[12] = {
#include "gm_sub/0092CD50.inc"
};
u32 D_0092CD80[8] = {
#include "gm_sub/0092CD80.inc"
};
u32 D_0092CDA0[12] = {
#include "gm_sub/0092CDA0.inc"
};
u32 D_0092CDD0[10] = {
#include "gm_sub/0092CDD0.inc"
};
u32 D_0092CDF8[3] = {
#include "gm_sub/0092CDF8.inc"
};
u32 D_0092CE10[6] = {
#include "gm_sub/0092CE10.inc"
};
u32 D_0092CE28[3] = {
#include "gm_sub/0092CE28.inc"
};
u32 D_0092CE38[2] = {
#include "gm_sub/0092CE38.inc"
};
u32 D_0092CE40[12] = {
#include "gm_sub/0092CE40.inc"
};
u32 D_0092CE70[12] = {
#include "gm_sub/0092CE70.inc"
};
u32 D_0092CEA0[8] = {
#include "gm_sub/0092CEA0.inc"
};
u32 D_0092CEC0[6] = {
#include "gm_sub/0092CEC0.inc"
};
u32 D_0092CED8[3] = {
#include "gm_sub/0092CED8.inc"
};
u32 D_0092CEF0[6] = {
#include "gm_sub/0092CEF0.inc"
};
u32 D_0092CF08[3] = {
#include "gm_sub/0092CF08.inc"
};
u32 D_0092CF20[8] = {
#include "gm_sub/0092CF20.inc"
};
u32 D_0092CF40[8] = {
#include "gm_sub/0092CF40.inc"
};
u32 D_0092CF60[4] = {
#include "gm_sub/0092CF60.inc"
};
u32 D_0092CF70[8] = {
#include "gm_sub/0092CF70.inc"
};
u32 D_0092CF90[8] = {
#include "gm_sub/0092CF90.inc"
};
u32 D_0092CFB0[12] = {
#include "gm_sub/0092CFB0.inc"
};
u32 D_0092CFE0[20] = {
#include "gm_sub/0092CFE0.inc"
};
u32 D_0092D030[8] = {
#include "gm_sub/0092D030.inc"
};
u32 D_0092D050[24] = {
#include "gm_sub/0092D050.inc"
};
u32 D_0092D0B0[8] = {
#include "gm_sub/0092D0B0.inc"
};
u32 D_0092D0D0[8] = {
#include "gm_sub/0092D0D0.inc"
};
u32 D_0092D0F0[16] = {
#include "gm_sub/0092D0F0.inc"
};
u32 D_0092D130[8] = {
#include "gm_sub/0092D130.inc"
};
u32 D_0092D150[4] = {
#include "gm_sub/0092D150.inc"
};
u32 D_0092D160[4] = {
#include "gm_sub/0092D160.inc"
};
u32 D_0092D170[24] = {
#include "gm_sub/0092D170.inc"
};
u32 D_0092D1D0[8] = {
#include "gm_sub/0092D1D0.inc"
};
u32 D_0092D1F0[8] = {
#include "gm_sub/0092D1F0.inc"
};
u32 D_0092D210[6] = {
#include "gm_sub/0092D210.inc"
};
u32 D_0092D228[3] = {
#include "gm_sub/0092D228.inc"
};
u32 D_0092D240[8] = {
#include "gm_sub/0092D240.inc"
};
u32 D_0092D260[12] = {
#include "gm_sub/0092D260.inc"
};
u32 D_0092D290[8] = {
#include "gm_sub/0092D290.inc"
};
u32 D_0092D2B0[8] = {
#include "gm_sub/0092D2B0.inc"
};
u32 D_0092D2D0[4] = {
#include "gm_sub/0092D2D0.inc"
};
u32 D_0092D2E0[8] = {
#include "gm_sub/0092D2E0.inc"
};
u32 D_0092D300[12] = {
#include "gm_sub/0092D300.inc"
};
u32 D_0092D330[8] = {
#include "gm_sub/0092D330.inc"
};
u32 D_0092D350[8] = {
#include "gm_sub/0092D350.inc"
};
u32 D_0092D370[24] = {
#include "gm_sub/0092D370.inc"
};
u32 D_0092D3D0[12] = {
#include "gm_sub/0092D3D0.inc"
};
u32 D_0092D400[20] = {
#include "gm_sub/0092D400.inc"
};
u32 D_0092D450[12] = {
#include "gm_sub/0092D450.inc"
};
u32 D_0092D480[8] = {
#include "gm_sub/0092D480.inc"
};
u32 D_0092D4A0[8] = {
#include "gm_sub/0092D4A0.inc"
};
u32 D_0092D4C0[12] = {
#include "gm_sub/0092D4C0.inc"
};
u32 D_0092D4F0[10] = {
#include "gm_sub/0092D4F0.inc"
};
u32 D_0092D518[3] = {
#include "gm_sub/0092D518.inc"
};
u32 D_0092D530[16] = {
#include "gm_sub/0092D530.inc"
};
u32 D_0092D570[8] = {
#include "gm_sub/0092D570.inc"
};
u32 D_0092D590[8] = {
#include "gm_sub/0092D590.inc"
};
u32 D_0092D5B0[8] = {
#include "gm_sub/0092D5B0.inc"
};
u32 D_0092D5D0[12] = {
#include "gm_sub/0092D5D0.inc"
};
u32 D_0092D600[8] = {
#include "gm_sub/0092D600.inc"
};
u32 D_0092D620[4] = {
#include "gm_sub/0092D620.inc"
};
u32 D_0092D630[8] = {
#include "gm_sub/0092D630.inc"
};
u32 D_0092D650[8] = {
#include "gm_sub/0092D650.inc"
};
u32 D_0092D670[2] = {
#include "gm_sub/0092D670.inc"
};
u32 D_0092D678[2] = {
#include "gm_sub/0092D678.inc"
};
u32 D_0092D680[4] = {
#include "gm_sub/0092D680.inc"
};
u32 D_0092D690[8] = {
#include "gm_sub/0092D690.inc"
};
u32 D_0092D6B0[8] = {
#include "gm_sub/0092D6B0.inc"
};
u32 D_0092D6D0[8] = {
#include "gm_sub/0092D6D0.inc"
};
u32 D_0092D6F0[4] = {
#include "gm_sub/0092D6F0.inc"
};
u32 D_0092D700[8] = {
#include "gm_sub/0092D700.inc"
};
u32 D_0092D720[12] = {
#include "gm_sub/0092D720.inc"
};
u32 D_0092D750[8] = {
#include "gm_sub/0092D750.inc"
};
u32 D_0092D770[16] = {
#include "gm_sub/0092D770.inc"
};
u32 D_0092D7B0[8] = {
#include "gm_sub/0092D7B0.inc"
};
u32 D_0092D7D0[8] = {
#include "gm_sub/0092D7D0.inc"
};
u32 D_0092D7F0[8] = {
#include "gm_sub/0092D7F0.inc"
};
u32 D_0092D810[4] = {
#include "gm_sub/0092D810.inc"
};
u32 D_0092D820[6] = {
#include "gm_sub/0092D820.inc"
};
u32 D_0092D838[2] = {
#include "gm_sub/0092D838.inc"
};
u32 D_0092D840[6] = {
#include "gm_sub/0092D840.inc"
};
u32 D_0092D858[2] = {
#include "gm_sub/0092D858.inc"
};
u32 D_0092D860[6] = {
#include "gm_sub/0092D860.inc"
};
u32 D_0092D878[3] = {
#include "gm_sub/0092D878.inc"
};
u32 D_0092D888[2] = {
#include "gm_sub/0092D888.inc"
};
u32 D_0092D890[8] = {
#include "gm_sub/0092D890.inc"
};
u32 D_0092D8B0[6] = {
#include "gm_sub/0092D8B0.inc"
};
u32 D_0092D8C8[2] = {
#include "gm_sub/0092D8C8.inc"
};
u32 D_0092D8D0[8] = {
#include "gm_sub/0092D8D0.inc"
};
u32 D_0092D8F0[6] = {
#include "gm_sub/0092D8F0.inc"
};
u32 D_0092D908[2] = {
#include "gm_sub/0092D908.inc"
};
u32 D_0092D910[4] = {
#include "gm_sub/0092D910.inc"
};
u32 D_0092D920[4] = {
#include "gm_sub/0092D920.inc"
};
u32 D_0092D930[4] = {
#include "gm_sub/0092D930.inc"
};
u32 D_0092D940[6] = {
#include "gm_sub/0092D940.inc"
};
u32 D_0092D958[3] = {
#include "gm_sub/0092D958.inc"
};
u32 D_0092D968[2] = {
#include "gm_sub/0092D968.inc"
};
u32 D_0092D970[6] = {
#include "gm_sub/0092D970.inc"
};
u32 D_0092D988[2] = {
#include "gm_sub/0092D988.inc"
};
u32 D_0092D990[6] = {
#include "gm_sub/0092D990.inc"
};
u32 D_0092D9A8[2] = {
#include "gm_sub/0092D9A8.inc"
};
u32 D_0092D9B0[10] = {
#include "gm_sub/0092D9B0.inc"
};
u32 D_0092D9D8[2] = {
#include "gm_sub/0092D9D8.inc"
};
u32 D_0092D9E0[6] = {
#include "gm_sub/0092D9E0.inc"
};
u32 D_0092D9F8[3] = {
#include "gm_sub/0092D9F8.inc"
};
u32 D_0092DA10[8] = {
#include "gm_sub/0092DA10.inc"
};
u32 D_0092DA30[6] = {
#include "gm_sub/0092DA30.inc"
};
u32 D_0092DA48[2] = {
#include "gm_sub/0092DA48.inc"
};
u32 D_0092DA50[12] = {
#include "gm_sub/0092DA50.inc"
};
u32 D_0092DA80[12] = {
#include "gm_sub/0092DA80.inc"
};
u32 D_0092DAB0[20] = {
#include "gm_sub/0092DAB0.inc"
};
u32 D_0092DB00[8] = {
#include "gm_sub/0092DB00.inc"
};
u32 D_0092DB20[6] = {
#include "gm_sub/0092DB20.inc"
};
u32 D_0092DB38[3] = {
#include "gm_sub/0092DB38.inc"
};
u32 D_0092DB48[2] = {
#include "gm_sub/0092DB48.inc"
};
u32 D_0092DB50[4] = {
#include "gm_sub/0092DB50.inc"
};
u32 D_0092DB60[4] = {
#include "gm_sub/0092DB60.inc"
};
u32 D_0092DB70[4] = {
#include "gm_sub/0092DB70.inc"
};
u32 D_0092DB80[10] = {
#include "gm_sub/0092DB80.inc"
};
u32 D_0092DBA8[2] = {
#include "gm_sub/0092DBA8.inc"
};
u32 D_0092DBB0[4] = {
#include "gm_sub/0092DBB0.inc"
};
u32 D_0092DBC0[4] = {
#include "gm_sub/0092DBC0.inc"
};
u32 D_0092DBD0[4] = {
#include "gm_sub/0092DBD0.inc"
};
u32 D_0092DBE0[6] = {
#include "gm_sub/0092DBE0.inc"
};
u32 D_0092DBF8[3] = {
#include "gm_sub/0092DBF8.inc"
};
u32 D_0092DC08[2] = {
#include "gm_sub/0092DC08.inc"
};
u32 D_0092DC10[4] = {
#include "gm_sub/0092DC10.inc"
};
u32 D_0092DC20[4] = {
#include "gm_sub/0092DC20.inc"
};
u32 D_0092DC30[4] = {
#include "gm_sub/0092DC30.inc"
};
u32 D_0092DC40[10] = {
#include "gm_sub/0092DC40.inc"
};
u32 D_0092DC68[2] = {
#include "gm_sub/0092DC68.inc"
};
u32 D_0092DC70[10] = {
#include "gm_sub/0092DC70.inc"
};
u32 D_0092DC98[2] = {
#include "gm_sub/0092DC98.inc"
};
u32 D_0092DCA0[8] = {
#include "gm_sub/0092DCA0.inc"
};
u32 D_0092DCC0[6] = {
#include "gm_sub/0092DCC0.inc"
};
u32 D_0092DCD8[3] = {
#include "gm_sub/0092DCD8.inc"
};
u32 D_0092DCF0[16] = {
#include "gm_sub/0092DCF0.inc"
};
u32 D_0092DD30[8] = {
#include "gm_sub/0092DD30.inc"
};
u32 D_0092DD50[10] = {
#include "gm_sub/0092DD50.inc"
};
u32 D_0092DD78[2] = {
#include "gm_sub/0092DD78.inc"
};
u32 D_0092DD80[4] = {
#include "gm_sub/0092DD80.inc"
};
u32 D_0092DD90[384] = {
#include "gm_sub/0092DD90.inc"
};
u32 D_0092E390[8] = {
#include "gm_sub/0092E390.inc"
};
u32 D_0092E3B0[4] = {
#include "gm_sub/0092E3B0.inc"
};
u32 D_0092E3C0[4] = {
#include "gm_sub/0092E3C0.inc"
};
u32 D_0092E3D0[4] = {
#include "gm_sub/0092E3D0.inc"
};
u32 D_0092E3E0[4] = {
#include "gm_sub/0092E3E0.inc"
};
u32 D_0092E3F0[4] = {
#include "gm_sub/0092E3F0.inc"
};
u32 D_0092E400[8] = {
#include "gm_sub/0092E400.inc"
};
u32 D_0092E420[8] = {
#include "gm_sub/0092E420.inc"
};
u32 D_0092E440[4] = {
#include "gm_sub/0092E440.inc"
};
u32 D_0092E450[8] = {
#include "gm_sub/0092E450.inc"
};
u32 D_0092E470[4] = {
#include "gm_sub/0092E470.inc"
};
u32 D_0092E480[4] = {
#include "gm_sub/0092E480.inc"
};
u32 D_0092E490[8] = {
#include "gm_sub/0092E490.inc"
};
u32 D_0092E4B0[4] = {
#include "gm_sub/0092E4B0.inc"
};
u32 D_0092E4C0[4] = {
#include "gm_sub/0092E4C0.inc"
};
u32 D_0092E4D0[4] = {
#include "gm_sub/0092E4D0.inc"
};
u32 D_0092E4E0[8] = {
#include "gm_sub/0092E4E0.inc"
};
u32 D_0092E500[4] = {
#include "gm_sub/0092E500.inc"
};
u32 D_0092E510[4] = {
#include "gm_sub/0092E510.inc"
};
u32 D_0092E520[4] = {
#include "gm_sub/0092E520.inc"
};
u32 D_0092E530[8] = {
#include "gm_sub/0092E530.inc"
};
u32 D_0092E550[4] = {
#include "gm_sub/0092E550.inc"
};
u32 D_0092E560[4] = {
#include "gm_sub/0092E560.inc"
};
u32 D_0092E570[4] = {
#include "gm_sub/0092E570.inc"
};
u32 D_0092E580[4] = {
#include "gm_sub/0092E580.inc"
};
u32 D_0092E590[4] = {
#include "gm_sub/0092E590.inc"
};
u32 D_0092E5A0[4] = {
#include "gm_sub/0092E5A0.inc"
};
u32 D_0092E5B0[4] = {
#include "gm_sub/0092E5B0.inc"
};
u32 D_0092E5C0[8] = {
#include "gm_sub/0092E5C0.inc"
};
u32 D_0092E5E0[4] = {
#include "gm_sub/0092E5E0.inc"
};
u32 D_0092E5F0[4] = {
#include "gm_sub/0092E5F0.inc"
};
u32 D_0092E600[8] = {
#include "gm_sub/0092E600.inc"
};
u32 D_0092E620[8] = {
#include "gm_sub/0092E620.inc"
};
u32 D_0092E640[8] = {
#include "gm_sub/0092E640.inc"
};
u32 D_0092E660[8] = {
#include "gm_sub/0092E660.inc"
};
u32 D_0092E680[8] = {
#include "gm_sub/0092E680.inc"
};
u32 D_0092E6A0[4] = {
#include "gm_sub/0092E6A0.inc"
};
u32 D_0092E6B0[4] = {
#include "gm_sub/0092E6B0.inc"
};
u32 D_0092E6C0[8] = {
#include "gm_sub/0092E6C0.inc"
};
u32 D_0092E6E0[4] = {
#include "gm_sub/0092E6E0.inc"
};
u32 D_0092E6F0[8] = {
#include "gm_sub/0092E6F0.inc"
};
u32 D_0092E710[4] = {
#include "gm_sub/0092E710.inc"
};
u32 D_0092E720[4] = {
#include "gm_sub/0092E720.inc"
};
u32 D_0092E730[4] = {
#include "gm_sub/0092E730.inc"
};
u32 D_0092E740[8] = {
#include "gm_sub/0092E740.inc"
};
u32 D_0092E760[4] = {
#include "gm_sub/0092E760.inc"
};
u32 D_0092E770[8] = {
#include "gm_sub/0092E770.inc"
};
u32 D_0092E790[4] = {
#include "gm_sub/0092E790.inc"
};
u32 D_0092E7A0[8] = {
#include "gm_sub/0092E7A0.inc"
};
u32 D_0092E7C0[8] = {
#include "gm_sub/0092E7C0.inc"
};
u32 D_0092E7E0[4] = {
#include "gm_sub/0092E7E0.inc"
};
u32 D_0092E7F0[4] = {
#include "gm_sub/0092E7F0.inc"
};
u32 D_0092E800[4] = {
#include "gm_sub/0092E800.inc"
};
u32 D_0092E810[8] = {
#include "gm_sub/0092E810.inc"
};
u32 D_0092E830[8] = {
#include "gm_sub/0092E830.inc"
};
u32 D_0092E850[4] = {
#include "gm_sub/0092E850.inc"
};
u32 D_0092E860[4] = {
#include "gm_sub/0092E860.inc"
};
u32 D_0092E870[4] = {
#include "gm_sub/0092E870.inc"
};
u32 D_0092E880[4] = {
#include "gm_sub/0092E880.inc"
};
u32 D_0092E890[4] = {
#include "gm_sub/0092E890.inc"
};
u32 D_0092E8A0[78] = {
#include "gm_sub/0092E8A0.inc"
};
u32 D_0092E9D8[2] = {
#include "gm_sub/0092E9D8.inc"
};
u32 D_0092E9E0[2] = {
#include "gm_sub/0092E9E0.inc"
};
u32 D_0092E9E8[2] = {
#include "gm_sub/0092E9E8.inc"
};
u32 D_0092E9F0[2] = {
#include "gm_sub/0092E9F0.inc"
};
u32 D_0092E9F8[2] = {
#include "gm_sub/0092E9F8.inc"
};
u32 D_0092EA00[2] = {
#include "gm_sub/0092EA00.inc"
};
u32 D_0092EA08[2] = {
#include "gm_sub/0092EA08.inc"
};
u32 D_0092EA10[2] = {
#include "gm_sub/0092EA10.inc"
};
u32 D_0092EA18[2] = {
#include "gm_sub/0092EA18.inc"
};
u32 D_0092EA20[2] = {
#include "gm_sub/0092EA20.inc"
};
u32 D_0092EA28[2] = {
#include "gm_sub/0092EA28.inc"
};
u32 D_0092EA30[2] = {
#include "gm_sub/0092EA30.inc"
};
u32 D_0092EA38[2] = {
#include "gm_sub/0092EA38.inc"
};
u32 D_0092EA40[2] = {
#include "gm_sub/0092EA40.inc"
};
u32 D_0092EA48[2] = {
#include "gm_sub/0092EA48.inc"
};
u32 D_0092EA50[2] = {
#include "gm_sub/0092EA50.inc"
};
u32 D_0092EA58[2] = {
#include "gm_sub/0092EA58.inc"
};
u32 D_0092EA60[2] = {
#include "gm_sub/0092EA60.inc"
};
u32 D_0092EA68[2] = {
#include "gm_sub/0092EA68.inc"
};
u32 D_0092EA70[2] = {
#include "gm_sub/0092EA70.inc"
};
u32 D_0092EA78[2] = {
#include "gm_sub/0092EA78.inc"
};
u32 D_0092EA80[2] = {
#include "gm_sub/0092EA80.inc"
};
u32 D_0092EA88[2] = {
#include "gm_sub/0092EA88.inc"
};
u32 D_0092EA90[2] = {
#include "gm_sub/0092EA90.inc"
};
u32 D_0092EA98[2] = {
#include "gm_sub/0092EA98.inc"
};
u32 D_0092EAA0[4] = {
#include "gm_sub/0092EAA0.inc"
};
u32 D_0092EAB0[6] = {
#include "gm_sub/0092EAB0.inc"
};
u32 D_0092EAC8[2] = {
#include "gm_sub/0092EAC8.inc"
};
u32 D_0092EAD0[2] = {
#include "gm_sub/0092EAD0.inc"
};
u32 D_0092EAD8[2] = {
#include "gm_sub/0092EAD8.inc"
};
u32 D_0092EAE0[2] = {
#include "gm_sub/0092EAE0.inc"
};
u32 D_0092EAE8[2] = {
#include "gm_sub/0092EAE8.inc"
};
u32 D_0092EAF0[2] = {
#include "gm_sub/0092EAF0.inc"
};
u32 D_0092EAF8[2] = {
#include "gm_sub/0092EAF8.inc"
};
u32 D_0092EB00[80] = {
#include "gm_sub/0092EB00.inc"
};
u32 D_0092EC40[8] = {
#include "gm_sub/0092EC40.inc"
};
u32 D_0092EC60[4] = {
#include "gm_sub/0092EC60.inc"
};
u32 D_0092EC70[4] = {
#include "gm_sub/0092EC70.inc"
};
u32 D_0092EC80[2] = {
#include "gm_sub/0092EC80.inc"
};
u32 D_0092EC88[2] = {
#include "gm_sub/0092EC88.inc"
};
u32 D_0092EC90[2] = {
#include "gm_sub/0092EC90.inc"
};
u32 D_0092EC98[2] = {
#include "gm_sub/0092EC98.inc"
};
u32 D_0092ECA0[4] = {
#include "gm_sub/0092ECA0.inc"
};
u32 D_0092ECB0[8] = {
#include "gm_sub/0092ECB0.inc"
};
u32 D_0092ECD0[4] = {
#include "gm_sub/0092ECD0.inc"
};
u32 D_0092ECE0[4] = {
#include "gm_sub/0092ECE0.inc"
};
u32 D_0092ECF0[12] = {
#include "gm_sub/0092ECF0.inc"
};
u32 D_0092ED20[20] = {
#include "gm_sub/0092ED20.inc"
};
u32 D_0092ED70[12] = {
#include "gm_sub/0092ED70.inc"
};
u32 D_0092EDA0[16] = {
#include "gm_sub/0092EDA0.inc"
};
u32 D_0092EDE0[12] = {
#include "gm_sub/0092EDE0.inc"
};
u32 D_0092EE10[8] = {
#include "gm_sub/0092EE10.inc"
};
u32 D_0092EE30[4] = {
#include "gm_sub/0092EE30.inc"
};
u32 D_0092EE40[8] = {
#include "gm_sub/0092EE40.inc"
};
u32 D_0092EE60[4] = {
#include "gm_sub/0092EE60.inc"
};
u32 D_0092EE70[12] = {
#include "gm_sub/0092EE70.inc"
};
u32 D_0092EEA0[12] = {
#include "gm_sub/0092EEA0.inc"
};
u32 D_0092EED0[12] = {
#include "gm_sub/0092EED0.inc"
};
u32 D_0092EF00[6] = {
#include "gm_sub/0092EF00.inc"
};
u32 D_0092EF18[2] = {
#include "gm_sub/0092EF18.inc"
};
u32 D_0092EF20[8] = {
#include "gm_sub/0092EF20.inc"
};
u32 D_0092EF40[8] = {
#include "gm_sub/0092EF40.inc"
};
u32 D_0092EF60[10] = {
#include "gm_sub/0092EF60.inc"
};
u32 D_0092EF88[2] = {
#include "gm_sub/0092EF88.inc"
};
u32 D_0092EF90[2] = {
#include "gm_sub/0092EF90.inc"
};
u32 D_0092EF98[2] = {
#include "gm_sub/0092EF98.inc"
};
u32 D_0092EFA0[8] = {
#include "gm_sub/0092EFA0.inc"
};
u32 D_0092EFC0[8] = {
#include "gm_sub/0092EFC0.inc"
};
u32 D_0092EFE0[4] = {
#include "gm_sub/0092EFE0.inc"
};
u32 D_0092EFF0[4] = {
#include "gm_sub/0092EFF0.inc"
};
u32 D_0092F000[8] = {
#include "gm_sub/0092F000.inc"
};
u32 D_0092F020[4] = {
#include "gm_sub/0092F020.inc"
};
u32 D_0092F030[12] = {
#include "gm_sub/0092F030.inc"
};
u32 D_0092F060[24] = {
#include "gm_sub/0092F060.inc"
};
u32 D_0092F0C0[16] = {
#include "gm_sub/0092F0C0.inc"
};
u32 D_0092F100[8] = {
#include "gm_sub/0092F100.inc"
};
u32 D_0092F120[4] = {
#include "gm_sub/0092F120.inc"
};
u32 D_0092F130[4] = {
#include "gm_sub/0092F130.inc"
};
u32 D_0092F140[8] = {
#include "gm_sub/0092F140.inc"
};
u32 D_0092F160[8] = {
#include "gm_sub/0092F160.inc"
};
u32 D_0092F180[8] = {
#include "gm_sub/0092F180.inc"
};
u32 D_0092F1A0[4] = {
#include "gm_sub/0092F1A0.inc"
};
u32 D_0092F1B0[8] = {
#include "gm_sub/0092F1B0.inc"
};
u32 D_0092F1D0[4] = {
#include "gm_sub/0092F1D0.inc"
};
u32 D_0092F1E0[12] = {
#include "gm_sub/0092F1E0.inc"
};
u32 D_0092F210[8] = {
#include "gm_sub/0092F210.inc"
};
u32 D_0092F230[4] = {
#include "gm_sub/0092F230.inc"
};
u32 D_0092F240[8] = {
#include "gm_sub/0092F240.inc"
};
u32 D_0092F260[8] = {
#include "gm_sub/0092F260.inc"
};
u32 D_0092F280[4] = {
#include "gm_sub/0092F280.inc"
};
u32 D_0092F290[20] = {
#include "gm_sub/0092F290.inc"
};
u32 D_0092F2E0[12] = {
#include "gm_sub/0092F2E0.inc"
};
u32 D_0092F310[16] = {
#include "gm_sub/0092F310.inc"
};
u32 D_0092F350[8] = {
#include "gm_sub/0092F350.inc"
};
u32 D_0092F370[8] = {
#include "gm_sub/0092F370.inc"
};
u32 D_0092F390[16] = {
#include "gm_sub/0092F390.inc"
};
u32 D_0092F3D0[16] = {
#include "gm_sub/0092F3D0.inc"
};
u32 D_0092F410[8] = {
#include "gm_sub/0092F410.inc"
};
u32 D_0092F430[12] = {
#include "gm_sub/0092F430.inc"
};
u32 D_0092F460[8] = {
#include "gm_sub/0092F460.inc"
};
u32 D_0092F480[24] = {
#include "gm_sub/0092F480.inc"
};
u32 D_0092F4E0[24] = {
#include "gm_sub/0092F4E0.inc"
};
u32 D_0092F540[8] = {
#include "gm_sub/0092F540.inc"
};
u32 D_0092F560[20] = {
#include "gm_sub/0092F560.inc"
};
u32 D_0092F5B0[6] = {
#include "gm_sub/0092F5B0.inc"
};
u32 D_0092F5C8[2] = {
#include "gm_sub/0092F5C8.inc"
};
u32 D_0092F5D0[2] = {
#include "gm_sub/0092F5D0.inc"
};
u32 D_0092F5D8[2] = {
#include "gm_sub/0092F5D8.inc"
};
u32 D_0092F5E0[8] = {
#include "gm_sub/0092F5E0.inc"
};
u32 D_0092F600[8] = {
#include "gm_sub/0092F600.inc"
};
u32 D_0092F620[4] = {
#include "gm_sub/0092F620.inc"
};
u32 D_0092F630[4] = {
#include "gm_sub/0092F630.inc"
};
u32 D_0092F640[2] = {
#include "gm_sub/0092F640.inc"
};
u32 D_0092F648[2] = {
#include "gm_sub/0092F648.inc"
};
u32 D_0092F650[2] = {
#include "gm_sub/0092F650.inc"
};
u32 D_0092F658[2] = {
#include "gm_sub/0092F658.inc"
};
u32 D_0092F660[4] = {
#include "gm_sub/0092F660.inc"
};
u32 D_0092F670[8] = {
#include "gm_sub/0092F670.inc"
};
u32 D_0092F690[12] = {
#include "gm_sub/0092F690.inc"
};
u32 D_0092F6C0[12] = {
#include "gm_sub/0092F6C0.inc"
};
u32 D_0092F6F0[16] = {
#include "gm_sub/0092F6F0.inc"
};
u32 D_0092F730[8] = {
#include "gm_sub/0092F730.inc"
};
u32 D_0092F750[8] = {
#include "gm_sub/0092F750.inc"
};
u32 D_0092F770[8] = {
#include "gm_sub/0092F770.inc"
};
u32 D_0092F790[8] = {
#include "gm_sub/0092F790.inc"
};
u32 D_0092F7B0[8] = {
#include "gm_sub/0092F7B0.inc"
};
u32 D_0092F7D0[12] = {
#include "gm_sub/0092F7D0.inc"
};
u32 D_0092F800[12] = {
#include "gm_sub/0092F800.inc"
};
u32 D_0092F830[8] = {
#include "gm_sub/0092F830.inc"
};
u32 D_0092F850[8] = {
#include "gm_sub/0092F850.inc"
};
u32 D_0092F870[8] = {
#include "gm_sub/0092F870.inc"
};
u32 D_0092F890[8] = {
#include "gm_sub/0092F890.inc"
};
u32 D_0092F8B0[8] = {
#include "gm_sub/0092F8B0.inc"
};
u32 D_0092F8D0[8] = {
#include "gm_sub/0092F8D0.inc"
};
u32 D_0092F8F0[8] = {
#include "gm_sub/0092F8F0.inc"
};
u32 D_0092F910[8] = {
#include "gm_sub/0092F910.inc"
};
u32 D_0092F930[8] = {
#include "gm_sub/0092F930.inc"
};
u32 D_0092F950[8] = {
#include "gm_sub/0092F950.inc"
};
u32 D_0092F970[12] = {
#include "gm_sub/0092F970.inc"
};
u32 D_0092F9A0[8] = {
#include "gm_sub/0092F9A0.inc"
};
u32 D_0092F9C0[8] = {
#include "gm_sub/0092F9C0.inc"
};
u32 D_0092F9E0[10] = {
#include "gm_sub/0092F9E0.inc"
};
u32 D_0092FA08[2] = {
#include "gm_sub/0092FA08.inc"
};
u32 D_0092FA10[2] = {
#include "gm_sub/0092FA10.inc"
};
u32 D_0092FA18[2] = {
#include "gm_sub/0092FA18.inc"
};
u32 D_0092FA20[8] = {
#include "gm_sub/0092FA20.inc"
};
u32 D_0092FA40[8] = {
#include "gm_sub/0092FA40.inc"
};
u32 D_0092FA60[4] = {
#include "gm_sub/0092FA60.inc"
};
u32 D_0092FA70[4] = {
#include "gm_sub/0092FA70.inc"
};
u32 D_0092FA80[8] = {
#include "gm_sub/0092FA80.inc"
};
u32 D_0092FAA0[4] = {
#include "gm_sub/0092FAA0.inc"
};
u32 D_0092FAB0[8] = {
#include "gm_sub/0092FAB0.inc"
};
u32 D_0092FAD0[8] = {
#include "gm_sub/0092FAD0.inc"
};
u32 D_0092FAF0[24] = {
#include "gm_sub/0092FAF0.inc"
};
u32 D_0092FB50[16] = {
#include "gm_sub/0092FB50.inc"
};
u32 D_0092FB90[8] = {
#include "gm_sub/0092FB90.inc"
};
u32 D_0092FBB0[4] = {
#include "gm_sub/0092FBB0.inc"
};
u32 D_0092FBC0[4] = {
#include "gm_sub/0092FBC0.inc"
};
u32 D_0092FBD0[8] = {
#include "gm_sub/0092FBD0.inc"
};
u32 D_0092FBF0[8] = {
#include "gm_sub/0092FBF0.inc"
};
u32 D_0092FC10[6] = {
#include "gm_sub/0092FC10.inc"
};
u32 D_0092FC28[2] = {
#include "gm_sub/0092FC28.inc"
};
u32 D_0092FC30[8] = {
#include "gm_sub/0092FC30.inc"
};
u32 D_0092FC50[8] = {
#include "gm_sub/0092FC50.inc"
};
u32 D_0092FC70[12] = {
#include "gm_sub/0092FC70.inc"
};
u32 D_0092FCA0[8] = {
#include "gm_sub/0092FCA0.inc"
};
u32 D_0092FCC0[8] = {
#include "gm_sub/0092FCC0.inc"
};
u32 D_0092FCE0[8] = {
#include "gm_sub/0092FCE0.inc"
};
u32 D_0092FD00[8] = {
#include "gm_sub/0092FD00.inc"
};
u32 D_0092FD20[4] = {
#include "gm_sub/0092FD20.inc"
};
u32 D_0092FD30[8] = {
#include "gm_sub/0092FD30.inc"
};
u32 D_0092FD50[8] = {
#include "gm_sub/0092FD50.inc"
};
u32 D_0092FD70[4] = {
#include "gm_sub/0092FD70.inc"
};
u32 D_0092FD80[4] = {
#include "gm_sub/0092FD80.inc"
};
u32 D_0092FD90[12] = {
#include "gm_sub/0092FD90.inc"
};
u32 D_0092FDC0[16] = {
#include "gm_sub/0092FDC0.inc"
};
u32 D_0092FE00[8] = {
#include "gm_sub/0092FE00.inc"
};
u32 D_0092FE20[8] = {
#include "gm_sub/0092FE20.inc"
};
u32 D_0092FE40[8] = {
#include "gm_sub/0092FE40.inc"
};
u32 D_0092FE60[4] = {
#include "gm_sub/0092FE60.inc"
};
u32 D_0092FE70[8] = {
#include "gm_sub/0092FE70.inc"
};
u32 D_0092FE90[8] = {
#include "gm_sub/0092FE90.inc"
};
u32 D_0092FEB0[12] = {
#include "gm_sub/0092FEB0.inc"
};
u32 D_0092FEE0[8] = {
#include "gm_sub/0092FEE0.inc"
};
u32 D_0092FF00[8] = {
#include "gm_sub/0092FF00.inc"
};
u32 D_0092FF20[8] = {
#include "gm_sub/0092FF20.inc"
};
u32 D_0092FF40[8] = {
#include "gm_sub/0092FF40.inc"
};
u32 D_0092FF60[16] = {
#include "gm_sub/0092FF60.inc"
};
u32 D_0092FFA0[12] = {
#include "gm_sub/0092FFA0.inc"
};
u32 D_0092FFD0[8] = {
#include "gm_sub/0092FFD0.inc"
};
u32 D_0092FFF0[12] = {
#include "gm_sub/0092FFF0.inc"
};
u32 D_00930020[8] = {
#include "gm_sub/00930020.inc"
};
u32 D_00930040[8] = {
#include "gm_sub/00930040.inc"
};
u32 D_00930060[8] = {
#include "gm_sub/00930060.inc"
};
u32 D_00930080[16] = {
#include "gm_sub/00930080.inc"
};
u32 D_009300C0[8] = {
#include "gm_sub/009300C0.inc"
};
u32 D_009300E0[8] = {
#include "gm_sub/009300E0.inc"
};
u32 D_00930100[2] = {
#include "gm_sub/00930100.inc"
};
u32 D_00930108[2] = {
#include "gm_sub/00930108.inc"
};
u32 D_00930110[2] = {
#include "gm_sub/00930110.inc"
};
u32 D_00930118[2] = {
#include "gm_sub/00930118.inc"
};
u32 D_00930120[4] = {
#include "gm_sub/00930120.inc"
};
u32 D_00930130[4] = {
#include "gm_sub/00930130.inc"
};
u32 D_00930140[4] = {
#include "gm_sub/00930140.inc"
};
u32 D_00930150[6] = {
#include "gm_sub/00930150.inc"
};
u32 D_00930168[2] = {
#include "gm_sub/00930168.inc"
};
u32 D_00930170[2] = {
#include "gm_sub/00930170.inc"
};
u32 D_00930178[2] = {
#include "gm_sub/00930178.inc"
};
u32 D_00930180[2] = {
#include "gm_sub/00930180.inc"
};
u32 D_00930188[2] = {
#include "gm_sub/00930188.inc"
};
u32 D_00930190[4] = {
#include "gm_sub/00930190.inc"
};
u32 D_009301A0[6] = {
#include "gm_sub/009301A0.inc"
};
u32 D_009301B8[2] = {
#include "gm_sub/009301B8.inc"
};
u32 D_009301C0[12] = {
#include "gm_sub/009301C0.inc"
};
u32 D_009301F0[24] = {
#include "gm_sub/009301F0.inc"
};
u32 D_00930250[8] = {
#include "gm_sub/00930250.inc"
};
u32 D_00930270[20] = {
#include "gm_sub/00930270.inc"
};
u32 D_009302C0[12] = {
#include "gm_sub/009302C0.inc"
};
u32 D_009302F0[8] = {
#include "gm_sub/009302F0.inc"
};
u32 D_00930310[16] = {
#include "gm_sub/00930310.inc"
};
u32 D_00930350[8] = {
#include "gm_sub/00930350.inc"
};
u32 D_00930370[12] = {
#include "gm_sub/00930370.inc"
};
u32 D_009303A0[12] = {
#include "gm_sub/009303A0.inc"
};
u32 D_009303D0[16] = {
#include "gm_sub/009303D0.inc"
};
u32 D_00930410[8] = {
#include "gm_sub/00930410.inc"
};
u32 D_00930430[10] = {
#include "gm_sub/00930430.inc"
};
u32 D_00930458[2] = {
#include "gm_sub/00930458.inc"
};
u32 D_00930460[2] = {
#include "gm_sub/00930460.inc"
};
u32 D_00930468[2] = {
#include "gm_sub/00930468.inc"
};
u32 D_00930470[2] = {
#include "gm_sub/00930470.inc"
};
u32 D_00930478[2] = {
#include "gm_sub/00930478.inc"
};
u32 D_00930480[16] = {
#include "gm_sub/00930480.inc"
};
u32 D_009304C0[12] = {
#include "gm_sub/009304C0.inc"
};
u32 D_009304F0[8] = {
#include "gm_sub/009304F0.inc"
};
u32 D_00930510[8] = {
#include "gm_sub/00930510.inc"
};
u32 D_00930530[24] = {
#include "gm_sub/00930530.inc"
};
u32 D_00930590[8] = {
#include "gm_sub/00930590.inc"
};
u32 D_009305B0[12] = {
#include "gm_sub/009305B0.inc"
};
u32 D_009305E0[8] = {
#include "gm_sub/009305E0.inc"
};
u32 D_00930600[16] = {
#include "gm_sub/00930600.inc"
};
u32 D_00930640[8] = {
#include "gm_sub/00930640.inc"
};
u32 D_00930660[6] = {
#include "gm_sub/00930660.inc"
};
u32 D_00930678[2] = {
#include "gm_sub/00930678.inc"
};
u32 D_00930680[12] = {
#include "gm_sub/00930680.inc"
};
u32 D_009306B0[12] = {
#include "gm_sub/009306B0.inc"
};
u32 D_009306E0[8] = {
#include "gm_sub/009306E0.inc"
};
u32 D_00930700[12] = {
#include "gm_sub/00930700.inc"
};
u32 D_00930730[8] = {
#include "gm_sub/00930730.inc"
};
u32 D_00930750[8] = {
#include "gm_sub/00930750.inc"
};
u32 D_00930770[8] = {
#include "gm_sub/00930770.inc"
};
u32 D_00930790[10] = {
#include "gm_sub/00930790.inc"
};
u32 D_009307B8[2] = {
#include "gm_sub/009307B8.inc"
};
u32 D_009307C0[12] = {
#include "gm_sub/009307C0.inc"
};
u32 D_009307F0[24] = {
#include "gm_sub/009307F0.inc"
};
u32 D_00930850[8] = {
#include "gm_sub/00930850.inc"
};
u32 D_00930870[12] = {
#include "gm_sub/00930870.inc"
};
u32 D_009308A0[8] = {
#include "gm_sub/009308A0.inc"
};
u32 D_009308C0[8] = {
#include "gm_sub/009308C0.inc"
};
u32 D_009308E0[16] = {
#include "gm_sub/009308E0.inc"
};
u32 D_00930920[12] = {
#include "gm_sub/00930920.inc"
};
u32 D_00930950[12] = {
#include "gm_sub/00930950.inc"
};
u32 D_00930980[12] = {
#include "gm_sub/00930980.inc"
};
u32 D_009309B0[12] = {
#include "gm_sub/009309B0.inc"
};
u32 D_009309E0[12] = {
#include "gm_sub/009309E0.inc"
};
u32 D_00930A10[16] = {
#include "gm_sub/00930A10.inc"
};
u32 D_00930A50[8] = {
#include "gm_sub/00930A50.inc"
};
u32 D_00930A70[8] = {
#include "gm_sub/00930A70.inc"
};
u32 D_00930A90[8] = {
#include "gm_sub/00930A90.inc"
};
u32 D_00930AB0[10] = {
#include "gm_sub/00930AB0.inc"
};
u32 D_00930AD8[2] = {
#include "gm_sub/00930AD8.inc"
};
u32 D_00930AE0[2] = {
#include "gm_sub/00930AE0.inc"
};
u32 D_00930AE8[2] = {
#include "gm_sub/00930AE8.inc"
};
u32 D_00930AF0[578] = {
#include "gm_sub/00930AF0.inc"
};
u32 D_009313F8[2] = {
#include "gm_sub/009313F8.inc"
};
u32 D_00931400[2] = {
    (u32)D_009313F8, (u32)D_009313F8,
};
u32 D_00931408[2] = {
#include "gm_sub/00931408.inc"
};
u32 D_00931410[4] = {
    (u32)D_00931408, (u32)D_00931408, 0x00000000, 0x00000000,
};
u32 D_00931420[6] = {
#include "gm_sub/00931420.inc"
};
u32 D_00931438[2] = {
    (u32)D_00931420, (u32)D_00931420,
};
u32 D_00931440[6] = {
#include "gm_sub/00931440.inc"
};
u32 D_00931458[2] = {
    (u32)D_00931440, (u32)D_00931440,
};
u32 D_00931460[6] = {
#include "gm_sub/00931460.inc"
};
u32 D_00931478[2] = {
    (u32)D_00931460, (u32)D_00931460,
};
u32 D_00931480[6] = {
#include "gm_sub/00931480.inc"
};
u32 D_00931498[2] = {
    (u32)D_00931480, (u32)D_00931480,
};
u32 D_009314A0[6] = {
#include "gm_sub/009314A0.inc"
};
u32 D_009314B8[2] = {
    (u32)D_009314A0, (u32)D_009314A0,
};
u32 D_009314C0[2] = {
#include "gm_sub/009314C0.inc"
};
u32 D_009314C8[2] = {
    (u32)D_009314C0, (u32)D_009314C0,
};
u32 D_009314D0[4] = {
#include "gm_sub/009314D0.inc"
};
u32 D_009314E0[2] = {
    (u32)D_009314D0, (u32)D_009314D0,
};
u32 D_009314E8[3] = {
#include "gm_sub/009314E8.inc"
};
u32 D_009314F8[2] = {
    (u32)D_009314E8, (u32)D_009314E8,
};
u32 D_00931500[4] = {
#include "gm_sub/00931500.inc"
};
u32 D_00931510[4] = {
    (u32)D_00931500, (u32)D_00931500, 0x00000000, 0x00000000,
};
u32 D_00931520[6] = {
#include "gm_sub/00931520.inc"
};
u32 D_00931538[2] = {
    (u32)D_00931520, (u32)D_00931520,
};
u32 D_00931540[2] = {
#include "gm_sub/00931540.inc"
};
u32 D_00931548[2] = {
    (u32)D_00931540, (u32)D_00931540,
};
u32 D_00931550[6] = {
#include "gm_sub/00931550.inc"
};
u32 D_00931568[2] = {
    (u32)D_00931550, (u32)D_00931550,
};
u32 D_00931570[2] = {
#include "gm_sub/00931570.inc"
};
u32 D_00931578[2] = {
    (u32)D_00931570, (u32)D_00931570,
};
u32 D_00931580[6] = {
#include "gm_sub/00931580.inc"
};
u32 D_00931598[2] = {
    (u32)D_00931580, (u32)D_00931580,
};
u32 D_009315A0[6] = {
#include "gm_sub/009315A0.inc"
};
u32 D_009315B8[2] = {
    (u32)D_009315A0, (u32)D_009315A0,
};
u32 D_009315C0[6] = {
#include "gm_sub/009315C0.inc"
};
u32 D_009315D8[2] = {
    (u32)D_009315C0, (u32)D_009315C0,
};
u32 D_009315E0[6] = {
#include "gm_sub/009315E0.inc"
};
u32 D_009315F8[2] = {
    (u32)D_009315E0, (u32)D_009315E0,
};
u32 D_00931600[6] = {
#include "gm_sub/00931600.inc"
};
u32 D_00931618[2] = {
    (u32)D_00931600, (u32)D_00931600,
};
u32 D_00931620[6] = {
#include "gm_sub/00931620.inc"
};
u32 D_00931638[2] = {
    (u32)D_00931620, (u32)D_00931620,
};
u32 D_00931640[6] = {
#include "gm_sub/00931640.inc"
};
u32 D_00931658[2] = {
    (u32)D_00931640, (u32)D_00931640,
};
u32 D_00931660[2] = {
#include "gm_sub/00931660.inc"
};
u32 D_00931668[2] = {
    (u32)D_00931660, (u32)D_00931660,
};
u32 D_00931670[2] = {
#include "gm_sub/00931670.inc"
};
u32 D_00931678[2] = {
    (u32)D_00931670, (u32)D_00931670,
};
u32 D_00931680[4] = {
#include "gm_sub/00931680.inc"
};
u32 D_00931690[36] = {
#include "gm_sub/00931690.inc"
};
u32 D_00931720[16] = {
#include "gm_sub/00931720.inc"
};
u32 D_00931760[4] = {
#include "gm_sub/00931760.inc"
};
u32 D_00931770[32] = {
#include "gm_sub/00931770.inc"
};
u32 D_009317F0[4] = {
#include "gm_sub/009317F0.inc"
};
u32 D_00931800[16] = {
#include "gm_sub/00931800.inc"
};
u32 D_00931840[152] = {
#include "gm_sub/00931840.inc"
};
u32 gGmKindFileLists_0[76] = {
#include "gm_sub/00931AA0.inc"
};
u32 gGmKindFileLists_2[76] = {
#include "gm_sub/00931BD0.inc"
};
u32 gGmKindFileLists_3[76] = {
#include "gm_sub/00931D00.inc"
};
u32 gGmKindFileLists_4[76] = {
#include "gm_sub/00931E30.inc"
};
u32 gGmKindFileLists_5[76] = {
#include "gm_sub/00931F60.inc"
};
u32 gGmKindFileLists_6[76] = {
#include "gm_sub/00932090.inc"
};
u32 gGmKindFileLists_7[76] = {
#include "gm_sub/009321C0.inc"
};
u32 gGmKindFileLists_8[76] = {
#include "gm_sub/009322F0.inc"
};
u32 gGmKindFileLists_9[76] = {
#include "gm_sub/00932420.inc"
};
u32 gGmKindFileLists_10[76] = {
#include "gm_sub/00932550.inc"
};
u32 gGmKindFileLists_11[76] = {
#include "gm_sub/00932680.inc"
};
u32 gGmKindFileLists_12[76] = {
#include "gm_sub/009327B0.inc"
};
u32 gGmKindFileLists_13[76] = {
#include "gm_sub/009328E0.inc"
};
u32 gGmKindFileLists_14[76] = {
#include "gm_sub/00932A10.inc"
};
u32 gGmKindFileLists_15[76] = {
#include "gm_sub/00932B40.inc"
};
