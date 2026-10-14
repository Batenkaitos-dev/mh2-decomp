#include "common.h"
#include "lobby_text.h"
#include "scene.h"
#include "save.h"
#include "item.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "quest.h"
void Font_SetPos(s16, s16);
void Font_SetSpacing(u8 a, u8 b);

void Text_PrintfColored(const char *, ...);
void QuestCounter_DrawQuestPage(s16 x, s16 y);
extern u8 gMenuWork;
void Text_SetSize(s32 size);
void QuestCounter_DrawTeamChoice(s32 x, s32 y0);
QuestInfo *Quest_GetInfoAny(u16 id);
/* Defined in unk_76DB40.c as (s16 x, s16 y, u8 *list, s8 room): this unit
 * passes y unextended, so it saw another declaration, and the two are separate
 * units. */
void Quest_DrawArenaTeams(s16, s32, NetUser *, s32); /* unit's own declaration: with the definition's void (s16 x, s16 y, u8 *list, s8 room) (lobby/unk_76DB40.c), QuestCounter_DrawArenaPage 6 of 46 words differ */
void Quest_DrawPartyList(s16 x, s32 y0, s32 k);
void Font_SetColor(s32 color);
void QuestCounter_Draw(void *);
void QuestCounter_Draw2(void *arg0);
void QuestCounter_DrawRecruit(WinSlide *, s16);
void QuestCounter_DrawQuestWindow(s16 x, s16 y);

void QuestCounter_DrawArenaPage(s16 x, s32 y)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;

    Quest_GetInfoAny(gPostedQuests[w->quest].quest);
    Font_SetColor(0x80101010);
    switch (w->page) {
    case 0:
        Quest_DrawArenaTeams(x, y, w->members, w->quest);
        break;
    case 1:
        Quest_DrawPartyList(x, y, w->quest);
        break;
    }
}

void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 kind, s32 b);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
extern char *D_0080C18C;

void QuestCounter_DrawQuestWindow(s16 x, s16 y)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    PostedQuest *q = &gPostedQuests[w->quest];

    Quest_GetInfoAny(q->quest);
    if (q->quest == 0) {
        return;
    }
    Font_SetSpacing(0x14, 0x14);
    Menu_DrawLabelBar(0x118, 0x12, 0xA0, 0xFFB0B000, D_0080C18C, 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    Menu_DrawQuestSheet(x, y, 0x140, 0x168, NULL, w->page, -1, -1);
    Font_SetColor(0x80101010);
    if (w->step == 6) {
        Menu_DrawPageNum(x + 0x104, y + 0xE, w->page, w->pages, 1);
    }
    Font_SetSpacing(0x12, 0x12);
    switch (q->desk) {
    case 0:
    case 1:
    case 2:
    case 4:
        QuestCounter_DrawQuestPage(x, y + 0xE);
        break;
    case 3:
        QuestCounter_DrawArenaPage(x, (s16)(y + 0xE));
        break;
    }
}

void QuestCounter_Draw(void *t)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;

    Text_SetSize(*(s32 *)((u8 *)t + 0x18));
    switch (w->step) {
    case 2:
    case 3:
        QuestCounter_DrawRecruit(w->board, 0x36);
        break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 20:
        QuestCounter_DrawQuestWindow(w->questWin.pos, 0x3A);
        break;
    }
}

extern char *gArenaQuestTexts[];
extern char *D_0080CCE8;

void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
void Text_SetColor(s16 color);
void Win_Draw(s32, s32, s32, s32, s32, s32, s16);

/* The window of three choices (gArenaQuestTexts[0x33 + i]) under the title
 * D_0080CCE8, arrows at the cursor (+0x4B5 of the menu work area). The
 * title's x has its own variable (tx): with cx for both, the loop's string
 * and x swap s0 and s1. */
void QuestCounter_DrawTeamChoice(s32 x, s32 y0)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    s16 y = y0;
    s16 i;
    char **t;
    s16 tx;
    s16 cx;
    char *s;

    Win_Draw(x, y0, 0x1A4, 0xDC, 0xFF808080, 0xE0402010, 3);
    y += 10;
    Font_SetSpacing(0x12, 0x12);
    Text_SetColor(0);
    s = D_0080CCE8;
    tx = Text_CenterX(s, 0x140, 0x12);
    Font_SetPos(tx, y += 0x18);
    Text_PrintfColored(s);
    y += 0x1E;
    for (i = 0, t = gArenaQuestTexts; i < 3; i++, t++) {
        s = t[0x33];
        cx = Text_CenterX(s, 0x140, 0x12);
        Font_SetPos(cx, y += 0x1B);
        Text_PrintfColored(s);
        if (w->cursor == i) {
            Menu_DrawArrowsRev(cx, y, 0x12, s, 0xFFA0A040);
        }
    }
}

void QuestCounter_Draw2(void *t)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;

    Text_SetSize(*(s32 *)((u8 *)t + 0x18));
    switch (w->step) {
    case 0xB:
        switch (w->subStep) {
        case 1:
        case 2:
            QuestCounter_DrawTeamChoice(0x6E, w->kindWin.pos);
            break;
        }
        break;
    }
}
