/*
 * Two townspeople's menus:
 * - the ship menu (TOWN_MODE_SHIP_MENU, ShipMenu): board, which plays a
 *   camera tour and a jingle, clears the meal's effects and takes the
 *   player to AREA_HUB_0x57, or see the offers (OfferMenu_Open);
 * - the village mine's menu (TOWN_MODE_MINE, MineMenu): the player gives
 *   the miner a pickaxe, he digs during the next quests (SaveData.mine),
 *   and hands over his find when next spoken to. The pickaxe list's input
 *   and draw, and the dig itself (Mine_Dig), follow in the next file.
 * Town_UpdateMode runs each menu's update; the talk code opens them.
 */
#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_vobj.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "lobby_npc.h"
#include "chat_log.h"
#include "area.h"
#include "lobby_labels.h"
#include "area_draw.h"
#include "bgm.h"
#include "camera.h"
#include "text.h"
#include "village_cond.h"
#include "ship_mine_menus.h"
#include "area_menu.h"
#include "item_shop.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "mine_model_objects.h"
#include "offer_menu.h"
#include "talk.h"
#include "town_frame.h"
#include "town_players.h"
#include "screen_fade.h"
#include "libc.h"
#pragma optimization_level 4

extern s32 gTownMode;
extern u8 gMenuWork[];
u8 Village_FacilityLevel(s32);
extern s32 gTownMenuOpen;

/* 1 while the ship menu's camera tour plays (the lobby's screen code
 * asks), else 0. */
s32 ShipMenu_IsTouring(void)
{
    if ((gTownMode == TOWN_MODE_SHIP_MENU) && (((ShipMenu *)gMenuWork)->state == SHIP_MENU_TOUR)) {
        return 1;
    }
    return 0;
}

/* 1 when the townsperson offers the ship: condition 26 holds and the
 * village facility 3 is at level 2. */
s32 ShipMenu_IsOffered(void)
{
    if ((Cond_Check(26) == 1) && ((Village_FacilityLevel(3) & 0xFF) == 2)) {
        return 1;
    }
    return 0;
}

extern u8 gMenuInputLock;
struct Em;
/* Opens the ship menu for npc: the offers are listed as a second choice
 * when OfferMenu_BuildList finds some. */
void ShipMenu_Open(u8 *npc)
{
    ShipMenu *menu;

    gTownMenuOpen = 1;
    menu = (ShipMenu *)gMenuWork;
    gTownMode = TOWN_MODE_SHIP_MENU;
    memset(menu, 0, MENU_WORK_SIZE);
    menu->npc = npc;
    if (OfferMenu_BuildList((struct Em *)npc, 0) != 0) {
        menu->count = 2;
    } else {
        menu->count = 1;
    }
    LobbyMenu_SetDraw(ShipMenu_Draw, 0, 0, 0);
    gMenuInputLock = 6;
    LobbySe_Play(17);
}

/* Closes the ship menu and gives the camera back. */
void ShipMenu_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
}

/*
 * The ship menu (TOWN_MODE_SHIP_MENU): boarding is confirmed in talk,
 * then the camera tour plays with the player hidden, the meal's effects
 * are cleared, the screen fades out and the player is taken to
 * AREA_HUB_0x57. The offers choice opens the offer menu; leaving ends
 * with the townsperson's words.
 */
