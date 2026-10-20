#include "common.h"
#include "sprite.h"
#include "player.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "pad.h"
#include "lobby_npc.h"
#include "lobby_menu.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

extern u8 gMenuWork;

/* prototypes */
/* end of prototypes */

u16 LobbyPad_Get(s32 a);
s32 Menu_WrapUpDown(s32 cur, s32 n);
void Talk_Say(s32 a, s8 b);
s32 Talk_Update(void);
void LobbySe_Play(s32);
s32 Minigame_Update(void);
s32 DrinkMug_IsBack(void);
void Actor_SetStateNow(Player *, u8, u8, u16);
void CamFocus_End(void);
s32 Award_Grant(s32 arg0);
void DrinkContest_Close(void);
u8 PersistentEm_LastKind(void);
s32 DrinkContest_HpStage(void);
extern s8 gDrinkTalkLines[];
extern s32 gDrinkHpTalkLines[];
/* unk_6F8CC0's .rodata (0x8382D0-0x838310) */
#pragma rodata on
char D_008382D0[0x20] = {
#include "lobby/008382D0.inc"
};
char D_008382F0[0x20] = {
#include "lobby/008382F0.inc"
};
#pragma rodata off

void DrinkContest_Update(void)
{
    Minigame *m = (Minigame *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);
    u8 r;

    switch (m->step) {
    case 0:
        if (pad & 0x3000) {
            m->cursor = Menu_WrapUpDown(m->cursor, 2);
        }
        if (m->menuWait++ >= 6) {
            m->menuWait = 6;
        }
        if (m->menuWait >= 6 && (pad & 0x20)) {
            switch (m->cursor) {
            case 0:
                Talk_Say(0xC, 0);
                m->step++;
                break;
            case 1:
                Talk_Say(5, 0);
                m->menuWait = 0;
                m->talkNext = 0;
                m->step = 4;
                break;
            }
            LobbySe_Play(8);
        }
        if (pad & 0x40) {
            Talk_Say(1, 0);
            m->step = 4;
            m->talkNext = 3;
            LobbySe_Play(4);
        }
        break;
    case 1:
        if (Talk_Update() == 0) {
            m->running = 1;
            m->step++;
        }
        break;
    case 2:
        if (Minigame_Update() == 1) {
            if (m->result == 0) {
                m->talkNext = 5;
                if (gSave.recordCounts[35] < 0xFDE8) {
                    (gSave.recordCounts[35])++;
                    if (gSave.recordCounts[35] >= 0x32) {
                        Award_Grant(0x25);
                    }
                }
            } else {
                m->talkNext = 3;
            }
            m->step = 4;
        }
        break;
    case 3:
        if (DrinkMug_IsBack() == 1) {
            Actor_SetStateNow(m->npc, 0, 0x73, 0);
            CamFocus_End();
            m->step = 8;
        }
        break;
    case 8:
        DrinkContest_Close();
        break;
    case 4:
        if (Talk_Update() == 0) {
            switch (m->talkNext) {
            case 3:
            default:
                m->step = m->talkNext;
                break;
            case 5:
                gElderForecastHeard = 1;
                switch (PersistentEm_LastKind()) {
                case 0x36:
                default:
                    r = 0;
                    break;
                case 0x40:
                    r = 1;
                    break;
                case 0x41:
                    r = 2;
                    break;
                case 0x3B:
                    r = 3;
                    break;
                }
                Talk_Say(gDrinkTalkLines[r], 0);
                m->talkNext = 6;
                break;
            case 6:
                Talk_Say(gDrinkHpTalkLines[DrinkContest_HpStage()], 0);
                m->talkNext = 7;
                break;
            case 7:
                Talk_Say(0x20, 0);
                m->talkNext = 3;
                break;
            }
        }
        break;
    }
}

