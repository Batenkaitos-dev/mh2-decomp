/* The HTML tag handlers: <OPTION>, <A>, <HR>, <FONT> and <SELECT>, with
 * their attributes. */
#include "common.h"
#include "html.h"

void *memset(void *dst, s32 c, u32 n);
char *strcpy(char *dst, const char *src);
s32 strlen(const u8 *s);

extern char gHtmlFontSizeDefault[]; /* "3" */

/* Closes an open <OPTION>: its text becomes an entry of the <SELECT>. */
void Html_CloseOption(u8 *text)
{
    if (gHtml->select.inOption != 0) {
        Layout_Option(text);
        gHtml->select.inOption = 0;
    }
}

/* <OPTION> */
s32 Html_TagOption(u8 **cur, u8 *text)
{
    Html_CloseOption(text);
    gHtml->select.inOption = 1;
    memset(&gHtml->option, 0, sizeof(HtmlOption));
    gHtml->textLen = 0;
    *text = 0;
    return 0;
}

/* <OPTION ...> */
s32 Html_TagOptionAttrs(u8 **cur, u8 *text)
{
    Html_CloseOption(text);
    gHtml->select.inOption = 1;
    memset(&gHtml->option, 0, sizeof(HtmlOption));
    return 0;
}

/* <OPTION VALUE=...> */
s32 Html_AttrOptionValue(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->option.value, HTML_ATTR_MAX);
    return 0;
}

/* <OPTION SELECTED> */
s32 Html_AttrOptionSelected(u8 **cur, u8 *text)
{
    gHtml->option.selected = 1;
    return 0;
}

/* </OPTION> */
s32 Html_TagEndOption(u8 **cur, u8 *text)
{
    Layout_Option(text);
    gHtml->select.inOption = 0;
    return 0;
}

/* <A ...> */
s32 Html_TagA(u8 **cur, u8 *text)
{
    gHtml->link = 0;
    Layout_Flush(text);
    return 0;
}

/* <A HREF=...> */
s32 Html_AttrAHref(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    if ((u8)buf[0] == '#') {
        gHtml->link = 1;
        strcpy(gHtml->anchor, buf);
    } else {
        gHtml->link = 2;
        strcpy(gHtml->href, buf);
    }
    return 0;
}

/* <A NAME=...> */
s32 Html_AttrAName(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    gHtml->link = 3;
    Layout_Anchor(buf);
    return 0;
}

/* <A ...>'s ">" */
s32 Html_TagAEnd(u8 **cur, u8 *text)
{
    return 0;
}

/* </A>: the link's text loses its trailing spaces. */
s32 Html_TagEndA(u8 **cur, u8 *text)
{
    s32 n;

    n = strlen(text);
    if (n != 0 && text[n - 1] == ' ') {
        do {
            text[n - 1] = 0;
            n--;
        } while (n != 0 && text[n - 1] == ' ');
    }
    Layout_Flush(text);
    gHtml->link = 0;
    return 0;
}

/* <HR> */
s32 Html_TagHr(u8 **cur, u8 *text)
{
    Layout_FlushBlock(text);
    gHtml->hrSize = 4;
    gHtml->hrWidth = 0;
    Layout_Rule(0);
    return 0;
}

/* <HR ...> */
s32 Html_TagHrAttrs(u8 **cur, u8 *text)
{
    Layout_FlushBlock(text);
    gHtml->hrSize = 4;
    gHtml->hrWidth = 0;
    return 0;
}

/* <HR ...>'s ">" */
s32 Html_TagHrEnd(u8 **cur, u8 *text)
{
    Layout_FlushBlock(text);
    if (gHtml->hrWidth != 0) {
        Layout_Rule(1);
    } else {
        Layout_Rule(0);
    }
    return 0;
}

/* <HR SIZE=...> */
s32 Html_AttrHrSize(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    gHtml->hrSize = Html_ParseNumber(buf);
    if (gHtml->hrSize < 4) {    /* a rule is at least 4 dots thick */
        gHtml->hrSize = 4;
    }
    return 0;
}

/* <HR WIDTH=...> */
s32 Html_AttrHrWidth(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    gHtml->hrWidth = Html_ParseLength(buf, 0);
    if (gHtml->hrWidth < 2) {
        gHtml->hrWidth = 2;
    }
    return 0;
}

/* <FONT> */
s32 Html_TagFont(u8 **cur, u8 *text)
{
    Html_ClearFontAttrs();
    strcpy(gHtml->fontAttr.size, gHtmlFontSizeDefault);
    Html_PushFont();
    return 0;
}

/* <FONT ...> */
s32 Html_TagFontAttrs(u8 **cur, u8 *text)
{
    Html_ClearFontAttrs();
    Layout_Flush(text);
    return 0;
}

/* <FONT COLOR=...> */
s32 Html_AttrFontColor(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->fontAttr.color, sizeof(gHtml->fontAttr.color));
    return 0;
}

/* <FONT SIZE=...> */
s32 Html_AttrFontSize(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->fontAttr.size, sizeof(gHtml->fontAttr.size));
    return 0;
}

/* <FONT ...>'s ">" */
s32 Html_TagFontEnd(u8 **cur, u8 *text)
{
    Html_PushFont();
    return 0;
}

/* </FONT>, </SMALL>, </BIG> */
s32 Html_TagEndFont(u8 **cur, u8 *text)
{
    Layout_Flush(text);
    Html_PopFont();
    return 0;
}

/* <SELECT> */
s32 Html_TagSelect(u8 **cur, u8 *text)
{
    Layout_Flush(text);
    memset(&gHtml->select, 0, sizeof(HtmlSelect));
    Layout_SelectBegin();
    return 0;
}

/* <SELECT ...> */
s32 Html_TagSelectAttrs(u8 **cur, u8 *text)
{
    Layout_Flush(text);
    memset(&gHtml->select, 0, sizeof(HtmlSelect));
    return 0;
}

/* <SELECT NAME=...> */
s32 Html_AttrSelectName(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->select.name, HTML_ATTR_MAX);
    return 0;
}

/* <SELECT ...>'s ">" */
s32 Html_TagSelectEnd(u8 **cur, u8 *text)
{
    Layout_SelectBegin();
    return 0;
}

/* </SELECT> */
s32 Html_TagEndSelect(u8 **cur, u8 *text)
{
    Html_CloseOption(text);
    Layout_SelectEnd();
    gHtml->textLen = 0;
    *text = 0;
    return 0;
}

