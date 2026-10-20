#ifndef LOBBY_GROUP_BOARD_H
#define LOBBY_GROUP_BOARD_H

/*
 * The group board's work (GroupBoardMenu) and its steps. The board lists the
 * groups (level-1 entries) of the player's area and searches the groups of
 * every land; its work sits in gMenuWork while TOWN_MODE_GROUP_BOARD runs.
 * It embeds the search's request (NetLevel1SearchQuery), so this header
 * includes lobby_net.h, which lobby_menu.h's users don't all take.
 */
#include "lobby_menu.h"
#include "lobby_net.h"

#define GROUP_ROWS_PER_PAGE 10      /* the groups on a page of the area's list or the results */
#define GROUP_TARGET_ANY 999        /* the search's first target: any quest (choosing it clears the marks) */
#define GROUP_MEMBERS_SHOWN 4       /* the members a searched group's details list: its first four users */
#define GROUP_INFO_MEMBERS 0        /* GroupBoardMenu.infoPage: the searched group's members */
#define GROUP_INFO_LOCATION 1       /* its land, area and text */
#define GROUP_SEARCH_TARGET_MAX 4   /* the targets a search may mark */
#define GROUP_TARGETS_PER_PAGE 11   /* the target quests on a page */
#define GROUP_TARGET_PAGES 6        /* the pages of target quests */

/* The group board (TOWN_MODE_GROUP_BOARD; gMenuWork from GroupBoard_Open). Up to
 * 0xB44 it has AreaBoardMenu's layout, because the area board's AreaBoard_Join,
 * AreaBoard_LoadPage and GroupBoard_DrawButtonHints run on it as on an AreaBoardMenu.
 * The board's first question picks GROUP_BOARD_CHOICE_AREA (the groups of the
 * player's area: open a free entry as a new group, or join one) or
 * GROUP_BOARD_CHOICE_SEARCH (search the groups of every land). */
