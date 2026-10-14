/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"

void LobbyMenu_Init(void);
void LobbyScreen_ClearRequests(void);
void LobbyText_Open(s32);
void Chat_ClearTargets(void);
void *memset(void *dst, s32 c, u32 n);
extern s8 gLobbyLevel1Sel;
extern s8 gLobbyLevel2Sel;
extern M2C_UNK gLobbyLevel1Entries;
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern M2C_UNK Lobby_DrawBlackLine;

void LobbyVillage_Restart1(void) {
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
    LobbyMenu_Init();
}

void LobbyVillage_Begin2(void) {
    gLobby->phase = 2;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->unk393C = 0;
    gLobby->unk393D = 0;
    gLobby->enterSeq = 2;
    LobbyScreen_ClearRequests();
    gLobbyLevel1Sel = 0;
    gLobbyLevel2Sel = 0;
    memset(&gLobbyLevel1Entries, 0, 0x3E94);
    Chat_ClearTargets();
    LobbyMenu_Init();
    LobbyText_Open(0x10);
}

void LobbyVillage_Begin3(s32 arg0) {
    gLobby->phase = 2;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->unk393C = 0;
    gLobby->unk393D = 0;
    gLobby->enterSeq = 3;
    LobbyScreen_ClearRequests();
    LobbyMenu_Init();
    gLobbyLevel1Sel = 0;
    gLobbyLevel2Sel = 0;
    memset(&gLobbyLevel1Entries, 0, 0x3E94);
    Chat_ClearTargets();
    LobbyMenu_SetDraw(&Lobby_DrawBlackLine, 0, 0, 0);
    if (arg0 != 0) {
        LobbyText_Open(0x11);
    }
}

void LobbyVillage_BeginLeave(void) {
    gLobby->phase = 2;
    gLobby->phaseStep = 0;
    gLobby->mode = 4;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->unk393C = 0;
    gLobby->unk393D = 0;
    LobbyScreen_ClearRequests();
    gLobby->leave6307 = 2;
}
