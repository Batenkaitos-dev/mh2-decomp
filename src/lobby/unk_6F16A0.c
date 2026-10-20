#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "save.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "quest.h"             /* TeamEntry: gTeamItemBits, gMonsterNoteMarks */
#include "lobby_labels.h"
#pragma optimization_level 4

typedef struct ArenaMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ WinSlide win2;
    /* 0x10 */ s32 values[7];
    /* 0x2C */ s32 value;
    /* 0x30 */ s16 unk30;
    /* 0x32 */ s16 page;
    /* 0x34 */ s16 pages;
    /* 0x36 */ s16 count;
    /* 0x38 */ s8 unk38;
    /* 0x39 */ s8 record;
    /* 0x3A */ s8 cursor;
    /* 0x3B */ s8 used;             /* records of gSave.arenaEms in use */
    /* 0x3C */ s8 toggle;
} ArenaMenu;
void TownMsg_SendArenaTeams(void);
void TownMsg_SendStatus(void);
void LobbyText_Open(s32);
extern s8 gQuestMinMembers;
extern s32 gTownMode;
extern u8 gMenuWork;
void Arena_DrawMonsterList(s32, s32, s32, s32); /* unit's own declaration: with the definition's void (s16 x, s16 y, s8 rec, s8 mode) (lobby/arena_cup.c), ArenaMenu_DrawRecord 10 of 22 words differ */
void TownMsg_SendArenaTeam(void *, s32);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern LobbyMenu *gLobbyMenu;
void LobbySe_Play(s32 se);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);
void ArenaFeed_LoadPage(s32 page);
void Text_SetSize(s32);
void Font_SetSpacing(u8 a, u8 b);
void Arena_DrawMonsterInfo(s16 x, s16 y, ArenaEm *r);
void *memset(void *d, s32 c, u32 n);

void Town_CheckQuestParty(void)
{
    if (gLobby->roomMembers < gQuestMinMembers) {
        LobbyText_Open(0x6B);
        gTownMode = 0x16;
        return;
    }
    if (D_6ADADE != 0 && D_6ADAFA != 0) {
        TownMsg_SendArenaTeams();
        TownMsg_SendStatus();
        gTownMode = 0x42;
    }
}

extern s32 gTownMenuOpen;
void LobbyMenu_Init(void);
void ArenaMenu_Draw(void *task);
void ArenaMenu_Draw2(u8 *o);
s32 WinSlide_Step(WinSlide *win, s16 from, s16 to);
void ArenaMenu_Open(void)
{
    ArenaMenu *m;
    s32 i;

    m = (ArenaMenu *)&gMenuWork;
    gTownMenuOpen = 1;
    gTownMode = 0x41;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(ArenaMenu_Draw, ArenaMenu_Draw2, 0, 0);
    m->unk30 = 0x708;
    WinSlide_Step(&m->win, 0x280, 0x104);
    for (i = 0; i < 3; i++) {
        if (gSave.arenaEms[i].emKind != 0) {
            m->used++;
        }
    }
    LobbySe_Play(0x17);
}

void ArenaMenu_Send(void)
{
    ArenaMenu *m = (ArenaMenu *)&gMenuWork;

    gTownMode = 0x40;
    if (m->value == 0) {
        m->value = 0;
    }
    D_6ADB14 = m->record;
    TownMsg_SendArenaTeam(&gSave.arenaEms[m->record], m->value);
    LobbyText_Open(0x8E);
    LobbyMenu_SetDraw(0, 0, 0, 0);
    gFieldControl = 0;
    *(s16 *)((u8 *)gLobbyMenu + 0x24) = 0x708;
}

u16 LobbyPad_Get(s32);
void Chat_SetWindowMode(s8 a, s8 b);
void ArenaMenu_Send(void);
void ArenaFeed_Open(void);
s32 ArenaMenu_ListInput(u16 pad);

