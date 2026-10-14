/* The HTML table layout: cells, rows and tables measured and placed, text
 * in cells; the image heap (zcalloc). */
#include "common.h"
#include "html.h"
#include "arena.h"
#include "browser.h"

/* Compiled at optimization level 4, as the lobby's HTML parser
 * (unk_741900.c): every function of the file matches with it. */
#pragma optimization_level 4

void Draw_Cell(u16 x, u16 y, u16 x2, u16 y2, u8 size, s32 color, u8 border, s32 bcolor, u8 *background);
void Layout_CellValign(HtmlCell *cell);
void Layout_CellAlign(HtmlCell *cell);
char *Layout_PopBackground(void);
void Layout_CellPlaceRun(HtmlLine *line);
void Draw_Rule(u16 x, u16 y, u16 x2, u16 y2);
void Layout_NextLine(void);
void Html_NoteBottom(HtmlPen *pen);
void Layout_DrawRun(u16 x, u16 y, HtmlLine *line);
void Layout_NextLineEntry(HtmlLine **lp);
void Layout_PenNewLine(HtmlPen *pen, HtmlLine **lp);
s32 Layout_CellText(u8 *text, HtmlLine **lp);
void Layout_CellFlushRun(HtmlLine *line);
void Layout_CellEndLine(HtmlLine **lp);
u16 Layout_CellWrap(HtmlCell *cell, HtmlLine **lp, u16 x);
void Layout_DrawImage(u16 x, u16 y, HtmlLine *line);
void Draw_SelectEnd(u16 x, u16 y, u16 x2, u16 y2);
void Layout_Input(u16 x, u16 y, u16 *runX, u16 *px, u16 w);
s32 Layout_OpenCell(s32 kind);
void Layout_CellFromRow(HtmlCell *cell, HtmlCell *row);
void Html_ClearTdAttrs(void);
s32 Html_TagEndTable(u8 **cur, u8 *text);
s32 Html_TagEndTr(u8 **cur, u8 *text);
s32 Html_TagEndTd(u8 **cur, u8 *text);
void Draw_Table(u16 x, u16 y, u16 x2, u16 y2, u8 size, u32 color, u8 border, u8 *background);
void Html_ClearTableAttrs(void);
void Html_ClearTrAttrs(void);
void Html_TagTableEnd(u8 **cur, u8 *text);
s32 Html_TagTrEnd(u8 **cur, u8 *text);
s32 Html_TagTdEnd(u8 **cur, u8 *text);
void *memset(void *dst, s32 c, u32 n);
u8 *Layout_CellSkipSpaces(u8 *p);
void Layout_CellBreakLine(HtmlLine **lp);
void Layout_AddWideChar(u8 **p, HtmlPen *pen);
void Layout_AddNbsp(u8 **p, HtmlPen *pen);
void Layout_AddChar(u8 **p, HtmlPen *pen);

extern u8 gCharClass[]; /* character classes */

/* See unk_746AD0.c. */
static inline void Html_MaxU16(u16 *p, s32 v) {
    if (*p < v) {
        *p = v;
    }
}

/* The cell pen starts at a cell's origin. */
void Layout_CellPenStart(HtmlCell *cell, HtmlCell *parent) {
    gHtml->cellPen.left = cell->x;
    gHtml->cellPen.h = cell->y;
    gHtml->cellPen.x = 0;
    gHtml->cellPen.runX = 0;
    gHtml->cellPen.y = 0;
    gHtml->cellPen.runY = 0;
    gHtml->cellPen.wrapW = cell->w;
    gHtml->cellPen.lineUsed = 0;
    gHtml->cellPenX = 0;
    cell->alignX = 0;
    cell->valignY = 0;
    if (gHtml->measure == 0) {
        Layout_CellValign(cell);
        Layout_CellAlign(cell);
    }
}

/* The last of a chain of continued cells (unk36); its index. */
u8 Layout_CellChainEnd(HtmlCell *cell) {
    u16 i;

    while ((i = cell->spanPrev) != 0) {
        cell = &gHtml->cells[i];
    }
    return cell->index;
}

/* A cell of a row that a cell above spans into (ROWSPAN): it takes that
 * cell's place, width and span, and is closed at once. */
s32 Layout_RowSpanCell(HtmlCell *cell, HtmlCell *row, HtmlCell *prev) {
    u8 end;
    HtmlCell *c;

    if (gHtml->measure == -10) {
        cell->rowSpan = prev->rowSpan - 1;
        cell->colSpan = prev->colSpan;
        cell->spanFirst = prev->spanFirst;
        cell->spanLast = prev->spanLast;
        cell->spanPrev = prev->index;
        prev->spanNext = cell->index;
    }
    cell->spanH = prev->spanH;
    end = Layout_CellChainEnd(cell);
    cell->w = prev->w;
    if (gHtml->measure == -10) {
        cell->h = 0;
        cell->innerH = 0;
        cell->cellCount = 0;
        cell->minW = prev->minW;
        cell->sizeFlags = prev->sizeFlags;
    } else if (gHtml->measure == 1) {
        if (cell->rowSpan == 1) {
            s32 b = cell->h + (cell->padding + (row->y + row->penY));
            s32 e = gHtml->cells[end].y + cell->spanH;
            if (b < e) {
                cell->h += (u16)(e - b);
            }
        }
        if (row->h < cell->h + cell->padding * 2) {
            row->h = cell->h + cell->padding * 2;
        } else {
            cell->h = row->h - cell->padding * 2;
        }
    } else if (gHtml->measure == 0) {
        if (row->h < cell->h + cell->padding * 2) {
            row->h = cell->h + cell->padding * 2;
        } else {
            cell->h = row->h - cell->padding * 2;
        }
    }
    cell->x = prev->x;
    cell->y = cell->padding + (row->y + row->penY);
    cell->penX = 0;
    cell->penY = 0;
    if (cell->colSpan == 0) {
        row->penX += (u16)(cell->spacing + (cell->w + cell->padding * 2));
        if (gHtml->measure == -10) {
            row->minW += (u16)(cell->spacing + (cell->w + cell->padding * 2));
            row->cellCount++;
        }
    } else if (cell->spanLast != 0) {
        gHtml->spanCell = cell->spanFirst;
        c = &gHtml->cells[gHtml->spanCell];
        row->penX += (u16)(c->spacing + (c->w + c->padding * 2));
        if (gHtml->measure == -10) {
            row->minW += (u16)(c->spacing + (c->w + c->padding * 2));
            row->cellCount++;
        }
    }
    cell->open = 0;
    if (gHtml->tableDepth != 0) {
        gHtml->tableDepth--;
    } else {
        return -1;
    }
    return 0;
}

