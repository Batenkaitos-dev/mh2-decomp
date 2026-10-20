/* The HTML parser: </TD>, the attribute value readers (a string, a
 * number, a percentage, a colour, a keyword) and the text run layout. */
#include "common.h"
#include "html.h"
#include "browser.h"

/* Compiled at optimization level 4, as the lobby's HTML parser
 * (Html_ParseSome's file): every function of the file matches with it. */
#pragma optimization_level 4
s32 strncmp(const char *, const char *, s32);

s32 strlen(const char *s);
s32 strncmp(const char *a, const char *b, s32 n);

void Html_SetCharWidths(u8 bold);
void Html_NoteBottom(HtmlPen *pen);
void Html_ReadAttrValue(u8 **cur, char *buf, s32 size);
void Layout_NextLine(void);
void Layout_FlushBlock(u8 *text);
void Layout_CellAlign(HtmlCell *cell);
s32 Layout_CellColSpan(HtmlCell *cell);
s32 Layout_BeforeClose(u8 **cur, u8 *text, u8 kind);
void Layout_DrawCellBox(HtmlCell *cell);
void Layout_DrawRun(u16 x, u16 y, HtmlLine *line);
u16 Layout_CenterOffset(HtmlLine *line);
void Draw_Anchor(u16 x, u16 y, u8 *name);
void Draw_Text(u16 x, u16 y, s32 color, u8 size, u16 x2, u16 y2, u8 *text, u8 a8);
void Draw_LinkText(u16 x, u16 y, s32 color, u8 size, u16 x2, u16 y2, u8 *href, u8 *text, u8 a8);
void Draw_StruckText(u16 x, u16 y, s32 color, u8 size, u16 x2, u16 y2, u8 *s1, u8 *s2, u8 a8);
void Draw_UnderlinedText(u16 x, u16 y, s32 color, u8 size, u16 x2, u16 y2, u8 *s1, u8 *s2, u8 a8);

extern u8 gFullCharWidths[2][8]; /* a full-width character's width: [bold][size] */
extern u8 gHalfCharWidths[2][8]; /* a half-width character's width: [bold][size] */
extern u8 gCharClass[]; /* character classes */
extern u8 gToUpper[]; /* to upper case */
extern u8 gHexDigitValues[]; /* hex digits: value + 1, 0 for others */
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

/* <TD BACKGROUND=...> */
s32 Html_AttrTdBackground(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->tdAttr.background, sizeof(gHtml->tdAttr.background));
    return 0;
}

/* </TD>, </TH> */
s32 Html_TagEndTd(u8 **cur, u8 *text)
{
    HtmlCell *cell;
    HtmlCell *parent;
    s32 n;

    if (gHtml->tableDepth == 0) {
        return 0;
    }
    if (Layout_BeforeClose(cur, text, HTML_CELL_TD) < 0) {
        return -1;
    }
    gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
    cell = &gHtml->cells[gHtml->curCell];
    parent = cell->parent;
    if (parent == NULL) {
        return -1;
    }
    Html_SetCharWidths(cell->bold);
    cell->open = 0;
    Layout_FlushBlock(text);
    gHtml->cells[gHtml->curCell].flags &= ~HTML_FLAG_PRE;
    gHtml->pre = 0;
    gHtml->cells[gHtml->curCell].bold = 0;
    if (gHtml->measure == HTML_PASS_MEASURE && cell->colSpan) {
        n = (cell->colSpan - 1) * (cell->spacing + cell->padding * 2);
        if (cell->minW < n) {
            cell->minW = n;
        }
    }
    if (cell->w < cell->minW) {
        cell->w = cell->minW;
    }
    if (cell->rowSpan == 0) {
        Html_MaxU16(&cell->h, gHtml->cellPen.runY);
        if (cell->h < cell->innerH) {
            cell->h = cell->innerH;
        }
    } else if (gHtml->measure != HTML_PASS_DRAW) {
        cell->spanH = gHtml->cellPen.runY;
    }
    cell->textH = gHtml->cellPen.runY;
    n = cell->h + cell->padding * 2;
    if (parent->h < n) {
        parent->h = n;
    }
    if (cell->colSpan == 0) {
        parent->penX += cell->spacing + (cell->w + cell->padding * 2);
        if (gHtml->measure == HTML_PASS_MEASURE) {
            parent->minW += cell->spacing + (cell->w + cell->padding * 2);
            parent->cellCount++;
        }
    }
    if (gHtml->measure == HTML_PASS_MEASURE) {
        parent->maxW += cell->spacing + (cell->maxW + cell->padding * 2);
    }
    Layout_DrawCellBox(cell);
    if (gHtml->measure == HTML_PASS_MEASURE) {
        Html_MaxU16(&gHtml->pageWidth, cell->x + cell->w);
    }
    if (gHtml->tableDepth != 0) {
        gHtml->tableDepth--;
    } else {
        return -1;
    }
    if (Layout_CellColSpan(cell) < 0) {
        return -1;
    }
    Layout_CellAlign(cell);
    Layout_NextLine();
    Layout_CellAlign(cell);
    return 0;
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
    char buf[0x100];
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
    char buf[0x100];
    u16 len;
    u16 i;
    u16 n;

    len = strlen(word);
    for (i = 0; i < len; i++) {
        buf[i] = gToUpper[(u8)word[i]];
    }
    buf[i] = 0;
    n = size / sizeof(HtmlKeyword);
    for (i = 0; i < n; i++) {
        if (strncmp(buf, table[i].name, table[i].size) == 0) {
            return table[i].value;
        }
    }
    return 0;
}

