/*
 * The quest advice messages (MsgEvent). They show in quests with advice
 * (Quest_HasAdvice). A message event (MsgEvent_Check, called by the quest,
 * the player's states, the supply box and the Apceros; Quest_CheckCampAdvice
 * fires the camp's) picks a message once per save, recorded in event flags;
 * MsgEvent_OpenPopup shows it in a popup, and the offline field menu lists
 * the quest set's messages already seen.
 *
 * Their state is QuestMsgWin (gQuestMsgWindow); gMsgEventSets,
 * gMsgEventSetEntries, gMsgEventTitles and gMsgEventLines give each set's
 * messages, titles and pages.
 */

#include "common.h"
#include "msg_event.h"
#include "hud.h"
#include "sound.h"
#include "pad.h"
#include "quest.h"
#include "player.h"
#include "scene.h"
#include "stageobj.h"
#include "window.h"
#include "area.h"
#include "texture.h"
#include "area_draw.h"
#include "shake.h"
#include "font.h"
#include "game_util.h"
#include "menu_util.h"
#include "libc.h"
#include "save.h"
#include "prim.h"

/* An entry of a dated message-event list (MsgEvent_CheckList): in quest questId, event flag flag is
 * set once, message msg is requested (QuestMsgWin.request) and the seen flags 200 + ev[i] of the
 * messages it unlocks are set. A negative flag ends the list. */
typedef struct MsgEvent {
    s16 flag;
    u16 questId;
    u8 msg;
    u8 ev[3];
} MsgEvent;

/* prototypes */
extern u16 gFrameTick;

struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
s32 Str_Length(s8 *text);
void Text_Draw(s8 *text);
extern ListPanel gMsgEventListPanel;
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void Text_PrintfStyled(s32 a, const char *fmt, ...);
typedef struct SprRect {
    s16 x, y, w, h;
    s32 color;
    s32 uv0;
    s32 uv1;
} SprRect;

void Tex_Bind(s32 tex);
extern u8 gMsgEventSetIcon[];
/* The message window of the quest's advice messages (gQuestMsgWindowBuf, which gQuestMsgWindow
 * points to once MsgEvent_Init ran): the field menu's list of the quest set's messages
 * (MsgEvent_OpenList, MsgEvent_ListInput: a Grid of 7 rows) and the popup of a message an event
 * chose (MsgEvent_Check, MsgEvent_OpenPopup, MsgEvent_PopupInput). */
typedef struct QuestMsgWin {
    u8 listStep;        /* 0x00: the list window: 0 open, 1 closing (Shake), 2 closed */
    u8 listSub;         /* 0x01: 0 the list, 1 a message's pages, 2 left with the back button */
    s16 unk02;          /* 0x02: cleared by MsgEvent_Init */
    u8 page;            /* 0x04: the page of the message shown (MsgEvent_Line) */
    u8 unk05;
    u8 pages;           /* 0x06: its pages (MsgEvent_PageCount) */
    u8 set;             /* 0x07: the message set: 18 .. 24 for seven quest ids, else gQuest.stage - 1 */
    u8 msg;             /* 0x08: the message shown */
    s8 request;         /* 0x09: the message an event chose (MsgEvent_CheckList, MsgEvent_CheckQuest), -1: none */
    u8 count;           /* 0x0A: the messages of the set (MsgEvent_CountInSet) */
    u8 popupStep;       /* 0x0B: the popup: 0 none, 1 opening, 2 shown, 3 closing (MsgEvent_PopupInput) */
    u8 popupReq;        /* 0x0C: 1: MsgEvent_Check chose a message; MsgEvent_OpenPopup opens the popup */
    u8 unk0D[16 - 13];
    Grid grid;          /* 0x10: the list's cursor (7 rows; grid.index is the message) */
} QuestMsgWin;

extern QuestMsgWin gQuestMsgWindowBuf;
extern MsgEvent gMsgEventsQuestStart[];
extern MsgEvent gMsgEventsSupplyBox[];
extern MsgEvent gMsgEventsGoalDelivered[];
extern MsgEvent gMsgEventsApcerosHunted[];
extern MsgEvent gMsgEventsCamp[];
extern MsgEvent gMsgEventsJungle1[];
extern MsgEvent gMsgEventsJungle4[];
extern u16 gMsgEventSets[][2];
extern QuestMsgWin *gQuestMsgWindow;
extern Panel gMsgEventPopupWin;
extern u8 *gMsgEventSetEntries[];
extern char *gMsgEventTitles[];
extern char **gMsgEventLines[];
/*
 * Fires message event 14, the camp advice (MsgEvent_Check), when the local
 * player is in the base camp (gScene.campArea) and a monster of kind 12 was
 * hunted this quest. Quest_Update calls it every frame of a quest with
 * advice.
 */
void Quest_CheckCampAdvice(void)
{
    Player *player = &gPlayers[gScene.localPlayer];

    if (player->area == gScene.campArea && gQuest.hunted[12] > 0) {
        MsgEvent_Check(MSGEV_CAMP);
    }
}

/*
 * Sets the message window up for the quest about to start (Quest_Init calls
 * it): points gQuestMsgWindow at gQuestMsgWindowBuf, clears the list's and
 * the popup's steps, and picks the quest's message set (sets 18 to 24 for
 * seven quest ids, else the quest id less 1) and the set's message count.
 * listStep and listSub are cleared by one 16-bit store, as the game's code
 * does.
 */
void MsgEvent_Init(void)
{
    gQuestMsgWindow = &gQuestMsgWindowBuf;
    *(s16 *)&gQuestMsgWindow->listStep = 0;
    gQuestMsgWindow->unk02 = 0;
    gQuestMsgWindow->popupReq = 0;
    gQuestMsgWindow->popupStep = 0;
    switch (gQuest.stage) {
    case QUEST_ID_STARTER_A:
    case QUEST_ID_STARTER_B:
    case QUEST_ID_STARTER_C:
        gQuestMsgWindow->set = 18;
        break;
    case QUEST_ID_DAIMYO_HERMITAUR_SCENE_2:
        gQuestMsgWindow->set = 19;
        break;
    case QUEST_ID_KUSHALA_JUNGLE_SCENE:
        gQuestMsgWindow->set = 20;
        break;
    case QUEST_ID_KUSHALA_SNOW_SCENE:
        gQuestMsgWindow->set = 21;
        break;
    case QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE:
        gQuestMsgWindow->set = 22;
        break;
    case QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE:
        gQuestMsgWindow->set = 23;
        break;
    case QUEST_ID_FORTRESS_STORY:
        gQuestMsgWindow->set = 24;
        break;
    default:
        gQuestMsgWindow->set = gQuest.stage - 1;
        break;
    }
    gQuestMsgWindow->count = MsgEvent_CountInSet(gQuestMsgWindow->set);
}

/*
 * Opens the popup of the message a check chose (QuestMsgWin.popupReq): page
 * 0, the message's page count, the game paused, the window sliding in and the
 * opening sound. ScEvent_Update calls it every frame while no scene event
 * runs.
 */
void MsgEvent_OpenPopup(void)
{
    if (gQuestMsgWindow->popupReq != 0) {
        gQuestMsgWindow->popupReq = 0U;
        gQuestMsgWindow->popupStep = (u8) (gQuestMsgWindow->popupStep + 1);
        gQuestMsgWindow->page = 0;
        gQuestMsgWindow->pages = MsgEvent_PageCount(gQuestMsgWindow->set, gQuestMsgWindow->msg);
        Game_Pause(0);
        Shake_Start(7, SHAKE_AXIS_Y, 330.0f);
        Sound_PlaySeFlat(SE_SLOT_SYSTEM, 16, 0);
    }
}

/*
 * Steps the popup with the buttons pad and returns QuestMsgWin.popupStep (0:
 * no popup). Step 1 waits for the window to slide in; step 2 turns pages with
 * PAD_LEFT and PAD_RIGHT and starts closing on PAD_CROSS; step 3 waits for
 * the window to slide out and resumes the game. Step 1 runs step 2's input on
 * the frame its slide ends: the case label stays inside the if, as in the
 * game's code.
 */
u8 MsgEvent_PopupInput(s32 pad)
{
    s32 buttons;
    u8 pageBefore;
    u8 step;
    u8 pageNow;

    step = gQuestMsgWindow->popupStep;
    switch (step) {
    case 0:
        break;
    case 1:
        if (Shake_Update(7U) != 1) {
            gQuestMsgWindow->popupStep = (u8) (gQuestMsgWindow->popupStep + 1);
        case 2:
            buttons = pad & 0xFFFF;
            if (buttons & PAD_CROSS) {
                gQuestMsgWindow->popupStep = (u8) (gQuestMsgWindow->popupStep + 1);
                Shake_Start(7, SHAKE_SLIDE | SHAKE_AXIS_Y, 330.0f);
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            } else if (buttons & PAD_LEFT) {
                pageBefore = gQuestMsgWindow->page;
                if (pageBefore != 0) {
                    gQuestMsgWindow->page = (u8) (pageBefore - 1);
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
                }
            } else if (buttons & PAD_RIGHT) {
                pageNow = gQuestMsgWindow->page;
                if ((s32) pageNow < (gQuestMsgWindow->pages - 1)) {
                    gQuestMsgWindow->page = (u8) (pageNow + 1);
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
                }
            }
        }
        break;
    case 3:
        if (Shake_Update(7U) == 0) {
            gQuestMsgWindow->popupStep = 0U;
            Game_Resume(0);
        }
        break;
    }
    return gQuestMsgWindow->popupStep;
}

extern s8 gHelpShow;
/*
 * Opens the message list, the offline field menu's advice entry: returns 1
 * and does nothing once the quest is over (Quest_PhaseIsOver); else pauses
 * the game, sets the list's Grid up (7 rows, as many columns as the set's
 * messages need), hides the help line, starts the window's slide and returns
 * 0.
 */
s32 MsgEvent_OpenList(void)
{
    if (Quest_PhaseIsOver(0) != 1) {
        gQuestMsgWindow->listSub = 0;
        gQuestMsgWindow->listStep = 0;
        Game_Pause(0);
        gQuestMsgWindow->grid.index = 0;
        gQuestMsgWindow->grid.rows = 7;
        gQuestMsgWindow->grid.row = 0;
        gQuestMsgWindow->grid.col = 0;
        gQuestMsgWindow->grid.cols = (gQuestMsgWindow->count + 6) / 7;
        gQuestMsgWindow->grid.lastRows = gQuestMsgWindow->count - (gQuestMsgWindow->grid.cols - 1) * 7;
        gHelpShow = 0;
        Shake_Start(7, SHAKE_AXIS_Y, 330.0f);
        return 0;
    }
    return 1;
}

/*
 * The list's input, which the field menu runs every frame with the buttons
 * pad. Returns PAD_START once the window has slid out after PAD_START,
 * PAD_CROSS after the player left the list with PAD_CROSS, else 0; while the
 * window slides in only PAD_START, the D-pad and PAD_CROSS count. In the list
 * PAD_CIRCLE opens a message already seen (MsgEvent_IsSeen) or plays the
 * refusal sound; in a message PAD_LEFT and PAD_RIGHT turn pages and PAD_CROSS
 * goes back to the list.
 */
