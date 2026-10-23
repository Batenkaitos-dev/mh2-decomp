/* The HTML layout: lines (the pending run of text drawn, centred or
 * placed on its line, an anchor's place, the next line), runs of text,
 * <HR>, <INPUT>, <SELECT>, <IMG>, image maps, fonts, <BODY>'s colours. */
#include "common.h"
#include "html.h"
#include "browser.h"

/* Compiled at optimization level 4, as the lobby's HTML parser
 * (Html_ParseSome's file): every function of the file matches with it. */
#pragma optimization_level 4

void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
s32 strlen(const char *s);

s32 strcmp(const char *a, const char *b); /* strcmp */

extern char gHtmlMapNoSrc[];
s32 Html_ParseColor(char *s);

/* Raises *dst to value (inlined, as in the table handlers: the game's
 * callers keep dst in a register of its own for the store). */
static inline void Html_MaxU16(u16 *dst, s32 value)
{
    if (*dst < value) {
        *dst = value;
    }
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

/* Goes to the next line (staying on the last of HTML_LINES). */
void Layout_NextLine(void)
{
    u16 line;

    line = gHtml->lineNo;
    gHtml->lineNo = line + 1;
    if (line >= HTML_LINES - 1) {
        gHtml->lineNo = HTML_LINES - 1;
    }
}

/* Goes to the next line, and *lp to its entry. */
void Layout_NextLineEntry(HtmlLine **linePtr)
{
    u16 line;

    line = gHtml->lineNo;
    gHtml->lineNo = line + 1;
    if (line >= HTML_LINES - 1) {
        gHtml->lineNo = HTML_LINES - 1;
        return;
    }
    (*linePtr)++;
}

/* <HR>: a rule of hrSize x hrWidth (the page's width when not given),
 * centred when asked, on a line of its own. */
void Layout_Rule(s32 center)
{
    HtmlLine *line;
    u16 x;
    s32 room;
    u16 *w;
    s32 y;

    if (gHtml->tableDepth != 0) {
        Layout_CellRule(center);
        return;
    }
    line = &gHtml->lines[gHtml->lineNo];
    x = gHtml->indent + gHtml->pen.runX;
    w = &gHtml->hrWidth;
    if (*w == 0) {
        if (gHtml->measure != HTML_PASS_DRAW) {
            *w = PAGE_VIEW_W;
        } else {
            *w = gHtml->width;
        }
    } else if (*w < 2) {
        *w = 2;
    }
    gHtml->pen.runY += 10;
    if (gHtml->measure == HTML_PASS_DRAW) {
        if (center != 0) {
            room = gHtml->width - gHtml->indent;
            if (line->w < room) {
                x += (room - line->w) / 2;
            }
        }
        if (gHtml->inFrame == 0) {
            y = gHtml->pen.h + gHtml->pen.runY;
            Draw_Rule(x, y, x + gHtml->hrWidth, gHtml->hrSize + y);
        }
    }
    gHtml->pen.runY += gHtml->hrSize + 10;
    gHtml->pen.y = gHtml->pen.runY;
    Html_NoteBottom(&gHtml->pen);
    if (gHtml->measure == HTML_PASS_LAYOUT) {
        line->w = x + gHtml->hrWidth;
        line->h = gHtml->hrSize + 20;
    }
    gHtml->hrSize = 0;
    Layout_NextLine();
}

/* Moves the pen to the start of the next line. */
void Layout_PenNewLine(HtmlPen *pen, HtmlLine **linePtr)
{
    pen->x = 0;
    pen->y += (*linePtr)->h;
    pen->runX = pen->x;
    pen->runY = pen->y;
    pen->lineUsed = 0;
}

/* Wraps the line when a run of width w at x would pass the page's width;
 * the new line's first x. */
u16 Layout_Wrap(HtmlLine **linePtr, u16 x)
{
    if (gHtml->measure == HTML_PASS_LAYOUT) {
        (*linePtr)->w = gHtml->pen.left + x;
    }
    gHtml->lineLen = 0;
    Layout_PenNewLine(&gHtml->pen, linePtr);
    Layout_NextLineEntry(linePtr);
    Html_NoteBottom(&gHtml->pen);
    return gHtml->pen.runX + (gHtml->indent + gHtml->pen.left);
}

/* Before an object w wide at (*xPtr, *yPtr): in a table, the cell's own
 * wrap; on the drawing pass, moves *xPtr by the line's left edge and
 * centring; while measuring, wraps to a new line when the object would
 * pass the page's width. */
void Layout_WrapBefore(u16 *xPtr, u16 *yPtr, u16 w, HtmlLine **linePtr)
{
    s32 end;
    u16 x;

    if (gHtml->tableDepth != 0) {
        Layout_CellWrapBefore(xPtr, yPtr, w, linePtr);
        return;
    }
    x = *xPtr;
    end = (u16)(x + w);
    if (gHtml->measure == HTML_PASS_DRAW) {
        *xPtr += (u16)(gHtml->pen.left + Layout_CenterOffset(*linePtr));
        return;
    }
    if (gHtml->pen.runX != 0 && gHtml->width < (u16)end) {
        *xPtr = Layout_Wrap(linePtr, x);
        *yPtr = gHtml->pen.h + gHtml->pen.runY;
    }
}

/* The key code (1-4 or 17) an access key gives a link, an input or a
 * map area (the key a page comment set, HtmlState.accessKey), which it
 * clears. */
u8 Html_MapKeyCode(u8 *key)
{
    u8 keyCode = 0;

    switch (*key) {
    case 8:
        keyCode = 3;
        break;
    case 11:
        keyCode = 2;
        break;
    case 9:
    case 10:
        keyCode = 1;
        break;
    case 40:
        keyCode = 4;
        break;
    case 44:
        keyCode = 1;
        break;
    case 45:
        keyCode = 2;
        break;
    case 46:
        keyCode = 3;
        break;
    case 47:
        keyCode = 17;
        break;
    }
    *key = 0;
    return keyCode;
}

/* Lays out and draws an <INPUT> of w characters (w pixels for an image) at
 * x, y; the pen moves past it. */
void Layout_Input(u16 x, u16 y, u16 *runX, u16 *penX, u16 w)
{
    HtmlLine *line;
    u16 pw = 0;
    u8 key;

    line = &gHtml->lines[gHtml->lineNo];
    if (gHtml->measure == HTML_PASS_LAYOUT) {
        if (gHtml->input.type != HTML_INPUT_IMAGE) {
            if (line->h < 20) {
                line->h = 20;
            }
        } else if (line->h < gHtml->input.height) {
            line->h = gHtml->input.height;
        }
    }
    switch (gHtml->input.type) {
    case HTML_INPUT_TEXT:
        if (w < 2) {
            w = 2;
        }
        pw = w * 8 + 8;
        Layout_WrapBefore(&x, &y, pw, &line);
        if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
            key = Html_MapKeyCode(&gHtml->accessKey);
            Draw_TextInput(x, y + line->h - 20, x + pw, y + line->h, (u8 *) gHtml->input.name, (u8 *) gHtml->input.value,
                          gHtml->input.maxLength, key);
        }
        break;
    case HTML_INPUT_PASSWORD:
        if (w < 2) {
            w = 2;
        }
        pw = w * 8 + 8;
        Layout_WrapBefore(&x, &y, pw, &line);
        if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
            key = Html_MapKeyCode(&gHtml->accessKey);
            Draw_PasswordInput(x, y + line->h - 20, x + pw, y + line->h, (u8 *) gHtml->input.name, (u8 *) gHtml->input.value,
                          gHtml->input.maxLength, key);
        }
        break;
    case HTML_INPUT_SUBMIT:
        if (w < 2) {
            w = 2;
        }
        pw = w * 8 + 8;
        Layout_WrapBefore(&x, &y, pw, &line);
        if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
            Draw_Submit(x, y + line->h - 20, x + pw, y + line->h, (u8 *) gHtml->input.value);
        }
        break;
    case HTML_INPUT_RESET:
        if (w < 2) {
            w = 2;
        }
        pw = w * 8 + 8;
        Layout_WrapBefore(&x, &y, pw, &line);
        if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
            Draw_Reset(x, y + line->h - 20, x + pw, y + line->h, (u8 *) gHtml->input.value);
        }
        break;
    case HTML_INPUT_RADIO:
        pw = 20;
        Layout_WrapBefore(&x, &y, 20, &line);
        if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
            key = Html_MapKeyCode(&gHtml->accessKey);
            Draw_Radio(x, y + line->h - 20, gHtml->input.checked, (u8 *) gHtml->input.name,
                          (u8 *) gHtml->input.value, key);
        }
        break;
    case HTML_INPUT_CHECKBOX:
        pw = 20;
        Layout_WrapBefore(&x, &y, 20, &line);
        if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
            key = Html_MapKeyCode(&gHtml->accessKey);
            Draw_Checkbox(x, y + line->h - 20, gHtml->input.checked, (u8 *) gHtml->input.name,
                          (u8 *) gHtml->input.value, key);
        }
        break;
    case HTML_INPUT_HIDDEN:
        if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
            Draw_Hidden((u8 *) gHtml->input.name, (u8 *) gHtml->input.value);
        }
        break;
    case HTML_INPUT_IMAGE:
        pw = w;
        Layout_WrapBefore(&x, &y, w, &line);
        if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
            Draw_InputImage(x, y + line->h - gHtml->input.height, x + pw, y + line->h, (u8 *) gHtml->input.name,
                          (u8 *) gHtml->input.src, (u8 *) gHtml->input.alt);
        }
        break;
    }
    *runX = *penX = *runX + pw;
}

