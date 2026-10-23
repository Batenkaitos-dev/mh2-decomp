/*
 * The dojo's menu (TOWN_MODE_DOJO), opened at the instructor by
 * DojoMenu_Open. Offline the instructor first offers a choice: the drills,
 * the training, or the instructor's offers (OfferMenu_Open); online the
 * menu goes straight to the drills.
 *
 * The drills menu has three categories: the drills (quests of the dojo,
 * hunts or arena, open by the season, the time of day, the village's
 * progress and the drills cleared before; taken through the quest desk, and
 * online posted to a room first), the point exchange (items for the points
 * the drills earn, gSave.points, from a stock chosen by progress) and the
 * rules (pages by topic). The training menu has the basics' pages and three
 * lists of training quests.
 *
 * The menu's state is gMenuWork read as a DojoMenu (lobby_menu.h):
 * DojoMenu_Update runs it each frame, and DojoMenu_DrawPanels and
 * DojoMenu_Draw draw it. The Dojo_*Cleared checks say whether a set of
 * drills is all cleared; the townspeople's talk uses them too.
 */
#include "common.h"
#include "village_cond.h"
#include "chat_log.h"
#include "window.h"
#include "syssettings.h"
#include "item.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "menu_task.h"
#include "lobby_globals.h"
#include "pad.h"
#include "player.h"
#include "lobby_npc.h"
#include "em.h"
#include "scene.h"
#include "quest.h"
#include "save.h"
#include "lobby_text.h"
#include "area.h"
#include "lobby_labels.h"
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "camera.h"
#include "text.h"
#include "dojo_menu.h"
#include "draw_helpers.h"
#include "font.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_leave_menus.h"
#include "lobby_queries.h"
#include "lobby_town.h"
#include "lobby_util.h"
#include "lobby_world.h"
#include "menu_sheets.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "offer_menu.h"
#include "quest_desk.h"
#include "quest_desk_draw.h"
#include "quest_offers.h"
#include "quest_order_terms.h"
#include "rand.h"
#include "talk.h"
#include "libc.h"

extern u8 gMenuWork[];
void Menu_DrawLabelList(s16 x, s8 **b, s16 n, s16 cur, u32 color);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u16 gDojoTrainingQuests1[];
extern u16 gDojoTrainingQuests2[];
extern u16 gDojoTrainingQuests2Full[];
extern u16 gDojoTrainingQuests3[];
u8 Village_FacilityLevel(s32 k);
extern s16 gDojoRulesPagesOn[];
extern s16 gDojoRulesPagesOff[];
extern char *gDojoBasicsPages[];

struct Em;
extern s8 gQuestMinMembers;

/* The pages of the rules (online and offline) and of the basics, reached
 * through gDojoRulesTextsOn, gDojoRulesTextsOff and gDojoBasicsPages. */
#pragma rodata on
char gDojoRulesTextsOn_0[384] = {
#include "lobby/008261A0.inc"
};
char gDojoRulesTextsOn_1[368] = {
#include "lobby/00826320.inc"
};
char gDojoRulesTextsOn_2[368] = {
#include "lobby/00826490.inc"
};
char gDojoRulesTextsOn_3[384] = {
#include "lobby/00826600.inc"
};
char gDojoRulesTextsOn_4[400] = {
#include "lobby/00826780.inc"
};
char gDojoRulesTextsOn_5[400] = {
#include "lobby/00826910.inc"
};
char gDojoRulesTextsOn_6[416] = {
#include "lobby/00826AA0.inc"
};
char gDojoRulesTextsOn_7[432] = {
#include "lobby/00826C40.inc"
};
char gDojoRulesTextsOn_8[352] = {
#include "lobby/00826DF0.inc"
};
char gDojoRulesTextsOn_9[384] = {
#include "lobby/00826F50.inc"
};
char gDojoRulesTextsOn_10[368] = {
#include "lobby/008270D0.inc"
};
char gDojoRulesTextsOn_11[384] = {
#include "lobby/00827240.inc"
};
char gDojoRulesTextsOn_12[416] = {
#include "lobby/008273C0.inc"
};
char gDojoRulesTextsOn_13[336] = {
#include "lobby/00827560.inc"
};
char gDojoRulesTextsOn_14[352] = {
#include "lobby/008276B0.inc"
};
char gDojoRulesTextsOn_15[384] = {
#include "lobby/00827810.inc"
};
char gDojoRulesTextsOn_16[464] = {
#include "lobby/00827990.inc"
};
char gDojoRulesTextsOn_17[400] = {
#include "lobby/00827B60.inc"
};
char gDojoRulesTextsOn_18[400] = {
#include "lobby/00827CF0.inc"
};
char gDojoRulesTextsOn_19[384] = {
#include "lobby/00827E80.inc"
};
char gDojoRulesTextsOn_20[368] = {
#include "lobby/00828000.inc"
};
char gDojoRulesTextsOn_21[368] = {
#include "lobby/00828170.inc"
};
char gDojoRulesTextsOn_22[400] = {
#include "lobby/008282E0.inc"
};
char gDojoRulesTextsOn_23[384] = {
#include "lobby/00828470.inc"
};
char gDojoRulesTextsOn_24[384] = {
#include "lobby/008285F0.inc"
};
char gDojoRulesTextsOn_25[400] = {
#include "lobby/00828770.inc"
};
char gDojoRulesTextsOn_26[304] = {
#include "lobby/00828900.inc"
};
char gDojoRulesTextsOn_27[416] = {
#include "lobby/00828A30.inc"
};
char gDojoRulesTextsOn_28[416] = {
#include "lobby/00828BD0.inc"
};
char gDojoRulesTextsOn_29[400] = {
#include "lobby/00828D70.inc"
};
char gDojoRulesTextsOff_0[384] = {
#include "lobby/00828F00.inc"
};
char gDojoRulesTextsOff_1[400] = {
#include "lobby/00829080.inc"
};
char gDojoRulesTextsOff_2[416] = {
#include "lobby/00829210.inc"
};
char gDojoRulesTextsOff_3[400] = {
#include "lobby/008293B0.inc"
};
char gDojoRulesTextsOff_4[416] = {
#include "lobby/00829540.inc"
};
char gDojoRulesTextsOff_5[416] = {
#include "lobby/008296E0.inc"
};
char gDojoRulesTextsOff_6[416] = {
#include "lobby/00829880.inc"
};
char gDojoRulesTextsOff_7[448] = {
#include "lobby/00829A20.inc"
};
char gDojoRulesTextsOff_8[400] = {
#include "lobby/00829BE0.inc"
};
char gDojoRulesTextsOff_9[416] = {
#include "lobby/00829D70.inc"
};
char gDojoRulesTextsOff_10[368] = {
#include "lobby/00829F10.inc"
};
char gDojoRulesTextsOff_11[384] = {
#include "lobby/0082A080.inc"
};
char gDojoRulesTextsOff_12[416] = {
#include "lobby/0082A200.inc"
};
char gDojoRulesTextsOff_13[416] = {
#include "lobby/0082A3A0.inc"
};
char gDojoRulesTextsOff_14[416] = {
#include "lobby/0082A540.inc"
};
char gDojoRulesTextsOff_15[432] = {
#include "lobby/0082A6E0.inc"
};
char gDojoRulesTextsOff_16[384] = {
#include "lobby/0082A890.inc"
};
char gDojoRulesTextsOff_17[352] = {
#include "lobby/0082AA10.inc"
};
char gDojoRulesTextsOff_18[352] = {
#include "lobby/0082AB70.inc"
};
char gDojoRulesTextsOff_19[384] = {
#include "lobby/0082ACD0.inc"
};
char gDojoRulesTextsOff_20[384] = {
#include "lobby/0082AE50.inc"
};
char gDojoRulesTextsOff_21[416] = {
#include "lobby/0082AFD0.inc"
};
char gDojoRulesTextsOff_22[384] = {
#include "lobby/0082B170.inc"
};
char gDojoRulesTextsOff_23[400] = {
#include "lobby/0082B2F0.inc"
};
char gDojoRulesTextsOff_24[320] = {
#include "lobby/0082B480.inc"
};
char gDojoRulesTextsOff_25[416] = {
#include "lobby/0082B5C0.inc"
};
char gDojoRulesTextsOff_26[416] = {
#include "lobby/0082B760.inc"
};
char gDojoRulesTextsOff_27[400] = {
#include "lobby/0082B900.inc"
};
char gDojoBasicsPages_0[304] = {
#include "lobby/0082BA90.inc"
};
char gDojoBasicsPages_1[304] = {
#include "lobby/0082BBC0.inc"
};
char gDojoBasicsPages_2[336] = {
#include "lobby/0082BCF0.inc"
};
char gDojoBasicsPages_3[336] = {
#include "lobby/0082BE40.inc"
};
char gDojoBasicsPages_4[304] = {
#include "lobby/0082BF90.inc"
};
char gDojoBasicsPages_5[288] = {
#include "lobby/0082C0C0.inc"
};
char gDojoBasicsPages_6[352] = {
#include "lobby/0082C1E0.inc"
};
char gDojoBasicsPages_7[320] = {
#include "lobby/0082C340.inc"
};
char gDojoBasicsPages_8[304] = {
#include "lobby/0082C480.inc"
};
char gDojoBasicsPages_9[336] = {
#include "lobby/0082C5B0.inc"
};
#pragma rodata off

#pragma optimization_level 4
/* Opens the dojo's menu at the instructor (TOWN_MODE_DOJO): clears gMenuWork,
 * sets the draw callbacks, has the instructor greet the hunter and notes the
 * season and the time of day the drills are matched against. Online the menu
 * goes on to the drills; offline it builds the first choice's lines: the drills
 * once Cond_Check(10) holds, the training, and the instructor's offers when
 * OfferMenu_BuildList finds any. */
void DojoMenu_Open(struct Em *instructor)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;

    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_DOJO;
    memset(dojo, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(DojoMenu_DrawPanels, DojoMenu_Draw, 0, 0);
    Talk_Say(0, 0);     /* the instructor's greeting */
    dojo->season = LobbyWorld_GetSeason();
    dojo->isPm = LobbyTime_IsPm();
    dojo->npc = instructor;
    if (Game_IsOnline() == 1) {
        dojo->mode = DOJO_MODE_DRILLS;
        dojo->cat = 0;
        dojo->choices[0] = DOJO_CHOICE_DRILLS;
        WinSlide_Step(&dojo->win, 640, 260);
        CamFocus_SetAside(1);
        LobbySe_Play(LOBBY_SE_WINDOW_IN);
    } else {
        dojo->choiceCount = 0;
        if (Cond_Check(10) == 1) {
            dojo->choices[dojo->choiceCount] = DOJO_CHOICE_DRILLS;
            dojo->choiceCount++;
        }
        dojo->choices[dojo->choiceCount] = DOJO_CHOICE_TRAINING;
        dojo->choiceCount++;
        if (OfferMenu_BuildList(instructor, 0) != 0) {
            dojo->choices[dojo->choiceCount] = DOJO_CHOICE_OFFERS;
            dojo->choiceCount++;
        }
    }
}

