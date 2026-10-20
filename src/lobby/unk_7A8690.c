#include "common.h"
#include "em.h"
#include "lobby_npc.h"
#include "player.h"
#include "save.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "scene.h"
#include "chat_log.h"
#include "lobby_labels.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "pad.h"

s32 Cond_Check(u16 id);
void Save_SetEventFlag(s32 flag);
s32 Save_TestEventFlag(s32 flag);
s32 Dojo_OnlineDrillsCleared(void);
s32 Dojo_OnlineChainCleared(void);
s32 ElderRaid_InState1(void);
extern s32 gTalkEventFlag;
s32 Village_Event186Pending(void);
s32 Town_SleepEventPending(void);
u8 Village_FacilityLevel(s32 k);
s32 PspGift_Pick(void);
void PspGift_Give(s32 gift);
/* unit's own declaration: with the definition's s16 (s32 arg0) (lobby/dojo_menu.c), Talk_Check0A: 28 of 47 words differ, Talk_Check0B: 31 of 71 words differ ... */
s32 Village_MilestoneState(s32);
s32 PspLink_Count(void);
s32 Combine_KnowsMostRecipes(void);
s32 Dojo_OfflineDrillsCleared(void);
s32 Dojo_OfflineChainCleared(void);
s32 ElderRaid_InState2(void);
s32 Save_HasTitle(s32);
s32 Cond_IsLatest(u16 id);
s32 Save_IsQuestCleared(u16 item);
void LobbySe_Play(s32);
void CamFocus_End(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;
s32 LobbyTime_IsPm(void);
s32 LobbyWorld_GetSeason(void);
void Jingle_Play(void *obj, s32 id);
extern u16 gTalkEventStep;
extern void *gTalkScriptRec;
void *memcpy(void *dst, const void *src, u32 n);

void Talk_Close(void)
{
    Player *p;

    p = &gPlayers[gScene.localPlayer];
    LobbySe_Play(4);
    p->drinkCount = 0;
    gMenuInputLock = 0x14;
    gTownMode = 0;
    gTownMenuOpen = 0;
    CamFocus_End();
}

extern u16 gTalkScript;
struct Em;
s16 OfferMenu_BuildList(struct Em *t, s32 b);
void QuestDesk_Open(u8 *npc);
void DojoMenu_Open(struct Em *instructor);
s32 ShipMenu_IsOffered(void);
void ShipMenu_Open(u8 *npc);
void ItemShop_Open(u8 *npc, s8 sel);
void CombShop_Open(u8 *npc);
void QuestList_Rebuild(void);
void Npc_RefreshOffers(void);
void EquipShop_Open(u8 *npc);
void Mine_OpenMenu(u8 *npc);
void TownGuide_Open(void);
void Workshop_Open(struct Em *npc);
void ElderForecast_Open(void);
s32 Menu_PushNoticeOnce(s32);
void OfficialDesk_Open(struct Em *npc);
void ArenaCup_Open(u8 *npc);
void SongMenu_Open(u8 *unused0);
void IngredientList_Open(struct Em *cook);
void HairMenu_Open(Em *npc, s32 b);
s32 MyHouse_IsOwner(void);
void OfferMenu_Open(struct Em *t);

void Talk_FollowUp(u16 kind)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 *npc = pl->target;
    s8 offers = OfferMenu_BuildList((struct Em *) npc, 0);

    switch (gTalkScript) {
    case 1:
        break;
    case 2:
        if (gTalkEventStep != 1 && Cond_Check(1)) {
            QuestDesk_Open(npc);
        }
        if (!Save_TestEventFlag(0x137)) {
            Save_SetEventFlag(0x137);
        }
        break;
    case 0x6C:
        QuestDesk_Open(npc);
        break;
    case 3:
        if (gTalkEventStep != 5 && Cond_Check(0x1F)) {
            DojoMenu_Open((struct Em *)npc);
        }
        break;
    case 5:
        if (ShipMenu_IsOffered() == 1) {
            ShipMenu_Open(npc);
        }
        break;
    case 7:
        if (gTalkEventStep != 1 && Cond_Check(1)) {
            gTownMode = 2;
            ItemShop_Open(npc, -1);
        }
        break;
    case 8:
        if (Cond_Check(5)) {
            CombShop_Open(npc);
        }
        break;
    case 0x2C:
        CombShop_Open(npc);
        break;
    case 0x10:
        if (Cond_IsLatest(0xF) == 1 && !Save_TestEventFlag(0x4D)) {
            Save_SetEventFlag(0x4D);
            QuestList_Rebuild();
        }
        if (Cond_IsLatest(0x13) == 1 && !Save_TestEventFlag(0x51)) {
            Save_SetEventFlag(0x51);
            Npc_RefreshOffers();
        }
        break;
    case 0x12:
        if (!Save_TestEventFlag(0x4A)) {
            Save_SetEventFlag(0x4A);
        }
        if (gTalkEventStep != 3) {
            ItemShop_Open(npc, -1);
        }
        break;
    case 0x42:
        if (Save_TestEventFlag(0xC4) == 1) {
            ItemShop_Open(npc, -1);
        }
        break;
    case 0xA:
    case 0xB:
    case 0x2D:
    case 0x2E:
    case 0x39:
    case 0x50:
    case 0x5D:
    case 0x6A:
    case 0xD:
    case 0x28:
        ItemShop_Open(npc, -1);
        break;
    case 0xC:
        if (*(f32 *)(npc + 0xAC) > 10200.0f) {
            ItemShop_Open(npc, -1);
        } else {
            EquipShop_Open(npc);
        }
        break;
    case 0x13:
        if (*(u8 *)((u8 *)&gSave + 0x37F3) != 1) {
            Mine_OpenMenu(npc);
        }
        break;
    case 0x1C:
        TownGuide_Open();
        break;
    case 0x56:
    case 0x5E:
        EquipShop_Open(npc);
        break;
    case 0xE:
        if (gTalkEventStep != 0xF) {
            if (pl->pos[1] < 100.0f && Cond_Check(0x2F) == 1) {
                EquipShop_Open(npc);
            } else if (Cond_Check(0x2F) == 1) {
                Workshop_Open((struct Em *)npc);
            }
        }
        break;
    case 0x1E:
        ElderForecast_Open();
        break;
    case 0x37:
        if (Cond_Check(0x59)) {
            DojoMenu_Open((struct Em *)npc);
        }
        break;
    case 0x38:
        if (Cond_Check(0x59)) {
            QuestDesk_Open(npc);
        }
        break;
    case 0x40:
        if (gTalkEventStep == 3) {
            Menu_PushNoticeOnce(0x11);
        }
    case 0x4E:
        if (gTalkEventStep == 0xFFFF || gLobby->elderRaidState != 0) {
            OfficialDesk_Open((struct Em *)npc);
        }
        break;
    case 0x41:
        ArenaCup_Open(npc);
        break;
    case 0x49:
        SongMenu_Open(npc);
        break;
    case 0x4F:
        QuestDesk_Open(npc);
        break;
    case 0x58:
    case 0xF:
    case 0x68:
    case 0x55:
        Workshop_Open((struct Em *)npc);
        break;
    case 0x71:
        if (gTalkEventStep == 0xFFFF && MyHouse_IsOwner() == 1) {
            IngredientList_Open((struct Em *)npc);
        }
        break;
    case 0x6F:
        HairMenu_Open((Em *)npc, 0);
    }
    switch (gTownMode) {
    case 4:
    case 5:
    case 0:
        if (offers == 0) {
            Talk_Close();
        } else {
            OfferMenu_Open((struct Em *) npc);
            LobbySe_Play(0x11);
        }
        break;
    }
}

void Talk_GiveOnStep3(u16 k)
{
    switch (k) {
    case 3:
        PspGift_Give(PspGift_Pick());
        break;
    }
}

void Talk_PlayAmJingle(u16 k)
{
    if ((gTalkEventStep == 0x63) && (LobbyTime_IsPm() == 0)) {
        Jingle_Play(&gPlayers[gScene.localPlayer], 0x133);
    }
}

void Talk_StepAction(u16 id, u16 k)
{
    switch (id) {
    case 0x12:
        Talk_GiveOnStep3(k);
        break;
    case 0x45:
    case 0x46:
    case 0x47:
        Talk_PlayAmJingle(k);
        break;
    }
}

void Npc_ClearAllTalked(void);
s32 Npc_IsType4D(Em *);
void CamFocus_Start(Em *npc);
void Talk_Start(Em *npc, s8 quiet);
s32 Talk_Step(s8 flag);
void Menu_PushNotice(s32 msg);
u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
void Save_AddMoney(s32 n);
u16 Box_FindFreeSlot(void *unused0);
s32 SavePouch_Space(u16 item);
u8 SavePouch_Add(u16 item, s16 n);
s32 Item_IsRecordOnly(u16 item);
u8 Notice_Push(u8, s16, s16);
u8 Notice_PushText(char *);
s32 sprintf(char *, const char *, ...);
void Sjis_Widen(char *src, char *dst);
extern s8 gTalkFinishStep;
extern s32 gTalkLine;
extern TalkLine *gTalkLines[];
extern TalkGift *gTalkGifts[];

/* The end of a talk: the gift of gTalkLines/gTalkGifts (money, a pouch item,
 * equipment), the event flag, then Talk_FollowUp. */
s32 Talk_Finish(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    TalkGift *g;
    TalkLine *talks;
    u8 gift;
    char msg[0x20];
    char buf[0x20];

    if (((u8 *)pl->target) == NULL) {
        gTownMode = 0;
        return 3;
    }
    switch (gTalkFinishStep) {
    case 0:
        if (Npc_IsType4D((Em *)((u8 *)pl->target)) == 0) {
            CamFocus_Start(pl->target);
        }
        Talk_Start(pl->target, 0);
        gTalkFinishStep++;
        break;
    case 1:
        switch (Talk_Step(0)) {
        case 0:
        case 3:
            if (gTalkEventStep != 0xFFFF) {
                Talk_StepAction(gTalkScript, gTalkEventStep);
                Npc_ClearAllTalked();
            }
            talks = gTalkLines[gTalkScript];
            talks += gTalkLine;
            gift = talks->gift;
            if (gift != TALK_GIFT_NONE) {
                g = gTalkGifts[gTalkScript];
                g += gift;
                switch (g->kind) {
                case TALK_GIFT_MONEY:
                    Save_AddMoney(g->a.value);
                    LobbySe_Play(LOBBY_SE_MONEY);
                    sprintf(msg, gTownLabels[TOWN_LABEL_ZENNY_GOT_FMT], g->a.value);
                    Sjis_Widen(msg, buf);
                    Notice_PushText(buf);
                    if (gTalkEventFlag != -1) {
                        Save_SetEventFlag(gTalkEventFlag);
                    }
                    break;
                case TALK_GIFT_ITEM:
                    if (SavePouch_Space(g->a.item) >= g->b.count || Item_IsRecordOnly(g->a.item) == 1) {
                        SavePouch_Add(g->a.item, g->b.count);
                        Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, g->a.value);
                        if (gTalkEventFlag != -1) {
                            Save_SetEventFlag(gTalkEventFlag);
                        }
                        break;
                    }
                    Menu_PushNotice(LOBBY_NOTICE_POUCH_FULL);
                    Talk_Close();
                    return 0;
                case TALK_GIFT_EQUIP:
                    if ((u16)Box_FindFreeSlot(&gSave) != BOX_NONE) {
                        Box_AddItem(&gSave, g->a.type, g->b.id, 0);
                        Menu_PushNotice(LOBBY_NOTICE_EQUIP_GOT);
                        if (gTalkEventFlag != -1) {
                            Save_SetEventFlag(gTalkEventFlag);
                        }
                        break;
                    }
                    Menu_PushNotice(LOBBY_NOTICE_BOX_FULL);
                    Talk_Close();
                    return 0;
                }
            } else if (gTalkEventFlag != -1) {
                Save_SetEventFlag(gTalkEventFlag);
            }
            Talk_FollowUp(gTalkScript);
            return 0;
        }
        break;
    }
    return 2;
}

void EventFlag_Set(s32 n);

