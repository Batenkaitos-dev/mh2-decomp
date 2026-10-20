/*
 * The lobby screen: the frame of the town and the village, and how it is
 * entered and left.
 *
 * gLobbyScreenMode (enum LobbyScreenMode) picks the frame's step from three
 * tables of the same functions: gLobbySceneSteps (LobbyScene_Run),
 * gLobbyScreenModes (LobbyScreen_Run) and gLobbyScreenSteps
 * (LobbyScreen_RunStep). The steps are the screen's start, entering an
 * area, the town's own frame (Town_Frame), leaving an area, and a camera
 * tour.
 *
 * LobbyScreen_EnterSeq enters the screen in one of the ways of enum
 * LobbyEnterMode: it resets the screen's state (LobbyScreen_Enter, which
 * resets the town's day with LobbyScreen_InitLists), loads the models
 * (LobbyScreen_LoadModels), picks the area to start in, loads it
 * (LobbyScreen_LoadArea) and the sound banks (LobbyScreen_LoadBanks), and
 * starts the area's one-time scene or tour (LobbyArea_StartEvent).
 * LobbyScreen_FadeOut leaves it. The lobby's network phases call these
 * from their own steps; offline, LobbyScreen_Start runs the entry itself.
 */
#include "common.h"
#include "card.h"
#include "chat_log.h"
#include "game_menu.h"
#include "syssettings.h"
#include "em.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "syssave.h"
#include "area.h"
#include "quest.h"
#include "camera.h"
#include "bgm.h"
#include "sound.h"
#include "lobby_menu.h"
#include "shadow.h"
#include "lobby_text.h"
void *memset(void *, s32, u32);
void Text_SetSize(s32);

void Lobby_CountPlayTime(void);
extern void (*gLobbySceneSteps[])(void);
void Loading_Start(void);
void Money_PayPending(u8 silent);
void Lobby_ReceiveWhileWaiting(void);
void TownPc_LoadAllModels(void);
void Hud_LoadTextures(void);
void Quest_LoadCommonModels(void);
void Quest_SetupStub(void);
void Shadow_Spawn(void *obj, s16 type);
void Loading_Stop(void);
void Loading_SetMessage(u32 mode);
/* unit's own declaration: the second parameter is a u8 here, as LobbyScreen_LoadArea passes it; with the definition's s32, that call compiles differently */
u8 Town_EnterArea(u16 area, u8 newDrawSlots);
void Town_SpawnNpcs(u16 a);

void Actor_SetPosture(void *a, u16 mode, s32 b, s32 c);
void Actor_SetState(void *obj, s32 group, s32 state);
s32 Area_IsOnlineHouse(s32);
void Town_FreeArea(void);
void LobbyText_Open(s32);
s32 HunterCard_SendChanged(void);
void LobbyBgm_Stop(void);
void LobbyBgm_StopAll(void);
void Stream_Pause(s32, s32);
void Stream_FadeOut(s32, s32);
u8 FadeA_GetState(void);
void FadeA_Start(s32);
s32 Bgm_IsNpcTunePlaying(void);
void Bgm_StopEvent(void *obj);
void Bgm_StopEventFade(Player *p);
s32 Game_IsOnline(void);
void Notice_ResetQueue(void);
extern s32 gTownMenuOpen;
extern s8 gLobbyScreenMode;
extern s8 gLobbyScreenStep;
extern s8 gLobbyScreenSubStep;
extern u8 gLobbyTextDirty;
extern LobbyMenu *gLobbyMenu;
s32 LobbyScreen_EnterSeq(s32);
extern s32 gTownMode;
extern u8 gMenuInputLock;

extern NetUser gNetUsers[10];         /* the members (their ids, names, texts) */
extern char gMyUserId[];                 /* the player's user id */



/* Runs one frame of the lobby's scene: counts the play time, then runs
 * the scene's step for gLobbyScreenMode (gLobbySceneSteps). */
void LobbyScene_Run(void)
{
    Lobby_CountPlayTime();
    gLobbySceneSteps[gLobbyScreenMode]();
}

void LobbyTime_Tick(void);
u16 LobbyPad_Read(s8 port);
void LobbyMenu_QueueLayers(void);
void Hud_StartQuest(void);
void LobbyMenu_SetDraw(void *draw0, void *draw1, void *draw2, void *draw3);
extern void (*gLobbyScreenModes[])(LobbyState *p);   /* the lobby screen's modes (gLobbyScreenMode) */

#define LOBBY_TEXT_SIZE 6   /* the lobby's text size (Text_SetSize), set each frame */

/*
 * Runs one frame of the lobby screen: counts the play time, moves the
 * town's clock, reads the pad into LobbyState.buttons, runs the mode
 * gLobbyScreenMode (gLobbyScreenModes) and queues the menus' layers
 * unless the mode hid them. Returns 0 while the lobby goes on. Once one
 * of the town's two leaving sequences has ended (TOWN_MODE_DEPARTED or
 * TOWN_MODE_QUIT_DONE, after its fade) it starts the quest's HUD and
 * returns 1 for the departure, which also leaves the lobby
 * (gScene.inLobby 0), or -1 for the quit.
 */
s32 LobbyScreen_Run(void)
{
    LobbyState *lobby;

    Lobby_CountPlayTime();
    LobbyTime_Tick();
    gLobbyMenu->redraw = 0;
    gLobbyMenu->hideLayers = 0;
    Text_SetSize(LOBBY_TEXT_SIZE);
    if (gMenuInputLock > 0) {
        gMenuInputLock--;
    }
    (lobby = gLobby)->buttons = LobbyPad_Read(0);
    gLobbyScreenModes[gLobbyScreenMode](lobby);
    if (gLobbyMenu->hideLayers == 0) {
        LobbyMenu_QueueLayers();
    }
    if (gTownMode == TOWN_MODE_DEPARTED) {
        gScene.inLobby = 0;
        Hud_StartQuest();
        LobbyMenu_SetDraw(0, 0, 0, 0);
        return 1;
    } else if (gTownMode == TOWN_MODE_QUIT_DONE) {
        Hud_StartQuest();
        LobbyMenu_SetDraw(0, 0, 0, 0);
        return -1;
    }
    return 0;
}

void Village_UpdateProgress(s32 a);
void Poogie_DailyReset(void);
extern s8 gContestPrizeGiven;
extern s16 gPetDebugBars[4];
void Save_ClearNewMarks(void);
void Rate_RaiseRandom(void);
s32 Rate_UpdateDue(void);
void Rate_UpdatePrices(void);
void Rate_TakeSnapshot(void);
void Rotation_Update(s32 kind);
s8 Contest_RollLevel(void);
extern s8 gContestLevel;
s32 LobbyWorld_GetSeason(void);
s32 LobbyTime_IsPm(void);
void ElderForecast_Roll(void);
void QuestList_Rebuild(void);
void *memcpy(void *dst, const void *src, u32 n);

/*
 * Resets the town's day on entering the lobby screen: the meal's
 * effects, the contest's prize, the village's progress and gathering,
 * and the Poogie's moods. The online bonus (gPoogieErrand) is kept
 * only offline, and only from 1 to 2 when the last quest wasn't
 * abandoned. Offline, the rates, the rotations and the contest's level
 * move on (Rate_UpdateDue, a random raise unless the last quest was
 * abandoned). The quest lists are rebuilt, with a new elder forecast
 * offline, unless the last quest was abandoned in the same season and
 * time of day: then the town's lists are copied back from the kept ones.
 */
