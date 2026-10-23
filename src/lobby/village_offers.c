/* The offline village's offers. Each offer townsperson (gOfferNpcs, an
 * OfferNpc: a kind 0-2 and four request templates) can hold an item request
 * (gSave.offers, an OfferRec built from a gRequestDefs template: one to
 * three items wanted, one of three titles and texts, and a reward in
 * points) or a quest (gSave.offerQuests).
 *
 * Offer_Generate deals new offers out by random rolls, on a new day or after
 * a quest; the offer menu shows them and finishes them. Offer_AddPoints
 * turns the rewards into points per townsperson, per kind and in total,
 * which reach the kinds' milestones (gOfferMilestonesA/B, event flags) and
 * the village's (Village_CheckMilestone). */
#include "common.h"
#include "em.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "area.h"
#include "village_offers.h"
#pragma optimization_level 4

s32 Offer_FindNpc(); /* unit's own declaration: declared without parameters, as Offer_GetQuest and Offer_FindRequest call it with no argument, the townsperson id they were given still in the argument register; with a prototype the compiler passes the id itself and their code differs */
#include "save.h"
#include "quest.h"
#include "lobby_state.h"
#include "village_cond.h"
#include "elder_forecast.h"
#include "lobby_world.h"
#include "quest_offers.h"
#include "rand.h"
#include "village_records.h"
#include "libc.h"
#include "lobby_npc.h"

#define OFFER_OPEN_MAX          3       /* the most offers open at once */
#define OFFER_OPEN_MAX_UPGRADED 4       /* the same once the tavern, smithy and port levels add up to 2 */
#define OFFER_NPC_NONE          0xFFFF  /* Offer_PickNpc: no townsperson of the kind is present */
#define OFFER_QUEST_NONE        0xFFFF  /* Offer_GetQuest: the townsperson offers no quest */
#define OFFER_SLOTS             16      /* gSave.offers, offerQuests and offerPoints: one per townsperson index */
#define OFFER_KIND_COUNT        3       /* the townsperson kinds (OfferNpc.kind) */
#define OFFER_KINDS_ALL         7       /* gSave.offerKindsUsed with every kind's bit set */
#define OFFER_KIND_TOTAL        3       /* offerKindPoints' and offerMilePoints' slot for the total */
#define OFFER_MILESTONES        5       /* gOfferMilestonesA/B: a kind's milestones */
#define REQUEST_ITEM_SLOTS      9       /* RequestDef.items: a template's item slots */
#define REQUEST_TEXTS           3       /* RequestDef.title and .text: the choices */
#define OFFER_POINTS_CAP        2000    /* offerPoints and offerMilePoints start again at 0 above it */
#define OFFER_KIND_POINTS_CAP   100000  /* offerKindPoints start again at 0 above it */
#define OFFER_QUEST_STARS_MAX   4       /* a quest's stars run 1-4; a quest without disc info counts as 4 */
#define QUEST_PICK_MAX          100     /* Offer_PickQuest gathers at most this many quests */

/* A request template (gRequestDefs, 0x1BA bytes). Its nine item slots are
 * read as halfwords, five per slot: the item, then two choices of {count,
 * points} (Offer_Generate takes one by a random bit). */
typedef struct RequestDef {
    s16 items[REQUEST_ITEM_SLOTS * 5];
    char title[REQUEST_TEXTS][29];    /* 0x5A */
    char text[REQUEST_TEXTS][88];     /* 0xB1 */
    u8 pad1B9;
} RequestDef;
extern RequestDef gRequestDefs[];

u8 Village_FacilityLevel(s32 k);
/* A season day's roll (gOfferDayRolls, by season day): the chance that offers come, and whether the first is a quest. */
typedef struct OfferDayRoll {
    s8 chance;          /* 0x0: percent */
    s8 isQuest;         /* 0x1 */
} OfferDayRoll;
/* A roll of how many offers come (gOfferCountRolls3/4): tried in order, the first that succeeds gives the count. */
typedef struct OfferCountRoll {
    s8 chance;          /* 0x0: percent */
    s8 count;           /* 0x1: the new offers */
} OfferCountRoll;
extern OfferDayRoll gOfferDayRolls[4];
extern OfferCountRoll gOfferCountRolls3[4];
extern OfferCountRoll gOfferCountRolls4[4];