/* The cell a cell continues: measuring, the cell of the row above in the
 * same column that spans into its row; otherwise its unk36. */
HtmlCell *Layout_CellSpanAbove(HtmlCell *cell) {
    HtmlCell *c;
    u8 col;
    u16 i;

    if (gHtml->measure == -10) {
        c = cell->parent;
        if (c == NULL) {
            return NULL;
        }
        if (c->rowNo == 0) {
            return NULL;
        }
        c = c->above;
        if (c == NULL) {
            return NULL;
        }
        c = c->first;
        if (c == NULL) {
            return NULL;
        }
        col = cell->colNo;
        do {
            if (c->colNo == col) {
                return (c->rowSpan > 1) ? c : NULL;
            }
            c = c->next;
        } while (c != NULL);
    } else if ((i = cell->spanPrev) != 0) {
        return &gHtml->cells[i];
    }
    return NULL;
}

/* The cell of the row above in the same column that spans down into this
 * row (ROWSPAN >= 2), or NULL. */
HtmlCell *Layout_CellAbove(HtmlCell *cell) {
    HtmlCell *c;
    u8 col;

    if (cell->above == NULL) {
        return NULL;
    }
    c = cell->above->first;
    if (c == NULL) {
        return NULL;
    }
    col = cell->colNo;
    do {
        if (c->colNo == col) {
            return (c->rowSpan > 1) ? c : NULL;
        }
        c = c->next;
    } while (c != NULL);
    return NULL;
}

/* The cells that cells above span into, from prev on (a row's start). */
s32 Layout_RowSpanCells(HtmlCell *prev, HtmlCell *row) {
    HtmlCell *cell;
    HtmlCell *parent;

    do {
        Html_ClearTdAttrs();
        if (Layout_OpenCell(4) < 0) {
            return -1;
        }
        cell = &gHtml->cells[gHtml->curCell];
        parent = cell->parent;
        if (parent == NULL) {
            return -1;
        }
        gHtml->paragraph = 1;
        Layout_CellFromRow(cell, parent);
        if (Layout_RowSpanCell(cell, parent, prev) < 0) {
            return -1;
        }
    } while ((prev = Layout_CellAbove(row)) != NULL);
    return 0;
}

/* After a cell: the cells that cells above span into, next to it. */
s32 Layout_CellSpanNext(void) {
    HtmlCell *cell;
    HtmlCell *parent;
    HtmlCell *prev;

    cell = &gHtml->cells[gHtml->curCell];
    parent = cell->parent;
    if (parent == NULL) {
        return -1;
    }
    prev = Layout_CellSpanAbove(cell);
    if (prev == NULL) {
        return 0;
    }
    do {
        if (Layout_RowSpanCell(cell, parent, prev) < 0) {
            return -1;
        }
        if (Layout_OpenCell(4) < 0) {
            return -1;
        }
        cell = &gHtml->cells[gHtml->curCell];
        parent = cell->parent;
        if (parent == NULL) {
            return -1;
        }
        Layout_CellFromRow(cell, parent);
    } while ((prev = Layout_CellSpanAbove(cell)) != NULL);
    return 0;
}

/* After a cell with COLSPAN: the cells it covers (each continues it). */
s32 Layout_CellColSpan(HtmlCell *cell) {
    HtmlCell *first;
    HtmlCell *row;
    s32 i;

    if (cell->colSpan != 0) {
        cell->spanFirst = gHtml->spanCell = gHtml->curCell;
        first = &gHtml->cells[gHtml->spanCell];
        for (i = 0; i < first->colSpan - 1; i++) {
            if (Layout_OpenCell(4) < 0) {
                return -1;
            }
            cell = &gHtml->cells[gHtml->curCell];
            row = cell->parent;
            if (row == NULL) {
                return -1;
            }
            if (gHtml->measure == -10) {
                cell->colNo = row->colNo;
                cell->rowNo = row->rowNo;
                cell->spanFirst = gHtml->spanCell;
                cell->colSpan = first->colSpan;
                cell->rowSpan = first->rowSpan;
                cell->border = row->border;
                cell->spacing = row->spacing;
                cell->padding = row->padding;
                cell->cellCount = 0;
                cell->minW = 0;
                cell->w = 0;
                cell->h = first->h;
                cell->innerH = first->innerH;
                cell->sizeFlags = first->sizeFlags;
            }
            row->colNo++;
            cell->x = cell->padding + (row->x + row->penX);
            cell->y = cell->padding + (row->y + row->penY);
            cell->penX = 0;
            cell->penY = 0;
            gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
            cell = &gHtml->cells[gHtml->curCell];
            row = cell->parent;
            if (row == NULL) {
                return -1;
            }
            if (gHtml->tableDepth != 0) {
                gHtml->tableDepth--;
            } else {
                return -1;
            }
        }
        if (gHtml->measure == -10) {
            cell->spanLast = 1;
        }
        row->penX += (u16)(first->spacing + (first->w + first->padding * 2));
        if (gHtml->measure == -10) {
            row->minW += (u16)(first->spacing + (first->w + first->padding * 2));
            row->cellCount++;
        }
    }
    return 0;
}

/* Before <TABLE>, <TR>, <TH> or <TD> (kind 1..4): closes what the new one
 * cannot be inside, and opens a missing <TABLE> or <TR>. */
