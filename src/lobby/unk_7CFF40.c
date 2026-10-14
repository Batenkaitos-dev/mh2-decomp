#include "common.h"
#include "window.h"
#include "net_mode.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "sprite.h"




typedef struct Spr {
    s16 x, y, w, h;
    u32 color;
    UV uv0;
    UV uv1;
} Spr;

/* a box of the screen (NetBox_StepOpen's opening animation, the bars) */
typedef struct NetBox {
    s32 unk0;
    s16 x, y, w, h;
} NetBox;

/* a sprite of the screen's tables (LobbyLogin_DrawSprites, 0x18 bytes) */
typedef struct NetSprDef {
    s16 tex;                    /* NetModeState.tex[] */
    s16 pad2;
    s16 x, y, w, h;
    s16 u0, v0, u1, v1;
    u32 color;
} NetSprDef;

extern UV gLoginIconRects[][2];
extern s32 gLoginIconTex;
extern char *gNetModeTitleTexts[];
s32 Gs_GetState(u8 reg);
s32 Gs_SetState(u8, u32);
s32 Tex_Use(s32 n, u32 *handles);
struct Sprite;
s32 Sprite_Draw(struct Sprite *);
s32 Str_Length(char *s);
u8 LoginFont_GetColumn(s32 err);
void NetMode_DrawBigText(char *str, s32 x, s32 y, s32 size);

void *memcpy(void *dst, const void *src, u32 n);

extern s32 D_0081A0C8[];
extern s32 D_0081A0CA[];
extern s32 gNetModeSprDefs[];
void NetModeMsg_Queue(s32 kind, s32 arg, s32 id);

/* prototypes */
void FreeList_Init(s32 n, u32 *p);
void Gfx_ResetRenderStates(s32 partial);
void Text_DrawBoxes(s32 layer);
void Text_Reset(void);
void func_007CFF30(void);
void NetMode_DrawPart(s32);
void LobbyMsg_DrawPage(s32 page, s32 part);
void func_007D4730(s32 unused, s32 unused2);
void NetMode_PrintField(s32 a, s32 b);
void LobbyMsg_DrawPage4(s32 page, s32 part);
void func_007D44B0(s32 unused, s32 unused2);
void LobbyMsg_DrawList(s32 a, s32 b);
void LobbyMsg_DrawCountdown(u8 mode);
void Font_SetSize(f32 scale);
void Font_DrawPage(s32 layer);
void Gfx_SetState63(s32);
void Text_DrawPage(s32);
void Task_RunList(u32 *list);
/* end of prototypes */

/* queues message id with argument arg: kind 0 in the first queue, kind 5
 * (always) or 0 in the second */
void NetModeMsg_Queue(s32 kind, s32 arg, s32 id)
{
    NetModeState *st;

    st = &gNetMode;
    switch (kind) {
    case 0:
        if (st->nA < 8) {
            st->idA[st->nA] = id;
            st->kindA[st->nA] = 0;
            st->argA[st->nA] = arg;
            st->nA++;
        }
        break;
    case 5:
        st->idB[st->nB] = id;
        st->kindB[st->nB] = 5;
        st->argB[st->nB] = arg;
        st->nB++;
        break;
    default:
        if (st->nB < 8) {
            st->idB[st->nB] = id;
            st->kindB[st->nB] = 0;
            st->argB[st->nB] = arg;
            st->nB++;
        }
        break;
    }
}

void NetModeMsg_Page0(s32 a0)
{
    NetModeMsg_Queue(0x1, 0, a0);
}

void NetModeMsg_Page2(s32 a0)
{
    NetModeMsg_Queue(0x1, 0x2, a0);
}

/* the x of box `which`'s cursor (0x5D4700 or 0x5D4702) for its layout */
void NetMode_SetCursorX(s8 which)
{
    s16 x;
    s16 layout;

    x = 0;
    if (which == 0) {
        layout = gNetMode.layout0;
    } else {
        layout = gNetMode.layout1;
    }
    switch (layout) {
    case 0:
        if (gNetMode.yesNo == 0) {
            x = 0x42;
        }
        if (gNetMode.yesNo == 1) {
            x = 0x6E;
        }
        break;
    case 1:
        if (gNetMode.sel3 == 0) {
            x = -0x58;
        }
        if (gNetMode.sel3 == 1) {
            x = -0x10;
        }
        if (gNetMode.sel3 == 2) {
            x = 0x38;
        }
        break;
    case 2:
        if (gNetMode.sel3 == 0) {
            x = -0x58;
        }
        if (gNetMode.sel3 == 1) {
            x = -0x10;
        }
        if (gNetMode.sel3 == 2) {
            x = 0x38;
        }
        break;
    case 4:
        if (gNetMode.sel2 == 0) {
            x = 0x42;
        }
        if (gNetMode.sel2 == 1) {
            x = 0x6E;
        }
        break;
    }
    if (which == 0) {
        gNetMode.cursorX0 = x;
    } else {
        gNetMode.cursorX1 = x;
    }
}

