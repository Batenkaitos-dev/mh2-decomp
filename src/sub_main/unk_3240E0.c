/*
 * The scene and the quest's start (sub_main): Scene_QuestTask, the task that
 * runs the scene gScene.scene (SCENE_* in scene.h) every frame, and the
 * scenes' steps: entering the field and starting a quest (Scene_EnterField,
 * Scene_QuestStartTask and its QSTART_* steps: the players, the models, the
 * sounds, the monsters), the field (Scene_UpdateQuest, with the area
 * change), the quest's end (Scene_ResultFade), the results and the opening
 * movie (Scene_Opening), and the offline pause and the play time.
 *
 * With them: the pool of stage objects (StageObj_*, include/stageobj.h), the
 * keyframe helpers the effects use (Key_Lerp*, Color_EvalKeys), the material
 * set draws of the effects (Model_DrawFade, Effect_DrawModel*), the two
 * screen fades (FadeA_*, FadeB_*, ScreenFade_Task; FADE_* in scene.h), the
 * brightness overlay, and the loading of an area's model and grids
 * (Area_Load, Area_LoadTables).
 *
 * The quest's own state is gQuest (quest.h, unk_3A1070.c); the scene state
 * is gScene (scene.h).
 */
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
void Task_EndGmSubForNetScene(void);
void *Hud_Init(void);
void Jingle_OnAreaChange(u16 area);
void UnreadFlags_ClearAll(void);
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
void Plsel_ResultsUpdate(void);
void * memset(void *p, s32 c, u32 n);
extern u8 gAreaKindColors[];
extern u8 D_0041CF01[];
extern u8 D_0041CF02[];
extern u8 D_0041CF03[];
extern s16 gAreaKinds[];
extern u8 gLightSets;
extern u16 gNextQuestId;
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
void Quest_SetupStub(void);
void Quest_EmptyHook(void);
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
extern s32 gArea83TexFiles[2];
extern s32 gArea84TexFiles[2];
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
void Plsel_ApplyPresetEquip(Player *p, s32 i);
/* unit's own declaration: with the definition's void (Player *) (plsel/plsel.c), Quest_InitPlayers: 4 of 178 words differ (the game passes the player's index too) */
void Plsel_ApplyStageEquip(Player *p, s32 i);
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
s32 Ending_Update(void);
void Font_SetSpacing(u8 a, u8 b);
void Text_SetColor(s16 color);
s32 Str_Length(char *s);
void Font_SetPos(s16 x, s16 y);
void Text_Printf(char *fmt, ...);
void Text_DrawAllPages(void);
u8 FadeA_GetState(void);
void FadeA_Reset(void);
extern u8 gCameraPreset;
void Pad_ResetInput(void);
void Area_ResetStub(void);
void Light_Reset(void);
void Area_Enter(u16 area);
void FadeB_Reset(void);
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
#define STAGEOBJ_COUNT 0x80             /* the pool's objects */
extern StageObj gStageObjs[STAGEOBJ_COUNT];
extern StageObj *gStageObjList;    /* the objects in use */

/* Clears the pool and stacks every object as free. */
void StageObj_InitPool(void)
{
    s32 i;
    StageObj *obj;

    memset(gStageObjs, 0, sizeof(gStageObjs));
    gStageObjList = NULL;
    obj = &gStageObjs[STAGEOBJ_COUNT - 1];
    gStageObjFreeTop = (StageObj **)gStageObjs;
    for (i = 0; i < STAGEOBJ_COUNT; i++, obj--) {
        *--gStageObjFreeTop = obj;
    }
    gScene.placed3 = 0;
    gStageObjFreeCount = STAGEOBJ_COUNT;
}

/* Frees every object in use but those marked keep. */
void StageObj_FreeUnkept(void)
{
    StageObj *obj = gStageObjList;
    StageObj *dead;

    while (obj != NULL) {
        if (obj->active != 0 && obj->keep == 0) {
            dead = obj;
            obj = obj->next;
            StageObj_Free(dead);
        } else {
            obj = obj->next;
        }
    }
}

/* Takes a free object with n blocks of the object buffer, at the head of the
 * list. */
StageObj *StageObj_Alloc(s32 blocks)
{
    s32 block;
    StageObj *obj;

    if ((block = ObjBuf_FindFree(blocks)) < 0) {
        return NULL;
    }
    if (gStageObjFreeCount == 0) {
        return NULL;
    }
    gStageObjFreeCount--;
    obj = *gStageObjFreeTop++;
    obj->prev = NULL;
    obj->next = gStageObjList;
    gStageObjList = obj;
    if (obj->next != NULL) {
        obj->next->prev = obj;
    }
    obj->active = 1;
    obj->keep = 0;
    obj->buf = ObjBuf_GetBlock(block);
    obj->blk = block;
    obj->nblk = blocks;
    ObjBuf_MarkUsed(block, blocks);
    return obj;
}

/* The same, at the tail of the list. */
StageObj *StageObj_AllocTail(s32 blocks)
{
    s32 block;
    StageObj *obj;
    StageObj *last;

    if ((block = ObjBuf_FindFree(blocks)) < 0) {
        return NULL;
    }
    if (gStageObjFreeCount == 0) {
        return NULL;
    }
    gStageObjFreeCount--;
    obj = *gStageObjFreeTop++;
    if (gStageObjList == NULL) {
        gStageObjList = obj;
        obj->prev = NULL;
    } else {
        last = gStageObjList;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = obj;
        obj->prev = last;
    }
    obj->next = NULL;
    obj->active = 1;
    obj->keep = 0;
    obj->buf = ObjBuf_GetBlock(block);
    obj->blk = block;
    obj->nblk = blocks;
    ObjBuf_MarkUsed(block, blocks);
    return obj;
}

/* Unlinks an object and gives it and its blocks back. */
void StageObj_Free(StageObj *obj)
{
    if (obj->prev == NULL) {
        gStageObjList = obj->next;
    } else {
        obj->prev->next = obj->next;
    }
    if (obj->next != NULL) {
        obj->next->prev = obj->prev;
    }
    gStageObjFreeCount++;
    *--gStageObjFreeTop = obj;
    ObjBuf_MarkFree(obj->blk, obj->nblk);
    memset(obj, 0, 8);
    obj->slot = 0;
    *(s32 *)&obj->node = 0;
}

/* Runs the update of every object in use. */
void StageObj_UpdateAll(void)
{
    StageObj *obj;

    for (obj = gStageObjList; obj != NULL; obj = obj->next) {
        if (obj->active != 0) {
            ((void (*)(StageObj *))obj->update)(obj);
        }
    }
}

/* Draws the visible objects, but kind 4 type 0 (drawn by StageObj_DrawKind4). */
void StageObj_DrawAll(void)
{
    StageObj *obj;

    for (obj = gStageObjList; obj != NULL; obj = obj->next) {
        if (obj->draw != NULL && obj->active != 0 && obj->visible != 0) {
            if (obj->kind != 4 || obj->type != 0) {
                ((void (*)(StageObj *))obj->draw)(obj);
            }
        }
    }
}

/* Draws the visible objects of kind 4 type 0. */
void StageObj_DrawKind4(void)
{
    StageObj *obj;

    for (obj = gStageObjList; obj != NULL; obj = obj->next) {
        if (obj->draw != NULL && obj->active != 0 && obj->visible != 0 && obj->kind == 4 && obj->type == 0) {
            ((void (*)(StageObj *))obj->draw)(obj);
        }
    }
}

/*
 * Keys {time, x, y, z}, ended by a time of -1.0f: the vector at `time`, by
 * linear interpolation between the two keys around it; a key's own time gives
 * its value, and a time past the last key the last key's value.
 */
