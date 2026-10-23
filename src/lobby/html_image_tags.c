/* The HTML tag handlers of the images: <IMG ...>, <MAP ...> and
 * <AREA ...>, with their attributes. */
#include "common.h"
#include "html.h"

void *memset(void *dst, s32 c, u32 n);
s32 strcmp(const char *a, const char *b); /* strcmp */

extern char gHtmlShapeRect[]; /* "RECT" */

/* <IMG>: laid out at once */
s32 Html_TagImg(u8 **cur, u8 *text)
{
    Layout_Flush(text);
    Html_ClearImgAttrs();
    Layout_Image();
    return 0;
}

/* <IMG ...> */
s32 Html_TagImgAttrs(u8 **cur, u8 *text)
{
    Layout_Flush(text);
    Html_ClearImgAttrs();
    return 0;
}

/* <IMG SRC=...> */
s32 Html_AttrImgSrc(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->imgSrc, sizeof(gHtml->imgSrc));
    return 0;
}

/* <IMG WIDTH=...> */
s32 Html_AttrImgWidth(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);  /* a byte less than buf */
    gHtml->imgWidth = Html_ParseNumber(buf);
    return 0;
}

/* <IMG HEIGHT=...> */
s32 Html_AttrImgHeight(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);  /* a byte less than buf */
    gHtml->imgHeight = Html_ParseNumber(buf);
    return 0;
}

/* <IMG ALT=...> */
s32 Html_AttrImgAlt(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->imgAlt, sizeof(gHtml->imgAlt));
    Html_DecodeEntities(gHtml->imgAlt);
    return 0;
}

/* <IMG USEMAP=...> */
s32 Html_AttrImgUsemap(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->imgUsemap, sizeof(gHtml->imgUsemap));
    return 0;
}

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

/* <AREA SHAPE=...> */
s32 Html_AttrAreaShape(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->area.shape, sizeof(gHtml->area.shape));
    return 0;
}

/* <AREA COORDS=x1,y1,x2,y2> */
s32 Html_AttrAreaCoords(u8 **cur, u8 *text)
{
    u8 buf[HTML_ATTR_MAX + 1];
    s32 i;
    u16 left;
    u8 *leftPos;
    u8 *topPos;
    u16 top;
    u8 *rightPos;
    u16 right;
    u8 *bottomPos;
    u16 bottom;

    Html_ReadAttrValue(cur, (char *)buf, HTML_ATTR_MAX);
    i = 0;
    left = 0;
    for (leftPos = buf; *leftPos != 0 && *leftPos != ','; leftPos++, i++) {
        if (*leftPos >= '0' && *leftPos <= '9') {
            left += *leftPos - '0';
            gHtml->area.coords[0] = left;
            left *= 10;
        }
    }
    if (buf[i] == 0) {
        return 0;
    }
    i++;
    top = 0;
    for (topPos = &buf[i]; *topPos != 0 && *topPos != ','; topPos++, i++) {
        if (*topPos >= '0' && *topPos <= '9') {
            top += *topPos - '0';
            gHtml->area.coords[1] = top;
            top *= 10;
        }
    }
    if (buf[i] == 0) {
        return 0;
    }
    i++;
    right = 0;
    for (rightPos = &buf[i]; *rightPos != 0 && *rightPos != ','; rightPos++, i++) {
        if (*rightPos >= '0' && *rightPos <= '9') {
            right += *rightPos - '0';
            gHtml->area.coords[2] = right;
            right *= 10;
        }
    }
    if (buf[i] == 0) {
        return 0;
    }
    i++;
    bottom = 0;
    for (bottomPos = &buf[i]; *bottomPos != 0 && *bottomPos != ','; bottomPos++, i++) {
        if (*bottomPos >= '0' && *bottomPos <= '9') {
            bottom += *bottomPos - '0';
            gHtml->area.coords[3] = bottom;
            bottom *= 10;
        }
    }
    return 0;
}

/* <AREA HREF=...> */
s32 Html_AttrAreaHref(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->area.href, HTML_ATTR_MAX);
    return 0;
}

/* <AREA ALT=...> */
s32 Html_AttrAreaAlt(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->area.alt, HTML_ATTR_MAX);
    return 0;
}

/* <AREA ...>'s ">": a rectangle is added to the map. */
s32 Html_TagAreaEnd(u8 **cur, u8 *text)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        gHtml->area.shape[i] = gToUpper[(u8)gHtml->area.shape[i]];
    }
    if (strcmp(gHtml->area.shape, gHtmlShapeRect) == 0) {
        Layout_MapArea();
    }
    return 0;
}
