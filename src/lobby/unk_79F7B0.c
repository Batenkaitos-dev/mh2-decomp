/*
 * The town's senders of the player's state and records (TOWN_MSG_*,
 * lobby_net.h): its state, position and look to every member or to one,
 * its facing, its area, its name as it arrives, the records another member
 * asks for, the arena quest's opening, and the house seats asked for, given,
 * taken and left. TownMsg_Send sends to every member, TownMsg_SendTo to
 * one and TownMsg_SendSelf to the player itself.
 */
#include "common.h"
#include "player.h"
#include "lobby_state.h"
#include "save.h"
#include "lobby_globals.h"
#include "scene.h"
#include "lobby_net.h"

void TownMsg_Send(u8 len, u8 cmd, void *data);
void TownMsg_SendState(void *unused, u8 flag);

/* The player's state after it changed (TownMsg_SendState): not sent for
 * the plain states. */
void TownMsg_SendStateChange(void *unused)
{
    TownMsg_SendState(unused, 0);
}

/* The player's state, sent whatever it is (TownMsg_SendState). */
void TownMsg_SendStateNow(Player *pl)
{
    TownMsg_SendState(pl, 0x1);
}

extern u8 gTakeSpotId;
/* TOWN_MSG_ACTOR's data (0xA bytes sent). */
typedef struct TownActMsg {
    s16 x;
    s16 z;
    u8 rot;         /* the facing's high byte */
    u8 spot;        /* the seat or action spot held (gTakeSpotId, the spot's type) */
    u8 stateGroup;
    u8 state;
    u8 drunk;       /* Player.drunk */
} TownActMsg;

s32 Town_IsAlone(s16 *unused0);
s32 Area_IsTownHall(s32 arg0);
void ActSpot_Release(s32 arg0);
void *memset(void *dst, s32 c, u32 n);
extern s8 gTownMoveTimer;
extern u16 gTownMoveFrames;
extern f32 gTownSentPos[3];
extern u8 gTownSentRot;
extern u8 gTownSentItem;
extern u8 gTownSentState0;
extern u8 gTownSentState1;
extern u8 gTownSentDrunk;

/*
 * The player's state, position, facing and drunkenness to every member
 * (TOWN_MSG_ACTOR), unless the player is alone; what is sent is kept
 * (gTownSent*). In group 0: the plain states (idle, walking, running,
 * turning, the drunk's idle) are sent only with `flag`; sitting down sends
 * where the player goes, and the seats and spot states the seat taken;
 * standing up and leaving a meal give up the action spot (in a town hall)
 * or the house seat; the drunk's slump and getting up keep the position
 * last sent.
 */
void TownMsg_SendState(void *unused, u8 flag)
{
    TownActMsg msg;
    Player *pl = &gPlayers[gScene.localPlayer];

    msg.stateGroup = pl->stateGroup;
    msg.state = pl->state;
    msg.rot = (u16)(pl->targetYaw >> 8);
    msg.drunk = pl->drunk;
    if (pl->stateGroup == 0) {
        switch (pl->state) {
        case TOWN_PC_RUN_STOP:
        case TOWN_PC_WALK:
        case 0x2:
        case TOWN_PC_RUN:
        case TOWN_PC_IDLE:
        case TOWN_PC_RUN_LOOP:
        case TOWN_PC_TURN_AROUND:
        case TOWN_PC_DRUNK_IDLE:
            if (flag == 0) {
                return;
            }
        default:
            msg.x = pl->pos[0];
            msg.z = pl->pos[2];
            break;
        case TOWN_PC_SIT_DOWN:
            msg.x = pl->moveTo[0];
            msg.z = pl->moveTo[2];
            msg.spot = gTakeSpotId;
            break;
        case TOWN_PC_SEAT_POSE_RIGHT:
        case TOWN_PC_SEAT_DRINK:
        case TOWN_PC_SEAT_CROSS:
        case TOWN_PC_SEAT_POSE_LEFT:
        case TOWN_PC_SIT_DOWN_SEATED:
        case TOWN_PC_SEAT_POSE_DOWN:
        case TOWN_PC_SEAT_POSE_UP:
        case TOWN_PC_SEAT_POSE_LEFT_END:
        case TOWN_PC_SEAT_POSE_UP_END:
        case TOWN_PC_SEAT_POSE_DOWN_END:
        case TOWN_PC_SEAT_POSE_RIGHT_END:
        case TOWN_PC_MEAL_SERVED:
        case TOWN_PC_KITCHEN_ORDER:
            msg.x = pl->pos[0];
            msg.z = pl->pos[2];
            msg.spot = gTakeSpotId;
            break;
        case TOWN_PC_STAND_UP:
        case TOWN_PC_MEAL_RISE:
        case TOWN_PC_MEAL_COLLAPSE:
        case TOWN_PC_MEAL_RISE_GOOD:
            msg.x = pl->pos[0];
            msg.z = pl->pos[2];
            msg.spot = *(u16 *)pl->spot;
            if (Area_IsTownHall(gScene.area) == 1) {
                ActSpot_Release(msg.spot);
            } else {
                u8 *slots = (u8 *)(pl->index + (s32)gLobby);   /* LobbyState.memberSlot, at the player's index */
                if (slots[0x1538] - '0' == gScene.localPlayer) {
                gLobby->houseSeatBits = gLobby->houseSeatBits & ~(1 << *(u16 *)pl->spot);
                memset(&gLobby->houseSeatIds[*(u16 *)pl->spot * 8], 0, 4);
                }
            }
            break;
        case 0x4A:
        case TOWN_PC_DRUNK_COLLAPSE:
        case TOWN_PC_DRUNK_GET_UP:
            msg.x = gTownSentPos[0];
            msg.z = gTownSentPos[2];
            break;
        }
    } else {
        msg.x = pl->pos[0];
        msg.z = pl->pos[2];
    }
    if ((s8)Town_IsAlone(&msg.x) == 0) {
        gTownMoveTimer = gTownMoveFrames;
        gTownSentPos[0] = msg.x;
        gTownSentPos[2] = msg.z;
        gTownSentState0 = msg.stateGroup;
        gTownSentState1 = msg.state;
        gTownSentRot = msg.rot;
        gTownSentDrunk = msg.drunk;
        gTownSentItem = msg.spot;
        TownMsg_Send(0xA, TOWN_MSG_ACTOR, &msg);
    }
}

