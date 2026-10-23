/*
 * The kitchen (TOWN_MODE_KITCHEN): the meal the house's cook serves at a
 * table. When the player sits at one of the house's tables, the cook
 * (NPC_HOUSE_COOK) walks to it and greets the diners. The player picks two
 * different ingredients from those held (gKitchenIngredients' groups), one
 * of each for every diner; or, once the village's meal points reach
 * MEAL_POINTS_SET_COURSE, the cook serves a set course of its own. The
 * meal plays as the cooking lesson (NPC_SCENE_COOKING), then gives its
 * effect: the scene's four meal bonuses for the next quest (stamina,
 * health, attack, defense) with a notice for each, a taste line, and the
 * meal points, records and awards.
 *
 * Online, the other diners at the table follow the host's meal through the
 * kitchen's town message (enum KitchenMsg) in the kitchen guest's menu
 * (TOWN_MODE_KITCHEN_GUEST), which calls this file's Kitchen_EatSetCourse,
 * Kitchen_EatDish and Kitchen_DrawConfirm. The host then has 60 seconds to
 * choose.
 *
 * The menu's state is a KitchenMenu in gMenuWork; the ingredient list's
 * functions (IngredientList_BuildPage, IngredientList_DrawWindow) build
 * and draw its list. The data: the ingredient groups, each group's two
 * stats (gMealStatKinds), the bonus of each stat by level (gMealBonus) and
 * each dish's scale (gDishScales).
 */
#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "item.h"
#include "scene.h"
#include "save.h"
#include "player.h"
#include "lobby_menu.h"
#include "lobby_npc.h"
#include "area.h"
#include "award.h"
#include "menu_task.h"
#include "em.h"
#include "pad.h"
#include "lobby_labels.h"
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "camera.h"
#include "lobby_net.h"
#include "text.h"
#include "kitchen.h"
#include "font.h"
#include "game_util.h"
#include "ingredient_list.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "lobby_world.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "offer_menu.h"
#include "rand.h"
#include "service_npc_states.h"
#include "talk.h"
#include "town_players.h"
#include "libc.h"

extern u8 gMenuWork[];
extern s32 gTownMode;
extern s32 gTownMenuOpen;

extern char gKitchenTextFmt[];

void NpcScene_Start(s8 kind, s32 seatMask, s32 level);
extern u8 gMenuInputLock;
extern u8 gTakeSpotId;

/*
 * Opens the kitchen at the table the player has just sat at; nothing when
 * the cook isn't there. The player sits down, the cook's talk starts, the
 * table's spot is taken from the seat (gTakeSpotId), the set course is
 * marked when it is ready, the diners are listed and, online, their
 * guests' menus opened, and the 60-second wait starts.
 */
void Kitchen_Open(void)
{
    KitchenMenu *menu = (KitchenMenu *)gMenuWork;
    struct Em *cook = Npc_Find(NPC_HOUSE_COOK);

    if (cook == NULL) {
        return;
    }
    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_KITCHEN;
    memset(menu, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Kitchen_Draw, 0, 0, 0);
    gMenuInputLock = 6; /* frames */
    menu->cook = cook;
    menu->player = &gPlayers[gScene.localPlayer];
    menu->diners = 1;
    Actor_SetState(menu->player, 0, 30); /* the player's town state: sits at the table */
    Talk_Start(menu->cook, 1);
    menu->rand = Rand_Next(1);
    if (gScene.area == AREA_HUB_0x57) {
        menu->spot = KITCHEN_SPOT_AWAY;
    } else if (gTakeSpotId <= 1) {
        menu->spot = KITCHEN_SPOT_SEATS_0_1;
    } else if (gTakeSpotId <= 5) {
        menu->spot = KITCHEN_SPOT_SEATS_2_5;
    } else {
        menu->spot = KITCHEN_SPOT_SEATS_6_15;
    }
    if (Kitchen_SetCourseReady() == 1) {
        menu->setCourse = 1;
    }
    Kitchen_ListDiners(menu);
    Kitchen_Broadcast(menu, KITCHEN_MSG_OPEN);
    menu->timer = 1800; /* frames: 60 s */
}

extern char gMyUserId[];

/*
 * Sends the kitchen's town message of the given kind (enum KitchenMsg),
 * online and with other diners only: KITCHEN_MSG_OPEN and
 * KITCHEN_MSG_CHOSEN (with the meal) to each diner but the player,
 * KITCHEN_MSG_ROUTE and KITCHEN_MSG_DONE (with the spot) to the room.
 */
void Kitchen_Broadcast(KitchenMenu *menu, u8 kind)
{
    KitchenMeal meal;
    u8 msg[80];
    s32 i;

    if (Game_IsOnline() == 0) {
        return;
    }
    if (menu->diners == 1) {
        return;
    }
    msg[0] = kind;
    switch (kind) {
    case KITCHEN_MSG_OPEN:
        for (i = 0; i < menu->diners; i++) {
            if (memcmp(menu->dinerIds[i], gMyUserId, LOBBY_USER_ID_LEN) != 0) {
                TownMsg_SendKitchen(msg, menu->dinerIds[i]);
            }
        }
        break;
    case KITCHEN_MSG_CHOSEN:
        meal.season = menu->season;
        meal.firstPick = menu->firstPick;
        meal.secondPick = menu->secondPick;
        meal.rand = menu->rand;
        meal.seatMask = menu->seatMask;
        meal.spot = menu->spot;
        memcpy(&msg[1], &meal, sizeof(KitchenMeal));
        for (i = 0; i < menu->diners; i++) {
            if (memcmp(menu->dinerIds[i], gMyUserId, LOBBY_USER_ID_LEN) != 0) {
                TownMsg_SendKitchen(msg, menu->dinerIds[i]);
            }
        }
        break;
    case KITCHEN_MSG_ROUTE:
    case KITCHEN_MSG_DONE:
        msg[1] = menu->spot;
        TownMsg_SendKitchen(msg, NULL);
        break;
    }
}

