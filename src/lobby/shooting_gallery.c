/*
 * The shooting gallery (TOWN_MODE_GALLERY): a menu of six courses, each
 * with a fee, a time and a number of shots; a round shot at a board of
 * monster targets from a fixed gun; the score; and the prize exchange,
 * where the points buy prizes for the pouch.
 *
 * Gallery_Open opens it and Gallery_Run is its update (PrizeMenu.step,
 * enum GalleryStep); Gallery_DrawShooting draws it. The state is a
 * PrizeMenu in gMenuWork (lobby_menu.h). The board (GalleryBoard_*), the
 * shots (GalleryShot_*) and the score are stage objects of their own,
 * which register here through Gallery_AddObj and ask Gallery_BeforeShooting,
 * Gallery_InMenuOrClose and Gallery_InResults where the round is.
 *
 * The functions reach gMenuWork as `(PrizeMenu *)gMenuWork.raw`, a pointer
 * cast from the work's bytes: written as `&gMenuWork.prize`, the compiler
 * addresses each field from the global's address instead of loading the
 * work's address once, as the game's code does. Gallery_PlaceTargets
 * reads the work as bytes for the same reason (PRIZE_OFS names the
 * fields' offsets).
 */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "menu_task.h"
#include "em.h"
#include "player.h"
#include "pad.h"
#include "scene.h"
#include "save.h"
#include "quest.h"
#include "award.h"
#include "lobby_vobj.h"
#include "window.h"
#include "sprite.h"
#include "render_state.h"
#include "prim.h"
#include "lobby_text.h"
#include "lobby_npc.h"
#include "texture.h"
#include "lobby/shooting_gallery.h"   /* the texts this file uses (tools/content_gen.py) */
#include "lobby_labels.h"
#include "area_draw.h"
#include "bgm.h"
#include "camera.h"
#include "capsule.h"
#include "fishing.h"
#include "hud.h"
#include "village_cond.h"
#include "shooting_gallery.h"
#include "engine_math.h"
#include "font.h"
#include "gallery_board.h"
#include "gallery_objects.h"
#include "gallery_targets.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_util.h"
#include "lobby_world.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "minigame_draw.h"
#include "name_tags.h"
#include "rand.h"
#include "town_players.h"
#include "vibration.h"
#include "lobby_net.h"

void *memset(void *dst, s32 c, u32 n);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern MenuWork gMenuWork;
/* prototypes */
extern u8 gTownAreaMsgPending;
/* end of prototypes */
extern s8 gGalleryCourseTimes[];
/* prototypes */
/* end of prototypes */
/* the calls cast to s8: this unit's code takes the value as s8 */
extern s16 gGalleryCourseFees[];
u8 SavePouch_Add(u16 item, s16 n);
void Gfx_SetTexFilter(s32);
void Text_SetSize(s32 size);
void Text_SetColor(s16 color);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
s32 sprintf(char *buf, char *fmt, ...);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);
struct Frame;
void Gfx_SetBlend(s32 a, s32 b);
void Tex_Bind(s32);
extern s8 gGalleryCourseShots[];
extern LobbyMenu *gLobbyMenu;

/*
 * Opens the shooting gallery (TOWN_MODE_GALLERY): clears gMenuWork, takes
 * field control away, hides the chat windows, sets Gallery_DrawShooting as
 * the draw callback, clears the shots, resets the work and the prize
 * lists, and marks the lobby link as not connected while it runs.
 */
void Gallery_Open(void)
{
    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_GALLERY;
    memset(&gMenuWork, 0, 3840);
    gFieldControl = 0;
    Chat_HideWindows();
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Gallery_DrawShooting, 0, 0, 0);
    Gallery_ClearShots();
    Gallery_ResetWork();
    gMenuWork.prize.played = 0;
    Gallery_InitPrizes();
    gLobby->connected = 0;
}

/*
 * Leaves the gallery. Without a paid round, goes back to the shop menu of
 * gallery keeper (NPC_GALLERY_KEEPER; cursor on line 2). After a round, gives the chosen prizes and
 * charges the course fee (Gallery_TakePrize), returns to TOWN_MODE_FREE,
 * idles the local player, sends its state, ends the camera focus and
 * clears aimMode.
 */
void Gallery_Close(void)
{
    gLobby->connected = 1;
    gTownAreaMsgPending = 1;
    if (gMenuWork.prize.played == 0) {
        ItemShop_Open((u8 *) (Npc_Find(NPC_GALLERY_KEEPER)), 2); /* back to its shop menu */
        CamFocus_SetAside(0);
        gFieldControl = 1;
        Chat_ShowWindows();
        return;
    }
    Gallery_TakePrize();
    gTownMenuOpen = 0;
    gFieldControl = 1;
    gTownMode = TOWN_MODE_FREE;
    Chat_ShowWindows();
    LobbyMenu_SetDraw(0, 0, NULL, 0);
    Actor_ReturnToIdle((u8 *) (&gPlayers[gScene.localPlayer]), 0, 6, 0);
    TownMsg_SendStateNow((void *)((s32) &gPlayers[gScene.localPlayer]));
    CamFocus_End();
    gPlayers[gScene.localPlayer].aimMode = 0;
}

extern PrizeEntry gGalleryPrizeList0[];
extern PrizeEntry gGalleryPrizeList1[];
extern PrizeEntry gGalleryPrizeList2[];
extern PrizeEntry gGalleryPrizeList3[];

/*
 * Picks the 13-prize list for the season (lists 0/1/2), or list 3 on event
 * days. Clears the courses' tooDear flags, the prizes' locked flags and
 * the 40 chosen-prize slots.
 */
void Gallery_InitPrizes(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    s16 i;

    switch (LobbyWorld_GetSeason()) {
    default:
        gal->list = gGalleryPrizeList0;
        break;
    case 2:
        gal->list = gGalleryPrizeList2;
        break;
    case 1:
        gal->list = gGalleryPrizeList1;
        break;
    }
    if (Town_IsSaleEvent() == 1) {
        gal->list = gGalleryPrizeList3;
    }
    gal->tooDear[0] = 0;
    gal->tooDear[1] = 0;
    gal->tooDear[2] = 0;
    gal->tooDear[3] = 0;
    gal->tooDear[4] = 0;
    gal->tooDear[5] = 0;
    for (i = 0; i < GALLERY_PRIZE_COUNT; i++) {
        gal->locked[i] = 0;
    }
    for (i = 0; i < GALLERY_CHOSEN_MAX; i++) {
        gal->chosen[i] = 0;
    }
}

/*
 * Resets the menu state for a new visit or round: aim to its centre (0,
 * ANGLE_TURN * 5 / 16), cursor, page, prize view, details, chosen count, flags, window
 * state, points, pouchFull, shotsLive, skipped.
 */
void Gallery_ResetWork(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;

    gal->yaw = 0;
    gal->pitch = ANGLE_TURN * 5 / 16;
    gal->cursor = 0;
    gal->prizeView = 0;
    gal->details = 0;
    gal->page = 0;
    gal->chosenCount = 0;
    gal->stacksChecked = 0;
    gal->allTaken = 0;
    gal->win.state = 0;
    gal->unk10C = 0;
    gal->toPrizes = 0;
    gal->points = 0;
    gal->pouchFull = 0;
    gal->shotsLive = 0;
    gal->skipped = 0;
}