void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to);
s32 TownPc_IsSeatedState(u8 arg0, u8 arg1);
void ActSpot_Release(s32 arg0);

/* Player `pl`'s state, position and look to member `member`
 * (TOWN_MSG_ACTOR_TO, the answer to TOWN_MSG_SYNC_ASK), once it has a
 * position; with the seat it holds, or the action spot it gives up as it
 * stands. */
void TownMsg_SendActor(Player *pl, void *member)
{
    TownActMsg msg;

    if (pl->pos[0] != 0.0f || pl->pos[1] != 0.0f || pl->pos[2] != 0.0f) {
        msg.x = pl->pos[0];
        msg.z = pl->pos[2];
        msg.stateGroup = pl->stateGroup;
        msg.state = pl->state;
        msg.rot = (u16)(pl->targetYaw >> 8);
        msg.drunk = pl->drunk;
        if (TownPc_IsSeatedState(pl->stateGroup, pl->state) == 1) {
            msg.spot = gTakeSpotId;
        } else if (pl->stateGroup == 0 && pl->state == TOWN_PC_STAND_UP) {
            msg.spot = *(u16 *)pl->spot;
            ActSpot_Release(msg.spot);
        }
        TownMsg_SendTo(0xA, TOWN_MSG_ACTOR_TO, &msg, member);
    }
}

/* The player's facing to every member (TOWN_MSG_FACING). */
void TownMsg_SendFacing(Player *pl)
{
    TownMsg_Send(0x2, TOWN_MSG_FACING, &pl->rot[1]);
}

void *memcpy(void *dst, const void *src, u32 n);
u8 Area_GetSetIndex(u16 id);
extern u8 gNetUsers[];

/* The area the player moved to, to every member (TOWN_MSG_AREA): its
 * number (Area_GetSetIndex) and the player's user id. */
void TownMsg_SendArea(u16 id)
{
    u8 buf[0x10];
    u8 *lobby = (u8 *)gLobby;
    s8 slot = lobby[gScene.localPlayer + 0x1538] - '0';   /* LobbyState.memberSlot */

    buf[0] = Area_GetSetIndex(id);
    memcpy(&buf[1], &gNetUsers[slot * sizeof(NetUser)], LOBBY_USER_ID_LEN);
    TownMsg_Send(9, TOWN_MSG_AREA, buf);
}

void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to);

/* TownMsg_SendArea to member `member` alone. */
void TownMsg_SendAreaTo(void *member, u16 id)
{
    u8 buf[0x10];
    u8 *lobby = (u8 *)gLobby;
    s8 slot = lobby[gScene.localPlayer + 0x1538] - '0';

    buf[0] = Area_GetSetIndex(id);
    memcpy(&buf[1], &gNetUsers[slot * sizeof(NetUser)], LOBBY_USER_ID_LEN);
    TownMsg_SendTo(9, TOWN_MSG_AREA, buf, member);
}

