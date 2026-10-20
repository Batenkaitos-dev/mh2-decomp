/*
 * The quest desk's draws and the monsters' availability.
 *
 * QuestDesk_Update's menu (the quest desk, TOWN_MODE_QUEST_DESK) has two
 * draws, set by QuestDesk_Open. QuestDesk_Draw2 draws the desk window as it
 * slides in and out (desk->win): the entries (QuestDesk_DrawFieldInfo), the
 * lists by map group or by type (QuestDesk_DrawIndexList) and, for a wanted
 * quest, its wanted card. QuestDesk_Draw draws, at the window's shown place,
 * the quest sheet (QuestBoard_Draw: three pages, Quest_DrawOverview,
 * Quest_DrawTerms and Quest_DrawDetails, with the clear mark and the
 * stamps), the online posting's recruit form, the field guide
 * (QuestDesk_DrawFieldGuide: a map group's monsters and specialty items by
 * season and time of day) or the elder's forecast.
 *
 * Monster_IsAvailable says whether a monster has come to the quests yet:
 * offline by the village's conditions and the monsters hunted, online by
 * the town's conditions and event flags. Quest_IsOffered and the field
 * guide ask it.
 *
 * The data are the field guide's: the monsters of each map group by season
 * (gFieldGuideTables), the specialty items (gFieldGuideLists), the desk's
 * map groups (gQuestMapGroups) and each map group's sheet frame
 * (gMapGroupMapIds).
 */
#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "lobby_globals.h"
#include "quest.h"
#include "lobby_menu.h"
#include "lobby_vobj.h"
#include "area.h"
#include "em.h"
#include "menu_task.h"
#include "lobby_labels.h"

/* prototypes */
s32 Monster_IsAvailableOffline(u16 em);
s32 Monster_IsAvailableOnline(u16 em);
s32 Game_IsOnline(void);
/* end of prototypes */

QuestInfo *Quest_GetDiscInfo(u16 id);
u16 Map_Group(u16 id);
void *memset(void *dst, s32 c, u32 n);
s32 Cond_Check(u16 id);

char *Quest_GetText(u16 id, s32 k);
void Text_PrintfColored(char *fmt, ...);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
extern u8 gMenuWork[];
extern u32 *gDecoFrameColors[];
extern char *gInfoIndexTexts[];
void Font_SetSpacing(u8 a, u8 b);
void Menu_DrawDecoFrame(s16 x, s16 y0, s16 w0, s16 h, s8 kind, s8 b);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);

/*
 * The desk window's entries, at (x, y): the season's frame
 * (Menu_DrawDecoFrame), the quest introduction's heading with the quest
 * entries under it (the board, by map group, by type), then the hunting
 * information's heading with the field guide and the forecast under it.
 * The entry at the cursor (desk->entrySel) has the frame's arrows and a
 * shadow; the others are in the frame's item colour.
 * The four colours are read with one pointer stepped through the season's
 * colour row (gDecoFrameColors: the arrows, the shadow, the text, the
 * items), as the game's code does.
 */
void QuestDesk_DrawFieldInfo(s16 x, s16 y)
{
    u32 *colors;
    s16 headY = y;
    u32 cArrow;
    u32 cShadow;
    u32 cItem;
    u32 cText;
    s16 centerX;
    QuestDesk *desk = (QuestDesk *)gMenuWork;
    s16 rowY;
    s32 i;
    s16 textX;
    char *text;
    s16 entry;

    Menu_DrawDecoFrame(x, y, QUEST_SHEET_W, QUEST_SHEET_H, 3, desk->season);
    colors = gDecoFrameColors[desk->season];
    cArrow = *(colors += 2);
    cShadow = *++colors;
    cText = *++colors;
    cItem = *++colors;
    Font_SetColor(MENU_INK_WHITE);
    centerX = x + 0xA0;
    rowY = (s16)y + 0x64;
    Font_SetSpacing(0x16, 0x16);
    text = gQuestSheetTexts[QUEST_SHEET_TEXT_INTRO_HEAD];
    Font_SetPos(Text_CenterX(text, centerX, 0x16), headY += 0x50);
    Text_PrintfColored(text);
    text = gQuestSheetTexts[QUEST_SHEET_TEXT_HUNT_INFO_HEAD];
    Font_SetPos(Text_CenterX(text, centerX, 0x16), headY + 0x78);
    Text_PrintfColored(text);
    Font_SetSpacing(0x14, 0x14);
    rowY += 0xA;
    for (i = 0; i < desk->entryCount; i++) {
        entry = desk->entries[i];
        text = gInfoIndexTexts[entry];
        if (entry > QUEST_DESK_FORECAST) {
            textX = Text_CenterX(text, centerX, 0x14);
            if (i == desk->entrySel) {
                Menu_DrawArrowsRev(textX, rowY, 0x14, text, cArrow);
                Text_DrawShadowRgb(textX, rowY, cText, cShadow, text);
            } else {
                Font_SetColor(cItem);
                Font_SetPos(textX, rowY);
                Text_PrintfColored(text);
            }
            rowY += 0x18;
        }
    }
    rowY = headY + 0x96;
    for (i = 0; i < desk->entryCount; i++) {
        entry = desk->entries[i];
        text = gInfoIndexTexts[entry];
        if (entry < QUEST_DESK_BOARD) {
            textX = Text_CenterX(text, centerX, 0x14);
            if (i == desk->entrySel) {
                Menu_DrawArrowsRev(textX, rowY, 0x14, text, cArrow);
                Text_DrawShadowRgb(textX, rowY, cText, cShadow, text);
            } else {
                Font_SetColor(cItem);
                Font_SetPos(textX, rowY);
                Text_PrintfColored(text);
            }
            rowY += 0x18;
        }
    }
}

extern char *gEmClassNames[];
extern s16 gQuestMapGroups[];
extern char *gStageNames[];

/*
 * The desk window's list choice, at (x, y): the season's frame, the chosen
 * entry's name in brackets as the heading, then the lists, centred: by
 * type the monster classes (gEmClassNames from 1), by map group the
 * stages of gQuestMapGroups. The list at the cursor (desk->list) has the
 * frame's arrows and a shadow. The colours are read as in
 * QuestDesk_DrawFieldInfo.
 */