/*
 * Deals out new offers (offline, on a new day or after a quest) once the
 * offers are unlocked (condition 7): at most OFFER_OPEN_MAX open at once,
 * OFFER_OPEN_MAX_UPGRADED once the tavern, smithy and port levels add up to
 * 2. The season day's roll (gOfferDayRolls: a chance, then whether the first
 * is a quest) decides whether any come; a second roll (gOfferCountRolls3/4)
 * how many. Each goes to a present townsperson of the next kind not yet given
 * one, alternating requests and quests: a request takes one of the
 * townsperson's first two templates (or, after condition 0x15, half the time
 * one of the last two), one to three distinct item slots with one of each
 * slot's two count and point choices, a random title and text, and the
 * points summed; a quest takes a random standard quest (Offer_PickQuest).
 * i is first the most open, then counts the new offers.
 */
void Offer_Generate(void)
{
    s16 *choice;
    OfferRec *req;
    s8 openCount;
    s16 total;
    u16 takenSlots;
    s8 j;
    u16 defIdx;
    OfferCountRoll *countRolls;
    s8 kind;
    s8 itemIdx;
    s32 npcIdx;
    u16 itemCount;
    s8 isQuest;
    s32 slotOfs;
    s8 i;
    s8 chance;
    s8 *npc;
    s8 newCount;
    s32 seasonDay;

    if (Cond_Check(7) == 0) {
        return;
    }
    if ((s8)((u8)Village_FacilityLevel(VILLAGE_FACILITY_TAVERN) + (u8)Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) + (u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT)) >= 2) {
        i = OFFER_OPEN_MAX_UPGRADED;
        countRolls = gOfferCountRolls4;
    } else {
        i = OFFER_OPEN_MAX;
        countRolls = gOfferCountRolls3;
    }
    openCount = Offer_CountOpen();
    if (openCount >= i) {
        return;
    }
    seasonDay = LobbyWorld_GetSeasonDay();
    chance = gOfferDayRolls[seasonDay].chance;
    isQuest = gOfferDayRolls[seasonDay].isQuest;
    if (Rand_Next(1) % 100 > chance) {
        return;
    }
    for (j = 0; j < i; j++) {
        chance = countRolls[j].chance;
        if (Rand_Next(1) % 100 <= chance) {
            newCount = countRolls[j].count;
            break;
        }
    }
    /* the game reads a register it never set: cnt is s3, still the caller's, when no roll succeeds */
    if (newCount + openCount > i) {
        newCount = i - openCount;
    }
    for (i = 0; i < newCount; i++) {
        kind = Offer_PickKind(Rand_Next(1));
        npcIdx = Offer_PickNpc(Rand_Next(1), kind);
        if (npcIdx == OFFER_NPC_NONE) {
            continue;
        }
        if (isQuest == 0) {
            req = &gSave.offers[npcIdx];
            memset(req, 0, sizeof(OfferRec));
            npc = (s8 *)&gOfferNpcs[npcIdx];
            if (Cond_Check(21) == 1 && (Rand_Next(1) & 1)) {
                defIdx = npc[(Rand_Next(1) & 1) + 3];
            } else {
                defIdx = npc[(Rand_Next(1) & 1) + 1];
            }
            itemCount = Rand_Next(1) % 3 + 1;
            takenSlots = 0;
            total = 0;
            for (itemIdx = 0; itemIdx < itemCount; itemIdx++) {
                slotOfs = (s16)Request_TakeItemSlot(&takenSlots, Rand_Next(1)) * 5;
                req->want[itemIdx].item = *(u16 *)&gRequestDefs[defIdx].items[slotOfs];
                choice = &gRequestDefs[defIdx].items[slotOfs] + (Rand_Next(1) & 1) * 2;
                req->want[itemIdx].count = choice[1];
                total += choice[2];
            }
            req->offerId = defIdx;
            req->textSel = Rand_Next(1) % 3;
            req->points = total;
            req->active = 1;
        } else {
            gSave.offerQuests[(u16)npcIdx] = Offer_PickQuest(Rand_Next(1));
        }
        isQuest ^= 1;
        gSave.offerKindsUsed |= 1 << kind;
    }
}

extern u16 *gQuestIdLists[6];
/*
 * Returns a random quest id (by seed) among the standard quests the village
 * offers now that are not in the elder's forecast, from the six quest id
 * lists (at most QUEST_PICK_MAX gathered).
 */
u16 Offer_PickQuest(u16 seed)
{
    s32 k;
    s32 count;
    u16 *entry;
    u16 quests[QUEST_PICK_MAX];

    memset(quests, 0, sizeof(quests));
    count = 0;
    for (k = 0; k < 6; k++) {
        for (entry = gQuestIdLists[k]; *entry != 0; entry++) {
            if (Quest_IsOffered(*entry, QUEST_LIST_VILLAGE, 0) == 1 && ElderForecast_HasQuest(*entry) == 0 && QuestId_IsStandard(*entry) == 1) {
                quests[count] = *entry;
                count++;
            }
            if (count >= QUEST_PICK_MAX) {
                break;
            }
        }
        if (count >= QUEST_PICK_MAX) {
            break;
        }
    }
    return quests[seed % count];
}

