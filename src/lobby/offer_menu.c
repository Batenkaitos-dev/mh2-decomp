/*
 * The offer menu a townsperson opens when talked to (gOfferMenu, an
 * OfferMenu, run in TOWN_MODE_OFFER_MENU): the deliveries the townsperson
 * wants and the quests it gives. A delivery is a story delivery from
 * gStoryDeliveries, picked by the village's progress and the event flags, or
 * the townsperson's village request kept in SaveData.offers; the quests are
 * story quests and the village request's quest. OfferMenu_BuildList fills the
 * menu for a townsperson, or only probes it for the mark over its head; the
 * update runs the top menu, the quest list (a quest ordered goes through
 * QuestDesk_Accept) and the delivery list (the items are taken from the pouch,
 * then the item box).
 *
 * Also the item helpers other lobby menus use: the counts across the pouch
 * and the item box, taking and giving items, and the item rows and need
 * lines the forge, workshop and combination menus draw.
 */
#include "common.h"
#include "lobby_text.h"
#include "stageobj.h"
#include "lobby_state.h"
#include "em.h"
#include "lobby_vobj.h"
#include "player.h"
#include "save.h"
#include "lobby_globals.h"
#include "scene.h"
#include "quest.h"
#include "lobby_menu.h"
#include "lobby_npc.h"
#include "pad.h"
#include "lobby_labels.h"
#include "offer_menu.h"
#include "area_draw.h"
#include "bgm.h"
#include "camera.h"
#include "font.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_util.h"
#include "lobby_world.h"
#include "menu_sheets.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "quest_desk.h"
#include "quest_desk_draw.h"
#include "quest_offers.h"
#include "rand.h"
#include "talk.h"
#include "town_frame.h"
#include "village_cond.h"
#include "village_offers.h"
#include "village_progress.h"
#include "village_records.h"
#include "libc.h"

extern OfferMenu gOfferMenu;
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern Delivery gStoryDeliveries[];
u16 Map_Group(u16 area);
u8 SavePouch_Add(u16 item, s16 n);
s32 SavePouch_Count(u16 item);

extern LobbyMenu *gLobbyMenu;
void Text_Printf(char *s, ...);
extern char *D_0080C044;
extern char *gItemNames[];
void Text_PrintfColored(char *s, ...);
void Text_SetSize(s32 size);

/*
 * Fills gOfferMenu with what the townsperson offers: by its id, the story
 * delivery or story quest its progress (Cond_IsLatest) or event flags call
 * for, then its village request and quest. Story delivery k (0 to 9) is
 * offered while event flag 50 + 2k is set and 51 + 2k isn't; delivering
 * it sets 51 + 2k. Returns what it found (enum OfferHas); nothing outside
 * the village. With probe set it fills a copy on the stack instead and returns
 * OFFER_HAS_STORY as soon as a story entry is found: the townspeople's marks
 * are made that way.
 */