void Key_LerpVec3(f32 *keys, f32 *out, f32 time)
{
    f32 frac;
    f32 nextTime;
    f32 keyTime;

    for (;; keys += 4) {
        keyTime = keys[0];
        if (keyTime == -1.0f) {
            break;
        }
        if (time == keyTime) {
            out[0] = keys[1];
            out[1] = keys[2];
            out[2] = keys[3];
            return;
        }
        if (time > keyTime && time < (nextTime = keys[4])) {
            frac = (time - keyTime) / (nextTime - keyTime);
            out[0] = keys[1] + frac * (keys[5] - keys[1]);
            out[1] = keys[2] + frac * (keys[6] - keys[2]);
            out[2] = keys[3] + frac * (keys[7] - keys[3]);
            return;
        }
    }
    out[0] = keys[-3];
    out[1] = keys[-2];
    out[2] = keys[-1];
}

/*
 * Keys {time, x, y}, ended by a time of -1.0f: x and y at `time`, as
 * Key_LerpVec3 does, with out[2] set to 1. Past the last key the game writes
 * the key's y to out[2], not out[1]. The volatile keeps the store of 1.0f
 * inside the loop, where the game has it.
 */
void Key_LerpVec2(f32 *keys, volatile f32 *out, f32 time)
{
    f32 frac;
    f32 nextTime;
    f32 keyTime;
    for (;; keys += 3) {
        out[2] = 1.0f;
        keyTime = keys[0];
        if (keyTime == -1.0f) {
            break;
        }
        if (time == keyTime) {
            out[0] = keys[1];
            out[1] = keys[2];
            return;
        }
        if (time > keyTime && time < (nextTime = keys[3])) {
            frac = (time - keyTime) / (nextTime - keyTime);
            out[0] = keys[1] + frac * (keys[4] - keys[1]);
            out[1] = keys[2] + frac * (keys[5] - keys[2]);
            return;
        }
    }
    out[0] = keys[-2];
    out[2] = keys[-1];
}

/* Keys {time, value}, ended by a time of -1.0f: the value at `time`, as
 * Key_LerpVec3 does. */
void Key_LerpFloat(f32 *keys, f32 *out, f32 time)
{
    f32 frac;
    f32 nextTime;
    f32 keyTime;

    for (;; keys += 2) {
        keyTime = keys[0];
        if (keyTime == -1.0f) {
            break;
        }
        if (time == keyTime) {
            out[0] = keys[1];
            return;
        }
        if (time > keyTime && time < (nextTime = keys[2])) {
            frac = (time - keyTime) / (nextTime - keyTime);
            out[0] = keys[1] + frac * (keys[3] - keys[1]);
            return;
        }
    }
    out[0] = keys[-1];
}

/*
 * Colour keys {time, 0xAARRGGBB}, ended by a time of -1: the colour at
 * `time`, each channel interpolated between the two keys around it. color0
 * first holds time - time0: computed into a variable of its own, the
 * difference is scheduled before time1 - time0, as in the game.
 */
void Color_EvalKeys(s32 *keys, s32 time, u32 *out)
{
    f32 frac;
    s32 keyTime;
    s32 nextTime;
    u32 colorFrom;
    u32 colorTo;
    u8 redFrom;
    u8 alphaFrom;
    u8 greenFrom;
    u8 blueFrom;
    u8 alpha;
    u8 red;
    u8 green;
    u8 blue;

    for (;; keys += 2) {
        keyTime = keys[0];
        if (keyTime == -1) {
            break;
        }
        if (time == keyTime) {
            *out = keys[1];
            return;
        }
        nextTime = keys[2];
        if (time > keyTime && time < nextTime) {
            colorFrom = time - keyTime;
            colorTo = nextTime - keyTime;
            frac = (f32)(s32)colorFrom / (f32)(s32)colorTo;
            colorFrom = keys[1];
            colorTo = keys[3];
            redFrom = colorFrom >> 16;
            greenFrom = colorFrom >> 8;
            blueFrom = colorFrom;
            alphaFrom = colorFrom >> 24;
            alpha = alphaFrom + (u8)(frac * ((f32)(u8)(colorTo >> 24) - (f32)alphaFrom));
            red = redFrom + (u8)(frac * ((f32)(u8)(colorTo >> 16) - (f32)redFrom));
            green = greenFrom + (u8)(frac * ((f32)(u8)(colorTo >> 8) - (f32)greenFrom));
            blue = blueFrom + (u8)(frac * ((f32)(u8)colorTo - (f32)blueFrom));
            *out = (alpha << 24) | (red << 16) | (green << 8) | blue;
            return;
        }
    }
    *out = keys[-1];
}

void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateZXY(f32 *m, f32 x, f32 y, f32 z);

/*
 * Builds mtx from a scale, rotations and a translation. rotOrder says which
 * rotations, in order: 0 none, 1 x, 2 y, 3 z, 4 x then y, 5 z then y, 6 the
 * three by Mtx_RotateXYZ, 7 by Mtx_RotateZXY.
 */
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 rotOrder, f32 *mtx)
{
    f32 x = trans[0];
    f32 y = trans[1];
    f32 z = trans[2];

    Mtx_Scale(mtx, scale[0], scale[1], scale[2]);
    switch (rotOrder) {
    case 6:
        Mtx_RotateXYZ(mtx, rot[0], rot[1], rot[2]);
        break;
    case 7:
        Mtx_RotateZXY(mtx, rot[0], rot[1], rot[2]);
        break;
    case 4:
        Mtx_RotateX(mtx, rot[0]);
        Mtx_RotateY(mtx, rot[1]);
        break;
    case 5:
        Mtx_RotateZ(mtx, rot[2]);
        Mtx_RotateY(mtx, rot[1]);
        break;
    case 1:
        Mtx_RotateX(mtx, rot[0]);
        break;
    case 2:
        Mtx_RotateY(mtx, rot[1]);
        break;
    case 3:
        Mtx_RotateZ(mtx, rot[2]);
        break;
    case 0:
        break;
    }
    Mtx_SetTrans(mtx, x, y, z);
}

/*
 * Draws a material set (ModelMatSet, include/model.h) with the matrix mtx and
 * the materials mats, faded to `alpha` (0 to 1): white, or black when bit 0 of
 * flags is set; the other bits of flags go to Draw_SetBlendFlags. Returns 0
 * when there is nothing to draw, else 1.
 */
s32 Model_DrawFade(void *set, f32 *mtx, u16 flags, void *mats, f32 alpha)
{
    if (set == NULL) {
        return 0;
    }
    if (((ModelMatSet *)set)->id == -1) {
        return 0;
    }
    Gs_SetState(0x60, 0);
    Gs_SetState(0x1A, (s32) mtx);
    if (flags & 1) {
        Gs_SetState(0x67, (u8)(255.0f * alpha) << 24);
    } else {
        Gs_SetState(0x67, ((u8)(255.0f * alpha) << 24) | 0xFFFFFF);
    }
    Gs_SetLightList(mats, set);
    Draw_ApplyStates(((ModelMatSet *)set)->flags);
    Draw_SetBlendFlags(flags);
    Model_Draw(((ModelMatSet *)set)->id, 0);
    Draw_SetDefaultState();
    return 1;
}

/* Model_DrawFade with a colour, 0xAARRGGBB (the callers build it so),
 * instead of a fade. */
