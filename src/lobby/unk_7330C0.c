/* The portal's first mode: a new page (the objects, the draw commands) and
 * the start-up steps. include/browser.h. */
#include "common.h"
#include "browser.h"

void *memset(void *dst, s32 c, u32 n);
void Gfx_SetState63(s32 on);                  /* sub_main */
void Browser_SetRenderState(s32 state, u32 value);
void Draw_Title(u8 *text);
void Page_FreeObjs(void);                  /* Page_FreeObjs */
void Page_AllocObjs(void);
BrowserObj *PageObj_Alloc(s32 layer, s32 atTail); /* PageObj_Alloc */
void PageObj_Nop(void);         /* PageObj_Nop */

/* The steps of the portal's mode 0 (.data). */
extern void (*jtbl_007EEB10[])(void);

/* Page_New: clears the page (its objects, its draw commands) and the
 * view: position 0, 0x248 by 0x17C. */
void Page_New(void)
{
    memset(gBrowserWork->kbdText, 0, 0x100);
    memset(gBrowserWork, 0, 4);
    memset(gBrowserWork->texHandle, 0, 0x50);
    Page_FreeObjs();
    Page_AllocObjs();
    memset(gBrowserWork->cmdBuf, 0, 0x8000);
    gBrowserWork->cmdPtr = gBrowserWork->cmdBuf;
    *gBrowserWork->cmdBuf = 0;
    gBrowserWork->cmdPtr++;
    Draw_Title((u8 *)"");
    gBrowserState->styleWait = 0x1E;
    gBrowserState->imgCount = 0;
    gBrowserState->vScroll = 0;
    gBrowserState->hScroll = 0;
    gBrowserState->x = 0;
    gBrowserState->y = 0;
    gBrowserState->pageW = 0x248;
    gBrowserState->pageH = 0x17C;
    gBrowserState->pageKind = -1;
    gBrowserWork->focus = 0;
    gBrowserWork->texCount = 0;
    Browser_SetRenderState(0x60, 0xFF000001);
    Browser_SetRenderState(0x5F, 5);
    Browser_SetRenderState(0x5E, 1);
    Gfx_SetState63(0);
}

/* Browser_RunStart: the portal's mode 0, its step. */
void Browser_RunStart(void)
{
    jtbl_007EEB10[gBrowserState->step]();
}

/* Page_AllocObjs: the page's 500 objects, empty. */
void Page_AllocObjs(void)
{
    u16 n;

    memset(gBrowserWork->texHandle, 0, 0x50);
    memset(gBrowserWork->texW, 0, 0x28);
    memset(gBrowserWork->texH, 0, 0x28);
    for (n = 0; n < 500; n++) {
        if ((gBrowserWork->obj[n] = PageObj_Alloc(1, 0)) == NULL) {
            return;
        }
        gBrowserWork->obj[n]->active = 0;
        gBrowserWork->obj[n]->tex = 0;
        gBrowserWork->obj[n]->update = (BrowserObjFunc)PageObj_Nop;
        gBrowserWork->obj[n]->draw = (BrowserObjFunc)PageObj_Nop;
        gBrowserWork->obj[n]->visible = 0;
        gBrowserWork->obj[n]->tex = 0;
        gBrowserWork->obj[n]->sub = 0;
    }
}

/* unk_7330C0's .data (0x7EEB10-0x7EEB20) */
void Portal_Start0(void);
void Portal_Start1(void);
void (*jtbl_007EEB10[4])(void) = {
    Portal_Start0, Portal_Start1, NULL, NULL,
};
