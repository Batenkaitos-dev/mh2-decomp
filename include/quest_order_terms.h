#ifndef QUEST_ORDER_TERMS_H
#define QUEST_ORDER_TERMS_H

#include "common.h"
#include "lobby_menu.h"

/*
 * The quest room's steps, the quest desk's checks and the quest room's
 * settings: settling the pending reward money (Money_PayPending), posting a
 * quest to the server (Quest_PostStep), typing a text with the software
 * keyboard (LobbyMenu_KbdEntry), whether the local hunter meets a quest's
 * order terms (Quest_CheckOrderTerms), and the paid quest's packet and the
 * room's settings (Quest_SendPaid, QuestRoom_ResetSettings).
 */

void Money_PayPending(u8 silent);
s32 Quest_PostStep(MsgQuestPost *req);
s32 LobbyMenu_KbdEntry(void *buf);
void Quest_SendPaid(void);
s32 QuestFlags_GetCategory(u32 *infoFlags);
s32 Quest_CheckOrderTerms(u8 id, u8 ordering);
s32 Quest_IsSpecialId(u16 id);
void QuestRoom_ResetSettings(void);

#endif
