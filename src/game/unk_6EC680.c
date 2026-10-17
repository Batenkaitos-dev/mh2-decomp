/*
 * Game overlay code at 0x6EC680-0x6F8A10, not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "quest.h"
#include "em.h"
#include "obj.h"
#include "objattach.h"
#include "player.h"
#include "sprite.h"
#include "scene.h"
#include "stageobj.h"
#include "window.h"
#include "area.h"
#include "grid.h"
#include "capsule.h"
/* prototypes of callees (round 13) */
s32 Player_TestSetFlags(void *, s32 mask);
struct MoveObj;
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
s32 Quest_PhaseIsOver(s32);
s32 Game_IsOnline(void);
s32 AreaObj_K05_SoundIdx(Obj *o);

/* An entry of the dated event tables (MsgEvent_CheckList): on day `day` (the u16 at 0x5D8268) event flag
 * `flag` is set once, message `msg` is chosen (gQuestMsgWindow +9) and flags 200 + ev[i] are set. A
 * negative flag ends the table. */
typedef struct MsgEvent {
    s16 flag;
    u16 day;
    u8 msg;
    u8 ev[3];
} MsgEvent;

/* A model entry of the stage data (the table at +0x30 of the stage block that 0x5DA070 points to):
 * the model drawn with Model_Draw and the value Draw_ApplyStates takes before it. */
/* prototypes */
extern u16 gFrameTick;
/* unit's own declaration: with the definition's s16 (u16 area) (sub_main/unk_376C00.c), AreaObj_K07_Run: 68 of 376 words differ */
s32 Area_MonsterState(u16 area);
extern s16 AreaObj_K07_SpinDef[];
extern s16 AreaObj_K07_Spin1[];
typedef struct ObjParts13 {
    u8 state[13];               /* 0x00 */
    u8 level[13];               /* 0x0D */
    u16 rot[13];                /* 0x1A */
    s16 slot[13];               /* 0x34: the draw slots (DrawSlot_Alloc) */
    u8 pad4E[0x50 - 0x4E];
    DrawSlot *ds[13];           /* 0x50 */
} ObjParts13;
u16 Mtx_Heading(f32 *m);
void Vec_RotateY(f32 *v, f32 angle);
void Mtx_GetTrans(f32 *out, f32 *m);
f32 Vec_Distance(f32 *a, f32 *b);
extern f32 AreaObj_K03_Sc32[];
extern f32 AreaObj_K03_R32[];
extern f32 AreaObj_K03_Sc82[];
extern f32 AreaObj_K03_R82[];
extern f32 AreaObj_K03_Sc90[];
extern f32 AreaObj_K03_R90[];
extern f32 AreaObj_K03_Sc44[];
extern f32 AreaObj_K03_R44[];
extern f32 AreaObj_K03_Sc9B[];
extern f32 AreaObj_K03_R9B[];
extern f32 AreaObj_K03_Sc49[];
extern f32 AreaObj_K03_R49[];
f32 Math_Sin(f32 x);
void Mtx_Copy(f32 *dst, f32 *src);
void Gfx_SetBlend(s32 a, s32 b);
void Mtx_ScaleBy(f32 *m, f32 x, f32 y, f32 z);
extern f32 gCamYawMtx[];
extern s16 AreaObj_K00_RotX[];
extern s16 D_008B00CA[];
extern f32 AreaObj_K00_Scale[];
extern f32 D_008B00D4[];
extern f32 D_008B00D8[];
extern s16 AreaObj_K00_RotZA[];
extern f32 AreaObj_K00_DrawF[];
extern s16 AreaObj_K00_RotZB[];
extern s16 AreaObj_K00_Models[];
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_TurnY(f32 *m, f32 a);
void Mtx_TurnZ(f32 *m, f32 a);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
/* The buffer of AreaObj_K09's types 1, 2, 3, 6 and 7 (AreaObj_K09_SetupTypeN sets it up, RunTypeN moves the
 * object, DrawType1, 2 and 6 draw it with m); type 1 tumbles; type 2 uses spinX as its turn mode and spinZ as the
 * frames left in it. */
typedef struct ObjTumble {
    f32 m[16];                  /* 0x00: the orientation */
    f32 v[3];                   /* 0x40: the velocity, in the object's frame */
    u16 rot[3];                 /* 0x4C: the angles about x, y and z */
    s16 spinX;                  /* 0x52 */
    s16 unk54;                  /* 0x54 */
    s16 spinZ;                  /* 0x56 */
} ObjTumble;
void Effect0D_SpawnAt(f32 *pos, s32 kind, f32 scale);
void SoundObj_Start(s32 bank, s32 se, s32 k, f32 *pos, s16 n, u8 quiet);
void AreaObj_K03_RunA(Obj *o);
void AreaObj_K03_RunB(Obj *o);
extern f32 AreaObj_K03_Points[];
void MsgEvent_CopyQuestInfo(s8 *arg0);
void *Shake_GetOffset(u8 id);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Menu_DrawFrame(void *b, s16 *pos);
void Window_DrawText(void *p, s16 *pos, u8 *text);
s32 Str_Length(s8 *text);
void Text_Draw(s8 *text);
extern ListPanel gMsgEventListPanel;
extern char gMsgEventListItems[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
void Mtx_Concat3x3(f32 *m, f32 *a);
extern f32 gCamMatrix[];
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
extern u8 TeostraBlastRing_ScaleKeysStart[];
extern u8 TeostraBlastRing_ColorKeysStart[];
extern u8 TeostraBlastRing_ScaleKeysMiddle[];
extern u8 TeostraBlastRing_ColorKeysMiddle[];
extern u8 TeostraBlastRing_ScaleKeysEnd[];
extern u8 TeostraBlastRing_ColorKeysEnd[];
extern f32 AreaObj_K00_PosArea29[];
extern f32 AreaObj_K00_PosArea2A[];
extern f32 AreaObj_K00_PosArea2B[];
extern f32 AreaObj_K00_PosArea5C[];
extern f32 AreaObj_K00_PosArea5E[];
extern f32 AreaObj_K00_PosArea82[];
extern f32 AreaObj_K00_PosArea32[];
extern f32 AreaObj_K00_PosArea44[];
extern f32 AreaObj_K00_PosArea45[];
void AreaObj_K00_Draw(DrawSlot *ds);
typedef struct ObjSlots6 {
    s16 slot[6];
    DrawSlot *ds[6];
} ObjSlots6;
/* The buffer of the swimming objects (AreaObj_K09_SetupType5 sets them up, AreaObj_K09_RunType5 moves them). */
typedef struct ObjSlots2 {
    s16 slot[2];
    f32 speed;                  /* 0x04: forward */
    f32 side;                   /* 0x08 */
    u16 heading;                /* 0x0C */
    s16 pitch;                  /* 0x0E */
    u16 goal;                   /* 0x10: the pitch it turns to */
    s16 steps;                  /* 0x12: the frames left to reach it */
    DrawSlot *ds[2];
} ObjSlots2;
f32 Math_Floor(f32 x);
void Gfx_SetTexFilter(s32 a);
extern f32 AreaObj_K08_Steps[];
s32 QuestGoal_DeliveryDone(void);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void AreaObj_K04_Draw(DrawSlot *ds);
void AreaObj_K04_Run(Obj *o);
extern f32 AreaObj_K09_Type2Pos[];
extern f32 D_008B09E4[];
extern f32 D_008B09E8[];
f32 Math_Atan2(f32 y, f32 x);
void AreaObj_K09_SetupType0(Obj *o);
void AreaObj_K09_SetupType1(Obj *o);
void AreaObj_K09_SetupType2(Obj *o);
void AreaObj_K09_SetupType3(Obj *o);
void AreaObj_K09_SetupType4(Obj *o);
void AreaObj_K09_SetupType5(Obj *o);
void AreaObj_K09_SetupType7(Obj *o);
void AreaObj_K09_SetupType8(Obj *o);
void AreaObj_K09_SetupType9(void);
void AreaObj_K09_SetupType10(void);
void AreaObj_K09_SetupType11(Obj *o);
void AreaObj_K09_SetupType6(Obj *o);
void AreaObj_K09_Draw(DrawSlot *ds);
extern u8 *AreaObj_K05_Kinds[];
extern f32 *AreaObj_K05_Frames6[];
extern f32 *AreaObj_K05_Frames2[];
extern f32 *AreaObj_K05_Frames8[];
extern f32 *AreaObj_K05_Frames9[];
extern f32 *AreaObj_K05_Params[];
void AreaObj_K05_Draw(DrawSlot *ds);
extern u16 AreaObj_K05_AreaVariants[];
extern s16 *AreaObj_K05_Models[];
extern u16 AreaObj_K05_Angles2[];
extern s16 AreaObj_K02_RandRange[];
extern s16 AreaObj_K02_WaitBase[];
extern u16 AreaObj_K02_WaitMask[];
void AreaObj_K02_Draw(DrawSlot *ds);
typedef struct ObjPart30 {
    s16 unk00;
    s16 slot;
    f32 unk04[3];
    u8 pad10[0xC];
    DrawSlot *ds;
    s16 unk20[2];
    s16 unk24;
    s16 unk26;
    s16 unk28;
    s16 unk2A;
    u8 pad2C[4];
} ObjPart30;
extern f32 gStartPos[];
extern f32 D_43F0A4[];
extern f32 D_43F0A8[];
void Mtx_Identity(f32 *m);
void AreaObj_K05_Update(Obj *o);
void ChameleosAttack_Update(ObjAttach *o);
void AreaObj_K03_Draw(DrawSlot *ds);
s16 AreaObj_K09_FindFreeChild(Obj *o);
typedef struct ObjList32 {
    Obj *obj[32];
    s16 count[16];
} ObjList32;
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void AreaObj_K08_Draw(DrawSlot *s);
s32 MsgEvent_IsSeen(u8 arg0, u8 arg1);
void Menu_GridCursor(Grid *g, u16 buttons);
char *MsgEvent_Title(u8 a, u8 b);
char *MsgEvent_Line(u8 a, u8 b, u8 c);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
u32 strlen(const void *s);
void Text_PrintfStyled(s32 a, const char *fmt, ...);
void MsgEvent_DrawTextBg(s16 x, s16 y);
void MsgEvent_DrawText(u8 flag, s32 arg1);
typedef struct SprRect {
    s16 x, y, w, h;
    s32 color;
    s32 uv0;
    s32 uv1;
} SprRect;
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 tex);
s32 Sprite_Draw(struct Sprite *s);
extern u8 gMsgEventIconShown[];
void MsgEvent_DrawIcon(s32 arg0, s32 arg1);
extern s16 AreaObj_K02_Models[];
extern s16 AreaObj_K02_Frames[];
/* The message window of the quest's advice messages (gQuestMsgWindowBuf; gQuestMsgWindow points to it,
 * MsgEvent_Init): the message list of the quest's set (MsgEvent_OpenList, MsgEvent_ListInput: a Grid of
 * 7 rows) and the popup of a message an event chose (MsgEvent_Check, MsgEvent_Open, MsgEvent_PopupInput). */
typedef struct QuestMsgWin {
    u8 listStep;        /* 0x00: the list window: 0 open, 1 closing (Shake), 2 closed */
    u8 listSub;         /* 0x01: 0 the list, 1 a message's pages, 2 left with the back button */
    s16 unk02;          /* 0x02: cleared by MsgEvent_Init */
    u8 page;            /* 0x04: the page of the message shown (MsgEvent_Line) */
    u8 unk05;
    u8 pages;           /* 0x06: its pages (MsgEvent_Kind) */
    u8 set;             /* 0x07: the message set: 0x12 .. 0x18 for some stages, else gQuest.stage - 1 */
    u8 msg;             /* 0x08: the message shown */
    s8 request;         /* 0x09: the message an event chose (MsgEvent_CheckList, MsgEvent_CheckDay), -1: none */
    u8 count;           /* 0x0A: the messages of the set (MsgEvent_SetCount) */
    u8 popupStep;       /* 0x0B: the popup: 0 none, 1 opening, 2 shown, 3 closing (MsgEvent_PopupInput) */
    u8 popupReq;        /* 0x0C: 1: MsgEvent_Check chose a message; MsgEvent_Open opens the popup */
    u8 unk0D[0x10 - 0x0D];
    Grid grid;          /* 0x10: the list's cursor (7 rows; grid.index is the message) */
} QuestMsgWin;
extern QuestMsgWin gQuestMsgWindowBuf;
u8 MsgEvent_SetCount(u8 arg0);
void RemobraAttack_Update(ObjAttach *o);
u8 Player_InCurrentArea(void *obj);
ObjAttach *ObjAttach_Alloc(s32 n);
s32 MsgEvent_CheckDay(s32 arg0, s32 arg1, s8 arg2);
s32 MsgEvent_CheckStageIntro(void);
s32 Quest_HasAdvice(void);
extern MsgEvent gMsgEventCheck0[];
extern MsgEvent gMsgEventCheck1[];
extern MsgEvent gMsgEventCheck15[];
extern MsgEvent gMsgEventCheck12[];
extern MsgEvent gMsgEventCheck14[];
extern MsgEvent gMsgEventCheck5[];
extern MsgEvent gMsgEventCheck8[];
extern HitShape *ChameleosAttack_HitShapes[];
extern void *ChameleosAttack_RowsPtr;
f32 Math_Absf(f32 x);
extern HitShape *RemobraAttack_HitShapes[];
extern void *RemobraAttack_RowsPtr;
extern u8 TeostraBlastRing_ModelKinds[];
s16 EmSlot_FindKind(s16 id);
s32 Model_Draw(u32 h, s32 flags);
void ObjAttach_Free(ObjAttach *p);
void Obj_Free(struct Obj *p);
u16 Rand_Next(s32 stream);
void Save_SetEventFlag(s32 n);
s32 Save_TestEventFlag(s32);
void Shake_Start(u8 id, u8 flags, f32 amp);
s32 Shake_Update(u8 id);
void ChameleosAttack_Setup(ObjAttach *o);
void ChameleosAttack_Run(ObjAttach *o);
void ChameleosAttack_End(ObjAttach *o);
void ChameleosAttack_Free(ObjAttach *o);
void RemobraAttack_Setup(ObjAttach *o);
void RemobraAttack_Run(ObjAttach *o);
void RemobraAttack_End(ObjAttach *o);
void RemobraAttack_Free(ObjAttach *o);
void MsgEvent_DrawList(void);
u8 MsgEvent_Kind(u8, u8);
void AreaObj_K00_Setup(Obj *o);
void AreaObj_K00_Run(Obj *);
void AreaObj_K00_End(Obj *o);
void AreaObj_K00_Free(Obj *o);
void AreaObj_K02_Setup(Obj *o);
void AreaObj_K02_Run(Obj *o);
void AreaObj_K02_End(Obj *o);
void AreaObj_K02_Free(Obj *o);
void AreaObj_K03_Setup(Obj *o);
void AreaObj_K03_Run(Obj *o);
void AreaObj_K03_End(Obj *o);
void AreaObj_K03_Free(Obj *o);
void AreaObj_K04_Setup(Obj *o);
void AreaObj_K04_Run(Obj *o);
void AreaObj_K04_End(Obj *o);
void AreaObj_K04_Free(Obj *o);
void AreaObj_K05_Setup(Obj *o);
void AreaObj_K05_Run(Obj *o);
void AreaObj_K05_End(Obj *o);
void AreaObj_K05_Free(Obj *o);
void AreaObj_K07_Setup(void *arg);
void AreaObj_K07_Draw(DrawSlot *ds);
extern f32 AreaObj_K07_PosArea2A[];
extern f32 AreaObj_K07_PosArea61[];
extern f32 AreaObj_K07_PosArea69[];
extern f32 AreaObj_K07_PosArea6A[];
extern f32 AreaObj_K07_PosArea6D[];
extern f32 AreaObj_K07_PosAreaB3[];
void AreaObj_K07_Run(Obj *o);
void AreaObj_K07_End(Obj *o);
void AreaObj_K07_Free(Obj *o);
void AreaObj_K08_Setup(Obj *o);
void AreaObj_K08_Run(Obj *o);
void AreaObj_K08_End(Obj *o);
void AreaObj_K08_Free(Obj *o);
void AreaObj_K09_Setup(Obj *o);
void AreaObj_K09_Run(Obj *o);
void AreaObj_K09_End(Obj *o);
void AreaObj_K09_Free(Obj *o);
void AreaObj_K09_RunType0(Obj *o);
void AreaObj_K09_RunType1(Obj *o);
void AreaObj_K09_DrawType1(DrawSlot *ds);
void AreaObj_K09_RunType2(Obj *o);
void AreaObj_K09_DrawType2(DrawSlot *ds);
void AreaObj_K09_RunType3(Obj *o);
void AreaObj_K09_DrawType3(DrawSlot *ds);
void AreaObj_K09_RunType4(Obj *o);
void AreaObj_K09_DrawType4(DrawSlot *ds);
void AreaObj_K09_RunType5(Obj *o);
void AreaObj_K09_DrawType5(DrawSlot *ds);
void AreaObj_K09_RunType6(Obj *);
void AreaObj_K09_DrawType6(DrawSlot *ds);
u32 AreaObj_K09_RunType7(Obj *o);
void AreaObj_K09_DrawType7(DrawSlot *ds);
void AreaObj_K09_RunType8(Obj *o);
void AreaObj_K09_DrawType8(DrawSlot *ds);
void AreaObj_K09_RunType9(void);
void AreaObj_K09_DrawType9(void);
void AreaObj_K09_RunType10(void);
void AreaObj_K09_DrawType10(void);
void AreaObj_K09_RunType11(Obj *o);
void AreaObj_K09_DrawType11(DrawSlot *s);
s32 Gs_SetState(u8 reg, u32 v);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
void Game_Pause(u8 arg0);
void Game_Resume(u8 arg0);
void DrawSlot_Free(s32 i);
u16 Area_DayId(u16 id);
void * QuestInfo_Get(s32);
extern u16 gMsgEventFlags[][2];
extern u8 D_008AFCB2[];
extern QuestMsgWin *gQuestMsgWindow;
Obj *Obj_Alloc(s32 n);
void AreaObj_K00_Update(Obj *o);
void AreaObj_K03_Update(Obj *o);
void AreaObj_K07_Update(Obj *o);
void AreaObj_K08_Update(Obj *o);
void AreaObj_K09_Update(Obj *o);
void AreaObj_K02_Update(Obj *o);
extern Panel gMsgEventPopupWin;
void AreaObj_K04_Update(Obj *o);
void Vec_Copy(f32 *dst, f32 *src);
extern s16 AreaObj_K02_Counts[];
extern u8 *gMsgEvents[];
extern char *gMsgEventTitles[];
extern char **gMsgEventLines[];
s32 MsgEvent_Check(u16);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetScale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
/* end of prototypes */

/* Frees the burst (ObjAttach_Free). */
void TeostraBlastRing_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Returns the slot (EmSlot_FindKind) of the first kind of
 * TeostraBlastRing_ModelKinds (Lunastra, then Teostra) that is loaded, or -1.
 */
s16 TeostraBlastRing_FindModelSlot(void)
{
    u8 *kinds = TeostraBlastRing_ModelKinds;
    u8 kind;
    s16 slot;

    for (kind = *kinds; kind != 0xFF; kind = *++kinds) {
        slot = EmSlot_FindKind(kind);
        if (slot != -1) {
            return slot;
        }
    }
    return -1;
}

/* draws a stage model of the common set (or of the bank TeostraBlastRing_FindModelSlot finds, kind 2) at a draw node
 * (d), for the object at +0x18 when it is in the current area */
void TeostraBlastRing_Draw(DrawSlot *drawSlot)
{
    u32 col;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    f32 uvMtx[16];
    ObjAttach *o;
    TeostraBlastRingBuf *ringBuf;
    Model *bank;
    u8 *matSet;
    u8 *scaleKeys;
    u8 *colorKeys;
    u16 flags;
    s16 i;
    s8 frame;

    bank = gStageModelSet;
    o = drawSlot->owner;
    ringBuf = (TeostraBlastRingBuf *) o->buf;
    flags = 0;
    if (o->area != gScene.area) {
        return;
    }
    switch (ringBuf->phase) {
    case 0:
        matSet = (u8 *) &bank->matSets[81];
        flags |= 2;
        scaleKeys = TeostraBlastRing_ScaleKeysStart;
        colorKeys = TeostraBlastRing_ColorKeysStart;
        break;
    case 1:
        matSet = (u8 *) &bank->matSets[78];
        frame = o->ownerMotion;
        flags |= 2;
        scaleKeys = TeostraBlastRing_ScaleKeysMiddle;
        colorKeys = TeostraBlastRing_ColorKeysMiddle;
        Mtx_Translation(uvMtx, 0.25f * (frame & 3), 0.25f * (frame / 4), 0.0f);
        Gs_SetState(0x19, (s32) uvMtx);
        break;
    case 2:
        i = TeostraBlastRing_FindModelSlot();
        if (i == -1) {
            return;
        }
        bank = gScene.partModels[i];
        flags |= 4;
        scaleKeys = TeostraBlastRing_ScaleKeysEnd;
        colorKeys = TeostraBlastRing_ColorKeysEnd;
        matSet = (u8 *) &bank->matSets[1];
        break;
    }
    if (bank != NULL && bank->active != 0) {
        bank = (Model *) bank->mats;
        rot[0] = 0.0f;
        rot[1] = 0.0f;
        rot[2] = 6.2831855f * ringBuf->rotZ / 65536.0f;
        Key_LerpVec3((f32 *) scaleKeys, (f32 *) scale, o->ownerMotion);
        scale[0] *= ringBuf->scale;
        scale[1] *= ringBuf->scale;
        Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Color_EvalKeys((s32 *) colorKeys, o->ownerMotion, &col);
        Gs_SetState(0x1A, (s32) mtx);
        Gs_SetState(0x67, col);
        Gs_SetState(0x6C, 0);
        Effect_DrawModel(matSet, mtx, col, flags, bank);
        Gs_SetState(0x6C, 1);
    }
}

/*
 * Returns a scatter value from bits: the low 16 - |16 - bitPos| bits, shifted
 * up by |16 - bitPos| and divided by 64000 (0 to about 1.02), negated when
 * the shift is even. TeostraBlastRing_Setup passes randBits times the burst's
 * number.
 */
f32 TeostraBlastRing_ScatterFromBits(u16 bits, u16 bitPos)
{
    s32 sign = -1;
    u16 shift = Math_Absf(16 - bitPos);
    u16 mask = 0xFFFF >> shift;

    if (shift & 1) {
        sign = 1;
    }
    return 0.000015625f * sign * ((bits & mask) << shift);
}

/*
 * Makes a ChameleosAttack of type for em, if a player is in em's area. Types
 * 4 and 7 face backward (yaw + 0x8000).
 */
void ChameleosAttack_Create(Em *em, s32 attackType)
{
    ObjAttach *o;

    if ((u8) Player_InCurrentArea(em) != 0 && (o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = ATTACH_CHAMELEOS_ATTACK;
        o->type = attackType;
        o->update = ChameleosAttack_Update;
        o->idx = em->index;
        o->isEm = em->isEm;
        o->ownerMotion = em->motionId[0];
        o->owner = em;
        o->yaw = em->rotY;
        switch (attackType) {
        case 4:
        case 7:
            *(u16 *) &o->yaw += 0x8000;
            break;
        }
        o->rotX = em->rotX;
        o->rotY = em->rotY;
        o->rotZ = em->rotZ;
        o->side = *(u8 *) &em->side;
        em->hitPending = 0;
        em->contactHit = 0;
    }
}

/*
 * The ChameleosAttack's per-frame update, by ObjAttach.state: a hit keeps it
 * running; states 4, 5, 7 and 9 end it.
 */
void ChameleosAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        ChameleosAttack_Setup(o);
        break;
    case ATTACH_RUN:
        ChameleosAttack_Run(o);
        break;
    case ATTACH_END:
        ChameleosAttack_End(o);
        break;
    case ATTACH_HIT:
        ChameleosAttack_Run(o);
        break;
    case 4:
        ChameleosAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        ChameleosAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        ChameleosAttack_End(o);
        break;
    case 7:
        ChameleosAttack_End(o);
        break;
    case ATTACH_FREE:
        ChameleosAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from attack row type: its hit shapes,
 * ATKF_OWNER_CONTACT, and hitDelay counted from the monster's motion (layer 1
 * for type 3, else layer 0).
 */
void ChameleosAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;
    s32 frame;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) em, o->type, &ChameleosAttack_RowsPtr);
    o->hitList = ChameleosAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        if (o->type == 3) {
            blend = Math_Absf((s16) em->motionBlend[1] % 100);
            frame = Math_Absf(em->motionFrame[1]);
        } else {
            blend = Math_Absf((s16) em->motionBlend[0] % 100);
            frame = Math_Absf(em->motionFrame[0]);
        }
        o->hitDelay += blend - frame - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Type 6 keeps the window open. Type 9 also survives motions 0x413 and 0x41D.
 * Otherwise the attack ends when the motion changes or the window closes.
 */
void ChameleosAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 6:
        o->hitFrames = 0x63;
        break;
    }
    switch (o->type) {
    case 9:
        if ((em->motionId[0] != 0x413 && em->motionId[0] != 0x41D && em->motionId[0] != o->ownerMotion) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    default:
        if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20, 0);
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot.
 * It is freed on the next update.
 */
void ChameleosAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void ChameleosAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a RemobraAttack of type for em, if a player is in em's area: 1 from
 * EmRemobra_State3_00, 2 and 3 from EmRemobra_State3_05.
 */
void RemobraAttack_Create(Em *em, u8 attackType)
{
    ObjAttach *o;

    if ((u8) Player_InCurrentArea(em) != 0 && (o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = ATTACH_HERMITAUR_ATTACK;
        o->type = attackType;
        o->update = RemobraAttack_Update;
        o->idx = em->index;
        o->isEm = em->isEm;
        o->ownerMotion = em->motionId[0];
        o->owner = em;
        o->yaw = em->rotY;
        o->rotX = em->rotX;
        o->rotY = em->rotY;
        o->rotZ = em->rotZ;
        o->side = *(u8 *) &em->side;
        em->hitPending = 0;
        em->contactHit = 0;
    }
}

/*
 * The RemobraAttack's per-frame update, by ObjAttach.state: a hit keeps it
 * running; states 4, 5, 7 and 9 end it.
 */
void RemobraAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        RemobraAttack_Setup(o);
        break;
    case ATTACH_RUN:
        RemobraAttack_Run(o);
        break;
    case ATTACH_END:
        RemobraAttack_End(o);
        break;
    case ATTACH_HIT:
        RemobraAttack_Run(o);
        break;
    case 4:
        RemobraAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        RemobraAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        RemobraAttack_End(o);
        break;
    case 7:
        RemobraAttack_End(o);
        break;
    case ATTACH_FREE:
        RemobraAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from attack row type at the monster's position (hitScale
 * 1.0): its hit shapes, ATKF_OWNER_CONTACT, and hitDelay counted from the
 * monster's motion.
 */
void RemobraAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) em, o->type, &RemobraAttack_RowsPtr);
    o->hitList = RemobraAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    Vec_Copy(o->pos, (f32 *)&em->posX);
    o->hitScale = 1.0f;
    if (o->hitDelay != 0) {
        o->hitDelay += (s32) Math_Absf((s16) em->motionBlend[0] % 100) - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or its hit window closes.
 */
void RemobraAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot.
 * It is freed on the next update.
 */
void RemobraAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void RemobraAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void Quest_CheckCampAdvice(void)
{
    Player *p = &gPlayers[gScene.localPlayer];

    if (p->area == gScene.campArea && gQuest.hunted[12] > 0) {
        MsgEvent_Check(0xE);
    }
}

void MsgEvent_Init(void)
{
    gQuestMsgWindow = &gQuestMsgWindowBuf;
    *(s16 *)&gQuestMsgWindow->listStep = 0;
    gQuestMsgWindow->unk02 = 0;
    gQuestMsgWindow->popupReq = 0;
    gQuestMsgWindow->popupStep = 0;
    switch (gQuest.stage) {
    case 0x3E9:
    case 0x43E:
    case 0x43F:
        gQuestMsgWindow->set = 0x12;
        break;
    case 0x1389:
        gQuestMsgWindow->set = 0x13;
        break;
    case 0x138A:
        gQuestMsgWindow->set = 0x14;
        break;
    case 0x138C:
        gQuestMsgWindow->set = 0x15;
        break;
    case 0x138D:
        gQuestMsgWindow->set = 0x16;
        break;
    case 0x138F:
        gQuestMsgWindow->set = 0x17;
        break;
    case 0x1772:
        gQuestMsgWindow->set = 0x18;
        break;
    default:
        gQuestMsgWindow->set = gQuest.stage - 1;
        break;
    }
    gQuestMsgWindow->count = MsgEvent_SetCount(gQuestMsgWindow->set);
}

void MsgEvent_Open(void)
{
    if (gQuestMsgWindow->popupReq != 0) {
        gQuestMsgWindow->popupReq = 0U;
        gQuestMsgWindow->popupStep = (u8) (gQuestMsgWindow->popupStep + 1);
        gQuestMsgWindow->page = 0;
        gQuestMsgWindow->pages = MsgEvent_Kind(gQuestMsgWindow->set, gQuestMsgWindow->msg);
        Game_Pause(0);
        Shake_Start(7U, 1U, 330.0f);
        Sound_PlaySeFlat(0, 0x10, 0);
    }
}

u8 MsgEvent_PopupInput(s32 pad)
{
    s32 btn;
    u8 sel;
    u8 temp_v1;
    u8 sel2;

    temp_v1 = gQuestMsgWindow->popupStep;
    switch (temp_v1) {
    case 0:
        break;
    case 1:
        if (Shake_Update(7U) != 1) {
            gQuestMsgWindow->popupStep = (u8) (gQuestMsgWindow->popupStep + 1);
        case 2:
            btn = pad & 0xFFFF;
            if (btn & 0x40) {
                gQuestMsgWindow->popupStep = (u8) (gQuestMsgWindow->popupStep + 1);
                Shake_Start(7U, 0x81U, 330.0f);
                Sound_PlaySeFlat(0, 0x14, 0);
            } else if (btn & 0x800) {
                sel = gQuestMsgWindow->page;
                if (sel != 0) {
                    gQuestMsgWindow->page = (u8) (sel - 1);
                    Sound_PlaySeFlat(0, 0x17, 0);
                }
            } else if (btn & 0x400) {
                sel2 = gQuestMsgWindow->page;
                if ((s32) sel2 < (gQuestMsgWindow->pages - 1)) {
                    gQuestMsgWindow->page = (u8) (sel2 + 1);
                    Sound_PlaySeFlat(0, 0x17, 0);
                }
            }
        }
        break;
    case 3:
        if (Shake_Update(7U) == 0) {
            gQuestMsgWindow->popupStep = 0U;
            Game_Resume(0);
        }
        break;
    }
    return gQuestMsgWindow->popupStep;
}

extern s8 gHelpShow;
s32 MsgEvent_OpenList(void)
{
    if (Quest_PhaseIsOver(0) != 1) {
        gQuestMsgWindow->listSub = 0;
        gQuestMsgWindow->listStep = 0;
        Game_Pause(0);
        gQuestMsgWindow->grid.index = 0;
        gQuestMsgWindow->grid.rows = 7;
        gQuestMsgWindow->grid.row = 0;
        gQuestMsgWindow->grid.col = 0;
        gQuestMsgWindow->grid.cols = (gQuestMsgWindow->count + 6) / 7;
        gQuestMsgWindow->grid.lastRows = gQuestMsgWindow->count - (gQuestMsgWindow->grid.cols - 1) * 7;
        gHelpShow = 0;
        Shake_Start(7, 1, 330.0f);
        return 0;
    }
    return 1;
}

s32 MsgEvent_ListInput(u16 pad)
{
    u16 trg;
    u8 *sub;

    switch (gQuestMsgWindow->listStep) {
    case 0:
        if (Shake_Update(7) == 1) {
            pad &= 0xBC40;
        }
        trg = pad;
        if (trg & 0x8000) {
            Shake_Start(7, 0x81, 330.0f);
            gQuestMsgWindow->listStep++;
            Sound_PlaySeFlat(0, 0x14, 0);
            break;
        }
        sub = &gQuestMsgWindow->listSub;
        switch (*sub) {
        case 0:
            if (trg & 0x40) {
                Game_Resume(0);
                gQuestMsgWindow->listSub = 2;
            } else if (gQuestMsgWindow->grid.cols != 0) {
                Menu_GridCursor(&gQuestMsgWindow->grid, pad);
                if (trg & 0x20) {
                    if (MsgEvent_IsSeen(gQuestMsgWindow->set, *(u8 *)&gQuestMsgWindow->grid.index) == 1) {
                        gQuestMsgWindow->page = 0;
                        gQuestMsgWindow->msg = gQuestMsgWindow->grid.index;
                        gQuestMsgWindow->pages = MsgEvent_Kind(gQuestMsgWindow->set, gQuestMsgWindow->msg);
                        gQuestMsgWindow->listSub++;
                        Sound_PlaySeFlat(0, 0x13, 0);
                    } else {
                        Sound_PlaySeFlat(0, 0x15, 0);
                    }
                }
            }
            break;
        case 1:
            if (trg & 0x40) {
                *sub = 0;
                Sound_PlaySeFlat(0, 0x14, 0);
            } else if (trg & 0x800) {
                if (gQuestMsgWindow->page != 0) {
                    gQuestMsgWindow->page--;
                    Sound_PlaySeFlat(0, 0x17, 0);
                }
            } else if (trg & 0x400) {
                if (gQuestMsgWindow->page < gQuestMsgWindow->pages - 1) {
                    gQuestMsgWindow->page++;
                    Sound_PlaySeFlat(0, 0x17, 0);
                }
            }
            break;
        case 2:
            return 0x40;
        }
        break;
    case 1:
        if (Shake_Update(7) == 0) {
            gQuestMsgWindow->listStep++;
            Game_Resume(0);
            return 0x8000;
        }
        break;
    }
    return 0;
}

void MsgEvent_DrawPopup(void)
{
    s16 *x = Shake_GetOffset(7);

    Window_DrawText(&gMsgEventPopupWin, x, 0);
    MsgEvent_DrawText(0, (s32) x);
    MsgEvent_DrawIcon(0, (s32) x);
}

/* The message strings of this unit (0x8DE300-0x8EC0A0), which only .data points to (e.g. the
 * table at 0x8AF3F0); they sit between RemobraAttack_Update's jump table and MsgEvent_DrawList's data. */
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE300);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE320);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE340);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE360);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE390);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE3C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE3E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE3F8);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE410);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE430);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE450);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE470);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE490);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE4B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE4C8);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE4E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE500);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE520);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE550);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE570);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE590);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE5B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE5E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE600);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE620);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE640);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE660);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE680);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE6A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE6B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE6D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE6E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE700);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE718);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE730);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE750);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE760);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE780);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE790);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE7A8);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE7C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE7D8);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE7F0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE810);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE830);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE850);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE870);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE890);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE8B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE8D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE8E8);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE8F8);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE910);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE930);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE940);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE960);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE970);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE990);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE9B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE9D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE9E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE9E8);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DE9F8);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEA08);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEA18);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEA30);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEA50);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEA70);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEA90);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEAB0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEAD0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEAE0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEB00);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEB20);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEB40);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEB58);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEB70);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEC20);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DECD0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DED80);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEE60);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEF30);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DEFE0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF0A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF170);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF1E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF2A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF360);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF400);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF4B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF580);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF660);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF730);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF7D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF880);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DF920);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DFA10);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DFAB0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DFB60);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DFC10);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DFD10);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DFDD0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DFE60);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008DFF30);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0040);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0100);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0190);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0290);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0320);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0400);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E04C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0580);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0620);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0710);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E07A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0850);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0930);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0A30);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0AE0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0B40);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0C20);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0CB0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0D60);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0E20);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0ED0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E0F90);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1070);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1130);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E11E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1260);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1310);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E13C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1480);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1530);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1610);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1690);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1750);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1810);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E18B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E19B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1A40);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1AE0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1B80);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1C00);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1C90);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1D30);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1DC0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1E80);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E1F60);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2040);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E20E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E21B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2240);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2300);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2360);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E23F0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2480);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2530);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2610);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E26D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E27B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2890);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2920);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E29D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2AA0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2B60);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2C50);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2CF0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2DA0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2E70);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2F70);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E2FF0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3090);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3120);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3200);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E32C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3380);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3420);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E34C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3550);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E35D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3670);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3770);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E37F0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E38B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3980);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3A30);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3AD0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3B70);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3C10);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3CD0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3D60);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3DE0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3EC0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E3F90);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4070);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4120);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E41E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E42B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4380);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4450);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E44E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E45A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4670);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4750);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4800);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4890);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4960);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4A20);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4AA0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4B80);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4C60);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4D50);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4E10);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4EA0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4F50);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E4FD0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5090);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5130);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E51E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E52C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E53A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5470);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5560);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E55E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5690);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5760);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5830);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5920);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E59E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5AA0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5B60);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5BE0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5CB0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5D90);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5E50);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E5F40);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6010);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E60F0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E61E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6270);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6340);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6410);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E64F0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E65C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6680);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6750);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E67F0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E68C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E69A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6A80);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6B50);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6C00);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6C90);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6D40);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6DD0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6E70);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6F40);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E6FE0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7060);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7130);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E71A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7230);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E72C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7390);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7450);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E74F0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E75C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E76A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7720);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E77E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E78C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E79A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7A50);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7B00);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7BA0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7C70);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7D10);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7DE0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7E90);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7F20);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E7FD0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8060);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8100);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8180);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8240);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8330);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8400);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E84D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8590);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8660);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8750);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E87E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E88D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8960);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8A20);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8B10);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8BE0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8C90);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8D20);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8DB0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8E70);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E8F20);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9000);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E90F0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E91C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E92A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9370);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9420);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E94D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E95A0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9680);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9700);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E97E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E98C0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9980);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9A00);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9AA0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9B30);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9C00);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9CC0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9DA0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9E90);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008E9F70);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA050);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA120);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA190);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA230);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA300);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA3E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA4B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA550);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA610);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA680);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA760);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA800);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA8F0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EA9B0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EAA70);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EAB30);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EABF0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EACB0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EAD50);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EAE10);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EAEA0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EAF30);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EAFB0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB030);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB0D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB190);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB260);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB300);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB3E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB440);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB500);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB570);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB5D0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB660);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB6F0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB770);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB800);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB850);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB8F0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB970);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EB9E0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EBA20);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EBA90);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EBB10);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EBBA0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EBC70);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EBD30);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EBDD0);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EBE90);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EBF50);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", D_008EC020);
INCLUDE_RODATA("asm/game/nonmatchings/unk_6EC680", gMsgEventListItems);

extern char *D_004469A8;
extern char *D_004469B0;
extern s8 *D_004469A4;
void MsgEvent_DrawList(void)
{
    s16 *w;
    s8 buf[0x20];
    s32 i;
    s16 y;
    u8 k;

    w = Shake_GetOffset(7);
    switch (gQuestMsgWindow->listStep) {
    case 0:
    case 1:
        switch (gQuestMsgWindow->listSub) {
        case 0:
        case 2:
            ListPanel_Draw(&gMsgEventListPanel, w, gMsgEventListItems, gQuestMsgWindow->grid.cols != 0 ? gQuestMsgWindow->grid.row : -1);
            MsgEvent_CopyQuestInfo(buf);
            i = Str_Length(buf);
            Font_SetColor(0x807F7F28);
            Font_SetPos(0x140 - i * 9 / 2, w[1] + 0x78);
            Text_Draw(buf);
            Font_SetColor(0x807F7F7F);
            if (gQuestMsgWindow->grid.cols > 1) {
                Font_SetPos(0x1CC, w[1] + 0x78);
                Text_Printf(D_004469B0, gQuestMsgWindow->grid.col + 1, gQuestMsgWindow->grid.cols);
                Menu_DrawFrame(&gMsgEventListPanel, w);
            }
            Font_SetPos(0x17E, w[1] + 0x128);
            Text_Printf(D_004469A8);
            y = w[1] + 0x8E;
            k = gQuestMsgWindow->grid.col * 7;
            for (i = 0; i < 7; i++) {
                Font_SetPos(0x7A, y);
                if (MsgEvent_IsSeen(gQuestMsgWindow->set, k) == 1) {
                    Font_SetColor(0x807F7F7F);
                    Text_PrintfColored(MsgEvent_Title(gQuestMsgWindow->set, k));
                } else {
                    Font_SetColor(0x80505050);
                    Text_Draw(D_004469A4);
                }
                k++;
                if (gQuestMsgWindow->count <= k) {
                    break;
                }
                y += 0x16;
            }
            break;
        case 1:
            Window_DrawText(&gMsgEventPopupWin, w, 0);
            MsgEvent_DrawText(1, (s32) w);
            MsgEvent_DrawIcon(0, (s32) w);
            break;
        }
        break;
    case 2:
        break;
    }
}

extern char *D_004469A0;
extern char *D_004469AC;
void MsgEvent_DrawText(u8 flag, s32 arg1)
{
    char *s;

    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x802B7F2B);
    s = MsgEvent_Title(gQuestMsgWindow->set, gQuestMsgWindow->msg);
    strlen(s);
    Font_SetPos(0x140 - strlen(s) * 9 / 2, *(s16 *) (arg1 + 2) + 0x81);
    Text_PrintfColored(s);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(0x7A, *(s16 *) (arg1 + 2) + 0xA0);
    Text_PrintfStyled(3, MsgEvent_Line(gQuestMsgWindow->set, gQuestMsgWindow->msg, gQuestMsgWindow->page));
    MsgEvent_DrawTextBg(0x12A, *(s16 *) (arg1 + 2) + 0x118);
    Font_SetPos(0x1C7, *(s16 *) (arg1 + 2) + 0x118);
    if (flag == 0) {
        Text_Printf(D_004469A0);
    } else {
        Text_Printf(D_004469AC);
    }
}

extern char *D_004469B4;
/* A page counter at (x, y): the text of 0x4469B4 with gQuestMsgWindow->page + 1 and gQuestMsgWindow->pages, and
 * the arrows to the previous and next pages, 3 or 8 pixels out by bit 4 of the frame counter
 * (gFrameTick, sub_main). */
void MsgEvent_DrawTextBg(s16 x, s16 y)
{
    SprRect r;
    s16 dx;
    int flash;
    s16 *py;
    s32 *col;
    s16 *uv0;
    s16 *uv1;

    Font_SetColor(0x807F7F7F);
    Font_SetPos(x, y);
    Text_PrintfColored(D_004469B4, gQuestMsgWindow->page + 1, gQuestMsgWindow->pages);
    if (gQuestMsgWindow->pages > 1) {
        flash = (gFrameTick & 0x10) == 0 ? (s16) 3 : (s16) 8;
        r.w = 0x12;
        r.h = 0x12;
        uv0 = (s16 *) &r.uv0;
        *(s32 *) uv0 = 0x6E0071;
        uv1 = (s16 *) &r.uv1;
        *(s32 *) uv1 = 0x7D007D;
        dx = flash;
        if (gQuestMsgWindow->page != 0) {
            r.x = (s16) (x - 0x12) - dx + 1;
            py = &r.y;
            col = &r.color;
            *py = y;
            *col = 0xFF203020;
            Sprite_Draw((struct Sprite *)&r);
            r.x -= 2;
            *py -= 1;
            *col = 0xFF18FF20;
            Sprite_Draw((struct Sprite *)&r);
        }
        if (gQuestMsgWindow->page != gQuestMsgWindow->pages - 1) {
            *uv0 = 0x7C;
            *uv1 = 0x70;
            py = &r.y;
            col = &r.color;
            r.x = (s16) (x - 0x12) + dx + 0x42;
            *py = y;
            *col = 0xFF203020;
            Sprite_Draw((struct Sprite *)&r);
            r.x -= 2;
            *py -= 1;
            *col = 0xFF18FF20;
            Sprite_Draw((struct Sprite *)&r);
        }
    }
}

void MsgEvent_DrawIcon(s32 unused, s32 arg1)
{
    SprRect r;

    Tex_Prepare(1, 0x147);
    Tex_Bind(0x147);
    r.x = 0x120;
    r.y = 0x38 - (s16) (s32) (0.37272727f * *(s16 *) (arg1 + 2));
    r.w = 0x40;
    r.h = 0x40;
    r.color = -1;
    if (gMsgEventIconShown[(s8) gQuestMsgWindow->set] == 0) {
        r.uv0 = 0xD3007F;
        r.uv1 = 0xFB00A7;
    } else {
        r.uv0 = 0xD30055;
        r.uv1 = 0xFB007D;
    }
    Sprite_Draw((struct Sprite *)&r);
}

void MsgEvent_CopyQuestInfo(s8 *buf)
{
    s32 left;
    s8 *info;
    s8 *dst;
    s8 c;

    dst = buf;
    info = QuestInfo_Get(0);
    left = 0x1E;
loop_1:
    c = *info;
    if (c == 0xA) {
        *dst = 0;
        return;
    }
    *dst = c;
    if (*info != 0) {
        left -= 1;
        info += 1;
        dst += 1;
        if (left <= 0) {
            *dst = 0;
        } else {
            goto loop_1;
        }
    }
}

void MsgEvent_Draw(void)
{
    MsgEvent_DrawList();
}

s32 MsgEvent_CheckList(MsgEvent *e);
s32 MsgEvent_Check(u16 kind)
{
    if (Game_IsOnline() == 1) {
        return 0;
    }
    if (Quest_HasAdvice() != 1) {
        return 0;
    }
    if (Quest_PhaseIsOver(0) == 1) {
        return 0;
    }
    gQuestMsgWindow->request = -1;
    switch (kind) {
    case 0:
        MsgEvent_CheckList(gMsgEventCheck0);
        break;
    case 1:
        MsgEvent_CheckList(gMsgEventCheck1);
        break;
    case 2:
        MsgEvent_CheckStageIntro();
        break;
    case 3:
        MsgEvent_CheckDay(0xCB, 1, 3);
        break;
    case 4:
        MsgEvent_CheckDay(0xCC, 1, 4);
        break;
    case 5:
        MsgEvent_CheckList(gMsgEventCheck5);
        break;
    case 6:
        MsgEvent_CheckDay(0xDC, 3, 4);
        break;
    case 7:
        MsgEvent_CheckDay(0xDB, 3, 3);
        break;
    case 8:
        MsgEvent_CheckList(gMsgEventCheck8);
        break;
    case 9:
        MsgEvent_CheckDay(0x107, 0x11, 1);
        break;
    case 10:
        MsgEvent_CheckDay(0xE2, 5, 1);
        break;
    case 11:
        MsgEvent_CheckDay(0x11B, 0x1772, 0);
        break;
    case 12:
        MsgEvent_CheckList(gMsgEventCheck12);
        break;
    case 13:
        MsgEvent_CheckDay(0xCF, 1, 7);
        break;
    case 14:
        MsgEvent_CheckList(gMsgEventCheck14);
        break;
    case 15:
        MsgEvent_CheckList(gMsgEventCheck15);
        break;
    case 16:
        MsgEvent_CheckDay(0x108, 0x11, 2);
        break;
    case 17:
        MsgEvent_CheckDay(0x113, 0x12, 9);
        break;
    case 18:
        MsgEvent_CheckDay(0x114, 0x12, 0xA);
        break;
    default:
        return 0;
    }
    if (gQuestMsgWindow->request >= 0) {
        gQuestMsgWindow->popupReq = 1;
        gQuestMsgWindow->msg = gQuestMsgWindow->request;
        return 1;
    }
    return 0;
}

s32 MsgEvent_CheckList(MsgEvent *e)
{
    s32 day;
    s16 flag = e->flag;

    if (flag >= 0) {
        day = gQuest.stage;
        do {
            if (e->day >= day) {
                if (e->day == day && Save_TestEventFlag(flag) != 1) {
                    Save_SetEventFlag(e->flag);
                    gQuestMsgWindow->request = e->msg;
                    if (e->ev[0] > 0) {
                        Save_SetEventFlag(e->ev[0] + 0xC8);
                    }
                    if (e->ev[1] > 0) {
                        Save_SetEventFlag(e->ev[1] + 0xC8);
                    }
                    if (e->ev[2] > 0) {
                        Save_SetEventFlag(e->ev[2] + 0xC8);
                    }
                    return 1;
                }
                break;
            }
            e++;
            flag = e->flag;
        } while (flag >= 0);
    }
    return 0;
}

s32 MsgEvent_CheckDay(s32 arg0, s32 stage, s8 arg2)
{
    s32 n;

    if ((stage & 0xFFFF) == gQuest.stage) {
        n = arg0 & 0xFFFF;
        if (Save_TestEventFlag(n) != 1) {
            Save_SetEventFlag(n);
            gQuestMsgWindow->request = arg2;
            return 1;
        }
    }
    return 0;
}

s32 MsgEvent_CheckStageIntro(void)
{
    switch (gQuest.stage) {
    case 1:
        if (Save_TestEventFlag(0xCA) != 1) {
            Save_SetEventFlag(0xCA);
            gQuestMsgWindow->request = 2;
            return 1;
        }
        break;
    case 18:
        if (Save_TestEventFlag(0x10B) != 1) {
            Save_SetEventFlag(0x10B);
            Save_SetEventFlag(0x10C);
            Save_SetEventFlag(0x10D);
            Save_SetEventFlag(0x10E);
            Save_SetEventFlag(0x10F);
            Save_SetEventFlag(0x110);
            Save_SetEventFlag(0x111);
            Save_SetEventFlag(0x112);
            gQuestMsgWindow->request = 1;
            return 1;
        }
        break;
    }
    return 0;
}

u8 MsgEvent_SetCount(u8 idx)
{
    return *(D_008AFCB2 + ((idx & 0xFF) * 4));
}

s32 MsgEvent_IsSeen(u8 idx, u8 k)
{
    return Save_TestEventFlag(k + gMsgEventFlags[idx][0]);
}

char *MsgEvent_Title(u8 a, u8 b)
{
    u8 *p = gMsgEvents[a];
    p += b + b;
    return gMsgEventTitles[*p];
}

