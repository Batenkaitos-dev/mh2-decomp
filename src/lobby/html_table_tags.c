/* The HTML tag handlers: <META ...> and its attributes, <PRE>, <DIV>, <SMALL>,
 * <BIG>, the entities, <TABLE>, <TR>, <TH>, <TD> and their attributes, and
 * </TD>. */
#include "common.h"
#include "html.h"
#include "browser.h"
#include "libc.h"

s32 Html_ParseColor(char *s);

extern HtmlKeyword gHtmlAlignKeywords[4]; /* ALIGN: LEFT CENTER MIDDLE RIGHT */
extern HtmlKeyword gHtmlValignKeywords[4]; /* VALIGN: MIDDLE CENTER TOP BOTTOM */
extern char gHtmlRefresh[]; /* "REFRESH" */

/* <META ...> */
s32 Html_TagMeta(u8 **cur, u8 *text)
{
    memset(&gHtml->meta, 0, sizeof(HtmlMeta));
    return 0;
}

/* Raises *p to v (inlined: the game's callers keep p in a register of its
 * own for the store). */
static inline void Html_MaxU16(u16 *dst, s32 value)
{
    if (*dst < value) {
        *dst = value;
    }
}

/* <META HTTP-EQUIV=...> */
s32 Html_AttrMetaHttpEquiv(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrKeyword(cur, buf, HTML_ATTR_MAX);
    if (strncmp(buf, gHtmlRefresh, 7) == 0) {    /* its seven letters */
        gHtml->meta.refresh = 1;
    }
    return 0;
}

/* <META CONTENT=...> */
s32 Html_AttrMetaContent(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    if (gHtml->meta.refresh != 0) {
        Html_ReadMetaContent(cur, buf, HTML_ATTR_MAX);
        gHtml->meta.seconds = Html_ParseNumber(buf);
    }
    return 0;
}

/* <META URL=...> */
s32 Html_AttrMetaUrl(u8 **cur, u8 *text)
{
    if (gHtml->meta.refresh != 0) {
        Html_ReadAttrValue(cur, gHtml->meta.url, HTML_ATTR_MAX);
    }
    return 0;
}

/* <META ...>'s ">": a refresh. */
s32 Html_TagMetaEnd(u8 **cur, u8 *text)
{
    if (gHtml->meta.refresh != 0 && gHtml->inFrame == 0) {
        if (gHtml->measure == HTML_PASS_DRAW) {
            Html_Refresh(gHtml->meta.seconds, gHtml->meta.url);
        } else if (gHtml->meta.seconds == 0) {
            Html_Refresh(gHtml->meta.seconds, gHtml->meta.url);
        }
    }
    return 0;
}

/* <PRE> */
s32 Html_TagPre(u8 **cur, u8 *text)
{
    Layout_FlushBreak(text);
    if (gHtml->tableDepth != 0) {
        gHtml->cells[gHtml->curCell].flags |= HTML_FLAG_PRE;
    } else {
        gHtml->flags |= HTML_FLAG_PRE;
    }
    gHtml->pre = 1;
    Layout_FlushBreak(text);
    return 0;
}

/* </PRE> */
s32 Html_TagEndPre(u8 **cur, u8 *text)
{
    Layout_FlushBreak(text);
    if (gHtml->tableDepth != 0) {
        gHtml->cells[gHtml->curCell].flags &= ~HTML_FLAG_PRE;
    } else {
        gHtml->flags &= ~HTML_FLAG_PRE;
    }
    gHtml->pre = 0;
    return 0;
}

/* <DIV ...> */
s32 Html_TagDiv(u8 **cur, u8 *text)
{
    gHtml->divAlign[gHtml->divDepth] = 0;
    gHtml->divDepth++;
    Layout_Flush(text);
    return 0;
}

