/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "lobby_menu.h"

s32 Cond_Check(u16);
void CamReq_Cancel(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
s32 Save_TestEventFlag(s32 flag);
void LobbyMenu_Init(void);
extern s8 gMenuInputLock;
extern s8 gLobbyLevel1Sel;
typedef struct ShopList {
    WinSlide win;
    WinSlide win2;
    s32 rows[8];                /* 0x10: the page's entries (indexes in the category) */
    s16 page;                   /* 0x30 */
    s16 pageCount;              /* 0x32 */
    s16 count;                  /* 0x34: rows of the category */
    s16 kind;                   /* 0x36: category (5) */
    s16 cursor;                 /* 0x38 */
    s16 first;                  /* 0x3A: index of the category's first entry */
    s8 state;                   /* 0x3C */
} ShopList;
extern s16 gVillageBoardCounts[];        /* entries per category */
void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32, s32);
s32 Menu_WrapLeftRight(s32, s32);
void VillageBoard_BuildPage(ShopList *m, s16 page);
typedef struct ShopEntry {
    s32 cond;                   /* Cond_Check */
    s32 flag;                   /* event flag, 0: none */
    char *name;                 /* 0x08 */
    char *text;                 /* 0x0C */
} ShopEntry;
extern ShopEntry *gShopStockLists[];
s32 Menu_CountPages(s16 n, s16 per);
extern char *gJumboBoardTexts[];
extern char D_00838078[];
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void Text_PrintfStyled(s32 a, const char *fmt, ...);

