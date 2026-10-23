/*
 * The drinking contest at the tavern (TOWN_MODE_DRINK_CONTEST): a
 * townsperson challenges the player to empty DRINK_MUGS mugs first. The
 * contest's state is a DrinkContest in gMenuWork (lobby_menu.h, with its
 * steps and modes); DrinkContest_Open sets it up and DrinkContest_Close
 * ends it. DrinkContest_Update runs the opening menu and the talk,
 * DrinkContest_Play the rounds of keys, DrinkContest_Draw draws them with
 * the helpers below it; DrinkContest_GetLevel picks the rounds' key counts
 * by the contests won. A win also earns the opponent's forecast of the
 * persistent elder dragon.
 */
#include "common.h"
#include "sprite.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "pad.h"
#include "lobby_npc.h"
#include "lobby_menu.h"
#include "lobby_state.h"
#include "em.h"
#include "award.h"
#include "texture.h"
#include "hud.h"
#include "render_state.h"
#include "bgm.h"
#include "camera.h"
#include "fishing.h"
#include "text.h"
#include "drink_contest.h"
#include "drink_mug.h"
#include "engine_math.h"
#include "font.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "minigame_draw.h"
#include "rand.h"
#include "talk.h"
#include "town_frame.h"
#include "town_input.h"
#include "town_player_states.h"
#include "town_players.h"
#include "libc.h"
#include "prim.h"
#include "sprite_queue.h"

extern u8 gMenuWork;

extern s32 gTownMenuOpen;
extern s32 gTownMode;
extern void *gNpcTalkSe[];
s32 Award_Grant(s32 arg0);
extern s8 gDrinkForecastLines[];
extern s32 gDrinkHpLines[];
/* The opening menu's two labels (gDrinkMenuTexts). */
#pragma rodata on
char gDrinkMenuPlayText[32] = {
#include "lobby/008382D0.inc"
};
char gDrinkMenuRulesText[32] = {
#include "lobby/008382F0.inc"
};
#pragma rodata off

/*
 * Opens the drinking contest with the townsperson npc (TOWN_MODE_DRINK_CONTEST):
 * clears gMenuWork as the DrinkContest, sets DrinkContest_Draw to draw it,
 * takes the field's control from the player, and starts at level 4 with
 * the key counts DrinkContest_GetLevel picks. Offline, the camera turns to
 * the opponent, the player sits down and the opponent greets them (talk
 * line 3); DrinkContest_Update then shows the opening menu.
 */
void DrinkContest_Open(Em *npc)
{
    DrinkContest *contest = (DrinkContest *)&gMenuWork;
    NpcWork *work = (NpcWork *)&npc->work;

    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_DRINK_CONTEST;
    memset(contest, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(DrinkContest_Draw, 0, 0, 0);
    gFieldControl = 0;
    contest->npc = npc;
    contest->step = DRINK_STEP_TALK;
    contest->mode = DRINK_MODE_SET_UP;
    contest->playStep = DRINK_PLAY_KEYS;
    contest->shakeArgB = 0;
    contest->level = 4;
    contest->cursor = 0;
    contest->menuWait = 0;
    contest->running = 0;
    contest->countTable = DrinkContest_GetLevel();
    if (Game_IsOnline() == 0) {
        CamFocus_Start(npc);
        Actor_SetState(npc, 0, NPC_STATE_TALK);
        TownPc_TakeSeat();
        if (Game_IsOnline() == 0) {
            Npc_PlaySeDelayed(contest->npc, gNpcTalkSe[work->id]);
        }
        Talk_Start(npc, 0);
        Talk_Say(3, 0);
    }
    LobbySe_Play(LOBBY_SE_MENU_OPEN);
}

/* Ends the drinking contest: closes the town's menu and gives the field's
 * control back to the player. */
void DrinkContest_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    gFieldControl = 1;
}

/* The contest's own code below was built at optimization level 4, the two
 * functions above at the module's default level: the pragma gives that code. */
#pragma optimization_level 4

/*
 * Runs a frame of the drinking contest's menu and talk (TOWN_MODE_DRINK_CONTEST):
 * the opening menu (play, or hear the rules; cross leaves), the contest
 * (DrinkContest_Play), then the result's talk. A win counts towards
 * RECORD_DRINK_WINS (AWARD_DRINK_WINS at 50), and the opponent then tells
 * of the persistent elder dragon saved last (gElderForecastHeard): which
 * one it is, then how worn it is. The contest closes once the mug is back.
 */