#pragma optimization_level reset

/* Closes the dojo menu: back to TOWN_MODE_FREE, the camera's focus ended, the
 * cancel sound, and the hunter takes the pad again. */
void DojoMenu_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_SetAside(0);
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
    gFieldControl = 1;
}

/* Always 1. After the instructor's talk, the dojo menu goes on to its choices
 * only when it is 1; else the instructor says goodbye. */
s32 Dojo_IsOpen(void)
{
    return 1;
}

/* Runs the dojo's menu each frame, by its mode: after the greeting, offline
 * the first choice and online the drills; the drills or the training menu;
 * then the instructor's last words and DojoMenu_Close. A drill taken ends
 * with the instructor's line 10 when the room needs no least members, else
 * line 11; leaving says line 1. */
void DojoMenu_Update(void)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (dojo->mode) {
    case DOJO_MODE_GREET:
        switch (Talk_Update()) {
        case MENU_DONE:
            /* Dojo_IsOpen is always 1: the training branch below never runs. */
            if (Dojo_IsOpen() == 1) {
                if (Game_IsOnline() == 0) {
                    dojo->mode++;
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                    break;
                }
                dojo->mode = DOJO_MODE_DRILLS;
                dojo->cat = 0;
                WinSlide_Step(&dojo->win, 640, 260);
                Talk_Say(2, 0);
                CamFocus_SetAside(1);
                break;
            }
            dojo->mode = DOJO_MODE_TRAINING;
            DojoMenu_StartTraining(dojo);
            break;
        }
        break;
    case DOJO_MODE_CHOICE:
        switch (DojoMenu_ChoiceInput(pad)) {
        case MENU_DONE:
            switch (dojo->choices[dojo->choiceCursor]) {
            case DOJO_CHOICE_DRILLS:
                dojo->mode++;
                dojo->cat = 0;
                WinSlide_Step(&dojo->win, 640, 260);
                CamFocus_SetAside(1);
                LobbySe_Play(LOBBY_SE_WINDOW_IN);
                break;
            case DOJO_CHOICE_TRAINING:
                dojo->mode = DOJO_MODE_TRAINING;
                DojoMenu_StartTraining(dojo);
                LobbySe_Play(LOBBY_SE_WINDOW_IN);
                break;
            case DOJO_CHOICE_OFFERS:
                OfferMenu_Open(dojo->npc);
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
                break;
            }
            break;
        case MENU_CANCEL:
            dojo->mode = DOJO_MODE_BYE;
            Talk_Say(1, 0);
            break;
        }
        break;
    case DOJO_MODE_DRILLS:
        switch (DojoMenu_DrillsInput(pad)) {
        case MENU_DONE:
            dojo->mode = DOJO_MODE_BYE;
            if (gQuestMinMembers == 0) {
                Talk_Say(10, 0);
            } else {
                Talk_Say(11, 0);
            }
            break;
        case MENU_CANCEL:
            if (Game_IsOnline() == 0) {
                dojo->mode = DOJO_MODE_CHOICE;
                break;
            }
            dojo->mode = DOJO_MODE_BYE;
            Talk_Say(1, 0);
            break;
        }
        break;
    case DOJO_MODE_TRAINING:
        switch (DojoMenu_TrainingInput(pad)) {
        case MENU_DONE:
            dojo->mode = DOJO_MODE_BYE;
            Talk_Say(1, 0);
            break;
        case MENU_CANCEL:
            if (Dojo_IsOpen() == 1) {
                dojo->mode = DOJO_MODE_CHOICE;
                break;
            }
            dojo->mode = DOJO_MODE_BYE;
            Talk_Say(1, 0);
            break;
        }
        break;
    case DOJO_MODE_BYE:
        switch (Talk_Update()) {
        case MENU_DONE:
            DojoMenu_Close();
            break;
        }
        break;
    }
}

/* Opens the training menu: the cursor on the first category, the window
 * slides in and the camera moves aside. The fourth category shows once
 * Cond_Check(3) holds. */
void DojoMenu_StartTraining(DojoMenu *dojo)
{
    dojo->cat = 0;
    WinSlide_Step(&dojo->win, 640, 260);
    CamFocus_SetAside(1);
    if (Cond_Check(3) != 0) {
        dojo->cats = 4;
    } else {
        dojo->cats = 3;
    }
}

/* The training menu, by its step: the categories (the basics' pages or a
 * list of training quests), the window's slides, the quest sheet and the
 * basics. Returns MENU_DONE once a quest is taken, MENU_CANCEL when the
 * menu is left, else MENU_RUNNING. */
s32 DojoMenu_TrainingInput(u16 pad)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;

    switch (dojo->step) {
    case TRAINING_STEP_SLIDE_IN:
        switch (WinSlide_Step(&dojo->win, 640, 260)) {
        case MENU_DONE:
            dojo->step++;
            break;
        }
        break;
    case TRAINING_STEP_CATEGORY:
        switch (DojoMenu_TrainingCategoryInput(dojo, pad)) {
        case MENU_DONE:
            switch (dojo->cat) {
            case TRAINING_CAT_BASICS:
                dojo->step = TRAINING_STEP_SLIDE_OUT;
                dojo->next = TRAINING_STEP_BASICS;
                dojo->page = 0;
                break;
            default:
                dojo->step = TRAINING_STEP_SLIDE_OUT;
                dojo->next = TRAINING_STEP_QUESTS;
                DojoMenu_LoadTrainingQuests(dojo);
                break;
            }
            break;
        case MENU_CANCEL:
            dojo->step = TRAINING_STEP_SLIDE_IN;
            return MENU_CANCEL;
        }
        break;
    case TRAINING_STEP_TALK:    /* no step leads here */
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            dojo->step = TRAINING_STEP_SLIDE_OUT;
            dojo->next = TRAINING_STEP_BASICS;
            break;
        }
        break;
    case TRAINING_STEP_QUESTS:
        switch (DojoMenu_QuestInput(pad)) {
        case MENU_DONE:
            dojo->step = TRAINING_STEP_SLIDE_IN;
            return MENU_DONE;
        case MENU_CANCEL:
            dojo->step = TRAINING_STEP_SLIDE_BACK;
            dojo->next = TRAINING_STEP_CATEGORY;
            WinSlide_Step(&dojo->win, 640, 260);
            break;
        }
        break;
    case TRAINING_STEP_BASICS:
        switch (DojoMenu_BasicsInput(dojo, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            dojo->step = TRAINING_STEP_SLIDE_BACK;
            dojo->next = TRAINING_STEP_CATEGORY;
            break;
        }
        break;
    case TRAINING_STEP_SLIDE_OUT:
        switch (WinSlide_Step(&dojo->win, 260, 640)) {
        case MENU_DONE:
            dojo->step = dojo->next;
            break;
        }
        break;
    case TRAINING_STEP_SLIDE_BACK:
        switch (WinSlide_Step(&dojo->win, 640, 260)) {
        case MENU_DONE:
            dojo->step = dojo->next;
            break;
        }
        break;
    }
    return MENU_RUNNING;
}

/* Lists the chosen training category's quests (a list of ids ended by 0;
 * category 2's longer list once Cond_Check(3) holds), with the cursor and
 * the sheet on their first page. */
void DojoMenu_LoadTrainingQuests(DojoMenu *dojo)
{
    u16 *ids;
    u16 count;

    switch (dojo->cat) {
    case TRAINING_CAT_QUESTS_1:
        ids = gDojoTrainingQuests1;
        break;
    case TRAINING_CAT_QUESTS_2:
        if (Cond_Check(3) != 0) {
            ids = gDojoTrainingQuests2Full;
        } else {
            ids = gDojoTrainingQuests2;
        }
        break;
    case TRAINING_CAT_QUESTS_3:
        ids = gDojoTrainingQuests3;
        break;
    }
    memset(dojo->list, 0, sizeof(dojo->list));
    dojo->page = 0;
    dojo->pages = QUEST_SHEET_PAGES;
    dojo->questCursor = 0;
    count = 0;
    while (*ids != 0) {
        dojo->list[count] = *ids;
        count++;
        ids++;
    }
    dojo->questCount = count;
}

/* The basics' pages: up and down turn the page, cross leaves (MENU_CANCEL).
 * Circle does nothing. */
s32 DojoMenu_BasicsInput(DojoMenu *dojo, u16 pad)
{
    if (!(pad & PAD_CIRCLE)) {
        if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            return MENU_CANCEL;
        }
        dojo->page = Menu_WrapUpDownSe(dojo->page, DOJO_BASICS_PAGES, LOBBY_SE_WINDOW_SLIDE);
    }
    return MENU_RUNNING;
}

/* The training menu's categories: the cursor moves over them; circle
 * returns MENU_DONE, cross MENU_CANCEL. */
