/*
 * The render-state shortcuts and the text layer (sub_main 0x368F10-0x36B5B0):
 * - Gfx_SetBlend, Gfx_SetTexFilter, Tex_Bind and Gfx_SetBlendOp set a render
 *   state (render_state.h) from a table by index, and skip it when the index
 *   cached in gSysSettings hasn't changed; Gfx_ResetRenderStates sets the
 *   defaults;
 * - the text layer over the font library of unk_2E3940.c (Font_Printf is
 *   its printf): printf-style drawing, the '~' control codes (text.h:
 *   colours, inserted numbers, icons queued per layer in gTextIcons, hidden
 *   parts) read by Text_ParseCode and drawn by Text_PrintCodes, text with a
 *   drop shadow, and Shift-JIS aware scanning (Text_NextChar);
 * - the pads' key repeat, applying the options, and the reset of the game's
 *   subsystems (Game_ResetSubsystems) with the motion table's frees.
 */
#include "common.h"
#include "model.h"
#include "sprite.h"
#include "syssettings.h"
#include "syssave.h"

#include "ee_stdarg.h"
#include "render_state.h"
#include "overlay.h"
#include "text.h"
#include "net_menu.h"
#include "motion.h"
#include "pad.h"

/* Key repeat state of a pad (Pad_KeyRepeat). */
typedef struct KeyRepeat {
    s16 repeating;      /* 0: first delay, 1: repeating */
    s16 timer;
} KeyRepeat;

extern s16 gTexPrepFrameCount;
extern s16 gTexPrepLastCount;
extern s16 gTexPrepLast;
extern s32 gDrawFrameDepth;
extern u32 gBlendModes[];
extern u32 gBlendModes2[];
extern u32 gTexFilterModes[];
extern u32 gBlendOps[];
extern u32 gTextCodeColors[];
extern u32 gTextColors[];       /* text colours TEXT_COLOR_RGB.. */
extern u32 gTexHandles[];
extern s32 gTextOff;          /* nonzero: text drawing is off */
extern s16 gTextIconCount[TEXT_LAYERS];
extern Icon gTextIcons[TEXT_LAYERS][TEXT_LAYER_ICONS];
extern KeyRepeat gKeyRepeat[2];
extern s16 gKeyRepeatDelay[2];
extern u8 gMemFileBuf[];
extern void *gObjBufBlocks;
extern void *gMatSets;
extern void *gMaterials;
extern void *gBones;
extern void *gModels;
extern void *gModelLoadBuf;
extern void *gPrims;
extern void *gWorkArea;
extern u8 gAreaSubModel[];
extern u8 gFieldControl;
extern u8 gScreenCaptureOk;
extern u8 gSoundLoading;
extern u8 gSysBusy;
extern void *gMotions[];

void *memset(void *p, s32 c, u32 n);
s32 Packet_SetMode1Ok(void);
s32 Packet_SetMode2Ok(void);
s32 Gs_SetState(u8 reg, u32 v);
s32 Overlay_IsLoaded(s32 a);
void sprintf(char *buf, const char *fmt, ...);
void vsnprintf(char *buf, s32 size, const char *fmt, va_list ap);
void ScreenTex_Release(void);
void Mem_Compact(void);
void Display_SetOffset(s32 a, s32 b);
void FileQueue_Cancel(void);
void Stream_Reset(void);
void Stream_UpdateMaxVol(s32 a);
void Sound_ApplyOutputMode(s32 mode);
void Sound_Silence(void);
void Gs_SetTableColor(s32 a);
void Font_Init(s32 unused0);
void TextObj_Reset(void);
void Font_ClearPage(s32 layer);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_SetSize(f32 scale);
void Font_SetColor(s32 color);
void Font_SetColorIndex(s16 color);
void Font_SetPal(s32 a);
s32 Font_GetPage(void);
s16 Font_GetAdvance(void);
s32 Font_GetPal(void);
u32 Font_GetColor(void);
s16 Font_GetX(void);
s16 Font_GetY(void);
char *strchr(char *s, s32 c);
char *strcpy(char *dst, char *src);
s16 Font_GetCharH(void);
s32 Font_GetHalfWidth(void);
s32 Font_IsLineGapOn(void);
/* unit's own declaration: s16 (the definition, unk_2E3940.c, returns s32: (s16) of a u8, 0-255); with s32, in every form tried, Text_PrintCodes sign-extends the value before y += (2 more words) */
s16 Font_GetLineGap(void);
void strncat(char *dst, u8 *src, s32 n);
void Font_SetLineGapOn(u8 on);
void Font_SetLineGap(s32 lineGap);
s32 Str_Length(char *s);
void Font_Printf(char *fmt, ...);
void Font_DrawPage(s32 layer);
void Font_DrawAllPages(void);
void Sjis_Widen(char *src, char *dst);
s32 Mem_FreeRet1(void *p);
void FreeList_Init(s32 n, u32 *p);
void FadeA_Reset(void);
void Area_ResetStub(void);
void Light_Reset(void);
void Tex_ReleaseAll(void);
void Tex_LoadFile(s32 a, s32 b, s32 c);
void Model_ResetAll(void);
void Model_FreeAll(void);
void EmSlotModel_Clear(s16 a, s8 b);
struct Icon;
void Draw_Icons(struct Icon *boxes, u8 count);
void Obj_InitAll(void);
void UnreadFlags_ClearAll(void);
void Callbacks_Clear(void);
void EquipPreview_Clear(void);
void DrawSlot_ResetAll(void);
void DrawSlotB_ResetAll(void);
void Notice_ResetQueue(void);
void Player_ClearAll(void);
s32 LoadCompressedFile(u8 *dst, s32 id);
void Movie_Stop(void);
void Camera_Init(void);
void Camera_SetPreset(s32 id);
void EmSlot_ClearAll(void);
void Area_ClearState(void);
void Vib_StopAll(void);
void Sound_ReverbOn(void);
s32 Sound_SetSpuMapPage(u32 page);
char *Text_ParseCode(char *s, s16 *code, s16 mode);
void Text_Printf(const char *fmt, ...);
void Text_PrintCodes(s16 max, const char *fmt, ...);
void Text_SetColor(s16 color);
void Text_DrawColored(char *s);
void Motion_FreeAll(void);
s32 Text_CountChars(char *s);
void Text_FormatVar(void *dst, s16 kind);

