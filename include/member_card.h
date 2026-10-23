#ifndef MEMBER_CARD_H
#define MEMBER_CARD_H

#include "common.h"
#include "item.h"
#include "lobby_state.h"

/*
 * The lobby's member card: a member's hunter card on its profile or
 * equipment page (MemberCard_Draw), with the friend registration line under
 * it (MemberCard_DrawFriendReg); MemberCard_DrawStatus and
 * MemberCard_DrawFriendLimit draw two of its profile's lines. Also a
 * member's card (LobbyMember_GetCard), its equipment put on its player
 * (LobbyMember_LoadEquip), and what a piece of equipment sells for
 * (Equip_GetSellPrice).
 */

s32 MemberCard_Draw(u8 pl, u8 page);
void MemberCard_DrawFriendReg(s16 y, u8 textIndex);
HunterCard *LobbyMember_GetCard(u8 member);
void LobbyMember_LoadEquip(u8 pl);
s32 Equip_GetSellPrice(EquipItem *item);
void MemberCard_DrawFriendLimit(HunterCard *card, s32 x, s32 y);
void MemberCard_DrawStatus(HunterCard *card, s32 x, s32 y);

#endif
