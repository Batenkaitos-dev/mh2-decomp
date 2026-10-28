/*
 * select: the PSP link, which connects to Monster Hunter Portable on a
 * PSP over USB (the overlay's last file with code, with its .data, .rodata
 * and .bss).
 *
 * Psp_ConnectTask runs the screens after the character selection: it
 * loads the IOP module for the PSP's USB link and starts Psp_Thread, which
 * talks to the device through sceDevctl. The PSP's answers pick what the
 * link offers (Psp_SelectFeatures), and the player moves a cursor over the
 * offered icons (gPspCursorGrid). The screens are drawn from PspWork
 * (select.h, in the select work area at 0x100): 32 actors (the sliding and
 * swinging screens, Psp_UpdateActor and Psp_DrawActor) and 32 animated
 * objects (PspObj, gPspObjAnims), then the step's text lines (Psp_DrawTexts).
 */
#include "common.h"
#include "window.h"
#include "syssettings.h"
#include "stageobj.h"
#include "stream.h"
#include "screen_fade.h"
extern u16 gFrameTick; /* sub_main: the frame counter */
#include "pad.h"
#include "render_state.h"
#include "select.h"
#include "syssave.h"
#include "save.h"
#include "quest.h"
#include "sif.h"
#include "texture.h"
#include "prim.h"
#include "sound.h"
#include "text.h"
#include "engine_math.h"
#include "file_load.h"
#include "font.h"
#include "frame.h"
#include "main.h"
#include "mem_util.h"
#include "packet_tim2.h"
#include "rand.h"
#include "game_util.h"

s32 func_114C60(s32, void *);       /* StartThread */
extern s32 gOvlTex[];
extern char _gp;


s32 sceSifLoadStartModule(char *, s32, char *, s32 *);
s32 sceSifStopModule(s32 modid, s32 args, const char *argp, s32 *result); /* main's libsif: sceSifStopModule's arguments */

/* .bss, declared from the highest address down (MWCC emits uninitialized
 * variables last-declared first). */
u8 gPspThreadStack[4208];
u8 gPspDevctlArg[16];
u8 gPspDevPath[64];
s32 gPspState;
s32 gPspThreadId;
PspWork *gPspWork;

/* .data */
Frame gPspFrameA = {
#include "select/006CC1E0.inc"
};
Frame gPspFrameB = {
#include "select/006CC200.inc"
};
s16 gPspAnimCells0[312] = {
#include "select/006CC220.inc"
};
PspAnim gPspObjAnims_0[50] = {
#include "select/006CC490.inc"
};
PspAnim gPspObjAnims_8[6] = {
#include "select/006CC940.inc"
};
PspAnim gPspObjAnims_9[1] = {
#include "select/006CC9D0.inc"
};
s16 gPspAnimCells1[40] = {
#include "select/006CC9F0.inc"
};
PspAnim gPspObjAnims_1[10] = {
#include "select/006CCA40.inc"
};
s16 gPspAnimCells2[112] = {
#include "select/006CCB30.inc"
};
PspAnim gPspObjAnims_2[22] = {
#include "select/006CCC10.inc"
};
PspAnim gPspObjAnims_3[8] = {
#include "select/006CCE20.inc"
};
s16 gPspAnimCells4[48] = {
#include "select/006CCEE0.inc"
};
PspAnim gPspObjAnims_4[6] = {
#include "select/006CCF40.inc"
};
PspAnim gPspObjAnims_5[6] = {
#include "select/006CCFD0.inc"
};
s16 gPspAnimCells6[72] = {
#include "select/006CD060.inc"
};
PspAnim gPspObjAnims_6[14] = {
#include "select/006CD0F0.inc"
};
s16 gPspAnimCells7[112] = {
#include "select/006CD240.inc"
};
PspAnim gPspObjAnims_7[18] = {
#include "select/006CD320.inc"
};
PspAnim *gPspObjAnims[12] = {
    gPspObjAnims_0, gPspObjAnims_1, gPspObjAnims_2, gPspObjAnims_3,
    gPspObjAnims_4, gPspObjAnims_5, gPspObjAnims_6, gPspObjAnims_7,
    gPspObjAnims_8, gPspObjAnims_9, NULL, NULL,
};
s16 gPspObjParams[56] = {
#include "select/006CD500.inc"
};
s16 gPspFrameRects[8][4] = {
#include "select/006CD570.inc"
};
u32 gPspFrameColors[8] = {
#include "select/006CD5B0.inc"
};
s8 gPspMarkKind[8] = {
#include "select/006CD5D0.inc"
};
s8 gPspIconIndex[8] = {
#include "select/006CD5D8.inc"
};
s8 gPspCursorGrid[4][4] = {
#include "select/006CD5E0.inc"
};
s8 gPspCursorCols[5][3] = {
#include "select/006CD5F0.inc"
};
s16 gPspStepTextPos[12][2] = {
#include "select/006CD600.inc"
};
s16 gPspLinePos[4][2] = {
#include "select/006CD630.inc"
};
PspPart gPspActorAnims[12] = {
#include "select/006CD640.inc"
};
s8 gPspActorInit[80] = {
#include "select/006CD700.inc"
};
s16 gPspIconPos[12] = {
#include "select/006CD750.inc"
};
s8 gPspIconActors[8] = {
#include "select/006CD768.inc"
};
s8 gPspIconLists[32] = {
#include "select/006CD770.inc"
};
s16 gPspViewRects[48] = {
#include "select/006CD790.inc"
};
/* Psp_UpdateActor: the flying screens' speeds */
s16 gPspActorSpeeds[4] = {
#include "select/006CD7F0.inc"
};


void Psp_PlayDecideSe(void)
{
    Sound_PlaySeFlat(0, 19, 0);
}

void Psp_PlayCancelSe(void)
{
    Sound_PlaySeFlat(0, 20, 0);
}

void Psp_PlayMoveSe(void)
{
    Sound_PlaySeFlat(7, 38, 0);
}

void Psp_PlayStopSe(void)
{
    Sound_PlaySeFlat(7, 37, 0);
}

void Psp_PlayConfirmSe(void)
{
    Sound_PlaySeFlat(1, 115, 0);
}

/* Starts icon screen i's swing: its amplitude, phase, speed and decay. */
void Psp_StartIconSwing(s32 icon)
{
    gPspWork->swingAmp[icon] = 6144;
    gPspWork->swingPhase[icon] = 0;
    gPspWork->swingSpeed[icon] = 16384;
    gPspWork->swingDecay[icon] = 2048;
}

const char gPspDevName[10] = {
#include "select/006D0720.inc"
};

s32 sceDevctl(const char *, s32, void *, s32, void *, s32);  /* devctl */
/* unit's own declaration: declared s32 as Psp_ConnectTask uses it; with the definition's u8 return, the call compiles differently */
s32 NetMenu_Update(void);
void Gfx_DrawBlackLineFixed(void);

/* CONNECT's task, every frame: waits for the character selection's end,
 * connects to the PSP (step 4, by gPspWork->unk0: the link, the features,
 * the objects sent and the icon grid), then leaves to the game (5) or
 * back (6, 8). */
/*
 * The PSP link's task, every frame (PspMode): starts the character
 * selection and waits for its end, asks to connect, runs the link
 * (PspStep: wait for a PSP, the thread's exchange, the features picked,
 * the show of the objects sent and the icon screen), then leaves to
 * gm_sub's menu (PSP_MODE_EXIT_TO_MENU) or on with the character
 * (PSP_MODE_EXIT).
 * The cursor's moves on the icon screen search the columns or rows of
 * gPspCursorGrid for an icon with something found; the gotos leave a
 * search when it finds one or runs out, and go on to the next button's
 * test, as the game's code does; cancel during the show jumps into
 * PSP_SHOW_WAIT's end (goto stop). Forms the match needs: each column search
 * is an empty-then if around a do-while that takes the column's table row
 * in colCells (the game's out-of-line preheader with the row hoisted; a
 * while reloads the row); the right search has its own row and counter
 * (j, rowsTried); the up and down searches and the loops after a left move
 * index the tables instead of walking them, with the column in a variable
 * of their own (i).
 */
