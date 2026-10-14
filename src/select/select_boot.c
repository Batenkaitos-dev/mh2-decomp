/*
 * select: the overlay's main task (Select_MainTask, which seeds the random
 * number generator), the copyright and warning screens (0x6B51B0-0x6B6700).
 * The file owns its .rodata (0x6CDEA0-0x6CDF50) and .bss (0x6D2180).
 */
#include "common.h"
#include "padio.h"
#include "pad.h"
#include "m2c_macros.h"
#include "select.h"
#include "syssettings.h"
#include "select/select_boot.h"   /* the texts this file uses (tools/content_gen.py) */

extern PadInput *gPadInputPtr; /* main: -> gPadIn */
void SysSettings_Reset(void);
s32 Select_IsPadReady(void);

/* The boot sequence's work, at the start of the shared work area
 * (gSysSettings.workArea). */
typedef struct BootWork {
    u8 state;               /* 0x00 */
    u8 step;                /* 0x01 */
    u8 unk2;
    u8 unk3;
    s32 unk4;               /* 0x04 */
    s32 timer;              /* 0x08: blinks the press-start prompt */
    u8 unkC[0x14];
    s32 wait;               /* 0x20 */
    u8 unk24[0x10];
    s32 skip;               /* 0x34 */
    s32 mode;               /* 0x38: 0 the opening movie, 1 the title screen, 2 the picture */
} BootWork;

void D_8FAFD0(TaskSlot *t); /* gm_sub GmSub_Task */
extern char *gBootTexts[];  /* messages (select_psp.c) */
extern char *gContentWarningText[];
extern s32 D_446FB0[];

void Game_ResetSubsystems(void);
void Quest_Clear(void);
void Gfx_ClearModelBuffers(void);
void SeqB_Reset(void);
void Tex_LoadSystemPack(s32 mode);
void LobbyDay_SetNoon(void);
void Text_PrintfColored(const char *fmt, ...);
void Text_SetColor(s16 color);
void Text_DrawAllAndStop(void);
u8 Pad_GetConnected(void);
void Gs_SetTableColor(s32 i);
void Movie_PrePlay(void);
s32 Movie_Play(s32 id);
void Movie_SetPosition(s32 x, s32 y);
void Movie_RequestStop(void);
void Movie_Stop(void);
s32 Movie_GetStatus(void);
s32 Movie_Update(void);
void Callbacks_Add(void (*fn)(void));
void Frame_Draw(void);
void Boot_ToTitle(void);
void Tex_LoadCommonPack(void);
void Rand_InitGauss(void);
void Rand_Seed(u8 seed);
s32 func_368E80(void);
void Model_ResetAll(void);
void Cam_ResetRequests_Unk(void);
void Card_Init(void);
void Hud_Reset(void);
void NetMenu_Open(u8 mode);
/* unit's own declaration: with the definition's u8 (void) (sub_main/unk_3D54B0.c), Select_MainTask: 13 of 322 words differ */
s32 NetMenu_Update(void);
u8 func_3DDBA0(void);
u8 SeqA_GetState(void);
void SeqA_Reset(void);
void Tex_ReleaseCommon(void);
extern u8 gQuestExtra[];
void Seq_UpdateAll(u8 *task);        /* sub_main: started as a task */
void Select_LoadSoundBanks(void);
s32 Select_CheckPad(void);
void Select_DrawPadWarning(void);
void Select_DrawContentWarning(void);
void Select_DrawSplash(void);
s32 Select_ApplyBootArgs(void);
void Sound_PlaySe73(void);
void Text_SetDefaults(void);
/* Static, called only by Boot_UpdateScreens: a void function keeps v0 free up to
 * its end, and through calls to static void functions (Boot_UpdateScreens does
 * not use v0 on the way to these calls, as in the game). */
static void Boot_ToPicture(void);
static void Boot_ToMovie(void);
static void Boot_DrawPicture(void);
void Boot_DrawTitleScreen(s32);
void Pad_EnableVibration(u8 on);
void Boot_Update(TaskSlot *t);
void Boot_UpdateMovie(void);
void Boot_UpdateScreens(void);
void func_006B5CE0(void);

/* .bss, declared from the highest address down (MWCC emits uninitialized
 * variables last-declared first). */
BootWork *gBootWork;


