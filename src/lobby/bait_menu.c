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

void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Win_Draw(s16, s16, s32, s32, s32, s32, s32); /* unit's own declaration: the definition takes s32 x and y and an s16 kind; declared as the calls here pass them, since with the definition's types BaitMenu_DrawWindow's calls compile differently */
void Font_SetSpacing(u8 a, u8 b);

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u8 gMenuWork[];
extern s8 gMenuInputLock;
extern void *gNpcTalkSe[];
extern f32 gFishingSpotPos[2][3];
s32 Game_IsOnline(void);
Em *Npc_Find(u8 kind);
void Player_SetFlag(void *obj, u32 flag);
void Npc_PlaySeDelayed(Em *npc, void *p);
void *memset(void *dst, s32 c, u32 n);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void BaitMenu_LoadPage(BaitMenu *m, s16 page);
void BaitMenu_Draw(MenuTask *task);
void Talk_Start(Em *npc, s8 quiet);
void Talk_Say(s32 a, s8 b);
void Npc_StartTalkState(Em *npc);
void Npc_HoldRoute(Em *npc);
void Npc_EnterRoutePoint(Em *npc);
void CamFocus_Start(Em *npc);
void CamFocus_End(void);
void LobbySe_Play(s32 se);
s32 Actor_HeadingDiff(void *obj, void *pos);
void Text_SetSize(s32 a);
void Font_SetColor(s32 color);
void BaitMenu_DrawList(s16 x, s16 y);
void BaitMenu_DrawCastHint(void);

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

s32 Item_IsBait(u16 item, s32 unused1);
s32 Items_HaveEnough(u16 item, s32 n);
s32 Menu_CountPages(s16 n, s16 per);
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

void Chat_SetWindowMode(s8 a, s8 b);
s32 Talk_Update(void);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
void CamFocus_SetAside(s8 a);
s32 BaitMenu_ListInput(u16 pad);
s32 BaitMenu_FishInput(u16 unused);
s32 Menu_PushNoticeOnce(s32);
void BaitMenu_StartFishing(void);
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
                WinSlide_Step(&menu->win, 0x280, 0x118);
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
        switch (WinSlide_Step(&menu->win, 0x280, 0x118)) {
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
            WinSlide_Step(&menu->win, 0x280, 0x118);
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

s32 Items_Total(u16 item);
void Actor_SetStateFlags(struct Player *obj, u8 state, u8 sub, u16 flags);
void Npc_StartTurnTo(Em *npc, s32 a, s32 b);
void Actor_SetState(void *, s32, s32);
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
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
                Npc_StartTurnTo(menu->npc, 10, 0x3BBC);
            } else {
                Npc_StartTurnTo(menu->npc, 10, 0xF6C1);
                Actor_SetState(menu->npc, 0, 0x9E);
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
            /* the townsperson's reaction: actor state 0x7C in the village,
             * 0x55 online */
            if (Game_IsOnline() == 0) {
                if (menu->npc->state != 0x7C && Motion_ReachedFrame(pl, 0, 60.0f) != 0) {
                    Actor_SetState(menu->npc, 0, 0x7C);
                    Npc_StartTurnTo(menu->npc, 10, menu->npcYaw);
                }
            } else {
                if (menu->npc->state != 0x55 && Motion_ReachedFrame(pl, 0, 60.0f) != 0) {
                    Actor_SetState(menu->npc, 0, 0x55);
                    Player_SetFlag(menu->npc, EM_FLAG_NO_ROOT_MOTION);
                }
            }
        }
        break;
    }
    return MENU_RUNNING;
}

s32 Menu_WrapLeftRight(s32 cur, s32 n);
s32 Menu_WrapUpDown(s32 cur, s32 n);
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
    Win_Draw(x, y, 0x154, 0x120, 0xBDB59F6C, 0xBB402010, 8);
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xC8, -0x4F5000, gListTitles[LIST_TITLE_BAIT], 2, 1);
}

s32 Hud_ItemGetBusy(void);
void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Menu_DrawItemCountRow(s16 x, s16 y, u16 item, u32 color);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 mode, s32 item, u16 extra, s8 attr, u32 color);
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *);
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
    y += 0x1E;
    Font_SetSpacing(0x14, 0x14);
    left = x;
    barX = left + 10;
    nameX = left + 0x3C;
    /* `row` steps through items[] one entry at a time as a BaitMenu
     * pointer: the game's loop advances its pointer this way */
    for (i = 0, row = menu; i < BAIT_MENU_ROWS; i++) {
        item = row->items[0];
        if (i == menu->cursor) {
            selItem = item;
            Menu_DrawLine(barX, y - 1, 0x140, 0x18, 0);
        }
        Menu_DrawItemCountRow(nameX, y, item, 0x807F7F7F);
        y += 0x16;
        row = (BaitMenu *)((u8 *)row + sizeof(menu->items[0]));
    }
    Gfx_DrawLine(left + 0x14, y + 2, 0x12C, 2, 0xBDB59F6C);
    ItemInfo_Draw((s16)(left + 0x3C) - 0x3C, y + 4, 0x154, menu->showInfo, selItem, 0, -1, 0x807F7F7F);
    Win_DrawText(0x118, 0x15E, 0x154, 0x50, 0xBDB59F6C, 0xC0101010, gTownLabels[TOWN_LABEL_BAIT_HELP]);
}

s32 sprintf(char *buf, const char *fmt, ...);
extern char *gItemNames[];
extern char gBaitHintFmt[];
s32 Lobby_StrLength(char *);
void Gfx_DrawPulseGradient(s16 x, s16 y, s16 w, s16 h, u32 color, u8 dir);
s16 Lobby_DrawButtonIcon(s16, s16, s32);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);

/* Draws, before a cast, the two bars at the left: the bait in use with how
 * many are held, and the cast and cancel help with their button icons.
 * Nothing while the item-get display is showing. */
void BaitMenu_DrawCastHint(void)
{
    BaitMenu *menu = (BaitMenu *)gMenuWork;
    char buf[0x40];

    if (Hud_ItemGetBusy() == 1) {
        return;
    }
    Font_SetColor(0x807F7F7F);
    sprintf(buf, gListTitles[LIST_TITLE_FISHING_FMT], gItemNames[menu->bait], Items_Total(menu->bait));
    Gfx_DrawPulseGradient(0, 0x6F, Lobby_StrLength(buf) * 9 + 0x50, 0x1E, 0x80006000, 0);
    Gfx_DrawPulseGradient(0, 0x8F, 0xC8, 0x1E, 0x80006000, 0);
    Lobby_DrawButtonIcon(0x14, 0x72, 0);
    Lobby_DrawButtonIcon(0x14, 0x92, 1);
    Font_SetSpacing(0x14, 0x14);
    Font_SetPos(0x38, 0x73);
    Text_PrintfColored(buf);
    Font_SetPos(0x38, 0x93);
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
        BaitMenu_DrawWindow(menu->win.pos, 0x34);
        break;
    case BAIT_STEP_LIST:
        BaitMenu_DrawList(menu->win.pos, 0x34);
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
char gBaitHintFmt[0x10] = {
#include "lobby/00826040.inc"
};
#pragma rodata off
