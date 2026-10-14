/* The HTML table layout: a row takes its table's settings. */
#include "common.h"
#include "html.h"

void Layout_RowFromTable(HtmlCell *row, HtmlCell *table) {
    if (gHtml->measure == -10) {
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
