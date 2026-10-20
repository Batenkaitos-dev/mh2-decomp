/*
 * The town's set-up and its first senders: the town's periods (their
 * defaults and the server's), the town's reset as it is entered, the areas'
 * numbering in the position messages, the chat lines the player sends, and
 * the town messages (TOWN_MSG_*, lobby_net.h) that carry the player's
 * position, the house Poogie's position, the kitchen's messages, the arena
 * quest's teams and the town's tune.
 *
 * A town message goes to every member (TownMsg_Send), to one
 * (TownMsg_SendTo) or to the player itself (TownMsg_SendSelf); the members'
 * TownMsg_Recv reads it. The file's data is gMsgAreaIds, then the equipment
 * shop's lists (gEquipShopLists: by the shop's kind, then by row).
 */
#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "player.h"
#include "em.h"
#include "scene.h"
#include "save.h"
#include "quest.h"
#include "chat_log.h"
#include "lobby_labels.h"
void TownMsg_SendSelf(u8, u8, void *);

void Mem_Fill(void *dst, u8 c, s32 n);
void HunterCard_Build(void *out);
void LobbyMenu_Init(void);
extern s16 gMenuOffsetY;
extern s16 gMenuOffsetX;
extern M2C_UNK gClockWobble;
extern M2C_UNK gTownMode;
extern M2C_UNK *gLobbyMenu;
extern M2C_UNK gLobbyMenuData;
void LobbyNet_SendChatSay(s32, u16, u8, u8);
s32 Lobby_Strlen(char *s);
s32 Game_IsOnline(void);

void *memcpy(void *dst, const void *src, u32 n);
extern u16 gTownMoveFrames;
extern u16 gRecruitFrames;
extern u16 gFriendCheckFrames;
extern u16 gAreaSearchFrames;
extern u8 gEmSizeRanges[];

/*
 * Sets the town's periods to their defaults, in frames: 20 for the
 * position messages (gTownMoveFrames, also the other players' walk time),
 * 5400 (90 s) for the friend check and the recruiting, 1800 (30 s) for the
 * area search. Lobby_ApplyServerTimers replaces them with the server's.
 */
void Lobby_ResetTimers(void)
{
    gTownMoveFrames = 0x14;
    gFriendCheckFrames = 0x1518;
    gAreaSearchFrames = 0x708;
    gRecruitFrames = 0x1518;
}

/*
 * Takes the server's town periods from gLobbyWorkBuf, each one only when
 * the server gave it (nonzero), and the server's monster size ranges
 * (gEmSizeRanges). The last four are read through a pointer to 0x6F14,
 * which the game's code builds in two additions.
 */
void Lobby_ApplyServerTimers(void)
{
    u8 *work = gLobbyWorkBuf;
    u8 *settings = (u8 *)((s32)work + 0x6EF4) + 0x20;

    if (*(u16 *)(work + LOBBY_WORK_TOWN_MOVE_FRAMES) != 0) {
        gTownMoveFrames = *(u16 *)(work + LOBBY_WORK_TOWN_MOVE_FRAMES);
    }
    if (*(u16 *)(settings + (LOBBY_WORK_RECRUIT_FRAMES - 0x6F14)) != 0) {
        gRecruitFrames = *(u16 *)(settings + (LOBBY_WORK_RECRUIT_FRAMES - 0x6F14));
    }
    if (*(u16 *)(settings + (LOBBY_WORK_FRIEND_CHECK_FRAMES - 0x6F14)) != 0) {
        gFriendCheckFrames = *(u16 *)(settings + (LOBBY_WORK_FRIEND_CHECK_FRAMES - 0x6F14));
    }
    if (*(u16 *)(settings + (LOBBY_WORK_AREA_SEARCH_FRAMES - 0x6F14)) != 0) {
        gAreaSearchFrames = *(u16 *)(settings + (LOBBY_WORK_AREA_SEARCH_FRAMES - 0x6F14));
    }
    memcpy(gEmSizeRanges, settings + (LOBBY_WORK_EM_SIZE_RANGES - 0x6F14), 10);
}

