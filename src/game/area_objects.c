/*
 * The attack families ChameleosAttack and RemobraAttack, the quest advice
 * messages (MsgEvent) and the first area objects.
 *
 * An attack family is a kind of ObjAttach on sub_main's object list: its
 * Create fills the object and its Update, run every frame, switches on
 * ObjAttach.state (enum ObjAttachState) to its Setup, Run, End and Free; its
 * type picks a row of its attack tables.
 *
 * The advice messages show in quests with advice (Quest_HasAdvice). A message
 * event (MsgEvent_Check, called by the quest, the player's states, the supply
 * box and the Apceros) picks a message once per save, recorded in event
 * flags; MsgEvent_OpenPopup shows it in a popup, and the offline field menu
 * lists the quest set's messages already seen.
 *
 * Their state is QuestMsgWin (gQuestMsgWindow); gMsgEventSets,
 * gMsgEventSetEntries, gMsgEventTitles and gMsgEventLines give each set's
 * messages, titles and pages.
 *
 * An area object is an Obj that Area_StartObjects makes for the area just
 * entered: per kind a creator, an update that switches on Obj.state (0 Setup,
 * 1 Run, 2 End, 3 Free) and a draw callback on a DrawSlot that draws models
 * of the area's model set (gAreaSubModels[0]).
 *
 * The kinds here: LightShaft, SkyStreak, AreaDrop, AreaFlag, AreaWeapon,
 * AnimatedProps, AnimatedGroundTiles and AmbientLife (the area's small
 * animals and leaves; its types 0 to 7 are set up here, the later types run
 * with the area objects that follow).
 */
#include "common.h"
#include "msg_event.h"
#include "render_state.h"
#include "hud.h"
#include "sound.h"
#include "pad.h"
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
#include "texture.h"
/* Declarations of the functions called below. */
s32 Player_TestSetFlags(void *, s32 mask);
void ObjAttach_StartOwnedAttack(ObjAttach *, s32 a, s32 b, void *c);
s32 Quest_PhaseIsOver(s32);
s32 Game_IsOnline(void);
s32 AreaWeapon_GunIndex(Obj *o);

/* An entry of a dated message-event list (MsgEvent_CheckList): in quest questId, event flag flag is
 * set once, message msg is requested (QuestMsgWin.request) and the seen flags 200 + ev[i] of the
 * messages it unlocks are set. A negative flag ends the list. */
typedef struct MsgEvent {
    s16 flag;
    u16 questId;
    u8 msg;
    u8 ev[3];
} MsgEvent;

/* prototypes */
extern u16 gFrameTick;
/* unit's own declaration: declared here to return s32 though the function returns s16: AnimatedProps_Run compiles as the game's code only when it takes the result as an s32. */
s32 Area_MonsterState(u16 area);
extern s16 AnimatedProps_SpinSnowyMountains6[];
extern s16 AnimatedProps_SpinSnowyNight6[];
typedef struct AnimatedPropsBuf {
    u8 state[13];               /* 0x00 */
    u8 level[13];               /* 0x0D */
    u16 rot[13];                /* 0x1A */
    s16 slot[13];               /* 0x34: the draw slots (DrawSlot_Alloc) */
    u8 pad4E[0x50 - 0x4E];
    DrawSlot *ds[13];           /* 0x50 */
} AnimatedPropsBuf;
u16 Mtx_Heading(f32 *m);
void Vec_RotateY(f32 *v, f32 angle);
void Mtx_GetTrans(f32 *out, f32 *m);
f32 Vec_Distance(f32 *a, f32 *b);
extern f32 AreaDrop_ZonesDesertNight4[];
extern f32 AreaDrop_ZoneRadiiDesertNight4[];
extern f32 AreaDrop_ZonesTower8[];
extern f32 AreaDrop_ZoneRadiiTower8[];
extern f32 AreaDrop_ZonesDesert4[];
extern f32 AreaDrop_ZoneRadiiDesert4[];
extern f32 AreaDrop_ZonesSwamp3[];
extern f32 AreaDrop_ZoneRadiiSwamp3[];
extern f32 AreaDrop_ZonesSwamp4[];
extern f32 AreaDrop_ZoneRadiiSwamp4[];
extern f32 AreaDrop_ZonesSwamp8[];
extern f32 AreaDrop_ZoneRadiiSwamp8[];
f32 Math_Sin(f32 x);
void Mtx_Copy(f32 *dst, f32 *src);
void Gfx_SetBlend(s32 a, s32 b);
void Mtx_ScaleBy(f32 *m, f32 x, f32 y, f32 z);
extern f32 gCamYawMtx[];
extern s16 LightShaft_TiltsForestHills7[];
extern f32 LightShaft_ScalesForestHills7[];
/* The y and z of the scale triples: the three loads compile as the game's code only from three symbols. */
extern f32 LightShaft_ScalesForestHills7Y[];
extern f32 LightShaft_ScalesForestHills7Z[];
extern s16 LightShaft_TiltsForestHills11[];
extern f32 LightShaft_WidthsForestHills11[];
extern s16 LightShaft_TiltsForestHills12[];
extern s16 LightShaft_ModelsDesert4[];
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_TurnY(f32 *m, f32 a);
void Mtx_TurnZ(f32 *m, f32 a);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
/* The buffer of AmbientLife's movers (falling leaf, wanderer, darter, steady flyer, spinner,
 * circling bird): m is the orientation they draw with, v the velocity in their own frame, rot the
 * angles; the wanderer uses spinX as its pitching mode and spinZ as that mode's frames, the darter
 * both as frame counters. */
typedef struct AmbientLifeMover {
    f32 m[16];                  /* 0x00: the orientation */
    f32 v[3];                   /* 0x40: the velocity, in the object's frame */
    u16 rot[3];                 /* 0x4C: the angles about x, y and z */
    s16 spinX;                  /* 0x52 */
    s16 circleStep;             /* 0x54: the circling bird's turn a frame */
    s16 spinZ;                  /* 0x56 */
} AmbientLifeMover;
void PartFx_SpawnAt(f32 *pos, s32 kind, f32 scale);
void SoundObj_Start(s32 bank, s32 se, s32 k, f32 *pos, s16 n, u8 quiet);
void AreaDrop_RunDrip(Obj *o);
void AreaDrop_RunWeaponBurst(Obj *o);
extern f32 AreaDrop_FallPointsDesert10[];
void MsgEvent_CopyQuestTitle(s8 *arg0);
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
extern f32 LightShaft_PosForestHills7[];
extern f32 LightShaft_PosForestHills11[];
extern f32 LightShaft_PosForestHills12[];
extern f32 LightShaft_PosSnowyMountains1[];
extern f32 LightShaft_PosSnowyMountains3[];
extern f32 LightShaft_PosTower8[];
extern f32 LightShaft_PosDesert4[];
extern f32 LightShaft_PosSwamp3[];
extern f32 LightShaft_PosSwamp4[];
void LightShaft_Draw(DrawSlot *ds);
typedef struct LightShaftBuf {
    s16 slot[6];
    DrawSlot *ds[6];
} LightShaftBuf;
/* The buffer of AmbientLife's butterflies (AmbientLife_SetupButterfly sets it up,
 * AmbientLife_RunButterfly moves it): a draw slot per wing; pitch is the wing angle, goal the angle
 * it beats to. */
typedef struct ButterflyBuf {
    s16 slot[2];
    f32 speed;                  /* 0x04: forward */
    f32 side;                   /* 0x08 */
    u16 heading;                /* 0x0C */
    s16 pitch;                  /* 0x0E */
    u16 goal;                   /* 0x10: the pitch it turns to */
    s16 steps;                  /* 0x12: the frames left to reach it */
    DrawSlot *ds[2];
} ButterflyBuf;
f32 Math_Floor(f32 x);
void Gfx_SetTexFilter(s32 a);
extern f32 AnimatedGroundTiles_UvSteps[];
s32 QuestGoal_DeliveryDone(void);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void AreaFlag_Draw(DrawSlot *ds);
void AreaFlag_Run(Obj *o);
extern f32 AmbientLife_WandererPoints[];
/* The y and z of the wanderers' points: AmbientLife_SetupWanderer compiles as the game's code only with them as symbols of their own. */
extern f32 AmbientLife_WandererPointsY[];
extern f32 AmbientLife_WandererPointsZ[];
f32 Math_Atan2(f32 y, f32 x);
void AmbientLife_SetupSpawner(Obj *o);
void AmbientLife_SetupFallingLeaf(Obj *o);
void AmbientLife_SetupWanderer(Obj *o);
void AmbientLife_SetupDarter(Obj *o);
void AmbientLife_SetupSteadyFlyer(Obj *o);
void AmbientLife_SetupButterfly(Obj *o);
void AmbientLife_SetupCirclingBird(Obj *o);
void AmbientLife_SetupGlowBug(Obj *o);
void AmbientLife_SetupUnused(void);
void AmbientLife_SetupUnusedNext(void);
void AmbientLife_SetupCrossingFlyer(Obj *o);
void AmbientLife_SetupSpinner(Obj *o);
void AmbientLife_Draw(DrawSlot *ds);
extern u8 *AreaWeapon_PartKinds[];
extern f32 *AreaWeapon_PosFortress5[];
extern f32 *AreaWeapon_PosCastle1[];
extern f32 *AreaWeapon_PosBattleTown1[];
extern f32 *AreaWeapon_PosBattleTown3[];
extern f32 *AreaWeapon_PartPos[];
void AreaWeapon_Draw(DrawSlot *ds);
extern u16 AreaWeapon_DayAreas[];
extern s16 *AreaWeapon_Models[];
extern u16 AreaWeapon_YawCastle1[];
extern s16 SkyStreak_RandRange[];
extern s16 SkyStreak_WaitBase[];
extern u16 SkyStreak_WaitMask[];
void SkyStreak_Draw(DrawSlot *ds);
typedef struct SkyStreakPart {
    s16 variant;
    s16 slot;
    f32 pos[3];
    u8 pad10[0xC];
    DrawSlot *ds;
    s16 unk20[2];
    s16 frame;
    s16 wait;
    s16 phase;
    s16 unk2A;
    u8 pad2C[4];
} SkyStreakPart;
extern f32 gStartPos[];
void Mtx_Identity(f32 *m);
void AreaWeapon_Update(Obj *o);
void ChameleosAttack_Update(ObjAttach *o);
void AreaDrop_Draw(DrawSlot *ds);
s16 AmbientLife_FindFreeChild(Obj *o);
typedef struct AmbientLifeChildren {
    Obj *obj[32];
    s16 count[16];
} AmbientLifeChildren;
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void AnimatedGroundTiles_Draw(DrawSlot *s);
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
void MsgEvent_DrawPageCounter(s16 x, s16 y);
void MsgEvent_DrawMessage(u8 flag, s32 arg1);
typedef struct SprRect {
    s16 x, y, w, h;
    s32 color;
    s32 uv0;
    s32 uv1;
} SprRect;
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 tex);
s32 Sprite_Draw(struct Sprite *s);
extern u8 gMsgEventSetIcon[];
void MsgEvent_DrawIcon(s32 arg0, s32 arg1);
extern s16 SkyStreak_Models[];
extern s16 SkyStreak_Frames[];
/* The message window of the quest's advice messages (gQuestMsgWindowBuf, which gQuestMsgWindow
 * points to once MsgEvent_Init ran): the field menu's list of the quest set's messages
 * (MsgEvent_OpenList, MsgEvent_ListInput: a Grid of 7 rows) and the popup of a message an event
 * chose (MsgEvent_Check, MsgEvent_OpenPopup, MsgEvent_PopupInput). */
typedef struct QuestMsgWin {
    u8 listStep;        /* 0x00: the list window: 0 open, 1 closing (Shake), 2 closed */
    u8 listSub;         /* 0x01: 0 the list, 1 a message's pages, 2 left with the back button */
    s16 unk02;          /* 0x02: cleared by MsgEvent_Init */
    u8 page;            /* 0x04: the page of the message shown (MsgEvent_Line) */
    u8 unk05;
    u8 pages;           /* 0x06: its pages (MsgEvent_PageCount) */
    u8 set;             /* 0x07: the message set: 0x12 .. 0x18 for seven quest ids, else gQuest.stage - 1 */
    u8 msg;             /* 0x08: the message shown */
    s8 request;         /* 0x09: the message an event chose (MsgEvent_CheckList, MsgEvent_CheckQuest), -1: none */
    u8 count;           /* 0x0A: the messages of the set (MsgEvent_CountInSet) */
    u8 popupStep;       /* 0x0B: the popup: 0 none, 1 opening, 2 shown, 3 closing (MsgEvent_PopupInput) */
    u8 popupReq;        /* 0x0C: 1: MsgEvent_Check chose a message; MsgEvent_OpenPopup opens the popup */
    u8 unk0D[0x10 - 0x0D];
    Grid grid;          /* 0x10: the list's cursor (7 rows; grid.index is the message) */
} QuestMsgWin;
extern QuestMsgWin gQuestMsgWindowBuf;
u8 MsgEvent_CountInSet(u8 arg0);
void RemobraAttack_Update(ObjAttach *o);
u8 Player_InCurrentArea(void *obj);
ObjAttach *ObjAttach_Alloc(s32 n);
s32 MsgEvent_CheckQuest(s32 arg0, s32 arg1, s8 arg2);
s32 MsgEvent_CheckSupplyBoxClosed(void);
s32 Quest_HasAdvice(void);
extern MsgEvent gMsgEventsQuestStart[];
extern MsgEvent gMsgEventsSupplyBox[];
extern MsgEvent gMsgEventsGoalDelivered[];
extern MsgEvent gMsgEventsApcerosHunted[];
extern MsgEvent gMsgEventsCamp[];
extern MsgEvent gMsgEventsJungle1[];
extern MsgEvent gMsgEventsJungle4[];
extern HitShape *ChameleosAttack_HitShapes[];
extern void *ChameleosAttack_RowsPtr;
f32 Math_Absf(f32 x);
extern HitShape *RemobraAttack_HitShapes[];
extern void *RemobraAttack_RowsPtr;
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
u8 MsgEvent_PageCount(u8, u8);
void LightShaft_Setup(Obj *o);
void LightShaft_Run(Obj *);
void LightShaft_End(Obj *o);
void LightShaft_Free(Obj *o);
void SkyStreak_Setup(Obj *o);
void SkyStreak_Run(Obj *o);
void SkyStreak_End(Obj *o);
void SkyStreak_Free(Obj *o);
void AreaDrop_Setup(Obj *o);
void AreaDrop_Run(Obj *o);
void AreaDrop_End(Obj *o);
void AreaDrop_Free(Obj *o);
void AreaFlag_Setup(Obj *o);
void AreaFlag_Run(Obj *o);
void AreaFlag_End(Obj *o);
void AreaFlag_Free(Obj *o);
void AreaWeapon_Setup(Obj *o);
void AreaWeapon_Run(Obj *o);
void AreaWeapon_End(Obj *o);
void AreaWeapon_Free(Obj *o);
void AnimatedProps_Setup(void *arg);
void AnimatedProps_Draw(DrawSlot *ds);
extern f32 AnimatedProps_PosForestHills11[];
extern f32 AnimatedProps_PosSnowyMountains6[];
extern f32 AnimatedProps_PosSnowyNight5[];
extern f32 AnimatedProps_PosSnowyNight6[];
extern f32 AnimatedProps_PosSnowyNight8[];
extern f32 AnimatedProps_PosBattleTown0[];
void AnimatedProps_Run(Obj *o);
void AnimatedProps_End(Obj *o);
void AnimatedProps_Free(Obj *o);
void AnimatedGroundTiles_Setup(Obj *o);
void AnimatedGroundTiles_Run(Obj *o);
void AnimatedGroundTiles_End(Obj *o);
void AnimatedGroundTiles_Free(Obj *o);
void AmbientLife_Setup(Obj *o);
void AmbientLife_Run(Obj *o);
void AmbientLife_End(Obj *o);
void AmbientLife_Free(Obj *o);
void AmbientLife_RunSpawner(Obj *o);
void AmbientLife_RunFallingLeaf(Obj *o);
void AmbientLife_DrawFallingLeaf(DrawSlot *ds);
void AmbientLife_RunWanderer(Obj *o);
void AmbientLife_DrawWanderer(DrawSlot *ds);
void AmbientLife_RunDarter(Obj *o);
void AmbientLife_DrawDarter(DrawSlot *ds);
void AmbientLife_RunSteadyFlyer(Obj *o);
void AmbientLife_DrawSteadyFlyer(DrawSlot *ds);
void AmbientLife_RunButterfly(Obj *o);
void AmbientLife_DrawButterfly(DrawSlot *ds);
void AmbientLife_RunSpinner(Obj *);
void AmbientLife_DrawSpinner(DrawSlot *ds);
u32 AmbientLife_RunCirclingFlyer(Obj *o);
void AmbientLife_DrawCirclingFlyer(DrawSlot *ds);
void AmbientLife_RunGlowBug(Obj *o);
void AmbientLife_DrawGlowBug(DrawSlot *ds);
void AmbientLife_RunUnused(void);
void AmbientLife_DrawUnused(void);
void AmbientLife_RunUnusedNext(void);
void AmbientLife_DrawUnusedNext(void);
void AmbientLife_RunCrossingFlyer(Obj *o);
void AmbientLife_DrawCrossingFlyer(DrawSlot *s);
s32 Gs_SetState(u8 reg, u32 v);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
void Game_Pause(u8 arg0);
void Game_Resume(u8 arg0);
void DrawSlot_Free(s32 i);
u16 Area_DayId(u16 id);
void * QuestInfo_Get(s32);
extern u16 gMsgEventSets[][2];
extern QuestMsgWin *gQuestMsgWindow;
Obj *Obj_Alloc(s32 n);
void LightShaft_Update(Obj *o);
void AreaDrop_Update(Obj *o);
void AnimatedProps_Update(Obj *o);
void AnimatedGroundTiles_Update(Obj *o);
void AmbientLife_Update(Obj *o);
void SkyStreak_Update(Obj *o);
extern Panel gMsgEventPopupWin;
void AreaFlag_Update(Obj *o);
void Vec_Copy(f32 *dst, f32 *src);
extern s16 SkyStreak_Counts[];
extern u8 *gMsgEventSetEntries[];
extern char *gMsgEventTitles[];
extern char **gMsgEventLines[];
s32 MsgEvent_Check(u16);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetScale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
/* end of prototypes */

/*
 * Makes a ChameleosAttack of type attackType for em when a player is in em's
 * area: it takes em's index, motion, facing and side, and clears em's
 * hitPending and contactHit. Types 4 and 7 face backward (yaw + 0x8000).
 */
void ChameleosAttack_Create(Em *em, s32 attackType)
{
    ObjAttach *attack;

    if ((u8) Player_InCurrentArea(em) != 0 && (attack = ObjAttach_Alloc(0)) != NULL) {
        attack->kind = ATTACH_CHAMELEOS_ATTACK;
        attack->type = attackType;
        attack->update = ChameleosAttack_Update;
        attack->idx = em->index;
        attack->isEm = em->isEm;
        attack->ownerMotion = em->motionId[0];
        attack->owner = em;
        attack->yaw = em->rotY;
        switch (attackType) {
        case 4:
        case 7:
            *(u16 *) &attack->yaw += 0x8000;
            break;
        }
        attack->rotX = em->rotX;
        attack->rotY = em->rotY;
        attack->rotZ = em->rotZ;
        attack->side = *(u8 *) &em->side;
        em->hitPending = 0;
        em->contactHit = 0;
    }
}

/*
 * The attack's update, run every frame: switches on ObjAttach.state;
 * ATTACH_HIT keeps it running, states 4, ATTACH_DEFLECTED, 7 and
 * ATTACH_DEFLECTED_AURA end it.
 */