void Psp_ConnectTask(TaskSlot *task)
{
    s32 prevIcon;
    s32 icon;
    s32 col;
    s32 row;
    s32 k;
    s32 i;
    s32 rowsTried;
    u16 trig;
    s32 j;
    s8 *colCells;

    trig = gPadState[0].pressed;
    switch (task->mode) {
    case PSP_MODE_START:
        FadeA_Reset();
        gSysSettings.galleryPlay = 2;
        gSysSettings.pspCharaWait = 1;
        TaskSlot_Set(Chara_SelectTask, 4);
        task->mode++;
    case PSP_MODE_WAIT_CHARA:
        if (gSysSettings.pspCharaWait != 0) {
            return;
        }
        Game_ResetSubsystems();
        Sound_LoadPspBank();
        Tex_LoadSystemPack(0);
        task->mode++;
    case PSP_MODE_INIT:
        Psp_Init();
        task->mode++;
        FadeA_Start(2);
        break;
    case PSP_MODE_PROMPT:
        if ((u8)FadeA_GetState() != 1) {
            gPspWork->textStep = PSP_TEXT_PROMPT;
            if (trig & PAD_CROSS) {
                Psp_PlayCancelSe();
                Psp_Shutdown(gPspWork);
                task->mode = PSP_MODE_CANCELLED;
            } else if (trig & PAD_CIRCLE) {
                Psp_PlayDecideSe();
                Psp_StartThread(gPspWork);
                Psp_InitItemMask();
                task->mode++;
                gPspWork->timer = 30;
            }
        }
        break;
    case PSP_MODE_CONNECT:
        switch (gPspWork->step) {
        case PSP_STEP_WAIT_PSP:
            gPspWork->textStep = PSP_TEXT_CONNECTING;
            if (gPspState == PSP_THREAD_CLOSE
                || sceDevctl(gPspDevName, PSP_DEVCTL_PRESENT, NULL, 0, &gPspWork->pspPresent, 4) != 0
                || gPspWork->pspPresent == 0) {
                if (trig & PAD_CROSS) {
                    Psp_PlayCancelSe();
                    Psp_Shutdown(gPspWork);
                    task->mode = PSP_MODE_CANCELLED;
                }
            } else {
                gPspWork->step++;
                gPspState = PSP_THREAD_EXCHANGE;
            }
            break;
        case PSP_STEP_EXCHANGE:
            gPspWork->textStep = PSP_TEXT_CONNECTING;
            if (gPspState == PSP_THREAD_EXCHANGE) {
                if (trig & PAD_CROSS) {
                    Psp_PlayCancelSe();
                    gPspWork->textStep = PSP_TEXT_EXCHANGE_CANCELLED;
                    gPspWork->step = PSP_STEP_ERROR;
                    gPspState = PSP_THREAD_IDLE;
                    Psp_Shutdown(gPspWork);
                }
                break;
            }
            if (gPspState == PSP_THREAD_ERROR) {
                gPspWork->textStep = PSP_TEXT_LINK_ERROR;
                gPspWork->step = PSP_STEP_ERROR;
                gPspState = PSP_THREAD_IDLE;
                Psp_Shutdown(gPspWork);
                break;
            }
            gPspWork->step++;
        case 2:
            gPspWork->step++;
        case 3:
            gPspWork->step++;
        case PSP_STEP_DONE:
            gPspWork->textStep = PSP_TEXT_EXCHANGED;
            Psp_Shutdown(gPspWork);
            Psp_SelectFeatures();
            gPspWork->step = PSP_STEP_RESULT;
            gPspWork->timer = 1;
            break;
        case PSP_STEP_ERROR:
            if (trig & PAD_CIRCLE) {
                Psp_PlayDecideSe();
                Psp_Shutdown(gPspWork);
                task->mode = PSP_MODE_PROMPT;
                gPspWork->unk44 = 0;
            }
            break;
        case PSP_STEP_RESULT:
            if (trig & PAD_CIRCLE) {
                Psp_PlayDecideSe();
                gPspWork->textStep = PSP_TEXT_NONE;
                if (gPspWork->spawnLeft == 0) {
                    FadeA_Start(11);
                    Stream_Reset();
                    task->mode = PSP_MODE_FADE_DONE;
                } else {
                    Stream_Play(0, 46);
                    gPspWork->step++;
                    gPspWork->subStep = PSP_SHOW_SPAWN;
                    gPspWork->timer = 1;
                    gPspWork->headerKind = 0;
                }
            }
            break;
        case PSP_STEP_SHOW:
            switch (gPspWork->subStep) {
            case PSP_SHOW_SPAWN:
                gPspWork->textStep = PSP_TEXT_SHOW;
                if (--gPspWork->timer == 0) {
                    for (row = 0; row < 16; row++) {
                        if (gPspWork->spawnQueue[row] != 0) {
                            break;
                        }
                    }
                    if (row == 16) {
                        gPspWork->spawnLeft = 0;
                        gPspWork->subStep++;
                        break;
                    }
                    Psp_SpawnObject(gPspWork->spawnQueue[row], 1);
                    gPspWork->spawnQueue[row] = 0;
                    gPspWork->spawnLeft--;
                    if (gPspWork->spawnLeft == 0) {
                        gPspWork->subStep++;
                        break;
                    }
                    gPspWork->timer = 90;
                }
                if (trig & PAD_CROSS) {
                    Psp_PlayCancelSe();
                    gPspWork->subStep++;
                    goto stop;
                }
                break;
            case PSP_SHOW_WAIT:
                gPspWork->textStep = PSP_TEXT_SHOW;
                if (Psp_AnyObjectActive() == 0 || (trig & PAD_CROSS)) {
                    Psp_PlayDecideSe();
                stop:
                    Psp_PlayStopSe();
                    gPspWork->textStep = PSP_TEXT_NONE;
                    Stream_FadeOut(0, 10);
                    memset(gPspWork->objs, 0, sizeof(gPspWork->objs));
                    gPspWork->subStep++;
                    gPspWork->timer = 30;
                }
                break;
            case PSP_SHOW_ICONS:
                if (gPspWork->timer != 0) {
                    gPspWork->timer--;
                } else {
                    gPspWork->textStep = PSP_TEXT_ICONS;
                }
                for (k = 0; k < PSP_ICON_COUNT; k++) {
                    if (gPspWork->swingAmp[k] != 0) {
                        gPspWork->swingPhase[k] += gPspWork->swingSpeed[k];
                        if (gPspWork->swingPhase[k] == 0) {
                            gPspWork->swingPhase[k] = 0;
                            gPspWork->swingAmp[k] -= gPspWork->swingDecay[k];
                            if (gPspWork->swingAmp[k] < 0) {
                                gPspWork->swingAmp[k] = 0;
                            }
                        }
                    }
                }
                if (trig & PAD_RIGHT) {
                    col = gPspWork->cursorCol;
                    prevIcon = gPspWork->cursorIcon;

                    if (col == 3) { /* compiled-out check */
                    } else {
                        do {

                            col++;
                            j = gPspWork->cursorRow;
                            colCells = gPspCursorCols[col];
                            for (rowsTried = 0; rowsTried < 3; rowsTried++) {
                                icon = colCells[j];
                                if (icon != -1 && gPspWork->iconFound[gPspIconIndex[icon]] != 0) {
                                    break;
                                }
                                j++;
                                if (j > 2) {
                                    j = 0;
                                }
                            }
                            if (rowsTried < 3) {
                                goto found_r;
                            }
                        } while (col != 3);
                    }
                    goto try_left;
                found_r:
                        gPspWork->cursorRow = j;
                        icon = gPspCursorGrid[gPspWork->cursorRow][col];
                        if (icon == 2) {
                            gPspWork->cameFrom2 = gPspWork->cursorIcon;
                            gPspWork->cursorIcon = gPspCursorGrid[gPspWork->cursorRow][col];
                            gPspWork->cursorCol = col;
                        } else if (icon == 4) {
                            gPspWork->cameFrom4 = gPspWork->cursorIcon;
                            gPspWork->cursorIcon = gPspCursorGrid[gPspWork->cursorRow][col];
                            gPspWork->cursorCol = col;
                        } else {
                            gPspWork->cursorIcon = icon;
                            gPspWork->cursorCol = col;
                        }
                        Psp_PlayMoveSe();
                        Psp_StartIconSwing(gPspCursorGrid[gPspWork->cursorRow][gPspWork->cursorCol]);
                        goto done;
                }
            try_left:
                if (trig & PAD_LEFT) {
                    col = gPspWork->cursorCol;

                    if (col == 0) { /* compiled-out check */
                    } else {
                        do {

                            col--;
                            row = gPspWork->cursorRow;
                            colCells = gPspCursorCols[col];
                            for (k = 0; k < 3; k++) {
                                icon = colCells[row];
                                if (icon != -1 && gPspWork->iconFound[gPspIconIndex[icon]] != 0) {
                                    break;
                                }
                                row++;
                                if (row > 2) {
                                    row = 0;
                                }
                            }
                            if (k < 3) {
                                goto found_l;
                            }
                        } while (col != 0);
                    }
                    goto try_down;
                found_l:
                        gPspWork->cursorRow = row;
                        if (prevIcon == 2 && gPspWork->cameFrom2 != -1) {
                            gPspWork->cursorIcon = gPspWork->cameFrom2;
                            for (row = 0; row < 3; row++) {
                                for (i = 0; i < 4; i++) {
                                    if (gPspWork->cursorIcon == gPspCursorGrid[row][i]) {
                                        gPspWork->cursorCol = i;
                                        gPspWork->cursorRow = row;
                                        gPspWork->cursorIcon = gPspCursorGrid[gPspWork->cursorRow][gPspWork->cursorCol];
                                        break;
                                    }
                                }
                            }
                        }
                        if (prevIcon == 4 && gPspWork->cameFrom4 != -1) {
                            gPspWork->cursorIcon = gPspWork->cameFrom4;
                            for (row = 0; row < 3; row++) {
                                for (i = 0; i < 4; i++) {
                                    if (gPspWork->cursorIcon == gPspCursorGrid[row][i]) {
                                        gPspWork->cursorCol = i;
                                        gPspWork->cursorRow = row;
                                        gPspWork->cursorIcon = gPspCursorGrid[gPspWork->cursorRow][gPspWork->cursorCol];
                                        break;
                                    }
                                }
                            }
                        }
                        gPspWork->cursorIcon = gPspCursorGrid[gPspWork->cursorRow][col];
                        gPspWork->cursorCol = col;
                        Psp_PlayMoveSe();
                        Psp_StartIconSwing(gPspCursorGrid[gPspWork->cursorRow][gPspWork->cursorCol]);
                        goto done;
                }
            try_down:
                if (trig & PAD_DOWN) {
                    row = gPspWork->cursorRow;
                    col = gPspWork->cursorCol;
                    icon = gPspCursorGrid[row][col];
                    if (icon != 2 && icon != 4) {
                        for (;;) {
                            if (row == 2) {
                                goto try_up;
                            }
                            row++;
                            icon = gPspCursorGrid[row][gPspWork->cursorCol];
                            if (icon == -1) {
                                continue;
                            }
                            if (gPspWork->iconFound[gPspIconIndex[icon]] != 0) {
                                break;
                            }
                        }
                        gPspWork->cursorIcon = gPspCursorGrid[row][col];
                        gPspWork->cursorRow = row;
                        Psp_PlayMoveSe();
                        Psp_StartIconSwing(gPspCursorGrid[gPspWork->cursorRow][gPspWork->cursorCol]);
                        break;
                    }
                }
            try_up:
                if (trig & PAD_UP) {
                    row = gPspWork->cursorRow;
                    col = gPspWork->cursorCol;
                    icon = gPspCursorGrid[row][col];
                    if (icon != 2 && icon != 4) {
                        for (;;) {
                            if (row == 0) {
                                goto try_confirm;
                            }
                            row--;
                            icon = gPspCursorGrid[row][gPspWork->cursorCol];
                            if (icon == -1) {
                                continue;
                            }
                            if (gPspWork->iconFound[gPspIconIndex[icon]] != 0) {
                                break;
                            }
                        }
                        gPspWork->cursorIcon = gPspCursorGrid[row][col];
                        gPspWork->cursorRow = row;
                        Psp_PlayMoveSe();
                        Psp_StartIconSwing(gPspCursorGrid[gPspWork->cursorRow][gPspWork->cursorCol]);
                        break;
                    }
                }
            try_confirm:
                if (trig & PAD_CIRCLE) {
                    Psp_PlayConfirmSe();
                    gPspWork->textStep = PSP_TEXT_NONE;
                    FadeA_Start(11);
                    Stream_Reset();
                    task->mode = PSP_MODE_FADE_DONE;
                }
            done:
                break;
            }
            break;
        }
        break;
    case PSP_MODE_EXIT_TO_MENU:
        if ((u8)FadeA_GetState() != 1) {
            Psp_Release();
            Stream_Stop(0);
            TaskSlot_End(task);
            gSysSettings.systemSaved = 0;
            Task_StartBootToGmSub();
            FadeA_Start(12);
            return;
        }
        break;
    case PSP_MODE_FADE_DONE:
        if ((u8)FadeA_GetState() != 1) {
            FadeA_Reset();
            Stream_Reset();
            task->mode = PSP_MODE_EXIT;
            return;
        }
        break;
    case PSP_MODE_NET_MENU:
        if ((u8)NetMenu_Update()) {
            task->mode++;
            FadeA_Start(7);
        }
        Gfx_DrawBlackLineFixed();
        Frame_Draw();
        return;
    case PSP_MODE_EXIT:
        Psp_Release();
        Stream_Stop(0);
        TaskSlot_End(task);
        Task_StartBootToGmSubKeep();
        FadeA_Start(12);
        return;
    case PSP_MODE_CANCELLED:
        gPspWork->textStep = PSP_TEXT_CANCELLED;
        if (trig & PAD_CIRCLE) {
            Psp_PlayDecideSe();
            FadeA_Start(11);
            task->mode = PSP_MODE_EXIT_TO_MENU;
        }
        break;
    }
    Psp_UpdateScene();
    Callbacks_Add(Psp_Draw);
    Callbacks_Add(Psp_DrawTexts);
}