/*
 * Resets the town as it is entered: clears the town's state (the 0xF68
 * bytes from gTownMode) and the clock's wobble, points gLobby and
 * gLobbyMenu at their data, sets the menus' offsets to 0, resets the lobby
 * menu, and builds the player's hunter card when the local player is
 * active.
 */
void Town_Reset(void)
{
    u8 localPlayer;

    Mem_Fill(&gTownMode, 0, 0xF68);
    Mem_Fill(&gClockWobble, 0, 6);
    gMenuOffsetY = 0;
    gLobby = &gLobbyState;
    gLobbyMenu = &gLobbyMenuData;
    gMenuOffsetX = 0;
    LobbyMenu_Init();
    localPlayer = gScene.localPlayer;
    if (gPlayers[localPlayer].active != 0) {
        HunterCard_Build(&gPlayerText);
    }
}

extern u16 gMsgAreaIds[];

/* The number the position messages give area `id`: its index in
 * gMsgAreaIds, the town's areas; 0 when it isn't one of them. */
u8 Area_GetSetIndex(u16 id)
{
    u16 i;

    for (i = 0; i < TOWN_MSG_AREAS; i++) {
        if (gMsgAreaIds[i] == id) {
            return i;
        }
    }
    return 0;
}

char *strcpy(void *dst, const void *src);
void Chat_Receive(u8 *arg0);
s32 LobbyReq_ChatTell(u8 arg0, void *arg1, s32 arg2, u16 arg3, void (*onReply)(NetMsg, NetMsg *));
void Chat_OnSendError(NetMsg m, NetMsg *pm);
extern s8 gChatTargets;
extern u8 gChatClassColors[];
extern M2C_UNK gMyName;
extern M2C_UNK gMyUserId;


/*
 * Sends a chat line (online, under 0x40 bytes). It is said to everyone as a
 * balloon (LobbyNet_SendChatSay, CHAT_MSG_BALLOON) unless
 * LobbyState.chatToTargets is set; then it is told to each user of
 * gChatTargets and shown to the player as a ChatMsg of its own, in the
 * colour of class `color`. The message is built as bytes, at a ChatMsg's
 * offsets.
 */
void Chat_SendText(s32 unused, void *text, u8 color)
{
    u8 buf[0x130];
    s32 len;
    s32 i;
    u8 cls;
    s8 *target;

    if (Game_IsOnline() == 0) {
        return;
    }
    len = Lobby_Strlen(text);
    if (len < 0x40) {
        if (gLobby->chatToTargets == 0) {
            LobbyNet_SendChatSay((s32) text, len & 0xFFFF, 0, CHAT_MSG_BALLOON);
            return;
        }
        cls = color;
        for (i = 0, target = &gChatTargets; i < LOBBY_CHAT_TARGETS; i++, target += LOBBY_USER_ID_LEN) {
            if (*target != 0) {
                LobbyReq_ChatTell(cls, target, (s32) text, len, Chat_OnSendError);
            }
        }
        memcpy(&buf[0], &gMyUserId, LOBBY_USER_ID_LEN);    /* id */
        memcpy(&buf[8], &gMyName, 0x10);                    /* name */
        strcpy(&buf[0x1C], text);                           /* text */
        buf[0x11E] = 0;                                     /* color[1] */
        buf[0x11D] = 0;                                     /* color[0] */
        buf[0x11F] = gChatClassColors[color];               /* color[2] */
        buf[0x120] = CHAT_MSG_BALLOON;                      /* kind */
        Chat_Receive(buf);
    }
}

void sprintf(void *dst, void *src);
char *strcpy(void *dst, const void *src);
void Chat_Receive(u8 *arg0);
extern char *gLobbyEventTexts[];

/* A chat tell's reply (LobbyReq_ChatTell): when it failed (kind -1), the
 * player gets a system line that the chat couldn't be sent, under the
 * system's name. */
