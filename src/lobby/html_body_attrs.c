/* The HTML tag handlers: <AREA>'s attributes, <BODY ...>'s attributes
 * and its closing ">", which applies them. */
#include "common.h"
#include "html.h"

s32 strcmp(const char *a, const char *b); /* strcmp */

void Html_ReadAttrValue(u8 **cur, char *buf, s32 size);
void Layout_MapArea(void);
s32 Html_TagBodyEnd(u8 **cur, u8 *text);
void Html_ApplyBody(void);

extern u8 gToUpper[]; /* to upper case */
extern char gHtmlShapeRect[]; /* "RECT" */

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

    for (i = 0; i < 0x10; i++) {
        gHtml->area.shape[i] = gToUpper[(u8)gHtml->area.shape[i]];
    }
    if (strcmp(gHtml->area.shape, gHtmlShapeRect) == 0) {
        Layout_MapArea();
    }
    return 0;
}

/* <BODY> */
s32 Html_TagBody(u8 **cur, u8 *text)
{
    gHtml->textLen = 0;
    *text = 0;
    Html_TagBodyEnd(cur, text);
    return 0;
}

/* <BODY BGCOLOR=...> */
s32 Html_AttrBodyBgcolor(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->bgColor, sizeof(gHtml->bgColor));
    return 0;
}

/* <BODY BACKGROUND=...> */
s32 Html_AttrBodyBackground(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->background, HTML_ATTR_MAX);
    return 0;
}

/* <BODY TEXT=...> */
s32 Html_AttrBodyText(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->textColor, sizeof(gHtml->textColor));
    return 0;
}

/* <BODY LINK=...> */
s32 Html_AttrBodyLink(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->linkColor, sizeof(gHtml->linkColor));
    return 0;
}

/* <BODY ...>'s ">": the attributes read are applied to the page. */
s32 Html_TagBodyEnd(u8 **cur, u8 *text)
{
    Html_ApplyBody();
    return 0;
}