/* <INPUT ...>'s ">": the control's width (characters, or pixels for an
 * image) and its layout. */
void Layout_InputEnd(void)
{
    u16 w = 0;

    switch (gHtml->input.type) {
    case HTML_INPUT_TEXT:
    case HTML_INPUT_PASSWORD:
        if (gHtml->input.size) {
            w = gHtml->input.size;
        } else {
            w = 16;
        }
        break;
    case HTML_INPUT_SUBMIT:
    case HTML_INPUT_RESET:
    case HTML_INPUT_RADIO:
    case HTML_INPUT_CHECKBOX:
        if (gHtml->input.size) {
            w = gHtml->input.size;
        } else {
            w = strlen(gHtml->input.value);
        }
        break;
    case HTML_INPUT_HIDDEN:
        break;
    case HTML_INPUT_IMAGE:
        w = gHtml->input.width;
        break;
    default:
        return;
    }
    if (gHtml->tableDepth != 0) {
        Layout_CellInput(w);
        return;
    }
    Layout_Input(gHtml->indent + gHtml->pen.runX, gHtml->pen.runY, &gHtml->pen.runX,
                  &gHtml->pen.x, w);
    if (gHtml->measure == HTML_PASS_MEASURE) {
        Html_MaxU16(&gHtml->pageWidth, gHtml->pen.left + gHtml->pen.x);
    }
}

