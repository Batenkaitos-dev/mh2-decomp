/*
 * The village's records in the save, kept between quests:
 * - a resident's gift: when its offer points pass a multiple of
 *   OFFER_POINTS_PER_GIFT, its event flag (gVillageGiftNpcs) is set; after
 *   the next quest it becomes a gift record (SaveData.gifts), handed over
 *   when the player talks to the resident (TOWN_MODE_NPC_GIFT, NpcGift_*);
 * - a resident's meal visit, likewise every OFFER_POINTS_PER_MEAL_VISIT
 *   points (gVillageMealVisitors): one at a time, cleared by the next quest;
 * - the milestones of the offer points (gVillageMilestones), each starting
 *   a village timer for some quests;
 * - the mine's count of digs, settled after each quest.
 * The offers add the points and call the checks; the quest's end calls
 * Village_ProcessEventFlags and Village_QuestTick.
 */
#include "common.h"
#include "chat_log.h"
#include "window.h"
#include "syssettings.h"
#include "item.h"
#include "lobby_state.h"

#include "lobby_menu.h"
#include "lobby_globals.h"
#include "pad.h"
#include "player.h"
#include "lobby_npc.h"
#include "em.h"
#include "scene.h"
#include "quest.h"
#include "save.h"
#include "lobby_text.h"

#include "area.h"
extern u8 gMenuWork[];
extern s32 gTownMode;
extern s32 gTownMenuOpen;
void CamFocus_Start(Em *obj);
extern VillageNpcEvent gVillageMealVisitors[];
s32 Cond_Check(u16 id);
s32 Village_HouseTourPending(void);
u16 Rand_Next(s32 stream);
s32 Save_TestEventFlag(s32 flag);
extern VillageNpcEvent gVillageGiftNpcs[];
void *memset(void *d, s32 c, u32 n);
u8 Village_FacilityLevel(s32 k);
s32 Village_MilestoneReady(s32 kind);
extern VillageMilestone gVillageMilestones[];
void Save_SetEventFlag(s32 flag);
void Save_ClearEventFlag(s32 flag);
s32 SavePouch_Space(u16 item);
u8 SavePouch_Add(u16 item, s16 n);
u16 LobbyPad_Get(s32 a);
s32 Talk_Update(void);
struct Em;
void Talk_Say(s32 msg, s8 a);
void CamFocus_End(void);
void LobbyMenu_Init(void);
/* Returns the talk line of resident's meal visit (the meal scene says it), 0 for none. */
s16 Village_MealVisitTalk(s32 resident)
{
    VillageNpcEvent *entry;
    u16 id;

    id = gVillageMealVisitors[0].id;
    entry = gVillageMealVisitors;
    while (id != 0) {
        if (id == (u16)resident) {
            return entry->talk;
        }
        entry++;
        id = entry->id;
    }
    return 0;
}

/* Clears resident's meal visit once its meal is eaten. */
void Village_ClearMealVisit(u16 resident)
{
    VillageNpcEvent *entry;
    u16 id;
    s16 i;

    id = gVillageMealVisitors[0].id;
    entry = gVillageMealVisitors;
    i = 0;
    while (id != 0) {
        if (id == resident) {
            memset(&gSave.mealVisits[i], 0, sizeof(MealVisit));
            return;
        }
        entry++;
        id = entry->id;
        i++;
    }
}

extern OfferNpc gOfferNpcs[];
/*
 * Checks the offer resident offerIdx (gOfferNpcs) after its points went from
 * before to after: past a multiple of OFFER_POINTS_PER_GIFT its gift flag
 * is set, past one of OFFER_POINTS_PER_MEAL_VISIT its meal visit flag.
 */
void Offer_CheckResidentRewards(u32 before, u32 after, s32 offerIdx)
{
    u16 resident = gOfferNpcs[offerIdx].id;
    VillageNpcEvent *entry;
    u16 id;

    if (before / OFFER_POINTS_PER_GIFT < after / OFFER_POINTS_PER_GIFT) {
        id = gVillageGiftNpcs[0].id;
        entry = gVillageGiftNpcs;
        while (id != 0) {
            if (id == resident && Save_TestEventFlag(entry->eventFlag) == 0) {
                Save_SetEventFlag(entry->eventFlag);
            }
            entry++;
            id = entry->id;
        }
    }
    if (Cond_Check(0x1A) == 1 && resident == 1) { /* resident 1 makes no meal visits under condition 0x1A */
        return;
    }
    if (before / OFFER_POINTS_PER_MEAL_VISIT < after / OFFER_POINTS_PER_MEAL_VISIT) {
        id = gVillageMealVisitors[0].id;
        entry = gVillageMealVisitors;
        while (id != 0) {
            if (id == resident && Save_TestEventFlag(entry->eventFlag) == 0) {
                Save_SetEventFlag(entry->eventFlag);
            }
            entry++;
            id = entry->id;
        }
    }
}

