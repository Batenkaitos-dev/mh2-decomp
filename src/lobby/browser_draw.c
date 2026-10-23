/*
 * The lobby browser's drawing: clipped rectangles, triangles and lines,
 * frames and tiled windows, text lines, the page objects (BrowserObj:
 * links, form fields, images, the open drop-down list, the focus; their
 * kinds are enum PageObjKind), the browser's widget sprites (enum
 * PageWidget), the dialog boxes and the overlay over the whole screen.
 *
 * The HTML parser's draw commands make the page objects; the page's and
 * the dialogs' draw callbacks (DlgPage_Draw, DlgMenu_Draw) and the portal's
 * steps draw them with the functions here.
 */
#include "common.h"
#include "net_mode.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_vobj.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "nethttp.h"
#include "render_state.h"

#include "ee_stdarg.h"
#include "browser.h"
#include "window.h"
#include "lobby/browser_draw.h"   /* the texts this file uses (tools/content_gen.py) */
#include "text.h"
#include "texture.h"
#include "draw_helpers.h"
#include "font.h"

void *memcpy(void *dst, void *src, u32 n);
s32 strlen(const char *s);
void vsnprintf(char *buf, s32 size, const char *fmt, va_list ap);
void Font_Printf(const char *fmt, ...);

/*
 * Draws a filled rectangle in color from its top left to its bottom right
 * corner, each corner first clamped to the 640 x 448 screen (the top edge
 * is clamped to 640, not 448, as in the game's code). The colour is the
 * first parameter, as in the line draws: only in this order do the callers
 * set up their arguments as the game's code does.
 */
void Draw_RectClipped(s32 color, f32 left, f32 top, f32 right, f32 bottom)
{
    f32 x1;
    f32 y1;
    f32 x2;
    f32 y2;

    x1 = left;
    y1 = top;
    x2 = right;
    y2 = bottom;
    if (x1 < 0.0f) {
        x1 = 0.0f;
    }
    if (y1 < 0.0f) {
        y1 = 0.0f;
    }
    if (!(x2 <= 640.0f)) {
        x2 = 640.0f;
    }
    if (!(y2 <= 448.0f)) {
        y2 = 448.0f;
    }
    if (!(x1 <= 640.0f)) {
        x1 = 640.0f;
    }
    if (!(y1 <= 640.0f)) {
        y1 = 640.0f;
    }
    if (x2 < 0.0f) {
        x2 = 0.0f;
    }
    if (y2 < 0.0f) {
        y2 = 0.0f;
    }
    Prim_DrawRect(color, x1, y1, x2, y2);
}

/*
 * Draws a filled triangle in color through the three points, unless it is
 * off the screen: nothing is drawn when the third point is left of or
 * above the screen, or the first point right of or below it. The points
 * are not clamped. Page_DrawObjBack draws a cell border's corners with it.
 */
void Draw_TriClipped(s32 color, f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2)
{
    if (x2 < 0.0f) {
        return;
    }
    if (y2 < 0.0f) {
        return;
    }
    if (!(x0 <= 640.0f)) {
        return;
    }
    if (!(y0 <= 448.0f)) {
        return;
    }
    Prim_DrawTri(color, x0, y0, x1, y1, x2, y2);
}

/*
 * Draws one small mark from the browser's first texture at (x, y): the
 * pointer (PAGE_WIDGET_POINTER, or _POINTER_ACTIVE over something it can
 * act on; both 3 pixels up and left of the point), a radio button or a
 * check box, off or on. Other kinds draw nothing.
 */
void Page_DrawMark(u8 kind, f32 x, f32 y)
{
    switch (kind) {
    case PAGE_WIDGET_POINTER:
        Prim_DrawSprite(-1, PageTex_Get(0), -3.0f + x, -3.0f + y, 32, 32, 193, 65, 224, 96);
        break;
    case PAGE_WIDGET_POINTER_ACTIVE:
        Prim_DrawSprite(-1, PageTex_Get(0), -3.0f + x, -3.0f + y, 32, 32, 161, 65, 192, 96);
        break;
    case PAGE_WIDGET_RADIO_OFF:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 20, 20, 224, 192, 240, 208);
        break;
    case PAGE_WIDGET_RADIO_ON:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 20, 20, 240, 192, 256, 208);
        break;
    case PAGE_WIDGET_CHECK_OFF:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 20, 20, 224, 208, 240, 224);
        break;
    case PAGE_WIDGET_CHECK_ON:
        Prim_DrawSprite(-1, PageTex_Get(0), x, y, 20, 20, 240, 208, 256, 224);
        break;
    }
}

/*
 * Draws a browser widget of the given kind (enum PageWidget) at (x, y),
 * w by h, from the browser's textures, tinted by color (0: untinted): the
 * status bar, a text box, a button, the scroll arrows, tracks and thumbs
 * (the thumbs in three pieces when long enough), a closed drop-down list,
 * and the highlights of image buttons. The pointer's kinds draw nothing
 * here (Page_DrawMark draws them); most kinds use only one of w and h, or
 * neither.
 */
void Page_DrawWidget(u8 kind, f32 x, f32 y, f32 w, f32 h, s32 color)
{
    if (color == 0) {
        color = -1;
    }
    switch (kind) {
    case PAGE_WIDGET_IMGBTN_1:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 86, 30, 64, 162, 150, 192);
        break;
    case PAGE_WIDGET_IMGBTN_2:
        Draw_RectClipped(0x80808080, x, y, 169.0f + x, 30.0f + y);
        break;
    case PAGE_WIDGET_IMGBTN_3:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 208, 32, 0, 192, 208, 224);
        break;
    case PAGE_WIDGET_IMGBTN_4:
        Prim_DrawSprite(color, PageTex_Get(1), x, y, 202, 32, 0, 192, 202, 224);
        break;
    case PAGE_WIDGET_BUTTON_PRESSED:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 20, 20, 0, 64, 32, 96);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 20, 20, 32, 64, 64, 96);
        Prim_DrawSprite(color, PageTex_Get(0), 16.0f + x, y, w - 32.0f, 20, 16, 64, 32, 96);
        break;
    case PAGE_WIDGET_BUTTON:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 20, 20, 0, 96, 32, 128);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 20, 20, 32, 96, 64, 128);
        Prim_DrawSprite(color, PageTex_Get(0), 16.0f + x, y, w - 32.0f, 20, 16, 96, 32, 128);
        break;
    case PAGE_WIDGET_TEXT_BOX:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 20, 20, 0, 128, 32, 160);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 20, 20, 32, 128, 64, 160);
        Prim_DrawSprite(color, PageTex_Get(0), 4.0f + x, y, w - 4.0f, 20, 16, 128, 32, 160);
        break;
    case PAGE_WIDGET_STATUS_BAR:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 20, 20, 0, 160, 32, 192);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 20.0f, y, 20, 20, 32, 160, 64, 192);
        Prim_DrawSprite(color, PageTex_Get(0), x - 1.0f, y, w - 1.0f, 20, 16, 160, 32, 192);
        break;
    case PAGE_WIDGET_ARROW_UP:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 16, h, 224, 64, 256, 96);
        break;
    case PAGE_WIDGET_ARROW_DOWN:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 16, h, 224, 96, 256, 128);
        break;
    case PAGE_WIDGET_ARROW_LEFT:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 16, h, 224, 128, 256, 160);
        break;
    case PAGE_WIDGET_ARROW_RIGHT:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 16, h, 224, 160, 256, 192);
        break;
    case PAGE_WIDGET_ARROW_UP_PRESSED:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 16, h, 256, 128, 224, 96);
        break;
    case PAGE_WIDGET_ARROW_DOWN_PRESSED:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 16, h, 256, 96, 224, 64);
        break;
    case PAGE_WIDGET_ARROW_LEFT_PRESSED:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 16, h, 256, 192, 224, 160);
        break;
    case PAGE_WIDGET_ARROW_RIGHT_PRESSED:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 16, h, 256, 160, 224, 128);
        break;
    case PAGE_WIDGET_SELECT:
        Page_DrawWidget(PAGE_WIDGET_TEXT_BOX, x, y, w, h, 0);
        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN, x + w - 16.0f, 1.0f + y, 16.0f, h, 0);
        break;
    case PAGE_WIDGET_SELECT_PRESSED:
        Page_DrawWidget(PAGE_WIDGET_TEXT_BOX, x, y, w, h, 0);
        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN_PRESSED, x + w - 16.0f, 1.0f + y, 16.0f, h, 0);
        break;
    case PAGE_WIDGET_VTHUMB:
        if (h < 32.0f) {
            Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 96, 64, 128, 96);
            break;
        }
        Prim_DrawSprite(color, PageTex_Get(0), x, y, w, 16, 96, 64, 128, 79);
        Prim_DrawSprite(color, PageTex_Get(0), x, y + h - 16.0f, w, 16, 96, 80, 128, 96);
        Prim_DrawSprite(color, PageTex_Get(0), x, 16.0f + y, w, h - 32.0f, 96, 72, 128, 87);
        break;
    case PAGE_WIDGET_VTRACK:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 128, 64, 160, 96);
        break;
    case PAGE_WIDGET_HTHUMB:
        if (w < 32.0f) {
            Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 96, 64, 128, 96);
            break;
        }
        Prim_DrawSprite(color, PageTex_Get(0), x, y, 16, h, 96, 64, 111, 96);
        Prim_DrawSprite(color, PageTex_Get(0), x + w - 16.0f, y, 16, h, 112, 64, 128, 96);
        Prim_DrawSprite(color, PageTex_Get(0), 16.0f + x, y, w - 32.0f, h, 104, 64, 119, 96);
        break;
    case PAGE_WIDGET_HTRACK:
        Prim_DrawSprite(color, PageTex_Get(0), x, y, w, h, 64, 64, 96, 96);
        break;
    case PAGE_WIDGET_POINTER:
    case PAGE_WIDGET_POINTER_ACTIVE:
        break;
    }
}

