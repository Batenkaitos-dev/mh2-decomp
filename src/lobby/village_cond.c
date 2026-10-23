/* The town's and the village's conditions: small tests of the lobby's
 * state (the elder raid, the town's sale event, the village's pending
 * story step), then Cond_Check and its two halves, by the save's event
 * flags (Cond_CheckEvent) and by the hunter rank (Cond_CheckRank), with
 * Cond_IsLatest, which finds the story's current step. The condition ids
 * are enum VillageCond (village_cond.h); the townspeople, the quest lists
 * and the menus pass them to decide what is shown and offered. */
#include "common.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "save.h"
#include "quest.h"
#include "village_cond.h"
#include "game_util.h"
#include "village_progress.h"

/* 1 while the elder raid is on with its movie. */
s32 ElderRaid_IsOn(void)
{
    return gLobby->elderRaidState == ELDER_RAID_ON;
}

/* 1 while the elder raid is on without its movie. */
s32 ElderRaid_IsOnNoMovie(void)
{
    return gLobby->elderRaidState == ELDER_RAID_ON_NO_MOVIE;
}

/* 1 offline while event flag 390 is set and 391 isn't: the village's
 * quest settling sets 391 once the player is back offline after 390. */
s32 Village_Event186Pending(void)
{
    if ((Game_IsOnline() == 0) && (Save_TestEventFlag(390) == 1) && (Save_TestEventFlag(391) == 0)) {
        return 1;
    }
    return 0;
}

/* 1 while the town's sale event is on (shops' prices halved online). */
s32 Town_IsSaleEvent(void)
{
    return gLobby->townEvent == TOWN_EVENT_SALE;
}

/* The village's conditions below COND_RANK_FIRST (enum VillageCond): each
 * tests the save's event flags (one flag, or two), another condition, or
 * Monster_AllHunted; 1 when it holds. The cases are in the order the
 * game's jump table gives their blocks. */
s32 Cond_CheckEvent(u16 id)
{
    switch (id) {
    case 27:
        if (Save_TestEventFlag(89) == 1) {
            return 1;
        }
        break;
    case 26:
        if (Save_TestEventFlag(88) == 1) {
            return 1;
        }
        break;
    case 25:
        if (Save_TestEventFlag(87) == 1) {
            return 1;
        }
        break;
    case 24:
        if (Save_TestEventFlag(85) == 1) {
            return 1;
        }
        break;
    case 23:
        if (Save_TestEventFlag(84) == 1 && Save_TestEventFlag(83) == 1) {
            return 1;
        }
        break;
    case 22:
        if (Save_TestEventFlag(83) == 1) {
            return 1;
        }
        break;
    case 21:
        if (Save_TestEventFlag(82) == 1) {
            return 1;
        }
        break;
    case 20:
        if (Save_TestEventFlag(81) == 1) {
            return 1;
        }
        break;
    case 19:
        if (Save_TestEventFlag(80) == 1) {
            return 1;
        }
        break;
    case 18:
        if (Save_TestEventFlag(79) == 1 && Save_TestEventFlag(78) == 1) {
            return 1;
        }
        break;
    case 17:
        if (Save_TestEventFlag(78) == 1) {
            return 1;
        }
        break;
    case 16:
        if (Save_TestEventFlag(77) == 1) {
            return 1;
        }
        break;
    case 15:
        if (Save_TestEventFlag(76) == 1) {
            return 1;
        }
        break;
    case 14:
        if (Save_TestEventFlag(75) == 1) {
            return 1;
        }
        break;
    case 13:
        if (Save_TestEventFlag(72) == 1) {
            return 1;
        }
        break;
    case 12:
        if (Save_TestEventFlag(71) == 1) {
            return 1;
        }
        break;
    case 11:
        if (Save_TestEventFlag(70) == 1) {
            return 1;
        }
        break;
    case 10:
        if (Save_TestEventFlag(18) == 1) {
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(17) == 1) {
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(12) == 1) {
            return 1;
        }
        break;
    case 7:
        if (Save_TestEventFlag(11) == 1) {
            return 1;
        }
        break;
    case 6:
        if (Save_TestEventFlag(9) == 1) {
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(8) == 1) {
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(7) == 1) {
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(6) == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(5) == 1) {
            return 1;
        }
        break;
    case 1:
        if (Save_TestEventFlag(0) == 1) {
            return 1;
        }
        break;
    case 62:
        if (Save_TestEventFlag(59)) {
            return 1;
        }
        break;
    case 60:
        if (Save_TestEventFlag(58)) {
            return 1;
        }
        break;
    case 59:
        if (Save_TestEventFlag(57)) {
            return 1;
        }
        break;
    case 58:
        if (Save_TestEventFlag(56)) {
            return 1;
        }
        break;
    case 57:
        if (Save_TestEventFlag(55)) {
            return 1;
        }
        break;
    case 56:
        if (Save_TestEventFlag(54)) {
            return 1;
        }
        break;
    case 55:
        if (Save_TestEventFlag(53)) {
            return 1;
        }
        break;
    case 53:
        if (Save_TestEventFlag(52)) {
            return 1;
        }
        break;
    case 52:
        if (Save_TestEventFlag(51)) {
            return 1;
        }
        break;
    case 51:
        if (Cond_Check(10)) {
            return 1;
        }
        break;
    case 50:
        if (Cond_Check(7)) {
            return 1;
        }
        break;
    case 49:
        if (Cond_Check(3)) {
            return 1;
        }
        break;
    case 48:
        if (Cond_Check(2)) {
            return 1;
        }
        break;
    case 47:
        if (Cond_Check(1)) {
            return 1;
        }
        break;
    case 33:
        if (Cond_Check(6)) {
            return 1;
        }
        break;
    case 32:
        if (Cond_Check(3)) {
            return 1;
        }
        break;
    case 31:
        if (Cond_Check(1)) {
            return 1;
        }
        break;
    case 78:
        if (Save_TestEventFlag(69)) {
            return 1;
        }
        break;
    case 76:
        if (Save_TestEventFlag(68)) {
            return 1;
        }
        break;
    case 75:
        if (Save_TestEventFlag(67)) {
            return 1;
        }
        break;
    case 74:
        if (Save_TestEventFlag(66)) {
            return 1;
        }
        break;
    case 73:
        if (Save_TestEventFlag(65)) {
            return 1;
        }
        break;
    case 72:
        if (Save_TestEventFlag(64)) {
            return 1;
        }
        break;
    case 71:
        if (Save_TestEventFlag(63)) {
            return 1;
        }
        break;
    case 69:
        if (Save_TestEventFlag(62)) {
            return 1;
        }
        break;
    case 68:
        if (Save_TestEventFlag(61)) {
            return 1;
        }
        break;
    case 67:
        if (Cond_Check(10)) {
            return 1;
        }
        break;
    case 66:
        if (Cond_Check(7)) {
            return 1;
        }
        break;
    case 65:
        if (Cond_Check(3)) {
            return 1;
        }
        break;
    case 64:
        if (Cond_Check(2)) {
            return 1;
        }
        break;
    case 63:
        if (Cond_Check(1)) {
            return 1;
        }
        break;
    case COND_ALL_MONSTERS_HUNTED:
        if (Monster_AllHunted() == 1) {
            return 1;
        }
        break;
    case 83:
        if (Save_TestEventFlag(314) == 1) {
            return 1;
        }
        break;
    case COND_HOUSE_SLEEP_EVENT:
        if (Save_TestEventFlag(EVENT_FLAG_HOUSE_SLEEP_EVENT) == 1) {
            return 1;
        }
        break;
    case 81:
        if (Save_TestEventFlag(312) == 1) {
            return 1;
        }
        break;
    case 80:
        if (Save_TestEventFlag(313) == 1) {
            return 1;
        }
        break;
    case 79:
        if (Save_TestEventFlag(310) == 1) {
            return 1;
        }
        break;
    case COND_ALWAYS:
        return 1;
    }
    return 0;
}