/* unk_6E82B0's .rodata (0x82EA90-0x838060) */
#pragma rodata on
char D_0082EA90[0x10] = {
#include "lobby/0082EA90.inc"
};
char D_0082EAA0[0x1D0] = {
#include "lobby/0082EAA0.inc"
};
char D_0082EC70[0x20] = {
#include "lobby/0082EC70.inc"
};
char D_0082EC90[0x178] = {
#include "lobby/0082EC90.inc"
};
char D_0082EE08[13] = {
#include "lobby/0082EE08.inc"
};
char D_0082EE20[0x188] = {
#include "lobby/0082EE20.inc"
};
char D_0082EFA8[13] = {
#include "lobby/0082EFA8.inc"
};
char D_0082EFC0[0x178] = {
#include "lobby/0082EFC0.inc"
};
char D_0082F138[0xC] = {
#include "lobby/0082F138.inc"
};
char D_0082F150[0x1B8] = {
#include "lobby/0082F150.inc"
};
char D_0082F308[13] = {
#include "lobby/0082F308.inc"
};
char D_0082F320[0x160] = {
#include "lobby/0082F320.inc"
};
char D_0082F480[0x20] = {
#include "lobby/0082F480.inc"
};
char D_0082F4A0[0x150] = {
#include "lobby/0082F4A0.inc"
};
char D_0082F5F0[0x20] = {
#include "lobby/0082F5F0.inc"
};
char D_0082F610[0x178] = {
#include "lobby/0082F610.inc"
};
char D_0082F788[13] = {
#include "lobby/0082F788.inc"
};
char D_0082F7A0[0x1A0] = {
#include "lobby/0082F7A0.inc"
};
char D_0082F940[0x180] = {
#include "lobby/0082F940.inc"
};
char D_0082FAC0[0x20] = {
#include "lobby/0082FAC0.inc"
};
char D_0082FAE0[0x160] = {
#include "lobby/0082FAE0.inc"
};
char D_0082FC40[0x20] = {
#include "lobby/0082FC40.inc"
};
char D_0082FC60[0x130] = {
#include "lobby/0082FC60.inc"
};
char D_0082FD90[0x10] = {
#include "lobby/0082FD90.inc"
};
char D_0082FDA0[0x150] = {
#include "lobby/0082FDA0.inc"
};
char D_0082FEF0[0x188] = {
#include "lobby/0082FEF0.inc"
};
char gShopStockLists_0_14[8] = {
#include "lobby/00830078.inc"
};
u32 D_00830080[4] = {
#include "lobby/00830080.inc"
};
char D_00830090[0x140] = {
#include "lobby/00830090.inc"
};
char D_008301D0[0x20] = {
#include "lobby/008301D0.inc"
};
char D_008301F0[0x130] = {
#include "lobby/008301F0.inc"
};
char D_00830320[0x20] = {
#include "lobby/00830320.inc"
};
char D_00830340[0x100] = {
#include "lobby/00830340.inc"
};
char D_00830440[0x20] = {
#include "lobby/00830440.inc"
};
char D_00830460[0x140] = {
#include "lobby/00830460.inc"
};
char D_008305A0[0x10] = {
#include "lobby/008305A0.inc"
};
char D_008305B0[0x180] = {
#include "lobby/008305B0.inc"
};
char D_00830730[0x20] = {
#include "lobby/00830730.inc"
};
char D_00830750[0x160] = {
#include "lobby/00830750.inc"
};
char D_008308B0[0x10] = {
#include "lobby/008308B0.inc"
};
char D_008308C0[0x170] = {
#include "lobby/008308C0.inc"
};
char D_00830A30[0x20] = {
#include "lobby/00830A30.inc"
};
char D_00830A50[0x140] = {
#include "lobby/00830A50.inc"
};
char D_00830B90[0x20] = {
#include "lobby/00830B90.inc"
};
char D_00830BB0[0x140] = {
#include "lobby/00830BB0.inc"
};
char D_00830CF0[0x20] = {
#include "lobby/00830CF0.inc"
};
char D_00830D10[0x190] = {
#include "lobby/00830D10.inc"
};
char D_00830EA0[0x20] = {
#include "lobby/00830EA0.inc"
};
char D_00830EC0[0x180] = {
#include "lobby/00830EC0.inc"
};
char D_00831040[0x20] = {
#include "lobby/00831040.inc"
};
char D_00831060[0xD0] = {
#include "lobby/00831060.inc"
};
char D_00831130[0x20] = {
#include "lobby/00831130.inc"
};
char D_00831150[0x170] = {
#include "lobby/00831150.inc"
};
char D_008312C0[0x20] = {
#include "lobby/008312C0.inc"
};
char D_008312E0[0x170] = {
#include "lobby/008312E0.inc"
};
char D_00831450[0x20] = {
#include "lobby/00831450.inc"
};
char D_00831470[0x128] = {
#include "lobby/00831470.inc"
};
char D_00831598[13] = {
#include "lobby/00831598.inc"
};
char D_008315B0[0xD0] = {
#include "lobby/008315B0.inc"
};
char D_00831680[0x10] = {
#include "lobby/00831680.inc"
};
char D_00831690[0x190] = {
#include "lobby/00831690.inc"
};
char D_00831820[0x20] = {
#include "lobby/00831820.inc"
};
char D_00831840[0x1B0] = {
#include "lobby/00831840.inc"
};
char D_008319F0[0x190] = {
#include "lobby/008319F0.inc"
};
char D_00831B80[0x10] = {
#include "lobby/00831B80.inc"
};
char D_00831B90[0x1A0] = {
#include "lobby/00831B90.inc"
};
char D_00831D30[0x190] = {
#include "lobby/00831D30.inc"
};
char D_00831EC0[0x20] = {
#include "lobby/00831EC0.inc"
};
char D_00831EE0[0x150] = {
#include "lobby/00831EE0.inc"
};
char D_00832030[0x20] = {
#include "lobby/00832030.inc"
};
char D_00832050[0x188] = {
#include "lobby/00832050.inc"
};
char D_008321D8[13] = {
#include "lobby/008321D8.inc"
};
char D_008321F0[0x180] = {
#include "lobby/008321F0.inc"
};
char D_00832370[0x10] = {
#include "lobby/00832370.inc"
};
char D_00832380[0x1A0] = {
#include "lobby/00832380.inc"
};
char D_00832520[0x10] = {
#include "lobby/00832520.inc"
};
char D_00832530[0x180] = {
#include "lobby/00832530.inc"
};
char D_008326B0[0x20] = {
#include "lobby/008326B0.inc"
};
char D_008326D0[0x1B0] = {
#include "lobby/008326D0.inc"
};
char D_00832880[0x20] = {
#include "lobby/00832880.inc"
};
char D_008328A0[0x190] = {
#include "lobby/008328A0.inc"
};
char D_00832A30[0x20] = {
#include "lobby/00832A30.inc"
};
char D_00832A50[0x1A0] = {
#include "lobby/00832A50.inc"
};
char D_00832BF0[0x20] = {
#include "lobby/00832BF0.inc"
};
char D_00832C10[0x180] = {
#include "lobby/00832C10.inc"
};
char D_00832D90[0x20] = {
#include "lobby/00832D90.inc"
};
char D_00832DB0[0x1A0] = {
#include "lobby/00832DB0.inc"
};
char D_00832F50[0x10] = {
#include "lobby/00832F50.inc"
};
char D_00832F60[0x160] = {
#include "lobby/00832F60.inc"
};
char D_008330C0[0x20] = {
#include "lobby/008330C0.inc"
};
char D_008330E0[0x1B0] = {
#include "lobby/008330E0.inc"
};
char D_00833290[0x20] = {
#include "lobby/00833290.inc"
};
char D_008332B0[0x1B0] = {
#include "lobby/008332B0.inc"
};
char D_00833460[0x20] = {
#include "lobby/00833460.inc"
};
char D_00833480[0x1A0] = {
#include "lobby/00833480.inc"
};
char D_00833620[0x20] = {
#include "lobby/00833620.inc"
};
char D_00833640[0x190] = {
#include "lobby/00833640.inc"
};
char D_008337D0[0x10] = {
#include "lobby/008337D0.inc"
};
char D_008337E0[0x1C0] = {
#include "lobby/008337E0.inc"
};
char D_008339A0[0x20] = {
#include "lobby/008339A0.inc"
};
char D_008339C0[0x1A8] = {
#include "lobby/008339C0.inc"
};
char D_00833B68[0xC] = {
#include "lobby/00833B68.inc"
};
char D_00833B80[0x150] = {
#include "lobby/00833B80.inc"
};
char D_00833CD0[0x20] = {
#include "lobby/00833CD0.inc"
};
char D_00833CF0[0x1B0] = {
#include "lobby/00833CF0.inc"
};
char D_00833EA0[0x1D0] = {
#include "lobby/00833EA0.inc"
};
char D_00834070[0x20] = {
#include "lobby/00834070.inc"
};
char D_00834090[0x160] = {
#include "lobby/00834090.inc"
};
char D_008341F0[0x20] = {
#include "lobby/008341F0.inc"
};
char D_00834210[0x1B0] = {
#include "lobby/00834210.inc"
};
char D_008343C0[0x20] = {
#include "lobby/008343C0.inc"
};
char D_008343E0[0x1B0] = {
#include "lobby/008343E0.inc"
};
char D_00834590[0x20] = {
#include "lobby/00834590.inc"
};
char D_008345B0[0x170] = {
#include "lobby/008345B0.inc"
};
char D_00834720[0x20] = {
#include "lobby/00834720.inc"
};
char D_00834740[0x1B0] = {
#include "lobby/00834740.inc"
};
char D_008348F0[0x20] = {
#include "lobby/008348F0.inc"
};
char D_00834910[0x1A0] = {
#include "lobby/00834910.inc"
};
char D_00834AB0[0x20] = {
#include "lobby/00834AB0.inc"
};
char D_00834AD0[0x1A0] = {
#include "lobby/00834AD0.inc"
};
char D_00834C70[0x10] = {
#include "lobby/00834C70.inc"
};
char D_00834C80[0x160] = {
#include "lobby/00834C80.inc"
};
char D_00834DE0[0x20] = {
#include "lobby/00834DE0.inc"
};
char D_00834E00[0x180] = {
#include "lobby/00834E00.inc"
};
char D_00834F80[0x20] = {
#include "lobby/00834F80.inc"
};
char D_00834FA0[0x1A0] = {
#include "lobby/00834FA0.inc"
};
char D_00835140[0x20] = {
#include "lobby/00835140.inc"
};
char D_00835160[0x1B0] = {
#include "lobby/00835160.inc"
};
char D_00835310[0x20] = {
#include "lobby/00835310.inc"
};
char D_00835330[0x1B8] = {
#include "lobby/00835330.inc"
};
char D_008354E8[0xC] = {
#include "lobby/008354E8.inc"
};
char D_00835500[0x180] = {
#include "lobby/00835500.inc"
};
char D_00835680[0x20] = {
#include "lobby/00835680.inc"
};
char D_008356A0[0x1A0] = {
#include "lobby/008356A0.inc"
};
char D_00835840[0x20] = {
#include "lobby/00835840.inc"
};
char D_00835860[0x1D0] = {
#include "lobby/00835860.inc"
};
char D_00835A30[0x20] = {
#include "lobby/00835A30.inc"
};
char D_00835A50[0x1B0] = {
#include "lobby/00835A50.inc"
};
char D_00835C00[0x20] = {
#include "lobby/00835C00.inc"
};
char D_00835C20[0x1D0] = {
#include "lobby/00835C20.inc"
};
char D_00835DF0[0x20] = {
#include "lobby/00835DF0.inc"
};
char D_00835E10[0x1B0] = {
#include "lobby/00835E10.inc"
};
char D_00835FC0[0x20] = {
#include "lobby/00835FC0.inc"
};
char D_00835FE0[0x190] = {
#include "lobby/00835FE0.inc"
};
char D_00836170[0x20] = {
#include "lobby/00836170.inc"
};
char D_00836190[0x1B0] = {
#include "lobby/00836190.inc"
};
char D_00836340[0x20] = {
#include "lobby/00836340.inc"
};
char D_00836360[0x190] = {
#include "lobby/00836360.inc"
};
char D_008364F0[0x20] = {
#include "lobby/008364F0.inc"
};
char D_00836510[0x1A0] = {
#include "lobby/00836510.inc"
};
char D_008366B0[0x20] = {
#include "lobby/008366B0.inc"
};
char D_008366D0[0x190] = {
#include "lobby/008366D0.inc"
};
char D_00836860[0x20] = {
#include "lobby/00836860.inc"
};
char D_00836880[0x1C0] = {
#include "lobby/00836880.inc"
};
char D_00836A40[0x10] = {
#include "lobby/00836A40.inc"
};
char D_00836A50[0x190] = {
#include "lobby/00836A50.inc"
};
char D_00836BE0[0x20] = {
#include "lobby/00836BE0.inc"
};
char D_00836C00[0x190] = {
#include "lobby/00836C00.inc"
};
char D_00836D90[0x20] = {
#include "lobby/00836D90.inc"
};
char D_00836DB0[0x178] = {
#include "lobby/00836DB0.inc"
};
char D_00836F28[13] = {
#include "lobby/00836F28.inc"
};
char D_00836F40[0x180] = {
#include "lobby/00836F40.inc"
};
char D_008370C0[0x180] = {
#include "lobby/008370C0.inc"
};
char D_00837240[0x10] = {
#include "lobby/00837240.inc"
};
char D_00837250[0x148] = {
#include "lobby/00837250.inc"
};
char D_00837398[0xC] = {
#include "lobby/00837398.inc"
};
char D_008373B0[0x158] = {
#include "lobby/008373B0.inc"
};
char D_00837508[0xC] = {
#include "lobby/00837508.inc"
};
char D_00837520[0x180] = {
#include "lobby/00837520.inc"
};
char D_008376A0[0x10] = {
#include "lobby/008376A0.inc"
};
char D_008376B0[0x1B0] = {
#include "lobby/008376B0.inc"
};
char D_00837860[0x20] = {
#include "lobby/00837860.inc"
};
char D_00837880[0x170] = {
#include "lobby/00837880.inc"
};
char D_008379F0[0x10] = {
#include "lobby/008379F0.inc"
};
char D_00837A00[0x1A0] = {
#include "lobby/00837A00.inc"
};
char D_00837BA0[0x10] = {
#include "lobby/00837BA0.inc"
};
char D_00837BB0[0x190] = {
#include "lobby/00837BB0.inc"
};
char D_00837D40[0x10] = {
#include "lobby/00837D40.inc"
};
char D_00837D50[0x190] = {
#include "lobby/00837D50.inc"
};
char D_00837EE0[0x10] = {
#include "lobby/00837EE0.inc"
};
char D_00837EF0[0x170] = {
#include "lobby/00837EF0.inc"
};
#pragma rodata off

