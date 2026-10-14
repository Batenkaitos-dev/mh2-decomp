/*
 * 0xD74450-0xD75F80: "galex".
 */
#include "common.h"
#include "plsel.h"
extern u8 gFieldControl;
extern s32 gStageFile; /* sub_main: the stage file's address */
#include "scene.h"
#include "task.h"
#include "pad.h"
#include "m2c_macros.h"
#include "em.h"
#include "save.h"
#include "player.h"
#include "syssettings.h"
#include "quest.h"
#include "scevent.h"
#include "area.h"

void Afs_LoadPartition(s32 ptid, s32 flid, void *ptinfo, char *name);
s32 Afs_ReadFileIdle(s32 flid, s32 ptid, void *buf);

/* An object of this unit (Galex_InitObj sets it up). */
typedef struct GalObj {
    s16 unk00;                  /* 0x00: index into gEmIsLarge2 */
    u8 unk02[0x04 - 0x02];
    s8 unk04;                   /* 0x04 */
    u8 unk05[0x2C - 0x05];
    s16 unk2C;                  /* 0x2C */
    s16 unk2E;                  /* 0x2E */
    u16 unk30;                  /* 0x30 */
    s16 unk32;                  /* 0x32 */
    s8 unk34;                   /* 0x34 */
    s8 unk35;                   /* 0x35 */
    s8 unk36;                   /* 0x36: level, 0..16 */
    u8 unk37[0x3C - 0x37];
} GalObj;

void Galex_RollLevel(GalObj *o);
extern u8 gEmIsLarge2[];

void CamReq_Lobby44(s32);
void Bgm_PlayField(s32, s32);
struct StageEntry;
void *StageEntry_GetData(struct StageEntry *e, s32 which);
void Em_CopyBoneMatrices(Em *);
s32 func_905930(Em *);
void Em_UpdateMatrix(Em *);
void Em_CarryRiders(Em *);
void Stream_UpdateFades(void);
void Em_CollideField(Em *em, u8 keep);
void CamCtl_Apply(void);
void LightSet_UpdateAll(void);
void StageObj_UpdateAll(void);
void ObjAttach_UpdateAll(void);
void Obj_UpdateAll(void);
void Area_UpdateTimers(void);

s32 Em_HasRandomSize(s16);
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Quest_GetSeason(void);
extern u16 gEmSizeSeasonAdd[][3];

u8 Daytime_Get(void);
u16 Quest_Random(void);
extern s8 *gEmLevelTables[];
extern s8 gEmLevelTableIdx[][2];
extern s8 gEmLevelSeasonAdd[][3];

/* .data (0xD79370-0xD79580) */
/* 0xD79370: the galex quests (8 bytes each): unk6 is the time limit in
 * frames (0: none). */
typedef struct GalQuest {
    u16 unk0;
    u16 unk2;
    u8 unk4;
    u8 unk5;
    u16 limit;                  /* 0x6 */
} GalQuest;

GalQuest gGalexQuests[46] = {
#include "plsel/00D79370.inc"
};
/* 0xD794E0: two lists of (key, value) byte pairs, each ended by a 0xFF
 * key: the galex quests' start times (at 0), then (slot, value) pairs for
 * the quest kinds (at 12). */
u8 gGalexDaytimes[80][2] = {
#include "plsel/00D794E0.inc"
};

void Galex_UpdateMonsters(void) {
    s32 i;
    Em *e;
    Em *e2;
    s32 j;

    for (i = 0, e = gEms; i < 20; i++, e++) {
        if (e->active != 0) {
            Em_CopyBoneMatrices(e);
            if (func_905930(e) == 0) {
                Em_UpdateMatrix(e);
                Em_CarryRiders(e);
            }
        }
    }
    Stream_UpdateFades();
    for (j = 0, e2 = gEms; j < 20; j++, e2++) {
        if (e2->active != 0 && ((u8 *)e2)[0x738] != 0) {
            Em_CollideField(e2, 1);
        }
    }
    CamCtl_Apply();
    LightSet_UpdateAll();
    StageObj_UpdateAll();
    ObjAttach_UpdateAll();
    Obj_UpdateAll();
    Area_UpdateTimers();
}

void func_00D74550(u8 a) {

}

/* The camera and area of a galex quest: the head of a scene event's record (scevent.h). */
typedef ScEventHead GalView;

