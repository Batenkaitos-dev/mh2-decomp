/*
 * The talk scripts' choices and the talk's end. A townsperson's talk plays
 * one of the lines of its script (the talk data, by script number: the
 * townsperson's kind, NpcWork.id, in gTalkScript). Talk_Start (the talk
 * system) chooses the line: first the script's events (gTalkEvents), each
 * an event step that Talk_Check tests here, through the script's own
 * Talk_CheckNN; then, when no event applies (TALK_EVENT_NONE), the
 * selection lists (gTalkSelects), by village condition (Cond_Check), by
 * season and by time of day.
 *
 * The event checks test the save's event flags (Save_TestEventFlag), the
 * village conditions, hunter rank and the like. An event that plays once
 * puts its event flag in gTalkEventFlag, and the talk's end sets it.
 *
 * The talk's end: Talk_Finish runs the talk (Talk_Step), gives the line's
 * gift (money, a pouch item, a piece of equipment), sets the event flag,
 * runs the event's action (Talk_StepAction) and then Talk_FollowUp, which
 * opens what the townsperson offers (a shop, a desk, a menu) or closes
 * the talk (Talk_Close).
 *
 * The numbers in the checks are mostly the scripts' own event flags and
 * event steps, which only the talk data explains: they stay numbers.
 */
#include "common.h"
#include "em.h"
#include "lobby_npc.h"
#include "player.h"
#include "save.h"
#include "quest.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "scene.h"
#include "chat_log.h"
#include "lobby_labels.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "pad.h"
#include "village_cond.h"
#include "area_draw.h"
#include "bgm.h"
#include "camera.h"
#include "combine.h"
#include "talk_scripts.h"
#include "arena_cup.h"
#include "combine_shop.h"
#include "dojo_menu.h"
#include "elder_forecast.h"
#include "equip_shop.h"
#include "font.h"
#include "hair_menu.h"
#include "ingredient_list.h"
#include "item_shop.h"
#include "lobby_util.h"
#include "market.h"
#include "menu_steps.h"
#include "my_house.h"
#include "offer_menu.h"
#include "official_desk.h"
#include "quest_desk.h"
#include "quest_offers.h"
#include "ship_mine_menus.h"
#include "song_menu.h"
#include "talk.h"
#include "town_guide.h"
#include "town_input.h"
#include "village_progress.h"
#include "workshop_menu.h"
#include "libc.h"

extern s32 gTalkEventFlag;
u8 Village_FacilityLevel(s32 k);
/* unit's own declaration: declared as these checks call it, with an s32 argument and result; with the definition's s16 ones the calls compile differently */
s32 Village_MilestoneState(s32);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;
extern u16 gTalkEventStep;

/* Ends a talk: the cancel sound, the menus' input locked for 20 frames,
 * the town back to TOWN_MODE_FREE and the camera's focus on the
 * townsperson ended. */
void Talk_Close(void)
{
    Player *pl;

    pl = &gPlayers[gScene.localPlayer];
    LobbySe_Play(LOBBY_SE_CANCEL);
    pl->drinkCount = 0;
    gMenuInputLock = 20;
    gTownMode = TOWN_MODE_FREE;
    gTownMenuOpen = 0;
    CamFocus_End();
}

extern u16 gTalkScript;
struct Em;

/* After a talk: opens what the townsperson of script gTalkScript offers
 * when its conditions hold (the quest desk, the dojo, a shop, the
 * workshop, the official desk, the arena cup ...). If the town is still
 * in TOWN_MODE_TALK (or mode 5, or free), the townsperson's offers
 * (OfferMenu) open if it has any, else the talk closes. */
void Talk_FollowUp(u16 script)
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
        if (!Save_TestEventFlag(311)) {
            Save_SetEventFlag(311);
        }
        break;
    case 108:
        QuestDesk_Open(npc);
        break;
    case 3:
        if (gTalkEventStep != 5 && Cond_Check(31)) {
            DojoMenu_Open((struct Em *)npc);
        }
        break;
    case NPC_CONTEST_HOST:
        if (ShipMenu_IsOffered() == 1) {
            ShipMenu_Open(npc);
        }
        break;
    case 7:
        if (gTalkEventStep != 1 && Cond_Check(1)) {
            gTownMode = TOWN_MODE_ITEM_SHOP;
            ItemShop_Open(npc, -1);
        }
        break;
    case 8:
        if (Cond_Check(5)) {
            CombShop_Open(npc);
        }
        break;
    case 44:
        CombShop_Open(npc);
        break;
    case NPC_DRINK_CONTEST_HOST:
        if (Cond_IsLatest(15) == 1 && !Save_TestEventFlag(77)) {
            Save_SetEventFlag(77);
            QuestList_Rebuild();
        }
        if (Cond_IsLatest(19) == 1 && !Save_TestEventFlag(81)) {
            Save_SetEventFlag(81);
            Npc_RefreshOffers();
        }
        break;
    case NPC_NAMING_TALKER:
        if (!Save_TestEventFlag(74)) {
            Save_SetEventFlag(74);
        }
        if (gTalkEventStep != 3) {
            ItemShop_Open(npc, -1);
        }
        break;
    case 66:
        if (Save_TestEventFlag(EVENT_FLAG_ARENA_CUP_OPEN) == 1) {
            ItemShop_Open(npc, -1);
        }
        break;
    case 10:
    case 11:
    case 45:
    case 46:
    case NPC_GALLERY_KEEPER:
    case 80:
    case 93:
    case 106:
    case 13:
    case 40:
        ItemShop_Open(npc, -1);
        break;
    case 12:
        if (*(f32 *)(npc + 172) > 10200.0f) {
            ItemShop_Open(npc, -1);
        } else {
            EquipShop_Open(npc);
        }
        break;
    case 19:
        if (*(u8 *)((u8 *)&gSave + 14323) != 1) {
            Mine_OpenMenu(npc);
        }
        break;
    case 28:
        TownGuide_Open();
        break;
    case 86:
    case 94:
        EquipShop_Open(npc);
        break;
    case NPC_WORKSHOP_SMITH:
        if (gTalkEventStep != 15) {
            if (pl->pos[1] < 100.0f && Cond_Check(47) == 1) {
                EquipShop_Open(npc);
            } else if (Cond_Check(47) == 1) {
                Workshop_Open((struct Em *)npc);
            }
        }
        break;
    case 30:
        ElderForecast_Open();
        break;
    case 55:
        if (Cond_Check(COND_HR_ANY)) {
            DojoMenu_Open((struct Em *)npc);
        }
        break;
    case 56:
        if (Cond_Check(COND_HR_ANY)) {
            QuestDesk_Open(npc);
        }
        break;
    case NPC_OFFICIAL_DESK_LOW_HR:
        if (gTalkEventStep == 3) {
            Menu_PushNoticeOnce(17);
        }
    case NPC_OFFICIAL_DESK_HIGH_HR:
        if (gTalkEventStep == TALK_EVENT_NONE || gLobby->elderRaidState != ELDER_RAID_OFF) {
            OfficialDesk_Open((struct Em *)npc);
        }
        break;
    case 65:
        ArenaCup_Open(npc);
        break;
    case 73:
        SongMenu_Open(npc);
        break;
    case NPC_QUEST_DESK_2:
        QuestDesk_Open(npc);
        break;
    case NPC_WORKSHOP_HELPER_ONLINE:
    case NPC_WORKSHOP_HELPER:
    case 104:
    case 85:
        Workshop_Open((struct Em *)npc);
        break;
    case NPC_HOUSE_COOK:
        if (gTalkEventStep == TALK_EVENT_NONE && MyHouse_IsOwner() == 1) {
            IngredientList_Open((struct Em *)npc);
        }
        break;
    case 111:
        HairMenu_Open((Em *)npc, 0);
    }
    switch (gTownMode) {
    case TOWN_MODE_TALK:
    case 5:
    case TOWN_MODE_FREE:
        if (offers == 0) {
            Talk_Close();
        } else {
            OfferMenu_Open((struct Em *) npc);
            LobbySe_Play(LOBBY_SE_MENU_OPEN);
        }
        break;
    }
}

