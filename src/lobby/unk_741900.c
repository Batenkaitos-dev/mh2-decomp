/* The primitives' GS packets, and the page's own blocks outside HTML:
 * <!--MMBB-FLAG-PASS-->, <GAME-STYLE> and <CSV> (the page's kind and style
 * keywords, the data for the MMBB block), then Html_Parse, which runs the
 * parser over a page. include/browser.h, include/html.h. */
#include "common.h"
#include "m2c_macros.h"
#include "browser.h"
#include "html.h"
#include "lobby_globals.h"
#include "hw_regs.h"

/* This unit was compiled at optimization level 4 (Html_ParseSome). */
#pragma optimization_level 4

s32 strncmp(const void *a, const void *b, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
void *memset(void *dst, s32 c, u32 n);
u32 strlen(const void *s);
s32 strcmp(const void *a, const void *b); /* strcmp */
s32 atoi(const char *s);               /* atoi */
void Page_SetDataMode(u8 mode);
void Mmbb_SetField(s8 which, s8 idx, char *value); /* idx as its callers see it (it is a u8 in unk_72AD50.c) */ /* unit's own declaration: with the definition's void (s8 which, u8 idx, char *value) (lobby/unk_72AD50.c), Html_Csv 220 of 364 words differ */

extern s32 gSysFrameMode;
/* The primitives' GS packets, as main's (src/main/unk_186D80.c, whose
 * Prim_FillRect this unit's Prim_Rect copies): main's render state, read
 * by address (the lobby has no symbols for it). */
#define gPrimFrame (gSysFrameMode)          /* 0: field (half height) */
extern s32 gSysFbWidth;
#define gPrimScreenW (gSysFbWidth)
extern s32 gSysFbLines;
#define gPrimScreenH (gSysFbLines)
extern f32 gSysZMax;
#define gPrimZ (*(volatile f32 *)&gSysZMax)
extern s32 gSysNoDraw;
#define gPrimOff (gSysNoDraw)
extern u32 gRenderFlags;
#define gPrimFlags (gRenderFlags)
#define GS_RGBAQ(r, g, b, a, q)     ((u64)(r) | ((u64)(g) << 8) | ((u64)(b) << 16) | ((u64)(a) << 24) | ((u64)(q) << 32))
#define GS_XYZ(x, y, z) ((u64)(x) | ((u64)(y) << 16) | ((u64)(z) << 32))
#define GS_UV(u, v) ((u64)(u) | ((u64)(v) << 16))
void Mem_CopyQwords(void *src, void *dst, s32 qwc);  /* main: copies quadwords */
u32 Gs_ColorToGs(u32 color, s32 mode);           /* main */
s32 Prim_SendPacket(u32 *); /* main */
extern u8 gPrimSpriteTmpl[];                         /* the packets' templates: Prim_Sprite's */
extern u8 gPrimRectTmpl[];                         /* Prim_Rect's */
extern u8 gPrimTriTmpl[];                         /* Prim_Tri's */
u8 *strstr(const void *s, const void *sub); /* strstr */
u8 Html_LookupKeyword(char *s, HtmlKeyword *table, u32 size);

extern HtmlKeyword gGameStyleKeywords[12]; /* the <GAME-STYLE> keywords */
extern HtmlKeyword gPageCommentKeywords[43]; /* the page keywords in comments */
extern u8 D_008F8AB0;
char *strncpy(char *dst, const char *src, u32 n); /* main: strncpy */
void Form_SetField1(u8 *arg0);
u8 Html_PageKeyword(u8 *s, HtmlKeyword *tbl, u32 size);
void Html_GameStyle(void);
void Html_Csv(u8 kind);
void *strcpy(char *dst, const char *src);
s32 Html_ParseColor(const char *s);             /* Html_ParseColor */
void Html_SetCharWidths(u8 bold);                  /* Html_SetCharWidths */
s32 Html_CloseTables(u8 **, u8 *); /* Html_CloseTables */
void Layout_FlushBreak(u8 *text);                 /* Layout_FlushBreak */
void Layout_TextFull(u8 *text);                 /* Layout_TextFull */
void Html_NewPage(void);
void Page_SetStyle(u16 flags, char *msg, u8 mode, u8 a3); /* a3 is an s32 in unk_72AD50.c */ /* unit's own declaration: with the definition's void (u16 flags, char *msg, u8 mode, s32 a3) (lobby/unk_72AD50.c), Html_Parse 3 of 300 words differ */
void Page_ExtendU16(u16 x, u16 y);
void Draw_PageButton(u16 x, u16 y, u16 x2, u16 y2, u8 *label);
void Page_SetKind(s8 kind);

extern HtmlTagSet gHtmlModeTags[]; /* the tag set of each mode */
extern u8 gHtmlParseStep;           /* Html_Parse's step */

/* The label of the page's close button (a local array's
 * initializer in .data: gHtmlLabelInit). */
typedef struct HtmlLabel {
    char s[7];
} HtmlLabel;
extern HtmlLabel gHtmlLabelInit;

extern u8 gCharClass[]; /* character classes (bit 0: a Shift-JIS lead byte) */
extern u8 gToUpper[]; /* to upper case */
extern u8 gHexDigitValues[]; /* a digit's value + 1 (0: not a digit) */

s32 Html_ReadQuoted(char *dst, s32 max, s32 skipNewlines);
void Lbs_ParseRow(u8 idx, u8 *s);
void Mem_Swap(void *a, void *b, s32 n);
void Lbs_Sort(void);
s32 Html_ParseSome(u16 maxTags);

/* unk_741900's .rodata (0x8393E0-0x839810) */
#pragma rodata on
char D_008393E0[0x8] = {
#include "lobby/008393E0.inc"
};
char D_008393E8[0x8] = {
#include "lobby/008393E8.inc"
};
char D_008393F0[0x8] = {
#include "lobby/008393F0.inc"
};
char D_008393F8[0x8] = {
#include "lobby/008393F8.inc"
};
char D_00839400[0x8] = {
#include "lobby/00839400.inc"
};
char D_00839408[0x8] = {
#include "lobby/00839408.inc"
};
char D_00839410[0x8] = {
#include "lobby/00839410.inc"
};
char D_00839418[0x8] = {
#include "lobby/00839418.inc"
};
char D_00839420[0x8] = {
#include "lobby/00839420.inc"
};
char D_00839428[0xC] = {
#include "lobby/00839428.inc"
};
char D_00839438[0x8] = {
#include "lobby/00839438.inc"
};
char D_00839440[0x10] = {
#include "lobby/00839440.inc"
};
u32 gPageCommentKeywords_0[4] = {
#include "lobby/00839450.inc"
};
char D_00839460[0x10] = {
#include "lobby/00839460.inc"
};
char D_00839470[0x10] = {
#include "lobby/00839470.inc"
};
char D_00839480[0x10] = {
#include "lobby/00839480.inc"
};
char D_00839490[0x10] = {
#include "lobby/00839490.inc"
};
char D_008394A0[0x10] = {
#include "lobby/008394A0.inc"
};
char D_008394B0[0x10] = {
#include "lobby/008394B0.inc"
};
char D_008394C0[0x10] = {
#include "lobby/008394C0.inc"
};
char D_008394D0[0x10] = {
#include "lobby/008394D0.inc"
};
u32 gPageCommentKeywords_9[4] = {
#include "lobby/008394E0.inc"
};
u32 gPageCommentKeywords_10[4] = {
#include "lobby/008394F0.inc"
};
u32 gPageCommentKeywords_11[4] = {
#include "lobby/00839500.inc"
};
u32 gPageCommentKeywords_12[4] = {
#include "lobby/00839510.inc"
};
u32 gPageCommentKeywords_14[6] = {
#include "lobby/00839520.inc"
};
u32 gPageCommentKeywords_15[3] = {
#include "lobby/00839538.inc"
};
u32 gPageCommentKeywords_16[2] = {
#include "lobby/00839548.inc"
};
u32 gPageCommentKeywords_17[4] = {
#include "lobby/00839550.inc"
};
u32 gPageCommentKeywords_18[8] = {
#include "lobby/00839560.inc"
};
u32 gPageCommentKeywords_19[8] = {
#include "lobby/00839580.inc"
};
u32 gPageCommentKeywords_20[8] = {
#include "lobby/008395A0.inc"
};
u32 gPageCommentKeywords_21[8] = {
#include "lobby/008395C0.inc"
};
u32 gPageCommentKeywords_22[8] = {
#include "lobby/008395E0.inc"
};
u32 gPageCommentKeywords_23[8] = {
#include "lobby/00839600.inc"
};
u32 gPageCommentKeywords_24[8] = {
#include "lobby/00839620.inc"
};
u32 gPageCommentKeywords_25[8] = {
#include "lobby/00839640.inc"
};
u32 gPageCommentKeywords_26[8] = {
#include "lobby/00839660.inc"
};
u32 gPageCommentKeywords_27[8] = {
#include "lobby/00839680.inc"
};
u32 gPageCommentKeywords_28[8] = {
#include "lobby/008396A0.inc"
};
u32 gPageCommentKeywords_29[8] = {
#include "lobby/008396C0.inc"
};
u32 gPageCommentKeywords_30[8] = {
#include "lobby/008396E0.inc"
};
u32 gPageCommentKeywords_31[4] = {
#include "lobby/00839700.inc"
};
u32 gPageCommentKeywords_32[4] = {
#include "lobby/00839710.inc"
};
u32 gPageCommentKeywords_33[4] = {
#include "lobby/00839720.inc"
};
u32 gPageCommentKeywords_34[4] = {
#include "lobby/00839730.inc"
};
u32 gPageCommentKeywords_35[4] = {
#include "lobby/00839740.inc"
};
u32 gPageCommentKeywords_36[8] = {
#include "lobby/00839750.inc"
};
u32 gPageCommentKeywords_37[8] = {
#include "lobby/00839770.inc"
};
u32 gPageCommentKeywords_38[8] = {
#include "lobby/00839790.inc"
};
u32 gPageCommentKeywords_39[6] = {
#include "lobby/008397B0.inc"
};
u32 gPageCommentKeywords_40[3] = {
#include "lobby/008397C8.inc"
};
u32 gPageCommentKeywords_41[8] = {
#include "lobby/008397E0.inc"
};
u32 gPageCommentKeywords_42[4] = {
#include "lobby/00839800.inc"
};
#pragma rodata off

void Prim_Sprite(Sprite *s)
{
    u64 *p;
    s32 x0;
    s32 y0;
    s32 x1;
    s32 y1;
    s32 w;
    s32 h;
    s32 ox;
    s32 oy;
    f32 scale;
    u32 c;

    if (gPrimOff != 0) {
        return;
    }
    ox = ((0x1000 - gPrimScreenW) / 2) << 4;
    oy = ((0x1000 - gPrimScreenH) / 2) << 4;
    scale = (f32)gPrimScreenW / 640.0f;
    x0 = 16.0f * (s->x * scale);
    y0 = s->y << 4;
    w = 16.0f * (s->w * scale);
    h = s->h << 4;
    if (gPrimFrame == 0) {
        y0 >>= 1;
        h >>= 1;
    }
    x0 += ox;
    x1 = x0 + w;
    y0 += oy;
    y1 = y0 + h;
    Mem_CopyQwords(gPrimSpriteTmpl, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x18);
    *p++ = 0x535310;
    p++;
    c = Gs_ColorToGs(s->color, 0);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    if ((gPrimFlags & 0x10000) == 0x10000) {
        *p++ = GS_UV(s->u0 << 4, s->v0 << 4);
        *p++ = GS_XYZ(x0, y0, (u32)gPrimZ);
        *p++ = GS_UV(s->u1 << 4, s->v1 << 4);
        *p++ = GS_XYZ(x1, y1, (u32)gPrimZ);
    } else {
        *p++ = GS_UV((s->u0 << 4) + 8, (s->v0 << 4) + 8);
        *p++ = GS_XYZ(x0, y0, (u32)gPrimZ);
        *p++ = GS_UV((s->u1 << 4) + 8, (s->v1 << 4) + 8);
        *p++ = GS_XYZ(x1, y1, (u32)gPrimZ);
    }
    Prim_SendPacket((u32 *)SCRATCHPAD);
}

void Prim_Rect(PrimLine *r)
{
    u64 *p;
    s32 x0, y0, x1, y1;
    s32 ox, oy;
    f32 scale;
    u32 c;

    if (gPrimOff != 0) {
        return;
    }
    ox = ((0x1000 - gPrimScreenW) / 2) << 4;
    oy = ((0x1000 - gPrimScreenH) / 2) << 4;
    scale = (f32)gPrimScreenW / 640.0f;
    x0 = 16.0f * (r->x0 * scale);
    y0 = r->y0 << 4;
    x1 = 16.0f * (r->x1 * scale);
    y1 = r->y1 << 4;
    if (gPrimFrame == 0) {
        y0 >>= 1;
        y1 >>= 1;
    }
    x0 += ox;
    x1 += ox;
    y0 += oy;
    y1 += oy;
    Mem_CopyQwords(gPrimRectTmpl, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(r->color, 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    *p++ = GS_XYZ(x0, y0, (u32)gPrimZ);
    *p++ = GS_XYZ(x1, y1, (u32)gPrimZ);
    Prim_SendPacket((u32 *)SCRATCHPAD);
}

s32 Prim_Tri(PrimTri *t)
{
    u64 *p;
    s32 x0, x1, y0, y1, x2, y2;
    s32 ox, oy;
    f32 scale;
    u32 c;

    if (gPrimOff != 0) {
        return 1;
    }
    Mem_CopyQwords(gPrimTriTmpl, (void *)SCRATCHPAD, 3);
    p = (u64 *)(SCRATCHPAD + 0x28);
    c = Gs_ColorToGs(t->color, 1);
    *p++ = GS_RGBAQ((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, (c >> 24) & 0xFF, 1);
    ox = ((0x1000 - gPrimScreenW) / 2) << 4;
    oy = ((0x1000 - gPrimScreenH) / 2) << 4;
    scale = (f32)gPrimScreenW / 640.0f;
    x0 = 16.0f * (t->x0 * scale);
    y0 = t->y0 << 4;
    x1 = 16.0f * (t->x1 * scale);
    y1 = t->y1 << 4;
    x2 = 16.0f * (t->x2 * scale);
    y2 = t->y2 << 4;
    if (gPrimFrame == 0) {
        y0 >>= 1;
        y1 >>= 1;
        y2 >>= 1;
    }
    x0 += ox;
    y0 += oy;
    x1 += ox;
    x2 += ox;
    y1 += oy;
    y2 += oy;
    *p++ = GS_XYZ(x0, y0, (u32)gPrimZ);
    *p++ = GS_XYZ(x1, y1, (u32)gPrimZ);
    *p++ = GS_XYZ(x2, y2, (u32)gPrimZ);
    Prim_SendPacket((u32 *)SCRATCHPAD);
    return 1;
}

/* Html_ReadQuoted: the next "..." from the parse position into dst (at
 * most max - 1 characters; newlines dropped when skipNewlines); 0, or 1
 * at a '<' before the quote, -1 at the end of the page. */
s32 Html_ReadQuoted(char *dst, s32 max, s32 skipNewlines)
{
    u8 *p;
    s32 state = 1;
    s32 n;
    u8 c;

    p = gHtml->cur;
    n = 0;

    for (c = *p; c != 0; c = *++p) {
        switch (state) {
        case 1:
            if (c == '"') {
                state = 2;
            } else if (c == '<') {
                return 1;
            }
            break;
        case 2:
            if (c == '"') {
                *dst = 0;
                gHtml->cur = p + 1;
                return 0;
            }
            if (skipNewlines && (c == '\r' || c == '\n')) {
                p++;
            }
            *dst++ = *p;
            n++;
            if (max < n + 1) {
                *dst = 0;
                state = 3;
            }
            break;
        case 3:
            if (c == '"') {
                gHtml->cur = p + 1;
                return 0;
            }
            break;
        }
    }
    return -1;
}

/* Html_ReadQuotedText */
s32 Html_ReadQuotedText(char *dst, s32 max)
{
    return Html_ReadQuoted(dst, max, 1);
}

/* Html_ReadQuotedRaw */
s32 Html_ReadQuotedRaw(char *dst, s32 max)
{
    return Html_ReadQuoted(dst, max, 0);
}

/* Html_PageKeyword: the value of the keyword at s (upper-cased, up to '>')
 * in a table of size bytes; 0 when none. */
u8 Html_PageKeyword(u8 *s, HtmlKeyword *tbl, u32 size)
{
    u8 buf[0x20];
    u16 k;
    u16 n;
    u8 c;

    for (k = 0; k < 30; k++) {
        c = s[k];
        if (c == 0) {
            break;
        }
        if (gCharClass[c] & 1) {
            buf[k] = c;
            k++;
            buf[k] = s[k];
        } else {
            if (c == '>') {
                break;
            }
            buf[k] = gToUpper[c];
        }
    }
    buf[k] = 0;
    n = size >> 3;
    for (k = 0; k < n; k++) {
        if (strncmp(buf, tbl[k].name, tbl[k].size - 1) == 0) {
            return tbl[k].value;
        }
    }
    return 0;
}

/* Html_GameStyle: <GAME-STYLE> "KEYWORD=ON" ... : what the page lets the
 * player do (HtmlState.pageStyle, a bit per keyword), the page's exit number
 * (EXIT=..:n), the back link (BACK=ON:url, or BACK=ON;url;url... chosen
 * by the exit number), the scroll and front label settings. */
void Html_GameStyle(void)
{
    char buf[0x110];
    u8 kind;
    u8 *p;
    u8 on;
    u8 *q;
    s32 i;
    u8 c;

    gHtml->cur += 12;
    for (;;) {
        memset(buf, 0, sizeof(buf));
        if (Html_ReadQuotedText(buf, sizeof(buf)) != 0) {
            return;
        }
        kind = Html_LookupKeyword(buf, gGameStyleKeywords, sizeof(gGameStyleKeywords));
        p = strstr(buf, "=");
        if (p == NULL) {
            continue;
        }
        on = 0;
        p[1] = gToUpper[p[1]];
        p[2] = gToUpper[p[2]];
        if (strncmp(p + 1, "ON", strlen("ON")) == 0) {
            on = 1;
        }
        switch (kind) {
        case 1: /* MOUSE */
            gHtml->pageStyle = (gHtml->pageStyle & ~0x1) | on;
            break;
        case 2: /* SCROLL */
            gHtml->pageStyle = (gHtml->pageStyle & ~0x2) | (on << 1);
            gHtml->scroll = 0;
            if (on == 1 && p[3] == ':') {
                c = p[4];
                if (c >= '0' && c <= '9') {
                    gHtml->scroll = c - '0';
                }
            } else if (on == 0 && p[4] == ':') {
                c = p[5];
                if (c >= '0' && c <= '9') {
                    gHtml->scroll = c - '0';
                }
            }
            break;
        case 3: /* TITLE */
            gHtml->pageStyle = (gHtml->pageStyle & ~0x4) | (on << 2);
            break;
        case 4: /* BACK */
            gHtml->pageStyle = (gHtml->pageStyle & ~0x8) | (on << 3);
            memset(gHtml->backUrl, 0, 0x100);
            if (on != 0) {
                if (p[3] == ':') {
                    memcpy(gHtml->backUrl, p + 4, 0xFF);
                } else if (p[3] == ';') {
                    q = p + 4;
                    for (i = 0; i < gBrowserWork->exitArgs; i++) {
                        p = strstr(q, ";");
                        if (p == NULL) {
                            return;
                        }
                        q = p + 1;
                    }
                    for (i = 0; i < 0xFF; i++) {
                        c = q[i];
                        if (c == ';' || c == 0) {
                            break;
                        }
                        gHtml->backUrl[i] = c;
                    }
                }
            } else if (p[4] == ':') {
                memcpy(gHtml->backUrl, p + 5, 0xFF);
            }
            break;
        case 5: /* FORWARD */
            gHtml->pageStyle = (gHtml->pageStyle & ~0x10) | (on << 4);
            break;
        case 6: /* CANCEL */
            gHtml->pageStyle = (gHtml->pageStyle & ~0x20) | (on << 5);
            break;
        case 7: /* RELOAD */
            gHtml->pageStyle = (gHtml->pageStyle & ~0x40) | (on << 6);
            break;
        case 8: /* LINK_U */
            gHtml->pageStyle = (gHtml->pageStyle & ~0x80) | (on << 7);
            break;
        case 9: /* X_SHOW */
            gHtml->pageStyle = (gHtml->pageStyle & ~0x100) | (on << 8);
            break;
        case 10: /* CHOICE_MV */
            gHtml->pageStyle = (gHtml->pageStyle & ~0x200) | (on << 9);
            break;
        case 0: /* EXIT */
            gBrowserWork->exitArgs = 0;
            if (p[3] == ':') {
                c = p[4];
                if (c >= '0' && c <= '9') {
                    gBrowserWork->exitArgs = c - '0';
                }
            }
            break;
        case 11: /* FRONT_LABEL */
            gHtml->frontLabel = 0;
            if (p[3] == ':') {
                c = p[4];
                if (c >= '0' && c <= '9') {
                    gHtml->frontLabel = c - '0';
                }
            }
            break;
        }
    }
}

/* Lbs_ParseRow: field 8 of lobby server idx, "key,text,value", into its
 * row (the value "0x..." in hex, else decimal). */
void Lbs_ParseRow(u8 idx, u8 *s)
{
    s32 i;
    s32 n;
    s32 v;
    u8 c;

    for (i = 0; i < 12; i++) {
        c = s[i];
        if (c == ',') {
            break;
        }
        if (c == 0) {
            return;
        }
        gLobbyServers.row[idx].key[i] = c;
    }
    if (s[i] != ',') {
        for (; i < 0x100; i++) {
            c = s[i];
            if (c == ',') {
                break;
            }
            if (c == 0) {
                return;
            }
        }
    }
    i++;
    n = 0;
    while (i < 0x100 && n < 0xF4) {
        c = s[i];
        if (gCharClass[c] & 1) {
            gLobbyServers.row[idx].text[n++] = c;
            gLobbyServers.row[idx].text[n++] = s[i + 1];
            i += 2;
        } else if (c == ',') {
            break;
        } else if (c == 0) {
            return;
        } else {
            gLobbyServers.row[idx].text[n] = c;
            i++;
            n++;
        }
    }
    if (s[i] != ',') {
        return;
    }
    i++;
    v = 0;
    n = 0;
    if (s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        for (i += 2; i < 0x100 || n < 8; i++, n++) {
            v = v * 16 + (gHexDigitValues[s[i]] - 1);
            if (s[i + 1] == 0) {
                break;
            }
        }
    } else {
        for (; i < 0x100 || n < 11; i++, n++) {
            v *= 10;
            v += gHexDigitValues[s[i]] - 1;
            if (s[i + 1] == 0) {
                break;
            }
        }
    }
    gLobbyServers.row[idx].value = v;
}

/* Mem_Swap: swaps n bytes (at most 0x110). */
void Mem_Swap(void *a, void *b, s32 n)
{
    u8 tmp[0x110];

    memcpy(tmp, a, n);
    memcpy(a, b, n);
    memcpy(b, tmp, n);
}

/* Lbs_Sort: sorts the lobby servers by key (a bubble sort of their
 * order, then the rows and their MMBB data swapped into place). */
void Lbs_Sort(void)
{
    LbsRow tmp;
    u8 order[0x30];
    u8 i;
    u8 j;
    u8 k;

    for (i = 0; i < gLobbyServers.count; i++) {
        order[i] = i;
    }
    for (i = 0; i < gLobbyServers.count - 1; i++) {
        for (j = 0; j < gLobbyServers.count - i - 1; j++) {
            if (strcmp(gLobbyServers.row[order[j]].key, gLobbyServers.row[order[j + 1]].key) > 0) {
                k = order[j];
                order[j] = order[j + 1];
                order[j + 1] = k;
            }
        }
    }
    for (i = 0; i < gLobbyServers.count; i++) {
        k = order[i];
        tmp = gLobbyServers.row[i];
        gLobbyServers.row[i] = gLobbyServers.row[k];
        gLobbyServers.row[k] = tmp;
        Mem_Swap(gMmbbData->f7[i], gMmbbData->f7[k], sizeof(gMmbbData->f7[0]));
        Mem_Swap(gMmbbData->f12[i], gMmbbData->f12[k], sizeof(gMmbbData->f12[0]));
        Mem_Swap(gMmbbData->f13[i], gMmbbData->f13[k], sizeof(gMmbbData->f13[0]));
        Mem_Swap(gMmbbData->f8[i], gMmbbData->f8[k], sizeof(gMmbbData->f8[0]));
        Mem_Swap(gMmbbData->f9[i], gMmbbData->f9[k], sizeof(gMmbbData->f9[0]));
        for (j = i + 1; j < gLobbyServers.count; j++) {
            if (order[j] == i) {
                order[j] = order[i];
            }
        }
    }
}

/* Html_Csv: the page's <CSV> block: the quoted items after it, by the
 * page's kind (HtmlState.csvKind), into the MMBB data's fields (and, for
 * the lobby servers' page, their rows). */
void Html_Csv(u8 kind)
{
    char buf[0x310];
    u8 n;

    gHtml->cur += 5;
    switch (kind) {
    case 1:
        Page_SetDataMode(kind);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(6, 0, buf);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(2, 0, buf);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(4, 0, buf);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(5, 0, buf);
        break;
    case 2:
        Page_SetDataMode(kind);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(6, 0, buf);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(1, 0, buf);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(2, 0, buf);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(4, 0, buf);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(5, 0, buf);
        break;
    case 4:
        memset(gLobbyServers.row, 0, sizeof(gLobbyServers.row));
        gLobbyServers.count = 0;
        gHtml->sortServers = 1;
        Page_SetDataMode(kind);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(6, 0, buf);
        if (strncmp(buf, "OK", 2) == 0) {
            n = 0;
            while (Html_ReadQuotedText(buf, 0x100) == 0 && n < 40) {
                n++;
                Mmbb_SetField(7, n - 1, buf);
                if (Html_ReadQuotedText(buf, 0x100) != 0) {
                    break;
                }
                Mmbb_SetField(12, n - 1, buf);
                gLobbyServers.row[n - 1].users = atoi(buf);
                if (Html_ReadQuotedText(buf, 0x100) != 0) {
                    break;
                }
                Mmbb_SetField(13, n - 1, buf);
                gLobbyServers.row[n - 1].capacity = atoi(buf);
                if (Html_ReadQuotedText(buf, 0x100) != 0) {
                    break;
                }
                Mmbb_SetField(8, n - 1, buf);
                Lbs_ParseRow(n - 1, (u8 *)buf);
                if (Html_ReadQuotedRaw(buf, 0x100) != 0) {
                    break;
                }
                Mmbb_SetField(9, n - 1, buf);
                if (Html_ReadQuotedText(buf, 0x100) != 0) {
                    break;
                }
            }
            gLobbyServers.count = n;
        } else {
            if (Html_ReadQuotedText(buf, 0x100) != 0) {
                break;
            }
            Mmbb_SetField(11, 0, buf);
        }
        break;
    case 5:
        Page_SetDataMode(kind);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(6, 0, buf);
        if (strncmp(buf, "OK", 2) == 0) {
            break;
        }
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(11, 0, buf);
        break;
    case 6:
        if (gHtml->csvArmed != 0) {
            gHtml->csvArmed = 0;
            Page_SetDataMode(kind);
            if (Html_ReadQuotedText(buf, 0x100) != 0) {
                break;
            }
            Mmbb_SetField(14, 0, buf);
            if (Html_ReadQuotedText(buf, 0x100) != 0) {
                break;
            }
            Mmbb_SetField(15, 0, buf);
            return;
        }
        Page_SetDataMode(kind + 1);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(16, 0, buf);
        if (strncmp(buf, "00", 2) == 0) {
            break;
        }
        n = 0;
        while (Html_ReadQuotedText(buf, 0x100) == 0 && n < 10) {
            n++;
            Mmbb_SetField(17, n - 1, buf);
            if (Html_ReadQuotedText(buf, 0x100) != 0) {
                break;
            }
            Mmbb_SetField(18, n - 1, buf);
            if (Html_ReadQuotedText(buf, 0x100) != 0) {
                break;
            }
            Mmbb_SetField(19, n - 1, buf);
        }
        break;
    case 7:
        if (gPortalPage == 2) {
            Page_SetDataMode(5);
        }
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(6, 0, buf);
        if (Html_ReadQuotedText(buf, 0x100) != 0) {
            break;
        }
        Mmbb_SetField(11, 0, buf);
        break;
    }
    gHtml->csvKind = 0;
}

/* Html_ParseTag: at a '<' or '&' (or inside a tag), looks the upper-cased
 * text up in the tag set: moves *cur past the tag's name, sets the mode
 * (or the comment flag) and calls the tag's handler. 0 when a tag was
 * handled (or an unknown tag skipped), 1 when the text is not a tag, -1
 * on an error. */
s32 Html_ParseTag(u8 **cur, HtmlTagSet *set, u8 *text)
{
    u8 name[0x10];
    HtmlTag *t;
    s32 i;
    s32 k;
    u8 *p;

    switch (gHtml->parseState) {
    case 0:
        if (**cur != '<' && **cur != '&') {
            break;
        }
        gHtml->parseState = 1;
    case 1:
        t = set->tags;
        for (k = 0; k < 13 && (*cur)[k] != 0; k++) {
            name[k] = gToUpper[(*cur)[k]];
        }
        for (i = 0; i < set->count; i++, t++) {
            if (strncmp(name, t->name, t->size - 1) == 0) {
                gHtml->tagCount++;
                if (t->next != 0) {
                    gHtml->mode = t->next;
                } else {
                    gHtml->inComment = 1;
                }
                *cur += t->size - 1;
                if (t->handler(cur, text) < 0) {
                    return -1;
                }
                {
                    HtmlState *st = gHtml;
                    s32 m = st->mode;

                    if (m == 2 || m == 0x12) {
                        st->parseState = 0;
                    } else {
                        st->parseState = 2;
                    }
                }
                return 0;
            }
        }
        gHtml->parseState = 0;
        if (i == set->count && **cur != '&') {
            p = strstr(*cur, ">");
            if (p != NULL) {
                *cur = p + 1;
                return 0;
            }
            return -1;
        }
        break;
    default:
        t = set->tags;
        for (k = 0; k < 13 && (*cur)[k] != 0; k++) {
            name[k] = gToUpper[(*cur)[k]];
        }
        for (i = 0; i < set->count; i++, t++) {
            if (strncmp(name, t->name, t->size - 1) == 0) {
                if (t->next != 0) {
                    gHtml->mode = t->next;
                } else {
                    gHtml->inComment = 1;
                }
                *cur += t->size - 1;
                if (t->handler(cur, text) < 0) {
                    return -1;
                }
                if (gHtml->mode == 2) {
                    gHtml->parseState = 0;
                }
                return 0;
            }
        }
        break;
    }
    return 1;
}

/* Html_Comment: a comment of the page. <!--MMBB-FLAG-PASS-n-->: with 2
 * (and D_008F8AB0 clear) the page skips to <!--/MMBB-FLAG-PASS-->.
 * Measuring (measure 1), a <GAME-STYLE> before the comment's end; else
 * the page keyword at the comment's start (gPageCommentKeywords: the page's kind
 * in pageKind, the input page.s in accessKey, the <CSV> kind in csvKind) and,
 * with a <CSV> kind, a <CSV> before the comment's end. */
void Html_Comment(void)
{
    char id[0x10];
    s32 skip = 0;
    u8 *p;
    u8 k;
    s32 c;

    p = gHtml->cur;
    if (*p == 'M' && strncmp(p, "MMBB-FLAG-PASS", 14) == 0) {
        gHtml->cur += 14;
        p = gHtml->cur;
        switch (p[1]) {
        case '1':
            skip = 0;
            break;
        case '2':
            if (D_008F8AB0 != 0) {
                skip = 0;
            } else {
                skip = 1;
            }
            break;
        case '3':
            skip = 0;
            break;
        }
        if (skip) {
            p = strstr(p, "<!--/MMBB-FLAG-PASS-->");
            if (p != NULL) {
                gHtml->cur = p + 19;
            }
        }
    } else if (gHtml->measure == 1) {
        while ((c = *(p = gHtml->cur)) != 0) {
            if (c == '-') {
                if (strncmp(p, "-->", 3) == 0) {
                    return;
                }
            } else if (c == '<') {
                if (strncmp(p, "<GAME-STYLE>", 12) == 0) {
                    Html_GameStyle();
                }
                break;
            }
            gHtml->cur++;
        }
    } else if (gHtml->measure == 0) {
        k = Html_PageKeyword(gHtml->cur, gPageCommentKeywords, sizeof(gPageCommentKeywords));
        switch (k) {
        case 1:
        case 2:
        case 4:
            gHtml->pageButton = 1;
        case 3:
        case 5:
        case 6:
        case 7:
        case 0x12:
        case 0x29:
        case 0x2A:
        case 0x2B:
            gHtml->pageKind = k;
            break;
        case 8:
        case 0xA:
        case 0xB:
        case 0x28:
        case 0x2C:
        case 0x2D:
        case 0x2E:
        case 0x2F:
            gHtml->accessKey = k;
            break;
        case 9:
            if (gHtml->pageKind == 4) {
                gHtml->accessKey = k;
                strncpy(id, (char *)gHtml->cur + gPageCommentKeywords[8].size + 2, 10);
                id[10] = 0;
                Form_SetField1((u8 *) id);
            }
            break;
        case 0xC:
            gHtml->pageButton = 1;
            break;
        case 0xD:
        case 0xE:
        case 0x10:
        case 0x11:
            gHtml->csvKind = k - 0xC;
            break;
        case 0x13:
            gHtml->csvKind = 6;
            gHtml->pageKind = k;
            break;
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
        case 0x18:
        case 0x19:
        case 0x1A:
        case 0x1B:
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
        case 0x20:
            break;
        case 0x21:
            gHtml->csvKind = 7;
            gHtml->pageKind = k;
            break;
        case 0x62:
            if (gHtml->csvKind == 1) {
                gHtml->csvKind = 2;
            }
            break;
        case 0x63:
            gHtml->csvArmed = 1;
            break;
        case 0:
            if (gHtml->csvKind == 0) {
                break;
            }
            while ((c = *(p = gHtml->cur)) != 0) {
                if (c == '-') {
                    if (strncmp(p, "-->", 3) == 0) {
                        return;
                    }
                } else if (c == '<') {
                    if (strncmp(p, "<CSV>", 5) == 0) {
                        Html_Csv(gHtml->csvKind);
                    }
                    break;
                }
                gHtml->cur++;
            }
            break;
        }
    }
}

/* Html_Reset: the parser's and layout's state for a pass over the page
 * (measure: -10 the first, measuring pass, 1 the second, 0 the drawing
 * pass). */
void Html_Reset(s8 measure)
{
    gHtml->parseState = 0;
    gHtml->measure = measure;
    gHtml->lineNo = 0;
    gHtml->link = 0;
    gHtml->mode = 0;
    gHtml->inComment = 0;
    gHtml->textLen = 0;
    gHtml->text[0] = 0;
    gHtml->pen.x = 0;
    gHtml->pen.runX = 0;
    gHtml->pen.y = 0;
    gHtml->pen.runY = 0;
    gHtml->selectWidth = 0;
    gHtml->pen.runW = 0;
    gHtml->pen.lineUsed = 0;
    gHtml->lineLen = 0;
    gHtml->cellCount = 0;
    gHtml->tableDepth = 0;
    gHtml->curCell = 0;
    gHtml->unkD896 = 0;
    gHtml->flags = 0;
    gHtml->pre = 0;
    gHtml->paragraph = 0;
    gHtml->center = 0;
    gHtml->bold = 0;
    gHtml->underline = 0;
    gHtml->strike = 0;
    gHtml->indent = 0;
    strcpy(gHtml->textColor, "#000000");
    strcpy(gHtml->linkColor, "#0000FF");
    strcpy(gHtml->bgColor, "#FFFFFF");
    strcpy(gHtml->background, "");
    gHtml->fontDepth = 0;
    gHtml->fontSize[gHtml->fontDepth] = 3;
    gHtml->fontColor[gHtml->fontDepth] = Html_ParseColor(gHtml->textColor);
    gHtml->bgColorValue = Html_ParseColor(gHtml->bgColor);
    gHtml->linkColorValue = Html_ParseColor(gHtml->linkColor);
    Html_SetCharWidths(gHtml->bold);
    memset(&gHtml->meta, 0, sizeof(gHtml->meta));
    gHtml->pageKind = 0;
    gHtml->accessKey = 0;
    gHtml->pageButton = 0;
    gHtml->csvKind = 0;
    gHtml->csvArmed = 0;
    memset(gHtml->mapName, 0, sizeof(gHtml->mapName));
    gHtml->sortServers = 0;
}

/* Html_ParseSome: runs the parser from cur until the page's end (0) or
 * until maxTags tags were handled (1); -1 on an error. Text between tags
 * (in <PRE> with its tabs and newlines) collects in text[]. */
s32 Html_ParseSome(u16 maxTags)
{
    HtmlTagSet *set;
    s32 kind;
    s32 r;
    u8 c;

    gHtml->tagCount = 0;
    while (*gHtml->cur != 0) {
        if (gHtml->inComment) {
            Html_Comment();
            gHtml->commentEnd = (char *)strstr(gHtml->cur, "-->");
            if (gHtml->commentEnd != NULL) {
                gHtml->inComment = 0;
                gHtml->cur = (u8 *)gHtml->commentEnd + 3;
            } else {
                gHtml->cur += strlen((char *)gHtml->cur);
            }
        } else {
            set = &gHtmlModeTags[gHtml->mode];
            kind = set->unk8;
            r = Html_ParseTag(&gHtml->cur, set, gHtml->text);
            if (r != 0) {
                if (r < 0) {
                    Html_CloseTables(&gHtml->cur, gHtml->text);
                    return -1;
                }
                if (kind == 1) {
                    c = *gHtml->cur;
                    if (c != '\r') {
                        if (gHtml->pre) {
                            if (c == '\n') {
                                Layout_FlushBreak(gHtml->text);
                            } else if (c == '\t') {
                                if (gHtml->textLen > 0x7A) {
                                    Layout_TextFull(gHtml->text);
                                }
                                gHtml->text[gHtml->textLen++] = 7;
                                gHtml->text[gHtml->textLen++] = 7;
                                gHtml->text[gHtml->textLen++] = 7;
                                gHtml->text[gHtml->textLen++] = 7;
                                gHtml->text[gHtml->textLen] = 0;
                            } else {
                                if (gHtml->textLen > 0x7D) {
                                    Layout_TextFull(gHtml->text);
                                }
                                gHtml->text[gHtml->textLen++] = *gHtml->cur;
                                gHtml->text[gHtml->textLen] = 0;
                            }
                        } else if (c != '\n') {
                            if (gHtml->textLen > 0x7D) {
                                Layout_TextFull(gHtml->text);
                            }
                            gHtml->text[gHtml->textLen++] = *gHtml->cur;
                            gHtml->text[gHtml->textLen] = 0;
                        }
                    }
                }
                gHtml->cur++;
            }
        }
        if (gHtml->tagCount >= maxTags) {
            return 1;
        }
    }
    Html_CloseTables(&gHtml->cur, gHtml->text);
    return 0;
}

/* Html_Parse: lays out and draws the page in steps (gHtmlParseStep: 0 start,
 * 1 the measuring pass, 2 the second pass, 3 the page's frame, 4 the
 * drawing pass), at most maxTags tags per call: 0 while measuring, 1
 * while drawing, 2 when done (or no page). */
s32 Html_Parse(u8 *page, u16 maxTags, u8 inFrame)
{
    HtmlLabel label;
    u16 extra;
    u16 w;

    if (page == NULL) {
        return 2;
    }
    switch (gHtmlParseStep) {
    case 0:
        gHtml = (HtmlState *)gBrowserWork->buf64k;
        memset(gHtml, 0, sizeof(HtmlState));
        gHtml->pageWidth = 0x248;
        gHtml->width = 0x248;
        gHtml->cur = page;
        Html_Reset(-10);
        gHtml->inFrame = inFrame;
        gHtml->pageStyle = 0x2E7;
        gHtmlParseStep = 1;
    case 1:
        if (Html_ParseSome(maxTags) > 0) {
            return 0;
        }
        if (gHtml->pageWidth < 0x248) {
            gHtml->pageWidth = 0x248;
        }
        memset(gHtml->lines, 0, sizeof(gHtml->lines));
        gHtml->bottom = 0x17C;
        gHtml->width = gHtml->pageWidth;
        gHtmlParseStep = 2;
        gHtml->cur = page;
        Html_Reset(1);
    case 2:
        if (Html_ParseSome(maxTags) > 0) {
            return 0;
        }
        gHtmlParseStep = 3;
        gHtml->cur = page;
        Html_Reset(0);
        return 1;
    case 3:
        if (gHtml->inFrame == 0) {
            Html_NewPage();
            Page_SetStyle(gHtml->pageStyle, (char *)gHtml->backUrl, gHtml->frontLabel, gHtml->scroll);
        }
        gHtmlParseStep = 4;
    case 4:
        label = gHtmlLabelInit;
        extra = 0;
        if (Html_ParseSome(maxTags) > 0) {
            return 1;
        }
        if (gHtml->measure == 0 && gHtml->pageButton != 0 && gHtml->inFrame == 0) {
            w = strlen(label.s) * 8 + 8;
            Draw_PageButton(0x10, gHtml->bottom, w + 0x10, gHtml->bottom + 0x14, (u8 *)label.s);
            extra = 0x28;
        }
        if (gHtml->inFrame == 0) {
            Page_ExtendU16(gHtml->width, gHtml->bottom + extra);
        }
        if (gHtml->measure == 0) {
            switch (gHtml->pageKind) {
            case 0x12:
                Page_SetKind(0);
                break;
            case 0x13:
                Page_SetKind(1);
                break;
            case 3:
                Page_SetKind(2);
                break;
            case 4:
                Page_SetKind(3);
                break;
            case 6:
                Page_SetKind(4);
                break;
            case 7:
                Page_SetKind(5);
                break;
            case 1:
                Page_SetKind(6);
                break;
            case 0x29:
                Page_SetKind(7);
                break;
            case 0x2A:
                Page_SetKind(8);
                break;
            case 0x2B:
                Page_SetKind(9);
                break;
            }
            if (gHtml->sortServers != 0) {
                Lbs_Sort();
            }
        }
        break;
    }
    return 2;
}

/* unk_741900's .data (0x7EEE30-0x7EFE70) */
extern char D_008393E0[];
extern char D_008393E8[];
extern char D_008393F0[];
extern char D_008393F8[];
extern char D_00839400[];
extern char D_00839408[];
extern char D_00839410[];
extern char D_00839418[];
extern char D_00839420[];
extern char D_00839428[];
extern char D_00839438[];
extern char D_00839440[];
extern char D_00839460[];
extern char D_00839470[];
extern char D_00839480[];
extern char D_00839490[];
extern char D_008394A0[];
extern char D_008394B0[];
extern char D_008394C0[];
extern char D_008394D0[];
extern char D_00839908[];
extern char D_00839910[];
extern char D_00839918[];
extern char D_00839920[];
extern char D_00839928[];
extern char D_00839930[];
extern char D_00839938[];
extern char D_00839940[];
extern char D_00839948[];
extern char D_00839950[];
extern char D_00839958[];
extern char D_00839960[];
extern char D_00839968[];
extern char D_00839970[];
extern char D_00839978[];
extern char D_00839980[];
extern char D_00839988[];
extern char D_00839990[];
extern char D_00839998[];
extern char D_008399A0[];
extern char D_008399A8[];
extern char D_008399B0[];
extern char D_008399B8[];
extern char D_008399C0[];
extern char D_008399D0[];
extern char D_008399D8[];
extern char D_008399E0[];
extern char D_008399E8[];
extern char D_008399F0[];
extern char D_008399F8[];
extern char D_00839A00[];
extern char D_00839A10[];
extern char D_00839A20[];
extern char D_00839A28[];
extern char D_00839A30[];
extern char D_00839A38[];
extern char D_00839A40[];
extern char D_00839A48[];
extern char D_00839A50[];
extern char D_00839A58[];
extern char D_00839A60[];
extern char D_00839A68[];
extern char D_00839A70[];
extern char D_00839A78[];
extern char D_00839A80[];
extern char D_00839A88[];
extern char D_00839A90[];
extern char D_00839A98[];
extern char D_00839AA0[];
extern char D_00839AB0[];
extern char D_00839AC0[];
extern char D_00839AD0[];
extern char D_00839AE0[];
extern char D_00839AF0[];
extern char D_00839B00[];
extern char D_00839B08[];
extern char D_00839B18[];
extern char D_00839B28[];
extern char D_00839B30[];
extern char D_00839B38[];
extern char D_00839B40[];
extern char D_00839B48[];
extern char D_00839B58[];
extern char D_00839B68[];
extern char D_00839B70[];
extern char D_00839B78[];
extern char D_00839B80[];
extern char D_00839B88[];
extern char D_00839B90[];
extern char D_00839B98[];
extern char D_00839BA0[];
extern char D_00839BA8[];
extern char D_00839BB0[];
extern char D_00839BB8[];
extern char D_00839BC0[];
extern char D_00839BC8[];
extern char D_00839BD0[];
extern char D_00839BD8[];
extern char D_00839BE0[];
extern char D_00839BE8[];
extern char D_00839BF0[];
extern char D_00839BF8[];
extern char D_00839C00[];
extern char D_00839C08[];
extern char D_00839C10[];
extern char D_00839C18[];
extern char D_00839C20[];
extern char D_00839C30[];
extern char D_00839C38[];
extern char D_00839C40[];
extern char D_00839C48[];
extern char D_00839C50[];
extern char D_00839C58[];
extern char D_00839C68[];
extern char D_00839C70[];
extern char D_00839C78[];
extern char D_00839C80[];
extern char D_00839C88[];
extern char D_00839C90[];
extern char D_00839C98[];
extern char D_00839CA0[];
extern char D_00839CA8[];
extern char D_00839CB0[];
extern char D_00839CB8[];
extern char D_00839CC0[];
extern char D_00839CC8[];
extern char D_00839CD0[];
extern char D_00839CE0[];
extern char D_00839CF0[];
extern char D_00839CF8[];
extern char D_00839D08[];
extern char D_00839D10[];
extern char D_00839D20[];
extern char D_00839D30[];
extern char D_00839D38[];
extern char D_00839D40[];
extern char D_00839D48[];
extern char D_00839D58[];
extern char D_00839D60[];
extern char D_00839D68[];
extern char D_00839D70[];
extern char D_00839D78[];
extern char D_00839D88[];
extern char D_00839D90[];
extern char D_00839D98[];
extern char D_00839DA0[];
extern char D_00839DA8[];
extern char D_00839DB0[];
extern char D_00839DB8[];
extern char D_00839DC0[];
extern char D_00839DC8[];
extern char D_00839DD0[];
extern char D_00839DD8[];
extern char D_00839DE8[];
extern char D_00839DF0[];
extern char D_00839DF8[];
extern char D_00839E08[];
s32 Html_AttrAHref(u8 **cur, u8 *text);
s32 Html_AttrAName(u8 **cur, u8 *text);
s32 Html_AttrAreaAlt(u8 **cur, u8 *text);
s32 Html_AttrAreaCoords(u8 **cur, u8 *text);
s32 Html_AttrAreaHref(u8 **cur, u8 *text);
s32 Html_AttrAreaShape(u8 **cur, u8 *text);
s32 Html_AttrBaseHref(u8 **cur, u8 *text);
s32 Html_AttrBodyBackground(u8 **cur, u8 *text);
s32 Html_AttrBodyBgcolor(u8 **cur, u8 *text);
s32 Html_AttrBodyLink(u8 **cur, u8 *text);
s32 Html_AttrBodyText(u8 **cur, u8 *text);
s32 Html_AttrDivAlign(u8 **cur, u8 *text);
s32 Html_AttrFontColor(u8 **cur, u8 *text);
s32 Html_AttrFontSize(u8 **cur, u8 *text);
s32 Html_AttrFormAction(u8 **cur, u8 *text);
s32 Html_AttrFormMethod(u8 **cur, u8 *text);
s32 Html_AttrHrSize(u8 **cur, u8 *text);
s32 Html_AttrHrWidth(u8 **cur, u8 *text);
s32 Html_AttrImgAlt(u8 **cur, u8 *text);
s32 Html_AttrImgHeight(u8 **cur, u8 *text);
s32 Html_AttrImgSrc(u8 **cur, u8 *text);
s32 Html_AttrImgUsemap(u8 **cur, u8 *text);
s32 Html_AttrImgWidth(u8 **cur, u8 *text);
s32 Html_AttrInputAlt(u8 **cur, u8 *text);
s32 Html_AttrInputChecked(u8 **cur, u8 *text);
s32 Html_AttrInputHeight(u8 **cur, u8 *text);
s32 Html_AttrInputMaxlength(u8 **cur, u8 *text);
s32 Html_AttrInputName(u8 **cur, u8 *text);
s32 Html_AttrInputSize(u8 **cur, u8 *text);
s32 Html_AttrInputSrc(u8 **cur, u8 *text);
s32 Html_AttrInputType(u8 **cur, u8 *text);
s32 Html_AttrInputValue(u8 **cur, u8 *text);
s32 Html_AttrInputWidth(u8 **cur, u8 *text);
s32 Html_AttrMapName(u8 **cur, u8 *text);
s32 Html_AttrMetaContent(u8 **cur, u8 *text);
s32 Html_AttrMetaHttpEquiv(u8 **cur, u8 *text);
s32 Html_AttrMetaUrl(u8 **cur, u8 *text);
s32 Html_AttrOptionSelected(u8 **cur, u8 *text);
s32 Html_AttrOptionValue(u8 **cur, u8 *text);
s32 Html_AttrSelectName(u8 **cur, u8 *text);
s32 Html_AttrTableBackground(u8 **cur, u8 *text);
s32 Html_AttrTableBgcolor(u8 **cur, u8 *text);
s32 Html_AttrTableBorder(u8 **cur, u8 *text);
s32 Html_AttrTableBordercolor(u8 **cur, u8 *text);
s32 Html_AttrTableCellpadding(u8 **cur, u8 *text);
s32 Html_AttrTableCellspacing(u8 **cur, u8 *text);
s32 Html_AttrTableHeight(u8 **cur, u8 *text);
s32 Html_AttrTableWidth(u8 **cur, u8 *text);
s32 Html_AttrTdAlign(u8 **cur, u8 *text);
s32 Html_AttrTdBackground(u8 **cur, u8 *text);
s32 Html_AttrTdBgcolor(u8 **cur, u8 *text);
s32 Html_AttrTdColspan(u8 **cur, u8 *text);
s32 Html_AttrTdHeight(u8 **cur, u8 *text);
s32 Html_AttrTdNowrap(u8 **cur, u8 *text);
s32 Html_AttrTdRowspan(u8 **cur, u8 *text);
s32 Html_AttrTdValign(u8 **cur, u8 *text);
s32 Html_AttrTdWidth(u8 **cur, u8 *text);
s32 Html_AttrTrAlign(u8 **cur, u8 *text);
s32 Html_AttrTrBgcolor(u8 **cur, u8 *text);
s32 Html_AttrTrValign(u8 **cur, u8 *text);
s32 Html_EntityAmp(u8 **cur, u8 *text);
s32 Html_EntityApos(u8 **cur, u8 *text);
s32 Html_EntityGt(u8 **cur, u8 *text);
s32 Html_EntityLt(u8 **cur, u8 *text);
s32 Html_EntityNbsp(u8 **cur, u8 *text);
s32 Html_EntityQuot(u8 **cur, u8 *text);
s32 Html_TagA(u8 **cur, u8 *text);
s32 Html_TagAEnd(u8 **cur, u8 *text);
s32 Html_TagArea(u8 **cur, u8 *text);
s32 Html_TagAreaEnd(u8 **cur, u8 *text);
s32 Html_TagB(u8 **cur, u8 *text);
s32 Html_TagBig(u8 **cur, u8 *text);
s32 Html_TagBody(u8 **cur, u8 *text);
s32 Html_TagBodyEnd(u8 **cur, u8 *text);
s32 Html_TagBr(u8 **cur, u8 *text);
s32 Html_TagCenter(u8 **cur, u8 *text);
s32 Html_TagDd(u8 **cur, u8 *text);
s32 Html_TagDiv(u8 **cur, u8 *text);
s32 Html_TagDl(u8 **cur, u8 *text);
s32 Html_TagDropText(u8 **cur, u8 *text);
s32 Html_TagDt(u8 **cur, u8 *text);
s32 Html_TagEndA(u8 **cur, u8 *text);
s32 Html_TagEndB(u8 **cur, u8 *text);
s32 Html_TagEndCenter(u8 **cur, u8 *text);
s32 Html_TagEndDiv(u8 **cur, u8 *text);
s32 Html_TagEndDl(u8 **cur, u8 *text);
s32 Html_TagEndFont(u8 **cur, u8 *text);
s32 Html_TagEndForm(u8 **cur, u8 *text);
s32 Html_TagEndH(u8 **cur, u8 *text);
s32 Html_TagEndHtml(u8 **cur, u8 *text);
s32 Html_TagEndMap(u8 **cur, u8 *text);
s32 Html_TagEndOption(u8 **cur, u8 *text);
s32 Html_TagEndP(u8 **cur, u8 *text);
s32 Html_TagEndPre(u8 **cur, u8 *text);
s32 Html_TagEndS(u8 **cur, u8 *text);
s32 Html_TagEndSelect(u8 **cur, u8 *text);
s32 Html_TagEndTable(u8 **cur, u8 *text);
s32 Html_TagEndTd(u8 **cur, u8 *text);
s32 Html_TagEndTitle(u8 **cur, u8 *text);
s32 Html_TagEndTr(u8 **cur, u8 *text);
s32 Html_TagEndU(u8 **cur, u8 *text);
s32 Html_TagFlush(u8 **cur, u8 *text);
s32 Html_TagFont(u8 **cur, u8 *text);
s32 Html_TagFontAttrs(u8 **cur, u8 *text);
s32 Html_TagFontEnd(u8 **cur, u8 *text);
s32 Html_TagForm(u8 **cur, u8 *text);
s32 Html_TagFormEnd(u8 **cur, u8 *text);
s32 Html_TagH(u8 **cur, u8 *text);
s32 Html_TagHr(u8 **cur, u8 *text);
s32 Html_TagHrAttrs(u8 **cur, u8 *text);
s32 Html_TagHrEnd(u8 **cur, u8 *text);
s32 Html_TagHtml(u8 **cur, u8 *text);
s32 Html_TagIgnore(u8 **cur, u8 *text);
s32 Html_TagImg(u8 **cur, u8 *text);
s32 Html_TagImgAttrs(u8 **cur, u8 *text);
s32 Html_TagImgEnd(u8 **cur, u8 *text);
s32 Html_TagInput(u8 **cur, u8 *text);
s32 Html_TagInputEnd(u8 **cur, u8 *text);
s32 Html_TagMeta(u8 **cur, u8 *text);
s32 Html_TagMetaEnd(u8 **cur, u8 *text);
s32 Html_TagOption(u8 **cur, u8 *text);
s32 Html_TagOptionAttrs(u8 **cur, u8 *text);
s32 Html_TagP(u8 **cur, u8 *text);
s32 Html_TagPre(u8 **cur, u8 *text);
s32 Html_TagS(u8 **cur, u8 *text);
s32 Html_TagSelect(u8 **cur, u8 *text);
s32 Html_TagSelectAttrs(u8 **cur, u8 *text);
s32 Html_TagSelectEnd(u8 **cur, u8 *text);
s32 Html_TagSmall(u8 **cur, u8 *text);
s32 Html_TagTable(u8 **cur, u8 *text);
s32 Html_TagTableAttrs(u8 **cur, u8 *text);
void Html_TagTableEnd(u8 **cur, u8 *text);
s32 Html_TagTd(u8 **cur, u8 *text);
s32 Html_TagTdAttrs(u8 **cur, u8 *text);
s32 Html_TagTdEnd(u8 **cur, u8 *text);
s32 Html_TagTh(u8 **cur, u8 *text);
s32 Html_TagThAttrs(u8 **cur, u8 *text);
s32 Html_TagTr(u8 **cur, u8 *text);
s32 Html_TagTrAttrs(u8 **cur, u8 *text);
s32 Html_TagTrEnd(u8 **cur, u8 *text);
s32 Html_TagU(u8 **cur, u8 *text);
u8 gPrimSpriteTmpl[80] = {
#include "lobby/007EEE30.inc"
};
u8 gPrimRectTmpl[64] = {
#include "lobby/007EEE80.inc"
};
u8 gPrimTriTmpl[80] = {
#include "lobby/007EEEC0.inc"
};
HtmlKeyword gGameStyleKeywords[12] = {
#include "lobby/007EEF10.inc"
};
HtmlKeyword gPageCommentKeywords[43] = {
#include "lobby/007EEF70.inc"
};
HtmlLabel gHtmlLabelInit = {
#include "lobby/007EF0C8.inc"
};
u32 gHtmlHeadingSizes[4] = {
#include "lobby/007EF0D0.inc"
};
u32 gFullCharWidths[4] = {
#include "lobby/007EF0E0.inc"
};
u32 gHalfCharWidths[4] = {
#include "lobby/007EF0F0.inc"
};
u32 gHtmlModeTags_0[48] = {
#include "lobby/007EF100.inc"
};
u32 gHtmlModeTags_2[356] = {
#include "lobby/007EF1C0.inc"
};
u32 gHtmlModeTags_24[20] = {
#include "lobby/007EF750.inc"
};
u32 gHtmlModeTags_23[8] = {
#include "lobby/007EF7A0.inc"
};
u32 gHtmlModeTags_22[12] = {
#include "lobby/007EF7C0.inc"
};
u32 gHtmlModeTags_21[12] = {
#include "lobby/007EF7F0.inc"
};
u32 gHtmlModeTags_20[8] = {
#include "lobby/007EF820.inc"
};
u32 gHtmlModeTags_19[4] = {
#include "lobby/007EF840.inc"
};
u32 gHtmlModeTags_18[4] = {
#include "lobby/007EF850.inc"
};
u32 gHtmlModeTags_14[36] = {
#include "lobby/007EF860.inc"
};
u32 gHtmlModeTags_15[16] = {
#include "lobby/007EF8F0.inc"
};
u32 gHtmlModeTags_16[40] = {
#include "lobby/007EF930.inc"
};
u32 gHtmlModeTags_17[40] = {
#include "lobby/007EF9D0.inc"
};
u32 gHtmlModeTags_3[8] = {
#include "lobby/007EFA70.inc"
};
u32 gHtmlModeTags_13[16] = {
#include "lobby/007EFA90.inc"
};
u32 gHtmlModeTags_4[20] = {
#include "lobby/007EFAD0.inc"
};
u32 gHtmlModeTags_5[12] = {
#include "lobby/007EFB20.inc"
};
u32 gHtmlModeTags_6[12] = {
#include "lobby/007EFB50.inc"
};
u32 gHtmlModeTags_7[44] = {
#include "lobby/007EFB80.inc"
};
u32 gHtmlModeTags_8[8] = {
#include "lobby/007EFC30.inc"
};
u32 gHtmlModeTags_9[12] = {
#include "lobby/007EFC50.inc"
};
u32 gHtmlModeTags_10[12] = {
#include "lobby/007EFC80.inc"
};
u32 gHtmlModeTags_11[12] = {
#include "lobby/007EFCB0.inc"
};
u32 gHtmlModeTags_12[24] = {
#include "lobby/007EFCE0.inc"
};
HtmlTagSet gHtmlModeTags[25] = {
#include "lobby/007EFD40.inc"
};

/* unk_741900's .rodata (0x839908-0x839E10) */
#pragma rodata on
char D_00839908[0x8] = {
#include "lobby/00839908.inc"
};
char D_00839910[0x8] = {
#include "lobby/00839910.inc"
};
char D_00839918[0x8] = {
#include "lobby/00839918.inc"
};
char D_00839920[0x8] = {
#include "lobby/00839920.inc"
};
char D_00839928[0x8] = {
#include "lobby/00839928.inc"
};
char D_00839930[0x8] = {
#include "lobby/00839930.inc"
};
char D_00839938[0x8] = {
#include "lobby/00839938.inc"
};
char D_00839940[0x8] = {
#include "lobby/00839940.inc"
};
char D_00839948[0x8] = {
#include "lobby/00839948.inc"
};
char D_00839950[0x8] = {
#include "lobby/00839950.inc"
};
char D_00839958[0x8] = {
#include "lobby/00839958.inc"
};
char D_00839960[0x8] = {
#include "lobby/00839960.inc"
};
char D_00839968[0x8] = {
#include "lobby/00839968.inc"
};
char D_00839970[0x8] = {
#include "lobby/00839970.inc"
};
char D_00839978[0x8] = {
#include "lobby/00839978.inc"
};
char D_00839980[0x8] = {
#include "lobby/00839980.inc"
};
char D_00839988[0x8] = {
#include "lobby/00839988.inc"
};
char D_00839990[0x8] = {
#include "lobby/00839990.inc"
};
char D_00839998[0x8] = {
#include "lobby/00839998.inc"
};
char D_008399A0[0x8] = {
#include "lobby/008399A0.inc"
};
char D_008399A8[0x8] = {
#include "lobby/008399A8.inc"
};
char D_008399B0[0x8] = {
#include "lobby/008399B0.inc"
};
char D_008399B8[0x8] = {
#include "lobby/008399B8.inc"
};
char D_008399C0[0x10] = {
#include "lobby/008399C0.inc"
};
char D_008399D0[0x8] = {
#include "lobby/008399D0.inc"
};
char D_008399D8[0x8] = {
#include "lobby/008399D8.inc"
};
char D_008399E0[0x8] = {
#include "lobby/008399E0.inc"
};
char D_008399E8[0x8] = {
#include "lobby/008399E8.inc"
};
char D_008399F0[0x8] = {
#include "lobby/008399F0.inc"
};
char D_008399F8[0x8] = {
#include "lobby/008399F8.inc"
};
char D_00839A00[0x10] = {
#include "lobby/00839A00.inc"
};
char D_00839A10[0x10] = {
#include "lobby/00839A10.inc"
};
char D_00839A20[0x8] = {
#include "lobby/00839A20.inc"
};
char D_00839A28[0x8] = {
#include "lobby/00839A28.inc"
};
char D_00839A30[0x8] = {
#include "lobby/00839A30.inc"
};
char D_00839A38[0x8] = {
#include "lobby/00839A38.inc"
};
char D_00839A40[0x8] = {
#include "lobby/00839A40.inc"
};
char D_00839A48[0x8] = {
#include "lobby/00839A48.inc"
};
char D_00839A50[0x8] = {
#include "lobby/00839A50.inc"
};
char D_00839A58[0x8] = {
#include "lobby/00839A58.inc"
};
char D_00839A60[0x8] = {
#include "lobby/00839A60.inc"
};
char D_00839A68[0x8] = {
#include "lobby/00839A68.inc"
};
char D_00839A70[0x8] = {
#include "lobby/00839A70.inc"
};
char D_00839A78[0x8] = {
#include "lobby/00839A78.inc"
};
char D_00839A80[0x8] = {
#include "lobby/00839A80.inc"
};
char D_00839A88[0x8] = {
#include "lobby/00839A88.inc"
};
char D_00839A90[0x8] = {
#include "lobby/00839A90.inc"
};
char D_00839A98[0x8] = {
#include "lobby/00839A98.inc"
};
char D_00839AA0[0x10] = {
#include "lobby/00839AA0.inc"
};
char D_00839AB0[0x10] = {
#include "lobby/00839AB0.inc"
};
char D_00839AC0[0x10] = {
#include "lobby/00839AC0.inc"
};
char D_00839AD0[0x10] = {
#include "lobby/00839AD0.inc"
};
char D_00839AE0[0x10] = {
#include "lobby/00839AE0.inc"
};
char D_00839AF0[0x10] = {
#include "lobby/00839AF0.inc"
};
char D_00839B00[0x8] = {
#include "lobby/00839B00.inc"
};
char D_00839B08[0xC] = {
#include "lobby/00839B08.inc"
};
char D_00839B18[0xC] = {
#include "lobby/00839B18.inc"
};
char D_00839B28[0x8] = {
#include "lobby/00839B28.inc"
};
char D_00839B30[0x8] = {
#include "lobby/00839B30.inc"
};
char D_00839B38[0x8] = {
#include "lobby/00839B38.inc"
};
char D_00839B40[0x8] = {
#include "lobby/00839B40.inc"
};
char D_00839B48[0xC] = {
#include "lobby/00839B48.inc"
};
char D_00839B58[0xC] = {
#include "lobby/00839B58.inc"
};
char D_00839B68[0x8] = {
#include "lobby/00839B68.inc"
};
char D_00839B70[0x8] = {
#include "lobby/00839B70.inc"
};
char D_00839B78[0x8] = {
#include "lobby/00839B78.inc"
};
char D_00839B80[0x8] = {
#include "lobby/00839B80.inc"
};
char D_00839B88[0x8] = {
#include "lobby/00839B88.inc"
};
char D_00839B90[0x8] = {
#include "lobby/00839B90.inc"
};
char D_00839B98[0x8] = {
#include "lobby/00839B98.inc"
};
char D_00839BA0[0x8] = {
#include "lobby/00839BA0.inc"
};
char D_00839BA8[0x8] = {
#include "lobby/00839BA8.inc"
};
char D_00839BB0[0x8] = {
#include "lobby/00839BB0.inc"
};
char D_00839BB8[0x8] = {
#include "lobby/00839BB8.inc"
};
char D_00839BC0[0x8] = {
#include "lobby/00839BC0.inc"
};
char D_00839BC8[0x8] = {
#include "lobby/00839BC8.inc"
};
char D_00839BD0[0x8] = {
#include "lobby/00839BD0.inc"
};
char D_00839BD8[0x8] = {
#include "lobby/00839BD8.inc"
};
char D_00839BE0[0x8] = {
#include "lobby/00839BE0.inc"
};
char D_00839BE8[0x8] = {
#include "lobby/00839BE8.inc"
};
char D_00839BF0[0x8] = {
#include "lobby/00839BF0.inc"
};
char D_00839BF8[0x8] = {
#include "lobby/00839BF8.inc"
};
char D_00839C00[0x8] = {
#include "lobby/00839C00.inc"
};
char D_00839C08[0x8] = {
#include "lobby/00839C08.inc"
};
char D_00839C10[0x8] = {
#include "lobby/00839C10.inc"
};
char D_00839C18[0x8] = {
#include "lobby/00839C18.inc"
};
char D_00839C20[0x10] = {
#include "lobby/00839C20.inc"
};
char D_00839C30[0x8] = {
#include "lobby/00839C30.inc"
};
char D_00839C38[0x8] = {
#include "lobby/00839C38.inc"
};
char D_00839C40[0x8] = {
#include "lobby/00839C40.inc"
};
char D_00839C48[0x8] = {
#include "lobby/00839C48.inc"
};
char D_00839C50[0x8] = {
#include "lobby/00839C50.inc"
};
char D_00839C58[0xC] = {
#include "lobby/00839C58.inc"
};
char D_00839C68[0x8] = {
#include "lobby/00839C68.inc"
};
char D_00839C70[0x8] = {
#include "lobby/00839C70.inc"
};
char D_00839C78[0x8] = {
#include "lobby/00839C78.inc"
};
char D_00839C80[0x8] = {
#include "lobby/00839C80.inc"
};
char D_00839C88[0x8] = {
#include "lobby/00839C88.inc"
};
char D_00839C90[0x8] = {
#include "lobby/00839C90.inc"
};
char D_00839C98[0x8] = {
#include "lobby/00839C98.inc"
};
char D_00839CA0[0x8] = {
#include "lobby/00839CA0.inc"
};
char D_00839CA8[0x8] = {
#include "lobby/00839CA8.inc"
};
char D_00839CB0[0x8] = {
#include "lobby/00839CB0.inc"
};
char D_00839CB8[0x8] = {
#include "lobby/00839CB8.inc"
};
char D_00839CC0[0x8] = {
#include "lobby/00839CC0.inc"
};
char D_00839CC8[0x8] = {
#include "lobby/00839CC8.inc"
};
char D_00839CD0[0x10] = {
#include "lobby/00839CD0.inc"
};
char D_00839CE0[0x10] = {
#include "lobby/00839CE0.inc"
};
char D_00839CF0[0x8] = {
#include "lobby/00839CF0.inc"
};
char D_00839CF8[0xC] = {
#include "lobby/00839CF8.inc"
};
char D_00839D08[0x8] = {
#include "lobby/00839D08.inc"
};
char D_00839D10[0x10] = {
#include "lobby/00839D10.inc"
};
char D_00839D20[0x10] = {
#include "lobby/00839D20.inc"
};
char D_00839D30[0x8] = {
#include "lobby/00839D30.inc"
};
char D_00839D38[0x8] = {
#include "lobby/00839D38.inc"
};
char D_00839D40[0x8] = {
#include "lobby/00839D40.inc"
};
char D_00839D48[0xC] = {
#include "lobby/00839D48.inc"
};
char D_00839D58[0x8] = {
#include "lobby/00839D58.inc"
};
char D_00839D60[0x8] = {
#include "lobby/00839D60.inc"
};
char D_00839D68[0x8] = {
#include "lobby/00839D68.inc"
};
char D_00839D70[0x8] = {
#include "lobby/00839D70.inc"
};
char D_00839D78[0xC] = {
#include "lobby/00839D78.inc"
};
char D_00839D88[0x8] = {
#include "lobby/00839D88.inc"
};
char D_00839D90[0x8] = {
#include "lobby/00839D90.inc"
};
char D_00839D98[0x8] = {
#include "lobby/00839D98.inc"
};
char D_00839DA0[0x8] = {
#include "lobby/00839DA0.inc"
};
char D_00839DA8[0x8] = {
#include "lobby/00839DA8.inc"
};
char D_00839DB0[0x8] = {
#include "lobby/00839DB0.inc"
};
char D_00839DB8[0x8] = {
#include "lobby/00839DB8.inc"
};
char D_00839DC0[0x8] = {
#include "lobby/00839DC0.inc"
};
char D_00839DC8[0x8] = {
#include "lobby/00839DC8.inc"
};
char D_00839DD0[0x8] = {
#include "lobby/00839DD0.inc"
};
char D_00839DD8[0xC] = {
#include "lobby/00839DD8.inc"
};
char D_00839DE8[0x8] = {
#include "lobby/00839DE8.inc"
};
char D_00839DF0[0x8] = {
#include "lobby/00839DF0.inc"
};
char D_00839DF8[0xC] = {
#include "lobby/00839DF8.inc"
};
char D_00839E08[0x8] = {
#include "lobby/00839E08.inc"
};
#pragma rodata off
