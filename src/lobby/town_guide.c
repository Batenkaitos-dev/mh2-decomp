/*
 * The town guide (TOWN_MODE_TOWN_GUIDE): a resident of the town who explains
 * it. The hunter picks one of three groups (enum GuideGroup), then a topic,
 * then one of the topic's entries; an entry shows only when its condition
 * holds (Cond_Check). In two groups the guide says the entry's message; in
 * the third the entry is pages of text, some with a picture.
 *
 * The guide's state is a GuideMenu in gMenuWork. Its tables (the topics'
 * titles and each topic's GuideEntry list) are the file's initialised data,
 * and the texts they point at are its read-only data. The guide's talk
 * script opens it through TownGuide_Open.
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "scene.h"
#include "player.h"
#include "lobby_menu.h"
#include "menu_task.h"
#include "pad.h"
#include "lobby_forge.h"
#include "lobby_labels.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

void LobbySe_Play(s32 se);

s32 WinSlide_Step(WinSlide *, s16 from, s16 to);
void *memset(void *dst, s32 c, u32 n);

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
    /* 0x08 */ char *topics[10];
    /* 0x30 */ GuideEntry *entries[10];
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

#define GUIDE_ROWS_PER_PAGE 10  /* the rows of a topic or entry list's page (GuideMenu.topics, .entries) */

/* GuideMenu.group: the guide's three groups of topics, named by their tables */
enum GuideGroup {
    GUIDE_GROUP_BASICS = 0,     /* gGuideBasicsTopics: each entry is a message the guide says */
    GUIDE_GROUP_PLACES = 1,     /* gGuidePlaceTopics: each entry is pages of text */
    GUIDE_GROUP_DRAGON = 2,     /* gGuideDragonTopics: each entry is a message the guide says */
    GUIDE_GROUP_COUNT = 3
};

/* GuideMenu.state */
enum GuideState {
    GUIDE_STATE_GROUPS = 0,     /* the group list */
    GUIDE_STATE_SLIDE_IN = 1,   /* the list window slides in */
    GUIDE_STATE_TOPICS = 2,     /* the topic list */
    GUIDE_STATE_ENTRIES = 3,    /* the entry list */
    GUIDE_STATE_TALK = 4,       /* the guide says the entry's message */
    GUIDE_STATE_PAGES = 5,      /* the entry's pages */
    GUIDE_STATE_FAREWELL = 6    /* the guide's parting words, then the guide closes */
};

/* TownGuide_DrawHeading's style */
enum GuideHeading {
    GUIDE_HEADING_TOPICS = 0,   /* from the left, the topic list's rule glyph */
    GUIDE_HEADING_ENTRIES = 1   /* centred, the entry list's rule glyph */
};

extern u8 gMenuWork[];
extern char *gGuideBasicsTopics[];
extern char *gGuidePlaceTopics[];
extern char *gGuideDragonTopics[];
extern GuideEntry *gGuideBasicsEntries[];
extern GuideEntry *gGuidePlaceEntries[];
extern GuideEntry *gGuideDragonEntries[];
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Menu_WrapLeftRight(s32 cursor, s32 count);
s32 Menu_CountPages(s16 n, s16 per);
s32 Cond_Check(u16 id);
s32 TownGuide_PageInput(GuideMenu *m, u16 pad);
s32 TownGuide_EntryInput(GuideMenu *m, u16 pad);
s32 TownGuide_TopicInput(GuideMenu *m, u16 pad);
void TownGuide_LoadEntries(GuideMenu *m, s16 page);
void TownGuide_LoadTopics(GuideMenu *m, s16 page);
s32 TownGuide_GroupInput(GuideMenu *m, u16 pad);