/* NPC_NAMING_TALKER's event action: event step 3 gives a PSP gift
 * (PspGift_Pick, PspGift_Give). */
void Talk_GivePspGift(u16 step)
{
    switch (step) {
    case 3:
        PspGift_Give(PspGift_Pick());
        break;
    }
}

/* The action of townspeople 69-71's events: event step 99 plays jingle 307
 * in the morning. */
void Talk_PlayMorningJingle(u16 step)
{
    if ((gTalkEventStep == 99) && (LobbyTime_IsPm() == 0)) {
        Jingle_Play(&gPlayers[gScene.localPlayer], 307);
    }
}

/* The action an event of script `id` runs at the talk's end, for event
 * step `step` (Talk_Finish). */
void Talk_StepAction(u16 id, u16 step)
{
    switch (id) {
    case NPC_NAMING_TALKER:
        Talk_GivePspGift(step);
        break;
    case 69:
    case 70:
    case 71:
        Talk_PlayMorningJingle(step);
        break;
    }
}

u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
u8 SavePouch_Add(u16 item, s16 n);
extern s8 gTalkFinishStep;
extern s32 gTalkLine;

/* The end of a talk, run each frame: the talk starts (Talk_Start, the
 * camera's focus), then runs (Talk_Step); when it is over, the event's
 * action runs (Talk_StepAction), the line's gift (gTalkLines, gTalkGifts:
 * money, a pouch item, equipment) is given, gTalkEventFlag is set unless
 * it is -1, and Talk_FollowUp runs. A gift with no room in the pouch or
 * the box shows its notice and closes the talk without the flag. Returns
 * 2 while it runs, 0 when it is over, 3 when the player has no target. */
