/*
 * The workshop's menu (TOWN_MODE_WORKSHOP), which a workshop townsperson
 * opens: a top list of the three counters (the weapon forge, the armour
 * forge and the decorations) and, when the townsperson has any, its offers.
 * NPC_WORKSHOP_HELPER and NPC_WORKSHOP_HELPER_ONLINE show a shorter list.
 *
 * The menu's header is a WorkshopMenu at the start of gMenuWork; the
 * counter chosen keeps its work after it and runs through the counter's
 * own functions (WeaponForge_*, ArmorForge_*, Deco_*), which this menu
 * calls for each step's input and draw. Once a counter's work is done the
 * forging scene plays: in the village the smith's hammering with a camera
 * request, online a lesson of the tutorial townsperson. Then the counter's
 * closing question runs, and the menu goes back to the top list.
 */
#include "common.h"
#include "area.h"
#include "em.h"
#include "lobby_forge.h"
#include "lobby_globals.h"
#include "lobby_text.h"
#include "menu_task.h"
#include "pad.h"
#include "save.h"
#include "scene.h"
#include "player.h"
#include "lobby_menu.h"
#include "lobby_npc.h"
#include "lobby_labels.h"

#define WORKSHOP_NPC_FORGING 0x69   /* the actor state of the smith's forging scene */
#define FORGE_WORK_SIZE 0xE00       /* a counter's work, cleared when it opens */

extern u8 gMenuWork[];
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;
void *memset(void *dst, s32 c, u32 n);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void LobbySe_Play(s32);
s16 OfferMenu_BuildList(struct Em *t, s32 b);
void Workshop_Draw(MenuTask *task);
void Workshop_Draw2(MenuTask *task);

/* Opens the workshop's menu at townsperson `npc`: restores the value the
 * menu keeps in the townsperson's block, and lists the three counters, plus
 * the offers when the townsperson has any; the helpers' list is one entry
 * shorter. */
void Workshop_Open(Em *npc)
{
    WorkshopMenu *menu = (WorkshopMenu *)gMenuWork;
    NpcWorkshopWork *npcWork = (NpcWorkshopWork *)&npc->work;

    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_WORKSHOP;
    memset(menu, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Workshop_Draw, Workshop_Draw2, 0, 0);
    menu->saved = npcWork->menuSave;
    if (OfferMenu_BuildList(npc, 0) != 0) {
        menu->hasOffers = 1;
        menu->count = 4;
    } else {
        menu->count = 3;
    }
    if (npcWork->id == NPC_WORKSHOP_HELPER_ONLINE || npcWork->id == NPC_WORKSHOP_HELPER) {
        menu->shortList = 1;
    } else {
        menu->shortList = 0;
    }
    if (menu->shortList != 0) {
        menu->count--;
    }
    gMenuInputLock = 6;
    LobbySe_Play(0x1B);
}

void CamFocus_End(void);
void CamReq_Cancel(void);
void EquipPreview_Free(void);
void Chat_SetColor(s16);

/* Closes the workshop: back to TOWN_MODE_FREE, the camera's focus and
 * request ended, the equipment preview freed, the chat's colour reset. */
void Workshop_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    CamFocus_End();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamReq_Cancel();
    EquipPreview_Free();
    Chat_SetColor(-1);
    LobbySe_Play(LOBBY_SE_CANCEL);
}

u16 LobbyPad_Get(s32);
struct Em *Npc_Find(u8 kind);
void Actor_SetState(void *, s32, s32);
s32 Game_IsOnline(void);
void OfferMenu_Open(struct Em *t);
void Talk_Say(s32 msg, s8 a);
s32 Talk_Update(void);
s32 Menu_WrapUpDownSe(s32 cur, s32 n, s32 se);
void CamReq_Workshop(u8 lv); /* unit's own declaration: the definition takes an s32; declared with the u8 the callers pass, since with the definition's type Workshop_Update's call compiles differently */
/* unit's own declaration: the definition returns a u8; declared with the s32 the callers read, since with the definition's type Workshop_Update's call compiles differently */
s32 Village_FacilityLevel(s32 a);
s32 Npc_NextRoutePoint(struct Em *);
void NpcScene_Start(s8 a, s32 b, s32 c);
s32 NpcScene_IsDone(void);
void Npc_StartTalkState(struct Em *p);
void Workshop_OpenCounter(void);
s32 Workshop_CounterInput(u16 pad);
s32 Workshop_ResultInput(u16 pad);
/* The menu's frame, by its step: the top list (circle opens the counter
 * under the cursor, or the offer menu on the offers' entry; cross says
 * goodbye), the counter's input, the forging scene and its wait, the
 * counter's closing question, and the last line before closing. The
 * village's forging scene doesn't play in AREA_HUB_0x57. */