/* draws the messages of queue `which` (0: A, else B) and empties it; per
 * message its kind picks the drawer: 0 a help page part, 1 func_007D4730,
 * 2 the percentage (NetMode_PrintField), 3 page 4 (LobbyMsg_DrawPage4), 4
 * func_007D44B0, 5 LobbyMsg_DrawList, 6 the countdown and page 6 part 1 */
void NetModeMsg_Flush(s8 which)
{
    NetModeState *st;
    u8 *kind;
    u8 *arg;
    u8 *id;
    s32 i;
    s32 n;

    st = &gNetMode;
    if (which == 0) {
        n = st->nA;
        kind = st->kindA;
        arg = st->argA;
        id = st->idA;
        st->nA = 0;
    } else {
        n = st->nB;
        kind = st->kindB;
        arg = st->argB;
        id = st->idB;
        st->nB = 0;
    }
    for (i = 0; i < n; i++, kind++, arg++, id++) {
        switch (*kind) {
        case 0:
            LobbyMsg_DrawPage(*arg, *id);
            break;
        case 1:
            func_007D4730(*arg, *id);
            break;
        case 2:
            NetMode_PrintField(*arg, *id);
            break;
        case 3:
            LobbyMsg_DrawPage4(*arg, *id);
            break;
        case 4:
            func_007D44B0(*arg, *id);
            break;
        case 5:
            LobbyMsg_DrawList(*arg, *id);
            break;
        case 6:
            LobbyMsg_DrawCountdown(*id);
            LobbyMsg_DrawPage(6, 1);
            break;
        }
    }
}

void NetMode_DrawScreen(s32 arg0)
{
    s32 v;

    if (++gNetMode.rowWait > 4) {
        gNetMode.rowWait = 0;
        if (++gNetMode.row >= 0x10) {
            gNetMode.row = 0;
        }
    }
    Gfx_ResetRenderStates(1);
    Gs_SetState(1, 0);
    Gs_SetState(0x15, 2);
    Gs_SetState(0xC, 0);
    Gs_SetState(0x62, 0);
    if (gNetMode.boxes & 1) {
        NetMode_DrawPart(1);
    }
    Gfx_SetState63(0);
    if (gNetMode.boxes & 2) {
        NetMode_DrawPart(2);
    }
    if (gNetMode.boxes & 0x80000) {
        NetMode_DrawPart(0x80000);
    }
    if (gNetMode.boxes & 4) {
        NetMode_DrawPart(4);
    }
    if (gNetMode.boxes & 8) {
        NetMode_DrawPart(8);
    }
    if (gNetMode.boxes & 0x10) {
        NetMode_DrawPart(0x10);
    }
    if (gNetMode.boxes & 0x4000) {
        NetMode_DrawPart(0x4000);
    }
    if (arg0 == 1) {
        v = Gs_GetState(0x6C);
        Gs_SetState(0x6C, 0);
        Task_RunList(gDrawListText4);
        Gfx_SetState63(1);
        Text_DrawPage(4);
        Text_DrawBoxes(4);
        Gs_SetState(0x6C, v);
        FreeList_Init(1, gDrawListText4);
    }
    Font_SetSize(621.4286f);
    NetModeMsg_Flush(0);
    Gs_SetState(0x6C, 0);
    Font_DrawPage(4);
    Text_DrawBoxes(4);
    Gs_SetState(0x6C, 0);
    if (gNetMode.boxes & 0x20) {
        NetMode_DrawPart(0x20);
    }
    if (gNetMode.boxes & 0x40000) {
        NetMode_DrawPart(0x40000);
    }
    if (gNetMode.boxes & 0x40) {
        Gfx_SetState63(1);
        NetMode_DrawPart(0x40);
        Gfx_SetState63(0);
    }
    if (gNetMode.boxes & 0x800000) {
        Gfx_SetState63(1);
        NetMode_DrawPart(0x800000);
        Gfx_SetState63(0);
    }
    if (gNetMode.boxes & 0x80) {
        NetMode_DrawPart(0x80);
    }
    Gfx_SetState63(1);
    if (gNetMode.boxes & 0x100) {
        NetMode_DrawPart(0x100);
    }
    if (gNetMode.boxes & 0x200) {
        NetMode_DrawPart(0x200);
    }
    if (gNetMode.boxes & 0x10000) {
        NetMode_DrawPart(0x10000);
    }
    if (gNetMode.boxes & 0x20000) {
        NetMode_DrawPart(0x20000);
    }
    if (gNetMode.boxes & 0x400) {
        NetMode_DrawPart(0x400);
    }
    Gfx_SetState63(0);
    if (gNetMode.boxes & 0x800) {
        NetMode_DrawPart(0x800);
    }
    if (gNetMode.boxes & 0x1000) {
        NetMode_DrawPart(0x1000);
    }
    if (gNetMode.boxes & 0x2000) {
        NetMode_DrawPart(0x2000);
    }
    if (gNetMode.boxes & 0x8000) {
        NetMode_DrawPart(0x8000);
    }
    if (gNetMode.boxes & 0x100000) {
        NetMode_DrawPart(0x100000);
    }
    if (gNetMode.boxes & 0x400000) {
        NetMode_DrawPart(0x400000);
    }
    if (gNetMode.boxes & 0x200000) {
        NetMode_DrawPart(0x200000);
    }
    Font_SetSize(478.57144f);
    NetModeMsg_Flush(1);
    Gs_SetState(0x6C, 0);
    Font_DrawPage(3);
    Text_DrawBoxes(3);
    Gs_SetState(0x6C, 0);
    Font_DrawPage(2);
    Text_DrawBoxes(2);
    Font_DrawPage(1);
    Text_DrawBoxes(1);
    Font_DrawPage(0);
    Text_DrawBoxes(0);
    func_007CFF30();
    Gs_SetState(0x5F, 4);
    Gs_SetState(0x60, 0);
    Gs_SetState(0x5E, 0x32);
    Gs_SetState(0x6C, 1);
    Gfx_SetState63(1);
    Text_Reset();
    Gfx_ResetRenderStates(1);
}

