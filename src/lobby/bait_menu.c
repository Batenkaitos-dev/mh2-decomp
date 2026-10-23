/*
 * The bait menu at the fishing spot (TOWN_MODE_BAIT_MENU): the townsperson
 * there (NPC_FISHING_SPOT in the village, NPC_FISHING_SPOT_ONLINE online)
 * greets the player, a window lists the bait the player holds, seven a
 * page, and once one is chosen the player stands at the spot and casts with
 * it, again and again, until the bait runs out or the player cancels.
 *
 * The menu keeps its state in gMenuWork as a BaitMenu. Talking to the
 * townsperson opens it (BaitMenu_Open), the town's mode switch runs
 * BaitMenu_Update every frame, and BaitMenu_Draw is its draw callback. The
 * cast itself is the town's fishing state machine; this menu only starts
 * it and watches the player's state and motion.
 */
#include "common.h"
#include "em.h"
#include "item.h"
#include "lobby_globals.h"
#include "lobby_text.h"
#include "menu_task.h"
#include "pad.h"
#include "scene.h"
#include "player.h"
#include "lobby_menu.h"
#include "lobby_npc.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "camera.h"
#include "combine.h"
#include "hud.h"
#include "quest.h"
#include "text.h"
#include "bait_menu.h"
#include "font.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "name_tags.h"
#include "offer_menu.h"
#include "talk.h"
#include "town_area_change.h"
#include "town_input.h"
#include "town_players.h"
#include "libc.h"

void Win_Draw(s16, s16, s32, s32, s32, s32, s32); /* unit's own declaration: the definition takes s32 x and y and an s16 kind; declared as the calls here pass them, since with the definition's types BaitMenu_DrawWindow's calls compile differently */
void Font_SetSpacing(u8 a, u8 b);

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u8 gMenuWork[];
extern s8 gMenuInputLock;
extern void *gNpcTalkSe[];
extern f32 gFishingSpotPos[2][3];

/* 1 while the bait menu runs, else 0. */
s32 BaitMenu_IsOpen(void)
{
    return gTownMode == TOWN_MODE_BAIT_MENU;
}

/* Opens the bait menu at the fishing spot's townsperson: lists the first
 * page of bait, starts the greeting, holds the townsperson at its route
 * point and turns the camera on it. Online the townsperson's motion keeps
 * its root still; in the village it plays its kind's sound first. Nothing
 * happens when the townsperson isn't there. */
void BaitMenu_Open(void)
{
    BaitMenu *menu = (BaitMenu *)gMenuWork;
    Em *npc;

    if (Game_IsOnline() == 1) {
        npc = Npc_Find(NPC_FISHING_SPOT_ONLINE);
        Player_SetFlag(npc, EM_FLAG_NO_ROOT_MOTION);
    } else {
        npc = Npc_Find(NPC_FISHING_SPOT);
    }
    if (npc != NULL) {
        if (Game_IsOnline() == 0) {
            Npc_PlaySeDelayed(npc, gNpcTalkSe[((NpcWork *)&npc->work)->id]);
        }
        gTownMenuOpen = 1;
        gTownMode = TOWN_MODE_BAIT_MENU;
        gMenuInputLock = 6;
        memset(gMenuWork, 0, MENU_WORK_SIZE);
        LobbyMenu_Init();
        LobbyMenu_SetDraw(BaitMenu_Draw, 0, 0, 0);
        BaitMenu_LoadPage(menu, 0);
        menu->npc = npc;
        Talk_Start(menu->npc, 0);
        Talk_Say(BAIT_TALK_GREET, 0);
        Npc_StartTalkState(menu->npc);
        Npc_HoldRoute(menu->npc);
        CamFocus_Start(menu->npc);
    }
}

/* Fills the menu with page `page` of the bait the player holds (in the
 * pouch or the item box; ITEM_ID_FISH_BAIT_LURE_ALT is never listed), and
 * sets the count, the page and the number of pages. The cursor is pulled
 * back onto the page. */