char *MsgEvent_Line(u8 a, u8 b, u8 c)
{
    u8 *p = gMsgEvents[a];
    p += b + b;
    return gMsgEventLines[*p][c];
}

u8 MsgEvent_Kind(u8 a, u8 b)
{
    u8 *p = gMsgEvents[a];
    p += b + b;
    return p[1];
}

void AreaObj_K00_Create(void)
{
    Obj *obj = Obj_Alloc(0x1);

    if (obj != NULL) {
        obj->kind = 0;
        obj->update = AreaObj_K00_Update;
    }
}

void AreaObj_K00_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K00_Setup(o);
        break;
    case 1:
        AreaObj_K00_Run(o);
        break;
    case 2:
        AreaObj_K00_End(o);
        break;
    case 3:
        AreaObj_K00_Free(o);
        break;
    }
}

void AreaObj_K00_Setup(Obj *o)
{
    DrawSlot *ds;
    s16 n;
    s16 i;
    f32 *pos;
    ObjSlots6 *v;

    v = o->buf;
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        n = 1;
        pos = AreaObj_K00_PosArea29;
        break;
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        n = 2;
        pos = AreaObj_K00_PosArea2A;
        break;
    case AREA_FOREST_HILLS_12:
    case AREA_FOREST_HILLS_NIGHT_12:
        n = 1;
        pos = AreaObj_K00_PosArea2B;
        break;
    case AREA_SNOWY_MOUNTAINS_1:
    case AREA_SNOWY_MOUNTAINS_NIGHT_1:
        n = 1;
        pos = AreaObj_K00_PosArea5C;
        break;
    case AREA_SNOWY_MOUNTAINS_3:
    case AREA_SNOWY_MOUNTAINS_NIGHT_3:
        n = 1;
        pos = AreaObj_K00_PosArea5E;
        break;
    case AREA_TOWER_8:
        n = 2;
        pos = AreaObj_K00_PosArea82;
        break;
    case AREA_DESERT_NIGHT_4:
    case AREA_DESERT_4:
        n = 2;
        pos = AreaObj_K00_PosArea32;
        break;
    case AREA_SWAMP_NIGHT_3:
    case AREA_SWAMP_3:
        n = 2;
        pos = AreaObj_K00_PosArea44;
        break;
    case AREA_SWAMP_NIGHT_4:
    case AREA_SWAMP_4:
        n = 2;
        pos = AreaObj_K00_PosArea45;
        break;
    }
    for (i = 0; i < 6; i++, pos += 3) {
        if (i < n) {
            v->slot[i] = DrawSlot_Alloc();
            if (v->slot[i] != -1) {
                ds = DrawSlot_Get(v->slot[i]);
                v->ds[i] = ds;
                ds->owner = o;
                ds->part = i;
                ds->draw = AreaObj_K00_Draw;
                ds->pos[0] = pos[0];
                ds->pos[1] = pos[1];
                ds->pos[2] = pos[2];
            } else if (i == 0) {
                AreaObj_K00_Free(o);
                return;
            } else {
                v->ds[i] = NULL;
            }
        } else {
            v->ds[i] = NULL;
        }
    }
    AreaObj_K00_Run(o);
}

void AreaObj_K00_Run(Obj *o)
{
    void **b = o->buf;
    s16 lim;
    s16 i;

    o->timer++;
    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_1:
    case AREA_SNOWY_MOUNTAINS_3:
    case AREA_SNOWY_MOUNTAINS_NIGHT_1:
    case AREA_SNOWY_MOUNTAINS_NIGHT_3:
        lim = 0x200;
        break;
    default:
        lim = 100;
        break;
    }
    if (o->timer >= lim) {
        o->timer = 0;
    }
    for (i = 0; i < 6; i++) {
        if (b[i + 3] != NULL) {
            OT_AddByDepth(gDrawOtSorted, b[i + 3], 0x40, 0);
        }
    }
}

void AreaObj_K00_End(Obj *o)
{
    o->state++;
    o->visible = 0;
}

void AreaObj_K00_Free(Obj *o)
{
    Obj_Free(o);
}

void AreaObj_K00_Draw(DrawSlot *ds)
{
    Model *stage = gAreaSubModels[0];
    Obj *o = ds->owner;
    ModelMatSet *mdl;
    s32 n;
    f32 f;
    s16 rz;
    f32 m[16];
    f32 m2[16];

    if (stage != NULL && stage->active != 0) {
        Gs_SetState(0x6C, 0);
        Gs_SetState(0x60, 0);
        Gfx_SetBlend(4, 1);
        Mtx_Copy(m, gCamYawMtx);
        n = 100;
        switch (gScene.area) {
        case AREA_FOREST_HILLS_7:
        case AREA_FOREST_HILLS_NIGHT_7:
            Mtx_ScaleBy(m, AreaObj_K00_Scale[ds->part * 3], D_008B00D4[ds->part * 3], D_008B00D8[ds->part * 3]);
            rz = D_008B00CA[ds->part * 2];
            mdl = stage->matSets + 1;
            Mtx_RotateX(m, 6.2831855f * AreaObj_K00_RotX[ds->part * 2] / 360.0f);
            Mtx_RotateZ(m, 6.2831855f * rz / 360.0f);
            break;
        case AREA_FOREST_HILLS_11:
        case AREA_FOREST_HILLS_NIGHT_11:
            f = AreaObj_K00_DrawF[ds->part];
            Mtx_ScaleBy(m, f, 1.0f, f);
            mdl = stage->matSets;
            Mtx_RotateZ(m, 6.2831855f * AreaObj_K00_RotZA[ds->part] / 360.0f);
            break;
        case AREA_FOREST_HILLS_12:
        case AREA_FOREST_HILLS_NIGHT_12:
            mdl = stage->matSets;
            Mtx_RotateZ(m, 6.2831855f * AreaObj_K00_RotZB[ds->part] / 360.0f);
            break;
        case AREA_SNOWY_MOUNTAINS_1:
        case AREA_SNOWY_MOUNTAINS_NIGHT_1:
            Mtx_Identity(m);
            n = 0x200;
            mdl = stage->matSets + 3;
            break;
        case AREA_SNOWY_MOUNTAINS_3:
        case AREA_SNOWY_MOUNTAINS_NIGHT_3:
            Mtx_Identity(m);
            n = 0x200;
            mdl = stage->matSets + 3;
            break;
        case AREA_TOWER_8:
            mdl = stage->matSets + ds->part + 2;
            break;
        case AREA_DESERT_NIGHT_4:
        case AREA_DESERT_4:
            mdl = stage->matSets + AreaObj_K00_Models[ds->part];
            break;
        case AREA_SWAMP_NIGHT_3:
        case AREA_SWAMP_NIGHT_4:
        case AREA_SWAMP_3:
        case AREA_SWAMP_4:
            mdl = stage->matSets;
            break;
        }
        f = (1.0f / n) * o->timer;
        Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        Gs_SetState(0x1A, (s32) m);
        Mtx_Translation(m2, f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) m2);
        if (mdl->id != -1) {
            Model_Draw(mdl->id, 0);
        }
        Gfx_SetBlend(4, 5);
        Gs_SetState(0x6C, 1);
    }
}

void AreaObj_K02_Create(u8 a)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 2;
        obj->type = a;
        obj->update = AreaObj_K02_Update;
    }
}

void AreaObj_K02_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K02_Setup(o);
        break;
    case 1:
        AreaObj_K02_Run(o);
        break;
    case 2:
        AreaObj_K02_End(o);
        break;
    case 3:
        AreaObj_K02_Free(o);
        break;
    }
}

//---
void AreaObj_K02_Setup(Obj *o)
{
    ObjPart30 *r = o->buf;
    s16 n;
    s16 i;

    o->state++;
    o->var0 = 0;
    o->visible = 1;
    o->draw = NULL;
    o->var3 = 0;
    o->timer = 0;
    o->fvar = 0.0f;
    n = AreaObj_K02_Counts[o->type];
    for (i = 0; i < n; i++, r++) {
        r->unk00 = Rand_Next(1) % AreaObj_K02_RandRange[o->type];
        r->unk28 = 0;
        r->unk2A = 0;
        r->unk24 = 0;
        r->unk26 = AreaObj_K02_WaitBase[o->type] + (AreaObj_K02_WaitMask[o->type] & Rand_Next(1));
        r->unk04[0] = 0.0f;
        r->unk04[1] = 0.0f;
        r->unk04[2] = 0.0f;
        r->slot = DrawSlot_Alloc();
        if (r->slot != -1) {
            r->ds = DrawSlot_Get(r->slot);
            if (r->ds != NULL) {
                r->ds->owner = o;
                r->ds->part = i;
                r->ds->draw = AreaObj_K02_Draw;
            }
        } else {
            r->ds = NULL;
        }
    }
    AreaObj_K02_Run(o);
}

//---
void AreaObj_K02_Run(Obj *o)
{
    ObjPart30 *r = o->buf;
    s16 n = AreaObj_K02_Counts[o->type];
    s16 i;

    for (i = 0; i < n; i++, r++) {
        switch (r->unk28) {
        case 0:
            if (++r->unk24 >= r->unk26) {
                r->unk28++;
                r->unk24 = 0;
                r->unk00 = Rand_Next(1) % AreaObj_K02_RandRange[o->type];
            }
            break;
        case 1:
            if (++r->unk24 >= AreaObj_K02_Frames[o->type]) {
                r->unk28 = 0;
                r->unk24 = 0;
                r->unk26 = AreaObj_K02_WaitBase[o->type] + (AreaObj_K02_WaitMask[o->type] & Rand_Next(1));
            } else if (r->ds != NULL) {
                Vec_Copy((f32 *)r->ds->pos, (f32 *)r->unk04);
                OT_AddByDepth(gDrawOt8, r->ds, 8, 1);
            }
            break;
        }
    }
}

void AreaObj_K02_End(Obj *o)
{
    u8 *p = o->buf;
    s16 n;
    s16 i;

    o->state++;
    n = AreaObj_K02_Counts[o->type];
    for (i = 0; i < n; i++) {
        if (*(s32 *) (p + 0x1C) != 0) {
            DrawSlot_Free(*(s16 *) (p + 2));
        }
        p += 0x30;
    }
    o->visible = 0;
}

void AreaObj_K02_Free(Obj *o)
{
    Obj_Free(o);
}

void AreaObj_K02_Draw(DrawSlot *ds)
{
    Obj *o = ds->owner;
    Model *stage = gAreaSubModels[0];
    ObjPart30 *e = (ObjPart30 *) o->buf + ds->part;
    f32 m0[16];
    f32 m1[16];
    ModelMatSet *model;

    if (stage != NULL && stage->active != 0) {
        model = stage->matSets + AreaObj_K02_Models[o->type] + e->unk00;
        Mtx_Translation(m0, ds->pos[0], ds->pos[1], ds->pos[2]);
        Gs_SetState(0x1A, (s32) m0);
        Gs_SetState(0x60, 0);
        Gs_SetState(0x67, -1);
        Mtx_Translation(m1, 0.0f, (f32) e->unk24 / (f32) AreaObj_K02_Frames[o->type], 0.0f);
        Gs_SetState(0x19, (s32) m1);
        if (model != NULL && model->id != -1) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void AreaObj_K03_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x3;
        obj->type = 0;
        obj->update = AreaObj_K03_Update;
    }
}

void AreaObj_K03_CreateAt(void *arg)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 3;
        obj->type = 1;
        obj->update = AreaObj_K03_Update;
        Vec_Copy((f32 *)obj->pos, (f32 *)arg);
    }
}

void AreaObj_K03_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K03_Setup(o);
        break;
    case 1:
        AreaObj_K03_Run(o);
        break;
    case 2:
        AreaObj_K03_End(o);
        break;
    case 3:
        AreaObj_K03_Free(o);
        break;
    }
}

void AreaObj_K03_Setup(Obj *o)
{

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->var3 = 0;
    switch (o->type) {
    case 0:
        o->timer = (Rand_Next(1) & 0x1F) + 10;
        break;
    case 1:
        o->timer = 0;
        o->fvar = 0.0f;
        break;
    }
    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->ds = DrawSlot_Get(o->slot);
        o->ds->owner = o;
        o->ds->draw = AreaObj_K03_Draw;
        AreaObj_K03_Run(o);
    } else {
        Obj_Free(o);
    }
}

void AreaObj_K03_Run(Obj *o)
{
    f32 (*tbl)[3];
    f32 pos[3];

    switch (gScene.area) {
    case AREA_DESERT_NIGHT_10:
    case AREA_DESERT_10:
        tbl = (f32 (*)[3]) AreaObj_K03_Points;
        break;
    case AREA_DESERT_NIGHT_4:
    case AREA_SWAMP_NIGHT_3:
    case AREA_SWAMP_NIGHT_8:
    case AREA_TOWER_8:
    case AREA_DESERT_4:
    case AREA_SWAMP_3:
    case AREA_SWAMP_4:
    case AREA_SWAMP_8:
        AreaObj_K03_RunA(o);
        return;
    case AREA_FORTRESS_5:
    case AREA_CASTLE_1:
    case AREA_FORTRESS_NIGHT_5:
    case AREA_BATTLE_TOWN_1:
    case AREA_BATTLE_TOWN_NIGHT_1:
        AreaObj_K03_RunB(o);
        return;
    default:
        return;
    }
    switch (o->var0) {
    case 0:
        if (--o->timer <= 0) {
            o->var0++;
            o->var1 = Rand_Next(1) % 4;
            o->timer = 0;
        }
        break;
    case 1:
        if (++o->timer >= 100) {
            o->var0 = 0;
            o->timer = Rand_Next(1) & 0x3F;
        } else {
            o->ds->pos[0] = tbl[o->var1][0];
            o->ds->pos[1] = tbl[o->var1][1];
            o->ds->pos[2] = tbl[o->var1][2];
            if (o->timer == 2) {
                SoundObj_Start(7, 0x20, 1, o->ds->pos, 100, 0);
            }
            OT_AddByDepth(gDrawOtSorted, o->ds, 0x40, 0);
        }
        switch (100 - o->timer) {
        case 0xF:
        case 0x14:
        case 0x19:
        case 0x1E:
        case 0x23:
            pos[0] = o->ds->pos[0];
            pos[1] = -10.0f;
            pos[2] = o->ds->pos[2];
            Effect0D_SpawnAt(pos, 0, 1.75f);
            break;
        }
        break;
    }
}

void AreaObj_K03_RunA(Obj *o)
{
    f32 *r;
    f32 *sc;
    u8 i;
    s16 n;
    u16 a;
    f32 *pz;
    f32 p[3];
    f32 q[3];
    f32 h;

    switch (o->var0) {
    case 0:
        if (--o->timer <= 0) {
            a = Mtx_Heading(gCamMatrix) + 0x4000;
            p[0] = 0.0f;
            p[1] = 0.0f;
            pz = &p[2];
            *pz = -450 - (Rand_Next(1) & 0x3FF);
            a += (u16) ((Rand_Next(1) & 0x3FFF) - 0x1FFF);
            Vec_RotateY(p, 6.2831855f * a / 65536.0f);
            Mtx_GetTrans(q, gCamMatrix);
            p[0] += q[0];
            *pz += q[2];
            switch (gScene.area) {
            default:
            case AREA_TOWER_8:
                o->var2 = 5;
                n = 1;
                sc = AreaObj_K03_Sc82;
                r = AreaObj_K03_R82;
                break;
            case AREA_DESERT_NIGHT_4:
                o->var2 = 8;
                n = 1;
                sc = AreaObj_K03_Sc32;
                r = AreaObj_K03_R32;
                break;
            case AREA_SWAMP_NIGHT_3:
                n = 2;
                o->var2 = 2;
                sc = AreaObj_K03_Sc44;
                r = AreaObj_K03_R44;
                break;
            case AREA_SWAMP_NIGHT_8:
                o->var2 = 1;
                n = 2;
                sc = AreaObj_K03_Sc49;
                r = AreaObj_K03_R49;
                break;
            case AREA_DESERT_4:
                o->var2 = 8;
                n = 1;
                sc = AreaObj_K03_Sc90;
                r = AreaObj_K03_R90;
                break;
            case AREA_SWAMP_3:
                n = 2;
                o->var2 = 2;
                sc = AreaObj_K03_Sc44;
                r = AreaObj_K03_R44;
                break;
            case AREA_SWAMP_4:
                n = 2;
                o->var2 = 2;
                sc = AreaObj_K03_Sc9B;
                r = AreaObj_K03_R9B;
                break;
            case AREA_SWAMP_8:
                o->var2 = 1;
                n = 2;
                sc = AreaObj_K03_Sc49;
                r = AreaObj_K03_R49;
                break;
            }
            for (i = 0; i < n; i++) {
                if (!(Vec_Distance(p, sc) < *r) == 0) {
                    Vec_Copy(o->pos, (f32 *)p);
                    o->var0++;
                    o->timer = 0;
                    return;
                }
                sc += 3;
                r++;
            }
        }
        break;
    case 1:
        if (++o->timer >= 30) {
            switch (gScene.area) {
            default:
            case AREA_TOWER_8:
                o->var2 = 4;
                break;
            case AREA_DESERT_NIGHT_4:
            case AREA_DESERT_4:
                o->var2 = 5;
                break;
            case AREA_SWAMP_NIGHT_3:
            case AREA_SWAMP_3:
                o->var2 = 1;
                break;
            case AREA_SWAMP_4:
                o->var2 = 1;
                break;
            case AREA_SWAMP_NIGHT_8:
            case AREA_SWAMP_8:
                o->var2 = 0;
                break;
            }
            o->var0++;
            o->timer = 0;
            return;
        }
        h = 1000.0f - 33.333332f * o->timer;
        o->ds->pos[0] = o->pos[0];
        o->ds->pos[1] = o->pos[1] + h;
        o->ds->pos[2] = o->pos[2];
        OT_AddByDepth(gDrawOtSorted, o->ds, 0x40, 0);
        break;
    case 2:
        if (++o->timer >= 60) {
            o->var0 = 0;
            o->timer = Rand_Next(1) & 0x3F;
            return;
        }
        o->ds->pos[0] = o->pos[0];
        o->ds->pos[1] = o->pos[1];
        o->ds->pos[2] = o->pos[2];
        OT_AddByDepth(gDrawOt8, o->ds, 8, 0);
        break;
    }
}

//---
void AreaObj_K03_RunB(Obj *o)
{

    if (++o->timer >= 20) {
        o->state++;
        return;
    }
    o->fvar += 0.05f;
    if (o->fvar > 1.0f) {
        o->fvar = 1.0f;
    }
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOtSorted, o->ds, 0x40, 0);
}

void AreaObj_K03_End(Obj *o)
{
    if (o->ds != NULL) {
        DrawSlot_Free(o->slot);
    }
    o->state++;
    o->visible = 0;
}

void AreaObj_K03_Free(Obj *o)
{
    Obj_Free(o);
}

void AreaObj_K03_Draw(DrawSlot *ds)
{
    Obj *o;
    ModelMatSet *mdl;
    u8 flat;
    Model *stage;
    u8 a;
    f32 sc;
    f32 m[16];
    f32 m2[16];

    stage = gAreaSubModels[0];
    o = ds->owner;
    flat = 0;

    if (stage != NULL && stage->active != 0) {
        Gs_SetState(0x60, 0);
        Gs_SetState(0x67, -1);
        Mtx_Translation(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        switch (gScene.area) {
        case AREA_DESERT_NIGHT_10:
        case AREA_DESERT_10:
            mdl = stage->matSets + 1;
            Mtx_Translation(m2, 0.0f, 1.0f - o->timer / 100.0f, 0.0f);
            break;
        case AREA_DESERT_NIGHT_4:
        case AREA_SWAMP_NIGHT_3:
        case AREA_SWAMP_NIGHT_8:
        case AREA_TOWER_8:
        case AREA_DESERT_4:
        case AREA_SWAMP_3:
        case AREA_SWAMP_4:
        case AREA_SWAMP_8:
            mdl = stage->matSets + o->var2;
            if (o->var0 == 1) {
                a = 8.5f * o->timer;
            } else {
                sc = 0.016666668f * o->timer;
                Mtx_SetScale(m, sc, 1.0f, sc);
                Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
                a = 0xFF - (u8) (4.25f * o->timer);
                Mtx_Translation(m2, 0.0f, 0.016666668f * o->timer, 0.0f);
            }
            Gs_SetState(0x67, (a << 24) | 0xFFFFFF);
            break;
        case AREA_BATTLE_TOWN_1:
        case AREA_BATTLE_TOWN_NIGHT_1:
            Mtx_Translation(m2, 0.0f, o->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 5;
            Gs_SetState(0x6C, 0);
            Gs_SetState(0x67, ((u8) (0xFF - (u8) (12.75f * o->timer)) << 24) | 0xFFFFFF);
            break;
        case AREA_FORTRESS_5:
            Mtx_Translation(m2, 0.0f, o->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 7;
            Gs_SetState(0x6C, 0);
            Gs_SetState(0x67, ((u8) (0xFF - (u8) (12.75f * o->timer)) << 24) | 0xFFFFFF);
            break;
        case AREA_FORTRESS_NIGHT_5:
            Mtx_Translation(m2, 0.0f, o->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 8;
            Gs_SetState(0x6C, 0);
            Gs_SetState(0x67, ((u8) (0xFF - (u8) (12.75f * o->timer)) << 24) | 0xFFFFFF);
            break;
        case AREA_CASTLE_1:
            Mtx_RotateY(m, 0.45378563f);
            Mtx_Translation(m2, 0.0f, o->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 15;
            Gs_SetState(0x6C, 0);
            Gs_SetState(0x67, ((u8) (0xFF - (u8) (12.75f * o->timer)) << 24) | 0xFFFFFF);
            break;
        }
        if (flat == 0) {
            Mtx_Concat3x3(m, gCamMatrix);
        }
        Gs_SetState(0x19, (s32) m2);
        Gs_SetState(0x1A, (s32) m);
        if (mdl != NULL && mdl->id != -1) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
        Gs_SetState(0x6C, 1);
    }
}

void AreaObj_K04_Create(void)
{
    Obj *obj;

    if ((gScene.area == AREA_FORTRESS_3 || gScene.area == AREA_FORTRESS_NIGHT_3) && (gScene.emEventFlags & 1)) {
        return;
    }
    obj = Obj_Alloc(0);
    if (obj != NULL) {
        obj->kind = 4;
        obj->update = AreaObj_K04_Update;
    }
}

void AreaObj_K04_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K04_Setup(o);
        break;
    case 1:
        AreaObj_K04_Run(o);
        break;
    case 2:
        AreaObj_K04_End(o);
        break;
    case 3:
        AreaObj_K04_Free(o);
        break;
    }
}

//---
void AreaObj_K04_Setup(Obj *o)
{

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    o->var3 = 0;
    switch (gScene.area) {
    case AREA_FORTRESS_1:
    case AREA_FORTRESS_NIGHT_1:
        o->pos[0] = 12680.0f;
        o->pos[1] = 696.0f;
        o->pos[2] = 12756.0f;
        break;
    case AREA_FORTRESS_3:
    case AREA_FORTRESS_NIGHT_3:
        o->pos[0] = 10400.0f;
        o->pos[1] = 1300.0f;
        o->pos[2] = 20570.0f;
        break;
    case AREA_ARENAS_NIGHT_0:
    case AREA_ARENAS_0:
        o->pos[0] = 10007.0f;
        o->pos[1] = 200.0f;
        o->pos[2] = 9377.0f;
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
    case AREA_GREAT_ARENA_1:
        o->pos[0] = 5129.0f;
        o->pos[1] = 1513.0f;
        o->pos[2] = 10935.0f;
        break;
    case AREA_ARENA_NIGHT_1:
    case AREA_ARENA_1:
        o->pos[0] = 9908.0f;
        o->pos[1] = 2957.0f;
        o->pos[2] = 15082.0f;
        break;
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        o->pos[0] = 11600.0f;
        o->pos[1] = 4460.0f;
        o->pos[2] = 7610.0f;
        break;
    default:
        Obj_Free(o);
        return;
    }
    o->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (o->ds != NULL) {
        o->ds->owner = o;
        o->ds->draw = AreaObj_K04_Draw;
        Vec_Copy(o->ds->pos, o->pos);
        AreaObj_K04_Run(o);
    } else {
        Obj_Free(o);
    }
}

//---
void AreaObj_K04_Run(Obj *o)
{
    s16 lo;
    s16 hi;

    o->var3++;
    switch (gScene.area) {
    case AREA_FORTRESS_1:
        lo = 2;
        hi = 0x11;
        break;
    case AREA_FORTRESS_NIGHT_1:
        lo = 3;
        hi = 0x12;
        break;
    case AREA_FORTRESS_3:
    case AREA_FORTRESS_NIGHT_3:
        if (gScene.emEventFlags & 1) {
            return;
        }
        lo = 4;
        hi = 0x13;
        break;
    case AREA_ARENAS_NIGHT_0:
    case AREA_ARENAS_0:
        lo = 1;
        hi = 0x10;
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
    case AREA_GREAT_ARENA_1:
        lo = 6;
        hi = 0x15;
        break;
    case AREA_ARENA_NIGHT_1:
    case AREA_ARENA_1:
        lo = 1;
        hi = 0x10;
        break;
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        switch (QuestGoal_DeliveryDone()) {
        case 0:
            return;
        case 1:
            lo = 0xD;
            hi = 0x1C;
            break;
        case 2:
            lo = 0x1D;
            hi = 0x2C;
            break;
        }
        o->pos[1] += 1.95f;
        if (o->pos[1] > 4850.0f) {
            o->pos[1] = 4850.0f;
        }
        break;
    default:
        return;
    }
    if (++o->timer >= hi - lo + 1) {
        o->timer = 0;
    }
    if (o->var3 % 55 == 0) {
        Sound_PlaySeAtPosChecked(7, 0x52, 0, o->pos, 0x11, 0);
    }
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
}

void AreaObj_K04_End(Obj *o)
{
    o->state++;
    o->visible = 0;
}

void AreaObj_K04_Free(Obj *o)
{
    Obj_Free(o);
}

void AreaObj_K04_Draw(DrawSlot *ds)
{
    Obj *o = ds->owner;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *mdl;
    s16 k;
    f32 m[16];

    if (stage != NULL && stage->active != 0) {
        Mtx_Translation(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        switch (gScene.area) {
        case AREA_FORTRESS_1:
            Mtx_RotateY(m, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (o->var3 << 8) / 65536.0f)) / 360.0f);
            k = 2;
            break;
        case AREA_FORTRESS_NIGHT_1:
            Mtx_RotateY(m, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (o->var3 << 8) / 65536.0f)) / 360.0f);
            k = 3;
            break;
        case AREA_FORTRESS_3:
        case AREA_FORTRESS_NIGHT_3:
            k = 4;
            break;
        case AREA_ARENAS_NIGHT_0:
        case AREA_ARENAS_0:
            Mtx_RotateY(m, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (o->var3 << 8) / 65536.0f)) / 360.0f);
            k = 1;
            break;
        case AREA_GREAT_ARENA_NIGHT_1:
        case AREA_GREAT_ARENA_1:
            Mtx_RotateY(m, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (o->var3 << 8) / 65536.0f)) / 360.0f);
            k = 6;
            break;
        case AREA_ARENA_NIGHT_1:
        case AREA_ARENA_1:
            Mtx_RotateY(m, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (o->var3 << 8) / 65536.0f)) / 360.0f);
            k = 1;
            break;
        case AREA_SNOWY_MOUNTAINS_6:
        case AREA_SNOWY_MOUNTAINS_NIGHT_6:
            Mtx_RotateY(m, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (o->var3 << 8) / 65536.0f)) / 360.0f);
            k = 13;
            break;
        default:
            return;
        }
        mdl = stage->matSets + k + o->timer;
        Gs_SetState(0x1A, (s32) m);
        if (mdl != NULL && mdl->id != -1) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void AreaObj_K05_Create(u8 kind)
{
    u8 n;
    u8 i;
    Obj *obj;

    switch (kind) {
    case 0:
    case 2:
    case 4:
    case 5:
    case 7:
    case 8:
        n = 3;
        break;
    case 1:
    case 6:
        n = 5;
        break;
    case 3:
        n = 2;
        break;
    case 9:
        n = 4;
        break;
    default:
        n = 0;
        break;
    }
    for (i = 0; i < n; i++) {
        obj = Obj_Alloc(0);
        if (obj != NULL) {
            obj->kind = 5;
            obj->type = kind;
            obj->update = AreaObj_K05_Update;
            obj->var2 = i;
        }
    }
}

