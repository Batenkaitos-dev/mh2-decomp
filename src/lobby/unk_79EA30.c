#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "player.h"
#include "em.h"
#include "scene.h"
#include "save.h"
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
void LobbyNet_Send6714(s32, u16, u8, u8);
s32 Lobby_Strlen(char *s);
s32 Game_IsOnline(void);

void *memcpy(void *dst, const void *src, u32 n);
extern u16 gTownMoveFrames;
extern u16 gRecruitFrames;
extern u16 gFriendCheckFrames;
extern u16 gAreaSearchFrames;
extern u8 gEmSizeRanges[];

void Lobby_ApplyServerTimers(void)
{
    u8 *p = gLobbyWorkBuf;
    u8 *q = (u8 *)((s32)p + 0x6EF4) + 0x20;

    if (*(u16 *)(p + 0x7A90) != 0) {
        gTownMoveFrames = *(u16 *)(p + 0x7A90);
    }
    if (*(u16 *)(q + 0xB7E) != 0) {
        gRecruitFrames = *(u16 *)(q + 0xB7E);
    }
    if (*(u16 *)(q + 0xB82) != 0) {
        gFriendCheckFrames = *(u16 *)(q + 0xB82);
    }
    if (*(u16 *)(q + 0xB86) != 0) {
        gAreaSearchFrames = *(u16 *)(q + 0xB86);
    }
    memcpy(gEmSizeRanges, q + 0xB8C, 10);
}

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

u8 Area_GetSetIndex(u16 id)
{
    u16 i;

    for (i = 0; i < 16; i++) {
        if (gMsgAreaIds[i] == id) {
            return i;
        }
    }
    return 0;
}

char *strcpy(void *dst, const void *src);
void Chat_Receive(u8 *arg0);
s32 LobbyReq_670B(u8 arg0, void *arg1, s32 arg2, u16 arg3, void (*onReply)(NetMsg, NetMsg *));
void Chat_OnSendError(NetMsg m, NetMsg *pm);
extern s8 gChatTargets;
extern u8 gChatClassColors[];
extern M2C_UNK gMyName;
extern M2C_UNK gMyUserId;


/* Sends a chat text (NETWORK): with +0x3967 of the lobby state 0 as the
 * notification 2 of LobbyNet_Send6714, else to each of the 7 addresses at
 * gChatTargets, then shows it as the player's own message (Chat_Receive). */
void Chat_SendText(s32 unused, void *text, u8 color)
{
    u8 buf[0x130];
    s32 len;
    s32 i;
    u8 c;
    s8 *to;

    if (Game_IsOnline() == 0) {
        return;
    }
    len = Lobby_Strlen(text);
    if (len < 0x40) {
        if (gLobby->unk3967 == 0) {
            LobbyNet_Send6714((s32) text, len & 0xFFFF, 0, 2);
            return;
        }
        c = color;
        for (i = 0, to = &gChatTargets; i < 7; i++, to += 8) {
            if (*to != 0) {
                LobbyReq_670B(c, to, (s32) text, len, Chat_OnSendError);
            }
        }
        memcpy(&buf[0], &gMyUserId, 8);
        memcpy(&buf[8], &gMyName, 0x10);
        strcpy(&buf[0x1C], text);
        buf[0x11E] = 0;
        buf[0x11D] = 0;
        buf[0x11F] = gChatClassColors[color];
        buf[0x120] = 2;
        Chat_Receive(buf);
    }
}

void sprintf(void *dst, void *src);
char *strcpy(void *dst, const void *src);
void Chat_Receive(u8 *arg0);
extern void *D_0080D458;
extern void *D_0080C6C4;

void Chat_OnSendError(NetMsg m, NetMsg *pm)
{
    u8 buf[0x130];

    if (m.kind == -1) {
        sprintf(&buf[0x1C], D_0080D458);
        strcpy(&buf[0], D_0080C6C4);
        strcpy(&buf[8], D_0080C6C4);
        buf[0x11F] = 6;
        buf[0x11E] = 6;
        buf[0x11D] = 6;
        buf[0x120] = 2;
        Chat_Receive(buf);
    }
}

void Chat_Send6714(s32 unused, M2C_UNK s)
{
    s32 len;

    if (Game_IsOnline() != 0) {
        len = Lobby_Strlen((char *) s);
        if (len < 0x40) {
            LobbyNet_Send6714(s, len & 0xFFFF, 0, 1);
        }
    }
}

f32 Vec_Distance(f32 *a, f32 *b);
extern f32 gTownSentPos;
extern f32 D_008BF124;

s32 TownMsg_PositionUnchanged(s16 *pos)
{
    f32 v[3];
    Player *pl = &gPlayers[gScene.localPlayer];

    v[0] = pos[0];
    v[1] = D_008BF124;
    v[2] = pos[1];
    if (!(5.0f <= Vec_Distance(v, &gTownSentPos))) {
        return 1;
    }
    if (gScene.playerCount == 1) {
        return 1;
    }
    switch (gTownMode) {
    case 0xE:
    case 0x3A:
    case 0x15:
        return 1;
    }
    if (pl->stateGroup == 0) {
        switch (pl->state) {
        case 0x12:
        case 0x4A:
        case 0x13:
        case 0x1D:
            return 1;
        }
    }
    return 0;
}