/* <DIV ALIGN=...> */
s32 Html_AttrDivAlign(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];
    HtmlState *html;
    HtmlCell *cell;
    u8 packed;
    u8 depth;

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    html = gHtml;
    if (html->divDepth <= HTML_DIV_DEPTH_MAX) {
        html->divAlign[html->divDepth - 1] = Html_LookupKeyword(buf, gHtmlAlignKeywords, sizeof(gHtmlAlignKeywords));
        if (gHtml->divAlign[gHtml->divDepth - 1] == HTML_ALIGN_CENTER) {
            if (gHtml->tableDepth != 0) {
                cell = &gHtml->cells[gHtml->curCell];
                packed = cell->center;
                depth = packed & HTML_DIV_CENTER_DEPTH;
                if (depth != HTML_DIV_CENTER_DEPTH) {
                    depth += HTML_DIV_CENTER_STEP;
                    cell->center = (packed & HTML_CENTER_DEPTH) | depth;
                }
                Layout_CellAlign(cell);
            } else {
                HtmlState *state = gHtml;
                packed = state->center;
                depth = packed & HTML_DIV_CENTER_DEPTH;
                if (depth != HTML_DIV_CENTER_DEPTH) {
                    depth += HTML_DIV_CENTER_STEP;
                    state->center = (packed & HTML_CENTER_DEPTH) | depth;
                }
            }
        }
    }
    return 0;
}

/* </DIV> */
s32 Html_TagEndDiv(u8 **cur, u8 *text)
{
    HtmlCell *cell;
    u8 packed;
    u8 depth;
    u8 divLevel;

    if (gHtml->divDepth > 0) {
        gHtml->divDepth--;
        divLevel = gHtml->divDepth;
        if (divLevel < HTML_DIV_DEPTH_MAX && gHtml->divAlign[divLevel] == HTML_ALIGN_CENTER) {
            if (gHtml->tableDepth != 0) {
                cell = &gHtml->cells[gHtml->curCell];
                packed = cell->center;
                depth = packed & HTML_DIV_CENTER_DEPTH;
                if (depth != 0) {
                    depth -= HTML_DIV_CENTER_STEP;
                    cell->center = (packed & HTML_CENTER_DEPTH) | depth;
                }
            } else {
                HtmlState *html = gHtml;
                packed = html->center;
                depth = packed & HTML_DIV_CENTER_DEPTH;
                if (depth != 0) {
                    depth -= HTML_DIV_CENTER_STEP;
                    html->center = (packed & HTML_CENTER_DEPTH) | depth;
                }
            }
        }
    } else if (gHtml->tableDepth != 0) {
        cell = &gHtml->cells[gHtml->curCell];
        cell->center &= HTML_CENTER_DEPTH;
    } else {
        gHtml->center &= HTML_CENTER_DEPTH;
    }
    return 0;
}

/* <SMALL> */
s32 Html_TagSmall(u8 **cur, u8 *text)
{
    u8 size;

    Html_ClearFontAttrs();
    Layout_Flush(text);
    size = gHtml->fontSize[gHtml->fontDepth];
    if (size > 1) {
        size--;
    }
    gHtml->fontAttr.size[0] = size + '0';
    gHtml->fontAttr.size[1] = 0;
    Html_PushFont();
    return 0;
}

/* <BIG> */
s32 Html_TagBig(u8 **cur, u8 *text)
{
    u8 size;

    Html_ClearFontAttrs();
    Layout_Flush(text);
    size = gHtml->fontSize[gHtml->fontDepth];
    if (size < HTML_FONT_SIZE_MAX) {
        size++;
    }
    gHtml->fontAttr.size[0] = size + '0';
    gHtml->fontAttr.size[1] = 0;
    Html_PushFont();
    return 0;
}

/* &LT; */
s32 Html_EntityLt(u8 **cur, u8 *text)
{
    if (gHtml->textLen > HTML_TEXT_FULL) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = '<';
    text[gHtml->textLen] = 0;
    return 0;
}

/* &GT; */
s32 Html_EntityGt(u8 **cur, u8 *text)
{
    if (gHtml->textLen > HTML_TEXT_FULL) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = '>';
    text[gHtml->textLen] = 0;
    return 0;
}

/* &QUOT; &#034; &#34; */
s32 Html_EntityQuot(u8 **cur, u8 *text)
{
    if (gHtml->textLen > HTML_TEXT_FULL) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = '"';
    text[gHtml->textLen] = 0;
    return 0;
}