void ShipMenu_Update(void)
{
    ShipMenu *menu = (ShipMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    s32 pad = LobbyPad_Get(0);

    switch (menu->state) {
    case SHIP_MENU_CHOOSE:
        switch (ShipMenu_Input(menu, pad)) {
        case MENU_DONE:
            if (menu->cursor == SHIP_CHOICE_BOARD) {
                menu->state += 1;
                Talk_Say(3, 1);
            } else {
                OfferMenu_Open((struct Em *)menu->npc);
            }
            break;
        case MENU_CANCEL:
            menu->state = SHIP_MENU_LEAVE;
            Talk_Say(1, 0);
            break;
        }
        break;
    case SHIP_MENU_CONFIRM:
        switch (Talk_Update()) {
        case MENU_DONE:
            menu->state += 1;
            Talk_Say(4, 0);
            gFieldControl = 0;
            break;
        case MENU_CANCEL:
            menu->state = SHIP_MENU_LEAVE;
            Talk_Say(5, 0);
            break;
        }
        break;
    case SHIP_MENU_LAST_WORDS:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->state += 1;
            CamReq_TownTour(14);
            Jingle_Play(pl, 217);
            pl->visible = 0;
            gScene.mealAttack = 0;
            gScene.mealDefense = 0;
            gScene.mealHealth = 0;
            gScene.mealStamina = 0;
            gLobby->mealEaten = 0;
            break;
        }
        break;
    case SHIP_MENU_TOUR:
        pl->visible = 0;
        /* 0: no request left, -1: held at its end */
        switch (CamReq_State()) {
        case 0:
        case -1:
            FadeA_Start(FADE_OUT_BLACK);
            menu->state += 1;
            break;
        }
        break;
    case SHIP_MENU_FADE:
        pl->visible = 0;
        if (FadeA_GetState() != FADE_STATE_RUNNING) {
            gFieldControl = 1;
            gTownMenuOpen = 0;
            gTownMode = 0;
            AreaMenu_Open(AREA_HUB_0x57);
        }
        break;
    case SHIP_MENU_LEAVE:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            ShipMenu_Close();
            break;
        }
        break;
    }
}

/* The choices' input: PAD_CIRCLE takes the choice (boarding is refused,
 * with a notice, while a quest is taken), PAD_CROSS leaves, up and down
 * move the cursor. Returns MENU_DONE, MENU_CANCEL or MENU_RUNNING. */
