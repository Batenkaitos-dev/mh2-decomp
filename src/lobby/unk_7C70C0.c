/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_net.h"

s32 LobbyReq_6303(u16 id, void (*onReply)(NetMsg, NetMsg *));
void LobbyScreen_ClearRequests(void);
s32 LobbyLevel_SelId(s32);
void Chat_ClearTargets(void);
void *memset(void *dst, s32 c, u32 n);
extern s8 gLobbyLevel2Sel;
void LobbyLevel1_On6307Reply(NetMsg m, NetMsg *pm);
void Kbd_Cancel(void);

void LobbyTown_Begin(void) {
    gLobby->phase = 3;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->unk393C = 0;
    gLobby->unk393D = 0;
    LobbyScreen_ClearRequests();
    memset(gLobbyLevel2Entries, 0, sizeof(gLobbyLevel2Entries));
    gLobbyLevel2Sel = 0;
    gLobby->entered6504 = 0;
    gFieldControl = 1;
    Chat_ClearTargets();
    LobbyReq_6303(LobbyLevel_SelId(1) & 0xFFFF, &LobbyLevel1_On6307Reply);
}

void LobbyTown_BeginAtStep3(void) {
    gLobby->phase = 3;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 3;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->unk393C = 0;
    gLobby->unk393D = 0;
    LobbyScreen_ClearRequests();
    memset(gLobbyLevel2Entries, 0, sizeof(gLobbyLevel2Entries));
    gLobbyLevel2Sel = 0;
    gLobby->entered6504 = 0;
    gFieldControl = 1;
    Chat_ClearTargets();
    LobbyReq_6303(LobbyLevel_SelId(1) & 0xFFFF, &LobbyLevel1_On6307Reply);
}

void LobbyTown_BeginLeave(void) {
    gLobby->phase = 3;
    gLobby->phaseStep = 0;
    gLobby->mode = 1;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->unk393C = 0;
    gLobby->unk393D = 0;
    LobbyScreen_ClearRequests();
    Kbd_Cancel();
    gLobby->leave640C = 2;
    gLobby->leave6505 = 2;
}