/* Closes the kitchen: back to TOWN_MODE_FREE, the player's target
 * cleared, the draw removed, the camera's request and focus ended. */
void Kitchen_Close(void)
{
    u8 localPlayer;

    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    localPlayer = gScene.localPlayer;
    gPlayers[localPlayer].target = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamReq_Cancel();
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
}

/*
 * 1 when the cook serves its set course: no entry of SaveData.mealVisits has
 * its flag set, and the village's meal points have reached
 * MEAL_POINTS_SET_COURSE; else 0. The townspeople's lists then mark the
 * cook for the house's owner.
 */
s32 Kitchen_SetCourseReady(void)
{
    s32 i;

    for (i = 0; i < 16; i++) {
        if (gSave.mealVisits[i].pending != 0) {
            return 0;
        }
    }
    return gSave.mealPoints >= MEAL_POINTS_SET_COURSE;
}

/*
 * Lists the diners at the player's table. Offline the player alone, with
 * the bit of its seat. Online, the users seated at the table's seats (0-1,
 * 2-5 or 6-15, by the player's seat; gLobby->houseSeatIds holds a user id per
 * seat): their ids, their seats' bits and their number. The player's seat
 * is the first halfword (the id) of the town spot it sits at.
 */
void Kitchen_ListDiners(KitchenMenu *menu)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    s32 i;

    if (Game_IsOnline() == 0) {
        menu->diners = 1;
        menu->seatMask = 1 << *(u16 *)pl->spot;
        return;
    }
    menu->diners = 0;
    menu->seatMask = 0;
    memset(menu->dinerIds, 0, sizeof(menu->dinerIds));
    if (*(u16 *)pl->spot <= 1) {
        for (i = 0; i < 2; i++) {
            if ((s8)gLobby->houseSeatIds[i * LOBBY_USER_ID_LEN] != 0) {
                memcpy(menu->dinerIds[menu->diners], &gLobby->houseSeatIds[i * LOBBY_USER_ID_LEN], LOBBY_USER_ID_LEN);
                menu->seatMask |= 1 << i;
                menu->diners++;
            }
        }
    } else if (*(u16 *)pl->spot <= 5) {
        for (i = 2; i < 6; i++) {
            if ((s8)gLobby->houseSeatIds[i * LOBBY_USER_ID_LEN] != 0) {
                memcpy(menu->dinerIds[menu->diners], &gLobby->houseSeatIds[i * LOBBY_USER_ID_LEN], LOBBY_USER_ID_LEN);
                menu->seatMask |= 1 << i;
                menu->diners++;
            }
        }
    } else {
        for (i = 6; i < 16; i++) {
            if ((s8)gLobby->houseSeatIds[i * LOBBY_USER_ID_LEN] != 0) {
                memcpy(menu->dinerIds[menu->diners], &gLobby->houseSeatIds[i * LOBBY_USER_ID_LEN], LOBBY_USER_ID_LEN);
                menu->seatMask |= 1 << i;
                menu->diners++;
            }
        }
    }
}

extern u16 gTalkLine;
extern char *gTalkLineText;
s32 Award_Grant(s32 a);
/*
 * The kitchen's frame (TOWN_MODE_KITCHEN), one KITCHEN_STEP_* at a time:
 * the player sits, the cook walks to the table and greets (a dish or the
 * set course), online the diners' line, the ingredient list slides in,
 * the two ingredients, the confirm window, the cooking lesson, the meal
 * and its effect, the cook's thanks. Online with others, the wait running
 * out before the lesson ends it with the cook's goodbye; so does holding
 * fewer than two ingredients.
 *
 * A dish takes one of each ingredient from every diner and gives
 * Kitchen_EatDish's effect; online with others the player may be given
 * money as well, and the village's meal points grow by (diners + 10) times
 * the dish's rank while below MEAL_POINTS_CAP. The set course clears the
 * meal points, gives Kitchen_EatSetCourse's effect, clears the cook's mark
 * and counts RECORD_SET_COURSES. Every meal adds the diners to
 * RECORD_MEALS.
 */