void Boot_InitWork(void) {
    gBootWork = (BootWork *)gSysSettings.workArea;
    memset(gBootWork, 0, sizeof(BootWork));
}

void Boot_SetMode(BootWork *w) {
    switch (gSysSettings.bootFrom) {
    case 0:
        w->mode = 0;
        break;
    case 1:
        w->mode = 1;
        break;
    default:
        w->mode = 2;
        break;
    }
}

void Boot_Task(TaskSlot *t) {
    func_006B5CE0();
    switch (t->mode) {
    case 0:
        t->mode++;
        Game_ResetSubsystems();
        Quest_Clear();
        Gfx_ClearModelBuffers();
        Boot_InitWork();
        gSysSettings.unkE5 = 0;
        SeqB_Reset();
        Tex_LoadSystemPack(0);
        LobbyDay_SetNoon();
        if (gSysSettings.galleryPlay == 1) {
            gSysSettings.bootMode = 0;
            TaskSlot_Set(D_8FAFD0, 2);
            TaskSlot_End(t);
            return;
        }
        switch (gSysSettings.bootMode) {
        case 0:
            Boot_SetMode(gBootWork);
            break;
        case 1:
            gSysSettings.bootMode = 0;
            TaskSlot_Set(D_8FAFD0, 2);
            TaskSlot_End(t);
            return;
        case 2:
            TaskSlot_Set(D_8FAFD0, 2);
            TaskSlot_End(t);
            return;
        }
        gSysSettings.bootMode = 0;
        break;
    case 1:
        Boot_Update(t);
        break;
    }
}

void Boot_DrawPressStart(void) {
    if (++gBootWork->timer & 0x20) {
        Font_SetSpacing(0x18, 0x18);
        Text_DrawShadow2(0x68, 0x15C, 1, 0, gBootTexts[0]);
    }
}

void Boot_Update(TaskSlot *t) {
    BootWork *w = gBootWork;

    switch (w->state) {
    case 0:
        w->state++;
        w->step = 0;
        SeqA_Start(2);
    case 1:
        switch (w->mode) {
        case 0:
            Boot_UpdateMovie();
            break;
        case 1:
        case 2:
            Boot_UpdateScreens();
            break;
        }
        break;
    case 2:
        TaskSlot_Set(D_8FAFD0, 2);
        TaskSlot_End(t);
        break;
    }
}

void Boot_ToTitle(void) {
    gBootWork->mode = 1;
    gBootWork->step = 0;
}

static void Boot_ToPicture(void) {
    SeqA_Start(2);
    gBootWork->mode = 2;
    gBootWork->step = 0;
}

static void Boot_ToMovie(void) {
    gBootWork->mode = 0;
    gBootWork->step = 0;
}

static void Boot_DrawPicture(void) {
    Sprite spr;

    Gfx_SetState63(1);
    Tex_Prepare(1, 2);
    Tex_Bind(2);
    spr.x = 0;
    spr.y = 0;
    spr.w = 0x280;
    spr.h = 0x1C0;
    spr.color = -1;
    spr.u0 = 0;
    spr.v0 = 0;
    spr.u1 = 0x200;
    spr.v1 = 0x1C0;
    Sprite_Draw(&spr);
}

void Boot_DrawCopyrightMark(s16 x, s16 y, s32 color) {
    Sprite spr;

    Tex_Prepare(1, 8);
    Tex_Bind(8);
    spr.v0 = 0xEE;
    spr.w = 0x11;
    spr.u1 = 0x12;
    spr.h = 0x11;
    spr.v1 = 0xFF;
    spr.x = x;
    spr.y = y;
    spr.color = color;
    spr.u0 = 0;
    Sprite_Draw(&spr);
}

void Font_SetHalfWidth(s32);
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);

/* The title screen: the picture (alpha: its fade), the logo, the copyright
 * lines (two layouts, by the value at 0x6ADFAA) and PRESS START. */