/* Clears the 15 target slots (no kind in any of them). */
void Gallery_ClearTargets(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    u8 i;

    for (i = 0; i < GALLERY_TARGETS; i++) {
        gal->slots[i] = 0;
    }
}

/* A field's offset in PrizeMenu, for the work read as bytes. */
#define PRIZE_OFS(field) ((s32)&((PrizeMenu *)0)->field)

/*
 * Fills empty target slots so the board has cfg's nA targets of kindA and
 * nB of kindB (setting each new slot's kind on the board), then shuffles
 * the 15 spots into order[]. With nA+nB <= 1 only kindB slots are kept;
 * after 2 short clears or past the last cfg only kindA slots are kept.
 */
void Gallery_PlaceTargets(ShootCfg *cfg)
{
    u8 *gal = gMenuWork.raw;
    u8 onlyB = 0;
    u8 *slot;
    u8 needA;
    u8 need;
    u8 i;
    u8 haveB = 0;
    u8 onlyA = 0;
    u8 haveA = 0;
    u8 kind;
    u8 j;
    u8 unused;
    u8 dup;

    i = 0;
    do {
        if ((gal + PRIZE_OFS(slots))[i] > 0) {
            if ((gal + PRIZE_OFS(slots))[i] == cfg->kindB) {
                haveB++;
            } else if ((gal + PRIZE_OFS(slots))[i] == cfg->kindA) {
                haveA++;
            }
        }
        i++;
    } while (i < GALLERY_TARGETS);
    needA = cfg->nA - haveA;
    need = needA + (u8)(cfg->nB - haveB);
    for (i = 0; i < need; i++) {
        if (i < needA) {
            kind = cfg->kindA;
        } else {
            kind = cfg->kindB;
        }
        for (j = 0; j < GALLERY_TARGETS; j++) {
            slot = gal + j + PRIZE_OFS(slots);
            if (*slot <= 0) {
                *slot = kind;
                GalleryBoard_SetTargetKind(*(void **)(gal + PRIZE_OFS(targets)), j, kind);
                break;
            }
        }
    }
    if ((u8)(cfg->nA + cfg->nB) <= 1) {
        onlyB = 1;
    }
    if (gal[PRIZE_OFS(shortClears)] > 1 || gal[PRIZE_OFS(lastCfg)] == 1) {
        onlyA = 1;
    }
    i = 0;
    do {
        do {
            slot = gal + i;
            slot[PRIZE_OFS(order)] = Rand_Next(1) % 15;
            dup = 0;
            for (j = 0; j < i; j++) {
                if (slot[PRIZE_OFS(order)] == gal[PRIZE_OFS(order) + j]) {
                    dup = 1;
                    break;
                }
            }
        } while (dup);
        if (onlyB == 1 && (gal + PRIZE_OFS(slots))[i] != cfg->kindB) {
            (gal + PRIZE_OFS(slots))[i] = 0;
        }
        if (onlyA == 1 && (gal + PRIZE_OFS(slots))[i] != cfg->kindA) {
            (gal + PRIZE_OFS(slots))[i] = 0;
        }
        i++;
    } while (i < GALLERY_TARGETS);
}

/*
 * Clears the shots in flight and sets the time left to the course's time.
 */
void Gallery_ResetShots(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;

    Gallery_ClearShots();
    gal->time = gGalleryCourseTimes[gal->course];
}


/*
 * Moves the aim from the d-pad (held, step 0x240) and the left stick (step
 * by tilt: >= 60, 90, 120), honouring gScene.aimReverse while in
 * aimMode. Up/down moves the `yaw` field (clamped to +-0xAAB), left/right
 * the `pitch` field (clamped ANGLE_TURN * 3 / 16 to ANGLE_TURN * 7 / 16).
 * Does nothing unless aiming (L1 held, not L2).
 */
void Gallery_Aim(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 reversed = 0;
    u16 step;
    s32 tilt;

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
            reversed = 1;
        }
        if (pl->input.held & 0x2000) {
            if (reversed <= 0) {
                gal->yaw += 0x240;
            } else {
                gal->yaw -= 0x240;
            }
        } else {
            if (reversed <= 0) {
                gal->yaw -= 0x240;
            } else {
                gal->yaw += 0x240;
            }
        }
    }
    if (pl->input.held & 0x800) {
        if (pl->aimMode != 0 && gScene.aimReverse == 2) {
            gal->pitch -= 0x240;
        } else {
            gal->pitch += 0x240;
        }
    }
    if (pl->input.held & 0x400) {
        if (pl->aimMode != 0 && gScene.aimReverse == 2) {
            gal->pitch += 0x240;
        } else {
            gal->pitch -= 0x240;
        }
    }
    if (gal->pitch < (ANGLE_TURN * 3 / 16)) {
        gal->pitch = ANGLE_TURN * 3 / 16;
    }
    if (gal->pitch > (ANGLE_TURN * 7 / 16)) {
        gal->pitch = ANGLE_TURN * 7 / 16;
    }
    if ((s16)gal->yaw < -0xAAB) {
        gal->yaw = -0xAAB;
    }
    if ((s16)gal->yaw > 0xAAB) {
        gal->yaw = 0xAAB;
    }

    if (!(pl->input.stickDir & 0x3C00) || (tilt = gPadInput.stickTilt[0].left) < 40) {
        return;
    }
    if (tilt >= 120) {
        if (pl->input.stickDir & 0x3000) {
            step = 273;
        } else {
            step = 432;
        }
    } else if (tilt >= 90) {
        if (pl->input.stickDir & 0x3000) {
            step = 109;
        } else {
            step = 192;
        }
    } else if (tilt >= 60) {
        if (pl->input.stickDir & 0x3000) {
            step = 36;
        } else {
            step = 64;
        }
    }
    if (pl->input.stickDir & 0x3000) {
        if (pl->aimMode != 0 && gScene.aimReverse != 0) {
            reversed = 1;
        }
        if (pl->input.stickDir & 0x2000) {
            if (reversed <= 0) {
                gal->yaw += step;
            } else {
                gal->yaw -= step;
            }
        } else {
            if (reversed <= 0) {
                gal->yaw -= step;
            } else {
                gal->yaw += step;
            }
        }
    }
    if (pl->input.stickDir & 0x800) {
        if (gScene.aimReverse == 2) {
            gal->pitch -= step;
        } else {
            gal->pitch += step;
        }
    }
    if (pl->input.stickDir & 0x400) {
        if (gScene.aimReverse == 2) {
            gal->pitch += step;
        } else {
            gal->pitch -= step;
        }
    }
    if (gal->pitch < (ANGLE_TURN * 3 / 16)) {
        gal->pitch = ANGLE_TURN * 3 / 16;
    }
    if (gal->pitch > (ANGLE_TURN * 7 / 16)) {
        gal->pitch = ANGLE_TURN * 7 / 16;
    }
    if ((s16)gal->yaw < -0xAAB) {
        gal->yaw = -0xAAB;
    }
    if ((s16)gal->yaw > 0xAAB) {
        gal->yaw = 0xAAB;
    }
}

extern f32 gCamMatrix[];
extern f32 gGalleryGunPos[4];

/*
 * Runs one shot of the round: on PAD_R3 (the fire button) spawns a shot
 * from the gun along the camera, the muzzle flash, a vibration and the
 * shot sound, then waits until no shot is in flight. Also counts the time
 * down (a tick sound each second). Returns 0 when the shot has landed, 1
 * when the time ran out (end jingle), 2 while running.
 */
