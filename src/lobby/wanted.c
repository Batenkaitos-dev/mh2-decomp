/*
 * The lobby overlay's first functions: the update of the town mode that
 * only waits for cancel, and the town's wanted lists.
 *
 * The wanted lists (gTownQuestLists, TownQuestLists) hold, per season,
 * three entries for each of the two hubs: AREA_TAVERN reads the first
 * set (a), AREA_ELDER_HALL the second (b), and the second only online. An
 * entry is posted when its three values are all nonzero (Wanted_IsSet).
 * The quest desk pays a bonus for a quest whose monster is wanted
 * (Wanted_MatchesQuest, QuestDesk_IsWanted).
 */
#include "common.h"
#include "scene.h"
#include "lobby_vobj.h"
#include "lobby_menu.h"
#include "lobby_globals.h"
#include "pad.h"
#include "area.h"
#include "quest.h"
#include "camera.h"
#include "save.h"
#include "wanted.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_world.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;

/*
 * The update of TOWN_MODE_WAIT_CANCEL: on PAD_CROSS, closes the town's
 * menu (no draw callbacks) and ends the camera's focus.
 */
void TownMode_WaitCancel(void)
{
    if (LobbyPad_Get(0) & PAD_CROSS) {
        gTownMode = TOWN_MODE_FREE;
        gTownMenuOpen = 0;
        LobbyMenu_SetDraw(0, 0, 0, 0);
        CamFocus_End();
    }
}

/*
 * Counts the wanted entries posted for this season: the first hub's three
 * (not in AREA_ELDER_HALL), then online the second hub's three (not in
 * AREA_TAVERN).
 */
s8 Wanted_Count(void)
{
    TownQuestLists *lists = &gTownQuestLists;
    s32 season = LobbyWorld_GetSeason();
    s8 j;
    s8 count = 0;

    if (gScene.area != AREA_ELDER_HALL) {
        for (j = 0; j < WANTED_ENTRIES; j++) {
            if (Wanted_IsSet(lists->a[season][j].v) == 1) {
                count++;
            }
        }
    }
    if (gScene.area != AREA_TAVERN && Game_IsOnline() != 0) {
        for (j = 0; j < WANTED_ENTRIES; j++) {
            if (Wanted_IsSet(lists->b[season][j].v) == 1) {
                count++;
            }
        }
    }
    return count;
}

/* 1 if a wanted entry is posted: its three values (TownEntry.v) are all
 * nonzero; else 0. */
s32 Wanted_IsSet(s16 *values)
{
    if (values[0] == 0 || values[1] == 0 || values[2] == 0) {
        return 0;
    }
    return 1;
}

/*
 * 1 if the quest's goal hunts a monster of the same group as the wanted
 * entry's monster (the low byte of v[1]): the entry must be posted, its
 * v[0] not 3, and the goal a QUEST_GOAL_HUNT one; else 0. Only the low
 * byte of v[1] is read, as a byte, as the game's code does.
 */
s32 Wanted_MatchesQuest(QuestGoal *goal, TownEntry *entry)
{
    s32 group;

    if (entry->v[0] == 3) {
        return 0;
    }
    if (Wanted_IsSet(entry->v) == 0) {
        return 0;
    }
    if (!(goal->flags & QUEST_GOAL_HUNT)) {
        return 0;
    }
    group = MonsterGroup_First(goal->target) & 0xFF;
    return group == (MonsterGroup_First(*(u8 *)&entry->v[1]) & 0xFF);
}

/*
 * 1 if the quest (a QuestInfo, which the quest desk passes as bytes) hunts
 * a monster wanted this season: in AREA_ELDER_HALL by the second hub's list,
 * in AREA_TAVERN or offline by the first's; else 0.
 */
s32 QuestDesk_IsWanted(u8 *questBytes)
{
    QuestInfo *quest = (QuestInfo *)questBytes;
    TownQuestLists *lists = &gTownQuestLists;
    s32 season = LobbyWorld_GetSeason();

    if (gScene.area == AREA_ELDER_HALL) {
        if (Wanted_MatchesQuest(&quest->goals[0], lists->b[season]) == 1) {
            return 1;
        }
    } else if (gScene.area == AREA_TAVERN || Game_IsOnline() == 0) {
        if (Wanted_MatchesQuest(&quest->goals[0], lists->a[season]) == 1) {
            return 1;
        }
    }
    return 0;
}

/* The words of the lobby's work buffer (gLobbyWorkBuf) that hold the wanted
 * fee each hub's server sent. */
#define LOBBY_WORK_WANTED_FEE_A 0x7AAA  /* s16: AREA_TAVERN's */
#define LOBBY_WORK_WANTED_FEE_B 0x7AAC  /* s16: AREA_ELDER_HALL's */
#define WANTED_FEE_OFFLINE      1000    /* money */

/*
 * The extra fee of a wanted quest: WANTED_FEE_OFFLINE offline; online the
 * fee the hub's server sent (0 outside the two hubs). The quest desk adds
 * it to the quest's fee and shows it.
 */
s16 QuestDesk_GetWantedFee(void)
{
    u16 area;

    if (Game_IsOnline() == 0) {
        return WANTED_FEE_OFFLINE;
    }
    area = gScene.area;
    switch (area) {
    case AREA_TAVERN:
        return *(s16 *)(gLobbyWorkBuf + LOBBY_WORK_WANTED_FEE_A);
    case AREA_ELDER_HALL:
        return *(s16 *)(gLobbyWorkBuf + LOBBY_WORK_WANTED_FEE_B);
    default:
        return 0;
    }
}