/* Starts a frame's drawing: packets may be built, the texture preparation
 * counters restart, and the nesting depth goes up. */
void Draw_BeginFrame(void)
{
    Packet_SetMode1Ok();
    gTexPrepFrameCount = 0;
    gTexPrepLast = -1;
    gTexPrepLastCount = -1;
    gDrawFrameDepth++;
}

void Draw_EndFrame(void)
{
    Packet_SetMode2Ok();
    gDrawFrameDepth--;
}

/* Sets RS_BLEND to the two tables' entries ORed (gBlendModes[mode],
 * gBlendModes2[mode2]), when either index changes. */
void Gfx_SetBlend(s32 modeA, s32 modeB)
{
    if (gSysSettings.blendA != modeA || gSysSettings.blendB != modeB) {
        gSysSettings.blendA = modeA;
        gSysSettings.blendB = modeB;
        Gs_SetState(RS_BLEND, gBlendModes[modeA] | gBlendModes2[modeB]);
    }
}

/* Sets RS_TEX_FILTER from gTexFilterModes (0 filtered, 1 not), when the
 * index changes. */
void Gfx_SetTexFilter(s32 mode)
{
    if (gSysSettings.texFilter != mode) {
        gSysSettings.texFilter = mode;
        Gs_SetState(RS_TEX_FILTER, gTexFilterModes[mode]);
    }
}

/* Binds texture slot `slot` (gTexHandles) and remembers it. */
void Tex_Bind(s32 slot)
{
    gSysSettings.boundTex = slot;
    Gs_SetState(RS_TEXTURE, gTexHandles[slot]);
}

/* Default render states (the fog ones only when partial is 0) and the
 * cached values of Gfx_SetBlend, Gfx_SetTexFilter, Tex_Bind and
 * Gfx_SetBlendOp. */
void Gfx_ResetRenderStates(s32 partial)
{
    f32 fogNear = 20000.0f;
    f32 fogFar = 30000.0f;

    if (partial == 0) {
        Gs_SetState(RS_COLOR, 0);
        Gs_SetState(1, 0x200);
        Gs_SetState(0x15, 2);
        Gs_SetState(RS_FOG_COLOR, 0x80FFFFFF);
        Gs_SetState(RS_FOG_NEAR, *(u32 *)&fogNear);
        Gs_SetState(RS_FOG_FAR, *(u32 *)&fogFar);
        Gs_SetState(RS_ALPHA_TEST, RS_CMP_GREATER);
    }
    gSysSettings.blendOp = 0;
    gSysSettings.blendA = 4;
    gSysSettings.blendB = 5;
    gSysSettings.texFilter = 1;
    gSysSettings.unk3D = 0x80;
    gSysSettings.boundTex = 0xFFFF; /* none */
    gSysSettings.unk3E = 1;
    Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    Gs_SetState(RS_TEX_FILTER, 0);
    Gs_SetState(0xD, 0);
    Gs_SetState(RS_ALPHA_REF, 0);
}

/* Sets render state 0xD (which of Gs_SetAlpha's tables RS_BLEND reads)
 * from gBlendOps, when the index changes. */
void Gfx_SetBlendOp(s32 opIndex)
{
    if (gSysSettings.blendOp != opIndex) {
        gSysSettings.blendOp = opIndex;
        Gs_SetState(0xD, gBlendOps[opIndex]);
    }
}

/* Empties the draw lists: the sorted ordering tables (64, 32, 16 and 8
 * entries) and the single lists of the stage, the sky and the text. */
