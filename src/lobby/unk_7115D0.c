#include "common.h"
#include "lobby_text.h"
#include "sprite.h"
#include "lobby_net.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "lobby_menu.h"
#pragma optimization_level 4
int abs(int);

extern char *D_0080C124[];
void Font_SetSpacing(u8 a, u8 b);
void Text_DrawShadowed(s16 x, s32 y, u32 color, s32 a, char *s); /* unit's own declaration: with the definition's void (s16 x, s16 y, s32 shadow, s32 color, char *s) (lobby/unk_790EC0.c), PriceInfo_DrawWanted 14 of 38 words differ */

void NetTask4_Run(s32 task);
void NetTask4_Steps(NetTask *t);
void NetTask4_Done(NetTask *t);
void NetTask4_Failed(NetTask *t);
void NetTask4_Step0(NetTask *t);
void NetTask4_Step1(NetTask *t);
void NetTask4_Step2(NetTask *t);
void NetTask4_Step3(NetTask *t);
void NetTask4_Step4(NetTask *t);
void NetTask4_Step5(NetTask *t);
typedef struct TextBox { u8 pad[0x18]; s32 size; } TextBox;
extern u8 gMenuWork[];
void Text_SetSize(s32 size);
s32 LobbyWorld_GetSeason(void);
s32 Wanted_IsSet(s16 *p);
void PriceInfo_DrawDetails(s16 x, s16 y, void *e, u8 mode, s8 n);
/* the board screen's state (gMenuWork) */
typedef struct BoardMenu {
    /* 0x00 */ u8 names[3][3][8];      /* per kind, 0x18 each */
    /* 0x48 */ u8 names2[3][3][8];
    /* 0x90 */ WinSlide win[2];      /* the list window and the detail window */
    /* 0xA0 */ WinSlide s[4];        /* the four windows */
    /* 0xC0 */ s8 page;
    /* 0xC1 */ s8 step;                /* PriceInfo_SlideWindows */
    /* 0xC2 */ s8 timer;
    /* 0xC3 */ s8 sel;
    /* 0xC4 */ u8 padC4[2];
    /* 0xC6 */ s8 unkC6;
    /* 0xC7 */ s8 unkC7;
    /* 0xC8 */ u8 padC8[2];
    /* 0xCA */ s16 ids[0x4A];          /* the list lines: an item, negative for none */
} BoardMenu;
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 colour, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void PriceInfo_DrawCategories(WinSlide *s, s16 y);
void PriceInfo_DrawList(s16 x, s16 y);
extern u8 gPricePageCounts[];                 /* the pages' line counts (Game_IsOnline == 0) */
extern u8 gPricePageCountsOnline[];                 /* the same, Game_IsOnline != 0 */
extern u8 gOnlinePriceIds[];                 /* page 2's items (Game_IsOnline != 0) */
extern u8 gFixedPriceIds[];                 /* page 2's items */
extern s32 gFixedPrices[];                /* page 2's prices */
extern s32 gRateSnapshot[];                  /* the items' prices */
extern char *gMonsterNames[];                /* the items' names */
extern char *gUnknownNameTexts[];
s32 Game_IsOnline(void);
void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 kind, s32 b);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
s32 abs(s32 x);
extern char *D_0080C11C;                /* the board's title */
typedef struct Colours3 {
    u32 c[3];
} Colours3;
extern Colours3 gPriceLegendColors;             /* the legend's colours */
void LobbySe_Play(s32 se);
s32 Menu_WrapUpDownSe(s32 cur, s32 n, M2C_UNK se);
extern u8 gLobbyRateStates[];                 /* the entries' levels and marks */
void Font_SetColor(s32 color);
extern char *gWantedTexts[];                /* the board's "none" line */
extern char D_00838858[];               /* "%s" */
extern u32 gPriceBoardColors[];                /* the board texts' colours */
void Icon_DrawRotated(s16 x, s16 y, s16 w, s16 h, u32 colour, u16 flags, s32 spr); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 color, s32 angle, s32 i) (lobby/unk_790EC0.c), PriceInfo_DrawRate 59 of 92 words differ */
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 colour, s32 spr, u8 a);
void Font_SetPos(s16, s16);
void Text_PrintfColored(char *s, ...);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);

