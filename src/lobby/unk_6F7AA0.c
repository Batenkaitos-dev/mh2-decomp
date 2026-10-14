#include "common.h"
#include "scene.h"
#include "player.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "pad.h"
#include "em.h"
#include "lobby_npc.h"
#include "lobby_menu.h"
s32 Talk_Update(void);
int abs(int);

/* The drinking contest and arm wrestling (work area gMenuWork; player
 * gTownSpotId - 1 of two). */
void *memcpy(void *dst, const void *src, u32 n);
extern s32 gTownMode;
extern u8 gTownSpotId;
extern u8 gMenuWork;
void Text_SetSize(s32);
void Contest_DrawPlay(void);
void Contest_DrawGauge(s32 mine, s32 theirs);
void Lobby_DrawTimeLeft(s16);
void Contest_DrawButtons(f32 *a, f32 *b, s16 *t);
void Minigame_DrawResult(s8 kind, s16 t, s8 flag);
void LobbySe_Play(s32 se);
s32 Cond_Check(u16 id);
u16 Rand_Next(s32 stream);
extern s8 gContestOpponents[][3];
extern u16 gContestPrizesA[5];
extern u16 gContestPrizesB[5];
s16 Pouch_GiveItem(Player *p, u16 item, s16 key, u8 flags);
extern s32 gTownMenuOpen;
extern void *gNpcKindSe[];
void *memset(void *dst, s32 c, u32 n);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void DrinkContest_Draw(void *arg0);
u8 *DrinkContest_GetLevel(void);
s32 Game_IsOnline(void);
void CamFocus_Start(Em *npc);
void Actor_SetState(void *, s32, s32);
void TownPc_TakeSpot(void);
void Npc_PlaySeDelayed(Em *npc, void *p);
void Talk_Start(Em *npc, s8 quiet);
void Talk_Say(s32 a, s8 b);
u8 Lobby_FindMember(u8 *id);
void Contest_Open(void *obj);
void Effect2C_Spawn(s16);

s32 Contest_TalkUpdate(void)
{
    if (Game_IsOnline() == 0) {
        switch (Talk_Update()) {
        case 0:
            return 0;
        case 3:
            return 3;
        }
    } else {
        return 0;
    }
    return 2;
}

void Contest_NpcPress(void)
{
    ContestMenu *m = (ContestMenu *)&gMenuWork;
    s8 i = gTownSpotId - 1;

    if (--m->wait <= 0) {
        m->power[i ^ 1] += 4;
        m->wait = gContestOpponents[m->level][Rand_Next(1) & 3];
        if (m->timeLeft > 0x5A && m->timeLeft < 0x96 && m->wait > 2) {
            m->wait -= 2;
        }
    }
}

void Contest_PadPress(void)
{
    ContestMenu *m = (ContestMenu *)&gMenuWork;
    s8 i = gTownSpotId - 1;
    u16 pad = gPadState[0].pressed;

    if ((pad & 0x20) && m->toggle == 0) {
        m->power[i] += 4;
        m->toggle = 1;
    } else if ((pad & 0x40) && m->toggle == 1) {
        m->power[i] += 4;
        m->toggle = 0;
    }
}

void TownMsg_Send20(void *u, s32 power);
void TownMsg_Send21(s32 *arg0, s8 arg1);
s32 abs(s32);
extern NetUser gNetUsers[];
extern u16 gTownMoveFrames;
void CamReq_Contest(s32 a, s32 b);
void Jingle_Play(void *obj, s32 id);
void CamReq_Cancel(void);
void Bgm_StopEventFade(Player *p);
void TownMsg_Send1D(Player *p);
void TownMsg_SendStateNow(Player *p);
s32 Player_PassedFrameScaled(void *obj, f32 frame, s32 layer);
void Contest_PadPress(void);
void Contest_NpcPress(void);