/*
 * Sets up the link's work at 0x100 in the select work area: clears it,
 * places the three exchange buffers after it, loads the files and sets up
 * the actors. No module is loaded and the thread is idle.
 */
void Psp_Init(void)
{
    u8 *base;

    base = gSysSettings.selWork;
    gPspWork = (PspWork *)(base + 256);
    memset(gPspWork, 0, sizeof(PspWork));
    memset(gPspWork->recvBuf = (s32 *)(base + 5568), 0, PSP_BUF_SIZE);
    memset(gPspWork->sendBuf = (s32 *)(base + 5632), 0, PSP_BUF_SIZE);
    memset(gPspWork->spareBuf = (s32 *)(base + 5696), 0, PSP_BUF_SIZE);
    Psp_LoadFiles(gPspWork->tex);
    Psp_InitActors();
    gPspWork->headerKind = 11;
    gPspWork->modId = -1;
    gPspState = PSP_THREAD_IDLE;
}

/* Forgets the work and frees the link's textures. */
void Psp_Release(void)
{
    gPspWork = NULL;
    Psp_FreeTextures();
}

/*
 * Resets the link for a new exchange (device closed, first step, both
 * buffers cleared) and writes into sendBuf the bits of the 14 items the
 * player has unlocked (Psp_IsItemUnlocked).
 */
void Psp_InitItemMask(void)
{
    s32 *mask;
    s32 i;

    gPspWork->fd = -1;
    gPspWork->step = 0;
    memset(gPspWork->sendBuf, 0, PSP_BUF_SIZE);
    memset(gPspWork->recvBuf, 0, PSP_BUF_SIZE);
    mask = gPspWork->sendBuf;
    for (i = 0; i < 14; i++) {
        if (i == 9) {
            if (Save_TestEventFlag(EVENT_FLAG_KUSHALA_SEEN) == 0) {
                continue;
            }
        }
        if (i == 10) {
            if (Save_IsQuestCleared(QUEST_ID_KUSHALA_SNOW_SCENE) == 0) {
                continue;
            }
        }
        *mask |= 1 << i;
    }
}

/*
 * Loads and starts an IOP module, retrying until the load succeeds.
 * Returns its id, or -1 when the module did not stay resident.
 */
s32 Psp_LoadModule(char *path, s32 argc, char *argv)
{
    s32 modResult;
    s32 ret;

    do {
        ret = sceSifLoadStartModule(path, argc, argv, &modResult);
    } while (ret < 0);
    if (modResult == 1) {
        ret = -1;
    }
    return ret;
}

/* Stops IOP module id and unloads it, retrying each until it succeeds. */
void Psp_UnloadModule(s32 id)
{
    s32 modResult;

    do {

    } while (sceSifStopModule(id, 0, 0, &modResult) < 0);
    do {

    } while (sceSifUnloadModule(id) < 0);
}

/*
 * Loads the USB module for the PSP if it isn't loaded and, once it is,
 * creates and starts the link thread (Psp_Thread) unless it exists.
 */
void Psp_StartThread(PspWork *work)
{
    struct ThreadParam param;
    s32 id;
    void *gpValue;

    if (work->modId < 0) {
        id = Psp_LoadModule("cdrom0:\\IOP\\MODULES\\USBPSPCM.IRX;1", 32, "-psp=1\0-unit=1\0-title=ULJM05066");
        if (id >= 0) {
            work->modId = id;
            Sys_EmptyHook();
            if (gPspThreadId == 0) {
                gPspState = PSP_THREAD_IDLE;
                param.entry = Psp_Thread;
                param.stack = gPspThreadStack;
                param.stackSize = 4096;
                __asm__ volatile ("move %0, $gp" : "=r"(gpValue));
                param.gpReg = gpValue;
                param.initPriority = 40;
                gPspThreadId = CreateThread(&param);
                func_114C60(gPspThreadId, 0);
            }
        }
    }
}



/*
 * Stops the link thread; when the module is loaded, ends and deletes the
 * thread and unloads the module.
 */
void Psp_Shutdown(PspWork *work)
{
    gPspState = PSP_THREAD_STOP;
    if (work->modId >= 0) {
        if (gPspThreadId != 0) {
            TerminateThread(gPspThreadId);
            DeleteThread(gPspThreadId);
        }
        Psp_UnloadModule(work->modId);
        work->modId = -1;
        gPspThreadId = 0;
        Sys_EmptyHook();
    }
}

/*
 * Picks what the link offers this time: the features of recvBuf's first
 * word whose event flag isn't set yet (and whose conditions hold), then the
 * menus they appear in, some at random.
 */
void Psp_SelectFeatures(void)
{
    s32 *recv;
    s32 mask;
    s32 i;
    s32 bit;

    recv = gPspWork->recvBuf;
    *recv |= 2;
    *recv |= 0x800;
    PspLink_AddCount();
    EventFlag_Set(53);
    Save_ClearEventFlag(49);
    mask = 0;
    for (i = 0; i < 12; i++) {
        bit = 1 << i;
        if (*recv & bit) {
            switch (i) {
            case 0:
                if (Save_TestEventFlag(120) == 0) {
                    mask |= bit;
                }
                break;
            case 1:
                if (Save_TestEventFlag(121) == 0 && Save_TestEventFlag(6) == 1) {
                    mask |= bit;
                }
                break;
            case 2:
                if (Save_TestEventFlag(122) == 0 && Save_TestEventFlag(182) == 1 && gSave.styleBits != 0
                    && !(gSave.rateFlags & 8)) {
                    mask |= bit;
                }
                break;
            case 3:
                mask |= bit;
                break;
            case 4:
                if (gSysSave.bonusUnlocked == 0 && Save_TestEventFlag(124) == 0) {
                    mask |= bit;
                }
                break;
            case 5:
                if (Save_TestEventFlag(125) == 0) {
                    mask |= bit;
                }
                break;
            case 6:
                if (Save_TestEventFlag(126) == 0) {
                    mask |= bit;
                }
                break;
            case 7:
                if (Save_TestEventFlag(127) == 0) {
                    mask |= bit;
                }
                break;
            case 8:
                if (Psp_IsItemUnlocked(10) == 1 && Save_TestEventFlag(130) == 0) {
                    mask |= bit;
                }
                break;
            case 9:
                if ((gSave.monsterFlags & 0x400) && Save_TestEventFlag(131) == 0) {
                    mask |= bit;
                }
                break;
            case 10:
                if (Save_TestEventFlag(73) == 0) {
                    mask |= bit;
                }
                break;
            case 11:
                mask |= bit;
                break;
            }
        }
    }
    PspLink_SetFlags(mask);
    for (i = 0; i < 11; i++) {
        if (mask & (1 << i)) {
            gPspWork->featureOn[i] = 1;
            switch (i) {
            case 0:
            case 3:
            case 6:
            case 10:
                gPspWork->menuObjType[1] = 6;
                break;
            case 2:
            case 7:
            case 8:
            case 9:
                gPspWork->menuObjType[4] = Rand_Next(1) % 2 + 4;
                break;
            case 1:
                gPspWork->menuObjType[3] = 1;
                break;
            case 5:
                gPspWork->menuObjType[2] = Rand_Next(1) % 2 + 2;
                break;
            case 4:
                gPspWork->menuObjType[6] = 7;
                break;
            }
        }
    }
    gPspWork->spawnLeft = 0;
    for (i = 0; i < 16; i++) {
        gPspWork->spawnQueue[i] = gPspWork->menuObjType[i];
        if (gPspWork->spawnQueue[i] != 0) {
            gPspWork->spawnLeft++;
        }
    }
    gPspWork->offerMask = mask;
}

/* Loads archive 39 and passes up to 13 of its files to Tex_CreateFromTim2; the
   results go to gOvlTex and out. The loop keeps the original's shape: i is
   set before the test, p after it. */
void Psp_LoadFiles(s32 *out)
{
    PspFileEnt *ent;
    u8 *buf;
    s32 count;
    s32 *texSlot;
    s32 i;

    buf = gSysSettings.loadBuf;
    if (LoadCompressedFile(buf, 39) > 0) {
        count = *(s32 *)(buf + 4);
        ent = (PspFileEnt *)(buf + 8);
        if (count > 13) {
            count = 13;
        }
        i = 0;
        if (i < count) {
            texSlot = gOvlTex;
            do {
                *texSlot = Tex_CreateFromTim2(buf + (ent++)->offset, TEX_MODE_ON_DEMAND);
                out[i] = *texSlot;
                texSlot++;
                i++;
            } while (i < count);
        }
        Mem_Compact();
    }
}

/* Frees the link's texture handles and compacts the memory. */
void Psp_FreeTextures(void)
{
    Tex_FreeRange(PSP_TEX_FIRST, PSP_FILE_COUNT);
    Mem_Compact();
}

/*
 * Sets up the actors from gPspActorInit (per actor: its index, visible,
 * part or -1, kind, large), all at scale 1, and clears the objects.
 */
void Psp_InitActors(void)
{
    s32 i;
    s8 *init;
    PspActor *actor;

    init = gPspActorInit;
    for (i = 0; i < 16; i++) {
        actor = &gPspWork->actors[*init++];
        actor->exists = 1;
        actor->scaleX = 1.0f;
        actor->scaleY = 1.0f;
        actor->visible = *init++;
        if (*init >= 0) {
            actor->part = &gPspActorAnims[*init++];
        } else {
            actor->part = NULL;
            init++;
        }
        actor->kind = *init++;
        if (actor->kind != 0) {
            actor->relative = 1;
            actor->parent = &gPspWork->actors[1];
        }
        actor->large = *init++;
    }
    gPspWork->actorCount = PSP_ACTORS_USED;
    gPspWork->sceneState = 0;
    memset(gPspWork->objs, 0, sizeof(gPspWork->objs));
}

