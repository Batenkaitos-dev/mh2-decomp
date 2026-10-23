#ifndef PETS_H
#define PETS_H

#include "common.h"
#include "em.h"
#include "lobby_menu.h"

/*
 * The town's pets: the Poogie, the care pet of AREA_ONLINE_VILLAGE and the
 * house pet. Talking to a pet opens one of five modes: the Poogie's naming
 * (PoogieNaming_*), its menus in and outside the house and the care pet's
 * (PoogieMenu_*), and the house pet's petting (HousePet_*). A pleasing
 * petting or a food adds to the pet's points for the day, and a full count
 * sends it digging (PoogieDig_*); Poogie_DailyReset starts a new day.
 */

void PoogieNaming_Open(Em *poogie);
void PoogieNaming_Update(void);
void PoogieMenu_Open(Em *poogie);
void PoogieMenu_Update(void);
void PoogieMenu_OpenOutside(Em *pet);
void PoogieMenu_UpdateOutside(void);
void PoogieMenu_OpenCare(Em *pet);
void PoogieMenu_UpdateCare(void);
void HousePet_Open(Em *pet);
void HousePet_Update(void);
void PoogieMenu_Draw(void);
void PoogieMenu_DrawPetFeed(void);
s8 Poogie_UpdatePetting(Em *pet);
s32 Poogie_UpdatePettingDebug(Em *pet);
u8 PoogieMenu_RowsOnPage(PoogieMenu *menu);
void Poogie_ResetPetting(void);
s32 PoogieMenu_GetOutfits(u8 *owned);
void PoogieMenu_DrawOutfitList(void);
void PoogieMenu_BuildFeedList(void);
void PoogieMenu_DrawFeedList(s16 x, s16 y);
void PoogieMenu_BuildOutfitList(void);
s32 PoogieMenu_ReactionDone(void);
u8 PoogieMenu_OutfitAtCursor(void);
u8 PoogieMenu_WornOutfitRow(void);
s32 PoogieMenu_Feed(void);
void PoogieDig_FindZone(f32 *pos, u8 *zone);
void Poogie_DailyReset(void);
u16 PoogieDig_RollItem(u16 unused, u8 kind);
s32 Poogie_IsSeasonFood(u16 item);
s32 PoogieDig_OverLimit(u8 kind);
s16 Poogie_RollFeedLimit(u8 kind);
void Poogie_NotifyMood(u8 kind);
s32 Poogie_AtGreatJoyStart(void);
s16 Poogie_JoyFrames(void);
u8 Pet_AreaKind(void);

#endif
