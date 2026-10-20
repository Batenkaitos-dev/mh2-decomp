/*
 * The village's progress and the quest desk's quest lookups.
 * - The story: Village_UpdateProgress sets the story's event flags from the
 *   hunter rank gates passed, the quests cleared and the monsters hunted
 *   (Monster_IsHunted over the gMonsterFlagBits list), raises the village
 *   facilities' levels, grants awards, and when Game_IsOnline is 0 completes
 *   and renews the quest offers.
 * - The MH Portable link: PspLink_GrantBonuses sets each link bonus's
 *   event flag once (EVENT_FLAG_PSP_LINK_*) and gives its rewards; PspGift_Pick
 *   and PspGift_Give pick and give the gift earned by the number of links.
 * - The quests: a quest's QuestInfo from the disc's lists by id group
 *   (gGuildQuestLists) or from the three downloaded quest files (gQuestFiles,
 *   ids from QUEST_ID_DOWNLOADED), and its texts, stars and list flags.
 * - The quest room: Quest_PostStep posts the selected quest to the server
 *   step by step, LobbyMenu_KbdEntry types a text with the software keyboard
 *   (the room's password), Money_PayPending settles gRewardMoney.
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
#include "em.h"
#include "award.h"
#include "lobby_labels.h"
void *memset(void *, s32, u32);
s32 PspLink_Count(void);
/* A link gift: the item and how many (gPspLinkGifts) */
typedef struct PspGift {
    u16 item;
    u16 count;
} PspGift;
extern u32 gStageFile;
extern MonsterBit gMonsterFlagBits[];
s32 Monster_IsHunted(u16 em);
s32 Cond_Check(u16 id);
QuestInfo *Quest_GetInfo(u16 id);
s32 Quest_IsSpecialId(u16 id);
QuestInfo *Quest_FindDownloaded(u16 id);
QuestInfo *Quest_GetDiscInfo(u16 id);
extern u16 gLobbyQuestId;
char *Quest_GetText(u16 id, s32 k);
extern QuestFile gQuestFiles[3];
s8 Quest_DownloadedSlot(u16 id);
QuestInfo *Quest_GetInfoAny(u16 id);
extern u16 gQuestGroupLastIds[];
extern QuestInfo **gGuildQuestLists[];
void Quest_SetCurrent(u16 id, void *p, s32 c);
extern PspGift gPspLinkGifts[];
s32 SavePouch_Space(u16 item);
u8 SavePouch_Add(u16 item, s16 n);
u8 Notice_Push(u8, s16, s16);
void Save_SetEventFlag(s32 flag);
void Menu_PushNotice(s32 msg);
extern u8 gMenuWork[];
void Save_AddMoney(s32 n);
void LobbySe_Play(s32 se);
u8 Village_FacilityLevel(s32 k);
s32 Save_TestEventFlag(s32 flag);
s32 Save_IsQuestCleared(u16 id);
void Save_ClearEventFlag(s32 flag);
s32 RateFlag_SetAndNotify(s32 a);
s32 PspLink_HasFlag(s32);
void SysSave_UnlockBonus(void);
extern u16 gPspLinkBonusItems[];
void Village_ProcessEventFlags(void);
void Village_QuestTick(void);
void Offer_Generate(void);
void Offer_ClearAll(void);
void Offer_CompleteQuest(s32);
void PspLink_GrantBonuses(void);
s32 LobbyWorld_GetSeason(void);
s32 LobbyWorld_GetSeasonDay(void);
s32 Save_HrGateCleared(s32 n);
s32 Award_Grant(s32 n);
s32 Game_IsOnline(void);
u8 Village_TotalLevel(void);
void Village_SetFacilityLevel(s32 k, u8 v);
s32 Save_PlayTimeReached(s16 n);
extern QuestRecord gQuestRecord; /* the last quest's record */
u16 Pad_GetPressedRepeat(s32 pad);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
void Kbd_Cancel(void);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);

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
            Menu_PushNotice(0x15);
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
 * clears event flag 0x151. Bonus 8 needs village condition 0xA, bonus 9 a
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
    if ((PspLink_HasFlag(0xA) != 0) && (Save_TestEventFlag(0x49) == 0)) {
        Save_SetEventFlag(0x49);
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
        Save_SetEventFlag(0x74);
    }
    if (PspLink_HasFlag(6) != 0) {
        if (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_6) == 0) {
            Save_SetEventFlag(EVENT_FLAG_PSP_LINK_6);
        }
        gSave.villageTimers[2].state = 1;
        gSave.villageTimers[2].questsLeft = 1;
        Save_ClearEventFlag(0x151);
    }
    if ((PspLink_HasFlag(7) != 0) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_7) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_PSP_LINK_7);
    }
    if ((PspLink_HasFlag(8) != 0) && (Cond_Check(0xA) == 1) && (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_8) == 0)) {
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
 * day's reset): the late story's flags 0x184-0x187 (with award 0x28 and rate
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
    if ((Save_IsQuestCleared(0x2775U) != 0) && (Save_IsQuestCleared(0x2776U) != 0) && (Save_IsQuestCleared(0x2777U) != 0) && (Save_TestEventFlag(EVENT_FLAG_ARENA_CUP_OPEN) == 0)) {
        Save_SetEventFlag(EVENT_FLAG_ARENA_CUP_OPEN);
    }
    if ((Save_TestEventFlag(5) == 0) && ((Save_IsQuestCleared(0x3E9U) != 0) || (Save_IsQuestCleared(0x43EU) != 0) || (Save_IsQuestCleared(0x43FU) != 0))) {
        Save_SetEventFlag(5);
    }
    if ((Monster_IsHunted(EM_KIND_VELOCIDROME) == 1) && (Save_TestEventFlag(7) == 0) && (Cond_Check(3) == 1)) {
        Save_SetEventFlag(7);
    }
    if ((Monster_IsHunted(EM_KIND_BULLDROME) == 1) && (Save_TestEventFlag(9) == 0) && (Cond_Check(5) == 1)) {
        Save_SetEventFlag(9);
    }
    if ((Monster_IsHunted(EM_KIND_CEPHADROME) == 1) && (Save_TestEventFlag(0x11) == 0) && (Cond_Check(8) == 1)) {
        Save_SetEventFlag(0x11);
    }
    if ((Monster_IsHunted(EM_KIND_GYPCEROS) == 1) && (Save_TestEventFlag(0x46) == 0) && (Cond_Check(0xA) == 1)) {
        Save_SetEventFlag(0x46);
    }
    if ((Monster_IsHunted(EM_KIND_CONGALALA) == 1) && (Save_TestEventFlag(0x47) == 0) && (Cond_Check(0xB) == 1)) {
        Save_SetEventFlag(0x47);
    }
    if ((Save_IsQuestCleared(0x138AU) != 0) && (Save_TestEventFlag(0x4B) == 0)) {
        Save_SetEventFlag(0x4B);
    }
    if ((Save_IsQuestCleared(0x138BU) != 0) && (Save_TestEventFlag(0x4C) == 0)) {
        Save_SetEventFlag(0x4C);
    }
    if ((Monster_IsHunted(EM_KIND_BLANGONGA) == 1) && (Save_TestEventFlag(0x4E) == 0)) {
        Save_SetEventFlag(0x4E);
    }
    if ((Monster_IsHunted(EM_KIND_RATHIAN) == 1) && (Save_TestEventFlag(0x4F) == 0)) {
        Save_SetEventFlag(0x4F);
    }
    if ((Save_IsQuestCleared(0x138CU) != 0) && (Save_TestEventFlag(0x52) == 0)) {
        Save_SetEventFlag(0x52);
    }
    if ((Monster_IsHunted(EM_KIND_SHOGUN_CEANATAUR) == 1) && (Save_TestEventFlag(0x53) == 0)) {
        Save_SetEventFlag(0x53);
    }
    if ((Monster_IsHunted(EM_KIND_GRAVIOS) == 1) && (Save_TestEventFlag(0x54) == 0)) {
        Save_SetEventFlag(0x54);
    }
    if ((Save_IsQuestCleared(0x138FU) != 0) && (Save_TestEventFlag(0x58) == 0)) {
        Save_SetEventFlag(0x58);
        if (gQuestOfferId == 1) {
            gQuestOfferId = 0U;
            memset(&gSave.offerQuests[0], 0, sizeof(gSave.offerQuests[0]));
        }
    }
    if ((Save_TestEventFlag(0x13F) == 0) && ((Village_TotalLevel() & 0xFF) >= 2)) {
        Save_SetEventFlag(0x13F);
    }
    if ((Monster_IsHunted(EM_KIND_RATHALOS) == 1) && (Save_TestEventFlag(0x139) == 0)) {
        Save_SetEventFlag(0x139);
    }
    if ((Save_IsQuestCleared(0x1390U) != 0) && (Save_TestEventFlag(0x13A) == 0)) {
        Save_SetEventFlag(0x13A);
    }
    if ((Save_TestEventFlag(0x59) == 0) && (Monster_AllHuntedBut5() == 1)) {
        Save_SetEventFlag(0x59);
    }
    if ((Save_TestEventFlag(0x1C9) == 0) && (Monster_AllHunted() == 1)) {
        Save_TestEventFlag(0x1C9);
    }
    if (!(afterOffer & 0xFF)) {
        if ((gSysSettings.offline != 0) && (Save_TestEventFlag(0x186) == 1) && (Save_TestEventFlag(0x187) == 0)) {
            Save_SetEventFlag(0x187);
        }
        if (Save_TestEventFlag(0x184) == 0) {
            if ((Cond_Check(0x1B) == 1) && ((Village_FacilityLevel(1) & 0xFF) == 2) && ((Village_FacilityLevel(2) & 0xFF) == 2) && ((Village_FacilityLevel(3) & 0xFF) == 2) && (gSave.awayFromVillage == 0) && (Game_IsOnline() == 0)) {
                Save_SetEventFlag(0x184);
            }
        } else if ((Save_TestEventFlag(0x186) == 0) && (Save_TestEventFlag(0x185) == 1)) {
            Save_SetEventFlag(0x186);
            if (!(gSave.rateFlags & 0x100)) {
                RateFlag_SetAndNotify(8U);
            }
            Award_Grant(0x28);
        }
        if (Save_PlayTimeReached(300 /* hours */) == 1) {
            Award_Grant(AWARD_PLAY_TIME);
        }
        if ((Save_IsQuestCleared(0x1389U) != 0) && (Save_TestEventFlag(0x12) == 0) && (Cond_Check(9) == 1)) {
            Save_SetEventFlag(0x12);
            Save_SetEventFlag(0x32);
            Save_SetEventFlag(0x3C);
            Village_SetFacilityLevel(1, 1);
            if (Save_TestEventFlag(0x49) == 0) {
                Save_SetEventFlag(0x49);
            }
        } else if ((Cond_Check(0x18) == 1) && ((Village_FacilityLevel(1) & 0xFF) < 2)) {
            Village_SetFacilityLevel(1, 2);
        } else if ((Save_TestEventFlag(0x35) == 1) && ((Village_FacilityLevel(2) & 0xFF) <= 0)) {
            Village_SetFacilityLevel(2, 1);
        } else if ((Save_TestEventFlag(0x3B) == 1) && ((Village_FacilityLevel(2) & 0xFF) < 2)) {
            Village_SetFacilityLevel(2, 2);
        } else if ((Save_TestEventFlag(0x3F) == 1) && ((Village_FacilityLevel(3) & 0xFF) <= 0)) {
            Village_SetFacilityLevel(3, 1);
        } else if ((Save_TestEventFlag(0x45) == 1) && ((Village_FacilityLevel(3) & 0xFF) < 2)) {
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

/* The QuestInfo of quest `id` on the disc (a downloaded id goes to
 * Quest_FindDownloaded): entry id % 100 of the list of the first id group
 * whose last id (gQuestGroupLastIds) is at least id. Quest_SetCurrent records
 * the entry. */
QuestInfo *Quest_GetDiscInfo(u16 id)
{
    s32 i;
    u16 *groupLast = gQuestGroupLastIds;
    QuestInfo **entry;

    if (id >= QUEST_ID_DOWNLOADED) {
        return Quest_FindDownloaded(id);
    }
    for (i = 0; *groupLast != 0; groupLast++, i++) {
        if (*groupLast >= id) {
            break;
        }
    }
    entry = gGuildQuestLists[i];
    entry += id % 100;
    Quest_SetCurrent(id, entry, 1);
    return *entry;
}

/* The QuestInfo of quest `id`, downloaded or on the disc, without
 * Quest_GetInfo's checks (a downloaded one may be NULL or another quest's). */
QuestInfo *Quest_GetInfoAny(u16 id)
{
    if (id >= QUEST_ID_DOWNLOADED) {
        return Quest_FindDownloaded(id);
    }
    return Quest_GetDiscInfo(id);
}

/* The QuestInfo of downloaded quest `id` in the three quest files
 * (gQuestFiles), or NULL. A file's first word is its QuestInfo's offset. */
QuestInfo *Quest_FindDownloaded(u16 id)
{
    s8 i = 0;
    QuestFile *file = gQuestFiles;

    for (; i < 3; i++, file++) {
        QuestInfo *questInfo = (QuestInfo *)(file->info + (u32)file);

        if (questInfo->id == id) {
            return questInfo;
        }
    }
    return NULL;
}

/* The QuestInfo of the stage file loaded (gStageFile: its first word is the
 * QuestInfo's offset in the file). */
QuestInfo *Quest_CurrentInfo(void)
{
    return (QuestInfo *)(((StageHeader *)gStageFile)->info + gStageFile);
}

/* The slot (0-2) of gQuestFiles that holds downloaded quest `id`, or 3 when
 * none does. */
s8 Quest_DownloadedSlot(u16 id)
{
    s8 i = 0;
    QuestFile *file = gQuestFiles;

    for (; i < 3; i++, file++) {
        QuestInfo *questInfo = (QuestInfo *)(file->info + (u32)file);

        if (questInfo->id == id) {
            return i;
        }
    }
    return i;
}

/* Text `which` of quest `id` (QuestInfo.texts). A downloaded quest's texts
 * are offsets in its file; they are read only while the downloaded quests
 * are ready (LobbyState.dlQuestsReady) and the file holds that quest.
 * Otherwise, and for a missing quest, the unknown quest's text
 * (gQuestCounterTexts[QUEST_COUNTER_TEXT_UNKNOWN_QUEST]). */
char *Quest_GetText(u16 id, s32 which)
{
    QuestInfo *info = Quest_GetInfoAny(id);

    if (id >= QUEST_ID_DOWNLOADED) {
        s8 slot;
        u32 fileBase;

        if (info == NULL) {
            return gQuestCounterTexts[QUEST_COUNTER_TEXT_UNKNOWN_QUEST];
        }
        if (id != info->id || gLobby->dlQuestsReady == 0) {
            return gQuestCounterTexts[QUEST_COUNTER_TEXT_UNKNOWN_QUEST];
        }
        slot = Quest_DownloadedSlot(id);
        fileBase = (u32)&gQuestFiles[slot];
        {
            s32 off = ((s32 *)((u32)info->texts + fileBase))[which];

            return (char *)(off + fileBase);
        }
    }
    if (info == NULL) {
        return gQuestCounterTexts[QUEST_COUNTER_TEXT_UNKNOWN_QUEST];
    }
    return ((char **)info->texts)[which];
}

/* Text `which` of the quest selected in the lobby (gLobbyQuestId); the quest
 * code reads its texts through it. */
char *Quest_SelectedText(s32 which)
{
    QuestInfo *info;

    if (gLobbyQuestId >= QUEST_ID_DOWNLOADED) {
        return Quest_GetText(gLobbyQuestId, which);
    }
    info = Quest_GetDiscInfo(gLobbyQuestId);
    return ((char **)info->texts)[which];
}

/* The QuestInfo of quest `id`, or NULL: a downloaded quest's only while the
 * downloaded quests are ready and its file holds that id. */
QuestInfo *Quest_GetInfo(u16 id)
{
    QuestInfo *info = NULL;

    if (id >= QUEST_ID_DOWNLOADED) {
        if (gLobby->dlQuestsReady == 1) {
            info = Quest_FindDownloaded(id);
            if (info != NULL && info->id != id) {
                info = NULL;
            }
        }
    } else {
        info = Quest_GetDiscInfo(id);
    }
    return info;
}

/* The marks the quest list shows for quest `id`: QuestInfo.flags bits 0-2
 * for a disc quest, 8 for the quests Quest_IsSpecialId lists, 0x10 for the
 * other downloaded quests; 0 for a missing quest. */
u8 Quest_GetListFlags(u16 id)
{
    QuestInfo *info = Quest_GetInfo(id);
    u8 listFlags;

    if (info == NULL) {
        return 0;
    }
    if (id >= QUEST_ID_DOWNLOADED) {
        listFlags = 0;
    } else {
        listFlags = info->flags & 7;
    }
    if (Quest_IsSpecialId(id) == 1) {
        listFlags |= 8;
    } else if (id >= QUEST_ID_DOWNLOADED) {
        listFlags |= 0x10;
    }
    return listFlags;
}

/* The difficulty stars of quest `id` (QuestInfo.stars bits 0-3); quests 3001
 * and 3002 show 4 and 6 stars whatever their info holds. 0 for a missing
 * quest. */
u8 Quest_GetStars(u16 id)
{
    QuestInfo *info = Quest_GetInfo(id);
    u8 starCount;

    if (info == NULL) {
        return 0;
    }
    switch (id) {
    case 0xBB9:
        starCount = 4;
        break;
    case 0xBBA:
        starCount = 6;
        break;
    default:
        starCount = info->stars & 0xF;
        break;
    }
    return starCount;
}

/* Adds gRewardMoney to the save's money (Save_AddMoney) and clears it; the
 * money sound plays unless `silent` or the sum is 0. */
void Money_PayPending(u8 silent)
{
    Save_AddMoney(gRewardMoney);
    if (silent == 0 && gRewardMoney != 0) {
        LobbySe_Play(10);
    }
    gRewardMoney = 0;
}

extern LobbyMenu *gLobbyMenu;

void *memcpy(void *dst, const void *src, u32 n);
u16 LobbyPad_Get(s32);
s32 LobbyStep_Download(s32 slot, s32 id);
void LobbyText_Open(s32);
void LobbyDialog_OpenWait(void *text);
s32 LobbyLevel2_FetchList(void);
s32 LobbyLevel2_EnterFree(s32 v);
s32 LobbyLevel2_FetchRecords(void);
s32 LobbyLevel2_SendSettings(void);
s32 LobbyLevel2_SendQuestAccept(u32 v);
s32 LobbyLevel2_SendQuestPost(MsgQuestPost *d);
s32 strcmp(const void *a, const void *b);
extern LobbyRecs gLobbyRecs;
extern s8 gRoomPassword[9];
extern s8 gQuestMinMembers;
extern s32 gTownMenuOpen;
extern s32 gTownMode;
extern s8 gTownModeStep;
extern u32 gQuestPostArg;

/*
 * Posts the selected quest (gLobbyQuestId) as a quest room, one step a frame
 * (QuestPostHead.postStep): a downloaded quest's file is loaded first, then
 * the server is asked for the room list, the entry, the records (the room's
 * password and the member count go in), the settings, the posting `req` and
 * the quest's acceptance. Returns 0 once posted (LobbyState.questPaid set),
 * 2 while running, 1 once an error shown has been closed with PAD_CIRCLE.
 * The hunter takes no pad input in the field meanwhile (gFieldControl).
 */
s32 Quest_PostStep(MsgQuestPost *req)
{
    QuestPostHead *post = (QuestPostHead *)gMenuWork;
    u16 pad;
    s32 i;
    s8 slot;
    QuestInfo *info;

    pad = LobbyPad_Get(0);
    switch (post->postStep) {
    case 0:
        if (gLobbyQuestId < QUEST_ID_DOWNLOADED) {
            post->postStep++;
            gFieldControl = 0;
        } else {
            slot = Quest_DownloadedSlot(gLobbyQuestId);
            switch (LobbyStep_Download(slot, gLobbyQuestId)) {
            case 0:
                info = Quest_CurrentInfo();
                if (info->id != gLobbyQuestId) {
                    LobbyText_Open(0x63);
                    post->postStep = 8;
                    gFieldControl = 0;
                } else {
                    post->postStep++;
                }
                break;
            case 1:
                LobbyText_Open(0x63);
                post->postStep = 8;
                gFieldControl = 0;
                break;
            }
        }
        break;
    case 1:
        switch (LobbyLevel2_FetchList()) {
        case 0:
            post->postStep++;
            break;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            post->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 2:
        switch (LobbyLevel2_EnterFree(0)) {
        case 0:
            post->postStep++;
            break;
        case 1:
            post->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 3:
        switch (LobbyLevel2_FetchRecords()) {
        case 0:
            memcpy(gLobbyRecs.pass, gRoomPassword, sizeof(gLobbyRecs.pass));
            for (i = 0; i < gLobbyRecs.rec[0].subCount; i++) {
                if (strcmp(gMemberCountTexts[gQuestMinMembers], gLobbyRecs.rec[0].subName[i]) == 0) {
                    gLobbyRecs.rec[0].cur = i;
                    break;
                }
            }
            post->postStep++;
            break;
        case 1:
            post->postStep = 8;
            gTownMenuOpen = 1;
            gTownMode = 0x18;
            gFieldControl = 0;
            gTownModeStep = 0;
            break;
        }
        break;
    case 4:
        switch (LobbyLevel2_SendSettings()) {
        case 0:
            post->postStep++;
            break;
        case 1:
            break;
        }
        break;
    case 5:
        switch (LobbyLevel2_SendQuestPost(req)) {
        case 0:
            post->postStep++;
            break;
        case 1:
            post->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 6:
        post->postStep++;
        gQuestPostArg = (gQuestPostArg & 0xFFFF0000) | gLobbyQuestId;
        break;
    case 7:
        switch (LobbyLevel2_SendQuestAccept(gQuestPostArg)) {
        case 0:
            post->postStep = 0;
            gLobby->questPaid = 1;
            gFieldControl = 1;
            return 0;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            post->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            gFieldControl = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return 1;
        }
        break;
    }
    return 2;
}

/* Types a text into `buf` with the software keyboard (the room's or the
 * records' password), one step a frame (LobbyMenu.screen): opens it, runs it
 * until a text is entered or PAD_SELECT closes it, then closes it. Returns 1
 * once closed, 0 before. */
s32 LobbyMenu_KbdEntry(void *buf)
{
    u16 pad = Pad_GetPressedRepeat(0);

    switch (gLobbyMenu->screen) {
    case 0:
        gLobbyMenu->screen++;
        Kbd_SetPos(0x3A, 80.0f);
        Chat_OpenKbd(3, 6, 8, buf);
        gFieldControl = 0;
        break;
    case 1:
        if (Kbd_Run(buf, (s16)gPadState[0].held, (s16)gPadState[0].pressed) != 0) {
            gLobbyMenu->screen++;
        } else if (pad == PAD_SELECT) {
            gLobbyMenu->screen++;
            LobbySe_Play(LOBBY_SE_CANCEL);
        }
        break;
    case 2:
        Kbd_Cancel();
        gLobbyMenu->screen = 0;
        gFieldControl = 1;
        return 1;
    }
    return 0;
}

/* The file's .data: link bonus 0's items (0 ends) and the gifts by link count. */
u16 gPspLinkBonusItems[8] = {
#include "lobby/007F1990.inc"
};
PspGift gPspLinkGifts[12] = {
#include "lobby/007F19A0.inc"
};
