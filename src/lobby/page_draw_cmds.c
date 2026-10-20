/*
 * The page settings and draw commands the HTML parser calls as it reads a
 * page. First the settings that reach the browser itself: a new page, the
 * page's refresh (<META HTTP-EQUIV=REFRESH>), its background colour and
 * its base URL (<BASE HREF>), kept in the page's state (gBrowserState) and
 * the browser's work area (gBrowserWork).
 *
 * Then the page's draw commands. The HTML layout calls one small function per
 * element it places (text, links, form fields, images, rules, tables and
 * cells ...); each extends the page to the element's bottom right corner
 * (Page_Extend) and adds a page object (Draw_Cmd: its kind, enum
 * PageObjKind, and sub-kind first), which the page draws then show.
 * After them: the page's data mode, the MMBB data's fields that a page
 * hands the game (Mmbb_SetField), the page's style (Page_SetStyle), and
 * the login id typed in the page's form (Form_SetLoginId).
 */
#include "common.h"
#include "browser.h"
#include "html.h"

/* The longest title shown whole. */
#define PAGE_TITLE_MAX 60

void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
u32 strlen(const void *s);
char *strcpy(char *dst, const char *src);
s32 strcmp(const void *a, const void *b);
s32 sprintf(char *dst, const char *fmt, ...);
void Page_Extend(s32 x, s32 y);
s32 Page_IsDataPage(void);
void Page_SetMargins(u8 a0, s32 a1, s32 a2);
void Page_New(void);
void Url_SetBase(char *url);
void Draw_Cmd(u8 kind, u8 sub, u16 x, u16 y, u16 right, u16 bottom, u32 color, u8 size, u8 field, u8 maxlen, u8 *s1, u8 *s2, u8 *s3);

/* Starts a new page (Page_New). */
void Html_NewPage(void)
{
    Page_New();
}

/*
 * A refresh tag: the browser goes to url after the given number of
 * seconds, counted in frames (60 a second; 0 seconds waits one second).
 */
void Html_Refresh(s32 sec, char *url)
{
    memset(gBrowserState->url, 0, sizeof(gBrowserState->url));
    strcpy(gBrowserState->url, url);
    if (sec == 0) {
        gBrowserWork->refreshTimer = 60;
    } else {
        gBrowserWork->refreshTimer = sec * 60;
    }
}

/*
 * Sets the page's background colour from the body's colour (rgb, made
 * opaque), or black when the page is shown plain.
 */
void Html_SetBgColor(u32 rgb)
{
    if (gBrowserState->plain != 0) {
        gBrowserState->bgColor = BROWSER_BLACK;
        return;
    }
    gBrowserState->bgColor = rgb | 0xFF000000;
}

/* A base tag: keeps url as the page's base URL and resolves links against it. */
void Html_SetBase(char *url)
{
    memset(gBrowserState->baseUrl, 0, sizeof(gBrowserState->baseUrl));
    strcpy(gBrowserState->baseUrl, url);
    Url_SetBase(gBrowserState->baseUrl);
}

/* The page's title, at the bottom left, cut to PAGE_TITLE_MAX characters
 * (its end replaced by " ..."). */
void Draw_Title(u8 *text)
{
    if (strlen(text) > PAGE_TITLE_MAX) {
        text[PAGE_TITLE_MAX - 4] = ' ';
        text[PAGE_TITLE_MAX - 3] = '.';
        text[PAGE_TITLE_MAX - 2] = '.';
        text[PAGE_TITLE_MAX - 1] = '.';
        text[PAGE_TITLE_MAX] = 0;
    }
    Draw_Cmd(PAGE_OBJ_TITLE, 1, 0x10, 0x1AC, 0, 0, BROWSER_BLACK, 0x14, 0, 0, (u8 *) text, (u8 *) (""), (u8 *) (""));
}

/* A run of text in the given colour and size (nothing for a lone space,
 * single- or double-byte). */
void Draw_Text(u16 x, u16 y, s32 color, u8 size, u16 right, u16 bottom, u8 *text, u8 bold)
{
    if (strcmp(text, " ") == 0 || strcmp(text, "\x81\x40") == 0) {
        return;
    }
    Page_Extend(right, bottom);
    Draw_Cmd(PAGE_OBJ_TEXT, 1, x, y, right, bottom, color, size, bold, 0, (u8 *) text, (u8 *) (""), (u8 *) (""));
}

