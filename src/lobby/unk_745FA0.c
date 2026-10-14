/* The HTML tag handlers: <FORM ...> and <INPUT ...>. */
#include "common.h"
#include "html.h"
s32 strncmp(const char *, const char *, s32);

void *memset(void *dst, s32 c, u32 n);
s32 strncmp(const char *a, const char *b, s32 n);

void Html_DecodeEntities(char *s);
void Html_ReadAttrValue(u8 **cur, char *buf, s32 size);
void Html_ReadAttrKeyword(u8 **cur, char *buf, s32 size);
s32 Html_ParseNumber(char *s);
u8 Html_LookupKeyword(char *s, HtmlKeyword *table, u32 size);
void Layout_Flush(u8 *text);
void Layout_FlushBlock(u8 *text);
void Html_ClearInputAttrs(void);
void Draw_FormBegin(u8 *s1, u8 a1);

extern HtmlKeyword gInputTypeKeywords[]; /* INPUT's TYPE */
extern char D_00839EF8[]; /* "POST" */

/* <FORM ...> */
s32 Html_TagForm(u8 **cur, u8 *text) {
    Layout_FlushBlock(text);
    memset(&gHtml->form, 0, sizeof(HtmlForm));
    return 0;
}

/* <FORM ACTION=...> */
s32 Html_AttrFormAction(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->form.action, 0x100);
    return 0;
}

/* <FORM METHOD=...> */
s32 Html_AttrFormMethod(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrKeyword(cur, buf, 0x100);
    if (strncmp(buf, D_00839EF8, 4) == 0) {
        gHtml->form.post = 1;
    }
    return 0;
}

/* <FORM ...>'s ">" */
s32 Html_TagFormEnd(u8 **cur, u8 *text) {
    if (gHtml->measure == 0 && gHtml->inFrame == 0) {
        Draw_FormBegin((u8 *) gHtml->form.action, gHtml->form.post);
    }
    return 0;
}

/* <INPUT ...> */
s32 Html_TagInput(u8 **cur, u8 *text) {
    Layout_Flush(text);
    Html_ClearInputAttrs();
    return 0;
}

/* <INPUT TYPE=...> */
s32 Html_AttrInputType(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    gHtml->input.type = Html_LookupKeyword(buf, gInputTypeKeywords, 0x48);
    return 0;
}

/* <INPUT NAME=...> */
s32 Html_AttrInputName(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->input.name, 0x100);
    return 0;
}

/* <INPUT VALUE=...> */
s32 Html_AttrInputValue(u8 **cur, u8 *text) {
    Html_ReadAttrValue(cur, gHtml->input.value, 0x100);
    Html_DecodeEntities(gHtml->input.value);
    return 0;
}

/* <INPUT SIZE=...> */
s32 Html_AttrInputSize(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    gHtml->input.size = Html_ParseNumber(buf);
    if (gHtml->input.size == 0) {
        gHtml->input.size = 16;
    }
    return 0;
}

/* <INPUT MAXLENGTH=...> */
s32 Html_AttrInputMaxlength(u8 **cur, u8 *text) {
    char buf[0x101];

    Html_ReadAttrValue(cur, buf, 0x100);
    gHtml->input.maxLength = Html_ParseNumber(buf);
    if (gHtml->input.maxLength) {
        if (gHtml->input.maxLength > 0x100) {
            gHtml->input.maxLength = 0x100;
        }
    } else {
        gHtml->input.maxLength = 0x100;
    }
    return 0;
}

/* unk_745FA0's .data (0x7EFE70-0x7EFEC0) */
extern u32 D_00839D98[];
extern char D_00839E10[];
extern char D_00839E20[];
extern char D_00839E28[];
extern char D_00839E30[];
extern char D_00839E38[];
extern char D_00839E48[];
extern char D_00839E50[];
extern char D_00839E58[];
HtmlKeyword gInputTypeKeywords[10] = {
#include "lobby/007EFE70.inc"
};

/* unk_745FA0's .rodata (0x839E10-0x839E68) */
#pragma rodata on
char D_00839E10[0x10] = {
#include "lobby/00839E10.inc"
};
char D_00839E20[0x8] = {
#include "lobby/00839E20.inc"
};
char D_00839E28[0x8] = {
#include "lobby/00839E28.inc"
};
char D_00839E30[0x8] = {
#include "lobby/00839E30.inc"
};
char D_00839E38[0xC] = {
#include "lobby/00839E38.inc"
};
char D_00839E48[0x8] = {
#include "lobby/00839E48.inc"
};
char D_00839E50[0x8] = {
#include "lobby/00839E50.inc"
};
char D_00839E58[0x8] = {
#include "lobby/00839E58.inc"
};
u32 gHtmlAlignKeywords_0[2] = {
#include "lobby/00839E60.inc"
};
#pragma rodata off
