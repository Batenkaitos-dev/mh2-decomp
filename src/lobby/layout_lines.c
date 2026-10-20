/* The HTML layout: the next line. */
#include "common.h"
#include "html.h"

/* Goes to the next line (staying on the last of HTML_LINES). */
void Layout_NextLine(void)
{
    u16 line;

    line = gHtml->lineNo;
    gHtml->lineNo = line + 1;
    if (line >= HTML_LINES - 1) {
        gHtml->lineNo = HTML_LINES - 1;
    }
}

/* Goes to the next line, and *lp to its entry. */
void Layout_NextLineEntry(HtmlLine **linePtr)
{
    u16 line;

    line = gHtml->lineNo;
    gHtml->lineNo = line + 1;
    if (line >= HTML_LINES - 1) {
        gHtml->lineNo = HTML_LINES - 1;
        return;
    }
    (*linePtr)++;
}