#pragma explicit_zero_data on
const char gPageEmptyText[1] = {
#include "lobby/00838E30.inc"
};
#pragma explicit_zero_data reset

/*
 * Draws a page image: the texture in the given slot of the page textures
 * (BrowserWork.texHandle), stretched over the rectangle at (x, y), w by h,
 * using the whole texture (texW by texH texels). The last two parameters
 * are never read; Page_DrawObj passes the texture's size, others 0.
 */
void Page_DrawImage(u16 texSlot, f32 x, f32 y, f32 w, f32 h, s32 srcW, s32 srcH)
{
    Prim_DrawSprite(-1, gBrowserWork->texHandle[texSlot], x, y, w, h, 0, 0,
                  (s16)gBrowserWork->texW[texSlot], (s16)gBrowserWork->texH[texSlot]);
}

struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, s8 *title, s32 sel, u8 alpha);
/*
 * Draws an empty window over the rectangle from (x0, y0) to (x1, y1):
 * a list window with no items (ListPanel_DrawAlpha, frame style 4, fully
 * opaque), its size counted in 20-pixel cells. The browser's dialog boxes
 * draw it as their background.
 */
void BrowserDlg_DrawWindow(f32 x0, f32 y0, f32 x1, f32 y1)
{
    ListWin window;

    window.x = x0;
    window.y = y0;
    window.charH = 20;
    window.charW = 20;
    window.lineChars = (s32)(x1 - x0) / 20;
    window.lines = (s32)(y1 - y0) / 20;
    window.color = 0;
    window.flags = 4;
    window.items = NULL;
    window.cursorColor = 0;
    window.color2 = 0;
    ListPanel_DrawAlpha(&window, 0, -1, 255);
}

/*
 * Clips the rectangle from (x0, y0) to (x1, y1) to the screen
 * (Rect_ClipToScreen) and, unless it is off the screen, draws it in color
 * one pixel wider. Every caller passes the same x twice, so it draws a
 * vertical line one pixel wide. The colour comes first,
 * as in Draw_RectClipped.
 */
void Draw_VLineClipped(s32 color, f32 x0, f32 y0, f32 x1, f32 y1)
{
    if (Rect_ClipToScreen(&x0, &y0, &x1, &y1) != 0) {
        Prim_DrawRect(color, x0, y0, 1.0f + x1, y1);
    }
}

/*
 * Clips the rectangle from (x0, y0) to (x1, y1) to the screen
 * (Rect_ClipToScreen) and, unless it is off the screen, draws it in color
 * one pixel taller. Every caller passes the same y twice, so it draws a
 * horizontal line one pixel high.
 */
void Draw_HLineClipped(s32 color, f32 x0, f32 y0, f32 x1, f32 y1)
{
    if (Rect_ClipToScreen(&x0, &y0, &x1, &y1) != 0) {
        Prim_DrawRect(color, x0, y0, x1, 1.0f + y1);
    }
}

/*
 * Draws a frame thickness pixels thick in color around the rectangle from
 * (x0, y0) to (x1, y1), clipped to the page view: the corners are kept
 * inside the page's margins and its 584 by 380 view, and nothing is
 * drawn when the top left corner lies right of or below the view. The
 * focus draws frame the focused object with it. Both branches of the
 * style test are the same; the test stays as the game's code has it.
 */
void Page_DrawFrame(s32 color, s32 thickness, f32 x0, f32 y0, f32 x1, f32 y1)
{
    BrowserState *pageState = gBrowserState;
    s16 left = pageState->marginLeft;
    s16 top;

    if (x0 < left) {
        x0 = left;
    }
    top = pageState->marginTop;
    if (y0 < top) {
        y0 = top;
    }
    if (x1 > 640 - pageState->marginRight) {
        x1 = (s16)(640 - pageState->marginRight);
    }
    if (y1 > 448 - pageState->marginBottom) {
        y1 = (s16)(448 - pageState->marginBottom);
    }
    if ((pageState->style & PAGE_STYLE_STATUS_BAR) >> 2 && (pageState->style & PAGE_STYLE_VSCROLL) >> 1) {
        if (x0 > left + PAGE_VIEW_W - 1) {
            return;
        }
        if (y0 > top + PAGE_VIEW_H - 1) {
            return;
        }
        if (x1 > left + PAGE_VIEW_W) {
            x1 = (s16)(left + PAGE_VIEW_W);
        }
        if (y1 > top + PAGE_VIEW_H) {
            y1 = (s16)(top + PAGE_VIEW_H);
        }
    } else {
        if (x0 > left + PAGE_VIEW_W - 1) {
            return;
        }
        if (y0 > top + PAGE_VIEW_H - 1) {
            return;
        }
        if (x1 > left + PAGE_VIEW_W) {
            x1 = (s16)(left + PAGE_VIEW_W);
        }
        if (y1 > top + PAGE_VIEW_H) {
            y1 = (s16)(top + PAGE_VIEW_H);
        }
    }
    Prim_DrawRect(color, x0, y0, x1, y0 + thickness);
    Prim_DrawRect(color, x1 - thickness, y0, x1, y1);
    Prim_DrawRect(color, x0, y1 - thickness, x1, y1);
    Prim_DrawRect(color, x0, y0, x0 + thickness, y1);
}

/*
 * Draws a one-pixel frame in color around the rectangle from (x0, y0) to
 * (x1, y1), its corners kept inside the page's margins. It frames the
 * drop-down list, an image not yet loaded and the choice in the browser's
 * menu box.
 */
void Page_DrawThinFrame(s32 color, f32 x0, f32 y0, f32 x1, f32 y1)
{
    BrowserState *pageState = gBrowserState;

    if (x0 < pageState->marginLeft) {
        x0 = pageState->marginLeft;
    }
    if (y0 < pageState->marginTop) {
        y0 = pageState->marginTop;
    }
    if (x1 > 640 - pageState->marginRight) {
        x1 = (s16)(640 - pageState->marginRight);
    }
    if (y1 > 448 - pageState->marginBottom) {
        y1 = (s16)(448 - pageState->marginBottom);
    }
    Prim_DrawRect(color, x0, y0, x1, 1.0f + y0);
    Prim_DrawRect(color, x1 - 1.0f, y0, x1, y1);
    Prim_DrawRect(color, x0, y1 - 1.0f, x1, y1);
    Prim_DrawRect(color, x0, y0, 1.0f + x0, y1);
}

char *strchr(char *s, s32 c);

/*
 * Draws one line of text at (x, y) in the given size and colour, unless
 * the text is empty (or is this file's shared empty string) or the line
 * lies wholly above or below the screen. The colour's red, green and blue
 * are halved and its alpha cleared for Font_SetColor. Every percent sign
 * in the text is turned into a space, in place, because the text goes to
 * Page_Printf as a format. The first and third parameters (the third is
 * the bold flag of a text object) are never read.
 */
void Page_DrawTextLine(u8 unused, u32 color, u8 bold, u8 size, f32 x, f32 y, char *text)
{
    char *percent;

    if (*(u8 *)text == 0 || text == gPageEmptyText) {
        return;
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Font_SetSpacing(size, size);
    Font_SetPal(0);
    color = ((color & 0xFF) >> 1 & 0xFF) | (((color & 0xFF0000) >> 1 & 0xFF0000) | ((color & 0xFF00) >> 1 & 0xFF00));
    Font_SetColor(color);
    if (y + size > 0.0f && y < 448.0f) {
        Font_SetPos(x, y);
        for (;;) {
            percent = strchr(text, 37);
            if (percent == NULL) {
                break;
            }
            *percent = 32;
        }
        Page_Printf(text);
        Font_DrawAndClearPages();
    }
}

/*
 * Formats fmt and its arguments into a 1024-byte buffer (vsnprintf) and
 * prints the result with Font_Printf at the font's current position.
 * Page_DrawTextLine calls it.
 */
void Page_Printf(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, 1024, fmt, ap);
    Font_Printf("%s", buf);
}

