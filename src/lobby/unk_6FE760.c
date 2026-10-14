#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "save.h"
#include "quest.h"
#include "lobby_menu.h"

/*
 * The quest menu of this file (work area gMenuWork, updated by
 * ElderForecast_Update, drawn by ElderForecast_Draw): a window of three rows; row 0
 * shows five quests of one of the lists of gTownQuestLists (ElderForecast_Load),
 * row 2 the downloaded quests (up to 3, Quest_DownloadedInfo), three pages each.
 */
typedef struct QuestBoard {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ s32 list;                /* which list of TownQuestLists */
    /* 0x0C */ s32 unkC;
    /* 0x10 */ u16 shown[5];            /* ElderForecast_Load */
    /* 0x1A */ u16 ids[3];              /* the downloaded quests */
    /* 0x20 */ s16 count;
    /* 0x22 */ s16 sel;                 /* the quest shown */
    /* 0x24 */ s16 page;
    /* 0x26 */ s16 pages;
    /* 0x28 */ s16 cursor;              /* the row of the first window */
    /* 0x2A */ s8 mode;                 /* 0 rows, 1 list, 2 message, 3 downloaded quests, 4 leaving */
    /* 0x2B */ s8 step;
} QuestBoard;
void LobbySe_Play(s32);
s32 WinSlide_Step(WinSlide *, s16 from, s16 to);
extern u16 gElderForecastQuests[5][3][2];
extern u8 gElderRaidEvent;
void *memset(void *dst, s32 c, u32 n);
void *Quest_DownloadedInfo(u8 slot);
s32 ElderForecast_IsUnlocked(void);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Cond_Check(u16 id);
void *memcpy(void *dst, const void *src, u32 n);
void ElderForecast_RollList(s32 k, u16 *out);
s32 Menu_WrapUpDownSe(s32 cur, s32 count, s32 step);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);
extern s16 gElderMonstersOnline[];
extern s16 gElderMonstersOffline[];
s32 Game_IsOnline(void);
QuestInfo *Quest_GetDiscInfo(u16 id);
extern u16 gTalkLine;
extern char *gTalkLineText;
void Talk_Say(s32 a, s8 b);
char *Talk_LineText(u16 id);
char *Text_FillDollar(char *fmt, s32 n);
char *Text_FillDollar2(s32 n);
u16 Rand_Next(s32 stream);
extern u8 gMenuWork;
extern char *gElderDragonInfoTexts[];
void Text_SetSize(s32);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
void ElderForecast_DrawPanel(u16 *ids, s16 x, s16 y, s32 list, s32 b);
void ElderForecast_DrawQuest(QuestBoard *m, s16 x, s16 y);

void ElderForecast_ListQuests(QuestBoard *m)
{
    s32 i;
    u8 *p;

    memset(m->ids, 0, 6);
    for (i = 0; i < gLobby->dlQuestCount; i++) {
        p = Quest_DownloadedInfo(i);
        if (p != NULL) {
            m->ids[i] = *(u16 *)(p + 0x2A);
        }
    }
    m->count = gLobby->dlQuestCount;
    m->sel = 0;
    m->page = 0;
    m->pages = 3;
}

u16 LobbyPad_Get(s32);
void Chat_SetWindowMode(s8 a, s8 b);
void CamFocus_SetAside(s8 a);
s32 Talk_Update(void);
void ElderForecast_Close(void);
s32 ElderForecast_ListInput(QuestBoard *m, u16 pad);
void ElderForecast_Load(u16 *out, s32 k);
s32 ElderForecast_ViewInput(QuestBoard *m, u16 pad);
void ElderForecast_SayRaid(void);
s32 ElderForecast_MenuInput(QuestBoard *m, u16 pad);

