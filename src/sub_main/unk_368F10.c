/*
 * sub_main 0x368F10-0x36B5B0:
 * - render-state shortcuts over Gs_SetState that skip unchanged values;
 * - the text layer: printf-style drawing through the font library at
 *   0x2E5E50-0x2E6DE0 (Font_Printf is its printf, Str_Length strlen),
 *   control codes introduced by '~' (Text_ParseCode), the text renderer
 *   Text_PrintCodes, drop-shadow text, Shift-JIS aware scanning;
 * - key repeat, applying the options, and the game-state reset
 *   (Game_ResetSubsystems).
 * The libc used: sprintf sprintf, vsnprintf vsnprintf, strchr
 * strchr, strcpy strcpy, strncat strncat (probably).
 */
#include "common.h"
#include "model.h"
#include "sprite.h"
#include "syssettings.h"
#include "syssave.h"

#include "ee_stdarg.h"

/* A box queued on a text layer (8 bytes; Text_AddBox). */
typedef struct TextBox {
    s16 x;
    s16 y;
    s16 w;
    u8 unk6;
    u8 unk7;            /* 0xFF */
} TextBox;

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
extern u32 D_0043C978[];
extern u32 gBlendOps[];
extern u32 gTextCodeColors[];
extern u32 gTextColors[];       /* text colours 0x20..0x34 */
extern u32 gTexHandles[];
extern s32 gTextOff;          /* nonzero: text drawing is off */
extern s16 gTextBoxCount[7];       /* boxes queued per text layer */
extern TextBox gTextBoxes[7][8];
extern s32 D_006AEFA8;
extern s32 D_006AEFD8;
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
s32 FontCache_GetUnk5E(void);
/* unit's own declaration: s16 (the definition, unk_2E3940.c, returns s32: (s16) of a u8, 0-255); with s32, in every form tried, Text_PrintCodes sign-extends the value before y += (2 more words) */
s16 FontCache_GetUnk5F(void);
void strncat(char *dst, u8 *src, s32 n);
void FontCache_SetUnk5E(u8 v);
void FontCache_SetUnk5F(s32 a);
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

/* Sets render state 0x5E from two table entries, when either changes. */
void Gfx_SetBlend(s32 a, s32 b)
{
    if (gSysSettings.blendA != a || gSysSettings.blendB != b) {
        gSysSettings.blendA = a;
        gSysSettings.blendB = b;
        Gs_SetState(0x5E, gBlendModes[a] | gBlendModes2[b]);
    }
}

void Gfx_SetState63(s32 a)
{
    if (gSysSettings.state63 != a) {
        gSysSettings.state63 = a;
        Gs_SetState(0x63, D_0043C978[a]);
    }
}

void Tex_Bind(s32 i)
{
    gSysSettings.boundTex = i;
    Gs_SetState(4, gTexHandles[i]);
}

/* Default render states (the fog ones only when partial is 0) and the
 * cached values of Gfx_SetBlend, Gfx_SetState63, Tex_Bind and
 * Gfx_SetBlendOp. */
void Gfx_ResetRenderStates(s32 partial)
{
    f32 fogNear = 20000.0f;
    f32 fogFar = 30000.0f;

    if (partial == 0) {
        Gs_SetState(0xE, 0);
        Gs_SetState(1, 0x200);
        Gs_SetState(0x15, 2);
        Gs_SetState(0xF, 0x80FFFFFF);
        Gs_SetState(0x10, *(u32 *)&fogNear);
        Gs_SetState(0x11, *(u32 *)&fogFar);
        Gs_SetState(0x5F, 4);
    }
    gSysSettings.blendOp = 0;
    gSysSettings.blendA = 4;
    gSysSettings.blendB = 5;
    gSysSettings.state63 = 1;
    gSysSettings.unk3D = 0x80;
    gSysSettings.boundTex = 0xFFFF;
    gSysSettings.unk3E = 1;
    Gs_SetState(0x5E, 0x32);
    Gs_SetState(0x63, 0);
    Gs_SetState(0xD, 0);
    Gs_SetState(0x60, 0);
}

void Gfx_SetBlendOp(s32 a)
{
    if (gSysSettings.blendOp != a) {
        gSysSettings.blendOp = a;
        Gs_SetState(0xD, gBlendOps[a]);
    }
}

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

void Text_Reset(void)
{
    TextObj_Reset();
    gTextOff = 0;
    gTextBoxCount[0] = 0;
    gTextBoxCount[1] = 0;
    gTextBoxCount[2] = 0;
    gTextBoxCount[3] = 0;
    gTextBoxCount[4] = 0;
    gTextBoxCount[5] = 0;
    gTextBoxCount[6] = 0;
}