void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
s32 Sprite_Draw(Sprite *);
void Tex_Prepare(s32 n, s32 tex);
void Gfx_SetTexFilter(s32 a);
void Tex_Bind(s32 a);
void DrinkContest_DrawCounts(void)
{
    Sprite a[10];
    Sprite b[10];
    Minigame *m = (Minigame *)&gMenuWork;
    s16 off;
    u8 i;

    off = (gFrameTick & 0x10) ? 0 : 6;
    Icon_Draw(140.0f + off, 0x2C, 0x18, 0x18, 0xFF20C0C0, 0x29);
    Icon_Draw(483.0f - off, 0x2C, 0x18, 0x18, 0xFF20C0C0, 0x28);
    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x168);
    Tex_Bind(0x168);
    for (i = 0; i < 10; i++) {
        a[i].u0 = 0x60;
        a[i].v0 = 0x60;
        a[i].u1 = 0x80;
        a[i].v1 = 0x80;
        if (i < 10 - m->playerMugs) {
            a[i].color = 0xFFFFFF00;
        } else {
            a[i].color = 0xB2505050;
        }
        a[i].x = 170.0f + 30.0f * i;
        a[i].y = 0x28;
        a[i].w = 0x20;
        a[i].h = 0x20;
        Sprite_Draw(&a[i]);
    }
    for (i = 0; i < 10; i++) {
        b[i].u0 = 0x60;
        b[i].v0 = 0x40;
        b[i].u1 = 0x80;
        b[i].v1 = 0x60;
        if (i < 10 - m->npcMugs) {
            b[i].color = 0xFFFFFFFF;
        } else {
            b[i].color = 0xB2505050;
        }
        b[i].x = 170.0f + 30.0f * i;
        b[i].y = 0x50;
        b[i].w = 0x20;
        b[i].h = 0x20;
        Sprite_Draw(&b[i]);
    }
}

