/*
 * The village's progress and the MH Portable link.
 * - The story: Village_UpdateProgress sets the story's event flags from the
 *   hunter rank gates passed, the quests cleared and the monsters hunted
 *   (Monster_IsHunted over the gMonsterFlagBits list), raises the village
 *   facilities' levels, grants awards, and when Game_IsOnline is 0 completes
 *   and renews the quest offers.
 * - The MH Portable link: PspLink_GrantBonuses sets each link bonus's
 *   event flag once (EVENT_FLAG_PSP_LINK_*) and gives its rewards; PspGift_Pick
 *   and PspGift_Give pick and give the gift earned by the number of links.
 * The quests' lookups (Quest_GetInfo and the rest) follow this file.
 * The .data holds bonus 0's items (gPspLinkBonusItems) and the link gifts
 * (gPspLinkGifts).
 */
#include "common.h"
#include "lobby_state.h"

#include "lobby_menu.h"
#include "lobby_globals.h"
#include "pad.h"
#include "quest.h"
#include "syssettings.h"
#include "save.h"
#include "player.h"
#include "syssave.h"
#include "em.h"
#include "award.h"
#include "chat_log.h"
#include "lobby_labels.h"
#include "area_draw.h"
#include "kbd.h"
#include "text.h"
#include "village_cond.h"
#include "window.h"
#include "village_progress.h"
#include "game_util.h"
#include "lobby_world.h"
#include "market.h"
#include "menu_steps.h"
#include "village_offers.h"
#include "village_records.h"
#include "libc.h"
/* A link gift: the item and how many (gPspLinkGifts) */
typedef struct PspGift {
    u16 item;
    u16 count;
} PspGift;
extern MonsterBit gMonsterFlagBits[];
extern PspGift gPspLinkGifts[];
u8 SavePouch_Add(u16 item, s16 n);
u8 Village_FacilityLevel(s32 k);
extern u16 gPspLinkBonusItems[];
s32 Award_Grant(s32 n);
extern QuestRecord gQuestRecord; /* the last quest's record */

/*
 * The gift (gPspLinkGifts index) the MH Portable links earn now, by their count
 * (PspLink_Count): 1, 2, 4, 6 and 8 links give gifts 0 to 4, then every third
 * link after 8 gives 5, 6, 7, 8, 9, then 4 again, and so on. -1 for no gift:
 * another count, link bonus 3 not reached (EVENT_FLAG_PSP_LINK_3), or this link's
 * gift already given (EVENT_FLAG_PSP_GIFT_GIVEN, cleared at each link).
 */
s32 PspGift_Pick(void)
{
    s32 linkCount = PspLink_Count();

    if (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_3) == 0) {
        return -1;
    }
    if (Save_TestEventFlag(EVENT_FLAG_PSP_GIFT_GIVEN) == 1) {
        return -1;
    }
    switch (linkCount) {
    case 1:
        return 0;
    case 2:
        return 1;
    case 4:
        return 2;
    case 6:
        return 3;
    case 8:
        return 4;
    default:
        if (linkCount > 8 && (linkCount - 8) % 3 == 0) {
            return (linkCount - 8) % 18 / 3 + 4;
        }
        return -1;
    }
}

/* Gives `gift` of gPspLinkGifts (PspGift_Pick's; -1 and 10 up give nothing): its
 * items go into the pouch with the received notice, and EVENT_FLAG_PSP_GIFT_GIVEN marks
 * the link's gift given. With no room in the pouch, only the pouch-full
 * notice shows and the gift stays. */
void PspGift_Give(s32 gift)
{
    u16 item;
    u16 giftCount;

    if (gift != -1 && gift < 10) {
        item = gPspLinkGifts[gift].item;
        giftCount = gPspLinkGifts[gift].count;
        if (SavePouch_Space(item) < giftCount) {
            Menu_PushNotice(21);
        } else {
            SavePouch_Add(item, giftCount);
            Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, item);
            Save_SetEventFlag(EVENT_FLAG_PSP_GIFT_GIVEN);
        }
    }
}

/*
 * Gives the MH Portable link's bonuses that the link data has reached
 * (PspLink_HasFlag): each bonus's event flag is set once. Bonus 0 also gives
 * one of each item of gPspLinkBonusItems; bonus 2 sets rate flag 3 unless set;
 * bonus 4 unlocks the system file's bonus (SysSave_UnlockBonus), and its flag
 * is also set once gSysSave.bonusUnlocked is; bonus 5 sets event flag 0x74
 * too; bonus 6, every call, starts village timer 2 with one quest left and
 * clears event flag 337. Bonus 8 needs village condition 10, bonus 9 a
 * Lunastra hunted.
 */