void Boot_DrawTitleScreen(s32 alpha) {
    Sprite spr;

    Gfx_SetState63(1);
    Tex_Prepare(1, 3);
    Tex_Bind(3);
    spr.x = 0;
    spr.y = 0;
    spr.w = 0x280;
    spr.h = 0x1C0;
    spr.color = ((u8)alpha << 24) | 0xFFFFFF;
    spr.u0 = 0;
    spr.v0 = 0;
    spr.u1 = 0x1FF;
    spr.v1 = 0x1BF;
    Sprite_Draw(&spr);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    spr.x = 0x1A7;
    spr.y = 0xC7;
    spr.h = 0x14;
    spr.w = 1.1f * spr.h;
    spr.color = ((u8)alpha << 24) | 0xFFFFFF;
    spr.u0 = 0x60;
    spr.v0 = 0x89;
    spr.u1 = spr.u0 + 0x1F;
    spr.v1 = spr.v0 + 0x1F;
    Sprite_Draw(&spr);
    Font_SetSpacing(0x12, 0x12);
    Font_SetHalfWidth(1);
    switch (gSysSettings.unk1A) {
    case 0:
    case 4:
    case 2:
        Boot_DrawCopyrightMark(0x7D, 0x17A, 0xFF000000);
        Boot_DrawCopyrightMark(0x7B, 0x178, -1);
        Text_DrawShadow2(0x91, 0x178, 1, 0, TXT(0x006CDEA0) /* the copyright line (Japan) */);
        break;
    default:
        Boot_DrawCopyrightMark(0x6C, 0x178, -1);
        Boot_DrawCopyrightMark(0x6C, 0x18C, -1);
        Text_PrintfAt(0x80, 0x178, 0, TXT(0x006CDED0) /* the copyright line (first of two) */);
        Text_PrintfAt(0x80, 0x18C, 0, TXT(0x006CDEF0) /* the copyright line (second of two) */);
        break;
    }
    Boot_DrawPressStart();
    Text_DrawAllAndStop();
}

void Boot_UpdateMovie(void) {
    u8 *step;
    u16 pad;
    s32 *wait;

    step = &gBootWork->step;
    switch (*step) {
    case 0:
        Gs_SetTableColor(0);
        SeqA_Start(2);
        gBootWork->step++;
        gBootWork->unk2 = 0;
        break;
    case 1:
        *step = *step + 1;
        Movie_PrePlay();
        gBootWork->unk4 = 0;
        Movie_Play(0);
        Movie_SetPosition(0, 0);
        gBootWork->wait = 0x1DF8;
        break;
    case 2:
        Movie_Update();
        if (Movie_GetStatus() == 3) {
            goto stop;
        }
        gBootWork->unk4++;
        pad = gPadState[0].pressed;
        if ((pad & 0x8000) || (pad & 0x20)) {
        stop:
            SeqA_Start(4);
            gBootWork->step = 3;
            gBootWork->wait = 0x96;
            Movie_RequestStop();
        }
        break;
    case 3:
        SeqA_Start(4);
        Movie_Update();
        if (Movie_GetStatus() == 0) {
        end_movie:
            Gs_SetTableColor(0);
            Movie_Stop();
            Boot_ToTitle();
            return;
        }
        wait = &gBootWork->wait;
        if (*wait == 0) {
            goto end_movie;
        }
        (*wait)--;
        break;
    }
    Callbacks_Add(Frame_Draw);
}

void Boot_UpdateScreens(void) {
    u16 pad;
    s32 *skip;

    switch (gBootWork->step) {
    case 0:
        if (gBootWork->mode != 1) {
            SeqA_Start(2);
            gBootWork->wait = 0x96;
        } else {
            gBootWork->wait = 0x14C;
        }
        gBootWork->step++;
        gBootWork->timer = 0x20;
        gBootWork->skip = 0;
    case 1:
        pad = gPadState[0].pressed;
        if ((pad & 0x8000) || (pad & 0x20)) {
            if (gBootWork->mode == 1) {
                Sound_PlaySe73();
                SeqA_Start(5);
                gBootWork->wait = 0x3C;
            } else {
                SeqA_Start(0xB);
                gBootWork->wait = 0xA;
            }
            gBootWork->skip = 1;
        } else {
            if (--gBootWork->wait > 0) {
                break;
            }
            SeqA_Start(1);
            gBootWork->wait = 0x10;
        }
        gBootWork->step++;
        break;
    case 2:
        if (--gBootWork->wait > 0) {
            break;
        }
        skip = &gBootWork->skip;
        if (*skip != 0) {
            *skip = 0;
            gBootWork->state++;
            TaskSlot_Set(D_8FAFD0, 2);
            return;
        }
        if (gBootWork->mode == 1) {
            Boot_ToPicture();
        } else {
            Boot_ToMovie();
        }
        return;
    }
    if (gBootWork->mode == 1) {
        Boot_DrawTitleScreen(0xFF);
        Text_SetDefaults();
    } else if (gBootWork->mode == 2) {
        Boot_DrawPicture();
    }
}

