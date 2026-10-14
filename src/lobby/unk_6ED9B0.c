#include "common.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "pad.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#pragma optimization_level 4
s32 Menu_WrapUpDown(s32, s32);

/* prototypes */
void Text_SetSize(s32);
/* end of prototypes */

/* the 6711 search's results (LobbyNet_Get6711) */
extern NetFound6711 *gFound6711;

void NameSearch_BuildPage(NameSearchMenu *m, s16 page);
s32 WinSlide_Step(WinSlide *, s16 from, s16 to);

extern u8 gMenuWork;
extern LobbyMenu *gLobbyMenu;
extern u8 gLobbyLevel1Sel;
void LobbyText_Open(s32 msg);
s32 NameSearch_InputName(NameSearchMenu *m, u16 pad);
s32 NameSearch_ResultsInput(NameSearchMenu *m, u16 pad);
void NameSearch_ShowResults(NameSearchMenu *m);
void NameSearch_Close(void);
s32 LobbyUser_FindByName(s8 *name);
s32 HunterInfo_Update(u16 pad);
void HunterInfo_Open(void *, s32);

s32 NameSearch_Update(u16 pad)
{
    NameSearchMenu *m = (NameSearchMenu *)&gMenuWork;

    switch (m->unk52) {
    case 0:
        switch (NameSearch_InputName(m, pad)) {
        case 0:
            m->unk52++;
            LobbyText_Open(0x20);
            gLobbyMenu->redraw = 1;
            break;
        case 3:
            NameSearch_Close();
            return 3;
        case 4:
            NameSearch_Close();
            return 4;
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (LobbyUser_FindByName(&m->unk38)) {
        case 0:
            m->unk52++;
            NameSearch_ShowResults(m);
            break;
        case 1:
            m->unk52 = 4;
            break;
        }
        break;
    case 2:
        switch (NameSearch_ResultsInput(m, pad)) {
        case 0:
            m->unk52 = 3;
            if (gLobbyLevel1Sel != 0) {
                HunterInfo_Open(m->rows[m->cursor], 3);
            } else {
                HunterInfo_Open(m->rows[m->cursor], 7);
            }
            break;
        case 3:
            m->unk52 = 0;
            m->state = 1;
            break;
        case 4:
            NameSearch_Close();
            return 4;
        }
        break;
    case 3:
        switch (HunterInfo_Update(pad)) {
        case 0:
        case 1:
        case 3:
            gFieldControl = 0;
            m->unk52 = 2;
            m->state = 1;
            break;
        case 4:
            NameSearch_Close();
            return 4;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            NameSearch_Close();
            return 1;
        }
        if (pad & 0x8000) {
            NameSearch_Close();
            return 4;
        }
        break;
    }
    return 2;
}

void LobbySe_Play(s32 se);
void LobbyDialog_Grow(void);
void LobbyText_Open(s32 msg);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
void Kbd_Cancel(void);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
extern LobbyMenu *gLobbyMenu;
s32 NameSearch_InputName(NameSearchMenu *m, u16 pad)
{
    switch (m->state) {
    case 0:
        switch (WinSlide_Step(&m->win2, 0x280, 0x190)) {
        case 0:
            m->state++;
            m->unk48 = 0;
            break;
        }
        break;
    case 1:
        if (pad & 0x20) {
            if (m->unk48 == 0) {
                m->state++;
                Kbd_SetPos(0x3A, 80.0f);
                Chat_OpenKbd(3, 0xF, 8, &m->unk38);
                LobbySe_Play(8);
            } else if (m->unk38 == 0) {
                m->state = 3;
                LobbyText_Open(0x24);
                LobbyDialog_Grow();
                LobbySe_Play(9);
            } else {
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
        } else {
            m->unk48 = Menu_WrapUpDown(m->unk48, 2);
        }
        break;
    case 2:
        if (Kbd_Run((char *) &m->unk38, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            Kbd_Cancel();
            if (m->unk38 == 0) {
                m->state = 3;
                LobbyText_Open(0x24);
                LobbyDialog_Grow();
            } else {
                m->state = 1;
                m->unk48++;
            }
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->state = 1;
            LobbySe_Play(8);
        } else if (pad & 0x8000) {
            return 4;
        }
        break;
    }
    return 2;
}

s32 Menu_CountPages(s16 n, s16 per);
void NameSearch_BuildPage(NameSearchMenu *m, s16 page)
{
    s16 i;
    s16 n = 0;
    s16 first = page * 10;
    s16 last = first + 9;

    for (i = 0; i < gFound6711->total; i++) {
        if (i >= first && i <= last) {
            m->rows[n] = &gFound6711->list[i];
            n++;
        }
    }
    m->count = n;
    m->page = page;
    m->pageCount = Menu_CountPages(gFound6711->total, 10);
    if (m->cursor >= m->count) {
        m->cursor = m->count - 1;
    }
}

void NameSearch_ShowResults(NameSearchMenu *m) {
    m->cursor = 0;
    NameSearch_BuildPage(m, 0);
    WinSlide_Step(&m->win, 0x280, 0x12C);
}

extern char gMyUserId[];
s32 memcmp(void *a, void *b, s32 n);
s32 Menu_WrapUpDown(s32, s32);
s32 Menu_WrapLeftRight(s32, s32);
s32 NameSearch_ResultsInput(NameSearchMenu *m, u16 pad)
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
        } else if (pad & 0xC00) {
            if (m->pageCount > 1) {
                m->page = Menu_WrapLeftRight(m->page, m->pageCount);
                NameSearch_BuildPage(m, m->page);
            }
        } else if (pad & 0x8000) {
            return 4;
        } else {
            m->cursor = Menu_WrapUpDown(m->cursor, m->count);
        }
        break;
    }
    return 2;
}

