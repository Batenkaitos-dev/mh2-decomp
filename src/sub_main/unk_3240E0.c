#include "common.h"
#include "syssettings.h"
#include "quest.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "pad.h"
#include "stageobj.h"
#include "model.h"
#include "syssave.h"
#include "save.h"
#include "task.h"
#include "area.h"
#include "qsync.h"
#include "sub_main/unk_3240E0.h"   /* the texts this file uses (tools/content_gen.py) */

/* prototypes */
void Bgm_EnterArea(u16 area);
void Frame_Draw(void);
void Game_ResetSubsystems(void);
s32 Model_Draw(u32 model, s32 flags);
void Model_FreeAll(void);
void Pad_UpdateInput(void);
u16 Rand_Next(s32 a);
void Anim_SetupCommon(void);
void Anim_SetupPlayer(s32);
void Anim_SetupEm(s32, s32);
void Anim_SetupEmSlot(s32);
void CamCtl_Clear(void);
void CamReq_Reset(void);
void StageObj_Free(StageObj *p);
void Draw_SetBlendFlags(u16 flags);
f32 Effect_Scale(void);
void Scene_StartQuest(void);
void Scene_StartQuestPlay(void);
void Scene_LoadQuestSounds(void);
void Scene_StartField(void);
s32 Game_PauseState(void);
void Quest_TeardownNet(void);
void Anim_LoadPlayer(s32 arg0, s32 arg1);
void Light_InitAll(s32 unused0);
void Load_PlayerAnim(s32 unused, s32 i);
void Load_CommonAnim(s32 i);
void Load_EmAnim(s32 unused, s32 i);
void Load_EmSlotAnim(s32 i, s32 j);
void Vec_Scale(f32 *, f32 *, f32);
void Draw_ApplyStates(u32 a);
void Draw_SetDefaultState(void);
void Player_InitPartTable(s32 idx);
void TaskSlot2_EndCode1(void);
void *Hud_Init(void);
void Jingle_OnAreaChange(u16 area);
void func_00368ED0(void);
void Gfx_SetBlend(s32 a, s32 b);
void Gfx_SetState63(s32 a);
void Gfx_SetBlendOp(s32 a);
void QuestSync_Clear(void);
s32 Game_IsOnline(void);
void Loading_Request(void);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Player_InitForQuest(Player *p);
void Mem_Compact(void);
s32 Gs_SetState(u8 a, u32 b);
void Stream_Reset(void);
void Sound_Silence(void);
void Sound_ReloadBank1(void);
s32 QuestNet_NoPeerReady(void);
void QSync_SendState(u8 kind, u8 who);
s32 func_D683C0(void);
void * memset(void *p, s32 c, u32 n);
extern u8 gAreaKindColors[];
extern u8 D_0041CF01[];
extern u8 D_0041CF02[];
extern u8 D_0041CF03[];
extern s16 gAreaKinds[];
extern u8 gLightSets;
extern u16 D_006ADB48;
extern s8 gGalleryTitle[];
void plsel_entry(TaskSlot *task); /* plsel (overlay 0xB) */
void Player_SetMyUserId(u8 i);
void Loading_SetMessage(u32 mode);
void Scene_LoadOverlay(s32 id);
void TaskSlot_Set(void *func, s16 slot);
void Field_UpdateAll(void);
void Hit_ClearStatusAll(void);
void ItemHud_Update(void);
void Frame_DrawOverCapture(void);
s32 Quest_PhaseIsOver(s32 a);
extern u8 gPadReplay;
extern u8 *gLoadBuf;
extern s32 gAreaTableFiles[];
extern void *gWorkArea;
s32 LoadCompressedFile(u8 *dst, s32 id);
void *Pack_GetPart(u8 *buf, s32 a);
s32 Pack_PartSize(u8 *buf, s32 i);
void *memcpy(void *dst, const void *src, u32 n);
void AreaTable_Relocate(u8 *file);
void AreaTable2_Relocate(u8 *file);

void Tex_LoadFile(s32 a, s32 b, s32 c);
void Area_Load(s32 area);
void Quest_InitPlayers(void);
void Players_UpdateSides(void);
void Hud_LoadTextures(void);
void Player_SetupQuest(s32 i);
void Quest_LoadCommonModels(void);
void func_00327F70(void);
void func_003350B0(void);
void func_0033A360(void);
void Anim_LoadCommon(s32 arg0);
s32 Gs_GetState(u8 reg);
struct PrimLine;
s32 Prim_FillRect(struct PrimLine *r);
/* A filled screen rectangle (Prim_FillRect). */
typedef struct ScreenRect {
    s16 x, y, w, h;
    u32 color;
} ScreenRect;
/* The header of the two tables Area_LoadTables loads (AreaTable_Relocate and
 * AreaTable2_Relocate relocate them): offsets from the header. */
typedef struct DataHdr {
    u32 unk00;
    u32 unk04;
    u32 rows;           /* 0x08 */
    u32 cols;           /* 0x0C */
    u8 unk10[0x18 - 0x10];
    u32 tblOff;         /* 0x18: rows * cols offsets of lists ended by -1 */
    u32 dataOff;        /* 0x1C: the base of the offsets in the lists */
} DataHdr;
extern u32 gAreaGridCellX;
extern u32 gAreaGridCellZ;
extern u32 gAreaGridCols;
extern u32 gAreaGridRows;
extern s32 **gAreaGridCells;
extern u32 gAreaGridData;
extern u32 gAreaGrid2CellX;
extern u32 gAreaGrid2CellZ;
extern u32 gAreaGrid2Cols;
extern u32 gAreaGrid2Rows;
extern s32 **gAreaGrid2Cells;
extern u32 gAreaGrid2Data;
extern s32 gAreaTexFiles[];
extern s32 gArea83TexFiles;
extern s32 D_0042582C;
extern s32 gArea84TexFiles;
extern s32 D_00425834;
void Load_AreaPack(s32 id);
s32 Model_Alloc(s32 n);
Model *Model_Get(s32 i);
void Model_Reserve(s32 first, s32 n);
s32 ElderRaid_InState1(void);
Model *Model_SetupTexPack(s16 idx, s32 arg1, s32 arg2, s32 arg3, s32 arg4);
void Area_LoadSubModels(u16 area);
void Area_LoadCamData(s32 area);
void Area_LoadTables(s32 id);
extern s32 gCommonModelTexFiles[];
void Load_CommonModelPack(s32 i);
Model *Model_SetupWithPack(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c);
extern u8 gSoundLoading;
void Plsel_SelectTask(TaskSlot *task); /* plsel */
void Gfx_ClearModelBuffers(void);
void Quest_Clear(void);
void Packet_SetMode0(void);
void Notice_ResetQueue(void);
void Quest_Init(void);
void Scene_InitSupplyBox(void);
s32 Quest_IsDrill(void);
s32 Quest_IsTraining(void);
extern s32 D_005B1DB8;
extern s32 D_005B1DC0;
s32 QSync_Begin(void);
void Quest_ResetTimer(void);
void Area_ApplyFog(u16 area);
void Area_StartObjects(u16 area);
void Tex_LoadCommon(void);
void CamCtl_Update(s32 a);
void Tex_PatchClut_Unk(void);
void Loading_Stop(void);
void QNetIf_Unready(void);
extern u16 gFrameTick;
void QNetIf_Ready(s32 a);
void *QuestInfo_Get(s32 i);
char *strcpy(char *dst, char *src);
void ScEvent_Init(void);
s32 Quest_IsVersus(void);
void Dmcam_Preload(void);
void ObjList_Update(void);
void SightBlock_ClearAll(void);
void FlashObj_ClearAll(void);
void ObjListAF0_Clear(void);
void ObjListA70_Clear(void);
void Versus_ConsumeTeamItem(void);
void Quest_SpawnMonsters(s32 a);
void Packet_SetMode2(void);
void Hud_Start(void);
void Quest_Setup(void);
/* A sprite description (Sprite_Queue; SprDef in unk_363550.c, with the
 * color as one word). */