s32 Layout_BeforeOpen(u8 **cur, u8 *text, u8 kind) {
    HtmlCell *cell;
    u16 i;

    gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
    i = gHtml->curCell;
    cell = &gHtml->cells[i];
    switch (kind) {
    case 1:
        if (i < 2) {
            return 0;
        }
        if (cell->kind == 1) {
            if (cell->open) {
                Html_TagEndTable(cur, text);
            }
        } else if (cell->kind == 2) {
            if (cell->open) {
                Html_TagEndTr(cur, text);
            }
            Html_TagEndTable(cur, text);
        }
        break;
    case 2:
        if (i == 0) {
            Html_ClearTableAttrs();
            if (Layout_OpenCell(1) < 0) {
                return -1;
            }
            Html_TagTableEnd(cur, text);
        } else if (i < 2) {
            return 0;
        }
        if (cell->kind == 2) {
            if (cell->open) {
                Html_TagEndTr(cur, text);
            }
        } else if (cell->kind == 3 || cell->kind == 4) {
            if (cell->open) {
                Html_TagEndTd(cur, text);
            }
            Html_TagEndTr(cur, text);
        }
        break;
    case 3:
    case 4:
        if (i == 0) {
            Html_ClearTableAttrs();
            if (Layout_OpenCell(1) < 0) {
                return -1;
            }
            Html_TagTableEnd(cur, text);
        }
        if (cell->kind == 1) {
            Html_ClearTrAttrs();
            if (Layout_OpenCell(2) < 0) {
                return -1;
            }
            Html_TagTrEnd(cur, text);
        } else if (cell->kind == 3 || cell->kind == 4) {
            if (cell->open) {
                Html_TagEndTd(cur, text);
            }
        }
        break;
    }
    return 0;
}

/* Before </TABLE>, </TR>, </TH> or </TD> (kind 1..4): closes what is open
 * inside it, and opens a missing <TR> or <TD> (an empty table or row). */
s32 Layout_BeforeClose(u8 **cur, u8 *text, u8 kind) {
    HtmlCell *cell;
    HtmlCell *prev;

    gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
    cell = &gHtml->cells[gHtml->curCell];
    switch (kind) {
    case 1:
        if (cell->kind == 1 && cell->first == NULL) {
            Html_ClearTrAttrs();
            if (Layout_OpenCell(2) < 0) {
                return -1;
            }
            Html_TagTrEnd(cur, text);
            Html_ClearTdAttrs();
            if (Layout_OpenCell(4) < 0) {
                return -1;
            }
            Html_TagTdEnd(cur, text);
        }
        if (cell->kind == 2 && cell->first == NULL) {
            Html_ClearTdAttrs();
            if (Layout_OpenCell(4) < 0) {
                return -1;
            }
            Html_TagTdEnd(cur, text);
            gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
            cell = &gHtml->cells[gHtml->curCell];
        }
        if (cell->kind == 3 || cell->kind == 4) {
            Html_TagEndTd(cur, text);
            gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
            cell = &gHtml->cells[gHtml->curCell];
        }
        if (cell->kind == 2) {
            Html_TagEndTr(cur, text);
        }
        break;
    case 2:
        if (cell->kind == 1) {
            if (cell->first == NULL) {
                Html_ClearTrAttrs();
                if (Layout_OpenCell(2) < 0) {
                    return -1;
                }
                Html_TagTrEnd(cur, text);
                gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
                cell = &gHtml->cells[gHtml->curCell];
            } else if (cell->open) {
                Html_TagEndTable(cur, text);
            }
        }
        if (cell->kind == 2 && (prev = Layout_CellAbove(cell)) != NULL) {
            if (Layout_RowSpanCells(prev, cell) < 0) {
                return -1;
            }
            gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
            cell = &gHtml->cells[gHtml->curCell];
        }
        if (cell->kind == 3 || cell->kind == 4) {
            Html_TagEndTd(cur, text);
        }
        break;
    case 3:
    case 4:
        if (cell->kind == 1) {
            if (cell->first == NULL) {
                Html_ClearTrAttrs();
                if (Layout_OpenCell(2) < 0) {
                    return -1;
                }
                Html_TagTrEnd(cur, text);
                Html_ClearTdAttrs();
                if (Layout_OpenCell(4) < 0) {
                    return -1;
                }
                Html_TagTdEnd(cur, text);
            } else if (cell->open) {
                Html_TagEndTable(cur, text);
            }
        } else if (cell->kind == 2) {
            Html_ClearTdAttrs();
            if (Layout_OpenCell(4) < 0) {
                return -1;
            }
            Html_TagTdEnd(cur, text);
        }
        break;
    }
    return 0;
}

/* Closes every open table, row and cell. */
s32 Html_CloseTables(u8 **cur, u8 *text) {
    u8 d;

    while ((d = gHtml->tableDepth) != 0) {
        gHtml->curCell = gHtml->cellStack[d];
        switch (gHtml->cells[gHtml->curCell].kind) {
        case 1:
            if (Html_TagEndTable(cur, text) < 0) {
                return -1;
            }
            break;
        case 2:
            if (Html_TagEndTr(cur, text) < 0) {
                return -1;
            }
            break;
        case 3:
        case 4:
            if (Html_TagEndTd(cur, text) < 0) {
                return -1;
            }
            break;
        default:
            return -1;
        }
    }
    return 0;
}

/* Spreads the table's width over its columns: each column gets its least
 * width (minCols), then the room left is shared out in proportion to what
 * each can still take up to its widest (maxCols); minCols gets the result. */
