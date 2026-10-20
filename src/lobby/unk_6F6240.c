/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

void Offer_Generate(void);
void Offer_ClearAll(void);
void QuestList_Rebuild(void);
void Npc_ClearAllTalked(void);
s32 LobbyTime_IsPm(void);
s32 LobbyWorld_GetSeason(void);
s32 Game_IsOnline(void);
void SeasonBanner_CreateOnline(s32, s32); /* unit's own declaration: with the definition's void (u8 a, u8 b) (sub_main/field_objects.c), LobbyWorld_OnNextDay 1 of 34 words differ */
extern s16 gClockSpinNextDay;
extern s32 gTownMode;

void LobbyWorld_OnNextDay(void) {
    s32 season;

    gClockSpinNextDay = 0xF;
    if (Game_IsOnline() == 0) {
        Offer_ClearAll();
        Offer_Generate();
    }
    QuestList_Rebuild();
    Npc_ClearAllTalked();
    if (gTownMode == 0) {
        season = LobbyWorld_GetSeason() & 0xFF;
        SeasonBanner_CreateOnline(season, LobbyTime_IsPm() & 0xFF);
    }
}