void BaitMenu_LoadPage(BaitMenu *menu, s16 page)
{
    s16 first;
    s16 last;
    s16 held = 0;
    s16 i;

    menu->count = 0;
    first = page * BAIT_MENU_ROWS;
    last = first + BAIT_MENU_ROWS - 1;
    memset(menu->items, 0, sizeof(menu->items));
    menu->count = 0;
    for (i = 0; i < ITEM_ID_COUNT; i++) {
        if (Item_IsBait(i, 0) == 1 && i != ITEM_ID_FISH_BAIT_LURE_ALT && Items_HaveEnough(i, 1) == 1) {
            if (held >= first && held <= last) {
                menu->items[menu->count] = i;
                menu->count++;
            }
            held++;
        }
    }
    menu->page = page;
    menu->pages = Menu_CountPages(held, BAIT_MENU_ROWS);
    if (menu->cursor >= menu->count) {
        menu->cursor = menu->count - 1;
    }
}

/* Closes the menu: back to TOWN_MODE_FREE, the camera's focus ended and the
 * townsperson sent back to its route's first point. */
void BaitMenu_Close(void)
{
    BaitMenu *menu = (BaitMenu *)gMenuWork;

    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    ((NpcWork *)&menu->npc->work)->routeIdx = 0;
    Npc_EnterRoutePoint(menu->npc);
    LobbySe_Play(LOBBY_SE_CANCEL);
}

/* The menu's frame, by its step: the greeting (with no bait held the
 * townsperson says so and the menu closes), the window sliding in, the
 * list, the fishing, and the last line before closing. Leaving the fishing
 * with bait still held goes back to the list. */
void BaitMenu_Update(void)
{
    BaitMenu *menu = (BaitMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = pl->input.pressed;

    Chat_SetWindowMode(1, 2);
    switch (menu->step) {
    case BAIT_STEP_GREET:
        switch (Talk_Update()) {
        case MENU_DONE:
            if (menu->count != 0) {
                menu->step++;
                gMenuInputLock = 6;
                CamFocus_SetAside(1);
                WinSlide_Step(&menu->win, 640, 280);
            } else {
                menu->step = BAIT_STEP_BYE;
                Talk_Say(BAIT_TALK_NO_BAIT, 0);
            }
            break;
        case MENU_CANCEL:
            menu->step = BAIT_STEP_BYE;
            Talk_Say(BAIT_TALK_BYE, 0);
            break;
        }
        break;
    case BAIT_STEP_OPEN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case 0:
            menu->step++;
            break;
        }
        break;
    case BAIT_STEP_LIST:
        switch (BaitMenu_ListInput(pad)) {
        case MENU_DONE:
            menu->step++;
            BaitMenu_StartFishing();
            Menu_PushNoticeOnce(LOBBY_NOTICE_BAIT_CHOSEN);
            menu->npcYaw = menu->npc->rotY;
            break;
        case MENU_CANCEL:
            CamFocus_SetAside(0);
            Talk_Say(BAIT_TALK_BYE, 0);
            menu->step = BAIT_STEP_BYE;
            break;
        }
        break;
    case BAIT_STEP_FISH:
        switch (BaitMenu_FishInput(pad)) {
        case MENU_DONE:
            menu->step = BAIT_STEP_BYE;
            Talk_Say(BAIT_TALK_OUT_OF_BAIT, 0);
            CamFocus_Start(menu->npc);
            Npc_StartTalkState(menu->npc);
            break;
        case MENU_CANCEL:
            BaitMenu_LoadPage(menu, 0);
            menu->step = BAIT_STEP_OPEN;
            gMenuInputLock = 6;
            WinSlide_Step(&menu->win, 640, 280);
            CamFocus_Start(menu->npc);
            CamFocus_SetAside(1);
            Npc_StartTalkState(menu->npc);
            break;
        }
        break;
    case BAIT_STEP_BYE:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            BaitMenu_Close();
            break;
        }
        break;
    }
}

/* Starts the fishing with the chosen bait: turns the player toward the
 * fishing spot (gFishingSpotPos: [0] in the village, [1] online), makes the
 * bait the item in use and ends the camera's focus. */
