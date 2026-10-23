#ifndef TOWN_GUIDE_H
#define TOWN_GUIDE_H

#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"

/*
 * The town guide (TOWN_MODE_TOWN_GUIDE): a resident who explains the town
 * by groups, topics and entries, each entry a message the guide says or
 * pages of text. The guide's talk script opens it (TownGuide_Open).
 */

#define GUIDE_ROWS_PER_PAGE 10  /* the rows of a topic or entry list's page (GuideMenu.topics, .entries) */

/* One entry of a topic: shown when cond (unless -1) or condAlt holds. */
typedef struct GuideEntry {
    /* 0x00 */ s32 cond; /* -1: none */
    /* 0x04 */ u16 condAlt;
    /* 0x06 */ u8 pad6[2];
    /* 0x08 */ s32 msg; /* groups 0 and 2: a message; group 1: a page count */
    /* 0x0C */ s32 firstPage; /* group 1: the first page (gGuidePageTexts) */
    /* 0x10 */ char *title; /* NULL: end of the list */
} GuideEntry;

/* The town guide's work area (gMenuWork while TOWN_MODE_TOWN_GUIDE runs). */
typedef struct GuideMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ char *topics[GUIDE_ROWS_PER_PAGE];
    /* 0x30 */ GuideEntry *entries[GUIDE_ROWS_PER_PAGE];
    /* 0x58 */ s16 group;
    /* 0x5A */ s16 topicPage;
    /* 0x5C */ s16 topicPages;
    /* 0x5E */ s16 topicCursor;
    /* 0x60 */ s16 topicCount;
    /* 0x62 */ s16 entryPage;
    /* 0x64 */ s16 entryPages;
    /* 0x66 */ s16 entryCursor;
    /* 0x68 */ s16 entryCount;
    /* 0x6A */ s16 page;
    /* 0x6C */ s16 pageCount;
    /* 0x6E */ s8 state;
    /* 0x6F */ u8 pad6F;
    /* 0x70 */ char *title; /* the chosen topic's (or entry's) title */
} GuideMenu;

void TownGuide_Open(void);
void TownGuide_Close(void);
void TownGuide_Update(void);
s32 TownGuide_PageInput(GuideMenu *guide, u16 pad);
s32 TownGuide_EntryInput(GuideMenu *guide, u16 pad);
s32 TownGuide_TopicInput(GuideMenu *guide, u16 pad);
void TownGuide_LoadEntries(GuideMenu *guide, s16 page);
void TownGuide_LoadTopics(GuideMenu *guide, s16 page);
s32 TownGuide_GroupInput(GuideMenu *guide, u16 pad);
void TownGuide_DrawTopics(GuideMenu *guide, s16 x, s16 y);
void TownGuide_DrawEntries(GuideMenu *guide, s16 x, s16 y);
void TownGuide_DrawPage(GuideMenu *guide, s16 x, s16 y);
void TownGuide_Draw(MenuTask *task);

#endif
