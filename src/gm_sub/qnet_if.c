/*
 * NETWORK: the game's side of the in-quest protocol (qnet.c): the session
 * start with its settings (QNetIf_Start), the callbacks it registers (a
 * game message, a player leaving or lost) and the wrappers the game calls.
 * The game messages themselves are qnet_sync.c's (qsync.h).
 */
#include "common.h"
#include "syssettings.h"
#include "scene.h"
#include "player.h"
#include "qnet.h"
#include "qsync.h"

/* The session's settings (QNetIf_Start; sub_main's gSrvQNetMaxStall,
 * gSrvQNetPeerTimeout and gQuestQNetReadyArg override some when nonzero). */
f32 gQNetSendInterval = {
#include "gm_sub/0090AC80.inc"
};   /* seconds between game sends (QNet.sendInterval) */
s32 gQNetSendBudget = {
#include "gm_sub/0090AC88.inc"
};    /* bytes a second (QNet.sendBudget) */
f32 gQNetPeerTimeout = {
#include "gm_sub/0090AC90.inc"
};   /* seconds: a silent player is lost after it (QNet.peerTimeout) */
f32 gQNetJoinTimeout = {
#include "gm_sub/0090AC98.inc"
};   /* seconds the join waits (QNet.joinTimeout) */
f32 gQNetMaxStall = {
#include "gm_sub/0090ACA0.inc"
};      /* seconds the send buffer may stay full (QNet.maxStall) */
s32 gQSyncResendDelay = {
#include "gm_sub/0090ACA8.inc"
};     /* frames before QSync_SendPlayer sends a player again (Player.syncTimer) */
s32 gQSyncSendPeriod = {
#include "gm_sub/0090ACB0.inc"
};      /* game: a player not waiting on syncTimer is sent when gFrameTick % this is 0 */
/* QuestNet_Ready's argument, by QNetIf_Ready's index. */
s32 gQNetReadyArg[2] = {
#include "gm_sub/0090ACB8.inc"
};

char gQNetIfText[0x100];        /* the messages shown when a player leaves */
/* gScene.inQuest as its own symbol: with &gScene.inQuest[peer],
 * QNetIf_OnPlayerLost and QNetIf_OnPlayerLeft compile differently. */
extern u8 gSceneInQuest[];
extern char *gQNetLeftText;     /* sub_main data: the text put after a leaving player's name */

void sprintf(char *, const char *, ...);
u8 Notice_PushText(char *s);
u8 Notice_PushItem(char *item);
void Quest_FinishIfIdle(void);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
s32 Game_IsOnline(void);
void QSync_OnPlayer(u8, QSyncPlayerMsg *, s32);
void QSync_OnEm(u8, QSyncEmMsg *, s32);
void QSync_OnState(u8, QSyncStateMsg *, s32);
void QSync_OnChat(u8, QSyncChatMsg *, s32);
void QSync_OnSupply(u8, QSyncSupplyMsg *, s32);
void QuestNet_Init(QNetConfig *);
void QNetIf_OnPlayerLost(u8);
void QNetIf_OnPlayerLeft(u8);
void QuestNet_Post(void *msg);
void QuestNet_Flush(void);
s32 QuestNet_IsActive(void);
s32 QuestNet_GetState(void);
s32 QuestNet_IsRunning(void);
u8 QuestNet_GetCount(void);
void QuestNet_Receive(void);
void QuestNet_Tick(void);
void QuestNet_Ready(u8);
void QuestNet_Unready(void);
void QuestNet_Leave(void);
void QuestNet_Close(void);

/* The session's message callback: hands a game message from player `from`
 * to its QSync_On* handler by its cmd. The local player's own messages are
 * dropped, but for the supply box's. Returns 0. */