/* Draws the pending run of text (gHtml->line) at (x, y) on its line, by
 * what it is (HtmlState.link): plain, struck or underlined text, or a
 * link with its target. */
void Layout_DrawRun(u16 x, u16 y, HtmlLine *line)
{
    u8 bold;

    if (strlen(gHtml->line) != 0 || gHtml->inFrame != 0) {
        if (gHtml->tableDepth != 0) {
            HtmlCell *cell = &gHtml->cells[gHtml->curCell];
            bold = cell->bold;
        } else {
            bold = gHtml->bold;
        }
        switch (gHtml->link) {
        case HTML_LINK_NONE:
            gHtml->anchor[0] = 0;
            if (gHtml->strike != 0) {
                Draw_StruckText(x, y + line->h - gHtml->charW, gHtml->fontColor[gHtml->fontDepth],
                              gHtml->charW, x + gHtml->pen.runW, y + line->h, (u8 *) gHtml->line,
                              (u8 *) gHtml->anchor, bold);
            } else if (gHtml->underline != 0) {
                Draw_UnderlinedText(x, y + line->h - gHtml->charW, gHtml->fontColor[gHtml->fontDepth],
                              gHtml->charW, x + gHtml->pen.runW, y + line->h, (u8 *) gHtml->line,
                              (u8 *) gHtml->anchor, bold);
            } else {
                Draw_Text(x, y + line->h - gHtml->charW, gHtml->fontColor[gHtml->fontDepth],
                              gHtml->charW, x + gHtml->pen.runW, y + line->h, (u8 *) gHtml->line, bold);
            }
            break;
        case HTML_LINK_LOCAL:
            if (gHtml->strike != 0) {
                Draw_StruckText(x, y + line->h - gHtml->charW, gHtml->linkColorValue, gHtml->charW,
                              x + gHtml->pen.runW, y + line->h, (u8 *) gHtml->line, (u8 *) gHtml->anchor, bold);
            } else if (gHtml->underline != 0) {
                Draw_UnderlinedText(x, y + line->h - gHtml->charW, gHtml->linkColorValue, gHtml->charW,
                              x + gHtml->pen.runW, y + line->h, (u8 *) gHtml->line, (u8 *) gHtml->anchor, bold);
            } else {
                Draw_LinkText(x, y + line->h - gHtml->charW, gHtml->linkColorValue, gHtml->charW,
                              x + gHtml->pen.runW, y + line->h, (u8 *) gHtml->line, (u8 *) gHtml->anchor, bold);
            }
            break;
        case HTML_LINK_URL:
            if (gHtml->strike != 0) {
                Draw_StruckText(x, y + line->h - gHtml->charW, gHtml->linkColorValue, gHtml->charW,
                              x + gHtml->pen.runW, y + line->h, (u8 *) gHtml->line, (u8 *) gHtml->href, bold);
            } else if (gHtml->underline != 0) {
                Draw_UnderlinedText(x, y + line->h - gHtml->charW, gHtml->linkColorValue, gHtml->charW,
                              x + gHtml->pen.runW, y + line->h, (u8 *) gHtml->line, (u8 *) gHtml->href, bold);
            } else {
                Draw_LinkText(x, y + line->h - gHtml->charW, gHtml->linkColorValue, gHtml->charW,
                              x + gHtml->pen.runW, y + line->h, (u8 *) gHtml->line, (u8 *) gHtml->href, bold);
            }
            break;
        case HTML_LINK_NAME:
            Draw_Text(x, y + line->h - gHtml->charW, gHtml->fontColor[gHtml->fontDepth],
                          gHtml->charW, x + gHtml->pen.runW, y + line->h, (u8 *) gHtml->line, bold);
            break;
        }
    }
}

