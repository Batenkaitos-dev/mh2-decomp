/* NETWORK: the game's side of the in-quest protocol (qnet.c): the session
 * start with its settings (QNetIf_Start), the callbacks it registers (a
 * message, a player leaving or lost) and the wrappers the game calls. */
#include "common.h"
#include "syssettings.h"
#include "scene.h"
#include "player.h"
#include "qnet.h"
#include "qsync.h"

/* Settings (QNetIf_Start; the debug values at 0x5BC4E8.. override some). */
f32 gQNetSendInterval = {
#include "gm_sub/0090AC80.inc"
};   /* seconds between sends (QNet.unk54) */
s32 gQNetSendBudget = {
#include "gm_sub/0090AC88.inc"
};    /* bytes a second (QNet.unk5C) */
f32 gQNetPeerTimeout = {
#include "gm_sub/0090AC90.inc"
};   /* a silent player is lost after it (QNet.rotY) */
f32 gQNetJoinTimeout = {
#include "gm_sub/0090AC98.inc"
};   /* QNet.rotZ */
f32 gQNetMaxStall = {
#include "gm_sub/0090ACA0.inc"
};      /* seconds of full send buffers (QNet.unkB0) */
s32 gQSyncResendDelay = {
#include "gm_sub/0090ACA8.inc"
};     /* frames before QSync_SendPlayer sends a player again (Player.syncTimer) */
s32 gQSyncSendPeriod = {
#include "gm_sub/0090ACB0.inc"
};      /* game: a player not waiting on syncTimer is sent when gFrameTick % this is 0 */
s32 gQNetReadyArg[2] = {
#include "gm_sub/0090ACB8.inc"
};

char gQNetIfText[0x100];        /* the messages shown when a player leaves */
/* gScene.inQuest as its own symbol: &gScene.inQuest[peer] changes QNetIf_OnPlayerLost/Left. */
extern u8 D_6ADED4[];
extern char *D_004476FC;        /* sub_main data: the text shown after a leaving player's name */

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

s32 QNetIf_OnMessage(u8 from, u8 *msg, s32 arg) {
    if (msg[3] != 7 && from == gScene.localPlayer) {
        return 0;
    }
    switch (msg[3]) {
    case 0:
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        QSync_OnPlayer(from, (QSyncPlayerMsg *)msg, 0);
        break;
    case 7:
        QSync_OnSupply(from, (QSyncSupplyMsg *)msg, 0);
        break;
    case 6:
        QSync_OnChat(from, (QSyncChatMsg *)msg, 0);
        break;
    case 8:
        QSync_OnEm(from, (QSyncEmMsg *)msg, 0);
        break;
    case 10:
        QSync_OnState(from, (QSyncStateMsg *)msg, 0);
        break;
    }
    return 0;
}

/* Makes the first player in the quest (gScene.inQuest[i] == 1) the host
 * (0x6ADF1E). Static: MWCC then knows it leaves v0 alone, so its callers
 * keep Player_IsState's result in v0 across it and sign-extend the result
 * into v1, as the original does. */
static void QNetIf_FindHost(void) {
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

void QNetIf_OnPlayerLost(u8 peer) {
    Player *p;
    u8 *flag = &D_6ADED4[peer];

    if (*flag != 0xFF) {
        *flag = 0xFF;
        p = &gPlayers[peer];
        sprintf(gQNetIfText, "%s%s", p->name, D_004476FC);
        Notice_PushText(gQNetIfText);
        if (Player_IsState(p, 4, 5) == 0) {
            Player_SetState(p, 4, 5, 2);
        }
        QNetIf_FindHost();
    }
}

/* As QNetIf_OnPlayerLost, and marks the player in the scene's byte at
 * 0x271 (0x6ADF21). The flag and the message text must be named objects:
 * with either one as an absolute address MWCC keeps the text's load after
 * the flag's store, and the mask gets a3 instead of the original's t0. */
void QNetIf_OnPlayerLeft(u8 peer) {
    Player *p;
    u8 *flag = &D_6ADED4[peer];

    if (*flag != 0xFF) {
        *flag = 0xFF;
        gScene.peersLeft |= 1 << peer;
        p = &gPlayers[peer];
        sprintf(gQNetIfText, "%s%s", p->name, D_004476FC);
        Notice_PushText(gQNetIfText);
        if (Player_IsState(p, 4, 5) == 0) {
            Player_SetState(p, 4, 5, 2);
        }
        QNetIf_FindHost();
    }
}

extern char *D_004476F8;
void QNetIf_Update(void) {
    gScene.prevHost = gScene.host;
    QuestNet_Tick();
    switch (QuestNet_GetState()) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        break;
    case 7:
        if (gScene.inQuest[gScene.localPlayer] != 0xFF) {
            gScene.inQuest[gScene.localPlayer] = 0xFF;
            sprintf(gQNetIfText, "%s", D_004476F8);
            Notice_PushItem(gQNetIfText);
            Quest_FinishIfIdle();
        }
        break;
    }
}

extern s32 gQuestQNetReadyArg;
extern u32 gSrvQNetPeerTimeout;
extern u32 gSrvQNetMaxStall;
void QNetIf_Start(s32 sock, u8 count, u8 self) {
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
    cfg.unk8 = gQNetSendInterval;
    cfg.unkC = gQNetSendBudget;
    cfg.unk10 = gQNetPeerTimeout;
    cfg.unk18 = gQNetJoinTimeout;
    cfg.unk14 = gQNetMaxStall;
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

/* The callers pass a second argument (0) it does not read. */
void QNetIf_Post(void *msg, s32 unused) {
    QuestNet_Post(msg);
}

void QNetIf_Flush(void) {
    QuestNet_Flush();
}

s32 QNetIf_IsRunning(void) {
    return QuestNet_IsRunning();
}

void QNetIf_UpdateReceive(void) {
    if ((QuestNet_IsActive() != 0) && (gSysSettings.soundLoading == 0)) {
        QNetIf_Update();
        QuestNet_Receive();
    }
}

void QNetIf_UpdateOnline(void) {
    if ((QuestNet_IsActive() != 0) && (Game_IsOnline() != 0)) {
        QNetIf_Update();
    }
}

void QNetIf_Leave(void) {
    QuestNet_Leave();
}

void QNetIf_Close(void) {
    QuestNet_Close();
}

u8 QNetIf_GetCount(void) {
    return QuestNet_GetCount();
}

void QNetIf_Ready(s32 idx) {
    if (Game_IsOnline() != 0) {
        QuestNet_Ready(gQNetReadyArg[idx & 1]);
    }
}

void QNetIf_Unready(void) {
    if (Game_IsOnline() != 0) {
        QuestNet_Unready();
    }
}