u16 LobbyPad_Get(s32);
void Chat_SetWindowMode(s8 a, s8 b);
void Talk_Say(s32 msg, s8);
s32 Talk_Update(void);
void TownGuide_Close(void);
void CamFocus_End(void);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void TownGuide_Draw(MenuTask *task);
extern s8 gMenuInputLock;
extern s32 gTownMenuOpen;
extern s32 gTownMode;
/* The guide's titles, topic names and page texts, which its tables point at. */
#pragma rodata on
char D_0082CB98[0xC] = {
#include "lobby/0082CB98.inc"
};
char D_0082CBA8[0xC] = {
#include "lobby/0082CBA8.inc"
};
char D_0082CBC0[0x18] = {
#include "lobby/0082CBC0.inc"
};
char D_0082CBD8[13] = {
#include "lobby/0082CBD8.inc"
};
char D_0082CBE8[0xC] = {
#include "lobby/0082CBE8.inc"
};
char D_0082CBF8[0xC] = {
#include "lobby/0082CBF8.inc"
};
char D_0082CC08[15] = {
#include "lobby/0082CC08.inc"
};
char D_0082CC18[0xC] = {
#include "lobby/0082CC18.inc"
};
char D_0082CC30[0x20] = {
#include "lobby/0082CC30.inc"
};
char D_0082CC50[0x20] = {
#include "lobby/0082CC50.inc"
};
char D_0082CC70[0x18] = {
#include "lobby/0082CC70.inc"
};
char D_0082CC88[0xC] = {
#include "lobby/0082CC88.inc"
};
char D_0082CC98[0xC] = {
#include "lobby/0082CC98.inc"
};
char D_0082CCA8[0xC] = {
#include "lobby/0082CCA8.inc"
};
char D_0082CCB8[0xC] = {
#include "lobby/0082CCB8.inc"
};
char D_0082CCD0[0x20] = {
#include "lobby/0082CCD0.inc"
};
char D_0082CCF0[0x18] = {
#include "lobby/0082CCF0.inc"
};
char D_0082CD08[0xC] = {
#include "lobby/0082CD08.inc"
};
char D_0082CD18[0xC] = {
#include "lobby/0082CD18.inc"
};
char D_0082CD30[0x18] = {
#include "lobby/0082CD30.inc"
};
char D_0082CD48[0xC] = {
#include "lobby/0082CD48.inc"
};
char D_0082CD58[0xC] = {
#include "lobby/0082CD58.inc"
};
char D_0082CD68[0xC] = {
#include "lobby/0082CD68.inc"
};
char D_0082CD78[0xC] = {
#include "lobby/0082CD78.inc"
};
char D_0082CD88[0xC] = {
#include "lobby/0082CD88.inc"
};
char D_0082CD98[0xC] = {
#include "lobby/0082CD98.inc"
};
char D_0082CDB0[0x20] = {
#include "lobby/0082CDB0.inc"
};
char D_0082CDD0[0x18] = {
#include "lobby/0082CDD0.inc"
};
char D_0082CDE8[15] = {
#include "lobby/0082CDE8.inc"
};
char D_0082CDF8[0xC] = {
#include "lobby/0082CDF8.inc"
};
char D_0082CE10[0x18] = {
#include "lobby/0082CE10.inc"
};
char D_0082CE28[0xC] = {
#include "lobby/0082CE28.inc"
};
char D_0082CE38[0xC] = {
#include "lobby/0082CE38.inc"
};
char D_0082CE48[15] = {
#include "lobby/0082CE48.inc"
};
char D_0082CE60[0x20] = {
#include "lobby/0082CE60.inc"
};
char D_0082CE80[0x10] = {
#include "lobby/0082CE80.inc"
};
char D_0082CE90[0x10] = {
#include "lobby/0082CE90.inc"
};
char D_0082CEA0[0x10] = {
#include "lobby/0082CEA0.inc"
};
char D_0082CEB0[0x18] = {
#include "lobby/0082CEB0.inc"
};
char D_0082CEC8[0x8] = {
#include "lobby/0082CEC8.inc"
};
char D_0082CED0[0x10] = {
#include "lobby/0082CED0.inc"
};
char D_0082CEE0[0x10] = {
#include "lobby/0082CEE0.inc"
};
char D_0082CEF0[0x10] = {
#include "lobby/0082CEF0.inc"
};
char D_0082CF00[0x8] = {
#include "lobby/0082CF00.inc"
};
char D_0082CF08[0xC] = {
#include "lobby/0082CF08.inc"
};
char D_0082CF20[0xD0] = {
#include "lobby/0082CF20.inc"
};
char D_0082CFF0[0xA0] = {
#include "lobby/0082CFF0.inc"
};
char D_0082D090[0xB0] = {
#include "lobby/0082D090.inc"
};
char D_0082D140[0xD0] = {
#include "lobby/0082D140.inc"
};
char D_0082D210[0xB0] = {
#include "lobby/0082D210.inc"
};
char D_0082D2C0[0xD0] = {
#include "lobby/0082D2C0.inc"
};
char D_0082D390[0xE0] = {
#include "lobby/0082D390.inc"
};
char D_0082D470[0xF0] = {
#include "lobby/0082D470.inc"
};
char D_0082D560[0xB0] = {
#include "lobby/0082D560.inc"
};
char D_0082D610[0xC0] = {
#include "lobby/0082D610.inc"
};
char D_0082D6D0[0xA0] = {
#include "lobby/0082D6D0.inc"
};
char D_0082D770[0x90] = {
#include "lobby/0082D770.inc"
};
char D_0082D800[0xA0] = {
#include "lobby/0082D800.inc"
};
char D_0082D8A0[0xE0] = {
#include "lobby/0082D8A0.inc"
};
char D_0082D980[0xF0] = {
#include "lobby/0082D980.inc"
};
char D_0082DA70[0xE0] = {
#include "lobby/0082DA70.inc"
};
char D_0082DB50[0xD0] = {
#include "lobby/0082DB50.inc"
};
char D_0082DC20[0xA0] = {
#include "lobby/0082DC20.inc"
};
char D_0082DCC0[0xA0] = {
#include "lobby/0082DCC0.inc"
};
char D_0082DD60[0x90] = {
#include "lobby/0082DD60.inc"
};
char D_0082DDF0[0xA0] = {
#include "lobby/0082DDF0.inc"
};
char D_0082DE90[0x100] = {
#include "lobby/0082DE90.inc"
};
char D_0082DF90[0x90] = {
#include "lobby/0082DF90.inc"
};
char D_0082E020[0xC0] = {
#include "lobby/0082E020.inc"
};
char D_0082E0E0[0xF0] = {
#include "lobby/0082E0E0.inc"
};
char D_0082E1D0[0xC0] = {
#include "lobby/0082E1D0.inc"
};
char D_0082E290[0xB0] = {
#include "lobby/0082E290.inc"
};
char D_0082E340[0xC0] = {
#include "lobby/0082E340.inc"
};
char D_0082E400[0xB0] = {
#include "lobby/0082E400.inc"
};
char D_0082E4B0[0x100] = {
#include "lobby/0082E4B0.inc"
};
char D_0082E5B0[0xA0] = {
#include "lobby/0082E5B0.inc"
};
char D_0082E650[0xE0] = {
#include "lobby/0082E650.inc"
};
char D_0082E730[0xE0] = {
#include "lobby/0082E730.inc"
};
char D_0082E810[0x20] = {
#include "lobby/0082E810.inc"
};
char D_0082E830[0x20] = {
#include "lobby/0082E830.inc"
};
char D_0082E850[0x18] = {
#include "lobby/0082E850.inc"
};
char D_0082E868[0x8] = {
#include "lobby/0082E868.inc"
};
char D_0082E870[0x10] = {
#include "lobby/0082E870.inc"
};
char D_0082E880[0x10] = {
#include "lobby/0082E880.inc"
};
char D_0082E890[0x10] = {
#include "lobby/0082E890.inc"
};
char D_0082E8A0[0x10] = {
#include "lobby/0082E8A0.inc"
};
char D_0082E8B0[0x10] = {
#include "lobby/0082E8B0.inc"
};
char D_0082E8C0[0x18] = {
#include "lobby/0082E8C0.inc"
};
char D_0082E8D8[0xC] = {
#include "lobby/0082E8D8.inc"
};
char D_0082E8E8[0xC] = {
#include "lobby/0082E8E8.inc"
};
#pragma rodata off

