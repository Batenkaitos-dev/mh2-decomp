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
extern char gMsgEventListItems[];
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
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE300);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE320);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE340);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE360);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE390);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE3C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE3E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE3F8);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE410);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE430);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE450);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE470);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE490);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE4B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE4C8);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE4E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE500);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE520);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE550);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE570);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE590);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE5B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE5E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE600);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE620);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE640);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE660);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE680);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE6A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE6B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE6D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE6E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE700);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE718);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE730);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE750);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE760);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE780);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE790);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE7A8);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE7C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE7D8);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE7F0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE810);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE830);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE850);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE870);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE890);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE8B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE8D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE8E8);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE8F8);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE910);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE930);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE940);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE960);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE970);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE990);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE9B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE9D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE9E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE9E8);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DE9F8);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEA08);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEA18);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEA30);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEA50);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEA70);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEA90);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEAB0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEAD0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEAE0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEB00);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEB20);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEB40);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEB58);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEB70);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEC20);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DECD0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DED80);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEE60);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEF30);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DEFE0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF0A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF170);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF1E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF2A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF360);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF400);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF4B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF580);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF660);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF730);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF7D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF880);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DF920);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DFA10);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DFAB0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DFB60);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DFC10);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DFD10);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DFDD0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DFE60);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008DFF30);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0040);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0100);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0190);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0290);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0320);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0400);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E04C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0580);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0620);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0710);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E07A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0850);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0930);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0A30);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0AE0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0B40);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0C20);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0CB0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0D60);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0E20);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0ED0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E0F90);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1070);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1130);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E11E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1260);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1310);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E13C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1480);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1530);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1610);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1690);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1750);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1810);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E18B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E19B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1A40);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1AE0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1B80);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1C00);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1C90);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1D30);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1DC0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1E80);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E1F60);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2040);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E20E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E21B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2240);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2300);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2360);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E23F0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2480);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2530);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2610);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E26D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E27B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2890);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2920);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E29D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2AA0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2B60);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2C50);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2CF0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2DA0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2E70);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2F70);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E2FF0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3090);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3120);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3200);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E32C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3380);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3420);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E34C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3550);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E35D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3670);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3770);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E37F0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E38B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3980);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3A30);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3AD0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3B70);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3C10);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3CD0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3D60);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3DE0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3EC0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E3F90);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4070);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4120);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E41E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E42B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4380);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4450);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E44E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E45A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4670);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4750);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4800);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4890);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4960);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4A20);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4AA0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4B80);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4C60);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4D50);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4E10);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4EA0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4F50);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E4FD0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5090);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5130);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E51E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E52C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E53A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5470);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5560);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E55E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5690);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5760);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5830);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5920);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E59E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5AA0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5B60);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5BE0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5CB0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5D90);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5E50);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E5F40);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6010);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E60F0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E61E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6270);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6340);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6410);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E64F0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E65C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6680);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6750);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E67F0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E68C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E69A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6A80);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6B50);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6C00);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6C90);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6D40);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6DD0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6E70);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6F40);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E6FE0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7060);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7130);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E71A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7230);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E72C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7390);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7450);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E74F0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E75C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E76A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7720);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E77E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E78C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E79A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7A50);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7B00);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7BA0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7C70);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7D10);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7DE0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7E90);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7F20);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E7FD0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8060);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8100);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8180);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8240);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8330);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8400);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E84D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8590);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8660);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8750);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E87E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E88D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8960);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8A20);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8B10);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8BE0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8C90);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8D20);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8DB0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8E70);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E8F20);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9000);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E90F0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E91C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E92A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9370);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9420);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E94D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E95A0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9680);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9700);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E97E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E98C0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9980);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9A00);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9AA0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9B30);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9C00);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9CC0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9DA0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9E90);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008E9F70);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA050);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA120);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA190);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA230);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA300);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA3E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA4B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA550);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA610);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA680);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA760);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA800);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA8F0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EA9B0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EAA70);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EAB30);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EABF0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EACB0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EAD50);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EAE10);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EAEA0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EAF30);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EAFB0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB030);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB0D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB190);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB260);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB300);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB3E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB440);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB500);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB570);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB5D0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB660);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB6F0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB770);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB800);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB850);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB8F0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB970);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EB9E0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EBA20);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EBA90);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EBB10);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EBBA0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EBC70);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EBD30);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EBDD0);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EBE90);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EBF50);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", D_008EC020);
INCLUDE_RODATA("asm/game/nonmatchings/msg_event", gMsgEventListItems);

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
extern u32 D_008DE300[];
extern u32 D_008DE320[];
extern u32 D_008DE340[];
extern u32 D_008DE360[];
extern u32 D_008DE390[];
extern u32 D_008DE3C0[];
extern u32 D_008DE3E0[];
extern u32 D_008DE3F8[];
extern u32 D_008DE410[];
extern u32 D_008DE430[];
extern u32 D_008DE450[];
extern u32 D_008DE470[];
extern u32 D_008DE490[];
extern u32 D_008DE4B0[];
extern u32 D_008DE4C8[];
extern u32 D_008DE4E0[];
extern u32 D_008DE500[];
extern u32 D_008DE520[];
extern u32 D_008DE550[];
extern u32 D_008DE570[];
extern u32 D_008DE590[];
extern u32 D_008DE5B0[];
extern u32 D_008DE5E0[];
extern u32 D_008DE600[];
extern u32 D_008DE620[];
extern u32 D_008DE640[];
extern u32 D_008DE660[];
extern u32 D_008DE680[];
extern u32 D_008DE6A0[];
extern u32 D_008DE6B0[];
extern u32 D_008DE6D0[];
extern u32 D_008DE6E0[];
extern u32 D_008DE700[];
extern u32 D_008DE718[];
extern u32 D_008DE730[];
extern u32 D_008DE750[];
extern u32 D_008DE760[];
extern u32 D_008DE780[];
extern u32 D_008DE790[];
extern u32 D_008DE7A8[];
extern u32 D_008DE7C0[];
extern u32 D_008DE7D8[];
extern u32 D_008DE7F0[];
extern u32 D_008DE810[];
extern u32 D_008DE830[];
extern u32 D_008DE850[];
extern u32 D_008DE870[];
extern u32 D_008DE890[];
extern u32 D_008DE8B0[];
extern u32 D_008DE8D0[];
extern u32 D_008DE8E8[];
extern u32 D_008DE8F8[];
extern u32 D_008DE910[];
extern u32 D_008DE930[];
extern u32 D_008DE940[];
extern u32 D_008DE960[];
extern u32 D_008DE970[];
extern u32 D_008DE990[];
extern u32 D_008DE9B0[];
extern u32 D_008DE9D0[];
extern u32 D_008DE9E0[];
extern u32 D_008DE9E8[];
extern u32 D_008DE9F8[];
extern u32 D_008DEA08[];
extern u32 D_008DEA18[];
extern u32 D_008DEA30[];
extern u32 D_008DEA50[];
extern u32 D_008DEA70[];
extern u32 D_008DEA90[];
extern u32 D_008DEAB0[];
extern u32 D_008DEAD0[];
extern u32 D_008DEAE0[];
extern u32 D_008DEB00[];
extern u32 D_008DEB20[];
extern u32 D_008DEB40[];
extern u32 D_008DEB58[];
extern u32 D_008DEB70[];
extern u32 D_008DEC20[];
extern u32 D_008DECD0[];
extern u32 D_008DED80[];
extern u32 D_008DEE60[];
extern u32 D_008DEF30[];
extern u32 D_008DEFE0[];
extern u32 D_008DF0A0[];
extern u32 D_008DF170[];
extern u32 D_008DF1E0[];
extern u32 D_008DF2A0[];
extern u32 D_008DF360[];
extern u32 D_008DF400[];
extern u32 D_008DF4B0[];
extern u32 D_008DF580[];
extern u32 D_008DF660[];
extern u32 D_008DF730[];
extern u32 D_008DF7D0[];
extern u32 D_008DF880[];
extern u32 D_008DF920[];
extern u32 D_008DFA10[];
extern u32 D_008DFAB0[];
extern u32 D_008DFB60[];
extern u32 D_008DFC10[];
extern u32 D_008DFD10[];
extern u32 D_008DFDD0[];
extern u32 D_008DFE60[];
extern u32 D_008DFF30[];
extern u32 D_008E0040[];
extern u32 D_008E0100[];
extern u32 D_008E0190[];
extern u32 D_008E0290[];
extern u32 D_008E0320[];
extern u32 D_008E0400[];
extern u32 D_008E04C0[];
extern u32 D_008E0580[];
extern u32 D_008E0620[];
extern u32 D_008E0710[];
extern u32 D_008E07A0[];
extern u32 D_008E0850[];
extern u32 D_008E0930[];
extern u32 D_008E0A30[];
extern u32 D_008E0AE0[];
extern u32 D_008E0B40[];
extern u32 D_008E0C20[];
extern u32 D_008E0CB0[];
extern u32 D_008E0D60[];
extern u32 D_008E0E20[];
extern u32 D_008E0ED0[];
extern u32 D_008E0F90[];
extern u32 D_008E1070[];
extern u32 D_008E1130[];
extern u32 D_008E11E0[];
extern u32 D_008E1260[];
extern u32 D_008E1310[];
extern u32 D_008E13C0[];
extern u32 D_008E1480[];
extern u32 D_008E1530[];
extern u32 D_008E1610[];
extern u32 D_008E1690[];
extern u32 D_008E1750[];
extern u32 D_008E1810[];
extern u32 D_008E18B0[];
extern u32 D_008E19B0[];
extern u32 D_008E1A40[];
extern u32 D_008E1AE0[];
extern u32 D_008E1B80[];
extern u32 D_008E1C00[];
extern u32 D_008E1C90[];
extern u32 D_008E1D30[];
extern u32 D_008E1DC0[];
extern u32 D_008E1E80[];
extern u32 D_008E1F60[];
extern u32 D_008E2040[];
extern u32 D_008E20E0[];
extern u32 D_008E21B0[];
extern u32 D_008E2240[];
extern u32 D_008E2300[];
extern u32 D_008E2360[];
extern u32 D_008E23F0[];
extern u32 D_008E2480[];
extern u32 D_008E2530[];
extern u32 D_008E2610[];
extern u32 D_008E26D0[];
extern u32 D_008E27B0[];
extern u32 D_008E2890[];
extern u32 D_008E2920[];
extern u32 D_008E29D0[];
extern u32 D_008E2AA0[];
extern u32 D_008E2B60[];
extern u32 D_008E2C50[];
extern u32 D_008E2CF0[];
extern u32 D_008E2DA0[];
extern u32 D_008E2E70[];
extern u32 D_008E2F70[];
extern u32 D_008E2FF0[];
extern u32 D_008E3090[];
extern u32 D_008E3120[];
extern u32 D_008E3200[];
extern u32 D_008E32C0[];
extern u32 D_008E3380[];
extern u32 D_008E3420[];
extern u32 D_008E34C0[];
extern u32 D_008E3550[];
extern u32 D_008E35D0[];
extern u32 D_008E3670[];
extern u32 D_008E3770[];
extern u32 D_008E37F0[];
extern u32 D_008E38B0[];
extern u32 D_008E3980[];
extern u32 D_008E3A30[];
extern u32 D_008E3AD0[];
extern u32 D_008E3B70[];
extern u32 D_008E3C10[];
extern u32 D_008E3CD0[];
extern u32 D_008E3D60[];
extern u32 D_008E3DE0[];
extern u32 D_008E3EC0[];
extern u32 D_008E3F90[];
extern u32 D_008E4070[];
extern u32 D_008E4120[];
extern u32 D_008E41E0[];
extern u32 D_008E42B0[];
extern u32 D_008E4380[];
extern u32 D_008E4450[];
extern u32 D_008E44E0[];
extern u32 D_008E45A0[];
extern u32 D_008E4670[];
extern u32 D_008E4750[];
extern u32 D_008E4800[];
extern u32 D_008E4890[];
extern u32 D_008E4960[];
extern u32 D_008E4A20[];
extern u32 D_008E4AA0[];
extern u32 D_008E4B80[];
extern u32 D_008E4C60[];
extern u32 D_008E4D50[];
extern u32 D_008E4E10[];
extern u32 D_008E4EA0[];
extern u32 D_008E4F50[];
extern u32 D_008E4FD0[];
extern u32 D_008E5090[];
extern u32 D_008E5130[];
extern u32 D_008E51E0[];
extern u32 D_008E52C0[];
extern u32 D_008E53A0[];
extern u32 D_008E5470[];
extern u32 D_008E5560[];
extern u32 D_008E55E0[];
extern u32 D_008E5690[];
extern u32 D_008E5760[];
extern u32 D_008E5830[];
extern u32 D_008E5920[];
extern u32 D_008E59E0[];
extern u32 D_008E5AA0[];
extern u32 D_008E5B60[];
extern u32 D_008E5BE0[];
extern u32 D_008E5CB0[];
extern u32 D_008E5D90[];
extern u32 D_008E5E50[];
extern u32 D_008E5F40[];
extern u32 D_008E6010[];
extern u32 D_008E60F0[];
extern u32 D_008E61E0[];
extern u32 D_008E6270[];
extern u32 D_008E6340[];
extern u32 D_008E6410[];
extern u32 D_008E64F0[];
extern u32 D_008E65C0[];
extern u32 D_008E6680[];
extern u32 D_008E6750[];
extern u32 D_008E67F0[];
extern u32 D_008E68C0[];
extern u32 D_008E69A0[];
extern u32 D_008E6A80[];
extern u32 D_008E6B50[];
extern u32 D_008E6C00[];
extern u32 D_008E6C90[];
extern u32 D_008E6D40[];
extern u32 D_008E6DD0[];
extern u32 D_008E6E70[];
extern u32 D_008E6F40[];
extern u32 D_008E6FE0[];
extern u32 D_008E7060[];
extern u32 D_008E7130[];
extern u32 D_008E71A0[];
extern u32 D_008E7230[];
extern u32 D_008E72C0[];
extern u32 D_008E7390[];
extern u32 D_008E7450[];
extern u32 D_008E74F0[];
extern u32 D_008E75C0[];
extern u32 D_008E76A0[];
extern u32 D_008E7720[];
extern u32 D_008E77E0[];
extern u32 D_008E78C0[];
extern u32 D_008E79A0[];
extern u32 D_008E7A50[];
extern u32 D_008E7B00[];
extern u32 D_008E7BA0[];
extern u32 D_008E7C70[];
extern u32 D_008E7D10[];
extern u32 D_008E7DE0[];
extern u32 D_008E7E90[];
extern u32 D_008E7F20[];
extern u32 D_008E7FD0[];
extern u32 D_008E8060[];
extern u32 D_008E8100[];
extern u32 D_008E8180[];
extern u32 D_008E8400[];
extern u32 D_008E84D0[];
extern u32 D_008E8590[];
extern u32 D_008E8660[];
extern u32 D_008E8750[];
extern u32 D_008E87E0[];
extern u32 D_008E88D0[];
extern u32 D_008E8960[];
extern u32 D_008E8A20[];
extern u32 D_008E8B10[];
extern u32 D_008E8BE0[];
extern u32 D_008E8C90[];
extern u32 D_008E8D20[];
extern u32 D_008E8DB0[];
extern u32 D_008E9000[];
extern u32 D_008E90F0[];
extern u32 D_008E92A0[];
extern u32 D_008E95A0[];
extern u32 D_008E9680[];
extern u32 D_008E9700[];
extern u32 D_008E97E0[];
extern u32 D_008E98C0[];
extern u32 D_008E9980[];
extern u32 D_008E9A00[];
extern u32 D_008E9AA0[];
extern u32 D_008E9B30[];
extern u32 D_008E9C00[];
extern u32 D_008E9CC0[];
extern u32 D_008E9DA0[];
extern u32 D_008E9E90[];
extern u32 D_008EA050[];
extern u32 D_008EA120[];
extern u32 D_008EA190[];
extern u32 D_008EA230[];
extern u32 D_008EA300[];
extern u32 D_008EA3E0[];
extern u32 D_008EA4B0[];
extern u32 D_008EA550[];
extern u32 D_008EA610[];
extern u32 D_008EA680[];
extern u32 D_008EA760[];
extern u32 D_008EA800[];
extern u32 D_008EA8F0[];
extern u32 D_008EA9B0[];
extern u32 D_008EAA70[];
extern u32 D_008EAB30[];
extern u32 D_008EABF0[];
extern u32 D_008EACB0[];
extern u32 D_008EAD50[];
extern u32 D_008EAE10[];
extern u32 D_008EAEA0[];
extern u32 D_008EAF30[];
extern u32 D_008EAFB0[];
extern u32 D_008EB030[];
extern u32 D_008EB0D0[];
extern u32 D_008EB190[];
extern u32 D_008EB260[];
extern u32 D_008EB300[];
extern u32 D_008EB3E0[];
extern u32 D_008EB440[];
extern u32 D_008EB500[];
extern u32 D_008EB570[];
extern u32 D_008EB5D0[];
extern u32 D_008EB660[];
extern u32 D_008EB6F0[];
extern u32 D_008EB770[];
extern u32 D_008EB800[];
extern u32 D_008EB850[];
extern u32 D_008EB8F0[];
extern u32 D_008EB970[];
extern u32 D_008EB9E0[];
extern u32 D_008EBA20[];
extern u32 D_008EBA90[];
extern u32 D_008EBB10[];
extern u32 D_008EBBA0[];
extern u32 D_008EBC70[];
extern u32 D_008EBD30[];
extern u32 D_008EBDD0[];
extern u32 D_008EBE90[];
extern u32 D_008EBF50[];
extern u32 D_008EC020[];
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
extern char D_008E8240[];
extern char D_008E8330[];
extern char D_008E8E70[];
extern char D_008E8F20[];
extern char D_008E91C0[];
extern char D_008E9370[];
extern char D_008E9420[];
extern char D_008E94D0[];
extern char D_008E9F70[];
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