void LobbyScreen_InitLists(void)
{
    s32 due;
    TownQuestLists *lists;
    TownQuestLists *kept;

    gScene.mealAttack = 0;
    gScene.mealDefense = 0;
    gScene.mealHealth = 0;
    gScene.mealStamina = 0;
    gLobby->mealEaten = 0;
    gContestPrizeGiven = 0;
    Village_UpdateProgress(0);
    Poogie_DailyReset();
    gPetDebugBars[0] = 0;
    gPetDebugBars[1] = 0;
    gPetDebugBars[2] = 0;
    gPetDebugBars[3] = 0;
    if (gSysSettings.offline == 0) {
        if (Game_IsOnline() == 0) {
            gPoogieErrand = 0;
        }
    } else {
        Save_ClearNewMarks();
        if (gPoogieErrand == 1) {
            if (gQuestRecordPhase != QUEST_PHASE_ABORTED) {
                gPoogieErrand = 2;
            } else {
                gPoogieErrand = 0;
            }
        } else {
            gPoogieErrand = 0;
        }
    }
    if (Game_IsOnline() == 0) {
        if (gSysSettings.offline != 0 && gQuestRecordPhase != QUEST_PHASE_ABORTED) {
            Rate_RaiseRandom();
        }
        due = Rate_UpdateDue();
        if (due == 1) {
            Rate_UpdatePrices();
        }
        Rate_TakeSnapshot();
        if (due == 1) {
            Rotation_Update(0);
            Rotation_Update(1);
            Rotation_Update(2);
        }
        gContestLevel = Contest_RollLevel();
    }
    if (gQuestRecordPhase != QUEST_PHASE_ABORTED || gLobbyLastSeason != LobbyWorld_GetSeason() || gLobbyLastPm != LobbyTime_IsPm()) {
        if (Game_IsOnline() == 0) {
            ElderForecast_Roll();
        }
        QuestList_Rebuild();
    } else {
        lists = &gTownQuestLists;
        kept = &gTownQuestListsCopy;
        if (Game_IsOnline() == 0) {
            memcpy(lists->quests, kept->quests, sizeof(lists->quests));
        }
    }
}

void FileQueue_Cancel(void);
void Game_ResetSubsystems(void);
void CamReq_Reset(void);
void Bgm_Init(void);
u16 Rand_Next(s32 stream);
extern u16 gLobbyEnterRoll;
extern u16 gLobbyEnterRand;
void Rate_ClearSnapshot(void);
void Season_Set(s32 a);
extern u32 gTownDay;
extern u32 gTownSeason;
extern u32 gTownSeasonDays[3];
extern u32 gTownDayTicks;
extern s32 gLobbyTime;
void LobbyTime_SetClock(u32 time);
void LobbyWorld_CatchUp(void);
extern u8 gMyName[];                   /* the player's name */
void Player_LoadFromSave(Player *p);
void HunterCard_Build(void *out);

extern u8 *gStageFile;
/*
 * Resets the lobby screen's state on entering it (`mode`: enum
 * LobbyEnterMode). The players' models and the area are to be loaded
 * again; the game's subsystems, the camera requests and the music are
 * reset, and two random numbers drawn. Offline (outside the gallery's
 * scenes) the town's clock comes from the save (its season, day and
 * time), the day's lists are reset (LobbyScreen_InitLists) and the wanted
 * entries are copied from the save. Unless the town's members are kept
 * (LOBBY_ENTER_TOWN_MEMBERS), the members and their models' state are
 * cleared. Then the player's name and user id (offline, the save's name
 * and no id; online, the stage file's buffer is cleared), the player
 * loaded from the save, and its hunter card built.
 */
void LobbyScreen_Enter(s32 mode)
{
    s32 i;
    TownQuestLists *lists;
    s32 season;

    FileQueue_Cancel();
    for (i = 0; i < LOBBY_MEMBER_COUNT; i++) {
        gLobby->modelLoaded[i] = 0;
    }
    gLobby->areaLoaded = 0;
    if (Game_IsOnline() == 1) {
        gLobby->unk3968 = 0;
        gLobby->unk396D = 0;
    }
    Game_ResetSubsystems();
    CamReq_Reset();
    Bgm_Init();
    gLobbyEnterRoll = Rand_Next(1);
    gLobbyEnterRand = Rand_Next(1);
    if (Game_IsOnline() == 0 && gSysSettings.galleryPlay == 0) {
        Rate_ClearSnapshot();
        Season_Set(gSave.season);
        gTownSeason = gSave.season;
        gTownSeasonDays[0] = 3;     /* days a season */
        gTownDayTicks = 2400;       /* seconds a day */
        gTownSeasonDays[1] = 3;
        gTownSeasonDays[2] = 3;
        gTownDay = gSave.day;
        gLobbyTime = gSave.time;
        LobbyTime_SetClock(gLobbyTime);
        if (gSysSettings.offline != 0) {
            LobbyWorld_CatchUp();
        }
        LobbyScreen_InitLists();
        lists = (TownQuestLists *)&gTownQuestLists;
        memset(lists->a, 0, sizeof(lists->a));
        for (season = 0; season < WANTED_SEASONS; season++) {
            memcpy(&lists->a[season][0], gSave.days[season][0], sizeof(TownEntry));
            memcpy(&lists->a[season][1], gSave.days[season][1], sizeof(TownEntry));
            memcpy(&lists->a[season][2], gSave.days[season][2], sizeof(TownEntry));
        }
    } else if (gTownDayTicks == 0) {
        gTownDayTicks = 2400;
        gLobbyTime = gSave.time;
        LobbyTime_SetClock(gLobbyTime);
    }
    if (mode != LOBBY_ENTER_TOWN_MEMBERS) {
        memset(gLobby->members, 0, sizeof(gLobby->members));
        memset(gNetUsers, 0, sizeof(gNetUsers));
        memset(gLobby->pcLoaded, 0, sizeof(gLobby->pcLoaded));
        memset(gLobby->lookPending, 0, LOBBY_MEMBER_COUNT);
    }
    if (Game_IsOnline() == 0) {
        gScene.playerCount = 1;
        memcpy(gMyName, gSave.name, LOBBY_USER_NAME_LEN + 1);
        memcpy(gMyUserId, "", LOBBY_USER_ID_LEN);
    } else {
        memset(gStageFile, 0, 0x8000);    /* the stage file's buffer, 32 KB */
    }
    Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
    memcpy(gNetUsers[gScene.localPlayer].name, gMyName, LOBBY_USER_NAME_LEN);
    memcpy(gNetUsers[gScene.localPlayer].id, gMyUserId, LOBBY_USER_ID_LEN);
    HunterCard_Build(gNetUsers[gScene.localPlayer].text);
}

/*
 * Loads what the lobby screen draws, under the loading screen: pays what
 * is pending (without its message), then loads the town's players' models,
 * the HUD's textures and the common models, receiving the network's
 * messages between the steps (Lobby_ReceiveWhileWaiting). Each player then
 * gets its shadow.
 */
void LobbyScreen_LoadModels(void)
{
    s32 i;
    Player *pl;

    gSoundLoading = 1;
    Loading_Start();
    Money_PayPending(1);
    Lobby_ReceiveWhileWaiting();
    TownPc_LoadAllModels();
    Lobby_ReceiveWhileWaiting();
    Hud_LoadTextures();
    Lobby_ReceiveWhileWaiting();
    Quest_LoadCommonModels();
    Lobby_ReceiveWhileWaiting();
    Quest_SetupStub();
    Lobby_ReceiveWhileWaiting();
    Lobby_ReceiveWhileWaiting();
    for (i = 0, pl = gPlayers; i < LOBBY_MEMBER_COUNT; i++, pl++) {
        Shadow_Spawn(pl, SHADOW_PLAYER);
    }
    if (Game_IsOnline() == 1) {
        gLobby->unk3968 = 1;
    }
    Loading_Stop();
    gSoundLoading = 0;
}

