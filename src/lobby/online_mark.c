/*
 * The online mark window: an entry of the field menu's status page
 * (STATUS_ONLINE_MARK; the status page's update calls these). The online
 * mark is one of ONLINE_MARK_COUNT statuses a hunter shows the others
 * beside the name and on the hunter card (HunterCard.onlineMark; the local
 * hunter's is gOnlineMark), each with a line of the window and an icon
 * (ICON_ONLINE_MARK + it; mark 0 has none). OnlineMark_Open, OnlineMark_Input
 * and OnlineMark_Close run the window in gLobbyMenu, OnlineMark_Draw draws
 * it.
 * The .data holds the window's list; the .rodata the three colour codes the
 * draw writes into the lines' texts, and padding.
 */
#include "common.h"
#include "game_menu.h"
#include "hud.h"
#include "lobby_menu.h"
#include "pad.h"
#include "sound.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_state.h"
#include "window.h"
#include "lobby_labels.h"
#include "text.h"
#include "online_mark.h"
#include "lobby_steps.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "libc.h"

struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
extern LobbyMenu *gLobbyMenu;

/* Opens the online mark window: the cursor on the hunter's mark, and the
 * window's draw callback. */
void OnlineMark_Open(void)
{
    gLobbyMenu->screen = 0;
    gLobbyMenu->cursor = gPlayerText.card.onlineMark;
    LobbyMenu_SetDraw(NULL, OnlineMark_Draw, NULL, NULL);
}

/* Closes the online mark window: clears its screen, cursor and draw
 * callback, and sets the help line to the first mark's. */
void OnlineMark_Close(void)
{
    gLobbyMenu->screen = 0;
    gLobbyMenu->cursor = 0;
    gHelpLine = gLobbyMenu->cursor + ONLINE_MARK_HELP_LINE;
    LobbyMenu_SetDraw(NULL, NULL, NULL, NULL);
}

/* The online mark window's input: PAD_CIRCLE makes the mark under the
 * cursor the hunter's (gOnlineMark) and, when it changed, sends it on the
 * card (HunterCard_SendOnlineMark) with the confirm sound; the window stays.
 * PAD_CROSS and PAD_START close the window; up and down move over the nine
 * marks, each with its help line. Returns pad as it came: the status page
 * goes back to its list on PAD_CROSS and closes the field menu on PAD_START. */
s32 OnlineMark_Input(s32 pad)
{
    s32 btn;
    u8 sel;

    switch (gLobbyMenu->screen) {
    case 0:
        btn = pad & PAD_ALL;
        if (btn & PAD_CIRCLE) {
            sel = gLobbyMenu->cursor;
            if (gOnlineMark != sel) {
                gOnlineMark = sel;
                HunterCard_SendOnlineMark();
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
        } else if (btn & PAD_CROSS) {
            OnlineMark_Close();
        } else if (btn & PAD_START) {
            OnlineMark_Close();
        } else {
            gLobbyMenu->cursor = Menu_WrapUpDown(gLobbyMenu->cursor, ONLINE_MARK_COUNT);
            gHelpLine = gLobbyMenu->cursor + ONLINE_MARK_HELP_LINE;
        }
        break;
    }
    return pad;
}

extern ListPanel gOnlineMarkPanel;

/* Draws the online mark window. First each mark's text gets its colour code
 * in its first four characters: all plain while the hunter has no mark, else
 * the hunter's mark highlighted and the others dimmed. Then the list window
 * with its title and the pulsing cursor, then the icons of marks 1-8 beside
 * their lines (mark 0 has none): untinted for the hunter's mark, or for all
 * while none is set, grey for the others. */
void OnlineMark_Draw(MenuTask *task)
{
    s16 i;
    char **text;
    HunterCard *myCard;
    s16 y;
    u32 color;

    myCard = &gPlayerText.card;
    for (i = 0, text = gOnlineMarkNames; i < ONLINE_MARK_COUNT; i++, text++) {
        if (myCard->onlineMark == ONLINE_MARK_NONE) {
            memcpy(*text, "~C00", 4);
        } else if (myCard->onlineMark == i) {
            memcpy(*text, "~C04", 4);
        } else {
            memcpy(*text, "~C10", 4);
        }
    }
    Text_SetSize(task->textSize);
    gOnlineMarkPanel.cursorColor = Color_PulseAlpha(32896);
    ListPanel_Draw(&gOnlineMarkPanel, (s16 *)0, gNetLabels[NET_LABEL_ONLINE_MARK], gLobbyMenu->cursor);
    for (y = 107, i = 1; i < ONLINE_MARK_COUNT; y += 23, i++) {
        if (myCard->onlineMark == i || myCard->onlineMark == ONLINE_MARK_NONE) {
            color = -1; /* untinted */
        } else {
            color = 0xFF707070; /* grey */
        }
        Icon_Draw(419, y, 25, 20, color, ICON_ONLINE_MARK + i);
    }
}

/* the padding at the end of the file's .rodata */
const u8 gOnlineMarkPad[8] = { 0 };

/* The online mark window's list window. */
ListPanel gOnlineMarkPanel = {
#include "lobby/007F2090.inc"
};