extern f32 gMinigameLevelC8[];
extern u8 gMinigameRollCounts[];
extern s8 gMinigameRollWeights[][7];
u16 Rand_Next(s32 stream);
void Effect2C_Spawn(s16 n);
void Actor_SetState(void *, s32, s32);
void CamReq_Minigame(s32 a);
void Jingle_Play(void *obj, s32 id);
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
s32 Game_IsOnline(void);
s32 DrinkContest_AllDone(void);
s32 CamReq_State(void);
void CamReq_Cancel(void);
void Bgm_StopEventFade(Player *p);
void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
#define NPC ((Player *)m->npc)
#define P154(p) (*(s32 *)((u8 *)(p) + 0x154))
s32 Minigame_Update(void)
{
    Minigame *m = (Minigame *)&gMenuWork;
    u8 key;
    Player *pl = &gPlayers[gScene.localPlayer];
    s8 r;
    u8 j;
    u16 pad;
    s8 run;
    s8 sum;
    u8 i;
    u8 v;
    NpcWork *q = (NpcWork *)((u8 *)m->npc + 0x438);

    switch (m->mode) {
    case 0:
        m->mode++;
        m->wait = 0x5A;
        m->playerMugs = 10;
        m->npcMugs = 10;
        m->hits = 0;
        m->keysLeft = 0;
        m->down = 0;
        m->result = 2;
        m->lastDir = 4;
        m->inputOn = 0;
        m->missed = 0;
        m->misses = 0;
        m->inputFrames = 0;
        m->quickSecs = 0.0f;
        m->unkCD = 0;
        m->shakeVel = 0;
        m->keysShown = 0;
        m->unkD0 = 9;
        m->timer[0] = 0x14;
        m->phase[0] = 2;
        m->timer[1] = 0x14;
        m->phase[1] = 2;
        m->timer[2] = 0x14;
        m->phase[2] = 2;
        m->timer[3] = 0x14;
        m->phase[3] = 2;
        m->timer[4] = 0x14;
        m->phase[4] = 2;
        m->timer[5] = 0x14;
        m->phase[5] = 2;
        m->timer[6] = 0x14;
        m->phase[6] = 2;
        m->quickSecs = gMinigameLevelC8[m->level];
        if (m->level < 4) {
            m->count = gMinigameRollCounts[m->level];
        } else {
            m->count = Rand_Next(1) % 4 + 3;
        }
        Effect2C_Spawn(0x100);
        Actor_SetState(m->npc, 0, 0x85);
        CamReq_Minigame(0);
        Jingle_Play(pl, 0x25);
        break;
    case 1:
        if (P154(NPC) <= 0) {
            m->mode++;
        }
        break;
    case 2:
        if (--m->wait < -9) {
            m->mode++;
        }
        break;
    case 3:
        if (P154(NPC) <= 0 && NPC->state == 0x85 && m->npcMugs > 0) {
            Actor_SetState(m->npc, 0, 0x86);
        }
        if (Motion_PassedFrame(m->npc, 0, 250.0f) != 0 && --m->npcMugs <= 0) {
            if (m->playerMugs > 0) {
                m->result = 1;
                CamReq_Minigame(2);
                Actor_SetState(m->npc, 0, 0x75);
                Jingle_Play(pl, 0x27);
                if (pl->state == 0x26) {
                    m->mode = 4;
                } else {
                    Actor_SetState(pl, 0, 0x90);
                    m->endWait = 0x2D;
                    m->mode = 5;
                }
            } else if (m->playerMugs <= 0) {
                if (Game_IsOnline() == 0) {
                    m->result = 0;
                    CamReq_Minigame(1);
                    Jingle_Play(pl, 0x26);
                    Actor_SetState(m->npc, 0, 0x74);
                    Actor_SetState(pl, 0, 0x91);
                    m->endWait = 0x2D;
                    m->mode = 5;
                    m->wait = 0x5A;
                } else {
                    m->result = 2;
                    CamReq_Minigame(3);
                }
            }
        } else {
            switch (m->playStep) {
            case 0:
                if (m->playerMugs <= 0) {
                    if (m->npcMugs > 0 || (m->npcMugs <= 0 && Game_IsOnline() == 0)) {
                        m->result = 0;
                        CamReq_Minigame(1);
                        Jingle_Play(pl, 0x26);
                        Actor_SetState(m->npc, 0, 0x74);
                        Actor_SetState(pl, 0, 0x91);
                    } else if (m->npcMugs <= 0 && Game_IsOnline() == 1) {
                        m->result = 2;
                        CamReq_Minigame(3);
                    }
                    m->endWait = 0x2D;
                    m->mode = 5;
                    m->wait = 0x5A;
                    break;
                }
                if (m->keysLeft <= 0 || m->missed == 1) {
                    if (m->missed == 1) {
                        m->misses++;
                        if (m->misses < 3) {
                            Actor_SetState(pl, 0, 0x8D);
                            pl->motion[0].step = 2.4f;
                        }
                        m->missed = 0;
                        m->keysLeft = 0;
                        m->inputOn = 0;
                        m->playStep = 1;
                        break;
                    }
                    if (m->hits >= m->count) {
                        Actor_SetState(pl, 0, 0x21);
                        if (m->inputFrames <= 30.0f * m->quickSecs) {
                            pl->motion[0].step = 3.4f;
                            pl->motion[1].step = 3.4f;
                        } else {
                            pl->motion[0].step = 2.0f;
                            pl->motion[1].step = 2.0f;
                        }
                        m->playStep = 1;
                    }
                    if (m->level >= 4) {
                        m->count = m->countTable[Rand_Next(1) & 0xF];
                    }
                    m->keysShown = 1;
                    run = 0;
                    m->inputFrames = 0;
                    m->hits = 0;
                    m->keyNext = 0;
                    m->keysLeft += m->count;
                    for (i = 0; i < m->count; i++) {
                        if (run <= 0) {
                            if (m->lastDir == 4) {
                                m->lastDir = Rand_Next(1) % 4;
                            } else {
                                v = m->lastDir + 1 + (u8)(Rand_Next(1) % 3);
                                if (v >= 4) {
                                    m->lastDir = v - 4;
                                } else {
                                    m->lastDir = v;
                                }
                            }
                            sum = 0;
                            run = 0;
                            for (j = 0; j < m->count; j++) {
                                sum += gMinigameRollWeights[m->level][j];
                            }
                            r = Rand_Next(1) % sum + 1;
                            for (j = 0; j < m->count; j++) {
                                r -= gMinigameRollWeights[m->level][j];
                                run++;
                                if (r <= 0) {
                                    break;
                                }
                            }
                        }
                        m->dir[i] = m->lastDir;
                        m->phase[i] = 2;
                        run--;
                    }
                }
                if (m->keyNext < m->count && m->inputOn == 1 && m->playStep == 0) {
                    m->inputFrames++;
                    pad = gPadState[0].pressed;
                    if (pad & 0x20) {
                        key = 0;
                    } else if (pad & 0x40) {
                        key = 1;
                    } else if (pad & 0x200) {
                        key = 2;
                    } else if (pad & 0x100) {
                        key = 3;
                    } else {
                        key = 99;
                    }
                    if (key == m->dir[m->keyNext]) {
                        m->phase[m->keyNext] = 1;
                        m->timer[m->keyNext] = 0;
                        m->vx[m->keyNext] = Rand_Next(1) % 21 - 10;
                        m->vy[m->keyNext] = -1.0f * (Rand_Next(1) % 11 + 20);
                        m->scale[m->keyNext] = 0.1f * (Rand_Next(1) % 15 + 6);
                        m->hits++;
                        m->keysLeft--;
                        m->keyNext++;
                        switch (key) {
                        case 0:
                            LobbySe_Play(0x31);
                            break;
                        case 1:
                            LobbySe_Play(0x32);
                            break;
                        case 2:
                            LobbySe_Play(0x33);
                            break;
                        case 3:
                            LobbySe_Play(0x34);
                            break;
                        }
                    } else if (key != 99) {
                        m->phase[m->keyNext] = 0;
                        m->timer[m->keyNext] = 0;
                        m->missed = 1;
                        m->keysLeft--;
                        m->keyNext++;
                        LobbySe_Play(0x35);
                    }
                }
                break;
            case 1:
                if ((P154(pl) <= 0 && pl->state == 0x8D) || (Motion_PassedFrame(pl, 0, 112.0f) != 0 && pl->state == 0x21)) {
                    if (Motion_PassedFrame(pl, 0, 112.0f) != 0 && pl->state == 0x21) {
                        m->playerMugs--;
                        LobbySe_Play(0x36);
                    } else {
                        pl->motion[0].step = 2.0f;
                    }
                    m->playStep = 0;
                    pl->motion[0].step = 2.0f;
                }
                if (m->misses >= 3) {
                    Actor_SetState(pl, 0, 0x26);
                    m->downWait = 0x78;
                    m->keysLeft = 0;
                    m->hits = 0;
                    m->misses = 0;
                    m->missed = 0;
                    m->down = 1;
                    m->inputOn = 0;
                    m->playStep = 2;
                }
                break;
            case 2:
                if (m->downWait-- <= 0) {
                    Actor_SetState(pl, 0, 7);
                    m->down = 0;
                    m->playStep = 3;
                }
                break;
            case 3:
                if (P154(pl) <= 0) {
                    m->playStep = 0;
                }
                break;
            case 4:
                if (DrinkContest_AllDone() == 1) {
                    m->playStep = 0;
                }
                break;
            }
        }
        break;
    case 4:
        if (m->down == 1) {
            if (m->downWait-- <= 0) {
                Actor_SetState(pl, 0, 7);
                m->down = 0;
            }
        } else if (P154(pl) <= 0) {
            Actor_SetState(pl, 0, 0x90);
            m->endWait = 0x2D;
            m->mode = 5;
        }
        break;
    case 5:
        if (m->endWait-- < 0) {
            m->mode = 6;
        }
        break;
    case 6:
        if (CamReq_State() != 1) {
            CamReq_Cancel();
            m->mode = 7;
            m->wait = 0x3C;
            if (m->result == 0) {
                LobbySe_Play(0x3D);
            }
        }
        break;
    case 7:
        if (m->wait-- <= 0) {
            switch (m->result) {
            case 1:
                Actor_SetState(pl, 0, 0x90);
                pl->drunk = 1;
                m->mode++;
                break;
            case 0:
                if (P154(pl) <= 0) {
                    Actor_SetState(pl, 0, 0x91);
                    m->mode++;
                }
                break;
            case 2:
                Bgm_StopEventFade(pl);
                Actor_SetMotion(pl, 0x261, 6, 0);
                Actor_SetState(pl, 0, 0x25);
                m->mode++;
                break;
            }
        }
        break;
    case 8:
        if (P154(NPC) <= 0) {
            switch (m->result) {
            case 1:
                Talk_Say(0xD, 0);
                break;
            case 0:
                Talk_Say(0xF, 0);
                break;
            }
            m->mode = 9;
            m->running = 0;
        }
        break;
    case 9:
        Actor_SetState(m->npc, 0, *(u16 *)&q->route[q->routeIdx].state);
        return 1;
    }
    if (NPC->state == 0x75 && P154(NPC) <= 0) {
        Actor_SetState(m->npc, 0, *(u16 *)&q->route[q->routeIdx].state);
    }
    return 0;
}

