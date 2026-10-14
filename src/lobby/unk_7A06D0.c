/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_menu.h"
#include "em.h"
#include "player.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "scene.h"

/* prototypes */
void TownMsg_Send1B(Player *arg0);
void TownMsg_Send1F(void *arg0, void *arg1);
void TownMsg_Send20(void *arg0, s32 arg1);
void TownMsg_Send04(Player *arg0);
void TownMsg_Send0D(void);
void TownMsg_OnSent(void);
void TownMsg_SendSelf(u8 len, u8 cmd, void *data);
void TownMenu_Close(void);
/* end of prototypes */

void TownMsg_Send(u8 len, u8 cmd, void *data);
void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to);
u8 Area_GetSetIndex(u16 id);
void CamFocus_End(void);
void LobbySe_Play(s32);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

void TownMsg_Send1B(Player *pc) {
    s8 data;

    data = (s8) *(u16 *)pc->spot;
    TownMsg_Send(2, 0x1B, &data);
}

extern s8 gTownSpotId;

extern s8 gTakeSpotId;

extern u8 gNetUsers;
extern s8 D_008C0148;

void TownMsg_Send1D(Player *p)
{
    TownMsg_Send(1, 0x1D, &gTownSpotId);
    TownMsg_SendSelf(1, 0x1D, &gTownSpotId);
}

void *memset(void *dst, s32 c, u32 n);
char *strcpy(void *dst, const void *src);

void TownMsg_Send1C(char *name, u8 a, u8 b)
{
    u8 buf[0x10];

    memset(buf, 0, 0xB);
    buf[0] = a;
    buf[1] = b;
    strcpy(&buf[2], name);
    TownMsg_Send(0xB, 0x1C, buf);
    TownMsg_SendSelf(0xB, 0x1C, (s8 *)buf);
}

void TownMsg_Send1E(void)
{
    TownMsg_Send(1, 0x1E, &gTownSpotId);
    TownMsg_SendSelf(1, 0x1E, &gTownSpotId);
}

void TownMsg_Send1F(void *arg0, void *arg1) {
    TownMsg_SendTo(8, 0x1F, arg1, arg0);
    TownMsg_SendTo(8, 0x1F, arg0, arg1);
}

void TownMsg_Send22(s32 *to)
{
    s8 data;

    TownMsg_SendTo(1, 0x22, &data, to);
}

void TownMsg_Send20(void *to, s32 power) {
    s32 data;

    data = power;
    TownMsg_SendTo(4, 0x20, &data, to);
}

void TownMsg_Send21(s32 *to, s8 v)
{
    s8 data;

    data = v;
    TownMsg_SendTo(1, 0x21, &data, to);
    TownMsg_SendSelf(1, 0x21, &data);
}

void TownMsg_Send04(Player *pc) {
    s8 data;

    data = (s8) *(u16 *)pc->spot;
    TownMsg_Send(2, 4, &data);
}

void TownMsg_Send07(Player *pl)
{
    TownMsg_Send(1, 7, &gTakeSpotId);
    TownMsg_SendSelf(1, 7, &gTakeSpotId);
}

void TownMsg_Send05(char *name, u8 a, u8 b)
{
    u8 buf[0x10];

    memset(buf, 0, 0xB);
    buf[0] = a;
    buf[1] = b;
    strcpy(&buf[2], name);
    TownMsg_Send(0xB, 5, buf);
    TownMsg_SendSelf(0xB, 5, (s8 *)buf);
}

void TownMsg_Send0F(void)
{
    TownMsg_Send(1, 0xF, &gTakeSpotId);
    TownMsg_SendSelf(1, 0xF, &gTakeSpotId);
}

typedef struct PlayerNameMsg {
    s16 a;
    s16 b;
    u8 name[8];
} PlayerNameMsg;
void *memcpy(void *dst, const void *src, u32 n);

void TownMsg_Send0AAll(Player *p)
{
    PlayerNameMsg m;

    memcpy(m.name, (u8 *)gLobby + p->offerTo * 0x11C + 0xA16, 8);
    m.a = p->offerItem;
    m.b = p->offerCount;
    TownMsg_Send(0xE, 0xA, &m);
}

void TownMsg_Send0A(Player *p, void *x)
{
    PlayerNameMsg m;

    memcpy(m.name, (u8 *)gLobby + p->offerTo * 0x11C + 0xA16, 8);
    m.a = p->offerItem;
    m.b = p->offerCount;
    TownMsg_SendTo(0xE, 0xA, &m, x);
}

void TownMsg_Send0B(void *x, Player *p)
{
    PlayerNameMsg m;

    memcpy(m.name, (u8 *)gLobby + p->offerTo * 0x11C + 0xA16, 8);
    m.a = p->offerItem;
    m.b = p->offerCount;
    TownMsg_SendTo(0xE, 0xB, &m, x);
}

typedef struct PlayerNameMsgC {
    s16 a;
    s16 b;
    u8 name[8];
    u8 c;
} PlayerNameMsgC;

