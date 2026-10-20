/*
 * The lobby's re-entry (LOBBY_PHASE_ENTER): coming back to the lobby
 * server's town, after a quest or when a group was joined. LobbyEnter_Begin
 * opens it with a message saying where the player goes back to;
 * LobbyEnter_Run asks the server which area and level-1 entry the player
 * was in (LobbyStep_CurrentPlace), fetches the areas, the level-1 list,
 * its users and the quest files again, then enters the town with its
 * members (LOBBY_ENTER_TOWN_MEMBERS). Without a place to go back to, or
 * for a group's join, the area list opens instead (AreaList_Begin).
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "area.h"

void LobbyText_Open(s32);
void Lobby_ClearRequests(void);
extern LobbyMenu *gLobbyMenu;

/*
 * Opens the re-entry at its first step, with the requests dropped. Going
 * back to the area left for the quest (LobbyState.returnToArea), the
 * message names that area's place (one of three halls, or the town);
 * otherwise it is the area list's fetching message.
 */
void LobbyEnter_Begin(void)
{
    u16 returnArea;

    gLobby->phase = LOBBY_PHASE_ENTER;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 0;
    gLobby->subStep = 0;
    Lobby_ClearRequests();
    if (gLobby->returnToArea == 1) {
        returnArea = gQuestReturnArea;
        switch (returnArea) {
        default:
            LobbyText_Open(LOBBY_TEXT_RETURN_TOWN);
            break;
        case AREA_TAVERN:
            LobbyText_Open(LOBBY_TEXT_RETURN_TAVERN);
            break;
        case AREA_TOWN_ARENA:
            LobbyText_Open(LOBBY_TEXT_RETURN_TOWN_ARENA);
            break;
        case AREA_ELDER_HALL:
            LobbyText_Open(LOBBY_TEXT_RETURN_ELDER_HALL);
            break;
        }
    } else {
        LobbyText_Open(LOBBY_TEXT_AREA_LIST_FETCH);
    }
    gLobbyMenu->redraw = 1;
}

s32 LobbyNet_GetErrText(char *);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
s32 LobbyNet_GetCurrentPlace(struct NetCurrentPlace *dst);
extern u8 gLobbyLevel0Sel;
extern u8 gLobbyLevel1Sel;
s32 LobbyReq_CurrentPlace(void (*onReply)(NetMsg, NetMsg *));
void LobbyStep_CurrentPlaceDone(NetMsg msg, NetMsg *msgPtr);

s32 LobbyStep_CurrentPlace(void);
void Player_LoadFromSave(Player *pl);
void AreaList_Begin(void);
s32 LobbyStep_Level0List(void);
void Lobby_Disconnect(s32);
s32 LobbyStep_Level1List(s32 a);
s32 LobbyStep_Level1Users(void);
s32 LobbyStep_DownloadQuests(void);
void Town_Reset(void);
s32 LobbyScreen_EnterSeq(s32 mode);
void LobbyTown_BeginRunning(void);
extern NetUser gNetUsers[10];
extern u8 gLobbyScreenMode;
void *memset(void *dst, s32 c, u32 n);
s32 HunterCard_SendChanged(void);

/*
 * Runs the re-entry each frame, by enum LobbyEnterStep:
 * START, PLACE. the place the server kept for the player: with a group's join the
 *    player's data is reloaded from the save and the hunter card sent;
 *    with an area and a level-1 entry the same, going back to the area
 *    (returnToArea 1); otherwise, or when the request fails, the area
 *    list opens;
 * AREAS. the areas fetched again (a group's join goes on in the area list);
 * LEVEL1_LIST. the level-1 list, with the members, users and loaded models cleared;
 * LEVEL1_USERS. the level-1 entry's users;
 * QUESTS. the quest files (done or not), then the town is reset;
 * TOWN. the town's entry with its members (LobbyScreen_EnterSeq, menu layers
 *    hidden); once it is done the screen is the town's.
 * A failed fetch leaves the lobby (LOBBY_QUIT_FAILED).
 */