/* <SELECT ...>'s ">": the list begins. */
void Layout_SelectBegin(void)
{
    gHtml->selectWidth = 0;
    if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
        Draw_SelectBegin((u8 *) gHtml->select.name);
    }
}

/* An <OPTION>'s text (trimmed) is added to the <SELECT>. */
void Layout_Option(u8 *text)
{
    u16 len;
    u8 *src;
    u8 ch;

    while (!(ch = *text) == 0 && (gCharClass[ch] & HTML_CHAR_BLANK)) {
        text++;
    }
    len = strlen((char *)text);
    if (len) {
        for (src = text + (len - 1); *src == ' ' || *src == '\t'; src--) {
            *src = 0;
        }
    }
    if (gHtml->selectWidth < len) {
        gHtml->selectWidth = len;
    }
    if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
        Draw_Option(text, gHtml->option.value, gHtml->option.selected);
        gHtml->option.selected = 0;
    }
}

/* </SELECT>: the list's box (8 pixels a character of the longest option,
 * plus 24) is laid out and drawn. */
void Layout_SelectEnd(void)
{
    HtmlLine *line;
    u16 x;
    u16 x2;
    u16 off;
    u16 rx;

    if (gHtml->tableDepth != 0) {
        Layout_CellSelectEnd();
        return;
    }
    line = &gHtml->lines[gHtml->lineNo];
    rx = gHtml->pen.runX;
    x = gHtml->indent + rx;
    if (rx != 0 && gHtml->width < x + (gHtml->selectWidth * 8 + 24)) {
        x = Layout_Wrap(&line, x);
    } else {
        x += gHtml->pen.left;
    }
    gHtml->pen.x += gHtml->selectWidth * 8 + 24;
    x2 = gHtml->pen.x + (gHtml->indent + gHtml->pen.left);
    if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
        off = Layout_CenterOffset(line);
        Draw_SelectEnd(x + off, gHtml->pen.runY + line->h - 20, x2 + off, gHtml->pen.runY + line->h);
    }
    gHtml->pen.runX = gHtml->pen.x;
    gHtml->pen.lineUsed = 1;
    if (gHtml->measure == HTML_PASS_LAYOUT && line->h < 20) {
        line->h = 20;
    }
}

