/*
 * select: the boot of the game.
 *
 * Select_MainTask runs first: it loads the common textures and sound banks,
 * waits for a controller (with a warning while none is ready), opens the
 * network menu once, then shows the content warning and four splash
 * pictures, and hands over to Boot_Task. It also applies the boot
 * arguments (language and video mode), which skip the attract screens.
 *
 * Boot_Task runs the attract loop over BootWork (select.h), at the start
 * of the shared work area: the opening movie, the title screen with its
 * start prompt, a still picture, then the movie again. A button
 * press on the title screen or the picture starts gm_sub's GmSub_Task,
 * which goes on to the menus.
 *
 * The file owns its .rodata and .bss.
 */
#include "common.h"
#include "card.h"
#include "padio.h"
#include "pad.h"
#include "select.h"
#include "syssettings.h"
#include "movie.h"
#include "select/select_boot.h"   /* the texts this file uses (tools/content_gen.py) */
#include "render_state.h"
#include "mine_point.h"
#include "sif.h"
#include "stageobj.h"
#include "camera.h"
#include "hud.h"
#include "sound.h"
#include "text.h"
#include "texture.h"
#include "file_load.h"
#include "font.h"
#include "frame.h"
#include "game_util.h"
#include "gm_main.h"
#include "rand.h"
#include "sys_main.h"
#include "screen_fade.h"
#include "quest.h"

extern PadInput *gPadInputPtr; /* main: -> gPadIn */

extern char *gBootTexts[];  /* the start prompt and the content warning's title (select_psp.c's .data) */
extern char *gContentWarningText[];
extern char *gPadWarningTexts[];

/* unit's own declaration: declared s32 as Select_MainTask uses it; with the definition's u8 return, the call compiles differently */
s32 NetMenu_Update(void);
extern u8 gNextQuest[];
/* Static, as Boot_UpdateScreens's code needs: it keeps v0 free across the
 * calls only when they go to local void functions. */
static void Boot_ToPicture(void);
static void Boot_ToMovie(void);
static void Boot_DrawPicture(void);

/* .bss */
BootWork *gBootWork;


/* Points gBootWork at the start of the shared work area and clears it. */
void Boot_InitWork(void)
{
    gBootWork = (BootWork *)gSysSettings.workArea;
    memset(gBootWork, 0, sizeof(BootWork));
}

/* Picks the first screen of the attract loop by where the boot was entered
 * from: the movie at power on, the title screen when the game was left
 * (gm_main), else the picture. */
void Boot_SetMode(BootWork *work)
{
    switch (gSysSettings.bootFrom) {
    case 0:
        work->mode = BOOT_MOVIE;
        break;
    case 1:
        work->mode = BOOT_TITLE;
        break;
    default:
        work->mode = BOOT_PICTURE;
        break;
    }
}

/*
 * The boot's task (slot 3). Its first frame resets the game's state, loads
 * the system textures and picks the first screen, unless the gallery is
 * playing a scene or gSysSettings.bootMode asks to go straight on (1: once,
 * then 0; 2: every time), when it starts GmSub_Task in slot 2 and ends.
 * Then it runs Boot_Update.
 */
void Boot_Task(TaskSlot *task)
{
    Select_BeginTaskFrame();
    switch (task->mode) {
    case 0:
        task->mode++;
        Game_ResetSubsystems();
        Quest_Clear();
        Gfx_ClearModelBuffers();
        Boot_InitWork();
        gSysSettings.petDayPending = 0;
        FadeB_Reset();
        Tex_LoadSystemPack(0);
        LobbyDay_SetNoon();
        if (gSysSettings.galleryPlay == 1) {
            gSysSettings.bootMode = 0;
            TaskSlot_Set(GmSub_Task, 2);
            TaskSlot_End(task);
            return;
        }
        switch (gSysSettings.bootMode) {
        case 0:
            Boot_SetMode(gBootWork);
            break;
        case 1:
            gSysSettings.bootMode = 0;
            TaskSlot_Set(GmSub_Task, 2);
            TaskSlot_End(task);
            return;
        case 2:
            TaskSlot_Set(GmSub_Task, 2);
            TaskSlot_End(task);
            return;
        }
        gSysSettings.bootMode = 0;
        break;
    case 1:
        Boot_Update(task);
        break;
    }
}

/* Draws the start prompt, blinking: 32 frames on, 32 off. */
void Boot_DrawStartPrompt(void)
{
    if (++gBootWork->blinkTimer & 0x20) {
        Font_SetSpacing(24, 24);
        Text_DrawShadow2(104, 348, 1, 0, gBootTexts[0]);
    }
}

