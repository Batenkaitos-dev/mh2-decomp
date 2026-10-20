/*
 * The lobby's leave and room steps, the short mail's sending, and the
 * character select menu.
 * - LobbyStep_Level2Leave leaves the quest room (level 2) and
 *   LobbyStep_SetOutLeave takes back the player's setting out;
 *   Lobby_HandleLeaveRequest starts leaving the screen, the room, the
 *   village or the town when the server has closed or sent the player out.
 * - LobbyStep_ShortMailSend sends a short mail (6704).
 * - The character select menu (IdMenu_*, gIdMenuScreens): the account's
 *   three characters, each with its id and name (LobbyState.charaIds,
 *   charaNames), one of which the player picks, or quits.
 * The request steps follow the lobby's pattern (LobbyStep_* in
 * lobby_steps.c's opening comment): a tag in LobbyState.awaitReply, the
 * reply timer, and a reply handler that moves the step on.
 */
#include "common.h"
#include "texture.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "sprite.h"
#include "em.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "player.h"
#include "pad.h"
#include "lobby_labels.h"
void Font_SetSpacing(u8 a, u8 b);

void EquipPreview_Hide(u8 hide);
void EquipPreview_Free(void);
s32 LobbyReq_Level2Leave(void (*onReply)(NetMsg, NetMsg *));
void LobbyStep_Level2LeaveDone(NetMsg m, NetMsg *pm);
s32 HunterCard_SendChanged(void);
void HunterCard_Build(void *out);
extern NetUser gNetUsers[10];
u16 LobbyPad_Get(s32 a);
void Menu_SetYesNo(s8 a);
s32 Menu_YesNoInput(void);
void FadeA_Start(s32 a);
u8 FadeA_GetState(void);

extern MailEntry gShortMailMsg;
s32 LobbyReq_ShortMailSend(void *id, char *s, void (*onReply)(NetMsg, NetMsg *));
void LobbySe_Play(s32 se);
void LobbyText_Open(s32 msg);
void LobbyDialog_Grow(void);
void LobbyStep_ShortMailSendDone(NetMsg m, NetMsg *pm);
void *memcpy(void *dst, const void *src, u32 n);

/* the redraw hook's argument: only the text size is read */
typedef struct TextBox {
    u8 pad[0x18];
    s32 size;
} TextBox;
void Lobby_Disconnect(s32);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
s32 LobbyReq_SetOut(u8 arg0, void (*onReply)(NetMsg, NetMsg *));
void LobbyStep_SetOutLeaveDone(NetMsg m, NetMsg *pm);
extern s8 gIdMenuResult;
extern void (*gIdMenuScreens[])(void);
void IdMenu_Draw(void);
void *memset(void *dst, s32 c, u32 n);
char *strcpy(char *dst, const char *src);
s32 sprintf(char *dst, const char *fmt, ...);  /* sprintf */
void Sjis_Widen(char *src, char *dst);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void IdMenu_Redraw(TextBox *t);
void LobbyTitle_Set(s32 a);
void LobbyHelp_Set(s32 a);
s32 memcmp(void *a, void *b, s32 n);
void Text_SetSize(s32 size);
void LobbyDialog_Draw(void);
s16 LobbyDialog_GetGrow(void);
void LobbyMenu_DrawNameId(s16 y, char *name, char *id);

