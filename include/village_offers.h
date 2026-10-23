#ifndef VILLAGE_OFFERS_H
#define VILLAGE_OFFERS_H

#include "common.h"
#include "lobby_globals.h"

/*
 * The offline village's offers: Offer_Generate deals item requests and
 * quests out to the offer townspeople (gOfferNpcs), the Offer_* and
 * Request_* calls read and finish them, and Offer_AddPoints turns their
 * rewards into points.
 */

void Offer_Generate(void);
u16 Offer_PickQuest(u16 seed);
void Offer_ClearAll(void);
u16 Offer_GetQuest(u8 npcId);
s32 Offer_FindRequest(u8 npcId);
char *Request_Title(s16 npcIdx);
char *Request_Text(s16 npcIdx);
s32 Offer_GetNpcIndex(u16 id);
s32 Request_TakeItemSlot(u16 *taken, u16 seed);
u16 Offer_PickNpc(u16 seed, s8 kind);
s8 Offer_PickKind(u16 seed);
s32 Offer_CountOpen(void);
void Offer_ClearRequest(s32 npcId);
void Offer_CompleteQuest(s32 npcId);
void Offer_AddPoints(s32 npcId, s32 points);
extern OfferNpc gOfferNpcs[15];

#endif