void PspLink_GrantBonuses(void)
{
    u16 *cur;
    u16 item;

    if ((PspLink_HasFlag(0) != 0) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_0) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_PSP_LINK_0);
        item = gPspLinkBonusItems[0];
        cur = gPspLinkBonusItems;
        if (item != 0) {
            do {
                SavePouch_Add(item, 1);
                cur += 1;
                item = *cur;
            } while (item != 0);
        }
    }
    if ((PspLink_HasFlag(10) != 0) && (Save_TestEventFlag(73) == 0)) {
        Save_SetEventFlag(73);
    }
    if ((PspLink_HasFlag(1) != 0) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_1) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_PSP_LINK_1);
    }
    if ((PspLink_HasFlag(2) != 0) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_2) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_PSP_LINK_2);
        if (!(gSave.rateFlags & 8)) {
            RateFlag_SetAndNotify(3U);
        }
    }
    if ((PspLink_HasFlag(3) != 0) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_3) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_PSP_LINK_3);
    }
    if ((PspLink_HasFlag(4) != 0) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_4) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_PSP_LINK_4);
        SysSave_UnlockBonus();
    }
    if ((gSysSave.bonusUnlocked != 0) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_4) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_PSP_LINK_4);
    }
    if ((PspLink_HasFlag(5) != 0) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_5) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_PSP_LINK_5);
        Save_SetEventFlag(116);
    }
    if (PspLink_HasFlag(6) != 0) {
        if (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_6) == 0) {
            Save_SetEventFlag(EVENT_FLAG_PSP_LINK_6);
        }
        gSave.villageTimers[2].state = 1;
        gSave.villageTimers[2].questsLeft = 1;
        Save_ClearEventFlag(337);
    }
    if ((PspLink_HasFlag(7) != 0) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_7) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_PSP_LINK_7);
    }
    if ((PspLink_HasFlag(8) != 0) && (Cond_Check(10) == 1) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_8) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_PSP_LINK_8);
    }
    if ((PspLink_HasFlag(9) != 0) && (Monster_IsHunted(EM_KIND_LUNASTRA) == 1) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_9) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_PSP_LINK_9);
    }
}

/* 1 when every monster of gMonsterFlagBits has been hunted, leaving out
 * Teostra, Rathalos, Monoblos, Chameleos and Yian Garuga. */
s32 Monster_AllHuntedBut5(void)
{
    MonsterBit *entry = gMonsterFlagBits;
    u16 em = gMonsterFlagBits[0].em;

    while (em != 0) {
        if (Monster_IsHunted(em) == 0) {
            switch (entry->em) {
            case EM_KIND_TEOSTRA:
            case EM_KIND_RATHALOS:
            case EM_KIND_MONOBLOS:
            case EM_KIND_CHAMELEOS:
            case EM_KIND_YIAN_GARUGA:
                break;
            default:
                return 0;
            }
        }
        entry++;
        em = entry->em;
    }
    return 1;
}

/* 1 when every monster of gMonsterFlagBits has been hunted, leaving out
 * Chameleos and Yian Garuga. */
s32 Monster_AllHunted(void)
{
    MonsterBit *entry = gMonsterFlagBits;
    u16 em = gMonsterFlagBits[0].em;

    while (em != 0) {
        if (Monster_IsHunted(em) == 0) {
            switch (entry->em) {
            case EM_KIND_CHAMELEOS:
            case EM_KIND_YIAN_GARUGA:
                break;
            default:
                return 0;
            }
        }
        entry++;
        em = entry->em;
    }
    return 1;
}

/*
 * Brings the village's story up to date. It sets the event flags of the
 * hunter rank gates passed (Save_HrGateCleared 0-4), of story quests cleared
 * and monsters hunted (some only at a village condition), and of the two
 * monster lists (Monster_AllHuntedBut5; Monster_AllHunted's flag 0x1C9 is
 * tested a second time instead of set, as in the game's code, so it is never
 * set here). Unless `afterOffer` (1 when an offer was just completed, 0 at the
 * day's reset): the late story's flags 388-391 (with award 40 and rate
 * flag 8), the play-time award, one step of the facilities' levels, and the
 * link bonuses (PspLink_GrantBonuses). When Game_IsOnline is 0 and
 * gSysSettings.offline is set: after a quest that was cleared or failed, the
 * offer taken is completed and the offers renewed on a new season or day, then
 * the village timers and milestones run; gQuestOfferId is cleared.
 */