static s32 NetMode_GetSprDefs(s32 idx)
{
    return *(M2C_UNK *)((u8 *)gNetModeSprDefs + (idx * 4));
}

static s32 NetMode_Pick3(s32 *tbl, s32 i)
{
    if (i >= 3) {
        i = 0;
    }
    return tbl[i];
}

static s32 NetMode_Pick3b(u8 *tbl, s32 i)
{
    if (i >= 3) {
        i = 0;
    }
    return tbl[i];
}

static void NetMode_CopySprDefs(void *src, void *dst, s32 n)
{
    memcpy(dst, src, n * 0x18);
}

/* the cursor x of the second box for its layout (the tests of `plain` == 0
 * are reversed, as in the original) */
static s32 NetMode_Box2CursorX(s32 plain)
{
    NetModeState *st;
    s32 x;

    st = &gNetMode;
    x = 0;
    switch (st->layout1) {
    case 1:
    case 2:
        if (plain == 0) {
            if (st->sel3 != 0) {
                x = -0x58;
            }
            if (st->sel3 != 1) {
                x = -0x10;
            }
            if (st->sel3 != 2) {
                x = 0x38;
            }
        } else {
            if (st->sel3 == 0) {
                x = -0x58;
            }
            if (st->sel3 == 1) {
                x = -0x10;
            }
            if (st->sel3 == 2) {
                x = 0x38;
            }
        }
        break;
    case 4:
        if (plain == 0) {
            if (st->sel2 != 0) {
                x = 0x42;
            }
            if (st->sel2 != 1) {
                x = 0x6E;
            }
        } else {
            if (st->sel2 == 0) {
                x = 0x42;
            }
            if (st->sel2 == 1) {
                x = 0x6E;
            }
        }
        break;
    }
    return x;
}

void NetMode_PlaceAt(NetSprDef *d, s32 idx)
{
    s32 off;

    off = idx * 4;
    d->x = *(s16 *)((u8 *)D_0081A0C8 + off);
    d->w = *(s16 *)((u8 *)D_0081A0CA + off);
}

/* the texture coordinates of list entry st->row: two columns of 8 rows,
 * 0x20 high, the second column 0x80 to the right */
static void NetMode_ListEntryUV(Spr *dst, Spr *src)
{
    NetModeState *st;

    st = &gNetMode;
    if (st->row < 8) {
        dst->uv0.u = src->uv0.u;
        dst->uv1.u = src->uv1.u;
        dst->uv0.v = src->uv0.v + (s16)(st->row << 5);
        dst->uv1.v = src->uv1.v + (s16)(st->row << 5);
    } else {
        dst->uv0.u = src->uv0.u + 0x80;
        dst->uv1.u = src->uv1.u + 0x80;
        dst->uv0.v = src->uv0.v + (s16)((st->row - 8) << 5);
        dst->uv1.v = src->uv1.v + (s16)((st->row - 8) << 5);
    }
}