void QuestDesk_DrawIndexList(s16 x, s16 y)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;
    s32 end;
    u32 cArrow;
    u32 cShadow;
    s32 i;
    char *text;
    u32 cItem;
    u32 *colors;
    s16 textX;
    s32 first;
    u32 cText;

    Menu_DrawDecoFrame(x, y, QUEST_SHEET_W, QUEST_SHEET_H, 3, desk->season);
    colors = gDecoFrameColors[desk->season];
    cArrow = *(colors += 2);
    cShadow = *++colors;
    cText = *++colors;
    cItem = *++colors;
    Font_SetColor(MENU_INK_WHITE);
    x = x + 0xA0;
    Font_SetSpacing(0x16, 0x16);
    text = gInfoIndexTexts[desk->entries[desk->entrySel]];
    Font_SetPos(Text_CenterX(text, x, 0x16) - 0x16, y + 0x3C);
    Text_PrintfColored(gPriceInfoTexts[PRICE_INFO_BRACKET_FMT], text);
    if (desk->entries[desk->entrySel] == QUEST_DESK_BY_TYPE) {
        first = 1;
        end = desk->listCount + 1;
    } else {
        first = 0;
        end = desk->listCount;
    }
    Font_SetSpacing(0x14, 0x14);
    if (desk->entries[desk->entrySel] == QUEST_DESK_BY_TYPE) {
        y += 0x84;
    } else {
        y += 0x6E;
    }
    for (i = first; i < end; i++) {
        if (desk->entries[desk->entrySel] == QUEST_DESK_BY_TYPE) {
            text = gEmClassNames[i];
        } else {
            text = gStageNames[gQuestMapGroups[i]];
        }
        textX = Text_CenterX(text, x, 0x14);
        if (desk->list == i - first) {
            Menu_DrawArrowsRev(textX, y, 0x14, text, cArrow);
            Text_DrawShadowRgb(textX, y, cText, cShadow, text);
        } else {
            Font_SetColor(cItem);
            Font_SetPos(textX, y);
            Text_PrintfColored(text);
        }
        y += 0x18;
    }
}

char *strcat(char *dst, const char *src);
void Quest_DrawMustOrderOverview(s16 x, s16 y, QuestInfo *q);
extern char gQuestSheetStrFmt[];
extern char *gQuestKindNames[];

/*
 * The quest sheet's first page, at (x, y): the title, then beside their
 * labels (in QUEST_SHEET_LABEL_INK, 0xA to the left) the main target, the
 * difficulty as one star per level (quest->stars), the two sub targets and the
 * order terms (quest->terms, by gQuestKindNames). Online, a must-order quest
 * shows its summary instead (Quest_DrawMustOrderOverview).
 */
void Quest_DrawOverview(s16 x, s16 y, QuestInfo *quest)
{
    s32 i;
    s32 id;
    s16 top;
    char stars[0x40];

    if ((quest->flags & QUEST_FLAG_MUST_ORDER) != 0 && Game_IsOnline() == 1) {
        Quest_DrawMustOrderOverview(x, y, quest);
        return;
    }
    id = quest->id;
    top = y;
    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(gQuestSheetStrFmt, Quest_GetText(id, QUEST_TEXT_TITLE));
    Font_SetPos(x, y += 0x3C);
    Text_PrintfColored(gQuestSheetStrFmt, Quest_GetText(id, QUEST_TEXT_MAIN_TARGET));
    Font_SetPos(x + 0x48, y += 0x18);
    stars[0] = 0;
    for (i = 0; i < quest->stars; i++) {
        strcat(stars, gLobbyMarks[LOBBY_MARK_STAR]);
    }
    Text_PrintfColored(gQuestSheetStrFmt, stars);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, Quest_GetText(id, QUEST_TEXT_SUB_TARGET_A));
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, Quest_GetText(id, QUEST_TEXT_SUB_TARGET_B));
    Font_SetPos(x, y + 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestKindNames[quest->terms]);
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    x = x - 0xA;
    y = top + 0x14;
    y += 0x2A;
    Font_SetPos(x, y);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_MAIN_TARGET]);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_DIFFICULTY]);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_SUB_TARGET_A]);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_SUB_TARGET_B]);
    Font_SetPos(x, y + 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_SPECIAL_COND]);
}

INCLUDE_RODATA("asm/lobby/nonmatchings/quest_desk_draw", gQuestSheetStrFmt);

s32 sprintf(char *buf, const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
u16 Dojo_GetParMinutes(u16 id);
void Quest_DrawMustOrderTerms(s16 x, s16 y, QuestInfo *q);

/*
 * The quest sheet's second page, at (x, y): the title, then beside their
 * labels the time limit in minutes, the contract fee, the main reward and
 * the two sub rewards in zenny, and the success and failure conditions. A
 * drill (QUEST_FLAG_DRILL) shows its time to beat (Dojo_GetParMinutes)
 * for the fee, and points for the rewards. The numbers are printed into
 * an ASCII line and widened to full-width characters (Sjis_Widen). Online,
 * a must-order quest shows its contract instead (Quest_DrawMustOrderTerms).
 */
void Quest_DrawTerms(s16 x, s16 y, QuestInfo *quest)
{
    s32 id;
    s16 top;
    char wide[0x40];
    char line[0x40];

    if ((quest->flags & QUEST_FLAG_MUST_ORDER) != 0 && Game_IsOnline() == 1) {
        Quest_DrawMustOrderTerms(x, y, quest);
        return;
    }
    id = quest->id;
    top = y;
    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(gQuestSheetStrFmt, Quest_GetText(id, QUEST_TEXT_TITLE));
    Font_SetPos(x + 0x7E, y += 0x2A);
    sprintf(line, "%d%s", quest->timeLimit / 60 / 30, gLobbyWords[LOBBY_WORD_MINUTES]);
    Sjis_Widen(line, wide);
    Text_PrintfColored(gQuestSheetStrFmt, wide);
    Font_SetPos(x + 0x7E, y += 0x18);
    if ((quest->flags & QUEST_FLAG_DRILL) != 0) {
        sprintf(line, gQuestSheetTexts[QUEST_SHEET_TEXT_TIME_ATTACK_FMT], Dojo_GetParMinutes(id));
    } else {
        sprintf(line, "%dz", quest->fee);
    }
    Sjis_Widen(line, wide);
    Text_PrintfColored(gQuestSheetStrFmt, wide);
    Font_SetPos(x + 0x7E, y += 0x18);
    if ((quest->flags & QUEST_FLAG_DRILL) != 0) {
        sprintf(line, "%dP", quest->reward);
    } else {
        sprintf(line, "%dz", quest->reward);
    }
    Sjis_Widen(line, wide);
    Text_PrintfColored(gQuestSheetStrFmt, wide);
    Font_SetPos(x + 0x7E, y += 0x18);
    if ((quest->flags & QUEST_FLAG_DRILL) != 0) {
        sprintf(line, "%dP", quest->rewardSubA);
    } else {
        sprintf(line, "%dz", quest->rewardSubA);
    }
    Sjis_Widen(line, wide);
    Text_PrintfColored(gQuestSheetStrFmt, wide);
    Font_SetPos(x + 0x7E, y += 0x18);
    if ((quest->flags & QUEST_FLAG_DRILL) != 0) {
        sprintf(line, "%dP", quest->rewardSubB);
    } else {
        sprintf(line, "%dz", quest->rewardSubB);
    }
    Sjis_Widen(line, wide);
    Text_PrintfColored(gQuestSheetStrFmt, wide);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, Quest_GetText(id, QUEST_TEXT_SUCCESS));
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(gQuestSheetStrFmt, Quest_GetText(id, QUEST_TEXT_FAILURE));
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    x = x - 0xA;
    y = top + 0x14;
    y += 0x2A;
    Font_SetPos(x, y);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_TIME_LIMIT]);
    Font_SetPos(x, y += 0x18);
    if ((quest->flags & QUEST_FLAG_DRILL) != 0) {
        Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_TIME_ATTACK]);
    } else {
        Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_CONTRACT_FEE]);
    }
    Font_SetPos(x, y += 0x18);
    if ((quest->flags & QUEST_FLAG_DRILL) != 0) {
        Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_MAIN_POINTS]);
    } else {
        Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_MAIN_REWARD]);
    }
    Font_SetPos(x, y += 0x18);
    if ((quest->flags & QUEST_FLAG_DRILL) != 0) {
        Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_SUB_A_POINTS]);
    } else {
        Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_SUB_A_REWARD]);
    }
    Font_SetPos(x, y += 0x18);
    if ((quest->flags & QUEST_FLAG_DRILL) != 0) {
        Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_SUB_B_POINTS]);
    } else {
        Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_SUB_B_REWARD]);
    }
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_CLEAR_COND]);
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_FAIL_COND]);
}

