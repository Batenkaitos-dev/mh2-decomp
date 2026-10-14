/* The HTML tag handlers: <META>'s attributes, <PRE>, <DIV>, <SMALL>,
 * <BIG>, the entities, <TABLE>, <TR>, <TH>, <TD> and their attributes. */
#include "common.h"
#include "html.h"
s32 strncmp(const char *, const char *, s32);

s32 strncmp(const char *a, const char *b, s32 n);

void Html_Refresh(s32 seconds, char *url);
void Html_SetCharWidths(u8 bold);
void Html_NoteBottom(HtmlPen *pen);
void Html_ReadAttrValue(u8 **cur, char *buf, s32 size);
void Html_ReadAttrKeyword(u8 **cur, char *buf, s32 size);
void Html_ReadMetaContent(u8 **cur, char *buf, s32 size);
s32 Html_ParseColor(char *s);
s32 Html_ParseNumber(char *s);
s32 Html_ParseLength(char *s, s32 whole);
u8 Html_LookupKeyword(char *s, HtmlKeyword *table, u32 size);
void Layout_TextFull(u8 *text);
void Layout_Flush(u8 *text);
void Layout_FlushBreak(u8 *text);
void Layout_FlushBlock(u8 *text);
void Html_ClearFontAttrs(void);
void Html_PushFont(void);
void Html_ClearTableAttrs(void);
void Html_ClearTrAttrs(void);
void Html_ClearTdAttrs(void);
void Layout_CellValign(HtmlCell *cell);
void Layout_CellAlign(HtmlCell *cell);
s32 Layout_OpenCell(s32 kind);
void Layout_PushBackground(char *background);
void Layout_TableFromAttrs(HtmlCell *cell);
s32 Layout_TableOrigin(HtmlCell *);
void Layout_RowFromTable(HtmlCell *cell, HtmlCell *parent);
void Layout_RowOrigin(HtmlCell *cell, HtmlCell *parent);
void Layout_CellFromRow(HtmlCell *cell, HtmlCell *parent);
void Layout_CellSize(HtmlCell *cell, HtmlCell *parent);
void Layout_CellOrigin(HtmlCell *cell, HtmlCell *parent);
void Layout_CellPenStart(HtmlCell *cell, HtmlCell *parent);
s32 Layout_CellSpanNext(void);
u16 Layout_MeasureColumns(HtmlCell *cell, s32 measure);
void Layout_DrawCell(HtmlCell *cell);
void Layout_NextLine(void);
s32 Layout_BeforeOpen(u8 **cur, u8 *text, u8 kind);
s32 Layout_BeforeClose(u8 **cur, u8 *text, u8 kind);

void Html_TagTableEnd(u8 **cur, u8 *text);
s32 Html_TagTrEnd(u8 **cur, u8 *text);
s32 Html_TagTdEnd(u8 **cur, u8 *text);

extern HtmlKeyword gHtmlAlignKeywords[]; /* ALIGN: LEFT CENTER MIDDLE RIGHT */
extern HtmlKeyword gHtmlValignKeywords[]; /* VALIGN: MIDDLE CENTER TOP BOTTOM */
extern char D_00839F08[]; /* "REFRESH" */

/* Raises *p to v (inlined: the game's callers keep p in a register of its
 * own for the store). */
static inline void Html_MaxU16(u16 *p, s32 v) {
    if (*p < v) {
        *p = v;
    }
}

/* <META HTTP-EQUIV=...> */
s32 Html_AttrMetaHttpEquiv(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrKeyword(cur, buf, 0x100);
    if (strncmp(buf, D_00839F08, 7) == 0) {
        gHtml->meta.refresh = 1;
    }
    return 0;
}

/* <META CONTENT=...> */
s32 Html_AttrMetaContent(u8 **cur, u8 *text) {
    char buf[0x101];

    if (gHtml->meta.refresh != 0) {
        Html_ReadMetaContent(cur, buf, 0x100);
        gHtml->meta.seconds = Html_ParseNumber(buf);
    }
    return 0;
}

