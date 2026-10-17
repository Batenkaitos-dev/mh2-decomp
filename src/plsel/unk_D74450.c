/*
 * The gallery's scenes ("galex", plsel 0xD74450-0xD75F80): the two tasks
 * that select's gallery (select_gallery.c) starts to play an entry, which
 * gSysSettings.galleryEntry names.
 *
 * - Galex_QuestTask plays a monster's or the world's entry: it sets up the
 *   entry's quest (or the gallery's own stage file, Galex_SetupQuest),
 *   loads its sounds and area, then runs the game overlay's scene event
 *   for the entry (gGalleryScEvents) until its time limit, a cancel or the
 *   event's end.
 * - Galex_Task plays a town tour with the lobby overlay's town code, with
 *   one village facility raised to the tour's level.
 * Both go back to select at the end (Task_StartBootToGmSub).
 *
 * Galex_SetupQuest and the quest-monster functions before it are copies of
 * sub_main's Quest_Init and QuestEm_* (unk_3A1070.c) for the gallery's
 * stage file. gGalexQuests and gGalexEntryPairs hold the entries' settings.
 */
#include "common.h"
#include "plsel.h"
#include "overlay.h"
#include "select.h"
extern u8 gFieldControl;
extern s32 gStageFile; /* sub_main: the stage file's address */
#include "scene.h"
#include "task.h"
#include "pad.h"
#include "em.h"
#include "save.h"
#include "player.h"
#include "syssettings.h"
#include "quest.h"
#include "scevent.h"
#include "area.h"

void Afs_LoadPartition(s32 ptid, s32 flid, void *ptinfo, char *name);
s32 Afs_ReadFileIdle(s32 flid, s32 ptid, void *buf);

void Galex_RollEmLevel(QuestEm *qem);
extern u8 gEmShowsOnMap[];

void CamReq_Lobby44(s32);
void Bgm_PlayField(s32, s32);
struct StageEntry;
void *StageEntry_GetData(struct StageEntry *e, s32 which);
void Em_CopyBoneMatrices(Em *);
s32 GmEm_Task(Em *em);
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
/* the calls cast its value to u8, as the game's code takes it */
s32 Quest_GetSeason(void);
extern u16 gEmSizeSeasonAdd[][3];

u8 Daytime_Get(void);
u16 Quest_Random(void);
extern s8 *gEmLevelTables[];
extern s8 gEmLevelTableIdx[][2];
extern s8 gEmLevelSeasonAdd[][3];

/* .data (0xD79370-0xD79580) */

GalQuest gGalexQuests[46] = {
#include "plsel/00D79370.inc"
};
/* gGalexEntryPairs (0xD794E0): two lists of byte pairs. From row 0, the
 * entries played at a fixed time of day: (entry, Daytime_Set's
 * DAYTIME_SET_* value), ended by a GALEX_PAIRS_END entry. From row
 * GALEX_TOUR_FACILITY_ROW, per town tour (Galex_TourIndex): the village
 * facility (SaveData.facilityLevel's index) and the level the tour shows. */
u8 gGalexEntryPairs[80][2] = {
#include "plsel/00D794E0.inc"
};

/*
 * The monsters' part of the field's frame, as sub_main's Field_UpdateAll
 * runs it without the players, the push-apart or the HUD: each active
 * monster's bones and game-overlay task, the streams' fades, a second field
 * collision for the monsters the push moved, then the camera, the lights,
 * the stage objects, the attachments, the objects and the area timers.
 * The second loop has its own counter and pointer, as the game's code does.
 */
