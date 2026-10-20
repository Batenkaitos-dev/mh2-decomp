/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"
#include "em.h"
#include "player.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "scene.h"

/* prototypes */
void TownMsg_SendContestSeatAsk(Player *arg0);
void TownMsg_SendContestOpen(void *arg0, void *arg1);
void TownMsg_SendContestPower(void *arg0, s32 arg1);
void TownMsg_SendSeatAsk(Player *arg0);
void TownMsg_SendSyncAsk(void);
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

void TownMsg_SendContestSeatAsk(Player *pc) {
    s8 data;

    data = (s8) *(u16 *)pc->spot;
    TownMsg_Send(2, TOWN_MSG_CONTEST_SEAT_ASK, &data);
}

extern s8 gContestSeatId;

extern s8 gTakeSpotId;

extern u8 gNetUsers;
extern s8 D_008C0148;

void TownMsg_SendContestSeatFree(Player *p)
{
    TownMsg_Send(1, TOWN_MSG_CONTEST_SEAT_FREE, &gContestSeatId);
    TownMsg_SendSelf(1, TOWN_MSG_CONTEST_SEAT_FREE, &gContestSeatId);
}

void *memset(void *dst, s32 c, u32 n);
char *strcpy(void *dst, const void *src);

void TownMsg_SendContestSeatAnswer(char *name, u8 a, u8 b)
{
    u8 buf[0x10];

    memset(buf, 0, 0xB);
    buf[0] = a;
    buf[1] = b;
    strcpy(&buf[2], name);
    TownMsg_Send(0xB, TOWN_MSG_CONTEST_SEAT_ANSWER, buf);
    TownMsg_SendSelf(0xB, TOWN_MSG_CONTEST_SEAT_ANSWER, (s8 *)buf);
}

void TownMsg_SendContestSeatTaken(void)
{
    TownMsg_Send(1, TOWN_MSG_CONTEST_SEAT_TAKEN, &gContestSeatId);
    TownMsg_SendSelf(1, TOWN_MSG_CONTEST_SEAT_TAKEN, &gContestSeatId);
}

void TownMsg_SendContestOpen(void *arg0, void *arg1) {
    TownMsg_SendTo(8, TOWN_MSG_CONTEST_OPEN, arg1, arg0);
    TownMsg_SendTo(8, TOWN_MSG_CONTEST_OPEN, arg0, arg1);
}

void TownMsg_SendContestStart(s32 *to)
{
    s8 data;

    TownMsg_SendTo(1, TOWN_MSG_CONTEST_START, &data, to);
}

void TownMsg_SendContestPower(void *to, s32 power) {
    s32 data;

    data = power;
    TownMsg_SendTo(4, TOWN_MSG_CONTEST_POWER, &data, to);
}

void TownMsg_SendContestResult(s32 *to, s8 v)
{
    s8 data;

    data = v;
    TownMsg_SendTo(1, TOWN_MSG_CONTEST_RESULT, &data, to);
    TownMsg_SendSelf(1, TOWN_MSG_CONTEST_RESULT, &data);
}

void TownMsg_SendSeatAsk(Player *pc) {
    s8 data;

    data = (s8) *(u16 *)pc->spot;
    TownMsg_Send(2, TOWN_MSG_SEAT_ASK, &data);
}

void TownMsg_SendSeatFree(Player *pl)
{
    TownMsg_Send(1, TOWN_MSG_SEAT_FREE, &gTakeSpotId);
    TownMsg_SendSelf(1, TOWN_MSG_SEAT_FREE, &gTakeSpotId);
}

void TownMsg_SendSeatAnswer(char *name, u8 a, u8 b)
{
    u8 buf[0x10];

    memset(buf, 0, 0xB);
    buf[0] = a;
    buf[1] = b;
    strcpy(&buf[2], name);
    TownMsg_Send(0xB, TOWN_MSG_SEAT_ANSWER, buf);
    TownMsg_SendSelf(0xB, TOWN_MSG_SEAT_ANSWER, (s8 *)buf);
}