s32 Talk_Check01(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0) == 0) {
            gTalkEventFlag = 0;
            EventFlag_Set(0x1E);
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0xD) == 0 && Cond_Check(2) == 1) {
            gTalkEventFlag = 0xD;
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(0xE) == 0 && Cond_Check(3) == 1) {
            gTalkEventFlag = 0xE;
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(0xF) == 0 && Cond_Check(6) == 1) {
            gTalkEventFlag = 0xF;
            Save_SetEventFlag(0xE);
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(0x10) == 0 && Cond_Check(7) == 1) {
            gTalkEventFlag = 0x10;
            return 1;
        }
        break;
    case 6:
        if (Save_TestEventFlag(0x8B) == 0 && Cond_Check(8) == 1) {
            gTalkEventFlag = 0x8B;
            return 1;
        }
        break;
    case 7:
        if (Save_TestEventFlag(0x17A) == 0 && Cond_Check(9) == 1) {
            gTalkEventFlag = 0x17A;
            Save_SetEventFlag(0x8B);
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(0x17B) == 0 && Cond_Check(0xA) == 1) {
            gTalkEventFlag = 0x17B;
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(0x17C) == 0 && Cond_Check(0xC) == 1) {
            gTalkEventFlag = 0x17C;
            Save_SetEventFlag(0x17B);
            return 1;
        }
        break;
    case 10:
        if (Save_TestEventFlag(0x17D) == 0 && Cond_Check(0xD) == 1) {
            gTalkEventFlag = 0x17D;
            Save_SetEventFlag(0x17C);
            Save_SetEventFlag(0x17B);
            return 1;
        }
        break;
    case 11:
        if (Save_TestEventFlag(0x17E) == 0 && Cond_Check(0x14) == 1) {
            gTalkEventFlag = 0x17E;
            Save_SetEventFlag(0x17D);
            Save_SetEventFlag(0x17C);
            Save_SetEventFlag(0x17B);
            return 1;
        }
        break;
    case 12:
        if (Save_TestEventFlag(0x17F) == 0 && Cond_Check(0x15) == 1) {
            gTalkEventFlag = 0x17F;
            Save_SetEventFlag(0x17E);
            Save_SetEventFlag(0x17D);
            Save_SetEventFlag(0x17C);
            Save_SetEventFlag(0x17B);
            return 1;
        }
        break;
    case 13:
        if (Save_TestEventFlag(0x180) == 0 && Cond_Check(0x17) == 1) {
            gTalkEventFlag = 0x180;
            Save_SetEventFlag(0x17F);
            Save_SetEventFlag(0x17E);
            Save_SetEventFlag(0x17D);
            Save_SetEventFlag(0x17C);
            Save_SetEventFlag(0x17B);
            return 1;
        }
        break;
    case 14:
        if (Save_TestEventFlag(0x181) == 0 && Cond_Check(0x18) == 1) {
            gTalkEventFlag = 0x181;
            Save_SetEventFlag(0x180);
            Save_SetEventFlag(0x17F);
            Save_SetEventFlag(0x17E);
            Save_SetEventFlag(0x17D);
            Save_SetEventFlag(0x17C);
            Save_SetEventFlag(0x17B);
            return 1;
        }
        break;
    case 15:
        if (Cond_Check(0x19) == 1 && Save_IsQuestCleared(0x138D) == 0 && Save_TestEventFlag(0x1CF) == 0) {
            gTalkEventFlag = 0x1CF;
            return 1;
        }
        break;
    case 16:
        if (Cond_Check(0x19) == 1 && Save_IsQuestCleared(0x138D) == 1 && Save_IsQuestCleared(0x138E) == 0 && Save_TestEventFlag(0x1D0) == 0) {
            gTalkEventFlag = 0x1D0;
            return 1;
        }
        break;
    case 17:
        if (Cond_Check(0x19) == 1 && Save_IsQuestCleared(0x138D) == 1 && Save_IsQuestCleared(0x138E) == 1 && Save_IsQuestCleared(0x138F) == 0 && Save_TestEventFlag(0x1D1) == 0) {
            gTalkEventFlag = 0x1D1;
            return 1;
        }
        break;
    case 18:
        if (Cond_Check(0x1A) == 1 && Save_TestEventFlag(0x182) == 0) {
            gTalkEventFlag = 0x182;
            return 1;
        }
        break;
    case 19:
        if (Cond_Check(0x1B) == 1 && Save_TestEventFlag(0x183) == 0) {
            gTalkEventFlag = 0x183;
            Save_SetEventFlag(0x182);
            return 1;
        }
        break;
    case 20:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    case 21:
        if (Village_Event186Pending() == 1 && Save_TestEventFlag(0x188) == 0) {
            gTalkEventFlag = 0x188;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check02(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0x13) == 0 && Cond_Check(1) == 1) {
            gTalkEventFlag = 0x13;
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x14) == 0 && Cond_Check(7) == 1) {
            gTalkEventFlag = 0x14;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(0xA) == 1 && Save_TestEventFlag(0x189) == 0) {
            gTalkEventFlag = 0x189;
            Save_SetEventFlag(0x14);
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(0x10) == 1 && Save_TestEventFlag(0x18A) == 0) {
            gTalkEventFlag = 0x18A;
            Save_SetEventFlag(0x189);
            Save_SetEventFlag(0x14);
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(0x15) == 1 && Save_TestEventFlag(0x18B) == 0) {
            gTalkEventFlag = 0x18B;
            Save_SetEventFlag(0x18A);
            Save_SetEventFlag(0x189);
            Save_SetEventFlag(0x14);
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(0x1A) == 1 && Save_TestEventFlag(0x18C) == 0) {
            gTalkEventFlag = 0x18C;
            Save_SetEventFlag(0x18B);
            Save_SetEventFlag(0x18A);
            Save_SetEventFlag(0x189);
            Save_SetEventFlag(0x14);
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(0x1B) == 1 && Save_TestEventFlag(0x18D) == 0) {
            gTalkEventFlag = 0x18D;
            Save_SetEventFlag(0x18C);
            Save_SetEventFlag(0x18B);
            Save_SetEventFlag(0x18A);
            Save_SetEventFlag(0x189);
            Save_SetEventFlag(0x14);
            return 1;
        }
        break;
    case 8:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check03(u16 k)
{
    switch (k) {
    case 5:
        if (Save_TestEventFlag(0x15) == 0 && Cond_Check(1) == 1) {
            gTalkEventFlag = 0x15;
            return 1;
        }
        break;
    case 1:
        if (Save_TestEventFlag(0x16) == 0 && Cond_Check(3) == 1) {
            gTalkEventFlag = 0x16;
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x17) == 0 && Cond_Check(7) == 1) {
            gTalkEventFlag = 0x17;
            Save_SetEventFlag(0x16);
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(0x18) == 0 && Cond_Check(0xA) == 1) {
            gTalkEventFlag = 0x18;
            Save_SetEventFlag(0x17);
            Save_SetEventFlag(0x16);
            return 1;
        }
        break;
    case 4:
        if ((u8)Village_FacilityLevel(1) == 2 && Save_TestEventFlag(0x18E) == 0) {
            gTalkEventFlag = 0x18E;
            Save_SetEventFlag(0x18);
            Save_SetEventFlag(0x17);
            Save_SetEventFlag(0x16);
            return 1;
        }
        break;
    case 6:
        if (Dojo_OfflineDrillsCleared() == 1 && Save_TestEventFlag(0x18F) == 0) {
            gTalkEventFlag = 0x18F;
            Save_SetEventFlag(0x18E);
            Save_SetEventFlag(0x18);
            Save_SetEventFlag(0x17);
            Save_SetEventFlag(0x16);
            return 1;
        }
        break;
    case 7:
        if (Dojo_OfflineChainCleared() == 1 && Save_TestEventFlag(0x190) == 0) {
            gTalkEventFlag = 0x190;
            return 1;
        }
        break;
    case 8:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check04(u16 k)
{
    switch (k) {
    case 4:
        if (Save_TestEventFlag(0x12E) == 0) {
            gTalkEventFlag = 0x12E;
            return 1;
        }
        break;
    case 1:
        if (Cond_Check(7) == 1 && Save_TestEventFlag(0x1CB) == 0) {
            gTalkEventFlag = 0x1CB;
            Save_SetEventFlag(0x12E);
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x1CD) == 0 && Save_TestEventFlag(0x79) == 1 && Save_TestEventFlag(0x30) == 0) {
            gTalkEventFlag = 0x1CD;
            return 1;
        }
        break;
    case 3:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

u16 Record_GroupLargest(SaveData *save, u8 group, u8 *out);
s32 ShipMenu_IsOffered(void);

s32 Talk_Check05(u16 k)
{
    switch (k) {
    case 13:
        if (Save_TestEventFlag(0x12F) == 0) {
            gTalkEventFlag = 0x12F;
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(0x43) == 1 && Save_TestEventFlag(0x192) == 0) {
            gTalkEventFlag = 0x192;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(0x44) == 1 && Save_TestEventFlag(0x193) == 0) {
            gTalkEventFlag = 0x193;
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(0x45) == 1 && Save_TestEventFlag(0x194) == 0) {
            gTalkEventFlag = 0x194;
            Save_SetEventFlag(0x193);
            return 1;
        }
        break;
    case 1:
        if ((u8)Village_FacilityLevel(3) > 0 && (u16)Record_GroupLargest(&gSave, 0x15, 0) >= 0x7B && Save_TestEventFlag(0x195) == 0) {
            gTalkEventFlag = 0x195;
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(0x47) == 1 && Save_TestEventFlag(0x196) == 0) {
            gTalkEventFlag = 0x196;
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(0x48) == 1 && Save_TestEventFlag(0x197) == 0) {
            gTalkEventFlag = 0x197;
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(0x49) == 1 && Save_TestEventFlag(0x198) == 0) {
            gTalkEventFlag = 0x198;
            return 1;
        }
        break;
    case 8:
        if (Cond_Check(0x4A) == 1 && Save_TestEventFlag(0x199) == 0) {
            gTalkEventFlag = 0x199;
            return 1;
        }
        break;
    case 9:
        if (Cond_Check(0x4B) == 1 && Save_TestEventFlag(0x19A) == 0) {
            gTalkEventFlag = 0x19A;
            return 1;
        }
        break;
    case 10:
        if (Cond_Check(0x4C) == 1 && Save_TestEventFlag(0x19B) == 0) {
            gTalkEventFlag = 0x19B;
            Save_SetEventFlag(0x19A);
            return 1;
        }
        break;
    case 12:
        if (ShipMenu_IsOffered() == 1 && Save_TestEventFlag(0x1C8) == 0) {
            gTalkEventFlag = 0x1C8;
            return 1;
        }
        break;
    case 11:
        if (Cond_Check(0x4E) == 1 && Save_TestEventFlag(0x19C) == 0) {
            gTalkEventFlag = 0x19C;
            return 1;
        }
        break;
    case 14:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check06(u16 k)
{
    switch (k) {
    case 0x1:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check07(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0x19) == 0 && Cond_Check(1) == 1) {
            gTalkEventFlag = 0x19;
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x1A) == 0 && Cond_Check(7) == 1) {
            gTalkEventFlag = 0x1A;
            Save_SetEventFlag(0x19);
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(0x130) == 0 && Cond_Check(0xA) == 1) {
            gTalkEventFlag = 0x130;
            Save_SetEventFlag(0x19);
            Save_SetEventFlag(0x1A);
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(0x131) == 0 && Cond_Check(0x10) == 1) {
            gTalkEventFlag = 0x131;
            Save_SetEventFlag(0x19);
            Save_SetEventFlag(0x1A);
            Save_SetEventFlag(0x130);
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(0x12) == 1 && Save_TestEventFlag(0x19D) == 0) {
            gTalkEventFlag = 0x19D;
            Save_SetEventFlag(0x19);
            Save_SetEventFlag(0x1A);
            Save_SetEventFlag(0x130);
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(0x13) == 1 && Save_TestEventFlag(0x19E) == 0) {
            gTalkEventFlag = 0x19E;
            return 1;
        }
        break;
    case 7:
        if (Save_TestEventFlag(0x132) == 0 && Cond_Check(0x15) == 1) {
            gTalkEventFlag = 0x132;
            Save_SetEventFlag(0x19E);
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(0x133) == 0 && Cond_Check(0x19) == 1) {
            gTalkEventFlag = 0x133;
            Save_SetEventFlag(0x19E);
            Save_SetEventFlag(0x132);
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(0x134) == 0 && Cond_Check(0x1A) == 1) {
            gTalkEventFlag = 0x134;
            Save_SetEventFlag(0x19E);
            Save_SetEventFlag(0x132);
            Save_SetEventFlag(0x133);
            return 1;
        }
        break;
    case 10:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check08(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0x1B) == 0) {
            gTalkEventFlag = 0x1B;
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x1C) == 0 && Cond_Check(4) == 1) {
            gTalkEventFlag = 0x1C;
            Save_SetEventFlag(0x1B);
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(0x135) == 0 && Cond_Check(5) == 1) {
            gTalkEventFlag = 0x135;
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(0x1D) == 0 && Cond_Check(7) == 1) {
            gTalkEventFlag = 0x1D;
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(0xA) == 1 && Save_TestEventFlag(0x19F) == 0) {
            gTalkEventFlag = 0x19F;
            Save_SetEventFlag(0x1D);
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(0x10) == 1 && Save_TestEventFlag(0x1A0) == 0) {
            gTalkEventFlag = 0x1A0;
            Save_SetEventFlag(0x19F);
            Save_SetEventFlag(0x1D);
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(0x15) == 1 && Save_TestEventFlag(0x1A1) == 0) {
            gTalkEventFlag = 0x1A1;
            Save_SetEventFlag(0x1A0);
            Save_SetEventFlag(0x19F);
            Save_SetEventFlag(0x1D);
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(0x1A) == 1 && Save_TestEventFlag(0x1A2) == 0) {
            gTalkEventFlag = 0x1A2;
            Save_SetEventFlag(0x1A1);
            Save_SetEventFlag(0x1A0);
            Save_SetEventFlag(0x19F);
            Save_SetEventFlag(0x1D);
            return 1;
        }
        break;
    case 8:
        if (Combine_KnowsMostRecipes() == 1 && Save_TestEventFlag(0xB8) == 0) {
            gTalkEventFlag = 0xB8;
            return 1;
        }
        break;
    case 10:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check09(u16 k)
{
    switch (k) {
    case 0x1:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check0A(u16 k)
{
    switch (k) {
    case 2:
        if (Save_TestEventFlag(0x1A3) == 0) {
            gTalkEventFlag = 0x1A3;
            return 1;
        }
        break;
    case 1:
        if (Village_MilestoneState(0) != 0) {
            if (Save_TestEventFlag(0x1A3) == 0) {
                Save_SetEventFlag(0x1A3);
            }
            return 1;
        }
        break;
    case 3:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check0B(u16 k)
{
    switch (k) {
    case 3:
        if (Save_TestEventFlag(0x1A4) == 0) {
            gTalkEventFlag = 0x1A4;
            return 1;
        }
        break;
    case 1:
        if ((u8)Village_FacilityLevel(3) == 2 && Save_TestEventFlag(0x1A5) == 0) {
            gTalkEventFlag = 0x1A5;
            Save_SetEventFlag(0x1A4);
            return 1;
        }
        break;
    case 2:
        if (Village_MilestoneState(0) != 0) {
            Save_SetEventFlag(0x1A5);
            Save_SetEventFlag(0x1A4);
            return 1;
        }
        break;
    case 4:
        if (Village_Event186Pending() == 1) {
            Save_SetEventFlag(0x1A5);
            Save_SetEventFlag(0x1A4);
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check0C(u16 k)
{
    switch (k) {
    case 2:
        if (Save_TestEventFlag(0x1A8) == 0) {
            gTalkEventFlag = 0x1A8;
            return 1;
        }
        break;
    case 1:
        if (Save_TestEventFlag(0x1A9) == 0 && (u8)Village_FacilityLevel(2) == 2) {
            gTalkEventFlag = 0x1A9;
            Save_SetEventFlag(0x1A8);
            return 1;
        }
        break;
    case 3:
        if (Village_Event186Pending() == 1) {
            Save_SetEventFlag(0x1A9);
            Save_SetEventFlag(0x1A8);
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check0D(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0x1AA) == 0) {
            gTalkEventFlag = 0x1AA;
            return 1;
        }
        break;
    case 2:
        if (Village_Event186Pending() == 1) {
            Save_SetEventFlag(0x1AA);
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check0E(u16 k)
{
    switch (k) {
    case 15:
        if (Save_TestEventFlag(0x1E) == 0 && Cond_Check(0x2F) == 1) {
            gTalkEventFlag = 0x1E;
            return 1;
        }
        break;
    case 14:
        if (Save_TestEventFlag(0x1F) == 0 && Cond_Check(0x30) == 1) {
            gTalkEventFlag = 0x1F;
            Save_SetEventFlag(0x1E);
            return 1;
        }
        break;
    case 13:
        if (Save_TestEventFlag(0x20) == 0 && Cond_Check(0x31) == 1) {
            gTalkEventFlag = 0x20;
            Save_SetEventFlag(0x1F);
            Save_SetEventFlag(0x1E);
            return 1;
        }
        break;
    case 12:
        if (Save_TestEventFlag(0x21) == 0 && Cond_Check(0x32) == 1) {
            gTalkEventFlag = 0x21;
            Save_SetEventFlag(0x1E);
            Save_SetEventFlag(0x1F);
            Save_SetEventFlag(0x20);
            return 1;
        }
        break;
    case 10:
        if (Save_TestEventFlag(0x22) == 0 && Cond_Check(0x33) == 1) {
            gTalkEventFlag = 0x22;
            Save_SetEventFlag(0x1E);
            Save_SetEventFlag(0x1F);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(0x21);
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(0x23) == 0 && Cond_Check(0x34) == 1) {
            gTalkEventFlag = 0x23;
            Save_SetEventFlag(0x1E);
            Save_SetEventFlag(0x1F);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(0x21);
            Save_SetEventFlag(0x22);
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(0x24) == 0 && Cond_Check(0x35) == 1) {
            gTalkEventFlag = 0x24;
            Save_SetEventFlag(0x1F);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(0x21);
            Save_SetEventFlag(0x22);
            Save_SetEventFlag(0x23);
            return 1;
        }
        break;
    case 6:
        if (Save_TestEventFlag(0x26) == 0 && Cond_Check(0x37) == 1) {
            gTalkEventFlag = 0x26;
            Save_SetEventFlag(0x1F);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(0x21);
            Save_SetEventFlag(0x22);
            Save_SetEventFlag(0x23);
            Save_SetEventFlag(0x24);
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(0x27) == 0 && Cond_Check(0x38) == 1) {
            gTalkEventFlag = 0x27;
            Save_SetEventFlag(0x1F);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(0x21);
            Save_SetEventFlag(0x22);
            Save_SetEventFlag(0x23);
            Save_SetEventFlag(0x24);
            Save_SetEventFlag(0x26);
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(0x28) == 0 && Cond_Check(0x39) == 1) {
            gTalkEventFlag = 0x28;
            Save_SetEventFlag(0x1F);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(0x21);
            Save_SetEventFlag(0x22);
            Save_SetEventFlag(0x23);
            Save_SetEventFlag(0x24);
            Save_SetEventFlag(0x26);
            Save_SetEventFlag(0x27);
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(0x3A) == 1 && Save_TestEventFlag(0x25) == 0) {
            gTalkEventFlag = 0x25;
            Save_SetEventFlag(0x28);
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(0x3B) == 1 && Save_TestEventFlag(1) == 0) {
            gTalkEventFlag = 1;
            return 1;
        }
        break;
    case 1:
        if (Cond_Check(0x3C) == 1 && Save_TestEventFlag(2) == 0) {
            gTalkEventFlag = 2;
            Save_SetEventFlag(1);
            return 1;
        }
        break;
    case 16:
        if (Village_MilestoneState(4) == 1) {
            return 1;
        }
        break;
    case 11:
        if (Cond_Check(0x3E) == 1 && Save_TestEventFlag(3) == 0) {
            gTalkEventFlag = 3;
            return 1;
        }
        break;
    case 17:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check0F(u16 k)
{
    switch (k) {
    case 2:
        if (Save_TestEventFlag(4) == 0) {
            gTalkEventFlag = 4;
            return 1;
        }
        break;
    case 1:
        if (Save_IsQuestCleared(0x1772) != 0 && Save_TestEventFlag(0x1AB) == 0) {
            gTalkEventFlag = 0x1AB;
            Save_SetEventFlag(4);
            return 1;
        }
        break;
    case 3:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check10(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0x1AC) == 0) {
            gTalkEventFlag = 0x1AC;
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(0xE) == 1 && Save_TestEventFlag(0x1AD) == 0) {
            gTalkEventFlag = 0x1AD;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(0xF) == 1 && Save_TestEventFlag(0x1AE) == 0) {
            gTalkEventFlag = 0x1AE;
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(0x13) == 1 && Save_TestEventFlag(0x1AF) == 0) {
            gTalkEventFlag = 0x1AF;
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(0x15) == 1 && Save_TestEventFlag(0x1A6) == 0) {
            gTalkEventFlag = 0x1A6;
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(0x18) == 1 && Save_TestEventFlag(0x1B0) == 0) {
            gTalkEventFlag = 0x1B0;
            Save_SetEventFlag(0x1A6);
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(0x19) == 1 && Save_TestEventFlag(0x1B1) == 0) {
            gTalkEventFlag = 0x1B1;
            return 1;
        }
        break;
    case 8:
        if (Cond_Check(0x1A) == 1 && Save_TestEventFlag(0x1B2) == 0) {
            gTalkEventFlag = 0x1B2;
            Save_SetEventFlag(0x1B1);
            return 1;
        }
        break;
    case 9:
        if (Cond_Check(0x1B) == 1 && Save_TestEventFlag(0x1B3) == 0) {
            gTalkEventFlag = 0x1B3;
            Save_SetEventFlag(0x1B2);
            Save_SetEventFlag(0x1B1);
            return 1;
        }
        break;
    case 10:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check12(u16 k)
{
    switch (k) {
    case 3:
        if (PspGift_Pick() != -1) {
            return 1;
        }
        break;
    case 1:
        if (Save_TestEventFlag(0x1B4) == 0) {
            gTalkEventFlag = 0x1B4;
            return 1;
        }
        break;
    case 2:
        if (Village_MilestoneState(3) != 0) {
            return 1;
        }
        break;
    case 4:
        if (PspLink_Count() != 0 && Save_TestEventFlag(0x1B5) == 0) {
            gTalkEventFlag = 0x1B5;
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(0x1B6) == 0) {
            gTalkEventFlag = 0x1B6;
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(0xC) == 1 && Save_TestEventFlag(0x1B7) == 0) {
            gTalkEventFlag = 0x1B7;
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(0xD) == 1 && Save_TestEventFlag(0x1B8) == 0) {
            gTalkEventFlag = 0x1B8;
            return 1;
        }
        break;
    case 9:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check13(u16 k)
{
    switch (k) {
    case 5:
        if (Save_TestEventFlag(0x16C) == 0) {
            gTalkEventFlag = 0x16C;
            return 1;
        }
        break;
    case 2:
        if ((u8)Village_FacilityLevel(0) >= 3 && Save_TestEventFlag(0x16D) == 0) {
            gTalkEventFlag = 0x16D;
            return 1;
        }
        break;
    case 1:
        if ((u8)Village_FacilityLevel(0) >= 6 && Save_TestEventFlag(0x16E) == 0) {
            gTalkEventFlag = 0x16E;
            return 1;
        }
        break;
    case 3:
        if (Town_SleepEventPending() == 1 && Save_TestEventFlag(0x16F) == 0) {
            gTalkEventFlag = 0x16F;
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(0x89) == 1 && Save_TestEventFlag(0x170) == 0) {
            gTalkEventFlag = 0x170;
            Save_SetEventFlag(0x16F);
            return 1;
        }
        break;
    case 6:
        if (*(u8 *)((u8 *)&gSave + 0x37F3) == 1) {
            return 1;
        }
        break;
    case 7:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check15(u16 k)
{
    switch (k) {
    case 3:
        if (Save_TestEventFlag(0x1B9) == 0) {
            gTalkEventFlag = 0x1B9;
            return 1;
        }
        break;
    case 2:
        if ((u8)Village_FacilityLevel(0) >= 3 && Save_TestEventFlag(0x1BA) == 0) {
            gTalkEventFlag = 0x1BA;
            return 1;
        }
        break;
    case 1:
        if ((u8)Village_FacilityLevel(0) >= 6 && Save_TestEventFlag(0x89) == 0 && Save_TestEventFlag(0x1A7) == 0) {
            gTalkEventFlag = 0x1A7;
            return 1;
        }
        break;
    case 5:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check16(u16 k)
{
    switch (k) {
    case 0x1:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check17(u16 k)
{
    switch (k) {
    case 0x1:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check18(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0x1BC) == 0) {
            gTalkEventFlag = 0x1BC;
            return 1;
        }
        break;
    case 2:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check19(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0x29) == 0) {
            gTalkEventFlag = 0x29;
            return 1;
        }
        break;
    case 2:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check1A(u16 k)
{
    switch (k) {
    case 3:
        if (Save_TestEventFlag(0x1BD) == 0) {
            gTalkEventFlag = 0x1BD;
            return 1;
        }
        break;
    case 1:
        if ((u8)Village_FacilityLevel(2) == 2 && Save_TestEventFlag(0x1BE) == 0) {
            gTalkEventFlag = 0x1BE;
            Save_SetEventFlag(0x1BD);
            return 1;
        }
        break;
    case 2:
        if (Village_MilestoneState(4) == 1) {
            return 1;
        }
        break;
    case 4:
        if (Village_Event186Pending() == 1) {
            Save_SetEventFlag(0x1BE);
            Save_SetEventFlag(0x1BD);
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check1B(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0x1BF) == 0) {
            gTalkEventFlag = 0x1BF;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check1C(u16 k)
{
    switch (k) {
    case 2:
        if (Save_TestEventFlag(0x71) == 0) {
            gTalkEventFlag = 0x71;
            return 1;
        }
        break;
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check1E(u16 k)
{
    switch (k) {
    case 3:
        if (Save_TestEventFlag(0x84) == 0) {
            gTalkEventFlag = 0x84;
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(0x85) == 0 && Cond_IsLatest(0x60) == 1) {
            gTalkEventFlag = 0x85;
            Save_SetEventFlag(0x84);
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(0x86) == 0 && Cond_Check(0x67) == 1) {
            gTalkEventFlag = 0x86;
            Save_SetEventFlag(0x85);
            Save_SetEventFlag(0x84);
            return 1;
        }
        break;
    case 2:
        if (ElderRaid_InState2() == 1) {
            return 1;
        }
        break;
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check1F(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check20(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check21(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check23(u16 k)
{
    switch (k) {
    case 2:
        if (Save_TestEventFlag(0x72) == 0) {
            gTalkEventFlag = 0x72;
            return 1;
        }
        break;
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check24(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check25(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check26(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check27(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check28(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x172) == 0) {
            gTalkEventFlag = 0x172;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(0x5F) == 1 && Save_TestEventFlag(0x173) == 0) {
            gTalkEventFlag = 0x173;
            Save_SetEventFlag(0x172);
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(0x64) == 1 && Save_TestEventFlag(0x174) == 0) {
            gTalkEventFlag = 0x174;
            Save_SetEventFlag(0x173);
            Save_SetEventFlag(0x172);
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check29(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check2A(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check2B(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check2C(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (gLobby->townEvent == 1) {
            return 1;
        }
        break;
    case 3:
        if (gLobby->townEvent == 4) {
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(0x64) == 1 && Save_TestEventFlag(0xB7) == 0) {
            gTalkEventFlag = 0xB7;
            return 1;
        }
        break;
    case 5:
        if (Combine_KnowsMostRecipes() == 1 && Save_TestEventFlag(0xB8) == 0) {
            gTalkEventFlag = 0xB8;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check2D(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (gLobby->townEvent == 3) {
            return 1;
        }
        break;
    case 3:
        if (gLobby->townEvent == 4) {
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(0x64) == 1 && Save_TestEventFlag(0xB9) == 0) {
            gTalkEventFlag = 0xB9;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check2E(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (gLobby->townEvent == 2) {
            return 1;
        }
        break;
    case 3:
        if (gLobby->townEvent == 4) {
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(0x64) == 1 && Save_TestEventFlag(0xBA) == 0) {
            gTalkEventFlag = 0xBA;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check2F(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(0x64) == 1 && Save_TestEventFlag(0x14C) == 0) {
            gTalkEventFlag = 0x14C;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check30(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0xBB) == 0) {
            gTalkEventFlag = 0xBB;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check33(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check37(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(0x59) == 1 && Save_TestEventFlag(0xBC) == 0) {
            gTalkEventFlag = 0xBC;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(0x5C) == 1 && Save_TestEventFlag(0xBD) == 0) {
            gTalkEventFlag = 0xBD;
            Save_SetEventFlag(0xBC);
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(0x64) == 1 && Save_TestEventFlag(0xBE) == 0) {
            gTalkEventFlag = 0xBE;
            Save_SetEventFlag(0xBD);
            Save_SetEventFlag(0xBC);
            return 1;
        }
        break;
    case 5:
        if (Dojo_OnlineChainCleared() == 1 && Save_TestEventFlag(0x175) == 0) {
            gTalkEventFlag = 0x175;
            return 1;
        }
        break;
    case 6:
        if (Dojo_OnlineDrillsCleared() == 1 && Save_TestEventFlag(0x176) == 0) {
            gTalkEventFlag = 0x176;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check38(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(0x59) == 1 && Save_TestEventFlag(0xC0) == 0) {
            gTalkEventFlag = 0xC0;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(0x5A) == 1 && Save_TestEventFlag(0xC1) == 0) {
            gTalkEventFlag = 0xC1;
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(0x5D) == 1 && Save_TestEventFlag(0xC2) == 0) {
            gTalkEventFlag = 0xC2;
            Save_SetEventFlag(0xC1);
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(0x62) == 1 && Save_TestEventFlag(0xC3) == 0) {
            gTalkEventFlag = 0xC3;
            Save_SetEventFlag(0xC2);
            Save_SetEventFlag(0xC1);
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check39(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x73) == 0) {
            gTalkEventFlag = 0x73;
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(0x5C) == 1 && Save_TestEventFlag(0x177) == 0) {
            gTalkEventFlag = 0x177;
            Save_SetEventFlag(0x73);
            return 1;
        }
        break;
    case 3:
        if (gLobby->townEvent == 4) {
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(0x5F) == 1 && Save_TestEventFlag(0x178) == 0) {
            gTalkEventFlag = 0x178;
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(0x60) == 1 && Save_TestEventFlag(0x179) == 0) {
            gTalkEventFlag = 0x179;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check3A(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check3B(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x87) == 0) {
            gTalkEventFlag = 0x87;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check3C(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x88) == 0) {
            gTalkEventFlag = 0x88;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check3D(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check40(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (ElderRaid_InState2() == 1) {
            return 1;
        }
        break;
    case 3:
        if (gSave.hr == 0) {
            gSave.hr = 1;
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(0x60) == 0 && Cond_IsLatest(0x5A) == 1) {
            gTalkEventFlag = 0x60;
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(0x61) == 0 && Cond_Check(0x5B) == 1) {
            gTalkEventFlag = 0x61;
            return 1;
        }
        break;
    case 6:
        if (Save_TestEventFlag(0x62) == 0 && Cond_IsLatest(0x5C) == 1 && gSave.hr < 0xC) {
            gTalkEventFlag = 0x62;
            return 1;
        }
        break;
    case 7:
        if (Save_TestEventFlag(0x63) == 0 && Cond_IsLatest(0x5D) == 1) {
            gTalkEventFlag = 0x63;
            Save_SetEventFlag(0x62);
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(0x64) == 0 && Cond_Check(0x5E) == 1) {
            gTalkEventFlag = 0x64;
            Save_SetEventFlag(0x62);
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(0x65) == 0 && Cond_IsLatest(0x5F) == 1 && gSave.hr < 0x12) {
            gTalkEventFlag = 0x65;
            Save_SetEventFlag(0x62);
            return 1;
        }
        break;
    case 10:
        if (gSave.hr >= 0x16 && Save_TestEventFlag(0x6E) == 0) {
            gTalkEventFlag = 0x6E;
            return 1;
        }
        break;
    case 11:
        if (Save_TestEventFlag(0x66) == 0 && Cond_Check(0x62) == 1) {
            Save_SetEventFlag(0x62);
            gTalkEventFlag = 0x66;
            return 1;
        }
        break;
    case 12:
        if (Save_TestEventFlag(0x67) == 0 && Cond_Check(0x63) == 1) {
            Save_SetEventFlag(0x62);
            gTalkEventFlag = 0x67;
            return 1;
        }
        break;
    case 13:
        if (Save_TestEventFlag(0x68) == 0 && Cond_Check(0x64) == 1) {
            Save_SetEventFlag(0x62);
            Save_SetEventFlag(0x68);
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check41(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0xC4) == 1 && Save_TestEventFlag(0xC5) == 0) {
            gTalkEventFlag = 0xC5;
            return 1;
        }
        break;
    case 3:
        if (Save_HasTitle(0x8D) == 1 && Save_TestEventFlag(0xC6) == 0) {
            gTalkEventFlag = 0xC6;
            return 1;
        }
        break;
    case 4:
        if (Save_HasTitle(0x8E) == 1 && Save_TestEventFlag(0xC7) == 0) {
            gTalkEventFlag = 0xC7;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check42(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0xC4) == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0xC4) == 1 && Save_TestEventFlag(0x1CA) == 0) {
            gTalkEventFlag = 0x1CA;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check45(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check46(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check47(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check49(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x13D) == 0) {
            gTalkEventFlag = 0x13D;
            return 1;
        }
        break;
    case 3:
        if (gSave.songPoints >= 0x32 && Save_TestEventFlag(0x13E) == 0) {
            gTalkEventFlag = 0x13E;
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(0x122) == 1 && Save_TestEventFlag(0x140) == 0) {
            gTalkEventFlag = 0x140;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check4A(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check4B(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check4C(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check4D(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (ElderRaid_InState2() == 1) {
            return 1;
        }
        break;
    case 3:
        if (gSave.questKills >= 0xC8 && gSave.hr >= 0x1F && Save_TestEventFlag(0xB6) == 0) {
            gTalkEventFlag = 0xB6;
            return 1;
        }
        break;
    case 4:
        if (gSave.unk2724 >= 0x14 && Save_TestEventFlag(0x14D) == 0) {
            gTalkEventFlag = 0x14D;
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(0x14E) == 0) {
            gTalkEventFlag = 0x14E;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check4E(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (ElderRaid_InState2() == 1) {
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(0x141) == 0) {
            gTalkEventFlag = 0x141;
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(0x69) == 0 && Cond_Check(0x65) == 1) {
            gTalkEventFlag = 0x69;
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(0x6A) == 0 && Cond_Check(0x66) == 1) {
            gTalkEventFlag = 0x6A;
            return 1;
        }
        break;
    case 6:
        if (Save_TestEventFlag(0x6B) == 0 && Cond_IsLatest(0x67) == 1) {
            Save_SetEventFlag(0x6B);
            return gSave.hr < 0x2A;
        }
        break;
    case 7:
        if (Save_TestEventFlag(0x6C) == 0 && Cond_Check(0x68) == 1) {
            gTalkEventFlag = 0x6C;
            return 1;
        }
        break;
    case 10:
        if (Save_TestEventFlag(0x6F) == 0 && Cond_Check(0x69) == 1) {
            gTalkEventFlag = 0x6F;
            return 1;
        }
        break;
    case 11:
        if (Save_TestEventFlag(0x70) == 0 && Cond_Check(0x6A) == 1) {
            gTalkEventFlag = 0x70;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check4F(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x142) == 0) {
            gTalkEventFlag = 0x142;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(0x65) == 1 && Save_TestEventFlag(0x143) == 0) {
            gTalkEventFlag = 0x143;
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(0x68) == 1 && Save_TestEventFlag(0x144) == 0) {
            gTalkEventFlag = 0x144;
            Save_SetEventFlag(0x143);
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check50(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x122) == 1 && Save_TestEventFlag(0x6D) == 0) {
            gTalkEventFlag = 0x6D;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check51(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check52(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check53(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check54(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check55(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x145) == 0) {
            gTalkEventFlag = 0x145;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check56(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(0x5C) == 1 && Save_TestEventFlag(0x146) == 0) {
            gTalkEventFlag = 0x146;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(0x5F) == 1 && Save_TestEventFlag(0x147) == 0) {
            gTalkEventFlag = 0x147;
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(0x64) == 1 && Save_TestEventFlag(0x149) == 0) {
            gTalkEventFlag = 0x149;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check57(u16 k)
{
    switch (k) {
    case 0x1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check58(u16 k)
{
    switch (k) {
    case 1:
        if (ElderRaid_InState1() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(0x14B) == 0) {
            gTalkEventFlag = 0x14B;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Monster_IsHunted(u16 em);
void QuestList_Rebuild(void);

s32 Talk_Check69(u16 k)
{
    switch (k) {
    case 1:
        if (Save_TestEventFlag(0x136) == 0) {
            Save_SetEventFlag(0x136);
            QuestList_Rebuild();
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(0x50) == 1 && Save_TestEventFlag(0x138) == 0) {
            gTalkEventFlag = 0x138;
            return 1;
        }
        break;
    case 3:
        if (Cond_IsLatest(0x52) == 1 && Save_TestEventFlag(0x1C0) == 0) {
            gTalkEventFlag = 0x1C0;
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(0x53) == 1 && Save_TestEventFlag(0x1C1) == 0) {
            gTalkEventFlag = 0x1C1;
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(0x54) == 1 && Save_TestEventFlag(0x1C2) == 0) {
            gTalkEventFlag = 0x1C2;
            return 1;
        }
        break;
    case 4:
        if (Monster_IsHunted(0x3B) == 1 && Save_TestEventFlag(0x1C3) == 0) {
            gTalkEventFlag = 0x1C3;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check6B(u16 k)
{
    switch (k) {
    case 2:
        if (Save_TestEventFlag(0x1C4) == 0) {
            gTalkEventFlag = 0x1C4;
            return 1;
        }
        break;
    case 1:
        if (Cond_Check(0x54) == 1 && Save_TestEventFlag(0x1C5) == 0) {
            if (Monster_IsHunted(0x3B) == 0) {
                gTalkEventFlag = 0x1C5;
                return 1;
            }
            Save_SetEventFlag(0x1C5);
        }
        break;
    }
    return 0;
}

s32 Talk_Check71(u16 k)
{
    switch (k) {
    case 2:
        if (Save_TestEventFlag(0x1C6) == 0) {
            gTalkEventFlag = 0x1C6;
            return 1;
        }
        break;
    case 3:
        if (gSave.recordCounts[33] >= 0x32 && Save_TestEventFlag(0x1CC) == 0) {
            gTalkEventFlag = 0x1CC;
            return 1;
        }
        break;
    }
    return 0;
}

s32 Talk_Check(u16 script, u16 step)
{
    switch (script) {
    case 0x1:
        return Talk_Check01(step);
    case 0x2:
        return Talk_Check02(step);
    case 0x3:
        return Talk_Check03(step);
    case 0x4:
        return Talk_Check04(step);
    case 0x5:
        return Talk_Check05(step);
    case 0x6:
        return Talk_Check06(step);
    case 0x7:
        return Talk_Check07(step);
    case 0x8:
        return Talk_Check08(step);
    case 0x9:
        return Talk_Check09(step);
    case 0xA:
        return Talk_Check0A(step);
    case 0xB:
        return Talk_Check0B(step);
    case 0xC:
        return Talk_Check0C(step);
    case 0xD:
        return Talk_Check0D(step);
    case 0xE:
        return Talk_Check0E(step);
    case 0xF:
        return Talk_Check0F(step);
    case 0x10:
        return Talk_Check10(step);
    case 0x12:
        return Talk_Check12(step);
    case 0x13:
        return Talk_Check13(step);
    case 0x15:
        return Talk_Check15(step);
    case 0x16:
        return Talk_Check16(step);
    case 0x17:
        return Talk_Check17(step);
    case 0x18:
        return Talk_Check18(step);
    case 0x19:
        return Talk_Check19(step);
    case 0x1A:
        return Talk_Check1A(step);
    case 0x1B:
        return Talk_Check1B(step);
    case 0x1C:
        return Talk_Check1C(step);
    case 0x1E:
        return Talk_Check1E(step);
    case 0x1F:
        return Talk_Check1F(step);
    case 0x20:
        return Talk_Check20(step);
    case 0x21:
        return Talk_Check21(step);
    case 0x23:
        return Talk_Check23(step);
    case 0x24:
        return Talk_Check24(step);
    case 0x25:
        return Talk_Check25(step);
    case 0x26:
        return Talk_Check26(step);
    case 0x27:
        return Talk_Check27(step);
    case 0x28:
        return Talk_Check28(step);
    case 0x29:
        return Talk_Check29(step);
    case 0x2A:
        return Talk_Check2A(step);
    case 0x2B:
        return Talk_Check2B(step);
    case 0x2C:
        return Talk_Check2C(step);
    case 0x2D:
        return Talk_Check2D(step);
    case 0x2E:
        return Talk_Check2E(step);
    case 0x2F:
        return Talk_Check2F(step);
    case 0x30:
        return Talk_Check30(step);
    case 0x33:
        return Talk_Check33(step);
    case 0x37:
        return Talk_Check37(step);
    case 0x38:
        return Talk_Check38(step);
    case 0x39:
        return Talk_Check39(step);
    case 0x3A:
        return Talk_Check3A(step);
    case 0x3B:
        return Talk_Check3B(step);
    case 0x3C:
        return Talk_Check3C(step);
    case 0x3D:
        return Talk_Check3D(step);
    case 0x40:
        return Talk_Check40(step);
    case 0x41:
        return Talk_Check41(step);
    case 0x42:
        return Talk_Check42(step);
    case 0x45:
        return Talk_Check45(step);
    case 0x46:
        return Talk_Check46(step);
    case 0x47:
        return Talk_Check47(step);
    case 0x49:
        return Talk_Check49(step);
    case 0x4A:
        return Talk_Check4A(step);
    case 0x4B:
        return Talk_Check4B(step);
    case 0x4C:
        return Talk_Check4C(step);
    case 0x4D:
        return Talk_Check4D(step);
    case 0x4E:
        return Talk_Check4E(step);
    case 0x4F:
        return Talk_Check4F(step);
    case 0x50:
        return Talk_Check50(step);
    case 0x51:
        return Talk_Check51(step);
    case 0x52:
        return Talk_Check52(step);
    case 0x53:
        return Talk_Check53(step);
    case 0x54:
        return Talk_Check54(step);
    case 0x55:
        return Talk_Check55(step);
    case 0x56:
        return Talk_Check56(step);
    case 0x57:
        return Talk_Check57(step);
    case 0x58:
        return Talk_Check58(step);
    case 0x69:
        return Talk_Check69(step);
    case 0x6B:
        return Talk_Check6B(step);
    case 0x71:
        return Talk_Check71(step);
    default:
        return 0;
    }
}

s32 Talk_CheckState(u16 k)
{
    s32 r = LobbyWorld_GetSeason();

    switch (k) {
    case 3:
        if (r == 2) {
            return 1;
        }
        break;
    case 2:
        if (r == 1) {
            return 1;
        }
        break;
    case 1:
        if (r == 0) {
            return 1;
        }
        break;
    case 0:
        return 1;
    }
    return 0;
}

s32 Talk_CheckMode(u16 k)
{
    s32 r = LobbyTime_IsPm();

    switch (k) {
    case 2:
        if (r == 1) {
            return 1;
        }
        break;
    case 1:
        if (r == 0) {
            return 1;
        }
        break;
    case 0:
        return 1;
    }
    return 0;
}

u8 *Talk_FindScript(u16 id);
/* A talk event of a townsperson kind: the event step Talk_Check tests, and the
 * talk it then plays. */
typedef struct TalkEvent {
    u16 step;
    u16 talk;
} TalkEvent;
/* An entry of the talk selection lists: a condition and what it leads to. */
typedef struct TalkSel {
    u16 id;
    void *next;
} TalkSel;
/* The block of a townsperson at +0x438. */
typedef struct NpcTalk {
    /* 0x0 */ u8 kind;
    /* 0x1 */ u8 unk1[2];
    /* 0x3 */ s8 talked;    /* talked to before (cleared by Npc_ResetTalked) */
    /* 0x4 */ u8 unk4[4];
    /* 0x8 */ s32 unk8;
} NpcTalk;
extern TalkEvent *gTalkEvents[];
extern TalkSel *gTalkSelects[];
extern s32 gTalkNpcName;
extern TalkSel *gTalkCondList;
extern TalkSel *gTalkStateList;
extern u16 *gTalkSelLines;
extern s32 gTalkTextFrames;
extern u16 gTalkScript;
extern u16 gTalkCondId;
extern u16 gTalkStateId;
extern u16 gTalkModeId;
extern s32 gTalkLine;
extern s8 gTalkMode;
s32 Village_HasGift(u16 kind);
void NpcGift_Open(Em *npc);
s32 Game_IsOnline(void);
s32 Area_IsHouse(u16 area);
s32 Village_HasMealVisit(u16 kind);
void MealScene_Open(Em *npc);
s32 MyHouse_IsOwner(void);
u16 Rand_Next(s32 stream);

void Talk_Start(Em *npc, s8 quiet)
{
    NpcTalk *m;
    TalkEvent *e;
    TalkSel *c;
    s32 i;

    gTalkTextFrames = 0;
    gTalkMode = 0;
    gTalkLine = 0;
    m = (NpcTalk *)&npc->work;
    gTalkScriptRec = Talk_FindScript(npc->work);
    gTalkNpcName = m->unk8;
    gTalkEventStep = 0xFFFF;
    gTalkEventFlag = -1;
    gTalkScript = m->kind;
    if (quiet == 0) {
        LobbySe_Play(0xE);
    }
    switch (gTownMode) {
    case 4:
    case 5:
        if (Village_HasGift(m->kind) == 1) {
            NpcGift_Open(npc);
            return;
        }
        break;
    }
    if (Game_IsOnline() == 0 && Area_IsHouse(gScene.area) == 1 && Village_HasMealVisit(m->kind) == 1) {
        MealScene_Open(npc);
        return;
    }
    if (m->kind == 0x71 && MyHouse_IsOwner() == 0) {
        if (gLobby->mealEaten != 0) {
            gTalkLine = 0x1D;
        } else {
            gTalkLine = 0x1B;
        }
        return;
    }
    e = gTalkEvents[m->kind];
    i = 0;
    while (e->step != 0xFFFF) {
        if (Talk_Check(m->kind, e->step) == 1) {
            gTalkEventStep = e->step;
            gTalkLine = e->talk;
            break;
        }
        if (++i > 100) {
            break;
        }
        e++;
    }
    if (gTalkEventStep == 0xFFFF) {
        c = gTalkSelects[m->kind];
        i = 0;
        while (c->next != NULL) {
            if (Cond_Check(c->id) == 1) {
                gTalkCondId = c->id;
                gTalkCondList = c->next;
                break;
            }
            if (++i > 1000) {
                break;
            }
            c++;
        }
        c = gTalkCondList;
        i = 0;
        while (c->next != NULL) {
            if (Talk_CheckState(c->id) == 1) {
                gTalkStateId = c->id;
                gTalkStateList = c->next;
                break;
            }
            if (++i > 1000) {
                break;
            }
            c++;
        }
        c = gTalkStateList;
        i = 0;
        while (c->next != NULL) {
            if (Talk_CheckMode(c->id) == 1) {
                gTalkModeId = c->id;
                gTalkSelLines = c->next;
                if (m->talked == 0) {
                    gTalkLine = ((u16 *)c->next)[0];
                } else {
                    gTalkLine = ((u16 *)c->next)[1];
                }
                break;
            }
            if (++i > 1000) {
                break;
            }
            c++;
        }
        if (gTalkEventStep == 0xFFFF) {
            switch (m->kind) {
            case 0x45:
            case 0x46:
            case 0x47:
                gTalkLine = (u16)Rand_Next(1) % 10;
                if (gTalkLine == 9) {
                    if (LobbyTime_IsPm() == 0) {
                        gTalkEventStep = 0x63;
                    } else {
                        gTalkLine = 0;
                    }
                }
                break;
            }
        }
        if (m->talked == 0) {
            m->talked++;
        }
    }
}

void Npc_ResetTalked(void)
{
    Em *em;
    s32 i;

    i = 0;
    em = (Em *) &gEms;
    do {
        if (em->active != 0) {
            ((NpcWork *) &em->work)->talked = 0;
        }
        i = (s16)((i + 1));
        em++;
    } while (i < 0x14);
}

u8 *Talk_FindScript(u16 id)
{
    u8 *p;
    s32 i;
    s32 rec[2];

    p = gLbguildFree;
    memcpy(rec, p, 8);
    i = 0;
    while (rec[0] != 0xFFFF) {
        if (rec[0] == id) {
            return p;
        }
        i++;
        p = gLbguildFree + i * 8;
        memcpy(rec, p, 8);
    }
    return NULL;
}

/* The talk data file (loaded at gLbguildFree by unk_7A6470.c). The inline
 * helper and the split sum below are what decomp-permuter found to match. */
static inline u8 *TalkFile(void)
{
    return gLbguildFree;
}

/* The address of step k of the current script (gTalkScriptRec: its {id, offset}
 * record in the talk data file, whose offset leads to a table of step offsets). */
s32 Talk_StepData(u16 k)
{
    s32 rec[2];
    s32 off;
    s32 pos;

    memcpy(rec, gTalkScriptRec, 8);
    pos = rec[1];
    pos += k * sizeof(s32);
    memcpy(&off, gLbguildFree + pos, 4);
    return rec[1] + ((s32)TalkFile() + off);
}

/* unk_7A8690's .data (0x80DD50-0x8193D0) */
extern u32 D_004054E0[];
extern u32 D_00840080[];
extern u32 D_008BF740[];
extern u32 D_008BF748[];
extern u32 D_008BF750[];
extern u32 D_008BF758[];
extern u32 D_008BF760[];
extern u32 D_008BF768[];
extern u32 D_008BF770[];
extern u32 D_008BF778[];
extern u32 D_008BF780[];
extern u32 D_008BF788[];
extern u32 D_008BF790[];
extern u32 D_008BF798[];
extern u32 D_008BF7A0[];
extern u32 D_008BF7A8[];
extern u32 D_008BF7B0[];
extern u32 D_008BF7B8[];
extern u32 D_008BF7C0[];
extern u32 D_008BF7C8[];
extern u32 D_008BF7D0[];
extern u32 D_008BF7D8[];
extern u32 D_008BF7E0[];
extern u32 D_008BF7E8[];
extern u32 D_008BF7F0[];
extern u32 D_008BF7F8[];
extern u32 D_008BF800[];
extern u32 D_008BF808[];
extern u32 D_008BF810[];
extern u32 D_008BF818[];
extern u32 D_008BF820[];
extern u32 D_008BF828[];
extern u32 D_008BF830[];
extern u32 D_008BF838[];
extern u32 D_008BF840[];
extern u32 D_008BF848[];
extern u32 D_008BF850[];
extern u32 D_008BF858[];
extern u32 D_008BF860[];
extern u32 D_008BF868[];
extern u32 D_008BF870[];
extern u32 D_008BF878[];
extern u32 D_008BF880[];
extern u32 D_008BF888[];
extern u32 D_008BF890[];
extern u32 D_008BF898[];
extern u32 D_008BF8A0[];
extern u32 D_008BF8A8[];
extern u32 D_008BF8B0[];
extern u32 D_008BF8B8[];
extern u32 D_008BF8C0[];
extern u32 D_008BF8C8[];
extern u32 D_008BF8D0[];
extern u32 D_008BF8D8[];
extern u32 D_008BF8E0[];
extern u32 D_008BF8E8[];
extern u32 D_008BF8F0[];
extern u32 D_008BF8F8[];
extern u32 D_008BF900[];
extern u32 D_008BF908[];
extern u32 D_008BF910[];
extern u32 D_008BF918[];
extern u32 D_008BF920[];
extern u32 D_008BF928[];
extern u32 D_008BF930[];
extern u32 D_008BF938[];
extern u32 D_008BF940[];
extern u32 D_008BF948[];
extern u32 D_008BF950[];
extern u32 D_008BF958[];
extern u32 D_008BF960[];
extern u32 D_008BF968[];
extern u32 D_008BF970[];
extern u32 D_008BF978[];
extern u32 D_008BF980[];
extern u32 D_008BF988[];
extern u32 D_008BF990[];
extern u32 D_008BF998[];
extern u32 D_008BF9A0[];
extern u32 D_008BF9A8[];
extern u32 D_008BF9B0[];
extern u32 D_008BF9B8[];
extern u32 D_008BF9C0[];
extern u32 D_008BF9C8[];
extern u32 D_008BF9D0[];
extern u32 D_008BF9D8[];
extern u32 D_008BF9E0[];
extern u32 D_008BF9E8[];
extern u32 D_008BF9F0[];
extern u32 D_008BF9F8[];
extern u32 D_008BFA00[];
extern u32 D_008BFA08[];
extern u32 D_008BFA10[];
extern u32 D_008BFA18[];
extern u32 D_008BFA20[];
extern u32 D_008BFA28[];
extern u32 D_008BFA30[];
extern u32 D_008BFA38[];
extern u32 D_008BFA40[];
extern u32 D_008BFA48[];
extern u32 D_008BFA50[];
extern u32 D_008BFA58[];
extern u32 D_008BFA60[];
extern u32 D_008BFA68[];
extern u32 D_008BFA70[];
extern u32 D_008BFA78[];
extern u32 D_008BFA80[];
extern u32 D_008BFA88[];
extern u32 D_008BFA90[];
extern u32 D_008BFA98[];
extern u32 D_008BFAA0[];
extern u32 D_008BFAA8[];
extern u32 D_008BFAB0[];
extern u32 gHeapBlocks[];
TalkEvent gTalkEvents_1[22] = {
#include "lobby/0080DD50.inc"
};
u32 gTalkSelects_1_0_1_1[2] = {
#include "lobby/0080DDA8.inc"
};
u32 gTalkSelects_1_0_1_3[2] = {
#include "lobby/0080DDB0.inc"
};
u32 gTalkSelects_1_1_1_1[2] = {
#include "lobby/0080DDB8.inc"
};
u32 gTalkSelects_1_2_1_1[2] = {
#include "lobby/0080DDC0.inc"
};
u32 gTalkSelects_1_2_3_1[2] = {
#include "lobby/0080DDC8.inc"
};
u32 gTalkSelects_1_2_5_1[2] = {
#include "lobby/0080DDD0.inc"
};
u32 gTalkSelects_1_3_1_1[2] = {
#include "lobby/0080DDD8.inc"
};
u32 gTalkSelects_1_3_3_1[2] = {
#include "lobby/0080DDE0.inc"
};
u32 gTalkSelects_1_3_5_1[2] = {
#include "lobby/0080DDE8.inc"
};
u32 gTalkSelects_1_4_1_1[2] = {
#include "lobby/0080DDF0.inc"
};
u32 gTalkSelects_1_4_1_3[2] = {
#include "lobby/0080DDF8.inc"
};
u32 gTalkSelects_1_4_3_1[2] = {
#include "lobby/0080DE00.inc"
};
u32 gTalkSelects_1_4_3_3[2] = {
#include "lobby/0080DE08.inc"
};
u32 gTalkSelects_1_4_5_1[2] = {
#include "lobby/0080DE10.inc"
};
u32 gTalkSelects_1_4_5_3[2] = {
#include "lobby/0080DE18.inc"
};
u32 gTalkSelects_1_5_1_1[2] = {
#include "lobby/0080DE20.inc"
};
u32 gTalkSelects_1_6_1_1[2] = {
#include "lobby/0080DE28.inc"
};
u32 gTalkSelects_1_7_1_1[2] = {
#include "lobby/0080DE30.inc"
};
u32 gTalkSelects_1_7_1_3[2] = {
#include "lobby/0080DE38.inc"
};
u32 gTalkSelects_1_8_1_1[2] = {
#include "lobby/0080DE40.inc"
};
u32 gTalkSelects_1_8_1_3[2] = {
#include "lobby/0080DE48.inc"
};
u32 gTalkSelects_1_8_3_1[2] = {
#include "lobby/0080DE50.inc"
};
u32 gTalkSelects_1_8_3_3[2] = {
#include "lobby/0080DE58.inc"
};
u32 gTalkSelects_1_8_5_1[2] = {
#include "lobby/0080DE60.inc"
};
u32 gTalkSelects_1_8_5_3[2] = {
#include "lobby/0080DE68.inc"
};
u32 gTalkSelects_1_9_1_1[2] = {
#include "lobby/0080DE70.inc"
};
u32 gTalkSelects_1_10_1_1[2] = {
#include "lobby/0080DE78.inc"
};
u32 gTalkSelects_1_10_1_3[2] = {
#include "lobby/0080DE80.inc"
};
u32 gTalkSelects_1_11_1_1[2] = {
#include "lobby/0080DE88.inc"
};
u32 gTalkSelects_1_11_1_3[2] = {
#include "lobby/0080DE90.inc"
};
u32 gTalkSelects_1_12_1_1[2] = {
#include "lobby/0080DE98.inc"
};
u32 gTalkSelects_1_12_1_3[2] = {
#include "lobby/0080DEA0.inc"
};
u32 gTalkSelects_1_13_1_1[2] = {
#include "lobby/0080DEA8.inc"
};
u32 gTalkSelects_1_13_1_3[2] = {
#include "lobby/0080DEB0.inc"
};
u32 gTalkSelects_1_13_3_1[2] = {
#include "lobby/0080DEB8.inc"
};
u32 gTalkSelects_1_13_3_3[2] = {
#include "lobby/0080DEC0.inc"
};
u32 gTalkSelects_1_13_5_1[2] = {
#include "lobby/0080DEC8.inc"
};
u32 gTalkSelects_1_13_5_3[2] = {
#include "lobby/0080DED0.inc"
};
u32 gTalkSelects_1_14_1_1[2] = {
#include "lobby/0080DED8.inc"
};
u32 gTalkSelects_1_14_3_1[2] = {
#include "lobby/0080DEE0.inc"
};
u32 gTalkSelects_1_14_5_1[2] = {
#include "lobby/0080DEE8.inc"
};
u32 gTalkSelects_1_15_1_1[2] = {
#include "lobby/0080DEF0.inc"
};
u32 gTalkSelects_1_15_1_3[2] = {
#include "lobby/0080DEF8.inc"
};
u32 gTalkSelects_1_15_3_1[2] = {
#include "lobby/0080DF00.inc"
};
u32 gTalkSelects_1_15_3_3[2] = {
#include "lobby/0080DF08.inc"
};
u32 gTalkSelects_1_15_5_1[2] = {
#include "lobby/0080DF10.inc"
};
u32 gTalkSelects_1_15_5_3[2] = {
#include "lobby/0080DF18.inc"
};
u32 gTalkSelects_1_16_1_1[2] = {
#include "lobby/0080DF20.inc"
};
u32 gTalkSelects_1_16_3_1[2] = {
#include "lobby/0080DF28.inc"
};
u32 gTalkSelects_1_16_5_1[2] = {
#include "lobby/0080DF30.inc"
};
u32 gTalkSelects_1_17_1_1[2] = {
#include "lobby/0080DF38.inc"
};
u32 gTalkSelects_1_17_3_1[2] = {
#include "lobby/0080DF40.inc"
};
u32 gTalkSelects_1_17_5_1[2] = {
#include "lobby/0080DF48.inc"
};
u32 gTalkSelects_1_18_1_1[2] = {
#include "lobby/0080DF50.inc"
};
u32 gTalkSelects_1_18_3_1[2] = {
#include "lobby/0080DF58.inc"
};
u32 gTalkSelects_1_18_5_1[2] = {
#include "lobby/0080DF60.inc"
};
u32 gTalkSelects_1_19_1_1[2] = {
#include "lobby/0080DF68.inc"
};
u32 gTalkSelects_1_19_1_3[2] = {
#include "lobby/0080DF70.inc"
};
u32 gTalkSelects_1_19_3_1[2] = {
#include "lobby/0080DF78.inc"
};
u32 gTalkSelects_1_19_3_3[2] = {
#include "lobby/0080DF80.inc"
};
u32 gTalkSelects_1_19_5_1[2] = {
#include "lobby/0080DF88.inc"
};
u32 gTalkSelects_1_19_5_3[2] = {
#include "lobby/0080DF90.inc"
};
u32 gTalkSelects_1_20_1_1[2] = {
#include "lobby/0080DF98.inc"
};
u32 gTalkSelects_1_20_1_3[2] = {
#include "lobby/0080DFA0.inc"
};
u32 gTalkSelects_1_21_1_1[2] = {
#include "lobby/0080DFA8.inc"
};
u32 gTalkSelects_1_21_1[2] = {
    0, (u32)gTalkSelects_1_21_1_1,
};
u32 gTalkSelects_1_21[2] = {
    0, (u32)gTalkSelects_1_21_1,
};
u32 gTalkSelects_1_20_1[4] = {
#include "lobby/0080DFC0.inc"
};
u32 gTalkSelects_1_20[4] = {
    0, (u32)gTalkSelects_1_20_1, 0, 0,
};
u32 gTalkSelects_1_19_5[4] = {
#include "lobby/0080DFE0.inc"
};
u32 gTalkSelects_1_19_3[4] = {
#include "lobby/0080DFF0.inc"
};
u32 gTalkSelects_1_19_1[4] = {
#include "lobby/0080E000.inc"
};
u32 gTalkSelects_1_19[6] = {
#include "lobby/0080E010.inc"
};
u32 gTalkSelects_1_18_5[2] = {
    0, (u32)gTalkSelects_1_18_5_1,
};
u32 gTalkSelects_1_18_3[2] = {
    0, (u32)gTalkSelects_1_18_3_1,
};
u32 gTalkSelects_1_18_1[2] = {
    0, (u32)gTalkSelects_1_18_1_1,
};
u32 gTalkSelects_1_18[6] = {
#include "lobby/0080E040.inc"
};
u32 gTalkSelects_1_17_5[2] = {
    0, (u32)gTalkSelects_1_17_5_1,
};
u32 gTalkSelects_1_17_3[2] = {
    0, (u32)gTalkSelects_1_17_3_1,
};
u32 gTalkSelects_1_17_1[2] = {
    0, (u32)gTalkSelects_1_17_1_1,
};
u32 gTalkSelects_1_17[6] = {
#include "lobby/0080E070.inc"
};
u32 gTalkSelects_1_16_5[2] = {
    0, (u32)gTalkSelects_1_16_5_1,
};
u32 gTalkSelects_1_16_3[2] = {
    0, (u32)gTalkSelects_1_16_3_1,
};
u32 gTalkSelects_1_16_1[2] = {
    0, (u32)gTalkSelects_1_16_1_1,
};
u32 gTalkSelects_1_16[8] = {
#include "lobby/0080E0A0.inc"
};
u32 gTalkSelects_1_15_5[4] = {
#include "lobby/0080E0C0.inc"
};
u32 gTalkSelects_1_15_3[4] = {
#include "lobby/0080E0D0.inc"
};
u32 gTalkSelects_1_15_1[4] = {
#include "lobby/0080E0E0.inc"
};
u32 gTalkSelects_1_15[6] = {
#include "lobby/0080E0F0.inc"
};
u32 gTalkSelects_1_14_5[2] = {
    0, (u32)gTalkSelects_1_14_5_1,
};
u32 gTalkSelects_1_14_3[2] = {
    0, (u32)gTalkSelects_1_14_3_1,
};
u32 gTalkSelects_1_14_1[2] = {
    0, (u32)gTalkSelects_1_14_1_1,
};
u32 gTalkSelects_1_14[8] = {
#include "lobby/0080E120.inc"
};
u32 gTalkSelects_1_13_5[4] = {
#include "lobby/0080E140.inc"
};
u32 gTalkSelects_1_13_3[4] = {
#include "lobby/0080E150.inc"
};
u32 gTalkSelects_1_13_1[4] = {
#include "lobby/0080E160.inc"
};
u32 gTalkSelects_1_13[8] = {
#include "lobby/0080E170.inc"
};
u32 gTalkSelects_1_12_1[4] = {
#include "lobby/0080E190.inc"
};
u32 gTalkSelects_1_12[4] = {
    0, (u32)gTalkSelects_1_12_1, 0, 0,
};
u32 gTalkSelects_1_11_1[4] = {
#include "lobby/0080E1B0.inc"
};
u32 gTalkSelects_1_11[4] = {
    0, (u32)gTalkSelects_1_11_1, 0, 0,
};
u32 gTalkSelects_1_10_1[4] = {
#include "lobby/0080E1D0.inc"
};
u32 gTalkSelects_1_10[2] = {
    0, (u32)gTalkSelects_1_10_1,
};
u32 gTalkSelects_1_9_1[2] = {
    0, (u32)gTalkSelects_1_9_1_1,
};
u32 gTalkSelects_1_9[4] = {
    0, (u32)gTalkSelects_1_9_1, 0, 0,
};
u32 gTalkSelects_1_8_5[4] = {
#include "lobby/0080E200.inc"
};
u32 gTalkSelects_1_8_3[4] = {
#include "lobby/0080E210.inc"
};
u32 gTalkSelects_1_8_1[4] = {
#include "lobby/0080E220.inc"
};
u32 gTalkSelects_1_8[8] = {
#include "lobby/0080E230.inc"
};
u32 gTalkSelects_1_7_1[4] = {
#include "lobby/0080E250.inc"
};
u32 gTalkSelects_1_7[2] = {
    0, (u32)gTalkSelects_1_7_1,
};
u32 gTalkSelects_1_6_1[2] = {
    0, (u32)gTalkSelects_1_6_1_1,
};
u32 gTalkSelects_1_6[2] = {
    0, (u32)gTalkSelects_1_6_1,
};
u32 gTalkSelects_1_5_1[2] = {
    0, (u32)gTalkSelects_1_5_1_1,
};
u32 gTalkSelects_1_5[4] = {
    0, (u32)gTalkSelects_1_5_1, 0, 0,
};
u32 gTalkSelects_1_4_5[4] = {
#include "lobby/0080E290.inc"
};
u32 gTalkSelects_1_4_3[4] = {
#include "lobby/0080E2A0.inc"
};
u32 gTalkSelects_1_4_1[4] = {
#include "lobby/0080E2B0.inc"
};
u32 gTalkSelects_1_4[6] = {
#include "lobby/0080E2C0.inc"
};
u32 gTalkSelects_1_3_5[2] = {
    0, (u32)gTalkSelects_1_3_5_1,
};
u32 gTalkSelects_1_3_3[2] = {
    0, (u32)gTalkSelects_1_3_3_1,
};
u32 gTalkSelects_1_3_1[2] = {
    0, (u32)gTalkSelects_1_3_1_1,
};
u32 gTalkSelects_1_3[6] = {
#include "lobby/0080E2F0.inc"
};
u32 gTalkSelects_1_2_5[2] = {
    0, (u32)gTalkSelects_1_2_5_1,
};
u32 gTalkSelects_1_2_3[2] = {
    0, (u32)gTalkSelects_1_2_3_1,
};
u32 gTalkSelects_1_2_1[2] = {
    0, (u32)gTalkSelects_1_2_1_1,
};
u32 gTalkSelects_1_2[6] = {
#include "lobby/0080E320.inc"
};
u32 gTalkSelects_1_1_1[2] = {
    0, (u32)gTalkSelects_1_1_1_1,
};
u32 gTalkSelects_1_1[4] = {
    0, (u32)gTalkSelects_1_1_1, 0, 0,
};
u32 gTalkSelects_1_0_1[4] = {
#include "lobby/0080E350.inc"
};
u32 gTalkSelects_1_0[4] = {
    0, (u32)gTalkSelects_1_0_1, 0, 0,
};
TalkSel gTalkSelects_1[22] = {
#include "lobby/0080E370.inc"
};
u32 D_0080E420[620] = {
#include "lobby/0080E420.inc"
};
TalkGift gTalkGifts_1[12] = {
#include "lobby/0080EDD0.inc"
};
TalkEvent gTalkEvents_2[10] = {
#include "lobby/0080EE30.inc"
};
u32 gTalkSelects_2_0_1_1[2] = {
#include "lobby/0080EE58.inc"
};
u32 gTalkSelects_2_0_1_3[2] = {
#include "lobby/0080EE60.inc"
};
u32 gTalkSelects_2_0_3_1[2] = {
#include "lobby/0080EE68.inc"
};
u32 gTalkSelects_2_0_3_3[2] = {
#include "lobby/0080EE70.inc"
};
u32 gTalkSelects_2_0_5_1[2] = {
#include "lobby/0080EE78.inc"
};
u32 gTalkSelects_2_0_5_3[2] = {
#include "lobby/0080EE80.inc"
};
u32 gTalkSelects_2_1_1_1[2] = {
#include "lobby/0080EE88.inc"
};
u32 gTalkSelects_2_1_1_3[2] = {
#include "lobby/0080EE90.inc"
};
u32 gTalkSelects_2_1_3_1[2] = {
#include "lobby/0080EE98.inc"
};
u32 gTalkSelects_2_1_3_3[2] = {
#include "lobby/0080EEA0.inc"
};
u32 gTalkSelects_2_1_5_1[2] = {
#include "lobby/0080EEA8.inc"
};
u32 gTalkSelects_2_1_5_3[2] = {
#include "lobby/0080EEB0.inc"
};
u32 gTalkSelects_2_2_1_1[2] = {
#include "lobby/0080EEB8.inc"
};
u32 gTalkSelects_2_2_1_3[2] = {
#include "lobby/0080EEC0.inc"
};
u32 gTalkSelects_2_2_3_1[2] = {
#include "lobby/0080EEC8.inc"
};
u32 gTalkSelects_2_2_3_3[2] = {
#include "lobby/0080EED0.inc"
};
u32 gTalkSelects_2_2_5_1[2] = {
#include "lobby/0080EED8.inc"
};
u32 gTalkSelects_2_2_5_3[2] = {
#include "lobby/0080EEE0.inc"
};
u32 gTalkSelects_2_3_1_1[2] = {
#include "lobby/0080EEE8.inc"
};
u32 gTalkSelects_2_3_1_3[2] = {
#include "lobby/0080EEF0.inc"
};
u32 gTalkSelects_2_3_3_1[2] = {
#include "lobby/0080EEF8.inc"
};
u32 gTalkSelects_2_3_3_3[2] = {
#include "lobby/0080EF00.inc"
};
u32 gTalkSelects_2_3_5_1[2] = {
#include "lobby/0080EF08.inc"
};
u32 gTalkSelects_2_3_5_3[2] = {
#include "lobby/0080EF10.inc"
};
u32 gTalkSelects_2_4_1_1[2] = {
#include "lobby/0080EF18.inc"
};
u32 gTalkSelects_2_4_1_3[2] = {
#include "lobby/0080EF20.inc"
};
u32 gTalkSelects_2_4_3_1[2] = {
#include "lobby/0080EF28.inc"
};
u32 gTalkSelects_2_4_3_3[2] = {
#include "lobby/0080EF30.inc"
};
u32 gTalkSelects_2_4_5_1[2] = {
#include "lobby/0080EF38.inc"
};
u32 gTalkSelects_2_4_5_3[2] = {
#include "lobby/0080EF40.inc"
};
u32 gTalkSelects_2_5_1_1[2] = {
#include "lobby/0080EF48.inc"
};
u32 gTalkSelects_2_5_1_3[2] = {
#include "lobby/0080EF50.inc"
};
u32 gTalkSelects_2_5_3_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x26E74), 0,
};
u32 gTalkSelects_2_5_3_3[2] = {
    (u32)((u8 *)gHeapBlocks + 0x16E73), 0,
};
u32 gTalkSelects_2_5_5_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x6E72), 0,
};
u32 gTalkSelects_2_5_5_3[2] = {
#include "lobby/0080EF70.inc"
};
u32 gTalkSelects_2_6_1_1[2] = {
#include "lobby/0080EF78.inc"
};
u32 gTalkSelects_2_6_1_3[2] = {
#include "lobby/0080EF80.inc"
};
u32 gTalkSelects_2_7_1_1[2] = {
#include "lobby/0080EF88.inc"
};
u32 gTalkSelects_2_7_1_3[2] = {
#include "lobby/0080EF90.inc"
};
u32 gTalkSelects_2_8_1_1[2] = {
#include "lobby/0080EF98.inc"
};
u32 gTalkSelects_2_8_1[2] = {
    0, (u32)gTalkSelects_2_8_1_1,
};
u32 gTalkSelects_2_8[2] = {
    0, (u32)gTalkSelects_2_8_1,
};
u32 gTalkSelects_2_7_1[4] = {
#include "lobby/0080EFB0.inc"
};
u32 gTalkSelects_2_7[4] = {
    0, (u32)gTalkSelects_2_7_1, 0, 0,
};
u32 gTalkSelects_2_6_1[4] = {
#include "lobby/0080EFD0.inc"
};
u32 gTalkSelects_2_6[4] = {
    0, (u32)gTalkSelects_2_6_1, 0, 0,
};
u32 gTalkSelects_2_5_5[4] = {
#include "lobby/0080EFF0.inc"
};
u32 gTalkSelects_2_5_3[4] = {
#include "lobby/0080F000.inc"
};
u32 gTalkSelects_2_5_1[4] = {
#include "lobby/0080F010.inc"
};
u32 gTalkSelects_2_5[8] = {
#include "lobby/0080F020.inc"
};
u32 gTalkSelects_2_4_5[4] = {
#include "lobby/0080F040.inc"
};
u32 gTalkSelects_2_4_3[4] = {
#include "lobby/0080F050.inc"
};
u32 gTalkSelects_2_4_1[4] = {
#include "lobby/0080F060.inc"
};
u32 gTalkSelects_2_4[8] = {
#include "lobby/0080F070.inc"
};
u32 gTalkSelects_2_3_5[4] = {
#include "lobby/0080F090.inc"
};
u32 gTalkSelects_2_3_3[4] = {
#include "lobby/0080F0A0.inc"
};
u32 gTalkSelects_2_3_1[4] = {
#include "lobby/0080F0B0.inc"
};
u32 gTalkSelects_2_3[8] = {
#include "lobby/0080F0C0.inc"
};
u32 gTalkSelects_2_2_5[4] = {
#include "lobby/0080F0E0.inc"
};
u32 gTalkSelects_2_2_3[4] = {
#include "lobby/0080F0F0.inc"
};
u32 gTalkSelects_2_2_1[4] = {
#include "lobby/0080F100.inc"
};
u32 gTalkSelects_2_2[8] = {
#include "lobby/0080F110.inc"
};
u32 gTalkSelects_2_1_5[4] = {
#include "lobby/0080F130.inc"
};
u32 gTalkSelects_2_1_3[4] = {
#include "lobby/0080F140.inc"
};
u32 gTalkSelects_2_1_1[4] = {
#include "lobby/0080F150.inc"
};
u32 gTalkSelects_2_1[8] = {
#include "lobby/0080F160.inc"
};
u32 gTalkSelects_2_0_5[4] = {
#include "lobby/0080F180.inc"
};
u32 gTalkSelects_2_0_3[4] = {
#include "lobby/0080F190.inc"
};
u32 gTalkSelects_2_0_1[4] = {
#include "lobby/0080F1A0.inc"
};
u32 gTalkSelects_2_0[8] = {
#include "lobby/0080F1B0.inc"
};
TalkSel gTalkSelects_2[10] = {
#include "lobby/0080F1D0.inc"
};
u32 D_0080F220[132] = {
#include "lobby/0080F220.inc"
};
TalkEvent gTalkEvents_3[10] = {
#include "lobby/0080F430.inc"
};
u32 gTalkSelects_3_0_1_1[2] = {
#include "lobby/0080F458.inc"
};
u32 gTalkSelects_3_0_1_3[2] = {
#include "lobby/0080F460.inc"
};
u32 gTalkSelects_3_0_3_1[2] = {
#include "lobby/0080F468.inc"
};
u32 gTalkSelects_3_0_3_3[2] = {
#include "lobby/0080F470.inc"
};
u32 gTalkSelects_3_0_5_1[2] = {
#include "lobby/0080F478.inc"
};
u32 gTalkSelects_3_0_5_3[2] = {
#include "lobby/0080F480.inc"
};
u32 gTalkSelects_3_1_1_1[2] = {
#include "lobby/0080F488.inc"
};
u32 gTalkSelects_3_1_1_3[2] = {
#include "lobby/0080F490.inc"
};
u32 gTalkSelects_3_1_3_1[2] = {
#include "lobby/0080F498.inc"
};
u32 gTalkSelects_3_1_3_3[2] = {
#include "lobby/0080F4A0.inc"
};
u32 gTalkSelects_3_1_5_1[2] = {
#include "lobby/0080F4A8.inc"
};
u32 gTalkSelects_3_1_5_3[2] = {
#include "lobby/0080F4B0.inc"
};
u32 gTalkSelects_3_2_1_1[2] = {
#include "lobby/0080F4B8.inc"
};
u32 gTalkSelects_3_2_1_3[2] = {
#include "lobby/0080F4C0.inc"
};
u32 gTalkSelects_3_3_1_1[2] = {
#include "lobby/0080F4C8.inc"
};
u32 gTalkSelects_3_3_1_3[2] = {
#include "lobby/0080F4D0.inc"
};
u32 gTalkSelects_3_3_3_1[2] = {
#include "lobby/0080F4D8.inc"
};
u32 gTalkSelects_3_3_3_3[2] = {
#include "lobby/0080F4E0.inc"
};
u32 gTalkSelects_3_3_5_1[2] = {
#include "lobby/0080F4E8.inc"
};
u32 gTalkSelects_3_3_5_3[2] = {
#include "lobby/0080F4F0.inc"
};
u32 gTalkSelects_3_4_1_1[2] = {
#include "lobby/0080F4F8.inc"
};
u32 gTalkSelects_3_4_1_3[2] = {
#include "lobby/0080F500.inc"
};
u32 gTalkSelects_3_4_3_1[2] = {
#include "lobby/0080F508.inc"
};
u32 gTalkSelects_3_4_3_3[2] = {
#include "lobby/0080F510.inc"
};
u32 gTalkSelects_3_4_5_1[2] = {
#include "lobby/0080F518.inc"
};
u32 gTalkSelects_3_4_5_3[2] = {
#include "lobby/0080F520.inc"
};
u32 gTalkSelects_3_5_1_1[2] = {
#include "lobby/0080F528.inc"
};
u32 gTalkSelects_3_5_1_3[2] = {
    (u32)((u8 *)gHeapBlocks + 0x6E72), 0,
};
u32 gTalkSelects_3_6_1_1[2] = {
#include "lobby/0080F538.inc"
};
u32 gTalkSelects_3_6_1_3[2] = {
#include "lobby/0080F540.inc"
};
u32 gTalkSelects_3_7_1_1[2] = {
#include "lobby/0080F548.inc"
};
u32 gTalkSelects_3_7_1[2] = {
    0, (u32)gTalkSelects_3_7_1_1,
};
u32 gTalkSelects_3_7[2] = {
    0, (u32)gTalkSelects_3_7_1,
};
u32 gTalkSelects_3_6_1[4] = {
#include "lobby/0080F560.inc"
};
u32 gTalkSelects_3_6[4] = {
    0, (u32)gTalkSelects_3_6_1, 0, 0,
};
u32 gTalkSelects_3_5_1[4] = {
#include "lobby/0080F580.inc"
};
u32 gTalkSelects_3_5[4] = {
    0, (u32)gTalkSelects_3_5_1, 0, 0,
};
u32 gTalkSelects_3_4_5[4] = {
#include "lobby/0080F5A0.inc"
};
u32 gTalkSelects_3_4_3[4] = {
#include "lobby/0080F5B0.inc"
};
u32 gTalkSelects_3_4_1[4] = {
#include "lobby/0080F5C0.inc"
};
u32 gTalkSelects_3_4[8] = {
#include "lobby/0080F5D0.inc"
};
u32 gTalkSelects_3_3_5[4] = {
#include "lobby/0080F5F0.inc"
};
u32 gTalkSelects_3_3_3[4] = {
#include "lobby/0080F600.inc"
};
u32 gTalkSelects_3_3_1[4] = {
#include "lobby/0080F610.inc"
};
u32 gTalkSelects_3_3[8] = {
#include "lobby/0080F620.inc"
};
u32 gTalkSelects_3_2_1[4] = {
#include "lobby/0080F640.inc"
};
u32 gTalkSelects_3_2[4] = {
    0, (u32)gTalkSelects_3_2_1, 0, 0,
};
u32 gTalkSelects_3_1_5[4] = {
#include "lobby/0080F660.inc"
};
u32 gTalkSelects_3_1_3[4] = {
#include "lobby/0080F670.inc"
};
u32 gTalkSelects_3_1_1[4] = {
#include "lobby/0080F680.inc"
};
u32 gTalkSelects_3_1[8] = {
#include "lobby/0080F690.inc"
};
u32 gTalkSelects_3_0_5[4] = {
#include "lobby/0080F6B0.inc"
};
u32 gTalkSelects_3_0_3[4] = {
#include "lobby/0080F6C0.inc"
};
u32 gTalkSelects_3_0_1[4] = {
#include "lobby/0080F6D0.inc"
};
u32 gTalkSelects_3_0[8] = {
#include "lobby/0080F6E0.inc"
};
TalkSel gTalkSelects_3[8] = {
#include "lobby/0080F700.inc"
};
u32 D_0080F740[144] = {
#include "lobby/0080F740.inc"
};
TalkGift gTalkGifts_3[4] = {
#include "lobby/0080F980.inc"
};
TalkEvent gTalkEvents_4[6] = {
#include "lobby/0080F9A0.inc"
};
u32 gTalkSelects_4_0_1_1[2] = {
#include "lobby/0080F9B8.inc"
};
u32 gTalkSelects_4_0_1_3[2] = {
#include "lobby/0080F9C0.inc"
};
u32 gTalkSelects_4_0_3_1[2] = {
#include "lobby/0080F9C8.inc"
};
u32 gTalkSelects_4_0_3_3[2] = {
#include "lobby/0080F9D0.inc"
};
u32 gTalkSelects_4_0_5_1[2] = {
#include "lobby/0080F9D8.inc"
};
u32 gTalkSelects_4_0_5_3[2] = {
#include "lobby/0080F9E0.inc"
};
u32 gTalkSelects_4_1_1_1[2] = {
#include "lobby/0080F9E8.inc"
};
u32 gTalkSelects_4_1_1_3[2] = {
#include "lobby/0080F9F0.inc"
};
u32 gTalkSelects_4_2_1_1[2] = {
#include "lobby/0080F9F8.inc"
};
u32 gTalkSelects_4_2_1_3[2] = {
#include "lobby/0080FA00.inc"
};
u32 gTalkSelects_4_3_1_1[2] = {
#include "lobby/0080FA08.inc"
};
u32 gTalkSelects_4_3_3_1[2] = {
#include "lobby/0080FA10.inc"
};
u32 gTalkSelects_4_3_5_1[2] = {
#include "lobby/0080FA18.inc"
};
u32 gTalkSelects_4_4_1_1[2] = {
#include "lobby/0080FA20.inc"
};
u32 gTalkSelects_4_4_1_3[2] = {
#include "lobby/0080FA28.inc"
};
u32 gTalkSelects_4_5_1_1[2] = {
#include "lobby/0080FA30.inc"
};
u32 gTalkSelects_4_5_3_1[2] = {
#include "lobby/0080FA38.inc"
};
u32 gTalkSelects_4_5_5_1[2] = {
#include "lobby/0080FA40.inc"
};
u32 gTalkSelects_4_6_1_1[2] = {
#include "lobby/0080FA48.inc"
};
u32 gTalkSelects_4_7_1_1[2] = {
#include "lobby/0080FA50.inc"
};
u32 gTalkSelects_4_7_1_3[2] = {
#include "lobby/0080FA58.inc"
};
u32 gTalkSelects_4_8_1_1[2] = {
#include "lobby/0080FA60.inc"
};
u32 gTalkSelects_4_8_1_3[2] = {
#include "lobby/0080FA68.inc"
};
u32 gTalkSelects_4_9_1_1[2] = {
#include "lobby/0080FA70.inc"
};
u32 gTalkSelects_4_9_1_3[2] = {
#include "lobby/0080FA78.inc"
};
u32 gTalkSelects_4_10_1_1[2] = {
#include "lobby/0080FA80.inc"
};
u32 gTalkSelects_4_10_1_3[2] = {
#include "lobby/0080FA88.inc"
};
u32 gTalkSelects_4_11_1_1[2] = {
#include "lobby/0080FA90.inc"
};
u32 gTalkSelects_4_11_1_3[2] = {
#include "lobby/0080FA98.inc"
};
u32 gTalkSelects_4_11_3_1[2] = {
#include "lobby/0080FAA0.inc"
};
u32 gTalkSelects_4_11_3_3[2] = {
#include "lobby/0080FAA8.inc"
};
u32 gTalkSelects_4_11_5_1[2] = {
#include "lobby/0080FAB0.inc"
};
u32 gTalkSelects_4_11_5_3[2] = {
#include "lobby/0080FAB8.inc"
};
u32 gTalkSelects_4_12_1_1[2] = {
#include "lobby/0080FAC0.inc"
};
u32 gTalkSelects_4_12_1_3[2] = {
#include "lobby/0080FAC8.inc"
};
u32 gTalkSelects_4_12_1[4] = {
#include "lobby/0080FAD0.inc"
};
u32 gTalkSelects_4_12[4] = {
    0, (u32)gTalkSelects_4_12_1, 0, 0,
};
u32 gTalkSelects_4_11_5[4] = {
#include "lobby/0080FAF0.inc"
};
u32 gTalkSelects_4_11_3[4] = {
#include "lobby/0080FB00.inc"
};
u32 gTalkSelects_4_11_1[4] = {
#include "lobby/0080FB10.inc"
};
u32 gTalkSelects_4_11[8] = {
#include "lobby/0080FB20.inc"
};
u32 gTalkSelects_4_10_1[4] = {
#include "lobby/0080FB40.inc"
};
u32 gTalkSelects_4_10[4] = {
    0, (u32)gTalkSelects_4_10_1, 0, 0,
};
u32 gTalkSelects_4_9_1[4] = {
#include "lobby/0080FB60.inc"
};
u32 gTalkSelects_4_9[4] = {
    0, (u32)gTalkSelects_4_9_1, 0, 0,
};
u32 gTalkSelects_4_8_1[4] = {
#include "lobby/0080FB80.inc"
};
u32 gTalkSelects_4_8[4] = {
    0, (u32)gTalkSelects_4_8_1, 0, 0,
};
u32 gTalkSelects_4_7_1[4] = {
#include "lobby/0080FBA0.inc"
};
u32 gTalkSelects_4_7[2] = {
    0, (u32)gTalkSelects_4_7_1,
};
u32 gTalkSelects_4_6_1[2] = {
    0, (u32)gTalkSelects_4_6_1_1,
};
u32 gTalkSelects_4_6[2] = {
    0, (u32)gTalkSelects_4_6_1,
};
u32 gTalkSelects_4_5_5[2] = {
    0, (u32)gTalkSelects_4_5_5_1,
};
u32 gTalkSelects_4_5_3[2] = {
    0, (u32)gTalkSelects_4_5_3_1,
};
u32 gTalkSelects_4_5_1[2] = {
    0, (u32)gTalkSelects_4_5_1_1,
};
u32 gTalkSelects_4_5[8] = {
#include "lobby/0080FBE0.inc"
};
u32 gTalkSelects_4_4_1[4] = {
#include "lobby/0080FC00.inc"
};
u32 gTalkSelects_4_4[2] = {
    0, (u32)gTalkSelects_4_4_1,
};
u32 gTalkSelects_4_3_5[2] = {
    0, (u32)gTalkSelects_4_3_5_1,
};
u32 gTalkSelects_4_3_3[2] = {
    0, (u32)gTalkSelects_4_3_3_1,
};
u32 gTalkSelects_4_3_1[2] = {
    0, (u32)gTalkSelects_4_3_1_1,
};
u32 gTalkSelects_4_3[8] = {
#include "lobby/0080FC30.inc"
};
u32 gTalkSelects_4_2_1[4] = {
#include "lobby/0080FC50.inc"
};
u32 gTalkSelects_4_2[4] = {
    0, (u32)gTalkSelects_4_2_1, 0, 0,
};
u32 gTalkSelects_4_1_1[4] = {
#include "lobby/0080FC70.inc"
};
u32 gTalkSelects_4_1[4] = {
    0, (u32)gTalkSelects_4_1_1, 0, 0,
};
u32 gTalkSelects_4_0_5[4] = {
#include "lobby/0080FC90.inc"
};
u32 gTalkSelects_4_0_3[4] = {
#include "lobby/0080FCA0.inc"
};
u32 gTalkSelects_4_0_1[4] = {
#include "lobby/0080FCB0.inc"
};
u32 gTalkSelects_4_0[8] = {
#include "lobby/0080FCC0.inc"
};
TalkSel gTalkSelects_4[14] = {
#include "lobby/0080FCE0.inc"
};
u32 D_0080FD50[252] = {
#include "lobby/0080FD50.inc"
};
TalkGift gTalkGifts_4[2] = {
#include "lobby/00810140.inc"
};
TalkEvent gTalkEvents_5[16] = {
#include "lobby/00810150.inc"
};
u32 gTalkSelects_5_0_1_1[2] = {
#include "lobby/00810190.inc"
};
u32 gTalkSelects_5_0_1_3[2] = {
#include "lobby/00810198.inc"
};
u32 gTalkSelects_5_0_3_1[2] = {
#include "lobby/008101A0.inc"
};
u32 gTalkSelects_5_0_3_3[2] = {
#include "lobby/008101A8.inc"
};
u32 gTalkSelects_5_0_5_1[2] = {
#include "lobby/008101B0.inc"
};
u32 gTalkSelects_5_0_5_3[2] = {
#include "lobby/008101B8.inc"
};
u32 gTalkSelects_5_1_1_1[2] = {
#include "lobby/008101C0.inc"
};
u32 gTalkSelects_5_1_1_3[2] = {
#include "lobby/008101C8.inc"
};
u32 gTalkSelects_5_2_1_1[2] = {
#include "lobby/008101D0.inc"
};
u32 gTalkSelects_5_2_1_3[2] = {
#include "lobby/008101D8.inc"
};
u32 gTalkSelects_5_3_1_1[2] = {
#include "lobby/008101E0.inc"
};
u32 gTalkSelects_5_3_1_3[2] = {
#include "lobby/008101E8.inc"
};
u32 gTalkSelects_5_4_1_1[2] = {
#include "lobby/008101F0.inc"
};
u32 gTalkSelects_5_4_1_3[2] = {
#include "lobby/008101F8.inc"
};
u32 gTalkSelects_5_5_1_1[2] = {
#include "lobby/00810200.inc"
};
u32 gTalkSelects_5_5_1_3[2] = {
#include "lobby/00810208.inc"
};
u32 gTalkSelects_5_6_1_1[2] = {
#include "lobby/00810210.inc"
};
u32 gTalkSelects_5_6_1_3[2] = {
#include "lobby/00810218.inc"
};
u32 gTalkSelects_5_7_1_1[2] = {
#include "lobby/00810220.inc"
};
u32 gTalkSelects_5_7_1_3[2] = {
#include "lobby/00810228.inc"
};
u32 gTalkSelects_5_8_1_1[2] = {
#include "lobby/00810230.inc"
};
u32 gTalkSelects_5_8_1_3[2] = {
#include "lobby/00810238.inc"
};
u32 gTalkSelects_5_9_1_1[2] = {
#include "lobby/00810240.inc"
};
u32 gTalkSelects_5_9_1_3[2] = {
#include "lobby/00810248.inc"
};
u32 gTalkSelects_5_10_1_1[2] = {
#include "lobby/00810250.inc"
};
u32 gTalkSelects_5_10_1_3[2] = {
#include "lobby/00810258.inc"
};
u32 gTalkSelects_5_10_3_1[2] = {
#include "lobby/00810260.inc"
};
u32 gTalkSelects_5_10_3_3[2] = {
#include "lobby/00810268.inc"
};
u32 gTalkSelects_5_10_5_1[2] = {
#include "lobby/00810270.inc"
};
u32 gTalkSelects_5_10_5_3[2] = {
#include "lobby/00810278.inc"
};
u32 gTalkSelects_5_11_1_1[2] = {
#include "lobby/00810280.inc"
};
u32 gTalkSelects_5_11_1_3[2] = {
#include "lobby/00810288.inc"
};
u32 gTalkSelects_5_12_1_1[2] = {
#include "lobby/00810290.inc"
};
u32 gTalkSelects_5_12_1_3[2] = {
#include "lobby/00810298.inc"
};
u32 gTalkSelects_5_13_1_1[2] = {
#include "lobby/008102A0.inc"
};
u32 gTalkSelects_5_13_1[2] = {
    0, (u32)gTalkSelects_5_13_1_1,
};
u32 gTalkSelects_5_13[4] = {
    0, (u32)gTalkSelects_5_13_1, 0, 0,
};
u32 gTalkSelects_5_12_1[4] = {
#include "lobby/008102C0.inc"
};
u32 gTalkSelects_5_12[4] = {
    0, (u32)gTalkSelects_5_12_1, 0, 0,
};
u32 gTalkSelects_5_11_1[4] = {
#include "lobby/008102E0.inc"
};
u32 gTalkSelects_5_11[4] = {
    0, (u32)gTalkSelects_5_11_1, 0, 0,
};
u32 gTalkSelects_5_10_5[4] = {
#include "lobby/00810300.inc"
};
u32 gTalkSelects_5_10_3[4] = {
#include "lobby/00810310.inc"
};
u32 gTalkSelects_5_10_1[4] = {
#include "lobby/00810320.inc"
};
u32 gTalkSelects_5_10[8] = {
#include "lobby/00810330.inc"
};
u32 gTalkSelects_5_9_1[4] = {
#include "lobby/00810350.inc"
};
u32 gTalkSelects_5_9[4] = {
    0, (u32)gTalkSelects_5_9_1, 0, 0,
};
u32 gTalkSelects_5_8_1[4] = {
#include "lobby/00810370.inc"
};
u32 gTalkSelects_5_8[4] = {
    0, (u32)gTalkSelects_5_8_1, 0, 0,
};
u32 gTalkSelects_5_7_1[4] = {
#include "lobby/00810390.inc"
};
u32 gTalkSelects_5_7[4] = {
    0, (u32)gTalkSelects_5_7_1, 0, 0,
};
u32 gTalkSelects_5_6_1[4] = {
#include "lobby/008103B0.inc"
};
u32 gTalkSelects_5_6[4] = {
    0, (u32)gTalkSelects_5_6_1, 0, 0,
};
u32 gTalkSelects_5_5_1[4] = {
#include "lobby/008103D0.inc"
};
u32 gTalkSelects_5_5[4] = {
    0, (u32)gTalkSelects_5_5_1, 0, 0,
};
u32 gTalkSelects_5_4_1[4] = {
#include "lobby/008103F0.inc"
};
u32 gTalkSelects_5_4[4] = {
    0, (u32)gTalkSelects_5_4_1, 0, 0,
};
u32 gTalkSelects_5_3_1[4] = {
#include "lobby/00810410.inc"
};
u32 gTalkSelects_5_3[4] = {
    0, (u32)gTalkSelects_5_3_1, 0, 0,
};
u32 gTalkSelects_5_2_1[4] = {
#include "lobby/00810430.inc"
};
u32 gTalkSelects_5_2[4] = {
    0, (u32)gTalkSelects_5_2_1, 0, 0,
};
u32 gTalkSelects_5_1_1[4] = {
#include "lobby/00810450.inc"
};
u32 gTalkSelects_5_1[4] = {
    0, (u32)gTalkSelects_5_1_1, 0, 0,
};
u32 gTalkSelects_5_0_5[4] = {
#include "lobby/00810470.inc"
};
u32 gTalkSelects_5_0_3[4] = {
#include "lobby/00810480.inc"
};
u32 gTalkSelects_5_0_1[4] = {
#include "lobby/00810490.inc"
};
u32 gTalkSelects_5_0[8] = {
#include "lobby/008104A0.inc"
};
TalkSel gTalkSelects_5[14] = {
#include "lobby/008104C0.inc"
};
u32 D_00810530[336] = {
#include "lobby/00810530.inc"
};
TalkGift gTalkGifts_5[6] = {
#include "lobby/00810A70.inc"
};
TalkEvent gTalkEvents_6[2] = {
#include "lobby/00810AA0.inc"
};
u32 gTalkSelects_6_0_1_1[2] = {
#include "lobby/00810AA8.inc"
};
u32 gTalkSelects_6_0_3_1[2] = {
#include "lobby/00810AB0.inc"
};
u32 gTalkSelects_6_0_5_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x6E6F), 0,
};
u32 gTalkSelects_6_1_1_1[2] = {
#include "lobby/00810AC0.inc"
};
u32 gTalkSelects_6_1_3_1[2] = {
#include "lobby/00810AC8.inc"
};
u32 gTalkSelects_6_1_5_1[2] = {
#include "lobby/00810AD0.inc"
};
u32 gTalkSelects_6_2_1_1[2] = {
#include "lobby/00810AD8.inc"
};
u32 gTalkSelects_6_3_1_1[2] = {
#include "lobby/00810AE0.inc"
};
u32 gTalkSelects_6_3_3_1[2] = {
#include "lobby/00810AE8.inc"
};
u32 gTalkSelects_6_3_5_1[2] = {
#include "lobby/00810AF0.inc"
};
u32 gTalkSelects_6_3_5[2] = {
    0, (u32)gTalkSelects_6_3_5_1,
};
u32 gTalkSelects_6_3_3[2] = {
    0, (u32)gTalkSelects_6_3_3_1,
};
u32 gTalkSelects_6_3_1[2] = {
    0, (u32)gTalkSelects_6_3_1_1,
};
u32 gTalkSelects_6_3[6] = {
#include "lobby/00810B10.inc"
};
u32 gTalkSelects_6_2_1[2] = {
    0, (u32)gTalkSelects_6_2_1_1,
};
u32 gTalkSelects_6_2[2] = {
    0, (u32)gTalkSelects_6_2_1,
};
u32 gTalkSelects_6_1_5[2] = {
    0, (u32)gTalkSelects_6_1_5_1,
};
u32 gTalkSelects_6_1_3[2] = {
    0, (u32)gTalkSelects_6_1_3_1,
};
u32 gTalkSelects_6_1_1[2] = {
    0, (u32)gTalkSelects_6_1_1_1,
};
u32 gTalkSelects_6_1[6] = {
#include "lobby/00810B50.inc"
};
u32 gTalkSelects_6_0_5[2] = {
    0, (u32)gTalkSelects_6_0_5_1,
};
u32 gTalkSelects_6_0_3[2] = {
    0, (u32)gTalkSelects_6_0_3_1,
};
u32 gTalkSelects_6_0_1[2] = {
    0, (u32)gTalkSelects_6_0_1_1,
};
u32 gTalkSelects_6_0[8] = {
#include "lobby/00810B80.inc"
};
TalkSel gTalkSelects_6[4] = {
#include "lobby/00810BA0.inc"
};
u32 D_00810BC0[52] = {
#include "lobby/00810BC0.inc"
};
TalkEvent gTalkEvents_7[12] = {
#include "lobby/00810C90.inc"
};
u32 gTalkSelects_7_0_1_1[2] = {
#include "lobby/00810CC0.inc"
};
u32 gTalkSelects_7_0_1_3[2] = {
#include "lobby/00810CC8.inc"
};
u32 gTalkSelects_7_0_3_1[2] = {
#include "lobby/00810CD0.inc"
};
u32 gTalkSelects_7_0_3_3[2] = {
#include "lobby/00810CD8.inc"
};
u32 gTalkSelects_7_0_5_1[2] = {
#include "lobby/00810CE0.inc"
};
u32 gTalkSelects_7_0_5_3[2] = {
#include "lobby/00810CE8.inc"
};
u32 gTalkSelects_7_1_1_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x26E74), 0,
};
u32 gTalkSelects_7_1_1_3[2] = {
    (u32)((u8 *)gHeapBlocks + 0x16E73), 0,
};
u32 gTalkSelects_7_2_1_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x6E72), 0,
};
u32 gTalkSelects_7_2_1_3[2] = {
#include "lobby/00810D08.inc"
};
u32 gTalkSelects_7_3_1_1[2] = {
#include "lobby/00810D10.inc"
};
u32 gTalkSelects_7_3_1_3[2] = {
#include "lobby/00810D18.inc"
};
u32 gTalkSelects_7_3_3_1[2] = {
#include "lobby/00810D20.inc"
};
u32 gTalkSelects_7_3_3_3[2] = {
#include "lobby/00810D28.inc"
};
u32 gTalkSelects_7_3_5_1[2] = {
#include "lobby/00810D30.inc"
};
u32 gTalkSelects_7_3_5_3[2] = {
#include "lobby/00810D38.inc"
};
u32 gTalkSelects_7_4_1_1[2] = {
#include "lobby/00810D40.inc"
};
u32 gTalkSelects_7_4_1_3[2] = {
#include "lobby/00810D48.inc"
};
u32 gTalkSelects_7_5_1_1[2] = {
#include "lobby/00810D50.inc"
};
u32 gTalkSelects_7_5_1_3[2] = {
#include "lobby/00810D58.inc"
};
u32 gTalkSelects_7_6_1_1[2] = {
#include "lobby/00810D60.inc"
};
u32 gTalkSelects_7_6_1_3[2] = {
#include "lobby/00810D68.inc"
};
u32 gTalkSelects_7_6_3_1[2] = {
#include "lobby/00810D70.inc"
};
u32 gTalkSelects_7_6_3_3[2] = {
#include "lobby/00810D78.inc"
};
u32 gTalkSelects_7_6_5_1[2] = {
#include "lobby/00810D80.inc"
};
u32 gTalkSelects_7_6_5_3[2] = {
#include "lobby/00810D88.inc"
};
u32 gTalkSelects_7_7_1_1[2] = {
#include "lobby/00810D90.inc"
};
u32 gTalkSelects_7_7_1_3[2] = {
#include "lobby/00810D98.inc"
};
u32 gTalkSelects_7_7_3_1[2] = {
#include "lobby/00810DA0.inc"
};
u32 gTalkSelects_7_7_3_3[2] = {
#include "lobby/00810DA8.inc"
};
u32 gTalkSelects_7_7_5_1[2] = {
#include "lobby/00810DB0.inc"
};
u32 gTalkSelects_7_7_5_3[2] = {
#include "lobby/00810DB8.inc"
};
u32 gTalkSelects_7_8_1_1[2] = {
#include "lobby/00810DC0.inc"
};
u32 gTalkSelects_7_8_1[2] = {
    0, (u32)gTalkSelects_7_8_1_1,
};
u32 gTalkSelects_7_8[4] = {
    0, (u32)gTalkSelects_7_8_1, 0, 0,
};
u32 gTalkSelects_7_7_5[4] = {
#include "lobby/00810DE0.inc"
};
u32 gTalkSelects_7_7_3[4] = {
#include "lobby/00810DF0.inc"
};
u32 gTalkSelects_7_7_1[4] = {
#include "lobby/00810E00.inc"
};
u32 gTalkSelects_7_7[8] = {
#include "lobby/00810E10.inc"
};
u32 gTalkSelects_7_6_5[4] = {
#include "lobby/00810E30.inc"
};
u32 gTalkSelects_7_6_3[4] = {
#include "lobby/00810E40.inc"
};
u32 gTalkSelects_7_6_1[4] = {
#include "lobby/00810E50.inc"
};
u32 gTalkSelects_7_6[8] = {
#include "lobby/00810E60.inc"
};
u32 gTalkSelects_7_5_1[4] = {
#include "lobby/00810E80.inc"
};
u32 gTalkSelects_7_5[4] = {
    0, (u32)gTalkSelects_7_5_1, 0, 0,
};
u32 gTalkSelects_7_4_1[4] = {
#include "lobby/00810EA0.inc"
};
u32 gTalkSelects_7_4[4] = {
    0, (u32)gTalkSelects_7_4_1, 0, 0,
};
u32 gTalkSelects_7_3_5[4] = {
#include "lobby/00810EC0.inc"
};
u32 gTalkSelects_7_3_3[4] = {
#include "lobby/00810ED0.inc"
};
u32 gTalkSelects_7_3_1[4] = {
#include "lobby/00810EE0.inc"
};
u32 gTalkSelects_7_3[8] = {
#include "lobby/00810EF0.inc"
};
u32 gTalkSelects_7_2_1[4] = {
#include "lobby/00810F10.inc"
};
u32 gTalkSelects_7_2[4] = {
    0, (u32)gTalkSelects_7_2_1, 0, 0,
};
u32 gTalkSelects_7_1_1[4] = {
#include "lobby/00810F30.inc"
};
u32 gTalkSelects_7_1[4] = {
    0, (u32)gTalkSelects_7_1_1, 0, 0,
};
u32 gTalkSelects_7_0_5[4] = {
#include "lobby/00810F50.inc"
};
u32 gTalkSelects_7_0_3[4] = {
#include "lobby/00810F60.inc"
};
u32 gTalkSelects_7_0_1[4] = {
#include "lobby/00810F70.inc"
};
u32 gTalkSelects_7_0[8] = {
#include "lobby/00810F80.inc"
};
TalkSel gTalkSelects_7[10] = {
#include "lobby/00810FA0.inc"
};
u32 D_00810FF0[88] = {
#include "lobby/00810FF0.inc"
};
TalkGift gTalkGifts_7[2] = {
#include "lobby/00811150.inc"
};
TalkEvent gTalkEvents_8[12] = {
#include "lobby/00811160.inc"
};
u32 gTalkSelects_8_0_1_1[2] = {
#include "lobby/00811190.inc"
};
u32 gTalkSelects_8_0_1_3[2] = {
#include "lobby/00811198.inc"
};
u32 gTalkSelects_8_0_3_1[2] = {
#include "lobby/008111A0.inc"
};
u32 gTalkSelects_8_0_3_3[2] = {
#include "lobby/008111A8.inc"
};
u32 gTalkSelects_8_0_5_1[2] = {
#include "lobby/008111B0.inc"
};
u32 gTalkSelects_8_0_5_3[2] = {
#include "lobby/008111B8.inc"
};
u32 gTalkSelects_8_1_1_1[2] = {
#include "lobby/008111C0.inc"
};
u32 gTalkSelects_8_1_1_3[2] = {
#include "lobby/008111C8.inc"
};
u32 gTalkSelects_8_1_3_1[2] = {
#include "lobby/008111D0.inc"
};
u32 gTalkSelects_8_1_3_3[2] = {
#include "lobby/008111D8.inc"
};
u32 gTalkSelects_8_1_5_1[2] = {
#include "lobby/008111E0.inc"
};
u32 gTalkSelects_8_1_5_3[2] = {
#include "lobby/008111E8.inc"
};
u32 gTalkSelects_8_2_1_1[2] = {
#include "lobby/008111F0.inc"
};
u32 gTalkSelects_8_3_1_1[2] = {
#include "lobby/008111F8.inc"
};
u32 gTalkSelects_8_3_1_3[2] = {
#include "lobby/00811200.inc"
};
u32 gTalkSelects_8_3_3_1[2] = {
#include "lobby/00811208.inc"
};
u32 gTalkSelects_8_3_3_3[2] = {
#include "lobby/00811210.inc"
};
u32 gTalkSelects_8_3_5_1[2] = {
#include "lobby/00811218.inc"
};
u32 gTalkSelects_8_3_5_3[2] = {
#include "lobby/00811220.inc"
};
u32 gTalkSelects_8_4_1_1[2] = {
#include "lobby/00811228.inc"
};
u32 gTalkSelects_8_4_1_3[2] = {
    (u32)((u8 *)gHeapBlocks + 0x26E74), 0,
};
u32 gTalkSelects_8_4_3_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x16E73), 0,
};
u32 gTalkSelects_8_4_3_3[2] = {
    (u32)((u8 *)gHeapBlocks + 0x6E72), 0,
};
u32 gTalkSelects_8_4_5_1[2] = {
#include "lobby/00811248.inc"
};
u32 gTalkSelects_8_4_5_3[2] = {
#include "lobby/00811250.inc"
};
u32 gTalkSelects_8_5_1_1[2] = {
#include "lobby/00811258.inc"
};
u32 gTalkSelects_8_5_1_3[2] = {
#include "lobby/00811260.inc"
};
u32 gTalkSelects_8_5_3_1[2] = {
#include "lobby/00811268.inc"
};
u32 gTalkSelects_8_5_3_3[2] = {
#include "lobby/00811270.inc"
};
u32 gTalkSelects_8_5_5_1[2] = {
#include "lobby/00811278.inc"
};
u32 gTalkSelects_8_5_5_3[2] = {
#include "lobby/00811280.inc"
};
u32 gTalkSelects_8_6_1_1[2] = {
#include "lobby/00811288.inc"
};
u32 gTalkSelects_8_6_1_3[2] = {
#include "lobby/00811290.inc"
};
u32 gTalkSelects_8_6_3_1[2] = {
#include "lobby/00811298.inc"
};
u32 gTalkSelects_8_6_3_3[2] = {
#include "lobby/008112A0.inc"
};
u32 gTalkSelects_8_6_5_1[2] = {
#include "lobby/008112A8.inc"
};
u32 gTalkSelects_8_6_5_3[2] = {
#include "lobby/008112B0.inc"
};
u32 gTalkSelects_8_7_1_1[2] = {
#include "lobby/008112B8.inc"
};
u32 gTalkSelects_8_8_1_1[2] = {
#include "lobby/008112C0.inc"
};
u32 gTalkSelects_8_8_1[2] = {
    0, (u32)gTalkSelects_8_8_1_1,
};
u32 gTalkSelects_8_8[2] = {
    0, (u32)gTalkSelects_8_8_1,
};
u32 gTalkSelects_8_7_1[2] = {
    0, (u32)gTalkSelects_8_7_1_1,
};
u32 gTalkSelects_8_7[4] = {
    0, (u32)gTalkSelects_8_7_1, 0, 0,
};
u32 gTalkSelects_8_6_5[4] = {
#include "lobby/008112F0.inc"
};
u32 gTalkSelects_8_6_3[4] = {
#include "lobby/00811300.inc"
};
u32 gTalkSelects_8_6_1[4] = {
#include "lobby/00811310.inc"
};
u32 gTalkSelects_8_6[8] = {
#include "lobby/00811320.inc"
};
u32 gTalkSelects_8_5_5[4] = {
#include "lobby/00811340.inc"
};
u32 gTalkSelects_8_5_3[4] = {
#include "lobby/00811350.inc"
};
u32 gTalkSelects_8_5_1[4] = {
#include "lobby/00811360.inc"
};
u32 gTalkSelects_8_5[8] = {
#include "lobby/00811370.inc"
};
u32 gTalkSelects_8_4_5[4] = {
#include "lobby/00811390.inc"
};
u32 gTalkSelects_8_4_3[4] = {
#include "lobby/008113A0.inc"
};
u32 gTalkSelects_8_4_1[4] = {
#include "lobby/008113B0.inc"
};
u32 gTalkSelects_8_4[8] = {
#include "lobby/008113C0.inc"
};
u32 gTalkSelects_8_3_5[4] = {
#include "lobby/008113E0.inc"
};
u32 gTalkSelects_8_3_3[4] = {
#include "lobby/008113F0.inc"
};
u32 gTalkSelects_8_3_1[4] = {
#include "lobby/00811400.inc"
};
u32 gTalkSelects_8_3[6] = {
#include "lobby/00811410.inc"
};
u32 gTalkSelects_8_2_1[2] = {
    0, (u32)gTalkSelects_8_2_1_1,
};
u32 gTalkSelects_8_2[4] = {
    0, (u32)gTalkSelects_8_2_1, 0, 0,
};
u32 gTalkSelects_8_1_5[4] = {
#include "lobby/00811440.inc"
};
u32 gTalkSelects_8_1_3[4] = {
#include "lobby/00811450.inc"
};
u32 gTalkSelects_8_1_1[4] = {
#include "lobby/00811460.inc"
};
u32 gTalkSelects_8_1[8] = {
#include "lobby/00811470.inc"
};
u32 gTalkSelects_8_0_5[4] = {
#include "lobby/00811490.inc"
};
u32 gTalkSelects_8_0_3[4] = {
#include "lobby/008114A0.inc"
};
u32 gTalkSelects_8_0_1[4] = {
#include "lobby/008114B0.inc"
};
u32 gTalkSelects_8_0[8] = {
#include "lobby/008114C0.inc"
};
TalkSel gTalkSelects_8[10] = {
#include "lobby/008114E0.inc"
};
u32 D_00811530[116] = {
#include "lobby/00811530.inc"
};
TalkGift gTalkGifts_8[3] = {
#include "lobby/00811700.inc"
};
TalkEvent gTalkEvents_9[2] = {
#include "lobby/00811718.inc"
};
u32 gTalkSelects_9_0_1_1[2] = {
#include "lobby/00811720.inc"
};
u32 gTalkSelects_9_0_3_1[2] = {
#include "lobby/00811728.inc"
};
u32 gTalkSelects_9_0_5_1[2] = {
#include "lobby/00811730.inc"
};
u32 gTalkSelects_9_1_1_1[2] = {
#include "lobby/00811738.inc"
};
u32 gTalkSelects_9_1_3_1[2] = {
#include "lobby/00811740.inc"
};
u32 gTalkSelects_9_1_5_1[2] = {
#include "lobby/00811748.inc"
};
u32 gTalkSelects_9_2_1_1[2] = {
#include "lobby/00811750.inc"
};
u32 gTalkSelects_9_2_3_1[2] = {
#include "lobby/00811758.inc"
};
u32 gTalkSelects_9_2_5_1[2] = {
#include "lobby/00811760.inc"
};
u32 gTalkSelects_9_3_1_1[2] = {
#include "lobby/00811768.inc"
};
u32 gTalkSelects_9_3_3_1[2] = {
#include "lobby/00811770.inc"
};
u32 gTalkSelects_9_3_5_1[2] = {
#include "lobby/00811778.inc"
};
u32 gTalkSelects_9_4_1_1[2] = {
#include "lobby/00811780.inc"
};
u32 gTalkSelects_9_4_3_1[2] = {
#include "lobby/00811788.inc"
};
u32 gTalkSelects_9_4_5_1[2] = {
#include "lobby/00811790.inc"
};
u32 gTalkSelects_9_5_1_1[2] = {
#include "lobby/00811798.inc"
};
u32 gTalkSelects_9_5_1_3[2] = {
#include "lobby/008117A0.inc"
};
u32 gTalkSelects_9_5_3_1[2] = {
    (u32)((u8 *)D_004054E0 + 0xAB5C), 0,
};
u32 gTalkSelects_9_5_3_3[2] = {
#include "lobby/008117B0.inc"
};
u32 gTalkSelects_9_5_5_1[2] = {
#include "lobby/008117B8.inc"
};
u32 gTalkSelects_9_5_5_3[2] = {
#include "lobby/008117C0.inc"
};
u32 gTalkSelects_9_6_1_1[2] = {
#include "lobby/008117C8.inc"
};
u32 gTalkSelects_9_6_1_3[2] = {
#include "lobby/008117D0.inc"
};
u32 gTalkSelects_9_7_1_1[2] = {
#include "lobby/008117D8.inc"
};
u32 gTalkSelects_9_7_1_3[2] = {
#include "lobby/008117E0.inc"
};
u32 gTalkSelects_9_8_1_1[2] = {
#include "lobby/008117E8.inc"
};
u32 gTalkSelects_9_8_1_3[4] = {
#include "lobby/008117F0.inc"
};
u32 gTalkSelects_9_8_1[4] = {
#include "lobby/00811800.inc"
};
u32 gTalkSelects_9_8[4] = {
    0, (u32)gTalkSelects_9_8_1, 0, 0,
};
u32 gTalkSelects_9_7_1[4] = {
#include "lobby/00811820.inc"
};
u32 gTalkSelects_9_7[4] = {
    0, (u32)gTalkSelects_9_7_1, 0, 0,
};
u32 gTalkSelects_9_6_1[4] = {
#include "lobby/00811840.inc"
};
u32 gTalkSelects_9_6[4] = {
    0, (u32)gTalkSelects_9_6_1, 0, 0,
};
u32 gTalkSelects_9_5_5[4] = {
#include "lobby/00811860.inc"
};
u32 gTalkSelects_9_5_3[4] = {
#include "lobby/00811870.inc"
};
u32 gTalkSelects_9_5_1[4] = {
#include "lobby/00811880.inc"
};
u32 gTalkSelects_9_5[6] = {
#include "lobby/00811890.inc"
};
u32 gTalkSelects_9_4_5[2] = {
    0, (u32)gTalkSelects_9_4_5_1,
};
u32 gTalkSelects_9_4_3[2] = {
    0, (u32)gTalkSelects_9_4_3_1,
};
u32 gTalkSelects_9_4_1[2] = {
    0, (u32)gTalkSelects_9_4_1_1,
};
u32 gTalkSelects_9_4[6] = {
#include "lobby/008118C0.inc"
};
u32 gTalkSelects_9_3_5[2] = {
    0, (u32)gTalkSelects_9_3_5_1,
};
u32 gTalkSelects_9_3_3[2] = {
    0, (u32)gTalkSelects_9_3_3_1,
};
u32 gTalkSelects_9_3_1[2] = {
    0, (u32)gTalkSelects_9_3_1_1,
};
u32 gTalkSelects_9_3[6] = {
#include "lobby/008118F0.inc"
};
u32 gTalkSelects_9_2_5[2] = {
    0, (u32)gTalkSelects_9_2_5_1,
};
u32 gTalkSelects_9_2_3[2] = {
    0, (u32)gTalkSelects_9_2_3_1,
};
u32 gTalkSelects_9_2_1[2] = {
    0, (u32)gTalkSelects_9_2_1_1,
};
u32 gTalkSelects_9_2[6] = {
#include "lobby/00811920.inc"
};
u32 gTalkSelects_9_1_5[2] = {
    0, (u32)gTalkSelects_9_1_5_1,
};
u32 gTalkSelects_9_1_3[2] = {
    0, (u32)gTalkSelects_9_1_3_1,
};
u32 gTalkSelects_9_1_1[2] = {
    0, (u32)gTalkSelects_9_1_1_1,
};
u32 gTalkSelects_9_1[6] = {
#include "lobby/00811950.inc"
};
u32 gTalkSelects_9_0_5[2] = {
    0, (u32)gTalkSelects_9_0_5_1,
};
u32 gTalkSelects_9_0_3[2] = {
    0, (u32)gTalkSelects_9_0_3_1,
};
u32 gTalkSelects_9_0_1[2] = {
    0, (u32)gTalkSelects_9_0_1_1,
};
u32 gTalkSelects_9_0[8] = {
#include "lobby/00811980.inc"
};
TalkSel gTalkSelects_9[10] = {
#include "lobby/008119A0.inc"
};
u32 D_008119F0[164] = {
#include "lobby/008119F0.inc"
};
TalkEvent gTalkEvents_10[4] = {
#include "lobby/00811C80.inc"
};
u32 gTalkSelects_10_1_1_1[2] = {
#include "lobby/00811C90.inc"
};
u32 gTalkSelects_10_1_1_3[2] = {
#include "lobby/00811C98.inc"
};
u32 gTalkSelects_10_1_3_1[2] = {
#include "lobby/00811CA0.inc"
};
u32 gTalkSelects_10_1_3_3[2] = {
#include "lobby/00811CA8.inc"
};
u32 gTalkSelects_10_1_5_1[2] = {
#include "lobby/00811CB0.inc"
};
u32 gTalkSelects_10_1_5_3[2] = {
#include "lobby/00811CB8.inc"
};
u32 gTalkSelects_10_1_5[4] = {
#include "lobby/00811CC0.inc"
};
u32 gTalkSelects_10_1_3[4] = {
#include "lobby/00811CD0.inc"
};
u32 gTalkSelects_10_1_1[4] = {
#include "lobby/00811CE0.inc"
};
u32 gTalkSelects_10_1[6] = {
#include "lobby/00811CF0.inc"
};
TalkSel gTalkSelects_10 = {0, (void *)gTalkSelects_10_1};
u32 D_00811D10[28] = {
#include "lobby/00811D10.inc"
};
TalkGift gTalkGifts_10[2] = {
#include "lobby/00811D80.inc"
};
TalkEvent gTalkEvents_11[6] = {
#include "lobby/00811D90.inc"
};
u32 gTalkSelects_11_1_1_1[2] = {
#include "lobby/00811DA8.inc"
};
u32 gTalkSelects_11_1_1_3[2] = {
#include "lobby/00811DB0.inc"
};
u32 gTalkSelects_11_1_3_1[2] = {
#include "lobby/00811DB8.inc"
};
u32 gTalkSelects_11_1_3_3[2] = {
#include "lobby/00811DC0.inc"
};
u32 gTalkSelects_11_1_5_1[2] = {
#include "lobby/00811DC8.inc"
};
u32 gTalkSelects_11_1_5_3[4] = {
#include "lobby/00811DD0.inc"
};
u32 gTalkSelects_11_1_5[4] = {
#include "lobby/00811DE0.inc"
};
u32 gTalkSelects_11_1_3[4] = {
#include "lobby/00811DF0.inc"
};
u32 gTalkSelects_11_1_1[4] = {
#include "lobby/00811E00.inc"
};
u32 gTalkSelects_11_1[6] = {
#include "lobby/00811E10.inc"
};
TalkSel gTalkSelects_11 = {0, (void *)gTalkSelects_11_1};
u32 D_00811E30[32] = {
#include "lobby/00811E30.inc"
};
TalkGift gTalkGifts_11[2] = {
#include "lobby/00811EB0.inc"
};
TalkEvent gTalkEvents_12[4] = {
#include "lobby/00811EC0.inc"
};
u32 gTalkSelects_12_0_1_1[2] = {
#include "lobby/00811ED0.inc"
};
u32 gTalkSelects_12_0_1_3[2] = {
#include "lobby/00811ED8.inc"
};
u32 gTalkSelects_12_1_1_1[2] = {
#include "lobby/00811EE0.inc"
};
u32 gTalkSelects_12_1_1_3[2] = {
#include "lobby/00811EE8.inc"
};
u32 gTalkSelects_12_1_1[4] = {
#include "lobby/00811EF0.inc"
};
u32 gTalkSelects_12_1[4] = {
    0, (u32)gTalkSelects_12_1_1, 0, 0,
};
u32 gTalkSelects_12_0_1[4] = {
#include "lobby/00811F10.inc"
};
u32 gTalkSelects_12_0[4] = {
    0, (u32)gTalkSelects_12_0_1, 0, 0,
};
TalkSel gTalkSelects_12[2] = {
#include "lobby/00811F30.inc"
};
u32 D_00811F40[32] = {
#include "lobby/00811F40.inc"
};
TalkGift gTalkGifts_12[2] = {
#include "lobby/00811FC0.inc"
};
TalkEvent gTalkEvents_13[4] = {
#include "lobby/00811FD0.inc"
};
u32 gTalkSelects_13_1_1_1[2] = {
#include "lobby/00811FE0.inc"
};
u32 gTalkSelects_13_1_1_3[2] = {
#include "lobby/00811FE8.inc"
};
u32 gTalkSelects_13_1_3_1[2] = {
#include "lobby/00811FF0.inc"
};
u32 gTalkSelects_13_1_3_3[2] = {
#include "lobby/00811FF8.inc"
};
u32 gTalkSelects_13_1_5_1[2] = {
#include "lobby/00812000.inc"
};
u32 gTalkSelects_13_1_5_3[2] = {
#include "lobby/00812008.inc"
};
u32 gTalkSelects_13_1_5[4] = {
#include "lobby/00812010.inc"
};
u32 gTalkSelects_13_1_3[4] = {
#include "lobby/00812020.inc"
};
u32 gTalkSelects_13_1_1[4] = {
#include "lobby/00812030.inc"
};
u32 gTalkSelects_13_1[6] = {
#include "lobby/00812040.inc"
};
TalkSel gTalkSelects_13 = {0, (void *)gTalkSelects_13_1};
u32 D_00812060[28] = {
#include "lobby/00812060.inc"
};
TalkGift gTalkGifts_13[2] = {
#include "lobby/008120D0.inc"
};
TalkEvent gTalkEvents_14[18] = {
#include "lobby/008120E0.inc"
};
u32 gTalkSelects_14_0_1_1[2] = {
#include "lobby/00812128.inc"
};
u32 gTalkSelects_14_0_1_3[2] = {
#include "lobby/00812130.inc"
};
u32 gTalkSelects_14_0_3_1[2] = {
#include "lobby/00812138.inc"
};
u32 gTalkSelects_14_0_3_3[2] = {
#include "lobby/00812140.inc"
};
u32 gTalkSelects_14_0_5_1[2] = {
#include "lobby/00812148.inc"
};
u32 gTalkSelects_14_0_5_3[2] = {
#include "lobby/00812150.inc"
};
u32 gTalkSelects_14_1_1_1[2] = {
#include "lobby/00812158.inc"
};
u32 gTalkSelects_14_1_1_3[2] = {
    (u32)((u8 *)gHeapBlocks + 0x26E74), 0,
};
u32 gTalkSelects_14_1_3_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x16E73), 0,
};
u32 gTalkSelects_14_1_3_3[2] = {
    (u32)((u8 *)gHeapBlocks + 0x6E72), 0,
};
u32 gTalkSelects_14_1_5_1[2] = {
#include "lobby/00812178.inc"
};
u32 gTalkSelects_14_1_5_3[2] = {
#include "lobby/00812180.inc"
};
u32 gTalkSelects_14_2_1_1[2] = {
#include "lobby/00812188.inc"
};
u32 gTalkSelects_14_2_1_3[2] = {
#include "lobby/00812190.inc"
};
u32 gTalkSelects_14_2_3_1[2] = {
#include "lobby/00812198.inc"
};
u32 gTalkSelects_14_2_3_3[2] = {
#include "lobby/008121A0.inc"
};
u32 gTalkSelects_14_2_5_1[2] = {
#include "lobby/008121A8.inc"
};
u32 gTalkSelects_14_2_5_3[2] = {
#include "lobby/008121B0.inc"
};
u32 gTalkSelects_14_3_1_1[2] = {
#include "lobby/008121B8.inc"
};
u32 gTalkSelects_14_3_1_3[2] = {
#include "lobby/008121C0.inc"
};
u32 gTalkSelects_14_3_3_1[2] = {
#include "lobby/008121C8.inc"
};
u32 gTalkSelects_14_3_3_3[2] = {
#include "lobby/008121D0.inc"
};
u32 gTalkSelects_14_3_5_1[2] = {
#include "lobby/008121D8.inc"
};
u32 gTalkSelects_14_3_5_3[2] = {
#include "lobby/008121E0.inc"
};
u32 gTalkSelects_14_4_1_1[2] = {
#include "lobby/008121E8.inc"
};
u32 gTalkSelects_14_4_1_3[2] = {
#include "lobby/008121F0.inc"
};
u32 gTalkSelects_14_4_3_1[2] = {
#include "lobby/008121F8.inc"
};
u32 gTalkSelects_14_4_3_3[2] = {
#include "lobby/00812200.inc"
};
u32 gTalkSelects_14_4_5_1[2] = {
#include "lobby/00812208.inc"
};
u32 gTalkSelects_14_4_5_3[2] = {
#include "lobby/00812210.inc"
};
u32 gTalkSelects_14_5_1_1[2] = {
#include "lobby/00812218.inc"
};
u32 gTalkSelects_14_5_1_3[2] = {
#include "lobby/00812220.inc"
};
u32 gTalkSelects_14_6_1_1[2] = {
#include "lobby/00812228.inc"
};
u32 gTalkSelects_14_7_1_1[2] = {
#include "lobby/00812230.inc"
};
u32 gTalkSelects_14_7_1[2] = {
    0, (u32)gTalkSelects_14_7_1_1,
};
u32 gTalkSelects_14_7[2] = {
    0, (u32)gTalkSelects_14_7_1,
};
u32 gTalkSelects_14_6_1[2] = {
    0, (u32)gTalkSelects_14_6_1_1,
};
u32 gTalkSelects_14_6[4] = {
    0, (u32)gTalkSelects_14_6_1, 0, 0,
};
u32 gTalkSelects_14_5_1[4] = {
#include "lobby/00812260.inc"
};
u32 gTalkSelects_14_5[4] = {
    0, (u32)gTalkSelects_14_5_1, 0, 0,
};
u32 gTalkSelects_14_4_5[4] = {
#include "lobby/00812280.inc"
};
u32 gTalkSelects_14_4_3[4] = {
#include "lobby/00812290.inc"
};
u32 gTalkSelects_14_4_1[4] = {
#include "lobby/008122A0.inc"
};
u32 gTalkSelects_14_4[8] = {
#include "lobby/008122B0.inc"
};
u32 gTalkSelects_14_3_5[4] = {
#include "lobby/008122D0.inc"
};
u32 gTalkSelects_14_3_3[4] = {
#include "lobby/008122E0.inc"
};
u32 gTalkSelects_14_3_1[4] = {
#include "lobby/008122F0.inc"
};
u32 gTalkSelects_14_3[8] = {
#include "lobby/00812300.inc"
};
u32 gTalkSelects_14_2_5[4] = {
#include "lobby/00812320.inc"
};
u32 gTalkSelects_14_2_3[4] = {
#include "lobby/00812330.inc"
};
u32 gTalkSelects_14_2_1[4] = {
#include "lobby/00812340.inc"
};
u32 gTalkSelects_14_2[8] = {
#include "lobby/00812350.inc"
};
u32 gTalkSelects_14_1_5[4] = {
#include "lobby/00812370.inc"
};
u32 gTalkSelects_14_1_3[4] = {
#include "lobby/00812380.inc"
};
u32 gTalkSelects_14_1_1[4] = {
#include "lobby/00812390.inc"
};
u32 gTalkSelects_14_1[8] = {
#include "lobby/008123A0.inc"
};
u32 gTalkSelects_14_0_5[4] = {
#include "lobby/008123C0.inc"
};
u32 gTalkSelects_14_0_3[4] = {
#include "lobby/008123D0.inc"
};
u32 gTalkSelects_14_0_1[4] = {
#include "lobby/008123E0.inc"
};
u32 gTalkSelects_14_0[8] = {
#include "lobby/008123F0.inc"
};
TalkSel gTalkSelects_14[8] = {
#include "lobby/00812410.inc"
};
u32 D_00812450[160] = {
#include "lobby/00812450.inc"
};
TalkGift gTalkGifts_14[2] = {
#include "lobby/008126D0.inc"
};
TalkEvent gTalkEvents_15[4] = {
#include "lobby/008126E0.inc"
};
u32 gTalkSelects_15_1_1_1[2] = {
#include "lobby/008126F0.inc"
};
u32 gTalkSelects_15_1_1[2] = {
    0, (u32)gTalkSelects_15_1_1_1,
};
u32 gTalkSelects_15_1[2] = {
    0, (u32)gTalkSelects_15_1_1,
};
TalkSel gTalkSelects_15 = {0, (void *)gTalkSelects_15_1};
u32 D_00812710[28] = {
#include "lobby/00812710.inc"
};
TalkEvent gTalkEvents_16[12] = {
#include "lobby/00812780.inc"
};
u32 gTalkSelects_16_0_1_1[2] = {
#include "lobby/008127B0.inc"
};
u32 gTalkSelects_16_0_1_3[2] = {
#include "lobby/008127B8.inc"
};
u32 gTalkSelects_16_0_3_1[2] = {
#include "lobby/008127C0.inc"
};
u32 gTalkSelects_16_0_3_3[2] = {
#include "lobby/008127C8.inc"
};
u32 gTalkSelects_16_0_5_1[2] = {
#include "lobby/008127D0.inc"
};
u32 gTalkSelects_16_0_5_3[2] = {
    (u32)((u8 *)D_00840080 + 0x3), 0,
};
u32 gTalkSelects_16_1_1_1[2] = {
#include "lobby/008127E0.inc"
};
u32 gTalkSelects_16_1_1_3[2] = {
#include "lobby/008127E8.inc"
};
u32 gTalkSelects_16_2_1_1[2] = {
#include "lobby/008127F0.inc"
};
u32 gTalkSelects_16_2_1_3[2] = {
#include "lobby/008127F8.inc"
};
u32 gTalkSelects_16_3_1_1[2] = {
#include "lobby/00812800.inc"
};
u32 gTalkSelects_16_4_1_1[2] = {
#include "lobby/00812808.inc"
};
u32 gTalkSelects_16_4_1_3[2] = {
#include "lobby/00812810.inc"
};
u32 gTalkSelects_16_5_1_1[2] = {
#include "lobby/00812818.inc"
};
u32 gTalkSelects_16_5_1_3[2] = {
#include "lobby/00812820.inc"
};
u32 gTalkSelects_16_6_1_1[2] = {
#include "lobby/00812828.inc"
};
u32 gTalkSelects_16_6_3_1[2] = {
#include "lobby/00812830.inc"
};
u32 gTalkSelects_16_6_5_1[2] = {
#include "lobby/00812838.inc"
};
u32 gTalkSelects_16_7_1_1[2] = {
#include "lobby/00812840.inc"
};
u32 gTalkSelects_16_7_1_3[2] = {
#include "lobby/00812848.inc"
};
u32 gTalkSelects_16_8_1_1[2] = {
#include "lobby/00812850.inc"
};
u32 gTalkSelects_16_8_1_3[2] = {
#include "lobby/00812858.inc"
};
u32 gTalkSelects_16_8_1[4] = {
#include "lobby/00812860.inc"
};
u32 gTalkSelects_16_8[4] = {
    0, (u32)gTalkSelects_16_8_1, 0, 0,
};
u32 gTalkSelects_16_7_1[4] = {
#include "lobby/00812880.inc"
};
u32 gTalkSelects_16_7[2] = {
    0, (u32)gTalkSelects_16_7_1,
};
u32 gTalkSelects_16_6_5[2] = {
    0, (u32)gTalkSelects_16_6_5_1,
};
u32 gTalkSelects_16_6_3[2] = {
    0, (u32)gTalkSelects_16_6_3_1,
};
u32 gTalkSelects_16_6_1[2] = {
    0, (u32)gTalkSelects_16_6_1_1,
};
u32 gTalkSelects_16_6[8] = {
#include "lobby/008128B0.inc"
};
u32 gTalkSelects_16_5_1[4] = {
#include "lobby/008128D0.inc"
};
u32 gTalkSelects_16_5[4] = {
    0, (u32)gTalkSelects_16_5_1, 0, 0,
};
u32 gTalkSelects_16_4_1[4] = {
#include "lobby/008128F0.inc"
};
u32 gTalkSelects_16_4[2] = {
    0, (u32)gTalkSelects_16_4_1,
};
u32 gTalkSelects_16_3_1[2] = {
    0, (u32)gTalkSelects_16_3_1_1,
};
u32 gTalkSelects_16_3[4] = {
    0, (u32)gTalkSelects_16_3_1, 0, 0,
};
u32 gTalkSelects_16_2_1[4] = {
#include "lobby/00812920.inc"
};
u32 gTalkSelects_16_2[4] = {
    0, (u32)gTalkSelects_16_2_1, 0, 0,
};
u32 gTalkSelects_16_1_1[4] = {
#include "lobby/00812940.inc"
};
u32 gTalkSelects_16_1[4] = {
    0, (u32)gTalkSelects_16_1_1, 0, 0,
};
u32 gTalkSelects_16_0_5[4] = {
#include "lobby/00812960.inc"
};
u32 gTalkSelects_16_0_3[4] = {
#include "lobby/00812970.inc"
};
u32 gTalkSelects_16_0_1[4] = {
#include "lobby/00812980.inc"
};
u32 gTalkSelects_16_0[8] = {
#include "lobby/00812990.inc"
};
TalkSel gTalkSelects_16[10] = {
#include "lobby/008129B0.inc"
};
u32 D_00812A00[240] = {
#include "lobby/00812A00.inc"
};
TalkGift gTalkGifts_16[4] = {
#include "lobby/00812DC0.inc"
};
TalkEvent gTalkEvents_18[10] = {
#include "lobby/00812DE0.inc"
};
u32 gTalkSelects_18_0_1_1[2] = {
#include "lobby/00812E08.inc"
};
u32 gTalkSelects_18_1_1_1[2] = {
#include "lobby/00812E10.inc"
};
u32 gTalkSelects_18_2_1_1[2] = {
#include "lobby/00812E18.inc"
};
u32 gTalkSelects_18_2_1[2] = {
    0, (u32)gTalkSelects_18_2_1_1,
};
u32 gTalkSelects_18_2[2] = {
    0, (u32)gTalkSelects_18_2_1,
};
u32 gTalkSelects_18_1_1[2] = {
    0, (u32)gTalkSelects_18_1_1_1,
};
u32 gTalkSelects_18_1[2] = {
    0, (u32)gTalkSelects_18_1_1,
};
u32 gTalkSelects_18_0_1[2] = {
    0, (u32)gTalkSelects_18_0_1_1,
};
u32 gTalkSelects_18_0[2] = {
    0, (u32)gTalkSelects_18_0_1,
};
TalkSel gTalkSelects_18[4] = {
#include "lobby/00812E50.inc"
};
u32 D_00812E70[44] = {
#include "lobby/00812E70.inc"
};
TalkGift gTalkGifts_18[2] = {
#include "lobby/00812F20.inc"
};
TalkEvent gTalkEvents_19[8] = {
#include "lobby/00812F30.inc"
};
u32 gTalkSelects_19_0_1_1[2] = {
#include "lobby/00812F50.inc"
};
u32 gTalkSelects_19_1_1_1[2] = {
#include "lobby/00812F58.inc"
};
u32 gTalkSelects_19_2_1_1[2] = {
#include "lobby/00812F60.inc"
};
u32 gTalkSelects_19_2_1[2] = {
    0, (u32)gTalkSelects_19_2_1_1,
};
u32 gTalkSelects_19_2[2] = {
    0, (u32)gTalkSelects_19_2_1,
};
u32 gTalkSelects_19_1_1[2] = {
    0, (u32)gTalkSelects_19_1_1_1,
};
u32 gTalkSelects_19_1[2] = {
    0, (u32)gTalkSelects_19_1_1,
};
u32 gTalkSelects_19_0_1[2] = {
    0, (u32)gTalkSelects_19_0_1_1,
};
u32 gTalkSelects_19_0[4] = {
    0, (u32)gTalkSelects_19_0_1, 0, 0,
};
TalkSel gTalkSelects_19[4] = {
#include "lobby/00812FA0.inc"
};
u32 D_00812FC0[48] = {
#include "lobby/00812FC0.inc"
};
TalkEvent gTalkEvents_21[6] = {
#include "lobby/00813080.inc"
};
u32 gTalkSelects_21_1_1_1[2] = {
#include "lobby/00813098.inc"
};
u32 gTalkSelects_21_1_1_3[2] = {
#include "lobby/008130A0.inc"
};
u32 gTalkSelects_21_1_3_1[2] = {
#include "lobby/008130A8.inc"
};
u32 gTalkSelects_21_1_3_3[2] = {
#include "lobby/008130B0.inc"
};
u32 gTalkSelects_21_1_5_1[2] = {
#include "lobby/008130B8.inc"
};
u32 gTalkSelects_21_1_5_3[4] = {
#include "lobby/008130C0.inc"
};
u32 gTalkSelects_21_1_5[4] = {
#include "lobby/008130D0.inc"
};
u32 gTalkSelects_21_1_3[4] = {
#include "lobby/008130E0.inc"
};
u32 gTalkSelects_21_1_1[4] = {
#include "lobby/008130F0.inc"
};
u32 gTalkSelects_21_1[6] = {
#include "lobby/00813100.inc"
};
TalkSel gTalkSelects_21 = {0, (void *)gTalkSelects_21_1};
u32 D_00813120[66] = {
#include "lobby/00813120.inc"
};
TalkEvent gTalkEvents_22[2] = {
#include "lobby/00813228.inc"
};
u32 gTalkSelects_22_1_1_1[2] = {
#include "lobby/00813230.inc"
};
u32 gTalkSelects_22_1_1_3[2] = {
#include "lobby/00813238.inc"
};
u32 gTalkSelects_22_1_3_1[2] = {
#include "lobby/00813240.inc"
};
u32 gTalkSelects_22_1_3_3[2] = {
#include "lobby/00813248.inc"
};
u32 gTalkSelects_22_1_5_1[2] = {
#include "lobby/00813250.inc"
};
u32 gTalkSelects_22_1_5_3[2] = {
#include "lobby/00813258.inc"
};
u32 gTalkSelects_22_1_5[4] = {
#include "lobby/00813260.inc"
};
u32 gTalkSelects_22_1_3[4] = {
#include "lobby/00813270.inc"
};
u32 gTalkSelects_22_1_1[4] = {
#include "lobby/00813280.inc"
};
u32 gTalkSelects_22_1[6] = {
#include "lobby/00813290.inc"
};
TalkSel gTalkSelects_22 = {0, (void *)gTalkSelects_22_1};
u32 D_008132B0[32] = {
#include "lobby/008132B0.inc"
};
TalkEvent gTalkEvents_23[2] = {
#include "lobby/00813330.inc"
};
u32 gTalkSelects_23_1_1_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x26E70), 0,
};
u32 gTalkSelects_23_1_1_3[2] = {
#include "lobby/00813340.inc"
};
u32 gTalkSelects_23_1_3_1[2] = {
#include "lobby/00813348.inc"
};
u32 gTalkSelects_23_1_3_3[2] = {
#include "lobby/00813350.inc"
};
u32 gTalkSelects_23_1_5_1[2] = {
#include "lobby/00813358.inc"
};
u32 gTalkSelects_23_1_5_3[4] = {
#include "lobby/00813360.inc"
};
u32 gTalkSelects_23_1_5[4] = {
#include "lobby/00813370.inc"
};
u32 gTalkSelects_23_1_3[4] = {
#include "lobby/00813380.inc"
};
u32 gTalkSelects_23_1_1[4] = {
#include "lobby/00813390.inc"
};
u32 gTalkSelects_23_1[6] = {
#include "lobby/008133A0.inc"
};
TalkSel gTalkSelects_23 = {0, (void *)gTalkSelects_23_1};
u32 D_008133C0[42] = {
#include "lobby/008133C0.inc"
};
TalkEvent gTalkEvents_24[3] = {
#include "lobby/00813468.inc"
};
u32 gTalkSelects_24_0_1_1[2] = {
#include "lobby/00813478.inc"
};
u32 gTalkSelects_24_0_1_3[2] = {
#include "lobby/00813480.inc"
};
u32 gTalkSelects_24_0_3_1[2] = {
#include "lobby/00813488.inc"
};
u32 gTalkSelects_24_0_3_3[2] = {
#include "lobby/00813490.inc"
};
u32 gTalkSelects_24_0_5_1[2] = {
#include "lobby/00813498.inc"
};
u32 gTalkSelects_24_0_5_3[2] = {
#include "lobby/008134A0.inc"
};
u32 gTalkSelects_24_1_1_1[2] = {
#include "lobby/008134A8.inc"
};
u32 gTalkSelects_24_2_1_1[2] = {
#include "lobby/008134B0.inc"
};
u32 gTalkSelects_24_2_1_3[2] = {
#include "lobby/008134B8.inc"
};
u32 gTalkSelects_24_3_1_1[2] = {
#include "lobby/008134C0.inc"
};
u32 gTalkSelects_24_3_3_1[2] = {
#include "lobby/008134C8.inc"
};
u32 gTalkSelects_24_3_5_1[2] = {
#include "lobby/008134D0.inc"
};
u32 gTalkSelects_24_4_1_1[2] = {
#include "lobby/008134D8.inc"
};
u32 gTalkSelects_24_4_1_3[2] = {
#include "lobby/008134E0.inc"
};
u32 gTalkSelects_24_4_3_1[2] = {
#include "lobby/008134E8.inc"
};
u32 gTalkSelects_24_4_3_3[2] = {
#include "lobby/008134F0.inc"
};
u32 gTalkSelects_24_4_5_1[2] = {
#include "lobby/008134F8.inc"
};
u32 gTalkSelects_24_4_5_3[2] = {
#include "lobby/00813500.inc"
};
u32 gTalkSelects_24_5_1_1[2] = {
#include "lobby/00813508.inc"
};
u32 gTalkSelects_24_5_1_3[2] = {
#include "lobby/00813510.inc"
};
u32 gTalkSelects_24_5_3_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x26E6F), 0,
};
u32 gTalkSelects_24_5_3_3[2] = {
#include "lobby/00813520.inc"
};
u32 gTalkSelects_24_5_5_1[2] = {
#include "lobby/00813528.inc"
};
u32 gTalkSelects_24_5_5_3[4] = {
#include "lobby/00813530.inc"
};
u32 gTalkSelects_24_5_5[4] = {
#include "lobby/00813540.inc"
};
u32 gTalkSelects_24_5_3[4] = {
#include "lobby/00813550.inc"
};
u32 gTalkSelects_24_5_1[4] = {
#include "lobby/00813560.inc"
};
u32 gTalkSelects_24_5[8] = {
#include "lobby/00813570.inc"
};
u32 gTalkSelects_24_4_5[4] = {
#include "lobby/00813590.inc"
};
u32 gTalkSelects_24_4_3[4] = {
#include "lobby/008135A0.inc"
};
u32 gTalkSelects_24_4_1[4] = {
#include "lobby/008135B0.inc"
};
u32 gTalkSelects_24_4[6] = {
#include "lobby/008135C0.inc"
};
u32 gTalkSelects_24_3_5[2] = {
    0, (u32)gTalkSelects_24_3_5_1,
};
u32 gTalkSelects_24_3_3[2] = {
    0, (u32)gTalkSelects_24_3_3_1,
};
u32 gTalkSelects_24_3_1[2] = {
    0, (u32)gTalkSelects_24_3_1_1,
};
u32 gTalkSelects_24_3[8] = {
#include "lobby/008135F0.inc"
};
u32 gTalkSelects_24_2_1[4] = {
#include "lobby/00813610.inc"
};
u32 gTalkSelects_24_2[2] = {
    0, (u32)gTalkSelects_24_2_1,
};
u32 gTalkSelects_24_1_1[2] = {
    0, (u32)gTalkSelects_24_1_1_1,
};
u32 gTalkSelects_24_1[4] = {
    0, (u32)gTalkSelects_24_1_1, 0, 0,
};
u32 gTalkSelects_24_0_5[4] = {
#include "lobby/00813640.inc"
};
u32 gTalkSelects_24_0_3[4] = {
#include "lobby/00813650.inc"
};
u32 gTalkSelects_24_0_1[4] = {
#include "lobby/00813660.inc"
};
u32 gTalkSelects_24_0[8] = {
#include "lobby/00813670.inc"
};
TalkSel gTalkSelects_24[6] = {
#include "lobby/00813690.inc"
};
u32 D_008136C0[176] = {
#include "lobby/008136C0.inc"
};
TalkGift gTalkGifts_24[2] = {
#include "lobby/00813980.inc"
};
TalkEvent gTalkEvents_25[4] = {
#include "lobby/00813990.inc"
};
u32 gTalkSelects_25_0_1_1[2] = {
#include "lobby/008139A0.inc"
};
u32 gTalkSelects_25_0_1_3[2] = {
#include "lobby/008139A8.inc"
};
u32 gTalkSelects_25_1_1_1[2] = {
#include "lobby/008139B0.inc"
};
u32 gTalkSelects_25_1_1_3[2] = {
#include "lobby/008139B8.inc"
};
u32 gTalkSelects_25_2_1_1[2] = {
#include "lobby/008139C0.inc"
};
u32 gTalkSelects_25_3_1_1[2] = {
#include "lobby/008139C8.inc"
};
u32 gTalkSelects_25_3_1_3[2] = {
#include "lobby/008139D0.inc"
};
u32 gTalkSelects_25_4_1_1[2] = {
#include "lobby/008139D8.inc"
};
u32 gTalkSelects_25_4_1[2] = {
    0, (u32)gTalkSelects_25_4_1_1,
};
u32 gTalkSelects_25_4[2] = {
    0, (u32)gTalkSelects_25_4_1,
};
u32 gTalkSelects_25_3_1[4] = {
#include "lobby/008139F0.inc"
};
u32 gTalkSelects_25_3[2] = {
    0, (u32)gTalkSelects_25_3_1,
};
u32 gTalkSelects_25_2_1[2] = {
    0, (u32)gTalkSelects_25_2_1_1,
};
u32 gTalkSelects_25_2[4] = {
    0, (u32)gTalkSelects_25_2_1, 0, 0,
};
u32 gTalkSelects_25_1_1[4] = {
#include "lobby/00813A20.inc"
};
u32 gTalkSelects_25_1[4] = {
    0, (u32)gTalkSelects_25_1_1, 0, 0,
};
u32 gTalkSelects_25_0_1[4] = {
#include "lobby/00813A40.inc"
};
u32 gTalkSelects_25_0[4] = {
    0, (u32)gTalkSelects_25_0_1, 0, 0,
};
TalkSel gTalkSelects_25[6] = {
#include "lobby/00813A60.inc"
};
u32 D_00813A90[60] = {
#include "lobby/00813A90.inc"
};
TalkGift gTalkGifts_25[2] = {
#include "lobby/00813B80.inc"
};
TalkEvent gTalkEvents_26[6] = {
#include "lobby/00813B90.inc"
};
u32 gTalkSelects_26_0_1_1[2] = {
#include "lobby/00813BA8.inc"
};
u32 gTalkSelects_26_0_1_3[2] = {
#include "lobby/00813BB0.inc"
};
u32 gTalkSelects_26_1_1_1[2] = {
#include "lobby/00813BB8.inc"
};
u32 gTalkSelects_26_2_1_1[2] = {
#include "lobby/00813BC0.inc"
};
u32 gTalkSelects_26_2_1_3[2] = {
#include "lobby/00813BC8.inc"
};
u32 gTalkSelects_26_3_1_1[2] = {
#include "lobby/00813BD0.inc"
};
u32 gTalkSelects_26_4_1_1[2] = {
#include "lobby/00813BD8.inc"
};
u32 gTalkSelects_26_4_1_3[2] = {
#include "lobby/00813BE0.inc"
};
u32 gTalkSelects_26_5_1_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x26E70), 0,
};
u32 gTalkSelects_26_5_3_1[2] = {
#include "lobby/00813BF0.inc"
};
u32 gTalkSelects_26_5_5_1[2] = {
#include "lobby/00813BF8.inc"
};
u32 gTalkSelects_26_6_1_1[2] = {
#include "lobby/00813C00.inc"
};
u32 gTalkSelects_26_6_1_3[2] = {
#include "lobby/00813C08.inc"
};
u32 gTalkSelects_26_6_1[4] = {
#include "lobby/00813C10.inc"
};
u32 gTalkSelects_26_6[2] = {
    0, (u32)gTalkSelects_26_6_1,
};
u32 gTalkSelects_26_5_5[2] = {
    0, (u32)gTalkSelects_26_5_5_1,
};
u32 gTalkSelects_26_5_3[2] = {
    0, (u32)gTalkSelects_26_5_3_1,
};
u32 gTalkSelects_26_5_1[2] = {
    0, (u32)gTalkSelects_26_5_1_1,
};
u32 gTalkSelects_26_5[8] = {
#include "lobby/00813C40.inc"
};
u32 gTalkSelects_26_4_1[4] = {
#include "lobby/00813C60.inc"
};
u32 gTalkSelects_26_4[2] = {
    0, (u32)gTalkSelects_26_4_1,
};
u32 gTalkSelects_26_3_1[2] = {
    0, (u32)gTalkSelects_26_3_1_1,
};
u32 gTalkSelects_26_3[4] = {
    0, (u32)gTalkSelects_26_3_1, 0, 0,
};
u32 gTalkSelects_26_2_1[4] = {
#include "lobby/00813C90.inc"
};
u32 gTalkSelects_26_2[2] = {
    0, (u32)gTalkSelects_26_2_1,
};
u32 gTalkSelects_26_1_1[2] = {
    0, (u32)gTalkSelects_26_1_1_1,
};
u32 gTalkSelects_26_1[4] = {
    0, (u32)gTalkSelects_26_1_1, 0, 0,
};
u32 gTalkSelects_26_0_1[4] = {
#include "lobby/00813CC0.inc"
};
u32 gTalkSelects_26_0[4] = {
    0, (u32)gTalkSelects_26_0_1, 0, 0,
};
TalkSel gTalkSelects_26[8] = {
#include "lobby/00813CE0.inc"
};
u32 D_00813D20[92] = {
#include "lobby/00813D20.inc"
};
TalkGift gTalkGifts_26[2] = {
#include "lobby/00813E90.inc"
};
TalkEvent gTalkEvents_27[2] = {
#include "lobby/00813EA0.inc"
};
u32 gTalkSelects_27_1_1_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x6E6E), 0,
};
u32 gTalkSelects_27_1_1_3[2] = {
#include "lobby/00813EB0.inc"
};
u32 gTalkSelects_27_1_3_1[2] = {
#include "lobby/00813EB8.inc"
};
u32 gTalkSelects_27_1_3_3[2] = {
#include "lobby/00813EC0.inc"
};
u32 gTalkSelects_27_1_5_1[2] = {
#include "lobby/00813EC8.inc"
};
u32 gTalkSelects_27_1_5_3[4] = {
#include "lobby/00813ED0.inc"
};
u32 gTalkSelects_27_1_5[4] = {
#include "lobby/00813EE0.inc"
};
u32 gTalkSelects_27_1_3[4] = {
#include "lobby/00813EF0.inc"
};
u32 gTalkSelects_27_1_1[4] = {
#include "lobby/00813F00.inc"
};
u32 gTalkSelects_27_1[6] = {
#include "lobby/00813F10.inc"
};
TalkSel gTalkSelects_27 = {0, (void *)gTalkSelects_27_1};
u32 D_00813F30[42] = {
#include "lobby/00813F30.inc"
};
TalkEvent gTalkEvents_28[3] = {
#include "lobby/00813FD8.inc"
};
u32 gTalkSelects_28_0_1_1[2] = {
#include "lobby/00813FE8.inc"
};
u32 gTalkSelects_28_1_1_1[2] = {
#include "lobby/00813FF0.inc"
};
u32 gTalkSelects_28_2_1_1[2] = {
#include "lobby/00813FF8.inc"
};
u32 gTalkSelects_28_2_1_3[4] = {
#include "lobby/00814000.inc"
};
u32 gTalkSelects_28_2_1[4] = {
#include "lobby/00814010.inc"
};
u32 gTalkSelects_28_2[2] = {
    0, (u32)gTalkSelects_28_2_1,
};
u32 gTalkSelects_28_1_1[2] = {
    0, (u32)gTalkSelects_28_1_1_1,
};
u32 gTalkSelects_28_1[2] = {
    0, (u32)gTalkSelects_28_1_1,
};
u32 gTalkSelects_28_0_1[2] = {
    0, (u32)gTalkSelects_28_0_1_1,
};
u32 gTalkSelects_28_0[4] = {
    0, (u32)gTalkSelects_28_0_1, 0, 0,
};
TalkSel gTalkSelects_28[4] = {
#include "lobby/00814050.inc"
};
u32 D_00814070[174] = {
#include "lobby/00814070.inc"
};
TalkEvent gTalkEvents_29[2] = {
#include "lobby/00814328.inc"
};
u32 gTalkSelects_29_1_1[2] = {
    0, (u32)D_008BF790,
};
u32 gTalkSelects_29_1[2] = {
    0, (u32)gTalkSelects_29_1_1,
};
TalkSel gTalkSelects_29 = {0, (void *)gTalkSelects_29_1};
u32 D_00814348[2] = {
#include "lobby/00814348.inc"
};
TalkEvent gTalkEvents_30[6] = {
#include "lobby/00814350.inc"
};
u32 gTalkSelects_30_0_1_1[2] = {
#include "lobby/00814368.inc"
};
u32 gTalkSelects_30_1_1_1[2] = {
#include "lobby/00814370.inc"
};
u32 gTalkSelects_30_2_1_1[2] = {
#include "lobby/00814378.inc"
};
u32 gTalkSelects_30_3_1_1[2] = {
#include "lobby/00814380.inc"
};
u32 gTalkSelects_30_3_1[2] = {
    0, (u32)gTalkSelects_30_3_1_1,
};
u32 gTalkSelects_30_3[2] = {
    0, (u32)gTalkSelects_30_3_1,
};
u32 gTalkSelects_30_2_1[2] = {
    0, (u32)gTalkSelects_30_2_1_1,
};
u32 gTalkSelects_30_2[2] = {
    0, (u32)gTalkSelects_30_2_1,
};
u32 gTalkSelects_30_1_1[2] = {
    0, (u32)gTalkSelects_30_1_1_1,
};
u32 gTalkSelects_30_1[2] = {
    0, (u32)gTalkSelects_30_1_1,
};
u32 gTalkSelects_30_0_1[2] = {
    0, (u32)gTalkSelects_30_0_1_1,
};
u32 gTalkSelects_30_0[4] = {
    0, (u32)gTalkSelects_30_0_1, 0, 0,
};
TalkSel gTalkSelects_30[4] = {
#include "lobby/008143D0.inc"
};
u32 D_008143F0[36] = {
#include "lobby/008143F0.inc"
};
TalkEvent gTalkEvents_31[2] = {
#include "lobby/00814480.inc"
};
u32 gTalkSelects_31_0_1_1[2] = {
#include "lobby/00814488.inc"
};
u32 gTalkSelects_31_1_1_1[2] = {
#include "lobby/00814490.inc"
};
u32 gTalkSelects_31_2_1_1[2] = {
#include "lobby/00814498.inc"
};
u32 gTalkSelects_31_2_3_1[2] = {
#include "lobby/008144A0.inc"
};
u32 gTalkSelects_31_2_5_1[2] = {
#include "lobby/008144A8.inc"
};
u32 gTalkSelects_31_2_5[2] = {
    0, (u32)gTalkSelects_31_2_5_1,
};
u32 gTalkSelects_31_2_3[2] = {
    0, (u32)gTalkSelects_31_2_3_1,
};
u32 gTalkSelects_31_2_1[4] = {
    0, (u32)gTalkSelects_31_2_1_1, 0, 0,
};
u32 gTalkSelects_31_2[6] = {
#include "lobby/008144D0.inc"
};
u32 gTalkSelects_31_1_1[2] = {
    0, (u32)gTalkSelects_31_1_1_1,
};
u32 gTalkSelects_31_1[2] = {
    0, (u32)gTalkSelects_31_1_1,
};
u32 gTalkSelects_31_0_1[2] = {
    0, (u32)gTalkSelects_31_0_1_1,
};
u32 gTalkSelects_31_0[4] = {
    0, (u32)gTalkSelects_31_0_1, 0, 0,
};
TalkSel gTalkSelects_31[4] = {
#include "lobby/00814510.inc"
};
u32 D_00814530[30] = {
#include "lobby/00814530.inc"
};
TalkEvent gTalkEvents_32[2] = {
#include "lobby/008145A8.inc"
};
u32 gTalkSelects_32_0_1_1[2] = {
#include "lobby/008145B0.inc"
};
u32 gTalkSelects_32_1_1_1[2] = {
#include "lobby/008145B8.inc"
};
u32 gTalkSelects_32_2_1[2] = {
    0, (u32)D_008BF7B0,
};
u32 gTalkSelects_32_2[2] = {
    0, (u32)gTalkSelects_32_2_1,
};
u32 gTalkSelects_32_1_1[2] = {
    0, (u32)gTalkSelects_32_1_1_1,
};
u32 gTalkSelects_32_1[2] = {
    0, (u32)gTalkSelects_32_1_1,
};
u32 gTalkSelects_32_0_1[2] = {
    0, (u32)gTalkSelects_32_0_1_1,
};
u32 gTalkSelects_32_0[2] = {
    0, (u32)gTalkSelects_32_0_1,
};
TalkSel gTalkSelects_32[4] = {
#include "lobby/008145F0.inc"
};
u32 D_00814610[24] = {
#include "lobby/00814610.inc"
};
TalkEvent gTalkEvents_33[2] = {
#include "lobby/00814670.inc"
};
u32 gTalkSelects_33_0_1_1[2] = {
#include "lobby/00814678.inc"
};
u32 gTalkSelects_33_1_1_1[2] = {
#include "lobby/00814680.inc"
};
u32 gTalkSelects_33_2_1_1[2] = {
#include "lobby/00814688.inc"
};
u32 gTalkSelects_33_2_1[2] = {
    0, (u32)gTalkSelects_33_2_1_1,
};
u32 gTalkSelects_33_2[2] = {
    0, (u32)gTalkSelects_33_2_1,
};
u32 gTalkSelects_33_1_1[2] = {
    0, (u32)gTalkSelects_33_1_1_1,
};
u32 gTalkSelects_33_1[2] = {
    0, (u32)gTalkSelects_33_1_1,
};
u32 gTalkSelects_33_0_1[2] = {
    0, (u32)gTalkSelects_33_0_1_1,
};
u32 gTalkSelects_33_0[2] = {
    0, (u32)gTalkSelects_33_0_1,
};
TalkSel gTalkSelects_33[4] = {
#include "lobby/008146C0.inc"
};
u32 D_008146E0[18] = {
#include "lobby/008146E0.inc"
};
TalkEvent gTalkEvents_34[2] = {
#include "lobby/00814728.inc"
};
u32 gTalkSelects_34_1_1[2] = {
    0, (u32)D_008BF7C8,
};
u32 gTalkSelects_34_1[2] = {
    0, (u32)gTalkSelects_34_1_1,
};
TalkSel gTalkSelects_34 = {0, (void *)gTalkSelects_34_1};
u32 D_00814748[2] = {
#include "lobby/00814748.inc"
};
TalkEvent gTalkEvents_35[4] = {
#include "lobby/00814750.inc"
};
u32 gTalkSelects_35_0_1_1[2] = {
#include "lobby/00814760.inc"
};
u32 gTalkSelects_35_1_1_1[2] = {
#include "lobby/00814768.inc"
};
u32 gTalkSelects_35_2_1[2] = {
    0, (u32)D_008BF7D8,
};
u32 gTalkSelects_35_2[2] = {
    0, (u32)gTalkSelects_35_2_1,
};
u32 gTalkSelects_35_1_1[2] = {
    0, (u32)gTalkSelects_35_1_1_1,
};
u32 gTalkSelects_35_1[2] = {
    0, (u32)gTalkSelects_35_1_1,
};
u32 gTalkSelects_35_0_1[2] = {
    0, (u32)gTalkSelects_35_0_1_1,
};
u32 gTalkSelects_35_0[2] = {
    0, (u32)gTalkSelects_35_0_1,
};
TalkSel gTalkSelects_35[4] = {
#include "lobby/008147A0.inc"
};
u32 D_008147C0[18] = {
#include "lobby/008147C0.inc"
};
TalkEvent gTalkEvents_36[2] = {
#include "lobby/00814808.inc"
};
u32 gTalkSelects_36_0_1_1[2] = {
#include "lobby/00814810.inc"
};
u32 gTalkSelects_36_1_1_1[2] = {
#include "lobby/00814818.inc"
};
u32 gTalkSelects_36_2_1_1[4] = {
#include "lobby/00814820.inc"
};
u32 gTalkSelects_36_2_1[4] = {
#include "lobby/00814830.inc"
};
u32 gTalkSelects_36_2[2] = {
    0, (u32)gTalkSelects_36_2_1,
};
u32 gTalkSelects_36_1_1[2] = {
    0, (u32)gTalkSelects_36_1_1_1,
};
u32 gTalkSelects_36_1[2] = {
    0, (u32)gTalkSelects_36_1_1,
};
u32 gTalkSelects_36_0_1[2] = {
    0, (u32)gTalkSelects_36_0_1_1,
};
u32 gTalkSelects_36_0[4] = {
    0, (u32)gTalkSelects_36_0_1, 0, 0,
};
TalkSel gTalkSelects_36[4] = {
#include "lobby/00814870.inc"
};
u32 D_00814890[20] = {
#include "lobby/00814890.inc"
};
TalkEvent gTalkEvents_37[2] = {
#include "lobby/008148E0.inc"
};
u32 gTalkSelects_37_0_1_1[2] = {
#include "lobby/008148E8.inc"
};
u32 gTalkSelects_37_1_1_1[2] = {
#include "lobby/008148F0.inc"
};
u32 gTalkSelects_37_2_1_1[2] = {
#include "lobby/008148F8.inc"
};
u32 gTalkSelects_37_2_1[2] = {
    0, (u32)gTalkSelects_37_2_1_1,
};
u32 gTalkSelects_37_2[2] = {
    0, (u32)gTalkSelects_37_2_1,
};
u32 gTalkSelects_37_1_1[2] = {
    0, (u32)gTalkSelects_37_1_1_1,
};
u32 gTalkSelects_37_1[2] = {
    0, (u32)gTalkSelects_37_1_1,
};
u32 gTalkSelects_37_0_1[2] = {
    0, (u32)gTalkSelects_37_0_1_1,
};
u32 gTalkSelects_37_0[2] = {
    0, (u32)gTalkSelects_37_0_1,
};
TalkSel gTalkSelects_37[4] = {
#include "lobby/00814930.inc"
};
u32 D_00814950[20] = {
#include "lobby/00814950.inc"
};
TalkEvent gTalkEvents_38[2] = {
#include "lobby/008149A0.inc"
};
u32 gTalkSelects_38_0_1_1[2] = {
#include "lobby/008149A8.inc"
};
u32 gTalkSelects_38_1_1_1[2] = {
#include "lobby/008149B0.inc"
};
u32 gTalkSelects_38_2_1_1[2] = {
#include "lobby/008149B8.inc"
};
u32 gTalkSelects_38_2_1[2] = {
    0, (u32)gTalkSelects_38_2_1_1,
};
u32 gTalkSelects_38_2[2] = {
    0, (u32)gTalkSelects_38_2_1,
};
u32 gTalkSelects_38_1_1[2] = {
    0, (u32)gTalkSelects_38_1_1_1,
};
u32 gTalkSelects_38_1[2] = {
    0, (u32)gTalkSelects_38_1_1,
};
u32 gTalkSelects_38_0_1[2] = {
    0, (u32)gTalkSelects_38_0_1_1,
};
u32 gTalkSelects_38_0[2] = {
    0, (u32)gTalkSelects_38_0_1,
};
TalkSel gTalkSelects_38[4] = {
#include "lobby/008149F0.inc"
};
u32 D_00814A10[20] = {
#include "lobby/00814A10.inc"
};
TalkEvent gTalkEvents_39[2] = {
#include "lobby/00814A60.inc"
};
u32 gTalkSelects_39_0_1_1[2] = {
#include "lobby/00814A68.inc"
};
u32 gTalkSelects_39_1_1_1[2] = {
#include "lobby/00814A70.inc"
};
u32 gTalkSelects_39_2_1_1[2] = {
#include "lobby/00814A78.inc"
};
u32 gTalkSelects_39_2_1[4] = {
#include "lobby/00814A80.inc"
};
u32 gTalkSelects_39_2[2] = {
    0, (u32)gTalkSelects_39_2_1,
};
u32 gTalkSelects_39_1_1[2] = {
    0, (u32)gTalkSelects_39_1_1_1,
};
u32 gTalkSelects_39_1[2] = {
    0, (u32)gTalkSelects_39_1_1,
};
u32 gTalkSelects_39_0_1[2] = {
    0, (u32)gTalkSelects_39_0_1_1,
};
u32 gTalkSelects_39_0[4] = {
    0, (u32)gTalkSelects_39_0_1, 0, 0,
};
TalkSel gTalkSelects_39[4] = {
#include "lobby/00814AC0.inc"
};
u32 D_00814AE0[24] = {
#include "lobby/00814AE0.inc"
};
TalkEvent gTalkEvents_40[6] = {
#include "lobby/00814B40.inc"
};
u32 gTalkSelects_40_0_1_1[2] = {
#include "lobby/00814B58.inc"
};
u32 gTalkSelects_40_1_1_1[2] = {
#include "lobby/00814B60.inc"
};
u32 gTalkSelects_40_2_1_1[2] = {
#include "lobby/00814B68.inc"
};
u32 gTalkSelects_40_2_1[2] = {
    0, (u32)gTalkSelects_40_2_1_1,
};
u32 gTalkSelects_40_2[2] = {
    0, (u32)gTalkSelects_40_2_1,
};
u32 gTalkSelects_40_1_1[2] = {
    0, (u32)gTalkSelects_40_1_1_1,
};
u32 gTalkSelects_40_1[2] = {
    0, (u32)gTalkSelects_40_1_1,
};
u32 gTalkSelects_40_0_1[2] = {
    0, (u32)gTalkSelects_40_0_1_1,
};
u32 gTalkSelects_40_0[2] = {
    0, (u32)gTalkSelects_40_0_1,
};
TalkSel gTalkSelects_40[4] = {
#include "lobby/00814BA0.inc"
};
u32 D_00814BC0[20] = {
#include "lobby/00814BC0.inc"
};
TalkEvent gTalkEvents_41[2] = {
#include "lobby/00814C10.inc"
};
u32 gTalkSelects_41_0_1_1[2] = {
#include "lobby/00814C18.inc"
};
u32 gTalkSelects_41_1_1_1[2] = {
#include "lobby/00814C20.inc"
};
u32 gTalkSelects_41_2_1_1[2] = {
#include "lobby/00814C28.inc"
};
u32 gTalkSelects_41_2_1[2] = {
    0, (u32)gTalkSelects_41_2_1_1,
};
u32 gTalkSelects_41_2[2] = {
    0, (u32)gTalkSelects_41_2_1,
};
u32 gTalkSelects_41_1_1[2] = {
    0, (u32)gTalkSelects_41_1_1_1,
};
u32 gTalkSelects_41_1[2] = {
    0, (u32)gTalkSelects_41_1_1,
};
u32 gTalkSelects_41_0_1[2] = {
    0, (u32)gTalkSelects_41_0_1_1,
};
u32 gTalkSelects_41_0[2] = {
    0, (u32)gTalkSelects_41_0_1,
};
TalkSel gTalkSelects_41[4] = {
#include "lobby/00814C60.inc"
};
u32 D_00814C80[24] = {
#include "lobby/00814C80.inc"
};
TalkEvent gTalkEvents_42[2] = {
#include "lobby/00814CE0.inc"
};
u32 gTalkSelects_42_0_1_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x6E72), 0,
};
u32 gTalkSelects_42_1_1_1[2] = {
#include "lobby/00814CF0.inc"
};
u32 gTalkSelects_42_1_3_1[2] = {
#include "lobby/00814CF8.inc"
};
u32 gTalkSelects_42_1_5_1[2] = {
#include "lobby/00814D00.inc"
};
u32 gTalkSelects_42_2_1_1[2] = {
#include "lobby/00814D08.inc"
};
u32 gTalkSelects_42_2_3_1[2] = {
#include "lobby/00814D10.inc"
};
u32 gTalkSelects_42_2_5_1[2] = {
#include "lobby/00814D18.inc"
};
u32 gTalkSelects_42_2_5[2] = {
    0, (u32)gTalkSelects_42_2_5_1,
};
u32 gTalkSelects_42_2_3[2] = {
    0, (u32)gTalkSelects_42_2_3_1,
};
u32 gTalkSelects_42_2_1[4] = {
    0, (u32)gTalkSelects_42_2_1_1, 0, 0,
};
u32 gTalkSelects_42_2[6] = {
#include "lobby/00814D40.inc"
};
u32 gTalkSelects_42_1_5[2] = {
    0, (u32)gTalkSelects_42_1_5_1,
};
u32 gTalkSelects_42_1_3[2] = {
    0, (u32)gTalkSelects_42_1_3_1,
};
u32 gTalkSelects_42_1_1[2] = {
    0, (u32)gTalkSelects_42_1_1_1,
};
u32 gTalkSelects_42_1[6] = {
#include "lobby/00814D70.inc"
};
u32 gTalkSelects_42_0_1[2] = {
    0, (u32)gTalkSelects_42_0_1_1,
};
u32 gTalkSelects_42_0[4] = {
    0, (u32)gTalkSelects_42_0_1, 0, 0,
};
TalkSel gTalkSelects_42[4] = {
#include "lobby/00814DA0.inc"
};
u32 D_00814DC0[40] = {
#include "lobby/00814DC0.inc"
};
TalkEvent gTalkEvents_43[2] = {
#include "lobby/00814E60.inc"
};
u32 gTalkSelects_43_0_1_1[2] = {
#include "lobby/00814E68.inc"
};
u32 gTalkSelects_43_1_1_1[2] = {
#include "lobby/00814E70.inc"
};
u32 gTalkSelects_43_2_1[2] = {
    0, (u32)D_008BF830,
};
u32 gTalkSelects_43_2[2] = {
    0, (u32)gTalkSelects_43_2_1,
};
u32 gTalkSelects_43_1_1[2] = {
    0, (u32)gTalkSelects_43_1_1_1,
};
u32 gTalkSelects_43_1[2] = {
    0, (u32)gTalkSelects_43_1_1,
};
u32 gTalkSelects_43_0_1[2] = {
    0, (u32)gTalkSelects_43_0_1_1,
};
u32 gTalkSelects_43_0[4] = {
    0, (u32)gTalkSelects_43_0_1, 0, 0,
};
TalkSel gTalkSelects_43[4] = {
#include "lobby/00814EB0.inc"
};
u32 D_00814ED0[16] = {
#include "lobby/00814ED0.inc"
};
TalkEvent gTalkEvents_44[6] = {
#include "lobby/00814F10.inc"
};
u32 gTalkSelects_44_0_1_1[2] = {
#include "lobby/00814F28.inc"
};
u32 gTalkSelects_44_1_1_1[2] = {
#include "lobby/00814F30.inc"
};
u32 gTalkSelects_44_2_1_1[2] = {
#include "lobby/00814F38.inc"
};
u32 gTalkSelects_44_2_1[2] = {
    0, (u32)gTalkSelects_44_2_1_1,
};
u32 gTalkSelects_44_2[2] = {
    0, (u32)gTalkSelects_44_2_1,
};
u32 gTalkSelects_44_1_1[2] = {
    0, (u32)gTalkSelects_44_1_1_1,
};
u32 gTalkSelects_44_1[2] = {
    0, (u32)gTalkSelects_44_1_1,
};
u32 gTalkSelects_44_0_1[2] = {
    0, (u32)gTalkSelects_44_0_1_1,
};
u32 gTalkSelects_44_0[2] = {
    0, (u32)gTalkSelects_44_0_1,
};
TalkSel gTalkSelects_44[4] = {
#include "lobby/00814F70.inc"
};
u32 D_00814F90[16] = {
#include "lobby/00814F90.inc"
};
TalkGift gTalkGifts_44[4] = {
#include "lobby/00814FD0.inc"
};
TalkEvent gTalkEvents_45[6] = {
#include "lobby/00814FF0.inc"
};
u32 gTalkSelects_45_0_1_1[2] = {
#include "lobby/00815008.inc"
};
u32 gTalkSelects_45_0_1_3[2] = {
#include "lobby/00815010.inc"
};
u32 gTalkSelects_45_0_3_1[2] = {
#include "lobby/00815018.inc"
};
u32 gTalkSelects_45_0_3_3[2] = {
#include "lobby/00815020.inc"
};
u32 gTalkSelects_45_0_5_1[2] = {
#include "lobby/00815028.inc"
};
u32 gTalkSelects_45_0_5_3[2] = {
#include "lobby/00815030.inc"
};
u32 gTalkSelects_45_1_1_1[2] = {
#include "lobby/00815038.inc"
};
u32 gTalkSelects_45_1_1_3[2] = {
#include "lobby/00815040.inc"
};
u32 gTalkSelects_45_1_3_1[2] = {
#include "lobby/00815048.inc"
};
u32 gTalkSelects_45_1_3_3[2] = {
#include "lobby/00815050.inc"
};
u32 gTalkSelects_45_1_5_1[2] = {
#include "lobby/00815058.inc"
};
u32 gTalkSelects_45_1_5_3[2] = {
#include "lobby/00815060.inc"
};
u32 gTalkSelects_45_2_1_1[2] = {
#include "lobby/00815068.inc"
};
u32 gTalkSelects_45_2_1_3[2] = {
#include "lobby/00815070.inc"
};
u32 gTalkSelects_45_2_3_1[2] = {
#include "lobby/00815078.inc"
};
u32 gTalkSelects_45_2_3_3[2] = {
#include "lobby/00815080.inc"
};
u32 gTalkSelects_45_2_5_1[2] = {
#include "lobby/00815088.inc"
};
u32 gTalkSelects_45_2_5_3[4] = {
#include "lobby/00815090.inc"
};
u32 gTalkSelects_45_2_5[4] = {
#include "lobby/008150A0.inc"
};
u32 gTalkSelects_45_2_3[4] = {
#include "lobby/008150B0.inc"
};
u32 gTalkSelects_45_2_1[4] = {
#include "lobby/008150C0.inc"
};
u32 gTalkSelects_45_2[8] = {
#include "lobby/008150D0.inc"
};
u32 gTalkSelects_45_1_5[4] = {
#include "lobby/008150F0.inc"
};
u32 gTalkSelects_45_1_3[4] = {
#include "lobby/00815100.inc"
};
u32 gTalkSelects_45_1_1[4] = {
#include "lobby/00815110.inc"
};
u32 gTalkSelects_45_1[8] = {
#include "lobby/00815120.inc"
};
u32 gTalkSelects_45_0_5[4] = {
#include "lobby/00815140.inc"
};
u32 gTalkSelects_45_0_3[4] = {
#include "lobby/00815150.inc"
};
u32 gTalkSelects_45_0_1[4] = {
#include "lobby/00815160.inc"
};
u32 gTalkSelects_45_0[8] = {
#include "lobby/00815170.inc"
};
TalkSel gTalkSelects_45[4] = {
#include "lobby/00815190.inc"
};
u32 D_008151B0[28] = {
#include "lobby/008151B0.inc"
};
TalkGift gTalkGifts_45[2] = {
#include "lobby/00815220.inc"
};
TalkEvent gTalkEvents_46[6] = {
#include "lobby/00815230.inc"
};
u32 gTalkSelects_46_0_1_1[2] = {
#include "lobby/00815248.inc"
};
u32 gTalkSelects_46_0_1_3[2] = {
#include "lobby/00815250.inc"
};
u32 gTalkSelects_46_1_1_1[2] = {
#include "lobby/00815258.inc"
};
u32 gTalkSelects_46_1_1_3[2] = {
#include "lobby/00815260.inc"
};
u32 gTalkSelects_46_2_1_1[2] = {
#include "lobby/00815268.inc"
};
u32 gTalkSelects_46_2_1_3[4] = {
#include "lobby/00815270.inc"
};
u32 gTalkSelects_46_2_1[4] = {
#include "lobby/00815280.inc"
};
u32 gTalkSelects_46_2[4] = {
    0, (u32)gTalkSelects_46_2_1, 0, 0,
};
u32 gTalkSelects_46_1_1[4] = {
#include "lobby/008152A0.inc"
};
u32 gTalkSelects_46_1[4] = {
    0, (u32)gTalkSelects_46_1_1, 0, 0,
};
u32 gTalkSelects_46_0_1[4] = {
#include "lobby/008152C0.inc"
};
u32 gTalkSelects_46_0[4] = {
    0, (u32)gTalkSelects_46_0_1, 0, 0,
};
TalkSel gTalkSelects_46[4] = {
#include "lobby/008152E0.inc"
};
u32 D_00815300[16] = {
#include "lobby/00815300.inc"
};
TalkGift gTalkGifts_46[2] = {
#include "lobby/00815340.inc"
};
TalkEvent gTalkEvents_47[4] = {
#include "lobby/00815350.inc"
};
u32 gTalkSelects_47_0_1_1[2] = {
#include "lobby/00815360.inc"
};
u32 gTalkSelects_47_1_1_1[2] = {
#include "lobby/00815368.inc"
};
u32 gTalkSelects_47_2_1_1[2] = {
#include "lobby/00815370.inc"
};
u32 gTalkSelects_47_2_1[2] = {
    0, (u32)gTalkSelects_47_2_1_1,
};
u32 gTalkSelects_47_2[2] = {
    0, (u32)gTalkSelects_47_2_1,
};
u32 gTalkSelects_47_1_1[2] = {
    0, (u32)gTalkSelects_47_1_1_1,
};
u32 gTalkSelects_47_1[2] = {
    0, (u32)gTalkSelects_47_1_1,
};
u32 gTalkSelects_47_0_1[2] = {
    0, (u32)gTalkSelects_47_0_1_1,
};
u32 gTalkSelects_47_0[4] = {
    0, (u32)gTalkSelects_47_0_1, 0, 0,
};
TalkSel gTalkSelects_47[4] = {
#include "lobby/008153B0.inc"
};
u32 D_008153D0[12] = {
#include "lobby/008153D0.inc"
};
TalkEvent gTalkEvents_48[2] = {
#include "lobby/00815400.inc"
};
u32 gTalkSelects_48_0_1_1[2] = {
#include "lobby/00815408.inc"
};
u32 gTalkSelects_48_0_1_3[2] = {
#include "lobby/00815410.inc"
};
u32 gTalkSelects_48_0_3_1[2] = {
#include "lobby/00815418.inc"
};
u32 gTalkSelects_48_0_3_3[2] = {
#include "lobby/00815420.inc"
};
u32 gTalkSelects_48_0_5_1[2] = {
#include "lobby/00815428.inc"
};
u32 gTalkSelects_48_0_5_3[2] = {
#include "lobby/00815430.inc"
};
u32 gTalkSelects_48_1_1_1[2] = {
#include "lobby/00815438.inc"
};
u32 gTalkSelects_48_1_1_3[2] = {
#include "lobby/00815440.inc"
};
u32 gTalkSelects_48_1_3_1[2] = {
#include "lobby/00815448.inc"
};
u32 gTalkSelects_48_1_3_3[2] = {
    (u32)((u8 *)gHeapBlocks + 0x16E70), 0,
};
u32 gTalkSelects_48_1_5_1[2] = {
#include "lobby/00815458.inc"
};
u32 gTalkSelects_48_1_5_3[2] = {
#include "lobby/00815460.inc"
};
u32 gTalkSelects_48_2_1_1[2] = {
#include "lobby/00815468.inc"
};
u32 gTalkSelects_48_2_1_3[2] = {
#include "lobby/00815470.inc"
};
u32 gTalkSelects_48_2_3_1[2] = {
#include "lobby/00815478.inc"
};
u32 gTalkSelects_48_2_3_3[2] = {
#include "lobby/00815480.inc"
};
u32 gTalkSelects_48_2_5_1[2] = {
#include "lobby/00815488.inc"
};
u32 gTalkSelects_48_2_5_3[4] = {
#include "lobby/00815490.inc"
};
u32 gTalkSelects_48_2_5[4] = {
#include "lobby/008154A0.inc"
};
u32 gTalkSelects_48_2_3[4] = {
#include "lobby/008154B0.inc"
};
u32 gTalkSelects_48_2_1[4] = {
#include "lobby/008154C0.inc"
};
u32 gTalkSelects_48_2[8] = {
#include "lobby/008154D0.inc"
};
u32 gTalkSelects_48_1_5[4] = {
#include "lobby/008154F0.inc"
};
u32 gTalkSelects_48_1_3[4] = {
#include "lobby/00815500.inc"
};
u32 gTalkSelects_48_1_1[4] = {
#include "lobby/00815510.inc"
};
u32 gTalkSelects_48_1[8] = {
#include "lobby/00815520.inc"
};
u32 gTalkSelects_48_0_5[4] = {
#include "lobby/00815540.inc"
};
u32 gTalkSelects_48_0_3[4] = {
#include "lobby/00815550.inc"
};
u32 gTalkSelects_48_0_1[4] = {
#include "lobby/00815560.inc"
};
u32 gTalkSelects_48_0[8] = {
#include "lobby/00815570.inc"
};
TalkSel gTalkSelects_48[4] = {
#include "lobby/00815590.inc"
};
u32 D_008155B0[78] = {
#include "lobby/008155B0.inc"
};
TalkEvent gTalkEvents_49[2] = {
#include "lobby/008156E8.inc"
};
u32 gTalkSelects_49_0_1_1[2] = {
#include "lobby/008156F0.inc"
};
u32 gTalkSelects_49_1_1_1[2] = {
#include "lobby/008156F8.inc"
};
u32 gTalkSelects_49_2_1_1[2] = {
#include "lobby/00815700.inc"
};
u32 gTalkSelects_49_2_1[2] = {
    0, (u32)gTalkSelects_49_2_1_1,
};
u32 gTalkSelects_49_2[2] = {
    0, (u32)gTalkSelects_49_2_1,
};
u32 gTalkSelects_49_1_1[2] = {
    0, (u32)gTalkSelects_49_1_1_1,
};
u32 gTalkSelects_49_1[2] = {
    0, (u32)gTalkSelects_49_1_1,
};
u32 gTalkSelects_49_0_1[2] = {
    0, (u32)gTalkSelects_49_0_1_1,
};
u32 gTalkSelects_49_0[4] = {
    0, (u32)gTalkSelects_49_0_1, 0, 0,
};
TalkSel gTalkSelects_49[4] = {
#include "lobby/00815740.inc"
};
u32 D_00815760[22] = {
#include "lobby/00815760.inc"
};
TalkEvent gTalkEvents_50[2] = {
#include "lobby/008157B8.inc"
};
u32 gTalkSelects_50_0_1_1[2] = {
#include "lobby/008157C0.inc"
};
u32 gTalkSelects_50_0_1_3[2] = {
    (u32)((u8 *)gHeapBlocks + 0x16E73), 0,
};
u32 gTalkSelects_50_1_1_1[2] = {
#include "lobby/008157D0.inc"
};
u32 gTalkSelects_50_1_1_3[2] = {
#include "lobby/008157D8.inc"
};
u32 gTalkSelects_50_1_3_1[2] = {
#include "lobby/008157E0.inc"
};
u32 gTalkSelects_50_1_3_3[2] = {
#include "lobby/008157E8.inc"
};
u32 gTalkSelects_50_1_5_1[2] = {
#include "lobby/008157F0.inc"
};
u32 gTalkSelects_50_1_5_3[2] = {
#include "lobby/008157F8.inc"
};
u32 gTalkSelects_50_2_1_1[2] = {
#include "lobby/00815800.inc"
};
u32 gTalkSelects_50_2_1_3[2] = {
#include "lobby/00815808.inc"
};
u32 gTalkSelects_50_2_3_1[2] = {
#include "lobby/00815810.inc"
};
u32 gTalkSelects_50_2_3_3[2] = {
#include "lobby/00815818.inc"
};
u32 gTalkSelects_50_2_5_1[4] = {
#include "lobby/00815820.inc"
};
u32 gTalkSelects_50_2_5[4] = {
#include "lobby/00815830.inc"
};
u32 gTalkSelects_50_2_3[4] = {
#include "lobby/00815840.inc"
};
u32 gTalkSelects_50_2_1[4] = {
#include "lobby/00815850.inc"
};
u32 gTalkSelects_50_2[8] = {
#include "lobby/00815860.inc"
};
u32 gTalkSelects_50_1_5[4] = {
#include "lobby/00815880.inc"
};
u32 gTalkSelects_50_1_3[4] = {
#include "lobby/00815890.inc"
};
u32 gTalkSelects_50_1_1[4] = {
#include "lobby/008158A0.inc"
};
u32 gTalkSelects_50_1[8] = {
#include "lobby/008158B0.inc"
};
u32 gTalkSelects_50_0_1[4] = {
#include "lobby/008158D0.inc"
};
u32 gTalkSelects_50_0[4] = {
    0, (u32)gTalkSelects_50_0_1, 0, 0,
};
TalkSel gTalkSelects_50[4] = {
#include "lobby/008158F0.inc"
};
u32 D_00815910[42] = {
#include "lobby/00815910.inc"
};
TalkEvent gTalkEvents_51[2] = {
#include "lobby/008159B8.inc"
};
u32 gTalkSelects_51_0_1_1[2] = {
#include "lobby/008159C0.inc"
};
u32 gTalkSelects_51_1_1_1[2] = {
#include "lobby/008159C8.inc"
};
u32 gTalkSelects_51_2_1_1[2] = {
#include "lobby/008159D0.inc"
};
u32 gTalkSelects_51_2_1[2] = {
    0, (u32)gTalkSelects_51_2_1_1,
};
u32 gTalkSelects_51_2[2] = {
    0, (u32)gTalkSelects_51_2_1,
};
u32 gTalkSelects_51_1_1[2] = {
    0, (u32)gTalkSelects_51_1_1_1,
};
u32 gTalkSelects_51_1[2] = {
    0, (u32)gTalkSelects_51_1_1,
};
u32 gTalkSelects_51_0_1[2] = {
    0, (u32)gTalkSelects_51_0_1_1,
};
u32 gTalkSelects_51_0[4] = {
    0, (u32)gTalkSelects_51_0_1, 0, 0,
};
TalkSel gTalkSelects_51[4] = {
#include "lobby/00815A10.inc"
};
u32 D_00815A30[12] = {
#include "lobby/00815A30.inc"
};
TalkEvent gTalkEvents_52[2] = {
#include "lobby/00815A60.inc"
};
u32 gTalkSelects_52_0_1_1[2] = {
#include "lobby/00815A68.inc"
};
u32 gTalkSelects_52_0_1[2] = {
    0, (u32)gTalkSelects_52_0_1_1,
};
u32 gTalkSelects_52_0[2] = {
    0, (u32)gTalkSelects_52_0_1,
};
TalkSel gTalkSelects_52[2] = {
    {0, (void *)gTalkSelects_52_0},
    {0, NULL},
};
u32 D_00815A90[6] = {
#include "lobby/00815A90.inc"
};
TalkEvent gTalkEvents_53[2] = {
#include "lobby/00815AA8.inc"
};
u32 gTalkSelects_53_1_1_1[2] = {
#include "lobby/00815AB0.inc"
};
u32 gTalkSelects_53_1_1[2] = {
    0, (u32)gTalkSelects_53_1_1_1,
};
u32 gTalkSelects_53_1[2] = {
    0, (u32)gTalkSelects_53_1_1,
};
TalkSel gTalkSelects_53 = {0, (void *)gTalkSelects_53_1};
u32 D_00815AD0[6] = {
#include "lobby/00815AD0.inc"
};
TalkEvent gTalkEvents_54[2] = {
#include "lobby/00815AE8.inc"
};
u32 gTalkSelects_54_1_1_1[2] = {
#include "lobby/00815AF0.inc"
};
u32 gTalkSelects_54_1_1[2] = {
    0, (u32)gTalkSelects_54_1_1_1,
};
u32 gTalkSelects_54_1[2] = {
    0, (u32)gTalkSelects_54_1_1,
};
TalkSel gTalkSelects_54 = {0, (void *)gTalkSelects_54_1};
u32 D_00815B10[8] = {
#include "lobby/00815B10.inc"
};
TalkEvent gTalkEvents_55[8] = {
#include "lobby/00815B30.inc"
};
u32 gTalkSelects_55_0_1_1[2] = {
#include "lobby/00815B50.inc"
};
u32 gTalkSelects_55_1_1_1[2] = {
#include "lobby/00815B58.inc"
};
u32 gTalkSelects_55_2_1_1[2] = {
#include "lobby/00815B60.inc"
};
u32 gTalkSelects_55_3_1_1[2] = {
#include "lobby/00815B68.inc"
};
u32 gTalkSelects_55_3_1_3[2] = {
#include "lobby/00815B70.inc"
};
u32 gTalkSelects_55_4_1_1[2] = {
#include "lobby/00815B78.inc"
};
u32 gTalkSelects_55_4_1[2] = {
    0, (u32)gTalkSelects_55_4_1_1,
};
u32 gTalkSelects_55_4[2] = {
    0, (u32)gTalkSelects_55_4_1,
};
u32 gTalkSelects_55_3_1[4] = {
#include "lobby/00815B90.inc"
};
u32 gTalkSelects_55_3[2] = {
    0, (u32)gTalkSelects_55_3_1,
};
u32 gTalkSelects_55_2_1[2] = {
    0, (u32)gTalkSelects_55_2_1_1,
};
u32 gTalkSelects_55_2[2] = {
    0, (u32)gTalkSelects_55_2_1,
};
u32 gTalkSelects_55_1_1[2] = {
    0, (u32)gTalkSelects_55_1_1_1,
};
u32 gTalkSelects_55_1[2] = {
    0, (u32)gTalkSelects_55_1_1,
};
u32 gTalkSelects_55_0_1[2] = {
    0, (u32)gTalkSelects_55_0_1_1,
};
u32 gTalkSelects_55_0[4] = {
    0, (u32)gTalkSelects_55_0_1, 0, 0,
};
TalkSel gTalkSelects_55[6] = {
#include "lobby/00815BE0.inc"
};
u32 D_00815C10[56] = {
#include "lobby/00815C10.inc"
};
TalkGift gTalkGifts_55[4] = {
#include "lobby/00815CF0.inc"
};
TalkEvent gTalkEvents_56[6] = {
#include "lobby/00815D10.inc"
};
u32 gTalkSelects_56_0_1_1[2] = {
#include "lobby/00815D28.inc"
};
u32 gTalkSelects_56_1_1_1[2] = {
#include "lobby/00815D30.inc"
};
u32 gTalkSelects_56_2_1_1[2] = {
#include "lobby/00815D38.inc"
};
u32 gTalkSelects_56_3_1_1[2] = {
#include "lobby/00815D40.inc"
};
u32 gTalkSelects_56_3_1[2] = {
    0, (u32)gTalkSelects_56_3_1_1,
};
u32 gTalkSelects_56_3[2] = {
    0, (u32)gTalkSelects_56_3_1,
};
u32 gTalkSelects_56_2_1[2] = {
    0, (u32)gTalkSelects_56_2_1_1,
};
u32 gTalkSelects_56_2[2] = {
    0, (u32)gTalkSelects_56_2_1,
};
u32 gTalkSelects_56_1_1[2] = {
    0, (u32)gTalkSelects_56_1_1_1,
};
u32 gTalkSelects_56_1[2] = {
    0, (u32)gTalkSelects_56_1_1,
};
u32 gTalkSelects_56_0_1[2] = {
    0, (u32)gTalkSelects_56_0_1_1,
};
u32 gTalkSelects_56_0[4] = {
    0, (u32)gTalkSelects_56_0_1, 0, 0,
};
TalkSel gTalkSelects_56[4] = {
#include "lobby/00815D90.inc"
};
u32 D_00815DB0[56] = {
#include "lobby/00815DB0.inc"
};
TalkEvent gTalkEvents_57[8] = {
#include "lobby/00815E90.inc"
};
u32 gTalkSelects_57_0_1_1[2] = {
#include "lobby/00815EB0.inc"
};
u32 gTalkSelects_57_0_3_1[2] = {
#include "lobby/00815EB8.inc"
};
u32 gTalkSelects_57_0_5_1[2] = {
#include "lobby/00815EC0.inc"
};
u32 gTalkSelects_57_1_1_1[2] = {
#include "lobby/00815EC8.inc"
};
u32 gTalkSelects_57_1_3_1[2] = {
#include "lobby/00815ED0.inc"
};
u32 gTalkSelects_57_1_5_1[2] = {
#include "lobby/00815ED8.inc"
};
u32 gTalkSelects_57_2_1_1[2] = {
#include "lobby/00815EE0.inc"
};
u32 gTalkSelects_57_2_3_1[2] = {
#include "lobby/00815EE8.inc"
};
u32 gTalkSelects_57_2_5_1[2] = {
#include "lobby/00815EF0.inc"
};
u32 gTalkSelects_57_2_5[2] = {
    0, (u32)gTalkSelects_57_2_5_1,
};
u32 gTalkSelects_57_2_3[2] = {
    0, (u32)gTalkSelects_57_2_3_1,
};
u32 gTalkSelects_57_2_1[2] = {
    0, (u32)gTalkSelects_57_2_1_1,
};
u32 gTalkSelects_57_2[6] = {
#include "lobby/00815F10.inc"
};
u32 gTalkSelects_57_1_5[2] = {
    0, (u32)gTalkSelects_57_1_5_1,
};
u32 gTalkSelects_57_1_3[2] = {
    0, (u32)gTalkSelects_57_1_3_1,
};
u32 gTalkSelects_57_1_1[2] = {
    0, (u32)gTalkSelects_57_1_1_1,
};
u32 gTalkSelects_57_1[6] = {
#include "lobby/00815F40.inc"
};
u32 gTalkSelects_57_0_5[2] = {
    0, (u32)gTalkSelects_57_0_5_1,
};
u32 gTalkSelects_57_0_3[2] = {
    0, (u32)gTalkSelects_57_0_3_1,
};
u32 gTalkSelects_57_0_1[2] = {
    0, (u32)gTalkSelects_57_0_1_1,
};
u32 gTalkSelects_57_0[8] = {
#include "lobby/00815F70.inc"
};
TalkSel gTalkSelects_57[4] = {
#include "lobby/00815F90.inc"
};
u32 D_00815FB0[30] = {
#include "lobby/00815FB0.inc"
};
TalkEvent gTalkEvents_58[2] = {
#include "lobby/00816028.inc"
};
u32 gTalkSelects_58_0_1_1[2] = {
#include "lobby/00816030.inc"
};
u32 gTalkSelects_58_1_1_1[2] = {
#include "lobby/00816038.inc"
};
u32 gTalkSelects_58_2_1_1[2] = {
#include "lobby/00816040.inc"
};
u32 gTalkSelects_58_2_1[2] = {
    0, (u32)gTalkSelects_58_2_1_1,
};
u32 gTalkSelects_58_2[2] = {
    0, (u32)gTalkSelects_58_2_1,
};
u32 gTalkSelects_58_1_1[2] = {
    0, (u32)gTalkSelects_58_1_1_1,
};
u32 gTalkSelects_58_1[2] = {
    0, (u32)gTalkSelects_58_1_1,
};
u32 gTalkSelects_58_0_1[2] = {
    0, (u32)gTalkSelects_58_0_1_1,
};
u32 gTalkSelects_58_0[4] = {
    0, (u32)gTalkSelects_58_0_1, 0, 0,
};
TalkSel gTalkSelects_58[4] = {
#include "lobby/00816080.inc"
};
u32 D_008160A0[18] = {
#include "lobby/008160A0.inc"
};
TalkEvent gTalkEvents_59[3] = {
#include "lobby/008160E8.inc"
};
u32 gTalkSelects_59_0_1_1[2] = {
#include "lobby/008160F8.inc"
};
u32 gTalkSelects_59_1_1_1[2] = {
#include "lobby/00816100.inc"
};
u32 gTalkSelects_59_2_1_1[2] = {
#include "lobby/00816108.inc"
};
u32 gTalkSelects_59_2_1_3[2] = {
#include "lobby/00816110.inc"
};
u32 gTalkSelects_59_2_3_1[2] = {
#include "lobby/00816118.inc"
};
u32 gTalkSelects_59_2_3_3[2] = {
#include "lobby/00816120.inc"
};
u32 gTalkSelects_59_2_5_1[2] = {
#include "lobby/00816128.inc"
};
u32 gTalkSelects_59_2_5_3[4] = {
#include "lobby/00816130.inc"
};
u32 gTalkSelects_59_2_5[4] = {
#include "lobby/00816140.inc"
};
u32 gTalkSelects_59_2_3[4] = {
#include "lobby/00816150.inc"
};
u32 gTalkSelects_59_2_1[4] = {
#include "lobby/00816160.inc"
};
u32 gTalkSelects_59_2[6] = {
#include "lobby/00816170.inc"
};
u32 gTalkSelects_59_1_1[2] = {
    0, (u32)gTalkSelects_59_1_1_1,
};
u32 gTalkSelects_59_1[2] = {
    0, (u32)gTalkSelects_59_1_1,
};
u32 gTalkSelects_59_0_1[2] = {
    0, (u32)gTalkSelects_59_0_1_1,
};
u32 gTalkSelects_59_0[4] = {
    0, (u32)gTalkSelects_59_0_1, 0, 0,
};
TalkSel gTalkSelects_59[4] = {
#include "lobby/008161B0.inc"
};
u32 D_008161D0[48] = {
#include "lobby/008161D0.inc"
};
TalkEvent gTalkEvents_60[4] = {
#include "lobby/00816290.inc"
};
u32 gTalkSelects_60_0_1_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x26E6D), 0,
};
u32 gTalkSelects_60_1_1_1[2] = {
#include "lobby/008162A8.inc"
};
u32 gTalkSelects_60_2_1_1[2] = {
#include "lobby/008162B0.inc"
};
u32 gTalkSelects_60_2_1_3[2] = {
#include "lobby/008162B8.inc"
};
u32 gTalkSelects_60_2_3_1[2] = {
#include "lobby/008162C0.inc"
};
u32 gTalkSelects_60_2_3_3[2] = {
#include "lobby/008162C8.inc"
};
u32 gTalkSelects_60_2_5_1[2] = {
#include "lobby/008162D0.inc"
};
u32 gTalkSelects_60_2_5_3[2] = {
#include "lobby/008162D8.inc"
};
u32 gTalkSelects_60_2_5[4] = {
#include "lobby/008162E0.inc"
};
u32 gTalkSelects_60_2_3[4] = {
#include "lobby/008162F0.inc"
};
u32 gTalkSelects_60_2_1[4] = {
#include "lobby/00816300.inc"
};
u32 gTalkSelects_60_2[6] = {
#include "lobby/00816310.inc"
};
u32 gTalkSelects_60_1_1[2] = {
    0, (u32)gTalkSelects_60_1_1_1,
};
u32 gTalkSelects_60_1[2] = {
    0, (u32)gTalkSelects_60_1_1,
};
u32 gTalkSelects_60_0_1[2] = {
    0, (u32)gTalkSelects_60_0_1_1,
};
u32 gTalkSelects_60_0[4] = {
    0, (u32)gTalkSelects_60_0_1, 0, 0,
};
TalkSel gTalkSelects_60[4] = {
#include "lobby/00816350.inc"
};
u32 D_00816370[46] = {
#include "lobby/00816370.inc"
};
TalkEvent gTalkEvents_61[2] = {
#include "lobby/00816428.inc"
};
u32 gTalkSelects_61_0_1_1[2] = {
#include "lobby/00816430.inc"
};
u32 gTalkSelects_61_1_1_1[2] = {
#include "lobby/00816438.inc"
};
u32 gTalkSelects_61_2_1_1[2] = {
#include "lobby/00816440.inc"
};
u32 gTalkSelects_61_2_3_1[2] = {
#include "lobby/00816448.inc"
};
u32 gTalkSelects_61_2_5[2] = {
    0, (u32)D_008BF8B0,
};
u32 gTalkSelects_61_2_3[2] = {
    0, (u32)gTalkSelects_61_2_3_1,
};
u32 gTalkSelects_61_2_1[4] = {
    0, (u32)gTalkSelects_61_2_1_1, 0, 0,
};
u32 gTalkSelects_61_2[6] = {
#include "lobby/00816470.inc"
};
u32 gTalkSelects_61_1_1[2] = {
    0, (u32)gTalkSelects_61_1_1_1,
};
u32 gTalkSelects_61_1[2] = {
    0, (u32)gTalkSelects_61_1_1,
};
u32 gTalkSelects_61_0_1[2] = {
    0, (u32)gTalkSelects_61_0_1_1,
};
u32 gTalkSelects_61_0[4] = {
    0, (u32)gTalkSelects_61_0_1, 0, 0,
};
TalkSel gTalkSelects_61[4] = {
#include "lobby/008164B0.inc"
};
u32 D_008164D0[36] = {
#include "lobby/008164D0.inc"
};
TalkEvent gTalkEvents_62[2] = {
#include "lobby/00816560.inc"
};
u32 gTalkSelects_62_0_1_1[2] = {
#include "lobby/00816568.inc"
};
u32 gTalkSelects_62_1_1_1[2] = {
#include "lobby/00816570.inc"
};
u32 gTalkSelects_62_2_1_1[2] = {
#include "lobby/00816578.inc"
};
u32 gTalkSelects_62_2_1_3[4] = {
#include "lobby/00816580.inc"
};
u32 gTalkSelects_62_2_1[4] = {
#include "lobby/00816590.inc"
};
u32 gTalkSelects_62_2[2] = {
    0, (u32)gTalkSelects_62_2_1,
};
u32 gTalkSelects_62_1_1[2] = {
    0, (u32)gTalkSelects_62_1_1_1,
};
u32 gTalkSelects_62_1[2] = {
    0, (u32)gTalkSelects_62_1_1,
};
u32 gTalkSelects_62_0_1[2] = {
    0, (u32)gTalkSelects_62_0_1_1,
};
u32 gTalkSelects_62_0[4] = {
    0, (u32)gTalkSelects_62_0_1, 0, 0,
};
TalkSel gTalkSelects_62[4] = {
#include "lobby/008165D0.inc"
};
u32 D_008165F0[26] = {
#include "lobby/008165F0.inc"
};
TalkEvent gTalkEvents_63[2] = {
#include "lobby/00816658.inc"
};
u32 gTalkSelects_63_1_1_1[2] = {
#include "lobby/00816660.inc"
};
u32 gTalkSelects_63_1_1[2] = {
    0, (u32)gTalkSelects_63_1_1_1,
};
u32 gTalkSelects_63_1[2] = {
    0, (u32)gTalkSelects_63_1_1,
};
TalkSel gTalkSelects_63 = {0, (void *)gTalkSelects_63_1};
u32 D_00816680[8] = {
#include "lobby/00816680.inc"
};
TalkEvent gTalkEvents_64[14] = {
#include "lobby/008166A0.inc"
};
u32 gTalkSelects_64_0_1_1[2] = {
#include "lobby/008166D8.inc"
};
u32 gTalkSelects_64_1_1_1[2] = {
#include "lobby/008166E0.inc"
};
u32 gTalkSelects_64_2_1_1[2] = {
#include "lobby/008166E8.inc"
};
u32 gTalkSelects_64_3_1_1[2] = {
#include "lobby/008166F0.inc"
};
u32 gTalkSelects_64_4_1_1[2] = {
#include "lobby/008166F8.inc"
};
u32 gTalkSelects_64_4_1[2] = {
    0, (u32)gTalkSelects_64_4_1_1,
};
u32 gTalkSelects_64_4[2] = {
    0, (u32)gTalkSelects_64_4_1,
};
u32 gTalkSelects_64_3_1[2] = {
    0, (u32)gTalkSelects_64_3_1_1,
};
u32 gTalkSelects_64_3[2] = {
    0, (u32)gTalkSelects_64_3_1,
};
u32 gTalkSelects_64_2_1[2] = {
    0, (u32)gTalkSelects_64_2_1_1,
};
u32 gTalkSelects_64_2[2] = {
    0, (u32)gTalkSelects_64_2_1,
};
u32 gTalkSelects_64_1_1[2] = {
    0, (u32)gTalkSelects_64_1_1_1,
};
u32 gTalkSelects_64_1[2] = {
    0, (u32)gTalkSelects_64_1_1,
};
u32 gTalkSelects_64_0_1[2] = {
    0, (u32)gTalkSelects_64_0_1_1,
};
u32 gTalkSelects_64_0[2] = {
    0, (u32)gTalkSelects_64_0_1,
};
TalkSel gTalkSelects_64[6] = {
#include "lobby/00816750.inc"
};
u32 D_00816780[108] = {
#include "lobby/00816780.inc"
};
TalkGift gTalkGifts_64[6] = {
#include "lobby/00816930.inc"
};
TalkEvent gTalkEvents_65[6] = {
#include "lobby/00816960.inc"
};
u32 gTalkSelects_65_1_1_1[2] = {
#include "lobby/00816978.inc"
};
u32 gTalkSelects_65_1_1_3[4] = {
#include "lobby/00816980.inc"
};
u32 gTalkSelects_65_1_1[4] = {
#include "lobby/00816990.inc"
};
u32 gTalkSelects_65_1[2] = {
    0, (u32)gTalkSelects_65_1_1,
};
TalkSel gTalkSelects_65 = {0, (void *)gTalkSelects_65_1};
u32 D_008169B0[40] = {
#include "lobby/008169B0.inc"
};
TalkGift gTalkGifts_65[4] = {
#include "lobby/00816A50.inc"
};
TalkEvent gTalkEvents_66[4] = {
#include "lobby/00816A70.inc"
};
u32 gTalkSelects_66_1_1_1[2] = {
#include "lobby/00816A80.inc"
};
u32 gTalkSelects_66_1_1[2] = {
    0, (u32)gTalkSelects_66_1_1_1,
};
u32 gTalkSelects_66_1[2] = {
    0, (u32)gTalkSelects_66_1_1,
};
TalkSel gTalkSelects_66 = {0, (void *)gTalkSelects_66_1};
u32 D_00816AA0[10] = {
#include "lobby/00816AA0.inc"
};
TalkEvent gTalkEvents_67[2] = {
#include "lobby/00816AC8.inc"
};
u32 gTalkSelects_67_1_1[2] = {
    0, (u32)D_008BF8D8,
};
u32 gTalkSelects_67_1[2] = {
    0, (u32)gTalkSelects_67_1_1,
};
TalkSel gTalkSelects_67 = {0, (void *)gTalkSelects_67_1};
u32 D_00816AE8[2] = {
#include "lobby/00816AE8.inc"
};
TalkEvent gTalkEvents_68[2] = {
#include "lobby/00816AF0.inc"
};
u32 gTalkSelects_68_1_1[2] = {
    0, (u32)D_008BF8E8,
};
u32 gTalkSelects_68_1[2] = {
    0, (u32)gTalkSelects_68_1_1,
};
TalkSel gTalkSelects_68 = {0, (void *)gTalkSelects_68_1};
u32 D_00816B10[2] = {
#include "lobby/00816B10.inc"
};
TalkEvent gTalkEvents_69[2] = {
#include "lobby/00816B18.inc"
};
u32 gTalkSelects_69_0_1[2] = {
    0, (u32)D_008BF8F8,
};
u32 gTalkSelects_69_0[2] = {
    0, (u32)gTalkSelects_69_0_1,
};
TalkSel gTalkSelects_69[2] = {
    {0, (void *)gTalkSelects_69_0},
    {0, NULL},
};
u32 D_00816B40[12] = {
#include "lobby/00816B40.inc"
};
TalkEvent gTalkEvents_70[2] = {
#include "lobby/00816B70.inc"
};
u32 gTalkSelects_70_1_1[2] = {
    0, (u32)D_008BF908,
};
u32 gTalkSelects_70_1[2] = {
    0, (u32)gTalkSelects_70_1_1,
};
TalkSel gTalkSelects_70 = {0, (void *)gTalkSelects_70_1};
u32 D_00816B90[12] = {
#include "lobby/00816B90.inc"
};
TalkEvent gTalkEvents_71[2] = {
#include "lobby/00816BC0.inc"
};
u32 gTalkSelects_71_1_1[2] = {
    0, (u32)D_008BF918,
};
u32 gTalkSelects_71_1[2] = {
    0, (u32)gTalkSelects_71_1_1,
};
TalkSel gTalkSelects_71 = {0, (void *)gTalkSelects_71_1};
u32 D_00816BE0[12] = {
#include "lobby/00816BE0.inc"
};
TalkEvent gTalkEvents_72[2] = {
#include "lobby/00816C10.inc"
};
u32 gTalkSelects_72_1_1[2] = {
    0, (u32)D_008BF928,
};
u32 gTalkSelects_72_1[2] = {
    0, (u32)gTalkSelects_72_1_1,
};
TalkSel gTalkSelects_72 = {0, (void *)gTalkSelects_72_1};
u32 D_00816C30[4] = {
#include "lobby/00816C30.inc"
};
TalkEvent gTalkEvents_73[6] = {
#include "lobby/00816C40.inc"
};
u32 gTalkSelects_73_0_1_1[2] = {
#include "lobby/00816C58.inc"
};
u32 gTalkSelects_73_1_1_1[2] = {
#include "lobby/00816C60.inc"
};
u32 gTalkSelects_73_2_1_1[2] = {
#include "lobby/00816C68.inc"
};
u32 gTalkSelects_73_2_1[2] = {
    0, (u32)gTalkSelects_73_2_1_1,
};
u32 gTalkSelects_73_2[2] = {
    0, (u32)gTalkSelects_73_2_1,
};
u32 gTalkSelects_73_1_1[2] = {
    0, (u32)gTalkSelects_73_1_1_1,
};
u32 gTalkSelects_73_1[2] = {
    0, (u32)gTalkSelects_73_1_1,
};
u32 gTalkSelects_73_0_1[2] = {
    0, (u32)gTalkSelects_73_0_1_1,
};
u32 gTalkSelects_73_0[2] = {
    0, (u32)gTalkSelects_73_0_1,
};
TalkSel gTalkSelects_73[4] = {
#include "lobby/00816CA0.inc"
};
u32 D_00816CC0[28] = {
#include "lobby/00816CC0.inc"
};
TalkGift gTalkGifts_73[2] = {
#include "lobby/00816D30.inc"
};
TalkEvent gTalkEvents_74[2] = {
#include "lobby/00816D40.inc"
};
u32 gTalkSelects_74_0_1_1[2] = {
#include "lobby/00816D48.inc"
};
u32 gTalkSelects_74_1_1_1[2] = {
#include "lobby/00816D50.inc"
};
u32 gTalkSelects_74_2_1_1[2] = {
#include "lobby/00816D58.inc"
};
u32 gTalkSelects_74_2_1[2] = {
    0, (u32)gTalkSelects_74_2_1_1,
};
u32 gTalkSelects_74_2[2] = {
    0, (u32)gTalkSelects_74_2_1,
};
u32 gTalkSelects_74_1_1[2] = {
    0, (u32)gTalkSelects_74_1_1_1,
};
u32 gTalkSelects_74_1[2] = {
    0, (u32)gTalkSelects_74_1_1,
};
u32 gTalkSelects_74_0_1[2] = {
    0, (u32)gTalkSelects_74_0_1_1,
};
u32 gTalkSelects_74_0[2] = {
    0, (u32)gTalkSelects_74_0_1,
};
TalkSel gTalkSelects_74[4] = {
#include "lobby/00816D90.inc"
};
u32 D_00816DB0[18] = {
#include "lobby/00816DB0.inc"
};
TalkEvent gTalkEvents_75[2] = {
#include "lobby/00816DF8.inc"
};
u32 gTalkSelects_75_0_1_1[2] = {
#include "lobby/00816E00.inc"
};
u32 gTalkSelects_75_1_1_1[2] = {
#include "lobby/00816E08.inc"
};
u32 gTalkSelects_75_2_1_1[2] = {
#include "lobby/00816E10.inc"
};
u32 gTalkSelects_75_2_1[2] = {
    0, (u32)gTalkSelects_75_2_1_1,
};
u32 gTalkSelects_75_2[2] = {
    0, (u32)gTalkSelects_75_2_1,
};
u32 gTalkSelects_75_1_1[2] = {
    0, (u32)gTalkSelects_75_1_1_1,
};
u32 gTalkSelects_75_1[2] = {
    0, (u32)gTalkSelects_75_1_1,
};
u32 gTalkSelects_75_0_1[2] = {
    0, (u32)gTalkSelects_75_0_1_1,
};
u32 gTalkSelects_75_0[4] = {
    0, (u32)gTalkSelects_75_0_1, 0, 0,
};
TalkSel gTalkSelects_75[4] = {
#include "lobby/00816E50.inc"
};
u32 D_00816E70[18] = {
#include "lobby/00816E70.inc"
};
TalkEvent gTalkEvents_76[2] = {
#include "lobby/00816EB8.inc"
};
u32 gTalkSelects_76_0_1_1[2] = {
#include "lobby/00816EC0.inc"
};
u32 gTalkSelects_76_1_1_1[2] = {
#include "lobby/00816EC8.inc"
};
u32 gTalkSelects_76_2_1_1[2] = {
#include "lobby/00816ED0.inc"
};
u32 gTalkSelects_76_2_1[2] = {
    0, (u32)gTalkSelects_76_2_1_1,
};
u32 gTalkSelects_76_2[2] = {
    0, (u32)gTalkSelects_76_2_1,
};
u32 gTalkSelects_76_1_1[2] = {
    0, (u32)gTalkSelects_76_1_1_1,
};
u32 gTalkSelects_76_1[2] = {
    0, (u32)gTalkSelects_76_1_1,
};
u32 gTalkSelects_76_0_1[2] = {
    0, (u32)gTalkSelects_76_0_1_1,
};
u32 gTalkSelects_76_0[4] = {
    0, (u32)gTalkSelects_76_0_1, 0, 0,
};
TalkSel gTalkSelects_76[4] = {
#include "lobby/00816F10.inc"
};
u32 D_00816F30[28] = {
#include "lobby/00816F30.inc"
};
TalkEvent gTalkEvents_77[6] = {
#include "lobby/00816FA0.inc"
};
u32 gTalkSelects_77_0_1_1[2] = {
#include "lobby/00816FB8.inc"
};
u32 gTalkSelects_77_1_1[2] = {
    0, (u32)D_008BF950,
};
u32 gTalkSelects_77_1[2] = {
    0, (u32)gTalkSelects_77_1_1,
};
u32 gTalkSelects_77_0_1[2] = {
    0, (u32)gTalkSelects_77_0_1_1,
};
u32 gTalkSelects_77_0[2] = {
    0, (u32)gTalkSelects_77_0_1,
};
TalkSel gTalkSelects_77[2] = {
#include "lobby/00816FE0.inc"
};
u32 D_00816FF0[36] = {
#include "lobby/00816FF0.inc"
};
TalkGift gTalkGifts_77[2] = {
#include "lobby/00817080.inc"
};
TalkEvent gTalkEvents_78[12] = {
#include "lobby/00817090.inc"
};
u32 gTalkSelects_78_0_1_1[2] = {
#include "lobby/008170C0.inc"
};
u32 gTalkSelects_78_1_1_1[2] = {
#include "lobby/008170C8.inc"
};
u32 gTalkSelects_78_1_1[2] = {
    0, (u32)gTalkSelects_78_1_1_1,
};
u32 gTalkSelects_78_1[2] = {
    0, (u32)gTalkSelects_78_1_1,
};
u32 gTalkSelects_78_0_1[2] = {
    0, (u32)gTalkSelects_78_0_1_1,
};
u32 gTalkSelects_78_0[2] = {
    0, (u32)gTalkSelects_78_0_1,
};
TalkSel gTalkSelects_78[2] = {
#include "lobby/008170F0.inc"
};
u32 D_00817100[80] = {
#include "lobby/00817100.inc"
};
TalkGift gTalkGifts_78[6] = {
#include "lobby/00817240.inc"
};
TalkEvent gTalkEvents_79[6] = {
#include "lobby/00817270.inc"
};
u32 gTalkSelects_79_0_1_1[2] = {
#include "lobby/00817288.inc"
};
u32 gTalkSelects_79_1_1_1[2] = {
#include "lobby/00817290.inc"
};
u32 gTalkSelects_79_1_1[2] = {
    0, (u32)gTalkSelects_79_1_1_1,
};
u32 gTalkSelects_79_1[2] = {
    0, (u32)gTalkSelects_79_1_1,
};
u32 gTalkSelects_79_0_1[2] = {
    0, (u32)gTalkSelects_79_0_1_1,
};
u32 gTalkSelects_79_0[4] = {
    0, (u32)gTalkSelects_79_0_1, 0, 0,
};
TalkSel gTalkSelects_79[2] = {
#include "lobby/008172C0.inc"
};
u32 D_008172D0[36] = {
#include "lobby/008172D0.inc"
};
TalkEvent gTalkEvents_80[4] = {
#include "lobby/00817360.inc"
};
u32 gTalkSelects_80_0_1_1[2] = {
#include "lobby/00817370.inc"
};
u32 gTalkSelects_80_1_1_1[2] = {
#include "lobby/00817378.inc"
};
u32 gTalkSelects_80_1_1[2] = {
    0, (u32)gTalkSelects_80_1_1_1,
};
u32 gTalkSelects_80_1[2] = {
    0, (u32)gTalkSelects_80_1_1,
};
u32 gTalkSelects_80_0_1[2] = {
    0, (u32)gTalkSelects_80_0_1_1,
};
u32 gTalkSelects_80_0[2] = {
    0, (u32)gTalkSelects_80_0_1,
};
TalkSel gTalkSelects_80[2] = {
#include "lobby/008173A0.inc"
};
u32 D_008173B0[10] = {
#include "lobby/008173B0.inc"
};
TalkEvent gTalkEvents_81[2] = {
#include "lobby/008173D8.inc"
};
u32 gTalkSelects_81_0_1_1[2] = {
#include "lobby/008173E0.inc"
};
u32 gTalkSelects_81_1_1[2] = {
    0, (u32)D_008BF968,
};
u32 gTalkSelects_81_1[2] = {
    0, (u32)gTalkSelects_81_1_1,
};
u32 gTalkSelects_81_0_1[2] = {
    0, (u32)gTalkSelects_81_0_1_1,
};
u32 gTalkSelects_81_0[4] = {
    0, (u32)gTalkSelects_81_0_1, 0, 0,
};
TalkSel gTalkSelects_81[2] = {
#include "lobby/00817410.inc"
};
u32 D_00817420[14] = {
#include "lobby/00817420.inc"
};
TalkEvent gTalkEvents_82[2] = {
#include "lobby/00817458.inc"
};
u32 gTalkSelects_82_0_1_1[2] = {
#include "lobby/00817460.inc"
};
u32 gTalkSelects_82_1_1[2] = {
    0, (u32)D_008BF978,
};
u32 gTalkSelects_82_1[2] = {
    0, (u32)gTalkSelects_82_1_1,
};
u32 gTalkSelects_82_0_1[2] = {
    0, (u32)gTalkSelects_82_0_1_1,
};
u32 gTalkSelects_82_0[4] = {
    0, (u32)gTalkSelects_82_0_1, 0, 0,
};
TalkSel gTalkSelects_82[2] = {
#include "lobby/00817490.inc"
};
u32 D_008174A0[14] = {
#include "lobby/008174A0.inc"
};
TalkEvent gTalkEvents_83[2] = {
#include "lobby/008174D8.inc"
};
u32 gTalkSelects_83_0_1_1[2] = {
#include "lobby/008174E0.inc"
};
u32 gTalkSelects_83_1_1_1[2] = {
#include "lobby/008174E8.inc"
};
u32 gTalkSelects_83_1_1[2] = {
    0, (u32)gTalkSelects_83_1_1_1,
};
u32 gTalkSelects_83_1[2] = {
    0, (u32)gTalkSelects_83_1_1,
};
u32 gTalkSelects_83_0_1[2] = {
    0, (u32)gTalkSelects_83_0_1_1,
};
u32 gTalkSelects_83_0[2] = {
    0, (u32)gTalkSelects_83_0_1,
};
TalkSel gTalkSelects_83[2] = {
#include "lobby/00817510.inc"
};
u32 D_00817520[12] = {
#include "lobby/00817520.inc"
};
TalkEvent gTalkEvents_84[2] = {
#include "lobby/00817550.inc"
};
u32 gTalkSelects_84_0_1_1[2] = {
#include "lobby/00817558.inc"
};
u32 gTalkSelects_84_1_1_1[2] = {
#include "lobby/00817560.inc"
};
u32 gTalkSelects_84_1_1[2] = {
    0, (u32)gTalkSelects_84_1_1_1,
};
u32 gTalkSelects_84_1[2] = {
    0, (u32)gTalkSelects_84_1_1,
};
u32 gTalkSelects_84_0_1[2] = {
    0, (u32)gTalkSelects_84_0_1_1,
};
u32 gTalkSelects_84_0[4] = {
    0, (u32)gTalkSelects_84_0_1, 0, 0,
};
TalkSel gTalkSelects_84[2] = {
#include "lobby/00817590.inc"
};
u32 D_008175A0[20] = {
#include "lobby/008175A0.inc"
};
TalkEvent gTalkEvents_85[4] = {
#include "lobby/008175F0.inc"
};
u32 gTalkSelects_85_0_1_1[2] = {
#include "lobby/00817600.inc"
};
u32 gTalkSelects_85_1_1_1[2] = {
#include "lobby/00817608.inc"
};
u32 gTalkSelects_85_2_1_1[2] = {
#include "lobby/00817610.inc"
};
u32 gTalkSelects_85_2_1[2] = {
    0, (u32)gTalkSelects_85_2_1_1,
};
u32 gTalkSelects_85_2[2] = {
    0, (u32)gTalkSelects_85_2_1,
};
u32 gTalkSelects_85_1_1[2] = {
    0, (u32)gTalkSelects_85_1_1_1,
};
u32 gTalkSelects_85_1[2] = {
    0, (u32)gTalkSelects_85_1_1,
};
u32 gTalkSelects_85_0_1[2] = {
    0, (u32)gTalkSelects_85_0_1_1,
};
u32 gTalkSelects_85_0[4] = {
    0, (u32)gTalkSelects_85_0_1, 0, 0,
};
TalkSel gTalkSelects_85[4] = {
#include "lobby/00817650.inc"
};
u32 D_00817670[12] = {
#include "lobby/00817670.inc"
};
TalkEvent gTalkEvents_86[8] = {
#include "lobby/008176A0.inc"
};
u32 gTalkSelects_86_0_1_1[2] = {
#include "lobby/008176C0.inc"
};
u32 gTalkSelects_86_1_1_1[2] = {
#include "lobby/008176C8.inc"
};
u32 gTalkSelects_86_2_1_1[2] = {
#include "lobby/008176D0.inc"
};
u32 gTalkSelects_86_2_1[2] = {
    0, (u32)gTalkSelects_86_2_1_1,
};
u32 gTalkSelects_86_2[2] = {
    0, (u32)gTalkSelects_86_2_1,
};
u32 gTalkSelects_86_1_1[2] = {
    0, (u32)gTalkSelects_86_1_1_1,
};
u32 gTalkSelects_86_1[2] = {
    0, (u32)gTalkSelects_86_1_1,
};
u32 gTalkSelects_86_0_1[2] = {
    0, (u32)gTalkSelects_86_0_1_1,
};
u32 gTalkSelects_86_0[4] = {
    0, (u32)gTalkSelects_86_0_1, 0, 0,
};
TalkSel gTalkSelects_86[4] = {
#include "lobby/00817710.inc"
};
u32 D_00817730[18] = {
#include "lobby/00817730.inc"
};
TalkEvent gTalkEvents_87[2] = {
#include "lobby/00817778.inc"
};
u32 gTalkSelects_87_0_1_1[2] = {
#include "lobby/00817780.inc"
};
u32 gTalkSelects_87_0_1_3[2] = {
#include "lobby/00817788.inc"
};
u32 gTalkSelects_87_1_1_1[2] = {
#include "lobby/00817790.inc"
};
u32 gTalkSelects_87_1_3_1[2] = {
#include "lobby/00817798.inc"
};
u32 gTalkSelects_87_1_5_1[2] = {
#include "lobby/008177A0.inc"
};
u32 gTalkSelects_87_2_1_1[2] = {
#include "lobby/008177A8.inc"
};
u32 gTalkSelects_87_2_1[4] = {
#include "lobby/008177B0.inc"
};
u32 gTalkSelects_87_2[2] = {
    0, (u32)gTalkSelects_87_2_1,
};
u32 gTalkSelects_87_1_5[2] = {
    0, (u32)gTalkSelects_87_1_5_1,
};
u32 gTalkSelects_87_1_3[2] = {
    0, (u32)gTalkSelects_87_1_3_1,
};
u32 gTalkSelects_87_1_1[2] = {
    0, (u32)gTalkSelects_87_1_1_1,
};
u32 gTalkSelects_87_1[8] = {
#include "lobby/008177E0.inc"
};
u32 gTalkSelects_87_0_1[4] = {
#include "lobby/00817800.inc"
};
u32 gTalkSelects_87_0[4] = {
    0, (u32)gTalkSelects_87_0_1, 0, 0,
};
TalkSel gTalkSelects_87[4] = {
#include "lobby/00817820.inc"
};
u32 D_00817840[52] = {
#include "lobby/00817840.inc"
};
TalkEvent gTalkEvents_88[4] = {
#include "lobby/00817910.inc"
};
u32 gTalkSelects_88_0_1[2] = {
    0, (u32)D_008BF9B8,
};
u32 gTalkSelects_88_0[2] = {
    0, (u32)gTalkSelects_88_0_1,
};
TalkSel gTalkSelects_88[2] = {
    {0, (void *)gTalkSelects_88_0},
    {0, NULL},
};
u32 D_00817940[6] = {
#include "lobby/00817940.inc"
};
TalkEvent gTalkEvents_89[2] = {
#include "lobby/00817958.inc"
};
u32 gTalkSelects_89_1_1[2] = {
    0, (u32)D_008BF9C8,
};
u32 gTalkSelects_89_1[2] = {
    0, (u32)gTalkSelects_89_1_1,
};
TalkSel gTalkSelects_89 = {0, (void *)gTalkSelects_89_1};
u32 D_00817978[2] = {
#include "lobby/00817978.inc"
};
TalkEvent gTalkEvents_90[2] = {
#include "lobby/00817980.inc"
};
u32 gTalkSelects_90_1_1[2] = {
    0, (u32)D_008BF9D8,
};
u32 gTalkSelects_90_1[2] = {
    0, (u32)gTalkSelects_90_1_1,
};
TalkSel gTalkSelects_90 = {0, (void *)gTalkSelects_90_1};
u32 D_008179A0[2] = {
#include "lobby/008179A0.inc"
};
TalkEvent gTalkEvents_91[2] = {
#include "lobby/008179A8.inc"
};
u32 gTalkSelects_91_1_1[2] = {
    0, (u32)D_008BF9E8,
};
u32 gTalkSelects_91_1[2] = {
    0, (u32)gTalkSelects_91_1_1,
};
TalkSel gTalkSelects_91 = {0, (void *)gTalkSelects_91_1};
u32 D_008179C8[2] = {
#include "lobby/008179C8.inc"
};
TalkEvent gTalkEvents_92[2] = {
#include "lobby/008179D0.inc"
};
u32 gTalkSelects_92_1_1[2] = {
    0, (u32)D_008BF9F8,
};
u32 gTalkSelects_92_1[2] = {
    0, (u32)gTalkSelects_92_1_1,
};
TalkSel gTalkSelects_92 = {0, (void *)gTalkSelects_92_1};
u32 D_008179F0[2] = {
#include "lobby/008179F0.inc"
};
TalkEvent gTalkEvents_93[2] = {
#include "lobby/008179F8.inc"
};
u32 gTalkSelects_93_1_1_1[2] = {
#include "lobby/00817A00.inc"
};
u32 gTalkSelects_93_1_1_3[2] = {
#include "lobby/00817A08.inc"
};
u32 gTalkSelects_93_1_3_1[2] = {
#include "lobby/00817A10.inc"
};
u32 gTalkSelects_93_1_3_3[2] = {
#include "lobby/00817A18.inc"
};
u32 gTalkSelects_93_1_5_1[2] = {
#include "lobby/00817A20.inc"
};
u32 gTalkSelects_93_1_5_3[2] = {
#include "lobby/00817A28.inc"
};
u32 gTalkSelects_93_1_5[4] = {
#include "lobby/00817A30.inc"
};
u32 gTalkSelects_93_1_3[4] = {
#include "lobby/00817A40.inc"
};
u32 gTalkSelects_93_1_1[4] = {
#include "lobby/00817A50.inc"
};
u32 gTalkSelects_93_1[6] = {
#include "lobby/00817A60.inc"
};
TalkSel gTalkSelects_93 = {0, (void *)gTalkSelects_93_1};
u32 D_00817A80[8] = {
#include "lobby/00817A80.inc"
};
TalkEvent gTalkEvents_94[2] = {
#include "lobby/00817AA0.inc"
};
u32 gTalkSelects_94_1_1_1[2] = {
#include "lobby/00817AA8.inc"
};
u32 gTalkSelects_94_1_1_3[2] = {
#include "lobby/00817AB0.inc"
};
u32 gTalkSelects_94_1_3_1[2] = {
#include "lobby/00817AB8.inc"
};
u32 gTalkSelects_94_1_3_3[2] = {
#include "lobby/00817AC0.inc"
};
u32 gTalkSelects_94_1_5_1[2] = {
#include "lobby/00817AC8.inc"
};
u32 gTalkSelects_94_1_5_3[4] = {
#include "lobby/00817AD0.inc"
};
u32 gTalkSelects_94_1_5[4] = {
#include "lobby/00817AE0.inc"
};
u32 gTalkSelects_94_1_3[4] = {
#include "lobby/00817AF0.inc"
};
u32 gTalkSelects_94_1_1[4] = {
#include "lobby/00817B00.inc"
};
u32 gTalkSelects_94_1[6] = {
#include "lobby/00817B10.inc"
};
TalkSel gTalkSelects_94 = {0, (void *)gTalkSelects_94_1};
u32 D_00817B30[8] = {
#include "lobby/00817B30.inc"
};
TalkEvent gTalkEvents_95[2] = {
#include "lobby/00817B50.inc"
};
u32 gTalkSelects_95_1_1_1[2] = {
#include "lobby/00817B58.inc"
};
u32 gTalkSelects_95_1_1_3[2] = {
#include "lobby/00817B60.inc"
};
u32 gTalkSelects_95_1_3_1[2] = {
#include "lobby/00817B68.inc"
};
u32 gTalkSelects_95_1_3_3[2] = {
#include "lobby/00817B70.inc"
};
u32 gTalkSelects_95_1_5_1[2] = {
#include "lobby/00817B78.inc"
};
u32 gTalkSelects_95_1_5_3[4] = {
#include "lobby/00817B80.inc"
};
u32 gTalkSelects_95_1_5[4] = {
#include "lobby/00817B90.inc"
};
u32 gTalkSelects_95_1_3[4] = {
#include "lobby/00817BA0.inc"
};
u32 gTalkSelects_95_1_1[4] = {
#include "lobby/00817BB0.inc"
};
u32 gTalkSelects_95_1[6] = {
#include "lobby/00817BC0.inc"
};
TalkSel gTalkSelects_95 = {0, (void *)gTalkSelects_95_1};
u32 D_00817BE0[30] = {
#include "lobby/00817BE0.inc"
};
TalkEvent gTalkEvents_96[2] = {
#include "lobby/00817C58.inc"
};
u32 gTalkSelects_96_1_1_1[2] = {
#include "lobby/00817C60.inc"
};
u32 gTalkSelects_96_1_3_1[2] = {
#include "lobby/00817C68.inc"
};
u32 gTalkSelects_96_1_5_1[2] = {
#include "lobby/00817C70.inc"
};
u32 gTalkSelects_96_1_5[2] = {
    0, (u32)gTalkSelects_96_1_5_1,
};
u32 gTalkSelects_96_1_3[2] = {
    0, (u32)gTalkSelects_96_1_3_1,
};
u32 gTalkSelects_96_1_1[2] = {
    0, (u32)gTalkSelects_96_1_1_1,
};
u32 gTalkSelects_96_1[6] = {
#include "lobby/00817C90.inc"
};
TalkSel gTalkSelects_96 = {0, (void *)gTalkSelects_96_1};
u32 D_00817CB0[16] = {
#include "lobby/00817CB0.inc"
};
TalkEvent gTalkEvents_97[2] = {
#include "lobby/00817CF0.inc"
};
u32 gTalkSelects_97_1_1_1[2] = {
#include "lobby/00817CF8.inc"
};
u32 gTalkSelects_97_1_3_1[2] = {
#include "lobby/00817D00.inc"
};
u32 gTalkSelects_97_1_5_1[2] = {
#include "lobby/00817D08.inc"
};
u32 gTalkSelects_97_1_5[2] = {
    0, (u32)gTalkSelects_97_1_5_1,
};
u32 gTalkSelects_97_1_3[2] = {
    0, (u32)gTalkSelects_97_1_3_1,
};
u32 gTalkSelects_97_1_1[4] = {
    0, (u32)gTalkSelects_97_1_1_1, 0, 0,
};
u32 gTalkSelects_97_1[6] = {
#include "lobby/00817D30.inc"
};
TalkSel gTalkSelects_97 = {0, (void *)gTalkSelects_97_1};
u32 D_00817D50[16] = {
#include "lobby/00817D50.inc"
};
TalkEvent gTalkEvents_98[2] = {
#include "lobby/00817D90.inc"
};
u32 gTalkSelects_98_1_1_1[2] = {
#include "lobby/00817D98.inc"
};
u32 gTalkSelects_98_1_1_3[2] = {
#include "lobby/00817DA0.inc"
};
u32 gTalkSelects_98_1_3_1[2] = {
#include "lobby/00817DA8.inc"
};
u32 gTalkSelects_98_1_3_3[2] = {
#include "lobby/00817DB0.inc"
};
u32 gTalkSelects_98_1_5_1[2] = {
#include "lobby/00817DB8.inc"
};
u32 gTalkSelects_98_1_5_3[4] = {
#include "lobby/00817DC0.inc"
};
u32 gTalkSelects_98_1_5[4] = {
#include "lobby/00817DD0.inc"
};
u32 gTalkSelects_98_1_3[4] = {
#include "lobby/00817DE0.inc"
};
u32 gTalkSelects_98_1_1[4] = {
#include "lobby/00817DF0.inc"
};
u32 gTalkSelects_98_1[6] = {
#include "lobby/00817E00.inc"
};
TalkSel gTalkSelects_98 = {0, (void *)gTalkSelects_98_1};
u32 D_00817E20[28] = {
#include "lobby/00817E20.inc"
};
TalkEvent gTalkEvents_99[2] = {
#include "lobby/00817E90.inc"
};
u32 gTalkSelects_99_0_1_1[2] = {
#include "lobby/00817E98.inc"
};
u32 gTalkSelects_99_0_1[2] = {
    0, (u32)gTalkSelects_99_0_1_1,
};
u32 gTalkSelects_99_0[2] = {
    0, (u32)gTalkSelects_99_0_1,
};
TalkSel gTalkSelects_99[2] = {
    {0, (void *)gTalkSelects_99_0},
    {0, NULL},
};
u32 D_00817EC0[12] = {
#include "lobby/00817EC0.inc"
};
TalkEvent gTalkEvents_100[2] = {
#include "lobby/00817EF0.inc"
};
u32 gTalkSelects_100_1_1_1[2] = {
#include "lobby/00817EF8.inc"
};
u32 gTalkSelects_100_1_1_3[4] = {
#include "lobby/00817F00.inc"
};
u32 gTalkSelects_100_1_1[4] = {
#include "lobby/00817F10.inc"
};
u32 gTalkSelects_100_1[2] = {
    0, (u32)gTalkSelects_100_1_1,
};
TalkSel gTalkSelects_100 = {0, (void *)gTalkSelects_100_1};
u32 D_00817F30[12] = {
#include "lobby/00817F30.inc"
};
TalkEvent gTalkEvents_101[2] = {
#include "lobby/00817F60.inc"
};
u32 gTalkSelects_101_1_1_1[2] = {
#include "lobby/00817F68.inc"
};
u32 gTalkSelects_101_1_1_3[2] = {
#include "lobby/00817F70.inc"
};
u32 gTalkSelects_101_1_3_1[2] = {
#include "lobby/00817F78.inc"
};
u32 gTalkSelects_101_1_3_3[2] = {
#include "lobby/00817F80.inc"
};
u32 gTalkSelects_101_1_5_1[2] = {
#include "lobby/00817F88.inc"
};
u32 gTalkSelects_101_1_5_3[4] = {
#include "lobby/00817F90.inc"
};
u32 gTalkSelects_101_1_5[4] = {
#include "lobby/00817FA0.inc"
};
u32 gTalkSelects_101_1_3[4] = {
#include "lobby/00817FB0.inc"
};
u32 gTalkSelects_101_1_1[4] = {
#include "lobby/00817FC0.inc"
};
u32 gTalkSelects_101_1[6] = {
#include "lobby/00817FD0.inc"
};
TalkSel gTalkSelects_101 = {0, (void *)gTalkSelects_101_1};
u32 D_00817FF0[40] = {
#include "lobby/00817FF0.inc"
};
TalkEvent gTalkEvents_102[2] = {
#include "lobby/00818090.inc"
};
u32 gTalkSelects_102_1_1_1[2] = {
#include "lobby/00818098.inc"
};
u32 gTalkSelects_102_1_1_3[2] = {
#include "lobby/008180A0.inc"
};
u32 gTalkSelects_102_1_3_1[2] = {
#include "lobby/008180A8.inc"
};
u32 gTalkSelects_102_1_3_3[2] = {
#include "lobby/008180B0.inc"
};
u32 gTalkSelects_102_1_5_1[2] = {
#include "lobby/008180B8.inc"
};
u32 gTalkSelects_102_1_5_3[4] = {
#include "lobby/008180C0.inc"
};
u32 gTalkSelects_102_1_5[4] = {
#include "lobby/008180D0.inc"
};
u32 gTalkSelects_102_1_3[4] = {
#include "lobby/008180E0.inc"
};
u32 gTalkSelects_102_1_1[4] = {
#include "lobby/008180F0.inc"
};
u32 gTalkSelects_102_1[6] = {
#include "lobby/00818100.inc"
};
TalkSel gTalkSelects_102 = {0, (void *)gTalkSelects_102_1};
u32 D_00818120[42] = {
#include "lobby/00818120.inc"
};
TalkEvent gTalkEvents_103[2] = {
#include "lobby/008181C8.inc"
};
u32 gTalkSelects_103_0_1_1[2] = {
#include "lobby/008181D0.inc"
};
u32 gTalkSelects_103_0_1_3[2] = {
#include "lobby/008181D8.inc"
};
u32 gTalkSelects_103_0_3_1[2] = {
#include "lobby/008181E0.inc"
};
u32 gTalkSelects_103_0_3_3[2] = {
#include "lobby/008181E8.inc"
};
u32 gTalkSelects_103_0_5_1[2] = {
    (u32)((u8 *)gHeapBlocks + 0x6E6E), 0,
};
u32 gTalkSelects_103_0_5_3[2] = {
#include "lobby/008181F8.inc"
};
u32 gTalkSelects_103_1_1_1[2] = {
#include "lobby/00818200.inc"
};
u32 gTalkSelects_103_1_1_3[2] = {
#include "lobby/00818208.inc"
};
u32 gTalkSelects_103_2_1_1[2] = {
#include "lobby/00818210.inc"
};
u32 gTalkSelects_103_2_1_3[2] = {
#include "lobby/00818218.inc"
};
u32 gTalkSelects_103_2_1[4] = {
#include "lobby/00818220.inc"
};
u32 gTalkSelects_103_2[4] = {
    0, (u32)gTalkSelects_103_2_1, 0, 0,
};
u32 gTalkSelects_103_1_1[4] = {
#include "lobby/00818240.inc"
};
u32 gTalkSelects_103_1[4] = {
    0, (u32)gTalkSelects_103_1_1, 0, 0,
};
u32 gTalkSelects_103_0_5[4] = {
#include "lobby/00818260.inc"
};
u32 gTalkSelects_103_0_3[4] = {
#include "lobby/00818270.inc"
};
u32 gTalkSelects_103_0_1[4] = {
#include "lobby/00818280.inc"
};
u32 gTalkSelects_103_0[8] = {
#include "lobby/00818290.inc"
};
TalkSel gTalkSelects_103[4] = {
#include "lobby/008182B0.inc"
};
u32 D_008182D0[62] = {
#include "lobby/008182D0.inc"
};
TalkEvent gTalkEvents_104[2] = {
#include "lobby/008183C8.inc"
};
u32 gTalkSelects_104_1_1_1[2] = {
#include "lobby/008183D0.inc"
};
u32 gTalkSelects_104_1_1_3[2] = {
#include "lobby/008183D8.inc"
};
u32 gTalkSelects_104_1_3_1[2] = {
#include "lobby/008183E0.inc"
};
u32 gTalkSelects_104_1_3_3[2] = {
#include "lobby/008183E8.inc"
};
u32 gTalkSelects_104_1_5_1[2] = {
#include "lobby/008183F0.inc"
};
u32 gTalkSelects_104_1_5_3[2] = {
#include "lobby/008183F8.inc"
};
u32 gTalkSelects_104_1_5[4] = {
#include "lobby/00818400.inc"
};
u32 gTalkSelects_104_1_3[4] = {
#include "lobby/00818410.inc"
};
u32 gTalkSelects_104_1_1[4] = {
#include "lobby/00818420.inc"
};
u32 gTalkSelects_104_1[6] = {
#include "lobby/00818430.inc"
};
TalkSel gTalkSelects_104 = {0, (void *)gTalkSelects_104_1};
u32 D_00818450[8] = {
#include "lobby/00818450.inc"
};
TalkEvent gTalkEvents_105[8] = {
#include "lobby/00818470.inc"
};
u32 gTalkSelects_105_0_1_1[2] = {
#include "lobby/00818490.inc"
};
u32 gTalkSelects_105_0_1_3[2] = {
#include "lobby/00818498.inc"
};
u32 gTalkSelects_105_0_3_1[2] = {
#include "lobby/008184A0.inc"
};
u32 gTalkSelects_105_0_3_3[2] = {
#include "lobby/008184A8.inc"
};
u32 gTalkSelects_105_0_5_1[2] = {
#include "lobby/008184B0.inc"
};
u32 gTalkSelects_105_0_5_3[2] = {
#include "lobby/008184B8.inc"
};
u32 gTalkSelects_105_1_1_1[2] = {
#include "lobby/008184C0.inc"
};
u32 gTalkSelects_105_1_1_3[2] = {
#include "lobby/008184C8.inc"
};
u32 gTalkSelects_105_2_1_1[2] = {
#include "lobby/008184D0.inc"
};
u32 gTalkSelects_105_3_1_1[2] = {
#include "lobby/008184D8.inc"
};
u32 gTalkSelects_105_4_1_1[2] = {
#include "lobby/008184E0.inc"
};
u32 gTalkSelects_105_4_1_3[2] = {
#include "lobby/008184E8.inc"
};
u32 gTalkSelects_105_4_1[4] = {
#include "lobby/008184F0.inc"
};
u32 gTalkSelects_105_4[2] = {
    0, (u32)gTalkSelects_105_4_1,
};
u32 gTalkSelects_105_3_1[2] = {
    0, (u32)gTalkSelects_105_3_1_1,
};
u32 gTalkSelects_105_3[2] = {
    0, (u32)gTalkSelects_105_3_1,
};
u32 gTalkSelects_105_2_1[2] = {
    0, (u32)gTalkSelects_105_2_1_1,
};
u32 gTalkSelects_105_2[4] = {
    0, (u32)gTalkSelects_105_2_1, 0, 0,
};
u32 gTalkSelects_105_1_1[4] = {
#include "lobby/00818530.inc"
};
u32 gTalkSelects_105_1[4] = {
    0, (u32)gTalkSelects_105_1_1, 0, 0,
};
u32 gTalkSelects_105_0_5[4] = {
#include "lobby/00818550.inc"
};
u32 gTalkSelects_105_0_3[4] = {
#include "lobby/00818560.inc"
};
u32 gTalkSelects_105_0_1[4] = {
#include "lobby/00818570.inc"
};
u32 gTalkSelects_105_0[8] = {
#include "lobby/00818580.inc"
};
TalkSel gTalkSelects_105[6] = {
#include "lobby/008185A0.inc"
};
u32 D_008185D0[104] = {
#include "lobby/008185D0.inc"
};
TalkGift gTalkGifts_105[4] = {
#include "lobby/00818770.inc"
};
TalkEvent gTalkEvents_106[2] = {
#include "lobby/00818790.inc"
};
u32 gTalkSelects_106_1_1_1[2] = {
#include "lobby/00818798.inc"
};
u32 gTalkSelects_106_1_3_1[2] = {
#include "lobby/008187A0.inc"
};
u32 gTalkSelects_106_1_5_1[2] = {
#include "lobby/008187A8.inc"
};
u32 gTalkSelects_106_1_5[2] = {
    0, (u32)gTalkSelects_106_1_5_1,
};
u32 gTalkSelects_106_1_3[2] = {
    0, (u32)gTalkSelects_106_1_3_1,
};
u32 gTalkSelects_106_1_1[4] = {
    0, (u32)gTalkSelects_106_1_1_1, 0, 0,
};
u32 gTalkSelects_106_1[6] = {
#include "lobby/008187D0.inc"
};
TalkSel gTalkSelects_106 = {0, (void *)gTalkSelects_106_1};
u32 D_008187F0[6] = {
#include "lobby/008187F0.inc"
};
TalkEvent gTalkEvents_107[3] = {
#include "lobby/00818808.inc"
};
u32 gTalkSelects_107_1_1_1[2] = {
#include "lobby/00818818.inc"
};
u32 gTalkSelects_107_1_1_3[2] = {
#include "lobby/00818820.inc"
};
u32 gTalkSelects_107_1_3_1[2] = {
#include "lobby/00818828.inc"
};
u32 gTalkSelects_107_1_3_3[2] = {
#include "lobby/00818830.inc"
};
u32 gTalkSelects_107_1_5_1[2] = {
#include "lobby/00818838.inc"
};
u32 gTalkSelects_107_1_5_3[4] = {
#include "lobby/00818840.inc"
};
u32 gTalkSelects_107_1_5[4] = {
#include "lobby/00818850.inc"
};
u32 gTalkSelects_107_1_3[4] = {
#include "lobby/00818860.inc"
};
u32 gTalkSelects_107_1_1[4] = {
#include "lobby/00818870.inc"
};
u32 gTalkSelects_107_1[6] = {
#include "lobby/00818880.inc"
};
TalkSel gTalkSelects_107 = {0, (void *)gTalkSelects_107_1};
u32 D_008188A0[44] = {
#include "lobby/008188A0.inc"
};
TalkEvent gTalkEvents_108[2] = {
#include "lobby/00818950.inc"
};
u32 gTalkSelects_108_1_1_1[2] = {
#include "lobby/00818958.inc"
};
u32 gTalkSelects_108_1_1_3[4] = {
#include "lobby/00818960.inc"
};
u32 gTalkSelects_108_1_1[4] = {
#include "lobby/00818970.inc"
};
u32 gTalkSelects_108_1[2] = {
    0, (u32)gTalkSelects_108_1_1,
};
TalkSel gTalkSelects_108 = {0, (void *)gTalkSelects_108_1};
u32 D_00818990[20] = {
#include "lobby/00818990.inc"
};
TalkEvent gTalkEvents_109[2] = {
#include "lobby/008189E0.inc"
};
u32 gTalkSelects_109_1_1[2] = {
    0, (u32)D_008BFA80,
};
u32 gTalkSelects_109_1[2] = {
    0, (u32)gTalkSelects_109_1_1,
};
TalkSel gTalkSelects_109 = {0, (void *)gTalkSelects_109_1};
u32 D_00818A00[2] = {
#include "lobby/00818A00.inc"
};
TalkEvent gTalkEvents_110[2] = {
#include "lobby/00818A08.inc"
};
u32 gTalkSelects_110_1_1_1[2] = {
#include "lobby/00818A10.inc"
};
u32 gTalkSelects_110_1_3_1[2] = {
#include "lobby/00818A18.inc"
};
u32 gTalkSelects_110_1_5_1[2] = {
#include "lobby/00818A20.inc"
};
u32 gTalkSelects_110_1_5[2] = {
    0, (u32)gTalkSelects_110_1_5_1,
};
u32 gTalkSelects_110_1_3[2] = {
    0, (u32)gTalkSelects_110_1_3_1,
};
u32 gTalkSelects_110_1_1[2] = {
    0, (u32)gTalkSelects_110_1_1_1,
};
u32 gTalkSelects_110_1[6] = {
#include "lobby/00818A40.inc"
};
TalkSel gTalkSelects_110 = {0, (void *)gTalkSelects_110_1};
u32 D_00818A60[16] = {
#include "lobby/00818A60.inc"
};
TalkEvent gTalkEvents_111[2] = {
#include "lobby/00818AA0.inc"
};
u32 gTalkSelects_111_0_1_1[2] = {
#include "lobby/00818AA8.inc"
};
u32 gTalkSelects_111_0_1[2] = {
    0, (u32)gTalkSelects_111_0_1_1,
};
u32 gTalkSelects_111_0[2] = {
    0, (u32)gTalkSelects_111_0_1,
};
TalkSel gTalkSelects_111[2] = {
    {0, (void *)gTalkSelects_111_0},
    {0, NULL},
};
u32 D_00818AD0[8] = {
#include "lobby/00818AD0.inc"
};
TalkEvent gTalkEvents_112[2] = {
#include "lobby/00818AF0.inc"
};
u32 gTalkSelects_112_0_1_1[2] = {
#include "lobby/00818AF8.inc"
};
u32 gTalkSelects_112_0_1[2] = {
    0, (u32)gTalkSelects_112_0_1_1,
};
u32 gTalkSelects_112_0[2] = {
    0, (u32)gTalkSelects_112_0_1,
};
TalkSel gTalkSelects_112[2] = {
    {0, (void *)gTalkSelects_112_0},
    {0, NULL},
};
u32 D_00818B20[8] = {
#include "lobby/00818B20.inc"
};
TalkEvent gTalkEvents_113[4] = {
#include "lobby/00818B40.inc"
};
u32 gTalkSelects_113_1_1_1[2] = {
#include "lobby/00818B50.inc"
};
u32 gTalkSelects_113_1_1_3[2] = {
#include "lobby/00818B58.inc"
};
u32 gTalkSelects_113_1_1[4] = {
#include "lobby/00818B60.inc"
};
u32 gTalkSelects_113_1[2] = {
    0, (u32)gTalkSelects_113_1_1,
};
TalkSel gTalkSelects_113 = {0, (void *)gTalkSelects_113_1};
u32 D_00818B80[52] = {
#include "lobby/00818B80.inc"
};
TalkGift gTalkGifts_113[3] = {
#include "lobby/00818C50.inc"
};
TalkEvent gTalkEvents_114[2] = {
#include "lobby/00818C68.inc"
};
u32 gTalkSelects_114_1_1[2] = {
    0, (u32)D_008BFAA8,
};
u32 gTalkSelects_114_1[2] = {
    0, (u32)gTalkSelects_114_1_1,
};
TalkSel gTalkSelects_114 = {0, (void *)gTalkSelects_114_1};
u32 D_00818C88[2] = {
#include "lobby/00818C88.inc"
};
TalkSel *gTalkSelects[116] = {
    NULL, gTalkSelects_1, gTalkSelects_2, gTalkSelects_3,
    gTalkSelects_4, gTalkSelects_5, gTalkSelects_6, gTalkSelects_7,
    gTalkSelects_8, gTalkSelects_9, &gTalkSelects_10, &gTalkSelects_11,
    gTalkSelects_12, &gTalkSelects_13, gTalkSelects_14, &gTalkSelects_15,
    gTalkSelects_16, NULL, gTalkSelects_18, gTalkSelects_19,
    NULL, &gTalkSelects_21, &gTalkSelects_22, &gTalkSelects_23,
    gTalkSelects_24, gTalkSelects_25, gTalkSelects_26, &gTalkSelects_27,
    gTalkSelects_28, &gTalkSelects_29, gTalkSelects_30, gTalkSelects_31,
    gTalkSelects_32, gTalkSelects_33, &gTalkSelects_34, gTalkSelects_35,
    gTalkSelects_36, gTalkSelects_37, gTalkSelects_38, gTalkSelects_39,
    gTalkSelects_40, gTalkSelects_41, gTalkSelects_42, gTalkSelects_43,
    gTalkSelects_44, gTalkSelects_45, gTalkSelects_46, gTalkSelects_47,
    gTalkSelects_48, gTalkSelects_49, gTalkSelects_50, gTalkSelects_51,
    gTalkSelects_52, &gTalkSelects_53, &gTalkSelects_54, gTalkSelects_55,
    gTalkSelects_56, gTalkSelects_57, gTalkSelects_58, gTalkSelects_59,
    gTalkSelects_60, gTalkSelects_61, gTalkSelects_62, &gTalkSelects_63,
    gTalkSelects_64, &gTalkSelects_65, &gTalkSelects_66, &gTalkSelects_67,
    &gTalkSelects_68, gTalkSelects_69, &gTalkSelects_70, &gTalkSelects_71,
    &gTalkSelects_72, gTalkSelects_73, gTalkSelects_74, gTalkSelects_75,
    gTalkSelects_76, gTalkSelects_77, gTalkSelects_78, gTalkSelects_79,
    gTalkSelects_80, gTalkSelects_81, gTalkSelects_82, gTalkSelects_83,
    gTalkSelects_84, gTalkSelects_85, gTalkSelects_86, gTalkSelects_87,
    gTalkSelects_88, &gTalkSelects_89, &gTalkSelects_90, &gTalkSelects_91,
    &gTalkSelects_92, &gTalkSelects_93, &gTalkSelects_94, &gTalkSelects_95,
    &gTalkSelects_96, &gTalkSelects_97, &gTalkSelects_98, gTalkSelects_99,
    &gTalkSelects_100, &gTalkSelects_101, &gTalkSelects_102, gTalkSelects_103,
    &gTalkSelects_104, gTalkSelects_105, &gTalkSelects_106, &gTalkSelects_107,
    &gTalkSelects_108, &gTalkSelects_109, &gTalkSelects_110, gTalkSelects_111,
    gTalkSelects_112, &gTalkSelects_113, &gTalkSelects_114, NULL,
};
TalkLine *gTalkLines[116] = {
    NULL, (void *)D_0080E420, (void *)D_0080F220, (void *)D_0080F740,
    (void *)D_0080FD50, (void *)D_00810530, (void *)D_00810BC0, (void *)D_00810FF0,
    (void *)D_00811530, (void *)D_008119F0, (void *)D_00811D10, (void *)D_00811E30,
    (void *)D_00811F40, (void *)D_00812060, (void *)D_00812450, (void *)D_00812710,
    (void *)D_00812A00, NULL, (void *)D_00812E70, (void *)D_00812FC0,
    NULL, (void *)D_00813120, (void *)D_008132B0, (void *)D_008133C0,
    (void *)D_008136C0, (void *)D_00813A90, (void *)D_00813D20, (void *)D_00813F30,
    (void *)D_00814070, (void *)D_00814348, (void *)D_008143F0, (void *)D_00814530,
    (void *)D_00814610, (void *)D_008146E0, (void *)D_00814748, (void *)D_008147C0,
    (void *)D_00814890, (void *)D_00814950, (void *)D_00814A10, (void *)D_00814AE0,
    (void *)D_00814BC0, (void *)D_00814C80, (void *)D_00814DC0, (void *)D_00814ED0,
    (void *)D_00814F90, (void *)D_008151B0, (void *)D_00815300, (void *)D_008153D0,
    (void *)D_008155B0, (void *)D_00815760, (void *)D_00815910, (void *)D_00815A30,
    (void *)D_00815A90, (void *)D_00815AD0, (void *)D_00815B10, (void *)D_00815C10,
    (void *)D_00815DB0, (void *)D_00815FB0, (void *)D_008160A0, (void *)D_008161D0,
    (void *)D_00816370, (void *)D_008164D0, (void *)D_008165F0, (void *)D_00816680,
    (void *)D_00816780, (void *)D_008169B0, (void *)D_00816AA0, (void *)D_00816AE8,
    (void *)D_00816B10, (void *)D_00816B40, (void *)D_00816B90, (void *)D_00816BE0,
    (void *)D_00816C30, (void *)D_00816CC0, (void *)D_00816DB0, (void *)D_00816E70,
    (void *)D_00816F30, (void *)D_00816FF0, (void *)D_00817100, (void *)D_008172D0,
    (void *)D_008173B0, (void *)D_00817420, (void *)D_008174A0, (void *)D_00817520,
    (void *)D_008175A0, (void *)D_00817670, (void *)D_00817730, (void *)D_00817840,
    (void *)D_00817940, (void *)D_00817978, (void *)D_008179A0, (void *)D_008179C8,
    (void *)D_008179F0, (void *)D_00817A80, (void *)D_00817B30, (void *)D_00817BE0,
    (void *)D_00817CB0, (void *)D_00817D50, (void *)D_00817E20, (void *)D_00817EC0,
    (void *)D_00817F30, (void *)D_00817FF0, (void *)D_00818120, (void *)D_008182D0,
    (void *)D_00818450, (void *)D_008185D0, (void *)D_008187F0, (void *)D_008188A0,
    (void *)D_00818990, (void *)D_00818A00, (void *)D_00818A60, (void *)D_00818AD0,
    (void *)D_00818B20, (void *)D_00818B80, (void *)D_00818C88, NULL,
};
TalkEvent *gTalkEvents[116] = {
    NULL, gTalkEvents_1, gTalkEvents_2, gTalkEvents_3,
    gTalkEvents_4, gTalkEvents_5, gTalkEvents_6, gTalkEvents_7,
    gTalkEvents_8, gTalkEvents_9, gTalkEvents_10, gTalkEvents_11,
    gTalkEvents_12, gTalkEvents_13, gTalkEvents_14, gTalkEvents_15,
    gTalkEvents_16, NULL, gTalkEvents_18, gTalkEvents_19,
    NULL, gTalkEvents_21, gTalkEvents_22, gTalkEvents_23,
    gTalkEvents_24, gTalkEvents_25, gTalkEvents_26, gTalkEvents_27,
    gTalkEvents_28, gTalkEvents_29, gTalkEvents_30, gTalkEvents_31,
    gTalkEvents_32, gTalkEvents_33, gTalkEvents_34, gTalkEvents_35,
    gTalkEvents_36, gTalkEvents_37, gTalkEvents_38, gTalkEvents_39,
    gTalkEvents_40, gTalkEvents_41, gTalkEvents_42, gTalkEvents_43,
    gTalkEvents_44, gTalkEvents_45, gTalkEvents_46, gTalkEvents_47,
    gTalkEvents_48, gTalkEvents_49, gTalkEvents_50, gTalkEvents_51,
    gTalkEvents_52, gTalkEvents_53, gTalkEvents_54, gTalkEvents_55,
    gTalkEvents_56, gTalkEvents_57, gTalkEvents_58, gTalkEvents_59,
    gTalkEvents_60, gTalkEvents_61, gTalkEvents_62, gTalkEvents_63,
    gTalkEvents_64, gTalkEvents_65, gTalkEvents_66, gTalkEvents_67,
    gTalkEvents_68, gTalkEvents_69, gTalkEvents_70, gTalkEvents_71,
    gTalkEvents_72, gTalkEvents_73, gTalkEvents_74, gTalkEvents_75,
    gTalkEvents_76, gTalkEvents_77, gTalkEvents_78, gTalkEvents_79,
    gTalkEvents_80, gTalkEvents_81, gTalkEvents_82, gTalkEvents_83,
    gTalkEvents_84, gTalkEvents_85, gTalkEvents_86, gTalkEvents_87,
    gTalkEvents_88, gTalkEvents_89, gTalkEvents_90, gTalkEvents_91,
    gTalkEvents_92, gTalkEvents_93, gTalkEvents_94, gTalkEvents_95,
    gTalkEvents_96, gTalkEvents_97, gTalkEvents_98, gTalkEvents_99,
    gTalkEvents_100, gTalkEvents_101, gTalkEvents_102, gTalkEvents_103,
    gTalkEvents_104, gTalkEvents_105, gTalkEvents_106, gTalkEvents_107,
    gTalkEvents_108, gTalkEvents_109, gTalkEvents_110, gTalkEvents_111,
    gTalkEvents_112, gTalkEvents_113, gTalkEvents_114, NULL,
};
TalkGift *gTalkGifts[116] = {
    NULL, gTalkGifts_1, (TalkGift *)D_008BF740, gTalkGifts_3,
    gTalkGifts_4, gTalkGifts_5, (TalkGift *)D_008BF748, gTalkGifts_7,
    gTalkGifts_8, (TalkGift *)D_008BF750, gTalkGifts_10, gTalkGifts_11,
    gTalkGifts_12, gTalkGifts_13, gTalkGifts_14, (TalkGift *)D_008BF758,
    gTalkGifts_16, NULL, gTalkGifts_18, (TalkGift *)D_008BF760,
    NULL, (TalkGift *)D_008BF768, (TalkGift *)D_008BF770, (TalkGift *)D_008BF778,
    gTalkGifts_24, gTalkGifts_25, gTalkGifts_26, (TalkGift *)D_008BF780,
    (TalkGift *)D_008BF788, (TalkGift *)D_008BF798, (TalkGift *)D_008BF7A0, (TalkGift *)D_008BF7A8,
    (TalkGift *)D_008BF7B8, (TalkGift *)D_008BF7C0, (TalkGift *)D_008BF7D0, (TalkGift *)D_008BF7E0,
    (TalkGift *)D_008BF7F0, (TalkGift *)D_008BF7F8, (TalkGift *)D_008BF800, (TalkGift *)D_008BF810,
    (TalkGift *)D_008BF818, (TalkGift *)D_008BF820, (TalkGift *)D_008BF828, (TalkGift *)D_008BF838,
    gTalkGifts_44, gTalkGifts_45, gTalkGifts_46, (TalkGift *)D_008BF840,
    (TalkGift *)D_008BF848, (TalkGift *)D_008BF850, (TalkGift *)D_008BF860, (TalkGift *)D_008BF868,
    (TalkGift *)D_008BF870, (TalkGift *)D_008BF878, (TalkGift *)D_008BF880, gTalkGifts_55,
    (TalkGift *)D_008BF888, (TalkGift *)D_008BF890, (TalkGift *)D_008BF898, (TalkGift *)D_008BF8A0,
    (TalkGift *)D_008BF8A8, (TalkGift *)D_008BF8B8, (TalkGift *)D_008BF8C0, (TalkGift *)D_008BF8C8,
    gTalkGifts_64, gTalkGifts_65, (TalkGift *)D_008BF8D0, (TalkGift *)D_008BF8E0,
    (TalkGift *)D_008BF8F0, (TalkGift *)D_008BF900, (TalkGift *)D_008BF910, (TalkGift *)D_008BF920,
    (TalkGift *)D_008BF930, gTalkGifts_73, (TalkGift *)D_008BF938, (TalkGift *)D_008BF940,
    (TalkGift *)D_008BF948, gTalkGifts_77, gTalkGifts_78, (TalkGift *)D_008BF958,
    (TalkGift *)D_008BF960, (TalkGift *)D_008BF970, (TalkGift *)D_008BF980, (TalkGift *)D_008BF988,
    (TalkGift *)D_008BF990, (TalkGift *)D_008BF998, (TalkGift *)D_008BF9A0, (TalkGift *)D_008BF9B0,
    (TalkGift *)D_008BF9C0, (TalkGift *)D_008BF9D0, (TalkGift *)D_008BF9E0, (TalkGift *)D_008BF9F0,
    (TalkGift *)D_008BFA00, (TalkGift *)D_008BFA08, (TalkGift *)D_008BFA10, (TalkGift *)D_008BFA18,
    (TalkGift *)D_008BFA20, (TalkGift *)D_008BFA28, (TalkGift *)D_008BFA30, (TalkGift *)D_008BFA38,
    (TalkGift *)D_008BFA40, (TalkGift *)D_008BFA48, (TalkGift *)D_008BFA50, (TalkGift *)D_008BFA58,
    (TalkGift *)D_008BFA60, gTalkGifts_105, (TalkGift *)D_008BFA68, (TalkGift *)D_008BFA70,
    (TalkGift *)D_008BFA78, (TalkGift *)D_008BFA88, (TalkGift *)D_008BFA90, (TalkGift *)D_008BFA98,
    (TalkGift *)D_008BFAA0, gTalkGifts_113, (TalkGift *)D_008BFAB0, NULL,
};

extern s32 gTalkLineText;

/* The text of a talk line (Talk_StepData). */
char *Talk_LineText(u16 line)
{
    return (char *)Talk_StepData(line);
}


/* The kind of the current talk line (TALK_LINE_*). The callers pass the
 * line (u16), which it does not use: it reads gTalkLine. The pointer is
 * stepped to the line first (talks += gTalkLine), the order of the game's
 * address arithmetic. */
s32 Talk_LineKind(u16 unused)
{
    TalkLine *talks;

    talks = gTalkLines[gTalkScript];
    talks += gTalkLine;
    return talks->kind;
}

u16 LobbyPad_Get(s32);
s32 Notice_OpenTextN(char *text, s32 width, s32 mode, s32 sel, s32 arg4); /* unit's own declaration: declared here with an s32 frame count, as the callers here pass it; with the definition's u32, Talk_RunText and Talk_Step compile two instructions differently each */

/* One frame of the current talk line's text, with no input (the line's
 * own text, or Talk_StepData's; gTalkTextFrames counts its frames, the
 * line's kind and the yes/no cursor gTalkMode choose the window).
 * Returns MENU_RUNNING while the text is still being shown, MENU_DONE once
 * it is all shown. */
s32 Talk_RunText(void)
{
    s32 text;

    LobbyPad_Get(0);
    if (gTalkTextFrames >= 0) {
        gTalkTextFrames++;
    }
    text = gTalkLineText != 0 ? gTalkLineText : Talk_StepData((u16)gTalkLine);
    if (Notice_OpenTextN((char *)text, gTalkTextFrames, Talk_LineKind(gTalkLine), gTalkMode, gTalkNpcName) == 0) {
        return MENU_DONE;
    }
    return MENU_RUNNING;
}


/*
 * One frame of the talk with the buttons. While the line's text is still
 * being shown, circle or cross shows it all at once (gTalkTextFrames -1).
 * Once it is shown:
 * - a TALK_LINE_NEXT line goes on to the next line with circle or cross;
 * - a TALK_LINE_ASK line moves the yes/no cursor (gTalkMode) with left and
 *   right; circle takes the cursor's answer, and cross answers no (the
 *   first press only moves the cursor to no). The answer skips ahead by
 *   the line's yesSkip or noSkip; with no skip the talk goes on to the
 *   next line, or ends when endOnAnswer is 1;
 * - any other line ends the talk.
 * Returns MENU_RUNNING while the talk goes on, MENU_DONE at its end.
 */
s32 Talk_Step(s8 endOnAnswer)
{
    u16 pad = LobbyPad_Get(0);
    s32 text;
    s32 kind;
    TalkLine *talks;
    s32 skip;

    if (gTalkTextFrames >= 0) {
        gTalkTextFrames++;
    }
    text = gTalkLineText != 0 ? gTalkLineText : Talk_StepData((u16)gTalkLine);
    kind = Talk_LineKind(gTalkLine);
    talks = gTalkLines[gTalkScript];
    talks += gTalkLine;
    if (Notice_OpenTextN((char *)text, gTalkTextFrames, kind, gTalkMode, gTalkNpcName) == 0) {
        if (pad & PAD_CIRCLE) {
            if (kind == TALK_LINE_NEXT) {
                gTalkTextFrames = 0;
                gTalkMode = TALK_CHOICE_YES;
                gTalkLine++;
            } else if (kind == TALK_LINE_ASK) {
                if (gTalkMode != TALK_CHOICE_NO) {
                    LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
                    if ((skip = talks->yesSkip) != 0) {
                        gTalkTextFrames = 0;
                        gTalkMode = TALK_CHOICE_YES;
                        gTalkLine += skip;
                    } else if (endOnAnswer == 1) {
                        gTalkTextFrames = 0;
                        return MENU_DONE;
                    } else {
                        gTalkTextFrames = 0;
                        gTalkMode = TALK_CHOICE_YES;
                        gTalkLine++;
                    }
                } else {
                    LobbySe_Play(LOBBY_SE_CANCEL);
                    if ((skip = talks->noSkip) != 0) {
                        gTalkTextFrames = 0;
                        gTalkMode = TALK_CHOICE_YES;
                        gTalkLine += skip;
                    } else if (endOnAnswer == 1) {
                        gTalkTextFrames = 0;
                        return MENU_DONE;
                    } else {
                        gTalkTextFrames = 0;
                        gTalkMode = TALK_CHOICE_YES;
                        gTalkLine++;
                    }
                }
            } else {
                return MENU_DONE;
            }
        } else if (pad & PAD_CROSS) {
            if (kind == TALK_LINE_NEXT) {
                gTalkTextFrames = 0;
                gTalkMode = TALK_CHOICE_YES;
                gTalkLine++;
            } else if (kind == TALK_LINE_ASK) {
                LobbySe_Play(LOBBY_SE_CANCEL);
                if (gTalkMode != TALK_CHOICE_NO) {
                    gTalkMode = TALK_CHOICE_NO;
                } else if ((skip = talks->noSkip) != 0) {
                    gTalkTextFrames = 0;
                    gTalkMode = TALK_CHOICE_YES;
                    gTalkLine += skip;
                } else if (endOnAnswer == 1) {
                    gTalkTextFrames = 0;
                    return MENU_DONE;
                } else {
                    gTalkTextFrames = 0;
                    gTalkMode = TALK_CHOICE_YES;
                    gTalkLine++;
                }
            } else {
                gTalkTextFrames = 0;
                return MENU_DONE;
            }
        } else if (kind == TALK_LINE_ASK) {
            if (pad & PAD_LEFT) {
                if (gTalkMode != TALK_CHOICE_YES) {
                    LobbySe_Play(LOBBY_SE_SCROLL);
                    gTalkMode = TALK_CHOICE_YES;
                }
            } else if (pad & PAD_RIGHT) {
                if (gTalkMode != TALK_CHOICE_NO) {
                    LobbySe_Play(LOBBY_SE_SCROLL);
                    gTalkMode = TALK_CHOICE_NO;
                }
            }
        }
    } else if (pad & (PAD_CIRCLE | PAD_CROSS)) {
        gTalkTextFrames = -1;
    }
    return MENU_RUNNING;
}


/* Talk_Step, ending on an answer. At the talk's end: MENU_CANCEL when the
 * last answer was no; otherwise the last line's gift is handed over (money,
 * with its notice; an item into the pouch, or a notice that it is full; a
 * piece of equipment into the box, or a notice that it is full) and
 * MENU_DONE. MENU_RUNNING while the talk goes on. */
s32 Talk_Update(void)
{
    TalkGift *given;
    TalkLine *talks;
    u8 giftIndex;
    s16 count;
    char msg[0x20];
    char wide[0x20];

    if (Talk_Step(1) == MENU_DONE) {
        if (gTalkMode == TALK_CHOICE_NO) {
            return MENU_CANCEL;
        }
        talks = gTalkLines[gTalkScript];
        talks += gTalkLine;
        giftIndex = talks->gift;
        if (giftIndex != TALK_GIFT_NONE) {
            given = gTalkGifts[gTalkScript];
            given += giftIndex;
            switch (given->kind) {
            case TALK_GIFT_MONEY:
                Save_AddMoney(given->a.value);
                LobbySe_Play(LOBBY_SE_MONEY);
                sprintf(msg, gTownLabels[TOWN_LABEL_ZENNY_GOT_FMT], given->a.value);
                Sjis_Widen(msg, wide);
                Notice_PushText(wide);
                break;
            case TALK_GIFT_ITEM:
                count = given->b.count;
                if (SavePouch_Space(given->a.item) >= count) {
                    SavePouch_Add(given->a.item, count);
                    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, given->a.value);
                } else {
                    Menu_PushNotice(LOBBY_NOTICE_POUCH_FULL);
                }
                break;
            case TALK_GIFT_EQUIP:
                if ((u16)Box_FindFreeSlot(&gSave) != BOX_NONE) {
                    Box_AddItem(&gSave, given->a.type, given->b.id, 0);
                    Menu_PushNotice(LOBBY_NOTICE_EQUIP_GOT);
                } else {
                    Menu_PushNotice(LOBBY_NOTICE_BOX_FULL);
                }
                break;
            }
        }
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/* Starts the talk's line `line`, the yes/no cursor at `choice`
 * (TALK_CHOICE_*), with the line's own text. */
void Talk_Say(s32 line, s8 choice)
{
    gTalkLine = line;
    gTalkMode = choice;
    gTalkLineText = 0;
    gTalkTextFrames = 0;
}