s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);

/* slides the board's four windows in from x from to to, one after the
 * other; 0 once none moves, 2 while they do */
s32 PriceInfo_SlideWindows(s16 from, s16 to)
{
    BoardMenu *c = (BoardMenu *)gMenuWork;
    s16 i;
    s16 busy = 0;

    for (i = 0; i < 4; i++) {
        if (c->s[i].state) {
            WinSlide_Step(&c->s[i], from, to);
            busy = 1;
        }
    }
    switch (c->step) {
    case 0:
        c->timer = 0;
        WinSlide_Step(&c->s[0], from, to);
        c->s[3].pos = from;
        busy = 1;
        c->s[2].pos = from;
        c->s[1].pos = from;
        c->step++;
        break;
    case 1:
        if (++c->timer > 0) {
            c->step++;
            c->timer = 0;
            WinSlide_Step(&c->s[1], from, to);
            busy = 1;
        }
        break;
    case 2:
        if (++c->timer > 0) {
            c->step++;
            c->timer = 0;
            WinSlide_Step(&c->s[2], from, to);
            busy = 1;
        }
        break;
    case 3:
        if (++c->timer > 0) {
            c->step++;
            c->timer = 0;
            WinSlide_Step(&c->s[3], from, to);
            busy = 1;
        }
        break;
    }
    if (busy == 0) {
        c->step = 0;
        return 0;
    }
    return 2;
}

void Chat_SetWindowMode(s8 a, s8 b);
u16 LobbyPad_Get(s32 a);
extern u8 gMenuInputLock;
s32 PriceInfo_ListInput(u16 pad);
s32 Menu_WrapLeftRightSe(s32 cur, s32 n, M2C_UNK se);
void PriceInfo_Close(void);
s32 PriceInfo_SlideWindows(s16 from, s16 to);

/* the board screen (BoardMenu.page): the windows slide in, the choice of
 * a line (PriceInfo_ListInput), its detail window when both of its names are
 * set, the detail's paging, and the windows slide out */
void PriceInfo_Update(void)
{
    BoardMenu *c = (BoardMenu *)gMenuWork;
    s32 k = LobbyWorld_GetSeason();
    u16 pad = LobbyPad_Get(0);
    s8 i;
    u8 *rec;

    Chat_SetWindowMode(1, 2);
    switch (c->page) {
    case 0:
        switch (PriceInfo_SlideWindows(0x280, 0x118)) {
        case 0:
            c->page++;
            break;
        }
        break;
    case 1:
        switch (PriceInfo_ListInput(pad)) {
        case 0:
            c->page++;
            WinSlide_Step(&c->win[0], 0x280, 0x140);
            c->unkC6 = 0;
            c->win[1].pos = c->win[0].pos;
            gMenuInputLock = 3;
            break;
        case 3:
            PriceInfo_SlideWindows(0x118, 0x280);
            c->page = 5;
            break;
        }
        break;
    case 2:
        switch (WinSlide_Step(&c->win[0], 0x280, 0x140)) {
        case 0:
            i = c->sel - 1;
            c->page = 4;
            rec = c->names[k][i];
            if (Wanted_IsSet((s16 *) (rec + 0x48)) == 1 && Wanted_IsSet((s16 *) rec) == 1) {
                c->page = 3;
                WinSlide_Step(&c->win[1], 0x140, 0x1E);
                return;
            }
            break;
        }
        rec = c->names[k][i];
        if (Wanted_IsSet((s16 *) (rec + 0x48)) == 1 && Wanted_IsSet((s16 *) rec) == 1) {
            if (c->win[0].timer < 0) {
                WinSlide_Step(&c->win[1], 0x140, 0x1E);
            } else {
                c->win[1].pos = c->win[0].pos;
            }
        }
        break;
    case 3:
        switch (WinSlide_Step(&c->win[1], 0x140, 0x1E)) {
        case 0:
            c->page++;
            break;
        }
        break;
    case 4:
        if (pad & 0x40) {
            c->page = 1;
            LobbySe_Play(0x30);
        } else if (c->sel == 0) {
            c->unkC6 = Menu_WrapLeftRightSe(c->unkC6, c->unkC7, 0x2D);
        }
        break;
    case 5:
        switch (PriceInfo_SlideWindows(0x118, 0x280)) {
        case 0:
            c->page++;
            PriceInfo_Close();
            break;
        }
        break;
    }
}