void Layout_SpreadColumns(HtmlCell *table, HtmlCol *minCols, HtmlCol *maxCols, s32 n) {
    u16 minW[32];
    u16 maxW[32];
    u16 total;
    u16 room;
    s32 w;
    u16 least;
    u16 prevMin;
    u16 prevMax;
    u8 cnt;
    u16 used;
    u16 given;
    u16 d;
    s32 i;

    total = 0;
    if (!(table->sizeFlags & 1)) {
        table->w = maxCols[n - 1].x + (table->border + table->spacing) * 2;
    }
    room = table->w - (table->border + table->spacing) * 2;
    if (minCols[n - 1].x == 0) {
        return;
    }
    least = minCols[n - 1].x - table->spacing;
    if (room <= least) {
        return;
    }
    d = room - least;
    room = d;
    prevMin = prevMax = 0;
    for (i = 0; i < n; i++) {
        if (prevMin < minCols[i].x) {
            minW[i] = minCols[i].x - prevMin;
        } else {
            minW[i] = 0;
        }
        prevMin = minCols[i].x;
        if (prevMax < maxCols[i].x) {
            maxW[i] = maxCols[i].x - prevMax;
        } else {
            maxW[i] = 0;
        }
        prevMax = maxCols[i].x;
    }
    for (i = 0; i < n; i++) {
        if (minW[i] <= maxW[i]) {
            total += maxW[i];
        }
    }
    if (total == 0) {
        total = n;
    }
    cnt = 0;
    used = given = 0;
    for (i = 0; i < n; i++) {
        if (minW[i] <= maxW[i]) {
            d = room - (total - maxW[i]) * room / total;
            w = d + minW[i];
            if (maxW[i] <= w) {
                used += (u16)(w - maxW[i]);
                minW[i] = maxW[i];
            } else if (d != 0) {
                minW[i] = w;
                cnt++;
                given += maxW[i];
            }
        }
    }
    while (used != 0 && cnt != 0) {
        total = given;
        room = used;
        if (total == 0) {
            total = cnt;
        }
        cnt = 0;
        used = 0;
        given = 0;
        for (i = 0; i < n; i++) {
            if (minW[i] < maxW[i]) {
                d = room - (total - maxW[i]) * room / total;
                w = d + minW[i];
                if (maxW[i] <= w) {
                    used += (u16)(w - maxW[i]);
                    minW[i] = maxW[i];
                } else if (d != 0) {
                    minW[i] = w;
                    cnt++;
                    given += maxW[i];
                }
            }
        }
    }
    if (used != 0) {
        total = 0;
        for (i = 0; i < n; i++) {
            if (!(maxCols[i].flags & 4)) {
                total += maxW[i];
            }
        }
        if (total == 0) {
            total = n;
        }
        for (i = 0; i < n; i++) {
            if (!(maxCols[i].flags & 4)) {
                minW[i] += (u16)(used - (total - maxW[i]) * used / total);
            }
        }
    }
    for (i = 0, total = 0; i < n; i++) {
        total += minW[i];
        minCols[i].x = total;
    }
}

/* Merges a row's column edges into the table's (both n long): a fixed
 * (WIDTH) edge wins, and the columns after a moved edge move with it. */
void Layout_MergeColumns(HtmlCol *acc, HtmlCol *row, s32 n) {
    s32 i;
    s32 j;
    u16 d;

    for (i = 0; i < n; i++) {
        if (acc[i].flags & 1) {
            if (row[i].flags & 1) {
                if (acc[i].x < row[i].x) {
                    d = row[i].x - acc[i].x;
                    for (j = i; j < n; j++) {
                        acc[j].x += d;
                    }
                } else if (row[i].x < acc[i].x && !(row[i].flags & 8)) {
                    d = acc[i].x - row[i].x;
                    for (j = i; j < n; j++) {
                        row[j].x += d;
                    }
                }
            } else {
                if (acc[i].x < row[i].x) {
                    d = row[i].x - acc[i].x;
                    for (j = i; j < n; j++) {
                        if (row[j].flags & 1) {
                            break;
                        }
                        row[j].x -= d;
                    }
                } else if (row[i].x < acc[i].x && !(row[i].flags & 8)) {
                    d = acc[i].x - row[i].x;
                    for (j = i; j < n; j++) {
                        row[j].x += d;
                    }
                }
            }
        } else {
            if (row[i].flags & 1) {
                if (acc[i].x < row[i].x) {
                    acc[i].x += (u16)(row[i].x - acc[i].x);
                    for (j = i + 1; j < n; j++) {
                        if (acc[j].x >= acc[j - 1].x) {
                            break;
                        }
                        if (acc[j].flags & 1) {
                            d = acc[j - 1].x - acc[j].x;
                            for (; j < n; j++) {
                                acc[j].x += d;
                            }
                            break;
                        }
                        acc[j].x = acc[j - 1].x;
                    }
                    acc[i].flags = row[i].flags;
                } else if (row[i].x < acc[i].x) {
                    d = acc[i].x - row[i].x;
                    for (j = i; j < n; j++) {
                        if (acc[j].flags & 1) {
                            break;
                        }
                        acc[j].x -= d;
                    }
                    acc[i].flags = row[i].flags;
                }
            } else {
                if (acc[i].x < row[i].x) {
                    d = row[i].x - acc[i].x;
                    for (j = i; j < n; j++) {
                        acc[j].x += d;
                    }
                } else if (row[i].x < acc[i].x && !(row[i].flags & 8)) {
                    d = acc[i].x - row[i].x;
                    for (j = i; j < n; j++) {
                        row[j].x += d;
                    }
                }
            }
        }
    }
}

/* Measures a table's columns (from its first row, when measuring as asked):
 * each row's cells give their least and widest column edges, merged over
 * the rows; the table's width is spread over the columns, and each cell
 * gets its column's place and width. Returns the table's inner width. */