void ElderForecast_Update(void)
{
    QuestBoard *m = (QuestBoard *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (m->mode) {
    case 0:
        switch (ElderForecast_MenuInput(m, pad)) {
        case 0:
            switch (m->cursor) {
            case 0:
                m->mode = 1;
                ElderForecast_Load(m->shown, m->list);
                WinSlide_Step(&m->win, 0x280, 0x140);
                ((u8 *)&gTownQuestListsCopy)[0x100] = 1;
                CamFocus_SetAside(1);
                break;
            case 1:
                m->mode = 2;
                ElderForecast_SayRaid();
                break;
            case 2:
                m->mode = 3;
                WinSlide_Step(&m->win, 0x280, 0x118);
                ElderForecast_ListQuests(m);
                CamFocus_SetAside(1);
                break;
            }
            break;
        case 3:
            m->mode = 4;
            Talk_Say(1, 0);
            break;
        }
        break;
    case 3:
        switch (ElderForecast_ListInput(m, pad)) {
        case 0:
        case 3:
            m->mode = 0;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case 1:
        switch (ElderForecast_ViewInput(m, pad)) {
        case 0:
        case 3:
            m->mode = 0;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case 2:
        switch (Talk_Update()) {
        case 0:
            m->mode = 0;
            break;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
            ElderForecast_Close();
            break;
        }
        break;
    }
}

s32 ElderForecast_ListInput(QuestBoard *m, u16 pad)
{
    switch (m->step) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->step++;
            break;
        }
        break;
    case 1:
        if (pad & 0x40) {
            m->step = 0;
            LobbySe_Play(4);
            return 3;
        }
        if (pad & 0x3000) {
            m->sel = Menu_WrapUpDownSe(m->sel, m->count, 0x17);
        } else {
            m->page = Menu_WrapLeftRight(m->page, m->pages);
        }
        break;
    }
    return 2;
}

void ElderForecast_Load(u16 *out, s32 k)
{
    TownQuestLists *a = (TownQuestLists *)&gTownQuestLists;
    s32 i;
    s32 j;
    u16 id;
    s16 *tbl;
    QuestInfo *q;

    if (Game_IsOnline() == 1) {
        tbl = gElderMonstersOnline;
    } else {
        tbl = gElderMonstersOffline;
    }
    memset(out, 0, 10);
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 5; j++) {
            if (Game_IsOnline() == 1 && gSave.hr >= 0x29) {
                id = a->quests[1][k][j];
            } else {
                id = a->quests[0][k][j];
            }
            if (id != 0) {
                q = Quest_GetDiscInfo(id);
                if (q->goals[0].target == tbl[i]) {
                    out[i] = id;
                }
            }
        }
    }
}

s32 ElderForecast_ViewInput(QuestBoard *m, u16 pad)
{
    switch (m->step) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x140)) {
        case 0:
            m->step++;
            break;
        }
        break;
    case 1:
        if (pad & 0x40) {
            m->step = 0;
            LobbySe_Play(4);
            return 3;
        }
        break;
    }
    return 2;
}

void ElderForecast_SayRaid(void)
{
    u8 *e = (u8 *)&gElderRaidEvent;

    Talk_Say(2, 0);
    Text_FillDollar(Talk_LineText(gTalkLine), *(s8 *)(e + 0x2A));
    gTalkLineText = Text_FillDollar2(*(s8 *)(e + 0x29));
}

