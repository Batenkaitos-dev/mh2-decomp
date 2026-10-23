/*
 * The lobby's login (LOBBY_PHASE_ENTRY): LobbyEntry_Run runs the step of
 * LobbyState.mode, enum LobbyEntryMode (gLobbyEntrySteps), and each step
 * runs its own steps in LobbyState.step.
 *
 * LobbyEntry_Start resets the game's subsystems, registers the route
 * handlers, sends the client's details and starts the login task; the
 * task's progress (LobbyLogin_OnProgress) then picks the next mode from
 * the server's answers: the terms of use's page, the server's notice, a
 * patch to download, the character's choice (or an error and the choice
 * again), the login summary, the server's information, or a web page.
 * The last step, LobbyEntry_Fetch, fetches the lobby's data (the quest
 * record, the downloaded quests, the server's name, the hunter card, the
 * chat colours, the town's clock and data) and enters the lobby
 * (LobbyEnter_Begin).
 *
 * gNetEntryMode says how the lobby's login was reached (LOGIN_ENTRY_*): the
 * first entry, which only chooses the character and then connects again
 * (to the server chosen), a later entry, or the return after a quest.
 * The server's notice and information text are each meant for some of
 * these (LobbyLogin_NoticeApplies, LobbyLogin_InfoApplies).
 *
 * After the steps: the login task's progress (LobbyLogin_OnProgress), the
 * save's summary sent to the server part by part (LobbyLogin_SendSummary
 * and its reply), and the login menu's draw (LobbyEntry_Draw).
 */
#include "common.h"
#include "lobby_state.h"
#include "card.h"
#include "lobby_menu.h"
#include "net_mode.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_net.h"
#include "scene.h"
#include "player.h"
#include "lobby_globals.h"
#include "save.h"
#include "syssettings.h"
#include "chat_log.h"
#include "lobby_text.h"
#include "pad.h"
#include "area.h"
#include "overlay.h"
#include "quest.h"
#include "menu_task.h"
#include "texture.h"
#include "render_state.h"
#include "lobby_servers.h"
#include "stream.h"
#include "text.h"
#include "lobby_login.h"
#include "draw_helpers.h"
#include "hunter_card.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_disconnect.h"
#include "lobby_enter.h"
#include "lobby_leave_menus.h"
#include "lobby_routes.h"
#include "lobby_screen.h"
#include "lobby_steps.h"
#include "lobby_util.h"
#include "lobby_web_users.h"
#include "lobby_world.h"
#include "main.h"
#include "menu_widgets.h"
#include "net_devices.h"
#include "quest_offers.h"
#include "server_name.h"
#include "screen_fade.h"
#include "game_util.h"

#define FRIEND_CHECK_TIMERS 12      /* gFriendCheckTimers' entries */
#define TOWN_DATA_SLOTS 4           /* the town data's slots (TOWN_DATA_SLOT_SIZE each) */
#define CLIENT_VERSION_LEN 10       /* the game's version's characters sent */
#define ACCOUNT_ID_LEN 10           /* the account id's characters */
#define ACCOUNT_PASS_LEN 16         /* the password's characters */

s32 LobbyNet_GetErrText(char *);
extern LobbyMenu *gLobbyMenu;
void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
char *strcpy(void *dst, const void *src);
extern char gLobbyWebUrl[256];
extern s8 gLobbyPatchBusy;
extern u8 gNetEntryMode;        /* LOGIN_ENTRY_* */
extern s16 gFriendCheckTimers[FRIEND_CHECK_TIMERS];
char *strncpy(char *dst, const char *src, s32 n);
extern s32 gLobbyActive;
extern s32 gQuestQNetReadyArg;
u8 NetMenu_Update(void);
extern char gVersionString[];
extern char gAccountId[];
extern char gAccountData[];
extern void (*gLobbyEntrySteps[])(void);

/* Runs the login's step of LobbyState.mode (gLobbyEntrySteps); a mode out
 * of range quits the lobby (and is still called). */
void LobbyEntry_Run(void)
{
    if (gLobby->mode >= LOBBY_ENTRY_MODES) {
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
    }
    gLobbyEntrySteps[gLobby->mode]();
}

/*
 * LOBBY_ENTRY_START. Step 0 resets the game's subsystems and the lobby's
 * menu (its draw LobbyEntry_Draw), loads the lobby's textures, clears the
 * login's notice and information, resets the network's state, registers
 * the route handlers and sends the client's details (the device's state,
 * the game's version); the title and the town-entry text show. Step 2
 * starts the login task with the account's id and password (the network
 * mode's entry, the extra save file's buffer for a patch), and step 3
 * waits for its progress under the menu's timer.
 */
