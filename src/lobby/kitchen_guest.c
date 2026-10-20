/*
 * The kitchen seen by a guest (TOWN_MODE_KITCHEN_GUEST). Online, when a
 * player cooks for the others in the house (Kitchen_Update), the host's
 * Kitchen_Broadcast sends the diners a town message, and its kinds run
 * this file: the host starts choosing (KitchenGuest_Open: a window saying
 * the host is choosing, with a 60-second countdown), the house's cook goes
 * to the host's spot (KitchenGuest_SendCook), the meal is chosen
 * (KitchenGuest_OnMealChosen: the KitchenMeal is kept in gKitchenGuestMeal,
 * its window slides in, the cooking scene plays and the guest eats the
 * same dish or set course), and the meal ends (KitchenGuest_OnMealEnd).
 *
 * The menu keeps its step and timers in gLobbyMenu (subStep, timer,
 * slideTimer) and the meal's window in gKitchenGuestSlide.
 *
 * KitchenGuest_Draw to KitchenGuest_SendCook are compiled at optimization
 * level 4 and the last two functions at the file's own level, as the
 * game's code is.
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "lobby_npc.h"
#include "lobby_globals.h"
#include "menu_task.h"
#include "scene.h"
#include "save.h"
#include "player.h"
#include "lobby_labels.h"

extern s32 gTownMode;
extern LobbyMenu *gLobbyMenu;
void LobbySe_Play(s32 se);
void KitchenGuest_Close(void);
struct Em *Npc_Find(u8 kind);
void Npc_AdvanceRoute(struct Em *npc);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMenuOpen;
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);

extern WinSlide gKitchenGuestSlide;
void *memcpy(void *dst, const void *src, u32 n);
void CamReq_Cancel(void);
void Npc_SetKitchenRoute(struct Em *obj, s32 kind); /* unit's own declaration: the definition takes a u8 kind; declared with the s32 the caller passes, since with the u8 the caller would narrow the value, which the game's code doesn't */

extern KitchenMeal gKitchenGuestMeal;
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *);
void Font_SetPos(s16, s16);
void Text_Printf(const char *fmt, ...);
void Kitchen_DrawConfirm(s16 x, s16 y, u16 a, u16 b);
void Text_SetSize(s32 a);
#pragma optimization_level 4
/* The guest menu's draw callback: while the host chooses, the waiting
 * window with the seconds left; while the meal's window is in, the dish or
 * set course chosen. */
void KitchenGuest_Draw(MenuTask *task)
{
    Text_SetSize(task->textSize);
    switch (gLobbyMenu->subStep) {
    case KITCHEN_GUEST_STEP_WAIT:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gTownLabels[TOWN_LABEL_HOST_CHOOSING]);
        Font_SetPos(0x1E8, 0x190);
        Text_Printf(gTownLabels[TOWN_LABEL_TIME_LEFT], gLobbyMenu->timer / 30 + 1);
        break;
    case KITCHEN_GUEST_STEP_SLIDE_IN:
    case KITCHEN_GUEST_STEP_SHOW:
    case KITCHEN_GUEST_STEP_SLIDE_OUT:
        Kitchen_DrawConfirm(0x122, gKitchenGuestSlide.pos, gKitchenGuestMeal.firstPick, gKitchenGuestMeal.secondPick);
        break;
    case KITCHEN_GUEST_STEP_COOK:
        break;
    }
}

/* Opens the guest menu when the host starts choosing: 1800 frames (60 s)
 * to wait for the host's meal. */
void KitchenGuest_Open(void)
{
    gTownMode = TOWN_MODE_KITCHEN_GUEST;
    gTownMenuOpen = 1;
    LobbyMenu_SetDraw(&KitchenGuest_Draw, 0, 0, 0);
    LobbyMenu_Init();
    gLobbyMenu->timer = 1800;
}

/* Closes the guest menu: back to TOWN_MODE_FREE, the draw removed and the
 * camera request cancelled. */
void KitchenGuest_Close(void)
{
    gTownMode = TOWN_MODE_FREE;
    gTownMenuOpen = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamReq_Cancel();
}

void CamReq_Kitchen(s32 a);
void NpcScene_Start(s8 a, s32 b, s32 c);
s32 NpcScene_IsDone(void);
void Actor_SetState(void *, s32, s32);
void Kitchen_EatSetCourse(u16 course, u16 rand);
s16 Kitchen_EatDish(s16 season, u16 first, u16 second, u16 rand);

/* The guest menu's frame, by its step: the wait for the host's choice; the
 * meal's window sliding in, then the kitchen camera on the host's spot and
 * the cooking lesson (at the dish's level: the two ingredients' mean
 * rarity, or 4 for a set course); 30 frames of the window shown; the
 * window sliding out; the scene's end, then the player's serve state
 * (0x29); at the serve motion (0x261) the guest eats the dish or the set
 * course and the cook reacts; when the player stands again (state 1) the
 * menu closes. */