void TownMsg_Send(u8 len, u8 cmd, void *data);
extern s8 gLobbyScreenMode;
extern s8 gTownMoveTimer;
extern s8 D_008BEB59;
extern f32 D_008BF128;
extern s16 gTownSentArea;
typedef struct TownPos {
    s16 x;
    s16 z;
    u8 area;
} TownPos;

void TownMsg_SendPosition(Player *unused0)
{
    TownPos m;
    Player *pl = &gPlayers[gScene.localPlayer];

    if (--gTownMoveTimer <= 0 && gLobbyScreenMode == 2) {
        m.x = pl->pos[0];
        m.z = pl->pos[2];
        m.area = Area_GetSetIndex(pl->area);
        if ((s8)TownMsg_PositionUnchanged(&m.x) == 0 || pl->state == 6) {
            D_008BEB59 = 0;
            gTownMoveTimer = gTownMoveFrames;
            gTownSentPos = m.x;
            D_008BF128 = m.z;
            gTownSentArea = m.area;
            TownMsg_Send(5, 0, &m);
        }
    }
}

void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to);
u8 Village_FacilityLevel(s32 k);
extern u8 gNpc14Left;

void TownMsg_Send39(Em *pl, void *to)
{
    s16 p[4];

    if (pl == NULL) {
        if (*(s8 *)&gSave.poogieName[0] == 0 || gOnlineBonusState == 1 || gNpc14Left == 1) {
            p[0] = 0;
            p[1] = 0;
        } else {
            switch (Village_FacilityLevel(4) & 0xFF) {
            case 0:
                p[0] = 0xA5C;
                p[1] = 0x965;
                break;
            case 1:
                p[0] = 0xAF3;
                p[1] = 0x9AB;
                break;
            case 2:
                p[0] = 0x8CF;
                p[1] = 0xC04;
                break;
            }
        }
    } else {
        p[0] = pl->posX;
        p[1] = pl->posZ;
    }
    TownMsg_SendTo(5, 0x39, p, to);
}

void TownMsg_Send(u8 len, u8 cmd, void *data);
void TownMsg_SendTo(u8 len, u8 cmd, void *data, void *to);

void TownMsg_Send31(u8 *p, void *to)
{
    u8 len;

    switch (p[0]) {
    case 0:
    case 3:
        len = 1;
        break;
    case 1:
        len = 2;
        break;
    case 2:
        len = 0x11;
        break;
    }
    if (to != NULL) {
        TownMsg_SendTo(len, 0x31, p, to);
    } else {
        TownMsg_Send(len, 0x31, p);
    }
}

extern NetUser gNetUsers[];
extern u8 D_6ADAD4[];

void TownMsg_Send28(void *data, s32 x)
{
    u8 buf[0x20];
    s8 i;
    s32 off;
    u8 *p;

    memcpy(buf, data, 0x1C);
    memcpy(buf + 0x1C, &x, 4);
    if (gLobby->questPaid != 0) {
        memcpy(D_6ADAD4, data, 0x1C);
        *(s32 *)&D_6ADB0C = x;
    } else {
        for (i = 0, off = 0; i < 4; i++, off += 0x11C) {
            p = (u8 *)gLobby + off;
            if (p[0x698] == 1) {
                TownMsg_SendTo(0x20, 0x28, buf, p + 0x5A6);
            }
        }
    }
}

extern u8 D_6ADB0C[];
extern u8 D_6ADB16[];
u8 Lobby_FindMember(u8 *id);

void TownMsg_Send29(void)
{
    u8 buf[0x40];
    s32 off2;
    s8 j;
    s8 i;
    s32 off;
    s8 k;
    u8 *p;

    memcpy(buf, D_6ADAD4, 0x38);
    memcpy(buf + 0x38, D_6ADB0C, 8);
    for (i = 0, off = 0; i < 4; i++, off += 0x11C) {
        p = (u8 *)gLobby + off;
        if ((s8)p[0x5A6] != 0) {
            TownMsg_SendTo(0x40, 0x29, buf, p + 0x5A6);
            k = Lobby_FindMember((u8 *)gLobby + off + 0x5A6);
            memcpy((u8 *)&gNextQuest + (((HunterCard *)gNetUsers[k].text)->arenaRole - 1) * 6 + 0x46, (u8 *)gLobby + off + 0x5A6, 6);
        }
    }
    for (j = 0, off2 = 0; j < 4; j++, off2 += 0x11C) {
        p = (u8 *)gLobby + off2;
        if ((s8)p[0x5A6] != 0) {
            TownMsg_SendTo(0x18, 0x2A, D_6ADB16, p + 0x5A6);
        }
    }
}

s32 LobbyBgm_GetTrack(void);
u32 LobbyBgm_GetPos(void);

void TownMsg_SendBgmTo(s32 unused, void *to)
{
    s32 v[2];

    v[0] = LobbyBgm_GetTrack();
    v[1] = LobbyBgm_GetPos();
    TownMsg_SendTo(8, 0x23, v, to);
}

void TownMsg_SendBgm(s32 x)
{
    s32 v[2];

    v[0] = x;
    v[1] = 0;
    TownMsg_Send(8, 0x23, v);
    TownMsg_SendSelf(8, 0x23, v);
}

/* unk_79EA30's .data (0x80ACD0-0x80B1A0) */
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