void Text_SetSize(s32 size);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
void Font_SetSpacing(u8 a, u8 b);
void Minigame_DrawResult(s8 kind, s16 t, s8 flag);
void DrinkContest_DrawCounts(void);
void DrinkContest_DrawHit(u32 r, u32 g, u32 b, u32 t, f32 x, f32 y);
s32 DrinkContest_FlyIcon(f32 *arg0, f32 *arg1, s32 *arg2, s32 *arg3, u8 *arg4, u8 arg5);
s32 DrinkContest_ShakeIcon(void *o, s8 *v, f32 *p, u8 bounce, u8 *unused);
void DrinkContest_DrawIcon(s16 kind, s16 x, s16 y, s32 tex, u32 color, f32 sy, f32 sx);
extern char *gDrinkContestHelpTexts[];
extern f32 gDrinkScaleSteps[];
void DrinkContest_Draw(void *t)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    Minigame *m = (Minigame *)&gMenuWork;
    f32 px;
    f32 sy;
    f32 sx;
    f32 py;
    u8 i;
    u32 x0;
    u32 color;

    Text_SetSize(*(s32 *)((u8 *)t + 0x18));
    switch (m->step) {
    case 0:
        Menu_DrawLabelList(0x190, (char **) (((u8 *)gDrinkContestHelpTexts)), 2, m->cursor, 0xFFB0B000);
        break;
    }
    switch (m->mode) {
    case 4:
    case 5:
    case 3:
        DrinkContest_DrawCounts();
        if (m->playerMugs > 0 && m->keysShown == 1 && (m->playStep != 1 || pl->state != 0x21)) {
            m->inputOn = 1;
            x0 = 320.0f - (u32)(24.0f + 30.0f * (m->count - 1)) / 2;
            for (i = 0; i < m->count; i++) {
                Font_SetSpacing(0x1E, 0x1E);
                switch (m->phase[i]) {
                case 0:
                    sy = 1.5f;
                    sx = sy;
                    px = 30.0f * i + x0 - 12.0f * (sx - 1.0f);
                    py = 120.0f - 12.0f * (sy - 1.0f);
                    if (m->timer[i] >= 0x32) {
                        color = 0xFF969696;
                        m->timer[i] = 0x32;
                    } else {
                        color = 0xFFFFFFFF;
                        if (DrinkContest_ShakeIcon(&m->unkCD, &m->shakeVel, &px, m->timer[i], &m->unk7) == 0) {
                            m->timer[i] = 0x32;
                        }
                        m->timer[i]++;
                    }
                    break;
                case 1:
                    if (m->timer[i] == 0) {
                        m->x[i] = 30.0f * i + x0 - 12.0f * (sx - 1.0f);
                        m->y[i] = 120.0f - 12.0f * (sy - 1.0f);
                        m->frame[i] = 0;
                    }
                    if (m->timer[i] >= 0x14) {
                        color = 0xFFFF00;
                        m->timer[i] = 0x14;
                    } else {
                        color = 0xFFFFFFFF;
                        if (DrinkContest_FlyIcon(&m->x[i], &m->y[i], &m->vx[i], &m->vy[i], &m->frame[i], m->timer[i]) == 0) {
                            m->timer[i] = 0x14;
                        }
                        DrinkContest_DrawHit(0xFF, 0xFF, 0, m->timer[i], 30.0f * i + x0, 120.0f);
                        m->timer[i]++;
                        if (i == m->count - 1) {
                            m->playStep = 4;
                        }
                        sy = m->scale[i];
                        sx = sy * gDrinkScaleSteps[m->frame[i]];
                        px = m->x[i] - 12.0f * (sx - 1.0f);
                        py = m->y[i] - 12.0f * (sy - 1.0f);
                    }
                    break;
                case 2:
                    sy = 1.0f;
                    sx = sy;
                    px = 30.0f * i + x0 - 12.0f * (sx - 1.0f);
                    py = 120.0f - 12.0f * (sy - 1.0f);
                    color = 0xFFFFFFFF;
                    DrinkContest_DrawHit(0xFF, 0xFF, 0, 0, 30.0f * i + x0, py);
                    break;
                }
                switch (m->dir[i]) {
                case 0:
                    DrinkContest_DrawIcon(0, px, py, 0x155, color, sy, sx);
                    break;
                case 1:
                    DrinkContest_DrawIcon(1, px, py, 0x155, color, sy, sx);
                    break;
                case 2:
                    DrinkContest_DrawIcon(3, px, py, 0x155, color, sy, sx);
                    break;
                case 3:
                    DrinkContest_DrawIcon(2, px, py, 0x155, color, sy, sx);
                    break;
                }
            }
        }
        break;
    case 7:
        Minigame_DrawResult(m->result, m->wait, 0);
        break;
    }
}