/* Marks the icons whose items are there: for each of the six icons, the
 * first entry of its list (gPspIconLists, 4 each, -1: none) that
 * PspLink_HasFlag accepts; its actor and the next one get their place and
 * scale. Then the last icon with one is looked up in gPspCursorGrid (3 rows
 * of 4). */
void Psp_MarkIcons(void)
{
    s8 *row;
    PspActor *actor;
    s32 i;
    s8 item;
    s32 j;
    s8 *entry;
    s8 *icon;
    s16 *pos;
    s8 *act;
    s8 *kind;
    s8 *lists;

    for (i = 0, act = gPspIconActors, lists = gPspIconLists, icon = gPspIconIndex, pos = gPspIconPos, kind = gPspMarkKind; i < 6; i++, act++, lists += 4, icon++, pos += 2, kind++) {
        entry = lists;
        actor = &gPspWork->actors[*act];
        for (j = 0; j < 4; j++) {
            item = *entry++;
            if (item != -1 && PspLink_HasFlag(item) != 0) {
                break;
            }
        }
        if (j == 4) {
            continue;
        }
        gPspWork->iconFound[*icon] = 1;
        gPspWork->iconEntry[*icon] = j;
        actor->visible = 1;
        actor->state++;
        if (actor->large == 0) {
            actor->scaleX = 1.0f;
            actor->scaleY = 1.2f;
        } else {
            actor->scaleX = 1.2f;
            actor->scaleY = 1.4f;
        }
        actor->x = pos[0];
        actor->y = pos[1];
        actor[1].exists = 1;
        actor[1].visible = 1;
        actor[1].kind = *kind;
        actor[1].large = actor->large;
        actor[1].scaleX = actor->scaleX;
        actor[1].scaleY = actor->scaleY;
        actor[1].x = actor->x;
        actor[1].y = actor->y;
        actor[1].relative = actor->relative;
        actor[1].parent = actor->parent;
    }
    gPspWork->cursorIcon = -1;
    gPspWork->cameFrom2 = -1;
    gPspWork->cameFrom4 = -1;
    for (i = 0; i < PSP_ICON_COUNT; i++) {
        if (gPspWork->iconFound[gPspIconIndex[i]] != 0) {
            gPspWork->cursorIcon = i;
        }
    }
    for (i = 0, row = (s8 *)gPspCursorGrid; i < 3; i++, row += 4) {
        for (j = 0; j < 4; j++) {
            if (gPspWork->cursorIcon == row[j]) {
                gPspWork->cursorCol = j;
                gPspWork->cursorRow = i;
                return;
            }
        }
    }
}

/* Steps actors 0, 1, 20 and 22, stops actors 2-7 and every picture, then
 * marks the icons again. MWCC reloads gPspWork after every store through
 * it; the element pointers a and o, each taken from gPspWork once, give
 * the game's loads (the 2-7 loop is unrolled fully). */
void Psp_ResetSceneActors(void)
{
    s32 i;
    PspActor *actor;
    PspObj *obj;

    gPspWork->actors[0].state++;
    actor = &gPspWork->actors[1];
    actor->visible = 1;
    actor->state++;
    gPspWork->actors[20].state++;
    gPspWork->actors[22].state++;
    for (i = 2; i < 8; i++) {
        actor = &gPspWork->actors[i];
        actor->state = PSP_ACTOR_INIT;
        actor->visible = 0;
    }
    obj = gPspWork->objs;
    for (i = 0; i < PSP_ACTOR_COUNT; i++, obj++) {
        obj->active = 0;
    }
    Psp_MarkIcons();
}

/*
 * Starts object obj on animation first: active, its first frame and that
 * frame's time, the frame's step applied (mirrored when flipped).
 */
static void Psp_SetAnim(PspObj *obj, PspAnim *first)
{
    s32 dx;
    s32 dy;

    obj->active = 1;
    obj->wait = 0;
    obj->count = 0;
    obj->unk1C = 0;
    obj->unk20 = 0;
    obj->anim = first;
    obj->frame = 0;
    obj->wait = first->wait;
    dx = first->dx;
    if (obj->flip != 0) {
        dx = -dx;
    }
    dy = first->dy;
    obj->x += dx;
    obj->y += dy;
}

/*
 * Moves an object of the show: its walk by type, then at the end of a
 * frame's time the next frame (random turns for types 2 and 6), the
 * frame's step and sound (panned by x); the end of a type 0 ends its type 9
 * child. Forms the match needs: `stepX *= -1` keeps the game's run-time
 * negation (`stepX = -stepX` is folded), and `pan = K * pan` on the int
 * gives the pan's operand order.
 */
void Psp_UpdateObj(PspObj *obj)
{
    PspAnim *cur;
    s32 stepX;
    s32 stepY;
    s32 pan;
    s32 snd;
    PspObj *other;
    s32 i;

    switch (obj->type) {
    case 1:
        stepX = -4;
        if (obj->flip != 0) {
            stepX *= -1;
        }
        obj->x += stepX;
        break;
    case 2:
        if (obj->frame < 4) {
            stepX = -1;
            if (obj->flip != 0) {
                stepX *= -1;
            }
            obj->x += stepX;
        }
        break;
    case 3:
        stepX = -1;
        if (obj->flip != 0) {
            stepX *= -1;
        }
        obj->x += stepX;
        break;
    case 4:
        stepX = -1;
        if (obj->flip != 0) {
            stepX *= -1;
        }
        obj->x += stepX;
        break;
    case 5:
        stepX = -2;
        if (obj->flip != 0) {
            stepX *= -2;
        }
        obj->x += stepX;
        break;
    case 6:
        if (obj->frame < 6) {
            stepX = -1;
            if (obj->flip != 0) {
                stepX *= -1;
            }
            obj->x += stepX;
        }
        break;
    case 9:
        return;
    }
    if (obj->wait > 0) {
        obj->wait--;
        return;
    }
    cur = &obj->anim[obj->frame];
    switch (obj->type) {
    case 0:
        if (cur->loopPoint == 1) {
            obj->count++;
            if (obj->count >= 8) {
                obj->count = 0;
                obj->frame = 7;
                if (obj->flip == 0) {
                    Psp_SpawnChild(obj, 8, 20, -98);
                } else {
                    Psp_SpawnChild(obj, 8, -20, -98);
                }
            } else {
                obj->frame = cur->next;
            }
        } else {
            obj->frame = cur->next;
        }
        break;
    case 2:
        if (cur->loopPoint == 1) {
            if ((Rand_Next(1) % 10) < 2) {
                obj->frame = 4;
            } else {
                obj->frame = cur->next;
            }
        } else {
            obj->frame = cur->next;
        }
        break;
    case 6:
        if (cur->loopPoint == 1) {
            if ((Rand_Next(1) % 10) < 2) {
                obj->frame = 6;
            } else {
                obj->frame = cur->next;
            }
        } else {
            obj->frame = cur->next;
        }
        break;
    default:
        if (cur->next < 0) {
            obj->active = 0;
            return;
        }
        obj->frame = cur->next;
        break;
    }
    cur = &obj->anim[obj->frame];
    obj->wait = cur->wait;
    stepX = cur->dx;
    if (obj->flip != 0) {
        stepX *= -1;
    }
    stepY = cur->dy;
    obj->x += stepX;
    obj->y += stepY;
    if (obj->flip != 0) {
        if (obj->x > obj->endX) {
            obj->active = 0;
        }
    } else if (obj->x < obj->endX) {
        obj->active = 0;
    }
    snd = cur->se;
    if (snd != 0) {
        pan = obj->x - 320;
        pan = 0.15625f * pan;
        if (pan < 0) {
            if (pan < -50) {
                pan = -50;
            }
        } else if (pan > 50) {
            pan = 50;
        }
        if (snd == 10) {
            obj->count++;
            if ((obj->count & 3) == 0) {
                Sound_PlaySeFlatPan(6, 7, 0, pan);
            } else {
                Sound_PlaySeFlatPan(6, cur->se - 1, 0, pan);
            }
        } else {
            Sound_PlaySeFlatPan(6, snd - 1, 0, pan);
        }
    }
    if (obj->active == 0 && obj->type == 0) {
        other = gPspWork->objs;
        for (i = 0; i < PSP_OBJ_COUNT; i++, other++) {
            if (other->active != 0 && other->type == 9 && obj->flip == other->flip) {
                other->active = 0;
                return;
            }
        }
    }
}

/*
 * Draws object obj's current frame: its cell (u, v, texture and screen
 * sizes) as a sprite, mirrored when flipped.
 */
void Psp_DrawObject(PspObj *obj)
{
    PspAnim *cur;
    s16 *cellUV;
    Sprite spr;
    s16 texU;
    s16 texV;
    s16 texW;
    s16 texH;
    s16 scrW;
    s16 scrH;

    cur = &obj->anim[obj->frame];
    cellUV = cur->cells + cur->cell * 6;
    if (gPspWork->tex[cur->tex] != 0) {
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
        Tex_Prepare(1, cur->tex + PSP_TEX_FIRST);
        Tex_Bind(cur->tex + PSP_TEX_FIRST);
        Gs_SetState(RS_TEX_FILTER, 0);
        texU = *cellUV++;
        texV = *cellUV++;
        texW = *cellUV++;
        texH = *cellUV++;
        scrW = *cellUV++;
        scrH = *cellUV;
        if (obj->flip == 0) {
            spr.x = (s16)obj->x + cur->ox;
        } else {
            spr.x = (s16)obj->x - (scrW + cur->ox);
        }
        spr.y = (s16)obj->y + cur->oy;
        spr.w = scrW;
        spr.h = scrH;
        spr.color = -1;
        if (obj->flip == 0) {
            spr.u0 = texU;
            spr.u1 = spr.u0 + texW - 1;
        } else {
            spr.u1 = texU;
            spr.u0 = spr.u1 + texW - 1;
        }
        spr.v0 = texV;
        spr.v1 = spr.v0 + texH - 1;
        Sprite_Draw(&spr);
    }
}

/* Returns 1 while any object of the show is active, else 0. */
s32 Psp_AnyObjectActive(void)
{
    PspObj *obj;
    s32 i;

    for (i = 0, obj = gPspWork->objs; i < PSP_OBJ_COUNT; i++, obj++) {
        if (obj->active != 0) {
            return 1;
        }
    }
    return 0;
}


/*
 * Starts an object of the given type in the first free slot, at its start
 * and end from gPspObjParams (the side by flip). Type 0 brings a type 9
 * child with it.
 */