/* Draws an <IMG> at x, y on a line (in a link: an image map area); an
 * image with USEMAP places its map. */
void Layout_DrawImage(u16 x, u16 y, HtmlLine *line)
{
    s32 key;
    s32 b;
    s32 b2;

    if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
        switch (gHtml->link) {
        case 0:
            Draw_Image(x, y + line->h - gHtml->imgHeight, x + gHtml->imgWidth, y + line->h,
                          (u8 *) gHtml->imgSrc, (u8 *) gHtml->imgAlt);
            if ((u8)gHtml->imgUsemap[0] != 0) {
                memcpy(gHtml->maps[gHtml->mapCount].usemap, gHtml->imgUsemap, 256);
                gHtml->maps[gHtml->mapCount].x = x;
                gHtml->maps[gHtml->mapCount].y = y + line->h - gHtml->imgHeight;
                gHtml->mapCount++;
            }
            break;
        case 1:
            key = Html_MapKeyCode(&gHtml->accessKey);
            b = y + line->h;
            Draw_ImageMap(x, b - gHtml->imgHeight, x + gHtml->imgWidth, b,
                          (u8 *) gHtml->imgSrc, (u8 *) gHtml->anchor, (u8 *) gHtml->imgAlt, key);
            break;
        case 2:
            key = Html_MapKeyCode(&gHtml->accessKey);
            b2 = y + line->h;
            Draw_ImageMap(x, b2 - gHtml->imgHeight, x + gHtml->imgWidth, b2,
                          (u8 *) gHtml->imgSrc, (u8 *) gHtml->href, (u8 *) gHtml->imgAlt, key);
            break;
        }
    }
}

/* "": the target of an image map area (Layout_MapArea). */
INCLUDE_RODATA("asm/lobby/nonmatchings/layout_text", gHtmlMapNoSrc);

/* On the drawing pass, draws an <AREA> of the image map named by <MAP>
 * (HtmlState.mapName) as a link over the image that uses it, with its
 * access key. */
void Layout_MapArea(void)
{
    s32 key;
    s32 i;

    if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
        for (i = 0; i < 16; i++) {
            if (strcmp(&gHtml->maps[i].usemap[1], gHtml->mapName) == 0) {
                key = Html_MapKeyCode(&gHtml->accessKey);
                Draw_ImageMap(gHtml->maps[i].x + gHtml->area.coords[0],
                              gHtml->maps[i].y + gHtml->area.coords[1],
                              gHtml->maps[i].x + gHtml->area.coords[2],
                              gHtml->maps[i].y + gHtml->area.coords[3], (u8 *) gHtmlMapNoSrc,
                              (u8 *) gHtml->area.href, (u8 *) gHtml->area.alt, key | 0x80);
                return;
            }
        }
    }
}

/* Clears <IMG>'s attributes (30 x 30 when not given). */
void Html_ClearImgAttrs(void)
{
    gHtml->imgWidth = gHtml->imgHeight = 30;
    memset(gHtml->imgSrc, 0, sizeof(gHtml->imgSrc));
    memset(gHtml->imgAlt, 0, sizeof(gHtml->imgAlt));
    memset(gHtml->imgUsemap, 0, sizeof(gHtml->imgUsemap));
}

/* <IMG>'s ">": the image is laid out (a new line when it doesn't fit)
 * and drawn. */
