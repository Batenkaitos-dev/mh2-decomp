#ifndef HUNTER_INFO_H
#define HUNTER_INFO_H

#include "common.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "lobby_menu.h"

/*
 * The hunter info window: another hunter's details, opened from the name
 * search and the area board's member lists (HunterInfo_Open, with the
 * HUNTER_INFO_* actions on offer). It shows where the hunter is, the
 * hunter's status and equipment, beside a list of actions: send a short
 * mail, add the hunter to the friend list, or join the hunter's group.
 * HunterInfo_Update runs it and returns a MENU_* result;
 * HunterInfo_DrawIfOpen draws it.
 */

/* The hunter info window's state (gHunterInfoMenu): the hunter's profile as
 * fetched, then the window's own fields. */
typedef struct HunterInfoMenu {
    /* 0x000 */ NetUserProfile user;    /* the hunter's details (LobbyUser_FetchDetails) */
    /* 0x3CC */ u8 userId[LOBBY_USER_ID_LEN]; /* the hunter's user id, as HunterInfo_Open was given it */
    /* 0x3D4 */ s8 acts[HUNTER_ACTS];   /* the actions offered, in list order (enum HunterInfoAct) */
    /* 0x3D7 */ u8 pad3D7;
    /* 0x3D8 */ WinSlide win;           /* the window's slide in from the right */
    /* 0x3E0 */ s32 flags;              /* the HUNTER_INFO_* actions offered */
    /* 0x3E4 */ s32 joinArea;           /* gLobbyLevel0Sel as the join started (Group_Join) */
    /* 0x3E8 */ s16 page;               /* the page shown (enum HunterInfoPage) */
    /* 0x3EA */ s16 sel;                /* the action under the cursor */
    /* 0x3EC */ s16 count;              /* the actions offered */
    /* 0x3EE */ s8 state;               /* enum HunterInfoState */
    /* 0x3EF */ s8 joinStep;            /* Group_Join's step (enum GroupJoinStep) */
} HunterInfoMenu;
typedef char HunterInfoMenu_size_check[(sizeof(HunterInfoMenu) == 1008) ? 1 : -1];

void HunterInfo_Open(void *id, s32 flags);
s32 HunterInfo_Update(u16 pad);
s32 Group_Join(HunterInfoMenu *menu, u16 pad);
s32 HunterInfo_ActionInput(HunterInfoMenu *menu, u16 pad);
void HunterInfo_DrawGroup(HunterInfoMenu *menu, s16 x, s16 y);
void HunterInfo_DrawStatus(HunterInfoMenu *menu, s16 x, s16 y);
void HunterInfo_DrawEquipment(HunterInfoMenu *menu, s16 x, s16 y);
void HunterInfo_DrawActions(HunterInfoMenu *menu, s16 x, s16 y);
void HunterInfo_Draw(HunterInfoMenu *menu, s16 x, s16 y);
void HunterInfo_DrawIfOpen(void);

#endif