/* A link: its text, and href, where it goes. */
void Draw_LinkText(u16 x, u16 y, s32 color, u8 size, u16 right, u16 bottom, u8 *href, u8 *text, u8 bold)
{
    Page_Extend(right, bottom);
    Draw_Cmd(PAGE_OBJ_LINK, 1, x, y, right, bottom, color, size, bold, 0, (u8 *) text, (u8 *) href, (u8 *) (""));
}

/* A form's submit button, with its label. */
void Draw_Submit(u16 x, u16 y, u16 right, u16 bottom, u8 *src)
{
    Page_Extend(right, bottom);
    Draw_Cmd(PAGE_OBJ_SUBMIT, 1, x, y, right, bottom, BROWSER_BLACK, 0x10, 0, 0, (u8 *) src, (u8 *) (""), (u8 *) (""));
}

/* A form's reset button, with its label. */
void Draw_Reset(u16 x, u16 y, u16 right, u16 bottom, u8 *src)
{
    Page_Extend(right, bottom);
    Draw_Cmd(PAGE_OBJ_RESET, 1, x, y, right, bottom, BROWSER_BLACK, 0x10, 0, 0, (u8 *) src, (u8 *) (""), (u8 *) (""));
}

/* A form's image button (name, image src, alt text); nothing for an
 * empty box or a hidden page. */
void Draw_InputImage(u16 x, u16 y, u16 right, u16 bottom, u8 *name, u8 *src, u8 *alt)
{
    if (right - x <= 0 || bottom - y <= 0) {
        return;
    }
    if (Page_IsDataPage() == 0) {
        Page_Extend(right, bottom);
        Draw_Cmd(PAGE_OBJ_IMAGE, PAGE_IMG_INPUT, x, y, right, bottom, BROWSER_BLACK, 0x10, 0, 0, name, src, alt);
    }
}

/* A page button, with its label. */
void Draw_PageButton(u16 x, u16 y, u16 right, u16 bottom, u8 *label)
{
    Page_Extend(right, bottom);
    Draw_Cmd(PAGE_OBJ_BUTTON, 1, x, y, right, bottom, BROWSER_BLACK, 0x10, 0, 0, (u8 *) label, (u8 *) (""), (u8 *) (""));
}

/*
 * A text box: name, value and maxlen. key is its access key code, which
 * also marks the login fields (1 to 3: at most 16 characters); a value
 * longer than maxlen is dropped, and maxlen is at most 0xFF.
 */
void Draw_TextInput(u16 x, u16 y, u16 right, u16 bottom, u8 *name, u8 *value, u16 maxlen, u8 key)
{
    Page_Extend(right, bottom);
    if (key > 0 && key < 4 && maxlen > 0x10) {
        maxlen = 0x10;
    }
    if (maxlen < strlen(value)) {
        value = (u8 *)"";
    }
    if (maxlen > 0xFF) {
        maxlen = 0xFF;
    }
    Draw_Cmd(PAGE_OBJ_TEXT_INPUT, 1, x, y, right, bottom, BROWSER_BLACK, 0x10, key, maxlen, (u8 *) name, (u8 *) value, (u8 *) (""));
}

/* A password box, as the text box above. */
void Draw_PasswordInput(u16 x, u16 y, u16 right, u16 bottom, u8 *name, u8 *value, u16 maxlen, u8 key)
{
    Page_Extend(right, bottom);
    if (key > 0 && key < 4 && maxlen > 0x10) {
        maxlen = 0x10;
    }
    if (maxlen < strlen(value)) {
        value = (u8 *)"";
    }
    if (maxlen > 0xFF) {
        maxlen = 0xFF;
    }
    Draw_Cmd(PAGE_OBJ_PASSWORD, 1, x, y, right, bottom, BROWSER_BLACK, 0x10, key, maxlen, (u8 *) name, (u8 *) value, (u8 *) (""));
}

