#include "common.h"
#include "player.h"
#include "lobby_state.h"
#include "save.h"
#include "lobby_globals.h"
#include "scene.h"

void TownMsg_Send(u8 len, u8 cmd, void *data);
void TownMsg_SendState(void *unused, u8 flag);

void TownMsg_SendStateChange(void *a0)
{
    TownMsg_SendState(a0, 0);
}

void TownMsg_SendStateNow(Player *p)
{
    TownMsg_SendState(p, 0x1);
}

extern u8 gTakeSpotId;
typedef struct TownActMsg {
    s16 x;
    s16 z;
    u8 rot;
    u8 item;
    u8 state0;
    u8 state1;
    u8 f844;
} TownActMsg;

s32 Town_IsAlone(s16 *unused0);
s32 Area_Is86to88(s32 arg0);
void ActSpot_Release(s32 arg0);
void *memset(void *dst, s32 c, u32 n);
extern s8 gTownMoveTimer;
extern u16 gTownMoveFrames;
extern f32 gTownSentPos;
extern f32 D_008BF128;
extern u8 gTownSentRot;
extern u8 gTownSentItem;
extern u8 gTownSentState0;
extern u8 gTownSentState1;
extern u8 gTownSentF844;


/* 5 words: the town index compare gets v0/v1 swapped */
void TownMsg_SendState(void *unused, u8 flag)
{
    TownActMsg m;
    Player *pl = &gPlayers[gScene.localPlayer];

    m.state0 = pl->stateGroup;
    m.state1 = pl->state;
    m.rot = (u16)(pl->targetYaw >> 8);
    m.f844 = pl->unk844;
    if (pl->stateGroup == 0) {
        switch (pl->state) {
        case 0x6:
        case 0x3:
        case 0x2:
        case 0x4:
        case 0x1:
        case 0x5:
        case 0x11:
        case 0x1D:
            if (flag == 0) {
                return;
            }
        default:
            m.x = pl->pos[0];
            m.z = pl->pos[2];
            break;
        case 0x14:
            m.x = pl->moveTo[0];
            m.z = pl->moveTo[2];
            m.item = gTakeSpotId;
            break;
        case 0x1C:
        case 0x21:
        case 0x22:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x28:
        case 0x16:
        case 0x8:
        case 0x7:
        case 0x9:
        case 0x29:
        case 0x1E:
            m.x = pl->pos[0];
            m.z = pl->pos[2];
            m.item = gTakeSpotId;
            break;
        case 0x15:
        case 0x27:
        case 0x2A:
        case 0x18:
            m.x = pl->pos[0];
            m.z = pl->pos[2];
            m.item = *(u16 *)pl->spot;
            if (Area_Is86to88(gScene.area) == 1) {
                ActSpot_Release(m.item);
            } else {
                u8 *n = (u8 *)(pl->index + (s32)gLobby);
                if (n[0x1538] - '0' == gScene.localPlayer) {
                *(u16 *)((u8 *)gLobby + 0x1562) = *(u16 *)((u8 *)gLobby + 0x1562) & ~(1 << *(u16 *)pl->spot);
                memset((u8 *)gLobby + *(u16 *)pl->spot * 8 + 0x1564, 0, 4);
                }
            }
            break;
        case 0x4A:
        case 0x12:
        case 0x13:
            m.x = gTownSentPos;
            m.z = D_008BF128;
            break;
        }
    } else {
        m.x = pl->pos[0];
        m.z = pl->pos[2];
    }
    if ((s8)Town_IsAlone(&m.x) == 0) {
        gTownMoveTimer = gTownMoveFrames;
        gTownSentPos = m.x;
        D_008BF128 = m.z;
        gTownSentState0 = m.state0;
        gTownSentState1 = m.state1;
        gTownSentRot = m.rot;
        gTownSentF844 = m.f844;
        gTownSentItem = m.item;
        TownMsg_Send(0xA, 1, &m);
    }
}

void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to);
s32 TownPc_IsSeatedState(u8 arg0, u8 arg1);
void ActSpot_Release(s32 arg0);