typedef struct SprDesc {
    s32 tex;                    /* 0x00 */
    f32 pos[3];                 /* 0x04 */
    f32 size[2];                /* 0x10 */
    f32 scale[2];               /* 0x18 */
    f32 uv[4];                  /* 0x20 */
    f32 alpha;                  /* 0x30 */
    s32 unk34;                  /* 0x34 */
    s32 flags;                  /* 0x38 */
    u32 color;                  /* 0x3C */
    f32 rot[3];                 /* 0x40 */
} SprDesc;
struct SprDef;
void Sprite_Queue(struct SprDef *s);
void Tex_LoadResultTex_Unk(void);
void AreaObj_K17_Create(u8 a);
void Notice_Update(void);
extern u8 gPlayerSys[];
extern u8 gWeaponKindClass[];
void Quest_SetStartArea(u16 who);
u16 Equip_GetKind(EquipItem *e);
s32 sprintf(char *dst, char *fmt, ...);
s32 Player_IsLocal(void *obj);
void Player_LoadFromSave(Player *p);
void Player_LoadName(Player *p);
void Player_LoadPreset(s8 *id, Player *p);
void func_D63170(Player *p, s32 i);
void func_D633F0(Player *p, s32 i);
void Player_CalcSkills(Player *p);
u8 WeaponClass_IsGunner(u8 cls);
u8 Equip_GetBladeSpecial(EquipItem *e);
u8 WeaponType_Flags(u8 cls);
void Player_BackupPouch(Player *pl, s32 load);
extern u16 D_006AE0B4;
extern char **gAwardNoticeLines[];
void Save_SetEventFlag(s32 n);
s32 Movie_Play(s32 id);
s32 Movie_Update(void);
s32 Movie_GetStatus(void);
void Movie_RequestStop(void);
void Movie_Stop(void);
void EventFlag_Set(s32 n);
s32 EventFlag_Test(s32 n);
void Ending_Init(void);
s32 func_D64D40(void);
void Font_SetSpacing(u8 a, u8 b);
void Text_SetColor(s16 color);
s32 Str_Length(char *s);
void Font_SetPos(s16 x, s16 y);
void Text_Printf(char *fmt, ...);
void Text_DrawAllPages(void);
u8 SeqA_GetState(void);
void SeqA_Reset(void);
extern u8 gCameraPreset;
void Pad_ResetInput(void);
void func_00328660(void);
void Light_Reset(void);
void Area_Enter(u16 area);
void SeqB_Reset(void);
void Camera_SetPreset(s32 id);
void CamCtl_Init(void);
void func_00364BD0(s32 unused0);
void func_00364BE0(s32 unused0);
void Quest_EnterArea(s32 area);
s32 Debug_IsOn(s32 unused0);
void GmLoad_Reset(void);
void GmLoad_LoadStagePartition(void);
void GmLoad_LoadEmData(s32 kind, s32 idx);
void EmSlot_LoadModels(s32 slot);
Em *EmSlot_Alloc(void);
extern s16 gAreaSeBanks[];
s32 Sound_StopBank(s32 slot);
s32 Sound_SetSpuMapPage(u32 page);
void FlushCache(s32 op);
void Sound_QueueBank(u32 id, void *buf);
s32 FileQueue_IsBusy(void);
struct SndFile;
s32 Sound_LoadBank(struct SndFile *f, u32 slot);
s32 Sound_CheckTransfer(void);
void SndLoad_Init(s8 count);
s32 SndLoad_QuestBanks(u8 *buf, u8 *quest);
s32 Sound_LoadStaged(struct SndFile *f, u8 *out, u32 slot);
s32 SndLoad_ListBanks(u8 *buf, u8 *list);
extern u8 gScreenCaptureOk;
extern u32 *gReadBuf;
void QSync_UpdatePlayerPos(void);
void ScEvent_Update(void);
s32 Quest_Update(void);
void Sound_ReverbOff(void);
void Stream_FadeOut(s32 ch, s32 frames);
void Stream_Stop(s32 ch);
s32 ScreenTex_Reserve(u32 *pixels);
s32 ScreenTex_Capture(s32 fromTex);
void Map_DrawMain_Unk(void);
void Vib_StopAll(void);
void Stream_Pause(s32 ch, s32 pause);
void Area_FreeAll(void);
void QuestEm_LeaveArea(s32 a, s32 area);
void MinePoint_RemoveTemporary(void);
void Obj_FreeAll(void);
void LightSet_UpdateAll(void);
void FreeList_InitAll(void);
void DrawSlot_ResetAll(void);
void Em_RunAll(void);
void QuestEm_SpawnArea(s32 area);
void Player_InitAll(s16 mode);
s32 Scene_FieldFrame(void);
/* end of prototypes */

/* The StageObj pool: 0x80 objects of 0x40 bytes, a free stack and a list of
 * the objects in use (include/stageobj.h). */
s32 ObjBuf_FindFree(s32 n);
u8 *ObjBuf_GetBlock(s32 blk);
void ObjBuf_MarkUsed(s32 start, s32 n);
void ObjBuf_MarkFree(s32 start, s32 n);
extern StageObj **gStageObjFreeTop;   /* the free stack */
extern s32 gStageObjFreeCount;          /* its number of free objects */
extern StageObj gStageObjs[0x80];
extern StageObj *gStageObjList;    /* the objects in use */

/* Clears the pool and stacks every object as free. */
void StageObj_InitPool(void)
{
    s32 i;
    StageObj *p;

    memset(gStageObjs, 0, sizeof(gStageObjs));
    gStageObjList = NULL;
    p = &gStageObjs[0x7F];
    gStageObjFreeTop = (StageObj **)gStageObjs;
    for (i = 0; i < 0x80; i++, p--) {
        *--gStageObjFreeTop = p;
    }
    gScene.placed3 = 0;
    gStageObjFreeCount = 0x80;
}

/* Frees every object in use but those marked keep. */
void StageObj_FreeUnkept(void)
{
    StageObj *p = gStageObjList;
    StageObj *q;

    while (p != NULL) {
        if (p->active != 0 && p->keep == 0) {
            q = p;
            p = p->next;
            StageObj_Free(q);
        } else {
            p = p->next;
        }
    }
}

/* Takes a free object with n blocks of the object buffer, at the head of the
 * list. */
StageObj *StageObj_Alloc(s32 n)
{
    s32 blk;
    StageObj *p;

    if ((blk = ObjBuf_FindFree(n)) < 0) {
        return NULL;
    }
    if (gStageObjFreeCount == 0) {
        return NULL;
    }
    gStageObjFreeCount--;
    p = *gStageObjFreeTop++;
    p->prev = NULL;
    p->next = gStageObjList;
    gStageObjList = p;
    if (p->next != NULL) {
        p->next->prev = p;
    }
    p->active = 1;
    p->keep = 0;
    p->buf = ObjBuf_GetBlock(blk);
    p->blk = blk;
    p->nblk = n;
    ObjBuf_MarkUsed(blk, n);
    return p;
}

/* The same, at the tail of the list. */
StageObj *StageObj_AllocTail(s32 n)
{
    s32 blk;
    StageObj *p;
    StageObj *q;

    if ((blk = ObjBuf_FindFree(n)) < 0) {
        return NULL;
    }
    if (gStageObjFreeCount == 0) {
        return NULL;
    }
    gStageObjFreeCount--;
    p = *gStageObjFreeTop++;
    if (gStageObjList == NULL) {
        gStageObjList = p;
        p->prev = NULL;
    } else {
        q = gStageObjList;
        while (q->next != NULL) {
            q = q->next;
        }
        q->next = p;
        p->prev = q;
    }
    p->next = NULL;
    p->active = 1;
    p->keep = 0;
    p->buf = ObjBuf_GetBlock(blk);
    p->blk = blk;
    p->nblk = n;
    ObjBuf_MarkUsed(blk, n);
    return p;
}

/* Unlinks an object and gives it and its blocks back. */
void StageObj_Free(StageObj *p)
{
    if (p->prev == NULL) {
        gStageObjList = p->next;
    } else {
        p->prev->next = p->next;
    }
    if (p->next != NULL) {
        p->next->prev = p->prev;
    }
    gStageObjFreeCount++;
    *--gStageObjFreeTop = p;
    ObjBuf_MarkFree(p->blk, p->nblk);
    memset(p, 0, 8);
    p->slot = 0;
    *(s32 *)&p->node = 0;
}

/* Runs the update of every object in use. */
void StageObj_UpdateAll(void)
{
    StageObj *p;

    for (p = gStageObjList; p != NULL; p = p->next) {
        if (p->active != 0) {
            ((void (*)(StageObj *))p->update)(p);
        }
    }
}

/* Draws the visible objects, but kind 4 type 0 (drawn by StageObj_DrawKind4). */
void StageObj_DrawAll(void)
{
    StageObj *p;

    for (p = gStageObjList; p != NULL; p = p->next) {
        if (p->draw != NULL && p->active != 0 && p->visible != 0) {
            if (p->kind != 4 || p->type != 0) {
                ((void (*)(StageObj *))p->draw)(p);
            }
        }
    }
}

/* Draws the visible objects of kind 4 type 0. */
void StageObj_DrawKind4(void)
{
    StageObj *p;

    for (p = gStageObjList; p != NULL; p = p->next) {
        if (p->draw != NULL && p->active != 0 && p->visible != 0 && p->kind == 4 && p->type == 0) {
            ((void (*)(StageObj *))p->draw)(p);
        }
    }
}

void Key_LerpVec3(f32 *k, f32 *out, f32 t)
{
    f32 r;
    f32 k1;
    f32 k0;

    for (;; k += 4) {
        k0 = k[0];
        if (k0 == -1.0f) {
            break;
        }
        if (t == k0) {
            out[0] = k[1];
            out[1] = k[2];
            out[2] = k[3];
            return;
        }
        if (t > k0 && t < (k1 = k[4])) {
            r = (t - k0) / (k1 - k0);
            out[0] = k[1] + r * (k[5] - k[1]);
            out[1] = k[2] + r * (k[6] - k[2]);
            out[2] = k[3] + r * (k[7] - k[3]);
            return;
        }
    }
    out[0] = k[-3];
    out[1] = k[-2];
    out[2] = k[-1];
}