/*
 * Draws a page object's background, before the objects themselves
 * (DlgPage_Draw calls it for each object, last to first), when it lies on
 * the page view: a table's colour and, with a border, a one-pixel bevel;
 * a cell's fill, or its border (a bevel of light and dark edges, or one
 * colour); a loaded table or cell background image, tiled. The light
 * edges are grey on a white page. The scroll x is declared before the
 * kind, and the colour local serves only the test and the first
 * rectangle: only so does the compiler assign registers as the game did.
 */
void Page_DrawObjBack(BrowserObj *pageObj)
{
    f32 innerY;
    f32 x0;
    f32 innerX;
    f32 y0;
    f32 x;
    f32 y;
    f32 x1;
    f32 y1;
    s32 scrollX;
    s32 kind;
    s32 color; /* the test and the first rectangle; the others read e->color again, as the game does */
    BrowserState *pageState;

    kind = pageObj->kind;
    switch (kind) {
    case PAGE_OBJ_CELL:
    case PAGE_OBJ_TABLE:
        break;
    case PAGE_OBJ_IMAGE:
        if (pageObj->sub == PAGE_IMG_LOADED && pageObj->active == PAGE_IMG_CELL_BACK) {
            break;
        }
    default:
        return;
    }
    pageState = gBrowserState;
    scrollX = pageState->x;
    x = pageObj->x;
    if (!(x <= (f32)(scrollX + 640 - pageState->marginRight))) {
        return;
    }
    if (x < (f32)scrollX - pageObj->w) {
        return;
    }
    y = pageObj->y;
    if (y < (f32)pageState->y - pageObj->h) {
        return;
    }
    if (!(y <= (f32)(pageState->y + 428))) {
        return;
    }
    x0 = x - (f32)scrollX;
    y0 = y - (f32)pageState->y;
    x1 = x0 + pageObj->w;
    y1 = y0 + pageObj->h;
    switch (kind) {
    case PAGE_OBJ_IMAGE:
        Page_TileImage(pageObj, pageObj->tex);
        return;
    case PAGE_OBJ_CELL:
        switch (pageObj->sub) {
        case PAGE_CELL_BORDER:
            if (pageObj->field == 0) {
                return;
            }
            color = pageObj->color;
            if (color <= 0) {
                if (pageState->bgColor == -1) {
                    Draw_RectClipped(BROWSER_BEVEL_LIGHT, x0, y0, x0 + (f32)(u32)pageObj->field, y1);
                    Draw_RectClipped(BROWSER_BEVEL_LIGHT, x0, y0, x1, y0 + (f32)(u32)pageObj->field);
                } else {
                    Draw_RectClipped(-1, x0, y0, x0 + (f32)(u32)pageObj->field, y1);
                    Draw_RectClipped(-1, x0, y0, x1, y0 + (f32)(u32)pageObj->field);
                }
                Draw_RectClipped(BROWSER_BEVEL_DARK, x1 - (f32)(u32)pageObj->field, y0 + (f32)(u32)pageObj->field, x1, y1);
                Draw_RectClipped(BROWSER_BEVEL_DARK, x0 + (f32)(u32)pageObj->field, y1 - (f32)(u32)pageObj->field, x1, y1);
                Draw_TriClipped(BROWSER_BEVEL_DARK, x0, y1, x0 + (f32)(u32)pageObj->field, y1, x0 + (f32)(u32)pageObj->field, y1 - (f32)(u32)pageObj->field);
                Draw_TriClipped(BROWSER_BEVEL_DARK, x1, y0, x1, y0 + (f32)(u32)pageObj->field, x1 - (f32)(u32)pageObj->field, y0 + (f32)(u32)pageObj->field);
                return;
            } else {
                Draw_RectClipped(color | 0xFF000000, x0, y0, x0 + (f32)(u32)pageObj->field, y1);
                Draw_RectClipped(pageObj->color | 0xFF000000, x0, y0, x1, y0 + (f32)(u32)pageObj->field);
                Draw_RectClipped(pageObj->color | 0xFF000000, x1 - (f32)(u32)pageObj->field - 1.0f, y0 + (f32)(u32)pageObj->field, x1, y1);
                Draw_RectClipped(pageObj->color | 0xFF000000, x0 + (f32)(u32)pageObj->field, y1 - (f32)(u32)pageObj->field - 1.0f, x1, y1);
            }
            return;
        case PAGE_CELL_FILL:
            if (pageObj->color >= 0) {
                Draw_RectClipped(pageObj->color | 0xFF000000, x0, y0, x1, y1);
                return;
            }
            break;
        }
        break;
    case PAGE_OBJ_TABLE:
        if (pageObj->color >= 0) {
            Draw_RectClipped(pageObj->color | 0xFF000000, x0, y0, x1, y1);
        }
        if (pageObj->field != 0) {
            innerY = 1.0f + y0;
            innerX = 1.0f + x0;
            Draw_HLineClipped(BROWSER_BEVEL_DARK, innerX, innerY, x0 - 1.0f + pageObj->w, innerY);
            Draw_VLineClipped(BROWSER_BEVEL_DARK, innerX, innerY, innerX, y0 - 1.0f + pageObj->h);
            if (gBrowserState->bgColor == -1) {
                Draw_VLineClipped(BROWSER_BEVEL_LIGHT, x1, y0, x1, y1);
                Draw_HLineClipped(BROWSER_BEVEL_LIGHT, x0, y1, x1, y1);
            } else {
                {
                    f32 rightX = x0 - 1.0f + pageObj->w;
                    Draw_VLineClipped(-1, rightX, y0 - 1.0f, rightX, y0 - 1.0f + pageObj->h);
                }
                {
                    f32 bottomY = y0 - 1.0f + pageObj->h;
                    Draw_HLineClipped(-1, x0 - 1.0f, bottomY, x0 - 1.0f + pageObj->w, bottomY);
                }
            }
        }
        break;
    }
}


/*
 * Draws one page object if it lies on the page view (text may start up
 * to 40 pixels above it) and sets its visible byte to say so. By kind:
 * text; links and underlined text (underlined with
 * PAGE_STYLE_UNDERLINE_LINKS); struck-out text; buttons (drawn pressed until the press timer
 * ends); text and password boxes (the value cut to the box, or
 * asterisks); the closed drop-down list; rules; radio buttons and check
 * boxes; images (the texture, or a frame with the image's text while it
 * isn't loaded). Page_DrawObjBack draws tables and cells.
 */