void Key_LerpFloat(f32 *k, f32 *out, f32 t);
extern u8 gDrinkHitSizeKeys[];
extern u8 gDrinkHitAlphaKeys[];
void DrinkContest_DrawHit(u32 r, u32 g, u32 b, u32 t, f32 x, f32 y)
{
    f32 size;
    f32 alpha;
    Sprite s;
    f32 off;

    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x168);
    Tex_Bind(0x168);
    s.v0 = 0x10;
    s.u0 = 0x20;
    s.u1 = 0x30;
    s.v1 = 0x20;
    Key_LerpFloat((f32 *) gDrinkHitAlphaKeys, (f32 *) &alpha, t);
    s.color = b | ((g << 8) | (((u8)(u32)(255.0f * alpha) << 24) | (r << 16)));
    Key_LerpFloat((f32 *) gDrinkHitSizeKeys, (f32 *) &size, t);
    off = (size - 26.0f) / 2.0f;
    s.h = s.w = size;
    s.y = y - off;
    s.x = x - off;
    Sprite_Draw(&s);
}

s32 DrinkContest_FlyIcon(f32 *x, f32 *y, s32 *vx, s32 *vy, u8 *frame, u8 animate)
{
    f32 fx;
    f32 fy;
    s32 v;

    *vy += 5;
    v = *vx;
    if (v >= 5) {
        *vx = v - 5;
    }
    *x += (f32) *vx;
    *y += (f32) *vy;
    if ((animate & 0xFF) > 0) {
        *frame += 1;
        *frame = (u8) ((s32) *frame % 4);
    }
    fx = *x;
    if (!(18.0f < fx) || !(fx < 620.0f) || (fy = *y, (fy <= 28.0f)) || !(fy < 422.0f)) {
        return 0;
    }
    return 1;
}