s32 LobbyNet_GetErrText(char *a);
void Money_PayPending(u8 silent);
void LobbyDialog_OpenWait(void *text);
void LobbyMenu_Init(void);
void Lobby_ClearRequests(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
s32 Lobby_GetPlace(void);
void Lobby_StartRoomPhase(void);
void LobbyRoom_BeginClose(u8 arg0);
void LobbyVillage_BeginLeave(void);
void LobbyTown_BeginLeave(void);
void CamFocus_End(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern LobbyMenu *gLobbyMenu;

/*
 * Leaves the quest room (level 2): hides the equipment preview, sends
 * LobbyReq_Level2Leave (6501) and waits for its reply; then clears the
 * player's quest status and arena role, sends the hunter card again and
 * rebuilds it into the player's member and user records. Steps on
 * LobbyState.subStep. Unlike the other steps it returns 1 once done and
 * 0 before (its callers test for 1); a failed reply leaves the lobby.
 */
s32 LobbyStep_Level2Leave(void)
{
    u8 *subStepPtr;
    LobbyState *lobby;
    u8 step;

    lobby = gLobby;
    subStepPtr = &lobby->subStep;
    step = *subStepPtr;
    switch (step) {
    case 0:
        *subStepPtr = (u8)(step + 1);
        EquipPreview_Hide(1);
        EquipPreview_Free();
        LobbyMenu_SetDraw(0, 0, NULL, 0);
    case 1:
        gLobby->subStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL2_LEAVE;
        LobbyReq_Level2Leave(LobbyStep_Level2LeaveDone);
        break;
    case 2:
        LobbyMenu_TickTimer();
        break;
    case 3:
        *subStepPtr = 0;
        gPlayers[gScene.localPlayer].questStatus = 0;
        gPlayerText.card.arenaRole = 0;
        HunterCard_SendChanged();
        HunterCard_Build(gLobby->members[gScene.localPlayer].text);
        HunterCard_Build(gNetUsers[gScene.localPlayer].text);
        return 1;
    }
    return 0;
}

/*
 * LobbyStep_Level2Leave's reply handler (LOBBY_AWAIT_LEVEL2_LEAVE): the
 * quest taken and paid flags are cleared either way; on success subStep
 * goes on, else the lobby is left (LOBBY_QUIT_FAILED).
 */
void LobbyStep_Level2LeaveDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL2_LEAVE) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                gLobby->questTaken = 0;
                gLobby->questPaid = 0;
                gLobby->subStep++;
            } else {
                gLobby->questTaken = 0;
                gLobby->questPaid = 0;
                Lobby_Disconnect(LOBBY_QUIT_FAILED);
            }
        }
    }
}

/*
 * Takes back the player's setting out: sends LobbyReq_SetOut(0) (6504)
 * and waits for the reply. Steps on LobbyState.subStep (3: the reply
 * failed). Returns LOBBY_STEP_BUSY while waiting, then LOBBY_STEP_DONE or
 * LOBBY_STEP_FAILED.
 */
s32 LobbyStep_SetOutLeave(void)
{
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_SET_OUT_LEAVE;
        LobbyReq_SetOut(0, LobbyStep_SetOutLeaveDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        return LOBBY_STEP_DONE;
    case 3:
        gLobby->subStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_SetOutLeave's reply handler (LOBBY_AWAIT_SET_OUT_LEAVE): on
 * success subStep goes on and LobbyState.setOutOn is cleared; else the
 * error text is shown in a wait dialog and subStep becomes 3.
 */
void LobbyStep_SetOutLeaveDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_SET_OUT_LEAVE) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                gLobby->subStep++;
                gLobby->setOutOn = 0;
            } else {
                LobbyNet_GetErrText(gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobby->subStep = 3;
            }
        }
    }
}

/*
 * Starts leaving when something has ended the player's place, and
 * returns 1 when it did, else 0. Only while connected:
 * - the party is ready (playersListed, set by LobbyRoute_PartyReady): the
 *   room phase starts (Lobby_StartRoomPhase);
 * - the room has closed (LobbyState.roomCloseKind): with its error text in
 *   a wait dialog (1) or without (2);
 * - the server sent the player out of the village (LOBBY_PHASE_VILLAGE,
 *   level0Kicked): the village is left;
 * - in the town (LOBBY_PHASE_TOWN, a level entered), sent out of it
 *   (level1Kicked) with no quest taken: the town is left; sent out of the
 *   quest room (level2Kicked) with a quest taken: the pending payment is
 *   made, the error text shown and the town menu set to
 *   TOWN_MODE_ROOM_LEAVE, which leaves the room.
 * The cases that start nothing jump to the one return 0 at the end, as
 * the game's code does; written as returns, they compile differently.
 */
