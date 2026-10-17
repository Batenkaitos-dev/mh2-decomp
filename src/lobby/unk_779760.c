#include "common.h"
#include "em.h"
#include "player.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "scene.h"
#include "chat_log.h"
#include "area.h"

typedef struct Delivery {
    /* 0x0 */ u16 item;
    /* 0x2 */ s16 count;
    /* 0x4 */ u8 unk4[8];
    /* 0xC */ u8 done;
    /* 0xD */ u8 unkD;
} Delivery;

extern NetUser gNetUsers[];

void *memcpy(void *dst, const void *src, u32 n);
/* unit's own declaration: with the definition's u8 (u16 item, s16 n) (sub_main/unk_3B4C80.c), Delivery_Take: 2 of 45 words differ */
u8 SavePouch_Add(u16 item, s32 n);
u8 Notice_Push(u8, s16, s16);
s16 Player_IsState(void *obj, u8 state, u8 sub);
u8 Lobby_FindMember(u8 *arg0);
void Chat_AddReceived(u8 arg0, s8 *arg1);
void ChatLog_Receive(ChatMsg *msg);
Em *Npc_Find(u8 kind);
void Actor_SetStateNow(struct Player *obj, u8 state, u8 sub, u16 flags);
s32 memcmp(void *, void *, s32);
s32 Lobby_GetPlace3928(void);
void Chat_SystemMsg(s32 msg, char *arg);
extern u8 gSongRequesterId[];
s32 LobbyBgm_IsActive(void);
void TownMsg_Send25(void *src, s32 x, u8 c);
extern u8 gMyUserId[];
void SongMenu_OnBusy(void);
void SongMenu_OnAccepted(void);
void TownMsg_SendBgm(s32 arg0);

void Actor_StartWalk(struct Player *obj, s16 n);
f32 Math_Atan2(f32 y, f32 x);
f32 Vec_Length(f32 *v);
s32 TownPc_IsSeatedState(u8 arg0, u8 arg1);
s32 Area_Is86to88(s32 arg0);
void Mem_Copy(void *, void *, s32);
extern s16 gTownMoveFrames;

typedef struct PosMsg {
    s16 x;
    s16 z;
    u8 area;
} PosMsg;

typedef struct Vec3f {
    f32 x, y, z;
} Vec3f;
extern u16 gMsgAreaIds[];


void Lobby_ShowMemberName(s8 idx, void *data)
{
    if (Lobby_GetPlace3928() != 0 && gNetUsers[idx].name[0] != 0) {
        if (gChatHideNames != 0) {
            Chat_SystemMsg(3, gNetUsers[idx].id);
        } else {
            Chat_SystemMsg(3, gNetUsers[idx].name);
        }
    }
}

void TownMsg_On24(void *src, void *data)
{
    s32 v;

    if (memcmp(gLobby->myId, gLobby->hostId, 8) == 0) {
        memcpy(&v, data, 4);
        if (LobbyBgm_IsActive() == 0 && gSongRequesterId[0] == 0) {
            memcpy(gSongRequesterId, src, 8);
            TownMsg_Send25(src, v, 0);
        } else {
            TownMsg_Send25(src, v, 1);
        }
    }
}

void TownMsg_On25(s8 *data)
{
    s32 v;

    if (data[0] == 1 && memcmp(data + 5, gMyUserId, 8) == 0) {
        SongMenu_OnBusy();
        return;
    }
    memcpy(gSongRequesterId, data + 5, 8);
    if (memcmp(gSongRequesterId, gMyUserId, 8) == 0) {
        memcpy(&v, data + 1, 4);
        TownMsg_SendBgm(v);
        SongMenu_OnAccepted();
    }
}

typedef struct V00779960_0 {
    u16 a;
    u16 b;
    u8 id[10];
} V00779960_0;

void TownMsg_On0A(s8 idx, void *src)
{
    Player *pl = &gPlayers[idx];
    V00779960_0 d;

    if (pl->active != 0) {
        memcpy(&d, src, 0xE);
        pl->offerTo = Lobby_FindMember(d.id);
        pl->offerItem = d.a;
        pl->offerCount = d.b;
        Actor_SetStateNow(pl, 0, 0xA, 0x10);
    }
}

s16 Pouch_Count(Player *p, u16 item);
void TownMsg_Send0C(void *to, u8 c);