void DrinkContest_Update(void)
{
    DrinkContest *contest = (DrinkContest *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);
    u8 line;

    switch (contest->step) {
    case DRINK_STEP_MENU:
        if (pad & (PAD_UP | PAD_DOWN)) {
            contest->cursor = Menu_WrapUpDown(contest->cursor, 2);
        }
        if (contest->menuWait++ >= 6) {
            contest->menuWait = 6;
        }
        if (contest->menuWait >= 6 && (pad & PAD_CIRCLE)) {
            switch (contest->cursor) {
            case DRINK_CHOICE_PLAY:
                Talk_Say(12, 0);
                contest->step++;
                break;
            case DRINK_CHOICE_RULES:
                Talk_Say(5, 0);
                contest->menuWait = 0;
                contest->talkNext = DRINK_STEP_MENU;
                contest->step = DRINK_STEP_TALK;
                break;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        if (pad & PAD_CROSS) {
            Talk_Say(1, 0);
            contest->step = DRINK_STEP_TALK;
            contest->talkNext = DRINK_STEP_CLOSE_WAIT;
            LobbySe_Play(LOBBY_SE_CANCEL);
        }
        break;
    case DRINK_STEP_START_TALK:
        if (Talk_Update() == 0) {
            contest->running = 1;
            contest->step++;
        }
        break;
    case DRINK_STEP_PLAY:
        if (DrinkContest_Play() == 1) {
            if (contest->result == MINIGAME_WIN) {
                contest->talkNext = DRINK_STEP_TALK_FORECAST;
                if (gSave.recordCounts[RECORD_DRINK_WINS] < RECORD_TALLY_MAX) {
                    (gSave.recordCounts[RECORD_DRINK_WINS])++;
                    if (gSave.recordCounts[RECORD_DRINK_WINS] >= 50) {
                        Award_Grant(AWARD_DRINK_WINS);
                    }
                }
            } else {
                contest->talkNext = DRINK_STEP_CLOSE_WAIT;
            }
            contest->step = DRINK_STEP_TALK;
        }
        break;
    case DRINK_STEP_CLOSE_WAIT:
        if (DrinkMug_IsBack() == 1) {
            Actor_SetStateNow((Player *)contest->npc, 0, NPC_STATE_CONTEST_END, 0);
            CamFocus_End();
            contest->step = DRINK_STEP_CLOSE;
        }
        break;
    case DRINK_STEP_CLOSE:
        DrinkContest_Close();
        break;
    case DRINK_STEP_TALK:
        if (Talk_Update() == 0) {
            switch (contest->talkNext) {
            case DRINK_STEP_CLOSE_WAIT:
            default:
                contest->step = contest->talkNext;
                break;
            case DRINK_STEP_TALK_FORECAST:
                gElderForecastHeard = 1;
                switch (PersistentEm_LastKind()) {
                case EM_KIND_KUSHALA_DAORA:
                default:
                    line = 0;
                    break;
                case EM_KIND_LUNASTRA:
                    line = 1;
                    break;
                case EM_KIND_TEOSTRA:
                    line = 2;
                    break;
                case EM_KIND_CHAMELEOS:
                    line = 3;
                    break;
                }
                Talk_Say(gDrinkForecastLines[line], 0);
                contest->talkNext = DRINK_STEP_TALK_HP;
                break;
            case DRINK_STEP_TALK_HP:
                Talk_Say(gDrinkHpLines[DrinkContest_HpStage()], 0);
                contest->talkNext = DRINK_STEP_TALK_LAST;
                break;
            case DRINK_STEP_TALK_LAST:
                Talk_Say(32, 0);
                contest->talkNext = DRINK_STEP_CLOSE_WAIT;
                break;
            }
        }
        break;
    }
}

/* Draws the two rows of DRINK_MUGS mugs, the player's above the opponent's,
 * the emptied ones lit, between two blinking cursors. */
void DrinkContest_DrawCounts(void)
{
    Sprite playerMugs[DRINK_MUGS];
    Sprite npcMugs[DRINK_MUGS];
    DrinkContest *contest = (DrinkContest *)&gMenuWork;
    s16 bounce;
    u8 i;

    bounce = (gFrameTick & 0x10) ? 0 : 6;
    Icon_Draw(140.0f + bounce, 44, 24, 24, 0xFF20C0C0, ICON_CURSOR);
    Icon_Draw(483.0f - bounce, 44, 24, 24, 0xFF20C0C0, ICON_CURSOR_MIRROR);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, TEX_SLOT_MINIGAME);
    Tex_Bind(TEX_SLOT_MINIGAME);
    for (i = 0; i < DRINK_MUGS; i++) {
        playerMugs[i].u0 = 96;
        playerMugs[i].v0 = 96;
        playerMugs[i].u1 = 128;
        playerMugs[i].v1 = 128;
        if (i < DRINK_MUGS - contest->playerMugs) {
            playerMugs[i].color = 0xFFFFFF00;
        } else {
            playerMugs[i].color = 0xB2505050;
        }
        playerMugs[i].x = 170.0f + 30.0f * i;
        playerMugs[i].y = 40;
        playerMugs[i].w = 32;
        playerMugs[i].h = 32;
        Sprite_Draw(&playerMugs[i]);
    }
    for (i = 0; i < DRINK_MUGS; i++) {
        npcMugs[i].u0 = 96;
        npcMugs[i].v0 = 64;
        npcMugs[i].u1 = 128;
        npcMugs[i].v1 = 96;
        if (i < DRINK_MUGS - contest->npcMugs) {
            npcMugs[i].color = 0xFFFFFFFF;
        } else {
            npcMugs[i].color = 0xB2505050;
        }
        npcMugs[i].x = 170.0f + 30.0f * i;
        npcMugs[i].y = 80;
        npcMugs[i].w = 32;
        npcMugs[i].h = 32;
        Sprite_Draw(&npcMugs[i]);
    }
}

extern f32 gDrinkQuickSecs[];
extern u8 gDrinkKeyCounts[];
extern s8 gDrinkRunWeights[][7];
/*
 * Runs a frame of the contest itself (DrinkContest.mode); returns 1 when it
 * is over, 0 before. Each round draws its keys in runs of one key (a run's
 * length from gDrinkRunWeights, the key never the last run's), and the
 * player presses them in order: all hit makes the player drink (faster
 * within quickSecs), and each drink empties a mug at the motion's frame
 * 112. A wrong key ends the round; the third such round knocks the player
 * down. The opponent drinks on its own, a mug at its motion's frame 250.
 * Whoever empties its mugs first wins; online, when the player and the
 * opponent both finish, the result is MINIGAME_DRAW. After the result's
 * camera view and banner the opponent says its line and goes back to its
 * route's state.
 */
s32 DrinkContest_Play(void)
{
    DrinkContest *contest = (DrinkContest *)&gMenuWork;
    u8 key;
    Player *pl = &gPlayers[gScene.localPlayer];
    s8 pick;
    u8 j;
    u16 pad;
    s8 run;
    s8 sum;
    u8 i;
    u8 nextDir;
    NpcWork *work = (NpcWork *)&contest->npc->work;

    switch (contest->mode) {
    case DRINK_MODE_SET_UP:
        contest->mode++;
        contest->wait = 90;
        contest->playerMugs = DRINK_MUGS;
        contest->npcMugs = DRINK_MUGS;
        contest->hits = 0;
        contest->keysLeft = 0;
        contest->down = 0;
        contest->result = MINIGAME_DRAW;
        contest->lastDir = DRINK_KEY_COUNT;
        contest->inputOn = 0;
        contest->missed = 0;
        contest->misses = 0;
        contest->inputFrames = 0;
        contest->quickSecs = 0.0f;
        contest->shakeArgA = 0;
        contest->shakeVel = 0;
        contest->keysShown = 0;
        contest->unkD0 = 9;
        contest->timer[0] = 20;
        contest->phase[0] = DRINK_KEY_WAITING;
        contest->timer[1] = 20;
        contest->phase[1] = DRINK_KEY_WAITING;
        contest->timer[2] = 20;
        contest->phase[2] = DRINK_KEY_WAITING;
        contest->timer[3] = 20;
        contest->phase[3] = DRINK_KEY_WAITING;
        contest->timer[4] = 20;
        contest->phase[4] = DRINK_KEY_WAITING;
        contest->timer[5] = 20;
        contest->phase[5] = DRINK_KEY_WAITING;
        contest->timer[6] = 20;
        contest->phase[6] = DRINK_KEY_WAITING;
        contest->quickSecs = gDrinkQuickSecs[contest->level];
        if (contest->level < 4) {
            contest->count = gDrinkKeyCounts[contest->level];
        } else {
            contest->count = Rand_Next(1) % 4 + 3;
        }
        Effect2C_Spawn(256);
        Actor_SetState(contest->npc, 0, NPC_STATE_DRINK_WAIT);
        CamReq_Minigame(DRINK_CAM_PLAY);
        Jingle_Play(pl, MINIGAME_JINGLE_START);
        break;
    case DRINK_MODE_SIT:
        if (contest->npc->motion[0].playing <= 0) {
            contest->mode++;
        }
        break;
    case DRINK_MODE_COUNTDOWN:
        if (--contest->wait < -9) {
            contest->mode++;
        }
        break;
    case DRINK_MODE_PLAY:
        if (contest->npc->motion[0].playing <= 0 && contest->npc->state == NPC_STATE_DRINK_WAIT && contest->npcMugs > 0) {
            Actor_SetState(contest->npc, 0, NPC_STATE_DRINK);
        }
        if (Motion_PassedFrame(contest->npc, 0, 250.0f) != 0 && --contest->npcMugs <= 0) {
            if (contest->playerMugs > 0) {
                contest->result = MINIGAME_LOSE;
                CamReq_Minigame(DRINK_CAM_LOSE);
                Actor_SetState(contest->npc, 0, NPC_STATE_CONTEST_WIN);
                Jingle_Play(pl, MINIGAME_JINGLE_LOSE);
                if (pl->state == TOWN_PC_SEAT_POSE_DOWN) {
                    contest->mode = DRINK_MODE_LOST;
                } else {
                    Actor_SetState(pl, 0, TOWN_PC_DRINK_LOSE);
                    contest->endWait = 45;
                    contest->mode = DRINK_MODE_END_WAIT;
                }
            } else if (contest->playerMugs <= 0) {
                if (Game_IsOnline() == 0) {
                    contest->result = MINIGAME_WIN;
                    CamReq_Minigame(DRINK_CAM_WIN);
                    Jingle_Play(pl, MINIGAME_JINGLE_WIN);
                    Actor_SetState(contest->npc, 0, NPC_STATE_CONTEST_LOSE);
                    Actor_SetState(pl, 0, TOWN_PC_DRINK_WIN);
                    contest->endWait = 45;
                    contest->mode = DRINK_MODE_END_WAIT;
                    contest->wait = 90;
                } else {
                    contest->result = MINIGAME_DRAW;
                    CamReq_Minigame(DRINK_CAM_DRAW);
                }
            }
        } else {
            switch (contest->playStep) {
            case DRINK_PLAY_KEYS:
                if (contest->playerMugs <= 0) {
                    if (contest->npcMugs > 0 || (contest->npcMugs <= 0 && Game_IsOnline() == 0)) {
                        contest->result = MINIGAME_WIN;
                        CamReq_Minigame(DRINK_CAM_WIN);
                        Jingle_Play(pl, MINIGAME_JINGLE_WIN);
                        Actor_SetState(contest->npc, 0, NPC_STATE_CONTEST_LOSE);
                        Actor_SetState(pl, 0, TOWN_PC_DRINK_WIN);
                    } else if (contest->npcMugs <= 0 && Game_IsOnline() == 1) {
                        contest->result = MINIGAME_DRAW;
                        CamReq_Minigame(DRINK_CAM_DRAW);
                    }
                    contest->endWait = 45;
                    contest->mode = DRINK_MODE_END_WAIT;
                    contest->wait = 90;
                    break;
                }
                if (contest->keysLeft <= 0 || contest->missed == 1) {
                    if (contest->missed == 1) {
                        contest->misses++;
                        if (contest->misses < 3) {
                            Actor_SetState(pl, 0, TOWN_PC_DRINK_MISS);
                            pl->motion[0].step = 2.4f;
                        }
                        contest->missed = 0;
                        contest->keysLeft = 0;
                        contest->inputOn = 0;
                        contest->playStep = DRINK_PLAY_DRINK;
                        break;
                    }
                    if (contest->hits >= contest->count) {
                        Actor_SetState(pl, 0, TOWN_PC_SEAT_DRINK);
                        if (contest->inputFrames <= 30.0f * contest->quickSecs) {
                            pl->motion[0].step = 3.4f;
                            pl->motion[1].step = 3.4f;
                        } else {
                            pl->motion[0].step = 2.0f;
                            pl->motion[1].step = 2.0f;
                        }
                        contest->playStep = DRINK_PLAY_DRINK;
                    }
                    if (contest->level >= 4) {
                        contest->count = contest->countTable[Rand_Next(1) & 0xF];
                    }
                    contest->keysShown = 1;
                    run = 0;
                    contest->inputFrames = 0;
                    contest->hits = 0;
                    contest->keyNext = 0;
                    contest->keysLeft += contest->count;
                    for (i = 0; i < contest->count; i++) {
                        if (run <= 0) {
                            if (contest->lastDir == DRINK_KEY_COUNT) {
                                contest->lastDir = Rand_Next(1) % DRINK_KEY_COUNT;
                            } else {
                                nextDir = contest->lastDir + 1 + (u8)(Rand_Next(1) % 3);
                                if (nextDir >= DRINK_KEY_COUNT) {
                                    contest->lastDir = nextDir - DRINK_KEY_COUNT;
                                } else {
                                    contest->lastDir = nextDir;
                                }
                            }
                            sum = 0;
                            run = 0;
                            for (j = 0; j < contest->count; j++) {
                                sum += gDrinkRunWeights[contest->level][j];
                            }
                            pick = Rand_Next(1) % sum + 1;
                            for (j = 0; j < contest->count; j++) {
                                pick -= gDrinkRunWeights[contest->level][j];
                                run++;
                                if (pick <= 0) {
                                    break;
                                }
                            }
                        }
                        contest->dir[i] = contest->lastDir;
                        contest->phase[i] = DRINK_KEY_WAITING;
                        run--;
                    }
                }
                if (contest->keyNext < contest->count && contest->inputOn == 1 && contest->playStep == DRINK_PLAY_KEYS) {
                    contest->inputFrames++;
                    pad = gPadState[0].pressed;
                    if (pad & PAD_CIRCLE) {
                        key = DRINK_KEY_CIRCLE;
                    } else if (pad & PAD_CROSS) {
                        key = DRINK_KEY_CROSS;
                    } else if (pad & PAD_SQUARE) {
                        key = DRINK_KEY_SQUARE;
                    } else if (pad & PAD_TRIANGLE) {
                        key = DRINK_KEY_TRIANGLE;
                    } else {
                        key = DRINK_KEY_NONE;
                    }
                    if (key == contest->dir[contest->keyNext]) {
                        contest->phase[contest->keyNext] = DRINK_KEY_HIT;
                        contest->timer[contest->keyNext] = 0;
                        contest->vx[contest->keyNext] = Rand_Next(1) % 21 - 10;
                        contest->vy[contest->keyNext] = -1.0f * (Rand_Next(1) % 11 + 20);
                        contest->scale[contest->keyNext] = 0.1f * (Rand_Next(1) % 15 + 6);
                        contest->hits++;
                        contest->keysLeft--;
                        contest->keyNext++;
                        switch (key) {
                        case DRINK_KEY_CIRCLE:
                            LobbySe_Play(LOBBY_SE_DRINK_CIRCLE);
                            break;
                        case DRINK_KEY_CROSS:
                            LobbySe_Play(LOBBY_SE_DRINK_CROSS);
                            break;
                        case DRINK_KEY_SQUARE:
                            LobbySe_Play(LOBBY_SE_DRINK_SQUARE);
                            break;
                        case DRINK_KEY_TRIANGLE:
                            LobbySe_Play(LOBBY_SE_DRINK_TRIANGLE);
                            break;
                        }
                    } else if (key != DRINK_KEY_NONE) {
                        contest->phase[contest->keyNext] = DRINK_KEY_MISSED;
                        contest->timer[contest->keyNext] = 0;
                        contest->missed = 1;
                        contest->keysLeft--;
                        contest->keyNext++;
                        LobbySe_Play(LOBBY_SE_DRINK_MISS);
                    }
                }
                break;
            case DRINK_PLAY_DRINK:
                if ((pl->motion[0].playing <= 0 && pl->state == TOWN_PC_DRINK_MISS) || (Motion_PassedFrame(pl, 0, 112.0f) != 0 && pl->state == TOWN_PC_SEAT_DRINK)) {
                    if (Motion_PassedFrame(pl, 0, 112.0f) != 0 && pl->state == TOWN_PC_SEAT_DRINK) {
                        contest->playerMugs--;
                        LobbySe_Play(LOBBY_SE_DRINK_MUG);
                    } else {
                        pl->motion[0].step = 2.0f;
                    }
                    contest->playStep = DRINK_PLAY_KEYS;
                    pl->motion[0].step = 2.0f;
                }
                if (contest->misses >= 3) {
                    Actor_SetState(pl, 0, TOWN_PC_SEAT_POSE_DOWN);
                    contest->downWait = 120;
                    contest->keysLeft = 0;
                    contest->hits = 0;
                    contest->misses = 0;
                    contest->missed = 0;
                    contest->down = 1;
                    contest->inputOn = 0;
                    contest->playStep = DRINK_PLAY_DOWN;
                }
                break;
            case DRINK_PLAY_DOWN:
                if (contest->downWait-- <= 0) {
                    Actor_SetState(pl, 0, TOWN_PC_SEAT_POSE_DOWN_END);
                    contest->down = 0;
                    contest->playStep = DRINK_PLAY_GET_UP;
                }
                break;
            case DRINK_PLAY_GET_UP:
                if (pl->motion[0].playing <= 0) {
                    contest->playStep = DRINK_PLAY_KEYS;
                }
                break;
            case DRINK_PLAY_KEYS_FLY:
                if (DrinkContest_KeysFlown() == 1) {
                    contest->playStep = DRINK_PLAY_KEYS;
                }
                break;
            }
        }
        break;
    case DRINK_MODE_LOST:
        if (contest->down == 1) {
            if (contest->downWait-- <= 0) {
                Actor_SetState(pl, 0, TOWN_PC_SEAT_POSE_DOWN_END);
                contest->down = 0;
            }
        } else if (pl->motion[0].playing <= 0) {
            Actor_SetState(pl, 0, TOWN_PC_DRINK_LOSE);
            contest->endWait = 45;
            contest->mode = DRINK_MODE_END_WAIT;
        }
        break;
    case DRINK_MODE_END_WAIT:
        if (contest->endWait-- < 0) {
            contest->mode = DRINK_MODE_CAMERA_WAIT;
        }
        break;
    case DRINK_MODE_CAMERA_WAIT:
        if (CamReq_State() != 1) {
            CamReq_Cancel();
            contest->mode = DRINK_MODE_RESULT;
            contest->wait = 60;
            if (contest->result == MINIGAME_WIN) {
                LobbySe_Play(LOBBY_SE_MINIGAME_WIN);
            }
        }
        break;
    case DRINK_MODE_RESULT:
        if (contest->wait-- <= 0) {
            switch (contest->result) {
            case MINIGAME_LOSE:
                Actor_SetState(pl, 0, TOWN_PC_DRINK_LOSE);
                pl->drunk = 1;
                contest->mode++;
                break;
            case MINIGAME_WIN:
                if (pl->motion[0].playing <= 0) {
                    Actor_SetState(pl, 0, TOWN_PC_DRINK_WIN);
                    contest->mode++;
                }
                break;
            case MINIGAME_DRAW:
                Bgm_StopEventFade(pl);
                Actor_SetMotion(pl, 609, 6, 0);
                Actor_SetState(pl, 0, TOWN_PC_SIT_DOWN_SEATED);
                contest->mode++;
                break;
            }
        }
        break;
    case DRINK_MODE_RESULT_TALK:
        if (contest->npc->motion[0].playing <= 0) {
            switch (contest->result) {
            case MINIGAME_LOSE:
                Talk_Say(13, 0);
                break;
            case MINIGAME_WIN:
                Talk_Say(15, 0);
                break;
            }
            contest->mode = DRINK_MODE_DONE;
            contest->running = 0;
        }
        break;
    case DRINK_MODE_DONE:
        Actor_SetState(contest->npc, 0, *(u16 *)&work->route[work->routeIdx].state);
        return 1;
    }
    if (contest->npc->state == NPC_STATE_CONTEST_WIN && contest->npc->motion[0].playing <= 0) {
        Actor_SetState(contest->npc, 0, *(u16 *)&work->route[work->routeIdx].state);
    }
    return 0;
}

extern char *gDrinkMenuTexts[];
extern f32 gDrinkScaleSteps[];
/*
 * Draws the drinking contest (the menu task's draw): the opening menu, then
 * during the rounds the mug counts and the row of keys. A waiting key sits
 * at its place; a missed one shakes, then greys; a hit one flashes and
 * flies off, and once the last one is hit the round waits for them all
 * (DRINK_PLAY_KEYS_FLY). Then the result's banner.
 */
void DrinkContest_Draw(MenuTask *task)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    DrinkContest *contest = (DrinkContest *)&gMenuWork;
    f32 keyX;
    f32 scaleY;
    f32 scaleX;
    f32 keyY;
    u8 i;
    u32 rowX;
    u32 color;

    Text_SetSize(task->textSize);
    switch (contest->step) {
    case DRINK_STEP_MENU:
        Menu_DrawLabelList(400, gDrinkMenuTexts, 2, contest->cursor, 0xFFB0B000);
        break;
    }
    switch (contest->mode) {
    case DRINK_MODE_LOST:
    case DRINK_MODE_END_WAIT:
    case DRINK_MODE_PLAY:
        DrinkContest_DrawCounts();
        if (contest->playerMugs > 0 && contest->keysShown == 1 && (contest->playStep != DRINK_PLAY_DRINK || pl->state != TOWN_PC_SEAT_DRINK)) {
            contest->inputOn = 1;
            rowX = 320.0f - (u32)(24.0f + 30.0f * (contest->count - 1)) / 2;
            for (i = 0; i < contest->count; i++) {
                Font_SetSpacing(30, 30);
                switch (contest->phase[i]) {
                case DRINK_KEY_MISSED:
                    scaleY = 1.5f;
                    scaleX = scaleY;
                    keyX = 30.0f * i + rowX - 12.0f * (scaleX - 1.0f);
                    keyY = 120.0f - 12.0f * (scaleY - 1.0f);
                    if (contest->timer[i] >= 50) {
                        color = 0xFF969696;
                        contest->timer[i] = 50;
                    } else {
                        color = 0xFFFFFFFF;
                        if (DrinkContest_ShakeKey(&contest->shakeArgA, &contest->shakeVel, &keyX, contest->timer[i], &contest->shakeArgB) == 0) {
                            contest->timer[i] = 50;
                        }
                        contest->timer[i]++;
                    }
                    break;
                case DRINK_KEY_HIT:
                    if (contest->timer[i] == 0) {
                        contest->x[i] = 30.0f * i + rowX - 12.0f * (scaleX - 1.0f);
                        contest->y[i] = 120.0f - 12.0f * (scaleY - 1.0f);
                        contest->frame[i] = 0;
                    }
                    if (contest->timer[i] >= 20) {
                        color = 0xFFFF00;
                        contest->timer[i] = 20;
                    } else {
                        color = 0xFFFFFFFF;
                        if (DrinkContest_FlyKey(&contest->x[i], &contest->y[i], &contest->vx[i], &contest->vy[i], &contest->frame[i], contest->timer[i]) == 0) {
                            contest->timer[i] = 20;
                        }
                        DrinkContest_DrawKeyFlash(255, 255, 0, contest->timer[i], 30.0f * i + rowX, 120.0f);
                        contest->timer[i]++;
                        if (i == contest->count - 1) {
                            contest->playStep = DRINK_PLAY_KEYS_FLY;
                        }
                        scaleY = contest->scale[i];
                        scaleX = scaleY * gDrinkScaleSteps[contest->frame[i]];
                        keyX = contest->x[i] - 12.0f * (scaleX - 1.0f);
                        keyY = contest->y[i] - 12.0f * (scaleY - 1.0f);
                    }
                    break;
                case DRINK_KEY_WAITING:
                    scaleY = 1.0f;
                    scaleX = scaleY;
                    keyX = 30.0f * i + rowX - 12.0f * (scaleX - 1.0f);
                    keyY = 120.0f - 12.0f * (scaleY - 1.0f);
                    color = 0xFFFFFFFF;
                    DrinkContest_DrawKeyFlash(255, 255, 0, 0, 30.0f * i + rowX, keyY);
                    break;
                }
                switch (contest->dir[i]) {
                case DRINK_KEY_CIRCLE:
                    DrinkContest_DrawButton(BUTTON_ICON_CIRCLE, keyX, keyY, TEX_SLOT_ICONS, color, scaleY, scaleX);
                    break;
                case DRINK_KEY_CROSS:
                    DrinkContest_DrawButton(BUTTON_ICON_CROSS, keyX, keyY, TEX_SLOT_ICONS, color, scaleY, scaleX);
                    break;
                case DRINK_KEY_SQUARE:
                    DrinkContest_DrawButton(BUTTON_ICON_SQUARE, keyX, keyY, TEX_SLOT_ICONS, color, scaleY, scaleX);
                    break;
                case DRINK_KEY_TRIANGLE:
                    DrinkContest_DrawButton(BUTTON_ICON_TRIANGLE, keyX, keyY, TEX_SLOT_ICONS, color, scaleY, scaleX);
                    break;
                }
            }
        }
        break;
    case DRINK_MODE_RESULT:
        Minigame_DrawResult(contest->result, contest->wait, 0);
        break;
    }
}

