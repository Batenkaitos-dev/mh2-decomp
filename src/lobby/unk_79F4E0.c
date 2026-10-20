/*
 * More of the town's senders (TOWN_MSG_*, lobby_net.h): the song request
 * and the host's answer, a house visit, the player's place, the end of
 * being drunk, and the quest information the town's host sends as members
 * set out (the season, the time of day and the wanted lists); with them
 * LobbyBgm_ClearState and Town_IsAlone. TownMsg_Send sends to every member
 * and TownMsg_SendSelf to the player itself.
 */
#include "common.h"
#include "m2c_macros.h"
#include "scene.h"
#include "lobby_globals.h"
#include "player.h"
#include "area.h"
#include "lobby_net.h"
#include "quest.h"

void TownMsg_Send(u8 len, u8 cmd, void *data);
extern u8 gSongRequesterId[];
void *memset(void *, s32, u32);
void TownMsg_SendSelf(u8, u8, void *);

/* Asks the host to play song `song` (TOWN_MSG_SONG_ASK), to every member
 * and to the player itself. */
void TownMsg_SendSongAsk(s32 song)
{
    s32 data;

    data = song;
    TownMsg_Send(4, TOWN_MSG_SONG_ASK, &data);
    TownMsg_SendSelf(4, TOWN_MSG_SONG_ASK, &data);
}

void *memcpy(void *dst, const void *src, u32 n);

/* The host's answer to a song request (TOWN_MSG_SONG_ANSWER), to every
 * member and to the player itself: `busy` (the first byte), the song and
 * the asker's id. */
void TownMsg_SendSongAnswer(void *askerId, s32 song, u8 busy)
{
    u8 buf[0x10];

    buf[0] = busy;
    memcpy(&buf[1], &song, 4);
    memcpy(&buf[5], askerId, LOBBY_USER_ID_LEN);
    TownMsg_Send(0xD, TOWN_MSG_SONG_ANSWER, buf);
    TownMsg_SendSelf(0xD, TOWN_MSG_SONG_ANSWER, (s32 *)buf);
}

/* Forgets who asked for the song being played (gSongRequesterId). */
void LobbyBgm_ClearState(void)
{
    memset(gSongRequesterId, 0, LOBBY_USER_ID_LEN);
}

/* Tells every member that the player went into the house of the member
 * whose user id `ownerId` is (TOWN_MSG_HOUSE_VISIT). */
void TownMsg_SendHouseVisit(void *ownerId)
{
    TownMsg_Send(LOBBY_USER_ID_LEN, TOWN_MSG_HOUSE_VISIT, ownerId);
}

/* Tells every member to put the player at its x and z at once
 * (TOWN_MSG_PLACE). */
void TownMsg_SendPlace(Player *pl)
{
    s16 pos[4];

    pos[0] = pl->pos[0];
    pos[1] = pl->pos[2];
    TownMsg_Send(4, TOWN_MSG_PLACE, pos);
}

/* TOWN_MSG_PLACE with the origin (0, 0): sent as the player enters an
 * area, before the area and the real place. */
void TownMsg_SendPlaceOrigin(void)
{
    s16 pos[4];

    pos[0] = 0;
    pos[1] = 0;
    TownMsg_Send(4, TOWN_MSG_PLACE, pos);
}

struct Player;

/* Tells every member that the player is no longer drunk (TOWN_MSG_SOBER). */
void TownMsg_SendSober(struct Player *pl)
{
    TownMsg_Send(0, TOWN_MSG_SOBER, 0);
}

s32 Lobby_EnterRollRare(void);
s32 LobbyTime_IsAm(void);
s32 LobbyTime_IsPm(void);
s32 LobbyWorld_GetSeason(void);
void Season_Set(u8); /* unit's own declaration: the definition takes an s32; declared with a u8, as TownMsg_SendStatus passes the season, since with the definition's type the call compiles differently */
void Daytime_Set(s32 a);
void NetQuest_SetRareRoll(s8);
void NetQuest_SetInfo(u8 *);
extern s8 gQuestIsOfficial;
/* TOWN_MSG_QUEST_INFO's data (0x1A bytes sent). */
typedef struct TownQuestInfoMsg {
    TownEntry wanted[WANTED_ENTRIES];   /* the season's wanted list of the hall (none for an official quest) */
    u8 seasonFlags;                     /* gSeasonFlags */
    s8 rareRoll;                        /* Lobby_EnterRollRare */
} TownQuestInfoMsg;

/*
 * The quest information as the party sets out: sets the time of day from
 * the town's clock and the season from the town's, keeps them as the last
 * seen, takes the season's wanted list of the hall (AREA_TAVERN's, else
 * the elder hall's; none for an official quest) and rolls the rare chance;
 * keeps them for the quest (NetQuest_SetInfo, NetQuest_SetRareRoll) and
 * sends them to every member (TOWN_MSG_QUEST_INFO). The lists are read
 * through a byte pointer, at the season's offset in each.
 */
void TownMsg_SendStatus(void)
{
    TownQuestInfoMsg msg;
    u8 *lists = (u8 *)&gTownQuestLists;
    s32 season;

    if (LobbyTime_IsAm() == 0) {
        Daytime_Set(DAYTIME_SET_NIGHT);
    } else {
        Daytime_Set(DAYTIME_SET_DAY);
    }
    season = LobbyWorld_GetSeason();
    Season_Set(season);
    gLobbyLastSeason = season;
    gLobbyLastPm = LobbyTime_IsPm();
    if (gScene.area == AREA_TAVERN) {
        memcpy(msg.wanted, lists + season * sizeof(msg.wanted) + 4, sizeof(msg.wanted));     /* .a[season] */
    } else {
        memcpy(msg.wanted, lists + season * sizeof(msg.wanted) + 0x4C, sizeof(msg.wanted));  /* .b[season] */
    }
    if (gQuestIsOfficial != 0) {
        memset(msg.wanted, 0, sizeof(msg.wanted));
    }
    msg.seasonFlags = gSeasonFlags;
    msg.rareRoll = Lobby_EnterRollRare();
    NetQuest_SetInfo((u8 *)msg.wanted);
    NetQuest_SetRareRoll(msg.rareRoll);
    TownMsg_Send(0x1A, TOWN_MSG_QUEST_INFO, &msg);
}

/* True when the player is alone in the town. */
s32 Town_IsAlone(s16 *unused0)
{
    return gScene.playerCount == 1;
}