/* The answer to TownMsg_On0A's request (the same 0xE-byte record): when
 * the local player is still in state 0xA for that member, item and count,
 * and has the items, they leave the pouch and TownMsg_Send0C answers 0 (the
 * player's +0x862 = 0xFF); else it answers 1. */
void TownMsg_On0B(void *to, void *src)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    V00779960_0 d;

    memcpy(&d, src, 0xE);
    if (Player_IsState(pl, 0, 0xA) != 0 && pl->offerTo == Lobby_FindMember(d.id)
        && pl->offerItem == d.a) {
        u16 n = d.b;

        if (Pouch_Count(pl, d.a) >= n && pl->offerCount == n && pl->offerState == 0) {
            SavePouch_Add(d.a, (s16)-n);
            TownMsg_Send0C(to, 0);
            pl->offerState = 0xFF;
            return;
        }
    }
    TownMsg_Send0C(to, 1);
}

void Delivery_Take(void *src)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    Delivery d;

    if (Player_IsState(pl, 0, 0xB) != 0) {
        memcpy(&d, src, sizeof(d));
        if (d.done == 0) {
            pl->offerState = 1;
            SavePouch_Add(d.item, d.count);
            Notice_Push(1, 0xD, d.item);
        } else {
            pl->offerState = 2;
        }
    }
}

void HunterCard_Apply(s8 idx, void *src);
void TownPc_Spawn(u8 pl, NetUser *u, char *name);
void TownPc_FreeChangedModels(Player *a0);

/* A member's 0x100-byte record (LobbyState +0xA16: members of 0x11C bytes,
 * the record at +0x1C, the name at +8) received: when its bytes 0x74-0x7F
 * or 0x80-0xC7 changed, HunterCard_Apply and TownPc_Spawn (and
 * TownPc_FreeChangedModels for a member in the village, returning 1); then it is
 * kept, and for the local player copied to gPlayerText. */
s32 LobbyMember_SetRecord(s8 idx, NetUser *a1, u8 *rec)
{
    u8 *t;
    s32 ret;
    s32 off;

    off = idx * 0x11C;
    t = (u8 *)(gLobby->members[0].id + off + 0x1C);
    ret = 0;
    if (memcmp(t + 0x80, rec + 0x80, 0x48) != 0 || memcmp(t + 0x74, rec + 0x74, 0xC) != 0) {
        if ((s8)gLobby->pcLoaded[idx] == 0) {
            HunterCard_Apply(idx, rec);
            TownPc_Spawn(idx, a1, gLobby->members[0].id + off + 8);
        } else {
            HunterCard_Apply(idx, rec);
            TownPc_FreeChangedModels(&gPlayers[idx]);
            TownPc_Spawn(idx, a1, gLobby->members[0].id + off + 8);
            ret = 1;
        }
        gLobby->pcLoaded[idx] = 0;
    } else {
        HunterCard_Apply(idx, rec);
    }
    memcpy(t, rec, 0x100);
    if (idx == gScene.localPlayer) {
        memcpy(&gPlayerText.card, rec, sizeof(HunterCard));
    }
    return ret;
}

void Chat_Receive(u8 *msg)
{
    u8 idx;

    if (msg[0x120] == 2) {
        idx = Lobby_FindMember(msg);
        Chat_AddReceived(idx, (s8 *)msg);
    } else if (msg[0x120] == 1) {
        ChatLog_Receive((ChatMsg *)msg);
    }
}

u8 Lobby_FindMember(u8 *id)
{
    u8 i;

    if (*(s8 *)id == 0) {
        return 0xFF;
    }
    for (i = 0; i < 10; i++) {
        if (memcmp(gNetUsers[i].id, id, 6) == 0) {
            return i;
        }
    }
    return 0xFF;
}

extern u8 gNpc14Left;
extern s32 gTownMode;
extern M2C_UNK D_008C0148;
void ActSpot_Take(u8 slot);
void ActSpot_Release(s32 slot);
s32 TownPc_IsLeaveSeatState(s32 a, s32 b);
void *memset(void *d, s32 c, u32 n);
void Actor_SetState(void *, s32, s32);
typedef struct ActMsg {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 z;
    /* 0x4 */ u8 angle;
    /* 0x5 */ u8 slot;
    /* 0x6 */ u8 group;
    /* 0x7 */ u8 state;
    /* 0x8 */ u8 item;
} ActMsg;

/* Commands 1 and 8 (TownMsg_SendActor): a member's position, angle,
 * held item and state. The member walks there and takes the state; a
 * ready (or unready) state sets the member's ready bit when it is in the
 * player's own group. */
