#ifndef LOBBY_SEARCH_H
#define LOBBY_SEARCH_H

/*
 * The area search's work (AreaSearchMenu) and its steps: the hunters of
 * the player's area, asked of the server and listed by pages. Its work
 * holds the search's conditions (lobby_net.h's NetUserSearchCond), so this
 * header includes lobby_net.h, which lobby_menu.h's users don't all take.
 * The name and id searches' work is in lobby_menu.h.
 */
#include "lobby_menu.h"
#include "lobby_net.h"
#include "lobby_state.h"

#define AREA_SEARCH_ROWS_PER_PAGE 10    /* the hunters a results page lists */
#define AREA_SEARCH_PAGES 10            /* the pages of a full answer */
#define AREA_SEARCH_CONDS 4             /* the room for search conditions (none is sent) */
/* The answer's last record (of 100) isn't listed: on the last page a note
 * takes its line (AreaSearch_DrawResults). */
#define AREA_SEARCH_UNLISTED (AREA_SEARCH_PAGES * AREA_SEARCH_ROWS_PER_PAGE - 1)

/* The area search's work (AreaSearch_Open, gMenuWork; AreaSearch_Update). */
typedef struct AreaSearchMenu {
    /* 0x000 */ WinSlide win;          /* the results (AreaSearch_ShowResults slides them in) */
    /* 0x008 */ NetUserSearchCond conds[AREA_SEARCH_CONDS]; /* the search's conditions: none is sent */
    /* 0x118 */ NetUser *rows[AREA_SEARCH_ROWS_PER_PAGE]; /* the page's hunters */
    /* 0x140 */ s16 cursor;            /* the cursor in rows */
    /* 0x142 */ s16 page;
    /* 0x144 */ s16 pageCount;
    /* 0x146 */ s16 count;             /* the hunters on the page */
    /* 0x148 */ s8 step;               /* enum AreaSearchStep */
    /* 0x149 */ s8 inputStep;          /* enum AreaResultsStep */
} AreaSearchMenu;

/* AreaSearchMenu.step */
enum AreaSearchStep {
    AREA_SEARCH_START = 0,          /* asks the server, or shows the answer kept */
    AREA_SEARCH_FINDING = 1,        /* LobbyUser_Search runs */
    AREA_SEARCH_RESULTS = 2,        /* AreaSearch_ResultsInput */
    AREA_SEARCH_HUNTER = 3,         /* a found hunter's info (HunterInfo_Update) */
    AREA_SEARCH_FAILED = 4,         /* the search failed: the server's notice until circle */
};

/* AreaSearchMenu.inputStep */
enum AreaResultsStep {
    AREA_RESULTS_SLIDE_IN = 0,
    AREA_RESULTS_LIST = 1,
};

#endif
