/* The page's frame: its margins, and the dialog objects (the select
 * list, the scroll bars, the status line, the dialogs, the keyboard): per
 * kind a create function (layer, position, size), an update function
 * (its state from the work area's flag) and a draw function.
 * include/browser.h. */
#include "common.h"
#include "browser.h"
#include "lobby_globals.h"
#include "pad.h"
#include "lobby/unk_72C7C0.h"   /* the texts this file uses (tools/content_gen.py) */

/* Compiled at optimization level 4 (Page_SetKind matches only so). */
#pragma optimization_level 4

void PageObj_DrawLayer(s32 layer);
void Browser_OpenDialog(s8 kind, s8 arg);
s32 strcmp(const char *a, const char *b); /* main: strcmp */
BrowserObj *PageObj_Alloc(s32 layer, s32 atTail);
void Font_SetSpacing(u8 a, u8 b);
s8 Kbd_Run(char *text, s16 held, s16 a2);   /* sub_main: the software keyboard? */
void Chat_OpenKbd(s32 a0, u8 mode, u16 max, void *d); /* sub_main: opens the software keyboard */ /* unit's own declaration: the second parameter u8 (the definition has s32); with the definition's type Browser_OpenKeyboard by 2 of 49 words differ */
void Kbd_SetPos(s32 a0, f32 a1);
u32 strlen(const char *s);
char *strcpy(char *dst, const char *src);
void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
s32 LobbyCard_RunNetSave(void);  /* the lobby: saves the NET file? 0 busy, 1 done, -1 failed */
void Dialog_ResetBoxState(void);
void Browser_DrawOverlay(void);
void PageObj_Free(BrowserObj *o);
void TextObj_Reset(void);                   /* sub_main */
char *Browser_GetText(u32 group, s32 i);
void TileWin_Draw(f32 x0, f32 y0, f32 x1, f32 y1);
void Page_DrawThinFrame(s32 color, f32 x0, f32 y0, f32 x1, f32 y1); /* a frame? */
void Page_DrawTextLine(u8 a0, u32 color, u8 a2, u8 size, f32 x, f32 y, char *text); /* a line of text */
void Page_DrawWidget(u8 kind, f32 x, f32 y, f32 w, f32 h, s32 color); /* a window */
char *BrowserHist_CurUrl(void); /* the page's URL? */
void Draw_RectClipped(s32 color, f32 x0, f32 y0, f32 x1, f32 y1); /* a rectangle */
struct PageElem;
void Page_DrawObjBack(struct PageElem *o);
void Page_DrawObj(struct PageElem *o);
void Page_DrawDropList(s32 sel);
void Browser_SetRenderState(s32 state, u32 value);
void Gfx_SetState63(s32 a);                    /* sub_main */
u32 PageTex_Get(s8 i);
s32 func_007D21E0(void);                    /* the lobby: nonzero while ... */
void LobbyLogin_DrawWaveTitle(s32 unused0);   /* the lobby's; its definition takes no argument */
void Prim_DrawSprite(u32 color, u32 tex, s16 x, s16 y, s16 w, s16 h, s16 u1, s16 v1, s16 u2, s16 v2);
void BrowserDlg_DrawStatus(struct PageElem *o);
void BrowserDlg_DrawYesNo(struct PageElem *o);
void BrowserDlg_DrawOk(struct PageElem *o);
void BrowserDlg_DrawCrsTop(struct PageElem *o);
void Page_DrawFocus(struct PageElem *o);
void Page_DrawFrame(s32 color, s32 width, f32 x0, f32 y0, f32 x1, f32 y1); /* a frame */
void Page_DrawMark(u8 on, f32 x, f32 y);   /* the pointer */
void Kbd_Cancel(void);                     /* sub_main */
void Chat_DrawKbdBox(s32 color);                /* sub_main */
void Font_SetHalfWidth(s32 a0);                   /* sub_main */
void Font_DrawAllPages(void);                     /* sub_main */
char *Browser_Keyboard(void);
/* The registration record being filled (0x1D0 bytes at 0x895280) and
 * the fields the keyboard dialog writes. */
extern u8 gRegRecord[];
extern u8 D_008952C1[];
extern u8 D_008952CC[];
extern u8 D_0089534D[];
extern s8 D_008953CE;
extern u8 D_008953CF[];
/* a page texture as a sprite; the game passed two zeros between the
 * coordinates (their order puts a1 and a2 where the game has them) */
void Page_DrawIcon(u16 tex, f32 x, f32 y, f32 w, f32 h, s32 u, s32 v);

extern char gMmbbLoginId[]; /* sub_main: the MMBB login id kept in the NET file? */
extern char D_5BE6BB[]; /* sub_main: its password? */

void DlgBack_Update(BrowserObj *o);
void DlgBack_Draw(BrowserObj *o);
void DlgPage_Update(BrowserObj *o);
void DlgPage_Draw(BrowserObj *o);
void DlgVScroll_Update(BrowserObj *o);
void DlgVScroll_Draw(BrowserObj *o);
void DlgHScroll_Update(BrowserObj *o);
void DlgHScroll_Draw(BrowserObj *o);
void DlgStatus_Update(BrowserObj *o);
void DlgStatus_Draw(BrowserObj *o);
void DlgMsg_Update(BrowserObj *o);
void DlgMsg_Draw(BrowserObj *o);
void DlgInput_Update(BrowserObj *o);
void DlgInput_Draw(BrowserObj *o);
void DlgCursor_Update(BrowserObj *o);
void DlgCursor_Draw(BrowserObj *o);
void DlgBox_Update(BrowserObj *o);
void DlgBox_Draw(BrowserObj *o);

