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
void *memset(void *, s32, u32);
s32 PspLink_Count(void);
typedef struct GiftEntry {
    u16 item;
    u16 count;
} GiftEntry;
typedef struct QuestFile {
    /* 0x000 */ s32 info;           /* offset of the QuestInfo in the file */
    /* 0x004 */ u8 data[0x2FC];
} QuestFile;
extern u32 gStageFile;
typedef struct MonsterBit {
    u16 em;
    u16 bit;
} MonsterBit;

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
extern char *gUnknownQuestTexts[];
s8 Quest_DownloadedSlot(u16 id);
QuestInfo *Quest_GetInfoAny(u16 id);
extern u16 gQuestGroupLastIds[];
extern QuestInfo **gGuildQuestLists[];
void Quest_SetCurrent(u16 id, void *p, s32 c);
extern GiftEntry gGiftTable[];
s32 SavePouch_Space(u16 item);
u8 SavePouch_Add(u16 item, s16 n);
u8 Notice_Push(u8, s16, s16);
void Save_SetEventFlag(s32 flag);
void Menu_PushNotice(s32 msg);
extern u8 gSkillPreview[];
extern u16 *gHrGateQuests[];
extern s16 gGalleryCourseFees[];
extern u8 gMenuWork[];
void Save_AddMoney(s32 n);
void LobbySe_Play(s32 se);
void Equip_Refresh(SaveData *save);
void Player_CalcSkills(Player *p);
void SkillList_Build(Player *p, Player *q);
u8 SkillList_PageCount(void);
u8 Village_FacilityLevel(s32 k);
s32 Save_TestEventFlag(s32 flag);
s32 Save_IsQuestCleared(u16 id);
void Save_ClearEventFlag(s32 flag);
s32 RateFlag_SetAndNotify(s32 a);
s32 PspLink_HasFlag(s32);
void SysSave_UnlockBonus(void);
extern u16 gStoryFlagList[];
void Village_ProcessEventFlags(void);
void Village_QuestTick(void);
void Offer_Generate(void);
void Offer_ClearAll(void);
void Offer_Complete(s32);
void Village_UpdateStoryFlags(void);
s32 LobbyWorld_GetSeason(void);
s32 LobbyWorld_GetSeasonDay(void);
s32 Save_HrGateCleared(s32 n);
s32 Award_Grant(s32 n);
s32 Game_IsOnline(void);
u8 Village_TotalLevel(void);
void Village_SetFacilityLevel(s32 k, u8 v);
s32 Save_PlayTimeReached(s16 n);
extern u8 gQuestRecord[]; /* main: the quest record (QuestRecord, main.c) */
u16 Pad_GetPressedRepeat(s32 pad);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
void Kbd_Cancel(void);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);

s32 Cond_IsLatest(u16 id)
{
    if (id != 0) {
        if (Cond_Check(id) == 1 && Cond_Check(id + 1) == 0) {
            return 1;
        }
    } else {
        return 0;
    }
    return 0;
}

s32 Gift_Which(void)
{
    s32 n = PspLink_Count();

    if (Save_TestEventFlag(0x7B) == 0) {
        return -1;
    }
    if (Save_TestEventFlag(0x31) == 1) {
        return -1;
    }
    switch (n) {
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
        if (n > 8 && (n - 8) % 3 == 0) {
            return (n - 8) % 18 / 3 + 4;
        }
        return -1;
    }
}

void Gift_Give(s32 idx)
{
    u16 item;
    u16 n;

    if (idx != -1 && idx < 10) {
        item = gGiftTable[idx].item;
        n = gGiftTable[idx].count;
        if (SavePouch_Space(item) < n) {
            Menu_PushNotice(0x15);
        } else {
            SavePouch_Add(item, n);
            Notice_Push(1, 0xD, item);
            Save_SetEventFlag(0x31);
        }
    }
}