void AreaObj_K05_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K05_Setup(o);
        break;
    case 1:
        AreaObj_K05_Run(o);
        break;
    case 2:
        AreaObj_K05_End(o);
        break;
    case 3:
        AreaObj_K05_Free(o);
        break;
    }
}

void AreaObj_K05_Setup(Obj *o)
{
    u8 t = AreaObj_K05_Kinds[o->type][o->var2];
    f32 *f;

    switch (o->type) {
    case 1:
    case 6:
        f = AreaObj_K05_Frames6[o->var2];
        break;
    case 2:
        f = AreaObj_K05_Frames2[o->var2];
        break;
    case 8:
        f = AreaObj_K05_Frames8[o->var2];
        break;
    case 9:
        f = AreaObj_K05_Frames9[o->var2];
        break;
    default:
        f = AreaObj_K05_Params[o->type] + o->var2 * 3;
        break;
    }
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    o->var3 = 0;
    if (t == 2) {
        if (gScene.countdownOn == 0) {
            o->var0 = 0;
        } else {
            o->timer = gQuest.countdown;
            o->var0 = 5;
        }
    }
    o->pos[0] = *f++;
    o->pos[1] = *f++;
    o->pos[2] = *f;
    o->slot = DrawSlot_Alloc();
    o->ds = DrawSlot_Get(o->slot);
    if (o->ds != NULL) {
        o->ds->owner = o;
        o->ds->draw = AreaObj_K05_Draw;
        Vec_Copy(o->ds->pos, o->pos);
        AreaObj_K05_Run(o);
    } else {
        Obj_Free(o);
    }
}

s32 Obj_InCurrentArea(void *obj);
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
s32 Player_IsLocal(void *obj);
/* unit's own declaration: with the definition's void (f32 *pos, u8 kind, u8 area, s32 ang) (game/unk_6D3B50.c), AreaObj_K05_Run: 3 of 681 words differ */
void StageShot_CreateAt(f32 *pos, u8 code, u8 area, u16 ang);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
extern u16 AreaObj_K05_Sounds[];
extern u16 AreaObj_K05_Unk008_8[];
extern f32 AreaObj_K05_SpawnPos6[];
extern f32 D_008B037C[];
extern f32 D_008B0388[];
extern f32 D_008B0394[];
extern f32 AreaObj_K05_SpawnPos2[];
extern f32 AreaObj_K05_SpawnPos8[];
extern f32 D_008B04BC[];
extern s32 gQuestCountdown;
/* K05 area object: in state 2 it counts up and, by step (unk005), creates
 * the K03 objects and K0E effects of its type (8; 1 and 6; 2) and an StageShot
 * effect; in state 1 it follows the players on it (motion 0x19E) with its
 * sound. The gScene.countdownOn test is a default-first switch: MWCC then leaves
 * the compare branches' slots empty in the whole function. */
void AreaObj_K05_Run(Obj *o)
{
    Player *p;
    u8 *f;
    s16 i;
    u8 local;
    u16 ang;
    u8 code;
    u8 j;
    Player *me;
    f32 *dy;
    f32 *dz;
    f32 d[3];
    f32 v[3];
    f32 w[3];
    u8 flags[4];

    dy = &d[1];
    local = 0;
    dz = &d[2];
    d[0] = 0.0f;
    *dy = 0.0f;
    *dz = 0.0f;
    p = gPlayers;
    switch (AreaObj_K05_Kinds[o->type][o->var2]) {
    case 2:
        o->timer++;
        switch (o->type) {
        case 1:
        case 6:
            ang = 0;
            code = 2;
            break;
        case 2:
            ang = AreaObj_K05_Angles2[o->var2];
            code = 3;
            break;
        case 8:
            code = 8;
            ang = 0x8000;
            break;
        default:
            ang = 0;
            code = 0xFF;
            break;
        }
        switch (o->var0) {
        case 0:
            switch (gScene.countdownOn) {
            default:
                o->var0++;
                o->timer = 0;
                break;
            case 0:
                break;
            }
            break;
        case 1:
            if (o->timer > 0x2D) {
                o->timer = 0;
                switch (o->type) {
                case 8:
                    AreaObj_K03_CreateAt(AreaObj_K05_SpawnPos8);
                    AreaObj_K03_CreateAt(D_008B04BC);
                    Vec_Copy((f32 *)v, (f32 *)AreaObj_K05_SpawnPos8);
                    v[2] += 100.0f;
                    StageObj_K0E_CreateScaled(v, 8, 2.0f);
                    Vec_Copy((f32 *)v, (f32 *)D_008B04BC);
                    v[2] += 100.0f;
                    StageObj_K0E_CreateScaled(v, 8, 2.0f);
                    break;
                case 1:
                case 6:
                    AreaObj_K03_CreateAt(AreaObj_K05_SpawnPos6);
                    AreaObj_K03_CreateAt(D_008B037C);
                    AreaObj_K03_CreateAt(D_008B0388);
                    AreaObj_K03_CreateAt(D_008B0394);
                    Vec_Copy((f32 *)v, (f32 *)AreaObj_K05_SpawnPos6);
                    v[2] += 100.0f;
                    StageObj_K0E_CreateScaled(v, 8, 1.0f);
                    Vec_Copy((f32 *)v, (f32 *)D_008B037C);
                    v[2] += 100.0f;
                    StageObj_K0E_CreateScaled(v, 8, 1.0f);
                    Vec_Copy((f32 *)v, (f32 *)D_008B0388);
                    v[2] += 100.0f;
                    StageObj_K0E_CreateScaled(v, 8, 1.0f);
                    Vec_Copy((f32 *)v, (f32 *)D_008B0394);
                    v[2] += 100.0f;
                    StageObj_K0E_CreateScaled(v, 8, 1.0f);
                    break;
                case 2:
                    AreaObj_K03_CreateAt(AreaObj_K05_SpawnPos2);
                    Vec_Copy((f32 *)v, (f32 *)AreaObj_K05_SpawnPos2);
                    w[2] = 100.0f;
                    w[0] = 0.0f;
                    w[1] = 0.0f;
                    Vec_RotateY(w, 0.45378563f);
                    Vec_Add(v, v, w);
                    StageObj_K0E_CreateScaled(v, 8, 2.0f);
                    break;
                }
                o->var0++;
                if (code != 0xFF) {
                    switch (o->type) {
                    case 8:
                        StageShot_CreateAt(o->ds->pos, code, gScene.area, ang);
                        break;
                    default:
                        StageShot_CreateAt(o->pos, code, gScene.area, ang);
                        break;
                    }
                }
            }
            break;
        case 2:
            o->var1++;
            *dz = 1300.0f * (o->timer / 4.0f);
            if (o->timer >= 4) {
                switch (o->type) {
                case 8:
                    Sound_PlaySeAtPosChecked(7, 0x36, 0, o->ds->pos, 7, 0);
                    break;
                default:
                    Sound_PlaySeAtPosChecked(7, 0x36, 0, o->pos, 7, 0);
                    break;
                }
                o->timer = 0;
                o->var0++;
            }
            break;
        case 3:
            o->var1++;
            *dz = 1300.0f;
            if (o->timer >= 90) {
                o->timer = 0;
                o->var0++;
            }
            break;
        case 4:
            o->var1++;
            *dz = 1300.0f - 1300.0f * (o->timer / 150.0f);
            if (o->timer >= 150) {
                o->timer = gQuestCountdown;
                o->var0 = 5;
            }
            break;
        case 5:
            if (o->timer < gQuestCountdown) {
                o->var0 = 1;
                o->timer = 0;
            } else {
                if (gScene.countdownOn == 0) {
                    o->var0 = 0;
                }
                o->timer = gQuestCountdown;
            }
            break;
        }
        Vec_RotateY(d, 6.2831855f * ang / 65536.0f);
        switch (o->type) {
        case 8:
            Vec_Copy(o->pos, (f32 *)d);
            OT_AddByDepth(gDrawOt32, o->ds, 0x20, 1);
            return;
        }
        break;
    case 1:
        if (o->var1 > 0) {
            o->var1--;
        }
        for (i = 0; i < 4; i++, p++) {
            flags[i] = 0;
            if (p->active && p->visible && (u8) Obj_InCurrentArea(p) && p->motionId[0] == 0x19E &&
                Motion_ReachedFrame(p, 0, 66.0f)) {
                flags[i] = 1;
                if (Player_IsLocal(p)) {
                    local = 1;
                }
            }
        }
        if (local) {
            d[0] = o->pos[0];
            me = &gPlayers[gScene.localPlayer];
            d[1] = me->pos[1];
            d[2] = o->pos[2];
            if (Vec_Distance(me->pos, d) <= 200.0f) {
                if (o->var1 == 0 && (u16) o->timer != me->rot[1]) {
                    Sound_PlaySeAtPosChecked(7, AreaObj_K05_Sounds[(u8) AreaObj_K05_SoundIdx(o)], 0, o->pos, 1, 0);
                    o->var1 = 10;
                }
                o->var3 = me->aimPitchPct;
                o->timer = me->rot[1];
                o->var4 = gScene.localPlayer;
                goto done;
            }
        }
        p = gPlayers;
        for (i = 0; i < 4; i++, p++) {
            if (flags[i] && p->active && p->visible && (u8) Obj_InCurrentArea(p) && !Player_IsLocal(p)) {
                d[0] = o->pos[0];
                d[1] = p->pos[1];
                d[2] = o->pos[2];
                if (Vec_Distance(p->pos, d) <= 200.0f) {
                    if (o->var1 == 0 && (u16) o->timer != p->rot[1]) {
                        Sound_PlaySeAtPosChecked(7, AreaObj_K05_Sounds[(u8) AreaObj_K05_SoundIdx(o)], 0, o->pos, 1, 0);
                        o->var1 = 10;
                    }
                    o->var3 = p->aimPitchPct;
                    o->timer = p->rot[1];
                    o->var4 = i;
                    goto done;
                }
            }
        }
        o->var3 = 0;
        switch (o->type) {
        case 1:
        case 6:
        case 9:
            o->timer = 0;
            break;
        case 2:
            o->timer = AreaObj_K05_Angles2[o->var2];
            break;
        case 8:
            o->timer = AreaObj_K05_Unk008_8[o->var2];
            break;
        }
    done:
        d[0] = 0.0f;
        *dy = 0.0f;
        *dz = 0.0f;
        break;
    }
    o->ds->pos[0] = o->pos[0] + d[0];
    o->ds->pos[1] = o->pos[1] + *dy;
    o->ds->pos[2] = o->pos[2] + *dz;
    OT_AddByDepth(gDrawOt32, o->ds, 0x20, 1);
}

void AreaObj_K05_End(Obj *o)
{
    o->state++;
    o->visible = 0;
    DrawSlot_Free(o->slot);
}

void AreaObj_K05_Free(Obj *o)
{
    Obj_Free(o);
}

/* The draw callback of the stage objects that AreaObj_K05_Setup sets up, drawn only in their day
 * variant of the area (AreaObj_K05_AreaVariants): up to two models (AreaObj_K05_Models), turned by the kind; kind 1
 * tilts its second model by unk00A. */
void AreaObj_K05_Draw(DrawSlot *ds)
{
    Obj *o = ds->owner;
    Model *stage = gAreaSubModels[0];
    u8 k = o->type;
    u16 day = AreaObj_K05_AreaVariants[k];
    u8 t;
    f32 *py;
    f32 *pz;
    ModelMatSet *mdl;
    s16 *id;
    s16 j;
    s32 off;
    s32 kind;
    f32 ry;
    u16 r;
    f32 turn;
    u8 *src;
    f32 p[3];
    f32 v[3];
    f32 m[16];
    f32 m2[16];

    if (day == Area_DayId(gScene.area) && stage != NULL && stage->active != 0) {
        t = AreaObj_K05_Kinds[k][o->var2];
        id = AreaObj_K05_Models[t] + o->type * 2;
        switch (o->type) {
        case 2:
            ry = 6.2831855f * AreaObj_K05_Angles2[o->var2] / 65536.0f;
            break;
        default:
            ry = 0.0f;
            break;
        }
        kind = t;
        for (j = 0, off = 0, py = &p[1], pz = &p[2]; j < 2; j++, off += 0xC, id++) {
            if (*id == -1) {
                break;
            }
            if (kind == 1) {
                Vec_Copy((f32 *)p, (f32 *)ds->pos);
                if (j == 1) {
                    v[0] = 0.0f;
                    v[1] = 100.0f;
                    v[2] = 75.0f;
                    Vec_RotateY(v, 6.2831855f * o->timer / 65536.0f);
                    p[0] += v[0];
                    *py += v[1];
                    *pz += v[2];
                }
            } else if (o->type == 8) {
                src = ((u8 **) AreaObj_K05_Params[o->type])[o->var2];
                src += off;
                Vec_Copy((f32 *)p, (f32 *)src);
                *pz += o->pos[2];
            } else {
                Vec_Copy((f32 *)p, (f32 *)ds->pos);
            }
            Mtx_Translation(m, p[0], *py, *pz);
            switch (kind) {
            case 2:
                switch (o->type) {
                case 1:
                case 6:
                    Mtx_Identity(m2);
                    Mtx_SetTrans(m2, 0.0f, 1.0f - (o->var1 & 7) / 8.0f, 0.0f);
                    Gs_SetState(0x19, (s32) m2);
                    break;
                default:
                case 2:
                    Mtx_RotateZ(m, 6.2831855f * (o->var1 << 13) / 65536.0f);
                    break;
                case 8:
                    if (o->var0 == 2) {
                        Mtx_RotateZ(m, 6.2831855f * (360.0f * (o->timer / 4.0f)) / 360.0f);
                    }
                    break;
                }
                Mtx_RotateY(m, ry);
                break;
            case 1:
                if (j == 1) {
                    if (o->var3 >= 0) {
                        turn = 65536.0f;
                        r = (s16) (u16) (s32) (0.5f + turn * (0.01f * (45.0f * o->var3)) / 360.0f);
                    } else {
                        turn = 65536.0f;
                        r = (s16) (u16) (s32) (0.5f + turn * (0.01f * (35.0f * o->var3)) / 360.0f);
                    }
                    Mtx_RotateX(m, 6.2831855f * -r / turn);
                }
                Mtx_RotateY(m, 6.2831855f * o->timer / 65536.0f);
                break;
            default:
                Mtx_RotateY(m, ry);
                break;
            }
            Gs_SetState(0x1A, (s32) m);
            mdl = stage->matSets + *id;
            if (mdl != NULL && mdl->id != -1) {
                Draw_ApplyStates(mdl->flags);
                Model_Draw(mdl->id, 0);
            }
        }
        Draw_SetDefaultState();
    }
}

s32 AreaObj_K05_SoundIdx(Obj *o)
{
    s32 day;
    u8 base;

    day = Area_DayId(gScene.area) & 0xFFFF;
    switch (day) {
    case 12:
        base = 2;
        break;
    case 25:
        base = 0;
        break;
    default:
        base = 0;
        break;
    }
    return (o->var2 - base) & 0xFF;
}

void AreaObj_K07_Create(void)
{
    Obj *obj = Obj_Alloc(0x1);

    if (obj != NULL) {
        obj->kind = 0x7;
        obj->type = 0;
        obj->update = AreaObj_K07_Update;
    }
}

void AreaObj_K07_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K07_Setup(o);
        break;
    case 1:
        AreaObj_K07_Run(o);
        break;
    case 2:
        AreaObj_K07_End(o);
        break;
    case 3:
        AreaObj_K07_Free(o);
        break;
    }
}

static s32 AreaObj_K07_Select3(s16 k, s32 a, s32 b, s32 c)
{
    switch (k) {
    default:
        return a;
    case 1:
        return b;
    case 2:
        return c;
    }
}

/* The set-up state of the parts object of AREA_FOREST_HILLS_11, AREA_SNOWY_MOUNTAINS_6, AREA_SNOWY_MOUNTAINS_NIGHT_5, AREA_SNOWY_MOUNTAINS_NIGHT_6, AREA_SNOWY_MOUNTAINS_NIGHT_8, AREA_BATTLE_TOWN_0 and AREA_BATTLE_TOWN_NIGHT_0 (its
 * update, AreaObj_K07_Run, runs them): n parts placed from the area table, each with its state and
 * level, and draw slots (AreaObj_K07_Draw) for up to m of them. */