/* the board's line choice: circle takes the chosen line when either of its
 * names is set (2 when none), cross goes back (3), up and down move (2) */
s32 PriceInfo_ListInput(u16 pad)
{
    BoardMenu *c = (BoardMenu *)gMenuWork;
    s32 k;
    u8 *rec;

    k = LobbyWorld_GetSeason();
    if (pad & 0x20) {
        switch (c->sel) {
        case 1:
        case 2:
        case 3:
            rec = c->names[k][(s8)(c->sel - 1)];
            if (Wanted_IsSet((s16 *) rec) == 0 && Wanted_IsSet((s16 *) (rec + 0x48)) == 0) {
                LobbySe_Play(9);
                return 2;
            }
            LobbySe_Play(0x2F);
            break;
        default:
            LobbySe_Play(0x2E);
            break;
        }
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(0x30);
        return 3;
    }
    c->sel = Menu_WrapUpDownSe(c->sel, 4, 0x2D);
    return 2;
}

/* draws the board text k (D_0080C124) right-aligned at x */
void PriceInfo_DrawWanted(s16 x, s32 y, s16 k, s32 color)
{
    char **t;
    s32 px;

    Font_SetSpacing(0x16, 0x16);
    t = &D_0080C124[k];
    px = Text_CenterX(*t, x, 0x16);
    Text_DrawShadowed(px, y, 0xFF000000, color, *t);
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7115D0", D_00838858);
void Tex_Bind(s32 i);

/* the label of board line n at x + 0xAA, y + 0x1D: board text n - 1 in its
 * colour (gPriceBoardColors; grey when neither of its names is set), or the "none"
 * text for line 0; returns the line's text colour. Text_CenterX returns
 * s16: its result, stored in x, is extended into x's register (an int
 * result goes straight into a0). */
u32 PriceInfo_DrawTitle(s16 x, s16 y, s16 n)
{
    BoardMenu *c = (BoardMenu *)gMenuWork;
    s32 k;
    s16 i;
    u8 *rec;

    k = LobbyWorld_GetSeason();
    Font_SetSpacing(0x16, 0x16);
    switch (n) {
    case 0:
        Font_SetColor(0x80101010);
        x = Text_CenterX(gWantedTexts[0], (s16)(x + 0xAA), 0x16);
        Font_SetPos(x, y + 0x1D);
        Text_PrintfColored(D_00838858, gWantedTexts[0]);
        return 0xFFE0D8C8;
    default:
        i = n - 1;
        rec = c->names[k][i];
        if (Wanted_IsSet((s16 *) rec) == 0 && Wanted_IsSet((s16 *) (rec + 0x48)) == 0) {
            PriceInfo_DrawWanted((s16)(x + 0xAA), (s16)(y + 0x1D), i, 0x80404040);
            return 0xFF808080;
        }
        PriceInfo_DrawWanted((s16)(x + 0xAA), (s16)(y + 0x1D), i, gPriceBoardColors[i]);
        return 0xFFE0D8C8;
    }
}

/* the board's four lines (their windows s): the frame and label
 * (PriceInfo_DrawTitle), 1 to 3 icons, and the blinking arrows on the chosen
 * line */
void PriceInfo_DrawCategories(WinSlide *s, s16 y)
{
    BoardMenu *c = (BoardMenu *)gMenuWork;
    s16 i;
    s16 blink;
    s16 ty;
    s16 x;

    for (i = 0; i < 4; i++) {
        x = s->pos;
        Icon_DrawMode(x, y, 0x154, 0x50, PriceInfo_DrawTitle(x, y, i), 0, i);
        ty = y + 0xA;
        switch (i) {
        case 3:
            Icon_Draw(x + 0x8C, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
            break;
        case 2:
            Icon_Draw(x + 0x5A, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
            Icon_Draw(x + 0xBE, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
            break;
        default:
            Icon_Draw(x + 0x28, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
            Icon_Draw(x + 0xF0, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
            Icon_Draw(x + 0x8C, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
            break;
        }
        if (i == c->sel) {
            if (gFrameTick & 0x10) {
                blink = 10;
            } else {
                blink = 0;
            }
            Icon_Draw(x + 0xA + blink, y + 0x18, 0x20, 0x20, 0xFFA0A040, 0x29);
            Icon_Draw(x + 0x12A - blink, y + 0x18, 0x20, 0x20, 0xFFA0A040, 0x28);
        }
        y += 0x5A;
        s++;
    }
}

void PriceInfo_DrawRate(s16 x, s16 y, s16 k)
{
    u8 f;
    s32 lv;
    u8 m;
    u32 colour;
    u16 fl;

    f = gLobbyRateStates[k];
    m = f >> 3;
    lv = f & 7;
    if (lv < 2) {
        colour = 0xFF802000;
    } else if (lv < 4) {
        colour = 0xFF387846;
    } else {
        colour = 0xFF4444A0;
    }
    if (k & 2) {
        fl = 0x8000;
    } else {
        fl = 0;
    }
    Icon_DrawRotated(x, y + 0xC, (lv + 1) * 0x18, 8, colour, fl, 0x6F);
    if (m & 1) {
        Icon_DrawMode(x + 0xE6, y - 1, 0x1E, 0x16, 0xFF4444A0, 0x1D, 2);
    } else if (m & 2) {
        Icon_DrawMode(x + 0xE6, y + 2, 0x1E, 0x16, 0xFF802020, 0x1E, 3);
    }
}

/* the board's legend at x, y: three texts (the pointers at sub_main
 * 0x447BF0) and three colour bars (gPriceLegendColors). */
void PriceInfo_DrawLegend(s16 x, s16 y)
{
    Colours3 cols;
    s32 i;
    s32 ly;

    cols = gPriceLegendColors;
    Font_SetPos(x - 4, y);
    Text_PrintfColored(gPriceLegendTexts[0]);
    ly = y;
    Font_SetPos(x - 4, ly + 0x17);
    Text_PrintfColored(gPriceLegendTexts[1]);
    Text_PrintfColored(gPriceLegendTexts[2]);
    x -= 10;
    for (i = 0; i < 3; i++) {
        Icon_Draw(x, ly + 0x24, 0x3C, 0xA, cols.c[i], 0x6E);
        x += 0x36;
    }
}

/* the board's list window at x, y: page m->unkC6's lines (pages 0-2 have
 * gPricePageCounts/gPricePageCountsOnline's line counts, from 3 on 11 each), each line an
 * item's name and price, or the "none" text. Page 2 lists the fixed items
 * (gFixedPriceIds and their prices gFixedPrices, or gOnlinePriceIds), shown when
 * gSave.hunted's count for them is nonzero; the others BoardMenu.ids.
 * It matches at optimization level 4 with PriceInfo_DrawRate compiled from C
 * (MWCC's level 4 uses the callee's registers; 95 words off while that is
 * assembly); at level 3, 33 words off (the saved registers rotated). */
void PriceInfo_DrawList(s16 x, s16 y)
{
    BoardMenu *m = (BoardMenu *)gMenuWork;
    s16 i;
    s16 cnt;
    s16 start;
    u8 *tbl;
    s16 k;
    s16 n;
    s32 price;

    Menu_DrawQuestSheet(x - 0x14, y, 0x140, 0x168, 0, m->unkC6, -1, -1);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum((s16)(x - 0x14) + 0xFA, y + 2, m->unkC6, m->unkC7, 1);
    Tex_Bind(0x16B);
    Font_SetSpacing(0x14, 0x14);
    i = y + 0x24;
    PriceInfo_DrawLegend((s16)(x + 0x14), i);
    y += 0x42;
    if (m->unkC6 > 2) {
        cnt = 0xB;
        start = m->unkC6 * 0xB;
    } else {
        if (!Game_IsOnline()) {
            tbl = gPricePageCounts;
        } else {
            tbl = gPricePageCountsOnline;
        }
        start = 0;
        cnt = tbl[m->unkC6];
        for (i = 0; i < m->unkC6; i++) {
            start += *tbl++;
        }
    }
    for (i = 0; i < cnt; i++) {
        n = start + i;
        if (n >= 0x4A) {
            return;
        }
        if (m->unkC6 == 2) {
            if (!Game_IsOnline()) {
                price = gFixedPrices[i];
                k = gFixedPriceIds[i];
            } else {
                k = gOnlinePriceIds[i];
                price = gRateSnapshot[k];
            }
            y += 0x17;
            Font_SetPos(x + 0x14, y);
            if (gSave.hunted[k] > 0) {
                Text_PrintfColored("%-16s:%4dz", gMonsterNames[k], price);
            } else {
                Text_PrintfColored(D_00838858, gUnknownNameTexts[0]);
            }
        } else {
            k = m->ids[n];
            price = gRateSnapshot[abs(k)];
            if (k != 0 && price != 0) {
                y += 0x17;
                Font_SetPos(x + 0x14, y);
                if (0 < k) {
                    Text_PrintfColored("%-16s:%4dz", gMonsterNames[k], gRateSnapshot[k]);
                    PriceInfo_DrawRate(x + 0x14, y, k);
                } else {
                    Text_PrintfColored(D_00838858, gUnknownNameTexts[0]);
                }
            }
        }
    }
}

/* a board line (the rows of BoardMenu.names, 8 bytes) */
typedef struct BoardEntry {
    s16 kind;                           /* 3: an item (gItemNames's names) */
    s16 id;
    s16 unk4;
    s16 price;
} BoardEntry;
extern char *gQuestHallTexts[];
extern char *D_0080C138;
extern char *D_0080C13C[];
extern char *gItemNames[];
extern char *gMonsterClassTexts[];
extern char *gRewardLabelTexts[];
s32 sprintf(char *buf, const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
void Text_SetColor(s16 color);
s32 Lobby_HrAtLeast31(void);
void Menu_DrawBarRotated(s16 x, s16 y, s16 w, s16 h, u32 color, s32 angle, s16 cx, s16 cy, u8 k);
void Menu_DrawFrameBox(s16 x, s16 y, s16 w, s16 h, u32 color);
void Tex_Prepare(s32 n, s32 tex);
s32 Tex147_GetCellUv(s32 *uv0, s32 *uv1, u16 id);
s32 Sprite_Draw(Sprite *s);
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
void PriceInfo_DrawDetails(s16 x, s16 y, void *entry, u8 mode0, s8 n)
{
    BoardEntry *e = entry;
    Sprite spr;
    char buf[0x40];
    char wbuf[0x20];
    s16 cx;
    u32 *col;
    s16 px;
    s32 mode = mode0;
    s16 ty;
    char *s;
    BoardMenu *m = (BoardMenu *)gMenuWork;

    if (mode == 1) {
        Menu_DrawBarRotated(x, y, 0x118, 0x13E, 0xFFCFBDBD, 0, 0, 0, e->id & 3);
    } else {
        Menu_DrawBarRotated(x, y, 0x118, 0x13E, 0xFFE0D8C8, 0, 0, 0, e->id & 3);
    }
    ty = y + 0x4A;
    switch (m->sel) {
    case 3:
        Icon_Draw(x + 0x6E, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
        break;
    case 2:
        Icon_Draw(x + 0x3C, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
        Icon_Draw(x + 0xA0, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
        break;
    default:
        Icon_Draw(x + 0x28, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
        Icon_Draw(x + 0xB4, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
        Icon_Draw(x + 0x6E, ty, 0x3C, 0x3C, 0x40000000, 0x1F);
        break;
    }
    cx = x + 0x8C;
    Font_SetSpacing(0x16, 0x16);
    col = &gPriceBoardColors[n];
    Font_SetColor(*col);
    PriceInfo_DrawWanted(cx, y += 0x14, n, *col);
    Icon_Draw(x + 0x14, y + 0xF, 0xF0, 0xC, 0x99106060, e->id % 6 + 0x6E);
    if (Game_IsOnline() == 1) {
        s = gQuestHallTexts[mode];
    } else {
        s = D_0080C138;
    }
    px = Text_CenterX(s, cx, 0x16);
    Font_SetPos(px, y += 0x23);
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x14, 0x14);
    Text_PrintfColored(s);
    Font_SetSpacing(0x14, 0x14);
    if (e->kind == 3) {
        s = gItemNames[e->id];
    } else {
        if (n == 0) {
            sprintf(buf, "%s%s", gMonsterClassTexts[e->id], gMonsterNames[e->id]);
        } else {
            sprintf(buf, D_00838858, gMonsterNames[e->id]);
        }
        s = buf;
    }
    if (Lobby_HrAtLeast31() == 0 && mode == 1) {
        s = gUnknownNameTexts[0];
    }
    px = Text_CenterX(s, cx, 0x14);
    Font_SetPos(px, y += 0x22);
    Text_PrintfColored(s);
    sprintf(buf, D_0080C13C[e->kind], e->unk4);
    Sjis_Widen(buf, wbuf);
    px = Text_CenterX(wbuf, cx, 0x14);
    Font_SetPos(px, y += 0x19);
    if (Lobby_HrAtLeast31() == 0 && mode == 1) {
        sprintf(wbuf, D_00838858, gUnknownNameTexts[0]);
    }
    Text_PrintfColored(wbuf);
    Font_SetSpacing(0x16, 0x16);
    spr.y = y + 0x21;
    spr.w = 0x60;
    spr.h = 0x60;
    spr.color = -1;
    if (e->kind == 3) {
        spr.x = cx - 60.0f;
        Menu_DrawFrameBox(spr.x - 0xA, spr.y - 5, 0x8C, 0x6A, 0xFF493120);
        if (Lobby_HrAtLeast31() == 0 && mode == 1) {
            spr.x = cx - 0x30;
            Menu_DrawFrameBox(spr.x - 0xA, spr.y - 5, 0x74, 0x6A, 0xFF493120);
            Tex_Prepare(1, 0x147);
            Tex_Bind(0x147);
            Tex147_GetCellUv((s32 *)&spr.u0, (s32 *)&spr.u1, 0);
            Sprite_Draw(&spr);
        } else {
            Item_DrawIcon(spr.x, spr.y, spr.w, e->id);
        }
    } else {
        spr.x = cx - 0x30;
        Menu_DrawFrameBox(spr.x - 0xA, spr.y - 5, 0x74, 0x6A, 0xFF493120);
        Tex_Prepare(1, 0x147);
        Tex_Bind(0x147);
        if (Lobby_HrAtLeast31() == 0 && mode == 1) {
            Tex147_GetCellUv((s32 *)&spr.u0, (s32 *)&spr.u1, 0);
        } else {
            Tex147_GetCellUv((s32 *)&spr.u0, (s32 *)&spr.u1, e->id);
        }
        Sprite_Draw(&spr);
    }
    if (Lobby_HrAtLeast31() == 0 && mode == 1) {
        sprintf(buf, "%s%sz", gRewardLabelTexts[0], gUnknownNameTexts[0] + 4);
    } else {
        sprintf(buf, "%s%dz", gRewardLabelTexts[0], e->price);
    }
    Sjis_Widen(buf, wbuf);
    cx = Text_CenterX(wbuf, cx, 0x16);
    Text_SetColor(1);
    y += 0x91;
    Font_SetPos(cx + 3, y + 2);
    Text_PrintfColored(wbuf);
    Font_SetColor(*col);
    Font_SetPos(cx, y);
    Text_PrintfColored(wbuf);
    Icon_Draw(cx, y + 0xF, 0xD2, 0xC, 0x99106060, (e->id + 1) % 6 + 0x6E);
}

/* the board's first window: its title (D_0080C11C) and, on pages 0, 1 and
 * 5, the list (PriceInfo_DrawCategories); on pages 2 to 4 the chosen kind's lines
 * (PriceInfo_DrawList) or the chosen line (PriceInfo_DrawDetails) */
void PriceInfo_Draw(TextBox *t)
{
    BoardMenu *c = (BoardMenu *)gMenuWork;
    s32 k;
    s16 x;

    k = LobbyWorld_GetSeason();
    Text_SetSize(t->size);
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xA0, 0xFFB0B000, D_0080C11C, 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    x = c->win[0].pos;
    switch (c->page) {
    case 0:
    case 1:
    case 5:
        PriceInfo_DrawCategories(c->s, 0x36);
        break;
    case 2:
    case 3:
    case 4:
        if (c->sel == 0) {
            PriceInfo_DrawList(x, 0x3A);
        } else {
            PriceInfo_DrawDetails(x, 0x46, c->names[k][(s16)(c->sel - 1)], 0, c->sel - 1);
        }
        break;
    }
}

/* the board's second window: the chosen line's entry (PriceInfo_DrawDetails, its
 * two names at +0 and +0x48) at x 0x98 (0x90 on page 2) when both are set */
void PriceInfo_Draw2(TextBox *t)
{
    u8 *rec;
    BoardMenu *c = (BoardMenu *)gMenuWork;
    s16 x;
    s32 k;
    s16 n;
    u8 *rec2;

    k = LobbyWorld_GetSeason();
    Text_SetSize(t->size);
    x = c->win[1].pos;
    switch (c->page) {
    case 2:
        x = c->win[0].pos;
    case 3:
    case 4:
        if (c->sel != 0) {
            n = c->sel - 1;
            rec = (u8 *)c + k * 0x18 + n * 8;
            rec2 = rec + 0x48;
            if (Wanted_IsSet((s16 *) rec2) == 1 && Wanted_IsSet((s16 *) rec) == 1) {
                PriceInfo_DrawDetails(x, 0x46, rec2, 1, n);
            }
        }
        break;
    }
}

s32 NetTask4_Start(s32 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *t = &gLobbyNet.task[4];

    if (t->active == 0) {
        memset(t, 0, sizeof(NetTask));
        t->active = 1;
        t->onDone = onDone;
        t->func = NetTask4_Run;
        t->reqFlags = flags & 0x5FF;
        return 0;
    }
    return -1;
}

void NetTask4_Run(s32 task)
{
    NetTask *t = &gLobbyNet.task[4];

    if (t->active != 0) {
        switch (t->state) {
        case 0:
            NetTask4_Steps(t);
            break;
        case 1:
            NetTask4_Done(t);
            break;
        case 2:
            NetTask4_Failed(t);
            break;
        }
    }
}

void NetTask4_Steps(NetTask *t)
{
    switch (t->step) {
    case 0:
        NetTask4_Step0(t);
        return;
    case 1:
        NetTask4_Step1(t);
        return;
    case 2:
        NetTask4_Step2(t);
        return;
    case 3:
        NetTask4_Step3(t);
        return;
    case 4:
        NetTask4_Step4(t);
        return;
    case 5:
        NetTask4_Step5(t);
    default:
        return;
    }
}

void NetTask4_Done(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    t->step = 0;
    m.kind = 0;
    t->onDone(m, &m);
}

void NetTask4_Failed(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    t->step = 0;
    m.kind = -1;
    t->onDone(m, &m);
}

void NetTask4_Step0(NetTask *t)
{
    u32 flags;
    u32 i;

    t->step++;
    t->idx = 1;
    t->pos = 1;
    t->reply = 0;
    flags = t->reqFlags;
    t->reqCount = 0;
    for (i = 0; i < 0x20; i++) {
        if (flags & 1) {
            t->reqCount++;
        }
        flags >>= 1;
    }
    gLobbyNet.nReplies = 0;
    gLobbyNet.count2 = 0;
    memset(gLobbyNet.list2, 0, 0x7BC);
}

/* unk_7115D0's .data (0x7EC968-0x7EC980) */
Colours3 gPriceLegendColors = {
#include "lobby/007EC968.inc"
};