s16 OfferMenu_BuildList(Em *townsperson, s32 probe)
{
    OfferMenu *menu;
    u8 *npcId = &((NpcWork *)&townsperson->work)->id;
    OfferMenu probeMenu;
    s32 request;
    u16 quest;

    if (Game_IsOnline() == 1) {
        return 0;
    }
    if (probe == 0) {
        menu = &gOfferMenu;
        memset(menu, 0, sizeof(OfferMenu));
    } else {
        menu = &probeMenu;
        memset(menu, 0, sizeof(OfferMenu));
    }
    menu->npc = townsperson;
    switch (*npcId) {
    case 1:
        if (Cond_IsLatest(2) == 1) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 10);
        } else if (Cond_IsLatest(6) == 1) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 12);
        } else if (Cond_IsLatest(7) == 1) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 13);
        } else if (Cond_IsLatest(9) == 1) {
            OfferMenu_AddQuest(menu, OFFER_SRC_STORY, 5001);
        } else if (Cond_IsLatest(20) == 1) {
            OfferMenu_AddQuest(menu, OFFER_SRC_STORY, 5004);
        } else if (Cond_IsLatest(23) == 1) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 16);
        } else if (Cond_IsLatest(25) == 1) {
            if (Save_IsQuestCleared(5005) == 0) {
                OfferMenu_AddQuest(menu, OFFER_SRC_STORY, 5005);
            } else if (Save_IsQuestCleared(5006) == 0) {
                OfferMenu_AddQuest(menu, OFFER_SRC_STORY, 5006);
            } else if (Save_IsQuestCleared(5007) == 0) {
                OfferMenu_AddQuest(menu, OFFER_SRC_STORY, 5007);
            } else {
                break;
            }
        } else {
            break;
        }
        if (probe == 1 && menu->questCount + menu->count != 0) {
            return OFFER_HAS_STORY;
        }
        break;
    case 4:
        if (Save_TestEventFlag(121) != 0) {
            if (Save_TestEventFlag(48) == 0) {
                OfferMenu_AddQuest(menu, OFFER_SRC_STORY, 6001);
            } else if (Village_MilestoneState(6)) {
                OfferMenu_AddQuest(menu, OFFER_SRC_STORY, 6001);
            }
        }
        break;
    case 5:
        if (Save_TestEventFlag(60) == 1 && Save_TestEventFlag(61) == 0) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 5);
        } else if (Save_TestEventFlag(62) == 1 && Save_TestEventFlag(63) == 0) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 6);
        } else if (Save_TestEventFlag(0x40) == 1 && Save_TestEventFlag(65) == 0) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 7);
        } else if (Save_TestEventFlag(66) == 1 && Save_TestEventFlag(67) == 0) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 8);
        } else if (Save_TestEventFlag(68) == 1 && Save_TestEventFlag(69) == 0) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 9);
        }
        if (probe == 1 && menu->questCount + menu->count != 0) {
            return OFFER_HAS_STORY;
        }
        break;
    case 7:
        if (Cond_IsLatest(18) == 1) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 15);
            if (probe == 1 && menu->questCount + menu->count != 0) {
                return OFFER_HAS_STORY;
            }
        }
        break;
    case 8:
        if (Cond_IsLatest(4) == 1) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 11);
            if (probe == 1 && menu->questCount + menu->count != 0) {
                return OFFER_HAS_STORY;
            }
        }
        break;
    case 14:
        if (Save_TestEventFlag(50) == 1 && Save_TestEventFlag(51) == 0) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 0);
        } else if (Save_TestEventFlag(52) == 1 && Save_TestEventFlag(53) == 0) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 1);
        } else if (Save_TestEventFlag(54) == 1 && Save_TestEventFlag(55) == 0) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 2);
        } else if (Save_TestEventFlag(56) == 1 && Save_TestEventFlag(57) == 0) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 3);
        } else if (Save_TestEventFlag(58) == 1 && Save_TestEventFlag(59) == 0) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 4);
        }
        if (probe == 1 && menu->questCount + menu->count != 0) {
            return OFFER_HAS_STORY;
        }
        break;
    case 15:
        OfferMenu_AddQuest(menu, OFFER_SRC_STORY, QUEST_ID_FORTRESS_STORY);
        break;
    case 16:
        if (Cond_IsLatest(13) == 1) {
            OfferMenu_AddQuest(menu, OFFER_SRC_STORY, 5002);
        } else if (Cond_IsLatest(14) == 1) {
            OfferMenu_AddQuest(menu, OFFER_SRC_STORY, 5003);
        } else if (Cond_IsLatest(24) == 1) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 17);
        } else {
            break;
        }
        if (probe == 1 && menu->questCount + menu->count != 0) {
            return OFFER_HAS_STORY;
        }
        break;
    case 18:
        if (Cond_IsLatest(12) == 1) {
            OfferMenu_AddDelivery(menu, OFFER_SRC_STORY, 14);
            if (probe == 1 && menu->questCount + menu->count != 0) {
                return OFFER_HAS_STORY;
            }
        }
        break;
    case 105:
        if (Cond_IsLatest(82) == 1) {
            OfferMenu_AddQuest(menu, OFFER_SRC_STORY, 5008);
            if (probe == 1 && menu->questCount + menu->count != 0) {
                return OFFER_HAS_STORY;
            }
        }
        break;
    }
    request = Offer_FindRequest(*npcId);
    if (request != -1) {
        OfferMenu_AddRequest(menu, OFFER_SRC_VILLAGE, request);
    }
    quest = Offer_GetQuest(*npcId);
    if (quest != 0xFFFF) {
        OfferMenu_AddQuest(menu, OFFER_SRC_VILLAGE, quest);
    }
    if (menu->count != 0 && menu->questCount != 0) {
        menu->has = OFFER_HAS_BOTH;
    } else if (menu->count != 0) {
        menu->has = OFFER_HAS_DELIVERIES;
    } else if (menu->questCount != 0) {
        menu->has = OFFER_HAS_QUESTS;
    } else {
        menu->has = OFFER_HAS_NONE;
    }
    return menu->has;
}

/* Adds a quest (source: OFFER_SRC_*; info its disc info) when there is
 * room and it fits the season and the time of day; keeps its map's group for the sheet's tab and
 * map strip. */
