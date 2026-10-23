/*
 * The lobby's message dialog, its help line and its title.
 *
 * - The message dialog (LobbyDialog_*, LobbyText_Open): one message of
 *   gLobbyMsgTexts, a waiting text or a countdown, in a box centred on the
 *   screen that grows in over a few frames. Its kind (enum LobbyDialogKind)
 *   adds a choice (yes or no, save or quit, a list), a page count, or makes
 *   the text blink. LobbyDialog_Draw draws it each frame, and
 *   LobbyDialog_ShowConnMsg keeps the connection's waiting message up.
 * - The help line at the foot of the screen (LobbyHelp_*), typed out one
 *   character a frame, and the screen's title (LobbyTitle_*).
 * - LobbyMenu_DrawNameId, a user's name and id under their labels.
 */
#include "common.h"
#include "lobby_menu.h"
#include "texture.h"
#include "render_state.h"
#include "lobby_text.h"
#include "em.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "pad.h"
#include "browser.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "net_mode.h"
#include "lobby_dialog_box.h"
#include "draw_helpers.h"
#include "font.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_queries.h"
#include "lobby_town.h"
#include "lobby_village.h"
#include "menu_widgets.h"
#include "name_tags.h"
#include "libc.h"

/* As this file's calls use it, with s16 coordinates and an s32 kind: with the definition's
 * s32 coordinates and s16 kind, LobbyDialog_DrawWin's and LobbyDialog_Draw's calls compile
 * differently. The unit's own declaration: */
void Win_Draw(s16, s16, s16, s16, s32, s32, s32);

static void LobbyHelp_Reset(void);
extern LobbyMenu *gLobbyMenu;
extern char *gLobbyTitleText;
extern s8 gLobbyDialogChoice;
extern u8 gMenuInputLock;
extern s16 gLobbyDialogGrow;
extern char gLobbyDialogMemberId[8];
extern char gLobbyDialogMemberName[17];
extern s32 gMsgWinTimer;
extern s32 gMsgWinSubTimer;
extern s16 gMsgWinX;
extern s16 gMsgWinY;
extern s16 gMsgWinW;
extern s32 gLobbyDialogMsg;
extern u8 gLobbyDialogOpen;
void Text_PrintCodes(s16 max, const char *fmt, ...);
extern s32 gLobbyDialogWaitText;
extern s32 gLobbyDialogCountdown;
extern s16 gLobbyDialogW;
extern s16 gLobbyDialogH;
extern u8 gLobbyDialogKind;
extern char *gMsgWinText;
extern void (*gLobbyConnSteps[])(void);      /* the connection's steps */
/* Where the message box sits: centred on the 640 x 448 screen. */
#define SCREEN_CENTER_X 320

#define SCREEN_CENTER_Y 224
/* The dialog's colours. */
#define DIALOG_FRAME_COLOR 0xBDB59F6C   /* the windows' frame (Win_Draw), the underlines */

#define DIALOG_FILL_COLOR 0x404020      /* the message box's fill */
#define CHOICE_BAR_COLOR 0xB0008000     /* the bar under the chosen answer (Menu_DrawGlowBar) */
#define CHOICE_ON_COLOR 0x807F5230      /* the chosen answer's text */
#define CHOICE_OFF_COLOR 0x80404040     /* the other answer's text */
#define TEXT_WHITE 0x807F7F7F
#define DIALOG_STATE_SIZE 60        /* the dialog's state, from gLobbyDialogMsg on (LobbyDialog_Clear) */
#define DIALOG_GROW_FRAMES 3        /* the box's opening (LobbyDialog_Grow) */
#define DIALOG_LARGE_W 500          /* the box of a waiting text or a countdown */
#define DIALOG_LARGE_H 380
#define DIALOG_LINE_H 20            /* a line of text */
#define DIALOG_MARGIN 60            /* the box's size beyond its text */
#define DIALOG_BLINK_PERIOD 40      /* frames: gLobbyDialogBlink's cycle */
/* Texts of the lobby's text tables that the dialog and the name line draw. */
#define TEXT_YES (gLobbyLabelTexts[LOBBY_LABEL_YES])