s32 ElderForecast_MenuInput(QuestBoard *m, u16 pad)
{
    u8 ok;

    if (pad & 0x20) {
        ok = 0;
        switch (m->cursor) {
        case 0:
            if (ElderForecast_IsUnlocked() == 0) {
                ok = 1;
                LobbySe_Play(9);
            } else {
                LobbySe_Play(0x17);
            }
            break;
        case 1:
            LobbySe_Play(8);
            break;
        case 2:
            if (gLobby->dlQuestsReady == 0) {
                ok = 1;
                LobbySe_Play(9);
            } else {
                LobbySe_Play(0x17);
            }
            break;
        }
        if (ok == 0) {
            return 0;
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else {
        m->cursor = Menu_WrapUpDown(m->cursor, 3);
    }
    return 2;
}

extern s32 gTownMode;
extern char *gSeasonNames[];
extern char *gMonsterNames[];
extern char *gStageNames[];
extern char D_00838460[];
extern char D_00838468[];
extern char D_00838470[];
extern char *D_0080C14C;
extern char *D_0080C2EC;
extern char *D_0080C074;
extern char *D_0080C054;
extern char *D_0080C2FC;
extern char *D_0080C2F4;
extern char *gNightTexts[];
extern char *gTimeOfDayTexts[];
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Text_PrintfColored(char *fmt, ...);
s32 sprintf(char *buf, char *fmt, ...);
void Font_SetPos(s16, s16);
void Font_SetColor(s32 color);
void Font_SetSpacing(u8 a, u8 b);
s32 Monster_IsAvailable(u16 arg0);
u16 Area_MapGroup(u16 a);

/* The monster list panel of the quest board: the list's title, then one row
 * per monster (Game_IsOnline picks the table) with its quest's state.
 * The frame's x steps (sx += 10, += 0x100, then sx = x) keep sx a variable. */
void ElderForecast_DrawPanel(u16 *ids, s16 x, s16 y, s32 list, s32 b)
{
    QuestInfo *info;
    s16 i;
    s16 cy;
    char buf[0x40];
    s16 ly;
    s16 *mon;
    s16 sx;
    s16 n;
    s32 k;

    sx = x;
    cy = y;
    if (gTownMode == 0x44) {
        Icon_Draw(x, y, 0x12C, 0x154, 0xFFE0D8C8, 0);
        ly = cy + 0xAA;
        sx += 10;
        Icon_Draw(sx, ly - 0x78, 0x100, 0x20, 0x80202020, 0x1B);
        Icon_Draw(sx, ly + 0x78, 0x100, 0x20, 0x80202020, 0x1B);
        sx += 0x100;
        Icon_Draw(sx, ly - 0x78, 0x2E, 0x20, 0x80202020, 0x1C);
        Icon_Draw(sx, ly + 0x78, 0x2E, 0x20, 0x80202020, 0x1C);
        sx = x;
    }
    Font_SetColor(0x80301010);
    Font_SetSpacing(0x14, 0x14);
    cy += 0x10;
    Font_SetColor(0x80301010);
    sprintf(buf, D_00838460, gSeasonNames[list]);
    Font_SetPos(Text_CenterX(buf, x + 0xA0, 0x14) - 0x14, cy);
    Text_PrintfColored(D_0080C14C, buf);
    Font_SetPos(Text_CenterX(D_0080C2EC, x + 0x96, 0x14), cy += 0x16);
    Text_PrintfColored(D_0080C2EC);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    if (Game_IsOnline() == 1) {
        if (Cond_Check(0x67) == 0) {
            n = 4;
        } else {
            n = 5;
        }
        mon = gElderMonstersOnline;
    } else {
        mon = gElderMonstersOffline;
        n = 0;
        for (i = 0; i < 5; i++) {
            if (Monster_IsAvailable(mon[i]) == 1) {
                n++;
            }
        }
    }
    sx += 0x28;
    for (i = 0; i < n; i++) {
        Font_SetPos(sx, cy + 0x1E);
        Text_PrintfColored(D_00838468, gMonsterNames[*mon], D_0080C074);
        Font_SetPos(sx + 0x28, cy += 0x34);
        if (*ids != 0) {
            info = Quest_GetDiscInfo(*ids);
            k = Area_MapGroup(info->map);
            if ((info->flags & 0x80000) && (info->flags & 0x100000)) {
                Text_PrintfColored(D_00838470, gStageNames[k], D_0080C054, D_0080C2FC);
            } else if (info->flags & 0x80000) {
                Text_PrintfColored(D_00838470, gStageNames[k], D_0080C054, D_0080C2F4);
            } else {
                Text_PrintfColored(D_00838470, gStageNames[k], D_0080C054, gNightTexts[0]);
            }
        } else {
            Text_PrintfColored(gTimeOfDayTexts[0]);
        }
        mon++;
        ids++;
    }
}

s32 Save_TestListBit(u16 id);
u16 Area_MapGroup(u16 a);
extern s8 gMapGroupMapIds[][2];
void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 kind, s32 b);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Quest_DrawOverview(s16, s16, QuestInfo *);
void Quest_DrawTerms(s16, s16, QuestInfo *);
void Quest_DrawDetails(s16, s16, QuestInfo *);
void Quest_DrawClearMark(s16, s16);
void Font_SetColor(s32 color);
void Font_SetSpacing(u8 a, u8 b);
static inline void func_006FF330_page(QuestBoard *m, s16 x, s16 y, QuestInfo *q)
{
    switch (m->page) {
    case 0:
        Quest_DrawOverview(x - 0x14, y, q);
        break;
    case 1:
        Quest_DrawTerms(x - 0x14, y, q);
        break;
    case 2:
        Quest_DrawDetails(x - 0x14, y, q);
        break;
    }
    if (Save_TestListBit(q->id)) {
        Quest_DrawClearMark(x, y);
    }
}

void ElderForecast_DrawQuest(QuestBoard *m, s16 x, s16 y)
{
    QuestInfo *q = Quest_GetDiscInfo(m->ids[m->sel]);
    s16 marks[4];
    s32 i;
    s8 k;

    memset(marks, 0, 8);
    for (i = 0; i < 3; i++) {
        if (m->ids[i] == 0) {
            break;
        }
        marks[i] = Area_MapGroup(Quest_GetDiscInfo(m->ids[i])->map);
        if (i == m->sel) {
            marks[i] = -marks[i];
        }
    }
    k = gMapGroupMapIds[Area_MapGroup(q->map)][0];
    Menu_DrawQuestSheet(x - 0x14, y - 0xE, 0x140, 0x168, marks, m->sel, k, m->list);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 0xD2, y += 5, m->page, 3, 1);
    Font_SetSpacing(0x12, 0x12);
    func_006FF330_page(m, x + 0x28, y, q);
}

