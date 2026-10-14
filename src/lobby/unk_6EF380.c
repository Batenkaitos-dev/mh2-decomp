#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "lobby_menu.h"
void *memset(void *, s32, u32);
void LobbyText_Open(s32 msg);

u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
s32 Win_Shake(WinShake *s);
s32 LobbyStep_DownloadQuests(void);
void LobbyTown_Begin(void);
void Stream_Stop(s32 ch);
void AreaBoard_Close(void);
s32 AreaBoard_ListInput(u16 pad);
s32 AreaBoard_RecruitInput(u16 pad);
s32 AreaBoard_EnterStep(u16 pad);
s32 AreaBoard_Join(u16 pad, RoomMenu *m, s32 slot);
extern u8 gMenuWork[];
extern LobbyMenu *gLobbyMenu;
extern s8 gLobbyLevel1Sel;
s32 WinSlide_Step(WinSlide *m, s16 from, s16 to);
void LobbySe_Play(s32);
void AreaBoard_Update(void)
{
    RoomMenu *m = (RoomMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (m->state) {
    case 0:
        switch (WinSlide_Step(&m->listWin, -0x104, 0x36)) {
        case 0:
            m->state = 2;
            break;
        }
        break;
    case 1:
        switch (WinSlide_Step(&m->listWin, 0x36, -0x104)) {
        case 0:
            AreaBoard_Close();
            break;
        }
        break;
    case 2:
        switch (AreaBoard_ListInput(pad)) {
        case 0:
            m->state = 3;
            m->unkB90 = 0;
            WinSlide_Step(&m->win, 0x280, 0x140);
            break;
        case 3:
            m->state = 1;
            WinSlide_Step(&m->listWin, 0x36, -0x104);
            break;
        case 1:
            m->state = 4;
            m->unkB90 = 1;
            gLobbyLevel1Sel = m->slot;
            break;
        }
        break;
    case 4:
        switch (AreaBoard_RecruitInput(pad)) {
        case 0:
            m->state = 5;
            m->unkB90 = 1;
            Win_Shake(&m->shake);
            m->unkB91 = 1;
            break;
        case 3:
            m->state = 7;
            break;
        case 1:
            m->state = 8;
            m->unkB5F = 2;
            break;
        }
        break;
    case 3:
        switch (AreaBoard_Join(pad, m, m->slot)) {
        case 0:
            m->state = 5;
            m->win3.pos = *(s16 *)m;
            Win_Shake(&m->shake);
            m->unkB91 = 1;
            break;
        case 3:
            m->state = 2;
            break;
        case 1:
            m->state = 8;
            m->unkB5F = 2;
            break;
        }
        break;
    case 5:
        switch (Win_Shake(&m->shake)) {
        case 0:
            (m->state)++;
            break;
        }
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_DownloadQuests()) {
        case 0:
        case 1:
            LobbyTown_Begin();
            Stream_Stop(0);
            Stream_Stop(1);
            break;
        }
        break;
    case 7:
        gLobbyMenu->redraw = 1;
        switch (AreaBoard_EnterStep(pad)) {
        case 0:
            m->state = 2;
            break;
        case 1:
            m->state = 8;
            m->unkB5F = 2;
            break;
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->state = m->unkB5F;
            LobbySe_Play(8);
        }
        break;
    }
}