/* Page_SetKind: what the page does for the game (Html_Parse, from the
 * page keyword: 0 CRS-TOP, 1 CRS-MENU, 2 INP-SIGNUP, 3 COMP-SIGNUP,
 * 4 INP-PWCHANGE, 5 CMP-PWCHANGE, 6 TOP-SIGNUP, 7 COMP-LOGIN,
 * 8 KEICAN-MMBB, 9 CANCAN-MMBB): the dialogs it opens (4 after a sign-up
 * or a new login id, 5 on the sign-up top with a login id, 6 KEICAN). */
void Page_SetKind(s8 kind)
{
    gBrowserState->pageKind = kind;
    switch (kind) {
    case 0:
        gBrowserState->fromTop = 1;
        break;
    case 1:
        gBrowserState->menuReached = 1;
        if (gBrowserState->fromTop != 0) {
            if (strcmp((char *)gBrowserState->loginId, gBrowserState->field1) != 0) {
                Browser_OpenDialog(4, 1);
            } else {
                gBrowserState->fromTop = 0;
            }
        }
        break;
    case 6:
        if ((u8)gBrowserState->loginId[0] != 0 && gBrowserState->fromTop != 0) {
            gBrowserState->fromTop = 0;
            Browser_OpenDialog(5, 2);
        }
        break;
    case 3:
        gBrowserState->menuReached = 1;
        Browser_OpenDialog(4, 1);
        gBrowserState->fromTop = 0;
        break;
    case 4:
        gBrowserState->fromTop = 1;
        break;
    case 5:
        Browser_OpenDialog(4, 1);
        gBrowserState->fromTop = 0;
        break;
    case 8:
        Browser_OpenDialog(6, 1);
        break;
    case 9:
        break;
    }
}

/* Page_Extend: the page reaches (x, y) at least: its size, the scroll
 * bars' scale, and whether it scrolls. */
void Page_Extend(s32 x, s32 y)
{
    if (gBrowserState->pageH < y) {
        gBrowserState->pageH = y;
        gBrowserState->vThumbLen = 0x20490 / gBrowserState->pageH;
        if (gBrowserState->pageH > 0x17C) {
            gBrowserState->vScroll = 1;
        }
    }
    if (x >= gBrowserState->pageW) {
        gBrowserState->pageW = x;
        gBrowserState->hThumbLen = 0x4EB40 / gBrowserState->pageW;
        if (gBrowserState->pageW > 0x248) {
            gBrowserState->hScroll = 1;
        }
    }
}

/* Page_IsHidden: data pages (modes 1, 4, 5) draw no form fields. */
s32 Page_IsHidden(void)
{
    switch (gBrowserState->pageMode) {
    case 1:
    case 4:
    case 5:
        return 1;
    }
    return 0;
}

/* Page_Margins: the page's margins from its style flags (4 and 2). The callers pass
 * three arguments (two in unk_732240.c's unit), which it does not read. */
void Page_Margins(u8 unused0, s32 unused1, s32 unused2)
{
    s32 flags = gBrowserState->style;
    s32 small = (flags & 4) >> 2;

    if (small != 0 && ((flags & 2) >> 1) != 0) {
        gBrowserState->marginTop = 0x10;
        gBrowserState->marginBottom = 0x10;
        gBrowserState->marginLeft = 0x14;
        gBrowserState->marginRight = 0x14;
        return;
    }
    if (small != 0 && ((flags & 2) >> 1) == 0) {
        gBrowserState->marginBottom = 0x1A;
        gBrowserState->marginTop = 0x1A;
        gBrowserState->marginRight = 0x1E;
        gBrowserState->marginLeft = 0x1E;
    } else {
        gBrowserState->marginBottom = 0x22;
        gBrowserState->marginTop = 0x22;
        gBrowserState->marginRight = 0x1C;
        gBrowserState->marginLeft = 0x1C;
    }
}

void DlgBack_DrawAll(void)
{
    PageObj_DrawLayer(0);
}

/* Dialog kind 0 (layer 0): the select list. */
void DlgBack_Create(s32 mode)
{
    BrowserObj *o;

    switch (mode) {
    case 1:
        o = PageObj_Alloc(0, 0);
        if (o != NULL) {
            o->visible = 1;
            o->active = 1;
            o->update = DlgBack_Update;
            o->draw = DlgBack_Draw;
            o->x = 0.0f;
            o->y = 0.0f;
            o->unk4C = 0;
            o->unk50 = 0;
            o->w = 0.0f;
            o->h = 0.0f;
            o->state = 0;
            o->boxStep = 0;
            o->blink = 0;
            o->blinkStep = 0;
            o->unkE = 0;
            o->unk10 = 0;
            o->field = 0;
            gBrowserWork->dlgBackState = 1;
        }
        break;
    }
}

void DlgBack_Update(BrowserObj *o)
{
    u16 i;

    switch (gBrowserWork->dlgBackState) {
    case 1:
        o->state = 0;
        for (i = 0; i < 500; i++) {
            if (gBrowserWork->obj[i] == NULL || !gBrowserWork->obj[i]->active) {
                break;
            }
            if (gBrowserWork->obj[i]->kind == 0xD && gBrowserWork->obj[i]->active == 1) {
                switch (gBrowserWork->obj[i]->sub) {
                case 0:
                case 1:
                    return;
                case 2:
                    o->tex = gBrowserWork->obj[i]->tex;
                    o->field = gBrowserWork->obj[i]->field;
                    o->w = gBrowserWork->texW[o->tex];
                    o->h = gBrowserWork->texH[o->tex];
                    gBrowserWork->dlgBackState = 2;
                    return;
                case 3:
                    gBrowserWork->dlgBackState = 3;
                    return;
                }
            }
        }
        break;
    case 3:
        o->state = 0;
        break;
    case 2:
        o->state = 1;
        break;
    case 4:
        o->state = 99;
        break;
    }
}