/* {t, x, y} keys, ended by t = -1.0f: x and y at t (z set to 1). The
 * volatile keeps the store of 1.0 in the loop, as in the original; the last
 * key case writes z, not y (sic). */
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t)
{
    f32 r;
    f32 k1;
    f32 k0;
    for (;; k += 3) {
        out[2] = 1.0f;
        k0 = k[0];
        if (k0 == -1.0f) {
            break;
        }
        if (t == k0) {
            out[0] = k[1];
            out[1] = k[2];
            return;
        }
        if (t > k0 && t < (k1 = k[3])) {
            r = (t - k0) / (k1 - k0);
            out[0] = k[1] + r * (k[4] - k[1]);
            out[1] = k[2] + r * (k[5] - k[2]);
            return;
        }
    }
    out[0] = k[-2];
    out[2] = k[-1];
}

void Key_LerpF32(f32 *k, f32 *out, f32 t)
{
    f32 r;
    f32 k1;
    f32 k0;

    for (;; k += 2) {
        k0 = k[0];
        if (k0 == -1.0f) {
            break;
        }
        if (t == k0) {
            out[0] = k[1];
            return;
        }
        if (t > k0 && t < (k1 = k[2])) {
            r = (t - k0) / (k1 - k0);
            out[0] = k[1] + r * (k[3] - k[1]);
            return;
        }
    }
    out[0] = k[-1];
}

/* Color keys {time, 0xAARRGGBB}, ended by time -1: the color at time t,
 * each channel interpolated. The numerator t - k0 goes in c0 (reused for the
 * first color below): as a new value it is scheduled before k1 - k0, as the
 * game does (an in-place t -= k0 went after it). */
void Color_EvalKeys(s32 *k, s32 t, u32 *out)
{
    f32 r;
    s32 k0;
    s32 k1;
    u32 c0;
    u32 c1;
    u8 r0;
    u8 a0;
    u8 g0;
    u8 b0;
    u8 a;
    u8 rr;
    u8 g;
    u8 b;

    for (;; k += 2) {
        k0 = k[0];
        if (k0 == -1) {
            break;
        }
        if (t == k0) {
            *out = k[1];
            return;
        }
        k1 = k[2];
        if (t > k0 && t < k1) {
            c0 = t - k0;
            c1 = k1 - k0;
            r = (f32)(s32)c0 / (f32)(s32)c1;
            c0 = k[1];
            c1 = k[3];
            r0 = c0 >> 16;
            g0 = c0 >> 8;
            b0 = c0;
            a0 = c0 >> 24;
            a = a0 + (u8)(r * ((f32)(u8)(c1 >> 24) - (f32)a0));
            rr = r0 + (u8)(r * ((f32)(u8)(c1 >> 16) - (f32)r0));
            g = g0 + (u8)(r * ((f32)(u8)(c1 >> 8) - (f32)g0));
            b = b0 + (u8)(r * ((f32)(u8)c1 - (f32)b0));
            *out = (a << 24) | (rr << 16) | (g << 8) | b;
            return;
        }
    }
    *out = k[-1];
}

void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateZXY(f32 *m, f32 x, f32 y, f32 z);

void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m)
{
    f32 x = trans[0];
    f32 y = trans[1];
    f32 z = trans[2];

    Mtx_Scale(m, scale[0], scale[1], scale[2]);
    switch (order) {
    case 6:
        Mtx_RotateXYZ(m, rot[0], rot[1], rot[2]);
        break;
    case 7:
        Mtx_RotateZXY(m, rot[0], rot[1], rot[2]);
        break;
    case 4:
        Mtx_RotateX(m, rot[0]);
        Mtx_RotateY(m, rot[1]);
        break;
    case 5:
        Mtx_RotateZ(m, rot[2]);
        Mtx_RotateY(m, rot[1]);
        break;
    case 1:
        Mtx_RotateX(m, rot[0]);
        break;
    case 2:
        Mtx_RotateY(m, rot[1]);
        break;
    case 3:
        Mtx_RotateZ(m, rot[2]);
        break;
    case 0:
        break;
    }
    Mtx_SetTrans(m, x, y, z);
}

s32 Model_DrawFade(void *mdl, f32 *a1, u16 flags, void *a3, f32 alpha)
{
    if (mdl == NULL) {
        return 0;
    }
    if (*(s32 *)mdl == -1) {
        return 0;
    }
    Gs_SetState(0x60, 0);
    Gs_SetState(0x1A, (s32) a1);
    if (flags & 1) {
        Gs_SetState(0x67, (u8)(255.0f * alpha) << 24);
    } else {
        Gs_SetState(0x67, ((u8)(255.0f * alpha) << 24) | 0xFFFFFF);
    }
    Gs_SetLightList(a3, mdl);
    Draw_ApplyStates(*(s32 *)((u8 *)mdl + 0x88));
    Draw_SetBlendFlags(flags);
    Model_Draw(*(s32 *)mdl, 0);
    Draw_SetDefaultState();
    return 1;
}

s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4)
{
    if (mdl == NULL) {
        return 0;
    }
    if (*(s32 *)mdl == -1) {
        return 0;
    }
    Gs_SetState(0x60, 0);
    Gs_SetState(0x1A, (s32) a1);
    Gs_SetState(0x67, color);
    Gs_SetLightList(a4, mdl);
    Draw_ApplyStates(*(s32 *)((u8 *)mdl + 0x88));
    Draw_SetBlendFlags(flags);
    Model_Draw(*(s32 *)mdl, 0);
    Draw_SetDefaultState();
    return 1;
}

s32 Effect_DrawModelAdd(void *list, f32 *v, void *base)
{
    if (list == NULL) {
        return 0;
    }
    if (*(s32 *)((u8 *)list + 0) == -1) {
        return 0;
    }
    Gs_SetState(0x60, 0x80);
    Gs_SetState(0x1A, (s32) v);
    Gs_SetLightList(base, list);
    Draw_ApplyStates(*(s32 *)((u8 *)list + 0x88));
    Model_Draw(*(s32 *)((u8 *)list + 0), 0);
    Draw_SetDefaultState();
    return 1;
}

void Draw_SetBlendFlags(u16 flags)
{
    if (flags & 0x10) {
        Gfx_SetState63(0);
    }
    switch (flags & 0xE) {
    case 2:
        Gfx_SetBlend(4, 1);
        break;
    case 4:
        Gfx_SetBlend(4, 5);
        break;
    case 8:
        Gfx_SetBlend(1, 1);
        Gfx_SetBlendOp(2);
        break;
    }
}

void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a)
{
    s32 ofs;

    ofs = gAreaKinds[area & 0xFFFF] * 4;
    *r = *(gAreaKindColors + ofs);
    *g = *(D_0041CF01 + ofs);
    *b = *(D_0041CF02 + ofs);
    *a = *(D_0041CF03 + ofs);
}

s32 Scene_FewPlaced3_Unk(void)
{
    return (s32) gScene.placed3 < 3;
}


s32 Effect_ExistsKind16(void *pl)
{
    u8 *p;
    s16 i;

    i = 0;
    p = (u8 *)&gStageObjs;
    do {
        if (p[0] != 0 && p[2] == 0x16 && p[3] == 1 && *(void **)(p + 0x34) == pl) {
            return 1;
        }
        i++;
        p += 0x40;
    } while (i < 0x80);
    return 0;
}

typedef struct KindRange {
    s16 lo;
    s16 hi;
} KindRange;

extern KindRange gDualSwordRanges[];

s16 Player_DualSwordIndex(void *obj)
{
    Player *p = obj;
    s16 i;
    s16 n;
    s32 kind;

    if (p->weaponType == 6) {
        for (i = 0, n = 0; gDualSwordRanges[i].lo != -1; i++) {
            kind = p->weaponKind;
            if (kind >= gDualSwordRanges[i].lo && kind <= gDualSwordRanges[i].hi) {
                return n + kind - gDualSwordRanges[i].lo;
            }
            n += gDualSwordRanges[i].hi - gDualSwordRanges[i].lo + 1;
        }
    }
    return -1;
}

void Effect_ScaleVec(f32 *v)
{
    Vec_Scale((f32 *) v, (f32 *) v, Effect_Scale());
}

f32 Effect_Scale(void)
{
    return 1.0f;
}

s32 Effect_KindClass_Unk(s32 ownerIndex)
{
    s32 owner;

    owner = ownerIndex & 0xFF;
    switch (owner) {
    case 0x13:
    case 0x18:
    case 0x38:
        return 2;
    case 0x8:
    case 0x22:
        return 6;
    case 0x30:
    case 0x37:
    case 0x42:
    case 0x49:
    case 0x43:
        return 7;
    default:
        return 0;
    }
}

f32 Lure_RandUnit(void)
{
    return 0.001f * (f32) (Rand_Next(1) & 0xFFFF & 0x3FF);
}

f32 Lure_RandSigned(void)
{
    return 0.002f * (f32) ((Rand_Next(1) & 0xFFFF & 0x3FF) - 0x200);
}

extern s16 gEffectTexIds[];

s16 Effect_MapTexId(s16 id)
{
    s16 i;

    if (gScene.online == 0) {
        return id;
    }
    for (i = 0; gEffectTexIds[i] != -1; i++) {
        if (id == gEffectTexIds[i]) {
            return i;
        }
    }
    return -1;
}