void Village_UpdateStoryFlags(void)
{
    u16 *cur;
    u16 item;

    if ((PspLink_HasFlag(0) != 0) && (Save_TestEventFlag(0x78) == 0)) {
        Save_SetEventFlag(0x78);
        item = gStoryFlagList[0];
        cur = gStoryFlagList;
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
    if ((PspLink_HasFlag(1) != 0) && (Save_TestEventFlag(0x79) == 0)) {
        Save_SetEventFlag(0x79);
    }
    if ((PspLink_HasFlag(2) != 0) && (Save_TestEventFlag(0x7A) == 0)) {
        Save_SetEventFlag(0x7A);
        if (!(gSave.rateFlags & 8)) {
            RateFlag_SetAndNotify(3U);
        }
    }
    if ((PspLink_HasFlag(3) != 0) && (Save_TestEventFlag(0x7B) == 0)) {
        Save_SetEventFlag(0x7B);
    }
    if ((PspLink_HasFlag(4) != 0) && (Save_TestEventFlag(0x7C) == 0)) {
        Save_SetEventFlag(0x7C);
        SysSave_UnlockBonus();
    }
    if ((gSysSave.bonusUnlocked != 0) && (Save_TestEventFlag(0x7C) == 0)) {
        Save_SetEventFlag(0x7C);
    }
    if ((PspLink_HasFlag(5) != 0) && (Save_TestEventFlag(0x7D) == 0)) {
        Save_SetEventFlag(0x7D);
        Save_SetEventFlag(0x74);
    }
    if (PspLink_HasFlag(6) != 0) {
        if (Save_TestEventFlag(0x7E) == 0) {
            Save_SetEventFlag(0x7E);
        }
        gSave.villageTimers[2].state = 1;
        gSave.villageTimers[2].left = 1;
        Save_ClearEventFlag(0x151);
    }
    if ((PspLink_HasFlag(7) != 0) && (Save_TestEventFlag(0x7F) == 0)) {
        Save_SetEventFlag(0x7F);
    }
    if ((PspLink_HasFlag(8) != 0) && (Cond_Check(0xA) == 1) && (Save_TestEventFlag(0x82) == 0)) {
        Save_SetEventFlag(0x82);
    }
    if ((PspLink_HasFlag(9) != 0) && (Monster_IsHunted(0x40U) == 1) && (Save_TestEventFlag(0x83) == 0)) {
        Save_SetEventFlag(0x83);
    }
}

s32 Monster_AllHuntedBut5(void)
{
    MonsterBit *p = gMonsterFlagBits;
    u16 em = gMonsterFlagBits[0].em;

    while (em != 0) {
        if (Monster_IsHunted(em) == 0) {
            switch (p->em) {
            case 0x41:
            case 0xB:
            case 0x1A:
            case 0x3B:
            case 0x28:
                break;
            default:
                return 0;
            }
        }
        p++;
        em = p->em;
    }
    return 1;
}

s32 Monster_AllHunted(void)
{
    MonsterBit *p = gMonsterFlagBits;
    u16 em = gMonsterFlagBits[0].em;

    while (em != 0) {
        if (Monster_IsHunted(em) == 0) {
            switch (p->em) {
            case 0x3B:
            case 0x28:
                break;
            default:
                return 0;
            }
        }
        p++;
        em = p->em;
    }
    return 1;
}

/* Raw addresses kept here: the named form changes the code. */
void Village_UpdateProgress(s32 arg0)
{
    s32 day;
    u16 id;
    u8 season;
    u8 temp_v1;

    day = LobbyWorld_GetSeasonDay();
    if ((Save_TestEventFlag(0x5A) == 0) && (Save_HrGateCleared(0) == 1)) {
        Save_SetEventFlag(0x5A);
    }
    if ((Save_TestEventFlag(0x5B) == 0) && (Save_HrGateCleared(1) == 1)) {
        Save_SetEventFlag(0x5B);
    }
    if ((Save_TestEventFlag(0x5C) == 0) && (Save_HrGateCleared(2) == 1)) {
        Save_SetEventFlag(0x5C);
    }
    if ((Save_TestEventFlag(0x5D) == 0) && (Save_HrGateCleared(3) == 1)) {
        Save_SetEventFlag(0x5D);
    }
    if ((Save_TestEventFlag(0x5E) == 0) && (Save_HrGateCleared(4) == 1)) {
        Save_SetEventFlag(0x5E);
    }
    if ((Save_IsQuestCleared(0x2775U) != 0) && (Save_IsQuestCleared(0x2776U) != 0) && (Save_IsQuestCleared(0x2777U) != 0) && (Save_TestEventFlag(0xC4) == 0)) {
        Save_SetEventFlag(0xC4);
    }
    if ((Save_TestEventFlag(5) == 0) && ((Save_IsQuestCleared(0x3E9U) != 0) || (Save_IsQuestCleared(0x43EU) != 0) || (Save_IsQuestCleared(0x43FU) != 0))) {
        Save_SetEventFlag(5);
    }
    if ((Monster_IsHunted(0x1BU) == 1) && (Save_TestEventFlag(7) == 0) && (Cond_Check(3) == 1)) {
        Save_SetEventFlag(7);
    }
    if ((Monster_IsHunted(0x44U) == 1) && (Save_TestEventFlag(9) == 0) && (Cond_Check(5) == 1)) {
        Save_SetEventFlag(9);
    }
    if ((Monster_IsHunted(8U) == 1) && (Save_TestEventFlag(0x11) == 0) && (Cond_Check(8) == 1)) {
        Save_SetEventFlag(0x11);
    }
    if ((Monster_IsHunted(0x14U) == 1) && (Save_TestEventFlag(0x46) == 0) && (Cond_Check(0xA) == 1)) {
        Save_SetEventFlag(0x46);
    }
    if ((Monster_IsHunted(0x34U) == 1) && (Save_TestEventFlag(0x47) == 0) && (Cond_Check(0xB) == 1)) {
        Save_SetEventFlag(0x47);
    }
    if ((Save_IsQuestCleared(0x138AU) != 0) && (Save_TestEventFlag(0x4B) == 0)) {
        Save_SetEventFlag(0x4B);
    }
    if ((Save_IsQuestCleared(0x138BU) != 0) && (Save_TestEventFlag(0x4C) == 0)) {
        Save_SetEventFlag(0x4C);
    }
    if ((Monster_IsHunted(0x33U) == 1) && (Save_TestEventFlag(0x4E) == 0)) {
        Save_SetEventFlag(0x4E);
    }
    if ((Monster_IsHunted(1U) == 1) && (Save_TestEventFlag(0x4F) == 0)) {
        Save_SetEventFlag(0x4F);
    }
    if ((Save_IsQuestCleared(0x138CU) != 0) && (Save_TestEventFlag(0x52) == 0)) {
        Save_SetEventFlag(0x52);
    }
    if ((Monster_IsHunted(0x43U) == 1) && (Save_TestEventFlag(0x53) == 0)) {
        Save_SetEventFlag(0x53);
    }
    if ((Monster_IsHunted(0x11U) == 1) && (Save_TestEventFlag(0x54) == 0)) {
        Save_SetEventFlag(0x54);
    }
    if ((Save_IsQuestCleared(0x138FU) != 0) && (Save_TestEventFlag(0x58) == 0)) {
        Save_SetEventFlag(0x58);
        if (gQuestOfferId == 1) {
            gQuestOfferId = 0U;
            memset(&gSave.offerQuests[0], 0, 2);
        }
    }
    if ((Save_TestEventFlag(0x13F) == 0) && ((Village_TotalLevel() & 0xFF) >= 2)) {
        Save_SetEventFlag(0x13F);
    }
    if ((Monster_IsHunted(0xBU) == 1) && (Save_TestEventFlag(0x139) == 0)) {
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
    if (!(arg0 & 0xFF)) {
        if ((gSysSettings.offline != 0) && (Save_TestEventFlag(0x186) == 1) && (Save_TestEventFlag(0x187) == 0)) {
            Save_SetEventFlag(0x187);
        }
        if (Save_TestEventFlag(0x184) == 0) {
            if ((Cond_Check(0x1B) == 1) && ((Village_FacilityLevel(1) & 0xFF) == 2) && ((Village_FacilityLevel(2) & 0xFF) == 2) && ((Village_FacilityLevel(3) & 0xFF) == 2) && (*(s8 *)((u8 *)&gSave + 0x36E8) == 0) && (Game_IsOnline() == 0)) {
                Save_SetEventFlag(0x184);
            }
        } else if ((Save_TestEventFlag(0x186) == 0) && (Save_TestEventFlag(0x185) == 1)) {
            Save_SetEventFlag(0x186);
            if (!(*(u16 *)((u8 *)&gSave + 0x27B4) & 0x100)) {
                RateFlag_SetAndNotify(8U);
            }
            Award_Grant(0x28);
        }
        if (Save_PlayTimeReached(0x12C) == 1) {
            Award_Grant(0x1F);
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
        Village_UpdateStoryFlags();
    }
    if (Game_IsOnline() == 0) {
        if (gSysSettings.offline != 0) {
            temp_v1 = gQuestRecord[4];
            if ((temp_v1 == 6) || (temp_v1 == 4)) {
                id = gQuestOfferId;
                if ((id != 0) && (temp_v1 == 4)) {
                    Offer_Complete((s32) id);
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

QuestInfo *Quest_GetDiscInfo(u16 id)
{
    s32 i;
    u16 *t = gQuestGroupLastIds;
    QuestInfo **p;

    if (id >= 50000) {
        return Quest_FindDownloaded(id);
    }
    for (i = 0; *t != 0; t++, i++) {
        if (*t >= id) {
            break;
        }
    }
    p = gGuildQuestLists[i];
    p += id % 100;
    Quest_SetCurrent(id, p, 1);
    return *p;
}

QuestInfo *Quest_GetInfoAny(u16 id)
{
    if (id >= 50000) {
        return Quest_FindDownloaded(id);
    }
    return Quest_GetDiscInfo(id);
}

QuestInfo *Quest_FindDownloaded(u16 id)
{
    s8 i = 0;
    QuestFile *f = gQuestFiles;

    for (; i < 3; i++, f++) {
        QuestInfo *q = (QuestInfo *)(f->info + (u32)f);

        if (q->id == id) {
            return q;
        }
    }
    return NULL;
}

QuestInfo *Quest_CurrentInfo(void)
{
    return (QuestInfo *)(*(s32 *)gStageFile + gStageFile);
}

s8 Quest_DownloadedSlot(u16 id)
{
    s8 i = 0;
    QuestFile *f = gQuestFiles;

    for (; i < 3; i++, f++) {
        QuestInfo *q = (QuestInfo *)(f->info + (u32)f);

        if (q->id == id) {
            return i;
        }
    }
    return i;
}

char *Quest_GetText(u16 id, s32 k)
{
    QuestInfo *q = Quest_GetInfoAny(id);

    if (id >= 50000) {
        s8 slot;
        u32 f;

        if (q == NULL) {
            return gUnknownQuestTexts[0];
        }
        if (id != q->id || gLobby->dlQuestsReady == 0) {
            return gUnknownQuestTexts[0];
        }
        slot = Quest_DownloadedSlot(id);
        f = (u32)&gQuestFiles[slot];
        {
            s32 off = ((s32 *)((u32)q->texts + f))[k];

            return (char *)(off + f);
        }
    }
    if (q == NULL) {
        return gUnknownQuestTexts[0];
    }
    return ((char **)q->texts)[k];
}

char *Quest_SelectedText(s32 k)
{
    QuestInfo *q;

    if (gLobbyQuestId >= 50000) {
        return Quest_GetText(gLobbyQuestId, k);
    }
    q = Quest_GetDiscInfo(gLobbyQuestId);
    return ((char **)q->texts)[k];
}

QuestInfo *Quest_GetInfo(u16 id)
{
    QuestInfo *q = NULL;

    if (id >= 50000) {
        if (gLobby->dlQuestsReady == 1) {
            q = Quest_FindDownloaded(id);
            if (q != NULL && q->id != id) {
                q = NULL;
            }
        }
    } else {
        q = Quest_GetDiscInfo(id);
    }
    return q;
}

u8 Quest_GetListFlags(u16 id)
{
    QuestInfo *q = Quest_GetInfo(id);
    u8 r;

    if (q == NULL) {
        return 0;
    }
    if (id >= 50000) {
        r = 0;
    } else {
        r = q->flags & 7;
    }
    if (Quest_IsSpecialId(id) == 1) {
        r |= 8;
    } else if (id >= 50000) {
        r |= 0x10;
    }
    return r;
}

u8 Quest_GetStars(u16 id)
{
    QuestInfo *q = Quest_GetInfo(id);
    u8 r;

    if (q == NULL) {
        return 0;
    }
    switch (id) {
    case 0xBB9:
        r = 4;
        break;
    case 0xBBA:
        r = 6;
        break;
    default:
        r = q->stars & 0xF;
        break;
    }
    return r;
}

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
s32 LobbyLevel2_Send6509(u32 v);
s32 LobbyLevel2_Send650B(Msg650B *d);
s32 strcmp(const void *a, const void *b);
extern LobbyRecs gLobbyRecs;
extern s8 gRoomPassword[9];
extern s8 gQuestMinMembers;
extern char *gMemberCountTexts[];
extern s32 gTownMenuOpen;
extern s32 gTownMode;
extern s8 gTownModeStep;
extern u32 gQuestPostArg;

s32 Quest_PostStep(Msg650B *req)
{
    QuestPostHead *w = (QuestPostHead *)gMenuWork;
    u16 pad;
    s32 i;
    s8 slot;
    QuestInfo *q;

    pad = LobbyPad_Get(0);
    switch (w->postStep) {
    case 0:
        if (gLobbyQuestId < 50000) {
            w->postStep++;
            gFieldControl = 0;
        } else {
            slot = Quest_DownloadedSlot(gLobbyQuestId);
            switch (LobbyStep_Download(slot, gLobbyQuestId)) {
            case 0:
                q = Quest_CurrentInfo();
                if (q->id != gLobbyQuestId) {
                    LobbyText_Open(0x63);
                    w->postStep = 8;
                    gFieldControl = 0;
                } else {
                    w->postStep++;
                }
                break;
            case 1:
                LobbyText_Open(0x63);
                w->postStep = 8;
                gFieldControl = 0;
                break;
            }
        }
        break;
    case 1:
        switch (LobbyLevel2_FetchList()) {
        case 0:
            w->postStep++;
            break;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            w->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 2:
        switch (LobbyLevel2_EnterFree(0)) {
        case 0:
            w->postStep++;
            break;
        case 1:
            w->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 3:
        switch (LobbyLevel2_FetchRecords()) {
        case 0:
            memcpy(gLobbyRecs.pass, gRoomPassword, 9);
            for (i = 0; i < gLobbyRecs.rec[0].m; i++) {
                if (strcmp(gMemberCountTexts[gQuestMinMembers], gLobbyRecs.rec[0].subName[i]) == 0) {
                    gLobbyRecs.rec[0].cur = i;
                    break;
                }
            }
            w->postStep++;
            break;
        case 1:
            w->postStep = 8;
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
            w->postStep++;
            break;
        case 1:
            break;
        }
        break;
    case 5:
        switch (LobbyLevel2_Send650B(req)) {
        case 0:
            w->postStep++;
            break;
        case 1:
            w->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 6:
        w->postStep++;
        gQuestPostArg = (gQuestPostArg & 0xFFFF0000) | gLobbyQuestId;
        break;
    case 7:
        switch (LobbyLevel2_Send6509(gQuestPostArg)) {
        case 0:
            w->postStep = 0;
            gLobby->questPaid = 1;
            gFieldControl = 1;
            return 0;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            w->postStep = 8;
            gFieldControl = 0;
            break;
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            gFieldControl = 1;
            LobbySe_Play(8);
            return 1;
        }
        break;
    }
    return 2;
}

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
        if (Kbd_Run(buf, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            gLobbyMenu->screen++;
        } else if (pad == 0x4000) {
            gLobbyMenu->screen++;
            LobbySe_Play(4);
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

/* unk_75E3B0's .data (0x7F1990-0x7F19D0) */
u16 gStoryFlagList[8] = {
#include "lobby/007F1990.inc"
};
GiftEntry gGiftTable[12] = {
#include "lobby/007F19A0.inc"
};