void Page_DrawObj(BrowserObj *pageObj)
{
    f32 x0;
    f32 y0;
    f32 x1;
    f32 y1;
    f32 dotRight;
    f32 strikeY;
    u8 size;
    u16 fitLen;
    s32 selectState;
    char buf[640];

    pageObj->visible = 0;
    switch (pageObj->kind) {
    case PAGE_OBJ_IMAGE:
        if (pageObj->active == PAGE_IMG_CELL_BACK) {
            return;
        }
        if (pageObj->active > PAGE_IMG_BODY_BACK) {
            BrowserState *pageState = gBrowserState;
            s32 scrollX = pageState->x;

            if (!(pageObj->x <= (f32)(scrollX + 640 - pageState->marginRight))) {
                return;
            }
            if (pageObj->x < (f32)scrollX - pageObj->w) {
                return;
            }
            if (pageObj->y < (f32)pageState->y - pageObj->h) {
                return;
            }
            if (!(pageObj->y <= (f32)(pageState->y + 428))) {
                return;
            }
        }
        break;
    case PAGE_OBJ_CELL:
    case PAGE_OBJ_TABLE:
        return;
    default: {
        BrowserState *pageState = gBrowserState;
        s32 scrollX = pageState->x;

        if (!(pageObj->x <= (f32)(scrollX + 640 - pageState->marginRight))) {
            return;
        }
        if (pageObj->x < (f32)scrollX - pageObj->w) {
            return;
        }
        if (pageObj->y < (f32)(pageState->y - 40)) {
            return;
        }
        if (!(pageObj->y <= (f32)(pageState->y + 428))) {
            return;
        }
        break;
    }
    }
    pageObj->visible = 1;
    x0 = pageObj->x - (f32)gBrowserState->x;
    y0 = pageObj->y - (f32)gBrowserState->y;
    x1 = x0 + pageObj->w;
    y1 = y0 + pageObj->h;
    switch (pageObj->kind) {
    case PAGE_OBJ_TEXT:
        Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, x0, y0, pageObj->s1);
        break;
    case PAGE_OBJ_LINK:
    case PAGE_OBJ_UNDERLINED:
        Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, x0, y0, pageObj->s2);
        if ((gBrowserState->style & PAGE_STYLE_UNDERLINE_LINKS) >> 7) {
            Draw_RectClipped(pageObj->color | 0xFF000000, x0, y1 - 2.0f, x1, y1);
        }
        break;
    case PAGE_OBJ_SUBMIT:
    case PAGE_OBJ_RESET:
    case PAGE_OBJ_BUTTON:
        if (gBrowserWork->pressTimer == 0) {
            pageObj->sub = 0;
        }
        switch (pageObj->sub) {
        case 0:
            Page_DrawWidget(PAGE_WIDGET_BUTTON, x0, y0, pageObj->w, pageObj->h, 0);
            if (strlen(pageObj->s1) == 1) {
                Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 6.0f + x0, 1.0f + y0, pageObj->s1);
            } else {
                Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 3.0f + x0, 1.0f + y0, pageObj->s1);
            }
            break;
        case 1:
            Page_DrawWidget(PAGE_WIDGET_BUTTON_PRESSED, x0, y0, pageObj->w, pageObj->h, 0);
            Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 3.0f + x0, 2.0f + y0, pageObj->s1);
            break;
        }
        break;
    case PAGE_OBJ_TEXT_INPUT:
        Page_DrawWidget(PAGE_WIDGET_TEXT_BOX, x0, y0, pageObj->w, pageObj->h, 0);
        if (strlen(pageObj->s2) == 0) {
            break;
        }
        size = pageObj->size;
        fitLen = pageObj->w;
        if (fitLen < (u32)(size * (strlen(pageObj->s2) + 2)) >> 1) {
            fitLen = fitLen / (size / 2) - 2;
            memcpy(buf, pageObj->s2, fitLen);
            buf[fitLen] = 0;
            Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 4.0f + x0, 3.0f + y0, buf);
        } else {
            Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, size, 4.0f + x0, 3.0f + y0, pageObj->s2);
        }
        break;
    case PAGE_OBJ_PASSWORD:
        Page_DrawWidget(PAGE_WIDGET_TEXT_BOX, x0, y0, pageObj->w, pageObj->h, 0);
        if (strlen(pageObj->s2) != 0) {
            Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 4.0f + x0, 3.0f + y0, "********");
        }
        break;
    case PAGE_OBJ_SELECT:
        selectState = pageObj->active;
        if ((u32)(selectState - PAGE_SELECT_CLOSED) < 2) {
            if (selectState == PAGE_SELECT_CLOSED) {
                Page_DrawWidget(PAGE_WIDGET_SELECT, x0, y0, pageObj->w, pageObj->h, 0);
            } else {
                Page_DrawWidget(PAGE_WIDGET_SELECT_PRESSED, x0, y0, pageObj->w, pageObj->h, 0);
                if (gBrowserWork->pressTimer == 0) {
                    pageObj->active = PAGE_SELECT_CLOSED;
                }
            }
            Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 4.0f + x0, 2.0f + y0, pageObj->s1);
        }
        break;
    case PAGE_OBJ_RULE:
        switch ((s16)pageObj->h) {
        case 2:
            if ((s16)pageObj->w == 2) {
                Draw_RectClipped(BROWSER_BLACK, x0, y0, 1.0f + x0, 1.0f + y0);
            } else {
                Draw_HLineClipped(BROWSER_BLACK, x0, y0, x1, y0);
            }
            break;
        case 4:
            if ((s16)pageObj->w == 2) {
                dotRight = 1.0f + x0;
                Draw_RectClipped(BROWSER_BLACK, x0, y0, dotRight, 1.0f + y0);
                Draw_RectClipped(BROWSER_BEVEL_LIGHT, x0, 2.0f + y0, dotRight, 3.0f + y0);
            } else {
                Draw_HLineClipped(BROWSER_BLACK, x0, y0, x1 - 2.0f, y0);
                Draw_HLineClipped(BROWSER_BEVEL_LIGHT, x0, y1 - 2.0f, x1 - 2.0f, y1 - 2.0f);
            }
            break;
        default:
            if ((s16)pageObj->w == 2) {
                Draw_RectClipped(BROWSER_BEVEL_LIGHT, x0, y0, 1.0f + x0, y1 - 2.0f);
            } else {
                Draw_HLineClipped(BROWSER_BLACK, x0, y0, x1, y0);
                Draw_HLineClipped(BROWSER_BEVEL_LIGHT, x0, y1 - 2.0f, x1, y1 - 2.0f);
                Draw_VLineClipped(BROWSER_BEVEL_LIGHT, x1, y0, x1, y1 - 2.0f);
                Draw_VLineClipped(BROWSER_BLACK, x0, y0, x0, y1 - 2.0f);
            }
            break;
        }
        break;
    case PAGE_OBJ_RADIO:
        if (pageObj->sub == 1) {
            Page_DrawMark(PAGE_WIDGET_RADIO_ON, x0, y0);
        } else {
            Page_DrawMark(PAGE_WIDGET_RADIO_OFF, x0, y0);
        }
        break;
    case PAGE_OBJ_CHECKBOX:
        if (pageObj->sub == 1) {
            Page_DrawMark(PAGE_WIDGET_CHECK_ON, x0, y0);
        } else {
            Page_DrawMark(PAGE_WIDGET_CHECK_OFF, x0, y0);
        }
        break;
    case PAGE_OBJ_IMAGE:
        if (pageObj->sub == PAGE_IMG_LOADED && gBrowserWork->texHandle[pageObj->tex] != 0) {
            if (*(u8 *)pageObj->s2 != 0) {
                Page_DrawImage(pageObj->tex, x0, y0, pageObj->w, pageObj->h, gBrowserWork->texW[pageObj->tex],
                              gBrowserWork->texH[pageObj->tex]);
            }
        } else if (!(pageObj->field & 0x80)) {
            Page_DrawThinFrame(BROWSER_BLACK, x0, y0, x1 - 1.0f, y1 - 1.0f);
            Draw_VLineClipped(BROWSER_BEVEL_LIGHT, x1 - 2.0f, y0, x1 - 2.0f, y1);
            Draw_HLineClipped(BROWSER_BEVEL_LIGHT, x0, y1 - 2.0f, x1, y1 - 2.0f);
            if (pageObj->s3 != 0) {
                Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, 4.0f + x0, 2.0f + y0, pageObj->s3);
            }
        }
        break;
    case PAGE_OBJ_STRUCK:
        Page_DrawTextLine(pageObj->unk05C, pageObj->color, pageObj->field, pageObj->size, x0, y0, pageObj->s2);
        strikeY = y1 - (f32)(pageObj->size / 2);
        Draw_RectClipped(pageObj->color | 0xFF000000, x0, strikeY - 1.0f, x1, 2.0f + strikeY);
        if (strlen(pageObj->s1) != 0) {
            Draw_RectClipped(pageObj->color | 0xFF000000, x0, y1 - 2.0f, x1, y1);
        }
        break;
    }
}

/* The drop-down list's draw needs the compiler's optimization level 4. */
#pragma optimization_level 4
/*
 * Returns the browser's work area (gBrowserWork). The drop-down list
 * reads the page objects through it: only through this inline function
 * does the compiler add the index before it loads the work area's
 * address, as the game's code does.
 */
static inline BrowserWork *Browser_Work(void)
{
    return gBrowserWork;
}

/*
 * Draws the open drop-down list of the select object at listObjIdx: it
 * places the scroll thumb from listTop (clamping both at the ends), draws
 * the box above or below the field (listUp), a blue bar on the pointer's
 * row, a scroll bar when there are over 10 options, and up to 10 options
 * from listTop, the chosen one on a green bar. Compiled at the compiler's
 * optimization level 4, as the game's function was. Kept for the match:
 * gBrowserSub->listCount read again in the division, the (u16)k casts, n an s32
 * declared last, and j and k reused for the option walk and line count.
 */