/*
 * Sets milestone idx's event flag when the points went past a multiple of
 * its step and the milestone can be reached; milestone 0 also lets the
 * village's return tour play again.
 */
void Village_CheckMilestone(u32 before, u32 after, s32 idx)
{
    VillageMilestone *milestone = &gVillageMilestones[idx];

    if (before / milestone->pointStep < after / milestone->pointStep) {
        if (Village_MilestoneReady(idx) == 1) {
            if (Save_TestEventFlag(milestone->eventFlag) == 0) {
                Save_SetEventFlag(milestone->eventFlag);
                if (idx == 0) {
                    Save_ClearEventFlag(EVENT_FLAG_TOUR_VILLAGE_RETURN); /* the tour plays again */
                }
            }
        }
    }
}

/*
 * Returns whether a milestone can be reached: 0 needs the port at level 2,
 * 4 the smithy at level 2, 5 condition 0x1A, 2 event flag 0x49, 6
 * EVENT_FLAG_PSP_LINK_1; the others always.
 */
s32 Village_MilestoneReady(s32 milestone)
{
    switch (milestone) {
    case 0:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) < 2) {
            return 0;
        }
        break;
    case 4:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) < 2) {
            return 0;
        }
        break;
    case 5:
        if (Cond_Check(0x1A) == 0) {
            return 0;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x49) == 0) {
            return 0;
        }
        break;
    case 6:
        if (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_1) == 0) {
            return 0;
        }
        break;
    }
    return 1;
}

/* Returns 1 while the house's tour for its level has not played; the meal visits wait for it. */
s32 Village_HouseTourPending(void)
{
    switch ((u8)Village_FacilityLevel(VILLAGE_FACILITY_HOUSE)) {
    case 1:
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_HOUSE_1) == 0) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(EVENT_FLAG_TOUR_HOUSE_2) == 0) {
            return 1;
        }
        break;
    }
    return 0;
}

/*
 * After a quest, turns the set event flags into save records: the first
 * pending meal visit (unless the house's tour is pending), every pending
 * gift (the 3rd, 6th, 10th, 15th and 20th milestone gifts 0 to 4, the count
 * starting again after the 20th; the others a random gift of the
 * resident's offer kind), and every reached milestone's timer; milestone 1
 * takes state 2 by a 30 percent chance once the item box is past level 1.
 * Event flag 0x49 without 0x4A keeps timer 2 running for one quest.
 */
void Village_ProcessEventFlags(void)
{
    VillageNpcEvent *entry;
    u16 id;
    s32 i;
    VillageMilestone *milestone;

    if (Village_HouseTourPending() == 0) {
        i = 0;
        id = gVillageMealVisitors[0].id;
        entry = gVillageMealVisitors;
        while (id != 0) {
            if (Save_TestEventFlag(entry->eventFlag) == 1 && gSave.mealVisits[i].pending == 0) {
                gSave.mealVisits[i].pending = 1;
                gSave.mealVisits[i].id = entry->id;
                Save_ClearEventFlag(entry->eventFlag);
                break;
            }
            entry++;
            id = entry->id;
            i++;
        }
    }
    i = 0;
    id = gVillageGiftNpcs[0].id;
    entry = gVillageGiftNpcs;
    while (id != 0) {
        if (Save_TestEventFlag(entry->eventFlag) == 1 && gSave.gifts[i].state == NPC_GIFT_NONE) {
            gSave.gifts[i].id = entry->id;
            gSave.giftCount++;
            switch (gSave.giftCount) {
            case 3:
                gSave.gifts[i].state = NPC_GIFT_MILESTONE;
                gSave.gifts[i].gift = 0;
                break;
            case 6:
                gSave.gifts[i].state = NPC_GIFT_MILESTONE;
                gSave.gifts[i].gift = 1;
                break;
            case 10:
                gSave.gifts[i].state = NPC_GIFT_MILESTONE;
                gSave.gifts[i].gift = 2;
                break;
            case 15:
                gSave.gifts[i].state = NPC_GIFT_MILESTONE;
                gSave.gifts[i].gift = 3;
                break;
            case 20:
                gSave.gifts[i].state = NPC_GIFT_MILESTONE;
                gSave.gifts[i].gift = 4;
                gSave.giftCount = 0;
                break;
            default:
                gSave.gifts[i].state = NPC_GIFT_TABLE;
                gSave.gifts[i].gift = Rand_Next(1) % NPC_GIFT_CHOICES;
                break;
            }
            Save_ClearEventFlag(entry->eventFlag);
        }
        entry++;
        id = entry->id;
        i++;
    }
    milestone = gVillageMilestones;
    for (i = 0; i < VILLAGE_MILESTONES; i++) {
        if (Save_TestEventFlag(milestone->eventFlag) == 1 && gSave.villageTimers[i].state == 0) {
            gSave.villageTimers[i].state = 1;
            switch (i) {
            case 1:
                if (Rand_Next(1) % 100 < 30 && (u8)Village_FacilityLevel(VILLAGE_FACILITY_BOX) > 1) {
                    gSave.villageTimers[i].state = 2;
                }
                break;
            }
            gSave.villageTimers[i].questsLeft = milestone->quests;
            Save_ClearEventFlag(milestone->eventFlag);
        }
        milestone++;
    }
    if (Save_TestEventFlag(0x49) == 1 && Save_TestEventFlag(0x4A) == 0) {
        gSave.villageTimers[2].state = 1;
        gSave.villageTimers[2].questsLeft = 1;
    }
}

