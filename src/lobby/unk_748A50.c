/* The HTML tag handlers: <TD NOWRAP>. */
#include "common.h"
#include "html.h"

/* <TD NOWRAP>, <TH NOWRAP> */
s32 Html_AttrTdNowrap(u8 **cur, u8 *text) {
    if (gHtml->measure != -10) {
        return 0;
    }
    gHtml->nowrap = 2;
    return 0;
}
