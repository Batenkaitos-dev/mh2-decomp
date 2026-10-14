/* The page's draw commands: one small function per kind of element (text,
 * links, images, form fields, rules, table cells ...), each extending the
 * page to its corner (Page_Extend) and adding a command (Draw_Cmd, kind
 * and sub-kind first); then the page's mode, and the MMBB data's fields
 * (Mmbb_SetField). include/browser.h. */
#include "common.h"
#include "browser.h"

void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
u32 strlen(const void *s);
char *strcpy(char *dst, const char *src);
s32 strcmp(const void *a, const void *b);             /* main: strcmp */
s32 sprintf(char *dst, const char *fmt, ...);          /* main: sprintf */
void Page_Extend(s32 x, s32 y);
s32 Page_IsHidden(void);
void Page_Margins(u8 a0, s32 a1, s32 a2);
void Draw_Cmd(u8 kind, u8 sub, u16 x, u16 y, u16 x2, u16 y2, u32 color, u8 size, u8 field, u8 maxlen, u8 *s1, u8 *s2, u8 *s3);

/* The page's title, at the bottom, cut to 60 characters. */
void Draw_Title(u8 *text)
{
    if (strlen(text) > 0x3C) {
        text[0x38] = ' ';
        text[0x39] = '.';
        text[0x3A] = '.';
        text[0x3B] = '.';
        text[0x3C] = 0;
    }
    Draw_Cmd(0, 1, 0x10, 0x1AC, 0, 0, 0xFF000001, 0x14, 0, 0, (u8 *) text, (u8 *) (""), (u8 *) (""));
}

/* A run of text (nothing for a lone space). */
void Draw_Text(u16 x, u16 y, s32 color, u8 size, u16 x2, u16 y2, u8 *text, u8 a8)
{
    if (strcmp(text, " ") == 0 || strcmp(text, "\x81\x40") == 0) {
        return;
    }
    Page_Extend(x2, y2);
    Draw_Cmd(1, 1, x, y, x2, y2, color, size, a8, 0, (u8 *) text, (u8 *) (""), (u8 *) (""));
}

/* A link's text (href: where it goes). */
void Draw_LinkText(u16 x, u16 y, s32 color, u8 size, u16 x2, u16 y2, u8 *href, u8 *text, u8 a8)
{
    Page_Extend(x2, y2);
    Draw_Cmd(2, 1, x, y, x2, y2, color, size, a8, 0, (u8 *) text, (u8 *) href, (u8 *) (""));
}

/* An <INPUT TYPE=SUBMIT> (label). */
void Draw_Submit(u16 x, u16 y, u16 x2, u16 y2, u8 *src)
{
    Page_Extend(x2, y2);
    Draw_Cmd(3, 1, x, y, x2, y2, 0xFF000001, 0x10, 0, 0, (u8 *) src, (u8 *) (""), (u8 *) (""));
}

/* An <INPUT TYPE=RESET> (label). */
void Draw_Reset(u16 x, u16 y, u16 x2, u16 y2, u8 *src)
{
    Page_Extend(x2, y2);
    Draw_Cmd(4, 1, x, y, x2, y2, 0xFF000001, 0x10, 0, 0, (u8 *) src, (u8 *) (""), (u8 *) (""));
}

/* An <INPUT TYPE=IMAGE>. */
void Draw_InputImage(u16 x, u16 y, u16 x2, u16 y2, u8 *s1, u8 *s2, u8 *s3)
{
    if (x2 - x <= 0 || y2 - y <= 0) {
        return;
    }
    if (Page_IsHidden() == 0) {
        Page_Extend(x2, y2);
        Draw_Cmd(0xD, 5, x, y, x2, y2, 0xFF000001, 0x10, 0, 0, s1, s2, s3);
    }
}

/* A button of the page's frame (the page loader's). */
void Draw_PageButton(u16 x, u16 y, u16 x2, u16 y2, u8 *label)
{
    Page_Extend(x2, y2);
    Draw_Cmd(5, 1, x, y, x2, y2, 0xFF000001, 0x10, 0, 0, (u8 *) label, (u8 *) (""), (u8 *) (""));
}