/* A drop-down list's start (its name). */
void Draw_SelectBegin(u8 *action)
{
    Draw_Cmd(PAGE_OBJ_SELECT, PAGE_SELECT_BEGIN, 0, 0, 0, 0, BROWSER_BLACK, 0x10, 0, 0, (u8 *) (""), (u8 *) action, (u8 *) (""));
}

/* One option of the list: its text, its value and whether it is selected. */
void Draw_Option(u8 *name, char *value, u8 selected)
{
    Draw_Cmd(PAGE_OBJ_SELECT, PAGE_SELECT_OPTION, 0, 0, 0, 0, BROWSER_BLACK, 0x10, 0, selected, (u8 *) name, (u8 *) value, (u8 *) (""));
}

/* The list's end: its box on the page. */
void Draw_SelectEnd(u16 x, u16 y, u16 right, u16 bottom)
{
    Page_Extend(right, bottom);
    Draw_Cmd(PAGE_OBJ_SELECT, PAGE_SELECT_END, x, y, right, bottom, BROWSER_BLACK, 0x10, 0, 0, (u8 *) (""), (u8 *) (""), (u8 *) (""));
}

/* A horizontal rule. */
void Draw_Rule(u16 x, u16 y, u16 right, u16 bottom)
{
    Draw_Cmd(PAGE_OBJ_RULE, 1, x, y, right, bottom, BROWSER_BLACK, 0, 0, 0, (u8 *) (""), (u8 *) (""), (u8 *) (""));
}

/* A radio button, 20 by 20 pixels: checked, name, value, access key. */
void Draw_Radio(u16 x, u16 y, u8 checked, u8 *name, u8 *value, u8 key)
{
    Page_Extend(x + 0x14, y + 0x14);
    Draw_Cmd(PAGE_OBJ_RADIO, 1, x, y, x + 0x14, y + 0x14, BROWSER_BLACK, 0, key, checked, (u8 *) name, (u8 *) value, (u8 *) (""));
}

/* A check box, 20 by 20 pixels: checked, name, value, access key. */
void Draw_Checkbox(u16 x, u16 y, u8 checked, u8 *name, u8 *value, u8 key)
{
    Page_Extend(x + 0x14, y + 0x14);
    Draw_Cmd(PAGE_OBJ_CHECKBOX, 1, x, y, x + 0x14, y + 0x14, BROWSER_BLACK, 0, key, checked, (u8 *) name, (u8 *) value, (u8 *) (""));
}

/* The page's background image (none: dlgBackImgState 3, else 1). */
void Draw_BodyBackground(u8 *name)
{
    if (Page_IsDataPage() == 0) {
        if (*name == 0) {
            gBrowserWork->dlgBackImgState = 3;
            return;
        }
        gBrowserWork->dlgBackImgState = 1;
        Draw_Cmd(PAGE_OBJ_IMAGE, PAGE_IMG_BODY_BACK, 0, 0, 0, 0, BROWSER_BLACK, 0x10, 0, 0, (u8 *) (""), (u8 *) name, (u8 *) (""));
    }
}

/* An image (src, alt text); nothing for an empty box or a hidden page. */
void Draw_Image(u16 x, u16 y, u16 right, u16 bottom, u8 *src, u8 *alt)
{
    if (right - x <= 0 || bottom - y <= 0) {
        return;
    }
    if (Page_IsDataPage() == 0) {
        Page_Extend(right, bottom);
        Draw_Cmd(PAGE_OBJ_IMAGE, PAGE_IMG_IMAGE, x, y, right, bottom, BROWSER_BLACK, 0x10, 0, 0, (u8 *) (""), (u8 *) src, (u8 *) alt);
    }
}

/* An image map, or one area of it (src, href, alt text; key is the access
 * key code, with 0x80 for a map's area). */
void Draw_ImageMap(u16 x, u16 y, u16 right, u16 bottom, u8 *src, u8 *href, u8 *alt, u8 key)
{
    if (right - x <= 0 || bottom - y <= 0) {
        return;
    }
    if (Page_IsDataPage() == 0) {
        Page_Extend(right, bottom);
        Draw_Cmd(PAGE_OBJ_IMAGE, PAGE_IMG_MAP, x, y, right, bottom, BROWSER_BLACK, 0x10, key, 0, href, src, alt);
    }
}