s32 QNetIf_OnMessage(u8 from, QSyncHead *msg, s32 arg)
{
    if (msg->cmd != QSYNC_CMD_SUPPLY && from == gScene.localPlayer) {
        return 0;
    }
    switch (msg->cmd) {
    case QSYNC_CMD_NONE:
        break;
    case QSYNC_CMD_PLAYER:      /* + the sender's index */
    case QSYNC_CMD_PLAYER + 1:
    case QSYNC_CMD_PLAYER + 2:
    case QSYNC_CMD_PLAYER + 3:
        QSync_OnPlayer(from, (QSyncPlayerMsg *)msg, 0);
        break;
    case QSYNC_CMD_SUPPLY:
        QSync_OnSupply(from, (QSyncSupplyMsg *)msg, 0);
        break;
    case QSYNC_CMD_CHAT:
        QSync_OnChat(from, (QSyncChatMsg *)msg, 0);
        break;
    case QSYNC_CMD_EM:
        QSync_OnEm(from, (QSyncEmMsg *)msg, 0);
        break;
    case QSYNC_CMD_QUEST:
        QSync_OnState(from, (QSyncStateMsg *)msg, 0);
        break;
    }
    return 0;
}

/* Makes the first player still in the quest (gScene.inQuest[i] == 1) the
 * host (gScene.host), and clears the local player's supplyTaken.
 * Static: so its callers keep a value in v0 across the call, as the
 * game's code does. */
static void QNetIf_FindHost(void)
{
    s16 i;

    for (i = 0; i < gScene.playerCount; i++) {
        if (gScene.inQuest[i] == 1) {
            gScene.host = i;
            if (gPlayers[gScene.localPlayer].supplyTaken != 0) {
                gPlayers[gScene.localPlayer].supplyTaken = 0;
            }
            return;
        }
    }
}

/* The session's callback for a player lost (silent too long): unless
 * already out, marks its slot empty (0xFF in gScene.inQuest), shows
 * its name with gQNetLeftText, puts it in the event group's leave state
 * and finds the host again. */
void QNetIf_OnPlayerLost(u8 peer)
{
    Player *pl;
    u8 *flag = &gSceneInQuest[peer];

    if (*flag != 0xFF) {
        *flag = 0xFF;
        pl = &gPlayers[peer];
        sprintf(gQNetIfText, "%s%s", pl->name, gQNetLeftText);
        Notice_PushText(gQNetIfText);
        if (Player_IsState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE) == 0) {
            Player_SetState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE, PL_SET_NO_SYNC);
        }
        QNetIf_FindHost();
    }
}

/* The session's callback for a player that left: as QNetIf_OnPlayerLost,
 * and sets the player's bit in gScene.peersLeft. gSceneInQuest and
 * gQNetLeftText are named objects here, not addresses: MWCC orders the
 * text's load and the flag's store as the game's code does only then. */
void QNetIf_OnPlayerLeft(u8 peer)
{
    Player *pl;
    u8 *flag = &gSceneInQuest[peer];

    if (*flag != 0xFF) {
        *flag = 0xFF;
        gScene.peersLeft |= 1 << peer;
        pl = &gPlayers[peer];
        sprintf(gQNetIfText, "%s%s", pl->name, gQNetLeftText);
        Notice_PushText(gQNetIfText);
        if (Player_IsState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE) == 0) {
            Player_SetState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE, PL_SET_NO_SYNC);
        }
        QNetIf_FindHost();
    }
}

extern char *gQNetErrorText;
/* Each frame: keeps the last host (gScene.prevHost) and steps the session.
 * When the session ends with an error (QNET_STATE_ERROR), the local player
 * leaves the quest once: it is marked out, a notice says so, and the quest
 * finishes if nothing holds it. */
