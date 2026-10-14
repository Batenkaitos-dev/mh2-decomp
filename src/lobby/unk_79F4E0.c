/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "scene.h"
#include "lobby_globals.h"
#include "player.h"

void TownMsg_Send(u8 len, u8 cmd, void *data);
extern u8 gSongRequesterId[];
void *memset(void *, s32, u32);
void TownMsg_SendSelf(u8, u8, void *);

void TownMsg_SendSong(s32 kind) {
    s32 data;

    data = kind;
    TownMsg_Send(4, 0x24, &data);
    TownMsg_SendSelf(4, 0x24, &data);
}

void *memcpy(void *dst, const void *src, u32 n);

void TownMsg_Send25(void *src, s32 x, u8 c)
{
    u8 buf[0x10];

    buf[0] = c;
    memcpy(&buf[1], &x, 4);
    memcpy(&buf[5], src, 8);
    TownMsg_Send(0xD, 0x25, buf);
    TownMsg_SendSelf(0xD, 0x25, (s32 *)buf);
}

void LobbyBgm_ClearState(void)
{
    memset(gSongRequesterId, 0, 0x8);
}

void TownMsg_Send14(void *a0)
{
    TownMsg_Send(0x8, 0x14, a0);
}

void TownMsg_Send02(Player *obj)
{
    s16 v[4];

    v[0] = obj->pos[0];
    v[1] = obj->pos[2];
    TownMsg_Send(4, 2, v);
}

void TownMsg_Send02Zero(void)
{
    s16 v[4];

    v[0] = 0;
    v[1] = 0;
    TownMsg_Send(4, 2, v);
}

struct Player;

void TownMsg_Send06(struct Player *pl)
{
    TownMsg_Send(0, 0x6, 0);
}

s32 Lobby_EnterRollRare(void);
s32 LobbyTime_IsAm(void);
s32 LobbyTime_IsPm(void);
s32 LobbyWorld_GetSeason(void);
void Season_Set(u8); /* unit's own declaration: with the definition's void (s32 season) (main/main.c), TownMsg_SendStatus 1 of 77 words differ */
void Daytime_Set(s32 a);
void NetQuest_SetUnk4C(s8);
void NetQuest_SetInfo(u8 *);
extern s8 gQuestIsOfficial;
typedef struct TownStatMsg {
    u8 data[0x18];
    u8 a;
    s8 b;
} TownStatMsg;

void TownMsg_SendStatus(void)
{
    TownStatMsg m;
    u8 *tbl = (u8 *)&gTownQuestLists;
    s32 k;

    if (LobbyTime_IsAm() == 0) {
        Daytime_Set(4);
    } else {
        Daytime_Set(3);
    }
    k = LobbyWorld_GetSeason();
    Season_Set(k);
    gLobbyLastSeason = k;
    gLobbyLastPm = LobbyTime_IsPm();
    if (gScene.area == 0x86) {
        memcpy(m.data, tbl + k * 0x18 + 4, 0x18);
    } else {
        memcpy(m.data, tbl + k * 0x18 + 0x4C, 0x18);
    }
    if (gQuestIsOfficial != 0) {
        memset(m.data, 0, 0x18);
    }
    m.a = gSeasonFlags;
    m.b = Lobby_EnterRollRare();
    NetQuest_SetInfo(m.data);
    NetQuest_SetUnk4C(m.b);
    TownMsg_Send(0x1A, 0x1A, &m);
}

s32 Town_IsAlone(s16 *unused0) {
    return gScene.playerCount == 1;
}