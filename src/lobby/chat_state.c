/*
 * The game menu's chat state page, the chat page's last entry
 * (CHAT_ENTRY_STATE): where the chat goes, to the lobby's chat
 * (CHAT_SEND_LOBBY) or to the chosen hunters (CHAT_SEND_TARGETS), kept in
 * gChatWindowSel and copied to gChatSendTo. In AREA_ONLINE_VILLAGE only the first
 * is offered and the window is drawn dimmed. ChatPage_OpenState opens it,
 * ChatPage_UpdateState takes the buttons and ChatPage_DrawState draws it;
 * the chat page's own list and the hunter page come just before it.
 * The .data holds the page's two windows, the .rodata its two lines'
 * formats.
 */
#include "common.h"
#include "game_menu.h"
#include "scene.h"
#include "pad.h"
#include "lobby_globals.h"
#include "window.h"
#include "area.h"
#include "chat_log.h"
#include "hud.h"
#include "sound.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "shake.h"
#include "text.h"
#include "chat_state.h"
#include "font.h"
#include "item_shop.h"

/* The chat page's open handler for the chat state page: puts the cursor on
 * the current state (gChatWindowSel), shows its help line (the first
 * state's in AREA_ONLINE_VILLAGE), slides the window in and returns 0; 1
 * (refused) when ChatMenu_CanSetState says no, which it never does. */
s32 ChatPage_OpenState(void)
{
    if (ChatMenu_CanSetState() != 1) {
        return 1;
    }
    gSubMenuWork->windowSel = gChatWindowSel;
    gHelpPage = HELP_PAGE_CHAT;
    gHelpShow = 0;
    if (gScene.area != AREA_ONLINE_VILLAGE) {
        gHelpLine = gSubMenuWork->windowSel + 5;
    } else {
        gHelpLine = 5;
    }
    Shake_Start(SHAKE_CHAT_STATE, SHAKE_AXIS_X, 300.0f);
    return 0;
}

/* unit's own declaration: the buttons are declared s32 (the definition takes a u16): ChatPage_UpdateState passes its s32 btn as it is, where with u16 the compiler would cut it to 16 bits first, unlike the game's code */
s32 Menu_CursorUpDown(u8 *cur, s32 buttons, u8 count);

/* The chat state page's input: up and down choose a state (not in
 * AREA_ONLINE_VILLAGE), PAD_CIRCLE makes it current (gChatWindowSel, and
 * gChatSendTo the same). PAD_START slides the window out and, once it is
 * out, returns PAD_START (the game menu closes); while the window slides
 * in, the D-pad and PAD_CROSS are ignored and the help line hidden.
 * Returns the buttons left for GameMenu_UpdateChat (PAD_CROSS goes back to
 * its list). */
s32 ChatPage_UpdateState(s32 btn)
{
    u8 *cur;
    u8 sel;

    if (Shake_Flags(SHAKE_CHAT_STATE) & SHAKE_SLIDE) { /* sliding out: the menu closes once it is out */
        return Shake_Update(SHAKE_CHAT_STATE) == 0 ? PAD_START : 0;
    }
    if ((u16)btn & PAD_START) {
        Shake_Start(SHAKE_CHAT_STATE, SHAKE_SLIDE, 333.0f);
        gHelpShow = 0;
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
        return 0;
    }
    gHelpShow = 1;
    if (Shake_Update(SHAKE_CHAT_STATE) == 1) {
        gHelpShow = 0;
        btn = (u16)(btn & (0xFFFF & ~(PAD_DPAD | PAD_CROSS))); /* no D-pad or cancel while the window slides in */
    }
    if (gScene.area != AREA_ONLINE_VILLAGE) {
        Menu_CursorUpDown(&gSubMenuWork->windowSel, btn, 2);
        cur = &gSubMenuWork->windowSel;
        gHelpLine = *cur + 5;
        if ((u16)btn & PAD_CIRCLE) {
            sel = *cur;
            if (sel != gChatWindowSel) {
                gChatWindowSel = sel;
                gChatSendTo = sel;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
            }
        }
    }
    return btn;
}

struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
struct ShakeOffset;
extern ListPanel gChatStatePanel;
extern ListPanel gChatStatePanelDim;

/* The chat state page's draw: the window (its title, the cursor on
 * GameMenu.windowSel) and the two states' names, the current one
 * (gChatWindowSel) in the first text style, the markers' colours telling
 * which is current; in AREA_ONLINE_VILLAGE the dimmed window with the first
 * state alone. The window moves with its slide (Shake_CopyOffset). */
void ChatPage_DrawState(void)
{
    s16 pos[2];
    s32 topColor;
    s32 bottomColor;

    Shake_CopyOffset(SHAKE_CHAT_STATE, (struct ShakeOffset *) pos);
    if (gScene.area != AREA_ONLINE_VILLAGE) {
        ListPanel_Draw(&gChatStatePanel, pos, gChatPageTexts[CHAT_PAGE_TEXT_STATE_HEAD], gSubMenuWork->windowSel);
        Font_SetPos(pos[0] + 425, 85);
        if (gChatWindowSel == CHAT_SEND_LOBBY) {
            Text_PrintfStyled(4, "~C00%s\n~C10%s", gChatStateTexts[0], gChatStateTexts[1]);
            topColor = 0;
            bottomColor = 2;
        } else {
            Text_PrintfStyled(4, "~C10%s\n~C00%s", gChatStateTexts[0], gChatStateTexts[1]);
            topColor = 2;
            bottomColor = 1;
        }
        Gfx_DrawMarker(pos[0] + 398, 85, 20, 18, topColor);
        Gfx_DrawMarker(pos[0] + 398, 107, 20, 18, bottomColor);
    } else {
        ListPanel_Draw(&gChatStatePanelDim, pos, gChatPageTexts[CHAT_PAGE_TEXT_STATE_HEAD], -1);
        Gfx_DrawMarker(pos[0] + 398, 85, 20, 18, 0);
        Font_SetColor(0x807F7F7F);
        Font_SetPos(pos[0] + 425, 85);
        Text_PrintfColored(gChatStateTexts[0]);
    }
}

/* The chat state page's windows: the usual one, and AREA_ONLINE_VILLAGE's
 * dimmed one. */
ListPanel gChatStatePanel = {
#include "lobby/007F1FD0.inc"
};
ListPanel gChatStatePanelDim = {
#include "lobby/007F1FF0.inc"
};