/* A key of a sequence (gSeqKeys, 0x1C bytes; a sequence numbers its keys
 * from 1): a colored rectangle (SeqA_Draw) whose alpha (SeqState.unk08,
 * 256 = opaque) goes from unk14 to unk16 by unk18 a frame, after unk1A
 * frames. */
typedef struct SeqKey {
    u8 unk00;           /* 0x00: nonzero on a sequence's last key */
    u8 color;           /* 0x01: an index into the color table */
    u8 unk02[2];
    f32 x;              /* 0x04 */
    f32 y;              /* 0x08 */
    f32 z;              /* 0x0C */
    s16 w;              /* 0x10 */
    s16 h;              /* 0x12 */
    s16 unk14;          /* 0x14: the alpha at the start */
    s16 unk16;          /* 0x16: the alpha at the end */
    s16 unk18;          /* 0x18: added to the alpha each frame */
    s16 unk1A;          /* 0x1A: frames before the alpha moves */
} SeqKey;

/* The two sequence players, gSeqA and gSeqB. */
typedef struct SeqState {
    u8 active;          /* 0x00: 1 running, 2 at the last key */
    u8 unk01;           /* 0x01 */
    u8 unk02[2];
    s16 id;             /* 0x04: the first key (from 1), 0 when none */
    s16 cur;            /* 0x06: the current key */
    s16 unk08;          /* 0x08: the alpha */
    s16 unk0A;          /* 0x0A: the alpha of the previous frame */
    SeqKey *key;        /* 0x0C: the current key */
    s16 unk10;          /* 0x10: frames left before the alpha moves */
} SeqState;

extern SeqKey gSeqKeys[];
extern SeqState gSeqB;
extern SeqState gSeqA;

void Seq_UpdateAll(u8 *task)
{
    SeqState *s;
    SeqKey *e;

    s = &gSeqA;
    switch (task[8]) {
    case 0:
        memset(s, 0, sizeof(SeqState));
        s->key = NULL;
        task[8]++;
    case 1:
        switch (s->active) {
        case 0:
            if (s->id == 0) {
                break;
            }
        start:
            s->active = 1;
            s->cur = s->id;
            s->unk08 = gSeqKeys[s->cur - 1].unk14;
            s->unk10 = gSeqKeys[s->cur - 1].unk1A;
        case 1:
        case 2:
            if (s->id == 0) {
                s->active = 0;
                s->key = NULL;
                break;
            }
            if (s->id != s->cur) {
                goto start;
            }
            e = &gSeqKeys[s->cur - 1];
            s->key = e;
            s->unk0A = s->unk08;
            if (s->unk08 != e->unk16) {
                if (s->unk10 == 0) {
                    s->unk08 += e->unk18;
                } else {
                    s->unk10--;
                }
                break;
            }
            s->unk01 = 0;
            if (e->unk00 != 0) {
                s->active = 0;
                s->id = 0;
                s->key = NULL;
                break;
            }
            if (s->active == 1) {
                s->active++;
            }
            break;
        }
        break;
    }
    s = &gSeqB;
    switch (task[9]) {
    case 0:
        memset(s, 0, sizeof(SeqState));
        s->key = NULL;
        task[9]++;
    case 1:
        switch (s->active) {
        case 0:
            if (s->id == 0) {
                break;
            }
        start2:
            s->active = 1;
            s->cur = s->id;
            s->unk08 = gSeqKeys[s->cur - 1].unk14;
            s->unk10 = gSeqKeys[s->cur - 1].unk1A;
        case 1:
        case 2:
            if (s->id == 0) {
                s->active = 0;
                s->key = NULL;
                break;
            }
            if (s->id != s->cur) {
                goto start2;
            }
            e = &gSeqKeys[s->cur - 1];
            s->key = e;
            s->unk0A = s->unk08;
            if (s->unk08 != e->unk16) {
                if (s->unk10 == 0) {
                    s->unk08 += e->unk18;
                } else {
                    s->unk10--;
                }
                break;
            }
            s->unk01 = 0;
            if (e->unk00 != 0) {
                s->active = 0;
                s->id = 0;
                s->key = NULL;
                break;
            }
            if (s->active == 1) {
                s->active++;
            }
            break;
        }
        break;
    }
}


typedef struct SeqQuad {
    f32 p0[4];
    f32 p1[4];
    u32 color;
} SeqQuad;

extern u32 gSeqAColors[];
extern u32 gSeqBColors[];
struct PrimLineF;
s32 Prim_FillRectF(struct PrimLineF *q);


u8 SeqA_GetState(void)
{
    SeqState *s = &gSeqA;
    SeqKey *e;

    if ((s->active == 0 && s->unk01 == 0) || s->id == 0) {
        return 0;
    }
    e = &gSeqKeys[s->cur - 1];
    if (s->unk08 != e->unk16 || (e->unk00 == 0 && s->active == 1)) {
        return 1;
    }
    return 2;
}

u8 SeqB_GetState(void)
{
    SeqState *s = &gSeqB;
    SeqKey *e;

    if ((s->active == 0 && s->unk01 == 0) || s->id == 0) {
        return 0;
    }
    e = &gSeqKeys[s->cur - 1];
    if (s->unk08 != e->unk16 || (e->unk00 == 0 && s->active == 1)) {
        return 1;
    }
    return 2;
}

void SeqA_Draw(void)
{
    SeqState *s = &gSeqA;
    f32 a;
    SeqQuad q;

    if (s->id == 0) {
        return;
    }
    if (s->key == NULL) {
        return;
    }
    a = (f32)s->unk08 / 256.0f;
    if (a <= 0.0f) {
        return;
    }
    Gfx_SetBlend(4, 5);
    Gs_SetState(0x6D, 7);
    q.p0[0] = s->key->x;
    q.p0[1] = s->key->y;
    q.p0[2] = s->key->z;
    q.p0[3] = 1.0f;
    q.p1[0] = q.p0[0] + s->key->w;
    q.p1[1] = q.p0[1] + s->key->h;
    q.p1[2] = q.p0[2];
    q.p1[3] = 1.0f;
    q.color = ((u32)(255.0f * a) << 24 & 0xFF000000) | (gSeqAColors[s->key->color] & 0xFFFFFF);
    Prim_FillRectF((struct PrimLineF *)&q);
    Gs_SetState(0x6D, 3);
}

void SeqB_Draw(void)
{
    SeqState *s = &gSeqB;
    f32 a;
    SeqQuad q;

    if (s->id == 0) {
        return;
    }
    if (s->key == NULL) {
        return;
    }
    a = (f32)s->unk08 / 256.0f;
    if (a <= 0.0f) {
        return;
    }
    Gfx_SetBlend(4, 5);
    Gs_SetState(0x6D, 7);
    q.p0[0] = s->key->x;
    q.p0[1] = s->key->y;
    q.p0[2] = s->key->z;
    q.p0[3] = 1.0f;
    q.p1[0] = q.p0[0] + s->key->w;
    q.p1[1] = q.p0[1] + s->key->h;
    q.p1[2] = q.p0[2];
    q.p1[3] = 1.0f;
    q.color = ((u32)(255.0f * a) << 24 & 0xFF000000) | (gSeqBColors[s->key->color] & 0xFFFFFF);
    Prim_FillRectF((struct PrimLineF *)&q);
    Gs_SetState(0x6D, 3);
}

void SeqA_Start(s32 id)
{
    SeqState *s = &gSeqA;
    SeqKey *e;
    s32 d;
    s32 n;

    s->id = id;
    e = &gSeqKeys[s->id - 1];
    d = e->unk16 - e->unk14;
    if (d < 0) {
        d = -d;
    }
    n = e->unk18;
    if (n < 0) {
        n = -n;
    }
    s->unk0A = d / n;
    s->cur = s->id;
    s->unk08 = gSeqKeys[s->cur - 1].unk14;
    s->unk10 = gSeqKeys[s->cur - 1].unk1A;
    s->unk01 = 1;
}

void SeqA_Reset(void)
{
    gSeqA.active = 0;
    gSeqA.unk01 = 0;
    gSeqA.id = 0;
    gSeqA.key = NULL;
    gSeqA.unk0A = 0;
}

void SeqB_Start(s32 id)
{
    SeqState *s = &gSeqB;
    SeqKey *e;
    s32 d;
    s32 n;

    s->id = id;
    e = &gSeqKeys[s->id - 1];
    d = e->unk16 - e->unk14;
    if (d < 0) {
        d = -d;
    }
    n = e->unk18;
    if (n < 0) {
        n = -n;
    }
    s->unk0A = d / n;
    s->cur = s->id;
    s->unk08 = gSeqKeys[s->cur - 1].unk14;
    s->unk10 = gSeqKeys[s->cur - 1].unk1A;
    s->unk01 = 1;
}

void SeqB_Reset(void)
{
    gSeqB.active = 0;
    gSeqB.unk01 = 0;
    gSeqB.id = 0;
    gSeqB.key = NULL;
    gSeqB.unk0A = 0;
}

