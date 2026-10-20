#ifndef AWARD_H
#define AWARD_H

/*
 * The awards: hunter titles the game grants for a feat. An award id is an
 * index into gHunterTitles and a bit of SaveData.titles. Award_Grant sets
 * the bit once and posts a notice; Save_HasTitle tests it. The names say
 * what earns each one, from the code that grants it.
 */
#define AWARD_MONEY         0x1E /* the hunter's money reaches 2,000,000 (Save_AddMoney) */
#define AWARD_PLAY_TIME     0x1F /* 300 hours of play (Save_PlayTimeReached(300)) */
#define AWARD_RECIPES       0x20 /* 0xD5 of the 0xED combination recipes known (Recipe_CheckAllKnown) */
#define AWARD_MEALS         0x21 /* the meals record (RECORD_MEALS) reaches 100 (Meal_ApplyEffects) */
#define AWARD_SET_COURSES   0x22 /* 50 set courses eaten in the kitchen (RECORD_SET_COURSES) */
#define AWARD_FRIENDS       0x23 /* the friend list's 50th entry filled */
#define AWARD_GALLERY_COURSE_5 0x26 /* the shooting gallery's course 5 scores GALLERY_AWARD_POINTS (4000) or more (Gallery_Run) */
#define AWARD_RARE_ITEMS    0x27 /* 15 of gObtainFlagItems obtained (Save_SetItemObtained) */
#define AWARD_WEAPONS       0x78 /* at least 80% of the weapons owned (EquipCollect_CheckWeapons) */
#define AWARD_ARMOR         0x79 /* at least half of the armour owned (EquipCollect_CheckArmor) */
#define AWARD_RATE_FLAGS    0x7A /* rate flags 0 to 10 all set (RateFlag_Set) */

/*
 * The rate flags: bits of SaveData.rateFlags, set once each by
 * RateFlag_Set (quietly) or RateFlag_SetAndNotify (with the rate notice).
 * When flags 0 to 10 are all set, AWARD_RATE_FLAGS is granted and
 * RATE_FLAG_ALL set. The names say what sets each one, from the code that
 * sets it.
 */
#define RATE_FLAG_FRIENDS   4    /* the friend list's 50th entry filled, with AWARD_FRIENDS */
#define RATE_FLAG_WEAPONS   5    /* at least 80% of the weapons owned, with AWARD_WEAPONS */
#define RATE_FLAG_ARMOR     6    /* at least half of the armour owned, with AWARD_ARMOR */
#define RATE_FLAG_MEALS     10   /* the meals record reaches 100, with AWARD_MEALS */
#define RATE_FLAG_ALL       11   /* flags 0 to 10 all set, with AWARD_RATE_FLAGS */
#define RATE_FLAGS_FIRST_11 0x7FF /* the mask of flags 0 to 10 */
#define RATE_FLAGS_MASK     0xFFF /* the mask of flags 0 to 11 */

#endif