void FreeList_InitAll(void)
{
    FreeList_Init(0x40, gDrawOtSorted);
    FreeList_Init(0x20, gDrawOt32);
    FreeList_Init(0x10, gDrawOtText3);
    FreeList_Init(8, gDrawOt8);
    FreeList_Init(1, gDrawListStage);
    FreeList_Init(1, gDrawListText0b);
    FreeList_Init(1, gDrawListText1);
    FreeList_Init(1, gDrawListText2);
    FreeList_Init(1, gDrawListText4);
    FreeList_Init(1, gDrawListText5);
    FreeList_Init(1, gDrawListSky);
    FreeList_Init(1, gDrawListText6);
    FreeList_Init(1, gDrawListText0a);
}

void Text_Init(void)
{
    Font_Init(0);
}

/* Turns text drawing back on and empties every layer's icon queue. */
void Text_Reset(void)
{
    TextObj_Reset();
    gTextOff = 0;
    gTextIconCount[0] = 0;
    gTextIconCount[1] = 0;
    gTextIconCount[2] = 0;
    gTextIconCount[3] = 0;
    gTextIconCount[4] = 0;
    gTextIconCount[5] = 0;
    gTextIconCount[6] = 0;
}

/* Draws the font's pages up to the current one, then clears the current
 * one. */
void Text_FlushPages(void)
{
    s32 page = Font_GetPage();
    s32 i;

    for (i = 0; i <= page; i++) {
        Font_DrawPage(i);
    }
    Font_ClearPage(page);
}

/* Draws every page, then turns text drawing off (gTextOff). */
void Text_DrawAllAndStop(void)
{
    s16 i;

    for (i = 0; i < TEXT_LAYERS; i++) {
        Font_DrawPage(i);
    }
    gTextOff = 1;
}

void Text_DrawPage(s32 layer)
{
    Font_DrawPage(layer);
}

void Text_DrawCurrentPage(void)
{
    Font_DrawPage(Font_GetPage());
}

/* Draws the icons queued on a layer (Text_AddIcon). */
void Text_DrawIcons(s32 layer)
{
    if (gTextIconCount[layer] != 0) {
        Draw_Icons(gTextIcons[layer], gTextIconCount[layer]);
    }
}

/* printf to the screen (control codes allowed; Text_DrawColored). */
void Text_PrintfColored(const char *fmt, ...)
{
    char buf[TEXT_BUF_SIZE];
    va_list args;

    if (gTextOff == 0) {
        va_start(args, fmt);
        vsnprintf(buf, TEXT_BUF_SIZE - 1, fmt, args);
        buf[TEXT_BUF_SIZE - 1] = 0;
        Text_DrawColored(buf);
    }
}

/* printf at (x, y) in colour index color. */
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...)
{
    char buf[TEXT_BUF_SIZE];
    va_list args;

    if (gTextOff == 0) {
        Font_SetPos(x, y);
        Text_SetColor(color);
        va_start(args, fmt);
        vsnprintf(buf, TEXT_BUF_SIZE, fmt, args);
        Font_Printf("%s", buf);
    }
}

/* Draws a list of strings (ended by NULL), one per line of dy. */
void Text_DrawLines(s16 x, s16 y, char **lines, s16 dy)
{
    while (*lines != NULL) {
        Font_SetPos(x, y);
        Text_Printf(*lines);
        lines++;
        y += dy;
    }
}

/* Draws a list of strings (ended by NULL) with colour codes, one per line
 * of dy. */
void Text_DrawLinesColored(s16 x, s16 y, char **lines, s16 dy)
{
    while (*lines != NULL) {
        Font_SetPos(x, y);
        Text_PrintfColored(*lines);
        lines++;
        y += dy;
    }
}

/* Writes the number a ~A code inserts (TEXT_VAR_*) to dst, in full-width
 * digits; dst is left alone for another kind. */
void Text_FormatVar(void *dst, s16 kind)
{
    char buf[0x80];

    switch (kind) {
    case TEXT_VAR_CARD_SLOT:
        sprintf(buf, "%d", gCardMenu.slot + 1);
        Sjis_Widen(buf, dst);
        break;
    case TEXT_VAR_CARD_BLOCKS:
        sprintf(buf, "%d", gCardMenu.fileBlocks);
        Sjis_Widen(buf, dst);
        break;
    }
}

/* Queues icon `kind` of the given size at (x, y) on the current text
 * layer, opaque; a layer takes at most TEXT_LAYER_ICONS. */
void Text_AddIcon(s16 x, s16 y, s16 size, s16 kind)
{
    s32 layer = Font_GetPage();
    s16 count = gTextIconCount[layer];

    if (count < TEXT_LAYER_ICONS) {
        gTextIcons[layer][count].x = x;
        gTextIcons[layer][count].y = y;
        gTextIcons[layer][count].size = size;
        gTextIcons[layer][count].kind = kind;
        gTextIcons[layer][count].alpha = 0xFF;
        gTextIconCount[layer]++;
    }
}

/* Reads up to two decimal digits at s into n (n is left alone when there
 * are none). */