void Psp_SpawnObject(s32 type, s32 flip)
{
    PspObj *obj;
    s32 i;
    s16 *params;

    for (i = 0, obj = gPspWork->objs; i < PSP_OBJ_COUNT; i++, obj++) {
        if (obj->active == 0) {
            break;
        }
    }
    if (i < PSP_OBJ_COUNT) {
        obj->type = type;
        obj->flip = flip;
        params = gPspObjParams + type * 5;
        if (flip != 0) {
            obj->x = params[0];
            obj->endX = params[4];
        } else {
            obj->x = params[1];
            obj->endX = params[3];
        }
        obj->y = params[2];
        Psp_SetAnim(obj, gPspObjAnims[type]);
        if (type == 0) {
            if (obj->flip == 0) {
                Psp_SpawnChild(obj, 9, 470, obj->y);
            } else {
                Psp_SpawnChild(obj, 9, 170, obj->y);
            }
        }
    }
}

/*
 * Starts a child object of src: type 9 at x, y on the screen, the others
 * relative to src; it takes src's direction and end.
 */
void Psp_SpawnChild(PspObj *src, s32 type, s32 x, s32 y)
{
    PspObj *obj;
    s32 i;

    for (i = 0, obj = gPspWork->objs; i < PSP_OBJ_COUNT; i++, obj++) {
        if (obj->active == 0) {
            break;
        }
    }
    if (i < PSP_OBJ_COUNT) {
        obj->type = type;
        obj->flip = src->flip;
        if (type == 9) {
            obj->x = x;
            obj->y = y;
            obj->endX = src->endX;
        } else {
            obj->x = src->x + x;
            obj->y = src->y + y;
            obj->endX = src->endX;
        }
        Psp_SetAnim(obj, gPspObjAnims[type]);
    }
}

/*
 * Gives actor `kind` its rectangle from gPspViewRects. The icon screens
 * (even actors 8-18) and actor 22 copy it to the next actor, their frame;
 * the frames (odd 9-19) and actor 21 keep theirs.
 */
void Psp_SetViewRect(u32 kind, PspActor *actor)
{
    s16 *rect;

    rect = gPspViewRects;
    switch (kind) {
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        rect += 20;
        rect += (kind - 2) * 4;
        break;
    case 8:
    case 10:
    case 12:
    case 14:
    case 16:
    case 18:
        rect += 16;
        break;
    case 21:
        return;
    case 20:
        rect += 4;
        break;
    case 9:
    case 11:
    case 13:
    case 15:
    case 17:
    case 19:
        return;
    case 22:
        rect += 44;
        break;
    }
    actor->x = *rect++;
    actor->y = *rect++;
    actor->w = *rect++;
    actor->h = *rect;
    switch (kind) {
    case 8:
    case 10:
    case 12:
    case 14:
    case 16:
    case 18:
    case 22:
        actor[1].x = actor->x;
        actor[1].y = actor->y;
        actor[1].w = actor->w;
        actor[1].h = actor->h;
        break;
    }
}

/*
 * Moves screen i by its state (PspActorState): the two that slide in, the
 * six that fly across at a random speed and height, and the icon screens,
 * dimmed unless theirs is the icon chosen. A visible actor goes into the
 * frame's draw list.
 */
void Psp_UpdateActor(s32 i, PspActor *actor)
{
    s32 rnd;
    PspActor **views;
    s16 spd;
    s32 speedIdx;
    PspWork *work;
    s32 icon;

    views = gPspWork->drawList;
    switch (i) {
    case 0:
        switch (actor->state) {
        case PSP_ACTOR_INIT:
            Psp_SetViewRect(i, actor);
            actor->x = 0.0f;
            actor->y = 0.0f;
            actor->w = 640;
            actor->speed = 30.0f;
            actor->state++;
            break;
        case PSP_ACTOR_WAIT:
            break;
        case PSP_ACTOR_RUN:
            actor->x += actor->speed;
            actor->w -= (s32)(s16)actor->speed;
            if (actor->x >= 640.0f) {
                actor->x = 640.0f;
                actor->state++;
                actor->visible = 0;
            }
            break;
        case PSP_ACTOR_DONE:
            break;
        }
        break;
    case 1:
        switch (actor->state) {
        case PSP_ACTOR_INIT:
            Psp_SetViewRect(i, actor);
            actor->speed = 30.0f;
            actor->x = -640.0f;
            actor->y = 0.0f;
            actor->state++;
            break;
        case PSP_ACTOR_WAIT:
            break;
        case PSP_ACTOR_RUN:
            actor->x += actor->speed;
            if (actor->x >= 0.0f) {
                actor->x = 0.0f;
                actor->state++;
            }
            break;
        case PSP_ACTOR_DONE:
            break;
        }
        break;
    case 20:
        switch (actor->state) {
        case PSP_ACTOR_INIT:
            Psp_SetViewRect(i, actor);
            actor->state++;
            break;
        case PSP_ACTOR_WAIT:
            break;
        case PSP_ACTOR_RUN:
            actor->visible = 0;
            actor->state++;
            break;
        case PSP_ACTOR_DONE:
            break;
        }
        break;
    case 21:
        switch (actor->state) {
        case PSP_ACTOR_INIT:
            Psp_SetViewRect(i, actor);
            actor->state++;
            break;
        case PSP_ACTOR_WAIT:
            break;
        }
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        switch (actor->state) {
        case PSP_ACTOR_INIT:
            break;
        case PSP_ACTOR_WAIT:
            Psp_SetViewRect(i, actor);
            rnd = Rand_Next(1) & 1;
            speedIdx = Rand_Next(1) % 4;
            spd = gPspActorSpeeds[speedIdx];
            actor->kind = rnd;
            if (rnd == 0) {
                actor->speed = -1.0f * spd;
                actor->x = 700.0f;
                actor->flipX = 0;
            } else {
                actor->speed = spd;
                actor->x = -200.0f;
                actor->flipX = 1;
            }
            rnd = Rand_Next(1) & 0x7FFF;
            actor->y = 100.0f + 100.0f * Math_Sin(6.2831855f * rnd / 65536.0f);
            actor->state++;
            break;
        case PSP_ACTOR_RUN:
            actor->x += actor->speed;
            if (actor->kind == 0) {
                if (actor->x <= -200.0f) {
                    actor->state = PSP_ACTOR_WAIT;
                }
            } else if (actor->x >= 700.0f) {
                actor->state = PSP_ACTOR_WAIT;
            }
            break;
        }
        break;
    case 8:
    case 10:
    case 12:
    case 14:
    case 16:
    case 18:
    case 22:
        switch (actor->state) {
        case PSP_ACTOR_INIT:
            Psp_SetViewRect(i, actor);
            actor->state++;
            break;
        case PSP_ACTOR_WAIT:
            break;
        case PSP_ACTOR_RUN:
            actor->dim = 1;
            work = gPspWork;
            icon = work->cursorIcon;
            if ((i - 8) / 2 == icon && work->iconFound[gPspIconIndex[icon]] != 0) {
                actor->dim = 0;
            }
            break;
        }
        break;
    }
    if (actor->visible != 0) {
        views[i] = actor;
    }
}

/*
 * Clears the draw list; once the show reaches its icons, sets the icon
 * screen's actors up (Psp_ResetSceneActors); then moves the actors in use
 * and the active objects.
 */
void Psp_UpdateScene(void)
{
    s32 *state;
    PspActor *actor;
    s32 i;
    s32 count;
    PspObj *obj;

    memset(gPspWork->drawList, 0, sizeof(gPspWork->drawList));
    state = &gPspWork->sceneState;
    switch (*state) {
    case 0:
        if (gPspWork->step == 11 && gPspWork->subStep == 2) {
            (*state)++;
            Psp_ResetSceneActors();
        }
        break;
    case 1:
        break;
    }
    count = gPspWork->actorCount;
    for (i = 0, actor = gPspWork->actors; i < count; i++, actor++) {
        Psp_UpdateActor(i, actor);
    }
    for (i = 0, obj = gPspWork->objs; i < PSP_OBJ_COUNT; i++, obj++) {
        if (obj->active != 0) {
            Psp_UpdateObj(obj);
        }
    }
}

struct PrimLine;

/*
 * Draws an actor: its part as a filled rectangle (kind 0) or a sprite,
 * placed relative to its parent when it is relative, scaled, mirrored and
 * dimmed as the actor says.
 */
void Psp_DrawActor(PspActor *actor)
{
    PspRect rect;
    Sprite spr;
    PspPart *draw;
    s16 parentX;
    s16 parentY;
    s16 width;
    s16 height;
    u32 tint;

    if (actor->exists == 0) {
        return;
    }
    if (actor->visible == 0) {
        return;
    }
    if ((draw = actor->part) == NULL) {
        return;
    }
    parentX = parentY = 0;
    if (actor->relative != 0 && actor->parent != NULL) {
        parentX = actor->parent->x;
        parentY = actor->parent->y;
    }
    width = actor->w * actor->scaleX;
    height = actor->h * actor->scaleY;
    if (draw->kind == 0) {
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
        rect.x0 = parentX + (s16)actor->x;
        rect.y0 = parentY + (s16)actor->y;
        rect.x1 = rect.x0 + width;
        rect.y1 = rect.y0 + height;
        rect.color = draw->color;
        Prim_FillRect((struct PrimLine *)&rect);
        return;
    }
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    if (gPspWork->tex[draw->tex] == 0) {
        return;
    }
    Tex_Prepare(1, draw->tex + PSP_TEX_FIRST);
    Tex_Bind(draw->tex + PSP_TEX_FIRST);
    if (draw->blend == 0) {
        Gs_SetState(RS_TEX_FILTER, 0x10000);
    } else {
        Gs_SetState(RS_TEX_FILTER, 0);
    }
    spr.x = parentX + (s16)actor->x;
    spr.y = parentY + (s16)actor->y;
    spr.w = width;
    spr.h = height;
    if (actor->dim == 0) {
        spr.color = draw->color;
    } else {
        tint = draw->color;
        spr.color = ((tint & 0xFF) >> 1 & 0xFF) | (((tint & 0xFF00) >> 1 & 0xFF00) | ((tint & 0xFF000000) | ((tint & 0xFF0000) >> 1 & 0xFF0000)));
    }
    if (actor->flipX == 0) {
        spr.u0 = draw->u;
        spr.u1 = spr.u0 + draw->uw - 1;
    } else {
        spr.u1 = draw->u;
        spr.u0 = spr.u1 + draw->uw - 1;
    }
    if (actor->flipY == 0) {
        spr.v0 = draw->v;
        spr.v1 = spr.v0 + draw->vh - 1;
    } else {
        spr.v1 = draw->v;
        spr.v0 = spr.v1 + draw->vh - 1;
    }
    Sprite_Draw(&spr);
}

struct PrimTexTri;