s32 Overlay_IsLoaded(s32);
extern GalView *gGalleryScEvents[];
void ScEvent_SetupB(ScEventHead *h);
void Scene_LoadOverlay(s32);
void SeqA_Reset(void);
void func_36BC60(void);
void *memset(void *, s32, u32);
void Game_ResetSubsystems(void);
void Gfx_ClearModelBuffers(void);
void Quest_Clear(void);
void Packet_SetMode0(void);
extern u8 gQuestExtra[];
void Daytime_Set(u8); /* unit's own declaration: the definition (main) takes s32; Galex_QuestTask is 1 word off with it */
void Season_Set(s32);
void Galex_SetupQuest(void);
void Quest_Init(void);
void ScEvent_StartFrom(ScEventHead *h);
u16 Area_OtherTimeId(u16);
void Hud_Start(void);
void Area_Load(s32);
void Quest_InitPlayers(void);
void Quest_LoadCommonModels(void);
void func_327F70(void);
void func_3350B0(void);
void func_33A360(void);
void Quest_SpawnMonsters(s32);
void Packet_SetMode2(void);
extern u8 *gLoadBuf;
s32 Sound_StopBank(s32 slot);
s32 Sound_SetSpuMapPage(u32 page);
void FlushCache(s32);
void Sound_QueueBank(u32 id, void *buf);
s32 FileQueue_IsBusy(void);
struct SndFile;
s32 Sound_LoadBank(struct SndFile *f, u32 slot);
s32 Sound_CheckTransfer(void);
extern s16 gAreaSeBanks[];
void SndLoad_Init(s8 count);
s32 SndLoad_QuestBanks(u8 *, u8 *);
s32 Sound_LoadStaged(struct SndFile *f, u8 *out, u32 slot);
s32 SndLoad_ListBanks(u8 *, u8 *);
void Area_ApplyFog(u16);
void Area_StartObjects(u16);
void Bgm_EnterArea(u16);
void ScEvent_SetCamera(u8, u8);
void Mem_Compact(void);
void CamCtl_Update(s32);
void SeqA_Start(s32);
void Loading_Stop(void);
void func_00D74550(u8);
void ScEvent_Update(void);
void Frame_Draw(void);
u8 SeqA_GetState(void);
void Pad_UpdateInput(void);
void Stream_FadeOut(s32, s32);
void CamReq_Cancel(void);
void TaskSlot_End(TaskSlot *t);
void Task_SetBootMode1(void);

extern u16 D_006ADB48;
/* The galex (gallery) quest from the player select: loads the quest, its
 * sounds and area, runs it (with the camera of the game overlay's table when
 * that overlay is loaded) and restores the scene. */