void KitchenGuest_Update(void)
{
    struct Em *cook;

    switch (gLobbyMenu->subStep) {
    case KITCHEN_GUEST_STEP_WAIT:
        if (--gLobbyMenu->timer <= 0) {
            KitchenGuest_Close();
        }
        break;
    case KITCHEN_GUEST_STEP_SLIDE_IN:
        if (gLobbyMenu->timer != 0) {
            gLobbyMenu->timer--;
        }
        switch (WinSlide_Step(&gKitchenGuestSlide, -0x118, 0x3C)) {
        case 0:
            gLobbyMenu->subStep++;
            gLobbyMenu->slideTimer = 30;
            CamReq_Kitchen(gKitchenGuestMeal.spot);
            if (gKitchenGuestMeal.firstPick == 0) {
                NpcScene_Start(NPC_SCENE_COOKING, gKitchenGuestMeal.seatMask, 4);
            } else {
                NpcScene_Start(NPC_SCENE_COOKING, gKitchenGuestMeal.seatMask, (gItemData[gKitchenGuestMeal.firstPick].rarity + gItemData[gKitchenGuestMeal.secondPick].rarity + 2) / 2 - 1);
            }
            break;
        }
        break;
    case KITCHEN_GUEST_STEP_SHOW:
        if (gLobbyMenu->timer != 0) {
            gLobbyMenu->timer--;
        }
        if (--gLobbyMenu->slideTimer <= 0) {
            gLobbyMenu->subStep++;
        }
        break;
    case KITCHEN_GUEST_STEP_SLIDE_OUT:
        if (gLobbyMenu->timer != 0) {
            gLobbyMenu->timer--;
        }
        switch (WinSlide_Step(&gKitchenGuestSlide, 0x3C, -0x118)) {
        case 0:
            gLobbyMenu->subStep++;
            break;
        }
        break;
    case KITCHEN_GUEST_STEP_COOK:
        if (NpcScene_IsDone() == 1) {
            gLobbyMenu->subStep++;
            Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0x29);
        }
        break;
    case KITCHEN_GUEST_STEP_SERVE:
        if (gPlayers[gScene.localPlayer].motionId[0] == 0x261) {
            gLobbyMenu->subStep++;
            if (gKitchenGuestMeal.firstPick == 0) {
                Kitchen_EatSetCourse(gKitchenGuestMeal.secondPick, gKitchenGuestMeal.rand);
            } else {
                Kitchen_EatDish(gKitchenGuestMeal.season, gKitchenGuestMeal.firstPick, gKitchenGuestMeal.secondPick, gKitchenGuestMeal.rand);
            }
            cook = Npc_Find(NPC_HOUSE_COOK);
            if (cook != NULL) {
                Actor_SetState(cook, 0, 0x55);
            }
        }
        break;
    case KITCHEN_GUEST_STEP_EAT:
        if (gPlayers[gScene.localPlayer].state == 1) {
            gLobbyMenu->timer = 0;
            gLobbyMenu->subStep = KITCHEN_GUEST_STEP_WAIT;
            KitchenGuest_Close();
        }
        break;
    }
}

/* Sends the house's cook along the kitchen route of the host's spot
 * `spot` (KitchenMeal.spot's values). Nothing when the cook isn't there. */
void KitchenGuest_SendCook(s32 spot)
{
    struct Em *cook;

    cook = Npc_Find(NPC_HOUSE_COOK);
    if (cook != NULL) {
        Npc_SetKitchenRoute(cook, spot);
    }
}

#pragma optimization_level reset
/* Keeps the meal the host chose, while the guest menu runs: the next
 * step, 150 frames on the timer, and the meal's window starts sliding
 * in. */
void KitchenGuest_OnMealChosen(KitchenMeal *meal)
{
    if (gTownMode == TOWN_MODE_KITCHEN_GUEST) {
        gLobbyMenu->subStep = (u8) (gLobbyMenu->subStep + 1);
        gLobbyMenu->timer = 150;
        memcpy(&gKitchenGuestMeal, meal, sizeof(KitchenMeal));
        WinSlide_Step(&gKitchenGuestSlide, -0x118, 0x3C);
        LobbySe_Play(LOBBY_SE_WINDOW_OUT);
    }
}

/* The meal's end: the cook goes on along its route, and a guest menu whose
 * window hasn't begun to slide out closes. */
void KitchenGuest_OnMealEnd(void)
{
    struct Em *cook;

    cook = Npc_Find(NPC_HOUSE_COOK);
    if (cook != 0) {
        Npc_AdvanceRoute(cook);
    }
    if ((gTownMode == TOWN_MODE_KITCHEN_GUEST) && ((s32) gLobbyMenu->subStep < KITCHEN_GUEST_STEP_SLIDE_OUT)) {
        KitchenGuest_Close();
    }
}