/*
 * Draws an icon screen's actor as two textured triangles, turned by icon
 * k's swing (swingAmp, swingPhase); its centre goes to centerX/centerY
 * for Psp_DrawSwingFrame.
 * FAKEMATCH: corner0Y, corner3X and corner3Y point at corners[0][1],
 * corners[3][0] and corners[3][1] only so that those three addresses are
 * declared values. The address of an element that is stored and read back
 * is one value across the calls; the game keeps these three in saved
 * registers and rematerialises the others, as MWCC does when they have one
 * reference more or are numbered first (a duplicate store to each also
 * gives it, at 3 extra words). The natural form behind it is not known.
 */
void Psp_DrawSwingActor(PspActor *actor, s32 icon)
{
    PspTri tris[2];
    f32 corners[4][3];
    PspPart *draw;
    s16 parentX;
    s16 parentY;
    s16 width;
    s16 height;
    s16 centreY;
    s16 centreX;
    f32 halfW;
    u16 angle;
    s32 i;
    u32 tint;
    f32 *corner0Y;
    f32 *corner3X;
    f32 *corner3Y;
    s16 x1;
    s16 y1;
    s16 x2;
    s16 y2;

    if (actor->exists == 0) {
        return;
    }
    if (actor->visible == 0) {
        return;
    }
    if ((draw = actor->part) == NULL) {
        return;
    }
    parentX = parentY = 0;
    if (actor->relative != 0 && actor->parent != NULL) {
        parentX = actor->parent->x;
        parentY = actor->parent->y;
    }
    width = actor->w * actor->scaleX;
    height = actor->h * actor->scaleY;
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    if (gPspWork->tex[draw->tex] == 0) {
        return;
    }
    Tex_Prepare(1, draw->tex + PSP_TEX_FIRST);
    Tex_Bind(draw->tex + PSP_TEX_FIRST);
    if (draw->blend == 0) {
        Gs_SetState(RS_TEX_FILTER, 0x10000);
    } else {
        Gs_SetState(RS_TEX_FILTER, 0);
    }
    centreX = width / 2 + (parentX + (s16)actor->x);
    centreY = parentY + (s16)actor->y;
    gPspWork->centerX[icon] = centreX;
    gPspWork->centerY[icon] = centreY;
    halfW = width / 2.0f;
    corner0Y = &corners[0][1];
    corner3X = &corners[3][0];
    corner3Y = &corners[3][1];
    corners[0][0] = -halfW;
    *corner0Y = 0.0f;
    corners[0][2] = 0.0f;
    corners[1][0] = -halfW;
    corners[1][1] = height;
    corners[1][2] = 0.0f;
    corners[2][0] = halfW;
    corners[2][1] = 0.0f;
    corners[2][2] = 0.0f;
    *corner3X = halfW;
    *corner3Y = height;
    corners[3][2] = 0.0f;
    angle = gPspWork->swingAmp[icon] * Math_Sin(6.2831855f * gPspWork->swingPhase[icon] / 65536.0f);
    for (i = 0; i < 4; i++) {
        Vec_RotateZ(corners[i], 6.2831855f * angle / 65536.0f);
    }
    if (actor->dim == 0) {
        tint = draw->color;
    } else {
        tint = draw->color;
        tint = ((tint & 0xFF) >> 1 & 0xFF) | (((tint & 0xFF00) >> 1 & 0xFF00) | ((tint & 0xFF000000) | ((tint & 0xFF0000) >> 1 & 0xFF0000)));
    }
    tris[0].color = tint;
    tris[1].color = tint;
    tris[0].x0 = centreX + (s16)corners[0][0];
    tris[0].y0 = centreY + (s16)*corner0Y;
    tris[0].x1 = x1 = centreX + (s16)corners[1][0];
    tris[0].y1 = y1 = centreY + (s16)corners[1][1];
    tris[0].x2 = x2 = centreX + (s16)corners[2][0];
    tris[0].y2 = y2 = centreY + (s16)corners[2][1];
    tris[0].u0 = draw->u;
    tris[0].v0 = draw->v;
    tris[0].u1 = draw->u;
    tris[0].v1 = draw->v + draw->vh - 1;
    tris[0].u2 = draw->u + draw->uw - 1;
    tris[0].v2 = draw->v;
    tris[1].x0 = x1;
    tris[1].y0 = y1;
    tris[1].x1 = x2;
    tris[1].y1 = y2;
    tris[1].x2 = centreX + (s16)*corner3X;
    tris[1].y2 = centreY + (s16)*corner3Y;
    tris[1].u0 = draw->u;
    tris[1].v0 = draw->v + draw->vh - 1;
    tris[1].u1 = draw->u + draw->uw - 1;
    tris[1].v1 = draw->v;
    tris[1].u2 = draw->u + draw->uw - 1;
    tris[1].v2 = draw->v + draw->vh - 1;
    Prim_DrawTexTri((struct PrimTexTri *)&tris[0]);
    Prim_DrawTexTri((struct PrimTexTri *)&tris[1]);
}

/*
 * Draws the frame of icon screen k (texture 175, its rectangle
 * gPspFrameRects[kind]) as two triangles turned by the screen's swing
 * around the centre Psp_DrawSwingActor left in centerX/centerY; dimmed
 * unless kind is the chosen icon's.
 * FAKEMATCH: corner0Y, corner3X and corner3Y point at corners[0][1],
 * corners[3][0] and corners[3][1], as in Psp_DrawSwingActor (the game keeps
 * those three addresses in saved registers); the natural form is not
 * known. The empty range check of k between the two centre reads is a
 * compiled-out check (docs/mwcc-matching.md, "Statements scheduled as if a
 * check were still there"): it splits the tail into blocks, so MWCC reuses
 * the k * 2 it made before the rotation loop for both reads, as the game
 * does.
 */
void Psp_DrawSwingFrame(PspActor *actor, s32 icon)
{
    s32 i;
    PspTri tris[2];
    f32 corners[4][3];
    s16 *frameRect;
    u32 tint;
    s16 height;
    s16 centreX;
    s16 centreY;
    s16 texU;
    s16 texV;
    s16 uEnd;
    s16 vEnd;
    s16 x1;
    s16 y1;
    s16 x2;
    s16 y2;
    f32 halfW;
    u16 angle;
    s16 width;
    f32 *corner0Y;
    f32 *corner3X;
    f32 *corner3Y;

    if (actor->exists == 0) {
        return;
    }
    if (actor->visible == 0) {
        return;
    }
    frameRect = gPspFrameRects[actor->kind];
    width = frameRect[2] * actor->scaleX;
    height = frameRect[3] * actor->scaleX;
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    if (gPspWork->tex[12] == 0) {
        return;
    }
    Tex_Prepare(1, 175);
    Tex_Bind(175);
    Gs_SetState(RS_TEX_FILTER, 0);
    tint = gPspFrameColors[actor->kind];
    if (actor->kind != gPspMarkKind[gPspWork->cursorIcon]) {
        tint = ((tint & 0xFF) >> 1 & 0xFF) | (((tint & 0xFF00) >> 1 & 0xFF00) | ((tint & 0xFF000000) | ((tint & 0xFF0000) >> 1 & 0xFF0000)));
    }
    halfW = width / 2.0f;
    corner0Y = &corners[0][1];
    corner3X = &corners[3][0];
    corner3Y = &corners[3][1];
    corners[0][0] = -halfW;
    corners[0][1] = 30.0f;
    corners[0][2] = 0.0f;
    corners[1][0] = -halfW;
    corners[1][1] = 30.0f + height;
    corners[1][2] = 0.0f;
    corners[2][0] = halfW;
    corners[2][1] = 30.0f;
    corners[2][2] = 0.0f;
    corners[3][0] = halfW;
    corners[3][1] = 30.0f + height;
    corners[3][2] = 0.0f;
    angle = gPspWork->swingAmp[icon] * Math_Sin(6.2831855f * gPspWork->swingPhase[icon] / 65536.0f);
    for (i = 0; i < 4; i++) {
        Vec_RotateZ(corners[i], 6.2831855f * angle / 65536.0f);
    }
    tris[0].color = tint;
    tris[1].color = tint;
    texU = frameRect[0];
    uEnd = texU + frameRect[2] - 1;
    texV = frameRect[1];
    vEnd = texV + frameRect[3] - 1;
    centreY = gPspWork->centerY[icon];
    if (icon < 0 || icon >= 6) { /* compiled-out check */
    }
    centreX = gPspWork->centerX[icon];
    tris[0].x0 = centreX + (s16)corners[0][0];
    tris[0].y0 = centreY + (s16)*corner0Y;
    tris[0].x1 = x1 = centreX + (s16)corners[1][0];
    tris[0].y1 = y1 = centreY + (s16)corners[1][1];
    tris[0].x2 = x2 = centreX + (s16)corners[2][0];
    tris[0].y2 = y2 = centreY + (s16)corners[2][1];
    tris[0].u0 = texU;
    tris[0].v0 = texV;
    tris[0].u1 = texU;
    tris[0].v1 = vEnd;
    tris[0].u2 = uEnd;
    tris[0].v2 = texV;
    tris[1].x0 = x1;
    tris[1].y0 = y1;
    tris[1].x1 = x2;
    tris[1].y1 = y2;
    tris[1].x2 = centreX + (s16)*corner3X;
    tris[1].y2 = centreY + (s16)*corner3Y;
    tris[1].u0 = texU;
    tris[1].v0 = vEnd;
    tris[1].u1 = uEnd;
    tris[1].v1 = texV;
    tris[1].u2 = uEnd;
    tris[1].v2 = vEnd;
    Prim_DrawTexTri((struct PrimTexTri *)&tris[0]);
    Prim_DrawTexTri((struct PrimTexTri *)&tris[1]);
}

/*
 * Draws the background picture (the link's second file) over the whole
 * screen.
 */
void Psp_DrawBackground(void)
{
    Sprite spr;

    if (gPspWork->tex[1] != 0) {
        Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
        Tex_Prepare(1, 164);
        Tex_Bind(164);
        Gs_SetState(RS_TEX_FILTER, 0);
        spr.x = 0;
        spr.y = 0;
        spr.w = 640;
        spr.h = 448;
        spr.color = -1;
        spr.u0 = 0;
        spr.v0 = 0;
        spr.u1 = 255;
        spr.v1 = 255;
        Sprite_Draw(&spr);
    }
}

/*
 * Draws the frame: the listed actors (the odd 9-19 as swing frames, the
 * even 8-18 as swing actors, the rest plain), the active objects, the
 * header when there is one, then the background.
 */
