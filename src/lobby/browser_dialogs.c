/*
 * The browser's dialog objects. Per dialog object (the page's background
 * image, the page itself, the scroll bars, the status line, the menu of
 * three, the software keyboard, the pointer and focus, and the dialog
 * box), in the order of their object lists (BLAYER_*): a function that
 * draws its list, a create function (its list, position and size), an
 * update function (its state, BrowserObj.state, from the work area's
 * BDLG_* field for it) and a draw function. The software keyboard's dialog
 * writes the registration form's boxes into the registration record
 * (gRegRecord).
 *
 * Last, the login the portal's dialogs keep: Browser_KeepLogin picks it by
 * the dialog's answer, and Browser_SaveLogin saves it in the network file.
 * The network mode's card save calls the two after them: the error message
 * box (Browser_SetErrorState) and an empty hook.
 * The objects are BrowserObj; the work area is BrowserWork (browser.h).
 */
#include "common.h"
#include "net_save.h"
#include "browser.h"
#include "html.h"
#include "render_state.h"
#include "lobby_globals.h"
#include "pad.h"
#include "lobby/browser_dialogs.h"   /* the texts this file draws */
#include "bgm.h"
#include "chat_box.h"
#include "kbd.h"
#include "text.h"
#include "font.h"
#include "net_file_save.h"
#include "net_mode.h"

/* The game's code for this file was built at the compiler's optimization
 * level 4: DlgPage_Draw, DlgInput_Draw, DlgPointer_Draw and
 * Browser_SaveLogin compile as the game's only at that level. */
#pragma optimization_level 4

s32 strcmp(const char *a, const char *b);
s8 Kbd_Run(char *text, s16 held, s16 pressed); /* runs the software keyboard one frame; nonzero once the text is entered */
void Chat_OpenKbd(s32 flags, u8 layout, u16 maxChars, void *text); /* opens the software keyboard */ /* unit's own declaration: the layout is declared u8 here (the definition takes s32): only so does Browser_OpenKeyboard pass it as the game's code does */
void Kbd_SetPos(s32 posY, f32 posX);
u32 strlen(const char *s);
char *strcpy(char *dst, const char *src);
void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);

/* Draws layer 0's objects: the page's background image. */
void DlgBackImg_DrawAll(void)
{
    PageObj_DrawLayer(BLAYER_BACK_IMAGE);
}

/* Dialog object 0 (layer 0): the page's background image, tiled. Mode 1
 * makes the object and sets it looking for the image. */
void DlgBackImg_Create(s32 mode)
{
    BrowserObj *obj;

    switch (mode) {
    case 1:
        obj = PageObj_Alloc(BLAYER_BACK_IMAGE, 0);
        if (obj != NULL) {
            obj->visible = 1;
            obj->active = 1;
            obj->update = DlgBackImg_Update;
            obj->draw = DlgBackImg_Draw;
            obj->x = 0.0f;
            obj->y = 0.0f;
            obj->unk4C = 0;
            obj->unk50 = 0;
            obj->w = 0.0f;
            obj->h = 0.0f;
            obj->state = BOBJ_STATE_HIDDEN;
            obj->glowDir = 0;
            obj->blink = 0;
            obj->glow = 0;
            obj->unkE = 0;
            obj->unk10 = 0;
            obj->field = 0;
            gBrowserWork->dlgBackImgState = BDLG_BACK_FIND;
        }
        break;
    }
}

/* Looks among the page's objects for the body's background image
 * (PAGE_IMG_BODY_BACK) until it has loaded, then takes its texture, size and
 * fixed flag (field); shows the object while the image is shown. */
void DlgBackImg_Update(BrowserObj *obj)
{
    u16 i;

    switch (gBrowserWork->dlgBackImgState) {
    case BDLG_BACK_FIND:
        obj->state = BOBJ_STATE_HIDDEN;
        for (i = 0; i < PAGE_OBJ_COUNT; i++) {
            if (gBrowserWork->obj[i] == NULL || !gBrowserWork->obj[i]->active) {
                break;
            }
            if (gBrowserWork->obj[i]->kind == PAGE_OBJ_IMAGE && gBrowserWork->obj[i]->active == PAGE_IMG_BODY_BACK) {
                switch (gBrowserWork->obj[i]->sub) {
                case 0:
                case PAGE_IMG_REQUESTED:
                    return;
                case PAGE_IMG_LOADED:
                    obj->tex = gBrowserWork->obj[i]->tex;
                    obj->field = gBrowserWork->obj[i]->field;
                    obj->w = gBrowserWork->texW[obj->tex];
                    obj->h = gBrowserWork->texH[obj->tex];
                    gBrowserWork->dlgBackImgState = BDLG_BACK_SHOWN;
                    return;
                case PAGE_IMG_FAILED:
                    gBrowserWork->dlgBackImgState = BDLG_BACK_NONE;
                    return;
                }
            }
        }
        break;
    case BDLG_BACK_NONE:
        obj->state = BOBJ_STATE_HIDDEN;
        break;
    case BDLG_BACK_SHOWN:
        obj->state = BOBJ_STATE_SHOWN;
        break;
    case BDLG_BACK_CLOSE:
        obj->state = BOBJ_STATE_FREE;
        break;
    }
}

/* Tiles the background image over the screen from the page's top-left
 * margin, scrolled with the page unless the image is fixed (field); at most
 * 512 tiles. */
void DlgBackImg_Draw(BrowserObj *obj)
{
    u16 tileCount;
    f32 x;
    f32 y;

    switch (obj->state) {
    case BOBJ_STATE_HIDDEN:
        return;
    case BOBJ_STATE_SHOWN:
        break;
    case BOBJ_STATE_FREE:
        PageObj_Free(obj);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserWork->texHandle[obj->tex] == 0) {
        return;
    }
    tileCount = 0;
    if (obj->field) {
        y = gBrowserState->marginTop;
    } else {
        y = gBrowserState->marginTop - (s16)gBrowserState->y % (s16)(obj->y + obj->h);
    }
    while (y < 448.0f) {
        if (obj->field) {
            x = gBrowserState->marginLeft;
        } else {
            x = gBrowserState->marginLeft - (s16)gBrowserState->x % (s16)(obj->x + obj->w);
        }
        while (x < 640.0f) {
            tileCount++;
            if (tileCount > 512) {
                return;
            }
            Page_DrawImage(obj->tex, x, y, obj->w, obj->h, 0, 0);
            x += obj->w;
        }
        y += obj->h;
    }
}

/* Draws layer 1's objects: the page. */
void DlgPage_DrawAll(void)
{
    PageObj_DrawLayer(BLAYER_PAGE);
}

