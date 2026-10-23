/*
 * The quest counter's draws (QuestCounterMenu in gMenuWork): its two draw layers,
 * QuestCounter_Draw (the recruit board while a quest is chosen, then the window of the
 * quest chosen) and QuestCounter_Draw2 (the arena cup's team choice); the quest window
 * itself (title bar, money, quest sheet, page number, and its page: the quest's own page,
 * or for an arena cup quest the two teams or the party list); and the team choice's
 * window. QuestCounter_Step, in the counter's main unit, runs the steps these draws
 * follow (QuestCounterMenu.step) and installs the two layers (LobbyMenu_SetDraw).
 */
#include "common.h"
#include "menu_task.h"
#include "lobby_text.h"
#include "scene.h"
#include "save.h"
#include "item.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "quest.h"
#include "lobby_labels.h"
#include "text.h"
#include "quest_counter_draw.h"
#include "font.h"
#include "menu_sheets.h"
#include "menu_widgets.h"

void QuestCounter_DrawQuestPage(s16 x, s16 y);
extern u8 gMenuWork;
/* unit's own declaration: y and the room are passed as plain ints here (the definition takes an s16 y and an s8 room); the arena page passes its y without narrowing it, as the game's code does, so this declaration stays */
void Quest_DrawArenaTeams(s16, s32, NetUser *, s32);
void QuestCounter_DrawRoomSettings(s16 x, s32 y0, s32 k);
void QuestCounter_DrawBoard(WinSlide *, s16);

/* The quest window's page for an arena cup quest: page 0 the room's members in their
 * two teams (Quest_DrawArenaTeams), page 1 the party list. The quest's info is looked up
 * and left unused, as the game's code does. */
void QuestCounter_DrawArenaPage(s16 x, s32 y)
{
    QuestCounterMenu *menu = (QuestCounterMenu *)&gMenuWork;

    Quest_GetInfoAny(gPostedQuests[menu->quest].quest);
    Font_SetColor(0x80101010);
    switch (menu->page) {
    case 0:
        Quest_DrawArenaTeams(x, y, menu->members, menu->quest);
        break;
    case 1:
        QuestCounter_DrawRoomSettings(x, y, menu->quest);
        break;
    }
}

/* The window of the posted quest chosen (nothing while the entry has no quest): the
 * counter's title bar, the money, the quest sheet at menu->page, the page number while
 * the room is viewed (step 6, QuestCounter_ViewQuest), then the page itself: the arena
 * page for an arena cup quest (desk 3), else the quest's page. The quest's info is looked
 * up and left unused, as the game's code does. */
void QuestCounter_DrawQuestWindow(s16 x, s16 y)
{
    QuestCounterMenu *menu = (QuestCounterMenu *)&gMenuWork;
    PostedQuest *posted = &gPostedQuests[menu->quest];

    Quest_GetInfoAny(posted->quest);
    if (posted->quest == 0) {
        return;
    }
    Font_SetSpacing(20, 20);
    Menu_DrawLabelBar(280, 18, 160, 0xFFB0B000, gListTitles[LIST_TITLE_HUNTER_RECRUIT], 2, 1);
    Menu_DrawMoney(480, 16, 0);
    Menu_DrawQuestSheet(x, y, 320, 360, NULL, menu->page, -1, -1);
    Font_SetColor(0x80101010);
    if (menu->step == QUEST_COUNTER_VIEW_QUEST) {
        Menu_DrawPageNum(x + 260, y + 14, menu->page, menu->pages, 1);
    }
    Font_SetSpacing(18, 18);
    switch (posted->desk) {
    case 0:
    case 1:
    case 2:
    case 4:
        QuestCounter_DrawQuestPage(x, y + 14);
        break;
    case 3:
        QuestCounter_DrawArenaPage(x, (s16)(y + 14));
        break;
    }
}

/* The quest counter's first draw layer: the recruit board while it opens and a quest is
 * chosen (steps 2-3), the chosen quest's window from the moment it slides in until the
 * room is joined or left (steps 5-13 and 20). */
void QuestCounter_Draw(MenuTask *task)
{
    QuestCounterMenu *menu = (QuestCounterMenu *)&gMenuWork;

    Text_SetSize(task->textSize);
    switch (menu->step) {
    case QUEST_COUNTER_OPEN_BOARD:
    case QUEST_COUNTER_CHOOSE_QUEST:
        QuestCounter_DrawBoard(menu->board, 54);
        break;
    case QUEST_COUNTER_OPEN_QUEST:
    case QUEST_COUNTER_VIEW_QUEST:
    case QUEST_COUNTER_ASK_JOIN:
    case QUEST_COUNTER_CHECK:
    case QUEST_COUNTER_PASSWORD:
    case QUEST_COUNTER_PREPARE:
    case QUEST_COUNTER_CHOOSE_ROLE:
    case QUEST_COUNTER_ENTER:
    case QUEST_COUNTER_ACCEPTED:
    case QUEST_COUNTER_LEAVE_ROOM:
        QuestCounter_DrawQuestWindow(menu->questWin.pos, 58);
        break;
    }
}

void Win_Draw(s32, s32, s32, s32, s32, s32, s16);

/* The arena cup's team choice (QuestCounter_ChooseArenaRole): a window with the prompt
 * (QUEST_COUNTER_TEXT_PICK_TEAM) over the three roles to join as, team A's
 * assist, team B's leader and team B's assist (cursor 0-2 becomes kind 2-4),
 * with arrows at menu->cursor. The roles (QUEST_COUNTER_TEXT_ROLES on) are
 * read through a pointer that steps from the table's start: only this form
 * gives the game's load.
 * The title's x has its own variable (titleX): with one variable for both, the
 * compiler swaps the registers of the loop's text and x. */
void QuestCounter_DrawTeamChoice(s32 x, s32 winY)
{
    QuestCounterMenu *menu = (QuestCounterMenu *)&gMenuWork;
    s16 y = winY;
    s16 i;
    char **choices;
    s16 titleX;
    s16 choiceX;
    char *text;

    Win_Draw(x, winY, 420, 220, 0xFF808080, 0xE0402010, 3);
    y += 10;
    Font_SetSpacing(18, 18);
    Text_SetColor(0);
    text = gQuestCounterTexts[QUEST_COUNTER_TEXT_PICK_TEAM];
    titleX = Text_CenterX(text, 320, 18);
    Font_SetPos(titleX, y += 24);
    Text_PrintfColored(text);
    y += 30;
    for (i = 0, choices = gQuestCounterTexts; i < 3; i++, choices++) {
        text = choices[QUEST_COUNTER_TEXT_ROLES];
        choiceX = Text_CenterX(text, 320, 18);
        Font_SetPos(choiceX, y += 27);
        Text_PrintfColored(text);
        if (menu->cursor == i) {
            Menu_DrawArrowsRev(choiceX, y, 18, text, 0xFFA0A040);
        }
    }
}

/* The quest counter's second draw layer: the arena cup's team choice while it slides in
 * and while it is chosen (step 11, sub-steps 1-2). */
void QuestCounter_Draw2(MenuTask *task)
{
    QuestCounterMenu *menu = (QuestCounterMenu *)&gMenuWork;

    Text_SetSize(task->textSize);
    switch (menu->step) {
    case QUEST_COUNTER_CHOOSE_ROLE:
        switch (menu->subStep) {
        case 1:
        case 2:
            QuestCounter_DrawTeamChoice(110, menu->kindWin.pos);
            break;
        }
        break;
    }
}