/* one step of box 0x4000's opening animation: it grows to 0x230 x 0x20
 * around the screen's centre in 10 steps; returns 0 when it is done */
s32 NetBox_StepOpen(NetBox *b)
{
    NetModeState *st;
    s16 hw;
    s16 hh;

    st = &gNetMode;
    if (++st->openW > 10) {
        st->boxes &= ~0x4000;
        return 0;
    }
    hw = (b->w + st->openW * (0x230 - b->w) / 10) / 2;
    b->x = 0x140 - hw;
    b->w = hw * 2;
    st->openH++;
    if (st->openH > 6) {
        st->openH = 6;
    }
    hh = (0x20 - st->openH * 30 / 6) / 2;
    if (hh <= 0) {
        hh = 1;
    }
    b->y = 0xE0 - hh;
    b->h = hh * 2;
    return 1;
}

/* draws one frame of an animated icon (gLoginIconRects: its texture rectangles)
 * at x, with the frame of animation 0 or 1 */
void LobbyLogin_DrawIcon(NetModeState *st, s32 x, s32 second)
{
    s32 r6C;
    s32 r63;
    s32 r5E;
    UV *uv;
    Spr s;

    r6C = Gs_GetState(0x6C);
    r63 = Gs_GetState(0x63);
    r5E = Gs_GetState(0x5E);
    Gs_SetState(0x6C, 0);
    Tex_Use(1, (u32 *) &gLoginIconTex);
    Gs_SetState(4, gLoginIconTex);
    Gs_SetState(0x63, 0);
    Gs_SetState(0x5E, 0x32);
    if (second == 0) {
        uv = gLoginIconRects[st->anim0Frame];
    } else {
        uv = gLoginIconRects[st->anim1Frame];
    }

    s.x = (s16)x + 0xA0;
    s.y = 0x140;
    s.w = 0x26;
    s.h = 0x20;

    s.uv0.u = uv->u;
    s.uv0.v = uv->v;
    s.uv1.u = (++uv)->u;
    s.uv1.v = uv->v;

    s.color = 0xFFFFBD29;
    Sprite_Draw((struct Sprite *)&s);
    Gs_SetState(0x6C, r6C);
    Gs_SetState(0x63, r63);
    Gs_SetState(0x5E, r5E);
}

/* the progress bar b (its width scaled by the percentage), with a blinking
 * message and the animated icon 0 */
void LobbyLogin_DrawProgressBar(NetBox *b)
{
    NetModeState *st;
    s32 w;

    st = &gNetMode;
    st->blink++;
    if (st->blink > 90) {
        st->blink = 0;
        st->blinkOn ^= 1;
    }
    NetModeMsg_Page0(2);
    NetModeMsg_Page0(st->blinkOn == 0 ? 0xA : 0xB);
    NetModeMsg_Page0(4);
    w = b->w * gNetMode.percent / 100.0f;
    st->anim0++;
    if ((st->anim0 & 3) == 0) {
        st->anim0Frame++;
        st->anim0Frame %= 3;
    }
    LobbyLogin_DrawIcon(st, w, 0);
}

/* the same with the animated icon 1 */
void LobbyLogin_DrawProgressBar2(NetBox *b)
{
    NetModeState *st;
    s32 w;

    st = &gNetMode;
    w = b->w * st->percent / 100.0f;
    st->anim1++;
    if ((st->anim1 & 3) == 0) {
        st->anim1Frame++;
        st->anim1Frame %= 3;
    }
    LobbyLogin_DrawIcon(st, w, 1);
}

/* scales the bar's width by the percentage (at most 100) */
void NetBox_ScaleByPercent(NetBox *b)
{
    if (gNetMode.percent > 100) {
        gNetMode.percent = 100;
    }
    b->w = (s16)(b->w * gNetMode.percent) / 100.0f;
}

/* message 1 blinking (30 frames on, 60 off) while unk7F is 1, then
 * messages 2 and 4 */
void NetMode_DrawBlinkMsg(void)
{
    NetModeState *st;

    st = &gNetMode;
    if (st->netResult == 1) {
        st->blink++;
        if (st->blink >= (st->blinkOn == 0 ? 0x3C : 0x1E)) {
            st->blink = 0;
            st->blinkOn ^= 1;
        }
    } else if (st->netResult != 0) {
        st->blinkOn = 0;
    }
    if (st->blinkOn == 0) {
        NetModeMsg_Page2(1);
    }
    NetModeMsg_Page2(2);
    NetModeMsg_Page2(4);
}

