/*
 * The village's conditions: the ids Cond_Check tests (the quest desk, the
 * townspeople's talk and offers, the market and the menus ask them).
 *
 * Cond_Check(0) always holds. Ids from COND_RANK_FIRST on are the hunter
 * rank's conditions (Cond_CheckRank): an HR reached, an HR gate cleared
 * (EVENT_FLAG_HR_GATE_*), or both. The ids below it (Cond_CheckEvent) test
 * the save's event flags, and the names wait for the flags' meanings:
 * - 1-27: the village story's steps, in order (Cond_IsLatest asks
 *   whether the story stands at one: id holds and id + 1 doesn't), each
 *   one event flag from 0 to 89 (18 and 23 need the flag before
 *   theirs too);
 * - 31-33: the same as steps 1, 3 and 6; 47-51 and
 *   63-67: the same as steps 1, 2, 3, 7 and 10;
 * - 52-62 and 68-78 (some ids unused): event flags 51-59 and
 *   61-69;
 * - 79-84: other flags, and the two named below.
 * Rank ids 96 (HR 22 and event flag 110), 97 (event flag 133) and
 * 105 (three quests cleared) aren't named yet either.
 */
#ifndef VILLAGE_COND_H
#define VILLAGE_COND_H

enum VillageCond {
    COND_ALWAYS = 0,
    COND_HOUSE_SLEEP_EVENT = 82,  /* EVENT_FLAG_HOUSE_SLEEP_EVENT is set */
    COND_ALL_MONSTERS_HUNTED = 84, /* Monster_AllHunted */
    COND_RANK_FIRST = 89,         /* the first of Cond_CheckRank's ids */
    COND_HR_ANY = 89,             /* HR above 0 */
    COND_HR_10 = 90,              /* HR 10 or more */
    COND_HR_GATE_0 = 91,          /* HR gate 0 cleared */
    COND_HR_GATE_0_HR_11 = 92,    /* HR gate 0 cleared and HR 11 or more */
    COND_HR_16 = 93,
    COND_HR_GATE_1 = 94,
    COND_HR_GATE_1_HR_17 = 95,
    COND_HR_30 = 98,
    COND_HR_GATE_2 = 99,
    COND_HR_GATE_2_HR_31 = 100,
    COND_HR_40 = 101,
    COND_HR_GATE_3 = 102,
    COND_HR_GATE_3_HR_41 = 103,
    COND_HR_50 = 104,
    COND_HR_GATE_4 = 106
};

s32 ElderRaid_IsOn(void);
s32 ElderRaid_IsOnNoMovie(void);
s32 Village_Event186Pending(void);
s32 Town_IsSaleEvent(void);
s32 Cond_CheckEvent(u16 id);
s32 Cond_CheckRank(u16 id);
s32 Cond_Check(u16 id);
s32 Cond_IsLatest(u16 id);

#endif /* VILLAGE_COND_H */