u16 Layout_MeasureColumns(HtmlCell *row, s32 measure) {
    HtmlCol minCols[32];
    HtmlCol maxCols[32];
    HtmlCol rowMin[32];
    HtmlCol rowMax[32];
    HtmlCell *r;
    s32 n;
    u16 xmin;
    u16 cx;
    u16 xmax;
    u16 tmax;
    HtmlCell *c;
    s32 i;
    s32 k;
    u16 wmin;
    u16 wmax;
    u16 x;

    if (gHtml->measure != measure) {
        return 0;
    }
    if (row->rowNo != 0) {
        return 0;
    }
    memset(minCols, 0, sizeof(minCols));
    memset(maxCols, 0, sizeof(maxCols));
    r = row;
    n = 0;
    do {
        memset(rowMin, 0, sizeof(rowMin));
        memset(rowMax, 0, sizeof(rowMax));
        i = 0;
        xmin = xmax = 0;
        for (c = r->first; c != NULL; c = c->next) {
            if (c->colSpan > 1) {
                wmin = wmax = 0;
                cx = c->minW;
                tmax = c->maxW;
                if (cx) {
                    wmin = (c->minW - (c->colSpan - 1) * (c->spacing + c->padding * 2)) / c->colSpan;
                }
                if (tmax) {
                    wmax = (tmax - (c->colSpan - 1) * (c->spacing + c->padding * 2)) / c->colSpan;
                }
                if (wmin > wmax || (c->sizeFlags & 1)) {
                    wmax = wmin;
                    tmax = cx;
                }
                cx += xmin;
                tmax += xmax;
                for (k = 0; k < c->colSpan - 1; k++) {
                    xmin += (u16)(wmin + (row->spacing + row->padding * 2));
                    xmax += (u16)(wmax + (row->spacing + row->padding * 2));
                    rowMin[i].x = xmin;
                    rowMin[i].flags = 8;
                    rowMax[i].x = xmax;
                    rowMax[i].flags = c->sizeFlags | 8;
                    c = c->next;
                    if (c == NULL) {
                        return 0;
                    }
                    if (++i >= 32) {
                        return 0;
                    }
                }
                xmin = cx + (row->spacing + row->padding * 2);
                rowMin[i].x = xmin;
                rowMin[i].flags = c->sizeFlags;
                xmax = tmax + (row->spacing + row->padding * 2);
                rowMax[i].x = xmax;
                rowMax[i].flags = c->sizeFlags;
            } else {
                wmin = c->minW + (row->spacing + row->padding * 2);
                wmax = c->maxW + (row->spacing + row->padding * 2);
                if (wmin > wmax || (c->sizeFlags & 1)) {
                    wmax = wmin;
                }
                xmin += wmin;
                xmax += wmax;
                rowMin[i].x = xmin;
                rowMin[i].flags = 1;
                rowMax[i].x = xmax;
                rowMax[i].flags = c->sizeFlags;
            }
            if (++i >= 32) {
                return 0;
            }
        }
        if (n < i) {
            n = i;
        }
        Layout_MergeColumns(minCols, rowMin, n);
        Layout_MergeColumns(maxCols, rowMax, n);
        r = r->next;
    } while (r != NULL);
    Layout_SpreadColumns(row->parent, minCols, maxCols, n);
    r = row;
    do {
        i = 0;
        x = 0;
        c = r->first;
        if (c != NULL) {
            xmin = c->x;
            do {
                if (c->colSpan) {
                    i += c->colSpan - 1;
                }
                c->x = xmin;
                c->w = minCols[i].x - x - (row->spacing + row->padding * 2);
                xmin += (u16)(minCols[i].x - x);
                x = minCols[i].x;
                if (c->colSpan) {
                    for (k = 0; k < c->colSpan - 1; k++) {
                        if (c->next == NULL) {
                            break;
                        }
                        c = c->next;
                    }
                }
                c = c->next;
                i++;
            } while (c != NULL);
        }
        r = r->next;
    } while (r != NULL);
    return minCols[n - 1].x;
}

/* Draws a table or cell's box and background (from the background stack). */
void Layout_DrawCell(HtmlCell *cell) {
    char *bg;

    if (gHtml->measure == 0 && gHtml->inFrame == 0) {
        bg = Layout_PopBackground();
        Draw_Cell(cell->x, cell->y, cell->x + cell->w, cell->y + cell->h, cell->depth,
                      cell->bgColor, cell->border, cell->borderColor, (u8 *)bg);
    }
}

/* Draws a cell's box and background (from the background stack). */
void Layout_DrawCellBox(HtmlCell *cell) {
    HtmlCell *parent;
    u16 h;
    s32 color;
    u8 border;
    char *bg;

    if (gHtml->measure == 0 && gHtml->inFrame == 0) {
        if (cell->w != 0 && cell->h != 0) {
            parent = cell->parent;
            if (parent != NULL && parent->parent != NULL) {
                border = parent->parent->border != 0;
                if (cell->rowSpan == 0) {
                    h = cell->padding + (cell->y + cell->h);
                } else {
                    h = cell->padding + (cell->y + cell->spanH);
                }
                color = cell->bgColor;
                if (color < 0) {
                    color = parent->bgColor;
                }
                bg = Layout_PopBackground();
                Draw_Table(cell->x - cell->padding, cell->y - cell->padding,
                              cell->padding + (cell->x + cell->w), h, cell->depth, color, border, (u8 *)bg);
            }
        } else {
            Layout_PopBackground();
        }
    }
}

/* Skips spaces at p in a cell, unless in <PRE>. */
u8 *Layout_CellSkipSpaces(u8 *p) {
    u8 c;

    if (gHtml->cellPen.lineUsed == 0 && !(gHtml->cells[gHtml->curCell].flags & 1)) {
        while (!(c = *p) == 0 && (gCharClass[c] & 2)) {
            p++;
        }
    }
    return p;
}