/* Dialog object 1 (layer 1): the page itself. Mode 1 makes it, shown. */
void DlgPage_Create(s32 mode)
{
    BrowserObj *obj;

    switch (mode) {
    case 1:
        obj = PageObj_Alloc(BLAYER_PAGE, 0);
        if (obj != NULL) {
            obj->visible = 1;
            obj->active = 1;
            obj->update = DlgPage_Update;
            obj->draw = DlgPage_Draw;
            obj->x = 0.0f;
            obj->y = 0.0f;
            obj->unk4C = 0;
            obj->unk50 = 0;
            obj->w = 0.0f;
            obj->h = 0.0f;
            obj->state = BOBJ_STATE_SHOWN;
            obj->glowDir = 0;
            obj->blink = 0;
            obj->glow = 0;
            obj->unkE = 0;
            obj->unk10 = 0;
            gBrowserWork->dlgPageState = BDLG_PAGE_SHOWN;
        }
        break;
    }
}

/* Follows dlgPageState: shown, or freed. */
void DlgPage_Update(BrowserObj *obj)
{
    switch (gBrowserWork->dlgPageState) {
    case BDLG_PAGE_SHOWN:
        obj->state = BOBJ_STATE_SHOWN;
        break;
    case BDLG_PAGE_CLOSE:
        obj->state = BOBJ_STATE_FREE;
        break;
    }
}

/* Draws the page (unless it is shown plain): the objects' backgrounds from
 * the last to the first, then the objects in order, a blue tint over the
 * active one (BrowserSub.activeObj) and, when the style asks for the
 * message icon and the page has a message, its label; then the open
 * drop-down list. */
void DlgPage_Draw(BrowserObj *obj)
{
    u16 i;
    BrowserObj *pageObj;
    f32 x;
    f32 y;

    switch (obj->state) {
    case BOBJ_STATE_HIDDEN:
        return;
    case BOBJ_STATE_SHOWN:
        break;
    case BOBJ_STATE_FREE:
        PageObj_Free(obj);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserState->plain == 0) {
        Browser_SetRenderState(RS_ZTEST, RS_CMP_ALWAYS);
        for (i = PAGE_OBJ_COUNT - 1; i > 0; i--) {
            pageObj = gBrowserWork->obj[i];
            if (pageObj != NULL && pageObj->active) {
                Page_DrawObjBack(pageObj);
            }
        }
        for (i = 1; i < PAGE_OBJ_COUNT; i++) {
            pageObj = gBrowserWork->obj[i];
            if (pageObj == NULL || !pageObj->active) {
                break;
            }
            {
                Page_DrawObj(pageObj);
                if (gBrowserSub->activeObj == i) {
                    y = gBrowserWork->obj[i]->y - (f32)gBrowserState->y;
                    x = gBrowserWork->obj[i]->x - (f32)gBrowserState->x;
                    Draw_RectClipped(0x400000FF, 1.0f + x, 1.0f + y, gBrowserWork->obj[i]->w + (x - 2.0f),
                                     gBrowserWork->obj[i]->h + (y - 1.0f));
                }
                if (((gBrowserState->style & PAGE_STYLE_NO_MSG_ICON) >> 9) == 0 && ((gBrowserState->style & PAGE_STYLE_MSG_ICON) >> 8) != 0 &&
                    strlen(gBrowserState->msg) != 0) {
                    Page_DrawTextLine(20, -1, 1, 20, 500.0f, 30.0f, TXT(0x00838F68) /* the label before the exit line of a page */);
                }
            }
        }
        Page_DrawDropList(gBrowserSub->activeObj);
        TextObj_Reset();
    }
}

/* Draws layer 2's objects: the scroll bars and the status line. */
void DlgVScroll_DrawAll(void)
{
    PageObj_DrawLayer(BLAYER_BARS);
}

/* Dialog object 2 (layer 2): the vertical scroll bar, at the right. Mode 1 makes it. */
void DlgVScroll_Create(s32 mode)
{
    BrowserObj *obj;

    switch (mode) {
    case 1:
        obj = PageObj_Alloc(BLAYER_BARS, 0);
        if (obj != NULL) {
            obj->visible = 1;
            obj->active = 1;
            obj->update = DlgVScroll_Update;
            obj->draw = DlgVScroll_Draw;
            obj->x = 624 - gBrowserState->marginRight;
            obj->y = gBrowserState->marginTop;
            obj->unk4C = 0;
            obj->unk50 = 0;
            obj->w = 16.0f;
            obj->h = PAGE_VIEW_H;
            obj->state = BOBJ_STATE_HIDDEN;
            obj->glowDir = 0;
            obj->blink = 0;
            obj->glow = 0;
            obj->unkE = 0;
            obj->unk10 = 0;
            obj->timer = 0;
            gBrowserWork->dlgVScrollState = BDLG_SCROLL_SHOWN;
        }
        break;
    }
}

/* Follows dlgVScrollState: hidden, shown, or an arrow pressed. A pressed
 * arrow shows pressed for 4 frames, then the bar is shown plain again. */
void DlgVScroll_Update(BrowserObj *obj)
{
    switch (gBrowserWork->dlgVScrollState) {
    case BDLG_SCROLL_HIDDEN:
        obj->state = BOBJ_STATE_HIDDEN;
        break;
    case BDLG_SCROLL_SHOWN:
        obj->state = BOBJ_STATE_SHOWN;
        break;
    case BDLG_SCROLL_FIRST_PRESSED:
        obj->state = BOBJ_STATE_FIRST_PRESSED;
        if (obj->timer++ > 3) {
            obj->timer = 0;
            gBrowserWork->dlgVScrollState = BDLG_SCROLL_SHOWN;
        }
        break;
    case BDLG_SCROLL_SECOND_PRESSED:
        obj->state = BOBJ_STATE_SECOND_PRESSED;
        if (obj->timer++ > 3) {
            obj->timer = 0;
            gBrowserWork->dlgVScrollState = BDLG_SCROLL_SHOWN;
        }
        break;
    case BDLG_SCROLL_CLOSE:
        obj->state = BOBJ_STATE_FREE;
        break;
    }
}

/* When the page's style has the vertical scroll bar: the up arrow, the
 * track, the thumb (when the page scrolls, placed by the page's y) and the
 * down arrow, each arrow drawn pressed while it is. */
void DlgVScroll_Draw(BrowserObj *obj)
{
    f32 arrowSize;

    switch (obj->state) {
    case BOBJ_STATE_HIDDEN:
        return;
    case BOBJ_STATE_SHOWN:
    case BOBJ_STATE_FIRST_PRESSED:
    case BOBJ_STATE_SECOND_PRESSED:
        break;
    case BOBJ_STATE_FREE:
        PageObj_Free(obj);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserState->plain == 0 && ((gBrowserState->style & PAGE_STYLE_VSCROLL) >> 1) != 0) {
        if (obj->state != BOBJ_STATE_FIRST_PRESSED) {
            arrowSize = 16.0f;
            Page_DrawWidget(PAGE_WIDGET_ARROW_UP, obj->x, obj->y, arrowSize, arrowSize, 0);
        } else {
            arrowSize = 16.0f;
            Page_DrawWidget(PAGE_WIDGET_ARROW_UP_PRESSED, obj->x, obj->y, arrowSize, arrowSize, 0);
        }
        Page_DrawWidget(PAGE_WIDGET_VTRACK, obj->x, 16.0f + obj->y, 16.0f, PAGE_VIEW_H, 0);
        if (gBrowserState->vScroll != 0) {
            gBrowserState->vThumbY = gBrowserState->marginTop + (gBrowserState->y * 348 / gBrowserState->pageH + 16);
            Page_DrawWidget(PAGE_WIDGET_VTHUMB, obj->x, gBrowserState->vThumbY, arrowSize, gBrowserState->vThumbLen, 0);
        }
        if (obj->state != BOBJ_STATE_SECOND_PRESSED) {
            Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN, obj->x, 380.0f, 16.0f, 16.0f, 0);
        } else {
            Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN_PRESSED, obj->x, 380.0f, 16.0f, 16.0f, 0);
        }
    }
}