s32 Talk_Finish(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    TalkGift *giftRec;
    TalkLine *talks;
    u8 gift;
    char msg[32];
    char buf[32];

    if (((u8 *)pl->target) == NULL) {
        gTownMode = TOWN_MODE_FREE;
        return 3;
    }
    switch (gTalkFinishStep) {
    case 0:
        if (Npc_IsType77((Em *)((u8 *)pl->target)) == 0) {
            CamFocus_Start(pl->target);
        }
        Talk_Start(pl->target, 0);
        gTalkFinishStep++;
        break;
    case 1:
        switch (Talk_Step(0)) {
        case 0:
        case 3:
            if (gTalkEventStep != TALK_EVENT_NONE) {
                Talk_StepAction(gTalkScript, gTalkEventStep);
                Npc_ClearAllTalked();
            }
            talks = gTalkLines[gTalkScript];
            talks += gTalkLine;
            gift = talks->gift;
            if (gift != TALK_GIFT_NONE) {
                giftRec = gTalkGifts[gTalkScript];
                giftRec += gift;
                switch (giftRec->kind) {
                case TALK_GIFT_MONEY:
                    Save_AddMoney(giftRec->a.value);
                    LobbySe_Play(LOBBY_SE_MONEY);
                    sprintf(msg, gTownLabels[TOWN_LABEL_ZENNY_GOT_FMT], giftRec->a.value);
                    Sjis_Widen(msg, buf);
                    Notice_PushText(buf);
                    if (gTalkEventFlag != -1) {
                        Save_SetEventFlag(gTalkEventFlag);
                    }
                    break;
                case TALK_GIFT_ITEM:
                    if (SavePouch_Space(giftRec->a.item) >= giftRec->b.count || Item_IsRecordOnly(giftRec->a.item) == 1) {
                        SavePouch_Add(giftRec->a.item, giftRec->b.count);
                        Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_RECEIVED, giftRec->a.value);
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
                        Box_AddItem(&gSave, giftRec->a.type, giftRec->b.id, 0);
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

/* The events of townsperson 1's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check01(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(0) == 0) {
            gTalkEventFlag = 0;
            EventFlag_Set(30);
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(13) == 0 && Cond_Check(2) == 1) {
            gTalkEventFlag = 13;
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(14) == 0 && Cond_Check(3) == 1) {
            gTalkEventFlag = 14;
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(15) == 0 && Cond_Check(6) == 1) {
            gTalkEventFlag = 15;
            Save_SetEventFlag(14);
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
        if (Save_TestEventFlag(139) == 0 && Cond_Check(8) == 1) {
            gTalkEventFlag = 139;
            return 1;
        }
        break;
    case 7:
        if (Save_TestEventFlag(378) == 0 && Cond_Check(9) == 1) {
            gTalkEventFlag = 378;
            Save_SetEventFlag(139);
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(379) == 0 && Cond_Check(10) == 1) {
            gTalkEventFlag = 379;
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(380) == 0 && Cond_Check(12) == 1) {
            gTalkEventFlag = 380;
            Save_SetEventFlag(379);
            return 1;
        }
        break;
    case 10:
        if (Save_TestEventFlag(381) == 0 && Cond_Check(13) == 1) {
            gTalkEventFlag = 381;
            Save_SetEventFlag(380);
            Save_SetEventFlag(379);
            return 1;
        }
        break;
    case 11:
        if (Save_TestEventFlag(382) == 0 && Cond_Check(20) == 1) {
            gTalkEventFlag = 382;
            Save_SetEventFlag(381);
            Save_SetEventFlag(380);
            Save_SetEventFlag(379);
            return 1;
        }
        break;
    case 12:
        if (Save_TestEventFlag(383) == 0 && Cond_Check(21) == 1) {
            gTalkEventFlag = 383;
            Save_SetEventFlag(382);
            Save_SetEventFlag(381);
            Save_SetEventFlag(380);
            Save_SetEventFlag(379);
            return 1;
        }
        break;
    case 13:
        if (Save_TestEventFlag(384) == 0 && Cond_Check(23) == 1) {
            gTalkEventFlag = 384;
            Save_SetEventFlag(383);
            Save_SetEventFlag(382);
            Save_SetEventFlag(381);
            Save_SetEventFlag(380);
            Save_SetEventFlag(379);
            return 1;
        }
        break;
    case 14:
        if (Save_TestEventFlag(385) == 0 && Cond_Check(24) == 1) {
            gTalkEventFlag = 385;
            Save_SetEventFlag(384);
            Save_SetEventFlag(383);
            Save_SetEventFlag(382);
            Save_SetEventFlag(381);
            Save_SetEventFlag(380);
            Save_SetEventFlag(379);
            return 1;
        }
        break;
    case 15:
        if (Cond_Check(25) == 1 && Save_IsQuestCleared(5005) == 0 && Save_TestEventFlag(463) == 0) {
            gTalkEventFlag = 463;
            return 1;
        }
        break;
    case 16:
        if (Cond_Check(25) == 1 && Save_IsQuestCleared(5005) == 1 && Save_IsQuestCleared(5006) == 0 && Save_TestEventFlag(464) == 0) {
            gTalkEventFlag = 464;
            return 1;
        }
        break;
    case 17:
        if (Cond_Check(25) == 1 && Save_IsQuestCleared(5005) == 1 && Save_IsQuestCleared(5006) == 1 && Save_IsQuestCleared(5007) == 0 && Save_TestEventFlag(465) == 0) {
            gTalkEventFlag = 465;
            return 1;
        }
        break;
    case 18:
        if (Cond_Check(26) == 1 && Save_TestEventFlag(386) == 0) {
            gTalkEventFlag = 386;
            return 1;
        }
        break;
    case 19:
        if (Cond_Check(27) == 1 && Save_TestEventFlag(387) == 0) {
            gTalkEventFlag = 387;
            Save_SetEventFlag(386);
            return 1;
        }
        break;
    case 20:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    case 21:
        if (Village_Event186Pending() == 1 && Save_TestEventFlag(392) == 0) {
            gTalkEventFlag = 392;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 2's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check02(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(19) == 0 && Cond_Check(1) == 1) {
            gTalkEventFlag = 19;
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(20) == 0 && Cond_Check(7) == 1) {
            gTalkEventFlag = 20;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(10) == 1 && Save_TestEventFlag(393) == 0) {
            gTalkEventFlag = 393;
            Save_SetEventFlag(20);
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(16) == 1 && Save_TestEventFlag(394) == 0) {
            gTalkEventFlag = 394;
            Save_SetEventFlag(393);
            Save_SetEventFlag(20);
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(21) == 1 && Save_TestEventFlag(395) == 0) {
            gTalkEventFlag = 395;
            Save_SetEventFlag(394);
            Save_SetEventFlag(393);
            Save_SetEventFlag(20);
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(26) == 1 && Save_TestEventFlag(396) == 0) {
            gTalkEventFlag = 396;
            Save_SetEventFlag(395);
            Save_SetEventFlag(394);
            Save_SetEventFlag(393);
            Save_SetEventFlag(20);
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(27) == 1 && Save_TestEventFlag(397) == 0) {
            gTalkEventFlag = 397;
            Save_SetEventFlag(396);
            Save_SetEventFlag(395);
            Save_SetEventFlag(394);
            Save_SetEventFlag(393);
            Save_SetEventFlag(20);
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

/* The events of townsperson 3's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check03(u16 step)
{
    switch (step) {
    case 5:
        if (Save_TestEventFlag(21) == 0 && Cond_Check(1) == 1) {
            gTalkEventFlag = 21;
            return 1;
        }
        break;
    case 1:
        if (Save_TestEventFlag(22) == 0 && Cond_Check(3) == 1) {
            gTalkEventFlag = 22;
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(23) == 0 && Cond_Check(7) == 1) {
            gTalkEventFlag = 23;
            Save_SetEventFlag(22);
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(24) == 0 && Cond_Check(10) == 1) {
            gTalkEventFlag = 24;
            Save_SetEventFlag(23);
            Save_SetEventFlag(22);
            return 1;
        }
        break;
    case 4:
        if ((u8)Village_FacilityLevel(1) == 2 && Save_TestEventFlag(398) == 0) {
            gTalkEventFlag = 398;
            Save_SetEventFlag(24);
            Save_SetEventFlag(23);
            Save_SetEventFlag(22);
            return 1;
        }
        break;
    case 6:
        if (Dojo_OfflineDrillsCleared() == 1 && Save_TestEventFlag(399) == 0) {
            gTalkEventFlag = 399;
            Save_SetEventFlag(398);
            Save_SetEventFlag(24);
            Save_SetEventFlag(23);
            Save_SetEventFlag(22);
            return 1;
        }
        break;
    case 7:
        if (Dojo_OfflineChainCleared() == 1 && Save_TestEventFlag(400) == 0) {
            gTalkEventFlag = 400;
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

/* The events of townsperson 4's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check04(u16 step)
{
    switch (step) {
    case 4:
        if (Save_TestEventFlag(302) == 0) {
            gTalkEventFlag = 302;
            return 1;
        }
        break;
    case 1:
        if (Cond_Check(7) == 1 && Save_TestEventFlag(459) == 0) {
            gTalkEventFlag = 459;
            Save_SetEventFlag(302);
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(461) == 0 && Save_TestEventFlag(EVENT_FLAG_PSP_LINK_1) == 1 && Save_TestEventFlag(48) == 0) {
            gTalkEventFlag = 461;
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


/* The events of NPC_CONTEST_HOST's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_CheckContestHost(u16 step)
{
    switch (step) {
    case 13:
        if (Save_TestEventFlag(303) == 0) {
            gTalkEventFlag = 303;
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(67) == 1 && Save_TestEventFlag(402) == 0) {
            gTalkEventFlag = 402;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(68) == 1 && Save_TestEventFlag(403) == 0) {
            gTalkEventFlag = 403;
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(69) == 1 && Save_TestEventFlag(404) == 0) {
            gTalkEventFlag = 404;
            Save_SetEventFlag(403);
            return 1;
        }
        break;
    case 1:
        if ((u8)Village_FacilityLevel(3) > 0 && (u16)Record_GroupLargest(&gSave, 21, 0) >= 123 && Save_TestEventFlag(405) == 0) {
            gTalkEventFlag = 405;
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(71) == 1 && Save_TestEventFlag(406) == 0) {
            gTalkEventFlag = 406;
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(72) == 1 && Save_TestEventFlag(407) == 0) {
            gTalkEventFlag = 407;
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(73) == 1 && Save_TestEventFlag(408) == 0) {
            gTalkEventFlag = 408;
            return 1;
        }
        break;
    case 8:
        if (Cond_Check(74) == 1 && Save_TestEventFlag(409) == 0) {
            gTalkEventFlag = 409;
            return 1;
        }
        break;
    case 9:
        if (Cond_Check(75) == 1 && Save_TestEventFlag(410) == 0) {
            gTalkEventFlag = 410;
            return 1;
        }
        break;
    case 10:
        if (Cond_Check(76) == 1 && Save_TestEventFlag(411) == 0) {
            gTalkEventFlag = 411;
            Save_SetEventFlag(410);
            return 1;
        }
        break;
    case 12:
        if (ShipMenu_IsOffered() == 1 && Save_TestEventFlag(456) == 0) {
            gTalkEventFlag = 456;
            return 1;
        }
        break;
    case 11:
        if (Cond_Check(78) == 1 && Save_TestEventFlag(412) == 0) {
            gTalkEventFlag = 412;
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

/* The events of townsperson 6's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check06(u16 step)
{
    switch (step) {
    case 1:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 7's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check07(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(25) == 0 && Cond_Check(1) == 1) {
            gTalkEventFlag = 25;
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(26) == 0 && Cond_Check(7) == 1) {
            gTalkEventFlag = 26;
            Save_SetEventFlag(25);
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(304) == 0 && Cond_Check(10) == 1) {
            gTalkEventFlag = 304;
            Save_SetEventFlag(25);
            Save_SetEventFlag(26);
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(305) == 0 && Cond_Check(16) == 1) {
            gTalkEventFlag = 305;
            Save_SetEventFlag(25);
            Save_SetEventFlag(26);
            Save_SetEventFlag(304);
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(18) == 1 && Save_TestEventFlag(413) == 0) {
            gTalkEventFlag = 413;
            Save_SetEventFlag(25);
            Save_SetEventFlag(26);
            Save_SetEventFlag(304);
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(19) == 1 && Save_TestEventFlag(414) == 0) {
            gTalkEventFlag = 414;
            return 1;
        }
        break;
    case 7:
        if (Save_TestEventFlag(306) == 0 && Cond_Check(21) == 1) {
            gTalkEventFlag = 306;
            Save_SetEventFlag(414);
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(307) == 0 && Cond_Check(25) == 1) {
            gTalkEventFlag = 307;
            Save_SetEventFlag(414);
            Save_SetEventFlag(306);
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(308) == 0 && Cond_Check(26) == 1) {
            gTalkEventFlag = 308;
            Save_SetEventFlag(414);
            Save_SetEventFlag(306);
            Save_SetEventFlag(307);
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

/* The events of townsperson 8's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check08(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(27) == 0) {
            gTalkEventFlag = 27;
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(28) == 0 && Cond_Check(4) == 1) {
            gTalkEventFlag = 28;
            Save_SetEventFlag(27);
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(309) == 0 && Cond_Check(5) == 1) {
            gTalkEventFlag = 309;
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(29) == 0 && Cond_Check(7) == 1) {
            gTalkEventFlag = 29;
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(10) == 1 && Save_TestEventFlag(415) == 0) {
            gTalkEventFlag = 415;
            Save_SetEventFlag(29);
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(16) == 1 && Save_TestEventFlag(416) == 0) {
            gTalkEventFlag = 416;
            Save_SetEventFlag(415);
            Save_SetEventFlag(29);
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(21) == 1 && Save_TestEventFlag(417) == 0) {
            gTalkEventFlag = 417;
            Save_SetEventFlag(416);
            Save_SetEventFlag(415);
            Save_SetEventFlag(29);
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(26) == 1 && Save_TestEventFlag(418) == 0) {
            gTalkEventFlag = 418;
            Save_SetEventFlag(417);
            Save_SetEventFlag(416);
            Save_SetEventFlag(415);
            Save_SetEventFlag(29);
            return 1;
        }
        break;
    case 8:
        if (Combine_KnowsMostRecipes() == 1 && Save_TestEventFlag(184) == 0) {
            gTalkEventFlag = 184;
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

/* The events of townsperson 9's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check09(u16 step)
{
    switch (step) {
    case 1:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 10's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check0A(u16 step)
{
    switch (step) {
    case 2:
        if (Save_TestEventFlag(419) == 0) {
            gTalkEventFlag = 419;
            return 1;
        }
        break;
    case 1:
        if (Village_MilestoneState(0) != 0) {
            if (Save_TestEventFlag(419) == 0) {
                Save_SetEventFlag(419);
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

/* The events of townsperson 11's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check0B(u16 step)
{
    switch (step) {
    case 3:
        if (Save_TestEventFlag(420) == 0) {
            gTalkEventFlag = 420;
            return 1;
        }
        break;
    case 1:
        if ((u8)Village_FacilityLevel(3) == 2 && Save_TestEventFlag(421) == 0) {
            gTalkEventFlag = 421;
            Save_SetEventFlag(420);
            return 1;
        }
        break;
    case 2:
        if (Village_MilestoneState(0) != 0) {
            Save_SetEventFlag(421);
            Save_SetEventFlag(420);
            return 1;
        }
        break;
    case 4:
        if (Village_Event186Pending() == 1) {
            Save_SetEventFlag(421);
            Save_SetEventFlag(420);
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 12's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check0C(u16 step)
{
    switch (step) {
    case 2:
        if (Save_TestEventFlag(424) == 0) {
            gTalkEventFlag = 424;
            return 1;
        }
        break;
    case 1:
        if (Save_TestEventFlag(425) == 0 && (u8)Village_FacilityLevel(2) == 2) {
            gTalkEventFlag = 425;
            Save_SetEventFlag(424);
            return 1;
        }
        break;
    case 3:
        if (Village_Event186Pending() == 1) {
            Save_SetEventFlag(425);
            Save_SetEventFlag(424);
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 13's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check0D(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(426) == 0) {
            gTalkEventFlag = 426;
            return 1;
        }
        break;
    case 2:
        if (Village_Event186Pending() == 1) {
            Save_SetEventFlag(426);
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of NPC_WORKSHOP_SMITH's script: 1 when event `step` applies
 * now, else 0. The events that play once put their event flag in
 * gTalkEventFlag. */
s32 Talk_CheckWorkshopSmith(u16 step)
{
    switch (step) {
    case 15:
        if (Save_TestEventFlag(30) == 0 && Cond_Check(47) == 1) {
            gTalkEventFlag = 30;
            return 1;
        }
        break;
    case 14:
        if (Save_TestEventFlag(31) == 0 && Cond_Check(48) == 1) {
            gTalkEventFlag = 31;
            Save_SetEventFlag(30);
            return 1;
        }
        break;
    case 13:
        if (Save_TestEventFlag(0x20) == 0 && Cond_Check(49) == 1) {
            gTalkEventFlag = 0x20;
            Save_SetEventFlag(31);
            Save_SetEventFlag(30);
            return 1;
        }
        break;
    case 12:
        if (Save_TestEventFlag(33) == 0 && Cond_Check(50) == 1) {
            gTalkEventFlag = 33;
            Save_SetEventFlag(30);
            Save_SetEventFlag(31);
            Save_SetEventFlag(0x20);
            return 1;
        }
        break;
    case 10:
        if (Save_TestEventFlag(34) == 0 && Cond_Check(51) == 1) {
            gTalkEventFlag = 34;
            Save_SetEventFlag(30);
            Save_SetEventFlag(31);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(33);
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(35) == 0 && Cond_Check(52) == 1) {
            gTalkEventFlag = 35;
            Save_SetEventFlag(30);
            Save_SetEventFlag(31);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(33);
            Save_SetEventFlag(34);
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(36) == 0 && Cond_Check(53) == 1) {
            gTalkEventFlag = 36;
            Save_SetEventFlag(31);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(33);
            Save_SetEventFlag(34);
            Save_SetEventFlag(35);
            return 1;
        }
        break;
    case 6:
        if (Save_TestEventFlag(38) == 0 && Cond_Check(55) == 1) {
            gTalkEventFlag = 38;
            Save_SetEventFlag(31);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(33);
            Save_SetEventFlag(34);
            Save_SetEventFlag(35);
            Save_SetEventFlag(36);
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(39) == 0 && Cond_Check(56) == 1) {
            gTalkEventFlag = 39;
            Save_SetEventFlag(31);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(33);
            Save_SetEventFlag(34);
            Save_SetEventFlag(35);
            Save_SetEventFlag(36);
            Save_SetEventFlag(38);
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(40) == 0 && Cond_Check(57) == 1) {
            gTalkEventFlag = 40;
            Save_SetEventFlag(31);
            Save_SetEventFlag(0x20);
            Save_SetEventFlag(33);
            Save_SetEventFlag(34);
            Save_SetEventFlag(35);
            Save_SetEventFlag(36);
            Save_SetEventFlag(38);
            Save_SetEventFlag(39);
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(58) == 1 && Save_TestEventFlag(37) == 0) {
            gTalkEventFlag = 37;
            Save_SetEventFlag(40);
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(59) == 1 && Save_TestEventFlag(1) == 0) {
            gTalkEventFlag = 1;
            return 1;
        }
        break;
    case 1:
        if (Cond_Check(60) == 1 && Save_TestEventFlag(2) == 0) {
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
        if (Cond_Check(62) == 1 && Save_TestEventFlag(3) == 0) {
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

/* The events of NPC_WORKSHOP_HELPER's script: 1 when event `step` applies
 * now, else 0. The events that play once put their event flag in
 * gTalkEventFlag. */
s32 Talk_CheckWorkshopHelper(u16 step)
{
    switch (step) {
    case 2:
        if (Save_TestEventFlag(4) == 0) {
            gTalkEventFlag = 4;
            return 1;
        }
        break;
    case 1:
        if (Save_IsQuestCleared(QUEST_ID_FORTRESS_STORY) != 0 && Save_TestEventFlag(427) == 0) {
            gTalkEventFlag = 427;
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

/* The events of NPC_DRINK_CONTEST_HOST's script: 1 when event `step` applies
 * now, else 0. The events that play once put their event flag in
 * gTalkEventFlag. */
s32 Talk_CheckDrinkContestHost(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(428) == 0) {
            gTalkEventFlag = 428;
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(14) == 1 && Save_TestEventFlag(429) == 0) {
            gTalkEventFlag = 429;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(15) == 1 && Save_TestEventFlag(430) == 0) {
            gTalkEventFlag = 430;
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(19) == 1 && Save_TestEventFlag(431) == 0) {
            gTalkEventFlag = 431;
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(21) == 1 && Save_TestEventFlag(422) == 0) {
            gTalkEventFlag = 422;
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(24) == 1 && Save_TestEventFlag(432) == 0) {
            gTalkEventFlag = 432;
            Save_SetEventFlag(422);
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(25) == 1 && Save_TestEventFlag(433) == 0) {
            gTalkEventFlag = 433;
            return 1;
        }
        break;
    case 8:
        if (Cond_Check(26) == 1 && Save_TestEventFlag(434) == 0) {
            gTalkEventFlag = 434;
            Save_SetEventFlag(433);
            return 1;
        }
        break;
    case 9:
        if (Cond_Check(27) == 1 && Save_TestEventFlag(435) == 0) {
            gTalkEventFlag = 435;
            Save_SetEventFlag(434);
            Save_SetEventFlag(433);
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

/* The events of NPC_NAMING_TALKER's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_CheckNamingTalker(u16 step)
{
    switch (step) {
    case 3:
        if (PspGift_Pick() != -1) {
            return 1;
        }
        break;
    case 1:
        if (Save_TestEventFlag(436) == 0) {
            gTalkEventFlag = 436;
            return 1;
        }
        break;
    case 2:
        if (Village_MilestoneState(3) != 0) {
            return 1;
        }
        break;
    case 4:
        if (PspLink_Count() != 0 && Save_TestEventFlag(437) == 0) {
            gTalkEventFlag = 437;
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(438) == 0) {
            gTalkEventFlag = 438;
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(12) == 1 && Save_TestEventFlag(439) == 0) {
            gTalkEventFlag = 439;
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(13) == 1 && Save_TestEventFlag(440) == 0) {
            gTalkEventFlag = 440;
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

/* The events of townsperson 19's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check13(u16 step)
{
    switch (step) {
    case 5:
        if (Save_TestEventFlag(364) == 0) {
            gTalkEventFlag = 364;
            return 1;
        }
        break;
    case 2:
        if ((u8)Village_FacilityLevel(0) >= 3 && Save_TestEventFlag(365) == 0) {
            gTalkEventFlag = 365;
            return 1;
        }
        break;
    case 1:
        if ((u8)Village_FacilityLevel(0) >= 6 && Save_TestEventFlag(366) == 0) {
            gTalkEventFlag = 366;
            return 1;
        }
        break;
    case 3:
        if (Town_SleepEventPending() == 1 && Save_TestEventFlag(367) == 0) {
            gTalkEventFlag = 367;
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(EVENT_FLAG_MINE_STRONG_PICK) == 1 && Save_TestEventFlag(368) == 0) {
            gTalkEventFlag = 368;
            Save_SetEventFlag(367);
            return 1;
        }
        break;
    case 6:
        if (*(u8 *)((u8 *)&gSave + 14323) == 1) {
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

/* The events of townsperson 21's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check15(u16 step)
{
    switch (step) {
    case 3:
        if (Save_TestEventFlag(441) == 0) {
            gTalkEventFlag = 441;
            return 1;
        }
        break;
    case 2:
        if ((u8)Village_FacilityLevel(0) >= 3 && Save_TestEventFlag(442) == 0) {
            gTalkEventFlag = 442;
            return 1;
        }
        break;
    case 1:
        if ((u8)Village_FacilityLevel(0) >= 6 && Save_TestEventFlag(EVENT_FLAG_MINE_STRONG_PICK) == 0 && Save_TestEventFlag(423) == 0) {
            gTalkEventFlag = 423;
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

/* The events of townsperson 22's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check16(u16 step)
{
    switch (step) {
    case 1:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 23's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check17(u16 step)
{
    switch (step) {
    case 1:
        if (Village_Event186Pending() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 24's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check18(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(444) == 0) {
            gTalkEventFlag = 444;
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

/* The events of NPC_FISHING_SPOT's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_CheckFishingSpot(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(41) == 0) {
            gTalkEventFlag = 41;
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

/* The events of townsperson 26's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check1A(u16 step)
{
    switch (step) {
    case 3:
        if (Save_TestEventFlag(445) == 0) {
            gTalkEventFlag = 445;
            return 1;
        }
        break;
    case 1:
        if ((u8)Village_FacilityLevel(2) == 2 && Save_TestEventFlag(446) == 0) {
            gTalkEventFlag = 446;
            Save_SetEventFlag(445);
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
            Save_SetEventFlag(446);
            Save_SetEventFlag(445);
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 27's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check1B(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(447) == 0) {
            gTalkEventFlag = 447;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 28's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check1C(u16 step)
{
    switch (step) {
    case 2:
        if (Save_TestEventFlag(113) == 0) {
            gTalkEventFlag = 113;
            return 1;
        }
        break;
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 30's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check1E(u16 step)
{
    switch (step) {
    case 3:
        if (Save_TestEventFlag(132) == 0) {
            gTalkEventFlag = 132;
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(133) == 0 && Cond_IsLatest(96) == 1) {
            gTalkEventFlag = 133;
            Save_SetEventFlag(132);
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(134) == 0 && Cond_Check(COND_HR_GATE_3_HR_41) == 1) {
            gTalkEventFlag = 134;
            Save_SetEventFlag(133);
            Save_SetEventFlag(132);
            return 1;
        }
        break;
    case 2:
        if (ElderRaid_IsOnNoMovie() == 1) {
            return 1;
        }
        break;
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 31's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check1F(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 32's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check20(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 33's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check21(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 35's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check23(u16 step)
{
    switch (step) {
    case 2:
        if (Save_TestEventFlag(114) == 0) {
            gTalkEventFlag = 114;
            return 1;
        }
        break;
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 36's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check24(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 37's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check25(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 38's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check26(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 39's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check27(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 40's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check28(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(370) == 0) {
            gTalkEventFlag = 370;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(COND_HR_GATE_1_HR_17) == 1 && Save_TestEventFlag(371) == 0) {
            gTalkEventFlag = 371;
            Save_SetEventFlag(370);
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(COND_HR_GATE_2_HR_31) == 1 && Save_TestEventFlag(372) == 0) {
            gTalkEventFlag = 372;
            Save_SetEventFlag(371);
            Save_SetEventFlag(370);
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 41's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check29(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 42's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check2A(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 43's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check2B(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 44's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check2C(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (gLobby->townEvent == 1) {
            return 1;
        }
        break;
    case 3:
        if (gLobby->townEvent == TOWN_EVENT_SALE) {
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(COND_HR_GATE_2_HR_31) == 1 && Save_TestEventFlag(183) == 0) {
            gTalkEventFlag = 183;
            return 1;
        }
        break;
    case 5:
        if (Combine_KnowsMostRecipes() == 1 && Save_TestEventFlag(184) == 0) {
            gTalkEventFlag = 184;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 45's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check2D(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (gLobby->townEvent == 3) {
            return 1;
        }
        break;
    case 3:
        if (gLobby->townEvent == TOWN_EVENT_SALE) {
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(COND_HR_GATE_2_HR_31) == 1 && Save_TestEventFlag(185) == 0) {
            gTalkEventFlag = 185;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 46's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check2E(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (gLobby->townEvent == 2) {
            return 1;
        }
        break;
    case 3:
        if (gLobby->townEvent == TOWN_EVENT_SALE) {
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(COND_HR_GATE_2_HR_31) == 1 && Save_TestEventFlag(186) == 0) {
            gTalkEventFlag = 186;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 47's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check2F(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(COND_HR_GATE_2_HR_31) == 1 && Save_TestEventFlag(332) == 0) {
            gTalkEventFlag = 332;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 48's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check30(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(187) == 0) {
            gTalkEventFlag = 187;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of NPC_FISHING_SPOT_ONLINE's script: 1 when event `step`
 * applies now, else 0. */
s32 Talk_CheckFishingSpotOnline(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 55's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check37(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(COND_HR_ANY) == 1 && Save_TestEventFlag(188) == 0) {
            gTalkEventFlag = 188;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(COND_HR_GATE_0_HR_11) == 1 && Save_TestEventFlag(189) == 0) {
            gTalkEventFlag = 189;
            Save_SetEventFlag(188);
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(COND_HR_GATE_2_HR_31) == 1 && Save_TestEventFlag(190) == 0) {
            gTalkEventFlag = 190;
            Save_SetEventFlag(189);
            Save_SetEventFlag(188);
            return 1;
        }
        break;
    case 5:
        if (Dojo_OnlineChainCleared() == 1 && Save_TestEventFlag(373) == 0) {
            gTalkEventFlag = 373;
            return 1;
        }
        break;
    case 6:
        if (Dojo_OnlineDrillsCleared() == 1 && Save_TestEventFlag(374) == 0) {
            gTalkEventFlag = 374;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 56's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check38(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(COND_HR_ANY) == 1 && Save_TestEventFlag(192) == 0) {
            gTalkEventFlag = 192;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(COND_HR_10) == 1 && Save_TestEventFlag(193) == 0) {
            gTalkEventFlag = 193;
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(COND_HR_16) == 1 && Save_TestEventFlag(194) == 0) {
            gTalkEventFlag = 194;
            Save_SetEventFlag(193);
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(COND_HR_30) == 1 && Save_TestEventFlag(195) == 0) {
            gTalkEventFlag = 195;
            Save_SetEventFlag(194);
            Save_SetEventFlag(193);
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of NPC_GALLERY_KEEPER's script: 1 when event `step` applies
 * now, else 0. The events that play once put their event flag in
 * gTalkEventFlag. */
s32 Talk_CheckGalleryKeeper(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(115) == 0) {
            gTalkEventFlag = 115;
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(COND_HR_GATE_0_HR_11) == 1 && Save_TestEventFlag(375) == 0) {
            gTalkEventFlag = 375;
            Save_SetEventFlag(115);
            return 1;
        }
        break;
    case 3:
        if (gLobby->townEvent == TOWN_EVENT_SALE) {
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(COND_HR_GATE_1_HR_17) == 1 && Save_TestEventFlag(376) == 0) {
            gTalkEventFlag = 376;
            return 1;
        }
        break;
    case 7:
        if (Cond_Check(96) == 1 && Save_TestEventFlag(377) == 0) {
            gTalkEventFlag = 377;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 58's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check3A(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 59's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check3B(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(135) == 0) {
            gTalkEventFlag = 135;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 60's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check3C(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(136) == 0) {
            gTalkEventFlag = 136;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 61's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check3D(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of NPC_OFFICIAL_DESK_LOW_HR's script: 1 when event `step`
 * applies now, else 0. The events that play once put their event flag in
 * gTalkEventFlag. */
s32 Talk_CheckOfficialDeskLowHr(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (ElderRaid_IsOnNoMovie() == 1) {
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
        if (Save_TestEventFlag(96) == 0 && Cond_IsLatest(COND_HR_10) == 1) {
            gTalkEventFlag = 96;
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(97) == 0 && Cond_Check(COND_HR_GATE_0) == 1) {
            gTalkEventFlag = 97;
            return 1;
        }
        break;
    case 6:
        if (Save_TestEventFlag(98) == 0 && Cond_IsLatest(COND_HR_GATE_0_HR_11) == 1 && gSave.hr < 12) {
            gTalkEventFlag = 98;
            return 1;
        }
        break;
    case 7:
        if (Save_TestEventFlag(99) == 0 && Cond_IsLatest(COND_HR_16) == 1) {
            gTalkEventFlag = 99;
            Save_SetEventFlag(98);
            return 1;
        }
        break;
    case 8:
        if (Save_TestEventFlag(100) == 0 && Cond_Check(COND_HR_GATE_1) == 1) {
            gTalkEventFlag = 100;
            Save_SetEventFlag(98);
            return 1;
        }
        break;
    case 9:
        if (Save_TestEventFlag(101) == 0 && Cond_IsLatest(COND_HR_GATE_1_HR_17) == 1 && gSave.hr < 18) {
            gTalkEventFlag = 101;
            Save_SetEventFlag(98);
            return 1;
        }
        break;
    case 10:
        if (gSave.hr >= 22 && Save_TestEventFlag(110) == 0) {
            gTalkEventFlag = 110;
            return 1;
        }
        break;
    case 11:
        if (Save_TestEventFlag(102) == 0 && Cond_Check(COND_HR_30) == 1) {
            Save_SetEventFlag(98);
            gTalkEventFlag = 102;
            return 1;
        }
        break;
    case 12:
        if (Save_TestEventFlag(103) == 0 && Cond_Check(COND_HR_GATE_2) == 1) {
            Save_SetEventFlag(98);
            gTalkEventFlag = 103;
            return 1;
        }
        break;
    case 13:
        if (Save_TestEventFlag(104) == 0 && Cond_Check(COND_HR_GATE_2_HR_31) == 1) {
            Save_SetEventFlag(98);
            Save_SetEventFlag(104);
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 65's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check41(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(EVENT_FLAG_ARENA_CUP_OPEN) == 1 && Save_TestEventFlag(197) == 0) {
            gTalkEventFlag = 197;
            return 1;
        }
        break;
    case 3:
        if (Save_HasTitle(141) == 1 && Save_TestEventFlag(198) == 0) {
            gTalkEventFlag = 198;
            return 1;
        }
        break;
    case 4:
        if (Save_HasTitle(142) == 1 && Save_TestEventFlag(199) == 0) {
            gTalkEventFlag = 199;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 66's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check42(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(EVENT_FLAG_ARENA_CUP_OPEN) == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(EVENT_FLAG_ARENA_CUP_OPEN) == 1 && Save_TestEventFlag(458) == 0) {
            gTalkEventFlag = 458;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 69's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check45(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 70's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check46(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 71's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check47(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 73's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check49(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(317) == 0) {
            gTalkEventFlag = 317;
            return 1;
        }
        break;
    case 3:
        if (gSave.songPoints >= 50 && Save_TestEventFlag(318) == 0) {
            gTalkEventFlag = 318;
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3) == 1 && Save_TestEventFlag(320) == 0) {
            gTalkEventFlag = 320;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 74's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check4A(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 75's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check4B(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 76's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check4C(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 77's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check4D(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (ElderRaid_IsOnNoMovie() == 1) {
            return 1;
        }
        break;
    case 3:
        if (gSave.questKills >= 200 && gSave.hr >= 31 && Save_TestEventFlag(182) == 0) {
            gTalkEventFlag = 182;
            return 1;
        }
        break;
    case 4:
        if (gSave.unk2724 >= 20 && Save_TestEventFlag(333) == 0) {
            gTalkEventFlag = 333;
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(334) == 0) {
            gTalkEventFlag = 334;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of NPC_OFFICIAL_DESK_HIGH_HR's script: 1 when event `step`
 * applies now, else 0. The events that play once put their event flag in
 * gTalkEventFlag. */
s32 Talk_CheckOfficialDeskHighHr(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (ElderRaid_IsOnNoMovie() == 1) {
            return 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(321) == 0) {
            gTalkEventFlag = 321;
            return 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(105) == 0 && Cond_Check(COND_HR_40) == 1) {
            gTalkEventFlag = 105;
            return 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(106) == 0 && Cond_Check(COND_HR_GATE_3) == 1) {
            gTalkEventFlag = 106;
            return 1;
        }
        break;
    case 6:
        if (Save_TestEventFlag(107) == 0 && Cond_IsLatest(COND_HR_GATE_3_HR_41) == 1) {
            Save_SetEventFlag(107);
            return gSave.hr < 42;
        }
        break;
    case 7:
        if (Save_TestEventFlag(108) == 0 && Cond_Check(COND_HR_50) == 1) {
            gTalkEventFlag = 108;
            return 1;
        }
        break;
    case 10:
        if (Save_TestEventFlag(111) == 0 && Cond_Check(105) == 1) {
            gTalkEventFlag = 111;
            return 1;
        }
        break;
    case 11:
        if (Save_TestEventFlag(112) == 0 && Cond_Check(COND_HR_GATE_4) == 1) {
            gTalkEventFlag = 112;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of NPC_QUEST_DESK_2's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_CheckQuestDesk2(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(322) == 0) {
            gTalkEventFlag = 322;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(COND_HR_40) == 1 && Save_TestEventFlag(323) == 0) {
            gTalkEventFlag = 323;
            return 1;
        }
        break;
    case 4:
        if (Cond_Check(COND_HR_50) == 1 && Save_TestEventFlag(324) == 0) {
            gTalkEventFlag = 324;
            Save_SetEventFlag(323);
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 80's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check50(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3) == 1 && Save_TestEventFlag(109) == 0) {
            gTalkEventFlag = 109;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 81's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check51(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 82's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check52(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 83's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check53(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 84's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check54(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 85's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check55(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(325) == 0) {
            gTalkEventFlag = 325;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 86's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check56(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(COND_HR_GATE_0_HR_11) == 1 && Save_TestEventFlag(326) == 0) {
            gTalkEventFlag = 326;
            return 1;
        }
        break;
    case 3:
        if (Cond_Check(COND_HR_GATE_1_HR_17) == 1 && Save_TestEventFlag(327) == 0) {
            gTalkEventFlag = 327;
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(COND_HR_GATE_2_HR_31) == 1 && Save_TestEventFlag(329) == 0) {
            gTalkEventFlag = 329;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 87's script: 1 when event `step` applies now,
 * else 0. */
s32 Talk_Check57(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of NPC_WORKSHOP_HELPER_ONLINE's script: 1 when event `step`
 * applies now, else 0. The events that play once put their event flag in
 * gTalkEventFlag. */
s32 Talk_CheckWorkshopHelperOnline(u16 step)
{
    switch (step) {
    case 1:
        if (ElderRaid_IsOn() == 1) {
            return 1;
        }
        break;
    case 2:
        if (Save_TestEventFlag(331) == 0) {
            gTalkEventFlag = 331;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 105's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check69(u16 step)
{
    switch (step) {
    case 1:
        if (Save_TestEventFlag(310) == 0) {
            Save_SetEventFlag(310);
            QuestList_Rebuild();
            return 1;
        }
        break;
    case 2:
        if (Cond_Check(80) == 1 && Save_TestEventFlag(312) == 0) {
            gTalkEventFlag = 312;
            return 1;
        }
        break;
    case 3:
        if (Cond_IsLatest(COND_HOUSE_SLEEP_EVENT) == 1 && Save_TestEventFlag(448) == 0) {
            gTalkEventFlag = 448;
            return 1;
        }
        break;
    case 6:
        if (Cond_Check(83) == 1 && Save_TestEventFlag(449) == 0) {
            gTalkEventFlag = 449;
            return 1;
        }
        break;
    case 5:
        if (Cond_Check(COND_ALL_MONSTERS_HUNTED) == 1 && Save_TestEventFlag(450) == 0) {
            gTalkEventFlag = 450;
            return 1;
        }
        break;
    case 4:
        if (Monster_IsHunted(59) == 1 && Save_TestEventFlag(451) == 0) {
            gTalkEventFlag = 451;
            return 1;
        }
        break;
    }
    return 0;
}

/* The events of townsperson 107's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_Check6B(u16 step)
{
    switch (step) {
    case 2:
        if (Save_TestEventFlag(452) == 0) {
            gTalkEventFlag = 452;
            return 1;
        }
        break;
    case 1:
        if (Cond_Check(COND_ALL_MONSTERS_HUNTED) == 1 && Save_TestEventFlag(453) == 0) {
            if (Monster_IsHunted(59) == 0) {
                gTalkEventFlag = 453;
                return 1;
            }
            Save_SetEventFlag(453);
        }
        break;
    }
    return 0;
}

/* The events of NPC_HOUSE_COOK's script: 1 when event `step` applies now,
 * else 0. The events that play once put their event flag in gTalkEventFlag. */
s32 Talk_CheckHouseCook(u16 step)
{
    switch (step) {
    case 2:
        if (Save_TestEventFlag(454) == 0) {
            gTalkEventFlag = 454;
            return 1;
        }
        break;
    case 3:
        if (gSave.recordCounts[33] >= 50 && Save_TestEventFlag(460) == 0) {
            gTalkEventFlag = 460;
            return 1;
        }
        break;
    }
    return 0;
}

/* Whether event `step` of script `script` (a townsperson's kind) applies
 * now: the script's own check. Returns 1 if so, 0 if not or for a script
 * with no events. */
s32 Talk_Check(u16 script, u16 step)
{
    switch (script) {
    case 1:
        return Talk_Check01(step);
    case 2:
        return Talk_Check02(step);
    case 3:
        return Talk_Check03(step);
    case 4:
        return Talk_Check04(step);
    case NPC_CONTEST_HOST:
        return Talk_CheckContestHost(step);
    case 6:
        return Talk_Check06(step);
    case 7:
        return Talk_Check07(step);
    case 8:
        return Talk_Check08(step);
    case 9:
        return Talk_Check09(step);
    case 10:
        return Talk_Check0A(step);
    case 11:
        return Talk_Check0B(step);
    case 12:
        return Talk_Check0C(step);
    case 13:
        return Talk_Check0D(step);
    case NPC_WORKSHOP_SMITH:
        return Talk_CheckWorkshopSmith(step);
    case NPC_WORKSHOP_HELPER:
        return Talk_CheckWorkshopHelper(step);
    case NPC_DRINK_CONTEST_HOST:
        return Talk_CheckDrinkContestHost(step);
    case NPC_NAMING_TALKER:
        return Talk_CheckNamingTalker(step);
    case 19:
        return Talk_Check13(step);
    case 21:
        return Talk_Check15(step);
    case 22:
        return Talk_Check16(step);
    case 23:
        return Talk_Check17(step);
    case 24:
        return Talk_Check18(step);
    case NPC_FISHING_SPOT:
        return Talk_CheckFishingSpot(step);
    case 26:
        return Talk_Check1A(step);
    case 27:
        return Talk_Check1B(step);
    case 28:
        return Talk_Check1C(step);
    case 30:
        return Talk_Check1E(step);
    case 31:
        return Talk_Check1F(step);
    case 32:
        return Talk_Check20(step);
    case 33:
        return Talk_Check21(step);
    case 35:
        return Talk_Check23(step);
    case 36:
        return Talk_Check24(step);
    case 37:
        return Talk_Check25(step);
    case 38:
        return Talk_Check26(step);
    case 39:
        return Talk_Check27(step);
    case 40:
        return Talk_Check28(step);
    case 41:
        return Talk_Check29(step);
    case 42:
        return Talk_Check2A(step);
    case 43:
        return Talk_Check2B(step);
    case 44:
        return Talk_Check2C(step);
    case 45:
        return Talk_Check2D(step);
    case 46:
        return Talk_Check2E(step);
    case 47:
        return Talk_Check2F(step);
    case 48:
        return Talk_Check30(step);
    case NPC_FISHING_SPOT_ONLINE:
        return Talk_CheckFishingSpotOnline(step);
    case 55:
        return Talk_Check37(step);
    case 56:
        return Talk_Check38(step);
    case NPC_GALLERY_KEEPER:
        return Talk_CheckGalleryKeeper(step);
    case 58:
        return Talk_Check3A(step);
    case 59:
        return Talk_Check3B(step);
    case 60:
        return Talk_Check3C(step);
    case 61:
        return Talk_Check3D(step);
    case NPC_OFFICIAL_DESK_LOW_HR:
        return Talk_CheckOfficialDeskLowHr(step);
    case 65:
        return Talk_Check41(step);
    case 66:
        return Talk_Check42(step);
    case 69:
        return Talk_Check45(step);
    case 70:
        return Talk_Check46(step);
    case 71:
        return Talk_Check47(step);
    case 73:
        return Talk_Check49(step);
    case 74:
        return Talk_Check4A(step);
    case 75:
        return Talk_Check4B(step);
    case 76:
        return Talk_Check4C(step);
    case 77:
        return Talk_Check4D(step);
    case NPC_OFFICIAL_DESK_HIGH_HR:
        return Talk_CheckOfficialDeskHighHr(step);
    case NPC_QUEST_DESK_2:
        return Talk_CheckQuestDesk2(step);
    case 80:
        return Talk_Check50(step);
    case 81:
        return Talk_Check51(step);
    case 82:
        return Talk_Check52(step);
    case 83:
        return Talk_Check53(step);
    case 84:
        return Talk_Check54(step);
    case 85:
        return Talk_Check55(step);
    case 86:
        return Talk_Check56(step);
    case 87:
        return Talk_Check57(step);
    case NPC_WORKSHOP_HELPER_ONLINE:
        return Talk_CheckWorkshopHelperOnline(step);
    case 105:
        return Talk_Check69(step);
    case 107:
        return Talk_Check6B(step);
    case NPC_HOUSE_COOK:
        return Talk_CheckHouseCook(step);
    default:
        return 0;
    }
}