void Chat_OnSendError(NetMsg reply, NetMsg *replyPtr)
{
    u8 buf[0x130];

    if (reply.kind == -1) {
        sprintf(&buf[0x1C], gLobbyEventTexts[LOBBY_EVENT_CHAT_SEND_ERROR]);
        strcpy(&buf[0], gMemberCountTexts[MEMBER_COUNT_SYSTEM_NAME]);
        strcpy(&buf[8], gMemberCountTexts[MEMBER_COUNT_SYSTEM_NAME]);
        buf[0x11F] = 6;
        buf[0x11E] = 6;
        buf[0x11D] = 6;
        buf[0x120] = CHAT_MSG_BALLOON;
        Chat_Receive(buf);
    }
}

/* Sends a line to the town's chat logs (online, under 0x40 bytes):
 * LobbyNet_SendChatSay with CHAT_MSG_LOG. */
void Chat_SendLogText(s32 unused, s32 text)
{
    s32 len;

    if (Game_IsOnline() != 0) {
        len = Lobby_Strlen((char *) text);
        if (len < 0x40) {
            LobbyNet_SendChatSay(text, len & 0xFFFF, 0, CHAT_MSG_LOG);
        }
    }
}

f32 Vec_Distance(f32 *a, f32 *b);
extern f32 gTownSentPos[3];

/*
 * True when the player's position needn't be sent: it is under 5 units
 * from the last one sent (gTownSentPos), the player is alone, the town mode
 * holds it in place (TOWN_MODE_EXIT, TOWN_MODE_AREA_MENU, mode 0x15), or it
 * is in a group-0 state that does (the drunk's slump and getting up, its
 * idle, state 0x4A).
 */
s32 TownMsg_PositionUnchanged(s16 *pos)
{
    f32 probe[3];
    Player *pl = &gPlayers[gScene.localPlayer];

    probe[0] = pos[0];
    probe[1] = gTownSentPos[1];
    probe[2] = pos[1];
    if (!(5.0f <= Vec_Distance(probe, gTownSentPos))) {
        return 1;
    }
    if (gScene.playerCount == 1) {
        return 1;
    }
    switch (gTownMode) {
    case TOWN_MODE_EXIT:
    case TOWN_MODE_AREA_MENU:
    case 0x15:
        return 1;
    }
    if (pl->stateGroup == 0) {
        switch (pl->state) {
        case TOWN_PC_DRUNK_COLLAPSE:
        case 0x4A:
        case TOWN_PC_DRUNK_GET_UP:
        case TOWN_PC_DRUNK_IDLE:
            return 1;
        }
    }
    return 0;
}

void TownMsg_Send(u8 len, u8 cmd, void *data);
extern s8 gLobbyScreenMode;
extern s8 gTownMoveTimer;
extern s8 D_008BEB59;
extern s16 gTownSentArea;
/* TOWN_MSG_WALK's data (5 bytes sent): the position and the area's number
 * (Area_GetSetIndex). */
typedef struct TownPos {
    s16 x;
    s16 z;
    u8 area;
} TownPos;

/*
 * The player's position message, every gTownMoveFrames frames in the
 * town (LOBBY_SCREEN_TOWN): TOWN_MSG_WALK when the position changed enough
 * (TownMsg_PositionUnchanged) or the player is stopping
 * (TOWN_PC_RUN_STOP); the position and area sent are kept.
 */
void TownMsg_SendPosition(Player *unused0)
{
    TownPos msg;
    Player *pl = &gPlayers[gScene.localPlayer];

    if (--gTownMoveTimer <= 0 && gLobbyScreenMode == LOBBY_SCREEN_TOWN) {
        msg.x = pl->pos[0];
        msg.z = pl->pos[2];
        msg.area = Area_GetSetIndex(pl->area);
        if ((s8)TownMsg_PositionUnchanged(&msg.x) == 0 || pl->state == TOWN_PC_RUN_STOP) {
            D_008BEB59 = 0;
            gTownMoveTimer = gTownMoveFrames;
            gTownSentPos[0] = msg.x;
            gTownSentPos[2] = msg.z;
            gTownSentArea = msg.area;
            TownMsg_Send(5, TOWN_MSG_WALK, &msg);
        }
    }
}