/* Its object as a void * beside the Obj view: the object kept on the stack is the view, as in the original. */
void AreaObj_K07_Setup(void *arg)
{
    Obj *o = (Obj *) arg;
    s16 lv;
    s16 cnt;
    s16 n;
    s16 m;
    f32 *pos;
    DrawSlot *ds;
    s16 i;
    u8 skip;
    ObjParts13 *l = o->buf;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->var3 = 0;
    cnt = 0;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        n = m = 8;
        pos = AreaObj_K07_PosArea2A;
        break;
    case AREA_SNOWY_MOUNTAINS_6:
        n = 6;
        m = 2;
        pos = AreaObj_K07_PosArea61;
        lv = Area_MonsterState(gScene.area);
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
        m = 1;
        n = 4;
        pos = AreaObj_K07_PosArea69;
        lv = Area_MonsterState(gScene.area);
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        n = 7;
        m = 2;
        pos = AreaObj_K07_PosArea6A;
        lv = Area_MonsterState(gScene.area);
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        m = 1;
        n = 3;
        pos = AreaObj_K07_PosArea6D;
        lv = Area_MonsterState(gScene.area);
        break;
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        n = 13;
        m = n;
        pos = AreaObj_K07_PosAreaB3;
        lv = Area_MonsterState(gScene.area);
        break;
    default:
        AreaObj_K07_Free(o);
        return;
    }
    for (i = 0; i < 13; i++, pos += 3) {
        if (i < n) {
            skip = 0;
            switch (gScene.area) {
            case AREA_SNOWY_MOUNTAINS_6:
                switch (i) {
                case 0:
                    l->state[i] = 1;
                    l->level[i] = AreaObj_K07_Select3(lv, 0, 0x80, 0xFF);
                    break;
                case 5:
                    l->state[i] = 2;
                    if (QuestGoal_DeliveryDone() == 0) {
                        l->level[i] = 0;
                    } else {
                        l->level[i] = 0xFF;
                    }
                    break;
                default:
                    l->state[i] = 0;
                    skip = 1;
                    l->level[i] = 0xFF;
                    break;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_NIGHT_5:
                switch (i) {
                case 0:
                    l->state[i] = 0;
                    l->level[i] = AreaObj_K07_Select3(lv, 0xFF, 0x80, 0);
                    break;
                case 1:
                    skip = 1;
                    l->state[i] = 3;
                    l->level[i] = AreaObj_K07_Select3(lv, 0xFF, 0x80, 0);
                    break;
                default:
                    l->state[i] = 1;
                    skip = 1;
                    l->level[i] = 0xFF;
                    break;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_NIGHT_6:
                switch (i) {
                case 0:
                    l->state[i] = 0;
                    l->level[i] = AreaObj_K07_Select3(lv, 0xFF, 0x80, 0);
                    break;
                case 1:
                    skip = 1;
                    l->state[i] = 3;
                    l->level[i] = AreaObj_K07_Select3(lv, 0xFF, 0x80, 0);
                    break;
                case 6:
                    l->state[i] = 2;
                    l->level[i] = 0;
                    break;
                default:
                    l->state[i] = 1;
                    skip = 1;
                    l->level[i] = 0xFF;
                    break;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_NIGHT_8:
                switch (i) {
                case 0:
                    l->state[i] = 0;
                    l->level[i] = AreaObj_K07_Select3(lv, 0xFF, 0x80, 0);
                    break;
                case 1:
                    skip = 1;
                    l->state[i] = 3;
                    l->level[i] = AreaObj_K07_Select3(lv, 0xFF, 0x80, 0);
                    break;
                default:
                    l->state[i] = 1;
                    skip = 1;
                    l->level[i] = 0xFF;
                    break;
                }
                break;
            case AREA_BATTLE_TOWN_0:
            case AREA_BATTLE_TOWN_NIGHT_0:
                switch (i) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                    l->state[i] = 0;
                    break;
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                    l->state[i] = 1;
                    break;
                default:
                    l->state[i] = 2;
                    break;
                }
                break;
            }
            l->rot[i] = 0;
            if (cnt < m) {
                if (skip == 0) {
                l->slot[i] = DrawSlot_Alloc();
                if (l->slot[i] != -1) {
                    ds = DrawSlot_Get(l->slot[i]);
                    l->ds[i] = ds;
                    ds->owner = o;
                    ds->part = i;
                    ds->draw = AreaObj_K07_Draw;
                    cnt++;
                    ds->pos[0] = pos[0];
                    ds->pos[1] = pos[1];
                    ds->pos[2] = pos[2];
                } else {
                    l->ds[i] = NULL;
                    if (cnt == 0) {
                        AreaObj_K07_Free(o);
                        return;
                    }
                }
                } else {
                    l->ds[i] = NULL;
                }
            } else {
                l->ds[i] = NULL;
            }
        } else {
            l->ds[i] = NULL;
        }
    }
    AreaObj_K07_Run(arg);
}

void AreaObj_K07_Run(Obj *o)
{
    AreaState *as = &gAreaState;
    s16 i;
    ObjParts13 *l = o->buf;
    u16 mode;
    u8 t;
    u8 *p;

    o->timer++;
    for (i = 0; i < 13; i++) {
        mode = 0;
        switch (gScene.area) {
        case AREA_SNOWY_MOUNTAINS_6:
            switch (l->state[i]) {
            default:
                l->rot[i] += AreaObj_K07_SpinDef[i];
                mode = 3;
                break;
            case 1:
                t = AreaObj_K07_Select3(Area_MonsterState(gScene.area), 0, 0x80, 0xFF);
                p = &l->level[i];
                if (t < *p) {
                    if (t + 8 < *p) {
                        *p = *p - 8;
                    } else {
                        *p = t;
                    }
                } else if (*p < t - 8) {
                    *p = *p + 8;
                } else {
                    *p = t;
                }
                mode = 3;
                break;
            case 2:
                if (QuestGoal_DeliveryDone() == 0) {
                    l->level[i] = 0;
                } else if (l->level[i] < 0xF7) {
                    l->level[i] += 8;
                } else {
                    l->level[i] = 0xFF;
                }
                mode = 0;
                break;
            }
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_5:
            switch (l->state[i]) {
            case 0:
            case 3:
                t = AreaObj_K07_Select3(Area_MonsterState(gScene.area), 0xFF, 0x80, 0);
                p = &l->level[i];
                if (t < *p) {
                    if (t + 8 < *p) {
                        *p = *p - 8;
                    } else {
                        *p = t;
                    }
                } else if (*p < t - 8) {
                    *p = *p + 8;
                } else {
                    *p = t;
                }
                mode = 3;
                break;
            }
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_6:
            switch (l->state[i]) {
            case 0:
            case 3:
                t = AreaObj_K07_Select3(Area_MonsterState(gScene.area), 0xFF, 0x80, 0);
                p = &l->level[i];
                if (t < *p) {
                    if (t + 8 < *p) {
                        *p = *p - 8;
                    } else {
                        *p = t;
                    }
                } else if (*p < t - 8) {
                    *p = *p + 8;
                } else {
                    *p = t;
                }
            default:
            case 1:
                l->rot[i] += AreaObj_K07_Spin1[i];
                mode = 3;
                break;
            case 2:
                if (QuestGoal_DeliveryDone() == 0) {
                    l->level[i] = 0;
                } else if (l->level[i] < 0xF7) {
                    l->level[i] += 8;
                } else {
                    l->level[i] = 0xFF;
                }
                mode = 0;
                break;
            }
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_8:
            switch (l->state[i]) {
            case 0:
            case 3:
                t = AreaObj_K07_Select3(Area_MonsterState(gScene.area), 0xFF, 0x80, 0);
                p = &l->level[i];
                if (t < *p) {
                    if (t + 8 < *p) {
                        *p = *p - 8;
                    } else {
                        *p = t;
                    }
                } else if (*p < t - 8) {
                    *p = *p + 8;
                } else {
                    *p = t;
                }
                mode = 3;
                break;
            }
            break;
        case AREA_BATTLE_TOWN_0:
        case AREA_BATTLE_TOWN_NIGHT_0:
            switch (l->state[i]) {
            case 1:
                l->level[i] = (u8) (24.0f * (1.0f + Math_Sin(6.2831855f * (as->frame << 12) / 65536.0f))) + 0xCF;
                mode = 0;
                break;
            }
            break;
        }
        if (l->ds[i] != NULL) {
            switch (mode) {
            default:
                OT_AddByDepth(gDrawOtSorted, l->ds[i], 0x40, 0);
                break;
            case 3:
                OT_AddByDepth(gDrawOt8, l->ds[i], 8, 1);
                break;
            }
        }
    }
}

void AreaObj_K07_End(Obj *o)
{
    o->state++;
    o->visible = 0;
}

void AreaObj_K07_Free(Obj *o)
{
    Obj_Free(o);
}

/* The per-part state of the objects AreaObj_K07_Draw draws (the object's buffer). */
typedef struct Obj3780Buf {
    u8 kind[13];                /* 0x00 */
    u8 alpha[13];               /* 0x0D */
    u16 ang[8];                 /* 0x1A */
} Obj3780Buf;

void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
extern f32 AreaObj_K07_Scale[];
extern s16 AreaObj_K07_Models1[];
extern s16 AreaObj_K07_Models69[];
extern s16 D_008B07F8[];
extern s16 AreaObj_K07_Models6D[];
extern s16 AreaObj_K07_ModelsB7[];

/* Draws the stage objects of AREA_FOREST_HILLS_11/AREA_FOREST_HILLS_NIGHT_11, AREA_SNOWY_MOUNTAINS_6, AREA_SNOWY_MOUNTAINS_NIGHT_5, AREA_SNOWY_MOUNTAINS_NIGHT_6, AREA_SNOWY_MOUNTAINS_NIGHT_8 and AREA_BATTLE_TOWN_0/AREA_BATTLE_TOWN_NIGHT_0. */
void AreaObj_K07_Draw(DrawSlot *ds)
{
    Model *stage = gAreaSubModels[0];
    Obj *o = ds->owner;
    Obj3780Buf *b = o->buf;
    ModelMatSet *mdl;
    AreaState *as = &gAreaState;
    s16 i;
    f32 u;
    f32 m[16];
    f32 uv[16];

    switch (gScene.area) {
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        stage = *(Model **) ((u8 *) as + 0x3C);
        break;
    default:
        if (stage == NULL) {
            return;
        }
        if (stage->active == 0) {
            return;
        }
        break;
    }
    switch (gScene.area) {
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        mdl = stage->matSets + 4;
        Mtx_Translation(uv, 0.25f * ((o->timer >> 1) & 3), 0.25f * ((o->timer >> 3) & 3), 0.0f);
        Gs_SetState(0x19, (s32) uv);
        Mtx_Translation(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        Mtx_Scale(m, AreaObj_K07_Scale[ds->part * 3], AreaObj_K07_Scale[ds->part * 3 + 1], AreaObj_K07_Scale[ds->part * 3 + 2]);
        Mtx_Concat3x3(m, gCamYawMtx);
        goto draw;
    case AREA_SNOWY_MOUNTAINS_6:
        Gs_SetState(0x60, 0);
        switch (b->kind[ds->part]) {
        case 0:
        case 1:
            for (i = 0; i < 5; i++) {
                mdl = stage->matSets + AreaObj_K07_Models1[i];
                Mtx_Translation(m, AreaObj_K07_PosArea61[i * 3], AreaObj_K07_PosArea61[i * 3 + 1], AreaObj_K07_PosArea61[i * 3 + 2]);
                if (b->kind[i] != 1) {
                    Mtx_RotateY(m, 6.2831855f * b->ang[i] / 65536.0f);
                } else {
                    Mtx_Translation(uv, (o->timer & 0x1FF) / 512.0f, 0.0f, 0.0f);
                    Gs_SetState(0x19, (s32) uv);
                    Gs_SetState(0x67, (b->alpha[i] << 24) | 0xFFFFFF);
                }
                Gs_SetState(0x1A, (s32) m);
                if (mdl != NULL && mdl->id != -1) {
                    Draw_ApplyStates(mdl->flags);
                    Model_Draw(mdl->id, 0);
                }
            }
            Draw_SetDefaultState();
            return;
        case 2:
        default:
            Gs_SetState(0x6C, 0);
            mdl = stage->matSets + AreaObj_K07_Models1[ds->part];
            Mtx_Translation(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            Mtx_Concat3x3(m, gCamYawMtx);
            Gs_SetState(0x67, (b->alpha[ds->part] << 24) | 0xFFFFFF);
            Mtx_Translation(uv, 0.0f, (o->timer & 0xFF) / 256.0f, 0.0f);
            Gs_SetState(0x19, (s32) uv);
            goto draw;
        }
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
        Gs_SetState(0x60, 0);
        for (i = 0; i < 4; i++) {
            mdl = stage->matSets + AreaObj_K07_Models69[i];
            Mtx_Translation(m, AreaObj_K07_PosArea69[i * 3], AreaObj_K07_PosArea69[i * 3 + 1], AreaObj_K07_PosArea69[i * 3 + 2]);
            switch (b->kind[i]) {
            case 3:
                Mtx_Translation(uv, (o->timer & 0x3FF) / 1024.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uv);
            case 0:
                Gs_SetState(0x67, (b->alpha[i] << 24) | 0xFFFFFF);
                break;
            case 1:
            default:
                if (i == 2) {
                    u = (o->timer & 0xFF) / 256.0f;
                } else {
                    u = (o->timer & 0x3F) / 64.0f;
                }
                Mtx_Translation(uv, u, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uv);
                break;
            }
            Gs_SetState(0x1A, (s32) m);
            if (mdl != NULL && mdl->id != -1) {
                Draw_ApplyStates(mdl->flags);
                Model_Draw(mdl->id, 0);
            }
        }
        Draw_SetDefaultState();
        return;
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        Gs_SetState(0x60, 0);
        switch (b->kind[ds->part]) {
        case 0:
        default:
        case 1:
        case 3:
            for (i = 0; i < 6; i++) {
                mdl = stage->matSets + D_008B07F8[i];
                Mtx_Translation(m, AreaObj_K07_PosArea6A[i * 3], AreaObj_K07_PosArea6A[i * 3 + 1], AreaObj_K07_PosArea6A[i * 3 + 2]);
                switch (b->kind[i]) {
                case 3:
                    Mtx_Translation(uv, (o->timer & 0x3FF) / 1024.0f, 0.0f, 0.0f);
                    Gs_SetState(0x19, (s32) uv);
                case 0:
                    Gs_SetState(0x67, (b->alpha[i] << 24) | 0xFFFFFF);
                    break;
                case 1:
                default:
                    Mtx_RotateY(m, 6.2831855f * b->ang[i] / 65536.0f);
                    break;
                }
                Gs_SetState(0x1A, (s32) m);
                if (mdl != NULL && mdl->id != -1) {
                    Draw_ApplyStates(mdl->flags);
                    Model_Draw(mdl->id, 0);
                }
            }
            Draw_SetDefaultState();
            return;
        case 2:
            Gs_SetState(0x6C, 0);
            mdl = stage->matSets + D_008B07F8[ds->part];
            Mtx_Translation(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            Mtx_Concat3x3(m, gCamYawMtx);
            Gs_SetState(0x67, (b->alpha[ds->part] << 24) | 0xFFFFFF);
            Mtx_Translation(uv, 0.0f, (o->timer & 0xFF) / 256.0f, 0.0f);
            Gs_SetState(0x19, (s32) uv);
            goto draw;
        }
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        Gs_SetState(0x60, 0);
        for (i = 0; i < 3; i++) {
            mdl = stage->matSets + AreaObj_K07_Models6D[i];
            Mtx_Translation(m, AreaObj_K07_PosArea6D[i * 3], AreaObj_K07_PosArea6D[i * 3 + 1], AreaObj_K07_PosArea6D[i * 3 + 2]);
            switch (b->kind[i]) {
            case 3:
                Mtx_Translation(uv, (o->timer & 0x3FF) / 1024.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uv);
            case 0:
            default:
                Gs_SetState(0x67, (b->alpha[i] << 24) | 0xFFFFFF);
                break;
            case 1:
                Mtx_Translation(uv, (o->timer & 0x7F) / 128.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) uv);
                break;
            }
            Gs_SetState(0x1A, (s32) m);
            if (mdl != NULL && mdl->id != -1) {
                Draw_ApplyStates(mdl->flags);
                Model_Draw(mdl->id, 0);
            }
        }
        Draw_SetDefaultState();
        return;
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        mdl = stage->matSets + AreaObj_K07_ModelsB7[ds->part];
        Gs_SetLightList(stage->mats, mdl);
        Mtx_Translation(m, ds->pos[0], ds->pos[1], ds->pos[2]);
        Mtx_Concat3x3(m, gCamMatrix);
        Gs_SetState(0x6C, 0);
        switch (AreaObj_K07_ModelsB7[ds->part]) {
        case 13:
        case 15:
        default:
            Mtx_Translation(uv, 0.25f * (as->frame & 3), 0.25f * ((as->frame >> 2) & 3), 0.0f);
            Gs_SetState(0x19, (s32) uv);
            Gs_SetState(0x67, -1);
            break;
        case 14:
            Gs_SetState(0x67, (b->alpha[ds->part] << 24) | 0xFFFFFF);
            break;
        }
        goto draw;
    }
    return;
draw:
    Gs_SetState(0x60, 0);
    Gs_SetState(0x1A, (s32) m);
    if (mdl != NULL && mdl->id != -1) {
        Draw_ApplyStates(mdl->flags);
        Model_Draw(mdl->id, 0);
    }
    Draw_SetDefaultState();
    Gs_SetState(0x6C, 1);
    return;
}

void AreaObj_K08_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x8;
        obj->type = 0;
        obj->update = AreaObj_K08_Update;
    }
}

void AreaObj_K08_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K08_Setup(o);
        break;
    case 1:
        AreaObj_K08_Run(o);
        break;
    case 2:
        AreaObj_K08_End(o);
        break;
    case 3:
        AreaObj_K08_Free(o);
        break;
    }
}

//---
void AreaObj_K08_Setup(Obj *o)
{

    o->state++;
    o->visible = 1;
    o->var3 = 0;
    o->pos[0] = 0.0f;
    o->pos[1] = 0.0f;
    o->pos[2] = 0.0f;
    o->timer = 0;
    o->draw = NULL;
    o->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (o->ds != NULL) {
        o->ds->owner = o;
        o->ds->draw = AreaObj_K08_Draw;
        AreaObj_K08_Run(o);
    } else {
        Obj_Free(o);
    }
}

void AreaObj_K08_Run(Obj *o)
{
    Player *p;

    p = &gPlayers[gScene.localPlayer];
    o->timer++;
    o->pos[0] = p->pos[0];
    o->pos[1] = 0.0f;
    o->pos[2] = p->pos[2];
    if (o->draw == NULL) {
        o->ds->pos[0] = o->pos[0];
        o->ds->pos[1] = o->pos[1];
        o->ds->pos[2] = o->pos[2];
        OT_AddByDepth(gDrawOt32, o->ds, 0x20, 1);
    }
}

void AreaObj_K08_End(Obj *o)
{
    o->state++;
    o->visible = 0;
}

void AreaObj_K08_Free(Obj *o)
{
    Obj_Free(o);
}

/* Draws the two stage models of the object 3 by 3 around it (a 1000-unit grid), their textures scrolled
 * by the table AreaObj_K08_Steps (16 steps by +8). */
void AreaObj_K08_Draw(DrawSlot *s)
{
    Obj *o = s->owner;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *mdl;
    f32 m2[16];
    f32 m1[16];
    f32 u;
    f32 x;
    f32 z0;
    f32 v;
    f32 z;
    s32 k;
    s32 i;
    s32 j;

    if (stage != NULL && stage->active != 0) {
        mdl = stage->matSets;
        Gs_SetState(0x60, 0x80);
        o->fvar = 1000.0f;
        for (k = 0; k < 2; mdl++, k++) {
            u = AreaObj_K08_Steps[((o->timer & 0x1E) >> 1) * 4 + k * 2];
            v = AreaObj_K08_Steps[((o->timer & 0x1E) >> 1) * 4 + k * 2 + 1];
            u -= Math_Floor(u);
            v -= Math_Floor(v);
            Mtx_Translation(m1, u, v, 0.0f);
            Gs_SetState(0x19, (s32) m1);
            x = 1000.0f * Math_Floor((500.0f + o->pos[0]) / 1000.0f) - 1000.0f;
            z0 = 1000.0f * Math_Floor((500.0f + o->pos[2]) / 1000.0f) - 1000.0f;
            for (i = 0; i < 3; i++, x += o->fvar) {
                for (j = 0, z = z0; j < 3; j++, z += o->fvar) {
                    Mtx_Translation(m2, x, 0.0f, z);
                    Gs_SetState(0x1A, (s32) m2);
                    if (mdl->id != -1) {
                        Draw_ApplyStates(mdl->flags);
                        Gfx_SetTexFilter(1);
                        Model_Draw(mdl->id, 0);
                    }
                }
            }
        }
        Gfx_SetTexFilter(0);
        Gs_SetState(0x60, 0);
        Draw_SetDefaultState();
    }
}

void AreaObj_K09_Create(u8 a) /* a: unused, sub_main's area setup passes the area's unk1 */
{
    Obj *obj = Obj_Alloc(0x1);

    if (obj != NULL) {
        obj->kind = 0x9;
        obj->type = 0;
        obj->var4 = 0;
        obj->update = AreaObj_K09_Update;
    }
}

void *Obj_AllocForKind(s16 kind)
{
    switch (kind) {
    case 8:
    case 0xA:
    case 0xB:
        return Obj_Alloc(0);
    default:
        return Obj_Alloc(1);
    }
}

void AreaObj_K09_Spawn(Obj *o, s16 kind, u8 c)
{
    ObjList32 *l = o->buf;
    s16 k;
    Obj *n;

    k = AreaObj_K09_FindFreeChild(o);
    if (k != -1) {
        n = Obj_AllocForKind(kind);
        if (n != NULL) {
            n->kind = 9;
            n->type = kind;
            n->update = AreaObj_K09_Update;
            n->var4 = c;
            l->obj[k] = n;
            l->count[kind]++;
            l->count[0]++;
        }
    }
}

void AreaObj_K09_Spawn0A(Obj *o, s16 kind, s16 c)
{
    ObjList32 *l = o->buf;
    s16 k;
    Obj *n;

    k = AreaObj_K09_FindFreeChild(o);
    if (k != -1) {
        n = Obj_AllocForKind(kind);
        if (n != NULL) {
            n->kind = 9;
            n->type = kind;
            n->update = AreaObj_K09_Update;
            n->var3 = c;
            n->var4 = 0;
            l->obj[k] = n;
            l->count[kind]++;
            l->count[0]++;
        }
    }
}

s16 AreaObj_K09_FindFreeChild(Obj *o)
{
    Obj **p = o->buf;
    s16 i;

    for (i = 0; i < 32; i++) {
        if (*p == NULL) {
            return i;
        }
        p++;
    }
    return -1;
}

void AreaObj_K09_CreateAt(void *src, s16 k)
{
    Obj *obj = Obj_AllocForKind(k);

    if (obj != NULL) {
        obj->kind = 9;
        obj->type = k;
        obj->update = AreaObj_K09_Update;
        Vec_Copy((f32 *)obj->pos, (f32 *)src);
        obj->var4 = 0xFF;
    }
}

void AreaObj_K09_Update(Obj *o)
{
    switch (o->state) {
    case 0:
        AreaObj_K09_Setup(o);
        break;
    case 1:
        AreaObj_K09_Run(o);
        break;
    case 2:
        AreaObj_K09_End(o);
        break;
    case 3:
        AreaObj_K09_Free(o);
        break;
    }
}

//---
void AreaObj_K09_Setup(Obj *o)
{

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    switch (o->type) {
    case 0:
        AreaObj_K09_SetupType0(o);
        break;
    case 1:
        AreaObj_K09_SetupType1(o);
        break;
    case 2:
        AreaObj_K09_SetupType2(o);
        break;
    case 3:
        AreaObj_K09_SetupType3(o);
        break;
    case 4:
        AreaObj_K09_SetupType4(o);
        break;
    case 5:
        AreaObj_K09_SetupType5(o);
        break;
    case 6:
        AreaObj_K09_SetupType6(o);
        break;
    case 7:
        AreaObj_K09_SetupType7(o);
        break;
    case 8:
        AreaObj_K09_SetupType8(o);
        break;
    case 9:
        AreaObj_K09_SetupType9();
        break;
    case 10:
        AreaObj_K09_SetupType10();
        break;
    case 11:
        AreaObj_K09_SetupType11(o);
        break;
    }
    if (o->type == 0 || o->type == 5) {
        return;
    }
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->ds = DrawSlot_Get(o->slot);
        if (o->ds != NULL) {
            o->ds->owner = o;
            o->ds->draw = AreaObj_K09_Draw;
        }
    } else {
        AreaObj_K09_Free(o);
    }
}

void AreaObj_K09_Run(Obj *o)
{
    switch (o->type) {
    case 0:
        AreaObj_K09_RunType0(o);
        return;
    case 1:
        AreaObj_K09_RunType1(o);
        return;
    case 2:
        AreaObj_K09_RunType2(o);
        return;
    case 3:
        AreaObj_K09_RunType3(o);
        return;
    case 4:
        AreaObj_K09_RunType4(o);
        return;
    case 5:
        AreaObj_K09_RunType5(o);
        return;
    case 6:
        AreaObj_K09_RunType6(o);
        return;
    case 7:
        AreaObj_K09_RunType7(o);
        return;
    case 8:
        AreaObj_K09_RunType8(o);
        return;
    case 9:
        AreaObj_K09_RunType9();
        return;
    case 10:
        AreaObj_K09_RunType10();
        return;
    case 11:
        AreaObj_K09_RunType11(o);
        
    default:
        return;
    }
}

typedef struct Handles2 { s16 id[2]; u8 unk4[0x14 - 4]; s32 h[2]; } Handles2;

void AreaObj_K09_End(Obj *o)
{
    Handles2 *x = o->buf;
    s16 i;

    o->state++;
    o->visible = 0;
    if (o->type != 5) {
        if (o->ds != NULL) {
            DrawSlot_Free(o->slot);
        }
    } else {
        for (i = 0; i < 2; i++) {
            if (x->h[i] != 0) {
                DrawSlot_Free(x->id[i]);
            }
        }
    }
}

void AreaObj_K09_Free(Obj *o)
{
    Obj_Free(o);
}

void AreaObj_K09_Draw(DrawSlot *ds)
{
    u8 type;

    type = ((u8 *) ds->owner)[3];
    switch (type) {
    case 1:
        AreaObj_K09_DrawType1(ds);
        break;
    case 2:
        AreaObj_K09_DrawType2(ds);
        break;
    case 3:
        AreaObj_K09_DrawType3(ds);
        break;
    case 4:
        AreaObj_K09_DrawType4(ds);
        break;
    case 5:
        AreaObj_K09_DrawType5(ds);
        break;
    case 6:
        AreaObj_K09_DrawType6(ds);
        break;
    case 7:
        AreaObj_K09_DrawType7(ds);
        break;
    case 8:
        AreaObj_K09_DrawType8(ds);
        break;
    case 9:
        AreaObj_K09_DrawType9();
        break;
    case 10:
        AreaObj_K09_DrawType10();
        break;
    case 11:
        AreaObj_K09_DrawType11(ds);
        
    case 0x0:
    default:
        break;
    }
}

void AreaObj_K09_DrawModel(ModelMatSet *set, s32 v)
{
    Gs_SetState(0x1A, v);
    if ((set != NULL) && (set->id != -1)) {
        Draw_ApplyStates(set->flags);
        Model_Draw(set->id, 0);
    }
    Draw_SetDefaultState();
}

//---
void AreaObj_K09_SetupType0(Obj *o)
{
    ObjList32 *l = o->buf;
    s16 i;

    o->timer = 0;
    for (i = 0; i < 32; i++) {
        l->obj[i] = NULL;
    }
    l->count[0] = 0;
    for (i = 0; i < 11; i++) {
        l->count[i + 1] = 0;
    }
    switch (gScene.area) {
    case AREA_FOREST_HILLS_6:
    case AREA_GREAT_ARENA_1:
    case AREA_DESERT_2:
    case AREA_DESERT_8:
    case AREA_DESERT_9:
        if (l->count[0] < 32) {
            for (i = 0; i < 4; i++) {
                if (l->count[7] < 4) {
                    AreaObj_K09_Spawn(o, 7, i);
                }
            }
        }
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (l->count[0] < 32 && l->count[6] <= 0) {
            AreaObj_K09_Spawn(o, 6, 0);
        }
        break;
    case AREA_TOWER_8:
        if (l->count[0] < 32) {
            for (i = 0; i < 10; i++) {
                AreaObj_K09_Spawn(o, 8, i);
            }
        }
        break;
    }
}

void AreaObj_K09_RunType0(Obj *o)
{
    ObjList32 *l = o->buf;
    s16 i;
    s32 n;
    Obj *c;

    o->timer++;
    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_9:
        if (l->count[0] < 32 && l->count[5] < 2) {
            AreaObj_K09_Spawn(o, 5, 0);
        }
        break;
    case AREA_FOREST_HILLS_3:
    case AREA_FOREST_HILLS_NIGHT_3:
        if (*(u8 *) &o->timer == 0 && l->count[0] < 32 && l->count[11] < 2) {
            n = (s16) (Rand_Next(1) & 1) + 1;
            for (i = 0; i < n; i++) {
                AreaObj_K09_Spawn0A(o, 11, 2);
                if (l->count[0] >= 32) {
                    break;
                }
            }
        }
        break;
    case AREA_FOREST_HILLS_9:
    case AREA_FOREST_HILLS_NIGHT_9:
        if (l->count[0] < 32 && l->count[8] < 2) {
            AreaObj_K09_Spawn(o, 8, 0);
        }
        break;
    case AREA_FOREST_HILLS_5:
    case AREA_FOREST_HILLS_NIGHT_5:
        if (l->count[0] < 32 && l->count[4] < 2) {
            AreaObj_K09_Spawn(o, 4, 0);
        }
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (o->timer >= 60 && l->count[0] < 32) {
            AreaObj_K09_Spawn(o, 1, 0);
            o->timer = 0;
        }
        if (l->count[0] < 32 && l->count[3] < 3) {
            AreaObj_K09_Spawn(o, 3, 0);
        }
        if (l->count[0] < 32 && l->count[4] < 5) {
            AreaObj_K09_Spawn(o, 4, 0);
        }
        if (l->count[0] < 32 && l->count[5] < 2) {
            AreaObj_K09_Spawn(o, 5, 0);
        }
        break;
    }
    for (i = 0; i < 32; i++) {
        c = l->obj[i];
        if (c != NULL && c->state >= 2) {
            l->count[c->type]--;
            l->obj[i] = NULL;
            l->count[0]--;
        }
    }
}