void Layout_Image(void)
{
    HtmlLine *line;
    u16 x;

    if (gHtml->tableDepth != 0) {
        Layout_CellImage();
        return;
    }
    line = &gHtml->lines[gHtml->lineNo];
    x = gHtml->indent + gHtml->pen.runX;
    if (gHtml->measure == HTML_PASS_DRAW) {
        x += Layout_CenterOffset(line);
    }
    if (gHtml->pen.runX != 0 && gHtml->width < x + gHtml->imgWidth) {
        x = Layout_Wrap(&line, x);
        if (gHtml->measure == HTML_PASS_DRAW) {
            x += Layout_CenterOffset(line);
        }
    } else {
        x += gHtml->pen.left;
    }
    if (gHtml->measure == HTML_PASS_MEASURE) {
        Html_MaxU16(&gHtml->pageWidth, x + gHtml->imgWidth);
    }
    Layout_DrawImage(x, gHtml->pen.h + gHtml->pen.runY, line);
    gHtml->pen.runX = gHtml->pen.x = gHtml->pen.runX + gHtml->imgWidth;
    gHtml->pen.lineUsed = 1;
    if (gHtml->measure == HTML_PASS_LAYOUT) {
        Html_MaxU16(&line->w, gHtml->pen.x);
        if (line->h < gHtml->imgHeight) {
            line->h = gHtml->imgHeight;
        }
    }
}

/* Ends the line: the run so far is laid out and the pen goes down. */
void Layout_BreakLine(HtmlLine **linePtr)
{
    if (gHtml->measure == HTML_PASS_LAYOUT) {
        (*linePtr)->w = gHtml->pen.x;
    }
    gHtml->pen.runW = gHtml->pen.x - gHtml->pen.runX;
    gHtml->line[gHtml->lineLen] = 0;
    Layout_PlaceRun(*linePtr);
    gHtml->lineLen = 0;
    Layout_PenNewLine(&gHtml->pen, linePtr);
    Layout_NextLineEntry(linePtr);
    Html_NoteBottom(&gHtml->pen);
}

/* Adds a full-width (two-byte) character to the run. */
void Layout_AddWideChar(u8 **src, HtmlPen *pen)
{
    gHtml->line[gHtml->lineLen] = **src;
    gHtml->lineLen++;
    (*src)++;
    gHtml->line[gHtml->lineLen] = **src;
    gHtml->lineLen++;
    pen->x += gHtml->charW;
}

/* Adds a space for &nbsp; to the run. */
void Layout_AddNbsp(u8 **src, HtmlPen *pen)
{
    gHtml->line[gHtml->lineLen] = ' ';
    gHtml->lineLen++;
    pen->x += gHtml->halfW;
}

/* Adds a half-width character to the run (a tab as a space). */
void Layout_AddChar(u8 **src, HtmlPen *pen)
{
    u8 ch = **src;

    if (ch != '\t') {
        gHtml->line[gHtml->lineLen] = ch;
    } else {
        gHtml->line[gHtml->lineLen] = ' ';
    }
    gHtml->lineLen++;
    pen->x += gHtml->halfW;
}

/* Lays out text (spaces collapsed unless in <PRE>) into runs, breaking
 * lines at the page's width; -1 when there is nothing to lay out. */
s32 Layout_Text(u8 *text, HtmlLine **linePtr)
{
    u8 *src;
    u8 space;
    u16 indent;

    src = Html_SkipSpaces(text);
    space = 0;
    indent = gHtml->indent;
    if (*src == 0) {
        return -1;
    }
    gHtml->paragraph = 0;
    for (; *src != 0; src++) {
        if (*src == ' ' || *src == '\t') {
            if (space != 0 && gHtml->flags == 0) {
                continue;
            }
        }
        if (*src == ' ' || *src == '\t') {
            space = 1;
        } else {
            space = 0;
        }
        if (gCharClass[*src] & HTML_CHAR_SJIS_LEAD) {
            if (gHtml->flags == 0 && gHtml->width < gHtml->charW + (indent + gHtml->pen.x)) {
                Layout_BreakLine(linePtr);
            }
            Layout_AddWideChar(&src, &gHtml->pen);
        } else if (*src == HTML_CHAR_NBSP) {
            if (gHtml->width < gHtml->halfW + (indent + gHtml->pen.x)) {
                Layout_BreakLine(linePtr);
            }
            Layout_AddNbsp(&src, &gHtml->pen);
        } else {
            if (gHtml->flags == 0 && gHtml->width < gHtml->halfW + (indent + gHtml->pen.x)) {
                Layout_BreakLine(linePtr);
            }
            Layout_AddChar(&src, &gHtml->pen);
        }
        if (gHtml->measure == HTML_PASS_LAYOUT) {
            if ((*linePtr)->h < gHtml->charW) {
                (*linePtr)->h = gHtml->charW;
            }
        }
    }
    return 0;
}