s32 AreaBoard_JoinInput(u16 pad, RoomMenu *m, s32 slot);
s32 Level1_CanEnter(s32 slot);
s32 LobbyStep_EnterLevel1(void);
s32 LobbyStep_630AUsers(NetUser *users, s16 slot); /* unit's own declaration: with the definition's s32 (NetUser *users, s32 id) (lobby/unk_7B3140.c), AreaBoard_Join 122 of 147 words differ */
s32 AreaBoard_Join(u16 pad, RoomMenu *m, s32 slot)
{
    switch (m->step) {
    case 0:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_630AUsers((NetUser *) m->unk8, slot)) {
        case 0:
            m->step++;
            m->page = 0;
            if (m->unk594 == 0) {
                m->pages = 1;
            } else {
                m->pages = 2;
            }
            WinSlide_Step(&m->win, 0x280, 0x140);
            break;
        }
        break;
    case 1:
        switch (WinSlide_Step(&m->win, 0x280, 0x140)) {
        case 0:
            m->step = 3;
            break;
        }
        break;
    case 2:
        switch (WinSlide_Step(&m->win, 0x140, 0x280)) {
        case 0:
            m->step = 0;
            return 3;
        }
        break;
    case 3:
        switch (AreaBoard_JoinInput(pad, m, slot)) {
        case 0:
            if (Level1_CanEnter(slot) == 1) {
                m->step++;
                gLobbyLevel1Sel = slot;
                break;
            }
            m->step = 0;
            gLobbyLevel1Sel = 0;
            LobbyText_Open(0x88);
            return 1;
        case 3:
            m->step = 2;
            WinSlide_Step(&m->win, 0x140, 0x280);
            break;
        case 1:
            m->step = 0;
            return 1;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_EnterLevel1()) {
        case 0:
            m->step = 0;
            return 0;
        case 1:
            m->step = 0;
            gLobbyLevel1Sel = 0;
            return 1;
        }
        break;
    }
    return 2;
}

extern u8 D_5C41E2[];
void LobbyText_Open(s32 msg);
s32 Menu_WrapUpDown(s32, s32);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);
s32 AreaBoard_ListInput(u16 pad)
{
    RoomMenu *w = (RoomMenu *)&gMenuWork;
    s32 idx;
    u8 st;

    if (pad & 0x20) {
        idx = w->slotCursor + w->unkB64 * 10 + 1;
        st = D_5C41E2[idx * 0x164];
        if (st == 3 || st == 4) {
            w->slot = idx;
            LobbyText_Open(0x78);
            LobbySe_Play(8);
            return 0;
        } else if (st == 1) {
            w->slot = idx;
            LobbyText_Open(0x4B);
            LobbySe_Play(8);
            return 1;
        } else {
            LobbySe_Play(9);
        }
    } else if (pad & 0x40) {
        w->slotCursor = 0;
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0xC00) {
        w->unkB64 = Menu_WrapLeftRight(w->unkB64, w->unkB66);
    } else {
        w->slotCursor = Menu_WrapUpDown(w->slotCursor, 10);
    }
    return 2;
}

extern LevelEntry gLobbyLevel1Entries[];
void LobbyDialog_Grow(void);
void Menu_SetYesNo(s8 a);
s32 Menu_YesNoInput(void);
void HunterInfo_Open(void *src, s32 flags);
s32 HunterInfo_Update(u16 pad);
s32 AreaBoard_JoinInput(u16 pad, RoomMenu *m, s32 slot)
{
    LevelEntry *rec = &gLobbyLevel1Entries[slot - 1];
    s32 i;

    switch (m->joinStep) {
    case 0:
        if (rec->state != 3 && rec->state != 4) {
            LobbyText_Open(0x49);
            LobbySe_Play(4);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
            return 1;
        }
        if (pad & 0x20) {
            m->joinStep = 3;
            LobbyText_Open(0x8B);
            Menu_SetYesNo(1);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
            LobbySe_Play(8);
        } else if (pad & 0x200) {
            m->shown = 0;
            for (i = 0; i < 5; i++) {
                if ((s8)m->unk8[i + m->page * 5][0] != 0) {
                    m->shown++;
                }
            }
            if (m->shown != 0) {
                m->joinStep++;
                m->unkB28 = 1;
                m->cursor = 0;
                LobbySe_Play(1);
            } else {
                LobbySe_Play(9);
            }
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            return 3;
        } else {
            m->page = Menu_WrapLeftRight(m->page, m->pages);
        }
        break;
    case 1:
        if (pad & 0x20) {
            m->joinStep++;
            m->unkB28 = 2;
            HunterInfo_Open(m->unk8[m->cursor + m->page * 5], 7);
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            m->joinStep--;
            m->unkB28 = 0;
            LobbySe_Play(4);
        } else if (pad & 0x3000) {
            m->cursor = Menu_WrapUpDown(m->cursor, m->shown);
        } else if (pad & 0xC00) {
            m->page = Menu_WrapLeftRight(m->page, m->pages);
            m->shown = 0;
            for (i = 0; i < 5; i++) {
                if ((s8)m->unk8[i + m->page * 5][0] != 0) {
                    m->shown++;
                }
            }
            if (m->cursor >= m->shown) {
                m->cursor = m->shown - 1;
            }
        }
        break;
    case 2:
        switch (HunterInfo_Update(pad)) {
        case 0:
        case 1:
        case 3:
            m->joinStep--;
            m->unkB28 = 1;
            break;
        }
        break;
    case 3:
        if (rec->state != 3 && rec->state != 4) {
            m->joinStep = 0;
            LobbyText_Open(0x49);
            LobbySe_Play(4);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
            return 1;
        }
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case 0:
            m->joinStep = 0;
            LobbyText_Open(0x78);
            gLobbyLevel1Sel = slot;
            return 0;
        case 3:
            m->joinStep = 0;
            break;
        }
        break;
    }
    return 2;
}