void AreaObj_K09_SetupType1(Obj *o)
{
    ObjTumble *t = o->buf;

    o->var0 = Rand_Next(1) & 3;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        o->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0x7FF);
        o->pos[1] = 3000.0f;
        o->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0x3FF);
        break;
    }
    o->timer = 0;
    t->v[0] = 0.0f;
    t->v[1] = 0.0f;
    t->v[2] = 0.0f;
    t->rot[0] = Rand_Next(1);
    t->rot[1] = Rand_Next(1);
    t->rot[2] = Rand_Next(1);
    t->spinX = 0;
    t->spinZ = 0;
}

void AreaObj_K09_RunType1(Obj *o)
{
    ObjTumble *t = o->buf;
    f32 d[3];

    t->spinX = 0.95f * t->spinX;
    t->spinZ = 0.975f * t->spinZ;
    t->spinX += (s16) ((Rand_Next(1) & 0xFF) - 0x80);
    t->spinZ += (s16) ((Rand_Next(1) & 0x3F) - 0x20);
    t->rot[0] += t->spinX;
    t->rot[1] += (u16) ((Rand_Next(1) & 0x1F) - 0xF);
    t->rot[2] += t->spinZ;
    Mtx_Identity(t->m);
    Mtx_RotateX(t->m, 6.2831855f * t->rot[0] / 65536.0f);
    Mtx_TurnY(t->m, 6.2831855f * t->rot[1] / 65536.0f);
    Mtx_TurnZ(t->m, 6.2831855f * t->rot[2] / 65536.0f);
    t->v[0] += -2.0f * t->m[1];
    t->v[1] += -2.0f * t->m[5];
    t->v[2] += -2.0f * t->m[9];
    t->v[0] *= 0.6f;
    t->v[1] *= 0.25f;
    t->v[2] *= 0.7f;
    Vec_Rotate(d, t->v, t->m);
    o->pos[0] += d[0];
    o->pos[1] += d[1];
    o->pos[2] += d[2];
    if (!(0.0f <= o->pos[1])) {
        o->state++;
    } else {
        o->ds->pos[0] = o->pos[0];
        o->ds->pos[1] = o->pos[1];
        o->ds->pos[2] = o->pos[2];
        OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
    }
}

void AreaObj_K09_DrawType1(DrawSlot *ds)
{
    f32 *m;
    ModelMatSet *set;
    f32 mtx[16];
    f32 var_f1;
    s16 idx;
    u8 temp_v0;
    Model *subs;
    Obj *w;

    w = ds->owner;
    subs = gAreaSubModels[0];
    m = w->buf;
    if ((subs != NULL) && (subs->active != 0)) {
        idx = gAreaAmbModels[gScene.area][2];
        if (idx >= 0) {
            temp_v0 = w->var0;
            set = &subs->matSets[idx];
            var_f1 = (f32) (u32) temp_v0;
            Mtx_Translation(mtx, 0.25f * var_f1, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) mtx);
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            AreaObj_K09_DrawModel(set, (s32) m);
        }
    }
}

//---
void AreaObj_K09_SetupType2(Obj *o)
{
    ObjTumble *t = o->buf;
    u8 k;
    f32 dx;
    f32 dz;

    o->var0 = Rand_Next(1) % 4;
    k = Rand_Next(1) % 3;
    if (k >= o->var0) {
        k++;
    }
    o->var1 = 0;
    o->var2 = 0;
    o->timer = 0;
    o->pos[0] = AreaObj_K09_Type2Pos[o->var0 * 3];
    o->pos[1] = D_008B09E4[o->var0 * 3];
    o->pos[2] = D_008B09E8[o->var0 * 3];
    dx = AreaObj_K09_Type2Pos[k * 3] - o->pos[0];
    dz = D_008B09E8[k * 3] - o->pos[2];
    t->rot[0] = (u8) Rand_Next(1) - 0xEF;
    t->rot[1] = (s32) (65536.0f * Math_Atan2(dx, dz) / 6.2831855f + 0.5f);
    t->rot[2] = 0;
    t->v[0] = 0.0f;
    t->v[1] = 0.0f;
    t->v[2] = 6.0f + 0.002f * (Rand_Next(1) & 0x3FF);
    t->spinX = 0;
    t->spinZ = 0;
}

void AreaObj_K09_RunType2(Obj *o)
{
    ObjTumble *t = o->buf;
    f32 d[3];

    o->timer++;
    if (o->timer >= 500) {
        o->state++;
        return;
    }
    switch (o->var1) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        o->var1 = Rand_Next(1) & 7;
        if (o->var1 > 5) {
            o->var2 = (Rand_Next(1) & 0xF) + 10;
        }
        break;
    case 6:
        o->var2--;
        t->rot[1] -= 0x20;
        if (o->var2 <= 0) {
            o->var1 = 0;
        }
        break;
    case 7:
        o->var2--;
        t->rot[1] += 0x20;
        if (o->var2 <= 0) {
            o->var1 = 0;
        }
        break;
    }
    switch (t->spinX) {
    case 0:
    case 1:
        if (--t->spinZ < 0) {
            t->spinX = Rand_Next(1) & 3;
            if (t->spinX > 1) {
                t->spinZ = (Rand_Next(1) & 7) + 3;
            } else {
                t->spinZ = 0;
            }
        }
        break;
    case 2:
        if (t->rot[0] > -0x1000) {
            t->rot[0] -= 0x60;
        }
        t->spinX = 0;
        break;
    case 3:
        if (t->rot[0] < 0) {
            t->rot[1] += 0x600;
        }
        t->spinX = 0;
        break;
    }
    Mtx_Identity(t->m);
    Mtx_RotateXYZ(t->m, 6.2831855f * t->rot[0] / 65536.0f, 6.2831855f * t->rot[1] / 65536.0f,
                  6.2831855f * t->rot[2] / 65536.0f);
    Vec_Rotate(d, t->v, t->m);
    o->pos[0] += d[0];
    o->pos[1] += d[1];
    o->pos[2] += d[2];
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
}

void AreaObj_K09_DrawType2(DrawSlot *ds)
{
    ModelMatSet *set;
    f32 *m;
    s16 idx;
    Model *subs;

    subs = gAreaSubModels[0];
    m = ((Obj *) ds->owner)->buf;
    if ((subs != NULL) && (subs->active != 0)) {
        idx = gAreaAmbModels[gScene.area][3];
        if (idx >= 0) {
            set = &subs->matSets[idx];
            Gs_SetState(0x60, 0);
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            AreaObj_K09_DrawModel(set, (s32) m);
            Gs_SetState(0x60, 0x80);
        }
    }
}

void AreaObj_K09_SetupType3(Obj *o)
{
    ObjTumble *t = o->buf;

    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    o->timer = 0;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        o->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0x7FF);
        o->pos[1] = 300.0f;
        o->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0x3FF);
        break;
    }
    t->rot[1] = Rand_Next(1);
    t->v[0] = 0.0f;
    t->v[1] = 0.0f;
    t->v[2] = 0.5f;
    t->spinX = Rand_Next(1) & 0x1F;
    t->spinZ = 0;
}

/* The run state of the wandering objects that AreaObj_K09_SetupType3 places (AREA_FOREST_HILLS_7 and AREA_FOREST_HILLS_NIGHT_7, where it
 * ends once out of its box): random bursts of speed (unk005), turns (unk006) and climbs (unk007). */
void AreaObj_K09_RunType3(Obj *o)
{
    ObjTumble *t = o->buf;
    f32 d[3];

    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (!(o->pos[0] >= 9773.0f && o->pos[0] <= 11174.0f && o->pos[1] <= 2000.0f && o->pos[2] >= 9173.0f &&
              o->pos[2] <= 10576.0f)) {
            o->state++;
            return;
        }
        break;
    }
    switch (o->var0) {
    case 0:
        if (--t->spinX <= 0) {
            o->var0++;
            t->spinX = Rand_Next(1) % 6 + 2;
        }
        break;
    case 1:
        if (t->v[2] < 10.0f) {
            if ((s16) (Rand_Next(1) & 1) == 0) {
                t->v[2] += 0.5f;
            } else {
                t->v[2] += 0.3f;
            }
        }
        if (--t->spinX <= 0) {
            o->var0++;
            t->spinX = Rand_Next(1) % 8 + 3;
        }
        break;
    case 2:
        if (--t->spinX <= 0) {
            o->var0++;
            t->spinX = (Rand_Next(1) & 4) + 2;
        }
        break;
    case 3:
        if ((s16) (Rand_Next(1) & 1) == 0) {
            t->v[2] *= 0.5f;
        } else {
            t->v[2] *= 0.4f;
        }
        if (--t->spinX <= 0) {
            o->var0 = 0;
            t->spinX = Rand_Next(1) % 20 + 7;
        }
        break;
    }
    switch (o->var1) {
    case 0:
        o->timer = Rand_Next(1) % 20 + 5;
        o->var1 = Rand_Next(1) % 3 + 1;
        break;
    case 1:
        if (--o->timer <= 0) {
            o->var1 = 0;
        }
        break;
    case 2:
        if (--o->timer <= 0) {
            o->timer = Rand_Next(1) % 10 + 5;
            o->var1 = 1;
        }
        t->rot[1] -= 0x180;
        break;
    case 3:
        if (--o->timer <= 0) {
            o->timer = Rand_Next(1) % 10 + 5;
            o->var1 = 1;
        }
        t->rot[1] += 0x180;
        break;
    case 4:
        if (--o->timer <= 0) {
            o->timer = Rand_Next(1) % 10 + 5;
            o->var1 = 1;
        }
        t->rot[1] -= 0x200;
        break;
    case 5:
        if (--o->timer <= 0) {
            o->timer = Rand_Next(1) % 10 + 5;
            o->var1 = 1;
        }
        t->rot[1] += 0x200;
        break;
    }
    switch (o->var2) {
    case 0:
        t->spinZ = (Rand_Next(1) & 0xF) + 5;
        o->var2 = Rand_Next(1) % 3 + 1;
        break;
    case 1:
        if (--t->spinZ <= 0) {
            o->var2 = 0;
        }
        break;
    case 2:
        if (--t->spinZ <= 0) {
            t->spinZ = (Rand_Next(1) & 0xF) + 5;
            o->var2 = 1;
        }
        o->pos[1] += 1.0f;
        break;
    case 3:
        if (--t->spinZ <= 0) {
            t->spinZ = (Rand_Next(1) & 0xF) + 5;
            o->var2 = 1;
        }
        if (o->pos[1] > 250.0f) {
            o->pos[1] -= 1.0f;
        }
        break;
    }
    Mtx_Identity(t->m);
    Mtx_RotateY(t->m, 6.2831855f * t->rot[1] / 65536.0f);
    Vec_Rotate(d, t->v, t->m);
    o->pos[0] += d[0];
    o->pos[1] += d[1];
    o->pos[2] += d[2];
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
}

void AreaObj_K09_DrawType3(DrawSlot *ds)
{
    ModelMatSet *set;
    f32 *m;
    s16 idx;
    Model *subs;

    subs = gAreaSubModels[0];
    m = ((Obj *) ds->owner)->buf;
    if ((subs != NULL) && (subs->active != 0)) {
        idx = gAreaAmbModels[gScene.area][4];
        if (idx >= 0) {
            set = &subs->matSets[idx];
            Gs_SetState(0x60, 0);
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            AreaObj_K09_DrawModel(set, (s32) m);
            Gs_SetState(0x60, 0x80);
        }
    }
}

void AreaObj_K09_SetupType4(Obj *o)
{
    u16 area;
    ObjTumble *t;

    t = o->buf;
    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    o->timer = 0;
    area = gScene.area;
    switch (area) {
    case AREA_FOREST_HILLS_5:
    case AREA_FOREST_HILLS_NIGHT_5:
        o->pos[0] = 10750.0f + (f32) (Rand_Next(1) & 0xFF);
        o->pos[1] = 450.0f;
        o->pos[2] = 11700.0f + (f32) (Rand_Next(1) & 0xFF);
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        o->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0xFFFF & 0x7FF);
        o->pos[1] = 150.0f;
        o->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0xFFFF & 0x3FF);
        break;
    }
    t->v[0] = 0.0f;
    t->v[1] = 0.0f;
    t->v[2] = 5.0f;
}

void AreaObj_K09_RunType4(Obj *o)
{
    ObjTumble *t = o->buf;
    f32 d[3];

    switch (gScene.area) {
    case AREA_FOREST_HILLS_5:
    case AREA_FOREST_HILLS_NIGHT_5:
        if (o->pos[0] < 10450.0f || o->pos[0] > 11850.0f || o->pos[1] > 1500.0f || o->pos[2] < 11400.0f ||
            o->pos[2] > 12500.0f) {
            o->state++;
            return;
        }
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (o->pos[0] < 9773.0f || o->pos[0] > 11174.0f || o->pos[1] > 2000.0f || o->pos[2] < 9173.0f ||
            o->pos[2] > 10576.0f) {
            o->state++;
            return;
        }
        break;
    }
    switch (o->var0) {
    case 0:
        o->timer = Rand_Next(1) % 40 + 10;
        o->var0 = Rand_Next(1) % 3 + 1;
        break;
    case 1:
        if (--o->timer <= 0) {
            o->var0 = 0;
        }
        break;
    case 2:
        if (--o->timer <= 0) {
            o->var0 = 1;
        }
        t->rot[1] -= 0x120;
        break;
    case 3:
        if (--o->timer <= 0) {
            o->var0 = 1;
        }
        t->rot[1] += 0x120;
        break;
    }
    switch (o->var1) {
    case 0:
        o->var2 = (Rand_Next(1) & 0xF) + 5;
        o->var1 = Rand_Next(1) % 2 + 1;
        break;
    case 1:
        if (--o->var2 <= 0) {
            o->var1 = 0;
        }
        break;
    case 2:
        if (--o->var2 <= 0) {
            o->var2 = (Rand_Next(1) & 0xF) + 5;
            o->var1 = 1;
        }
        o->pos[1] += 1.0f;
        break;
    case 3:
        if (--o->var2 <= 0) {
            o->var2 = (Rand_Next(1) & 0xF) + 5;
            o->var1 = 1;
        }
        if (o->pos[1] > 50.0f) {
            o->pos[1] -= 1.0f;
        }
        break;
    }
    Mtx_Identity(t->m);
    Mtx_RotateY(t->m, 6.2831855f * t->rot[1] / 65536.0f);
    Vec_Rotate(d, t->v, t->m);
    o->pos[0] += d[0];
    o->pos[1] += d[1];
    o->pos[2] += d[2];
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
}

void AreaObj_K09_DrawType4(DrawSlot *ds)
{
    ModelMatSet *set;
    f32 *m;
    s16 idx;
    Model *subs;

    subs = gAreaSubModels[0];
    m = ((Obj *) ds->owner)->buf;
    if ((subs != NULL) && (subs->active != 0)) {
        idx = gAreaAmbModels[gScene.area][5];
        if (idx >= 0) {
            set = &subs->matSets[idx];
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            AreaObj_K09_DrawModel(set, (s32) m);
        }
    }
}

//---
void AreaObj_K09_SetupType5(Obj *o)
{
    ObjSlots2 *v = o->buf;
    s16 i;

    o->var0 = 0;
    o->var1 = 0;
    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_9:
        o->pos[0] = 6500.0f + (f32) (Rand_Next(1) & 0x7FF);
        o->pos[1] = 150.0f;
        o->pos[2] = 12750.0f + (f32) (Rand_Next(1) & 0x7FF);
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        o->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0x7FF);
        o->pos[1] = 150.0f;
        o->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0x3FF);
        break;
    default:
        o->pos[0] = (Rand_Next(1) & 0x7FF) + 0x5DC;
        o->pos[1] = (Rand_Next(1) & 0xF) + 0x1E;
        o->pos[2] = (Rand_Next(1) & 0xFFF) + 0x7D0;
        v->heading = Rand_Next(1);
        break;
    }
    o->timer = 0;
    v->speed = 0.0f;
    v->side = 0.0f;
    v->pitch = Rand_Next(1) % 0x6000 - 0x3000;
    for (i = 0; i < 2; i++) {
        v->slot[i] = DrawSlot_Alloc();
        if (v->slot[i] != -1) {
            v->ds[i] = DrawSlot_Get(v->slot[i]);
            v->ds[i]->owner = o;
            v->ds[i]->part = i;
            v->ds[i]->draw = AreaObj_K09_Draw;
        } else {
            v->ds[i] = NULL;
        }
    }
}

/* The run state of the swimming objects of areas 6, 0x29 and 0xC2 and the default area (it ends
 * once out of the area box): random speed and sideways drift, dives and climbs to a random pitch
 * (unk005), turns (unk006); both draw slots follow the position. */
void AreaObj_K09_RunType5(Obj *o)
{
    ObjSlots2 *v = o->buf;
    f32 sc;
    s16 i;
    s16 r;
    s16 a;
    s16 dp;
    f32 *dy;
    f32 *dz;
    f32 d[3];

    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_9:
        if (!(o->pos[0] >= 6500.0f && o->pos[0] <= 9000.0f && o->pos[1] <= 2000.0f && o->pos[1] >= -100.0f &&
              o->pos[2] >= 12750.0f && o->pos[2] <= 15000.0f)) {
            o->state++;
            return;
        }
        sc = 1.0f;
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (!(o->pos[0] >= 9773.0f && o->pos[0] <= 11174.0f && o->pos[1] <= 2000.0f && o->pos[1] >= -100.0f &&
              o->pos[2] >= 9173.0f && o->pos[2] <= 10576.0f)) {
            o->state++;
            return;
        }
        sc = 1.0f;
        break;
    default:
        if (!(o->pos[0] >= 1500.0f && o->pos[0] <= 4000.0f && o->pos[1] <= 2000.0f && o->pos[1] >= -100.0f &&
              o->pos[2] >= 3000.0f && o->pos[2] <= 7000.0f)) {
            o->state++;
            return;
        }
        sc = 1.0f;
        break;
    }
    r = Rand_Next(1) % 5;
    switch (r) {
    case 0:
        break;
    case 1:
        if (v->speed < 4.0f * sc) {
            v->speed += 0.15f;
        }
        break;
    case 2:
        v->speed -= 0.15f * sc;
        if (v->speed < 0.0f) {
            v->speed = 0.0f;
        }
        break;
    case 3:
        if (v->speed < 4.0f * sc) {
            v->speed += 0.2f;
        }
        break;
    case 4:
        v->speed *= 0.5f;
        break;
    }
    r = Rand_Next(1) % 3;
    switch (r) {
    case 0:
        break;
    case 1:
        if (v->side < 0.8f * sc) {
            v->side += 0.05f;
        }
        break;
    case 2:
        if (v->side > 0.8f * -sc) {
            v->side -= 0.05f;
        }
        break;
    }
    dy = &d[1];
    dz = &d[2];
    d[0] = v->side;
    *dy = 0.0f;
    *dz = v->speed;
    switch (o->var0) {
    case 0:
        o->var0++;
        a = Rand_Next(1) % 0x1800;
        v->steps = (s16) (2.0f * sc) + Rand_Next(1) % 3;
        if (o->pos[1] < 200.0f) {
            v->steps--;
        } else if (o->pos[1] > 300.0f) {
            v->steps++;
        }
        if (v->pitch >= 0) {
            v->goal = -0x2000 - a;
        } else {
            v->goal = a + 0x2000;
        }
        break;
    case 1:
        dp = (s16) (v->goal - v->pitch) / v->steps;
        if (dp < 0) {
            o->pos[1] -= 0.6f * ((1.5f + sc) * ((dp - 0x300) / 1600.0f));
        }
        *(u16 *) &v->pitch += dp;
        v->steps--;
        if (v->steps <= 0) {
            o->var0++;
        }
        break;
    case 2:
        o->var0 = 0;
        break;
    }
    switch (o->var1) {
    case 0:
        o->timer = Rand_Next(1) % 20 + 10;
        o->var1 = Rand_Next(1) % 3 + 1;
    case 1:
        if (--o->timer <= 0) {
            o->var1 = 0;
        }
        break;
    case 2:
        if (--o->timer <= 0) {
            o->var1 = 1;
        }
        v->heading -= 0x180;
        break;
    case 3:
        if (--o->timer <= 0) {
            o->var1 = 1;
        }
        v->heading += 0x180;
        break;
    }
    o->pos[1] += -2.9f * (((s16) Math_Absf(v->pitch) + 0x1000) / 15000.0f);
    Vec_RotateY(d, 6.2831855f * v->heading / 65536.0f);
    o->pos[0] += d[0];
    o->pos[1] += *dy;
    o->pos[2] += *dz;
    for (i = 0; i < 2; i++) {
        if (v->ds[i] != NULL) {
            v->ds[i]->pos[0] = o->pos[0];
            v->ds[i]->pos[1] = o->pos[1];
            v->ds[i]->pos[2] = o->pos[2];
            OT_AddByDepth(gDrawOt32, v->ds[i], 0x20, 0);
        }
    }
}

//---
void AreaObj_K09_DrawType5(DrawSlot *s)
{
    ObjSlots2 *d = ((Obj *) s->owner)->buf;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *model;
    f32 m[16];
    s16 k;

    if (stage != NULL && stage->active != 0) {
        k = gAreaAmbModels[gScene.area][6];
        if (k >= 0) {
            model = &stage->matSets[k] + s->part;
            Mtx_Identity(m);
            if (s->part == 0) {
                Mtx_RotateZ(m, (6.2831855f * (u16) d->pitch) / 65536.0f);
            } else {
                Mtx_RotateZ(m, (6.2831855f * -(u16) d->pitch) / 65536.0f);
            }
            Mtx_RotateY(m, (6.2831855f * d->heading) / 65536.0f);
            Mtx_SetTrans(m, s->pos[0], s->pos[1], s->pos[2]);
            AreaObj_K09_DrawModel(model, (s32) m);
        }
    }
}

//---
void AreaObj_K09_SetupType6(Obj *o)
{
    ObjTumble *t = o->buf;

    o->pos[0] = gStartPos[gScene.area * 3];
    o->pos[1] = 100.0f + D_43F0A4[gScene.area * 3];
    o->pos[2] = D_43F0A8[gScene.area * 3];
    o->timer = 0;
    t->rot[0] = (Rand_Next(1) & 0xF) - 8;
    t->rot[1] = (Rand_Next(1) & 0xF) - 8;
    t->rot[2] = (Rand_Next(1) & 0xF) - 8;
    Mtx_Identity(t->m);
}

void AreaObj_K09_RunType6(Obj *o)
{
    ObjTumble *t = o->buf;

    Mtx_RotateXYZ(t->m, (6.2831855f * t->rot[0]) / 65536.0f,
                  (6.2831855f * t->rot[1]) / 65536.0f,
                  (6.2831855f * t->rot[2]) / 65536.0f);
    o->timer += 2;
    if (o->timer >= 20) {
        o->timer = 0;
    }
    o->ds->pos[0] = o->pos[0];
    o->ds->pos[1] = o->pos[1];
    o->ds->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->ds, 0x20, 0);
}

void AreaObj_K09_DrawType6(DrawSlot *ds)
{
    f32 mtx[16];
    ModelMatSet *set;
    f32 *m;
    s16 idx;
    Model *subs;
    Obj *w;

    w = ds->owner;
    subs = gAreaSubModels[0];
    m = w->buf;
    if ((subs != NULL) && (subs->active != 0)) {
        idx = gAreaAmbModels[gScene.area][7];
        if (idx >= 0) {
            set = &subs->matSets[idx];
            Mtx_Translation(mtx, 0.05f * (f32) w->timer, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) mtx);
            Mtx_SetTrans(m, ds->pos[0], ds->pos[1], ds->pos[2]);
            AreaObj_K09_DrawModel(set, (s32) m);
        }
    }
}