s32 Gallery_ShootInput(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    u16 pad = LobbyPad_Get(0);
    Player *pl = &gPlayers[gScene.localPlayer];
    f32 aimPoint[3];
    f32 rotated[3];

    switch (gal->shootStep) {
    case GALLERY_SHOOT_READY:
        gal->shootStep++;
        gal->recoil = 0;
        break;
    case GALLERY_SHOOT_AIM:
        if (pad & PAD_R3) {
            gal->shootStep++;
            gal->recoil = 30;
            aimPoint[0] = 0.0f;
            aimPoint[1] = 0.0f;
            aimPoint[2] = -400.0f;
            Vec_Rotate((f32 *) rotated, (f32 *) aimPoint, gCamMatrix);
            aimPoint[0] = rotated[0] + gGalleryGunPos[0];
            aimPoint[1] = rotated[1] + gGalleryGunPos[1];
            aimPoint[2] = rotated[2] + gGalleryGunPos[2];
            GalleryShot_Spawn((Vec3f *) gGalleryGunPos, (Vec3f *) aimPoint);
            GalleryMuzzle_Spawn(gal->muzzle);
            Vib_StartLocal(&gPlayers[gScene.localPlayer], 1);
            LobbySe_Play(LOBBY_SE_GALLERY_SHOT);
        }
        Gallery_Aim();
        break;
    case GALLERY_SHOOT_FIRED:
        gal->shootStep++;
        break;
    case GALLERY_SHOOT_IN_FLIGHT:
        if (gal->recoil >= 5) {
            gal->recoil -= 5;
        }
        if (gal->shotsLive == 0) {
            return 0;
        }
        break;
    }
    if (--gal->timer <= 0) {
        if (--gal->time > 0) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        } else {
            Jingle_Play(pl, GALLERY_JINGLE_END);
            return 1;
        }
        gal->timer = 30;
    }
    return 2;
}

extern ShootCfg *gGalleryCourseCfgs[];
extern f32 gGalleryCourseHoldFrames[];
s32 Award_Grant(s32 a);

/*
 * TOWN_MODE_GALLERY's update, by PrizeMenu.step: slides the window in,
 * runs the course menu (circle starts a course if the fee is affordable,
 * square shows the prizes, cross leaves), the countdown, the round (shots,
 * refilling the targets and advancing the ShootCfg list by points and
 * kindB hits), the score, the award/event checks, the prize exchange and
 * the quit question. Aims the camera last.
 */
void Gallery_Run(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = LobbyPad_Get(0);
    f32 sideOffset[3];
    f32 boardDir[3];
    s32 i;

    switch (gal->step) {
    case GALLERY_STEP_OPEN:
        Gallery_MarkTooDearCourses();
        gal->win.pos = 640;
        gal->step = GALLERY_STEP_SLIDE_IN;
        LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
        break;
    case GALLERY_STEP_SLIDE_IN:
        if (WinSlide_Step(&gal->win, 640, 280) == 0) {
            if (gal->toPrizes == 0) {
                gal->step = GALLERY_STEP_COURSE;
            } else {
                gal->step = GALLERY_STEP_PRIZES;
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            }
        }
        break;
    case GALLERY_STEP_COURSE:
        if ((pad & PAD_CIRCLE) && gal->prizeView == 0) {
            if (CamFocus_GetState() == -1) {
                if (gGalleryCourseFees[gal->course] <= gSave.money) {
                    s32 localIdx;

                    gal->step = GALLERY_STEP_COUNTDOWN;
                    gal->shootStep = 0;
                    gal->timer = 90;
                    gal->cfg = gGalleryCourseCfgs[gal->course];
                    gal->restart = 0;
                    gal->hitsB = 0;
                    gal->shortClears = 0;
                    gal->lastCfg = 0;
                    gal->shotsLeft = gGalleryCourseShots[gal->course];
                    gal->page = 0;
                    gal->played = 1;
                    gal->playedCourse = gal->course;
                    localIdx = gScene.localPlayer;
                    gPlayers[localIdx].aimMode = 1;
                    gPlayers[localIdx].rot[1] = gPlayers[localIdx].targetYaw = 0xCC17;
                    Actor_SetState(&gPlayers[localIdx], 0, TOWN_PC_GALLERY_AIM);
                    Gallery_ResetShots();
                    Effect2C_Spawn(gal->timer);
                    gal->kindHits[0] = 0;
                    gal->kindHits[1] = 0;
                    gal->kindHits[2] = 0;
                    gal->kindHits[3] = 0;
                    sideOffset[0] = -500.0f;
                    sideOffset[1] = 0.0f;
                    sideOffset[2] = 0.0f;
                    Vec_Rotate((f32 *) gal->sideOffset, (f32 *) sideOffset, gCamMatrix);
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                    Jingle_Play(pl, GALLERY_JINGLE_START);
                } else {
                    Menu_PushNoticeOnce(LOBBY_NOTICE_NO_MONEY);
                    LobbySe_Play(LOBBY_SE_REFUSE);
                }
            }
        } else if (pad & PAD_CROSS) {
            if (gal->prizeView == 1) {
                gal->step = GALLERY_STEP_SLIDE_IN;
                gal->win.pos = 640;
                gal->prizeView = 0;
                gal->details = 0;
            } else {
                gal->step = GALLERY_STEP_CLOSE;
            }
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & PAD_SQUARE) {
            if (gal->prizeView == 0) {
                gal->step = GALLERY_STEP_SLIDE_IN;
                gal->win.pos = 640;
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            }
            gal->prizeView = 1;
            Gallery_CheckPouchFull();
            Gallery_MarkFullPrizes();
        } else if (gal->prizeView == 0) {
            gal->course = Menu_WrapUpDown(gal->course, 6);
        }
        break;
    case GALLERY_STEP_COUNTDOWN:
        if (--gal->timer < -9) {
            gal->step = GALLERY_STEP_SHOOT;
            gal->shootStep = 0;
            gal->timer = 30;
            gal->details = 0;
            gal->cursor = 0;
            for (i = 0; i < GALLERY_PRIZE_COUNT; i++) {
                gal->locked[i] = 0;
            }
            boardDir[0] = -1.0f;
            boardDir[1] = 0.0f;
            boardDir[2] = 0.0f;
            Vec_RotateSelf((f32 *) boardDir, gCamMatrix);
            Vec_Copy(gal->spawnDir, boardDir);
            GalleryBoard_Spawn(gal->slots, gal->order, (Vec3f *) gal->spawnDir, gGalleryCourseHoldFrames[gal->course]);
            Gallery_PlaceTargets(gal->cfg);
        }
        break;
    case GALLERY_STEP_SHOOT:
        Gallery_UpdateShots();
        switch (Gallery_ShootInput()) {
        case 0:
            if (--gal->shotsLeft <= 0) {
                Jingle_Play(pl, GALLERY_JINGLE_END);
                gal->step = GALLERY_STEP_ROUND_END;
                gal->timer = 30;
            } else {
                gal->shootStep = 0;
            }
            break;
        case 1:
            gal->step = GALLERY_STEP_ROUND_END;
            gal->timer = 30;
            break;
        }
        if (GalleryBoard_IsCleared(gal->targets)) {
            if (gal->restart == 1 || gal->lastCfg == 1) {
                Gallery_PlaceTargets(gGalleryCourseCfgs[gal->course]);
                gal->restart = 0;
            } else {
                Gallery_PlaceTargets(gal->cfg);
            }
            if (gal->hitsB < gal->cfg->nB) {
                gal->shortClears++;
            }
        }
        if (gal->lastCfg <= 0) {
            if (gal->points >= gal->cfg->points) {
                if (gal->hitsB >= gal->cfg->nB || gal->shortClears >= 2) {
                    gal->cfg++;
                    gal->hitsB = 0;
                    gal->shortClears = 0;
                    gal->restart = 0;
                    if (gal->cfg->points < 0) {
                        gal->lastCfg = 1;
                    }
                }
            } else if (gal->hitsB >= gal->cfg->nB || gal->shortClears >= 2) {
                gal->restart = 1;
                gal->hitsB = 0;
            }
        }
        break;
    case GALLERY_STEP_ROUND_END:
        if (--gal->timer <= 0) {
            u16 ybase;
            u16 height;

            gal->step = GALLERY_STEP_SCORE;
            gal->timer = 160;
            if (gal->course == 0) {
                ybase = 80;
                height = 200;
            } else {
                ybase = 40;
                height = 290;
            }
            if (gal->points > 0) {
                GalleryScore_Spawn(gal->points, ybase + height - 64);
            }
        }
        break;
    case GALLERY_STEP_SCORE:
        if (pad & PAD_CIRCLE) {
            gal->timer = 0;
            gal->skipped = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        }
        if (--gal->timer <= 0) {
            gal->step = GALLERY_STEP_RESULT;
            if (gal->points >= GALLERY_BONUS_POINTS && Save_TestEventFlag(EVENT_FLAG_POOGIE_OUTFIT_1) == 0) {
                Save_SetEventFlag(EVENT_FLAG_POOGIE_OUTFIT_1);
                Notice_PushItem(TXT(0x00838610) /* the notice of a new outfit for the pet pig */);
            }
            if (gal->playedCourse == 5 && gal->points >= GALLERY_AWARD_POINTS) {
                Award_Grant(AWARD_GALLERY_COURSE_5);
            }
        }
        break;
    case GALLERY_STEP_RESULT:
        if (pad & PAD_CIRCLE) {
            if (gal->course > 0) {
                gal->prizeView = 1;
                gal->toPrizes = 1;
                gal->step = GALLERY_STEP_SLIDE_IN;
                gal->win.pos = 640;
                if (SavePouch_CountFree() <= 0) {
                    Gallery_MarkFullStacks();
                }
                Gallery_MarkFullPrizes();
                Gallery_MarkTooDear();
            } else {
                gal->step = GALLERY_STEP_OPEN;
                Gallery_ResetWork();
                Gallery_ClearTargets();
            }
        }
        break;
    case GALLERY_STEP_PRIZES:
        if (pad & (PAD_UP | PAD_DOWN)) {
            if (gal->page > 0) {
                gal->cursor = Menu_WrapUpDown(gal->cursor, 6);
            } else {
                gal->cursor = Menu_WrapUpDown(gal->cursor, 7);
            }
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            gal->page = Menu_WrapLeftRight(gal->page, 2);
            gal->cursor = 0;
        } else if (pad & PAD_SQUARE) {
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
            if (gal->details == 0) {
                gal->details = 1;
            } else {
                gal->details = 0;
            }
        } else if (pad & PAD_CIRCLE) {
            u8 sel = gal->cursor + gal->page * 7;
            u16 cost = gal->list[sel].points;

            if (cost <= gal->points && gal->locked[sel] <= 0) {
                gal->points -= (s16)cost;
                Gallery_ChoosePrize();
                Gallery_CheckPouchFull();
                Gallery_MarkFullPrizes();
                Gallery_MarkTooDear();
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            gal->step = GALLERY_STEP_QUIT_ASK;
        }
        break;
    case GALLERY_STEP_QUIT_ASK:
        if (pad & PAD_SQUARE) {
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
            if (gal->details == 0) {
                gal->details = 1;
            } else {
                gal->details = 0;
            }
        }
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            gal->step = GALLERY_STEP_CLOSE;
            break;
        case MENU_CANCEL:
            gal->step = GALLERY_STEP_PRIZES;
            Menu_SetYesNo(0);
            break;
        }
        break;
    case GALLERY_STEP_CLOSE:
        Gallery_Close();
        return;
    }
    CamAim_Set(gGalleryGunPos, gal->yaw, gal->pitch);
}