/*
 * Clears every request and offered quest, and every townsperson's offer mark
 * (the byte of its Em the NPC setup sets for an offer).
 */
void Offer_ClearAll(void)
{
    s32 i;

    memset(&gSave.offers, 0, sizeof(gSave.offers));
    memset(&gSave.offerQuests, 0, sizeof(gSave.offerQuests));
    for (i = 0; i < 20; i++) {
        gEms[i].fullCloak = 0;
    }
}

/*
 * Returns the gOfferNpcs index of townsperson npcId, or -1.
 */
s32 Offer_FindNpc(id)
    u16 id;
{
    u16 npcId = gOfferNpcs[0].id;
    OfferNpc *npc = gOfferNpcs;
    s32 i = 0;

    while (npcId != 0) {
        if (npcId == id) {
            return i;
        }
        npc++;
        npcId = npc->id;
        i++;
    }
    return -1;
}

/*
 * Returns the quest townsperson npcId offers, or OFFER_QUEST_NONE. npcId
 * reaches Offer_FindNpc in the argument register (see its declaration).
 */
u16 Offer_GetQuest(u8 npcId)
{
    s32 idx;
    u16 quest;

    idx = Offer_FindNpc();
    if (idx != -1) {
        quest = gSave.offerQuests[idx];
        if (quest != 0) {
            return quest;
        }
    }
    return OFFER_QUEST_NONE;
}

/*
 * Returns the gOfferNpcs index of townsperson npcId when it holds an open
 * request, else -1.
 */
s32 Offer_FindRequest(u8 npcId)
{
    s32 npcIdx;

    npcIdx = Offer_FindNpc();
    /* gSave.offers[ret].active: the field form changes the address arithmetic */
    if ((npcIdx != -1) && (*((s8 *)&gSave + npcIdx * 20 + 10665) != 0)) {
        return npcIdx;
    }
    return -1;
}

/*
 * Returns the title of the request at gOfferNpcs index npcIdx (the chosen one
 * of its template's three).
 */
char *Request_Title(s16 npcIdx)
{
    return gRequestDefs[gSave.offers[npcIdx].offerId].title[gSave.offers[npcIdx].textSel];
}

/*
 * Returns the text of the request at gOfferNpcs index npcIdx (the chosen one
 * of its template's three).
 */
char *Request_Text(s16 npcIdx)
{
    return gRequestDefs[gSave.offers[npcIdx].offerId].text[gSave.offers[npcIdx].textSel];
}

/*
 * Returns the gOfferNpcs index of townsperson npcId, or -1: Offer_FindNpc's
 * search, declared with its parameter (the offer menu calls this one).
 */
s32 Offer_GetNpcIndex(u16 id)
{
    s32 i = 0;
    OfferNpc *npc = gOfferNpcs;
    u16 npcId = gOfferNpcs[0].id;

    while (npcId != 0) {
        if (npcId == id) {
            return i;
        }
        npc++;
        npcId = npc->id;
        i++;
    }
    return -1;
}

/*
 * Picks one of a request template's nine item slots not yet taken: seed %
 * REQUEST_ITEM_SLOTS, else two on, else two on again (taken or not); marks it
 * in *taken and returns it.
 */
s32 Request_TakeItemSlot(u16 *taken, u16 seed)
{
    s32 slot;
    u16 bits;

    bits = *taken;
    slot = ((s32) (seed & 0xFFFF) % REQUEST_ITEM_SLOTS) & 0xFFFF;
    if (bits & (1 << slot)) {
        slot = ((s32) (slot + 2) % REQUEST_ITEM_SLOTS) & 0xFFFF;
        if (bits & (1 << slot)) {
            slot = ((s32) (slot + 2) % REQUEST_ITEM_SLOTS) & 0xFFFF;
        }
    }
    *taken = bits | (1 << (slot & 0xFFFF));
    return (s16)(slot);
}

s32 Npc_IsPresent(u8 kind, s32 id, u8 force);
/*
 * Returns the gOfferNpcs index of a random townsperson (by seed) of the kind
 * who is present in the village hub, or OFFER_NPC_NONE when none is.
 */