/* Lays out the run of text so far and starts a new one at the pen. */
void Layout_FlushRun(HtmlLine *line)
{
    if (gHtml->lineLen != 0) {
        gHtml->pen.runW = gHtml->pen.x - gHtml->pen.runX;
        gHtml->line[gHtml->lineLen] = 0;
        Layout_PlaceRun(line);
        gHtml->pen.runX = gHtml->pen.x;
        gHtml->lineLen = 0;
        gHtml->paragraph = 0;
    }
}

/* Ends a line: its width (measuring), the pen to the next line. */
void Layout_EndLine(HtmlLine **linePtr)
{
    s8 pass = gHtml->measure;

    if (pass == -10) {
        Html_MaxU16(&gHtml->pageWidth, gHtml->pen.x);
    } else if (pass == 1) {
        (*linePtr)->w = gHtml->pen.x;
        if ((*linePtr)->h == 0) {
            (*linePtr)->h = gHtml->charW;
        }
    }
    Layout_PenNewLine(&gHtml->pen, linePtr);
    Layout_NextLineEntry(linePtr);
}

/* FAKEMATCH: the length is read through a copied pointer (tl, then len = tl); with
 * gHtml->textLen read directly, or through one pointer, last and the loop hoisted
 * &textLen swap a1 and a2. Optimization level 3 (the file is at 4). */
#pragma optimization_level 3
/* Lays out the pending text when text[] is full, keeping a Shift-JIS
 * lead byte left at its end for the next text. */
void Layout_TextFull(u8 *text)
{
    u8 i = 0;
    u8 ch = 0;
    s32 n;
    s32 *len;
    s32 *lenPtr;
    s32 last;
    u8 next;
    u8 *src;

    lenPtr = &gHtml->textLen;
    len = lenPtr;
    n = *len;
    last = n - 1;
    for (i = 0; i < *len - 1; i++) {
        if (gCharClass[text[i]] & HTML_CHAR_SJIS_LEAD) {
            i++;
        }
    }
    if (i >= last && i != n) {
        src = (u8 *)(n + (s32)text);
        next = src[-1];
        if (gCharClass[next] & HTML_CHAR_SJIS_LEAD) {
            src[-1] = 0;
            ch = next;
        }
    }
    Layout_Flush(text);
    if (ch == 0) {
        gHtml->textLen = 0;
        text[0] = 0;
    } else {
        gHtml->textLen = 1;
        text[0] = ch;
        text[1] = 0;
    }
}
#pragma optimization_level reset

/* Lays out the pending text (no line break). */
void Layout_Flush(u8 *text)
{
    HtmlLine *line;

    if (gHtml->tableDepth != 0) {
        Layout_CellFlush(text);
        return;
    }
    line = &gHtml->lines[gHtml->lineNo];
    Layout_Text(text, &line);
    Layout_FlushRun(line);
    gHtml->pen.runX = gHtml->pen.x;
    gHtml->pen.runY = gHtml->pen.y;
    Html_NoteBottom(&gHtml->pen);
    gHtml->textLen = 0;
    *text = 0;
}

/* Lays out the pending text and breaks the line (<BR>). */
void Layout_FlushBreak(u8 *text)
{
    HtmlLine *line;

    if (gHtml->tableDepth != 0) {
        Layout_CellFlushBreak(text);
        return;
    }
    line = &gHtml->lines[gHtml->lineNo];
    Layout_Text(text, &line);
    Layout_FlushRun(line);
    Layout_EndLine(&line);
    Html_NoteBottom(&gHtml->pen);
    gHtml->textLen = 0;
    *text = 0;
}

