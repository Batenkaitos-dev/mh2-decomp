/* The HTML parser's helpers: the font's character widths, the page's
 * lowest line, and the attribute value readers (a string, a number, a
 * percentage, a colour, a keyword). */
#include "common.h"
#include "html.h"
#include "browser.h"

/* Compiled at optimization level 4, as the lobby's HTML parser
 * (Html_ParseSome's file): every function of the file matches with it. */
#pragma optimization_level 4
s32 strncmp(const char *, const char *, s32);

s32 strlen(const char *s);
s32 strncmp(const char *a, const char *b, s32 n);

extern u8 gFullCharWidths[2][8]; /* a full-width character's width: [bold][size] */
extern u8 gHalfCharWidths[2][8]; /* a half-width character's width: [bold][size] */
extern HtmlColor gHtmlColorNames[];
extern char gHtmlHash[]; /* "#" */

/* Raises *dst to value (inlined, as in the table handlers: the game's
 * callers keep dst in a register of its own for the store). */
static inline void Html_MaxU16(u16 *dst, s32 value)
{
    if (*dst < value) {
        *dst = value;
    }
}

/* Sets the character widths for the current font size, bold or not. */
void Html_SetCharWidths(u8 bold)
{
    gHtml->charW = gFullCharWidths[bold][gHtml->fontSize[gHtml->fontDepth]];
    gHtml->halfW = gHalfCharWidths[bold][gHtml->fontSize[gHtml->fontDepth]];
}

/* Notes the pen's bottom as the page's lowest y (when drawing). */
void Html_NoteBottom(HtmlPen *pen)
{
    if (gHtml->measure == HTML_PASS_DRAW) {
        Html_MaxU16(&gHtml->bottom, pen->h + pen->y);
    }
}

/* Reads an attribute's value at *cur (after the name: `=` then the value,
 * quoted or up to a space or `>`) into buf (at most size - 1 bytes, a
 * Shift-JIS character kept whole) and moves *cur past it. */
void Html_ReadAttrValue(u8 **cur, char *buf, s32 size)
{
    u8 *pos;
    s32 state;
    s32 ch;
    s32 n;
    u8 charClass;

    pos = *cur;
    state = 0;
    n = 0;
    while ((ch = *pos) != 0) {
        if (n > size - 3) {
            break;
        }
        switch (state) {
        case 0:
            if (ch == '=') {
                state = 1;
            } else {
                state = 1;
                goto end;   /* no '=': the attribute has no value */
            }
            break;
        case 1:
            if (ch == '"') {
                state = 2;
            } else {
                state = 3;
                continue;
            }
            break;
        case 2:
            if (gCharClass[(u8)ch] & HTML_CHAR_SJIS_LEAD) {
                *buf = ch;
                pos++;
                n += 2;
                buf[1] = *pos;
                buf += 2;
                break;
            }
            if (ch == '"') {
                *buf = 0;
                *cur = pos;
                return;
            }
            *buf++ = ch;
            n++;
            break;
        case 3:
            charClass = gCharClass[(u8)ch];
            if (charClass & HTML_CHAR_SJIS_LEAD) {
                *buf = ch;
                pos++;
                n += 2;
                buf[1] = *pos;
                buf += 2;
                break;
            }
            if (charClass & HTML_CHAR_VALUE_END) {
                *buf = 0;
                *cur = pos;
                return;
            }
            *buf++ = ch;
            n++;
            break;
        }
        pos++;
    }
end:
    *buf = 0;
    *cur = pos;
}

