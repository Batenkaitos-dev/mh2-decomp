#include "common.h"
#include "sprite.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "lobby_state.h"
#include "lobby_menu.h"

#pragma optimization_level 4

typedef struct V006E5D50_0 {
    u8 pad0[0x80];
    s16 f80;
} V006E5D50_0;
typedef struct V006E5DE0_0 {
    u8 pad0[0x78];
    s16 f78;
    s16 f7A;
    s16 f7C;
    s16 f7E;
} V006E5DE0_0;

/* The list menu at 0x8BD610 (0x84 bytes, set up by FriendList_Open) */
typedef struct ListMenu {
    WinSlide win;
    u8 unk8[0x40];
    s32 list[10];               /* 0x48: the page's record indexes */
    s32 unk70;
    s32 unk74;                  /* 0x74: FriendList_Open's argument */
    s16 page;                   /* 0x78 */
    s16 pageCount;              /* 0x7A */
    s16 cursor;                 /* 0x7C */
    s16 count;                  /* 0x7E */
    s16 unk80;
    s8 state;                   /* 0x82 */
    s8 step;                    /* 0x83 */
} ListMenu;

/* 50 records of 0x30 bytes at 0x5C39D0 */
typedef struct Rec5C39D0 {
    s8 used;
    u8 unk1[0x2F];
} Rec5C39D0;

void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);
extern s8 gFriendStatus[];
extern M2C_UNK D_008BD658;
extern s16 D_008BD68C;
extern Rec5C39D0 gFriendList[];

struct Rec30;
s32 FriendList_DeleteAt(struct Rec30 *, u8 idx, u8 n);
extern s8 D_008BD601;

extern s8 gFriendListOpen;
static void FriendList_ClearOpen(void)
{
    gFriendListOpen = 0;
}

void *FriendList_GetSelectedId(void)
{
    return (u8 *)&gFriendList + (*(M2C_UNK *)((u8 *)&D_008BD658 + (D_008BD68C * 4)) * 0x30);
}

void *FriendList_GetSelectedName(void)
{
    return (u8 *)&gFriendList + (*(M2C_UNK *)((u8 *)&D_008BD658 + (D_008BD68C * 4)) * 0x30) + 8;
}

extern u8 gFriendListMenu;
extern s8 D_008BD692;
extern u8 gLobbyLevel1Sel;
extern LobbyMenu *gLobbyMenu;
s32 WinSlide_Step(WinSlide *m, s16 from, s16 to);
s32 FriendList_HasUnchecked(u8 *arg0);
s32 FriendList_CheckStatus(ListMenu *m);
typedef struct V006E5D50_0 V006E5D50_0;
typedef struct V006E5DE0_0 V006E5DE0_0;
s32 FriendList_ListInput(V006E5DE0_0 *arg0, u16 arg1);
s32 FriendList_ActionInput(V006E5D50_0 *arg0, u16 arg1);
s32 FriendList_RemoveInput(ListMenu *m, u16 pad);
void FriendList_BuildPage(ListMenu *m, s16 page);
void HunterInfo_Open(void *src, s32 flags);
s32 HunterInfo_Update(u16 pad);
void LobbyText_Open(s32);
void Menu_SetYesNo(s8 a);
void LobbyDialog_Grow(void);
void LobbyDialog_SetMember(void *a, void *b);
s32 OT_AddAt(u32 *, void *, s32, s32);
s32 FriendList_Update(u16 pad)
{
    ListMenu *m = (ListMenu *)&gFriendListMenu;
    s32 id;

    switch (D_008BD692) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0xE6)) {
        case 0:
            if (m->count != 0 && FriendList_HasUnchecked((u8 *)m) == 1) {
                m->state++;
            } else {
                m->state = 2;
            }
            break;
        }
        break;
    case 1:
        switch (FriendList_CheckStatus(m)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 2:
        switch (FriendList_ListInput((V006E5DE0_0 *)m, pad)) {
        case 0:
            FriendList_BuildPage(m, m->page);
            if (m->count != 0 && FriendList_HasUnchecked((u8 *)m) == 1) {
                m->state = 1;
            }
            break;
        case 1:
            if (m->unk74 == 1) {
                return 0;
            }
            m->state = 3;
            m->unk80 = 0;
            break;
        case 3:
            FriendList_ClearOpen();
            return 3;
        case 4:
            return 4;
        }
        break;
    case 3:
        switch (FriendList_ActionInput((V006E5D50_0 *)m, pad)) {
        case 0:
            id = m->list[m->cursor];
            switch (m->unk80) {
            case 0:
                m->state = 5;
                if (gLobbyLevel1Sel) {
                    HunterInfo_Open(&gFriendList[id], 1);
                } else {
                    HunterInfo_Open(&gFriendList[id], 5);
                }
                break;
            case 1:
                m->state = 4;
                LobbyText_Open(0x1E);
                Menu_SetYesNo(1);
                LobbyDialog_Grow();
                LobbyDialog_SetMember(&gFriendList[id], &gFriendList[id].unk1[7]);
                gLobbyMenu->redraw = 1;
                gFieldControl = 0;
                break;
            }
            break;
        case 3:
            m->state = 2;
            break;
        case 4:
            return 4;
        }
        break;
    case 4:
        switch (FriendList_RemoveInput(m, pad)) {
        case 0:
            m->state = 1;
            gFieldControl = 1;
            break;
        case 3:
            m->state = 3;
            gFieldControl = 1;
            break;
        case 4:
            gFieldControl = 1;
            return 4;
        }
        break;
    case 5:
        switch (HunterInfo_Update(pad)) {
        case 0:
        case 1:
        case 3:
            m->state = 3;
            break;
        case 4:
            return 4;
        }
        break;
    }
    OT_AddAt(gDrawListText0b, (u8 *)m + 8, 0, 1);
    OT_AddAt(gDrawListText1, (u8 *)m + 0x28, 0, 1);
    return 2;
}