/* Draws layer 2's objects (as DlgVScroll_DrawAll). */
void DlgHScroll_DrawAll(void)
{
    PageObj_DrawLayer(BLAYER_BARS);
}

/* Dialog object 3 (layer 2): the horizontal scroll bar, at the bottom. Mode 1 makes it. */
void DlgHScroll_Create(s32 mode)
{
    BrowserObj *obj;

    switch (mode) {
    case 1:
        obj = PageObj_Alloc(BLAYER_BARS, 0);
        if (obj != NULL) {
            obj->visible = 1;
            obj->active = 1;
            obj->update = DlgHScroll_Update;
            obj->draw = DlgHScroll_Draw;
            obj->x = gBrowserState->marginLeft;
            obj->y = 412 - gBrowserState->marginBottom;
            obj->unk4C = 0;
            obj->unk50 = 0;
            obj->w = PAGE_VIEW_W;
            obj->h = 16.0f;
            obj->state = BOBJ_STATE_HIDDEN;
            obj->glowDir = 0;
            obj->blink = 0;
            obj->glow = 0;
            obj->unkE = 0;
            obj->unk10 = 0;
            obj->timer = 0;
            gBrowserWork->dlgHScrollState = BDLG_SCROLL_SHOWN;
        }
        break;
    }
}

/* Follows dlgHScrollState, as DlgVScroll_Update. */
void DlgHScroll_Update(BrowserObj *obj)
{
    switch (gBrowserWork->dlgHScrollState) {
    case BDLG_SCROLL_HIDDEN:
        obj->state = BOBJ_STATE_HIDDEN;
        break;
    case BDLG_SCROLL_SHOWN:
        obj->state = BOBJ_STATE_SHOWN;
        break;
    case BDLG_SCROLL_FIRST_PRESSED:
        obj->state = BOBJ_STATE_FIRST_PRESSED;
        if (obj->timer++ > 3) {
            obj->timer = 0;
            gBrowserWork->dlgHScrollState = BDLG_SCROLL_SHOWN;
        }
        break;
    case BDLG_SCROLL_SECOND_PRESSED:
        obj->state = BOBJ_STATE_SECOND_PRESSED;
        if (obj->timer++ > 3) {
            obj->timer = 0;
            gBrowserWork->dlgHScrollState = BDLG_SCROLL_SHOWN;
        }
        break;
    case BDLG_SCROLL_CLOSE:
        obj->state = BOBJ_STATE_FREE;
        break;
    }
}

/* When the page's style has the horizontal scroll bar: the left arrow, the
 * track, the thumb (when the page scrolls, placed by the page's x) and the
 * right arrow, each arrow drawn pressed while it is. */
void DlgHScroll_Draw(BrowserObj *obj)
{
    f32 arrowSize;

    switch (obj->state) {
    case BOBJ_STATE_HIDDEN:
        return;
    case BOBJ_STATE_SHOWN:
    case BOBJ_STATE_FIRST_PRESSED:
    case BOBJ_STATE_SECOND_PRESSED:
        break;
    case BOBJ_STATE_FREE:
        PageObj_Free(obj);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserState->plain == 0 && ((gBrowserState->style & PAGE_STYLE_VSCROLL) >> 1) != 0) {
        Gfx_SetTexFilter(GFX_TEX_NEAREST);
        if (obj->state != BOBJ_STATE_FIRST_PRESSED) {
            arrowSize = 16.0f;
            Page_DrawWidget(PAGE_WIDGET_ARROW_LEFT, obj->x, obj->y, arrowSize, arrowSize, 0);
        } else {
            arrowSize = 16.0f;
            Page_DrawWidget(PAGE_WIDGET_ARROW_LEFT_PRESSED, obj->x, obj->y, arrowSize, arrowSize, 0);
        }
        Page_DrawWidget(PAGE_WIDGET_HTRACK, 16.0f + obj->x, obj->y, PAGE_VIEW_W, 16.0f, 0);
        if (gBrowserState->hScroll != 0) {
            gBrowserState->hThumbX = gBrowserState->marginRight + (gBrowserState->x * 552 / gBrowserState->pageW + 16);
            Page_DrawWidget(PAGE_WIDGET_HTHUMB, gBrowserState->hThumbX, obj->y, gBrowserState->hThumbLen, arrowSize, 0);
        }
        if (obj->state != BOBJ_STATE_SECOND_PRESSED) {
            Page_DrawWidget(PAGE_WIDGET_ARROW_RIGHT, 588.0f, obj->y, 16.0f, 16.0f, 0);
        } else {
            Page_DrawWidget(PAGE_WIDGET_ARROW_RIGHT_PRESSED, 588.0f, obj->y, 16.0f, 16.0f, 0);
        }
    }
}

/* Draws layer 2's objects (as DlgVScroll_DrawAll). */
void DlgStatus_DrawAll(void)
{
    PageObj_DrawLayer(BLAYER_BARS);
}

/* Dialog object 4 (layer 2): the status line, at the bottom. Mode 1 makes it. */
void DlgStatus_Create(s32 mode)
{
    BrowserObj *obj;

    switch (mode) {
    case 1:
        obj = PageObj_Alloc(BLAYER_BARS, 0);
        if (obj != NULL) {
            obj->visible = 1;
            obj->active = 1;
            obj->update = DlgStatus_Update;
            obj->draw = DlgStatus_Draw;
            obj->x = gBrowserState->marginLeft;
            obj->y = 428 - gBrowserState->marginBottom;
            obj->unk4C = 0;
            obj->unk50 = 0;
            obj->w = 640 - gBrowserState->marginLeft - gBrowserState->marginRight;
            obj->h = 20.0f;
            obj->state = BOBJ_STATE_HIDDEN;
            obj->glowDir = 0;
            obj->blink = 0;
            obj->glow = 0;
            obj->unkE = 0;
            obj->unk10 = 0;
            gBrowserWork->dlgStatusState = BDLG_STATUS_TITLE;
        }
        break;
    }
}

