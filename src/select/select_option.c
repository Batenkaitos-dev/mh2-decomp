/*
 * select: the options and the network options.
 *
 * Both run as a task (Option_Task, NetOption_Task) over OptWork (select.h,
 * gOptWork). The options have four pages: the main one (sound mode,
 * volumes, the equipment view, and rows that open the others), the
 * controls (vibration, chat size, aim and camera), the screen position
 * and the brightness. The network options have their settings, the
 * network menu (sub_main's NetMenu, run as a page) and a confirmation that
 * starts the network setup guide (the yn overlay). Leaving saves the
 * settings to the memory card when they changed.
 *
 * The file owns its .data, .rodata and .bss. The two share it: the network
 * options call the options' static arrow helpers, and their tables lie
 * among the options' in .data.
 */
#include "common.h"
#include "card.h"
#include "window.h"
#include "syssettings.h"
#include "pad.h"
#include "m2c_macros.h"
#include "select.h"
#include "syssave.h"
#include "render_state.h"
#include "stageobj.h"
#include "stream.h"
#include "area_draw.h"
#include "camera.h"
#include "text.h"
#include "display_util.h"
#include "font.h"
#include "frame.h"
#include "game_util.h"
#include "main.h"
#include "sys_main.h"
#include "vibration.h"
#include "screen_fade.h"
#include "sprite_queue.h"

struct TexRect;

/* .bss, declared from the highest address down (MWCC emits uninitialized
 * variables last-declared first). */
OptWork *gOptWork;
u8 gNetOptDefaults[16];

