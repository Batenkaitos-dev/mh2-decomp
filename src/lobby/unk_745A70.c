/* The HTML tag handlers: <AREA>'s attributes, <BODY ...>'s attributes. */
#include "common.h"
#include "html.h"

s32 strcmp(const char *a, const char *b); /* strcmp */

void Html_ReadAttrValue(u8 **cur, char *buf, s32 size);
void Layout_MapArea(void);
s32 Html_TagBodyEnd(u8 **cur, u8 *text);

extern u8 gToUpper[]; /* to upper case */
extern char D_00839EF0[]; /* "RECT" */

/* <AREA SHAPE=...> */
s32 Html_AttrAreaShape(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->area.shape, 0x10);
    return 0;
}

/* <AREA COORDS=x1,y1,x2,y2> */
s32 Html_AttrAreaCoords(u8 **cur, u8 *text) {
    u8 buf[0x101];
    s32 i;
    u16 n;
    u8 *p;
    u8 *p2;
    u16 n2;
    u8 *p3;
    u16 n3;
    u8 *p4;
    u16 n4;

    Html_ReadAttrValue(cur, (char *)buf, 0x100);
    i = 0;
    n = 0;
    for (p = buf; *p != 0 && *p != ','; p++, i++) {
        if (*p >= '0' && *p <= '9') {
            n += *p - '0';
            gHtml->area.coords[0] = n;
            n *= 10;
        }
    }
    if (buf[i] == 0) {
        return 0;
    }
    i++;
    n2 = 0;
    for (p2 = &buf[i]; *p2 != 0 && *p2 != ','; p2++, i++) {
        if (*p2 >= '0' && *p2 <= '9') {
            n2 += *p2 - '0';
            gHtml->area.coords[1] = n2;
            n2 *= 10;
        }
    }
    if (buf[i] == 0) {
        return 0;
    }
    i++;
    n3 = 0;
    for (p3 = &buf[i]; *p3 != 0 && *p3 != ','; p3++, i++) {
        if (*p3 >= '0' && *p3 <= '9') {
            n3 += *p3 - '0';
            gHtml->area.coords[2] = n3;
            n3 *= 10;
        }
    }
    if (buf[i] == 0) {
        return 0;
    }
    i++;
    n4 = 0;
    for (p4 = &buf[i]; *p4 != 0 && *p4 != ','; p4++, i++) {
        if (*p4 >= '0' && *p4 <= '9') {
            n4 += *p4 - '0';
            gHtml->area.coords[3] = n4;
            n4 *= 10;
        }
    }
    return 0;
}

/* <AREA HREF=...> */
s32 Html_AttrAreaHref(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->area.href, 0x100);
    return 0;
}

/* <AREA ALT=...> */
s32 Html_AttrAreaAlt(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->area.alt, 0x100);
    return 0;
}

/* <AREA ...>'s ">": a rectangle is added to the map. */
s32 Html_TagAreaEnd(u8 **cur, u8 *text) {
    s32 i;

    for (i = 0; i < 0x10; i++) {
        gHtml->area.shape[i] = gToUpper[(u8)gHtml->area.shape[i]];
    }
    if (strcmp(gHtml->area.shape, D_00839EF0) == 0) {
        Layout_MapArea();
    }
    return 0;
}

/* <BODY> */
s32 Html_TagBody(u8 **cur, u8 *text) {
    gHtml->textLen = 0;
    *text = 0;
    Html_TagBodyEnd(cur, text);
    return 0;
}

/* <BODY BGCOLOR=...> */
s32 Html_AttrBodyBgcolor(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->bgColor, 0x10);
    return 0;
}

/* <BODY BACKGROUND=...> */
s32 Html_AttrBodyBackground(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->background, 0x100);
    return 0;
}

/* <BODY TEXT=...> */
s32 Html_AttrBodyText(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->textColor, 0x10);
    return 0;
}

/* <BODY LINK=...> */
s32 Html_AttrBodyLink(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->linkColor, 0x10);
    return 0;
}