/* Follows dlgStatusState: what the status line shows, hidden, or freed. */
void DlgStatus_Update(BrowserObj *obj)
{
    switch (gBrowserWork->dlgStatusState) {
    case BDLG_STATUS_HIDDEN:
        obj->state = BOBJ_STATE_HIDDEN;
        break;
    case BDLG_STATUS_TITLE:
        obj->state = BDLG_STATUS_TITLE;
        break;
    case BDLG_STATUS_URL:
        obj->state = BDLG_STATUS_URL;
        break;
    case BDLG_STATUS_TARGET:
        obj->state = BDLG_STATUS_TARGET;
        break;
    case BDLG_STATUS_CLOSE:
        obj->state = BOBJ_STATE_FREE;
        break;
    }
}

/* When the page's style has the status bar and the keyboard is not up:
 * the bar, then the page's title, a label and the current URL, or the text
 * of what the pointer is on. The page's first object, the title object,
 * holds the title and that text, as its first and second strings. */
void DlgStatus_Draw(BrowserObj *obj)
{
    BrowserObj *titleObj;

    switch (obj->state) {
    case BOBJ_STATE_HIDDEN:
        return;
    case BDLG_STATUS_TITLE:
    case BDLG_STATUS_URL:
    case BDLG_STATUS_TARGET:
        break;
    case BOBJ_STATE_FREE:
        PageObj_Free(obj);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserState->plain == 0 && ((gBrowserState->style & PAGE_STYLE_STATUS_BAR) >> 2) != 0) {
        Page_DrawWidget(PAGE_WIDGET_STATUS_BAR, obj->x, obj->y, obj->w, obj->h, 0);
        if (gBrowserWork->dlgInputState != BDLG_INPUT_OPEN) {
            titleObj = gBrowserWork->obj[0];
            if (titleObj != NULL) {
                switch (obj->state) {
                case BDLG_STATUS_TITLE:
                    if (titleObj->s1 != NULL) {
                        Page_DrawTextLine(21, BROWSER_BLACK, 0, 20, obj->x, obj->y, titleObj->s1);
                    }
                    break;
                case BDLG_STATUS_URL:
                    Page_DrawTextLine(21, BROWSER_BLACK, 0, 16, 2.0f + obj->x, 2.0f + obj->y, Browser_GetText(BTEXT_STATUS, 0));
                    Page_DrawTextLine(21, BROWSER_BLACK, 0, 16, 72.0f + (2.0f + obj->x), 2.0f + obj->y,
                                      BrowserHist_CurUrl());
                    break;
                case BDLG_STATUS_TARGET:
                    Page_DrawTextLine(21, BROWSER_BLACK, 0, 16, 2.0f + obj->x, 2.0f + obj->y, titleObj->s2);
                    break;
                }
            }
            if (gBrowserSub->dlgMode == BDLG_MODE_PAGE) {
                TextObj_Reset();
            }
        }
    }
}

/* Draws layer 3's objects: the menu of three. */
void DlgMenu_DrawAll(void)
{
    PageObj_DrawLayer(BLAYER_MENU);
}

/* Dialog object 5 (layer 3): the menu of three. Mode 1 makes it. */
void DlgMenu_Create(s32 mode)
{
    BrowserObj *obj;

    switch (mode) {
    case 1:
        obj = PageObj_Alloc(BLAYER_MENU, 0);
        if (obj != NULL) {
            obj->visible = 1;
            obj->active = 1;
            obj->update = DlgMenu_Update;
            obj->draw = DlgMenu_Draw;
            obj->x = 200.0f;
            obj->y = 150.0f;
            obj->unk4C = 0;
            obj->unk50 = 0;
            obj->w = 200.0f;
            obj->h = 150.0f;
            obj->state = BOBJ_STATE_HIDDEN;
            obj->glowDir = 0;
            obj->blink = 0;
            obj->glow = 0;
            obj->unkE = 0;
            obj->unk10 = 0;
            gBrowserWork->dlgMenuState = BDLG_MENU_HIDDEN;
        }
        break;
    }
}

/* Follows dlgMenuState: hidden, shown, or freed. */
void DlgMenu_Update(BrowserObj *obj)
{
    switch (gBrowserWork->dlgMenuState) {
    case BDLG_MENU_HIDDEN:
        obj->state = BOBJ_STATE_HIDDEN;
        break;
    case BDLG_MENU_SHOWN:
        obj->state = BOBJ_STATE_SHOWN;
        break;
    case BDLG_MENU_CLOSE:
        obj->state = BOBJ_STATE_FREE;
        break;
    }
}

/* Draws the browser's overlay over the page (its margins, the message
 * icon, the dimming, the fade: Browser_DrawOverlay), hidden or shown; shown,
 * then the menu's window, its three lines and a frame around the line
 * chosen (BrowserSub.dlgCursor).
 * The shown case jumps into the default case's drawing: the game's code
 * has that layout. */
void DlgMenu_Draw(BrowserObj *obj)
{
    u8 i;
    s32 lineOfs;
    f32 y;
    f32 cursorY;

    switch (obj->state) {
    case BOBJ_STATE_HIDDEN:
    case BOBJ_STATE_SHOWN:
        Browser_DrawOverlay();
        if (obj->state == BOBJ_STATE_HIDDEN) {
            break;
        }
        goto draw;
    case BOBJ_STATE_FREE:
        PageObj_Free(obj);
        gBrowserWork->freedObjs++;
        break;
    default:
    draw:
        obj->h = 110.0f;
        obj->w = 200.0f;
        BrowserDlg_DrawWindow(obj->x, obj->y - 20.0f, obj->x + obj->w, obj->y + obj->h);
        for (i = 0, lineOfs = 0; i < 3; i++, lineOfs += 20) {
            y = 20.0f + obj->y + (f32)lineOfs;
            Page_DrawTextLine(20, -1, 0, 20, 40.0f + obj->x, y, Browser_GetText(BTEXT_MENU, i));
        }
        TextObj_Reset();
        cursorY = obj->y + (f32)(gBrowserSub->dlgCursor * 20);
        Page_DrawThinFrame(-1, 40.0f + obj->x - 2.0f, cursorY - 2.0f, 2.0f + (160.0f + obj->x), 2.0f + (20.0f + cursorY));
        break;
    }
}

/* Opens the software keyboard on text (kbdText keeps a
 * copy: "" when text is too long), at most max characters. */
void Browser_OpenKeyboard(char *text, s16 max, s32 mode)
{
    if (max > 255u) {
        max = 255;
    }
    if (strlen(text) > 255) {
        strcpy(gBrowserWork->kbdText, "");
    } else {
        strcpy(gBrowserWork->kbdText, text);
    }
    Chat_OpenKbd(0, mode, max, text);
    Kbd_SetPos(300, 84.0f);
}

/* Runs the software keyboard for a text field one frame; returns its text once
 * entered, else NULL. */
char *Browser_RunKeyboard(void)
{
    Font_SetSpacing(22, 22);
    if (Kbd_Run(gBrowserWork->kbdText, gBrowserWork->padHeld, *(s16 *)&gPadState[0].pressed) != 0) {
        return gBrowserWork->kbdText;
    }
    return NULL;
}