/*
 * 1 while the gallery is open (TOWN_MODE_GALLERY) and its step is below
 * the score (step < 6), else 0. The target board ends itself when this
 * returns 0.
 */
s32 Gallery_BeforeShooting(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;

    if (gTownMode != 47) {
        return 0;
    }
    return gal->step < 6;
}

/* The courses' and the targets' tables. */
f32 gGalleryCourseHoldFrames[8] = {
#include "lobby/007EC2F0.inc"
};
u32 gGalleryKindMonsters[24] = {
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

/*
 * Draws the gun's debug marker from the gun position into PrizeMenu.muzzle
 * (Gallery_DrawGunDebug).
 */
void Gallery_RunGunDebug(void)
{
    Gallery_DrawGunDebug(gGalleryGunPos, gMenuWork.prize.muzzle);
}

/*
 * The gallery's draw callback, by step: the course list or prize page
 * during the slide (1) and the course menu (2, which also moves the prize
 * page's cursor/page/details), the shot marks, time, points and the
 * controls legend during the round (4), the result window (6, 7: course
 * name, each target monster's picture, name and hits, the points; 7 adds
 * the next-button label), the prize window (8) and the prize window with
 * the quit question and yes/no (9).
 */
void Gallery_DrawShooting(MenuTask *task)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    char buf[32];
    Sprite spr;
    PrimTri tri;
    u8 rows;
    u16 height;
    s32 firstKind;
    u16 pad;
    s32 savedAlphaTest;
    s32 i;
    s32 level;
    s16 *sprY;
    char *label;
    u16 ybase;
    s16 y;
    s32 savedBlend;
    s32 lim;
    u32 *monster;
    s16 *triY;
    f32 nameX;
    s32 top;

    pad = LobbyPad_Get(0);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    Text_SetSize(task->textSize);
    Text_SetColor(0);
    Font_SetSpacing(18, 18);
    switch (gal->step) {
    case GALLERY_STEP_SLIDE_IN:
        if (gal->toPrizes == 1) {
            Gallery_DrawPrizeWindow(gal->win.pos, 0);
            return;
        }
        if (gal->prizeView == 0) {
            Gallery_DrawPrizeList(gal->win.pos);
            return;
        }
        Gallery_DrawPrizePage(gal->win.pos);
        return;
    case GALLERY_STEP_COURSE:
        if (gal->prizeView == 1) {
            if (pad & (PAD_UP | PAD_DOWN)) {
                if (gal->page > 0) {
                    gal->cursor = Menu_WrapUpDown(gal->cursor, 6);
                } else {
                    gal->cursor = Menu_WrapUpDown(gal->cursor, 7);
                }
            } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
                gal->page = Menu_WrapLeftRight(gal->page, 2);
                gal->cursor = 0;
            } else if (pad & PAD_SQUARE) {
                if (gal->details == 0) {
                    gal->details = 1;
                } else {
                    gal->details = 0;
                }
                LobbySe_Play(LOBBY_SE_DESCRIPTION);
            }
            Gallery_DrawPrizePage(280);
            return;
        }
        Gallery_DrawPrizeList(280);
        return;
    case GALLERY_STEP_SHOOT:
        Gallery_DrawShotMarks(gGalleryCourseShots[gal->course], gal->shotsLeft);
        sprintf(buf, TXT(0x00838660) /* the time left, with its label */, gal->time);
        Font_SetSpacing(22, 22);
        Menu_DrawRowBar(30, 16, 160, 0xFFB0B000, 2, 1);
        Text_DrawShadowRgb(30, 16, 0xFF202020, -1, buf);
        sprintf(buf, TXT(0x00838670) /* the points, with their label */, gal->points);
        Font_SetSpacing(22, 22);
        Menu_DrawRowBar(430, 16, 180, 0xFFB0B000, 2, 1);
        Text_DrawShadowRgb(430, 16, 0xFF202020, -1, buf);
        y = 336;
        WindowFrame_Draw(&gGalleryFrame, 0);
        Font_SetSpacing(18, 18);
        Font_SetPos(432, y);
        Text_PrintfColored(TXT(0x00838680) /* the name of the control for aiming */);
        Font_SetPos(432, y += 20);
        Text_PrintfColored(TXT(0x008386A0) /* what that control does (aiming the gun) */);
        Font_SetPos(432, y += 22);
        Text_PrintfColored(TXT(0x008386B0) /* the name of the control for shooting */);
        Font_SetPos(432, y + 20);
        Text_PrintfColored(TXT(0x008386D0) /* what that control does (firing) */);
        Gallery_RunGunDebug();
        return;
    case GALLERY_STEP_SCORE:
    case GALLERY_STEP_RESULT:
        level = gal->course;
        if (level == 0) {
            ybase = 80;
            height = 200;
            level = 1;
            rows = 1;
        } else {
            ybase = 40;
            height = 290;
            rows = 4;
        }
        Icon_Draw(110, ybase, 420, height, -8739, 0);
        Font_SetColor(0x80101010);
        Font_SetSpacing(20, 20);
        top = ybase;
        Font_SetPos(260, top + 25);
        Text_PrintfColored(gGalleryCourseTexts[gal->course]);
        Menu_DrawRowBar(245, top + 20, 160, 0xBDB59F6C, 2, 1);
        y = top + 35;
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
        Tex_Prepare(1, TEX_SLOT_PICTURES);
        Tex_Bind(TEX_SLOT_PICTURES);
        spr.color = -1;
        firstKind = level * 4 - 3;
        spr.w = 25;
        tri.color = 0xFF000000;
        spr.h = 25;
        savedAlphaTest = Gs_GetState(RS_ALPHA_TEST);
        savedBlend = Gs_GetState(RS_BLEND);
        Gs_SetState(RS_ALPHA_TEST, RS_CMP_ALWAYS);
        Gs_SetState(RS_BLEND, RS_BLEND_OPAQUE);
        nameX = 227.0f;
        lim = rows;
        i = 0;
        if (i < lim) {
            sprY = &spr.y;
            triY = &tri.y0;
            do {
            Font_SetPos(nameX, y += 32);
            monster = &gGalleryKindMonsters[firstKind + i];
            Text_PrintfColored("%-20sx%3d", gMonsterNames[*monster], gal->kindHits[i]);
            spr.x = 192;
            *sprY = y;
            tri.x0 = spr.x;
            *triY = *sprY;
            tri.x1 = 25.0f + spr.x;
            tri.y1 = *sprY;
            tri.x2 = spr.x;
            tri.y2 = 25.0f + *sprY;
            Prim_FillTri((struct PrimTri *)&tri);
            tri.x0 = 25.0f + spr.x;
            *triY = 25.0f + *sprY;
            Prim_FillTri((struct PrimTri *)&tri);
            EntryPicture_GetUV((s32 *)&spr.u0, (s32 *)&spr.u1, *monster);
            Sprite_Draw(&spr);
            } while (++i < lim);
        }
        Gs_SetState(RS_ALPHA_TEST, savedAlphaTest);
        Gs_SetState(RS_BLEND, savedBlend);
        lim = height;
        Font_SetPos(192, top + lim - 64);
        label = TXT(0x008386F0) /* the label of the points won */;
        if (gal->points > 0) {
            Text_PrintfColored("%-20s      P", label);
        } else {
            Text_PrintfColored("                       0 P", label);
        }
        if (gal->step == GALLERY_STEP_RESULT) {
            Lobby_DrawButtonIcon(412, top + lim - 35, 0);
            Font_SetPos(445, top + lim - 35);
            Text_PrintfColored(TXT(0x00838730) /* the label of the next button */);
            return;
        }
        break;
    case GALLERY_STEP_PRIZES:
        Gallery_DrawPrizeWindow(280, 0);
        return;
    case GALLERY_STEP_QUIT_ASK:
        Gallery_DrawPrizeWindow(280, 1);
        sprintf(buf, TXT(0x00838740) /* the question whether to quit */);
        Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, buf);
        Font_SetPos(418, 402);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

