/*
 * The scene and the quest's start (sub_main): Scene_QuestTask, the task that
 * runs the scene gScene.scene (SCENE_* in scene.h) every frame, and the
 * scenes' steps: entering the field and starting a quest (Scene_EnterField,
 * Scene_QuestStartTask and its QSTART_* steps: the players, the models, the
 * sounds, the monsters), the field (Scene_UpdateQuest, with the area
 * change), the quest's end (Scene_ResultFade), the results and the opening
 * movie (Scene_Opening), and the offline pause and the play time.
 *
 * With them: the loading of an area's model (Area_Load; the collision code
 * loads its grids) and of the common, player and monster animations
 * (Anim_Load*).
 *
 * The quest's own state is gQuest (quest.h); the scene state is gScene
 * (scene.h).
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
#include "sub_main/scene.h"   /* the texts this file uses (tools/content_gen.py) */
#include "render_state.h"
#include "area_draw.h"
#include "bgm.h"
#include "obj.h"
#include "place_rules.h"
#include "camera.h"
#include "capsule.h"
#include "hud.h"
#include "light.h"
#include "movie.h"
#include "plsel.h"
#include "prim.h"
#include "qnet.h"
#include "sound.h"
#include "stream.h"
#include "texture.h"
#include "village_cond.h"
#include "draw_helpers.h"
#include "draw_slots.h"
#include "file_load.h"
#include "font.h"
#include "frame.h"
#include "game_util.h"
#include "gm_load.h"
#include "main.h"
#include "mem_util.h"
#include "menu_util.h"
#include "packet_mode.h"
#include "rand.h"
#include "sprite_queue.h"
#include "vibration.h"
#include "screen_fade.h"
#include "libkernl.h"
#include "tex_files.h"

/* prototypes */
void * memset(void *p, s32 c, u32 n);
extern u8 gLightSets;
extern u16 gNextQuestId;
extern s8 gGalleryTitle[];
extern u8 gPadReplay;
extern u8 *gLoadBuf;

extern s32 gAreaTexFiles[];
extern s32 gOnlineVillageTexFiles[2];
extern s32 gArea84TexFiles[2];
extern s32 gCommonModelTexFiles[];
Model *Model_SetupWithPack(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c);
extern u8 gSoundLoading;
extern s32 D_005B1DB8;
extern s32 D_005B1DC0;
extern u16 gFrameTick;
char *strcpy(char *dst, char *src);
/* A sprite description, as Sprite_Queue takes it, with the colour as one
 * word. */
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
extern u8 gPlayerSys[];
s32 sprintf(char *dst, char *fmt, ...);
/* unit's own declaration: with the player's index, which Quest_InitPlayers passes too; with the definition's (Player *) alone the call compiles differently */
void Plsel_ApplyStageEquip(Player *p, s32 i);
void Text_SetColor(s16 color);
s32 Str_Length(char *s);
void Text_Printf(char *fmt, ...);
void Text_DrawAllPages(void);
extern u8 gCameraPreset;
extern s16 gAreaSeBanks[];
struct SndFile;
s32 Sound_LoadBank(struct SndFile *f, u32 slot);
s32 Sound_LoadStaged(struct SndFile *f, u8 *out, u32 slot);
extern u8 gScreenCaptureOk;
extern u32 *gReadBuf;
void FreeList_InitAll(void);
/* end of prototypes */

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
    Scene_EmptyFrameHook();
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
 * preset hunters, and starts the quest preparation (plsel_entry, with
 * OVERLAY_GAME and OVERLAY_PLSEL) to run alongside.
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
        gScene.userIds[i][0] = 0;
        gScene.userIds[i][1] = 0;
        gScene.userIds[i][2] = 0;
        gScene.userIds[i][3] = 0;
        gScene.userIds[i][4] = 0;
        gScene.userIds[i][5] = 0;
        gScene.userIds[i][6] = 0;
        gScene.userIds[i][7] = 0;
    }
    Player_SetMyUserId(gScene.localPlayer);
    Loading_SetMessage(5);
    Scene_LoadOverlay(3);
    Scene_LoadOverlay(11);
    TaskSlot_Set(plsel_entry, 6);
}