/* The x offset that centres a line (in <CENTER>), else 0. */
u16 Layout_CenterOffset(HtmlLine *line)
{
    s32 off;
    s32 room;

    off = 0;
    if (gHtml->center != 0) {
        room = gHtml->width - gHtml->indent;
        if (line->w < room) {
            off = (room - line->w) / 2;
        }
    }
    return off;
}

/* <A NAME=...>: an anchor at the pen's position. */
void Layout_Anchor(char *name)
{
    HtmlCell *cell;
    HtmlLine *line;
    u16 x;
    u16 y;

    if (gHtml->measure == HTML_PASS_DRAW) {
        line = &gHtml->lines[gHtml->lineNo];
        if (gHtml->tableDepth == 0) {
            x = gHtml->pen.runX + (gHtml->indent + gHtml->pen.left);
            x += Layout_CenterOffset(line);
            y = gHtml->pen.h + gHtml->pen.runY;
        } else {
            cell = &gHtml->cells[gHtml->curCell];
            x = gHtml->cellPen.runX + (cell->indent + gHtml->cellPen.left);
            Layout_CellAlign(cell);
            x += cell->alignX;
            y = cell->valignY;
            y += gHtml->cellPen.h + gHtml->cellPen.runY;
        }
        if (gHtml->inFrame == 0) {
            Draw_Anchor(x, y + line->h - gHtml->charW, (u8 *) name);
        }
    }
}

/* Lays out the current run of text (line[]) at the pen. */
void Layout_PlaceRun(HtmlLine *line)
{
    u16 x;

    if (gHtml->measure == HTML_PASS_DRAW) {
        x = gHtml->pen.runX + (gHtml->indent + gHtml->pen.left);
        x += Layout_CenterOffset(line);
        Layout_DrawRun(x, gHtml->pen.h + gHtml->pen.runY, line);
    }
}

/* The strings the tables below point at, then the parser's character
 * tables (gCharClass, gHexDigitValues, gToUpper), the entities and the
 * colour names (gHtmlColorNames). */