void VillageBoard_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamReq_Cancel();
}

extern u8 gMenuWork;
u16 LobbyPad_Get(s32);
s32 WinSlide_Step(WinSlide *p, s16 from, s16 to);
s32 VillageBoard_TopInput(ShopList *m, u16 pad);
s32 VillageBoard_ListInput(ShopList *m, u16 pad);
s32 VillageBoard_ArticleInput(void *m, u16 pad);
void VillageBoard_MarkRead(s16 n);

void VillageBoard_Update(void)
{
    ShopList *m = (ShopList *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);

    switch (m->state) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x11C)) {
        case 0:
            m->state++;
        }
        break;
    case 1:
        switch (VillageBoard_TopInput(m, pad)) {
        case 0:
            m->state++;
            m->cursor = 0;
            VillageBoard_BuildPage(m, 0);
            break;
        case 3:
            m->state = 5;
            WinSlide_Step(&m->win, 0x11C, 0x280);
            break;
        }
        break;
    case 2:
        switch (VillageBoard_ListInput(m, pad)) {
        case 0:
            m->state++;
            VillageBoard_MarkRead(m->first + m->rows[m->cursor]);
            WinSlide_Step(&m->win2, -0x140, 0x50);
            LobbySe_Play(0x2E);
            break;
        case 3:
            m->state = 1;
            break;
        }
        break;
    case 3:
        switch (WinSlide_Step(&m->win2, -0x140, 0x50)) {
        case 0:
            m->state++;
        }
        break;
    case 4:
        switch (VillageBoard_ArticleInput(m, pad)) {
        case 0:
        case 3:
            m->state = 2;
            break;
        }
        break;
    case 5:
        switch (WinSlide_Step(&m->win, 0x11C, 0x280)) {
        case 0:
            VillageBoard_Close();
        }
        break;
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6E82B0", D_00838078);