void Galex_QuestTask(TaskSlot *t) {
    u8 time;
    GalView *v;
    s32 k;
    u8 *buf;
    GalView view;
    u8 (*e)[2];
    u8 key;
    u16 area;
    s16 n;
    u32 bank;
    u8 f;
    s32 j;

    v = NULL;
    if (Overlay_IsLoaded(3) == 1) {
        v = &view;
        k = gSysSettings.galleryEntry & 0x7F;
        *v = *gGalleryScEvents[k];
        ScEvent_SetupB((ScEventHead *)v);
    }
    switch (t->mode) {
    case 0:
        Scene_LoadOverlay(3);
        t->step = 0;
        SeqA_Reset();
        t->mode++;
        memset(&gScene, 0, 0x2BC);
        func_36BC60();
        break;
    case 1:
        Game_ResetSubsystems();
        Gfx_ClearModelBuffers();
        Quest_Clear();
        gSysSettings.soundLoading = 1;
        t->mode++;
        t->step = 0;
        Packet_SetMode0();
        memset(gQuestExtra, 0, 0x80);
        f = gSysSettings.galleryEntry;
        j = f & 0x7F;
        D_006ADB48 = gGalexQuests[j].unk0;
        if (D_006ADB48 == 0) {
            D_006ADB48 = 0x3AB1;
        }
        time = f & 0x80 ? 4 : 3;
        e = gGalexDaytimes;
        key = (*e)[0];
        while (key != 0xFF) {
            if (key == j) {
                time = (*e)[1];
                break;
            }
            e++;
            key = (*e)[0];
        }
        Daytime_Set(time);
        Season_Set(gGalexQuests[j].unk4);
        if (D_006ADB48 == 0x3AB1) {
            Galex_SetupQuest();
        } else {
            Quest_Init();
        }
        ScEvent_StartFrom((ScEventHead *)v);
        area = v->area;
        if (time == 4) {
            area = Area_OtherTimeId(area);
        }
        gScene.area = area;
        gScene.startArea = area;
        gScene.playerCount = 0;
        gScene.localPlayer = 0;
        Hud_Start();
        Game_ResetSubsystems();
        Area_Load(gScene.startArea);
        Quest_InitPlayers();
        Quest_LoadCommonModels();
        func_327F70();
        func_3350B0();
        func_33A360();
        Quest_SpawnMonsters(1);
        Packet_SetMode2();
        break;
    case 2:
        buf = gLoadBuf;
        func_36BC60();
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
            n = (gScene.subStep - 6) / 3;
            Sound_StopBank(n + 2);
            FlushCache(0);
            SndLoad_Init(2);
            return;
        case 7:
        case 10:
        case 13:
        case 16:
            n = (gScene.subStep - 7) / 3;
            if (SndLoad_QuestBanks(buf, (u8 *)&gPlayers[n])) {
                gScene.subStep++;
                Sound_LoadStaged((struct SndFile *) buf, buf + 0x20000, n + 2);
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
                gSysSettings.soundLoading = 0;
                gScene.step++;
                gScene.subStep++;
                func_36BC60();
                t->mode++;
            }
            return;
        }
        break;
    case 3:
        gScene.musicFlags = 0;
        gScene.musicState = 0;
        Area_ApplyFog(gScene.area);
        Area_StartObjects(gScene.area);
        Bgm_EnterArea(gScene.area);
        gScene.eventRunning = 1;
        t->mode++;
        ScEvent_SetCamera(v->camera, v->camGroup);
        Mem_Compact();
        CamCtl_Update(1);
        gScene.eventRunning = 1;
        Galex_UpdateMonsters();
        SeqA_Start(2);
        gAreaState.drawSky = 1;
        Loading_Stop();
        gScene.galexTimer = 0;
        break;
    case 4:
        gScene.frame++;
        (gScene.galexTimer)++;
        func_00D74550(v->music);
        ScEvent_Update();
        Galex_UpdateMonsters();
        Frame_Draw();
        if (SeqA_GetState() != 1) {
            t->mode++;
        }
        break;
    case 5:
        Pad_UpdateInput();
        gScene.frame++;
        (gScene.galexTimer)++;
        func_00D74550(v->music);
        ScEvent_Update();
        Galex_UpdateMonsters();
        Frame_Draw();
        if ((gGalexQuests[(u8)k].limit != 0 && gGalexQuests[(u8)k].limit <= gScene.galexTimer) || (gPadState[0].pressed & 0x40) || gScene.eventRunning == 0) {
            t->mode++;
            Stream_FadeOut(0, 15);
            Stream_FadeOut(1, 15);
            SeqA_Start(1);
        }
        break;
    case 6:
        gScene.frame++;
        (gScene.galexTimer)++;
        Galex_UpdateMonsters();
        if (SeqA_GetState() != 1) {
            gScene.eventRunning = 0;
            CamReq_Cancel();
            t->mode++;
        }
        Frame_Draw();
        break;
    case 7:
        Game_ResetSubsystems();
        Gfx_ClearModelBuffers();
        memset(&gScene, 0, 0x2BC);
        TaskSlot_End(t);
        gSysSettings.gmSubLoaded = 0;
        gSysSettings.galleryEntry &= 0x7F;
        Scene_LoadOverlay(2);
        Task_SetBootMode1();
        SeqA_Start(2);
        gSysSettings.plselLoaded = 0;
        break;
    }
}

s32 Galex_StageIndex(void) {
    switch (gSysSettings.galleryEntry & 0x7F) {
    case 0x1F:
        return 0;
    case 0x20:
        return 1;
    case 0x21:
        return 2;
    case 0x22:
        return 3;
    case 0x23:
        return 4;
    case 0x24:
        return 5;
    }
    return 0;
}