extern char *gNameSearchTexts[];
extern char *gSearchStartTexts[];
extern char D_00838198[];
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), NameSearch_DrawResults 206 of 230 words, NameSearch_DrawForm 10 of 152 words differ */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
u32 Color_PulseAlpha(u32 color);
void Menu_DrawSideArrows(s16 x, s16 y, s16 w, s16 size, s8 flip, u32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);

void Sjis_Widen(char *src, char *dst);
void Menu_DrawLine(s16 x, s16 y, s32 w, s32 h, s32 sel); /* unit's own declaration: with the definition's void (s16 x0, s16 y0, s16 x1, s16 y1, s8 flag) (lobby/unk_790EC0.c), NameSearch_DrawResults 86 of 230 words differ */
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Text_Printf(const char *fmt, ...);
extern char *D_0080D378;
extern char *D_0080C4A4;
extern char *D_0080C4AC;
extern char *gConnStateTexts[];
extern char *gCheckingStatusTexts[];
extern char *D_0080C50C;
extern char *gOnQuestStatusTexts[];
extern char *D_0080C514;
void NameSearch_DrawResults(NameSearchMenu *m, s16 x, s16 y)
{
    s16 ty = y;
    s16 i;
    s32 tx;
    s32 sel;
    char *s;
    char buf[32];

    Win_Draw(x, y, 0x140, 0x122, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, y + 0xC, 0x130, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    tx = x + 0x10;
    Font_SetPos(tx, y + 0xE);
    Text_PrintfColored(gNameSearchTexts[0]);
    if (m->count == 0) {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(Text_CenterX(D_0080D378, x + 0xA0, 0x12), (s16)(y + 0xE) + 0x64);
        Text_PrintfColored(D_0080D378);
        return;
    }
    Menu_DrawPageNum(x + 0xE6, y + 0xE, m->page, m->pageCount, 1);
    ty += 0x24;
    Font_SetColor(0x807F7F28);
    Font_SetPos(tx, ty);
    Text_PrintfColored(D_0080C4A4);
    Font_SetPos(x + 0x62, ty);
    Text_PrintfColored(D_0080C4AC);
    Font_SetPos(x + 0xD8, ty);
    Text_PrintfColored(gConnStateTexts[0]);
    for (i = 0; i < m->count; i++) {
        Font_SetColor(0x807F7F7F);
        ty += 0x16;
        if (m->cursor == i) {
            switch (m->unk52) {
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
        Text_PrintfColored(D_00838198, m->rows[i]->name);
        Font_SetPos(x + 0x62, ty);
        Sjis_Widen(m->rows[i]->id, buf);
        Text_PrintfColored(buf);
        Font_SetPos(x + 0xD8, ty);
        switch ((u8)m->rows[i]->text[0]) {
        case -1:
            s = gCheckingStatusTexts[0];
            break;
        case 0:
            s = D_0080C50C;
            break;
        case 1:
            s = gOnQuestStatusTexts[0];
            break;
        case 2:
            s = D_0080C514;
            break;
        }
        Text_Printf(s);
    }
}

void NameSearch_DrawForm(s8 *w, s16 x, s16 y)
{
    s16 tx;
    u32 c1;
    u32 c2;
    s16 ty = y;

    Win_Draw(x, y, 0xC8, 0x82, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, ty + 0xC, 0xB8, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0x10, ty + 0xE);
    Text_PrintfColored(gNameSearchTexts[0]);
    ty += 0x36;
    if (*(s16 *)(w + 0x48) == 0) {
        c1 = Color_PulseAlpha(0x8080);
        c2 = 0x80404040;
        Menu_DrawSideArrows(x + 0xA, ty, 0xB4, 0x16, 0, 0xFF008080);
    } else {
        c1 = 0x80808080;
        c2 = 0x802B7F7F;
    }
    Gfx_DrawRectFrame(x + 0x14, ty, 0xA0, 0x18, c1);
    Gfx_DrawRectFrame(x + 0x13, ty - 1, 0xA2, 0x1A, c1);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(Text_CenterX((char *)(w + 0x38), x + 0x64, 0x12), ty + 4);
    Text_PrintfColored(D_00838198, w + 0x38);
    Font_SetColor(c2);
    tx = Text_CenterX(gSearchStartTexts[0], x + 0x64, 0x12);
    Font_SetPos(tx, ty += 0x28);
    Text_PrintfColored(gSearchStartTexts[0]);
    if (*(s16 *)(w + 0x48) == 1) {
        Menu_DrawArrowsRev(tx, ty, 0x12, gSearchStartTexts[0], 0xFF008080);
    }
}

extern u8 gMenuWork;
void Font_SetSpacing(u8 a, u8 b);
void NameSearch_DrawForm(s8 *w, s16 id, s16 y);
void NameSearch_DrawResults(NameSearchMenu *m, s16 x, s16 y);
void HunterInfo_DrawIfOpen(void);
void NameSearch_Draw(u8 *o)
{
    s8 *w = (s8 *)&gMenuWork;

    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    switch (w[0x52]) {
    case 0:
        NameSearch_DrawForm(w, *(s16 *)(w + 8), 0x3A);
        break;
    case 2:
        NameSearch_DrawResults((NameSearchMenu *)w, *(s16 *)w, 0x3A);
        break;
    case 3:
        HunterInfo_DrawIfOpen();
        break;
    }
}

void NameSearch_Draw2(u8 *a0)
{
    Text_SetSize(*(s32 *)(a0 + 0x18));
}

/* unk_6ED9B0's .rodata (0x838198-0x8381A0) */
#pragma rodata on
char D_00838198[0x8] = {
#include "lobby/00838198.inc"
};
#pragma rodata off