void Psp_Draw(void)
{
    PspActor **entry;
    PspObj *obj;
    s32 i;

    Gs_SetState(RS_ZWRITE, 0);
    for (i = 0, entry = gPspWork->drawList; i < PSP_ACTOR_COUNT; i++, entry++) {
        if (*entry != NULL) {
            switch (i) {
            case 9:
            case 11:
            case 13:
            case 15:
            case 17:
            case 19:
                Psp_DrawSwingFrame(*entry, (i - 9) / 2);
                break;
            case 8:
            case 10:
            case 12:
            case 14:
            case 16:
            case 18:
                Psp_DrawSwingActor(*entry, (i - 8) / 2);
                break;
            default:
                Psp_DrawActor(*entry);
                break;
            }
        }
    }
    for (i = 0, obj = gPspWork->objs; i < PSP_OBJ_COUNT; i++, obj++) {
        if (obj->active != 0) {
            Psp_DrawObject(obj);
        }
    }
    if (gPspWork->headerKind != 0) {
        Sel_DrawHeader(gPspWork->headerKind);
    }
    Psp_DrawBackground();
    Gs_SetState(RS_ZWRITE, 1);
}

extern char **gPspStepTexts[12];
extern char **gPspLines[2];
extern char **gPspPromptTexts[2];
extern char **gPspValueNames[8];
char *strcpy(char *, char *);
char *strchr(char *, s32);                  /* strchr */
/* unit's own declaration: x and y as s16, as Psp_DrawTexts passes them; with the definition's s32, the calls compile differently */
void Hud_DrawButtonIcon(s32 id, s16 x, s16 y, s32 depth, u32 color, f32 scale);  /* a button icon */
s32 Str_Length(char *);                         /* a message's width */
struct Frame;

/*
 * The link's texts: the found item's message (gPspValueNames) and the lines
 * of the current step (gPspStepTexts[textStep]); `%a` and `%b` lines are
 * replaced by the centred gPspPromptTexts prompts, and `%r` is drawn as a
 * button. The three loops are copies that differ in where the position is
 * set (the centred cases share one marking loop through a goto).
 * PSP_TEXT_CANCELLED (13) reads past gPspStepTexts and gPspStepTextPos into
 * gPspLines and gPspLinePos, which follow them, as the game does.
 * Forms the match needs: `str = buf;` inside each loop, no guards around
 * the loops, the position tables as s16 pairs (gPspStepTextPos + 2 as its
 * own base), the width as `textX = Str_Length(..); textX = (640.0f - 10.0f
 * * textX) / 2.0f;` (reused as the x), lines loaded before x and y,
 * Hud_DrawButtonIcon's s16 x and y, and an early return when there are no
 * lines.
 * FAKEMATCH: the dead `lines = NULL;` after the help text's lookup gives
 * lines a web there, which puts lines in s4 and esc in s3 in the second and
 * third loops as in the game; without it the two swap.
 */
void Psp_DrawTexts(void)
{
    char buf[256];
    char **lines;
    char *str;
    char *esc;
    s32 x;
    s32 y;
    char **prompt;
    s32 textX;
    s8 icon;
    s8 kind;

    if (gPspWork->step == 11 && gPspWork->subStep == 2 && gPspWork->timer == 0) {
        if (gPspWork->hideFrames == 0) {
            WindowFrame_Draw(&gPspFrameB, 0);
        }
        icon = gPspWork->cursorIcon;
        if (icon != -1) {
            kind = gPspIconIndex[icon];
            str = (gPspValueNames[kind])[gPspWork->iconEntry[kind]];
        } else {
            str = (gPspStepTexts[11])[0];
        }
        lines = NULL;
        if (str != NULL) {
            Font_SetPos(50, 370);
            Font_SetSpacing(20, 20);
            Font_SetPal(0);
            Font_SetColor(0x80808080);
            for (;;) {
                esc = strchr(str, '%');
                if (!esc != 0) {
                    break;
                }
                if (esc[1] == 'r') {
                    Hud_DrawButtonIcon(25, (esc - str) * 10 + 50, 370, 341, -1, 1.0f);
                    esc[0] = ' ';
                    esc[1] = ' ';
                }
            }
            Text_PrintfColored(str);
        }
    }
    if (gPspWork->step < 11 && gPspWork->hideFrames == 0) {
        WindowFrame_Draw(&gPspFrameA, 0);
    }
    if (gPspWork->textStep == 0) {
        return;
    }
    Font_SetSpacing(20, 20);
    Font_SetPal(0);
    Font_SetColor(0x80808080);
    lines = gPspStepTexts[gPspWork->textStep];
    x = gPspStepTextPos[gPspWork->textStep][0];
    y = gPspStepTextPos[gPspWork->textStep][1];
    for (; *lines != NULL; lines++, y += 24) {
        str = buf;
        strcpy(buf, *lines);
        if (buf[0] == '%' && buf[1] == 'a') {
            if (gFrameTick & 0x10) {
                continue;
            }
            prompt = gPspPromptTexts[1];
            strcpy(buf, *prompt);
            textX = Str_Length(*prompt);
            textX = (640.0f - 10.0f * textX) / 2.0f;
            Font_SetPos(textX, y);
            goto mark1;
        } else if (buf[0] == '%' && buf[1] == 'b') {
            if (gFrameTick & 0x10) {
                continue;
            }
            prompt = gPspPromptTexts[0];
            strcpy(buf, *prompt);
            textX = Str_Length(*prompt);
            textX = (640.0f - 10.0f * textX) / 2.0f;
            Font_SetPos(textX, y);
            goto mark1;
        } else if (x == -1) {
            textX = Str_Length(*lines);
            textX = (640.0f - 10.0f * textX) / 2.0f;
            Font_SetPos(textX, y);
    mark1:
            for (;;) {
                esc = strchr(str, '%');
                if (!esc != 0) {
                    break;
                }
                if (esc[1] == 'r') {
                    Hud_DrawButtonIcon(25, textX + (esc - str) * 10, y, 341, -1, 1.0f);
                    esc[0] = ' ';
                    esc[1] = ' ';
                }
            }
        } else {
            Font_SetPos(x, y);
            for (;;) {
                esc = strchr(str, '%');
                if (!esc != 0) {
                    break;
                }
                if (esc[1] == 'r') {
                    Hud_DrawButtonIcon(25, x + (esc - str) * 10, y, 341, -1, 1.0f);
                    esc[0] = ' ';
                    esc[1] = ' ';
                }
            }
        }
        Text_Printf(str);
    }
    if (gPspWork->textStep == 2 && (gFrameTick & 0x10)) {
        lines = gPspStepTexts[3];
        x = gPspStepTextPos[3][0];
        y = gPspStepTextPos[3][1];
        for (; *lines != NULL; lines++, y += 24) {
            str = buf;
            strcpy(buf, *lines);
            if (buf[0] == '%' && buf[1] == 'a') {
                if (gFrameTick & 0x10) {
                    continue;
                }
                prompt = gPspPromptTexts[1];
                strcpy(buf, *prompt);
                textX = Str_Length(*prompt);
                textX = (640.0f - 10.0f * textX) / 2.0f;
                Font_SetPos(textX, y);
                goto mark2;
            } else if (buf[0] == '%' && buf[1] == 'b') {
                if (gFrameTick & 0x10) {
                    continue;
                }
                prompt = gPspPromptTexts[0];
                strcpy(buf, *prompt);
                textX = Str_Length(*prompt);
                textX = (640.0f - 10.0f * textX) / 2.0f;
                Font_SetPos(textX, y);
                goto mark2;
            } else if (x == -1) {
                textX = Str_Length(*lines);
                textX = (640.0f - 10.0f * textX) / 2.0f;
        mark2:
                for (;;) {
                    esc = strchr(str, '%');
                    if (!esc != 0) {
                        break;
                    }
                    if (esc[1] == 'r') {
                        Hud_DrawButtonIcon(25, textX + (esc - str) * 10, y, 341, -1, 1.0f);
                        esc[0] = ' ';
                        esc[1] = ' ';
                    }
                }
                Font_SetPos(textX, y);
            } else {
                for (;;) {
                    esc = strchr(str, '%');
                    if (!esc != 0) {
                        break;
                    }
                    if (esc[1] == 'r') {
                        Hud_DrawButtonIcon(25, x + (esc - str) * 10, y, 341, -1, 1.0f);
                        esc[0] = ' ';
                        esc[1] = ' ';
                    }
                }
                Font_SetPos(x, y);
            }
            Text_Printf(str);
        }
    }
    if (gPspWork->textStep == 6 || gPspWork->textStep == 2) {
        lines = gPspLines[0];
        x = gPspLinePos[0][0];
        y = gPspLinePos[0][1];
        for (; *lines != NULL; lines++, y += 24) {
            str = buf;
            strcpy(buf, *lines);
            if (buf[0] == '%' && buf[1] == 'a') {
                if (gFrameTick & 0x10) {
                    continue;
                }
                prompt = gPspPromptTexts[1];
                strcpy(buf, *prompt);
                textX = Str_Length(*prompt);
                textX = (640.0f - 10.0f * textX) / 2.0f;
                Font_SetPos(textX, y);
                goto mark3;
            } else if (buf[0] == '%' && buf[1] == 'b') {
                if (gFrameTick & 0x10) {
                    continue;
                }
                prompt = gPspPromptTexts[0];
                strcpy(buf, *prompt);
                textX = Str_Length(*prompt);
                textX = (640.0f - 10.0f * textX) / 2.0f;
                Font_SetPos(textX, y);
                goto mark3;
            } else if (x == -1) {
                textX = Str_Length(*lines);
                textX = (640.0f - 10.0f * textX) / 2.0f;
                Font_SetPos(textX, y);
        mark3:
                for (;;) {
                    esc = strchr(str, '%');
                    if (!esc != 0) {
                        break;
                    }
                    if (esc[1] == 'r') {
                        Hud_DrawButtonIcon(25, textX + (esc - str) * 10, y, 341, -1, 1.0f);
                        esc[0] = ' ';
                        esc[1] = ' ';
                    }
                }
            } else {
                for (;;) {
                    esc = strchr(str, '%');
                    if (!esc != 0) {
                        break;
                    }
                    if (esc[1] == 'r') {
                        Hud_DrawButtonIcon(25, x + (esc - str) * 10, y, 341, -1, 1.0f);
                        esc[0] = ' ';
                        esc[1] = ' ';
                    }
                }
                Font_SetPos(x, y);
            }
            Text_Printf(str);
        }
    }
    Text_DrawIcons(Font_GetPage());
    Font_DrawAllPages();
}


/*
 * The link's thread (PspThreadState): idle, or one exchange (the device's
 * path, open, send sendBuf, read recvBuf, close), done or failed;
 * between polls it waits DelayThread(100).
 */