/* A room of the online town (the lobby work buffer + 0x6EF4 + 0x20). */
typedef struct Slot6EF4 {
    u8 unk0[8];
    char text[0x18];            /* 0x08: the recruiting text; empty: free */
} Slot6EF4;
typedef struct Room6F14 {
    Slot6EF4 slots[16];
    u8 unk200[8];
    s8 color[16];               /* 0x208: the text colour of each slot */
    u8 unk218[4];
    s8 vis[16][16];             /* 0x21C */
} Room6F14;
extern LevelEntry gLobbyLevel0Entries[];
extern u8 gLobbyLevel0Sel;
s32 Menu_CountPages(s16 n, s16 per);
void AreaBoard_LoadPage(s16 page)
{
    Room6F14 *room = (Room6F14 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20);
    RoomMenu *w = (RoomMenu *)&gMenuWork;
    s16 first = page * 11;
    s16 i;
    u8 no;
    s16 n;

    w->listCount = 0;
    no = gLobbyLevel0Sel;
    i = 0;
    n = 0;
    for (; i < 16; i++) {
        if (room->slots[i].text[0] != 0 && room->vis[(gLobbyLevel0Entries + no - 1)->kind][i] != 0) {
            if (n >= first && n <= (s16)(first + 10)) {
                w->list[w->listCount] = i;
                (w->listCount)++;
            }
            n++;
        }
    }
    w->unkB42 = Menu_CountPages(n, 11);
    if (w->listCursor >= w->listCount) {
        w->listCursor = w->listCount - 1;
    }
    w->unkB40 = page;
}