void Galex_UpdateMonsters(void)
{
    s32 i;
    Em *em;
    Em *pushedEm;
    s32 j;

    for (i = 0, em = gEms; i < EM_COUNT; i++, em++) {
        if (em->active != 0) {
            Em_CopyBoneMatrices(em);
            if (GmEm_Task(em) == 0) {
                Em_UpdateMatrix(em);
                Em_CarryRiders(em);
            }
        }
    }
    Stream_UpdateFades();
    for (j = 0, pushedEm = gEms; j < EM_COUNT; j++, pushedEm++) {
        if (pushedEm->active != 0 && pushedEm->pushed != 0) {
            Em_CollideField(pushedEm, 1);
        }
    }
    CamCtl_Apply();
    LightSet_UpdateAll();
    StageObj_UpdateAll();
    ObjAttach_UpdateAll();
    Obj_UpdateAll();
    Area_UpdateTimers();
}

/* Empty: Galex_QuestTask calls it each frame with the scene event's music
 * value (ScEventHead.music). */
void Galex_MusicHook(u8 music)
{
}

s32 Overlay_IsLoaded(s32);
extern ScEventHead *gGalleryScEvents[];
void ScEvent_SetupB(ScEventHead *h);
void Scene_LoadOverlay(s32);
void FadeA_Reset(void);
void func_36BC60(void);
void *memset(void *, s32, u32);
void Game_ResetSubsystems(void);
void Gfx_ClearModelBuffers(void);
void Quest_Clear(void);
void Packet_SetMode0(void);
extern NextQuest gNextQuest;
void Daytime_Set(u8); /* unit's own declaration: u8 here; with the definition's s32 (main), Galex_QuestTask's code differs */
void Season_Set(s32);
void Galex_SetupQuest(void);
void Quest_Init(void);
void ScEvent_StartFrom(ScEventHead *h);
u16 Area_OtherTimeId(u16);
void Hud_Start(void);
void Area_Load(s32);
void Quest_InitPlayers(void);
void Quest_LoadCommonModels(void);
void Quest_SetupStub(void);
void Quest_EmptyHook(void);
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
void FadeA_Start(s32);
void Loading_Stop(void);
void Galex_MusicHook(u8);
void ScEvent_Update(void);
void Frame_Draw(void);
u8 FadeA_GetState(void);
void Pad_UpdateInput(void);
void Stream_FadeOut(s32, s32);
void CamReq_Cancel(void);
void TaskSlot_End(TaskSlot *t);
void Task_StartBootToGmSub(void);

extern u16 gNextQuestId;
/*
 * The task that plays a monster's or the world's gallery entry (TaskSlot
 * mode GALEX_QUEST_*): loads the game overlay, sets up the entry's quest
 * at its time of day and season, loads the sound banks one step a frame,
 * starts the area with the entry's scene event (a copy of its record in
 * gGalleryScEvents, read once the game overlay is in), and plays it until
 * the entry's time limit, PAD_CROSS or the event's end. Then it fades out
 * and goes back to select.
 */