void DlgBack_Draw(BrowserObj *o)
{
    u16 n;
    f32 x;
    f32 y;

    switch (o->state) {
    case 0:
        return;
    case 1:
        break;
    case 99:
        PageObj_Free(o);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserWork->texHandle[o->tex] == 0) {
        return;
    }
    n = 0;
    if (o->field) {
        y = gBrowserState->marginTop;
    } else {
        y = gBrowserState->marginTop - (s16)gBrowserState->y % (s16)(o->y + o->h);
    }
    while (y < 448.0f) {
        if (o->field) {
            x = gBrowserState->marginLeft;
        } else {
            x = gBrowserState->marginLeft - (s16)gBrowserState->x % (s16)(o->x + o->w);
        }
        while (x < 640.0f) {
            n++;
            if (n > 0x200) {
                return;
            }
            Page_DrawIcon(o->tex, x, y, o->w, o->h, 0, 0);
            x += o->w;
        }
        y += o->h;
    }
}

void DlgPage_DrawAll(void)
{
    PageObj_DrawLayer(0x1);
}

/* Dialog kind 1 (layer 1). */
void DlgPage_Create(s32 mode)
{
    BrowserObj *o;

    switch (mode) {
    case 1:
        o = PageObj_Alloc(1, 0);
        if (o != NULL) {
            o->visible = 1;
            o->active = 1;
            o->update = DlgPage_Update;
            o->draw = DlgPage_Draw;
            o->x = 0.0f;
            o->y = 0.0f;
            o->unk4C = 0;
            o->unk50 = 0;
            o->w = 0.0f;
            o->h = 0.0f;
            o->state = 1;
            o->boxStep = 0;
            o->blink = 0;
            o->blinkStep = 0;
            o->unkE = 0;
            o->unk10 = 0;
            gBrowserWork->dlgPageState = 3;
        }
        break;
    }
}

void DlgPage_Update(BrowserObj *o)
{
    switch (gBrowserWork->dlgPageState) {
    case 3:
        o->state = 1;
        break;
    case 5:
        o->state = 99;
        break;
    }
}

void DlgPage_Draw(BrowserObj *o)
{
    u16 i;
    BrowserObj *p;
    f32 x;
    f32 y;

    switch (o->state) {
    case 0:
        return;
    case 1:
        break;
    case 99:
        PageObj_Free(o);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserState->plain == 0) {
        Browser_SetRenderState(0x6D, 7);
        for (i = 499; i > 0; i--) {
            p = gBrowserWork->obj[i];
            if (p != NULL && p->active) {
                Page_DrawObjBack((struct PageElem *)p);
            }
        }
        for (i = 1; i < 500; i++) {
            p = gBrowserWork->obj[i];
            if (p == NULL || !p->active) {
                break;
            }
            {
                Page_DrawObj((struct PageElem *)p);
                if (gBrowserSub->listObj == i) {
                    y = gBrowserWork->obj[i]->y - (f32)gBrowserState->y;
                    x = gBrowserWork->obj[i]->x - (f32)gBrowserState->x;
                    Draw_RectClipped(0x400000FF, 1.0f + x, 1.0f + y, gBrowserWork->obj[i]->w + (x - 2.0f),
                                     gBrowserWork->obj[i]->h + (y - 1.0f));
                }
                if (((gBrowserState->style & 0x200) >> 9) == 0 && ((gBrowserState->style & 0x100) >> 8) != 0 &&
                    strlen(gBrowserState->msg) != 0) {
                    Page_DrawTextLine(20, -1, 1, 20, 500.0f, 30.0f, TXT(0x00838F68) /* the label before the exit line of a page */);
                }
            }
        }
        Page_DrawDropList(gBrowserSub->listObj);
        TextObj_Reset();
    }
}

void DlgVScroll_DrawAll(void)
{
    PageObj_DrawLayer(0x2);
}

/* Dialog kind 2 (layer 2): the vertical scroll bar, at the right. */
void DlgVScroll_Create(s32 mode)
{
    BrowserObj *o;

    switch (mode) {
    case 1:
        o = PageObj_Alloc(2, 0);
        if (o != NULL) {
            o->visible = 1;
            o->active = 1;
            o->update = DlgVScroll_Update;
            o->draw = DlgVScroll_Draw;
            o->x = 0x270 - gBrowserState->marginRight;
            o->y = gBrowserState->marginTop;
            o->unk4C = 0;
            o->unk50 = 0;
            o->w = 16.0f;
            o->h = 380.0f;
            o->state = 0;
            o->boxStep = 0;
            o->blink = 0;
            o->blinkStep = 0;
            o->unkE = 0;
            o->unk10 = 0;
            o->timer = 0;
            gBrowserWork->dlgVScrollState = 2;
        }
        break;
    }
}

void DlgVScroll_Update(BrowserObj *o)
{
    switch (gBrowserWork->dlgVScrollState) {
    case 1:
        o->state = 0;
        break;
    case 2:
        o->state = 1;
        break;
    case 3:
        o->state = 2;
        if (o->timer++ > 3) {
            o->timer = 0;
            gBrowserWork->dlgVScrollState = 2;
        }
        break;
    case 4:
        o->state = 3;
        if (o->timer++ > 3) {
            o->timer = 0;
            gBrowserWork->dlgVScrollState = 2;
        }
        break;
    case 5:
        o->state = 99;
        break;
    }
}