void TownMsg_Send0C(void *to, u8 c)
{
    PlayerNameMsgC m;
    Player *p = &gPlayers[gScene.localPlayer];

    memcpy(m.name, (u8 *)gLobby + p->offerTo * 0x11C + 0xA16, 8);
    m.a = p->offerItem;
    m.b = p->offerCount;
    m.c = c;
    TownMsg_SendTo(0xE, 0xC, &m, to);
}

void TownMsg_Send12(void)
{
    TownMsg_Send(0, 0x12, 0);
}

void TownMsg_Send0D(void) {
    s8 data;

    data = Area_GetSetIndex(gScene.area);
    TownMsg_Send(2, 0xD, &data);
}

void TownMsg_Send0DTo(u8 i)
{
    s8 data;
    s32 temp_a1;

    data = Area_GetSetIndex(gScene.area);
    temp_a1 = (i & 0xFF) * 0x11C;
    if (*(&D_008C0148 + temp_a1) != 0) {
        TownMsg_SendTo(2, 0xD, &data, (s32 *) (&gNetUsers + temp_a1));
    }
}

void Mem_Copy(void *dst, void *src, s32 n);
s32 Game_IsOnline(void);
s32 Lobby_GetPlace3928(void);
s32 Lobby_SendHeld(void);
void LobbyNet_Send6708(s32 arg0, u16 arg1);
extern s8 gTownMsgCmd;
extern M2C_UNK gTownMsgData;

/* Sends a town message (NETWORK): gTownMsgCmd = cmd, then len bytes of data;
 * only when the lobby state's +0x3936 is 2, 3 or 7 (and, but for command 3,
 * when Lobby_SendHeld() is not 1). The (s32) cast keeps MWCC from keeping the
 * masked len across the copy (the original masks it again after the call). */
void TownMsg_Send(u8 len, u8 cmd, void *data)
{
    u8 st;

    if (Game_IsOnline() != 0 && Lobby_GetPlace3928() != 0) {
        if (cmd == 3 || Lobby_SendHeld() != 1) {
            st = gLobby->phase;
            if (st == 2 || st == 3 || st == 7) {
                goto send;
            }
        }
        return;
send:
        gTownMsgCmd = cmd;
        if ((s32)len != 0) {
            Mem_Copy(&gTownMsgData, (void *)((M2C_UNK)data), len);
        }
        LobbyNet_Send6708((s32)&gTownMsgCmd, (u8)(len + 1));
    }
}

s32 memcmp(void *, void *, s32);
s32 LobbyReq_670D(void *src, void *data, u16 n, void (*onReply)(NetMsg, NetMsg *));
void TownMsg_OnSent(void);
extern M2C_UNK gMyUserId;

/* Sends a town message to one member (NETWORK): to the player itself
 * (the 8-byte id at gMyUserId) it goes straight to TownMsg_SendSelf. */
void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to)
{
    if (Game_IsOnline() != 0 && Lobby_SendHeld() != 1) {
        gTownMsgCmd = cmd;
        if ((s32)len != 0) {
            Mem_Copy(&gTownMsgData, (void *)((M2C_UNK)data), len);
        }
        if (memcmp(&gMyUserId, to, 8) == 0) {
            TownMsg_SendSelf((u8)(len + 1), cmd, data);
            return;
        }
        LobbyReq_670D(to, &gTownMsgCmd, (u8)(len + 1), (void (*)(NetMsg, NetMsg *))TownMsg_OnSent); /* defined (void): it ignores the reply */
    }
}

void TownMsg_OnSent(void) {

}

void TownMsg_Recv(void *from, u8 *pkt);
/* A town message to the player itself: handled at once (TownMsg_Recv) as
 * if received from the player's own id (gMyUserId). */
void TownMsg_SendSelf(u8 len, u8 cmd, void *data)
{
    if (Game_IsOnline() != 0) {
        gTownMsgCmd = cmd;
        Mem_Copy(&gTownMsgData, (void *)((M2C_UNK)data), len);
        TownMsg_Recv(&gMyUserId, (u8 *) &gTownMsgCmd);
    }
}

void *memset(void *dst, s32 c, u32 n);
extern u8 gMenuWork[];
extern u8 gMenuInputLock;
void ItemShop_Draw(u8 *);

/* Opens a townsperson's menu (task ItemShop_Draw, menu 2 in the shared work
 * area gMenuWork, a ShopMenu): the townsperson, its id (npc and list.kind), cur
 * the first choice when sel >= 0; the menu's sound unless reopened from 0x2F. */
void ItemShop_Open(u8 *npc, s8 sel)
{
    ShopMenu *w;
    u8 *kind;
    u8 again;

    w = (ShopMenu *)gMenuWork;
    kind = npc + 0x438;
    again = 0;
    gTownMenuOpen = 1;
    if (gTownMode == 0x2F) {
        again = 1;
    }
    gTownMode = 2;
    memset(gMenuWork, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(ItemShop_Draw, 0, 0, 0);
    gMenuInputLock = 6;
    w->person = npc;
    w->npc = *kind;
    w->list.kind = *kind;
    if (sel >= 0) {
        w->cur = sel;
    }
    if (*kind == 0xD) {
        LobbySe_Play(0x1B);
    } else if (again == 0) {
        LobbySe_Play(0x11);
    }
}

void TownMenu_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(4);
}