void Village_UpdateProgress(s32 afterOffer)
{
    s32 day;
    u16 id;
    u8 season;
    u8 phase;

    day = LobbyWorld_GetSeasonDay();
    if ((Save_TestEventFlag(EVENT_FLAG_HR_GATE_0) == 0) && (Save_HrGateCleared(0) == 1)) {
        Save_SetEventFlag(EVENT_FLAG_HR_GATE_0);
    }
    if ((Save_TestEventFlag(EVENT_FLAG_HR_GATE_1) == 0) && (Save_HrGateCleared(1) == 1)) {
        Save_SetEventFlag(EVENT_FLAG_HR_GATE_1);
    }
    if ((Save_TestEventFlag(EVENT_FLAG_HR_GATE_2) == 0) && (Save_HrGateCleared(2) == 1)) {
        Save_SetEventFlag(EVENT_FLAG_HR_GATE_2);
    }
    if ((Save_TestEventFlag(EVENT_FLAG_HR_GATE_3) == 0) && (Save_HrGateCleared(3) == 1)) {
        Save_SetEventFlag(EVENT_FLAG_HR_GATE_3);
    }
    if ((Save_TestEventFlag(EVENT_FLAG_HR_GATE_4) == 0) && (Save_HrGateCleared(4) == 1)) {
        Save_SetEventFlag(EVENT_FLAG_HR_GATE_4);
    }
    if ((Save_IsQuestCleared(10101U) != 0) && (Save_IsQuestCleared(10102U) != 0) && (Save_IsQuestCleared(10103U) != 0) && (Save_TestEventFlag(EVENT_FLAG_ARENA_CUP_OPEN) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_ARENA_CUP_OPEN);
    }
    if ((Save_TestEventFlag(5) == 0) && ((Save_IsQuestCleared(1001U) != 0) || (Save_IsQuestCleared(1086U) != 0) || (Save_IsQuestCleared(1087U) != 0))) {
        Save_SetEventFlag(5);
    }
    if ((Monster_IsHunted(EM_KIND_VELOCIDROME) == 1) && (Save_TestEventFlag(7) == 0) && (Cond_Check(3) == 1)) {
        Save_SetEventFlag(7);
    }
    if ((Monster_IsHunted(EM_KIND_BULLDROME) == 1) && (Save_TestEventFlag(9) == 0) && (Cond_Check(5) == 1)) {
        Save_SetEventFlag(9);
    }
    if ((Monster_IsHunted(EM_KIND_CEPHADROME) == 1) && (Save_TestEventFlag(17) == 0) && (Cond_Check(8) == 1)) {
        Save_SetEventFlag(17);
    }
    if ((Monster_IsHunted(EM_KIND_GYPCEROS) == 1) && (Save_TestEventFlag(70) == 0) && (Cond_Check(10) == 1)) {
        Save_SetEventFlag(70);
    }
    if ((Monster_IsHunted(EM_KIND_CONGALALA) == 1) && (Save_TestEventFlag(71) == 0) && (Cond_Check(11) == 1)) {
        Save_SetEventFlag(71);
    }
    if ((Save_IsQuestCleared(5002U) != 0) && (Save_TestEventFlag(75) == 0)) {
        Save_SetEventFlag(75);
    }
    if ((Save_IsQuestCleared(5003U) != 0) && (Save_TestEventFlag(76) == 0)) {
        Save_SetEventFlag(76);
    }
    if ((Monster_IsHunted(EM_KIND_BLANGONGA) == 1) && (Save_TestEventFlag(78) == 0)) {
        Save_SetEventFlag(78);
    }
    if ((Monster_IsHunted(EM_KIND_RATHIAN) == 1) && (Save_TestEventFlag(79) == 0)) {
        Save_SetEventFlag(79);
    }
    if ((Save_IsQuestCleared(5004U) != 0) && (Save_TestEventFlag(82) == 0)) {
        Save_SetEventFlag(82);
    }
    if ((Monster_IsHunted(EM_KIND_SHOGUN_CEANATAUR) == 1) && (Save_TestEventFlag(83) == 0)) {
        Save_SetEventFlag(83);
    }
    if ((Monster_IsHunted(EM_KIND_GRAVIOS) == 1) && (Save_TestEventFlag(84) == 0)) {
        Save_SetEventFlag(84);
    }
    if ((Save_IsQuestCleared(5007U) != 0) && (Save_TestEventFlag(88) == 0)) {
        Save_SetEventFlag(88);
        if (gQuestOfferId == 1) {
            gQuestOfferId = 0U;
            memset(&gSave.offerQuests[0], 0, sizeof(gSave.offerQuests[0]));
        }
    }
    if ((Save_TestEventFlag(319) == 0) && ((Village_TotalLevel() & 0xFF) >= 2)) {
        Save_SetEventFlag(319);
    }
    if ((Monster_IsHunted(EM_KIND_RATHALOS) == 1) && (Save_TestEventFlag(313) == 0)) {
        Save_SetEventFlag(313);
    }
    if ((Save_IsQuestCleared(5008U) != 0) && (Save_TestEventFlag(314) == 0)) {
        Save_SetEventFlag(314);
    }
    if ((Save_TestEventFlag(89) == 0) && (Monster_AllHuntedBut5() == 1)) {
        Save_SetEventFlag(89);
    }
    if ((Save_TestEventFlag(457) == 0) && (Monster_AllHunted() == 1)) {
        Save_TestEventFlag(457);
    }
    if (!(afterOffer & 0xFF)) {
        if ((gSysSettings.offline != 0) && (Save_TestEventFlag(390) == 1) && (Save_TestEventFlag(391) == 0)) {
            Save_SetEventFlag(391);
        }
        if (Save_TestEventFlag(388) == 0) {
            if ((Cond_Check(27) == 1) && ((Village_FacilityLevel(1) & 0xFF) == 2) && ((Village_FacilityLevel(2) & 0xFF) == 2) && ((Village_FacilityLevel(3) & 0xFF) == 2) && (gSave.awayFromVillage == 0) && (Game_IsOnline() == 0)) {
                Save_SetEventFlag(388);
            }
        } else if ((Save_TestEventFlag(390) == 0) && (Save_TestEventFlag(389) == 1)) {
            Save_SetEventFlag(390);
            if (!(gSave.rateFlags & 0x100)) {
                RateFlag_SetAndNotify(8U);
            }
            Award_Grant(40);
        }
        if (Save_PlayTimeReached(300 /* hours */) == 1) {
            Award_Grant(AWARD_PLAY_TIME);
        }
        if ((Save_IsQuestCleared(5001U) != 0) && (Save_TestEventFlag(18) == 0) && (Cond_Check(9) == 1)) {
            Save_SetEventFlag(18);
            Save_SetEventFlag(50);
            Save_SetEventFlag(60);
            Village_SetFacilityLevel(1, 1);
            if (Save_TestEventFlag(73) == 0) {
                Save_SetEventFlag(73);
            }
        } else if ((Cond_Check(24) == 1) && ((Village_FacilityLevel(1) & 0xFF) < 2)) {
            Village_SetFacilityLevel(1, 2);
        } else if ((Save_TestEventFlag(53) == 1) && ((Village_FacilityLevel(2) & 0xFF) <= 0)) {
            Village_SetFacilityLevel(2, 1);
        } else if ((Save_TestEventFlag(59) == 1) && ((Village_FacilityLevel(2) & 0xFF) < 2)) {
            Village_SetFacilityLevel(2, 2);
        } else if ((Save_TestEventFlag(63) == 1) && ((Village_FacilityLevel(3) & 0xFF) <= 0)) {
            Village_SetFacilityLevel(3, 1);
        } else if ((Save_TestEventFlag(69) == 1) && ((Village_FacilityLevel(3) & 0xFF) < 2)) {
            Village_SetFacilityLevel(3, 2);
        }
        PspLink_GrantBonuses();
    }
    if (Game_IsOnline() == 0) {
        if (gSysSettings.offline != 0) {
            phase = gQuestRecord.phase;
            if ((phase == QUEST_PHASE_FAILED) || (phase == QUEST_PHASE_CLEARED)) {
                id = gQuestOfferId;
                if ((id != 0) && (phase == QUEST_PHASE_CLEARED)) {
                    Offer_CompleteQuest((s32) id);
                }
                season = gLobbyLastSeason;
                if (season != LobbyWorld_GetSeason()) {
                    Offer_ClearAll();
                    Offer_Generate();
                } else if (gQuestAcceptDay != day) {
                    Offer_Generate();
                }
            }
            Village_QuestTick();
            Village_ProcessEventFlags();
        }
        gQuestOfferId = 0U;
    }
}

/* The file's .data: link bonus 0's items (0 ends) and the gifts by link count. */
u16 gPspLinkBonusItems[8] = {
#include "lobby/007F1990.inc"
};
PspGift gPspLinkGifts[12] = {
#include "lobby/007F19A0.inc"
};