/* Layout_Rule in a cell: an <HR> (measuring: widens the cell). */
void Layout_CellRule(s32 center) {
    HtmlCell *cell;
    HtmlLine *line;
    u16 x;
    u16 *w;
    u16 y;
    u16 ind;

    line = &gHtml->lines[gHtml->lineNo];
    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure != -10) {
        x = gHtml->cellPen.runX + (cell->indent + gHtml->cellPen.left);
        w = &gHtml->hrWidth;
        if (*w == 0) {
            if (gHtml->measure != 0) {
                *w = 0x248;
            } else {
                *w = gHtml->cellPen.wrapW - cell->indent;
            }
        } else if (*w < 2) {
            *w = 2;
        }
        gHtml->cellPen.runY += 10;
        if (gHtml->measure == 0 && gHtml->inFrame == 0) {
            if (center != 0) {
                x += cell->alignX;
            }
            y = gHtml->cellPen.runY + (cell->valignY + gHtml->cellPen.h);
            Draw_Rule(x, y, x + gHtml->hrWidth, y + gHtml->hrSize);
        }
        gHtml->cellPen.runY += gHtml->hrSize + 10;
        gHtml->cellPen.y = gHtml->cellPen.runY;
        Html_NoteBottom(&gHtml->cellPen);
        if (gHtml->measure == 1) {
            line->w = x + gHtml->hrWidth;
            line->h = gHtml->hrSize + 20;
            cell->textH = gHtml->cellPen.y;
        }
        gHtml->hrSize = 0;
        Layout_CellAlign(cell);
        Layout_NextLine();
        Layout_CellAlign(cell);
        return;
    }
    ind = cell->indent;
    Html_MaxU16(&cell->minW, ind + gHtml->hrWidth);
    Html_MaxU16(&cell->maxW, ind + gHtml->hrWidth);
}

/* Layout_WrapBefore in a cell: wraps before a thing of width w unless it fits
 * (or NOWRAP); moves *px, *py to where it goes (measuring: widens the
 * cell). */
void Layout_CellWrapBefore(u16 *px, u16 *py, u16 w, HtmlLine **lp) {
    s32 ww = w;
    u16 x;
    u16 end;
    HtmlCell *cell;
    u16 base = 0;

    x = *px;
    end = x + ww;
    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure != -10) {
        if (gHtml->measure == 0) {
            *px = x + cell->alignX;
            x = *px;
            end = x + ww;
        }
        if (!(cell->flags & 2)) {
            if (gHtml->cellPen.x != 0 && gHtml->cellPen.wrapW < end) {
                *px = Layout_CellWrap(cell, lp, x);
                if (gHtml->measure == 0) {
                    base = cell->valignY;
                }
                *py = gHtml->cellPen.runY + (base + gHtml->cellPen.h);
            } else {
                *px += gHtml->cellPen.left;
                *py += gHtml->cellPen.h + cell->valignY;
            }
        } else {
            *px += gHtml->cellPen.left;
            *py += gHtml->cellPen.h + cell->valignY;
        }
        if (gHtml->measure == 1) {
            cell->textH = gHtml->cellPen.y;
        }
    } else if (!(cell->flags & 2) && cell->minW < end) {
        cell->minW = end;
    }
}

/* Layout_InputEnd in a cell: an <INPUT> of w characters (measuring: widens
 * the cell). */
void Layout_CellInput(u16 w) {
    u16 base = 0;
    u16 x;
    HtmlCell *cell;
    u16 pw = 0;
    u16 ind;

    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure != -10) {
        x = cell->indent + gHtml->cellPen.runX;
        /* FAKEMATCH: base is already 0, so this store changes nothing (MWCC drops
         * it and its test). The join it leaves keeps base in t2 (no constant
         * folded) and makes MWCC load gHtml again for runY and &cellPen.x, as the
         * game does; the original likely set base here (cf. Layout_CellPen's
         * valign) from a value we don't see. Only a compare with 0 leaves no code. */
        if (gHtml->measure == 0) {
            base = 0;
        }
        Layout_Input(x, base + gHtml->cellPen.runY, &gHtml->cellPen.runX, &gHtml->cellPen.x, w);
        return;
    }
    ind = cell->indent;
    switch (gHtml->input.type) {
    case 0:
    case 1:
        if (w < 8) {
            w = 8;
        }
        pw = w * 8 + 8;
        break;
    case 2:
    case 3:
        if (w < 2) {
            w = 2;
        }
        pw = w * 8 + 8;
        break;
    case 4:
    case 5:
        pw = 20;
        break;
    case 7:
        pw = w;
        break;
    }
    gHtml->cellPenX += pw;
    gHtml->cellPen.lineUsed = 1;
    if (cell->maxW < ind + gHtml->cellPenX) {
        cell->maxW = ind + gHtml->cellPenX;
    }
    if (cell->flags & 2) {
        if (cell->minW < cell->maxW) {
            cell->minW = cell->maxW;
        }
    } else if (cell->minW < ind + pw) {
        cell->minW = ind + pw;
    }
}

/* Layout_SelectEnd in a cell: </SELECT>'s box (measuring: widens the cell). */
void Layout_CellSelectEnd(void) {
    HtmlLine *lp;
    HtmlCell *cell;
    u16 x;
    u16 rx;
    u16 off;
    u16 y;
    u16 ind;
    s32 b;

    lp = &gHtml->lines[gHtml->lineNo];
    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure != -10) {
        rx = gHtml->cellPen.runX;
        x = cell->indent + rx;
        if (!(cell->flags & 2)) {
            if (rx != 0 && gHtml->cellPen.wrapW < x + (gHtml->selectWidth * 8 + 24)) {
                x = Layout_CellWrap(cell, &lp, x);
            } else {
                x += gHtml->cellPen.left;
            }
        } else {
            x += gHtml->cellPen.left;
        }
        gHtml->cellPen.x += gHtml->selectWidth * 8 + 24;
        gHtml->cellPenX += gHtml->selectWidth * 8 + 24;
        if (gHtml->measure == 0 && gHtml->inFrame == 0) {
            off = cell->alignX;
            x += off;
            y = gHtml->cellPen.h + (cell->valignY + gHtml->cellPen.runY);
            b = y + lp->h;
            Draw_SelectEnd(x, b - 20, off + (gHtml->cellPen.x + (cell->indent + gHtml->cellPen.left)), b);
        }
        gHtml->cellPen.runX = gHtml->cellPen.x;
        gHtml->cellPen.lineUsed = 1;
        if (gHtml->measure == 1) {
            if (lp->h < 20) {
                lp->h = 20;
            }
            cell->textH = gHtml->cellPen.y;
        }
    } else {
        ind = cell->indent;
        gHtml->cellPenX += gHtml->selectWidth * 8 + 24;
        gHtml->cellPen.lineUsed = 1;
        if (cell->maxW < ind + gHtml->cellPenX) {
            cell->maxW = ind + gHtml->cellPenX;
        }
        if (cell->flags & 2) {
            if (cell->minW < cell->maxW) {
                cell->minW = cell->maxW;
            }
        } else if (cell->minW < ind + gHtml->selectWidth * 8 + 24) {
            cell->minW = ind + gHtml->selectWidth * 8 + 24;
        }
    }
}