s32 Lobby_HandleLeaveRequest(void)
{
    s8 closeKind;
    u8 phase;
    u8 questTaken;

    if (gLobby->connected != 0) {
        if (gLobby->playersListed != 0) {
            Lobby_StartRoomPhase();
            return 1;
        }
        closeKind = gLobby->roomCloseKind;
        if (closeKind == ROOM_CLOSE_WITH_TEXT) {
            LobbyRoom_BeginClose(0);
            LobbyDialog_OpenWait(gLobby->errText);
            return 1;
        }
        if (closeKind == ROOM_CLOSE_QUIET) {
            LobbyRoom_BeginClose(1);
            return 1;
        }
        phase = gLobby->phase;
        if (phase == LOBBY_PHASE_VILLAGE) {
            if (gLobby->level0Kicked == LEAVE_SENT_OUT) {
                LobbyVillage_BeginLeave();
                return 1;
            }
            goto none;
        }
        if ((phase == LOBBY_PHASE_TOWN) && (Lobby_GetPlace() != LOBBY_PLACE_LEVEL0)) {
            questTaken = gLobby->questTaken;
            if (questTaken == 0) {
                if (gLobby->level1Kicked == LEAVE_SENT_OUT) {
                    if (questTaken != 0) {
                        Money_PayPending(1U);
                    }
                    LobbyTown_BeginLeave();
                    return 1;
                }
                goto none;
            }
            if (gLobby->level2Kicked == LEAVE_SENT_OUT) {
                if (gLobby->questPaid != 0) {
                    gLobby->questPaid = 0U;
                }
                Money_PayPending(1U);
                CamFocus_End();
                LobbyDialog_OpenWait(gLobby->errText);
                gFieldControl = 0;
                gTownMode = TOWN_MODE_ROOM_LEAVE;
                gTownMenuOpen = 1;
                gLobby->awaitReply = LOBBY_AWAIT_NONE;
                gLobby->subStep = 0;
                LobbyMenu_Init();
                LobbyMenu_SetDraw(0, 0, NULL, 0);
                Lobby_ClearRequests();
                gLobbyMenu->redraw = 1;
                return 1;
            }
            goto none;
        }
        goto none;
    }
none:
    return 0;
}

/*
 * Sends the short mail msg (its addressee's id and its text) with
 * LobbyReq_ShortMailSend (6704), keeping a copy in gShortMailMsg, and
 * waits for the reply. Steps on LobbyState.seqStep. Returns
 * LOBBY_STEP_BUSY while waiting, then LOBBY_STEP_DONE, or
 * LOBBY_STEP_FAILED with the error text in a wait dialog; a step it does
 * not know leaves the lobby.
 */
s32 LobbyStep_ShortMailSend(MailEntry *msg)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        memcpy(&gShortMailMsg, msg, sizeof(MailEntry));
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_SHORT_MAIL;
        LobbyReq_ShortMailSend(msg->id, msg->text, LobbyStep_ShortMailSendDone);
        gLobby->seqStep++;
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        LobbyDialog_OpenWait(gLobby->errText);
        return LOBBY_STEP_FAILED;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    return LOBBY_STEP_BUSY;
}

/*
 * LobbyStep_ShortMailSend's reply handler (LOBBY_AWAIT_SHORT_MAIL):
 * seqStep goes on, or LOBBY_SEQ_FAILED with the error text in errText.
 */
void LobbyStep_ShortMailSendDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_SHORT_MAIL) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText(gLobby->errText);
            }
        }
    }
}

/*
 * The character select menu's frame: runs its screen
 * (gIdMenuScreens[gLobbyMenu->screen]) and draws it. Returns
 * gIdMenuResult: -1 while it runs, -2 when the player quits from the
 * list, 0 once the quit's fade is over.
 */
s32 IdMenu_Run(void)
{
    gIdMenuResult = -1;
    gIdMenuScreens[gLobbyMenu->screen]();
    IdMenu_Draw();
    return gIdMenuResult;
}

#define ID_MENU_CHARAS 3     /* the characters the menu lists */
#define CHARA_NAME_SIZE 0x11  /* each name's bytes in LobbyState.charaNames */

/*
 * The menu's first screen: sets its redraw hook, title and help, and puts
 * the cursor on the character whose name is the save's hunter name (the
 * first one when none is).
 */
void IdMenu_Start(void)
{
    s8 i;
    s32 off;

    LobbyMenu_SetDraw(0, 0, IdMenu_Redraw, 0);
    gLobbyMenu->screen++;
    LobbyTitle_Set(LOBBY_TITLE_ID_MENU);
    LobbyHelp_Set(1);
    gLobbyMenu->cursor = 0;
    for (i = 0, off = 0; i < ID_MENU_CHARAS; i++, off += CHARA_NAME_SIZE) {
        if (memcmp(gLobby->charaNames + off, (u8 *)gSave.name, 0x10) == 0) {
            gLobbyMenu->cursor = i;
            return;
        }
    }
}

/* The menu's second screen: up and down choose one of the three
 * characters (LobbyMenu.cursor), circle goes on, cross asks whether to quit
 * (LOBBY_TEXT_QUIT_ASK with yes and no). Going on shows the message for an
 * empty slot or a used one and moves to the next screen.
 * The step is reached through a pointer taken first (its address is numbered
 * before the gLobbyMenu load), and case 0 tests an s32 copy of the buttons
 * (the game re-extends them there: its second andi). */