/* Boot_Task's frame: fades in, runs the current screen, and once a screen
 * was skipped (BOOT_LEAVE) starts GmSub_Task and ends the task. */
void Boot_Update(TaskSlot *task)
{
    BootWork *work = gBootWork;

    switch (work->state) {
    case BOOT_START:
        work->state++;
        work->step = 0;
        FadeA_Start(2);
    case BOOT_RUN:
        switch (work->mode) {
        case BOOT_MOVIE:
            Boot_UpdateMovie();
            break;
        case BOOT_TITLE:
        case BOOT_PICTURE:
            Boot_UpdateScreens();
            break;
        }
        break;
    case BOOT_LEAVE:
        TaskSlot_Set(GmSub_Task, 2);
        TaskSlot_End(task);
        break;
    }
}

/* Goes on to the title screen (when the movie ends). */
void Boot_ToTitle(void)
{
    gBootWork->mode = BOOT_TITLE;
    gBootWork->step = 0;
}

/* Goes on to the still picture, fading in. */
static void Boot_ToPicture(void)
{
    FadeA_Start(2);
    gBootWork->mode = BOOT_PICTURE;
    gBootWork->step = 0;
}

static void Boot_ToMovie(void)
{
    gBootWork->mode = BOOT_MOVIE;
    gBootWork->step = 0;
}

/* Draws the still picture full screen (system texture 2). */
static void Boot_DrawPicture(void)
{
    Sprite spr;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 2);
    Tex_Bind(2);
    spr.x = 0;
    spr.y = 0;
    spr.w = 640;
    spr.h = 448;
    spr.color = -1;
    spr.u0 = 0;
    spr.v0 = 0;
    spr.u1 = 512;
    spr.v1 = 448;
    Sprite_Draw(&spr);
}

/* Draws the copyright mark (a 17x17 piece of system texture 8) at x, y. */
void Boot_DrawCopyrightMark(s16 x, s16 y, s32 color)
{
    Sprite spr;

    Tex_Prepare(1, 8);
    Tex_Bind(8);
    spr.v0 = 238;
    spr.w = 17;
    spr.u1 = 18;
    spr.h = 17;
    spr.v1 = 255;
    spr.x = x;
    spr.y = y;
    spr.color = color;
    spr.u0 = 0;
    Sprite_Draw(&spr);
}

/* Draws the title screen: the picture and the logo (alpha: their opacity,
 * 0-255), the copyright lines (one layout when gSysSettings.unk1A is 0, 2
 * or 4, another otherwise) and the start prompt. */
void Boot_DrawTitleScreen(s32 alpha)
{
    Sprite spr;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 3);
    Tex_Bind(3);
    spr.x = 0;
    spr.y = 0;
    spr.w = 640;
    spr.h = 448;
    spr.color = ((u8)alpha << 24) | 0xFFFFFF;
    spr.u0 = 0;
    spr.v0 = 0;
    spr.u1 = 511;
    spr.v1 = 447;
    Sprite_Draw(&spr);
    Tex_Prepare(1, 341);
    Tex_Bind(341);
    spr.x = 423;
    spr.y = 199;
    spr.h = 20;
    spr.w = 1.1f * spr.h;
    spr.color = ((u8)alpha << 24) | 0xFFFFFF;
    spr.u0 = 96;
    spr.v0 = 137;
    spr.u1 = spr.u0 + 31;
    spr.v1 = spr.v0 + 31;
    Sprite_Draw(&spr);
    Font_SetSpacing(18, 18);
    Font_SetHalfWidth(1);
    switch (gSysSettings.unk1A) {
    case 0:
    case 4:
    case 2:
        Boot_DrawCopyrightMark(125, 378, 0xFF000000);
        Boot_DrawCopyrightMark(123, 376, -1);
        Text_DrawShadow2(145, 376, 1, 0, TXT(0x006CDEA0) /* the copyright line (Japan) */);
        break;
    default:
        Boot_DrawCopyrightMark(108, 376, -1);
        Boot_DrawCopyrightMark(108, 396, -1);
        Text_PrintfAt(128, 376, 0, TXT(0x006CDED0) /* the copyright line (first of two) */);
        Text_PrintfAt(128, 396, 0, TXT(0x006CDEF0) /* the copyright line (second of two) */);
        break;
    }
    Boot_DrawStartPrompt();
    Text_DrawAllAndStop();
}

/*
 * The opening movie: plays movie 0 until it ends or a button is pressed,
 * then stops it (waiting at most 150 frames for the stop) and goes on to
 * the title screen. The gotos join the end-of-movie paths as the game's
 * code does.
 */