#define READ_NUM(s, n)                                  \
    if (*(s) >= '0' && *(s) <= '9') {                   \
        (n) = *(s)++ - '0';                             \
        if (*(s) >= '0' && *(s) <= '9') {               \
            (n) = (s16)(*(s)++ - '0') + (n) * 10;       \
        }                                               \
    }

/*
 * Parses the control code at s ('~' then a letter, text.h) and returns the
 * text after it. Mode TEXT_PARSE_CODE gives the code: TEXT_CODE_COLOR | n
 * for ~Cnn, TEXT_CODE_VAR | n for ~Ann, TEXT_CODE_ICON | n for ~Bnn,
 * TEXT_CODE_HIDE for ~%, and 0 for ~~ or an unknown code. Modes
 * TEXT_PARSE_WIDTH and TEXT_PARSE_CHARS give what the code adds to the
 * string's displayed length and the characters it stands for.
 */
char *Text_ParseCode(char *text, s16 *code, s16 mode)
{
    char buf[0x80];
    s16 num;

    *code = 0;
    text++;
    switch (*text) {
    case '~':
        switch (mode) {
        case 0:
            *code = 0;
            break;
        case 1:
            text++;
            *code = -1;
            break;
        case 2:
            text++;
            *code = 1;
            break;
        }
        break;
    /* n is left unset when no digit follows ~C, ~A or ~B: the game then
     * uses whatever its register held (the code pointer) */
    case 'C':
        text++;
        switch (mode) {
        case TEXT_PARSE_CODE:
            READ_NUM(text, num);
            *code = num | TEXT_CODE_COLOR;
            break;
        case 1:
            text += 2;
            *code = -4;
            break;
        case 2:
            *code = 0;
            text += 2;
            break;
        }
        break;
    case 'A':
        text++;
        READ_NUM(text, num);
        switch (mode) {
        case TEXT_PARSE_CODE:
            *code = num | TEXT_CODE_VAR;
            break;
        case 1:
            Text_FormatVar(buf, num);
            *code = Str_Length(buf) - 4;
            break;
        case 2:
            Text_FormatVar(buf, num);
            *code = Text_CountChars(buf);
            break;
        }
        break;
    case 'B':
        text++;
        switch (mode) {
        case TEXT_PARSE_CODE:
            READ_NUM(text, num);
            *code = num | TEXT_CODE_ICON;
            break;
        case 1:
            text += 2;
            *code = -2;
            break;
        case 2:
            text += 2;
            *code = 1;
            break;
        }
        break;
    case '%':
        text++;
        switch (mode) {
        case TEXT_PARSE_CODE:
            *code = TEXT_CODE_HIDE;
            break;
        case 1:
            *code = -2;
            break;
        case 2:
            *code = 0;
            break;
        }
        break;
    default:
        switch (mode) {
        case 0:
            *code = 0;
            break;
        case 1:
            *code = -1;
            break;
        case 2:
            *code = 0;
            break;
        }
        break;
    }
    return text;
}

/* printf through the text renderer (Text_PrintCodes). */
void Text_Printf(const char *fmt, ...)
{
    char buf[TEXT_BUF_SIZE];
    va_list args;

    if (gTextOff == 0) {
        va_start(args, fmt);
        vsnprintf(buf, TEXT_BUF_SIZE, fmt, args);
        Text_PrintCodes(-1, "%s", buf);
    }
}

/* printf through the text renderer, with lineGap more between lines. */
void Text_PrintfStyled(s32 lineGap, const char *fmt, ...)
{
    char buf[TEXT_BUF_SIZE];
    va_list args;

    if (gTextOff == 0) {
        va_start(args, fmt);
        vsnprintf(buf, TEXT_BUF_SIZE, fmt, args);
        Font_SetLineGapOn(1);
        Font_SetLineGap(lineGap);
        Text_PrintCodes(-1, "%s", buf);
        Font_SetLineGapOn(0);
    }
}

/*
 * The text renderer: prints fmt with the control codes of Text_ParseCode
 * (~Cnn colour, ~Ann inserted number, ~Bnn icon, ~% hides the text until
 * the next ~%), half-width characters, newlines, and at most max characters
 * (-1: no limit). Each run of text between codes goes to Font_Printf; the
 * runs alternate between two buffers so that ~A can splice its number in
 * front of the rest of the string.
 */