extern s32 gLobbyStartPosIdx;
void Galex_PlayStageBgm(void) {
    switch (Galex_StageIndex()) {
    case 0:
        gLobbyStartPosIdx = 1;
        CamReq_Lobby44(0);
        Bgm_PlayField(0, 0);
        break;
    case 1:
        gLobbyStartPosIdx = 2;
        CamReq_Lobby44(1);
        Bgm_PlayField(0, 0);
        break;
    case 2:
        gLobbyStartPosIdx = 3;
        CamReq_Lobby44(2);
        Bgm_PlayField(0, 0);
        break;
    case 3:
        gLobbyStartPosIdx = 1;
        CamReq_Lobby44(3);
        Bgm_PlayField(0, 0);
        break;
    case 4:
        gLobbyStartPosIdx = 2;
        CamReq_Lobby44(4);
        Bgm_PlayField(0, 0);
        break;
    case 5:
        gLobbyStartPosIdx = 3;
        CamReq_Lobby44(5);
        Bgm_PlayField(0, 0);
        break;
    }
}

typedef struct GalVec {
    f32 x;
    f32 y;
    f32 z;
} GalVec;

void Scene_LoadOverlay(s32);
void SeqA_Reset(void);
void func_36BC60(void);
void *memset(void *, s32, u32);
/* unit's own declaration: with the definition's u16 (SaveData *save, u8 type, u16 id, u16 level) (sub_main/unk_3B4C80.c), Galex_Task: 1 of 323 words differ */
s32 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
s32 Equip_EquipSlot(SaveData *save, u16 idx);
void Equip_Refresh(SaveData *save);
void Town_Reset(void);
void Dmcam_PreloadAlt(void);
void Gfx_ClearModelBuffers(void);
void Scene_ResetAll(void);
void LobbyTime_SetClock(u32);
void LobbyTime_Tick(void);
s32 LobbyScreen_RunStep(void);
void Town_Frame(void);
s32 CamReq_State(void);
void Stream_FadeOut(s32, s32);
void SeqA_Start(s32);
u8 SeqA_GetState(void);
void Frame_Draw(void);
void CamReq_Cancel(void);
void Game_ResetSubsystems(void);
void Task_SetBootMode1(void);
extern GalVec gLobbyStartPos[];
extern u8 D_5F8A7C[];
extern u8 D_5F9124[];
s32 Galex_StageIndex(void);

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s32 gTownDay;
extern s32 gTownSeason;
extern s32 gTownSeasonDays[3];
extern s32 gTownDayTicks;
extern s8 gLobbyScreenMode;
extern s8 gLobbyScreenStep;
/* The galex (gallery quest) task: sets up the save, the player and the
 * area for the chosen galex quest, runs it until the time limit or a
 * cancel, then restores everything. */
void Galex_Task(TaskSlot *t) {
    s32 k;
    u8 (*tb)[2];
    u8 *e;
    u8 c;
    GalVec v;
    u16 lim;
    s32 pl;

    k = gSysSettings.galleryEntry & 0x7F;
    switch (t->mode) {
    case 0:
        Scene_LoadOverlay(4);
        t->step = 0;
        SeqA_Reset();
        t->mode++;
        func_36BC60();
        memset(gSave.pouch, 0, 0x50);
        memset(gSave.box, 0, 0x1770);
        memset(gSave.itemBox, 0, 0x7D0);
        gSave.equipIdx[1] = 0xFFFF;
        gSave.equipIdx[2] = 0xFFFF;
        gSave.equipIdx[3] = 0xFFFF;
        gSave.equipIdx[4] = 0xFFFF;
        gSave.equipIdx[5] = 0xFFFF;
        Equip_EquipSlot(&gSave, Box_AddItem(&gSave, 6, 0x9C, 0));
        Equip_Refresh(&gSave);
        gSave.equip[1].type = 0;
        gSave.equip[2].type = 2;
        gSave.equip[3].type = 3;
        gSave.equip[4].type = 4;
        gSave.equip[5].type = 5;
        tb = &gGalexDaytimes[12];
        e = tb[(s16)Galex_StageIndex()];
        c = e[1];
        gSave.facilityLevel[e[0]] = c;
        Town_Reset();
        Dmcam_PreloadAlt();
        Gfx_ClearModelBuffers();
        Scene_ResetAll();
        gTownSeason = 0;
        gScene.online = 1;
        gTownSeasonDays[0] = 3;
        gTownSeasonDays[1] = 3;
        gTownSeasonDays[2] = 3;
        gTownDayTicks = 86400;
        gTownDay = 0;
        if (gSysSettings.galleryEntry & 0x80) {
            LobbyTime_SetClock(0xD2F0);
        } else {
            LobbyTime_SetClock(0x2A30);
        }
        break;
    case 1:
        LobbyTime_Tick();
        LobbyScreen_RunStep();
        if (gLobbyScreenMode == 2) {
            Galex_PlayStageBgm();
            v = gLobbyStartPos[gLobbyStartPosIdx];
            pl = gScene.localPlayer * 0x970;
            *(GalVec *)(D_5F8A7C + pl) = v;
            *(GalVec *)(D_5F9124 + pl) = v;
            gLobbyScreenMode = 4;
            gTownMenuOpen = 1;
            gTownMode = 0x22;
            gFieldControl = 0;
            gLobbyScreenStep = 0;
            t->mode++;
        }
        gScene.galexTimer = 0;
        break;
    case 2:
        (gScene.galexTimer)++;
        gScene.frame++;
        Town_Frame();
        lim = gGalexQuests[(u8)k].limit;
        if ((lim != 0 && lim <= gScene.galexTimer) || (gPadState[0].pressed & 0x40) || CamReq_State() != 1) {
            Stream_FadeOut(0, 15);
            t->mode++;
            SeqA_Start(1);
        }
        gPlayers[0].visible = 0;
        Frame_Draw();
        break;
    case 3:
        (gScene.galexTimer)++;
        gScene.frame++;
        if (CamReq_State() == 1) {
            Town_Frame();
        }
        if (SeqA_GetState() != 1) {
            CamReq_Cancel();
            t->mode++;
        }
        gPlayers[0].visible = 0;
        Frame_Draw();
        break;
    case 4:
        Game_ResetSubsystems();
        Gfx_ClearModelBuffers();
        memset(&gScene, 0, 0x2BC);
        memset(&gSave, 0, sizeof(SaveData));
        TaskSlot_End(t);
        gSysSettings.gmSubLoaded = 0;
        gSysSettings.galleryEntry &= 0x7F;
        Scene_LoadOverlay(2);
        Task_SetBootMode1();
        SeqA_Start(2);
        gSysSettings.plselLoaded = 0;
        break;
    }
}