extern u8 gTownActionDelay;
extern u8 gMemberNamesOff;
extern u8 gTownCamEndOnClose;
void Quest_Clear(void);
void *Hud_Init(void);
void LobbyBgm_ClearState(void);

/*
 * Resets the town's state for the player entering it: the player's action
 * and state, the slots' user ids, the ready bits, the quest taken and
 * paid, the models being loaded, the town's mode, the menus' draws, the
 * quest, the HUD and the lobby's music state. The town's actions wait 20
 * frames (gTownActionDelay).
 */
void LobbyScreen_ResetTown(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    s32 i;

    Actor_SetPosture(pl, 0, 0, 0);
    Actor_SetState(pl, 0, 1);
    gTownMenuOpen = 1;
    memset(gLobby->houseSeatIds, 0, sizeof(gLobby->houseSeatIds));
    gLobby->houseSeatBits = 0;
    gLobby->questTaken = 0;
    gLobby->questPaid = 0;
    for (i = 0; i < LOBBY_MEMBER_COUNT; i++) {
        gLobby->modelLoading[i] = 0;
    }
    gLobby->returnToArea = 0;
    gScene.menuOpen = 0;
    pl->drunk = 0;
    gTownActionDelay = 20;
    gTownCamEndOnClose = 1;
    gTownMode = TOWN_MODE_FREE;
    gMemberNamesOff = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    Quest_Clear();
    Hud_Init();
    LobbyBgm_ClearState();
}

/*
 * Loads the player's next area (Player.nextArea) under the loading
 * screen, with its town characters unless it is the other half of the
 * place the player is in (Town_EnterArea). The entries that loaded the
 * screen's models (LOBBY_ENTER_VILLAGE_OFFLINE, _VILLAGE, _TOWN_MEMBERS)
 * keep the players' draw slots; the others give them new ones.
 */
void LobbyScreen_LoadArea(s32 mode)
{
    u8 newDrawSlots;

    switch (mode) {
    case LOBBY_ENTER_TOWN_MEMBERS:
    case LOBBY_ENTER_VILLAGE_OFFLINE:
    case LOBBY_ENTER_VILLAGE:
        newDrawSlots = 0;
        break;
    default:
        newDrawSlots = 1;
        break;
    }
    gSoundLoading = 1;
    Lobby_ReceiveWhileWaiting();
    Loading_SetMessage(0);
    if (Town_EnterArea(gPlayers[gScene.localPlayer].nextArea, newDrawSlots) == 0) {
        Lobby_ReceiveWhileWaiting();
        Town_SpawnNpcs(gPlayers[gScene.localPlayer].nextArea);
    }
    Loading_Stop();
    gSoundLoading = 0;
    gLobby->unk3968 = 1;
}

extern LobbyStartPos gLobbyStartPos;
extern u8 gTownSpawnSet;
/*
 * The players' positions and moveTo points as rows of gPlayers' stride:
 * gPlayerPosRows[i].v is gPlayers[i].pos and gPlayerMoveToRows[i].v is
 * gPlayers[i].moveTo. The game's code reaches them through these names of
 * their own, so the field's offset is in the address and not in the load
 * or store.
 */
typedef struct PlayerVecRow {
    Vec3f v;
    u8 rest[sizeof(Player) - sizeof(Vec3f)];
} PlayerVecRow;
extern PlayerVecRow gPlayerPosRows[];
extern PlayerVecRow gPlayerMoveToRows[];
extern s32 gLobbyStartPosIdx;
void HunterCard_Apply(s8 idx, void *src);
void TownPc_Spawn(u8 i, NetUser *u, char *name);
void Town_PlaceLocalPlayer(s32 area);
void CamCtl_Init(void);
void Town_SetDrawSky(s8 a);
void Town_StartAreaMusic(s32 a);
void TownPc_SyncMembers(void);
void TownMsg_SendNameNotice(void);
void TownMsg_SendSyncAsk(void);
void TownMsg_SendArea(u16 area);
void TownMsg_SendPlace(Player *obj);
void CamCtl_Update(s32 a);
s32 LobbyScreen_LoadBanks(void);
void Loading_Request(void);
s32 LobbyArea_StartEvent(void);

/*
 * Enters the lobby screen in `mode` (enum LobbyEnterMode), a step a call
 * (LobbyState.seqStep):
 * - 0: the screen goes black; its state is reset (LobbyScreen_Enter) and
 *   the players spawned: the members with theirs (LOBBY_ENTER_MEMBERS,
 *   _TOWN_MEMBERS), the player alone (the village's two entries). The
 *   other modes skip to step 2.
 * - 1: the models loaded (LobbyScreen_LoadModels).
 * - 2: the town's state reset (LobbyScreen_ResetTown) and the area to
 *   start in set (Player.nextArea, and for LOBBY_ENTER_VILLAGE_RETURN a set
 *   spot); the area is loaded, or for LOBBY_ENTER_VILLAGE_STAY the player
 *   placed again. The modes that loaded the screen's models go on to step
 *   3, the others to step 4.
 * - 3: the sound banks (LobbyScreen_LoadBanks); then the camera, the
 *   area's music and the fade in. LOBBY_ENTER_TOWN_MEMBERS tells the
 *   members where the player is. Offline, the area's one-time scene or
 *   tour may start (LobbyArea_StartEvent): the player is then put at the
 *   tour's start position.
 * - 4: the same ending without the banks; LOBBY_ENTER_TOWN tells the
 *   members.
 * Returns LOBBY_STEP_BUSY while running, LOBBY_STEP_DONE when done, and
 * LOBBY_ENTER_TOURING when done with a scene or tour started.
 */