void DojoMenu_DrawDrillDetails(s16 x, s16 y, QuestInfo *quest);

/*
 * The quest sheet's third page, at (x, y): the title, then beside their
 * labels the client, the destination (the stage of the quest's map group)
 * and the quest's details. A drill shows its own page instead
 * (DojoMenu_DrawDrillDetails).
 */
void Quest_DrawDetails(s16 x, s16 y, QuestInfo *quest)
{
    s32 id = quest->id;
    s16 top = y;

    if ((quest->flags & QUEST_FLAG_DRILL) != 0) {
        DojoMenu_DrawDrillDetails(x, y, quest);
        return;
    }
    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(gQuestSheetStrFmt, Quest_GetText(id, QUEST_TEXT_TITLE));
    Font_SetPos(x + 0x5A, y += 0x34);
    Text_PrintfColored(gQuestSheetStrFmt, Quest_GetText(id, QUEST_TEXT_CLIENT));
    Font_SetPos(x + 0x5A, y += 0x18);
    Text_PrintfColored(gStageNames[Map_Group(quest->map)]);
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(gQuestSheetStrFmt, Quest_GetText(id, QUEST_TEXT_DETAILS));
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    x = x - 0xA;
    y = top + 0x14;
    y += 0x34;
    Font_SetPos(x, y);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_CLIENT]);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_DESTINATION]);
    Font_SetPos(x, y + 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_CLIENT_DETAILS]);
}

void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);

/* The cleared mark at the quest sheet's foot (x, y the sheet's corner). */
void Quest_DrawClearMark(s16 x, s16 y)
{
    Font_SetSpacing(0x12, 0x12);
    Text_DrawShadow(x + 0xC8, y + 0x132, 1, 7, gListTitles[LIST_TITLE_CLEAR]);
}

void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 kind, s32 b);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl);
void QuestBoard_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
s32 Save_IsQuestCleared(u16 id);
extern u8 gMapGroupMapIds[][2];
extern u16 gQuestStampRand;

/*
 * The quest sheet, at (x, y): the sheet of the chosen list's quest at the
 * cursor (desk->quest instead once one is chosen), on the pile of the list's
 * quests: one tab per quest with its map group, the cursor's on the sheet
 * (Menu_DrawQuestSheet). Its frame is the map group's (gMapGroupMapIds),
 * its emblem the season's, none for a drill or a training quest. Then the
 * page number (unless desk->hidePageNum), the page desk->page (in the drop talk,
 * of the taken quest), the cleared mark for a quest cleared before, and
 * the stamps: the wanted stamp shaking, then the accepted stamp, both for
 * a wanted quest. Nothing is drawn while the sheet is hidden (desk->unk6D or
 * the drop talk's hideSheet).
 */