void ChameleosAttack_Update(ObjAttach *attack)
{
    switch (attack->state) {
    case ATTACH_SETUP:
        ChameleosAttack_Setup(attack);
        break;
    case ATTACH_RUN:
        ChameleosAttack_Run(attack);
        break;
    case ATTACH_END:
        ChameleosAttack_End(attack);
        break;
    case ATTACH_HIT:
        ChameleosAttack_Run(attack);
        break;
    case 4:
        ChameleosAttack_End(attack);
        break;
    case ATTACH_DEFLECTED:
        ChameleosAttack_End(attack);
        break;
    case ATTACH_DEFLECTED_AURA:
        ChameleosAttack_End(attack);
        break;
    case 7:
        ChameleosAttack_End(attack);
        break;
    case ATTACH_FREE:
        ChameleosAttack_Free(attack);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from row type of its attack table with
 * ATKF_OWNER_CONTACT: its hit shapes (ChameleosAttack_HitShapes) and em's
 * area. A nonzero hitDelay is counted from the monster's motion (its blend
 * count less its frame, on layer 1 for type 3, else layer 0) and kept at 0 or
 * more; it gets no draw slot.
 */
void ChameleosAttack_Setup(ObjAttach *attack)
{
    Em *em = &gEms[attack->idx];
    s32 blend;
    s32 frame;

    attack->state++;
    attack->visible = 1;
    attack->draw = NULL;
    Player_TestSetFlags(attack, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(attack, (s32) em, attack->type, &ChameleosAttack_RowsPtr);
    attack->hitList = ChameleosAttack_HitShapes[attack->hitListNo];
    attack->hurtList = 0;
    attack->area = em->area;
    if (attack->hitDelay != 0) {
        if (attack->type == 3) {
            blend = Math_Absf((s16) em->motionBlend[1] % 100);
            frame = Math_Absf(em->motionFrame[1]);
        } else {
            blend = Math_Absf((s16) em->motionBlend[0] % 100);
            frame = Math_Absf(em->motionFrame[0]);
        }
        attack->hitDelay += blend - frame - 1;
        if ((s8) attack->hitDelay < 0) {
            attack->hitDelay = 0;
        }
    }
    attack->drawSlotBuf = NULL;
}

/*
 * Ends the attack once the monster's motion is no longer the one that made it
 * or its hit window closed; type 6 holds its window open (hitFrames 99) and
 * type 9 also lives through motions 0x413 and 0x41D. A draw slot would follow
 * it, but Setup gives it none.
 */
void ChameleosAttack_Run(ObjAttach *attack)
{
    Em *em = &gEms[attack->idx];

    switch (attack->type) {
    case 6:
        attack->hitFrames = 0x63;
        break;
    }
    switch (attack->type) {
    case 9:
        if ((em->motionId[0] != 0x413 && em->motionId[0] != 0x41D && em->motionId[0] != attack->ownerMotion) || attack->hitPhase == 0) {
            attack->hitPhase = 0;
            attack->state = ATTACH_END;
        }
        break;
    default:
        if (em->motionId[0] != attack->ownerMotion || attack->hitPhase == 0) {
            attack->hitPhase = 0;
            attack->state = ATTACH_END;
        }
        break;
    }
    if (attack->drawSlotBuf != NULL) {
        Vec_Copy(attack->drawSlotBuf->pos, attack->pos);
        OT_AddByDepth(gDrawOt32, attack->drawSlotBuf, 0x20, 0);
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot if
 * it has one; it is freed on the next update.
 */
void ChameleosAttack_End(ObjAttach *attack)
{
    attack->hitPhase = 0;
    attack->state = ATTACH_FREE;
    attack->visible = 0;
    if (attack->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) attack->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void ChameleosAttack_Free(ObjAttach *attack)
{
    ObjAttach_Free(attack);
}

/*
 * Makes a RemobraAttack of type attackType for em when a player is in em's
 * area (type 1 from EmRemobra_Strike, 2 and 3 from EmRemobra_SwoopStrike):
 * it takes em's index, motion, facing and side, and clears em's hitPending
 * and contactHit.
 */
void RemobraAttack_Create(Em *em, u8 attackType)
{
    ObjAttach *attack;

    if ((u8) Player_InCurrentArea(em) != 0 && (attack = ObjAttach_Alloc(0)) != NULL) {
        attack->kind = ATTACH_HERMITAUR_ATTACK;
        attack->type = attackType;
        attack->update = RemobraAttack_Update;
        attack->idx = em->index;
        attack->isEm = em->isEm;
        attack->ownerMotion = em->motionId[0];
        attack->owner = em;
        attack->yaw = em->rotY;
        attack->rotX = em->rotX;
        attack->rotY = em->rotY;
        attack->rotZ = em->rotZ;
        attack->side = *(u8 *) &em->side;
        em->hitPending = 0;
        em->contactHit = 0;
    }
}

/*
 * The attack's update, run every frame: switches on ObjAttach.state;
 * ATTACH_HIT keeps it running, states 4, ATTACH_DEFLECTED, 7 and
 * ATTACH_DEFLECTED_AURA end it.
 */
void RemobraAttack_Update(ObjAttach *attack)
{
    switch (attack->state) {
    case ATTACH_SETUP:
        RemobraAttack_Setup(attack);
        break;
    case ATTACH_RUN:
        RemobraAttack_Run(attack);
        break;
    case ATTACH_END:
        RemobraAttack_End(attack);
        break;
    case ATTACH_HIT:
        RemobraAttack_Run(attack);
        break;
    case 4:
        RemobraAttack_End(attack);
        break;
    case ATTACH_DEFLECTED:
        RemobraAttack_End(attack);
        break;
    case ATTACH_DEFLECTED_AURA:
        RemobraAttack_End(attack);
        break;
    case 7:
        RemobraAttack_End(attack);
        break;
    case ATTACH_FREE:
        RemobraAttack_Free(attack);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from row type of its attack table with
 * ATKF_OWNER_CONTACT, at the monster's position with hitScale 1.0: its hit
 * shapes (RemobraAttack_HitShapes) and em's area. A nonzero hitDelay is
 * counted from the monster's motion on layer 0 and kept at 0 or more; it gets
 * no draw slot.
 */
void RemobraAttack_Setup(ObjAttach *attack)
{
    Em *em = &gEms[attack->idx];

    attack->state++;
    attack->visible = 1;
    attack->draw = NULL;
    Player_TestSetFlags(attack, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(attack, (s32) em, attack->type, &RemobraAttack_RowsPtr);
    attack->hitList = RemobraAttack_HitShapes[attack->hitListNo];
    attack->hurtList = 0;
    attack->area = em->area;
    Vec_Copy(attack->pos, (f32 *)&em->posX);
    attack->hitScale = 1.0f;
    if (attack->hitDelay != 0) {
        attack->hitDelay += (s32) Math_Absf((s16) em->motionBlend[0] % 100) - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) attack->hitDelay < 0) {
            attack->hitDelay = 0;
        }
    }
    attack->drawSlotBuf = NULL;
}

/*
 * Ends the attack once the monster's motion is no longer the one that made it
 * or its hit window closed. A draw slot would follow it, but Setup gives it
 * none.
 */
void RemobraAttack_Run(ObjAttach *attack)
{
    Em *em = &gEms[attack->idx];

    if (em->motionId[0] != attack->ownerMotion || attack->hitPhase == 0) {
        attack->hitPhase = 0;
        attack->state = ATTACH_END;
    }
    if (attack->drawSlotBuf != NULL) {
        Vec_Copy(attack->drawSlotBuf->pos, attack->pos);
        OT_AddByDepth(gDrawOt32, attack->drawSlotBuf, 0x20U, 0);
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot if
 * it has one; it is freed on the next update.
 */
void RemobraAttack_End(ObjAttach *attack)
{
    attack->hitPhase = 0;
    attack->state = ATTACH_FREE;
    attack->visible = 0;
    if (attack->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) attack->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void RemobraAttack_Free(ObjAttach *attack)
{
    ObjAttach_Free(attack);
}

/*
 * Fires message event 14, the camp advice (MsgEvent_Check), when the local
 * player is in the base camp (gScene.campArea) and a monster of kind 0x0C was
 * hunted this quest. Quest_Update calls it every frame of a quest with
 * advice.
 */
void Quest_CheckCampAdvice(void)
{
    Player *player = &gPlayers[gScene.localPlayer];

    if (player->area == gScene.campArea && gQuest.hunted[12] > 0) {
        MsgEvent_Check(MSGEV_CAMP);
    }
}

/*
 * Sets the message window up for the quest about to start (Quest_Init calls
 * it): points gQuestMsgWindow at gQuestMsgWindowBuf, clears the list's and
 * the popup's steps, and picks the quest's message set (sets 0x12 to 0x18 for
 * seven quest ids, else the quest id less 1) and the set's message count.
 * listStep and listSub are cleared by one 16-bit store, as the game's code
 * does.
 */
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
    case QUEST_ID_DAIMYO_HERMITAUR_SCENE_2:
        gQuestMsgWindow->set = 0x13;
        break;
    case QUEST_ID_KUSHALA_JUNGLE_SCENE:
        gQuestMsgWindow->set = 0x14;
        break;
    case QUEST_ID_KUSHALA_SNOW_SCENE:
        gQuestMsgWindow->set = 0x15;
        break;
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE:
        gQuestMsgWindow->set = 0x16;
        break;
    case QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE:
        gQuestMsgWindow->set = 0x17;
        break;
    case 0x1772:
        gQuestMsgWindow->set = 0x18;
        break;
    default:
        gQuestMsgWindow->set = gQuest.stage - 1;
        break;
    }
    gQuestMsgWindow->count = MsgEvent_CountInSet(gQuestMsgWindow->set);
}

/*
 * Opens the popup of the message a check chose (QuestMsgWin.popupReq): page
 * 0, the message's page count, the game paused, the window sliding in and the
 * opening sound. ScEvent_Update calls it every frame while no scene event
 * runs.
 */
void MsgEvent_OpenPopup(void)
{
    if (gQuestMsgWindow->popupReq != 0) {
        gQuestMsgWindow->popupReq = 0U;
        gQuestMsgWindow->popupStep = (u8) (gQuestMsgWindow->popupStep + 1);
        gQuestMsgWindow->page = 0;
        gQuestMsgWindow->pages = MsgEvent_PageCount(gQuestMsgWindow->set, gQuestMsgWindow->msg);
        Game_Pause(0);
        Shake_Start(7, SHAKE_AXIS_Y, 330.0f);
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, 0x10, 0);
    }
}

/*
 * Steps the popup with the buttons pad and returns QuestMsgWin.popupStep (0:
 * no popup). Step 1 waits for the window to slide in; step 2 turns pages with
 * PAD_LEFT and PAD_RIGHT and starts closing on PAD_CROSS; step 3 waits for
 * the window to slide out and resumes the game. Step 1 runs step 2's input on
 * the frame its slide ends: the case label stays inside the if, as in the
 * game's code.
 */
u8 MsgEvent_PopupInput(s32 pad)
{
    s32 buttons;
    u8 pageBefore;
    u8 step;
    u8 pageNow;

    step = gQuestMsgWindow->popupStep;
    switch (step) {
    case 0:
        break;
    case 1:
        if (Shake_Update(7U) != 1) {
            gQuestMsgWindow->popupStep = (u8) (gQuestMsgWindow->popupStep + 1);
        case 2:
            buttons = pad & 0xFFFF;
            if (buttons & PAD_CROSS) {
                gQuestMsgWindow->popupStep = (u8) (gQuestMsgWindow->popupStep + 1);
                Shake_Start(7, SHAKE_SLIDE | SHAKE_AXIS_Y, 330.0f);
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            } else if (buttons & PAD_LEFT) {
                pageBefore = gQuestMsgWindow->page;
                if (pageBefore != 0) {
                    gQuestMsgWindow->page = (u8) (pageBefore - 1);
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
                }
            } else if (buttons & PAD_RIGHT) {
                pageNow = gQuestMsgWindow->page;
                if ((s32) pageNow < (gQuestMsgWindow->pages - 1)) {
                    gQuestMsgWindow->page = (u8) (pageNow + 1);
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
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
/*
 * Opens the message list, the offline field menu's advice entry: returns 1
 * and does nothing once the quest is over (Quest_PhaseIsOver); else pauses
 * the game, sets the list's Grid up (7 rows, as many columns as the set's
 * messages need), hides the help line, starts the window's slide and returns
 * 0.
 */
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
        Shake_Start(7, SHAKE_AXIS_Y, 330.0f);
        return 0;
    }
    return 1;
}

/*
 * The list's input, which the field menu runs every frame with the buttons
 * pad. Returns PAD_START once the window has slid out after PAD_START,
 * PAD_CROSS after the player left the list with PAD_CROSS, else 0; while the
 * window slides in only PAD_START, the D-pad and PAD_CROSS count. In the list
 * PAD_CIRCLE opens a message already seen (MsgEvent_IsSeen) or plays the
 * refusal sound; in a message PAD_LEFT and PAD_RIGHT turn pages and PAD_CROSS
 * goes back to the list.
 */
s32 MsgEvent_ListInput(u16 pad)
{
    u16 buttons;
    u8 *listSubPtr;

    switch (gQuestMsgWindow->listStep) {
    case 0:
        if (Shake_Update(7) == 1) {
            pad &= PAD_START | PAD_DPAD | PAD_CROSS;
        }
        buttons = pad;
        if (buttons & PAD_START) {
            Shake_Start(7, SHAKE_SLIDE | SHAKE_AXIS_Y, 330.0f);
            gQuestMsgWindow->listStep++;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            break;
        }
        listSubPtr = &gQuestMsgWindow->listSub;
        switch (*listSubPtr) {
        case 0:
            if (buttons & PAD_CROSS) {
                Game_Resume(0);
                gQuestMsgWindow->listSub = 2;
            } else if (gQuestMsgWindow->grid.cols != 0) {
                Menu_GridCursor(&gQuestMsgWindow->grid, pad);
                if (buttons & PAD_CIRCLE) {
                    if (MsgEvent_IsSeen(gQuestMsgWindow->set, *(u8 *)&gQuestMsgWindow->grid.index) == 1) {
                        gQuestMsgWindow->page = 0;
                        gQuestMsgWindow->msg = gQuestMsgWindow->grid.index;
                        gQuestMsgWindow->pages = MsgEvent_PageCount(gQuestMsgWindow->set, gQuestMsgWindow->msg);
                        gQuestMsgWindow->listSub++;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                    }
                }
            }
            break;
        case 1:
            if (buttons & PAD_CROSS) {
                *listSubPtr = 0;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            } else if (buttons & PAD_LEFT) {
                if (gQuestMsgWindow->page != 0) {
                    gQuestMsgWindow->page--;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
                }
            } else if (buttons & PAD_RIGHT) {
                if (gQuestMsgWindow->page < gQuestMsgWindow->pages - 1) {
                    gQuestMsgWindow->page++;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
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

/*
 * Draws the popup at its slide offset: its window, the message with the
 * popup's button hint (MsgEvent_DrawMessage) and the icon. Hud_DrawLayer2
 * calls it while a popup is open.
 */
void MsgEvent_DrawPopup(void)
{
    s16 *slideOfs = Shake_GetOffset(7);

    Window_DrawText(&gMsgEventPopupWin, slideOfs, 0);
    MsgEvent_DrawMessage(0, (s32) slideOfs);
    MsgEvent_DrawIcon(0, (s32) slideOfs);
}

/* The texts of the advice messages, reached only through gMsgEventTitles and the gMsgEventLines
 * tables; they stay here, in the game's order, between RemobraAttack_Update's jump table and
 * MsgEvent_DrawList's constants. */
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE300);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE320);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE340);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE360);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE390);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE3C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE3E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE3F8);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE410);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE430);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE450);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE470);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE490);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE4B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE4C8);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE4E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE500);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE520);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE550);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE570);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE590);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE5B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE5E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE600);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE620);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE640);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE660);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE680);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE6A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE6B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE6D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE6E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE700);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE718);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE730);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE750);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE760);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE780);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE790);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE7A8);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE7C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE7D8);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE7F0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE810);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE830);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE850);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE870);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE890);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE8B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE8D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE8E8);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE8F8);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE910);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE930);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE940);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE960);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE970);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE990);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE9B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE9D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE9E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE9E8);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DE9F8);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEA08);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEA18);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEA30);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEA50);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEA70);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEA90);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEAB0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEAD0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEAE0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEB00);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEB20);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEB40);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEB58);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEB70);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEC20);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DECD0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DED80);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEE60);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEF30);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DEFE0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF0A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF170);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF1E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF2A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF360);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF400);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF4B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF580);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF660);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF730);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF7D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF880);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DF920);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DFA10);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DFAB0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DFB60);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DFC10);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DFD10);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DFDD0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DFE60);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008DFF30);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0040);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0100);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0190);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0290);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0320);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0400);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E04C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0580);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0620);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0710);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E07A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0850);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0930);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0A30);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0AE0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0B40);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0C20);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0CB0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0D60);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0E20);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0ED0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E0F90);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1070);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1130);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E11E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1260);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1310);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E13C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1480);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1530);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1610);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1690);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1750);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1810);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E18B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E19B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1A40);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1AE0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1B80);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1C00);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1C90);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1D30);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1DC0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1E80);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E1F60);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2040);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E20E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E21B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2240);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2300);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2360);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E23F0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2480);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2530);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2610);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E26D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E27B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2890);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2920);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E29D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2AA0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2B60);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2C50);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2CF0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2DA0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2E70);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2F70);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E2FF0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3090);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3120);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3200);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E32C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3380);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3420);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E34C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3550);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E35D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3670);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3770);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E37F0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E38B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3980);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3A30);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3AD0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3B70);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3C10);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3CD0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3D60);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3DE0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3EC0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E3F90);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4070);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4120);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E41E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E42B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4380);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4450);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E44E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E45A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4670);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4750);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4800);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4890);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4960);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4A20);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4AA0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4B80);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4C60);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4D50);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4E10);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4EA0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4F50);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E4FD0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5090);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5130);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E51E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E52C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E53A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5470);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5560);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E55E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5690);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5760);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5830);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5920);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E59E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5AA0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5B60);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5BE0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5CB0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5D90);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5E50);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E5F40);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6010);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E60F0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E61E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6270);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6340);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6410);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E64F0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E65C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6680);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6750);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E67F0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E68C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E69A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6A80);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6B50);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6C00);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6C90);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6D40);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6DD0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6E70);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6F40);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E6FE0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7060);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7130);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E71A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7230);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E72C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7390);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7450);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E74F0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E75C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E76A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7720);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E77E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E78C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E79A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7A50);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7B00);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7BA0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7C70);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7D10);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7DE0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7E90);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7F20);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E7FD0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8060);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8100);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8180);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8240);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8330);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8400);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E84D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8590);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8660);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8750);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E87E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E88D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8960);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8A20);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8B10);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8BE0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8C90);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8D20);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8DB0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8E70);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E8F20);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9000);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E90F0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E91C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E92A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9370);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9420);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E94D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E95A0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9680);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9700);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E97E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E98C0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9980);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9A00);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9AA0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9B30);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9C00);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9CC0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9DA0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9E90);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008E9F70);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA050);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA120);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA190);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA230);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA300);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA3E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA4B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA550);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA610);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA680);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA760);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA800);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA8F0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EA9B0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EAA70);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EAB30);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EABF0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EACB0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EAD50);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EAE10);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EAEA0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EAF30);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EAFB0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB030);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB0D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB190);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB260);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB300);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB3E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB440);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB500);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB570);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB5D0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB660);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB6F0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB770);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB800);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB850);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB8F0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB970);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EB9E0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EBA20);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EBA90);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EBB10);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EBBA0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EBC70);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EBD30);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EBDD0);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EBE90);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EBF50);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", D_008EC020);
INCLUDE_RODATA("asm/game/nonmatchings/area_objects", gMsgEventListItems);

/* The texts of the advice window's frame, by their index in gMsgEventTexts. */
enum MsgEventText {
    MSG_EVENT_TEXT_POPUP_HINT = 0,  /* the popup's button hint */
    MSG_EVENT_TEXT_UNSEEN = 1,      /* the list's placeholder for a message not seen yet */
    MSG_EVENT_TEXT_LIST_HELP = 2,   /* the help line under the list */
    MSG_EVENT_TEXT_LIST_HINT = 3,   /* the button hint of a message opened from the list */
    MSG_EVENT_TEXT_COLUMN_FMT = 4,  /* the list's column counter: column, columns */
    MSG_EVENT_TEXT_PAGE_FMT = 5     /* a message's page counter: page, pages */
};
extern char *gMsgEventTexts[];
/*
 * Draws the message list window at its slide offset. In the list: the quest's
 * title (MsgEvent_CopyQuestTitle) centred, the column counter when there is
 * more than one column, the help line and the current column's seven titles,
 * a grey placeholder for a message not seen yet; in a message: the message as
 * the popup draws it, with the list's button hint. Nothing once the window
 * has closed.
 */
void MsgEvent_DrawList(void)
{
    s16 *slideOfs;
    s8 questTitle[0x20];
    s32 i;
    s16 rowY;
    u8 k;

    slideOfs = Shake_GetOffset(7);
    switch (gQuestMsgWindow->listStep) {
    case 0:
    case 1:
        switch (gQuestMsgWindow->listSub) {
        case 0:
        case 2:
            ListPanel_Draw(&gMsgEventListPanel, slideOfs, gMsgEventListItems, gQuestMsgWindow->grid.cols != 0 ? gQuestMsgWindow->grid.row : -1);
            MsgEvent_CopyQuestTitle(questTitle);
            i = Str_Length(questTitle);
            Font_SetColor(0x807F7F28);
            Font_SetPos(0x140 - i * 9 / 2, slideOfs[1] + 0x78);
            Text_Draw(questTitle);
            Font_SetColor(0x807F7F7F);
            if (gQuestMsgWindow->grid.cols > 1) {
                Font_SetPos(0x1CC, slideOfs[1] + 0x78);
                Text_Printf(gMsgEventTexts[MSG_EVENT_TEXT_COLUMN_FMT], gQuestMsgWindow->grid.col + 1, gQuestMsgWindow->grid.cols);
                Menu_DrawFrame(&gMsgEventListPanel, slideOfs);
            }
            Font_SetPos(0x17E, slideOfs[1] + 0x128);
            Text_Printf(gMsgEventTexts[MSG_EVENT_TEXT_LIST_HELP]);
            rowY = slideOfs[1] + 0x8E;
            k = gQuestMsgWindow->grid.col * 7;
            for (i = 0; i < 7; i++) {
                Font_SetPos(0x7A, rowY);
                if (MsgEvent_IsSeen(gQuestMsgWindow->set, k) == 1) {
                    Font_SetColor(0x807F7F7F);
                    Text_PrintfColored(MsgEvent_Title(gQuestMsgWindow->set, k));
                } else {
                    Font_SetColor(0x80505050);
                    Text_Draw((s8 *)gMsgEventTexts[MSG_EVENT_TEXT_UNSEEN]);
                }
                k++;
                if (gQuestMsgWindow->count <= k) {
                    break;
                }
                rowY += 0x16;
            }
            break;
        case 1:
            Window_DrawText(&gMsgEventPopupWin, slideOfs, 0);
            MsgEvent_DrawMessage(1, (s32) slideOfs);
            MsgEvent_DrawIcon(0, (s32) slideOfs);
            break;
        }
        break;
    case 2:
        break;
    }
}

/*
 * Draws the message shown (QuestMsgWin.msg) in a window at offset windowOfs
 * (an s16 x, y pair passed as an s32): its title centred, the current page's
 * text, the page counter (MsgEvent_DrawPageCounter) and the button hint, the
 * popup's when inList is 0, the list's when it is 1. The first strlen call's
 * result is unused, as in the game's code.
 */
void MsgEvent_DrawMessage(u8 inList, s32 windowOfs)
{
    char *title;

    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x802B7F2B);
    title = MsgEvent_Title(gQuestMsgWindow->set, gQuestMsgWindow->msg);
    strlen(title);
    Font_SetPos(0x140 - strlen(title) * 9 / 2, *(s16 *) (windowOfs + 2) + 0x81);
    Text_PrintfColored(title);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(0x7A, *(s16 *) (windowOfs + 2) + 0xA0);
    Text_PrintfStyled(3, MsgEvent_Line(gQuestMsgWindow->set, gQuestMsgWindow->msg, gQuestMsgWindow->page));
    MsgEvent_DrawPageCounter(0x12A, *(s16 *) (windowOfs + 2) + 0x118);
    Font_SetPos(0x1C7, *(s16 *) (windowOfs + 2) + 0x118);
    if (inList == 0) {
        Text_Printf(gMsgEventTexts[MSG_EVENT_TEXT_POPUP_HINT]);
    } else {
        Text_Printf(gMsgEventTexts[MSG_EVENT_TEXT_LIST_HINT]);
    }
}

/*
 * Draws the page counter at (x, y), the page shown and the page count, and
 * for a message of more than one page the arrows to the previous and the next
 * page (each a dark and a bright sprite), 3 or 8 pixels out by bit 4 of
 * gFrameTick so that they pulse.
 */
void MsgEvent_DrawPageCounter(s16 x, s16 y)
{
    SprRect arrow;
    s16 pulse;
    int flash;
    s16 *arrowY;
    s32 *arrowColor;
    s16 *uvStart;
    s16 *uvEnd;

    Font_SetColor(0x807F7F7F);
    Font_SetPos(x, y);
    Text_PrintfColored(gMsgEventTexts[MSG_EVENT_TEXT_PAGE_FMT], gQuestMsgWindow->page + 1, gQuestMsgWindow->pages);
    if (gQuestMsgWindow->pages > 1) {
        flash = (gFrameTick & 0x10) == 0 ? (s16) 3 : (s16) 8;
        arrow.w = 0x12;
        arrow.h = 0x12;
        uvStart = (s16 *) &arrow.uv0;
        *(s32 *) uvStart = 0x6E0071;
        uvEnd = (s16 *) &arrow.uv1;
        *(s32 *) uvEnd = 0x7D007D;
        pulse = flash;
        if (gQuestMsgWindow->page != 0) {
            arrow.x = (s16) (x - 0x12) - pulse + 1;
            arrowY = &arrow.y;
            arrowColor = &arrow.color;
            *arrowY = y;
            *arrowColor = 0xFF203020;
            Sprite_Draw((struct Sprite *)&arrow);
            arrow.x -= 2;
            *arrowY -= 1;
            *arrowColor = 0xFF18FF20;
            Sprite_Draw((struct Sprite *)&arrow);
        }
        if (gQuestMsgWindow->page != gQuestMsgWindow->pages - 1) {
            *uvStart = 0x7C;
            *uvEnd = 0x70;
            arrowY = &arrow.y;
            arrowColor = &arrow.color;
            arrow.x = (s16) (x - 0x12) + pulse + 0x42;
            *arrowY = y;
            *arrowColor = 0xFF203020;
            Sprite_Draw((struct Sprite *)&arrow);
            arrow.x -= 2;
            *arrowY -= 1;
            *arrowColor = 0xFF18FF20;
            Sprite_Draw((struct Sprite *)&arrow);
        }
    }
}