void Text_PrintCodes(s16 max, const char *fmt, ...)
{
    u8 *codeStart;
    s32 codeValue;
    s16 bufIndex;
    u8 *src;
    s16 count;
    s16 len;
    s16 x;
    s16 y;
    s16 lineX;
    s16 skip;
    s16 charKind;
    s16 advance;
    s16 code;
    u16 color;
    u8 **out;
    u8 *after;
    u8 byte;
    va_list args;
    u8 *bufs[2] = {0};
    u8 firstBuf[TEXT_BUF_SIZE];
    u8 secondBuf[TEXT_BUF_SIZE];

    src = firstBuf;
    bufs[1] = secondBuf;
    bufIndex = 0;
    bufs[0] = src;
    skip = 0;
    if (gTextOff == 0) {
        va_start(args, fmt);
        vsnprintf((char *)firstBuf, TEXT_BUF_SIZE, fmt, args);
        firstBuf[TEXT_BUF_SIZE - 1] = 0;
        lineX = Font_GetX();
        x = Font_GetX();
        y = Font_GetY();
        count = 0;
        len = 0;
        do {
            byte = *src;
            if (byte == 0) {
                break;
            }
            if (byte == '~') {
                if (skip) {
                    after = (u8 *)Text_ParseCode((char *)src, &code, TEXT_PARSE_CODE);
                    switch (code & TEXT_CODE_KIND) {
                    case TEXT_CODE_HIDE:
                        count += after - src;
                        src = after;
                        skip = 0;
                        continue;
                    }
                    goto normal;
                }
                bufs[bufIndex ^ 1][len] = 0;
                Font_Printf("%s", bufs[bufIndex ^ 1]);
                len = 0;
                Font_SetPos(x, y);
                codeStart = src;
                src = (u8 *)Text_ParseCode((char *)src, &code, TEXT_PARSE_CODE);
                codeValue = code;
                count += src - codeStart;
                switch (codeValue & TEXT_CODE_KIND) {
                case 0:
                    break;
                case TEXT_CODE_COLOR:
                    /* ~C00-~C15: the font's colours; ~C16 on: gTextColors from its 0x10th */
                    color = (s16)(codeValue & TEXT_CODE_ARG);
                    if (color >= 0x10) {
                        color += TEXT_COLOR_RGB;
                    }
                    Text_SetColor(color);
                    continue;
                case TEXT_CODE_VAR:
                    bufIndex ^= 1;
                    Text_FormatVar(bufs[bufIndex], codeValue & TEXT_CODE_ARG);
                    strncat((char *)bufs[bufIndex], src, TEXT_BUF_SIZE - Str_Length((char *)bufs[bufIndex]));
                    src = bufs[bufIndex];
                    count = 0;
                    continue;
                case TEXT_CODE_ICON:
                    Text_AddIcon(x, y, Font_GetCharH(), codeValue & TEXT_CODE_ARG);
                    x += Font_GetAdvance();
                    Font_SetPos(x, y);
                    if (max > 0) {
                        max--;
                    }
                    if (max == 0) {
                        if (count >= TEXT_BUF_SIZE) {
                            src = codeStart;
                        }
                        *src = 0;
                    }
                    continue;
                case TEXT_CODE_HIDE:
                    skip = 1;
                    continue;
                }
                if (*src == 0) {
                    break;
                }
            }
        normal:
            out = &bufs[bufIndex ^ 1];
            (*out)[len++] = *src;
            codeStart = src;
            byte = *src;
            /* charKind: 0 a two-byte character, 1 a one-byte one, 2 a newline */
            if (SJIS_IS_LEAD(byte)) {
                src++;
                (*out)[len++] = *src;
                count++;
                charKind = 0;
                if (max > 0) {
                    max--;
                }
            } else if (byte == '\n') {
                charKind = 2;
            } else {
                charKind = 1;
                if (max > 0) {
                    max--;
                }
            }
            count++;
            src++;
            if (charKind == 0) {
                x += Font_GetAdvance();
            } else if (charKind == 1) {
                advance = Font_GetAdvance();
                if (Font_GetHalfWidth() != 0) {
                    x += advance / 2;
                } else {
                    x += advance * 2 / 3;
                }
            } else {
                x = lineX;
                y += Font_GetCharH();
                if (Font_IsLineGapOn() != 0) {
                    y += Font_GetLineGap();
                }
                (*out)[len - 1] = 0;
                len = 0;
                Font_Printf("%s", *out);
                Font_SetPos(lineX, y);
            }
            if (max == 0) {
                if (count >= TEXT_BUF_SIZE) {
                    src = codeStart;
                }
                *src = 0;
            }
        } while (count < TEXT_BUF_SIZE - 1);
        bufs[bufIndex ^ 1][len] = 0;
        Font_Printf("%s", bufs[bufIndex ^ 1]);
    }
}

/* Text with a drop shadow: in colour shadowColor (Text_SetColor) offset by
 * the font's size, then in color at (x, y). */
void Text_DrawShadow(s16 x, s16 y, s16 shadowColor, s16 color, char *text)
{
    s16 shadowX;
    s16 shadowY;
    s32 width;
    s32 height;

    if (gTextOff == 0) {
        shadowX = 3;
        shadowY = 2;
        width = Font_GetAdvance();
        height = Font_GetAdvance();
        if (width >= 0x18) {
            shadowX++;
        }
        if (height >= 0x18) {
            shadowY += 2;
        }
        if (height > 0x14 && height < 0x18) {
            shadowY++;
        }
        Font_SetPos(x + shadowX, y + shadowY);
        Text_SetColor(shadowColor);
        Font_Printf("%s", text);
        Font_SetPos(x, y);
        Text_SetColor(color);
        Font_Printf("%s", text);
    }
}