/* &AMP; */
s32 Html_EntityAmp(u8 **cur, u8 *text)
{
    if (gHtml->textLen > HTML_TEXT_FULL) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = '&';
    text[gHtml->textLen] = 0;
    return 0;
}

/* &NBSP;: HTML_CHAR_NBSP, a space that doesn't break */
s32 Html_EntityNbsp(u8 **cur, u8 *text)
{
    if (gHtml->textLen > HTML_TEXT_FULL) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = HTML_CHAR_NBSP;
    text[gHtml->textLen] = 0;
    return 0;
}

/* &#039; &#39; */
s32 Html_EntityApos(u8 **cur, u8 *text)
{
    if (gHtml->textLen > HTML_TEXT_FULL) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = '\'';
    text[gHtml->textLen] = 0;
    return 0;
}

/* <TABLE ...> */
s32 Html_TagTableAttrs(u8 **cur, u8 *text)
{
    if (Layout_BeforeOpen(cur, text, HTML_CELL_TABLE) < 0) {
        return -1;
    }
    Layout_FlushBlock(text);
    Html_ClearTableAttrs();
    return -(Layout_OpenCell(HTML_CELL_TABLE) < 0);
}

/* <TABLE> */
s32 Html_TagTable(u8 **cur, u8 *text)
{
    if (Layout_BeforeOpen(cur, text, HTML_CELL_TABLE) < 0) {
        return -1;
    }
    Layout_FlushBlock(text);
    Html_ClearTableAttrs();
    if (Layout_OpenCell(HTML_CELL_TABLE) < 0) {
        return -1;
    }
    Html_TagTableEnd(cur, text);
    return 0;
}

/* <TABLE ...>'s ">" */
void Html_TagTableEnd(u8 **cur, u8 *text)
{
    HtmlState *html;
    HtmlCell *cell;

    html = gHtml;
    cell = &html->cells[html->curCell];
    html->paragraph = 0;
    Html_SetCharWidths(cell->bold);
    Layout_TableFromAttrs(cell);
    if (gHtml->measure == HTML_PASS_DRAW) {
        Layout_PushBackground(gHtml->tableAttr.background);
    }
    Layout_TableOrigin(cell);
}

/* <TABLE BORDER=...> */
s32 Html_AttrTableBorder(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    gHtml->tableAttr.border = Html_ParseNumber(buf);
    if (gHtml->tableAttr.border != 0) {
        gHtml->tableAttr.border++;
    }
    return 0;
}

/* <TABLE CELLPADDING=...> */
s32 Html_AttrTableCellpadding(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    gHtml->tableAttr.cellPadding = Html_ParseNumber(buf);
    return 0;
}

/* <TABLE CELLSPACING=...> */
s32 Html_AttrTableCellspacing(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    gHtml->tableAttr.cellSpacing = Html_ParseNumber(buf);
    return 0;
}

/* <TABLE WIDTH=...> */
s32 Html_AttrTableWidth(u8 **cur, u8 *text)
{
    s32 whole = 0;
    char buf[HTML_ATTR_MAX + 1];
    HtmlCell *parent;

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    parent = gHtml->cells[gHtml->curCell].parent;
    if (parent != NULL) {
        whole = parent->w;
    }
    gHtml->tableAttr.width = Html_ParseLength(buf, whole);
    return 0;
}

/* <TABLE HEIGHT=...> */
s32 Html_AttrTableHeight(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    gHtml->tableAttr.height = Html_ParseNumber(buf);
    return 0;
}

/* <TABLE BGCOLOR=...> */
s32 Html_AttrTableBgcolor(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    if ((u8)buf[0] == 0) {
        return 0;
    }
    gHtml->tableAttr.bgColor = Html_ParseColor(buf);
    return 0;
}

/* <TABLE BORDERCOLOR=...> */
s32 Html_AttrTableBordercolor(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    gHtml->tableAttr.borderColor = Html_ParseColor(buf);
    return 0;
}

/* <TABLE BACKGROUND=...> */
s32 Html_AttrTableBackground(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->tableAttr.background, HTML_ATTR_MAX);
    return 0;
}

