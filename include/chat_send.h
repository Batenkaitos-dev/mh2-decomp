#ifndef CHAT_SEND_H
#define CHAT_SEND_H

#include "common.h"
#include "quest.h"

/*
 * The chat's sending to the whole lobby (Chat_SendToLobby) and the chat
 * mark beside the village clock, with three small helpers: whether the town
 * runs a mode (Town_IsMode), the quest party's other players as gNetUsers
 * bits (QuestParty_UserMask), and making a quest the quest taken, with its
 * time left and its reward (Quest_SetInfo).
 */

s32 Town_IsMode(s32 mode);
void Chat_SendToLobby(s32 sender, s32 text);
void VillageClock_DrawChatMark(s32 x, s32 y);
u16 QuestParty_UserMask(void);
void Quest_SetInfo(QuestInfo *quest);

#endif