s32 VillageBoard_ArticleInput(void *m, u16 pad)
{
    if (pad & 0x40) {
        LobbySe_Play(0x30);
        return 3;
    }
    return 2;
}

s32 VillageBoard_ListInput(ShopList *m, u16 pad)
{
    if (pad & 0x20) {
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(0x30);
        return 3;
    }
    if (pad & 0xC00) {
        if (m->pageCount > 1) {
            m->page = Menu_WrapLeftRight(m->page, m->pageCount);
            VillageBoard_BuildPage(m, m->page);
        }
    } else {
        m->cursor = Menu_WrapUpDown(m->cursor, m->count);
    }
    return 2;
}

s32 VillageBoard_CondCheck(s32 a0)
{
    return Cond_Check((u16)a0);
}

s32 VillageBoard_TestFlag(s32 flag) {
    if (flag != 0) {
        return Save_TestEventFlag(flag);
    }
    return 1;
}

void VillageBoard_BuildPage(ShopList *m, s16 page)
{
    ShopEntry *e;
    s16 i = 0;
    s16 n = 0;
    s16 first;
    s16 last;
    s16 count;

    m->count = 0;
    first = page * 8;
    last = first + 7;
    e = gShopStockLists[m->kind];
    count = gVillageBoardCounts[m->kind];
    for (; i < count; i++, e++) {
        if (VillageBoard_CondCheck(e->cond) == 1 && VillageBoard_TestFlag(e->flag) == 1) {
            if (n >= first && n <= last) {
                m->rows[m->count] = i;
                m->count++;
            }
            n++;
        }
    }
    m->page = page;
    m->pageCount = Menu_CountPages(n, 8);
    if (m->cursor >= m->count) {
        m->cursor = m->count - 1;
    }
}