/* </TABLE> */
s32 Html_TagEndTable(u8 **cur, u8 *text)
{
    HtmlCell *cell;
    HtmlCell *parent;
    s32 n;
    u16 w;

    if (gHtml->tableDepth == 0) {
        return 0;
    }
    if (Layout_BeforeClose(cur, text, 1) < 0) {
        return -1;
    }
    gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
    cell = &gHtml->cells[gHtml->curCell];
    parent = cell->parent;
    if (cell->first != NULL) {
        w = Layout_MeasureColumns(cell->first, -10);
        n = cell->spacing + (w + cell->border * 2);
        if (cell->w < n) {
            cell->w = n;
        }
    }
    cell->open = 0;
    gHtml->textLen = 0;
    *text = 0;
    n = cell->border + (cell->innerH + cell->spacing);
    if (cell->h < n) {
        cell->h = n;
    }
    if (gHtml->tableDepth <= 1) {
        if (gHtml->tableDepth != 0) {
            gHtml->tableDepth--;
        }
        if (gHtml->tableDepth == 0) {
            if (gHtml->measure == HTML_PASS_LAYOUT) {
                gHtml->lines[gHtml->lineNo].w = cell->x + cell->w;
                gHtml->lines[gHtml->lineNo].h = cell->y + cell->h;
            } else if (gHtml->measure == HTML_PASS_MEASURE) {
                n = cell->x + cell->w;
                Html_MaxU16(&gHtml->pageWidth, n);
            }
            gHtml->pen.y += cell->h;
            gHtml->pen.runY = gHtml->pen.y;
            Html_NoteBottom(&gHtml->pen);
            Layout_NextLine();
            if (gHtml->flags != 0) {
                gHtml->pre = 1;
            }
        }
    } else {
        if (parent == NULL) {
            return -1;
        }
        parent->innerH += cell->h;
        parent->penY += cell->h;
        if (gHtml->measure == HTML_PASS_MEASURE) {
            n = cell->w + parent->indent;
            if (parent->minW < n) {
                parent->minW = n;
            }
            parent->cellCount++;
        }
        if (gHtml->tableDepth != 0) {
            gHtml->tableDepth--;
        }
        if (parent->flags & HTML_FLAG_PRE) {
            gHtml->pre = 1;
        }
    }
    Layout_DrawCell(cell);
    if (gHtml->tableDepth >= 2) {
        gHtml->cellPenX = 0;
        gHtml->cellPen.left = parent->x;
        gHtml->cellPen.h = parent->y;
        gHtml->cellPen.x = 0;
        gHtml->cellPen.runX = 0;
        gHtml->cellPen.lineUsed = 0;
        gHtml->cellPen.runY = gHtml->cellPen.y = parent->penY;
        gHtml->cellPen.wrapW = parent->minW;
    }
    return 0;
}