void *memcpy(void *dst, const void *src, u32 n);
extern u8 gMyName[];

/* The player's name to every member as it arrives (TOWN_MSG_NAME_NOTICE),
 * then its state. */
void TownMsg_SendNameNotice(void)
{
    u8 buf[0x20];

    memcpy(buf, gMyName, 0x10);
    TownMsg_Send(0x10, TOWN_MSG_NAME_NOTICE, buf);
    TownMsg_SendStateNow(&gPlayers[gScene.localPlayer]);
}

void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to);
extern u8 gNetUsers[];

/* Asks for one kind of records (TownMsg_SendRecords' kinds;
 * TOWN_MSG_RECORDS_ASK) from the user in the player's member slot. */
void TownMsg_SendRecordsAsk(u8 kind)
{
    u8 data = kind;
    u8 *lobby = (u8 *)gLobby;

    TownMsg_SendTo(1, TOWN_MSG_RECORDS_ASK, &data, &gNetUsers[(s8)(lobby[gScene.localPlayer + 0x1538] - '0') * sizeof(NetUser)]);
}

extern s8 gTownMoveTimer;
extern u16 gTownMoveFrames;
extern f32 gTownSentPos[3];
extern u8 gTownSentRot;
extern u8 gTownSentItem;
extern u8 gTownSentState0;
extern u8 gTownSentState1;
extern u8 gTownSentDrunk;
void Bits_Pack(u8 *dst, u8 *src, s32 n);
void Records_TallyWeapons(s32 *out);
void Records_TallyArmor(s32 *arg);

/*
 * Sends one kind of the player's records to member `member`: 2 the monsters'
 * largest and smallest sizes and their hunted and captured counts, 3 the
 * items seen, 4 and 5 the weapons' and the armour's tallies, 6 the titles,
 * 10 and 12-17 the equipment made (packed bits) by type; the other kinds
 * send nothing.
 */