/*
 * Draws the message windows' icon (texture TEX_SLOT_PICTURES), which moves with the
 * window's slide (windowOfs, an s16 x, y pair passed as an s32);
 * gMsgEventSetIcon picks one of two pictures by the message set. The first
 * parameter is unused.
 */
void MsgEvent_DrawIcon(s32 unused, s32 windowOfs)
{
    SprRect sprite;

    Tex_Prepare(1, TEX_SLOT_PICTURES);
    Tex_Bind(TEX_SLOT_PICTURES);
    sprite.x = 0x120;
    sprite.y = 0x38 - (s16) (s32) (0.37272727f * *(s16 *) (windowOfs + 2));
    sprite.w = 0x40;
    sprite.h = 0x40;
    sprite.color = -1;
    if (gMsgEventSetIcon[(s8) gQuestMsgWindow->set] == 0) {
        sprite.uv0 = 0xD3007F;
        sprite.uv1 = 0xFB00A7;
    } else {
        sprite.uv0 = 0xD30055;
        sprite.uv1 = 0xFB007D;
    }
    Sprite_Draw((struct Sprite *)&sprite);
}

/*
 * Copies into buf the first line of the quest's text 0 (QuestInfo_Get; likely
 * the quest's name), at most 30 characters, and ends it with a 0.
 */
void MsgEvent_CopyQuestTitle(s8 *buf)
{
    s32 left;
    s8 *info;
    s8 *dst;
    s8 letter;

    dst = buf;
    info = QuestInfo_Get(0);
    left = 0x1E;
loop_1:
    letter = *info;
    if (letter == 0xA) {
        *dst = 0;
        return;
    }
    *dst = letter;
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

/* The field menu's draw entry for the message list: MsgEvent_DrawList. */
void MsgEvent_Draw(void)
{
    MsgEvent_DrawList();
}

s32 MsgEvent_CheckList(MsgEvent *e);
/*
 * Runs message event kind (0 to 18; what fires each is given with the kinds)
 * and returns 1 when it chose a message: the event sets QuestMsgWin.request,
 * and the message goes to the popup that MsgEvent_OpenPopup opens. Nothing
 * fires online, in a quest without advice (Quest_HasAdvice) or once the quest
 * is over; each event fires once per save, as an event flag records it.
 */
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
    case MSGEV_QUEST_START:
        MsgEvent_CheckList(gMsgEventsQuestStart);
        break;
    case MSGEV_SUPPLY_BOX:
        MsgEvent_CheckList(gMsgEventsSupplyBox);
        break;
    case MSGEV_SUPPLY_BOX_CLOSED:
        MsgEvent_CheckSupplyBoxClosed();
        break;
    case MSGEV_DELIVERY_BOX:
        MsgEvent_CheckQuest(0xCB, 1, 3);
        break;
    case MSGEV_BED:
        MsgEvent_CheckQuest(0xCC, 1, 4);
        break;
    case MSGEV_JUNGLE_1:
        MsgEvent_CheckList(gMsgEventsJungle1);
        break;
    case MSGEV_JUNGLE_2:
        MsgEvent_CheckQuest(0xDC, 3, 4);
        break;
    case MSGEV_JUNGLE_3:
        MsgEvent_CheckQuest(0xDB, 3, 3);
        break;
    case MSGEV_JUNGLE_4:
        MsgEvent_CheckList(gMsgEventsJungle4);
        break;
    case MSGEV_JUNGLE_8:
        MsgEvent_CheckQuest(0x107, 0x11, 1);
        break;
    case MSGEV_JUNGLE_6:
        MsgEvent_CheckQuest(0xE2, 5, 1);
        break;
    case MSGEV_FORTRESS_0:
        MsgEvent_CheckQuest(0x11B, 0x1772, 0);
        break;
    case MSGEV_APCEROS_HUNTED:
        MsgEvent_CheckList(gMsgEventsApcerosHunted);
        break;
    case MSGEV_MINING:
        MsgEvent_CheckQuest(0xCF, 1, 7);
        break;
    case MSGEV_CAMP:
        MsgEvent_CheckList(gMsgEventsCamp);
        break;
    case MSGEV_GOAL_DELIVERED:
        MsgEvent_CheckList(gMsgEventsGoalDelivered);
        break;
    case MSGEV_CARRY_ITEM:
        MsgEvent_CheckQuest(0x108, 0x11, 2);
        break;
    case MSGEV_GOAL_KILLED:
        MsgEvent_CheckQuest(0x113, 0x12, 9);
        break;
    case MSGEV_GOAL_CAPTURED:
        MsgEvent_CheckQuest(0x114, 0x12, 0xA);
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

/*
 * Looks the current quest (gQuest.stage) up in the dated event list e (sorted
 * by quest id, ended by a negative flag): when its entry has not fired yet,
 * sets the entry's event flag, requests its message, sets the seen flags (200
 * + n) of the up to three messages it unlocks and returns 1; else returns 0.
 */
s32 MsgEvent_CheckList(MsgEvent *entry)
{
    s32 curQuest;
    s16 flag = entry->flag;

    if (flag >= 0) {
        curQuest = gQuest.stage;
        do {
            if (entry->questId >= curQuest) {
                if (entry->questId == curQuest && Save_TestEventFlag(flag) != 1) {
                    Save_SetEventFlag(entry->flag);
                    gQuestMsgWindow->request = entry->msg;
                    if (entry->ev[0] > 0) {
                        Save_SetEventFlag(entry->ev[0] + MSGEV_SEEN_FLAG_BASE);
                    }
                    if (entry->ev[1] > 0) {
                        Save_SetEventFlag(entry->ev[1] + MSGEV_SEEN_FLAG_BASE);
                    }
                    if (entry->ev[2] > 0) {
                        Save_SetEventFlag(entry->ev[2] + MSGEV_SEEN_FLAG_BASE);
                    }
                    return 1;
                }
                break;
            }
            entry++;
            flag = entry->flag;
        } while (flag >= 0);
    }
    return 0;
}

/*
 * In quest questId, when event flag flag is not set yet: sets it, requests
 * message msgNo and returns 1; else returns 0.
 */
s32 MsgEvent_CheckQuest(s32 flag, s32 questId, s8 msgNo)
{
    s32 flagNo;

    if ((questId & 0xFFFF) == gQuest.stage) {
        flagNo = flag & 0xFFFF;
        if (Save_TestEventFlag(flagNo) != 1) {
            Save_SetEventFlag(flagNo);
            gQuestMsgWindow->request = msgNo;
            return 1;
        }
    }
    return 0;
}

/*
 * Message event 2, when the player closes the supply box: in quest 1 requests
 * message 2 once (event flag 0xCA); in quest 18 requests message 1 once and
 * sets event flags 0x10B to 0x112. Returns 1 when it requested a message.
 */
s32 MsgEvent_CheckSupplyBoxClosed(void)
{
    switch (gQuest.stage) {
    case 1:
        if (Save_TestEventFlag(0xCA) != 1) {
            Save_SetEventFlag(0xCA);
            gQuestMsgWindow->request = 2;
            return 1;
        }
        break;
    case QUEST_ID_VELOCIDROME_SCENE_1:
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

/*
 * Returns the number of messages of set msgSet (the second half of its row of
 * gMsgEventSets).
 */
u8 MsgEvent_CountInSet(u8 msgSet)
{
    return gMsgEventSets[msgSet & 0xFF][1];
}

/*
 * Returns 1 when message msgNo of set msgSet was seen: its seen flag, the
 * set's first flag (gMsgEventSets) plus msgNo, is set.
 */
s32 MsgEvent_IsSeen(u8 msgSet, u8 msgNo)
{
    return Save_TestEventFlag(msgNo + gMsgEventSets[msgSet][0]);
}

/*
 * Returns the title of message msgNo of set msgSet: its entry in
 * gMsgEventSetEntries gives its text's index in gMsgEventTitles.
 */
char *MsgEvent_Title(u8 msgSet, u8 msgNo)
{
    u8 *entry = gMsgEventSetEntries[msgSet];
    entry += msgNo + msgNo;
    return gMsgEventTitles[*entry];
}

/*
 * Returns page pageNo of message msgNo of set msgSet (gMsgEventLines, by its
 * entry's text index).
 */
char *MsgEvent_Line(u8 msgSet, u8 msgNo, u8 pageNo)
{
    u8 *entry = gMsgEventSetEntries[msgSet];
    entry += msgNo + msgNo;
    return gMsgEventLines[*entry][pageNo];
}

/*
 * Returns the page count of message msgNo of set msgSet (the second byte of
 * its entry in gMsgEventSetEntries).
 */
u8 MsgEvent_PageCount(u8 msgSet, u8 msgNo)
{
    u8 *entry = gMsgEventSetEntries[msgSet];
    entry += msgNo + msgNo;
    return entry[1];
}

/*
 * Makes the area's LightShaft object; Area_StartObjects calls it in the areas
 * LightShaft_Setup places shafts in.
 */
void LightShaft_Create(void)
{
    Obj *obj = Obj_Alloc(0x1);

    if (obj != NULL) {
        obj->kind = 0;
        obj->update = LightShaft_Update;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * LightShaft_Setup, _Run, _End and _Free.
 */
void LightShaft_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        LightShaft_Setup(obj);
        break;
    case 1:
        LightShaft_Run(obj);
        break;
    case 2:
        LightShaft_End(obj);
        break;
    case 3:
        LightShaft_Free(obj);
        break;
    }
}

/*
 * Sets the object up for the current area: one or two shafts at the area's
 * positions (the LightShaft_Pos* tables), each on a draw slot of its own
 * drawn by LightShaft_Draw (DrawSlot.part is the shaft). Without a slot for
 * the first shaft it frees itself; else its first frame runs.
 */
void LightShaft_Setup(Obj *obj)
{
    DrawSlot *drawSlot;
    s16 count;
    s16 i;
    f32 *pos;
    LightShaftBuf *shafts;

    shafts = obj->buf;
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        count = 1;
        pos = LightShaft_PosForestHills7;
        break;
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        count = 2;
        pos = LightShaft_PosForestHills11;
        break;
    case AREA_FOREST_HILLS_12:
    case AREA_FOREST_HILLS_NIGHT_12:
        count = 1;
        pos = LightShaft_PosForestHills12;
        break;
    case AREA_SNOWY_MOUNTAINS_1:
    case AREA_SNOWY_MOUNTAINS_NIGHT_1:
        count = 1;
        pos = LightShaft_PosSnowyMountains1;
        break;
    case AREA_SNOWY_MOUNTAINS_3:
    case AREA_SNOWY_MOUNTAINS_NIGHT_3:
        count = 1;
        pos = LightShaft_PosSnowyMountains3;
        break;
    case AREA_TOWER_8:
        count = 2;
        pos = LightShaft_PosTower8;
        break;
    case AREA_DESERT_NIGHT_4:
    case AREA_DESERT_4:
        count = 2;
        pos = LightShaft_PosDesert4;
        break;
    case AREA_SWAMP_NIGHT_3:
    case AREA_SWAMP_3:
        count = 2;
        pos = LightShaft_PosSwamp3;
        break;
    case AREA_SWAMP_NIGHT_4:
    case AREA_SWAMP_4:
        count = 2;
        pos = LightShaft_PosSwamp4;
        break;
    }
    for (i = 0; i < 6; i++, pos += 3) {
        if (i < count) {
            shafts->slot[i] = DrawSlot_Alloc();
            if (shafts->slot[i] != -1) {
                drawSlot = DrawSlot_Get(shafts->slot[i]);
                shafts->ds[i] = drawSlot;
                drawSlot->owner = obj;
                drawSlot->part = i;
                drawSlot->draw = LightShaft_Draw;
                drawSlot->pos[0] = pos[0];
                drawSlot->pos[1] = pos[1];
                drawSlot->pos[2] = pos[2];
            } else if (i == 0) {
                LightShaft_Free(obj);
                return;
            } else {
                shafts->ds[i] = NULL;
            }
        } else {
            shafts->ds[i] = NULL;
        }
    }
    LightShaft_Run(obj);
}

/*
 * Counts the frames of the texture scroll (a 512-frame period in the snowy
 * mountains' areas 1 and 3, else 100) and adds each shaft's draw slot to the
 * sorted draw list. It reads the slots through a pointer view of its buffer:
 * entry i + 3 is LightShaftBuf.ds[i].
 */
void LightShaft_Run(Obj *obj)
{
    void **slotPtrs = obj->buf;
    s16 period;
    s16 i;

    obj->timer++;
    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_1:
    case AREA_SNOWY_MOUNTAINS_3:
    case AREA_SNOWY_MOUNTAINS_NIGHT_1:
    case AREA_SNOWY_MOUNTAINS_NIGHT_3:
        period = 0x200;
        break;
    default:
        period = 100;
        break;
    }
    if (obj->timer >= period) {
        obj->timer = 0;
    }
    for (i = 0; i < 6; i++) {
        if (slotPtrs[i + 3] != NULL) {
            OT_AddByDepth(gDrawOtSorted, slotPtrs[i + 3], 0x40, 0);
        }
    }
}

/* Hides the object; it is freed on the next update. */
void LightShaft_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void LightShaft_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws one shaft with additive blending, no alpha test and no depth writes:
 * a model of the area's set facing the camera's yaw (not turned in the snowy
 * mountains' areas), scaled and tilted per shaft in the forest's area 7,
 * widened and tilted in its area 11, tilted in its area 12, its texture
 * scrolled along u once per period (Obj.timer).
 */
void LightShaft_Draw(DrawSlot *drawSlot)
{
    Model *stage = gAreaSubModels[0];
    Obj *obj = drawSlot->owner;
    ModelMatSet *mdl;
    s32 period;
    f32 factor;
    s16 tiltZ;
    f32 mtx[16];
    f32 uvMtx[16];

    if (stage != NULL && stage->active != 0) {
        Gs_SetState(RS_ZWRITE, 0);
        Gs_SetState(RS_ALPHA_REF, 0);
        Gfx_SetBlend(4, 1);
        Mtx_Copy(mtx, gCamYawMtx);
        period = 100;
        switch (gScene.area) {
        case AREA_FOREST_HILLS_7:
        case AREA_FOREST_HILLS_NIGHT_7:
            Mtx_ScaleBy(mtx, LightShaft_ScalesForestHills7[drawSlot->part * 3], LightShaft_ScalesForestHills7Y[drawSlot->part * 3], LightShaft_ScalesForestHills7Z[drawSlot->part * 3]);
            tiltZ = LightShaft_TiltsForestHills7[drawSlot->part * 2 + 1];
            mdl = stage->matSets + 1;
            Mtx_RotateX(mtx, 6.2831855f * LightShaft_TiltsForestHills7[drawSlot->part * 2] / 360.0f);
            Mtx_RotateZ(mtx, 6.2831855f * tiltZ / 360.0f);
            break;
        case AREA_FOREST_HILLS_11:
        case AREA_FOREST_HILLS_NIGHT_11:
            factor = LightShaft_WidthsForestHills11[drawSlot->part];
            Mtx_ScaleBy(mtx, factor, 1.0f, factor);
            mdl = stage->matSets;
            Mtx_RotateZ(mtx, 6.2831855f * LightShaft_TiltsForestHills11[drawSlot->part] / 360.0f);
            break;
        case AREA_FOREST_HILLS_12:
        case AREA_FOREST_HILLS_NIGHT_12:
            mdl = stage->matSets;
            Mtx_RotateZ(mtx, 6.2831855f * LightShaft_TiltsForestHills12[drawSlot->part] / 360.0f);
            break;
        case AREA_SNOWY_MOUNTAINS_1:
        case AREA_SNOWY_MOUNTAINS_NIGHT_1:
            Mtx_Identity(mtx);
            period = 0x200;
            mdl = stage->matSets + 3;
            break;
        case AREA_SNOWY_MOUNTAINS_3:
        case AREA_SNOWY_MOUNTAINS_NIGHT_3:
            Mtx_Identity(mtx);
            period = 0x200;
            mdl = stage->matSets + 3;
            break;
        case AREA_TOWER_8:
            mdl = stage->matSets + drawSlot->part + 2;
            break;
        case AREA_DESERT_NIGHT_4:
        case AREA_DESERT_4:
            mdl = stage->matSets + LightShaft_ModelsDesert4[drawSlot->part];
            break;
        case AREA_SWAMP_NIGHT_3:
        case AREA_SWAMP_NIGHT_4:
        case AREA_SWAMP_3:
        case AREA_SWAMP_4:
            mdl = stage->matSets;
            break;
        }
        factor = (1.0f / period) * obj->timer;
        Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        Mtx_Translation(uvMtx, factor, 0.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        if (mdl->id != -1) {
            Model_Draw(mdl->id, 0);
        }
        Gfx_SetBlend(4, 5);
        Gs_SetState(RS_ZWRITE, 1);
    }
}

/*
 * Makes a SkyStreak object of type streakType (Area_StartObjects makes type 0
 * in the desert's night area 3).
 */
void SkyStreak_Create(u8 streakType)
{
    Obj *obj = Obj_Alloc(1);

    if (obj != NULL) {
        obj->kind = 2;
        obj->type = streakType;
        obj->update = SkyStreak_Update;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * SkyStreak_Setup, _Run, _End and _Free.
 */
void SkyStreak_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        SkyStreak_Setup(obj);
        break;
    case 1:
        SkyStreak_Run(obj);
        break;
    case 2:
        SkyStreak_End(obj);
        break;
    case 3:
        SkyStreak_Free(obj);
        break;
    }
}

/*
 * Sets up the type's parts (SkyStreak_Counts): each waits 100 to 163 frames
 * (SkyStreak_WaitBase plus random SkyStreak_WaitMask bits) with a random
 * model variant and a draw slot of its own (SkyStreak_Draw); then the first
 * frame runs.
 */
void SkyStreak_Setup(Obj *obj)
{
    SkyStreakPart *streak = obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    obj->var0 = 0;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var3 = 0;
    obj->timer = 0;
    obj->fvar = 0.0f;
    count = SkyStreak_Counts[obj->type];
    for (i = 0; i < count; i++, streak++) {
        streak->variant = Rand_Next(1) % SkyStreak_RandRange[obj->type];
        streak->phase = 0;
        streak->unk2A = 0;
        streak->frame = 0;
        streak->wait = SkyStreak_WaitBase[obj->type] + (SkyStreak_WaitMask[obj->type] & Rand_Next(1));
        streak->pos[0] = 0.0f;
        streak->pos[1] = 0.0f;
        streak->pos[2] = 0.0f;
        streak->slot = DrawSlot_Alloc();
        if (streak->slot != -1) {
            streak->ds = DrawSlot_Get(streak->slot);
            if (streak->ds != NULL) {
                streak->ds->owner = obj;
                streak->ds->part = i;
                streak->ds->draw = SkyStreak_Draw;
            }
        } else {
            streak->ds = NULL;
        }
    }
    SkyStreak_Run(obj);
}

/*
 * Steps each part: after its wait it picks a random model variant (below
 * SkyStreak_RandRange) and shows it for SkyStreak_Frames frames, added to the
 * 8-entry draw list, then waits again.
 */
void SkyStreak_Run(Obj *obj)
{
    SkyStreakPart *part = obj->buf;
    s16 count = SkyStreak_Counts[obj->type];
    s16 i;

    for (i = 0; i < count; i++, part++) {
        switch (part->phase) {
        case 0:
            if (++part->frame >= part->wait) {
                part->phase++;
                part->frame = 0;
                part->variant = Rand_Next(1) % SkyStreak_RandRange[obj->type];
            }
            break;
        case 1:
            if (++part->frame >= SkyStreak_Frames[obj->type]) {
                part->phase = 0;
                part->frame = 0;
                part->wait = SkyStreak_WaitBase[obj->type] + (SkyStreak_WaitMask[obj->type] & Rand_Next(1));
            } else if (part->ds != NULL) {
                Vec_Copy((f32 *)part->ds->pos, (f32 *)part->pos);
                OT_AddByDepth(gDrawOt8, part->ds, 8, 1);
            }
            break;
        }
    }
}

/*
 * Frees each part's draw slot and hides the object; it is freed on the next update.
 */
void SkyStreak_End(Obj *obj)
{
    SkyStreakPart *part = obj->buf;
    s16 count;
    s16 i;

    obj->state++;
    count = SkyStreak_Counts[obj->type];
    for (i = 0; i < count; i++) {
        if (part->ds != NULL) {
            DrawSlot_Free(part->slot);
        }
        part++;
    }
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void SkyStreak_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws a part's model (the type's first model, SkyStreak_Models, plus its
 * variant) at the part's position, the origin, the model carrying its own
 * place; no alpha test, a white colour, its texture scrolled once along v
 * over the showing.
 */
void SkyStreak_Draw(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    SkyStreakPart *streak = (SkyStreakPart *) obj->buf + drawSlot->part;
    f32 mtx[16];
    f32 uvMtx[16];
    ModelMatSet *model;

    if (stage != NULL && stage->active != 0) {
        model = stage->matSets + SkyStreak_Models[obj->type] + streak->variant;
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        Gs_SetState(RS_ALPHA_REF, 0);
        Gs_SetState(RS_COLOR2, -1);
        Mtx_Translation(uvMtx, 0.0f, (f32) streak->frame / (f32) SkyStreak_Frames[obj->type], 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        if (model != NULL && model->id != -1) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Makes the area's ambient AreaDrop (type 0); Area_StartObjects calls it in
 * the desert's areas 4 and 10, the swamp's 3, 4 and 8 and the tower's area 8.
 */
void AreaDrop_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x3;
        obj->type = 0;
        obj->update = AreaDrop_Update;
    }
}

/*
 * Makes a weapon burst (type 1) at spot; AreaWeapon_Run makes them at its
 * burst points when a great weapon fires.
 */
void AreaDrop_CreateWeaponBurst(void *spot)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 3;
        obj->type = 1;
        obj->update = AreaDrop_Update;
        Vec_Copy((f32 *)obj->pos, (f32 *)spot);
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * AreaDrop_Setup, _Run, _End and _Free.
 */
void AreaDrop_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AreaDrop_Setup(obj);
        break;
    case 1:
        AreaDrop_Run(obj);
        break;
    case 2:
        AreaDrop_End(obj);
        break;
    case 3:
        AreaDrop_Free(obj);
        break;
    }
}

/*
 * Sets the object up: type 0 waits 10 to 41 frames, type 1 starts its scroll
 * at 0. It takes a draw slot (AreaDrop_Draw) and runs its first frame, or
 * frees itself without one.
 */
void AreaDrop_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var3 = 0;
    switch (obj->type) {
    case 0:
        obj->timer = (Rand_Next(1) & 0x1F) + 10;
        break;
    case 1:
        obj->timer = 0;
        obj->fvar = 0.0f;
        break;
    }
    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlot_Get(obj->slot);
        obj->ds->owner = obj;
        obj->ds->draw = AreaDrop_Draw;
        AreaDrop_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Runs the object by area: drips (AreaDrop_RunDrip) in the desert's area 4,
 * the swamp's 3, 4 and 8 and the tower's 8; weapon bursts
 * (AreaDrop_RunWeaponBurst) in the fortress's area 5, the castle's 1 and the
 * battle town's 1; and here the desert's area 10, where after a random wait
 * one of four points (AreaDrop_FallPointsDesert10) shows its falling stream
 * for 100 frames, with a sound on frame 2 and an Effect0D puff at the floor
 * 15, 20, 25, 30 and 35 frames before the end.
 */
void AreaDrop_Run(Obj *obj)
{
    f32 (*points)[3];
    f32 pos[3];

    switch (gScene.area) {
    case AREA_DESERT_NIGHT_10:
    case AREA_DESERT_10:
        points = (f32 (*)[3]) AreaDrop_FallPointsDesert10;
        break;
    case AREA_DESERT_NIGHT_4:
    case AREA_SWAMP_NIGHT_3:
    case AREA_SWAMP_NIGHT_8:
    case AREA_TOWER_8:
    case AREA_DESERT_4:
    case AREA_SWAMP_3:
    case AREA_SWAMP_4:
    case AREA_SWAMP_8:
        AreaDrop_RunDrip(obj);
        return;
    case AREA_FORTRESS_5:
    case AREA_CASTLE_1:
    case AREA_FORTRESS_NIGHT_5:
    case AREA_BATTLE_TOWN_1:
    case AREA_BATTLE_TOWN_NIGHT_1:
        AreaDrop_RunWeaponBurst(obj);
        return;
    default:
        return;
    }
    switch (obj->var0) {
    case 0:
        if (--obj->timer <= 0) {
            obj->var0++;
            obj->var1 = Rand_Next(1) % 4;
            obj->timer = 0;
        }
        break;
    case 1:
        if (++obj->timer >= 100) {
            obj->var0 = 0;
            obj->timer = Rand_Next(1) & 0x3F;
        } else {
            obj->ds->pos[0] = points[obj->var1][0];
            obj->ds->pos[1] = points[obj->var1][1];
            obj->ds->pos[2] = points[obj->var1][2];
            if (obj->timer == 2) {
                SoundObj_Start(7, 0x20, 1, obj->ds->pos, 100, 0);
            }
            OT_AddByDepth(gDrawOtSorted, obj->ds, 0x40, 0);
        }
        switch (100 - obj->timer) {
        case 0xF:
        case 0x14:
        case 0x19:
        case 0x1E:
        case 0x23:
            pos[0] = obj->ds->pos[0];
            pos[1] = -10.0f;
            pos[2] = obj->ds->pos[2];
            PartFx_SpawnAt(pos, 0, 1.75f);
            break;
        }
        break;
    }
}

/*
 * A drip: after a random wait it picks a spot on the ground 450 to 1473 units
 * from the camera, in a 90-degree fan around the camera's heading turned a
 * quarter, and keeps it only inside one of the area's drip zones (the
 * AreaDrop_Zones* centres and radii; else it tries again the next frame). The
 * drop then falls 1000 units in 30 frames and its splash spreads for 60
 * frames before a new wait of up to 63 frames; Obj.var2 is the model each
 * step shows.
 */
void AreaDrop_RunDrip(Obj *obj)
{
    f32 *radius;
    f32 *zoneCenter;
    u8 i;
    s16 zoneCount;
    u16 angle;
    f32 *spotZ;
    f32 spot[3];
    f32 camPos[3];
    f32 height;

    switch (obj->var0) {
    case 0:
        if (--obj->timer <= 0) {
            angle = Mtx_Heading(gCamMatrix) + 0x4000;
            spot[0] = 0.0f;
            spot[1] = 0.0f;
            spotZ = &spot[2];
            *spotZ = -450 - (Rand_Next(1) & 0x3FF);
            angle += (u16) ((Rand_Next(1) & 0x3FFF) - 0x1FFF);
            Vec_RotateY(spot, 6.2831855f * angle / 65536.0f);
            Mtx_GetTrans(camPos, gCamMatrix);
            spot[0] += camPos[0];
            *spotZ += camPos[2];
            switch (gScene.area) {
            default:
            case AREA_TOWER_8:
                obj->var2 = 5;
                zoneCount = 1;
                zoneCenter = AreaDrop_ZonesTower8;
                radius = AreaDrop_ZoneRadiiTower8;
                break;
            case AREA_DESERT_NIGHT_4:
                obj->var2 = 8;
                zoneCount = 1;
                zoneCenter = AreaDrop_ZonesDesertNight4;
                radius = AreaDrop_ZoneRadiiDesertNight4;
                break;
            case AREA_SWAMP_NIGHT_3:
                zoneCount = 2;
                obj->var2 = 2;
                zoneCenter = AreaDrop_ZonesSwamp3;
                radius = AreaDrop_ZoneRadiiSwamp3;
                break;
            case AREA_SWAMP_NIGHT_8:
                obj->var2 = 1;
                zoneCount = 2;
                zoneCenter = AreaDrop_ZonesSwamp8;
                radius = AreaDrop_ZoneRadiiSwamp8;
                break;
            case AREA_DESERT_4:
                obj->var2 = 8;
                zoneCount = 1;
                zoneCenter = AreaDrop_ZonesDesert4;
                radius = AreaDrop_ZoneRadiiDesert4;
                break;
            case AREA_SWAMP_3:
                zoneCount = 2;
                obj->var2 = 2;
                zoneCenter = AreaDrop_ZonesSwamp3;
                radius = AreaDrop_ZoneRadiiSwamp3;
                break;
            case AREA_SWAMP_4:
                zoneCount = 2;
                obj->var2 = 2;
                zoneCenter = AreaDrop_ZonesSwamp4;
                radius = AreaDrop_ZoneRadiiSwamp4;
                break;
            case AREA_SWAMP_8:
                obj->var2 = 1;
                zoneCount = 2;
                zoneCenter = AreaDrop_ZonesSwamp8;
                radius = AreaDrop_ZoneRadiiSwamp8;
                break;
            }
            for (i = 0; i < zoneCount; i++) {
                if (!(Vec_Distance(spot, zoneCenter) < *radius) == 0) {
                    Vec_Copy(obj->pos, (f32 *)spot);
                    obj->var0++;
                    obj->timer = 0;
                    return;
                }
                zoneCenter += 3;
                radius++;
            }
        }
        break;
    case 1:
        if (++obj->timer >= 30) {
            switch (gScene.area) {
            default:
            case AREA_TOWER_8:
                obj->var2 = 4;
                break;
            case AREA_DESERT_NIGHT_4:
            case AREA_DESERT_4:
                obj->var2 = 5;
                break;
            case AREA_SWAMP_NIGHT_3:
            case AREA_SWAMP_3:
                obj->var2 = 1;
                break;
            case AREA_SWAMP_4:
                obj->var2 = 1;
                break;
            case AREA_SWAMP_NIGHT_8:
            case AREA_SWAMP_8:
                obj->var2 = 0;
                break;
            }
            obj->var0++;
            obj->timer = 0;
            return;
        }
        height = 1000.0f - 33.333332f * obj->timer;
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1] + height;
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOtSorted, obj->ds, 0x40, 0);
        break;
    case 2:
        if (++obj->timer >= 60) {
            obj->var0 = 0;
            obj->timer = Rand_Next(1) & 0x3F;
            return;
        }
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOt8, obj->ds, 8, 0);
        break;
    }
}