void TownMsg_SendSeatTaken(void)
{
    TownMsg_Send(1, TOWN_MSG_SEAT_TAKEN, &gTakeSpotId);
    TownMsg_SendSelf(1, TOWN_MSG_SEAT_TAKEN, &gTakeSpotId);
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
    TownMsg_Send(0xE, TOWN_MSG_OFFER, &m);
}

void TownMsg_SendOffer(Player *p, void *x)
{
    PlayerNameMsg m;

    memcpy(m.name, (u8 *)gLobby + p->offerTo * 0x11C + 0xA16, 8);
    m.a = p->offerItem;
    m.b = p->offerCount;
    TownMsg_SendTo(0xE, TOWN_MSG_OFFER, &m, x);
}

void TownMsg_SendOfferTake(void *x, Player *p)
{
    PlayerNameMsg m;

    memcpy(m.name, (u8 *)gLobby + p->offerTo * 0x11C + 0xA16, 8);
    m.a = p->offerItem;
    m.b = p->offerCount;
    TownMsg_SendTo(0xE, TOWN_MSG_OFFER_TAKE, &m, x);
}

typedef struct PlayerNameMsgC {
    s16 a;
    s16 b;
    u8 name[8];
    u8 c;
} PlayerNameMsgC;

void TownMsg_SendOfferResult(void *to, u8 c)
{
    PlayerNameMsgC m;
    Player *p = &gPlayers[gScene.localPlayer];

    memcpy(m.name, (u8 *)gLobby + p->offerTo * 0x11C + 0xA16, 8);
    m.a = p->offerItem;
    m.b = p->offerCount;
    m.c = c;
    TownMsg_SendTo(0xE, TOWN_MSG_OFFER_RESULT, &m, to);
}

void TownMsg_SendOfferEnd(void)
{
    TownMsg_Send(0, TOWN_MSG_OFFER_END, 0);
}

void TownMsg_SendSyncAsk(void) {
    s8 data;

    data = Area_GetSetIndex(gScene.area);
    TownMsg_Send(2, TOWN_MSG_SYNC_ASK, &data);
}

void TownMsg_Send0DTo(u8 i)
{
    s8 data;
    s32 temp_a1;

    data = Area_GetSetIndex(gScene.area);
    temp_a1 = (i & 0xFF) * 0x11C;
    if (*(&D_008C0148 + temp_a1) != 0) {
        TownMsg_SendTo(2, TOWN_MSG_SYNC_ASK, &data, (s32 *) (&gNetUsers + temp_a1));
    }
}

void Mem_Copy(void *dst, void *src, s32 n);
s32 Game_IsOnline(void);
s32 Lobby_GetPlace(void);
s32 Lobby_SendHeld(void);
void LobbyNet_SendTownMsg(s32 arg0, u16 arg1);
extern s8 gTownMsgCmd;
extern M2C_UNK gTownMsgData;

/* Sends a town message (NETWORK): gTownMsgCmd = cmd, then len bytes of data;
 * only when the lobby state's +0x3936 is 2, 3 or 7 (and, but for command 3,
 * when Lobby_SendHeld() is not 1). The (s32) cast keeps MWCC from keeping the
 * masked len across the copy (the original masks it again after the call). */
void TownMsg_Send(u8 len, u8 cmd, void *data)
{
    u8 st;

    if (Game_IsOnline() != 0 && Lobby_GetPlace() != 0) {
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
        LobbyNet_SendTownMsg((s32)&gTownMsgCmd, (u8)(len + 1));
    }
}

s32 memcmp(void *, void *, s32);
s32 LobbyReq_TownMsgTo(void *src, void *data, u16 n, void (*onReply)(NetMsg, NetMsg *));
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
        LobbyReq_TownMsgTo(to, &gTownMsgCmd, (u8)(len + 1), (void (*)(NetMsg, NetMsg *))TownMsg_OnSent); /* defined (void): it ignores the reply */
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
void ItemShop_Draw(MenuTask *task);

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