void Boot_UpdateMovie(void)
{
    u8 *step;
    u16 pad;
    s32 *waitFrames;

    step = &gBootWork->step;
    switch (*step) {
    case 0:
        Gs_SetTableColor(0);
        FadeA_Start(2);
        gBootWork->step++;
        gBootWork->unk2 = 0;
        break;
    case 1:
        *step = *step + 1;
        Movie_PrePlay();
        gBootWork->movieFrames = 0;
        Movie_Play(0);
        Movie_SetPosition(0, 0);
        gBootWork->waitFrames = 7672;
        break;
    case 2:
        Movie_Update();
        if (Movie_GetStatus() == MOVIE_STATUS_FINISHED) {
            goto stop;
        }
        gBootWork->movieFrames++;
        pad = gPadState[0].pressed;
        if ((pad & 0x8000) || (pad & 0x20)) {
        stop:
            FadeA_Start(4);
            gBootWork->step = 3;
            gBootWork->waitFrames = 150;
            Movie_RequestStop();
        }
        break;
    case 3:
        FadeA_Start(4);
        Movie_Update();
        if (Movie_GetStatus() == MOVIE_STATUS_STOPPED) {
        end_movie:
            Gs_SetTableColor(0);
            Movie_Stop();
            Boot_ToTitle();
            return;
        }
        waitFrames = &gBootWork->waitFrames;
        if (*waitFrames == 0) {
            goto end_movie;
        }
        (*waitFrames)--;
        break;
    }
    Callbacks_Add(Frame_Draw);
}

/*
 * The title screen and the picture: each shows for a while (332 frames for
 * the title, 150 for the picture), then fades out to the next screen of the
 * loop. A button press fades out instead and marks the boot as skipped:
 * then GmSub_Task starts and Boot_Update leaves (BOOT_LEAVE).
 */
void Boot_UpdateScreens(void)
{
    u16 pad;
    s32 *skipped;

    switch (gBootWork->step) {
    case 0:
        if (gBootWork->mode != BOOT_TITLE) {
            FadeA_Start(2);
            gBootWork->waitFrames = 150;
        } else {
            gBootWork->waitFrames = 332;
        }
        gBootWork->step++;
        gBootWork->blinkTimer = 32;
        gBootWork->skipped = 0;
    case 1:
        pad = gPadState[0].pressed;
        if ((pad & 0x8000) || (pad & 0x20)) {
            if (gBootWork->mode == BOOT_TITLE) {
                Sound_PlayConfirm();
                FadeA_Start(5);
                gBootWork->waitFrames = 60;
            } else {
                FadeA_Start(11);
                gBootWork->waitFrames = 10;
            }
            gBootWork->skipped = 1;
        } else {
            if (--gBootWork->waitFrames > 0) {
                break;
            }
            FadeA_Start(1);
            gBootWork->waitFrames = 16;
        }
        gBootWork->step++;
        break;
    case 2:
        if (--gBootWork->waitFrames > 0) {
            break;
        }
        skipped = &gBootWork->skipped;
        if (*skipped != 0) {
            *skipped = 0;
            gBootWork->state++;
            TaskSlot_Set(GmSub_Task, 2);
            return;
        }
        if (gBootWork->mode == BOOT_TITLE) {
            Boot_ToPicture();
        } else {
            Boot_ToMovie();
        }
        return;
    }
    if (gBootWork->mode == BOOT_TITLE) {
        Boot_DrawTitleScreen(255);
        Text_SetDefaults();
    } else if (gBootWork->mode == BOOT_PICTURE) {
        Boot_DrawPicture();
    }
}

/* Empty in this build. */
void Select_BeginTaskFrame(void)
{

}

/*
 * The overlay's first task (slot 0, t->mode its stage):
 *  0  loads the common textures and sound banks and resets the game's state;
 *  1  waits for a controller, warning while none is ready;
 *  2  opens the network menu the first time, waits for it, seeds the random
 *     number generator and reads the boot arguments (which skip to 8);
 *  3  the content warning, 4-7 the splash pictures (90 frames each, a press
 *     shortens them);
 *  8  starts Boot_Task in slot 3 and ends.
 */