void QuestBoard_Draw(s16 x, s16 y)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;
    QuestInfo *info;
    QuestDeskCancel *cancel;
    s32 i;
    u16 id;
    u8 frame;
    s32 sheetX;
    s16 tabs[QUEST_LIST_LEN];

    cancel = &desk->cancel;
    info = Quest_GetDiscInfo(desk->lists[desk->list][desk->sel]);
    if (desk->unk6D == 1 || cancel->hideSheet == 1) {
        return;
    }
    if (desk->quest != NULL) {
        info = desk->quest;
    }
    memset(tabs, 0, sizeof(tabs));
    for (i = 0; i < QUEST_LIST_LEN; i++) {
        id = desk->lists[desk->list][i];
        if (id == 0) {
            break;
        }
        tabs[i] = Map_Group(Quest_GetDiscInfo(id)->map);
        if (i == desk->sel) {
            tabs[i] = -tabs[i];
        }
    }
    frame = gMapGroupMapIds[Map_Group(info->map)][0];
    if ((info->flags & QUEST_FLAG_DRILL) != 0 || (info->flags & QUEST_FLAG_TRAINING) != 0) {
        Menu_DrawQuestSheet(x, y, QUEST_SHEET_W, QUEST_SHEET_H, tabs, desk->sel, frame, -1);
    } else {
        Menu_DrawQuestSheet(x, y, QUEST_SHEET_W, QUEST_SHEET_H, tabs, desk->sel, frame, desk->season);
    }
    Font_SetColor(QUEST_SHEET_INK);
    if (desk->hidePageNum == 0) {
        Menu_DrawPageNum(0x21C, y += 5, desk->page, 3, 1);
    } else {
        y += 5;
    }
    Font_SetSpacing(0x12, 0x12);
    switch (desk->page) {
    case QUEST_SHEET_OVERVIEW:
        if (desk->step == QUEST_DESK_STEP_DROP) {
            info = desk->quest;
        }
        Quest_DrawOverview((s16)(x + 0x28) + 0xA, y + 4, info);
        break;
    case QUEST_SHEET_TERMS:
        if (desk->step == QUEST_DESK_STEP_DROP) {
            info = desk->quest;
        }
        Quest_DrawTerms((s16)(x + 0x28) + 0xA, y + 4, info);
        break;
    case QUEST_SHEET_DETAILS:
        if (desk->step == QUEST_DESK_STEP_DROP) {
            info = desk->quest;
        }
        Quest_DrawDetails((s16)(x + 0x28) + 0xA, y + 4, info);
        break;
    }
    if (Save_IsQuestCleared(info->id) != 0) {
        Quest_DrawClearMark(x + 0x28, y);
    }
    if (desk->quest != NULL) {
        switch (desk->step) {
        case QUEST_DESK_STEP_WANTED_STAMP:
            Menu_DrawStamp(x + 0x28, y, QUEST_SHEET_W, QUEST_SHEET_H, gQuestStampRand, desk->wantedStamp.timer);
            break;
        case QUEST_DESK_STEP_DROP:
        case QUEST_DESK_STEP_STAMP:
        case QUEST_DESK_STEP_WAIT_TALK:
        case QUEST_DESK_STEP_CLOSE:
            if (desk->wantedFee > 0) {
                sheetX = x + 0x28;
                QuestBoard_DrawStamp(sheetX, y, QUEST_SHEET_W, QUEST_SHEET_H, gQuestStampRand, desk->stamp.timer);
                Menu_DrawStamp(sheetX, y, QUEST_SHEET_W, QUEST_SHEET_H, gQuestStampRand, desk->wantedStamp.timer);
                break;
            }
            Menu_DrawStamp(x + 0x28, y, QUEST_SHEET_W, QUEST_SHEET_H, gQuestStampRand, desk->stamp.timer);
            break;
        }
    }
}

void Menu_DrawArrows(s16 x, s16 y, s16 size, char *text, s32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
extern s8 gQuestMinMembers;
extern s8 gRoomPassOn;
extern char gRoomPassword[];

/*
 * The online posting's recruit form, at (x, y), on a plain sheet: the
 * heading, then beside their labels the least members to set out
 * (gQuestMinMembers) and whether the room has a password (gRoomPassOn),
 * the password in its box, and the decide button. The row at the cursor
 * (desk->sel, enum RoomSetting) has arrows, and the password's box is drawn in the cursor's
 * colour when it is chosen.
 */
void QuestDesk_DrawRecruitForm(s16 x, s16 y)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;
    s16 top = y;
    s32 labelTop;
    s16 labelX;
    s32 valueX;
    s32 boxX;
    s32 left;

    Menu_DrawQuestSheet(x, y, QUEST_SHEET_W, QUEST_SHEET_H, NULL, 1, -1, -1);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(QUEST_SHEET_INK);
    y += 0x28;
    left = x;
    valueX = left + 0xD8;
    Font_SetPos(valueX, y += 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, gMemberCountTexts[gQuestMinMembers]);
    if (desk->sel == ROOM_SETTING_MEMBERS) {
        Menu_DrawArrows(valueX, y, 0x12, gMemberCountTexts[gQuestMinMembers], QUEST_FORM_CURSOR_COLOR);
    }
    Font_SetPos(valueX, y += 0x2A);
    Text_PrintfColored(gNoneSetTexts[gRoomPassOn]);
    if (desk->sel == ROOM_SETTING_PASS_ON) {
        Menu_DrawArrows(valueX, y, 0x12, gNoneSetTexts[gRoomPassOn], QUEST_FORM_CURSOR_COLOR);
    }
    Font_SetPos(Text_CenterX(gRoomPassword, 0x1B8, 0x12), y += 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, gRoomPassword);
    if (desk->sel == ROOM_SETTING_PASSWORD) {
        boxX = left + 0x64;
        /* the arrows span the box: thirteen spaces give them its width */
        Menu_DrawArrowsRev(boxX, y, 0x12, "             ", QUEST_FORM_CURSOR_COLOR);
        Gfx_DrawRectFrame(boxX, y - 1, 0x78, 0x14, QUEST_FORM_CURSOR_COLOR);
        Gfx_DrawRectFrame(left + 0x63, y - 2, 0x7A, 0x16, QUEST_FORM_CURSOR_COLOR);
    } else {
        Gfx_DrawRectFrame(left + 0x64, y - 1, 0x78, 0x14, QUEST_FORM_BOX_COLOR);
        Gfx_DrawRectFrame(left + 0x63, y - 2, 0x7A, 0x16, QUEST_FORM_BOX_COLOR);
    }
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    labelTop = top + 0x28;
    y = labelTop;
    labelX = left + 0x28;
    Font_SetPos(Text_CenterX(gQuestSheetTexts[QUEST_SHEET_TEXT_RECRUIT], 0x1B8, 0x12), labelTop);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_RECRUIT]);
    Font_SetPos(labelX, y += 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_MEMBERS]);
    Font_SetPos(labelX, y += 0x2A);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_PASSWORD]);
    labelX = Text_CenterX(gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE], 0x1B8, 0x12);
    Font_SetPos(labelX, y += 0x78);
    Text_PrintfColored(gQuestSheetStrFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE]);
    if (desk->sel == ROOM_SETTING_DECIDE) {
        Menu_DrawArrowsRev(labelX, y, 0x12, gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE], QUEST_FORM_CURSOR_COLOR);
    }
}

s32 Monster_IsHunted(u16 em);
s32 ElderForecast_IsUnlocked(void);

/*
 * Offline, 1 when monster kind `em` has come to the village's quests,
 * else 0. The Velocidrome and the Bulldrome always; the others once the
 * village condition (Cond_Check) of their map's opening is met: 7 the
 * desert's, 0xA the swamp's, 0x10 the snowy mountains', 0x15 the
 * volcano's and the towers', and more for the later ones; the Rathalos
 * only away from the village. A subspecies comes once its original has
 * been hunted (Monster_IsHunted). The Yian Garuga and the Rajang never;
 * any kind not listed (the elder dragons) once the forecast is told
 * (ElderForecast_IsUnlocked).
 */
