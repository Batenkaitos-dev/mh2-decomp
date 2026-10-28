/*
 * The network screen's drawing: the message queues, the boxes, the progress
 * bars, the menus' cursors and the big-letter titles.
 *
 * Each frame NetMode_DrawScreen draws the boxes whose bits are set in
 * gNetMode.boxes (NET_BOX_*, through NetMode_DrawPart) in layers: the
 * frame, the title and menu 0, then the messages of queue A (drawn under
 * the boxes), the icons, the progress bars and the status, menu 1 and the
 * yes/no bars, then the messages of queue B. A message is queued with
 * NetModeMsg_Queue (or the helpers NetModeMsg_Page0/_Page2 and those of
 * the queue helpers' file) and drawn and dropped by NetModeMsg_Flush. Most
 * boxes are sprite sets of gNetModeSprDefs (NET_SPR_*, tables of
 * NetSprDef), copied into the load buffer and changed there: the progress
 * bar is scaled by gNetMode.percent, the cursor's texture rows step with
 * gNetMode.row, a chosen row grows across the screen (NetBox_GrowPicked)
 * and the transfer's status sprites fade by its result
 * (NetMode_FadeStatus). NetMode_DrawBigText draws a title in the capital
 * letters' font, and LobbyLogin_DrawWaveTitle, last, the login screen's
 * title in it, its letters waving (NetMode_WaveTitleTex tells whether that
 * font is loaded). The .data holds the sprite sets, the menus' row tables,
 * the icons' windows and the login screen's tables.
 */
#include "common.h"
#include "window.h"
#include "net_mode.h"
#include "lobby_globals.h"
#include "sprite.h"
#include "render_state.h"
#include "model.h"
#include "text.h"
#include "texture.h"
#include "draw_helpers.h"
#include "draw_slots.h"
#include "font.h"
#include "net_file_save.h"
#include "libc.h"
#include "engine_util.h"
#include "prim.h"






extern UV gNetBarIconRects[][2];
extern s32 gLoginIconTex;
s32 Str_Length(char *s);

extern s32 gNetCursorSpans[];
extern s16 gNetCursorWidths[];

#define NET_CURSOR_UNSET 0x8000     /* cursorY0/1 before NetMode_SetCursorY */
extern NetSprDef *gNetModeSprDefs[];

/* prototypes */
/* end of prototypes */

/* Queues part `part` of message page `page`: with NETMODE_QUEUE_BACK in
 * queue A (drawn under the boxes), with NETMODE_QUEUE_LIST in queue B as a
 * list of choices (without checking the queue's room), with any other in
 * queue B. A full queue drops it. */
void NetModeMsg_Queue(s32 where, s32 page, s32 part)
{
    NetModeState *st;

    st = &gNetMode;
    switch (where) {
    case NETMODE_QUEUE_BACK:
        if (st->nA < NETMODE_QUEUE_LEN) {
            st->idA[st->nA] = part;
            st->kindA[st->nA] = NETMODE_KIND_PAGE;
            st->argA[st->nA] = page;
            st->nA++;
        }
        break;
    case NETMODE_QUEUE_LIST:
        st->idB[st->nB] = part;
        st->kindB[st->nB] = NETMODE_KIND_LIST;
        st->argB[st->nB] = page;
        st->nB++;
        break;
    default:
        if (st->nB < NETMODE_QUEUE_LEN) {
            st->idB[st->nB] = part;
            st->kindB[st->nB] = NETMODE_KIND_PAGE;
            st->argB[st->nB] = page;
            st->nB++;
        }
        break;
    }
}

/* Queues part `part` of page 0 over the boxes. */
void NetModeMsg_Page0(s32 part)
{
    NetModeMsg_Queue(NETMODE_QUEUE_FRONT, 0, part);
}

/* Queues part `part` of page 2 over the boxes. */
void NetModeMsg_Page2(s32 part)
{
    NetModeMsg_Queue(NETMODE_QUEUE_FRONT, 2, part);
}

/* Sets menu `which`'s (0 or 1) cursor offset (cursorY0, cursorY1) for its
 * layout and the current choice: layout 0 two rows on gNetMode.yesNo,
 * layouts 1 and 2 three rows on sel3, layout 4 two rows on sel2; any other
 * layout 0. */
void NetMode_SetCursorY(s8 which)
{
    s16 y;
    s16 layout;

    y = 0;
    if (which == 0) {
        layout = gNetMode.layout0;
    } else {
        layout = gNetMode.layout1;
    }
    switch (layout) {
    case 0:
        if (gNetMode.yesNo == 0) {
            y = 66;
        }
        if (gNetMode.yesNo == 1) {
            y = 110;
        }
        break;
    case 1:
        if (gNetMode.sel3 == 0) {
            y = -88;
        }
        if (gNetMode.sel3 == 1) {
            y = -16;
        }
        if (gNetMode.sel3 == 2) {
            y = 56;
        }
        break;
    case 2:
        if (gNetMode.sel3 == 0) {
            y = -88;
        }
        if (gNetMode.sel3 == 1) {
            y = -16;
        }
        if (gNetMode.sel3 == 2) {
            y = 56;
        }
        break;
    case 4:
        if (gNetMode.sel2 == 0) {
            y = 66;
        }
        if (gNetMode.sel2 == 1) {
            y = 110;
        }
        break;
    }
    if (which == 0) {
        gNetMode.cursorY0 = y;
    } else {
        gNetMode.cursorY1 = y;
    }
}