void IdMenu_Choose(void)
{
    u16 pad;
    s32 buttons;
    u8 *stepPtr;

    pad = LobbyPad_Get(0);
    stepPtr = &gLobbyMenu->step;
    switch (*stepPtr) {
    case 0:
        buttons = pad;
        if (buttons & PAD_UP) {
            if (gLobbyMenu->cursor == 0) {
                gLobbyMenu->cursor = 2;
            } else {
                gLobbyMenu->cursor--;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        } else if (buttons & PAD_DOWN) {
            if (++gLobbyMenu->cursor > 2) {
                gLobbyMenu->cursor = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        } else if (buttons & PAD_CIRCLE) {
            *stepPtr += 1;
            gLobbyMenu->timer = 0x28;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        } else if (buttons & PAD_CROSS) {
            *stepPtr = 2;
            LobbyText_Open(LOBBY_TEXT_QUIT_ASK);
            Menu_SetYesNo(1);
            gLobbyMenu->redraw = 1;
            LobbySe_Play(LOBBY_SE_CANCEL);
        }
        break;
    case 1:
        if (gLobby->charaIds[gLobbyMenu->cursor][0] == 0) {
            LobbyText_Open(LOBBY_TEXT_CHARA_SLOT_EMPTY);
        } else {
            LobbyText_Open(LOBBY_TEXT_CHARA_SLOT_USED);
        }
        LobbyDialog_Grow();
        gLobbyMenu->screen++;
        gLobbyMenu->step = 0;
        gLobbyMenu->unk30 = 0;
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            gIdMenuResult = -2;
            break;
        case MENU_CANCEL:
            gLobbyMenu->step = 0;
            break;
        }
        break;
    }
}

/*
 * The menu's quit screen: a yes or no; yes fades out and then sets
 * gIdMenuResult to 0, no goes back to the list.
 */
void IdMenu_Quit(void)
{
    LobbyPad_Get(0);
    gLobbyMenu->redraw = 1;
    switch (gLobbyMenu->step) {
    case 0:
        gLobbyMenu->step++;
        Menu_SetYesNo(0);
        break;
    case 1:
        if (LobbyDialog_GetGrow() == -1) {
            switch (Menu_YesNoInput()) {
            case MENU_DONE:
                FadeA_Start(1);
                gLobbyMenu->step++;
                break;
            case MENU_CANCEL:
                gLobbyMenu->screen = 1;
                gLobbyMenu->step = 0;
                LobbyTitle_Set(LOBBY_TITLE_ID_MENU);
                LobbyHelp_Set(1);
                break;
            }
        }
        break;
    case 2:
        if ((u8)FadeA_GetState() != 1) {
            gIdMenuResult = 0;
        }
        break;
    }
}

/* A character's id line: character idx's id (LobbyState.charaIds, widened;
 * gLobbyWords[LOBBY_WORD_UNREGISTERED] when it is empty) at x + 0x3C after its label
 * (gLobbyLabelTexts[LOBBY_LABEL_ID]), highlighted (colour 4) on the cursor's line. */
void IdMenu_DrawId(s16 x, s16 y, u8 cur, u8 idx, u8 unused)
{
    char buf[0x1E];
    s8 color;
    LobbyState *lobby;

    memset(buf, 0, sizeof(buf));
    lobby = gLobby;
    if (lobby->charaIds[idx][0] == 0) {
        strcpy(buf, gLobbyWords[LOBBY_WORD_UNREGISTERED]);
    } else {
        Sjis_Widen(lobby->charaIds[idx], buf);
    }
    Font_SetSpacing(0x14, 0x14);
    color = (cur != idx) ? 0 : 4;
    Text_DrawShadow(x + 0x3C, y, 1, color, buf);
    Text_DrawShadow(x, y, 1, color, gLobbyLabelTexts[LOBBY_LABEL_ID]);
}

/* The same for character idx's name (LobbyState.charaNames, 0x11 bytes
 * each) after its label (gLobbyHelpTexts[LOBBY_HELP_NAME_LABEL]); an empty one is printed as
 * gLobbyWords[LOBBY_WORD_UNREGISTERED], in the narrower spacing. cur and idx are compared as
 * ints: the compiler narrows idx again there, as the game's code does. */
void IdMenu_DrawText(s16 x, s16 y, u8 cur, u8 idx, u8 unused)
{
    char buf[0x1E];
    s8 color;

    memset(buf, 0, sizeof(buf));
    if (((char *)gLobby->charaNames)[idx * CHARA_NAME_SIZE] == 0) {
        Font_SetSpacing(0x14, 0x14);
        sprintf(buf, gLobbyWords[LOBBY_WORD_UNREGISTERED]);
    } else {
        Font_SetSpacing(0x18, 0x14);
        strcpy(buf, (char *)&gLobby->charaNames[idx * CHARA_NAME_SIZE]);
    }
    color = ((s32)cur != (s32)idx) ? 0 : 4;
    Text_DrawShadow(x + 0x3C, y, 1, color, buf);
    Font_SetSpacing(0x14, 0x14);
    Text_DrawShadow(x, y, 1, color, gLobbyHelpTexts[LOBBY_HELP_NAME_LABEL]);
}

void Icon_DrawPreset(s32 a);
void Win_Draw(s32 x, s32 y, s32 w, s32 h, s32 color, s32 color2, s16 kind);
u32 Color_Pulse(void);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i, u8 mode);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 col);
void LobbyHelp_Draw(void);
void LobbyTitle_Draw(void);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 a);
s32 Sprite_Draw(Sprite *s);
void Font_SetPos(s16 x, s16 y);
void Text_Printf(char *fmt, ...);