s32 Monster_IsAvailableOffline(u16 em)
{
    switch (em) {
    case EM_KIND_VELOCIDROME:
    case EM_KIND_BULLDROME:
        return 1;
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_DAIMYO_HERMITAUR:
    case EM_KIND_GENDROME:
    case EM_KIND_CEPHADROME:
        return Cond_Check(7U);
    case EM_KIND_CONGALALA:
    case EM_KIND_PLESIOTH:
    case EM_KIND_IODROME:
    case EM_KIND_GYPCEROS:
        return Cond_Check(0xAU);
    case EM_KIND_RATHIAN:
    case EM_KIND_BLANGONGA:
    case EM_KIND_KHEZU:
        return Cond_Check(0x10U);
    case EM_KIND_DIABLOS:
    case EM_KIND_SHOGUN_CEANATAUR:
    case EM_KIND_BASARIOS:
    case EM_KIND_GRAVIOS:
        return Cond_Check(0x15U);
    case EM_KIND_MONOBLOS:
        return Cond_Check(0x52U);
    case EM_KIND_RATHALOS:
        if ((Cond_Check(0x4FU) == 1) && (gSave.awayFromVillage != 0)) {
            return 1;
        }
        break;
    case EM_KIND_KUSHALA_DAORA:
        return Cond_Check(0xDU);
    case EM_KIND_LUNASTRA:
    case EM_KIND_KIRIN:
        return Cond_Check(0x1AU);
    case EM_KIND_TEOSTRA:
        return Cond_Check(0x1BU);
    case EM_KIND_CHAMELEOS:
        return Cond_Check(0x54U);
    case EM_KIND_YIAN_GARUGA:
        return 0;
    case EM_KIND_BLUE_YIAN_KUT_KU:
        return Monster_IsHunted(EM_KIND_YIAN_KUT_KU);
    case EM_KIND_PURPLE_GYPCEROS:
        return Monster_IsHunted(EM_KIND_GYPCEROS);
    case EM_KIND_GREEN_PLESIOTH:
        return Monster_IsHunted(EM_KIND_PLESIOTH);
    case EM_KIND_RED_KHEZU:
        return Monster_IsHunted(EM_KIND_KHEZU);
    case EM_KIND_PINK_RATHIAN:
        return Monster_IsHunted(EM_KIND_RATHIAN);
    case EM_KIND_BLACK_DIABLOS:
        return Monster_IsHunted(EM_KIND_DIABLOS);
    case EM_KIND_BLACK_GRAVIOS:
        return Monster_IsHunted(EM_KIND_GRAVIOS);
    case EM_KIND_AZURE_RATHALOS:
        return Monster_IsHunted(EM_KIND_RATHALOS);
    case EM_KIND_WHITE_MONOBLOS:
        return Monster_IsHunted(EM_KIND_MONOBLOS);
    case EM_KIND_RAJANG:
        return 0;
    default:
        return ElderForecast_IsUnlocked();
    }
    return 0;
}

s32 Save_TestEventFlag(s32 flag);

/*
 * Online, 1 when monster kind `em` has come to the lobby's quests, else
 * 0: the town's conditions (Cond_Check) 0x5C and 0x5F each open a group of
 * monsters, 0x64 the Rajang and 0x67 the Yama Tsukami; a subspecies needs
 * its original hunted (gSave.hunted) and its group's condition; the
 * Rathalos and the Azure Rathalos need EVENT_FLAG_SONG_TRACK_3. The Yian Garuga,
 * the White Monoblos and the Monoblos never; any kind not listed always.
 */
s32 Monster_IsAvailableOnline(u16 em)
{
    switch (em) {
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IODROME:
    case EM_KIND_BULLDROME:
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_GYPCEROS:
    case EM_KIND_CEPHADROME:
    case EM_KIND_DAIMYO_HERMITAUR:
    case EM_KIND_CONGALALA:
        return 1;
    case EM_KIND_BASARIOS:
    case EM_KIND_RATHIAN:
    case EM_KIND_KHEZU:
    case EM_KIND_BLANGONGA:
        if (Cond_Check(0x5CU) == 0) {
            return 0;
        }
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_DIABLOS:
    case EM_KIND_GRAVIOS:
    case EM_KIND_SHOGUN_CEANATAUR:
        if (Cond_Check(0x5FU) == 0) {
            return 0;
        }
        break;
    case EM_KIND_RAJANG:
        if (Cond_Check(0x64U) == 0) {
            return 0;
        }
        break;
    case EM_KIND_YAMA_TSUKAMI:
        return Cond_Check(0x67U);
    case EM_KIND_YIAN_GARUGA:
        return 0;
    case EM_KIND_BLUE_YIAN_KUT_KU:
        if (gSave.hunted[EM_KIND_YIAN_KUT_KU] == 0) {
            return 0;
        }
        break;
    case EM_KIND_PURPLE_GYPCEROS:
        if (gSave.hunted[EM_KIND_GYPCEROS] == 0) {
            return 0;
        }
        break;
    case EM_KIND_GREEN_PLESIOTH:
        if ((gSave.hunted[EM_KIND_PLESIOTH] == 0) || (Cond_Check(0x5FU) == 0)) {
            return 0;
        }
        break;
    case EM_KIND_RED_KHEZU:
        if ((gSave.hunted[EM_KIND_KHEZU] == 0) || (Cond_Check(0x5CU) == 0)) {
            return 0;
        }
        break;
    case EM_KIND_PINK_RATHIAN:
        if ((gSave.hunted[EM_KIND_RATHIAN] == 0) || (Cond_Check(0x5CU) == 0)) {
            return 0;
        }
        break;
    case EM_KIND_BLACK_DIABLOS:
        if ((gSave.hunted[EM_KIND_DIABLOS] == 0) || (Cond_Check(0x5FU) == 0)) {
            return 0;
        }
        break;
    case EM_KIND_BLACK_GRAVIOS:
        if ((gSave.hunted[EM_KIND_GRAVIOS] == 0) || (Cond_Check(0x5FU) == 0)) {
            return 0;
        }
        break;
    case EM_KIND_AZURE_RATHALOS:
        if ((gSave.hunted[EM_KIND_RATHALOS] == 0) || (Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3) == 0)) {
            return 0;
        }
        break;
    case EM_KIND_RATHALOS:
        if (Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3) == 0) {
            return 0;
        }
        break;
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_MONOBLOS:
        return 0;
    }
    return 1;
}

/*
 * 1 when monster kind `em` has come to the quests, else 0: online
 * Monster_IsAvailableOnline, offline Monster_IsAvailableOffline.
 */