s32 DojoMenu_TrainingCategoryInput(DojoMenu *dojo, u16 pad)
{
    if (pad & PAD_CIRCLE) {
        LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    dojo->cat = Menu_WrapUpDown(dojo->cat, dojo->cats);
    return MENU_RUNNING;
}

/* The offline first choice: the cursor moves over its lines; circle returns
 * MENU_DONE (the caller plays the sound), cross MENU_CANCEL. */
s32 DojoMenu_ChoiceInput(u16 pad)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;

    if (pad & PAD_CIRCLE) {
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    dojo->choiceCursor = Menu_WrapUpDown(dojo->choiceCursor, dojo->choiceCount);
    return MENU_RUNNING;
}

/* The drills menu, by its step: the categories (the drills, the point
 * exchange, the rules), the drills' kind (hunts or arena), the instructor's
 * words, the window's slides, the drills' quest sheet, the exchange and the
 * rules. Online it opens at once; offline it is the first choice's drills.
 * Returns MENU_DONE once a drill is taken, MENU_CANCEL when the menu is
 * left, else MENU_RUNNING. */
s32 DojoMenu_DrillsInput(u16 pad)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;

    switch (dojo->step) {
    case DRILLS_STEP_CATEGORY:
        switch (DojoMenu_DrillsCategoryInput(pad)) {
        case MENU_DONE:
            switch (dojo->cat) {
            case DRILLS_CAT_QUESTS:
                dojo->step++;
                dojo->drillKind = DRILL_KIND_HUNT;
                break;
            case DRILLS_CAT_RULES:
                dojo->step = DRILLS_STEP_SLIDE_OUT;
                dojo->next = DRILLS_STEP_RULES;
                dojo->page = 0;
                if (Game_IsOnline() == 1) {
                    if (Cond_Check(COND_HR_GATE_2_HR_31) == 1) {
                        dojo->pages = DOJO_RULES_TOPICS_ALL;
                    } else {
                        dojo->pages = DOJO_RULES_TOPICS_ALL - 1;
                    }
                    dojo->rulesPages = gDojoRulesPagesOn[dojo->page];
                } else {
                    if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_TAVERN) == 2) {
                        dojo->pages = DOJO_RULES_TOPICS_ALL;
                    } else {
                        dojo->pages = DOJO_RULES_TOPICS_ALL - 1;
                    }
                    dojo->rulesPages = gDojoRulesPagesOff[dojo->page];
                }
                break;
            case DRILLS_CAT_EXCHANGE:
                dojo->step = DRILLS_STEP_SLIDE_OUT;
                dojo->next = DRILLS_STEP_SHOP;
                dojo->showInfo = 0;
                dojo->page = 0;
                PointShop_LoadPage(0);
                break;
            }
            break;
        case MENU_CANCEL:
            dojo->step = DRILLS_STEP_CATEGORY;
            CamFocus_SetAside(0);
            return MENU_CANCEL;
        }
        break;
    case DRILLS_STEP_KIND:
        switch (DojoMenu_DrillKindInput(pad)) {
        case MENU_DONE:
            switch (dojo->drillKind) {
            case DRILL_KIND_HUNT:
                DojoMenu_ListHuntDrills();
                break;
            case DRILL_KIND_ARENA:
                DojoMenu_ListArenaDrills();
                break;
            }
            if (dojo->questCount != 0) {
                dojo->step++;
                dojo->next = DRILLS_STEP_QUESTS;
                dojo->questPicked = 0;
                dojo->questCursor = 0;
                dojo->page = 0;
                dojo->pages = QUEST_SHEET_PAGES;
                Talk_Say(2, 0);
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
            break;
        case MENU_CANCEL:
            dojo->step = DRILLS_STEP_CATEGORY;
            dojo->state = DRILLS_CATS_INPUT;
            break;
        }
        break;
    case DRILLS_STEP_TALK:
        switch (Talk_Update()) {
        case MENU_DONE:
            dojo->step++;
            WinSlide_Step(&dojo->win, 260, 640);
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            break;
        }
        break;
    case DRILLS_STEP_SLIDE_OUT:
        switch (WinSlide_Step(&dojo->win, 260, 640)) {
        case MENU_DONE:
            dojo->step = dojo->next;
            break;
        }
        break;
    case DRILLS_STEP_QUESTS:
        switch (DojoMenu_QuestInput(pad)) {
        case MENU_DONE:
            dojo->step = DRILLS_STEP_CATEGORY;
            return MENU_DONE;
        case MENU_CANCEL:
            dojo->step = DRILLS_STEP_BACK_FROM_QUESTS;
            dojo->next = DRILLS_STEP_KIND;
            WinSlide_Step(&dojo->win, 640, 260);
            break;
        }
        break;
    case DRILLS_STEP_BACK_FROM_SHOP:
    case DRILLS_STEP_BACK_FROM_RULES:
    case DRILLS_STEP_BACK_FROM_QUESTS:
        switch (WinSlide_Step(&dojo->win, 640, 260)) {
        case MENU_DONE:
            dojo->step = dojo->next;
            if (dojo->next == DRILLS_STEP_CATEGORY) {
                dojo->state = DRILLS_CATS_INPUT;
            }
            break;
        }
        break;
    case DRILLS_STEP_SHOP:
        switch (PointShop_Input(pad)) {
        case MENU_DONE:
            dojo->step = DRILLS_STEP_CATEGORY;
            return MENU_DONE;
        case MENU_CANCEL:
            dojo->step = DRILLS_STEP_BACK_FROM_SHOP;
            dojo->next = DRILLS_STEP_CATEGORY;
            break;
        }
        break;
    case DRILLS_STEP_RULES:
        switch (DojoMenu_RulesInput(pad)) {
        case MENU_DONE:
            dojo->step = DRILLS_STEP_CATEGORY;
            return MENU_DONE;
        case MENU_CANCEL:
            dojo->step = DRILLS_STEP_BACK_FROM_RULES;
            dojo->next = DRILLS_STEP_CATEGORY;
            break;
        }
        break;
    }
    return MENU_RUNNING;
}

/* The rules' pages: up and down change the topic and load its page count,
 * left and right turn the topic's pages, cross leaves (MENU_CANCEL). */
s32 DojoMenu_RulesInput(u16 pad)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    s16 oldTopic;

    if (!(pad & PAD_CIRCLE)) {
        if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            return MENU_CANCEL;
        }
        if (pad & (PAD_UP | PAD_DOWN)) {
            oldTopic = dojo->page;
            dojo->page = Menu_WrapUpDownSe(oldTopic, dojo->pages, LOBBY_SE_WINDOW_SLIDE);
            if (oldTopic != dojo->page) {
                dojo->rulesPage = 0;
                if (Game_IsOnline() == 1) {
                    dojo->rulesPages = gDojoRulesPagesOn[dojo->page];
                } else {
                    dojo->rulesPages = gDojoRulesPagesOff[dojo->page];
                }
            }
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            dojo->rulesPage = Menu_WrapLeftRight(dojo->rulesPage, dojo->rulesPages);
        }
    }
    return MENU_RUNNING;
}

/* 1 when the offline drills 101 to 109 are all cleared. */
s32 Dojo_OfflineDrillsCleared(void)
{
    s32 id;

    for (id = 101;;) {
        if (Save_IsQuestCleared(id) == 0) {
            return 0;
        }
        id++;
        if (id > 109) {
            return 1;
        }
    }
}

/* 1 when the offline chain 201 to 205 (each opens after the one before) is
 * all cleared. */
s32 Dojo_OfflineChainCleared(void)
{
    s32 id;

    for (id = 201;;) {
        if (Save_IsQuestCleared(id) == 0) {
            return 0;
        }
        id++;
        if (id > 205) {
            return 1;
        }
    }
}

/* 1 when the offline drills and the offline chain are all cleared: the
 * point exchange then sells its POINT_SHOP_OFFLINE_CLEARED stock. */
s32 Dojo_OfflineAllCleared(void)
{
    if (Dojo_OfflineDrillsCleared() == 0) {
        return 0;
    }
    if (Dojo_OfflineChainCleared() != 0) {
        return 1;
    }
    return 0;
}

/* 1 when the online drills 10001 to 10009 are all cleared. */
s32 Dojo_OnlineDrillsCleared(void)
{
    s32 id;

    for (id = 10001;;) {
        if (Save_IsQuestCleared(id) == 0) {
            return 0;
        }
        id++;
        if (id > 10009) {
            return 1;
        }
    }
}

/* 1 when the online chain 10101 to 10105 is all cleared. */
s32 Dojo_OnlineChainCleared(void)
{
    s32 id;

    for (id = 10101;;) {
        if (Save_IsQuestCleared(id) == 0) {
            return 0;
        }
        id++;
        if (id > 10105) {
            return 1;
        }
    }
}

extern PointShopLine *gPointShopStocks[];
/* Loads page PAGE of the point exchange: picks the stock by the game's mode
 * and progress (POINT_SHOP_*), copies the page's lines to the menu and
 * counts the pages; the cursor goes to the first line. */
void PointShop_LoadPage(s16 page)
{
    s16 last;
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    s16 index = 0;
    s16 shown = 0;
    s16 first = page * POINT_SHOP_ROWS;
    PointShopLine *line;

    last = first + POINT_SHOP_ROWS - 1;

    if (Game_IsOnline() == 0) {
        if (Dojo_OfflineAllCleared() == 1) {
            dojo->shopStock = line = gPointShopStocks[POINT_SHOP_OFFLINE_CLEARED];
        } else if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_TAVERN) < 2) {
            dojo->shopStock = line = gPointShopStocks[POINT_SHOP_OFFLINE];
        } else {
            dojo->shopStock = line = gPointShopStocks[POINT_SHOP_OFFLINE_LV2];
        }
    } else if (Dojo_OnlineChainCleared() == 1) {
        dojo->shopStock = line = gPointShopStocks[POINT_SHOP_ONLINE_CLEARED];
    } else if (Cond_Check(COND_HR_GATE_2_HR_31) == 0) {
        dojo->shopStock = line = gPointShopStocks[POINT_SHOP_ONLINE];
    } else {
        dojo->shopStock = line = gPointShopStocks[POINT_SHOP_ONLINE_HR31];
    }
    for (; line->item != 0; line++) {
        if (index >= first && index <= last) {
            dojo->shopLines[shown] = *line;
            shown++;
        }
        index++;
    }
    dojo->pages = Menu_CountPages(index, POINT_SHOP_ROWS);
    dojo->shopCount = shown;
    dojo->shopCursor = 0;
}

/* 1 when the points held pay for COUNT of the page's line ROW and the item
 * box and the pouch have room for them. */
s32 PointShop_CanBuy(s16 row, s16 count)
{
    PointShopLine *entry = &((DojoMenu *)gMenuWork)->shopLines[row];
    s32 room;
    s16 space;

    if (gSave.points < entry->price * count) {
        return 0;
    }
    room = ItemBox_Room(entry->item);
    space = SavePouch_Space(entry->item);
    if (space == -1) {
        space = 0;
    }
    if (room + space < count) {
        return 0;
    }
    return 1;
}

/* The point exchange, by its step: the list (circle takes a line the hunter
 * can pay for, square shows or hides the item's description, left and right
 * turn the page), how many (up and down by one, left back to one, right the
 * most the hunter can pay for and hold), then the price's yes or no. On yes
 * the points are paid and the items given; a notice says when some went to
 * the item box. Cross on the list returns MENU_CANCEL. */
