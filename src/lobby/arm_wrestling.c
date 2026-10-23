/*
 * The arm-wrestling contest (TOWN_MODE_CONTEST): its frame and its match.
 *
 * The frame opens and closes the contest (Contest_Open, Contest_Close) and
 * runs the opponent's talk around the match and the prize (Contest_Update).
 * Offline the opponent is a townsperson: its offer (a yes or no), then the
 * match, then its line for the result and, after a win, a prize
 * (Contest_GivePrize; the top prize once, at CONTEST_LEVEL_TOP). Online the
 * opponent is the other member in the contest's seats, and the match
 * starts at once.
 *
 * The match (Contest_Play): the sitting down, the count, the presses
 * (circle and cross in turn, CONTEST_PRESS_POWER each), the result and its
 * banner; offline the townsperson's presses (Contest_NpcPress, at the pace
 * of its level's row in gContestOpponents), online the contestants'
 * messages (Contest_RecvStart, Contest_RecvPower, Contest_RecvResult). Then
 * the opponent's level (Contest_RollLevel) and the prizes
 * (Contest_GivePrize). The two contestants are the seats gContestSeatId - 1
 * and the other; the state is ContestMenu in gMenuWork.
 *
 * The file's data: gContestOpponents, the opponents' talk ids
 * (gContestTalkLow, gContestTalkTop) and the prizes.
 */
#include "common.h"
#include "scene.h"
#include "player.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "pad.h"
#include "em.h"
#include "lobby_npc.h"
#include "lobby_menu.h"
#include "menu_task.h"
#include "lobby_text.h"
#include "quest.h"
#include "bgm.h"
#include "camera.h"
#include "fishing.h"
#include "lobby_net.h"
#include "save.h"
#include "text.h"
#include "village_cond.h"
#include "arm_wrestling.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_util.h"
#include "minigame_draw.h"
#include "rand.h"
#include "talk.h"
#include "town_player_states.h"
#include "town_players.h"
#include "libc.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gTownAreaMsgPending;
extern u8 gMenuWork;

/* Whether the arm-wrestling contest is open. */
s32 Contest_IsOpen(void)
{
    return gTownMode == TOWN_MODE_CONTEST;
}

extern NetUser gNetUsers[];
extern s8 gContestLevel;
extern u8 gContestSeatId;
extern s8 gContestTalkLow[];
extern s8 gContestTalkTop[];
/*
 * Opens the contest against `opponent` (a townsperson offline, read as a
 * Player, or the other member online). Offline the opponent's level picks
 * its lines and it makes its offer; online the start goes to the other
 * member (TownMsg_SendContestStart) and the match begins. The player's
 * collision, the field's control and the lobby's timeouts are off until
 * Contest_Close.
 */
void Contest_Open(void *opponent)
{
    Player *pl = opponent;
    ContestMenu *menu = (ContestMenu *)&gMenuWork;

    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_CONTEST;
    memset(menu, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Contest_Draw, 0, 0, 0);
    menu->spot = gPlayers[gScene.localPlayer].spot;
    gPlayers[gScene.localPlayer].fieldCollide = 0;
    if (Game_IsOnline() == 0) {
        menu->level = gContestLevel;
        if (menu->level < CONTEST_LEVEL_TOP) {
            menu->msg = gContestTalkLow;
        } else {
            menu->msg = gContestTalkTop;
        }
        gContestSeatId = 1;
        Talk_Start((struct Em *)pl, 1);
        Talk_Say(menu->msg[CONTEST_TALK_OFFER], 0);
    } else {
        TownMsg_SendContestStart((s32 *) &gNetUsers[pl->index]);
        menu->state = CONTEST_PLAY;
    }
    menu->next = CONTEST_TALK;
    menu->who = pl;
    menu->played = 0;
    menu->round = 0;
    gFieldControl = 0;
    gLobby->connected = 0;
}

/* Closes the contest: the town again (TOWN_MODE_FREE), with the player's
 * collision, the camera, the field's control and the lobby's timeouts back,
 * and the area's message shown when due (gTownAreaMsgPending). */