typedef struct GroupBoardMenu {
    /* 0x000 */ WinSlide joinWin;   /* AreaBoardMenu.joinWin: the joined group's members, which AreaBoard_Join slides in */
    /* 0x008 */ u8 roomMembers[0xB20 - 8]; /* AreaBoardMenu's members and its other fields, AreaBoard_Join's */
    /* 0xB20 */ s16 roomPage;       /* AreaBoardMenu.memberPage */
    /* 0xB22 */ s16 roomPages;      /* AreaBoardMenu.memberPages */
    /* 0xB24 */ s16 memberCursor;   /* the cursor over a searched group's members */
    /* 0xB26 */ s16 memberCount;    /* the searched group's members (its used users among the first 4) */
    /* 0xB28 */ s8 memberView;      /* enum MemberViewMode */
    /* 0xB29 */ s8 roomStep;        /* AreaBoardMenu.step, AreaBoard_Join's */
    /* 0xB2A */ s8 joinStep;        /* AreaBoardMenu.joinStep: nonzero keeps GroupBoard_DrawButtonHints' circle from pressing down */
    /* 0xB2B */ u8 padB2B;
    /* 0xB2C */ s8 textSlots[16];   /* the room slots whose recruiting texts the text picker lists (AreaBoard_LoadPage) */
    /* 0xB3C */ s16 textCount;      /* the texts in textSlots */
    union {
        /* 0xB3E */ s16 textCursor; /* a new group's text picker: the cursor in textSlots */
        /* 0xB3E */ s16 searchLimit; /* the search form's limit choice, enum GroupSearchLimit */
    };
    /* 0xB40 */ s16 textPage;       /* the text picker's page (AreaBoard_LoadPage) */
    /* 0xB42 */ s16 textPages;
    /* 0xB44 */ NetLevel1SearchQuery query; /* the search sent (GroupBoard_BuildSearch, LobbyLevel1_Search) */
    /* 0xC68 */ WinSlide listWin;   /* the group list (the area's groups or the search results), from the left */
    /* 0xC70 */ WinSlide panelWin;  /* the right-hand panel: the search form, the new group's form, a group's details */
    /* 0xC78 */ WinShake stampShake; /* the panel's shake when a group is stamped as joined or opened */
    /* 0xC80 */ RoomJoin join;      /* the new group's settings */
    /* 0xC88 */ s32 entryNo;        /* the area's level-1 entry chosen, from 1 (gLobbyLevel1Entries[entryNo - 1]) */
    /* 0xC8C */ s16 formTimer;      /* frames left on the new group's form; at 0 the form is left */
    /* 0xC8E */ s16 rowCursor;      /* the cursor on a page of GROUP_ROWS_PER_PAGE groups */
    /* 0xC90 */ s16 row;            /* the form's row: enum GroupSearchRow or enum GroupNewRow */
    /* 0xC92 */ s16 targetCursor;   /* the cursor in a page of the target quests */
    /* 0xC94 */ s16 choice;         /* the first question's answer, enum GroupBoardChoice */
    /* 0xC96 */ s16 page;           /* the page of groups */
    /* 0xC98 */ s16 pageCount;
    /* 0xC9A */ s16 infoPage;       /* a searched group's details: 0 the members, 1 the location */
    /* 0xC9C */ s16 infoPages;      /* 2 */
    /* 0xC9E */ s16 targetPage;     /* the page of target quests */
    /* 0xCA0 */ s16 targetPages;
    /* 0xCA2 */ s16 targetCount;    /* the target quests listed */
    /* 0xCA4 */ s16 targetsChosen;  /* the targets marked for the search, at most GROUP_SEARCH_TARGET_MAX */
    /* 0xCA6 */ s16 targetsOnPage[GROUP_TARGET_PAGES];
    /* 0xCB2 */ s16 targets[GROUP_TARGET_PAGES][GROUP_TARGETS_PER_PAGE]; /* RoomData.quests indexes; in the search, index + 1, negated when marked, GROUP_TARGET_ANY first */
    /* 0xD36 */ s16 recruitCount;   /* the room's first GROUP_BOARD_SLOTS slots holding a recruiting text (GroupBoard_Open) */
    /* 0xD38 */ s16 stampSeed;      /* Menu_DrawStamp's seed (Rand_Next) */
    /* 0xD3A */ s8 state;           /* enum GroupBoardState */
    /* 0xD3B */ s8 step;            /* the flow's step: enum GroupSearchStep or enum GroupAreaStep */
    /* 0xD3C */ s8 subStep;         /* enum GroupResultsStep, GroupFormStep or GroupNewStep */
    /* 0xD3D */ s8 picker;          /* the new group's form: enum GroupPicker */
    /* 0xD3E */ s8 entryChoice;     /* what the area's entry chosen leads to: enum GroupEntryChoice */
    /* 0xD3F */ s8 showForm;        /* 1: the panel shows the new group's form; 0: the members of the group joined */
    /* 0xD40 */ s8 stamped;         /* the stamp is drawn over the panel */
    /* 0xD41 */ s8 viaSearch;       /* the group was entered from the search results */
} GroupBoardMenu;
typedef char GroupBoardMenu_size_check[(sizeof(GroupBoardMenu) == 0xD44) ? 1 : -1];


/* GroupBoardMenu.state */
enum GroupBoardState {
    GROUP_BOARD_ASK = 0,            /* the first question */
    GROUP_BOARD_AREA = 1,           /* the area's groups (GroupBoard_AreaGroups) */
    GROUP_BOARD_SEARCH = 2,         /* the group search (GroupBoard_Search) */
    GROUP_BOARD_STAMP = 3,          /* a group was joined or opened: the panel shakes under the stamp */
    GROUP_BOARD_ENTER_TOWN = 4,     /* the quest files are downloaded and the town entered */
    GROUP_BOARD_ERROR = 5,          /* the server's error text; circle closes the board */
};

/* GroupBoardMenu.choice: the first question's answer */
enum GroupBoardChoice {
    GROUP_BOARD_CHOICE_AREA = 0,
    GROUP_BOARD_CHOICE_SEARCH = 1,
};

/* GroupBoardMenu.step in GROUP_BOARD_SEARCH (GroupBoard_Search) */
enum GroupSearchStep {
    GROUP_SEARCH_SLIDE_IN = 0,
    GROUP_SEARCH_FORM = 1,          /* GroupBoard_SearchInput */
    GROUP_SEARCH_SEND = 2,          /* LobbyLevel1_Search */
    GROUP_SEARCH_RESULTS = 3,       /* GroupBoard_ResultsStep */
    GROUP_SEARCH_NONE_FOUND = 4,
};