void Village_SetFacilityLevel(s32 sub, u8 lvl);
/*
 * After a quest: unless it was abandoned, a digging mine is dug and its
 * bonus counts down by the digs; at 0 the mine goes up a level (event flag
 * 0x1C7 from level 6). Then each village timer counts down one quest, and
 * every meal visit is cleared.
 */
void Village_QuestTick(void)
{
    u8 level;
    s32 i;
    s16 bonus;
    SaveData *save;

    if (gQuestRecordPhase != QUEST_PHASE_ABORTED && gSave.mine.active == MINE_DIGGING) {
        bonus = gSave.mine.bonus;
        gSave.mine.active = MINE_DUG;
        if (bonus > 0) {
            gSave.mine.bonus = bonus - gSave.mine.digs;
            if (gSave.mine.bonus <= 0) {
                level = Village_FacilityLevel(VILLAGE_FACILITY_MINE);
                if (level < 9) { /* the mine's highest level */
                    Village_SetFacilityLevel(VILLAGE_FACILITY_MINE, level + 1);
                    if (level + 1 >= 6) {
                        Save_SetEventFlag(0x1C7);
                    }
                }
                gSave.mine.bonus = 0;
            }
        }
    }
    i = 0;
    save = &gSave;
    for (; i < VILLAGE_MILESTONES; i++) {
        if (save->villageTimers[i].questsLeft != 0) {
            if (--save->villageTimers[i].questsLeft <= 0) {
                memset(&save->villageTimers[i], 0, sizeof(VillageTimer));
            }
        }
    }
    save = &gSave;
    for (i = 0; i < MEAL_VISITS; i++) {
        memset(&save->mealVisits[i], 0, sizeof(MealVisit));
    }
}

/* Returns village timer idx's state: 0 none, 1 or 2 running. */
s16 Village_MilestoneState(s32 idx)
{
    return gSave.villageTimers[idx].state;
}

/* Returns 1 when resident has a gift waiting. */
s32 Village_HasGift(u16 resident)
{
    VillageNpcEvent *entry;
    u16 id;
    s32 i;

    id = gVillageGiftNpcs[0].id;
    entry = gVillageGiftNpcs;
    i = 0;
    while (id != 0) {
        if (resident == id) {
            return gSave.gifts[i].state != NPC_GIFT_NONE;
        }
        entry++;
        id = entry->id;
        i++;
    }
    return 0;
}

/*
 * Returns 1 when resident has a meal visit waiting; never for resident 1
 * under condition 0x1A, nor while the house's tour is pending.
 */
s32 Village_HasMealVisit(u16 resident)
{
    VillageNpcEvent *entry = gVillageMealVisitors;
    s32 i;
    u16 id;

    if (Cond_Check(0x1A) == 1 && resident == 1) {
        return 0;
    }
    if (Village_HouseTourPending() == 1) {
        return 0;
    }
    id = entry->id;
    i = 0;
    while (id != 0) {
        if (resident == id) {
            return gSave.mealVisits[i].pending != 0;
        }
        entry++;
        id = entry->id;
        i++;
    }
    return 0;
}