/*
 * A weapon burst: for 20 frames moves its texture by 0.05 a frame (Obj.fvar,
 * up to 1.0) at its position, then ends.
 */
void AreaDrop_RunWeaponBurst(Obj *obj)
{

    if (++obj->timer >= 20) {
        obj->state++;
        return;
    }
    obj->fvar += 0.05f;
    if (obj->fvar > 1.0f) {
        obj->fvar = 1.0f;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOtSorted, obj->ds, 0x40, 0);
}

/*
 * Frees the draw slot, when there is one, and hides the object; it is freed
 * on the next update.
 */
void AreaDrop_End(Obj *obj)
{
    if (obj->ds != NULL) {
        DrawSlot_Free(obj->slot);
    }
    obj->state++;
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void AreaDrop_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws the object, facing the camera except a weapon burst: the desert's
 * area 10 stream with its texture scrolled down through its 100 frames; a
 * drip's drop fading in as it falls, then its splash widening from nothing
 * while it fades and scrolls; a weapon burst flat (turned 26 degrees in the
 * castle) without depth writes, fading out over its 20 frames, its texture
 * moved by Obj.fvar. No alpha test; depth writes are back on after.
 */
void AreaDrop_Draw(DrawSlot *drawSlot)
{
    Obj *obj;
    ModelMatSet *mdl;
    u8 flat;
    Model *stage;
    u8 alpha;
    f32 spread;
    f32 mtx[16];
    f32 uvMtx[16];

    stage = gAreaSubModels[0];
    obj = drawSlot->owner;
    flat = 0;

    if (stage != NULL && stage->active != 0) {
        Gs_SetState(RS_ALPHA_REF, 0);
        Gs_SetState(RS_COLOR2, -1);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        switch (gScene.area) {
        case AREA_DESERT_NIGHT_10:
        case AREA_DESERT_10:
            mdl = stage->matSets + 1;
            Mtx_Translation(uvMtx, 0.0f, 1.0f - obj->timer / 100.0f, 0.0f);
            break;
        case AREA_DESERT_NIGHT_4:
        case AREA_SWAMP_NIGHT_3:
        case AREA_SWAMP_NIGHT_8:
        case AREA_TOWER_8:
        case AREA_DESERT_4:
        case AREA_SWAMP_3:
        case AREA_SWAMP_4:
        case AREA_SWAMP_8:
            mdl = stage->matSets + obj->var2;
            if (obj->var0 == 1) {
                alpha = 8.5f * obj->timer;
            } else {
                spread = 0.016666668f * obj->timer;
                Mtx_SetScale(mtx, spread, 1.0f, spread);
                Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
                alpha = 0xFF - (u8) (4.25f * obj->timer);
                Mtx_Translation(uvMtx, 0.0f, 0.016666668f * obj->timer, 0.0f);
            }
            Gs_SetState(RS_COLOR2, (alpha << 24) | 0xFFFFFF);
            break;
        case AREA_BATTLE_TOWN_1:
        case AREA_BATTLE_TOWN_NIGHT_1:
            Mtx_Translation(uvMtx, 0.0f, obj->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 5;
            Gs_SetState(RS_ZWRITE, 0);
            Gs_SetState(RS_COLOR2, ((u8) (0xFF - (u8) (12.75f * obj->timer)) << 24) | 0xFFFFFF);
            break;
        case AREA_FORTRESS_5:
            Mtx_Translation(uvMtx, 0.0f, obj->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 7;
            Gs_SetState(RS_ZWRITE, 0);
            Gs_SetState(RS_COLOR2, ((u8) (0xFF - (u8) (12.75f * obj->timer)) << 24) | 0xFFFFFF);
            break;
        case AREA_FORTRESS_NIGHT_5:
            Mtx_Translation(uvMtx, 0.0f, obj->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 8;
            Gs_SetState(RS_ZWRITE, 0);
            Gs_SetState(RS_COLOR2, ((u8) (0xFF - (u8) (12.75f * obj->timer)) << 24) | 0xFFFFFF);
            break;
        case AREA_CASTLE_1:
            Mtx_RotateY(mtx, 0.45378563f);
            Mtx_Translation(uvMtx, 0.0f, obj->fvar, 0.0f);
            flat = 1;
            mdl = stage->matSets + 15;
            Gs_SetState(RS_ZWRITE, 0);
            Gs_SetState(RS_COLOR2, ((u8) (0xFF - (u8) (12.75f * obj->timer)) << 24) | 0xFFFFFF);
            break;
        }
        if (flat == 0) {
            Mtx_Concat3x3(mtx, gCamMatrix);
        }
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        if (mdl != NULL && mdl->id != -1) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
        Gs_SetState(RS_ZWRITE, 1);
    }
}

/*
 * Makes the area's flag, except in the fortress's area 3 once
 * gScene.emEventFlags bit 0 is set (Shen Gaoren and Lao-Shan Lung set it).
 */
void AreaFlag_Create(void)
{
    Obj *obj;

    if ((gScene.area == AREA_FORTRESS_3 || gScene.area == AREA_FORTRESS_NIGHT_3) && (gScene.emEventFlags & 1)) {
        return;
    }
    obj = Obj_Alloc(0);
    if (obj != NULL) {
        obj->kind = 4;
        obj->update = AreaFlag_Update;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * AreaFlag_Setup, _Run, _End and _Free.
 */
void AreaFlag_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AreaFlag_Setup(obj);
        break;
    case 1:
        AreaFlag_Run(obj);
        break;
    case 2:
        AreaFlag_End(obj);
        break;
    case 3:
        AreaFlag_Free(obj);
        break;
    }
}

/*
 * Places the flag at its area's fixed position (the fortress's areas 1 and 3,
 * the arenas' area 0, the great arena's and the arena's area 1, the snowy
 * mountains' area 6); in another area, or without a draw slot, it frees
 * itself, else its first frame runs.
 */
void AreaFlag_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    obj->var3 = 0;
    switch (gScene.area) {
    case AREA_FORTRESS_1:
    case AREA_FORTRESS_NIGHT_1:
        obj->pos[0] = 12680.0f;
        obj->pos[1] = 696.0f;
        obj->pos[2] = 12756.0f;
        break;
    case AREA_FORTRESS_3:
    case AREA_FORTRESS_NIGHT_3:
        obj->pos[0] = 10400.0f;
        obj->pos[1] = 1300.0f;
        obj->pos[2] = 20570.0f;
        break;
    case AREA_ARENAS_NIGHT_0:
    case AREA_ARENAS_0:
        obj->pos[0] = 10007.0f;
        obj->pos[1] = 200.0f;
        obj->pos[2] = 9377.0f;
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
    case AREA_GREAT_ARENA_1:
        obj->pos[0] = 5129.0f;
        obj->pos[1] = 1513.0f;
        obj->pos[2] = 10935.0f;
        break;
    case AREA_ARENA_NIGHT_1:
    case AREA_ARENA_1:
        obj->pos[0] = 9908.0f;
        obj->pos[1] = 2957.0f;
        obj->pos[2] = 15082.0f;
        break;
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        obj->pos[0] = 11600.0f;
        obj->pos[1] = 4460.0f;
        obj->pos[2] = 7610.0f;
        break;
    default:
        Obj_Free(obj);
        return;
    }
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = AreaFlag_Draw;
        Vec_Copy(obj->ds->pos, obj->pos);
        AreaFlag_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Steps the flag's 16-frame animation (Obj.timer), plays its sound every 55
 * frames (Obj.var3 counts frames) and adds it to the 32-entry draw list. In
 * the fortress's area 3 it stops once emEventFlags bit 0 is set; in the snowy
 * mountains' area 6 it runs only once a set-down delivery is done
 * (QuestGoal_DeliveryDone) and rises 1.95 units a frame up to height 4850.
 */
void AreaFlag_Run(Obj *obj)
{
    s16 firstFrame;
    s16 lastFrame;

    obj->var3++;
    switch (gScene.area) {
    case AREA_FORTRESS_1:
        firstFrame = 2;
        lastFrame = 0x11;
        break;
    case AREA_FORTRESS_NIGHT_1:
        firstFrame = 3;
        lastFrame = 0x12;
        break;
    case AREA_FORTRESS_3:
    case AREA_FORTRESS_NIGHT_3:
        if (gScene.emEventFlags & 1) {
            return;
        }
        firstFrame = 4;
        lastFrame = 0x13;
        break;
    case AREA_ARENAS_NIGHT_0:
    case AREA_ARENAS_0:
        firstFrame = 1;
        lastFrame = 0x10;
        break;
    case AREA_GREAT_ARENA_NIGHT_1:
    case AREA_GREAT_ARENA_1:
        firstFrame = 6;
        lastFrame = 0x15;
        break;
    case AREA_ARENA_NIGHT_1:
    case AREA_ARENA_1:
        firstFrame = 1;
        lastFrame = 0x10;
        break;
    case AREA_SNOWY_MOUNTAINS_6:
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        switch (QuestGoal_DeliveryDone()) {
        case 0:
            return;
        case 1:
            firstFrame = 0xD;
            lastFrame = 0x1C;
            break;
        case 2:
            firstFrame = 0x1D;
            lastFrame = 0x2C;
            break;
        }
        obj->pos[1] += 1.95f;
        if (obj->pos[1] > 4850.0f) {
            obj->pos[1] = 4850.0f;
        }
        break;
    default:
        return;
    }
    if (++obj->timer >= lastFrame - firstFrame + 1) {
        obj->timer = 0;
    }
    if (obj->var3 % 55 == 0) {
        Sound_PlaySeAtPosChecked(7, 0x52, 0, obj->pos, 0x11, 0);
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
}

/* Hides the object; it is freed on the next update. */
void AreaFlag_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void AreaFlag_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws the flag's current frame (the area's first frame model plus
 * Obj.timer), swaying up to 30 degrees either way about y over 256 frames,
 * except in the fortress's area 3.
 */
void AreaFlag_Draw(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *mdl;
    s16 firstModel;
    f32 mtx[16];

    if (stage != NULL && stage->active != 0) {
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        switch (gScene.area) {
        case AREA_FORTRESS_1:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 2;
            break;
        case AREA_FORTRESS_NIGHT_1:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 3;
            break;
        case AREA_FORTRESS_3:
        case AREA_FORTRESS_NIGHT_3:
            firstModel = 4;
            break;
        case AREA_ARENAS_NIGHT_0:
        case AREA_ARENAS_0:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 1;
            break;
        case AREA_GREAT_ARENA_NIGHT_1:
        case AREA_GREAT_ARENA_1:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 6;
            break;
        case AREA_ARENA_NIGHT_1:
        case AREA_ARENA_1:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 1;
            break;
        case AREA_SNOWY_MOUNTAINS_6:
        case AREA_SNOWY_MOUNTAINS_NIGHT_6:
            Mtx_RotateY(mtx, 6.2831855f * (30.0f * Math_Sin(6.2831855f * (obj->var3 << 8) / 65536.0f)) / 360.0f);
            firstModel = 13;
            break;
        default:
            return;
        }
        mdl = stage->matSets + firstModel + obj->timer;
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        if (mdl != NULL && mdl->id != -1) {
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Makes the parts of the area's weapon set weaponSet (0 to 9, two to five
 * parts), one object a part with its index in Obj.var2.
 */
void AreaWeapon_Create(u8 weaponSet)
{
    u8 count;
    u8 i;
    Obj *obj;

    switch (weaponSet) {
    case 0:
    case 2:
    case 4:
    case 5:
    case 7:
    case 8:
        count = 3;
        break;
    case 1:
    case 6:
        count = 5;
        break;
    case 3:
        count = 2;
        break;
    case 9:
        count = 4;
        break;
    default:
        count = 0;
        break;
    }
    for (i = 0; i < count; i++) {
        obj = Obj_Alloc(0);
        if (obj != NULL) {
            obj->kind = 5;
            obj->type = weaponSet;
            obj->update = AreaWeapon_Update;
            obj->var2 = i;
        }
    }
}

/*
 * The part's update, run every frame: switches on Obj.state to
 * AreaWeapon_Setup, _Run, _End and _Free.
 */
void AreaWeapon_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AreaWeapon_Setup(obj);
        break;
    case 1:
        AreaWeapon_Run(obj);
        break;
    case 2:
        AreaWeapon_End(obj);
        break;
    case 3:
        AreaWeapon_Free(obj);
        break;
    }
}

/*
 * Sets a part up from its set's tables: its position (AreaWeapon_PartPos)
 * and, for a great weapon, its countdown state (waiting, or done for a
 * countdown already running). It takes a draw slot (AreaWeapon_Draw) and runs
 * its first frame, or frees itself without one.
 */
void AreaWeapon_Setup(Obj *obj)
{
    u8 partKind = AreaWeapon_PartKinds[obj->type][obj->var2];
    f32 *posSrc;

    switch (obj->type) {
    case 1:
    case 6:
        posSrc = AreaWeapon_PosFortress5[obj->var2];
        break;
    case 2:
        posSrc = AreaWeapon_PosCastle1[obj->var2];
        break;
    case 8:
        posSrc = AreaWeapon_PosBattleTown1[obj->var2];
        break;
    case 9:
        posSrc = AreaWeapon_PosBattleTown3[obj->var2];
        break;
    default:
        posSrc = AreaWeapon_PartPos[obj->type] + obj->var2 * 3;
        break;
    }
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->timer = 0;
    obj->var3 = 0;
    if (partKind == 2) {
        if (gScene.countdownOn == 0) {
            obj->var0 = 0;
        } else {
            obj->timer = gQuest.countdown;
            obj->var0 = 5;
        }
    }
    obj->pos[0] = *posSrc++;
    obj->pos[1] = *posSrc++;
    obj->pos[2] = *posSrc;
    obj->slot = DrawSlot_Alloc();
    obj->ds = DrawSlot_Get(obj->slot);
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = AreaWeapon_Draw;
        Vec_Copy(obj->ds->pos, obj->pos);
        AreaWeapon_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

s32 Obj_InCurrentArea(void *obj);
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
s32 Player_IsLocal(void *obj);
/* unit's own declaration: declared here with ang as u16 though the function takes an s32: AreaWeapon_Run compiles as the game's code only with this declaration. */
void StageShot_CreateAt(f32 *pos, u8 code, u8 area, u16 ang);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void FireBlast_CreateScaled(f32 *pos, s16 k, f32 scale);
extern u16 AreaWeapon_GunTurnSounds[];
extern u16 AreaWeapon_GunRestYawBattleTown1[];
extern f32 AreaWeapon_BurstPosFortress5[];
extern f32 AreaWeapon_BurstPosCastle1[];
extern f32 AreaWeapon_BurstPosBattleTown1[];
extern s32 gQuestCountdown;
/*
 * Runs a part by its part kind. A mounted gun follows the player operating it
 * (motion 0x19E past frame 66, within 200 units; the local player first): his
 * heading goes to Obj.timer and his aim pitch to Obj.var3, its turning sound
 * plays at most every 10 frames, and with no one on it it goes back to its
 * rest heading. A great weapon waits for the quest countdown: 45 frames after
 * the countdown starts it makes the weapon bursts
 * (AreaDrop_CreateWeaponBurst), StageObj_K0E effects and its StageShot hit,
 * thrusts out 1300 units along its yaw in 4 frames (with its sound), holds 90
 * frames, pulls back over 150 and waits for the next countdown. The countdown
 * test is a switch with its default first: the compiler then leaves the
 * branch delay slots of the whole function empty, as in the game's code.
 */
void AreaWeapon_Run(Obj *obj)
{
    Player *player;
    u8 *unusedPtr;
    s16 i;
    u8 local;
    u16 ang;
    u8 code;
    u8 unusedIdx;
    Player *localPl;
    f32 *offsetY;
    f32 *offsetZ;
    f32 offset[3];
    f32 effectPos[3];
    f32 burstOfs[3];
    u8 flags[4];

    offsetY = &offset[1];
    local = 0;
    offsetZ = &offset[2];
    offset[0] = 0.0f;
    *offsetY = 0.0f;
    *offsetZ = 0.0f;
    player = gPlayers;
    switch (AreaWeapon_PartKinds[obj->type][obj->var2]) {
    case 2:
        obj->timer++;
        switch (obj->type) {
        case 1:
        case 6:
            ang = 0;
            code = 2;
            break;
        case 2:
            ang = AreaWeapon_YawCastle1[obj->var2];
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
        switch (obj->var0) {
        case 0:
            switch (gScene.countdownOn) {
            default:
                obj->var0++;
                obj->timer = 0;
                break;
            case 0:
                break;
            }
            break;
        case 1:
            if (obj->timer > 0x2D) {
                obj->timer = 0;
                switch (obj->type) {
                case 8:
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosBattleTown1);
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosBattleTown1 + 3);
                    Vec_Copy((f32 *)effectPos, (f32 *)AreaWeapon_BurstPosBattleTown1);
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 2.0f);
                    Vec_Copy((f32 *)effectPos, (f32 *)(AreaWeapon_BurstPosBattleTown1 + 3));
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 2.0f);
                    break;
                case 1:
                case 6:
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosFortress5);
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosFortress5 + 3);
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosFortress5 + 6);
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosFortress5 + 9);
                    Vec_Copy((f32 *)effectPos, (f32 *)AreaWeapon_BurstPosFortress5);
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 1.0f);
                    Vec_Copy((f32 *)effectPos, (f32 *)(AreaWeapon_BurstPosFortress5 + 3));
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 1.0f);
                    Vec_Copy((f32 *)effectPos, (f32 *)(AreaWeapon_BurstPosFortress5 + 6));
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 1.0f);
                    Vec_Copy((f32 *)effectPos, (f32 *)(AreaWeapon_BurstPosFortress5 + 9));
                    effectPos[2] += 100.0f;
                    FireBlast_CreateScaled(effectPos, 8, 1.0f);
                    break;
                case 2:
                    AreaDrop_CreateWeaponBurst(AreaWeapon_BurstPosCastle1);
                    Vec_Copy((f32 *)effectPos, (f32 *)AreaWeapon_BurstPosCastle1);
                    burstOfs[2] = 100.0f;
                    burstOfs[0] = 0.0f;
                    burstOfs[1] = 0.0f;
                    Vec_RotateY(burstOfs, 0.45378563f);
                    Vec_Add(effectPos, effectPos, burstOfs);
                    FireBlast_CreateScaled(effectPos, 8, 2.0f);
                    break;
                }
                obj->var0++;
                if (code != 0xFF) {
                    switch (obj->type) {
                    case 8:
                        StageShot_CreateAt(obj->ds->pos, code, gScene.area, ang);
                        break;
                    default:
                        StageShot_CreateAt(obj->pos, code, gScene.area, ang);
                        break;
                    }
                }
            }
            break;
        case 2:
            obj->var1++;
            *offsetZ = 1300.0f * (obj->timer / 4.0f);
            if (obj->timer >= 4) {
                switch (obj->type) {
                case 8:
                    Sound_PlaySeAtPosChecked(7, 0x36, 0, obj->ds->pos, 7, 0);
                    break;
                default:
                    Sound_PlaySeAtPosChecked(7, 0x36, 0, obj->pos, 7, 0);
                    break;
                }
                obj->timer = 0;
                obj->var0++;
            }
            break;
        case 3:
            obj->var1++;
            *offsetZ = 1300.0f;
            if (obj->timer >= 90) {
                obj->timer = 0;
                obj->var0++;
            }
            break;
        case 4:
            obj->var1++;
            *offsetZ = 1300.0f - 1300.0f * (obj->timer / 150.0f);
            if (obj->timer >= 150) {
                obj->timer = gQuestCountdown;
                obj->var0 = 5;
            }
            break;
        case 5:
            if (obj->timer < gQuestCountdown) {
                obj->var0 = 1;
                obj->timer = 0;
            } else {
                if (gScene.countdownOn == 0) {
                    obj->var0 = 0;
                }
                obj->timer = gQuestCountdown;
            }
            break;
        }
        Vec_RotateY(offset, 6.2831855f * ang / 65536.0f);
        switch (obj->type) {
        case 8:
            Vec_Copy(obj->pos, (f32 *)offset);
            OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 1);
            return;
        }
        break;
    case 1:
        if (obj->var1 > 0) {
            obj->var1--;
        }
        for (i = 0; i < 4; i++, player++) {
            flags[i] = 0;
            if (player->active && player->visible && (u8) Obj_InCurrentArea(player) && player->motionId[0] == 0x19E &&
                Motion_ReachedFrame(player, 0, 66.0f)) {
                flags[i] = 1;
                if (Player_IsLocal(player)) {
                    local = 1;
                }
            }
        }
        if (local) {
            offset[0] = obj->pos[0];
            localPl = &gPlayers[gScene.localPlayer];
            offset[1] = localPl->pos[1];
            offset[2] = obj->pos[2];
            if (Vec_Distance(localPl->pos, offset) <= 200.0f) {
                if (obj->var1 == 0 && (u16) obj->timer != localPl->rot[1]) {
                    Sound_PlaySeAtPosChecked(7, AreaWeapon_GunTurnSounds[(u8) AreaWeapon_GunIndex(obj)], 0, obj->pos, 1, 0);
                    obj->var1 = 10;
                }
                obj->var3 = localPl->aimPitchPct;
                obj->timer = localPl->rot[1];
                obj->var4 = gScene.localPlayer;
                goto done;
            }
        }
        player = gPlayers;
        for (i = 0; i < 4; i++, player++) {
            if (flags[i] && player->active && player->visible && (u8) Obj_InCurrentArea(player) && !Player_IsLocal(player)) {
                offset[0] = obj->pos[0];
                offset[1] = player->pos[1];
                offset[2] = obj->pos[2];
                if (Vec_Distance(player->pos, offset) <= 200.0f) {
                    if (obj->var1 == 0 && (u16) obj->timer != player->rot[1]) {
                        Sound_PlaySeAtPosChecked(7, AreaWeapon_GunTurnSounds[(u8) AreaWeapon_GunIndex(obj)], 0, obj->pos, 1, 0);
                        obj->var1 = 10;
                    }
                    obj->var3 = player->aimPitchPct;
                    obj->timer = player->rot[1];
                    obj->var4 = i;
                    goto done;
                }
            }
        }
        obj->var3 = 0;
        switch (obj->type) {
        case 1:
        case 6:
        case 9:
            obj->timer = 0;
            break;
        case 2:
            obj->timer = AreaWeapon_YawCastle1[obj->var2];
            break;
        case 8:
            obj->timer = AreaWeapon_GunRestYawBattleTown1[obj->var2];
            break;
        }
    done:
        offset[0] = 0.0f;
        *offsetY = 0.0f;
        *offsetZ = 0.0f;
        break;
    }
    obj->ds->pos[0] = obj->pos[0] + offset[0];
    obj->ds->pos[1] = obj->pos[1] + *offsetY;
    obj->ds->pos[2] = obj->pos[2] + *offsetZ;
    OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 1);
}