void TownMsg_SendRecords(void *member, u8 kind)
{
    u8 big[0x500];
    u8 small[0x30];
    s32 i;
    u8 *out;

    switch (kind) {
    case 4:
        Records_TallyWeapons((s32 *) small);
        TownMsg_SendTo(0x28, TOWN_MSG_RECORDS_WEAPON_TALLY, small, member);
        break;
    case 5:
        Records_TallyArmor((s32 *) small);
        TownMsg_SendTo(0x28, TOWN_MSG_RECORDS_ARMOR_TALLY, small, member);
        break;
    case 3:
        TownMsg_SendTo(0x80, TOWN_MSG_RECORDS_ITEMS, (u8 *)gSave.itemBits, member);
        break;
    case 6:
        TownMsg_SendTo(0x20, TOWN_MSG_RECORDS_TITLES, gSave.titles, member);
        break;
    case 2:
        out = big;
        for (i = 0; i < 0x4A; i++) {
            *out++ = gSave.sizeMax[i];
        }
        TownMsg_SendTo(0x4A, TOWN_MSG_RECORDS_SIZE_MAX, big, member);
        out = big;
        for (i = 0; i < 0x4A; i++) {
            *out++ = gSave.sizeMin[i];
        }
        TownMsg_SendTo(0x4A, TOWN_MSG_RECORDS_SIZE_MIN, big, member);
        memcpy(big, gSave.hunted, 0x100);
        for (i = 0; i < 0x4A; i++) {
            ((u16 *)big)[i] += gSave.captured[i];
        }
        TownMsg_SendTo(0x94, TOWN_MSG_RECORDS_HUNTED, gSave.hunted, member);
        break;
    case 10:
        Bits_Pack(big, gSave.armorFlags[4], 0x280);
        TownMsg_SendTo(0x50, TOWN_MSG_RECORDS_LEGS, big, member);
        break;
    case 12:
        Bits_Pack(big, gSave.armorFlags[0], 0x280);
        TownMsg_SendTo(0x50, TOWN_MSG_RECORDS_HEAD, big, member);
        break;
    case 13:
        Bits_Pack(big, gSave.armorFlags[1], 0x280);
        TownMsg_SendTo(0x50, TOWN_MSG_RECORDS_BODY, big, member);
        break;
    case 14:
        Bits_Pack(big, gSave.armorFlags[2], 0x280);
        TownMsg_SendTo(0x50, TOWN_MSG_RECORDS_ARMS, big, member);
        break;
    case 15:
        Bits_Pack(big, gSave.armorFlags[3], 0x280);
        TownMsg_SendTo(0x50, TOWN_MSG_RECORDS_WAIST, big, member);
        break;
    case 16:
        Bits_Pack(big, gSave.bladeFlags, 0x280);
        TownMsg_SendTo(0x50, TOWN_MSG_RECORDS_BLADES, big, member);
        break;
    case 17:
        Bits_Pack(big, gSave.gunFlags, 0x280);
        TownMsg_SendTo(0x50, TOWN_MSG_RECORDS_GUNS, big, member);
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

/* Tells each player of the room that the arena quest opens
 * (TOWN_MSG_ARENA_OPEN), with the level-2 choice. */
void TownMsg_SendArenaOpen(void)
{
    s8 i;
    s32 off;
    LobbyState *slot;   /* gLobby moved by off: its players[0] is player i */

    for (i = 0, off = 0; i < 4; i++, off += sizeof(NetUser)) {
        slot = (LobbyState *)((u8 *)gLobby + off);
        if (slot->players[0].id[0] != 0) {
            TownMsg_SendTo(1, TOWN_MSG_ARENA_OPEN, gLobbyLevel2Sel, slot->players[0].id);
        }
    }
}

/* Asks for the house seat of the player's action spot (its type)
 * (TOWN_MSG_HOUSE_SEAT_ASK), of the user in the player's member slot. */
void TownMsg_SendHouseSeatAsk(Player *pl)
{
    u8 *lobby = (u8 *)gLobby;
    u8 seat = *(u16 *)pl->spot;

    TownMsg_SendTo(2, TOWN_MSG_HOUSE_SEAT_ASK, &seat, &gNetUsers[(s8)(lobby[gScene.localPlayer + 0x1538] - '0') * sizeof(NetUser)]);
}

s8 MyHouse_LocalSlot(void);
void TownMsg_SendSelf(u8 len, u8 cmd, void *data);
void *memset(void *dst, s32 c, u32 n);
extern u8 gTakeSpotId;

/* The player left house seat gTakeSpotId (TOWN_MSG_HOUSE_SEAT_FREE), to
 * every member and to the player itself, with its user id; only while it
 * is in a house (MyHouse_LocalSlot). */
void TownMsg_SendHouseSeatFree(void *pl)
{
    u8 buf[9];
    s8 i;

    memset(buf, 0, 9);
    buf[0] = gTakeSpotId;
    i = MyHouse_LocalSlot();
    if (i != -1) {
        memcpy(&buf[1], &gNetUsers[i * sizeof(NetUser)], LOBBY_USER_ID_LEN);
        TownMsg_Send(9, TOWN_MSG_HOUSE_SEAT_FREE, buf);
        TownMsg_SendSelf(9, TOWN_MSG_HOUSE_SEAT_FREE, buf);
    }
}

void *memset(void *dst, s32 c, u32 n);
char *strcpy(void *dst, const void *src);
void TownMsg_SendSelf(u8 len, u8 cmd, void *data);

/* The house owner's answer to a seat request (TOWN_MSG_HOUSE_SEAT_ANSWER),
 * to every member and to the player itself: yes or no, the seat and the
 * asker's id. */
void TownMsg_SendHouseSeatAnswer(char *askerId, u8 answer, u8 seat)
{
    u8 buf[0x10];

    memset(buf, 0, 0xB);
    buf[0] = answer;
    buf[1] = seat;
    strcpy(&buf[2], askerId);
    TownMsg_Send(0xB, TOWN_MSG_HOUSE_SEAT_ANSWER, buf);
    TownMsg_SendSelf(0xB, TOWN_MSG_HOUSE_SEAT_ANSWER, buf);
}

/* The player sat in house seat gTakeSpotId (TOWN_MSG_HOUSE_SEAT_TAKEN), to
 * every member and to the player itself, with its user id; only while it
 * is in a house. */
void TownMsg_SendHouseSeatTaken(void)
{
    u8 buf[9];
    s8 i;

    memset(buf, 0, 9);
    buf[0] = gTakeSpotId;
    i = MyHouse_LocalSlot();
    if (i != -1) {
        memcpy(&buf[1], &gNetUsers[i * sizeof(NetUser)], LOBBY_USER_ID_LEN);
        TownMsg_Send(9, TOWN_MSG_HOUSE_SEAT_TAKEN, buf);
        TownMsg_SendSelf(9, TOWN_MSG_HOUSE_SEAT_TAKEN, buf);
    }
}