void Kitchen_Update(void)
{
    KitchenMenu *menu = (KitchenMenu *)gMenuWork;
    NpcWork *cookWork = (NpcWork *)&menu->cook->work;
    u16 pad = LobbyPad_Get(0);
    s16 rank;
    u16 roll;
    u32 points;

    Chat_SetWindowMode(1, 2);
    if (Game_IsOnline() == 1 && menu->diners > 1 && (menu->step < KITCHEN_STEP_COOKING || menu->step == KITCHEN_STEP_ASK_LEAVE)) {
        if (--menu->timer <= 0) {
            menu->step = KITCHEN_STEP_BYE;
            Talk_Say(COOK_TALK_BYE, 0);
        }
    }
    switch (menu->step) {
    case KITCHEN_STEP_SIT:
        if (Motion_ReachedFrame(menu->player, 0, 30.0f) != 0) {
            menu->step++;
            Npc_SetKitchenRoute(menu->cook, menu->spot);
            Kitchen_Broadcast(menu, KITCHEN_MSG_ROUTE);
        }
        break;
    case KITCHEN_STEP_COOK_WALK:
        if (menu->cook->state == 89) { /* the cook's state: at the table */
            menu->step++;
            if (menu->setCourse == 0) {
                Talk_Say(COOK_TALK_GREET, 0);
            } else {
                Talk_Say(COOK_TALK_SET_COURSE, 0);
            }
        }
        break;
    case KITCHEN_STEP_GREET:
        switch (Talk_Update()) {
        case MENU_DONE:
            IngredientList_BuildPage(menu, 0);
            if (menu->setCourse == 0 && menu->count < 2) {
                menu->step = KITCHEN_STEP_BYE;
                Talk_Say(COOK_TALK_TOO_FEW, 0);
            } else if (menu->diners > 1) {
                menu->step++;
                Talk_Say(COOK_TALK_PARTY, 0);
                gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), menu->diners);
            } else if (menu->setCourse == 1) {
                menu->step = KITCHEN_STEP_CONFIRM_IN;
                WinSlide_Step(&menu->confirmWin, -280, 60);
            } else {
                menu->step = KITCHEN_STEP_LIST_IN;
                WinSlide_Step(&menu->win, 640, 280);
            }
            break;
        case MENU_CANCEL:
            menu->step = KITCHEN_STEP_BYE;
            Talk_Say(COOK_TALK_BYE, 0);
            break;
        }
        break;
    case KITCHEN_STEP_PARTY:
        switch (Talk_Update()) {
        case MENU_DONE:
            if (menu->setCourse == 1) {
                menu->step = KITCHEN_STEP_CONFIRM_IN;
                WinSlide_Step(&menu->confirmWin, -280, 60);
            } else {
                menu->step++;
                WinSlide_Step(&menu->win, 640, 280);
            }
            break;
        case MENU_CANCEL:
            menu->step = KITCHEN_STEP_ASK_LEAVE;
            menu->backStep = KITCHEN_STEP_PARTY;
            Talk_Say(COOK_TALK_ASK_LEAVE, 1);
            break;
        }
        break;
    case KITCHEN_STEP_LIST_IN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case MENU_DONE:
            menu->step++;
            menu->firstPick = 0;
            break;
        }
        break;
    case KITCHEN_STEP_PICK_FIRST:
        switch (Kitchen_ChooseIngredient(menu, pad)) {
        case MENU_DONE:
            menu->step++;
            menu->secondPick = 0;
            break;
        case MENU_CANCEL:
            menu->step = KITCHEN_STEP_ASK_LEAVE;
            Talk_Say(COOK_TALK_ASK_LEAVE, 1);
            break;
        }
        break;
    case KITCHEN_STEP_PICK_SECOND:
        switch (Kitchen_ChooseIngredient(menu, pad)) {
        case MENU_DONE:
            menu->step++;
            WinSlide_Step(&menu->confirmWin, -280, 60);
            Menu_SetYesNo(1);
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            break;
        case MENU_CANCEL:
            menu->firstPick = 0;
            menu->step = KITCHEN_STEP_PICK_FIRST;
            break;
        }
        break;
    case KITCHEN_STEP_CONFIRM_IN:
        switch (WinSlide_Step(&menu->confirmWin, -280, 60)) {
        case MENU_DONE:
            menu->step++;
            menu->backStep = KITCHEN_STEP_PICK_SECOND;
            break;
        }
        break;
    case KITCHEN_STEP_CONFIRM:
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            menu->step++;
            menu->season = LobbyWorld_GetSeason();
            if (Game_IsOnline() == 1 || gSave.awayFromVillage == 0) {
                CamReq_Kitchen(menu->spot);
            } else {
                CamReq_Kitchen(KITCHEN_SPOT_AWAY);
            }
            if (menu->setCourse != 0) {
                /* the set course: 0 (40%), 1 (30%), 2 (20%) or 3 (10%) */
                roll = Rand_Next(1) % 100;
                menu->firstPick = 0;
                if (roll < 40) {
                    menu->secondPick = 0;
                } else if (roll < 70) {
                    menu->secondPick = 1;
                } else if (roll < 90) {
                    menu->secondPick = 2;
                } else {
                    menu->secondPick = 3;
                }
                NpcScene_Start(NPC_SCENE_COOKING, menu->seatMask, 4);
            } else {
                /* the lesson's level: the two ingredients' average rarity, less one */
                NpcScene_Start(NPC_SCENE_COOKING, menu->seatMask, (gItemData[menu->firstPick].rarity + gItemData[menu->secondPick].rarity + 2) / 2 - 1);
            }
            Kitchen_Broadcast(menu, KITCHEN_MSG_CHOSEN);
            break;
        case MENU_CANCEL:
            if (menu->setCourse == 1) {
                menu->step = KITCHEN_STEP_ASK_LEAVE;
                Talk_Say(COOK_TALK_ASK_LEAVE, 1);
            } else {
                menu->step = menu->backStep;
                menu->secondPick = 0;
                if (menu->backStep == KITCHEN_STEP_PARTY) {
                    Talk_Say(COOK_TALK_PARTY, 0);
                    gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), menu->diners);
                }
            }
            break;
        }
        break;
    case KITCHEN_STEP_COOKING:
        if (NpcScene_IsDone() == 1) {
            menu->step++;
            Actor_SetState(menu->player, 0, 41); /* the player's town state: eats */
        }
        break;
    case KITCHEN_STEP_EAT:
        if (menu->player->motionId[0] == 609) { /* the player's motion: seated at the table */
            menu->step++;
            if (menu->setCourse == 0) {
                Items_Take(menu->firstPick, menu->diners);
                Items_Take(menu->secondPick, menu->diners);
                rank = Kitchen_EatDish(menu->season, menu->firstPick, menu->secondPick, menu->rand);
                if (Game_IsOnline() == 1 && menu->diners > 1 && *(u16 *)(gLobbyWorkBuf + LOBBY_WORK_MEAL_GIFT) > 0 && (Rand_Next(1) & 1)) {
                    Save_AddMoney((*(u16 *)(gLobbyWorkBuf + LOBBY_WORK_MEAL_GIFT)));
                    Menu_PushNoticeOnce(LOBBY_NOTICE_MEAL_MONEY);
                }
                points = menu->diners + 10;
                points *= rank;
                if (gSave.mealPoints < MEAL_POINTS_CAP) {
                    gSave.mealPoints += points;
                }
            } else {
                gSave.mealPoints = 0;
                Kitchen_EatSetCourse(menu->secondPick, menu->rand);
                cookWork->offerMark = 0;
            }
            if (menu->setCourse != 0) {
                if (gSave.recordCounts[RECORD_SET_COURSES] < 5000) {
                    gSave.recordCounts[RECORD_SET_COURSES] += 1;
                    if (gSave.recordCounts[RECORD_SET_COURSES] >= 50) {
                        Award_Grant(AWARD_SET_COURSES);
                    }
                }
            }
            if (gSave.recordCounts[RECORD_MEALS] < RECORD_TALLY_MAX) {
                gSave.recordCounts[RECORD_MEALS] += menu->diners;
                if (gSave.recordCounts[RECORD_MEALS] >= 100) {
                    if (!(gSave.rateFlags & (1 << RATE_FLAG_MEALS))) {
                        RateFlag_SetAndNotify(RATE_FLAG_MEALS);
                    }
                    Award_Grant(AWARD_MEALS);
                }
            }
            Actor_SetState(menu->cook, 0, 85); /* the cook's state after the meal */
        }
        break;
    case KITCHEN_STEP_REACT:
        if (menu->player->state == 1) { /* the player's town state: idle */
            menu->step++;
            Talk_Say(COOK_TALK_THANKS, 0);
            Npc_AdvanceRoute(menu->cook);
            Kitchen_Broadcast(menu, KITCHEN_MSG_DONE);
        }
        break;
    case KITCHEN_STEP_THANKS:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->step = KITCHEN_STEP_CLOSE;
            break;
        }
        break;
    case KITCHEN_STEP_ASK_LEAVE:
        switch (Talk_Update()) {
        case MENU_DONE:
            menu->step = KITCHEN_STEP_BYE;
            Talk_Say(COOK_TALK_BYE, 0);
            break;
        case MENU_CANCEL:
            menu->step = KITCHEN_STEP_GREET;
            if (menu->setCourse == 0) {
                Talk_Say(COOK_TALK_GREET, 0);
            } else {
                Talk_Say(COOK_TALK_SET_COURSE, 0);
            }
            break;
        }
        break;
    case KITCHEN_STEP_BYE:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->step = KITCHEN_STEP_CLOSE;
            Actor_SetState(menu->player, 0, 21); /* the player's town state: stands up */
            Npc_AdvanceRoute(menu->cook);
            Kitchen_Broadcast(menu, KITCHEN_MSG_DONE);
            break;
        }
        break;
    case KITCHEN_STEP_CLOSE:
        Kitchen_Close();
        break;
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/kitchen", gKitchenTextFmt);

