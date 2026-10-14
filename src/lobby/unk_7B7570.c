/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_state.h"

void LobbySe_Play(s32);
s32 Menu_CountPages(s16, s16);
void LobbyTitle_Set(s32);
void LobbyHelp_Set(s32);
void LobbyHelp_SetBox(s16, s16, s16);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void NetMode_Init(void);
void LoginWave_Clear(void);
void LoginWave_Mark(s32);
void SeqA_Start(s32);
void SrvMenu_DrawInfo(MenuTask *task);
void SrvMenu_DrawWindow(MenuTask *task);
extern LobbyMenu *gLobbyMenu;
extern M2C_UNK LobbyDialog_DrawPending;

void SrvMenu_Start(void) {
    NetMode_Init();
    LoginWave_Clear();
    LoginWave_Mark(0);
    gLobbyMenu->screen = (u8) (gLobbyMenu->screen + 1);
    gLobbyMenu->step = 0;
    gLobbyMenu->cursor = 0;
    gLobbyMenu->slideTimer = 6;
    gLobbyMenu->page = 0;
    gLobbyMenu->pages = Menu_CountPages(D_5C34F8, 8);
    LobbyMenu_SetDraw(SrvMenu_DrawInfo, SrvMenu_DrawWindow, 0, &LobbyDialog_DrawPending);
    SeqA_Start(2);
    LobbyTitle_Set(2);
    LobbyHelp_Set(0);
    LobbySe_Play(0x16);
    LobbyHelp_SetBox(0x24, 0x16A, 0x154);
}