void AreaObj_K09_SetupType7(Obj *o)
{
    ObjTumble *t = o->buf;
    f32 r;
    f32 e;

    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_NIGHT_9:
        o->pos[0] = 2000.0f + (Rand_Next(1) & 0x3FFF);
        o->pos[1] = 2000.0f;
        o->pos[2] = 2000.0f + (Rand_Next(1) & 0x3FFF);
        r = o->pos[0];
        if (r < 10000.0f) {
            r = r;
        } else {
            r = 20000.0f - r;
        }
        e = o->pos[2];
        if (e < 10000.0f) {
            e = e;
        } else {
            e = 20000.0f - e;
        }
        if (e < r) {
            r = e;
        }
        if (r > 6000.0f) {
            r = 6000.0f;
        }
        break;
    case AREA_FOREST_HILLS_6:
        o->pos[0] = 9737.0f + ((Rand_Next(1) & 0x3FF) - 0x200);
        o->pos[1] = 8412.0f + (Rand_Next(1) & 0x3FF);
        o->pos[2] = 6057.0f + ((Rand_Next(1) & 0x3FF) - 0x200);
        r = 2000.0f + (Rand_Next(1) & 0x3FF);
        break;
    case AREA_GREAT_ARENA_1:
        o->pos[0] = 10500.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        o->pos[1] = 3000.0f + (Rand_Next(1) & 0x3FF);
        o->pos[2] = 9500.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        r = 2000.0f + (Rand_Next(1) & 0xFF);
        break;
    case AREA_DESERT_2:
        o->pos[0] = 10500.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        o->pos[1] = 3000.0f + (Rand_Next(1) & 0x7FF);
        o->pos[2] = 10000.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        r = 2000.0f + (Rand_Next(1) & 0x1FF);
        break;
    case AREA_DESERT_8:
        o->pos[0] = 6286.0f + ((Rand_Next(1) & 0x3FF) - 0x200);
        o->pos[1] = 1500.0f + (Rand_Next(1) & 0x3FF);
        o->pos[2] = 5300.0f + ((Rand_Next(1) & 0x3FF) - 0x200);
        r = 1000.0f + (Rand_Next(1) & 0xFF);
        break;
    case AREA_DESERT_9:
        o->pos[0] = 10000.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        o->pos[1] = 2800.0f + (Rand_Next(1) & 0x7FF);
        o->pos[2] = 9500.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        r = 3500.0f + (Rand_Next(1) & 0x1FF);
        break;
    }
    t->unk54 = 65536.0f * (33.0f / (6.2831855f * r));
    o->timer = 0;
    t->rot[1] = Rand_Next(1);
    Mtx_Translation(t->m, 0.0f, 0.0f, r);
    if (Rand_Next(1) & 1) {
        Mtx_RotateY(t->m, 1.5707964f);
        o->var2 = 0;
    } else {
        t->unk54 = -t->unk54;
        Mtx_RotateY(t->m, 4.712389f);
        o->var2 = 1;
    }
}