void TownMsg_OnActor(u8 pl, void *data)
{
    Player *p = &gPlayers[pl];
    u8 buf[10];
    ActMsg *q;
    u16 a;

    Mem_Copy(buf, data, 10);
    q = (ActMsg *)buf;
    if (q->item != 0 && Area_Is86to88(p->area) == 0) {
        return;
    }
    a = q->angle << 8;
    p->netActive = 1;
    if (p->pos[0] == 0.0f && p->pos[1] == 0.0f) {
        p->rot[1] = a;
        p->targetYaw = a;
    } else {
        p->targetYaw = a;
    }
    p->moveTo[0] = q->x;
    p->moveTo[2] = q->z;
    p->unk844 = q->item;
    Actor_StartWalk((struct Player *)((u8 *) p), 10);
    if (TownPc_IsSeatedState(q->group, q->state) == 1) {
        if ((u8)gLobby->memberSlot[pl] == (s8)(gScene.localPlayer + '0')) {
            gLobby->readyBits = gLobby->readyBits | (1 << q->slot);
            memcpy(&gLobby->slotIds[q->slot * 8], gNetUsers[pl].id, 8);
        } else {
            ActSpot_Take(q->slot);
        }
    } else if (TownPc_IsLeaveSeatState(q->group, q->state) == 1) {
        if ((u8)gLobby->memberSlot[pl] == (s8)(gScene.localPlayer + '0')) {
            gLobby->readyBits = gLobby->readyBits & ~(1 << q->slot);
            memset(&gLobby->slotIds[q->slot * 8], 0, 8);
        } else {
            ActSpot_Release(q->slot);
        }
    }
    if (q->state == 0x14 && p->stateGroup == 0 && q->state != p->state) {
        Actor_SetStateNow((struct Player *)((u8 *) p), 0, 0x14, 0);
        return;
    }
    if (p->state == q->state && p->stateGroup == q->group) {
        return;
    }
    if (gTownMode == 0x30 && q->group == 0 && q->state == 0x6F) {
        return;
    }
    Actor_SetState(p, q->group, q->state);
}

/* Commands 0 and 2: a member's position (x, z) and area index. Command 0
 * walks the member there when it is idle in the player's area; otherwise
 * it turns toward the point and jumps there. */
void TownMsg_OnMove(u8 pl, void *data, u8 cmd)
{
    Player *p = &gPlayers[pl];
    PosMsg msg;
    f32 d[3];
    PosMsg *q;

    Mem_Copy(&msg, data, 5);
    if (p->netActive != 0) {
        q = &msg;
        if (cmd == 0 && p->area == gScene.area) {
            if (TownPc_IsSeatedState(p->stateGroup, p->state) == 0) {
                p->moveTo[0] = q->x;
                p->moveTo[2] = q->z;
                Actor_StartWalk((struct Player *)((u8 *) p), gTownMoveFrames);
                p->area = gMsgAreaIds[q->area];
                if (Area_Is86to88(p->area) == 0) {
                    p->unk844 = 0;
                    p->unk867 = 0;
                }
            }
        } else {
            d[0] = p->moveTo[0] - p->pos[0];
            d[1] = 0.0f;
            d[2] = p->moveTo[2] - p->pos[2];
            p->moveToFrames = 0;
            if (!(Vec_Length(d) <= 5.0f)) {
                s32 a = 0.5f + 65536.0f * Math_Atan2(d[0], d[2]) / 6.2831855f;
                p->targetYaw = a;
                p->rot[1] = a & 0xFFFF;
            }
            p->moveTo[0] = q->x;
            p->moveTo[2] = q->z;
            *(Vec3f *)p->pos = *(Vec3f *)p->moveTo;
            *(Vec3f *)p->prevPos = *(Vec3f *)p->pos;
        }
    }
}

typedef struct V0077A330_0 {
    s16 x;
    s16 z;
    s16 unk4;
} V0077A330_0;

void TownMsg_On39(u8 unused, void *data)
{
    Em *em = Npc_Find(0x14);
    V0077A330_0 pos;

    memcpy(&pos, data, 6);
    if (em != NULL) {
        em->posX = pos.x;
        em->posZ = pos.z;
    }
}

void TownMsg_Send39(Em *pl, void *to);
s32 Area_IsOnlineHouse(u16 arg0); /* unit's own declaration: with the definition's s32 (s32 area) (lobby/unk_7949D0.c), TownMsg_OnArea 1 of 160 words differ */
void Actor_SetAction(void *a, u16 mode, s32 b, s32 c);
void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);