/* <TR ...> */
s32 Html_TagTrAttrs(u8 **cur, u8 *text)
{
    u8 *src;

    if (Layout_BeforeOpen(cur, text, HTML_CELL_TR) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTrAttrs();
    if (Layout_OpenCell(HTML_CELL_TR) < 0) {
        return -1;
    }
    if (gHtml->measure != HTML_PASS_MEASURE) {
        src = gHtml->cells[gHtml->curCell].src;
        if (src != NULL) {
            *cur = src;
            gHtml->mode = 2;
            Html_TagTrEnd(cur, text);
        }
    }
    return 0;
}

/* <TR> */
s32 Html_TagTr(u8 **cur, u8 *text)
{
    if (Layout_BeforeOpen(cur, text, HTML_CELL_TR) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTrAttrs();
    if (Layout_OpenCell(HTML_CELL_TR) < 0) {
        return -1;
    }
    Html_TagTrEnd(cur, text);
    return 0;
}

/* <TR ...>'s ">" */
s32 Html_TagTrEnd(u8 **cur, u8 *text)
{
    HtmlCell *cell;
    HtmlCell *parent;

    cell = &gHtml->cells[gHtml->curCell];
    parent = cell->parent;
    if (parent == NULL) {
        return -1;
    }
    gHtml->paragraph = 0;
    cell->colNo = 0;
    if (gHtml->measure == HTML_PASS_MEASURE) {
        cell->src = *cur;
    }
    Layout_RowFromTable(cell, parent);
    Layout_RowOrigin(cell, parent);
    return 0;
}

/* <TR ALIGN=...> */
s32 Html_AttrTrAlign(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    gHtml->tdAttr.align = Html_LookupKeyword(buf, gHtmlAlignKeywords, sizeof(gHtmlAlignKeywords));
    return 0;
}

/* <TR VALIGN=...> */
s32 Html_AttrTrValign(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    gHtml->tdAttr.valign = Html_LookupKeyword(buf, gHtmlValignKeywords, sizeof(gHtmlValignKeywords));
    return 0;
}

/* <TR BGCOLOR=...> */
s32 Html_AttrTrBgcolor(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if ((u8)buf[0] == 0) {
        return 0;
    }
    gHtml->tdAttr.bgColor = Html_ParseColor(buf);
    return 0;
}

/* </TR> */
s32 Html_TagEndTr(u8 **cur, u8 *text)
{
    HtmlCell *cell;
    HtmlCell *parent;
    s32 n;

    if (gHtml->tableDepth == 0) {
        return 0;
    }
    if (Layout_BeforeClose(cur, text, 2) < 0) {
        return -1;
    }
    gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
    cell = &gHtml->cells[gHtml->curCell];
    parent = cell->parent;
    if (parent == NULL) {
        return -1;
    }
    cell->open = 0;
    gHtml->textLen = 0;
    *text = 0;
    parent->penY += cell->spacing + cell->h;
    parent->innerH = parent->penY;
    n = cell->minW - cell->spacing;
    if (cell->w < n) {
        cell->w = n;
    }
    n = cell->w + cell->border * 2 + cell->spacing * 2;
    if (parent->w < n) {
        parent->w = n;
    }
    if (gHtml->measure == HTML_PASS_MEASURE) {
        n = cell->x + cell->w;
        Html_MaxU16(&gHtml->pageWidth, n);
    }
    if (gHtml->tableDepth != 0) {
        gHtml->tableDepth--;
    }
    return 0;
}

/* <TH ...> */
s32 Html_TagThAttrs(u8 **cur, u8 *text)
{
    if (Layout_BeforeOpen(cur, text, HTML_CELL_TH) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTdAttrs();
    if (Layout_OpenCell(HTML_CELL_TH) < 0) {
        return -1;
    }
    gHtml->cells[gHtml->curCell].bold = 1;
    Html_SetCharWidths(1);
    return 0;
}

/* <TH> */
s32 Html_TagTh(u8 **cur, u8 *text)
{
    if (Layout_BeforeOpen(cur, text, HTML_CELL_TH) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTdAttrs();
    if (Layout_OpenCell(HTML_CELL_TH) < 0) {
        return -1;
    }
    gHtml->cells[gHtml->curCell].bold = 1;
    Html_SetCharWidths(1);
    Html_TagTdEnd(cur, text);
    return 0;
}

/* <TD ...> */
s32 Html_TagTdAttrs(u8 **cur, u8 *text)
{
    if (Layout_BeforeOpen(cur, text, HTML_CELL_TD) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTdAttrs();
    return -(Layout_OpenCell(HTML_CELL_TD) < 0);
}

/* <TD> */
s32 Html_TagTd(u8 **cur, u8 *text)
{
    if (Layout_BeforeOpen(cur, text, HTML_CELL_TD) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTdAttrs();
    if (Layout_OpenCell(HTML_CELL_TD) < 0) {
        return -1;
    }
    Html_TagTdEnd(cur, text);
    return 0;
}

/* <TD ...>'s ">" */
s32 Html_TagTdEnd(u8 **cur, u8 *text)
{
    HtmlCell *cell;
    HtmlCell *parent;
    HtmlCell *spanCell;

    cell = &gHtml->cells[gHtml->curCell];
    parent = cell->parent;
    if (parent == NULL) {
        return -1;
    }
    gHtml->paragraph = 1;
    Layout_CellFromRow(cell, parent);
    if (gHtml->measure == HTML_PASS_DRAW) {
        Layout_PushBackground(gHtml->tdAttr.background);
    }
    if (Layout_CellSpanNext() < 0) {
        return -1;
    }
    cell = &gHtml->cells[gHtml->curCell];
    parent = cell->parent;
    if (parent == NULL) {
        return -1;
    }
    cell->innerH = 0;
    Layout_CellSize(cell, parent);
    Layout_CellOrigin(cell, parent);
    Layout_CellPenStart(cell, parent);
    if (cell->rowSpan != 0) {
        cell->spanH = cell->parent->h - cell->padding * 2;
        spanCell = cell;
        if (cell->spanNext != 0) {
            do {
                spanCell = &gHtml->cells[spanCell->spanNext];
                cell->spanH += cell->spacing + cell->padding * 2;
                cell->spanH += spanCell->parent->h - cell->padding * 2;
            } while (spanCell->spanNext != 0);
        }
        Layout_CellValign(cell);
    }
    Layout_CellAlign(cell);
    return 0;
}

/* <TD ALIGN=...>, <TH ALIGN=...> */
s32 Html_AttrTdAlign(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    gHtml->tdAttr.align = Html_LookupKeyword(buf, gHtmlAlignKeywords, sizeof(gHtmlAlignKeywords));
    return 0;
}

/* <TD VALIGN=...> */
s32 Html_AttrTdValign(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    gHtml->tdAttr.valign = Html_LookupKeyword(buf, gHtmlValignKeywords, sizeof(gHtmlValignKeywords));
    return 0;
}

/* <TD WIDTH=...> */
s32 Html_AttrTdWidth(u8 **cur, u8 *text)
{
    s32 whole = 0;
    char buf[HTML_ATTR_MAX + 1];
    HtmlCell *parent;

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    parent = gHtml->cells[gHtml->curCell].parent;
    if (parent != NULL) {
        whole = parent->w;
    }
    gHtml->tdAttr.width = Html_ParseLength(buf, whole);
    return 0;
}

/* <TD HEIGHT=...> */
s32 Html_AttrTdHeight(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    gHtml->tdAttr.height = Html_ParseNumber(buf);
    return 0;
}

/* <TD ROWSPAN=...> */
s32 Html_AttrTdRowspan(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    gHtml->tdAttr.rowSpan = Html_ParseNumber(buf);
    if (gHtml->tdAttr.rowSpan == 1) {
        gHtml->tdAttr.rowSpan = 0;
    }
    return 0;
}

/* <TD COLSPAN=...> */
s32 Html_AttrTdColspan(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    gHtml->tdAttr.colSpan = Html_ParseNumber(buf);
    if (gHtml->tdAttr.colSpan == 1) {
        gHtml->tdAttr.colSpan = 0;
    }
    return 0;
}

/* <TD BGCOLOR=...> */
s32 Html_AttrTdBgcolor(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    if ((u8)buf[0] == 0) {
        return 0;
    }
    gHtml->tdAttr.bgColor = Html_ParseColor(buf);
    return 0;
}

/* <TD NOWRAP>, <TH NOWRAP> */
s32 Html_AttrTdNowrap(u8 **cur, u8 *text)
{
    if (gHtml->measure != HTML_PASS_MEASURE) {
        return 0;
    }
    gHtml->nowrap = HTML_FLAG_NOWRAP;
    return 0;
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

/* The keyword tables (data), then the strings they point at. */
extern char D_00839E68[];
extern char gHtmlAlignKeywords_4[];
extern char gHtmlAlignKeywords_3[];
extern char gHtmlValignKeywords_2[];
extern char gHtmlValignKeywords_3[];
extern char gHtmlAlignKeywords_0[];
HtmlKeyword gHtmlAlignKeywords[4] = {
#include "lobby/007EFEC0.inc"
};
HtmlKeyword gHtmlValignKeywords[4] = {
#include "lobby/007EFEE0.inc"
};

/* The keywords' strings (read-only data). */
#pragma rodata on
char D_00839E68[8] = {
#include "lobby/00839E68.inc"
};
char gHtmlAlignKeywords_4[8] = {
#include "lobby/00839E70.inc"
};
char gHtmlAlignKeywords_3[8] = {
#include "lobby/00839E78.inc"
};
char gHtmlValignKeywords_2[8] = {
#include "lobby/00839E80.inc"
};
char gHtmlValignKeywords_3[8] = {
#include "lobby/00839E88.inc"
};
#pragma rodata off