/* An <INPUT TYPE=TEXT> (name, value, maxlength; the mode limits 1 to 3
 * to 16 characters). */
void Draw_TextInput(u16 x, u16 y, u16 x2, u16 y2, u8 *name, u8 *value, u16 maxlen, u8 mode)
{
    Page_Extend(x2, y2);
    if (mode > 0 && mode < 4 && maxlen > 0x10) {
        maxlen = 0x10;
    }
    if (maxlen < strlen(value)) {
        value = (u8 *)"";
    }
    if (maxlen > 0xFF) {
        maxlen = 0xFF;
    }
    Draw_Cmd(6, 1, x, y, x2, y2, 0xFF000001, 0x10, mode, maxlen, (u8 *) name, (u8 *) value, (u8 *) (""));
}

/* An <INPUT TYPE=PASSWORD>. */
void Draw_PasswordInput(u16 x, u16 y, u16 x2, u16 y2, u8 *name, u8 *value, u16 maxlen, u8 mode)
{
    Page_Extend(x2, y2);
    if (mode > 0 && mode < 4 && maxlen > 0x10) {
        maxlen = 0x10;
    }
    if (maxlen < strlen(value)) {
        value = (u8 *)"";
    }
    if (maxlen > 0xFF) {
        maxlen = 0xFF;
    }
    Draw_Cmd(7, 1, x, y, x2, y2, 0xFF000001, 0x10, mode, maxlen, (u8 *) name, (u8 *) value, (u8 *) (""));
}

/* A <SELECT NAME=...>. */
void Draw_SelectBegin(u8 *action)
{
    Draw_Cmd(8, 0xA, 0, 0, 0, 0, 0xFF000001, 0x10, 0, 0, (u8 *) (""), (u8 *) action, (u8 *) (""));
}

/* An <OPTION> of the <SELECT>. */
void Draw_Option(u8 *name, char *value, u8 a2)
{
    Draw_Cmd(8, 0x14, 0, 0, 0, 0, 0xFF000001, 0x10, 0, a2, (u8 *) name, (u8 *) value, (u8 *) (""));
}

/* The </SELECT>: the list's box. */
void Draw_SelectEnd(u16 x, u16 y, u16 x2, u16 y2)
{
    Page_Extend(x2, y2);
    Draw_Cmd(8, 0x1E, x, y, x2, y2, 0xFF000001, 0x10, 0, 0, (u8 *) (""), (u8 *) (""), (u8 *) (""));
}

/* A <HR>. */
void Draw_Rule(u16 x, u16 y, u16 x2, u16 y2)
{
    Draw_Cmd(0xA, 1, x, y, x2, y2, 0xFF000001, 0, 0, 0, (u8 *) (""), (u8 *) (""), (u8 *) (""));
}

/* An <INPUT TYPE=RADIO> (20 by 20). */
void Draw_Radio(u16 x, u16 y, u8 checked, u8 *name, u8 *value, u8 a5)
{
    Page_Extend(x + 0x14, y + 0x14);
    Draw_Cmd(0xB, 1, x, y, x + 0x14, y + 0x14, 0xFF000001, 0, a5, checked, (u8 *) name, (u8 *) value, (u8 *) (""));
}

/* An <INPUT TYPE=CHECKBOX> (20 by 20). */
void Draw_Checkbox(u16 x, u16 y, u8 checked, u8 *name, u8 *value, u8 a5)
{
    Page_Extend(x + 0x14, y + 0x14);
    Draw_Cmd(0xC, 1, x, y, x + 0x14, y + 0x14, 0xFF000001, 0, a5, checked, (u8 *) name, (u8 *) value, (u8 *) (""));
}

/* The <BODY BACKGROUND=...> image (none: dlgBackState = 3). */
void Draw_BodyBackground(u8 *name)
{
    if (Page_IsHidden() == 0) {
        if (*name == 0) {
            gBrowserWork->dlgBackState = 3;
            return;
        }
        gBrowserWork->dlgBackState = 1;
        Draw_Cmd(0xD, 1, 0, 0, 0, 0, 0xFF000001, 0x10, 0, 0, (u8 *) (""), (u8 *) name, (u8 *) (""));
    }
}