s32 MsgEvent_ListInput(u16 pad)
{
    u16 buttons;
    u8 *listSubPtr;

    switch (gQuestMsgWindow->listStep) {
    case 0:
        if (Shake_Update(7) == 1) {
            pad &= PAD_START | PAD_DPAD | PAD_CROSS;
        }
        buttons = pad;
        if (buttons & PAD_START) {
            Shake_Start(7, SHAKE_SLIDE | SHAKE_AXIS_Y, 330.0f);
            gQuestMsgWindow->listStep++;
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            break;
        }
        listSubPtr = &gQuestMsgWindow->listSub;
        switch (*listSubPtr) {
        case 0:
            if (buttons & PAD_CROSS) {
                Game_Resume(0);
                gQuestMsgWindow->listSub = 2;
            } else if (gQuestMsgWindow->grid.cols != 0) {
                Menu_GridCursor(&gQuestMsgWindow->grid, pad);
                if (buttons & PAD_CIRCLE) {
                    if (MsgEvent_IsSeen(gQuestMsgWindow->set, *(u8 *)&gQuestMsgWindow->grid.index) == 1) {
                        gQuestMsgWindow->page = 0;
                        gQuestMsgWindow->msg = gQuestMsgWindow->grid.index;
                        gQuestMsgWindow->pages = MsgEvent_PageCount(gQuestMsgWindow->set, gQuestMsgWindow->msg);
                        gQuestMsgWindow->listSub++;
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
                    }
                }
            }
            break;
        case 1:
            if (buttons & PAD_CROSS) {
                *listSubPtr = 0;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            } else if (buttons & PAD_LEFT) {
                if (gQuestMsgWindow->page != 0) {
                    gQuestMsgWindow->page--;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
                }
            } else if (buttons & PAD_RIGHT) {
                if (gQuestMsgWindow->page < gQuestMsgWindow->pages - 1) {
                    gQuestMsgWindow->page++;
                    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_SCROLL, 0);
                }
            }
            break;
        case 2:
            return 64;
        }
        break;
    case 1:
        if (Shake_Update(7) == 0) {
            gQuestMsgWindow->listStep++;
            Game_Resume(0);
            return 32768;
        }
        break;
    }
    return 0;
}

/*
 * Draws the popup at its slide offset: its window, the message with the
 * popup's button hint (MsgEvent_DrawMessage) and the icon. Hud_DrawLayer2
 * calls it while a popup is open.
 */
void MsgEvent_DrawPopup(void)
{
    s16 *slideOfs = Shake_GetOffset(7);

    Window_DrawText(&gMsgEventPopupWin, slideOfs, 0);
    MsgEvent_DrawMessage(0, (s32) slideOfs);
    MsgEvent_DrawIcon(0, (s32) slideOfs);
}

/* The texts of the advice messages, reached only through gMsgEventTitles and the gMsgEventLines
 * tables; they stay here, in the game's order, at the start of the file's constants, before
 * MsgEvent_DrawList's. */