#define TEXT_NO (gLobbyNoSaveTexts[LOBBY_NO_SAVE_NO])
#define TEXT_SAVE (gLobbyNoSaveTexts[LOBBY_NO_SAVE_SAVE])
#define TEXT_QUIT_NO_SAVE (gLobbyButtonTexts[LOBBY_BUTTON_QUIT_NO_SAVE])
#define TEXT_BACK (gLobbyButtonTexts[LOBBY_BUTTON_BACK])
#define TEXT_NAME_LABEL (gLobbyHelpTexts[LOBBY_HELP_NAME_LABEL])
#define TEXT_ID_LABEL (gLobbyLabelTexts[LOBBY_LABEL_ID])
/* The text that nothing is offered now (DIALOG_PAGES without pages):
 * gLobbyLabelTexts[LOBBY_LABEL_NOTHING_OFFERED] under a name of its own (a
 * linker alias), which the match needs: read through the table, it is loaded
 * into another register. */
extern char *gTextNoneNow;

/* Opens the dialog on a text of its own (the login's information, a tagged text) to
 * wait on: DIALOG_WAIT, in the large box. */
void LobbyDialog_OpenWait(void *text)
{
    gLobbyDialogWaitText = (s32) text;
    gLobbyDialogKind = DIALOG_WAIT;
    gLobbyDialogW = DIALOG_LARGE_W;
    gLobbyDialogH = DIALOG_LARGE_H;
    LobbyDialog_ClearMember();
}

/* As LobbyDialog_OpenWait, with a countdown of countdown frames shown in seconds
 * (DIALOG_COUNTDOWN). */
void LobbyDialog_OpenCountdown(void *text, s32 countdown)
{
    gLobbyDialogCountdown = countdown;
    gLobbyDialogWaitText = (s32) text;
    gLobbyDialogKind = DIALOG_COUNTDOWN;
    gLobbyDialogW = DIALOG_LARGE_W;
    gLobbyDialogH = DIALOG_LARGE_H;
    LobbyDialog_ClearMember();
}

/* Sets the dialog's choice and the menu's yes/no cursor (0 yes). */
void Menu_SetYesNo(s8 choice)
{
    gLobbyDialogChoice = choice;
    gLobbyMenu->yesNo = choice;
}

/* Clears the dialog's state (DIALOG_STATE_SIZE bytes from gLobbyDialogMsg). */
s32 LobbyDialog_Clear(void)
{
    return (s32)memset(&gLobbyDialogMsg, 0, DIALOG_STATE_SIZE);
}

/* Starts the box's opening: it grows in over DIALOG_GROW_FRAMES frames, and the menu
 * takes no input for 9. */
void LobbyDialog_Grow(void)
{
    gLobbyDialogGrow = DIALOG_GROW_FRAMES;
    gMenuInputLock = 9;
}

/* The frames left of the box's opening; negative once it is open. */
s16 LobbyDialog_GetGrow(void)
{
    return gLobbyDialogGrow;
}

/* The user whose id and name the dialog shows under its text (LobbyDialog_DrawMember). */
void LobbyDialog_SetMember(void *id, void *name)
{
    memcpy(&gLobbyDialogMemberId, id, sizeof(gLobbyDialogMemberId));
    memcpy(&gLobbyDialogMemberName, name, sizeof(gLobbyDialogMemberName));
}

/* Clears that user: the dialog shows none. */
void LobbyDialog_ClearMember(void)
{
    memset(&gLobbyDialogMemberId, 0, sizeof(gLobbyDialogMemberId));
    memset(&gLobbyDialogMemberName, 0, sizeof(gLobbyDialogMemberName));
}

extern char *gLobbyDialogText;
extern s16 gLobbyDialogSpare;           /* cleared when a message opens; nothing reads it */
extern u16 gLobbyDialogBlink;
extern s16 gLobbyDialogX;
extern s16 gLobbyDialogY;
extern s8 gLobbyDialogLines;
extern s8 gLobbyDialogNoLine;
s32 Text_DisplayLength(char *s);

/*
 * Opens the dialog on message msg of gLobbyMsgTexts: its kind from the message's flags,
 * the screen dimmed behind it when LOBBY_MSG_DIM is set, no user shown. The box is
 * sized to the text, 10 pixels a byte of its longest line and DIALOG_LINE_H a line
 * (two lines more for an answer below it), and centred.
 */