s32 func_00D75530(void) {
    return 0x64;
}

void Galex_RollLevel(GalObj *o) {
    u16 sum;
    s8 *p;
    u16 r;
    u16 acc;
    s8 lv;

    if (gQuest.flags & 0x4000) {
        o->unk36 = gQuest.emLevel;
        return;
    }
    p = gEmLevelTables[gEmLevelTableIdx[o->unk00][Daytime_Get()]];
    sum = 0;
    {
        s8 *e;

        for (e = p; *e != -1; e += 2) {
            sum += *e;
        }
    }
    r = Quest_Random() % sum;
    acc = 0;
    while (*p != -1) {
        if (r < (acc += *p++)) {
            break;
        }
        p++;
    }
    lv = *p + gEmLevelSeasonAdd[o->unk00][(u8)Quest_GetSeason()];
    if (lv > 2) {
        lv = 2;
    }
    o->unk36 = gQuest.emLevel + lv;
    if (o->unk36 > 16) {
        o->unk36 = 16;
    }
    if (o->unk36 < 0) {
        o->unk36 = 0;
    }
}

void Galex_InitObj(GalObj *o, s32 *cnt) {
    o->unk2E = 2;
    o->unk32 = -1;
    o->unk34 = -1;
    o->unk2C = *cnt;
    Galex_RollLevel(o);
    if ((o->unk35 = gEmIsLarge2[o->unk00]) != 0) {
        gQuest.emLeft += o->unk04;
    }
    (*cnt)++;
}

void Galex_LoadFiles(void) {
    Afs_LoadPartition(5, 0x19F4, gSysSettings.readBuf, "galex");
    Afs_ReadFileIdle(0x37, 5, (void *)gStageFile);
}

void Galex_PlaceRecords(u8 *p, s32 *cnt) {
    void *a;
    GalObj *b;
    u16 r;

    for (;;) {
        a = StageEntry_GetData((struct StageEntry *) p, 0);
        b = StageEntry_GetData((struct StageEntry *) p, 1);
        if (a == NULL) {
            break;
        }
        if (b != NULL) {
            for (;;) {
                if (b->unk00 <= 0) {
                    break;
                }
                Galex_InitObj(b, cnt);
                if (Em_HasRandomSize(b->unk00) == 1) {
                    b->unk30 = gQuest.header->emSize;
                    r = func_00D75530();
                    b->unk30 = b->unk30 * r / 100;
                    b->unk30 += gEmSizeSeasonAdd[b->unk00][(u8)Quest_GetSeason()];
                } else {
                    b->unk30 = 100;
                }
                b++;
            }
        }
        p += 0x10;
    }
}

