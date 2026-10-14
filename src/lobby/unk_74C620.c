/* The HTML table layout: cell alignment, new cells, the background stack. */
#include "common.h"
#include "html.h"

/* Compiled at optimization level 4, as the lobby's HTML parser
 * (unk_741900.c): every function of the file matches with it. */
#pragma optimization_level 4

void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);

/* Clears <TD>/<TH>'s attributes. */
void Html_ClearTdAttrs(void) {
    memset(&gHtml->tdAttr, 0, sizeof(HtmlTdAttr));
    gHtml->tdAttr.bgColor = -1;
    gHtml->nowrap = 0;
    gHtml->pre = 0;
}

/* A cell's y offset for its VALIGN (2 TOP, 3 BOTTOM, else the middle). */
void Layout_CellValign(HtmlCell *cell) {
    if (cell->flags & 1) {
        cell->valignY = 0;
        return;
    }
    if (cell->rowSpan == 0) {
        switch (cell->valign) {
        case 3:
            cell->valignY = gHtml->cellPen.y + cell->h - cell->textH;
            return;
        case 2:
            cell->valignY = 0;
            return;
        default:
            cell->valignY = (gHtml->cellPen.y + cell->h - cell->textH) / 2;
            return;
        }
    } else {
        switch (cell->valign) {
        case 3:
            cell->valignY = gHtml->cellPen.y + cell->spanH - cell->textH;
            return;
        case 2:
            cell->valignY = 0;
            return;
        default:
            cell->valignY = (gHtml->cellPen.y + cell->spanH - cell->textH) / 2;
            return;
        }
    }
}

/* A cell's x offset for the current line: centred (ALIGN=CENTER, in
 * <CENTER>, or a <TH>) or right-aligned. */
void Layout_CellAlign(HtmlCell *cell) {
    HtmlLine *line;
    s32 room;

    if (cell->flags & 1) {
        cell->alignX = 0;
        return;
    }
    line = &gHtml->lines[gHtml->lineNo];
    if (gHtml->measure == 0) {
        cell->alignX = 0;
        room = gHtml->cellPen.wrapW - cell->indent;
        if (line->w < room) {
            if (cell->align == 2 || cell->center != 0 || cell->kind == 3) {
                cell->alignX = (room - line->w) / 2;
            } else if (cell->align == 3) {
                cell->alignX = room - line->w;
            }
        }
    }
}

/* Opens a table, row or cell of the kind (1 <TABLE>, 2 <TR>, 3 <TH>, 4
 * <TD>): the next of cells[], pushed on the cell stack and (measuring)
 * linked as its parent's last child. -1: no room, or nested too deep. */
s32 Layout_OpenCell(s32 kind) {
    HtmlCell *cell;

    if (gHtml->cellCount >= 499) {
        return -1;
    }
    gHtml->curCell = ++gHtml->cellCount;
    cell = &gHtml->cells[gHtml->curCell];
    gHtml->tableDepth++;
    if (gHtml->tableDepth >= 16) {
        gHtml->tableDepth = 15;
        gHtml->maxDepth = 16;
        return -1;
    }
    if (gHtml->maxDepth < gHtml->tableDepth) {
        gHtml->maxDepth = gHtml->tableDepth;
    }
    gHtml->cellStack[gHtml->tableDepth] = gHtml->curCell;
    cell->open = 1;
    if (gHtml->measure != -10) {
        return 0;
    }
    cell->depth = gHtml->tableDepth;
    cell->index = gHtml->curCell;
    cell->parent = &gHtml->cells[gHtml->cellStack[gHtml->tableDepth - 1]];
    cell->kind = kind;
    cell->above = cell->parent->last;
    cell->next = NULL;
    cell->parent->last = cell;
    if (cell->above != NULL) {
        cell->above->next = cell;
    }
    if (cell->parent->first == NULL) {
        cell->parent->first = cell;
    }
    return 0;
}