s32 LobbyScreen_EnterSeq(s32 mode)
{
    s16 *step;
    s32 i;
    NetUser *user;

    gLobby->connected = 0;
    step = &gLobby->seqStep;
    switch (*step) {
    case 0:
        (*step)++;
        FadeA_Start(FADE_BLACK);
        switch (mode) {
        case LOBBY_ENTER_MEMBERS:
        case LOBBY_ENTER_TOWN_MEMBERS:
            LobbyScreen_Enter(mode);
            for (i = 0, user = gNetUsers; i < LOBBY_MEMBER_COUNT; i++, user++) {
                if (user->id[0] != 0) {
                    HunterCard_Apply(i, user->text);
                    TownPc_Spawn(i, user, user->name);
                }
            }
            break;
        case LOBBY_ENTER_VILLAGE_OFFLINE:
            LobbyScreen_Enter(mode);
            HunterCard_Apply(gScene.localPlayer, gNetUsers[gScene.localPlayer].text);
            TownPc_Spawn(gScene.localPlayer, &gNetUsers[gScene.localPlayer], gNetUsers[gScene.localPlayer].name);
            memcpy(&gLobby->members[gScene.localPlayer], &gNetUsers[gScene.localPlayer], sizeof(NetUser));
            gScene.playerCount = 1;
            break;
        case LOBBY_ENTER_VILLAGE:
            LobbyScreen_Enter(mode);
            HunterCard_Apply(gScene.localPlayer, gNetUsers[gScene.localPlayer].text);
            TownPc_Spawn(gScene.localPlayer, &gNetUsers[gScene.localPlayer], gNetUsers[gScene.localPlayer].name);
            memcpy(&gLobby->members[gScene.localPlayer], &gNetUsers[gScene.localPlayer], sizeof(NetUser));
            break;
        default:
            gLobby->seqStep = 2;
            break;
        }
        break;
    case 1:
        LobbyScreen_LoadModels();
        gLobby->seqStep++;
    case 2:
        LobbyScreen_ResetTown();
        switch (mode) {
        case LOBBY_ENTER_MEMBERS:
            gLobby->seqStep = 3;
            return LOBBY_STEP_BUSY;
        case LOBBY_ENTER_TOWN_MEMBERS:
            gTownSpawnSet = 0;
            if (gLobby->groupJoin != 0) {
                gPlayers[gScene.localPlayer].nextArea = AREA_HUB_0x84;
            } else if (gQuestReturnArea == 0) {
                gPlayers[gScene.localPlayer].nextArea = AREA_HUB_0x84;
            } else {
                gPlayers[gScene.localPlayer].nextArea = gQuestReturnArea;
            }
            break;
        case LOBBY_ENTER_VILLAGE_OFFLINE:
            gTownSpawnSet = 0;
            if (gSave.awayFromVillage != 0) {
                gPlayers[gScene.localPlayer].nextArea = AREA_HUB_0x57;
            } else {
                gPlayers[gScene.localPlayer].nextArea = AREA_HUB_0x89;
            }
            break;
        case LOBBY_ENTER_VILLAGE_RETURN: {
            s32 slot = gScene.localPlayer;

            gPlayers[slot].startPos[0] = 5762.0f;
            gPlayers[slot].startPos[1] = -30.0f;
            gPlayers[slot].startPos[2] = 10434.0f;
            gPlayers[slot].startAngle = 0;
            gPlayers[slot].nextArea = AREA_HUB_0x83;
            gTownSpawnSet = 1;
            break;
        }
        case LOBBY_ENTER_VILLAGE:
        case LOBBY_ENTER_VILLAGE_STAY:
            gTownSpawnSet = 0;
            gPlayers[gScene.localPlayer].nextArea = AREA_HUB_0x83;
            break;
        case LOBBY_ENTER_TOWN:
            gTownSpawnSet = 0;
            gPlayers[gScene.localPlayer].nextArea = AREA_HUB_0x84;
            break;
        default:
            gTownSpawnSet = 0;
            gPlayers[gScene.localPlayer].nextArea = AREA_HUB_0x83;
            break;
        }
        switch (mode) {
        case LOBBY_ENTER_VILLAGE_OFFLINE:
        case LOBBY_ENTER_VILLAGE:
        case LOBBY_ENTER_TOWN_MEMBERS:
            gLobby->seqStep++;
            gLobbyScreenSubStep = 0;
            LobbyScreen_LoadArea(mode);
            break;
        case LOBBY_ENTER_VILLAGE_STAY:
            gLobby->seqStep = 4;
            Town_PlaceLocalPlayer(gPlayers[gScene.localPlayer].area);
            CamCtl_Init();
            break;
        case LOBBY_ENTER_VILLAGE_RETURN:
        case LOBBY_ENTER_TOWN:
        default:
            gLobby->seqStep = 4;
            LobbyScreen_LoadArea(mode);
            break;
        }
        Town_SetDrawSky(0);
        break;
    case 3:
        switch (LobbyScreen_LoadBanks()) {
        case 0:
        CamCtl_Update(0);
        gLobby->seqStep = 0;
        gLobby->connected = 1;
        Town_StartAreaMusic(mode);
        FadeA_Start(FADE_IN_BLACK_DELAYED);
        Town_SetDrawSky(1);
        LobbyMenu_SetDraw(0, 0, 0, 0);
        switch (mode) {
        case LOBBY_ENTER_TOWN_MEMBERS:
            TownPc_SyncMembers();
            TownMsg_SendNameNotice();
            TownMsg_SendSyncAsk();
            TownMsg_SendArea(gScene.area);
            TownMsg_SendPlace(&gPlayers[gScene.localPlayer]);
        }
        if (Game_IsOnline() == 0) {
            if (gSysSettings.galleryPlay == 1) {
                return LOBBY_STEP_DONE;
            }
            if (LobbyArea_StartEvent() == 1) {
                if (gLobbyStartPosIdx != LOBBY_START_POS_HERE && gLobbyStartPosIdx != LOBBY_START_POS_AREA) {
                    /* the rows reached by a byte offset, which the game's code computes once */
                    s32 ofs = gScene.localPlayer * sizeof(Player);
                    Vec3f startPos;

                    gLobbyStartPos.pos[0] = ((PlayerVecRow *)((u8 *)gPlayerPosRows + ofs))->v;
                    startPos = gLobbyStartPos.pos[gLobbyStartPosIdx];
                    ((PlayerVecRow *)((u8 *)gPlayerPosRows + ofs))->v = startPos;
                    ((PlayerVecRow *)((u8 *)gPlayerMoveToRows + ofs))->v = startPos;
                }
                gFieldControl = 0;
                gSysSettings.offline = 0;
                return LOBBY_ENTER_TOURING;
            }
        }
        gSysSettings.offline = 0;
        return LOBBY_STEP_DONE;
        }
        break;
    case 4:
        Loading_Request();
        switch (mode) {
        case LOBBY_ENTER_TOWN:
            TownPc_SyncMembers();
            TownMsg_SendNameNotice();
            TownMsg_SendSyncAsk();
            TownMsg_SendArea(gScene.area);
            TownMsg_SendPlace(&gPlayers[gScene.localPlayer]);
        }
        gTownMenuOpen = 0;
        {
            LobbyState *lobby = gLobby;

            gFieldControl = 1;
            lobby->seqStep = 0;
        }
        gLobby->connected = 1;
        CamCtl_Update(0);
        FadeA_Start(FADE_IN_BLACK_DELAYED);
        Town_StartAreaMusic(mode);
        Town_SetDrawSky(1);
        LobbyMenu_SetDraw(0, 0, 0, 0);
        gSysSettings.offline = 0;
        return LOBBY_STEP_DONE;
    }
    return LOBBY_STEP_BUSY;
}

s32 Save_TestEventFlag(s32 flag);
void Save_SetEventFlag(s32 flag);
u8 Village_FacilityLevel(s32 kind);
/* unit's own declaration: declared s32 here; with the definition's s16 return, the tests of its result in LobbyArea_StartEvent compile differently */
s32 Village_MilestoneState(s32 timer);
s32 MyHouse_IsOwner(void);
void CamReq_LobbyEvent(void);
void CamReq_TownTour(s32 n);
void Bgm_PlayTownTune(s32 a, s32 b);
void EventFlag_Set(s32 n);
void Jingle_Play(void *obj, s32 id);
extern s32 gLobbyStartPosIdx;

/* Places the player at an area's start: its position, its moveTo point
 * (so it doesn't walk off) and its facing. */
static inline void Player_SetStart(Player *pl, f32 x, f32 y, f32 z, s32 angle)
{
    pl->pos[0] = x;
    pl->pos[1] = y;
    pl->pos[2] = z;
    pl->rot[1] = pl->targetYaw = angle;
    *(Vec3f *)pl->moveTo = *(Vec3f *)pl->pos;
}

/*
 * Starts the one-time scene or camera tour of the area just entered
 * (gScene.area); each plays once, by its event flag. Returns 1 when one
 * started, else 0. With LOBBY_START_POS_AREA the player is first put at
 * the area's own start (AREA_HUB_0x57, AREA_HUB_0x89).
 * - AREA_HUB_0x57: its first entry's scene (CAM_REQ_LOBBY_EVENT).
 * - AREA_HUB_0xAA, 0xAB and 0xAC: their first visit's tour, with its tune.
 * - AREA_HUB_0x89, the village: the first tour not yet played of a
 *   facility that has reached its level (the tavern, the smithy and the
 *   port, at levels 1 and 2; each one is also added to the gallery), then
 *   the village's level-2 tour, then the return tour (on arriving at the
 *   area's start, or while village timer 0 runs).
 * - The online house at house levels 1 and 2: its owner's first visit.
 * A tour also sets where the player stands once it ends (gLobbyStartPosIdx).
 */