void Psp_Thread(void *arg)
{
    for (;;) {
        switch (gPspState) {
        case PSP_THREAD_IDLE:
            gPspState = PSP_THREAD_IDLE;
            gPspWork->fd = -1;
            break;
        case PSP_THREAD_EXCHANGE:
            *(s32 *)gPspDevctlArg = 0;
            if (sceDevctl(gPspDevName, PSP_DEVCTL_GET_PATH, gPspDevctlArg, 8, gPspDevPath, 16) < 0) {
                gPspState = PSP_THREAD_ERROR;
                break;
            }
            gPspWork->fd = sceOpen(gPspDevPath, 3, 0);
            if (gPspWork->fd < 0) {
                gPspState = PSP_THREAD_ERROR;
                break;
            }
            if (sceWrite(gPspWork->fd, gPspWork->sendBuf, PSP_BUF_SIZE) < 0) {
                if (gPspWork->fd >= 0) {
                    sceClose(gPspWork->fd);
                }
                gPspState = PSP_THREAD_ERROR;
                break;
            }
            if (sceRead(gPspWork->fd, gPspWork->recvBuf, PSP_BUF_SIZE) < 0) {
                if (gPspWork->fd >= 0) {
                    sceClose(gPspWork->fd);
                }
                gPspState = PSP_THREAD_ERROR;
                break;
            }
            sceClose(gPspWork->fd);
            gPspState = PSP_THREAD_DONE;
            break;
        case PSP_THREAD_CLOSE:
            if (gPspWork->fd >= 0) {
                sceClose(gPspWork->fd);
            }
            gPspState = PSP_THREAD_IDLE;
            break;
        case PSP_THREAD_DONE:
        case PSP_THREAD_STOP:
        case PSP_THREAD_ERROR:
            break;
        }
        DelayThread(100);
    }
}

/* Whether an item of the exchange is unlocked (event flags; the sets are
 * unlocked when one of their items is). */
s32 Psp_IsItemUnlocked(u16 id)
{
    switch (id) {
    case 10:
        if (Save_TestEventFlag(18) == 1) {
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(17) == 1) {
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(12) == 1) {
            return 1;
        }
        break;
    case 7:
        if (Save_TestEventFlag(11) == 1) {
            return 1;
        }
        break;
    case 6:
        if (Save_TestEventFlag(9) == 1) {
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(8) == 1) {
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(7) == 1) {
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(6) == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(5) == 1) {
            return 1;
        }
        break;
    case 1:
        if (Save_TestEventFlag(0) == 1) {
            return 1;
        }
        break;
    case 51:
        if (Psp_IsItemUnlocked(10) != 0) {
            return 1;
        }
        break;
    case 50:
        if (Psp_IsItemUnlocked(7) != 0) {
            return 1;
        }
        break;
    case 49:
        if (Psp_IsItemUnlocked(3) != 0) {
            return 1;
        }
        break;
    case 48:
        if (Psp_IsItemUnlocked(2) != 0) {
            return 1;
        }
        break;
    case 47:
        if (Psp_IsItemUnlocked(1) != 0) {
            return 1;
        }
        break;
    case 33:
        if (Psp_IsItemUnlocked(6) != 0) {
            return 1;
        }
        break;
    case 32:
        if (Psp_IsItemUnlocked(3) != 0) {
            return 1;
        }
        break;
    case 31:
        if (Psp_IsItemUnlocked(1) != 0) {
            return 1;
        }
        break;
    case 67:
        if (Psp_IsItemUnlocked(10) != 0) {
            return 1;
        }
        break;
    case 66:
        if (Psp_IsItemUnlocked(7) != 0) {
            return 1;
        }
        break;
    case 65:
        if (Psp_IsItemUnlocked(3) != 0) {
            return 1;
        }
        break;
    case 64:
        if (Psp_IsItemUnlocked(2) != 0) {
            return 1;
        }
        break;
    case 63:
        if (Psp_IsItemUnlocked(1) != 0) {
            return 1;
        }
        break;
    case 0:
        return 1;
    }
    return 0;
}

/*
 * The overlay's messages (in .data, their strings in .rodata):
 * the texts of every screen, as tables of string pointers that all the units
 * use. They come after this unit's own data and strings, so they were either
 * a unit of their own with no code, linked last, or defined at the end of
 * this one; they are kept here because the build needs code in every C file.
 */
/* .data */
char *gBootTexts[2] = {
#include "select/006CD7F8.inc"
};
char *gContentWarningText[4] = {
#include "select/006CD800.inc"
};
char *gCharaPartNames[8] = {
#include "select/006CD810.inc"
};
char *gCharaListHelpTexts[4] = {
#include "select/006CD830.inc"
};
char *gCharaPartHelpTexts[8] = {
#include "select/006CD840.inc"
};
char *gCharaPartTitles[8] = {
#include "select/006CD860.inc"
};
char *gRotateTexts[2] = {
#include "select/006CD880.inc"
};
char *gNameInvalidTexts[2] = {
#include "select/006CD888.inc"
};
char *gCharaConfirmTexts[2] = {
#include "select/006CD890.inc"
};
char *gCharaQuestionTexts[2] = {
#include "select/006CD898.inc"
};
char *gCharaYesNoTexts[2] = {
#include "select/006CD8A0.inc"
};
char *gCharaYesNoTexts2[2] = {
#include "select/006CD8A8.inc"
};
char *gCharaEditHelpTexts[2] = {
#include "select/006CD8B0.inc"
};
char *gCharaSelectTexts[2] = {
#include "select/006CD8B8.inc"
};
char *gCardSavingTexts[4] = {
#include "select/006CD8C0.inc"
};
char *gCharaPageTexts[2] = {
#include "select/006CD8D0.inc"
};
char *gOkTypeTexts[2] = {
#include "select/006CD8D8.inc"
};
char *gGenderBackTexts[2] = {
#include "select/006CD8E0.inc"
};
char *gAgeFeatureTexts[2] = {
#include "select/006CD8E8.inc"
};
char *gHairRowTexts[2] = {
#include "select/006CD8F0.inc"
};
char *gFaceHairRowTexts[2] = {
#include "select/006CD8F8.inc"
};
char *gNoMatchTexts[4] = {
#include "select/006CD900.inc"
};
char *gPartsLabels[40] = {
#include "select/006CD910.inc"
};
char *gTitleMenuA[8] = {
#include "select/006CD9B0.inc"
};
char *gTitleMenuB[12] = {
#include "select/006CD9D0.inc"
};
char *gModeSelTexts[4] = {
#include "select/006CDA00.inc"
};
char *gTitleHelpMenu[4] = {
#include "select/006CDA10.inc"
};
char *gTitleHelpTexts[2] = {
#include "select/006CDA20.inc"
};
char *gPspStepTexts_1[3] = {
#include "select/006CDA28.inc"
};
char *gPspStepTexts_2[8] = {
#include "select/006CDA40.inc"
};
char *gPspStepTexts_3[8] = {
#include "select/006CDA60.inc"
};
char *gPspStepTexts_4[4] = {
#include "select/006CDA80.inc"
};
char *gPspStepTexts_5[4] = {
#include "select/006CDA90.inc"
};
char *gPspStepTexts_6[8] = {
#include "select/006CDAA0.inc"
};
char *gPspStepTexts_7[4] = {
#include "select/006CDAC0.inc"
};
char *gPspStepTexts_8[4] = {
#include "select/006CDAD0.inc"
};
char *gPspStepTexts_9[2] = {
#include "select/006CDAE0.inc"
};
char *gPspStepTexts_10[2] = {
#include "select/006CDAE8.inc"
};
char *gPspStepTexts_11[2] = {
#include "select/006CDAF0.inc"
};
char *gPspLines_0[2] = {
#include "select/006CDAF8.inc"
};
char *gPspLines_1[4] = {
#include "select/006CDB00.inc"
};
char *gPspPromptTexts_0[2] = {
#include "select/006CDB10.inc"
};
char *gPspPromptTexts_1[2] = {
#include "select/006CDB18.inc"
};
char **gPspStepTexts[12] = {
    0, gPspStepTexts_1, gPspStepTexts_2, gPspStepTexts_3,
    gPspStepTexts_4, gPspStepTexts_5, gPspStepTexts_6, gPspStepTexts_7,
    gPspStepTexts_8, gPspStepTexts_9, gPspStepTexts_10, gPspStepTexts_11,
};
char **gPspLines[2] = {
    gPspLines_0, gPspLines_1,
};
char **gPspPromptTexts[2] = {
    gPspPromptTexts_0, gPspPromptTexts_1,
};
char *gPspValueNames_0[4] = {
#include "select/006CDB60.inc"
};
char *gPspValueNames_5[4] = {
#include "select/006CDB70.inc"
};
char *gPspValueNames_2[4] = {
#include "select/006CDB80.inc"
};
char *gPspValueNames_3[4] = {
#include "select/006CDB90.inc"
};
char *gPspValueNames_1[4] = {
#include "select/006CDBA0.inc"
};
char *gPspValueNames_4[4] = {
#include "select/006CDBB0.inc"
};
char **gPspValueNames[8] = {
    gPspValueNames_0, gPspValueNames_1, gPspValueNames_2, gPspValueNames_3,
    gPspValueNames_4, gPspValueNames_5, 0, 0,
};
char *gOptMainLabels[12] = {
#include "select/006CDBE0.inc"
};
char *gOptMainHelp[12] = {
#include "select/006CDC10.inc"
};
char *gOptControlLabels[8] = {
#include "select/006CDC40.inc"
};
char *gOptControlHelp[8] = {
#include "select/006CDC60.inc"
};
char *gSoundModeTexts[4] = {
#include "select/006CDC80.inc"
};
char *gVolumeBarTexts[8] = {
#include "select/006CDC90.inc"
};
char *gOffOnTexts[2] = {
#include "select/006CDCB0.inc"
};
char *gReverseTexts[3] = {
#include "select/006CDCB8.inc"
};
char *gSizeTexts[3] = {
#include "select/006CDCC8.inc"
};
char *gTypeNumTexts[2] = {
#include "select/006CDCD8.inc"
};
char *gNetOptLabels[8] = {
#include "select/006CDCE0.inc"
};
char *gNetOptHelp[8] = {
#include "select/006CDD00.inc"
};
char *gNetOptValueNames_0[4] = {
#include "select/006CDD20.inc"
};
char *gNetOptValueNames_1[4] = {
#include "select/006CDD30.inc"
};
char *gNetOptConfirmTexts[8] = {
#include "select/006CDD40.inc"
};
char *gScreenPosTexts[2] = {
#include "select/006CDD60.inc"
};
char *gBackResetTexts[2] = {
#include "select/006CDD68.inc"
};
char *gOptBrightnessTexts[4] = {
#include "select/006CDD70.inc"
};
