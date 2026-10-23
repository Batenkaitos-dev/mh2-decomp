#ifndef FRIEND_LIST_H
#define FRIEND_LIST_H

#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"

/*
 * The friend list's menu: a page of friends with their connection status,
 * where a row opens the hunter info window or takes the friend off the list
 * (FriendList_Open, FriendList_Update, FriendList_Draw). Opened for a pick,
 * it returns the chosen friend instead (FriendList_GetSelectedId,
 * FriendList_GetSelectedName).
 */

#define FRIEND_PAGE_LEN 10      /* the rows of a page of the list */
#define FRIEND_LIST_OPEN_PICK 1 /* FriendList_Open's argument: the short mail form picks its addressee */

/* The friend list's menu (gFriendListMenu, 132 bytes): FriendList_Open
 * clears and sets it up. */
typedef struct FriendListMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ MenuTask layer[2];   /* the draw tasks: the list, then the hunter info window */
    /* 0x48 */ s32 list[FRIEND_PAGE_LEN]; /* the page's gFriendList indexes */
    /* 0x70 */ s32 checkIdx;        /* the row whose status is being asked of the server */
    /* 0x74 */ s32 pick;            /* FRIEND_LIST_OPEN_PICK: a choice returns to the caller */
    /* 0x78 */ s16 page;
    /* 0x7A */ s16 pageCount;
    /* 0x7C */ s16 cursor;
    /* 0x7E */ s16 count;           /* the rows on the page */
    /* 0x80 */ s16 actionSel;       /* the action window's cursor (enum FriendAction) */
    /* 0x82 */ s8 state;            /* enum FriendListState */
    /* 0x83 */ s8 step;
} FriendListMenu;

void FriendList_Open(s32 mode);
void *FriendList_GetSelectedId(void);
void *FriendList_GetSelectedName(void);
s32 FriendList_Update(u16 pad);
s32 FriendList_HasUnchecked(FriendListMenu *menu);
void FriendList_Remove(s32 idx);
s32 FriendList_RemoveInput(FriendListMenu *menu, u16 pad);
s32 FriendList_ActionInput(FriendListMenu *menu, u16 pad);
s32 FriendList_ListInput(FriendListMenu *menu, u16 pad);
s32 FriendList_CheckStatus(FriendListMenu *menu);
void FriendList_BuildPage(FriendListMenu *menu, s16 page);
void FriendList_DrawActions(FriendListMenu *menu, s16 x, s16 y);
void FriendList_Draw(MenuTask *task);
void FriendList_Draw2(MenuTask *task);

#endif