s32 LobbyStep_630C(s32 v);
s32 LobbyStep_Task8(char *text, u8 *vals);
s32 LobbyStep_UsersLevel1(void);
void LobbyDialog_OpenWait(void *);
char *strcpy(char *dst, const char *src);
s32 LobbyStep_6310(s32 id);
extern u16 gRecruitFrames;
s32 AreaBoard_RecruitInput(u16 pad)
{
    RoomMenu *w = (RoomMenu *)&gMenuWork;
    Room6F14 *room = (Room6F14 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20);

    switch (w->recruitStep) {
    case 1:
    case 2:
    case 3:
    case 7:
        if (--w->unkB68 <= 0) {
            w->recruitStep = 0;
            w->yesNo = 0;
            LobbyText_Open(0x77);
            LobbySe_Play(4);
            memset(w->text, 0, 0x20);
            return 3;
        }
        break;
    }
    switch (w->recruitStep) {
    case 0:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_6310(*(u16 *)&w->slot)) {
        case 0:
            w->recruitStep++;
            w->unkB68 = gRecruitFrames;
            AreaBoard_LoadPage(0);
            WinSlide_Step(&w->win3, 0x280, 0x140);
            break;
        case 1:
            w->recruitStep = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return 1;
        }
        break;
    case 1:
        switch (WinSlide_Step(&w->win3, 0x280, 0x140)) {
        case 0:
            memset(w->text, 0, 0x20);
            w->recruitStep++;
            break;
        }
        break;
    case 2:
        if (pad & 0x20) {
            if (w->yesNo == 0) {
                w->recruitStep = 3;
                w->listCursor = 0;
            } else if (w->text[0] == 0) {
                w->recruitStep = 7;
                LobbyText_Open(0x94);
                LobbyDialog_Grow();
            } else {
                w->recruitStep = 4;
            }
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            w->recruitStep = 0;
            w->yesNo = 0;
            LobbyText_Open(0x77);
            LobbySe_Play(4);
            memset(w->text, 0, 0x20);
            return 3;
        } else {
            w->yesNo = Menu_WrapUpDown(w->yesNo, 2);
        }
        break;
    case 3:
        if (pad & 0x20) {
            if (((s8 *)room->unk218)[(s8)w->list[w->listCursor]] == 0) {
                w->recruitStep = 2;
                memset(w->text, 0, 0x20);
                strcpy(w->text, room->slots[(s8)w->list[w->listCursor]].text);
                LobbyText_Open(0x1D);
                LobbySe_Play(8);
            }
        } else if (pad & 0x40) {
            w->recruitStep = 2;
            LobbySe_Play(4);
        } else if (pad & 0xC00) {
            if (w->unkB42 > 1) {
                w->unkB40 = Menu_WrapLeftRight(w->unkB40, w->unkB42);
                AreaBoard_LoadPage(w->unkB40);
            }
        } else {
            w->listCursor = Menu_WrapUpDown(w->listCursor, w->listCount);
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_630C(room->color[(s8)w->list[w->listCursor]])) {
        case 0:
            w->recruitStep++;
            break;
        case 1:
            w->recruitStep = 0;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return 3;
        }
        break;
    case 5:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_Task8(w->text, 0)) {
        case 0:
            w->recruitStep++;
            break;
        case 1:
            w->recruitStep = 0;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return 3;
        }
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_UsersLevel1()) {
        case 0:
            w->recruitStep = 0;
            return 0;
        case 1:
            gLobbyMenu->step = 0;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return 3;
        }
        break;
    case 7:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            w->recruitStep = 2;
            LobbySe_Play(8);
        }
        break;
    }
    return 2;
}

/* The state of a room entry sequence (the group board keeps one at +0xC80). */
typedef struct RoomJoin {
    /* 0x0 */ s16 idx;              /* in RoomMenu.list */
    /* 0x2 */ u8 unk2[4];
    /* 0x6 */ s8 step;
} RoomJoin;
s16 GroupBoard_JoinStep(RoomJoin *j)
{
    RoomMenu *m = (RoomMenu *)&gMenuWork;
    Room6F14 *room = (Room6F14 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20);

    switch (j->step) {
    case 0:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_630C(room->color[m->list[j->idx]])) {
        case 0:
            j->step++;
            break;
        case 1:
            j->step = 0;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return 1;
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_Task8(room->slots[m->list[j->idx]].text, j->unk2)) {
        case 0:
            j->step++;
            break;
        case 1:
            j->step = 0;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return 1;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_UsersLevel1()) {
        case 0:
            j->step = 0;
            return 0;
        case 1:
            gLobbyMenu->screen = 0;
            gLobbyLevel1Sel = 0;
            LobbyDialog_OpenWait(gLobby->errText);
            return 1;
        }
        break;
    }
    return 2;
}

s32 LobbyStep_6408(void);
s32 LobbyStep_Level1List(s32);
s32 AreaBoard_EnterStep(u16 pad)
{
    switch (gLobby->subStep2) {
    case 0:
        switch (LobbyStep_6408()) {
        case 0:
            gLobby->subStep2++;
            gLobby->seqStep = 0;
            gLobbyLevel1Sel = 0;
            break;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            gLobby->subStep2 = 0;
            return 1;
        }
        break;
    case 1:
        switch (LobbyStep_Level1List(0)) {
        case 0:
            gLobby->subStep2 = 0;
            return 0;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            gLobby->subStep2 = 0;
            return 1;
        }
        break;
    }
    return 2;
}

