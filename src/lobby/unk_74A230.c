/* The HTML layout: the next line. */
#include "common.h"
#include "html.h"

/* Goes to the next line (at most 999). */
void Layout_NextLine(void) {
    u16 n;

    n = gHtml->lineNo;
    gHtml->lineNo = n + 1;
    if (n >= 999) {
        gHtml->lineNo = 999;
    }
}

/* Goes to the next line, and *lp to its entry. */
void Layout_NextLineEntry(HtmlLine **lp) {
    u16 n;

    n = gHtml->lineNo;
    gHtml->lineNo = n + 1;
    if (n >= 999) {
        gHtml->lineNo = 999;
        return;
    }
    (*lp)++;
}
