#include "common.h"
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
void func_327F70(void);
void Effect01_Spawn(void *obj, s16 type);
void Loading_Stop(void);
void Loading_SetMessage(u32 mode);
u8 Town_EnterArea(u16 a, u8 b); /* unit's own declaration: with the definition's u8 (u16 area, s32 v) (lobby/unk_7A6470.c), LobbyScreen_LoadArea 1 of 58 words differ */
void Town_SpawnNpcs(u16 a);

void Actor_SetAction(void *a, u16 mode, s32 b, s32 c);
void Actor_SetState(void *obj, s32 group, s32 state); /* real: void (void *obj, s32 group, s32 state); this file passes the object as an int */
s32 Area_IsOnlineHouse(s32);
void Town_FreeArea(void);
void LobbyText_Open(s32);
s32 LobbyText_Update(void);
void LobbyBgm_Stop(void);
void LobbyBgm_StopAll(void);
void Stream_Pause(s32, s32);
void Stream_FadeOut(s32, s32);
u8 SeqA_GetState(void);
void SeqA_Start(s32);
s32 func_367690(void);
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
s32 strcmp(char *a, char *b);     /* strcmp */

extern NetUser gNetUsers[10];         /* the members (their ids, names, texts) */
extern char gMyUserId[];                 /* the player's id (sub_main) */
extern char *D_0080C6C4;
s32 memcmp(void *a, void *b, s32 n);
char *strcpy(char *dst, const char *src);

s32 ChatLog_Lines(void);
u8 ChatLog_LinesBefore(u8 skip);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);


/* the chat log's scrolling (sub_main 0x5BC500: +5 the lines scrolled back,
 * +6 1 at the newest page, +7 the window's arrows: 0x80, 2 up, 1 more
 * above, 4 scrolled up, 8 scrolled down): pad 0x2000 scrolls back a line,
 * 0x1000 forward; returns pad */
u16 ChatLog_Scroll(u16 pad)
{
    u8 k;

    gHelpShow = 0;
    gChatLog.win.flags = 0x80;
    if (ChatLog_Lines() > 11U) {
        if (gChatLog.win.scroll != 0) {
            gChatLog.win.flags |= 2;
        }
        if (ChatLog_LinesBefore(gChatLog.win.scroll) > 0xB) {
            gChatLog.win.flags |= 1;
            if (pad & 0x2000) {
                k = gChatLog.win.scroll;
                if (k < gChatLog.win.count - 1) {
                    gChatLog.win.atEnd = 0;
                    gChatLog.win.scroll = k + 1;
                    gChatLog.win.flags |= 4;
                    Sound_PlaySeFlat(0, 0x17, 0);
                    if (ChatLog_LinesBefore(gChatLog.win.scroll) <= 0xB) {
                        gChatLog.win.atEnd = 1;
                    }
                }
            }
        } else {
            gChatLog.win.atEnd = 1;
        }
        if (pad & 0x1000) {
            if (gChatLog.win.scroll > 0) {
                gChatLog.win.scroll--;
                gChatLog.win.flags |= 8;
                Sound_PlaySeFlat(0, 0x17, 0);
                gChatLog.win.atEnd = 0;
            }
        }
    }
    if (gChatLog.win.scroll == 0) {
        gChatLog.win.open = gChatLog.win.timer = 0;
    }
    return pad;
}

void Gfx_SetState63(s32 a);
void ChatLog_DrawFrame(s32 a, u8 b, u8 c);
void ChatLog_DrawLines(u8 scroll, s32 dy, u8 atEnd, s32 size, u8 e); /* unit's own declaration: with the definition's void (u8 skip, s16 yoff, u8 fromTop, u8 size, u8 atEnd) (lobby/unk_7B8610.c), ChatLog_Draw 3 of 129 words differ */
f32 Math_Sin(f32 x);
void Chat_DrawArrow(s16 x, s16 y, s16 w, s16 h, u32 color, u8 flags);
extern u8 gChatTextSizes[];                   /* the chat text sizes (sub_main) */

/* draws the chat log window, and its two arrows (up at the size's left,
 * blinking while more lines are above; down) */
void ChatLog_Draw(void)
{
    u8 size;
    s16 x;
    s16 y;
    u32 col;
    u32 c;
    s8 a;

    Gfx_SetState63(0);
    size = gChatTextSizes[gSysSave.options.chatSize];
    ChatLog_DrawFrame(1, gSysSave.options.chatSize, 0);
    ChatLog_DrawLines(gChatLog.win.scroll, -10, gChatLog.win.atEnd, *(u8 *)&gSysSave.options.chatSize, 0);
    a = 48.0f * Math_Sin(9.5873801e-05f * (u16)((gFrameTick & 0x1F) << 11));
    col = ((a + 0xCF) << 24) | 0x10F213;
    x = size * 10 + 9;
    y = 0x194 - (size * 18 / 21 + 1) * 12;
    if (gChatLog.win.flags & 1) {
        c = col;
        if (gChatLog.win.flags & 4) {
            y -= 2;
        }
    } else {
        c = 0xD8707070;
    }
    Chat_DrawArrow(x, y, 0x1E, 0x18, c, 2);
    y = 0x19B;
    if (gChatLog.win.flags & 2) {
        if (gChatLog.win.flags & 8) {
            y += 2;
        }
    } else {
        col = 0xD8707070;
    }
    Chat_DrawArrow(x, y, 0x1E, 0x18, col, 3);
}