s32 PointShop_Input(u16 pad)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    s32 qty;
    s32 oldQty;
    PointShopLine *entry;

    switch (dojo->state) {
    case DOJO_SHOP_LIST:
        if (pad & PAD_CIRCLE) {
            if (PointShop_CanBuy(dojo->shopCursor, 1) == 1) {
                dojo->state++;
                dojo->shopQty = 1;
                Menu_SetYesNo(1);
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            return MENU_CANCEL;
        } else if (pad & PAD_SQUARE) {
            dojo->showInfo ^= 1;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (dojo->pages > 1) {
                dojo->page = Menu_WrapLeftRight(dojo->page, dojo->pages);
                PointShop_LoadPage(dojo->page);
            }
        } else {
            dojo->shopCursor = Menu_WrapUpDown(dojo->shopCursor, dojo->shopCount);
        }
        break;
    case DOJO_SHOP_COUNT:
        if (pad & PAD_CIRCLE) {
            dojo->state++;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            dojo->state = DOJO_SHOP_LIST;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & PAD_SQUARE) {
            dojo->showInfo ^= 1;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        } else if (pad & PAD_UP) {
            if (PointShop_CanBuy(dojo->shopCursor, dojo->shopQty + 1) == 1 && dojo->shopQty <= POINT_SHOP_MAX_QTY - 1) {
                dojo->shopQty++;
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_DOWN) {
            if (dojo->shopQty > 1) {
                dojo->shopQty--;
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_LEFT) {
            if (dojo->shopQty > 1) {
                dojo->shopQty = 1;
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_RIGHT) {
            oldQty = dojo->shopQty;
            for (qty = oldQty; qty < POINT_SHOP_MAX_QTY; qty++) {
                if (PointShop_CanBuy(dojo->shopCursor, qty) == 0) {
                    dojo->shopQty = qty - 1;
                    break;
                }
            }
            if (qty == POINT_SHOP_MAX_QTY) {
                dojo->shopQty = POINT_SHOP_MAX_QTY;
            }
            if (oldQty != dojo->shopQty) {
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        }
        break;
    case DOJO_SHOP_CONFIRM:
        if (pad & PAD_SQUARE) {
            dojo->showInfo ^= 1;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        } else {
            switch (Menu_YesNoInput()) {
            case MENU_DONE:
                dojo->state = DOJO_SHOP_LIST;
                entry = &dojo->shopLines[dojo->shopCursor];
                gSave.points -= entry->price * dojo->shopQty;
                if (Items_Give(entry->item, dojo->shopQty) != 0) {
                    Menu_PushNoticeOnce(15);   /* some of them went to the item box */
                }
                dojo->shopQty = 1;
                break;
            case MENU_CANCEL:
                dojo->state--;
                break;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

extern LobbyMenu *gLobbyMenu;
extern s16 gQuestStampRand;
void Dojo_SetBestTime(s32 a, s32 value);

/* The quest sheet of the drills or of the training, by its step. Circle
 * picks a quest: the instructor refuses it while a quest is taken, the
 * order terms' message shows when they aren't met, else the instructor
 * speaks and the quest is stamped (online after the posting). Left and right
 * turn the sheet's pages; select with start asks to clear the quest's best
 * score and time. Returns MENU_DONE once QuestDesk_Accept has taken the
 * quest, MENU_CANCEL when the sheet is left. */
s32 DojoMenu_QuestInput(u16 pad)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    QuestInfo *quest;

    switch (dojo->state) {
    case DOJO_QUEST_LIST:
        if (pad & PAD_CIRCLE) {
            quest = Quest_GetInfoAny(dojo->list[dojo->questCursor]);
            if (gLobby->questTaken != 0) {
                dojo->state = DOJO_QUEST_TAKEN;
                Talk_Say(15, 0);
            } else if (Quest_CheckOrderTerms(quest->terms, 0) == 0) {
                dojo->state = DOJO_QUEST_TERMS;
                LobbyText_Open(LOBBY_TEXT_ORDER_TERMS);
                LobbyDialog_Grow();
                gLobbyMenu->redraw = 1;
            } else {
                dojo->state++;
                Talk_Say(5, 0);
            }
            dojo->questPicked = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            return MENU_CANCEL;
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            dojo->page = Menu_WrapLeftRight(dojo->page, dojo->pages);
        } else if ((gPadState[0].held & PAD_SELECT && pad & PAD_START) || (pad & PAD_SELECT && gPadState[0].held & PAD_START)) {
            dojo->state = DOJO_QUEST_RESET;
            LobbyText_Open(149);   /* the question to clear the best score and time */
            Menu_SetYesNo(1);
        } else {
            dojo->questCursor = Menu_WrapUpDownSe(dojo->questCursor, dojo->questCount, LOBBY_SE_WINDOW_SLIDE);
        }
        break;
    case DOJO_QUEST_TALK:
        switch (Talk_Update()) {
        case MENU_DONE:
            if (Game_IsOnline() == 0) {
                dojo->state = DOJO_QUEST_STAMP;
                dojo->stamped = 1;
                gQuestStampRand = Rand_Next(1);
                Win_Shake(&dojo->shake);
            } else {
                dojo->state++;
                QuestRoom_ResetSettings();
                LobbyText_Open(LOBBY_TEXT_SENDING);
            }
            break;
        case MENU_CANCEL:
            dojo->state = DOJO_QUEST_LIST;
            dojo->questPicked = 0;
            break;
        }
        break;
    case DOJO_QUEST_POST:
        switch (DojoMenu_PostOnline(pad)) {
        case MENU_DONE:
            dojo->state++;
            dojo->stamped = 1;
            gQuestStampRand = Rand_Next(1);
            Win_Shake(&dojo->shake);
            break;
        }
        break;
    case DOJO_QUEST_STAMP:
        gFieldControl = 0;
        switch (Win_Shake(&dojo->shake)) {
        case 0:
            gLobby->questPaid = 1;
            dojo->questAccepted = 1;
            QuestDesk_Accept(dojo->list[dojo->questCursor], 0, 1);
            gFieldControl = 1;
            return MENU_DONE;
        }
        break;
    case DOJO_QUEST_TAKEN:
        switch (Talk_Update()) {
        case MENU_DONE:
            dojo->state = DOJO_QUEST_LIST;
            dojo->questPicked = 0;
            break;
        }
        break;
    case DOJO_QUEST_TERMS:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            dojo->state = DOJO_QUEST_LIST;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_CANCEL;
        }
        break;
    case DOJO_QUEST_RESET:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            dojo->state = DOJO_QUEST_LIST;
            Dojo_SetBestTime(dojo->list[dojo->questCursor], 0);
            Dojo_SetBestScore(dojo->list[dojo->questCursor], 0);
            break;
        case MENU_CANCEL:
            dojo->state = DOJO_QUEST_LIST;
            break;
        }
        break;
    }
    return MENU_RUNNING;
}

extern u32 gQuestPostArg;
extern u16 gLobbyQuestId;

/* Posts the picked drill online, by its step: fetch the quest list, enter
 * the free quest level, fetch the records, the room's settings, send them,
 * post the quest and accept it. Leaving the settings leaves the level
 * (MENU_CANCEL); a server error shows until circle closes the menu. Returns
 * MENU_DONE once accepted. */
s32 DojoMenu_PostOnline(u16 pad)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    u8 *paid;

    switch (dojo->postStep) {
    case DOJO_POST_FETCH_LIST:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_FetchList()) {
        case LOBBY_STEP_DONE:
            dojo->postStep++;
            break;
        case LOBBY_STEP_FAILED:
            LobbyDialog_OpenWait(gLobby->errText);
            dojo->postStep = DOJO_POST_ERROR;
            gSysSettings.fieldControl = 0;
            break;
        }
        break;
    case DOJO_POST_ENTER:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_EnterFree(1)) {
        case LOBBY_STEP_DONE:
            dojo->postStep++;
            break;
        case LOBBY_STEP_FAILED:
            dojo->postStep = DOJO_POST_ERROR;
            gSysSettings.fieldControl = 0;
            break;
        }
        break;
    case DOJO_POST_FETCH_RECORDS:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_FetchRecords()) {
        case LOBBY_STEP_DONE:
            dojo->postStep++;
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            break;
        case LOBBY_STEP_FAILED:
            dojo->postStep = DOJO_POST_ERROR;
            gSysSettings.fieldControl = 0;
            break;
        }
        break;
    case DOJO_POST_SETTINGS:
        switch (DojoMenu_SettingsInput(pad)) {
        case MENU_DONE:
            dojo->postStep++;
            LobbyText_Open(LOBBY_TEXT_SENDING);
            dojo->req.quest = dojo->list[dojo->questCursor];
            dojo->req.money = 0;
            dojo->req.desk = QUEST_POST_DESK_DOJO;
            dojo->req.area = gScene.area;
            break;
        case MENU_CANCEL:
            dojo->postStep = DOJO_POST_LEAVE;
            LobbyText_Open(LOBBY_TEXT_ROOM_LEAVING);
            break;
        }
        break;
    case DOJO_POST_SEND_SETTINGS:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_SendSettings()) {
        case LOBBY_STEP_DONE:
            dojo->postStep++;
            break;
        case LOBBY_STEP_FAILED:
            break;
        }
        gSysSettings.fieldControl = 0;
        break;
    case DOJO_POST_SEND:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_SendQuestPost(&dojo->req)) {
        case LOBBY_STEP_DONE:
            dojo->postStep++;
            gQuestPostArg = (gQuestPostArg & 0xFFFF0000) | gLobbyQuestId;
            break;
        case LOBBY_STEP_FAILED:
            dojo->postStep = DOJO_POST_ERROR;
            gSysSettings.fieldControl = 0;
            break;
        }
        gSysSettings.fieldControl = 0;
        break;
    case DOJO_POST_ACCEPT:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_SendQuestAccept(gQuestPostArg)) {
        case LOBBY_STEP_DONE:
            dojo->postStep = DOJO_POST_FETCH_LIST;
            gLobby->questPaid = 1;
            QuestDesk_Accept(dojo->list[dojo->questCursor], 0, 1);
            /* clears the next quest's versus fields, teams to timeLimit */
            memset(gNextQuest.teams, 0, 92);
            gSysSettings.fieldControl = 0;
            return MENU_DONE;
        case LOBBY_STEP_FAILED:
            LobbyDialog_OpenWait(gLobby->errText);
            dojo->postStep = DOJO_POST_ERROR;
            gSysSettings.fieldControl = 0;
            break;
        }
        gSysSettings.fieldControl = 0;
        break;
    case DOJO_POST_LEAVE:
        gLobbyMenu->redraw = 1;
        if (LobbyStep_Level2Leave() == 1) {    /* the level is left */
            paid = &gLobby->questPaid;
            if (*paid == 1) {
                *paid = 0;
            }
            dojo->postStep = DOJO_POST_FETCH_LIST;
            gSysSettings.fieldControl = 1;
            return MENU_CANCEL;
        }
        break;
    case DOJO_POST_ERROR:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            dojo->postStep = DOJO_POST_FETCH_LIST;
            DojoMenu_Close();
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
    return MENU_RUNNING;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/dojo_menu", gDojoTextFmt);