/* Draws layer 4's objects: the software keyboard. */
void DlgInput_DrawAll(void)
{
    PageObj_DrawLayer(BLAYER_KEYBOARD);
}

/* Dialog object 6 (layer 4): the software keyboard. Mode 1 makes it. */
void DlgInput_Create(s32 mode)
{
    BrowserObj *obj;

    switch (mode) {
    case 1:
        obj = PageObj_Alloc(BLAYER_KEYBOARD, 0);
        if (obj != NULL) {
            obj->visible = 1;
            obj->active = 1;
            obj->update = DlgInput_Update;
            obj->draw = DlgInput_Draw;
            obj->x = 0.0f;
            obj->y = 0.0f;
            obj->unk4C = 0;
            obj->unk50 = 0;
            obj->w = 0.0f;
            obj->h = 0.0f;
            obj->state = BOBJ_STATE_SHOWN;
            obj->glowDir = 0;
            obj->blink = 0;
            obj->glow = 0;
            obj->unkE = 0;
            obj->unk10 = 0;
            gBrowserWork->dlgInputState = BDLG_INPUT_IDLE;
        }
        break;
    }
}

/* Follows dlgInputState: shown while the keyboard is up, else hidden; freed on close. */
void DlgInput_Update(BrowserObj *obj)
{
    switch (gBrowserWork->dlgInputState) {
    case BDLG_INPUT_IDLE:
        obj->state = BOBJ_STATE_HIDDEN;
        break;
    case BDLG_INPUT_OPEN:
        obj->state = BOBJ_STATE_SHOWN;
        break;
    case BDLG_INPUT_CLOSE:
        obj->state = BOBJ_STATE_FREE;
        break;
    }
}

/*
 * Runs the software keyboard over the page. Once the text is entered it
 * goes into the active text box, cut to the box's length, and a
 * registration box (field 4) also writes it into its gRegRecord field, by
 * the box's two-digit name; box "02" takes a number, and a 0 empties the
 * box. Entered or cancelled, the pad goes back to the page, and the page
 * goes back to checking its images (PAGE_LOAD_IMAGES) so that it is drawn
 * again. Freeing the object cancels the keyboard.
 */
void DlgInput_Draw(BrowserObj *obj)
{
    char *text;
    s16 maxLen;
    s16 len;
    u16 i;

    switch (obj->state) {
    case BOBJ_STATE_HIDDEN:
        return;
    case BOBJ_STATE_SHOWN:
        break;
    case BOBJ_STATE_FREE:
        PageObj_Free(obj);
        Kbd_Cancel();
        gBrowserWork->freedObjs++;
        return;
    }
    Browser_SetRenderState(RS_ZTEST, RS_CMP_ALWAYS);
    Chat_DrawKbdBox(0xBD6C6CC5);
    Font_SetHalfWidth(1);
    text = Browser_RunKeyboard();
    Font_DrawAllPages();
    if (text == NULL) {
        return;
    }
    if (gBrowserWork->kbdText[0] == -1) {
        gBrowserSub->activeObj = 0;
        gBrowserSub->dlgMode = BDLG_MODE_PAGE;
        gBrowserWork->dlgInputState = BDLG_INPUT_IDLE;
        gBrowserWork->dlgPointerState = BDLG_POINTER_SHOWN;
        gBrowserWork->dlgMenuState = BDLG_MENU_HIDDEN;
        gBrowserState->page = PAGE_LOAD_IMAGES;
        return;
    }
    maxLen = gBrowserWork->obj[gBrowserSub->activeObj]->sub;
    len = strlen(gBrowserWork->kbdText);
    if (gBrowserWork->obj[gBrowserSub->activeObj]->sub < len) {
        len = maxLen;
    }
    for (i = 0; i < len; i++) {
        *gBrowserWork->obj[gBrowserSub->activeObj]->s2 = gBrowserWork->kbdText[i];
        gBrowserWork->obj[gBrowserSub->activeObj]->s2++;
    }
    *gBrowserWork->obj[gBrowserSub->activeObj]->s2 = 0;
    gBrowserWork->obj[gBrowserSub->activeObj]->s2 -= len;
    if (gBrowserWork->obj[gBrowserSub->activeObj]->field == 4) {
        switch ((((u8 *)gBrowserWork->obj[gBrowserSub->activeObj]->s1)[1] - '0') + (((u8 *)gBrowserWork->obj[gBrowserSub->activeObj]->s1)[0] - '0') * 10) {
        case 0:
            memcpy(gRegRecord.box00, gBrowserWork->kbdText, len + 1);
            break;
        case 1:
            memcpy(gRegRecord.box01, gBrowserWork->kbdText, len + 1);
            break;
        case 2:
            switch (strlen(gBrowserWork->kbdText)) {
            case 1:
                if (gBrowserWork->kbdText[0] == '0') {
                    *gBrowserWork->obj[gBrowserSub->activeObj]->s2 = 0;
                } else {
                    gRegRecord.box02 = gBrowserWork->kbdText[0] - '0';
                }
                break;
            case 2:
                gRegRecord.box02 = (gBrowserWork->kbdText[0] - '0') * 10 + (gBrowserWork->kbdText[1] - '0');
                if (gRegRecord.box02 == 0) {
                    *gBrowserWork->obj[gBrowserSub->activeObj]->s2 = 0;
                }
                break;
            case 0:
            default:
                gRegRecord.box02 = 0;
                break;
            }
            break;
        case 3:
            memcpy(gRegRecord.box03, gBrowserWork->kbdText, 8);
            break;
        case 8:
            memcpy(gRegRecord.box00, gBrowserWork->kbdText, len + 1);
            break;
        case 5:
            memcpy(gRegRecord.box05, gBrowserWork->kbdText, len + 1);
            break;
        case 7:
            memcpy(gRegRecord.box07, gBrowserWork->kbdText, len + 1);
            break;
        }
    }
    gBrowserSub->activeObj = 0;
    gBrowserSub->dlgMode = BDLG_MODE_PAGE;
    gBrowserWork->dlgPointerState = BDLG_POINTER_SHOWN;
    gBrowserWork->dlgInputState = BDLG_INPUT_IDLE;
    gBrowserState->page = PAGE_LOAD_IMAGES;
}

/* Draws layer 8's objects: the pointer. */
void DlgPointer_DrawAll(void)
{
    PageObj_DrawLayer(BLAYER_POINTER);
}