s32 FriendList_HasUnchecked(u8 *m)
{
    s16 n;
    s32 i;
    u8 *cur;

    cur = m;
    n = *(s16 *)((u8 *)cur + 0x7E);
    i = 0;
    if (0 < n) {
loop_2:
        if (gFriendStatus[*(s32 *)((u8 *)cur + 0x48)] == -1) {
            return 1;
        }
        i += 1;
        cur += 4;
        if (i >= n) {
            goto block_6;
        }
        goto loop_2;
    }
block_6:
    return 0;
}

void FriendList_Remove(s32 idx)
{
    s32 i;
    u8 *s;

    i = idx;
    FriendList_DeleteAt((struct Rec30 *)&gFriendList, idx & 0xFF, 0x32);
    if (i < 0x31) {
        s = (u8 *)gFriendStatus + i;
        do {
            i += 1;
            *(s8 *)((u8 *)s + 0) = (s8) *(s8 *)((u8 *)s + 1);
            s += 1;
        } while (i < 0x31);
    }
    D_008BD601 = -1;
}

extern s8 gMenuInputLock;
s32 Menu_YesNoInput(void);
void LobbyText_Open(s32);
void LobbyDialog_SetMember(void *a, void *b);
void FriendList_Remove(s32 idx);
void FriendList_BuildPage(ListMenu *m, s16 page);
s32 FriendList_RemoveInput(ListMenu *m, u16 pad)
{
    s32 idx;
    u8 *r;

    if (pad & 0x8000) {
        return 4;
    }
    gLobbyMenu->redraw = 1;
    switch (m->step) {
    case 0:
        switch (Menu_YesNoInput()) {
        case 0:
            m->step++;
            LobbyText_Open(0x1F);
            idx = m->list[m->cursor];
            r = (u8 *)&gFriendList[idx];
            LobbyDialog_SetMember(r, r + 8);
            FriendList_Remove(idx);
            FriendList_BuildPage(m, m->page);
            if (m->cursor >= m->count) {
                m->cursor = m->count - 1;
            }
            gMenuInputLock = 3;
            break;
        case 3:
            m->step = 0;
            return 3;
        }
        break;
    case 1:
        if (pad & 0x20) {
            m->step = 0;
            LobbySe_Play(8);
            return 0;
        }
        break;
    }
    return 2;
}