void LobbyEntry_Start(void)
{
    NetLoginParams login;
    NetClientInfo client;
    s16 device0;
    s16 device1;
    s16 device2;
    s16 device3;
    u8 *step;

    step = &gLobby->step;
    switch (*step) {
    case 0:
        Game_ResetSubsystems();
        LobbyMenu_Init();
        LobbyMenu_SetDraw(LobbyEntry_Draw, 0, NULL, 0);
        Lobby_LoadTextures();
        gLobbyActive = 1;
        gLobby->step++;
        gLobby->connected = 1;
        memset(&gLobby->loginNotice, 0, sizeof(NetLoginNotice));
        memset(gLobby->loginInfo, 0, sizeof(NetLoginInfo));
        LobbyNet_Reset();
        LobbyRoute_Register();
        memset(&client, 0, sizeof(client));
        Net_GetDeviceState(&device0, &device1, &device2, &device3);
        client.unk1 = 1;
        client.unk2 = 8;
        client.unk0 = 0;
        strncpy(client.version, gVersionString, CLIENT_VERSION_LEN);
        gSrvQNetMaxStall = 0;
        client.state[4] = 0;
        gSrvQNetPeerTimeout = 0;
        client.state[0] = device0;
        client.state[1] = device1;
        client.state[2] = device2;
        client.state[3] = device3;
        client.state[5] = 0;
        client.state[6] = 0;
        client.state[7] = 0;
        gQuestQNetReadyArg = 0;
        LobbyNet_SetClientInfo(&client);
        LobbyTitle_Set(LOBBY_TITLE_LOGIN);
        LobbyText_Open(LOBBY_TEXT_TO_TOWN);
        break;
    case 1:
        (*step)++;
    case 2:
        gLobby->step++;
        memset(&login, 0, sizeof(login));
        login.entry = gNetMode.entry;
        login.type = 1;
        login.patchBuf = gSysSettings.extraBuf;
        strncpy(login.id, gAccountId, ACCOUNT_ID_LEN);
        strncpy(login.pass, gAccountData, ACCOUNT_PASS_LEN);
        LobbyMenu_StartTimer();
        NetLoginTask_Start(&login, LobbyLogin_OnProgress);
        break;
    case 3:
        LobbyMenu_TickTimer();
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    gLobbyMenu->redraw = 1;
}

/* LOBBY_ENTRY_TERMS: the terms of use's page opens in the lobby's web
 * pages (LobbyWeb_Run) once the screen fades in; when they are left
 * (LINK_STATE_WEB_BACK), the screen fades out and step 3 waits under
 * the menu's timer for the login's progress. */
void LobbyEntry_Terms(void)
{
    u8 step;

    step = gLobby->step;
    switch (step) {
    case 0:
        gLobby->step = (u8) (step + 1);
        FadeA_Start(FADE_IN_BLACK);
        strcpy(gLobbyWebUrl, "file://SRC/BROWSER/DATA/N_KYK_TP.HTM");
        Lobby_MarkDisconnected();
        break;
    case 1:
        if (gLobby->linkState == LINK_STATE_WEB_BACK) {
            gLobby->step = (u8) (step + 1);
            gLobby->linkState = 0U;
            FadeA_Start(FADE_OUT_BLACK);
            break;
        }
        LobbyWeb_Run();
        break;
    case 3:
        LobbyMenu_TickTimer();
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
}

/*
 * LOBBY_ENTRY_NOTICE: the server's notice at login (LobbyState.loginNotice).
 * A notice for a network menu entry the player didn't come by
 * (LobbyLogin_NoticeApplies) is answered as not shown and skipped. Otherwise a
 * notice of kind 0 quits the lobby with its text (LOBBY_QUIT_SERVER_CLOSE);
 * any other shows with its countdown (loginNoticeSecs, counted down each
 * second), and once it reaches 0, PAD_CIRCLE answers it as shown. Step 5
 * waits for the login's progress under the menu's timer.
 */
void LobbyEntry_Notice(void)
{
    u16 pad;
    LobbyState *lobby;
    u8 *step;
    s32 *frames;
    s16 *secs;

    pad = LobbyPad_Get(0);
    lobby = gLobby;
    step = &lobby->step;
    switch (*step) {
    case 0:
        if (LobbyLogin_NoticeApplies(lobby->loginNotice) == 0) {
            gLobby->step = 5;
            LobbyMenu_StartTimer();
            LobbyNet_SendLoginNotice(0);
        } else {
            gLobby->step++;
        }
        break;
    case 1:
        (*step)++;
        gLobby->countdown = 30;     /* frames: a second */
        break;
    case 2:
        (*step)++;
        if (gLobby->loginNotice != 0) {
            gLobby->waitTimer = 8;
            LobbyDialog_OpenCountdown(gLobby->loginNoticeText, *(u16 *)&gLobby->loginNoticeSecs * 30);
        } else {
            memcpy(gLobby->errText, gLobby->loginNoticeText, sizeof(gLobby->errText));
            Lobby_Disconnect(LOBBY_QUIT_SERVER_CLOSE);
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        if (*(u16 *)&gLobby->loginNoticeSecs != 0) {
            gLobby->countdown--;
            frames = &gLobby->countdown;
            if (*(s8 *)frames <= 0) {
                *frames = 30;
                (*(u16 *)&gLobby->loginNoticeSecs)--;
                secs = &gLobby->loginNoticeSecs;
                if (*secs < 0) {
                    *secs = 0;
                }
            }
        } else if (pad & PAD_CIRCLE) {
            gLobby->step++;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            LobbyText_Open(LOBBY_TEXT_TO_TOWN);
        }
        break;
    case 4:
        (*step)++;
        LobbyMenu_StartTimer();
        LobbyNet_SendLoginNotice(1);
        break;
    case 5:
        LobbyMenu_TickTimer();
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
}

/*
 * LOBBY_ENTRY_PATCH: the patch the server announced (LobbyState.patchInfo)
 * is downloaded (LobbyPatch_Install, the network mode's screen drawn
 * meanwhile, gLobbyPatchBusy set), with the lobby's textures released and
 * its data overlay loaded again after it. When it is done, the patch's end
 * is sent and step 5 waits under the menu's timer; when it failed, the
 * lobby is quit (LOBBY_QUIT_TO_GMSUB).
 */
void LobbyEntry_Patch(void)
{
    LobbyState *lobby;
    u8 *step;
    s32 result;

    lobby = gLobby;
    step = &lobby->step;
    switch (*step) {
    case 0:
        (*step)++;
        gLobby->connected = 0;
        LobbyNet_GetPatchInfo(&gLobby->patchInfo);
        LobbyPatch_ResetInstall();
        Game_ResetSubsystems();
        gLobbyPatchBusy = 1;
        break;
    case 1:
        (*step)++;
        gLobbyPatchBusy = 1;
        break;
    case 2:
        gLobbyPatchBusy = 1;
        result = LobbyPatch_Install();
        if (result == 1) {
            gLobby->step++;
            Lobby_ReleaseNetTextures();
            LoadOverlay(OVERLAY_LBGUILD);
        } else if (result == -1) {
            gLobby->step = 6;
            Lobby_ReleaseNetTextures();
            LoadOverlay(OVERLAY_LBGUILD);
        } else {
            NetMode_CallDrawScreen(1);
        }
        break;
    case 3:
        (*step)++;
        Lobby_LoadTextures();
        gLobbyPatchBusy = 0;
        break;
    case 4:
        (*step)++;
        gLobby->connected = 1;
        LobbyMenu_StartTimer();
        LobbyNet_SendPatchEnd();
        break;
    case 5:
        LobbyMenu_TickTimer();
        break;
    case 6:
        (*step)++;
        Lobby_LoadTextures();
        gLobbyPatchBusy = 0;
        break;
    case 7:
        lobby->connected = 1;
        Lobby_Disconnect(LOBBY_QUIT_TO_GMSUB);
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
}

/*
 * LOBBY_ENTRY_ID_SELECT: the character's choice. Step 0 fetches the
 * account's characters (their ids, names and card texts) into
 * LobbyState.charaIds, charaNames and charaTexts, step 1 their text
 * (LobbyStep_CharaSelectText), and the id menu opens with the prompt and
 * cursor kept (idMenuPrompt, idCursor). On the first entry the group join
 * is dropped and the screen fades in. Step 3 runs the menu (IdMenu_Run: -1
 * while it runs, -2 when the player quits, 0 once it closes); step 5 takes
 * the chosen character: the players are cleared, the player loaded from
 * the save, its name and checked card sent as the player's own
 * (LobbyNet_SetMe), and its id and name kept. Step 6 waits for the login's
 * progress under the menu's timer, with the loading mark.
 */
void LobbyEntry_IdSelect(void)
{
    s32 i;
    s32 count;
    s32 nameOffset;
    s32 chosen;
    s32 idOffset;

    switch (gLobby->step) {
    case 0:
        gLobbyMenu->redraw = 1;
        gLobby->idMenuPrompt = 0;
        gLobby->idCursor = 0;
        memset(gLobby->charaIds, 0, sizeof(gLobby->charaIds));
        memset(gLobby->charaNames, 0, sizeof(gLobby->charaNames));
        memset(gLobby->charaTexts, 0, sizeof(gLobby->charaTexts));
        count = (u8)LobbyNet_GetCharaCount();
        for (i = 0; i < count; i++) {
            LobbyNet_GetCharaId(i, gLobby->charaIds[i]);
            LobbyNet_GetCharaName(i, (char *) (&gLobby->charaNames[i * (LOBBY_USER_NAME_LEN + 1)]));
            LobbyNet_GetCharaText(i, &gLobby->charaTexts[i * 256]);
        }
        gLobby->step++;
        gLobby->subStep = 0;
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_CharaSelectText()) {
        case LOBBY_STEP_DONE:
        case LOBBY_STEP_FAILED:
            gLobby->step++;
            LobbyMenu_Init();
            gLobbyMenu->promptStep = gLobby->idMenuPrompt;
            gLobbyMenu->cursor = gLobby->idCursor;
            break;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            gLobby->step++;
            if (gNetEntryMode == LOGIN_ENTRY_FIRST) {
                memset(gLobby, 0, LOBBY_GROUP_JOIN_SIZE);
                gElderForecastHeard = 0;
                FadeA_Start(FADE_IN_BLACK);
                if ((u8)FadeB_GetState() != FADE_STATE_IDLE) {
                    FadeB_Start(FADE_IN_BLACK);
                }
            }
        }
        break;
    case 3:
        switch (IdMenu_Run()) {
        case 0:
            gMemberNamesOffSaved = 0;
            gOnlineMark = 0;
            gLobby->step++;
            break;
        case -2:
            Lobby_Disconnect(LOBBY_QUIT_TO_GMSUB);
            break;
        case -1:
            break;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            gLobby->step++;
        }
    case 5:
        gLobby->step++;
        gLobbyMenu->redraw = 1;
        chosen = gLobbyMenu->cursor;
        gLobby->idCursor = chosen;
        LobbyMenu_StartTimer();
        for (i = 0; i < LOBBY_MEMBER_COUNT; i++) {
            memset(&gPlayers[i], 0, sizeof(Player));
        }
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        nameOffset = chosen * (LOBBY_USER_NAME_LEN + 1);
        memcpy(&gLobby->charaNames[nameOffset], gSave.name, LOBBY_USER_NAME_LEN);
        idOffset = chosen * LOBBY_USER_ID_LEN;
        if (gLobby->charaIds[0][idOffset] == 0) {
            HunterCard_Sanitize(&gLobby->charaTexts[chosen * 256]);
            LobbyNet_SetMe((char *) NULL, (char *) &gLobby->charaNames[nameOffset], &gLobby->charaTexts[chosen * 256]);
        } else {
            HunterCard_Sanitize(&gLobby->charaTexts[chosen * 256]);
            LobbyNet_SetMe((char *) &gLobby->charaIds[0][idOffset], (char *) &gLobby->charaNames[nameOffset], &gLobby->charaTexts[chosen * 256]);
        }
        LobbyNet_GetMyCharaId((char *) gLobby->myId);
        LobbyNet_GetMyCharaName((char *) gLobby->myName);
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        if (LobbyMenu_TickTimer() == 0) {
            Loading_Request();
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
}

/* LOBBY_ENTRY_ERROR: the server's error text (LobbyState.errText) shows
 * for 20 seconds, or until PAD_CIRCLE after the first 4; the screen then
 * fades out and the character's choice comes back (its step 1). */
void LobbyEntry_Error(void)
{
    u16 pad;

    pad = LobbyPad_Get(0);
    switch (gLobby->step) {
    case 0:
        LobbyMenu_Init();
        LobbyMenu_SetDraw(LobbyEntry_Draw, 0, NULL, 0);
        gLobby->step++;
        FadeA_Start(FADE_IN_BLACK);
        gLobby->countdown = 600;    /* frames: 20 s */
        LobbyDialog_OpenWait(gLobby->errText);
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        gLobby->countdown--;
        if (gLobby->countdown > 0) {
            if (gLobby->countdown < 480 && (pad & PAD_CIRCLE)) {
                gLobby->step++;
                LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
                FadeA_Start(FADE_OUT_BLACK);
            }
        } else {
            gLobby->step++;
            FadeA_Start(FADE_OUT_BLACK);
        }
        break;
    case 2:
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            gLobbyMenu->hideLayers = 1;
            gLobby->mode = LOBBY_ENTRY_ID_SELECT;
            gLobby->step = 1;
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
}

/*
 * LOBBY_ENTRY_SUMMARY: the login summary is sent (LobbyLogin_SendSummary),
 * with the loading mark while it is busy. Then, on the first entry, the
 * hunter card is built from the save; the player starts in AREA_ONLINE_VILLAGE
 * (AREA_TAVERN after a quest), the downloaded
 * quests' records are forgotten, the friend list's checks reset, and the
 * card (its online mark, no quest status) is sent (LobbyStep_HunterCard).
 * On the first entry the lobby is then left to connect again
 * (LOBBY_QUIT_RECONNECT, LOGIN_ENTRY_SERVER); otherwise the server's
 * information follows.
 */
void LobbyEntry_Summary(void)
{
    s32 local;
    s32 i;
    s16 area;
    HunterCard *card;

    card = &gPlayerText.card;
    switch (gLobby->step) {
    case 0:
        switch (LobbyLogin_SendSummary()) {
        case LOBBY_STEP_DONE:
            gLobby->step++;
            break;
        case LOBBY_STEP_FAILED:
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
            break;
        default:
            Loading_Request();
            break;
        }
        break;
    case 1:
        if (gNetEntryMode == LOGIN_ENTRY_FIRST) {
            HunterCard_Build(card);
            gPlayerText.card.arenaRole = 0;
            HunterCard_Sanitize(card);
        }
        area = (gNetEntryMode != LOGIN_ENTRY_QUEST_RETURN) ? AREA_ONLINE_VILLAGE : AREA_TAVERN;
        local = gScene.localPlayer;
        gScene.area = gPlayers[local].nextArea = gPlayers[local].area = area;
        memset(QDL_SAVED, 0, QDL_FILE_COUNT * sizeof(NetDlFile));
        card->onlineMark = gOnlineMark;
        card->questStatus = 0;
        for (i = 0; i < FRIEND_CHECK_TIMERS; i++) {
            gFriendCheckTimers[i] = 0;
        }

        gLobby->step++;
        gLobby->seqStep = 0;
        break;
    case 2:
        switch (LobbyStep_HunterCard()) {
        case LOBBY_STEP_DONE:
            gLobby->step++;
            break;
        case LOBBY_STEP_FAILED:
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
            break;
        }
        break;
    case 3:
        ServerList_ClearTried();
        if (gNetEntryMode == LOGIN_ENTRY_FIRST) {
            gNetEntryMode = LOGIN_ENTRY_SERVER;
            Lobby_Disconnect(LOBBY_QUIT_RECONNECT);
        } else {
            gLobby->mode = LOBBY_ENTRY_INFO;
            gLobby->step = 0;
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    gLobbyMenu->redraw = 1;
}

/*
 * LOBBY_ENTRY_INFO: the server's information text (LobbyStep_LoginInfo),
 * when there is one for the network menu entry the player came by
 * (LobbyLogin_InfoApplies), shows until PAD_CIRCLE or a minute
 * (waitTimer); then the area information's text opens and the lobby's data
 * is fetched (LOBBY_ENTRY_FETCH).
 */
void LobbyEntry_Info(void)
{
    LobbyState *lobby;
    u8 *step;
    s32 *delay;
    u16 pad;

    pad = LobbyPad_Get(0);
    lobby = gLobby;
    step = &lobby->step;
    switch (*step) {
    case 0:
        (*step)++;
        gLobby->seqStep = 0;
        LobbyMenu_SetDraw(LobbyEntry_Draw, 0, NULL, 0);
        LobbyTitle_Set(LOBBY_TITLE_INFO);
        break;
    case 1:
        switch (LobbyStep_LoginInfo()) {
        case LOBBY_STEP_DONE:
            gLobby->step++;
            break;
        case LOBBY_STEP_FAILED:
            gLobby->step = 7;
            break;
        }
        break;
    case 2:
        (*step)++;
    case 3:
        if (LobbyLogin_InfoApplies(gLobby->loginInfo[0])) {
            gLobby->step++;
        } else {
            gLobby->step = 7;
        }
        break;
    case 4:
        (*step)++;
        if ((u8)FadeA_GetState() != FADE_STATE_IDLE) {
            FadeA_Start(FADE_IN_BLACK);
        }
        LobbyDialog_OpenWait(gLobby->loginInfoText);
        gLobby->countdown = 30;     /* frames: a second before input counts */
        gLobby->waitTimer = 1800;   /* frames: a minute */
        break;
    case 5:
        delay = &lobby->countdown;
        if (*delay != 0) {
            (*delay)--;
        } else if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            if ((pad & PAD_CIRCLE) || --gLobby->waitTimer <= 0) {
                gLobby->step++;
                LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
                LobbyText_Open(LOBBY_TEXT_AREA_LIST_FETCH);
            }
        }
        break;
    case 6:
        if ((u8)FadeA_GetState() == FADE_STATE_RUNNING) {
            break;
        }
    case 7:
        gLobby->mode = LOBBY_ENTRY_FETCH;
        gLobby->step = 0;
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    gLobbyMenu->redraw = 1;
}

/*
 * LOBBY_ENTRY_FETCH: after a quest, the game's save
 * menu runs first (NETMENU_MODE_GAME_SAVE_ENTRY, after 3 frames); then the
 * login's end is sent and the lobby's data fetched in turn: the quest
 * record, the downloaded quests, the server's name, the hunter card, the
 * chat colour mode, the town's clock, and the town's data into the four
 * slots of gTownQuestLists' buffer (a failure shows the error text, and
 * PAD_CIRCLE then quits). The town's quest lists are rebuilt unless the
 * last quest was aborted in the same season and half of the day. Once the
 * screen has faded, the lobby is entered (LobbyEnter_Begin; back to the
 * area left after a quest, unless by a group's join),
 * and gNetEntryMode becomes LOGIN_ENTRY_AGAIN.
 */
void LobbyEntry_Fetch(void)
{
    LobbyState *lobby;
    u8 *step;
    s32 *delay;
    s32 i;
    char *buf;

    lobby = gLobby;
    step = &lobby->step;
    switch (*step) {
    case 0:
        if (gNetEntryMode == LOGIN_ENTRY_QUEST_RETURN) {
            FileQueue_Flush();
            gLobby->step++;
            NetMenu_Open(NETMENU_MODE_GAME_SAVE_ENTRY);
            gLobby->countdown = 3;     /* frames */
            Stream_Reset();
            gLobby->connected = 0;
            gLobby->dlQuestsReady = 0;
        } else {
            lobby->dlQuestsReady = 0;
            LobbyNet_SendLoginReady();
            gLobby->step = 3;
            gLobby->subStep = 0;
            gLobby->connected = 1;
        }
        LobbyText_Open(LOBBY_TEXT_AREA_LIST_FETCH);
        if ((u8)FadeA_GetState() != FADE_STATE_IDLE) {
            FadeA_Start(FADE_IN_BLACK);
        }
        break;
    case 1:
        delay = &lobby->countdown;
        if (*delay == 0) {
            if ((u8)NetMenu_Update() != 0) {
                gLobby->step++;
                LobbyNet_SendLoginReady();
                LobbyText_Open(LOBBY_TEXT_AREA_LIST_FETCH);
                gLobbyMenu->redraw = 0;
                gLobby->connected = 1;
                return;
            }
        } else {
            (*delay)--;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_QuestRecord()) {
        case LOBBY_STEP_FAILED:
        case LOBBY_STEP_DONE:
            gLobby->step++;
            return;
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_DownloadQuests()) {
        case LOBBY_STEP_FAILED:
        case LOBBY_STEP_DONE:
            gLobby->step++;
            return;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_ServerName()) {
        case LOBBY_STEP_DONE:
            gLobby->step++;
            return;
        }
        break;
    case 5:
        (*step)++;
        gLobby->seqStep = 0;
    case 6:
        switch (LobbyStep_HunterCard()) {
        case LOBBY_STEP_DONE:
        case LOBBY_STEP_FAILED:
            gLobby->step++;
            gLobby->seqStep = 0;
            break;
        }
        break;
    case 7:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_ChatColorMode()) {
        case LOBBY_STEP_FAILED:
        case LOBBY_STEP_DONE:
            gLobby->step++;
            break;
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_TownClock()) {
        case LOBBY_STEP_FAILED:
        case LOBBY_STEP_DONE:
            gLobby->step++;
            LobbyScreen_InitLists();
            for (i = 0, buf = (char *)&gTownQuestLists; i < TOWN_DATA_SLOTS; i++, buf += TOWN_DATA_SLOT_SIZE) {
                LobbyNet_SetSlotBuf(i, buf, TOWN_DATA_SLOT_SIZE);
            }
            break;
        }
        break;
    case 9:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_TownData(0, TOWN_DATA_SLOTS - 1)) {
        case LOBBY_STEP_FAILED:
            LobbyDialog_OpenWait(gLobby->errText);
            gLobby->step = 11;
            break;
        case LOBBY_STEP_DONE:
            gLobby->step++;
            if (gQuestRecordPhase != QUEST_PHASE_ABORTED || gLobbyLastSeason != LobbyWorld_GetSeason() || gLobbyLastPm != LobbyTime_IsPm()) {
                QuestList_Rebuild();
            }
            break;
        }
        break;
    case 10:
        gLobbyMenu->redraw = 1;
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            if (gNetEntryMode == LOGIN_ENTRY_QUEST_RETURN && gLobby->groupJoin == 0) {
                gLobby->returnToArea = 1;
            } else {
                gLobby->returnToArea = 0;
            }
            LobbyEnter_Begin();
            gNetEntryMode = LOGIN_ENTRY_AGAIN;
        }
        break;
    case 11:
        gLobbyMenu->redraw = 1;
        if (LobbyPad_Get(0) & PAD_CIRCLE) {
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    if (gLobby->step >= 2) {
        gLobbyMenu->redraw = 1;
    }
}

/* LOBBY_ENTRY_WEB_PAGE: the web page the server named (gNetLoginWebPage)
 * opens in the lobby's web pages (LobbyWeb_Run), after its text shows for
 * its seconds when it has one. */
void LobbyEntry_WebPage(void)
{
    NetLoginWebPage page;
    u8 *step;
    u16 *secs;

    step = &gLobby->step;
    switch (*step) {
    case 0:
        LobbyNet_GetLoginWebPage(&page);
        memcpy(gLobbyWebUrl, page.url, sizeof(page.url));
        secs = &page.secs;
        if (*secs != 0) {
            gLobby->step++;
            gLobby->countdown = *secs * 30;
            memcpy(gLobby->errText, page.text, sizeof(gLobby->errText));
            LobbyDialog_OpenCountdown(gLobby->errText, gLobby->countdown);
            gLobbyMenu->redraw = 1;
        } else {
            gLobby->step = 2;
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        if (--gLobby->countdown <= 0) {
            gLobby->step++;
        }
        break;
    case 2:
        (*step)++;
        Lobby_MarkDisconnected();
        break;
    case 3:
        LobbyWeb_Run();
        break;
    }
}

extern char gMyName[];               /* the character's name (16 bytes) */
extern char gMyUserId[];             /* the character's id (8) */

/* 1 while the login shows a web page the server named
 * (LOBBY_ENTRY_WEB_PAGE, in LOBBY_PHASE_ENTRY). */
s32 LobbyEntry_AtWebPage(void)
{
    if ((gLobby->mode == LOBBY_ENTRY_WEB_PAGE) && (gLobby->phase == LOBBY_PHASE_ENTRY)) {
        return 1;
    }
    return 0;
}

/*
 * The login task's progress (NetLoginTask's onDone; msg.flag, enum
 * NetProgress, says how far it got), which picks the login's next mode:
 * - NET_PROGRESS_WEB_PAGE: the web page the server names;
 * - NET_PROGRESS_NOTICE: the server's notice, copied into the lobby's
 *   state;
 * - NET_PROGRESS_CHARAS: the character's choice: on the first entry the
 *   choice menu after a fade; on a later one the character already chosen
 *   (the lobby's, or after a quest the save's id and name) is sent with
 *   its hunter card, and the choice waits for the answer;
 * - NET_PROGRESS_CHARA_CHOSEN: the chosen character's id and name are
 *   kept, in the lobby's state and in the save's copies;
 * - NET_PROGRESS_PATCH_OK: the patch step; NET_PROGRESS_LOGIN_DONE: the
 *   login summary; flag 5 (no sender seen): the terms of use's page.
 * A failure (kind NET_MSG_FAILED): NET_PROGRESS_CHARA_FAILED shows the
 * server's error after a fade, and flag 8 (no sender seen) the same
 * without the fade; each sets the choice menu's prompt. A bad patch
 * (NET_PROGRESS_PATCH_BAD) quits the lobby.
 */
void LobbyLogin_OnProgress(NetMsg msg, NetMsg *msgRef)
{
    if (msg.kind != NET_MSG_FAILED) {
        switch (msg.flag) {
        case NET_PROGRESS_WEB_PAGE:
            gLobby->mode = LOBBY_ENTRY_WEB_PAGE;
            gLobby->step = 0;
            break;
        case NET_PROGRESS_NOTICE:
            gLobby->mode = LOBBY_ENTRY_NOTICE;
            gLobby->step = 0;
            LobbyNet_GetLoginNotice((NetLoginNotice *) &gLobby->loginNotice);
            break;
        case NET_PROGRESS_CHARAS:
            switch (gNetEntryMode) {
            case LOGIN_ENTRY_FIRST:
                gLobby->mode = LOBBY_ENTRY_ID_SELECT;
                gLobby->step = 0;
                gLobby->subStep = 0;
                FadeA_Start(1);
                break;
            case LOGIN_ENTRY_SERVER:
            case LOGIN_ENTRY_AGAIN:
                gLobby->mode = LOBBY_ENTRY_ID_SELECT;
                gLobby->step = 6;
                gLobby->subStep = 0;
                LobbyMenu_StartTimer();
                HunterCard_Sanitize(gPlayerText.raw);
                LobbyNet_SetMe((char *) gLobby->myId, (char *) gLobby->myName, gPlayerText.raw);
                break;
            case LOGIN_ENTRY_QUEST_RETURN:
                gLobby->mode = LOBBY_ENTRY_ID_SELECT;
                gLobby->step = 6;
                gLobby->subStep = 0;
                LobbyMenu_StartTimer();
                HunterCard_Sanitize(gPlayerText.raw);
                LobbyNet_SetMe(gMyUserId, gMyName, gPlayerText.raw);
                break;
            }
            break;
        case NET_PROGRESS_CHARA_CHOSEN:
            memset(gLobby->myId, 0, sizeof(gLobby->myId));
            LobbyNet_GetMyCharaId((char *) gLobby->myId);
            memset(gMyUserId, 0, sizeof(gLobby->myId));
            memcpy(gMyUserId, gLobby->myId, sizeof(gLobby->myId));
            memset(gLobby->myName, 0, sizeof(gLobby->myName));
            LobbyNet_GetMyCharaName((char *) gLobby->myName);
            memcpy(gMyName, gLobby->myName, sizeof(gLobby->myName));
            LobbyMenu_StartTimer();
            gLobby->step = 6;
            break;
        case 5:
            gLobby->mode = LOBBY_ENTRY_TERMS;
            gLobby->step = 0;
            break;
        case NET_PROGRESS_PATCH_OK:
            gLobby->mode = LOBBY_ENTRY_PATCH;
            gLobby->step = 0;
            break;
        case NET_PROGRESS_LOGIN_DONE:
            gLobby->mode = LOBBY_ENTRY_SUMMARY;
            gLobby->step = 0;
            break;
        }
    } else {
        switch (msg.flag) {
        case NET_PROGRESS_CHARA_FAILED:
            gLobby->mode = LOBBY_ENTRY_ERROR;
            gLobby->step = 0;
            gLobby->idMenuPrompt = 1;
            FadeA_Start(1);
            LobbyNet_GetErrText(gLobby->errText);
            break;
        case 8:
            gLobby->mode = LOBBY_ENTRY_ERROR;
            gLobby->step = 0;
            gLobby->idMenuPrompt = 2;
            LobbyNet_GetErrText(gLobby->errText);
            break;
        case NET_PROGRESS_PATCH_BAD:
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
            break;
        }
    }
}

/* 1 if the server's login notice is meant for the way the lobby was
 * reached (gNetEntryMode). The notice's target (its first byte, 0-4):
 * 0 every entry; 1 the first only; 2 the first or the return after a
 * quest; 3 those and a later entry; 4 those and the second connection. */
s32 LobbyLogin_NoticeApplies(u8 target)
{
    switch (target) {
    case 0:
        return 1;
    case 1:
        if (gNetEntryMode == LOGIN_ENTRY_FIRST) {
            return 1;
        }
        break;
    case 2:
        if (gNetEntryMode == LOGIN_ENTRY_FIRST) {
            return 1;
        }
        if (gNetEntryMode == LOGIN_ENTRY_QUEST_RETURN) {
            return 1;
        }
        break;
    case 3:
        if (gNetEntryMode == LOGIN_ENTRY_FIRST) {
            return 1;
        }
        if (gNetEntryMode == LOGIN_ENTRY_QUEST_RETURN) {
            return 1;
        }
        if (gNetEntryMode == LOGIN_ENTRY_AGAIN) {
            return 1;
        }
        break;
    case 4:
        if (gNetEntryMode == LOGIN_ENTRY_FIRST) {
            return 1;
        }
        if (gNetEntryMode == LOGIN_ENTRY_QUEST_RETURN) {
            return 1;
        }
        if (gNetEntryMode == LOGIN_ENTRY_AGAIN) {
            return 1;
        }
        if (gNetEntryMode == LOGIN_ENTRY_SERVER) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The same for the server's information text (its first byte, 0-3):
 * 0 never; 1 the second connection; 2 that or the return after a quest;
 * 3 those and a later entry. */
s32 LobbyLogin_InfoApplies(u8 target)
{
    switch (target) {
    case 0:
        return 0;
    case 1:
        if (gNetEntryMode == LOGIN_ENTRY_SERVER) {
            return 1;
        }
        break;
    case 2:
        if (gNetEntryMode == LOGIN_ENTRY_SERVER) {
            return 1;
        }
        if (gNetEntryMode == LOGIN_ENTRY_QUEST_RETURN) {
            return 1;
        }
        break;
    case 3:
        if (gNetEntryMode == LOGIN_ENTRY_SERVER) {
            return 1;
        }
        if (gNetEntryMode == LOGIN_ENTRY_QUEST_RETURN) {
            return 1;
        }
        if (gNetEntryMode == LOGIN_ENTRY_AGAIN) {
            return 1;
        }
        break;
    }
    return 0;
}

#define SUMMARY_PART_DONE 0         /* the server asks for no more */
#define SUMMARY_PART_BOX_LAST 20    /* parts 1-20: the box's pages 0-19 */
#define SUMMARY_PART_EQUIP 21       /* the equipment worn */
#define SUMMARY_MODE_EQUIP 1        /* Save_BuildSummary's mode for the equipment */
#define SUMMARY_MODE_BOX 2          /* and for a page of the box */

/*
 * Sends the save's summary to the server at login (LobbyState.subStep),
 * the part the server asks for each time (subStep2): the summary's start
 * first (LobbyReq_SaveSummaryBegin, with gNetEntryMode), then each part
 * asked for (LobbyReq_SaveSummaryPart): SUMMARY_PART_EQUIP, the equipment
 * worn, or a page of the box, until the server asks for
 * SUMMARY_PART_DONE. Returns 2 while it runs, 0 when it is done, 1 when
 * it failed.
 */
s32 LobbyLogin_SendSummary(void)
{
    s32 page;
    void *data;
    s32 mode;
    s32 size;

    switch (gLobby->subStep) {
    case 0:
        gLobby->subStep++;
        gLobby->awaitReply = LOBBY_AWAIT_SUMMARY_BEGIN;
        LobbyMenu_StartTimer();
        LobbyReq_SaveSummaryBegin(gNetEntryMode, LobbyLogin_OnSummaryReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        switch (gLobby->subStep2) {
        case SUMMARY_PART_DONE:
            gLobby->subStep = 0;
            gLobby->subStep2 = 0;
            return 0;
        case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: case 10:
        case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 18: case 19: case SUMMARY_PART_BOX_LAST:
            page = gLobby->subStep2 - 1;
            mode = SUMMARY_MODE_BOX;
            break;
        case SUMMARY_PART_EQUIP:
            mode = SUMMARY_MODE_EQUIP;
            page = 0;
            break;
        default:
            gLobby->subStep = 0;
            gLobby->subStep2 = 0;
            return 1;
        }
        size = Save_SummarySize(mode);
        data = Save_BuildSummary(mode, page);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_SUMMARY_PART;
        LobbyReq_SaveSummaryPart(gNetEntryMode, gLobby->subStep2, (s32)data, size, LobbyLogin_OnSummaryReply);
        gLobby->subStep = 1;
        break;
    case LOBBY_SEQ_FAILED:
        gLobby->subStep = 0;
        gLobby->subStep2 = 0;
        return 1;
    default:
        gLobby->subStep = 0;
        gLobby->subStep2 = 0;
        return 1;
    }
    return 2;
}

/* The reply to the summary's start or one of its parts: the part the
 * server asks for next (subStep2, LobbyNet_GetSaveSummaryPart), or
 * LOBBY_SEQ_FAILED. Ignored while the lobby is being left. */
void LobbyLogin_OnSummaryReply(NetMsg msg, NetMsg *msgRef)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;
    u8 part;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_SUMMARY_BEGIN || *awaited == LOBBY_AWAIT_SUMMARY_PART) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                LobbyNet_GetSaveSummaryPart(&part);
                gLobby->subStep++;
                gLobby->subStep2 = part;
            } else {
                gLobby->subStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/* The login's menu draw (LobbyMenu_SetDraw): before the summary on the
 * first entry the preset icons, else the login's sprites; then the
 * icons' texture. The title and help line would draw in the character's
 * choice and the patch, but the game's test asks for both modes at once,
 * so they never do. */
void LobbyEntry_Draw(MenuTask *task)
{
    u8 mode;

    Text_SetSize(task->textSize);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    if (((s32) gLobby->mode < LOBBY_ENTRY_SUMMARY) && (gNetEntryMode == LOGIN_ENTRY_FIRST)) {
        Icon_DrawPreset(2);
    } else {
        LobbyLogin_DrawFrameBase();
    }
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    mode = gLobby->mode;
    if ((mode == LOBBY_ENTRY_ID_SELECT) && (mode == LOBBY_ENTRY_PATCH)) {
        LobbyTitle_Draw();
        LobbyHelp_Draw();
    }
}

/* Marks the lobby server's connection as lost: LINK_STATE_DOWN, not
 * connected, the web pages' step back to 0. */
void Lobby_MarkDisconnected(void)
{
    gLobby->webStep = 0;
    gLobby->linkState = LINK_STATE_DOWN;
    gLobby->connected = 0;
}

/* The login's steps, by LobbyState.mode (enum LobbyEntryMode). */
void (*gLobbyEntrySteps[LOBBY_ENTRY_MODES])(void) = {
    LobbyEntry_Start, LobbyEntry_Terms, LobbyEntry_Notice, LobbyEntry_Patch,
    LobbyEntry_IdSelect, LobbyEntry_Error, LobbyEntry_Summary, LobbyEntry_Info,
    LobbyEntry_Fetch, LobbyEntry_WebPage,
};
