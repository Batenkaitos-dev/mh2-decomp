#include "common.h"
#include "lobby_state.h"
#include "em.h"
#include "player.h"
#include "pad.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "lobby_vobj.h"
#include "lobby_menu.h"
#include "lobby/unk_70CC10.h"   /* the texts this file uses (tools/content_gen.py) */

/* prototypes */
void Gallery_TakePrize(void);
Em *Npc_Find(u8);
void Actor_SetIdle(void *, s32, s32, s32);
void TownMsg_SendStateNow(Player *p);
void ItemShop_Open(u8 *npc, s8 sel);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void CamFocus_End(void);
void CamFocus_SetAside(s8 arg0);
void Chat_ShowWindows(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u8 gTownMsg0DPending;
/* end of prototypes */
extern u8 gMenuWork[];
extern s8 gGalleryCourseTimes[];
void Gallery_ClearShots(void);

void Gallery_Close(void)
{
    gLobby->connected = 1;
    gTownMsg0DPending = 1;
    if (((PrizeMenu *)gMenuWork)->played == 0) {
        ItemShop_Open((u8 *) (Npc_Find(0x39)), 2);
        CamFocus_SetAside(0);
        gFieldControl = 1;
        Chat_ShowWindows();
        return;
    }
    Gallery_TakePrize();
    gTownMenuOpen = 0;
    gFieldControl = 1;
    gTownMode = 0;
    Chat_ShowWindows();
    LobbyMenu_SetDraw(0, 0, NULL, 0);
    Actor_SetIdle((u8 *) (&gPlayers[gScene.localPlayer]), 0, 6, 0);
    TownMsg_SendStateNow((void *)((s32) &gPlayers[gScene.localPlayer]));
    CamFocus_End();
    gPlayers[gScene.localPlayer].aimMode = 0;
}

s32 LobbyWorld_GetSeason(void);
s32 Village_Is3924Kind4(void);
extern PrizeEntry gGalleryPrizeList0[];
extern PrizeEntry gGalleryPrizeList1[];
extern PrizeEntry gGalleryPrizeList2[];
extern PrizeEntry gGalleryPrizeList3[];

/* the prize exchange's start: the prizes by the day's kind (a special
 * list on event days), the choices cleared */
void Gallery_InitPrizes(void)
{
    PrizeMenu *m = (PrizeMenu *)gMenuWork;
    s16 i;

    switch (LobbyWorld_GetSeason()) {
    default:
        m->list = gGalleryPrizeList0;
        break;
    case 2:
        m->list = gGalleryPrizeList2;
        break;
    case 1:
        m->list = gGalleryPrizeList1;
        break;
    }
    if (Village_Is3924Kind4() == 1) {
        m->list = gGalleryPrizeList3;
    }
    m->tooDear[0] = 0;
    m->tooDear[1] = 0;
    m->tooDear[2] = 0;
    m->tooDear[3] = 0;
    m->tooDear[4] = 0;
    m->tooDear[5] = 0;
    for (i = 0; i < 13; i++) {
        m->full[i] = 0;
    }
    for (i = 0; i < 0x28; i++) {
        m->items[i] = 0;
    }
}

void Gallery_ResetWork(void)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;

    c->yaw = 0;
    c->pitch = 0x5000;
    c->cursor = 0;
    c->prizeView = 0;
    c->details = 0;
    c->page = 0;
    c->count = 0;
    c->checked = 0;
    c->allTaken = 0;
    c->win.state = 0;
    c->unk10C = 0;
    c->toPrizes = 0;
    c->points = 0;
    c->pouchFull = 0;
    c->shotsLive = 0;
    c->skipped = 0;
}

void Gallery_ClearTargets(void)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;
    u8 i;

    for (i = 0; i < 15; i++) {
        c->slots[i] = 0;
    }
}

u16 Rand_Next(s32 stream);
void VObj_ResetTimer(VObj *o, u8 i, u8 k);

/* places the round's targets: the 15 slots (+0xD0) filled up to cfg's
 * counts of each kind, then shuffled (+0xDF) */
