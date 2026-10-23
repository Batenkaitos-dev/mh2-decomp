/*
 * The house access menu (TOWN_MODE_HOUSE_ACCESS): the town's members
 * (gNetUsers, LOBBY_MEMBER_COUNT rows) with whether each one's house is
 * open (HunterCard.houseAccess). Picking an open house sends the visit
 * (TownMsg_SendHouseVisit) and opens the area menu on that house's area;
 * a house with a password asks for it on the keyboard first.
 *
 * The menu's state is HouseAccessMenu in gMenuWork. HouseAccess_Update runs
 * it (enum HouseVisitStep), HouseAccess_Draw draws the list.
 */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "scene.h"
#include "pad.h"
#include "player.h"
#include "area.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "lobby_text.h"
#include "menu_task.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "kbd.h"
#include "lobby_net.h"
#include "text.h"
#include "window.h"
#include "house_access.h"
#include "area_menu.h"
#include "font.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "my_house.h"
#include "libc.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern MenuWork gMenuWork;

/* Opens the house access menu: the town mode, a cleared gMenuWork, the
 * draw callback, and the window's slide in from the right. */
void HouseAccess_Open(void)
{
    gTownMode = TOWN_MODE_HOUSE_ACCESS;
    gTownMenuOpen = 1;
    memset(&gMenuWork, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, HouseAccess_Draw, 0, 0);
    WinSlide_Step(&gMenuWork.win, 640, 410);
    LobbySe_Play(LOBBY_SE_CONFIRM);
}

/* Closes the house access menu. */
void HouseAccess_Close(void)
{
    gTownMode = 0;
    gTownMenuOpen = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
}

extern NetUser gNetUsers[];
extern char gMyUserId[];
extern s16 gHouseCloseTimer;
extern LobbyMenu *gLobbyMenu;
s8 Kbd_Run(char *dst, s16 held, s16 pressed);

/* Returns the town's member `slot`. */
static inline NetUser *Member_Get(s8 slot)
{
    return &gNetUsers[slot];
}

#pragma optimization_level 4
/*
 * The house access menu's frame (enum HouseVisitStep). On the list,
 * confirm on a member who is in the town (not the player) visits an open
 * house at once, or asks for a password-locked house's password (up to 8
 * characters) on the keyboard; a wrong or empty password shows
 * LOBBY_TEXT_HOUSE_PASSWORD_WRONG. Cancel closes the menu. A visit records
 * the member's slot (as a character, '0' + slot) for the local player and
 * opens the area menu on the house's area.
 *
 * The password step reads the member again into its own locals (card,
 * slot): the game's code keeps them in other registers than the list
 * step's.
 */