extern FloatKey gDrinkHitSizeKeys[];
extern FloatKey gDrinkHitAlphaKeys[];
/* Draws the flash of a key at x, y in the colour red, green, blue, its size
 * and alpha at frame `frame` of gDrinkHitSizeKeys and gDrinkHitAlphaKeys,
 * centred on the key's 26-pixel cell. */
void DrinkContest_DrawKeyFlash(u32 red, u32 green, u32 blue, u32 frame, f32 x, f32 y)
{
    f32 size;
    f32 alpha;
    Sprite spr;
    f32 off;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, TEX_SLOT_MINIGAME);
    Tex_Bind(TEX_SLOT_MINIGAME);
    spr.v0 = 16;
    spr.u0 = 32;
    spr.u1 = 48;
    spr.v1 = 32;
    Key_LerpFloat((f32 *) gDrinkHitAlphaKeys, (f32 *) &alpha, frame);
    spr.color = blue | ((green << 8) | (((u8)(u32)(255.0f * alpha) << 24) | (red << 16)));
    Key_LerpFloat((f32 *) gDrinkHitSizeKeys, (f32 *) &size, frame);
    off = (size - 26.0f) / 2.0f;
    spr.h = spr.w = size;
    spr.y = y - off;
    spr.x = x - off;
    Sprite_Draw(&spr);
}