extern LobbyRecs gLobbyRecs;

/* The posting's room settings: up and down move over the lines (skipping
 * the password while it is off), left and right change the least members or
 * turn the password on or off, circle on the password opens the keyboard
 * and on the last line decides (MENU_DONE; a notice when the password is on
 * but empty). */
s32 DojoMenu_SettingsInput(u16 pad)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    s16 oldCursor;

    switch (dojo->settingsStep) {
    case DOJO_SETTINGS_LIST:
        if (pad & PAD_CIRCLE) {
            if (dojo->settingsCursor == ROOM_SETTING_DECIDE) {
                dojo->settingsStep = DOJO_SETTINGS_LIST;
                if (gLobbyRecs.usePass != 0 && gLobbyRecs.pass[0] == 0) {
                    Menu_PushNoticeOnce(LOBBY_NOTICE_PASSWORD_EMPTY);
                    break;
                }
                gQuestMinMembers = gLobbyRecs.rec[0].cur;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                return MENU_DONE;
            }
            if (dojo->settingsCursor == ROOM_SETTING_PASSWORD) {
                dojo->settingsStep++;
                gFieldControl = 0;
                LobbySe_Play(LOBBY_SE_CONFIRM);
                LobbyMenu_Init();
                LobbyMenu_KbdEntry(gLobbyRecs.pass);
            }
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (dojo->settingsCursor == ROOM_SETTING_PASS_ON) {
                gLobbyRecs.usePass ^= 1;
                if (gLobbyRecs.usePass == 0) {
                    memset(gLobbyRecs.pass, 0, sizeof(gLobbyRecs.pass));
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            } else if (dojo->settingsCursor == ROOM_SETTING_MEMBERS) {
                gLobbyRecs.rec[0].cur = Menu_WrapLeftRight(gLobbyRecs.rec[0].cur, ROOM_MIN_MEMBERS_CHOICES);
            }
        } else {
            oldCursor = dojo->settingsCursor;
            dojo->settingsCursor = Menu_WrapUpDown(oldCursor, ROOM_SETTINGS);
            if ((s8)oldCursor != dojo->settingsCursor && dojo->settingsCursor == ROOM_SETTING_PASSWORD && gLobbyRecs.usePass == 0) {
                dojo->settingsCursor += dojo->settingsCursor - (s8)oldCursor;
            }
        }
        break;
    case DOJO_SETTINGS_PASSWORD:
        if (LobbyMenu_KbdEntry(gLobbyRecs.pass) == 1) {
            dojo->settingsStep = DOJO_SETTINGS_LIST;
            LobbyMenu_Init();
            gFieldControl = 1;
        }
        break;
    }
    return MENU_RUNNING;
}

/* 1 when quest ID is a drill open now: a drill (QUEST_FLAG_DRILL) for the
 * season and the time of day the menu opened in, and open by progress. The
 * offline drills 101-105 need the facility at level 1 and 106-109 at
 * level 2; the offline chain 201-205 needs level 2 and each the one before
 * cleared. Online, 10001-10004 are always open, 10005-10009 need
 * COND_HR_GATE_2_HR_31, and the chain 10101-10105 needs
 * COND_HR_GATE_0_HR_11 and each the one before. The chains' cases fall through on purpose. */
s32 DojoMenu_IsDrillOpen(u16 id)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    QuestInfo *quest = Quest_GetDiscInfo(id);

    if (quest == NULL) {
        return 0;
    }
    if (!(quest->flags & QUEST_FLAG_DRILL)) {
        return 0;
    }
    if (Quest_MatchesSeasonTime(id, dojo->season, dojo->isPm) == 0) {
        return 0;
    }
    switch (id) {
    case 101:
    case 102:
    case 103:
    case 104:
    case 105:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_TAVERN) <= 0) {
            return 0;
        }
        break;
    case 106:
    case 107:
    case 108:
    case 109:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_TAVERN) <= 1) {
            return 0;
        }
        break;
    case 205:
        if (Save_IsQuestCleared(204) == 0) {
            return 0;
        }
    case 204:
        if (Save_IsQuestCleared(203) == 0) {
            return 0;
        }
    case 203:
        if (Save_IsQuestCleared(202) == 0) {
            return 0;
        }
    case 202:
        if (Save_IsQuestCleared(201) == 0) {
            return 0;
        }
    case 201:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_TAVERN) != 2) {
            return 0;
        }
        break;
    case 10001:
    case 10002:
    case 10003:
    case 10004:
        return 1;
    case 10005:
    case 10006:
    case 10007:
    case 10008:
    case 10009:
        if (Cond_Check(COND_HR_GATE_2_HR_31) == 0) {
            return 0;
        }
        break;
    case 10105:
        if (Save_IsQuestCleared(10104) == 0) {
            return 0;
        }
    case 10104:
        if (Save_IsQuestCleared(10103) == 0) {
            return 0;
        }
    case 10103:
        if (Save_IsQuestCleared(10102) == 0) {
            return 0;
        }
    case 10102:
        if (Save_IsQuestCleared(10101) == 0) {
            return 0;
        }
    case 10101:
        if (Cond_Check(COND_HR_GATE_0_HR_11) == 0) {
            return 0;
        }
        break;
    }
    return 1;
}

extern u16 *gQuestIdLists[];
/* Lists the open drills (up to DOJO_LIST_MAX) of every quest list whose map
 * isn't one of the arenas: offline the standard quests, online the others
 * (QuestId_IsStandard). */
void DojoMenu_ListHuntDrills(void)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    u16 *ids;
    u16 count;
    u16 i;
    s32 mapId;

    memset(dojo->list, 0, sizeof(dojo->list));
    count = 0;
    for (i = 0; i < 8; i++) {
        for (ids = gQuestIdLists[i]; *ids != 0; ids++) {
            mapId = Quest_GetDiscInfo(*ids)->map;
            if (mapId == MAP_GREAT_ARENA || mapId == MAP_ARENA) {
                continue;
            }
            if (DojoMenu_IsDrillOpen(*ids) == 1) {
                if (Game_IsOnline() == 0) {
                    if (QuestId_IsStandard(*ids) == 1) {
                        dojo->list[count++] = *ids;
                    }
                } else if (QuestId_IsStandard(*ids) == 0) {
                    dojo->list[count++] = *ids;
                }
            }
            if (count >= DOJO_LIST_MAX) {
                break;
            }
        }
        if (count >= DOJO_LIST_MAX) {
            break;
        }
    }
    dojo->questCount = count;
}

/* As DojoMenu_ListHuntDrills, for the drills on MAP_GREAT_ARENA and
 * MAP_ARENA. */
void DojoMenu_ListArenaDrills(void)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    u16 *ids;
    u16 count;
    u16 i;
    s32 mapId;

    memset(dojo->list, 0, sizeof(dojo->list));
    count = 0;
    for (i = 0; i < 8; i++) {
        for (ids = gQuestIdLists[i]; *ids != 0; ids++) {
            mapId = Quest_GetDiscInfo(*ids)->map;
            if (mapId != MAP_GREAT_ARENA && mapId != MAP_ARENA) {
                continue;
            }
            if (DojoMenu_IsDrillOpen(*ids) == 1) {
                if (Game_IsOnline() == 0) {
                    if (QuestId_IsStandard(*ids) == 1) {
                        dojo->list[count++] = *ids;
                    }
                } else if (QuestId_IsStandard(*ids) == 0) {
                    dojo->list[count++] = *ids;
                }
            }
            if (count >= DOJO_LIST_MAX) {
                break;
            }
        }
        if (count >= DOJO_LIST_MAX) {
            break;
        }
    }
    dojo->questCount = count;
}

/* The drills menu's categories: the window slides in first, then the cursor
 * moves over them; circle returns MENU_DONE, cross MENU_CANCEL. */
s32 DojoMenu_DrillsCategoryInput(u16 pad)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;

    switch (dojo->state) {
    case DRILLS_CATS_SLIDE_IN:
        switch (WinSlide_Step(&dojo->win, 640, 260)) {
        case MENU_DONE:
            dojo->state++;
            break;
        }
        break;
    case DRILLS_CATS_INPUT:
        if (pad & PAD_CIRCLE) {
            dojo->state = DRILLS_CATS_SLIDE_IN;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
        if (pad & PAD_CROSS) {
            dojo->state = DRILLS_CATS_SLIDE_IN;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        }
        dojo->cat = Menu_WrapUpDown(dojo->cat, DRILLS_CATS);
        return MENU_RUNNING;
    }
    return MENU_RUNNING;
}

/* The drills' kind, hunts or arena: circle returns MENU_DONE (the caller
 * plays the sound), cross MENU_CANCEL. */
s32 DojoMenu_DrillKindInput(u16 pad)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;

    if (pad & PAD_CIRCLE) {
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    dojo->drillKind = Menu_WrapUpDown(dojo->drillKind, DRILL_KINDS);
    return MENU_RUNNING;
}

/* Draws the drills menu's window at X, Y: the categories or the kinds, with
 * the cursor, for the menu's step (its next step while the window slides,
 * when BYNEXT is 1).
 * Each case moves x and y in place (`x = x + 170`): the game's code keeps
 * the parameters in their registers into the cases, which `x += 170` or
 * separate locals don't give. */