/* Pushes a background (a table's or a cell's). */
void Layout_PushBackground(char *background) {
    if (gHtml->measure == 0) {
        memcpy(gHtml->bgStack[gHtml->bgDepth], background, 0x100);
        if (gHtml->bgDepth < 15) {
            gHtml->bgDepth++;
        }
    }
}

/* Pops a background; the one now on top. */
char *Layout_PopBackground(void) {
    if (gHtml->bgDepth != 0) {
        gHtml->bgDepth--;
    }
    return gHtml->bgStack[gHtml->bgDepth];
}

/* A table takes <TABLE>'s attributes (measuring). */
void Layout_TableFromAttrs(HtmlCell *table) {
    if (gHtml->measure == -10) {
        table->colNo = 0;
        table->rowNo = 0;
        table->w = gHtml->tableAttr.width;
        table->h = gHtml->tableAttr.height;
        table->widthAttr = gHtml->tableAttr.width;
        if (gHtml->tableAttr.width) {
            table->sizeFlags |= 5;
        }
        if (gHtml->tableAttr.height) {
            table->sizeFlags |= 2;
        }
        table->border = gHtml->tableAttr.border;
        if (gHtml->tableAttr.border != 0) {
            u16 *p = &gHtml->tableAttr.cellPadding;
            if (*p < 2) {
                *p = 2;
            }
        }
        table->spacing = gHtml->tableAttr.cellSpacing;
        table->padding = gHtml->tableAttr.cellPadding;
        table->cellCount = 0;
        table->minW = gHtml->tableAttr.width;
        {
            s32 n = (table->border + table->spacing) * 2;
            if (table->w < n) {
                table->w = n;
            }
        }
    }
}

/* A table's origin: on the page at the pen (centred in <CENTER>), or in its
 * cell, aligned by the cell's ALIGN and VALIGN. */
s32 Layout_TableOrigin(HtmlCell *table) {
    HtmlCell *parent;
    u16 pw;

    if (gHtml->tableDepth == 1) {
        if (gHtml->measure == -10) {
            table->bgColor = gHtml->tableAttr.bgColor;
            table->borderColor = gHtml->tableAttr.borderColor;
        }
        table->x = gHtml->pen.runX + gHtml->indent;
        table->y = gHtml->pen.runY;
        if (gHtml->measure == 0 && gHtml->center != 0) {
            table->x += (gHtml->width - gHtml->indent - table->w) / 2;
        }
        table->penX = table->border;
        table->penY = table->border;
    } else {
        parent = table->parent;
        if (parent == NULL) {
            return -1;
        }
        if (gHtml->measure == -10) {
            table->bgColor = gHtml->tableAttr.bgColor;
            table->borderColor = gHtml->tableAttr.borderColor;
        }
        table->align = parent->align;
        table->valign = parent->valign;
        parent->penY = gHtml->cellPen.y;
        table->x = parent->indent + (parent->x + parent->penX);
        table->y = parent->y + parent->penY;
        if (gHtml->measure == 0) {
            pw = parent->widthAttr ? parent->widthAttr : parent->w;
            if (table->w < pw) {
                switch (table->align) {
                case 2:
                    table->x += (pw - table->indent - table->w) / 2;
                    break;
                case 3:
                    table->x += pw - table->indent - table->w;
                    break;
                default:
                    if (parent->center) {
                        table->x += (pw - table->indent - table->w) / 2;
                    }
                    break;
                }
            }
            if (table->textH < parent->h) {
                if (parent->rowSpan == 0) {
                    switch (table->valign) {
                    case 2:
                        break;
                    case 3:
                        table->y += parent->h - parent->textH;
                        break;
                    default:
                        table->y += (parent->h - parent->textH) / 2;
                        break;
                    }
                } else {
                    switch (table->valign) {
                    case 2:
                        break;
                    case 3:
                        table->y += parent->spanH - parent->textH;
                        break;
                    default:
                        table->y += (parent->spanH - parent->textH) / 2;
                        break;
                    }
                }
            }
        }
        table->penX = table->border;
        table->penY = table->border;
    }
    return 0;
}