/* The anchor's name format ("#" and the name), 8 bytes with a stray newline: included whole. */
INCLUDE_RODATA("asm/lobby/nonmatchings/page_draw_cmds", gAnchorNameFormat);
extern char gAnchorNameFormat[];

/* A named place in the page, kept as "#name". */
void Draw_Anchor(u16 x, u16 y, u8 *name)
{
    char buf[0x100];

    memset(buf, 0, sizeof(buf));
    sprintf(buf, gAnchorNameFormat, name);
    Draw_Cmd(PAGE_OBJ_ANCHOR, 1, x, y, 0, 0, BROWSER_BLACK, 0, 0, 0, (u8 *) buf, (u8 *) (""), (u8 *) (""));
}

/* A table cell: its background image, then its fill inside the border and
 * the border itself (bcolor). */
void Draw_Cell(u16 x, u16 y, u16 right, u16 bottom, u8 size, s32 color, u8 border, s32 bcolor, u8 *background)
{
    Page_Extend(right, bottom);
    if (*background != 0) {
        Draw_Cmd(PAGE_OBJ_IMAGE, PAGE_IMG_CELL_BACK, x, y, right, bottom, BROWSER_BLACK, size, 0, 0, (u8 *) (""), (u8 *) background, (u8 *) (""));
    }
    Draw_Cmd(PAGE_OBJ_CELL, 1, x + border, y + border, right - border, bottom - border, color, size, border, PAGE_CELL_FILL, (u8 *) (""), (u8 *) (""),
                  (u8 *) (""));
    Draw_Cmd(PAGE_OBJ_CELL, 1, x, y, right, bottom, bcolor, size, border, PAGE_CELL_BORDER, (u8 *) (""), (u8 *) (""), (u8 *) (""));
}

/* A table: its background image, then the table itself. */
void Draw_Table(u16 x, u16 y, u16 right, u16 bottom, u8 size, u32 color, u8 border, u8 *background)
{
    Page_Extend(right, bottom);
    if (*background != 0) {
        Draw_Cmd(PAGE_OBJ_IMAGE, PAGE_IMG_CELL_BACK, x, y, right, bottom, BROWSER_BLACK, size, 0, 0, (u8 *) (""), (u8 *) background, (u8 *) (""));
    }
    Draw_Cmd(PAGE_OBJ_TABLE, 1, x, y, right, bottom, color, size, border, 0, (u8 *) (""), (u8 *) (""), (u8 *) (""));
}

/* Struck-out text (with href when it is a link). */
void Draw_StruckText(u16 x, u16 y, s32 color, u8 size, u16 right, u16 bottom, u8 *text, u8 *href, u8 bold)
{
    Page_Extend(right, bottom);
    Draw_Cmd(PAGE_OBJ_STRUCK, 1, x, y, right, bottom, color, size, bold, 0, (u8 *) href, (u8 *) text, (u8 *) (""));
}

/* Underlined text (with href when it is a link). */
void Draw_UnderlinedText(u16 x, u16 y, s32 color, u8 size, u16 right, u16 bottom, u8 *text, u8 *href, u8 bold)
{
    Page_Extend(right, bottom);
    Draw_Cmd(PAGE_OBJ_UNDERLINED, 1, x, y, right, bottom, color, size, bold, 0, (u8 *) href, (u8 *) text, (u8 *) (""));
}

/* A form's start: its action and whether it posts. */
void Draw_FormBegin(u8 *action, u8 post)
{
    Draw_Cmd(PAGE_OBJ_FORM, 1, 0, 0, 0, 0, BROWSER_BLACK, 0x10, 0, post, (u8 *) action, (u8 *) (""), (u8 *) (""));
}

/* A form's hidden value (name, value). */
void Draw_Hidden(u8 *name, u8 *value)
{
    Draw_Cmd(PAGE_OBJ_HIDDEN, 1, 0, 0, 0, 0, BROWSER_BLACK, 0x10, 0, 0, (u8 *) name, (u8 *) value, (u8 *) (""));
}

