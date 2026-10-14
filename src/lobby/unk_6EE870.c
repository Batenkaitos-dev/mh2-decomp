#include "common.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#pragma optimization_level 4

/* the 6709 search's users (LobbyNet_Get6709) */
extern NetFound6709 *gFound6709;

/* A menu that shows 10 entries a page */
typedef struct PageMenu2 {
    WinSlide win;
    u8 unk8[0x110];
    NetUser *rows[10];      /* 0x118: the page's entries */
    s16 cursor;                 /* 0x140 */
    s16 page;                   /* 0x142 */
    s16 pageCount;              /* 0x144 */
    s16 count;                  /* 0x146 */
    s8 unk148;
    s8 state;                   /* 0x149 */
} PageMenu2;

void AreaSearch_BuildPage(PageMenu2 *m, s16 page);
s32 WinSlide_Step(WinSlide *, s16 from, s16 to);
extern u8 gLobbyLevel0Sel;
extern M2C_UNK D_5C81EA;
void Text_SetSize(s32);

extern u8 gMenuWork;
extern LobbyMenu *gLobbyMenu;
extern u8 gLobbyLevel1Sel;
extern u16 gAreaSearchTimer;
extern u16 gAreaSearchFrames;
void LobbyText_Open(s32);
void AreaSearch_ShowResults(PageMenu2 *arg0);
s32 AreaSearch_ResultsInput(PageMenu2 *m, u16 pad);
void AreaSearch_Close(void);
s32 LobbyUser_Search(NetCond6709 *cond, s8 n);
s32 HunterInfo_Update(u16 pad);
void HunterInfo_Open(void *, s32);

s32 AreaSearch_Update(u16 pad)
{
    PageMenu2 *m = (PageMenu2 *)&gMenuWork;

    switch (m->unk148) {
    case 0:
        if (gAreaSearchTimer == 0) {
            m->unk148++;
            m->unk8[0] = 1;
            LobbyText_Open(0x20);
            gLobbyMenu->redraw = 1;
        } else {
            m->unk148 = 2;
            AreaSearch_ShowResults(m);
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (LobbyUser_Search((NetCond6709 *) &m->unk8[0], 0)) {
        case 0:
            m->unk148++;
            gAreaSearchTimer = gAreaSearchFrames;
            AreaSearch_ShowResults(m);
            break;
        case 1:
            m->unk148 = 4;
            break;
        }
        break;
    case 2:
        switch (AreaSearch_ResultsInput(m, pad)) {
        case 0:
            m->unk148 = 3;
            if (gLobbyLevel1Sel != 0) {
                HunterInfo_Open(m->rows[m->cursor], 3);
            } else {
                HunterInfo_Open(m->rows[m->cursor], 7);
            }
            break;
        case 3:
            AreaSearch_Close();
            return 3;
        case 4:
            AreaSearch_Close();
            return 4;
        }
        break;
    case 3:
        switch (HunterInfo_Update(pad)) {
        case 0:
        case 1:
        case 3:
            m->unk148 = 2;
            m->state = 1;
            break;
        case 4:
            AreaSearch_Close();
            return 4;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            AreaSearch_Close();
            return 1;
        }
        if (pad & 0x8000) {
            AreaSearch_Close();
            return 4;
        }
        break;
    }
    return 2;
}

s32 Menu_CountPages(s16 n, s16 per);
void AreaSearch_BuildPage(PageMenu2 *m, s16 page)
{
    s16 i;
    s16 n = 0;
    s16 first = page * 10;
    s16 last = first + 9;

    for (i = 0; i < gFound6709->total; i++) {
        if (i >= first && i <= last && i != 99) {
            m->rows[n] = &gFound6709->users[i];
            n++;
        }
    }
    m->count = n;
    m->page = page;
    m->pageCount = Menu_CountPages(gFound6709->total, 10);
    if (m->cursor >= m->count) {
        m->cursor = m->count - 1;
    }
}

void AreaSearch_ShowResults(PageMenu2 *m)
{
    AreaSearch_BuildPage(m, 0);
    WinSlide_Step(&m->win, 0x280, 0x12C);
}

extern char gMyUserId[];
s32 memcmp(void *a, void *b, s32 n);
void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32, s32);
s32 Menu_WrapLeftRight(s32, s32);
s32 AreaSearch_ResultsInput(PageMenu2 *m, u16 pad)
{
    switch (m->state) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x12C)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            if (m->count != 0) {
                if (memcmp(m->rows[m->cursor], gMyUserId, 8) == 0) {
                    LobbySe_Play(9);
                    break;
                }
                m->state = 0;
                LobbySe_Play(8);
                return 0;
            }
        } else if (pad & 0x40) {
            m->state = 0;
            LobbySe_Play(4);
            return 3;
        } else if (pad & 0x8000) {
            return 4;
        } else if (pad & 0xC00) {
            if (m->pageCount > 1) {
                m->page = Menu_WrapLeftRight(m->page, m->pageCount);
                AreaSearch_BuildPage(m, m->page);
            }
        } else {
            m->cursor = Menu_WrapUpDown(m->cursor, m->count);
        }
        break;
    }
    return 2;
}