s32 FriendList_ActionInput(V006E5D50_0 *m, u16 pad)
{
    s32 btn;

    btn = pad & 0xFFFF;
    if (btn & 0x20) {
        LobbySe_Play(8);
        return 0;
    }
    if (btn & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    if (btn & 0x8000) {
        return 4;
    }
    m->f80 = Menu_WrapUpDown((s32) m->f80, 2);
    return 2;
}

s32 FriendList_ListInput(V006E5DE0_0 *m, u16 pad)
{
    s16 f7A;
    s32 btn;

    btn = pad & 0xFFFF;
    if (btn & 0x20) {
        if (m->f7E != 0) {
            LobbySe_Play(8);
            return 1;
        }
        goto block_13;
    }
    if (btn & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    if (btn & 0x8000) {
        return 4;
    }
    if (btn & 0xC00) {
        f7A = m->f7A;
        if (f7A > 1) {
            m->f78 = Menu_WrapLeftRight((s32) m->f78, (s32) f7A);
            return 0;
        }
        goto block_13;
    }
    m->f7C = Menu_WrapUpDown((s32) m->f7C, (s32) m->f7E);
block_13:
    return 2;
}

extern s16 gFriendCheckTimers[];
extern u16 gFriendCheckFrames;
s32 LobbyUser_FindById(s8 *id);
void LobbyNet_Get6707(u16 *arg0, u16 *arg1, u16 *arg2, u8 *arg3);
s32 FriendList_CheckStatus(ListMenu *m)
{
    s32 id;
    u16 tmp;

    switch (m->step) {
    case 0:
        gFriendCheckTimers[m->page] = gFriendCheckFrames;
        m->step++;
        m->unk70 = 0;
    case 1:
        if (gFriendStatus[m->list[m->unk70]] != -1) {
            if (++m->unk70 >= m->count) {
                m->step = 0;
                return 0;
            }
            break;
        }
        m->step++;
    case 2:
        id = m->list[m->unk70];
        switch (LobbyUser_FindById((s8 *)&gFriendList[id])) {
        case 0:
            LobbyNet_Get6707(&tmp, &tmp, &tmp, (u8 *)&gFriendStatus[id]);
            if (++m->unk70 >= m->count) {
                m->step = 0;
                return 0;
            }
            m->step--;
            break;
        case 3:
        case 1:
            gFriendStatus[id] = 2;
            if (++m->unk70 >= m->count) {
                m->step = 0;
                return 0;
            }
            m->step--;
            break;
        }
        break;
    default:
        m->step = 0;
        return 0;
    }
    return 2;
}

s32 Menu_CountPages(s16 n, s16 per);
void FriendList_BuildPage(ListMenu *m, s16 page)
{
    s32 i;
    s32 n = 0;
    s16 first;
    s16 last;

    m->count = 0;
    first = page * 10;
    last = first + 9;
    for (i = 0; i < 50; i++) {
        if (gFriendList[i].used != 0) {
            if (n >= first && n <= last) {
                m->list[m->count] = i;
                m->count++;
            }
            n++;
        }
    }
    m->page = page;
    m->pageCount = Menu_CountPages(n, 10);
    if (page >= m->pageCount) {
        FriendList_BuildPage(m, page - 1);
    }
    if (m->cursor >= m->count) {
        m->cursor = m->count - 1;
    }
}

void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), FriendList_DrawList 219 of 236 words, FriendList_DrawActions 98 of 102 words differ */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Menu_DrawLine(s16 x, s16 y, s32 w, s32 h, s32 sel); /* unit's own declaration: with the definition's void (s16 x0, s16 y0, s16 x1, s16 y1, s8 flag) (lobby/unk_790EC0.c), FriendList_DrawList 84 of 236 words, FriendList_DrawActions 37 of 104 words differ */
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
extern char *gGroupBoardTexts[];
extern char *D_0080C0CC;
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Sjis_Widen(char *src, char *dst);
void FriendList_DrawList(ListMenu *m, s16 x, s16 y)
{
    Rec5C39D0 *r;
    char buf[64];
    char *s;
    s32 sel;
    s32 i;
    s32 id;
    s16 ly = y;

    Win_Draw(x, y, 0x186, 0x122, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, y + 0xC, 0x176, 0x14, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0x10, y + 0xE);
    Text_PrintfColored(D_0080C0CC);
    if (m->count == 0) {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(Text_CenterX(gGroupBoardTexts[29], x + 0xC3, 0x12), (s16)(y + 0xE) + 0x5A);
        Text_PrintfColored(gGroupBoardTexts[29]);
        return;
    }
    Menu_DrawPageNum(x + 0x12C, y + 0xE, m->page, m->pageCount, 1);
    ly += 0x24;
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0x10, ly);
    Text_PrintfColored(gGroupBoardTexts[5]);
    Font_SetPos(x + 0xAA, ly);
    Text_PrintfColored(gGroupBoardTexts[7]);
    Font_SetPos(x + 0x120, ly);
    Text_PrintfColored(gGroupBoardTexts[8]);
    for (i = 0; i < m->count; i++) {
        id = m->list[i];
        r = &gFriendList[id];
        Font_SetColor(0x807F7F7F);
        ly += 0x16;
        if (m->cursor == i) {
            switch (m->state) {
            case 2:
                sel = 0;
                break;
            default:
                sel = 1;
                break;
            }
            Menu_DrawLine(x + 8, ly - 1, 0x176, 0x14, sel);
        }
        Font_SetPos(x + 0x10, ly);
        Text_PrintfColored("%s", &r->unk1[7]);
        Font_SetPos(x + 0xAA, ly);
        Sjis_Widen((char *)r, buf);
        Text_PrintfColored(buf);
        Font_SetPos(x + 0x120, ly);
        switch (gFriendStatus[id]) {
        case -1:
            s = gGroupBoardTexts[30];
            break;
        case 0:
            s = gGroupBoardTexts[31];
            break;
        case 1:
            s = gGroupBoardTexts[32];
            break;
        case 2:
            s = gGroupBoardTexts[33];
            break;
        }
        Text_Printf(s);
    }
}