/* Html_ReadAttrValue for keywords: the value upper-cased, no Shift-JIS. */
void Html_ReadAttrKeyword(u8 **cur, char *buf, s32 size)
{
    u8 *pos;
    s32 state;
    s32 n;
    s32 ch;
    u8 charClass;

    pos = *cur;
    state = 0;
    n = 0;
    while ((ch = *pos) != 0) {
        if (n > size - 2) {
            break;
        }
        switch (state) {
        case 0:
            if (ch == '=') {
                state = 1;
            } else {
                state = 1;
                goto end;   /* no '=': the attribute has no value */
            }
            break;
        case 1:
            if (ch == '"') {
                state = 2;
            } else {
                state = 3;
                continue;
            }
            break;
        case 2:
            if (ch == '"') {
                *buf = 0;
                *cur = pos;
                return;
            }
            *buf++ = gToUpper[(u8)ch];
            n++;
            break;
        case 3:
            charClass = gCharClass[(u8)ch];
            if (charClass & HTML_CHAR_VALUE_END) {
                *buf = 0;
                *cur = pos;
                return;
            }
            *buf++ = gToUpper[(u8)ch];
            n++;
            break;
        }
        pos++;
    }
end:
    *buf = 0;
    *cur = pos;
}

/* Html_ReadAttrValue for <META CONTENT=...>: the value ends at `"` or `;`. */
void Html_ReadMetaContent(u8 **cur, char *buf, s32 size)
{
    u8 *pos;
    s32 state;
    s32 n;
    s32 ch;

    pos = *cur;
    state = 0;
    n = 0;
    while ((ch = *pos) != 0) {
        if (n > size - 2) {
            break;
        }
        switch (state) {
        case 0:
            if (ch == '=') {
                state = 1;
            } else {
                state = 1;
                goto end;   /* no '=': the attribute has no value */
            }
            break;
        case 1:
            state = 2;
            if (ch != '"') {
                continue;
            }
            break;
        case 2:
            if (ch == '"' || ch == ';') {
                *buf = 0;
                *cur = pos;
                return;
            }
            *buf++ = ch;
            n++;
            break;
        }
        pos++;
    }
end:
    *buf = 0;
    *cur = pos;
}

/* A colour: "#RRGGBB" or a colour name (gHtmlColorNames); -1 when unknown. */
s32 Html_ParseColor(char *str)
{
    char buf[256];
    s32 j;
    s32 rgb = 0;
    s32 len;
    s32 i;
    u8 digit;
    HtmlColor *names;
    HtmlColor *entry;

    len = strlen(str);
    if (strncmp(str, gHtmlHash, 1) == 0) {
        str++;
        for (i = 1; i < len; i++, str++) {
            digit = gHexDigitValues[(u8)*str];
            if (digit) {
                rgb *= 16;
                rgb += digit - 1;
            }
        }
        return rgb;
    }
    len = strlen(str);
    for (j = 0; j < len; j++) {
        buf[j] = gToUpper[(u8)str[j]];
    }
    buf[j] = 0;
    names = gHtmlColorNames;
    for (i = 0, entry = names; i < HTML_COLOR_NAMES; i++, entry++) {
        if (strncmp(buf, entry->name, entry->size) == 0) {
            return (names[i].r << 16) + (names[i].g << 8) + names[i].b;
        }
    }
    return -1;
}

/* A number: the decimal digits of s (other characters are skipped). */
s32 Html_ParseNumber(char *str)
{
    u8 *pos = (u8 *)str;
    s32 len;
    s32 i;
    s32 n;

    len = strlen(str);
    n = 0;
    for (i = 0; i < len; i++, pos++) {
        if (*pos <= '9' && *pos >= '0') {
            n *= 10;
            n += *pos - '0';
        }
    }
    return n;
}

/* A length: "N" pixels, or "N%" of whole (of the page's 584 when whole is
 * 0). */