/* GroupBoardMenu.step in GROUP_BOARD_AREA (GroupBoard_AreaGroups) */
enum GroupAreaStep {
    GROUP_AREA_SLIDE_IN = 0,
    GROUP_AREA_LIST = 1,            /* GroupBoard_ListInput */
    GROUP_AREA_NEW_GROUP = 2,       /* GroupBoard_NewGroup */
    GROUP_AREA_LEAVE = 3,           /* the new group is given up (AreaBoard_LeaveEntry) */
    GROUP_AREA_JOIN = 4,            /* AreaBoard_Join */
    GROUP_AREA_NOTICE = 5,          /* a failure's notice; circle returns to the list */
};

/* GroupBoardMenu.subStep in GroupBoard_ResultsStep */
enum GroupResultsStep {
    GROUP_RESULTS_SLIDE_IN = 0,
    GROUP_RESULTS_LIST = 1,
    GROUP_RESULTS_FETCH = 2,        /* LobbyLevel1_FetchInfo: the chosen group's details */
    GROUP_RESULTS_SLIDE_PANEL = 3,
    GROUP_RESULTS_DETAILS = 4,
    GROUP_RESULTS_JOIN_ASK = 5,
    GROUP_RESULTS_ENTER = 6,        /* LobbyLogin_EnterEntry, on the player's own land */
    GROUP_RESULTS_RANK_NOTICE = 7,  /* the group's limit doesn't admit the player's hunter rank */
    GROUP_RESULTS_MEMBERS = 8,      /* the cursor over the members */
    GROUP_RESULTS_HUNTER_CARD = 9,
};

/* GroupBoardMenu.subStep in GroupBoard_SearchInput */
enum GroupFormStep {
    GROUP_FORM_ROWS = 0,
    GROUP_FORM_TARGETS = 1,         /* the target list */
};

/* GroupBoardMenu.subStep in GroupBoard_NewGroup */
enum GroupNewStep {
    GROUP_NEW_OPEN = 0,             /* LobbyStep_Level1Open */
    GROUP_NEW_SLIDE_IN = 1,
    GROUP_NEW_FORM = 2,             /* GroupBoard_NewGroupInput */
    GROUP_NEW_SEND = 3,             /* GroupBoard_SendSettings */
};

/* GroupBoardMenu.picker: what the new group's form has open */
enum GroupPicker {
    GROUP_PICK_NONE = 0,            /* the form's rows */
    GROUP_PICK_TARGET = 1,
    GROUP_PICK_TEXT = 2,
};

/* GroupBoardMenu.row in the search form */
enum GroupSearchRow {
    GROUP_SEARCH_ROW_LIMIT = 0,
    GROUP_SEARCH_ROW_TARGETS = 1,
    GROUP_SEARCH_ROW_START = 2,
    GROUP_SEARCH_ROWS = 3,
};

/* GroupBoardMenu.row in the new group's form */
enum GroupNewRow {
    GROUP_NEW_ROW_LIMIT = 0,
    GROUP_NEW_ROW_TARGET = 1,
    GROUP_NEW_ROW_TEXT = 2,
    GROUP_NEW_ROW_CONFIRM = 3,
    GROUP_NEW_ROWS = 4,
};

/* GroupBoardMenu.entryChoice */
enum GroupEntryChoice {
    GROUP_ENTRY_NEW = 1,            /* a free entry: open it as a new group */
    GROUP_ENTRY_JOIN = 3,           /* an open entry: join it */
};

/* GroupBoardMenu.searchLimit: the search's limit condition (gGroupLimitNewTexts and
 * gGroupLimitExpTexts by this) */
enum GroupSearchLimit {
    GROUP_SEARCH_LIMIT_ANY = 0,
    GROUP_SEARCH_LIMIT_NONE = 1,    /* groups open to everyone */
    GROUP_SEARCH_LIMIT_OWN = 2,     /* groups limited to the player's rank band */
    GROUP_SEARCH_LIMIT_NONE_OWN = 3, /* either */
    GROUP_SEARCH_LIMITS = 4,
};

#endif
