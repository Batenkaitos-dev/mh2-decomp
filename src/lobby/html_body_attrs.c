/* The HTML tag handlers of <BODY ...>: the tag, its attributes and its
 * closing ">", which applies them. */
#include "common.h"
#include "html.h"

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
