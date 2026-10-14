/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"

void LobbyText_Open(s32);
void LobbyMenu_Init(void);
void LobbyScreen_ClearRequests(void);
void Chat_ClearTargets(void);
void *memset(void *dst, s32 c, u32 n);
extern s8 gLobbyLevel1Sel;
extern s8 gLobbyLevel2Sel;
extern M2C_UNK gLobbyLevel1Entries;

void LobbyVillage_Begin1(void) {
    gLobby->phase = 2;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->unk393C = 0;
    gLobby->unk393D = 0;
    gLobby->enterSeq = 1;
    LobbyScreen_ClearRequests();
    gLobbyLevel1Sel = 0;
    gLobbyLevel2Sel = 0;
    memset(&gLobbyLevel1Entries, 0, 0x3E94);
    Chat_ClearTargets();
    LobbyMenu_Init();
    LobbyText_Open(0x10);
}
