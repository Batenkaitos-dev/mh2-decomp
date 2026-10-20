/*
 * The game menu's exit entry (online): GameMenu_OpenExit opens the quit menu
 * (leave the lobby, and how), and GameMenu_UpdateExit runs it, turning the
 * quit menu's result into the pad bits the game menu acts on.
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "pad.h"

void QuitMenu_Open(void);
s32 GameMenu_IsBusy(void);
s32 Menu_PushNoticeOnce(s32);
s32 QuitMenu_Update(s32); /* unit's own declaration: the definition takes a u16 pad; declared with an s32 here, the call passes pad without masking it to 16 bits, as the game's code does */
s32 Chat_InInputMode(void);

/* The game menu's open handler for the exit: refused (returns 1) while a quest
 * is taken, with LOBBY_NOTICE_QUEST_TAKEN, or while the game menu is busy (GameMenu_IsBusy).
 * Otherwise it opens the quit menu, hides the help line and returns 0. */
s32 GameMenu_OpenExit(void *menu)
{
    if (gLobby->questTaken == 1) {
        Menu_PushNoticeOnce(LOBBY_NOTICE_QUEST_TAKEN);
        return 1;
    }
    if (GameMenu_IsBusy() == 1) {
        return 1;
    }
    QuitMenu_Open();
    gHelpShow = 0;
    return 0;
}

/* The exit's update. While the chat takes typed input, pad goes back as it
 * came. Otherwise the quit menu runs: cancelled, it returns PAD_CROSS; closed
 * with PAD_START, PAD_START; 0 while it runs or once a way to leave is
 * chosen. */
s32 GameMenu_UpdateExit(s32 pad)
{
    s32 result;

    if (Chat_InInputMode() == 1) {
        return pad;
    }
    result = QuitMenu_Update(pad);
    switch (result) {
    case MENU_CANCEL:
        return PAD_CROSS;
    case MENU_CLOSE:
        return PAD_START;
    default:
        return 0;
    }
}
