/*
 * Small queries on the HUD state (gHudState, hud.h) for the other menu and
 * field code: whether the HUD's menus take the pad (Hud_InputBlocked: a menu, the chat
 * input, the pouch-full dialog, or in a quest a message popup), whether a town menu is open, whether the chat key may open
 * the chat, how a monster shows on the map, the supply box and delivery
 * menus of the local player, the menus' D-pad key repeat, and closing the
 * field menu.
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "hud.h"
#include "village_cond.h"
#include "game_util.h"
#include "menu_util.h"
#include "net_scene.h"


/* prototypes */
/* end of prototypes */

extern u8 gHelpShow;

extern s32 gTownMenuOpen[];
extern u8 gChatInputMode;

/* Whether the HUD's input is taken by something else: a menu, the chat
 * input, the pouch-full dialog, or in a quest a message popup. */
s32 Hud_InputBlocked(void)
{
    if (gScene.menuOpen != 0) {
        return 1;
    }
    if (gChatInputMode != 0) {
        return 1;
    }
    if (gHudState->dropState != 0) {
        return 1;
    }
    if ((gScene.inLobby == 0) && (gHudState->popupOpen != 0)) {
        return 1;
    }
    return 0;
}

/* Whether one of the town's menus is open (the lobby's gTownMenuOpen),
 * online and in the town only; always 0 otherwise. */
s32 Hud_TownMenuOpen(void)
{
    if (Game_IsOnline() == 1 && gScene.inLobby == 1) {
        return *gTownMenuOpen;
    }
    return 0;
}

s32 Hud_ItemGetBusy(void)
{
    return gHudState->dropState != 0;
}

/* Whether the chat key may open the chat input now (Hud_Update): not while
 * the lobby's offline pause (Lobby_GetPauseState) is set, nor while the field
 * control is off, and offline only once the game's condition 1 holds
 * (Cond_Check). Returns 1 if allowed, else 0. */
s32 Hud_ChatKeyAllowed(void)
{
    if (Lobby_GetPauseState() != 0) {
        return 0;
    }
    if (Field_ControlEnabled() != 1) {
        return 0;
    }
    if ((Game_IsOnline() == 0) && (Cond_Check(1) == 0)) {
        return 0;
    }
    return 1;
}

/* How monster em shows on the map: 0 not at all, else a DETECT_* level.
 * Only a monster in the map half shown counts. A kind drawn as nothing or
 * as a small dot (gMapIconKinds) shows only while paint-marked (1). Any
 * other shows by the local player's
 * detectLevel, with DETECT_TIMER added while it is marked; the detect
 * skill's lower level alone shows nothing. */
u8 Map_MonsterVisibility(Em *em)
{
    HudState *hud;
    u8 half;
    u8 iconKind;
    u8 level;

    if (em->active == 0) {
        return 0;
    }
    hud = gHudState;
    half = hud->mapHalf;
    if (half != (u8)Map_GetHalf(gScene.map, em->area)) {
        return 0;
    }
    iconKind = gMapIconKinds[em->type];
    if (iconKind == MAP_ICON_NONE || (iconKind & (MAP_ICON_DOT | MAP_ICON_DOT_LARGE)) == MAP_ICON_DOT) {
        if (em->painted != 0) {
            return 1;
        }
        return 0;
    }
    level = hud->detectLevel;
    if (em->painted != 0) {
        level |= DETECT_TIMER;
    }
    if (level == DETECT_SKILL) {
        return 0;
    }
    return level;
}

s32 Hud_SupplyBoxOpen(void)
{
    return gHudState->player->supplyBoxOpen != 0;
}

/* Whether the local player's delivery menu is up (Player.deliveryMenu). */
s32 Hud_SupplyBoxActive(void)
{
    return gHudState->player->deliveryMenu != 0;
}

/* Whether the local player's delivery window is open (Player.deliveryOpen). */
s32 Hud_DeliveryOpen(void)
{
    return gHudState->player->deliveryOpen != 0;
}

void Hud_SetCancel(void)
{
    gHudState->cancel = 1;
}

void Hud_SetCamReqStarted(void)
{
    gHudState->camReqStarted = 1;
}

/* Clears the menus' D-pad key repeat (Menu_ReadDpad). */
void Hud_ResetMenuState(void)
{
    gHudState->dpadRepeatDir = 0;
    gHudState->dpadRepeatTimer = 0;
    gHudState->dpadRepeat = 0;
    gHudState->dpadPrev = 0;
}

/* The D-pad of pad (the stick counted as the D-pad) with key repeat, kept in
 * gHudState->dpadRepeat and returned: a new press at once, a held direction
 * after DPAD_REPEAT_DELAY frames, then every DPAD_REPEAT_RATE. */
u16 Menu_ReadDpad(u8 pad)
{
    u16 held;
    u16 pressed;
    u16 repeat;

    held = Pad_StickToDpad(pad);
    pressed = held & ~gHudState->dpadPrev;
    if (pressed != 0) {
        repeat = gHudState->dpadRepeatDir = pressed;
        gHudState->dpadRepeatTimer = DPAD_REPEAT_DELAY;
    } else if (held == 0) {
        repeat = gHudState->dpadRepeatDir = 0;
    } else {
        gHudState->dpadRepeatTimer--;
        repeat = 0;
        if (gHudState->dpadRepeatTimer <= 0) {
            gHudState->dpadRepeatTimer = DPAD_REPEAT_RATE;
            repeat = gHudState->dpadRepeatDir &= held;
        }
    }
    gHudState->dpadPrev = held;
    gHudState->dpadRepeat = repeat;
    return repeat;
}

/* Closes the field menu: its mode and page, the scene's menu flag, the
 * help line and any menu fade. */
void FieldMenu_Exit(void)
{
    gHudState->mode = 0;
    gHudState->menuPhase = 0;
    gScene.menuOpen = 0;
    gHelpShow = 0;
    gHudState->fadeActive = 0;
}