/* Hides the part and frees its draw slot; it is freed on the next update. */
void AreaWeapon_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
    DrawSlot_Free(obj->slot);
}

/* Frees the part (Obj_Free). */
void AreaWeapon_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws a part, only in its set's day area or that area's night twin
 * (AreaWeapon_DayAreas): up to two models of its part kind
 * (AreaWeapon_*Models). A mounted gun turns by Obj.timer and tilts its barrel
 * (the second model, 100 up and 75 ahead) by the aim pitch, up to 45 degrees
 * up or 35 down; a great weapon scrolls its texture (fortress), spins about
 * its length (castle) or rolls while it thrusts (battle town, whose two
 * models sit at their own points moved by the thrust kept in Obj.pos); the
 * castle's parts turn by their yaw.
 */
void AreaWeapon_Draw(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    u8 weaponSet = obj->type;
    u16 day = AreaWeapon_DayAreas[weaponSet];
    u8 partKind;
    f32 *modelPosY;
    f32 *modelPosZ;
    ModelMatSet *mdl;
    s16 *modelId;
    s16 j;
    s32 modelOfs;
    s32 drawKind;
    f32 yaw;
    u16 pitchTurn;
    f32 turn;
    u8 *posSrc;
    f32 modelPos[3];
    f32 barrelOfs[3];
    f32 mtx[16];
    f32 uvMtx[16];

    if (day == Area_DayId(gScene.area) && stage != NULL && stage->active != 0) {
        partKind = AreaWeapon_PartKinds[weaponSet][obj->var2];
        modelId = AreaWeapon_Models[partKind] + obj->type * 2;
        switch (obj->type) {
        case 2:
            yaw = 6.2831855f * AreaWeapon_YawCastle1[obj->var2] / 65536.0f;
            break;
        default:
            yaw = 0.0f;
            break;
        }
        drawKind = partKind;
        for (j = 0, modelOfs = 0, modelPosY = &modelPos[1], modelPosZ = &modelPos[2]; j < 2; j++, modelOfs += 0xC, modelId++) {
            if (*modelId == -1) {
                break;
            }
            if (drawKind == 1) {
                Vec_Copy((f32 *)modelPos, (f32 *)drawSlot->pos);
                if (j == 1) {
                    barrelOfs[0] = 0.0f;
                    barrelOfs[1] = 100.0f;
                    barrelOfs[2] = 75.0f;
                    Vec_RotateY(barrelOfs, 6.2831855f * obj->timer / 65536.0f);
                    modelPos[0] += barrelOfs[0];
                    *modelPosY += barrelOfs[1];
                    *modelPosZ += barrelOfs[2];
                }
            } else if (obj->type == 8) {
                posSrc = ((u8 **) AreaWeapon_PartPos[obj->type])[obj->var2];
                posSrc += modelOfs;
                Vec_Copy((f32 *)modelPos, (f32 *)posSrc);
                *modelPosZ += obj->pos[2];
            } else {
                Vec_Copy((f32 *)modelPos, (f32 *)drawSlot->pos);
            }
            Mtx_Translation(mtx, modelPos[0], *modelPosY, *modelPosZ);
            switch (drawKind) {
            case 2:
                switch (obj->type) {
                case 1:
                case 6:
                    Mtx_Identity(uvMtx);
                    Mtx_SetTrans(uvMtx, 0.0f, 1.0f - (obj->var1 & 7) / 8.0f, 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                    break;
                default:
                case 2:
                    Mtx_RotateZ(mtx, 6.2831855f * (obj->var1 << 13) / 65536.0f);
                    break;
                case 8:
                    if (obj->var0 == 2) {
                        Mtx_RotateZ(mtx, 6.2831855f * (360.0f * (obj->timer / 4.0f)) / 360.0f);
                    }
                    break;
                }
                Mtx_RotateY(mtx, yaw);
                break;
            case 1:
                if (j == 1) {
                    if (obj->var3 >= 0) {
                        turn = 65536.0f;
                        pitchTurn = (s16) (u16) (s32) (0.5f + turn * (0.01f * (45.0f * obj->var3)) / 360.0f);
                    } else {
                        turn = 65536.0f;
                        pitchTurn = (s16) (u16) (s32) (0.5f + turn * (0.01f * (35.0f * obj->var3)) / 360.0f);
                    }
                    Mtx_RotateX(mtx, 6.2831855f * -pitchTurn / turn);
                }
                Mtx_RotateY(mtx, 6.2831855f * obj->timer / 65536.0f);
                break;
            default:
                Mtx_RotateY(mtx, yaw);
                break;
            }
            Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
            mdl = stage->matSets + *modelId;
            if (mdl != NULL && mdl->id != -1) {
                Draw_ApplyStates(mdl->flags);
                Model_Draw(mdl->id, 0);
            }
        }
        Draw_SetDefaultState();
    }
}

/*
 * Returns the part's index among its area's mounted guns, which picks its
 * turning sound in AreaWeapon_GunTurnSounds: the part index less 2 in the
 * fortress's area 5 (found by its day area), else the part index.
 */
s32 AreaWeapon_GunIndex(Obj *obj)
{
    s32 dayArea;
    u8 firstGun;

    dayArea = Area_DayId(gScene.area) & 0xFFFF;
    switch (dayArea) {
    case 12:
        firstGun = 2;
        break;
    case 25:
        firstGun = 0;
        break;
    default:
        firstGun = 0;
        break;
    }
    return (obj->var2 - firstGun) & 0xFF;
}

/*
 * Makes the area's AnimatedProps object; Area_StartObjects calls it in the
 * forest's area 11, the snowy mountains' areas 6 and night 5, 6 and 8, and
 * the battle town's area 0.
 */
void AnimatedProps_Create(void)
{
    Obj *obj = Obj_Alloc(0x1);

    if (obj != NULL) {
        obj->kind = 0x7;
        obj->type = 0;
        obj->update = AnimatedProps_Update;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * AnimatedProps_Setup, _Run, _End and _Free.
 */
void AnimatedProps_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AnimatedProps_Setup(obj);
        break;
    case 1:
        AnimatedProps_Run(obj);
        break;
    case 2:
        AnimatedProps_End(obj);
        break;
    case 3:
        AnimatedProps_Free(obj);
        break;
    }
}

/*
 * Returns ifState0 when weather is 0 (or any other value), ifState1 when it
 * is 1 and ifState2 when it is 2 (Area_MonsterState: 2 while a Kushala Daora
 * is in the area, else 0 or 1 by the quest time).
 */
static s32 AnimatedProps_PickByWeather(s16 weather, s32 ifState0, s32 ifState1, s32 ifState2)
{
    switch (weather) {
    default:
        return ifState0;
    case 1:
        return ifState1;
    case 2:
        return ifState2;
    }
}

/*
 * Sets the props up for the current area from its position table: per prop a
 * state (how AnimatedProps_Run moves it) and a level (the alpha it draws
 * with: by the weather, by the set-down delivery, or full), and draw slots
 * for up to slotMax props, skipping those another prop draws with its own. In
 * another area, or without the first slot, it frees itself; else its first
 * frame runs. The object comes in as a void * and is viewed as an Obj, as the
 * game's code does.
 */
void AnimatedProps_Setup(void *objArg)
{
    Obj *obj = (Obj *) objArg;
    s16 weather;
    s16 slotCount;
    s16 propCount;
    s16 slotMax;
    f32 *pos;
    DrawSlot *drawSlot;
    s16 i;
    u8 skip;
    AnimatedPropsBuf *props = obj->buf;

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    obj->var3 = 0;
    slotCount = 0;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_11:
    case AREA_FOREST_HILLS_NIGHT_11:
        propCount = slotMax = 8;
        pos = AnimatedProps_PosForestHills11;
        break;
    case AREA_SNOWY_MOUNTAINS_6:
        propCount = 6;
        slotMax = 2;
        pos = AnimatedProps_PosSnowyMountains6;
        weather = Area_MonsterState(gScene.area);
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
        slotMax = 1;
        propCount = 4;
        pos = AnimatedProps_PosSnowyNight5;
        weather = Area_MonsterState(gScene.area);
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        propCount = 7;
        slotMax = 2;
        pos = AnimatedProps_PosSnowyNight6;
        weather = Area_MonsterState(gScene.area);
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        slotMax = 1;
        propCount = 3;
        pos = AnimatedProps_PosSnowyNight8;
        weather = Area_MonsterState(gScene.area);
        break;
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        propCount = 13;
        slotMax = propCount;
        pos = AnimatedProps_PosBattleTown0;
        weather = Area_MonsterState(gScene.area);
        break;
    default:
        AnimatedProps_Free(obj);
        return;
    }
    for (i = 0; i < 13; i++, pos += 3) {
        if (i < propCount) {
            skip = 0;
            switch (gScene.area) {
            case AREA_SNOWY_MOUNTAINS_6:
                switch (i) {
                case 0:
                    props->state[i] = 1;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 0, 0x80, 0xFF);
                    break;
                case 5:
                    props->state[i] = 2;
                    if (QuestGoal_DeliveryDone() == 0) {
                        props->level[i] = 0;
                    } else {
                        props->level[i] = 0xFF;
                    }
                    break;
                default:
                    props->state[i] = 0;
                    skip = 1;
                    props->level[i] = 0xFF;
                    break;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_NIGHT_5:
                switch (i) {
                case 0:
                    props->state[i] = 0;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 0xFF, 0x80, 0);
                    break;
                case 1:
                    skip = 1;
                    props->state[i] = 3;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 0xFF, 0x80, 0);
                    break;
                default:
                    props->state[i] = 1;
                    skip = 1;
                    props->level[i] = 0xFF;
                    break;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_NIGHT_6:
                switch (i) {
                case 0:
                    props->state[i] = 0;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 0xFF, 0x80, 0);
                    break;
                case 1:
                    skip = 1;
                    props->state[i] = 3;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 0xFF, 0x80, 0);
                    break;
                case 6:
                    props->state[i] = 2;
                    props->level[i] = 0;
                    break;
                default:
                    props->state[i] = 1;
                    skip = 1;
                    props->level[i] = 0xFF;
                    break;
                }
                break;
            case AREA_SNOWY_MOUNTAINS_NIGHT_8:
                switch (i) {
                case 0:
                    props->state[i] = 0;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 0xFF, 0x80, 0);
                    break;
                case 1:
                    skip = 1;
                    props->state[i] = 3;
                    props->level[i] = AnimatedProps_PickByWeather(weather, 0xFF, 0x80, 0);
                    break;
                default:
                    props->state[i] = 1;
                    skip = 1;
                    props->level[i] = 0xFF;
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
                    props->state[i] = 0;
                    break;
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                    props->state[i] = 1;
                    break;
                default:
                    props->state[i] = 2;
                    break;
                }
                break;
            }
            props->rot[i] = 0;
            if (slotCount < slotMax) {
                if (skip == 0) {
                props->slot[i] = DrawSlot_Alloc();
                if (props->slot[i] != -1) {
                    drawSlot = DrawSlot_Get(props->slot[i]);
                    props->ds[i] = drawSlot;
                    drawSlot->owner = obj;
                    drawSlot->part = i;
                    drawSlot->draw = AnimatedProps_Draw;
                    slotCount++;
                    drawSlot->pos[0] = pos[0];
                    drawSlot->pos[1] = pos[1];
                    drawSlot->pos[2] = pos[2];
                } else {
                    props->ds[i] = NULL;
                    if (slotCount == 0) {
                        AnimatedProps_Free(obj);
                        return;
                    }
                }
                } else {
                    props->ds[i] = NULL;
                }
            } else {
                props->ds[i] = NULL;
            }
        } else {
            props->ds[i] = NULL;
        }
    }
    AnimatedProps_Run(objArg);
}

/*
 * Every frame, by area: in the snowy mountains eases each weather-driven
 * prop's level by 8 toward its weather's level (AnimatedProps_PickByWeather),
 * turns the spinning props (AnimatedProps_Spin*) and brings the delivery prop
 * in by 8 once a set-down delivery is done; in the battle town's area 0
 * pulses the glows between 0xCF and 0xFF. Then adds each prop that has a draw
 * slot to the 8-entry list or the sorted list.
 */
void AnimatedProps_Run(Obj *obj)
{
    AreaState *areaState = &gAreaState;
    s16 i;
    AnimatedPropsBuf *props = obj->buf;
    u16 mode;
    u8 goal;
    u8 *levelPtr;

    obj->timer++;
    for (i = 0; i < 13; i++) {
        mode = 0;
        switch (gScene.area) {
        case AREA_SNOWY_MOUNTAINS_6:
            switch (props->state[i]) {
            default:
                props->rot[i] += AnimatedProps_SpinSnowyMountains6[i];
                mode = 3;
                break;
            case 1:
                goal = AnimatedProps_PickByWeather(Area_MonsterState(gScene.area), 0, 0x80, 0xFF);
                levelPtr = &props->level[i];
                if (goal < *levelPtr) {
                    if (goal + 8 < *levelPtr) {
                        *levelPtr = *levelPtr - 8;
                    } else {
                        *levelPtr = goal;
                    }
                } else if (*levelPtr < goal - 8) {
                    *levelPtr = *levelPtr + 8;
                } else {
                    *levelPtr = goal;
                }
                mode = 3;
                break;
            case 2:
                if (QuestGoal_DeliveryDone() == 0) {
                    props->level[i] = 0;
                } else if (props->level[i] < 0xF7) {
                    props->level[i] += 8;
                } else {
                    props->level[i] = 0xFF;
                }
                mode = 0;
                break;
            }
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_5:
            switch (props->state[i]) {
            case 0:
            case 3:
                goal = AnimatedProps_PickByWeather(Area_MonsterState(gScene.area), 0xFF, 0x80, 0);
                levelPtr = &props->level[i];
                if (goal < *levelPtr) {
                    if (goal + 8 < *levelPtr) {
                        *levelPtr = *levelPtr - 8;
                    } else {
                        *levelPtr = goal;
                    }
                } else if (*levelPtr < goal - 8) {
                    *levelPtr = *levelPtr + 8;
                } else {
                    *levelPtr = goal;
                }
                mode = 3;
                break;
            }
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_6:
            switch (props->state[i]) {
            case 0:
            case 3:
                goal = AnimatedProps_PickByWeather(Area_MonsterState(gScene.area), 0xFF, 0x80, 0);
                levelPtr = &props->level[i];
                if (goal < *levelPtr) {
                    if (goal + 8 < *levelPtr) {
                        *levelPtr = *levelPtr - 8;
                    } else {
                        *levelPtr = goal;
                    }
                } else if (*levelPtr < goal - 8) {
                    *levelPtr = *levelPtr + 8;
                } else {
                    *levelPtr = goal;
                }
            default:
            case 1:
                props->rot[i] += AnimatedProps_SpinSnowyNight6[i];
                mode = 3;
                break;
            case 2:
                if (QuestGoal_DeliveryDone() == 0) {
                    props->level[i] = 0;
                } else if (props->level[i] < 0xF7) {
                    props->level[i] += 8;
                } else {
                    props->level[i] = 0xFF;
                }
                mode = 0;
                break;
            }
            break;
        case AREA_SNOWY_MOUNTAINS_NIGHT_8:
            switch (props->state[i]) {
            case 0:
            case 3:
                goal = AnimatedProps_PickByWeather(Area_MonsterState(gScene.area), 0xFF, 0x80, 0);
                levelPtr = &props->level[i];
                if (goal < *levelPtr) {
                    if (goal + 8 < *levelPtr) {
                        *levelPtr = *levelPtr - 8;
                    } else {
                        *levelPtr = goal;
                    }
                } else if (*levelPtr < goal - 8) {
                    *levelPtr = *levelPtr + 8;
                } else {
                    *levelPtr = goal;
                }
                mode = 3;
                break;
            }
            break;
        case AREA_BATTLE_TOWN_0:
        case AREA_BATTLE_TOWN_NIGHT_0:
            switch (props->state[i]) {
            case 1:
                props->level[i] = (u8) (24.0f * (1.0f + Math_Sin(6.2831855f * (areaState->frame << 12) / 65536.0f))) + 0xCF;
                mode = 0;
                break;
            }
            break;
        }
        if (props->ds[i] != NULL) {
            switch (mode) {
            default:
                OT_AddByDepth(gDrawOtSorted, props->ds[i], 0x40, 0);
                break;
            case 3:
                OT_AddByDepth(gDrawOt8, props->ds[i], 8, 1);
                break;
            }
        }
    }
}

