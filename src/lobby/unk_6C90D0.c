#include "common.h"
#include "lobby_state.h"
#include "em.h"
#include "lobby_menu.h"

/* prototypes */
void LobbySe_Play(s32);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
void *memcpy(void *dst, const void *src, u32 n);
extern WinSlide gKitchenGuestSlide;
extern u8 gKitchenGuestMeal;
extern s32 gTownMode;
extern LobbyMenu *gLobbyMenu;
/* end of prototypes */

void KitchenGuest_OnChosen(u8 *meal)
{
    if (gTownMode == 0x49) {
        *(u8 *)((u8 *)gLobbyMenu + 0x31) = (u8) (*(u8 *)((u8 *)gLobbyMenu + 0x31) + 1);
        *(s16 *)((u8 *)gLobbyMenu + 0x24) = 0x96;
        memcpy(&gKitchenGuestMeal, meal, 0x10);
        WinSlide_Step(&gKitchenGuestSlide, -0x118, 0x3C);
        LobbySe_Play(0x17);
    }
}