void Fade_Draw(void)
{
    s32 dark;
    s32 a;
    s32 c;
    s32 alpha;
    s32 test;
    ScreenRect r;

    if (gSysSettings.brightness == 0x80) {
        return;
    }
    if (gSysSettings.brightness > 0x80U) {
        a = gSysSettings.brightness - 0x80;
        dark = 0;
        c = 0xFF;
    } else {
        a = 0x80 - gSysSettings.brightness;
        dark = 1;
        a *= 2;
        c = 0;
    }
    if (a == 0) {
        return;
    }
    r.w = 0x280;
    r.y = 0;
    r.h = 0x1C0;
    r.x = 0;
    r.color = (a << 24) | (c << 16) | (c << 8) | c;
    alpha = Gs_GetState(0xD);
    test = Gs_GetState(0x5E);
    Gs_SetState(0xD, 0);
    if (dark == 0) {
        Gs_SetState(0x5E, 0x12);
    } else {
        Gs_SetState(0x5E, 0x32);
    }
    Prim_FillRect((struct PrimLine *)&r);
    Gs_SetState(0xD, alpha);
    Gs_SetState(0x5E, test);
}

void PlayTime_Tick(void);
void func_0036CCE0(void);
void Overlay_LoadGmSub(void);
s32 Scene_RequestPause(s32 mode);
void QNetIf_UpdateReceive(void);
void QNetIf_Flush(void);
void Scene_EnterField(void);
void Scene_QuestStartTask(void);
void Scene_UpdateQuest(void);
void Scene_ResultFade(void);
void func_003274E0(void);
void Scene_QuestFrameTick(void);
void Scene_Opening(void);
s32 Quest_Teardown(s32 arg0);
extern u8 gGmSubLoaded;

void Scene_QuestTask(u8 *obj)
{
    PlayTime_Tick();
    func_0036CCE0();
    switch (obj[8]) {
    case 0:
        if (Game_IsOnline() == 0) {
            gGmSubLoaded = 0;
            Overlay_LoadGmSub();
        }
        Scene_RequestPause(0);
        obj[8]++;
    case 1:
        if (Game_IsOnline() == 1) {
            QNetIf_UpdateReceive();
        }
        switch (gScene.scene) {
        case 0:
            Scene_EnterField();
        case 1:
            Scene_QuestStartTask();
            break;
        case 2:
            Scene_UpdateQuest();
            break;
        case 3:
            Scene_ResultFade();
            break;
        case 4:
            func_003274E0();
            break;
        case 5:
            Scene_QuestFrameTick();
            break;
        case 7:
            Scene_Opening();
            break;
        case 6:
            if (Quest_Teardown((s32)obj) == 1) {
                return;
            }
            break;
        }
        if (Game_IsOnline() == 1) {
            QNetIf_Flush();
        }
        break;
    }
}

void Scene_EnterField(void)
{
    s16 i;

    gScene.step = 0;
    gScene.subStep = 0;
    gScene.scene = 1;
    gScene.allLocal = 1;
    gScene.openingFirst = 0;
    gGalleryTitle[0] = 0;
    for (i = 0; i < 4; i++) {
        gScene.preset[i][0] = 0;
        gScene.preset[i][1] = 0;
        gScene.preset[i][2] = 0;
        gScene.preset[i][3] = 0;
        gScene.preset[i][4] = 0;
        gScene.preset[i][5] = 0;
        gScene.preset[i][6] = 0;
        gScene.preset[i][7] = 0;
    }
    Player_SetMyUserId(gScene.localPlayer);
    Loading_SetMessage(5);
    Scene_LoadOverlay(3);
    Scene_LoadOverlay(0xB);
    TaskSlot_Set(plsel_entry, 6);
}

void Scene_StartQuest(void)
{
    u8 save;
    s32 i;

    if (gScene.allLocal != 0) {
        return;
    }
    save = gSoundLoading;
    Game_ResetSubsystems();
    Gfx_ClearModelBuffers();
    gSoundLoading = save;
    gScene.subStep = 0;
    gScene.step++;
    Quest_Clear();
    gScene.aimReverse = gSysSave.options.aimReverse;
    gScene.camControl = gSysSave.options.camControl;
    gScene.camFollow = gSysSave.options.camFollow;
    for (i = 0; i < gScene.playerCount; i++) {
        gScene.playerGender[i] = gQuestSync.gender[i];
        gScene.presetEquip[i] = gQuestSync.presetEquip[i];
        gScene.weapon[i] = gQuestSync.weapon[i];
        gScene.startItemList[i] = gQuestSync.startItemList[i];
    }
    Packet_SetMode0();
    Notice_ResetQueue();
    Players_UpdateSides();
    Quest_Init();
    Scene_InitSupplyBox();
    if (Quest_IsDrill() == 1) {
        Loading_SetMessage(4);
        gScene.allLocal = 1;
        Scene_LoadOverlay(0xB);
        TaskSlot_Set(Plsel_SelectTask, 6);
    }
    if (Quest_IsTraining() == 1) {
        Scene_LoadOverlay(0xB);
    }
    QSync_SendState(8, gScene.localPlayer);
}

void Scene_StartQuestPlay(void)
{
    s16 i;

    if (gScene.allLocal != 0) {
        if ((gFrameTick & 0xF) == 0) {
            QSync_SendState(8, gScene.localPlayer);
        }
        if (gScene.questPhase == 8) {
            Loading_Stop();
            goto quit;
        }
        return;
    }
    if (Quest_IsDrill() == 1 && gScene.questPhase == 8) {
    quit:
        gScene.step = 0;
        gScene.scene = 5;
        Scene_LoadOverlay(0xB);
        return;
    }
    gSysSettings.plselLoaded = 0;
    for (i = 0; i < gScene.playerCount; i++) {
        gScene.playerGender[i] = gQuestSync.gender[i];
        gScene.presetEquip[i] = gQuestSync.presetEquip[i];
        gScene.weapon[i] = gQuestSync.weapon[i];
        gScene.startItemList[i] = gQuestSync.startItemList[i];
    }
    QNetIf_Ready(0);
    strcpy((char *)gGalleryTitle, QuestInfo_Get(1));
    Loading_SetMessage(5);
    ScEvent_Init();
    if (Quest_IsVersus() == 1) {
        Dmcam_Preload();
    }
    Hud_Start();
    Quest_Setup();
    ObjList_Update();
    SightBlock_ClearAll();
    FlashObj_ClearAll();
    ObjListAF0_Clear();
    ObjListA70_Clear();
    Versus_ConsumeTeamItem();
    for (i = 0; i < 10; i++) {
        gScene.itemUses[i] = 0; /* itemUses and the 6 bytes after */
    }
    Quest_SpawnMonsters(1);
    Packet_SetMode2();
    gSoundLoading = 0;
    gScene.step++;
    QSync_SendState(8, gScene.localPlayer);
}

void Scene_LoadQuestSounds(void)
{
    u8 *buf = gLoadBuf;
    s16 k;
    u32 bank;

    Loading_SetMessage(1);
    switch (gScene.subStep) {
    case 0:
        gScene.subStep++;
        gScene.openingFirst = 0;
        Sound_StopBank(1);
        Sound_SetSpuMapPage(1);
        FlushCache(0);
        Sound_QueueBank(0x10001, buf);
        return;
    case 1:
        if (!FileQueue_IsBusy()) {
            gScene.subStep++;
            Sound_LoadBank((struct SndFile *) buf, 1);
        }
        return;
    case 2:
        if (!Sound_CheckTransfer()) {
            gScene.subStep++;
        }
        return;
    case 3:
        Sound_StopBank(7);
        bank = (gAreaSeBanks[gScene.area] + 0xB) | 0x10000;
        FlushCache(0);
        Sound_QueueBank(bank, buf);
        gScene.subStep++;
        return;
    case 4:
        if (!FileQueue_IsBusy()) {
            gScene.subStep++;
            Sound_LoadBank((struct SndFile *) buf, 7);
        }
        return;
    case 5:
        if (!Sound_CheckTransfer()) {
            gScene.subStep++;
        }
        return;
    case 6:
    case 9:
    case 12:
    case 15:
        gScene.subStep++;
        k = (gScene.subStep - 6) / 3;
        Sound_StopBank(k + 2);
        FlushCache(0);
        SndLoad_Init(2);
        return;
    case 7:
    case 10:
    case 13:
    case 16:
        k = (gScene.subStep - 7) / 3;
        if (SndLoad_QuestBanks(buf, (u8 *)&gPlayers[k])) {
            gScene.subStep++;
            Sound_LoadStaged((struct SndFile *) buf, buf + 0x20000, k + 2);
        }
        return;
    case 8:
    case 11:
    case 14:
    case 17:
        if (!Sound_CheckTransfer()) {
            if (gScene.subStep == 0x11) {
                Sound_StopBank(6);
                SndLoad_Init(4);
            }
            gScene.subStep++;
        }
        return;
    case 18:
        if (SndLoad_ListBanks(buf, gScene.emKind)) {
            gScene.subStep++;
        }
        return;
    case 19:
        if (!FileQueue_IsBusy()) {
            gScene.subStep++;
            Sound_LoadStaged((struct SndFile *) buf, buf + 0x4800, 6);
        }
        return;
    case 20:
        if (!Sound_CheckTransfer()) {
            gSoundLoading = 0;
            gScene.step++;
            gScene.subStep++;
        }
        return;
    }
}