void TownMsg_SendActor(Player *a, void *to)
{
    TownActMsg m;

    if (a->pos[0] != 0.0f || a->pos[1] != 0.0f || a->pos[2] != 0.0f) {
        m.x = a->pos[0];
        m.z = a->pos[2];
        m.state0 = a->stateGroup;
        m.state1 = a->state;
        m.rot = (u16)(a->targetYaw >> 8);
        m.f844 = a->unk844;
        if (TownPc_IsSeatedState(a->stateGroup, a->state) == 1) {
            m.item = gTakeSpotId;
        } else if (a->stateGroup == 0 && a->state == 0x15) {
            m.item = *(u16 *)a->spot;
            ActSpot_Release(m.item);
        }
        TownMsg_SendTo(0xA, 8, &m, to);
    }
}

void TownMsg_Send10(Player *p)
{
    TownMsg_Send(0x2, 0x10, &p->rot[1]);
}

void *memcpy(void *dst, const void *src, u32 n);
u8 Area_GetSetIndex(u16 id);
extern u8 gNetUsers[];

void TownMsg_SendArea(u16 id)
{
    u8 buf[0x10];
    u8 *s = (u8 *)gLobby;
    s8 i = s[gScene.localPlayer + 0x1538] - '0';

    buf[0] = Area_GetSetIndex(id);
    memcpy(&buf[1], &gNetUsers[i * 0x11C], 8);
    TownMsg_Send(9, 3, buf);
}

void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to);

/* TownMsg_SendArea to one member: command 3, the area and the player's name. */
void TownMsg_SendAreaTo(void *to, u16 id)
{
    u8 buf[0x10];
    u8 *s = (u8 *)gLobby;
    s8 i = s[gScene.localPlayer + 0x1538] - '0';

    buf[0] = Area_GetSetIndex(id);
    memcpy(&buf[1], &gNetUsers[i * 0x11C], 8);
    TownMsg_SendTo(9, 3, buf, to);
}

void *memcpy(void *dst, const void *src, u32 n);
extern u8 gMyName[];

void TownMsg_Send0E(void)
{
    u8 buf[0x20];

    memcpy(buf, gMyName, 0x10);
    TownMsg_Send(0x10, 0xE, buf);
    TownMsg_SendStateNow(&gPlayers[gScene.localPlayer]);
}

void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to);
extern u8 gNetUsers[];

void TownMsg_Send2B(u8 c)
{
    u8 b = c;
    u8 *s = (u8 *)gLobby;

    TownMsg_SendTo(1, 0x2B, &b, &gNetUsers[(s8)(s[gScene.localPlayer + 0x1538] - '0') * 0x11C]);
}

extern s8 gTownMoveTimer;
extern u16 gTownMoveFrames;
extern f32 gTownSentPos;
extern f32 D_008BF128;
extern u8 gTownSentRot;
extern u8 gTownSentItem;
extern u8 gTownSentState0;
extern u8 gTownSentState1;
extern u8 gTownSentF844;
void Bits_Pack(u8 *dst, u8 *src, s32 n);
void Records_TallyWeapons(s32 *out);
void Records_TallyArmor(s32 *arg);

/* Sends one kind of the player's records to a member (NETWORK): the monster
 * sizes and counts (kind 2), bit fields of the save (10, 12-17), ... */