void Text_FlushPages(void)
{
    s32 n = Font_GetPage();
    s32 i;

    for (i = 0; i <= n; i++) {
        Font_DrawPage(i);
    }
    Font_ClearPage(n);
}

void Text_DrawAllAndStop(void)
{
    s16 i;

    for (i = 0; i < 7; i++) {
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

void Text_DrawBoxes(s32 layer)
{
    if (gTextBoxCount[layer] != 0) {
        Draw_Icons((struct Icon *)gTextBoxes[layer], gTextBoxCount[layer]);
    }
}

/* printf to the screen (control codes allowed; Text_DrawColored). */
void Text_PrintfColored(const char *fmt, ...)
{
    char buf[0x400];
    va_list ap;

    if (gTextOff == 0) {
        va_start(ap, fmt);
        vsnprintf(buf, 0x3FF, fmt, ap);
        buf[0x3FF] = 0;
        Text_DrawColored(buf);
    }
}

/* printf at (x, y) in colour index color. */
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...)
{
    char buf[0x400];
    va_list ap;

    if (gTextOff == 0) {
        Font_SetPos(x, y);
        Text_SetColor(color);
        va_start(ap, fmt);
        vsnprintf(buf, 0x400, fmt, ap);
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

void Text_DrawLinesColored(s16 x, s16 y, char **lines, s16 dy)
{
    while (*lines != NULL) {
        Font_SetPos(x, y);
        Text_PrintfColored(*lines);
        lines++;
        y += dy;
    }
}

void Text_FormatVar(void *dst, s16 kind)
{
    char buf[0x80];

    switch (kind) {
    case 0:
        sprintf(buf, "%d", D_006AEFA8 + 1);
        Sjis_Widen(buf, dst);
        break;
    case 1:
        sprintf(buf, "%d", D_006AEFD8);
        Sjis_Widen(buf, dst);
        break;
    }
}

/* Queues a box on the current text layer (at most 8). */
void Text_AddBox(s16 x, s16 y, s16 w, s16 c)
{
    s32 layer = Font_GetPage();
    s16 n = gTextBoxCount[layer];

    if (n < 8) {
        gTextBoxes[layer][n].x = x;
        gTextBoxes[layer][n].y = y;
        gTextBoxes[layer][n].w = w;
        gTextBoxes[layer][n].unk6 = c;
        gTextBoxes[layer][n].unk7 = 0xFF;
        gTextBoxCount[layer]++;
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
 * Parses the control code at s ('~' then a letter) and returns the text
 * after it. mode 0 gives the code: 0x100 | n for ~Cnn (colour), 0x200 | n for
 * ~Ann (a number inserted by Text_FormatVar), 0x400 | n for ~Bnn, 0x800 for
 * ~%; modes 1 and 2 give the code's width and character count instead.
 */
char *Text_ParseCode(char *s, s16 *code, s16 mode)
{
    char buf[0x80];
    s16 n;

    *code = 0;
    s++;
    switch (*s) {
    case '~':
        switch (mode) {
        case 0:
            *code = 0;
            break;
        case 1:
            s++;
            *code = -1;
            break;
        case 2:
            s++;
            *code = 1;
            break;
        }
        break;
    /* the game reads a register it never set: n is a1, which holds code (the pointer) when no digit follows ~C, ~A or ~B */
    case 'C':
        s++;
        switch (mode) {
        case 0:
            READ_NUM(s, n);
            *code = n | 0x100;
            break;
        case 1:
            s += 2;
            *code = -4;
            break;
        case 2:
            *code = 0;
            s += 2;
            break;
        }
        break;
    case 'A':
        s++;
        READ_NUM(s, n);
        switch (mode) {
        case 0:
            *code = n | 0x200;
            break;
        case 1:
            Text_FormatVar(buf, n);
            *code = Str_Length(buf) - 4;
            break;
        case 2:
            Text_FormatVar(buf, n);
            *code = Text_CountChars(buf);
            break;
        }
        break;
    case 'B':
        s++;
        switch (mode) {
        case 0:
            READ_NUM(s, n);
            *code = n | 0x400;
            break;
        case 1:
            s += 2;
            *code = -2;
            break;
        case 2:
            s += 2;
            *code = 1;
            break;
        }
        break;
    case '%':
        s++;
        switch (mode) {
        case 0:
            *code = 0x800;
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
    return s;
}

/* printf through the text renderer (Text_PrintCodes). */
void Text_Printf(const char *fmt, ...)
{
    char buf[0x400];
    va_list ap;

    if (gTextOff == 0) {
        va_start(ap, fmt);
        vsnprintf(buf, 0x400, fmt, ap);
        Text_PrintCodes(-1, "%s", buf);
    }
}

void Text_PrintfStyled(s32 a, const char *fmt, ...)
{
    char buf[0x400];
    va_list ap;

    if (gTextOff == 0) {
        va_start(ap, fmt);
        vsnprintf(buf, 0x400, fmt, ap);
        FontCache_SetUnk5E(1);
        FontCache_SetUnk5F(a);
        Text_PrintCodes(-1, "%s", buf);
        FontCache_SetUnk5E(0);
    }
}

/*
 * The text renderer: prints fmt with the control codes of Text_ParseCode
 * (~Cnn colour, ~Ann inserted number, ~Bnn box, ~% skip until the next ~%),
 * half-width characters, newlines, and at most max characters (-1: no limit).
 * The output alternates between two buffers so that ~A can splice text in.
 */
void Text_PrintCodes(s16 max, const char *fmt, ...)
{
    u8 *prev;
    s32 c7;
    s16 bi;
    u8 *p;
    s16 count;
    s16 len;
    s16 x;
    s16 y;
    s16 x0;
    s16 skip;
    s16 kind;
    s16 w;
    s16 code;
    u16 col;
    u8 **t;
    u8 *q;
    u8 c;
    va_list ap;
    u8 *bufs[2] = {0};
    u8 buf1[0x400];
    u8 buf2[0x400];

    p = buf1;
    bufs[1] = buf2;
    bi = 0;
    bufs[0] = p;
    skip = 0;
    if (gTextOff == 0) {
        va_start(ap, fmt);
        vsnprintf((char *)buf1, 0x400, fmt, ap);
        buf1[0x3FF] = 0;
        x0 = Font_GetX();
        x = Font_GetX();
        y = Font_GetY();
        count = 0;
        len = 0;
        do {
            c = *p;
            if (c == 0) {
                break;
            }
            if (c == '~') {
                if (skip) {
                    q = (u8 *)Text_ParseCode((char *)p, &code, 0);
                    switch (code & 0xFF00) {
                    case 0x800:
                        count += q - p;
                        p = q;
                        skip = 0;
                        continue;
                    }
                    goto normal;
                }
                bufs[bi ^ 1][len] = 0;
                Font_Printf("%s", bufs[bi ^ 1]);
                len = 0;
                Font_SetPos(x, y);
                prev = p;
                p = (u8 *)Text_ParseCode((char *)p, &code, 0);
                c7 = code;
                count += p - prev;
                switch (c7 & 0xFF00) {
                case 0:
                    break;
                case 0x100:
                    col = (s16)(c7 & 0xFF);
                    if (col >= 0x10) {
                        col += 0x20;
                    }
                    Text_SetColor(col);
                    continue;
                case 0x200:
                    bi ^= 1;
                    Text_FormatVar(bufs[bi], c7 & 0xFF);
                    strncat((char *)bufs[bi], p, 0x400 - Str_Length((char *)bufs[bi]));
                    p = bufs[bi];
                    count = 0;
                    continue;
                case 0x400:
                    Text_AddBox(x, y, Font_GetCharH(), c7 & 0xFF);
                    x += Font_GetAdvance();
                    Font_SetPos(x, y);
                    if (max > 0) {
                        max--;
                    }
                    if (max == 0) {
                        if (count >= 0x400) {
                            p = prev;
                        }
                        *p = 0;
                    }
                    continue;
                case 0x800:
                    skip = 1;
                    continue;
                }
                if (*p == 0) {
                    break;
                }
            }
        normal:
            t = &bufs[bi ^ 1];
            (*t)[len++] = *p;
            prev = p;
            c = *p;
            if ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFF)) {
                p++;
                (*t)[len++] = *p;
                count++;
                kind = 0;
                if (max > 0) {
                    max--;
                }
            } else if (c == '\n') {
                kind = 2;
            } else {
                kind = 1;
                if (max > 0) {
                    max--;
                }
            }
            count++;
            p++;
            if (kind == 0) {
                x += Font_GetAdvance();
            } else if (kind == 1) {
                w = Font_GetAdvance();
                if (Font_GetHalfWidth() != 0) {
                    x += w / 2;
                } else {
                    x += w * 2 / 3;
                }
            } else {
                x = x0;
                y += Font_GetCharH();
                if (FontCache_GetUnk5E() != 0) {
                    y += FontCache_GetUnk5F();
                }
                (*t)[len - 1] = 0;
                len = 0;
                Font_Printf("%s", *t);
                Font_SetPos(x0, y);
            }
            if (max == 0) {
                if (count >= 0x400) {
                    p = prev;
                }
                *p = 0;
            }
        } while (count < 0x3FF);
        bufs[bi ^ 1][len] = 0;
        Font_Printf("%s", bufs[bi ^ 1]);
    }
}

/* Text with a drop shadow: colour c1 offset by the font size, then c2. */
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s)
{
    s16 dx;
    s16 dy;
    s32 w;
    s32 h;

    if (gTextOff == 0) {
        dx = 3;
        dy = 2;
        w = Font_GetAdvance();
        h = Font_GetAdvance();
        if (w >= 0x18) {
            dx++;
        }
        if (h >= 0x18) {
            dy += 2;
        }
        if (h > 0x14 && h < 0x18) {
            dy++;
        }
        Font_SetPos(x + dx, y + dy);
        Text_SetColor(c1);
        Font_Printf("%s", s);
        Font_SetPos(x, y);
        Text_SetColor(c2);
        Font_Printf("%s", s);
    }
}

void Text_DrawShadow2(s16 x, s16 y, s32 c1, s32 c2, char *s)
{
    s16 dx;
    s16 dy;
    s32 w;
    s32 h;

    if (gTextOff == 0) {
        dy = dx = 2;
        w = Font_GetAdvance();
        h = Font_GetAdvance();
        if (w >= 0x18) {
            dx += 2;
        }
        if (w > 0x14 && w < 0x18) {
            dx++;
        }
        if (h >= 0x18) {
            dy += 2;
        }
        if (h > 0x14 && h < 0x18) {
            dy++;
        }
        Font_SetPos(x + dx, y + dy);
        Font_SetPal(0);
        Font_SetColorIndex(c1);
        Font_Printf("%s", s);
        Font_SetPos(x, y);
        Font_SetColorIndex(c2);
        Font_Printf("%s", s);
    }
}

void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s)
{
    if (gTextOff == 0) {
        Font_SetPos(x + 2, y + 2);
        Font_SetColor(c1);
        Font_Printf("%s", s);
        Font_SetPos(x, y);
        Font_SetColor(c2);
        Font_Printf("%s", s);
    }
}

void Text_Draw(char *s)
{
    if (gTextOff == 0) {
        Font_Printf("%s", s);
    }
}

/* The length of a string as displayed: strlen less the control codes
 * (Text_ParseCode mode 1), ~A numbers counted as inserted. */
s32 Text_DisplayLength(char *s)
{
    s16 code;
    s32 w;
    s16 skip;
    s8 ch;
    u8 c;

    code = 0;
    skip = 0;
    w = Str_Length(s);
    while (*s != 0) {
        ch = *s;
        c = *s;
        if ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFF)) {
            s += 2;
        } else if (ch == '~') {
            Text_ParseCode(s, &code, 0);
            if ((code & 0xFF00) == 0x800) {
                skip ^= 1;
                s = Text_ParseCode(s, &code, 1);
                w += code;
            } else if (skip == 0) {
                s = Text_ParseCode(s, &code, 1);
                w += code;
            } else {
                goto next;
            }
        } else {
        next:
            s++;
        }
    }
    return w;
}

/* The number of characters of a string, not counting newlines. */
s32 Text_CountChars(char *s)
{
    s32 n = 0;
    s32 ch;
    u8 c;

    while (*s != 0) {
        ch = *s;
        c = *s;
        if ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFF)) {
            s++;
        } else if ((char)ch == '\n') {
            s++;
            continue;
        }
        s++;
        n++;
    }
    return n;
}

