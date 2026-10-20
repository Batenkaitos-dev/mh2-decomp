/* The HTML table layout: a row takes its table's settings, rows and
 * cells are placed in their parents, a cell takes <TD>'s attributes. */
#include "common.h"
#include "html.h"

/* On the measuring pass, a new row takes its table's column, the next
 * row number, border, spacing and padding, the <TR> attributes, and the
 * table's width less its border and spacing. */
void Layout_RowFromTable(HtmlCell *row, HtmlCell *table)
{
    if (gHtml->measure == HTML_PASS_MEASURE) {
        row->colNo = table->colNo;
        row->rowNo = table->rowNo++;
        row->border = table->border;
        row->spacing = table->spacing;
        row->padding = table->padding;
        row->bgColor = gHtml->tdAttr.bgColor;
        row->align = gHtml->tdAttr.align;
        row->valign = gHtml->tdAttr.valign;
        row->sizeFlags = table->sizeFlags;
        row->cellCount = 0;
        row->minW = 0;
        row->w = table->w - (row->border + row->spacing) * 2;
    }
}

/* A row's origin: its table's origin and the rows so far. */
void Layout_RowOrigin(HtmlCell *row, HtmlCell *table)
{
    row->x = row->spacing + (table->x + table->penX);
    row->y = row->spacing + (table->y + table->penY);
    row->penX = 0;
    row->penY = 0;
}

/* A cell takes its row's settings and <TD>'s attributes (measuring). */
void Layout_CellFromRow(HtmlCell *cell, HtmlCell *row)
{
    if (gHtml->measure == HTML_PASS_MEASURE) {
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
void Layout_CellSize(HtmlCell *cell, HtmlCell *row)
{
    s32 n;

    if (gHtml->measure == HTML_PASS_MEASURE) {
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
            cell->sizeFlags |= HTML_SIZE_FIXED;
        }
        if (gHtml->tdAttr.height != 0) {
            cell->sizeFlags |= HTML_SIZE_HEIGHT;
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
void Layout_CellOrigin(HtmlCell *cell, HtmlCell *row)
{
    cell->x = cell->padding + (row->x + row->penX);
    cell->y = cell->padding + (row->y + row->penY);
    cell->penX = 0;
    cell->penY = 0;
}