void TownMsg_SendRecords(void *to, u8 kind)
{
    u8 big[0x500];
    u8 small[0x30];
    s32 i;
    u8 *d;

    switch (kind) {
    case 4:
        Records_TallyWeapons((s32 *) small);
        TownMsg_SendTo(0x28, 0x3A, small, to);
        break;
    case 5:
        Records_TallyArmor((s32 *) small);
        TownMsg_SendTo(0x28, 0x3B, small, to);
        break;
    case 3:
        TownMsg_SendTo(0x80, 0x2C, (u8 *)gSave.itemBits, to);
        break;
    case 6:
        TownMsg_SendTo(0x20, 0x2D, gSave.unk3660, to);
        break;
    case 2:
        d = big;
        for (i = 0; i < 0x4A; i++) {
            *d++ = gSave.sizeMax[i];
        }
        TownMsg_SendTo(0x4A, 0x2E, big, to);
        d = big;
        for (i = 0; i < 0x4A; i++) {
            *d++ = gSave.sizeMin[i];
        }
        TownMsg_SendTo(0x4A, 0x2F, big, to);
        memcpy(big, gSave.hunted, 0x100);
        for (i = 0; i < 0x4A; i++) {
            ((u16 *)big)[i] += gSave.captured[i];
        }
        TownMsg_SendTo(0x94, 0x30, gSave.hunted, to);
        break;
    case 10:
        Bits_Pack(big, gSave.armorFlags[4], 0x280);
        TownMsg_SendTo(0x50, 0x32, big, to);
        break;
    case 12:
        Bits_Pack(big, gSave.armorFlags[0], 0x280);
        TownMsg_SendTo(0x50, 0x33, big, to);
        break;
    case 13:
        Bits_Pack(big, gSave.armorFlags[1], 0x280);
        TownMsg_SendTo(0x50, 0x34, big, to);
        break;
    case 14:
        Bits_Pack(big, gSave.armorFlags[2], 0x280);
        TownMsg_SendTo(0x50, 0x35, big, to);
        break;
    case 15:
        Bits_Pack(big, gSave.armorFlags[3], 0x280);
        TownMsg_SendTo(0x50, 0x36, big, to);
        break;
    case 16:
        Bits_Pack(big, gSave.bladeFlags, 0x280);
        TownMsg_SendTo(0x50, 0x37, big, to);
        break;
    case 17:
        Bits_Pack(big, gSave.gunFlags, 0x280);
        TownMsg_SendTo(0x50, 0x38, big, to);
        break;
    case 0:
    case 1:
    case 7:
    case 8:
    case 9:
    case 11:
        break;
    }
}

extern u8 gLobbyLevel2Sel[];

void TownMsg_Send27(void)
{
    s8 i;
    s32 off;
    u8 *p;

    for (i = 0, off = 0; i < 4; i++, off += 0x11C) {
        p = (u8 *)gLobby + off;
        if ((s8)p[0x5A6] != 0) {
            TownMsg_SendTo(1, 0x27, gLobbyLevel2Sel, p + 0x5A6);
        }
    }
}

/* The held item of a townsperson (its route point's first u16) to the
 * player's slot, command 0x15. */
void TownMsg_Send15(Player *a)
{
    u8 *s = (u8 *)gLobby;
    u8 b = *(u16 *)a->spot;

    TownMsg_SendTo(2, 0x15, &b, &gNetUsers[(s8)(s[gScene.localPlayer + 0x1538] - '0') * 0x11C]);
}

s8 MyHouse_LocalSlot(void);
void TownMsg_SendSelf(u8 len, u8 cmd, void *data);
void *memset(void *dst, s32 c, u32 n);
extern u8 gTakeSpotId;

void TownMsg_Send17(void *pl)
{
    u8 buf[9];
    s8 i;

    memset(buf, 0, 9);
    buf[0] = gTakeSpotId;
    i = MyHouse_LocalSlot();
    if (i != -1) {
        memcpy(&buf[1], &gNetUsers[i * 0x11C], 8);
        TownMsg_Send(9, 0x17, buf);
        TownMsg_SendSelf(9, 0x17, buf);
    }
}

void *memset(void *dst, s32 c, u32 n);
char *strcpy(void *dst, const void *src);
void TownMsg_SendSelf(u8 len, u8 cmd, void *data);

void TownMsg_Send16(char *name, u8 a, u8 b)
{
    u8 buf[0x10];

    memset(buf, 0, 0xB);
    buf[0] = a;
    buf[1] = b;
    strcpy(&buf[2], name);
    TownMsg_Send(0xB, 0x16, buf);
    TownMsg_SendSelf(0xB, 0x16, buf);
}

void TownMsg_Send18(void)
{
    u8 buf[9];
    s8 i;

    memset(buf, 0, 9);
    buf[0] = gTakeSpotId;
    i = MyHouse_LocalSlot();
    if (i != -1) {
        memcpy(&buf[1], &gNetUsers[i * 0x11C], 8);
        TownMsg_Send(9, 0x18, buf);
        TownMsg_SendSelf(9, 0x18, buf);
    }
}