/* 1 when the item is an ingredient (in one of gKitchenIngredients'
 * groups), else 0. The Poogie's menu filters the pouch with it too. */
s32 Kitchen_IsIngredient(u16 item)
{
    s32 i;
    s32 j;
    KitchenIngredient *group;

    for (i = 0; gKitchenIngredients[i] != NULL; i++) {
        for (group = gKitchenIngredients[i], j = 0; group[j].item != 0; j++) {
            if (item == group[j].item) {
                return 1;
            }
        }
    }
    return 0;
}

/* The index of the ingredient's group in gKitchenIngredients, which picks
 * its two stats in gMealStatKinds; 0 when not found. */
static s32 Kitchen_IngredientGroup(u16 item)
{
    KitchenIngredient *group;
    s16 i;
    s32 j;

    for (i = 0; gKitchenIngredients[i] != NULL; i++) {
        for (group = gKitchenIngredients[i], j = 0; group[j].item != 0; j++) {
            if (item == group[j].item) {
                return i;
            }
        }
    }
    return 0;
}

/* The ingredient's record; the first group's first record when the item
 * isn't an ingredient. */
static KitchenIngredient *Kitchen_FindIngredient(u16 item)
{
    s32 i;
    s16 j;
    KitchenIngredient *group;
    KitchenIngredient *fallback;

    fallback = gKitchenIngredients[0];
    for (i = 0; gKitchenIngredients[i] != NULL; i++) {
        for (group = gKitchenIngredients[i], j = 0; group[j].item != 0; j++) {
            if (item == group[j].item) {
                return &group[j];
            }
        }
    }
    return fallback;
}

/* The level (0 to MEAL_LEVELS - 1) of a stat change by its size, either
 * sign: up to 5 is 0, then one more for each further 5, from 26 on 5. */
s32 Kitchen_EffectLevel(s32 change)
{
    change = abs(change);

    if (change < 6) {
        return 0;
    }
    if (change < 11) {
        return 1;
    }
    if (change < 16) {
        return 2;
    }
    if (change < 21) {
        return 3;
    }
    return change < 26 ? 4 : 5;
}

extern MealBonusTable gMealBonus;

/*
 * Gives the set course's effect: courses 1 and 3 raise stamina and health
 * (by 20 and 30), the others attack and defense (by 10, or 25 for course
 * 2). Each change sets its meal bonus in gScene from gMealBonus by its
 * level, with a notice going up ([0] of its texts) or down ([1]); the
 * player reacts and the taste line is posted (enum MealTaste, one of four
 * by the roll `rand`). The kitchen guest gives the same effect.
 */
