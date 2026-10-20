/*
 * The lobby's server name request (68C0) and three small helpers: a reply handler
 * that ignores the reply, Lobby_LacksQuest and Lobby_ClearConnected.
 *
 * LobbyStep_ServerName is one of the lobby's sequence steps (LobbyState.seqStep): it
 * sends the current server's id and waits for the name, which the reply writes into
 * gCurServerName; a failed reply leaves the lobby.
 */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_net.h"
#include "lobby_state.h"

#define SERVER_NAME_LEN 0x80    /* the bytes the reply may write into gCurServerName */

void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
void Lobby_Disconnect(s32);
s32 LobbyReq_ServerName(void *id, void *dst, s32 arg2, void (*onReply)(NetMsg, NetMsg *));
extern char gCurServerId[];
extern u8 gCurServerName;
void LobbyStep_ServerNameDone(NetMsg msg, NetMsg *msgPtr);

s32 Lobby_HasQuest(void);
s32 Game_IsOnline(void);

/* A reply handler for requests whose reply needs nothing done. */
void Lobby_IgnoreReply(NetMsg msg, NetMsg *msgPtr)
{
}

/* The server name's reply (msg; msgPtr is the same message, by address): unless the
 * lobby is being left, and if this request's reply was awaited, the sequence goes on
 * (NET_MSG_DONE) or fails. */
void LobbyStep_ServerNameDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_SERVER_NAME) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/*
 * A sequence step: sends 68C0 with the current server's id (gCurServerId), the
 * reply's name (SERVER_NAME_LEN bytes) going into gCurServerName, and waits for it.
 * Returns LOBBY_STEP_DONE once it is done, else LOBBY_STEP_BUSY; a failed
 * reply leaves the lobby (Lobby_Disconnect(LOBBY_QUIT_FAILED)).
 */
s32 LobbyStep_ServerName(void)
{
    s16 *seq;
    LobbyState *lobby;
    s16 step;

    lobby = gLobby;
    seq = &lobby->seqStep;
    step = *seq;
    switch (step) {
    case 0:
        *seq = (s16)(step + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_SERVER_NAME;
        LobbyReq_ServerName(gCurServerId, &gCurServerName, SERVER_NAME_LEN, LobbyStep_ServerNameDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        *seq = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    return LOBBY_STEP_BUSY;
}

/* 1 when the lobby has no quest (Lobby_HasQuest isn't 1). */
s32 Lobby_LacksQuest(void)
{
    return Lobby_HasQuest() != 1;
}

/* Online, marks the lobby as off the lobby server (LobbyState.connected 0). */
void Lobby_ClearConnected(void)
{
    if (Game_IsOnline() == 1) {
        gLobby->connected = 0;
    }
}