/* Dialog object 7 (layer 8): the pointer and the focus frame. Mode 1 makes it. */
void DlgPointer_Create(s32 mode)
{
    BrowserObj *obj;

    switch (mode) {
    case 1:
        obj = PageObj_Alloc(BLAYER_POINTER, 0);
        if (obj != NULL) {
            obj->visible = 1;
            obj->active = 1;
            obj->update = DlgPointer_Update;
            obj->draw = DlgPointer_Draw;
            obj->x = 150.0f;
            obj->y = 100.0f;
            obj->unk4C = 0;
            obj->unk50 = 0;
            obj->w = 20.0f;
            obj->h = 20.0f;
            obj->state = BOBJ_STATE_HIDDEN;
            obj->glowDir = 0;
            obj->blink = 0;
            obj->glow = 5;
            obj->unkE = 0;
            obj->unk10 = 0;
            gBrowserWork->dlgPointerState = BDLG_POINTER_SHOWN;
            gBrowserWork->cursorOnLink = 0;
        }
        break;
    }
}

/* Follows dlgPointerState: shown, hidden while a dialog has the pad, or freed. */
void DlgPointer_Update(BrowserObj *obj)
{
    switch (gBrowserWork->dlgPointerState) {
    case BDLG_POINTER_HIDDEN:
        obj->state = BOBJ_STATE_HIDDEN;
        break;
    case BDLG_POINTER_SHOWN:
        obj->state = BOBJ_STATE_SHOWN;
        break;
    case BDLG_POINTER_CLOSE:
        obj->state = BOBJ_STATE_FREE;
        break;
    }
}

/*
 * Draws the pointer, or the focus. Nothing on a plain page, nor while the
 * browser's first page is requested, parsed or fading in. Without a free
 * pointer (PAGE_STYLE_POINTER clear) the focused object's frame pulses
 * (Page_DrawFocus). With one, the pointer is tested against the page's
 * links and form fields: the one under it is framed (red while the pulse
 * rises; a link's neighbouring parts with the same address too) and becomes
 * the focus, and the pointer is drawn active. The cases that test more
 * first jump into the shared test (check), as the game's code does.
 */
void DlgPointer_Draw(BrowserObj *obj)
{
    u16 i;
    s8 onTarget;
    s32 color;
    f32 x;
    f32 y;
    f32 pointerX;
    f32 pointerY;
    BrowserObj *target;

    switch (obj->state) {
    case BOBJ_STATE_HIDDEN:
        return;
    case BOBJ_STATE_SHOWN:
        break;
    case BOBJ_STATE_FREE:
        PageObj_Free(obj);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserState->plain != 0) {
        return;
    }
    if (gBrowserState->initMode != 0) {
        switch (gBrowserState->page) {
        case PAGE_LOAD_REQUESTED:
        case PAGE_LOAD_PARSING:
            return;
        }
        if (gBrowserState->fade != 0) {
            return;
        }
    }
    if ((gBrowserState->style & PAGE_STYLE_POINTER) == 0) {
        if (gBrowserWork->focus == 0) {
            gBrowserSub->cursorX = gBrowserState->marginLeft * 2;
            gBrowserSub->cursorY = 414;
        } else {
            if (gBrowserWork->cursorOnLink != 0) {
                obj->blink = 0;
                gBrowserWork->cursorOnLink = 0;
            }
            if (obj->blink <= 0) {
                obj->glow = 15;
            }
            if (obj->blink >= 255) {
                obj->glow = -15;
            }
            obj->blink += obj->glow;
            if (gBrowserWork->dlgBoxKind != BDLG_KIND_NONE) {
                return;
            }
            Page_DrawFocus(obj);
        }
        return;
    }
    switch (gBrowserWork->dlgStatusState) {
    case BDLG_STATUS_TARGET:
        onTarget = 1;
        break;
    default:
        onTarget = 0;
        break;
    }
    if (gBrowserSub->cursorX > gBrowserState->marginLeft + PAGE_VIEW_W) {
        onTarget = 0;
    }
    if (gBrowserSub->cursorY > gBrowserState->marginTop + PAGE_VIEW_H) {
        onTarget = 0;
    }
    switch (gBrowserWork->dlgBoxKind) {
    case BDLG_KIND_NONE:
    case BDLG_KIND_LOADING:
        if (gBrowserWork->dlgMenuState == BDLG_MENU_HIDDEN && 412 - gBrowserState->marginBottom > gBrowserSub->cursorY &&
            gBrowserWork->obj[gBrowserSub->activeObj] != NULL && gBrowserWork->obj[gBrowserSub->activeObj]->kind != PAGE_OBJ_SELECT) {
            for (i = 0; i < PAGE_OBJ_COUNT; i++) {
                if (gBrowserWork->obj[i] == NULL) {
                    break;
                }
                if (!gBrowserWork->obj[i]->active) {
                    continue;
                }
                switch (gBrowserWork->obj[i]->kind) {
                case PAGE_OBJ_STRUCK:
                case PAGE_OBJ_UNDERLINED:
                    if ((u8)*gBrowserWork->obj[i]->s1 == 0) {
                        continue;
                    }
                    goto check;
                case PAGE_OBJ_IMAGE:
                    if (gBrowserWork->obj[i]->active == PAGE_IMG_MAP) {
                        goto check;
                    }
                    if (gBrowserWork->obj[i]->active != PAGE_IMG_INPUT) {
                        continue;
                    }
                case PAGE_OBJ_LINK:
                case PAGE_OBJ_TEXT_INPUT:
                case PAGE_OBJ_PASSWORD:
                case PAGE_OBJ_SELECT:
                case PAGE_OBJ_SUBMIT:
                case PAGE_OBJ_RESET:
                case PAGE_OBJ_BUTTON:
                case PAGE_OBJ_RADIO:
                case PAGE_OBJ_CHECKBOX:
                check:
                    target = gBrowserWork->obj[i];
                    x = gBrowserWork->obj[i]->x - (f32)gBrowserState->x;
                    pointerX = gBrowserSub->cursorX;
                    if (pointerX < x) {
                        continue;
                    }
                    if (!(pointerX <= gBrowserWork->obj[i]->w + x)) {
                        continue;
                    }
                    y = target->y - (f32)gBrowserState->y;
                    pointerY = gBrowserSub->cursorY;
                    if (pointerY <= y) {
                        continue;
                    }
                    if (!(pointerY < target->h + y)) {
                        continue;
                    }
                    if (gBrowserWork->cursorOnLink != 0) {
                        obj->blink = 0;
                        gBrowserWork->cursorOnLink = 0;
                    }
                    if (obj->blink <= 0) {
                        obj->glow = 15;
                    }
                    if (obj->blink >= 255) {
                        obj->glow = -15;
                    }
                    obj->blink += obj->glow;
                    color = obj->glow > 0 ? BROWSER_CURSOR_RED : -1;
                    Page_DrawFrame(color, 3, gBrowserWork->obj[i]->x - (f32)gBrowserState->x - 1.0f,
                                   gBrowserWork->obj[i]->y - (f32)gBrowserState->y - 1.0f,
                                   3.0f + (gBrowserWork->obj[i]->x + gBrowserWork->obj[i]->w - (f32)gBrowserState->x),
                                   3.0f + (gBrowserWork->obj[i]->y + gBrowserWork->obj[i]->h - (f32)gBrowserState->y));
                    if (gBrowserWork->obj[i]->kind == PAGE_OBJ_LINK) {
                        if (gBrowserWork->obj[i - 1] != NULL && gBrowserWork->obj[i - 1]->active &&
                            strcmp(gBrowserWork->obj[i - 1]->s1, gBrowserWork->obj[i]->s1) == 0) {
                            Page_DrawFrame(color, 3, gBrowserWork->obj[i - 1]->x - (f32)gBrowserState->x - 1.0f,
                                           gBrowserWork->obj[i - 1]->y - (f32)gBrowserState->y - 1.0f,
                                           3.0f + (gBrowserWork->obj[i - 1]->x + gBrowserWork->obj[i - 1]->w - (f32)gBrowserState->x),
                                           3.0f + (gBrowserWork->obj[i - 1]->y + gBrowserWork->obj[i - 1]->h - (f32)gBrowserState->y));
                        }
                        if (gBrowserWork->obj[i + 1] != NULL && gBrowserWork->obj[i + 1]->active &&
                            strcmp(gBrowserWork->obj[i + 1]->s1, gBrowserWork->obj[i]->s1) == 0) {
                            Page_DrawFrame(color, 3, gBrowserWork->obj[i + 1]->x - (f32)gBrowserState->x - 1.0f,
                                           gBrowserWork->obj[i + 1]->y - (f32)gBrowserState->y - 1.0f,
                                           3.0f + (gBrowserWork->obj[i + 1]->x + gBrowserWork->obj[i + 1]->w - (f32)gBrowserState->x),
                                           3.0f + (gBrowserWork->obj[i + 1]->y + gBrowserWork->obj[i + 1]->h - (f32)gBrowserState->y));
                        }
                    }
                    onTarget = 1;
                    gBrowserWork->focus = i;
                    goto done;
                default:
                    continue;
                }
            }
        }
        break;
    }
done:
    switch (onTarget) {
    case 0:
        Page_DrawMark(PAGE_WIDGET_POINTER, gBrowserSub->cursorX, gBrowserSub->cursorY);
        break;
    case 1:
        Page_DrawMark(PAGE_WIDGET_POINTER_ACTIVE, gBrowserSub->cursorX, gBrowserSub->cursorY);
        break;
    }
}