/* the error's message (LoginFont_GetColumn), then a second line by its range */
void NetMode_DrawError(void)
{
    NetModeState *st;
    s32 id;

    st = &gNetMode;
    NetModeMsg_Page2(LoginFont_GetColumn(st->err));
    if (st->err != 0x7F) {
        if (st->err <= -601) {
            id = 5;
        } else if (st->err <= -701) {
            id = 6;
        } else if (st->err <= -801) {
            id = 7;
        } else {
            id = 5;
        }
        NetModeMsg_Page2(id);
    }
}

/* A colour's alpha byte from a float. */
inline u32 Alpha8(f32 a)
{
    return (u32)a & 0xFF;
}

/* A colour from its four bytes. */
inline u32 Rgba(u32 r, u32 g, u32 b, u32 a)
{
    return (a << 24) | (b << 16) | (g << 8) | r;
}

/* the fades of the two sprites d[0] and d[1] (alpha in the colour's top
 * byte) by the animation step anim0: 24 frames in, 24 out, 8 across */
static void NetMode_FadeSprites(NetSprDef *d)
{
    NetModeState *st;

    st = &gNetMode;
    switch (st->anim0) {
    case 0:
        if (st->netResult == 1) {
            st->anim0++;
            st->anim0Frame = 0;
        }
        d[0].color = 0xFFFFFF;
        d[1].color = 0xFFFFFF;
        break;
    case 1:
        st->anim0Frame++;
        if (st->anim0Frame > 24) {
            st->anim0++;
            d[0].color = 0xFFFFFFFF;
            d[1].color = 0xFFFFFF;
            break;
        }
        d[0].color = Rgba(0xFF, 0xFF, 0xFF, Alpha8(255.0f * st->anim0Frame / 24.0f));
        d[1].color = 0xFFFFFF;
        break;
    case 2:
        switch (st->netResult) {
        case 2:
            st->anim0 = 5;
            st->anim0Frame = 0;
            break;
        case 3:
            st->anim0++;
            st->anim0Frame = 0;
            break;
        }
        d[0].color = 0xFFFFFFFF;
        d[1].color = 0xFFFFFF;
        break;
    case 3:
        st->anim0Frame++;
        if (st->anim0Frame > 24) {
            st->anim0++;
            st->anim0Frame = 0;
            d[0].color = 0xFFFFFF;
        } else {
            d[0].color = Rgba(0xFF, 0xFF, 0xFF, Alpha8(255.0f * (24 - st->anim0Frame) / 24.0f));
        }
        d[1].color = 0xFFFFFF;
        break;
    case 4:
        d[0].color = 0xFFFFFF;
        d[1].color = 0xFFFFFF;
        break;
    case 5:
        st->anim0Frame++;
        if (st->anim0Frame > 8) {
            st->anim0++;
            st->anim0Frame = 0;
            d[0].color = 0xFFFFFF;
            d[1].color = 0xFFFFFFFF;
            break;
        }
        d[0].color = Rgba(0xFF, 0xFF, 0xFF, Alpha8(255.0f * (8 - st->anim0Frame) / 8.0f));
        d[1].color = Rgba(0xFF, 0xFF, 0xFF, Alpha8(255.0f * st->anim0Frame / 8.0f));
        break;
    case 6:
        if (st->netResult != 2) {
            st->anim0++;
            st->anim0Frame = 0;
        }
        d[0].color = 0xFFFFFF;
        d[1].color = 0xFFFFFFFF;
        break;
    case 7:
        st->anim0Frame++;
        if (st->anim0Frame > 24) {
            st->anim0++;
            st->anim0Frame = 0;
            d[1].color = 0xFFFFFF;
        } else {
            d[1].color = Rgba(0xFF, 0xFF, 0xFF, Alpha8(255.0f * (24 - st->anim0Frame) / 24.0f));
        }
        d[0].color = 0xFFFFFF;
        break;
    case 8:
        d[0].color = 0xFFFFFF;
        d[1].color = 0xFFFFFF;
        break;
    }
}

/* a window of gNetModeIcons for Window_DrawTextAlpha (sub_main; 0x14 bytes) */
typedef struct NetIcon {
    Window win;
    u32 unk10;                  /* 0x10: 0 in every entry */
} NetIcon;


extern s32 D_0081A098[];
extern u8 D_0081A0A8[];
extern s32 D_0081A0B0[];
extern u8 D_0081A0C0[];
extern NetIcon gNetModeIcons[];
extern ListWin gNetModeWin;

