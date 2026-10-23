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
char D_008DE300[25] = {
#include "game/008DE300.inc"
};
char D_008DE320[27] = {
#include "game/008DE320.inc"
};
char D_008DE340[23] = {
#include "game/008DE340.inc"
};
char D_008DE360[35] = {
#include "game/008DE360.inc"
};
char D_008DE390[37] = {
#include "game/008DE390.inc"
};
char D_008DE3C0[31] = {
#include "game/008DE3C0.inc"
};
char D_008DE3E0[23] = {
#include "game/008DE3E0.inc"
};
char D_008DE3F8[15] = {
#include "game/008DE3F8.inc"
};
char D_008DE410[29] = {
#include "game/008DE410.inc"
};
char D_008DE430[23] = {
#include "game/008DE430.inc"
};
char D_008DE450[21] = {
#include "game/008DE450.inc"
};
char D_008DE470[31] = {
#include "game/008DE470.inc"
};
char D_008DE490[25] = {
#include "game/008DE490.inc"
};
char D_008DE4B0[23] = {
#include "game/008DE4B0.inc"
};
char D_008DE4C8[15] = {
#include "game/008DE4C8.inc"
};
char D_008DE4E0[31] = {
#include "game/008DE4E0.inc"
};
char D_008DE500[21] = {
#include "game/008DE500.inc"
};
char D_008DE520[37] = {
#include "game/008DE520.inc"
};
char D_008DE550[25] = {
#include "game/008DE550.inc"
};
char D_008DE570[25] = {
#include "game/008DE570.inc"
};
char D_008DE590[17] = {
#include "game/008DE590.inc"
};
char D_008DE5B0[33] = {
#include "game/008DE5B0.inc"
};
char D_008DE5E0[31] = {
#include "game/008DE5E0.inc"
};
char D_008DE600[25] = {
#include "game/008DE600.inc"
};
char D_008DE620[29] = {
#include "game/008DE620.inc"
};
char D_008DE640[23] = {
#include "game/008DE640.inc"
};
char D_008DE660[25] = {
#include "game/008DE660.inc"
};
char D_008DE680[31] = {
#include "game/008DE680.inc"
};
char D_008DE6A0[13] = {
#include "game/008DE6A0.inc"
};
char D_008DE6B0[25] = {
#include "game/008DE6B0.inc"
};
char D_008DE6D0[11] = {
#include "game/008DE6D0.inc"
};
char D_008DE6E0[23] = {
#include "game/008DE6E0.inc"
};
char D_008DE700[21] = {
#include "game/008DE700.inc"
};
char D_008DE718[15] = {
#include "game/008DE718.inc"
};
char D_008DE730[29] = {
#include "game/008DE730.inc"
};
char D_008DE750[13] = {
#include "game/008DE750.inc"
};
char D_008DE760[27] = {
#include "game/008DE760.inc"
};
char D_008DE780[11] = {
#include "game/008DE780.inc"
};
char D_008DE790[23] = {
#include "game/008DE790.inc"
};
char D_008DE7A8[11] = {
#include "game/008DE7A8.inc"
};
char D_008DE7C0[21] = {
#include "game/008DE7C0.inc"
};
char D_008DE7D8[13] = {
#include "game/008DE7D8.inc"
};
char D_008DE7F0[23] = {
#include "game/008DE7F0.inc"
};
char D_008DE810[17] = {
#include "game/008DE810.inc"
};
char D_008DE830[19] = {
#include "game/008DE830.inc"
};
char D_008DE850[19] = {
#include "game/008DE850.inc"
};
char D_008DE870[21] = {
#include "game/008DE870.inc"
};
char D_008DE890[27] = {
#include "game/008DE890.inc"
};
char D_008DE8B0[21] = {
#include "game/008DE8B0.inc"
};
char D_008DE8D0[23] = {
#include "game/008DE8D0.inc"
};
char D_008DE8E8[9] = {
#include "game/008DE8E8.inc"
};
char D_008DE8F8[11] = {
#include "game/008DE8F8.inc"
};
char D_008DE910[25] = {
#include "game/008DE910.inc"
};
char D_008DE930[15] = {
#include "game/008DE930.inc"
};
char D_008DE940[27] = {
#include "game/008DE940.inc"
};
char D_008DE960[13] = {
#include "game/008DE960.inc"
};
char D_008DE970[23] = {
#include "game/008DE970.inc"
};
char D_008DE990[23] = {
#include "game/008DE990.inc"
};
char D_008DE9B0[25] = {
#include "game/008DE9B0.inc"
};
char D_008DE9D0[15] = {
#include "game/008DE9D0.inc"
};
char D_008DE9E0[7] = {
#include "game/008DE9E0.inc"
};
char D_008DE9E8[9] = {
#include "game/008DE9E8.inc"
};
char D_008DE9F8[11] = {
#include "game/008DE9F8.inc"
};
char D_008DEA08[11] = {
#include "game/008DEA08.inc"
};
char D_008DEA18[9] = {
#include "game/008DEA18.inc"
};
char D_008DEA30[21] = {
#include "game/008DEA30.inc"
};
char D_008DEA50[19] = {
#include "game/008DEA50.inc"
};
char D_008DEA70[19] = {
#include "game/008DEA70.inc"
};
char D_008DEA90[17] = {
#include "game/008DEA90.inc"
};
char D_008DEAB0[27] = {
#include "game/008DEAB0.inc"
};
char D_008DEAD0[13] = {
#include "game/008DEAD0.inc"
};
char D_008DEAE0[31] = {
#include "game/008DEAE0.inc"
};
char D_008DEB00[26] = {
#include "game/008DEB00.inc"
};
char D_008DEB20[21] = {
#include "game/008DEB20.inc"
};
char D_008DEB40[17] = {
#include "game/008DEB40.inc"
};
char D_008DEB58[15] = {
#include "game/008DEB58.inc"
};
char D_008DEB70[167] = {
#include "game/008DEB70.inc"
};
char D_008DEC20[161] = {
#include "game/008DEC20.inc"
};
char D_008DECD0[161] = {
#include "game/008DECD0.inc"
};
char D_008DED80[224] = {
#include "game/008DED80.inc"
};
char D_008DEE60[199] = {
#include "game/008DEE60.inc"
};
char D_008DEF30[167] = {
#include "game/008DEF30.inc"
};
char D_008DEFE0[180] = {
#include "game/008DEFE0.inc"
};
char D_008DF0A0[205] = {
#include "game/008DF0A0.inc"
};
char D_008DF170[107] = {
#include "game/008DF170.inc"
};
char D_008DF1E0[179] = {
#include "game/008DF1E0.inc"
};
char D_008DF2A0[185] = {
#include "game/008DF2A0.inc"
};
char D_008DF360[146] = {
#include "game/008DF360.inc"
};
char D_008DF400[176] = {
#include "game/008DF400.inc"
};
char D_008DF4B0[199] = {
#include "game/008DF4B0.inc"
};
char D_008DF580[218] = {
#include "game/008DF580.inc"
};
char D_008DF660[197] = {
#include "game/008DF660.inc"
};
char D_008DF730[157] = {
#include "game/008DF730.inc"
};
char D_008DF7D0[165] = {
#include "game/008DF7D0.inc"
};
char D_008DF880[156] = {
#include "game/008DF880.inc"
};
char D_008DF920[227] = {
#include "game/008DF920.inc"
};
char D_008DFA10[155] = {
#include "game/008DFA10.inc"
};
char D_008DFAB0[172] = {
#include "game/008DFAB0.inc"
};
char D_008DFB60[168] = {
#include "game/008DFB60.inc"
};
char D_008DFC10[249] = {
#include "game/008DFC10.inc"
};
char D_008DFD10[189] = {
#include "game/008DFD10.inc"
};
char D_008DFDD0[134] = {
#include "game/008DFDD0.inc"
};
char D_008DFE60[203] = {
#include "game/008DFE60.inc"
};
char D_008DFF30[257] = {
#include "game/008DFF30.inc"
};
char D_008E0040[178] = {
#include "game/008E0040.inc"
};
char D_008E0100[129] = {
#include "game/008E0100.inc"
};
char D_008E0190[250] = {
#include "game/008E0190.inc"
};
char D_008E0290[136] = {
#include "game/008E0290.inc"
};
char D_008E0320[211] = {
#include "game/008E0320.inc"
};
char D_008E0400[177] = {
#include "game/008E0400.inc"
};
char D_008E04C0[183] = {
#include "game/008E04C0.inc"
};
char D_008E0580[160] = {
#include "game/008E0580.inc"
};
char D_008E0620[235] = {
#include "game/008E0620.inc"
};
char D_008E0710[130] = {
#include "game/008E0710.inc"
};
char D_008E07A0[162] = {
#include "game/008E07A0.inc"
};
char D_008E0850[209] = {
#include "game/008E0850.inc"
};
char D_008E0930[242] = {
#include "game/008E0930.inc"
};
char D_008E0A30[172] = {
#include "game/008E0A30.inc"
};
char D_008E0AE0[86] = {
#include "game/008E0AE0.inc"
};
char D_008E0B40[219] = {
#include "game/008E0B40.inc"
};
char D_008E0C20[132] = {
#include "game/008E0C20.inc"
};
char D_008E0CB0[168] = {
#include "game/008E0CB0.inc"
};
char D_008E0D60[182] = {
#include "game/008E0D60.inc"
};
char D_008E0E20[166] = {
#include "game/008E0E20.inc"
};
char D_008E0ED0[179] = {
#include "game/008E0ED0.inc"
};
char D_008E0F90[223] = {
#include "game/008E0F90.inc"
};
char D_008E1070[192] = {
#include "game/008E1070.inc"
};
char D_008E1130[169] = {
#include "game/008E1130.inc"
};
char D_008E11E0[123] = {
#include "game/008E11E0.inc"
};
char D_008E1260[171] = {
#include "game/008E1260.inc"
};
char D_008E1310[172] = {
#include "game/008E1310.inc"
};
char D_008E13C0[190] = {
#include "game/008E13C0.inc"
};
char D_008E1480[171] = {
#include "game/008E1480.inc"
};
char D_008E1530[223] = {
#include "game/008E1530.inc"
};
char D_008E1610[126] = {
#include "game/008E1610.inc"
};
char D_008E1690[191] = {
#include "game/008E1690.inc"
};
char D_008E1750[189] = {
#include "game/008E1750.inc"
};
char D_008E1810[150] = {
#include "game/008E1810.inc"
};
char D_008E18B0[243] = {
#include "game/008E18B0.inc"
};
char D_008E19B0[130] = {
#include "game/008E19B0.inc"
};
char D_008E1A40[156] = {
#include "game/008E1A40.inc"
};
char D_008E1AE0[150] = {
#include "game/008E1AE0.inc"
};
char D_008E1B80[119] = {
#include "game/008E1B80.inc"
};
char D_008E1C00[138] = {
#include "game/008E1C00.inc"
};
char D_008E1C90[150] = {
#include "game/008E1C90.inc"
};
char D_008E1D30[136] = {
#include "game/008E1D30.inc"
};
char D_008E1DC0[178] = {
#include "game/008E1DC0.inc"
};
char D_008E1E80[213] = {
#include "game/008E1E80.inc"
};
char D_008E1F60[210] = {
#include "game/008E1F60.inc"
};
char D_008E2040[158] = {
#include "game/008E2040.inc"
};
char D_008E20E0[199] = {
#include "game/008E20E0.inc"
};
char D_008E21B0[137] = {
#include "game/008E21B0.inc"
};
char D_008E2240[192] = {
#include "game/008E2240.inc"
};
char D_008E2300[96] = {
#include "game/008E2300.inc"
};
char D_008E2360[137] = {
#include "game/008E2360.inc"
};
char D_008E23F0[131] = {
#include "game/008E23F0.inc"
};
char D_008E2480[162] = {
#include "game/008E2480.inc"
};
char D_008E2530[215] = {
#include "game/008E2530.inc"
};
char D_008E2610[178] = {
#include "game/008E2610.inc"
};
char D_008E26D0[215] = {
#include "game/008E26D0.inc"
};
char D_008E27B0[221] = {
#include "game/008E27B0.inc"
};
char D_008E2890[130] = {
#include "game/008E2890.inc"
};
char D_008E2920[162] = {
#include "game/008E2920.inc"
};
char D_008E29D0[196] = {
#include "game/008E29D0.inc"
};
char D_008E2AA0[183] = {
#include "game/008E2AA0.inc"
};
char D_008E2B60[239] = {
#include "game/008E2B60.inc"
};
char D_008E2C50[151] = {
#include "game/008E2C50.inc"
};
char D_008E2CF0[168] = {
#include "game/008E2CF0.inc"
};
char D_008E2DA0[200] = {
#include "game/008E2DA0.inc"
};
char D_008E2E70[251] = {
#include "game/008E2E70.inc"
};
char D_008E2F70[126] = {
#include "game/008E2F70.inc"
};
char D_008E2FF0[152] = {
#include "game/008E2FF0.inc"
};
char D_008E3090[142] = {
#include "game/008E3090.inc"
};
char D_008E3120[217] = {
#include "game/008E3120.inc"
};
char D_008E3200[187] = {
#include "game/008E3200.inc"
};
char D_008E32C0[181] = {
#include "game/008E32C0.inc"
};
char D_008E3380[156] = {
#include "game/008E3380.inc"
};
char D_008E3420[157] = {
#include "game/008E3420.inc"
};
char D_008E34C0[134] = {
#include "game/008E34C0.inc"
};
char D_008E3550[113] = {
#include "game/008E3550.inc"
};
char D_008E35D0[152] = {
#include "game/008E35D0.inc"
};
char D_008E3670[241] = {
#include "game/008E3670.inc"
};
char D_008E3770[128] = {
#include "game/008E3770.inc"
};
char D_008E37F0[179] = {
#include "game/008E37F0.inc"
};
char D_008E38B0[193] = {
#include "game/008E38B0.inc"
};
char D_008E3980[170] = {
#include "game/008E3980.inc"
};
char D_008E3A30[156] = {
#include "game/008E3A30.inc"
};
char D_008E3AD0[156] = {
#include "game/008E3AD0.inc"
};
char D_008E3B70[156] = {
#include "game/008E3B70.inc"
};
char D_008E3C10[185] = {
#include "game/008E3C10.inc"
};
char D_008E3CD0[136] = {
#include "game/008E3CD0.inc"
};
char D_008E3D60[126] = {
#include "game/008E3D60.inc"
};
char D_008E3DE0[218] = {
#include "game/008E3DE0.inc"
};
char D_008E3EC0[207] = {
#include "game/008E3EC0.inc"
};
char D_008E3F90[215] = {
#include "game/008E3F90.inc"
};
char D_008E4070[164] = {
#include "game/008E4070.inc"
};
char D_008E4120[187] = {
#include "game/008E4120.inc"
};
char D_008E41E0[207] = {
#include "game/008E41E0.inc"
};
char D_008E42B0[205] = {
#include "game/008E42B0.inc"
};
char D_008E4380[197] = {
#include "game/008E4380.inc"
};
char D_008E4450[130] = {
#include "game/008E4450.inc"
};
char D_008E44E0[183] = {
#include "game/008E44E0.inc"
};
char D_008E45A0[201] = {
#include "game/008E45A0.inc"
};
char D_008E4670[213] = {
#include "game/008E4670.inc"
};
char D_008E4750[173] = {
#include "game/008E4750.inc"
};
char D_008E4800[135] = {
#include "game/008E4800.inc"
};
char D_008E4890[199] = {
#include "game/008E4890.inc"
};
char D_008E4960[185] = {
#include "game/008E4960.inc"
};
char D_008E4A20[128] = {
#include "game/008E4A20.inc"
};
char D_008E4AA0[216] = {
#include "game/008E4AA0.inc"
};
char D_008E4B80[215] = {
#include "game/008E4B80.inc"
};
char D_008E4C60[229] = {
#include "game/008E4C60.inc"
};
char D_008E4D50[187] = {
#include "game/008E4D50.inc"
};
char D_008E4E10[142] = {
#include "game/008E4E10.inc"
};
char D_008E4EA0[165] = {
#include "game/008E4EA0.inc"
};
char D_008E4F50[126] = {
#include "game/008E4F50.inc"
};
char D_008E4FD0[182] = {
#include "game/008E4FD0.inc"
};
char D_008E5090[152] = {
#include "game/008E5090.inc"
};
char D_008E5130[162] = {
#include "game/008E5130.inc"
};
char D_008E51E0[217] = {
#include "game/008E51E0.inc"
};
char D_008E52C0[215] = {
#include "game/008E52C0.inc"
};
char D_008E53A0[198] = {
#include "game/008E53A0.inc"
};
char D_008E5470[231] = {
#include "game/008E5470.inc"
};
char D_008E5560[126] = {
#include "game/008E5560.inc"
};
char D_008E55E0[174] = {
#include "game/008E55E0.inc"
};
char D_008E5690[205] = {
#include "game/008E5690.inc"
};
char D_008E5760[193] = {
#include "game/008E5760.inc"
};
char D_008E5830[233] = {
#include "game/008E5830.inc"
};
char D_008E5920[177] = {
#include "game/008E5920.inc"
};
char D_008E59E0[177] = {
#include "game/008E59E0.inc"
};
char D_008E5AA0[185] = {
#include "game/008E5AA0.inc"
};
char D_008E5B60[128] = {
#include "game/008E5B60.inc"
};
char D_008E5BE0[197] = {
#include "game/008E5BE0.inc"
};
char D_008E5CB0[221] = {
#include "game/008E5CB0.inc"
};
char D_008E5D90[191] = {
#include "game/008E5D90.inc"
};
char D_008E5E50[225] = {
#include "game/008E5E50.inc"
};
char D_008E5F40[203] = {
#include "game/008E5F40.inc"
};
char D_008E6010[211] = {
#include "game/008E6010.inc"
};
char D_008E60F0[228] = {
#include "game/008E60F0.inc"
};
char D_008E61E0[133] = {
#include "game/008E61E0.inc"
};
char D_008E6270[202] = {
#include "game/008E6270.inc"
};
char D_008E6340[203] = {
#include "game/008E6340.inc"
};
char D_008E6410[213] = {
#include "game/008E6410.inc"
};
char D_008E64F0[199] = {
#include "game/008E64F0.inc"
};
char D_008E65C0[181] = {
#include "game/008E65C0.inc"
};
char D_008E6680[201] = {
#include "game/008E6680.inc"
};
char D_008E6750[146] = {
#include "game/008E6750.inc"
};
char D_008E67F0[199] = {
#include "game/008E67F0.inc"
};
char D_008E68C0[213] = {
#include "game/008E68C0.inc"
};
char D_008E69A0[211] = {
#include "game/008E69A0.inc"
};
char D_008E6A80[197] = {
#include "game/008E6A80.inc"
};
char D_008E6B50[164] = {
#include "game/008E6B50.inc"
};
char D_008E6C00[137] = {
#include "game/008E6C00.inc"
};
char D_008E6C90[176] = {
#include "game/008E6C90.inc"
};
char D_008E6D40[134] = {
#include "game/008E6D40.inc"
};
char D_008E6DD0[154] = {
#include "game/008E6DD0.inc"
};
char D_008E6E70[205] = {
#include "game/008E6E70.inc"
};
char D_008E6F40[154] = {
#include "game/008E6F40.inc"
};
char D_008E6FE0[127] = {
#include "game/008E6FE0.inc"
};
char D_008E7060[207] = {
#include "game/008E7060.inc"
};
char D_008E7130[105] = {
#include "game/008E7130.inc"
};
char D_008E71A0[137] = {
#include "game/008E71A0.inc"
};
char D_008E7230[130] = {
#include "game/008E7230.inc"
};
char D_008E72C0[195] = {
#include "game/008E72C0.inc"
};
char D_008E7390[189] = {
#include "game/008E7390.inc"
};
char D_008E7450[154] = {
#include "game/008E7450.inc"
};
char D_008E74F0[201] = {
#include "game/008E74F0.inc"
};
char D_008E75C0[223] = {
#include "game/008E75C0.inc"
};
char D_008E76A0[124] = {
#include "game/008E76A0.inc"
};
char D_008E7720[183] = {
#include "game/008E7720.inc"
};
char D_008E77E0[221] = {
#include "game/008E77E0.inc"
};
char D_008E78C0[222] = {
#include "game/008E78C0.inc"
};
char D_008E79A0[164] = {
#include "game/008E79A0.inc"
};
char D_008E7A50[162] = {
#include "game/008E7A50.inc"
};
char D_008E7B00[152] = {
#include "game/008E7B00.inc"
};
char D_008E7BA0[199] = {
#include "game/008E7BA0.inc"
};
char D_008E7C70[152] = {
#include "game/008E7C70.inc"
};
char D_008E7D10[195] = {
#include "game/008E7D10.inc"
};
char D_008E7DE0[176] = {
#include "game/008E7DE0.inc"
};
char D_008E7E90[139] = {
#include "game/008E7E90.inc"
};
char D_008E7F20[171] = {
#include "game/008E7F20.inc"
};
char D_008E7FD0[141] = {
#include "game/008E7FD0.inc"
};
char D_008E8060[156] = {
#include "game/008E8060.inc"
};
char D_008E8100[125] = {
#include "game/008E8100.inc"
};
char D_008E8180[192] = {
#include "game/008E8180.inc"
};
char D_008E8240[239] = {
#include "game/008E8240.inc"
};
char D_008E8330[195] = {
#include "game/008E8330.inc"
};
char D_008E8400[207] = {
#include "game/008E8400.inc"
};
char D_008E84D0[189] = {
#include "game/008E84D0.inc"
};
char D_008E8590[205] = {
#include "game/008E8590.inc"
};
char D_008E8660[235] = {
#include "game/008E8660.inc"
};
char D_008E8750[137] = {
#include "game/008E8750.inc"
};
char D_008E87E0[233] = {
#include "game/008E87E0.inc"
};
char D_008E88D0[138] = {
#include "game/008E88D0.inc"
};
char D_008E8960[188] = {
#include "game/008E8960.inc"
};
char D_008E8A20[233] = {
#include "game/008E8A20.inc"
};
char D_008E8B10[203] = {
#include "game/008E8B10.inc"
};
char D_008E8BE0[162] = {
#include "game/008E8BE0.inc"
};
char D_008E8C90[132] = {
#include "game/008E8C90.inc"
};
char D_008E8D20[142] = {
#include "game/008E8D20.inc"
};
char D_008E8DB0[191] = {
#include "game/008E8DB0.inc"
};
char D_008E8E70[166] = {
#include "game/008E8E70.inc"
};
char D_008E8F20[213] = {
#include "game/008E8F20.inc"
};
char D_008E9000[233] = {
#include "game/008E9000.inc"
};
char D_008E90F0[195] = {
#include "game/008E90F0.inc"
};
char D_008E91C0[217] = {
#include "game/008E91C0.inc"
};
char D_008E92A0[207] = {
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
char D_008E95A0[211] = {
#include "game/008E95A0.inc"
};
char D_008E9680[116] = {
#include "game/008E9680.inc"
};
char D_008E9700[213] = {
#include "game/008E9700.inc"
};
char D_008E97E0[217] = {
#include "game/008E97E0.inc"
};
char D_008E98C0[189] = {
#include "game/008E98C0.inc"
};
char D_008E9980[118] = {
#include "game/008E9980.inc"
};
char D_008E9A00[146] = {
#include "game/008E9A00.inc"
};
char D_008E9AA0[140] = {
#include "game/008E9AA0.inc"
};
char D_008E9B30[203] = {
#include "game/008E9B30.inc"
};
char D_008E9C00[189] = {
#include "game/008E9C00.inc"
};
char D_008E9CC0[221] = {
#include "game/008E9CC0.inc"
};
char D_008E9DA0[235] = {
#include "game/008E9DA0.inc"
};
char D_008E9E90[213] = {
#include "game/008E9E90.inc"
};
char D_008E9F70[224] = {
#include "game/008E9F70.inc"
};
char D_008EA050[202] = {
#include "game/008EA050.inc"
};
char D_008EA120[107] = {
#include "game/008EA120.inc"
};
char D_008EA190[158] = {
#include "game/008EA190.inc"
};
char D_008EA230[205] = {
#include "game/008EA230.inc"
};
char D_008EA300[210] = {
#include "game/008EA300.inc"
};
char D_008EA3E0[205] = {
#include "game/008EA3E0.inc"
};
char D_008EA4B0[160] = {
#include "game/008EA4B0.inc"
};
char D_008EA550[187] = {
#include "game/008EA550.inc"
};
char D_008EA610[111] = {
#include "game/008EA610.inc"
};
char D_008EA680[215] = {
#include "game/008EA680.inc"
};
char D_008EA760[147] = {
#include "game/008EA760.inc"
};
char D_008EA800[239] = {
#include "game/008EA800.inc"
};
char D_008EA8F0[189] = {
#include "game/008EA8F0.inc"
};
char D_008EA9B0[177] = {
#include "game/008EA9B0.inc"
};
char D_008EAA70[177] = {
#include "game/008EAA70.inc"
};
char D_008EAB30[189] = {
#include "game/008EAB30.inc"
};
char D_008EABF0[178] = {
#include "game/008EABF0.inc"
};
char D_008EACB0[155] = {
#include "game/008EACB0.inc"
};
char D_008EAD50[178] = {
#include "game/008EAD50.inc"
};
char D_008EAE10[142] = {
#include "game/008EAE10.inc"
};
char D_008EAEA0[142] = {
#include "game/008EAEA0.inc"
};
char D_008EAF30[123] = {
#include "game/008EAF30.inc"
};
char D_008EAFB0[128] = {
#include "game/008EAFB0.inc"
};
char D_008EB030[160] = {
#include "game/008EB030.inc"
};
char D_008EB0D0[177] = {
#include "game/008EB0D0.inc"
};
char D_008EB190[196] = {
#include "game/008EB190.inc"
};
char D_008EB260[157] = {
#include "game/008EB260.inc"
};
char D_008EB300[215] = {
#include "game/008EB300.inc"
};
char D_008EB3E0[86] = {
#include "game/008EB3E0.inc"
};
char D_008EB440[178] = {
#include "game/008EB440.inc"
};
char D_008EB500[107] = {
#include "game/008EB500.inc"
};
char D_008EB570[96] = {
#include "game/008EB570.inc"
};
char D_008EB5D0[141] = {
#include "game/008EB5D0.inc"
};
char D_008EB660[135] = {
#include "game/008EB660.inc"
};
char D_008EB6F0[123] = {
#include "game/008EB6F0.inc"
};
char D_008EB770[133] = {
#include "game/008EB770.inc"
};
char D_008EB800[73] = {
#include "game/008EB800.inc"
};
char D_008EB850[157] = {
#include "game/008EB850.inc"
};
char D_008EB8F0[123] = {
#include "game/008EB8F0.inc"
};
char D_008EB970[103] = {
#include "game/008EB970.inc"
};
char D_008EB9E0[52] = {
#include "game/008EB9E0.inc"
};
char D_008EBA20[112] = {
#include "game/008EBA20.inc"
};
char D_008EBA90[113] = {
#include "game/008EBA90.inc"
};
char D_008EBB10[141] = {
#include "game/008EBB10.inc"
};
char D_008EBBA0[195] = {
#include "game/008EBBA0.inc"
};
char D_008EBC70[192] = {
#include "game/008EBC70.inc"
};
char D_008EBD30[149] = {
#include "game/008EBD30.inc"
};
char D_008EBDD0[178] = {
#include "game/008EBDD0.inc"
};
char D_008EBE90[187] = {
#include "game/008EBE90.inc"
};
char D_008EBF50[206] = {
#include "game/008EBF50.inc"
};
char D_008EC020[121] = {
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
    (char *)D_008DE300, (char *)D_008DE320, (char *)D_008DE340, (char *)D_008DE360,
    (char *)D_008DE390, (char *)D_008DE3C0, (char *)D_008DE3E0, (char *)D_008DE3F8,
    (char *)D_008DE410, (char *)D_008DE430, (char *)D_008DE450, (char *)D_008DE470,
    (char *)D_008DE490, (char *)D_008DE4B0, (char *)D_008DE4C8, (char *)D_008DE4E0,
    (char *)D_008DE500, (char *)D_008DE520, (char *)D_008DE550, (char *)D_008DE570,
    (char *)D_008DE590, (char *)D_008DE5B0, (char *)D_008DE5E0, (char *)D_008DE600,
    (char *)D_008DE620, (char *)D_008DE640, (char *)D_008DE660, (char *)D_008DE680,
    (char *)D_008DE6A0, (char *)D_008DE6B0, (char *)D_008DE6D0, (char *)D_008DE6E0,
    (char *)D_008DE700, (char *)D_008DE718, (char *)D_008DE730, (char *)D_008DE750,
    (char *)D_008DE760, (char *)D_008DE780, (char *)D_008DE790, (char *)D_008DE7A8,
    (char *)D_008DE7C0, (char *)D_008DE7D8, (char *)D_008DE7F0, (char *)D_008DE810,
    (char *)D_008DE830, (char *)D_008DE850, (char *)D_008DE870, (char *)D_008DE890,
    (char *)D_008DE8B0, (char *)D_008DE8D0, (char *)D_008DE8E8, (char *)D_008DE8F8,
    (char *)D_008DE910, (char *)D_008DE930, (char *)D_008DE940, (char *)D_008DE960,
    (char *)D_008DE970, (char *)D_008DE990, (char *)D_008DE9B0, (char *)D_008DE9D0,
    (char *)D_008DE9E0, (char *)D_008DE9E8, (char *)D_008DE9F8, (char *)D_008DEA08,
    (char *)D_008DEA18, (char *)D_008DEA30, (char *)D_008DEA50, (char *)D_008DEA50,
    (char *)D_008DEA70, (char *)D_008DEA90, (char *)D_008DEAB0, (char *)D_008DEAD0,
    (char *)D_008DEAE0, (char *)D_008DEB00, (char *)D_008DEB20, (char *)D_008DEB40,
    (char *)D_008DEB58, NULL, NULL, NULL,
};
char *gMsgEventLines_0[6] = {
    (char *)D_008DEB70, (char *)D_008DEC20, (char *)D_008DECD0, (char *)D_008DED80,
    (char *)D_008DEE60, NULL,
};
char *gMsgEventLines_1[3] = {
    (char *)D_008DEF30, (char *)D_008DEFE0, (char *)D_008DF0A0,
};
char *gMsgEventLines_2[4] = {
    (char *)D_008DF170, (char *)D_008DF1E0, (char *)D_008DF2A0, (char *)D_008DF360,
};
char *gMsgEventLines_3[4] = {
    (char *)D_008DF400, (char *)D_008DF4B0, (char *)D_008DF580, NULL,
};
char *gMsgEventLines_4[4] = {
    (char *)D_008DF660, (char *)D_008DF730, (char *)D_008DF7D0, NULL,
};
char *gMsgEventLines_5[4] = {
    (char *)D_008DF880, (char *)D_008DF920, (char *)D_008DFA10, (char *)D_008DFAB0,
};
char *gMsgEventLines_6[4] = {
    (char *)D_008DFB60, (char *)D_008DFC10, (char *)D_008DFD10, NULL,
};
char *gMsgEventLines_7[8] = {
    (char *)D_008DFDD0, (char *)D_008DFE60, (char *)D_008DFF30, (char *)D_008E0040,
    (char *)D_008E0100, (char *)D_008E0190, (char *)D_008E0290, NULL,
};
char *gMsgEventLines_8[4] = {
    (char *)D_008E0320, (char *)D_008E0400, (char *)D_008E04C0, (char *)D_008E0580,
};
char *gMsgEventLines_9[4] = {
    (char *)D_008E0620, NULL, NULL, NULL,
};
char *gMsgEventLines_10[8] = {
    (char *)D_008E0710, (char *)D_008E07A0, (char *)D_008E0850, (char *)D_008E0930,
    (char *)D_008E0A30, NULL, NULL, NULL,
};
char *gMsgEventLines_11[8] = {
    (char *)D_008E0AE0, (char *)D_008E0B40, (char *)D_008E0C20, (char *)D_008E0CB0,
    (char *)D_008E0D60, NULL, NULL, NULL,
};
char *gMsgEventLines_12[8] = {
    (char *)D_008E0E20, (char *)D_008E0ED0, (char *)D_008E0F90, (char *)D_008E1070,
    (char *)D_008E1130, (char *)D_008E11E0, NULL, NULL,
};
char *gMsgEventLines_13[8] = {
    (char *)D_008E1260, (char *)D_008E1310, (char *)D_008E13C0, (char *)D_008E1480,
    (char *)D_008E1530, NULL, NULL, NULL,
};
char *gMsgEventLines_14[4] = {
    (char *)D_008E1610, (char *)D_008E1690, (char *)D_008E1750, (char *)D_008E1810,
};
char *gMsgEventLines_15[2] = {
    (char *)D_008E18B0, NULL,
};
char *gMsgEventLines_16[3] = {
    (char *)D_008E19B0, (char *)D_008E1A40, (char *)D_008E1AE0,
};
char *gMsgEventLines_17[4] = {
    (char *)D_008E1B80, (char *)D_008E1C00, (char *)D_008E1C90, (char *)D_008E1D30,
};
char *gMsgEventLines_18[8] = {
    (char *)D_008E1DC0, (char *)D_008E1E80, (char *)D_008E1F60, (char *)D_008E2040,
    (char *)D_008E20E0, NULL, NULL, NULL,
};
char *gMsgEventLines_19[4] = {
    (char *)D_008E21B0, (char *)D_008E2240, (char *)D_008E2300, (char *)D_008E2360,
};
char *gMsgEventLines_20[6] = {
    (char *)D_008E23F0, (char *)D_008E2480, (char *)D_008E2530, (char *)D_008E2610,
    (char *)D_008E26D0, NULL,
};
char *gMsgEventLines_21[2] = {
    (char *)D_008E27B0, NULL,
};
char *gMsgEventLines_22[6] = {
    (char *)D_008E2890, (char *)D_008E2920, (char *)D_008E29D0, (char *)D_008E2AA0,
    (char *)D_008E2B60, NULL,
};
char *gMsgEventLines_23[3] = {
    (char *)D_008E2C50, (char *)D_008E2CF0, (char *)D_008E2DA0,
};
char *gMsgEventLines_24[2] = {
    (char *)D_008E2E70, NULL,
};
char *gMsgEventLines_25[8] = {
    (char *)D_008E2F70, (char *)D_008E2FF0, (char *)D_008E3090, (char *)D_008E3120,
    (char *)D_008E3200, NULL, NULL, NULL,
};
char *gMsgEventLines_26[6] = {
    (char *)D_008E32C0, (char *)D_008E3380, (char *)D_008E3420, (char *)D_008E34C0,
    (char *)D_008E3550, (char *)D_008E35D0,
};
char *gMsgEventLines_27[2] = {
    (char *)D_008E3670, NULL,
};
char *gMsgEventLines_28[6] = {
    (char *)D_008E3770, (char *)D_008E37F0, (char *)D_008E38B0, (char *)D_008E3980,
    (char *)D_008E3A30, (char *)D_008E3AD0,
};
char *gMsgEventLines_29[3] = {
    (char *)D_008E3B70, (char *)D_008E3C10, (char *)D_008E3CD0,
};
char *gMsgEventLines_30[6] = {
    (char *)D_008E3D60, (char *)D_008E37F0, (char *)D_008E3DE0, (char *)D_008E3EC0,
    (char *)D_008E3F90, NULL,
};
char *gMsgEventLines_31[3] = {
    (char *)D_008E4070, (char *)D_008E4120, (char *)D_008E41E0,
};
char *gMsgEventLines_32[2] = {
    (char *)D_008E42B0, (char *)D_008E4380,
};
char *gMsgEventLines_33[6] = {
    (char *)D_008E4450, (char *)D_008E37F0, (char *)D_008E44E0, (char *)D_008E45A0,
    (char *)D_008E4670, (char *)D_008E4750,
};
char *gMsgEventLines_34[3] = {
    (char *)D_008E4800, (char *)D_008E4890, (char *)D_008E4960,
};
char *gMsgEventLines_35[8] = {
    (char *)D_008E4A20, (char *)D_008E37F0, (char *)D_008E4AA0, (char *)D_008E4B80,
    (char *)D_008E4C60, (char *)D_008E4D50, NULL, NULL,
};
char *gMsgEventLines_36[4] = {
    (char *)D_008E3B70, (char *)D_008E3C10, (char *)D_008E4E10, (char *)D_008E4EA0,
};
char *gMsgEventLines_37[6] = {
    (char *)D_008E4F50, (char *)D_008E37F0, (char *)D_008E4FD0, (char *)D_008E5090,
    (char *)D_008E5130, (char *)D_008E51E0,
};
char *gMsgEventLines_38[3] = {
    (char *)D_008E52C0, (char *)D_008E53A0, (char *)D_008E5470,
};
char *gMsgEventLines_39[8] = {
    (char *)D_008E5560, (char *)D_008E37F0, (char *)D_008E55E0, (char *)D_008E5690,
    (char *)D_008E5760, NULL, NULL, NULL,
};
char *gMsgEventLines_40[4] = {
    (char *)D_008E5830, (char *)D_008E5920, (char *)D_008E59E0, (char *)D_008E5AA0,
};
char *gMsgEventLines_41[8] = {
    (char *)D_008E5B60, (char *)D_008E37F0, (char *)D_008E5BE0, (char *)D_008E5CB0,
    (char *)D_008E5D90, NULL, NULL, NULL,
};
char *gMsgEventLines_42[4] = {
    (char *)D_008E5E50, (char *)D_008E5F40, (char *)D_008E6010, (char *)D_008E60F0,
};
char *gMsgEventLines_43[8] = {
    (char *)D_008E61E0, (char *)D_008E37F0, (char *)D_008E6270, (char *)D_008E6340,
    (char *)D_008E6410, (char *)D_008E64F0, NULL, NULL,
};
char *gMsgEventLines_44[4] = {
    (char *)D_008E65C0, (char *)D_008E6680, (char *)D_008E6750, (char *)D_008E67F0,
};
char *gMsgEventLines_45[4] = {
    (char *)D_008E68C0, (char *)D_008E69A0, (char *)D_008E6A80, (char *)D_008E6B50,
};
char *gMsgEventLines_46[8] = {
    (char *)D_008E6C00, (char *)D_008E6C90, (char *)D_008E6D40, (char *)D_008E6DD0,
    (char *)D_008E6E70, NULL, NULL, NULL,
};
char *gMsgEventLines_47[4] = {
    (char *)D_008E6F40, (char *)D_008E6FE0, (char *)D_008E7060, (char *)D_008E7130,
};
char *gMsgEventLines_48[8] = {
    (char *)D_008E71A0, (char *)D_008E6C90, (char *)D_008E7230, (char *)D_008E72C0,
    (char *)D_008E7390, NULL, NULL, NULL,
};
char *gMsgEventLines_49[4] = {
    (char *)D_008E7450, (char *)D_008E74F0, (char *)D_008E75C0, (char *)D_008E7130,
};
char *gMsgEventLines_50[6] = {
    (char *)D_008E76A0, (char *)D_008E7720, (char *)D_008E77E0, (char *)D_008E78C0,
    (char *)D_008E79A0, NULL,
};
char *gMsgEventLines_51[2] = {
    (char *)D_008E7A50, (char *)D_008E7B00,
};
char *gMsgEventLines_52[4] = {
    (char *)D_008E7BA0, (char *)D_008E7C70, (char *)D_008E7D10, (char *)D_008E7DE0,
};
char *gMsgEventLines_53[4] = {
    (char *)D_008E7E90, (char *)D_008E7F20, (char *)D_008E7FD0, NULL,
};
char *gMsgEventLines_54[4] = {
    (char *)D_008E8060, (char *)D_008E8100, (char *)D_008E8180, NULL,
};
char *gMsgEventLines_55[6] = {
    D_008E8240, D_008E8330, (char *)D_008E8400, (char *)D_008E84D0,
    (char *)D_008E8590, NULL,
};
char *gMsgEventLines_56[2] = {
    (char *)D_008E8660, NULL,
};
char *gMsgEventLines_57[8] = {
    (char *)D_008E8750, (char *)D_008E87E0, (char *)D_008E88D0, (char *)D_008E8960,
    (char *)D_008E8A20, (char *)D_008E8B10, (char *)D_008E8BE0, NULL,
};
char *gMsgEventLines_58[4] = {
    (char *)D_008E8C90, (char *)D_008E8D20, (char *)D_008E8DB0, NULL,
};
char *gMsgEventLines_59[4] = {
    D_008E8E70, D_008E8F20, (char *)D_008E9000, (char *)D_008E90F0,
};
char *gMsgEventLines_60[2] = {
    D_008E91C0, (char *)D_008E92A0,
};
char *gMsgEventLines_61[3] = {
    D_008E9370, D_008E9420, D_008E94D0,
};
char *gMsgEventLines_62[2] = {
    (char *)D_008E95A0, (char *)D_008E9680,
};
char *gMsgEventLines_63[2] = {
    (char *)D_008E9700, (char *)D_008E97E0,
};
char *gMsgEventLines_64[2] = {
    (char *)D_008E98C0, (char *)D_008E9980,
};
char *gMsgEventLines_65[6] = {
    (char *)D_008E9A00, (char *)D_008E9AA0, (char *)D_008E9B30, (char *)D_008E9C00,
    (char *)D_008E9CC0, NULL,
};
char *gMsgEventLines_66[2] = {
    (char *)D_008E9DA0, NULL,
};
char *gMsgEventLines_67[2] = {
    (char *)D_008E9E90, NULL,
};
char *gMsgEventLines_68[3] = {
    D_008E9F70, (char *)D_008EA050, (char *)D_008EA120,
};
char *gMsgEventLines_69[8] = {
    (char *)D_008EA190, (char *)D_008EA230, (char *)D_008EA300, (char *)D_008EA3E0,
    (char *)D_008EA4B0, NULL, NULL, NULL,
};
char *gMsgEventLines_70[8] = {
    (char *)D_008EA550, (char *)D_008EA610, (char *)D_008EA680, (char *)D_008EA760,
    (char *)D_008EA800, (char *)D_008EA8F0, (char *)D_008EA9B0, (char *)D_008EAA70,
};
char *gMsgEventLines_71[8] = {
    (char *)D_008EAB30, (char *)D_008EABF0, (char *)D_008EACB0, (char *)D_008EAD50,
    (char *)D_008EAE10, (char *)D_008EAEA0, (char *)D_008EAF30, (char *)D_008EAFB0,
};
char *gMsgEventLines_72[4] = {
    (char *)D_008EB030, (char *)D_008EB0D0, (char *)D_008EB190, (char *)D_008EB260,
};
char *gMsgEventLines_73[4] = {
    (char *)D_008EB300, (char *)D_008EB3E0, (char *)D_008EB440, (char *)D_008EB500,
};
char *gMsgEventLines_74[8] = {
    (char *)D_008EB570, (char *)D_008EB5D0, (char *)D_008EB660, (char *)D_008EB6F0,
    (char *)D_008EB770, NULL, NULL, NULL,
};
char *gMsgEventLines_75[8] = {
    (char *)D_008EB800, (char *)D_008EB850, (char *)D_008EB8F0, (char *)D_008EB970,
    (char *)D_008EB9E0, (char *)D_008EBA20, NULL, NULL,
};
char *gMsgEventLines_76[12] = {
    (char *)D_008EBA90, (char *)D_008EBB10, (char *)D_008EBBA0, (char *)D_008EBC70,
    (char *)D_008EBD30, (char *)D_008EBDD0, (char *)D_008EBE90, (char *)D_008EBF50,
    (char *)D_008EC020, NULL, NULL, NULL,
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