void DlgVScroll_Draw(BrowserObj *o)
{
    f32 w;

    switch (o->state) {
    case 0:
        return;
    case 1:
    case 2:
    case 3:
        break;
    case 99:
        PageObj_Free(o);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserState->plain == 0 && ((gBrowserState->style & 2) >> 1) != 0) {
        if (o->state != 2) {
            w = 16.0f;
            Page_DrawWidget(0xA, o->x, o->y, w, w, 0);
        } else {
            w = 16.0f;
            Page_DrawWidget(0xE, o->x, o->y, w, w, 0);
        }
        Page_DrawWidget(0x12, o->x, 16.0f + o->y, 16.0f, 380.0f, 0);
        if (gBrowserState->vScroll != 0) {
            gBrowserState->vThumbY = gBrowserState->marginTop + (gBrowserState->y * 348 / gBrowserState->pageH + 16);
            Page_DrawWidget(0x13, o->x, gBrowserState->vThumbY, w, gBrowserState->vThumbLen, 0);
        }
        if (o->state != 3) {
            Page_DrawWidget(0xB, o->x, 380.0f, 16.0f, 16.0f, 0);
        } else {
            Page_DrawWidget(0xF, o->x, 380.0f, 16.0f, 16.0f, 0);
        }
    }
}

void DlgHScroll_DrawAll(void)
{
    PageObj_DrawLayer(0x2);
}

/* Dialog kind 3 (layer 2): the horizontal scroll bar, at the bottom. */
void DlgHScroll_Create(s32 mode)
{
    BrowserObj *o;

    switch (mode) {
    case 1:
        o = PageObj_Alloc(2, 0);
        if (o != NULL) {
            o->visible = 1;
            o->active = 1;
            o->update = DlgHScroll_Update;
            o->draw = DlgHScroll_Draw;
            o->x = gBrowserState->marginLeft;
            o->y = 0x19C - gBrowserState->marginBottom;
            o->unk4C = 0;
            o->unk50 = 0;
            o->w = 584.0f;
            o->h = 16.0f;
            o->state = 0;
            o->boxStep = 0;
            o->blink = 0;
            o->blinkStep = 0;
            o->unkE = 0;
            o->unk10 = 0;
            o->timer = 0;
            gBrowserWork->dlgHScrollState = 2;
        }
        break;
    }
}

void DlgHScroll_Update(BrowserObj *o)
{
    switch (gBrowserWork->dlgHScrollState) {
    case 1:
        o->state = 0;
        break;
    case 2:
        o->state = 1;
        break;
    case 3:
        o->state = 2;
        if (o->timer++ > 3) {
            o->timer = 0;
            gBrowserWork->dlgHScrollState = 2;
        }
        break;
    case 4:
        o->state = 3;
        if (o->timer++ > 3) {
            o->timer = 0;
            gBrowserWork->dlgHScrollState = 2;
        }
        break;
    case 5:
        o->state = 99;
        break;
    }
}

void DlgHScroll_Draw(BrowserObj *o)
{
    f32 h;

    switch (o->state) {
    case 0:
        return;
    case 1:
    case 2:
    case 3:
        break;
    case 99:
        PageObj_Free(o);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserState->plain == 0 && ((gBrowserState->style & 2) >> 1) != 0) {
        Gfx_SetState63(0);
        if (o->state != 2) {
            h = 16.0f;
            Page_DrawWidget(0xC, o->x, o->y, h, h, 0);
        } else {
            h = 16.0f;
            Page_DrawWidget(0x10, o->x, o->y, h, h, 0);
        }
        Page_DrawWidget(0x14, 16.0f + o->x, o->y, 584.0f, 16.0f, 0);
        if (gBrowserState->hScroll != 0) {
            gBrowserState->hThumbX = gBrowserState->marginRight + (gBrowserState->x * 552 / gBrowserState->pageW + 16);
            Page_DrawWidget(0x15, gBrowserState->hThumbX, o->y, gBrowserState->hThumbLen, h, 0);
        }
        if (o->state != 3) {
            Page_DrawWidget(0xD, 588.0f, o->y, 16.0f, 16.0f, 0);
        } else {
            Page_DrawWidget(0x11, 588.0f, o->y, 16.0f, 16.0f, 0);
        }
    }
}

void DlgStatus_DrawAll(void)
{
    PageObj_DrawLayer(0x2);
}

/* Dialog kind 4 (layer 2): the status line, at the bottom. */
void DlgStatus_Create(s32 mode)
{
    BrowserObj *o;

    switch (mode) {
    case 1:
        o = PageObj_Alloc(2, 0);
        if (o != NULL) {
            o->visible = 1;
            o->active = 1;
            o->update = DlgStatus_Update;
            o->draw = DlgStatus_Draw;
            o->x = gBrowserState->marginLeft;
            o->y = 0x1AC - gBrowserState->marginBottom;
            o->unk4C = 0;
            o->unk50 = 0;
            o->w = 0x280 - gBrowserState->marginLeft - gBrowserState->marginRight;
            o->h = 20.0f;
            o->state = 0;
            o->boxStep = 0;
            o->blink = 0;
            o->blinkStep = 0;
            o->unkE = 0;
            o->unk10 = 0;
            gBrowserWork->dlgStatusState = 1;
        }
        break;
    }
}

void DlgStatus_Update(BrowserObj *o)
{
    switch (gBrowserWork->dlgStatusState) {
    case 4:
        o->state = 0;
        break;
    case 1:
        o->state = 1;
        break;
    case 2:
        o->state = 2;
        break;
    case 3:
        o->state = 3;
        break;
    case 5:
        o->state = 99;
        break;
    }
}