/*
 * Sets the page's data mode (pageMode, keeping the one before in
 * prevPageMode) to the kind of the page's <CSV> block (CSV_KIND_*): the
 * session, server-list and result blocks show the page plain, the others
 * not.
 */
void Page_SetDataMode(u8 mode)
{
    s32 dataMode;

    dataMode = mode & 0xFF;
    switch (dataMode) {
    case CSV_KIND_SESSION:
    case CSV_KIND_SERVERS:
    case CSV_KIND_RESULT:
        gBrowserState->plain = 1;
        break;
    default:
    case CSV_KIND_MENU:
    case CSV_KIND_PORTAL:
        gBrowserState->plain = 0;
        break;
    }
    gBrowserState->prevPageMode = gBrowserState->pageMode;
    gBrowserState->pageMode = mode;
}

/*
 * Copies value into the MMBB data's field which (MMBB_FIELD_*, or the
 * field's number), into entry idx (at most 39) of the list fields, cut to
 * the field's size; a value too long for a 0x100-byte field is dropped.
 */
void Mmbb_SetField(s8 which, u8 idx, char *value)
{
    u8 len = strlen(value);

    if (idx > 39) {
        idx = 39;
    }
    switch (which) {
    case MMBB_FIELD_USER_ID:
        if (len > 14) {
            len = 14;
        }
        memcpy(gMmbbData->userId, value, len + 1);
        break;
    case MMBB_FIELD_CONNECTION_ID:
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
    case MMBB_FIELD_NEXT_URL:
        if (len > 0x100) {
            len = 0;
        }
        memcpy(gMmbbData->nextUrl, value, len + 1);
        break;
    case MMBB_FIELD_RESULT:
        if (len > 0x10) {
            len = 0x10;
        }
        memcpy(gMmbbData->result, value, len + 1);
        break;
    case MMBB_FIELD_SERVER_ADDR:
        if (len > 0x61) {
            len = 0x61;
        }
        memcpy(gMmbbData->serverAddr[idx], value, len + 1);
        break;
    case MMBB_FIELD_SERVER_ROW:
        if (len > 0x100) {
            len = 0;
        }
        memcpy(gMmbbData->serverRow[idx], value, len + 1);
        break;
    case 9:
        if (len > 0x100) {
            len = 0;
        }
        memcpy(gMmbbData->f9[idx], value, len + 1);
        break;
    case MMBB_FIELD_ERROR_TEXT:
        if (len > 0x100) {
            len = 0;
        }
        memcpy(gMmbbData->errorText, value, len + 1);
        break;
    case MMBB_FIELD_SERVER_USERS:
        if (len > 0x20) {
            len = 0x20;
        }
        memcpy(gMmbbData->serverUsers[idx], value, len + 1);
        break;
    case MMBB_FIELD_SERVER_CAPACITY:
        if (len > 0x20) {
            len = 0x20;
        }
        memcpy(gMmbbData->serverCapacity[idx], value, len + 1);
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

/* Sets the page's style flags (PAGE_STYLE_*), its message and its mode,
 * then its margins (Page_SetMargins); no reload for 30 frames. */
void Page_SetStyle(u16 flags, char *msg, u8 mode, s32 marginMode)
{
    gBrowserState->style = flags;
    if (gBrowserState->forceStyle8 != 0) {
        if (((gBrowserState->style & PAGE_STYLE_BACK) >> 3) == 0) {
            gBrowserState->style += PAGE_STYLE_BACK;
        }
    }
    strcpy(gBrowserState->msg, msg);
    gBrowserState->styleMode = mode;
    gBrowserState->styleWait = 0x1E;
    gBrowserState->styled = 1;
    Page_SetMargins(mode, marginMode, 0x1E);
}

/* Page_Extend for 16-bit coordinates. */
void Page_ExtendU16(u16 x, u16 y)
{
    Page_Extend(x, y);
}

/* Keeps id as the form's login id and, unless it is empty, as the saved copy. */
void Form_SetLoginId(u8 *id)
{
    strcpy(gBrowserState->formLoginId, (char *)id);
    if ((id == NULL) || (*id == 0)) {
        return;
    }
    strcpy(gBrowserState->loginIdSave, (char *)id);
}
