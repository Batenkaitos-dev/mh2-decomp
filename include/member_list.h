#ifndef MEMBER_LIST_H
#define MEMBER_LIST_H

#include "common.h"
#include "menu_task.h"

/*
 * The town's member list, a page of the game menu online
 * (LobbyMember_OpenList, LobbyMember_ListInput, LobbyMember_DrawList), with
 * whether a member is in the local hunter's quest party
 * (LobbyMember_InMyParty) and the town's window of the members' names
 * (LobbyMenu_DrawMemberNames).
 */

s32 LobbyMember_OpenList(void);
u16 LobbyMember_ListInput(u16 pad);
void LobbyMember_DrawList(void);
s32 LobbyMember_InMyParty(s32 idx);
void LobbyMenu_DrawMemberNames(MenuTask *task);

#endif