/* Draws the messages of queue `which` (0: A, else B) and empties it. Each
 * message's kind (NETMODE_KIND_*) picks its drawer. */
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
        case NETMODE_KIND_PAGE:
            LobbyMsg_DrawPage(*arg, *id);
            break;
        case NETMODE_KIND_NONE:
            LobbyMsg_DrawNone(*arg, *id);
            break;
        case NETMODE_KIND_FIELD:
            NetMode_PrintField(*arg, *id);
            break;
        case NETMODE_KIND_PAGE4:
            LobbyMsg_DrawPage4(*arg, *id);
            break;
        case NETMODE_KIND_NONE_ALT:
            LobbyMsg_DrawNoneAlt(*arg, *id);
            break;
        case NETMODE_KIND_LIST:
            LobbyMsg_DrawList(*arg, *id);
            break;
        case NETMODE_KIND_COUNTDOWN:
            LobbyMsg_DrawCountdown(*id);
            LobbyMsg_DrawPage(6, 1);
            break;
        }
    }
}

/* Draws the network screen's boxes and messages for this frame (above),
 * and steps the cursor's texture row every fifth frame. withText 1 also
 * draws the text layer's page 4 (gDrawListText4) under queue A. */
void NetMode_DrawScreen(s32 withText)
{
    s32 oldZwrite;

    if (++gNetMode.rowWait > 4) {
        gNetMode.rowWait = 0;
        if (++gNetMode.row >= 16) {
            gNetMode.row = 0;
        }
    }
    Gfx_ResetRenderStates(1);
    Gs_SetState(1, 0);
    Gs_SetState(21, 2);
    Gs_SetState(RS_SCISSOR, 0);
    Gs_SetState(98, 0);
    if (gNetMode.boxes & NET_BOX_FRAME) {
        NetMode_DrawPart(NET_BOX_FRAME);
    }
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    if (gNetMode.boxes & NET_BOX_PANEL) {
        NetMode_DrawPart(NET_BOX_PANEL);
    }
    if (gNetMode.boxes & NET_BOX_TITLE) {
        NetMode_DrawPart(NET_BOX_TITLE);
    }
    if (gNetMode.boxes & NET_BOX_MENU0_ROWS) {
        NetMode_DrawPart(NET_BOX_MENU0_ROWS);
    }
    if (gNetMode.boxes & NET_BOX_MENU0_LIT) {
        NetMode_DrawPart(NET_BOX_MENU0_LIT);
    }
    if (gNetMode.boxes & NET_BOX_MENU0_CURSOR) {
        NetMode_DrawPart(NET_BOX_MENU0_CURSOR);
    }
    if (gNetMode.boxes & NET_BOX_MENU0_PICKED) {
        NetMode_DrawPart(NET_BOX_MENU0_PICKED);
    }
    if (withText == 1) {
        oldZwrite = Gs_GetState(RS_ZWRITE);
        Gs_SetState(RS_ZWRITE, 0);
        Task_RunList(gDrawListText4);
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        Text_DrawPage(4);
        Text_DrawIcons(4);
        Gs_SetState(RS_ZWRITE, oldZwrite);
        FreeList_Init(1, gDrawListText4);
    }
    Font_SetSize(621.4286f);
    NetModeMsg_Flush(0);
    Gs_SetState(RS_ZWRITE, 0);
    Font_DrawPage(4);
    Text_DrawIcons(4);
    Gs_SetState(RS_ZWRITE, 0);
    if (gNetMode.boxes & NET_BOX_ICON) {
        NetMode_DrawPart(NET_BOX_ICON);
    }
    if (gNetMode.boxes & NET_BOX_ICON2) {
        NetMode_DrawPart(NET_BOX_ICON2);
    }
    if (gNetMode.boxes & NET_BOX_PROGRESS) {
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        NetMode_DrawPart(NET_BOX_PROGRESS);
        Gfx_SetTexFilter(GFX_TEX_NEAREST);
    }
    if (gNetMode.boxes & NET_BOX_PROGRESS2) {
        Gfx_SetTexFilter(GFX_TEX_BILINEAR);
        NetMode_DrawPart(NET_BOX_PROGRESS2);
        Gfx_SetTexFilter(GFX_TEX_NEAREST);
    }
    if (gNetMode.boxes & NET_BOX_PERCENT) {
        NetMode_DrawPart(NET_BOX_PERCENT);
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    if (gNetMode.boxes & NET_BOX_STATUS_MSG) {
        NetMode_DrawPart(NET_BOX_STATUS_MSG);
    }
    if (gNetMode.boxes & NET_BOX_STATUS) {
        NetMode_DrawPart(NET_BOX_STATUS);
    }
    if (gNetMode.boxes & NET_BOX_STATUS_NONE) {
        NetMode_DrawPart(NET_BOX_STATUS_NONE);
    }
    if (gNetMode.boxes & NET_BOX_ERROR) {
        NetMode_DrawPart(NET_BOX_ERROR);
    }
    if (gNetMode.boxes & NET_BOX_SPARE) {
        NetMode_DrawPart(NET_BOX_SPARE);
    }
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    if (gNetMode.boxes & NET_BOX_MENU1_ROWS) {
        NetMode_DrawPart(NET_BOX_MENU1_ROWS);
    }
    if (gNetMode.boxes & NET_BOX_MENU1_LIT) {
        NetMode_DrawPart(NET_BOX_MENU1_LIT);
    }
    if (gNetMode.boxes & NET_BOX_MENU1_CURSOR) {
        NetMode_DrawPart(NET_BOX_MENU1_CURSOR);
    }
    if (gNetMode.boxes & NET_BOX_MENU1_PICKED) {
        NetMode_DrawPart(NET_BOX_MENU1_PICKED);
    }
    if (gNetMode.boxes & NET_BOX_YES_NO_LARGE) {
        NetMode_DrawPart(NET_BOX_YES_NO_LARGE);
    }
    if (gNetMode.boxes & NET_BOX_YES_NO) {
        NetMode_DrawPart(NET_BOX_YES_NO);
    }
    if (gNetMode.boxes & NET_BOX_SLOT_CHOICE) {
        NetMode_DrawPart(NET_BOX_SLOT_CHOICE);
    }
    Font_SetSize(478.57144f);
    NetModeMsg_Flush(1);
    Gs_SetState(RS_ZWRITE, 0);
    Font_DrawPage(3);
    Text_DrawIcons(3);
    Gs_SetState(RS_ZWRITE, 0);
    Font_DrawPage(2);
    Text_DrawIcons(2);
    Font_DrawPage(1);
    Text_DrawIcons(1);
    Font_DrawPage(0);
    Text_DrawIcons(0);
    NetMode_EmptyDrawEnd();
    Gs_SetState(RS_ALPHA_TEST, RS_CMP_GREATER);
    Gs_SetState(RS_ALPHA_REF, 0);
    Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    Gs_SetState(RS_ZWRITE, 1);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Text_Reset();
    Gfx_ResetRenderStates(1);
}

/* The table of sprite set `set` (NET_SPR_*). */
static NetSprDef *NetMode_GetSprDefs(s32 set)
{
    return gNetModeSprDefs[set];
}

/* Entry `set` of a menu sprite table (gNetMenuRows, gNetMenuLitRows); 3 and up read entry 0. */
static NetSprDef *NetMode_PickRows(NetSprDef **rows, s32 set)
{
    if (set >= 3) {
        set = 0;
    }
    return rows[set];
}

/* The same for the tables of their sprite counts. */
static s32 NetMode_PickRowCount(u8 *counts, s32 set)
{
    if (set >= 3) {
        set = 0;
    }
    return counts[set];
}

static void NetMode_CopySprDefs(NetSprDef *src, NetSprDef *dst, s32 count)
{
    memcpy(dst, src, count * sizeof(NetSprDef));
}

/* The y offset of menu 1's rows for its layout (gNetMode.layout1) and
 * choice: with lit 1, the chosen row's offset (as NetMode_SetCursorY); with
 * lit 0 the tests are reversed, so that the last row not chosen wins. */
static s32 NetMode_MenuRowsY(s32 lit)
{
    NetModeState *st;
    s32 y;

    st = &gNetMode;
    y = 0;
    switch (st->layout1) {
    case 1:
    case 2:
        if (lit == 0) {
            if (st->sel3 != 0) {
                y = -88;
            }
            if (st->sel3 != 1) {
                y = -16;
            }
            if (st->sel3 != 2) {
                y = 56;
            }
        } else {
            if (st->sel3 == 0) {
                y = -88;
            }
            if (st->sel3 == 1) {
                y = -16;
            }
            if (st->sel3 == 2) {
                y = 56;
            }
        }
        break;
    case 4:
        if (lit == 0) {
            if (st->sel2 != 0) {
                y = 66;
            }
            if (st->sel2 != 1) {
                y = 110;
            }
        } else {
            if (st->sel2 == 0) {
                y = 66;
            }
            if (st->sel2 == 1) {
                y = 110;
            }
        }
        break;
    }
    return y;
}

/* Places the cursor sprite d across the screen by the menus' sprite set `set` (gNetCursorSpans: its x and width). */
void NetMode_PlaceCursor(NetSprDef *d, s32 set)
{
    s32 off;

    /* the width is read through its own name, gNetCursorWidths (a linker
     * alias 2 bytes into gNetCursorSpans): the game's code loads the two
     * addresses separately */
    off = set * 4;
    d->x = *(s16 *)((u8 *)gNetCursorSpans + off);
    d->w = *(s16 *)((u8 *)gNetCursorWidths + off);
}

/* Sets dst's texture coordinates to src's moved to the cursor's texture
 * row gNetMode.row: two columns of 8 rows, 32 high, the second 128 to the
 * right. */
static void NetMode_ListEntryUV(NetSprDef *dst, NetSprDef *src)
{
    NetModeState *st;

    st = &gNetMode;
    if (st->row < 8) {
        dst->u0 = src->u0;
        dst->u1 = src->u1;
        dst->v0 = src->v0 + (s16)(st->row << 5);
        dst->v1 = src->v1 + (s16)(st->row << 5);
    } else {
        dst->u0 = src->u0 + 128;
        dst->u1 = src->u1 + 128;
        dst->v0 = src->v0 + (s16)((st->row - 8) << 5);
        dst->v1 = src->v1 + (s16)((st->row - 8) << 5);
    }
}

/* One step of a chosen row's animation (NET_BOX_MENU0_PICKED,
 * NET_BOX_MENU1_PICKED): the row b grows to 560 x 32 around the screen's
 * centre in 10 steps. Returns 1 while it runs; at the end it closes
 * NET_BOX_MENU0_PICKED (for either menu) and returns 0. */
s32 NetBox_GrowPicked(NetSprDef *b)
{
    NetModeState *st;
    s16 halfW;
    s16 halfH;

    st = &gNetMode;
    if (++st->openW > 10) {
        st->boxes &= ~NET_BOX_MENU0_PICKED;
        return 0;
    }
    halfW = (b->w + st->openW * (560 - b->w) / 10) / 2;
    b->x = 320 - halfW;
    b->w = halfW * 2;
    st->openH++;
    if (st->openH > 6) {
        st->openH = 6;
    }
    halfH = (32 - st->openH * 30 / 6) / 2;
    if (halfH <= 0) {
        halfH = 1;
    }
    b->y = 224 - halfH;
    b->h = halfH * 2;
    return 1;
}

/* Draws the progress bar's icon at x (from x 160, y 320) in its current
 * frame: icon 0 the first bar's (anim0Frame), 1 the second's (anim1Frame);
 * gNetBarIconRects are the frames' texture rectangles. */
void NetMode_DrawBarIcon(NetModeState *st, s32 x, s32 icon)
{
    s32 oldZwrite;
    s32 oldFilter;
    s32 oldBlend;
    UV *uv;
    Sprite s;

    oldZwrite = Gs_GetState(RS_ZWRITE);
    oldFilter = Gs_GetState(RS_TEX_FILTER);
    oldBlend = Gs_GetState(RS_BLEND);
    Gs_SetState(RS_ZWRITE, 0);
    Tex_Use(1, (u32 *) &gLoginIconTex);
    Gs_SetState(RS_TEXTURE, gLoginIconTex);
    Gs_SetState(RS_TEX_FILTER, 0);
    Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    if (icon == 0) {
        uv = gNetBarIconRects[st->anim0Frame];
    } else {
        uv = gNetBarIconRects[st->anim1Frame];
    }

    s.x = (s16)x + 160;
    s.y = 320;
    s.w = 38;
    s.h = 32;

    s.u0 = uv->u;
    s.v0 = uv->v;
    s.u1 = (++uv)->u;
    s.v1 = uv->v;

    s.color = 0xFFFFBD29;
    Sprite_Draw(&s);
    Gs_SetState(RS_ZWRITE, oldZwrite);
    Gs_SetState(RS_TEX_FILTER, oldFilter);
    Gs_SetState(RS_BLEND, oldBlend);
}

/* The first progress bar's extras: page 0's messages 2, 10 or 11 by turns
 * (90 frames each) and 4, and the icon moved along the bar b by the
 * percentage, its frame stepping every fourth frame. */
void NetMode_DrawProgress(NetSprDef *b)
{
    NetModeState *st;
    s32 iconX;

    st = &gNetMode;
    st->blink++;
    if (st->blink > 90) {
        st->blink = 0;
        st->blinkOn ^= 1;
    }
    NetModeMsg_Page0(2);
    NetModeMsg_Page0(st->blinkOn == 0 ? 10 : 11);
    NetModeMsg_Page0(4);
    iconX = b->w * gNetMode.percent / 100.0f;
    st->anim0++;
    if ((st->anim0 & 3) == 0) {
        st->anim0Frame++;
        st->anim0Frame %= 3;
    }
    NetMode_DrawBarIcon(st, iconX, 0);
}

/* The second bar's icon, moved along b by the percentage. */
void NetMode_DrawProgress2(NetSprDef *b)
{
    NetModeState *st;
    s32 iconX;

    st = &gNetMode;
    iconX = b->w * st->percent / 100.0f;
    st->anim1++;
    if ((st->anim1 & 3) == 0) {
        st->anim1Frame++;
        st->anim1Frame %= 3;
    }
    NetMode_DrawBarIcon(st, iconX, 1);
}

/* Scales the bar b's width by the percentage, capped at 100. */
void NetBox_ScaleByPercent(NetSprDef *b)
{
    if (gNetMode.percent > 100) {
        gNetMode.percent = 100;
    }
    b->w = (s16)(b->w * gNetMode.percent) / 100.0f;
}

/* The transfer's messages (page 2): part 1 blinking (60 frames lit, 30
 * dark) while gNetMode.netResult is NET_RESULT_RUNNING, lit once it has
 * ended; then parts 2 and 4. */
void NetMode_DrawStatusMsg(void)
{
    NetModeState *st;

    st = &gNetMode;
    if (st->netResult == NET_RESULT_RUNNING) {
        st->blink++;
        if (st->blink >= (st->blinkOn == 0 ? 60 : 30)) {
            st->blink = 0;
            st->blinkOn ^= 1;
        }
    } else if (st->netResult != NET_RESULT_NONE) {
        st->blinkOn = 0;
    }
    if (st->blinkOn == 0) {
        NetModeMsg_Page2(1);
    }
    NetModeMsg_Page2(2);
    NetModeMsg_Page2(4);
}

/* The error's messages (page 2): the part for gNetMode.err (NetMode_ErrorMsg),
 * then, unless err is 127, part 5. The tests for parts 6 and 7 can never
 * pass after the first. */
void NetMode_DrawError(void)
{
    NetModeState *st;
    s32 part;

    st = &gNetMode;
    NetModeMsg_Page2(NetMode_ErrorMsg(st->err));
    if (st->err != 127) {
        if (st->err <= -601) {
            part = 5;
        } else if (st->err <= -701) {
            part = 6;
        } else if (st->err <= -801) {
            part = 7;
        } else {
            part = 5;
        }
        NetModeMsg_Page2(part);
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

/* The transfer's two status sprites d[0] and d[1], faded by the animation
 * step anim0 and gNetMode.netResult (the alpha is the colour's top byte):
 * d[0] fades in over 24 frames once the transfer runs; when it fails, d[0]
 * gives way to d[1] over 8 frames, and d[1] fades out over 24 once the
 * failure is over; when it succeeds, d[0] fades out over 24 frames. */
static void NetMode_FadeStatus(NetSprDef *d)
{
    NetModeState *st;

    st = &gNetMode;
    switch (st->anim0) {
    case 0:
        if (st->netResult == NET_RESULT_RUNNING) {
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
        d[0].color = Rgba(255, 255, 255, Alpha8(255.0f * st->anim0Frame / 24.0f));
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
            d[0].color = Rgba(255, 255, 255, Alpha8(255.0f * (24 - st->anim0Frame) / 24.0f));
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
        d[0].color = Rgba(255, 255, 255, Alpha8(255.0f * (8 - st->anim0Frame) / 8.0f));
        d[1].color = Rgba(255, 255, 255, Alpha8(255.0f * st->anim0Frame / 8.0f));
        break;
    case 6:
        if (st->netResult != NET_RESULT_FAILED) {
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
            d[1].color = Rgba(255, 255, 255, Alpha8(255.0f * (24 - st->anim0Frame) / 24.0f));
        }
        d[0].color = 0xFFFFFF;
        break;
    case 8:
        d[0].color = 0xFFFFFF;
        d[1].color = 0xFFFFFF;
        break;
    }
}

/* An icon of the network screen (gNetModeIcons): a window drawn by
 * Window_DrawTextAlpha. */
typedef struct NetIcon {
    Window win;
    u32 unk10;                  /* 0x10: 0 in every entry */
} NetIcon;


extern NetSprDef *gNetMenuLitRows[];
extern u8 gNetMenuLitRowCounts[];
extern NetSprDef *gNetMenuRows[];
extern u8 gNetMenuRowCounts[];
extern NetIcon gNetModeIcons[];
extern ListWin gNetModeWin;

void NetMode_DrawSprites(NetSprDef *d, s32 n, s32 dy);
void Window_DrawTextAlpha(Window *w, u8 *text, u8 alpha);
struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, s8 *title, s32 sel, u8 alpha);

/* Draws the box `part` (one NET_BOX_* bit). The sprite sets are copied
 * into the load buffer (gLoadBuf) when they are changed before drawing. */
void NetMode_DrawPart(s32 part)
{
    NetSprDef *buf;
    NetModeState *st;
    NetSprDef *rows;
    s32 count;
    s32 rowsY;
    s32 i;
    s16 x;
    s32 y;
    f32 shift;
    NetIcon icon;
    NetIcon secondIcon;
    ListWin win;

    buf = *(NetSprDef * *)&gLoadBuf;
    st = &gNetMode;
    switch (part) {
    case NET_BOX_FRAME:
        NetMode_DrawSprites(NetMode_GetSprDefs(NET_SPR_FRAME), 1, 0);
        break;
    case NET_BOX_PANEL:
        NetMode_DrawSprites(NetMode_GetSprDefs(NET_SPR_PANEL), 2, 0);
        break;
    case NET_BOX_TITLE:
        NetMode_DrawTitle();
        break;
    case NET_BOX_MENU0_ROWS:
        i = st->menuSet;
        rows = NetMode_PickRows(gNetMenuRows, i);
        count = NetMode_PickRowCount(gNetMenuRowCounts, i);
        rowsY = NetMode_MenuRowsY(0);
        goto draw;
    case NET_BOX_MENU0_LIT:
        i = st->menuSet;
        rows = NetMode_PickRows(gNetMenuLitRows, i);
        count = NetMode_PickRowCount(gNetMenuLitRowCounts, i);
        rowsY = NetMode_MenuRowsY(1);
    draw:
        /* the menus' row boxes share this drawing through gotos, laid out
         * as the game's code is */
        if (rowsY != 0) {
            NetMode_CopySprDefs(rows, buf, count);
            NetMode_DrawSprites(buf, count, rowsY);
        }
        break;
    case NET_BOX_MENU0_CURSOR:
        NetMode_CopySprDefs(NetMode_GetSprDefs(NET_SPR_CURSOR), buf, 1);
        NetMode_PlaceCursor(buf, st->menuSet);
        NetMode_ListEntryUV(buf, NetMode_GetSprDefs(NET_SPR_CURSOR));
        /* the test below always passes: the s16 holds -32768 */
        st->cursorY0 = NET_CURSOR_UNSET;
        NetMode_SetCursorY(0);
        if (st->cursorY0 != NET_CURSOR_UNSET) {
            NetMode_DrawSprites(buf, 1, st->cursorY0);
        }
        break;
    case NET_BOX_MENU0_PICKED:
        NetMode_CopySprDefs(NetMode_GetSprDefs(NET_SPR_CURSOR), buf, 1);
        NetMode_PlaceCursor(buf, st->menuSet);
        NetMode_ListEntryUV(buf, NetMode_GetSprDefs(NET_SPR_CURSOR));
        if (NetBox_GrowPicked(buf) == 1) {
            NetMode_DrawSprites(buf, 1, st->cursorY0);
        }
        break;
    case NET_BOX_ICON:
        i = st->icon;
        icon = gNetModeIcons[i];
        y = st->iconY;
        icon.win.y += y;
        Window_DrawTextAlpha(&icon.win, NULL, 128);
        break;
    case NET_BOX_ICON2:
        i = st->icon2;
        secondIcon = gNetModeIcons[i];
        y = st->iconY;
        secondIcon.win.y += y;
        Window_DrawTextAlpha(&secondIcon.win, NULL, 128);
        break;
    case NET_BOX_PROGRESS:
        NetMode_CopySprDefs(NetMode_GetSprDefs(NET_SPR_BAR), buf, 4);
        NetMode_DrawProgress(&buf[3]);
        break;
    case NET_BOX_PROGRESS2:
        NetMode_CopySprDefs(NetMode_GetSprDefs(NET_SPR_BAR), buf, 4);
        NetMode_DrawProgress2(&buf[3]);
        break;
    case NET_BOX_PERCENT:
        NetMode_CopySprDefs(NetMode_GetSprDefs(NET_SPR_BAR), buf, 4);
        NetBox_ScaleByPercent(&buf[3]);
        NetMode_DrawSprites(buf, 4, 0);
        NetModeMsg_QueueField(5);
        break;
    case NET_BOX_STATUS_MSG:
        NetMode_DrawStatusMsg();
        break;
    case NET_BOX_STATUS:
        NetMode_CopySprDefs(NetMode_GetSprDefs(NET_SPR_STATUS), buf, 2);
        NetMode_FadeStatus(buf);
        NetMode_DrawSprites(buf, 2, 0);
        break;
    case NET_BOX_STATUS_NONE:
        break;
    case NET_BOX_ERROR:
        NetMode_DrawError();
        break;
    case NET_BOX_SPARE:
        NetMode_DrawSprites(NetMode_GetSprDefs(NET_SPR_SPARE), 2, 0);
        break;
    case NET_BOX_MENU1_ROWS:
        i = st->menuSet;
        rows = NetMode_PickRows(gNetMenuRows, i);
        count = NetMode_PickRowCount(gNetMenuRowCounts, i);
        rowsY = NetMode_MenuRowsY(0);
        goto draw;
    case NET_BOX_MENU1_LIT:
        i = st->menuSet;
        rows = NetMode_PickRows(gNetMenuLitRows, i);
        count = NetMode_PickRowCount(gNetMenuLitRowCounts, i);
        rowsY = NetMode_MenuRowsY(1);
        goto draw;
    case NET_BOX_MENU1_CURSOR:
        NetMode_CopySprDefs(NetMode_GetSprDefs(NET_SPR_CURSOR), buf, 1);
        NetMode_PlaceCursor(buf, st->menuSet);
        NetMode_ListEntryUV(buf, NetMode_GetSprDefs(NET_SPR_CURSOR));
        st->cursorY1 = NET_CURSOR_UNSET;
        NetMode_SetCursorY(1);
        if (st->cursorY1 != NET_CURSOR_UNSET) {
            NetMode_DrawSprites(buf, 1, st->cursorY1);
        }
        break;
    case NET_BOX_MENU1_PICKED:
        NetMode_CopySprDefs(NetMode_GetSprDefs(NET_SPR_CURSOR), buf, 1);
        NetMode_PlaceCursor(buf, st->menuSet);
        NetMode_ListEntryUV(buf, NetMode_GetSprDefs(NET_SPR_CURSOR));
        if (NetBox_GrowPicked(buf) == 1) {
            NetMode_DrawSprites(buf, 1, st->cursorY1);
        }
        break;
    case NET_BOX_YES_NO_LARGE:
        x = LobbyMsg_CenterX(LobbyMsg_GetLines(0, 14)[2], 20);
        if (st->yesNo == 0) {
            x = x + 20;
        } else {
            shift = 30.0f;
            x = (s16)shift + (x + 100);
        }
        Menu_DrawGlowBar(x, 322, 120, 24, 0xFF20C0C0);
        break;
    case NET_BOX_YES_NO:
        x = LobbyMsg_CenterX(LobbyMsg_GetLines(0, 14)[2], 18);
        if (st->yesNo == 0) {
            x = x + 18;
        } else {
            shift = 27.0f;
            x = (s16)shift + (x + 90);
        }
        Menu_DrawGlowBar(x, 322, 108, 22, 0xFF20C0C0);
        break;
    case NET_BOX_SLOT_CHOICE:
        win = gNetModeWin;
        ListPanel_DrawAlpha(&win, 0, -1, 128);
        y = 280;
        if (st->sel2 == 1) {
            y += 44;
        }
        Menu_DrawGlowBar(320, y - 2, 306, 22, 0xFF20C0C0);
        break;
    }
}

/* Draws `count` sprites of the table d, moved down by dy; a sprite whose
 * texture (gNetMode.tex) isn't loaded is skipped. */
void NetMode_DrawSprites(NetSprDef *d, s32 count, s32 dy)
{
    NetModeState *st;
    s32 i;
    Sprite s;
    s32 tex;

    st = &gNetMode;
    for (i = 0; i < count; i++, d++) {
        tex = st->tex[d->tex];
        if (tex != 0) {
            Tex_Use(1, (u32 *) &tex);
            Gs_SetState(RS_TEXTURE, tex);
            s.x = d->x;
            s.y = d->y;
            s.y += (s16)dy;
            s.w = d->w;
            s.h = d->h;
            s.u0 = d->u0;
            s.v0 = d->v0;
            s.u1 = d->u1;
            s.v1 = d->v1;
            s.u1 -= 1;
            s.v1 -= 1;
            s.color = d->color;
            Sprite_Draw(&s);
        }
    }
}

/* Draws the title gNetMode.title (NET_TITLE_*), when there is one, in big letters. */
void NetMode_DrawTitle(void)
{
    NetModeState *st;

    st = &gNetMode;
    if (st->title <= 0 || st->title >= 8) {
        return;
    }
    NetMode_DrawBigText(gNetModeTitleTexts[st->title], -1, 28, 34);
}

/* Str_Peek and the copy `peek` in NetMode_DrawBigText's loop are kept for
 * the match: they make the compiler load *str again in the loop's body, as
 * the game's code does (a plain `while (*str)` reuses the test's load). */
static inline char Str_Peek(char *s)
{
    return *s;
}

/* Draws str (capitals and spaces) in the capital letters' font
 * (NET_TEX_LETTERS), size x size a letter, at (x, y); x -1 centres it. */
void NetMode_DrawBigText(char *str, s32 x, s32 y, s32 size)
{
    NetModeState *st;
    Sprite s;
    s32 letter;
    s32 width;
    char *peek;

    st = &gNetMode;
    if (str == NULL) {
        return;
    }
    letter = Str_Length(str);
    width = letter * size;
    if (x == -1) {
        x = (640.0f - width) / 2.0f;
    }
    Tex_Use(1, (u32 *) &st->tex[NET_TEX_LETTERS]);
    Gs_SetState(RS_TEXTURE, st->tex[NET_TEX_LETTERS]);
    s.w = s.h = size;
    while (Str_Peek(peek = str)) {
        if (Str_Peek(peek) == ' ') {
            x += size;
            str++;
        } else {
            letter = *str - 'A';
            s.u0 = (letter % 8) * 32;
            s.v0 = (letter / 8) * 32;
            s.u1 = s.u0 + 31;
            s.v1 = s.v0 + 31;
            s.color = 0xFF2691A1;
            s.x = x;
            s.y = y;
            Sprite_Draw(&s);
            x += size;
            str++;
        }
    }
}

extern f32 gWaveTitleSine[12];        /* the wave's 12 steps: a letter is lowered by 4 times its step's value */
extern UV gWaveTitleLetters[20];       /* the 10 letters: each one's texture corner, then its x and y */

/* The texture handle of the network screen's tenth texture: nonzero while
 * it is loaded (the web browser then shows the waving title as its loading
 * mark). */
s32 NetMode_WaveTitleTex(void)
{
    return gNetMode.tex[9];
}

/* Draws the login screen's title: its 10 letters (gWaveTitleLetters: each
 * letter's texture corner, then its x and y) in the capital letters' font,
 * each lowered by 4 * gWaveTitleSine[phase], a black shadow 2 to the right
 * and below. The wave's phase steps every other frame (gNetMode.blinkCount
 * counts the frames), and each letter is a step on from the one before
 * (two after the third). The counter is read as an s8, as the game's code
 * does. */
void LobbyLogin_DrawWaveTitle(s32 unused)
{
    NetModeState *st;
    s32 oldZwrite;
    s32 oldFilter;
    s32 oldBlend;
    Sprite s;
    s32 i;
    s32 phase;
    UV *letter;
    s16 x;
    s16 y;

    st = &gNetMode;
    ((*(s8 *)&gNetMode.blinkCount))++;
    if ((*(s8 *)&gNetMode.blinkCount) & 1) {
        st->wave++;
    }
    if (st->wave > 11) {
        st->wave -= 12;
    }
    oldZwrite = Gs_GetState(RS_ZWRITE);
    oldFilter = Gs_GetState(RS_TEX_FILTER);
    oldBlend = Gs_GetState(RS_BLEND);
    Gs_SetState(RS_ZWRITE, 0);
    Tex_Use(1, (u32 *) &st->tex[NET_TEX_LETTERS]);
    Gs_SetState(RS_TEXTURE, st->tex[NET_TEX_LETTERS]);
    Gs_SetState(RS_TEX_FILTER, 0);
    Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    s.w = 28;
    s.h = 32;
    letter = gWaveTitleLetters;
    phase = st->wave;
    for (i = 0; i < 10; i++) {
        s.u0 = letter->u;
        s.v0 = letter->v;
        s.u1 = s.u0 + 31;
        s.v1 = s.v0 + 31;
        x = (++letter)->u;
        y = letter->v + 4.0f * gWaveTitleSine[phase];
        letter++;
        s.color = 0xFF000000;
        s.x = x + 2;
        s.y = y + 2;
        s.w = 28;
        s.h = 32;
        Sprite_Draw(&s);
        s.color = -1;
        s.x = x;
        s.y = y;
        s.w = 28;
        s.h = 32;
        Sprite_Draw(&s);
        if (i == 2) {
            phase += 2;
        } else {
            phase++;
        }
        if (phase > 11) {
            phase -= 12;
        }
    }
    Gs_SetState(RS_ZWRITE, oldZwrite);
    Gs_SetState(RS_TEX_FILTER, oldFilter);
    Gs_SetState(RS_BLEND, oldBlend);
}

/* The file's .data */
extern char gFullWidthDigits_0[];
UV gNetBarIconRects[4][2] = {
#include "lobby/00819870.inc"
};
ListWin gNetModeWin = {
#include "lobby/00819890.inc"
};
f32 gWaveTitleSine[12] = {
#include "lobby/008198B0.inc"
};
UV gWaveTitleLetters[20] = {
#include "lobby/008198E0.inc"
};
char *gFullWidthDigits[4] = {
    gFullWidthDigits_0, NULL, NULL, NULL,
};
NetSprDef gNetModeSprDefs_Frame[1] = {
#include "lobby/00819940.inc"
};
NetSprDef gNetModeSprDefs_Panel[2] = {
#include "lobby/00819960.inc"
};
NetSprDef gNetModeSprDefs_Cursor[1] = {
#include "lobby/00819990.inc"
};
NetSprDef gNetModeSprDefs_5[8] = {
#include "lobby/008199B0.inc"
};
NetSprDef gNetModeSprDefs_7[1] = {
#include "lobby/00819A70.inc"
};
NetSprDef gNetModeSprDefs_Bar[4] = {
#include "lobby/00819A90.inc"
};
NetSprDef gNetModeSprDefs_9[1] = {
#include "lobby/00819AF0.inc"
};
NetSprDef gNetModeSprDefs_Status[2] = {
#include "lobby/00819B10.inc"
};
NetSprDef gNetModeSprDefs_Spare[2] = {
#include "lobby/00819B40.inc"
};
NetSprDef *gNetModeSprDefs[12] = {
    gNetModeSprDefs_Frame, gNetModeSprDefs_Panel, gNetModeSprDefs_Cursor, NULL,
    NULL, gNetModeSprDefs_5, NULL, gNetModeSprDefs_7,
    gNetModeSprDefs_Bar, gNetModeSprDefs_9, gNetModeSprDefs_Status, gNetModeSprDefs_Spare,
};
NetSprDef gNetMenuLitRows_0[9] = {
#include "lobby/00819BA0.inc"
};
NetSprDef gNetMenuLitRows_1[10] = {
#include "lobby/00819C80.inc"
};
NetSprDef gNetMenuLitRows_2[7] = {
#include "lobby/00819D70.inc"
};
NetSprDef gNetMenuRows_0[9] = {
#include "lobby/00819E20.inc"
};
NetSprDef gNetMenuRows_1[10] = {
#include "lobby/00819F00.inc"
};
NetSprDef gNetMenuRows_2[7] = {
#include "lobby/00819FF0.inc"
};
NetSprDef *gNetMenuLitRows[3] = {
    gNetMenuLitRows_0, gNetMenuLitRows_1, gNetMenuLitRows_2,
};
u8 gNetMenuLitRowCounts[8] = {
#include "lobby/0081A0A8.inc"
};
NetSprDef *gNetMenuRows[4] = {
    gNetMenuRows_0, gNetMenuRows_1, gNetMenuRows_2, 0,
};
u8 gNetMenuRowCounts[8] = {
#include "lobby/0081A0C0.inc"
};
s32 gNetCursorSpans[3] = {
#include "lobby/0081A0C8.inc"
};
NetIcon gNetModeIcons[8] = {
#include "lobby/0081A0E0.inc"
};
NetSprDef gLoginFrameSprites[11] = {
#include "lobby/0081A180.inc"
};
u32 gLoginItemColors[8] = {
#include "lobby/0081A290.inc"
};
u32 gLoginLitColors[8] = {
#include "lobby/0081A2B0.inc"
};
LoginBox gLoginBoxSprites[4] = {
#include "lobby/0081A2D0.inc"
};
u8 gLoginPageColorIdx[16] = {
#include "lobby/0081A2F0.inc"
};