void LobbyText_Open(s32 msg)
{
    LobbyMsgDef *def;
    char *line;
    char *cur;
    s32 kind;
    s32 n;
    s32 w = 0;
    s32 len;
    s32 boxW;
    s32 boxH;

    gLobbyDialogMsg = msg;
    gLobbyDialogText = gLobbyMsgTexts[msg].text;
    def = &gLobbyMsgTexts[msg];
    if (def->flags & LOBBY_MSG_DIM) {
        gLobbyDialogNoLine = 0;
    } else {
        gLobbyDialogNoLine = 1;
    }
    memset(gLobbyDialogMemberId, 0, sizeof(gLobbyDialogMemberId));
    memset(gLobbyDialogMemberName, 0, sizeof(gLobbyDialogMemberName));
    cur = gLobbyDialogText;
    n = 0;
    kind = def->flags & ~LOBBY_MSG_DIM;
    gLobbyDialogOpen = 1;
    gLobbyDialogKind = kind;
    gLobbyDialogSpare = 0;
    gLobbyDialogBlink = 0;
    line = cur;
    if (*cur != 0) {
        do {
            cur = strchr(cur, '\n');
            n++;
            if (cur != NULL) {
                len = cur - line;
                line = cur;
            } else {
                len = Text_DisplayLength(line);
            }
            if (len < 0) {
                len = -len;
            }
            if (w < len) {
                w = len;
            }
            if (cur == NULL) {
                break;
            }
            cur++;
        } while (*cur != 0);
    }
    switch (gLobbyDialogKind) {
    case DIALOG_YES_NO:
    case DIALOG_SAVE_QUIT:
        gLobbyDialogChoice = 0;
    case DIALOG_OK:
        n += 2;
        break;
    }
    boxW = w * 10 + DIALOG_MARGIN;
    gLobbyDialogLines = n;
    boxH = n * DIALOG_LINE_H + DIALOG_MARGIN;
    gLobbyDialogX = SCREEN_CENTER_X - boxW / 2;
    gLobbyDialogY = SCREEN_CENTER_Y - boxH / 2;
    gLobbyDialogW = boxW;
    gLobbyDialogH = boxH;
}

/* The screen's title (LobbyTitle_Draw): entry idx of gLobbyTitleTexts. */
void LobbyTitle_Set(s32 idx)
{
    gLobbyTitleText = gLobbyTitleTexts[idx];
}

/* Starts the help line's typing again, in its usual place at the foot of the screen. */
static void LobbyHelp_Reset(void)
{
    gMsgWinTimer = 0;
    gMsgWinSubTimer = 0;
    gMsgWinX = 57;
    gMsgWinY = 364;
    gMsgWinW = 580;
}

/* The help line shows entry i of gLobbyHelpTexts. */
void LobbyHelp_Set(s32 i)
{
    gMsgWinText = gLobbyHelpTexts[i];
    LobbyHelp_Reset();
}

/* The help line shows text. */
void LobbyHelp_SetText(void *text)
{
    gMsgWinText = text;
    LobbyHelp_Reset();
}

/* Moves the help line's window: its text at (x, y), width w. */
void LobbyHelp_SetBox(s16 x, s16 y, s16 w)
{
    gMsgWinX = x;
    gMsgWinY = y;
    gMsgWinW = w;
}

/* A window (Win_Draw's frame style 2) at (x, y), w by h, filled with fillColor, or
 * with a dark grey when that is 0. */
void LobbyDialog_DrawWin(s16 x, s16 y, s16 w, s16 height, s32 fillColor)
{
    if (fillColor != 0) {
        Win_Draw(x, y, w, height, DIALOG_FRAME_COLOR, fillColor, 2);
        return;
    }
    Win_Draw(x, y, w, height, DIALOG_FRAME_COLOR, 0x80202020, 2);
}

extern u32 gMsgBoxGradient[4];              /* the message box's gradient */
void Tex_Bind(s32 a);
void Gfx_SetTexFilter(s32 a);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
/* gLobbyDialogKind read as a struct member: the compiler then loads it again after
 * each store, as the game's code does. */
typedef struct LobbyMsgKind {
    u8 kind;
} LobbyMsgKind;
#define MSG_KIND (((LobbyMsgKind *)&gLobbyDialogKind)->kind)

/*
 * Draws the dialog of kind gLobbyDialogKind (enum LobbyDialogKind): the dimmed screen,
 * the box with its shadow (smaller while it grows in: 15% of its size a frame left),
 * then, once it is open, its text and its answers with the bar under the chosen one.
 */