void Kitchen_EatSetCourse(u16 course, u16 rand)
{
    s32 gained;
    s32 lost;
    s32 defense = 0;
    s32 gainSum;
    s16 level;
    s32 lossSum;
    s32 attack = 0;
    Player *pl = &gPlayers[gScene.localPlayer];
    s32 taste;
    s32 stamina = 0;
    s32 health = 0;

    switch (course) {
    default:
        attack = 10;
        defense = attack;
        break;
    case 1:
        stamina = 20;
        health = stamina;
        break;
    case 2:
        attack = 25;
        defense = attack;
        break;
    case 3:
        stamina = 30;
        health = stamina;
        break;
    }
    lossSum = 0;
    gainSum = 0;
    if (stamina > 0) {
        gainSum += stamina;
    } else {
        lossSum -= stamina;
    }
    if (health > 0) {
        gainSum += health;
    } else {
        lossSum -= health;
    }
    if (attack > 0) {
        gainSum += attack;
    } else {
        lossSum -= attack;
    }
    if (defense > 0) {
        gainSum += defense;
    } else {
        lossSum -= defense;
    }
    gained = 0;
    lost = 0;
    level = Kitchen_EffectLevel(stamina);
    if (stamina < 0) {
        gScene.mealStamina = -gMealBonus.bonus[MEAL_STAT_STAMINA][level];
    } else {
        gScene.mealStamina = gMealBonus.bonus[MEAL_STAT_STAMINA][level];
    }
    if (gScene.mealStamina > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_STAMINA_UP]);
        gained += gScene.mealStamina;
    } else if (gScene.mealStamina < 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_STAMINA_DOWN]);
        lost += gScene.mealStamina;
    }
    level = Kitchen_EffectLevel(health);
    if (health < 0) {
        gScene.mealHealth = -gMealBonus.bonus[MEAL_STAT_HEALTH][level];
    } else {
        gScene.mealHealth = gMealBonus.bonus[MEAL_STAT_HEALTH][level];
    }
    if (gScene.mealHealth > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_HEALTH_UP]);
        gained += gScene.mealHealth;
    } else if (gScene.mealHealth < 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_HEALTH_DOWN]);
        lost += gScene.mealHealth;
    }
    level = Kitchen_EffectLevel(attack);
    if (attack < 0) {
        gScene.mealAttack = -gMealBonus.bonus[MEAL_STAT_ATTACK][level];
    } else {
        gScene.mealAttack = gMealBonus.bonus[MEAL_STAT_ATTACK][level];
    }
    if (gScene.mealAttack > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_ATTACK_UP]);
        gained += gScene.mealAttack;
    } else if (gScene.mealAttack < 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_ATTACK_DOWN]);
        lost += gScene.mealAttack;
    }
    level = Kitchen_EffectLevel(defense);
    if (defense < 0) {
        gScene.mealDefense = -gMealBonus.bonus[MEAL_STAT_DEFENSE][level];
    } else {
        gScene.mealDefense = gMealBonus.bonus[MEAL_STAT_DEFENSE][level];
    }
    if (gScene.mealDefense > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_DEFENSE_UP]);
        gained += gScene.mealDefense;
    } else if (gScene.mealDefense < 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_DEFENSE_DOWN]);
        lost += gScene.mealDefense;
    }
    if (gained == 0 && lost == 0) {
        taste = MEAL_TASTE_NONE;
        Actor_SetState(pl, 0, 39); /* the player's town state: a plain reaction (no change, or both gains and losses) */
    } else if (lost == 0) {
        if (gainSum >= MEAL_STRONG_TASTE) {
            taste = MEAL_TASTE_GREAT;
        } else {
            taste = MEAL_TASTE_GOOD;
        }
        Actor_SetState(pl, 0, 24); /* the player's town state: a pleased reaction */
    } else if (gained == 0) {
        if (lossSum >= MEAL_STRONG_TASTE) {
            taste = MEAL_TASTE_AWFUL;
        } else {
            taste = MEAL_TASTE_BAD;
        }
        Actor_SetState(pl, 0, 42); /* the player's town state: a displeased reaction */
    } else {
        if (lossSum < gainSum) {
            taste = MEAL_TASTE_MIXED_GOOD;
        } else {
            taste = MEAL_TASTE_MIXED_BAD;
        }
        Actor_SetState(pl, 0, 39);
    }
    Notice_PushItem(gMealTasteTexts[taste][rand % 4]);
    gLobby->mealEaten = 1;
}

extern s8 gMealStatKinds[][2];
extern s8 gDishScales[];

/* The dish's rank, 1 to 3: one more for each ingredient of rarity 3 or
 * more. Written as an inline function: the game's code computes the rank
 * inside Kitchen_EatDish, where its 1 shares the register of the 1 stored
 * just before. */
static inline s16 Kitchen_DishRank(u16 first, u16 second)
{
    if (gItemData[first].rarity >= 3) {
        if (gItemData[second].rarity >= 3) {
            return 3;
        }
        return 2;
    }
    if (gItemData[second].rarity >= 3) {
        return 2;
    }
    return 1;
}

/*
 * Gives the effect of the dish of the two ingredients in the season
 * (SEASON_*): each ingredient adds its season's values to its group's two
 * stats (gMealStatKinds), and the sums are scaled by the dish's entry of
 * gDishScales (in tenths; a negative scale turns them around). Then as
 * Kitchen_EatSetCourse: the bonuses with their notices, the player's
 * reaction and the taste line. Returns the dish's rank. The kitchen guest
 * gives the same effect.
 *
 * Kitchen_IngredientGroup and Kitchen_FindIngredient are static and
 * defined above it: the game's code calls them as the file's own
 * functions.
 */