void Gallery_PlaceTargets(ShootCfg *cfg)
{
    u8 *w = gMenuWork;
    u8 flagA = 0;
    u8 *p;
    u8 n1;
    u8 total;
    u8 i;
    u8 b = 0;
    u8 flagB = 0;
    u8 a = 0;
    u8 kind;
    u8 j;
    u8 m;
    u8 dup;

    i = 0;
    do {
        if ((w + 0xD0)[i] > 0) {
            if ((w + 0xD0)[i] == cfg->kindB) {
                b++;
            } else if ((w + 0xD0)[i] == cfg->kindA) {
                a++;
            }
        }
        i++;
    } while (i < 15);
    n1 = cfg->nA - a;
    total = n1 + (u8)(cfg->nB - b);
    for (i = 0; i < total; i++) {
        if (i < n1) {
            kind = cfg->kindA;
        } else {
            kind = cfg->kindB;
        }
        for (j = 0; j < 15; j++) {
            p = w + j + 0xD0;
            if (*p <= 0) {
                *p = kind;
                VObj_ResetTimer(*(void **)(w + 0xC), j, kind);
                break;
            }
        }
    }
    if ((u8)(cfg->nA + cfg->nB) <= 1) {
        flagA = 1;
    }
    if (w[0xCD] > 1 || w[0xCF] == 1) {
        flagB = 1;
    }
    i = 0;
    do {
        do {
            p = w + i;
            p[0xDF] = Rand_Next(1) % 15;
            dup = 0;
            for (j = 0; j < i; j++) {
                if (p[0xDF] == w[0xDF + j]) {
                    dup = 1;
                    break;
                }
            }
        } while (dup);
        if (flagA == 1 && (w + 0xD0)[i] != cfg->kindB) {
            (w + 0xD0)[i] = 0;
        }
        if (flagB == 1 && (w + 0xD0)[i] != cfg->kindA) {
            (w + 0xD0)[i] = 0;
        }
        i++;
    } while (i < 15);
}

void Gallery_ResetShots(void)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;

    Gallery_ClearShots();
    c->time = gGalleryCourseTimes[c->course];
}


/* the gallery's aim: the d-pad (held) and the analog stick (stickDir, its tilt
 * at 0x6AE088 picks the speed) turn yaw and pitch, within their limits;
 * 0x6ADDE1 is the camera option (nonzero: yaw reversed, 2: pitch
 * reversed) */