s32 Html_ParseLength(char *str, s32 whole)
{
    s32 len;
    s32 i;
    s32 n;

    len = strlen(str);
    n = 0;
    if ((u8)str[len - 1] != '%') {
        for (i = 0; i < len; i++, str++) {
            if (*(u8 *)str <= '9' && *(u8 *)str >= '0') {
                n *= 10;
                n += *(u8 *)str - '0';
            }
        }
    } else {
        for (i = 0; i < len - 1; i++, str++) {
            if (*(u8 *)str <= '9' && *(u8 *)str >= '0') {
                n *= 10;
                n += *(u8 *)str - '0';
            }
        }
        if (whole != 0) {
            n = whole * n / 100;
        } else {
            n = n * PAGE_VIEW_W / 100;
        }
    }
    return n;
}

/* <FONT SIZE=...>: "N", "+N" or "-N" (from 3), 1..7; the current size
 * when it is none of these. */
s32 Html_ParseFontSize(char *str)
{
    u8 *chars = (u8 *)str;
    s32 size;

    size = gHtml->fontSize[gHtml->fontDepth];
    if (chars[0] == '-') {
        if (chars[1] <= '9' && chars[1] >= '0') {
            size = HTML_FONT_SIZE_BASE - (chars[1] - '0');
            if (size <= 0) {
                size = 1;
            }
        }
    } else if (chars[0] == '+') {
        if (chars[1] <= '9' && chars[1] >= '0') {
            size = (chars[1] - '0') + HTML_FONT_SIZE_BASE;
            if (size > HTML_FONT_SIZE_MAX) {
                size = HTML_FONT_SIZE_MAX;
            }
        }
    } else if (chars[0] <= '9' && chars[0] >= '0') {
        size = chars[0] - '0';
    }
    return size;
}

/* Skips spaces and tabs at p, unless in <PRE>. */
u8 *Html_SkipSpaces(u8 *pos)
{
    u8 ch;

    if (gHtml->pen.lineUsed == 0 && gHtml->flags == 0) {
        while (!(ch = *pos) == 0 && (gCharClass[ch] & HTML_CHAR_BLANK)) {
            pos++;
        }
    }
    return pos;
}

/* A keyword: s upper-cased, looked up in table (size bytes of HtmlKeyword);
 * its value, or 0. */
u8 Html_LookupKeyword(char *word, HtmlKeyword *table, u32 size)
{
    char buf[256];
    u16 len;
    u16 i;
    u16 n;

    len = strlen(word);
    for (i = 0; i < len; i++) {
        buf[i] = gToUpper[(u8)word[i]];
    }
    buf[i] = 0;
    n = size / HTML_KEYWORD_SIZE;
    for (i = 0; i < n; i++) {
        if (strncmp(buf, table[i].name, table[i].size) == 0) {
            return table[i].value;
        }
    }
    return 0;
}

/* The strings the tables below point at, then the parser's character
 * tables (gCharClass, gHexDigitValues, gToUpper), the entities and the
 * colour names (gHtmlColorNames). */