/* Text_DrawShadow with the font's colour indexes (Font_SetColorIndex) and
 * a shadow offset of 2 to 4 each way by the font's size. */
void Text_DrawShadow2(s16 x, s16 y, s32 shadowColor, s32 color, char *text)
{
    s16 shadowX;
    s16 shadowY;
    s32 width;
    s32 height;

    if (gTextOff == 0) {
        shadowY = shadowX = 2;
        width = Font_GetAdvance();
        height = Font_GetAdvance();
        if (width >= 0x18) {
            shadowX += 2;
        }
        if (width > 0x14 && width < 0x18) {
            shadowX++;
        }
        if (height >= 0x18) {
            shadowY += 2;
        }
        if (height > 0x14 && height < 0x18) {
            shadowY++;
        }
        Font_SetPos(x + shadowX, y + shadowY);
        Font_SetPal(0);
        Font_SetColorIndex(shadowColor);
        Font_Printf("%s", text);
        Font_SetPos(x, y);
        Font_SetColorIndex(color);
        Font_Printf("%s", text);
    }
}

/* Text with a drop shadow 2 below and right, both colours RGBA. */
void Text_DrawShadowRgb(s16 x, s16 y, u32 shadowColor, u32 color, char *text)
{
    if (gTextOff == 0) {
        Font_SetPos(x + 2, y + 2);
        Font_SetColor(shadowColor);
        Font_Printf("%s", text);
        Font_SetPos(x, y);
        Font_SetColor(color);
        Font_Printf("%s", text);
    }
}

/* Draws a string at the font's position, unless text drawing is off. */
void Text_Draw(char *text)
{
    if (gTextOff == 0) {
        Font_Printf("%s", text);
    }
}

/* The length of a string as displayed: strlen less the control codes
 * (Text_ParseCode mode 1), ~A numbers counted as inserted. */
s32 Text_DisplayLength(char *text)
{
    s16 code;
    s32 len;
    s16 skip;
    s8 character;
    u8 byte;

    code = 0;
    skip = 0;
    len = Str_Length(text);
    while (*text != 0) {
        character = *text;
        byte = *text;
        if (SJIS_IS_LEAD(byte)) {
            text += 2;
        } else if (character == '~') {
            Text_ParseCode(text, &code, TEXT_PARSE_CODE);
            if ((code & TEXT_CODE_KIND) == TEXT_CODE_HIDE) {
                skip ^= 1;
                text = Text_ParseCode(text, &code, TEXT_PARSE_WIDTH);
                len += code;
            } else if (skip == 0) {
                text = Text_ParseCode(text, &code, TEXT_PARSE_WIDTH);
                len += code;
            } else {
                goto next;
            }
        } else {
        next:
            text++;
        }
    }
    return len;
}

/* The number of characters of a string, not counting newlines. */
s32 Text_CountChars(char *text)
{
    s32 count = 0;
    s32 character;
    u8 byte;

    while (*text != 0) {
        character = *text;
        byte = *text;
        if (SJIS_IS_LEAD(byte)) {
            text++;
        } else if ((char)character == '\n') {
            text++;
            continue;
        }
        text++;
        count++;
    }
    return count;
}

/*
 * Copies the next character (1 or 2 bytes) or control code of text into out
 * and returns what it was, TEXT_CHAR_*. Colour and hide codes are skipped;
 * *next is set after it and any hide codes that follow.
 */
s32 Text_NextChar(char *out, char *text, char **next)
{
    s16 code;
    s32 type;
    s32 character;

    if (*text == 0) {
        *next = text;
        *out = 0;
        return TEXT_CHAR_END;
    }
    for (;;) {
        character = *text;
        if (character == '~') {
            code = 0;
            text = Text_ParseCode(text, &code, TEXT_PARSE_CODE);
            character = code;
            switch (character & TEXT_CODE_KIND) {
            case 0:
                text++;
                out[0] = '~';
                type = TEXT_CHAR_SINGLE;
                out[1] = 0;
                break;
            case TEXT_CODE_COLOR:
            case TEXT_CODE_HIDE:
                continue;
            case TEXT_CODE_VAR:
                out[0] = character;
                out[1] = 0;
                type = TEXT_CHAR_VAR;
                break;
            case TEXT_CODE_ICON:
                out[0] = character;
                out[1] = 0;
                type = TEXT_CHAR_ICON;
                break;
            default:
                continue;
            }
        } else {
            if (SJIS_IS_LEAD((u8)character)) {
                *out = character;
                text++;
                out++;
                type = TEXT_CHAR_DOUBLE;
            } else {
                type = TEXT_CHAR_SINGLE;
            }
            *out = *text++;
            out[1] = 0;
        }
        break;
    }
    for (;;) {
        *next = text;
        if (*text != '~') {
            break;
        }
        text = Text_ParseCode(text, &code, TEXT_PARSE_CODE);
        switch (code & TEXT_CODE_KIND) {
        case 0:
        case TEXT_CODE_VAR:
        case TEXT_CODE_ICON:
        case TEXT_CODE_COLOR:
            goto end;
        case TEXT_CODE_HIDE:
            break;
        }
    }
end:
    return type;
}