void ArenaMenu_Update(void)
{
    ArenaMenu *m = (ArenaMenu *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    if (--m->unk30 <= 0) {
        ArenaMenu_Send();
    }
    switch (m->unk38) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x104)) {
        case 0:
            m->unk38++;
        }
        break;
    case 1:
        if (pad & 0x20) {
            m->unk38 = 5;
            ArenaFeed_Open();
            WinSlide_Step(&m->win2, 0x280, 0x10A);
            LobbySe_Play(8);
        } else if (pad & 0x200) {
            m->unk38 = 2;
            WinSlide_Step(&m->win2, 0x280, 0x11C);
            LobbySe_Play(0x17);
        } else {
            m->record = Menu_WrapUpDown(m->record, m->used);
        }
        break;
    case 2:
        switch (WinSlide_Step(&m->win2, 0x280, 0x11C)) {
        case 0:
            m->unk38 = 4;
        }
        break;
    case 3:
        switch (WinSlide_Step(&m->win2, 0x11C, 0x280)) {
        case 0:
            m->unk38 = 1;
        }
        break;
    case 4:
        if (pad & 0x40) {
            m->unk38 = 3;
            WinSlide_Step(&m->win2, 0x11C, 0x280);
            LobbySe_Play(4);
        }
        break;
    case 5:
        switch (WinSlide_Step(&m->win2, 0x280, 0x10A)) {
        case 0:
            m->unk38++;
        }
        break;
    case 6:
        switch (ArenaMenu_ListInput(pad)) {
        case 0:
            ArenaMenu_Send();
            break;
        case 3:
            m->unk38 = 0;
            m->value = 0;
            WinSlide_Step(&m->win, 0x280, 0x104);
            break;
        }
        break;
    }
}

s32 ArenaMenu_ListInput(u16 pad)
{
    ArenaMenu *m = (ArenaMenu *)&gMenuWork;

    if (pad & 0x20) {
        m->value = m->values[m->cursor];
        LobbySe_Play(8);
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    if (pad & 0x200) {
        m->toggle ^= 1;
        LobbySe_Play(0x39);
    } else if (pad & 0xC00) {
        if (m->pages != 1) {
            m->page = Menu_WrapLeftRight(m->page, m->pages);
            ArenaFeed_LoadPage(m->page);
            m->cursor = 0;
        }
    } else {
        m->cursor = Menu_WrapUpDown(m->cursor, m->count);
    }
    return 2;
}

extern TeamEntry gTeamItemBits[];
extern TeamEntry *gMonsterNoteMarks[];
extern u16 gMonsterNoteIds[];
s32 SavePouch_Count(u16 item);
s32 Menu_CountPages(s16 n, s16 per);
void ArenaFeed_Open(void)
{
    ArenaMenu *m = (ArenaMenu *)&gMenuWork;
    s32 em;
    s32 n = 0;
    s32 i = 0;
    s32 k = 0xFF;
    TeamEntry *p;
    s32 key;
    u16 *q;
    s32 j;

    p = gTeamItemBits;
    em = gSave.arenaEms[m->record].emKind;

    for (; i < 0x399; i++, p++) {
        key = p->key;
        if (key == 0) {
            break;
        }
        if (SavePouch_Count(key) != 0) {
            n++;
        }
    }
    for (j = 0, q = gMonsterNoteIds; j < 0x49; j++, q++) {
        if (*q == 0) {
            break;
        }
        if (em == *q) {
            k = j;
            break;
        }
    }
    if (k != 0xFF) {
        p = gMonsterNoteMarks[k];
        key = p->key;
        while (key != 0) {
            if (SavePouch_Count(key) != 0) {
                n++;
            }
            p++;
            key = p->key;
        }
    }
    m->pages = Menu_CountPages(n, 7);
    m->page = 0;
    m->cursor = 0;
    ArenaFeed_LoadPage(0);
}

void ArenaFeed_LoadPage(s32 page)
{
    ArenaMenu *m = (ArenaMenu *)&gMenuWork;
    s32 em;
    s32 k;
    TeamEntry *p;
    s32 last;
    s32 n = 0;
    s32 key;
    s32 i;
    TeamEntry *r;
    s32 first;
    s32 count;
    u16 *q;
    s32 j;
    u16 v;

    em = gSave.arenaEms[m->record].emKind;
    k = 0xFF;
    memset(m->values, 0, sizeof(m->values));
    first = page * 7;
    count = 0;
    i = 0;
    last = first + 6;
    for (p = gTeamItemBits; i < 0x399; i++, p++) {
        key = p->key;
        if (key == 0) {
            break;
        }
        if (SavePouch_Count(key) != 0) {
            if (n >= first && n <= last) {
                m->values[count] = key;
                count++;
            }
            n++;
        }
    }
    for (j = 0, q = gMonsterNoteIds; j < 0x49; j++, q++) {
        if (*q == 0) {
            break;
        }
        if (em == *q) {
            k = j;
            break;
        }
    }
    if (k != 0xFF) {
        r = gMonsterNoteMarks[k];
        v = r->key;
        while (v != 0) {
            key = v;
            if (SavePouch_Count(key) != 0) {
                if (n >= first && n <= last) {
                    m->values[count] = key;
                    count++;
                }
                n++;
            }
            r++;
            v = r->key;
        }
    }
    m->count = count;
}

void ArenaMenu_DrawRecord(s32 a, s32 b)
{
    ArenaMenu *m = (ArenaMenu *)&gMenuWork;
    s32 k;

    switch (m->unk38) {
    case 0:
    case 1:
        k = 1;
        break;
    default:
        k = 2;
        break;
    }
    Arena_DrawMonsterList(a, b, m->record, k);
}

void LobbyDialog_DrawWin(s16 arg0, s16 arg1, s16 arg2, s16 arg3, s32 arg4);
void Menu_DrawPageArrows(s16 x, s16 y, s16 page, s16 n, u32 color);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 a3, s32 item, u16 t1, s8 t2, u32 color);
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
void Text_PrintfColored(const char *fmt, ...);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
extern char *gItemNames[];
void ArenaFeed_DrawList(s16 x, s16 y)
{
    ArenaMenu *m = (ArenaMenu *)&gMenuWork;
    s32 item;
    s16 i;
    s16 ty = y;
    s32 sel;

    LobbyDialog_DrawWin(x, y, 0x15A, 0x120, 0x80FF2020);
    if (m->pages > 1) {
        Menu_DrawPageArrows(x + 0x104, ty + 10, m->page, m->pages, 0xBDB59F6C);
    }
    Font_SetColor(0x807F7F7F);
    ty += 0x1E;
    Font_SetSpacing(0x14, 0x14);
    if (m->count == 0) {
        Font_SetPos(Text_CenterX(gArenaFeedTexts[ARENA_FEED_NO_FEED], x + 0xAD, 0x14), y + 0x86);
        Text_PrintfColored(gArenaFeedTexts[ARENA_FEED_NO_FEED]);
        return;
    }
    i = 0;
    do {
        Font_SetPos(x + 0x3C, ty);
        item = m->values[i];
        if (item != 0) {
            Text_PrintfColored("%-18s%s%2d", gItemNames[item], gLobbyWords[LOBBY_WORD_TIMES], SavePouch_Count(item));
            Item_DrawIcon((s16)(x + 0x3C) - 0x24, ty - 2, 0x14, item);
            if (i == m->cursor) {
                sel = item;
                Menu_DrawGlowBar((s16)(x + 0x3C) + 0x85, ty - 1, 0x15A, 0x18, 0xB0008000);
            }
        }
        ty += 0x16;
        i++;
    } while (i < 7);
    ItemInfo_Draw((s16)(x + 0x3C) - 0x32, ty + 4, 0x15A, m->toggle, sel, 0, -1, 0x807F7F7F);
}