/* Hides the object; it is freed on the next update. */
void AnimatedProps_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void AnimatedProps_Free(Obj *obj)
{
    Obj_Free(obj);
}

/* The props' buffer as AnimatedProps_Draw names it: the AnimatedPropsBuf layout, kind being a
 * prop's state, alpha its level and ang its turn. */
typedef struct AnimatedPropsDrawView {
    u8 kind[13];                /* 0x00 */
    u8 alpha[13];               /* 0x0D */
    u16 ang[8];                 /* 0x1A */
} AnimatedPropsDrawView;

void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
extern f32 AnimatedProps_ScalesForestHills11[];
extern s16 AnimatedProps_ModelsSnowyMountains6[];
extern s16 AnimatedProps_ModelsSnowyNight5[];
extern s16 AnimatedProps_ModelsSnowyNight6[];
extern s16 AnimatedProps_ModelsSnowyNight8[];
extern s16 AnimatedProps_ModelsBattleTown0[];

/*
 * Draws a prop by area, with no alpha test: in the forest's area 11 a camera-
 * yaw billboard stepping a 4 by 4 texture flipbook; in the snowy areas all
 * the area's props together, turned by their spin or scrolled and faded by
 * their level, or the delivery prop as a camera-yaw billboard scrolling up
 * without depth writes; in the battle town's area 0 camera-facing flames on a
 * 4 by 4 flipbook and glows faded by their level, from the area model
 * (gAreaState.model) with its lights.
 */
