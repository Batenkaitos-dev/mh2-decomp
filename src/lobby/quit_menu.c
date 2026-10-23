/*
 * The quit menu: three ways to leave the lobby (enum QuitChoice), each
 * confirmed with a yes/no, after which Lobby_Disconnect ends the session
 * with the matching LOBBY_QUIT_* reason. QuitMenu_Open starts it and
 * QuitMenu_Update runs it a frame at a time; its state is gQuitMenu.
 */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "pad.h"
#include "quit_menu.h"
#include "lobby_disconnect.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "libc.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;

void Menu_SetYesNo(u8); /* unit's own declaration: declared with the u8 the callers pass; with the definition's s8 choice, QuitMenu_Open and QuitMenu_Update compile differently */
void LobbyDialog_Grow(void);
void LobbyText_Open(s32);
extern QuitMenu gQuitMenu;
extern LobbyMenu *gLobbyMenu;

/* Opens the quit menu on its first choice, with its text in the dialog,
 * and takes the player's control. */
void QuitMenu_Open(void)
{
    memset(&gQuitMenu, 0, sizeof(gQuitMenu));
    gQuitMenu.choice = QUIT_CHOICE_GMSUB;
    LobbyText_Open(LOBBY_TEXT_QUIT_MENU);
    LobbyDialog_Grow();
    Menu_SetYesNo((u8) gQuitMenu.choice);
    gLobbyMenu->redraw = 1;
    gFieldControl = 0;
}

/*
 * The quit menu's frame (enum QuitMenuStep): up and down pick a choice,
 * confirm asks the choice's question, and yes ends the session
 * (Lobby_Disconnect); no goes back to the choices. Returns MENU_CANCEL when
 * left with cancel, MENU_CLOSE on PAD_START, else MENU_RUNNING.
 */
s32 QuitMenu_Update(u16 pad)
{
    QuitMenu *menu = &gQuitMenu;

    gLobbyMenu->redraw = 1;
    switch (gQuitMenu.step) {
    case QUIT_MENU_STEP_CHOOSE:
        if (pad & PAD_CIRCLE) {
            switch (menu->choice) {
            case QUIT_CHOICE_GMSUB:
                LobbyText_Open(LOBBY_TEXT_QUIT_ASK_GMSUB);
                break;
            case QUIT_CHOICE_GMSUB_KEEP:
                LobbyText_Open(LOBBY_TEXT_QUIT_ASK_GMSUB_KEEP);
                break;
            case QUIT_CHOICE_PORTAL:
                LobbyText_Open(LOBBY_TEXT_QUIT_ASK_PORTAL);
                break;
            }
            menu->step++;
            LobbyDialog_Grow();
            Menu_SetYesNo(1);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            gFieldControl = 1;
            return MENU_CANCEL;
        } else if (pad & PAD_START) {
            gFieldControl = 1;
            return MENU_CLOSE;
        } else {
            menu->choice = Menu_WrapUpDown(menu->choice, QUIT_CHOICES);
            Menu_SetYesNo(menu->choice);
        }
        break;
    case QUIT_MENU_STEP_CONFIRM:
        if (pad & PAD_START) {
            gFieldControl = 1;
            return MENU_CLOSE;
        }
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            gFieldControl = 1;
            switch (menu->choice) {
            case QUIT_CHOICE_GMSUB:
                Lobby_Disconnect(LOBBY_QUIT_TO_GMSUB);
                break;
            case QUIT_CHOICE_GMSUB_KEEP:
                Lobby_Disconnect(LOBBY_QUIT_TO_GMSUB_KEEP);
                break;
            case QUIT_CHOICE_PORTAL:
                Lobby_Disconnect(LOBBY_QUIT_TO_PORTAL);
                break;
            }
            break;
        case MENU_CANCEL:
            menu->step = QUIT_MENU_STEP_CHOOSE;
            LobbyText_Open(LOBBY_TEXT_QUIT_MENU);
            LobbyDialog_Grow();
            Menu_SetYesNo(menu->choice);
            break;
        }
        break;
    }
    return MENU_RUNNING;
}