#pragma rodata on
char gMsgEventTitles_0[25] = {
#include "game/008DE300.inc"
};
char gMsgEventTitles_1[27] = {
#include "game/008DE320.inc"
};
char gMsgEventTitles_2[23] = {
#include "game/008DE340.inc"
};
char gMsgEventTitles_3[35] = {
#include "game/008DE360.inc"
};
char gMsgEventTitles_4[37] = {
#include "game/008DE390.inc"
};
char gMsgEventTitles_5[31] = {
#include "game/008DE3C0.inc"
};
char gMsgEventTitles_6[23] = {
#include "game/008DE3E0.inc"
};
char gMsgEventTitles_7[15] = {
#include "game/008DE3F8.inc"
};
char gMsgEventTitles_8[29] = {
#include "game/008DE410.inc"
};
char gMsgEventTitles_9[23] = {
#include "game/008DE430.inc"
};
char gMsgEventTitles_10[21] = {
#include "game/008DE450.inc"
};
char gMsgEventTitles_11[31] = {
#include "game/008DE470.inc"
};
char gMsgEventTitles_12[25] = {
#include "game/008DE490.inc"
};
char gMsgEventTitles_13[23] = {
#include "game/008DE4B0.inc"
};
char gMsgEventTitles_14[15] = {
#include "game/008DE4C8.inc"
};
char gMsgEventTitles_15[31] = {
#include "game/008DE4E0.inc"
};
char gMsgEventTitles_16[21] = {
#include "game/008DE500.inc"
};
char gMsgEventTitles_17[37] = {
#include "game/008DE520.inc"
};
char gMsgEventTitles_18[25] = {
#include "game/008DE550.inc"
};
char gMsgEventTitles_19[25] = {
#include "game/008DE570.inc"
};
char gMsgEventTitles_20[17] = {
#include "game/008DE590.inc"
};
char gMsgEventTitles_21[33] = {
#include "game/008DE5B0.inc"
};
char gMsgEventTitles_22[31] = {
#include "game/008DE5E0.inc"
};
char gMsgEventTitles_23[25] = {
#include "game/008DE600.inc"
};
char gMsgEventTitles_24[29] = {
#include "game/008DE620.inc"
};
char gMsgEventTitles_25[23] = {
#include "game/008DE640.inc"
};
char gMsgEventTitles_26[25] = {
#include "game/008DE660.inc"
};
char gMsgEventTitles_27[31] = {
#include "game/008DE680.inc"
};
char gMsgEventTitles_28[13] = {
#include "game/008DE6A0.inc"
};
char gMsgEventTitles_29[25] = {
#include "game/008DE6B0.inc"
};
char gMsgEventTitles_30[11] = {
#include "game/008DE6D0.inc"
};
char gMsgEventTitles_31[23] = {
#include "game/008DE6E0.inc"
};
char gMsgEventTitles_32[21] = {
#include "game/008DE700.inc"
};
char gMsgEventTitles_33[15] = {
#include "game/008DE718.inc"
};
char gMsgEventTitles_34[29] = {
#include "game/008DE730.inc"
};
char gMsgEventTitles_35[13] = {
#include "game/008DE750.inc"
};
char gMsgEventTitles_36[27] = {
#include "game/008DE760.inc"
};
char gMsgEventTitles_37[11] = {
#include "game/008DE780.inc"
};
char gMsgEventTitles_38[23] = {
#include "game/008DE790.inc"
};
char gMsgEventTitles_39[11] = {
#include "game/008DE7A8.inc"
};
char gMsgEventTitles_40[21] = {
#include "game/008DE7C0.inc"
};
char gMsgEventTitles_41[13] = {
#include "game/008DE7D8.inc"
};
char gMsgEventTitles_42[23] = {
#include "game/008DE7F0.inc"
};
char gMsgEventTitles_43[17] = {
#include "game/008DE810.inc"
};
char gMsgEventTitles_44[19] = {
#include "game/008DE830.inc"
};
char gMsgEventTitles_45[19] = {
#include "game/008DE850.inc"
};
char gMsgEventTitles_46[21] = {
#include "game/008DE870.inc"
};
char gMsgEventTitles_47[27] = {
#include "game/008DE890.inc"
};
char gMsgEventTitles_48[21] = {
#include "game/008DE8B0.inc"
};
char gMsgEventTitles_49[23] = {
#include "game/008DE8D0.inc"
};
char gMsgEventTitles_50[9] = {
#include "game/008DE8E8.inc"
};
char gMsgEventTitles_51[11] = {
#include "game/008DE8F8.inc"
};
char gMsgEventTitles_52[25] = {
#include "game/008DE910.inc"
};
char gMsgEventTitles_53[15] = {
#include "game/008DE930.inc"
};
char gMsgEventTitles_54[27] = {
#include "game/008DE940.inc"
};
char gMsgEventTitles_55[13] = {
#include "game/008DE960.inc"
};
char gMsgEventTitles_56[23] = {
#include "game/008DE970.inc"
};
char gMsgEventTitles_57[23] = {
#include "game/008DE990.inc"
};
char gMsgEventTitles_58[25] = {
#include "game/008DE9B0.inc"
};
char gMsgEventTitles_59[15] = {
#include "game/008DE9D0.inc"
};
char gMsgEventTitles_60[7] = {
#include "game/008DE9E0.inc"
};
char gMsgEventTitles_61[9] = {
#include "game/008DE9E8.inc"
};
char gMsgEventTitles_62[11] = {
#include "game/008DE9F8.inc"
};
char gMsgEventTitles_63[11] = {
#include "game/008DEA08.inc"
};
char gMsgEventTitles_64[9] = {
#include "game/008DEA18.inc"
};
char gMsgEventTitles_65[21] = {
#include "game/008DEA30.inc"
};
char D_008DEA50[19] = {
#include "game/008DEA50.inc"
};
char gMsgEventTitles_68[19] = {
#include "game/008DEA70.inc"
};
char gMsgEventTitles_69[17] = {
#include "game/008DEA90.inc"
};
char gMsgEventTitles_70[27] = {
#include "game/008DEAB0.inc"
};
char gMsgEventTitles_71[13] = {
#include "game/008DEAD0.inc"
};
char gMsgEventTitles_72[31] = {
#include "game/008DEAE0.inc"
};
char gMsgEventTitles_73[26] = {
#include "game/008DEB00.inc"
};
char gMsgEventTitles_74[21] = {
#include "game/008DEB20.inc"
};
char gMsgEventTitles_75[17] = {
#include "game/008DEB40.inc"
};
char gMsgEventTitles_76[15] = {
#include "game/008DEB58.inc"
};
char gMsgEventLines_0_0[167] = {
#include "game/008DEB70.inc"
};
char gMsgEventLines_0_1[161] = {
#include "game/008DEC20.inc"
};
char gMsgEventLines_0_2[161] = {
#include "game/008DECD0.inc"
};
char gMsgEventLines_0_3[224] = {
#include "game/008DED80.inc"
};
char gMsgEventLines_0_4[199] = {
#include "game/008DEE60.inc"
};
char gMsgEventLines_1_0[167] = {
#include "game/008DEF30.inc"
};
char gMsgEventLines_1_1[180] = {
#include "game/008DEFE0.inc"
};
char gMsgEventLines_1_2[205] = {
#include "game/008DF0A0.inc"
};
char gMsgEventLines_2_0[107] = {
#include "game/008DF170.inc"
};
char gMsgEventLines_2_1[179] = {
#include "game/008DF1E0.inc"
};
char gMsgEventLines_2_2[185] = {
#include "game/008DF2A0.inc"
};
char gMsgEventLines_2_3[146] = {
#include "game/008DF360.inc"
};
char gMsgEventLines_3_0[176] = {
#include "game/008DF400.inc"
};
char gMsgEventLines_3_1[199] = {
#include "game/008DF4B0.inc"
};
char gMsgEventLines_3_2[218] = {
#include "game/008DF580.inc"
};
char gMsgEventLines_4_0[197] = {
#include "game/008DF660.inc"
};
char gMsgEventLines_4_1[157] = {
#include "game/008DF730.inc"
};
char gMsgEventLines_4_2[165] = {
#include "game/008DF7D0.inc"
};
char gMsgEventLines_5_0[156] = {
#include "game/008DF880.inc"
};
char gMsgEventLines_5_1[227] = {
#include "game/008DF920.inc"
};
char gMsgEventLines_5_2[155] = {
#include "game/008DFA10.inc"
};
char gMsgEventLines_5_3[172] = {
#include "game/008DFAB0.inc"
};
char gMsgEventLines_6_0[168] = {
#include "game/008DFB60.inc"
};
char gMsgEventLines_6_1[249] = {
#include "game/008DFC10.inc"
};
char gMsgEventLines_6_2[189] = {
#include "game/008DFD10.inc"
};
char gMsgEventLines_7_0[134] = {
#include "game/008DFDD0.inc"
};
char gMsgEventLines_7_1[203] = {
#include "game/008DFE60.inc"
};
char gMsgEventLines_7_2[257] = {
#include "game/008DFF30.inc"
};
char gMsgEventLines_7_3[178] = {
#include "game/008E0040.inc"
};
char gMsgEventLines_7_4[129] = {
#include "game/008E0100.inc"
};
char gMsgEventLines_7_5[250] = {
#include "game/008E0190.inc"
};
char gMsgEventLines_7_6[136] = {
#include "game/008E0290.inc"
};
char gMsgEventLines_8_0[211] = {
#include "game/008E0320.inc"
};
char gMsgEventLines_8_1[177] = {
#include "game/008E0400.inc"
};
char gMsgEventLines_8_2[183] = {
#include "game/008E04C0.inc"
};
char gMsgEventLines_8_3[160] = {
#include "game/008E0580.inc"
};
char gMsgEventLines_9_0[235] = {
#include "game/008E0620.inc"
};
char gMsgEventLines_10_0[130] = {
#include "game/008E0710.inc"
};
char gMsgEventLines_10_1[162] = {
#include "game/008E07A0.inc"
};
char gMsgEventLines_10_2[209] = {
#include "game/008E0850.inc"
};
char gMsgEventLines_10_3[242] = {
#include "game/008E0930.inc"
};
char gMsgEventLines_10_4[172] = {
#include "game/008E0A30.inc"
};
char gMsgEventLines_11_0[86] = {
#include "game/008E0AE0.inc"
};
char gMsgEventLines_11_1[219] = {
#include "game/008E0B40.inc"
};
char gMsgEventLines_11_2[132] = {
#include "game/008E0C20.inc"
};
char gMsgEventLines_11_3[168] = {
#include "game/008E0CB0.inc"
};
char gMsgEventLines_11_4[182] = {
#include "game/008E0D60.inc"
};
char gMsgEventLines_12_0[166] = {
#include "game/008E0E20.inc"
};
char gMsgEventLines_12_1[179] = {
#include "game/008E0ED0.inc"
};
char gMsgEventLines_12_2[223] = {
#include "game/008E0F90.inc"
};
char gMsgEventLines_12_3[192] = {
#include "game/008E1070.inc"
};
char gMsgEventLines_12_4[169] = {
#include "game/008E1130.inc"
};
char gMsgEventLines_12_5[123] = {
#include "game/008E11E0.inc"
};
char gMsgEventLines_13_0[171] = {
#include "game/008E1260.inc"
};
char gMsgEventLines_13_1[172] = {
#include "game/008E1310.inc"
};
char gMsgEventLines_13_2[190] = {
#include "game/008E13C0.inc"
};
char gMsgEventLines_13_3[171] = {
#include "game/008E1480.inc"
};
char gMsgEventLines_13_4[223] = {
#include "game/008E1530.inc"
};
char gMsgEventLines_14_0[126] = {
#include "game/008E1610.inc"
};
char gMsgEventLines_14_1[191] = {
#include "game/008E1690.inc"
};
char gMsgEventLines_14_2[189] = {
#include "game/008E1750.inc"
};
char gMsgEventLines_14_3[150] = {
#include "game/008E1810.inc"
};
char gMsgEventLines_15_0[243] = {
#include "game/008E18B0.inc"
};
char gMsgEventLines_16_0[130] = {
#include "game/008E19B0.inc"
};
char gMsgEventLines_16_1[156] = {
#include "game/008E1A40.inc"
};
char gMsgEventLines_16_2[150] = {
#include "game/008E1AE0.inc"
};
char gMsgEventLines_17_0[119] = {
#include "game/008E1B80.inc"
};
char gMsgEventLines_17_1[138] = {
#include "game/008E1C00.inc"
};
char gMsgEventLines_17_2[150] = {
#include "game/008E1C90.inc"
};
char gMsgEventLines_17_3[136] = {
#include "game/008E1D30.inc"
};
char gMsgEventLines_18_0[178] = {
#include "game/008E1DC0.inc"
};
char gMsgEventLines_18_1[213] = {
#include "game/008E1E80.inc"
};
char gMsgEventLines_18_2[210] = {
#include "game/008E1F60.inc"
};
char gMsgEventLines_18_3[158] = {
#include "game/008E2040.inc"
};
char gMsgEventLines_18_4[199] = {
#include "game/008E20E0.inc"
};
char gMsgEventLines_19_0[137] = {
#include "game/008E21B0.inc"
};
char gMsgEventLines_19_1[192] = {
#include "game/008E2240.inc"
};
char gMsgEventLines_19_2[96] = {
#include "game/008E2300.inc"
};
char gMsgEventLines_19_3[137] = {
#include "game/008E2360.inc"
};
char gMsgEventLines_20_0[131] = {
#include "game/008E23F0.inc"
};
char gMsgEventLines_20_1[162] = {
#include "game/008E2480.inc"
};
char gMsgEventLines_20_2[215] = {
#include "game/008E2530.inc"
};
char gMsgEventLines_20_3[178] = {
#include "game/008E2610.inc"
};
char gMsgEventLines_20_4[215] = {
#include "game/008E26D0.inc"
};
char gMsgEventLines_21_0[221] = {
#include "game/008E27B0.inc"
};
char gMsgEventLines_22_0[130] = {
#include "game/008E2890.inc"
};
char gMsgEventLines_22_1[162] = {
#include "game/008E2920.inc"
};
char gMsgEventLines_22_2[196] = {
#include "game/008E29D0.inc"
};
char gMsgEventLines_22_3[183] = {
#include "game/008E2AA0.inc"
};
char gMsgEventLines_22_4[239] = {
#include "game/008E2B60.inc"
};
char gMsgEventLines_23_0[151] = {
#include "game/008E2C50.inc"
};
char gMsgEventLines_23_1[168] = {
#include "game/008E2CF0.inc"
};
char gMsgEventLines_23_2[200] = {
#include "game/008E2DA0.inc"
};
char gMsgEventLines_24_0[251] = {
#include "game/008E2E70.inc"
};
char gMsgEventLines_25_0[126] = {
#include "game/008E2F70.inc"
};
char gMsgEventLines_25_1[152] = {
#include "game/008E2FF0.inc"
};
char gMsgEventLines_25_2[142] = {
#include "game/008E3090.inc"
};
char gMsgEventLines_25_3[217] = {
#include "game/008E3120.inc"
};
char gMsgEventLines_25_4[187] = {
#include "game/008E3200.inc"
};
char gMsgEventLines_26_0[181] = {
#include "game/008E32C0.inc"
};
char gMsgEventLines_26_1[156] = {
#include "game/008E3380.inc"
};
char gMsgEventLines_26_2[157] = {
#include "game/008E3420.inc"
};
char gMsgEventLines_26_3[134] = {
#include "game/008E34C0.inc"
};
char gMsgEventLines_26_4[113] = {
#include "game/008E3550.inc"
};
char gMsgEventLines_26_5[152] = {
#include "game/008E35D0.inc"
};
char gMsgEventLines_27_0[241] = {
#include "game/008E3670.inc"
};
char gMsgEventLines_28_0[128] = {
#include "game/008E3770.inc"
};
char D_008E37F0[179] = {
#include "game/008E37F0.inc"
};
char gMsgEventLines_28_2[193] = {
#include "game/008E38B0.inc"
};
char gMsgEventLines_28_3[170] = {
#include "game/008E3980.inc"
};
char gMsgEventLines_28_4[156] = {
#include "game/008E3A30.inc"
};
char gMsgEventLines_28_5[156] = {
#include "game/008E3AD0.inc"
};
char D_008E3B70[156] = {
#include "game/008E3B70.inc"
};
char D_008E3C10[185] = {
#include "game/008E3C10.inc"
};
char gMsgEventLines_29_2[136] = {
#include "game/008E3CD0.inc"
};
char gMsgEventLines_30_0[126] = {
#include "game/008E3D60.inc"
};
char gMsgEventLines_30_2[218] = {
#include "game/008E3DE0.inc"
};
char gMsgEventLines_30_3[207] = {
#include "game/008E3EC0.inc"
};
char gMsgEventLines_30_4[215] = {
#include "game/008E3F90.inc"
};
char gMsgEventLines_31_0[164] = {
#include "game/008E4070.inc"
};
char gMsgEventLines_31_1[187] = {
#include "game/008E4120.inc"
};
char gMsgEventLines_31_2[207] = {
#include "game/008E41E0.inc"
};
char gMsgEventLines_32_0[205] = {
#include "game/008E42B0.inc"
};
char gMsgEventLines_32_1[197] = {
#include "game/008E4380.inc"
};
char gMsgEventLines_33_0[130] = {
#include "game/008E4450.inc"
};
char gMsgEventLines_33_2[183] = {
#include "game/008E44E0.inc"
};
char gMsgEventLines_33_3[201] = {
#include "game/008E45A0.inc"
};
char gMsgEventLines_33_4[213] = {
#include "game/008E4670.inc"
};
char gMsgEventLines_33_5[173] = {
#include "game/008E4750.inc"
};
char gMsgEventLines_34_0[135] = {
#include "game/008E4800.inc"
};
char gMsgEventLines_34_1[199] = {
#include "game/008E4890.inc"
};
char gMsgEventLines_34_2[185] = {
#include "game/008E4960.inc"
};
char gMsgEventLines_35_0[128] = {
#include "game/008E4A20.inc"
};
char gMsgEventLines_35_2[216] = {
#include "game/008E4AA0.inc"
};
char gMsgEventLines_35_3[215] = {
#include "game/008E4B80.inc"
};
char gMsgEventLines_35_4[229] = {
#include "game/008E4C60.inc"
};
char gMsgEventLines_35_5[187] = {
#include "game/008E4D50.inc"
};
char gMsgEventLines_36_2[142] = {
#include "game/008E4E10.inc"
};
char gMsgEventLines_36_3[165] = {
#include "game/008E4EA0.inc"
};
char gMsgEventLines_37_0[126] = {
#include "game/008E4F50.inc"
};
char gMsgEventLines_37_2[182] = {
#include "game/008E4FD0.inc"
};
char gMsgEventLines_37_3[152] = {
#include "game/008E5090.inc"
};
char gMsgEventLines_37_4[162] = {
#include "game/008E5130.inc"
};
char gMsgEventLines_37_5[217] = {
#include "game/008E51E0.inc"
};
char gMsgEventLines_38_0[215] = {
#include "game/008E52C0.inc"
};
char gMsgEventLines_38_1[198] = {
#include "game/008E53A0.inc"
};
char gMsgEventLines_38_2[231] = {
#include "game/008E5470.inc"
};
char gMsgEventLines_39_0[126] = {
#include "game/008E5560.inc"
};
char gMsgEventLines_39_2[174] = {
#include "game/008E55E0.inc"
};
char gMsgEventLines_39_3[205] = {
#include "game/008E5690.inc"
};
char gMsgEventLines_39_4[193] = {
#include "game/008E5760.inc"
};
char gMsgEventLines_40_0[233] = {
#include "game/008E5830.inc"
};
char gMsgEventLines_40_1[177] = {
#include "game/008E5920.inc"
};
char gMsgEventLines_40_2[177] = {
#include "game/008E59E0.inc"
};
char gMsgEventLines_40_3[185] = {
#include "game/008E5AA0.inc"
};
char gMsgEventLines_41_0[128] = {
#include "game/008E5B60.inc"
};
char gMsgEventLines_41_2[197] = {
#include "game/008E5BE0.inc"
};
char gMsgEventLines_41_3[221] = {
#include "game/008E5CB0.inc"
};
char gMsgEventLines_41_4[191] = {
#include "game/008E5D90.inc"
};
char gMsgEventLines_42_0[225] = {
#include "game/008E5E50.inc"
};
char gMsgEventLines_42_1[203] = {
#include "game/008E5F40.inc"
};
char gMsgEventLines_42_2[211] = {
#include "game/008E6010.inc"
};
char gMsgEventLines_42_3[228] = {
#include "game/008E60F0.inc"
};
char gMsgEventLines_43_0[133] = {
#include "game/008E61E0.inc"
};
char gMsgEventLines_43_2[202] = {
#include "game/008E6270.inc"
};
char gMsgEventLines_43_3[203] = {
#include "game/008E6340.inc"
};
char gMsgEventLines_43_4[213] = {
#include "game/008E6410.inc"
};
char gMsgEventLines_43_5[199] = {
#include "game/008E64F0.inc"
};
char gMsgEventLines_44_0[181] = {
#include "game/008E65C0.inc"
};
char gMsgEventLines_44_1[201] = {
#include "game/008E6680.inc"
};
char gMsgEventLines_44_2[146] = {
#include "game/008E6750.inc"
};
char gMsgEventLines_44_3[199] = {
#include "game/008E67F0.inc"
};
char gMsgEventLines_45_0[213] = {
#include "game/008E68C0.inc"
};
char gMsgEventLines_45_1[211] = {
#include "game/008E69A0.inc"
};
char gMsgEventLines_45_2[197] = {
#include "game/008E6A80.inc"
};
char gMsgEventLines_45_3[164] = {
#include "game/008E6B50.inc"
};
char gMsgEventLines_46_0[137] = {
#include "game/008E6C00.inc"
};
char D_008E6C90[176] = {
#include "game/008E6C90.inc"
};
char gMsgEventLines_46_2[134] = {
#include "game/008E6D40.inc"
};
char gMsgEventLines_46_3[154] = {
#include "game/008E6DD0.inc"
};
char gMsgEventLines_46_4[205] = {
#include "game/008E6E70.inc"
};
char gMsgEventLines_47_0[154] = {
#include "game/008E6F40.inc"
};
char gMsgEventLines_47_1[127] = {
#include "game/008E6FE0.inc"
};
char gMsgEventLines_47_2[207] = {
#include "game/008E7060.inc"
};
char D_008E7130[105] = {
#include "game/008E7130.inc"
};
char gMsgEventLines_48_0[137] = {
#include "game/008E71A0.inc"
};
char gMsgEventLines_48_2[130] = {
#include "game/008E7230.inc"
};
char gMsgEventLines_48_3[195] = {
#include "game/008E72C0.inc"
};
char gMsgEventLines_48_4[189] = {
#include "game/008E7390.inc"
};
char gMsgEventLines_49_0[154] = {
#include "game/008E7450.inc"
};
char gMsgEventLines_49_1[201] = {
#include "game/008E74F0.inc"
};
char gMsgEventLines_49_2[223] = {
#include "game/008E75C0.inc"
};
char gMsgEventLines_50_0[124] = {
#include "game/008E76A0.inc"
};
char gMsgEventLines_50_1[183] = {
#include "game/008E7720.inc"
};
char gMsgEventLines_50_2[221] = {
#include "game/008E77E0.inc"
};
char gMsgEventLines_50_3[222] = {
#include "game/008E78C0.inc"
};
char gMsgEventLines_50_4[164] = {
#include "game/008E79A0.inc"
};
char gMsgEventLines_51_0[162] = {
#include "game/008E7A50.inc"
};
char gMsgEventLines_51_1[152] = {
#include "game/008E7B00.inc"
};
char gMsgEventLines_52_0[199] = {
#include "game/008E7BA0.inc"
};
char gMsgEventLines_52_1[152] = {
#include "game/008E7C70.inc"
};
char gMsgEventLines_52_2[195] = {
#include "game/008E7D10.inc"
};
char gMsgEventLines_52_3[176] = {
#include "game/008E7DE0.inc"
};
char gMsgEventLines_53_0[139] = {
#include "game/008E7E90.inc"
};
char gMsgEventLines_53_1[171] = {
#include "game/008E7F20.inc"
};
char gMsgEventLines_53_2[141] = {
#include "game/008E7FD0.inc"
};
char gMsgEventLines_54_0[156] = {
#include "game/008E8060.inc"
};
char gMsgEventLines_54_1[125] = {
#include "game/008E8100.inc"
};
char gMsgEventLines_54_2[192] = {
#include "game/008E8180.inc"
};
char D_008E8240[239] = {
#include "game/008E8240.inc"
};
char D_008E8330[195] = {
#include "game/008E8330.inc"
};
char gMsgEventLines_55_0[207] = {
#include "game/008E8400.inc"
};
char gMsgEventLines_55_1[189] = {
#include "game/008E84D0.inc"
};
char gMsgEventLines_55_2[205] = {
#include "game/008E8590.inc"
};
char gMsgEventLines_56_0[235] = {
#include "game/008E8660.inc"
};
char gMsgEventLines_57_0[137] = {
#include "game/008E8750.inc"
};
char gMsgEventLines_57_1[233] = {
#include "game/008E87E0.inc"
};
char gMsgEventLines_57_2[138] = {
#include "game/008E88D0.inc"
};
char gMsgEventLines_57_3[188] = {
#include "game/008E8960.inc"
};
char gMsgEventLines_57_4[233] = {
#include "game/008E8A20.inc"
};
char gMsgEventLines_57_5[203] = {
#include "game/008E8B10.inc"
};
char gMsgEventLines_57_6[162] = {
#include "game/008E8BE0.inc"
};
char gMsgEventLines_58_0[132] = {
#include "game/008E8C90.inc"
};
char gMsgEventLines_58_1[142] = {
#include "game/008E8D20.inc"
};
char gMsgEventLines_58_2[191] = {
#include "game/008E8DB0.inc"
};
char D_008E8E70[166] = {
#include "game/008E8E70.inc"
};
char D_008E8F20[213] = {
#include "game/008E8F20.inc"
};
char gMsgEventLines_59_0[233] = {
#include "game/008E9000.inc"
};
char gMsgEventLines_59_1[195] = {
#include "game/008E90F0.inc"
};
char D_008E91C0[217] = {
#include "game/008E91C0.inc"
};
char gMsgEventLines_60_0[207] = {
#include "game/008E92A0.inc"
};
char D_008E9370[167] = {
#include "game/008E9370.inc"
};
char D_008E9420[168] = {
#include "game/008E9420.inc"
};
char D_008E94D0[199] = {
#include "game/008E94D0.inc"
};
char gMsgEventLines_62_0[211] = {
#include "game/008E95A0.inc"
};
char gMsgEventLines_62_1[116] = {
#include "game/008E9680.inc"
};
char gMsgEventLines_63_0[213] = {
#include "game/008E9700.inc"
};
char gMsgEventLines_63_1[217] = {
#include "game/008E97E0.inc"
};
char gMsgEventLines_64_0[189] = {
#include "game/008E98C0.inc"
};
char gMsgEventLines_64_1[118] = {
#include "game/008E9980.inc"
};
char gMsgEventLines_65_0[146] = {
#include "game/008E9A00.inc"
};
char gMsgEventLines_65_1[140] = {
#include "game/008E9AA0.inc"
};
char gMsgEventLines_65_2[203] = {
#include "game/008E9B30.inc"
};
char gMsgEventLines_65_3[189] = {
#include "game/008E9C00.inc"
};
char gMsgEventLines_65_4[221] = {
#include "game/008E9CC0.inc"
};
char gMsgEventLines_66_0[235] = {
#include "game/008E9DA0.inc"
};
char gMsgEventLines_67_0[213] = {
#include "game/008E9E90.inc"
};
char D_008E9F70[224] = {
#include "game/008E9F70.inc"
};
char gMsgEventLines_68_0[202] = {
#include "game/008EA050.inc"
};
char gMsgEventLines_68_1[107] = {
#include "game/008EA120.inc"
};
char gMsgEventLines_69_0[158] = {
#include "game/008EA190.inc"
};
char gMsgEventLines_69_1[205] = {
#include "game/008EA230.inc"
};
char gMsgEventLines_69_2[210] = {
#include "game/008EA300.inc"
};
char gMsgEventLines_69_3[205] = {
#include "game/008EA3E0.inc"
};
char gMsgEventLines_69_4[160] = {
#include "game/008EA4B0.inc"
};
char gMsgEventLines_70_0[187] = {
#include "game/008EA550.inc"
};
char gMsgEventLines_70_1[111] = {
#include "game/008EA610.inc"
};
char gMsgEventLines_70_2[215] = {
#include "game/008EA680.inc"
};
char gMsgEventLines_70_3[147] = {
#include "game/008EA760.inc"
};
char gMsgEventLines_70_4[239] = {
#include "game/008EA800.inc"
};
char gMsgEventLines_70_5[189] = {
#include "game/008EA8F0.inc"
};
char gMsgEventLines_70_6[177] = {
#include "game/008EA9B0.inc"
};
char gMsgEventLines_70_7[177] = {
#include "game/008EAA70.inc"
};
char gMsgEventLines_71_0[189] = {
#include "game/008EAB30.inc"
};
char gMsgEventLines_71_1[178] = {
#include "game/008EABF0.inc"
};
char gMsgEventLines_71_2[155] = {
#include "game/008EACB0.inc"
};
char gMsgEventLines_71_3[178] = {
#include "game/008EAD50.inc"
};
char gMsgEventLines_71_4[142] = {
#include "game/008EAE10.inc"
};
char gMsgEventLines_71_5[142] = {
#include "game/008EAEA0.inc"
};
char gMsgEventLines_71_6[123] = {
#include "game/008EAF30.inc"
};
char gMsgEventLines_71_7[128] = {
#include "game/008EAFB0.inc"
};
char gMsgEventLines_72_0[160] = {
#include "game/008EB030.inc"
};
char gMsgEventLines_72_1[177] = {
#include "game/008EB0D0.inc"
};
char gMsgEventLines_72_2[196] = {
#include "game/008EB190.inc"
};
char gMsgEventLines_72_3[157] = {
#include "game/008EB260.inc"
};
char gMsgEventLines_73_0[215] = {
#include "game/008EB300.inc"
};
char gMsgEventLines_73_1[86] = {
#include "game/008EB3E0.inc"
};
char gMsgEventLines_73_2[178] = {
#include "game/008EB440.inc"
};
char gMsgEventLines_73_3[107] = {
#include "game/008EB500.inc"
};
char gMsgEventLines_74_0[96] = {
#include "game/008EB570.inc"
};
char gMsgEventLines_74_1[141] = {
#include "game/008EB5D0.inc"
};
char gMsgEventLines_74_2[135] = {
#include "game/008EB660.inc"
};
char gMsgEventLines_74_3[123] = {
#include "game/008EB6F0.inc"
};
char gMsgEventLines_74_4[133] = {
#include "game/008EB770.inc"
};
char gMsgEventLines_75_0[73] = {
#include "game/008EB800.inc"
};
char gMsgEventLines_75_1[157] = {
#include "game/008EB850.inc"
};
char gMsgEventLines_75_2[123] = {
#include "game/008EB8F0.inc"
};
char gMsgEventLines_75_3[103] = {
#include "game/008EB970.inc"
};
char gMsgEventLines_75_4[52] = {
#include "game/008EB9E0.inc"
};
char gMsgEventLines_75_5[112] = {
#include "game/008EBA20.inc"
};
char gMsgEventLines_76_0[113] = {
#include "game/008EBA90.inc"
};
char gMsgEventLines_76_1[141] = {
#include "game/008EBB10.inc"
};
char gMsgEventLines_76_2[195] = {
#include "game/008EBBA0.inc"
};
char gMsgEventLines_76_3[192] = {
#include "game/008EBC70.inc"
};
char gMsgEventLines_76_4[149] = {
#include "game/008EBD30.inc"
};
char gMsgEventLines_76_5[178] = {
#include "game/008EBDD0.inc"
};
char gMsgEventLines_76_6[187] = {
#include "game/008EBE90.inc"
};
char gMsgEventLines_76_7[206] = {
#include "game/008EBF50.inc"
};
char gMsgEventLines_76_8[121] = {
#include "game/008EC020.inc"
};
#pragma explicit_zero_data on
char gMsgEventListItems[1] = {
#include "game/008EC0A0.inc"
};
#pragma explicit_zero_data reset
#pragma rodata off