void Scene_StartField(void)
{
    s16 i;

    Loading_SetMessage(1);
    if (Game_IsOnline() == 1 && QSync_Begin() == 0) {
        return;
    }
    Field_UpdateAll();
    gAreaState.drawSky = 1;
    for (i = 0; i < 10; i++) {
        gPlayers[i].visible = gPlayers[i].active;
    }
    gScene.padRead = 1;
    Pad_UpdateInput();
    gScene.emEventFlags = 0;
    gScene.countdownOn = 0;
    gScene.fortStage = 0;
    gScene.peersLeft = 0;
    D_005B1DC0 = 0;
    D_005B1DB8 = 0;
    Quest_ResetTimer();
    gScene.musicFlags = 0;
    gScene.musicState = 0;
    Area_ApplyFog(gScene.area);
    Area_StartObjects(gScene.area);
    Tex_LoadCommon();
    Mem_Compact();
    CamCtl_Update(1);
    SeqA_Start(2);
    Tex_PatchClut_Unk();
    Loading_Stop();
    gGalleryTitle[0] = 0;
    QNetIf_Unready();
    gScene.step++;
}

void Scene_EnterArea(void)
{
    if ((Game_IsOnline() == 1) && (QuestNet_NoPeerReady() == 0)) {
        Loading_Request();
        return;
    }
    gScene.step = 0;
    gScene.scene = 2;
    Bgm_EnterArea(gScene.area);
    Jingle_OnAreaChange(gScene.area);
}

void Scene_QuestStartTask(void)
{
    Pad_UpdateInput();
    switch (gScene.step) {
    case 0:
        Scene_StartQuest();
        break;
    case 1:
        Scene_StartQuestPlay();
        break;
    case 2:
        Scene_LoadQuestSounds();
        break;
    case 3:
        Scene_StartField();
        break;
    case 4:
        Scene_EnterArea();
        break;
    }
}

void Scene_UpdateQuest(void)
{
    Player *p = &gPlayers[gScene.localPlayer];
    u8 *buf;
    u32 bank;

    if (Game_IsOnline() == 1) {
        QSync_UpdatePlayerPos();
    }
    switch (gScene.step) {
    case 0:
    case 1:
        ScEvent_Update();
        if (Scene_FieldFrame() != 0) {
            break;
        }
        if (Game_PauseState() > 1) {
            break;
        }
        Notice_Update();
        Quest_Update();
        if (Quest_PhaseIsOver(2) == 1) {
            gScene.step = 0;
            gScene.scene = 3;
            p->aimLockTimer = 0;
            p->aimMode = 0;
            p->unk393[0x395 - 0x393] = 0;
            *(s16 *)&p->unk6B1[0x6B2 - 0x6B1] = 0;
            p->hitStop = 0;
            p->hitStopAdd = 0;
            p->teamItemMask = 0;
            p->actRequest = 0;
            p->offerRequest = 0;
            if (gScene.questPhase == 8) {
                gScene.step = 0;
                gScene.scene = 5;
                Scene_LoadOverlay(0xB);
            }
            break;
        }
        if (p->areaMoveState != 0) {
            gScene.step++;
            if (gScene.step == 2) {
                Sound_Silence();
                Sound_ReverbOff();
                gScene.subStep = 10;
                Stream_FadeOut(0, 10);
                Stream_Stop(1);
            }
        } else {
            gScene.step = 0;
        }
        break;
    case 2:
        Pad_UpdateInput();
        if ((gScreenCaptureOk & 0x7F) == 1 && ScreenTex_Reserve(gReadBuf) != 0) {
            gScreenCaptureOk = (gScreenCaptureOk & 0x80) | 2;
        }
        if (gScene.subStep != 0) {
            gScene.subStep--;
            Map_DrawMain_Unk();
            break;
        }
        QNetIf_Ready(1);
        gSoundLoading = 1;
        Vib_StopAll();
        Stream_Pause(0, 1);
        Area_FreeAll();
        QuestEm_LeaveArea(p->area, p->nextArea);
        MinePoint_RemoveTemporary();
        gScene.area = (u8)p->nextArea;
        p->area = p->nextArea;
        Mem_Compact();
        Area_Load(gScene.area);
        Obj_FreeAll();
        LightSet_UpdateAll();
        FreeList_InitAll();
        DrawSlot_ResetAll();
        Light_Reset();
        Quest_SpawnMonsters(0xFF);
        Area_Enter(gScene.area);
        gAreaState.drawSky = 1;
        Area_ApplyFog(gScene.area);
        Area_StartObjects(gScene.area);
        Em_RunAll();
        QuestEm_SpawnArea(gScene.area);
        Sound_StopBank(6);
        SndLoad_Init(4);
        gSoundLoading = 0;
        gScene.step++;
        Loading_Request();
        break;
    case 3:
        Pad_UpdateInput();
        buf = gLoadBuf;
        gSoundLoading = 1;
        if (SndLoad_ListBanks(buf, gScene.emKind)) {
            Sound_LoadStaged((struct SndFile *) buf, buf + 0x4800, 6);
            gScene.step++;
        }
        gSoundLoading = 0;
        Loading_Request();
        break;
    case 4:
        Pad_UpdateInput();
        if (!Sound_CheckTransfer()) {
            gScene.step++;
            Sound_StopBank(7);
            bank = (gAreaSeBanks[gScene.area] + 0xB) | 0x10000;
            FlushCache(0);
            Sound_QueueBank(bank, (*(u8 * *)&gSysSettings.dataBuf));
        }
        Loading_Request();
        break;
    case 5:
        Pad_UpdateInput();
        if (!FileQueue_IsBusy()) {
            gScene.step++;
            Sound_LoadBank((struct SndFile *) ((*(u8 * *)&gSysSettings.dataBuf)), 7);
        }
        Loading_Request();
        break;
    case 6:
        Pad_UpdateInput();
        if (!Sound_CheckTransfer()) {
            gScene.step = 0;
            Player_InitAll(1);
            CamCtl_Update(1);
            SeqA_Start(2);
            Tex_PatchClut_Unk();
            QNetIf_Unready();
            if ((gScreenCaptureOk & 0x7F) == 2 && ScreenTex_Capture(1) != 0) {
                gScreenCaptureOk = (gScreenCaptureOk & 0x80) | 1;
            }
            Bgm_EnterArea(gScene.area);
        }
        Loading_Request();
        break;
    }
}

void Scene_ResultFade(void)
{
    SceneState *sc = &gScene;
    SprDesc spr;

    Pad_UpdateInput();
    switch (gScene.step) {
    case 0:
        sc->step++;
        sc->timer = 0;
        sc->timer2 = 0;
        break;
    case 1:
        if (++sc->timer >= 120) {
            sc->step++;
        }
        break;
    case 2:
        sc->timer += 8;
        if (sc->timer >= 0xC0) {
            sc->step++;
            sc->timer = 0xC0;
            sc->timer2 = 0;
            Tex_LoadResultTex_Unk();
            AreaObj_K17_Create(gScene.questPhase);
        }
        break;
    case 3:
        if (++sc->timer2 >= 150) {
            sc->step++;
        }
        break;
    case 4:
        sc->scene = 5;
        Scene_LoadOverlay(0xB);
        sc->step = 0;
        gQuestSync.ready[gScene.localPlayer] = 1;
        if (Game_IsOnline() != 0) {
            QSync_SendState(2, gScene.localPlayer);
        }
        return;
    }
    spr.tex = 0;
    spr.pos[0] = 0.0f;
    spr.pos[1] = 0.0f;
    spr.pos[2] = 0.0f;
    spr.size[0] = 640.0f;
    spr.size[1] = 448.0f;
    spr.scale[0] = 1.0f;
    spr.scale[1] = 1.0f;
    spr.alpha = (f32)sc->timer / 255.0f;
    spr.unk34 = 0;
    spr.flags = 5;
    spr.color = 0xFF703A18;
    Sprite_Queue((struct SprDef *)&spr);
    Notice_Update();
    Field_UpdateAll();
    Frame_Draw();
}

void func_003274E0(void)
{
}

void Scene_QuestFrameTick(void)
{
    s16 frame;

    func_D683C0();
    frame = gScene.stateSyncFrame + 1;
    gScene.stateSyncFrame = frame;
    if (frame >= 0x3C) {
        gScene.stateSyncFrame = 0;
        QSync_SendState(8, gScene.localPlayer);
    }
    Frame_Draw();
}

s32 Scene_FieldFrame(void)
{
    Pad_UpdateInput();
    if (Game_PauseState() < 2) {
    draw:
        Field_UpdateAll();
        Frame_Draw();
        Hit_ClearStatusAll();
        return 0;
    }
    if (Game_PauseState() == 4) {
        gScene.pauseState = 0;
        goto draw;
    }
    ItemHud_Update();
    Frame_DrawOverCapture();
    return 0;
}

void PlayTime_Tick(void)
{
    gScene.frame += 1;
    if (Game_IsOnline() == 0) {
        if (Game_PauseState() < 2) {
            goto block_3;
        }
    } else {
block_3:
        if (((s32) gScene.frame % 30) == 0) {
            gSave.playTime += 1;
            if (gSave.playTime > 35999999U) {
                gSave.playTime = 0x022550FF;
            }
        }
    }
}