/* Galex_PlaceRecords for a NULL-terminated list of record lists (offsets into
 * the stage file when 0x5D8268 is set), every object at the default size.
 * The then-branch re-assigns the value p already holds: MWCC deletes the
 * store but keeps the branch over the else block (bnez, then `b join`). */
void Galex_PlaceObjectLists(u8 **list, s32 *cnt) {
    GalObj *b;
    void *a;
    u8 *p;

    for (;;) {
        p = *list;
        if (p == NULL) {
            break;
        }
        if (gQuest.stage == 0) {
            p = *list;
        } else {
            p = *list + gStageFile;
        }
        for (;;) {
            a = StageEntry_GetData((struct StageEntry *) p, 0);
            b = StageEntry_GetData((struct StageEntry *) p, 1);
            p += 0x10;
            if (a == NULL) {
                break;
            }
            if (b != NULL) {
                for (;;) {
                    if (b->unk00 <= 0) {
                        break;
                    }
                    Galex_InitObj(b, cnt);
                    b->unk30 = 100;
                    b++;
                }
            }
        }
        list++;
    }
}

extern u8 D_6ADB30[];
extern u16 D_006ADB48;
extern s8 D_006ADB4C;
void *memcpy(void *, const void *, u32);
void Bgm_Init(void);
u16 Rand_Next(s32);
u16 Save_RankFromPoints(SaveData *s);
void Quest_SetStartArea(u16);
s32 Quest_GetTime(s32);
void func_90AAA0(void);

/* Sets up the quest state (gQuest, 0x5D8260) for the quest being started:
 * clears it, draws its 15 random bytes, the rank of the local player, the
 * time limit (in 9000-frame steps), the pointers into the stage file
 * (gStageFile, offsets from its header) and the two sub-quest records, then
 * places the objects of the two record lists.
 * It is a copy of sub_main's Quest_Init tail (unk_3A1070.c) with gStageFile
 * read from gSysSettings and Quest_LoadTargets inlined: written with gQuest's
 * fields (named: the stores don't alias the loads).
 * Quest_LoadTargets is inlined here (Galex_LoadTargets, its copy), and the
 * stage file base is held in i, the counter of the random bytes' loop: with
 * a separate base local, or the loop written out, the base and the targets
 * walker trade v1 and a1.
 * The last function of plsel's .text: the zero padding after it is plsel.yaml's
 * `text_end` piece (0x15D3C), as docs/mwcc-matching.md, "Data and .rodata
 * placement", describes. */
static inline void Galex_LoadTargets(void)
{
    QuestTarget *dst = gQuest.targets;
    QuestTarget *src = gQuest.header->targets;
    s32 i;

    for (i = 0; i < 2; i++, dst++, src++) {
        if (src->state != 0) {
            dst->state = src->state;
            dst->item = src->item;
            dst->count = src->count;
            dst->idx = i + 1;
        }
    }
}