/* 1 when id isn't among the n speakers of list */
s32 ChatLog_IsNewSpeaker(char *id, ChatSpeaker *list, s32 n)
{
    ChatSpeaker *p;
    s32 k;

    for (p = list, k = n; k != 0; k--, p++) {
        if (strcmp(p->id, id) == 0) {
            return 0;
        }
    }
    return 1;
}


/* lists the speakers of the chat log (up to 0x40), without the player,
 * D_0080C6C4, "STAFF" ids and repeats; returns their number */
s32 ChatLog_ListSpeakers(ChatSpeaker *out)
{
    ChatLogLine *e;
    s32 k;
    s32 n;

    k = gChatLog.win.count;
    if (k == 0) {
        out[0].id[0] = 0;
        out[0].name[0] = 0;
        return 0;
    }
    for (e = gChatLog.lines, n = 0; k != 0; k--, e++) {
        if (e->color[0] != 0xFF && strcmp(e->id, gMyUserId) != 0 && strcmp(e->id, D_0080C6C4) != 0
            && memcmp(e->id, "STAFF", 5) != 0 && ChatLog_IsNewSpeaker(e->id, out, n) == 1) {
            strcpy(out[n].id, e->id);
            strcpy(out[n].name, e->name);
            n++;
        }
    }
    if ((u32)n < 0x40) {
        out[n].id[0] = 0;
        out[n].name[0] = 0;
    }
    return n;
}

/* the scene's step (gLobbySceneSteps[gLobbyScreenMode]) after Lobby_CountPlayTime */
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

/* the lobby screen, each frame: the mode gLobbyScreenMode's function
 * (gLobbyScreenModes), then the message window; 1 when gTownMode is 0x21 (leave
 * to the village), -1 when it is 0x25, else 0 */
s32 LobbyScreen_Run(void)
{
    LobbyState *p;

    Lobby_CountPlayTime();
    LobbyTime_Tick();
    gLobbyMenu->redraw = 0;
    gLobbyMenu->hideLayers = 0;
    Text_SetSize(6);
    if (gMenuInputLock > 0) {
        gMenuInputLock--;
    }
    (p = gLobby)->buttons = LobbyPad_Read(0);
    gLobbyScreenModes[gLobbyScreenMode](p);
    if (gLobbyMenu->hideLayers == 0) {
        LobbyMenu_QueueLayers();
    }
    if (gTownMode == 0x21) {
        gScene.online = 0;
        Hud_StartQuest();
        LobbyMenu_SetDraw(0, 0, 0, 0);
        return 1;
    } else if (gTownMode == 0x25) {
        Hud_StartQuest();
        LobbyMenu_SetDraw(0, 0, 0, 0);
        return -1;
    }
    return 0;
}

void Village_UpdateProgress(s32 a);
void VillageGather_DailyReset(void);
extern s8 gContestPrizeGiven;
extern s16 gPoogieMoods;
extern s16 D_008BD6C6;
extern s16 D_008BD6C8;
extern s16 D_008BD6CA;
void Save_ClearNewMarks(void);
void Rate_RaiseRandom(void);
s32 func_007106B0(void);
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

/* entering the lobby screen: resets, the online bonus state (0x5D4780),
 * the prizes and boards (Rate_RaiseRandom ...), and the quest lists: kept
 * from sub_main's copy when the day and the list are the same */
void LobbyScreen_InitLists(void)
{
    s32 r;
    TownQuestLists *a;
    TownQuestLists *b;

    gScene.mealAttack = 0;
    gScene.mealDefense = 0;
    gScene.mealHealth = 0;
    gScene.mealStamina = 0;
    gLobby->mealEaten = 0;
    gContestPrizeGiven = 0;
    Village_UpdateProgress(0);
    VillageGather_DailyReset();
    gPoogieMoods = 0;
    D_008BD6C6 = 0;
    D_008BD6C8 = 0;
    D_008BD6CA = 0;
    if (gSysSettings.offline == 0) {
        if (Game_IsOnline() == 0) {
            gOnlineBonusState = 0;
        }
    } else {
        Save_ClearNewMarks();
        if (gOnlineBonusState == 1) {
            if (D_5D4924 != 7) {
                gOnlineBonusState = 2;
            } else {
                gOnlineBonusState = 0;
            }
        } else {
            gOnlineBonusState = 0;
        }
    }
    if (Game_IsOnline() == 0) {
        if (gSysSettings.offline != 0 && D_5D4924 != 7) {
            Rate_RaiseRandom();
        }
        r = func_007106B0();
        if (r == 1) {
            Rate_UpdatePrices();
        }
        Rate_TakeSnapshot();
        if (r == 1) {
            Rotation_Update(0);
            Rotation_Update(1);
            Rotation_Update(2);
        }
        gContestLevel = Contest_RollLevel();
    }
    if (D_5D4924 != 7 || gLobbyLastSeason != LobbyWorld_GetSeason() || gLobbyLastPm != LobbyTime_IsPm()) {
        if (Game_IsOnline() == 0) {
            ElderForecast_Roll();
        }
        QuestList_Rebuild();
    } else {
        a = &gTownQuestLists;
        b = &gTownQuestListsCopy;
        if (Game_IsOnline() == 0) {
            memcpy(a->quests, b->quests, sizeof(a->quests));
        }
    }
}

void FileQueue_Cancel(void);
void Game_ResetSubsystems(void);
void CamReq_Reset(void);
void Bgm_Init(void);
u16 Rand_Next(s32 stream);
extern u16 D_008BEB64;
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
extern u8 gMyName[];                   /* the player's name (sub_main) */
void Player_LoadFromSave(Player *p);
void HunterCard_Build(void *out);