s16 Kitchen_EatDish(s16 season, u16 first, u16 second, u16 rand)
{
    s32 attack;
    s32 defense;
    s16 level;
    s32 gained;
    s32 lost;
    f32 scale;
    KitchenIngredient *firstRec;
    s32 health;
    s32 firstGroup;
    s32 gainSum;
    s32 lossSum;
    s32 taste;
    s32 secondGroup;
    s32 stamina;
    s32 attackFx;
    s32 defenseFx;
    Player *pl;
    s32 healthFx;
    s32 staminaFx;
    KitchenIngredient *secondRec;

    pl = &gPlayers[gScene.localPlayer];
    firstGroup = Kitchen_IngredientGroup(first);
    secondGroup = Kitchen_IngredientGroup(second);
    firstRec = Kitchen_FindIngredient(first);
    secondRec = Kitchen_FindIngredient(second);
    attack = 0;
    defense = 0;
    health = 0;
    stamina = 0;
    switch (gMealStatKinds[firstGroup][0]) {
    case MEAL_STAT_STAMINA:
        stamina += firstRec->stat[season][0];
        break;
    case MEAL_STAT_HEALTH:
        health += firstRec->stat[season][0];
        break;
    case MEAL_STAT_ATTACK:
        attack += firstRec->stat[season][0];
        break;
    case MEAL_STAT_DEFENSE:
        defense += firstRec->stat[season][0];
        break;
    }
    switch (gMealStatKinds[firstGroup][1]) {
    case MEAL_STAT_STAMINA:
        stamina += firstRec->stat[season][1];
        break;
    case MEAL_STAT_HEALTH:
        health += firstRec->stat[season][1];
        break;
    case MEAL_STAT_ATTACK:
        attack += firstRec->stat[season][1];
        break;
    case MEAL_STAT_DEFENSE:
        defense += firstRec->stat[season][1];
        break;
    }
    switch (gMealStatKinds[secondGroup][0]) {
    case MEAL_STAT_STAMINA:
        stamina += secondRec->stat[season][0];
        break;
    case MEAL_STAT_HEALTH:
        health += secondRec->stat[season][0];
        break;
    case MEAL_STAT_ATTACK:
        attack += secondRec->stat[season][0];
        break;
    case MEAL_STAT_DEFENSE:
        defense += secondRec->stat[season][0];
        break;
    }
    switch (gMealStatKinds[secondGroup][1]) {
    case MEAL_STAT_STAMINA:
        stamina += secondRec->stat[season][1];
        break;
    case MEAL_STAT_HEALTH:
        health += secondRec->stat[season][1];
        break;
    case MEAL_STAT_ATTACK:
        attack += secondRec->stat[season][1];
        break;
    case MEAL_STAT_DEFENSE:
        defense += secondRec->stat[season][1];
        break;
    }
    scale = gDishScales[(first + second) % MEAL_DISHES];
    scale *= 0.1f;
    lossSum = 0;
    staminaFx = stamina * scale;
    healthFx = health * scale;
    attackFx = attack * scale;
    defenseFx = defense * scale;
    gainSum = 0;
    if (staminaFx > 0) {
        gainSum += staminaFx;
    } else {
        lossSum -= staminaFx;
    }
    if (healthFx > 0) {
        gainSum += healthFx;
    } else {
        lossSum -= healthFx;
    }
    if (attackFx > 0) {
        gainSum += attackFx;
    } else {
        lossSum -= attackFx;
    }
    if (defenseFx > 0) {
        gainSum += defenseFx;
    } else {
        lossSum -= defenseFx;
    }
    gained = 0;
    lost = 0;
    level = Kitchen_EffectLevel(staminaFx);
    if (staminaFx < 0) {
        gScene.mealStamina = -gMealBonus.bonus[MEAL_STAT_STAMINA][level];
    } else {
        gScene.mealStamina = gMealBonus.bonus[MEAL_STAT_STAMINA][level];
    }
    if (gScene.mealStamina > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_STAMINA_UP]);
        gained += gScene.mealStamina;
    } else if (gScene.mealStamina < 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_STAMINA_DOWN]);
        lost += gScene.mealStamina;
    }
    level = Kitchen_EffectLevel(healthFx);
    if (healthFx < 0) {
        gScene.mealHealth = -gMealBonus.bonus[MEAL_STAT_HEALTH][level];
    } else {
        gScene.mealHealth = gMealBonus.bonus[MEAL_STAT_HEALTH][level];
    }
    if (gScene.mealHealth > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_HEALTH_UP]);
        gained += gScene.mealHealth;
    } else if (gScene.mealHealth < 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_HEALTH_DOWN]);
        lost += gScene.mealHealth;
    }
    level = Kitchen_EffectLevel(attackFx);
    if (attackFx < 0) {
        gScene.mealAttack = -gMealBonus.bonus[MEAL_STAT_ATTACK][level];
    } else {
        gScene.mealAttack = gMealBonus.bonus[MEAL_STAT_ATTACK][level];
    }
    if (gScene.mealAttack > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_ATTACK_UP]);
        gained += gScene.mealAttack;
    } else if (gScene.mealAttack < 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_ATTACK_DOWN]);
        lost += gScene.mealAttack;
    }
    level = Kitchen_EffectLevel(defenseFx);
    if (defenseFx < 0) {
        gScene.mealDefense = -gMealBonus.bonus[MEAL_STAT_DEFENSE][level];
    } else {
        gScene.mealDefense = gMealBonus.bonus[MEAL_STAT_DEFENSE][level];
    }
    if (gScene.mealDefense > 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_DEFENSE_UP]);
        gained += gScene.mealDefense;
    } else if (gScene.mealDefense < 0) {
        Notice_PushItem(gKitchenTexts[KITCHEN_TEXT_DEFENSE_DOWN]);
        lost += gScene.mealDefense;
    }
    if (gained == 0 && lost == 0) {
        taste = MEAL_TASTE_NONE;
        Actor_SetState(pl, 0, 39); /* the player's town state: a plain reaction (no change, or both gains and losses) */
    } else if (lost == 0) {
        if (gainSum >= MEAL_STRONG_TASTE) {
            taste = MEAL_TASTE_GREAT;
        } else {
            taste = MEAL_TASTE_GOOD;
        }
        Actor_SetState(pl, 0, 24); /* the player's town state: a pleased reaction */
    } else if (gained == 0) {
        if (lossSum >= MEAL_STRONG_TASTE) {
            taste = MEAL_TASTE_AWFUL;
        } else {
            taste = MEAL_TASTE_BAD;
        }
        Actor_SetState(pl, 0, 42); /* the player's town state: a displeased reaction */
    } else {
        if (lossSum < gainSum) {
            taste = MEAL_TASTE_MIXED_GOOD;
        } else {
            taste = MEAL_TASTE_MIXED_BAD;
        }
        Actor_SetState(pl, 0, 39);
    }
    Notice_PushItem(gMealTasteTexts[taste][rand % 4]);
    gLobby->mealEaten = 1;
    return Kitchen_DishRank(first, second);
}