void LobbyLogin_DrawSprites(NetSprDef *d, s32 n, s32 dy);
void NetMode_DrawTitle(void);
void NetModeMsg_QueueField(u8 id);
void Window_DrawTextAlpha(Window *w, u8 *text, u8 alpha);
struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, s8 *title, s32 sel, u8 alpha);
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
char **LobbyMsg_GetLines(s32 page, s32 part);
s32 LobbyMsg_CenterX(char *str, s32 size);

/* draws the screen's part `part` (a bit of NetModeState.boxes) with the
 * sprite buffer at *(0x6ADFE8) */
void NetMode_DrawPart(s32 part)
{
    NetSprDef *buf;
    NetModeState *st;
    s32 src;
    s32 n;
    s32 k;
    s32 i;
    s16 x;
    s32 y;
    f32 f;
    NetIcon icon;
    NetIcon icon2;
    ListWin win;

    buf = *(NetSprDef * *)&gLoadBuf;
    st = &gNetMode;
    switch (part) {
    case 1:
        LobbyLogin_DrawSprites((NetSprDef *)NetMode_GetSprDefs(0), 1, 0);
        break;
    case 2:
        LobbyLogin_DrawSprites((NetSprDef *)NetMode_GetSprDefs(1), 2, 0);
        break;
    case 0x80000:
        NetMode_DrawTitle();
        break;
    case 4:
        i = st->unk24;
        src = NetMode_Pick3(D_0081A0B0, i);
        n = NetMode_Pick3b(D_0081A0C0, i);
        k = NetMode_Box2CursorX(0);
        goto draw;
    case 8:
        i = st->unk24;
        src = NetMode_Pick3(D_0081A098, i);
        n = NetMode_Pick3b(D_0081A0A8, i);
        k = NetMode_Box2CursorX(1);
    draw:
        if (k != 0) {
            NetMode_CopySprDefs((void *)src, buf, n);
            LobbyLogin_DrawSprites(buf, n, k);
        }
        break;
    case 0x10:
        NetMode_CopySprDefs((void *)NetMode_GetSprDefs(2), buf, 1);
        NetMode_PlaceAt(buf, st->unk24);
        NetMode_ListEntryUV((Spr *)buf, (Spr *)NetMode_GetSprDefs(2));
        st->cursorX0 = 0x8000;
        NetMode_SetCursorX(0);
        if (st->cursorX0 != 0x8000) {
            LobbyLogin_DrawSprites(buf, 1, st->cursorX0);
        }
        break;
    case 0x4000:
        NetMode_CopySprDefs((void *)NetMode_GetSprDefs(2), buf, 1);
        NetMode_PlaceAt(buf, st->unk24);
        NetMode_ListEntryUV((Spr *)buf, (Spr *)NetMode_GetSprDefs(2));
        if (NetBox_StepOpen((NetBox *)buf) == 1) {
            LobbyLogin_DrawSprites(buf, 1, st->cursorX0);
        }
        break;
    case 0x20:
        i = st->icon;
        icon = gNetModeIcons[i];
        y = st->iconY;
        icon.win.y += y;
        Window_DrawTextAlpha(&icon.win, NULL, 0x80);
        break;
    case 0x40000:
        i = st->icon2;
        icon2 = gNetModeIcons[i];
        y = st->iconY;
        icon2.win.y += y;
        Window_DrawTextAlpha(&icon2.win, NULL, 0x80);
        break;
    case 0x40:
        NetMode_CopySprDefs((void *)NetMode_GetSprDefs(8), buf, 4);
        LobbyLogin_DrawProgressBar((NetBox *)&buf[3]);
        break;
    case 0x800000:
        NetMode_CopySprDefs((void *)NetMode_GetSprDefs(8), buf, 4);
        LobbyLogin_DrawProgressBar2((NetBox *)&buf[3]);
        break;
    case 0x80:
        NetMode_CopySprDefs((void *)NetMode_GetSprDefs(8), buf, 4);
        NetBox_ScaleByPercent((NetBox *)&buf[3]);
        LobbyLogin_DrawSprites(buf, 4, 0);
        NetModeMsg_QueueField(5);
        break;
    case 0x100:
        NetMode_DrawBlinkMsg();
        break;
    case 0x200:
        NetMode_CopySprDefs((void *)NetMode_GetSprDefs(0xA), buf, 2);
        NetMode_FadeSprites(buf);
        LobbyLogin_DrawSprites(buf, 2, 0);
        break;
    case 0x10000:
        break;
    case 0x20000:
        NetMode_DrawError();
        break;
    case 0x400:
        LobbyLogin_DrawSprites((NetSprDef *)NetMode_GetSprDefs(0xB), 2, 0);
        break;
    case 0x800:
        i = st->unk24;
        src = NetMode_Pick3(D_0081A0B0, i);
        n = NetMode_Pick3b(D_0081A0C0, i);
        k = NetMode_Box2CursorX(0);
        goto draw;
    case 0x1000:
        i = st->unk24;
        src = NetMode_Pick3(D_0081A098, i);
        n = NetMode_Pick3b(D_0081A0A8, i);
        k = NetMode_Box2CursorX(1);
        goto draw;
    case 0x2000:
        NetMode_CopySprDefs((void *)NetMode_GetSprDefs(2), buf, 1);
        NetMode_PlaceAt(buf, st->unk24);
        NetMode_ListEntryUV((Spr *)buf, (Spr *)NetMode_GetSprDefs(2));
        st->cursorX1 = 0x8000;
        NetMode_SetCursorX(1);
        if (st->cursorX1 != 0x8000) {
            LobbyLogin_DrawSprites(buf, 1, st->cursorX1);
        }
        break;
    case 0x8000:
        NetMode_CopySprDefs((void *)NetMode_GetSprDefs(2), buf, 1);
        NetMode_PlaceAt(buf, st->unk24);
        NetMode_ListEntryUV((Spr *)buf, (Spr *)NetMode_GetSprDefs(2));
        if (NetBox_StepOpen((NetBox *)buf) == 1) {
            LobbyLogin_DrawSprites(buf, 1, st->cursorX1);
        }
        break;
    case 0x100000:
        x = LobbyMsg_CenterX(LobbyMsg_GetLines(0, 0xE)[2], 0x14);
        if (st->yesNo == 0) {
            x = x + 0x14;
        } else {
            f = 30.0f;
            x = (s16)f + (x + 100);
        }
        Menu_DrawGlowBar(x, 0x142, 0x78, 0x18, 0xFF20C0C0);
        break;
    case 0x400000:
        x = LobbyMsg_CenterX(LobbyMsg_GetLines(0, 0xE)[2], 0x12);
        if (st->yesNo == 0) {
            x = x + 0x12;
        } else {
            f = 27.0f;
            x = (s16)f + (x + 90);
        }
        Menu_DrawGlowBar(x, 0x142, 0x6C, 0x16, 0xFF20C0C0);
        break;
    case 0x200000:
        win = gNetModeWin;
        ListPanel_DrawAlpha(&win, 0, -1, 0x80);
        y = 0x118;
        if (st->sel2 == 1) {
            y += 0x2C;
        }
        Menu_DrawGlowBar(0x140, y - 2, 0x132, 0x16, 0xFF20C0C0);
        break;
    }
}

