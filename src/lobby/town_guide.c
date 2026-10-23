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
#include "bgm.h"
#include "chat_box.h"
#include "camera.h"
#include "text.h"
#include "village_cond.h"
#include "town_guide.h"
#include "font.h"
#include "item_shop.h"
#include "lobby_util.h"
#include "menu_sheets.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "talk.h"
#include "town_frame.h"
#include "libc.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

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

extern s8 gMenuInputLock;
extern s32 gTownMenuOpen;
extern s32 gTownMode;
/* The guide's titles, topic names and page texts, which its tables point at. */
#pragma rodata on
char gGuideBasicsEntries_0_0[12] = {
#include "lobby/0082CB98.inc"
};
char gGuideBasicsEntries_0_1[12] = {
#include "lobby/0082CBA8.inc"
};
char gGuideBasicsEntries_0_2[24] = {
#include "lobby/0082CBC0.inc"
};
char gGuideBasicsEntries_0_3[13] = {
#include "lobby/0082CBD8.inc"
};
char gGuideBasicsEntries_0_4[12] = {
#include "lobby/0082CBE8.inc"
};
char gGuideBasicsEntries_0_5[12] = {
#include "lobby/0082CBF8.inc"
};
char D_0082CC08[15] = {
#include "lobby/0082CC08.inc"
};
char gGuideBasicsEntries_1_1[12] = {
#include "lobby/0082CC18.inc"
};
char gGuideBasicsEntries_1_2[32] = {
#include "lobby/0082CC30.inc"
};
char gGuideBasicsEntries_2_0[32] = {
#include "lobby/0082CC50.inc"
};
char gGuideBasicsEntries_2_1[24] = {
#include "lobby/0082CC70.inc"
};
char gGuideBasicsEntries_2_2[12] = {
#include "lobby/0082CC88.inc"
};
char gGuideBasicsEntries_2_3[12] = {
#include "lobby/0082CC98.inc"
};
char gGuideBasicsEntries_2_4[12] = {
#include "lobby/0082CCA8.inc"
};
char gGuideBasicsEntries_2_5[12] = {
#include "lobby/0082CCB8.inc"
};
char gGuideBasicsEntries_2_6[32] = {
#include "lobby/0082CCD0.inc"
};
char gGuidePlaceEntries_0_0[24] = {
#include "lobby/0082CCF0.inc"
};
char gGuidePlaceEntries_0_1[12] = {
#include "lobby/0082CD08.inc"
};
char gGuidePlaceEntries_0_2[12] = {
#include "lobby/0082CD18.inc"
};
char gGuidePlaceEntries_1_0[24] = {
#include "lobby/0082CD30.inc"
};
char gGuidePlaceEntries_1_1[12] = {
#include "lobby/0082CD48.inc"
};
char gGuidePlaceEntries_1_2[12] = {
#include "lobby/0082CD58.inc"
};
char gGuidePlaceEntries_1_3[12] = {
#include "lobby/0082CD68.inc"
};
char gGuidePlaceEntries_1_4[12] = {
#include "lobby/0082CD78.inc"
};
char gGuidePlaceEntries_1_5[12] = {
#include "lobby/0082CD88.inc"
};
char gGuidePlaceEntries_1_6[12] = {
#include "lobby/0082CD98.inc"
};
char gGuidePlaceEntries_2_0[32] = {
#include "lobby/0082CDB0.inc"
};
char gGuidePlaceEntries_3_0[24] = {
#include "lobby/0082CDD0.inc"
};
char gGuidePlaceEntries_3_1[15] = {
#include "lobby/0082CDE8.inc"
};
char gGuidePlaceEntries_3_2[12] = {
#include "lobby/0082CDF8.inc"
};
char gGuidePlaceEntries_4_0[24] = {
#include "lobby/0082CE10.inc"
};
char gGuidePlaceEntries_4_1[12] = {
#include "lobby/0082CE28.inc"
};
char gGuidePlaceEntries_4_2[12] = {
#include "lobby/0082CE38.inc"
};
char gGuidePlaceEntries_5_0[15] = {
#include "lobby/0082CE48.inc"
};
char gGuidePlaceEntries_6_0[32] = {
#include "lobby/0082CE60.inc"
};
char gGuideDragonEntries_0_0[16] = {
#include "lobby/0082CE80.inc"
};
char gGuideDragonEntries_0_1[16] = {
#include "lobby/0082CE90.inc"
};
char gGuideDragonEntries_0_2[16] = {
#include "lobby/0082CEA0.inc"
};
char gGuideDragonEntries_0_3[24] = {
#include "lobby/0082CEB0.inc"
};
char gGuideDragonEntries_0_4[8] = {
#include "lobby/0082CEC8.inc"
};
char gGuideDragonEntries_0_5[16] = {
#include "lobby/0082CED0.inc"
};
char gGuideDragonEntries_0_6[16] = {
#include "lobby/0082CEE0.inc"
};
char gGuideDragonEntries_0_7[16] = {
#include "lobby/0082CEF0.inc"
};
char gGuideDragonEntries_1_0[8] = {
#include "lobby/0082CF00.inc"
};
char gGuideDragonEntries_1_1[12] = {
#include "lobby/0082CF08.inc"
};
char gGuidePageTexts_0[208] = {
#include "lobby/0082CF20.inc"
};
char gGuidePageTexts_1[160] = {
#include "lobby/0082CFF0.inc"
};
char gGuidePageTexts_2[176] = {
#include "lobby/0082D090.inc"
};
char gGuidePageTexts_3[208] = {
#include "lobby/0082D140.inc"
};
char gGuidePageTexts_4[176] = {
#include "lobby/0082D210.inc"
};
char gGuidePageTexts_5[208] = {
#include "lobby/0082D2C0.inc"
};
char gGuidePageTexts_6[224] = {
#include "lobby/0082D390.inc"
};
char gGuidePageTexts_7[240] = {
#include "lobby/0082D470.inc"
};
char gGuidePageTexts_8[176] = {
#include "lobby/0082D560.inc"
};
char gGuidePageTexts_9[192] = {
#include "lobby/0082D610.inc"
};
char gGuidePageTexts_10[160] = {
#include "lobby/0082D6D0.inc"
};
char gGuidePageTexts_11[144] = {
#include "lobby/0082D770.inc"
};
char gGuidePageTexts_12[160] = {
#include "lobby/0082D800.inc"
};
char gGuidePageTexts_13[224] = {
#include "lobby/0082D8A0.inc"
};
char gGuidePageTexts_14[240] = {
#include "lobby/0082D980.inc"
};
char gGuidePageTexts_15[224] = {
#include "lobby/0082DA70.inc"
};
char gGuidePageTexts_16[208] = {
#include "lobby/0082DB50.inc"
};
char gGuidePageTexts_17[160] = {
#include "lobby/0082DC20.inc"
};
char gGuidePageTexts_18[160] = {
#include "lobby/0082DCC0.inc"
};
char gGuidePageTexts_19[144] = {
#include "lobby/0082DD60.inc"
};
char gGuidePageTexts_20[160] = {
#include "lobby/0082DDF0.inc"
};
char gGuidePageTexts_21[256] = {
#include "lobby/0082DE90.inc"
};
char gGuidePageTexts_22[144] = {
#include "lobby/0082DF90.inc"
};
char gGuidePageTexts_23[192] = {
#include "lobby/0082E020.inc"
};
char gGuidePageTexts_24[240] = {
#include "lobby/0082E0E0.inc"
};
char gGuidePageTexts_25[192] = {
#include "lobby/0082E1D0.inc"
};
char gGuidePageTexts_26[176] = {
#include "lobby/0082E290.inc"
};
char gGuidePageTexts_27[192] = {
#include "lobby/0082E340.inc"
};
char gGuidePageTexts_28[176] = {
#include "lobby/0082E400.inc"
};
char gGuidePageTexts_29[256] = {
#include "lobby/0082E4B0.inc"
};
char gGuidePageTexts_30[160] = {
#include "lobby/0082E5B0.inc"
};
char gGuidePageTexts_31[224] = {
#include "lobby/0082E650.inc"
};
char gGuidePageTexts_32[224] = {
#include "lobby/0082E730.inc"
};
char gGuideBasicsTopics_0[32] = {
#include "lobby/0082E810.inc"
};
char gGuideBasicsTopics_1[32] = {
#include "lobby/0082E830.inc"
};
char gGuideBasicsTopics_2[24] = {
#include "lobby/0082E850.inc"
};
char gGuidePlaceTopics_0[8] = {
#include "lobby/0082E868.inc"
};
char gGuidePlaceTopics_1[16] = {
#include "lobby/0082E870.inc"
};
char gGuidePlaceTopics_2[16] = {
#include "lobby/0082E880.inc"
};
char gGuidePlaceTopics_3[16] = {
#include "lobby/0082E890.inc"
};
char gGuidePlaceTopics_4[16] = {
#include "lobby/0082E8A0.inc"
};
char gGuidePlaceTopics_5[16] = {
#include "lobby/0082E8B0.inc"
};
char gGuidePlaceTopics_6[24] = {
#include "lobby/0082E8C0.inc"
};
char gGuideDragonTopics_0[12] = {
#include "lobby/0082E8D8.inc"
};
char gGuideDragonTopics_1[12] = {
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
            WinSlide_Step(&guide->win, 640, 320);
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
        switch (WinSlide_Step(&guide->win, 640, 320)) {
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
    Menu_DrawLabelBar(320, 18, 240, 0xFFB0B000, gTownGuideTopics[guide->group], 2, 1);
    Icon_Draw(x, y, 300, 270, 0xFFE0D8C8, 2);
    Font_SetColor(0x80101010);
    if (guide->topicPages > 1) {
        Menu_DrawPageNum(x + 250, y + 8, guide->topicPage, guide->topicPages, 1);
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
    Menu_DrawLabelBar(320, 18, 240, 0xFFB0B000, gTownGuideTopics[guide->group], 2, 1);
    Icon_Draw(x, y, 300, 270, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    if (guide->entryPages > 1) {
        Menu_DrawPageNum(x + 250, y + 8, guide->entryPage, guide->entryPages, 1);
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
    Menu_DrawLabelBar(320, 18, 240, 0xFFB0B000, gTownGuideTopics[guide->group], 2, 1);
    Icon_Draw(x, y, 300, 340, 0xFFE0D8C8, 1);
    Font_SetColor(0x80101010);
    if (guide->pageCount > 1) {
        Menu_DrawPageNum(x + 220, y + 8, guide->page, guide->pageCount, 1);
    }
    entry = guide->entries[guide->entryCursor];
    Font_SetSpacing(18, 18);
    textY = y + 40;
    left = x;
    Font_SetPos(left + 40, textY);
    Text_PrintfStyled(4, "%s", gGuidePageTexts[entry->firstPage + guide->page]);
    switch (entry->firstPage + guide->page) {
    case 10:
        Icon_Draw(left + 170, textY, 120, 120, 0xA0000000, 37);
        break;
    case 11:
        Icon_Draw(left + 160, textY, 135, 135, 0xA0000000, 34);
        break;
    case 12:
        Icon_Draw(left + 120, textY, 170, 127, 0xA0000000, 33);
        break;
    case 17:
    case 18:
        Icon_Draw(left + 170, textY, 100, 150, 0xA0000000, 36);
        break;
    case 19:
    case 20:
        Icon_Draw(left + 160, textY, 120, 120, 0xA0000000, 38);
        break;
    }
}

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
        Menu_DrawLabelList(450, gTownGuideTopics, GUIDE_GROUP_COUNT, guide->group, 0xFFB0B000);
        break;
    case GUIDE_STATE_SLIDE_IN:
    case GUIDE_STATE_TOPICS:
        TownGuide_DrawTopics(guide, guide->win.pos, 58);
        break;
    case GUIDE_STATE_ENTRIES:
        TownGuide_DrawEntries(guide, guide->win.pos, 58);
        break;
    case GUIDE_STATE_PAGES:
        TownGuide_DrawPage(guide, guide->win.pos, 58);
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
    gGuidePageTexts_0, gGuidePageTexts_1, gGuidePageTexts_2, gGuidePageTexts_3,
    gGuidePageTexts_4, gGuidePageTexts_5, gGuidePageTexts_6, gGuidePageTexts_7,
    gGuidePageTexts_8, gGuidePageTexts_9, gGuidePageTexts_10, gGuidePageTexts_11,
    gGuidePageTexts_12, gGuidePageTexts_13, gGuidePageTexts_14, gGuidePageTexts_15,
    gGuidePageTexts_16, gGuidePageTexts_17, gGuidePageTexts_18, gGuidePageTexts_19,
    gGuidePageTexts_20, gGuidePageTexts_21, gGuidePageTexts_22, gGuidePageTexts_23,
    gGuidePageTexts_24, gGuidePageTexts_25, gGuidePageTexts_26, gGuidePageTexts_27,
    gGuidePageTexts_28, gGuidePageTexts_29, gGuidePageTexts_30, gGuidePageTexts_31,
    gGuidePageTexts_32, NULL, NULL, NULL,
};
char *gGuideBasicsTopics[4] = {
    gGuideBasicsTopics_0, gGuideBasicsTopics_1, gGuideBasicsTopics_2, NULL,
};
char *gGuidePlaceTopics[8] = {
    gGuidePlaceTopics_0, gGuidePlaceTopics_1, gGuidePlaceTopics_2, gGuidePlaceTopics_3,
    gGuidePlaceTopics_4, gGuidePlaceTopics_5, gGuidePlaceTopics_6, NULL,
};
char *gGuideDragonTopics[4] = {
    gGuideDragonTopics_0, gGuideDragonTopics_1, NULL, NULL,
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
