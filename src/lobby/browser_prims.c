/*
 * The browser's primitives (a flat triangle, a flat rectangle, a textured
 * rectangle), each setting the render state it needs first, the error
 * dialog's opening, and the HTML parse's start and stop (Html_LoadStep
 * steps the parse each frame while loadState is set).
 */
#include "common.h"
#include "browser.h"
#include "render_state.h"

void *memset(void *dst, s32 c, u32 n);
s32 Gs_SetState(u8 state, u32 value);
void Gfx_SetTexFilter(s32 on);
void Prim_Sprite(Sprite *s);
void Prim_Rect(PrimLine *r);
s32 Prim_Tri(PrimTri *tri);
void Html_ResetLoadStep(void);

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

/* Sets one of the GS render states (Gs_SetState; RS_* where named). */
void Browser_SetRenderState(s32 state, u32 value)
{
    Gs_SetState(state, value);
}

/* Draws a flat triangle in color, untextured. */
void Prim_DrawTri(u32 color, f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2)
{
    PrimTri tri;

    Browser_SetRenderState(1, 0);
    tri.color = color;
    tri.x0 = x0;
    tri.y0 = y0;
    tri.x1 = x1;
    tri.y1 = y1;
    tri.x2 = x2;
    tri.y2 = y2;
    Prim_Tri(&tri);
}

/*
 * Draws a flat rectangle in color, untextured, from (x1, y1) to (x2, y2).
 * An edge on column 4, 9, 14 ... (every fifth from 4) is moved out by one
 * pixel.
 */
void Prim_DrawRect(s32 color, f32 x1, f32 y1, f32 x2, f32 y2)
{
    PrimLine rect;

    if (((s16)x1 - 4) % 5 == 0) {
        x1 -= 1.0f;
    }
    if (((s16)x2 - 4) % 5 == 0) {
        x2 += 1.0f;
    }
    Browser_SetRenderState(1, 0);
    rect.color = color;
    rect.x0 = x1;
    rect.y0 = y1;
    rect.x1 = x2;
    rect.y1 = y2;
    Prim_Rect(&rect);
}

/*
 * Draws the texture tex, texels (u1, v1) to (u2, v2), over the rectangle at
 * (x, y), w by h, tinted by color: with depth writes off and the texture
 * filtered, the filter turned off again after.
 */
void Prim_DrawSprite(u32 color, u32 tex, s16 x, s16 y, s16 w, s16 h, s16 u1, s16 v1, s16 u2, s16 v2)
{
    Sprite sprite;

    Gs_SetState(RS_ZWRITE, 0);
    Gfx_SetTexFilter(1);
    Browser_SetRenderState(RS_TEXTURE, tex);
    sprite.color = color;
    sprite.x = x;
    sprite.y = y;
    sprite.w = w;
    sprite.h = h;
    sprite.u1 = u2 - 1;
    sprite.v1 = v2 - 1;
    sprite.u0 = u1;
    sprite.v0 = v1;
    Prim_Sprite(&sprite);
    Gfx_SetTexFilter(0);
}

/* Clears the HTML parse: none running and no result. */
void Html_ResetLoad(void)
{
    gBrowserWork->loadState = 0;
    memset(&gBrowserWork->loadResult, 0, sizeof(s32));
}

/* Starts parsing the page's HTML src unless a parse is running
 * (Html_LoadStep goes on with it). */
void Html_StartLoad(char *src)
{
    if (gBrowserWork->loadState == 0) {
        Html_ResetLoadStep();
        gBrowserWork->loadSrc = src;
        gBrowserWork->loadState = 1;
        gBrowserWork->loadPos = 0;
    }
}

/* Stops the HTML parse. */
void Html_StopLoad(void)
{
    gBrowserWork->loadState = 0;
}