void BaitMenu_StartFishing(void)
{
    BaitMenu *menu = (BaitMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    f32 *spot;

    menu->fishStep = BAIT_FISH_READY;
    if (Game_IsOnline() == 1) {
        spot = gFishingSpotPos[1];
    } else {
        spot = gFishingSpotPos[0];
    }
    pl->targetYaw += (u16)Actor_HeadingDiff(pl, spot);
    pl->useItem = menu->bait;
    CamFocus_End();
}

/* The fishing's input. Before a cast, circle casts (TOWN_PC_FISH_CAST,
 * which goes on to TOWN_PC_FISH_WAIT) and turns the townsperson toward
 * the water, cross goes back to the list. While the line is out it waits
 * for the player to stand again (TOWN_PC_IDLE), counts the bait left, and
 * makes the townsperson react when the player lands a catch
 * (PL_MOT_FISH_CATCH, frame 60). Returns MENU_DONE when no bait is left at
 * all, MENU_CANCEL to go back to the list (cross, or this bait ran out
 * while other bait is held), else
 * MENU_RUNNING. */
s32 BaitMenu_FishInput(u16 unused)
{
    BaitMenu *menu = (BaitMenu *)gMenuWork;
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 pad = pl->input.pressed;

    switch (menu->fishStep) {
    case BAIT_FISH_READY:
        if (Items_HaveEnough(menu->bait, 1) == 0) {
            BaitMenu_LoadPage(menu, 0);
            if (menu->count == 0) {
                return MENU_DONE;
            }
            Menu_PushNoticeOnce(LOBBY_NOTICE_BAIT_GONE);
            return MENU_CANCEL;
        }
        if (pad & PAD_CIRCLE) {
            LobbySe_Play(LOBBY_SE_CONFIRM);
            Actor_SetStateFlags(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_FISH_CAST, 0);
            menu->fishStep++;
            if (Game_IsOnline() == 0) {
                Npc_StartTalkState(menu->npc);
                Npc_StartTurnTo(menu->npc, 10, 15292);
            } else {
                Npc_StartTurnTo(menu->npc, 10, 63169);
                Actor_SetState(menu->npc, 0, 158);
                Player_SetFlag(menu->npc, EM_FLAG_NO_ROOT_MOTION);
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        }
        break;
    case BAIT_FISH_LINE_OUT:
        if (pl->state == TOWN_PC_IDLE) {
            if (Items_HaveEnough(menu->bait, 1) == 1) {
                menu->fishStep = BAIT_FISH_READY;
                menu->baitHeld = Items_Total(menu->bait);
                Npc_StartTalkState(menu->npc);
            } else {
                BaitMenu_LoadPage(menu, 0);
                if (menu->count == 0) {
                    return MENU_DONE;
                }
                Menu_PushNoticeOnce(LOBBY_NOTICE_BAIT_GONE);
                return MENU_CANCEL;
            }
        }
        if (pl->motionId[0] == PL_MOT_FISH_CATCH) {
            /* the townsperson's reaction: actor state 124 in the village,
             * 85 online */
            if (Game_IsOnline() == 0) {
                if (menu->npc->state != 124 && Motion_ReachedFrame(pl, 0, 60.0f) != 0) {
                    Actor_SetState(menu->npc, 0, 124);
                    Npc_StartTurnTo(menu->npc, 10, menu->npcYaw);
                }
            } else {
                if (menu->npc->state != 85 && Motion_ReachedFrame(pl, 0, 60.0f) != 0) {
                    Actor_SetState(menu->npc, 0, 85);
                    Player_SetFlag(menu->npc, EM_FLAG_NO_ROOT_MOTION);
                }
            }
        }
        break;
    }
    return MENU_RUNNING;
}

/* The list's input: circle takes the bait under the cursor (MENU_DONE),
 * cross leaves (MENU_CANCEL), square shows or hides its description, left
 * and right turn the page, up and down move the cursor. Returns
 * MENU_RUNNING otherwise. */
s32 BaitMenu_ListInput(u16 pad)
{
    BaitMenu *menu = (BaitMenu *)gMenuWork;

    if (pad & PAD_CIRCLE) {
        menu->bait = menu->items[menu->cursor];
        menu->baitHeld = Items_Total(menu->bait);
        menu->cursor = 0;
        menu->page = 0;
        LobbySe_Play(LOBBY_SE_CONFIRM);
        return MENU_DONE;
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & PAD_SQUARE) {
        menu->showInfo ^= 1;
        LobbySe_Play(LOBBY_SE_DESCRIPTION);
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (menu->pages > 1) {
            menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
            BaitMenu_LoadPage(menu, menu->page);
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else {
        menu->cursor = Menu_WrapUpDown(menu->cursor, menu->count);
    }
    return MENU_RUNNING;
}

/* Draws the list's window at (x, y) and its title bar. */
void BaitMenu_DrawWindow(s16 x, s16 y)
{
    Win_Draw(x, y, 340, 288, 0xBDB59F6C, 0xBB402010, 8);
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(280, 18, 200, -0x4F5000, gListTitles[LIST_TITLE_BAIT], 2, 1);
}

/* Draws the list at (x, y): the window, the page's bait with how many of
 * each are held, the cursor's bar, the selected bait's description under a
 * rule, and the help window at the bottom. Nothing while the item-get
 * display is showing. */
void BaitMenu_DrawList(s16 x, s16 y)
{
    BaitMenu *menu = (BaitMenu *)gMenuWork;
    s32 left;
    s32 item;
    s32 selItem;
    s16 i;
    BaitMenu *row;
    s32 barX;
    s32 nameX;
    if (Hud_ItemGetBusy() == 1) {
        return;
    }
    BaitMenu_DrawWindow(x, y);
    Font_SetColor(0x807F7F7F);
    y += 30;
    Font_SetSpacing(20, 20);
    left = x;
    barX = left + 10;
    nameX = left + 60;
    /* `row` steps through items[] one entry at a time as a BaitMenu
     * pointer: the game's loop advances its pointer this way */
    for (i = 0, row = menu; i < BAIT_MENU_ROWS; i++) {
        item = row->items[0];
        if (i == menu->cursor) {
            selItem = item;
            Menu_DrawRowHighlight(barX, y - 1, 320, 24, 0);
        }
        Menu_DrawItemCountRow(nameX, y, item, 0x807F7F7F);
        y += 22;
        row = (BaitMenu *)((u8 *)row + sizeof(menu->items[0]));
    }
    Gfx_DrawLine(left + 20, y + 2, 300, 2, 0xBDB59F6C);
    ItemInfo_Draw((s16)(left + 60) - 60, y + 4, 340, menu->showInfo, selItem, 0, -1, 0x807F7F7F);
    Win_DrawText(280, 350, 340, 80, 0xBDB59F6C, 0xC0101010, gTownLabels[TOWN_LABEL_BAIT_HELP]);
}

extern char *gItemNames[];
extern char gBaitHintFmt[];

/* Draws, before a cast, the two bars at the left: the bait in use with how
 * many are held, and the cast and cancel help with their button icons.
 * Nothing while the item-get display is showing. */
void BaitMenu_DrawCastHint(void)
{
    BaitMenu *menu = (BaitMenu *)gMenuWork;
    char buf[64];

    if (Hud_ItemGetBusy() == 1) {
        return;
    }
    Font_SetColor(0x807F7F7F);
    sprintf(buf, gListTitles[LIST_TITLE_FISHING_FMT], gItemNames[menu->bait], Items_Total(menu->bait));
    Gfx_DrawPulseGradient(0, 111, Lobby_StrLength(buf) * 9 + 80, 30, 0x80006000, 0);
    Gfx_DrawPulseGradient(0, 143, 200, 30, 0x80006000, 0);
    Lobby_DrawButtonIcon(20, 114, 0);
    Lobby_DrawButtonIcon(20, 146, 1);
    Font_SetSpacing(20, 20);
    Font_SetPos(56, 115);
    Text_PrintfColored(buf);
    Font_SetPos(56, 147);
    Text_PrintfColored(gBaitHintFmt, gListTitles[LIST_TITLE_BACK_TO_BAIT]);
}

/* The menu's draw callback: the window while it slides in, the list, or
 * the cast help while the player stands ready to cast. */
void BaitMenu_Draw(MenuTask *task)
{
    BaitMenu *menu = (BaitMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    Font_SetColor(0x80101010);
    switch (menu->step) {
    case BAIT_STEP_OPEN:
        BaitMenu_DrawWindow(menu->win.pos, 52);
        break;
    case BAIT_STEP_LIST:
        BaitMenu_DrawList(menu->win.pos, 52);
        break;
    case BAIT_STEP_FISH:
        if (menu->fishStep == BAIT_FISH_READY) {
            BaitMenu_DrawCastHint();
        }
        break;
    }
}

/* The fishing spot's position the player turns toward: [0] in the village,
 * [1] online. */
f32 gFishingSpotPos[2][3] = {
#include "lobby/007E7940.inc"
};

/* The cast help's format. */
#pragma rodata on
char gBaitHintFmt[16] = {
#include "lobby/00826040.inc"
};
#pragma rodata off