/* The texts of the advice window's frame, by their index in gMsgEventTexts. */
enum MsgEventText {
    MSG_EVENT_TEXT_POPUP_HINT = 0,  /* the popup's button hint */
    MSG_EVENT_TEXT_UNSEEN = 1,      /* the list's placeholder for a message not seen yet */
    MSG_EVENT_TEXT_LIST_HELP = 2,   /* the help line under the list */
    MSG_EVENT_TEXT_LIST_HINT = 3,   /* the button hint of a message opened from the list */
    MSG_EVENT_TEXT_COLUMN_FMT = 4,  /* the list's column counter: column, columns */
    MSG_EVENT_TEXT_PAGE_FMT = 5     /* a message's page counter: page, pages */
};
/*
 * Draws the message list window at its slide offset. In the list: the quest's
 * title (MsgEvent_CopyQuestTitle) centred, the column counter when there is
 * more than one column, the help line and the current column's seven titles,
 * a grey placeholder for a message not seen yet; in a message: the message as
 * the popup draws it, with the list's button hint. Nothing once the window
 * has closed.
 */
void MsgEvent_DrawList(void)
{
    s16 *slideOfs;
    s8 questTitle[32];
    s32 i;
    s16 rowY;
    u8 k;

    slideOfs = Shake_GetOffset(7);
    switch (gQuestMsgWindow->listStep) {
    case 0:
    case 1:
        switch (gQuestMsgWindow->listSub) {
        case 0:
        case 2:
            ListPanel_Draw(&gMsgEventListPanel, slideOfs, gMsgEventListItems, gQuestMsgWindow->grid.cols != 0 ? gQuestMsgWindow->grid.row : -1);
            MsgEvent_CopyQuestTitle(questTitle);
            i = Str_Length(questTitle);
            Font_SetColor(0x807F7F28);
            Font_SetPos(320 - i * 9 / 2, slideOfs[1] + 120);
            Text_Draw(questTitle);
            Font_SetColor(0x807F7F7F);
            if (gQuestMsgWindow->grid.cols > 1) {
                Font_SetPos(460, slideOfs[1] + 120);
                Text_Printf(gMsgEventTexts[MSG_EVENT_TEXT_COLUMN_FMT], gQuestMsgWindow->grid.col + 1, gQuestMsgWindow->grid.cols);
                Menu_DrawFrame(&gMsgEventListPanel, slideOfs);
            }
            Font_SetPos(382, slideOfs[1] + 296);
            Text_Printf(gMsgEventTexts[MSG_EVENT_TEXT_LIST_HELP]);
            rowY = slideOfs[1] + 142;
            k = gQuestMsgWindow->grid.col * 7;
            for (i = 0; i < 7; i++) {
                Font_SetPos(122, rowY);
                if (MsgEvent_IsSeen(gQuestMsgWindow->set, k) == 1) {
                    Font_SetColor(0x807F7F7F);
                    Text_PrintfColored(MsgEvent_Title(gQuestMsgWindow->set, k));
                } else {
                    Font_SetColor(0x80505050);
                    Text_Draw((s8 *)gMsgEventTexts[MSG_EVENT_TEXT_UNSEEN]);
                }
                k++;
                if (gQuestMsgWindow->count <= k) {
                    break;
                }
                rowY += 22;
            }
            break;
        case 1:
            Window_DrawText(&gMsgEventPopupWin, slideOfs, 0);
            MsgEvent_DrawMessage(1, (s32) slideOfs);
            MsgEvent_DrawIcon(0, (s32) slideOfs);
            break;
        }
        break;
    case 2:
        break;
    }
}