/*
 * Registers a gallery object: mode GALLERY_OBJ_TARGETS sets the target
 * board; otherwise puts a shot into the first free of the 10 shot slots.
 * Returns 1 when stored, 0 when all slots are taken.
 */
s32 Gallery_AddObj(StageObj *obj, u8 mode)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    s32 i;

    if (mode == 0) {
        gal->targets = obj;
        return 1;
    }
    for (i = 0; i < 10; i++) {
        if (gal->shots[i] == NULL) {
            gal->shots[i] = obj;
            return 1;
        }
    }
    return 0;
}

/*
 * Returns the number of shots in flight; 1 without counting when the
 * flag's low byte is 0.
 */
s32 Gallery_CountShots(s32 doCount)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    s32 i;
    s32 n;

    n = 0;
    if (!(doCount & 0xFF)) {
        return 1;
    }
    i = 0;
    do {
        if (gal->shots[i] != NULL) {
            n += 1;
        }
        i += 1;
    } while (i < 10);
    return n;
}

/*
 * Adds board target i's points to the round's points and flashes the
 * points display (Hud_FadeStart).
 */
void Gallery_ScoreTarget(void *board, u8 i)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;

    gal->points += GalleryBoard_GetTargetPoints(board, i);
    Hud_FadeStart(1, 510, 70);
}

/* Empties the 10 shot slots. */
void Gallery_ClearShots(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;

    memset(gal->shots, 0, sizeof(gal->shots));
}

u8 Capsule_HitsRect(f32 *ray, Vec3f *pos, s32 a, f32 c, f32 r); /* unit's own declaration: declared as the gallery calls it, with a shot's ray, a target's position and an angle as an int; with the definition's types (a capsule, a centre, a u16 angle) the call compiles differently */
u8 GalleryBoard_IsTargetDown(StageObj *o, u8 i);
extern Vec3f gGalleryWalls[];
extern f32 gGalleryWallYRange[];