void ElderForecast_Draw(u8 *o)
{
    QuestBoard *m = (QuestBoard *)&gMenuWork;

    Text_SetSize(*(s32 *)(o + 0x18));
    switch (m->mode) {
    case 0:
        Menu_DrawLabelList(0x1C2, gElderDragonInfoTexts, 3, m->cursor, 0xFFB0B000);
        break;
    case 1:
        ElderForecast_DrawPanel(m->shown, m->win.pos, 0x3A, m->list, m->unkC);
        break;
    case 3:
        ElderForecast_DrawQuest(m, m->win.pos, 0x3A);
        break;
    }
}

void ElderForecast_RollList(s32 k, u16 *out)
{
    s32 i;
    u16 *e;
    s32 r;

    if (k < 0 || k > 2) {
        return;
    }
    for (i = 0; i < 5; i++) {
        e = gElderForecastQuests[i][k];
        r = Rand_Next(1) % 100;
        if (e[1] == 0) {
            if (r < 60) {
                out[i] = e[0];
            } else {
                out[i] = 0;
            }
        } else if (r < 40) {
            out[i] = e[0];
        } else if (r < 80) {
            out[i] = e[1];
        } else {
            out[i] = 0;
        }
    }
}

s32 ElderForecast_HasQuest(u16 id)
{
    s16 i;
    s16 j;
    s16 k;

    for (i = 0; i < 5; i++) {
        for (j = 0; j < 3; j++) {
            for (k = 0; k < 2; k++) {
                if (id == gElderForecastQuests[i][j][k]) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void ElderForecast_Roll(void)
{
    TownQuestLists *a = (TownQuestLists *)&gTownQuestLists;
    TownQuestLists *b = (TownQuestLists *)&gTownQuestListsCopy;
    s32 i;

    if (Cond_Check(0x15) != 0) {
        memset(a->quests, 0, sizeof(a->quests));
        for (i = 0; i < 3; i++) {
            ElderForecast_RollList(i, a->quests[0][i]);
        }
        memcpy(b->quests, a->quests, sizeof(a->quests));
    }
}

void ElderRaid_SetState(void)
{
    u8 *e = (u8 *)&gElderRaidEvent;

    if (*(s8 *)(e + 0x28) != 1) {
        gLobby->elderRaidState = 1;
    } else {
        gLobby->elderRaidState = 2;
    }
    if (*(s16 *)(e + 0x20) != gSave.unk3696 && gLobby->elderRaidState == 1) {
        gLobby->movieState = 1;
    }
}

/* unk_6FE760's .data (0x7EC178-0x7EC1E0) */
s16 gElderMonstersOnline[6] = {
#include "lobby/007EC178.inc"
};
s16 gElderMonstersOffline[6] = {
#include "lobby/007EC188.inc"
};
u16 gElderForecastQuests[5][3][2] = {
#include "lobby/007EC1A0.inc"
};

/* unk_6FE760's .rodata (0x838460-0x838490) */
#pragma rodata on
char D_00838460[0x8] = {
#include "lobby/00838460.inc"
};
char D_00838468[0x8] = {
#include "lobby/00838468.inc"
};
char D_00838470[0x20] = {
#include "lobby/00838470.inc"
};
#pragma rodata off
