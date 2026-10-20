/*
 * Which quests the village and the lobby offer. MapGroup_IsOpen says
 * whether a map's quests are open (offline by the story's progress, the
 * village's conditions; online by the hunter rank), Quest_MatchesSeasonTime
 * whether a quest runs in a season and at a time of day, Quest_CheckOrder
 * whether the taken quest can set out now, and Quest_IsOffered whether a
 * quest goes on the quest lists (QuestList_Build). The elder's forecast
 * (ElderForecast_IsUnlocked, ElderForecast_QuestAppears) adds the forecast
 * quests at random. QuestList_Rebuild rebuilds the lists (put off while the
 * quest desk is open) and Npc_RefreshOffers the townspeople's offer marks.
 * QuestList_Build fills a quest list set (gQuestLists) by its list kind and
 * thins it out at random by map group (QuestList_DropRandom), and
 * QuestList_PickShown picks the quest board's quests from it.
 */
#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "save.h"
#include "lobby_globals.h"
#include "quest.h"
#include "area.h"
#include "em.h"
#include "lobby_menu.h"
#include "lobby_npc.h"


QuestInfo *Quest_GetDiscInfo(u16 id);
s32 Quest_CheckOrderTerms(u8, u8);
s32 LobbyTime_IsPm(void);
s32 LobbyWorld_GetSeason(void);
s32 Quest_MatchesSeasonTime(u16 id, s32 season, s32 isPm);
s32 Game_IsOnline(void);
s32 Cond_Check(u16 id);
s32 Save_TestEventFlag(s32 flag);
/* unit's own declaration: returning s32, as Quest_IsOffered passes it on; with the definition's u16 the call compiles differently */
s32 Map_Group(u16 id);
s32 Monster_IsAvailable(u16 id);
s32 ElderForecast_QuestAppears(u16 id, s32 listKind);
extern u16 gQuestLists[QUEST_SETS][QUEST_SET_LEN];
void *memset(void *dst, s32 c, u32 n);
u16 Rand_Next(s32);
s32 QuestList_CountGroup(s32 group, s8 set);
void QuestList_DropGroup(s32 group, s32 keep, s8 set);

/*
 * 1 when the quests of map group `map` (Map_Group of a quest's map) are open,
 * else 0. Offline the jungle always, the others by the village's conditions
 * (Cond_Check): the desert 7, the swamp 0xA, the snowy mountains 0x10, the
 * volcano and the towers 0x15, the forest and hills 0x4F while away from the
 * village (gSave.awayFromVillage). Online the jungle, the desert and the
 * swamp always, the snowy mountains from hunter rank 5, the volcano from 11,
 * the towers from 22, the forest and hills with event flag 0x122.
 */