extern u16 gLobbyLevel1Count;
extern char *gGroupBoardTexts[];
extern char *D_0080C494;
extern char *gRecruitMsgTexts[];
extern char *D_0080C49C;
extern char *gInTownTexts[];
extern char *gLongDashTexts[];
extern char *D_0080C4C4;
extern char *D_0080C4DC;
extern char *D_0080C554;
extern char D_00838208[];
extern char D_00838210[];
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
void Text_SetColor(s16 color);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), AreaBoard_DrawList 213 of 296 words, GroupBoard_DrawRecruitText 107 of 132 words differ */
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void AreaBoard_DrawList(s16 x, s16 y)
{
    RoomMenu *w = (RoomMenu *)&gMenuWork;
    LevelEntry *rec;
    u32 color;
    s16 lx;
    s32 i;
    s32 n;
    s16 ty = y;
    s8 k;

    Font_SetSpacing(0x16, 0x16);
    switch (w->state) {
    case 1:
    case 0:
        break;
    case 2:
        Menu_DrawLabelBar(0x14, 0x12, 0xC8, 0xFFB0B000, gGroupBoardTexts[0], 2, 1);
        Menu_DrawPageNum(x + 0x200, 0x16, w->unkB64, w->unkB66, 1);
        break;
    default:
        if (w->unkB90 == 1) {
            Menu_DrawLabelBar(0x140, 0x12, 0xC8, 0xFFB0B000, D_0080C4C4, 2, 1);
        } else {
            Menu_DrawLabelBar(0x140, 0x12, 0xC8, 0xFFB0B000, D_0080C4DC, 2, 1);
        }
        break;
    }
    Win_Draw(x, y, 0x264, 0xFA, 0xBDB59F6C, 0xB0F04020, 7);
    ty += 2;
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F28);
    lx = x;
    Font_SetPos(lx + 0xA, ty);
    Text_PrintfColored(D_0080C494);
    Font_SetPos(lx + 0x3C, ty);
    Text_PrintfColored(gRecruitMsgTexts[0]);
    Font_SetPos(lx + 0x1CC, ty);
    Text_PrintfColored(D_0080C49C);
    Font_SetPos(lx + 0x21C, ty);
    Text_PrintfColored(gInTownTexts[0]);
    for (i = 0; i < 10; i++) {
        ty += 0x16;
        n = i + w->unkB64 * 10;
        if (n < gLobbyLevel1Count) {
            rec = &gLobbyLevel1Entries[n];
            if (rec->users == 0 || (rec->state != 3 && rec->state != 4)) {
                color = 0x80404040;
            } else {
                color = 0x807F7F7F;
            }
            Font_SetColor(color);
            Font_SetPos(lx + 0xA, ty);
            Text_PrintfColored("%2d", n + 1);
            if (color == 0x807F7F7F) {
                Text_SetColor(rec->color);
            }
            Font_SetPos(lx + 0x3C, ty);
            Text_PrintfColored(D_00838208, rec->text);
            Font_SetColor(color);
            Font_SetPos(lx + 0x1C6, ty);
            Text_PrintfColored(D_00838210, rec->users, rec->maxUsers);
            Font_SetPos(lx + 0x21C, ty);
            Text_PrintfColored("%2d", rec->users - rec->questUsers);
        } else {
            Font_SetColor(0x80404040);
            Font_SetPos(lx + 0xA, ty);
            Text_PrintfColored(gLongDashTexts[0]);
        }
        if (i == w->slotCursor) {
            switch (w->state) {
            case 0:
            case 1:
            case 2:
                k = 0;
                break;
            default:
                k = 1;
                break;
            }
            Menu_DrawLine(x, ty - 1, 0x264, 0x14, k);
        }
    }
    Win_DrawText(0x12C, 0x154, 0x140, 0x50, 0xBDB59F6C, 0xC0201010, D_0080C554);
    switch (w->state) {
    case 0:
    case 1:
    case 2:
        break;
    default:
        return;
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6EF380", D_00838208);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6EF380", D_00838210);