s32 LobbyArea_StartEvent(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];

    switch (gScene.area) {
    case AREA_HUB_0x57:
        if (gLobbyStartPosIdx == LOBBY_START_POS_AREA) {
            Player_SetStart(pl, 10615.0f, 0.0f, 14774.0f, 0x8000);
        }
        if (Save_TestEventFlag(EVENT_FLAG_HUB_0x57_SCENE) == 0) {
            Save_SetEventFlag(EVENT_FLAG_HUB_0x57_SCENE);
            CamReq_LobbyEvent();
            return 1;
        }
        break;
    case AREA_HUB_0xAA:
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_HUB_0xAA) == 0) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_HUB_0xAA);
            gLobbyStartPosIdx = LOBBY_START_POS_HERE;
            CamReq_TownTour(TOWN_TOUR_HUB_0xAA);
            Jingle_Play(pl, BGM_TUNE_TOUR_HUB_0xAA);
            return 1;
        }
        break;
    case AREA_HUB_0xAB:
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_HUB_0xAB) == 0) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_HUB_0xAB);
            gLobbyStartPosIdx = LOBBY_START_POS_HERE;
            CamReq_TownTour(TOWN_TOUR_HUB_0xAB);
            Jingle_Play(pl, BGM_TUNE_TOUR_HUB_0xAB);
            return 1;
        }
        break;
    case AREA_HUB_0xAC:
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_HUB_0xAC) == 0) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_HUB_0xAC);
            gLobbyStartPosIdx = LOBBY_START_POS_HERE;
            CamReq_TownTour(TOWN_TOUR_HUB_0xAC);
            Jingle_Play(pl, BGM_TUNE_TOUR_HUB_0xAC);
            return 1;
        }
        break;
    case AREA_HUB_0x89:
        if (gLobbyStartPosIdx == LOBBY_START_POS_AREA) {
            Player_SetStart(pl, 7623.0f, -56.0f, 10173.9f, 0x40B6);
        }
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_TAVERN_1) == 0 && Village_FacilityLevel(VILLAGE_FACILITY_TAVERN) > 0) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_TAVERN_1);
            gLobbyStartPosIdx = LOBBY_START_POS_TAVERN;
            CamReq_TownTour(TOWN_TOUR_TAVERN_1);
            Bgm_PlayTownTune(0, 0);
            EventFlag_Set(TOWN_TOUR_GALLERY_ENTRY + TOWN_TOUR_TAVERN_1);
            return 1;
        }
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_TAVERN_2) == 0 && Village_FacilityLevel(VILLAGE_FACILITY_TAVERN) >= 2) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_TAVERN_2);
            gLobbyStartPosIdx = LOBBY_START_POS_TAVERN;
            CamReq_TownTour(TOWN_TOUR_TAVERN_2);
            Bgm_PlayTownTune(0, 0);
            EventFlag_Set(TOWN_TOUR_GALLERY_ENTRY + TOWN_TOUR_TAVERN_2);
            return 1;
        }
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_SMITHY_1) == 0 && Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) > 0) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_SMITHY_1);
            gLobbyStartPosIdx = LOBBY_START_POS_SMITHY;
            CamReq_TownTour(TOWN_TOUR_SMITHY_1);
            Bgm_PlayTownTune(0, 0);
            EventFlag_Set(TOWN_TOUR_GALLERY_ENTRY + TOWN_TOUR_SMITHY_1);
            return 1;
        }
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_SMITHY_2) == 0 && Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) >= 2) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_SMITHY_2);
            gLobbyStartPosIdx = LOBBY_START_POS_SMITHY;
            CamReq_TownTour(TOWN_TOUR_SMITHY_2);
            Bgm_PlayTownTune(0, 0);
            EventFlag_Set(TOWN_TOUR_GALLERY_ENTRY + TOWN_TOUR_SMITHY_2);
            return 1;
        }
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_PORT_1) == 0 && Village_FacilityLevel(VILLAGE_FACILITY_PORT) > 0) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_PORT_1);
            gLobbyStartPosIdx = LOBBY_START_POS_PORT;
            CamReq_TownTour(TOWN_TOUR_PORT_1);
            Bgm_PlayTownTune(0, 0);
            EventFlag_Set(TOWN_TOUR_GALLERY_ENTRY + TOWN_TOUR_PORT_1);
            return 1;
        }
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_PORT_2) == 0 && Village_FacilityLevel(VILLAGE_FACILITY_PORT) >= 2) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_PORT_2);
            gLobbyStartPosIdx = LOBBY_START_POS_PORT;
            CamReq_TownTour(TOWN_TOUR_PORT_2);
            Bgm_PlayTownTune(0, 0);
            EventFlag_Set(TOWN_TOUR_GALLERY_ENTRY + TOWN_TOUR_PORT_2);
            return 1;
        }
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_VILLAGE_LEVEL_2) == 0 && Save_TestEventFlag(EVENT_FLAG_VILLAGE_LEVEL_2) == 1) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_VILLAGE_LEVEL_2);
            gLobbyStartPosIdx = LOBBY_START_POS_VILLAGE;
            CamReq_TownTour(TOWN_TOUR_VILLAGE_LEVEL_2);
            return 1;
        }
        if (gLobbyStartPosIdx == LOBBY_START_POS_AREA || Village_MilestoneState(0) != 0) {
            if (Save_TestEventFlag(EVENT_FLAG_TOUR_VILLAGE_RETURN) == 0) {
                gLobbyStartPosIdx = LOBBY_START_POS_RETURN;
                CamReq_TownTour(TOWN_TOUR_VILLAGE_RETURN);
                Jingle_Play(pl, BGM_TUNE_TOUR_VILLAGE_RETURN);
                Save_SetEventFlag(EVENT_FLAG_TOUR_VILLAGE_RETURN);
                return 1;
            }
        }
        break;
    case AREA_ONLINE_HOUSE_1:
        if (MyHouse_IsOwner() == 1 && Save_TestEventFlag(EVENT_FLAG_TOUR_HOUSE_1) == 0) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_HOUSE_1);
            gLobbyStartPosIdx = LOBBY_START_POS_HERE;
            CamReq_TownTour(TOWN_TOUR_HOUSE_1);
            Jingle_Play(pl, BGM_TUNE_TOUR_HOUSE);
            return 1;
        }
        break;
    case AREA_ONLINE_HOUSE_2:
        if (MyHouse_IsOwner() == 1 && Save_TestEventFlag(EVENT_FLAG_TOUR_HOUSE_2) == 0) {
            Save_SetEventFlag(EVENT_FLAG_TOUR_HOUSE_2);
            gLobbyStartPosIdx = LOBBY_START_POS_HERE;
            CamReq_TownTour(TOWN_TOUR_HOUSE_2);
            Jingle_Play(pl, BGM_TUNE_TOUR_HOUSE);
            return 1;
        }
        break;
    }
    return 0;
}

void Stream_Stop(s32 ch);
void Stream_Reset(void);
void LobbyBgm_Shutdown(void);
void TownPc_Remove(Player *p);
void Mem_Fill(void *dst, u8 c, s32 n);
void TownPc_InitAll(s32 a);