void DlgStatus_Draw(BrowserObj *o)
{
    BrowserObj *top;

    switch (o->state) {
    case 0:
        return;
    case 1:
    case 2:
    case 3:
        break;
    case 99:
        PageObj_Free(o);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserState->plain == 0 && ((gBrowserState->style & 4) >> 2) != 0) {
        Page_DrawWidget(6, o->x, o->y, o->w, o->h, 0);
        if (gBrowserWork->dlgInputState != 1) {
            top = gBrowserWork->obj[0];
            if (top != NULL) {
                switch (o->state) {
                case 1:
                    if (top->s1 != NULL) {
                        Page_DrawTextLine(0x15, 0xFF000001, 0, 0x14, o->x, o->y, top->s1);
                    }
                    break;
                case 2:
                    Page_DrawTextLine(0x15, 0xFF000001, 0, 0x10, 2.0f + o->x, 2.0f + o->y, Browser_GetText(4, 0));
                    Page_DrawTextLine(0x15, 0xFF000001, 0, 0x10, 72.0f + (2.0f + o->x), 2.0f + o->y,
                                      BrowserHist_CurUrl());
                    break;
                case 3:
                    Page_DrawTextLine(0x15, 0xFF000001, 0, 0x10, 2.0f + o->x, 2.0f + o->y, top->s2);
                    break;
                }
            }
            if (gBrowserSub->dlgMode == 0) {
                TextObj_Reset();
            }
        }
    }
}

void DlgMsg_DrawAll(void)
{
    PageObj_DrawLayer(0x3);
}

/* Dialog kind 5 (layer 3): a message box. */
void DlgMsg_Create(s32 mode)
{
    BrowserObj *o;

    switch (mode) {
    case 1:
        o = PageObj_Alloc(3, 0);
        if (o != NULL) {
            o->visible = 1;
            o->active = 1;
            o->update = DlgMsg_Update;
            o->draw = DlgMsg_Draw;
            o->x = 200.0f;
            o->y = 150.0f;
            o->unk4C = 0;
            o->unk50 = 0;
            o->w = 200.0f;
            o->h = 150.0f;
            o->state = 0;
            o->boxStep = 0;
            o->blink = 0;
            o->blinkStep = 0;
            o->unkE = 0;
            o->unk10 = 0;
            gBrowserWork->dlgMsgState = 0;
        }
        break;
    }
}

void DlgMsg_Update(BrowserObj *o)
{
    switch (gBrowserWork->dlgMsgState) {
    case 0:
        o->state = 0;
        break;
    case 1:
        o->state = 1;
        break;
    case 2:
        o->state = 99;
        break;
    }
}

void DlgMsg_Draw(BrowserObj *o)
{
    u8 i;
    s32 dy;
    f32 y;
    f32 cy;

    switch (o->state) {
    case 0:
    case 1:
        Browser_DrawOverlay();
        if (o->state == 0) {
            break;
        }
        goto draw;
    case 99:
        PageObj_Free(o);
        gBrowserWork->freedObjs++;
        break;
    default:
    draw:
        o->h = 110.0f;
        o->w = 200.0f;
        TileWin_Draw(o->x, o->y - 20.0f, o->x + o->w, o->y + o->h);
        for (i = 0, dy = 0; i < 3; i++, dy += 20) {
            y = 20.0f + o->y + (f32)dy;
            Page_DrawTextLine(20, -1, 0, 20, 40.0f + o->x, y, Browser_GetText(3, i));
        }
        TextObj_Reset();
        cy = o->y + (f32)(gBrowserSub->dlgCursor * 20);
        Page_DrawThinFrame(-1, 40.0f + o->x - 2.0f, cy - 2.0f, 2.0f + (160.0f + o->x), 2.0f + (20.0f + cy));
        break;
    }
}

/* Browser_OpenKeyboard: the software keyboard on text (kbdText keeps a
 * copy: "" when text is too long), at most max characters. */
void Browser_OpenKeyboard(char *text, s16 max, s32 mode)
{
    if (max > 0xFFu) {
        max = 0xFF;
    }
    if (strlen(text) > 0xFF) {
        strcpy(gBrowserWork->kbdText, "");
    } else {
        strcpy(gBrowserWork->kbdText, text);
    }
    Chat_OpenKbd(0, mode, max, text);
    Kbd_SetPos(300, 84.0f);
}

/* Browser_Keyboard: the software keyboard for a text field; its text once
 * entered, else NULL. */
char *Browser_Keyboard(void)
{
    Font_SetSpacing(0x16, 0x16);
    if (Kbd_Run(gBrowserWork->kbdText, gBrowserWork->padHeld, *(s16 *)&gPadState[0].pressed) != 0) {
        return gBrowserWork->kbdText;
    }
    return NULL;
}

void DlgInput_DrawAll(void)
{
    PageObj_DrawLayer(0x4);
}

/* Dialog kind 6 (layer 4). */
void DlgInput_Create(s32 mode)
{
    BrowserObj *o;

    switch (mode) {
    case 1:
        o = PageObj_Alloc(4, 0);
        if (o != NULL) {
            o->visible = 1;
            o->active = 1;
            o->update = DlgInput_Update;
            o->draw = DlgInput_Draw;
            o->x = 0.0f;
            o->y = 0.0f;
            o->unk4C = 0;
            o->unk50 = 0;
            o->w = 0.0f;
            o->h = 0.0f;
            o->state = 1;
            o->boxStep = 0;
            o->blink = 0;
            o->blinkStep = 0;
            o->unkE = 0;
            o->unk10 = 0;
            gBrowserWork->dlgInputState = 2;
        }
        break;
    }
}

void DlgInput_Update(BrowserObj *o)
{
    switch (gBrowserWork->dlgInputState) {
    case 2:
        o->state = 0;
        break;
    case 1:
        o->state = 1;
        break;
    case 3:
        o->state = 99;
        break;
    }
}

