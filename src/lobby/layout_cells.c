/* The HTML table layout, second part: tables, rows and cells opened and
 * closed around the tags (Layout_BeforeOpen, Layout_BeforeClose), the
 * cells that ROWSPAN and COLSPAN cells cover, the columns measured and the
 * table's width spread over them (Layout_MeasureColumns), the cells'
 * boxes drawn, and the cell versions of the text layout (Layout_CellText
 * and the others: each is layout_text.c's function of the same name
 * without "Cell", working from the cell's pen, gHtml->cellPen, and
 * wrapping at the cell's width).
 *
 * The layout runs three passes over the page (HTML_PASS_*): the
 * measuring pass notes each cell's least and widest widths, the second
 * places the columns, and the drawing pass draws.
 *
 * At the end, the lobby's image heap (gImgHeap, an Arena) and zcalloc,
 * the allocator the game gives zlib for the images it inflates. */
#include "common.h"
#include "html.h"
#include "arena.h"
#include "browser.h"

/* Compiled at optimization level 4, as the lobby's HTML parser
 * (html_parse.c): every function of the file matches with it. */
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

/* Raises *dst to value. */
static inline void Html_MaxU16(u16 *dst, s32 value)
{
    if (*dst < value) {
        *dst = value;
    }
}

/* The cell pen starts at a cell's origin. */
void Layout_CellPenStart(HtmlCell *cell, HtmlCell *parent)
{
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
    if (gHtml->measure == HTML_PASS_DRAW) {
        Layout_CellValign(cell);
        Layout_CellAlign(cell);
    }
}

/* The first cell of a ROWSPAN chain (followed up its spanPrev links); its
 * index. */
u8 Layout_CellChainEnd(HtmlCell *cell)
{
    u16 i;

    while ((i = cell->spanPrev) != 0) {
        cell = &gHtml->cells[i];
    }
    return cell->index;
}

/* A cell of a row that a cell above spans into (ROWSPAN): it takes that
 * cell's place, width and span, and is closed at once. */