extern u8 *gStageFile;
/* entering the lobby screen (kind 5: back from a quest, the members kept):
 * clears the room's state, the world clock from the save when offline
 * (0x5D7188), the lists (LobbyScreen_InitLists), the player's id, name and text */
void LobbyScreen_Enter(s32 kind)
{
    s32 i;
    TownQuestLists *t;
    s32 j;

    FileQueue_Cancel();
    for (i = 0; i < 10; i++) {
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
    D_008BEB64 = Rand_Next(1);
    gLobbyEnterRand = Rand_Next(1);
    if (Game_IsOnline() == 0 && gSysSettings.galleryPlay == 0) {
        Rate_ClearSnapshot();
        Season_Set(gSave.season);
        gTownSeason = gSave.season;
        gTownSeasonDays[0] = 3;
        gTownDayTicks = 0x960;
        gTownSeasonDays[1] = 3;
        gTownSeasonDays[2] = 3;
        gTownDay = gSave.day;
        gLobbyTime = gSave.time;
        LobbyTime_SetClock(gLobbyTime);
        if (gSysSettings.offline != 0) {
            LobbyWorld_CatchUp();
        }
        LobbyScreen_InitLists();
        t = (TownQuestLists *)&gTownQuestLists;
        memset(t->a, 0, sizeof(t->a));
        for (j = 0; j < 3; j++) {
            memcpy(&t->a[j][0], gSave.days[j][0], 8);
            memcpy(&t->a[j][1], gSave.days[j][1], 8);
            memcpy(&t->a[j][2], gSave.days[j][2], 8);
        }
    } else if (gTownDayTicks == 0) {
        gTownDayTicks = 0x960;
        gLobbyTime = gSave.time;
        LobbyTime_SetClock(gLobbyTime);
    }
    if (kind != 5) {
        memset(gLobby->members, 0, sizeof(gLobby->members));
        memset(gNetUsers, 0, sizeof(gNetUsers));
        memset(gLobby->pcLoaded, 0, 10);
        memset(gLobby->lookPending, 0, 10);
    }
    if (Game_IsOnline() == 0) {
        gScene.playerCount = 1;
        memcpy(gMyName, gSave.name, 0x11);
        memcpy(gMyUserId, "", 8);
    } else {
        memset(gStageFile, 0, 0x8000);
    }
    Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
    memcpy(gNetUsers[gScene.localPlayer].name, gMyName, 0x10);
    memcpy(gNetUsers[gScene.localPlayer].id, gMyUserId, 8);
    HunterCard_Build(gNetUsers[gScene.localPlayer].text);
}

/* before leaving for a quest: pays what is pending and resets the
 * players, receiving between the steps (Lobby_ReceiveWhileWaiting) */
void Lobby_PrepareQuest(void)
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
    func_327F70();
    Lobby_ReceiveWhileWaiting();
    Lobby_ReceiveWhileWaiting();
    for (i = 0, pl = gPlayers; i < 10; i++, pl++) {
        Effect01_Spawn(pl, 1);
    }
    if (Game_IsOnline() == 1) {
        gLobby->unk3968 = 1;
    }
    Loading_Stop();
    gSoundLoading = 0;
}

extern u8 gTownActionDelay;
extern u8 gMemberNamesOff;
extern u8 D_008BEB63;
void Quest_Clear(void);
void *Hud_Init(void);
void LobbyBgm_ClearState(void);

/* leaves the lobby screen's party state: the player's status reset,
 * the ready bits, the room's objects and the quest cleared */
void LobbyScreen_LeaveParty(void)
{
    Player *p = &gPlayers[gScene.localPlayer];
    s32 i;

    Actor_SetAction(p, 0, 0, 0);
    Actor_SetState(p, 0, 1);
    gTownMenuOpen = 1;
    memset(gLobby->slotIds, 0, 0x80);
    gLobby->readyBits = 0;
    gLobby->questTaken = 0;
    gLobby->questPaid = 0;
    for (i = 0; i < 10; i++) {
        gLobby->modelLoading[i] = 0;
    }
    gLobby->unk3969 = 0;
    gScene.menuOpen = 0;
    p->unk844 = 0;
    gTownActionDelay = 0x14;
    D_008BEB63 = 1;
    gTownMode = 0;
    gMemberNamesOff = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    Quest_Clear();
    Hud_Init();
    LobbyBgm_ClearState();
}

void LobbyScreen_LoadArea(s32 mode)
{
    u8 v;

    switch (mode) {
    case 5:
    case 0:
    case 1:
        v = 0;
        break;
    default:
        v = 1;
        break;
    }
    gSoundLoading = 1;
    Lobby_ReceiveWhileWaiting();
    Loading_SetMessage(0);
    if (Town_EnterArea(gPlayers[gScene.localPlayer].nextArea, v) == 0) {
        Lobby_ReceiveWhileWaiting();
        Town_SpawnNpcs(gPlayers[gScene.localPlayer].nextArea);
    }
    Loading_Stop();
    gSoundLoading = 0;
    gLobby->unk3968 = 1;
}