/* Layout_Image in a cell: an <IMG> laid out (measuring: widens the cell)
 * and drawn. */
void Layout_CellImage(void) {
    HtmlLine *lp;
    HtmlCell *cell;
    u16 base = 0;
    u16 x;
    u16 ind;
    u16 rx;

    lp = &gHtml->lines[gHtml->lineNo];
    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure != -10) {
        rx = gHtml->cellPen.runX;
        x = cell->indent + rx;
        if (!(cell->flags & 2)) {
            if (rx != 0 && gHtml->cellPen.wrapW < x + gHtml->imgWidth) {
                x = Layout_CellWrap(cell, &lp, x);
            } else {
                x += gHtml->cellPen.left;
            }
        } else {
            x += gHtml->cellPen.left;
        }
        if (gHtml->measure == 0) {
            x += cell->alignX;
            base = cell->valignY;
        }
        Layout_DrawImage(x, gHtml->cellPen.runY + (base + gHtml->cellPen.h), lp);
        gHtml->cellPen.runX = gHtml->cellPen.x = gHtml->cellPen.runX + gHtml->imgWidth;
        gHtml->cellPenX += gHtml->imgWidth;
        gHtml->cellPen.lineUsed += gHtml->imgWidth;
        if (gHtml->measure == 1) {
            Html_MaxU16(&lp->w, gHtml->cellPen.x);
            if (lp->h < gHtml->imgHeight) {
                lp->h = gHtml->imgHeight;
            }
            cell->textH = gHtml->cellPen.y;
        }
    } else {
        ind = cell->indent;
        gHtml->cellPenX += gHtml->imgWidth;
        gHtml->cellPen.lineUsed += gHtml->imgWidth;
        if (cell->maxW < ind + gHtml->cellPenX) {
            cell->maxW = ind + gHtml->cellPenX;
        }
        if (cell->flags & 2) {
            if (cell->minW < cell->maxW) {
                cell->minW = cell->maxW;
            }
        } else if (cell->minW < ind + gHtml->imgWidth) {
            cell->minW = ind + gHtml->imgWidth;
        }
    }
}

/* Layout_PlaceRun in a cell: lays out the run of text at the cell pen. */
void Layout_CellPlaceRun(HtmlLine *line) {
    HtmlCell *cell;
    u16 x;

    if (gHtml->measure == 0) {
        cell = &gHtml->cells[gHtml->curCell];
        x = gHtml->cellPen.runX + (cell->indent + gHtml->cellPen.left);
        Layout_CellAlign(cell);
        x += cell->alignX;
        Layout_DrawRun(x, gHtml->cellPen.runY + (cell->valignY + gHtml->cellPen.h), line);
    }
}

/* Layout_Wrap in a cell. */
u16 Layout_CellWrap(HtmlCell *cell, HtmlLine **lp, u16 x) {
    if (gHtml->measure == 1) {
        (*lp)->w = gHtml->cellPen.left + x;
    }
    gHtml->lineLen = 0;
    Layout_PenNewLine(&gHtml->cellPen, lp);
    Layout_NextLineEntry(lp);
    Html_NoteBottom(&gHtml->cellPen);
    Layout_CellAlign(cell);
    return gHtml->cellPen.runX + (cell->indent + gHtml->cellPen.left);
}

/* Layout_BreakLine in a cell. */
void Layout_CellBreakLine(HtmlLine **lp) {
    HtmlCell *cell;

    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure == 1) {
        (*lp)->w = gHtml->cellPen.x;
    }
    gHtml->pen.runW = gHtml->cellPen.x - gHtml->cellPen.runX;
    gHtml->line[gHtml->lineLen] = 0;
    Layout_CellPlaceRun(*lp);
    gHtml->lineLen = 0;
    Layout_PenNewLine(&gHtml->cellPen, lp);
    Layout_NextLineEntry(lp);
    Html_NoteBottom(&gHtml->cellPen);
    Layout_CellAlign(cell);
}

/* Lays out text in a cell (Layout_Text's cell version, from the cell's
 * pen and wrapping at the cell's width); measuring a table, only the
 * cell's least and widest widths are noted. -1: nothing to lay out. */