void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to);
u8 Village_FacilityLevel(s32 k);
extern u8 gPoogieLeft;

/*
 * Tells member `member` where the house's Poogie stands (TOWN_MSG_POOGIE_POS):
 * the Poogie `poogie`'s x and z, or with none its spot in the house by the
 * house's level; (0, 0) when the Poogie has no name yet, is out on an
 * errand or has left.
 */
void TownMsg_SendPoogiePos(Em *poogie, void *member)
{
    s16 pos[4];

    if (poogie == NULL) {
        if (*(s8 *)&gSave.poogieName[0] == 0 || gPoogieErrand == 1 || gPoogieLeft == 1) {
            pos[0] = 0;
            pos[1] = 0;
        } else {
            switch (Village_FacilityLevel(VILLAGE_FACILITY_HOUSE) & 0xFF) {
            case 0:
                pos[0] = 0xA5C;
                pos[1] = 0x965;
                break;
            case 1:
                pos[0] = 0xAF3;
                pos[1] = 0x9AB;
                break;
            case 2:
                pos[0] = 0x8CF;
                pos[1] = 0xC04;
                break;
            }
        }
    } else {
        pos[0] = poogie->posX;
        pos[1] = poogie->posZ;
    }
    TownMsg_SendTo(5, TOWN_MSG_POOGIE_POS, pos, member);
}

void TownMsg_Send(u8 len, u8 cmd, void *data);
void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to);

/* Sends a kitchen message (TOWN_MSG_KITCHEN) to member `member`, or to every
 * member with none; its length by its kind (enum KitchenMsg, the first
 * byte). */
void TownMsg_SendKitchen(u8 *msg, void *member)
{
    u8 len;

    switch (msg[0]) {
    case KITCHEN_MSG_OPEN:
    case KITCHEN_MSG_DONE:
        len = 1;
        break;
    case KITCHEN_MSG_ROUTE:
        len = 2;
        break;
    case KITCHEN_MSG_CHOSEN:
        len = 0x11;
        break;
    }
    if (member != NULL) {
        TownMsg_SendTo(len, TOWN_MSG_KITCHEN, msg, member);
    } else {
        TownMsg_Send(len, TOWN_MSG_KITCHEN, msg);
    }
}

extern NetUser gNetUsers[];

/*
 * An arena quest's team (0x1C bytes, a gSave.arenaEms record) and its
 * item: once the quest is paid, the player keeps them as the first side
 * (gNextQuest.teams[0], .teamItems[0]); before, they go to the players
 * whose card's arena role is 1, the quest's poster (TOWN_MSG_ARENA_TEAM).
 */
void TownMsg_SendArenaTeam(void *team, s32 item)
{
    u8 buf[0x20];
    s8 i;
    s32 off;

    memcpy(buf, team, 0x1C);
    memcpy(buf + 0x1C, &item, 4);
    if (gLobby->questPaid != 0) {
        memcpy(&gNextQuest.teams[0], team, 0x1C);
        gNextQuest.teamItems[0] = item;
    } else {
        for (i = 0, off = 0; i < 4; i++, off += sizeof(NetUser)) {
            if (LOBBY_PLAYER_CARD_AT(off)->arenaRole == 1) {
                TownMsg_SendTo(0x20, TOWN_MSG_ARENA_TEAM, buf, LOBBY_PLAYER_AT(off).id);
            }
        }
    }
}

u8 Lobby_FindMember(u8 *id);

/*
 * The poster's last step of an arena quest's set-up: sends both teams and
 * items (TOWN_MSG_ARENA_TEAMS) to each player, noting in gNextQuest.sideIds
 * which side each one is on by its card's arena role, then the sides
 * (TOWN_MSG_ARENA_SIDES) to each player.
 */