void DlgInput_Draw(BrowserObj *o)
{
    char *text;
    s16 max;
    s16 n;
    u16 i;

    switch (o->state) {
    case 0:
        return;
    case 1:
        break;
    case 99:
        PageObj_Free(o);
        Kbd_Cancel();
        gBrowserWork->freedObjs++;
        return;
    }
    Browser_SetRenderState(0x6D, 7);
    Chat_DrawKbdBox(0xBD6C6CC5);
    Font_SetHalfWidth(1);
    text = Browser_Keyboard();
    Font_DrawAllPages();
    if (text == NULL) {
        return;
    }
    if (gBrowserWork->kbdText[0] == -1) {
        gBrowserSub->listObj = 0;
        gBrowserSub->dlgMode = 0;
        gBrowserWork->dlgInputState = 2;
        gBrowserWork->dlgCursorState = 1;
        gBrowserWork->dlgMsgState = 0;
        gBrowserState->page = 8;
        return;
    }
    max = gBrowserWork->obj[gBrowserSub->listObj]->sub;
    n = strlen(gBrowserWork->kbdText);
    if (gBrowserWork->obj[gBrowserSub->listObj]->sub < n) {
        n = max;
    }
    for (i = 0; i < n; i++) {
        *gBrowserWork->obj[gBrowserSub->listObj]->s2 = gBrowserWork->kbdText[i];
        gBrowserWork->obj[gBrowserSub->listObj]->s2++;
    }
    *gBrowserWork->obj[gBrowserSub->listObj]->s2 = 0;
    gBrowserWork->obj[gBrowserSub->listObj]->s2 -= n;
    if (gBrowserWork->obj[gBrowserSub->listObj]->field == 4) {
        switch ((((u8 *)gBrowserWork->obj[gBrowserSub->listObj]->s1)[1] - '0') + (((u8 *)gBrowserWork->obj[gBrowserSub->listObj]->s1)[0] - '0') * 10) {
        case 0:
            memcpy(D_0089534D, gBrowserWork->kbdText, n + 1);
            break;
        case 1:
            memcpy(gRegRecord, gBrowserWork->kbdText, n + 1);
            break;
        case 2:
            switch (strlen(gBrowserWork->kbdText)) {
            case 1:
                if (gBrowserWork->kbdText[0] == '0') {
                    *gBrowserWork->obj[gBrowserSub->listObj]->s2 = 0;
                } else {
                    D_008953CE = gBrowserWork->kbdText[0] - '0';
                }
                break;
            case 2:
                D_008953CE = (gBrowserWork->kbdText[0] - '0') * 10 + (gBrowserWork->kbdText[1] - '0');
                if (D_008953CE == 0) {
                    *gBrowserWork->obj[gBrowserSub->listObj]->s2 = 0;
                }
                break;
            case 0:
            default:
                D_008953CE = 0;
                break;
            }
            break;
        case 3:
            memcpy(D_008952C1, gBrowserWork->kbdText, 8);
            break;
        case 8:
            memcpy(D_0089534D, gBrowserWork->kbdText, n + 1);
            break;
        case 5:
            memcpy(D_008952CC, gBrowserWork->kbdText, n + 1);
            break;
        case 7:
            memcpy(D_008953CF, gBrowserWork->kbdText, n + 1);
            break;
        }
    }
    gBrowserSub->listObj = 0;
    gBrowserSub->dlgMode = 0;
    gBrowserWork->dlgCursorState = 1;
    gBrowserWork->dlgInputState = 2;
    gBrowserState->page = 8;
}

void DlgCursor_DrawAll(void)
{
    PageObj_DrawLayer(0x8);
}

/* Dialog kind 7 (layer 8): the list's cursor. */
void DlgCursor_Create(s32 mode)
{
    BrowserObj *o;

    switch (mode) {
    case 1:
        o = PageObj_Alloc(8, 0);
        if (o != NULL) {
            o->visible = 1;
            o->active = 1;
            o->update = DlgCursor_Update;
            o->draw = DlgCursor_Draw;
            o->x = 150.0f;
            o->y = 100.0f;
            o->unk4C = 0;
            o->unk50 = 0;
            o->w = 20.0f;
            o->h = 20.0f;
            o->state = 0;
            o->boxStep = 0;
            o->blink = 0;
            o->blinkStep = 5;
            o->unkE = 0;
            o->unk10 = 0;
            gBrowserWork->dlgCursorState = 1;
            gBrowserWork->cursorOnLink = 0;
        }
        break;
    }
}

void DlgCursor_Update(BrowserObj *o)
{
    switch (gBrowserWork->dlgCursorState) {
    case 2:
        o->state = 0;
        break;
    case 1:
        o->state = 1;
        break;
    case 3:
        o->state = 99;
        break;
    }
}