/* Moves a hit key one frame along its flight: gravity pulls vy down the
 * screen and vx slows; with animate set its frame steps through the four of
 * gDrinkScaleSteps. Returns 0 once it leaves the screen, 1 before. */
s32 DrinkContest_FlyKey(f32 *x, f32 *y, s32 *vx, s32 *vy, u8 *frame, u8 animate)
{
    f32 keyX;
    f32 keyY;
    s32 speedX;

    *vy += 5;
    speedX = *vx;
    if (speedX >= 5) {
        *vx = speedX - 5;
    }
    *x += (f32) *vx;
    *y += (f32) *vy;
    if ((animate & 0xFF) > 0) {
        *frame += 1;
        *frame = (u8) ((s32) *frame % 4);
    }
    keyX = *x;
    if (!(18.0f < keyX) || !(keyX < 620.0f) || (keyY = *y, (keyY <= 28.0f)) || !(keyY < 422.0f)) {
        return 0;
    }
    return 1;
}

/* Shakes a missed key: moves *pos by 3 * *vel and reverses *vel each frame,
 * one step smaller every other frame; bounce 0 (its first frame)
 * starts it at 3. Returns 0 once the shake has died out, 1 before. arg
 * and unused aren't read. */
s32 DrinkContest_ShakeKey(void *arg, s8 *vel, f32 *pos, u8 bounce, u8 *unused)
{
    s8 mag;
    s8 dir;

    if (bounce == 0) {
        *vel = 3;
        *pos += 3.0f * *vel;
        return 1;
    }
    *vel *= -1.0f;
    *pos += 3.0f * *vel;
    mag = Math_Absf(*vel);
    dir = *vel / mag;
    if (dir <= 0) {
        if (--mag <= 0) {
            return 0;
        }
    }
    *vel = mag * dir;
    return 1;
}