extern char gHtmlEntityNames_0[];
extern char gHtmlEntityNames_1[];
extern char gHtmlEntityNames_2[];
extern char gHtmlEntityNames_3[];
extern char gHtmlEntityNames_4[];
extern char gHtmlEntityNames_5[];
extern char gHtmlEntityNames_6[];
extern char gHtmlEntityNames_7[];
extern char gHtmlEntityNames_8[];
extern char gHtmlColorNames_0[];
extern char gHtmlColorNames_1[];
extern char gHtmlColorNames_2[];
extern char gHtmlColorNames_3[];
extern char gHtmlColorNames_4[];
extern char gHtmlColorNames_5[];
extern char gHtmlColorNames_6[];
extern char gHtmlColorNames_7[];
extern u32 D_00839F68[];
extern char gHtmlColorNames_9[];
extern char gHtmlColorNames_10[];
extern char gHtmlColorNames_11[];
extern char gHtmlColorNames_12[];
extern char gHtmlColorNames_13[];
extern char gHtmlColorNames_14[];
extern char gHtmlColorNames_15[];
extern char gHtmlColorNames_16[];
extern char gHtmlColorNames_17[];
extern char gHtmlColorNames_18[];
extern char gHtmlColorNames_19[];
extern char gHtmlColorNames_20[];
extern char gHtmlColorNames_21[];
extern char gHtmlColorNames_22[];
extern char gHtmlColorNames_23[];
extern char gHtmlColorNames_24[];
extern char gHtmlColorNames_25[];
extern char gHtmlColorNames_26[];
extern char gHtmlColorNames_27[];
extern char gHtmlColorNames_28[];
extern char gHtmlColorNames_29[];
extern char gHtmlColorNames_30[];
extern char gHtmlColorNames_31[];
extern char gHtmlColorNames_32[];
extern char gHtmlColorNames_33[];
extern char gHtmlColorNames_34[];
extern char gHtmlColorNames_35[];
extern char gHtmlColorNames_36[];
extern u32 D_0083A0F8[];
extern u32 D_0083A108[];
extern u32 D_0083A118[];
extern u32 D_0083A128[];
extern u32 D_0083A138[];
extern char gHtmlColorNames_42[];
extern char gHtmlColorNames_43[];
extern char gHtmlColorNames_44[];
extern char gHtmlColorNames_45[];
extern char gHtmlColorNames_46[];
extern char gHtmlColorNames_47[];
extern char gHtmlColorNames_48[];
extern char gHtmlColorNames_49[];
extern char gHtmlColorNames_50[];
extern char gHtmlColorNames_51[];
extern char gHtmlColorNames_52[];
extern char gHtmlColorNames_53[];
extern char gHtmlColorNames_54[];
extern char gHtmlColorNames_55[];
extern char gHtmlColorNames_56[];
extern char gHtmlColorNames_57[];
extern char gHtmlColorNames_58[];
extern char gHtmlColorNames_59[];
extern char gHtmlColorNames_60[];
extern char gHtmlColorNames_61[];
extern char gHtmlColorNames_62[];
extern char gHtmlColorNames_63[];
extern char gHtmlColorNames_64[];
extern char gHtmlColorNames_65[];
extern char gHtmlColorNames_66[];
extern char gHtmlColorNames_67[];
extern char gHtmlColorNames_68[];
extern char gHtmlColorNames_69[];
extern char gHtmlColorNames_70[];
extern char gHtmlColorNames_71[];
extern char gHtmlColorNames_72[];
extern u32 D_0083A2F8[];
extern u32 D_0083A308[];
extern u32 D_0083A318[];
extern u32 D_0083A328[];
extern u32 D_0083A338[];
extern u32 D_0083A348[];
extern u32 D_0083A358[];
extern u32 D_0083A368[];
extern char gHtmlColorNames_81[];
extern char gHtmlColorNames_82[];
extern char gHtmlColorNames_83[];
extern char gHtmlColorNames_84[];
extern char gHtmlColorNames_85[];
extern char gHtmlColorNames_86[];
extern char gHtmlColorNames_87[];
extern u32 D_0083A3D8[];
extern u32 D_0083A3E8[];
extern u32 D_0083A3F8[];
extern u32 D_0083A408[];
extern u32 D_0083A418[];
extern char gHtmlColorNames_93[];
extern char gHtmlColorNames_94[];
extern char gHtmlColorNames_95[];
extern char gHtmlColorNames_96[];
extern char gHtmlColorNames_97[];
extern char gHtmlColorNames_98[];
extern char gHtmlColorNames_99[];
extern char gHtmlColorNames_100[];
extern char gHtmlColorNames_101[];
extern char gHtmlColorNames_102[];
extern char gHtmlColorNames_103[];
extern char gHtmlColorNames_104[];
extern char gHtmlColorNames_105[];
extern char gHtmlColorNames_106[];
extern char gHtmlColorNames_107[];
extern u32 D_0083A508[];
extern u32 D_0083A518[];
extern u32 D_0083A528[];
extern u32 D_0083A538[];
extern char gHtmlColorNames_112[];
extern char gHtmlColorNames_113[];
extern char gHtmlColorNames_114[];
extern char gHtmlColorNames_115[];
extern char gHtmlColorNames_116[];
extern char gHtmlColorNames_117[];
extern char gHtmlColorNames_118[];
extern char gHtmlColorNames_119[];
extern char gHtmlColorNames_120[];
extern char gHtmlColorNames_121[];
extern char gHtmlColorNames_122[];
extern char gHtmlColorNames_123[];
extern char gHtmlColorNames_124[];
extern char gHtmlColorNames_125[];
extern char gHtmlColorNames_126[];
extern char gHtmlColorNames_127[];
extern char gHtmlColorNames_128[];
extern char gHtmlColorNames_129[];
extern char gHtmlColorNames_130[];
extern char gHtmlColorNames_131[];
extern char gHtmlColorNames_132[];
extern char gHtmlColorNames_133[];
extern char gHtmlColorNames_134[];
extern char gHtmlColorNames_135[];
extern char gHtmlColorNames_136[];
extern char gHtmlColorNames_137[];
extern char gHtmlColorNames_138[];
extern char gHtmlColorNames_139[];
extern char gHtmlColorNames_140[];
extern char gHtmlColorNames_141[];
extern char gHtmlColorNames_142[];
extern char gHtmlColorNames_143[];
extern char gHtmlColorNames_144[];
extern char gHtmlColorNames_145[];
extern char gHtmlColorNames_146[];
u8 gCharClass[256] = {
#include "lobby/007EFF00.inc"
};
u8 gHexDigitValues[256] = {
#include "lobby/007F0000.inc"
};
u8 gToUpper[256] = {
#include "lobby/007F0100.inc"
};
char *gHtmlEntityNames[10] = {
    gHtmlEntityNames_0, gHtmlEntityNames_1, gHtmlEntityNames_2, gHtmlEntityNames_3,
    gHtmlEntityNames_4, gHtmlEntityNames_5, gHtmlEntityNames_6, gHtmlEntityNames_7,
    gHtmlEntityNames_8, NULL,
};
char gHtmlEntityChars[12] = {
#include "lobby/007F0228.inc"
};
HtmlColor gHtmlColorNames[148] = {
#include "lobby/007F0240.inc"
};