s32 VillageBoard_TopInput(ShopList *m, u16 pad)
{
    s16 i;

    if (pad & 0x20) {
        m->first = 0;
        for (i = 0; i < m->kind; i++) {
            m->first += gVillageBoardCounts[i];
        }
        LobbySe_Play(0x2E);
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(0x30);
        return 3;
    }
    m->kind = Menu_WrapUpDown(m->kind, 5);
    return 2;
}


extern char *gJumboBoardTitleTexts[];
extern char *D_0080C2A4;
extern char *gBoardPromptTexts[];
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *);

void VillageBoard_DrawTop(ShopList *m, s16 x, s16 y)
{
    s32 i;
    s16 ty = y;
    s16 bx;
    s32 lx;
    s16 tx;
    char *s;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x11C, 0x12, 0xC8, 0xFFB0B000, gJumboBoardTitleTexts[0], 2, 1);
    Icon_Draw(x, y, 0x14E, 0x11A, 0xFFE0D8C8, 0);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    lx = x;
    Font_SetPos(lx + 0x28, ty += 0x16);
    Text_PrintfColored(D_0080C2A4);
    Menu_DrawDashLine(lx + 0x14, ty + 0x2C, 0x126, 0xFF000000);
    ty += 0x3E;
    bx = lx + 0xA7;
    for (i = 0; i < 5; i++) {
        s = gJumboBoardTexts[i];
        tx = Text_CenterX(s, bx, 0x12);
        if (m->kind == i) {
            Menu_DrawArrowsRev(tx, ty, 0x12, s, 0xFF008080);
        }
        Font_SetPos(tx, ty);
        Text_PrintfColored(s);
        ty += 0x16;
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gBoardPromptTexts[0]);
}

extern char *D_0080C2AC;
extern char *D_0080C14C;
extern char *gWideSpaceTexts[];
extern char *D_0080C2C4[];
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
s32 sprintf(char *buf, char *fmt, ...);
s32 VillageBoard_IsRead(u16 n);

