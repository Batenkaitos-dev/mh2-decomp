/*
 * select: OPTIONS (game settings, sound, screen position, brightness) and
 * NETWORK OPTIONS (0x6B6700-0x6B9B10). The file owns its .data
 * (0x6CAB60-0x6CACC0), .rodata (0x6CDF50-0x6CDFD8) and .bss
 * (0x6D2190-0x6D21A8).
 */
#include "common.h"
#include "window.h"
#include "syssettings.h"
#include "pad.h"
#include "m2c_macros.h"
#include "select.h"
#include "syssave.h"

void SysSettings_Reset(void);
u8 SeqA_GetState(void);
void Option_Update(TaskSlot *t);
void Option_Start(TaskSlot *t, u16 net);
void Option_Exit(void *t, s32 net);
void Gfx_SetBlend(s32, s32);
void Frame_Draw(void);
/* OPTIONS' work (gOptWork). */
typedef struct OptWork {
    s32 state;              /* 0x00 */
    s32 step;               /* 0x04 */
    s32 unk8;               /* 0x08 */
    u8 unkC[4];
    s32 unk10;              /* 0x10 */
    u8 unk14[0xC];
    s32 page;               /* 0x20: the page shown */
    s32 nextPage;           /* 0x24: the page after the fade */
    s32 unk28;              /* 0x28 */
    s32 unk2C;              /* 0x2C */
    u8 unk30[8];
    s32 unk38;              /* 0x38 */
    s32 unk3C;              /* 0x3C */
    s32 unk40;              /* 0x40 */
    s32 unk44;              /* 0x44 */
    s32 unk48;              /* 0x48 */
    f32 unk4C;              /* 0x4C */
    s32 unk50;              /* 0x50 */
    s32 unk54;              /* 0x54: a colour */
    u32 unk58;              /* 0x58: a colour */
    u32 unk5C;              /* 0x5C: a colour */
    u32 unk60;              /* 0x60: a colour */
    s32 unk64;              /* 0x64 */
    s32 unk68;              /* 0x68 */
    s32 unk6C;              /* 0x6C */
    s32 unk70;              /* 0x70 */
    s32 unk74;              /* 0x74 */
    s32 unk78;              /* 0x78 */
    s32 unk7C;              /* 0x7C */
    s8 unk80[0x10];         /* 0x80: a copy of gSysSave.options */
    s8 unk90[0x10];         /* 0x90: a copy of gSysSave.netOptions */
    s32 unkA0;              /* 0xA0: the arrows are shown */
    s16 unkA4;              /* 0xA4: 1 left, 2 right (the arrow pressed) */
    s16 unkA6;              /* 0xA6: left arrow x */
    s16 unkA8;              /* 0xA8: right arrow x */
    s16 unkAA;              /* 0xAA: left arrow offset */
    s16 unkAC;              /* 0xAC: right arrow offset */
    s16 unkAE;              /* 0xAE: arrows y */
} OptWork;

void Stream_UpdateMaxVol(s32 apply);
void Sound_PlaySe73(void);
void Sound_PlayCursorMove(void);
void Sound_PlayCancel(void);
struct TexRect;
void TexRect_DrawRotated(struct TexRect *r, s32 angle);

void Title_DrawMenu(void *unused);
void Sel_DrawHeader(s16 kind);
extern M2C_UNK D_5B3680;
void NetOption_Update(TaskSlot *t, s32 unused);

extern u32 gNetOptValueNames_0[];
extern u32 gNetOptValueNames_1[];

/* .bss, declared from the highest address down (MWCC emits uninitialized
 * variables last-declared first). */
OptWork *gOptWork;
u8 gNetOptDefaults[0x10];

/* .data */
u32 gOptSysDefaults[4] = {
#include "select/006CAB60.inc"
};
u32 gOptGameDefaults[4] = {
#include "select/006CAB70.inc"
};
u32 gOptSoundDefaults[4] = {
#include "select/006CAB80.inc"
};
u32 gOptGameValueCounts[4] = {
#include "select/006CAB90.inc"
};
u32 gOptSoundValueCounts[4] = {
#include "select/006CABA0.inc"
};
Frame gOptPageFrame = {
#include "select/006CABB0.inc"
};
Frame gNetOptPageFrame = {
#include "select/006CABD0.inc"
};
u32 gNetOptValueNames[6] = {
    (u32)gNetOptValueNames_0, (u32)gNetOptValueNames_1, 0x00000000, 0x00000000,
    0x00000000, 0x00000000,
};
u32 gNetOptValueCounts[2] = {
#include "select/006CAC08.inc"
};
u32 gOptScreenPosRects[8] = {
#include "select/006CAC10.inc"
};
u32 gOptGameArrows[8] = {
#include "select/006CAC30.inc"
};
u32 gOptSoundArrows[6] = {
#include "select/006CAC50.inc"
};
u32 gNetOptArrows[2] = {
#include "select/006CAC68.inc"
};
s16 gOptCornerPosA[4][2] = {
#include "select/006CAC70.inc"
};
s16 gOptCornerPosB[4][2] = {
#include "select/006CAC80.inc"
};
u16 gOptCornerAngles[8][1] = {
#include "select/006CAC90.inc"
};
u32 gNetOptConfirmWin[8] = {
#include "select/006CACA0.inc"
};


void Option_SetDefaults(void) {
    memcpy(&gSysSave.options, &gOptSysDefaults, 0x10);
    SysSettings_Reset();
}

void Option_GameDefaults(void) {
    s8 *src;
    s8 *dst;
    s32 i;

    dst = &gSysSave.options.soundMode;
    src = (s8 *)gOptGameDefaults;
    for (i = 0; i < 0x10; i++, dst++, src++) {
        if (*src != -1) {
            *dst = *src;
        }
    }
    SysSettings_Reset();
    Stream_UpdateMaxVol(1);
}

void Option_NetDefaults(void) {
    s8 *src;
    s8 *dst;
    s32 i;

    dst = (s8 *)gSysSave.netOptions;
    src = (s8 *)gNetOptDefaults;
    for (i = 0; i < 0x10; i++, dst++, src++) {
        if (*src != -1) {
            *dst = *src;
        }
    }
}

void Option_SoundDefaults(void) {
    s8 *src;
    s8 *dst;
    s32 i;

    dst = &gSysSave.options.soundMode;
    src = (s8 *)gOptSoundDefaults;
    for (i = 0; i < 0x10; i++, dst++, src++) {
        if (*src != -1) {
            *dst = *src;
        }
    }
}