void Contest_Close(void)
{
    u8 localPlayer;

    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
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

extern u8 gContestPrizeGiven;
/*
 * The contest's frame: the offer's answer (no: the opponent leaves), the
 * match (Contest_Play), the line for its result, then after a win the
 * prize (the top prize's line at CONTEST_LEVEL_TOP the first time), and
 * the close once the opponent is idle again. Online the match's end closes
 * it at once.
 */
void Contest_Update(void)
{
    ContestMenu *menu = (ContestMenu *)&gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];

    LobbyPad_Get(0);
    switch (menu->state) {
    case CONTEST_TALK:
        switch (Contest_TalkUpdate()) {
        case MENU_DONE:
            if (Game_IsOnline() == 0) {
                Talk_Say(menu->msg[CONTEST_TALK_START], 0);
                menu->state = CONTEST_MESSAGE;
                menu->next = CONTEST_PLAY;
            } else {
                menu->state = CONTEST_PLAY;
            }
            break;
        case MENU_CANCEL:
            menu->state = CONTEST_END;
            Talk_Say(menu->msg[CONTEST_TALK_DECLINED], 0);
            Actor_SetState(menu->who, 0, NPC_STATE_CONTEST_LEAVE);
            break;
        }
        break;
    case CONTEST_PLAY:
        if (Contest_Play() == MENU_DONE) {
            if (Game_IsOnline() == 0) {
                if (Contest_PrizeDue() == 1) {
                    menu->next = CONTEST_PRIZE;
                } else {
                    menu->next = CONTEST_END;
                }
                menu->state = CONTEST_MESSAGE;
            } else {
                gLobby->connected = 1;
                Contest_Close();
            }
        }
        break;
    case CONTEST_END:
        if (Game_IsOnline() == 0) {
            if (menu->who->state == NPC_STATE_IDLE) {
                Contest_Close();
            }
        } else {
            if (Game_IsOnline() == 1) {
                gLobby->connected = 1;
            }
            Contest_Close();
        }
        break;
    case CONTEST_MESSAGE:
        if (Talk_Update() == MENU_DONE) {
            switch (menu->next) {
            case CONTEST_PLAY:
                CamReq_Contest(0, *(u16 *)pl->spot);
                pl->spot = menu->spot;
                TownPc_WalkToSpot(pl);
                Jingle_Play(pl, CONTEST_JINGLE_START);
                menu->played = 1;
                menu->state = menu->next;
                break;
            case CONTEST_PRIZE:
                if (menu->level < CONTEST_LEVEL_TOP) {
                    Talk_Say(menu->msg[CONTEST_TALK_PRIZE], 0);
                } else if (Contest_IsTopPrize() == 1) {
                    Talk_Say(menu->msg[CONTEST_TALK_TOP_PRIZE], 0);
                } else {
                    Talk_Say(menu->msg[CONTEST_TALK_PRIZE], 0);
                }
                menu->state = menu->next;
                break;
            case CONTEST_END:
                menu->state = menu->next;
                break;
            default:
                Actor_SetState(menu->who, 0, NPC_STATE_CONTEST_LEAVE);
                Contest_Close();
                break;
            }
        }
        break;
    case CONTEST_PRIZE:
        if (Talk_Update() == MENU_DONE) {
            if (Contest_PrizeDue() == 1) {
                if (Contest_IsTopPrize() == 1) {
                    Contest_GivePrize(1);
                    Save_SetEventFlag(EVENT_FLAG_CONTEST_TOP_PRIZE);
                } else {
                    Contest_GivePrize(0);
                }
                gContestPrizeGiven = 1;
            }
            menu->state = CONTEST_END;
        }
        break;
    }
}

/* Whether no match is being played (ContestMenu.state isn't CONTEST_PLAY). */
s32 Contest_IsIdle(void)
{
    return gMenuWork != CONTEST_PLAY;
}

/* Whether a prize is due: a match was played and won, and no prize given
 * yet (gContestPrizeGiven). */
s32 Contest_PrizeDue(void)
{
    ContestMenu *menu = (ContestMenu *)&gMenuWork;

    if (menu->played == 1 && menu->result == MINIGAME_WIN && gContestPrizeGiven == 0) {
        return 1;
    }
    return 0;
}

/* Whether the prize is the top one: the top level's opponent, the first
 * time (EVENT_FLAG_CONTEST_TOP_PRIZE not set). */
s32 Contest_IsTopPrize(void)
{
    ContestMenu *menu = (ContestMenu *)&gMenuWork;

    if ((menu->level == CONTEST_LEVEL_TOP) && (Save_TestEventFlag(EVENT_FLAG_CONTEST_TOP_PRIZE) == 0)) {
        return 1;
    }
    return 0;
}