u16 Offer_PickNpc(u16 seed, s8 kind)
{
    OfferNpc *npc = gOfferNpcs;
    s32 i = 0;
    u16 npcId = gOfferNpcs[0].id;
    s32 count = 0;
    u16 candidates[OFFER_SLOTS];

    while (npcId != 0) {
        if (npc->kind == kind && Npc_IsPresent(npcId, AREA_HUB_0x89, 1) == 1) {
            candidates[count] = i;
            count++;
        }
        npc++;
        npcId = npc->id;
        i++;
    }
    if (count == 0) {
        return OFFER_NPC_NONE;
    }
    return candidates[seed % count];
}

/*
 * Returns the first townsperson kind, counting from seed % OFFER_KIND_COUNT,
 * whose bit in gSave.offerKindsUsed is clear; the bits are cleared once all
 * three are set.
 */
s8 Offer_PickKind(u16 seed)
{
    s32 i;
    s32 pick;
    u8 usedKinds;
    s8 kind;

    if (gSave.offerKindsUsed == OFFER_KINDS_ALL) {
        gSave.offerKindsUsed = 0;
    }
    usedKinds = gSave.offerKindsUsed;
    pick = seed;
    for (i = 0; i < OFFER_KIND_COUNT; i++) {
        kind = pick % OFFER_KIND_COUNT;
        if (!(usedKinds & (1 << kind))) {
            return kind;
        }
        pick++;
    }
    return 0;
}

/*
 * Returns how many offers are open: the requests in use plus the quests
 * offered. The quests are read through a byte pointer stepped from gSave
 * with the field's offset added at each load: indexed as the field, the
 * compiler builds the address differently from the game's code.
 */
s32 Offer_CountOpen(void)
{
    OfferRec *req;
    u8 *questBase;
    s32 i;
    s32 j;
    s32 count;

    count = 0;
    req = gSave.offers;
    i = 0;
    do {
        if (req->active != 0) {
            count += 1;
        }
        i += 1;
        req++;
    } while (i < OFFER_SLOTS);
    j = 0;
    questBase = (u8 *)&gSave;
    do {
        if (*(u16 *)(questBase + 10972) != 0) {
            count += 1;
        }
        j += 1;
        questBase += 2;
    } while (j < OFFER_SLOTS);
    return count;
}

/*
 * Clears the request of townsperson npcId (the request is done).
 */
void Offer_ClearRequest(s32 npcId)
{
    s32 npcIdx;

    npcIdx = Offer_FindNpc(npcId);
    if (npcIdx != -1) {
        memset(&gSave.offers[npcIdx], 0, sizeof(OfferRec));
    }
}

extern u32 gOfferQuestPoints[];
/*
 * The quest townsperson npcId offered is cleared: adds its points by the
 * quest's stars (gOfferQuestPoints; a quest without disc info counts as
 * OFFER_QUEST_STARS_MAX stars), clears the quest and the townsperson's offer
 * mark. The quest is reached through a cast pointer to gSave.offerQuests:
 * indexed as the field, the compiler shares the base address differently
 * from the game's code.
 */
void Offer_CompleteQuest(s32 npcId)
{
    s32 npcIdx = Offer_FindNpc(npcId);
    u16 *quest;
    s32 stars;
    s32 points;
    Em *npc;
    QuestInfo *info;

    if (npcIdx != -1) {
        quest = (u16 *)gSave.offerQuests + npcIdx;
        if (*quest != 0) {
            info = Quest_GetDiscInfo(*quest);
            stars = info->stars;
        } else {
            stars = OFFER_QUEST_STARS_MAX;
        }
        if (stars <= OFFER_QUEST_STARS_MAX && stars > 0) {
            points = gOfferQuestPoints[stars - 1];
        } else {
            points = 0;
        }
        Offer_AddPoints(npcId, points);
        *quest = 0;
        npc = Npc_Find(npcId);
        if (npc != NULL) {
            npc->fullCloak = 0;
        }
    }
}

/* A kind's milestone (gOfferMilestonesA for kind 1, B for kind 2). */
typedef struct OfferMilestone {
    u32 need;           /* the points towards it (gSave.offerMilePoints) */
    u32 entry;          /* the offer menu's entry it unlocks (0-4 kind 1, 5-9 kind 2) */
    s32 reached;        /* event flag: set when the points reach need */
    s32 done;           /* event flag: set by the offer menu once taken */
} OfferMilestone;
extern OfferMilestone gOfferMilestonesA[];
extern OfferMilestone gOfferMilestonesB[];
/*
 * Adds points to townsperson npcId, to its kind and to the total. Once
 * condition 10 holds, kinds 1 and 2 also add them towards their next
 * milestone not taken, whose reached flag is set when they reach its need.
 * Then the village's milestones (Village_CheckMilestone) and the
 * townsperson's point flags (Offer_CheckResidentRewards) are checked against the
 * points before; counters past OFFER_POINTS_CAP (OFFER_KIND_POINTS_CAP for
 * the kinds' totals) start again at 0.
 */