s32 Monster_IsAvailable(u16 em)
{
    if (Game_IsOnline() == 1) {
        return Monster_IsAvailableOnline(em);
    }
    return Monster_IsAvailableOffline(em);
}

void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void ElderForecast_DrawPanel(u16 *ids, s16 x, s16 y, s32 list, s32 b);

/*
 * The elder's forecast, at (x, y): a plain sheet with one tab, two rules
 * above and below the middle (icons 0x1B and 0x1C), and the forecast's
 * panel (ElderForecast_DrawPanel: desk->forecast for the season and the time
 * of day).
 */
void QuestDesk_DrawElderInfo(s16 x, s16 y)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;
    s32 top;
    s32 left;
    s32 ruleTop;
    s16 centerY;
    s32 ruleBottom;
    s16 tabs[2];

    memset(tabs, 0, 4);
    top = y;
    tabs[0] = -99;
    Menu_DrawQuestSheet(x, top + 10, QUEST_SHEET_W, QUEST_SHEET_H, tabs, 0, -1, -2);
    left = x;
    centerY = top + 0xB4;
    ruleTop = centerY - 0x78;
    Icon_Draw(left + 10, ruleTop, 0x100, 0x20, 0x80202020, 0x1B);
    ruleBottom = centerY + 0x78;
    Icon_Draw(left + 10, ruleBottom, 0x100, 0x20, 0x80202020, 0x1B);
    Icon_Draw(left + 0x10A, ruleTop, 0x2E, 0x20, 0x80202020, 0x1C);
    Icon_Draw(left + 0x10A, ruleBottom, 0x2E, 0x20, 0x80202020, 0x1C);
    Font_SetSpacing(0x12, 0x12);
    ElderForecast_DrawPanel(desk->forecast, left + 0x14, top + 0x1E, desk->season, desk->pm);
}

extern char *gSeasonNames[];
extern char *gDaytimeNames[];
extern char *gMonsterNames[];
extern char *gItemNames[];
extern u16 **gFieldGuideTables[];
extern u16 *gFieldGuideLists[2][6];
s32 Monster_IsAvailable(u16 em);

/*
 * The field guide, at (x, y): the sheet with one tab per open map group
 * (desk->groupCount of gQuestMapGroups, map group 0xD's tab drawn as 2's) and
 * the cursor's group (desk->sel) on it, framed by its map; a group of -99 has
 * a plain sheet. On it: the season and the time of day in brackets, the
 * group's stage name with the field information's heading, then the
 * monsters of the group in that season (gFieldGuideTables: up to seven
 * pairs of a condition and a monster kind) that have come
 * (Monster_IsAvailable) and, online, whose condition is met (Cond_Check;
 * 0xFFFF none), and last the three specialty items of the season and the
 * time of day (gFieldGuideLists, the second set in AREA_ELDER_HALL).
 * Forms kept for the match: i, the map groups' counter, then holds the
 * time of day (desk->pm) until the specialty items; j holds the centre's x
 * before it counts the monsters; y keeps the monster list's top; listX and
 * labelX each hold another x first (the title's centred x, and the
 * title's x).
 */
void QuestDesk_DrawFieldGuide(s16 x, s16 y)
{
    s32 labelX;
    u16 *monsters;
    u16 *items;
    s32 i;
    char line[0x40];
    QuestDesk *desk;
    s16 itemY;
    s32 group;
    s16 listX;
    s16 rowY;
    s32 left;
    s32 j;
    s16 tabs[7];
    s8 season;

    desk = (QuestDesk *)gMenuWork;
    memset(tabs, 0, 0xE);
    for (i = 0; i < desk->groupCount; i++) {
        tabs[i] = gQuestMapGroups[i];
        if (tabs[i] == 0xD) {
            tabs[i] = 2;
        }
        if (i == desk->sel) {
            tabs[i] = -tabs[i];
        }
    }
    group = gQuestMapGroups[desk->sel];
    if (group == -99) {
        Menu_DrawQuestSheet(x, y + 10, QUEST_SHEET_W, QUEST_SHEET_H, tabs, desk->sel, -1, -2);
    } else {
        if (group == 0xD) {
            group = 2;
        }
        Menu_DrawQuestSheet(x, y, QUEST_SHEET_W, QUEST_SHEET_H, tabs, desk->sel, gMapGroupMapIds[group][0], desk->season);
    }
    Font_SetSpacing(0x12, 0x12);
    rowY = y + 0x28;
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    i = desk->pm;
    season = desk->season;
    sprintf(line, "%s%s%s", gSeasonNames[season], gLobbyWords[LOBBY_WORD_MID_DOT], gDaytimeNames[i]);
    left = x;
    j = left + 0xA0;
    listX = Text_CenterX(line, j, 0x12);
    labelX = listX - 0x12;
    Font_SetPos(labelX, rowY);
    Text_PrintfColored(gPriceInfoTexts[PRICE_INFO_BRACKET_FMT], line);
    sprintf(line, "%s%s", gStageNames[group], gQuestSheetTexts[QUEST_SHEET_TEXT_FIELD_INFO]);
    Font_SetPos(Text_CenterX(line, j, 0x12), rowY += 0x14);
    Text_PrintfColored(line);
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    labelX = left + 0x28;
    Font_SetPos(labelX, rowY += 0x1E);
    Text_PrintfColored(gQuestSheetTexts[QUEST_SHEET_TEXT_ECOLOGY]);
    Font_SetColor(QUEST_SHEET_INK);
    listX = labelX;
    monsters = gFieldGuideTables[group][season];
    y = rowY;
    for (j = 0; j < 7; j++) {
        if (monsters[1] != 0) {
            if (((monsters[0] != 0xFFFF && Cond_Check(monsters[0]) == 1) || Game_IsOnline() == 0) && Monster_IsAvailable(monsters[1]) == 1) {
                Font_SetPos(listX, rowY += 0x14);
                Text_PrintfColored(gMonsterNames[monsters[1]]);
            }
            monsters += 2;
        }
    }
    itemY = y + 0x8C;
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    Font_SetPos(labelX, itemY += 0x1E);
    Text_PrintfColored(gQuestSheetTexts[QUEST_SHEET_TEXT_SPECIALTY_ITEMS]);
    if (gScene.area == AREA_ELDER_HALL) {
        items = gFieldGuideLists[1][desk->sel];
    } else {
        items = gFieldGuideLists[0][desk->sel];
    }
    items += desk->season * 6;
    if (i == 1) {
        items += 3;
    }
    Font_SetColor(QUEST_SHEET_INK);
    Font_SetPos(listX, itemY += 0x14);
    Text_PrintfColored(gItemNames[*items]);
    items++;
    Font_SetPos(listX, itemY += 0x14);
    Text_PrintfColored(gItemNames[*items]);
    items++;
    Font_SetPos(listX, itemY + 0x14);
    Text_PrintfColored(gItemNames[*items]);
}