void LobbyEnter_Run(void)
{
    u8 *seq;
    LobbyState *lobby;

    seq = &gLobby->phaseStep;
    switch (*seq) {
    case LOBBY_ENTER_STEP_START:
        (*seq)++;
        gLobby->seqStep = 0;
    case LOBBY_ENTER_STEP_PLACE:
        switch (LobbyStep_CurrentPlace()) {
        case LOBBY_STEP_DONE:
            lobby = gLobby;
            if (lobby->groupJoin != 0) {
                lobby->phaseStep++;
                gLobby->seqStep = 0;
                gLobby->returnToArea = 0;
                Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
                HunterCard_SendChanged();
            } else if (gLobbyLevel0Sel == 0 || gLobbyLevel1Sel == 0) {
                AreaList_Begin();
                gLobby->returnToArea = 0;
            } else {
                lobby->phaseStep++;
                gLobby->seqStep = 0;
                gLobby->returnToArea = 1;
                Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
                HunterCard_SendChanged();
            }
            break;
        case LOBBY_STEP_FAILED:
            AreaList_Begin();
            gLobby->returnToArea = 0;
            break;
        }
        break;
    case LOBBY_ENTER_STEP_AREAS:
        switch (LobbyStep_Level0List()) {
        case LOBBY_STEP_DONE:
            lobby = gLobby;
            if (lobby->groupJoin != 0) {
                AreaList_Begin();
            } else {
                lobby->phaseStep++;
                gLobby->seqStep = 0;
            }
            break;
        case LOBBY_STEP_FAILED:
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
            break;
        }
        break;
    case LOBBY_ENTER_STEP_LEVEL1_LIST:
        switch (LobbyStep_Level1List(0)) {
        case LOBBY_STEP_DONE:
            gLobby->phaseStep++;
            memset(gLobby->members, 0, sizeof(gLobby->members));
            memset(gNetUsers, 0, sizeof(gNetUsers));
            memset(gLobby->pcLoaded, 0, sizeof(gLobby->pcLoaded));
            break;
        case LOBBY_STEP_FAILED:
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
            break;
        }
        break;
    case LOBBY_ENTER_STEP_LEVEL1_USERS:
        switch (LobbyStep_Level1Users()) {
        case LOBBY_STEP_DONE:
            gLobby->phaseStep++;
            break;
        case LOBBY_STEP_FAILED:
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
            break;
        }
        break;
    case LOBBY_ENTER_STEP_QUESTS:
        switch (LobbyStep_DownloadQuests()) {
        case LOBBY_STEP_FAILED:
        case LOBBY_STEP_DONE:
            gLobby->phaseStep++;
            gLobby->seqStep = 0;
            gLobbyMenu->redraw = 1;
            Town_Reset();
            return;
        }
        break;
    case LOBBY_ENTER_STEP_TOWN:
        gLobbyMenu->hideLayers = 1;
        switch (LobbyScreen_EnterSeq(LOBBY_ENTER_TOWN_MEMBERS)) {
        case LOBBY_STEP_DONE:
            gLobbyScreenMode = LOBBY_SCREEN_TOWN;
            gLobby->connected = 1;
            LobbyTown_BeginRunning();
            break;
        }
        break;
    }
    gLobbyMenu->redraw = 1;
}

/*
 * The request step that asks the server for the player's place (6891):
 * it waits for LobbyStep_CurrentPlaceDone. Returns LOBBY_STEP_DONE,
 * LOBBY_STEP_FAILED or LOBBY_STEP_BUSY.
 */
s32 LobbyStep_CurrentPlace(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        gLobby->awaitReply = LOBBY_AWAIT_CURRENT_PLACE;
        LobbyMenu_StartTimer();
        LobbyReq_CurrentPlace(LobbyStep_CurrentPlaceDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* The place's reply (three u16): the first two are the area and the
 * level-1 entry, kept as gLobbyLevel0Sel and gLobbyLevel1Sel. */
void LobbyStep_CurrentPlaceDone(NetMsg msg, NetMsg *msgPtr)
{
    u16 place[3];
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_CURRENT_PLACE) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                gLobby->seqStep++;
                LobbyNet_GetCurrentPlace((struct NetCurrentPlace *)place);
                gLobbyLevel0Sel = place[0];
                gLobbyLevel1Sel = place[1];
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}