/* The strings (read-only data): the entities' and the colours' names,
 * and the short keywords and colours the handlers compare with. */
#pragma rodata on
char gHtmlEntityNames_0[8] = {
#include "lobby/00839E90.inc"
};
char gHtmlEntityNames_1[8] = {
#include "lobby/00839E98.inc"
};
char gHtmlEntityNames_2[8] = {
#include "lobby/00839EA0.inc"
};
char gHtmlEntityNames_3[8] = {
#include "lobby/00839EA8.inc"
};
char gHtmlEntityNames_4[8] = {
#include "lobby/00839EB0.inc"
};
char gHtmlEntityNames_5[8] = {
#include "lobby/00839EB8.inc"
};
char gHtmlEntityNames_6[8] = {
#include "lobby/00839EC0.inc"
};
char gHtmlEntityNames_7[8] = {
#include "lobby/00839EC8.inc"
};
char gHtmlEntityNames_8[8] = {
#include "lobby/00839ED0.inc"
};
char gHtmlAmpersand[8] = {
#include "lobby/00839ED8.inc"
};
char gHtmlBlack[8] = {
#include "lobby/00839EE0.inc"
};
char gHtmlBlue[8] = {
#include "lobby/00839EE8.inc"
};
char gHtmlShapeRect[8] = {
#include "lobby/00839EF0.inc"
};
char gHtmlMethodPost[8] = {
#include "lobby/00839EF8.inc"
};
char gHtmlFontSizeDefault[8] = {
#include "lobby/00839F00.inc"
};
char gHtmlRefresh[8] = {
#include "lobby/00839F08.inc"
};
char gHtmlColorNames_0[16] = {
#include "lobby/00839F10.inc"
};
char gHtmlColorNames_1[16] = {
#include "lobby/00839F20.inc"
};
char gHtmlColorNames_2[8] = {
#include "lobby/00839F30.inc"
};
char gHtmlColorNames_3[12] = {
#include "lobby/00839F38.inc"
};
char gHtmlColorNames_4[8] = {
#include "lobby/00839F48.inc"
};
char gHtmlColorNames_5[8] = {
#include "lobby/00839F50.inc"
};
char gHtmlColorNames_6[8] = {
#include "lobby/00839F58.inc"
};
char gHtmlColorNames_7[24] = {
#include "lobby/00839F60.inc"
};
char gHtmlColorNames_9[8] = {
#include "lobby/00839F78.inc"
};
char gHtmlColorNames_10[16] = {
#include "lobby/00839F80.inc"
};
char gHtmlColorNames_11[8] = {
#include "lobby/00839F90.inc"
};
char gHtmlColorNames_12[8] = {
#include "lobby/00839F98.inc"
};
char gHtmlColorNames_13[16] = {
#include "lobby/00839FA0.inc"
};
char gHtmlColorNames_14[16] = {
#include "lobby/00839FB0.inc"
};
char gHtmlColorNames_15[16] = {
#include "lobby/00839FC0.inc"
};
char gHtmlColorNames_16[16] = {
#include "lobby/00839FD0.inc"
};
char gHtmlColorNames_17[16] = {
#include "lobby/00839FE0.inc"
};
char gHtmlColorNames_18[8] = {
#include "lobby/00839FF0.inc"
};
char gHtmlColorNames_19[8] = {
#include "lobby/00839FF8.inc"
};
char gHtmlColorNames_20[16] = {
#include "lobby/0083A000.inc"
};
char gHtmlColorNames_21[16] = {
#include "lobby/0083A010.inc"
};
char gHtmlColorNames_22[16] = {
#include "lobby/0083A020.inc"
};
char gHtmlColorNames_23[8] = {
#include "lobby/0083A030.inc"
};
char gHtmlColorNames_24[8] = {
#include "lobby/0083A038.inc"
};
char gHtmlColorNames_25[16] = {
#include "lobby/0083A040.inc"
};
char gHtmlColorNames_26[16] = {
#include "lobby/0083A050.inc"
};
char gHtmlColorNames_27[16] = {
#include "lobby/0083A060.inc"
};
char gHtmlColorNames_28[16] = {
#include "lobby/0083A070.inc"
};
char gHtmlColorNames_29[16] = {
#include "lobby/0083A080.inc"
};
char gHtmlColorNames_30[16] = {
#include "lobby/0083A090.inc"
};
char gHtmlColorNames_31[16] = {
#include "lobby/0083A0A0.inc"
};
char gHtmlColorNames_32[16] = {
#include "lobby/0083A0B0.inc"
};
char gHtmlColorNames_33[16] = {
#include "lobby/0083A0C0.inc"
};
char gHtmlColorNames_34[16] = {
#include "lobby/0083A0D0.inc"
};
char gHtmlColorNames_35[16] = {
#include "lobby/0083A0E0.inc"
};
char gHtmlColorNames_36[88] = {
#include "lobby/0083A0F0.inc"
};
char gHtmlColorNames_42[12] = {
#include "lobby/0083A148.inc"
};
char gHtmlColorNames_43[12] = {
#include "lobby/0083A158.inc"
};
char gHtmlColorNames_44[12] = {
#include "lobby/0083A168.inc"
};
char gHtmlColorNames_45[8] = {
#include "lobby/0083A178.inc"
};
char gHtmlColorNames_46[16] = {
#include "lobby/0083A180.inc"
};
char gHtmlColorNames_47[16] = {
#include "lobby/0083A190.inc"
};
char gHtmlColorNames_48[16] = {
#include "lobby/0083A1A0.inc"
};
char gHtmlColorNames_49[16] = {
#include "lobby/0083A1B0.inc"
};
char gHtmlColorNames_50[16] = {
#include "lobby/0083A1C0.inc"
};
char gHtmlColorNames_51[8] = {
#include "lobby/0083A1D0.inc"
};
char gHtmlColorNames_52[12] = {
#include "lobby/0083A1D8.inc"
};
char gHtmlColorNames_53[12] = {
#include "lobby/0083A1E8.inc"
};
char gHtmlColorNames_54[8] = {
#include "lobby/0083A1F8.inc"
};
char gHtmlColorNames_55[16] = {
#include "lobby/0083A200.inc"
};
char gHtmlColorNames_56[8] = {
#include "lobby/0083A210.inc"
};
char gHtmlColorNames_57[8] = {
#include "lobby/0083A218.inc"
};
char gHtmlColorNames_58[16] = {
#include "lobby/0083A220.inc"
};
char gHtmlColorNames_59[16] = {
#include "lobby/0083A230.inc"
};
char gHtmlColorNames_60[8] = {
#include "lobby/0083A240.inc"
};
char gHtmlColorNames_61[12] = {
#include "lobby/0083A248.inc"
};
char gHtmlColorNames_62[8] = {
#include "lobby/0083A258.inc"
};
char gHtmlColorNames_63[8] = {
#include "lobby/0083A260.inc"
};
char gHtmlColorNames_64[8] = {
#include "lobby/0083A268.inc"
};
char gHtmlColorNames_65[16] = {
#include "lobby/0083A270.inc"
};
char gHtmlColorNames_66[16] = {
#include "lobby/0083A280.inc"
};
char gHtmlColorNames_67[16] = {
#include "lobby/0083A290.inc"
};
char gHtmlColorNames_68[16] = {
#include "lobby/0083A2A0.inc"
};
char gHtmlColorNames_69[16] = {
#include "lobby/0083A2B0.inc"
};
char gHtmlColorNames_70[16] = {
#include "lobby/0083A2C0.inc"
};
char gHtmlColorNames_71[16] = {
#include "lobby/0083A2D0.inc"
};
char gHtmlColorNames_72[152] = {
#include "lobby/0083A2E0.inc"
};
char gHtmlColorNames_81[12] = {
#include "lobby/0083A378.inc"
};
char gHtmlColorNames_82[8] = {
#include "lobby/0083A388.inc"
};
char gHtmlColorNames_83[16] = {
#include "lobby/0083A390.inc"
};
char gHtmlColorNames_84[8] = {
#include "lobby/0083A3A0.inc"
};
char gHtmlColorNames_85[8] = {
#include "lobby/0083A3A8.inc"
};
char gHtmlColorNames_86[16] = {
#include "lobby/0083A3B0.inc"
};
char gHtmlColorNames_87[112] = {
#include "lobby/0083A3C0.inc"
};
char gHtmlColorNames_93[32] = {
#include "lobby/0083A430.inc"
};
char gHtmlColorNames_94[16] = {
#include "lobby/0083A450.inc"
};
char gHtmlColorNames_95[16] = {
#include "lobby/0083A460.inc"
};
char gHtmlColorNames_96[16] = {
#include "lobby/0083A470.inc"
};
char gHtmlColorNames_97[16] = {
#include "lobby/0083A480.inc"
};
char gHtmlColorNames_98[16] = {
#include "lobby/0083A490.inc"
};
char gHtmlColorNames_99[16] = {
#include "lobby/0083A4A0.inc"
};
char gHtmlColorNames_100[16] = {
#include "lobby/0083A4B0.inc"
};
char gHtmlColorNames_101[8] = {
#include "lobby/0083A4C0.inc"
};
char gHtmlColorNames_102[8] = {
#include "lobby/0083A4C8.inc"
};
char gHtmlColorNames_103[8] = {
#include "lobby/0083A4D0.inc"
};
char gHtmlColorNames_104[12] = {
#include "lobby/0083A4D8.inc"
};
char gHtmlColorNames_105[8] = {
#include "lobby/0083A4E8.inc"
};
char gHtmlColorNames_106[16] = {
#include "lobby/0083A4F0.inc"
};
char gHtmlColorNames_107[72] = {
#include "lobby/0083A500.inc"
};
char gHtmlColorNames_112[12] = {
#include "lobby/0083A548.inc"
};
char gHtmlColorNames_113[12] = {
#include "lobby/0083A558.inc"
};
char gHtmlColorNames_114[8] = {
#include "lobby/0083A568.inc"
};
char gHtmlColorNames_115[8] = {
#include "lobby/0083A570.inc"
};
char gHtmlColorNames_116[8] = {
#include "lobby/0083A578.inc"
};
char gHtmlColorNames_117[16] = {
#include "lobby/0083A580.inc"
};
char gHtmlColorNames_118[8] = {
#include "lobby/0083A590.inc"
};
char gHtmlColorNames_119[8] = {
#include "lobby/0083A598.inc"
};
char gHtmlColorNames_120[16] = {
#include "lobby/0083A5A0.inc"
};
char gHtmlColorNames_121[16] = {
#include "lobby/0083A5B0.inc"
};
char gHtmlColorNames_122[16] = {
#include "lobby/0083A5C0.inc"
};
char gHtmlColorNames_123[16] = {
#include "lobby/0083A5D0.inc"
};
char gHtmlColorNames_124[8] = {
#include "lobby/0083A5E0.inc"
};
char gHtmlColorNames_125[12] = {
#include "lobby/0083A5E8.inc"
};
char gHtmlColorNames_126[12] = {
#include "lobby/0083A5F8.inc"
};
char gHtmlColorNames_127[12] = {
#include "lobby/0083A608.inc"
};
char gHtmlColorNames_128[8] = {
#include "lobby/0083A618.inc"
};
char gHtmlColorNames_129[8] = {
#include "lobby/0083A620.inc"
};
char gHtmlColorNames_130[8] = {
#include "lobby/0083A628.inc"
};
char gHtmlColorNames_131[16] = {
#include "lobby/0083A630.inc"
};
char gHtmlColorNames_132[16] = {
#include "lobby/0083A640.inc"
};
char gHtmlColorNames_133[8] = {
#include "lobby/0083A650.inc"
};
char gHtmlColorNames_134[12] = {
#include "lobby/0083A658.inc"
};
char gHtmlColorNames_135[12] = {
#include "lobby/0083A668.inc"
};
char gHtmlColorNames_136[8] = {
#include "lobby/0083A678.inc"
};
char gHtmlColorNames_137[8] = {
#include "lobby/0083A680.inc"
};
char gHtmlColorNames_138[8] = {
#include "lobby/0083A688.inc"
};
char gHtmlColorNames_139[8] = {
#include "lobby/0083A690.inc"
};
char gHtmlColorNames_140[12] = {
#include "lobby/0083A698.inc"
};
char gHtmlColorNames_141[8] = {
#include "lobby/0083A6A8.inc"
};
char gHtmlColorNames_142[8] = {
#include "lobby/0083A6B0.inc"
};
char gHtmlColorNames_143[8] = {
#include "lobby/0083A6B8.inc"
};
char gHtmlColorNames_144[16] = {
#include "lobby/0083A6C0.inc"
};
char gHtmlColorNames_145[8] = {
#include "lobby/0083A6D0.inc"
};
char gHtmlColorNames_146[12] = {
#include "lobby/0083A6D8.inc"
};
char gHtmlHash[8] = {
#include "lobby/0083A6E8.inc"
};
#pragma rodata off
