/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_net.h"

void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
void Lobby_Disconnect(s32);
s32 LobbyReq_68C0(void *id, void *dst, s32 arg2, void (*onReply)(NetMsg, NetMsg *));
extern char gCurServerId[];
extern u8 gLobbyLocText;
void LobbyStep_68C0Done(NetMsg m, NetMsg *pm);

s32 Lobby_HasQuest(void);
s32 Game_IsOnline(void);

void func_007B61E0(NetMsg m, NetMsg *pm) {

}

void LobbyStep_68C0Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 2) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = 99;
            }
        }
    }
}

/* sends 68C0 (the player's id; the reply, 0x80 bytes, into gLobbyLocText) */
s32 LobbyStep_68C0(void)
{
    s16 *seq;
    LobbyState *p;
    s16 step;

    p = gLobby;
    seq = &p->seqStep;
    step = *seq;
    switch (step) {
    case 0:
        *seq = (s16)(step + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 2;
        LobbyReq_68C0(gCurServerId, &gLobbyLocText, 0x80, LobbyStep_68C0Done);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        *seq = 0;
        return 0;
    case 99:
        Lobby_Disconnect(5);
        break;
    }
    return 2;
}

s32 Lobby_LacksQuest(void) {
    return Lobby_HasQuest() != 1;
}

void Lobby_ClearConnected(void) {
    if (Game_IsOnline() == 1) {
        gLobby->connected = 0;
    }
}
