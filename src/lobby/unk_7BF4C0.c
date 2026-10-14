/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_menu.h"

void LobbyText_Open(s32);
void LobbyHelp_SetBox(s16, s16, s16);
void LobbyScreen_ClearRequests(void);
void *memset(void *dst, s32 c, u32 n);
extern MenuWork gMenuWork;
extern LobbyMenu *gLobbyMenu;

void LevelList_Begin(void) {
    gLobby->phase = 1;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 0;
    gLobby->subStep = 0;
    LobbyScreen_ClearRequests();
    LobbyText_Open(0x1B);
    gLobbyMenu->redraw = 1;
    memset(&gMenuWork, 0, 0xF00);
    gMenuWork.levelList.wait = 6;
    LobbyHelp_SetBox(0x24, 0x16A, 0x154);
}