void TownMsg_SendArenaTeams(void)
{
    u8 buf[0x40];
    s32 sideOff;
    s8 j;
    s8 i;
    s32 off;
    s8 k;
    LobbyState *slot; /* gLobby moved by off (or sideOff): its players[0] is player i */

    memcpy(buf, gNextQuest.teams, 0x38);
    memcpy(buf + 0x38, gNextQuest.teamItems, 8);
    for (i = 0, off = 0; i < 4; i++, off += sizeof(NetUser)) {
        slot = (LobbyState *)((u8 *)gLobby + off);
        if (slot->players[0].id[0] != 0) {
            TownMsg_SendTo(0x40, TOWN_MSG_ARENA_TEAMS, buf, slot->players[0].id);
            k = Lobby_FindMember((u8 *)LOBBY_PLAYER_AT(off).id);
            memcpy(gNextQuest.sideIds[((HunterCard *)gNetUsers[k].text)->arenaRole - 1], LOBBY_PLAYER_AT(off).id, 6);
        }
    }
    for (j = 0, sideOff = 0; j < 4; j++, sideOff += sizeof(NetUser)) {
        slot = (LobbyState *)((u8 *)gLobby + sideOff);
        if (slot->players[0].id[0] != 0) {
            TownMsg_SendTo(0x18, TOWN_MSG_ARENA_SIDES, gNextQuest.sideIds, slot->players[0].id);
        }
    }
}

s32 LobbyBgm_GetTrack(void);
u32 LobbyBgm_GetPos(void);

/* Tells member `member` the town's tune and its position (TOWN_MSG_BGM). */
void TownMsg_SendBgmTo(s32 unused, void *member)
{
    s32 tune[2];

    tune[0] = LobbyBgm_GetTrack();
    tune[1] = LobbyBgm_GetPos();
    TownMsg_SendTo(8, TOWN_MSG_BGM, tune, member);
}

/* Starts tune `track` from its start in the town: sends it to every member
 * and to the player itself (TOWN_MSG_BGM). */
void TownMsg_SendBgm(s32 track)
{
    s32 tune[2];

    tune[0] = track;
    tune[1] = 0;
    TownMsg_Send(8, TOWN_MSG_BGM, tune);
    TownMsg_SendSelf(8, TOWN_MSG_BGM, tune);
}