void LobbyDialog_Draw(void)
{
    s16 w;
    s16 height;
    s16 x;
    s16 y;
    char *text;
    char *newline;
    PrimGradRect shadow; /* the box's shadow, a gradient */
    char buf[128];

    if (MSG_KIND == DIALOG_NONE) {
        return;
    }
    if (++gLobbyDialogBlink >= DIALOG_BLINK_PERIOD) {
        gLobbyDialogBlink = 0;
    }
    if (MSG_KIND == DIALOG_WAIT) {
        Gfx_DrawBlackLineFixed();
    } else if (gLobbyDialogNoLine == 0 || MSG_KIND == DIALOG_DIM) {
        Gfx_DrawBlackLineFixed();
        if (MSG_KIND == DIALOG_DIM) {
            return;
        }
    }
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    shadow.color[0] = gMsgBoxGradient[0];
    shadow.color[1] = gMsgBoxGradient[1];
    shadow.color[2] = gMsgBoxGradient[2];
    shadow.color[3] = gMsgBoxGradient[3];
    switch (MSG_KIND) {
    case DIALOG_CHOICE2:
    case DIALOG_CHOICES:
        {
            s16 boxX;
            s16 boxY;
            s16 boxW;
            s16 boxH;

            if (gLobbyDialogGrow >= 0) {
                boxW = gLobbyDialogW - 0.15f * gLobbyDialogW * gLobbyDialogGrow;
                boxH = gLobbyDialogH - 0.15f * gLobbyDialogH * gLobbyDialogGrow;
            } else {
                boxW = gLobbyDialogW;
                boxH = gLobbyDialogH;
            }
            boxX = SCREEN_CENTER_X - boxW / 2;
            boxY = SCREEN_CENTER_Y - boxH / 2;
            shadow.x0 = boxX - 4;
            shadow.y0 = boxY - 4;
            shadow.x1 = boxX + boxW + 4;
            shadow.y1 = boxY + boxH + 4;
            Prim_FillGradRect(&shadow);
            Win_Draw(boxX, boxY, boxW, boxH, DIALOG_FRAME_COLOR, DIALOG_FILL_COLOR, 7);
        }
        break;
    case DIALOG_PAGES:
        break;
    default:
        if (gLobbyDialogGrow >= 0) {
            w = gLobbyDialogW - 0.15f * gLobbyDialogW * gLobbyDialogGrow;
            height = gLobbyDialogH - 0.15f * gLobbyDialogH * gLobbyDialogGrow;
        } else {
            w = gLobbyDialogW;
            height = gLobbyDialogH;
        }
        x = SCREEN_CENTER_X - w / 2;
        y = SCREEN_CENTER_Y - height / 2;
        shadow.x0 = x - 4;
        shadow.y0 = y - 4;
        shadow.x1 = x + w + 4;
        shadow.y1 = y + height + 4;
        Prim_FillGradRect(&shadow);
        Win_Draw(x, y, w, height, DIALOG_FRAME_COLOR, DIALOG_FILL_COLOR, 1);
        if (MSG_KIND == DIALOG_BLINK && gLobbyDialogBlink >= 21) {
            return;
        }
        break;
    }
    if (gLobbyDialogGrow >= 0) {
        gLobbyDialogGrow--;
        return;
    }
    Font_SetSpacing(20, 20);
    switch (MSG_KIND) {
    case DIALOG_COUNTDOWN:
        TagText_Print((char *)gLobbyDialogWaitText, 100.0f, 60.0f);
        Font_SetColor(0x807F7F28);
        Font_SetSpacing(20, 20);
        if (--gLobbyDialogCountdown < 0) {
            gLobbyDialogCountdown = 0;
            Lobby_DrawPressButton();
            return;
        }
        Font_SetPos(500, 64);
        Text_PrintfColored("%2d", gLobbyDialogCountdown / 30 + 1);
        return;
    case DIALOG_WAIT:
        TagText_Print((char *)gLobbyDialogWaitText, 100.0f, 60.0f);
        if (gLobby->level0Kicked == 0 && gLobby->level1Kicked == 0 && gLobby->level2Kicked == 0) {
            Lobby_DrawPressButton();
        }
        break;
    case DIALOG_PAGES:
        Gfx_DrawBlackLine(70, 34, DIALOG_LARGE_W, DIALOG_LARGE_H);
        if (gLobbyMenu->pages != 0) {
            Menu_DrawPageNum(410, 374, gLobbyMenu->page, gLobbyMenu->pages, 0);
            TagText_Print((char *)gLobbyDialogWaitText, 120.0f, 60.0f);
        } else {
            text = gTextNoneNow;
            Text_DrawShadow(Text_CenterX(text, SCREEN_CENTER_X, 20), 160, 1, 4, text);
        }
        if (gFrameTick & 0x10) {
            Lobby_DrawButtonIcon(120, 372, 1);
        }
        Font_SetPos(150, 376);
        Font_SetColor(TEXT_WHITE);
        Font_SetSpacing(18, 18);
        Text_PrintfColored(TEXT_BACK);
        break;
    default:
        y = gLobbyDialogY + 30;
        text = gLobbyDialogText;
        while (*text != 0) {
            memcpy(buf, text, sizeof(buf));
            newline = strchr(buf, '\n');
            if (newline != NULL) {
                *newline = 0;
            }
            {
                s32 lineX = Text_CenterX(buf, SCREEN_CENTER_X, 20);

                Font_SetPos(lineX, y);
            }
            Text_Printf(buf);
            if (newline == NULL) {
                break;
            }
            y += DIALOG_LINE_H;
            text += Lobby_Strlen(buf) + 1;
        }
        switch (MSG_KIND) {
        case DIALOG_YES_NO:
            if (gLobbyDialogChoice == 0) {
                Font_SetColor(CHOICE_ON_COLOR);
                y += 40;
                Font_SetPos(250, y);
                Text_PrintfColored("%s", TEXT_YES);
                Font_SetColor(CHOICE_OFF_COLOR);
                Font_SetPos(340, y);
                Text_PrintfColored("%s", TEXT_NO);
                Menu_DrawGlowBar(270, y - 2, 80, 24, CHOICE_BAR_COLOR);
            } else {
                Font_SetColor(CHOICE_OFF_COLOR);
                y += 40;
                Font_SetPos(250, y);
                Text_PrintfColored("%s", TEXT_YES);
                Font_SetColor(CHOICE_ON_COLOR);
                Font_SetPos(340, y);
                Text_PrintfColored("%s", TEXT_NO);
                Menu_DrawGlowBar(370, y - 2, 100, 24, CHOICE_BAR_COLOR);
            }
            break;
        case DIALOG_OK:
            Font_SetColor(CHOICE_ON_COLOR);
            y += 40;
            Font_SetPos(300, y);
            Text_PrintfColored("%s", TEXT_YES);
            Menu_DrawGlowBar(SCREEN_CENTER_X, y - 2, 80, 24, CHOICE_BAR_COLOR);
            break;
        case DIALOG_CHOICE2:
            y += 40;
            if (gLobbyDialogChoice == 0) {
                Menu_DrawGlowBar(SCREEN_CENTER_X, y - 162, 240, 24, CHOICE_BAR_COLOR);
            } else {
                Menu_DrawGlowBar(SCREEN_CENTER_X, y - 122, 240, 24, CHOICE_BAR_COLOR);
            }
            break;
        case DIALOG_CHOICES:
            y += 40;
            Menu_DrawGlowBar(SCREEN_CENTER_X, y - (10 - gLobbyDialogChoice * 2) * DIALOG_LINE_H - 2, 240, 24,
                             CHOICE_BAR_COLOR);
            break;
        case DIALOG_SAVE_QUIT:
            if (gLobbyDialogChoice == 0) {
                Font_SetColor(CHOICE_ON_COLOR);
                y += 40;
                Font_SetPos(196, y);
                Text_PrintfColored("%s", TEXT_SAVE);
                Font_SetColor(CHOICE_OFF_COLOR);
                Font_SetPos(340, y);
                Text_PrintfColored("%s", TEXT_QUIT_NO_SAVE);
                Menu_DrawGlowBar(246, y - 2, 140, 24, CHOICE_BAR_COLOR);
            } else {
                Font_SetColor(CHOICE_OFF_COLOR);
                y += 40;
                Font_SetPos(196, y);
                Text_PrintfColored("%s", TEXT_SAVE);
                Font_SetColor(CHOICE_ON_COLOR);
                Font_SetPos(340, y);
                Text_PrintfColored("%s", TEXT_QUIT_NO_SAVE);
                Menu_DrawGlowBar(417, y - 2, 194, 24, CHOICE_BAR_COLOR);
            }
            break;
        }
        break;
    }
}

