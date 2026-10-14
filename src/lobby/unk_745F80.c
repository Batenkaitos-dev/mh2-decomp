/* The HTML tag handlers: <BODY ...>'s ">". */
#include "common.h"
#include "html.h"

void Html_ApplyBody(void);

s32 Html_TagBodyEnd(u8 **cur, u8 *text) {
    Html_ApplyBody();
    return 0;
}