s32 Contest_Play(void)
{
    ContestMenu *m = (ContestMenu *)&gMenuWork;
    Player *p = &gPlayers[gScene.localPlayer];
    s8 i = gTownSpotId - 1;

    switch (m->phase) {
    case 0:
        if (Game_IsOnline() == 0) {
            switch (m->step) {
            case 0:
                m->timer = 0x32;
                Effect2C_Spawn(0xFD);
                Actor_SetState(m->who, 0, 0x70);
                m->step++;
                break;
            case 1:
                if (m->timer-- <= 0) {
                    m->step++;
                    m->phase++;
                    Actor_SetState(p, 0, 0x70);
                }
                break;
            case 2:
                break;
            }
        } else {
            m->phase = 6;
            m->timeLeft = 0x12C;
        }
        break;
    case 1:
        if (p->motion[0].playing <= 0) {
            m->timeLeft = 0x5A;
            m->phase++;
            if (Game_IsOnline() == 1) {
                gLobby->connected = 0;
            }
        }
        break;
    case 2:
        if (--m->timeLeft < -9) {
            m->phase++;
            m->timeLeft = 0x12C;
            Actor_SetState(p, 0, 0x6F);
            Actor_SetState(m->who, 0, 0x6F);
            m->scale[0] = 1.0f;
            m->scale[1] = 1.0f;
            m->bump = 0;
        }
        break;
    case 3:
        if (--m->timeLeft <= 0 || abs(m->power[i] - m->power[i ^ 1]) > 0x32) {
            if (Game_IsOnline() == 1) {
                s32 *pw;

                m->phase = 6;
                pw = (s32 *)((u8 *)m->power + i * 4);
                TownMsg_Send20(&gNetUsers[m->who->index], m->power[i]);
                if (i == 1 && abs(*pw - m->power[i ^ 1]) <= 0x32) {
                    m->timeLeft = 0x96;
                } else {
                    m->timeLeft = 0x12C;
                    TownMsg_Send21((s32 *) (&gNetUsers[m->who->index]), m->result);
                }
            } else {
                m->phase++;
                m->timeLeft = 0x3C;
                if (Game_IsOnline() == 0) {
                    if (m->power[i] == m->power[i ^ 1]) {
                        m->power[i]++;
                    }
                }
                if (m->power[i] > m->power[i ^ 1]) {
                    m->result = 0;
                    CamReq_Contest(1, *(u16 *)p->spot);
                    Jingle_Play(p, 0x24);
                    Actor_SetState(p, 0, 0x75);
                    Actor_SetState(m->who, 0, 0x74);
                    Talk_Say(m->msg[3], 0);
                } else if (m->power[i] == m->power[i ^ 1]) {
                    m->result = 2;
                    Actor_SetState(p, 0, 1);
                    Actor_SetState(m->who, 0, 1);
                    m->phase = 5;
                    CamReq_Cancel();
                    Bgm_StopEventFade(p);
                    Talk_Say(1, 0);
                } else {
                    m->result = 1;
                    CamReq_Contest(2, *(u16 *)p->spot);
                    Jingle_Play(p, 0x27);
                    Actor_SetState(m->who, 0, 0x75);
                    Actor_SetState(p, 0, 0x74);
                    Talk_Say(m->msg[4], 0);
                }
            }
        } else {
            Contest_PadPress();
            if (Game_IsOnline() == 0) {
                Contest_NpcPress();
            }
            if (++m->tick > gTownMoveFrames) {
                m->tick = 0;
                TownMsg_Send20(&gNetUsers[m->who->index], m->power[i]);
            }
        }
        break;
    case 4:
        switch (m->result) {
        case 0:
            if (Player_PassedFrameScaled(m->who, 428.0f, 0)) {
                m->phase = 5;
            }
            break;
        case 1:
            if (p->motion[0].playing <= 0) {
                m->phase = 5;
            }
            break;
        }
        break;
    case 5:
        if (m->round == 0) {
            m->round = 1;
        } else {
            m->round = 2;
        }
        if (m->timeLeft > 0) {
            m->timeLeft--;
            break;
        }
        if (Game_IsOnline() == 0) {
            Actor_SetState(m->who, 0, 0x73);
        } else {
            TownMsg_Send1D(p);
        }
        return 0;
    case 6:
        if (--m->timeLeft <= 0) {
            Actor_SetState(p, 0, 1);
            TownMsg_Send1D(p);
            TownMsg_SendStateNow(p);
            return 0;
        }
        break;
    }
    return 2;
}

void Contest_RecvStart(void *msg)
{
    ContestMenu *m = (ContestMenu *)&gMenuWork;
    Player *p = &gPlayers[gScene.localPlayer];

    if (gTownMode == 0x30 || gTownMode == 0x36 || gTownMode == 0x37U || p->state == 0x70 || p->state == 0x71) {
        if (gTownMode != 0x30) {
            Contest_Open(&gPlayers[(s8)Lobby_FindMember(msg)]);
        } else if (m->phase == 6) {
            Contest_Open(&gPlayers[(s8)Lobby_FindMember(msg)]);
        }
        m->state = 1;
        m->phase = 2;
        m->timeLeft = 0x5A;
        if (m->fxSpawned == 0) {
            Effect2C_Spawn(m->timeLeft);
            m->fxSpawned = 1;
        }
    }
}

void Contest_RecvPower(void *src)
{
    u8 *m = &gMenuWork;
    s8 i = gTownSpotId - 1;

    if (gTownMode == 0x30) {
        memcpy(m + (i ^ 1) * 4 + 0xC, src, 4);
    }
}