/* Online, once the box is open: the name and id of the user the dialog is about
 * (LobbyDialog_SetMember), if any. */
void LobbyDialog_DrawMember(void)
{
    if (Game_IsOnline() != 0 && gLobbyDialogGrow < 0 && gLobbyDialogMemberId[0] != 0) {
        LobbyMenu_DrawNameId(152, gLobbyDialogMemberName, gLobbyDialogMemberId);
    }
}

/* Draws the screen's title (LobbyTitle_Set) in large letters, centred at the top. */
void LobbyTitle_Draw(void)
{
    NetMode_DrawBigText(gLobbyTitleText, -1, 28, 34);
}

/*
 * Draws the help line: its window and gMsgWinText typed out one character a frame
 * (gMsgWinTimer, the characters shown; -1 all of them, once 256 are or on
 * PAD_TRIANGLE).
 */
void LobbyHelp_Draw(void)
{
    u16 pad;

    pad = LobbyPad_Get(0);
    Win_Draw((s16)(gMsgWinX - 16), (s16)(gMsgWinY - 8), gMsgWinW, 67, DIALOG_FRAME_COLOR, 0x70402020, 0);
    if (gMsgWinTimer >= 0) {
        if (++gMsgWinSubTimer > 0) {
            gMsgWinSubTimer = 0;
            if (++gMsgWinTimer > 256) {
                gMsgWinTimer = -1;
            }
        }
    }
    if (pad & PAD_TRIANGLE) {
        gMsgWinTimer = -1;
    }
    Font_SetSpacing(18, 18);
    Font_SetColor(TEXT_WHITE);
    if (gMsgWinText != NULL) {
        Font_SetPos(gMsgWinX, gMsgWinY);
        Text_PrintCodes(gMsgWinTimer, gMsgWinText);
    }
}