/*
 * The list's input while an ingredient is chosen (KITCHEN_STEP_PICK_FIRST,
 * then KITCHEN_STEP_PICK_SECOND, whose pick must differ). Circle takes the
 * ingredient under the cursor when one is held for each diner (MENU_DONE);
 * cross returns MENU_CANCEL; square shows or hides the description;
 * left and right turn the page, up and down move the cursor. MENU_RUNNING
 * otherwise.
 */
s32 Kitchen_ChooseIngredient(KitchenMenu *menu, u16 pad)
{
    if (pad & PAD_CIRCLE) {
        if (Items_HaveEnough(menu->items[menu->cursor], menu->diners) != 0) {
            switch (menu->step) {
            case KITCHEN_STEP_PICK_FIRST:
                menu->firstPick = menu->items[menu->cursor];
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            case KITCHEN_STEP_PICK_SECOND:
                if (menu->firstPick != menu->items[menu->cursor]) {
                    menu->secondPick = menu->items[menu->cursor];
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                    return MENU_DONE;
                }
                LobbySe_Play(LOBBY_SE_REFUSE);
                break;
            }
        } else {
            LobbySe_Play(LOBBY_SE_REFUSE);
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & PAD_SQUARE) {
        if (menu->count != 0) {
            menu->showInfo ^= 1;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        }
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (menu->pages > 1) {
            menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
            IngredientList_BuildPage(menu, menu->page);
        }
    } else {
        menu->cursor = Menu_WrapUpDown(menu->cursor, menu->count);
    }
    return MENU_RUNNING;
}

extern char *gItemNames[];
void Win_Draw(s32, s32, s32, s32, s32, s32, s16);
/* Draws the first ingredient's window at (x, y): its label and, once
 * chosen, its name. */
void Kitchen_DrawFirstPick(KitchenMenu *menu, s32 x, s32 y)
{
    s16 top = y;
    s16 left;

    Font_SetSpacing(18, 18);
    Win_Draw(x, y, 200, 60, 0xBDB59F6C, 0xBB402010, 3);
    Font_SetColor(0x807F7F28);
    left = x;
    Font_SetPos(left + 10, top += 10);
    Text_PrintfColored(gKitchenTextFmt, gKitchenTexts[KITCHEN_TEXT_INGREDIENT_A]);
    if (menu->firstPick != 0) {
        Font_SetColor(0x807F7F7F);
        Font_SetPos(left + 30, top + 22);
        Text_PrintfColored(gKitchenTextFmt, gItemNames[menu->firstPick]);
    }
}

/* The same for the second ingredient. */
void Kitchen_DrawSecondPick(KitchenMenu *menu, s32 x, s32 y)
{
    s16 top = y;
    s16 left;

    Font_SetSpacing(18, 18);
    Win_Draw(x, y, 200, 60, 0xBDB59F6C, 0xBB402010, 3);
    Font_SetColor(0x807F7F28);
    left = x;
    Font_SetPos(left + 10, top += 10);
    Text_PrintfColored(gKitchenTextFmt, gKitchenTexts[KITCHEN_TEXT_INGREDIENT_B]);
    if (menu->secondPick != 0) {
        Font_SetColor(0x807F7F7F);
        Font_SetPos(left + 30, top + 22);
        Text_PrintfColored(gKitchenTextFmt, gItemNames[menu->secondPick]);
    }
}


/* Draws the set course's two lines, centred in the confirm window at
 * (x, y). */
void Kitchen_DrawSetCourse(s16 x, s16 y)
{
    char *line;

    Font_SetSpacing(24, 24);
    line = gKitchenTexts[KITCHEN_TEXT_COURSE_SPECIAL];
    Font_SetPos(Text_CenterX(line, x + 160, 24), y += 84);
    Text_PrintfColored(gKitchenTextFmt, line);
    line = gKitchenTexts[KITCHEN_TEXT_COURSE_SERVICE];
    Font_SetPos(Text_CenterX(line, x + 160, 24), y + 60);
    Text_PrintfColored(gKitchenTextFmt, line);
}

extern LobbyMenu *gLobbyMenu;

/*
 * Draws the confirm window at (x, y): the set course's lines when
 * firstPick is 0, else both ingredients and the dish's name
 * (gMealNameTexts); then the yes/no question below it, unless the kitchen
 * guest draws it. The dish's label is gKitchenTexts[KITCHEN_TEXT_DISH_LABEL].
 *
 * The second bar of each column adds an unsigned constant (10U, 266U).
 * The game's code widens x once but computes x + 10 and x + 266 again
 * for the second bar: two sums of the same type would be computed once
 * and shared, so the second sum of each pair is written unsigned.
 */
void Kitchen_DrawConfirm(s16 x, s16 y, u16 firstPick, u16 secondPick)
{
    s16 textX;
    s16 dish;
    s16 midY;
    s16 textY = y;

    Font_SetSpacing(20, 20);
    Icon_Draw(x, y, 320, 260, 0xFFD4BF9F, 0);
    midY = textY + 130;
    Icon_Draw(x + 10, midY - 90, 256, 32, 0x803E2502, 27);
    Icon_Draw(x + 10U, midY + 70, 256, 32, 0x803E2502, 27);
    Icon_Draw(x + 266, midY - 90, 46, 32, 0x803E2502, 28);
    Icon_Draw(x + 266U, midY + 70, 46, 32, 0x803E2502, 28);
    Font_SetColor(0x80101010);
    if (firstPick == 0) {
        Kitchen_DrawSetCourse(x, y);
        if (gTownMode != TOWN_MODE_KITCHEN_GUEST) {
            Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gKitchenTexts[KITCHEN_TEXT_CONFIRM_DISH]);
            Font_SetPos(418, 402);
            Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        }
    } else {
        textX = x + 36;
        Font_SetPos(textX, textY += 74);
        Text_PrintfColored("%s%s", gKitchenTexts[KITCHEN_TEXT_INGREDIENT_A], gItemNames[firstPick]);
        Font_SetPos(textX, textY += 48);
        Text_PrintfColored("%s%s", gKitchenTexts[KITCHEN_TEXT_INGREDIENT_B], gItemNames[secondPick]);
        Font_SetColor(0x80301010);
        Font_SetPos(textX, textY + 48);
        dish = (firstPick + secondPick) % MEAL_DISHES;
        Text_PrintfColored("%s%s", gKitchenTexts[KITCHEN_TEXT_DISH_LABEL], gMealNameTexts[dish % MEAL_DISHES]);
        if (gTownMode != TOWN_MODE_KITCHEN_GUEST) {
            Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gKitchenTexts[KITCHEN_TEXT_CONFIRM_DISH]);
            Font_SetPos(418, 402);
            Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        }
    }
}