s32 ShipMenu_Input(ShipMenu *menu, s32 pad)
{
    s32 btn;

    btn = pad & PAD_ALL;
    if (btn & PAD_CIRCLE) {
        if ((menu->cursor == SHIP_CHOICE_BOARD) && (gLobby->questTaken != 0)) {
            Menu_PushNoticeOnce(LOBBY_NOTICE_QUEST_TAKEN);
            LobbySe_Play(LOBBY_SE_REFUSE);
            /* a jump to the shared return: a return here compiles as a
             * second exit */
            goto running;
        }
        LobbySe_Play(LOBBY_SE_CONFIRM);
        return MENU_DONE;
    }
    if (btn & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    menu->cursor = Menu_WrapUpDown((s32)menu->cursor, (s32)menu->count);
running:
    return MENU_RUNNING;
}

/* The ship menu's draw: its choices while they are shown. */
void ShipMenu_Draw(MenuTask *task)
{
    ShipMenu *menu = (ShipMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    switch (menu->state) {
    case SHIP_MENU_CHOOSE:
        Menu_DrawLabelList(460, gShipMenuTexts, menu->count, menu->cursor, 0xFFB0B000);
        break;
    }
}

/* Opens the mine's menu for the miner npc: his report on his last dig
 * when he has dug (MINE_DUG: one line for one dig, another
 * for two or three, another for more), else his opening words. */
void Mine_OpenMenu(u8 *npc)
{
    MineMenu *menu;

    gTownMenuOpen = 1;
    menu = (MineMenu *)gMenuWork;
    gTownMode = TOWN_MODE_MINE;
    memset(menu, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Mine_DrawMenu, 0, 0, 0);
    menu->miner = (struct Em *)npc;
    if (gSave.mine.active == MINE_DUG) {
        switch (gSave.mine.digs) {
        case 1:
            Talk_Say(4, 0);
            break;
        case 2:
        case 3:
            Talk_Say(5, 0);
            break;
        default:
            Talk_Say(6, 0);
            break;
        }
        menu->state = MINE_MENU_DONE_TALK;
    } else {
        Talk_Say(1, 0);
    }
    gMenuInputLock = 6;
}

/* Closes the mine's menu and gives the camera back. */
void Mine_CloseMenu(void)
{
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
}

/* Starts the pickaxe list's window sliding in and lists the pickaxes
 * held, from the weakest. */
void Mine_BuildPickaxeList(MineMenu *menu)
{
    WinSlide_Step(&menu->win, 640, 280);
    memset(menu->items, 0, MINE_PICKAXE_KINDS * sizeof(u16));
    menu->count = 0;
    if (Items_HaveEnough(ITEM_ID_MINE_TOOL_WEAK, 1) == 1) {
        menu->items[menu->count] = ITEM_ID_MINE_TOOL_WEAK;
        menu->count++;
    }
    if (Items_HaveEnough(ITEM_ID_MINE_TOOL_MID, 1) == 1) {
        menu->items[menu->count] = ITEM_ID_MINE_TOOL_MID;
        menu->count++;
    }
    if (Items_HaveEnough(ITEM_ID_MINE_TOOL_STRONG, 1) == 1) {
        menu->items[menu->count] = ITEM_ID_MINE_TOOL_STRONG;
        menu->count++;
    }
}

/* unit's own declaration: the pad as an s32, as the caller passes it; with the definition's u16 pad, the call in Mine_UpdateMenu compiles differently */
s32 Mine_PickaxeInput(MineMenu *m, s32 pad);
u8 SavePouch_Add(u16 item, s16 n);
/*
 * The mine's menu (TOWN_MODE_MINE). After his opening words the pickaxe
 * list slides in; a pickaxe given is dug with (Mine_Dig) and the miner
 * starts on his route, chosen by the mine's level (VILLAGE_FACILITY_MINE:
 * 6 and over, under 3, or between). When he has dug, his find goes to the
 * pouch and the mine's state is cleared, or a notice says the pouch is
 * full.
 */
void Mine_UpdateMenu(void)
{
    MineMenu *menu = (MineMenu *)gMenuWork;
    NpcWork *w = (NpcWork *)&menu->miner->work;
    s32 pad = LobbyPad_Get(0);

    switch (menu->state) {
    case MINE_MENU_TALK:
        switch (Talk_Update()) {
        case MENU_DONE:
            menu->state += 1;
            Mine_BuildPickaxeList(menu);
            break;
        case MENU_CANCEL:
            menu->state = MINE_MENU_REFUSED;
            Talk_Say(0, 0);
            break;
        }
        break;
    case MINE_MENU_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case MENU_DONE:
            menu->state += 1;
            break;
        }
        break;
    case MINE_MENU_CHOOSE:
        switch (Mine_PickaxeInput(menu, pad)) {
        case MENU_DONE:
            menu->state += 1;
            Talk_Say(3, 0);
            Mine_Dig(menu);
            w->routeIdx = 0;
            if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_MINE) >= 6) {
                w->route = gMinerRouteHigh;
            } else if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_MINE) < 3) {
                w->route = gMinerRouteLow;
            } else {
                w->route = gMinerRouteMid;
            }
            /* he starts in the state of his route's first point */
            Actor_SetState(menu->miner, 0, (u16)w->route[w->routeIdx].state);
            break;
        case MENU_CANCEL:
            menu->state = MINE_MENU_REFUSED;
            Talk_Say(0, 0);
            break;
        }
        break;
    case MINE_MENU_DIGGING:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            Mine_CloseMenu();
            break;
        }
        break;
    case MINE_MENU_REFUSED:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            Mine_CloseMenu();
            break;
        }
        break;
    case MINE_MENU_DONE_TALK:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->state += 1;
            Talk_Say(7, 0);
            break;
        }
        break;
    case MINE_MENU_HAND_OVER:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            if (SavePouch_Space(gSave.mine.find) > 0) {
                SavePouch_Add(gSave.mine.find, 1);
                Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, *(s16 *)&gSave.mine.find);
                gSave.mine.find = 0;
                gSave.mine.digs = 0;
                gSave.mine.active = MINE_IDLE;
            } else {
                Menu_PushNotice(21);
            }
            Mine_CloseMenu();
            break;
        }
        break;
    }
}

/* 1 when the mine is open further (condition 21 holds, 25 doesn't,
 * and the player has no ITEM_ID_MINE_CLOSER): Mine_Dig then draws from each
 * pickaxe's other table of finds. */
s32 Mine_IsOpen(void)
{
    if ((Cond_Check(21) == 1) && (Cond_Check(25) == 0) && (Items_HaveEnough(ITEM_ID_MINE_CLOSER, 1) == 0)) {
        return 1;
    }
    return 0;
}