s32 Scene_RequestPause(s32 mode)
{
    s32 st;

    if (Game_IsOnline() == 1) {
        gScene.pauseState = 0;
        return 0;
    }
    if (Quest_PhaseIsOver(0) == 1) {
        gScene.pauseState = 0;
        return 0;
    }
    st = gScene.pauseState;
    if (mode == 0 && st >= 2 && st < 4) {
        gScene.pauseState = 4;
        return 1;
    }
    if (mode == 1 && st == 0) {
        gScene.pauseState = 1;
        return 1;
    }
    return 0;
}

s32 Game_PauseState(void)
{
    return (s32) gScene.pauseState;
}

s32 Quest_Teardown(s32 unused)
{
    gSysSettings.plselLoaded = 0;
    QuestSync_Clear();
    Stream_Reset();
    Sound_Silence();
    Sound_ReloadBank1();
    if (Game_IsOnline() != 0) {
        gScene.step = 0;
        Quest_TeardownNet();
        return 1;
    }
    gQuest.flags &= 0xFFDFFFFF;
    Game_ResetSubsystems();
    gScene.step = 1;
    Quest_TeardownNet();
    return 1;
}

void Quest_TeardownNet(void)
{
    TaskSlot2_EndCode1();
}

void Scene_Opening(void)
{
    SceneState *sc = &gScene;
    s32 st;
    char **lines;
    s32 y;
    s32 x;

    switch (gScene.step) {
    case 0:
        if (sc->subStep == 0) {
            Save_SetEventFlag(0x185);
            Movie_Play(7);
            EventFlag_Set(0x25);
        } else {
            Movie_Play(8);
            if (EventFlag_Test(0x26) == 0) {
                sc->openingFirst = 1;
            } else {
                sc->openingFirst = 0;
            }
            EventFlag_Set(0x26);
            EventFlag_Set(0x31);
            EventFlag_Set(0x32);
            EventFlag_Set(0x33);
            EventFlag_Set(0x34);
        }
        sc->step++;
        sc->timer = 0;
        break;
    case 1:
        Movie_Update();
        st = Movie_GetStatus();
        if (st != 3) {
            sc->timer++;
            if (st != 2 || !(D_006AE0B4 & 0x8000)) {
                break;
            }
        }
        SeqA_Start(1);
        sc->step++;
        sc->timer = 30;
        Movie_RequestStop();
        break;
    case 2:
        Movie_Update();
        if (SeqA_GetState() == 1) {
            break;
        }
        if (Movie_GetStatus() == 0) {
        stop:
            Movie_Stop();
            if (sc->subStep == 0) {
                SeqA_Start(2);
                sc->scene = 6;
                sc->step = 0;
                sc->subStep = 0;
            } else {
                sc->step++;
                Ending_Init();
            }
        } else {
            if (sc->timer == 0) {
                goto stop;
            }
            sc->timer--;
        }
        break;
    case 3:
        if (func_D64D40() != 0) {
            break;
        }
        if (sc->openingFirst == 1) {
            SeqA_Reset();
            sc->step++;
            sc->timer = 150;
        } else {
            SeqA_Start(2);
            sc->scene = 6;
            sc->step = 0;
            sc->subStep = 0;
        }
        break;
    case 4:
        if (--sc->timer != 0) {
            if (sc->timer < 90 && (D_006AE0B4 & 0x20)) {
                sc->timer = 1;
            }
            lines = gAwardNoticeLines[5];
            Font_SetSpacing(0x12, 0x12);
            Text_SetColor(0);
            for (y = 0xA0; *lines != NULL; lines++, y += 0x16) {
                x = Str_Length(*lines);
                x = (0x280 - x * 9) / 2;
                Font_SetPos(x, y);
                Text_Printf(*lines);
            }
            Text_DrawAllPages();
        } else {
            SeqA_Start(2);
            sc->scene = 6;
            sc->step = 0;
            sc->subStep = 0;
            return;
        }
        break;
    }
}

void Scene_ResetAll(void)
{
    memset(&gScene, 0, 0x2BCU);
    Model_FreeAll();
    func_00368ED0();
    Mem_Compact();
    gScene.frame = 0;
    gScene.localPlayer = 0;
    D_006ADB48 = 1;
    gScene.padIndex[1] = 1;
    *gScene.padIndex = 0;
    gScene.online = 0;
    gScene.charIndex = gQuestSync.charIndex;
    CamCtl_Clear();
}

void Hud_Start(void)
{
    Hud_Init();
    CamReq_Reset();
}

void Quest_Setup(void)
{
    s32 i;

    Game_ResetSubsystems();
    Tex_LoadFile(6, 0x154, 0);
    Area_Load(gScene.startArea);
    Anim_LoadCommon(0);
    Quest_InitPlayers();
    Players_UpdateSides();
    for (i = 0; i < gScene.playerCount; i++) {
        if (gScene.inQuest[i] == 1) {
            Player_SetupQuest(i);
        } else {
            gPlayers[i].active = 0;
            gPlayers[i].visible = 0;
        }
    }
    Hud_LoadTextures();
    Quest_LoadCommonModels();
    func_00327F70();
    func_003350B0();
    func_0033A360();
}

void Player_SetupQuest(s32 idx)
{
    Player *p;

    p = &gPlayers[idx];
    Anim_LoadPlayer(idx, (s32) p->weaponType);
    Player_InitPartTable(idx);
    Player_InitForQuest(p);
}

void Quest_LoadCommonModels(void)
{
    s32 kind;
    s32 m;
    s32 m2;

    kind = gScene.online == 0 ? 0 : 5;
    Load_CommonModelPack(kind);
    m = Model_Alloc(1);
    if (m < 0) {
        return;
    }
    gStageModelSets[0] = Model_Get(m);
    Model_Reserve(m, 1);
    Model_SetupWithPack(m, (*(s32 *)&gSysSettings.modelPack), 0xF3, gCommonModelTexFiles[kind], 0xB00, 0);
    Load_CommonModelPack(1);
    m2 = Model_Alloc(1);
    if (m2 < 0) {
        return;
    }
    gStageModelSets[1] = Model_Get(m2);
    Model_Reserve(m2, 1);
    Model_SetupWithPack(m2, (*(s32 *)&gSysSettings.modelPack), 0x106, gCommonModelTexFiles[1], 0x900, 1);
    Load_CommonModelPack(2);
    m2 = Model_Alloc(1);
    if (m2 < 0) {
        return;
    }
    gStageModelSets[2] = Model_Get(m2);
    Model_Reserve(m2, 1);
    Model_SetupWithPack(m2, (*(s32 *)&gSysSettings.modelPack), 0x105, gCommonModelTexFiles[2], 0x900, 1);
    Load_CommonModelPack(4);
    m2 = Model_Alloc(1);
    if (m2 < 0) {
        return;
    }
    gStageModelSets[4] = Model_Get(m2);
    Model_Reserve(m2, 1);
    Model_SetupWithPack(m2, (*(s32 *)&gSysSettings.modelPack), 0x103, gCommonModelTexFiles[4], 0x800, 1);
}

void func_00327F70(void)
{
}

void Quest_SpawnMonsters(s32 mode)
{
    s16 i;
    s16 n;
    s16 id;
    s16 j;
    u8 cnt[4];
    Em *em;

    Pad_ResetInput();
    func_00328660();
    Light_Reset();
    Area_Enter(gScene.area);
    SeqA_Reset();
    SeqB_Reset();
    gScene.pauseState = 0;
    gCameraPreset = 0xFF;
    gScene.heldMask = 0;
    Camera_SetPreset(0);
    CamCtl_Init();
    if (mode == 0xFF) {
        return;
    }
    if (gScene.area == 0xD) {
        func_00364BE0(0x64);
        func_00364BE0(0x65);
        func_00364BE0(0x66);
    } else {
        func_00364BD0(0x64);
        func_00364BD0(0x65);
        func_00364BD0(0x66);
        func_00364BD0(0x67);
        func_00364BD0(0x68);
    }
    if (gScene.questId != 0) {
        Quest_EnterArea(gScene.area);
        return;
    }
    if (Debug_IsOn(0x17) == 0) {
        GmLoad_Reset();
        GmLoad_LoadStagePartition();
        cnt[0] = 0;
        cnt[1] = 0;
        cnt[2] = 0;
        cnt[3] = 0;
        id = 0;
        for (i = 0; i < 4; i++) {
            gScene.emKind[i] = gQuestSync.emKind[i];
            if (gScene.emKind[i] != 0) {
                GmLoad_LoadEmData(gScene.map, gScene.emKind[i]);
                EmSlot_LoadModels(i);
                for (n = 0; n < gQuestSync.emCount[i]; n++, id++) {
                    em = EmSlot_Alloc();
                    if (em != NULL) {
                        em->modelSlot = i;
                        em->type = gScene.emKind[em->modelSlot];
                        em->variant = cnt[i]++;
                        em->index = id;
                        em->sizePct = 100;
                        em->area = gScene.area;
                    }
                }
            }
        }
        return;
    }
    gScene.emKind[0] = 0xB;
    gScene.emKind[1] = 0xA;
    gScene.emKind[2] = 3;
    gScene.emKind[3] = 4;
    for (j = 0; j < 4; j++) {
        EmSlot_LoadModels(j);
    }
    for (j = 0; j < 2; j++) {
        if (Debug_IsOn(0x74) != 0) {
            if (gScene.area != 0) {
                continue;
            }
        } else if (j != 0) {
            continue;
        }
        em = EmSlot_Alloc();
        if (em == NULL) {
            continue;
        }
        if (j == 0) {
            em->modelSlot = 0;
        } else if (j > 0 && j <= 2) {
            em->modelSlot = 1;
        } else if (j > 2 && j <= 4) {
            em->modelSlot = 2;
        } else {
            em->modelSlot = 3;
            if (j == 5) {
                em->unk436 = 1;
            } else {
                em->unk436 = 0;
            }
        }
        em->modelSlot = 0;
        em->type = gScene.emKind[em->modelSlot];
        if ((em->type == 1 || em->type == 0x2A || em->type == 0x25) && gScene.area == 0xB) {
            em->variant = 1;
        } else {
            em->variant = 0;
        }
        em->index = j;
        em->area = gScene.area;
    }
}