void Contest_RecvResult(s8 idx, void *data)
{
    ContestMenu *m = (ContestMenu *)&gMenuWork;
    s8 i = gTownSpotId - 1;

    if (gTownMode != 0x30) {
        return;
    }
    if (i == 1) {
        if (idx != gScene.localPlayer) {
            if (m->power[i] > m->power[i ^ 1]) {
                m->result = 0;
            } else if (m->power[i] == m->power[i ^ 1]) {
                m->result = 2;
            } else {
                m->result = 1;
            }
            TownMsg_Send21((s32 *) (&gNetUsers[m->who->index]), m->result);
            m->phase = 5;
            m->timeLeft = 0xB4;
            if (m->result == 0) {
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0x75);
                Actor_SetState(m->who, 0, 0x74);
            } else if (m->result == 2) {
                m->phase = 5;
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0xA0);
            } else {
                Actor_SetState(m->who, 0, 0x75);
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0x74);
            }
            return;
        } else if (abs(m->power[i] - m->power[i ^ 1]) > 0x32) {
            m->phase = 5;
            m->timeLeft = 0xB4;
            if (m->result == 0) {
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0x75);
                Actor_SetState(m->who, 0, 0x74);
            } else if (m->result == 2) {
                m->phase = 5;
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0xA0);
            } else {
                Actor_SetState(m->who, 0, 0x75);
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0x74);
            }
        }
    } else if (idx != gScene.localPlayer) {
        switch (*(u8 *)data) {
        case 0:
            m->result = 1;
            break;
        case 1:
            m->result = 0;
            break;
        case 2:
            m->result = 2;
            break;
        }
        m->phase = 5;
        m->timeLeft = 0xB4;
        if (m->result == 0) {
            Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0x75);
            Actor_SetState(m->who, 0, 0x74);
        } else if (m->result == 2) {
            m->phase = 5;
            Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0xA0);
        } else {
            Actor_SetState(m->who, 0, 0x75);
            Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0x74);
        }
    }
}

void Contest_DrawPlay(void)
{
    ContestMenu *m = (ContestMenu *)&gMenuWork;
    s8 i = gTownSpotId - 1;

    switch (m->phase) {
    case 3:
        Contest_DrawGauge(m->power[i], m->power[i ^ 1]);
        Lobby_DrawTimeLeft(m->timeLeft);
        Contest_DrawButtons(&m->scale[0], &m->scale[1], &m->bump);
        break;
    case 5:
        Minigame_DrawResult(m->result, m->timeLeft, 1);
        if (m->round == 1 && m->result == 0) {
            LobbySe_Play(0x3D);
        }
        break;
    }
}

void Contest_Draw(u8 *o)
{
    Text_SetSize(*(s32 *)(o + 0x18));
    switch (gMenuWork) {
    case 0:
        break;
    case 1:
        Contest_DrawPlay();
        break;
    }
}

s8 Contest_RollLevel(void)
{
    s8 r = 6;

    if (Cond_Check(0x40) == 1) {
        if (Cond_Check(0x47) == 1) {
            r = (Rand_Next(1) & 1) + 6;
        }
    } else {
        r = 6;
    }
    return r;
}

void Contest_GivePrize(u8 won)
{
    Player *p = &gPlayers[gScene.localPlayer];
    ContestMenu *m = (ContestMenu *)&gMenuWork;
    u16 *tbl = gContestPrizesA;

    if (won <= 0) {
        if (Cond_Check(0x47) == 1 && m->level > 6) {
            tbl = gContestPrizesB;
        }
        Pouch_GiveItem(p, tbl[Rand_Next(1) % 5], p->gotItem, 1);
    } else {
        Pouch_GiveItem(p, 0x261, p->gotItem, 1);
    }
}

void DrinkContest_Open(Em *npc)
{
    Minigame *m = (Minigame *)&gMenuWork;
    NpcWork *w = (NpcWork *)&npc->work;

    gTownMenuOpen = 1;
    gTownMode = 0x32;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(DrinkContest_Draw, 0, 0, 0);
    gFieldControl = 0;
    m->npc = npc;
    m->step = 4;
    m->mode = 0;
    m->playStep = 0;
    m->unk7 = 0;
    m->level = 4;
    m->cursor = 0;
    m->menuWait = 0;
    m->running = 0;
    m->countTable = DrinkContest_GetLevel();
    if (Game_IsOnline() == 0) {
        CamFocus_Start(npc);
        Actor_SetState(npc, 0, 0x2C);
        TownPc_TakeSpot();
        if (Game_IsOnline() == 0) {
            Npc_PlaySeDelayed(m->npc, gNpcKindSe[w->id]);
        }
        Talk_Start(npc, 0);
        Talk_Say(3, 0);
    }
    LobbySe_Play(0x11);
}

/* unk_6F7AA0's .data (0x7EC000-0x7EC050) */
s8 gContestOpponents[10][3] = {
#include "lobby/007EC000.inc"
};
u32 gContestTalkLow[2] = {
#include "lobby/007EC020.inc"
};
u32 gContestTalkTop[2] = {
#include "lobby/007EC028.inc"
};
u16 gContestPrizesA[5] = {
#include "lobby/007EC030.inc"
};
u16 gContestPrizesB[5] = {
#include "lobby/007EC040.inc"
};
