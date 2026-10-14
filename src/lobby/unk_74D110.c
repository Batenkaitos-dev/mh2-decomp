/* The HTML table layout: rows and cells placed in their parents, a cell
 * takes <TD>'s attributes. */
#include "common.h"
#include "html.h"

/* A row's origin: its table's origin and the rows so far. */
void Layout_RowOrigin(HtmlCell *row, HtmlCell *table) {
    row->x = row->spacing + (table->x + table->penX);
    row->y = row->spacing + (table->y + table->penY);
    row->penX = 0;
    row->penY = 0;
}

/* A cell takes its row's settings and <TD>'s attributes (measuring). */
void Layout_CellFromRow(HtmlCell *cell, HtmlCell *row) {
    if (gHtml->measure == -10) {
        cell->colNo = row->colNo;
        cell->rowNo = row->rowNo;
        cell->border = row->border;
        cell->spacing = row->spacing;
        cell->padding = row->padding;
        cell->flags |= gHtml->nowrap;
        cell->bgColor = gHtml->tdAttr.bgColor;
        if (gHtml->tdAttr.align) {
            cell->align = gHtml->tdAttr.align;
        } else {
            cell->align = row->align;
        }
        if (gHtml->tdAttr.valign) {
            cell->valign = gHtml->tdAttr.valign;
        } else {
            cell->valign = row->valign;
        }
        cell->cellCount = 0;
        cell->minW = gHtml->tdAttr.width;
    }
    row->colNo++;
}

/* A cell's size from <TD WIDTH HEIGHT COLSPAN ROWSPAN> (measuring), or at
 * least its row's height. */
void Layout_CellSize(HtmlCell *cell, HtmlCell *row) {
    s32 n;

    if (gHtml->measure == -10) {
        cell->colSpan = gHtml->tdAttr.colSpan;
        cell->rowSpan = gHtml->tdAttr.rowSpan;
        cell->w = gHtml->tdAttr.width;
        cell->widthAttr = gHtml->tdAttr.width;
        if (cell->rowSpan == 0 || gHtml->tdAttr.height == 0) {
            cell->h = gHtml->tdAttr.height;
        } else {
            n = (cell->rowSpan - 1) * (cell->spacing + cell->padding * 2);
            if (gHtml->tdAttr.height < n) {
                cell->h = 0;
            } else {
                cell->h = (gHtml->tdAttr.height - n) / cell->rowSpan;
            }
        }
        if (gHtml->tdAttr.width != 0) {
            cell->sizeFlags |= 5;
        }
        if (gHtml->tdAttr.height != 0) {
            cell->sizeFlags |= 2;
        }
        if (cell->colSpan) {
            n = (u16)(cell->colSpan * (cell->spacing + cell->padding * 2));
            if (cell->w < n) {
                cell->w = n;
            }
        }
    } else {
        n = row->h - cell->padding * 2;
        if (cell->h < n) {
            cell->h = n;
        }
    }
}

/* A cell's origin in its row. */
void Layout_CellOrigin(HtmlCell *cell, HtmlCell *row) {
    cell->x = cell->padding + (row->x + row->penX);
    cell->y = cell->padding + (row->y + row->penY);
    cell->penX = 0;
    cell->penY = 0;
}