/* The town guide's opening and closing are built at the default level. */
#pragma optimization_level reset

/*
 * Opens the town guide (TOWN_MODE_TOWN_GUIDE): clears gMenuWork, resets
 * the lobby menu, sets TownGuide_Draw as the draw callback, locks menu
 * input for 6 frames and plays the confirm sound.
 */
void TownGuide_Open(void)
{
    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_TOWN_GUIDE;
    memset(gMenuWork, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(&TownGuide_Draw, 0, 0, 0);
    gMenuInputLock = 6;
    LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
}

/*
 * Closes the town guide: back to TOWN_MODE_FREE, the draw callback
 * removed, the camera's focus released, and the cancel sound.
 */
void TownGuide_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
}
#pragma optimization_level 4

/*
 * Runs the town guide each frame, a state machine on GuideMenu.state: the
 * group list, the window sliding in, the topic list, the entry list, then
 * either the guide's spoken message (groups 0 and 2) or the entry's pages
 * (group 1). Cancelling the group list makes the guide say his parting
 * words, and the guide closes when they end.
 */
void TownGuide_Update(void)
{
    GuideMenu *guide = (GuideMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (guide->state) {
    case GUIDE_STATE_GROUPS:
        switch (TownGuide_GroupInput(guide, pad)) {
        case MENU_DONE:
            guide->state++;
            WinSlide_Step(&guide->win, 0x280, 0x140);
            guide->topicCursor = 0;
            TownGuide_LoadTopics(guide, 0);
            break;
        case MENU_CANCEL:
            guide->state = GUIDE_STATE_FAREWELL;
            Talk_Say(1, 0); /* the guide's parting words */
            break;
        }
        break;
    case GUIDE_STATE_SLIDE_IN:
        switch (WinSlide_Step(&guide->win, 0x280, 0x140)) {
        case MENU_DONE:
            guide->state++;
            break;
        }
        break;
    case GUIDE_STATE_TOPICS:
        switch (TownGuide_TopicInput(guide, pad)) {
        case MENU_DONE:
            guide->state++;
            guide->entryCursor = 0;
            TownGuide_LoadEntries(guide, 0);
            break;
        case MENU_CANCEL:
            guide->state = GUIDE_STATE_GROUPS;
            break;
        }
        break;
    case GUIDE_STATE_ENTRIES:
        switch (TownGuide_EntryInput(guide, pad)) {
        case MENU_DONE:
            switch (guide->group) {
            case GUIDE_GROUP_BASICS:
            case GUIDE_GROUP_DRAGON:
                guide->state = GUIDE_STATE_TALK;
                Talk_Say(guide->entries[guide->entryCursor]->msg, 0);
                break;
            default:
                guide->state = GUIDE_STATE_PAGES;
                guide->page = 0;
                guide->pageCount = guide->entries[guide->entryCursor]->msg;
                break;
            }
            break;
        case MENU_CANCEL:
            guide->state = GUIDE_STATE_TOPICS;
            break;
        }
        break;
    case GUIDE_STATE_PAGES:
        switch (TownGuide_PageInput(guide, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            guide->state = GUIDE_STATE_ENTRIES;
            break;
        }
        break;
    case GUIDE_STATE_TALK:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            guide->state = GUIDE_STATE_ENTRIES;
            break;
        }
        break;
    case GUIDE_STATE_FAREWELL:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            TownGuide_Close();
            break;
        }
        break;
    }
}