/*
 * Updates shotsLive, then tests each shot in flight as a capsule: against
 * the 4 walls (gGalleryWalls; a wall hit ends it, LOBBY_SE_GALLERY_WALL),
 * then against the board's targets. A hit on a live, standing target
 * either downs it (points, the kind's hit count, kindB hits, the points
 * popup, a burst, LOBBY_SE_GALLERY_DOWN and the monster's cry) or only
 * swings it (LOBBY_SE_GALLERY_SWING). Empties the shot slots at the end.
 * The 64-byte buffer goes to GalleryShot_GetCapsule as a pointer: passed
 * as an int, its address would be kept in a saved register, which the
 * game's code doesn't do.
 */
void Gallery_UpdateShots(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    Vec3f hitPos;
    Vec3f wallPos;
    Vec3f wallDir;
    Vec3f hit;
    Vec3f targetPos;
    f32 ray[8];
    f32 mtx[16];
    s32 i;
    s32 j;
    s32 k;
    f32 halfTall;
    f32 halfSize;
    u8 level;
    u8 *slot;

    gal->shotsLive = Gallery_CountShots(1);
    for (i = 0; i < 10; i++) {
        if (gal->shots[i] == NULL) {
            continue;
        }
        GalleryShot_GetCapsule(ray, mtx, gal->shots[i], 3.0f);
        wallDir.x = 93.97f;
        wallDir.y = 0.0f;
        wallDir.z = -34.2f;
        for (j = 0; j < 4; j++) {
            wallPos.x = gGalleryWalls[j].x;
            wallPos.y = gGalleryWalls[j].y;
            wallPos.z = gGalleryWalls[j].z;
            if (Seg_ClipYRange(ray, gGalleryWallYRange[j], 0.0f, &wallPos, (f32 *) &wallDir, &hit)) {
                GalleryShot_SetHit(gal->shots[i]);
                GalleryShot_SpawnWallHit((struct Vec3f *)&hit, (struct Vec3f *)&wallDir);
                LobbySe_Play(LOBBY_SE_GALLERY_WALL);
                break;
            }
        }
        if (gal->targets == NULL || ((s32 *)gal->targets)[6] == 0) {
            return;
        }
        for (k = 0; k < GALLERY_TARGETS; k++) {
            halfSize = 0.5f * gGalleryTargetSizes[GalleryBoard_GetTargetSize(gal->targets, k)];
            halfTall = halfSize * Math_Cos(GalleryBoard_GetTargetTilt(gal->targets, k));
            GalleryBoard_GetTargetPos((struct Vec3f *)&targetPos, gal->targets, k);
            if (Capsule_HitsRect(ray, &targetPos, GalleryBoard_GetTargetFacing(gal->targets, k), halfTall, halfSize) && *(slot = &gal->slots[k]) > 0
                && GalleryBoard_IsTargetDown(gal->targets, k) == 0 && GalleryBoard_IsFlippedDown(gal->targets) == 0) {
                hitPos = targetPos;
                GalleryShot_SetHit(gal->shots[i]);
                if (GalleryBoard_HitTarget(gal->targets, k)) {
                    GalleryBoard_DownTarget(gal->targets, k);
                    Gallery_ScoreTarget(gal->targets, k);
                    if (gal->course == 0) {
                        level = 1;
                    } else {
                        level = gal->course;
                    }
                    gal->kindHits[(u8)(*slot - (level * 4 - 3))]++;
                    GalleryPoints_Spawn((struct Vec3f *)&hitPos, GalleryBoard_GetTargetPoints(gal->targets, k));
                    if (*slot == gal->cfg->kindB) {
                        gal->hitsB++;
                    }
                    GalleryBurst_SpawnDowned((struct Vec3f *)&hitPos);
                    LobbySe_Play(LOBBY_SE_GALLERY_DOWN);
                    Gallery_PlayHitSe(GalleryBoard_GetTargetPicture(gal->targets, k));
                } else {
                    GalleryBurst_SpawnHit((struct Vec3f *)&hitPos);
                    GalleryBoard_SwingTarget(gal->targets, k);
                    LobbySe_Play(LOBBY_SE_GALLERY_SWING);
                }
            }
        }
    }
    Gallery_ClearShots();
}


/*
 * Draws n shot marks, 20 to a line from (30, 50); the first `lit` of
 * them in the lit sprite, the rest in the spent sprite.
 */