/* Command 3 (TownMsg_SendArea): the area a member moved to (an index
 * into gMsgAreaIds) and its id: its group slot, its area, and the name
 * shown when it comes back to the town (area 0). */
void TownMsg_OnArea(u8 pl, u8 *msg)
{
    Player *p = &gPlayers[pl];
    Player *me = &gPlayers[gScene.localPlayer];
    u16 area = gMsgAreaIds[msg[0]];
    Em *em = Npc_Find(0x14);
    s8 slot;

    if (Area_IsOnlineHouse(area) == 1) {
        slot = Lobby_FindMember(msg + 1);
        gLobby->memberSlot[pl] = slot + '0';
        if (slot == gScene.localPlayer && gOnlineBonusState != 1 && gNpc14Left != 1) {
            TownMsg_Send39(em, &gNetUsers[pl]);
        }
    } else {
        gLobby->memberSlot[pl] = 0;
    }
    if (area != p->area) {
        if (p->area == 0) {
            p->netActive = 1;
        }
        p->rot[1] = (u16)p->targetYaw;
        p->area = area;
        if (Area_Is86to88(p->area) == 0 && p->active != 0) {
            p->unk844 = 0;
            Actor_SetAction(p, 0, 0, 0);
            Actor_SetMotion(p, 1, 0, 0);
            Actor_SetState(p, 0, 1);
        }
        p->unk844 = 0;
        p->unk867 = 0;
        if (me->target != NULL && ((Player *)me->target)->index == pl && ((Player *)me->target)->ownTask == 0) {
            me->target = NULL;
        }
        if (area == 0 && Lobby_GetPlace3928() != 0 && gNetUsers[pl].name[0] != 0) {
            if (gChatHideNames != 0) {
                Chat_SystemMsg(7, (char *) gNetUsers[pl].id);
            } else {
                Chat_SystemMsg(7, (char *) gNetUsers[pl].name);
            }
        }
    }
}

extern u8 gTownSeats[];
void TownMsg_Send1C(char *name, u8 a, u8 b);
void TownMsg_Send1F(void *a, void *b);

/* A town message taking seat msg[0] of gTownSeats (the member index +
 * 0x30) while the local player hosts (LobbyState +0x48A == +0x1E): answered
 * by TownMsg_Send1C with 1, or 0 when the seat is taken; with seats 1 and 2
 * both taken, TownMsg_Send1F gets their two members. */
void TownMsg_On1B(void *id, u8 *msg)
{
    u8 k;
    u8 *p;

    if (memcmp(gLobby->myId, gLobby->hostId, 8) == 0) {
        k = msg[0];
        p = gTownSeats + k;
        if (*p != 0) {
            TownMsg_Send1C(id, 0, k);
            return;
        }
        *p = Lobby_FindMember(id) + 0x30;
        TownMsg_Send1C(id, 1, k);
        if (gTownSeats[1] != 0 && gTownSeats[2] != 0) {
            TownMsg_Send1F(&gNetUsers[(u8)(gTownSeats[2] - 0x30)], &gNetUsers[(u8)(gTownSeats[1] - 0x30)]);
        }
    }
}

void Actor_SetIdle(void *, s32, s32, s32);
void Contest_TakeSeat(void);
void TownMsg_Send1D(struct Player *arg0);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

void TownMsg_On1C(s8 idx, u8 *msg)
{
    s32 me;
    s32 k;

    if (msg[0] == 0) {
        me = gScene.localPlayer;
        if (me == Lobby_FindMember(msg + 2) && gTownMode == 0x36) {
            gTownMode = 0;
            gTownMenuOpen = 0;
            gFieldControl = 1;
            Actor_SetIdle(&gPlayers[(u8)me], 0, 0x14, 0);
        }
        return;
    }
    k = msg[1];
    gTownSeats[(u8)k] = Lobby_FindMember(msg + 2) + 0x30;
    me = gScene.localPlayer;
    if (me == Lobby_FindMember(msg + 2)) {
        if (gScene.area == AREA_HUB_0x86 && gTownMode == 0x36) {
            Contest_TakeSeat();
            return;
        }
        TownMsg_Send1D(&gPlayers[(u8)me]);
        return;
    }
}


void TownMsg_On1E(u8 *id, u8 *data)
{
    u8 idx = *data;

    gTownSeats[idx] = Lobby_FindMember(id) + 0x30;
}