/*
 * Leaves the lobby screen before the entry `mode` (enum LobbyEnterMode)
 * that follows. For LOBBY_ENTER_VILLAGE_RETURN and LOBBY_ENTER_TOWN the
 * music and the screen fade out (16 frames). Once the fade has ended the
 * streams stop, the lobby's music shuts down and the area is freed;
 * LOBBY_ENTER_VILLAGE_RETURN also removes the other players (their
 * characters, gPlayers rows, gNetUsers and members). Returns
 * LOBBY_STEP_BUSY, then LOBBY_STEP_DONE.
 */
s32 LobbyScreen_FadeOut(s32 mode)
{
    s16 *step = &gLobby->seqStep;
    s32 i;

    switch (*step) {
    case 0:
        *step = *step + 1;
        switch (mode) {
        case LOBBY_ENTER_TOWN:
        case LOBBY_ENTER_VILLAGE_RETURN:
            Stream_FadeOut(BGM_CH_MUSIC, 0x10);
            gLobby->countdown = 0x10;
            FadeA_Start(FADE_OUT_BLACK);
            break;
        }
        break;
    case 1:
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            Stream_Stop(BGM_CH_MUSIC);
            Stream_Stop(BGM_CH_TUNE);
            Stream_Reset();
            LobbyBgm_Shutdown();
            Town_FreeArea();
            switch (mode) {
            case LOBBY_ENTER_VILLAGE_RETURN:
                for (i = 0; i < LOBBY_MEMBER_COUNT; i++) {
                    if (gScene.localPlayer != i) {
                        TownPc_Remove(&gPlayers[i]);
                        Mem_Fill(&gPlayers[i], 0, sizeof(Player));
                        Mem_Fill(&gNetUsers[i], 0, sizeof(NetUser));
                        Mem_Fill(&gLobby->members[i], 0, sizeof(NetUser));
                        gLobby->pcLoaded[i] = 0;
                    }
                }
                TownPc_InitAll(1);
                break;
            }
            gLobby->seqStep = 0;
            return LOBBY_STEP_DONE;
        }
        break;
    }
    return LOBBY_STEP_BUSY;
}

void Loading_Request(void);
s32 Sound_StopBank(s32 slot);
void FlushCache(s32 kind);
void Sound_QueueBank(u32 id, void *buf);
s32 FileQueue_IsBusy(void);
struct SndFile;
s32 Sound_LoadBank(struct SndFile *f, u32 slot);
s32 Sound_CheckTransfer(void);
void NetMenu_Open(u8 mode);
/* unit's own declaration: declared s32 here, and the caller keeps the low byte; with the definition's u8 return, LobbyScreen_LoadBanks compiles differently */
s32 NetMenu_Update(void);
void Stream_SetVol(s32 ch, s32 vol);
void Stream_FadeToMax(s32 ch, s16 frames);
extern s8 gVoiceBanks[];                   /* the voice banks: 16 voices per sex */
extern s16 gAreaSeBanks[];                  /* an area's own bank, by area */
extern s16 gLobbyCommonSeBank;
void Lobby_DrawBlackLine(void);

/*
 * Loads the lobby screen's sound banks, each in three sub-steps
 * (gLobbyScreenSubStep: the file queued, loaded into its slot, then the
 * transfer waited for): the common bank (gLobbyCommonSeBank), the
 * player's voice, the town's bank (the house's, or the town's online or
 * offline) in the monsters' slot, and the area's own bank. Offline the
 * menus' layers stay hidden meanwhile. With gSysSettings.offline set
 * (offline only), the music then fades out, the network menu asks
 * whether to save (NETMENU_MODE_SAVE_ASK) and the screen fades in again.
 * Returns LOBBY_STEP_BUSY, then LOBBY_STEP_DONE.
 */