/* Draws layer 6's objects: the dialog box. */
void DlgBox_DrawAll(void)
{
    PageObj_DrawLayer(BLAYER_BOX);
}

/* Dialog object 8 (layer 6): the dialog box. Mode 1 makes it, with no box asked for. */
void DlgBox_Create(s32 mode)
{
    BrowserObj *obj;

    switch (mode) {
    case 1:
        obj = PageObj_Alloc(BLAYER_BOX, 0);
        if (obj != NULL) {
            obj->visible = 1;
            obj->active = 1;
            obj->update = DlgBox_Update;
            obj->draw = DlgBox_Draw;
            obj->x = 213.0f;
            obj->y = 128.0f;
            obj->unk4C = 0;
            obj->unk50 = 0;
            obj->w = 213.0f;
            obj->h = 128.0f;
            obj->state = BDLG_BOX_HIDDEN;
            obj->glowDir = GLOW_RISING;
            obj->blink = 0;
            obj->glow = 0;
            obj->unkE = 0;
            obj->unk10 = 0;
            gBrowserWork->dlgBoxKind = BDLG_KIND_NONE;
        }
        break;
    }
}

/*
 * Picks the box's state, place and size from the box asked for
 * (dlgBoxKind): with none, the loading mark while the page's images load;
 * the loading mark, centred or small at the bottom right; a message box,
 * also for the kinds without a box of their own; the registration's two
 * boxes; the leave dialog.
 */
void DlgBox_Update(BrowserObj *obj)
{
    switch (gBrowserWork->dlgBoxKind) {
    case BDLG_KIND_NONE:
        if (gBrowserState->page == PAGE_LOAD_IMAGES) {
            obj->state = BDLG_BOX_LOADING;
        } else {
            obj->state = BDLG_BOX_HIDDEN;
        }
        break;
    case BDLG_KIND_LOADING:
        obj->state = BDLG_BOX_LOADING;
        obj->x = 213.0f;
        obj->y = 128.0f;
        obj->w = 213.0f;
        obj->h = 128.0f;
        break;
    case BDLG_KIND_LOADING_SMALL:
        obj->state = BDLG_BOX_LOADING_SMALL;
        obj->x = 496 - gBrowserState->marginLeft;
        obj->y = 364 - gBrowserState->marginBottom;
        obj->w = 128.0f;
        obj->h = 64.0f;
        break;
    case 0:
    case BDLG_KIND_MESSAGE:
    case BDLG_KIND_YES_NO:
    case BDLG_KIND_CLEAR_LOGIN:
    case 9:
    case 10:
    default:
        obj->state = BDLG_BOX_MESSAGE;
        obj->x = 60.0f;
        obj->y = 80.0f;
        obj->w = 640.0f - 2.0f * obj->x;
        obj->h = 448.0f - 2.0f * obj->y;
        break;
    case BDLG_KIND_REGISTER_ASK:
        obj->state = BDLG_BOX_YES_NO;
        obj->x = 175.0f;
        obj->y = 102.0f;
        obj->w = 290.0f;
        obj->h = 244.0f;
        break;
    case BDLG_KIND_REGISTER_INCOMPLETE:
        obj->state = BDLG_BOX_OK;
        obj->x = 175.0f;
        obj->y = 102.0f;
        obj->w = 290.0f;
        obj->h = 244.0f;
        break;
    case BDLG_KIND_EXIT:
        obj->state = BDLG_BOX_EXIT;
        obj->x = 130.0f;
        obj->y = 82.0f;
        obj->w = 380.0f;
        obj->h = 284.0f;
        break;
    case BDLG_KIND_CLOSE:
        obj->state = BOBJ_STATE_FREE;
        break;
    }
}

/*
 * Draws the dialog box. A box's glow brightens and dims in steps of 15.
 * The loading mark glows the same way, its frame counter wrapping after
 * 600 frames; while the lobby's waving title is in use (the net mode's
 * texture slot 9) it is drawn instead of the mark. A box is drawn by its kind.
 */