/*
 * Draws the message shown (QuestMsgWin.msg) in a window at offset windowOfs
 * (an s16 x, y pair passed as an s32): its title centred, the current page's
 * text, the page counter (MsgEvent_DrawPageCounter) and the button hint, the
 * popup's when inList is 0, the list's when it is 1. The first strlen call's
 * result is unused, as in the game's code.
 */
void MsgEvent_DrawMessage(u8 inList, s32 windowOfs)
{
    char *title;

    Font_SetSpacing(18, 18);
    Font_SetColor(0x802B7F2B);
    title = MsgEvent_Title(gQuestMsgWindow->set, gQuestMsgWindow->msg);
    strlen(title);
    Font_SetPos(320 - strlen(title) * 9 / 2, *(s16 *) (windowOfs + 2) + 129);
    Text_PrintfColored(title);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(122, *(s16 *) (windowOfs + 2) + 160);
    Text_PrintfStyled(3, MsgEvent_Line(gQuestMsgWindow->set, gQuestMsgWindow->msg, gQuestMsgWindow->page));
    MsgEvent_DrawPageCounter(298, *(s16 *) (windowOfs + 2) + 280);
    Font_SetPos(455, *(s16 *) (windowOfs + 2) + 280);
    if (inList == 0) {
        Text_Printf(gMsgEventTexts[MSG_EVENT_TEXT_POPUP_HINT]);
    } else {
        Text_Printf(gMsgEventTexts[MSG_EVENT_TEXT_LIST_HINT]);
    }
}

/*
 * Draws the page counter at (x, y), the page shown and the page count, and
 * for a message of more than one page the arrows to the previous and the next
 * page (each a dark and a bright sprite), 3 or 8 pixels out by bit 4 of
 * gFrameTick so that they pulse.
 */
void MsgEvent_DrawPageCounter(s16 x, s16 y)
{
    SprRect arrow;
    s16 pulse;
    int flash;
    s16 *arrowY;
    s32 *arrowColor;
    s16 *uvStart;
    s16 *uvEnd;

    Font_SetColor(0x807F7F7F);
    Font_SetPos(x, y);
    Text_PrintfColored(gMsgEventTexts[MSG_EVENT_TEXT_PAGE_FMT], gQuestMsgWindow->page + 1, gQuestMsgWindow->pages);
    if (gQuestMsgWindow->pages > 1) {
        flash = (gFrameTick & 0x10) == 0 ? (s16) 3 : (s16) 8;
        arrow.w = 18;
        arrow.h = 18;
        uvStart = (s16 *) &arrow.uv0;
        *(s32 *) uvStart = 0x6E0071;
        uvEnd = (s16 *) &arrow.uv1;
        *(s32 *) uvEnd = 0x7D007D;
        pulse = flash;
        if (gQuestMsgWindow->page != 0) {
            arrow.x = (s16) (x - 18) - pulse + 1;
            arrowY = &arrow.y;
            arrowColor = &arrow.color;
            *arrowY = y;
            *arrowColor = 0xFF203020;
            Sprite_Draw((struct Sprite *)&arrow);
            arrow.x -= 2;
            *arrowY -= 1;
            *arrowColor = 0xFF18FF20;
            Sprite_Draw((struct Sprite *)&arrow);
        }
        if (gQuestMsgWindow->page != gQuestMsgWindow->pages - 1) {
            *uvStart = 124;
            *uvEnd = 112;
            arrowY = &arrow.y;
            arrowColor = &arrow.color;
            arrow.x = (s16) (x - 18) + pulse + 66;
            *arrowY = y;
            *arrowColor = 0xFF203020;
            Sprite_Draw((struct Sprite *)&arrow);
            arrow.x -= 2;
            *arrowY -= 1;
            *arrowColor = 0xFF18FF20;
            Sprite_Draw((struct Sprite *)&arrow);
        }
    }
}

/*
 * Draws the message windows' icon (texture TEX_SLOT_PICTURES), which moves with the
 * window's slide (windowOfs, an s16 x, y pair passed as an s32);
 * gMsgEventSetIcon picks one of two pictures by the message set. The first
 * parameter is unused.
 */
void MsgEvent_DrawIcon(s32 unused, s32 windowOfs)
{
    SprRect sprite;

    Tex_Prepare(1, TEX_SLOT_PICTURES);
    Tex_Bind(TEX_SLOT_PICTURES);
    sprite.x = 288;
    sprite.y = 56 - (s16) (s32) (0.37272727f * *(s16 *) (windowOfs + 2));
    sprite.w = 64;
    sprite.h = 64;
    sprite.color = -1;
    if (gMsgEventSetIcon[(s8) gQuestMsgWindow->set] == 0) {
        sprite.uv0 = 0xD3007F;
        sprite.uv1 = 0xFB00A7;
    } else {
        sprite.uv0 = 0xD30055;
        sprite.uv1 = 0xFB007D;
    }
    Sprite_Draw((struct Sprite *)&sprite);
}

/*
 * Copies into buf the first line of the quest's text 0 (QuestInfo_Get; likely
 * the quest's name), at most 30 characters, and ends it with a 0.
 */
void MsgEvent_CopyQuestTitle(s8 *buf)
{
    s32 left;
    s8 *info;
    s8 *dst;
    s8 letter;

    dst = buf;
    info = QuestInfo_Get(0);
    left = 30;
loop_1:
    letter = *info;
    if (letter == 10) {
        *dst = 0;
        return;
    }
    *dst = letter;
    if (*info != 0) {
        left -= 1;
        info += 1;
        dst += 1;
        if (left <= 0) {
            *dst = 0;
        } else {
            goto loop_1;
        }
    }
}

/* The field menu's draw entry for the message list: MsgEvent_DrawList. */
void MsgEvent_Draw(void)
{
    MsgEvent_DrawList();
}

s32 MsgEvent_CheckList(MsgEvent *e);
/*
 * Runs message event kind (0 to 18; what fires each is given with the kinds)
 * and returns 1 when it chose a message: the event sets QuestMsgWin.request,
 * and the message goes to the popup that MsgEvent_OpenPopup opens. Nothing
 * fires online, in a quest without advice (Quest_HasAdvice) or once the quest
 * is over; each event fires once per save, as an event flag records it.
 */