void VillageBoard_DrawArticles(ShopList *m, s16 x, s16 y)
{
    s32 lx;
    s32 i;
    s16 ty = y;
    char *s;
    char buf[128];
    ShopEntry *e;
    s16 bx;
    s16 tx;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x11C, 0x12, 0xC8, 0xFFB0B000, gJumboBoardTitleTexts[0], 2, 1);
    Icon_Draw(x, y, 0x14E, 0x11A, 0xFFE0D8C8, 1);
    lx = x;
    bx = lx + 0xA7;
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(lx + 0xFE, ty + 4, m->page, m->pageCount, 1);
    sprintf(buf, D_0080C14C, gJumboBoardTexts[m->kind]);
    Font_SetPos(Text_CenterX(buf, bx, 0x12), ty += 0x18);
    Text_PrintfColored(D_00838078, buf);
    s = D_0080C2C4[m->kind];
    Font_SetPos(Text_CenterX(s, bx, 0x12), ty += 0x16);
    Text_PrintfColored(D_00838078, s);
    Menu_DrawDashLine(lx + 0x14, ty + 0x16, 0x126, 0xFF000000);
    e = gShopStockLists[m->kind];
    ty += 0x28;
    for (i = 0; i < m->count; i++) {
        sprintf(buf, "%s%s%s", gWideSpaceTexts[0], e[m->rows[i]].name, gWideSpaceTexts[0]);
        s = buf;
        tx = Text_CenterX(buf, bx, 0x12);
        Font_SetPos(tx, ty);
        Text_PrintfColored(buf);
        if (VillageBoard_IsRead(m->first + m->rows[i]) == 0) {
            Icon_Draw(tx, ty, 0x10, 0x14, 0xFFC03000, 0x63);
        }
        if (m->cursor == i) {
            Menu_DrawArrowsRev(tx, ty, 0x12, s, 0xFF008080);
        }
        ty += 0x16;
    }
    Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, D_0080C2AC);
}

void VillageBoard_DrawArticle(ShopList *m, s16 x, s16 y)
{
    ShopEntry *e;
    char *name;
    s16 py = y;
    s16 cx;
    s32 px = x;

    cx = px + 0x118;
    if (m->state == 4) {
        Font_SetSpacing(0x16, 0x16);
        Menu_DrawLabelBar(0x17C, 0x30, 0xF0, 0xFFB0B000, gJumboBoardTexts[m->kind], 2, 1);
    }
    Icon_Draw(x, y, 0x230, 0x140, 0xFFE0D8C8, 2);
    e = gShopStockLists[m->kind];
    e += m->rows[m->cursor];
    Font_SetColor(0x80101010);
    name = e->name;
    Font_SetSpacing(0x16, 0x16);
    Font_SetPos(Text_CenterX(name, cx, 0x16), py += 0x1E);
    Text_PrintfColored(D_00838078, name);
    Menu_DrawDashLine(px + 0x32, py + 0x1A, 0x1CC, 0xFF000000);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(px + 0x1E, py + 0x28);
    Text_PrintfStyled(4, D_00838078, e->text);
}

extern u8 gMenuWork;
void Text_SetSize(s32);
void VillageBoard_DrawTop(ShopList *m, s16 a, s16 b);
void VillageBoard_DrawArticles(ShopList *m, s16 a, s16 b);
void VillageBoard_DrawArticle(ShopList *m, s16 x, s16 y);
void VillageBoard_Draw(u8 *o)
{
    ShopList *w = (ShopList *)&gMenuWork;

    Text_SetSize(*(s32 *)(o + 0x18));
    switch (w->state) {
    case 0:
    case 1:
    case 5:
        VillageBoard_DrawTop(w, w->win.pos, 0x38);
        break;
    case 2:
        VillageBoard_DrawArticles(w, w->win.pos, 0x38);
        break;
    case 3:
    case 4:
        VillageBoard_DrawArticle(w, 0x28, w->win2.pos);
        break;
    }
}