void func_006B5CE0(void) {

}

void Select_MainTask(TaskSlot *t) {
    s32 r;

    func_006B5CE0();
    switch (t->mode) {
    case 0:
        Tex_LoadCommonPack();
        Select_LoadSoundBanks();
        Rand_InitGauss();
        func_368E80();
        Model_ResetAll();
        Cam_ResetRequests_Unk();
        Card_Init();
        memset(gQuestExtra, 0, 0x80);
        Quest_Clear();
        Hud_Reset();
        TaskSlot_Set(Seq_UpdateAll, 9);
        t->mode++;
        t->step = 0;
        t->flag = 0;
        break;
    case 1:
        switch (t->step) {
        case 0:
            r = Select_CheckPad();
            switch (r) {
            case -1:
                break;
            case -2:
                t->step++;
                t->flag = 0;
                break;
            default:
                t->mode++;
                t->step = 0;
                break;
            }
            break;
        case 1:
            if (t->flag == 0) {
                if (Select_IsPadReady() != 0) {
                    t->flag = 1;
                }
            } else {
                if (gPadState[0].held & 0x20) {
                    t->mode++;
                    t->step = 0;
                    t->flag = 0;
                    break;
                }
                if (Select_IsPadReady() == 0) {
                    t->flag = 0;
                }
            }
            Select_DrawPadWarning();
            break;
        }
        Callbacks_Add(Frame_Draw);
        break;
    case 2:
        switch (t->step) {
        case 0:
            t->step++;
            if (gSysSettings.netMenuShown == 0) {
                gSysSettings.netMenuShown = 1;
                NetMenu_Open(0);
            } else {
                t->step++;
                SeqA_Start(0xB);
            }
            break;
        case 1:
            if ((u8)NetMenu_Update()) {
                t->step++;
                SeqA_Start(0xB);
                Rand_Seed(func_3DDBA0());
            }
            break;
        case 2:
            if ((u8)SeqA_GetState() != 1) {
                SysSettings_Reset();
                gSysSettings.bootFrom = 0;
                if (Select_ApplyBootArgs() != 0) {
                    goto start_boot;
                }
                t->mode++;
                t->step = 0;
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
        switch (t->step) {
        case 0:
            t->step++;
            SeqA_Start(2);
            t->timeLeft = 0x5A;
            break;
        case 1:
            if ((u8)SeqA_GetState() != 1) {
                t->step++;
            }
            break;
        case 2:
            if (--t->timeLeft == 0) {
                t->step++;
                SeqA_Start(1);
            } else if (t->mode != 4 || t->timeLeft < 0x1E) {
                if (gPadState[0].pressed & 0x8000) {
                    t->timeLeft = 1;
                }
            }
            break;
        case 3:
            if ((u8)SeqA_GetState() != 1) {
                t->mode++;
                t->step = 0;
                switch (t->mode) {
                case 3:
                    break;
                case 4:
                    t->splash = 0;
                    SeqA_Start(2);
                    break;
                case 5:
                    t->splash = 2;
                    SeqA_Start(2);
                    break;
                case 6:
                    t->splash = 7;
                    SeqA_Start(2);
                    break;
                case 7:
                    t->splash = 3;
                    SeqA_Start(2);
                    break;
                default:
                    SeqA_Reset();
                    return;
                }
            }
            break;
        }
        if (t->mode == 3) {
            Callbacks_Add(Select_DrawContentWarning);
        } else {
            Callbacks_Add(Select_DrawSplash);
        }
        break;
    case 8:
    start_boot:
        Tex_ReleaseCommon();
        TaskSlot_Set(Boot_Task, 3);
        TaskSlot_End(t);
        break;
    }
}

void FlushCache(s32);
struct SndFile;
s32 Sound_LoadBankWait(struct SndFile *f, u32 slot); /* main: f is a SndFile * */
s32 Sound_ReadBank(u32 id, void *buf);
void Select_LoadSoundBanks(void) {
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

s32 Select_CheckPad(void) {
    s32 i;
    u8 *p;

    for (i = 0, p = (u8 *)gPadInputPtr; i < 1; i++, p += 0x88) {
        if (*p == 2) {
            return -1;
        }
    }
    Pad_EnableVibration(1);
    if (gPadInputPtr->kind == 0 || Select_IsPadReady() == 0) {
        return -2;
    }
    return (u8)Pad_GetConnected();
}

s32 Select_IsPadReady(void) {
    return gPadInputPtr->kind == 1;
}

void Select_DrawPadWarning(void) {
    s32 *p;
    s32 y;

    y = 0xB4;
    Font_SetSpacing(0x12, 0x12);
    Text_SetColor(0);
    p = D_446FB0;
    if (D_446FB0[0] != 0) {
        do {
            Font_SetPos(0x3A, y);
            Text_PrintfColored((const char *)*p);
            p++;
            y += 0x18;
        } while (*p != 0);
    }
}

void Select_DrawContentWarning(void) {
    Sprite spr;

    Tex_Prepare(1, 6);
    Tex_Bind(6);
    Gfx_SetState63(0);
    spr.w = 0x280;
    spr.u1 = 0xFF;
    spr.h = 0x1C0;
    spr.v1 = 0xFF;
    spr.x = 0;
    spr.color = 0xFF606060;
    spr.y = 0;
    spr.u0 = 0;
    spr.v0 = 0;
    Sprite_Draw(&spr);
    Font_SetSpacing(0x1A, 0x1A);
    Text_DrawShadow2(0x63, 0xC8, 1, 0, gBootTexts[1]);
    Text_DrawShadow2(0x63, 0xF0, 1, 0, gContentWarningText[0]);
    Text_DrawAllAndStop();
}

void Select_DrawSplash(void) {
    Sprite spr;
    s32 n;

    n = TaskSlot_Get(0)->splash;
    Gfx_SetState63(1);
    Tex_Prepare(1, n + 2);
    Tex_Bind(n + 2);
    if (n == 7 || n == 0) {
        spr.x = 0;
        spr.y = 0;
        spr.w = 0x280;
        spr.h = 0x1C0;
        spr.color = -1;
        spr.u0 = 0;
        spr.v0 = 0;
        spr.u1 = 0x1FF;
        spr.v1 = 0x1BF;
    } else {
        spr.x = 0;
        spr.w = 0x280;
        if (n == 6) {
            spr.y = -0x20;
        } else {
            spr.y = 0;
        }
        spr.h = 0x200;
        spr.v0 = 0;
        spr.color = -1;
        spr.u0 = 0;
        spr.u1 = 0x1FF;
        spr.v1 = 0x1FF;
    }
    Sprite_Draw(&spr);
}

extern s8 gArgv[3][0x40];      /* main: the boot arguments (argv[1] a "0"/"1" flag, argv[2] the language letter) */

/* Maps the language letter gArgv[2][0] (0x24C110) (j, e, f, g, i, s) to 0-5, else -1. */
static s32 Select_ParseLanguage(void) {
    s8 c;

    c = gArgv[2][0];
    switch (c) {                              /* irregular */
    case 0x6A:
        return 0;
    case 0x65:
        return 1;
    case 0x66:
        return 2;
    case 0x67:
        return 3;
    case 0x69:
        return 4;
    case 0x73:
        return 5;
    default:
        return -1;
    }
}

/* Maps the boot flag gArgv[1][0] (0x24C0D0) ("0"/"1") to 0/1, else -1. */
static s32 Select_ParseVideoMode(void) {
    s8 c;

    c = gArgv[1][0];
    switch (c) {                              /* irregular */
    case 48:
        return 0;
    case 49:
        return 1;
    default:
        return -1;
    }
}

static void func_006B6680(void) {

}

/* Applies the boot bytes (Select_ParseLanguage, Select_ParseVideoMode) to 0x6ADFE0/0x6ADFE1; 1 (and 0x6AE04C = 1) when one was set. */
s32 Select_ApplyBootArgs(void) {
    s32 changed = 0;
    s32 v;

    v = Select_ParseLanguage();
    if (v >= 0) {
        gSysSettings.language = v;
        changed = 1;
    }
    v = Select_ParseVideoMode();
    if (v >= 0) {
        gSysSettings.pal = v;
        changed = 1;
    }
    func_006B6680();
    if (changed) {
        gSysSettings.bootMode = 1;
        return 1;
    }
    return 0;
}