s32 Effect_DrawModel(void *set, f32 *mtx, s32 color, u16 flags, void *mats)
{
    if (set == NULL) {
        return 0;
    }
    if (((ModelMatSet *)set)->id == -1) {
        return 0;
    }
    Gs_SetState(0x60, 0);
    Gs_SetState(0x1A, (s32) mtx);
    Gs_SetState(0x67, color);
    Gs_SetLightList(mats, set);
    Draw_ApplyStates(((ModelMatSet *)set)->flags);
    Draw_SetBlendFlags(flags);
    Model_Draw(((ModelMatSet *)set)->id, 0);
    Draw_SetDefaultState();
    return 1;
}

/* Draws a material set with the matrix mtx, GS state 0x60 set to 0x80 where
 * the other two set 0, and the set's own colour and blending. */
s32 Effect_DrawModelAdd(void *set, f32 *mtx, void *mats)
{
    if (set == NULL) {
        return 0;
    }
    if (((ModelMatSet *)set)->id == -1) {
        return 0;
    }
    Gs_SetState(0x60, 0x80);
    Gs_SetState(0x1A, (s32) mtx);
    Gs_SetLightList(mats, set);
    Draw_ApplyStates(((ModelMatSet *)set)->flags);
    Model_Draw(((ModelMatSet *)set)->id, 0);
    Draw_SetDefaultState();
    return 1;
}

/*
 * Sets the blending of the next draw from a model's draw flags: bit 4 calls
 * Gfx_SetState63(0); bits 1 to 3 pick the blend: 2 Gfx_SetBlend(4, 1),
 * 4 Gfx_SetBlend(4, 5), 8 Gfx_SetBlend(1, 1) with Gfx_SetBlendOp(2).
 */
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

/*
 * An area's effect colour: its kind's entry of gAreaKindColors (four bytes,
 * r, g, b, a). The four bytes are read through four names (the table and the
 * aliases D_0041CF01 to D_0041CF03 at +1 to +3): the game loads each with its
 * own address, which one array read would not give.
 */
void Effect_GetColor(s32 area, u8 *red, u8 *green, u8 *blue, u8 *alpha)
{
    s32 ofs;

    ofs = gAreaKinds[area & 0xFFFF] * 4;
    *red = *(gAreaKindColors + ofs);
    *green = *(D_0041CF01 + ofs);
    *blue = *(D_0041CF02 + ofs);
    *alpha = *(D_0041CF03 + ofs);
}

/* Whether the local player may place another object counted by
 * gScene.placed3: fewer than 3 are out (the items 0x28 and 0x2C to 0x2E test
 * it before use, unk_3549F0.c and the game's unk_70A290.c). */
s32 Scene_CanPlaceKind3(void)
{
    return (s32) gScene.placed3 < 3;
}

/*
 * Whether pl has a lure of type 1 out: a stage object of kind 0x16 (the lure,
 * unk_3E3D00.c) and type 1 owned by pl. Seen: Plesioth bites only that type
 * (em_plesioth.c), the fish never do (unk_3E3D00.c).
 */
s32 Lure_PlayerHasMonsterBait(void *pl)
{
    StageObj *obj;
    s16 i;

    i = 0;
    obj = gStageObjs;
    do {
        if (obj->active != 0 && obj->kind == 0x16 && obj->type == 1 && obj->owner == pl) {
            return 1;
        }
        i++;
        obj++;
    } while (i < STAGEOBJ_COUNT);
    return 0;
}

extern WeaponKindRange gDualSwordRanges[];

/*
 * For a player holding a weapon of type 6 (the dual swords): its place in the
 * list of dual swords that gDualSwordRanges gives as ranges of weapon kinds
 * (-1 ends it), counted from 0 across the ranges; -1 for another weapon.
 * Seen: Bulldrome's effect keeps it per player (em_bulldrome.c).
 */
s16 Player_DualSwordIndex(void *obj)
{
    Player *pl = obj;
    s16 i;
    s16 index;
    s32 kind;

    if (pl->weaponType == 6) {
        for (i = 0, index = 0; gDualSwordRanges[i].lo != -1; i++) {
            kind = pl->weaponKind;
            if (kind >= gDualSwordRanges[i].lo && kind <= gDualSwordRanges[i].hi) {
                return index + kind - gDualSwordRanges[i].lo;
            }
            index += gDualSwordRanges[i].hi - gDualSwordRanges[i].lo + 1;
        }
    }
    return -1;
}

/* Scales a vector by Effect_Scale. */
void Effect_ScaleVec(f32 *vec)
{
    Vec_Scale((f32 *) vec, (f32 *) vec, Effect_Scale());
}

/* The effects' global scale: always 1.0f. */
f32 Effect_Scale(void)
{
    return 1.0f;
}

/*
 * The row of gEffectColors a stage object of kind 0x10 takes from its
 * ownerIndex (unk_868110.c): 2 for 0x13, 0x18 and 0x38; 6 for 0x08 and 0x22;
 * 7 for 0x30, 0x37, 0x42, 0x43 and 0x49; 0 for the rest.
 */