f32 Math_Absf(f32 x);

s32 DrinkContest_ShakeIcon(void *o, s8 *v, f32 *p, u8 bounce, u8 *unused)
{
    s8 mag;
    s8 dir;

    if (bounce == 0) {
        *v = 3;
        *p += 3.0f * *v;
        return 1;
    }
    *v *= -1.0f;
    *p += 3.0f * *v;
    mag = Math_Absf(*v);
    dir = *v / mag;
    if (dir <= 0) {
        if (--mag <= 0) {
            return 0;
        }
    }
    *v = mag * dir;
    return 1;
}

s32 DrinkContest_AllDone(void)
{
    Minigame *m = (Minigame *)&gMenuWork;
    u8 i;

    for (i = 0; i < m->count; i++) {
        if (m->timer[i] < 0x14) {
            return 0;
        }
    }
    return 1;
}

struct TexRect;
void TexRect_DrawRotated(struct TexRect *r, s32 angle);
void DrinkContest_DrawIcon(s16 kind, s16 x, s16 y, s32 tex, u32 color, f32 sy, f32 sx)
{
    Sprite s;
    f32 size;
    s16 k = kind;

    if ((u32)(k - 0x16) <= 1 || k == 0x18) {
        kind = 0x10;
    }
    if (kind == 8 || kind == 9) {
        size = 32.0f;
    } else {
        size = 24.0f;
    }
    if (kind == 0x14) {
        kind = 0;
    } else if (kind == 0x15) {
        kind = 1;
    }
    Gfx_SetTexFilter(0);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    s.x = x;
    s.y = y;
    s.color = color;
    s.w = size * sx;
    s.h = 24.0f * sy;
    if (kind == 9) {
        s.u0 = 0xE0;
    } else {
        s.u0 = (kind % 10) * 24;
    }
    s.v0 = (kind / 10) * 24;
    s.u1 = s.u0 + (s16)size + 1;
    if (kind == 0x12 || kind == 0x11 || kind == 0x13) {
        s.v1 = s.v0 + 0x18;
    } else {
        s.v1 = s.v0 + 0x19;
    }
    switch (k) {
    case 0x16: {
        s16 t = s.u1 - 1;
        s.u1 = s.u0;
        s.u0 = t;
        Sprite_Draw(&s);
        break;
    }
    case 0x17:
        s.u1 -= 1;
        TexRect_DrawRotated((struct TexRect *)&s, 0x4000);
        break;
    case 0x18:
        s.u1 -= 1;
        TexRect_DrawRotated((struct TexRect *)&s, 0xC000);
        break;
    default:
        Sprite_Draw(&s);
        break;
    }
    Gfx_SetTexFilter(1);
}