extern s32 gTownMode;

/*
 * A user's name and id (made full width) after their labels, each underlined, the name
 * at y; not while the lobby, the village or the town is being left, nor in the town's
 * mode TOWN_MODE_ROOM_LEAVE (a message box up: Town_CheckQuestParty).
 */
void LobbyMenu_DrawNameId(s16 y, char *name, char *id)
{
    char buf[64];

    if (LobbyVillage_IsLeaving() == 1 || LobbyTown_IsLeaving() == 1 || gTownMode == TOWN_MODE_ROOM_LEAVE) {
        return;
    }
    if (gLobby->phase != LOBBY_PHASE_EXIT) {
        Font_SetSpacing(20, 20);
        Sjis_Widen(name, buf);
        Text_DrawShadow(238, y, 1, 4, buf);
        Sjis_Widen(id, buf);
        Text_DrawShadow(238, y + 30, 1, 4, buf);
        Text_DrawShadow(178, y, 1, 4, TEXT_NAME_LABEL);
        Text_DrawShadow(178, y + 30, 1, 4, TEXT_ID_LABEL);
        Gfx_DrawRectFrame(178, y + 22, 284, 1, DIALOG_FRAME_COLOR);
        Gfx_DrawRectFrame(178, y + 52, 284, 1, DIALOG_FRAME_COLOR);
    }
}

/*
 * Shows the connection's waiting message for stage (1 the quest is being prepared,
 * 2 on the way to the town, 3 the network is being prepared; 0 none), opening it
 * unless it is already up, and draws the dialog.
 */
void LobbyDialog_ShowConnMsg(s32 stage)
{
    switch (stage) {
    case 0:
        break;
    case 1:
        if (gLobbyDialogOpen == 0 || gLobbyDialogMsg != LOBBY_TEXT_QUEST_PREPARING) {
            LobbyText_Open(LOBBY_TEXT_QUEST_PREPARING);
        }
        break;
    case 2:
        if (gLobbyDialogOpen == 0 || gLobbyDialogMsg != LOBBY_TEXT_TO_TOWN) {
            LobbyText_Open(LOBBY_TEXT_TO_TOWN);
        }
        break;
    case 3:
        if ((gLobbyDialogOpen == 0) || (gLobbyDialogMsg != LOBBY_TEXT_NET_PREPARING)) {
            LobbyText_Open(LOBBY_TEXT_NET_PREPARING);
        }
        break;
    }
    LobbyDialog_Draw();
}

/* The message box's shadow, its four corners' colours (LobbyDialog_Draw). */
u32 gMsgBoxGradient[4] = {
#include "lobby/00819420.inc"
};