typedef struct LVec3 {
    f32 x;
    f32 y;
    f32 z;
} LVec3;
/* gPlayers seen with its two positions as vectors */
typedef struct PlayerPosView {
    u8 pad0[0xAC];
    LVec3 pos;
    u8 padB8[0x754 - 0xB8];
    LVec3 pos2;
    u8 pad760[0x970 - 0x760];
} PlayerPosView;
typedef struct StartPos {
    LVec3 pos[6];
} StartPos;
extern StartPos gLobbyStartPos;              /* the lobby's start positions */
/* the stride of gPlayers from one of its vectors (the original addresses
 * gPlayers + 0xAC and + 0x754 through symbols of their own) */
typedef struct PlayerVecAt {
    LVec3 v;
    u8 pad[0x970 - 0xC];
} PlayerVecAt;
extern PlayerVecAt D_5F8A7C[];
extern PlayerVecAt D_5F9124[];
extern u8 gTownSpawnSet;
extern s32 gLobbyStartPosIdx;
void HunterCard_Apply(s8 idx, void *src);
void TownPc_Spawn(u8 i, NetUser *u, char *name);
void Town_PlaceLocalPlayer(s32 area);
void CamCtl_Init(void);
void func_007A6460(s8 a);
void Town_StartAreaMusic(s32 a);
void TownPc_SyncMembers(void);
void TownMsg_Send0E(void);
void TownMsg_Send0D(void);
void TownMsg_SendArea(u16 area);
void TownMsg_Send02(Player *obj);
void CamCtl_Update(s32 a);
s32 LobbyScreen_LoadBanks(void);
void Loading_Request(void);
s32 LobbyArea_StartEvent(void);

/* the steps of entering the lobby screen (kind mode: 0, 1 from the
 * village, 5 or 6 online): the screen and its members, the party left and
 * the start area set (gPlayers[me] + 0x684), the banks loaded, then the
 * area started. Returns 2 while running, 3 when an area event starts, 0
 * when done */