void HouseAccess_Update(void)
{
    HunterCard *card;
    HouseAccessMenu *menu = (HouseAccessMenu *)&gMenuWork;
    s32 slot;
    u16 pad;
    s8 i;
    HunterCard *pickCard;
    NetUser *user;

    MyHouse_LocalSlot();
    pad = LobbyPad_Get(0);
    Chat_SetWindowMode(1, 2);
    switch (menu->step) {
    case HOUSE_VISIT_STEP_OPEN:
        switch (WinSlide_Step(&menu->win, 640, 410)) {
        case MENU_DONE:
            menu->step++;
        }
        break;
    case HOUSE_VISIT_STEP_LIST:
        if (pad & PAD_CIRCLE) {
            if (gNetUsers[menu->cursor].id[0] == 0 ||
                memcmp(gNetUsers[menu->cursor].id, gMyUserId, NET_USER_ID_MATCH) == 0) {
                LobbySe_Play(LOBBY_SE_REFUSE);
                break;
            }
            if (gPlayers[menu->cursor].area == 0) {
                LobbySe_Play(LOBBY_SE_REFUSE);
                break;
            }
            i = menu->cursor;
            user = Member_Get(i);
            pickCard = (HunterCard *)user->text;
            if (((HunterCard *)user->text)->houseAccess == HOUSE_ACCESS_OPEN) {
                TownMsg_SendHouseVisit(user);
                gLobby->memberSlot[gScene.localPlayer] = i + '0';
                LobbyMenu_SetDraw(0, 0, 0, 0);
                AreaMenu_Open(pickCard->houseLevel + AREA_ONLINE_HOUSE_0);
                gHouseCloseTimer = -1;
                LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            } else if (((HunterCard *)user->text)->houseAccess == HOUSE_ACCESS_PASSWORD) {
                menu->step = HOUSE_VISIT_STEP_PASSWORD;
                gFieldControl = 0;
                Kbd_SetPos(58, 80.0f);
                Chat_OpenKbd(0, 6, sizeof(menu->pass) - 1, menu->pass);
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
        } else if (pad & PAD_CROSS) {
            menu->cursor = 0;
            menu->step = HOUSE_VISIT_STEP_OPEN;
            HouseAccess_Close();
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else {
            menu->cursor = Menu_WrapUpDown(menu->cursor, LOBBY_MEMBER_COUNT);
        }
        break;
    case HOUSE_VISIT_STEP_PASSWORD:
        if (((HunterCard *)Member_Get(menu->cursor)->text)->houseAccess != HOUSE_ACCESS_PASSWORD || gNetUsers[menu->cursor].id[0] == 0 ||
            memcmp(gNetUsers[menu->cursor].id, gMyUserId, NET_USER_ID_MATCH) == 0) {
            menu->step = HOUSE_VISIT_STEP_LIST;
            LobbySe_Play(LOBBY_SE_CANCEL);
            Kbd_Cancel();
            gFieldControl = 1;
            break;
        }
        if (gPlayers[menu->cursor].area == 0) {
            menu->step = HOUSE_VISIT_STEP_LIST;
            LobbySe_Play(LOBBY_SE_CANCEL);
            Kbd_Cancel();
            gFieldControl = 1;
            break;
        }
        if (Kbd_Run(menu->pass, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            Kbd_Cancel();
            gFieldControl = 1;
            if (menu->pass[0] != 0) {
                menu->step = HOUSE_VISIT_STEP_LIST;
                card = (HunterCard *)Member_Get(menu->cursor)->text;
                slot = (s8)menu->cursor;
                if (memcmp(card->housePass, menu->pass, sizeof(menu->pass)) == 0) {
                    TownMsg_SendHouseVisit(&gNetUsers[slot]);
                    gLobby->memberSlot[gScene.localPlayer] = slot + '0';
                    LobbyMenu_SetDraw(0, 0, 0, 0);
                    AreaMenu_Open(card->houseLevel + AREA_ONLINE_HOUSE_0);
                    gHouseCloseTimer = -1;
                } else {
                    menu->step = HOUSE_VISIT_STEP_WRONG;
                    LobbyText_Open(LOBBY_TEXT_HOUSE_PASSWORD_WRONG);
                    LobbyDialog_Grow();
                    gLobbyMenu->redraw = 1;
                }
            } else {
                menu->step = HOUSE_VISIT_STEP_WRONG;
                LobbyText_Open(LOBBY_TEXT_HOUSE_PASSWORD_WRONG);
                LobbyDialog_Grow();
                gLobbyMenu->redraw = 1;
            }
        }
        break;
    case HOUSE_VISIT_STEP_WRONG:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            menu->step = HOUSE_VISIT_STEP_LIST;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            memset(menu->pass, 0, sizeof(menu->pass));
        }
        break;
    }
}
#pragma optimization_level reset

void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: declared with the s16 sizes the caller passes; with the definition's s32 parameters and s16 kind, HouseAccess_DrawList compiles differently */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);

/*
 * Draws the member list at (x, y): the title, the cursor's row (dimmed
 * while the keyboard is up), and per member the house's state with its
 * icon (the away icon for a member in no area) and the member's name, hidden
 * behind the user id when the chat hides names. x + 10 is a local (lineX)
 * for the three lines, as in the game's code.
 */
void HouseAccess_DrawList(s16 x, s16 y)
{
    HunterCard *card;
    NetUser *user;
    s16 rowY = y;
    HouseAccessMenu *menu = (HouseAccessMenu *)&gMenuWork;
    s32 lineX;
    s8 i;

    Win_Draw(x, y, 210, 260, 0xBDB59F6C, -1, 2);
    lineX = x + 10;
    Gfx_DrawLine(lineX, rowY + 10, 190, 20, 0x80B59F6C);
    rowY += 10;
    Font_SetPos((s16)(x + 30) - 20, rowY);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(gTownLabels[TOWN_LABEL_HOUSE_ACCESS]);
    if (menu->step < HOUSE_VISIT_STEP_PASSWORD) {
        Menu_DrawRowHighlight(lineX, rowY + 20 + menu->cursor * 22, 190, 22, 0);
    } else {
        Menu_DrawRowHighlight(lineX, rowY + 20 + menu->cursor * 22, 190, 22, 1);
    }
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < LOBBY_MEMBER_COUNT; i++) {
        rowY += 22;
        user = &gNetUsers[i];
        card = (HunterCard *)user->text;
        Font_SetPos((s16)(x + 30) + 90, rowY);
        if (user->id[0] != 0) {
            if (gPlayers[i].area == 0) {
                Font_SetColor(0x80404040);
                Text_PrintfColored(gTownLabels[TOWN_LABEL_GONE_OUT]);
                Icon_Draw((s16)(x + 30) - 20, rowY, 20, 20, -1, 71);
            } else {
                switch (card->houseAccess) {
                case HOUSE_ACCESS_CLOSED:
                    Font_SetColor(0x80404040);
                    Text_PrintfColored(gTownLabels[TOWN_LABEL_CLOSE]);
                    Icon_Draw((s16)(x + 30) - 20, rowY, 20, 20, -1, 71);
                    break;
                case HOUSE_ACCESS_OPEN:
                    Font_SetColor(0x807F7F7F);
                    Text_PrintfColored(gTownLabels[TOWN_LABEL_OPEN]);
                    break;
                case HOUSE_ACCESS_PASSWORD:
                    Font_SetColor(0x807F7F7F);
                    Text_PrintfColored(gTownLabels[TOWN_LABEL_PASSWORD]);
                    Icon_Draw((s16)(x + 30) - 20, rowY, 20, 20, -1, 89);
                    break;
                }
            }
        }
        Gfx_DrawLine(x + 10, rowY - 2, 190, 2, 0xBDB59F6C);
        if (user->id[0] != 0) {
            Font_SetPos(x + 30, rowY);
            if (user->id[0] != 0) {
                if (gChatHideNames == 0) {
                    Text_PrintfColored(user->name);
                } else {
                    Text_PrintfColored(user->id);
                }
            } else {
                Text_PrintfColored("--------");
            }
        }
    }
}

/* The menu's draw callback: the list at the window's slide position. */
void HouseAccess_Draw(MenuTask *task)
{
    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    HouseAccess_DrawList(gMenuWork.win.pos, 60);
}