void AnimatedProps_Draw(DrawSlot *drawSlot)
{
    Model *stage = gAreaSubModels[0];
    Obj *obj = drawSlot->owner;
    AnimatedPropsDrawView *props = obj->buf;
    ModelMatSet *mdl;
    AreaState *areaState = &gAreaState;
    s16 i;
    f32 scrollU;
    f32 mtx[16];
    f32 uvMtx[16];

    switch (gScene.area) {
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        stage = areaState->model;
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
        Mtx_Translation(uvMtx, 0.25f * ((obj->timer >> 1) & 3), 0.25f * ((obj->timer >> 3) & 3), 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Mtx_Scale(mtx, AnimatedProps_ScalesForestHills11[drawSlot->part * 3], AnimatedProps_ScalesForestHills11[drawSlot->part * 3 + 1], AnimatedProps_ScalesForestHills11[drawSlot->part * 3 + 2]);
        Mtx_Concat3x3(mtx, gCamYawMtx);
        goto draw;
    case AREA_SNOWY_MOUNTAINS_6:
        Gs_SetState(RS_ALPHA_REF, 0);
        switch (props->kind[drawSlot->part]) {
        case 0:
        case 1:
            for (i = 0; i < 5; i++) {
                mdl = stage->matSets + AnimatedProps_ModelsSnowyMountains6[i];
                Mtx_Translation(mtx, AnimatedProps_PosSnowyMountains6[i * 3], AnimatedProps_PosSnowyMountains6[i * 3 + 1], AnimatedProps_PosSnowyMountains6[i * 3 + 2]);
                if (props->kind[i] != 1) {
                    Mtx_RotateY(mtx, 6.2831855f * props->ang[i] / 65536.0f);
                } else {
                    Mtx_Translation(uvMtx, (obj->timer & 0x1FF) / 512.0f, 0.0f, 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                    Gs_SetState(RS_COLOR2, (props->alpha[i] << 24) | 0xFFFFFF);
                }
                Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
                if (mdl != NULL && mdl->id != -1) {
                    Draw_ApplyStates(mdl->flags);
                    Model_Draw(mdl->id, 0);
                }
            }
            Draw_SetDefaultState();
            return;
        case 2:
        default:
            Gs_SetState(RS_ZWRITE, 0);
            mdl = stage->matSets + AnimatedProps_ModelsSnowyMountains6[drawSlot->part];
            Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            Mtx_Concat3x3(mtx, gCamYawMtx);
            Gs_SetState(RS_COLOR2, (props->alpha[drawSlot->part] << 24) | 0xFFFFFF);
            Mtx_Translation(uvMtx, 0.0f, (obj->timer & 0xFF) / 256.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            goto draw;
        }
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_5:
        Gs_SetState(RS_ALPHA_REF, 0);
        for (i = 0; i < 4; i++) {
            mdl = stage->matSets + AnimatedProps_ModelsSnowyNight5[i];
            Mtx_Translation(mtx, AnimatedProps_PosSnowyNight5[i * 3], AnimatedProps_PosSnowyNight5[i * 3 + 1], AnimatedProps_PosSnowyNight5[i * 3 + 2]);
            switch (props->kind[i]) {
            case 3:
                Mtx_Translation(uvMtx, (obj->timer & 0x3FF) / 1024.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            case 0:
                Gs_SetState(RS_COLOR2, (props->alpha[i] << 24) | 0xFFFFFF);
                break;
            case 1:
            default:
                if (i == 2) {
                    scrollU = (obj->timer & 0xFF) / 256.0f;
                } else {
                    scrollU = (obj->timer & 0x3F) / 64.0f;
                }
                Mtx_Translation(uvMtx, scrollU, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            }
            Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
            if (mdl != NULL && mdl->id != -1) {
                Draw_ApplyStates(mdl->flags);
                Model_Draw(mdl->id, 0);
            }
        }
        Draw_SetDefaultState();
        return;
    case AREA_SNOWY_MOUNTAINS_NIGHT_6:
        Gs_SetState(RS_ALPHA_REF, 0);
        switch (props->kind[drawSlot->part]) {
        case 0:
        default:
        case 1:
        case 3:
            for (i = 0; i < 6; i++) {
                mdl = stage->matSets + AnimatedProps_ModelsSnowyNight6[i];
                Mtx_Translation(mtx, AnimatedProps_PosSnowyNight6[i * 3], AnimatedProps_PosSnowyNight6[i * 3 + 1], AnimatedProps_PosSnowyNight6[i * 3 + 2]);
                switch (props->kind[i]) {
                case 3:
                    Mtx_Translation(uvMtx, (obj->timer & 0x3FF) / 1024.0f, 0.0f, 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                case 0:
                    Gs_SetState(RS_COLOR2, (props->alpha[i] << 24) | 0xFFFFFF);
                    break;
                case 1:
                default:
                    Mtx_RotateY(mtx, 6.2831855f * props->ang[i] / 65536.0f);
                    break;
                }
                Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
                if (mdl != NULL && mdl->id != -1) {
                    Draw_ApplyStates(mdl->flags);
                    Model_Draw(mdl->id, 0);
                }
            }
            Draw_SetDefaultState();
            return;
        case 2:
            Gs_SetState(RS_ZWRITE, 0);
            mdl = stage->matSets + AnimatedProps_ModelsSnowyNight6[drawSlot->part];
            Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            Mtx_Concat3x3(mtx, gCamYawMtx);
            Gs_SetState(RS_COLOR2, (props->alpha[drawSlot->part] << 24) | 0xFFFFFF);
            Mtx_Translation(uvMtx, 0.0f, (obj->timer & 0xFF) / 256.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            goto draw;
        }
        break;
    case AREA_SNOWY_MOUNTAINS_NIGHT_8:
        Gs_SetState(RS_ALPHA_REF, 0);
        for (i = 0; i < 3; i++) {
            mdl = stage->matSets + AnimatedProps_ModelsSnowyNight8[i];
            Mtx_Translation(mtx, AnimatedProps_PosSnowyNight8[i * 3], AnimatedProps_PosSnowyNight8[i * 3 + 1], AnimatedProps_PosSnowyNight8[i * 3 + 2]);
            switch (props->kind[i]) {
            case 3:
                Mtx_Translation(uvMtx, (obj->timer & 0x3FF) / 1024.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            case 0:
            default:
                Gs_SetState(RS_COLOR2, (props->alpha[i] << 24) | 0xFFFFFF);
                break;
            case 1:
                Mtx_Translation(uvMtx, (obj->timer & 0x7F) / 128.0f, 0.0f, 0.0f);
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                break;
            }
            Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
            if (mdl != NULL && mdl->id != -1) {
                Draw_ApplyStates(mdl->flags);
                Model_Draw(mdl->id, 0);
            }
        }
        Draw_SetDefaultState();
        return;
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        mdl = stage->matSets + AnimatedProps_ModelsBattleTown0[drawSlot->part];
        Gs_SetLightList(stage->mats, mdl);
        Mtx_Translation(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Gs_SetState(RS_ZWRITE, 0);
        switch (AnimatedProps_ModelsBattleTown0[drawSlot->part]) {
        case 13:
        case 15:
        default:
            Mtx_Translation(uvMtx, 0.25f * (areaState->frame & 3), 0.25f * ((areaState->frame >> 2) & 3), 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            Gs_SetState(RS_COLOR2, -1);
            break;
        case 14:
            Gs_SetState(RS_COLOR2, (props->alpha[drawSlot->part] << 24) | 0xFFFFFF);
            break;
        }
        goto draw;
    }
    return;
draw:
    Gs_SetState(RS_ALPHA_REF, 0);
    Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
    if (mdl != NULL && mdl->id != -1) {
        Draw_ApplyStates(mdl->flags);
        Model_Draw(mdl->id, 0);
    }
    Draw_SetDefaultState();
    Gs_SetState(RS_ZWRITE, 1);
    return;
}

/*
 * Makes the area's AnimatedGroundTiles object; Area_StartObjects calls it in
 * the swamp's area 5, day and night.
 */
void AnimatedGroundTiles_Create(void)
{
    Obj *obj = Obj_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x8;
        obj->type = 0;
        obj->update = AnimatedGroundTiles_Update;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * AnimatedGroundTiles_Setup, _Run, _End and _Free.
 */
void AnimatedGroundTiles_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AnimatedGroundTiles_Setup(obj);
        break;
    case 1:
        AnimatedGroundTiles_Run(obj);
        break;
    case 2:
        AnimatedGroundTiles_End(obj);
        break;
    case 3:
        AnimatedGroundTiles_Free(obj);
        break;
    }
}

/*
 * Sets the tiles up at the origin with a draw slot
 * (AnimatedGroundTiles_Draw), or frees itself without one; then the first
 * frame runs.
 */
void AnimatedGroundTiles_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->var3 = 0;
    obj->pos[0] = 0.0f;
    obj->pos[1] = 0.0f;
    obj->pos[2] = 0.0f;
    obj->timer = 0;
    obj->draw = NULL;
    obj->ds = DrawSlot_Get(DrawSlot_Alloc());
    if (obj->ds != NULL) {
        obj->ds->owner = obj;
        obj->ds->draw = AnimatedGroundTiles_Draw;
        AnimatedGroundTiles_Run(obj);
    } else {
        Obj_Free(obj);
    }
}

/*
 * Counts frames and follows the local player in x and z at height 0, adding
 * the tiles to the 32-entry draw list.
 */
void AnimatedGroundTiles_Run(Obj *obj)
{
    Player *player;

    player = &gPlayers[gScene.localPlayer];
    obj->timer++;
    obj->pos[0] = player->pos[0];
    obj->pos[1] = 0.0f;
    obj->pos[2] = player->pos[2];
    if (obj->draw == NULL) {
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 1);
    }
}

/* Hides the object; it is freed on the next update. */
void AnimatedGroundTiles_End(Obj *obj)
{
    obj->state++;
    obj->visible = 0;
}

/* Frees the object (Obj_Free). */
void AnimatedGroundTiles_Free(Obj *obj)
{
    Obj_Free(obj);
}

/*
 * Draws two layers, the area's first two models, on a 3 by 3 grid of
 * 1000-unit cells around the object, with alpha test reference 0x80 and
 * Gfx_SetTexFilter(1); each layer steps through a 4 by 4 texture atlas every
 * 2 frames (AnimatedGroundTiles_UvSteps, 16 steps).
 */
void AnimatedGroundTiles_Draw(DrawSlot *drawSlot)
{
    Obj *obj = drawSlot->owner;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *mdl;
    f32 mtx[16];
    f32 uvMtx[16];
    f32 uvU;
    f32 cellX;
    f32 firstCellZ;
    f32 uvV;
    f32 z;
    s32 layer;
    s32 i;
    s32 j;

    if (stage != NULL && stage->active != 0) {
        mdl = stage->matSets;
        Gs_SetState(RS_ALPHA_REF, 0x80);
        obj->fvar = 1000.0f;
        for (layer = 0; layer < 2; mdl++, layer++) {
            uvU = AnimatedGroundTiles_UvSteps[((obj->timer & 0x1E) >> 1) * 4 + layer * 2];
            uvV = AnimatedGroundTiles_UvSteps[((obj->timer & 0x1E) >> 1) * 4 + layer * 2 + 1];
            uvU -= Math_Floor(uvU);
            uvV -= Math_Floor(uvV);
            Mtx_Translation(uvMtx, uvU, uvV, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            cellX = 1000.0f * Math_Floor((500.0f + obj->pos[0]) / 1000.0f) - 1000.0f;
            firstCellZ = 1000.0f * Math_Floor((500.0f + obj->pos[2]) / 1000.0f) - 1000.0f;
            for (i = 0; i < 3; i++, cellX += obj->fvar) {
                for (j = 0, z = firstCellZ; j < 3; j++, z += obj->fvar) {
                    Mtx_Translation(mtx, cellX, 0.0f, z);
                    Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
                    if (mdl->id != -1) {
                        Draw_ApplyStates(mdl->flags);
                        Gfx_SetTexFilter(1);
                        Model_Draw(mdl->id, 0);
                    }
                }
            }
        }
        Gfx_SetTexFilter(0);
        Gs_SetState(RS_ALPHA_REF, 0);
        Draw_SetDefaultState();
    }
}

/*
 * Makes the area's AmbientLife spawner (type 0); Area_StartObjects calls it
 * for an area with an ambient set and passes the set, which is unused.
 */
void AmbientLife_Create(u8 unusedSet)
{
    Obj *obj = Obj_Alloc(0x1);

    if (obj != NULL) {
        obj->kind = 0x9;
        obj->type = 0;
        obj->var4 = 0;
        obj->update = AmbientLife_Update;
    }
}

/*
 * Allocates an AmbientLife object of type childType: Obj_Alloc(0) for types
 * 8, 10 and 11, Obj_Alloc(1) for the others.
 */
void *AmbientLife_Alloc(s16 childType)
{
    switch (childType) {
    case 8:
    case 0xA:
    case 0xB:
        return Obj_Alloc(0);
    default:
        return Obj_Alloc(1);
    }
}

/*
 * Makes a child of type childType for the spawner parent in the first free
 * entry of its list of 32, with index in Obj.var4, and counts it (per type,
 * and all of them in count[0]); nothing when the list is full or no object is
 * free.
 */
void AmbientLife_SpawnChild(Obj *parent, s16 childType, u8 index)
{
    AmbientLifeChildren *children = parent->buf;
    s16 freeIdx;
    Obj *child;

    freeIdx = AmbientLife_FindFreeChild(parent);
    if (freeIdx != -1) {
        child = AmbientLife_Alloc(childType);
        if (child != NULL) {
            child->kind = 9;
            child->type = childType;
            child->update = AmbientLife_Update;
            child->var4 = index;
            children->obj[freeIdx] = child;
            children->count[childType]++;
            children->count[0]++;
        }
    }
}

/*
 * As AmbientLife_SpawnChild, but the third argument goes to Obj.var3
 * (Obj.var4 is 0): the number of points a type 11 child moves between.
 */
void AmbientLife_SpawnChildWithPoints(Obj *parent, s16 childType, s16 pointCount)
{
    AmbientLifeChildren *children = parent->buf;
    s16 freeIdx;
    Obj *child;

    freeIdx = AmbientLife_FindFreeChild(parent);
    if (freeIdx != -1) {
        child = AmbientLife_Alloc(childType);
        if (child != NULL) {
            child->kind = 9;
            child->type = childType;
            child->update = AmbientLife_Update;
            child->var3 = pointCount;
            child->var4 = 0;
            children->obj[freeIdx] = child;
            children->count[childType]++;
            children->count[0]++;
        }
    }
}

/*
 * Returns the first free entry of the spawner parent's list of 32 children,
 * or -1.
 */
s16 AmbientLife_FindFreeChild(Obj *parent)
{
    Obj **entry = parent->buf;
    s16 i;

    for (i = 0; i < 32; i++) {
        if (*entry == NULL) {
            return i;
        }
        entry++;
    }
    return -1;
}

/*
 * Makes an AmbientLife object of type childType at spot with no spawner
 * (Obj.var4 0xFF); Area_Update makes type 8 this way at the bug-net gathering
 * points, offline, every 128 frames.
 */
void AmbientLife_CreateAt(void *spot, s16 childType)
{
    Obj *obj = AmbientLife_Alloc(childType);

    if (obj != NULL) {
        obj->kind = 9;
        obj->type = childType;
        obj->update = AmbientLife_Update;
        Vec_Copy((f32 *)obj->pos, (f32 *)spot);
        obj->var4 = 0xFF;
    }
}

/*
 * The object's update, run every frame: switches on Obj.state to
 * AmbientLife_Setup, _Run, _End and _Free.
 */
void AmbientLife_Update(Obj *obj)
{
    switch (obj->state) {
    case 0:
        AmbientLife_Setup(obj);
        break;
    case 1:
        AmbientLife_Run(obj);
        break;
    case 2:
        AmbientLife_End(obj);
        break;
    case 3:
        AmbientLife_Free(obj);
        break;
    }
}

/*
 * Sets an object up by its type (AmbientLife_Setup*); every type but the
 * spawner and the butterfly then takes a draw slot (AmbientLife_Draw), or
 * frees itself without one.
 */
void AmbientLife_Setup(Obj *obj)
{

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    switch (obj->type) {
    case 0:
        AmbientLife_SetupSpawner(obj);
        break;
    case 1:
        AmbientLife_SetupFallingLeaf(obj);
        break;
    case 2:
        AmbientLife_SetupWanderer(obj);
        break;
    case 3:
        AmbientLife_SetupDarter(obj);
        break;
    case 4:
        AmbientLife_SetupSteadyFlyer(obj);
        break;
    case 5:
        AmbientLife_SetupButterfly(obj);
        break;
    case 6:
        AmbientLife_SetupSpinner(obj);
        break;
    case 7:
        AmbientLife_SetupCirclingBird(obj);
        break;
    case 8:
        AmbientLife_SetupGlowBug(obj);
        break;
    case 9:
        AmbientLife_SetupUnused();
        break;
    case 10:
        AmbientLife_SetupUnusedNext();
        break;
    case 11:
        AmbientLife_SetupCrossingFlyer(obj);
        break;
    }
    if (obj->type == 0 || obj->type == 5) {
        return;
    }
    obj->slot = DrawSlot_Alloc();
    if (obj->slot != -1) {
        obj->ds = DrawSlot_Get(obj->slot);
        if (obj->ds != NULL) {
            obj->ds->owner = obj;
            obj->ds->draw = AmbientLife_Draw;
        }
    } else {
        AmbientLife_Free(obj);
    }
}

/* Runs the object by its type. */
void AmbientLife_Run(Obj *obj)
{
    switch (obj->type) {
    case 0:
        AmbientLife_RunSpawner(obj);
        return;
    case 1:
        AmbientLife_RunFallingLeaf(obj);
        return;
    case 2:
        AmbientLife_RunWanderer(obj);
        return;
    case 3:
        AmbientLife_RunDarter(obj);
        return;
    case 4:
        AmbientLife_RunSteadyFlyer(obj);
        return;
    case 5:
        AmbientLife_RunButterfly(obj);
        return;
    case 6:
        AmbientLife_RunSpinner(obj);
        return;
    case 7:
        AmbientLife_RunCirclingFlyer(obj);
        return;
    case 8:
        AmbientLife_RunGlowBug(obj);
        return;
    case 9:
        AmbientLife_RunUnused();
        return;
    case 10:
        AmbientLife_RunUnusedNext();
        return;
    case 11:
        AmbientLife_RunCrossingFlyer(obj);
        
    default:
        return;
    }
}

typedef struct ButterflySlotsView { s16 id[2]; u8 unk4[0x14 - 4]; s32 h[2]; } ButterflySlotsView;

/*
 * Hides the object and frees its draw slot, or a butterfly's two (read
 * through ButterflySlotsView); it is freed on the next update.
 */
void AmbientLife_End(Obj *obj)
{
    ButterflySlotsView *slots = obj->buf;
    s16 i;

    obj->state++;
    obj->visible = 0;
    if (obj->type != 5) {
        if (obj->ds != NULL) {
            DrawSlot_Free(obj->slot);
        }
    } else {
        for (i = 0; i < 2; i++) {
            if (slots->h[i] != 0) {
                DrawSlot_Free(slots->id[i]);
            }
        }
    }
}

/* Frees the object (Obj_Free). */
void AmbientLife_Free(Obj *obj)
{
    Obj_Free(obj);
}

/* Draws the object by its type, read as a byte of the owner (Obj.type). */
void AmbientLife_Draw(DrawSlot *drawSlot)
{
    u8 type;

    type = ((u8 *) drawSlot->owner)[3];
    switch (type) {
    case 1:
        AmbientLife_DrawFallingLeaf(drawSlot);
        break;
    case 2:
        AmbientLife_DrawWanderer(drawSlot);
        break;
    case 3:
        AmbientLife_DrawDarter(drawSlot);
        break;
    case 4:
        AmbientLife_DrawSteadyFlyer(drawSlot);
        break;
    case 5:
        AmbientLife_DrawButterfly(drawSlot);
        break;
    case 6:
        AmbientLife_DrawSpinner(drawSlot);
        break;
    case 7:
        AmbientLife_DrawCirclingFlyer(drawSlot);
        break;
    case 8:
        AmbientLife_DrawGlowBug(drawSlot);
        break;
    case 9:
        AmbientLife_DrawUnused();
        break;
    case 10:
        AmbientLife_DrawUnusedNext();
        break;
    case 11:
        AmbientLife_DrawCrossingFlyer(drawSlot);
        
    case 0x0:
    default:
        break;
    }
}

/*
 * Draws model set with the matrix at address mtx (render slot 0) when it has
 * a model, then restores the default draw state.
 */
void AmbientLife_DrawModel(ModelMatSet *set, s32 mtx)
{
    Gs_SetState(RS_MTX_SLOT(0), mtx);
    if ((set != NULL) && (set->id != -1)) {
        Draw_ApplyStates(set->flags);
        Model_Draw(set->id, 0);
    }
    Draw_SetDefaultState();
}

/*
 * Clears the spawner's list and counts, then makes the area's lasting
 * children: four circling birds in the forest's area 6, the great arena's
 * area 1 and the desert's areas 2, 8 and 9; a spinner in the forest's area 7;
 * ten type 8 children in the tower's area 8.
 */
void AmbientLife_SetupSpawner(Obj *spawner)
{
    AmbientLifeChildren *children = spawner->buf;
    s16 i;

    spawner->timer = 0;
    for (i = 0; i < 32; i++) {
        children->obj[i] = NULL;
    }
    children->count[0] = 0;
    for (i = 0; i < 11; i++) {
        children->count[i + 1] = 0;
    }
    switch (gScene.area) {
    case AREA_FOREST_HILLS_6:
    case AREA_GREAT_ARENA_1:
    case AREA_DESERT_2:
    case AREA_DESERT_8:
    case AREA_DESERT_9:
        if (children->count[0] < 32) {
            for (i = 0; i < 4; i++) {
                if (children->count[7] < 4) {
                    AmbientLife_SpawnChild(spawner, 7, i);
                }
            }
        }
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (children->count[0] < 32 && children->count[6] <= 0) {
            AmbientLife_SpawnChild(spawner, 6, 0);
        }
        break;
    case AREA_TOWER_8:
        if (children->count[0] < 32) {
            for (i = 0; i < 10; i++) {
                AmbientLife_SpawnChild(spawner, 8, i);
            }
        }
        break;
    }
}

/*
 * Tops the area's children up every frame: two butterflies in the snowy
 * mountains' area 9; one or two type 11 children every 256 frames in the
 * forest's area 3 (up to two); up to two type 8 children in its area 9 and
 * two steady flyers in its area 5; in its area 7 a falling leaf every 60
 * frames, three darters, five steady flyers and two butterflies. Then forgets
 * the children that ended (state 2 or more).
 */
void AmbientLife_RunSpawner(Obj *spawner)
{
    AmbientLifeChildren *children = spawner->buf;
    s16 i;
    s32 n;
    Obj *child;

    spawner->timer++;
    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_9:
        if (children->count[0] < 32 && children->count[5] < 2) {
            AmbientLife_SpawnChild(spawner, 5, 0);
        }
        break;
    case AREA_FOREST_HILLS_3:
    case AREA_FOREST_HILLS_NIGHT_3:
        if (*(u8 *) &spawner->timer == 0 && children->count[0] < 32 && children->count[11] < 2) {
            n = (s16) (Rand_Next(1) & 1) + 1;
            for (i = 0; i < n; i++) {
                AmbientLife_SpawnChildWithPoints(spawner, 11, 2);
                if (children->count[0] >= 32) {
                    break;
                }
            }
        }
        break;
    case AREA_FOREST_HILLS_9:
    case AREA_FOREST_HILLS_NIGHT_9:
        if (children->count[0] < 32 && children->count[8] < 2) {
            AmbientLife_SpawnChild(spawner, 8, 0);
        }
        break;
    case AREA_FOREST_HILLS_5:
    case AREA_FOREST_HILLS_NIGHT_5:
        if (children->count[0] < 32 && children->count[4] < 2) {
            AmbientLife_SpawnChild(spawner, 4, 0);
        }
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (spawner->timer >= 60 && children->count[0] < 32) {
            AmbientLife_SpawnChild(spawner, 1, 0);
            spawner->timer = 0;
        }
        if (children->count[0] < 32 && children->count[3] < 3) {
            AmbientLife_SpawnChild(spawner, 3, 0);
        }
        if (children->count[0] < 32 && children->count[4] < 5) {
            AmbientLife_SpawnChild(spawner, 4, 0);
        }
        if (children->count[0] < 32 && children->count[5] < 2) {
            AmbientLife_SpawnChild(spawner, 5, 0);
        }
        break;
    }
    for (i = 0; i < 32; i++) {
        child = children->obj[i];
        if (child != NULL && child->state >= 2) {
            children->count[child->type]--;
            children->obj[i] = NULL;
            children->count[0]--;
        }
    }
}

/*
 * A falling leaf: one of four texture variants (Obj.var0), a random place
 * 3000 up in the forest's area 7's box, a random orientation and no speed.
 */
void AmbientLife_SetupFallingLeaf(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;

    obj->var0 = Rand_Next(1) & 3;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        obj->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0x7FF);
        obj->pos[1] = 3000.0f;
        obj->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0x3FF);
        break;
    }
    obj->timer = 0;
    mover->v[0] = 0.0f;
    mover->v[1] = 0.0f;
    mover->v[2] = 0.0f;
    mover->rot[0] = Rand_Next(1);
    mover->rot[1] = Rand_Next(1);
    mover->rot[2] = Rand_Next(1);
    mover->spinX = 0;
    mover->spinZ = 0;
}

/*
 * Tumbles the leaf with damped random spins and lets it slide down along its
 * own tilt, moving it in its own frame; it ends below height 0.
 */
void AmbientLife_RunFallingLeaf(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    f32 step[3];

    mover->spinX = 0.95f * mover->spinX;
    mover->spinZ = 0.975f * mover->spinZ;
    mover->spinX += (s16) ((Rand_Next(1) & 0xFF) - 0x80);
    mover->spinZ += (s16) ((Rand_Next(1) & 0x3F) - 0x20);
    mover->rot[0] += mover->spinX;
    mover->rot[1] += (u16) ((Rand_Next(1) & 0x1F) - 0xF);
    mover->rot[2] += mover->spinZ;
    Mtx_Identity(mover->m);
    Mtx_RotateX(mover->m, 6.2831855f * mover->rot[0] / 65536.0f);
    Mtx_TurnY(mover->m, 6.2831855f * mover->rot[1] / 65536.0f);
    Mtx_TurnZ(mover->m, 6.2831855f * mover->rot[2] / 65536.0f);
    mover->v[0] += -2.0f * mover->m[1];
    mover->v[1] += -2.0f * mover->m[5];
    mover->v[2] += -2.0f * mover->m[9];
    mover->v[0] *= 0.6f;
    mover->v[1] *= 0.25f;
    mover->v[2] *= 0.7f;
    Vec_Rotate(step, mover->v, mover->m);
    obj->pos[0] += step[0];
    obj->pos[1] += step[1];
    obj->pos[2] += step[2];
    if (!(0.0f <= obj->pos[1])) {
        obj->state++;
    } else {
        obj->ds->pos[0] = obj->pos[0];
        obj->ds->pos[1] = obj->pos[1];
        obj->ds->pos[2] = obj->pos[2];
        OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
    }
}

/*
 * Draws the leaf with its orientation (the area's ambient model 2), its
 * texture moved to its variant.
 */
void AmbientLife_DrawFallingLeaf(DrawSlot *drawSlot)
{
    f32 *orient;
    ModelMatSet *set;
    f32 uvMtx[16];
    f32 variantU;
    s16 idx;
    u8 variant;
    Model *subModels;
    Obj *w;

    w = drawSlot->owner;
    subModels = gAreaSubModels[0];
    orient = w->buf;
    if ((subModels != NULL) && (subModels->active != 0)) {
        idx = gAreaFxModels[gScene.area][2];
        if (idx >= 0) {
            variant = w->var0;
            set = &subModels->matSets[idx];
            variantU = (f32) (u32) variant;
            Mtx_Translation(uvMtx, 0.25f * variantU, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            Mtx_SetTrans(orient, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(set, (s32) orient);
        }
    }
}

/*
 * A wanderer: starts at one of four points (AmbientLife_WandererPoints)
 * heading for another, its nose about level, flying 6 to 8 units a frame.
 */
void AmbientLife_SetupWanderer(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    u8 goal;
    f32 toGoalX;
    f32 toGoalZ;

    obj->var0 = Rand_Next(1) % 4;
    goal = Rand_Next(1) % 3;
    if (goal >= obj->var0) {
        goal++;
    }
    obj->var1 = 0;
    obj->var2 = 0;
    obj->timer = 0;
    obj->pos[0] = AmbientLife_WandererPoints[obj->var0 * 3];
    obj->pos[1] = AmbientLife_WandererPointsY[obj->var0 * 3];
    obj->pos[2] = AmbientLife_WandererPointsZ[obj->var0 * 3];
    toGoalX = AmbientLife_WandererPoints[goal * 3] - obj->pos[0];
    toGoalZ = AmbientLife_WandererPointsZ[goal * 3] - obj->pos[2];
    mover->rot[0] = (u8) Rand_Next(1) - 0xEF;
    mover->rot[1] = (s32) (65536.0f * Math_Atan2(toGoalX, toGoalZ) / 6.2831855f + 0.5f);
    mover->rot[2] = 0;
    mover->v[0] = 0.0f;
    mover->v[1] = 0.0f;
    mover->v[2] = 6.0f + 0.002f * (Rand_Next(1) & 0x3FF);
    mover->spinX = 0;
    mover->spinZ = 0;
}

/*
 * Flies the wanderer along its heading for 500 frames: random turns of 10 to
 * 25 frames, random pitching (0x60 a frame, down to -0x1000) and sharp turns
 * while its pitch is negative; spinX and spinZ serve as that mode and its
 * frames.
 */
void AmbientLife_RunWanderer(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    f32 step[3];

    obj->timer++;
    if (obj->timer >= 500) {
        obj->state++;
        return;
    }
    switch (obj->var1) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        obj->var1 = Rand_Next(1) & 7;
        if (obj->var1 > 5) {
            obj->var2 = (Rand_Next(1) & 0xF) + 10;
        }
        break;
    case 6:
        obj->var2--;
        mover->rot[1] -= 0x20;
        if (obj->var2 <= 0) {
            obj->var1 = 0;
        }
        break;
    case 7:
        obj->var2--;
        mover->rot[1] += 0x20;
        if (obj->var2 <= 0) {
            obj->var1 = 0;
        }
        break;
    }
    switch (mover->spinX) {
    case 0:
    case 1:
        if (--mover->spinZ < 0) {
            mover->spinX = Rand_Next(1) & 3;
            if (mover->spinX > 1) {
                mover->spinZ = (Rand_Next(1) & 7) + 3;
            } else {
                mover->spinZ = 0;
            }
        }
        break;
    case 2:
        if (mover->rot[0] > -0x1000) {
            mover->rot[0] -= 0x60;
        }
        mover->spinX = 0;
        break;
    case 3:
        if (mover->rot[0] < 0) {
            mover->rot[1] += 0x600;
        }
        mover->spinX = 0;
        break;
    }
    Mtx_Identity(mover->m);
    Mtx_RotateXYZ(mover->m, 6.2831855f * mover->rot[0] / 65536.0f, 6.2831855f * mover->rot[1] / 65536.0f,
                  6.2831855f * mover->rot[2] / 65536.0f);
    Vec_Rotate(step, mover->v, mover->m);
    obj->pos[0] += step[0];
    obj->pos[1] += step[1];
    obj->pos[2] += step[2];
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
}

/*
 * Draws the wanderer with its orientation (the area's ambient model 3)
 * without alpha test.
 */
void AmbientLife_DrawWanderer(DrawSlot *drawSlot)
{
    ModelMatSet *set;
    f32 *orient;
    s16 idx;
    Model *subModels;

    subModels = gAreaSubModels[0];
    orient = ((Obj *) drawSlot->owner)->buf;
    if ((subModels != NULL) && (subModels->active != 0)) {
        idx = gAreaFxModels[gScene.area][3];
        if (idx >= 0) {
            set = &subModels->matSets[idx];
            Gs_SetState(RS_ALPHA_REF, 0);
            Mtx_SetTrans(orient, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(set, (s32) orient);
            Gs_SetState(RS_ALPHA_REF, 0x80);
        }
    }
}

/*
 * A darter: a random place 300 up in the forest's area 7's box with a random
 * heading, creeping forward at 0.5 a frame.
 */
void AmbientLife_SetupDarter(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;

    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->timer = 0;
    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        obj->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0x7FF);
        obj->pos[1] = 300.0f;
        obj->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0x3FF);
        break;
    }
    mover->rot[1] = Rand_Next(1);
    mover->v[0] = 0.0f;
    mover->v[1] = 0.0f;
    mover->v[2] = 0.5f;
    mover->spinX = Rand_Next(1) & 0x1F;
    mover->spinZ = 0;
}

/*
 * Moves the darter in bursts: it waits, speeds up toward 10, cruises, then
 * cuts its speed by half or more (Obj.var0, spinX the frames); turns 0x180 or
 * 0x200 a frame for random spells (Obj.var1, Obj.timer) and climbs or sinks
 * (not below 250) for others (Obj.var2, spinZ). It ends once out of the
 * forest's area 7's box.
 */
void AmbientLife_RunDarter(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    f32 step[3];

    switch (gScene.area) {
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (!(obj->pos[0] >= 9773.0f && obj->pos[0] <= 11174.0f && obj->pos[1] <= 2000.0f && obj->pos[2] >= 9173.0f &&
              obj->pos[2] <= 10576.0f)) {
            obj->state++;
            return;
        }
        break;
    }
    switch (obj->var0) {
    case 0:
        if (--mover->spinX <= 0) {
            obj->var0++;
            mover->spinX = Rand_Next(1) % 6 + 2;
        }
        break;
    case 1:
        if (mover->v[2] < 10.0f) {
            if ((s16) (Rand_Next(1) & 1) == 0) {
                mover->v[2] += 0.5f;
            } else {
                mover->v[2] += 0.3f;
            }
        }
        if (--mover->spinX <= 0) {
            obj->var0++;
            mover->spinX = Rand_Next(1) % 8 + 3;
        }
        break;
    case 2:
        if (--mover->spinX <= 0) {
            obj->var0++;
            mover->spinX = (Rand_Next(1) & 4) + 2;
        }
        break;
    case 3:
        if ((s16) (Rand_Next(1) & 1) == 0) {
            mover->v[2] *= 0.5f;
        } else {
            mover->v[2] *= 0.4f;
        }
        if (--mover->spinX <= 0) {
            obj->var0 = 0;
            mover->spinX = Rand_Next(1) % 20 + 7;
        }
        break;
    }
    switch (obj->var1) {
    case 0:
        obj->timer = Rand_Next(1) % 20 + 5;
        obj->var1 = Rand_Next(1) % 3 + 1;
        break;
    case 1:
        if (--obj->timer <= 0) {
            obj->var1 = 0;
        }
        break;
    case 2:
        if (--obj->timer <= 0) {
            obj->timer = Rand_Next(1) % 10 + 5;
            obj->var1 = 1;
        }
        mover->rot[1] -= 0x180;
        break;
    case 3:
        if (--obj->timer <= 0) {
            obj->timer = Rand_Next(1) % 10 + 5;
            obj->var1 = 1;
        }
        mover->rot[1] += 0x180;
        break;
    case 4:
        if (--obj->timer <= 0) {
            obj->timer = Rand_Next(1) % 10 + 5;
            obj->var1 = 1;
        }
        mover->rot[1] -= 0x200;
        break;
    case 5:
        if (--obj->timer <= 0) {
            obj->timer = Rand_Next(1) % 10 + 5;
            obj->var1 = 1;
        }
        mover->rot[1] += 0x200;
        break;
    }
    switch (obj->var2) {
    case 0:
        mover->spinZ = (Rand_Next(1) & 0xF) + 5;
        obj->var2 = Rand_Next(1) % 3 + 1;
        break;
    case 1:
        if (--mover->spinZ <= 0) {
            obj->var2 = 0;
        }
        break;
    case 2:
        if (--mover->spinZ <= 0) {
            mover->spinZ = (Rand_Next(1) & 0xF) + 5;
            obj->var2 = 1;
        }
        obj->pos[1] += 1.0f;
        break;
    case 3:
        if (--mover->spinZ <= 0) {
            mover->spinZ = (Rand_Next(1) & 0xF) + 5;
            obj->var2 = 1;
        }
        if (obj->pos[1] > 250.0f) {
            obj->pos[1] -= 1.0f;
        }
        break;
    }
    Mtx_Identity(mover->m);
    Mtx_RotateY(mover->m, 6.2831855f * mover->rot[1] / 65536.0f);
    Vec_Rotate(step, mover->v, mover->m);
    obj->pos[0] += step[0];
    obj->pos[1] += step[1];
    obj->pos[2] += step[2];
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
}

/*
 * Draws the darter facing its heading (the area's ambient model 4) without
 * alpha test.
 */
void AmbientLife_DrawDarter(DrawSlot *drawSlot)
{
    ModelMatSet *set;
    f32 *orient;
    s16 idx;
    Model *subModels;

    subModels = gAreaSubModels[0];
    orient = ((Obj *) drawSlot->owner)->buf;
    if ((subModels != NULL) && (subModels->active != 0)) {
        idx = gAreaFxModels[gScene.area][4];
        if (idx >= 0) {
            set = &subModels->matSets[idx];
            Gs_SetState(RS_ALPHA_REF, 0);
            Mtx_SetTrans(orient, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(set, (s32) orient);
            Gs_SetState(RS_ALPHA_REF, 0x80);
        }
    }
}

/*
 * A steady flyer: a random place 450 up in the forest's area 5's box or 150
 * up in its area 7's, flying 5 units a frame.
 */
void AmbientLife_SetupSteadyFlyer(Obj *obj)
{
    u16 area;
    AmbientLifeMover *mover;

    mover = obj->buf;
    obj->var0 = 0;
    obj->var1 = 0;
    obj->var2 = 0;
    obj->timer = 0;
    area = gScene.area;
    switch (area) {
    case AREA_FOREST_HILLS_5:
    case AREA_FOREST_HILLS_NIGHT_5:
        obj->pos[0] = 10750.0f + (f32) (Rand_Next(1) & 0xFF);
        obj->pos[1] = 450.0f;
        obj->pos[2] = 11700.0f + (f32) (Rand_Next(1) & 0xFF);
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        obj->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0xFFFF & 0x7FF);
        obj->pos[1] = 150.0f;
        obj->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0xFFFF & 0x3FF);
        break;
    }
    mover->v[0] = 0.0f;
    mover->v[1] = 0.0f;
    mover->v[2] = 5.0f;
}

/*
 * Flies it at a steady speed with random turns (0x120 a frame) and climbs or
 * sinks (not below 50); it ends once out of its area's box.
 */
void AmbientLife_RunSteadyFlyer(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    f32 step[3];

    switch (gScene.area) {
    case AREA_FOREST_HILLS_5:
    case AREA_FOREST_HILLS_NIGHT_5:
        if (obj->pos[0] < 10450.0f || obj->pos[0] > 11850.0f || obj->pos[1] > 1500.0f || obj->pos[2] < 11400.0f ||
            obj->pos[2] > 12500.0f) {
            obj->state++;
            return;
        }
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (obj->pos[0] < 9773.0f || obj->pos[0] > 11174.0f || obj->pos[1] > 2000.0f || obj->pos[2] < 9173.0f ||
            obj->pos[2] > 10576.0f) {
            obj->state++;
            return;
        }
        break;
    }
    switch (obj->var0) {
    case 0:
        obj->timer = Rand_Next(1) % 40 + 10;
        obj->var0 = Rand_Next(1) % 3 + 1;
        break;
    case 1:
        if (--obj->timer <= 0) {
            obj->var0 = 0;
        }
        break;
    case 2:
        if (--obj->timer <= 0) {
            obj->var0 = 1;
        }
        mover->rot[1] -= 0x120;
        break;
    case 3:
        if (--obj->timer <= 0) {
            obj->var0 = 1;
        }
        mover->rot[1] += 0x120;
        break;
    }
    switch (obj->var1) {
    case 0:
        obj->var2 = (Rand_Next(1) & 0xF) + 5;
        obj->var1 = Rand_Next(1) % 2 + 1;
        break;
    case 1:
        if (--obj->var2 <= 0) {
            obj->var1 = 0;
        }
        break;
    case 2:
        if (--obj->var2 <= 0) {
            obj->var2 = (Rand_Next(1) & 0xF) + 5;
            obj->var1 = 1;
        }
        obj->pos[1] += 1.0f;
        break;
    case 3:
        if (--obj->var2 <= 0) {
            obj->var2 = (Rand_Next(1) & 0xF) + 5;
            obj->var1 = 1;
        }
        if (obj->pos[1] > 50.0f) {
            obj->pos[1] -= 1.0f;
        }
        break;
    }
    Mtx_Identity(mover->m);
    Mtx_RotateY(mover->m, 6.2831855f * mover->rot[1] / 65536.0f);
    Vec_Rotate(step, mover->v, mover->m);
    obj->pos[0] += step[0];
    obj->pos[1] += step[1];
    obj->pos[2] += step[2];
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
}

/* Draws the steady flyer facing its heading (the area's ambient model 5). */
void AmbientLife_DrawSteadyFlyer(DrawSlot *drawSlot)
{
    ModelMatSet *set;
    f32 *orient;
    s16 idx;
    Model *subModels;

    subModels = gAreaSubModels[0];
    orient = ((Obj *) drawSlot->owner)->buf;
    if ((subModels != NULL) && (subModels->active != 0)) {
        idx = gAreaFxModels[gScene.area][5];
        if (idx >= 0) {
            set = &subModels->matSets[idx];
            Mtx_SetTrans(orient, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(set, (s32) orient);
        }
    }
}

/*
 * A butterfly: a random place 150 up in the snowy mountains' area 9's or the
 * forest's area 7's box (elsewhere a default box, with a random heading), at
 * rest with a random wing angle, and a draw slot for each wing.
 */
void AmbientLife_SetupButterfly(Obj *obj)
{
    ButterflyBuf *butterfly = obj->buf;
    s16 i;

    obj->var0 = 0;
    obj->var1 = 0;
    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_9:
        obj->pos[0] = 6500.0f + (f32) (Rand_Next(1) & 0x7FF);
        obj->pos[1] = 150.0f;
        obj->pos[2] = 12750.0f + (f32) (Rand_Next(1) & 0x7FF);
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        obj->pos[0] = 9773.0f + (f32) (Rand_Next(1) & 0x7FF);
        obj->pos[1] = 150.0f;
        obj->pos[2] = 9173.0f + (f32) (Rand_Next(1) & 0x3FF);
        break;
    default:
        obj->pos[0] = (Rand_Next(1) & 0x7FF) + 0x5DC;
        obj->pos[1] = (Rand_Next(1) & 0xF) + 0x1E;
        obj->pos[2] = (Rand_Next(1) & 0xFFF) + 0x7D0;
        butterfly->heading = Rand_Next(1);
        break;
    }
    obj->timer = 0;
    butterfly->speed = 0.0f;
    butterfly->side = 0.0f;
    butterfly->pitch = Rand_Next(1) % 0x6000 - 0x3000;
    for (i = 0; i < 2; i++) {
        butterfly->slot[i] = DrawSlot_Alloc();
        if (butterfly->slot[i] != -1) {
            butterfly->ds[i] = DrawSlot_Get(butterfly->slot[i]);
            butterfly->ds[i]->owner = obj;
            butterfly->ds[i]->part = i;
            butterfly->ds[i]->draw = AmbientLife_Draw;
        } else {
            butterfly->ds[i] = NULL;
        }
    }
}

/*
 * Flutters the butterfly: random changes of speed (up to 4) and of sideways
 * drift, wing beats to a random angle on the other side (each downbeat
 * lifting it), random turns (0x180 a frame) and a sink by the wing angle;
 * both wings' slots follow it. It ends once out of its area's box.
 */
void AmbientLife_RunButterfly(Obj *obj)
{
    ButterflyBuf *butterfly = obj->buf;
    f32 speedScale;
    s16 i;
    s16 choice;
    s16 swing;
    s16 pitchStep;
    f32 *stepY;
    f32 *stepZ;
    f32 step[3];

    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_9:
        if (!(obj->pos[0] >= 6500.0f && obj->pos[0] <= 9000.0f && obj->pos[1] <= 2000.0f && obj->pos[1] >= -100.0f &&
              obj->pos[2] >= 12750.0f && obj->pos[2] <= 15000.0f)) {
            obj->state++;
            return;
        }
        speedScale = 1.0f;
        break;
    case AREA_FOREST_HILLS_7:
    case AREA_FOREST_HILLS_NIGHT_7:
        if (!(obj->pos[0] >= 9773.0f && obj->pos[0] <= 11174.0f && obj->pos[1] <= 2000.0f && obj->pos[1] >= -100.0f &&
              obj->pos[2] >= 9173.0f && obj->pos[2] <= 10576.0f)) {
            obj->state++;
            return;
        }
        speedScale = 1.0f;
        break;
    default:
        if (!(obj->pos[0] >= 1500.0f && obj->pos[0] <= 4000.0f && obj->pos[1] <= 2000.0f && obj->pos[1] >= -100.0f &&
              obj->pos[2] >= 3000.0f && obj->pos[2] <= 7000.0f)) {
            obj->state++;
            return;
        }
        speedScale = 1.0f;
        break;
    }
    choice = Rand_Next(1) % 5;
    switch (choice) {
    case 0:
        break;
    case 1:
        if (butterfly->speed < 4.0f * speedScale) {
            butterfly->speed += 0.15f;
        }
        break;
    case 2:
        butterfly->speed -= 0.15f * speedScale;
        if (butterfly->speed < 0.0f) {
            butterfly->speed = 0.0f;
        }
        break;
    case 3:
        if (butterfly->speed < 4.0f * speedScale) {
            butterfly->speed += 0.2f;
        }
        break;
    case 4:
        butterfly->speed *= 0.5f;
        break;
    }
    choice = Rand_Next(1) % 3;
    switch (choice) {
    case 0:
        break;
    case 1:
        if (butterfly->side < 0.8f * speedScale) {
            butterfly->side += 0.05f;
        }
        break;
    case 2:
        if (butterfly->side > 0.8f * -speedScale) {
            butterfly->side -= 0.05f;
        }
        break;
    }
    stepY = &step[1];
    stepZ = &step[2];
    step[0] = butterfly->side;
    *stepY = 0.0f;
    *stepZ = butterfly->speed;
    switch (obj->var0) {
    case 0:
        obj->var0++;
        swing = Rand_Next(1) % 0x1800;
        butterfly->steps = (s16) (2.0f * speedScale) + Rand_Next(1) % 3;
        if (obj->pos[1] < 200.0f) {
            butterfly->steps--;
        } else if (obj->pos[1] > 300.0f) {
            butterfly->steps++;
        }
        if (butterfly->pitch >= 0) {
            butterfly->goal = -0x2000 - swing;
        } else {
            butterfly->goal = swing + 0x2000;
        }
        break;
    case 1:
        pitchStep = (s16) (butterfly->goal - butterfly->pitch) / butterfly->steps;
        if (pitchStep < 0) {
            obj->pos[1] -= 0.6f * ((1.5f + speedScale) * ((pitchStep - 0x300) / 1600.0f));
        }
        *(u16 *) &butterfly->pitch += pitchStep;
        butterfly->steps--;
        if (butterfly->steps <= 0) {
            obj->var0++;
        }
        break;
    case 2:
        obj->var0 = 0;
        break;
    }
    switch (obj->var1) {
    case 0:
        obj->timer = Rand_Next(1) % 20 + 10;
        obj->var1 = Rand_Next(1) % 3 + 1;
    case 1:
        if (--obj->timer <= 0) {
            obj->var1 = 0;
        }
        break;
    case 2:
        if (--obj->timer <= 0) {
            obj->var1 = 1;
        }
        butterfly->heading -= 0x180;
        break;
    case 3:
        if (--obj->timer <= 0) {
            obj->var1 = 1;
        }
        butterfly->heading += 0x180;
        break;
    }
    obj->pos[1] += -2.9f * (((s16) Math_Absf(butterfly->pitch) + 0x1000) / 15000.0f);
    Vec_RotateY(step, 6.2831855f * butterfly->heading / 65536.0f);
    obj->pos[0] += step[0];
    obj->pos[1] += *stepY;
    obj->pos[2] += *stepZ;
    for (i = 0; i < 2; i++) {
        if (butterfly->ds[i] != NULL) {
            butterfly->ds[i]->pos[0] = obj->pos[0];
            butterfly->ds[i]->pos[1] = obj->pos[1];
            butterfly->ds[i]->pos[2] = obj->pos[2];
            OT_AddByDepth(gDrawOt32, butterfly->ds[i], 0x20, 0);
        }
    }
}

/*
 * Draws one wing (the area's ambient model 6 plus the wing), turned about the
 * body by the wing angle (the second wing mirrored) and then to the heading.
 */
void AmbientLife_DrawButterfly(DrawSlot *drawSlot)
{
    ButterflyBuf *butterfly = ((Obj *) drawSlot->owner)->buf;
    Model *stage = gAreaSubModels[0];
    ModelMatSet *model;
    f32 mtx[16];
    s16 modelIdx;

    if (stage != NULL && stage->active != 0) {
        modelIdx = gAreaFxModels[gScene.area][6];
        if (modelIdx >= 0) {
            model = &stage->matSets[modelIdx] + drawSlot->part;
            Mtx_Identity(mtx);
            if (drawSlot->part == 0) {
                Mtx_RotateZ(mtx, (6.2831855f * (u16) butterfly->pitch) / 65536.0f);
            } else {
                Mtx_RotateZ(mtx, (6.2831855f * -(u16) butterfly->pitch) / 65536.0f);
            }
            Mtx_RotateY(mtx, (6.2831855f * butterfly->heading) / 65536.0f);
            Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(model, (s32) mtx);
        }
    }
}

/*
 * A spinner: 100 above the area's start position (gStartPos), with a small
 * random turn per frame about each axis.
 */
void AmbientLife_SetupSpinner(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;

    obj->pos[0] = gStartPos[gScene.area * 3];
    obj->pos[1] = 100.0f + gStartPos[gScene.area * 3 + 1];
    obj->pos[2] = gStartPos[gScene.area * 3 + 2];
    obj->timer = 0;
    mover->rot[0] = (Rand_Next(1) & 0xF) - 8;
    mover->rot[1] = (Rand_Next(1) & 0xF) - 8;
    mover->rot[2] = (Rand_Next(1) & 0xF) - 8;
    Mtx_Identity(mover->m);
}

/*
 * Turns the spinner by its per-frame turn, steps its 10-frame texture
 * animation (Obj.timer by 2, up to 20) and adds it to the 32-entry draw list.
 */
void AmbientLife_RunSpinner(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;

    Mtx_RotateXYZ(mover->m, (6.2831855f * mover->rot[0]) / 65536.0f,
                  (6.2831855f * mover->rot[1]) / 65536.0f,
                  (6.2831855f * mover->rot[2]) / 65536.0f);
    obj->timer += 2;
    if (obj->timer >= 20) {
        obj->timer = 0;
    }
    obj->ds->pos[0] = obj->pos[0];
    obj->ds->pos[1] = obj->pos[1];
    obj->ds->pos[2] = obj->pos[2];
    OT_AddByDepth(gDrawOt32, obj->ds, 0x20, 0);
}

/*
 * Draws the spinner with its orientation (the area's ambient model 7), its
 * texture moved to the frame.
 */
void AmbientLife_DrawSpinner(DrawSlot *drawSlot)
{
    f32 uvMtx[16];
    ModelMatSet *set;
    f32 *orient;
    s16 idx;
    Model *subModels;
    Obj *w;

    w = drawSlot->owner;
    subModels = gAreaSubModels[0];
    orient = w->buf;
    if ((subModels != NULL) && (subModels->active != 0)) {
        idx = gAreaFxModels[gScene.area][7];
        if (idx >= 0) {
            set = &subModels->matSets[idx];
            Mtx_Translation(uvMtx, 0.05f * (f32) w->timer, 0.0f, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            Mtx_SetTrans(orient, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
            AmbientLife_DrawModel(set, (s32) orient);
        }
    }
}

/*
 * A circling bird: a random centre high over its area (the snowy mountains'
 * night area 9, the forest's area 6, the great arena's area 1, the desert's
 * areas 2, 8 and 9) and a radius (in the snowy area the distance to the
 * area's nearest edge, at most 6000); it starts at a random angle and goes
 * round at 33 units a frame (AmbientLifeMover.circleStep), one way or the
 * other at random (Obj.var2).
 */
void AmbientLife_SetupCirclingBird(Obj *obj)
{
    AmbientLifeMover *mover = obj->buf;
    f32 radius;
    f32 edgeDist;

    switch (gScene.area) {
    case AREA_SNOWY_MOUNTAINS_NIGHT_9:
        obj->pos[0] = 2000.0f + (Rand_Next(1) & 0x3FFF);
        obj->pos[1] = 2000.0f;
        obj->pos[2] = 2000.0f + (Rand_Next(1) & 0x3FFF);
        radius = obj->pos[0];
        if (radius < 10000.0f) {
            radius = radius;
        } else {
            radius = 20000.0f - radius;
        }
        edgeDist = obj->pos[2];
        if (edgeDist < 10000.0f) {
            edgeDist = edgeDist;
        } else {
            edgeDist = 20000.0f - edgeDist;
        }
        if (edgeDist < radius) {
            radius = edgeDist;
        }
        if (radius > 6000.0f) {
            radius = 6000.0f;
        }
        break;
    case AREA_FOREST_HILLS_6:
        obj->pos[0] = 9737.0f + ((Rand_Next(1) & 0x3FF) - 0x200);
        obj->pos[1] = 8412.0f + (Rand_Next(1) & 0x3FF);
        obj->pos[2] = 6057.0f + ((Rand_Next(1) & 0x3FF) - 0x200);
        radius = 2000.0f + (Rand_Next(1) & 0x3FF);
        break;
    case AREA_GREAT_ARENA_1:
        obj->pos[0] = 10500.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        obj->pos[1] = 3000.0f + (Rand_Next(1) & 0x3FF);
        obj->pos[2] = 9500.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        radius = 2000.0f + (Rand_Next(1) & 0xFF);
        break;
    case AREA_DESERT_2:
        obj->pos[0] = 10500.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        obj->pos[1] = 3000.0f + (Rand_Next(1) & 0x7FF);
        obj->pos[2] = 10000.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        radius = 2000.0f + (Rand_Next(1) & 0x1FF);
        break;
    case AREA_DESERT_8:
        obj->pos[0] = 6286.0f + ((Rand_Next(1) & 0x3FF) - 0x200);
        obj->pos[1] = 1500.0f + (Rand_Next(1) & 0x3FF);
        obj->pos[2] = 5300.0f + ((Rand_Next(1) & 0x3FF) - 0x200);
        radius = 1000.0f + (Rand_Next(1) & 0xFF);
        break;
    case AREA_DESERT_9:
        obj->pos[0] = 10000.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        obj->pos[1] = 2800.0f + (Rand_Next(1) & 0x7FF);
        obj->pos[2] = 9500.0f + ((Rand_Next(1) & 0x7FF) - 0x400);
        radius = 3500.0f + (Rand_Next(1) & 0x1FF);
        break;
    }
    mover->circleStep = 65536.0f * (33.0f / (6.2831855f * radius));
    obj->timer = 0;
    mover->rot[1] = Rand_Next(1);
    Mtx_Translation(mover->m, 0.0f, 0.0f, radius);
    if (Rand_Next(1) & 1) {
        Mtx_RotateY(mover->m, 1.5707964f);
        obj->var2 = 0;
    } else {
        mover->circleStep = -mover->circleStep;
        Mtx_RotateY(mover->m, 4.712389f);
        obj->var2 = 1;
    }
}

/* The file's data. */
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
u16 gMsgEventSets[28][2] = {
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
u8 *gMsgEventSetEntries[28] = {
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
u8 gMsgEventSetIcon[32] = {
#include "game/008AFED0.inc"
};
MsgEvent gMsgEventsQuestStart[28] = {
#include "game/008AFEF0.inc"
};
MsgEvent gMsgEventsSupplyBox[4] = {
#include "game/008AFFD0.inc"
};
MsgEvent gMsgEventsGoalDelivered[8] = {
#include "game/008AFFF0.inc"
};
MsgEvent gMsgEventsApcerosHunted[4] = {
#include "game/008B0030.inc"
};
MsgEvent gMsgEventsCamp[4] = {
#include "game/008B0050.inc"
};
MsgEvent gMsgEventsJungle1[6] = {
#include "game/008B0070.inc"
};
MsgEvent gMsgEventsJungle4[3] = {
#include "game/008B00A0.inc"
};
f32 LightShaft_PosForestHills7[3] = {
#include "game/008B00B8.inc"
};
s16 LightShaft_TiltsForestHills7[4] = {
#include "game/008B00C8.inc"
};
f32 LightShaft_ScalesForestHills7[2] = {
#include "game/008B00D0.inc"
};
f32 LightShaft_ScalesForestHills7Z[2] = {
#include "game/008B00D8.inc"
};
f32 LightShaft_PosForestHills11[6] = {
#include "game/008B00E0.inc"
};
s16 LightShaft_TiltsForestHills11[4] = {
#include "game/008B00F8.inc"
};
f32 LightShaft_WidthsForestHills11[2] = {
#include "game/008B0100.inc"
};
f32 LightShaft_PosForestHills12[3] = {
#include "game/008B0108.inc"
};
s16 LightShaft_TiltsForestHills12[4] = {
#include "game/008B0118.inc"
};
f32 LightShaft_PosSnowyMountains1[4] = {
#include "game/008B0120.inc"
};
f32 LightShaft_PosSnowyMountains3[4] = {
#include "game/008B0130.inc"
};
f32 LightShaft_PosTower8[8] = {
#include "game/008B0140.inc"
};
f32 LightShaft_PosDesert4[6] = {
#include "game/008B0160.inc"
};
s16 LightShaft_ModelsDesert4[4] = {
#include "game/008B0178.inc"
};
f32 LightShaft_PosSwamp3[8] = {
#include "game/008B0180.inc"
};
f32 LightShaft_PosSwamp4[6] = {
#include "game/008B01A0.inc"
};
s16 SkyStreak_Counts[4] = {
#include "game/008B01B8.inc"
};
s16 SkyStreak_RandRange[4] = {
#include "game/008B01C0.inc"
};
s16 SkyStreak_WaitBase[4] = {
#include "game/008B01C8.inc"
};
u16 SkyStreak_WaitMask[4] = {
#include "game/008B01D0.inc"
};
s16 SkyStreak_Frames[4] = {
#include "game/008B01D8.inc"
};
s16 SkyStreak_Models[4] = {
#include "game/008B01E0.inc"
};
f32 AreaDrop_ZonesDesertNight4[3] = {
#include "game/008B01E8.inc"
};
f32 AreaDrop_ZoneRadiiDesertNight4[2] = {
#include "game/008B01F8.inc"
};
f32 AreaDrop_ZonesTower8[4] = {
#include "game/008B0200.inc"
};
f32 AreaDrop_ZoneRadiiTower8[2] = {
#include "game/008B0210.inc"
};
f32 AreaDrop_ZonesDesert4[3] = {
#include "game/008B0218.inc"
};
f32 AreaDrop_ZoneRadiiDesert4[2] = {
#include "game/008B0228.inc"
};
f32 AreaDrop_FallPointsDesert10[12] = {
#include "game/008B0230.inc"
};
f32 AreaDrop_ZonesSwamp3[6] = {
#include "game/008B0260.inc"
};
f32 AreaDrop_ZoneRadiiSwamp3[2] = {
#include "game/008B0278.inc"
};
f32 AreaDrop_ZonesSwamp4[6] = {
#include "game/008B0280.inc"
};
f32 AreaDrop_ZoneRadiiSwamp4[2] = {
#include "game/008B0298.inc"
};
f32 AreaDrop_ZonesSwamp8[6] = {
#include "game/008B02A0.inc"
};
f32 AreaDrop_ZoneRadiiSwamp8[2] = {
#include "game/008B02B8.inc"
};
u16 AreaWeapon_GunTurnSounds[4] = {
#include "game/008B02C0.inc"
};
u8 AreaWeapon_PartKindsFortress4[8] = {
#include "game/008B02C8.inc"
};
f32 AreaWeapon_PosFortress4[10] = {
#include "game/008B02D0.inc"
};
u8 AreaWeapon_PartKindsFortress5[8] = {
#include "game/008B02F8.inc"
};
f32 AreaWeapon_Frames6_0[4] = {
#include "game/008B0300.inc"
};
f32 AreaWeapon_Frames6_1[4] = {
#include "game/008B0310.inc"
};
f32 AreaWeapon_Frames6_2[4] = {
#include "game/008B0320.inc"
};
f32 AreaWeapon_Frames6_3[4] = {
#include "game/008B0330.inc"
};
f32 AreaWeapon_Frames6_4[4] = {
#include "game/008B0340.inc"
};
f32 *AreaWeapon_PosFortress5[8] = {
    AreaWeapon_Frames6_0, AreaWeapon_Frames6_1, AreaWeapon_Frames6_2, AreaWeapon_Frames6_3,
    AreaWeapon_Frames6_4, NULL, NULL, NULL,
};
f32 AreaWeapon_BurstPosFortress5[12] = {
#include "game/008B0370.inc"
};
u8 AreaWeapon_PartKindsCastle1[8] = {
#include "game/008B03A0.inc"
};
f32 AreaWeapon_Frames2_0[3] = {
#include "game/008B03A8.inc"
};
f32 AreaWeapon_Frames2_1[3] = {
#include "game/008B03B8.inc"
};
f32 AreaWeapon_Frames2_2[3] = {
#include "game/008B03C8.inc"
};
f32 *AreaWeapon_PosCastle1[3] = {
    AreaWeapon_Frames2_0, AreaWeapon_Frames2_1, AreaWeapon_Frames2_2,
};
u16 AreaWeapon_YawCastle1[4] = {
#include "game/008B03E8.inc"
};
f32 AreaWeapon_BurstPosCastle1[4] = {
#include "game/008B03F0.inc"
};
u8 AreaWeapon_PartKindsFortress3[16] = {
#include "game/008B0400.inc"
};
f32 AreaWeapon_PosFortress3[6] = {
#include "game/008B0410.inc"
};
u8 AreaWeapon_PartKindsFortress2[8] = {
#include "game/008B0428.inc"
};
f32 AreaWeapon_PosFortress2[10] = {
#include "game/008B0430.inc"
};
u8 AreaWeapon_PartKindsBattleTown1[8] = {
#include "game/008B0458.inc"
};
f32 AreaWeapon_Frames8_0[4] = {
#include "game/008B0460.inc"
};
f32 AreaWeapon_Frames8_1[4] = {
#include "game/008B0470.inc"
};
f32 AreaWeapon_Frames8_2[6] = {
#include "game/008B0480.inc"
};
f32 *AreaWeapon_PosBattleTown1[3] = {
    AreaWeapon_Frames8_0, AreaWeapon_Frames8_1, AreaWeapon_Frames8_2,
};
u16 AreaWeapon_GunRestYawBattleTown1[4] = {
#include "game/008B04A8.inc"
};
f32 AreaWeapon_BurstPosBattleTown1[6] = {
#include "game/008B04B0.inc"
};
u8 AreaWeapon_PartKindsBattleTown3[8] = {
#include "game/008B04C8.inc"
};
f32 AreaWeapon_Frames9_0[4] = {
#include "game/008B04D0.inc"
};
f32 AreaWeapon_Frames9_1[4] = {
#include "game/008B04E0.inc"
};
f32 AreaWeapon_Frames9_2[4] = {
#include "game/008B04F0.inc"
};
f32 AreaWeapon_Frames9_3[4] = {
#include "game/008B0500.inc"
};
f32 *AreaWeapon_PosBattleTown3[4] = {
    AreaWeapon_Frames9_0, AreaWeapon_Frames9_1, AreaWeapon_Frames9_2, AreaWeapon_Frames9_3,
};
u16 AreaWeapon_DayAreas[16] = {
#include "game/008B0520.inc"
};
f32 *AreaWeapon_PartPos[12] = {
    AreaWeapon_PosFortress4, (f32 *)AreaWeapon_PosFortress5, (f32 *)AreaWeapon_PosCastle1, AreaWeapon_PosFortress3,
    AreaWeapon_PosFortress2, AreaWeapon_PosFortress4, (f32 *)AreaWeapon_PosFortress5, AreaWeapon_PosFortress2,
    (f32 *)AreaWeapon_PosBattleTown1, (f32 *)AreaWeapon_PosBattleTown3, NULL, NULL,
};
u8 *AreaWeapon_PartKinds[12] = {
    AreaWeapon_PartKindsFortress4, AreaWeapon_PartKindsFortress5, AreaWeapon_PartKindsCastle1, AreaWeapon_PartKindsFortress3,
    AreaWeapon_PartKindsFortress2, AreaWeapon_PartKindsFortress4, AreaWeapon_PartKindsFortress5, AreaWeapon_PartKindsFortress2,
    AreaWeapon_PartKindsBattleTown1, AreaWeapon_PartKindsBattleTown3, NULL, NULL,
};
s16 AreaWeapon_NoModels[24] = {
#include "game/008B05A0.inc"
};
s16 AreaWeapon_GunModels[24] = {
#include "game/008B05D0.inc"
};
s16 AreaWeapon_GreatWeaponModels[24] = {
#include "game/008B0600.inc"
};
s16 AreaWeapon_FixedModels[24] = {
#include "game/008B0630.inc"
};
s16 *AreaWeapon_Models[4] = {
    AreaWeapon_NoModels, AreaWeapon_GunModels, AreaWeapon_GreatWeaponModels, AreaWeapon_FixedModels,
};
f32 AnimatedProps_PosForestHills11[24] = {
#include "game/008B0670.inc"
};
f32 AnimatedProps_ScalesForestHills11[24] = {
#include "game/008B06D0.inc"
};
f32 AnimatedProps_PosSnowyMountains6[18] = {
#include "game/008B0730.inc"
};
s16 AnimatedProps_ModelsSnowyMountains6[6] = {
#include "game/008B0778.inc"
};
s16 AnimatedProps_SpinSnowyMountains6[6] = {
#include "game/008B0788.inc"
};
s16 AnimatedProps_ModelsSnowyNight5[4] = {
#include "game/008B0798.inc"
};
f32 AnimatedProps_PosSnowyNight6[26] = {
#include "game/008B07A0.inc"
};
s16 AnimatedProps_SpinSnowyNight6[6] = {
#include "game/008B0808.inc"
};
s16 AnimatedProps_ModelsSnowyNight8[4] = {
#include "game/008B0818.inc"
};
f32 AnimatedProps_PosBattleTown0[40] = {
#include "game/008B0820.inc"
};
s16 AnimatedProps_ModelsBattleTown0[16] = {
#include "game/008B08C0.inc"
};
f32 AnimatedGroundTiles_UvSteps[64] = {
#include "game/008B08E0.inc"
};
f32 AmbientLife_WandererPoints[12] = {
#include "game/008B09E0.inc"
};