/*
 * QSTART_INIT, run once the quest preparation has cleared allLocal: resets
 * the subsystems (keeping gSoundLoading), copies the options and each
 * player's choices from gQuestSync, sets up the quest and its supply box,
 * and tells the peers. A drill starts plsel's select task; a training quest
 * loads OVERLAY_PLSEL too.
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
        Scene_LoadOverlay(11);
        TaskSlot_Set(Plsel_SelectTask, 6);
    }
    if (Quest_IsTraining() == 1) {
        Scene_LoadOverlay(11);
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
        Scene_LoadOverlay(11);
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
    UnusedObjs_Clear();
    EmSenseObjs_Clear();
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
        bank = (gAreaSeBanks[gScene.area] + 11) | 0x10000;
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
        if (SndLoad_PlayerBanks(buf, &gPlayers[player])) {
            gScene.subStep++;
            Sound_LoadStaged((struct SndFile *) buf, buf + 0x20000, player + 2);
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
        if (SndLoad_EmBanks(buf, gScene.emKind)) {
            gScene.subStep++;
        }
        return;
    case 19:
        if (!FileQueue_IsBusy()) {
            gScene.subStep++;
            Sound_LoadStaged((struct SndFile *) buf, buf + 18432, 6);
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
    Tex_LoadQuestEndFile();
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
            pl->unk393[917 - 915] = 0;
            *(s16 *)&pl->unk6B1[1714 - 1713] = 0;
            pl->hitStop = 0;
            pl->hitStopAdd = 0;
            pl->teamItemMask = 0;
            pl->actRequest = 0;
            pl->offerRequest = 0;
            if (gScene.questPhase == QUEST_PHASE_QUIT) {
                gScene.step = 0;
                gScene.scene = SCENE_RESULTS;
                Scene_LoadOverlay(11);
            }
            break;
        }
        if (pl->areaMoveState != PL_AREA_MOVE_NONE) {
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
            Map_DrawAreaChange();
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
        Quest_SpawnMonsters(255);
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
        if (SndLoad_EmBanks(buf, gScene.emKind)) {
            Sound_LoadStaged((struct SndFile *) buf, buf + 18432, 6);
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
            bank = (gAreaSeBanks[gScene.area] + 11) | 0x10000;
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
            Tex_LoadQuestEndFile();
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
 * SCENE_QUEST_END: waits 120 frames, darkens the screen to alpha 192 by 8 a
 * frame, shows the quest's end (the result texture and an area object of
 * kind 23 with the quest phase) for 150 frames, then marks the local
 * player ready, goes to SCENE_RESULTS and loads OVERLAY_PLSEL. The screen is
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
        if (sceneState->timer >= 192) {
            sceneState->step++;
            sceneState->timer = 192;
            sceneState->timer2 = 0;
            Tex_SetQuestEndImages();
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
        Scene_LoadOverlay(11);
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

/* SCENE_RESULTS: runs OVERLAY_PLSEL's results frame (Plsel_ResultsUpdate), sending
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
    Sound_ReloadCommonBank();
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
 * screen for 150 frames (PAD_CIRCLE shortens it after the first 60), then
 * SCENE_EXIT. PAD_START stops a movie that is playing; each movie ends
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
            Save_SetEventFlag(389);
            Movie_Play(7);
            EventFlag_Set(37);
        } else {
            Movie_Play(8);
            if (EventFlag_Test(38) == 0) {
                sceneState->openingFirst = 1;
            } else {
                sceneState->openingFirst = 0;
            }
            EventFlag_Set(38);
            EventFlag_Set(49);
            EventFlag_Set(50);
            EventFlag_Set(51);
            EventFlag_Set(52);
        }
        sceneState->step++;
        sceneState->timer = 0;
        break;
    case 1:
        Movie_Update();
        status = Movie_GetStatus();
        if (status != 3) {
            sceneState->timer++;
            if (status != 2 || !(gPadState[0].pressed & PAD_START)) {
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
        if (FadeA_GetState() == FADE_STATE_RUNNING) {
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
            if (sceneState->timer < 90 && (gPadState[0].pressed & PAD_CIRCLE)) {
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
    gScene.inLobby = 0;
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
    Tex_LoadFile(6, 340, 0);
    Area_Load(gScene.startArea);
    Anim_LoadCommon(0);
    Quest_InitPlayers();
    Players_UpdateSides();
    for (i = 0; i < gScene.playerCount; i++) {
        if (gScene.inQuest[i] == IN_QUEST_YES) {
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
    Quest_EmptySetupHook();
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

    pack = gScene.inLobby == 0 ? 0 : 5;
    Load_CommonModelPack(pack);
    slot = Model_Alloc(1);
    if (slot < 0) {
        return;
    }
    gStageModelSets[0] = Model_Get(slot);
    Model_Reserve(slot, 1);
    Model_SetupWithPack(slot, (*(s32 *)&gSysSettings.modelPack), 243, gCommonModelTexFiles[pack], 2816, 0);
    Load_CommonModelPack(1);
    nextSlot = Model_Alloc(1);
    if (nextSlot < 0) {
        return;
    }
    gStageModelSets[1] = Model_Get(nextSlot);
    Model_Reserve(nextSlot, 1);
    Model_SetupWithPack(nextSlot, (*(s32 *)&gSysSettings.modelPack), 262, gCommonModelTexFiles[1], 2304, 1);
    Load_CommonModelPack(2);
    nextSlot = Model_Alloc(1);
    if (nextSlot < 0) {
        return;
    }
    gStageModelSets[2] = Model_Get(nextSlot);
    Model_Reserve(nextSlot, 1);
    Model_SetupWithPack(nextSlot, (*(s32 *)&gSysSettings.modelPack), 261, gCommonModelTexFiles[2], 2304, 1);
    Load_CommonModelPack(4);
    nextSlot = Model_Alloc(1);
    if (nextSlot < 0) {
        return;
    }
    gStageModelSets[4] = Model_Get(nextSlot);
    Model_Reserve(nextSlot, 1);
    Model_SetupWithPack(nextSlot, (*(s32 *)&gSysSettings.modelPack), 259, gCommonModelTexFiles[4], 2048, 1);
}

/* An empty step of Quest_Setup. */
void Quest_SetupStub(void)
{
}