s32 MsgEvent_Check(u16 kind)
{
    if (Game_IsOnline() == 1) {
        return 0;
    }
    if (Quest_HasAdvice() != 1) {
        return 0;
    }
    if (Quest_PhaseIsOver(0) == 1) {
        return 0;
    }
    gQuestMsgWindow->request = -1;
    switch (kind) {
    case MSGEV_QUEST_START:
        MsgEvent_CheckList(gMsgEventsQuestStart);
        break;
    case MSGEV_SUPPLY_BOX:
        MsgEvent_CheckList(gMsgEventsSupplyBox);
        break;
    case MSGEV_SUPPLY_BOX_CLOSED:
        MsgEvent_CheckSupplyBoxClosed();
        break;
    case MSGEV_DELIVERY_BOX:
        MsgEvent_CheckQuest(203, 1, 3);
        break;
    case MSGEV_BED:
        MsgEvent_CheckQuest(204, 1, 4);
        break;
    case MSGEV_JUNGLE_1:
        MsgEvent_CheckList(gMsgEventsJungle1);
        break;
    case MSGEV_JUNGLE_2:
        MsgEvent_CheckQuest(220, 3, 4);
        break;
    case MSGEV_JUNGLE_3:
        MsgEvent_CheckQuest(219, 3, 3);
        break;
    case MSGEV_JUNGLE_4:
        MsgEvent_CheckList(gMsgEventsJungle4);
        break;
    case MSGEV_JUNGLE_8:
        MsgEvent_CheckQuest(263, 17, 1);
        break;
    case MSGEV_JUNGLE_6:
        MsgEvent_CheckQuest(226, 5, 1);
        break;
    case MSGEV_FORTRESS_0:
        MsgEvent_CheckQuest(283, QUEST_ID_FORTRESS_STORY, 0);
        break;
    case MSGEV_APCEROS_HUNTED:
        MsgEvent_CheckList(gMsgEventsApcerosHunted);
        break;
    case MSGEV_MINING:
        MsgEvent_CheckQuest(207, 1, 7);
        break;
    case MSGEV_CAMP:
        MsgEvent_CheckList(gMsgEventsCamp);
        break;
    case MSGEV_GOAL_DELIVERED:
        MsgEvent_CheckList(gMsgEventsGoalDelivered);
        break;
    case MSGEV_CARRY_ITEM:
        MsgEvent_CheckQuest(264, 17, 2);
        break;
    case MSGEV_GOAL_KILLED:
        MsgEvent_CheckQuest(275, 18, 9);
        break;
    case MSGEV_GOAL_CAPTURED:
        MsgEvent_CheckQuest(276, 18, 10);
        break;
    default:
        return 0;
    }
    if (gQuestMsgWindow->request >= 0) {
        gQuestMsgWindow->popupReq = 1;
        gQuestMsgWindow->msg = gQuestMsgWindow->request;
        return 1;
    }
    return 0;
}

/*
 * Looks the current quest (gQuest.stage) up in the dated event list e (sorted
 * by quest id, ended by a negative flag): when its entry has not fired yet,
 * sets the entry's event flag, requests its message, sets the seen flags (200
 * + n) of the up to three messages it unlocks and returns 1; else returns 0.
 */
s32 MsgEvent_CheckList(MsgEvent *entry)
{
    s32 curQuest;
    s16 flag = entry->flag;

    if (flag >= 0) {
        curQuest = gQuest.stage;
        do {
            if (entry->questId >= curQuest) {
                if (entry->questId == curQuest && Save_TestEventFlag(flag) != 1) {
                    Save_SetEventFlag(entry->flag);
                    gQuestMsgWindow->request = entry->msg;
                    if (entry->ev[0] > 0) {
                        Save_SetEventFlag(entry->ev[0] + MSGEV_SEEN_FLAG_BASE);
                    }
                    if (entry->ev[1] > 0) {
                        Save_SetEventFlag(entry->ev[1] + MSGEV_SEEN_FLAG_BASE);
                    }
                    if (entry->ev[2] > 0) {
                        Save_SetEventFlag(entry->ev[2] + MSGEV_SEEN_FLAG_BASE);
                    }
                    return 1;
                }
                break;
            }
            entry++;
            flag = entry->flag;
        } while (flag >= 0);
    }
    return 0;
}

/*
 * In quest questId, when event flag flag is not set yet: sets it, requests
 * message msgNo and returns 1; else returns 0.
 */
s32 MsgEvent_CheckQuest(s32 flag, s32 questId, s8 msgNo)
{
    s32 flagNo;

    if ((questId & 0xFFFF) == gQuest.stage) {
        flagNo = flag & 0xFFFF;
        if (Save_TestEventFlag(flagNo) != 1) {
            Save_SetEventFlag(flagNo);
            gQuestMsgWindow->request = msgNo;
            return 1;
        }
    }
    return 0;
}

/*
 * Message event 2, when the player closes the supply box: in quest 1 requests
 * message 2 once (event flag 202); in quest 18 requests message 1 once and
 * sets event flags 267 to 274. Returns 1 when it requested a message.
 */
s32 MsgEvent_CheckSupplyBoxClosed(void)
{
    switch (gQuest.stage) {
    case 1:
        if (Save_TestEventFlag(202) != 1) {
            Save_SetEventFlag(202);
            gQuestMsgWindow->request = 2;
            return 1;
        }
        break;
    case QUEST_ID_VELOCIDROME_SCENE_1:
        if (Save_TestEventFlag(267) != 1) {
            Save_SetEventFlag(267);
            Save_SetEventFlag(268);
            Save_SetEventFlag(269);
            Save_SetEventFlag(270);
            Save_SetEventFlag(271);
            Save_SetEventFlag(272);
            Save_SetEventFlag(273);
            Save_SetEventFlag(274);
            gQuestMsgWindow->request = 1;
            return 1;
        }
        break;
    }
    return 0;
}

/*
 * Returns the number of messages of set msgSet (the second half of its row of
 * gMsgEventSets).
 */
u8 MsgEvent_CountInSet(u8 msgSet)
{
    return gMsgEventSets[msgSet & 0xFF][1];
}

/*
 * Returns 1 when message msgNo of set msgSet was seen: its seen flag, the
 * set's first flag (gMsgEventSets) plus msgNo, is set.
 */
s32 MsgEvent_IsSeen(u8 msgSet, u8 msgNo)
{
    return Save_TestEventFlag(msgNo + gMsgEventSets[msgSet][0]);
}

/*
 * Returns the title of message msgNo of set msgSet: its entry in
 * gMsgEventSetEntries gives its text's index in gMsgEventTitles.
 */
char *MsgEvent_Title(u8 msgSet, u8 msgNo)
{
    u8 *entry = gMsgEventSetEntries[msgSet];
    entry += msgNo + msgNo;
    return gMsgEventTitles[*entry];
}

/*
 * Returns page pageNo of message msgNo of set msgSet (gMsgEventLines, by its
 * entry's text index).
 */
char *MsgEvent_Line(u8 msgSet, u8 msgNo, u8 pageNo)
{
    u8 *entry = gMsgEventSetEntries[msgSet];
    entry += msgNo + msgNo;
    return gMsgEventLines[*entry][pageNo];
}

/*
 * Returns the page count of message msgNo of set msgSet (the second byte of
 * its entry in gMsgEventSetEntries).
 */
u8 MsgEvent_PageCount(u8 msgSet, u8 msgNo)
{
    u8 *entry = gMsgEventSetEntries[msgSet];
    entry += msgNo + msgNo;
    return entry[1];
}