extern s8 gContestOpponents[][3];
extern u16 gContestPrizesA[5];
extern u16 gContestPrizesB[5];
s16 Pouch_GiveItem(Player *p, u16 item, s16 key, u8 flags);

/* Offline, the opponent's talk (Talk_Update): MENU_DONE, MENU_CANCEL (the
 * offer refused) or MENU_RUNNING. Online there is no talk: MENU_DONE. */
s32 Contest_TalkUpdate(void)
{
    if (Game_IsOnline() == 0) {
        switch (Talk_Update()) {
        case MENU_DONE:
            return MENU_DONE;
        case MENU_CANCEL:
            return MENU_CANCEL;
        }
    } else {
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/* The townsperson's press: each time its wait runs out it gains
 * CONTEST_PRESS_POWER and waits again by its level's row of
 * gContestOpponents (a random entry); between 150 and 90 frames left it
 * presses faster. */
void Contest_NpcPress(void)
{
    ContestMenu *menu = (ContestMenu *)&gMenuWork;
    s8 seat = gContestSeatId - 1;

    if (--menu->wait <= 0) {
        menu->power[seat ^ 1] += CONTEST_PRESS_POWER;
        menu->wait = gContestOpponents[menu->level][Rand_Next(1) & 3];
        if (menu->timeLeft > 90 && menu->timeLeft < 150 && menu->wait > 2) {
            menu->wait -= 2;
        }
    }
}

/* The player's press: circle and cross in turn, CONTEST_PRESS_POWER each. */
void Contest_PadPress(void)
{
    ContestMenu *menu = (ContestMenu *)&gMenuWork;
    s8 seat = gContestSeatId - 1;
    u16 pad = gPadState[0].pressed;

    if ((pad & PAD_CIRCLE) && menu->toggle == 0) {
        menu->power[seat] += CONTEST_PRESS_POWER;
        menu->toggle = 1;
    } else if ((pad & PAD_CROSS) && menu->toggle == 1) {
        menu->power[seat] += CONTEST_PRESS_POWER;
        menu->toggle = 0;
    }
}

extern u16 gTownMoveFrames;
s32 EmMotion_PassedFrameScaled(void *obj, f32 frame, s32 layer);

/*
 * The match's frame, by ContestMenu.phase. Offline the opponent and then
 * the player sit down; after the count the presses run for 300 frames or
 * until one leads by more than CONTEST_WIN_MARGIN (offline an even score
 * goes to the player). Offline the result is decided here, with its
 * camera, jingle, poses and line; online each contestant sends its power,
 * and the result comes by message (Contest_RecvResult). The result's
 * banner then shows. Returns MENU_DONE when the match is over,
 * MENU_RUNNING before.
 */
s32 Contest_Play(void)
{
    ContestMenu *menu = (ContestMenu *)&gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    s8 seat = gContestSeatId - 1;

    switch (menu->phase) {
    case CONTEST_PHASE_SIT:
        if (Game_IsOnline() == 0) {
            switch (menu->step) {
            case 0:
                menu->timer = 50;
                Effect2C_Spawn(253);
                Actor_SetState(menu->who, 0, NPC_STATE_CONTEST_SEAT);
                menu->step++;
                break;
            case 1:
                if (menu->timer-- <= 0) {
                    menu->step++;
                    menu->phase++;
                    Actor_SetState(pl, 0, TOWN_PC_CONTEST_SEAT_A);
                }
                break;
            case 2:
                break;
            }
        } else {
            menu->phase = CONTEST_PHASE_ONLINE_WAIT;
            menu->timeLeft = 300;
        }
        break;
    case CONTEST_PHASE_SIT_MOTION:
        if (pl->motion[0].playing <= 0) {
            menu->timeLeft = 90;
            menu->phase++;
            if (Game_IsOnline() == 1) {
                gLobby->connected = 0;
            }
        }
        break;
    case CONTEST_PHASE_COUNTDOWN:
        if (--menu->timeLeft < -9) {
            menu->phase++;
            menu->timeLeft = 300;
            Actor_SetState(pl, 0, TOWN_PC_CONTEST_GRIP);
            Actor_SetState(menu->who, 0, TOWN_PC_CONTEST_GRIP); /* NPC_STATE_CONTEST_GRIP for a townsperson: the same number */
            menu->scale[0] = 1.0f;
            menu->scale[1] = 1.0f;
            menu->bump = 0;
        }
        break;
    case CONTEST_PHASE_MATCH:
        if (--menu->timeLeft <= 0 || abs(menu->power[seat] - menu->power[seat ^ 1]) > CONTEST_WIN_MARGIN) {
            if (Game_IsOnline() == 1) {
                s32 *myPower;

                menu->phase = CONTEST_PHASE_ONLINE_WAIT;
                myPower = (s32 *)((u8 *)menu->power + seat * 4); /* &menu->power[seat] by its byte offset: as an index, the compiler adds the two registers the other way round */
                TownMsg_SendContestPower(&gNetUsers[menu->who->index], menu->power[seat]);
                if (seat == 1 && abs(*myPower - menu->power[seat ^ 1]) <= CONTEST_WIN_MARGIN) {
                    menu->timeLeft = 150;
                } else {
                    menu->timeLeft = 300;
                    TownMsg_SendContestResult((s32 *) (&gNetUsers[menu->who->index]), menu->result);
                }
            } else {
                menu->phase++;
                menu->timeLeft = 60;
                if (Game_IsOnline() == 0) {
                    if (menu->power[seat] == menu->power[seat ^ 1]) {
                        menu->power[seat]++;
                    }
                }
                if (menu->power[seat] > menu->power[seat ^ 1]) {
                    menu->result = MINIGAME_WIN;
                    CamReq_Contest(1, *(u16 *)pl->spot);
                    Jingle_Play(pl, CONTEST_JINGLE_WIN);
                    Actor_SetState(pl, 0, TOWN_PC_CONTEST_WIN);
                    Actor_SetState(menu->who, 0, NPC_STATE_CONTEST_LOSE);
                    Talk_Say(menu->msg[CONTEST_TALK_WON], 0);
                } else if (menu->power[seat] == menu->power[seat ^ 1]) {
                    menu->result = MINIGAME_DRAW;
                    Actor_SetState(pl, 0, TOWN_PC_IDLE);
                    Actor_SetState(menu->who, 0, NPC_STATE_IDLE);
                    menu->phase = CONTEST_PHASE_RESULT;
                    CamReq_Cancel();
                    Bgm_StopEventFade(pl);
                    Talk_Say(1, 0);
                } else {
                    menu->result = MINIGAME_LOSE;
                    CamReq_Contest(2, *(u16 *)pl->spot);
                    Jingle_Play(pl, CONTEST_JINGLE_LOSE);
                    Actor_SetState(menu->who, 0, NPC_STATE_CONTEST_WIN);
                    Actor_SetState(pl, 0, TOWN_PC_CONTEST_LOSE);
                    Talk_Say(menu->msg[CONTEST_TALK_LOST], 0);
                }
            }
        } else {
            Contest_PadPress();
            if (Game_IsOnline() == 0) {
                Contest_NpcPress();
            }
            if (++menu->tick > gTownMoveFrames) {
                menu->tick = 0;
                TownMsg_SendContestPower(&gNetUsers[menu->who->index], menu->power[seat]);
            }
        }
        break;
    case CONTEST_PHASE_FINISH:
        switch (menu->result) {
        case MINIGAME_WIN:
            if (EmMotion_PassedFrameScaled(menu->who, 428.0f, 0)) {
                menu->phase = CONTEST_PHASE_RESULT;
            }
            break;
        case MINIGAME_LOSE:
            if (pl->motion[0].playing <= 0) {
                menu->phase = CONTEST_PHASE_RESULT;
            }
            break;
        }
        break;
    case CONTEST_PHASE_RESULT:
        if (menu->round == 0) {
            menu->round = 1;
        } else {
            menu->round = 2;
        }
        if (menu->timeLeft > 0) {
            menu->timeLeft--;
            break;
        }
        if (Game_IsOnline() == 0) {
            Actor_SetState(menu->who, 0, NPC_STATE_CONTEST_END);
        } else {
            TownMsg_SendContestSeatFree(pl);
        }
        return MENU_DONE;
    case CONTEST_PHASE_ONLINE_WAIT:
        if (--menu->timeLeft <= 0) {
            Actor_SetState(pl, 0, TOWN_PC_IDLE);
            TownMsg_SendContestSeatFree(pl);
            TownMsg_SendStateNow(pl);
            return MENU_DONE;
        }
        break;
    }
    return MENU_RUNNING;
}

/* TOWN_MSG_CONTEST_START: the other contestant (msg holds its user id)
 * starts the match. In the contest or in a contest seat, the contest opens
 * against that member if it isn't open already (or is waiting online) and
 * the match goes to its count. */
void Contest_RecvStart(void *msg)
{
    ContestMenu *menu = (ContestMenu *)&gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];

    if (gTownMode == TOWN_MODE_CONTEST || gTownMode == TOWN_MODE_CONTEST_SEAT_WAIT || gTownMode == (u32)TOWN_MODE_CONTEST_SEATED
        || pl->state == TOWN_PC_CONTEST_SEAT_A || pl->state == TOWN_PC_CONTEST_SEAT_B) {
        if (gTownMode != TOWN_MODE_CONTEST) {
            Contest_Open(&gPlayers[(s8)Lobby_FindMember(msg)]);
        } else if (menu->phase == CONTEST_PHASE_ONLINE_WAIT) {
            Contest_Open(&gPlayers[(s8)Lobby_FindMember(msg)]);
        }
        menu->state = CONTEST_PLAY;
        menu->phase = CONTEST_PHASE_COUNTDOWN;
        menu->timeLeft = 90;
        if (menu->fxSpawned == 0) {
            Effect2C_Spawn(menu->timeLeft);
            menu->fxSpawned = 1;
        }
    }
}

/* TOWN_MSG_CONTEST_POWER: the other contestant's power, into its seat's
 * ContestMenu.power. */
void Contest_RecvPower(void *src)
{
    ContestMenu *menu = (ContestMenu *)&gMenuWork;
    s8 seat = gContestSeatId - 1;

    if (gTownMode == TOWN_MODE_CONTEST) {
        memcpy(&menu->power[seat ^ 1], src, sizeof(s32));
    }
}

/*
 * TOWN_MSG_CONTEST_RESULT from player slot `slot`. In seat 2 the result is
 * worked out from the powers and sent back; in seat 1 the other's result
 * (`theirResult`) is turned round (its win is the player's loss). Either way the
 * banner shows for 180 frames, with the winner's and the loser's poses
 * (TOWN_PC_CONTEST_DRAW on the player for a draw).
 */
void Contest_RecvResult(s8 slot, void *theirResult)
{
    ContestMenu *menu = (ContestMenu *)&gMenuWork;
    s8 seat = gContestSeatId - 1;

    if (gTownMode != TOWN_MODE_CONTEST) {
        return;
    }
    if (seat == 1) {
        if (slot != gScene.localPlayer) {
            if (menu->power[seat] > menu->power[seat ^ 1]) {
                menu->result = MINIGAME_WIN;
            } else if (menu->power[seat] == menu->power[seat ^ 1]) {
                menu->result = MINIGAME_DRAW;
            } else {
                menu->result = MINIGAME_LOSE;
            }
            TownMsg_SendContestResult((s32 *) (&gNetUsers[menu->who->index]), menu->result);
            menu->phase = CONTEST_PHASE_RESULT;
            menu->timeLeft = 180;
            if (menu->result == MINIGAME_WIN) {
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, TOWN_PC_CONTEST_WIN);
                Actor_SetState(menu->who, 0, TOWN_PC_CONTEST_LOSE);
            } else if (menu->result == MINIGAME_DRAW) {
                menu->phase = CONTEST_PHASE_RESULT;
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, TOWN_PC_CONTEST_DRAW);
            } else {
                Actor_SetState(menu->who, 0, TOWN_PC_CONTEST_WIN);
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, TOWN_PC_CONTEST_LOSE);
            }
            return;
        } else if (abs(menu->power[seat] - menu->power[seat ^ 1]) > CONTEST_WIN_MARGIN) {
            menu->phase = CONTEST_PHASE_RESULT;
            menu->timeLeft = 180;
            if (menu->result == MINIGAME_WIN) {
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, TOWN_PC_CONTEST_WIN);
                Actor_SetState(menu->who, 0, TOWN_PC_CONTEST_LOSE);
            } else if (menu->result == MINIGAME_DRAW) {
                menu->phase = CONTEST_PHASE_RESULT;
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, TOWN_PC_CONTEST_DRAW);
            } else {
                Actor_SetState(menu->who, 0, TOWN_PC_CONTEST_WIN);
                Actor_SetState(&gPlayers[gScene.localPlayer], 0, TOWN_PC_CONTEST_LOSE);
            }
        }
    } else if (slot != gScene.localPlayer) {
        switch (*(u8 *)theirResult) {
        case MINIGAME_WIN:
            menu->result = MINIGAME_LOSE;
            break;
        case MINIGAME_LOSE:
            menu->result = MINIGAME_WIN;
            break;
        case MINIGAME_DRAW:
            menu->result = MINIGAME_DRAW;
            break;
        }
        menu->phase = CONTEST_PHASE_RESULT;
        menu->timeLeft = 180;
        if (menu->result == MINIGAME_WIN) {
            Actor_SetState(&gPlayers[gScene.localPlayer], 0, TOWN_PC_CONTEST_WIN);
            Actor_SetState(menu->who, 0, TOWN_PC_CONTEST_LOSE);
        } else if (menu->result == MINIGAME_DRAW) {
            menu->phase = CONTEST_PHASE_RESULT;
            Actor_SetState(&gPlayers[gScene.localPlayer], 0, TOWN_PC_CONTEST_DRAW);
        } else {
            Actor_SetState(menu->who, 0, TOWN_PC_CONTEST_WIN);
            Actor_SetState(&gPlayers[gScene.localPlayer], 0, TOWN_PC_CONTEST_LOSE);
        }
    }
}