/* draws n sprites of the table d (0x18 bytes each), moved down by dy */
void LobbyLogin_DrawSprites(NetSprDef *d, s32 n, s32 dy)
{
    NetModeState *st;
    s32 i;
    Spr s;
    s32 tex;

    st = &gNetMode;
    for (i = 0; i < n; i++, d++) {
        tex = st->tex[d->tex];
        if (tex != 0) {
            Tex_Use(1, (u32 *) &tex);
            Gs_SetState(4, tex);
            s.x = d->x;
            s.y = d->y;
            s.y += (s16)dy;
            s.w = d->w;
            s.h = d->h;
            s.uv0.u = d->u0;
            s.uv0.v = d->v0;
            s.uv1.u = d->u1;
            s.uv1.v = d->v1;
            s.uv1.u -= 1;
            s.uv1.v -= 1;
            s.color = d->color;
            Sprite_Draw((struct Sprite *)&s);
        }
    }
}

/* the title of screen unk6B (1-7) */
void NetMode_DrawTitle(void)
{
    NetModeState *st;

    st = &gNetMode;
    if (st->title <= 0 || st->title >= 8) {
        return;
    }
    NetMode_DrawBigText(gNetModeTitleTexts[st->title], -1, 0x1C, 0x22);
}

/* draws str (capitals and spaces) with the font texture tex[11], size x
 * size per letter; x -1 centres it */
/* FAKEMATCH: Str_Peek and the copy q exist only to make MWCC load *str again
   in the loop body (a plain `while (*str) { if (*str == 0x20)` reuses the
   test's load: 3 words); from a permuter find (round 15). */
static inline char Str_Peek(char *s)
{
    return *s;
}