void Gallery_Aim(void)
{
    PrizeMenu *w = (PrizeMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 inv = 0;
    u16 spd;
    s32 mag;

    if (pl->aimLockTimer == 0 && pl->aimMode == 0) {
        if (pl->input.held & 4) {
            return;
        }
        if (!(pl->input.held & 8)) {
            return;
        }
    }
    if (pl->input.held & 0x3000) {
        if (pl->aimMode != 0 && gScene.aimReverse != 0) {
            inv = 1;
        }
        if (pl->input.held & 0x2000) {
            if (inv <= 0) {
                w->yaw += 0x240;
            } else {
                w->yaw -= 0x240;
            }
        } else {
            if (inv <= 0) {
                w->yaw -= 0x240;
            } else {
                w->yaw += 0x240;
            }
        }
    }
    if (pl->input.held & 0x800) {
        if (pl->aimMode != 0 && gScene.aimReverse == 2) {
            w->pitch -= 0x240;
        } else {
            w->pitch += 0x240;
        }
    }
    if (pl->input.held & 0x400) {
        if (pl->aimMode != 0 && gScene.aimReverse == 2) {
            w->pitch += 0x240;
        } else {
            w->pitch -= 0x240;
        }
    }
    if (w->pitch < 0x3000) {
        w->pitch = 0x3000;
    }
    if (w->pitch > 0x7000) {
        w->pitch = 0x7000;
    }
    if ((s16)w->yaw < -0xAAB) {
        w->yaw = -0xAAB;
    }
    if ((s16)w->yaw > 0xAAB) {
        w->yaw = 0xAAB;
    }

    if (!(pl->input.stickDir & 0x3C00) || (mag = gPadInput.stickTilt[0].cur) < 0x28) {
        return;
    }
    if (mag >= 0x78) {
        if (pl->input.stickDir & 0x3000) {
            spd = 0x111;
        } else {
            spd = 0x1B0;
        }
    } else if (mag >= 0x5A) {
        if (pl->input.stickDir & 0x3000) {
            spd = 0x6D;
        } else {
            spd = 0xC0;
        }
    } else if (mag >= 0x3C) {
        if (pl->input.stickDir & 0x3000) {
            spd = 0x24;
        } else {
            spd = 0x40;
        }
    }
    if (pl->input.stickDir & 0x3000) {
        if (pl->aimMode != 0 && gScene.aimReverse != 0) {
            inv = 1;
        }
        if (pl->input.stickDir & 0x2000) {
            if (inv <= 0) {
                w->yaw += spd;
            } else {
                w->yaw -= spd;
            }
        } else {
            if (inv <= 0) {
                w->yaw -= spd;
            } else {
                w->yaw += spd;
            }
        }
    }
    if (pl->input.stickDir & 0x800) {
        if (gScene.aimReverse == 2) {
            w->pitch -= spd;
        } else {
            w->pitch += spd;
        }
    }
    if (pl->input.stickDir & 0x400) {
        if (gScene.aimReverse == 2) {
            w->pitch += spd;
        } else {
            w->pitch -= spd;
        }
    }
    if (w->pitch < 0x3000) {
        w->pitch = 0x3000;
    }
    if (w->pitch > 0x7000) {
        w->pitch = 0x7000;
    }
    if ((s16)w->yaw < -0xAAB) {
        w->yaw = -0xAAB;
    }
    if ((s16)w->yaw > 0xAAB) {
        w->yaw = 0xAAB;
    }
}

u16 LobbyPad_Get(s32 a);
void LobbySe_Play(s32 se);
void Jingle_Play(void *obj, s32 id);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
extern f32 gCamMatrix[];
extern f32 gGalleryGunPos[3];
void VFx_SpawnShot(Vec3f *from, Vec3f *to);
void VMuzzle_Spawn(f32 *pos);
void Vib_StartLocal(void *port, s32 i);
void Gallery_Aim(void);

/* the shooting gallery's round: a shot (pad 1) spawns a target in front
 * of the camera, the timer counts the shots down; 1 when they run out,
 * 0 when the round ends (unkB9 cleared), 2 while it runs */
s32 Gallery_ShootInput(void)
{
    PrizeMenu *w = (PrizeMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);
    Player *p = &gPlayers[gScene.localPlayer];
    f32 d[3];
    f32 v[3];

    switch (w->shootStep) {
    case 0:
        w->shootStep++;
        w->unk4A = 0;
        break;
    case 1:
        if (pad & 1) {
            w->shootStep++;
            w->unk4A = 0x1E;
            d[0] = 0.0f;
            d[1] = 0.0f;
            d[2] = -400.0f;
            Vec_Rotate((f32 *) v, (f32 *) d, gCamMatrix);
            d[0] = v[0] + gGalleryGunPos[0];
            d[1] = v[1] + gGalleryGunPos[1];
            d[2] = v[2] + gGalleryGunPos[2];
            VFx_SpawnShot((Vec3f *) gGalleryGunPos, (Vec3f *) d);
            VMuzzle_Spawn(w->muzzle);
            Vib_StartLocal(&gPlayers[gScene.localPlayer], 1);
            LobbySe_Play(0x19);
        }
        Gallery_Aim();
        break;
    case 2:
        w->shootStep++;
        break;
    case 3:
        if (w->unk4A >= 5) {
            w->unk4A -= 5;
        }
        if (w->shotsLive == 0) {
            return 0;
        }
        break;
    }
    if (--w->timer <= 0) {
        if (--w->time > 0) {
            LobbySe_Play(1);
        } else {
            Jingle_Play(p, 0x29);
            return 1;
        }
        w->timer = 0x1E;
    }
    return 2;
}

extern ShootCfg *gGalleryCourseCfgs[];
extern s8 gGalleryCourseShots[];
extern s16 gGalleryCourseFees[];
extern f32 D_007EC2F0[];
void Gallery_MarkTooDearCourses(void);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
s32 CamFocus_GetState(void);
void Actor_SetState(void *, s32, s32);
void Gallery_ResetShots(void);
void Effect2C_Spawn(s16 n);
s32 Menu_PushNoticeOnce(s32 a);
void Gallery_CheckPouchFull(void);
void Gallery_MarkFullPrizes(void);
void Gallery_MarkFullStacks(void);
s32 Menu_WrapUpDown(s32 a, s32 b);
s32 Menu_WrapLeftRight(s32 a, s32 b);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_Copy(f32 *out, f32 *in);
void VObj_Spawn(u8 *kinds, u8 *order, Vec3f *pos, f32 f);
void Gallery_UpdateShots(void);
u8 VObj_IsCleared(VObj *o);
void VDigits_Spawn(s16 a, u16 b);
s32 Save_TestEventFlag(s32 flag);
void Save_SetEventFlag(s32 flag);
u8 Notice_PushItem(char *);
s32 Award_Grant(s32 a);
s16 SavePouch_CountFree(void);
void Gallery_MarkTooDear(void);
void Gallery_ChoosePrize(void);
s32 Menu_YesNoInput(void);
void Menu_SetYesNo(s8 a);
void CamAim_Set(f32 *pos, s16 a, s16 b);

/* the shooting gallery's steps: entering (the camera), choosing the
 * course (pad 0x20 pays gGalleryCourseFees's fee), the round (Gallery_ShootInput, the
 * targets refilled from the course's ShootCfg list as the points rise),
 * the score, then the prize exchange (pages of 7) */
void Gallery_Run(void)
{
    PrizeMenu *w = (PrizeMenu *)gMenuWork;
    Player *p = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);
    f32 d[3];
    f32 v[3];
    s32 i;

    switch (w->step) {
    case 0:
        Gallery_MarkTooDearCourses();
        w->win.pos = 0x280;
        w->step = 1;
        LobbySe_Play(0x17);
        break;
    case 1:
        if (WinSlide_Step(&w->win, 0x280, 0x118) == 0) {
            if (w->toPrizes == 0) {
                w->step = 2;
            } else {
                w->step = 8;
                LobbySe_Play(0x17);
            }
        }
        break;
    case 2:
        if ((pad & 0x20) && w->prizeView == 0) {
            if (CamFocus_GetState() == -1) {
                if (gGalleryCourseFees[w->course] <= gSave.money) {
                    s32 k;

                    w->step = 3;
                    w->shootStep = 0;
                    w->timer = 0x5A;
                    w->cfg = gGalleryCourseCfgs[w->course];
                    w->restart = 0;
                    w->hitsB = 0;
                    w->shortClears = 0;
                    w->lastCfg = 0;
                    w->shotsLeft = gGalleryCourseShots[w->course];
                    w->page = 0;
                    w->played = 1;
                    w->playedCourse = w->course;
                    k = gScene.localPlayer;
                    gPlayers[k].aimMode = 1;
                    gPlayers[k].rot[1] = gPlayers[k].targetYaw = 0xCC17;
                    Actor_SetState(&gPlayers[k], 0, 0x79);
                    Gallery_ResetShots();
                    Effect2C_Spawn(w->timer);
                    w->kindHits[0] = 0;
                    w->kindHits[1] = 0;
                    w->kindHits[2] = 0;
                    w->kindHits[3] = 0;
                    d[0] = -500.0f;
                    d[1] = 0.0f;
                    d[2] = 0.0f;
                    Vec_Rotate((f32 *) w->unk38, (f32 *) d, gCamMatrix);
                    LobbySe_Play(8);
                    Jingle_Play(p, 0x28);
                } else {
                    Menu_PushNoticeOnce(0x16);
                    LobbySe_Play(9);
                }
            }
        } else if (pad & 0x40) {
            if (w->prizeView == 1) {
                w->step = 1;
                w->win.pos = 0x280;
                w->prizeView = 0;
                w->details = 0;
            } else {
                w->step = 10;
            }
            LobbySe_Play(4);
        } else if (pad & 0x200) {
            if (w->prizeView == 0) {
                w->step = 1;
                w->win.pos = 0x280;
                LobbySe_Play(0x17);
            }
            w->prizeView = 1;
            Gallery_CheckPouchFull();
            Gallery_MarkFullPrizes();
        } else if (w->prizeView == 0) {
            w->course = Menu_WrapUpDown(w->course, 6);
        }
        break;
    case 3:
        if (--w->timer < -9) {
            w->step = 4;
            w->shootStep = 0;
            w->timer = 0x1E;
            w->details = 0;
            w->cursor = 0;
            for (i = 0; i < 13; i++) {
                w->full[i] = 0;
            }
            v[0] = -1.0f;
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateSelf((f32 *) v, gCamMatrix);
            Vec_Copy(w->spawnDir, v);
            VObj_Spawn(w->slots, w->order, (Vec3f *) w->spawnDir, D_007EC2F0[w->course]);
            Gallery_PlaceTargets(w->cfg);
        }
        break;
    case 4:
        Gallery_UpdateShots();
        switch (Gallery_ShootInput()) {
        case 0:
            if (--w->shotsLeft <= 0) {
                Jingle_Play(p, 0x29);
                w->step = 5;
                w->timer = 0x1E;
            } else {
                w->shootStep = 0;
            }
            break;
        case 1:
            w->step = 5;
            w->timer = 0x1E;
            break;
        }
        if (VObj_IsCleared(w->targets)) {
            if (w->restart == 1 || w->lastCfg == 1) {
                Gallery_PlaceTargets(gGalleryCourseCfgs[w->course]);
                w->restart = 0;
            } else {
                Gallery_PlaceTargets(w->cfg);
            }
            if (w->hitsB < w->cfg->nB) {
                w->shortClears++;
            }
        }
        if (w->lastCfg <= 0) {
            if (w->points >= w->cfg->points) {
                if (w->hitsB >= w->cfg->nB || w->shortClears >= 2) {
                    w->cfg++;
                    w->hitsB = 0;
                    w->shortClears = 0;
                    w->restart = 0;
                    if (w->cfg->points < 0) {
                        w->lastCfg = 1;
                    }
                }
            } else if (w->hitsB >= w->cfg->nB || w->shortClears >= 2) {
                w->restart = 1;
                w->hitsB = 0;
            }
        }
        break;
    case 5:
        if (--w->timer <= 0) {
            u16 x;
            u16 y;

            w->step = 6;
            w->timer = 0xA0;
            if (w->course == 0) {
                x = 0x50;
                y = 0xC8;
            } else {
                x = 0x28;
                y = 0x122;
            }
            if (w->points > 0) {
                VDigits_Spawn(w->points, x + y - 0x40);
            }
        }
        break;
    case 6:
        if (pad & 0x20) {
            w->timer = 0;
            w->skipped = 1;
            LobbySe_Play(0);
        }
        if (--w->timer <= 0) {
            w->step = 7;
            if (w->points >= 0xDAC && Save_TestEventFlag(0x75) == 0) {
                Save_SetEventFlag(0x75);
                Notice_PushItem(TXT(0x00838610) /* the notice of a new outfit for the pet pig */);
            }
            if (w->playedCourse == 5 && w->points >= 4000) {
                Award_Grant(0x26);
            }
        }
        break;
    case 7:
        if (pad & 0x20) {
            if (w->course > 0) {
                w->prizeView = 1;
                w->toPrizes = 1;
                w->step = 1;
                w->win.pos = 0x280;
                if (SavePouch_CountFree() <= 0) {
                    Gallery_MarkFullStacks();
                }
                Gallery_MarkFullPrizes();
                Gallery_MarkTooDear();
            } else {
                w->step = 0;
                Gallery_ResetWork();
                Gallery_ClearTargets();
            }
        }
        break;
    case 8:
        if (pad & 0x3000) {
            if (w->page > 0) {
                w->cursor = Menu_WrapUpDown(w->cursor, 6);
            } else {
                w->cursor = Menu_WrapUpDown(w->cursor, 7);
            }
        } else if (pad & 0xC00) {
            w->page = Menu_WrapLeftRight(w->page, 2);
            w->cursor = 0;
        } else if (pad & 0x200) {
            LobbySe_Play(0x39);
            if (w->details == 0) {
                w->details = 1;
            } else {
                w->details = 0;
            }
        } else if (pad & 0x20) {
            u8 n = w->cursor + w->page * 7;
            u16 cost = w->list[n].points;

            if (cost <= w->points && w->full[n] <= 0) {
                w->points -= (s16)cost;
                Gallery_ChoosePrize();
                Gallery_CheckPouchFull();
                Gallery_MarkFullPrizes();
                Gallery_MarkTooDear();
                LobbySe_Play(8);
            } else {
                LobbySe_Play(9);
            }
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            w->step = 9;
        }
        break;
    case 9:
        if (pad & 0x200) {
            LobbySe_Play(0x39);
            if (w->details == 0) {
                w->details = 1;
            } else {
                w->details = 0;
            }
        }
        switch (Menu_YesNoInput()) {
        case 0:
            w->step = 10;
            break;
        case 3:
            w->step = 8;
            Menu_SetYesNo(0);
            break;
        }
        break;
    case 10:
        Gallery_Close();
        return;
    }
    CamAim_Set(gGalleryGunPos, w->yaw, w->pitch);
}