void DojoMenu_DrawDrillsMenu(s16 x, s16 y, s8 byNext)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    char *label;
    s32 i;
    s16 labelX;
    s8 screen;

    Menu_DrawDecoFrame(x, y, 340, 360, 5, 0);
    Font_SetColor(0x807F7F7F);
    if (byNext == 0) {
        screen = dojo->step;
    } else {
        screen = dojo->next;
    }
    switch (screen) {
    case DRILLS_STEP_CATEGORY:
        x = x + 170;
        y = y + 100;
        Font_SetSpacing(24, 24);
        for (i = 0; i < 3; i++) {
            label = gDojoMenuTexts[DOJO_TEXT_DRILL_MENU + i]; /* the categories' labels */
            labelX = Text_CenterX(label, x, 24);
            if (i == dojo->cat) {
                Menu_DrawArrowsRev(labelX, y, 24, label, 0xFFB57510);
                Text_DrawShadowRgb(labelX, y, 0xFF101010, 0xFF106060, label);
            } else {
                Font_SetColor(0xFF101010);
                Font_SetPos(labelX, y);
                Text_PrintfColored(label);
            }
            y += 70;
        }
        break;
    case DRILLS_STEP_KIND:
    case DRILLS_STEP_TALK:
        x = x + 170;
        y = y + 110;
        Font_SetSpacing(26, 26);
        for (i = 0; i < 2; i++) {
            label = gDojoMenuTexts[DOJO_TEXT_DRILL_KINDS + i]; /* the kinds' labels */
            labelX = Text_CenterX(label, x, 26);
            if (i == dojo->drillKind) {
                Menu_DrawArrowsRev(labelX, y, 26, label, 0xFFB57510);
                Text_DrawShadowRgb(labelX, y, 0xFF101010, 0xFF106060, label);
            } else {
                Font_SetColor(0xFF101010);
                Font_SetPos(labelX, y);
                Text_PrintfColored(label);
            }
            y += 100;
        }
        break;
    }
}

u16 Map_Group(u16 id);
extern char gDojoTextFmt[];

/* Draws the third page of a drill's sheet: its title, client, map, the best
 * score and the best time (a dash until the drill is cleared), its details,
 * and the labels beside them. */
void DojoMenu_DrawDrillDetails(s16 x, s16 y, QuestInfo *quest)
{
    s16 top = y;
    s32 id = quest->id;
    char num[64];
    char wide[64];

    Font_SetPos(x, y += 20);
    Text_PrintfColored(gDojoTextFmt, Quest_GetText(id, QUEST_TEXT_TITLE));
    Font_SetPos(x + 126, y += 42);
    Text_PrintfColored(gDojoTextFmt, Quest_GetText(id, QUEST_TEXT_CLIENT));
    Font_SetPos(x + 126, y += 24);
    /* the map is read as a halfword, as the game's code loads it */
    Text_PrintfColored(gStageNames[Map_Group(*(u16 *)&quest->map)]);
    Font_SetPos(x + 126, y += 24);
    if (Save_IsQuestCleared(id) == 0 || Dojo_GetBestScore(id) == 0) {
        sprintf(wide, gDojoTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_NO_VALUE]);
    } else {
        sprintf(num, "%d%s", Dojo_GetBestScore(id), gLobbyWords[LOBBY_WORD_POINTS]);
        Sjis_Widen(num, wide);
    }
    Text_PrintfColored(gDojoTextFmt, wide);
    Font_SetPos(x + 126, y += 24);
    if (Save_IsQuestCleared(id) == 0 || Dojo_GetBestTime(id) / 30 == 0) {
        sprintf(wide, gDojoTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_NO_VALUE]);
    } else {
        sprintf(num, gDojoTextFmt, Time_FormatMinSecInHour(Dojo_GetBestTime(id) / 30), gLobbyWords[LOBBY_WORD_POINTS]);
        Sjis_Widen(num, wide);
    }
    Text_PrintfColored(gDojoTextFmt, wide);
    Font_SetPos(x, y + 42);
    Text_PrintfColored(gDojoTextFmt, Quest_GetText(id, QUEST_TEXT_DETAILS));
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    x = x - 10;
    y = top + 20;
    y += 42;
    Font_SetPos(x, y);
    Text_PrintfColored(gDojoTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_CLIENT]);
    Font_SetPos(x, y += 24);
    Text_PrintfColored(gDojoTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_DESTINATION]);
    Font_SetPos(x, y += 24);
    Text_PrintfColored(gDojoTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_HIGH_SCORE]);
    Font_SetPos(x, y += 24);
    Text_PrintfColored(gDojoTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_CLEAR_TIME]);
    Font_SetPos(x, y + 24);
    Text_PrintfColored(gDojoTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_CLIENT_DETAILS]);
}

/* Draws the posting's room settings sheet: the least members, the password
 * on or off, the password's box and the decide line, with the cursor. */
void DojoMenu_DrawSettings(s16 x, s16 y)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    s16 top;
    s16 decideX;
    s32 valueX;
    s32 passX;
    s32 labelY;
    s16 labelX;
    s32 left;

    Quest_GetDiscInfo(dojo->list[dojo->questCursor]);
    top = y;
    Font_SetSpacing(22, 22);
    /* the drill kinds' titles follow the exchange's first three texts */
    Menu_DrawLabelBar(280, 18, 180, 0xFFB0B000, gDojoMenuTexts[DOJO_TEXT_DRILL_KINDS + dojo->drillKind], 2, 1);
    Menu_DrawQuestSheet(x, y, 320, 360, NULL, 1, -1, -1);
    Font_SetSpacing(18, 18);
    Font_SetColor(QUEST_SHEET_INK);
    y += 40;
    left = x;
    valueX = left + 216;
    Font_SetPos(valueX, y += 42);
    Text_PrintfColored(gDojoTextFmt, gMemberCountTexts[gLobbyRecs.rec[0].cur]);
    if (dojo->settingsCursor == ROOM_SETTING_MEMBERS) {
        Menu_DrawArrows(valueX, y, 18, gMemberCountTexts[gLobbyRecs.rec[0].cur], 0xFF106060);
    }
    Font_SetPos(valueX, y += 42);
    Text_PrintfColored(gNoneSetTexts[gLobbyRecs.usePass]);
    if (dojo->settingsCursor == ROOM_SETTING_PASS_ON) {
        Menu_DrawArrows(valueX, y, 18, gNoneSetTexts[gLobbyRecs.usePass], 0xFF106060);
    }
    Font_SetPos(Text_CenterX(gLobbyRecs.pass, left + 160, 18), y += 42);
    Text_PrintfColored(gDojoTextFmt, gLobbyRecs.pass);
    if (dojo->settingsCursor == ROOM_SETTING_PASSWORD) {
        passX = left + 100;
        Menu_DrawArrowsRev(passX, y, 18, "             ", 0xFF106060);
        Gfx_DrawRectFrame(passX, (s16)y - 1, 120, 20, 0xFF106060);
        Gfx_DrawRectFrame(left + 99, (s16)y - 2, 122, 22, 0xFF106060);
    } else {
        Gfx_DrawRectFrame(left + 100, (s16)y - 1, 120, 20, 0xFF606060);
        Gfx_DrawRectFrame(left + 99, (s16)y - 2, 122, 22, 0xFF606060);
    }
    Font_SetColor(QUEST_SHEET_LABEL_INK);
    labelY = top + 40;
    y = labelY;
    labelX = left + 40;
    Font_SetPos(Text_CenterX(gQuestSheetTexts[QUEST_SHEET_TEXT_RECRUIT], 440, 18), labelY);
    Text_PrintfColored(gDojoTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_RECRUIT]);
    Font_SetPos(labelX, y += 42);
    Text_PrintfColored(gDojoTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_MEMBERS]);
    Font_SetPos(labelX, y += 42);
    Text_PrintfColored(gDojoTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_PASSWORD]);
    decideX = Text_CenterX(gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE], 440, 18);
    Font_SetPos(decideX, y += 120);
    Text_PrintfColored(gDojoTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE]);
    if (dojo->settingsCursor == ROOM_SETTING_DECIDE) {
        Menu_DrawArrowsRev(decideX, y, 18, gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE], 0xFF106060);
    }
}

extern s8 gMapGroupMapIds[][2];

/* Draws the quest sheet of the listed quests: a tab per quest (the
 * selected one marked), the sheet's page (the details are the drill's
 * own for a drill), the cleared mark, and the stamp once the quest is
 * taken. */
void DojoMenu_DrawQuestPage(s16 x, s16 y)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    QuestInfo *quest = Quest_GetDiscInfo(dojo->list[dojo->questCursor]);
    s16 tabs[DOJO_LIST_MAX];
    s16 i;

    Font_SetSpacing(22, 22);
    if (quest->flags & QUEST_FLAG_TRAINING) {
        Menu_DrawLabelBar(280, 18, 180, 0xFFB0B000, gDojoMenuTexts[DOJO_TEXT_TRAINING + dojo->cat], 2, 255);
    } else {
        /* the drill kinds' titles follow the exchange's first three texts */
        Menu_DrawLabelBar(280, 18, 180, 0xFFB0B000, gDojoMenuTexts[DOJO_TEXT_DRILL_KINDS + dojo->drillKind], 2, 255);
    }
    memset(tabs, 0, sizeof(tabs));
    for (i = 0; i < DOJO_LIST_MAX; i++) {
        if (dojo->list[i] == 0) {
            break;
        }
        tabs[i] = Map_Group(Quest_GetDiscInfo(dojo->list[i])->map);
        if (i == dojo->questCursor) {
            tabs[i] = -tabs[i];
        }
    }
    /* the map is read as a halfword, as the game's code loads it */
    Menu_DrawQuestSheet(x, y, 340, 360, tabs, dojo->questCursor, gMapGroupMapIds[Map_Group(*(u16 *)&quest->map)][0], -1);
    Font_SetColor(QUEST_SHEET_INK);
    if (dojo->questPicked == 0) {
        Menu_DrawPageNum(540, y + 14, dojo->page, QUEST_SHEET_PAGES, 1);
    }
    Font_SetSpacing(18, 18);
    switch (dojo->page) {
    case QUEST_SHEET_OVERVIEW:
        Quest_DrawOverview((s16)(x + 40) + 10, (s16)(y + 14) + 4, quest);
        break;
    case QUEST_SHEET_TERMS:
        Quest_DrawTerms((s16)(x + 40) + 10, (s16)(y + 14) + 4, quest);
        break;
    case QUEST_SHEET_DETAILS:
        if (quest->flags & QUEST_FLAG_TRAINING) {
            Quest_DrawDetails((s16)(x + 40) + 10, (s16)(y + 14) + 4, quest);
        } else {
            DojoMenu_DrawDrillDetails((s16)(x + 40) + 10, (s16)(y + 14) + 4, quest);
        }
        break;
    }
    if (Save_IsQuestCleared(quest->id)) {
        Quest_DrawClearMark(x + 40, y + 14);
    }
    if (dojo->stamped) {
        Menu_DrawStamp(x + 40, y + 14, 340, 360, (u16)gQuestStampRand, dojo->shake.timer);
    }
}