void Text_SetSize(s32 size);
void Menu_DrawDecoBox(s16 x, s16 y, s16 w, s16 h, s8 kind);
void QuestBoard_Draw(s16 x, s16 y);
void QuestDesk_DrawRecruitForm(s16 x, s16 y);
void QuestDesk_DrawFieldGuide(s16 x, s16 y);
void QuestDesk_DrawElderInfo(s16 x, s16 y);
void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);

/*
 * The quest desk's first draw (task is the menu's task): by the step, the
 * desk window's box as it slides in, the quest sheet (QuestBoard_Draw,
 * moved by the stamps' shakes), the field guide, the forecast or the
 * recruit form; nothing in the other steps. Over a shown sheet, the label
 * bar and the money.
 */
void QuestDesk_Draw(MenuTask *task)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;
    QuestDeskCancel *cancel = &desk->cancel;

    Text_SetSize(task->textSize);
    switch (desk->step) {
    case QUEST_DESK_STEP_SLIDE_IN:
    case QUEST_DESK_STEP_TOP_MENU:
    case QUEST_DESK_STEP_CHOOSE_LIST:
        Menu_DrawDecoBox(desk->win.pos, QUEST_DESK_WIN_Y, QUEST_SHEET_W, QUEST_SHEET_H, 3);
        break;
    case QUEST_DESK_STEP_SLIDE_OUT:
    case QUEST_DESK_STEP_SLIDE_BACK:
        switch (desk->entries[desk->entrySel]) {
        case QUEST_DESK_BY_TYPE:
        case QUEST_DESK_BY_MAP:
        case QUEST_DESK_BOARD:
            /* the y adds the wanted stamp's x shake too, as the game's code does */
            QuestBoard_Draw(desk->stamp.dx + QUEST_DESK_WIN_X + desk->wantedStamp.dx,
                          desk->stamp.dy + QUEST_DESK_WIN_Y + desk->wantedStamp.dx);
            break;
        case QUEST_DESK_GUIDE:
            QuestDesk_DrawFieldGuide(QUEST_DESK_WIN_X, QUEST_DESK_WIN_Y);
            break;
        case QUEST_DESK_FORECAST:
            QuestDesk_DrawElderInfo(QUEST_DESK_WIN_X, QUEST_DESK_WIN_Y);
            break;
        }
        break;
    case QUEST_DESK_STEP_CHOOSE_QUEST:
    case QUEST_DESK_STEP_CHECK_FEE:
    case QUEST_DESK_STEP_WANTED_STAMP:
    case QUEST_DESK_STEP_STAMP:
    case QUEST_DESK_STEP_DROP:
        QuestBoard_Draw(desk->stamp.dx + QUEST_DESK_WIN_X + desk->wantedStamp.dx,
                      desk->stamp.dy + QUEST_DESK_WIN_Y + desk->wantedStamp.dx);
        break;
    case QUEST_DESK_STEP_POST:
        if (desk->subStep == 0) {
            QuestDesk_DrawRecruitForm(QUEST_DESK_WIN_X, QUEST_DESK_WIN_Y);
            break;
        }
        return;
    case QUEST_DESK_STEP_WAIT_TALK:
    case QUEST_DESK_STEP_CLOSE:
        if (desk->quest != NULL) {
            QuestBoard_Draw(QUEST_DESK_WIN_X, QUEST_DESK_WIN_Y);
            break;
        }
        return;
    case QUEST_DESK_STEP_GUIDE:
        QuestDesk_DrawFieldGuide(QUEST_DESK_WIN_X, QUEST_DESK_WIN_Y);
        break;
    case QUEST_DESK_STEP_FORECAST:
        QuestDesk_DrawElderInfo(QUEST_DESK_WIN_X, QUEST_DESK_WIN_Y);
        break;
    default:
        return;
    }
    if (desk->unk6D != 1 && cancel->hideSheet != 1) {
        Font_SetSpacing(0x16, 0x16);
        Menu_DrawLabelBar(QUEST_DESK_WIN_X, 0x12, 0xB4, 0xFFB0B000, gQuestSheetTexts[QUEST_SHEET_TEXT_FREE_QUEST], 2, 1);
        Menu_DrawMoney(0x1E0, 0x10, 0);
    }
}

s32 LobbyWorld_GetSeason(void);
void QuestDesk_DrawFieldInfo(s16 x, s16 y);
void QuestDesk_DrawIndexList(s16 x, s16 y);
s32 QuestDesk_IsWanted(u8 *p);
void PriceInfo_DrawDetails(s16 x, s16 y, void *list, u8 mode0, s8 n);

/*
 * The quest desk's second draw (task is the menu's task): the desk window
 * at its sliding x (desk->win.pos), with the entries or, while a list is
 * chosen by map group or by type, the lists; while the quest sheet of a
 * wanted quest is up, the card of the season's first wanted entry
 * (PriceInfo_DrawDetails): gTownQuestLists.b in AREA_ELDER_HALL, .a in
 * AREA_TAVERN or offline.
 */
void QuestDesk_Draw2(MenuTask *task)
{
    QuestDesk *desk = (QuestDesk *)gMenuWork;
    TownQuestLists *lists = &gTownQuestLists;
    TownEntry *wanted;
    s32 season = LobbyWorld_GetSeason();
    s16 x;

    Text_SetSize(task->textSize);
    switch (desk->step) {
    case QUEST_DESK_STEP_SLIDE_IN:
    case QUEST_DESK_STEP_TOP_MENU:
        QuestDesk_DrawFieldInfo(desk->win.pos, QUEST_DESK_WIN_Y);
        break;
    case QUEST_DESK_STEP_CHOOSE_LIST:
    case QUEST_DESK_STEP_SLIDE_OUT:
    case QUEST_DESK_STEP_SLIDE_BACK:
        x = desk->win.pos;
        switch (desk->entries[desk->entrySel]) {
        case QUEST_DESK_BY_TYPE:
        case QUEST_DESK_BY_MAP:
            QuestDesk_DrawIndexList(x, QUEST_DESK_WIN_Y);
            break;
        default:
            QuestDesk_DrawFieldInfo(x, QUEST_DESK_WIN_Y);
            break;
        }
        break;
    case QUEST_DESK_STEP_CHOOSE_QUEST:
        switch (desk->subStep) {
        case 1:
        case 2:
        case 3:
            if ((u8 *)desk->quest != NULL && QuestDesk_IsWanted((u8 *)desk->quest) != 0) {
                if (gScene.area == AREA_ELDER_HALL) {
                    wanted = lists->b[season];
                } else if (gScene.area == AREA_TAVERN || Game_IsOnline() == 0) {
                    wanted = lists->a[season];
                }
                /* the game reads a register it never set: wanted is s1, still the caller's, online outside AREA_TAVERN and AREA_ELDER_HALL */
                PriceInfo_DrawDetails(desk->win.pos, 0x44, wanted, 0, 0);
            }
            break;
        }
        break;
    }
}