void Workshop_Update(void)
{
    WorkshopMenu *menu = (WorkshopMenu *)gMenuWork;
    Em *talker = gPlayers[gScene.localPlayer].target;
    Em *smith = Npc_Find(NPC_WORKSHOP_SMITH);
    NpcWorkshopWork *talkerWork = (NpcWorkshopWork *)&talker->work;
    u16 pad = LobbyPad_Get(0);

    switch (menu->step) {
    case WORKSHOP_STEP_TOP:
        if (pad & PAD_CIRCLE) {
            if (menu->shortList != 0) {
                if (menu->choice == menu->count - 1 && Game_IsOnline() == 0) {
                    OfferMenu_Open(talker);
                    LobbySe_Play(0x1B);
                    break;
                }
            } else if (menu->choice == WORKSHOP_OFFERS) {
                OfferMenu_Open(talker);
                LobbySe_Play(0x1B);
                break;
            }
            menu->step++;
            memset((u8 *)menu + FORGE_WORK_OFFSET, 0, FORGE_WORK_SIZE);
            Workshop_OpenCounter();
            LobbySe_Play(0x1B);
        } else if (pad & PAD_CROSS) {
            menu->step = WORKSHOP_STEP_BYE;
            Talk_Say(1, 0);
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else {
            menu->choice = Menu_WrapUpDownSe(menu->choice, menu->count, 2);
        }
        break;
    case WORKSHOP_STEP_COUNTER:
        switch (Workshop_CounterInput(pad)) {
        case MENU_DONE:
            talkerWork->menuSave = menu->saved;
            menu->step++;
            Chat_SetColor(-1);
            break;
        case MENU_CANCEL:
            menu->step--;
            break;
        }
        break;
    case WORKSHOP_STEP_FORGE:
        menu->step++;
        if (Game_IsOnline() == 0) {
            if (gScene.area == AREA_HUB_0x57) {
                break;
            }
            if (talkerWork->id == NPC_WORKSHOP_HELPER) {
                Actor_SetState(smith, 0, WORKSHOP_NPC_FORGING);
                Npc_NextRoutePoint(talker);
            } else {
                Actor_SetState(talker, 0, WORKSHOP_NPC_FORGING);
            }
            CamReq_Workshop(Village_FacilityLevel(VILLAGE_FACILITY_SMITHY));
        } else if (talkerWork->id == NPC_WORKSHOP_HELPER_ONLINE) {
            NpcScene_Start(NPC_SCENE_WORKSHOP_HELPER, 0, 0);
            CamReq_Workshop(4);
        } else {
            NpcScene_Start(NPC_SCENE_WORKSHOP, 0, 0);
            CamReq_Workshop(3);
        }
        break;
    case WORKSHOP_STEP_FORGE_WAIT:
        if (Game_IsOnline() == 0) {
            if (gScene.area == AREA_HUB_0x57) {
                menu->step++;
                break;
            }
            if (smith->state != WORKSHOP_NPC_FORGING) {
                CamReq_Cancel();
                menu->step++;
                if (talkerWork->id == NPC_WORKSHOP_HELPER) {
                    Npc_StartTalkState(talker);
                }
            }
        } else if (NpcScene_IsDone() == 1) {
            menu->step++;
        }
        break;
    case WORKSHOP_STEP_RESULT:
        switch (Workshop_ResultInput(pad)) {
        case MENU_DONE:
            menu->step = WORKSHOP_STEP_COUNTER;
            Chat_SetColor(0);
            break;
        }
        break;
    case WORKSHOP_STEP_BYE:
        switch (Talk_Update()) {
        case MENU_DONE:
            Workshop_Close();
            break;
        }
        break;
    }
}

void WeaponForge_Open(WorkshopMenu *menu);
void ArmorForge_Open(WorkshopMenu *menu);
void Deco_Open(void);
/* Opens the counter under the top list's cursor. */
void Workshop_OpenCounter(void)
{
    WorkshopMenu *menu = (WorkshopMenu *)gMenuWork;

    switch (menu->choice) {
    case WORKSHOP_WEAPONS:
        WeaponForge_Open(menu);
        break;
    case WORKSHOP_ARMOR:
        ArmorForge_Open(menu);
        break;
    case WORKSHOP_DECOS:
        Deco_Open();
        break;
    }
}

void Chat_SetWindowMode(s8 a, s8 b);
s32 WeaponForge_Input(u8 *task, u16 pad);
s32 ArmorForge_Input(u8 *task, u16 pad);
s32 Deco_Input(u8 *task, u16 pad);
/* Runs the open counter's input; returns its MENU_* result. */
s32 Workshop_CounterInput(u16 pad)
{
    WorkshopMenu *menu = (WorkshopMenu *)gMenuWork;

    Chat_SetWindowMode(2, 2);
    switch (menu->choice) {
    case WORKSHOP_WEAPONS:
        return WeaponForge_Input((u8 *) menu, pad);
    case WORKSHOP_ARMOR:
        return ArmorForge_Input((u8 *) menu, pad);
    case WORKSHOP_DECOS:
        return Deco_Input((u8 *) menu, pad);
    }
    return MENU_RUNNING;
}

s32 WeaponForge_Confirm(u8 *task, u16 pad);
s32 ArmorForge_Confirm(u8 *task, u16 pad);
s32 Deco_Confirm(u8 *task, u16 pad);
/* Runs the open counter's closing question after the forging scene;
 * returns its MENU_* result. */
s32 Workshop_ResultInput(u16 pad)
{
    WorkshopMenu *menu = (WorkshopMenu *)gMenuWork;

    Chat_SetWindowMode(2, 2);
    switch (menu->choice) {
    case WORKSHOP_WEAPONS:
        return WeaponForge_Confirm((u8 *) menu, pad);
    case WORKSHOP_ARMOR:
        return ArmorForge_Confirm((u8 *) menu, pad);
    case WORKSHOP_DECOS:
        return Deco_Confirm((u8 *) menu, pad);
    }
    return MENU_RUNNING;
}

void Font_SetSpacing(u8 a, u8 b);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
/* Draws the top list: the helpers' shorter list or the full one. */
void Workshop_DrawTop(s32 x, s32 y)
{
    WorkshopMenu *menu = (WorkshopMenu *)gMenuWork;

    Font_SetSpacing(0x14, 0x14);
    if (menu->shortList != 0) {
        Menu_DrawLabelList(0x1CC, gWorkshopTexts, menu->count, menu->choice, 0xFFB0B000);
    } else {
        Menu_DrawLabelList(0x1CC, gSmithyMenuTexts, menu->count, menu->choice, 0xFFB0B000);
    }
}

void Text_SetSize(s32 a);
void WeaponForge_Draw(u8 *o);
void ArmorForge_Draw(u8 *task);
void Deco_Draw(u8 *task);
void WeaponForge_DrawPreview(u8 *o);
void ArmorForge_DrawEquipPrompt(u8 *task);
void Deco_DrawEquipPrompt(u8 *task);
/* The menu's first draw callback: the top list, the open counter's
 * screens, or its closing question. */
void Workshop_Draw(MenuTask *task)
{
    WorkshopMenu *menu = (WorkshopMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    switch (menu->step) {
    case WORKSHOP_STEP_TOP:
        Workshop_DrawTop(0x1CC, 0x40);
        break;
    case WORKSHOP_STEP_COUNTER:
        switch (menu->choice) {
        case WORKSHOP_WEAPONS:
            WeaponForge_Draw((u8 *) menu);
            break;
        case WORKSHOP_ARMOR:
            ArmorForge_Draw((u8 *) menu);
            break;
        case WORKSHOP_DECOS:
            Deco_Draw((u8 *) menu);
            break;
        }
        break;
    case WORKSHOP_STEP_RESULT:
        switch (menu->choice) {
        case WORKSHOP_WEAPONS:
            WeaponForge_DrawPreview((u8 *) menu);
            break;
        case WORKSHOP_ARMOR:
            ArmorForge_DrawEquipPrompt((u8 *) menu);
            break;
        case WORKSHOP_DECOS:
            Deco_DrawEquipPrompt((u8 *) menu);
            break;
        }
        break;
    }
}

void Deco_Draw2(u8 *unused0);
/* The menu's second draw callback: only the decorations counter draws on
 * this layer. The empty cases stay: the game's switch has them. */
void Workshop_Draw2(MenuTask *task)
{
    WorkshopMenu *menu = (WorkshopMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    switch (menu->step) {
    case WORKSHOP_STEP_TOP:
        break;
    case WORKSHOP_STEP_COUNTER:
        switch (menu->choice) {
        case WORKSHOP_DECOS:
            Deco_Draw2((u8 *)menu);
            break;
        }
        break;
    case WORKSHOP_STEP_RESULT:
        break;
    }
}