extern char *gItemNames[];

/* Draws the point exchange: the points held, the page, the lines (greyed
 * when the hunter can't pay for one), the cursor, the item's description,
 * and the step's window: the hint, how many, or the price's question. */
void PointShop_Draw(s16 x, s16 y)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    s16 rowY = y;
    s16 i;
    s16 nudge;
    char buf[16];
    char wide[32];

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(280, 18, 180, 0xFFB0B000, gDojoMenuTexts[DOJO_TEXT_POINT_EXCHANGE], 2, 1);
    Menu_DrawPoints(480, 16);
    Icon_Draw(x, y, 340, 360, 0xFFE0D8C8, 0);
    Font_SetColor(QUEST_SHEET_INK);
    Menu_DrawPageNum(510, rowY + 14, dojo->page, dojo->pages, 1);
    Font_SetSpacing(20, 20);
    rowY += 36;
    for (i = 0; i < dojo->shopCount; i++) {
        if (PointShop_CanBuy(i, 1) == 1) {
            Font_SetColor(QUEST_SHEET_INK);
        } else {
            Font_SetColor(0x80404040);
        }
        Font_SetPos(x + 46, rowY);
        Text_PrintfColored(gDojoTextFmt, gItemNames[dojo->shopLines[i].item]);
        Font_SetPos(x + 240, rowY);
        Text_PrintfColored("%5dP", dojo->shopLines[i].price);
        Item_DrawIcon(x + 18, rowY, 20, dojo->shopLines[i].item);
        if (i == dojo->shopCursor) {
            if (gFrameTick & 0x10) nudge = 4; else nudge = 10;
            Icon_Draw(x + 46, rowY + 14, 250, 8, 0xA03030A0, 111);
            Menu_DrawCursorIcon(x - 4 - nudge, rowY - 2, 24, 0xBDB59F6C);
        }
        rowY += 24;
    }
    Menu_DrawDashLine(x + 20, 262, 300, 0xFF000000);
    Font_SetColor(QUEST_SHEET_INK);
    ItemInfo_Draw(x + 10, 266, 300, dojo->showInfo, dojo->shopLines[dojo->shopCursor].item, 0, -1, 0x80101010);
    Font_SetSpacing(18, 18);
    switch (dojo->state) {
    case DOJO_SHOP_LIST:
        Win_DrawText(260, 362, 340, 80, 0xBDB59F6C, 0xC0101010, gDojoMenuTexts[DOJO_TEXT_EXCHANGE_HELP]);
        break;
    case DOJO_SHOP_COUNT:
        Win_DrawText(260, 362, 340, 80, 0xBDB59F6C, 0xC0101010, gDojoMenuTexts[DOJO_TEXT_EXCHANGE_COUNT_ASK]);
        Font_SetPos(479, 398);
        sprintf(buf, "%02d", dojo->shopQty);
        Sjis_Widen(buf, wide);
        Text_PrintfColored(wide);
        if (PointShop_CanBuy(dojo->shopCursor, dojo->shopQty + 1) == 1 && dojo->shopQty < POINT_SHOP_MAX_QTY) {
            Icon_DrawMode(480, 376, 36, 22, 0xFF00FF00, 41, 2);
        }
        if (dojo->shopQty > 1) {
            Icon_DrawMode(481, 416, 36, 22, 0xFF00FF00, 40, 2);
        }
        break;
    case DOJO_SHOP_CONFIRM:
        Win_DrawText(260, 362, 340, 80, 0xBDB59F6C, 0xC0101010,
                      Text_FillDollar(gDojoMenuTexts[DOJO_TEXT_EXCHANGE_COUNT_HELP], dojo->shopLines[dojo->shopCursor].price * dojo->shopQty));
        Font_SetPos(390, 412);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

extern char *gDojoRulesTextsOn[];
extern char *gDojoRulesTextsOff[];
/* Draws the rules' sheet: a tab per topic, the topic's page number and the
 * page's text (the online or the offline rules). */
void DojoMenu_DrawRules(s16 x, s16 y)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;
    s16 tabs[10];
    s16 i;
    s16 pageIndex;
    char *text;

    for (i = 0; i < dojo->pages; i++) {
        tabs[i] = 16;
        if (dojo->page == i) {
            tabs[i] = -tabs[i];
        }
    }
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(280, 18, 180, 0xFFB0B000, gDojoMenuTexts[DOJO_TEXT_RULES], 2, 1);
    Menu_DrawQuestSheet(x, y, 340, 360, tabs, dojo->page, -1, -2);
    Font_SetColor(QUEST_SHEET_INK);
    Menu_DrawPageNum(x + 260, y + 20, dojo->rulesPage, dojo->rulesPages, 1);
    Font_SetSpacing(18, 18);
    Font_SetPos(x + 40, y + 50);
    pageIndex = dojo->rulesPage + DojoMenu_RulesFirstPage(dojo->page);
    if (Game_IsOnline() == 1) {
        text = gDojoRulesTextsOn[pageIndex];
    } else {
        text = gDojoRulesTextsOff[pageIndex];
    }
    Text_Printf(text);
}

/* The index of topic TOPIC's first page in the rules' texts: the page
 * counts of the topics before it, summed. */
s16 DojoMenu_RulesFirstPage(s16 topic)
{
    u8 i;
    s16 first = 0;

    if (topic == 0) {
        return 0;
    }
    for (i = 0; i < topic; i++) {
        if (Game_IsOnline() == 1) {
            first += gDojoRulesPagesOn[i];
        } else {
            first += gDojoRulesPagesOff[i];
        }
    }
    return first;
}

/* Draws the training menu's window at X, Y with its categories and the
 * cursor. */
void DojoMenu_DrawTrainingCategories(DojoMenu *dojo, s16 x, s16 y)
{
    char *label;
    s32 i;
    s16 labelX;

    Menu_DrawDecoFrame(x, y, 340, 360, 5, 0);
    Font_SetColor(0x807F7F7F);
    x = x + 170;
    y = y + 100;
    Font_SetSpacing(24, 24);
    for (i = 0; i < dojo->cats; i++) {
        label = gDojoMenuTexts[DOJO_TEXT_TRAINING + i]; /* the training categories' labels */
        labelX = Text_CenterX(label, x, 24);
        if (i == dojo->cat) {
            Menu_DrawArrowsRev(labelX, y, 24, label, 0xFFB57510);
            Text_DrawShadowRgb(labelX, y, 0xFF101010, 0xFF106060, label);
        } else {
            Font_SetColor(0xFF101010);
            Font_SetPos(labelX, y);
            Text_PrintfColored(label);
        }
        y += 60;
    }
}

/* Draws the basics' sheet: a tab per page and the page's text. */
void DojoMenu_DrawBasics(DojoMenu *dojo, s16 x, s16 y)
{
    s16 tabs[DOJO_BASICS_PAGES];
    s16 i;

    for (i = 0; i < DOJO_BASICS_PAGES; i++) {
        tabs[i] = 16;
        if (dojo->page == i) {
            tabs[i] = -tabs[i];
        }
    }
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(280, 18, 260, 0xFFB0B000, gDojoMenuTexts[DOJO_TEXT_TRAINING], 2, 1);
    Menu_DrawQuestSheet(x, y, 340, 360, tabs, dojo->page, -1, -2);
    Font_SetColor(QUEST_SHEET_INK);
    Font_SetSpacing(18, 18);
    Font_SetPos(x + 40, y + 40);
    Text_Printf(gDojoBasicsPages[dojo->page]);
}

/* The menu's first draw callback: the first choice's lines, and the drills'
 * or the training's sheet for the step (the quest sheet shaking with
 * DojoMenu.shake). After a drill is taken the sheet stays up through the
 * goodbye. */
void DojoMenu_DrawPanels(MenuTask *task)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    Font_SetColor(0x80101010);
    switch (dojo->mode) {
    case DOJO_MODE_CHOICE:
        /* the lines start at the drills' label when the drills are offered */
        if (Cond_Check(10) == 1) {
            Menu_DrawLabelList(400, (s8 **)&gDojoMenuTexts[DOJO_TEXT_FIRST_CHOICE], dojo->choiceCount, dojo->choiceCursor, 0xFFB0B000);
        } else {
            Menu_DrawLabelList(400, (s8 **)&gDojoMenuTexts[1], dojo->choiceCount, dojo->choiceCursor, 0xFFB0B000);
        }
        break;
    case DOJO_MODE_DRILLS:
        switch (dojo->step) {
        case DRILLS_STEP_CATEGORY:
        case DRILLS_STEP_KIND:
        case DRILLS_STEP_TALK:
            Menu_DrawDecoBox(dojo->win.pos, 58, 340, 360, 5);
            break;
        case DRILLS_STEP_SLIDE_OUT:
        case DRILLS_STEP_BACK_FROM_SHOP:
            if (dojo->next == DRILLS_STEP_QUESTS) {
                DojoMenu_DrawQuestPage(dojo->shake.dx + 260, dojo->shake.dy + 58);
            } else if (dojo->next == DRILLS_STEP_RULES) {
                DojoMenu_DrawRules(260, 58);
            } else {
                PointShop_Draw(260, 58);
            }
            break;
        case DRILLS_STEP_BACK_FROM_RULES:
            DojoMenu_DrawRules(260, 58);
            break;
        case DRILLS_STEP_BACK_FROM_QUESTS:
            DojoMenu_DrawQuestPage(dojo->shake.dx + 260, dojo->shake.dy + 58);
            break;
        case DRILLS_STEP_QUESTS:
            if (dojo->state == DOJO_QUEST_POST && dojo->postStep == DOJO_POST_SETTINGS) {
                DojoMenu_DrawSettings(dojo->shake.dx + 280, dojo->shake.dy + 58);
            } else {
                DojoMenu_DrawQuestPage(dojo->shake.dx + 260, dojo->shake.dy + 58);
            }
            break;
        case DRILLS_STEP_SHOP:
            PointShop_Draw(260, 58);
            break;
        case DRILLS_STEP_RULES:
            DojoMenu_DrawRules(260, 58);
            break;
        }
        break;
    case DOJO_MODE_TRAINING:
        switch (dojo->step) {
        case TRAINING_STEP_SLIDE_IN:
        case TRAINING_STEP_CATEGORY:
        case TRAINING_STEP_TALK:
            Menu_DrawDecoBox(dojo->win.pos, 58, 340, 360, 5);
            break;
        case TRAINING_STEP_SLIDE_OUT:
        case TRAINING_STEP_SLIDE_BACK:
            switch (dojo->cat) {
            case TRAINING_CAT_BASICS:
                DojoMenu_DrawBasics(dojo, 260, 58);
                break;
            default:
                DojoMenu_DrawQuestPage(dojo->shake.dx + 260, dojo->shake.dy + 58);
                break;
            }
            break;
        case TRAINING_STEP_BASICS:
            DojoMenu_DrawBasics(dojo, 260, 58);
            break;
        case TRAINING_STEP_QUESTS:
            DojoMenu_DrawQuestPage(dojo->shake.dx + 260, dojo->shake.dy + 58);
            break;
        }
        break;
    case DOJO_MODE_BYE:
        if (gLobby->questPaid && dojo->questAccepted) {
            DojoMenu_DrawQuestPage(dojo->shake.dx + 260, dojo->shake.dy + 58);
        }
        break;
    }
}

