/* The HTML tag handlers: <IMG ...>'s ">", <MAP ...>, <AREA ...>. */
#include "common.h"
#include "html.h"

void *memset(void *dst, s32 c, u32 n);

void Html_ReadAttrValue(u8 **cur, char *buf, s32 size);
void Layout_Image(void);

/* <IMG ...>'s ">": the image is laid out */
s32 Html_TagImgEnd(u8 **cur, u8 *text)
{
    Layout_Image();
    return 0;
}

/* <MAP NAME=...> */
s32 Html_AttrMapName(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->mapName, HTML_ATTR_MAX);
    return 0;
}

/* </MAP> */
s32 Html_TagEndMap(u8 **cur, u8 *text)
{
    memset(gHtml->mapName, 0, sizeof(gHtml->mapName));
    return 0;
}

/* <AREA ...> */
s32 Html_TagArea(u8 **cur, u8 *text)
{
    memset(&gHtml->area, 0, sizeof(HtmlArea));
    return 0;
}