/* The font size, 0..6. */
void Text_SetSize(s32 size)
{
    if (size < 0) {
        size = 0;
    }
    if (size >= 7) {
        size = 6;
    }
    Font_SetSize(1.0f + 1000.0f * size / 7.0f);
}

/* The text colour by index: TEXT_COLOR_RGB up to TEXT_COLOR_RGB_END from
 * gTextCodeColors, below it the font's own colours. */
void Text_SetColor(s16 color)
{
    if (color >= TEXT_COLOR_RGB) {
        if (color < TEXT_COLOR_RGB_END) {
            Font_SetPal(0);
            Font_SetColor(gTextCodeColors[color]);
        }
    } else {
        Font_SetColorIndex(color);
    }
}

void Text_DrawAllPages(void)
{
    Font_DrawAllPages();
}

/* The text's defaults: spacing 0x14 each way, size 0, colour 0. */
void Text_SetDefaults(void)
{
    Font_SetSpacing(0x14, 0x14);
    Text_SetSize(0);
    Text_SetColor(0);
}

void Text_Disable(void)
{
    gTextOff = 1;
}

void MemFile_Load(void)
{
    gSysSettings.memFile = gMemFileBuf;
    gSysSettings.memFileSize = LoadCompressedFile(gMemFileBuf, 0x19EB);
}

/* Key repeat for PAD_UP and PAD_DOWN: returns the buttons pressed, or the
 * held ones after a first delay of 10 frames, then every 4. */
u16 Pad_KeyRepeat(s32 pad, u16 held, u16 pressed)
{
    u16 result = pressed;

    if (pressed & (PAD_UP | PAD_DOWN)) {
        gKeyRepeat[pad].repeating = 0;
        gKeyRepeat[pad].timer = 0;
    } else if (held & (PAD_UP | PAD_DOWN)) {
        result = 0;
        if (gKeyRepeat[pad].repeating != 0) {
            gKeyRepeatDelay[pad] = 4;
        } else {
            gKeyRepeatDelay[pad] = 10;
        }
        if (++gKeyRepeat[pad].timer > gKeyRepeatDelay[pad]) {
            gKeyRepeat[pad].timer = 0;
            result = held;
            gKeyRepeat[pad].repeating = 1;
        }
    } else {
        gKeyRepeat[pad].repeating = 0;
        gKeyRepeat[pad].timer = 0;
    }
    return result;
}

/*
 * Draws a string with ~Cnn colour codes, each the colour TEXT_COLOR_RGB + nn
 * of gTextColors (other codes are drawn as they are). The text before each
 * code that changes the colour is drawn and cut off.
 */
void Text_DrawColored(char *text)
{
    char buf[TEXT_BUF_SIZE];
    s16 size;
    char *rest;
    char *tilde;
    s32 len;
    s16 x;
    s16 y;
    u32 color;
    s32 colorIndex;

    strcpy(buf, text);
    tilde = strchr(buf, '~');
    if (tilde == NULL) {
        Font_Printf("%s", buf);
        return;
    }
    size = Font_GetAdvance();
    x = Font_GetX();
    y = Font_GetY();
    do {
        if (tilde[1] == 'C') {
            colorIndex = (u8)((tilde[3] - '0') + (tilde[2] - '0') * 10) + TEXT_COLOR_RGB;
            if (colorIndex >= TEXT_COLOR_RGB_END) {
                strcpy(tilde, tilde + 4);
                rest = tilde;
            } else {
                color = gTextColors[colorIndex - TEXT_COLOR_RGB];
                if (Font_GetPal() == 0 && color == Font_GetColor()) {
                    strcpy(tilde, tilde + 4);
                    rest = tilde;
                } else {
                    rest = buf;
                    *tilde = 0;
                    len = Str_Length(rest);
                    if (len != 0) {
                        Font_SetPos(x, y);
                        Font_Printf(rest);
                        x += (s16)(len * (size / 2.0f));
                    }
                    strcpy(rest, tilde + 4);
                    Font_SetColor(color);
                }
            }
        } else {
            rest = tilde + 1;
        }
        tilde = strchr(rest, '~');
    } while (tilde != NULL);
    if (Str_Length(buf) != 0) {
        Font_SetPos(x, y);
        Font_Printf(buf);
    }
}

/* Applies the options to the system settings. */
void Options_Apply(void)
{
    SysSettings *sys = &gSysSettings;

    gSysSettings.soundMode = gSysSave.options.soundMode;
    gSysSettings.musicVolume = gSysSave.options.musicVolume;
    gSysSettings.seVolume = gSysSave.options.seVolume;
    gSysSettings.brightness = gSysSave.options.brightness;
    if (gSysSettings.brightness < 0x38 || sys->brightness > 0xA4) {
        sys->brightness = 0x80;
        gSysSave.options.brightness = 0x80;
    }
    Sound_ApplyOutputMode(sys->soundMode);
    Stream_UpdateMaxVol(1);
    Display_SetOffset(gSysSave.options.screenX, gSysSave.options.screenY);
}