void Offer_AddPoints(s32 npcId, s32 points)
{
    u32 kindBefore[4];
    u32 npcBefore[OFFER_SLOTS];
    SaveData *save;
    OfferNpc *npc;
    s32 i;
    OfferMilestone *milestones = NULL;
    s32 j;

    i = Offer_FindNpc(npcId);
    if (i == -1) {
        return;
    }
    memcpy(kindBefore, gSave.offerKindPoints, sizeof(kindBefore));
    memcpy(npcBefore, gSave.offerPoints, sizeof(npcBefore));
    gSave.offerPoints[i] += points;
    npc = &gOfferNpcs[i];
    gSave.offerKindPoints[npc->kind] += points;
    switch (npc->kind) {
    case 0:
        break;
    case 1:
        milestones = gOfferMilestonesA;
        break;
    case 2:
        milestones = gOfferMilestonesB;
        break;
    }
    if (Cond_Check(10) == 1 && milestones != NULL) {
        for (j = 0; j < OFFER_MILESTONES; j++) {
            if (!Save_TestEventFlag(milestones[j].done)) {
                if (!Save_TestEventFlag(milestones[j].reached)) {
                    gSave.offerMilePoints[npc->kind] += points;
                    if (gSave.offerMilePoints[npc->kind] >= milestones[j].need) {
                        Save_SetEventFlag(milestones[j].reached);
                    }
                }
                break;
            }
        }
    }
    gSave.offerKindPoints[OFFER_KIND_TOTAL] += points;
    gSave.offerMilePoints[OFFER_KIND_TOTAL] += points;
    Village_CheckMilestone(kindBefore[2], gSave.offerKindPoints[2], 0);
    Village_CheckMilestone(kindBefore[OFFER_KIND_TOTAL], gSave.offerKindPoints[OFFER_KIND_TOTAL], 1);
    Village_CheckMilestone(kindBefore[OFFER_KIND_TOTAL], gSave.offerKindPoints[OFFER_KIND_TOTAL], 2);
    Village_CheckMilestone(kindBefore[OFFER_KIND_TOTAL], gSave.offerKindPoints[OFFER_KIND_TOTAL], 3);
    Village_CheckMilestone(kindBefore[1], gSave.offerKindPoints[1], 4);
    Village_CheckMilestone(kindBefore[OFFER_KIND_TOTAL], gSave.offerKindPoints[OFFER_KIND_TOTAL], 5);
    Village_CheckMilestone(kindBefore[OFFER_KIND_TOTAL], gSave.offerKindPoints[OFFER_KIND_TOTAL], 6);
    save = &gSave;
    for (j = 0; j < OFFER_SLOTS; j++) {
        Offer_CheckResidentRewards(npcBefore[j], save->offerPoints[j], j);
        if (save->offerPoints[j] > OFFER_POINTS_CAP) {
            save->offerPoints[j] = 0;
        }
    }
    save = &gSave;
    for (j = 0; j < 4; j++) {
        if (save->offerMilePoints[j] > OFFER_POINTS_CAP) {
            save->offerMilePoints[j] = 0;
        }
        if (save->offerKindPoints[j] > OFFER_KIND_POINTS_CAP) {
            save->offerKindPoints[j] = 0;
        }
    }
}

/* The offers' data: the offer townspeople, the day and count rolls, the
 * milestones, the request templates and the quest points. */
OfferNpc gOfferNpcs[15] = {
#include "lobby/007E9410.inc"
};
OfferDayRoll gOfferDayRolls[4] = {
#include "lobby/007E9488.inc"
};
OfferCountRoll gOfferCountRolls3[4] = {
#include "lobby/007E9490.inc"
};
OfferCountRoll gOfferCountRolls4[4] = {
#include "lobby/007E9498.inc"
};
OfferMilestone gOfferMilestonesA[5] = {
#include "lobby/007E94A0.inc"
};
OfferMilestone gOfferMilestonesB[5] = {
#include "lobby/007E94F0.inc"
};
RequestDef gRequestDefs[17] = {
#include "lobby/007E9540.inc"
};
u32 gOfferQuestPoints[8] = {
#include "lobby/007EB2A0.inc"
};