void Quest_InitPlayers(void)
{
    s16 i;
    Player *p;

    memset(gPlayerSys, 0, 0x10);
    for (i = 0; i < 4; i++) {
        p = &gPlayers[i];
        memset(p, 0, sizeof(Player));
        if (i >= gScene.playerCount) {
            continue;
        }
        if (gScene.inQuest[i] == 1) {
            p->active = 1;
        } else {
            p->active = 0;
            continue;
        }
        p->index = i;
        p->gender = gScene.playerGender[i];
        p->presetEquip = gScene.presetEquip[i];
        p->startItemList = gScene.startItemList[i];
        Quest_SetStartArea(i);
        if (Game_IsOnline() == 0 && i > 0) {
            p->equip[6] = gScene.weapon[i];
            p->weaponKind = Equip_GetKind(&gScene.weapon[i]);
            sprintf((char *)p->name, TXT(0x0046CE10) /* a default player name, with the number */, i + 1);
        } else if (gScene.presetEquip[i] == 0 || gSysSettings.useSavedEquip != 0) {
            if (Player_IsLocal(p) == 1) {
                Player_LoadFromSave(p);
                Player_LoadName(p);
            } else {
                Player_LoadPreset(gScene.preset[i], p);
            }
        } else {
            p->equip[6] = gScene.weapon[i];
            p->weaponKind = Equip_GetKind(&gScene.weapon[i]);
            sprintf((char *)p->name, TXT(0x0046CE10) /* a default player name, with the number */, i + 1);
        }
        if (Quest_IsDrill() == 1) {
            func_D63170(p, i);
        }
        if (Quest_IsTraining() == 1) {
            func_D633F0(p, i);
        }
        Player_CalcSkills(p);
        p->weaponType = gWeaponKindClass[p->weaponKind];
        p->isGunner = WeaponClass_IsGunner(p->weaponType);
        p->bladeSpecial = Equip_GetBladeSpecial(&p->equip[6]);
        p->weaponFlags = WeaponType_Flags(p->weaponType);
        if (Player_IsLocal(p) == 1) {
            Player_BackupPouch(p, 0);
        }
    }
}

void func_00328660(void)
{
}

void Light_Reset(void)
{
    memset(&gLightSets, 0, 0x290U);
    Light_InitAll((s32) gScene.area);
}

void Pad_ResetInput(void)
{
    memset(&gPadReplay, 0, 0x20U);
    gPadInput.buttons[0].cur = 0;
    gPadInput.buttons[0].prev = 0;
    gPadInput.stickDir[0].cur = 0;
    gPadInput.stickDir[0].prev = 0;
    gPadInput.stickAngle[0].cur = 0;
    gPadInput.stickAngle[0].prev = 0;
    gPadInput.stickTilt[0].cur = 0;
    gPadInput.stickTilt[0].prev = 0;
    gPadInput.buttons[1].cur = 0;
    gPadInput.buttons[1].prev = 0;
    gPadInput.stickDir[1].cur = 0;
    gPadInput.stickDir[1].prev = 0;
    gPadInput.stickAngle[1].cur = 0;
    gPadInput.stickAngle[1].prev = 0;
    gPadInput.stickTilt[1].cur = 0;
    gPadInput.stickTilt[1].prev = 0;
}

void Area_Load(s32 area)
{
    AreaState *as = &gAreaState;
    s32 m;
    s32 tex;

    gScene.area = area;
    as->area = area;
    Load_AreaPack(gScene.area);
    Area_LoadTables(gScene.area);
    m = Model_Alloc(1);
    if (m >= 0) {
        as->modelSlot = m;
        as->modelCount = 1;
        as->model = Model_Get(m);
        Model_Reserve(m, 1);
        switch (gScene.area) {
        case 0x83:
            if (gScene.online != 0 && ElderRaid_InState1() == 1) {
                tex = D_0042582C;
            } else {
                tex = gArea83TexFiles;
            }
            break;
        case 0x84:
            if (gScene.online != 0 && ElderRaid_InState1() == 1) {
                tex = D_00425834;
            } else {
                tex = gArea84TexFiles;
            }
            break;
        default:
            tex = gAreaTexFiles[gScene.area];
            break;
        }
        Model_SetupTexPack(m, (*(s32 *)&gSysSettings.modelPack), 0x108, tex, 0x100);
        Area_LoadSubModels(gScene.area);
        Area_LoadCamData(gScene.area);
    }
}

void Anim_LoadCommon(s32 i)
{
    Load_CommonAnim(i);
    Anim_SetupCommon();
}

void Anim_LoadPlayer(s32 idx, s32 i)
{
    Load_PlayerAnim(idx, i);
    Anim_SetupPlayer(idx);
}

void Anim_LoadEm(s32 idx, s32 arg1)
{
    Load_EmAnim(idx, arg1);
    Anim_SetupEm(idx, arg1);
}

void Anim_LoadEmSlot(s32 i, s32 j, s32 idx)
{
    Load_EmSlotAnim(i, j);
    Anim_SetupEmSlot(idx);
}

void Area_LoadTables(s32 id)
{
    u8 *buf;
    void *dst;

    buf = gLoadBuf;
    LoadCompressedFile(buf, gAreaTableFiles[id]);
    dst = gWorkArea;
    memcpy(dst, (void *)Pack_GetPart(buf, 0), Pack_PartSize(buf, 0));
    AreaTable_Relocate(dst);
    dst = gSysSettings.extraBuf;
    memcpy(dst, (void *)Pack_GetPart(buf, 1), Pack_PartSize(buf, 1));
    AreaTable2_Relocate(dst);
}

void AreaTable_Relocate(u8 *file)
{
    DataHdr *h = (DataHdr *)(file + 8);
    u32 i;
    u32 j;
    s32 k;
    s32 **pp;

    gAreaGridCellX = h->unk00;
    gAreaGridCellZ = h->unk04;
    gAreaGridRows = h->rows;
    gAreaGridCols = h->cols;
    gAreaGridCells = (s32 **)((u8 *)h + h->tblOff);
    gAreaGridData = (u32)((u8 *)h + h->dataOff);
    for (i = 0; i < h->rows; i++) {
        for (j = 0; j < h->cols; j++) {
            pp = &gAreaGridCells[i * h->cols + j];
            *pp = (s32 *)((u8 *)*pp + (u32)h);
            for (k = 0; (*pp)[k] != -1; k++) {
                (*pp)[k] += gAreaGridData;
            }
        }
    }
}

void AreaTable2_Relocate(u8 *file)
{
    DataHdr *h = (DataHdr *)(file + 8);
    u32 i;
    u32 j;
    s32 k;
    s32 **pp;

    gAreaGrid2CellX = h->unk00;
    gAreaGrid2CellZ = h->unk04;
    gAreaGrid2Rows = h->rows;
    gAreaGrid2Cols = h->cols;
    gAreaGrid2Cells = (s32 **)((u8 *)h + h->tblOff);
    gAreaGrid2Data = (u32)((u8 *)h + h->dataOff);
    for (i = 0; i < h->rows; i++) {
        for (j = 0; j < h->cols; j++) {
            pp = &gAreaGrid2Cells[i * h->cols + j];
            *pp = (s32 *)((u8 *)*pp + (u32)h);
            for (k = 0; (*pp)[k] != -1; k++) {
                (*pp)[k] += gAreaGrid2Data;
            }
        }
    }
}

/* unk_3240E0's .data (0x41CF00-0x41D1F0) */
u8 gAreaKindColors[80] = {
#include "sub_main/0041CF00.inc"
};
KindRange gDualSwordRanges[12] = {
#include "sub_main/0041CF50.inc"
};
s16 gEffectTexIds[44] = {
#include "sub_main/0041CF80.inc"
};
#pragma explicit_zero_data on
u32 D_0041CFD8[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
SeqKey gSeqKeys[15] = {
#include "sub_main/0041CFE0.inc"
};
u32 gSeqAColors[12] = {
#include "sub_main/0041D190.inc"
};
u32 gSeqBColors[12] = {
#include "sub_main/0041D1C0.inc"
};