s32 LobbyScreen_EnterSeq(s32 mode)
{
    s16 *step;
    s32 i;
    NetUser *u;

    gLobby->connected = 0;
    step = &gLobby->seqStep;
    switch (*step) {
    case 0:
        (*step)++;
        SeqA_Start(0xA);
        switch (mode) {
        case 6:
        case 5:
            LobbyScreen_Enter(mode);
            for (i = 0, u = gNetUsers; i < 10; i++, u++) {
                if (u->id[0] != 0) {
                    HunterCard_Apply(i, u->text);
                    TownPc_Spawn(i, u, u->name);
                }
            }
            break;
        case 0:
            LobbyScreen_Enter(mode);
            HunterCard_Apply(gScene.localPlayer, gNetUsers[gScene.localPlayer].text);
            TownPc_Spawn(gScene.localPlayer, &gNetUsers[gScene.localPlayer], gNetUsers[gScene.localPlayer].name);
            memcpy(&gLobby->members[gScene.localPlayer], &gNetUsers[gScene.localPlayer], sizeof(NetUser));
            gScene.playerCount = 1;
            break;
        case 1:
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
        Lobby_PrepareQuest();
        gLobby->seqStep++;
    case 2:
        LobbyScreen_LeaveParty();
        switch (mode) {
        case 6:
            gLobby->seqStep = 3;
            return 2;
        case 5:
            gTownSpawnSet = 0;
            if (gLobby->groupJoin != 0) {
                gPlayers[gScene.localPlayer].nextArea = 0x84;
            } else if (D_5CA910 == 0) {
                gPlayers[gScene.localPlayer].nextArea = 0x84;
            } else {
                gPlayers[gScene.localPlayer].nextArea = D_5CA910;
            }
            break;
        case 0:
            gTownSpawnSet = 0;
            if (gSave.unk36E8 != 0) {
                gPlayers[gScene.localPlayer].nextArea = 0x57;
            } else {
                gPlayers[gScene.localPlayer].nextArea = 0x89;
            }
            break;
        case 3: {
            s32 k = gScene.localPlayer;

            gPlayers[k].startPos[0] = 5762.0f;
            gPlayers[k].startPos[1] = -30.0f;
            gPlayers[k].startPos[2] = 10434.0f;
            gPlayers[k].startAngle = 0;
            gPlayers[k].nextArea = 0x83;
            gTownSpawnSet = 1;
            break;
        }
        case 1:
        case 2:
            gTownSpawnSet = 0;
            gPlayers[gScene.localPlayer].nextArea = 0x83;
            break;
        case 4:
            gTownSpawnSet = 0;
            gPlayers[gScene.localPlayer].nextArea = 0x84;
            break;
        default:
            gTownSpawnSet = 0;
            gPlayers[gScene.localPlayer].nextArea = 0x83;
            break;
        }
        switch (mode) {
        case 0:
        case 1:
        case 5:
            gLobby->seqStep++;
            gLobbyScreenSubStep = 0;
            LobbyScreen_LoadArea(mode);
            break;
        case 2:
            gLobby->seqStep = 4;
            Town_PlaceLocalPlayer(gPlayers[gScene.localPlayer].area);
            CamCtl_Init();
            break;
        case 3:
        case 4:
        default:
            gLobby->seqStep = 4;
            LobbyScreen_LoadArea(mode);
            break;
        }
        func_007A6460(0);
        break;
    case 3:
        switch (LobbyScreen_LoadBanks()) {
        case 0:
        CamCtl_Update(0);
        gLobby->seqStep = 0;
        gLobby->connected = 1;
        Town_StartAreaMusic(mode);
        SeqA_Start(0xF);
        func_007A6460(1);
        LobbyMenu_SetDraw(0, 0, 0, 0);
        switch (mode) {
        case 5:
            TownPc_SyncMembers();
            TownMsg_Send0E();
            TownMsg_Send0D();
            TownMsg_SendArea(gScene.area);
            TownMsg_Send02(&gPlayers[gScene.localPlayer]);
        }
        if (Game_IsOnline() == 0) {
            if (gSysSettings.galleryPlay == 1) {
                return 0;
            }
            if (LobbyArea_StartEvent() == 1) {
                if (gLobbyStartPosIdx != 0 && gLobbyStartPosIdx != -1) {
                    s32 k = gScene.localPlayer * 0x970;
                    LVec3 v;

                    gLobbyStartPos.pos[0] = ((PlayerVecAt *)((u8 *)D_5F8A7C + k))->v;
                    v = gLobbyStartPos.pos[gLobbyStartPosIdx];
                    ((PlayerVecAt *)((u8 *)D_5F8A7C + k))->v = v;
                    ((PlayerVecAt *)((u8 *)D_5F9124 + k))->v = v;
                }
                gFieldControl = 0;
                gSysSettings.offline = 0;
                return 3;
            }
        }
        gSysSettings.offline = 0;
        return 0;
        }
        break;
    case 4:
        Loading_Request();
        switch (mode) {
        case 4:
            TownPc_SyncMembers();
            TownMsg_Send0E();
            TownMsg_Send0D();
            TownMsg_SendArea(gScene.area);
            TownMsg_Send02(&gPlayers[gScene.localPlayer]);
        }
        gTownMenuOpen = 0;
        {
            LobbyState *p = gLobby;

            gFieldControl = 1;
            p->seqStep = 0;
        }
        gLobby->connected = 1;
        CamCtl_Update(0);
        SeqA_Start(0xF);
        Town_StartAreaMusic(mode);
        func_007A6460(1);
        LobbyMenu_SetDraw(0, 0, 0, 0);
        gSysSettings.offline = 0;
        return 0;
    }
    return 2;
}

s32 Save_TestEventFlag(s32 flag);
void Save_SetEventFlag(s32 flag);
u8 Village_FacilityLevel(s32 kind);
/* unit's own declaration: with the definition's s16 (s32 arg0) (lobby/unk_6CB690.c), LobbyArea_StartEvent: 86 of 370 words differ */
s32 Village_GetTimerState(s32 a);
s32 MyHouse_IsOwner(void);
void CamReq_LobbyEvent(void);
void CamReq_Lobby44(s32 n);
void Bgm_PlayField(s32 a, s32 b);
void EventFlag_Set(s32 n);
void Jingle_Play(void *obj, s32 id);
extern s32 gLobbyStartPosIdx;

/* places the player at an area's start: its position (also the previous
 * one, 0x754) and facing (0xE, 0xA4) */
static inline void Player_SetStart(Player *p, f32 x, f32 y, f32 z, s32 ang)
{
    p->pos[0] = x;
    p->pos[1] = y;
    p->pos[2] = z;
    p->rot[1] = p->targetYaw = ang;
    ((PlayerPosView *)p)->pos2 = ((PlayerPosView *)p)->pos;
}

/* on entering an area (0x6ADCC4), the one-time event it starts (the
 * facility and quest events, each guarded by its event flag): 1 when one
 * started. gLobbyStartPosIdx == -1: the player is placed at the area's start first */
s32 LobbyArea_StartEvent(void)
{
    Player *p = &gPlayers[gScene.localPlayer];

    switch (gScene.area) {
    case 0x57:
        if (gLobbyStartPosIdx == -1) {
            Player_SetStart(p, 10615.0f, 0.0f, 14774.0f, 0x8000);
        }
        if (Save_TestEventFlag(0x123) == 0) {
            Save_SetEventFlag(0x123);
            CamReq_LobbyEvent();
            return 1;
        }
        break;
    case 0xAA:
        if (Save_TestEventFlag(0x12B) == 0) {
            Save_SetEventFlag(0x12B);
            gLobbyStartPosIdx = 0;
            CamReq_Lobby44(7);
            Jingle_Play(p, 0xD6);
            return 1;
        }
        break;
    case 0xAB:
        if (Save_TestEventFlag(0x12C) == 0) {
            Save_SetEventFlag(0x12C);
            gLobbyStartPosIdx = 0;
            CamReq_Lobby44(8);
            Jingle_Play(p, 0xD7);
            return 1;
        }
        break;
    case 0xAC:
        if (Save_TestEventFlag(0x12D) == 0) {
            Save_SetEventFlag(0x12D);
            gLobbyStartPosIdx = 0;
            CamReq_Lobby44(9);
            Jingle_Play(p, 0xD8);
            return 1;
        }
        break;
    case 0x89:
        if (gLobbyStartPosIdx == -1) {
            Player_SetStart(p, 7623.0f, -56.0f, 10173.9f, 0x40B6);
        }
        if (Save_TestEventFlag(0x124) == 0 && Village_FacilityLevel(1) > 0) {
            Save_SetEventFlag(0x124);
            gLobbyStartPosIdx = 1;
            CamReq_Lobby44(0);
            Bgm_PlayField(0, 0);
            EventFlag_Set(0x1F);
            return 1;
        }
        if (Save_TestEventFlag(0x125) == 0 && Village_FacilityLevel(1) >= 2) {
            Save_SetEventFlag(0x125);
            gLobbyStartPosIdx = 1;
            CamReq_Lobby44(3);
            Bgm_PlayField(0, 0);
            EventFlag_Set(0x22);
            return 1;
        }
        if (Save_TestEventFlag(0x126) == 0 && Village_FacilityLevel(2) > 0) {
            Save_SetEventFlag(0x126);
            gLobbyStartPosIdx = 2;
            CamReq_Lobby44(1);
            Bgm_PlayField(0, 0);
            EventFlag_Set(0x20);
            return 1;
        }
        if (Save_TestEventFlag(0x127) == 0 && Village_FacilityLevel(2) >= 2) {
            Save_SetEventFlag(0x127);
            gLobbyStartPosIdx = 2;
            CamReq_Lobby44(4);
            Bgm_PlayField(0, 0);
            EventFlag_Set(0x23);
            return 1;
        }
        if (Save_TestEventFlag(0x128) == 0 && Village_FacilityLevel(3) > 0) {
            Save_SetEventFlag(0x128);
            gLobbyStartPosIdx = 3;
            CamReq_Lobby44(2);
            Bgm_PlayField(0, 0);
            EventFlag_Set(0x21);
            return 1;
        }
        if (Save_TestEventFlag(0x129) == 0 && Village_FacilityLevel(3) >= 2) {
            Save_SetEventFlag(0x129);
            gLobbyStartPosIdx = 3;
            CamReq_Lobby44(5);
            Bgm_PlayField(0, 0);
            EventFlag_Set(0x24);
            return 1;
        }
        if (Save_TestEventFlag(0x12A) == 0 && Save_TestEventFlag(0x13F) == 1) {
            Save_SetEventFlag(0x12A);
            gLobbyStartPosIdx = 4;
            CamReq_Lobby44(6);
            return 1;
        }
        if (gLobbyStartPosIdx == -1 || Village_GetTimerState(0) != 0) {
            if (Save_TestEventFlag(0x1CE) == 0) {
                gLobbyStartPosIdx = 5;
                CamReq_Lobby44(0xF);
                Jingle_Play(p, 0xDA);
                Save_SetEventFlag(0x1CE);
                return 1;
            }
        }
        break;
    case 0xAE:
        if (MyHouse_IsOwner() == 1 && Save_TestEventFlag(0x13B) == 0) {
            Save_SetEventFlag(0x13B);
            gLobbyStartPosIdx = 0;
            CamReq_Lobby44(0xC);
            Jingle_Play(p, 0xDB);
            return 1;
        }
        break;
    case 0xAF:
        if (MyHouse_IsOwner() == 1 && Save_TestEventFlag(0x13C) == 0) {
            Save_SetEventFlag(0x13C);
            gLobbyStartPosIdx = 0;
            CamReq_Lobby44(0xD);
            Jingle_Play(p, 0xDB);
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

/* the lobby screen's music fade out before leaving (kind 3 or 4), then
 * the streams stopped; kind 3 also clears the other players (their
 * gPlayers rows, the member lists). 0 done, 2 busy */
s32 LobbyScreen_FadeOut(s32 kind)
{
    s16 *step = &gLobby->seqStep;
    s32 i;

    switch (*step) {
    case 0:
        *step = *step + 1;
        switch (kind) {
        case 4:
        case 3:
            Stream_FadeOut(0, 0x10);
            gLobby->countdown = 0x10;
            SeqA_Start(1);
            break;
        }
        break;
    case 1:
        if ((u8)SeqA_GetState() != 1) {
            Stream_Stop(0);
            Stream_Stop(1);
            Stream_Reset();
            LobbyBgm_Shutdown();
            Town_FreeArea();
            switch (kind) {
            case 3:
                for (i = 0; i < 10; i++) {
                    if (gScene.localPlayer != i) {
                        TownPc_Remove(&gPlayers[i]);
                        Mem_Fill(&gPlayers[i], 0, 0x970);
                        Mem_Fill(&gNetUsers[i], 0, 0x11C);
                        Mem_Fill(&gLobby->members[i], 0, 0x11C);
                        gLobby->pcLoaded[i] = 0;
                    }
                }
                TownPc_InitAll(1);
                break;
            }
            gLobby->seqStep = 0;
            return 0;
        }
        break;
    }
    return 2;
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
/* unit's own declaration: with the definition's u8 (void) (sub_main/unk_3D54B0.c), LobbyScreen_LoadBanks: 19 of 365 words differ */
s32 NetMenu_Update(void);
void Stream_SetVol(s32 ch, s32 vol);
void Stream_FadeToMax(s32 ch, s16 frames);
extern s8 gVoiceBanks[];                   /* sub_main: a bank per gender (16 each) and ... */
extern s16 gAreaSeBanks[];                  /* sub_main: a bank per area */
extern s16 D_5BA5E0;
void Lobby_DrawBlackLine(void);

/* loads the lobby's sound banks (the lobby's, the player's voice by gender,
 * the area's, the area music's), then online the network menu (9)
 * before entering; 0 done, 2 busy */
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
        Sound_StopBank(1);
        bank = D_5BA5E0 | 0x10000;
        FlushCache(0);
        Sound_QueueBank(bank, buf);
        break;
    case 1:
        Loading_Request();
        if (FileQueue_IsBusy() == 0) {
            gLobbyScreenSubStep++;
            Sound_LoadBank((struct SndFile *) buf, 1);
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
        Sound_StopBank(2);
        bank = (gVoiceBanks[gPlayers[gScene.localPlayer].gender * 0x10 + gPlayers[gScene.localPlayer].voice] + 0x17D) | 0x10000;
        FlushCache(0);
        Sound_QueueBank(bank, buf);
        break;
    case 4:
        Loading_Request();
        if (FileQueue_IsBusy() == 0) {
            gLobbyScreenSubStep++;
            Sound_LoadBank((struct SndFile *) buf, 2);
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
        Sound_StopBank(6);
        if (gScene.area == 0x56 || gScene.area == 0x57) {
            bank = 0x10006;
        } else if (Game_IsOnline() == 1) {
            bank = 0x10005;
        } else {
            bank = 0x10004;
        }
        FlushCache(0);
        Sound_QueueBank(bank, buf);
        break;
    case 7:
        Loading_Request();
        if (FileQueue_IsBusy() == 0) {
            gLobbyScreenSubStep++;
            Sound_LoadBank((struct SndFile *) buf, 6);
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
        Sound_StopBank(7);
        bank = (gAreaSeBanks[gScene.area] + 0xB) | 0x10000;
        FlushCache(0);
        Sound_QueueBank(bank, buf);
        break;
    case 10:
        Loading_Request();
        if (FileQueue_IsBusy() == 0) {
            gLobbyScreenSubStep++;
            Sound_LoadBank((struct SndFile *) buf, 7);
        }
        break;
    case 11:
        if (Sound_CheckTransfer() == 0) {
            if (Game_IsOnline() == 1 || gSysSettings.offline == 0) {
                gLobbyScreenSubStep = 0xE;
            } else {
                gLobbyScreenSubStep++;
                Stream_FadeOut(0, 0xF);
                gLobby->countdown = 0xF;
                NetMenu_Open(9);
                LobbyMenu_SetDraw(Lobby_DrawBlackLine, 0, 0, 0);
            }
            gSoundLoading = 0;
        }
        break;
    case 12:
        if (gLobby->countdown != 0) {
            gLobby->countdown--;
        } else {
            Stream_Stop(1);
            Stream_Pause(0, 1);
            gLobbyScreenSubStep++;
            SeqA_Start(0xF);
            gLobbyMenu->hideLayers = 0;
        }
        break;
    case 13:
        gLobbyMenu->hideLayers = 0;
        if ((u8)NetMenu_Update()) {
            gLobbyScreenSubStep++;
            SeqA_Start(0xA);
            Stream_Pause(0, 0);
            Stream_SetVol(0, 0);
            Stream_FadeToMax(0, 0xF);
        }
        break;
    case 14:
        gLobbyMenu->hideLayers = 0;
        if ((u8)SeqA_GetState() != 1) {
            gSoundLoading = 0;
            gLobby->connected = 1;
            SeqA_Start(0xF);
            gLobbyScreenSubStep = 0;
            return 0;
        }
        break;
    }
    return 2;
}

void LobbyScreen_Start(void)
{
    s32 temp_v0;
    u8 state;

    if (Game_IsOnline() == 0) {
        *(s8 *)((u8 *)gLobbyMenu + 0x3D) = 1;
    }
    switch (gLobbyScreenStep) {
    case 0:
        SeqA_Start(0xA);
        gLobbyScreenStep += 1;
        break;
    case 1:
        if ((SeqA_GetState() & 0xFF) != 1) {
            gLobbyScreenStep += 1;
        }
        break;
    case 2:
        state = SeqA_GetState() & 0xFF;
        if (state != 1) {
            if (Game_IsOnline() == 0) {
                gLobbyScreenStep += 1;
                break;
            }
            gLobbyScreenStep = 0;
            gLobbyScreenMode += 1;
        }
        break;
    case 3:
        temp_v0 = LobbyScreen_EnterSeq(0);
        switch (temp_v0) {
        case 0:
            gLobbyScreenStep = 0;
            gMenuInputLock = 6;
            gLobbyScreenMode = 2;
            break;
        case 3:
            gLobbyScreenStep = 0;
            gMenuInputLock = 6;
            gLobbyScreenMode = 4;
            gTownMenuOpen = 1;
            gTownMode = 0x22;
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

/* the lobby screen's mode: entering the area (its music, the fade), then
 * the player placed at start position gLobbyStartPosIdx (0: where it was).
 * The positions are copied through D_5F8A7C and D_5F9124 (gPlayers + 0xAC
 * and + 0x754, the original's own symbols for them). */
void LobbyScreen_EnterArea(void)
{
    Player *p = &gPlayers[gScene.localPlayer];
    u8 r;
    LVec3 v;

    switch (gLobbyScreenStep) {
    case 0:
        gLobby->connected = 0;
        gSoundLoading = 1;
        Loading_Start();
        r = Town_EnterArea(p->nextArea, 1);
        if (r == 0) {
            Town_SpawnNpcs(p->nextArea);
        } else {
            Town_SetNpcArea(p->nextArea);
        }
        gTownActionDelay = 0x14;
        TownPc_SyncMembers();
        SeqA_Start(0xA);
        gSoundLoading = 0;
        Loading_Stop();
        if (p->nextArea == 0x57 && r == 0) {
            gLobbyScreenStep++;
        } else {
            gLobbyScreenStep += 2;
            Bgm_PlayAreaMusic(p->nextArea);
        }
        if (gScene.area == 0x87 && LobbyBgm_GetTrack() != -1) {
            LobbyBgm_Replay();
        }
        break;
    case 1:
        switch (LobbyScreen_LoadBanks()) {
        case 0:
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
            SeqA_Start(0xF);
            if (gSysSettings.galleryPlay != 1) {
                gLobby->connected = 1;
                gTownMenuOpen = 0;
                CamCtl_Update(0);
                if (LobbyArea_StartEvent() == 1) {
                    if (gLobbyStartPosIdx != 0 && gLobbyStartPosIdx != -1) {
                        gLobbyStartPos.pos[0] = D_5F8A7C[gScene.localPlayer].v;
                        v = gLobbyStartPos.pos[gLobbyStartPosIdx];
                        D_5F8A7C[gScene.localPlayer].v = v;
                        D_5F9124[gScene.localPlayer].v = v;
                    }
                    gFieldControl = 0;
                    gLobbyScreenStep = 0;
                    gLobbyScreenMode = 4;
                    gTownMenuOpen = 1;
                    gTownMode = 0x22;
                }
            }
        }
        break;
    }
}

void LobbyScreen_LeaveArea(void)
{
    Player *p;
    u16 area;
    u8 localPlayer;
    u8 state;

    localPlayer = gScene.localPlayer;
    p = &gPlayers[localPlayer];
    if (Game_IsOnline() == 1) {
        *(s8 *)((u8 *)gLobbyMenu + 0x39) = 1;
    }
    switch (gLobbyScreenStep) {
    case 0:
        if (Game_IsOnline() == 1) {
            LobbyText_Open(0x7D);
        } else {
            SeqA_Start(0xA);
        }
        p->unk844 = 0;
        Actor_SetAction(p, 0, 0, 0);
        Actor_SetState(p, 0, 1U);
        gTownMenuOpen = 1;
        if ((Game_IsOnline() == 1) && ((area = gScene.area, (area == 0x85)) || (Area_IsOnlineHouse(area) == 1)) && (gLobbyTextDirty != 0)) {
            LobbyText_Update();
        }
        gLobbyScreenStep += 1;
        
    case 1:
        Stream_FadeOut(0, 0x10);
        if (func_367690() == 1) {
            Bgm_StopEvent(p);
        } else if (gScene.area == 0x87) {
            Bgm_StopEventFade(p);
        }
        gLobbyScreenStep += 1;
        break;
    case 2:
        state = SeqA_GetState() & 0xFF;
        if (state != 1) {
            if (Game_IsOnline() == 0) {
                Stream_Pause(0, 1);
            } else {
                LobbyBgm_StopAll();
                LobbyBgm_Stop();
            }
            Town_FreeArea();
            gLobbyScreenStep = 0;
            gLobbyScreenMode = 1;
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
s32 Npc14_PlaceByPlayer(void);
Em *Npc_Find(u8 kind);

void LobbyScreen_CamIntro(void)
{
    Player *p = &gPlayers[gScene.localPlayer];
    LVec3 v;

    Town_Frame();
    switch (gLobbyScreenStep) {
    case 0:
        gTownMenuOpen = 1;
        gTownMode = 0x22;
        gLobbyScreenStep++;
        switch (gScene.area) {
        case 0xAE:
        case 0xAF:
            gLobbyMenu->timer = 0x1E0;
            break;
        }
        break;
    case 1:
        p->visible = 0;
        switch (gScene.area) {
        case 0xAE:
        case 0xAF:
            if (--gLobbyMenu->timer == 1) {
                Jingle_Play(&gPlayers[gScene.localPlayer], 0xB);
            }
            break;
        }
        switch (CamReq_State()) {
        case 0:
        case -1:
            gLobbyScreenStep++;
            SeqA_Start(1);
            break;
        }
        break;
    case 2:
        p->visible = 0;
        if ((u8)SeqA_GetState() != 1) {
            gLobbyScreenStep++;
            gLobbyScreenSubStep = 5;
            if (gLobbyStartPosIdx != 0 && gLobbyStartPosIdx != -1) {
                s32 k = gScene.localPlayer * 0x970;

                v = gLobbyStartPos.pos[0];
                ((PlayerVecAt *)((u8 *)D_5F8A7C + k))->v = v;
                ((PlayerVecAt *)((u8 *)D_5F9124 + k))->v = v;
            }
            p->visible = 1;
            CamReq_Cancel();
            if (Npc14_PlaceByPlayer() == 1) {
                Actor_SetState(Npc_Find(0x14), 0, 0x58);
            }
        }
        break;
    case 3:
        if (--gLobbyScreenSubStep <= 0) {
            SeqA_Start(2);
            gLobbyScreenStep = 0;
            gLobbyScreenMode = 2;
            gFieldControl = 1;
            gLobbyScreenSubStep = 0;
            gTownMenuOpen = 0;
            gTownMode = 0;
            gLobbyStartPosIdx = 0;
        }
        break;
    }
}

/* unk_7B91F0's .data (0x8195A0-0x819630) */
void LobbyScreen_EnterArea(void);
void LobbyScreen_Start(void);
void LobbyScreen_LeaveArea(void);
void LobbyScreen_CamIntro(void);
StartPos gLobbyStartPos = {
#include "lobby/008195A0.inc"
};
void (*gLobbySceneSteps[8])(void) = {
    LobbyScreen_Start, LobbyScreen_EnterArea, Town_Frame, LobbyScreen_LeaveArea,
    LobbyScreen_CamIntro, NULL, NULL, NULL,
};
void (*gLobbyScreenModes[8])(LobbyState *p) = {
    (void (*)(LobbyState *p))LobbyScreen_Start, (void (*)(LobbyState *p))LobbyScreen_EnterArea, (void (*)(LobbyState *p))Town_Frame, (void (*)(LobbyState *p))LobbyScreen_LeaveArea,
    (void (*)(LobbyState *p))LobbyScreen_CamIntro, NULL, NULL, NULL,
};