/* unk_6EC680's .data (0x8AEC50-0x8B0A10) */
extern u32 D_008DE300[];
extern u32 D_008DE320[];
extern u32 D_008DE340[];
extern u32 D_008DE360[];
extern u32 D_008DE390[];
extern u32 D_008DE3C0[];
extern u32 D_008DE3E0[];
extern u32 D_008DE3F8[];
extern u32 D_008DE410[];
extern u32 D_008DE430[];
extern u32 D_008DE450[];
extern u32 D_008DE470[];
extern u32 D_008DE490[];
extern u32 D_008DE4B0[];
extern u32 D_008DE4C8[];
extern u32 D_008DE4E0[];
extern u32 D_008DE500[];
extern u32 D_008DE520[];
extern u32 D_008DE550[];
extern u32 D_008DE570[];
extern u32 D_008DE590[];
extern u32 D_008DE5B0[];
extern u32 D_008DE5E0[];
extern u32 D_008DE600[];
extern u32 D_008DE620[];
extern u32 D_008DE640[];
extern u32 D_008DE660[];
extern u32 D_008DE680[];
extern u32 D_008DE6A0[];
extern u32 D_008DE6B0[];
extern u32 D_008DE6D0[];
extern u32 D_008DE6E0[];
extern u32 D_008DE700[];
extern u32 D_008DE718[];
extern u32 D_008DE730[];
extern u32 D_008DE750[];
extern u32 D_008DE760[];
extern u32 D_008DE780[];
extern u32 D_008DE790[];
extern u32 D_008DE7A8[];
extern u32 D_008DE7C0[];
extern u32 D_008DE7D8[];
extern u32 D_008DE7F0[];
extern u32 D_008DE810[];
extern u32 D_008DE830[];
extern u32 D_008DE850[];
extern u32 D_008DE870[];
extern u32 D_008DE890[];
extern u32 D_008DE8B0[];
extern u32 D_008DE8D0[];
extern u32 D_008DE8E8[];
extern u32 D_008DE8F8[];
extern u32 D_008DE910[];
extern u32 D_008DE930[];
extern u32 D_008DE940[];
extern u32 D_008DE960[];
extern u32 D_008DE970[];
extern u32 D_008DE990[];
extern u32 D_008DE9B0[];
extern u32 D_008DE9D0[];
extern u32 D_008DE9E0[];
extern u32 D_008DE9E8[];
extern u32 D_008DE9F8[];
extern u32 D_008DEA08[];
extern u32 D_008DEA18[];
extern u32 D_008DEA30[];
extern u32 D_008DEA50[];
extern u32 D_008DEA70[];
extern u32 D_008DEA90[];
extern u32 D_008DEAB0[];
extern u32 D_008DEAD0[];
extern u32 D_008DEAE0[];
extern u32 D_008DEB00[];
extern u32 D_008DEB20[];
extern u32 D_008DEB40[];
extern u32 D_008DEB58[];
extern u32 D_008DEB70[];
extern u32 D_008DEC20[];
extern u32 D_008DECD0[];
extern u32 D_008DED80[];
extern u32 D_008DEE60[];
extern u32 D_008DEF30[];
extern u32 D_008DEFE0[];
extern u32 D_008DF0A0[];
extern u32 D_008DF170[];
extern u32 D_008DF1E0[];
extern u32 D_008DF2A0[];
extern u32 D_008DF360[];
extern u32 D_008DF400[];
extern u32 D_008DF4B0[];
extern u32 D_008DF580[];
extern u32 D_008DF660[];
extern u32 D_008DF730[];
extern u32 D_008DF7D0[];
extern u32 D_008DF880[];
extern u32 D_008DF920[];
extern u32 D_008DFA10[];
extern u32 D_008DFAB0[];
extern u32 D_008DFB60[];
extern u32 D_008DFC10[];
extern u32 D_008DFD10[];
extern u32 D_008DFDD0[];
extern u32 D_008DFE60[];
extern u32 D_008DFF30[];
extern u32 D_008E0040[];
extern u32 D_008E0100[];
extern u32 D_008E0190[];
extern u32 D_008E0290[];
extern u32 D_008E0320[];
extern u32 D_008E0400[];
extern u32 D_008E04C0[];
extern u32 D_008E0580[];
extern u32 D_008E0620[];
extern u32 D_008E0710[];
extern u32 D_008E07A0[];
extern u32 D_008E0850[];
extern u32 D_008E0930[];
extern u32 D_008E0A30[];
extern u32 D_008E0AE0[];
extern u32 D_008E0B40[];
extern u32 D_008E0C20[];
extern u32 D_008E0CB0[];
extern u32 D_008E0D60[];
extern u32 D_008E0E20[];
extern u32 D_008E0ED0[];
extern u32 D_008E0F90[];
extern u32 D_008E1070[];
extern u32 D_008E1130[];
extern u32 D_008E11E0[];
extern u32 D_008E1260[];
extern u32 D_008E1310[];
extern u32 D_008E13C0[];
extern u32 D_008E1480[];
extern u32 D_008E1530[];
extern u32 D_008E1610[];
extern u32 D_008E1690[];
extern u32 D_008E1750[];
extern u32 D_008E1810[];
extern u32 D_008E18B0[];
extern u32 D_008E19B0[];
extern u32 D_008E1A40[];
extern u32 D_008E1AE0[];
extern u32 D_008E1B80[];
extern u32 D_008E1C00[];
extern u32 D_008E1C90[];
extern u32 D_008E1D30[];
extern u32 D_008E1DC0[];
extern u32 D_008E1E80[];
extern u32 D_008E1F60[];
extern u32 D_008E2040[];
extern u32 D_008E20E0[];
extern u32 D_008E21B0[];
extern u32 D_008E2240[];
extern u32 D_008E2300[];
extern u32 D_008E2360[];
extern u32 D_008E23F0[];
extern u32 D_008E2480[];
extern u32 D_008E2530[];
extern u32 D_008E2610[];
extern u32 D_008E26D0[];
extern u32 D_008E27B0[];
extern u32 D_008E2890[];
extern u32 D_008E2920[];
extern u32 D_008E29D0[];
extern u32 D_008E2AA0[];
extern u32 D_008E2B60[];
extern u32 D_008E2C50[];
extern u32 D_008E2CF0[];
extern u32 D_008E2DA0[];
extern u32 D_008E2E70[];
extern u32 D_008E2F70[];
extern u32 D_008E2FF0[];
extern u32 D_008E3090[];
extern u32 D_008E3120[];
extern u32 D_008E3200[];
extern u32 D_008E32C0[];
extern u32 D_008E3380[];
extern u32 D_008E3420[];
extern u32 D_008E34C0[];
extern u32 D_008E3550[];
extern u32 D_008E35D0[];
extern u32 D_008E3670[];
extern u32 D_008E3770[];
extern u32 D_008E37F0[];
extern u32 D_008E38B0[];
extern u32 D_008E3980[];
extern u32 D_008E3A30[];
extern u32 D_008E3AD0[];
extern u32 D_008E3B70[];
extern u32 D_008E3C10[];
extern u32 D_008E3CD0[];
extern u32 D_008E3D60[];
extern u32 D_008E3DE0[];
extern u32 D_008E3EC0[];
extern u32 D_008E3F90[];
extern u32 D_008E4070[];
extern u32 D_008E4120[];
extern u32 D_008E41E0[];
extern u32 D_008E42B0[];
extern u32 D_008E4380[];
extern u32 D_008E4450[];
extern u32 D_008E44E0[];
extern u32 D_008E45A0[];
extern u32 D_008E4670[];
extern u32 D_008E4750[];
extern u32 D_008E4800[];
extern u32 D_008E4890[];
extern u32 D_008E4960[];
extern u32 D_008E4A20[];
extern u32 D_008E4AA0[];
extern u32 D_008E4B80[];
extern u32 D_008E4C60[];
extern u32 D_008E4D50[];
extern u32 D_008E4E10[];
extern u32 D_008E4EA0[];
extern u32 D_008E4F50[];
extern u32 D_008E4FD0[];
extern u32 D_008E5090[];
extern u32 D_008E5130[];
extern u32 D_008E51E0[];
extern u32 D_008E52C0[];
extern u32 D_008E53A0[];
extern u32 D_008E5470[];
extern u32 D_008E5560[];
extern u32 D_008E55E0[];
extern u32 D_008E5690[];
extern u32 D_008E5760[];
extern u32 D_008E5830[];
extern u32 D_008E5920[];
extern u32 D_008E59E0[];
extern u32 D_008E5AA0[];
extern u32 D_008E5B60[];
extern u32 D_008E5BE0[];
extern u32 D_008E5CB0[];
extern u32 D_008E5D90[];
extern u32 D_008E5E50[];
extern u32 D_008E5F40[];
extern u32 D_008E6010[];
extern u32 D_008E60F0[];
extern u32 D_008E61E0[];
extern u32 D_008E6270[];
extern u32 D_008E6340[];
extern u32 D_008E6410[];
extern u32 D_008E64F0[];
extern u32 D_008E65C0[];
extern u32 D_008E6680[];
extern u32 D_008E6750[];
extern u32 D_008E67F0[];
extern u32 D_008E68C0[];
extern u32 D_008E69A0[];
extern u32 D_008E6A80[];
extern u32 D_008E6B50[];
extern u32 D_008E6C00[];
extern u32 D_008E6C90[];
extern u32 D_008E6D40[];
extern u32 D_008E6DD0[];
extern u32 D_008E6E70[];
extern u32 D_008E6F40[];
extern u32 D_008E6FE0[];
extern u32 D_008E7060[];
extern u32 D_008E7130[];
extern u32 D_008E71A0[];
extern u32 D_008E7230[];
extern u32 D_008E72C0[];
extern u32 D_008E7390[];
extern u32 D_008E7450[];
extern u32 D_008E74F0[];
extern u32 D_008E75C0[];
extern u32 D_008E76A0[];
extern u32 D_008E7720[];
extern u32 D_008E77E0[];
extern u32 D_008E78C0[];
extern u32 D_008E79A0[];
extern u32 D_008E7A50[];
extern u32 D_008E7B00[];
extern u32 D_008E7BA0[];
extern u32 D_008E7C70[];
extern u32 D_008E7D10[];
extern u32 D_008E7DE0[];
extern u32 D_008E7E90[];
extern u32 D_008E7F20[];
extern u32 D_008E7FD0[];
extern u32 D_008E8060[];
extern u32 D_008E8100[];
extern u32 D_008E8180[];
extern u32 D_008E8400[];
extern u32 D_008E84D0[];
extern u32 D_008E8590[];
extern u32 D_008E8660[];
extern u32 D_008E8750[];
extern u32 D_008E87E0[];
extern u32 D_008E88D0[];
extern u32 D_008E8960[];
extern u32 D_008E8A20[];
extern u32 D_008E8B10[];
extern u32 D_008E8BE0[];
extern u32 D_008E8C90[];
extern u32 D_008E8D20[];
extern u32 D_008E8DB0[];
extern u32 D_008E9000[];
extern u32 D_008E90F0[];
extern u32 D_008E92A0[];
extern u32 D_008E95A0[];
extern u32 D_008E9680[];
extern u32 D_008E9700[];
extern u32 D_008E97E0[];
extern u32 D_008E98C0[];
extern u32 D_008E9980[];
extern u32 D_008E9A00[];
extern u32 D_008E9AA0[];
extern u32 D_008E9B30[];
extern u32 D_008E9C00[];
extern u32 D_008E9CC0[];
extern u32 D_008E9DA0[];
extern u32 D_008E9E90[];
extern u32 D_008EA050[];
extern u32 D_008EA120[];
extern u32 D_008EA190[];
extern u32 D_008EA230[];
extern u32 D_008EA300[];
extern u32 D_008EA3E0[];
extern u32 D_008EA4B0[];
extern u32 D_008EA550[];
extern u32 D_008EA610[];
extern u32 D_008EA680[];
extern u32 D_008EA760[];
extern u32 D_008EA800[];
extern u32 D_008EA8F0[];
extern u32 D_008EA9B0[];
extern u32 D_008EAA70[];
extern u32 D_008EAB30[];
extern u32 D_008EABF0[];
extern u32 D_008EACB0[];
extern u32 D_008EAD50[];
extern u32 D_008EAE10[];
extern u32 D_008EAEA0[];
extern u32 D_008EAF30[];
extern u32 D_008EAFB0[];
extern u32 D_008EB030[];
extern u32 D_008EB0D0[];
extern u32 D_008EB190[];
extern u32 D_008EB260[];
extern u32 D_008EB300[];
extern u32 D_008EB3E0[];
extern u32 D_008EB440[];
extern u32 D_008EB500[];
extern u32 D_008EB570[];
extern u32 D_008EB5D0[];
extern u32 D_008EB660[];
extern u32 D_008EB6F0[];
extern u32 D_008EB770[];
extern u32 D_008EB800[];
extern u32 D_008EB850[];
extern u32 D_008EB8F0[];
extern u32 D_008EB970[];
extern u32 D_008EB9E0[];
extern u32 D_008EBA20[];
extern u32 D_008EBA90[];
extern u32 D_008EBB10[];
extern u32 D_008EBBA0[];
extern u32 D_008EBC70[];
extern u32 D_008EBD30[];
extern u32 D_008EBDD0[];
extern u32 D_008EBE90[];
extern u32 D_008EBF50[];
extern u32 D_008EC020[];
u8 TeostraBlastRing_ScaleKeysStart[64] = {
#include "game/008AEC50.inc"
};
u8 TeostraBlastRing_ColorKeysStart[32] = {
#include "game/008AEC90.inc"
};
u8 TeostraBlastRing_ScaleKeysMiddle[48] = {
#include "game/008AECB0.inc"
};
u8 TeostraBlastRing_ColorKeysMiddle[32] = {
#include "game/008AECE0.inc"
};
u8 TeostraBlastRing_ScaleKeysEnd[48] = {
#include "game/008AED00.inc"
};
u8 TeostraBlastRing_ColorKeysEnd[40] = {
#include "game/008AED30.inc"
};
u8 TeostraBlastRing_ModelKinds[8] = {
#include "game/008AED58.inc"
};
HitShape ChameleosAttack_HitShapes_0[2] = {
#include "game/008AED60.inc"
};
HitShape ChameleosAttack_HitShapes_1[3] = {
#include "game/008AEDB0.inc"
};
HitShape ChameleosAttack_HitShapes_2[3] = {
#include "game/008AEE30.inc"
};
HitShape ChameleosAttack_HitShapes_3[7] = {
#include "game/008AEEB0.inc"
};
HitShape ChameleosAttack_HitShapes_4[3] = {
#include "game/008AEFD0.inc"
};
HitShape ChameleosAttack_HitShapes_5[2] = {
#include "game/008AF050.inc"
};
HitShape ChameleosAttack_HitShapes_6[4] = {
#include "game/008AF0A0.inc"
};
HitShape *ChameleosAttack_HitShapes[8] = {
    ChameleosAttack_HitShapes_0, ChameleosAttack_HitShapes_1, ChameleosAttack_HitShapes_2, ChameleosAttack_HitShapes_3,
    ChameleosAttack_HitShapes_4, ChameleosAttack_HitShapes_5, ChameleosAttack_HitShapes_6, 0,
};
u32 ChameleosAttack_Rows[60] = {
#include "game/008AF160.inc"
};
void *ChameleosAttack_RowsPtr = (void *)ChameleosAttack_Rows;
HitShape RemobraAttack_HitShapes_0[4] = {
#include "game/008AF260.inc"
};
HitShape RemobraAttack_HitShapes_1[3] = {
#include "game/008AF300.inc"
};
HitShape *RemobraAttack_HitShapes[2] = {
    RemobraAttack_HitShapes_0, RemobraAttack_HitShapes_1,
};
u32 RemobraAttack_Rows[24] = {
#include "game/008AF380.inc"
};
void *RemobraAttack_RowsPtr = (void *)RemobraAttack_Rows;
char *gMsgEventTitles[80] = {
    (char *)D_008DE300, (char *)D_008DE320, (char *)D_008DE340, (char *)D_008DE360,
    (char *)D_008DE390, (char *)D_008DE3C0, (char *)D_008DE3E0, (char *)D_008DE3F8,
    (char *)D_008DE410, (char *)D_008DE430, (char *)D_008DE450, (char *)D_008DE470,
    (char *)D_008DE490, (char *)D_008DE4B0, (char *)D_008DE4C8, (char *)D_008DE4E0,
    (char *)D_008DE500, (char *)D_008DE520, (char *)D_008DE550, (char *)D_008DE570,
    (char *)D_008DE590, (char *)D_008DE5B0, (char *)D_008DE5E0, (char *)D_008DE600,
    (char *)D_008DE620, (char *)D_008DE640, (char *)D_008DE660, (char *)D_008DE680,
    (char *)D_008DE6A0, (char *)D_008DE6B0, (char *)D_008DE6D0, (char *)D_008DE6E0,
    (char *)D_008DE700, (char *)D_008DE718, (char *)D_008DE730, (char *)D_008DE750,
    (char *)D_008DE760, (char *)D_008DE780, (char *)D_008DE790, (char *)D_008DE7A8,
    (char *)D_008DE7C0, (char *)D_008DE7D8, (char *)D_008DE7F0, (char *)D_008DE810,
    (char *)D_008DE830, (char *)D_008DE850, (char *)D_008DE870, (char *)D_008DE890,
    (char *)D_008DE8B0, (char *)D_008DE8D0, (char *)D_008DE8E8, (char *)D_008DE8F8,
    (char *)D_008DE910, (char *)D_008DE930, (char *)D_008DE940, (char *)D_008DE960,
    (char *)D_008DE970, (char *)D_008DE990, (char *)D_008DE9B0, (char *)D_008DE9D0,
    (char *)D_008DE9E0, (char *)D_008DE9E8, (char *)D_008DE9F8, (char *)D_008DEA08,
    (char *)D_008DEA18, (char *)D_008DEA30, (char *)D_008DEA50, (char *)D_008DEA50,
    (char *)D_008DEA70, (char *)D_008DEA90, (char *)D_008DEAB0, (char *)D_008DEAD0,
    (char *)D_008DEAE0, (char *)D_008DEB00, (char *)D_008DEB20, (char *)D_008DEB40,
    (char *)D_008DEB58, NULL, NULL, NULL,
};
char *gMsgEventLines_0[6] = {
    (char *)D_008DEB70, (char *)D_008DEC20, (char *)D_008DECD0, (char *)D_008DED80,
    (char *)D_008DEE60, NULL,
};
char *gMsgEventLines_1[3] = {
    (char *)D_008DEF30, (char *)D_008DEFE0, (char *)D_008DF0A0,
};
char *gMsgEventLines_2[4] = {
    (char *)D_008DF170, (char *)D_008DF1E0, (char *)D_008DF2A0, (char *)D_008DF360,
};
char *gMsgEventLines_3[4] = {
    (char *)D_008DF400, (char *)D_008DF4B0, (char *)D_008DF580, NULL,
};
char *gMsgEventLines_4[4] = {
    (char *)D_008DF660, (char *)D_008DF730, (char *)D_008DF7D0, NULL,
};
char *gMsgEventLines_5[4] = {
    (char *)D_008DF880, (char *)D_008DF920, (char *)D_008DFA10, (char *)D_008DFAB0,
};
char *gMsgEventLines_6[4] = {
    (char *)D_008DFB60, (char *)D_008DFC10, (char *)D_008DFD10, NULL,
};
char *gMsgEventLines_7[8] = {
    (char *)D_008DFDD0, (char *)D_008DFE60, (char *)D_008DFF30, (char *)D_008E0040,
    (char *)D_008E0100, (char *)D_008E0190, (char *)D_008E0290, NULL,
};
char *gMsgEventLines_8[4] = {
    (char *)D_008E0320, (char *)D_008E0400, (char *)D_008E04C0, (char *)D_008E0580,
};
char *gMsgEventLines_9[4] = {
    (char *)D_008E0620, NULL, NULL, NULL,
};
char *gMsgEventLines_10[8] = {
    (char *)D_008E0710, (char *)D_008E07A0, (char *)D_008E0850, (char *)D_008E0930,
    (char *)D_008E0A30, NULL, NULL, NULL,
};
char *gMsgEventLines_11[8] = {
    (char *)D_008E0AE0, (char *)D_008E0B40, (char *)D_008E0C20, (char *)D_008E0CB0,
    (char *)D_008E0D60, NULL, NULL, NULL,
};
char *gMsgEventLines_12[8] = {
    (char *)D_008E0E20, (char *)D_008E0ED0, (char *)D_008E0F90, (char *)D_008E1070,
    (char *)D_008E1130, (char *)D_008E11E0, NULL, NULL,
};
char *gMsgEventLines_13[8] = {
    (char *)D_008E1260, (char *)D_008E1310, (char *)D_008E13C0, (char *)D_008E1480,
    (char *)D_008E1530, NULL, NULL, NULL,
};
char *gMsgEventLines_14[4] = {
    (char *)D_008E1610, (char *)D_008E1690, (char *)D_008E1750, (char *)D_008E1810,
};
char *gMsgEventLines_15[2] = {
    (char *)D_008E18B0, NULL,
};
char *gMsgEventLines_16[3] = {
    (char *)D_008E19B0, (char *)D_008E1A40, (char *)D_008E1AE0,
};
char *gMsgEventLines_17[4] = {
    (char *)D_008E1B80, (char *)D_008E1C00, (char *)D_008E1C90, (char *)D_008E1D30,
};
char *gMsgEventLines_18[8] = {
    (char *)D_008E1DC0, (char *)D_008E1E80, (char *)D_008E1F60, (char *)D_008E2040,
    (char *)D_008E20E0, NULL, NULL, NULL,
};
char *gMsgEventLines_19[4] = {
    (char *)D_008E21B0, (char *)D_008E2240, (char *)D_008E2300, (char *)D_008E2360,
};
char *gMsgEventLines_20[6] = {
    (char *)D_008E23F0, (char *)D_008E2480, (char *)D_008E2530, (char *)D_008E2610,
    (char *)D_008E26D0, NULL,
};
char *gMsgEventLines_21[2] = {
    (char *)D_008E27B0, NULL,
};
char *gMsgEventLines_22[6] = {
    (char *)D_008E2890, (char *)D_008E2920, (char *)D_008E29D0, (char *)D_008E2AA0,
    (char *)D_008E2B60, NULL,
};
char *gMsgEventLines_23[3] = {
    (char *)D_008E2C50, (char *)D_008E2CF0, (char *)D_008E2DA0,
};
char *gMsgEventLines_24[2] = {
    (char *)D_008E2E70, NULL,
};
char *gMsgEventLines_25[8] = {
    (char *)D_008E2F70, (char *)D_008E2FF0, (char *)D_008E3090, (char *)D_008E3120,
    (char *)D_008E3200, NULL, NULL, NULL,
};
char *gMsgEventLines_26[6] = {
    (char *)D_008E32C0, (char *)D_008E3380, (char *)D_008E3420, (char *)D_008E34C0,
    (char *)D_008E3550, (char *)D_008E35D0,
};
char *gMsgEventLines_27[2] = {
    (char *)D_008E3670, NULL,
};
char *gMsgEventLines_28[6] = {
    (char *)D_008E3770, (char *)D_008E37F0, (char *)D_008E38B0, (char *)D_008E3980,
    (char *)D_008E3A30, (char *)D_008E3AD0,
};
char *gMsgEventLines_29[3] = {
    (char *)D_008E3B70, (char *)D_008E3C10, (char *)D_008E3CD0,
};
char *gMsgEventLines_30[6] = {
    (char *)D_008E3D60, (char *)D_008E37F0, (char *)D_008E3DE0, (char *)D_008E3EC0,
    (char *)D_008E3F90, NULL,
};
char *gMsgEventLines_31[3] = {
    (char *)D_008E4070, (char *)D_008E4120, (char *)D_008E41E0,
};
char *gMsgEventLines_32[2] = {
    (char *)D_008E42B0, (char *)D_008E4380,
};
char *gMsgEventLines_33[6] = {
    (char *)D_008E4450, (char *)D_008E37F0, (char *)D_008E44E0, (char *)D_008E45A0,
    (char *)D_008E4670, (char *)D_008E4750,
};
char *gMsgEventLines_34[3] = {
    (char *)D_008E4800, (char *)D_008E4890, (char *)D_008E4960,
};
char *gMsgEventLines_35[8] = {
    (char *)D_008E4A20, (char *)D_008E37F0, (char *)D_008E4AA0, (char *)D_008E4B80,
    (char *)D_008E4C60, (char *)D_008E4D50, NULL, NULL,
};
char *gMsgEventLines_36[4] = {
    (char *)D_008E3B70, (char *)D_008E3C10, (char *)D_008E4E10, (char *)D_008E4EA0,
};
char *gMsgEventLines_37[6] = {
    (char *)D_008E4F50, (char *)D_008E37F0, (char *)D_008E4FD0, (char *)D_008E5090,
    (char *)D_008E5130, (char *)D_008E51E0,
};
char *gMsgEventLines_38[3] = {
    (char *)D_008E52C0, (char *)D_008E53A0, (char *)D_008E5470,
};
char *gMsgEventLines_39[8] = {
    (char *)D_008E5560, (char *)D_008E37F0, (char *)D_008E55E0, (char *)D_008E5690,
    (char *)D_008E5760, NULL, NULL, NULL,
};
char *gMsgEventLines_40[4] = {
    (char *)D_008E5830, (char *)D_008E5920, (char *)D_008E59E0, (char *)D_008E5AA0,
};
char *gMsgEventLines_41[8] = {
    (char *)D_008E5B60, (char *)D_008E37F0, (char *)D_008E5BE0, (char *)D_008E5CB0,
    (char *)D_008E5D90, NULL, NULL, NULL,
};
char *gMsgEventLines_42[4] = {
    (char *)D_008E5E50, (char *)D_008E5F40, (char *)D_008E6010, (char *)D_008E60F0,
};
char *gMsgEventLines_43[8] = {
    (char *)D_008E61E0, (char *)D_008E37F0, (char *)D_008E6270, (char *)D_008E6340,
    (char *)D_008E6410, (char *)D_008E64F0, NULL, NULL,
};
char *gMsgEventLines_44[4] = {
    (char *)D_008E65C0, (char *)D_008E6680, (char *)D_008E6750, (char *)D_008E67F0,
};
char *gMsgEventLines_45[4] = {
    (char *)D_008E68C0, (char *)D_008E69A0, (char *)D_008E6A80, (char *)D_008E6B50,
};
char *gMsgEventLines_46[8] = {
    (char *)D_008E6C00, (char *)D_008E6C90, (char *)D_008E6D40, (char *)D_008E6DD0,
    (char *)D_008E6E70, NULL, NULL, NULL,
};
char *gMsgEventLines_47[4] = {
    (char *)D_008E6F40, (char *)D_008E6FE0, (char *)D_008E7060, (char *)D_008E7130,
};
char *gMsgEventLines_48[8] = {
    (char *)D_008E71A0, (char *)D_008E6C90, (char *)D_008E7230, (char *)D_008E72C0,
    (char *)D_008E7390, NULL, NULL, NULL,
};
char *gMsgEventLines_49[4] = {
    (char *)D_008E7450, (char *)D_008E74F0, (char *)D_008E75C0, (char *)D_008E7130,
};
char *gMsgEventLines_50[6] = {
    (char *)D_008E76A0, (char *)D_008E7720, (char *)D_008E77E0, (char *)D_008E78C0,
    (char *)D_008E79A0, NULL,
};
char *gMsgEventLines_51[2] = {
    (char *)D_008E7A50, (char *)D_008E7B00,
};
char *gMsgEventLines_52[4] = {
    (char *)D_008E7BA0, (char *)D_008E7C70, (char *)D_008E7D10, (char *)D_008E7DE0,
};
char *gMsgEventLines_53[4] = {
    (char *)D_008E7E90, (char *)D_008E7F20, (char *)D_008E7FD0, NULL,
};
char *gMsgEventLines_54[4] = {
    (char *)D_008E8060, (char *)D_008E8100, (char *)D_008E8180, NULL,
};
extern char D_008E8240[];
extern char D_008E8330[];
extern char D_008E8E70[];
extern char D_008E8F20[];
extern char D_008E91C0[];
extern char D_008E9370[];
extern char D_008E9420[];
extern char D_008E94D0[];
extern char D_008E9F70[];
char *gMsgEventLines_55[6] = {
    D_008E8240, D_008E8330, (char *)D_008E8400, (char *)D_008E84D0,
    (char *)D_008E8590, NULL,
};
char *gMsgEventLines_56[2] = {
    (char *)D_008E8660, NULL,
};
char *gMsgEventLines_57[8] = {
    (char *)D_008E8750, (char *)D_008E87E0, (char *)D_008E88D0, (char *)D_008E8960,
    (char *)D_008E8A20, (char *)D_008E8B10, (char *)D_008E8BE0, NULL,
};
char *gMsgEventLines_58[4] = {
    (char *)D_008E8C90, (char *)D_008E8D20, (char *)D_008E8DB0, NULL,
};
char *gMsgEventLines_59[4] = {
    D_008E8E70, D_008E8F20, (char *)D_008E9000, (char *)D_008E90F0,
};
char *gMsgEventLines_60[2] = {
    D_008E91C0, (char *)D_008E92A0,
};
char *gMsgEventLines_61[3] = {
    D_008E9370, D_008E9420, D_008E94D0,
};
char *gMsgEventLines_62[2] = {
    (char *)D_008E95A0, (char *)D_008E9680,
};
char *gMsgEventLines_63[2] = {
    (char *)D_008E9700, (char *)D_008E97E0,
};
char *gMsgEventLines_64[2] = {
    (char *)D_008E98C0, (char *)D_008E9980,
};
char *gMsgEventLines_65[6] = {
    (char *)D_008E9A00, (char *)D_008E9AA0, (char *)D_008E9B30, (char *)D_008E9C00,
    (char *)D_008E9CC0, NULL,
};
char *gMsgEventLines_66[2] = {
    (char *)D_008E9DA0, NULL,
};
char *gMsgEventLines_67[2] = {
    (char *)D_008E9E90, NULL,
};
char *gMsgEventLines_68[3] = {
    D_008E9F70, (char *)D_008EA050, (char *)D_008EA120,
};
char *gMsgEventLines_69[8] = {
    (char *)D_008EA190, (char *)D_008EA230, (char *)D_008EA300, (char *)D_008EA3E0,
    (char *)D_008EA4B0, NULL, NULL, NULL,
};
char *gMsgEventLines_70[8] = {
    (char *)D_008EA550, (char *)D_008EA610, (char *)D_008EA680, (char *)D_008EA760,
    (char *)D_008EA800, (char *)D_008EA8F0, (char *)D_008EA9B0, (char *)D_008EAA70,
};
char *gMsgEventLines_71[8] = {
    (char *)D_008EAB30, (char *)D_008EABF0, (char *)D_008EACB0, (char *)D_008EAD50,
    (char *)D_008EAE10, (char *)D_008EAEA0, (char *)D_008EAF30, (char *)D_008EAFB0,
};
char *gMsgEventLines_72[4] = {
    (char *)D_008EB030, (char *)D_008EB0D0, (char *)D_008EB190, (char *)D_008EB260,
};
char *gMsgEventLines_73[4] = {
    (char *)D_008EB300, (char *)D_008EB3E0, (char *)D_008EB440, (char *)D_008EB500,
};
char *gMsgEventLines_74[8] = {
    (char *)D_008EB570, (char *)D_008EB5D0, (char *)D_008EB660, (char *)D_008EB6F0,
    (char *)D_008EB770, NULL, NULL, NULL,
};
char *gMsgEventLines_75[8] = {
    (char *)D_008EB800, (char *)D_008EB850, (char *)D_008EB8F0, (char *)D_008EB970,
    (char *)D_008EB9E0, (char *)D_008EBA20, NULL, NULL,
};
char *gMsgEventLines_76[12] = {
    (char *)D_008EBA90, (char *)D_008EBB10, (char *)D_008EBBA0, (char *)D_008EBC70,
    (char *)D_008EBD30, (char *)D_008EBDD0, (char *)D_008EBE90, (char *)D_008EBF50,
    (char *)D_008EC020, NULL, NULL, NULL,
};
char* *gMsgEventLines[80] = {
    gMsgEventLines_0, gMsgEventLines_1, gMsgEventLines_2, gMsgEventLines_3,
    gMsgEventLines_4, gMsgEventLines_5, gMsgEventLines_6, gMsgEventLines_7,
    gMsgEventLines_8, gMsgEventLines_9, gMsgEventLines_10, gMsgEventLines_11,
    gMsgEventLines_12, gMsgEventLines_13, gMsgEventLines_14, gMsgEventLines_15,
    gMsgEventLines_16, gMsgEventLines_17, gMsgEventLines_18, gMsgEventLines_19,
    gMsgEventLines_20, gMsgEventLines_21, gMsgEventLines_22, gMsgEventLines_23,
    gMsgEventLines_24, gMsgEventLines_25, gMsgEventLines_26, gMsgEventLines_27,
    gMsgEventLines_28, gMsgEventLines_29, gMsgEventLines_30, gMsgEventLines_31,
    gMsgEventLines_32, gMsgEventLines_33, gMsgEventLines_34, gMsgEventLines_35,
    gMsgEventLines_36, gMsgEventLines_37, gMsgEventLines_38, gMsgEventLines_39,
    gMsgEventLines_40, gMsgEventLines_41, gMsgEventLines_42, gMsgEventLines_43,
    gMsgEventLines_44, gMsgEventLines_45, gMsgEventLines_46, gMsgEventLines_47,
    gMsgEventLines_48, gMsgEventLines_49, gMsgEventLines_50, gMsgEventLines_51,
    gMsgEventLines_52, gMsgEventLines_53, gMsgEventLines_54, gMsgEventLines_55,
    gMsgEventLines_56, gMsgEventLines_57, gMsgEventLines_58, gMsgEventLines_59,
    gMsgEventLines_60, gMsgEventLines_61, gMsgEventLines_62, gMsgEventLines_63,
    gMsgEventLines_64, gMsgEventLines_65, gMsgEventLines_66, gMsgEventLines_67,
    gMsgEventLines_68, gMsgEventLines_69, gMsgEventLines_70, gMsgEventLines_71,
    gMsgEventLines_72, gMsgEventLines_73, gMsgEventLines_74, gMsgEventLines_75,
    gMsgEventLines_76, NULL, NULL, NULL,
};
u16 gMsgEventFlags[28][2] = {
#include "game/008AFCB0.inc"
};
u8 gMsgEvents_0[24] = {
#include "game/008AFD20.inc"
};
u8 gMsgEvents_1[12] = {
#include "game/008AFD38.inc"
};
u8 gMsgEvents_2[12] = {
#include "game/008AFD48.inc"
};
u8 gMsgEvents_3[8] = {
#include "game/008AFD58.inc"
};
u8 gMsgEvents_4[8] = {
#include "game/008AFD60.inc"
};
u8 gMsgEvents_5[8] = {
#include "game/008AFD68.inc"
};
u8 gMsgEvents_6[8] = {
#include "game/008AFD70.inc"
};
u8 gMsgEvents_7[8] = {
#include "game/008AFD78.inc"
};
u8 gMsgEvents_8[8] = {
#include "game/008AFD80.inc"
};
u8 gMsgEvents_9[8] = {
#include "game/008AFD88.inc"
};
u8 gMsgEvents_10[8] = {
#include "game/008AFD90.inc"
};
u8 gMsgEvents_11[8] = {
#include "game/008AFD98.inc"
};
u8 gMsgEvents_12[8] = {
#include "game/008AFDA0.inc"
};
u8 gMsgEvents_13[8] = {
#include "game/008AFDA8.inc"
};
u8 gMsgEvents_14[8] = {
#include "game/008AFDB0.inc"
};
u8 gMsgEvents_15[8] = {
#include "game/008AFDB8.inc"
};
u8 gMsgEvents_16[16] = {
#include "game/008AFDC0.inc"
};
u8 gMsgEvents_17[24] = {
#include "game/008AFDD0.inc"
};
u8 gMsgEvents_18[8] = {
#include "game/008AFDE8.inc"
};
u8 gMsgEvents_19[8] = {
#include "game/008AFDF0.inc"
};
u8 gMsgEvents_20[8] = {
#include "game/008AFDF8.inc"
};
u8 gMsgEvents_21[8] = {
#include "game/008AFE00.inc"
};
u8 gMsgEvents_22[8] = {
#include "game/008AFE08.inc"
};
u8 gMsgEvents_23[8] = {
#include "game/008AFE10.inc"
};
u8 gMsgEvents_24[8] = {
#include "game/008AFE18.inc"
};
u8 *gMsgEvents[28] = {
    gMsgEvents_0, gMsgEvents_1, gMsgEvents_2, gMsgEvents_3,
    gMsgEvents_4, gMsgEvents_5, gMsgEvents_6, gMsgEvents_7,
    gMsgEvents_8, gMsgEvents_9, gMsgEvents_10, gMsgEvents_11,
    gMsgEvents_12, gMsgEvents_13, gMsgEvents_14, gMsgEvents_15,
    gMsgEvents_16, gMsgEvents_17, gMsgEvents_18, gMsgEvents_19,
    gMsgEvents_20, gMsgEvents_21, gMsgEvents_22, gMsgEvents_23,
    gMsgEvents_24, NULL, NULL, NULL,
};
ListPanel gMsgEventListPanel = {
#include "game/008AFE90.inc"
};
Panel gMsgEventPopupWin = {
#include "game/008AFEB0.inc"
};
u8 gMsgEventIconShown[32] = {
#include "game/008AFED0.inc"
};
MsgEvent gMsgEventCheck0[28] = {
#include "game/008AFEF0.inc"
};
MsgEvent gMsgEventCheck1[4] = {
#include "game/008AFFD0.inc"
};
MsgEvent gMsgEventCheck15[8] = {
#include "game/008AFFF0.inc"
};
MsgEvent gMsgEventCheck12[4] = {
#include "game/008B0030.inc"
};
MsgEvent gMsgEventCheck14[4] = {
#include "game/008B0050.inc"
};
MsgEvent gMsgEventCheck5[6] = {
#include "game/008B0070.inc"
};
MsgEvent gMsgEventCheck8[3] = {
#include "game/008B00A0.inc"
};
f32 AreaObj_K00_PosArea29[3] = {
#include "game/008B00B8.inc"
};
s16 AreaObj_K00_RotX[4] = {
#include "game/008B00C8.inc"
};
f32 AreaObj_K00_Scale[2] = {
#include "game/008B00D0.inc"
};
f32 D_008B00D8[2] = {
#include "game/008B00D8.inc"
};
f32 AreaObj_K00_PosArea2A[6] = {
#include "game/008B00E0.inc"
};
s16 AreaObj_K00_RotZA[4] = {
#include "game/008B00F8.inc"
};
f32 AreaObj_K00_DrawF[2] = {
#include "game/008B0100.inc"
};
f32 AreaObj_K00_PosArea2B[3] = {
#include "game/008B0108.inc"
};
s16 AreaObj_K00_RotZB[4] = {
#include "game/008B0118.inc"
};
f32 AreaObj_K00_PosArea5C[4] = {
#include "game/008B0120.inc"
};
f32 AreaObj_K00_PosArea5E[4] = {
#include "game/008B0130.inc"
};
f32 AreaObj_K00_PosArea82[8] = {
#include "game/008B0140.inc"
};
f32 AreaObj_K00_PosArea32[6] = {
#include "game/008B0160.inc"
};
s16 AreaObj_K00_Models[4] = {
#include "game/008B0178.inc"
};
f32 AreaObj_K00_PosArea44[8] = {
#include "game/008B0180.inc"
};
f32 AreaObj_K00_PosArea45[6] = {
#include "game/008B01A0.inc"
};
s16 AreaObj_K02_Counts[4] = {
#include "game/008B01B8.inc"
};
s16 AreaObj_K02_RandRange[4] = {
#include "game/008B01C0.inc"
};
s16 AreaObj_K02_WaitBase[4] = {
#include "game/008B01C8.inc"
};
u16 AreaObj_K02_WaitMask[4] = {
#include "game/008B01D0.inc"
};
s16 AreaObj_K02_Frames[4] = {
#include "game/008B01D8.inc"
};
s16 AreaObj_K02_Models[4] = {
#include "game/008B01E0.inc"
};
f32 AreaObj_K03_Sc32[3] = {
#include "game/008B01E8.inc"
};
f32 AreaObj_K03_R32[2] = {
#include "game/008B01F8.inc"
};
f32 AreaObj_K03_Sc82[4] = {
#include "game/008B0200.inc"
};
f32 AreaObj_K03_R82[2] = {
#include "game/008B0210.inc"
};
f32 AreaObj_K03_Sc90[3] = {
#include "game/008B0218.inc"
};
f32 AreaObj_K03_R90[2] = {
#include "game/008B0228.inc"
};
f32 AreaObj_K03_Points[12] = {
#include "game/008B0230.inc"
};
f32 AreaObj_K03_Sc44[6] = {
#include "game/008B0260.inc"
};
f32 AreaObj_K03_R44[2] = {
#include "game/008B0278.inc"
};
f32 AreaObj_K03_Sc9B[6] = {
#include "game/008B0280.inc"
};
f32 AreaObj_K03_R9B[2] = {
#include "game/008B0298.inc"
};
f32 AreaObj_K03_Sc49[6] = {
#include "game/008B02A0.inc"
};
f32 AreaObj_K03_R49[2] = {
#include "game/008B02B8.inc"
};
u16 AreaObj_K05_Sounds[4] = {
#include "game/008B02C0.inc"
};
u8 AreaObj_K05_Kinds_0[8] = {
#include "game/008B02C8.inc"
};
f32 AreaObj_K05_Params_0[10] = {
#include "game/008B02D0.inc"
};
u8 AreaObj_K05_Kinds_1[8] = {
#include "game/008B02F8.inc"
};
f32 AreaObj_K05_Frames6_0[4] = {
#include "game/008B0300.inc"
};
f32 AreaObj_K05_Frames6_1[4] = {
#include "game/008B0310.inc"
};
f32 AreaObj_K05_Frames6_2[4] = {
#include "game/008B0320.inc"
};
f32 AreaObj_K05_Frames6_3[4] = {
#include "game/008B0330.inc"
};
f32 AreaObj_K05_Frames6_4[4] = {
#include "game/008B0340.inc"
};
f32 *AreaObj_K05_Frames6[8] = {
    AreaObj_K05_Frames6_0, AreaObj_K05_Frames6_1, AreaObj_K05_Frames6_2, AreaObj_K05_Frames6_3,
    AreaObj_K05_Frames6_4, NULL, NULL, NULL,
};
f32 AreaObj_K05_SpawnPos6[12] = {
#include "game/008B0370.inc"
};
u8 AreaObj_K05_Kinds_2[8] = {
#include "game/008B03A0.inc"
};
f32 AreaObj_K05_Frames2_0[3] = {
#include "game/008B03A8.inc"
};
f32 AreaObj_K05_Frames2_1[3] = {
#include "game/008B03B8.inc"
};
f32 AreaObj_K05_Frames2_2[3] = {
#include "game/008B03C8.inc"
};
f32 *AreaObj_K05_Frames2[3] = {
    AreaObj_K05_Frames2_0, AreaObj_K05_Frames2_1, AreaObj_K05_Frames2_2,
};
u16 AreaObj_K05_Angles2[4] = {
#include "game/008B03E8.inc"
};
f32 AreaObj_K05_SpawnPos2[4] = {
#include "game/008B03F0.inc"
};
u8 AreaObj_K05_Kinds_3[16] = {
#include "game/008B0400.inc"
};
f32 AreaObj_K05_Params_3[6] = {
#include "game/008B0410.inc"
};
u8 AreaObj_K05_Kinds_4[8] = {
#include "game/008B0428.inc"
};
f32 AreaObj_K05_Params_4[10] = {
#include "game/008B0430.inc"
};
u8 AreaObj_K05_Kinds_8[8] = {
#include "game/008B0458.inc"
};
f32 AreaObj_K05_Frames8_0[4] = {
#include "game/008B0460.inc"
};
f32 AreaObj_K05_Frames8_1[4] = {
#include "game/008B0470.inc"
};
f32 AreaObj_K05_Frames8_2[6] = {
#include "game/008B0480.inc"
};
f32 *AreaObj_K05_Frames8[3] = {
    AreaObj_K05_Frames8_0, AreaObj_K05_Frames8_1, AreaObj_K05_Frames8_2,
};
u16 AreaObj_K05_Unk008_8[4] = {
#include "game/008B04A8.inc"
};
f32 AreaObj_K05_SpawnPos8[6] = {
#include "game/008B04B0.inc"
};
u8 AreaObj_K05_Kinds_9[8] = {
#include "game/008B04C8.inc"
};
f32 AreaObj_K05_Frames9_0[4] = {
#include "game/008B04D0.inc"
};
f32 AreaObj_K05_Frames9_1[4] = {
#include "game/008B04E0.inc"
};
f32 AreaObj_K05_Frames9_2[4] = {
#include "game/008B04F0.inc"
};
f32 AreaObj_K05_Frames9_3[4] = {
#include "game/008B0500.inc"
};
f32 *AreaObj_K05_Frames9[4] = {
    AreaObj_K05_Frames9_0, AreaObj_K05_Frames9_1, AreaObj_K05_Frames9_2, AreaObj_K05_Frames9_3,
};
u16 AreaObj_K05_AreaVariants[16] = {
#include "game/008B0520.inc"
};
f32 *AreaObj_K05_Params[12] = {
    AreaObj_K05_Params_0, (f32 *)AreaObj_K05_Frames6, (f32 *)AreaObj_K05_Frames2, AreaObj_K05_Params_3,
    AreaObj_K05_Params_4, AreaObj_K05_Params_0, (f32 *)AreaObj_K05_Frames6, AreaObj_K05_Params_4,
    (f32 *)AreaObj_K05_Frames8, (f32 *)AreaObj_K05_Frames9, NULL, NULL,
};
u8 *AreaObj_K05_Kinds[12] = {
    AreaObj_K05_Kinds_0, AreaObj_K05_Kinds_1, AreaObj_K05_Kinds_2, AreaObj_K05_Kinds_3,
    AreaObj_K05_Kinds_4, AreaObj_K05_Kinds_0, AreaObj_K05_Kinds_1, AreaObj_K05_Kinds_4,
    AreaObj_K05_Kinds_8, AreaObj_K05_Kinds_9, NULL, NULL,
};
s16 AreaObj_K05_Models_0[24] = {
#include "game/008B05A0.inc"
};
s16 AreaObj_K05_Models_1[24] = {
#include "game/008B05D0.inc"
};
s16 AreaObj_K05_Models_2[24] = {
#include "game/008B0600.inc"
};
s16 AreaObj_K05_Models_3[24] = {
#include "game/008B0630.inc"
};
s16 *AreaObj_K05_Models[4] = {
    AreaObj_K05_Models_0, AreaObj_K05_Models_1, AreaObj_K05_Models_2, AreaObj_K05_Models_3,
};
f32 AreaObj_K07_PosArea2A[24] = {
#include "game/008B0670.inc"
};
f32 AreaObj_K07_Scale[24] = {
#include "game/008B06D0.inc"
};
f32 AreaObj_K07_PosArea61[18] = {
#include "game/008B0730.inc"
};
s16 AreaObj_K07_Models1[6] = {
#include "game/008B0778.inc"
};
s16 AreaObj_K07_SpinDef[6] = {
#include "game/008B0788.inc"
};
s16 AreaObj_K07_Models69[4] = {
#include "game/008B0798.inc"
};
f32 AreaObj_K07_PosArea6A[26] = {
#include "game/008B07A0.inc"
};
s16 AreaObj_K07_Spin1[6] = {
#include "game/008B0808.inc"
};
s16 AreaObj_K07_Models6D[4] = {
#include "game/008B0818.inc"
};
f32 AreaObj_K07_PosAreaB3[40] = {
#include "game/008B0820.inc"
};
s16 AreaObj_K07_ModelsB7[16] = {
#include "game/008B08C0.inc"
};
f32 AreaObj_K08_Steps[64] = {
#include "game/008B08E0.inc"
};
f32 AreaObj_K09_Type2Pos[12] = {
#include "game/008B09E0.inc"
};