/* This file's initialised data. */
char *gMsgEventTitles[80] = {
    (char *)gMsgEventTitles_0, (char *)gMsgEventTitles_1, (char *)gMsgEventTitles_2, (char *)gMsgEventTitles_3,
    (char *)gMsgEventTitles_4, (char *)gMsgEventTitles_5, (char *)gMsgEventTitles_6, (char *)gMsgEventTitles_7,
    (char *)gMsgEventTitles_8, (char *)gMsgEventTitles_9, (char *)gMsgEventTitles_10, (char *)gMsgEventTitles_11,
    (char *)gMsgEventTitles_12, (char *)gMsgEventTitles_13, (char *)gMsgEventTitles_14, (char *)gMsgEventTitles_15,
    (char *)gMsgEventTitles_16, (char *)gMsgEventTitles_17, (char *)gMsgEventTitles_18, (char *)gMsgEventTitles_19,
    (char *)gMsgEventTitles_20, (char *)gMsgEventTitles_21, (char *)gMsgEventTitles_22, (char *)gMsgEventTitles_23,
    (char *)gMsgEventTitles_24, (char *)gMsgEventTitles_25, (char *)gMsgEventTitles_26, (char *)gMsgEventTitles_27,
    (char *)gMsgEventTitles_28, (char *)gMsgEventTitles_29, (char *)gMsgEventTitles_30, (char *)gMsgEventTitles_31,
    (char *)gMsgEventTitles_32, (char *)gMsgEventTitles_33, (char *)gMsgEventTitles_34, (char *)gMsgEventTitles_35,
    (char *)gMsgEventTitles_36, (char *)gMsgEventTitles_37, (char *)gMsgEventTitles_38, (char *)gMsgEventTitles_39,
    (char *)gMsgEventTitles_40, (char *)gMsgEventTitles_41, (char *)gMsgEventTitles_42, (char *)gMsgEventTitles_43,
    (char *)gMsgEventTitles_44, (char *)gMsgEventTitles_45, (char *)gMsgEventTitles_46, (char *)gMsgEventTitles_47,
    (char *)gMsgEventTitles_48, (char *)gMsgEventTitles_49, (char *)gMsgEventTitles_50, (char *)gMsgEventTitles_51,
    (char *)gMsgEventTitles_52, (char *)gMsgEventTitles_53, (char *)gMsgEventTitles_54, (char *)gMsgEventTitles_55,
    (char *)gMsgEventTitles_56, (char *)gMsgEventTitles_57, (char *)gMsgEventTitles_58, (char *)gMsgEventTitles_59,
    (char *)gMsgEventTitles_60, (char *)gMsgEventTitles_61, (char *)gMsgEventTitles_62, (char *)gMsgEventTitles_63,
    (char *)gMsgEventTitles_64, (char *)gMsgEventTitles_65, (char *)D_008DEA50, (char *)D_008DEA50,
    (char *)gMsgEventTitles_68, (char *)gMsgEventTitles_69, (char *)gMsgEventTitles_70, (char *)gMsgEventTitles_71,
    (char *)gMsgEventTitles_72, (char *)gMsgEventTitles_73, (char *)gMsgEventTitles_74, (char *)gMsgEventTitles_75,
    (char *)gMsgEventTitles_76, NULL, NULL, NULL,
};
char *gMsgEventLines_0[6] = {
    (char *)gMsgEventLines_0_0, (char *)gMsgEventLines_0_1, (char *)gMsgEventLines_0_2, (char *)gMsgEventLines_0_3,
    (char *)gMsgEventLines_0_4, NULL,
};
char *gMsgEventLines_1[3] = {
    (char *)gMsgEventLines_1_0, (char *)gMsgEventLines_1_1, (char *)gMsgEventLines_1_2,
};
char *gMsgEventLines_2[4] = {
    (char *)gMsgEventLines_2_0, (char *)gMsgEventLines_2_1, (char *)gMsgEventLines_2_2, (char *)gMsgEventLines_2_3,
};
char *gMsgEventLines_3[4] = {
    (char *)gMsgEventLines_3_0, (char *)gMsgEventLines_3_1, (char *)gMsgEventLines_3_2, NULL,
};
char *gMsgEventLines_4[4] = {
    (char *)gMsgEventLines_4_0, (char *)gMsgEventLines_4_1, (char *)gMsgEventLines_4_2, NULL,
};
char *gMsgEventLines_5[4] = {
    (char *)gMsgEventLines_5_0, (char *)gMsgEventLines_5_1, (char *)gMsgEventLines_5_2, (char *)gMsgEventLines_5_3,
};
char *gMsgEventLines_6[4] = {
    (char *)gMsgEventLines_6_0, (char *)gMsgEventLines_6_1, (char *)gMsgEventLines_6_2, NULL,
};
char *gMsgEventLines_7[8] = {
    (char *)gMsgEventLines_7_0, (char *)gMsgEventLines_7_1, (char *)gMsgEventLines_7_2, (char *)gMsgEventLines_7_3,
    (char *)gMsgEventLines_7_4, (char *)gMsgEventLines_7_5, (char *)gMsgEventLines_7_6, NULL,
};
char *gMsgEventLines_8[4] = {
    (char *)gMsgEventLines_8_0, (char *)gMsgEventLines_8_1, (char *)gMsgEventLines_8_2, (char *)gMsgEventLines_8_3,
};
char *gMsgEventLines_9[4] = {
    (char *)gMsgEventLines_9_0, NULL, NULL, NULL,
};
char *gMsgEventLines_10[8] = {
    (char *)gMsgEventLines_10_0, (char *)gMsgEventLines_10_1, (char *)gMsgEventLines_10_2, (char *)gMsgEventLines_10_3,
    (char *)gMsgEventLines_10_4, NULL, NULL, NULL,
};
char *gMsgEventLines_11[8] = {
    (char *)gMsgEventLines_11_0, (char *)gMsgEventLines_11_1, (char *)gMsgEventLines_11_2, (char *)gMsgEventLines_11_3,
    (char *)gMsgEventLines_11_4, NULL, NULL, NULL,
};
char *gMsgEventLines_12[8] = {
    (char *)gMsgEventLines_12_0, (char *)gMsgEventLines_12_1, (char *)gMsgEventLines_12_2, (char *)gMsgEventLines_12_3,
    (char *)gMsgEventLines_12_4, (char *)gMsgEventLines_12_5, NULL, NULL,
};
char *gMsgEventLines_13[8] = {
    (char *)gMsgEventLines_13_0, (char *)gMsgEventLines_13_1, (char *)gMsgEventLines_13_2, (char *)gMsgEventLines_13_3,
    (char *)gMsgEventLines_13_4, NULL, NULL, NULL,
};
char *gMsgEventLines_14[4] = {
    (char *)gMsgEventLines_14_0, (char *)gMsgEventLines_14_1, (char *)gMsgEventLines_14_2, (char *)gMsgEventLines_14_3,
};
char *gMsgEventLines_15[2] = {
    (char *)gMsgEventLines_15_0, NULL,
};
char *gMsgEventLines_16[3] = {
    (char *)gMsgEventLines_16_0, (char *)gMsgEventLines_16_1, (char *)gMsgEventLines_16_2,
};
char *gMsgEventLines_17[4] = {
    (char *)gMsgEventLines_17_0, (char *)gMsgEventLines_17_1, (char *)gMsgEventLines_17_2, (char *)gMsgEventLines_17_3,
};
char *gMsgEventLines_18[8] = {
    (char *)gMsgEventLines_18_0, (char *)gMsgEventLines_18_1, (char *)gMsgEventLines_18_2, (char *)gMsgEventLines_18_3,
    (char *)gMsgEventLines_18_4, NULL, NULL, NULL,
};
char *gMsgEventLines_19[4] = {
    (char *)gMsgEventLines_19_0, (char *)gMsgEventLines_19_1, (char *)gMsgEventLines_19_2, (char *)gMsgEventLines_19_3,
};
char *gMsgEventLines_20[6] = {
    (char *)gMsgEventLines_20_0, (char *)gMsgEventLines_20_1, (char *)gMsgEventLines_20_2, (char *)gMsgEventLines_20_3,
    (char *)gMsgEventLines_20_4, NULL,
};
char *gMsgEventLines_21[2] = {
    (char *)gMsgEventLines_21_0, NULL,
};
char *gMsgEventLines_22[6] = {
    (char *)gMsgEventLines_22_0, (char *)gMsgEventLines_22_1, (char *)gMsgEventLines_22_2, (char *)gMsgEventLines_22_3,
    (char *)gMsgEventLines_22_4, NULL,
};
char *gMsgEventLines_23[3] = {
    (char *)gMsgEventLines_23_0, (char *)gMsgEventLines_23_1, (char *)gMsgEventLines_23_2,
};
char *gMsgEventLines_24[2] = {
    (char *)gMsgEventLines_24_0, NULL,
};
char *gMsgEventLines_25[8] = {
    (char *)gMsgEventLines_25_0, (char *)gMsgEventLines_25_1, (char *)gMsgEventLines_25_2, (char *)gMsgEventLines_25_3,
    (char *)gMsgEventLines_25_4, NULL, NULL, NULL,
};
char *gMsgEventLines_26[6] = {
    (char *)gMsgEventLines_26_0, (char *)gMsgEventLines_26_1, (char *)gMsgEventLines_26_2, (char *)gMsgEventLines_26_3,
    (char *)gMsgEventLines_26_4, (char *)gMsgEventLines_26_5,
};
char *gMsgEventLines_27[2] = {
    (char *)gMsgEventLines_27_0, NULL,
};
char *gMsgEventLines_28[6] = {
    (char *)gMsgEventLines_28_0, (char *)D_008E37F0, (char *)gMsgEventLines_28_2, (char *)gMsgEventLines_28_3,
    (char *)gMsgEventLines_28_4, (char *)gMsgEventLines_28_5,
};
char *gMsgEventLines_29[3] = {
    (char *)D_008E3B70, (char *)D_008E3C10, (char *)gMsgEventLines_29_2,
};
char *gMsgEventLines_30[6] = {
    (char *)gMsgEventLines_30_0, (char *)D_008E37F0, (char *)gMsgEventLines_30_2, (char *)gMsgEventLines_30_3,
    (char *)gMsgEventLines_30_4, NULL,
};
char *gMsgEventLines_31[3] = {
    (char *)gMsgEventLines_31_0, (char *)gMsgEventLines_31_1, (char *)gMsgEventLines_31_2,
};
char *gMsgEventLines_32[2] = {
    (char *)gMsgEventLines_32_0, (char *)gMsgEventLines_32_1,
};
char *gMsgEventLines_33[6] = {
    (char *)gMsgEventLines_33_0, (char *)D_008E37F0, (char *)gMsgEventLines_33_2, (char *)gMsgEventLines_33_3,
    (char *)gMsgEventLines_33_4, (char *)gMsgEventLines_33_5,
};
char *gMsgEventLines_34[3] = {
    (char *)gMsgEventLines_34_0, (char *)gMsgEventLines_34_1, (char *)gMsgEventLines_34_2,
};
char *gMsgEventLines_35[8] = {
    (char *)gMsgEventLines_35_0, (char *)D_008E37F0, (char *)gMsgEventLines_35_2, (char *)gMsgEventLines_35_3,
    (char *)gMsgEventLines_35_4, (char *)gMsgEventLines_35_5, NULL, NULL,
};
char *gMsgEventLines_36[4] = {
    (char *)D_008E3B70, (char *)D_008E3C10, (char *)gMsgEventLines_36_2, (char *)gMsgEventLines_36_3,
};
char *gMsgEventLines_37[6] = {
    (char *)gMsgEventLines_37_0, (char *)D_008E37F0, (char *)gMsgEventLines_37_2, (char *)gMsgEventLines_37_3,
    (char *)gMsgEventLines_37_4, (char *)gMsgEventLines_37_5,
};
char *gMsgEventLines_38[3] = {
    (char *)gMsgEventLines_38_0, (char *)gMsgEventLines_38_1, (char *)gMsgEventLines_38_2,
};
char *gMsgEventLines_39[8] = {
    (char *)gMsgEventLines_39_0, (char *)D_008E37F0, (char *)gMsgEventLines_39_2, (char *)gMsgEventLines_39_3,
    (char *)gMsgEventLines_39_4, NULL, NULL, NULL,
};
char *gMsgEventLines_40[4] = {
    (char *)gMsgEventLines_40_0, (char *)gMsgEventLines_40_1, (char *)gMsgEventLines_40_2, (char *)gMsgEventLines_40_3,
};
char *gMsgEventLines_41[8] = {
    (char *)gMsgEventLines_41_0, (char *)D_008E37F0, (char *)gMsgEventLines_41_2, (char *)gMsgEventLines_41_3,
    (char *)gMsgEventLines_41_4, NULL, NULL, NULL,
};
char *gMsgEventLines_42[4] = {
    (char *)gMsgEventLines_42_0, (char *)gMsgEventLines_42_1, (char *)gMsgEventLines_42_2, (char *)gMsgEventLines_42_3,
};
char *gMsgEventLines_43[8] = {
    (char *)gMsgEventLines_43_0, (char *)D_008E37F0, (char *)gMsgEventLines_43_2, (char *)gMsgEventLines_43_3,
    (char *)gMsgEventLines_43_4, (char *)gMsgEventLines_43_5, NULL, NULL,
};
char *gMsgEventLines_44[4] = {
    (char *)gMsgEventLines_44_0, (char *)gMsgEventLines_44_1, (char *)gMsgEventLines_44_2, (char *)gMsgEventLines_44_3,
};
char *gMsgEventLines_45[4] = {
    (char *)gMsgEventLines_45_0, (char *)gMsgEventLines_45_1, (char *)gMsgEventLines_45_2, (char *)gMsgEventLines_45_3,
};
char *gMsgEventLines_46[8] = {
    (char *)gMsgEventLines_46_0, (char *)D_008E6C90, (char *)gMsgEventLines_46_2, (char *)gMsgEventLines_46_3,
    (char *)gMsgEventLines_46_4, NULL, NULL, NULL,
};
char *gMsgEventLines_47[4] = {
    (char *)gMsgEventLines_47_0, (char *)gMsgEventLines_47_1, (char *)gMsgEventLines_47_2, (char *)D_008E7130,
};
char *gMsgEventLines_48[8] = {
    (char *)gMsgEventLines_48_0, (char *)D_008E6C90, (char *)gMsgEventLines_48_2, (char *)gMsgEventLines_48_3,
    (char *)gMsgEventLines_48_4, NULL, NULL, NULL,
};
char *gMsgEventLines_49[4] = {
    (char *)gMsgEventLines_49_0, (char *)gMsgEventLines_49_1, (char *)gMsgEventLines_49_2, (char *)D_008E7130,
};
char *gMsgEventLines_50[6] = {
    (char *)gMsgEventLines_50_0, (char *)gMsgEventLines_50_1, (char *)gMsgEventLines_50_2, (char *)gMsgEventLines_50_3,
    (char *)gMsgEventLines_50_4, NULL,
};
char *gMsgEventLines_51[2] = {
    (char *)gMsgEventLines_51_0, (char *)gMsgEventLines_51_1,
};
char *gMsgEventLines_52[4] = {
    (char *)gMsgEventLines_52_0, (char *)gMsgEventLines_52_1, (char *)gMsgEventLines_52_2, (char *)gMsgEventLines_52_3,
};
char *gMsgEventLines_53[4] = {
    (char *)gMsgEventLines_53_0, (char *)gMsgEventLines_53_1, (char *)gMsgEventLines_53_2, NULL,
};
char *gMsgEventLines_54[4] = {
    (char *)gMsgEventLines_54_0, (char *)gMsgEventLines_54_1, (char *)gMsgEventLines_54_2, NULL,
};
char *gMsgEventLines_55[6] = {
    D_008E8240, D_008E8330, (char *)gMsgEventLines_55_0, (char *)gMsgEventLines_55_1,
    (char *)gMsgEventLines_55_2, NULL,
};
char *gMsgEventLines_56[2] = {
    (char *)gMsgEventLines_56_0, NULL,
};
char *gMsgEventLines_57[8] = {
    (char *)gMsgEventLines_57_0, (char *)gMsgEventLines_57_1, (char *)gMsgEventLines_57_2, (char *)gMsgEventLines_57_3,
    (char *)gMsgEventLines_57_4, (char *)gMsgEventLines_57_5, (char *)gMsgEventLines_57_6, NULL,
};
char *gMsgEventLines_58[4] = {
    (char *)gMsgEventLines_58_0, (char *)gMsgEventLines_58_1, (char *)gMsgEventLines_58_2, NULL,
};
char *gMsgEventLines_59[4] = {
    D_008E8E70, D_008E8F20, (char *)gMsgEventLines_59_0, (char *)gMsgEventLines_59_1,
};
char *gMsgEventLines_60[2] = {
    D_008E91C0, (char *)gMsgEventLines_60_0,
};
char *gMsgEventLines_61[3] = {
    D_008E9370, D_008E9420, D_008E94D0,
};
char *gMsgEventLines_62[2] = {
    (char *)gMsgEventLines_62_0, (char *)gMsgEventLines_62_1,
};
char *gMsgEventLines_63[2] = {
    (char *)gMsgEventLines_63_0, (char *)gMsgEventLines_63_1,
};
char *gMsgEventLines_64[2] = {
    (char *)gMsgEventLines_64_0, (char *)gMsgEventLines_64_1,
};
char *gMsgEventLines_65[6] = {
    (char *)gMsgEventLines_65_0, (char *)gMsgEventLines_65_1, (char *)gMsgEventLines_65_2, (char *)gMsgEventLines_65_3,
    (char *)gMsgEventLines_65_4, NULL,
};
char *gMsgEventLines_66[2] = {
    (char *)gMsgEventLines_66_0, NULL,
};
char *gMsgEventLines_67[2] = {
    (char *)gMsgEventLines_67_0, NULL,
};
char *gMsgEventLines_68[3] = {
    D_008E9F70, (char *)gMsgEventLines_68_0, (char *)gMsgEventLines_68_1,
};
char *gMsgEventLines_69[8] = {
    (char *)gMsgEventLines_69_0, (char *)gMsgEventLines_69_1, (char *)gMsgEventLines_69_2, (char *)gMsgEventLines_69_3,
    (char *)gMsgEventLines_69_4, NULL, NULL, NULL,
};
char *gMsgEventLines_70[8] = {
    (char *)gMsgEventLines_70_0, (char *)gMsgEventLines_70_1, (char *)gMsgEventLines_70_2, (char *)gMsgEventLines_70_3,
    (char *)gMsgEventLines_70_4, (char *)gMsgEventLines_70_5, (char *)gMsgEventLines_70_6, (char *)gMsgEventLines_70_7,
};
char *gMsgEventLines_71[8] = {
    (char *)gMsgEventLines_71_0, (char *)gMsgEventLines_71_1, (char *)gMsgEventLines_71_2, (char *)gMsgEventLines_71_3,
    (char *)gMsgEventLines_71_4, (char *)gMsgEventLines_71_5, (char *)gMsgEventLines_71_6, (char *)gMsgEventLines_71_7,
};
char *gMsgEventLines_72[4] = {
    (char *)gMsgEventLines_72_0, (char *)gMsgEventLines_72_1, (char *)gMsgEventLines_72_2, (char *)gMsgEventLines_72_3,
};
char *gMsgEventLines_73[4] = {
    (char *)gMsgEventLines_73_0, (char *)gMsgEventLines_73_1, (char *)gMsgEventLines_73_2, (char *)gMsgEventLines_73_3,
};
char *gMsgEventLines_74[8] = {
    (char *)gMsgEventLines_74_0, (char *)gMsgEventLines_74_1, (char *)gMsgEventLines_74_2, (char *)gMsgEventLines_74_3,
    (char *)gMsgEventLines_74_4, NULL, NULL, NULL,
};
char *gMsgEventLines_75[8] = {
    (char *)gMsgEventLines_75_0, (char *)gMsgEventLines_75_1, (char *)gMsgEventLines_75_2, (char *)gMsgEventLines_75_3,
    (char *)gMsgEventLines_75_4, (char *)gMsgEventLines_75_5, NULL, NULL,
};
char *gMsgEventLines_76[12] = {
    (char *)gMsgEventLines_76_0, (char *)gMsgEventLines_76_1, (char *)gMsgEventLines_76_2, (char *)gMsgEventLines_76_3,
    (char *)gMsgEventLines_76_4, (char *)gMsgEventLines_76_5, (char *)gMsgEventLines_76_6, (char *)gMsgEventLines_76_7,
    (char *)gMsgEventLines_76_8, NULL, NULL, NULL,
};
char* *gMsgEventLines[80] = {
    gMsgEventLines_0, gMsgEventLines_1, gMsgEventLines_2, gMsgEventLines_3,
    gMsgEventLines_4, gMsgEventLines_5, gMsgEventLines_6, gMsgEventLines_7,
    gMsgEventLines_8, gMsgEventLines_9, gMsgEventLines_10, gMsgEventLines_11,
    gMsgEventLines_12, gMsgEventLines_13, gMsgEventLines_14, gMsgEventLines_15,
    gMsgEventLines_16, gMsgEventLines_17, gMsgEventLines_18, gMsgEventLines_19,
    gMsgEventLines_20, gMsgEventLines_21, gMsgEventLines_22, gMsgEventLines_23,
    gMsgEventLines_24, gMsgEventLines_25, gMsgEventLines_26, gMsgEventLines_27,
    gMsgEventLines_28, gMsgEventLines_29, gMsgEventLines_30, gMsgEventLines_31,
    gMsgEventLines_32, gMsgEventLines_33, gMsgEventLines_34, gMsgEventLines_35,
    gMsgEventLines_36, gMsgEventLines_37, gMsgEventLines_38, gMsgEventLines_39,
    gMsgEventLines_40, gMsgEventLines_41, gMsgEventLines_42, gMsgEventLines_43,
    gMsgEventLines_44, gMsgEventLines_45, gMsgEventLines_46, gMsgEventLines_47,
    gMsgEventLines_48, gMsgEventLines_49, gMsgEventLines_50, gMsgEventLines_51,
    gMsgEventLines_52, gMsgEventLines_53, gMsgEventLines_54, gMsgEventLines_55,
    gMsgEventLines_56, gMsgEventLines_57, gMsgEventLines_58, gMsgEventLines_59,
    gMsgEventLines_60, gMsgEventLines_61, gMsgEventLines_62, gMsgEventLines_63,
    gMsgEventLines_64, gMsgEventLines_65, gMsgEventLines_66, gMsgEventLines_67,
    gMsgEventLines_68, gMsgEventLines_69, gMsgEventLines_70, gMsgEventLines_71,
    gMsgEventLines_72, gMsgEventLines_73, gMsgEventLines_74, gMsgEventLines_75,
    gMsgEventLines_76, NULL, NULL, NULL,
};
u16 gMsgEventSets[28][2] = {
#include "game/008AFCB0.inc"
};
u8 gMsgEventSetEntries_0[24] = {
#include "game/008AFD20.inc"
};
u8 gMsgEventSetEntries_1[12] = {
#include "game/008AFD38.inc"
};
u8 gMsgEventSetEntries_2[12] = {
#include "game/008AFD48.inc"
};
u8 gMsgEventSetEntries_3[8] = {
#include "game/008AFD58.inc"
};
u8 gMsgEventSetEntries_4[8] = {
#include "game/008AFD60.inc"
};
u8 gMsgEventSetEntries_5[8] = {
#include "game/008AFD68.inc"
};
u8 gMsgEventSetEntries_6[8] = {
#include "game/008AFD70.inc"
};
u8 gMsgEventSetEntries_7[8] = {
#include "game/008AFD78.inc"
};
u8 gMsgEventSetEntries_8[8] = {
#include "game/008AFD80.inc"
};
u8 gMsgEventSetEntries_9[8] = {
#include "game/008AFD88.inc"
};
u8 gMsgEventSetEntries_10[8] = {
#include "game/008AFD90.inc"
};
u8 gMsgEventSetEntries_11[8] = {
#include "game/008AFD98.inc"
};
u8 gMsgEventSetEntries_12[8] = {
#include "game/008AFDA0.inc"
};
u8 gMsgEventSetEntries_13[8] = {
#include "game/008AFDA8.inc"
};
u8 gMsgEventSetEntries_14[8] = {
#include "game/008AFDB0.inc"
};
u8 gMsgEventSetEntries_15[8] = {
#include "game/008AFDB8.inc"
};
u8 gMsgEventSetEntries_16[16] = {
#include "game/008AFDC0.inc"
};
u8 gMsgEventSetEntries_17[24] = {
#include "game/008AFDD0.inc"
};
u8 gMsgEventSetEntries_18[8] = {
#include "game/008AFDE8.inc"
};
u8 gMsgEventSetEntries_19[8] = {
#include "game/008AFDF0.inc"
};
u8 gMsgEventSetEntries_20[8] = {
#include "game/008AFDF8.inc"
};
u8 gMsgEventSetEntries_21[8] = {
#include "game/008AFE00.inc"
};
u8 gMsgEventSetEntries_22[8] = {
#include "game/008AFE08.inc"
};
u8 gMsgEventSetEntries_23[8] = {
#include "game/008AFE10.inc"
};
u8 gMsgEventSetEntries_24[8] = {
#include "game/008AFE18.inc"
};
u8 *gMsgEventSetEntries[28] = {
    gMsgEventSetEntries_0, gMsgEventSetEntries_1, gMsgEventSetEntries_2, gMsgEventSetEntries_3,
    gMsgEventSetEntries_4, gMsgEventSetEntries_5, gMsgEventSetEntries_6, gMsgEventSetEntries_7,
    gMsgEventSetEntries_8, gMsgEventSetEntries_9, gMsgEventSetEntries_10, gMsgEventSetEntries_11,
    gMsgEventSetEntries_12, gMsgEventSetEntries_13, gMsgEventSetEntries_14, gMsgEventSetEntries_15,
    gMsgEventSetEntries_16, gMsgEventSetEntries_17, gMsgEventSetEntries_18, gMsgEventSetEntries_19,
    gMsgEventSetEntries_20, gMsgEventSetEntries_21, gMsgEventSetEntries_22, gMsgEventSetEntries_23,
    gMsgEventSetEntries_24, NULL, NULL, NULL,
};
ListPanel gMsgEventListPanel = {
#include "game/008AFE90.inc"
};
Panel gMsgEventPopupWin = {
#include "game/008AFEB0.inc"
};
u8 gMsgEventSetIcon[32] = {
#include "game/008AFED0.inc"
};
MsgEvent gMsgEventsQuestStart[28] = {
#include "game/008AFEF0.inc"
};
MsgEvent gMsgEventsSupplyBox[4] = {
#include "game/008AFFD0.inc"
};
MsgEvent gMsgEventsGoalDelivered[8] = {
#include "game/008AFFF0.inc"
};
MsgEvent gMsgEventsApcerosHunted[4] = {
#include "game/008B0030.inc"
};
MsgEvent gMsgEventsCamp[4] = {
#include "game/008B0050.inc"
};
MsgEvent gMsgEventsJungle1[6] = {
#include "game/008B0070.inc"
};
MsgEvent gMsgEventsJungle4[3] = {
#include "game/008B00A0.inc"
};