/* <META URL=...> */
s32 Html_AttrMetaUrl(u8 **cur, u8 *text) {
    if (gHtml->meta.refresh != 0) {
        Html_ReadAttrValue(cur, gHtml->meta.url, 0x100);
    }
    return 0;
}

/* <META ...>'s ">": a refresh. */
s32 Html_TagMetaEnd(u8 **cur, u8 *text) {
    if (gHtml->meta.refresh != 0 && gHtml->inFrame == 0) {
        if (gHtml->measure == 0) {
            Html_Refresh(gHtml->meta.seconds, gHtml->meta.url);
        } else if (gHtml->meta.seconds == 0) {
            Html_Refresh(gHtml->meta.seconds, gHtml->meta.url);
        }
    }
    return 0;
}

/* <PRE> */
s32 Html_TagPre(u8 **cur, u8 *text) {
    Layout_FlushBreak(text);
    if (gHtml->tableDepth != 0) {
        gHtml->cells[gHtml->curCell].flags |= 1;
    } else {
        gHtml->flags |= 1;
    }
    gHtml->pre = 1;
    Layout_FlushBreak(text);
    return 0;
}

/* </PRE> */
s32 Html_TagEndPre(u8 **cur, u8 *text) {
    Layout_FlushBreak(text);
    if (gHtml->tableDepth != 0) {
        gHtml->cells[gHtml->curCell].flags &= ~1;
    } else {
        gHtml->flags &= ~1;
    }
    gHtml->pre = 0;
    return 0;
}

/* <DIV ...> */
s32 Html_TagDiv(u8 **cur, u8 *text) {
    gHtml->divAlign[gHtml->divDepth] = 0;
    gHtml->divDepth++;
    Layout_Flush(text);
    return 0;
}

/* <DIV ALIGN=...> */
s32 Html_AttrDivAlign(u8 **cur, u8 *text) {
    char buf[0x101];
    HtmlState *s;
    HtmlCell *cell;
    u8 v;
    u8 n;

    Html_ReadAttrValue(cur, buf, 0x100);
    s = gHtml;
    if (s->divDepth <= 0x10) {
        s->divAlign[s->divDepth - 1] = Html_LookupKeyword(buf, gHtmlAlignKeywords, 0x20);
        if (gHtml->divAlign[gHtml->divDepth - 1] == 2) {
            if (gHtml->tableDepth != 0) {
                cell = &gHtml->cells[gHtml->curCell];
                v = cell->center;
                n = v & 0xF0;
                if (n != 0xF0) {
                    n += 0x10;
                    cell->center = (v & 0xF) | n;
                }
                Layout_CellAlign(cell);
            } else {
                HtmlState *s2 = gHtml;
                v = s2->center;
                n = v & 0xF0;
                if (n != 0xF0) {
                    n += 0x10;
                    s2->center = (v & 0xF) | n;
                }
            }
        }
    }
    return 0;
}

/* </DIV> */
s32 Html_TagEndDiv(u8 **cur, u8 *text) {
    HtmlCell *cell;
    u8 v;
    u8 n;
    u8 depth;

    if (gHtml->divDepth > 0) {
        gHtml->divDepth--;
        depth = gHtml->divDepth;
        if (depth < 0x10 && gHtml->divAlign[depth] == 2) {
            if (gHtml->tableDepth != 0) {
                cell = &gHtml->cells[gHtml->curCell];
                v = cell->center;
                n = v & 0xF0;
                if (n != 0) {
                    n -= 0x10;
                    cell->center = (v & 0xF) | n;
                }
            } else {
                HtmlState *s = gHtml;
                v = s->center;
                n = v & 0xF0;
                if (n != 0) {
                    n -= 0x10;
                    s->center = (v & 0xF) | n;
                }
            }
        }
    } else if (gHtml->tableDepth != 0) {
        cell = &gHtml->cells[gHtml->curCell];
        cell->center &= 0xF;
    } else {
        gHtml->center &= 0xF;
    }
    return 0;
}