extern char D_00839E90[];
extern char D_00839E98[];
extern char D_00839EA0[];
extern char D_00839EA8[];
extern char D_00839EB0[];
extern char D_00839EB8[];
extern char D_00839EC0[];
extern char D_00839EC8[];
extern char D_00839ED0[];
extern char D_00839F10[];
extern char D_00839F20[];
extern char D_00839F30[];
extern char D_00839F38[];
extern char D_00839F48[];
extern char D_00839F50[];
extern char D_00839F58[];
extern char D_00839F60[];
extern u32 D_00839F68[];
extern char D_00839F78[];
extern char D_00839F80[];
extern char D_00839F90[];
extern char D_00839F98[];
extern char D_00839FA0[];
extern char D_00839FB0[];
extern char D_00839FC0[];
extern char D_00839FD0[];
extern char D_00839FE0[];
extern char D_00839FF0[];
extern char D_00839FF8[];
extern char D_0083A000[];
extern char D_0083A010[];
extern char D_0083A020[];
extern char D_0083A030[];
extern char D_0083A038[];
extern char D_0083A040[];
extern char D_0083A050[];
extern char D_0083A060[];
extern char D_0083A070[];
extern char D_0083A080[];
extern char D_0083A090[];
extern char D_0083A0A0[];
extern char D_0083A0B0[];
extern char D_0083A0C0[];
extern char D_0083A0D0[];
extern char D_0083A0E0[];
extern char D_0083A0F0[];
extern u32 D_0083A0F8[];
extern u32 D_0083A108[];
extern u32 D_0083A118[];
extern u32 D_0083A128[];
extern u32 D_0083A138[];
extern char D_0083A148[];
extern char D_0083A158[];
extern char D_0083A168[];
extern char D_0083A178[];
extern char D_0083A180[];
extern char D_0083A190[];
extern char D_0083A1A0[];
extern char D_0083A1B0[];
extern char D_0083A1C0[];
extern char D_0083A1D0[];
extern char D_0083A1D8[];
extern char D_0083A1E8[];
extern char D_0083A1F8[];
extern char D_0083A200[];
extern char D_0083A210[];
extern char D_0083A218[];
extern char D_0083A220[];
extern char D_0083A230[];
extern char D_0083A240[];
extern char D_0083A248[];
extern char D_0083A258[];
extern char D_0083A260[];
extern char D_0083A268[];
extern char D_0083A270[];
extern char D_0083A280[];
extern char D_0083A290[];
extern char D_0083A2A0[];
extern char D_0083A2B0[];
extern char D_0083A2C0[];
extern char D_0083A2D0[];
extern char D_0083A2E0[];
extern u32 D_0083A2F8[];
extern u32 D_0083A308[];
extern u32 D_0083A318[];
extern u32 D_0083A328[];
extern u32 D_0083A338[];
extern u32 D_0083A348[];
extern u32 D_0083A358[];
extern u32 D_0083A368[];
extern char D_0083A378[];
extern char D_0083A388[];
extern char D_0083A390[];
extern char D_0083A3A0[];
extern char D_0083A3A8[];
extern char D_0083A3B0[];
extern char D_0083A3C0[];
extern u32 D_0083A3D8[];
extern u32 D_0083A3E8[];
extern u32 D_0083A3F8[];
extern u32 D_0083A408[];
extern u32 D_0083A418[];
extern char D_0083A430[];
extern char D_0083A450[];
extern char D_0083A460[];
extern char D_0083A470[];
extern char D_0083A480[];
extern char D_0083A490[];
extern char D_0083A4A0[];
extern char D_0083A4B0[];
extern char D_0083A4C0[];
extern char D_0083A4C8[];
extern char D_0083A4D0[];
extern char D_0083A4D8[];
extern char D_0083A4E8[];
extern char D_0083A4F0[];
extern char D_0083A500[];
extern u32 D_0083A508[];
extern u32 D_0083A518[];
extern u32 D_0083A528[];
extern u32 D_0083A538[];
extern char D_0083A548[];
extern char D_0083A558[];
extern char D_0083A568[];
extern char D_0083A570[];
extern char D_0083A578[];
extern char D_0083A580[];
extern char D_0083A590[];
extern char D_0083A598[];
extern char D_0083A5A0[];
extern char D_0083A5B0[];
extern char D_0083A5C0[];
extern char D_0083A5D0[];
extern char D_0083A5E0[];
extern char D_0083A5E8[];
extern char D_0083A5F8[];
extern char D_0083A608[];
extern char D_0083A618[];
extern char D_0083A620[];
extern char D_0083A628[];
extern char D_0083A630[];
extern char D_0083A640[];
extern char D_0083A650[];
extern char D_0083A658[];
extern char D_0083A668[];
extern char D_0083A678[];
extern char D_0083A680[];
extern char D_0083A688[];
extern char D_0083A690[];
extern char D_0083A698[];
extern char D_0083A6A8[];
extern char D_0083A6B0[];
extern char D_0083A6B8[];
extern char D_0083A6C0[];
extern char D_0083A6D0[];
extern char D_0083A6D8[];
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
    D_00839E90, D_00839E98, D_00839EA0, D_00839EA8,
    D_00839EB0, D_00839EB8, D_00839EC0, D_00839EC8,
    D_00839ED0, NULL,
};
u32 gHtmlEntityChars[3] = {
#include "lobby/007F0228.inc"
};
HtmlColor gHtmlColorNames[148] = {
#include "lobby/007F0240.inc"
};