void Page_DrawDropList(s32 listObjIdx)
{
    BrowserObj *selectObj;
    BrowserObj *option;
    u16 i;
    u16 j;
    u16 k;
    s32 rowCount;
    u16 selectIdx;
    f32 x0;
    f32 y0;
    f32 x1;
    s16 pointerX;
    s16 pointerY;
    s32 optionCount;

    for (i = 0; i < PAGE_OBJ_COUNT; i++) {
        if (Browser_Work()->obj[i] == NULL || Browser_Work()->obj[i]->active == 0) {
            break;
        }
        if (Browser_Work()->obj[i]->kind == PAGE_OBJ_SELECT && gBrowserSub->activeObj == i) {
            selectIdx = i;
            optionCount = gBrowserSub->listCount;
            if (!optionCount) {
                return;
            }
            for (j = selectIdx + 1; j < PAGE_OBJ_COUNT; j++) {
                if (Browser_Work()->obj[j]->kind != PAGE_OBJ_SELECT || !(Browser_Work()->obj[j]->tex < 11) || Browser_Work()->obj[j]->tex == 0) {
                    break;
                }
            }
            gBrowserSub->thumbTop = (gBrowserSub->listTop - 1) * 144 / gBrowserSub->listCount + 18;
            gBrowserSub->thumbBottom = 1440 / gBrowserSub->listCount + gBrowserSub->thumbTop;
            gBrowserSub->thumbLen = gBrowserSub->thumbBottom - gBrowserSub->thumbTop;
            if (gBrowserSub->thumbBottom > 180) {
                gBrowserSub->thumbBottom = 180;
            }
            if (gBrowserSub->listCount > DROPLIST_ROWS) {
                if (gBrowserSub->thumbTop < 18) {
                    gBrowserSub->thumbTop = 18;
                    gBrowserSub->thumbBottom = gBrowserSub->thumbLen + 18;
                    gBrowserSub->listTop = 1;
                }
                if (gBrowserSub->thumbBottom > 162) {
                    gBrowserSub->thumbTop = 162 - gBrowserSub->thumbLen;
                    gBrowserSub->thumbBottom = 162;
                    gBrowserSub->listTop = gBrowserSub->listCount - 9;
                }
            }
            selectObj = Browser_Work()->obj[i];
            x0 = selectObj->x - (f32)gBrowserState->x;
            y0 = selectObj->y - (f32)gBrowserState->y;
            x1 = x0 + selectObj->w;
            if (gBrowserSub->listUp != 0) {
                Draw_RectClipped(-1, x0, y0, x1, y0 - (f32)(Browser_Work()->obj[j - 1]->tex * DROPLIST_ROW_H));
                Page_DrawThinFrame(BROWSER_BLACK, x0, y0, x1, y0 - (f32)(Browser_Work()->obj[j - 1]->tex * DROPLIST_ROW_H));
                pointerX = gBrowserSub->cursorX;
                if (!((f32)pointerX <= x0) && ((f32)pointerX <= x1 - 16.0f || (gBrowserSub->listCount < 11 && (f32)pointerX < x1))) {
                    if ((f32)gBrowserSub->cursorY < y0) {
                        rowCount = Browser_Work()->obj[j - 1]->tex + 1;
                        if (!((f32)gBrowserSub->cursorY <= y0 - (f32)(rowCount * DROPLIST_ROW_H))) {
                            for (k = 0; k < Browser_Work()->obj[j - 1]->tex + 1; k++) {
                                if (!((f32)gBrowserSub->cursorY <= y0 - (f32)(k * DROPLIST_ROW_H)) && (f32)gBrowserSub->cursorY <= y0 - (f32)((k - 1) * DROPLIST_ROW_H)) {
                                    Draw_RectClipped(BROWSER_LIST_HOVER, x0, y0 - (f32)(k * DROPLIST_ROW_H), x1, y0 - (f32)((k - 1) * DROPLIST_ROW_H));
                                    break;
                                }
                            }
                        }
                    }
                }
                if (optionCount > DROPLIST_ROWS) {
                    if (gBrowserSub->moveKind != BMOVE_LIST_UP) {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_UP, x1 - 16.0f, y0 - 180.0f, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_UP_PRESSED, x1 - 16.0f, y0 - 180.0f, 16.0f, 20.0f, 0);
                        if (gBrowserWork->pressTimer == 0) {
                            gBrowserSub->moveKind = 0;
                        }
                    }
                    Page_DrawWidget(PAGE_WIDGET_VTRACK, x1 - 16.0f, y0 - 20.0f, 16.0f, -140.0f, 0);
                    if (gBrowserSub->moveKind != BMOVE_LIST_DOWN) {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN, x1 - 16.0f, y0 - 20.0f, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN_PRESSED, x1 - 16.0f, y0 - 20.0f, 16.0f, 20.0f, 0);
                        if (gBrowserWork->pressTimer == 0) {
                            gBrowserSub->moveKind = 0;
                        }
                    }
                    Page_DrawWidget(PAGE_WIDGET_VTHUMB, x1 - 16.0f, y0 - 180.0f + (f32)gBrowserSub->thumbTop, 16.0f, (f32)(gBrowserSub->thumbBottom - gBrowserSub->thumbTop), 0);
                }
            } else {
                Draw_RectClipped(-1, x0, 20.0f + y0, x1, (20.0f + y0) + (f32)(Browser_Work()->obj[j - 1]->tex * DROPLIST_ROW_H));
                Page_DrawThinFrame(BROWSER_BLACK, x0, 4.0f + (16.0f + y0), x1, (20.0f + y0) + (f32)(Browser_Work()->obj[j - 1]->tex * DROPLIST_ROW_H));
                pointerX = gBrowserSub->cursorX;
                if (!((f32)pointerX <= x0) && (f32)pointerX < x1 - 16.0f) {
                    if (!((f32)gBrowserSub->cursorY <= (20.0f + y0))) {
                        rowCount = Browser_Work()->obj[j - 1]->tex;
                        if ((f32)gBrowserSub->cursorY < (20.0f + y0) + (f32)(rowCount * DROPLIST_ROW_H)) {
                            for (k = 0; k < Browser_Work()->obj[j - 1]->tex; k++) {
                                if (!((f32)gBrowserSub->cursorY <= (20.0f + y0) + (f32)(k * DROPLIST_ROW_H)) && (f32)gBrowserSub->cursorY < (20.0f + y0) + (f32)((k + 1) * DROPLIST_ROW_H)) {
                                    Draw_RectClipped(BROWSER_LIST_HOVER, x0, (20.0f + y0) + (f32)(k * DROPLIST_ROW_H), x1, (20.0f + y0) + (f32)((k + 1) * DROPLIST_ROW_H));
                                    break;
                                }
                            }
                        }
                    }
                }
                if (optionCount > DROPLIST_ROWS) {
                    if (gBrowserSub->moveKind != BMOVE_LIST_UP) {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_UP, x1 - 16.0f, 20.0f + y0, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_UP_PRESSED, x1 - 16.0f, 20.0f + y0, 16.0f, 20.0f, 0);
                        if (gBrowserWork->pressTimer == 0) {
                            gBrowserSub->moveKind = 0;
                        }
                    }
                    Page_DrawWidget(PAGE_WIDGET_VTRACK, x1 - 16.0f, 20.0f + (20.0f + y0), 16.0f, 160.0f, 0);
                    if (gBrowserSub->moveKind != BMOVE_LIST_DOWN) {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN, x1 - 16.0f, 180.0f + (20.0f + y0) - 20.0f, 16.0f, 20.0f, 0);
                    } else {
                        Page_DrawWidget(PAGE_WIDGET_ARROW_DOWN_PRESSED, x1 - 16.0f, 180.0f + (20.0f + y0) - 20.0f, 16.0f, 20.0f, 0);
                        if (gBrowserWork->pressTimer == 0) {
                            gBrowserSub->moveKind = 0;
                        }
                    }
                    Page_DrawWidget(PAGE_WIDGET_VTHUMB, x1 - 16.0f, (20.0f + y0) + (f32)gBrowserSub->thumbTop, 16.0f, (f32)(gBrowserSub->thumbBottom - gBrowserSub->thumbTop), 0);
                }
            }
            k = 0;
            for (j = selectIdx + gBrowserSub->listTop; j < PAGE_OBJ_COUNT; j++) {
                option = Browser_Work()->obj[j];
                if (option->kind != PAGE_OBJ_SELECT) {
                    return;
                }
                if (option->tex == 0) {
                    return;
                }
                k++;
                if (!(k < DROPLIST_ROWS + 1)) {
                    return;
                }
                if (gBrowserSub->listUp != 0) {
                    if (optionCount < DROPLIST_ROWS + 1) {
                        if (option->tex == Browser_Work()->obj[listObjIdx]->sub) {
                            Draw_RectClipped(BROWSER_LIST_CHOSEN, x0, y0 - (f32)((optionCount - k + 1) * DROPLIST_ROW_H), x0 + Browser_Work()->obj[listObjIdx]->w, 18.0f + (y0 - (f32)((optionCount - k + 1) * DROPLIST_ROW_H)));
                        }
                        Page_DrawTextLine(21, BROWSER_BLACK, 0, Browser_Work()->obj[j]->size, 4.0f + x0,
                                          2.0f + y0 - (f32)((optionCount - (u16)k + 1) * DROPLIST_ROW_H), Browser_Work()->obj[j]->s1);
                    } else {
                        if (option->tex == Browser_Work()->obj[listObjIdx]->sub) {
                            Draw_RectClipped(BROWSER_LIST_CHOSEN, x0, y0 - (f32)((11 - k) * DROPLIST_ROW_H), x0 + Browser_Work()->obj[listObjIdx]->w - 16.0f, 18.0f + (y0 - (f32)((11 - k) * DROPLIST_ROW_H)));
                        }
                        Page_DrawTextLine(21, BROWSER_BLACK, 0, Browser_Work()->obj[j]->size, 4.0f + x0,
                                          2.0f + y0 - (f32)((11 - (u16)k) * DROPLIST_ROW_H), Browser_Work()->obj[j]->s1);
                    }
                } else {
                    if (optionCount < DROPLIST_ROWS + 1) {
                        if (option->tex == Browser_Work()->obj[listObjIdx]->sub) {
                            Draw_RectClipped(BROWSER_LIST_CHOSEN, x0, y0 + (f32)(k * DROPLIST_ROW_H), x0 + Browser_Work()->obj[listObjIdx]->w, 2.0f + (18.0f + (y0 + (f32)(k * DROPLIST_ROW_H))));
                        }
                    } else {
                        if (option->tex == Browser_Work()->obj[listObjIdx]->sub) {
                            Draw_RectClipped(BROWSER_LIST_CHOSEN, x0, y0 + (f32)(k * DROPLIST_ROW_H), x0 + Browser_Work()->obj[listObjIdx]->w - 16.0f, 2.0f + (18.0f + (y0 + (f32)(k * DROPLIST_ROW_H))));
                        }
                    }
                    Page_DrawTextLine(21, BROWSER_BLACK, 0, Browser_Work()->obj[j]->size, 4.0f + x0, 2.0f + y0 + (f32)(k * DROPLIST_ROW_H),
                                      Browser_Work()->obj[j]->s1);
                }
            }
            break;
        }
    }
}
#pragma optimization_level reset