void Galex_SetupQuest(void) {
    s32 cnt;
    u16 id;
    s8 c;
    QuestTarget *dst;
    s32 i;
    u8 *q;
    StageHeader *h;
    u32 *p;

    Galex_LoadFiles();
    id = D_006ADB48;
    c = D_006ADB4C;
    gScene.questId = id;
    gQuest.stage = id;
    gQuest.unk15C = c;
    memcpy(&gQuest.huntGoals, &D_6ADB30, 0x18);
    gScene.rewardDeducted = 0;
    gScene.questPhase = 2;
    Bgm_Init();
    dst = (QuestTarget *)(u8 *)gQuest.targets;
    dst[0].state = 0;
    dst[0].item = 0;
    dst[0].count = 0;
    dst[1].state = 0;
    dst[1].item = 0;
    dst[1].count = 0;
    for (i = 0, q = (u8 *)&gQuest; i < 15; i++, q++) {
        q[0x139] = Rand_Next(0) & 0xFFFF;
    }
    gQuest.capturedType = 0;
    gQuest.capturedSize = 0;
    gQuest.capturedLevel = 0;
    gQuest.acks = 0;
    gQuest.netMsgArgs = 0;
    gQuest.unk138 = 0;
    gQuest.unk137 = 0;
    gQuest.goalLabel = 0;
    gQuest.hunterRank[0][0] = 0;
    gQuest.hunterRank[0][1] = 0;
    gQuest.hunterRank[1][0] = 0;
    gQuest.hunterRank[1][1] = 0;
    gQuest.hunterRank[2][0] = 0;
    gQuest.hunterRank[2][1] = 0;
    gQuest.hunterRank[3][0] = 0;
    gQuest.hunterRank[3][1] = 0;
    gQuest.emLeft = 0;
    gQuest.savedEmParts = 0;
    gQuest.savedEmHp = 0;
    gQuest.anyFainted = 0;
    gQuest.hunterRank[gScene.localPlayer][1] = Save_RankFromPoints(&gSave);
    gQuest.unk096 = 1;
    gScene.emKind[0] = 0;
    gScene.emKind[1] = 0;
    gScene.emKind[2] = 0;
    gScene.emKind[3] = 0;
    gQuest.rewards[0].id = 0;
    gQuest.rewards[0].val = 0;
    gQuest.rewards[1].id = 0;
    gQuest.rewards[1].val = 0;
    gQuest.rewards[2].id = 0;
    gQuest.rewards[2].val = 0;
    gQuest.rewards[3].id = 0;
    gQuest.rewards[3].val = 0;
    gQuest.rewards[4].id = 0;
    gQuest.rewards[4].val = 0;
    gQuest.rewardNext = 0;
    gQuest.rewardsPending = 0;
    gQuest.unk003 = 0;
    gQuest.state = 0;
    gQuest.step = 0;
    gQuest.progress = 0;
    gQuest.downEm = NULL;
    gQuest.timeLeft = 0;
    gQuest.savedEmType = 0;
    gQuest.unk15B = 0;
    gQuest.traderWantItem = 0;
    h = (StageHeader *) gSysSettings.stageFile;
    gQuest.info = (QuestInfo *) (h->info + gSysSettings.stageFile);
    gQuest.inputLock = 60;
    gQuest.header = h;
    gQuest.nextSide = 0;
    Quest_SetStartArea(0xFF);
    gQuest.timeLeft = Quest_GetTime(1);
    gQuest.timeNoticeIdx = (gQuest.timeLeft - 1) / 9000;
    if (gQuest.timeNoticeIdx > 2) {
        gQuest.timeNoticeIdx = 2;
    }
    i = gSysSettings.stageFile;
    gQuest.script = (void *) (h->script + i);
    gQuest.loopPc = 0;
    gQuest.scriptPc = 0;
    gQuest.areaData = (void *) (h->areaData + i);
    gQuest.noticeTexts = (void *) (h->unk30 + i);
    gQuest.texts = (void *) (gQuest.info->texts + i);
    gQuest.emSpawns = (void *) (h->emSpawns + i);
    gQuest.stageEntry = (void *) (h->unk18 + i);
    gQuest.areaRecords = (void *) (h->areaRecords + i);
    gQuest.gatherPoints = (void *) (h->gatherPoints + i);
    gQuest.actionSpots = (void *) (h->actionSpots + i);
    gQuest.dataTable = (void *) (h->dataTable + i);
    gQuest.seasonalData = (void *) (h->seasonalData + i);
    gQuest.reward = gQuest.info->reward;
    gQuest.penalty = gQuest.info->penalty;
    gQuest.rewardTable = (void *) (h->unk0C + i);
    gQuest.flags = gQuest.info->flags;
    gQuest.emLevel = h->emLevel;
    gQuest.header->tables += gSysSettings.stageFile;
    p = (u32 *) (h->stageInfo + gSysSettings.stageFile);
    gScene.map = p[0];
    gScene.campArea = p[1];
    gQuest.timedEvent = h->timedEvent;
    gQuest.eventKind = h->eventKind;
    gQuest.eventCount = h->eventCount;
    gQuest.timedEventSet = gQuest.timeLeft / 9000;
    gQuest.unk09C = 0;
    gQuest.unk15A = 0;
    Galex_LoadTargets();
    cnt = 0;
    Galex_PlaceRecords((u8 *) gQuest.stageEntry, &cnt);
    Galex_PlaceObjectLists((u8 **) gQuest.emSpawns, &cnt);
    func_90AAA0();
}

/* Zeros up to 0xD7BD80, the end of .rodata: the game's linker ended each
 * section on a 0x80-byte boundary (.text at 0xD75F80, .data at 0xD79580). */
const u8 D_00D7BD40[0x40] = {0};