void Galex_QuestTask(TaskSlot *task)
{
    u8 daytime;
    ScEventHead *event;
    s32 eventEntry;
    u8 *loadBuf;
    ScEventHead eventCopy;
    u8 (*pair)[2];
    u8 pairEntry;
    u16 area;
    s16 player;
    u32 bankId;
    u8 entryValue;
    s32 entry;

    event = NULL;
    if (Overlay_IsLoaded(OVERLAY_GAME) == 1) {
        event = &eventCopy;
        eventEntry = gSysSettings.galleryEntry & GAL_ENTRY_MASK;
        *event = *gGalleryScEvents[eventEntry];
        ScEvent_SetupB(event);
    }
    switch (task->mode) {
    case GALEX_QUEST_LOAD:
        Scene_LoadOverlay(OVERLAY_GAME);
        task->step = 0;
        FadeA_Reset();
        task->mode++;
        memset(&gScene, 0, sizeof(gScene));
        func_36BC60();
        break;
    case GALEX_QUEST_SETUP:
        Game_ResetSubsystems();
        Gfx_ClearModelBuffers();
        Quest_Clear();
        gSysSettings.soundLoading = 1;
        task->mode++;
        task->step = 0;
        Packet_SetMode0();
        memset(&gNextQuest, 0, sizeof(gNextQuest));
        entryValue = gSysSettings.galleryEntry;
        entry = entryValue & GAL_ENTRY_MASK;
        gNextQuestId = gGalexQuests[entry].questId;
        if (gNextQuestId == 0) {
            gNextQuestId = GALEX_QUEST_ID;
        }
        daytime = entryValue & GAL_ENTRY_NIGHT ? DAYTIME_SET_NIGHT : DAYTIME_SET_DAY;
        pair = gGalexEntryPairs;
        pairEntry = (*pair)[0];
        while (pairEntry != GALEX_PAIRS_END) {
            if (pairEntry == entry) {
                daytime = (*pair)[1];
                break;
            }
            pair++;
            pairEntry = (*pair)[0];
        }
        Daytime_Set(daytime);
        Season_Set(gGalexQuests[entry].season);
        if (gNextQuestId == GALEX_QUEST_ID) {
            Galex_SetupQuest();
        } else {
            Quest_Init();
        }
        ScEvent_StartFrom(event);
        area = event->area;
        if (daytime == DAYTIME_SET_NIGHT) {
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
        Quest_SetupStub();
        Quest_EmptyHook();
        func_33A360();
        Quest_SpawnMonsters(1);
        Packet_SetMode2();
        break;
    case GALEX_QUEST_SOUNDS:
        /* Each bank: queue its file, load it when the file queue is idle,
         * then wait for its transfer. Steps 0-2 the common bank (slot 1),
         * 3-5 the area's sounds (slot 7), 6-17 each player's banks (slots
         * 2-5, three steps per player), 18-20 the monsters' (slot 6). */
        loadBuf = gLoadBuf;
        func_36BC60();
        switch (gScene.subStep) {
        case 0:
            gScene.subStep++;
            gScene.openingFirst = 0;
            Sound_StopBank(1);
            Sound_SetSpuMapPage(1);
            FlushCache(0);
            Sound_QueueBank(0x10001, loadBuf);
            return;
        case 1:
            if (!FileQueue_IsBusy()) {
                gScene.subStep++;
                Sound_LoadBank((struct SndFile *) loadBuf, 1);
            }
            return;
        case 2:
            if (!Sound_CheckTransfer()) {
                gScene.subStep++;
            }
            return;
        case 3:
            Sound_StopBank(7);
            bankId = (gAreaSeBanks[gScene.area] + 0xB) | 0x10000;
            FlushCache(0);
            Sound_QueueBank(bankId, loadBuf);
            gScene.subStep++;
            return;
        case 4:
            if (!FileQueue_IsBusy()) {
                gScene.subStep++;
                Sound_LoadBank((struct SndFile *) loadBuf, 7);
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
            if (SndLoad_QuestBanks(loadBuf, (u8 *)&gPlayers[player])) {
                gScene.subStep++;
                Sound_LoadStaged((struct SndFile *) loadBuf, loadBuf + 0x20000, player + 2);
            }
            return;
        case 8:
        case 11:
        case 14:
        case 17:
            if (!Sound_CheckTransfer()) {
                if (gScene.subStep == 17) {
                    Sound_StopBank(6);
                    SndLoad_Init(4);
                }
                gScene.subStep++;
            }
            return;
        case 18:
            if (SndLoad_ListBanks(loadBuf, gScene.emKind)) {
                gScene.subStep++;
            }
            return;
        case 19:
            if (!FileQueue_IsBusy()) {
                gScene.subStep++;
                Sound_LoadStaged((struct SndFile *) loadBuf, loadBuf + 0x4800, 6);
            }
            return;
        case 20:
            if (!Sound_CheckTransfer()) {
                gSysSettings.soundLoading = 0;
                gScene.step++;
                gScene.subStep++;
                func_36BC60();
                task->mode++;
            }
            return;
        }
        break;
    case GALEX_QUEST_START:
        gScene.musicFlags = 0;
        gScene.musicState = 0;
        Area_ApplyFog(gScene.area);
        Area_StartObjects(gScene.area);
        Bgm_EnterArea(gScene.area);
        gScene.eventRunning = 1;
        task->mode++;
        ScEvent_SetCamera(event->camera, event->camGroup);
        Mem_Compact();
        CamCtl_Update(1);
        gScene.eventRunning = 1;
        Galex_UpdateMonsters();
        FadeA_Start(2);
        gAreaState.drawSky = 1;
        Loading_Stop();
        gScene.galexTimer = 0;
        break;
    case GALEX_QUEST_FADE_IN:
        gScene.frame++;
        (gScene.galexTimer)++;
        Galex_MusicHook(event->music);
        ScEvent_Update();
        Galex_UpdateMonsters();
        Frame_Draw();
        if (FadeA_GetState() != 1) {
            task->mode++;
        }
        break;
    case GALEX_QUEST_PLAY:
        Pad_UpdateInput();
        gScene.frame++;
        (gScene.galexTimer)++;
        Galex_MusicHook(event->music);
        ScEvent_Update();
        Galex_UpdateMonsters();
        Frame_Draw();
        if ((gGalexQuests[(u8)eventEntry].limit != 0 && gGalexQuests[(u8)eventEntry].limit <= gScene.galexTimer) || (gPadState[0].pressed & PAD_CROSS) || gScene.eventRunning == 0) {
            task->mode++;
            Stream_FadeOut(0, 15); /* frames */
            Stream_FadeOut(1, 15);
            FadeA_Start(1);
        }
        break;
    case GALEX_QUEST_FADE_OUT:
        gScene.frame++;
        (gScene.galexTimer)++;
        Galex_UpdateMonsters();
        if (FadeA_GetState() != 1) {
            gScene.eventRunning = 0;
            CamReq_Cancel();
            task->mode++;
        }
        Frame_Draw();
        break;
    case GALEX_QUEST_END:
        Game_ResetSubsystems();
        Gfx_ClearModelBuffers();
        memset(&gScene, 0, sizeof(gScene));
        TaskSlot_End(task);
        gSysSettings.gmSubLoaded = 0;
        gSysSettings.galleryEntry &= GAL_ENTRY_MASK;
        Scene_LoadOverlay(OVERLAY_SELECT);
        Task_StartBootToGmSub();
        FadeA_Start(2);
        gSysSettings.plselLoaded = 0;
        break;
    }
}

/* The town tour (0-5) of the entry being played: gallery entries 0x1F to
 * 0x24 in order; 0 for any other. */
s32 Galex_TourIndex(void)
{
    switch (gSysSettings.galleryEntry & GAL_ENTRY_MASK) {
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
/* Starts the town tour's camera (CamReq_Lobby44 with the tour's number)
 * and the field music, and picks the start position the player is placed
 * at (gLobbyStartPosIdx: 1, 2, 3 for tours 0-2 and again for 3-5). */
void Galex_StartTourCamera(void)
{
    switch (Galex_TourIndex()) {
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

/* A position, copied whole (a 12-byte struct copy in the game's code). */
typedef struct GalVec {
    f32 x;
    f32 y;
    f32 z;
} GalVec;

void Scene_LoadOverlay(s32);
void FadeA_Reset(void);
void func_36BC60(void);
void *memset(void *, s32, u32);
/* unit's own declaration: s32 here; with the definition's u16 return (sub_main/unk_3B4C80.c), Galex_Task's code differs */
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
void FadeA_Start(s32);
u8 FadeA_GetState(void);
void Frame_Draw(void);
void CamReq_Cancel(void);
void Game_ResetSubsystems(void);
void Task_StartBootToGmSub(void);
extern GalVec gLobbyStartPos[];
s32 Galex_TourIndex(void);

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s32 gTownDay;
extern s32 gTownSeason;
extern s32 gTownSeasonDays[3];
extern s32 gTownDayTicks;
extern s8 gLobbyScreenMode;
extern s8 gLobbyScreenStep;
/*
 * The task that plays a town tour (TaskSlot mode GALEX_TOUR_*): loads the
 * lobby overlay and sets up a save with nothing but one blade weapon,
 * the tour's village facility at its level, and the town's clock (the
 * night variant's time or the day's). Once the lobby's screen is in the
 * town, it places the player at the tour's start position, starts the
 * tour's camera and runs the town until the time limit, PAD_CROSS or the
 * camera's end; then it fades out, clears the save and goes back to
 * select.
 */
void Galex_Task(TaskSlot *task)
{
    s32 entry;
    u8 (*tourPairs)[2];
    u8 *pair;
    u8 level;
    GalVec startPos;
    u16 timeLimit;
    s32 playerOfs;

    entry = gSysSettings.galleryEntry & GAL_ENTRY_MASK;
    switch (task->mode) {
    case GALEX_TOUR_SETUP:
        Scene_LoadOverlay(OVERLAY_LOBBY);
        task->step = 0;
        FadeA_Reset();
        task->mode++;
        func_36BC60();
        memset(gSave.pouch, 0, sizeof(gSave.pouch));
        memset(gSave.box, 0, sizeof(gSave.box));
        memset(gSave.itemBox, 0, sizeof(gSave.itemBox));
        gSave.equipIdx[1] = 0xFFFF;
        gSave.equipIdx[2] = 0xFFFF;
        gSave.equipIdx[3] = 0xFFFF;
        gSave.equipIdx[4] = 0xFFFF;
        gSave.equipIdx[5] = 0xFFFF;
        /* a blade weapon (EquipItem.type 6, id 0x9C), equipped */
        Equip_EquipSlot(&gSave, Box_AddItem(&gSave, 6, 0x9C, 0));
        Equip_Refresh(&gSave);
        gSave.equip[1].type = 0;
        gSave.equip[2].type = 2;
        gSave.equip[3].type = 3;
        gSave.equip[4].type = 4;
        gSave.equip[5].type = 5;
        tourPairs = &gGalexEntryPairs[GALEX_TOUR_FACILITY_ROW];
        pair = tourPairs[(s16)Galex_TourIndex()];
        level = pair[1];
        gSave.facilityLevel[pair[0]] = level;
        Town_Reset();
        Dmcam_PreloadAlt();
        Gfx_ClearModelBuffers();
        Scene_ResetAll();
        gTownSeason = 0;
        gScene.online = 1;
        gTownSeasonDays[0] = 3;
        gTownSeasonDays[1] = 3;
        gTownSeasonDays[2] = 3;
        gTownDayTicks = 86400; /* seconds */
        gTownDay = 0;
        if (gSysSettings.galleryEntry & GAL_ENTRY_NIGHT) {
            LobbyTime_SetClock(54000);
        } else {
            LobbyTime_SetClock(10800);
        }
        break;
    case GALEX_TOUR_ENTER:
        LobbyTime_Tick();
        LobbyScreen_RunStep();
        if (gLobbyScreenMode == 2) {
            Galex_StartTourCamera();
            startPos = gLobbyStartPos[gLobbyStartPosIdx];
            /* the local player's pos and moveTo, reached from player 0's
             * fields by the slot's offset (the game's code adds it so) */
            playerOfs = gScene.localPlayer * sizeof(Player);
            *(GalVec *)((u8 *)gPlayers[0].pos + playerOfs) = startPos;
            *(GalVec *)((u8 *)gPlayers[0].moveTo + playerOfs) = startPos;
            gLobbyScreenMode = 4;
            gTownMenuOpen = 1;
            gTownMode = 0x22;
            gFieldControl = 0;
            gLobbyScreenStep = 0;
            task->mode++;
        }
        gScene.galexTimer = 0;
        break;
    case GALEX_TOUR_PLAY:
        (gScene.galexTimer)++;
        gScene.frame++;
        Town_Frame();
        timeLimit = gGalexQuests[(u8)entry].limit;
        if ((timeLimit != 0 && timeLimit <= gScene.galexTimer) || (gPadState[0].pressed & PAD_CROSS) || CamReq_State() != 1) {
            Stream_FadeOut(0, 15); /* frames */
            task->mode++;
            FadeA_Start(1);
        }
        gPlayers[0].visible = 0;
        Frame_Draw();
        break;
    case GALEX_TOUR_FADE_OUT:
        (gScene.galexTimer)++;
        gScene.frame++;
        if (CamReq_State() == 1) {
            Town_Frame();
        }
        if (FadeA_GetState() != 1) {
            CamReq_Cancel();
            task->mode++;
        }
        gPlayers[0].visible = 0;
        Frame_Draw();
        break;
    case GALEX_TOUR_END:
        Game_ResetSubsystems();
        Gfx_ClearModelBuffers();
        memset(&gScene, 0, sizeof(gScene));
        memset(&gSave, 0, sizeof(SaveData));
        TaskSlot_End(task);
        gSysSettings.gmSubLoaded = 0;
        gSysSettings.galleryEntry &= GAL_ENTRY_MASK;
        Scene_LoadOverlay(OVERLAY_SELECT);
        Task_StartBootToGmSub();
        FadeA_Start(2);
        gSysSettings.plselLoaded = 0;
        break;
    }
}

/* The size roll of sub_main's QuestEm_RollSize, fixed: 100 (percent). */
s32 Galex_RollEmSize(void)
{
    return 100;
}

/* Rolls the level of quest monster qem from its kind's weighted table for
 * the time of day, plus the season's step (at most 2), clamped to 0..16;
 * with QUEST_FLAG_FIXED_LEVEL, the quest's level as it is (sub_main's
 * QuestEm_RollLevel). */
void Galex_RollEmLevel(QuestEm *qem)
{
    u16 sum;
    s8 *entry;
    u16 roll;
    u16 running;
    s8 levelAdd;

    if (gQuest.flags & QUEST_FLAG_FIXED_LEVEL) {
        qem->level = gQuest.emLevel;
        return;
    }
    entry = gEmLevelTables[gEmLevelTableIdx[qem->kind][Daytime_Get()]];
    sum = 0;
    {
        s8 *weight;

        for (weight = entry; *weight != -1; weight += 2) {
            sum += *weight;
        }
    }
    roll = Quest_Random() % sum;
    running = 0;
    while (*entry != -1) {
        if (roll < (running += *entry++)) {
            break;
        }
        entry++;
    }
    levelAdd = *entry + gEmLevelSeasonAdd[qem->kind][(u8)Quest_GetSeason()];
    if (levelAdd > 2) {
        levelAdd = 2;
    }
    qem->level = gQuest.emLevel + levelAdd;
    if (qem->level > 16) {
        qem->level = 16;
    }
    if (qem->level < 0) {
        qem->level = 0;
    }
}

/* Sets up quest monster qem as number *nextIndex, rolls its level and counts
 * a large one's appearances in gQuest.emLeft (sub_main's QuestEm_Init). */
void Galex_InitQuestEm(QuestEm *qem, s32 *nextIndex)
{
    qem->flags = QUEST_EM_ACTIVE;
    qem->hp = -1;
    qem->emIndex = -1;
    qem->index = *nextIndex;
    Galex_RollEmLevel(qem);
    if ((qem->large = gEmShowsOnMap[qem->kind]) != 0) {
        gQuest.emLeft += qem->count;
    }
    (*nextIndex)++;
}

/* Opens the gallery's partition of the AFS (partition 5, its table in
 * gSysSettings.readBuf) and reads its stage file (file 0x37) into the
 * stage file's buffer. */
void Galex_LoadStageFile(void)
{
    Afs_LoadPartition(5, 0x19F4, gSysSettings.readBuf, "galex");
    Afs_ReadFileIdle(0x37, 5, (void *)gStageFile);
}

/* Sets up the quest monsters of the stage's monster sets (0x10-byte
 * entries until one has no count), with the sizes of the kinds that roll
 * one (sub_main's QuestEm_InitSets). */
void Galex_InitEmSets(u8 *set, s32 *nextIndex)
{
    void *count;
    QuestEm *qem;
    u16 sizePct;

    for (;;) {
        count = StageEntry_GetData((struct StageEntry *) set, 0);
        qem = StageEntry_GetData((struct StageEntry *) set, 1);
        if (count == NULL) {
            break;
        }
        if (qem != NULL) {
            for (;;) {
                if (qem->kind <= 0) {
                    break;
                }
                Galex_InitQuestEm(qem, nextIndex);
                if (Em_HasRandomSize(qem->kind) == 1) {
                    qem->size = gQuest.header->emSize;
                    sizePct = Galex_RollEmSize();
                    qem->size = qem->size * sizePct / 100;
                    qem->size += gEmSizeSeasonAdd[qem->kind][(u8)Quest_GetSeason()];
                } else {
                    qem->size = 100;
                }
                qem++;
            }
        }
        set += 0x10;
    }
}

/* Galex_InitEmSets for each list of a NULL-terminated list of monster
 * sets (pointers for quest 0, else offsets into the stage file), every
 * monster at size 100 (sub_main's QuestEm_InitAreaSets). The first
 * branch of the quest test stores the value `set` already holds: the
 * game's code has the test and its branch. */
void Galex_InitEmAreaSets(u8 **list, s32 *nextIndex)
{
    QuestEm *qem;
    void *count;
    u8 *set;

    for (;;) {
        set = *list;
        if (set == NULL) {
            break;
        }
        if (gQuest.stage == 0) {
            set = *list;
        } else {
            set = *list + gStageFile;
        }
        for (;;) {
            count = StageEntry_GetData((struct StageEntry *) set, 0);
            qem = StageEntry_GetData((struct StageEntry *) set, 1);
            set += 0x10;
            if (count == NULL) {
                break;
            }
            if (qem != NULL) {
                for (;;) {
                    if (qem->kind <= 0) {
                        break;
                    }
                    Galex_InitQuestEm(qem, nextIndex);
                    qem->size = 100;
                    qem++;
                }
            }
        }
        list++;
    }
}

extern u16 gNextQuestId;
void *memcpy(void *, const void *, u32);
void Bgm_Init(void);
u16 Rand_Next(s32);
u16 Save_RankFromPoints(SaveData *s);
void Quest_SetStartArea(u16);
s32 Quest_GetTime(s32);
void GmLoad_Reset(void);

/* sub_main's Quest_LoadTargets, inlined into Galex_SetupQuest: copies the
 * stage's sub-quest targets that are set, numbered 1 and 2. */
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

/*
 * Sets up the quest state (gQuest) for the gallery's own stage file, as
 * sub_main's Quest_Init does for a quest: reads the stage file
 * (Galex_LoadStageFile), clears the quest's records, rolls its seed bytes,
 * takes the local player's rank, the time limit, the pointers into the
 * stage file (offsets from its header, gSysSettings.stageFile) and the
 * sub-quest targets, then sets up the quest monsters of the two kinds of
 * monster lists.
 * The stage file's address is kept in i, the seed loop's counter, after
 * the loop: the game's code keeps both in one register. The last function
 * of plsel's .text: func_diff counts the zero padding after it
 * (plsel.yaml's text_end piece) with it, and so reports a DIFF on it.
 */
void Galex_SetupQuest(void)
{
    s32 nextIndex;
    u16 questId;
    s8 flyer;
    QuestTarget *target;
    s32 i;
    u8 *questByte;
    StageHeader *header;
    u32 *mapInfo;

    Galex_LoadStageFile();
    questId = gNextQuestId;
    flyer = gNextQuest.flyer;
    gScene.questId = questId;
    gQuest.stage = questId;
    gQuest.flyer = flyer;
    memcpy(&gQuest.huntGoals, &gNextQuest.huntGoals, sizeof(gQuest.huntGoals));
    gScene.rewardDeducted = 0;
    gScene.questPhase = QUEST_PHASE_READY;
    Bgm_Init();
    target = gQuest.targets;
    target[0].state = 0;
    target[0].item = 0;
    target[0].count = 0;
    target[1].state = 0;
    target[1].item = 0;
    target[1].count = 0;
    /* questByte walks gQuest a byte at a time, so its seed[0] is gQuest.seed[i] */
    for (i = 0, questByte = (u8 *)&gQuest; i < (s32)sizeof(gQuest.seed); i++, questByte++) {
        ((QuestState *)questByte)->seed[0] = Rand_Next(0) & 0xFFFF;
    }
    gQuest.capturedType = 0;
    gQuest.capturedSize = 0;
    gQuest.capturedLevel = 0;
    gQuest.acks = 0;
    gQuest.netMsgArgs = 0;
    gQuest.eventTriggered = 0;
    gQuest.eventLabel = 0;
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
    header = (StageHeader *) gSysSettings.stageFile;
    gQuest.info = (QuestInfo *) (header->info + gSysSettings.stageFile);
    gQuest.inputLock = 60; /* frames: 2 s */
    gQuest.header = header;
    gQuest.nextSide = 0;
    Quest_SetStartArea(START_AREA_SCENE);
    gQuest.timeLeft = Quest_GetTime(QUEST_TIME_LIMIT);
    gQuest.timeNoticeIdx = (gQuest.timeLeft - 1) / QUEST_TIME_NOTICE_STEP;
    if (gQuest.timeNoticeIdx > 2) {
        gQuest.timeNoticeIdx = 2;
    }
    i = gSysSettings.stageFile;
    gQuest.script = (void *) (header->script + i);
    gQuest.loopPc = 0;
    gQuest.scriptPc = 0;
    gQuest.areaData = (void *) (header->areaData + i);
    gQuest.noticeTexts = (void *) (header->noticeTexts + i);
    gQuest.texts = (void *) (gQuest.info->texts + i);
    gQuest.emSpawns = (void *) (header->emSpawns + i);
    gQuest.stageEntry = (void *) (header->stageEntry + i);
    gQuest.areaRecords = (void *) (header->areaRecords + i);
    gQuest.gatherPoints = (void *) (header->gatherPoints + i);
    gQuest.actionSpots = (void *) (header->actionSpots + i);
    gQuest.dataTable = (void *) (header->dataTable + i);
    gQuest.seasonalData = (void *) (header->seasonalData + i);
    gQuest.reward = gQuest.info->reward;
    gQuest.penalty = gQuest.info->penalty;
    gQuest.rewardTable = (void *) (header->rewardTable + i);
    gQuest.flags = gQuest.info->flags;
    gQuest.emLevel = header->emLevel;
    gQuest.header->tables += gSysSettings.stageFile;
    mapInfo = (u32 *) (header->stageInfo + gSysSettings.stageFile);
    gScene.map = mapInfo[0];
    gScene.campArea = mapInfo[1];
    gQuest.timedEvent = header->timedEvent;
    gQuest.eventKind = header->eventKind;
    gQuest.eventCount = header->eventCount;
    gQuest.timedEventSet = gQuest.timeLeft / QUEST_TIME_NOTICE_STEP;
    gQuest.timedEventStep = 0;
    gQuest.traderVariant = 0;
    Galex_LoadTargets();
    nextIndex = 0;
    Galex_InitEmSets((u8 *) gQuest.stageEntry, &nextIndex);
    Galex_InitEmAreaSets((u8 **) gQuest.emSpawns, &nextIndex);
    GmLoad_Reset();
}

/* Zeros up to 0xD7BD80, the end of .rodata: the game's linker ended each
 * section on a 0x80-byte boundary (.text at 0xD75F80, .data at 0xD79580). */
const u8 D_00D7BD40[0x40] = {0};