void Gallery_DrawShotMarks(s8 count, s8 lit)
{
    Sprite spr;
    u8 i;
    u8 col = 0;
    s16 y = 50;
    s16 x;

    Tex_Prepare(1, 341);
    Tex_Bind(341);
    for (i = 0; i < count; i++) {
        if (col < 20) {
            x = col * 20 + 30;
            col++;
        } else {
            y += 20;
            col = 1;
            x = 30;
        }
        if (i < lit) {
            spr.u0 = 137;
            spr.v0 = 164;
            spr.u1 = 151;
            spr.v1 = 188;
        } else {
            spr.u0 = 149;
            spr.v0 = 164;
            spr.u1 = 163;
            spr.v1 = 186;
        }
        spr.color = -1;
        spr.w = 20;
        spr.h = 20;
        spr.y = y;
        spr.x = x;
        Sprite_Draw(&spr);
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/shooting_gallery", gGalleryLabelText);

extern u8 gGalleryLabelText[];
extern s16 gGalleryCourseBest[];
extern u8 gGalleryCourseLevels[];

/* the prize list of the shooting gallery: each prize, its points (or free),
 * the cursor; the round card below. Level 4 (the file is at 3): at 3, xs is re-extended
 * after the loop. */
#pragma optimization_level 4
/*
 * Draws the course menu: label bar, money, the 6 courses with fee (greyed
 * when too dear) or the free label, the blinking cursor on the current
 * course, the course card (time, shots, level, best) and, when the pouch
 * is full, the pouch-full prompt (also sets pouchFull).
 */
void Gallery_DrawPrizeList(s16 x)
{
    s16 idx;
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    s16 blink;
    s16 y;
    s16 i;
    s16 *feePtr;
    char **name;
    s16 left;
    s16 fee;
    s8 row;


    Gallery_DrawLabelBar((s32)gGalleryLabelText);
    Menu_DrawMoney(480, 16, 0);
    y = 50;
    Icon_Draw(x, y, 350, 306, -8739, 0);
    Font_SetSpacing(20, 20);
    left = x + 0;
    y += 20;
    Font_SetPos(left + 24, y);
    for (i = 0, feePtr = gGalleryCourseFees, name = gGalleryCourseTexts; i < 6; i++) {
        idx = i;
        if (idx == gal->course) {
            blink = (gFrameTick & 0x10) ? (s16)0 : (s16)6;
            Icon_Draw(left + 40, y + 14, 274, 8, 0x99106060, 111);
            Menu_DrawCursorIcon(left - blink, y - 2, 24, 0xFF20C0C0);
        }
        fee = *feePtr;
        if (fee > 0) {
            if (gal->tooDear[idx] > 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Text_PrintfColored("%-21s %6d%s", *name, fee, gLobbyWords[LOBBY_WORD_ZENNY]);
        } else {
            Font_SetColor(0x80101010);
            Text_PrintfColored("%-24s %s", *name, TXT(0x008387B0) /* the price shown when it costs nothing */);
        }
        Font_SetPos(left + 24, y += 22);
        feePtr++;
        name++;
    }
    row = gal->course;
    Gallery_DrawCourseCard(left + 24, 241, gGalleryCourseTimes[row], gGalleryCourseShots[row], gGalleryCourseBest[row], gGalleryCourseLevels[row]);
    Menu_DrawDashLine(left + 24, 230, 300, 0xFF000000);
    if (Gallery_IsPouchFull() == 1) {
        Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_PICK_COURSE]);
        gal->pouchFull = 1;
    }
}
#pragma optimization_level reset

/* Draws the screen's label bar with the given text at the top. */
void Gallery_DrawLabelBar(s32 label)
{
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(302, 18, 160, -0x4F5000, (char *)label, 2, 1);
}


/*
 * Draws a course card: the time limit and shots line, the difficulty and
 * best-score line, and the start prompt with a button icon.
 */
void Gallery_DrawCourseCard(s16 x, s16 y, s16 time, s8 shots, s16 best, u8 level)
{
    Font_SetSpacing(18, 18);
    Font_SetColor(0x80101010);
    Font_SetPos(x + 18, y);
    Text_PrintfColored(TXT(0x008387C0) /* the time limit and the number of shots */, time, shots);
    Font_SetPos(x + 18, y += 22);
    Text_PrintfColored(TXT(0x008387E0) /* the difficulty and the best score */, level, best);
    Font_SetPos(x + 210, y + 22);
    Font_SetPos(x + 220, y + 50);
    Text_PrintfColored(gListTitles[LIST_TITLE_PRIZE_CHECK]); /* the start prompt, the table's second line */
    Lobby_DrawButtonIcon(x + 190, y + 45, 3);
}

INCLUDE_RODATA("asm/lobby/nonmatchings/shooting_gallery", gGalleryPrizeHeadText);

INCLUDE_RODATA("asm/lobby/nonmatchings/shooting_gallery", gGalleryPrizeRowFmt);

extern char gGalleryPrizeHeadText[];
extern char gGalleryPrizeRowFmt[];
extern char *gItemNames[];

/*
 * Draws the prize page (7 prizes per page, 2 pages): label bar, money,
 * page number, the column header, each prize's icon, name and points
 * (greyed when locked), the cursor, the cursor prize's description, and
 * the pouch-full message when pouchFull is set.
 */
void Gallery_DrawPrizePage(s16 x)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    s16 left;
    s16 y;
    s16 blink;
    u8 first;
    u8 last;
    u8 i;

    Gallery_DrawLabelBar((s32)gGalleryLabelText);
    Menu_DrawMoney(480, 16, 0);
    y = 50;
    Icon_Draw(x, y, 350, 306, -8739, 0);
    left = x + 0;
    Menu_DrawPageNum(left + 265, 55, gal->page, 2, 1);
    Font_SetColor(0x80101010);
    Font_SetSpacing(20, 20);

    Font_SetPos(left + 20, y += 20);
    Text_PrintfColored(gGalleryPrizeHeadText);
    Font_SetPos(x, y += 22);
    first = gal->page * 7;
    last = first + 6;
    if (last >= 13) {
        last = 12;
    }
    for (i = first; i <= last; i++) {
        if (i % 7 == gal->cursor) {
            blink = (gFrameTick & 0x10) ? (s16)0 : (s16)6;
            Icon_Draw(left + 40, y + 14, 274, 8, 0x99106060, 111);
            Menu_DrawCursorIcon(left - blink, y - 2, 24, 0xFF20C0C0);
        }
        if (gal->locked[i] <= 0) {
            Font_SetColor(0x80101010);
        } else {
            Font_SetColor(0x8042352E);
        }
        Item_DrawIcon(left + 20, y, 20, gal->list[i].item);
        Font_SetPos(left + 50, y);
        Text_PrintfColored(gGalleryPrizeRowFmt, gItemNames[gal->list[i].item], gal->list[i].points);
        y += 22;
    }
    Font_SetColor(0x80101010);
    ItemInfo_Draw(x + 8, 252, 330, gal->details, gal->list[(u8)(gal->cursor + gal->page * 7)].item, 0, -1, 0x80101010);
    Menu_DrawDashLine(left + 20, 250, 300, 0xFF000000);
    if (gal->pouchFull == 1) {
        Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_PRIZES_NOW]); /* the pouch is full */
    }
}

/*
 * As Gallery_DrawPrizePage, under a frame with the course name and the
 * points won, and with the count of the cursor prize already chosen in the
 * description. When the full stacks were checked and `asking` is 0, shows
 * the stacks-full message.
 */
void Gallery_DrawPrizeWindow(s16 x, u8 asking)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    s16 left;
    s16 y;
    s16 blink;
    u8 first;
    u8 last;
    u8 i;
    u8 sel;
    u16 have;

    WindowFrame_Draw(&gGalleryPrizeFrame, 0);
    Font_SetColor(0x807F7F7F);
    Font_SetSpacing(20, 20);
    Font_SetPos(70, 70);
    Text_PrintfColored(gGalleryCourseTexts[gal->course]);
    Font_SetPos(40, 90);
    Text_PrintfColored(TXT(0x00838820) /* the points won, with their label and unit */, gal->points);
    Gallery_DrawLabelBar((s32)gGalleryLabelText);
    Menu_DrawMoney(480, 16, 0);
    y = 50;
    Icon_Draw(x, y, 350, 306, -8739, 0);
    left = x + 0;
    Menu_DrawPageNum(left + 265, 55, gal->page, 2, 1);
    Font_SetColor(0x80101010);
    Font_SetSpacing(20, 20);

    Font_SetPos(left + 20, y += 20);
    Text_PrintfColored(gGalleryPrizeHeadText);
    Font_SetPos(x, y += 22);
    first = gal->page * 7;
    last = first + 6;
    if (last >= 13) {
        last = 12;
    }
    for (i = first; i <= last; i++) {
        if (i % 7 == gal->cursor) {
            blink = (gFrameTick & 0x10) ? (s16)0 : (s16)6;
            Icon_Draw(left + 40, y + 14, 274, 8, 0x99106060, 111);
            Menu_DrawCursorIcon(left - blink, y - 2, 24, 0xFF20C0C0);
        }
        if (gal->locked[i] <= 0) {
            Font_SetColor(0x80101010);
        } else {
            Font_SetColor(0x8042352E);
        }
        Item_DrawIcon(left + 20, y, 20, gal->list[i].item);
        Font_SetPos(left + 50, y);
        Text_PrintfColored(gGalleryPrizeRowFmt, gItemNames[gal->list[i].item], gal->list[i].points);
        y += 22;
    }
    Font_SetColor(0x80101010);
    have = 0;
    sel = gal->cursor + gal->page * 7;
    for (i = 0; i < gal->chosenCount; i++) {
        if (gal->chosen[i] == ((PrizeEntry *)((u8 *)gal->list + sel * 4))->item) {
            have++;
        }
    }
    ItemInfo_Draw(x + 8, 252, 330, gal->details, gal->list[sel].item, have, -1, 0x80101010);
    Menu_DrawDashLine(left + 20, 250, 300, 0xFF000000);
    if (gal->stacksChecked == 1 && asking == 0) {
        Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_PICK_PRIZE]); /* no room for the prizes chosen */
    }
}

/*
 * 1 when the step is the window slide (1) or the close (10); the score
 * object ends then.
 */
s32 Gallery_InMenuOrClose(void)
{
    if (gMenuWork.prize.step == GALLERY_STEP_SLIDE_IN || gMenuWork.prize.step == GALLERY_STEP_CLOSE) {
        return 1;
    }
    return 0;
}

/*
 * 1 at the result screen (step 7) after the score was skipped; the score
 * object then shows its total.
 */
