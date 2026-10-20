/*
 * The village's conditions: the ids Cond_Check tests (the quest desk, the
 * townspeople's talk and offers, the market and the menus ask them).
 *
 * Cond_Check(0) always holds. Ids from COND_RANK_FIRST on are the hunter
 * rank's conditions (Cond_CheckRank): an HR reached, an HR gate cleared
 * (EVENT_FLAG_HR_GATE_*), or both. The ids below it (Cond_CheckEvent) test
 * the save's event flags, and the names wait for the flags' meanings:
 * - 0x01-0x1B: the village story's steps, in order (Cond_IsLatest asks
 *   whether the story stands at one: id holds and id + 1 doesn't), each
 *   one event flag from 0x00 to 0x59 (0x12 and 0x17 need the flag before
 *   theirs too);
 * - 0x1F-0x21: the same as steps 0x01, 0x03 and 0x06; 0x2F-0x33 and
 *   0x3F-0x43: the same as steps 0x01, 0x02, 0x03, 0x07 and 0x0A;
 * - 0x34-0x3E and 0x44-0x4E (some ids unused): event flags 0x33-0x3B and
 *   0x3D-0x45;
 * - 0x4F-0x54: other flags, and the two named below.
 * Rank ids 0x60 (HR 22 and event flag 0x6E), 0x61 (event flag 0x85) and
 * 0x69 (three quests cleared) aren't named yet either.
 */
#ifndef VILLAGE_COND_H
#define VILLAGE_COND_H

enum VillageCond {
    COND_ALWAYS = 0x00,
    COND_HOUSE_SLEEP_EVENT = 0x52,  /* EVENT_FLAG_HOUSE_SLEEP_EVENT is set */
    COND_ALL_MONSTERS_HUNTED = 0x54, /* Monster_AllHunted */
    COND_RANK_FIRST = 0x59,         /* the first of Cond_CheckRank's ids */
    COND_HR_ANY = 0x59,             /* HR above 0 */
    COND_HR_10 = 0x5A,              /* HR 10 or more */
    COND_HR_GATE_0 = 0x5B,          /* HR gate 0 cleared */
    COND_HR_GATE_0_HR_11 = 0x5C,    /* HR gate 0 cleared and HR 11 or more */
    COND_HR_16 = 0x5D,
    COND_HR_GATE_1 = 0x5E,
    COND_HR_GATE_1_HR_17 = 0x5F,
    COND_HR_30 = 0x62,
    COND_HR_GATE_2 = 0x63,
    COND_HR_GATE_2_HR_31 = 0x64,
    COND_HR_40 = 0x65,
    COND_HR_GATE_3 = 0x66,
    COND_HR_GATE_3_HR_41 = 0x67,
    COND_HR_50 = 0x68,
    COND_HR_GATE_4 = 0x6A
};

#endif /* VILLAGE_COND_H */
