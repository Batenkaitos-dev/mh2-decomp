/* The HTML tag handlers: <FORM ...> and <INPUT ...>, with their attributes. */
#include "common.h"
#include "html.h"
#include "browser.h"
#include "libc.h"

extern HtmlKeyword gInputTypeKeywords[]; /* INPUT's TYPE */
extern char gHtmlMethodPost[]; /* "POST" */

/* <FORM ...> */
s32 Html_TagForm(u8 **cur, u8 *text)
{
    Layout_FlushBlock(text);
    memset(&gHtml->form, 0, sizeof(HtmlForm));
    return 0;
}

/* <FORM ACTION=...> */
s32 Html_AttrFormAction(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->form.action, HTML_ATTR_MAX);
    return 0;
}

/* <FORM METHOD=...> */
s32 Html_AttrFormMethod(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrKeyword(cur, buf, HTML_ATTR_MAX);
    if (strncmp(buf, gHtmlMethodPost, 4) == 0) {
        gHtml->form.post = 1;
    }
    return 0;
}

/* <FORM ...>'s ">" */
s32 Html_TagFormEnd(u8 **cur, u8 *text)
{
    if (gHtml->measure == HTML_PASS_DRAW && gHtml->inFrame == 0) {
        Draw_FormBegin((u8 *) gHtml->form.action, gHtml->form.post);
    }
    return 0;
}

/* <INPUT ...> */
s32 Html_TagInput(u8 **cur, u8 *text)
{
    Layout_Flush(text);
    Html_ClearInputAttrs();
    return 0;
}

/* <INPUT TYPE=...> */
s32 Html_AttrInputType(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    gHtml->input.type = Html_LookupKeyword(buf, gInputTypeKeywords, 72); /* the first nine of its ten keywords */
    return 0;
}

/* <INPUT NAME=...> */
s32 Html_AttrInputName(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->input.name, HTML_ATTR_MAX);
    return 0;
}

/* <INPUT VALUE=...> */
s32 Html_AttrInputValue(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->input.value, HTML_ATTR_MAX);
    Html_DecodeEntities(gHtml->input.value);
    return 0;
}

/* <INPUT SIZE=...> */
s32 Html_AttrInputSize(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    gHtml->input.size = Html_ParseNumber(buf);
    if (gHtml->input.size == 0) {
        gHtml->input.size = 16;    /* the default width, in characters */
    }
    return 0;
}

/* <INPUT MAXLENGTH=...> */
s32 Html_AttrInputMaxlength(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    gHtml->input.maxLength = Html_ParseNumber(buf);
    if (gHtml->input.maxLength) {
        if (gHtml->input.maxLength > HTML_ATTR_MAX) {
            gHtml->input.maxLength = HTML_ATTR_MAX;
        }
    } else {
        gHtml->input.maxLength = HTML_ATTR_MAX;
    }
    return 0;
}

/* <INPUT CHECKED> */
s32 Html_AttrInputChecked(u8 **cur, u8 *text)
{
    gHtml->input.checked = 1;
    return 0;
}

/* <INPUT ...>'s ">" */
s32 Html_TagInputEnd(u8 **cur, u8 *text)
{
    Layout_InputEnd();
    return 0;
}

/* <INPUT SRC=...> */
s32 Html_AttrInputSrc(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->input.src, HTML_ATTR_MAX);
    return 0;
}

/* <INPUT WIDTH=...> */
s32 Html_AttrInputWidth(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    gHtml->input.width = Html_ParseNumber(buf);
    return 0;
}

/* <INPUT HEIGHT=...> */
s32 Html_AttrInputHeight(u8 **cur, u8 *text)
{
    char buf[HTML_ATTR_MAX + 1];

    Html_ReadAttrValue(cur, buf, HTML_ATTR_MAX);
    gHtml->input.height = Html_ParseNumber(buf);
    return 0;
}

/* <INPUT ALT=...> */
s32 Html_AttrInputAlt(u8 **cur, u8 *text)
{
    Html_ReadAttrValue(cur, gHtml->input.alt, HTML_ATTR_MAX);
    Html_DecodeEntities(gHtml->input.alt);
    return 0;
}

/* The keyword tables (data), then the strings they point at. */
extern u32 D_00839D98[];
extern char gInputTypeKeywords_1[];
extern char gInputTypeKeywords_2[];
extern char gInputTypeKeywords_3[];
extern char gInputTypeKeywords_4[];
extern char gInputTypeKeywords_5[];
extern char gInputTypeKeywords_6[];
extern char gInputTypeKeywords_7[];
extern char gInputTypeKeywords_8[];
HtmlKeyword gInputTypeKeywords[10] = {
#include "lobby/007EFE70.inc"
};

/* The keywords' strings (read-only data). */
#pragma rodata on
char gInputTypeKeywords_1[16] = {
#include "lobby/00839E10.inc"
};
char gInputTypeKeywords_2[8] = {
#include "lobby/00839E20.inc"
};
char gInputTypeKeywords_3[8] = {
#include "lobby/00839E28.inc"
};
char gInputTypeKeywords_4[8] = {
#include "lobby/00839E30.inc"
};
char gInputTypeKeywords_5[12] = {
#include "lobby/00839E38.inc"
};
char gInputTypeKeywords_6[8] = {
#include "lobby/00839E48.inc"
};
char gInputTypeKeywords_7[8] = {
#include "lobby/00839E50.inc"
};
char gInputTypeKeywords_8[8] = {
#include "lobby/00839E58.inc"
};
char gHtmlAlignKeywords_0[8] = {
#include "lobby/00839E60.inc"
};
#pragma rodata off
