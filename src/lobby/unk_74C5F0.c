/* The HTML layout: <TR>'s attributes cleared. */
#include "common.h"
#include "html.h"

void Html_ClearTrAttrs(void) {
    gHtml->tdAttr.align = 0;
    gHtml->tdAttr.valign = 0;
    gHtml->tdAttr.bgColor = -1;
}
