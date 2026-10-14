#include "common.h"

#include "scene.h"
extern s16 gClockSpinAddDays;
extern u8 gLobbyLevel1Sel;
s32 LobbyTime_IsPm(void);
s32 Game_IsOnline(void);
void Offer_Generate(void);
void LobbyBgm_Play(s32, s32, s32);
void QuestList_Rebuild(void);
void Npc_ClearAll43B(void);

void LobbyWorld_OnAddDays(void)
{
    s32 r = LobbyTime_IsPm();

    gClockSpinAddDays = 0xF;
    if (r == 0) {
        if (Game_IsOnline() == 0) {
            Offer_Generate();
        }
    } else if (Game_IsOnline() == 1 && gLobbyLevel1Sel != 0) {
        if (gScene.area == 0x87) {
            LobbyBgm_Play(0, 0, 0);
        } else {
            LobbyBgm_Play(0, 0, 1);
        }
    }
    QuestList_Rebuild();
    Npc_ClearAll43B();
}