/* 11 records after the 0x20 bytes at +0x6EF4 of the lobby work buffer gLobbyWorkBuf */
typedef struct Slot6EF4 {
    u8 unk0[8];
    s8 used;
    u8 unk9[0x17];
} Slot6EF4;
void *memset(void *dst, s32 c, u32 n);
void LobbyText_Open(s32);
void LobbyDialog_Grow(void);
void Menu_SetYesNo(s8 arg0);
void GroupBoard_Draw(u8 *);
void GroupBoard_DrawDetails(u8 *);
void GroupBoard_Open(void)
{
    u8 *w = (u8 *)&gMenuWork;
    s16 i;

    gTownMenuOpen = 1;
    gTownMode = 0x3E;
    memset(w, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(GroupBoard_Draw, GroupBoard_DrawDetails, 0, 0);
    LobbyText_Open(0x8F);
    LobbyDialog_Grow();
    Menu_SetYesNo(0);
    *(s16 *)(w + 0xD36) = 0;
    for (i = 0; i < 11; i++) {
        if (((Slot6EF4 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20))[i].used != 0) {
            (*(s16 *)(w + 0xD36))++;
        }
    }
    LobbySe_Play(8);
}

typedef struct RoomQuest {
    u8 used;
    u8 kind;                    /* what unlocks it */
    u8 arg1;
    u8 arg2;
    u8 unk4[0x14];
} RoomQuest;
typedef struct Room {
    Slot6EF4 slots[16];
    u8 unk200[0xBD0 - 0x200];
    RoomQuest quests[64];       /* 0xBD0 */
} Room;
#define LOBBY_ROOM ((Room *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20))
s32 ElderRaid_InState1(void);
s32 GroupBoard_QuestAllowed(s16 i)
{
    Room *room;

    if ((room = LOBBY_ROOM)->quests[i].used == 0) {
        return 0;
    }
    switch (room->quests[i].kind) {
    case 0:
        break;
    case 1:
        if (gSave.hr < room->quests[i].arg1) {
            return 0;
        }
        break;
    case 2:
        if (gSave.hunted[room->quests[i].arg1] == 0) {
            return 0;
        }
        if (gSave.hr < room->quests[i].arg2) {
            return 0;
        }
        break;
    case 3:
        if (!Save_TestEventFlag(0xC4)) {
            return 0;
        }
        break;
    case 4:
        if (!ElderRaid_InState1()) {
            return 0;
        }
        break;
    case 5:
        if (!Save_TestEventFlag(0x122)) {
            return 0;
        }
        break;
    case 6:
        if (!Save_TestEventFlag(0x122)) {
            return 0;
        }
        if (gSave.hunted[LOBBY_ROOM->quests[i].arg1] == 0) {
            return 0;
        }
        break;
    }
    return 1;
}

void GroupBoard_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    gMenuInputLock = 3;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    gLobbyLevel1Sel = 0;
}