/*
 * Copies the next character (1 or 2 bytes) or control code of s into out
 * and returns its kind: 0 a 1-byte character, 1 a 2-byte one, 2 a ~B code,
 * 3 a ~A code, 4 the end of the string. Codes 0x100 and 0x800 are skipped;
 * *next is set after it and any following 0x800 codes.
 */
s32 Text_NextChar(char *out, char *s, char **next)
{
    s16 code;
    s32 type;
    s32 c;

    if (*s == 0) {
        *next = s;
        *out = 0;
        return 4;
    }
    for (;;) {
        c = *s;
        if (c == '~') {
            code = 0;
            s = Text_ParseCode(s, &code, 0);
            c = code;
            switch (c & 0xFF00) {
            case 0:
                s++;
                out[0] = '~';
                type = 0;
                out[1] = 0;
                break;
            case 0x100:
            case 0x800:
                continue;
            case 0x200:
                out[0] = c;
                out[1] = 0;
                type = 3;
                break;
            case 0x400:
                out[0] = c;
                out[1] = 0;
                type = 2;
                break;
            default:
                continue;
            }
        } else {
            if (((u8)c >= 0x80 && (u8)c <= 0x9F) || ((u8)c >= 0xE0 && (u8)c <= 0xFF)) {
                *out = c;
                s++;
                out++;
                type = 1;
            } else {
                type = 0;
            }
            *out = *s++;
            out[1] = 0;
        }
        break;
    }
    for (;;) {
        *next = s;
        if (*s != '~') {
            break;
        }
        s = Text_ParseCode(s, &code, 0);
        switch (code & 0xFF00) {
        case 0:
        case 0x200:
        case 0x400:
        case 0x100:
            goto end;
        case 0x800:
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

/* The text colour by index: 0x20..0x34 from gTextCodeColors, below 0x20 the
 * font's own colours. */
void Text_SetColor(s16 color)
{
    if (color >= 0x20) {
        if (color < 0x35) {
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

/* Key repeat for buttons 0x3000: a first delay of 10 frames, then every 4. */
u16 Pad_KeyRepeat(s32 pad, u16 held, u16 pressed)
{
    u16 r = pressed;

    if (pressed & 0x3000) {
        gKeyRepeat[pad].repeating = 0;
        gKeyRepeat[pad].timer = 0;
    } else if (held & 0x3000) {
        r = 0;
        if (gKeyRepeat[pad].repeating != 0) {
            gKeyRepeatDelay[pad] = 4;
        } else {
            gKeyRepeatDelay[pad] = 10;
        }
        if (++gKeyRepeat[pad].timer > gKeyRepeatDelay[pad]) {
            gKeyRepeat[pad].timer = 0;
            r = held;
            gKeyRepeat[pad].repeating = 1;
        }
    } else {
        gKeyRepeat[pad].repeating = 0;
        gKeyRepeat[pad].timer = 0;
    }
    return r;
}

/* Draws a string with "~Cnn" colour codes (0x20..0x34 through gTextCodeColors). */
void Text_DrawColored(char *s)
{
    char buf[0x400];
    s16 size;
    char *q;
    char *p;
    s32 w;
    s16 x;
    s16 y;
    u32 color;
    s32 idx;

    strcpy(buf, s);
    p = strchr(buf, '~');
    if (p == NULL) {
        Font_Printf("%s", buf);
        return;
    }
    size = Font_GetAdvance();
    x = Font_GetX();
    y = Font_GetY();
    do {
        if (p[1] == 'C') {
            idx = (u8)((p[3] - '0') + (p[2] - '0') * 10) + 0x20;
            if (idx >= 0x35) {
                strcpy(p, p + 4);
                q = p;
            } else {
                color = gTextColors[idx - 0x20];
                if (Font_GetPal() == 0 && color == Font_GetColor()) {
                    strcpy(p, p + 4);
                    q = p;
                } else {
                    q = buf;
                    *p = 0;
                    w = Str_Length(q);
                    if (w != 0) {
                        Font_SetPos(x, y);
                        Font_Printf(q);
                        x += (s16)(w * (size / 2.0f));
                    }
                    strcpy(q, p + 4);
                    Font_SetColor(color);
                }
            }
        } else {
            q = p + 1;
        }
        p = strchr(q, '~');
    } while (p != NULL);
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

void Quest_SetCurrent(u16 a, void *b, s32 c)
{
    gSysSettings.questData = b;
    gSysSettings.questId = a;
    gSysSettings.questFromList = c;
}

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

/* Resets the game state. */
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
    gSysSettings.unk12 = 1;
    if (Overlay_IsLoaded(1) == 1) {
        Notice_ResetQueue();
    }
    for (i = 0; i < 4; i++) {
        EmSlotModel_Clear(i, 2);
    }
}

void Motion_FreeAll(void)
{
    s32 i;
    void **p = gMotions;

    for (i = 0; i < 0x10D6; i++, p++) {
        if (*p != NULL && Mem_FreeRet1(*p) == 1) {
            *p = NULL;
        }
    }
}

void Motion_FreeCommon(void)
{
    s32 i;
    void **p = gMotions;

    for (i = 0; i < 0x2C6; i++, p++) {
        if (*p != NULL && Mem_FreeRet1(*p) == 1) {
            *p = NULL;
        }
    }
}

void Motion_FreeEmSlot(s16 n)
{
    s32 end;
    s16 i;

    EmSlotModel_Clear(n, 1);
    for (i = n * 600 + 0x776, end = i + 600; i < end; i++) {
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
u32 D_0043C978[2] = {
#include "sub_main/0043C978.inc"
};
u32 gBlendOps[4] = {
#include "sub_main/0043C980.inc"
};