/* Lays out the pending text and breaks the line unless it is empty. */
void Layout_FlushBlock(u8 *text)
{
    HtmlLine *line;

    if (gHtml->tableDepth != 0) {
        Layout_CellFlushBlock(text);
        return;
    }
    line = &gHtml->lines[gHtml->lineNo];
    Layout_Text(text, &line);
    Layout_FlushRun(line);
    if (gHtml->pen.runX != 0) {
        Layout_EndLine(&line);
    }
    Html_NoteBottom(&gHtml->pen);
    gHtml->textLen = 0;
    *text = 0;
}

/* Clears <INPUT>'s attributes. */
void Html_ClearInputAttrs(void)
{
    memset(&gHtml->input, 0, sizeof(HtmlInput));
    gHtml->input.maxLength = 256;
    gHtml->input.width = gHtml->input.height = 30;
}

/* Clears <FONT>'s attributes. */
void Html_ClearFontAttrs(void)
{
    memset(&gHtml->fontAttr, 0, sizeof(HtmlFontAttr));
}

/* Pushes a font (<FONT>, <Hn>, <SMALL>, <BIG>): the colour and size read,
 * or the current ones. */
void Html_PushFont(void)
{
    u8 bold;
    HtmlState *html;
    s32 color;
    char *sizeText;

    if (gHtml->tableDepth != 0) {
        bold = gHtml->cells[gHtml->curCell].bold;
    } else {
        bold = gHtml->bold;
    }
    if (gHtml->fontDepth > 14) {
        gHtml->fontDepth = 14;
    }
    if ((u8)gHtml->fontAttr.color[0] == 0) {
        gHtml->fontColor[gHtml->fontDepth + 1] = gHtml->fontColor[gHtml->fontDepth];
    } else {
        color = Html_ParseColor(gHtml->fontAttr.color);
        if (color < 0) {
            gHtml->fontColor[gHtml->fontDepth + 1] = gHtml->fontColor[gHtml->fontDepth];
        } else {
            gHtml->fontColor[gHtml->fontDepth + 1] = color;
        }
    }
    html = gHtml;
    sizeText = html->fontAttr.size;
    if ((u8)*sizeText == 0) {
        html->fontSize[html->fontDepth + 1] = html->fontSize[html->fontDepth];
    } else {
        html->fontSize[html->fontDepth + 1] = Html_ParseFontSize(sizeText);
    }
    if (gHtml->fontDepth < 14) {
        gHtml->fontDepth++;
    }
    Html_SetCharWidths(bold);
}

/* Pops a font (</FONT>, </Hn>, </SMALL>, </BIG>). */
void Html_PopFont(void)
{
    u8 bold;

    if (gHtml->tableDepth != 0) {
        bold = gHtml->cells[gHtml->curCell].bold;
    } else {
        bold = gHtml->bold;
    }
    if (gHtml->fontDepth > 0) {
        gHtml->fontDepth--;
    } else {
        gHtml->fontDepth = 0;
    }
    Html_SetCharWidths(bold);
}

/* <BODY ...>'s ">": the page's colours and background. */
void Html_ApplyBody(void)
{
    u8 bold;
    s32 color;

    bold = gHtml->bold;
    gHtml->fontDepth = 0;
    gHtml->fontSize[gHtml->fontDepth] = 3;
    color = Html_ParseColor(gHtml->textColor);
    if (color >= 0) {
        gHtml->fontColor[gHtml->fontDepth] = color;
    }
    color = Html_ParseColor(gHtml->bgColor);
    if (color >= 0) {
        gHtml->bgColorValue = color;
    }
    color = Html_ParseColor(gHtml->linkColor);
    if (color >= 0) {
        gHtml->linkColorValue = color;
    }
    Html_SetCharWidths(bold);
    if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
        Html_SetBgColor(gHtml->bgColorValue);
        Draw_BodyBackground((u8 *) gHtml->background);
    }
}

/* </TITLE>: the pending text is the title. */
void Html_SetTitle(u8 *text)
{
    if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
        Draw_Title(text);
    }
    gHtml->textLen = 0;
    *text = 0;
}