s32 Gallery_InResults(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;

    if (gal->step == GALLERY_STEP_RESULT && gal->skipped == 1) {
        return 1;
    }
    return 0;
}

/* Locks each of the 13 prizes whose points are above the points left. */
void Gallery_MarkTooDear(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    u8 i;

    for (i = 0; i < GALLERY_PRIZE_COUNT; i++) {
        if (gal->list[i].points > gal->points) {
            if (gal->locked[i] <= 0) {
                gal->locked[i] = 1;
            }
        }
    }
}

/*
 * Locks each prize whose chosen count has reached the pouch's room for
 * that item.
 */
void Gallery_MarkFullPrizes(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    u8 i;
    u8 chosen;
    u8 k;

    chosen = 0;
    for (i = 0; i < GALLERY_PRIZE_COUNT; i++) {
        for (k = 0; k < gal->chosenCount; k++) {
            if (gal->chosen[k] == gal->list[i].item) {
                chosen++;
            }
        }
        if ((s8)SavePouch_Space(gal->list[i].item) <= chosen) {
            if (gal->locked[i] <= 0) {
                gal->locked[i] = 1;
            }
        }
        chosen = 0;
    }
}

/*
 * Counts the chosen prizes (each item once) that would need a new pouch
 * slot; when that is at least the pouch's free slots, locks the prizes
 * needing a new slot (Gallery_MarkFullStacks).
 */
void Gallery_CheckPouchFull(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    s16 newSlots;
    u8 i;
    s16 room;

    newSlots = 0;
    for (i = 0; i < gal->chosenCount; i++) {
        room = SavePouch_Room(gal->chosen[i]);
        if (room == gItemData[gal->chosen[i]].max && Gallery_IsRepeatPrize(i) == 0) {
            newSlots++;
        }
    }
    if (SavePouch_CountFree() <= newSlots) {
        Gallery_MarkFullStacks();
    }
}

/*
 * 1 when the chosen prizes needing a new pouch slot (each item once) are
 * at least the free slots.
 */
s32 Gallery_IsPouchFull(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    s16 newSlots;
    u8 i;
    s16 room;

    newSlots = 0;
    for (i = 0; i < gal->chosenCount; i++) {
        room = SavePouch_Room(gal->chosen[i]);
        if (room == gItemData[gal->chosen[i]].max && Gallery_IsRepeatPrize(i) == 0) {
            newSlots++;
        }
    }
    return newSlots >= SavePouch_CountFree();
}

/*
 * 1 when chosen prize n's item is also an earlier chosen prize's, else 0.
 */
u8 Gallery_IsRepeatPrize(u8 idx)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    u8 repeat = 0;
    u8 i;

    for (i = 0; i < idx; i++) {
        if (gal->chosen[i] == gal->chosen[idx]) {
            repeat = 1;
        }
    }
    return repeat;
}

/* 1 when list prize k's item is among the chosen prizes, else 0. */
u8 Gallery_IsPrizeChosen(u8 prize)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    u8 chosen = 0;
    u8 i = 0;

    if (0 < gal->chosenCount) {
        do {
            /* c->list[k].item scales k before loading c->list */
            if (gal->chosen[i] == *(u16 *)((u8 *)gal->list + prize * 4)) {
                chosen = 1;
            }
            i++;
        } while (i < gal->chosenCount);
    }
    return chosen;
}

/*
 * Adds the cursor prize's item to the chosen list unless it is locked; at
 * 40 chosen, locks all.
 */
void Gallery_ChoosePrize(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    u16 sel;

    sel = gal->cursor + gal->page * 7;
    if (gal->locked[sel] <= 0) {
        gal->chosen[gal->chosenCount] = gal->list[sel].item;
        gal->chosenCount++;
        if (gal->chosenCount == 40) {
            Gallery_MarkAllTaken();
        }
    }
}

/* Flags each of the 6 courses whose fee is above the money held. */
void Gallery_MarkTooDearCourses(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    u8 i;
    s16 *fee;

    for (i = 0, fee = gGalleryCourseFees; i < 6; i++, fee++) {
        if (*fee > gSave.money) {
            gal->tooDear[i] = 1;
        }
    }
}

/*
 * After a paid round: adds each chosen prize to the pouch and pays the
 * played course's fee.
 */
void Gallery_TakePrize(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    u8 i;

    if (gal->played != 0) {
        for (i = 0; i < gal->chosenCount; i++) {
            SavePouch_Add(gal->chosen[i], 1);
        }
        Save_AddMoney(-gGalleryCourseFees[gal->playedCourse]);
    }
}

/*
 * Locks each prize whose item would need a new pouch slot and is not
 * chosen yet; sets `checked`.
 */
void Gallery_MarkFullStacks(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    u8 i;
    u8 room;

    for (i = 0; i < GALLERY_PRIZE_COUNT; i++) {
        room = SavePouch_Room(gal->list[i].item);
        if (room == gItemData[gal->list[i].item].max && (u8)Gallery_IsPrizeChosen(i) == 0) {
            gal->locked[i] = 1;
        }
    }
    gal->stacksChecked = 1;
}

/* Locks all 13 prizes and sets allTaken. */
void Gallery_MarkAllTaken(void)
{
    PrizeMenu *gal = (PrizeMenu *)gMenuWork.raw;
    u8 i;

    for (i = 0; i < GALLERY_PRIZE_COUNT; i++) {
        gal->locked[i] = 1;
    }
    gal->allTaken = 1;
}

/*
 * Plays the cry of the downed target's monster (its picture, an EM_KIND_*
 * id); others are silent.
 */
void Gallery_PlayHitSe(u16 picture)
{
    switch (picture) {
    case EM_KIND_VELOCIPREY:
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IOPREY:
    case EM_KIND_IODROME:
    case EM_KIND_GENPREY:
        LobbySe_Play(29);
        break;
    case EM_KIND_YIAN_KUT_KU:
        LobbySe_Play(30);
        break;
    case EM_KIND_GYPCEROS:
        LobbySe_Play(31);
        break;
    case EM_KIND_GRAVIOS:
    case EM_KIND_FELYNE_CART:
    case EM_KIND_DIABLOS:
    case EM_KIND_BASARIOS:
        LobbySe_Play(32);
        break;
    case EM_KIND_KHEZU:
        LobbySe_Play(33);
        break;
    case EM_KIND_RATHIAN:
        LobbySe_Play(34);
        break;
    case EM_KIND_KIRIN:
        LobbySe_Play(35);
        break;
    case EM_KIND_BLANGONGA:
        LobbySe_Play(36);
        break;
    case EM_KIND_CONGALALA:
        LobbySe_Play(37);
        break;
    case EM_KIND_DAIMYO_HERMITAUR:
        LobbySe_Play(38);
        break;
    case EM_KIND_SHOGUN_CEANATAUR:
        LobbySe_Play(39);
        break;
    case EM_KIND_BULLDROME:
        LobbySe_Play(40);
        break;
    }
}

/* The courses' best scores and levels, and the gun's position (x, y, z, 0). */
s16 gGalleryCourseBest[6] = {
#include "lobby/007EC5C8.inc"
};
u8 gGalleryCourseLevels[8] = {
#include "lobby/007EC5D8.inc"
};
f32 gGalleryGunPos[4] = {
#include "lobby/007EC5E0.inc"
};
