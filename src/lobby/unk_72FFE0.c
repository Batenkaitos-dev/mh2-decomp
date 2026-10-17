/* The browser's primitives (a triangle, a rectangle, a sprite) and the
 * page loading's start and end. include/browser.h. */
#include "common.h"
#include "browser.h"

void *memset(void *dst, s32 c, u32 n);
s32 Gs_SetState(u8, u32); /* main: flSetRenderState? */
void Gfx_SetTexFilter(s32 on);               /* sub_main */
void Prim_Sprite(Sprite *s);   /* Prim_Sprite */
void Prim_Rect(PrimLine *r);     /* Prim_Rect */
s32 Prim_Tri(PrimTri *); /* Prim_Tri */
void Html_ResetLoadStep(void);               /* Html_Reset */

/* Browser_SetErrorPage? */
void Browser_SetErrorState(s32 kind)
{
    gBrowserWork->dlgBoxKind = 4;
    gBrowserWork->errorKind = kind;
}

void func_00730010(void)
{
}

/* Browser_SetRenderState */
void Browser_SetRenderState(s32 state, u32 value)
{
    Gs_SetState(state, value);
}

/* Prim_DrawTri: a flat triangle. */
void Prim_DrawTri(u32 color, f32 x0, f32 y0, f32 x1, f32 y1, f32 x2, f32 y2)
{
    PrimTri t;

    Browser_SetRenderState(1, 0);
    t.color = color;
    t.x0 = x0;
    t.y0 = y0;
    t.x1 = x1;
    t.y1 = y1;
    t.x2 = x2;
    t.y2 = y2;
    Prim_Tri(&t);
}

/* Prim_DrawRect: a flat rectangle (x1 and x2 moved off the columns 4, 9,
 * 14 ... by one pixel). */
void Prim_DrawRect(s32 color, f32 x1, f32 y1, f32 x2, f32 y2)
{
    PrimLine r;

    if (((s16)x1 - 4) % 5 == 0) {
        x1 -= 1.0f;
    }
    if (((s16)x2 - 4) % 5 == 0) {
        x2 += 1.0f;
    }
    Browser_SetRenderState(1, 0);
    r.color = color;
    r.x0 = x1;
    r.y0 = y1;
    r.x1 = x2;
    r.y1 = y2;
    Prim_Rect(&r);
}

/* Prim_DrawSprite: a textured rectangle. */
void Prim_DrawSprite(u32 color, u32 tex, s16 x, s16 y, s16 w, s16 h, s16 u1, s16 v1, s16 u2, s16 v2)
{
    Sprite s;

    Gs_SetState(0x6C, 0);
    Gfx_SetTexFilter(1);
    Browser_SetRenderState(4, tex);
    s.color = color;
    s.x = x;
    s.y = y;
    s.w = w;
    s.h = h;
    s.u1 = u2 - 1;
    s.v1 = v2 - 1;
    s.u0 = u1;
    s.v0 = v1;
    Prim_Sprite(&s);
    Gfx_SetTexFilter(0);
}

/* Html_ResetLoad */
void Html_ResetLoad(void)
{
    gBrowserWork->loadState = 0;
    memset(&gBrowserWork->loadResult, 0, 4);
}

/* Html_StartLoad: starts parsing the page src (Html_LoadStep goes on). */
void Html_StartLoad(char *src)
{
    if (gBrowserWork->loadState == 0) {
        Html_ResetLoadStep();
        gBrowserWork->loadSrc = src;
        gBrowserWork->loadState = 1;
        gBrowserWork->loadPos = 0;
    }
}

/* Html_StopLoad */
void Html_StopLoad(void)
{
    gBrowserWork->loadState = 0;
}