void QNetIf_Update(void)
{
    gScene.prevHost = gScene.host;
    QuestNet_Tick();
    switch (QuestNet_GetState()) {
    case QNET_STATE_INIT:
    case QNET_STATE_INIT2:
    case QNET_STATE_OPEN:
    case QNET_STATE_JOIN:
    case QNET_STATE_RUN:
    case QNET_STATE_UNREADY:
    case QNET_STATE_LEAVE:
        break;
    case QNET_STATE_ERROR:
        if (gScene.inQuest[gScene.localPlayer] != 0xFF) {
            gScene.inQuest[gScene.localPlayer] = 0xFF;
            sprintf(gQNetIfText, "%s", gQNetErrorText);
            Notice_PushItem(gQNetIfText);
            Quest_FinishIfIdle();
        }
        break;
    }
}

extern s32 gQuestQNetReadyArg;
extern u32 gSrvQNetPeerTimeout;
extern u32 gSrvQNetMaxStall;
/* Starts the session on socket `sock` for `count` players, the local one
 * being player `self` (who becomes gScene.localPlayer; player 0 is the
 * host), with the settings above and this file's callbacks. */
void QNetIf_Start(s32 sock, u8 count, u8 self)
{
    QNetConfig cfg;

    if (gSrvQNetMaxStall != 0) {
        gQNetMaxStall = gSrvQNetMaxStall;
    }
    if (gSrvQNetPeerTimeout != 0) {
        gQNetPeerTimeout = gSrvQNetPeerTimeout;
    }
    if (gQuestQNetReadyArg != 0) {
        gQNetReadyArg[1] = gQNetReadyArg[0] = gQuestQNetReadyArg;
    }
    cfg.mode = 1;
    cfg.count = count;
    cfg.self = self;
    cfg.sock = sock;
    cfg.sendInterval = gQNetSendInterval;
    cfg.sendBudget = gQNetSendBudget;
    cfg.peerTimeout = gQNetPeerTimeout;
    cfg.joinTimeout = gQNetJoinTimeout;
    cfg.maxStall = gQNetMaxStall;
    cfg.onMsg = (QNetMsgFunc)QNetIf_OnMessage;
    cfg.onMsgArg = 1;
    cfg.onLeave = (QNetPeerFunc)QNetIf_OnPlayerLeft;
    cfg.onLeaveArg = 2;
    cfg.onLost = (QNetPeerFunc)QNetIf_OnPlayerLost;
    cfg.onLostArg = 3;
    QuestNet_Init(&cfg);
    gScene.localPlayer = self;
    gScene.host = 0;
}

/* Posts a game message. The callers pass a second argument (0) it does
 * not read. */
void QNetIf_Post(void *msg, s32 unused)
{
    QuestNet_Post(msg);
}

void QNetIf_Flush(void)
{
    QuestNet_Flush();
}

s32 QNetIf_IsRunning(void)
{
    return QuestNet_IsRunning();
}

/* Each frame while the session is active and no sound is loading: steps
 * it (QNetIf_Update) and handles the messages received. */
void QNetIf_UpdateReceive(void)
{
    if ((QuestNet_IsActive() != 0) && (gSysSettings.soundLoading == 0)) {
        QNetIf_Update();
        QuestNet_Receive();
    }
}

/* Each frame online while the session is active: steps it. */
void QNetIf_UpdateOnline(void)
{
    if ((QuestNet_IsActive() != 0) && (Game_IsOnline() != 0)) {
        QNetIf_Update();
    }
}

void QNetIf_Leave(void)
{
    QuestNet_Leave();
}

void QNetIf_Close(void)
{
    QuestNet_Close();
}

u8 QNetIf_GetCount(void)
{
    return QuestNet_GetCount();
}

/* Online: tells the session the local player is ready, with
 * gQNetReadyArg[which & 1]. */
void QNetIf_Ready(s32 which)
{
    if (Game_IsOnline() != 0) {
        QuestNet_Ready(gQNetReadyArg[which & 1]);
    }
}

/* Online: tells the session the local player is no longer ready. */
void QNetIf_Unready(void)
{
    if (Game_IsOnline() != 0) {
        QuestNet_Unready();
    }
}