void Text_Printf(char *fmt, ...);
void ArenaFeed_DrawList(s16 x, s16 y);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *);
void Font_SetPos(s16 x, s16 y);
void ArenaMenu_Draw(void *task)
{
    ArenaMenu *m = (ArenaMenu *)&gMenuWork;
    s32 sec = m->unk30 / 30;

    Text_SetSize(*(s32 *)((u8 *)task + 0x18));
    Font_SetSpacing(0x12, 0x12);
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x104, 0x12, 0x15E, 0xFFB0B000, gArenaCupTexts[ARENA_TEXT_PREPARE], 2, 1);
    Font_SetSpacing(0x14, 0x14);
    Font_SetPos(0x1E0, 0x14);
    Text_Printf(gArenaCupTexts[ARENA_TEXT_TIME_LEFT], sec);
    switch (m->unk38) {
    case 5:
    case 6:
        ArenaFeed_DrawList(m->win2.pos, 0x3A);
        break;
    default:
        ArenaMenu_DrawRecord(m->win.pos, 0x3A);
        break;
    }
    switch (m->unk38) {
    case 0:
    case 1:
        Win_DrawText(0x104, 0x160, 0x168, 0x50, 0xBDB59F6C, 0x70401010, gArenaCupTexts[ARENA_TEXT_SECONDS]);
        break;
    case 5:
    case 6:
        Win_DrawText(0x104, 0x160, 0x168, 0x50, 0xBDB59F6C, 0x70401010, gArenaFeedTexts[ARENA_FEED_TITLE]);
        break;
    }
}

void ArenaMenu_Draw2(u8 *o)
{
    ArenaMenu *m = (ArenaMenu *)&gMenuWork;

    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    switch (m->unk38) {
    case 2:
    case 3:
    case 4:
        Arena_DrawMonsterInfo(m->win2.pos, 0x4E, &gSave.arenaEms[m->record]);
        break;
    }
}