s32 Layout_CellText(u8 *text, HtmlLine **lp) {
    HtmlCell *cell;
    u8 *p;
    u8 space;
    u16 indent;

    cell = &gHtml->cells[gHtml->curCell];
    p = Layout_CellSkipSpaces(text);
    space = 0;
    indent = cell->indent;
    if (gHtml->measure != -10) {
        if (*p == 0) {
            return -1;
        }
        gHtml->paragraph = 0;
        for (; *p != 0; p++) {
            if (*p == ' ' || *p == '\t') {
                if (space != 0 && !(cell->flags & 1)) {
                    continue;
                }
            }
            if (*p == ' ' || *p == '\t') {
                space = 1;
            } else {
                space = 0;
            }
            if (gCharClass[*p] & 1) {
                if (cell->flags == 0 && gHtml->cellPen.wrapW < gHtml->charW + (indent + gHtml->cellPen.x)) {
                    Layout_CellBreakLine(lp);
                }
                Layout_AddWideChar(&p, &gHtml->cellPen);
                gHtml->cellPen.lineUsed += gHtml->charW;
            } else if (*p == 7) {
                if (gHtml->cellPen.wrapW < gHtml->halfW + (indent + gHtml->cellPen.x)) {
                    Layout_CellBreakLine(lp);
                }
                Layout_AddNbsp(&p, &gHtml->cellPen);
                gHtml->cellPen.lineUsed += gHtml->halfW;
            } else {
                if (cell->flags == 0 && gHtml->cellPen.wrapW < gHtml->halfW + (indent + gHtml->cellPen.x)) {
                    Layout_CellBreakLine(lp);
                    p = Layout_CellSkipSpaces(p);
                    if (*p == 0) {
                        break;
                    }
                }
                Layout_AddChar(&p, &gHtml->cellPen);
                gHtml->cellPen.lineUsed += gHtml->halfW;
            }
            if (gHtml->measure == 1) {
                Html_MaxU16(&(*lp)->h, gHtml->charW);
            }
        }
    } else {
        for (; *p != 0; p++) {
            if (*p == ' ' || *p == '\t') {
                if (space != 0 && !(cell->flags & 1)) {
                    continue;
                }
            }
            if (*p == ' ' || *p == '\t') {
                space = 1;
            } else {
                space = 0;
            }
            if (gCharClass[*p] & 1) {
                if (cell->flags == 0) {
                    Html_MaxU16(&cell->minW, indent + gHtml->charW);
                }
                gHtml->cellPenX += gHtml->charW;
                gHtml->cellPen.lineUsed += gHtml->charW;
                p++;
            } else {
                if (cell->flags == 0) {
                    Html_MaxU16(&cell->minW, indent + gHtml->halfW);
                }
                gHtml->cellPenX += gHtml->halfW;
                gHtml->cellPen.lineUsed += gHtml->halfW;
            }
        }
        Html_MaxU16(&cell->maxW, indent + gHtml->cellPenX);
        if (cell->flags != 0) {
            Html_MaxU16(&cell->minW, cell->maxW);
        }
    }
    return 0;
}

/* Layout_FlushRun in a cell. */
void Layout_CellFlushRun(HtmlLine *line) {
    if (gHtml->lineLen != 0) {
        gHtml->pen.runW = gHtml->cellPen.x - gHtml->cellPen.runX;
        gHtml->line[gHtml->lineLen] = 0;
        Layout_CellPlaceRun(line);
        gHtml->cellPen.runX = gHtml->cellPen.x;
        gHtml->lineLen = 0;
        gHtml->paragraph = 0;
    }
}

/* Layout_EndLine in a cell. */
void Layout_CellEndLine(HtmlLine **lp) {
    HtmlCell *cell;

    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure == 1) {
        (*lp)->w = gHtml->cellPen.x;
        if ((*lp)->h == 0) {
            (*lp)->h = gHtml->charW;
        }
    }
    gHtml->cellPenX = 0;
    Layout_PenNewLine(&gHtml->cellPen, lp);
    Layout_NextLineEntry(lp);
    Html_NoteBottom(&gHtml->cellPen);
    Layout_CellAlign(cell);
}

/* Layout_Flush in a cell. */
void Layout_CellFlush(u8 *text) {
    HtmlLine *lp;

    lp = &gHtml->lines[gHtml->lineNo];
    Layout_CellText(text, &lp);
    Layout_CellFlushRun(lp);
    gHtml->cellPen.runX = gHtml->cellPen.x;
    gHtml->cellPen.runY = gHtml->cellPen.y;
    if (gHtml->measure == 1) {
        gHtml->cells[gHtml->curCell].textH = gHtml->cellPen.y;
    }
    Html_NoteBottom(&gHtml->cellPen);
    gHtml->textLen = 0;
    *text = 0;
}

/* Layout_FlushBreak in a cell. */
void Layout_CellFlushBreak(u8 *text) {
    HtmlLine *lp;
    HtmlCell *cell;

    lp = &gHtml->lines[gHtml->lineNo];
    cell = &gHtml->cells[gHtml->curCell];
    Layout_CellText(text, &lp);
    Layout_CellFlushRun(lp);
    Layout_CellEndLine(&lp);
    if (gHtml->measure == 1) {
        gHtml->cells[gHtml->curCell].textH = gHtml->cellPen.y;
    }
    Html_NoteBottom(&gHtml->cellPen);
    Layout_CellAlign(cell);
    gHtml->textLen = 0;
    *text = 0;
}

/* Layout_FlushBlock in a cell. */
void Layout_CellFlushBlock(u8 *text) {
    HtmlLine *lp;
    HtmlCell *cell;

    lp = &gHtml->lines[gHtml->lineNo];
    cell = &gHtml->cells[gHtml->curCell];
    Layout_CellText(text, &lp);
    Layout_CellFlushRun(lp);
    if (gHtml->cellPen.runX != 0) {
        Layout_CellEndLine(&lp);
    }
    if (gHtml->measure == 1) {
        gHtml->cells[gHtml->curCell].textH = gHtml->cellPen.y;
    }
    Html_NoteBottom(&gHtml->cellPen);
    Layout_CellAlign(cell);
    gHtml->textLen = 0;
    *text = 0;
}

/* The image heap gImgHeap (an Arena, 0x10-aligned): 64 KB at the work
 * area's buf64k, 64 block records at gImgBlocks. */
extern Arena gImgHeap;
extern ArenaBlk gImgBlocks[];
extern BrowserWork *gBrowserWork;
void Arena_Init(Arena *heap); /* sub_main: set up a heap */

void Img_InitHeap(void)
{
    gImgHeap.align = 0x10;
    gImgHeap.base = (s32)gBrowserWork->buf64k;
    gImgHeap.size = 0x10000;
    gImgHeap.recs = gImgBlocks;
    gImgHeap.nrecs = 0x40;
    gImgHeap.unk16 = 2;
    gImgHeap.locked = 0;
    Arena_Init(&gImgHeap);
}

/* zlib's allocator (zutil.c's zcalloc, the game's own version): items *
 * size bytes from the image heap gImgHeap, cleared. */
void *Arena_Alloc(Arena *h, s32 size); /* sub_main: allocate from a heap */

void *zcalloc(void *opaque, u32 items, u32 size)
{
    u32 n = items * size;
    void *p;

    p = Arena_Alloc(&gImgHeap, n);
    if (p == NULL) {
        return NULL;
    }
    memset(p, 0, n);
    return p;
}