extern u8 D_5C8086[];
extern char *D_0080C4A4;
extern char *gHrLabelTexts[];
extern char *D_0080C4B4;
u32 LevelList_SlotColor(u8 slot);
void LobbyUi_DrawIcon(s16 x, s16 y, s16 size, u32 color, s8 kind);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 a, s16 b);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 color, u32 color2);
s32 sprintf(char *buf, const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
struct EquipItem;
/* GroupBoard_DrawMembers casts the value to s32 before Icon_DrawKind48's u8 parameter: MWCC keeps the game's andi that way */
u8 Equip_GetClass(struct EquipItem *e);
void Icon_DrawKind48(s16, s16, s32, s32, u8, s32); /* unit's own declaration: with the definition's void (s32 a, s32 b, s32 c, s32 d, s16 e, s8 f) (lobby/unk_78F070.c), GroupBoard_DrawMembers 51 of 226 words differ */
void GroupBoard_DrawMemberMenu(void *w, s16 x, s16 y, u32 color);
void GroupBoard_DrawMembers(s16 x, s16 y, s32 n, WinShake *p, u16 a, s8 show)
{
    RoomMenu *w = (RoomMenu *)gMenuWork;
    u8 *e;
    u8 *h;
    s16 ty = y;
    s16 i;
    char buf[16];
    char wide[16];

    Icon_Draw(x, y, 0x12C, 0x10E, 0xFFE0D8C8, 1);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 0xDC, y + 6, w->page, w->pages, 1);
    LobbyUi_DrawIcon(x + 0x96, y + 0x87, 0x70, LevelList_SlotColor(D_5C8086[gLobbyLevel0Sel * 0x164]), 3);
    Font_SetColor(0x80301010);
    Font_SetPos(x + 0x28, y + 0x14);
    Text_PrintfColored(D_00838208, gLobbyLevel1Entries[n - 1].name);
    ty += 0x2A;
    Font_SetPos(x + 0x28, ty);
    Text_PrintfColored(D_0080C4A4);
    Font_SetPos(x + 0xBE, ty);
    Text_PrintfColored(gHrLabelTexts[0]);
    Font_SetPos(x + 0xF0, ty);
    Text_PrintfColored(D_0080C4B4);
    Font_SetColor(0x80101010);
    for (i = 0; i < 5; i++) {
        ty += 0x16;
        e = (u8 *)w + 8 + (i + w->page * 5) * 0x11C;
        h = e + 0x1C;
        if (*(s8 *)e != 0) {
            if (w->unkB28 == 1 && i == w->cursor) {
                Menu_DrawCursorRow(x + 0x28, ty, 0xBE, 0x14, 0xC0106060, 0xFF106060);
            }
            Font_SetPos(x + 0x28, ty);
            Text_PrintfColored(D_00838208, e + 8);
            Font_SetPos(x + 0xAC, ty);
            sprintf(buf, "%3d", *(u16 *)h);
            Sjis_Widen(buf, wide);
            Text_PrintfColored(wide);
            Icon_DrawKind48(x + 0xF6, ty, 0x18, -1, (s32)Equip_GetClass((struct EquipItem *)(h + 0x74)), 0);
        }
    }
    GroupBoard_DrawMemberMenu(w, x + 0x28, ty + 0x24, 0x80101010);
    if (show) {
        Menu_DrawStamp(x, y, 0x12C, 0x10E, a, p->timer);
    }
}

void AreaBoard_DrawRecruitBox(s16 x, s16 y)
{
    RoomMenu *w = (RoomMenu *)&gMenuWork;
    s16 d;

    if (w->recruitStep == 2) {
        d = (gFrameTick & 0x10) ? 0 : 6;
        switch (w->yesNo) {
        case 0:
            Icon_Draw(x + 10 - d, y + 0x7C, 0x18, 0x18, 0xFF106060, 0x29);
            Icon_Draw(x + 0x10A + d, y + 0x7C, 0x18, 0x18, 0xFF106060, 0x28);
            break;
        case 1:
            Icon_Draw(x + 100 - d, y + 0xBE, 0x18, 0x18, 0xFF106060, 0x29);
            Icon_Draw(x + 0xB0 + d, y + 0xBE, 0x18, 0x18, 0xFF106060, 0x28);
            break;
        }
    }
}