/* 1 once every key of the round has finished its flight (timer 20). */
s32 DrinkContest_KeysFlown(void)
{
    DrinkContest *contest = (DrinkContest *)&gMenuWork;
    u8 i;

    for (i = 0; i < contest->count; i++) {
        if (contest->timer[i] < 20) {
            return 0;
        }
    }
    return 1;
}

struct TexRect;
/*
 * Draws pad button icon id of the icon sheet at x, y, scaled by scaleX and
 * scaleY: the lobby's copy of Hud_DrawButtonIcon, with a scale for each axis.
 * A cell is 24x24 (32 wide for 8 and 9); 22-24 are the arrow of cell 16,
 * mirrored or turned a quarter turn either way, and 20 and 21 use cells 0
 * and 1. depth isn't read.
 */
void DrinkContest_DrawButton(s16 id, s16 x, s16 y, s32 depth, u32 color, f32 scaleY, f32 scaleX)
{
    Sprite spr;
    f32 size;
    s16 asked = id;

    if ((u32)(asked - 22) <= 1 || asked == 24) {
        id = 16;
    }
    if (id == 8 || id == 9) {
        size = 32.0f;
    } else {
        size = 24.0f;
    }
    if (id == 20) {
        id = 0;
    } else if (id == 21) {
        id = 1;
    }
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    spr.x = x;
    spr.y = y;
    spr.color = color;
    spr.w = size * scaleX;
    spr.h = 24.0f * scaleY;
    if (id == 9) {
        spr.u0 = 224;
    } else {
        spr.u0 = (id % 10) * 24;
    }
    spr.v0 = (id / 10) * 24;
    spr.u1 = spr.u0 + (s16)size + 1;
    if (id == 18 || id == 17 || id == 19) {
        spr.v1 = spr.v0 + 24;
    } else {
        spr.v1 = spr.v0 + 25;
    }
    switch (asked) {
    case 22: {
        s16 left = spr.u1 - 1;
        spr.u1 = spr.u0;
        spr.u0 = left;
        Sprite_Draw(&spr);
        break;
    }
    case 23:
        spr.u1 -= 1;
        TexRect_DrawRotated((struct TexRect *)&spr, ANGLE_TURN / 4);
        break;
    case 24:
        spr.u1 -= 1;
        TexRect_DrawRotated((struct TexRect *)&spr, ANGLE_TURN * 3 / 4);
        break;
    default:
        Sprite_Draw(&spr);
        break;
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
}

extern u8 gDrinkCountsWins0[];
extern u8 gDrinkCountsWins5[];
extern u8 gDrinkCountsWins10[];
extern u8 gDrinkCountsWins20[];
extern u8 gDrinkCountsWins40[];
/* 1 while the drinking contest runs (DrinkContest.running). */
s32 DrinkContest_IsOn(void)
{
    return ((DrinkContest *)&gMenuWork)->running == 1;
}

/* The key counts of the rounds from level 4 (DrinkContest.countTable), by
 * the drinking contests won so far: more wins, more keys. */
u8 *DrinkContest_GetLevel(void)
{
    u16 wins;

    wins = gSave.recordCounts[RECORD_DRINK_WINS];
    if ((s32) wins < 5) {
        return gDrinkCountsWins0;
    }
    if ((s32) wins < 10) {
        return gDrinkCountsWins5;
    }
    if ((s32) wins < 20) {
        return gDrinkCountsWins10;
    }
    if ((s32) wins < 40) {
        return gDrinkCountsWins20;
    }
    return gDrinkCountsWins40;
}

/* The contest's tables: the key counts, the runs' weights, the quick
 * rounds' times, the flash's key frames and the talk's lines. */
extern char gDrinkMenuRulesText[];
u8 gDrinkKeyCounts[16] = {
#include "lobby/007EC050.inc"
};
f32 gDrinkScaleSteps[4] = {
#include "lobby/007EC060.inc"
};
s8 gDrinkRunWeights[6][7] = {
#include "lobby/007EC070.inc"
};
u8 gDrinkCountsWins0[16] = {
#include "lobby/007EC0A0.inc"
};
u8 gDrinkCountsWins5[16] = {
#include "lobby/007EC0B0.inc"
};
u8 gDrinkCountsWins10[16] = {
#include "lobby/007EC0C0.inc"
};
u8 gDrinkCountsWins20[16] = {
#include "lobby/007EC0D0.inc"
};
u8 gDrinkCountsWins40[16] = {
#include "lobby/007EC0E0.inc"
};
FloatKey gDrinkHitSizeKeys[4] = {
#include "lobby/007EC0F0.inc"
};
FloatKey gDrinkHitAlphaKeys[4] = {
#include "lobby/007EC110.inc"
};
f32 gDrinkQuickSecs[6] = {
#include "lobby/007EC130.inc"
};
s8 gDrinkForecastLines[8] = {
#include "lobby/007EC148.inc"
};
s32 gDrinkHpLines[6] = {
#include "lobby/007EC150.inc"
};
char *gDrinkMenuTexts[2] = {
    gDrinkMenuPlayText, gDrinkMenuRulesText,
};