void Select_MainTask(TaskSlot *task)
{
    s32 padResult;

    Select_BeginTaskFrame();
    switch (task->mode) {
    case 0:
        Tex_LoadCommonPack();
        Select_LoadSoundBanks();
        Rand_InitGauss();
        UnreadFlags_Init();
        Model_ResetAll();
        Boot_ResetMenus();
        Card_Init();
        memset(gNextQuest, 0, 128);
        Quest_Clear();
        Hud_Reset();
        TaskSlot_Set(ScreenFade_Task, 9);
        task->mode++;
        task->step = 0;
        task->flag = 0;
        break;
    case 1:
        switch (task->step) {
        case 0:
            padResult = Select_CheckPad();
            switch (padResult) {
            case -1:
                break;
            case -2:
                task->step++;
                task->flag = 0;
                break;
            default:
                task->mode++;
                task->step = 0;
                break;
            }
            break;
        case 1:
            if (task->flag == 0) {
                if (Select_IsPadReady() != 0) {
                    task->flag = 1;
                }
            } else {
                if (gPadState[0].held & 0x20) {
                    task->mode++;
                    task->step = 0;
                    task->flag = 0;
                    break;
                }
                if (Select_IsPadReady() == 0) {
                    task->flag = 0;
                }
            }
            Select_DrawPadWarning();
            break;
        }
        Callbacks_Add(Frame_Draw);
        break;
    case 2:
        switch (task->step) {
        case 0:
            task->step++;
            if (gSysSettings.netMenuShown == 0) {
                gSysSettings.netMenuShown = 1;
                NetMenu_Open(NETMENU_MODE_BOOT);
            } else {
                task->step++;
                FadeA_Start(11);
            }
            break;
        case 1:
            if ((u8)NetMenu_Update()) {
                task->step++;
                FadeA_Start(11);
                Rand_Seed(Card_GetSeed());
            }
            break;
        case 2:
            if ((u8)FadeA_GetState() != 1) {
                SysSettings_Reset();
                gSysSettings.bootFrom = 0;
                if (Select_ApplyBootArgs() != 0) {
                    goto start_boot; /* into case 8, as the game's code jumps */
                }
                task->mode++;
                task->step = 0;
                return;
            }
            break;
        }
        Callbacks_Add(Frame_Draw);
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        switch (task->step) {
        case 0:
            task->step++;
            FadeA_Start(2);
            task->timeLeft = 90;
            break;
        case 1:
            if ((u8)FadeA_GetState() != 1) {
                task->step++;
            }
            break;
        case 2:
            if (--task->timeLeft == 0) {
                task->step++;
                FadeA_Start(1);
            } else if (task->mode != 4 || task->timeLeft < 30) {
                if (gPadState[0].pressed & 0x8000) {
                    task->timeLeft = 1;
                }
            }
            break;
        case 3:
            if ((u8)FadeA_GetState() != 1) {
                task->mode++;
                task->step = 0;
                switch (task->mode) {
                case 3:
                    break;
                case 4:
                    task->splash = 0;
                    FadeA_Start(2);
                    break;
                case 5:
                    task->splash = 2;
                    FadeA_Start(2);
                    break;
                case 6:
                    task->splash = 7;
                    FadeA_Start(2);
                    break;
                case 7:
                    task->splash = 3;
                    FadeA_Start(2);
                    break;
                default:
                    FadeA_Reset();
                    return;
                }
            }
            break;
        }
        if (task->mode == 3) {
            Callbacks_Add(Select_DrawContentWarning);
        } else {
            Callbacks_Add(Select_DrawSplash);
        }
        break;
    case 8:
    start_boot:
        Tex_ReleaseCommon();
        TaskSlot_Set(Boot_Task, 3);
        TaskSlot_End(task);
        break;
    }
}

struct SndFile;
s32 Sound_LoadBankWait(struct SndFile *f, u32 slot); /* main: f is a SndFile * */

/* Reads the overlay's four sound banks through the load buffer into the
 * sound slots 0, 1, 6 and 7. */
void Select_LoadSoundBanks(void)
{
    u8 *loadBuf;

    loadBuf = gSysSettings.loadBuf;
    FlushCache(0);
    Sound_ReadBank(0x10037, loadBuf);
    Sound_LoadBankWait((struct SndFile *) loadBuf, 0);
    FlushCache(0);
    Sound_ReadBank(0x10002, loadBuf);
    Sound_LoadBankWait((struct SndFile *) loadBuf, 1);
    FlushCache(0);
    Sound_ReadBank(0x10007, loadBuf);
    Sound_LoadBankWait((struct SndFile *) loadBuf, 6);
    FlushCache(0);
    Sound_ReadBank(0x10008, loadBuf);
    Sound_LoadBankWait((struct SndFile *) loadBuf, 7);
}

/* Checks the controller: -1 while a port is not read yet, -2 when no
 * controller (or one of another kind) is connected, else the connection
 * state (Pad_GetConnected). Turns vibration on. */