extern char *gRecruitMsgColonTexts[];
extern char *D_0080C4CC;
extern char *gTimeLeftSecTexts[];
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
void Text_Printf(const char *fmt, ...);
void AreaBoard_DrawRecruit(s16 x, s16 y)
{
    RoomMenu *w = (RoomMenu *)gMenuWork;
    LevelEntry *rec = &gLobbyLevel1Entries[w->slot - 1];
    s16 ty = y;
    u32 color = w->yesNo == 0 ? 0xFF106060 : 0xFF606060;
    s32 ly;
    s32 lx;
    u32 c;
    s32 cx;
    s32 tx;

    Icon_Draw(x, y, 0x12C, 0x10E, 0xFFE0D8C8, 1);
    c = LevelList_SlotColor(D_5C8086[gLobbyLevel0Sel * 0x164]);
    ly = ty;
    lx = x;
    cx = lx + 0x96;
    LobbyUi_DrawIcon(cx, ly + 0x87, 0x70, c, 3);
    Font_SetColor(0x80301010);
    Font_SetPos(lx + 0x28, ly + 0x14);
    Text_PrintfColored(D_00838208, rec->name);
    ty += 0x2A;
    tx = lx + 0x28;
    Font_SetPos(tx, ty += 0x32);
    Text_PrintfColored(D_00838208, gRecruitMsgColonTexts[0]);
    Gfx_DrawRectFrame(tx, ty += 0x20, 0xDC, 0x18, color);
    Font_SetColor(0x80101010);
    Font_SetPos(Text_CenterX(w->text, cx, 0x12), ty + 3);
    Text_PrintfColored(D_00838208, w->text);
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(D_0080C4CC, cx, 0x12), ty + 0x46);
    Text_PrintfColored(D_00838208, D_0080C4CC);
    AreaBoard_DrawRecruitBox(x, y);
    if (w->state != 7) {
        Font_SetPos(lx + 0x9C, ly + 0xEC);
        Text_Printf(gTimeLeftSecTexts[0], w->unkB68 / 30);
    }
    if (w->unkB91 != 0) {
        Menu_DrawStamp(x, y, 0x12C, 0x10E, w->unkB6E, w->shake.timer);
    }
}


typedef struct NameTbl {
    struct {
        u8 pad0[8];
        char name[24];
    } ent[16];
    u8 pad200[8];
    s8 color[16];               /* 0x208 */
} NameTbl;
typedef struct MemberView {
    u8 pad0[0xB2C];
    s8 idx[0x14];               /* 0xB2C */
    s16 page;                   /* 0xB40 */
    s16 pages;                  /* 0xB42 */
} MemberView;

void GroupBoard_DrawRecruitText(s16 x, s16 y, s16 cur, s16 n)
{
    MemberView *w = (MemberView *)gMenuWork;
    NameTbl *names = (NameTbl *)((u8 *)(*(s32 *)&gLobbyWorkBuf + 0x6EF4) + 0x20);
    s32 i;
    s16 ty = y;
    s8 *p;

    Win_Draw(x, y, 0x118, 0x122, 0xBDB59F6C, 0xC0101010, 2);
    ty += 10;
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 0x28, ty);
    Text_PrintfColored(gRecruitMsgTexts[0]);
    Font_SetColor(0x807F7F7F);
    Menu_DrawPageNum((s16)(x + 0x28) + 0xA0, ty, w->page, w->pages, 1);
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < n; i++) {
        if (names->ent[w->idx[i]].name[0] != 0) {
            p = &w->idx[i];
            ty += 0x16;
            Font_SetPos(x + 0x28, ty);
            Text_SetColor(names->color[*p]);
            Text_PrintfColored(D_00838208, names->ent[*p].name);
            if (cur == i) {
                Menu_DrawLine(x + 10, ty - 1, 0x104, 0x14, 0);
            }
        }
    }
}
