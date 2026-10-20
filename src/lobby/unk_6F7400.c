/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_menu.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"

void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void CamFocus_End(void);
s32 Game_IsOnline(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gTownAreaMsgPending;
extern u8 gMenuWork;
s32 Save_TestEventFlag(s32);

s32 Contest_IsOpen(void) {
    return gTownMode == 0x30;
}

void *memset(void *d, s32 c, u32 n);
void Contest_Draw(u8 *);
extern NetUser gNetUsers[];
extern s8 gContestLevel;
extern u8 gContestSeatId;
extern s8 gContestTalkLow[];
extern s8 gContestTalkTop[];
void Talk_Start(struct Em *npc, s8 quiet);
void Talk_Say(s32 a, s8 b);
void TownMsg_SendContestStart(s32 *arg0);
void Contest_Open(void *obj)
{
    Player *p = obj;
    ContestMenu *m = (ContestMenu *)&gMenuWork;

    gTownMenuOpen = 1;
    gTownMode = 0x30;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Contest_Draw, 0, 0, 0);
    m->spot = gPlayers[gScene.localPlayer].spot;
    gPlayers[gScene.localPlayer].fieldCollide = 0;
    if (Game_IsOnline() == 0) {
        m->level = gContestLevel;
        if (m->level < 7) {
            m->msg = gContestTalkLow;
        } else {
            m->msg = gContestTalkTop;
        }
        gContestSeatId = 1;
        Talk_Start((struct Em *)p, 1); /* a player: the contest's other hunter */
        Talk_Say(*m->msg, 0);
    } else {
        TownMsg_SendContestStart((s32 *) &gNetUsers[p->index]);
        m->state = 1;
    }
    m->next = 0;
    m->who = p;
    m->done = 0;
    m->round = 0;
    gFieldControl = 0;
    gLobby->connected = 0;
}

void Contest_Close(void)
{
    u8 localPlayer;

    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    localPlayer = gScene.localPlayer;
    gPlayers[localPlayer].fieldCollide = 1;
    if (Game_IsOnline() == 0) {
        CamFocus_End();
    }
    gFieldControl = 1;
    gLobby->connected = 1;
    gTownAreaMsgPending = 1;
}

u16 LobbyPad_Get(s32 a);
void Actor_SetState(void *, s32, s32);
s32 Talk_Update(void);
void Jingle_Play(void *obj, s32 id);
s32 Contest_TalkUpdate(void);
s32 Contest_Play(void);
void CamReq_Contest(s32 a, s32 b);
void TownPc_WalkToSpot(Player *obj);
void Contest_GivePrize(u8 won);
void Save_SetEventFlag(s32 flag);
s32 Contest_PrizeDue(void);
s32 Contest_IsTopPrize(void);
extern u8 gContestPrizeGiven;
void Contest_Update(void)
{
    ContestMenu *m = (ContestMenu *)&gMenuWork;
    Player *p = &gPlayers[gScene.localPlayer];

    LobbyPad_Get(0);
    switch (m->state) {
    case 0:
        switch (Contest_TalkUpdate()) {
        case 0:
            if (Game_IsOnline() == 0) {
                Talk_Say(m->msg[2], 0);
                m->state = 3;
                m->next = 1;
            } else {
                m->state = 1;
            }
            break;
        case 3:
            m->state = 2;
            Talk_Say(m->msg[1], 0);
            Actor_SetState(m->who, 0, 0x72);
            break;
        }
        break;
    case 1:
        if (Contest_Play() == 0) {
            if (Game_IsOnline() == 0) {
                if (Contest_PrizeDue() == 1) {
                    m->next = 4;
                } else {
                    m->next = 2;
                }
                m->state = 3;
            } else {
                *((u8 *)gLobby + 0x395E) = 1;
                Contest_Close();
            }
        }
        break;
    case 2:
        if (Game_IsOnline() == 0) {
            if (m->who->state == 1) {
                Contest_Close();
            }
        } else {
            if (Game_IsOnline() == 1) {
                *((u8 *)gLobby + 0x395E) = 1;
            }
            Contest_Close();
        }
        break;
    case 3:
        if (Talk_Update() == 0) {
            switch (m->next) {
            case 1:
                CamReq_Contest(0, *(u16 *)p->spot);
                p->spot = m->spot;
                TownPc_WalkToSpot((Player *)((u8 *) p));
                Jingle_Play(p, 0x23);
                m->done = 1;
                m->state = m->next;
                break;
            case 4:
                if (m->level < 7) {
                    Talk_Say(m->msg[5], 0);
                } else if (Contest_IsTopPrize() == 1) {
                    Talk_Say(m->msg[6], 0);
                } else {
                    Talk_Say(m->msg[5], 0);
                }
                m->state = m->next;
                break;
            case 2:
                m->state = m->next;
                break;
            default:
                Actor_SetState(m->who, 0, 0x72);
                Contest_Close();
                break;
            }
        }
        break;
    case 4:
        if (Talk_Update() == 0) {
            if (Contest_PrizeDue() == 1) {
                if (Contest_IsTopPrize() == 1) {
                    Contest_GivePrize(1);
                    Save_SetEventFlag(0xBF);
                } else {
                    Contest_GivePrize(0);
                }
                gContestPrizeGiven = 1;
            }
            m->state = 2;
        }
        break;
    }
}

s32 Contest_IsIdle(void) {
    return gMenuWork != 1;
}

extern u8 gContestPrizeGiven;
s32 Contest_PrizeDue(void)
{
    ContestMenu *m = (ContestMenu *)&gMenuWork;

    if (m->done == 1 && m->result == 0 && gContestPrizeGiven == 0) {
        return 1;
    }
    return 0;
}

s32 Contest_IsTopPrize(void) {
    ContestMenu *m = (ContestMenu *)&gMenuWork;

    if ((m->level == 7) && (Save_TestEventFlag(0xBF) == 0)) {
        return 1;
    }
    return 0;
}