/* Draws the match: the power gauge, the time left and the buttons while it
 * runs; the result's banner after (with LOBBY_SE_MINIGAME_WIN on the first
 * match's win). */
void Contest_DrawPlay(void)
{
    ContestMenu *menu = (ContestMenu *)&gMenuWork;
    s8 seat = gContestSeatId - 1;

    switch (menu->phase) {
    case CONTEST_PHASE_MATCH:
        Contest_DrawGauge(menu->power[seat], menu->power[seat ^ 1]);
        Lobby_DrawTimeLeft(menu->timeLeft);
        Contest_DrawButtons(&menu->scale[0], &menu->scale[1], &menu->bump);
        break;
    case CONTEST_PHASE_RESULT:
        Minigame_DrawResult(menu->result, menu->timeLeft, 1);
        if (menu->round == 1 && menu->result == MINIGAME_WIN) {
            LobbySe_Play(LOBBY_SE_MINIGAME_WIN);
        }
        break;
    }
}

/* The contest's draw callback: the match while it is played. The state is
 * read as gMenuWork's first byte (ContestMenu.state). */
void Contest_Draw(MenuTask *task)
{
    Text_SetSize(task->textSize);
    switch (gMenuWork) {
    case CONTEST_TALK:
        break;
    case CONTEST_PLAY:
        Contest_DrawPlay();
        break;
    }
}