/*
 * Resets the input, the lights, the fades, the pause and the camera for the
 * area entered; mode 255 (an area change) stops there. Otherwise it places
 * the quest's monsters: a quest (questId set) through Quest_EnterArea;
 * without one, each monster slot's kind and count from gQuestSync. With
 * debug switch 23 on, test monsters of fixed kinds instead (debug switch
 * 116 gives a second one in area 0).
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
    gCameraPreset = 255;
    gScene.heldMask = 0;
    Camera_SetPreset(0);
    CamCtl_Init();
    if (mode == 255) {
        return;
    }
    if (gScene.area == 13) {
        Debug_SwitchStubAlt(100);
        Debug_SwitchStubAlt(101);
        Debug_SwitchStubAlt(102);
    } else {
        Debug_SwitchStub(100);
        Debug_SwitchStub(101);
        Debug_SwitchStub(102);
        Debug_SwitchStub(103);
        Debug_SwitchStub(104);
    }
    if (gScene.questId != 0) {
        Quest_EnterArea(gScene.area);
        return;
    }
    if (Debug_IsOn(23) == 0) {
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
    gScene.emKind[0] = 11;
    gScene.emKind[1] = 10;
    gScene.emKind[2] = 3;
    gScene.emKind[3] = 4;
    for (j = 0; j < 4; j++) {
        EmSlot_LoadModels(j);
    }
    for (j = 0; j < 2; j++) {
        if (Debug_IsOn(116) != 0) {
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
        if ((em->type == 1 || em->type == 42 || em->type == 37) && gScene.area == AREA_FORTRESS_4) {
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

    memset(gPlayerSys, 0, 16);
    for (i = 0; i < 4; i++) {
        pl = &gPlayers[i];
        memset(pl, 0, sizeof(Player));
        if (i >= gScene.playerCount) {
            continue;
        }
        if (gScene.inQuest[i] == IN_QUEST_YES) {
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
            pl->equip[EQUIP_SLOT_WEAPON] = gScene.weapon[i];
            pl->weaponKind = Equip_GetKind(&gScene.weapon[i]);
            sprintf((char *)pl->name, TXT(0x0046CE10) /* a default player name, with the number */, i + 1);
        } else if (gScene.presetEquip[i] == 0 || gSysSettings.useSavedEquip != 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_LoadFromSave(pl);
                Player_LoadName(pl);
            } else {
                Player_LoadPreset(gScene.userIds[i], pl);
            }
        } else {
            pl->equip[EQUIP_SLOT_WEAPON] = gScene.weapon[i];
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
        pl->bladeSpecial = Equip_GetBladeSpecial(&pl->equip[EQUIP_SLOT_WEAPON]);
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
    memset(&gLightSets, 0, 656U);
    Light_InitAll((s32) gScene.area);
}

/* Clears the pad replay and both ports' input, this frame's and the last. */
void Pad_ResetInput(void)
{
    memset(&gPadReplay, 0, 32U);
    gPadInput.buttons[0].cur = 0;
    gPadInput.buttons[0].prev = 0;
    gPadInput.stickDir[0].cur = 0;
    gPadInput.stickDir[0].prev = 0;
    gPadInput.stickAngle[0].left = 0;
    gPadInput.stickAngle[0].right = 0;
    gPadInput.stickTilt[0].left = 0;
    gPadInput.stickTilt[0].right = 0;
    gPadInput.buttons[1].cur = 0;
    gPadInput.buttons[1].prev = 0;
    gPadInput.stickDir[1].cur = 0;
    gPadInput.stickDir[1].prev = 0;
    gPadInput.stickAngle[1].left = 0;
    gPadInput.stickAngle[1].right = 0;
    gPadInput.stickTilt[1].left = 0;
    gPadInput.stickTilt[1].right = 0;
}

/*
 * Loads an area: its pack, its grids (Area_LoadTables), its model with its
 * texture file, its sub-models and its camera data. AREA_ONLINE_VILLAGE and AREA_HUB_0x84 take
 * a second texture file online while ElderRaid_IsOn.
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
        case AREA_ONLINE_VILLAGE:
            if (gScene.inLobby != 0 && ElderRaid_IsOn() == 1) {
                tex = gOnlineVillageTexFiles[1];
            } else {
                tex = gOnlineVillageTexFiles[0];
            }
            break;
        case AREA_HUB_0x84:
            if (gScene.inLobby != 0 && ElderRaid_IsOn() == 1) {
                tex = gArea84TexFiles[1];
            } else {
                tex = gArea84TexFiles[0];
            }
            break;
        default:
            tex = gAreaTexFiles[gScene.area];
            break;
        }
        Model_SetupTexPack(slot, (*(s32 *)&gSysSettings.modelPack), 264, tex, 256);
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