/* The strings (read-only data): the entities' and the colours' names,
 * and the short keywords and colours the handlers compare with. */
#pragma rodata on
char D_00839E90[0x8] = {
#include "lobby/00839E90.inc"
};
char D_00839E98[0x8] = {
#include "lobby/00839E98.inc"
};
char D_00839EA0[0x8] = {
#include "lobby/00839EA0.inc"
};
char D_00839EA8[0x8] = {
#include "lobby/00839EA8.inc"
};
char D_00839EB0[0x8] = {
#include "lobby/00839EB0.inc"
};
char D_00839EB8[0x8] = {
#include "lobby/00839EB8.inc"
};
char D_00839EC0[0x8] = {
#include "lobby/00839EC0.inc"
};
char D_00839EC8[0x8] = {
#include "lobby/00839EC8.inc"
};
char D_00839ED0[0x8] = {
#include "lobby/00839ED0.inc"
};
u32 gHtmlAmpersand[2] = {
#include "lobby/00839ED8.inc"
};
u32 gHtmlBlack[2] = {
#include "lobby/00839EE0.inc"
};
u32 gHtmlBlue[2] = {
#include "lobby/00839EE8.inc"
};
u32 gHtmlShapeRect[2] = {
#include "lobby/00839EF0.inc"
};
u32 gHtmlMethodPost[2] = {
#include "lobby/00839EF8.inc"
};
u32 gHtmlFontSizeDefault[2] = {
#include "lobby/00839F00.inc"
};
u32 gHtmlRefresh[2] = {
#include "lobby/00839F08.inc"
};
char D_00839F10[0x10] = {
#include "lobby/00839F10.inc"
};
char D_00839F20[0x10] = {
#include "lobby/00839F20.inc"
};
char D_00839F30[0x8] = {
#include "lobby/00839F30.inc"
};
char D_00839F38[0xC] = {
#include "lobby/00839F38.inc"
};
char D_00839F48[0x8] = {
#include "lobby/00839F48.inc"
};
char D_00839F50[0x8] = {
#include "lobby/00839F50.inc"
};
char D_00839F58[0x8] = {
#include "lobby/00839F58.inc"
};
char D_00839F60[0x18] = {
#include "lobby/00839F60.inc"
};
char D_00839F78[0x8] = {
#include "lobby/00839F78.inc"
};
char D_00839F80[0x10] = {
#include "lobby/00839F80.inc"
};
char D_00839F90[0x8] = {
#include "lobby/00839F90.inc"
};
char D_00839F98[0x8] = {
#include "lobby/00839F98.inc"
};
char D_00839FA0[0x10] = {
#include "lobby/00839FA0.inc"
};
char D_00839FB0[0x10] = {
#include "lobby/00839FB0.inc"
};
char D_00839FC0[0x10] = {
#include "lobby/00839FC0.inc"
};
char D_00839FD0[0x10] = {
#include "lobby/00839FD0.inc"
};
char D_00839FE0[0x10] = {
#include "lobby/00839FE0.inc"
};
char D_00839FF0[0x8] = {
#include "lobby/00839FF0.inc"
};
char D_00839FF8[0x8] = {
#include "lobby/00839FF8.inc"
};
char D_0083A000[0x10] = {
#include "lobby/0083A000.inc"
};
char D_0083A010[0x10] = {
#include "lobby/0083A010.inc"
};
char D_0083A020[0x10] = {
#include "lobby/0083A020.inc"
};
char D_0083A030[0x8] = {
#include "lobby/0083A030.inc"
};
char D_0083A038[0x8] = {
#include "lobby/0083A038.inc"
};
char D_0083A040[0x10] = {
#include "lobby/0083A040.inc"
};
char D_0083A050[0x10] = {
#include "lobby/0083A050.inc"
};
char D_0083A060[0x10] = {
#include "lobby/0083A060.inc"
};
char D_0083A070[0x10] = {
#include "lobby/0083A070.inc"
};
char D_0083A080[0x10] = {
#include "lobby/0083A080.inc"
};
char D_0083A090[0x10] = {
#include "lobby/0083A090.inc"
};
char D_0083A0A0[0x10] = {
#include "lobby/0083A0A0.inc"
};
char D_0083A0B0[0x10] = {
#include "lobby/0083A0B0.inc"
};
char D_0083A0C0[0x10] = {
#include "lobby/0083A0C0.inc"
};
char D_0083A0D0[0x10] = {
#include "lobby/0083A0D0.inc"
};
char D_0083A0E0[0x10] = {
#include "lobby/0083A0E0.inc"
};
char D_0083A0F0[0x58] = {
#include "lobby/0083A0F0.inc"
};
char D_0083A148[0xC] = {
#include "lobby/0083A148.inc"
};
char D_0083A158[0xC] = {
#include "lobby/0083A158.inc"
};
char D_0083A168[0xC] = {
#include "lobby/0083A168.inc"
};
char D_0083A178[0x8] = {
#include "lobby/0083A178.inc"
};
char D_0083A180[0x10] = {
#include "lobby/0083A180.inc"
};
char D_0083A190[0x10] = {
#include "lobby/0083A190.inc"
};
char D_0083A1A0[0x10] = {
#include "lobby/0083A1A0.inc"
};
char D_0083A1B0[0x10] = {
#include "lobby/0083A1B0.inc"
};
char D_0083A1C0[0x10] = {
#include "lobby/0083A1C0.inc"
};
char D_0083A1D0[0x8] = {
#include "lobby/0083A1D0.inc"
};
char D_0083A1D8[0xC] = {
#include "lobby/0083A1D8.inc"
};
char D_0083A1E8[0xC] = {
#include "lobby/0083A1E8.inc"
};
char D_0083A1F8[0x8] = {
#include "lobby/0083A1F8.inc"
};
char D_0083A200[0x10] = {
#include "lobby/0083A200.inc"
};
char D_0083A210[0x8] = {
#include "lobby/0083A210.inc"
};
char D_0083A218[0x8] = {
#include "lobby/0083A218.inc"
};
char D_0083A220[0x10] = {
#include "lobby/0083A220.inc"
};
char D_0083A230[0x10] = {
#include "lobby/0083A230.inc"
};
char D_0083A240[0x8] = {
#include "lobby/0083A240.inc"
};
char D_0083A248[0xC] = {
#include "lobby/0083A248.inc"
};
char D_0083A258[0x8] = {
#include "lobby/0083A258.inc"
};
char D_0083A260[0x8] = {
#include "lobby/0083A260.inc"
};
char D_0083A268[0x8] = {
#include "lobby/0083A268.inc"
};
char D_0083A270[0x10] = {
#include "lobby/0083A270.inc"
};
char D_0083A280[0x10] = {
#include "lobby/0083A280.inc"
};
char D_0083A290[0x10] = {
#include "lobby/0083A290.inc"
};
char D_0083A2A0[0x10] = {
#include "lobby/0083A2A0.inc"
};
char D_0083A2B0[0x10] = {
#include "lobby/0083A2B0.inc"
};
char D_0083A2C0[0x10] = {
#include "lobby/0083A2C0.inc"
};
char D_0083A2D0[0x10] = {
#include "lobby/0083A2D0.inc"
};
char D_0083A2E0[0x98] = {
#include "lobby/0083A2E0.inc"
};
char D_0083A378[0xC] = {
#include "lobby/0083A378.inc"
};
char D_0083A388[0x8] = {
#include "lobby/0083A388.inc"
};
char D_0083A390[0x10] = {
#include "lobby/0083A390.inc"
};
char D_0083A3A0[0x8] = {
#include "lobby/0083A3A0.inc"
};
char D_0083A3A8[0x8] = {
#include "lobby/0083A3A8.inc"
};
char D_0083A3B0[0x10] = {
#include "lobby/0083A3B0.inc"
};
char D_0083A3C0[0x70] = {
#include "lobby/0083A3C0.inc"
};
char D_0083A430[0x20] = {
#include "lobby/0083A430.inc"
};
char D_0083A450[0x10] = {
#include "lobby/0083A450.inc"
};
char D_0083A460[0x10] = {
#include "lobby/0083A460.inc"
};
char D_0083A470[0x10] = {
#include "lobby/0083A470.inc"
};
char D_0083A480[0x10] = {
#include "lobby/0083A480.inc"
};
char D_0083A490[0x10] = {
#include "lobby/0083A490.inc"
};
char D_0083A4A0[0x10] = {
#include "lobby/0083A4A0.inc"
};
char D_0083A4B0[0x10] = {
#include "lobby/0083A4B0.inc"
};
char D_0083A4C0[0x8] = {
#include "lobby/0083A4C0.inc"
};
char D_0083A4C8[0x8] = {
#include "lobby/0083A4C8.inc"
};
char D_0083A4D0[0x8] = {
#include "lobby/0083A4D0.inc"
};
char D_0083A4D8[0xC] = {
#include "lobby/0083A4D8.inc"
};
char D_0083A4E8[0x8] = {
#include "lobby/0083A4E8.inc"
};
char D_0083A4F0[0x10] = {
#include "lobby/0083A4F0.inc"
};
char D_0083A500[0x48] = {
#include "lobby/0083A500.inc"
};
char D_0083A548[0xC] = {
#include "lobby/0083A548.inc"
};
char D_0083A558[0xC] = {
#include "lobby/0083A558.inc"
};
char D_0083A568[0x8] = {
#include "lobby/0083A568.inc"
};
char D_0083A570[0x8] = {
#include "lobby/0083A570.inc"
};
char D_0083A578[0x8] = {
#include "lobby/0083A578.inc"
};
char D_0083A580[0x10] = {
#include "lobby/0083A580.inc"
};
char D_0083A590[0x8] = {
#include "lobby/0083A590.inc"
};
char D_0083A598[0x8] = {
#include "lobby/0083A598.inc"
};
char D_0083A5A0[0x10] = {
#include "lobby/0083A5A0.inc"
};
char D_0083A5B0[0x10] = {
#include "lobby/0083A5B0.inc"
};
char D_0083A5C0[0x10] = {
#include "lobby/0083A5C0.inc"
};
char D_0083A5D0[0x10] = {
#include "lobby/0083A5D0.inc"
};
char D_0083A5E0[0x8] = {
#include "lobby/0083A5E0.inc"
};
char D_0083A5E8[0xC] = {
#include "lobby/0083A5E8.inc"
};
char D_0083A5F8[0xC] = {
#include "lobby/0083A5F8.inc"
};
char D_0083A608[0xC] = {
#include "lobby/0083A608.inc"
};
char D_0083A618[0x8] = {
#include "lobby/0083A618.inc"
};
char D_0083A620[0x8] = {
#include "lobby/0083A620.inc"
};
char D_0083A628[0x8] = {
#include "lobby/0083A628.inc"
};
char D_0083A630[0x10] = {
#include "lobby/0083A630.inc"
};
char D_0083A640[0x10] = {
#include "lobby/0083A640.inc"
};
char D_0083A650[0x8] = {
#include "lobby/0083A650.inc"
};
char D_0083A658[0xC] = {
#include "lobby/0083A658.inc"
};
char D_0083A668[0xC] = {
#include "lobby/0083A668.inc"
};
char D_0083A678[0x8] = {
#include "lobby/0083A678.inc"
};
char D_0083A680[0x8] = {
#include "lobby/0083A680.inc"
};
char D_0083A688[0x8] = {
#include "lobby/0083A688.inc"
};
char D_0083A690[0x8] = {
#include "lobby/0083A690.inc"
};
char D_0083A698[0xC] = {
#include "lobby/0083A698.inc"
};
char D_0083A6A8[0x8] = {
#include "lobby/0083A6A8.inc"
};
char D_0083A6B0[0x8] = {
#include "lobby/0083A6B0.inc"
};
char D_0083A6B8[0x8] = {
#include "lobby/0083A6B8.inc"
};
char D_0083A6C0[0x10] = {
#include "lobby/0083A6C0.inc"
};
char D_0083A6D0[0x8] = {
#include "lobby/0083A6D0.inc"
};
char D_0083A6D8[0xC] = {
#include "lobby/0083A6D8.inc"
};
char gHtmlHash[0x8] = {
#include "lobby/0083A6E8.inc"
};
#pragma rodata off