/* The opponent's level for the day: 6, or once village conditions 64 and
 * 71 hold, 6 or CONTEST_LEVEL_TOP at random. */
s8 Contest_RollLevel(void)
{
    s8 level = 6;

    if (Cond_Check(64) == 1) {
        if (Cond_Check(71) == 1) {
            level = (Rand_Next(1) & 1) + 6;
        }
    } else {
        level = 6;
    }
    return level;
}

/* Gives the player the prize: with `won` (the top prize) item 609, otherwise
 * one of gContestPrizesA at random (gContestPrizesB against the top level
 * once village condition 71 holds). */
void Contest_GivePrize(u8 won)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    ContestMenu *menu = (ContestMenu *)&gMenuWork;
    u16 *prizes = gContestPrizesA;

    if (won <= 0) {
        if (Cond_Check(71) == 1 && menu->level > CONTEST_LEVEL_TOP - 1) {
            prizes = gContestPrizesB;
        }
        Pouch_GiveItem(pl, prizes[Rand_Next(1) % CONTEST_PRIZE_COUNT], pl->gotItem, 1);
    } else {
        Pouch_GiveItem(pl, 609, pl->gotItem, 1);
    }
}

/* The contest's data: the opponents by round, the talk lines by result, and the two prize tables. */
s8 gContestOpponents[10][3] = {
#include "lobby/007EC000.inc"
};
s8 gContestTalkLow[8] = {
#include "lobby/007EC020.inc"
};
s8 gContestTalkTop[8] = {
#include "lobby/007EC028.inc"
};
u16 gContestPrizesA[5] = {
#include "lobby/007EC030.inc"
};
u16 gContestPrizesB[5] = {
#include "lobby/007EC040.inc"
};