void OfferMenu_AddQuest(OfferMenu *menu, s16 source, u16 quest)
{
    QuestInfo *info = Quest_GetDiscInfo(quest);
    s32 season;
    s32 isPm;

    if (menu->questCount != OFFER_MENU_ENTRIES) {
        season = LobbyWorld_GetSeason();
        isPm = LobbyTime_IsPm();
        if (Quest_MatchesSeasonTime(quest, season, isPm) != 0) {
            menu->questSource[menu->questCount] = source;
            menu->quests[menu->questCount] = quest;
            menu->questTabs[menu->questCount] = Map_Group(info->map);
            if (source == 0) {
                menu->questIsVillage[menu->questCount] = 0;
            } else {
                menu->questIsVillage[menu->questCount] = 1;
            }
            menu->questCount++;
        }
    }
}

/* Adds a delivery (origin: OFFER_SRC_*) when there is room; a story one is a
 * copy of gStoryDeliveries[storyIdx]. */
void OfferMenu_AddDelivery(OfferMenu *menu, s16 origin, s16 storyIdx)
{
    if (menu->count != OFFER_MENU_ENTRIES) {
        menu->source[menu->count] = origin;
        menu->story[menu->count] = storyIdx;
        menu->tabs[menu->count] = 16;
        if (origin == 0) {
            menu->entries[menu->count] = gStoryDeliveries[storyIdx];
            menu->period[menu->count] = 0;
        }
        menu->count++;
    }
}

/* SaveData.offers[idx], reached from the save's address: the record of the
 * village request (offers is at the offset 0x299C; with gSave.entries[idx] the
 * compiler computes the address differently). */
static inline OfferRec *Save_OfferRec(u8 *save, s32 idx)
{
    return (OfferRec *)(save + idx * sizeof(OfferRec) + 10652);
}
/* Adds the village request kept in SaveData.offers[offer] as a delivery: its
 * items and points, and the title and text it was made with. */
void OfferMenu_AddRequest(OfferMenu *menu, s16 origin, s32 offer)
{
    OfferRec *rec;

    if (menu->count != OFFER_MENU_ENTRIES) {
        menu->source[menu->count] = origin;
        menu->story[menu->count] = 0;
        menu->tabs[menu->count] = 16;
        rec = Save_OfferRec((u8 *)&gSave, offer);
        memcpy(&menu->entries[menu->count], rec, sizeof(rec->want));
        menu->entries[menu->count].points = rec->points;
        strcpy(menu->entries[menu->count].title, Request_Title(offer));
        strcpy(menu->entries[menu->count].text, Request_Text(offer));
        menu->period[menu->count] = 1;
        menu->count++;
    }
}

/* Opens the menu built for the townsperson at the top menu. When the menu
 * running was built for another townsperson, it is closed first. */
void OfferMenu_Open(Em *townsperson)
{
    OfferMenu *menu = &gOfferMenu;

    if (menu->npc->index != townsperson->index) {
        gTownMode = TOWN_MODE_FREE;
        LobbyMenu_SetDraw(0, 0, 0, 0);
        gTownMenuOpen = 0;
        CamFocus_End();
    }
    menu->step = 1;
    gTownMode = TOWN_MODE_OFFER_MENU;
    LobbyMenu_SetDraw(OfferMenu_Draw, 0, 0, 0);
}

/* Closes the menu and refreshes the townspeople's marks. */
void OfferMenu_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
    Npc_RefreshOffers();
}

/* The menu's frame, by OfferMenu.step: the opening talk, the top menu
 * (deliveries or quests; cross says goodbye), the quest list, the delivery
 * list, and the goodbye talk before it closes. */