/* unk_6F8CC0's .data (0x7EC050-0x7EC170) */
extern char D_008382F0[];
u8 gMinigameRollCounts[16] = {
#include "lobby/007EC050.inc"
};
f32 gDrinkScaleSteps[4] = {
#include "lobby/007EC060.inc"
};
s8 gMinigameRollWeights[6][7] = {
#include "lobby/007EC070.inc"
};
u8 gDrinkLevel0[16] = {
#include "lobby/007EC0A0.inc"
};
u8 gDrinkLevel1[16] = {
#include "lobby/007EC0B0.inc"
};
u8 gDrinkLevel2[16] = {
#include "lobby/007EC0C0.inc"
};
u8 gDrinkLevel3[16] = {
#include "lobby/007EC0D0.inc"
};
u8 gDrinkLevel4[16] = {
#include "lobby/007EC0E0.inc"
};
u8 gDrinkHitSizeKeys[32] = {
#include "lobby/007EC0F0.inc"
};
u8 gDrinkHitAlphaKeys[32] = {
#include "lobby/007EC110.inc"
};
f32 gMinigameLevelC8[6] = {
#include "lobby/007EC130.inc"
};
s8 gDrinkTalkLines[8] = {
#include "lobby/007EC148.inc"
};
s32 gDrinkHpTalkLines[6] = {
#include "lobby/007EC150.inc"
};
char *gDrinkContestHelpTexts[2] = {
    D_008382D0, D_008382F0,
};