void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), AreaSearch_DrawResults 64 of 252 words differ */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Text_PrintfColored(const char *fmt, ...);
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Icon_DrawKind48(s16, s16, s32, s32, u8, s32); /* unit's own declaration: with the definition's void (s32 a, s32 b, s32 c, s32 d, s16 e, s8 f) (lobby/unk_78F070.c), AreaSearch_DrawResults 67 of 252 words differ */
void Sjis_Widen(char *src, char *dst);
extern char *D_0080CF54;
extern char *D_0080D378;
extern char *D_0080C4A4;
extern char *gHrLabelTexts[];
extern char *D_0080C4AC;
extern char *D_0080C4B4;
extern char *D_0080C55C;
extern char D_008381A0[];
extern char D_008381A8[];

void AreaSearch_DrawResults(PageMenu2 *m, s16 x, s16 y)
{
    u8 *e;
    char buf[0x20];
    s8 sel;
    s16 ty = y;
    s16 i;

    Win_Draw(x, y, 0x140, 0x122, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, y + 0xC, 0x130, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    y += 0xE;
    Font_SetPos(x + 0x10, y);
    Text_PrintfColored(D_0080CF54);
    if (m->count == 0) {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(Text_CenterX(D_0080D378, x + 0xA0, 0x12), y + 0x64);
        Text_PrintfColored(D_0080D378);
        return;
    }
    Menu_DrawPageNum(x + 0xE6, y, m->page, m->pageCount, 1);
    ty += 0x24;
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0x10, ty);
    Text_PrintfColored(D_0080C4A4);
    Font_SetPos(x + 0x62, ty);
    Text_PrintfColored(D_0080C4AC);
    Font_SetPos(x + 0xD8, ty);
    Text_PrintfColored(D_0080C4B4);
    Font_SetPos(x + 0x106, ty);
    Text_PrintfColored(gHrLabelTexts[0]);
    for (i = 0; i < m->count; i++) {
        Font_SetColor(0x807F7F7F);
        ty += 0x16;
        if (m->cursor == i) {
            switch (m->unk148) {
            case 2:
                sel = 0;
                break;
            default:
                sel = 1;
                break;
            }
            Menu_DrawLine(x + 8, ty - 1, 0x130, 0x14, sel);
        }
        Font_SetPos(x + 0x10, ty);
        Text_PrintfColored(D_008381A0, m->rows[i]->name);
        Font_SetPos(x + 0x62, ty);
        Sjis_Widen(m->rows[i]->id, buf);
        Text_PrintfColored(buf);
        e = m->rows[i]->text;
        Icon_DrawKind48(x + 0xE2, ty - 2, 0x16, -1, e[3], 0);
        Font_SetPos(x + 0x106, ty);
        Text_PrintfColored(D_008381A8, *(u16 *)e);
    }
    if (m->page == 9 && i == 9) {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(x + 0x10, ty + 0x16);
        Text_PrintfColored(D_008381A0, D_0080C55C);
    }
}

extern u8 gMenuWork;
void Font_SetSpacing(u8 a, u8 b);
void AreaSearch_DrawResults(PageMenu2 *m, s16 x, s16 y);
void HunterInfo_DrawIfOpen(void);
void AreaSearch_Draw(u8 *o)
{
    s8 *w = (s8 *)&gMenuWork;

    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    switch (w[0x148]) {
    case 2:
        AreaSearch_DrawResults((PageMenu2 *)w, *(s16 *)w, 0x3A);
        break;
    case 3:
        HunterInfo_DrawIfOpen();
        break;
    }
}

void AreaSearch_Draw2(u8 *a0)
{
    Text_SetSize(*(s32 *)(a0 + 0x18));
}

s32 LobbyLevel0_HasGroups(void)
{
    return *(u8 *)((u8 *)&D_5C81EA + ((gLobbyLevel0Sel - 1) * 0x164)) == 4;
}

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s16 gLobbyLevel1Count;
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
u16 Rand_Next(s32 stream);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void AreaBoard_Draw(MenuTask *task);
void AreaBoard_Draw2(void *task);
void AreaBoard_Open(void)
{
    RoomMenu *w = (RoomMenu *)&gMenuWork;

    gTownMenuOpen = 1;
    gTownMode = 0x3D;
    memset(w, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(AreaBoard_Draw, AreaBoard_Draw2, 0, 0);
    w->unkB66 = Menu_CountPages(gLobbyLevel1Count, 10);
    WinSlide_Step(&w->listWin, -0x104, 0x36);
    w->unkB6E = Rand_Next(1);
    Sound_PlaySeFlat(1, 0x95, 0);
}

/* unk_6EE870's .rodata (0x8381A0-0x8381B0) */
#pragma rodata on
char D_008381A0[0x8] = {
#include "lobby/008381A0.inc"
};
char D_008381A8[0x8] = {
#include "lobby/008381A8.inc"
};
#pragma rodata off