/* An <IMG> (src, alt). */
void Draw_Image(u16 x, u16 y, u16 x2, u16 y2, u8 *value, u8 *text)
{
    if (x2 - x <= 0 || y2 - y <= 0) {
        return;
    }
    if (Page_IsHidden() == 0) {
        Page_Extend(x2, y2);
        Draw_Cmd(0xD, 2, x, y, x2, y2, 0xFF000001, 0x10, 0, 0, (u8 *) (""), (u8 *) value, (u8 *) text);
    }
}

/* An <IMG USEMAP=...>. */
void Draw_ImageMap(u16 x, u16 y, u16 x2, u16 y2, u8 *value, u8 *text, u8 *s3, u8 a7)
{
    if (x2 - x <= 0 || y2 - y <= 0) {
        return;
    }
    if (Page_IsHidden() == 0) {
        Page_Extend(x2, y2);
        Draw_Cmd(0xD, 3, x, y, x2, y2, 0xFF000001, 0x10, a7, 0, text, value, s3);
    }
}

/* "#%s", with a "
" after it in the same 8 bytes (the game's data) */
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_72AD50", D_00838EE8);
extern char D_00838EE8[];

/* An <A NAME=...> anchor, as "#name". */
void Draw_Anchor(u16 x, u16 y, u8 *name)
{
    char buf[0x100];

    memset(buf, 0, 0x100);
    sprintf(buf, D_00838EE8, name);
    Draw_Cmd(0xE, 1, x, y, 0, 0, 0xFF000001, 0, 0, 0, (u8 *) buf, (u8 *) (""), (u8 *) (""));
}

/* A table cell: its background image, its colour and its border. */
void Draw_Cell(u16 x, u16 y, u16 x2, u16 y2, u8 size, s32 color, u8 border, s32 bcolor, u8 *background)
{
    Page_Extend(x2, y2);
    if (*background != 0) {
        Draw_Cmd(0xD, 4, x, y, x2, y2, 0xFF000001, size, 0, 0, (u8 *) (""), (u8 *) background, (u8 *) (""));
    }
    Draw_Cmd(0x10, 1, x + border, y + border, x2 - border, y2 - border, color, size, border, 2, (u8 *) (""), (u8 *) (""),
                  (u8 *) (""));
    Draw_Cmd(0x10, 1, x, y, x2, y2, bcolor, size, border, 1, (u8 *) (""), (u8 *) (""), (u8 *) (""));
}

/* A table: its background image and its border. */
void Draw_Table(u16 x, u16 y, u16 x2, u16 y2, u8 size, u32 color, u8 border, u8 *background)
{
    Page_Extend(x2, y2);
    if (*background != 0) {
        Draw_Cmd(0xD, 4, x, y, x2, y2, 0xFF000001, size, 0, 0, (u8 *) (""), (u8 *) background, (u8 *) (""));
    }
    Draw_Cmd(0xF, 1, x, y, x2, y2, color, size, border, 0, (u8 *) (""), (u8 *) (""), (u8 *) (""));
}

/* Text of kind 0x11 (from the text output, Layout_DrawRun). */
void Draw_Text11(u16 x, u16 y, s32 color, u8 size, u16 x2, u16 y2, u8 *s1, u8 *s2, u8 a8)
{
    Page_Extend(x2, y2);
    Draw_Cmd(0x11, 1, x, y, x2, y2, color, size, a8, 0, (u8 *) s2, (u8 *) s1, (u8 *) (""));
}

/* Text of kind 0x12 (from the text output, Layout_DrawRun). */
void Draw_Text12(u16 x, u16 y, s32 color, u8 size, u16 x2, u16 y2, u8 *s1, u8 *s2, u8 a8)
{
    Page_Extend(x2, y2);
    Draw_Cmd(0x12, 1, x, y, x2, y2, color, size, a8, 0, (u8 *) s2, (u8 *) s1, (u8 *) (""));
}

/* A <FORM> (action, method). */
void Draw_FormBegin(u8 *s1, u8 a1)
{
    Draw_Cmd(0x13, 1, 0, 0, 0, 0, 0xFF000001, 0x10, 0, a1, (u8 *) s1, (u8 *) (""), (u8 *) (""));
}