void Option_Task(TaskSlot *t) {
    u8 step;

    Gfx_SetBlend(4, 5);
    step = t->mode;
    switch (step) {                              /* irregular */
    case 0:
        Option_Start(t, 0);
        break;
    case 1:
        Option_Update(t);
        break;
    case 2:
        Option_Exit(t, 0);
        break;
    }
    Frame_Draw();
}

void Option_OpenGamePage(void) {
    gOptWork->nextPage = 0;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

void Option_OpenSoundPage(void) {
    gOptWork->nextPage = 1;
    gOptWork->unk2C = 0;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

void Option_OpenScreenPage(void) {
    gOptWork->nextPage = 2;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

void Option_OpenBrightnessPage(void) {
    gOptWork->nextPage = 3;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

void Option_DrawArrows(void) {
    Sprite spr;

    if (gOptWork->unkA0 != 0) {
        Gfx_SetState63(1);
        Tex_Prepare(1, 0x155);
        Tex_Bind(0x155);
        spr.x = gOptWork->unkA6 - gOptWork->unkAA;
        spr.y = gOptWork->unkAE;
        spr.w = 0x18;
        spr.h = 0x1E;
        spr.color = 0xFF32BC64;
        spr.u0 = 0xFF;
        spr.v0 = 0xEC;
        spr.u1 = 0xF0;
        spr.v1 = 0xFF;
        Sprite_Draw(&spr);
        spr.x = gOptWork->unkA8 + gOptWork->unkAC;
        spr.u0 = 0xF0;
        spr.u1 = 0xFF;
        Sprite_Draw(&spr);
    }
}

/* The arrow helpers are static: MWCC knows which registers they use, and
 * the input handlers (Option_GameInput, Option_SoundInput, NetOption_Input) keep
 * values in the others across the calls. */
static void Option_ResetArrows(void) {
    gOptWork->unkAA = 0;
    gOptWork->unkAC = 0;
    gOptWork->unkA4 = 0;
}

static void Option_PressLeft(void) {
    gOptWork->unkA4 = 1;
}

static void Option_PressRight(void) {
    gOptWork->unkA4 = 2;
}

static void Option_UpdateArrows(void) {
    s32 d;

    if ((d = gOptWork->unkA4) != 0) {
        if ((s16)d == 1) {
            gOptWork->unkAA = 0;
            gOptWork->unkAC++;
            if (gOptWork->unkAC >= 5) {
                gOptWork->unkAC = 0;
                gOptWork->unkA4 = 0;
            }
        } else {
            gOptWork->unkAC = 0;
            gOptWork->unkAA++;
            if (gOptWork->unkAA >= 5) {
                gOptWork->unkAA = 0;
                gOptWork->unkA4 = 0;
            }
        }
    }
}

void Options_Apply(void);

/* The game page's input: 1 when the page is left. */
s32 Option_GameInput(u16 trig, u16 rep) {
    s8 n;
    s8 k;
    s8 *v;

    if (rep & 0x2000) {
        do {
            gOptWork->unk28--;
            if (gOptWork->unk28 < 0) {
                gOptWork->unk28 = 8;
            }
        } while (gOptWork->unk28 == 7 && gOptWork->unk70 == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return 0;
    }
    if (rep & 0x1000) {
        do {
            gOptWork->unk28++;
            if (gOptWork->unk28 >= 9) {
                gOptWork->unk28 = 0;
            }
        } while (gOptWork->unk28 == 7 && gOptWork->unk70 == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return 0;
    }
    if (trig & 0x40) {
        Sound_PlayCancel();
        if (gOptWork->unk28 == 8) {
            return 1;
        }
        gOptWork->unk28 = 8;
        return 0;
    }
    n = ((s8 *)gOptGameValueCounts)[gOptWork->unk28];
    switch (gOptWork->unk28) {
    case 0:
        if (trig & 0x20) {
            Option_OpenSoundPage();
            Sound_PlaySe73();
        }
        break;
    case 1:
        v = &gSysSave.options.soundMode;
    wrap:
        k = *v;
        if (trig & 0x400) {
            k++;
            k %= n;
            Option_PressLeft();
        wrap_set:
            *v = k;
            if (gOptWork->unk28 == 1) {
                Options_Apply();
            }
            Sound_PlayCursorMove();
            break;
        }
        if (trig & 0x800) {
            k--;
            if (k < 0) {
                k += n;
            }
            Option_PressRight();
            goto wrap_set;
        }
        break;
    case 2:
        v = &gSysSave.options.musicVolume;
    clamp:
        k = *v;
        if ((rep & 0x400) && k < n - 1) {
            k++;
            k %= n;
            Option_PressLeft();
        clamp_set:
            *v = k;
            if (gOptWork->unk28 == 2 || gOptWork->unk28 == 3) {
                Options_Apply();
            }
            if (gOptWork->unk28 == 1) {
                Options_Apply();
            }
            Sound_PlayCursorMove();
            break;
        }
        if ((rep & 0x800) && k > 0) {
            k--;
            if (k < 0) {
                k += n;
            }
            Option_PressRight();
            goto clamp_set;
        }
        break;
    case 3:
        v = &gSysSave.options.seVolume;
        goto clamp;
    case 4:
        v = &gSysSave.options.equipView;
        goto wrap;
    case 5:
        if (trig & 0x20) {
            Option_OpenScreenPage();
            Sound_PlaySe73();
        }
        break;
    case 6:
        if (trig & 0x20) {
            Option_OpenBrightnessPage();
            Sound_PlaySe73();
        }
        break;
    case 7:
        if (trig & 0x20) {
            Sound_PlaySe73();
            Option_GameDefaults();
        }
        break;
    case 8:
        if (trig & 0x20) {
            Sound_PlaySe73();
            return 1;
        }
        break;
    }
    Option_UpdateArrows();
    return 0;
}

void Option_DrawText(s32 x, s32 y, char *str, s32 size, s32 color) {
    s32 d;

    Font_SetSpacing(size, size);
    d = (size > 0x16) ? 3 : 2;
    Font_SetPos(x + d, y + d);
    Font_SetColor(0x80000000);
    Font_Printf(str);
    Font_SetColor(color);
    Font_SetPos(x, y);
    Font_Printf(str);
}

extern char *gOptGameLabels[];     /* messages (select_psp.c) */
extern char *gOptGameHelp[];
extern char *gOptSoundLabels[];
extern char *gOptSoundHelp[];
extern char *gSoundModeTexts[];
extern char *gVolumeBarTexts[];
extern char *gOffOnTexts[];
extern char *gReverseTexts[];
extern char *gSizeTexts[];
extern char *gTypeNumTexts[];
struct Frame;
void WindowFrame_Draw(struct Frame *f, s16 *off);
void Font_SetPal(s32);
s32 Str_Length(char *);
void Sel_DrawBackground(u8);

/* The game settings page: one row per setting, the cursor row enlarged. */
void Option_DrawGamePage(void) {
    s32 w;
    u32 c;
    s32 sz;
    s32 ty;
    s32 x;
    char **label;
    s32 i;
    s32 y;
    u32 color;
    s16 (*arrow)[2];
    char **p;
    s32 size;
    char **tbl;
    s32 k;

    Sel_DrawBackground(0xFF);
    Sel_DrawHeader(2);
    WindowFrame_Draw(&gOptPageFrame, 0);
    Font_SetPal(0);
    color = 0;
    label = gOptGameLabels;
    i = 0;
    arrow = (s16 (*)[2])gOptGameArrows;
    x = gOptWork->unk40;
    y = gOptWork->unk44;
    for (; i < 9; i++, label++, arrow++, y += gOptWork->unk48) {
        gOptWork->unkA0 = 0;
        if (i == 8) {
            y += (s32)(0.5f * gOptWork->unk48);
        }
        ty = y;
        sz = gOptWork->unk38;
        if (i == gOptWork->unk28) {
            sz = sz * gOptWork->unk4C;
            ty = y - (sz - gOptWork->unk38) / 2;
            c = gOptWork->unk58;
        } else if (i == 7) {
            if (gOptWork->unk70 != 0) {
                c = gOptWork->unk5C;
            } else {
                c = gOptWork->unk60;
            }
        } else {
            c = gOptWork->unk54;
        }
        if (c != color) {
            color = c;
        }
        Option_DrawText(x, ty, *label, sz, color);
        size = gOptWork->unk38;
        switch (i) {
        case 1:
            k = gSysSave.options.soundMode;
            tbl = gSoundModeTexts;
        common:
            if (i == gOptWork->unk28) {
                gOptWork->unkA0 = 1;
                gOptWork->unkAE = y - 6;
            }
            p = &tbl[k];
            w = Str_Length(*p);
            Option_DrawText(gOptWork->unk50 + (s32)((320.0f - (size / 2) * w) / 2.0f),
                          y, *p, size, color);
            if (gOptWork->unkA0 != 0) {
                gOptWork->unkA6 = (*arrow)[0];
                gOptWork->unkA8 = (*arrow)[1];
            }
            Option_DrawArrows();
            break;
        case 2:
            k = gSysSave.options.musicVolume;
            tbl = gVolumeBarTexts;
            goto common;
        case 3:
            k = gSysSave.options.seVolume;
            tbl = gVolumeBarTexts;
            goto common;
        case 4:
            k = gSysSave.options.equipView;
            tbl = gOffOnTexts;
            goto common;
        }
    }
    Option_DrawText(gOptWork->unk64, gOptWork->unk68,
                  gOptGameHelp[gOptWork->unk28], gOptWork->unk38,
                  gOptWork->unk54);
}

void Vib_Start(s32, s32);

/* The sound page's input. */
void Option_SoundInput(u16 trig, u16 rep) {
    s8 n;
    s8 *v;
    s8 k;

    if (rep & 0x2000) {
        do {
            gOptWork->unk2C--;
            if (gOptWork->unk2C < 0) {
                gOptWork->unk2C = 6;
            }
        } while (gOptWork->unk2C == 5 && gOptWork->unk74 == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return;
    }
    if (rep & 0x1000) {
        do {
            gOptWork->unk2C++;
            if (gOptWork->unk2C >= 7) {
                gOptWork->unk2C = 0;
            }
        } while (gOptWork->unk2C == 5 && gOptWork->unk74 == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return;
    }
    if (trig & 0x40) {
        Sound_PlayCancel();
        if (gOptWork->unk2C == 6) {
            Option_OpenGamePage();
            return;
        }
        gOptWork->unk2C = 6;
        return;
    }
    n = ((s8 *)gOptSoundValueCounts)[gOptWork->unk2C];
    switch (gOptWork->unk2C) {
    case 2:
        v = &gSysSave.options.aimReverse;
    common:
        k = *v;
        if (trig & 0x400) {
            k++;
            k %= n;
            Option_PressLeft();
        set:
            *v = k;
            if (gOptWork->unk2C == 0 && k == 1) {
                Vib_Start(0, 1);
            }
            Sound_PlayCursorMove();
            break;
        }
        if (trig & 0x800) {
            k--;
            if (k < 0) {
                k += n;
            }
            Option_PressRight();
            goto set;
        }
        break;
    case 3:
        v = &gSysSave.options.camControl;
        goto common;
    case 4:
        v = &gSysSave.options.camFollow;
        goto common;
    case 0:
        v = &gSysSave.options.vibration;
        goto common;
    case 1:
        v = &gSysSave.options.chatSize;
        goto common;
    case 5:
        if (trig & 0x20) {
            Sound_PlaySe73();
            Option_SoundDefaults();
        }
        break;
    case 6:
        if (trig & 0x20) {
            Option_OpenGamePage();
            Sound_PlaySe73();
        }
        break;
    }
    Option_UpdateArrows();
}

/* The sound page: one row per setting, the cursor row enlarged. */
void Option_DrawSoundPage(void) {
    s32 w;
    u32 c;
    s32 sz;
    s32 ty;
    s32 x;
    char **label;
    s32 i;
    s32 y;
    u32 color;
    s16 (*arrow)[2];
    char **p;
    s32 size;
    char **tbl;
    s32 k;

    Sel_DrawBackground(0xFF);
    Sel_DrawHeader(0xA);
    WindowFrame_Draw(&gOptPageFrame, 0);
    Font_SetPal(0);
    color = 0;
    label = gOptSoundLabels;
    i = 0;
    arrow = (s16 (*)[2])gOptSoundArrows;
    x = gOptWork->unk40;
    y = gOptWork->unk44 + gOptWork->unk48;
    for (; i < 7; i++, label++, arrow++, y += gOptWork->unk48) {
        gOptWork->unkA0 = 0;
        if (i == 6) {
            y += (s32)(0.5f * gOptWork->unk48);
        }
        ty = y;
        sz = gOptWork->unk38;
        if (i == gOptWork->unk2C) {
            sz = sz * gOptWork->unk4C;
            ty = y - (sz - gOptWork->unk38) / 2;
            c = gOptWork->unk58;
        } else if (i == 5) {
            if (gOptWork->unk74 != 0) {
                c = gOptWork->unk5C;
            } else {
                c = gOptWork->unk60;
            }
        } else {
            c = gOptWork->unk54;
        }
        if (c != color) {
            color = c;
        }
        Option_DrawText(x, ty, *label, sz, color);
        size = gOptWork->unk38;
        switch (i) {
        case 2:
            k = gSysSave.options.aimReverse;
            tbl = gReverseTexts;
        common:
            if (i == gOptWork->unk2C) {
                gOptWork->unkA0 = 1;
                gOptWork->unkAE = y - 6;
            }
            p = &tbl[k];
            w = Str_Length(*p);
            Option_DrawText(gOptWork->unk50 + (s32)((320.0f - (size / 2) * w) / 2.0f),
                          y, *p, size, color);
            if (gOptWork->unkA0 != 0) {
                gOptWork->unkA6 = (*arrow)[0];
                gOptWork->unkA8 = (*arrow)[1];
            }
            Option_DrawArrows();
            break;
        case 3:
            k = gSysSave.options.camControl;
            tbl = gReverseTexts;
            goto common;
        case 4:
            k = gSysSave.options.camFollow;
            tbl = gTypeNumTexts;
            goto common;
        case 0:
            k = gSysSave.options.vibration;
            tbl = gOffOnTexts;
            goto common;
        case 1:
            k = gSysSave.options.chatSize;
            tbl = gSizeTexts;
            goto common;
        }
    }
    Option_DrawText(gOptWork->unk64, gOptWork->unk68,
                  gOptSoundHelp[gOptWork->unk2C], gOptWork->unk38,
                  gOptWork->unk54);
}

void Option_DrawFrame(void) {
    Sprite spr;

    Gfx_SetState63(1);
    Tex_Prepare(1, 4);
    Tex_Bind(4);
    Gs_SetState(0x6C, 0);
    spr.x = 0x7D;
    spr.y = 0x1D;
    spr.w = 0x186;
    spr.h = 0x186;
    spr.color = -1;
    spr.u0 = 0;
    spr.v0 = 0;
    spr.u1 = 0xFF;
    spr.v1 = 0xEF;
    Sprite_Draw(&spr);
    Gs_SetState(0x6C, 1);
}

void Display_SetOffset(s32 x, s32 y);

/* The screen position setting: moves the picture with the pad. */
/* Each offset is read into an int, then stored before the flags are set;
 * changed is tested as an s16. */
void Option_ScreenPosInput(u16 trig, u16 rep) {
    s32 changed;
    u16 moved;
    s8 *px;
    s8 *py;
    s32 y;

    changed = 0;
    moved = 0;
    px = &gSysSave.options.screenX;
    py = &gSysSave.options.screenY;
    gOptWork->unk6C = 0;
    if (trig & 0x40) {
        Option_OpenGamePage();
        Sound_PlayCancel();
        return;
    }
    if (trig & 0x200) {
        Sound_PlaySe73();
        *px = 0;
        changed = 1;
        *py = 0;
    } else {
        if (rep & 0x2000) {
            y = *py;
            if (y > -0x10) {
                *py = y - 1;
                moved |= 0x2000;
                changed = 1;
            } else {
                *py = -0x10;
            }
        }
        if (rep & 0x1000) {
            y = *py;
            if (y < 0x10) {
                *py = y + 1;
                moved |= 0x1000;
                changed = 1;
            } else {
                *py = 0x10;
            }
        }
        if (rep & 0x800) {
            y = *px;
            if (y > -0x10) {
                *px = y - 1;
                moved |= 0x800;
                changed = 1;
            } else {
                *px = -0x10;
            }
        }
        if (rep & 0x400) {
            y = *px;
            if (y < 0x10) {
                *px = y + 1;
                moved |= 0x400;
                changed = 1;
            } else {
                *px = 0x10;
            }
        }
        if ((s16)changed) {
            Sound_PlayCursorMove();
        }
    }
    if ((s16)changed) {
        Display_SetOffset(*px, *py);
    }
    gOptWork->unk6C = moved;
}

void Option_DrawCorners(void) {
    Sprite a;
    Sprite b;
    s32 i;

    Gfx_SetState63(1);
    Tex_Prepare(1, 4);
    Tex_Bind(4);
    Gs_SetState(0x6C, 0);
    a.w = 0x60;
    a.h = 0xA;
    a.color = -1;
    a.u0 = 0;
    a.v0 = 0xF0;
    a.u1 = 0x2F;
    a.v1 = 0xF7;
    b.w = 0x60;
    b.h = 0xA;
    b.color = -1;
    b.u0 = 0;
    b.v0 = 0xF8;
    b.u1 = 0x2F;
    b.v1 = 0xFF;
    for (i = 0; i < 4; i++) {
        a.x = gOptCornerPosA[i][0];
        a.y = gOptCornerPosA[i][1];
        Sprite_Draw(&a);
        b.x = gOptCornerPosB[i][0];
        b.y = gOptCornerPosB[i][1];
        TexRect_DrawRotated((struct TexRect *)&b, gOptCornerAngles[i][0]);
    }
    Gs_SetState(0x6C, 1);
}

const char D_006CDFA0[3] = {
#include "select/006CDFA0.inc"
};

extern char *gScreenPosTexts[];     /* messages (select_psp.c) */
extern char *gBackResetTexts[];
s32 sprintf(char *, const char *, ...);    /* sprintf */
s32 strlen(char *);                         /* strlen */
void Hud_DrawButtonIconWhite(s32, s16, s16, s32, f32);       /* a button icon */ /* unit's own declaration: x, y s16 (the definition has s32); with the definition's type Option_DrawScreenPosPage by 99 of 158, Option_DrawBrightnessPage by 19 of 138 words differ */
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void Sel_DrawBackground(u8);

/* The screen position page. */
void Option_DrawScreenPosPage(void) {
    Sprite spr;
    char buf[0x80];
    u16 *rec;
    s32 i;

    Sel_DrawBackground(0xFF);
    Option_DrawFrame();
    Option_DrawCorners();
    Sel_DrawHeader(3);
    i = 0xA0;
    Font_SetSpacing(0x12, 0x12);
    Text_DrawShadow(0xB8, 0x130, 1, 0, gScreenPosTexts[0]);
    Text_DrawShadow(0xB8, 0x154, 1, 0, gScreenPosTexts[1]);
    Hud_DrawButtonIconWhite(0x15, i, 0x185, 8, 1.0f);
    i += 0x18;
    Text_DrawShadow(i, 0x188, 1, 0, gBackResetTexts[0]);
    i += strlen(gBackResetTexts[0]) * 9 + 0x5A;
    Hud_DrawButtonIconWhite(3, i, 0x185, 8, 1.0f);
    Text_DrawShadow(i + 0x18, 0x188, 1, 0, gBackResetTexts[1]);
    Font_SetSpacing(0x24, 0x12);
    sprintf(buf, D_006CDFA0, gSysSave.options.screenX);
    Text_DrawShadow(0x1A2, 0x130, 1, 0, buf);
    sprintf(buf, D_006CDFA0, gSysSave.options.screenY);
    Text_DrawShadow(0x1A2, 0x154, 1, 0, buf);
    Tex_Prepare(1, 8);
    Tex_Bind(8);
    Gfx_SetState63(1);
    spr.w = 0x40;
    spr.h = 0x40;
    spr.u0 = 0;
    spr.v0 = 0xA0;
    spr.u1 = spr.u0 + 0x3F;
    spr.v1 = spr.v0 + 0x3F;
    rec = (u16 *)gOptScreenPosRects;
    for (i = 0; i < 4; i++, rec += 2) {
        spr.x = *rec++;
        spr.y = *rec++;
        if (gOptWork->unk6C & rec[0]) {
            spr.color = 0xFF00FFFF;
        } else {
            spr.color = -1;
        }
        TexRect_DrawRotated((struct TexRect *)&spr, rec[1]);
    }
}

/* The brightness page's input: left/right step the brightness (0x80 is the
 * middle; 1 per step above it, 2 below, within 0x39..0xA3). The game's
 * "changed" test compares the old offset with the same value recomputed,
 * so the cursor sound never plays. v an s32 (a u8 lets MWCC fold that
 * test), and the no-op cast keeps &gSysSettings.brightness in s0 for the load. */
void Option_BrightnessInput(u16 trig, u16 rep) {
    s32 v;
    u8 *q;
    u8 *p;
    s32 old;

    q = (u8 *)&gSysSave.options.brightness;
    p = (u8 *)&gSysSettings.brightness;
    v = *p;
    if (rep & 0x800) {
        old = v - 0x80;
        if (old == 0) {
            *p = v - 2;
        } else if (old > 0) {
            if (v - 1 > 0x38) {
                *p = v - 1;
            }
        } else {
            if (v - 2 > 0x38) {
                *p = v - 2;
            }
        }
        if (old != v - 0x80) {
            Sound_PlayCursorMove();
        }
        return;
    }
    if (rep & 0x400) {
        old = v - 0x80;
        if (old == 0) {
            *p = v + 1;
        } else if (old > 0) {
            if (v + 1 < 0xA4) {
                *p = v + 1;
            }
        } else {
            if (v + 2 < 0xA4) {
                *p = v + 2;
            }
        }
        if (old != v - 0x80) {
            Sound_PlayCursorMove();
        }
        return;
    }
    if (trig & 0x40) {
        *q = v;
        Option_OpenGamePage();
        Sound_PlayCancel();
        return;
    }
    if (trig & 0x200) {
        Sound_PlaySe73();
        *p = 0x80;
    }
}

extern char *gBackResetTexts[];     /* messages (select_psp.c) */
extern char *gOptBrightnessTexts[];
s32 sprintf(char *, const char *, ...);    /* sprintf */
s32 strlen(char *);                         /* strlen */
s32 Str_Length(char *);                         /* a message's width */
void Font_SetPal(s32);
void Hud_DrawButtonIconWhite(s32, s16, s16, s32, f32);       /* a button icon */ /* unit's own declaration: x, y s16 (the definition has s32); with the definition's type Option_DrawScreenPosPage by 99 of 158, Option_DrawBrightnessPage by 19 of 138 words differ */
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void Sel_DrawBackground(u8);
void Option_DrawBrightnessStrip(void);

/* The brightness setting screen. */
void Option_DrawBrightnessPage(void) {
    char buf[0x100];
    s32 v;
    s32 i;
    s32 y;
    char **s;
    s32 w;
    s32 x;

    Sel_DrawBackground(0xFF);
    Option_DrawFrame();
    Sel_DrawHeader(9);
    Option_DrawBrightnessStrip();
    v = gSysSettings.brightness - 0x80;
    if (v == 0) {
        sprintf(buf, " %d", v);
    } else if (v > 0) {
        sprintf(buf, "+%d", v);
    } else {
        sprintf(buf, D_006CDFA0, v / 2);
    }
    Font_SetPal(0);
    Option_DrawText(0x20C, 0x158, buf, 0x14, 0x80808080);
    s = gOptBrightnessTexts;
    y = 0xF0;
    for (i = 0; i < 3; i++, s++) {
        w = Str_Length(*s);
        if (w != 0) {
            Option_DrawText((640.0f - 10.0f * w) / 2.0f, y, *s, 0x14, 0x80808080);
            y += 0x18;
        }
    }
    x = 0xA0;
    Font_SetSpacing(0x12, 0x12);
    Hud_DrawButtonIconWhite(0x15, x, 0x185, 8, 1.0f);
    x += 0x18;
    Text_DrawShadow(x, 0x188, 1, 0, gBackResetTexts[0]);
    x += strlen(gBackResetTexts[0]) * 9 + 0x5A;
    Hud_DrawButtonIconWhite(3, x, 0x185, 8, 1.0f);
    Text_DrawShadow(x + 0x18, 0x188, 1, 0, gBackResetTexts[1]);
}

s32 Option_Differs(s8 *opts) {
    s32 i;
    s8 *p;
    s8 *cur;
    s8 v;

    p = opts;
    i = 0;
    cur = &gSysSave.options.soundMode;
loop_1:
    v = *p;
    if ((v != -1) && (v != *cur)) {
        return 1;
    }
    i += 1;
    p += 1;
    cur += 1;
    if (i >= 0x10) {
        return 0;
    }
    goto loop_1;
}

s32 Option_GameInput(u16 trig, u16 rep);
void Option_SoundInput(u16 trig, u16 rep);
void Option_ScreenPosInput(u16 trig, u16 rep);
void Option_BrightnessInput(u16 trig, u16 rep);
void Option_DrawSoundPage(void);
void Option_DrawScreenPosPage(void);
void Option_DrawGamePage(void);

/* OPTIONS' task: the four pages (game, sound, screen position, brightness). */
void Option_Update(TaskSlot *t) {
    u16 rep;
    u16 trig;
    s32 i;
    s8 *a;
    s8 *b;

    trig = gPadState[0].pressed;
    rep = gPadState[0].repeat | trig;
    switch (gOptWork->state) {
    case 0:
        SeqA_Start(0xC);
        gOptWork->state++;
    case 1:
        if ((u8)SeqA_GetState() != 1) {
            gOptWork->state++;
        case 2:
            switch (gOptWork->page) {
            case 0:
                if (Option_GameInput(trig, rep) == 1) {
                    t->mode = 2;
                }
                break;
            case 1:
                Option_SoundInput(trig, rep);
                break;
            case 2:
                Option_ScreenPosInput(trig, rep);
                break;
            case 3:
                Option_BrightnessInput(trig, rep);
                break;
            }
        }
        break;
    case 3:
        SeqA_Start(0xB);
        gOptWork->state++;
    case 4:
        if ((u8)SeqA_GetState() != 1) {
            gOptWork->page = gOptWork->nextPage;
            gOptWork->state = 0;
        }
        break;
    }
    gOptWork->unk70 = Option_Differs((s8 *)gOptGameDefaults);
    gOptWork->unk74 = Option_Differs((s8 *)gOptSoundDefaults);
    b = &gSysSave.options.soundMode;
    i = 0;
    a = (s8 *)gOptWork + 0x80;
    gOptWork->unk78 = 0;
    for (; i < 0x10; i++, a++, b++) {
        if (*a != *b) {
            gOptWork->unk78 = 1;
            break;
        }
    }
    switch (gOptWork->page) {
    case 0:
        Option_DrawGamePage();
        break;
    case 1:
        Option_DrawSoundPage();
        break;
    case 2:
        Option_DrawScreenPosPage();
        break;
    case 3:
        Option_DrawBrightnessPage();
        break;
    }
}

void Stream_Play(s32, s32);
void Gs_SetTableColor(s32);

/* OPTIONS' start (net: NETWORK OPTIONS). */
void Option_Start(TaskSlot *t, u16 net) {
    if ((u8)SeqA_GetState() != 1) {
        t->unk14 = 0;
        t->unk15 = 0;
        t->mode++;
        if (net == 0) {
            Stream_Play(0, 0xC9);
        }
        Gs_SetTableColor(0);
        gOptWork = (OptWork *)(gSysSettings.selWork + 0x100);
        memset(gOptWork, 0, 0xB0);
        memcpy((u8 *)gOptWork + 0x80, &gSysSave.options, 0x10);
        memcpy((u8 *)gOptWork + 0x90, gSysSave.netOptions, 0x10);
        gOptWork->unk38 = 0x12;
        gOptWork->unk3C = 0x12;
        if (net == 0) {
            gOptWork->unk40 = 0x3C;
            gOptWork->unk44 = 0x46;
            gOptWork->unk48 = 0x1E;
            gOptWork->unk4C = 1.35f;
            gOptWork->unk64 = 0x32;
            gOptWork->unk68 = 0x170;
            gOptWork->unk50 = 0x122;
        } else {
            gOptWork->unk40 = 0x3C;
            gOptWork->unk44 = 0x5A;
            gOptWork->unk48 = 0x1E;
            gOptWork->unk4C = 1.35f;
            gOptWork->unk64 = 0x32;
            gOptWork->unk68 = 0x15E;
            gOptWork->unk50 = 0x140;
        }
        gOptWork->unk54 = 0x80808080;
        gOptWork->unk58 = 0x80784100;
        gOptWork->unk5C = 0x802B802B;
        gOptWork->unk60 = 0x80404040;
    }
}

void Stream_Reset(void);
/* unit's own declaration: with the definition's u8 (void) (sub_main/unk_3D54B0.c), NetOption_Update: 68 of 196 words differ */
s32 NetMenu_Update(void);
void NetMenu_Open(u8 mode);
void Vib_StopAll(void);
void func_3240D0(void);
void Task_SetGmSub(void);
void Option_DrawGamePage(void);
void NetOption_DrawPage(void);

/* Leaving OPTIONS: saves the settings to the memory card first when needed. */
void Option_Exit(void *t, s32 net) {
    switch (gOptWork->step) {
    case 0:
        Stream_Reset();
        if (gOptWork->unk78 == 0) {
            SeqA_Start(0xB);
            gOptWork->step = 2;
            gOptWork->unk10 = 0;
        } else {
            gOptWork->step++;
            NetMenu_Open(1);
        }
        break;
    case 1:
        if ((u8)NetMenu_Update() != 0) {
            gOptWork->step++;
            SeqA_Start(0xB);
        }
        break;
    case 2:
        if ((u8)SeqA_GetState() != 1) {
            gOptWork->step++;
            gOptWork->unk10 = 0;
            SysSettings_Reset();
            Stream_Reset();
            Vib_StopAll();
            func_3240D0();
        }
        break;
    case 3:
        if (++gOptWork->unk10 >= 10) {
            gOptWork = NULL;
            TaskSlot_End(t);
            Task_SetGmSub();
            SeqA_Start(0xC);
            return;
        }
        break;
    }
    if ((u16)net == 0) {
        Option_DrawGamePage();
    } else {
        NetOption_DrawPage();
    }
}

void NetOption_OpenMainPage(void) {
    gOptWork->nextPage = 0;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

void NetOption_OpenNetMenuPage(void) {
    gOptWork->nextPage = 1;
    gOptWork->unk2C = 1;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

void NetOption_OpenConfirmPage(void) {
    gOptWork->nextPage = 2;
    gOptWork->unk2C = 1;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

void NetOption_Task(TaskSlot *t) {
    u8 step;

    Gfx_SetBlend(4, 5);
    Gfx_SetBlend(4, 5);
    step = t->mode;
    switch (step) {                              /* irregular */
    case 0:
        Option_Start(t, 1);
        break;
    case 1:
        NetOption_Update(t, 1);
        break;
    case 2:
        Option_Exit(t, 1);
        break;
    }
    Frame_Draw();
}

s32 NetOption_SettingsDiffer(s8 *opts) {
    s32 i;
    s8 *p;
    s8 *cur;
    s8 v;

    p = opts;
    i = 0;
    cur = (s8 *)gSysSave.netOptions;
loop_1:
    v = *p;
    if ((v != -1) && (v != *cur)) {
        return 1;
    }
    i += 1;
    p += 1;
    cur += 1;
    if (i >= 0x10) {
        return 0;
    }
    goto loop_1;
}

s32 NetOption_Input(u16 trig, u16 rep);
void NetOption_ConfirmInput(u16 pad, u16 rep);
void NetOption_DrawPage(void);
void func_006B98A0(void);
void NetOption_DrawConfirm(void);

/* NETWORK OPTIONS' task. */
void NetOption_Update(TaskSlot *t, s32 unused) {
    u16 rep;
    u16 trig;
    s32 i;
    s8 *a;
    s8 *b;

    trig = gPadState[0].pressed;
    rep = gPadState[0].repeat | trig;
    switch (gOptWork->state) {
    case 0:
        SeqA_Start(0xC);
        gOptWork->state++;
    case 1:
        if ((u8)SeqA_GetState() != 1) {
            gOptWork->state++;
        case 2:
            switch (gOptWork->page) {
            case 0:
                gOptWork->unk7C = NetOption_SettingsDiffer((s8 *)gNetOptDefaults);
                switch (NetOption_Input(trig, rep)) {
                case 0:
                    break;
                case 1:
                    NetOption_OpenNetMenuPage();
                    break;
                case 2:
                    NetOption_OpenConfirmPage();
                    break;
                case -1:
                    t->mode = 2;
                    b = (s8 *)gSysSave.netOptions;
                    i = 0;
                    a = (s8 *)gOptWork + 0x90;
                    gOptWork->unk78 = 0;
                    for (; i < 0x10; i++, a++, b++) {
                        if (*a != *b) {
                            gOptWork->unk78 = 1;
                            break;
                        }
                    }
                    break;
                }
                break;
            case 1:
                switch (gOptWork->step) {
                case 0:
                    NetMenu_Open(0x12);
                    gOptWork->step++;
                    break;
                case 1:
                    if ((u8)NetMenu_Update()) {
                        NetOption_OpenMainPage();
                    }
                    break;
                }
                break;
            case 2:
                NetOption_ConfirmInput(trig, rep);
                break;
            }
        }
        break;
    case 3:
        SeqA_Start(0xB);
        gOptWork->state++;
    case 4:
        if ((u8)SeqA_GetState() != 1) {
            gOptWork->page = gOptWork->nextPage;
            gOptWork->state = 0;
        }
        break;
    }
    switch (gOptWork->page) {
    case 0:
        NetOption_DrawPage();
        break;
    case 1:
        func_006B98A0();
        break;
    case 2:
        NetOption_DrawConfirm();
        break;
    }
}

/* "%s" comes here in the game's .rodata, before NetOption_Input's jump table,
 * although its only user is NetOption_DrawConfirm (a function inlined or stripped
 * earlier in the file may have used it first). */
INCLUDE_RODATA("asm/select/nonmatchings/select_option", D_006CDFB8);

/* NETWORK OPTIONS' input: 1 or 2 for the pages behind rows 2 and 3, -1
 * when the page is left. */
s32 NetOption_Input(u16 trig, u16 rep) {
    s8 n;
    s8 k;
    s8 *v;
    s32 cursor;

    if (rep & 0x2000) {
        do {
            gOptWork->unk28--;
            if (gOptWork->unk28 < 0) {
                gOptWork->unk28 = 5;
            }
        } while (gOptWork->unk28 == 4 && gOptWork->unk7C == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return 0;
    }
    if (rep & 0x1000) {
        do {
            gOptWork->unk28++;
            if (gOptWork->unk28 >= 6) {
                gOptWork->unk28 = 0;
            }
        } while (gOptWork->unk28 == 4 && gOptWork->unk7C == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return 0;
    }
    if (trig & 0x40) {
        Sound_PlayCancel();
        if (gOptWork->unk28 == 5) {
            return -1;
        }
        gOptWork->unk28 = 5;
        return 0;
    }
    n = ((s8 *)gNetOptValueCounts)[gOptWork->unk28];
    switch (cursor = gOptWork->unk28) {
    case 0:
    case 1:
        v = (s8 *)&gSysSave.netOptions[cursor];
        k = *v;
        if (rep & 0x400) {
            k++;
            k %= n;
            Option_PressLeft();
        set:
            *v = k;
            Sound_PlayCursorMove();
            break;
        }
        if (rep & 0x800) {
            k--;
            if (k < 0) {
                k += n;
            }
            Option_PressRight();
            goto set;
        }
        break;
    case 2:
        if (trig & 0x20) {
            Sound_PlaySe73();
            return 1;
        }
        break;
    case 3:
        if (trig & 0x20) {
            Sound_PlaySe73();
            return 2;
        }
        break;
    case 4:
        if (trig & 0x20) {
            Sound_PlaySe73();
            Option_NetDefaults();
        }
        break;
    case 5:
        if (trig & 0x20) {
            Sound_PlaySe73();
            return -1;
        }
        break;
    }
    Option_UpdateArrows();
    return 0;
}

extern char *gNetOptLabels[];     /* messages (select_psp.c) */
extern char *gNetOptHelp[];

/* NETWORK OPTIONS' page. */
void NetOption_DrawPage(void) {
    s32 w;
    u32 c;
    s32 sz;
    s32 ty;
    s32 x;
    char **label;
    s32 i;
    s32 y;
    u32 color;
    s16 (*arrow)[2];
    char **p;
    s32 size;
    char ***tbl;
    u8 *set;
    s32 xx;

    Sel_DrawBackground(0xFF);
    Sel_DrawHeader(7);
    WindowFrame_Draw(&gNetOptPageFrame, 0);
    label = gNetOptLabels;
    i = 0;
    tbl = (char ***)gNetOptValueNames;
    set = (u8 *)&gSysSave.options;
    arrow = (s16 (*)[2])gNetOptArrows;
    x = gOptWork->unk40;
    y = gOptWork->unk44;
    for (; i < 6; i++, tbl++, set++, arrow++, y += gOptWork->unk48) {
        gOptWork->unkA0 = 0;
        if (i == 5) {
            y += (s32)(0.5f * gOptWork->unk48);
        }
        ty = y;
        sz = gOptWork->unk38;
        if (i == gOptWork->unk28) {
            sz = sz * gOptWork->unk4C;
            ty = y - (sz - gOptWork->unk38) / 2;
            c = gOptWork->unk58;
        } else if (i == 4) {
            if (gOptWork->unk7C != 0) {
                c = gOptWork->unk5C;
            } else {
                c = gOptWork->unk60;
            }
        } else {
            c = gOptWork->unk54;
        }
        if (c != color) {
            color = c;
        }
        Option_DrawText(x, ty, *label, sz, color);
        size = gOptWork->unk38;
        switch (i) {
        case 0:
        case 1:
            if (i == gOptWork->unk28) {
                gOptWork->unkA0 = 1;
                if (i == 0) {
                    gOptWork->unkAE = y + gOptWork->unk48 / 2 - 6;
                } else {
                    gOptWork->unkAE = y - 6;
                }
            }
            p = &(*tbl)[set[0x10]];
            w = Str_Length(*p);
            xx = gOptWork->unk50 + (s32)((320.0f - (size / 2) * w) / 2.0f);
            Font_SetSpacing(size, size);
            if (i == 0) {
                Option_DrawText(xx, y + gOptWork->unk48 / 2, *p, size, color);
            } else {
                Option_DrawText(xx, y, *p, size, color);
            }
            if (gOptWork->unkA0 != 0) {
                gOptWork->unkA6 = (*arrow)[0];
                gOptWork->unkA8 = (*arrow)[1];
            }
            Option_DrawArrows();
            break;
        case 2:
        case 3:
        case 5:
            break;
        }
        label++;
        if (i == 0) {
            y += (s32)(gOptWork->unk48 * (1.0f / gOptWork->unk4C));
        }
    }
    Option_DrawText(gOptWork->unk64, gOptWork->unk68,
                  gNetOptHelp[gOptWork->unk28], gOptWork->unk38,
                  gOptWork->unk54);
}

void LoadOverlay(s32 id);
void func_ACE630(void);

void NetOption_ConfirmInput(u16 pad, u16 rep) {
    u16 b;

    switch (gOptWork->step) {
    case 0:
        gOptWork->unk2C = 1;
        gOptWork->step++;
        gOptWork->unk8 = 0;
        break;
    case 1:
        switch (gOptWork->unk8) {
        case 0:
            b = pad;
            if ((b & 0x400) || (b & 0x800)) {
                Sound_PlayCursorMove();
                gOptWork->unk2C ^= 1;
            } else if (b & 0x20) {
                Sound_PlaySe73();
                if (gOptWork->unk2C == 0) {
                    gOptWork->unk8++;
                    gOptWork->unk10 = 0x14;
                } else {
                    NetOption_OpenMainPage();
                }
            } else if (b & 0x40) {
                Sound_PlayCancel();
                if (gOptWork->unk2C == 0) {
                    gOptWork->unk2C = 1;
                } else {
                    NetOption_OpenMainPage();
                }
            }
            break;
        case 1:
            gOptWork->unk10--;
            if (gOptWork->unk10 == 0) {
                LoadOverlay(9);
                func_ACE630();
            }
            break;
        }
        break;
    }
}

void func_006B98A0(void) {

}

extern char *gNetOptConfirmTexts[];      /* NETWORK OPTIONS' notice (select_psp.c) */
extern char *gPouchMenuPanel2_6[];        /* sub_main: the YES/NO strings */
extern char D_006CDFB8[];
void Text_SetColor(s16 color);
void Text_PrintfColored(const char *fmt, ...);
s32 Str_Length(char *);
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
struct Window;
void Window_DrawTextAlpha(struct Window *w, u8 *text, u8 alpha);
void Sel_DrawBackground(u8);

void NetOption_DrawConfirm(void) {
    s32 y;
    s32 i;
    char **s;
    s32 x;
    char **p;
    s32 j;
    s32 color;
    s32 w;

    Sel_DrawBackground(0xFF);
    Sel_DrawHeader(7);
    Window_DrawTextAlpha((struct Window *) gNetOptConfirmWin, 0, 0x80);
    Font_SetSpacing(0x12, 0x12);
    s = gNetOptConfirmTexts;
    y = 0x78;
    Text_SetColor(0);
    for (i = 0; i < 6; i++) {
        if (**s != ' ') {
            w = Str_Length(*s);
            w = (0x280 - w * 9) / 2;
            Font_SetPos(w, y);
            Text_PrintfColored(D_006CDFB8, *s);
        }
        y += 0x18;
        s++;
    }
    for (j = 0, p = gPouchMenuPanel2_6; j < 2; j++, p++) {
        color = (gOptWork->unk2C == j) ? 5 : 0;
        x = 0xF0;
        if (j == 0) {
            x += 0x14;
        } else {
            x += 0x82;
        }
        Text_SetColor(color);
        if (j == 0) {
            Font_SetPos(x - 0x14, 0x144);
        } else {
            Font_SetPos(x - 0x1E, 0x144);
        }
        Text_PrintfColored(D_006CDFB8, *p);
        if (gOptWork->unk2C == j) {
            Menu_DrawGlowBar(x, 0x142, 0x78, 0x18, 0xFF20C0C0);
        }
    }
    Font_SetSpacing(0x12, 0x12);
}

void Option_DrawBrightnessStrip(void) {
    Sprite spr;

    Gfx_SetState63(1);
    Tex_Prepare(1, 5);
    Tex_Bind(5);
    Gs_SetState(0x6C, 0);
    spr.x = 0;
    spr.y = 0x12C;
    spr.w = 0x280;
    spr.h = 0x50;
    spr.color = -1;
    spr.u0 = 0;
    spr.v0 = 0;
    spr.u1 = 0x1FF;
    spr.v1 = 0x3F;
    Sprite_Draw(&spr);
    Gs_SetState(0x6C, 1);
}