s32 Select_CheckPad(void)
{
    s32 i;
    PadInput *port;

    for (i = 0, port = gPadInputPtr; i < 1; i++, port++) {
        if (port->state == 2) {
            return -1;
        }
    }
    Pad_EnableVibration(1);
    if (gPadInputPtr->kind == 0 || Select_IsPadReady() == 0) {
        return -2;
    }
    return (u8)Pad_GetConnected();
}

/* 1 when the controller is of the kind the game takes (PadInput.kind 1). */
s32 Select_IsPadReady(void)
{
    return gPadInputPtr->kind == 1;
}

/* Draws the no-controller warning, one line every 24 pixels. */
void Select_DrawPadWarning(void)
{
    char **line;
    s32 y;

    y = 180;
    Font_SetSpacing(18, 18);
    Text_SetColor(0);
    line = gPadWarningTexts;
    if (gPadWarningTexts[0] != 0) {
        do {
            Font_SetPos(58, y);
            Text_PrintfColored(*line);
            line++;
            y += 24;
        } while (*line != 0);
    }
}

/* Draws the content warning screen: a dimmed background and two texts. */
void Select_DrawContentWarning(void)
{
    Sprite spr;

    Tex_Prepare(1, 6);
    Tex_Bind(6);
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    spr.w = 640;
    spr.u1 = 255;
    spr.h = 448;
    spr.v1 = 255;
    spr.x = 0;
    spr.color = 0xFF606060;
    spr.y = 0;
    spr.u0 = 0;
    spr.v0 = 0;
    Sprite_Draw(&spr);
    Font_SetSpacing(26, 26);
    Text_DrawShadow2(99, 200, 1, 0, gBootTexts[1]);
    Text_DrawShadow2(99, 240, 1, 0, gContentWarningText[0]);
    Text_DrawAllAndStop();
}

/* Draws the splash picture TaskSlot 0's splash names: system texture
 * splash + 2, full screen (splash 6 drawn 32 pixels higher). */
void Select_DrawSplash(void)
{
    Sprite spr;
    s32 splash;

    splash = TaskSlot_Get(0)->splash;
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, splash + 2);
    Tex_Bind(splash + 2);
    if (splash == 7 || splash == 0) {
        spr.x = 0;
        spr.y = 0;
        spr.w = 640;
        spr.h = 448;
        spr.color = -1;
        spr.u0 = 0;
        spr.v0 = 0;
        spr.u1 = 511;
        spr.v1 = 447;
    } else {
        spr.x = 0;
        spr.w = 640;
        if (splash == 6) {
            spr.y = -32;
        } else {
            spr.y = 0;
        }
        spr.h = 512;
        spr.v0 = 0;
        spr.color = -1;
        spr.u0 = 0;
        spr.u1 = 511;
        spr.v1 = 511;
    }
    Sprite_Draw(&spr);
}

extern s8 gArgv[3][64];      /* main: the boot arguments (argv[1] a "0"/"1" flag, argv[2] the language letter) */

/* The language of the boot argument gArgv[2]'s letter: j, e, f, g, i, s
 * give 0-5; -1 for anything else. */
static s32 Select_ParseLanguage(void)
{
    s8 letter;

    letter = gArgv[2][0];
    switch (letter) {
    case 'j':
        return 0;
    case 'e':
        return 1;
    case 'f':
        return 2;
    case 'g':
        return 3;
    case 'i':
        return 4;
    case 's':
        return 5;
    default:
        return -1;
    }
}

/* The video mode of the boot argument gArgv[1]: '0' or '1' give 0 or 1
 * (gSysSettings.pal); -1 for anything else. */
static s32 Select_ParseVideoMode(void)
{
    s8 letter;

    letter = gArgv[1][0];
    switch (letter) {
    case '0':
        return 0;
    case '1':
        return 1;
    default:
        return -1;
    }
}

/* Empty in this build. */
static void Select_EndBootArgs(void)
{

}

/* Applies the boot arguments: the language to gSysSettings.language, the
 * video mode to gSysSettings.pal. When either was given, sets
 * gSysSettings.bootMode to 1 (Boot_Task skips the attract screens once)
 * and returns 1; else 0. */
s32 Select_ApplyBootArgs(void)
{
    s32 changed = 0;
    s32 value;

    value = Select_ParseLanguage();
    if (value >= 0) {
        gSysSettings.language = value;
        changed = 1;
    }
    value = Select_ParseVideoMode();
    if (value >= 0) {
        gSysSettings.pal = value;
        changed = 1;
    }
    Select_EndBootArgs();
    if (changed) {
        gSysSettings.bootMode = 1;
        return 1;
    }
    return 0;
}