s32 MapGroup_IsOpen(u16 map)
{
    if (Game_IsOnline() == 0) {
        switch (map) {
        case MAP_JUNGLE:
            return 1;
        case MAP_DESERT:
            if (Cond_Check(7) == 1) {
                return 1;
            }
            break;
        case MAP_SWAMP:
            if (Cond_Check(0xA) == 1) {
                return 1;
            }
            break;
        case MAP_SNOWY_MOUNTAINS:
            if (Cond_Check(0x10) == 1) {
                return 1;
            }
            break;
        case MAP_VOLCANO:
            if (Cond_Check(0x15) == 1) {
                return 1;
            }
            break;
        case MAP_TOWER:
        case MAP_TOWER_B:
        case MAP_TOWER_C:
            if (Cond_Check(0x15) == 1) {
                return 1;
            }
            break;
        case MAP_FOREST_HILLS:
            if (Cond_Check(0x4F) == 1 && gSave.awayFromVillage != 0) {
                return 1;
            }
            break;
        }
    } else {
        switch (map) {
        case MAP_JUNGLE:
        case MAP_DESERT:
        case MAP_SWAMP:
            return 1;
        case MAP_SNOWY_MOUNTAINS:
            if (gSave.hr >= 5) {
                return 1;
            }
            break;
        case MAP_VOLCANO:
            if (gSave.hr >= 0xB) {
                return 1;
            }
            break;
        case MAP_TOWER:
        case MAP_TOWER_B:
        case MAP_TOWER_C:
            if (gSave.hr >= 0x16) {
                return 1;
            }
            break;
        case MAP_FOREST_HILLS:
            if (Save_TestEventFlag(0x122) == 1) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/*
 * 1 when quest id runs in `season` (SEASON_*) and at the time of day `isPm`
 * (0 day, 1 night) by its QuestInfo.flags, else 0 (0 also for no quest).
 */
s32 Quest_MatchesSeasonTime(u16 id, s32 season, s32 isPm)
{
    QuestInfo *quest = Quest_GetDiscInfo(id);
    u32 mask;

    if (quest == NULL) {
        return 0;
    }
    switch (season) {
    case SEASON_WARM:
        mask = 0x20000;
        break;
    case SEASON_COLD:
        mask = QUEST_FLAG_COLD;
        break;
    case SEASON_BREEDING:
        mask = QUEST_FLAG_BREEDING;
        break;
    }
    if (!(quest->flags & mask)) {
        return 0;
    }
    switch (isPm) {
    case 0:
        mask = QUEST_FLAG_DAY;
        break;
    case 1:
        mask = 0x100000;
        break;
    }
    return (quest->flags & mask) != 0;
}

/*
 * Whether quest id can set out now: QUEST_ORDER_TERMS when its order terms
 * aren't met (Quest_CheckOrderTerms; a paid quest is checked with flag 0,
 * else 1), QUEST_ORDER_WRONG_TIME when a built-in quest doesn't run in the
 * town's season and time of day, else QUEST_ORDER_OK.
 */
s32 Quest_CheckOrder(u16 id)
{
    u8 season;
    u8 isPm;
    QuestInfo *quest;

    quest = Quest_GetDiscInfo(id);
    season = LobbyWorld_GetSeason();
    isPm = LobbyTime_IsPm();
    if (gLobby->questPaid != 0) {
        if (Quest_CheckOrderTerms(quest->terms, 0) == 0) {
            return QUEST_ORDER_TERMS;
        }
    } else if (Quest_CheckOrderTerms(quest->terms, 1) == 0) {
        return QUEST_ORDER_TERMS;
    }
    if (id >= QUEST_ID_DOWNLOADED) {
        return QUEST_ORDER_OK;
    }
    return Quest_MatchesSeasonTime(id, season, isPm) != 1;
}

/*
 * 1 when quest id goes on the quest list of kind listKind (QuestList_Build's
 * kind), else 0. Not offered: no quest, a drill, a training quest, a quest
 * that must be ordered unless withMustOrder, a quest out of the town's
 * season or time of day, a quest of a map group not open
 * (MapGroup_IsOpen). Offline, the village's quests (ids 1001 on) each wait
 * for a village condition or an event flag, the others for their monster
 * (Monster_IsAvailable); online, quests 5001-5008 are never offered and the
 * others wait for their monster. A quest of type 6 on a monster is offered
 * only when the forecast picks it (ElderForecast_QuestAppears).
 */
s32 Quest_IsOffered(u16 id, s32 listKind, s32 withMustOrder)
{
    QuestInfo *quest = Quest_GetDiscInfo(id);
    u8 season = LobbyWorld_GetSeason();
    u8 isPm = LobbyTime_IsPm();

    if (quest == NULL) {
        return 0;
    }
    if (quest->flags & QUEST_FLAG_DRILL) {
        return 0;
    }
    if (withMustOrder == 0 && (quest->flags & QUEST_FLAG_MUST_ORDER)) {
        return 0;
    }
    if (quest->flags & QUEST_FLAG_TRAINING) {
        return 0;
    }
    if (Quest_MatchesSeasonTime(id, season, isPm) == 0) {
        return 0;
    }
    if (Game_IsOnline() == 0) {
        if (MapGroup_IsOpen(Map_Group(quest->map)) == 0) {
            return 0;
        }
        switch (id) {
        case 1001:
            break;
        case 1002:
        case 1003:
        case 1004:
        case 1005:
            if (Cond_Check(3) == 0) {
                return 0;
            }
            break;
        case 1006:
        case 1008:
        case 1021:
        case 1022:
        case 1023:
        case 1024:
            if (Cond_Check(7) == 0) {
                return 0;
            }
            break;
        case 1011:
        case 1012:
        case 1026:
        case 1038:
        case 1039:
        case 1040:
        case 1042:
        case 1044:
            if (Cond_Check(0xA) == 0) {
                return 0;
            }
            break;
        case 1014:
        case 1016:
        case 1056:
        case 1059:
        case 1060:
            if (Cond_Check(0x10) == 0) {
                return 0;
            }
            break;
        case 1017:
        case 1018:
        case 1030:
        case 1032:
        case 1033:
        case 1063:
        case 1064:
        case 1065:
        case 1066:
        case 1067:
        case 1068:
        case 1070:
        case 1071:
            if (Cond_Check(0x15) == 0) {
                return 0;
            }
            break;
        case 1036:
        case 1037:
        case 1061:
        case 1062:
        case 1074:
        case 1075:
        case 1084:
        case 1085:
            if (Cond_Check(0x1A) == 0) {
                return 0;
            }
            break;
        case 1034:
        case 1035:
        case 1054:
        case 1055:
        case 1072:
        case 1073:
            if (Cond_Check(0x1B) == 0) {
                return 0;
            }
            break;
        case 1009:
            if (Save_TestEventFlag(0x9A) == 0) {
                return 0;
            }
            break;
        case 1049:
        case 1050:
            if (Save_TestEventFlag(EVENT_FLAG_CONGALALA_SCENE) == 0) {
                return 0;
            }
            break;
        case 1047:
        case 1048:
            if (Save_TestEventFlag(0xA5) == 0) {
                return 0;
            }
            break;
        case 1081:
        case 1082:
            if (Save_TestEventFlag(EVENT_FLAG_KIRIN_SCENE) == 0) {
                return 0;
            }
            break;
        case 1046:
            if (Save_TestEventFlag(0x9C) == 0) {
                return 0;
            }
            break;
        case 1045:
            if (Save_TestEventFlag(0xA7) == 0) {
                return 0;
            }
            break;
        case 1078:
            if (Save_TestEventFlag(0xAB) == 0) {
                return 0;
            }
            break;
        default:
            if ((quest->goals[0].flags & QUEST_GOAL_MONSTER) && Monster_IsAvailable(quest->goals[0].target) == 0) {
                return 0;
            }
            break;
        case 1086:
        case 1087:
            break;
        }
    } else {
        if (MapGroup_IsOpen(Map_Group(quest->map)) == 0) {
            return 0;
        }
        switch (id) {
        case 5001:
        case 5002:
        case 5003:
        case 5004:
        case 5005:
        case 5006:
        case 5007:
        case 5008:
            return 0;
        default:
            if ((quest->goals[0].flags & QUEST_GOAL_MONSTER) && Monster_IsAvailable(quest->goals[0].target) == 0) {
                return 0;
            }
            break;
        }
    }
    if ((quest->goals[0].flags & QUEST_GOAL_MONSTER) && quest->type == 6 && ElderForecast_QuestAppears(id, listKind) == 0) {
        return 0;
    }
    return 1;
}

void *memset(void *dst, s32 c, u32 n);
void Npc_RefreshOffers(void);
void QuestList_Build(s32);
extern s32 gTownMode;
extern s8 gQuestListRebuildPending;

/*
 * 1 when the elder's forecast is told: online under the village's condition
 * 0x61, offline under condition 0x15 (Cond_Check); else 0.
 */
s32 ElderForecast_IsUnlocked(void)
{
    if (Game_IsOnline() == 1) {
        if (Cond_Check(0x61) == 0) {
            return 0;
        }
    } else if (Cond_Check(0x15) == 0) {
        return 0;
    }
    return 1;
}

s32 LobbyWorld_GetSeason(void);

/*
 * 1 when the forecast puts quest id on the list of kind listKind, else 0:
 * the forecast must be told (ElderForecast_IsUnlocked), quest 0x2F2E also
 * needs monster 11 hunted (gSave.hunted), and the quest must be one of the
 * season's five forecast quests (gTownQuestLists' first row for the list
 * kinds of quest list set 0, the second for QUEST_LIST_ONLINE_DESK_2's);
 * then a 40 in 100 roll.
 */
s32 ElderForecast_QuestAppears(u16 id, s32 listKind)
{
    TownQuestLists *lists = &gTownQuestLists;
    s32 row;
    s32 i;
    s32 season;

    if (ElderForecast_IsUnlocked() == 0) {
        return 0;
    }
    if (id == 0x2F2E && gSave.hunted[11] == 0) {
        return 0;
    }
    if ((u32)listKind <= QUEST_LIST_ONLINE || listKind == QUEST_LIST_AWAY) {
        row = 0;
    } else {
        row = 1;
    }
    season = LobbyWorld_GetSeason();
    for (i = 0; i < 5; i++) {
        if (lists->quests[row][season][i] == id && Rand_Next(1) % 100 < 40) {
            return 1;
        }
    }
    return 0;
}

extern s8 gOffersRefreshPending;
struct Em;
s16 OfferMenu_BuildList(struct Em *e, s32 b);
s32 MyHouse_IsOwner(void);
s32 Kitchen_SetCourseReady(void);

/*
 * Sets each townsperson's offer mark (NpcWork.offerMark) from the
 * offers it has (OfferMenu_BuildList): 0 none, 2 for its result 4, else 1;
 * townsperson 0x71 shows 2 at the player's own house when the meal points
 * are ready (Kitchen_SetCourseReady). While a menu other than a talk is
 * open, the refresh is put off (gOffersRefreshPending).
 */
void Npc_RefreshOffers(void)
{
    NpcWork *npc;
    s32 i;
    Em *em;

    if (gTownMode == TOWN_MODE_FREE || gTownMode == TOWN_MODE_TALK) {
        for (i = 0, em = gEms; i < EM_COUNT; i++, em++) {
            if (em->active != 0) {
                npc = (NpcWork *)&em->work;
                switch (OfferMenu_BuildList((struct Em *)em, 1)) {
                default:
                    npc->offerMark = 1;
                    break;
                case 0:
                    npc->offerMark = 0;
                    break;
                case 4:
                    npc->offerMark = 2;
                    break;
                }
                if (npc->id == 0x71 && MyHouse_IsOwner() != 0 && Kitchen_SetCourseReady() == 1) {
                    npc->offerMark = 2;
                }
            }
        }
    } else {
        gOffersRefreshPending = 1;
    }
}

/*
 * Rebuilds the quest lists (gQuestLists): offline QUEST_LIST_AWAY while away
 * from the village, else QUEST_LIST_VILLAGE; online QUEST_LIST_ONLINE and
 * QUEST_LIST_ONLINE_DESK_2. Then refreshes the
 * townspeople's offers. While the quest desk is open only marks the
 * rebuild pending (QuestDesk_Close does it).
 */
void QuestList_Rebuild(void)
{
    if (gTownMode == TOWN_MODE_QUEST_DESK) {
        gQuestListRebuildPending = 1;
        return;
    }
    memset(&gQuestLists, 0, sizeof(gQuestLists));
    if (Game_IsOnline() == 0) {
        if (gSave.awayFromVillage != 0) {
            QuestList_Build(QUEST_LIST_AWAY);
        } else {
            QuestList_Build(QUEST_LIST_VILLAGE);
        }
    } else {
        QuestList_Build(QUEST_LIST_ONLINE);
        QuestList_Build(QUEST_LIST_ONLINE_DESK_2);
    }
    Npc_RefreshOffers();
}

/*
 * Picks the quests the quest board shows (gQuestDeskShown[set]) from the
 * quest list set: each quest of the set, in order, has a 40 in 100 chance,
 * up to QUEST_BOARD_LEN. Offline, while village condition 7 is unmet
 * (Cond_Check), every quest is taken. When none was picked, the set's first
 * quest is shown. Returns 0.
 */
s32 QuestList_PickShown(s8 set)
{
    s32 i;
    s32 shownCount;
    u16 *id;
    u16 *setIds;
    u16 *shown;

    shownCount = 0;
    shown = gQuestDeskShown[set];
    memset(shown, 0, sizeof(gQuestDeskShown[set]));
    i = 0;
    setIds = gQuestLists[set];
    id = setIds;
    for (; i < QUEST_SET_LEN; i++, id++) {
        if (*id == 0) {
            break;
        }
        if (shownCount <= QUEST_BOARD_LEN - 1) {
            if (Game_IsOnline() == 0) {
                if ((Rand_Next(1) & 0xFFFF) % 100 < 40 || Cond_Check(7) == 0) {
                    gQuestDeskShown[set][shownCount++] = *id;
                }
            } else {
                if ((Rand_Next(1) & 0xFFFF) % 100 <= 39) {
                    gQuestDeskShown[set][shownCount++] = *id;
                }
            }
        }
    }
    if (shownCount == 0) {
        shown[0] = setIds[0];
    }
    return 0;
}

extern u16 *gQuestIdLists[];
s32 Quest_IsOffered(u16 id, s32 kind, s32 c);
s32 QuestId_IsStandard(s32 id);
void QuestList_DropRandom(s8 set);

/*
 * Builds the quest list set of list kind `kind` (enum QuestListKind):
 * QUEST_LIST_ONLINE_DESK_2 fills set 1 from the source lists
 * gQuestIdLists[4..7], the other kinds set 0 from gQuestIdLists[0..3]. A
 * quest goes in when Quest_IsOffered allows it and its id fits the kind:
 * offline kinds take the standard quests (QuestId_IsStandard), online kinds
 * the others; at most QUEST_SET_LEN. Online, or once village condition 7 is
 * met, part of each map group's quests is dropped at random
 * (QuestList_DropRandom). Then the board's quests are picked
 * (QuestList_PickShown).
 */
void QuestList_Build(s32 kind)
{
    s8 set;
    s32 i;
    s32 end;
    s32 count;
    u16 *id;


    switch (kind) {
    case QUEST_LIST_VILLAGE:
    case QUEST_LIST_ONLINE:
    case QUEST_LIST_AWAY:
        set = 0;
        i = 0;
        end = 4;
        break;
    case QUEST_LIST_ONLINE_DESK_2:
        set = 1;
        i = 4;
        end = 8;
        break;
    }
    count = 0;
    for (; i < end; i++) {

        id = gQuestIdLists[i];
        while (*id != 0) {
            if (Quest_IsOffered(*id, kind, 0) == 1) {
                if (kind == QUEST_LIST_VILLAGE || kind == QUEST_LIST_AWAY) {
                    if (QuestId_IsStandard(*id) == 1) {
                        gQuestLists[set][count++] = *id;
                    }
                } else if (QuestId_IsStandard(*id) == 0) {
                    gQuestLists[set][count++] = *id;
                }
            }
            if (count > QUEST_SET_LEN - 1) {
                break;
            }
            id++;
        }
        if (count > QUEST_SET_LEN - 1) {
            break;
        }
    }
    if (Game_IsOnline() == 1 || Cond_Check(7) == 1) {
        QuestList_DropRandom(set);
    }
    QuestList_PickShown(set);
}

/*
 * The number of quests in quest list set `set` whose map is in map group
 * `group` (Map_Group).
 */
s32 QuestList_CountGroup(s32 group, s8 set)
{
    s32 count;
    s32 i;
    s32 questGroup;

    count = 0;
    for (i = 0; i < QUEST_SET_LEN; i++) {
        if (gQuestLists[set][i] == 0) {
            break;
        }
        /* Map_Group returns a u16; it is declared s32 above for Quest_IsOffered, and the cast keeps the zero-extension the game's code does here */
        questGroup = (u16)Map_Group(Quest_GetDiscInfo(gQuestLists[set][i])->map);
        if (questGroup == group) {
            count++;
        }
    }
    return count;
}

/*
 * Drops quests of map group `group` from quest list set `set` at random,
 * keeping the group's quest number `keep` (counted in the set's order): a
 * quest on EM_KIND_RAJANG goes with a 90 in 100 chance, any other quest
 * not of type 6 (the forecast's) with 20 in 100. The set is closed up
 * behind each dropped quest.
 */
void QuestList_DropGroup(s32 group, s32 keep, s8 set)
{
    u16 drop[QUEST_SET_LEN];
    QuestInfo *info;
    s32 groupIdx;
    s32 i;
    s32 questGroup;
    s32 j;
    u16 *setIds;
    u16 *dropFlag;
    u16 *last;

    memset(drop, 0, sizeof(drop));
    groupIdx = 0;
    i = 0;
    setIds = gQuestLists[set];
    for (; i < QUEST_SET_LEN; i++) {
        if (setIds[i] == 0) {
            break;
        }
        info = Quest_GetDiscInfo(setIds[i]);
        /* the cast as in QuestList_CountGroup */
        questGroup = (u16)Map_Group(info->map);
        if (questGroup == group) {
            if (groupIdx != keep) {
                if (info->goals[0].target == EM_KIND_RAJANG) {
                    if ((Rand_Next(1) & 0xFFFF) % 100 <= 89) {
                        drop[i] = 1;
                    }
                } else if (info->type != 6) {
                    if ((Rand_Next(1) & 0xFFFF) % 100 <= 19) {
                        drop[i] = 1;
                    }
                }
            }
            groupIdx++;
        }
    }
    i = QUEST_SET_LEN - 1;
    dropFlag = &drop[QUEST_SET_LEN - 1];
    last = &setIds[QUEST_SET_LEN - 1];
    for (; i >= 0; i--, dropFlag--) {
        if (*dropFlag == 1) {
            for (j = i; j < QUEST_SET_LEN - 1; j++) {
                setIds[j] = setIds[j + 1];
            }
            *last = 0;
        }
    }
}

/*
 * Thins quest list set `set` out: for each map group 0-25 with quests in
 * the set, one of them chosen at random is kept and the others may be
 * dropped (QuestList_DropGroup).
 */
void QuestList_DropRandom(s8 set)
{
    s32 group;
    s32 count;

    group = 0;
    do {
        count = QuestList_CountGroup(group, set);
        if (count != 0) {
            QuestList_DropGroup(group, (s32) (Rand_Next(1) & 0xFFFF) % count, set);
        }
        group += 1;
    } while (group < 26);
}