/*
 * The page view's input: cross goes back (returns MENU_DONE, which the
 * caller treats as leaving the pages); otherwise left and right turn the
 * page and it returns MENU_RUNNING.
 */
s32 TownGuide_PageInput(GuideMenu *guide, u16 pad)
{
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_LIST_BACK);
        return MENU_DONE;
    }
    guide->page = Menu_WrapLeftRight(guide->page, guide->pageCount);
    return MENU_RUNNING;
}

/*
 * The entry list's input: circle chooses the entry (MENU_DONE), cross goes
 * back (MENU_CANCEL); left and right turn the list's page when there is
 * more than one, up and down move the cursor. Returns MENU_RUNNING
 * otherwise.
 */
s32 TownGuide_EntryInput(GuideMenu *guide, u16 pad)
{
    if (pad & PAD_CIRCLE) {
        LobbySe_Play(LOBBY_SE_LIST_SELECT);
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_LIST_BACK);
        return MENU_CANCEL;
    }
    if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (guide->entryPages > 1) {
            guide->entryPage = Menu_WrapLeftRight(guide->entryPage, guide->entryPages);
            TownGuide_LoadEntries(guide, guide->entryPage);
        }
    } else {
        guide->entryCursor = Menu_WrapUpDown(guide->entryCursor, guide->entryCount);
    }
    return MENU_RUNNING;
}