/* .data */
SysOptions gOptSysDefaults = {
#include "select/006CAB60.inc"
};
s8 gOptMainDefaults[16] = {
#include "select/006CAB70.inc"
};
s8 gOptControlDefaults[16] = {
#include "select/006CAB80.inc"
};
s8 gOptMainValueCounts[16] = {
#include "select/006CAB90.inc"
};
s8 gOptControlValueCounts[16] = {
#include "select/006CABA0.inc"
};
Frame gOptPageFrame = {
#include "select/006CABB0.inc"
};
Frame gNetOptPageFrame = {
#include "select/006CABD0.inc"
};
char **gNetOptValueNames[6] = {
    gNetOptValueNames_0, gNetOptValueNames_1, NULL, NULL,
    NULL, NULL,
};
s8 gNetOptValueCounts[8] = {
#include "select/006CAC08.inc"
};
u16 gOptScreenPosRects[16] = {
#include "select/006CAC10.inc"
};
s16 gOptMainArrows[8][2] = {
#include "select/006CAC30.inc"
};
s16 gOptControlArrows[6][2] = {
#include "select/006CAC50.inc"
};
s16 gNetOptArrows[2][2] = {
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
Window gNetOptConfirmWin[2] = {
#include "select/006CACA0.inc"
};


/* Resets every option to its default and applies them. */
void Option_SetDefaults(void)
{
    memcpy(&gSysSave.options, &gOptSysDefaults, 16);
    SysSettings_Reset();
}

/*
 * Resets the main page's settings to their defaults (gOptMainDefaults,
 * whose -1 bytes leave a setting alone), applies them and the music volume.
 */
void Option_MainDefaults(void)
{
    s8 *src;
    s8 *dst;
    s32 i;

    dst = &gSysSave.options.soundMode;
    src = (s8 *)gOptMainDefaults;
    for (i = 0; i < 16; i++, dst++, src++) {
        if (*src != -1) {
            *dst = *src;
        }
    }
    SysSettings_Reset();
    Stream_UpdateMaxVol(1);
}

/*
 * Resets the network settings to their defaults (gNetOptDefaults, whose -1
 * bytes leave a setting alone).
 */
void Option_NetDefaults(void)
{
    s8 *src;
    s8 *dst;
    s32 i;

    dst = (s8 *)gSysSave.netOptions;
    src = (s8 *)gNetOptDefaults;
    for (i = 0; i < 16; i++, dst++, src++) {
        if (*src != -1) {
            *dst = *src;
        }
    }
}

/*
 * Resets the controls page's settings to their defaults (gOptControlDefaults,
 * whose -1 bytes leave a setting alone).
 */
void Option_ControlDefaults(void)
{
    s8 *src;
    s8 *dst;
    s32 i;

    dst = &gSysSave.options.soundMode;
    src = (s8 *)gOptControlDefaults;
    for (i = 0; i < 16; i++, dst++, src++) {
        if (*src != -1) {
            *dst = *src;
        }
    }
}

/*
 * The options' task: starts them, runs the pages, then leaves (task->mode
 * 0, 1, 2).
 */
void Option_Task(TaskSlot *task)
{
    u8 step;

    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    step = task->mode;
    switch (step) {
    case 0:
        Option_Start(task, 0);
        break;
    case 1:
        Option_Update(task);
        break;
    case 2:
        Option_Exit(task, 0);
        break;
    }
    Frame_Draw();
}

/* Fades to the main page. */
void Option_OpenMainPage(void)
{
    gOptWork->nextPage = 0;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

/* Fades to the controls page, its cursor on the first row. */
void Option_OpenControlPage(void)
{
    gOptWork->nextPage = 1;
    gOptWork->subCursor = 0;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

/* Fades to the screen position page. */
void Option_OpenScreenPage(void)
{
    gOptWork->nextPage = 2;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

/* Fades to the brightness page. */
void Option_OpenBrightnessPage(void)
{
    gOptWork->nextPage = 3;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

/*
 * Draws the left and right arrows of the row under the cursor, with their
 * bounce offsets.
 */
void Option_DrawArrows(void)
{
    Sprite spr;

    if (gOptWork->arrowsShown != 0) {
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Tex_Prepare(1, 341);
        Tex_Bind(341);
        spr.x = gOptWork->arrowLeftX - gOptWork->arrowLeftOfs;
        spr.y = gOptWork->arrowY;
        spr.w = 24;
        spr.h = 30;
        spr.color = 0xFF32BC64;
        spr.u0 = 255;
        spr.v0 = 236;
        spr.u1 = 240;
        spr.v1 = 255;
        Sprite_Draw(&spr);
        spr.x = gOptWork->arrowRightX + gOptWork->arrowRightOfs;
        spr.u0 = 240;
        spr.u1 = 255;
        Sprite_Draw(&spr);
    }
}

/* Stops the arrows' bounce. The arrow helpers are static, as the input
 * handlers' code needs (it keeps values in registers across the calls). */
static void Option_ResetArrows(void)
{
    gOptWork->arrowLeftOfs = 0;
    gOptWork->arrowRightOfs = 0;
    gOptWork->arrowBounce = 0;
}

/* Bounces the right arrow (the value went up). */
static void Option_BounceRightArrow(void)
{
    gOptWork->arrowBounce = 1;
}

/* Bounces the left arrow (the value went down). */
static void Option_BounceLeftArrow(void)
{
    gOptWork->arrowBounce = 2;
}

/* Steps the arrow's bounce: 5 frames, then it stops. */
static void Option_UpdateArrows(void)
{
    s32 bounce;

    if ((bounce = gOptWork->arrowBounce) != 0) {
        if ((s16)bounce == 1) {
            gOptWork->arrowLeftOfs = 0;
            gOptWork->arrowRightOfs++;
            if (gOptWork->arrowRightOfs >= 5) {
                gOptWork->arrowRightOfs = 0;
                gOptWork->arrowBounce = 0;
            }
        } else {
            gOptWork->arrowRightOfs = 0;
            gOptWork->arrowLeftOfs++;
            if (gOptWork->arrowLeftOfs >= 5) {
                gOptWork->arrowLeftOfs = 0;
                gOptWork->arrowBounce = 0;
            }
        }
    }
}

/*
 * The main page's input: up and down move the cursor (skipping the defaults
 * row while nothing differs from them), left and right change the value
 * under the cursor (the sound mode and the equipment view wrap, the volumes
 * stop at their ends), confirm opens the controls, screen position or
 * brightness page, resets to the defaults or leaves. Returns 1 when the page
 * is left. The gotos share the value code between rows, as the game's code
 * does.
 */
s32 Option_MainInput(u16 trig, u16 rep)
{
    s8 count;
    s8 value;
    s8 *setting;

    if (rep & 0x2000) {
        do {
            gOptWork->cursor--;
            if (gOptWork->cursor < 0) {
                gOptWork->cursor = 8;
            }
        } while (gOptWork->cursor == 7 && gOptWork->mainDiffers == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return 0;
    }
    if (rep & 0x1000) {
        do {
            gOptWork->cursor++;
            if (gOptWork->cursor >= 9) {
                gOptWork->cursor = 0;
            }
        } while (gOptWork->cursor == 7 && gOptWork->mainDiffers == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return 0;
    }
    if (trig & 0x40) {
        Sound_PlayCancel();
        if (gOptWork->cursor == 8) {
            return 1;
        }
        gOptWork->cursor = 8;
        return 0;
    }
    count = ((s8 *)gOptMainValueCounts)[gOptWork->cursor];
    switch (gOptWork->cursor) {
    case 0:
        if (trig & 0x20) {
            Option_OpenControlPage();
            Sound_PlayConfirm();
        }
        break;
    case 1:
        setting = &gSysSave.options.soundMode;
    wrap:
        value = *setting;
        if (trig & 0x400) {
            value++;
            value %= count;
            Option_BounceRightArrow();
        wrap_set:
            *setting = value;
            if (gOptWork->cursor == 1) {
                Options_Apply();
            }
            Sound_PlayCursorMove();
            break;
        }
        if (trig & 0x800) {
            value--;
            if (value < 0) {
                value += count;
            }
            Option_BounceLeftArrow();
            goto wrap_set;
        }
        break;
    case 2:
        setting = &gSysSave.options.musicVolume;
    clamp:
        value = *setting;
        if ((rep & 0x400) && value < count - 1) {
            value++;
            value %= count;
            Option_BounceRightArrow();
        clamp_set:
            *setting = value;
            if (gOptWork->cursor == 2 || gOptWork->cursor == 3) {
                Options_Apply();
            }
            if (gOptWork->cursor == 1) {
                Options_Apply();
            }
            Sound_PlayCursorMove();
            break;
        }
        if ((rep & 0x800) && value > 0) {
            value--;
            if (value < 0) {
                value += count;
            }
            Option_BounceLeftArrow();
            goto clamp_set;
        }
        break;
    case 3:
        setting = &gSysSave.options.seVolume;
        goto clamp;
    case 4:
        setting = &gSysSave.options.equipView;
        goto wrap;
    case 5:
        if (trig & 0x20) {
            Option_OpenScreenPage();
            Sound_PlayConfirm();
        }
        break;
    case 6:
        if (trig & 0x20) {
            Option_OpenBrightnessPage();
            Sound_PlayConfirm();
        }
        break;
    case 7:
        if (trig & 0x20) {
            Sound_PlayConfirm();
            Option_MainDefaults();
        }
        break;
    case 8:
        if (trig & 0x20) {
            Sound_PlayConfirm();
            return 1;
        }
        break;
    }
    Option_UpdateArrows();
    return 0;
}

/* Draws a text with a black shadow (3 pixels off for large letters, else 2). */
void Option_DrawText(s32 x, s32 y, char *str, s32 size, s32 color)
{
    s32 shadowOfs;

    Font_SetSpacing(size, size);
    shadowOfs = (size > 22) ? 3 : 2;
    Font_SetPos(x + shadowOfs, y + shadowOfs);
    Font_SetColor(0x80000000);
    Font_Printf(str);
    Font_SetColor(color);
    Font_SetPos(x, y);
    Font_Printf(str);
}

extern char *gOptMainLabels[];     /* messages (select_psp.c) */
extern char *gOptMainHelp[];
extern char *gOptControlLabels[];
extern char *gOptControlHelp[];
extern char *gSoundModeTexts[];
extern char *gVolumeBarTexts[];
extern char *gOffOnTexts[];
extern char *gReverseTexts[];
extern char *gSizeTexts[];
extern char *gTypeNumTexts[];
struct Frame;
s32 Str_Length(char *);

/*
 * The main page: one row per setting with its value and arrows, the row
 * under the cursor enlarged, then the help line of that row. The gotos
 * share the value code between rows, as the game's code does.
 */
void Option_DrawMainPage(void)
{
    s32 width;
    u32 rowColor;
    s32 rowSize;
    s32 rowY;
    s32 x;
    char **label;
    s32 i;
    s32 y;
    u32 color;
    s16 (*arrow)[2];
    char **valueText;
    s32 size;
    char **valueTexts;
    s32 value;

    Sel_DrawBackground(255);
    Sel_DrawHeader(2);
    WindowFrame_Draw(&gOptPageFrame, 0);
    Font_SetPal(0);
    color = 0;
    label = gOptMainLabels;
    i = 0;
    arrow = (s16 (*)[2])gOptMainArrows;
    x = gOptWork->rowX;
    y = gOptWork->rowY;
    for (; i < 9; i++, label++, arrow++, y += gOptWork->rowStep) {
        gOptWork->arrowsShown = 0;
        if (i == 8) {
            y += (s32)(0.5f * gOptWork->rowStep);
        }
        rowY = y;
        rowSize = gOptWork->textSize;
        if (i == gOptWork->cursor) {
            rowSize = rowSize * gOptWork->selScale;
            rowY = y - (rowSize - gOptWork->textSize) / 2;
            rowColor = gOptWork->selColor;
        } else if (i == 7) {
            if (gOptWork->mainDiffers != 0) {
                rowColor = gOptWork->defaultsColor;
            } else {
                rowColor = gOptWork->dimColor;
            }
        } else {
            rowColor = gOptWork->textColor;
        }
        if (rowColor != color) {
            color = rowColor;
        }
        Option_DrawText(x, rowY, *label, rowSize, color);
        size = gOptWork->textSize;
        switch (i) {
        case 1:
            value = gSysSave.options.soundMode;
            valueTexts = gSoundModeTexts;
        common:
            if (i == gOptWork->cursor) {
                gOptWork->arrowsShown = 1;
                gOptWork->arrowY = y - 6;
            }
            valueText = &valueTexts[value];
            width = Str_Length(*valueText);
            Option_DrawText(gOptWork->valueX + (s32)((320.0f - (size / 2) * width) / 2.0f),
                          y, *valueText, size, color);
            if (gOptWork->arrowsShown != 0) {
                gOptWork->arrowLeftX = (*arrow)[0];
                gOptWork->arrowRightX = (*arrow)[1];
            }
            Option_DrawArrows();
            break;
        case 2:
            value = gSysSave.options.musicVolume;
            valueTexts = gVolumeBarTexts;
            goto common;
        case 3:
            value = gSysSave.options.seVolume;
            valueTexts = gVolumeBarTexts;
            goto common;
        case 4:
            value = gSysSave.options.equipView;
            valueTexts = gOffOnTexts;
            goto common;
        }
    }
    Option_DrawText(gOptWork->helpX, gOptWork->helpY,
                  gOptMainHelp[gOptWork->cursor], gOptWork->textSize,
                  gOptWork->textColor);
}

/*
 * The controls page's input, as Option_MainInput's for its rows (every
 * value wraps); turning vibration on starts a short vibration. The gotos
 * share the value code between rows, as the game's code does.
 */
void Option_ControlInput(u16 trig, u16 rep)
{
    s8 count;
    s8 *setting;
    s8 value;

    if (rep & 0x2000) {
        do {
            gOptWork->subCursor--;
            if (gOptWork->subCursor < 0) {
                gOptWork->subCursor = 6;
            }
        } while (gOptWork->subCursor == 5 && gOptWork->controlsDiffer == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return;
    }
    if (rep & 0x1000) {
        do {
            gOptWork->subCursor++;
            if (gOptWork->subCursor >= 7) {
                gOptWork->subCursor = 0;
            }
        } while (gOptWork->subCursor == 5 && gOptWork->controlsDiffer == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return;
    }
    if (trig & 0x40) {
        Sound_PlayCancel();
        if (gOptWork->subCursor == 6) {
            Option_OpenMainPage();
            return;
        }
        gOptWork->subCursor = 6;
        return;
    }
    count = ((s8 *)gOptControlValueCounts)[gOptWork->subCursor];
    switch (gOptWork->subCursor) {
    case 2:
        setting = &gSysSave.options.aimReverse;
    common:
        value = *setting;
        if (trig & 0x400) {
            value++;
            value %= count;
            Option_BounceRightArrow();
        set:
            *setting = value;
            if (gOptWork->subCursor == 0 && value == 1) {
                Vib_Start(0, 1);
            }
            Sound_PlayCursorMove();
            break;
        }
        if (trig & 0x800) {
            value--;
            if (value < 0) {
                value += count;
            }
            Option_BounceLeftArrow();
            goto set;
        }
        break;
    case 3:
        setting = &gSysSave.options.camControl;
        goto common;
    case 4:
        setting = &gSysSave.options.camFollow;
        goto common;
    case 0:
        setting = &gSysSave.options.vibration;
        goto common;
    case 1:
        setting = &gSysSave.options.chatSize;
        goto common;
    case 5:
        if (trig & 0x20) {
            Sound_PlayConfirm();
            Option_ControlDefaults();
        }
        break;
    case 6:
        if (trig & 0x20) {
            Option_OpenMainPage();
            Sound_PlayConfirm();
        }
        break;
    }
    Option_UpdateArrows();
}

/*
 * The controls page: one row per setting with its value and arrows, the row
 * under the cursor enlarged, then the help line of that row. The gotos
 * share the value code between rows, as the game's code does.
 */
void Option_DrawControlPage(void)
{
    s32 width;
    u32 rowColor;
    s32 rowSize;
    s32 rowY;
    s32 x;
    char **label;
    s32 i;
    s32 y;
    u32 color;
    s16 (*arrow)[2];
    char **valueText;
    s32 size;
    char **valueTexts;
    s32 value;

    Sel_DrawBackground(255);
    Sel_DrawHeader(10);
    WindowFrame_Draw(&gOptPageFrame, 0);
    Font_SetPal(0);
    color = 0;
    label = gOptControlLabels;
    i = 0;
    arrow = (s16 (*)[2])gOptControlArrows;
    x = gOptWork->rowX;
    y = gOptWork->rowY + gOptWork->rowStep;
    for (; i < 7; i++, label++, arrow++, y += gOptWork->rowStep) {
        gOptWork->arrowsShown = 0;
        if (i == 6) {
            y += (s32)(0.5f * gOptWork->rowStep);
        }
        rowY = y;
        rowSize = gOptWork->textSize;
        if (i == gOptWork->subCursor) {
            rowSize = rowSize * gOptWork->selScale;
            rowY = y - (rowSize - gOptWork->textSize) / 2;
            rowColor = gOptWork->selColor;
        } else if (i == 5) {
            if (gOptWork->controlsDiffer != 0) {
                rowColor = gOptWork->defaultsColor;
            } else {
                rowColor = gOptWork->dimColor;
            }
        } else {
            rowColor = gOptWork->textColor;
        }
        if (rowColor != color) {
            color = rowColor;
        }
        Option_DrawText(x, rowY, *label, rowSize, color);
        size = gOptWork->textSize;
        switch (i) {
        case 2:
            value = gSysSave.options.aimReverse;
            valueTexts = gReverseTexts;
        common:
            if (i == gOptWork->subCursor) {
                gOptWork->arrowsShown = 1;
                gOptWork->arrowY = y - 6;
            }
            valueText = &valueTexts[value];
            width = Str_Length(*valueText);
            Option_DrawText(gOptWork->valueX + (s32)((320.0f - (size / 2) * width) / 2.0f),
                          y, *valueText, size, color);
            if (gOptWork->arrowsShown != 0) {
                gOptWork->arrowLeftX = (*arrow)[0];
                gOptWork->arrowRightX = (*arrow)[1];
            }
            Option_DrawArrows();
            break;
        case 3:
            value = gSysSave.options.camControl;
            valueTexts = gReverseTexts;
            goto common;
        case 4:
            value = gSysSave.options.camFollow;
            valueTexts = gTypeNumTexts;
            goto common;
        case 0:
            value = gSysSave.options.vibration;
            valueTexts = gOffOnTexts;
            goto common;
        case 1:
            value = gSysSave.options.chatSize;
            valueTexts = gSizeTexts;
            goto common;
        }
    }
    Option_DrawText(gOptWork->helpX, gOptWork->helpY,
                  gOptControlHelp[gOptWork->subCursor], gOptWork->textSize,
                  gOptWork->textColor);
}

/* Draws the screen position page's frame (system texture 4). */
void Option_DrawFrame(void)
{
    Sprite spr;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 4);
    Tex_Bind(4);
    Gs_SetState(RS_ZWRITE, 0);
    spr.x = 125;
    spr.y = 29;
    spr.w = 390;
    spr.h = 390;
    spr.color = -1;
    spr.u0 = 0;
    spr.v0 = 0;
    spr.u1 = 255;
    spr.v1 = 239;
    Sprite_Draw(&spr);
    Gs_SetState(RS_ZWRITE, 1);
}

/* The screen position page's input: the D-pad moves the picture (-16 to 16
 * each way), 0x200 centres it, cancel goes back. screenPosMoved keeps the
 * directions moved this frame. The int offset and the (s16) tests of
 * changed are what the match needs. */
void Option_ScreenPosInput(u16 trig, u16 rep)
{
    s32 changed;
    u16 moved;
    s8 *offsetX;
    s8 *offsetY;
    s32 offset;

    changed = 0;
    moved = 0;
    offsetX = &gSysSave.options.screenX;
    offsetY = &gSysSave.options.screenY;
    gOptWork->screenPosMoved = 0;
    if (trig & 0x40) {
        Option_OpenMainPage();
        Sound_PlayCancel();
        return;
    }
    if (trig & 0x200) {
        Sound_PlayConfirm();
        *offsetX = 0;
        changed = 1;
        *offsetY = 0;
    } else {
        if (rep & 0x2000) {
            offset = *offsetY;
            if (offset > -16) {
                *offsetY = offset - 1;
                moved |= 0x2000;
                changed = 1;
            } else {
                *offsetY = -16;
            }
        }
        if (rep & 0x1000) {
            offset = *offsetY;
            if (offset < 16) {
                *offsetY = offset + 1;
                moved |= 0x1000;
                changed = 1;
            } else {
                *offsetY = 16;
            }
        }
        if (rep & 0x800) {
            offset = *offsetX;
            if (offset > -16) {
                *offsetX = offset - 1;
                moved |= 0x800;
                changed = 1;
            } else {
                *offsetX = -16;
            }
        }
        if (rep & 0x400) {
            offset = *offsetX;
            if (offset < 16) {
                *offsetX = offset + 1;
                moved |= 0x400;
                changed = 1;
            } else {
                *offsetX = 16;
            }
        }
        if ((s16)changed) {
            Sound_PlayCursorMove();
        }
    }
    if ((s16)changed) {
        Display_SetOffset(*offsetX, *offsetY);
    }
    gOptWork->screenPosMoved = moved;
}

/* Draws the four corner marks of the screen position page. */
void Option_DrawCorners(void)
{
    Sprite corner;
    Sprite cornerRotated;
    s32 i;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 4);
    Tex_Bind(4);
    Gs_SetState(RS_ZWRITE, 0);
    corner.w = 96;
    corner.h = 10;
    corner.color = -1;
    corner.u0 = 0;
    corner.v0 = 240;
    corner.u1 = 47;
    corner.v1 = 247;
    cornerRotated.w = 96;
    cornerRotated.h = 10;
    cornerRotated.color = -1;
    cornerRotated.u0 = 0;
    cornerRotated.v0 = 248;
    cornerRotated.u1 = 47;
    cornerRotated.v1 = 255;
    for (i = 0; i < 4; i++) {
        corner.x = gOptCornerPosA[i][0];
        corner.y = gOptCornerPosA[i][1];
        Sprite_Draw(&corner);
        cornerRotated.x = gOptCornerPosB[i][0];
        cornerRotated.y = gOptCornerPosB[i][1];
        TexRect_DrawRotated((struct TexRect *)&cornerRotated, gOptCornerAngles[i][0]);
    }
    Gs_SetState(RS_ZWRITE, 1);
}

const char gOptNumberFormat[3] = {
#include "select/006CDFA0.inc"
};

extern char *gScreenPosTexts[];     /* messages (select_psp.c) */
extern char *gBackResetTexts[];
s32 sprintf(char *, const char *, ...);    /* sprintf */
s32 strlen(char *);                         /* strlen */
void Hud_DrawButtonIconWhite(s32, s16, s16, s32, f32); /* unit's own declaration: s16 x and y, though the definition (sub_main) takes s32; the page drawers' code needs them */
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
/* The screen position page: the offsets, the frame and corner marks, and
 * four arrows, lit while their direction moves. */
void Option_DrawScreenPosPage(void)
{
    Sprite spr;
    char buf[128];
    u16 *rec;
    s32 i;

    Sel_DrawBackground(255);
    Option_DrawFrame();
    Option_DrawCorners();
    Sel_DrawHeader(3);
    i = 160;
    Font_SetSpacing(18, 18);
    Text_DrawShadow(184, 304, 1, 0, gScreenPosTexts[0]);
    Text_DrawShadow(184, 340, 1, 0, gScreenPosTexts[1]);
    Hud_DrawButtonIconWhite(21, i, 389, 8, 1.0f);
    i += 24;
    Text_DrawShadow(i, 392, 1, 0, gBackResetTexts[0]);
    i += strlen(gBackResetTexts[0]) * 9 + 90;
    Hud_DrawButtonIconWhite(3, i, 389, 8, 1.0f);
    Text_DrawShadow(i + 24, 392, 1, 0, gBackResetTexts[1]);
    Font_SetSpacing(36, 18);
    sprintf(buf, gOptNumberFormat, gSysSave.options.screenX);
    Text_DrawShadow(418, 304, 1, 0, buf);
    sprintf(buf, gOptNumberFormat, gSysSave.options.screenY);
    Text_DrawShadow(418, 340, 1, 0, buf);
    Tex_Prepare(1, 8);
    Tex_Bind(8);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    spr.w = 64;
    spr.h = 64;
    spr.u0 = 0;
    spr.v0 = 160;
    spr.u1 = spr.u0 + 63;
    spr.v1 = spr.v0 + 63;
    rec = (u16 *)gOptScreenPosRects;
    for (i = 0; i < 4; i++, rec += 2) {
        spr.x = *rec++;
        spr.y = *rec++;
        if (gOptWork->screenPosMoved & rec[0]) {
            spr.color = 0xFF00FFFF;
        } else {
            spr.color = -1;
        }
        TexRect_DrawRotated((struct TexRect *)&spr, rec[1]);
    }
}

/* The brightness page's input: left/right step the brightness (128 is the
 * middle; 1 per step above it, 2 below, within 57..163), 0x200 resets
 * it, cancel keeps it and goes back. The game's "changed" test compares
 * the old offset with the same value recomputed, so the cursor sound never
 * plays. level is an s32 and the pointers are cast as the match needs. */
void Option_BrightnessInput(u16 trig, u16 rep)
{
    s32 level;
    u8 *savedPtr;
    u8 *levelPtr;
    s32 old;

    savedPtr = (u8 *)&gSysSave.options.brightness;
    levelPtr = (u8 *)&gSysSettings.brightness;
    level = *levelPtr;
    if (rep & 0x800) {
        old = level - 128;
        if (old == 0) {
            *levelPtr = level - 2;
        } else if (old > 0) {
            if (level - 1 > 56) {
                *levelPtr = level - 1;
            }
        } else {
            if (level - 2 > 56) {
                *levelPtr = level - 2;
            }
        }
        if (old != level - 128) {
            Sound_PlayCursorMove();
        }
        return;
    }
    if (rep & 0x400) {
        old = level - 128;
        if (old == 0) {
            *levelPtr = level + 1;
        } else if (old > 0) {
            if (level + 1 < 164) {
                *levelPtr = level + 1;
            }
        } else {
            if (level + 2 < 164) {
                *levelPtr = level + 2;
            }
        }
        if (old != level - 128) {
            Sound_PlayCursorMove();
        }
        return;
    }
    if (trig & 0x40) {
        *savedPtr = level;
        Option_OpenMainPage();
        Sound_PlayCancel();
        return;
    }
    if (trig & 0x200) {
        Sound_PlayConfirm();
        *levelPtr = 128;
    }
}

extern char *gBackResetTexts[];     /* messages (select_psp.c) */
extern char *gOptBrightnessTexts[];
s32 Str_Length(char *);                         /* a message's width */

/*
 * The brightness page: the brightness strip, the offset from the middle
 * (halved below it), the help lines and the button help.
 */
void Option_DrawBrightnessPage(void)
{
    char buf[256];
    s32 offset;
    s32 i;
    s32 y;
    char **line;
    s32 width;
    s32 x;

    Sel_DrawBackground(255);
    Option_DrawFrame();
    Sel_DrawHeader(9);
    Option_DrawBrightnessStrip();
    offset = gSysSettings.brightness - 128;
    if (offset == 0) {
        sprintf(buf, " %d", offset);
    } else if (offset > 0) {
        sprintf(buf, "+%d", offset);
    } else {
        sprintf(buf, gOptNumberFormat, offset / 2);
    }
    Font_SetPal(0);
    Option_DrawText(524, 344, buf, 20, 0x80808080);
    line = gOptBrightnessTexts;
    y = 240;
    for (i = 0; i < 3; i++, line++) {
        width = Str_Length(*line);
        if (width != 0) {
            Option_DrawText((640.0f - 10.0f * width) / 2.0f, y, *line, 20, 0x80808080);
            y += 24;
        }
    }
    x = 160;
    Font_SetSpacing(18, 18);
    Hud_DrawButtonIconWhite(21, x, 389, 8, 1.0f);
    x += 24;
    Text_DrawShadow(x, 392, 1, 0, gBackResetTexts[0]);
    x += strlen(gBackResetTexts[0]) * 9 + 90;
    Hud_DrawButtonIconWhite(3, x, 389, 8, 1.0f);
    Text_DrawShadow(x + 24, 392, 1, 0, gBackResetTexts[1]);
}

/*
 * 1 when a setting of gSysSave.options differs from its default in opts (a
 * -1 there is not compared). The loop is written with a goto, as the game's
 * code has it.
 */
s32 Option_DiffersFrom(s8 *opts)
{
    s32 i;
    s8 *def;
    s8 *cur;
    s8 value;

    def = opts;
    i = 0;
    cur = &gSysSave.options.soundMode;
loop_1:
    value = *def;
    if ((value != -1) && (value != *cur)) {
        return 1;
    }
    i += 1;
    def += 1;
    cur += 1;
    if (i >= 16) {
        return 0;
    }
    goto loop_1;
}

/*
 * The options' frame: the fades between the pages, the current page's input
 * and drawing; also notes which settings differ from the defaults and from
 * those at the start.
 */
void Option_Update(TaskSlot *task)
{
    u16 rep;
    u16 trig;
    s32 i;
    s8 *saved;
    s8 *current;

    trig = gPadState[0].pressed;
    rep = gPadState[0].repeat | trig;
    switch (gOptWork->state) {
    case 0:
        FadeA_Start(12);
        gOptWork->state++;
    case 1:
        if ((u8)FadeA_GetState() != 1) {
            gOptWork->state++;
        case 2:
            switch (gOptWork->page) {
            case 0:
                if (Option_MainInput(trig, rep) == 1) {
                    task->mode = 2;
                }
                break;
            case 1:
                Option_ControlInput(trig, rep);
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
        FadeA_Start(11);
        gOptWork->state++;
    case 4:
        if ((u8)FadeA_GetState() != 1) {
            gOptWork->page = gOptWork->nextPage;
            gOptWork->state = 0;
        }
        break;
    }
    gOptWork->mainDiffers = Option_DiffersFrom((s8 *)gOptMainDefaults);
    gOptWork->controlsDiffer = Option_DiffersFrom((s8 *)gOptControlDefaults);
    current = &gSysSave.options.soundMode;
    i = 0;
    saved = gOptWork->savedOptions;
    gOptWork->changed = 0;
    for (; i < 16; i++, saved++, current++) {
        if (*saved != *current) {
            gOptWork->changed = 1;
            break;
        }
    }
    switch (gOptWork->page) {
    case 0:
        Option_DrawMainPage();
        break;
    case 1:
        Option_DrawControlPage();
        break;
    case 2:
        Option_DrawScreenPosPage();
        break;
    case 3:
        Option_DrawBrightnessPage();
        break;
    }
}

/*
 * Starts the options (net: the network options instead): the music for the
 * plain options, the work at 0x100 in gSysSettings.selWork, the settings at
 * the start kept, and the layout of the rows.
 */
void Option_Start(TaskSlot *task, u16 net)
{
    if ((u8)FadeA_GetState() != 1) {
        task->unk14 = 0;
        task->unk15 = 0;
        task->mode++;
        if (net == 0) {
            Stream_Play(0, 201);
        }
        Gs_SetTableColor(0);
        gOptWork = (OptWork *)(gSysSettings.selWork + 256);
        memset(gOptWork, 0, 176);
        memcpy(gOptWork->savedOptions, &gSysSave.options, 16);
        memcpy(gOptWork->savedNetOptions, gSysSave.netOptions, 16);
        gOptWork->textSize = 18;
        gOptWork->textSize2 = 18;
        if (net == 0) {
            gOptWork->rowX = 60;
            gOptWork->rowY = 70;
            gOptWork->rowStep = 30;
            gOptWork->selScale = 1.35f;
            gOptWork->helpX = 50;
            gOptWork->helpY = 368;
            gOptWork->valueX = 290;
        } else {
            gOptWork->rowX = 60;
            gOptWork->rowY = 90;
            gOptWork->rowStep = 30;
            gOptWork->selScale = 1.35f;
            gOptWork->helpX = 50;
            gOptWork->helpY = 350;
            gOptWork->valueX = 320;
        }
        gOptWork->textColor = 0x80808080;
        gOptWork->selColor = 0x80784100;
        gOptWork->defaultsColor = 0x802B802B;
        gOptWork->dimColor = 0x80404040;
    }
}

/* unit's own declaration: declared s32 as NetOption_Update uses it; with the definition's u8 return, the call compiles differently */
s32 NetMenu_Update(void);

/*
 * Leaves the options: saves the settings to the memory card first (through
 * the network menu's save) when they changed, fades out and starts gm_sub.
 */
void Option_Exit(void *task, s32 net)
{
    switch (gOptWork->step) {
    case 0:
        Stream_Reset();
        if (gOptWork->changed == 0) {
            FadeA_Start(11);
            gOptWork->step = 2;
            gOptWork->timer = 0;
        } else {
            gOptWork->step++;
            NetMenu_Open(NETMENU_MODE_SAVE_SYSTEM);
        }
        break;
    case 1:
        if ((u8)NetMenu_Update() != 0) {
            gOptWork->step++;
            FadeA_Start(11);
        }
        break;
    case 2:
        if ((u8)FadeA_GetState() != 1) {
            gOptWork->step++;
            gOptWork->timer = 0;
            SysSettings_Reset();
            Stream_Reset();
            Vib_StopAll();
            CamReq_ResetStub();
        }
        break;
    case 3:
        if (++gOptWork->timer >= 10) {
            gOptWork = NULL;
            TaskSlot_End(task);
            Task_StartGmSub();
            FadeA_Start(12);
            return;
        }
        break;
    }
    if ((u16)net == 0) {
        Option_DrawMainPage();
    } else {
        NetOption_DrawPage();
    }
}

/* Fades to the network options' main page. */
void NetOption_OpenMainPage(void)
{
    gOptWork->nextPage = 0;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

/* Fades to the network menu's page. */
void NetOption_OpenNetMenuPage(void)
{
    gOptWork->nextPage = 1;
    gOptWork->subCursor = 1;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

/* Fades to the confirmation before the network setup guide. */
void NetOption_OpenConfirmPage(void)
{
    gOptWork->nextPage = 2;
    gOptWork->subCursor = 1;
    gOptWork->state = 3;
    gOptWork->step = 0;
}

/* The network options' task: starts them, runs the pages, then leaves. */
void NetOption_Task(TaskSlot *task)
{
    u8 step;

    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    step = task->mode;
    switch (step) {
    case 0:
        Option_Start(task, 1);
        break;
    case 1:
        NetOption_Update(task, 1);
        break;
    case 2:
        Option_Exit(task, 1);
        break;
    }
    Frame_Draw();
}

/*
 * 1 when a network setting differs from its default in opts (a -1 there is
 * not compared). The loop is written with a goto, as the game's code has it.
 */
s32 NetOption_DiffersFrom(s8 *opts)
{
    s32 i;
    s8 *def;
    s8 *cur;
    s8 value;

    def = opts;
    i = 0;
    cur = (s8 *)gSysSave.netOptions;
loop_1:
    value = *def;
    if ((value != -1) && (value != *cur)) {
        return 1;
    }
    i += 1;
    def += 1;
    cur += 1;
    if (i >= 16) {
        return 0;
    }
    goto loop_1;
}

/*
 * The network options' frame: the fades between the pages; the main page's
 * input (rows 2 and 3 open the network menu's page and the setup guide's
 * confirmation; leaving notes whether anything changed); the network menu
 * run as a page; the confirmation.
 */
void NetOption_Update(TaskSlot *task, s32 unused)
{
    u16 rep;
    u16 trig;
    s32 i;
    s8 *saved;
    s8 *current;

    trig = gPadState[0].pressed;
    rep = gPadState[0].repeat | trig;
    switch (gOptWork->state) {
    case 0:
        FadeA_Start(12);
        gOptWork->state++;
    case 1:
        if ((u8)FadeA_GetState() != 1) {
            gOptWork->state++;
        case 2:
            switch (gOptWork->page) {
            case 0:
                gOptWork->netDiffers = NetOption_DiffersFrom((s8 *)gNetOptDefaults);
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
                    task->mode = 2;
                    current = (s8 *)gSysSave.netOptions;
                    i = 0;
                    saved = gOptWork->savedNetOptions;
                    gOptWork->changed = 0;
                    for (; i < 16; i++, saved++, current++) {
                        if (*saved != *current) {
                            gOptWork->changed = 1;
                            break;
                        }
                    }
                    break;
                }
                break;
            case 1:
                switch (gOptWork->step) {
                case 0:
                    NetMenu_Open(NETMENU_MODE_DELETE_LOGIN_ID);
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
        FadeA_Start(11);
        gOptWork->state++;
    case 4:
        if ((u8)FadeA_GetState() != 1) {
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
        NetOption_DrawNetMenuPage();
        break;
    case 2:
        NetOption_DrawConfirm();
        break;
    }
}

/* gSelectFmtString, a format that prints one string, sits here in the game's
 * .rodata, before NetOption_Input's jump table, though only
 * NetOption_DrawConfirm uses it. */
INCLUDE_RODATA("asm/select/nonmatchings/select_option", gSelectFmtString);

/*
 * The network options' input: two settings that wrap, two rows that open
 * pages, the defaults and the exit. Returns 1 or 2 for the pages, -1 when
 * the page is left, else 0. The goto shares the value code, as the game's
 * code does.
 */
s32 NetOption_Input(u16 trig, u16 rep)
{
    s8 count;
    s8 value;
    s8 *setting;
    s32 cursor;

    if (rep & 0x2000) {
        do {
            gOptWork->cursor--;
            if (gOptWork->cursor < 0) {
                gOptWork->cursor = 5;
            }
        } while (gOptWork->cursor == 4 && gOptWork->netDiffers == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return 0;
    }
    if (rep & 0x1000) {
        do {
            gOptWork->cursor++;
            if (gOptWork->cursor >= 6) {
                gOptWork->cursor = 0;
            }
        } while (gOptWork->cursor == 4 && gOptWork->netDiffers == 0);
        Option_ResetArrows();
        Sound_PlayCursorMove();
        return 0;
    }
    if (trig & 0x40) {
        Sound_PlayCancel();
        if (gOptWork->cursor == 5) {
            return -1;
        }
        gOptWork->cursor = 5;
        return 0;
    }
    count = ((s8 *)gNetOptValueCounts)[gOptWork->cursor];
    switch (cursor = gOptWork->cursor) {
    case 0:
    case 1:
        setting = (s8 *)&gSysSave.netOptions[cursor];
        value = *setting;
        if (rep & 0x400) {
            value++;
            value %= count;
            Option_BounceRightArrow();
        set:
            *setting = value;
            Sound_PlayCursorMove();
            break;
        }
        if (rep & 0x800) {
            value--;
            if (value < 0) {
                value += count;
            }
            Option_BounceLeftArrow();
            goto set;
        }
        break;
    case 2:
        if (trig & 0x20) {
            Sound_PlayConfirm();
            return 1;
        }
        break;
    case 3:
        if (trig & 0x20) {
            Sound_PlayConfirm();
            return 2;
        }
        break;
    case 4:
        if (trig & 0x20) {
            Sound_PlayConfirm();
            Option_NetDefaults();
        }
        break;
    case 5:
        if (trig & 0x20) {
            Sound_PlayConfirm();
            return -1;
        }
        break;
    }
    Option_UpdateArrows();
    return 0;
}

extern char *gNetOptLabels[];     /* messages (select_psp.c) */
extern char *gNetOptHelp[];

/*
 * The network options' page: one row per setting with its value and
 * arrows, the row under the cursor enlarged, then the help line. The values
 * are read at netSetting[16], 16 bytes past gSysSave.options: the
 * network settings, read this way in the game's code.
 */
void NetOption_DrawPage(void)
{
    s32 width;
    u32 rowColor;
    s32 rowSize;
    s32 rowY;
    s32 x;
    char **label;
    s32 i;
    s32 y;
    u32 color;
    s16 (*arrow)[2];
    char **valueText;
    s32 size;
    char ***valueTexts;
    u8 *netSetting;
    s32 valueX;

    Sel_DrawBackground(255);
    Sel_DrawHeader(7);
    WindowFrame_Draw(&gNetOptPageFrame, 0);
    label = gNetOptLabels;
    i = 0;
    valueTexts = gNetOptValueNames;
    netSetting = (u8 *)&gSysSave.options;
    arrow = (s16 (*)[2])gNetOptArrows;
    x = gOptWork->rowX;
    y = gOptWork->rowY;
    for (; i < 6; i++, valueTexts++, netSetting++, arrow++, y += gOptWork->rowStep) {
        gOptWork->arrowsShown = 0;
        if (i == 5) {
            y += (s32)(0.5f * gOptWork->rowStep);
        }
        rowY = y;
        rowSize = gOptWork->textSize;
        if (i == gOptWork->cursor) {
            rowSize = rowSize * gOptWork->selScale;
            rowY = y - (rowSize - gOptWork->textSize) / 2;
            rowColor = gOptWork->selColor;
        } else if (i == 4) {
            if (gOptWork->netDiffers != 0) {
                rowColor = gOptWork->defaultsColor;
            } else {
                rowColor = gOptWork->dimColor;
            }
        } else {
            rowColor = gOptWork->textColor;
        }
        if (rowColor != color) {
            color = rowColor;
        }
        Option_DrawText(x, rowY, *label, rowSize, color);
        size = gOptWork->textSize;
        switch (i) {
        case 0:
        case 1:
            if (i == gOptWork->cursor) {
                gOptWork->arrowsShown = 1;
                if (i == 0) {
                    gOptWork->arrowY = y + gOptWork->rowStep / 2 - 6;
                } else {
                    gOptWork->arrowY = y - 6;
                }
            }
            valueText = &(*valueTexts)[netSetting[16]];
            width = Str_Length(*valueText);
            valueX = gOptWork->valueX + (s32)((320.0f - (size / 2) * width) / 2.0f);
            Font_SetSpacing(size, size);
            if (i == 0) {
                Option_DrawText(valueX, y + gOptWork->rowStep / 2, *valueText, size, color);
            } else {
                Option_DrawText(valueX, y, *valueText, size, color);
            }
            if (gOptWork->arrowsShown != 0) {
                gOptWork->arrowLeftX = (*arrow)[0];
                gOptWork->arrowRightX = (*arrow)[1];
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
            y += (s32)(gOptWork->rowStep * (1.0f / gOptWork->selScale));
        }
    }
    Option_DrawText(gOptWork->helpX, gOptWork->helpY,
                  gNetOptHelp[gOptWork->cursor], gOptWork->textSize,
                  gOptWork->textColor);
}

void Yn_LaunchSetupGuide(void); /* yn: the overlay's entry */

/*
 * The confirmation before the network setup guide: left and right choose
 * yes or no; yes waits 20 frames, then loads the yn overlay (9) and starts
 * it; no, or cancel on no, goes back.
 */
void NetOption_ConfirmInput(u16 pad, u16 rep)
{
    u16 buttons;

    switch (gOptWork->step) {
    case 0:
        gOptWork->subCursor = 1;
        gOptWork->step++;
        gOptWork->confirmStep = 0;
        break;
    case 1:
        switch (gOptWork->confirmStep) {
        case 0:
            buttons = pad;
            if ((buttons & 0x400) || (buttons & 0x800)) {
                Sound_PlayCursorMove();
                gOptWork->subCursor ^= 1;
            } else if (buttons & 0x20) {
                Sound_PlayConfirm();
                if (gOptWork->subCursor == 0) {
                    gOptWork->confirmStep++;
                    gOptWork->timer = 20;
                } else {
                    NetOption_OpenMainPage();
                }
            } else if (buttons & 0x40) {
                Sound_PlayCancel();
                if (gOptWork->subCursor == 0) {
                    gOptWork->subCursor = 1;
                } else {
                    NetOption_OpenMainPage();
                }
            }
            break;
        case 1:
            gOptWork->timer--;
            if (gOptWork->timer == 0) {
                LoadOverlay(9);
                Yn_LaunchSetupGuide();
            }
            break;
        }
        break;
    }
}

/* Empty: the network menu draws itself. */
void NetOption_DrawNetMenuPage(void)
{

}

extern char *gNetOptConfirmTexts[];      /* the network options' notice (select_psp.c) */
extern char gSelectFmtString[];
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
struct Window;
void Window_DrawTextAlpha(struct Window *w, u8 *text, u8 alpha);

/*
 * Draws the confirmation: the notice's lines and the two choices, the
 * chosen one on a glowing bar.
 */
void NetOption_DrawConfirm(void)
{
    s32 y;
    s32 i;
    char **line;
    s32 x;
    char **choice;
    s32 j;
    s32 color;
    s32 lineX;

    Sel_DrawBackground(255);
    Sel_DrawHeader(7);
    Window_DrawTextAlpha((struct Window *) gNetOptConfirmWin, 0, 128);
    Font_SetSpacing(18, 18);
    line = gNetOptConfirmTexts;
    y = 120;
    Text_SetColor(0);
    for (i = 0; i < 6; i++) {
        if (**line != ' ') {
            lineX = Str_Length(*line);
            lineX = (640 - lineX * 9) / 2;
            Font_SetPos(lineX, y);
            Text_PrintfColored(gSelectFmtString, *line);
        }
        y += 24;
        line++;
    }
    for (j = 0, choice = gPouchMenuPanel2_6; j < 2; j++, choice++) {
        color = (gOptWork->subCursor == j) ? 5 : 0;
        x = 240;
        if (j == 0) {
            x += 20;
        } else {
            x += 130;
        }
        Text_SetColor(color);
        if (j == 0) {
            Font_SetPos(x - 20, 324);
        } else {
            Font_SetPos(x - 30, 324);
        }
        Text_PrintfColored(gSelectFmtString, *choice);
        if (gOptWork->subCursor == j) {
            Menu_DrawGlowBar(x, 322, 120, 24, 0xFF20C0C0);
        }
    }
    Font_SetSpacing(18, 18);
}

/*
 * Draws the strip of grey steps the brightness is judged by (system texture
 * 5).
 */
void Option_DrawBrightnessStrip(void)
{
    Sprite spr;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 5);
    Tex_Bind(5);
    Gs_SetState(RS_ZWRITE, 0);
    spr.x = 0;
    spr.y = 300;
    spr.w = 640;
    spr.h = 80;
    spr.color = -1;
    spr.u0 = 0;
    spr.v0 = 0;
    spr.u1 = 511;
    spr.v1 = 63;
    Sprite_Draw(&spr);
    Gs_SetState(RS_ZWRITE, 1);
}