/* <SMALL> */
s32 Html_TagSmall(u8 **cur, u8 *text) {
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
s32 Html_TagBig(u8 **cur, u8 *text) {
    u8 size;

    Html_ClearFontAttrs();
    Layout_Flush(text);
    size = gHtml->fontSize[gHtml->fontDepth];
    if (size < 7) {
        size++;
    }
    gHtml->fontAttr.size[0] = size + '0';
    gHtml->fontAttr.size[1] = 0;
    Html_PushFont();
    return 0;
}

/* &LT; */
s32 Html_EntityLt(u8 **cur, u8 *text) {
    if (gHtml->textLen > 0x7D) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = '<';
    text[gHtml->textLen] = 0;
    return 0;
}

/* &GT; */
s32 Html_EntityGt(u8 **cur, u8 *text) {
    if (gHtml->textLen > 0x7D) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = '>';
    text[gHtml->textLen] = 0;
    return 0;
}

/* &QUOT; &#034; &#34; */
s32 Html_EntityQuot(u8 **cur, u8 *text) {
    if (gHtml->textLen > 0x7D) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = '"';
    text[gHtml->textLen] = 0;
    return 0;
}

/* &AMP; */
s32 Html_EntityAmp(u8 **cur, u8 *text) {
    if (gHtml->textLen > 0x7D) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = '&';
    text[gHtml->textLen] = 0;
    return 0;
}

/* &NBSP;: 7, a space that doesn't break */
s32 Html_EntityNbsp(u8 **cur, u8 *text) {
    if (gHtml->textLen > 0x7D) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = 7;
    text[gHtml->textLen] = 0;
    return 0;
}

/* &#039; &#39; */
s32 Html_EntityApos(u8 **cur, u8 *text) {
    if (gHtml->textLen > 0x7D) {
        Layout_TextFull(gHtml->text);
    }
    text[gHtml->textLen++] = '\'';
    text[gHtml->textLen] = 0;
    return 0;
}

/* <TABLE ...> */
s32 Html_TagTableAttrs(u8 **cur, u8 *text) {
    if (Layout_BeforeOpen(cur, text, 1) < 0) {
        return -1;
    }
    Layout_FlushBlock(text);
    Html_ClearTableAttrs();
    return -(Layout_OpenCell(1) < 0);
}

/* <TABLE> */
s32 Html_TagTable(u8 **cur, u8 *text) {
    if (Layout_BeforeOpen(cur, text, 1) < 0) {
        return -1;
    }
    Layout_FlushBlock(text);
    Html_ClearTableAttrs();
    if (Layout_OpenCell(1) < 0) {
        return -1;
    }
    Html_TagTableEnd(cur, text);
    return 0;
}

/* <TABLE ...>'s ">" */
void Html_TagTableEnd(u8 **cur, u8 *text) {
    HtmlState *s;
    HtmlCell *cell;

    s = gHtml;
    cell = &s->cells[s->curCell];
    s->paragraph = 0;
    Html_SetCharWidths(cell->bold);
    Layout_TableFromAttrs(cell);
    if (gHtml->measure == 0) {
        Layout_PushBackground(gHtml->tableAttr.background);
    }
    Layout_TableOrigin(cell);
}

/* <TABLE BORDER=...> */
s32 Html_AttrTableBorder(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    gHtml->tableAttr.border = Html_ParseNumber(buf);
    if (gHtml->tableAttr.border != 0) {
        gHtml->tableAttr.border++;
    }
    return 0;
}

/* <TABLE CELLPADDING=...> */
s32 Html_AttrTableCellpadding(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    gHtml->tableAttr.cellPadding = Html_ParseNumber(buf);
    return 0;
}

/* <TABLE CELLSPACING=...> */
s32 Html_AttrTableCellspacing(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    gHtml->tableAttr.cellSpacing = Html_ParseNumber(buf);
    return 0;
}

/* <TABLE WIDTH=...> */
s32 Html_AttrTableWidth(u8 **cur, u8 *text) {
    s32 whole = 0;
    char buf[0x101];
    HtmlCell *parent;

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
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
s32 Html_AttrTableHeight(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    gHtml->tableAttr.height = Html_ParseNumber(buf);
    return 0;
}

/* <TABLE BGCOLOR=...> */
s32 Html_AttrTableBgcolor(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    if ((u8)buf[0] == 0) {
        return 0;
    }
    gHtml->tableAttr.bgColor = Html_ParseColor(buf);
    return 0;
}

/* <TABLE BORDERCOLOR=...> */
s32 Html_AttrTableBordercolor(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    gHtml->tableAttr.borderColor = Html_ParseColor(buf);
    return 0;
}

/* <TABLE BACKGROUND=...> */
s32 Html_AttrTableBackground(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->tableAttr.background, 0x100);
    return 0;
}

/* </TABLE> */
s32 Html_TagEndTable(u8 **cur, u8 *text) {
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
            if (gHtml->measure == 1) {
                gHtml->lines[gHtml->lineNo].w = cell->x + cell->w;
                gHtml->lines[gHtml->lineNo].h = cell->y + cell->h;
            } else if (gHtml->measure == -10) {
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
        if (gHtml->measure == -10) {
            n = cell->w + parent->indent;
            if (parent->minW < n) {
                parent->minW = n;
            }
            parent->cellCount++;
        }
        if (gHtml->tableDepth != 0) {
            gHtml->tableDepth--;
        }
        if (parent->flags & 1) {
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
s32 Html_TagTrAttrs(u8 **cur, u8 *text) {
    u8 *src;

    if (Layout_BeforeOpen(cur, text, 2) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTrAttrs();
    if (Layout_OpenCell(2) < 0) {
        return -1;
    }
    if (gHtml->measure != -10) {
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
s32 Html_TagTr(u8 **cur, u8 *text) {
    if (Layout_BeforeOpen(cur, text, 2) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTrAttrs();
    if (Layout_OpenCell(2) < 0) {
        return -1;
    }
    Html_TagTrEnd(cur, text);
    return 0;
}

/* <TR ...>'s ">" */
s32 Html_TagTrEnd(u8 **cur, u8 *text) {
    HtmlCell *cell;
    HtmlCell *parent;

    cell = &gHtml->cells[gHtml->curCell];
    parent = cell->parent;
    if (parent == NULL) {
        return -1;
    }
    gHtml->paragraph = 0;
    cell->colNo = 0;
    if (gHtml->measure == -10) {
        cell->src = *cur;
    }
    Layout_RowFromTable(cell, parent);
    Layout_RowOrigin(cell, parent);
    return 0;
}

/* <TR ALIGN=...> */
s32 Html_AttrTrAlign(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    gHtml->tdAttr.align = Html_LookupKeyword(buf, gHtmlAlignKeywords, 0x20);
    return 0;
}

/* <TR VALIGN=...> */
s32 Html_AttrTrValign(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    gHtml->tdAttr.valign = Html_LookupKeyword(buf, gHtmlValignKeywords, 0x20);
    return 0;
}

/* <TR BGCOLOR=...> */
s32 Html_AttrTrBgcolor(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if ((u8)buf[0] == 0) {
        return 0;
    }
    gHtml->tdAttr.bgColor = Html_ParseColor(buf);
    return 0;
}

/* </TR> */
s32 Html_TagEndTr(u8 **cur, u8 *text) {
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
    if (gHtml->measure == -10) {
        n = cell->x + cell->w;
        Html_MaxU16(&gHtml->pageWidth, n);
    }
    if (gHtml->tableDepth != 0) {
        gHtml->tableDepth--;
    }
    return 0;
}

/* <TH ...> */
s32 Html_TagThAttrs(u8 **cur, u8 *text) {
    if (Layout_BeforeOpen(cur, text, 3) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTdAttrs();
    if (Layout_OpenCell(3) < 0) {
        return -1;
    }
    gHtml->cells[gHtml->curCell].bold = 1;
    Html_SetCharWidths(1);
    return 0;
}

/* <TH> */
s32 Html_TagTh(u8 **cur, u8 *text) {
    if (Layout_BeforeOpen(cur, text, 3) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTdAttrs();
    if (Layout_OpenCell(3) < 0) {
        return -1;
    }
    gHtml->cells[gHtml->curCell].bold = 1;
    Html_SetCharWidths(1);
    Html_TagTdEnd(cur, text);
    return 0;
}

/* <TD ...> */
s32 Html_TagTdAttrs(u8 **cur, u8 *text) {
    if (Layout_BeforeOpen(cur, text, 4) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTdAttrs();
    return -(Layout_OpenCell(4) < 0);
}

/* <TD> */
s32 Html_TagTd(u8 **cur, u8 *text) {
    if (Layout_BeforeOpen(cur, text, 4) < 0) {
        return -1;
    }
    gHtml->textLen = 0;
    *text = 0;
    Html_ClearTdAttrs();
    if (Layout_OpenCell(4) < 0) {
        return -1;
    }
    Html_TagTdEnd(cur, text);
    return 0;
}

/* <TD ...>'s ">" */
s32 Html_TagTdEnd(u8 **cur, u8 *text) {
    HtmlCell *cell;
    HtmlCell *parent;
    HtmlCell *c;

    cell = &gHtml->cells[gHtml->curCell];
    parent = cell->parent;
    if (parent == NULL) {
        return -1;
    }
    gHtml->paragraph = 1;
    Layout_CellFromRow(cell, parent);
    if (gHtml->measure == 0) {
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
        c = cell;
        if (cell->spanNext != 0) {
            do {
                c = &gHtml->cells[c->spanNext];
                cell->spanH += cell->spacing + cell->padding * 2;
                cell->spanH += c->parent->h - cell->padding * 2;
            } while (c->spanNext != 0);
        }
        Layout_CellValign(cell);
    }
    Layout_CellAlign(cell);
    return 0;
}

/* <TD ALIGN=...>, <TH ALIGN=...> */
s32 Html_AttrTdAlign(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    gHtml->tdAttr.align = Html_LookupKeyword(buf, gHtmlAlignKeywords, 0x20);
    return 0;
}

/* <TD VALIGN=...> */
s32 Html_AttrTdValign(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    gHtml->tdAttr.valign = Html_LookupKeyword(buf, gHtmlValignKeywords, 0x20);
    return 0;
}

/* <TD WIDTH=...> */
s32 Html_AttrTdWidth(u8 **cur, u8 *text) {
    s32 whole = 0;
    char buf[0x101];
    HtmlCell *parent;

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
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
s32 Html_AttrTdHeight(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    gHtml->tdAttr.height = Html_ParseNumber(buf);
    return 0;
}

/* <TD ROWSPAN=...> */
s32 Html_AttrTdRowspan(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    gHtml->tdAttr.rowSpan = Html_ParseNumber(buf);
    if (gHtml->tdAttr.rowSpan == 1) {
        gHtml->tdAttr.rowSpan = 0;
    }
    return 0;
}

/* <TD COLSPAN=...> */
s32 Html_AttrTdColspan(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    gHtml->tdAttr.colSpan = Html_ParseNumber(buf);
    if (gHtml->tdAttr.colSpan == 1) {
        gHtml->tdAttr.colSpan = 0;
    }
    return 0;
}

/* <TD BGCOLOR=...> */
s32 Html_AttrTdBgcolor(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    if (gHtml->measure != -10) {
        return 0;
    }
    if ((u8)buf[0] == 0) {
        return 0;
    }
    gHtml->tdAttr.bgColor = Html_ParseColor(buf);
    return 0;
}

/* unk_746AD0's .data (0x7EFEC0-0x7EFF00) */
extern char D_00839E68[];
extern char D_00839E70[];
extern char D_00839E78[];
extern char D_00839E80[];
extern char D_00839E88[];
extern char gHtmlAlignKeywords_0[];
extern char D_00839E70[];
HtmlKeyword gHtmlAlignKeywords[4] = {
#include "lobby/007EFEC0.inc"
};
HtmlKeyword gHtmlValignKeywords[4] = {
#include "lobby/007EFEE0.inc"
};

/* unk_746AD0's .rodata (0x839E68-0x839E90) */
#pragma rodata on
char D_00839E68[0x8] = {
#include "lobby/00839E68.inc"
};
char D_00839E70[0x8] = {
#include "lobby/00839E70.inc"
};
char D_00839E78[0x8] = {
#include "lobby/00839E78.inc"
};
char D_00839E80[0x8] = {
#include "lobby/00839E80.inc"
};
char D_00839E88[0x8] = {
#include "lobby/00839E88.inc"
};
#pragma rodata off
