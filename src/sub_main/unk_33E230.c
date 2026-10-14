/*
 * Queries and resets on the state at *gHudState (+8 is a Player).
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "hud.h"
#include "objhead.h"

/* prototypes */
s32 Field_ControlEnabled(void);
s32 Cond_Check(u16 id);
s32 Lobby_GetDayState(void);
u16 Pad_StickToDpad(u8 pad);
/* end of prototypes */

extern u8 gHelpShow;

s32 Game_IsOnline(void);

extern s32 gTownMenuOpen[];
s32 Hud_TownMenuOpen(void)
{
    if (Game_IsOnline() == 1 && gScene.online == 1) {
        return *gTownMenuOpen;
    }
    return 0;
}

s32 Hud_ItemGetBusy(void)
{
    return gHudState->dropState != 0;
}

s32 Hud_FieldInputAllowed_Unk(void)
{
    if (Lobby_GetDayState() != 0) {
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

s32 Map_IsMaplessArea(u8 stage, u8 area);
extern u8 gMapIconKinds[];

u8 Map_PlayerIconKind(void *obj)
{
    ObjHead *o = obj;
    HudState *s;
    u8 team;
    u8 k;
    u8 v;

    if (o->active == 0) {
        return 0;
    }
    s = gHudState;
    team = s->maplessArea;
    if (team != (u8)Map_IsMaplessArea(gScene.map, o->area)) {
        return 0;
    }
    k = gMapIconKinds[o->weaponType];
    if (k == 0xFF || (k & 0xC0) == 0x80) {
        if (o->painted != 0) {
            return 1;
        }
        return 0;
    }
    v = s->unkAA;
    if (o->painted != 0) {
        v |= 1;
    }
    if (v == 2) {
        return 0;
    }
    return v;
}

s32 Hud_SupplyBoxOpen(void)
{
    return gHudState->player->supplyBoxOpen != 0;
}

s32 Hud_SupplyBoxActive(void)
{
    return gHudState->player->deliveryMenu != 0;
}

s32 Hud_PlayerBusy8D5_Unk(void)
{
    return gHudState->player->deliveryOpen != 0;
}

void Hud_SetCancel(void)
{
    gHudState->cancel = 1;
}

void Hud_SetUnkA9(void)
{
    gHudState->camReqStarted = 1;
}

void Hud_ResetMenuState(void)
{
    gHudState->dpadRepeatDir = 0;
    gHudState->dpadRepeatTimer = 0;
    gHudState->dpadRepeat = 0;
    gHudState->dpadPrev = 0;
}

/* The D-pad of pad (stick included) with key repeat, into +0xAE and returned: a new press
 * at once, a held direction after 8 frames, then every 3. */
u16 Menu_ReadDpad(u8 pad)
{
    u16 held;
    u16 edge;
    u16 rep;

    held = Pad_StickToDpad(pad);
    edge = held & ~gHudState->dpadPrev;
    if (edge != 0) {
        rep = gHudState->dpadRepeatDir = edge;
        gHudState->dpadRepeatTimer = 8;
    } else if (held == 0) {
        rep = gHudState->dpadRepeatDir = 0;
    } else {
        gHudState->dpadRepeatTimer--;
        rep = 0;
        if (gHudState->dpadRepeatTimer <= 0) {
            gHudState->dpadRepeatTimer = 3;
            rep = gHudState->dpadRepeatDir &= held;
        }
    }
    gHudState->dpadPrev = held;
    gHudState->dpadRepeat = rep;
    return rep;
}

void FieldMenu_Exit(void)
{
    gHudState->mode = 0;
    gHudState->menuPhase = 0;
    gScene.menuOpen = 0;
    gHelpShow = 0;
    gHudState->fadeActive = 0;
}