void OfferMenu_Update(void)
{
    OfferMenu *menu = &gOfferMenu;
    u16 pad = LobbyPad_Get(0);

    switch (menu->step) {
    case OFFER_STEP_TALK:
        switch (Talk_Update()) {
        case 0:
        case 3:
            menu->step++;
            LobbySe_Play(17);
            break;
        }
        break;
    case OFFER_STEP_TOP:
        if (pad & PAD_CIRCLE) {
            menu->listSel = menu->topCur;
            menu->cur = 0;
            if (menu->has == OFFER_HAS_QUESTS) {
                menu->listSel = 1;
            }
            if (menu->listSel == 0) {
                menu->step = OFFER_STEP_DELIVERIES;
                menu->listLen = menu->count;
            } else {
                menu->step = OFFER_STEP_QUESTS;
                menu->listLen = menu->questCount;
                menu->page = 0;
                if (Equip_GunHasNoAmmo(&gPlayers[gScene.localPlayer], &gSave) == 1) {
                    Menu_PushNoticeOnce(19);
                }
            }
            WinSlide_Step(&menu->win, 640, 280);
            CamFocus_SetAside(1);
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
        } else if (pad & PAD_CROSS) {
            menu->step = OFFER_STEP_BYE;
            Talk_Say(1, 0);
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (menu->has == 1) {
            menu->topCur = Menu_WrapUpDown(menu->topCur, 2);
        }
        break;
    case OFFER_STEP_QUESTS:
        switch (OfferMenu_QuestList(pad)) {
        case MENU_DONE:
            menu->step = OFFER_STEP_BYE;
            break;
        case MENU_CANCEL:
            menu->step = OFFER_STEP_TOP;
            break;
        }
        break;
    case OFFER_STEP_DELIVERIES:
        switch (OfferMenu_DeliveryList(pad)) {
        case MENU_DONE:
            menu->step++;
            break;
        case MENU_CANCEL:
            menu->step = OFFER_STEP_TOP;
            break;
        }
        break;
    case OFFER_STEP_BYE:
        switch (Talk_Update()) {
        case 0:
            OfferMenu_Close();
            break;
        }
        break;
    }
}

extern u16 gQuestStampRand;
/*
 * The quest list, by OfferMenu.listStep: circle orders the quest under the
 * cursor when no quest is taken and the fee can be paid (a yes/no), up and
 * down move the cursor, left and right turn the sheet's pages. A yes stamps
 * the sheet, orders the quest and notes the townsperson in gQuestOfferId.
 * Returns MENU_DONE after the closing circle, MENU_CANCEL on cross, else
 * MENU_RUNNING.
 */
s32 OfferMenu_QuestList(u16 pad)
{
    OfferMenu *menu = &gOfferMenu;
    QuestInfo *info = Quest_GetDiscInfo(menu->quests[menu->cur]);

    switch (menu->listStep) {
    case OFFER_LIST_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case MENU_DONE:
            menu->listStep++;
            break;
        }
        break;
    case OFFER_LIST_CHOOSE:
        if (pad & PAD_CIRCLE) {
            if (gLobby->questTaken == 0) {
                if (info->fee > gSave.money) {
                    LobbySe_Play(LOBBY_SE_REFUSE);
                } else {
                    menu->listStep++;
                    Menu_SetYesNo(0);
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                }
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
        } else if (pad & PAD_CROSS) {
            menu->listStep = OFFER_LIST_SLIDE_IN;
            CamFocus_SetAside(0);
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else {
            menu->cur = Menu_WrapUpDownSe(menu->cur, menu->listLen, 23);
            menu->page = Menu_WrapLeftRight(menu->page, QUEST_SHEET_PAGES);
        }
        break;
    case OFFER_LIST_CONFIRM:
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            menu->listStep++;
            menu->stampSeed = Rand_Next(1);
            if (menu->stampSeed == 0) {
                menu->stampSeed = 1;
            }
            gQuestStampRand = menu->stampSeed;
            Win_Shake(&menu->shake);
            break;
        case MENU_CANCEL:
            menu->listStep = OFFER_LIST_CHOOSE;
            break;
        }
        break;
    case OFFER_LIST_STAMP:
        switch (Win_Shake(&menu->shake)) {
        case MENU_DONE:
            menu->listStep++;
            QuestDesk_Accept(info->id, 0, 1);
            gQuestOfferId = ((NpcWork *)&menu->npc->work)->id;
            break;
        }
        break;
    case OFFER_LIST_DONE:
        if (pad & PAD_CIRCLE) {
            menu->listStep = OFFER_LIST_SLIDE_IN;
            CamFocus_SetAside(0);
            Talk_Say(1, 0);
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * The delivery list, by OfferMenu.listStep: circle offers to deliver the
 * entry under the cursor when its items are at hand (a yes/no); a yes stamps
 * the sheet and plays the delivery's jingle, and the closing circle thanks
 * (townsperson 3 with its own line) and delivers. Returns as
 * OfferMenu_QuestList.
 */
s32 OfferMenu_DeliveryList(u16 pad)
{
    OfferMenu *menu = &gOfferMenu;
    u8 *npcId = &((NpcWork *)&menu->npc->work)->id;

    switch (menu->listStep) {
    case OFFER_LIST_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case MENU_DONE:
            menu->listStep++;
            break;
        }
        break;
    case OFFER_LIST_CHOOSE:
        if (pad & PAD_CIRCLE) {
            if (OfferMenu_HasItems() == 1) {
                menu->listStep++;
                Menu_SetYesNo(1);
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
        } else if (pad & PAD_CROSS) {
            menu->listStep = OFFER_LIST_SLIDE_IN;
            CamFocus_SetAside(0);
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else {
            menu->cur = Menu_WrapUpDown(menu->cur, menu->listLen);
        }
        break;
    case OFFER_LIST_CONFIRM:
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            menu->listStep++;
            menu->stampSeed = Rand_Next(1);
            if (menu->stampSeed == 0) {
                menu->stampSeed = 1;
            }
            Win_Shake(&menu->shake);
            break;
        case MENU_CANCEL:
            menu->listStep = OFFER_LIST_CHOOSE;
            break;
        }
        break;
    case OFFER_LIST_STAMP:
        switch (Win_Shake(&menu->shake)) {
        case MENU_DONE:
            menu->listStep++;
            Jingle_Play(&gPlayers[gScene.localPlayer], 11);
            break;
        }
        break;
    case OFFER_LIST_DONE:
        if (pad & PAD_CIRCLE) {
            menu->listStep = OFFER_LIST_SLIDE_IN;
            CamFocus_SetAside(0);
            if (*npcId == 3) {
                Talk_Say(18, 0);
            } else {
                Talk_Say(2, 0);
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
            OfferMenu_Deliver();
            return MENU_DONE;
        }
        break;
    }
    return MENU_RUNNING;
}

/* Takes count of item from the pouch first, the rest from the item box;
 * nothing for item 0. */
void Items_Take(s32 item, s32 count)
{
    s32 inPouch;

    if (item != 0) {
        inPouch = SavePouch_Count(item);
        if (inPouch != 0) {
            if (inPouch >= count) {
                SavePouch_Add(item, -count);
                return;
            }
            SavePouch_Add(item, -inPouch);
            count -= inPouch;
        }
        ItemBox_Total(item);
        ItemBox_Remove(item, count);
    }
}

/* Puts count of item in the pouch as far as it has room, the rest in the item
 * box. Returns 1 when some went to the box, else 0 (also for item 0). */
s8 Items_Give(s32 item, s32 count)
{
    s32 space;

    if (item == 0) {
        return 0;
    }
    space = SavePouch_Space(item);
    if (space != 0 && space != -1) {
        if (space < count) {
            SavePouch_Add(item, space);
            count -= space;
        } else {
            SavePouch_Add(item, count);
            return 0;
        }
    }
    ItemBox_AddStacks(item, count);
    return 1;
}

/* How many more of item the pouch and the item box can take together, at
 * most 999. */
s32 Items_Room(s32 item)
{
    s32 space;
    s32 total;

    space = SavePouch_Space(item);
    if (space == -1) {
        space = 0;
    }
    total = space + ItemBox_Room(item);
    if (total > 999) {
        return 999;
    }
    return total;
}

/*
 * Delivers the entry under the cursor: takes its items and gives its points
 * to the townsperson. A story delivery sets its event flag (some also reset
 * the townspeople's talks and rebuild the quest list) and moves the village
 * on; a village request is cleared from the save.
 */
void OfferMenu_Deliver(void)
{
    OfferMenu *menu = &gOfferMenu;
    s8 i;
    u8 *npcId;

    for (i = 0; i < DELIVERY_WANTS; i++) {
        if (menu->entries[menu->cur].want[i].item != 0) {
            Items_Take(menu->entries[menu->cur].want[i].item, menu->entries[menu->cur].want[i].count);
        }
    }
    npcId = &((NpcWork *)&menu->npc->work)->id;
    Offer_AddPoints(*npcId, menu->entries[menu->cur].points);
    if (menu->source[menu->cur] == 0) {
        switch (menu->story[menu->cur]) {
        case 10:
            Save_SetEventFlag(6);
            Npc_ResetTalked();
            QuestList_Rebuild();
            break;
        case 11:
            Save_SetEventFlag(8);
            Npc_ResetTalked();
            break;
        case 12:
            Save_SetEventFlag(11);
            Npc_ResetTalked();
            QuestList_Rebuild();
            break;
        case 13:
            Save_SetEventFlag(12);
            Npc_ResetTalked();
            break;
        case 14:
            Save_SetEventFlag(72);
            Npc_ResetTalked();
            break;
        case 15:
            Save_SetEventFlag(80);
            Npc_ResetTalked();
            break;
        case 16:
            Save_SetEventFlag(85);
            Npc_ResetTalked();
            break;
        case 17:
            Save_SetEventFlag(87);
            Npc_ResetTalked();
            break;
        case 0:
            Save_SetEventFlag(51);
            break;
        case 1:
            Save_SetEventFlag(53);
            break;
        case 2:
            Save_SetEventFlag(55);
            break;
        case 3:
            Save_SetEventFlag(57);
            break;
        case 4:
            Save_SetEventFlag(59);
            break;
        case 5:
            Save_SetEventFlag(61);
            break;
        case 6:
            Save_SetEventFlag(63);
            break;
        case 7:
            Save_SetEventFlag(65);
            break;
        case 8:
            Save_SetEventFlag(67);
            break;
        case 9:
            Save_SetEventFlag(69);
            break;
        }
        Village_UpdateProgress(1);
    } else {
        Offer_ClearRequest(*npcId);
    }
}

/* The need lines' format: the name, a separator, the pouch and box counts,
 * the count needed and a mark. */
#pragma rodata on
char gItemNeedFmt[22] = {
#include "lobby/00826000.inc"
};
#pragma rodata off

/* 1 when the pouch and the item box hold count of item together (always for
 * item 0), else 0. */
s32 Items_HaveEnough(u16 item, s32 count)
{
    s32 have;

    if (item == 0) {
        return 1;
    }
    have = SavePouch_Count(item);
    return have + ItemBox_Total(item) >= count;
}

/* How many of item the pouch and the item box hold together, at most 999; 0
 * for item 0. */
s32 Items_Total(u16 item)
{
    s32 total;

    if (item == 0) {
        return 0;
    }
    total = SavePouch_Count(item);
    total += ItemBox_Total(item);
    if (total > 999) {
        total = 999;
    }
    return total;
}

/* 1 when every item the entry under the cursor wants is at hand, else 0. */
s32 OfferMenu_HasItems(void)
{
    OfferMenu *menu = &gOfferMenu;
    s8 i;

    for (i = 0; i < DELIVERY_WANTS; i++) {
        ItemStack *wanted = &menu->entries[menu->cur].want[i];

        if (wanted->item != 0 && Items_HaveEnough(wanted->item, wanted->count) == 0) {
            return 0;
        }
    }
    return 1;
}

/* Draws the message window under the delivery sheet with the prompt's text
 * (enum DeliveryPrompt); with the yes/no, the choice too. */
void OfferMenu_DrawDeliveryPrompt(s16 x, s16 y, s8 prompt)
{
    Win_DrawText(x, y, 338, 60, 0xBDB59F6C, 0xC0101010, gOfferMenuTexts[OFFER_TEXT_DELIVERY_PROMPT + prompt]);
    if (prompt == 2) {
        Font_SetPos(x + 140, y + 30);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
    }
}

/* Draws an item's icon and its need line: the name, the pouch and box counts
 * (at most 999) and the count needed, greyed when there aren't enough.
 * Returns 0 when there aren't enough, else 1 (also for item 0). */
s32 Items_DrawNeedLine(s16 x, s16 y, u16 item, s32 count, u32 color)
{
    s32 pouch;
    s32 box;
    s8 missing = 0;

    if (item == 0) {
        return 1;
    }
    pouch = SavePouch_Count(item);
    box = ItemBox_Total(item);
    if (box > 999) {
        box = 999;
    }
    if (Items_HaveEnough(item, count) == 0) {
        Font_SetColor(0x80303030);
        missing = 1;
    } else {
        Font_SetColor(color);
    }
    Font_SetPos(x, y);
    Text_PrintfColored(gItemNeedFmt, gItemNames[item], gLobbyWords[LOBBY_WORD_COLON], pouch, box, count, D_0080C044);
    Item_DrawIcon(x - 26, y - 2, 20, item);
    if (missing != 0) {
        return 0;
    }
    return 1;
}

/* As Items_DrawNeedLine, with the pouch and box counts as they will be once
 * count is taken (the pouch first). Returns 1. */
s32 Items_DrawNeedLineAfter(s16 x, s16 y, u16 item, s32 count, u32 color)
{
    s32 need;
    s32 pouch;
    s32 box;

    if (item == 0) {
        return 1;
    }
    need = count;
    pouch = SavePouch_Count(item);
    box = ItemBox_Total(item);
    Font_SetColor(color);
    if (pouch >= count) {
        pouch -= count;
    } else {
        count -= pouch;
        box -= count;
        pouch = 0;
    }
    if (box > 999) {
        box = 999;
    }
    Font_SetPos(x, y);
    Text_PrintfColored(gItemNeedFmt, gItemNames[item], gLobbyWords[LOBBY_WORD_COLON], pouch, box, need, D_0080C044);
    Item_DrawIcon(x - 26, y - 2, 20, item);
    return 1;
}

/* Draws an item's icon, name, pouch count and box count (at most 999);
 * nothing for item 0. */
void Menu_DrawItemCountRow(s16 x, s16 y, u16 item, u32 color)
{
    s32 pouch;
    s32 box;

    if (item != 0) {
        pouch = SavePouch_Count(item);
        box = ItemBox_Total(item);
        if (box > 999) {
            box = 999;
        }
        Font_SetColor(color);
        Font_SetPos(x, y);
        Text_PrintfColored("%-16s%s%2d(%3d)", gItemNames[item], gLobbyWords[LOBBY_WORD_COLON], pouch, box);
        Item_DrawIcon(x - 36, y - 2, 20, item);
    }
}

/* Draws an item's icon, name and price with the money mark; nothing for item
 * 0. */
void Menu_DrawItemPriceRow(s16 x, s16 y, u16 item, u32 color, s32 price)
{
    if (item != 0) {
        Font_SetColor(color);
        Font_SetPos(x, y);
        Text_PrintfColored("%-16s %7d%s", gItemNames[item], price, gLobbyWords[LOBBY_WORD_ZENNY]);
        Item_DrawIcon(x - 36, y - 2, 20, item);
    }
}

/*
 * Draws the delivery sheet under the cursor: the title bar, the sheet with a
 * tab per entry, the entry's title, its items' need lines, the period and the
 * client's name, its text, the prompt window and the stamp once stamped.
 * The title is centred on the parameter x + 180, not on its copy left: then
 * the need lines' x + 60 needs no extra register copy, as in the game's
 * code. Menu_DrawStamp is declared with a u16 seed and an s16 frame: with an
 * s32, the load of y moves.
 */
void OfferMenu_DrawDelivery(s16 x, s16 y)
{
    s32 labelX;
    s16 tabLabels[7];
    s32 left;
    OfferMenu *menu = &gOfferMenu;
    s16 lineY = y;
    s32 valueX;
    u16 item;
    u8 *client = &((NpcWork *)&menu->npc->work)->id;
    s8 missing;
    s16 i;

    for (i = 0; i < OFFER_MENU_ENTRIES + 1; i++) {
        /* read as unsigned: the game's code loads the labels with lhu */
        tabLabels[i] = *(u16 *)&menu->tabs[i];
        if (menu->cur == i) {
            tabLabels[i] = -tabLabels[i];
        }
    }
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(280, 18, 180, 0xFF106060, gOfferMenuTexts[OFFER_TEXT_DELIVERY], 2, 1);
    Menu_DrawQuestSheet(x, y, 360, 300, tabLabels, menu->cur, -1, -2);
    Font_SetSpacing(18, 18);
    Font_SetColor(0x80101010);
    left = x;
    Font_SetPos(Text_CenterX(menu->entries[menu->cur].title, x + 180, 18), lineY += 34);
    Text_PrintfColored(menu->entries[menu->cur].title);
    Font_SetColor(0x80301010);
    lineY += 16;
    missing = 0;
    i = 0;
    for (; i < DELIVERY_WANTS; i++) {
        item = menu->entries[menu->cur].want[i].item;
        Font_SetPos(x + 60, lineY += 20);
        if (item != 0) {
            if (Items_DrawNeedLine(x + 60, lineY, item, menu->entries[menu->cur].want[i].count, 0x80101010) != 1) {
                missing = 1;
            }
        }
    }
    Font_SetColor(0x80301010);
    labelX = left + 40;
    Font_SetPos(labelX, lineY += 36);
    Text_PrintfColored(gOfferMenuTexts[OFFER_TEXT_LIMIT]);
    Font_SetColor(0x80101010);
    valueX = left + 140;
    Font_SetPos(valueX, lineY);
    Text_PrintfColored(gQuestPeriodTexts[menu->period[menu->cur]]);
    Font_SetColor(0x80301010);
    Font_SetPos(labelX, lineY += 26);
    Text_PrintfColored(gOfferMenuTexts[OFFER_TEXT_CLIENT]);
    Font_SetColor(0x80101010);
    Font_SetPos(valueX, lineY);
    Text_PrintfColored(((NpcWork *)client)->name);
    Font_SetColor(0x80101010);
    Font_SetPos(left + 50, (s16)(lineY + 8) + 20);
    Text_PrintfColored(menu->entries[menu->cur].text);
    switch (menu->listStep) {
    case OFFER_LIST_STAMP:
    case OFFER_LIST_DONE:
        Win_DrawText(left + 10, 372, 338, 60, 0xBDB59F6C, 0xC0101010, gOfferMenuTexts[OFFER_TEXT_ITEM_LINE]);
        break;
    case OFFER_LIST_CONFIRM:
        OfferMenu_DrawDeliveryPrompt(left + 10, 372, DELIVERY_PROMPT_CONFIRM);
        break;
    default:
        OfferMenu_DrawDeliveryPrompt(left + 10, 372, missing);
        break;
    }
    if (menu->stampSeed != 0) {
        Menu_DrawStamp(x, y, 360, 300, menu->stampSeed, menu->shake.timer);
    }
}

extern s8 gMapGroupMapIds[][2];
/* Draws the quest sheet under the cursor: the title bar, the money, the sheet
 * with the quest's map strip and page (overview, terms, details), the prompt
 * window, the stamp and the cleared mark. */
void OfferMenu_DrawQuest(s16 x, s16 y)
{
    OfferMenu *menu = &gOfferMenu;
    QuestInfo *info;
    s16 tabLabels[7];
    s16 i;

    info = Quest_GetDiscInfo(menu->quests[menu->cur]);
    for (i = 0; i < OFFER_MENU_ENTRIES + 1; i++) {
        /* read as unsigned: the game's code loads the labels with lhu */
        tabLabels[i] = *(u16 *)&menu->questTabs[i];
        if (menu->cur == i) {
            tabLabels[i] = -tabLabels[i];
        }
    }
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(280, 18, 180, 0xFF106060, gOfferMenuTexts[OFFER_TEXT_QUEST_ORDER], 2, 1);
    Menu_DrawMoney(480, 16, 0);
    Menu_DrawQuestSheet(x, y, 340, 360, tabLabels, menu->cur, gMapGroupMapIds[(u16)menu->questTabs[menu->cur]][0], -2);
    Font_SetSpacing(18, 18);
    Font_SetColor(0x80101010);
    Font_SetColor(0x80101010);
    if (menu->listStep == 1) {
        Menu_DrawPageNum(x + 290, y + 14, menu->page, QUEST_SHEET_PAGES, 1);
    }
    Font_SetSpacing(18, 18);
    switch (menu->page) {
    case 0:
        Quest_DrawOverview(x + 40, y + 14, info);
        break;
    case 1:
        Quest_DrawTerms(x + 40, y + 14, info);
        break;
    case 2:
        Quest_DrawDetails(x + 40, y + 14, info);
        break;
    }
    if (gLobby->questTaken != 0 && menu->listStep != OFFER_LIST_STAMP && menu->listStep != OFFER_LIST_DONE) {
        Win_DrawText(x, 372, 338, 60, 0xBDB59F6C, 0xC0101010, gOfferHelpTexts[OFFER_HELP_QUEST_TAKEN]);
    } else {
        switch (menu->listStep) {
        case OFFER_LIST_CONFIRM:
            Win_DrawText(x, 372, 338, 60, 0xBDB59F6C, 0xC0101010, gOfferHelpTexts[OFFER_HELP_ORDER_CONFIRM]);
            Font_SetPos(x + 140, 402);
            Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
            break;
        case OFFER_LIST_STAMP:
        case OFFER_LIST_DONE:
            Win_DrawText(x, 372, 338, 60, 0xBDB59F6C, 0xC0101010, gOfferHelpTexts[OFFER_HELP_ORDERED]);
            break;
        default:
            if (info->fee > gSave.money) {
                Win_DrawText(x, 372, 338, 60, 0xBDB59F6C, 0xC0101010, gOfferHelpTexts[OFFER_HELP_NO_MONEY]);
            } else {
                Win_DrawText(x, 372, 338, 60, 0xBDB59F6C, 0xC0101010, gOfferHelpTexts[OFFER_HELP_ORDER_ASK]);
            }
            break;
        }
    }
    if (menu->stampSeed != 0) {
        Menu_DrawStamp(x + 40, y + 14, 340, 360, menu->stampSeed, menu->shake.timer);
    }
    if (Save_IsQuestCleared(info->id) != 0) {
        Quest_DrawClearMark((s16)(x + 40) + 30, -214);
    }
}

/* The menu's draw: the top menu's choices, or the delivery or quest sheet at
 * the window's slid and shaken position. */
void OfferMenu_Draw(MenuTask *task)
{
    OfferMenu *menu = &gOfferMenu;
    s16 x;
    s16 y;

    Text_SetSize(task->textSize);
    Font_SetColor(0x80101010);
    x = menu->win.pos + menu->shake.dx;
    y = menu->shake.dy + 58;
    switch (menu->step) {
    case OFFER_STEP_TOP:
        switch (menu->has) {
        case OFFER_HAS_DELIVERIES:
            Menu_DrawLabelList(460, gOfferMenuTexts, 1, menu->topCur, 0xFF106060);
            break;
        case OFFER_HAS_QUESTS:
            Menu_DrawLabelList(460, &gOfferMenuTexts[OFFER_TEXT_QUEST_ORDER], 1, menu->topCur, 0xFF106060);
            break;
        default:
            Menu_DrawLabelList(460, gOfferMenuTexts, 2, menu->topCur, 0xFF106060);
            break;
        }
        break;
    case OFFER_STEP_DELIVERIES:
        OfferMenu_DrawDelivery(x, y);
        break;
    case OFFER_STEP_QUESTS:
        OfferMenu_DrawQuest(x, y);
        break;
    }
}

/* The story deliveries: what each wants, its points, title and text. */
Delivery gStoryDeliveries[18] = {
#include "lobby/007E7010.inc"
};