/* Draws the character select menu: the hunter's name box, the three
 * characters with the cursor's bar, the help and title lines, and the
 * error text with its picture when there is one. */
void IdMenu_Draw(void)
{
    Sprite spr;
    s32 i;
    s16 y;

    LobbyPad_Get(0);
    Icon_DrawPreset(2);
    Font_SetSpacing(0x14, 0x14);
    Win_Draw(0x1E, 0x44, 0x10E, 0x42, 0xBDB59F6C, 0x80505000, 2);
    Text_DrawShadow(0x2A, 0x4F, 1, 5, gIdMenuTexts[ID_MENU_TEXT_NAME]);
    Text_DrawShadow(0x2A, 0x65, 1, 0, (char *)gSave.name);
    Icon_DrawMode(0x96, 0x84, 0x18, 0x18, Color_Pulse(), 0x29, 3);
    y = 0x9C;
    Win_Draw(0x1E, y, 0x10E, 0xBE, 0xBDB59F6C, 0x80502020, 3);
    for (i = 0; i < 3; i++) {
        if (i != 0) {
            Gfx_DrawRectFrame(0x32, y + 5, 0xE6, 2, 0xBDB59F6C);
        }
        if (i == gLobbyMenu->cursor) {
            Menu_DrawGlowBar(0xA5, y + 8, 0x10E, 0x3A, 0x70208080);
        }
        IdMenu_DrawId(0x28, y + 0x25, gLobbyMenu->cursor, i, i);
        IdMenu_DrawText(0x28, y + 0xF, gLobbyMenu->cursor, i, i);
        y += 0x3A;
    }
    LobbyHelp_Draw();
    LobbyTitle_Draw();
    if (gLobby->errText[0] != 0) {
        Icon_Draw(0x14A, 0x44, 0x122, 0x112, 0xFFC0C0C0, 0);
        Tex_Prepare(1, TEX_SLOT_PICTURES);
        Tex_Bind(TEX_SLOT_PICTURES);
        spr.y = 0xFA;
        spr.x = 0x1BC;
        spr.u0 = 0x7E;
        spr.w = 0x40;
        spr.v0 = 0xD2;
        spr.h = 0x40;
        spr.u1 = 0xA7;
        spr.v1 = 0xFB;
        spr.color = -1;
        Sprite_Draw(&spr);
        Font_SetPos(0x168, 0x50);
        Text_Printf(gLobby->errText);
    }
}

/* The menu's redraw hook: when LobbyMenu.redraw is set, draws the message
 * dialog and, on the confirm screen with no dialog growing, the hunter's
 * name with the id of the character under the cursor. */
void IdMenu_Redraw(TextBox *box)
{
    Text_SetSize(box->size);
    if (gLobbyMenu->redraw != 0) {
        LobbyDialog_Draw();
        if (gLobbyMenu->screen == 2 && LobbyDialog_GetGrow() == -1) {
            if (gLobby->charaIds[gLobbyMenu->cursor][0] != 0) {
                LobbyMenu_DrawNameId(0x86, (char *)gSave.name, (char *) gLobby->charaIds[gLobbyMenu->cursor]);
            }
        }
        gLobbyMenu->redraw = 0;
    }
}

/* The character select menu's screens. */
void IdMenu_Choose(void);
void IdMenu_Quit(void);
void (*gIdMenuScreens[4])(void) = {
    IdMenu_Start, IdMenu_Choose, IdMenu_Quit, NULL,
};