/* Records the quest to run: its id, its data and whether it came from the
 * quest list. */
void Quest_SetCurrent(u16 questId, void *questData, s32 fromList)
{
    gSysSettings.questData = questData;
    gSysSettings.questId = questId;
    gSysSettings.questFromList = fromList;
}

/* Clears the model, material, bone and primitive pools, the work area,
 * the camera data and the model load buffer. */
void Gfx_ClearModelBuffers(void)
{
    memset(gObjBufBlocks, 0, 0x20000);
    memset(gMatSets, 0, 0xD200);
    memset(gMaterials, 0, 0x14000);
    memset(gBones, 0, 0xC8000);
    memset(gModels, 0, 0x4000);
    memset(gPrims, 0, 0x5000);
    memset(gSysSettings.extraBuf, 0, 0x14000);
    memset(gWorkArea, 0, 0x18000);
    memset(gSysSettings.camData, 0, 0x2000);
    memset(gModelLoadBuf, 0, 0x134000);
}

/*
 * Resets the game's subsystems for a new scene: the camera, render states,
 * motions, models, textures, objects, players, monsters, areas, draw lists,
 * sound and text, and reloads the two common texture files (slots 4 and 5).
 */
void Game_ResetSubsystems(void)
{
    s16 i;

    FileQueue_Cancel();
    Camera_Init();
    Camera_SetPreset(0);
    Gfx_ResetRenderStates(0);
    Motion_FreeAll();
    Model_FreeAll();
    UnreadFlags_ClearAll();
    Movie_Stop();
    ScreenTex_Release();
    Tex_ReleaseAll();
    Mem_Compact();
    Model_ResetAll();
    Obj_InitAll();
    Area_ResetStub();
    Light_Reset();
    Player_ClearAll();
    EmSlot_ClearAll();
    Area_ClearState();
    EquipPreview_Clear();
    memset(gAreaSubModels, 0, 0x10);
    memset(gAreaSubModel, 0, 8);
    memset(gCommonModels, 0, 0x14);
    Mem_Compact();
    Callbacks_Clear();
    Gs_SetTableColor(0);
    Tex_LoadFile(4, 0x148, 0);
    Tex_LoadFile(5, 0x155, 0);
    FadeA_Reset();
    FreeList_InitAll();
    DrawSlot_ResetAll();
    DrawSlotB_ResetAll();
    Sound_Silence();
    Stream_Reset();
    Vib_StopAll();
    Sound_ReverbOn();
    Sound_SetSpuMapPage(0);
    gScreenCaptureOk = 0;
    gFieldControl = 1;
    gSysSettings.clearCapture = 0;
    gSysBusy = 0;
    Text_Reset();
    gSoundLoading = 0;
    gSysSettings.cardMenuDone = 1;
    if (Overlay_IsLoaded(OVERLAY_SUB_MAIN) == 1) {
        Notice_ResetQueue();
    }
    for (i = 0; i < MOTIONS_EM_SLOTS; i++) {
        EmSlotModel_Clear(i, 2);
    }
}

/* Frees every loaded motion (gMotions) and clears its entry. */
void Motion_FreeAll(void)
{
    s32 i;
    void **motion = gMotions;

    for (i = 0; i < MOTIONS_TOTAL; i++, motion++) {
        if (*motion != NULL && Mem_FreeRet1(*motion) == 1) {
            *motion = NULL;
        }
    }
}

/* Frees the common set's motions (those before the players'). */
void Motion_FreeCommon(void)
{
    s32 i;
    void **motion = gMotions;

    for (i = 0; i < MOTIONS_PLAYER_FIRST; i++, motion++) {
        if (*motion != NULL && Mem_FreeRet1(*motion) == 1) {
            *motion = NULL;
        }
    }
}

/* Clears monster slot `slot`'s model and frees its motions. */
void Motion_FreeEmSlot(s16 slot)
{
    s32 end;
    s16 i;

    EmSlotModel_Clear(slot, 1);
    for (i = slot * MOTIONS_PER_EM + MOTIONS_EM_FIRST, end = i + MOTIONS_PER_EM; i < end; i++) {
        if (gMotions[i] != NULL && Mem_FreeRet1(gMotions[i]) == 1) {
            gMotions[i] = NULL;
        }
    }
}

/* unk_368F10's .data (0x43C8C0-0x43C990) */
u32 gTextColors[24] = {
#include "sub_main/0043C8C0.inc"
};
u32 gBlendModes[12] = {
#include "sub_main/0043C920.inc"
};
u32 gBlendModes2[10] = {
#include "sub_main/0043C950.inc"
};
u32 gTexFilterModes[2] = {
#include "sub_main/0043C978.inc"
};
u32 gBlendOps[4] = {
#include "sub_main/0043C980.inc"
};