/* unk_6E82B0's .data (0x7EB2C0-0x7EB8C0) */
extern char D_0082EA90[];
extern char D_0082EAA0[];
extern char D_0082EC70[];
extern char D_0082EC90[];
extern char D_0082EE08[];
extern char D_0082EE20[];
extern char D_0082EFA8[];
extern char D_0082EFC0[];
extern char D_0082F138[];
extern char D_0082F150[];
extern char D_0082F308[];
extern char D_0082F320[];
extern char D_0082F480[];
extern char D_0082F4A0[];
extern char D_0082F5F0[];
extern char D_0082F610[];
extern char D_0082F788[];
extern char D_0082F7A0[];
extern u32 D_0082F928[];
extern char D_0082F940[];
extern char D_0082FAC0[];
extern char D_0082FAE0[];
extern char D_0082FC40[];
extern char D_0082FC60[];
extern char D_0082FD90[];
extern char D_0082FDA0[];
extern u32 D_0082FED8[];
extern char D_0082FEF0[];
extern char gShopStockLists_0_14[];
extern char D_00830090[];
extern char D_008301D0[];
extern char D_008301F0[];
extern char D_00830320[];
extern char D_00830340[];
extern char D_00830440[];
extern char D_00830460[];
extern char D_008305A0[];
extern char D_008305B0[];
extern char D_00830730[];
extern char D_00830750[];
extern char D_008308B0[];
extern char D_008308C0[];
extern char D_00830A30[];
extern char D_00830A50[];
extern char D_00830B90[];
extern char D_00830BB0[];
extern char D_00830CF0[];
extern char D_00830D10[];
extern char D_00830EA0[];
extern char D_00830EC0[];
extern char D_00831040[];
extern char D_00831060[];
extern char D_00831130[];
extern char D_00831150[];
extern char D_008312C0[];
extern char D_008312E0[];
extern char D_00831450[];
extern char D_00831470[];
extern char D_00831598[];
extern char D_008315B0[];
extern char D_00831680[];
extern char D_00831690[];
extern char D_00831820[];
extern char D_00831840[];
extern u32 D_008319D8[];
extern char D_008319F0[];
extern char D_00831B80[];
extern char D_00831B90[];
extern u32 D_00831D18[];
extern char D_00831D30[];
extern char D_00831EC0[];
extern char D_00831EE0[];
extern char D_00832030[];
extern char D_00832050[];
extern char D_008321D8[];
extern char D_008321F0[];
extern char D_00832370[];
extern char D_00832380[];
extern char D_00832520[];
extern char D_00832530[];
extern char D_008326B0[];
extern char D_008326D0[];
extern char D_00832880[];
extern char D_008328A0[];
extern char D_00832A30[];
extern char D_00832A50[];
extern char D_00832BF0[];
extern char D_00832C10[];
extern char D_00832D90[];
extern char D_00832DB0[];
extern char D_00832F50[];
extern char D_00832F60[];
extern char D_008330C0[];
extern char D_008330E0[];
extern char D_00833290[];
extern char D_008332B0[];
extern char D_00833460[];
extern char D_00833480[];
extern char D_00833620[];
extern char D_00833640[];
extern char D_008337D0[];
extern char D_008337E0[];
extern char D_008339A0[];
extern char D_008339C0[];
extern char D_00833B68[];
extern char D_00833B80[];
extern char D_00833CD0[];
extern char D_00833CF0[];
extern u32 D_00833E88[];
extern char D_00833EA0[];
extern char D_00834070[];
extern char D_00834090[];
extern char D_008341F0[];
extern char D_00834210[];
extern char D_008343C0[];
extern char D_008343E0[];
extern char D_00834590[];
extern char D_008345B0[];
extern char D_00834720[];
extern char D_00834740[];
extern char D_008348F0[];
extern char D_00834910[];
extern char D_00834AB0[];
extern char D_00834AD0[];
extern char D_00834C70[];
extern char D_00834C80[];
extern char D_00834DE0[];
extern char D_00834E00[];
extern char D_00834F80[];
extern char D_00834FA0[];
extern char D_00835140[];
extern char D_00835160[];
extern char D_00835310[];
extern char D_00835330[];
extern char D_008354E8[];
extern char D_00835500[];
extern char D_00835680[];
extern char D_008356A0[];
extern char D_00835840[];
extern char D_00835860[];
extern char D_00835A30[];
extern char D_00835A50[];
extern char D_00835C00[];
extern char D_00835C20[];
extern char D_00835DF0[];
extern char D_00835E10[];
extern char D_00835FC0[];
extern char D_00835FE0[];
extern char D_00836170[];
extern char D_00836190[];
extern char D_00836340[];
extern char D_00836360[];
extern char D_008364F0[];
extern char D_00836510[];
extern char D_008366B0[];
extern char D_008366D0[];
extern char D_00836860[];
extern char D_00836880[];
extern char D_00836A40[];
extern char D_00836A50[];
extern char D_00836BE0[];
extern char D_00836C00[];
extern char D_00836D90[];
extern char D_00836DB0[];
extern char D_00836F28[];
extern char D_00836F40[];
extern u32 D_008370A8[];
extern char D_008370C0[];
extern char D_00837240[];
extern char D_00837250[];
extern char D_00837398[];
extern char D_008373B0[];
extern char D_00837508[];
extern char D_00837520[];
extern char D_008376A0[];
extern char D_008376B0[];
extern char D_00837860[];
extern char D_00837880[];
extern char D_008379F0[];
extern char D_00837A00[];
extern char D_00837BA0[];
extern char D_00837BB0[];
extern char D_00837D40[];
extern char D_00837D50[];
extern char D_00837EE0[];
extern char D_00837EF0[];
ShopEntry gShopStockLists_0[19] = {
#include "lobby/007EB2C0.inc"
};
ShopEntry gShopStockLists_4[11] = {
#include "lobby/007EB3F0.inc"
};
ShopEntry gShopStockLists_3[11] = {
#include "lobby/007EB4A0.inc"
};
ShopEntry gShopStockLists_1[27] = {
#include "lobby/007EB550.inc"
};
ShopEntry gShopStockLists_2[25] = {
#include "lobby/007EB700.inc"
};
ShopEntry *gShopStockLists[6] = {
    gShopStockLists_0, gShopStockLists_1, gShopStockLists_2, gShopStockLists_3,
    gShopStockLists_4, NULL,
};
s16 gVillageBoardCounts[6] = {
#include "lobby/007EB8A8.inc"
};