/*
 * Draws the highlight on the focused page object (BrowserWork.focus).
 * cursorObj is the cursor object: the highlight is red while its blink
 * rises (glow above 0), else white. An image button gets its
 * highlight widget (PAGE_WIDGET_IMGBTN_1 to _4, by its kind), then the overlay again
 * when the page's styleMode is set; any other object gets a 3-pixel frame
 * (Page_DrawFrame) just outside it. The switch reads the focused object
 * again from gBrowserWork rather than from the local focusObj: from the
 * local, the compiler's code differs.
 */
void Page_DrawFocus(BrowserObj *cursorObj)
{
    s32 color;
    BrowserObj *focusObj;
    u8 kind;
    s8 widgetKind;

    color = cursorObj->glow > 0 ? BROWSER_CURSOR_RED : -1;
    focusObj = gBrowserWork->obj[gBrowserWork->focus];
    kind = focusObj->kind;
    if (kind == PAGE_OBJ_LINK || kind == PAGE_OBJ_CHECKBOX || kind == PAGE_OBJ_RADIO || kind == PAGE_OBJ_SELECT || kind == PAGE_OBJ_PASSWORD || kind == PAGE_OBJ_TEXT_INPUT || kind == PAGE_OBJ_BUTTON
        || kind == PAGE_OBJ_RESET || kind == PAGE_OBJ_SUBMIT || kind == PAGE_OBJ_UNDERLINED || kind == PAGE_OBJ_STRUCK) {
        goto frame;
    }
    switch (kind) {
    case PAGE_OBJ_IMAGE:
        switch (gBrowserWork->obj[gBrowserWork->focus]->field & 0xF) {
        case 1:
            if ((focusObj->field & 0x10) == 16) {
                widgetKind = PAGE_WIDGET_IMGBTN_3;
            } else {
                widgetKind = PAGE_WIDGET_IMGBTN_1;
            }
            break;
        case 2:
            widgetKind = PAGE_WIDGET_IMGBTN_2;
            break;
        case 3:
            widgetKind = PAGE_WIDGET_IMGBTN_3;
            break;
        case 4:
            widgetKind = PAGE_WIDGET_IMGBTN_4;
            break;
        default:
            widgetKind = 0;
            break;
        }
        if (widgetKind) {
            Page_DrawWidget(widgetKind, focusObj->x - gBrowserState->x, focusObj->y - gBrowserState->y, 0.0f, 0.0f, color);
            if (gBrowserState->styleMode != 0) {
                Browser_DrawOverlay();
            }
            return;
        }
        break;
    }
frame:
    Page_DrawFrame(color, 3, focusObj->x - gBrowserState->x - 1.0f, focusObj->y - gBrowserState->y - 1.0f,
                   3.0f + (focusObj->x + focusObj->w - gBrowserState->x), 3.0f + (focusObj->y + focusObj->h - gBrowserState->y));
}

extern s32 gTextOff;
/*
 * Draws the message box of dialog kinds 4, 5 and 6 (dlgBoxKind): a window
 * with one message and, while the dialog asks (dlgMode 3), a glowing bar
 * on the choice (dlgCursor). Kind 4 asks, then (dlgMode 4) shows the net
 * mode's message, or after kind 6's question the message for errorKind;
 * kind 5 only asks; kind 6 asks, then shows the error. Messages come from
 * the lobby's message pages (LobbyMsg_DrawPage5) or text group 5. Draws
 * nothing for other kinds or an unknown error.
 */
void BrowserDlg_DrawMsgBox(BrowserObj *dlgObj)
{
    s8 msg = 0;
    s8 row = 8;
    s32 highlight = 0;
    s32 useMsgPage = 0;
    s32 line;
    s32 glowLeft;
    s32 w;
    s32 glowTop;
    s32 glowRight;
    s32 glowBottom;
    s16 k;

    switch (gBrowserWork->dlgBoxKind) {
    case BDLG_KIND_YES_NO:
        msg = 3;
        highlight = 1;
        row = 9;
        break;
    case BDLG_KIND_MESSAGE:
        switch (gBrowserSub->dlgMode) {
        case BDLG_MODE_CHOICE:
            highlight = 1;
            gBrowserWork->dlgQuestion = 1;
            row = 7;
            break;
        case BDLG_MODE_SAVE_LOGIN:
            switch ((s8)gBrowserWork->dlgQuestion) {
            case 1:
            default:
                msg = gNetMode.cardMsg;
                useMsgPage = 1;
                highlight = 0;
                break;
            case 2:
                switch ((s8)gBrowserWork->errorKind) {
                case 0:
                case 1:
                case 2:
                case 10:
                case 20:
                case 21:
                case 22:
                case 23:
                case 30:
                    useMsgPage = 1;
                    msg = 1;
                    break;
                case 40:
                    msg = 7;
                    break;
                case 41:
                    msg = 9;
                    break;
                case -10:
                    msg = 4;
                    useMsgPage = 1;
                    break;
                case -21:
                    useMsgPage = 1;
                    msg = 5;
                    break;
                case -23:
                    msg = 6;
                    useMsgPage = 1;
                    break;
                default:
                    return;
                }
                highlight = 0;
                break;
            }
            break;
        }
        break;
    case BDLG_KIND_CLEAR_LOGIN:
        switch (gBrowserSub->dlgMode) {
        case BDLG_MODE_CHOICE:
            msg = 4;
            highlight = 1;
            gBrowserWork->dlgQuestion = 2;
            break;
        case BDLG_MODE_SAVE_LOGIN:
            switch ((s8)gBrowserWork->errorKind) {
            case 0:
            case 1:
            case 2:
            case 10:
            case 20:
            case 21:
            case 22:
            case 23:
            case 30:
                useMsgPage = 1;
                msg = 1;
                break;
            }
            highlight = 0;
            break;
        }
        row = 10;
        break;
    default:
        return;
    }
    BrowserDlg_DrawWindow(dlgObj->x, dlgObj->y - 40.0f, dlgObj->x + dlgObj->w, dlgObj->y + dlgObj->h);
    if (highlight != 0) {
        glowLeft = 90.0f + dlgObj->x;
        line = row + gBrowserSub->dlgCursor;
        glowTop = dlgObj->y + (f32)((line - 2) * 20);
        glowRight = 10.0f + (420.0f + dlgObj->x);
        glowBottom = dlgObj->y + (f32)((line - 1) * 20);
        Menu_DrawGlowBar(glowLeft + (glowRight - glowLeft) / 2, glowTop, (glowRight - glowLeft) + 80, glowBottom - glowTop, 0xFF20C0C0);
    }
    if (useMsgPage == 0) {
        Page_DrawTextLine(20, -1, 0, 20, 20.0f + dlgObj->x, dlgObj->y, Browser_GetText(BTEXT_NET_SAVE, msg));
    } else {
        Text_Reset();
        Font_SetHalfWidth(1);
        LobbyMsg_DrawPage5(msg);
        for (k = 0; k < 7; k++) {
            Text_DrawIcons(k);
            Font_DrawPage(k);
        }
        gTextOff = 1;
    }
    NetModeMsg_Flush(1);
}