s32 LobbyScreen_LoadBanks(void)
{
    u8 *buf = gSysSettings.dataBuf;
    u32 bank;

    gSoundLoading = 1;
    if (Game_IsOnline() == 0) {
        gLobbyMenu->hideLayers = 1;
    }
    switch (gLobbyScreenSubStep) {
    case 0:
        Loading_Request();
        gLobbyScreenSubStep++;
        Sound_StopBank(SE_SLOT_COMMON);
        bank = SND_BANK_FILE(gLobbyCommonSeBank);
        FlushCache(0);
        Sound_QueueBank(bank, buf);
        break;
    case 1:
        Loading_Request();
        if (FileQueue_IsBusy() == 0) {
            gLobbyScreenSubStep++;
            Sound_LoadBank((struct SndFile *) buf, SE_SLOT_COMMON);
        }
        break;
    case 2:
        Loading_Request();
        if (Sound_CheckTransfer() == 0) {
            gLobbyScreenSubStep++;
        }
        break;
    case 3:
        Loading_Request();
        gLobbyScreenSubStep++;
        Sound_StopBank(SE_SLOT_PLAYER(0));
        bank = SND_BANK_FILE(gVoiceBanks[gPlayers[gScene.localPlayer].gender * 0x10 + gPlayers[gScene.localPlayer].voice] + SND_BANK_VOICES);
        FlushCache(0);
        Sound_QueueBank(bank, buf);
        break;
    case 4:
        Loading_Request();
        if (FileQueue_IsBusy() == 0) {
            gLobbyScreenSubStep++;
            Sound_LoadBank((struct SndFile *) buf, SE_SLOT_PLAYER(0));
        }
        break;
    case 5:
        Loading_Request();
        if (Sound_CheckTransfer() == 0) {
            gLobbyScreenSubStep++;
        }
        break;
    case 6:
        Loading_Request();
        gLobbyScreenSubStep++;
        Sound_StopBank(SE_SLOT_EM);     /* the town has no monsters: its bank goes in their slot */
        if (gScene.area == AREA_HOUSE || gScene.area == AREA_HUB_0x57) {
            bank = SND_BANK_FILE(SND_BANK_HOUSE);
        } else if (Game_IsOnline() == 1) {
            bank = SND_BANK_FILE(SND_BANK_TOWN_NET);
        } else {
            bank = SND_BANK_FILE(SND_BANK_TOWN);
        }
        FlushCache(0);
        Sound_QueueBank(bank, buf);
        break;
    case 7:
        Loading_Request();
        if (FileQueue_IsBusy() == 0) {
            gLobbyScreenSubStep++;
            Sound_LoadBank((struct SndFile *) buf, SE_SLOT_EM);
        }
        break;
    case 8:
        Loading_Request();
        if (Sound_CheckTransfer() == 0) {
            gLobbyScreenSubStep++;
        }
        break;
    case 9:
        Loading_Request();
        gLobbyScreenSubStep++;
        Sound_StopBank(SE_SLOT_SCENE);
        bank = SND_BANK_FILE(gAreaSeBanks[gScene.area] + SND_BANK_AREAS);
        FlushCache(0);
        Sound_QueueBank(bank, buf);
        break;
    case 10:
        Loading_Request();
        if (FileQueue_IsBusy() == 0) {
            gLobbyScreenSubStep++;
            Sound_LoadBank((struct SndFile *) buf, SE_SLOT_SCENE);
        }
        break;
    case 11:
        if (Sound_CheckTransfer() == 0) {
            if (Game_IsOnline() == 1 || gSysSettings.offline == 0) {
                gLobbyScreenSubStep = 0xE;
            } else {
                gLobbyScreenSubStep++;
                Stream_FadeOut(BGM_CH_MUSIC, BGM_FADE_FRAMES);
                gLobby->countdown = 0xF;
                NetMenu_Open(NETMENU_MODE_SAVE_ASK);
                LobbyMenu_SetDraw(Lobby_DrawBlackLine, 0, 0, 0);
            }
            gSoundLoading = 0;
        }
        break;
    case 12:
        if (gLobby->countdown != 0) {
            gLobby->countdown--;
        } else {
            Stream_Stop(BGM_CH_TUNE);
            Stream_Pause(BGM_CH_MUSIC, 1);
            gLobbyScreenSubStep++;
            FadeA_Start(FADE_IN_BLACK_DELAYED);
            gLobbyMenu->hideLayers = 0;
        }
        break;
    case 13:
        gLobbyMenu->hideLayers = 0;
        if ((u8)NetMenu_Update()) {
            gLobbyScreenSubStep++;
            FadeA_Start(FADE_BLACK);
            Stream_Pause(BGM_CH_MUSIC, 0);
            Stream_SetVol(BGM_CH_MUSIC, 0);
            Stream_FadeToMax(BGM_CH_MUSIC, BGM_FADE_FRAMES);
        }
        break;
    case 14:
        gLobbyMenu->hideLayers = 0;
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            gSoundLoading = 0;
            gLobby->connected = 1;
            FadeA_Start(FADE_IN_BLACK_DELAYED);
            gLobbyScreenSubStep = 0;
            return LOBBY_STEP_DONE;
        }
        break;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * The lobby screen's mode LOBBY_SCREEN_START: the screen goes black and
 * the fade is waited for. Online the mode goes on to
 * LOBBY_SCREEN_ENTER_AREA; offline the village is entered here
 * (LOBBY_ENTER_VILLAGE_OFFLINE, the menus' layers hidden meanwhile), then
 * the town runs, or the area's tour if one started. The pad is ignored
 * for 6 frames after.
 */
void LobbyScreen_Start(void)
{
    s32 result;
    u8 state;

    if (Game_IsOnline() == 0) {
        gLobbyMenu->hideLayers = 1;
    }
    switch (gLobbyScreenStep) {
    case 0:
        FadeA_Start(FADE_BLACK);
        gLobbyScreenStep += 1;
        break;
    case 1:
        if ((FadeA_GetState() & 0xFF) != FADE_STATE_RUNNING) {
            gLobbyScreenStep += 1;
        }
        break;
    case 2:
        state = FadeA_GetState() & 0xFF;
        if (state != FADE_STATE_RUNNING) {
            if (Game_IsOnline() == 0) {
                gLobbyScreenStep += 1;
                break;
            }
            gLobbyScreenStep = 0;
            gLobbyScreenMode += 1;
        }
        break;
    case 3:
        result = LobbyScreen_EnterSeq(LOBBY_ENTER_VILLAGE_OFFLINE);
        switch (result) {
        case LOBBY_STEP_DONE:
            gLobbyScreenStep = 0;
            gMenuInputLock = 6;
            gLobbyScreenMode = LOBBY_SCREEN_TOWN;
            break;
        case LOBBY_ENTER_TOURING:
            gLobbyScreenStep = 0;
            gMenuInputLock = 6;
            gLobbyScreenMode = LOBBY_SCREEN_TOUR;
            gTownMenuOpen = 1;
            gTownMode = TOWN_MODE_SCENE;
        }
        break;
    }
}

void Town_SetNpcArea(u16 area);
void TownPc_SyncMembers(void);
void Bgm_PlayAreaMusic(u16);
s32 LobbyBgm_GetTrack(void);
void LobbyBgm_Replay(void);
void CamCtl_Update(s32 a);
s32 LobbyArea_StartEvent(void);
s32 LobbyScreen_LoadBanks(void);

/*
 * The lobby screen's mode LOBBY_SCREEN_ENTER_AREA, after LEAVE_AREA: loads
 * the area the player goes to (Player.nextArea) with its characters, or
 * moves them over when it is the other half of the same place
 * (Town_SetNpcArea); entering AREA_HUB_0x57 from elsewhere loads the sound
 * banks again. Then the area's music and the fade in, and the town runs
 * (LOBBY_SCREEN_TOWN), unless the area's one-time scene or tour starts
 * (LOBBY_SCREEN_TOUR): the player is then put at the tour's start
 * position.
 */
void LobbyScreen_EnterArea(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 samePlace;
    Vec3f startPos;

    switch (gLobbyScreenStep) {
    case 0:
        gLobby->connected = 0;
        gSoundLoading = 1;
        Loading_Start();
        samePlace = Town_EnterArea(pl->nextArea, 1);
        if (samePlace == 0) {
            Town_SpawnNpcs(pl->nextArea);
        } else {
            Town_SetNpcArea(pl->nextArea);
        }
        gTownActionDelay = 20;
        TownPc_SyncMembers();
        FadeA_Start(FADE_BLACK);
        gSoundLoading = 0;
        Loading_Stop();
        if (pl->nextArea == AREA_HUB_0x57 && samePlace == 0) {
            gLobbyScreenStep++;
        } else {
            gLobbyScreenStep += 2;
            Bgm_PlayAreaMusic(pl->nextArea);
        }
        if (gScene.area == AREA_TOWN_ARENA && LobbyBgm_GetTrack() != -1) {
            LobbyBgm_Replay();
        }
        break;
    case 1:
        switch (LobbyScreen_LoadBanks()) {
        case LOBBY_STEP_DONE:
            gLobbyScreenStep++;
            Bgm_PlayAreaMusic(gScene.area);
            break;
        }
        break;
    case 2:
        if (--gLobbyScreenSubStep <= 0) {
            gLobbyScreenStep = 0;
            gLobbyScreenSubStep = 0;
            gLobbyScreenMode++;
            FadeA_Start(FADE_IN_BLACK_DELAYED);
            if (gSysSettings.galleryPlay != 1) {
                gLobby->connected = 1;
                gTownMenuOpen = 0;
                CamCtl_Update(0);
                if (LobbyArea_StartEvent() == 1) {
                    if (gLobbyStartPosIdx != LOBBY_START_POS_HERE && gLobbyStartPosIdx != LOBBY_START_POS_AREA) {
                        gLobbyStartPos.pos[0] = gPlayerPosRows[gScene.localPlayer].v;
                        startPos = gLobbyStartPos.pos[gLobbyStartPosIdx];
                        gPlayerPosRows[gScene.localPlayer].v = startPos;
                        gPlayerMoveToRows[gScene.localPlayer].v = startPos;
                    }
                    gFieldControl = 0;
                    gLobbyScreenStep = 0;
                    gLobbyScreenMode = LOBBY_SCREEN_TOUR;
                    gTownMenuOpen = 1;
                    gTownMode = TOWN_MODE_SCENE;
                }
            }
        }
        break;
    }
}

/*
 * The lobby screen's mode LOBBY_SCREEN_LEAVE_AREA: online the moving
 * message shows, and leaving AREA_HUB_0x85 or an online house sends the
 * hunter card if it changed; offline the screen goes black. The player
 * stops and the music fades out (an NPC's tune, or AREA_TOWN_ARENA's, is
 * stopped). Once the fade has ended the music is paused (offline) or
 * stopped, the area freed and the notices cleared:
 * LOBBY_SCREEN_ENTER_AREA follows.
 */
void LobbyScreen_LeaveArea(void)
{
    Player *pl;
    u16 area;
    u8 localPlayer;
    u8 state;

    localPlayer = gScene.localPlayer;
    pl = &gPlayers[localPlayer];
    if (Game_IsOnline() == 1) {
        gLobbyMenu->redraw = 1;
    }
    switch (gLobbyScreenStep) {
    case 0:
        if (Game_IsOnline() == 1) {
            LobbyText_Open(LOBBY_TEXT_AREA_MOVING);
        } else {
            FadeA_Start(FADE_BLACK);
        }
        pl->drunk = 0;
        Actor_SetPosture(pl, 0, 0, 0);
        Actor_SetState(pl, 0, 1U);
        gTownMenuOpen = 1;
        if ((Game_IsOnline() == 1) && ((area = gScene.area, (area == AREA_HUB_0x85)) || (Area_IsOnlineHouse(area) == 1)) && (gLobbyTextDirty != 0)) {
            HunterCard_SendChanged();
        }
        gLobbyScreenStep += 1;
        /* falls through */
    case 1:
        Stream_FadeOut(BGM_CH_MUSIC, 0x10);
        if (Bgm_IsNpcTunePlaying() == 1) {
            Bgm_StopEvent(pl);
        } else if (gScene.area == AREA_TOWN_ARENA) {
            Bgm_StopEventFade(pl);
        }
        gLobbyScreenStep += 1;
        break;
    case 2:
        state = FadeA_GetState() & 0xFF;
        if (state != FADE_STATE_RUNNING) {
            if (Game_IsOnline() == 0) {
                Stream_Pause(BGM_CH_MUSIC, 1);
            } else {
                LobbyBgm_StopAll();
                LobbyBgm_Stop();
            }
            Town_FreeArea();
            gLobbyScreenStep = 0;
            gLobbyScreenMode = LOBBY_SCREEN_ENTER_AREA;
            gLobbyScreenSubStep = 0;
            Notice_ResetQueue();
        }
        break;
    }
}

void Town_Frame(void);
void Jingle_Play(void *obj, s32 id);
s32 CamReq_State(void);
void CamReq_Cancel(void);
s32 Poogie_PlaceByPlayer(void);
Em *Npc_Find(u8 kind);

/*
 * The lobby screen's mode LOBBY_SCREEN_TOUR: a scene or camera tour runs
 * with the player hidden, under TOWN_MODE_SCENE; in the online houses a
 * tune plays near the end of its 480 frames (16 s). Once the camera's
 * request has ended, the screen fades out. The player is put back where it
 * stood before the tour (gLobbyStartPos entry 0) when the tour set a
 * start position, and the NPC of kind 0x14 placed by it (Poogie_PlaceByPlayer). After 5 frames
 * the screen fades in and the town runs again (LOBBY_SCREEN_TOWN).
 */
void LobbyScreen_RunTour(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    Vec3f startPos;

    Town_Frame();
    switch (gLobbyScreenStep) {
    case 0:
        gTownMenuOpen = 1;
        gTownMode = TOWN_MODE_SCENE;
        gLobbyScreenStep++;
        switch (gScene.area) {
        case AREA_ONLINE_HOUSE_1:
        case AREA_ONLINE_HOUSE_2:
            gLobbyMenu->timer = 480;
            break;
        }
        break;
    case 1:
        pl->visible = 0;
        switch (gScene.area) {
        case AREA_ONLINE_HOUSE_1:
        case AREA_ONLINE_HOUSE_2:
            if (--gLobbyMenu->timer == 1) {
                Jingle_Play(&gPlayers[gScene.localPlayer], 0xB);
            }
            break;
        }
        switch (CamReq_State()) {
        case 0:
        case -1:
            gLobbyScreenStep++;
            FadeA_Start(FADE_OUT_BLACK);
            break;
        }
        break;
    case 2:
        pl->visible = 0;
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            gLobbyScreenStep++;
            gLobbyScreenSubStep = 5;
            if (gLobbyStartPosIdx != LOBBY_START_POS_HERE && gLobbyStartPosIdx != LOBBY_START_POS_AREA) {
                s32 ofs = gScene.localPlayer * sizeof(Player);

                startPos = gLobbyStartPos.pos[0];
                ((PlayerVecRow *)((u8 *)gPlayerPosRows + ofs))->v = startPos;
                ((PlayerVecRow *)((u8 *)gPlayerMoveToRows + ofs))->v = startPos;
            }
            pl->visible = 1;
            CamReq_Cancel();
            if (Poogie_PlaceByPlayer() == 1) {
                Actor_SetState(Npc_Find(0x14), 0, 0x58);
            }
        }
        break;
    case 3:
        if (--gLobbyScreenSubStep <= 0) {
            FadeA_Start(FADE_IN_BLACK);
            gLobbyScreenStep = 0;
            gLobbyScreenMode = LOBBY_SCREEN_TOWN;
            gFieldControl = 1;
            gLobbyScreenSubStep = 0;
            gTownMenuOpen = 0;
            gTownMode = 0;
            gLobbyStartPosIdx = LOBBY_START_POS_HERE;
        }
        break;
    }
}

s32 ShipMenu_IsTouring(void);

/*
 * Which tour runs, for the village port's ship (Area_Update): 2 during
 * TOWN_TOUR_VILLAGE_RETURN (the tour that leaves the player at
 * LOBBY_START_POS_RETURN), 1 during the ship menu's tour, else 0.
 */
s32 LobbyScreen_TourKind(void)
{
    if (gLobbyScreenMode == LOBBY_SCREEN_TOUR) {
        if (gLobbyStartPosIdx == LOBBY_START_POS_RETURN) {
            return 2;
        }
        goto none;  /* the jump stays: the game's code tests the ship's tour only outside a tour */
    }
    if (ShipMenu_IsTouring() == 1) {
        return 1;
    }
none:
    return 0;
}

extern void (*gLobbyScreenSteps[])(void);

/* 1 while a scene or tour runs (LOBBY_SCREEN_TOUR) or the ship menu's
 * tour does, else 0. The season's banner (SeasonBanner_Run) waits for it. */
s32 LobbyScreen_IsTouring(void)
{
    if (gLobbyScreenMode == LOBBY_SCREEN_TOUR) {
        return 1;
    }
    if (ShipMenu_IsTouring() == 1) {
        return 1;
    }
    return 0;
}

/* Runs the lobby screen's mode for the frame (gLobbyScreenSteps), then
 * queues the menus' layers unless the mode hid them. Returns 0. The
 * gallery's town tours run the screen with it. */
s32 LobbyScreen_RunStep(void)
{
    gLobbyScreenSteps[gLobbyScreenMode]();
    if (gLobbyMenu->hideLayers == 0) {
        LobbyMenu_QueueLayers();
    }
    return 0;
}

/* The file's data: the start positions, then the screen's three tables of
 * steps by mode (enum LobbyScreenMode). */
void LobbyScreen_EnterArea(void);
void LobbyScreen_Start(void);
void LobbyScreen_LeaveArea(void);
void LobbyScreen_RunTour(void);
LobbyStartPos gLobbyStartPos = {
#include "lobby/008195A0.inc"
};
void (*gLobbySceneSteps[8])(void) = {
    LobbyScreen_Start, LobbyScreen_EnterArea, Town_Frame, LobbyScreen_LeaveArea,
    LobbyScreen_RunTour, NULL, NULL, NULL,
};
void (*gLobbyScreenModes[8])(LobbyState *p) = {
    (void (*)(LobbyState *p))LobbyScreen_Start, (void (*)(LobbyState *p))LobbyScreen_EnterArea, (void (*)(LobbyState *p))Town_Frame, (void (*)(LobbyState *p))LobbyScreen_LeaveArea,
    (void (*)(LobbyState *p))LobbyScreen_RunTour, NULL, NULL, NULL,
};
void (*gLobbyScreenSteps[8])(void) = {
    LobbyScreen_Start, LobbyScreen_EnterArea, Town_Frame, LobbyScreen_LeaveArea,
    LobbyScreen_RunTour, NULL, NULL, NULL,
};