/* The menu's second draw callback: the title bar and the drills' or the
 * training's menu window. */
void DojoMenu_Draw(MenuTask *task)
{
    DojoMenu *dojo = (DojoMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    Font_SetColor(0x80101010);
    switch (dojo->mode) {
    case DOJO_MODE_CHOICE:
        break;
    case DOJO_MODE_DRILLS:
        switch (dojo->step) {
        case DRILLS_STEP_SLIDE_OUT:
        case DRILLS_STEP_BACK_FROM_SHOP:
        case DRILLS_STEP_BACK_FROM_RULES:
        case DRILLS_STEP_BACK_FROM_QUESTS:
            DojoMenu_DrawDrillsMenu(dojo->win.pos, 58, 1);
            break;
        case DRILLS_STEP_CATEGORY:
        case DRILLS_STEP_KIND:
        case DRILLS_STEP_TALK:
            Font_SetSpacing(22, 22);
            Menu_DrawLabelBar(280, 18, 180, 0xFFB0B000, gTownLabels[TOWN_LABEL_HUNTER_DOJO], 2, 255);
            DojoMenu_DrawDrillsMenu(dojo->win.pos, 58, 0);
            break;
        }
        break;
    case DOJO_MODE_TRAINING:
        switch (dojo->step) {
        case TRAINING_STEP_SLIDE_IN:
        case TRAINING_STEP_CATEGORY:
        case TRAINING_STEP_TALK:
            Font_SetSpacing(22, 22);
            Menu_DrawLabelBar(280, 18, 180, 0xFFB0B000, gTownLabels[TOWN_LABEL_HUNTER_DOJO], 2, 255);
        case TRAINING_STEP_SLIDE_OUT:
        case TRAINING_STEP_SLIDE_BACK:
            DojoMenu_DrawTrainingCategories(dojo, dojo->win.pos, 58);
            break;
        }
        break;
    }
}

/* The file's initialised data, in the game's order */
extern char gDojoRulesTextsOn_0[];
extern char gDojoRulesTextsOn_1[];
extern char gDojoRulesTextsOn_2[];
extern char gDojoRulesTextsOn_3[];
extern char gDojoRulesTextsOn_4[];
extern char gDojoRulesTextsOn_5[];
extern char gDojoRulesTextsOn_6[];
extern char gDojoRulesTextsOn_7[];
extern char gDojoRulesTextsOn_8[];
extern char gDojoRulesTextsOn_9[];
extern char gDojoRulesTextsOn_10[];
extern char gDojoRulesTextsOn_11[];
extern char gDojoRulesTextsOn_12[];
extern char gDojoRulesTextsOn_13[];
extern char gDojoRulesTextsOn_14[];
extern char gDojoRulesTextsOn_15[];
extern char gDojoRulesTextsOn_16[];
extern char gDojoRulesTextsOn_17[];
extern char gDojoRulesTextsOn_18[];
extern char gDojoRulesTextsOn_19[];
extern char gDojoRulesTextsOn_20[];
extern char gDojoRulesTextsOn_21[];
extern char gDojoRulesTextsOn_22[];
extern char gDojoRulesTextsOn_23[];
extern char gDojoRulesTextsOn_24[];
extern char gDojoRulesTextsOn_25[];
extern char gDojoRulesTextsOn_26[];
extern char gDojoRulesTextsOn_27[];
extern char gDojoRulesTextsOn_28[];
extern char gDojoRulesTextsOn_29[];
extern char gDojoRulesTextsOff_0[];
extern char gDojoRulesTextsOff_1[];
extern char gDojoRulesTextsOff_2[];
extern char gDojoRulesTextsOff_3[];
extern char gDojoRulesTextsOff_4[];
extern char gDojoRulesTextsOff_5[];
extern char gDojoRulesTextsOff_6[];
extern char gDojoRulesTextsOff_7[];
extern char gDojoRulesTextsOff_8[];
extern char gDojoRulesTextsOff_9[];
extern char gDojoRulesTextsOff_10[];
extern char gDojoRulesTextsOff_11[];
extern char gDojoRulesTextsOff_12[];
extern char gDojoRulesTextsOff_13[];
extern char gDojoRulesTextsOff_14[];
extern char gDojoRulesTextsOff_15[];
extern char gDojoRulesTextsOff_16[];
extern char gDojoRulesTextsOff_17[];
extern char gDojoRulesTextsOff_18[];
extern char gDojoRulesTextsOff_19[];
extern char gDojoRulesTextsOff_20[];
extern char gDojoRulesTextsOff_21[];
extern char gDojoRulesTextsOff_22[];
extern char gDojoRulesTextsOff_23[];
extern char gDojoRulesTextsOff_24[];
extern char gDojoRulesTextsOff_25[];
extern char gDojoRulesTextsOff_26[];
extern char gDojoRulesTextsOff_27[];
extern char gDojoBasicsPages_0[];
extern char gDojoBasicsPages_1[];
extern char gDojoBasicsPages_2[];
extern char gDojoBasicsPages_3[];
extern char gDojoBasicsPages_4[];
extern char gDojoBasicsPages_5[];
extern char gDojoBasicsPages_6[];
extern char gDojoBasicsPages_7[];
extern char gDojoBasicsPages_8[];
extern char gDojoBasicsPages_9[];
char *gDojoRulesTextsOn[32] = {
    gDojoRulesTextsOn_0, gDojoRulesTextsOn_1, gDojoRulesTextsOn_2, gDojoRulesTextsOn_3,
    gDojoRulesTextsOn_4, gDojoRulesTextsOn_5, gDojoRulesTextsOn_6, gDojoRulesTextsOn_7,
    gDojoRulesTextsOn_8, gDojoRulesTextsOn_9, gDojoRulesTextsOn_10, gDojoRulesTextsOn_11,
    gDojoRulesTextsOn_12, gDojoRulesTextsOn_13, gDojoRulesTextsOn_14, gDojoRulesTextsOn_15,
    gDojoRulesTextsOn_16, gDojoRulesTextsOn_17, gDojoRulesTextsOn_18, gDojoRulesTextsOn_19,
    gDojoRulesTextsOn_20, gDojoRulesTextsOn_21, gDojoRulesTextsOn_22, gDojoRulesTextsOn_23,
    gDojoRulesTextsOn_24, gDojoRulesTextsOn_25, gDojoRulesTextsOn_26, gDojoRulesTextsOn_27,
    gDojoRulesTextsOn_28, gDojoRulesTextsOn_29, NULL, NULL,
};
char *gDojoRulesTextsOff[28] = {
    gDojoRulesTextsOff_0, gDojoRulesTextsOff_1, gDojoRulesTextsOff_2, gDojoRulesTextsOff_3,
    gDojoRulesTextsOff_4, gDojoRulesTextsOff_5, gDojoRulesTextsOff_6, gDojoRulesTextsOff_7,
    gDojoRulesTextsOff_8, gDojoRulesTextsOff_9, gDojoRulesTextsOff_10, gDojoRulesTextsOff_11,
    gDojoRulesTextsOff_12, gDojoRulesTextsOff_13, gDojoRulesTextsOff_14, gDojoRulesTextsOff_15,
    gDojoRulesTextsOff_16, gDojoRulesTextsOff_17, gDojoRulesTextsOff_18, gDojoRulesTextsOff_19,
    gDojoRulesTextsOff_20, gDojoRulesTextsOff_21, gDojoRulesTextsOff_22, gDojoRulesTextsOff_23,
    gDojoRulesTextsOff_24, gDojoRulesTextsOff_25, gDojoRulesTextsOff_26, gDojoRulesTextsOff_27,
};
char *gDojoBasicsPages[12] = {
    gDojoBasicsPages_0, gDojoBasicsPages_1, gDojoBasicsPages_2, gDojoBasicsPages_3,
    gDojoBasicsPages_4, gDojoBasicsPages_5, gDojoBasicsPages_6, gDojoBasicsPages_7,
    gDojoBasicsPages_8, gDojoBasicsPages_9, NULL, NULL,
};
PointShopLine gPointShopOffline[8] = {
#include "lobby/007E7C90.inc"
};
PointShopLine gPointShopOfflineLv2[10] = {
#include "lobby/007E7CD0.inc"
};
PointShopLine gPointShopOfflineCleared[12] = {
#include "lobby/007E7D20.inc"
};
PointShopLine gPointShopOnline[8] = {
#include "lobby/007E7D80.inc"
};
PointShopLine gPointShopOnlineHr31[10] = {
#include "lobby/007E7DC0.inc"
};
PointShopLine gPointShopOnlineCleared[12] = {
#include "lobby/007E7E10.inc"
};
PointShopLine *gPointShopStocks[POINT_SHOP_STOCKS] = {
    gPointShopOffline, gPointShopOfflineLv2, gPointShopOfflineCleared,
    gPointShopOnline, gPointShopOnlineHr31, gPointShopOnlineCleared,
};
u16 gDojoTrainingQuests1[6] = {
#include "lobby/007E7E88.inc"
};
u16 gDojoTrainingQuests2[8] = {
#include "lobby/007E7EA0.inc"
};
u16 gDojoTrainingQuests2Full[12] = {
#include "lobby/007E7EB0.inc"
};
u16 gDojoTrainingQuests3[4] = {
#include "lobby/007E7EC8.inc"
};
s16 gDojoRulesPagesOff[16] = {
#include "lobby/007E7ED0.inc"
};
s16 gDojoRulesPagesOn[16] = {
#include "lobby/007E7EF0.inc"
};
