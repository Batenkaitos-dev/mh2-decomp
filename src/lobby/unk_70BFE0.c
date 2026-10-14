#include "common.h"
#include "lobby_net.h"
#include "em.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"

s32 LobbyPkt_EntryFlag(s32 kind, s32 arg1); /* unit's own declaration: with the definition's s32 (u16 kind, u16 v) (lobby/unk_7046A0.c), LobbyReq_620B 1 of 29 words differ */
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u8 gMenuWork[];
void LobbyMsg_ReadU16U8b2(void *a, void *b);
struct TextBox;
void TownSave_Draw(struct TextBox *t);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
s32 Game_IsOnline(void);
void ActSpot_PlaceLocal(void);
void LobbySe_Play(s32);

void LobbyMsg_ReadU16(void *a0)
{
    LobbyMsg_GetU16(a0, gLobbyRecvData);
}

void LobbyMsg_ReadU16Str(void *a, void *b)
{
    LobbyMsg_GetString(b, LobbyMsg_GetU16(a, gLobbyRecvData));
}

void LobbyMsg_ReadU16U8b(void *a, void *b)
{
    LobbyMsg_GetU8(b, LobbyMsg_GetU16(a, gLobbyRecvData));
}

void LobbyMsg_ReadU16Str2(void *a, void *b)
{
    LobbyMsg_GetString(b, LobbyMsg_GetU16(a, gLobbyRecvData));
}

void LobbyMsg_ReadU16U8b2(void *a, void *b)
{
    LobbyMsg_GetU8(b, LobbyMsg_GetU16(a, gLobbyRecvData));
}

s32 LobbyReq_620B(s32 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_EntryFlag(0U, v);
        return req;
    }
    return -1;
}

s32 NetList0_GetKind(u16 id, u8 *out)
{
    *out = gLobbyNet.list0[(id & 0xFFFF) - 1].kind;
    return 0;
}

/* the reply to 620B and its notification: u16 id, u8; route 0x34 */
void LobbyReply_620B(void)
{
    u16 id;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b2(&id, &v);
        gLobbyNet.list0[id - 1].id = id;
        gLobbyNet.list0[id - 1].kind = v;
        gLobbyNet.list0[id - 1].flags |= 0x400;
    }
    Lobby_RouteMessage(0x34);
}

/* opens the shooting gallery (menu 0x14) */
void TownSave_Open(void)
{
    u8 *w;

    gLobby->connected = 0;
    gTownMode = 0x14;
    gTownMenuOpen = 1;
    w = (u8 *)gMenuWork;
    memset(w, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(TownSave_Draw, 0, 0, 0);
    gFieldControl = 0;
    *(s32 *)(w + 8) = 0;
    *(s32 *)(w + 0xC) = 400;
    *(s32 *)(w + 0x10) = 800;
    if (Game_IsOnline() == 1) {
        ActSpot_PlaceLocal();
    } else {
        LobbySe_Play(0x11);
    }
}