s32 Str_Length(char *s);
/*
 * Draws the two-button dialog: a window, the question (text group 7, text
 * 0) centred, and two buttons labelled with texts 1 and 2. The button
 * under the cursor (dlgCursor 1 or 2) is drawn again as the cursor
 * button, red while the dialog's glow brightens, else white.
 */
void BrowserDlg_DrawYesNo(BrowserObj *dlgObj)
{
    s16 buttonX;
    u32 color;
    f32 textX;

    BrowserDlg_DrawWindow(dlgObj->x, dlgObj->y, dlgObj->x + dlgObj->w, dlgObj->y + dlgObj->h);
    Prim_DrawSprite(-1, PageTex_Get(0), 49.0f + dlgObj->x, 145.0f + dlgObj->y, 84, 32, 42, 0, 128, 32);
    Prim_DrawSprite(-1, PageTex_Get(0), 157.0f + dlgObj->x, 145.0f + dlgObj->y, 84, 32, 42, 0, 128, 32);
    textX = dlgObj->x + (dlgObj->w - (f32)(Str_Length(Browser_GetText(BTEXT_REGISTER, 0)) * 20 / 2)) / 2.0f;
    Page_DrawTextLine(20, -1, 0, 20, textX, 85.0f + dlgObj->y, Browser_GetText(BTEXT_REGISTER, 0));
    textX = 49.0f + dlgObj->x + (f32)((84 - Str_Length(Browser_GetText(BTEXT_REGISTER, 1)) * 20 / 2) / 2);
    Page_DrawTextLine(20, -1, 1, 20, textX, 6.0f + (145.0f + dlgObj->y), Browser_GetText(BTEXT_REGISTER, 1));
    textX = 157.0f + dlgObj->x + (f32)((84 - Str_Length(Browser_GetText(BTEXT_REGISTER, 2)) * 20 / 2) / 2);
    Page_DrawTextLine(20, -1, 1, 20, textX, 6.0f + (145.0f + dlgObj->y), Browser_GetText(BTEXT_REGISTER, 2));
    switch (gBrowserSub->dlgCursor) {
    case 1:
        buttonX = 49.0f + dlgObj->x;
        break;
    case 2:
        buttonX = 157.0f + dlgObj->x;
        break;
    }
    color = -1;
    if (dlgObj->glowDir == GLOW_RISING) {
        color = BROWSER_CURSOR_RED;
    }
    Prim_DrawSprite(color, PageTex_Get(0), buttonX, 145.0f + dlgObj->y, 84, 32, 42, 32, 128, 64);
    TextObj_Reset();
}


/*
 * Draws the one-button dialog: a window, a message (text group 7, text 3)
 * and one button labelled with text 4, drawn again as the cursor button
 * (red while the glow brightens). The message is centred by the width of
 * text 0, not text 3, as in the game's code.
 */
void BrowserDlg_DrawOk(BrowserObj *dlgObj)
{
    s16 buttonX;
    u32 color;
    f32 textX;

    BrowserDlg_DrawWindow(dlgObj->x, dlgObj->y, dlgObj->x + dlgObj->w, dlgObj->y + dlgObj->h);
    Prim_DrawSprite(-1, PageTex_Get(0), dlgObj->x + (dlgObj->w - 84.0f) / 2.0f, 145.0f + dlgObj->y, 84, 32, 42, 0, 128, 32);
    textX = dlgObj->x + (dlgObj->w - (f32)(Str_Length(Browser_GetText(BTEXT_REGISTER, 0)) * 20 / 2)) / 2.0f;
    Page_DrawTextLine(20, -1, 0, 20, textX, 85.0f + dlgObj->y, Browser_GetText(BTEXT_REGISTER, 3));
    textX = dlgObj->x + (dlgObj->w - (f32)(Str_Length(Browser_GetText(BTEXT_REGISTER, 4)) * 20 / 2)) / 2.0f;
    Page_DrawTextLine(20, -1, 1, 20, textX, 6.0f + (145.0f + dlgObj->y), Browser_GetText(BTEXT_REGISTER, 4));
    switch (gBrowserSub->dlgCursor) {
    case 1:
        buttonX = dlgObj->x + (dlgObj->w - 84.0f) / 2.0f;
        break;
    }
    color = -1;
    if (dlgObj->glowDir == GLOW_RISING) {
        color = BROWSER_CURSOR_RED;
    }
    Prim_DrawSprite(color, PageTex_Get(0), buttonX, 145.0f + dlgObj->y, 84, 32, 42, 32, 128, 64);
    TextObj_Reset();
}

s32 strcmp(char *a, char *b);

/*
 * Draws the dialog of text group 8. While the browser is ending
 * (endReason above 0) without a network error and the page is the
 * portal's top page, it shows a message (text 2) and two buttons (texts 3
 * and 4); otherwise a message (text 0) and one button (text 1). The cursor
 * button is drawn as in BrowserDlg_DrawYesNo.
 */
void BrowserDlg_DrawPortalTop(BrowserObj *dlgObj)
{
    s16 buttonX;
    u32 color;
    f32 textX;

    if ((s8)gBrowserWork->endReason > 0 && gBrowserWork->netError == 0
        && strcmp(gBrowserState->url, TXT(0x00838EA0) /* the portal's top page */) == 0) {
        BrowserDlg_DrawWindow(dlgObj->x, dlgObj->y, dlgObj->x + dlgObj->w, dlgObj->y + dlgObj->h);
        Prim_DrawSprite(-1, PageTex_Get(0), 80.0f + dlgObj->x, 165.0f + dlgObj->y, 84, 32, 42, 0, 128, 32);
        Prim_DrawSprite(-1, PageTex_Get(0), 216.0f + dlgObj->x, 165.0f + dlgObj->y, 84, 32, 42, 0, 128, 32);
        Page_DrawTextLine(20, -1, 0, 20, 40.0f + dlgObj->x, 80.0f + dlgObj->y, Browser_GetText(BTEXT_COMM_ERROR, 2));
        textX = 80.0f + dlgObj->x + (f32)((84 - Str_Length(Browser_GetText(BTEXT_COMM_ERROR, 3)) * 20 / 2) / 2);
        Page_DrawTextLine(20, -1, 1, 20, textX, 6.0f + (165.0f + dlgObj->y), Browser_GetText(BTEXT_COMM_ERROR, 3));
        textX = 216.0f + dlgObj->x + (f32)((84 - Str_Length(Browser_GetText(BTEXT_COMM_ERROR, 4)) * 20 / 2) / 2);
        Page_DrawTextLine(20, -1, 1, 20, textX, 6.0f + (165.0f + dlgObj->y), Browser_GetText(BTEXT_COMM_ERROR, 4));
        switch (gBrowserSub->dlgCursor) {
        case 1:
            buttonX = 80.0f + dlgObj->x;
            break;
        case 2:
            buttonX = 216.0f + dlgObj->x;
            break;
        }
        color = -1;
        if (dlgObj->glowDir == GLOW_RISING) {
            color = BROWSER_CURSOR_RED;
        }
        Prim_DrawSprite(color, PageTex_Get(0), buttonX, 165.0f + dlgObj->y, 84, 32, 42, 32, 128, 64);
        TextObj_Reset();
        return;
    }
    BrowserDlg_DrawWindow(dlgObj->x, dlgObj->y, dlgObj->x + dlgObj->w, dlgObj->y + dlgObj->h);
    Prim_DrawSprite(-1, PageTex_Get(0), 138.0f + dlgObj->x, 185.0f + dlgObj->y, 84, 32, 42, 0, 128, 32);
    Page_DrawTextLine(20, -1, 0, 20, 40.0f + dlgObj->x, 85.0f + dlgObj->y, Browser_GetText(BTEXT_COMM_ERROR, 0));
    textX = 138.0f + dlgObj->x + (f32)((84 - Str_Length(Browser_GetText(BTEXT_COMM_ERROR, 1)) * 20 / 2) / 2);
    Page_DrawTextLine(20, -1, 1, 20, textX, 6.0f + (185.0f + dlgObj->y), Browser_GetText(BTEXT_COMM_ERROR, 1));
    switch (gBrowserSub->dlgCursor) {
    case 1:
        buttonX = 138.0f + dlgObj->x;
        break;
    }
    color = -1;
    if (dlgObj->glowDir == GLOW_RISING) {
        color = BROWSER_CURSOR_RED;
    }
    Prim_DrawSprite(color, PageTex_Get(0), buttonX, 185.0f + dlgObj->y, 84, 32, 42, 32, 128, 64);
    TextObj_Reset();
}