/*
 * The topic list's input, as TownGuide_EntryInput does for the entries:
 * circle chooses (MENU_DONE), cross goes back (MENU_CANCEL), left/right
 * turn the page, up/down move the cursor; MENU_RUNNING otherwise.
 */
s32 TownGuide_TopicInput(GuideMenu *guide, u16 pad)
{
    if (pad & PAD_CIRCLE) {
        LobbySe_Play(LOBBY_SE_LIST_SELECT);
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_LIST_BACK);
        return MENU_CANCEL;
    }
    if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (guide->topicPages > 1) {
            guide->topicPage = Menu_WrapLeftRight(guide->topicPage, guide->topicPages);
            TownGuide_LoadTopics(guide, guide->topicPage);
        }
    } else {
        guide->topicCursor = Menu_WrapUpDown(guide->topicCursor, guide->topicCount);
    }
    return MENU_RUNNING;
}

/*
 * Fills the entry list's page `page` (10 rows) from the chosen topic's
 * entries in the current group, counting only entries whose condition
 * holds (cond when it is not -1, or condAlt). Sets the page and page count
 * and keeps the cursor inside the list.
 */
void TownGuide_LoadEntries(GuideMenu *guide, s16 page)
{
    GuideEntry *entry;
    s16 shown;
    s16 first;
    s16 last;

    switch (guide->group) {
    default:
        entry = gGuideBasicsEntries[guide->topicCursor];
        break;
    case GUIDE_GROUP_PLACES:
        entry = gGuidePlaceEntries[guide->topicCursor];
        break;
    case GUIDE_GROUP_DRAGON:
        entry = gGuideDragonEntries[guide->topicCursor];
        break;
    }
    guide->entryCount = 0;
    shown = 0;
    first = page * GUIDE_ROWS_PER_PAGE;
    last = first + GUIDE_ROWS_PER_PAGE - 1;
    for (; entry->title != NULL; entry++) {
        if ((entry->cond != -1 && Cond_Check(entry->cond) == 1) || Cond_Check(entry->condAlt) == 1) {
            if (shown >= first && shown <= last) {
                guide->entries[guide->entryCount] = entry;
                guide->entryCount++;
            }
            shown++;
        }
    }
    guide->entryPage = page;
    guide->entryPages = Menu_CountPages(shown, GUIDE_ROWS_PER_PAGE);
    if (guide->entryCursor >= guide->entryCount) {
        guide->entryCursor = guide->entryCount - 1;
    }
}

/*
 * Fills the topic list's page `page` (10 rows) from the current group's
 * topics (a NULL-ended list of titles), sets the page and page count and
 * keeps the cursor inside the list.
 */