/* The file's .data: gMsgAreaIds, then the equipment shop's lists. */
u16 gMsgAreaIds[16] = {
#include "lobby/0080ACD0.inc"
};
u32 gEquipShopLists_0_0[4] = {
#include "lobby/0080ACF0.inc"
};
u32 gEquipShopLists_1_0[4] = {
#include "lobby/0080AD00.inc"
};
u32 gEquipShopLists_0_1[4] = {
#include "lobby/0080AD10.inc"
};
u32 gEquipShopLists_1_1[4] = {
#include "lobby/0080AD20.inc"
};
u32 gEquipShopLists_0_2[4] = {
#include "lobby/0080AD30.inc"
};
u32 gEquipShopLists_1_2[4] = {
#include "lobby/0080AD40.inc"
};
u32 gEquipShopLists_0_3[4] = {
#include "lobby/0080AD50.inc"
};
u32 gEquipShopLists_1_3[4] = {
#include "lobby/0080AD60.inc"
};
u32 gEquipShopLists_0_4[4] = {
#include "lobby/0080AD70.inc"
};
u32 gEquipShopLists_1_4[8] = {
#include "lobby/0080AD80.inc"
};
u32 gEquipShopLists_0_5[4] = {
#include "lobby/0080ADA0.inc"
};
u32 gEquipShopLists_1_5[8] = {
#include "lobby/0080ADB0.inc"
};
u32 gEquipShopLists_2_5[12] = {
#include "lobby/0080ADD0.inc"
};
u32 gEquipShopLists_3_5[10] = {
#include "lobby/0080AE00.inc"
};
u32 gEquipShopLists_0_6[3] = {
#include "lobby/0080AE28.inc"
};
u32 gEquipShopLists_1_6[8] = {
#include "lobby/0080AE40.inc"
};
u32 gEquipShopLists_2_6[12] = {
#include "lobby/0080AE60.inc"
};
u32 gEquipShopLists_3_6[12] = {
#include "lobby/0080AE90.inc"
};
u32 gEquipShopLists_0_7[4] = {
#include "lobby/0080AEC0.inc"
};
u32 gEquipShopLists_1_7[8] = {
#include "lobby/0080AED0.inc"
};
u32 gEquipShopLists_2_7[12] = {
#include "lobby/0080AEF0.inc"
};
u32 gEquipShopLists_3_7[12] = {
#include "lobby/0080AF20.inc"
};
u32 gEquipShopLists_0_8[4] = {
#include "lobby/0080AF50.inc"
};
u32 gEquipShopLists_1_8[12] = {
#include "lobby/0080AF60.inc"
};
u32 gEquipShopLists_2_8[16] = {
#include "lobby/0080AF90.inc"
};
u32 gEquipShopLists_3_8[14] = {
#include "lobby/0080AFD0.inc"
};
u32 gEquipShopLists_0_9[3] = {
#include "lobby/0080B008.inc"
};
u32 gEquipShopLists_1_9[12] = {
#include "lobby/0080B020.inc"
};
u32 gEquipShopLists_2_9[16] = {
#include "lobby/0080B050.inc"
};
u32 gEquipShopLists_3_9[16] = {
#include "lobby/0080B090.inc"
};
u32 gEquipShopLists_0[12] = {
    (u32)gEquipShopLists_0_0, (u32)gEquipShopLists_0_1, (u32)gEquipShopLists_0_2, (u32)gEquipShopLists_0_3,
    (u32)gEquipShopLists_0_4, (u32)gEquipShopLists_0_5, (u32)gEquipShopLists_0_6, (u32)gEquipShopLists_0_7,
    (u32)gEquipShopLists_0_8, (u32)gEquipShopLists_0_9, 0, 0,
};
u32 gEquipShopLists_1[12] = {
    (u32)gEquipShopLists_1_0, (u32)gEquipShopLists_1_1, (u32)gEquipShopLists_1_2, (u32)gEquipShopLists_1_3,
    (u32)gEquipShopLists_1_4, (u32)gEquipShopLists_1_5, (u32)gEquipShopLists_1_6, (u32)gEquipShopLists_1_7,
    (u32)gEquipShopLists_1_8, (u32)gEquipShopLists_1_9, 0, 0,
};
u32 gEquipShopLists_2[12] = {
    (u32)gEquipShopLists_1_0, (u32)gEquipShopLists_1_1, (u32)gEquipShopLists_1_2, (u32)gEquipShopLists_1_3,
    (u32)gEquipShopLists_1_4, (u32)gEquipShopLists_2_5, (u32)gEquipShopLists_2_6, (u32)gEquipShopLists_2_7,
    (u32)gEquipShopLists_2_8, (u32)gEquipShopLists_2_9, 0, 0,
};
u32 gEquipShopLists_3[12] = {
    (u32)gEquipShopLists_1_0, (u32)gEquipShopLists_1_1, (u32)gEquipShopLists_1_2, (u32)gEquipShopLists_1_3,
    (u32)gEquipShopLists_1_4, (u32)gEquipShopLists_3_5, (u32)gEquipShopLists_3_6, (u32)gEquipShopLists_3_7,
    (u32)gEquipShopLists_3_8, (u32)gEquipShopLists_3_9, 0, 0,
};
u32 gEquipShopLists[4] = {
    (u32)gEquipShopLists_0, (u32)gEquipShopLists_1, (u32)gEquipShopLists_2, (u32)gEquipShopLists_3,
};