#pragma optimization_level 4

/*
 * Tiles the page texture in slot texSlot over the object's rectangle, from
 * its top left corner, row by row; the tiles of the last column and row are
 * cut to the rectangle, texels and all. It stops after 512 tiles.
 * Page_DrawObjBack draws a table's or cell's background image with it.
 * Compiled at the compiler's optimization level 4, as the game's function
 * was. The empty first branch of the cut test (no cut) stays: the game's
 * code tests that case too, and it compiles to no instructions.
 */
void Page_TileImage(BrowserObj *pageObj, u8 texSlot)
{
    f32 overhang;
    f32 x;
    u8 tileCount;
    f32 y;
    f32 w;
    f32 h;
    f32 cutRight;
    f32 srcW;
    f32 cutBottom;
    f32 srcH;

    tileCount = 0;
    cutBottom = 0.0f;
    y = pageObj->y - gBrowserState->y;
    for (;;) {
        overhang = (y + (f32)gBrowserWork->texH[texSlot]) - (pageObj->h + (pageObj->y - gBrowserState->y));
        if (overhang > 0.0f) {
            cutBottom = overhang;
            if (cutBottom == 0.0f) {
                return;
            }
        }
        cutRight = 0.0f;
        x = pageObj->x - gBrowserState->x;
        for (;;) {
            overhang = (x + (f32)gBrowserWork->texW[texSlot]) - (pageObj->w + (pageObj->x - gBrowserState->x));
            if (overhang > 0.0f) {
                cutRight = overhang;
                if (cutRight == 0.0f) {
                    break;
                }
            }
            w = gBrowserWork->texW[texSlot];
            h = gBrowserWork->texH[texSlot];
            srcW = gBrowserWork->texW[texSlot];
            srcH = gBrowserWork->texH[texSlot];
            if ((cutRight == 0.0f || cutRight == 1.0f) && (cutBottom == 0.0f || cutBottom == 1.0f)) { /* kept as the game's code tests it; it compiles to nothing */
            } else if ((cutRight != 0.0f || cutRight == 1.0f) && (cutBottom == 0.0f || cutBottom == 1.0f)) {
                w -= cutRight;
                srcW -= cutRight;
            } else if ((cutRight == 0.0f || cutRight == 1.0f) && cutBottom != 0.0f && cutBottom != 1.0f) {
                h -= cutBottom;
                srcH -= cutBottom;
            } else {
                w -= cutRight;
                h -= cutBottom;
                srcW -= cutRight;
                srcH -= cutBottom;
            }
            Prim_DrawSprite(-1, gBrowserWork->texHandle[texSlot], x, y, w, h, 0, 0, srcW, srcH);
            tileCount++;
            if (tileCount > 512) {
                return;
            }
            if (cutRight > 0.0f) {
                break;
            }
            x += gBrowserWork->texW[texSlot];
        }
        if (cutBottom > 0.0f) {
            return;
        }
        y += gBrowserWork->texH[texSlot];
    }
}
#pragma optimization_level reset

/*
 * Darkens the whole screen with a half-transparent black rectangle.
 * Browser_DrawOverlay calls it to dim the page behind a dialog.
 */
void Draw_DimScreen(void)
{
    Draw_RectClipped(BROWSER_DIM, 0.0f, 0.0f, 640.0f, 448.0f);
}

/*
 * Draws over the screen after the page: black over the four margins
 * around the page view; the message icon at the top right when the page
 * has a message and style PAGE_STYLE_MSG_ICON without
 * PAGE_STYLE_NO_MSG_ICON; the dimming (Draw_DimScreen) when
 * PAGE_STYLE_POINTER is clear and a dialog, a message or
 * the input box is up; then the fade, while initMode is set: a positive
 * fade counts down from black, a negative one counts up to black. Else
 * fade is cleared. The two margin branches are the same, as in the game.
 */
void Browser_DrawOverlay(void)
{
    s8 *fade;
    s32 fadeColor;

    if ((gBrowserState->style & PAGE_STYLE_NO_MSG_ICON) >> 9) {
        if (gBrowserState->plain != 0) {
            Draw_RectClipped(BROWSER_BLACK, 0.0f, 0.0f, 640.0f, gBrowserState->marginTop);
            Draw_RectClipped(BROWSER_BLACK, 0.0f, gBrowserState->marginTop, gBrowserState->marginLeft, 448 - gBrowserState->marginBottom);
            Draw_RectClipped(BROWSER_BLACK, 0.0f, 448 - gBrowserState->marginBottom, 640.0f, 448.0f);
            Draw_RectClipped(BROWSER_BLACK, 640 - gBrowserState->marginRight, gBrowserState->marginTop, 640.0f, 448 - gBrowserState->marginBottom);
        } else {
            Draw_RectClipped(BROWSER_BLACK, 0.0f, 0.0f, 640.0f, gBrowserState->marginTop);
            Draw_RectClipped(BROWSER_BLACK, 0.0f, gBrowserState->marginTop, gBrowserState->marginLeft, 448 - gBrowserState->marginBottom);
            Draw_RectClipped(BROWSER_BLACK, 0.0f, 448 - gBrowserState->marginBottom, 640.0f, 448.0f);
            Draw_RectClipped(BROWSER_BLACK, 640 - gBrowserState->marginRight, gBrowserState->marginTop, 640.0f, 448 - gBrowserState->marginBottom);
        }
    } else {
        Draw_RectClipped(BROWSER_BLACK, 0.0f, 0.0f, 640.0f, gBrowserState->marginTop);
        Draw_RectClipped(BROWSER_BLACK, 0.0f, gBrowserState->marginTop, gBrowserState->marginLeft, 448 - gBrowserState->marginBottom);
        Draw_RectClipped(BROWSER_BLACK, 0.0f, 448 - gBrowserState->marginBottom, 640.0f, 448.0f);
        Draw_RectClipped(BROWSER_BLACK, 640 - gBrowserState->marginRight, gBrowserState->marginTop, 640.0f, 448 - gBrowserState->marginBottom);
        if ((gBrowserState->style & PAGE_STYLE_MSG_ICON) >> 8 && strlen(gBrowserState->msg) != 0) {
            Prim_DrawSprite(-1, PageTex_Get(0), 552, 30, 24, 24, 160, 0, 192, 32);
        }
    }
    if (!(gBrowserState->style & PAGE_STYLE_POINTER)) {
        if (gBrowserWork->dlgMenuState != BDLG_MENU_HIDDEN || gBrowserWork->dlgInputState != BDLG_INPUT_IDLE || gBrowserWork->dlgBoxKind != BDLG_KIND_NONE) {
            Draw_DimScreen();
        }
    }
    if (gBrowserState->initMode != 0) {
        fade = &gBrowserState->fade;
        if (*fade > 0) {
            *fade = *fade - 1;
            fadeColor = (gBrowserState->fade * 255 / 16) << 24;
            Draw_RectClipped(fadeColor, 0.0f, 0.0f, 640.0f, 448.0f);
        } else if (*fade < 0) {
            *fade = *fade + 1;
            fadeColor = (gBrowserState->fade * 255 / 16 + 255) << 24;
            Draw_RectClipped(fadeColor, 0.0f, 0.0f, 640.0f, 448.0f);
        }
    } else {
        gBrowserState->fade = 0;
    }
}

/*
 * Clips the rectangle with corners (*x0, *y0) and (*x1, *y1) to the 640 x 448
 * screen in place and returns 1, or returns 0 when it lies wholly off the
 * screen (its left edge right of the screen, its right edge left of it,
 * and so for the top and bottom). The left and top edges are clamped to
 * 0, the right to 640 and the bottom to 448. The line draws call it.
 */
s32 Rect_ClipToScreen(f32 *x0, f32 *y0, f32 *x1, f32 *y1)
{
    f32 left;
    f32 right;
    f32 top;
    f32 bottom;

    left = *x0;
    if (left < 0.0f) {
        *x0 = 0.0f;
        goto checkRight;
    }
    if (!(left <= 640.0f)) {
        return 0;
    }
checkRight:
    right = *x1;
    if (right < 0.0f) {
        return 0;
    }
    if (!(right <= 640.0f)) {
        *x1 = 640.0f;
    }
    top = *y0;
    if (top < 0.0f) {
        *y0 = 0.0f;
        goto checkBottom;
    }
    if (!(top <= 448.0f)) {
        return 0;
    }
checkBottom:
    bottom = *y1;
    if (bottom < 0.0f) {
        return 0;
    }
    if (!(bottom <= 448.0f)) {
        *y1 = 448.0f;
    }
    return 1;
}