/* The field guide's data. The empty lists among gFieldGuideTables' entries are in the zeroed .bss. */
extern u32 gFieldGuideTables_3_0[];
extern u32 gFieldGuideTables_11_1[];
extern u32 gFieldGuideTables_13_0[];
extern u32 gFieldGuideTables_13_1[];
extern u32 gFieldGuideTables_13_2[];
u32 gFieldGuideTables_6_0[8] = {
#include "lobby/007ED460.inc"
};
u32 gFieldGuideTables_6_1[8] = {
#include "lobby/007ED480.inc"
};
u32 gFieldGuideTables_6_2[6] = {
#include "lobby/007ED4A0.inc"
};
u16 *gFieldGuideTables_6[3] = {
    (u16 *)gFieldGuideTables_6_0, (u16 *)gFieldGuideTables_6_1, (u16 *)gFieldGuideTables_6_2,
};
u32 gFieldGuideTables_3_1[8] = {
#include "lobby/007ED4D0.inc"
};
u32 gFieldGuideTables_3_2[8] = {
#include "lobby/007ED4F0.inc"
};
u16 *gFieldGuideTables_3[4] = {
    (u16 *)gFieldGuideTables_3_0, (u16 *)gFieldGuideTables_3_1, (u16 *)gFieldGuideTables_3_2, NULL,
};
u32 gFieldGuideTables_4_0[8] = {
#include "lobby/007ED520.inc"
};
u32 gFieldGuideTables_4_1[8] = {
#include "lobby/007ED540.inc"
};
u32 gFieldGuideTables_4_2[6] = {
#include "lobby/007ED560.inc"
};
u16 *gFieldGuideTables_4[3] = {
    (u16 *)gFieldGuideTables_4_0, (u16 *)gFieldGuideTables_4_1, (u16 *)gFieldGuideTables_4_2,
};
u32 gFieldGuideTables_11_0[4] = {
#include "lobby/007ED590.inc"
};
u32 gFieldGuideTables_11_2[4] = {
#include "lobby/007ED5A0.inc"
};
u16 *gFieldGuideTables_11[4] = {
    (u16 *)gFieldGuideTables_11_0, (u16 *)gFieldGuideTables_11_1, (u16 *)gFieldGuideTables_11_2, NULL,
};
u32 gFieldGuideTables_5_0[6] = {
#include "lobby/007ED5C0.inc"
};
u32 gFieldGuideTables_5_1[3] = {
#include "lobby/007ED5D8.inc"
};
u32 gFieldGuideTables_5_2[3] = {
#include "lobby/007ED5E8.inc"
};
u16 *gFieldGuideTables_5[3] = {
    (u16 *)gFieldGuideTables_5_0, (u16 *)gFieldGuideTables_5_1, (u16 *)gFieldGuideTables_5_2,
};
u16 *gFieldGuideTables_13[3] = {
    (u16 *)gFieldGuideTables_13_0, (u16 *)gFieldGuideTables_13_1, (u16 *)gFieldGuideTables_13_2,
};
u32 gFieldGuideTables_2_0[2] = {
#include "lobby/007ED618.inc"
};
u32 gFieldGuideTables_2_1[2] = {
#include "lobby/007ED620.inc"
};
u32 gFieldGuideTables_2_2[3] = {
#include "lobby/007ED628.inc"
};
u16 *gFieldGuideTables_2[3] = {
    (u16 *)gFieldGuideTables_2_0, (u16 *)gFieldGuideTables_2_1, (u16 *)gFieldGuideTables_2_2,
};
u16* *gFieldGuideTables[16] = {
    NULL, NULL, gFieldGuideTables_2, gFieldGuideTables_3,
    gFieldGuideTables_4, gFieldGuideTables_5, gFieldGuideTables_6, NULL,
    NULL, NULL, NULL, gFieldGuideTables_11,
    NULL, gFieldGuideTables_13, NULL, NULL,
};
u16 gFieldGuideLists_0[24] = {
#include "lobby/007ED690.inc"
};
u16 gFieldGuideLists_1[24] = {
#include "lobby/007ED6C0.inc"
};
u16 gFieldGuideLists_2[24] = {
#include "lobby/007ED6F0.inc"
};
u16 gFieldGuideLists_3[24] = {
#include "lobby/007ED720.inc"
};
u16 gFieldGuideLists_4[24] = {
#include "lobby/007ED750.inc"
};
u16 gFieldGuideLists_5[24] = {
#include "lobby/007ED780.inc"
};
u16 gFieldGuideLists_6[24] = {
#include "lobby/007ED7B0.inc"
};
u16 gFieldGuideLists_7[24] = {
#include "lobby/007ED7E0.inc"
};
u16 gFieldGuideLists_8[24] = {
#include "lobby/007ED810.inc"
};
u16 gFieldGuideLists_9[24] = {
#include "lobby/007ED840.inc"
};
u16 gFieldGuideLists_10[24] = {
#include "lobby/007ED870.inc"
};
u16 gFieldGuideLists_11[24] = {
#include "lobby/007ED8A0.inc"
};
u16 *gFieldGuideLists[2][6] = {
    { gFieldGuideLists_0, gFieldGuideLists_1, gFieldGuideLists_2, gFieldGuideLists_3,
      gFieldGuideLists_4, gFieldGuideLists_5 },
    { gFieldGuideLists_6, gFieldGuideLists_7,
      gFieldGuideLists_8, gFieldGuideLists_9, gFieldGuideLists_10, gFieldGuideLists_11 },
};
s16 gQuestMapGroups[16] = {
#include "lobby/007ED900.inc"
};
u8 gMapGroupMapIds[24][2] = {
#include "lobby/007ED920.inc"
};