void DlgBox_Draw(BrowserObj *obj)
{
    s16 glowLevel;

    Browser_SetRenderState(RS_ZTEST, RS_CMP_ALWAYS);
    switch (obj->state) {
    case BDLG_BOX_MESSAGE:
    case BDLG_BOX_YES_NO:
    case BDLG_BOX_OK:
    case BDLG_BOX_EXIT:
        switch (obj->glowDir) {
        case GLOW_RISING:
            if (obj->glow <= 254) {
                obj->glow += 15;
            } else {
                obj->glowDir = GLOW_FALLING;
            }
            break;
        case GLOW_FALLING:
            if (obj->glow > 0) {
                obj->glow -= 15;
            } else {
                obj->glowDir = GLOW_RISING;
            }
            break;
        }
        break;
    }
    switch (obj->state) {
    case BDLG_BOX_HIDDEN:
        obj->blink = 0;
        break;
    case BDLG_BOX_LOADING:
    case BDLG_BOX_LOADING_SMALL:
        if (obj->blink++ > 600) {
            obj->blink = 0;
        }
        if (obj->blink >= 0) {
            switch (obj->glowDir) {
            case GLOW_RISING:
                if (obj->glow <= 254) {
                    obj->glow += 15;
                } else {
                    obj->glowDir = GLOW_FALLING;
                }
                break;
            case GLOW_FALLING:
                if (obj->glow > 0) {
                    obj->glow -= 15;
                } else {
                    obj->glowDir = GLOW_RISING;
                }
                break;
            }
            if (NetMode_WaveTitleTex() == 0) {
                glowLevel = obj->glow;
                Prim_DrawSprite(glowLevel | (((glowLevel << 16) | 0xFF000000) | (glowLevel << 8)), PageTex_Get(0), 336, 362, 224, 32, 0, 224, 256, 256);
            } else {
                glowLevel = obj->glow;
                LobbyLogin_DrawWaveTitle(glowLevel | (((glowLevel << 16) | 0xFF000000) | (glowLevel << 8)));
            }
        }
        break;
    case BDLG_BOX_MESSAGE:
        BrowserDlg_DrawMsgBox(obj);
        break;
    case BDLG_BOX_YES_NO:
        BrowserDlg_DrawYesNo(obj);
        break;
    case BDLG_BOX_OK:
        BrowserDlg_DrawOk(obj);
        break;
    case BDLG_BOX_EXIT:
        BrowserDlg_DrawPortalTop(obj);
        break;
    case BOBJ_STATE_FREE:
        PageObj_Free(obj);
        gBrowserWork->freedObjs++;
        return;
    }
}

/* An empty string, the password kept with a login id alone. The game kept
 * it as its own object beside the one Browser_OpenKeyboard uses, so it is
 * included as it is. */
INCLUDE_RODATA("asm/lobby/nonmatchings/browser_dialogs", gEmptyPassword);
extern char gEmptyPassword[];

/* Copies into the kept login (loginId, password) what the login dialog's
 * answer says to keep (BrowserWork.loginSave): the login id from the form,
 * or its saved copy, with the password from the form or none; or clears
 * them all. */
void Browser_KeepLogin(void)
{
    switch (gBrowserWork->loginSave) {
    case LOGIN_SAVE(PAGE_KIND_MENU, LOGIN_KEEP_ID):
        strcpy((char *)gBrowserState->loginId, gBrowserState->formLoginId);
        strcpy((char *)gBrowserState->password, gEmptyPassword);
        break;
    case LOGIN_SAVE(PAGE_KIND_MENU, LOGIN_KEEP_ID_PASSWORD):
        strcpy((char *)gBrowserState->loginId, gBrowserState->formLoginId);
        strcpy((char *)gBrowserState->password, gBrowserState->field2);
        break;
    case LOGIN_SAVE(PAGE_KIND_SIGNUP_DONE, LOGIN_KEEP_ID):
        strcpy((char *)gBrowserState->loginId, gBrowserState->loginIdSave);
        strcpy((char *)gBrowserState->password, gEmptyPassword);
        break;
    case LOGIN_SAVE(PAGE_KIND_SIGNUP_DONE, LOGIN_KEEP_ID_PASSWORD):
        strcpy((char *)gBrowserState->loginId, gBrowserState->loginIdSave);
        strcpy((char *)gBrowserState->password, gBrowserState->field3);
        break;
    case LOGIN_SAVE(PAGE_KIND_PASSWORD_DONE, LOGIN_KEEP_ID):
        strcpy((char *)gBrowserState->loginId, gBrowserState->formLoginId);
        strcpy((char *)gBrowserState->password, gEmptyPassword);
        break;
    case LOGIN_SAVE(PAGE_KIND_PASSWORD_DONE, LOGIN_KEEP_ID_PASSWORD):
        strcpy((char *)gBrowserState->password, gBrowserState->field3);
        break;
    case LOGIN_SAVE(PAGE_KIND_MMBB_KEICAN, LOGIN_KEEP_NONE):
        break;
    case LOGIN_SAVE(PAGE_KIND_MMBB_KEICAN, LOGIN_CLEAR):
        memset(gBrowserState->loginId, 0, sizeof(gBrowserState->loginId));
        memset(gBrowserState->password, 0, sizeof(gBrowserState->password));
        memset(gBrowserState->formLoginId, 0, sizeof(gBrowserState->formLoginId));
        memset(gBrowserState->field2, 0, sizeof(gBrowserState->field2));
        break;
    }
}

/* Saves the login, a step a frame (BrowserWork.keepLoginStep): after three
 * frames' wait, the login id and password kept (Browser_KeepLogin) go into
 * the network file's copy, which is saved (LobbyCard_RunNetSave); on failure
 * the old ones come back. Each end closes the box. Steps 100 and 101 close
 * it after one frame (seen: nothing sets them). */
void Browser_SaveLogin(void)
{
    switch (gBrowserWork->keepLoginStep) {
    case 0:
        gBrowserWork->keepLoginStep++;
        break;
    case 1:
        gBrowserWork->keepLoginStep++;
        break;
    case 2:
        gBrowserWork->keepLoginStep++;
        break;
    case 3:
        memcpy(gBrowserWork->savedId, gNetSaveData.mmbbLoginId, sizeof(gBrowserWork->savedId));
        memcpy(gBrowserWork->savedPass, gNetSaveData.mmbbPassword, sizeof(gBrowserWork->savedPass));
        Browser_KeepLogin();
        memcpy(gNetSaveData.mmbbLoginId, gBrowserState->loginId, sizeof(gBrowserWork->savedId));
        memcpy(gNetSaveData.mmbbPassword, gBrowserState->password, sizeof(gBrowserWork->savedPass));
        gBrowserWork->keepLoginStep++;
        break;
    case 4:
        switch (LobbyCard_RunNetSave()) {
        case 0:
            break;
        case 1:
            gBrowserWork->keepLoginStep++;
            break;
        case -1:
            memcpy(gNetSaveData.mmbbLoginId, gBrowserWork->savedId, sizeof(gBrowserWork->savedId));
            memcpy(gNetSaveData.mmbbPassword, gBrowserWork->savedPass, sizeof(gBrowserWork->savedPass));
            Dialog_ResetBoxState();
            break;
        }
        break;
    case 5:
        Dialog_ResetBoxState();
        break;
    case 100:
        gBrowserWork->keepLoginStep++;
        break;
    case 101:
        Dialog_ResetBoxState();
        break;
    }
}

/* Opens the error message box (dialog kind 4) for the given error. */
void Browser_SetErrorState(s32 kind)
{
    gBrowserWork->dlgBoxKind = BDLG_KIND_MESSAGE;
    gBrowserWork->errorKind = kind;
}

/* Does nothing; the network mode's card save calls it at one step. */
void Browser_EmptyHook(void)
{
}
