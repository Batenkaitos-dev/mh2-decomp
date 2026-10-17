/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"

void LobbyText_Open(s32);
void LobbyScreen_ClearRequests(void);
extern LobbyMenu *gLobbyMenu;

void LobbyEnter_Begin(void) {
    u16 temp_v1;

    gLobby->phase = 7;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 0;
    gLobby->subStep = 0;
    LobbyScreen_ClearRequests();
    if (gLobby->unk3969 == 1) {
        temp_v1 = gQuestReturnArea;
        switch (temp_v1) {                          /* irregular */
        default:
            LobbyText_Open(6);
            break;
        case 0x86:
            LobbyText_Open(3);
            break;
        case 0x87:
            LobbyText_Open(4);
            break;
        case 0x88:
            LobbyText_Open(5);
            break;
        }
    } else {
        LobbyText_Open(0x1B);
    }
    gLobbyMenu->redraw = 1;
}