s32 Effect_OwnerColorRow(s32 ownerIndex)
{
    s32 ownerKind;

    ownerKind = ownerIndex & 0xFF;
    switch (ownerKind) {
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

/* A random number from 0 to 1.023, in steps of 0.001. */
f32 Rand_Unit(void)
{
    return 0.001f * (f32) (Rand_Next(1) & 0xFFFF & 0x3FF);
}

/* A random number from -1.024 to 1.022, in steps of 0.002. */
f32 Rand_Signed(void)
{
    return 0.002f * (f32) ((Rand_Next(1) & 0xFFFF & 0x3FF) - 0x200);
}

extern s16 gEffectTexIds[];

/*
 * An effect texture's id as loaded: offline the id itself; online, where only
 * the textures of gEffectTexIds are loaded, its index in that list (-1 ends
 * it), or -1 when it is not there.
 */
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

extern FadeKey gFadeKeys[];
extern FadeState gFadeB;
extern FadeState gFadeA;

/*
 * The task (started by select's boot) that steps the two screen fades, gFadeA
 * and gFadeB, every frame: the task's mode is gFadeA's step, its step
 * gFadeB's (0 clears the fade, 1 runs it). A fade starts at its key `id` (set
 * by FadeX_Start), waits the key's delay, moves its alpha to the key's
 * alphaTo, then goes on to the next key or, after the last, stops. A new id
 * restarts it from that key. The goto keeps the restart inside the switch,
 * as the game's code has it.
 */
void ScreenFade_Task(u8 *task)
{
    TaskSlot *slot = (TaskSlot *)task;
    FadeState *fade;
    FadeKey *key;

    fade = &gFadeA;
    switch (slot->mode) {
    case 0:
        memset(fade, 0, sizeof(FadeState));
        fade->key = NULL;
        slot->mode++;
    case 1:
        switch (fade->active) {
        case 0:
            if (fade->id == 0) {
                break;
            }
        start:
            fade->active = 1;
            fade->cur = fade->id;
            fade->alpha = gFadeKeys[fade->cur - 1].alphaFrom;
            fade->delay = gFadeKeys[fade->cur - 1].delay;
        case 1:
        case 2:
            if (fade->id == 0) {
                fade->active = 0;
                fade->key = NULL;
                break;
            }
            if (fade->id != fade->cur) {
                goto start;
            }
            key = &gFadeKeys[fade->cur - 1];
            fade->key = key;
            fade->prevAlpha = fade->alpha;
            if (fade->alpha != key->alphaTo) {
                if (fade->delay == 0) {
                    fade->alpha += key->alphaStep;
                } else {
                    fade->delay--;
                }
                break;
            }
            fade->starting = 0;
            if (key->last != 0) {
                fade->active = 0;
                fade->id = 0;
                fade->key = NULL;
                break;
            }
            if (fade->active == 1) {
                fade->active++;
            }
            break;
        }
        break;
    }
    fade = &gFadeB;
    switch (slot->step) {
    case 0:
        memset(fade, 0, sizeof(FadeState));
        fade->key = NULL;
        slot->step++;
    case 1:
        switch (fade->active) {
        case 0:
            if (fade->id == 0) {
                break;
            }
        start2:
            fade->active = 1;
            fade->cur = fade->id;
            fade->alpha = gFadeKeys[fade->cur - 1].alphaFrom;
            fade->delay = gFadeKeys[fade->cur - 1].delay;
        case 1:
        case 2:
            if (fade->id == 0) {
                fade->active = 0;
                fade->key = NULL;
                break;
            }
            if (fade->id != fade->cur) {
                goto start2;
            }
            key = &gFadeKeys[fade->cur - 1];
            fade->key = key;
            fade->prevAlpha = fade->alpha;
            if (fade->alpha != key->alphaTo) {
                if (fade->delay == 0) {
                    fade->alpha += key->alphaStep;
                } else {
                    fade->delay--;
                }
                break;
            }
            fade->starting = 0;
            if (key->last != 0) {
                fade->active = 0;
                fade->id = 0;
                fade->key = NULL;
                break;
            }
            if (fade->active == 1) {
                fade->active++;
            }
            break;
        }
        break;
    }
}


extern u32 gFadeAColors[];
extern u32 gFadeBColors[];
struct PrimLineF;
s32 Prim_FillRectF(struct PrimLineF *q);

/*
 * gFadeA's state: 0 idle, 1 while its alpha moves (or before the last key's
 * first pass ends), 2 when it holds its last alpha. The lobby and the game
 * wait for it to leave 1.
 */
u8 FadeA_GetState(void)
{
    FadeState *fade = &gFadeA;
    FadeKey *key;

    if ((fade->active == 0 && fade->starting == 0) || fade->id == 0) {
        return 0;
    }
    key = &gFadeKeys[fade->cur - 1];
    if (fade->alpha != key->alphaTo || (key->last == 0 && fade->active == 1)) {
        return 1;
    }
    return 2;
}

/* The same for gFadeB. */
u8 FadeB_GetState(void)
{
    FadeState *fade = &gFadeB;
    FadeKey *key;

    if ((fade->active == 0 && fade->starting == 0) || fade->id == 0) {
        return 0;
    }
    key = &gFadeKeys[fade->cur - 1];
    if (fade->alpha != key->alphaTo || (key->last == 0 && fade->active == 1)) {
        return 1;
    }
    return 2;
}

/* Draws gFadeA's current key: its rectangle in its colour (gFadeAColors) at
 * the fade's alpha. */
void FadeA_Draw(void)
{
    FadeState *fade = &gFadeA;
    f32 alpha;
    FadeQuad quad;

    if (fade->id == 0) {
        return;
    }
    if (fade->key == NULL) {
        return;
    }
    alpha = (f32)fade->alpha / 256.0f;
    if (alpha <= 0.0f) {
        return;
    }
    Gfx_SetBlend(4, 5);
    Gs_SetState(0x6D, 7);
    quad.p0[0] = fade->key->x;
    quad.p0[1] = fade->key->y;
    quad.p0[2] = fade->key->z;
    quad.p0[3] = 1.0f;
    quad.p1[0] = quad.p0[0] + fade->key->w;
    quad.p1[1] = quad.p0[1] + fade->key->h;
    quad.p1[2] = quad.p0[2];
    quad.p1[3] = 1.0f;
    quad.color = ((u32)(255.0f * alpha) << 24 & 0xFF000000) | (gFadeAColors[fade->key->color] & 0xFFFFFF);
    Prim_FillRectF((struct PrimLineF *)&quad);
    Gs_SetState(0x6D, 3);
}

/* The same for gFadeB (gFadeBColors). */
void FadeB_Draw(void)
{
    FadeState *fade = &gFadeB;
    f32 alpha;
    FadeQuad quad;

    if (fade->id == 0) {
        return;
    }
    if (fade->key == NULL) {
        return;
    }
    alpha = (f32)fade->alpha / 256.0f;
    if (alpha <= 0.0f) {
        return;
    }
    Gfx_SetBlend(4, 5);
    Gs_SetState(0x6D, 7);
    quad.p0[0] = fade->key->x;
    quad.p0[1] = fade->key->y;
    quad.p0[2] = fade->key->z;
    quad.p0[3] = 1.0f;
    quad.p1[0] = quad.p0[0] + fade->key->w;
    quad.p1[1] = quad.p0[1] + fade->key->h;
    quad.p1[2] = quad.p0[2];
    quad.p1[3] = 1.0f;
    quad.color = ((u32)(255.0f * alpha) << 24 & 0xFF000000) | (gFadeBColors[fade->key->color] & 0xFFFFFF);
    Prim_FillRectF((struct PrimLineF *)&quad);
    Gs_SetState(0x6D, 3);
}

/*
 * Starts gFadeA at key `id` (FADE_* in scene.h). prevAlpha holds the first
 * ramp's length in frames until Fade_Task's first step overwrites it.
 */
void FadeA_Start(s32 id)
{
    FadeState *fade = &gFadeA;
    FadeKey *key;
    s32 range;
    s32 step;

    fade->id = id;
    key = &gFadeKeys[fade->id - 1];
    range = key->alphaTo - key->alphaFrom;
    if (range < 0) {
        range = -range;
    }
    step = key->alphaStep;
    if (step < 0) {
        step = -step;
    }
    fade->prevAlpha = range / step;
    fade->cur = fade->id;
    fade->alpha = gFadeKeys[fade->cur - 1].alphaFrom;
    fade->delay = gFadeKeys[fade->cur - 1].delay;
    fade->starting = 1;
}

/* Stops gFadeA at once. */
void FadeA_Reset(void)
{
    gFadeA.active = 0;
    gFadeA.starting = 0;
    gFadeA.id = 0;
    gFadeA.key = NULL;
    gFadeA.prevAlpha = 0;
}

/* The same as FadeA_Start, for gFadeB. */
void FadeB_Start(s32 id)
{
    FadeState *fade = &gFadeB;
    FadeKey *key;
    s32 range;
    s32 step;

    fade->id = id;
    key = &gFadeKeys[fade->id - 1];
    range = key->alphaTo - key->alphaFrom;
    if (range < 0) {
        range = -range;
    }
    step = key->alphaStep;
    if (step < 0) {
        step = -step;
    }
    fade->prevAlpha = range / step;
    fade->cur = fade->id;
    fade->alpha = gFadeKeys[fade->cur - 1].alphaFrom;
    fade->delay = gFadeKeys[fade->cur - 1].delay;
    fade->starting = 1;
}

/* Stops gFadeB at once. */
void FadeB_Reset(void)
{
    gFadeB.active = 0;
    gFadeB.starting = 0;
    gFadeB.id = 0;
    gFadeB.key = NULL;
    gFadeB.prevAlpha = 0;
}

/*
 * Draws the brightness option over the whole screen (640 x 448): above the
 * neutral 0x80 a white rectangle of alpha brightness - 0x80, below it a black
 * one of alpha (0x80 - brightness) * 2, each with its own GS blending (state
 * 0x5E); GS states 0xD and 0x5E are put back after.
 */
void Brightness_Draw(void)
{
    s32 dark;
    s32 alphaLevel;
    s32 level;
    s32 savedAlpha;
    s32 savedTest;
    ScreenRect rect;

    if (gSysSettings.brightness == 0x80) {
        return;
    }
    if (gSysSettings.brightness > 0x80U) {
        alphaLevel = gSysSettings.brightness - 0x80;
        dark = 0;
        level = 0xFF;
    } else {
        alphaLevel = 0x80 - gSysSettings.brightness;
        dark = 1;
        alphaLevel *= 2;
        level = 0;
    }
    if (alphaLevel == 0) {
        return;
    }
    rect.w = 640;
    rect.y = 0;
    rect.h = 448;
    rect.x = 0;
    rect.color = (alphaLevel << 24) | (level << 16) | (level << 8) | level;
    savedAlpha = Gs_GetState(0xD);
    savedTest = Gs_GetState(0x5E);
    Gs_SetState(0xD, 0);
    if (dark == 0) {
        Gs_SetState(0x5E, 0x12);
    } else {
        Gs_SetState(0x5E, 0x32);
    }
    Prim_FillRect((struct PrimLine *)&rect);
    Gs_SetState(0xD, savedAlpha);
    Gs_SetState(0x5E, savedTest);
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
void Scene_Idle(void);
void Scene_QuestFrameTick(void);
void Scene_Opening(void);
s32 Quest_Teardown(s32 arg0);
extern u8 gGmSubLoaded;

/*
 * The scene task (a task slot; the lobby starts it too): counts the play
 * time, loads gm_sub offline on its first frame, then each frame runs the
 * scene gScene.scene (SCENE_*), between the network's receive and flush when
 * online.
 */
void Scene_QuestTask(u8 *task)
{
    TaskSlot *slot = (TaskSlot *)task;

    PlayTime_Tick();
    func_0036CCE0();
    switch (slot->mode) {
    case 0:
        if (Game_IsOnline() == 0) {
            gGmSubLoaded = 0;
            Overlay_LoadGmSub();
        }
        Scene_RequestPause(0);
        slot->mode++;
    case 1:
        if (Game_IsOnline() == 1) {
            QNetIf_UpdateReceive();
        }
        switch (gScene.scene) {
        case SCENE_ENTER:
            Scene_EnterField();
        case SCENE_QUEST_START:
            Scene_QuestStartTask();
            break;
        case SCENE_FIELD:
            Scene_UpdateQuest();
            break;
        case SCENE_QUEST_END:
            Scene_ResultFade();
            break;
        case SCENE_IDLE:
            Scene_Idle();
            break;
        case SCENE_RESULTS:
            Scene_QuestFrameTick();
            break;
        case SCENE_OPENING:
            Scene_Opening();
            break;
        case SCENE_EXIT:
            if (Quest_Teardown((s32)task) == 1) {
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

/*
 * SCENE_ENTER: goes on to SCENE_QUEST_START with every player local and no
 * preset hunters, and starts the quest preparation (plsel_entry, with the
 * overlays 3 and 0xB) to run alongside.
 */
void Scene_EnterField(void)
{
    s16 i;

    gScene.step = 0;
    gScene.subStep = 0;
    gScene.scene = SCENE_QUEST_START;
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

/*
 * QSTART_INIT, run once the quest preparation has cleared allLocal: resets
 * the subsystems (keeping gSoundLoading), copies the options and each
 * player's choices from gQuestSync, sets up the quest and its supply box,
 * and tells the peers. A drill starts plsel's select task; a training quest
 * loads overlay 0xB too.
 */
void Scene_StartQuest(void)
{
    u8 soundLoading;
    s32 i;

    if (gScene.allLocal != 0) {
        return;
    }
    soundLoading = gSoundLoading;
    Game_ResetSubsystems();
    Gfx_ClearModelBuffers();
    gSoundLoading = soundLoading;
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

/*
 * QSTART_PLAY: while a drill's select screen runs (allLocal set) it sends
 * the state every 16 frames and waits; a quest given up there
 * (QUEST_PHASE_QUIT) goes to SCENE_RESULTS. Then it copies the players'
 * choices again, sets up the HUD, the quest, the object lists and the
 * monsters, and goes on to load the sounds. The goto joins the two ways to
 * SCENE_RESULTS, as the game's code does.
 */
void Scene_StartQuestPlay(void)
{
    s16 i;

    if (gScene.allLocal != 0) {
        if ((gFrameTick & 0xF) == 0) {
            QSync_SendState(8, gScene.localPlayer);
        }
        if (gScene.questPhase == QUEST_PHASE_QUIT) {
            Loading_Stop();
            goto quit;
        }
        return;
    }
    if (Quest_IsDrill() == 1 && gScene.questPhase == QUEST_PHASE_QUIT) {
    quit:
        gScene.step = 0;
        gScene.scene = SCENE_RESULTS;
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
        gScene.itemUses[i] = 0; /* itemUses and the 6 bytes after it */
    }
    Quest_SpawnMonsters(1);
    Packet_SetMode2();
    gSoundLoading = 0;
    gScene.step++;
    QSync_SendState(8, gScene.localPlayer);
}

/*
 * QSTART_SOUNDS, one subStep a frame: the common bank 1, the area's sound
 * effect bank 7 (gAreaSeBanks), each of the four players' banks 2 to 5
 * (three subSteps each: clear, list and load, wait), then the monsters'
 * bank 6; then the next scene step. Banks are read into gLoadBuf.
 */
void Scene_LoadQuestSounds(void)
{
    u8 *buf = gLoadBuf;
    s16 player;
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
        player = (gScene.subStep - 6) / 3;
        Sound_StopBank(player + 2);
        FlushCache(0);
        SndLoad_Init(2);
        return;
    case 7:
    case 10:
    case 13:
    case 16:
        player = (gScene.subStep - 7) / 3;
        if (SndLoad_QuestBanks(buf, (u8 *)&gPlayers[player])) {
            gScene.subStep++;
            Sound_LoadStaged((struct SndFile *) buf, buf + 0x20000, player + 2);
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

/*
 * QSTART_FIELD: online, waits for every peer (QSync_Begin); then runs the
 * field once, shows the players, clears the quest's per-run scene state and
 * the timer, starts the area's fog and objects, and fades in.
 */
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
    FadeA_Start(FADE_IN_BLACK);
    Tex_PatchClut_Unk();
    Loading_Stop();
    gGalleryTitle[0] = 0;
    QNetIf_Unready();
    gScene.step++;
}

/* QSTART_AREA: online, waits for the peers; then goes to SCENE_FIELD and
 * starts the area's music. */
void Scene_EnterArea(void)
{
    if ((Game_IsOnline() == 1) && (QuestNet_NoPeerReady() == 0)) {
        Loading_Request();
        return;
    }
    gScene.step = 0;
    gScene.scene = SCENE_FIELD;
    Bgm_EnterArea(gScene.area);
    Jingle_OnAreaChange(gScene.area);
}

/* SCENE_QUEST_START: runs the quest start's step (QSTART_*). */
void Scene_QuestStartTask(void)
{
    Pad_UpdateInput();
    switch (gScene.step) {
    case QSTART_INIT:
        Scene_StartQuest();
        break;
    case QSTART_PLAY:
        Scene_StartQuestPlay();
        break;
    case QSTART_SOUNDS:
        Scene_LoadQuestSounds();
        break;
    case QSTART_FIELD:
        Scene_StartField();
        break;
    case QSTART_AREA:
        Scene_EnterArea();
        break;
    }
}

/*
 * SCENE_FIELD, the quest in the field. Steps 0 and 1 run the frame: events,
 * the field, the notices and the quest; when the quest is over it clears the
 * local player's aim and requests and goes to SCENE_QUEST_END (or to
 * SCENE_RESULTS when the quest was given up). When the local player starts
 * to leave the area, step 2 silences the sound and draws the map for 10
 * frames, then swaps the areas: frees the old one, loads the new one, its
 * objects and monsters. Steps 3 to 6 load the monsters' bank 6 and the
 * area's bank 7, then fade in.
 */
void Scene_UpdateQuest(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
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
            gScene.scene = SCENE_QUEST_END;
            pl->aimLockTimer = 0;
            pl->aimMode = 0;
            pl->unk393[0x395 - 0x393] = 0;
            *(s16 *)&pl->unk6B1[0x6B2 - 0x6B1] = 0;
            pl->hitStop = 0;
            pl->hitStopAdd = 0;
            pl->teamItemMask = 0;
            pl->actRequest = 0;
            pl->offerRequest = 0;
            if (gScene.questPhase == QUEST_PHASE_QUIT) {
                gScene.step = 0;
                gScene.scene = SCENE_RESULTS;
                Scene_LoadOverlay(0xB);
            }
            break;
        }
        if (pl->areaMoveState != 0) {
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
        QuestEm_LeaveArea(pl->area, pl->nextArea);
        MinePoint_RemoveTemporary();
        gScene.area = (u8)pl->nextArea;
        pl->area = pl->nextArea;
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
            FadeA_Start(FADE_IN_BLACK);
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

/*
 * SCENE_QUEST_END: waits 120 frames, darkens the screen to alpha 0xC0 by 8 a
 * frame, shows the quest's end (the result texture and an area object of
 * kind 0x17 with the quest phase) for 150 frames, then marks the local
 * player ready, goes to SCENE_RESULTS and loads overlay 0xB. The screen is
 * covered by a sprite of colour 0xFF703A18 at the timer's alpha.
 */
void Scene_ResultFade(void)
{
    SceneState *sceneState = &gScene;
    SprDesc spr;

    Pad_UpdateInput();
    switch (gScene.step) {
    case 0:
        sceneState->step++;
        sceneState->timer = 0;
        sceneState->timer2 = 0;
        break;
    case 1:
        if (++sceneState->timer >= 120) {
            sceneState->step++;
        }
        break;
    case 2:
        sceneState->timer += 8;
        if (sceneState->timer >= 0xC0) {
            sceneState->step++;
            sceneState->timer = 0xC0;
            sceneState->timer2 = 0;
            Tex_LoadResultTex_Unk();
            AreaObj_K17_Create(gScene.questPhase);
        }
        break;
    case 3:
        if (++sceneState->timer2 >= 150) {
            sceneState->step++;
        }
        break;
    case 4:
        sceneState->scene = SCENE_RESULTS;
        Scene_LoadOverlay(0xB);
        sceneState->step = 0;
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
    spr.alpha = (f32)sceneState->timer / 255.0f;
    spr.unk34 = 0;
    spr.flags = 5;
    spr.color = 0xFF703A18;
    Sprite_Queue((struct SprDef *)&spr);
    Notice_Update();
    Field_UpdateAll();
    Frame_Draw();
}

/* SCENE_IDLE: nothing. */
void Scene_Idle(void)
{
}

/* SCENE_RESULTS: runs overlay 0xB's results frame (Plsel_ResultsUpdate), sending
 * the state to the peers every 60 frames. */
void Scene_QuestFrameTick(void)
{
    s16 frame;

    Plsel_ResultsUpdate();
    frame = gScene.stateSyncFrame + 1;
    gScene.stateSyncFrame = frame;
    if (frame >= 60) {
        gScene.stateSyncFrame = 0;
        QSync_SendState(8, gScene.localPlayer);
    }
    Frame_Draw();
}

/*
 * Runs and draws one field frame; while paused it only updates the item HUD
 * and draws over the screen's capture, and on leaving the pause it clears
 * the state and runs the frame. Returns 0. The goto shares the frame's code
 * between the two ways in, as the game's code does.
 */
s32 Scene_FieldFrame(void)
{
    Pad_UpdateInput();
    if (Game_PauseState() < PAUSE_CAPTURED) {
    draw:
        Field_UpdateAll();
        Frame_Draw();
        Hit_ClearStatusAll();
        return 0;
    }
    if (Game_PauseState() == PAUSE_LEAVING) {
        gScene.pauseState = PAUSE_NONE;
        goto draw;
    }
    ItemHud_Update();
    Frame_DrawOverCapture();
    return 0;
}

/*
 * Counts gScene.frame, and the save's play time in seconds every 30 frames
 * (not while paused offline), up to PLAYTIME_MAX.
 */
void PlayTime_Tick(void)
{
    gScene.frame += 1;
    if (Game_IsOnline() == 0) {
        if (Game_PauseState() < PAUSE_CAPTURED) {
            goto count;
        }
    } else {
count:
        if (((s32) gScene.frame % 30) == 0) {
            gSave.playTime += 1;
            if (gSave.playTime > (u32)PLAYTIME_MAX) {
                gSave.playTime = PLAYTIME_MAX;
            }
        }
    }
}

/*
 * The offline pause: mode 1 asks for it (from no pause), mode 0 leaves it
 * (from a pause shown). Returns 1 when the state changed. Online, or once
 * the quest is over, there is no pause.
 */
s32 Scene_RequestPause(s32 mode)
{
    s32 state;

    if (Game_IsOnline() == 1) {
        gScene.pauseState = PAUSE_NONE;
        return 0;
    }
    if (Quest_PhaseIsOver(0) == 1) {
        gScene.pauseState = PAUSE_NONE;
        return 0;
    }
    state = gScene.pauseState;
    if (mode == 0 && state >= PAUSE_CAPTURED && state < PAUSE_LEAVING) {
        gScene.pauseState = PAUSE_LEAVING;
        return 1;
    }
    if (mode == 1 && state == PAUSE_NONE) {
        gScene.pauseState = PAUSE_REQUESTED;
        return 1;
    }
    return 0;
}

/* The pause state, PAUSE_*. */
s32 Game_PauseState(void)
{
    return (s32) gScene.pauseState;
}

/*
 * SCENE_EXIT: ends the quest's sound and sync. Online the lobby takes over
 * (step 0); offline it resets the subsystems and clears quest flag 0x200000.
 * Returns 1. The argument (the task) is not used.
 */
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

/* Ends the task in slot 2 with code 1 (Task_EndGmSubForNetScene). */
void Quest_TeardownNet(void)
{
    Task_EndGmSubForNetScene();
}

/*
 * SCENE_OPENING plays a movie. subStep 0 is the first boot: movie 7, then
 * SCENE_EXIT. Otherwise movie 8, then the overlay's sequence that follows it
 * (Ending_Init, Ending_Update), and, the first time only (openingFirst, event
 * flag 0x26 not yet set), the award texts gAwardNoticeLines[5] centred on the
 * screen for 150 frames (button 0x20 shortens it after the first 60), then
 * SCENE_EXIT. Button 0x8000 stops a movie that is playing; each movie ends
 * with FADE_OUT_BLACK and a wait of at most 30 frames for it to stop.
 */
void Scene_Opening(void)
{
    SceneState *sceneState = &gScene;
    s32 status;
    char **lines;
    s32 y;
    s32 x;

    switch (gScene.step) {
    case 0:
        if (sceneState->subStep == 0) {
            Save_SetEventFlag(0x185);
            Movie_Play(7);
            EventFlag_Set(0x25);
        } else {
            Movie_Play(8);
            if (EventFlag_Test(0x26) == 0) {
                sceneState->openingFirst = 1;
            } else {
                sceneState->openingFirst = 0;
            }
            EventFlag_Set(0x26);
            EventFlag_Set(0x31);
            EventFlag_Set(0x32);
            EventFlag_Set(0x33);
            EventFlag_Set(0x34);
        }
        sceneState->step++;
        sceneState->timer = 0;
        break;
    case 1:
        Movie_Update();
        status = Movie_GetStatus();
        if (status != 3) {
            sceneState->timer++;
            if (status != 2 || !(gPadState[0].pressed & 0x8000)) {
                break;
            }
        }
        FadeA_Start(FADE_OUT_BLACK);
        sceneState->step++;
        sceneState->timer = 30;
        Movie_RequestStop();
        break;
    case 2:
        Movie_Update();
        if (FadeA_GetState() == 1) {
            break;
        }
        if (Movie_GetStatus() == 0) {
        stop:
            Movie_Stop();
            if (sceneState->subStep == 0) {
                FadeA_Start(FADE_IN_BLACK);
                sceneState->scene = SCENE_EXIT;
                sceneState->step = 0;
                sceneState->subStep = 0;
            } else {
                sceneState->step++;
                Ending_Init();
            }
        } else {
            if (sceneState->timer == 0) {
                goto stop;
            }
            sceneState->timer--;
        }
        break;
    case 3:
        if (Ending_Update() != 0) {
            break;
        }
        if (sceneState->openingFirst == 1) {
            FadeA_Reset();
            sceneState->step++;
            sceneState->timer = 150;
        } else {
            FadeA_Start(FADE_IN_BLACK);
            sceneState->scene = SCENE_EXIT;
            sceneState->step = 0;
            sceneState->subStep = 0;
        }
        break;
    case 4:
        if (--sceneState->timer != 0) {
            if (sceneState->timer < 90 && (gPadState[0].pressed & 0x20)) {
                sceneState->timer = 1;
            }
            lines = gAwardNoticeLines[5];
            Font_SetSpacing(18, 18);
            Text_SetColor(0);
            for (y = 160; *lines != NULL; lines++, y += 22) {
                x = Str_Length(*lines);
                x = (640 - x * 9) / 2;
                Font_SetPos(x, y);
                Text_Printf(*lines);
            }
            Text_DrawAllPages();
        } else {
            FadeA_Start(FADE_IN_BLACK);
            sceneState->scene = SCENE_EXIT;
            sceneState->step = 0;
            sceneState->subStep = 0;
            return;
        }
        break;
    }
}

/* Clears the scene state and the models: player 0 local, pads 0 and 1, the
 * next quest id 1, offline, the save's character slot from gQuestSync. */
void Scene_ResetAll(void)
{
    memset(&gScene, 0, sizeof(gScene));
    Model_FreeAll();
    UnreadFlags_ClearAll();
    Mem_Compact();
    gScene.frame = 0;
    gScene.localPlayer = 0;
    gNextQuestId = 1;
    gScene.padIndex[1] = 1;
    *gScene.padIndex = 0;
    gScene.online = 0;
    gScene.charIndex = gQuestSync.charIndex;
    CamCtl_Clear();
}

/* Sets up the HUD and resets the camera requests. */
void Hud_Start(void)
{
    Hud_Init();
    CamReq_Reset();
}

/*
 * Loads the quest's start: the common textures, the start area, the common
 * animations, the players (those not in the quest hidden), the HUD's
 * textures and the common models.
 */
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
    Quest_SetupStub();
    Quest_EmptyHook();
    func_0033A360();
}

/* Loads player idx's animations for its weapon and sets it up for the
 * quest. */
void Player_SetupQuest(s32 idx)
{
    Player *pl;

    pl = &gPlayers[idx];
    Anim_LoadPlayer(idx, (s32) pl->weaponType);
    Player_InitPartTable(idx);
    Player_InitForQuest(pl);
}

/*
 * Loads the stage objects' common model sets gStageModelSets[0], [1], [2]
 * and [4], each from its pack with its texture file (gCommonModelTexFiles);
 * set 0 comes from pack 5 online, pack 0 offline. Stops at the first model
 * slot it can't get.
 */
void Quest_LoadCommonModels(void)
{
    s32 pack;
    s32 slot;
    s32 nextSlot;

    pack = gScene.online == 0 ? 0 : 5;
    Load_CommonModelPack(pack);
    slot = Model_Alloc(1);
    if (slot < 0) {
        return;
    }
    gStageModelSets[0] = Model_Get(slot);
    Model_Reserve(slot, 1);
    Model_SetupWithPack(slot, (*(s32 *)&gSysSettings.modelPack), 0xF3, gCommonModelTexFiles[pack], 0xB00, 0);
    Load_CommonModelPack(1);
    nextSlot = Model_Alloc(1);
    if (nextSlot < 0) {
        return;
    }
    gStageModelSets[1] = Model_Get(nextSlot);
    Model_Reserve(nextSlot, 1);
    Model_SetupWithPack(nextSlot, (*(s32 *)&gSysSettings.modelPack), 0x106, gCommonModelTexFiles[1], 0x900, 1);
    Load_CommonModelPack(2);
    nextSlot = Model_Alloc(1);
    if (nextSlot < 0) {
        return;
    }
    gStageModelSets[2] = Model_Get(nextSlot);
    Model_Reserve(nextSlot, 1);
    Model_SetupWithPack(nextSlot, (*(s32 *)&gSysSettings.modelPack), 0x105, gCommonModelTexFiles[2], 0x900, 1);
    Load_CommonModelPack(4);
    nextSlot = Model_Alloc(1);
    if (nextSlot < 0) {
        return;
    }
    gStageModelSets[4] = Model_Get(nextSlot);
    Model_Reserve(nextSlot, 1);
    Model_SetupWithPack(nextSlot, (*(s32 *)&gSysSettings.modelPack), 0x103, gCommonModelTexFiles[4], 0x800, 1);
}

/* An empty step of Quest_Setup. */
void Quest_SetupStub(void)
{
}

/*
 * Resets the input, the lights, the fades, the pause and the camera for the
 * area entered; mode 0xFF (an area change) stops there. Otherwise it places
 * the quest's monsters: a quest (questId set) through Quest_EnterArea;
 * without one, each monster slot's kind and count from gQuestSync. With
 * debug switch 0x17 on, test monsters of fixed kinds instead (debug switch
 * 0x74 gives a second one in area 0).
 */
void Quest_SpawnMonsters(s32 mode)
{
    s16 i;
    s16 n;
    s16 index;
    s16 j;
    u8 variants[4];
    Em *em;

    Pad_ResetInput();
    Area_ResetStub();
    Light_Reset();
    Area_Enter(gScene.area);
    FadeA_Reset();
    FadeB_Reset();
    gScene.pauseState = PAUSE_NONE;
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
        variants[0] = 0;
        variants[1] = 0;
        variants[2] = 0;
        variants[3] = 0;
        index = 0;
        for (i = 0; i < 4; i++) {
            gScene.emKind[i] = gQuestSync.emKind[i];
            if (gScene.emKind[i] != 0) {
                GmLoad_LoadEmData(gScene.map, gScene.emKind[i]);
                EmSlot_LoadModels(i);
                for (n = 0; n < gQuestSync.emCount[i]; n++, index++) {
                    em = EmSlot_Alloc();
                    if (em != NULL) {
                        em->modelSlot = i;
                        em->type = gScene.emKind[em->modelSlot];
                        em->variant = variants[i]++;
                        em->index = index;
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

/*
 * Clears the players and sets up those in the quest from the scene's copies
 * of their choices: offline, players after the first get their weapon from
 * gQuestSync and a numbered default name; online a player uses the save
 * (the local one) or its preset, unless presetEquip asks for the weapon
 * given. Then the drill's and the training's set-ups (overlay functions),
 * the skills, and the weapon's class and flags.
 */
void Quest_InitPlayers(void)
{
    s16 i;
    Player *pl;

    memset(gPlayerSys, 0, 0x10);
    for (i = 0; i < 4; i++) {
        pl = &gPlayers[i];
        memset(pl, 0, sizeof(Player));
        if (i >= gScene.playerCount) {
            continue;
        }
        if (gScene.inQuest[i] == 1) {
            pl->active = 1;
        } else {
            pl->active = 0;
            continue;
        }
        pl->index = i;
        pl->gender = gScene.playerGender[i];
        pl->presetEquip = gScene.presetEquip[i];
        pl->startItemList = gScene.startItemList[i];
        Quest_SetStartArea(i);
        if (Game_IsOnline() == 0 && i > 0) {
            pl->equip[6] = gScene.weapon[i];
            pl->weaponKind = Equip_GetKind(&gScene.weapon[i]);
            sprintf((char *)pl->name, TXT(0x0046CE10) /* a default player name, with the number */, i + 1);
        } else if (gScene.presetEquip[i] == 0 || gSysSettings.useSavedEquip != 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_LoadFromSave(pl);
                Player_LoadName(pl);
            } else {
                Player_LoadPreset(gScene.preset[i], pl);
            }
        } else {
            pl->equip[6] = gScene.weapon[i];
            pl->weaponKind = Equip_GetKind(&gScene.weapon[i]);
            sprintf((char *)pl->name, TXT(0x0046CE10) /* a default player name, with the number */, i + 1);
        }
        if (Quest_IsDrill() == 1) {
            Plsel_ApplyPresetEquip(pl, i);
        }
        if (Quest_IsTraining() == 1) {
            Plsel_ApplyStageEquip(pl, i);
        }
        Player_CalcSkills(pl);
        pl->weaponType = gWeaponKindClass[pl->weaponKind];
        pl->isGunner = WeaponClass_IsGunner(pl->weaponType);
        pl->bladeSpecial = Equip_GetBladeSpecial(&pl->equip[6]);
        pl->weaponFlags = WeaponType_Flags(pl->weaponType);
        if (Player_IsLocal(pl) == 1) {
            Player_BackupPouch(pl, 0);
        }
    }
}

/* An empty step of Quest_SpawnMonsters. */
void Area_ResetStub(void)
{
}

/* Clears the light sets and sets up the area's. */
void Light_Reset(void)
{
    memset(&gLightSets, 0, 0x290U);
    Light_InitAll((s32) gScene.area);
}

/* Clears the pad replay and both ports' input, this frame's and the last. */
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

/*
 * Loads an area: its pack, its grids (Area_LoadTables), its model with its
 * texture file, its sub-models and its camera data. Areas 0x83 and 0x84 take
 * a second texture file online while ElderRaid_InState1.
 */
void Area_Load(s32 area)
{
    AreaState *areaState = &gAreaState;
    s32 slot;
    s32 tex;

    gScene.area = area;
    areaState->area = area;
    Load_AreaPack(gScene.area);
    Area_LoadTables(gScene.area);
    slot = Model_Alloc(1);
    if (slot >= 0) {
        areaState->modelSlot = slot;
        areaState->modelCount = 1;
        areaState->model = Model_Get(slot);
        Model_Reserve(slot, 1);
        switch (gScene.area) {
        case 0x83:
            if (gScene.online != 0 && ElderRaid_InState1() == 1) {
                tex = gArea83TexFiles[1];
            } else {
                tex = gArea83TexFiles[0];
            }
            break;
        case 0x84:
            if (gScene.online != 0 && ElderRaid_InState1() == 1) {
                tex = gArea84TexFiles[1];
            } else {
                tex = gArea84TexFiles[0];
            }
            break;
        default:
            tex = gAreaTexFiles[gScene.area];
            break;
        }
        Model_SetupTexPack(slot, (*(s32 *)&gSysSettings.modelPack), 0x108, tex, 0x100);
        Area_LoadSubModels(gScene.area);
        Area_LoadCamData(gScene.area);
    }
}

/* Loads and sets up the common animations (set `set`). */
void Anim_LoadCommon(s32 set)
{
    Load_CommonAnim(set);
    Anim_SetupCommon();
}

/* Loads and sets up player idx's animations for a weapon type. */
void Anim_LoadPlayer(s32 idx, s32 weaponType)
{
    Load_PlayerAnim(idx, weaponType);
    Anim_SetupPlayer(idx);
}

/* Loads and sets up the animations of monster slot idx (`kind` is passed on). */
void Anim_LoadEm(s32 idx, s32 kind)
{
    Load_EmAnim(idx, kind);
    Anim_SetupEm(idx, kind);
}

/* Loads animation set (slot, set) and sets up idx's. */
void Anim_LoadEmSlot(s32 slot, s32 set, s32 idx)
{
    Load_EmSlotAnim(slot, set);
    Anim_SetupEmSlot(idx);
}

/*
 * Loads an area's two grids from its table file (gAreaTableFiles): part 0
 * into gWorkArea, part 1 into gSysSettings.extraBuf, each relocated in place.
 */
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

/*
 * Relocates the first grid (the header 8 bytes into the part): each cell's
 * list of offsets, ended by -1, becomes pointers, and the grid's globals
 * gAreaGrid* are set.
 */
void AreaTable_Relocate(u8 *file)
{
    AreaGridHeader *hdr = (AreaGridHeader *)(file + 8);
    u32 i;
    u32 j;
    s32 k;
    s32 **cell;

    gAreaGridCellX = hdr->cellX;
    gAreaGridCellZ = hdr->cellZ;
    gAreaGridRows = hdr->rows;
    gAreaGridCols = hdr->cols;
    gAreaGridCells = (s32 **)((u8 *)hdr + hdr->tblOff);
    gAreaGridData = (u32)((u8 *)hdr + hdr->dataOff);
    for (i = 0; i < hdr->rows; i++) {
        for (j = 0; j < hdr->cols; j++) {
            cell = &gAreaGridCells[i * hdr->cols + j];
            *cell = (s32 *)((u8 *)*cell + (u32)hdr);
            for (k = 0; (*cell)[k] != -1; k++) {
                (*cell)[k] += gAreaGridData;
            }
        }
    }
}

/* The same for the second grid (gAreaGrid2*). */
void AreaTable2_Relocate(u8 *file)
{
    AreaGridHeader *hdr = (AreaGridHeader *)(file + 8);
    u32 i;
    u32 j;
    s32 k;
    s32 **cell;

    gAreaGrid2CellX = hdr->cellX;
    gAreaGrid2CellZ = hdr->cellZ;
    gAreaGrid2Rows = hdr->rows;
    gAreaGrid2Cols = hdr->cols;
    gAreaGrid2Cells = (s32 **)((u8 *)hdr + hdr->tblOff);
    gAreaGrid2Data = (u32)((u8 *)hdr + hdr->dataOff);
    for (i = 0; i < hdr->rows; i++) {
        for (j = 0; j < hdr->cols; j++) {
            cell = &gAreaGrid2Cells[i * hdr->cols + j];
            *cell = (s32 *)((u8 *)*cell + (u32)hdr);
            for (k = 0; (*cell)[k] != -1; k++) {
                (*cell)[k] += gAreaGrid2Data;
            }
        }
    }
}

/* unk_3240E0's .data (0x41CF00-0x41D1F0) */
u8 gAreaKindColors[80] = {
#include "sub_main/0041CF00.inc"
};
WeaponKindRange gDualSwordRanges[12] = {
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
FadeKey gFadeKeys[15] = {
#include "sub_main/0041CFE0.inc"
};
u32 gFadeAColors[12] = {
#include "sub_main/0041D190.inc"
};
u32 gFadeBColors[12] = {
#include "sub_main/0041D1C0.inc"
};