void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), FriendList_DrawList 219 of 236 words, FriendList_DrawActions 98 of 102 words differ */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Menu_DrawLine(s16 x, s16 y, s32 w, s32 h, s32 sel); /* unit's own declaration: with the definition's void (s16 x0, s16 y0, s16 x1, s16 y1, s8 flag) (lobby/unk_790EC0.c), FriendList_DrawList 84 of 236 words, FriendList_DrawActions 37 of 104 words differ */
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(char *fmt, ...);
extern char *gGroupBoardTexts[];
void FriendList_DrawActions(u8 *m, s16 x, s16 y)
{
    s16 i;
    s32 sel;

    Win_Draw(x - 0x6E, y, 0x6E, 0x3A, 0xBDB59F6C, 0xBB402010, 3);
    y += 8;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < 2; i++) {
        if (i > 0) {
            Gfx_DrawLine((s16)(x - 0x6E) + 8, y - 2, 0x5E, 2, 0xBDB59F6C);
        }
        if (*(s16 *)(m + 0x80) == i) {
            switch ((s8)m[0x82]) {
            case 3:
                sel = 0;
                break;
            default:
                sel = 1;
                break;
            }
            Menu_DrawLine((s16)(x - 0x6E) + 8, y - 1, 0x5E, 0x14, sel);
        }
        Font_SetPos((s16)(x - 0x6E) + 8, y);
        Text_PrintfColored(gGroupBoardTexts[0x22 + i]);
        y += 0x16;
    }
}

extern u8 gFriendListMenu;
extern s8 D_008BD692;
void Text_SetSize(s32);
void Font_SetSpacing(u8 a, u8 b);
void FriendList_DrawActions(u8 *m, s16 x, s16 y);
void FriendList_Draw(u8 *o)
{
    u8 *w = &gFriendListMenu;
    s16 id;

    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    if (D_008BD692 != 5) {
        id = *(s16 *)w;
        FriendList_DrawList((ListMenu *)w, id, 0x3A);
        switch (*(s8 *)(w + 0x82)) {
        case 4:
        case 3:
            FriendList_DrawActions(w, id, 0x3A);
            break;
        }
    }
}

extern s8 D_008BD692;
void Text_SetSize(s32);
void Font_SetSpacing(u8 a, u8 b);
void HunterInfo_DrawIfOpen(void);
void FriendList_Draw2(u8 *o)
{
    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    switch (D_008BD692) {
    case 5:
        HunterInfo_DrawIfOpen();
        break;
    }
}