/* An <INPUT TYPE=HIDDEN> (name, value). */
void Draw_Hidden(u8 *s1, u8 *s2)
{
    Draw_Cmd(0x14, 1, 0, 0, 0, 0, 0xFF000001, 0x10, 0, 0, (u8 *) s1, (u8 *) s2, (u8 *) (""));
}

void Page_SetDataMode(u8 mode)
{
    s32 temp_a1;

    temp_a1 = mode & 0xFF;
    switch (temp_a1) {
    case 1:
    case 4:
    case 5:
        gBrowserState->plain = 1;
        break;
    default:
    case 6:
    case 7:
        gBrowserState->plain = 0;
        break;
    }
    gBrowserState->prevPageMode = gBrowserState->pageMode;
    gBrowserState->pageMode = mode;
}

/* Field which (entry idx, 0..39, of the list fields) of the MMBB data :=
 * value, cut to the field's size (a value too long for a 0x100 field is
 * dropped). */
void Mmbb_SetField(s8 which, u8 idx, char *value)
{
    u8 len = strlen(value);

    if (idx > 39) {
        idx = 39;
    }
    switch (which) {
    case 1:
        if (len > 14) {
            len = 14;
        }
        memcpy(gMmbbData->userId, value, len + 1);
        break;
    case 2:
        if (len > 10) {
            len = 10;
        }
        memcpy(gMmbbData->connectionId, value, len + 1);
        break;
    case 4:
        if (len > 0x100) {
            len = 0;
        }
        memcpy(gMmbbData->f4, value, len + 1);
        break;
    case 5:
        if (len > 0x100) {
            len = 0;
        }
        memcpy(gMmbbData->nextUrl, value, len + 1);
        break;
    case 6:
        if (len > 0x10) {
            len = 0x10;
        }
        memcpy(gMmbbData->f6, value, len + 1);
        break;
    case 7:
        if (len > 0x61) {
            len = 0x61;
        }
        memcpy(gMmbbData->f7[idx], value, len + 1);
        break;
    case 8:
        if (len > 0x100) {
            len = 0;
        }
        memcpy(gMmbbData->f8[idx], value, len + 1);
        break;
    case 9:
        if (len > 0x100) {
            len = 0;
        }
        memcpy(gMmbbData->f9[idx], value, len + 1);
        break;
    case 11:
        if (len > 0x100) {
            len = 0;
        }
        memcpy(gMmbbData->f11, value, len + 1);
        break;
    case 12:
        if (len > 0x20) {
            len = 0x20;
        }
        memcpy(gMmbbData->f12[idx], value, len + 1);
        break;
    case 13:
        if (len > 0x20) {
            len = 0x20;
        }
        memcpy(gMmbbData->f13[idx], value, len + 1);
        break;
    case 14:
        if (len > 14) {
            len = 14;
        }
        memcpy(gMmbbData->f14, value, len + 1);
        break;
    case 15:
        if (len > 0x100) {
            len = 0;
        }
        memcpy(gMmbbData->f15, value, len + 1);
        break;
    case 16:
        if (len > 2) {
            len = 2;
        }
        memcpy(gMmbbData->f16[idx], value, len + 1);
        break;
    case 17:
        if (len > 4) {
            len = 4;
        }
        memcpy(gMmbbData->f17[idx], value, len + 1);
        break;
    case 18:
        if (len > 0x100) {
            len = 0;
        }
        memcpy(gMmbbData->f18[idx], value, len + 1);
        break;
    case 0:
    case 3:
    case 10:
    case 19:
        break;
    }
}

/* The page's style flags, its message and its mode. */
void Page_SetStyle(u16 flags, char *msg, u8 mode, s32 a3)
{
    gBrowserState->style = flags;
    if (gBrowserState->forceStyle8 != 0) {
        if (((gBrowserState->style & 8) >> 3) == 0) {
            gBrowserState->style += 8;
        }
    }
    strcpy(gBrowserState->msg, msg);
    gBrowserState->styleMode = mode;
    gBrowserState->styleWait = 0x1E;
    gBrowserState->styled = 1;
    Page_Margins(mode, a3, 0x1E);
}

void Page_ExtendU16(u16 x, u16 y)
{
    Page_Extend(x, y);
}