void TownGuide_LoadTopics(GuideMenu *guide, s16 page)
{
    char **topic;
    s16 total;
    s16 first;
    s16 last;

    switch (guide->group) {
    default:
        topic = gGuideBasicsTopics;
        break;
    case GUIDE_GROUP_PLACES:
        topic = gGuidePlaceTopics;
        break;
    case GUIDE_GROUP_DRAGON:
        topic = gGuideDragonTopics;
        break;
    }
    guide->topicCount = 0;
    total = 0;
    first = page * GUIDE_ROWS_PER_PAGE;
    last = first + GUIDE_ROWS_PER_PAGE - 1;
    for (; *topic != NULL; topic++) {
        if (total >= first && total <= last) {
            guide->topics[guide->topicCount] = *topic;
            guide->topicCount++;
        }
        total++;
    }
    guide->topicPage = page;
    guide->topicPages = Menu_CountPages(total, GUIDE_ROWS_PER_PAGE);
    if (guide->topicCursor >= guide->topicCount) {
        guide->topicCursor = guide->topicCount - 1;
    }
}

/*
 * The group list's input: circle chooses the group (MENU_DONE), cross
 * leaves the guide (MENU_CANCEL), up and down move between the three
 * groups (MENU_RUNNING).
 */
s32 TownGuide_GroupInput(GuideMenu *guide, u16 pad)
{
    if (pad & PAD_CIRCLE) {
        LobbySe_Play(LOBBY_SE_LIST_SELECT);
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_LIST_BACK);
        return MENU_CANCEL;
    }
    guide->group = Menu_WrapUpDown(guide->group, GUIDE_GROUP_COUNT);
    return MENU_RUNNING;
}

void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void Text_Draw(char *s);
/*
 * Draws a list's heading at (x, y) and a rule of nine glyphs under it.
 * Style 0 (topic list) prints the heading from the left with one rule
 * glyph; style 1 (entry list) centres it and uses the other glyph. Returns
 * the y where the list's rows start.
 */
s16 TownGuide_DrawHeading(char *title, s16 x, s16 y, s32 style)
{
    s16 left = x;
    s16 textX;
    s32 i;

    Font_SetSpacing(20, 20);
    if (style == GUIDE_HEADING_TOPICS) {
        textX = left + 28;
    } else {
        x += 150;
        textX = Text_CenterX(title, x, 20);
    }
    Font_SetPos(textX, y + 24);
    Text_PrintfColored(title);
    if (style == GUIDE_HEADING_TOPICS) {
        y += 48;
    } else {
        y += 28;
    }
    Font_SetSpacing(14, 14);
    if (style == GUIDE_HEADING_ENTRIES) {
        x = left;
    }
    x += 20;
    for (i = 0; i < 9; i++) {
        Font_SetPos(x, y + 22);
        if (style == GUIDE_HEADING_TOPICS) {
            Text_Draw(gTownGuideHelp[TOWN_GUIDE_ARROWS]);
        } else {
            Text_Draw(gTownGuideHelp[TOWN_GUIDE_ARROWS_DIM]);
        }
        x += 28;
    }
    return y;
}

void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Font_SetColor(s32 color);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
/*
 * Draws the topic list: the group's name bar, the panel, the page number
 * when there is more than one page, the group's heading, and the topics
 * centred one per row with the cursor's arrows on the chosen one, whose
 * title is kept as the entry list's heading.
 */
void TownGuide_DrawTopics(GuideMenu *guide, s16 x, s16 y)
{
    char *topicTitle;
    s16 centerX;
    s16 rowY;
    s16 i;
    s16 textX;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(0x140, 0x12, 0xF0, 0xFFB0B000, gTownGuideTopics[guide->group], 2, 1);
    Icon_Draw(x, y, 0x12C, 0x10E, 0xFFE0D8C8, 2);
    Font_SetColor(0x80101010);
    if (guide->topicPages > 1) {
        Menu_DrawPageNum(x + 0xFA, y + 8, guide->topicPage, guide->topicPages, 1);
    }
    rowY = TownGuide_DrawHeading(gTownGuideHelp[guide->group], x, y, GUIDE_HEADING_TOPICS);
    Font_SetSpacing(18, 18);
    centerX = x + 150;
    rowY += 20;
    for (i = 0; i < guide->topicCount; i++) {
        topicTitle = guide->topics[i];
        textX = Text_CenterX(topicTitle, centerX, 18);
        rowY += 22;
        Font_SetPos(textX, rowY);
        Text_PrintfColored(topicTitle);
        if (guide->topicCursor == i) {
            Menu_DrawArrowsRev(textX, rowY, 18, topicTitle, 0xFF106060);
            guide->title = topicTitle;
        }
    }
}