void NetMode_DrawBigText(char *str, s32 x, s32 y, s32 size)
{
    NetModeState *st;
    Spr s;
    s32 c;
    s32 w;
    char *q;

    st = &gNetMode;
    if (str == NULL) {
        return;
    }
    c = Str_Length(str);
    w = c * size;
    if (x == -1) {
        x = (640.0f - w) / 2.0f;
    }
    Tex_Use(1, (u32 *) &st->tex[11]);
    Gs_SetState(4, st->tex[11]);
    s.w = s.h = size;
    while (Str_Peek(q = str)) {
        if (Str_Peek(q) == ' ') {
            x += size;
            str++;
        } else {
            c = *str - 'A';
            s.uv0.u = (c % 8) * 32;
            s.uv0.v = (c / 8) * 32;
            s.uv1.u = s.uv0.u + 0x1F;
            s.uv1.v = s.uv0.v + 0x1F;
            s.color = 0xFF2691A1;
            s.x = x;
            s.y = y;
            Sprite_Draw((struct Sprite *)&s);
            x += size;
            str++;
        }
    }
}

/* unk_7CFF40's .data (0x819870-0x81A300) */
extern char gFullWidthDigits_0[];
UV gLoginIconRects[4][2] = {
#include "lobby/00819870.inc"
};
ListWin gNetModeWin = {
#include "lobby/00819890.inc"
};
u32 gWaveTitleSine[12] = {
#include "lobby/008198B0.inc"
};
u32 D_008198E0[20] = {
#include "lobby/008198E0.inc"
};
char *gFullWidthDigits[4] = {
    gFullWidthDigits_0, NULL, NULL, NULL,
};
u32 gNetModeSprDefs_0[8] = {
#include "lobby/00819940.inc"
};
u32 gNetModeSprDefs_1[12] = {
#include "lobby/00819960.inc"
};
u32 gNetModeSprDefs_2[8] = {
#include "lobby/00819990.inc"
};
u32 gNetModeSprDefs_5[48] = {
#include "lobby/008199B0.inc"
};
u32 gNetModeSprDefs_7[8] = {
#include "lobby/00819A70.inc"
};
u32 gNetModeSprDefs_8[24] = {
#include "lobby/00819A90.inc"
};
u32 gNetModeSprDefs_9[8] = {
#include "lobby/00819AF0.inc"
};
u32 gNetModeSprDefs_10[12] = {
#include "lobby/00819B10.inc"
};
u32 gNetModeSprDefs_11[12] = {
#include "lobby/00819B40.inc"
};
s32 gNetModeSprDefs[12] = {
    (s32)gNetModeSprDefs_0, (s32)gNetModeSprDefs_1, (s32)gNetModeSprDefs_2, 0,
    0, (s32)gNetModeSprDefs_5, 0, (s32)gNetModeSprDefs_7,
    (s32)gNetModeSprDefs_8, (s32)gNetModeSprDefs_9, (s32)gNetModeSprDefs_10, (s32)gNetModeSprDefs_11,
};
u32 D_00819BA0[56] = {
#include "lobby/00819BA0.inc"
};
u32 D_00819C80[60] = {
#include "lobby/00819C80.inc"
};
u32 D_00819D70[44] = {
#include "lobby/00819D70.inc"
};
u32 D_00819E20[56] = {
#include "lobby/00819E20.inc"
};
u32 D_00819F00[60] = {
#include "lobby/00819F00.inc"
};
u32 D_00819FF0[42] = {
#include "lobby/00819FF0.inc"
};
s32 D_0081A098[3] = {
    (s32)D_00819BA0, (s32)D_00819C80, (s32)D_00819D70,
};
u8 D_0081A0A8[8] = {
#include "lobby/0081A0A8.inc"
};
s32 D_0081A0B0[4] = {
    (s32)D_00819E20, (s32)D_00819F00, (s32)D_00819FF0, 0,
};
u8 D_0081A0C0[8] = {
#include "lobby/0081A0C0.inc"
};
s32 D_0081A0C8[3] = {
#include "lobby/0081A0C8.inc"
};
NetIcon gNetModeIcons[8] = {
#include "lobby/0081A0E0.inc"
};
u32 gLoginFrameSprites[68] = {
#include "lobby/0081A180.inc"
};
u32 gLoginItemColors[8] = {
#include "lobby/0081A290.inc"
};
u32 gLoginLitColors[8] = {
#include "lobby/0081A2B0.inc"
};
u32 gLoginBoxSprites[8] = {
#include "lobby/0081A2D0.inc"
};
u32 gLoginPageColorIdx[4] = {
#include "lobby/0081A2F0.inc"
};