extern NpcGiftItem *gNpcGiftsByKind[];
extern NpcGiftItem gNpcMilestoneGifts[];
extern u8 gMenuInputLock;
extern u16 gLobbyEnterRand;
s32 Offer_GetNpcIndex(u16 id);
/*
 * Opens resident npc's gift (TOWN_MODE_NPC_GIFT): copies the gift from its
 * table (its offer kind's, or the milestone gifts), notes the record, turns
 * the camera to it and has it say one of three lines. Nothing opens when it
 * has no gift, or a table gift and no offers.
 */
void NpcGift_Open(Em *npc)
{
    u8 *resident = &npc->work; /* NpcWork.id */
    NpcGift *gift = (NpcGift *)gMenuWork;
    VillageNpcEvent *entry;
    s32 i;
    u16 id;
    s32 offerIdx;
    s8 state;

    memset(gift, 0, MENU_WORK_SIZE);
    i = 0;
    id = gVillageGiftNpcs[0].id;
    entry = gVillageGiftNpcs;
    while (id != 0) {
        if (id == *resident) {
            state = gSave.gifts[i].state;
            if (state != NPC_GIFT_NONE) {
                if (state == NPC_GIFT_TABLE) {
                    offerIdx = Offer_GetNpcIndex(id);
                    if (offerIdx == -1) {
                        return;
                    }
                    gift->give = gNpcGiftsByKind[gOfferNpcs[offerIdx].kind][gSave.gifts[i].gift];
                } else {
                    gift->give = gNpcMilestoneGifts[gSave.gifts[i].gift];
                }
                gift->record = i;
                break;
            } else {
                return;
            }
        }
        entry++;
        id = entry->id;
        i++;
    }
    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_NPC_GIFT;
    LobbyMenu_Init();
    gMenuInputLock = 6; /* frames */
    CamFocus_Start(npc);
    Talk_Say(entry->talk + gLobbyEnterRand % 3, 0); /* one of its three gift lines */
}

/* Closes the gift: back to TOWN_MODE_FREE, the lobby menu reset, menu input
 * held for 6 frames and the camera's focus on the resident released. */
void NpcGift_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_Init();
    gMenuInputLock = 6;
    CamFocus_End();
}

u16 LobbyPad_Get(s32 a);
s32 Talk_Update(void);
s32 SavePouch_Space(u16 item);
u8 SavePouch_Add(u16 item, s16 n);
u8 Notice_Push(u8, s16, s16);
void Menu_PushNotice(s32 msg);
/* Runs the gift each frame: waits for the resident's words to end, then
 * puts the item in the pouch, shows the received-item notice and clears
 * the resident's record; with no room in the pouch it shows the pouch-full
 * notice and keeps the record for another time. Either way the gift closes.
 * The pad is read and its value not used. */
void NpcGift_Update(void)
{
    s16 count;
    NpcGift *gift = (NpcGift *)gMenuWork;

    LobbyPad_Get(0);
    switch (gift->step) {
    case NPC_GIFT_STEP_TALK:
        switch (Talk_Update()) {
        case MENU_DONE:
            gift->step++;
            break;
        }
        break;
    case NPC_GIFT_STEP_GIVE:
        count = gift->give.count;
        if (SavePouch_Space(gift->give.item) >= count) {
            SavePouch_Add(gift->give.item, count);
            Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, gift->give.item);
            memset(&gSave.gifts[gift->record], 0, sizeof(VillageGift));
        } else {
            Menu_PushNotice(LOBBY_NOTICE_POUCH_FULL);
        }
        NpcGift_Close();
        break;
    }
}

/* The file's initialised data, in the game's order */
VillageMilestone gVillageMilestones[VILLAGE_MILESTONES + 1] = {
#include "lobby/007E8350.inc"
};
VillageNpcEvent gVillageGiftNpcs[16] = {
#include "lobby/007E8390.inc"
};
VillageNpcEvent gVillageMealVisitors[13] = {
#include "lobby/007E83F0.inc"
};
NpcGiftItem gNpcGiftsByKind_0[8] = {
#include "lobby/007E8440.inc"
};
NpcGiftItem gNpcGiftsByKind_1[8] = {
#include "lobby/007E8460.inc"
};
NpcGiftItem gNpcGiftsByKind_2[6] = {
#include "lobby/007E8480.inc"
};
NpcGiftItem *gNpcGiftsByKind[3] = {
    gNpcGiftsByKind_0, gNpcGiftsByKind_1, gNpcGiftsByKind_2,
};
NpcGiftItem gNpcMilestoneGifts[8] = {
#include "lobby/007E84B0.inc"
};