s32 Gallery_BeforeShooting(void)
{
    PrizeMenu *c = (PrizeMenu *)gMenuWork;

    if (gTownMode != 0x2F) {
        return 0;
    }
    return c->step < 6;
}

/* unk_70CC10's .data (0x7EC2F0-0x7EC5C8) */
f32 D_007EC2F0[8] = {
#include "lobby/007EC2F0.inc"
};
u32 gGalleryPrizes[24] = {
#include "lobby/007EC310.inc"
};
ShootCfg gGalleryCourseCfgs_0[8] = {
#include "lobby/007EC370.inc"
};
ShootCfg gGalleryCourseCfgs_1[8] = {
#include "lobby/007EC3A0.inc"
};
ShootCfg gGalleryCourseCfgs_2[8] = {
#include "lobby/007EC3D0.inc"
};
ShootCfg gGalleryCourseCfgs_3[8] = {
#include "lobby/007EC400.inc"
};
ShootCfg gGalleryCourseCfgs_4[8] = {
#include "lobby/007EC430.inc"
};
ShootCfg gGalleryCourseCfgs_5[8] = {
#include "lobby/007EC460.inc"
};
ShootCfg *gGalleryCourseCfgs[6] = {
    gGalleryCourseCfgs_0, gGalleryCourseCfgs_1, gGalleryCourseCfgs_2, gGalleryCourseCfgs_3,
    gGalleryCourseCfgs_4, gGalleryCourseCfgs_5,
};
s8 gGalleryCourseShots[8] = {
#include "lobby/007EC4A8.inc"
};
s8 gGalleryCourseTimes[16] = {
#include "lobby/007EC4B0.inc"
};
PrizeEntry gGalleryPrizeList0[16] = {
#include "lobby/007EC4C0.inc"
};
PrizeEntry gGalleryPrizeList1[16] = {
#include "lobby/007EC500.inc"
};
PrizeEntry gGalleryPrizeList2[16] = {
#include "lobby/007EC540.inc"
};
PrizeEntry gGalleryPrizeList3[14] = {
#include "lobby/007EC580.inc"
};
s16 gGalleryCourseFees[6] = {
#include "lobby/007EC5B8.inc"
};