/* The hunter rank's conditions (enum VillageCond, from COND_RANK_FIRST):
 * an HR reached, an HR gate cleared, or both; 1 when it holds. */
s32 Cond_CheckRank(u16 id)
{
    switch (id) {
    case COND_HR_GATE_4:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_4) == 1) {
            return 1;
        }
        break;
    case 105:
        if (Save_IsQuestCleared(15022) && Save_IsQuestCleared(15023) && Save_IsQuestCleared(15024)) {
            return 1;
        }
        break;
    case COND_HR_50:
        if (gSave.hr >= 50) {
            return 1;
        }
        break;
    case COND_HR_GATE_3_HR_41:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_3) == 1 && gSave.hr >= 41) {
            return 1;
        }
        break;
    case COND_HR_GATE_3:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_3) == 1) {
            return 1;
        }
        break;
    case COND_HR_40:
        if (gSave.hr >= 40) {
            return 1;
        }
        break;
    case COND_HR_GATE_2_HR_31:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_2) == 1 && gSave.hr >= 31) {
            return 1;
        }
        break;
    case COND_HR_GATE_2:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_2) == 1) {
            return 1;
        }
        break;
    case COND_HR_30:
        if (gSave.hr >= 30) {
            return 1;
        }
        break;
    case 97:
        if (Save_TestEventFlag(133) == 1) {
            return 1;
        }
        break;
    case 96:
        if (gSave.hr >= 22 && Save_TestEventFlag(110) == 1) {
            return 1;
        }
        break;
    case COND_HR_GATE_1_HR_17:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_1) == 1 && gSave.hr >= 17) {
            return 1;
        }
        break;
    case COND_HR_GATE_1:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_1) == 1) {
            return 1;
        }
        break;
    case COND_HR_16:
        if (gSave.hr >= 16) {
            return 1;
        }
        break;
    case COND_HR_GATE_0_HR_11:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_0) == 1 && gSave.hr >= 11) {
            return 1;
        }
        break;
    case COND_HR_GATE_0:
        if (Save_TestEventFlag(EVENT_FLAG_HR_GATE_0) == 1) {
            return 1;
        }
        break;
    case COND_HR_10:
        if (gSave.hr >= 10) {
            return 1;
        }
        break;
    case COND_HR_ANY:
        if (gSave.hr > 0) {
            return 1;
        }
        break;
    case COND_ALWAYS:
        return 1;
    }
    return 0;
}

/* 1 when the village's condition id (enum VillageCond) holds: COND_ALWAYS
 * always, the rank's conditions by Cond_CheckRank, the others by the event
 * flags (Cond_CheckEvent). */
s32 Cond_Check(u16 id)
{
    if (id == COND_ALWAYS) {
        return 1;
    }
    if (id >= COND_RANK_FIRST) {
        return Cond_CheckRank(id);
    }
    return Cond_CheckEvent(id);
}

/* 1 when village condition `id` holds and condition id + 1 does not: the
 * story stands at that condition's step (Cond_Check). 0 for id 0. */
s32 Cond_IsLatest(u16 id)
{
    if (id != COND_ALWAYS) {
        if (Cond_Check(id) == 1 && Cond_Check(id + 1) == 0) {
            return 1;
        }
    } else {
        return 0;
    }
    return 0;
}