void DlgCursor_Draw(BrowserObj *o)
{
    u16 i;
    s8 hit;
    s32 color;
    f32 x;
    f32 y;
    f32 cx;
    f32 cy;
    BrowserObj *p;

    switch (o->state) {
    case 0:
        return;
    case 1:
        break;
    case 99:
        PageObj_Free(o);
        gBrowserWork->freedObjs++;
        return;
    }
    if (gBrowserState->plain != 0) {
        return;
    }
    if (gBrowserState->initMode != 0) {
        switch (gBrowserState->page) {
        case 1:
        case 3:
            return;
        }
        if (gBrowserState->fade != 0) {
            return;
        }
    }
    if ((gBrowserState->style & 1) == 0) {
        if (gBrowserWork->focus == 0) {
            gBrowserSub->cursorX = gBrowserState->marginLeft * 2;
            gBrowserSub->cursorY = 0x19E;
        } else {
            if (gBrowserWork->cursorOnLink != 0) {
                o->blink = 0;
                gBrowserWork->cursorOnLink = 0;
            }
            if (o->blink <= 0) {
                o->blinkStep = 15;
            }
            if (o->blink >= 0xFF) {
                o->blinkStep = -15;
            }
            o->blink += o->blinkStep;
            if (gBrowserWork->dlgBoxKind != 1) {
                return;
            }
            Page_DrawFocus((struct PageElem *)o);
        }
        return;
    }
    switch (gBrowserWork->dlgStatusState) {
    case 3:
        hit = 1;
        break;
    default:
        hit = 0;
        break;
    }
    if (gBrowserSub->cursorX > gBrowserState->marginLeft + 0x248) {
        hit = 0;
    }
    if (gBrowserSub->cursorY > gBrowserState->marginTop + 0x17C) {
        hit = 0;
    }
    switch (gBrowserWork->dlgBoxKind) {
    case 1:
    case 2:
        if (gBrowserWork->dlgMsgState == 0 && 0x19C - gBrowserState->marginBottom > gBrowserSub->cursorY &&
            gBrowserWork->obj[gBrowserSub->listObj] != NULL && gBrowserWork->obj[gBrowserSub->listObj]->kind != 8) {
            for (i = 0; i < 500; i++) {
                if (gBrowserWork->obj[i] == NULL) {
                    break;
                }
                if (!gBrowserWork->obj[i]->active) {
                    continue;
                }
                switch (gBrowserWork->obj[i]->kind) {
                case 17:
                case 18:
                    if ((u8)*gBrowserWork->obj[i]->s1 == 0) {
                        continue;
                    }
                    goto check;
                case 13:
                    if (gBrowserWork->obj[i]->active == 3) {
                        goto check;
                    }
                    if (gBrowserWork->obj[i]->active != 5) {
                        continue;
                    }
                case 2:
                case 6:
                case 7:
                case 8:
                case 3:
                case 4:
                case 5:
                case 11:
                case 12:
                check:
                    p = gBrowserWork->obj[i];
                    x = gBrowserWork->obj[i]->x - (f32)gBrowserState->x;
                    cx = gBrowserSub->cursorX;
                    if (cx < x) {
                        continue;
                    }
                    if (!(cx <= gBrowserWork->obj[i]->w + x)) {
                        continue;
                    }
                    y = p->y - (f32)gBrowserState->y;
                    cy = gBrowserSub->cursorY;
                    if (cy <= y) {
                        continue;
                    }
                    if (!(cy < p->h + y)) {
                        continue;
                    }
                    if (gBrowserWork->cursorOnLink != 0) {
                        o->blink = 0;
                        gBrowserWork->cursorOnLink = 0;
                    }
                    if (o->blink <= 0) {
                        o->blinkStep = 15;
                    }
                    if (o->blink >= 0xFF) {
                        o->blinkStep = -15;
                    }
                    o->blink += o->blinkStep;
                    color = o->blinkStep > 0 ? 0xFFFE2B2C : -1;
                    Page_DrawFrame(color, 3, gBrowserWork->obj[i]->x - (f32)gBrowserState->x - 1.0f,
                                   gBrowserWork->obj[i]->y - (f32)gBrowserState->y - 1.0f,
                                   3.0f + (gBrowserWork->obj[i]->x + gBrowserWork->obj[i]->w - (f32)gBrowserState->x),
                                   3.0f + (gBrowserWork->obj[i]->y + gBrowserWork->obj[i]->h - (f32)gBrowserState->y));
                    if (gBrowserWork->obj[i]->kind == 2) {
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
                    hit = 1;
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
    switch (hit) {
    case 0:
        Page_DrawMark(0, gBrowserSub->cursorX, gBrowserSub->cursorY);
        break;
    case 1:
        Page_DrawMark(1, gBrowserSub->cursorX, gBrowserSub->cursorY);
        break;
    }
}

void DlgBox_DrawAll(void)
{
    PageObj_DrawLayer(0x6);
}

/* Dialog kind 8 (layer 6): the dialog box (yes / no ...). */
void DlgBox_Create(s32 mode)
{
    BrowserObj *o;

    switch (mode) {
    case 1:
        o = PageObj_Alloc(6, 0);
        if (o != NULL) {
            o->visible = 1;
            o->active = 1;
            o->update = DlgBox_Update;
            o->draw = DlgBox_Draw;
            o->x = 213.0f;
            o->y = 128.0f;
            o->unk4C = 0;
            o->unk50 = 0;
            o->w = 213.0f;
            o->h = 128.0f;
            o->state = 0;
            o->boxStep = 1;
            o->blink = 0;
            o->blinkStep = 0;
            o->unkE = 0;
            o->unk10 = 0;
            gBrowserWork->dlgBoxKind = 1;
        }
        break;
    }
}

void DlgBox_Update(BrowserObj *o)
{
    switch (gBrowserWork->dlgBoxKind) {
    case 1:
        if (gBrowserState->page == 8) {
            o->state = 1;
        } else {
            o->state = 0;
        }
        break;
    case 2:
        o->state = 1;
        o->x = 213.0f;
        o->y = 128.0f;
        o->w = 213.0f;
        o->h = 128.0f;
        break;
    case 3:
        o->state = 2;
        o->x = 0x1F0 - gBrowserState->marginLeft;
        o->y = 0x16C - gBrowserState->marginBottom;
        o->w = 128.0f;
        o->h = 64.0f;
        break;
    case 0:
    case 4:
    case 5:
    case 6:
    case 9:
    case 10:
    default:
        o->state = 3;
        o->x = 60.0f;
        o->y = 80.0f;
        o->w = 640.0f - 2.0f * o->x;
        o->h = 448.0f - 2.0f * o->y;
        break;
    case 7:
        o->state = 4;
        o->x = 175.0f;
        o->y = 102.0f;
        o->w = 290.0f;
        o->h = 244.0f;
        break;
    case 8:
        o->state = 5;
        o->x = 175.0f;
        o->y = 102.0f;
        o->w = 290.0f;
        o->h = 244.0f;
        break;
    case 11:
        o->state = 7;
        o->x = 130.0f;
        o->y = 82.0f;
        o->w = 380.0f;
        o->h = 284.0f;
        break;
    case 12:
        o->state = 99;
        break;
    }
}

void DlgBox_Draw(BrowserObj *o)
{
    s16 c;

    Browser_SetRenderState(0x6D, 7);
    switch (o->state) {
    case 3:
    case 4:
    case 5:
    case 7:
        switch (o->boxStep) {
        case 1:
            if (o->blinkStep <= 254) {
                o->blinkStep += 15;
            } else {
                o->boxStep = 2;
            }
            break;
        case 2:
            if (o->blinkStep > 0) {
                o->blinkStep -= 15;
            } else {
                o->boxStep = 1;
            }
            break;
        }
        break;
    }
    switch (o->state) {
    case 0:
        o->blink = 0;
        break;
    case 1:
    case 2:
        if (o->blink++ > 600) {
            o->blink = 0;
        }
        if (o->blink >= 0) {
            switch (o->boxStep) {
            case 1:
                if (o->blinkStep <= 254) {
                    o->blinkStep += 15;
                } else {
                    o->boxStep = 2;
                }
                break;
            case 2:
                if (o->blinkStep > 0) {
                    o->blinkStep -= 15;
                } else {
                    o->boxStep = 1;
                }
                break;
            }
            if (func_007D21E0() == 0) {
                c = o->blinkStep;
                Prim_DrawSprite(c | (((c << 16) | 0xFF000000) | (c << 8)), PageTex_Get(0), 336, 362, 224, 32, 0, 224, 256, 256);
            } else {
                c = o->blinkStep;
                LobbyLogin_DrawWaveTitle(c | (((c << 16) | 0xFF000000) | (c << 8)));
            }
        }
        break;
    case 3:
        BrowserDlg_DrawStatus((struct PageElem *)o);
        break;
    case 4:
        BrowserDlg_DrawYesNo((struct PageElem *)o);
        break;
    case 5:
        BrowserDlg_DrawOk((struct PageElem *)o);
        break;
    case 7:
        BrowserDlg_DrawCrsTop((struct PageElem *)o);
        break;
    case 99:
        PageObj_Free(o);
        gBrowserWork->freedObjs++;
        return;
    }
}

/* "", Browser_KeepLogin's: a second "" in this file (Browser_OpenKeyboard has
 * the first), so by name: probably another of the game's units from here. */
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_72C7C0", D_00838FE8);
extern char D_00838FE8[];

/* Browser_KeepLogin: the login id and password to keep (loginId,
 * password), from the form's fields by what the form was
 * (BrowserWork.formKind); 0x52 clears them all. */
void Browser_KeepLogin(void)
{
    switch (gBrowserWork->formKind) {
    case 0xB:
        strcpy((char *)gBrowserState->loginId, gBrowserState->field1);
        strcpy((char *)gBrowserState->password, D_00838FE8);
        break;
    case 0xC:
        strcpy((char *)gBrowserState->loginId, gBrowserState->field1);
        strcpy((char *)gBrowserState->password, gBrowserState->field2);
        break;
    case 0x1F:
        strcpy((char *)gBrowserState->loginId, gBrowserState->loginIdSave);
        strcpy((char *)gBrowserState->password, D_00838FE8);
        break;
    case 0x20:
        strcpy((char *)gBrowserState->loginId, gBrowserState->loginIdSave);
        strcpy((char *)gBrowserState->password, gBrowserState->field3);
        break;
    case 0x33:
        strcpy((char *)gBrowserState->loginId, gBrowserState->field1);
        strcpy((char *)gBrowserState->password, D_00838FE8);
        break;
    case 0x34:
        strcpy((char *)gBrowserState->password, gBrowserState->field3);
        break;
    case 0x50:
        break;
    case 0x52:
        memset(gBrowserState->loginId, 0, sizeof(gBrowserState->loginId));
        memset(gBrowserState->password, 0, sizeof(gBrowserState->password));
        memset(gBrowserState->field1, 0, sizeof(gBrowserState->field1));
        memset(gBrowserState->field2, 0, sizeof(gBrowserState->field2));
        break;
    }
}

/* Browser_SaveLogin: a step a frame (BrowserWork.keepLoginStep): the login id
 * and password kept (Browser_KeepLogin) go into sub_main's copy and are
 * saved (LobbyCard_RunNetSave); on failure the old ones come back. */
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
        memcpy(gBrowserWork->savedId, gMmbbLoginId, sizeof(gBrowserWork->savedId));
        memcpy(gBrowserWork->savedPass, D_5BE6BB, sizeof(gBrowserWork->savedPass));
        Browser_KeepLogin();
        memcpy(gMmbbLoginId, gBrowserState->loginId, sizeof(gBrowserWork->savedId));
        memcpy(D_5BE6BB, gBrowserState->password, sizeof(gBrowserWork->savedPass));
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
            memcpy(gMmbbLoginId, gBrowserWork->savedId, sizeof(gBrowserWork->savedId));
            memcpy(D_5BE6BB, gBrowserWork->savedPass, sizeof(gBrowserWork->savedPass));
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