s32 Layout_RowSpanCell(HtmlCell *cell, HtmlCell *row, HtmlCell *prev)
{
    u8 end;
    HtmlCell *first;

    if (gHtml->measure == HTML_PASS_MEASURE) {
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
    if (gHtml->measure == HTML_PASS_MEASURE) {
        cell->h = 0;
        cell->innerH = 0;
        cell->cellCount = 0;
        cell->minW = prev->minW;
        cell->sizeFlags = prev->sizeFlags;
    } else if (gHtml->measure == HTML_PASS_LAYOUT) {
        if (cell->rowSpan == 1) {
            s32 cellBottom = cell->h + (cell->padding + (row->y + row->penY));
            s32 spanBottom = gHtml->cells[end].y + cell->spanH;
            if (cellBottom < spanBottom) {
                cell->h += (u16)(spanBottom - cellBottom);
            }
        }
        if (row->h < cell->h + cell->padding * 2) {
            row->h = cell->h + cell->padding * 2;
        } else {
            cell->h = row->h - cell->padding * 2;
        }
    } else if (gHtml->measure == HTML_PASS_DRAW) {
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
        if (gHtml->measure == HTML_PASS_MEASURE) {
            row->minW += (u16)(cell->spacing + (cell->w + cell->padding * 2));
            row->cellCount++;
        }
    } else if (cell->spanLast != 0) {
        gHtml->spanCell = cell->spanFirst;
        first = &gHtml->cells[gHtml->spanCell];
        row->penX += (u16)(first->spacing + (first->w + first->padding * 2));
        if (gHtml->measure == HTML_PASS_MEASURE) {
            row->minW += (u16)(first->spacing + (first->w + first->padding * 2));
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
 * same column that spans into its row; otherwise the cell its spanPrev
 * names. */
HtmlCell *Layout_CellSpanAbove(HtmlCell *cell)
{
    HtmlCell *walk;
    u8 col;
    u16 i;

    if (gHtml->measure == HTML_PASS_MEASURE) {
        walk = cell->parent;
        if (walk == NULL) {
            return NULL;
        }
        if (walk->rowNo == 0) {
            return NULL;
        }
        walk = walk->above;
        if (walk == NULL) {
            return NULL;
        }
        walk = walk->first;
        if (walk == NULL) {
            return NULL;
        }
        col = cell->colNo;
        do {
            if (walk->colNo == col) {
                return (walk->rowSpan > 1) ? walk : NULL;
            }
            walk = walk->next;
        } while (walk != NULL);
    } else if ((i = cell->spanPrev) != 0) {
        return &gHtml->cells[i];
    }
    return NULL;
}

/* The cell of the row above in the same column that spans down into this
 * row (ROWSPAN >= 2), or NULL. */
HtmlCell *Layout_CellAbove(HtmlCell *cell)
{
    HtmlCell *walk;
    u8 col;

    if (cell->above == NULL) {
        return NULL;
    }
    walk = cell->above->first;
    if (walk == NULL) {
        return NULL;
    }
    col = cell->colNo;
    do {
        if (walk->colNo == col) {
            return (walk->rowSpan > 1) ? walk : NULL;
        }
        walk = walk->next;
    } while (walk != NULL);
    return NULL;
}

/* The cells that cells above span into, from prev on (a row's start). */
s32 Layout_RowSpanCells(HtmlCell *prev, HtmlCell *row)
{
    HtmlCell *cell;
    HtmlCell *parent;

    do {
        Html_ClearTdAttrs();
        if (Layout_OpenCell(HTML_CELL_TD) < 0) {
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
s32 Layout_CellSpanNext(void)
{
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
        if (Layout_OpenCell(HTML_CELL_TD) < 0) {
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
s32 Layout_CellColSpan(HtmlCell *cell)
{
    HtmlCell *first;
    HtmlCell *row;
    s32 i;

    if (cell->colSpan != 0) {
        cell->spanFirst = gHtml->spanCell = gHtml->curCell;
        first = &gHtml->cells[gHtml->spanCell];
        for (i = 0; i < first->colSpan - 1; i++) {
            if (Layout_OpenCell(HTML_CELL_TD) < 0) {
                return -1;
            }
            cell = &gHtml->cells[gHtml->curCell];
            row = cell->parent;
            if (row == NULL) {
                return -1;
            }
            if (gHtml->measure == HTML_PASS_MEASURE) {
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
        if (gHtml->measure == HTML_PASS_MEASURE) {
            cell->spanLast = 1;
        }
        row->penX += (u16)(first->spacing + (first->w + first->padding * 2));
        if (gHtml->measure == HTML_PASS_MEASURE) {
            row->minW += (u16)(first->spacing + (first->w + first->padding * 2));
            row->cellCount++;
        }
    }
    return 0;
}

/* Before <TABLE>, <TR>, <TH> or <TD> (kind, HTML_CELL_*): closes what the new one
 * cannot be inside, and opens a missing <TABLE> or <TR>. */
s32 Layout_BeforeOpen(u8 **cur, u8 *text, u8 kind)
{
    HtmlCell *cell;
    u16 i;

    gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
    i = gHtml->curCell;
    cell = &gHtml->cells[i];
    switch (kind) {
    case HTML_CELL_TABLE:
        if (i < 2) {
            return 0;
        }
        if (cell->kind == HTML_CELL_TABLE) {
            if (cell->open) {
                Html_TagEndTable(cur, text);
            }
        } else if (cell->kind == HTML_CELL_TR) {
            if (cell->open) {
                Html_TagEndTr(cur, text);
            }
            Html_TagEndTable(cur, text);
        }
        break;
    case HTML_CELL_TR:
        if (i == 0) {
            Html_ClearTableAttrs();
            if (Layout_OpenCell(HTML_CELL_TABLE) < 0) {
                return -1;
            }
            Html_TagTableEnd(cur, text);
        } else if (i < 2) {
            return 0;
        }
        if (cell->kind == HTML_CELL_TR) {
            if (cell->open) {
                Html_TagEndTr(cur, text);
            }
        } else if (cell->kind == HTML_CELL_TH || cell->kind == HTML_CELL_TD) {
            if (cell->open) {
                Html_TagEndTd(cur, text);
            }
            Html_TagEndTr(cur, text);
        }
        break;
    case HTML_CELL_TH:
    case HTML_CELL_TD:
        if (i == 0) {
            Html_ClearTableAttrs();
            if (Layout_OpenCell(HTML_CELL_TABLE) < 0) {
                return -1;
            }
            Html_TagTableEnd(cur, text);
        }
        if (cell->kind == HTML_CELL_TABLE) {
            Html_ClearTrAttrs();
            if (Layout_OpenCell(HTML_CELL_TR) < 0) {
                return -1;
            }
            Html_TagTrEnd(cur, text);
        } else if (cell->kind == HTML_CELL_TH || cell->kind == HTML_CELL_TD) {
            if (cell->open) {
                Html_TagEndTd(cur, text);
            }
        }
        break;
    }
    return 0;
}

/* Before </TABLE>, </TR>, </TH> or </TD> (kind, HTML_CELL_*): closes what is open
 * inside it, and opens a missing <TR> or <TD> (an empty table or row). */
s32 Layout_BeforeClose(u8 **cur, u8 *text, u8 kind)
{
    HtmlCell *cell;
    HtmlCell *prev;

    gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
    cell = &gHtml->cells[gHtml->curCell];
    switch (kind) {
    case HTML_CELL_TABLE:
        if (cell->kind == HTML_CELL_TABLE && cell->first == NULL) {
            Html_ClearTrAttrs();
            if (Layout_OpenCell(HTML_CELL_TR) < 0) {
                return -1;
            }
            Html_TagTrEnd(cur, text);
            Html_ClearTdAttrs();
            if (Layout_OpenCell(HTML_CELL_TD) < 0) {
                return -1;
            }
            Html_TagTdEnd(cur, text);
        }
        if (cell->kind == HTML_CELL_TR && cell->first == NULL) {
            Html_ClearTdAttrs();
            if (Layout_OpenCell(HTML_CELL_TD) < 0) {
                return -1;
            }
            Html_TagTdEnd(cur, text);
            gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
            cell = &gHtml->cells[gHtml->curCell];
        }
        if (cell->kind == HTML_CELL_TH || cell->kind == HTML_CELL_TD) {
            Html_TagEndTd(cur, text);
            gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
            cell = &gHtml->cells[gHtml->curCell];
        }
        if (cell->kind == HTML_CELL_TR) {
            Html_TagEndTr(cur, text);
        }
        break;
    case HTML_CELL_TR:
        if (cell->kind == HTML_CELL_TABLE) {
            if (cell->first == NULL) {
                Html_ClearTrAttrs();
                if (Layout_OpenCell(HTML_CELL_TR) < 0) {
                    return -1;
                }
                Html_TagTrEnd(cur, text);
                gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
                cell = &gHtml->cells[gHtml->curCell];
            } else if (cell->open) {
                Html_TagEndTable(cur, text);
            }
        }
        if (cell->kind == HTML_CELL_TR && (prev = Layout_CellAbove(cell)) != NULL) {
            if (Layout_RowSpanCells(prev, cell) < 0) {
                return -1;
            }
            gHtml->curCell = gHtml->cellStack[gHtml->tableDepth];
            cell = &gHtml->cells[gHtml->curCell];
        }
        if (cell->kind == HTML_CELL_TH || cell->kind == HTML_CELL_TD) {
            Html_TagEndTd(cur, text);
        }
        break;
    case HTML_CELL_TH:
    case HTML_CELL_TD:
        if (cell->kind == HTML_CELL_TABLE) {
            if (cell->first == NULL) {
                Html_ClearTrAttrs();
                if (Layout_OpenCell(HTML_CELL_TR) < 0) {
                    return -1;
                }
                Html_TagTrEnd(cur, text);
                Html_ClearTdAttrs();
                if (Layout_OpenCell(HTML_CELL_TD) < 0) {
                    return -1;
                }
                Html_TagTdEnd(cur, text);
            } else if (cell->open) {
                Html_TagEndTable(cur, text);
            }
        } else if (cell->kind == HTML_CELL_TR) {
            Html_ClearTdAttrs();
            if (Layout_OpenCell(HTML_CELL_TD) < 0) {
                return -1;
            }
            Html_TagTdEnd(cur, text);
        }
        break;
    }
    return 0;
}

/* Closes every open table, row and cell. */
s32 Html_CloseTables(u8 **cur, u8 *text)
{
    u8 depth;

    while ((depth = gHtml->tableDepth) != 0) {
        gHtml->curCell = gHtml->cellStack[depth];
        switch (gHtml->cells[gHtml->curCell].kind) {
        case HTML_CELL_TABLE:
            if (Html_TagEndTable(cur, text) < 0) {
                return -1;
            }
            break;
        case HTML_CELL_TR:
            if (Html_TagEndTr(cur, text) < 0) {
                return -1;
            }
            break;
        case HTML_CELL_TH:
        case HTML_CELL_TD:
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
void Layout_SpreadColumns(HtmlCell *table, HtmlCol *minCols, HtmlCol *maxCols, s32 n)
{
    u16 minW[HTML_COLS];
    u16 maxW[HTML_COLS];
    u16 total;
    u16 room;
    s32 w;
    u16 least;
    u16 prevMin;
    u16 prevMax;
    u8 cnt;
    u16 used;
    u16 given;
    u16 share;
    s32 i;

    total = 0;
    if (!(table->sizeFlags & HTML_SIZE_WIDTH)) {
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
    share = room - least;
    room = share;
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
            share = room - (total - maxW[i]) * room / total;
            w = share + minW[i];
            if (maxW[i] <= w) {
                used += (u16)(w - maxW[i]);
                minW[i] = maxW[i];
            } else if (share != 0) {
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
                share = room - (total - maxW[i]) * room / total;
                w = share + minW[i];
                if (maxW[i] <= w) {
                    used += (u16)(w - maxW[i]);
                    minW[i] = maxW[i];
                } else if (share != 0) {
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
            if (!(maxCols[i].flags & HTML_SIZE_NO_GROW)) {
                total += maxW[i];
            }
        }
        if (total == 0) {
            total = n;
        }
        for (i = 0; i < n; i++) {
            if (!(maxCols[i].flags & HTML_SIZE_NO_GROW)) {
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
void Layout_MergeColumns(HtmlCol *acc, HtmlCol *row, s32 n)
{
    s32 i;
    s32 j;
    u16 shift;

    for (i = 0; i < n; i++) {
        if (acc[i].flags & HTML_SIZE_WIDTH) {
            if (row[i].flags & HTML_SIZE_WIDTH) {
                if (acc[i].x < row[i].x) {
                    shift = row[i].x - acc[i].x;
                    for (j = i; j < n; j++) {
                        acc[j].x += shift;
                    }
                } else if (row[i].x < acc[i].x && !(row[i].flags & HTML_SIZE_SPANNED)) {
                    shift = acc[i].x - row[i].x;
                    for (j = i; j < n; j++) {
                        row[j].x += shift;
                    }
                }
            } else {
                if (acc[i].x < row[i].x) {
                    shift = row[i].x - acc[i].x;
                    for (j = i; j < n; j++) {
                        if (row[j].flags & HTML_SIZE_WIDTH) {
                            break;
                        }
                        row[j].x -= shift;
                    }
                } else if (row[i].x < acc[i].x && !(row[i].flags & HTML_SIZE_SPANNED)) {
                    shift = acc[i].x - row[i].x;
                    for (j = i; j < n; j++) {
                        row[j].x += shift;
                    }
                }
            }
        } else {
            if (row[i].flags & HTML_SIZE_WIDTH) {
                if (acc[i].x < row[i].x) {
                    acc[i].x += (u16)(row[i].x - acc[i].x);
                    for (j = i + 1; j < n; j++) {
                        if (acc[j].x >= acc[j - 1].x) {
                            break;
                        }
                        if (acc[j].flags & HTML_SIZE_WIDTH) {
                            shift = acc[j - 1].x - acc[j].x;
                            for (; j < n; j++) {
                                acc[j].x += shift;
                            }
                            break;
                        }
                        acc[j].x = acc[j - 1].x;
                    }
                    acc[i].flags = row[i].flags;
                } else if (row[i].x < acc[i].x) {
                    shift = acc[i].x - row[i].x;
                    for (j = i; j < n; j++) {
                        if (acc[j].flags & HTML_SIZE_WIDTH) {
                            break;
                        }
                        acc[j].x -= shift;
                    }
                    acc[i].flags = row[i].flags;
                }
            } else {
                if (acc[i].x < row[i].x) {
                    shift = row[i].x - acc[i].x;
                    for (j = i; j < n; j++) {
                        acc[j].x += shift;
                    }
                } else if (row[i].x < acc[i].x && !(row[i].flags & HTML_SIZE_SPANNED)) {
                    shift = acc[i].x - row[i].x;
                    for (j = i; j < n; j++) {
                        row[j].x += shift;
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
u16 Layout_MeasureColumns(HtmlCell *row, s32 measure)
{
    HtmlCol minCols[HTML_COLS];
    HtmlCol maxCols[HTML_COLS];
    HtmlCol rowMin[HTML_COLS];
    HtmlCol rowMax[HTML_COLS];
    HtmlCell *curRow;
    s32 n;
    u16 xmin;
    u16 spanMin;
    u16 xmax;
    u16 spanMax;
    HtmlCell *cell;
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
    curRow = row;
    n = 0;
    do {
        memset(rowMin, 0, sizeof(rowMin));
        memset(rowMax, 0, sizeof(rowMax));
        i = 0;
        xmin = xmax = 0;
        for (cell = curRow->first; cell != NULL; cell = cell->next) {
            if (cell->colSpan > 1) {
                wmin = wmax = 0;
                spanMin = cell->minW;
                spanMax = cell->maxW;
                if (spanMin) {
                    wmin = (cell->minW - (cell->colSpan - 1) * (cell->spacing + cell->padding * 2)) / cell->colSpan;
                }
                if (spanMax) {
                    wmax = (spanMax - (cell->colSpan - 1) * (cell->spacing + cell->padding * 2)) / cell->colSpan;
                }
                if (wmin > wmax || (cell->sizeFlags & HTML_SIZE_WIDTH)) {
                    wmax = wmin;
                    spanMax = spanMin;
                }
                spanMin += xmin;
                spanMax += xmax;
                for (k = 0; k < cell->colSpan - 1; k++) {
                    xmin += (u16)(wmin + (row->spacing + row->padding * 2));
                    xmax += (u16)(wmax + (row->spacing + row->padding * 2));
                    rowMin[i].x = xmin;
                    rowMin[i].flags = HTML_SIZE_SPANNED;
                    rowMax[i].x = xmax;
                    rowMax[i].flags = cell->sizeFlags | HTML_SIZE_SPANNED;
                    cell = cell->next;
                    if (cell == NULL) {
                        return 0;
                    }
                    if (++i >= HTML_COLS) {
                        return 0;
                    }
                }
                xmin = spanMin + (row->spacing + row->padding * 2);
                rowMin[i].x = xmin;
                rowMin[i].flags = cell->sizeFlags;
                xmax = spanMax + (row->spacing + row->padding * 2);
                rowMax[i].x = xmax;
                rowMax[i].flags = cell->sizeFlags;
            } else {
                wmin = cell->minW + (row->spacing + row->padding * 2);
                wmax = cell->maxW + (row->spacing + row->padding * 2);
                if (wmin > wmax || (cell->sizeFlags & HTML_SIZE_WIDTH)) {
                    wmax = wmin;
                }
                xmin += wmin;
                xmax += wmax;
                rowMin[i].x = xmin;
                rowMin[i].flags = HTML_SIZE_WIDTH;
                rowMax[i].x = xmax;
                rowMax[i].flags = cell->sizeFlags;
            }
            if (++i >= HTML_COLS) {
                return 0;
            }
        }
        if (n < i) {
            n = i;
        }
        Layout_MergeColumns(minCols, rowMin, n);
        Layout_MergeColumns(maxCols, rowMax, n);
        curRow = curRow->next;
    } while (curRow != NULL);
    Layout_SpreadColumns(row->parent, minCols, maxCols, n);
    curRow = row;
    do {
        i = 0;
        x = 0;
        cell = curRow->first;
        if (cell != NULL) {
            xmin = cell->x;
            do {
                if (cell->colSpan) {
                    i += cell->colSpan - 1;
                }
                cell->x = xmin;
                cell->w = minCols[i].x - x - (row->spacing + row->padding * 2);
                xmin += (u16)(minCols[i].x - x);
                x = minCols[i].x;
                if (cell->colSpan) {
                    for (k = 0; k < cell->colSpan - 1; k++) {
                        if (cell->next == NULL) {
                            break;
                        }
                        cell = cell->next;
                    }
                }
                cell = cell->next;
                i++;
            } while (cell != NULL);
        }
        curRow = curRow->next;
    } while (curRow != NULL);
    return minCols[n - 1].x;
}

/* Draws a table or cell's box and background (from the background stack). */
void Layout_DrawCell(HtmlCell *cell)
{
    char *background;

    if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
        background = Layout_PopBackground();
        Draw_Cell(cell->x, cell->y, cell->x + cell->w, cell->y + cell->h, cell->depth,
                      cell->bgColor, cell->border, cell->borderColor, (u8 *)background);
    }
}

/* Draws a cell's box and background (from the background stack). */
void Layout_DrawCellBox(HtmlCell *cell)
{
    HtmlCell *parent;
    u16 bottom;
    s32 color;
    u8 border;
    char *background;

    if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
        if (cell->w != 0 && cell->h != 0) {
            parent = cell->parent;
            if (parent != NULL && parent->parent != NULL) {
                border = parent->parent->border != 0;
                if (cell->rowSpan == 0) {
                    bottom = cell->padding + (cell->y + cell->h);
                } else {
                    bottom = cell->padding + (cell->y + cell->spanH);
                }
                color = cell->bgColor;
                if (color < 0) {
                    color = parent->bgColor;
                }
                background = Layout_PopBackground();
                Draw_Table(cell->x - cell->padding, cell->y - cell->padding,
                              cell->padding + (cell->x + cell->w), bottom, cell->depth, color, border, (u8 *)background);
            }
        } else {
            Layout_PopBackground();
        }
    }
}

/* Skips spaces at p in a cell, unless in <PRE>. */
u8 *Layout_CellSkipSpaces(u8 *text)
{
    u8 byte;

    if (gHtml->cellPen.lineUsed == 0 && !(gHtml->cells[gHtml->curCell].flags & HTML_FLAG_PRE)) {
        while (!(byte = *text) == 0 && (gCharClass[byte] & HTML_CHAR_BLANK)) {
            text++;
        }
    }
    return text;
}

/* Layout_Rule in a cell: an <HR> (measuring: widens the cell). */
void Layout_CellRule(s32 center)
{
    HtmlCell *cell;
    HtmlLine *line;
    u16 x;
    u16 *w;
    u16 y;
    u16 ind;

    line = &gHtml->lines[gHtml->lineNo];
    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure != HTML_PASS_MEASURE) {
        x = gHtml->cellPen.runX + (cell->indent + gHtml->cellPen.left);
        w = &gHtml->hrWidth;
        if (*w == 0) {
            if (gHtml->measure != HTML_PASS_DRAW) {
                *w = PAGE_VIEW_W;
            } else {
                *w = gHtml->cellPen.wrapW - cell->indent;
            }
        } else if (*w < 2) {
            *w = 2;
        }
        gHtml->cellPen.runY += 10;
        if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
            if (center != 0) {
                x += cell->alignX;
            }
            y = gHtml->cellPen.runY + (cell->valignY + gHtml->cellPen.h);
            Draw_Rule(x, y, x + gHtml->hrWidth, y + gHtml->hrSize);
        }
        gHtml->cellPen.runY += gHtml->hrSize + 10;
        gHtml->cellPen.y = gHtml->cellPen.runY;
        Html_NoteBottom(&gHtml->cellPen);
        if (gHtml->measure == HTML_PASS_LAYOUT) {
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
void Layout_CellWrapBefore(u16 *xPtr, u16 *yPtr, u16 w, HtmlLine **linePtr)
{
    s32 width = w;
    u16 x;
    u16 end;
    HtmlCell *cell;
    u16 base = 0;

    x = *xPtr;
    end = x + width;
    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure != HTML_PASS_MEASURE) {
        if (gHtml->measure == HTML_PASS_DRAW) {
            *xPtr = x + cell->alignX;
            x = *xPtr;
            end = x + width;
        }
        if (!(cell->flags & HTML_FLAG_NOWRAP)) {
            if (gHtml->cellPen.x != 0 && gHtml->cellPen.wrapW < end) {
                *xPtr = Layout_CellWrap(cell, linePtr, x);
                if (gHtml->measure == HTML_PASS_DRAW) {
                    base = cell->valignY;
                }
                *yPtr = gHtml->cellPen.runY + (base + gHtml->cellPen.h);
            } else {
                *xPtr += gHtml->cellPen.left;
                *yPtr += gHtml->cellPen.h + cell->valignY;
            }
        } else {
            *xPtr += gHtml->cellPen.left;
            *yPtr += gHtml->cellPen.h + cell->valignY;
        }
        if (gHtml->measure == HTML_PASS_LAYOUT) {
            cell->textH = gHtml->cellPen.y;
        }
    } else if (!(cell->flags & HTML_FLAG_NOWRAP) && cell->minW < end) {
        cell->minW = end;
    }
}

/* Layout_InputEnd in a cell: an <INPUT> of w characters (measuring: widens
 * the cell). */
void Layout_CellInput(u16 w)
{
    u16 base = 0;
    u16 x;
    HtmlCell *cell;
    u16 boxW = 0;
    u16 ind;

    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure != HTML_PASS_MEASURE) {
        x = cell->indent + gHtml->cellPen.runX;
        /* FAKEMATCH: base is already 0, so this test and its store change
         * nothing, and the compiler drops them. The branch they leave makes
         * it keep base in a register instead of folding the 0, and read
         * gHtml again for runY and &cellPen.x, as the game's code does. The
         * other cell functions set base from the cell's valignY here; this
         * one's real value isn't known. */
        if (gHtml->measure == HTML_PASS_DRAW) {
            base = 0;
        }
        Layout_Input(x, base + gHtml->cellPen.runY, &gHtml->cellPen.runX, &gHtml->cellPen.x, w);
        return;
    }
    ind = cell->indent;
    switch (gHtml->input.type) {
    case HTML_INPUT_TEXT:
    case HTML_INPUT_PASSWORD:
        if (w < 8) {
            w = 8;
        }
        boxW = w * 8 + 8;
        break;
    case HTML_INPUT_SUBMIT:
    case HTML_INPUT_RESET:
        if (w < 2) {
            w = 2;
        }
        boxW = w * 8 + 8;
        break;
    case HTML_INPUT_RADIO:
    case HTML_INPUT_CHECKBOX:
        boxW = 20;
        break;
    case HTML_INPUT_IMAGE:
        boxW = w;
        break;
    }
    gHtml->cellPenX += boxW;
    gHtml->cellPen.lineUsed = 1;
    if (cell->maxW < ind + gHtml->cellPenX) {
        cell->maxW = ind + gHtml->cellPenX;
    }
    if (cell->flags & HTML_FLAG_NOWRAP) {
        if (cell->minW < cell->maxW) {
            cell->minW = cell->maxW;
        }
    } else if (cell->minW < ind + boxW) {
        cell->minW = ind + boxW;
    }
}

/* Layout_SelectEnd in a cell: </SELECT>'s box (measuring: widens the cell). */
void Layout_CellSelectEnd(void)
{
    HtmlLine *line;
    HtmlCell *cell;
    u16 x;
    u16 runStart;
    u16 off;
    u16 y;
    u16 ind;
    s32 bottom;

    line = &gHtml->lines[gHtml->lineNo];
    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure != HTML_PASS_MEASURE) {
        runStart = gHtml->cellPen.runX;
        x = cell->indent + runStart;
        if (!(cell->flags & HTML_FLAG_NOWRAP)) {
            if (runStart != 0 && gHtml->cellPen.wrapW < x + (gHtml->selectWidth * 8 + 24)) {
                x = Layout_CellWrap(cell, &line, x);
            } else {
                x += gHtml->cellPen.left;
            }
        } else {
            x += gHtml->cellPen.left;
        }
        gHtml->cellPen.x += gHtml->selectWidth * 8 + 24;
        gHtml->cellPenX += gHtml->selectWidth * 8 + 24;
        if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
            off = cell->alignX;
            x += off;
            y = gHtml->cellPen.h + (cell->valignY + gHtml->cellPen.runY);
            bottom = y + line->h;
            Draw_SelectEnd(x, bottom - 20, off + (gHtml->cellPen.x + (cell->indent + gHtml->cellPen.left)), bottom);
        }
        gHtml->cellPen.runX = gHtml->cellPen.x;
        gHtml->cellPen.lineUsed = 1;
        if (gHtml->measure == HTML_PASS_LAYOUT) {
            if (line->h < 20) {
                line->h = 20;
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
        if (cell->flags & HTML_FLAG_NOWRAP) {
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
void Layout_CellImage(void)
{
    HtmlLine *line;
    HtmlCell *cell;
    u16 base = 0;
    u16 x;
    u16 ind;
    u16 runStart;

    line = &gHtml->lines[gHtml->lineNo];
    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure != HTML_PASS_MEASURE) {
        runStart = gHtml->cellPen.runX;
        x = cell->indent + runStart;
        if (!(cell->flags & HTML_FLAG_NOWRAP)) {
            if (runStart != 0 && gHtml->cellPen.wrapW < x + gHtml->imgWidth) {
                x = Layout_CellWrap(cell, &line, x);
            } else {
                x += gHtml->cellPen.left;
            }
        } else {
            x += gHtml->cellPen.left;
        }
        if (gHtml->measure == HTML_PASS_DRAW) {
            x += cell->alignX;
            base = cell->valignY;
        }
        Layout_DrawImage(x, gHtml->cellPen.runY + (base + gHtml->cellPen.h), line);
        gHtml->cellPen.runX = gHtml->cellPen.x = gHtml->cellPen.runX + gHtml->imgWidth;
        gHtml->cellPenX += gHtml->imgWidth;
        gHtml->cellPen.lineUsed += gHtml->imgWidth;
        if (gHtml->measure == HTML_PASS_LAYOUT) {
            Html_MaxU16(&line->w, gHtml->cellPen.x);
            if (line->h < gHtml->imgHeight) {
                line->h = gHtml->imgHeight;
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
        if (cell->flags & HTML_FLAG_NOWRAP) {
            if (cell->minW < cell->maxW) {
                cell->minW = cell->maxW;
            }
        } else if (cell->minW < ind + gHtml->imgWidth) {
            cell->minW = ind + gHtml->imgWidth;
        }
    }
}

/* Layout_PlaceRun in a cell: lays out the run of text at the cell pen. */
void Layout_CellPlaceRun(HtmlLine *line)
{
    HtmlCell *cell;
    u16 x;

    if (gHtml->measure == HTML_PASS_DRAW) {
        cell = &gHtml->cells[gHtml->curCell];
        x = gHtml->cellPen.runX + (cell->indent + gHtml->cellPen.left);
        Layout_CellAlign(cell);
        x += cell->alignX;
        Layout_DrawRun(x, gHtml->cellPen.runY + (cell->valignY + gHtml->cellPen.h), line);
    }
}

/* Layout_Wrap in a cell. */
u16 Layout_CellWrap(HtmlCell *cell, HtmlLine **linePtr, u16 x)
{
    if (gHtml->measure == HTML_PASS_LAYOUT) {
        (*linePtr)->w = gHtml->cellPen.left + x;
    }
    gHtml->lineLen = 0;
    Layout_PenNewLine(&gHtml->cellPen, linePtr);
    Layout_NextLineEntry(linePtr);
    Html_NoteBottom(&gHtml->cellPen);
    Layout_CellAlign(cell);
    return gHtml->cellPen.runX + (cell->indent + gHtml->cellPen.left);
}

/* Layout_BreakLine in a cell. */
void Layout_CellBreakLine(HtmlLine **linePtr)
{
    HtmlCell *cell;

    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure == HTML_PASS_LAYOUT) {
        (*linePtr)->w = gHtml->cellPen.x;
    }
    gHtml->pen.runW = gHtml->cellPen.x - gHtml->cellPen.runX;
    gHtml->line[gHtml->lineLen] = 0;
    Layout_CellPlaceRun(*linePtr);
    gHtml->lineLen = 0;
    Layout_PenNewLine(&gHtml->cellPen, linePtr);
    Layout_NextLineEntry(linePtr);
    Html_NoteBottom(&gHtml->cellPen);
    Layout_CellAlign(cell);
}

/* Lays out text in a cell (Layout_Text's cell version, from the cell's
 * pen and wrapping at the cell's width); measuring a table, only the
 * cell's least and widest widths are noted. -1: nothing to lay out. */
s32 Layout_CellText(u8 *text, HtmlLine **linePtr)
{
    HtmlCell *cell;
    u8 *pos;
    u8 space;
    u16 indent;

    cell = &gHtml->cells[gHtml->curCell];
    pos = Layout_CellSkipSpaces(text);
    space = 0;
    indent = cell->indent;
    if (gHtml->measure != HTML_PASS_MEASURE) {
        if (*pos == 0) {
            return -1;
        }
        gHtml->paragraph = 0;
        for (; *pos != 0; pos++) {
            if (*pos == ' ' || *pos == '\t') {
                if (space != 0 && !(cell->flags & HTML_FLAG_PRE)) {
                    continue;
                }
            }
            if (*pos == ' ' || *pos == '\t') {
                space = 1;
            } else {
                space = 0;
            }
            if (gCharClass[*pos] & HTML_CHAR_SJIS_LEAD) {
                if (cell->flags == 0 && gHtml->cellPen.wrapW < gHtml->charW + (indent + gHtml->cellPen.x)) {
                    Layout_CellBreakLine(linePtr);
                }
                Layout_AddWideChar(&pos, &gHtml->cellPen);
                gHtml->cellPen.lineUsed += gHtml->charW;
            } else if (*pos == HTML_CHAR_NBSP) {
                if (gHtml->cellPen.wrapW < gHtml->halfW + (indent + gHtml->cellPen.x)) {
                    Layout_CellBreakLine(linePtr);
                }
                Layout_AddNbsp(&pos, &gHtml->cellPen);
                gHtml->cellPen.lineUsed += gHtml->halfW;
            } else {
                if (cell->flags == 0 && gHtml->cellPen.wrapW < gHtml->halfW + (indent + gHtml->cellPen.x)) {
                    Layout_CellBreakLine(linePtr);
                    pos = Layout_CellSkipSpaces(pos);
                    if (*pos == 0) {
                        break;
                    }
                }
                Layout_AddChar(&pos, &gHtml->cellPen);
                gHtml->cellPen.lineUsed += gHtml->halfW;
            }
            if (gHtml->measure == HTML_PASS_LAYOUT) {
                Html_MaxU16(&(*linePtr)->h, gHtml->charW);
            }
        }
    } else {
        for (; *pos != 0; pos++) {
            if (*pos == ' ' || *pos == '\t') {
                if (space != 0 && !(cell->flags & HTML_FLAG_PRE)) {
                    continue;
                }
            }
            if (*pos == ' ' || *pos == '\t') {
                space = 1;
            } else {
                space = 0;
            }
            if (gCharClass[*pos] & HTML_CHAR_SJIS_LEAD) {
                if (cell->flags == 0) {
                    Html_MaxU16(&cell->minW, indent + gHtml->charW);
                }
                gHtml->cellPenX += gHtml->charW;
                gHtml->cellPen.lineUsed += gHtml->charW;
                pos++;
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
void Layout_CellFlushRun(HtmlLine *line)
{
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
void Layout_CellEndLine(HtmlLine **linePtr)
{
    HtmlCell *cell;

    cell = &gHtml->cells[gHtml->curCell];
    if (gHtml->measure == HTML_PASS_LAYOUT) {
        (*linePtr)->w = gHtml->cellPen.x;
        if ((*linePtr)->h == 0) {
            (*linePtr)->h = gHtml->charW;
        }
    }
    gHtml->cellPenX = 0;
    Layout_PenNewLine(&gHtml->cellPen, linePtr);
    Layout_NextLineEntry(linePtr);
    Html_NoteBottom(&gHtml->cellPen);
    Layout_CellAlign(cell);
}

/* Layout_Flush in a cell. */
void Layout_CellFlush(u8 *text)
{
    HtmlLine *line;

    line = &gHtml->lines[gHtml->lineNo];
    Layout_CellText(text, &line);
    Layout_CellFlushRun(line);
    gHtml->cellPen.runX = gHtml->cellPen.x;
    gHtml->cellPen.runY = gHtml->cellPen.y;
    if (gHtml->measure == HTML_PASS_LAYOUT) {
        gHtml->cells[gHtml->curCell].textH = gHtml->cellPen.y;
    }
    Html_NoteBottom(&gHtml->cellPen);
    gHtml->textLen = 0;
    *text = 0;
}

/* Layout_FlushBreak in a cell. */
void Layout_CellFlushBreak(u8 *text)
{
    HtmlLine *line;
    HtmlCell *cell;

    line = &gHtml->lines[gHtml->lineNo];
    cell = &gHtml->cells[gHtml->curCell];
    Layout_CellText(text, &line);
    Layout_CellFlushRun(line);
    Layout_CellEndLine(&line);
    if (gHtml->measure == HTML_PASS_LAYOUT) {
        gHtml->cells[gHtml->curCell].textH = gHtml->cellPen.y;
    }
    Html_NoteBottom(&gHtml->cellPen);
    Layout_CellAlign(cell);
    gHtml->textLen = 0;
    *text = 0;
}

/* Layout_FlushBlock in a cell. */
void Layout_CellFlushBlock(u8 *text)
{
    HtmlLine *line;
    HtmlCell *cell;

    line = &gHtml->lines[gHtml->lineNo];
    cell = &gHtml->cells[gHtml->curCell];
    Layout_CellText(text, &line);
    Layout_CellFlushRun(line);
    if (gHtml->cellPen.runX != 0) {
        Layout_CellEndLine(&line);
    }
    if (gHtml->measure == HTML_PASS_LAYOUT) {
        gHtml->cells[gHtml->curCell].textH = gHtml->cellPen.y;
    }
    Html_NoteBottom(&gHtml->cellPen);
    Layout_CellAlign(cell);
    gHtml->textLen = 0;
    *text = 0;
}

extern Arena gImgHeap;
extern ArenaBlk gImgBlocks[IMG_HEAP_RECS];
extern BrowserWork *gBrowserWork;
void Arena_Init(Arena *heap); /* sub_main: set up a heap */

/* Sets up the image heap gImgHeap: an Arena of 16-byte-aligned blocks
 * over the browser work area's buf64k, with gImgBlocks' records. */
void Img_InitHeap(void)
{
    gImgHeap.align = 0x10;
    gImgHeap.base = (s32)gBrowserWork->buf64k;
    gImgHeap.size = IMG_HEAP_SIZE;
    gImgHeap.recs = gImgBlocks;
    gImgHeap.nrecs = IMG_HEAP_RECS;
    gImgHeap.unk16 = 2;
    gImgHeap.locked = 0;
    Arena_Init(&gImgHeap);
}

void *Arena_Alloc(Arena *h, s32 size); /* sub_main: allocate from a heap */

/* zlib's allocator, the game's own version of zcalloc: items * size
 * bytes from the image heap gImgHeap, cleared. NULL when the heap is full. */
void *zcalloc(void *opaque, u32 items, u32 size)
{
    u32 n = items * size;
    void *mem;

    mem = Arena_Alloc(&gImgHeap, n);
    if (mem == NULL) {
        return NULL;
    }
    memset(mem, 0, n);
    return mem;
}