/*
 * Draws the entry list as TownGuide_DrawTopics does, under the chosen
 * topic's title, with the cursor's arrows on the chosen entry.
 */
void TownGuide_DrawEntries(GuideMenu *guide, s16 x, s16 y)
{
    GuideEntry *entry;
    s16 centerX;
    s16 rowY;
    s16 i;
    s16 textX;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(0x140, 0x12, 0xF0, 0xFFB0B000, gTownGuideTopics[guide->group], 2, 1);
    Icon_Draw(x, y, 0x12C, 0x10E, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    if (guide->entryPages > 1) {
        Menu_DrawPageNum(x + 0xFA, y + 8, guide->entryPage, guide->entryPages, 1);
    }
    rowY = TownGuide_DrawHeading(guide->title, x, y, GUIDE_HEADING_ENTRIES);
    Font_SetSpacing(18, 18);
    centerX = x + 150;
    rowY += 20;
    for (i = 0; i < guide->entryCount; i++) {
        entry = guide->entries[i];
        textX = Text_CenterX(entry->title, centerX, 18);
        rowY += 22;
        Font_SetPos(textX, rowY);
        Text_PrintfColored(entry->title);
        if (guide->entryCursor == i) {
            Menu_DrawArrowsRev(textX, rowY, 18, entry->title, 0xFF106060);
        }
    }
}

void Text_PrintfStyled(s32 a, const char *fmt, ...);
extern char *gGuidePageTexts[];
/*
 * Draws one page of a group-1 entry: the group's name bar, a taller panel,
 * the page number when there are several, the page's text
 * (gGuidePageTexts, the entry's first page plus the page shown) and, on
 * the pages that have one, a picture beside the text.
 */
void TownGuide_DrawPage(GuideMenu *guide, s16 x, s16 y)
{
    GuideEntry *entry;
    s32 textY;
    s32 left;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(0x140, 0x12, 0xF0, 0xFFB0B000, gTownGuideTopics[guide->group], 2, 1);
    Icon_Draw(x, y, 0x12C, 0x154, 0xFFE0D8C8, 1);
    Font_SetColor(0x80101010);
    if (guide->pageCount > 1) {
        Menu_DrawPageNum(x + 0xDC, y + 8, guide->page, guide->pageCount, 1);
    }
    entry = guide->entries[guide->entryCursor];
    Font_SetSpacing(18, 18);
    textY = y + 40;
    left = x;
    Font_SetPos(left + 40, textY);
    Text_PrintfStyled(4, "%s", gGuidePageTexts[entry->firstPage + guide->page]);
    switch (entry->firstPage + guide->page) {
    case 10:
        Icon_Draw(left + 0xAA, textY, 0x78, 0x78, 0xA0000000, 0x25);
        break;
    case 11:
        Icon_Draw(left + 0xA0, textY, 0x87, 0x87, 0xA0000000, 0x22);
        break;
    case 12:
        Icon_Draw(left + 0x78, textY, 0xAA, 0x7F, 0xA0000000, 0x21);
        break;
    case 17:
    case 18:
        Icon_Draw(left + 0xAA, textY, 0x64, 0x96, 0xA0000000, 0x24);
        break;
    case 19:
    case 20:
        Icon_Draw(left + 0xA0, textY, 0x78, 0x78, 0xA0000000, 0x26);
        break;
    }
}

void Text_SetSize(s32 size);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
void TownGuide_DrawTopics(GuideMenu *m, s16 x, s16 y);
void TownGuide_DrawEntries(GuideMenu *m, s16 x, s16 y);
void TownGuide_DrawPage(GuideMenu *m, s16 x, s16 y);
/*
 * The guide's draw callback: sets the task's text size, then draws by the
 * guide's state the group list, the topic list (also while the window
 * slides in), the entry list or the page. Nothing is drawn while the guide
 * speaks.
 */
void TownGuide_Draw(MenuTask *task)
{
    GuideMenu *guide = (GuideMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    switch (guide->state) {
    case GUIDE_STATE_GROUPS:
        Menu_DrawLabelList(0x1C2, gTownGuideTopics, GUIDE_GROUP_COUNT, guide->group, 0xFFB0B000);
        break;
    case GUIDE_STATE_SLIDE_IN:
    case GUIDE_STATE_TOPICS:
        TownGuide_DrawTopics(guide, guide->win.pos, 0x3A);
        break;
    case GUIDE_STATE_ENTRIES:
        TownGuide_DrawEntries(guide, guide->win.pos, 0x3A);
        break;
    case GUIDE_STATE_PAGES:
        TownGuide_DrawPage(guide, guide->win.pos, 0x3A);
        break;
    }
}

/* The file's initialised data, in the game's order */
GuideEntry gGuideBasicsEntries_0[7] = {
#include "lobby/007E84D0.inc"
};
GuideEntry gGuideBasicsEntries_1[4] = {
#include "lobby/007E8560.inc"
};
GuideEntry gGuideBasicsEntries_2[8] = {
#include "lobby/007E85B0.inc"
};
GuideEntry gGuidePlaceEntries_0[5] = {
#include "lobby/007E8650.inc"
};
GuideEntry gGuidePlaceEntries_1[8] = {
#include "lobby/007E86C0.inc"
};
GuideEntry gGuidePlaceEntries_2[2] = {
#include "lobby/007E8760.inc"
};
GuideEntry gGuidePlaceEntries_3[4] = {
#include "lobby/007E8790.inc"
};
GuideEntry gGuidePlaceEntries_4[4] = {
#include "lobby/007E87E0.inc"
};
GuideEntry gGuidePlaceEntries_5[2] = {
#include "lobby/007E8830.inc"
};
GuideEntry gGuidePlaceEntries_6[2] = {
#include "lobby/007E8860.inc"
};
GuideEntry gGuideDragonEntries_0[9] = {
#include "lobby/007E8890.inc"
};
GuideEntry gGuideDragonEntries_1[3] = {
#include "lobby/007E8950.inc"
};
char *gGuidePageTexts[36] = {
    D_0082CF20, D_0082CFF0, D_0082D090, D_0082D140,
    D_0082D210, D_0082D2C0, D_0082D390, D_0082D470,
    D_0082D560, D_0082D610, D_0082D6D0, D_0082D770,
    D_0082D800, D_0082D8A0, D_0082D980, D_0082DA70,
    D_0082DB50, D_0082DC20, D_0082DCC0, D_0082DD60,
    D_0082DDF0, D_0082DE90, D_0082DF90, D_0082E020,
    D_0082E0E0, D_0082E1D0, D_0082E290, D_0082E340,
    D_0082E400, D_0082E4B0, D_0082E5B0, D_0082E650,
    D_0082E730, NULL, NULL, NULL,
};
char *gGuideBasicsTopics[4] = {
    D_0082E810, D_0082E830, D_0082E850, NULL,
};
char *gGuidePlaceTopics[8] = {
    D_0082E868, D_0082E870, D_0082E880, D_0082E890,
    D_0082E8A0, D_0082E8B0, D_0082E8C0, NULL,
};
char *gGuideDragonTopics[4] = {
    D_0082E8D8, D_0082E8E8, NULL, NULL,
};
GuideEntry *gGuideBasicsEntries[4] = {
    gGuideBasicsEntries_0, gGuideBasicsEntries_1, gGuideBasicsEntries_2, NULL,
};
GuideEntry *gGuidePlaceEntries[8] = {
    gGuidePlaceEntries_0, gGuidePlaceEntries_1, gGuidePlaceEntries_2, gGuidePlaceEntries_3,
    gGuidePlaceEntries_4, gGuidePlaceEntries_5, gGuidePlaceEntries_6, NULL,
};
GuideEntry *gGuideDragonEntries[4] = {
    gGuideDragonEntries_0, gGuideDragonEntries_1, NULL, NULL,
};