/*
 * The kitchen's draw: the ingredient list with the windows of the
 * ingredients chosen so far and the prompt (for one diner or a party)
 * while choosing; the confirm window, with its title bar once it is in;
 * and online with others the seconds left to choose.
 */
void Kitchen_Draw(MenuTask *task)
{
    KitchenMenu *menu = (KitchenMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    switch (menu->step) {
    case KITCHEN_STEP_LIST_IN:
        IngredientList_DrawWindow(menu, menu->win.pos, 58);
        if (menu->diners > 1) {
            Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gKitchenTexts[KITCHEN_TEXT_PICK_TWO_PARTY]);
        } else {
            Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gKitchenTexts[KITCHEN_TEXT_PICK_TWO]);
        }
        break;
    case KITCHEN_STEP_PICK_FIRST:
        IngredientList_DrawWindow(menu, menu->win.pos, 58);
        Kitchen_DrawFirstPick(menu, (s16)(menu->win.pos - 200), 58);
        if (menu->diners > 1) {
            Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gKitchenTexts[KITCHEN_TEXT_PICK_TWO_PARTY]);
        } else {
            Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gKitchenTexts[KITCHEN_TEXT_PICK_TWO]);
        }
        break;
    case KITCHEN_STEP_PICK_SECOND:
        IngredientList_DrawWindow(menu, menu->win.pos, 58);
        Kitchen_DrawFirstPick(menu, (s16)(menu->win.pos - 200), 58);
        Kitchen_DrawSecondPick(menu, (s16)(menu->win.pos - 200), 118);
        if (menu->diners > 1) {
            Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gKitchenTexts[KITCHEN_TEXT_PICK_TWO_PARTY]);
        } else {
            Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gKitchenTexts[KITCHEN_TEXT_PICK_TWO]);
        }
        break;
    case KITCHEN_STEP_CONFIRM:
        Font_SetSpacing(22, 22);
        Menu_DrawLabelBar(290, 18, 160, 0xFFB0B000, gKitchenTexts[KITCHEN_TEXT_TODAYS_DISH], 2, 1);
    case KITCHEN_STEP_CONFIRM_IN:
        Kitchen_DrawConfirm(290, menu->confirmWin.pos, menu->firstPick, menu->secondPick);
        break;
    case KITCHEN_STEP_COOKING:
        break;
    }
    if (Game_IsOnline() == 1 && menu->diners > 1 && (menu->step < KITCHEN_STEP_COOKING || menu->step == KITCHEN_STEP_ASK_LEAVE)) {
        Win_Draw(490, 20, 120, 22, 0xBDB59F6C, 0x80202020, 6);
        Font_SetPos(490, 22);
        Font_SetSpacing(20, 20);
        Font_SetColor(0x807F7F28);
        Text_Printf(gTownLabels[TOWN_LABEL_TIME_LEFT], menu->timer / 30 + 1); /* 30 frames a second */
    }
}

/* The kitchen's data: the ingredient groups, each group's two stats, the
 * bonuses by stat and level, the dishes' scales. */
KitchenIngredient gKitchenIngredients_0[6] = {
#include "lobby/007E7960.inc"
};
KitchenIngredient gKitchenIngredients_1[6] = {
#include "lobby/007E7990.inc"
};
KitchenIngredient gKitchenIngredients_2[6] = {
#include "lobby/007E79C0.inc"
};
KitchenIngredient gKitchenIngredients_3[6] = {
#include "lobby/007E79F0.inc"
};
KitchenIngredient gKitchenIngredients_4[6] = {
#include "lobby/007E7A20.inc"
};
KitchenIngredient gKitchenIngredients_5[4] = {
#include "lobby/007E7A50.inc"
};
KitchenIngredient gKitchenIngredients_6[4] = {
#include "lobby/007E7A70.inc"
};
KitchenIngredient gKitchenIngredients_7[4] = {
#include "lobby/007E7A90.inc"
};
KitchenIngredient gKitchenIngredients_8[2] = {
#include "lobby/007E7AB0.inc"
};
KitchenIngredient gKitchenIngredients_9[2] = {
#include "lobby/007E7AC0.inc"
};
KitchenIngredient *gKitchenIngredients[12] = {
    gKitchenIngredients_0, gKitchenIngredients_1, gKitchenIngredients_2, gKitchenIngredients_3,
    gKitchenIngredients_4, gKitchenIngredients_5, gKitchenIngredients_6, gKitchenIngredients_7,
    gKitchenIngredients_8, gKitchenIngredients_9, NULL, NULL,
};
s8 gMealStatKinds[16][2] = {
#include "lobby/007E7B00.inc"
};
MealBonusTable gMealBonus = {
#include "lobby/007E7B20.inc"
};
s8 gDishScales[16] = {
#include "lobby/007E7B60.inc"
};
