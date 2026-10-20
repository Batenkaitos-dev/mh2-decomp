/*
 * The official desk (TOWN_MODE_OFFICIAL_DESK): the lobby's counter for the
 * official quests. Its window shows three rows of up to nine quests each:
 * the downloaded event quests, the next quest of the HR gate the clerk
 * offers (from gHrGateQuests), and the elder dragon raid's quests while a
 * raid runs. The player picks a row, then a quest on its quest sheet
 * (three pages); the desk checks that no quest is taken, the order terms
 * and the fee, then the room settings (the least members, a password) are
 * set and the quest is posted to the lobby (Quest_PostStep). A stamp lands
 * on the sheet and the quest is accepted as official (QuestDesk_Accept,
 * gQuestIsOfficial).
 *
 * The menu's state is an OfficialDeskMenu in gMenuWork. OfficialDesk_Draw
 * draws the sheet and OfficialDesk_Draw2 the rows. The must-order quests'
 * first two sheet pages (Quest_DrawMustOrderOverview, _Terms) are drawn
 * here; the common pages' functions hand such quests to them.
 */
#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "quest.h"
#include "lobby_menu.h"
#include "lobby_npc.h"
#include "lobby_labels.h"
/* The game's code for the official desk was built at optimization level 4. */
#pragma optimization_level 4
void Menu_DrawLabelBar(s16, s16, s16, u32, char *, u8, u8);

extern u8 gMenuWork[];

void LobbySe_Play(s32 se);
u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
s32 Win_Shake(WinShake *s);
s32 OfficialDesk_ChooseRow(OfficialDeskMenu *m, u16 pad);
s32 OfficialDesk_ChooseQuest(OfficialDeskMenu *m, u16 pad);
s32 OfficialDesk_CheckOrder(OfficialDeskMenu *m, u16 pad);
s32 OfficialDesk_Post(OfficialDeskMenu *m, u16 pad);
void OfficialDesk_Close(void);
void CamFocus_SetAside(s8 a);
void Talk_Say(s32 line, s8 question);
s32 Talk_Update(void);
u16 Rand_Next(s32 stream);
void QuestRoom_ResetSettings(void);
void QuestDesk_Accept(u16 id, s16 money, s32 pay);
extern u16 gQuestStampRand;
extern u16 gLobbyQuestId;
extern u8 gQuestIsOfficial;
extern s8 gQuestMinMembers;
void CamFocus_End(void);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
s32 Save_IsQuestCleared(u16 id);
extern u16 *gHrGateQuests[];
extern s8 gMenuInputLock;
extern s32 gTownMenuOpen;
extern s32 gTownMode;
void *memset(void *dst, s32 c, u32 n);
s32 ElderRaid_InState1(void);
s32 ElderRaid_InState2(void);
s32 Equip_GunHasNoAmmo(struct Player *pl, struct SaveData *save);
s32 Menu_PushNoticeOnce(s32);
void OfficialDesk_FindGateQuest(OfficialDeskMenu *m, s32 gate);
void OfficialDesk_ListRaid(OfficialDeskMenu *m);
s32 OfficialDesk_GateIndex(struct Em *npc);
void OfficialDesk_ListEvents(OfficialDeskMenu *m);
void OfficialDesk_Draw(MenuTask *task);
void OfficialDesk_Draw2(MenuTask *task);

/*
 * Opens the desk at its clerk: TOWN_MODE_OFFICIAL_DESK, the rows window
 * sliding in, the camera set aside. Fills the rows: the events once the
 * downloaded quests are ready, the raid's quests while a raid runs, the
 * next quest of the HR gate this clerk offers now; a row with nothing is
 * OFFICIAL_ROW_NONE. Warns when the equipped gun has no ammo.
 */
void OfficialDesk_Open(struct Em *npc)
{
    OfficialDeskMenu *desk = (OfficialDeskMenu *)gMenuWork;
    s32 gate;

    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_OFFICIAL_DESK;
    memset(desk, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(OfficialDesk_Draw, OfficialDesk_Draw2, 0, 0);
    WinSlide_Step(&desk->win, 0x280, 0x118);
    CamFocus_SetAside(1);
    desk->npc = npc;
    if (gLobby->dlQuestsReady != 0) {
        desk->row[OFFICIAL_ROW_EVENTS] = OFFICIAL_ROW_EVENTS;
        OfficialDesk_ListEvents(desk);
    } else {
        desk->row[OFFICIAL_ROW_EVENTS] = OFFICIAL_ROW_NONE;
    }
    if (ElderRaid_InState1() == 1 || ElderRaid_InState2() == 1) {
        desk->row[OFFICIAL_ROW_RAID] = OFFICIAL_ROW_RAID;
        OfficialDesk_ListRaid(desk);
    } else {
        desk->row[OFFICIAL_ROW_RAID] = OFFICIAL_ROW_NONE;
    }
    gate = OfficialDesk_GateIndex(npc);
    if (gate != -1) {
        desk->row[OFFICIAL_ROW_GATE] = OFFICIAL_ROW_GATE;
        OfficialDesk_FindGateQuest(desk, gate);
    } else {
        desk->row[OFFICIAL_ROW_GATE] = OFFICIAL_ROW_NONE;
    }
    if (Equip_GunHasNoAmmo(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave) == 1) {
        Menu_PushNoticeOnce(LOBBY_NOTICE_GUN_NO_AMMO);
    }
    LobbySe_Play(LOBBY_SE_WINDOW_IN);
}

extern QuestFile gQuestFiles[];

/* The QuestInfo of downloaded quest `slot`: its stage file in gQuestFiles
 * holds the info's offset in its first word. */
void *Quest_DownloadedInfo(u8 slot)
{
    QuestFile *file = &gQuestFiles[slot];
    return (u8 *)file->info + (s32)file;
}

/* Fills the events row's list with the downloaded quests' ids. */
void OfficialDesk_ListEvents(OfficialDeskMenu *desk)
{
    s16 i;
    QuestInfo *info;

    memset(desk->list[OFFICIAL_ROW_EVENTS], 0, sizeof(desk->list[OFFICIAL_ROW_EVENTS]));
    for (i = 0; i < gLobby->dlQuestCount; i++) {
        info = Quest_DownloadedInfo(i);
        if (info != NULL) {
            desk->list[OFFICIAL_ROW_EVENTS][i] = info->id;
        }
    }
}

/* Puts in the gate row's list the first quest of HR gate `gate`
 * (gHrGateQuests) not cleared yet. */
void OfficialDesk_FindGateQuest(OfficialDeskMenu *desk, s32 gate)
{
    u16 *quest = gHrGateQuests[gate];

    while (*quest != 0) {
        if (Save_IsQuestCleared(*quest) == 0) {
            desk->list[OFFICIAL_ROW_GATE][0] = *quest;
            return;
        }
        quest++;
    }
}

s32 Cond_IsLatest(u16 id);

/*
 * The HR gate this clerk offers now, or -1: NPC_OFFICIAL_DESK_LOW_HR
 * offers gates 0-2 and NPC_OFFICIAL_DESK_HIGH_HR gates 3-4, each while
 * its story condition is the latest one reached (Cond_IsLatest).
 */
s32 OfficialDesk_GateIndex(struct Em *npc)
{
    switch (((NpcWork *)&npc->work)->id) {
    case NPC_OFFICIAL_DESK_LOW_HR:
        if (Cond_IsLatest(0x5A) != 0) {
            return 0;
        }
        if (Cond_IsLatest(0x5D) != 0) {
            return 1;
        }
        if (Cond_IsLatest(0x62) != 0) {
            return 2;
        }
        break;
    case NPC_OFFICIAL_DESK_HIGH_HR:
        if (Cond_IsLatest(0x65) != 0) {
            return 3;
        }
        if (Cond_IsLatest(0x68) != 0 || Cond_IsLatest(0x69) != 0) {
            return 4;
        }
        break;
    }
    return -1;
}

extern ElderRaidEvent gElderRaidEvent;

/* Fills the raid row's list with the raid's quests for this clerk:
 * NPC_OFFICIAL_DESK_LOW_HR the first, NPC_OFFICIAL_DESK_HIGH_HR the second
 * and the third. */
void OfficialDesk_ListRaid(OfficialDeskMenu *desk)
{
    u8 clerk = ((NpcWork *)&desk->npc->work)->id;
    ElderRaidEvent *raid = &gElderRaidEvent;

    switch (clerk) {
    case NPC_OFFICIAL_DESK_LOW_HR:
        desk->list[OFFICIAL_ROW_RAID][0] = raid->quests[0];
        break;
    case NPC_OFFICIAL_DESK_HIGH_HR:
        desk->list[OFFICIAL_ROW_RAID][0] = raid->quests[1];
        desk->list[OFFICIAL_ROW_RAID][1] = raid->quests[2];
        break;
    }
}

/* Closes the desk: back to TOWN_MODE_FREE, the draws removed, the camera
 * back, the input held 20 frames, and the field's control given back. */
void OfficialDesk_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_SetAside(0);
    CamFocus_End();
    gMenuInputLock = 0x14; /* frames */
    LobbySe_Play(LOBBY_SE_CANCEL);
    gFieldControl = 1;
}

/*
 * The desk's frame (TOWN_MODE_OFFICIAL_DESK), one OFFICIAL_STEP_* at a
 * time: the rows window slides in, the row, the window slides out, the
 * quest, the checks, the room settings and the posting, the stamp, the
 * clerk's last words. A cancel slides the rows back in; leaving the rows
 * says goodbye. When the stamp settles, the quest is accepted with its fee
 * paid and marked official. The field takes no input while the posting
 * and the stamp run.
 */
void OfficialDesk_Update(void)
{
    OfficialDeskMenu *desk = (OfficialDeskMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (desk->state) {
    case OFFICIAL_STEP_OPEN:
        switch (WinSlide_Step(&desk->win, 0x280, 0x118)) {
        case MENU_DONE:
            desk->state++;
            break;
        }
        break;
    case OFFICIAL_STEP_ROWS:
        switch (OfficialDesk_ChooseRow(desk, pad)) {
        case MENU_DONE:
            desk->state++;
            WinSlide_Step(&desk->win, 0x118, 0x280);
            LobbySe_Play(LOBBY_SE_WINDOW_OUT);
            break;
        case MENU_CANCEL:
            desk->state = OFFICIAL_STEP_LEAVE;
            CamFocus_SetAside(0);
            Talk_Say(1, 0); /* the clerk's goodbye */
            break;
        }
        break;
    case OFFICIAL_STEP_ROWS_OUT:
        switch (WinSlide_Step(&desk->win, 0x118, 0x280)) {
        case MENU_DONE:
            desk->state++;
            break;
        }
        break;
    case OFFICIAL_STEP_CHOOSE:
        switch (OfficialDesk_ChooseQuest(desk, pad)) {
        case MENU_DONE:
            desk->state++;
            gQuestStampRand = Rand_Next(1);
            break;
        case MENU_CANCEL:
            desk->state = OFFICIAL_STEP_ROWS_BACK;
            WinSlide_Step(&desk->win, 0x280, 0x118);
            break;
        }
        break;
    case OFFICIAL_STEP_CHECK:
        switch (OfficialDesk_CheckOrder(desk, pad)) {
        case MENU_DONE:
            desk->state++;
            desk->menuCursor = ROOM_SETTING_MEMBERS;
            gLobbyQuestId = desk->quest->id;
            QuestRoom_ResetSettings();
            break;
        case MENU_CANCEL:
            desk->state = OFFICIAL_STEP_ROWS_BACK;
            WinSlide_Step(&desk->win, 0x280, 0x118);
            break;
        }
        break;
    case OFFICIAL_STEP_POST:
        switch (OfficialDesk_Post(desk, pad)) {
        case MENU_DONE:
            desk->state++;
            Win_Shake(&desk->shake);
            break;
        case MENU_UNDONE:
            OfficialDesk_Close();
            break;
        }
        gFieldControl = 0;
        break;
    case OFFICIAL_STEP_STAMP:
        gFieldControl = 0;
        switch (Win_Shake(&desk->shake)) {
        case MENU_DONE:
            desk->state++;
            if (gQuestMinMembers == 0) { /* the clerk's words, by the least members */
                Talk_Say(0xA, 0);
            } else {
                Talk_Say(0xB, 0);
            }
            QuestDesk_Accept(desk->quest->id, 0, 1); /* no wanted fee; the quest's fee paid */
            gQuestIsOfficial = 1;
            gFieldControl = 1;
            break;
        }
        break;
    case OFFICIAL_STEP_ACCEPTED:
        switch (Talk_Update()) {
        case MENU_DONE:
            OfficialDesk_Close();
            break;
        }
        break;
    case OFFICIAL_STEP_ROWS_BACK:
        switch (WinSlide_Step(&desk->win, 0x280, 0x118)) {
        case MENU_DONE:
            desk->state = OFFICIAL_STEP_ROWS;
            break;
        }
        break;
    case OFFICIAL_STEP_LEAVE:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            OfficialDesk_Close();
            break;
        }
        break;
    }
}

/* The plain-string format follows OfficialDesk_Update's jump table in
 * .rodata, so it is included after that function. */
INCLUDE_RODATA("asm/lobby/nonmatchings/official_desk", gOfficialDeskTextFmt);

extern s8 gRoomPassOn;
extern s8 gRoomPassword[9];
s32 LobbyMenu_KbdEntry(void *opts);
s32 Menu_PushNoticeOnce(s32);
void LobbySe_Play(s32 se);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void *memset(void *dst, s32 c, u32 n);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
s32 Menu_WrapUpDown(s32 cur, s32 n);
/*
 * The room settings' input: up and down move over the four lines (the
 * password's line is skipped while the password is off), left and right
 * change the least members or turn the password on and off (off clears
 * it), circle on the password's line opens the keyboard. Returns 1 when
 * circle is pressed on the last line (refused while the password is on but
 * empty), else 0.
 */
s32 OfficialDesk_RoomSettings(OfficialDeskMenu *desk, u16 pad)
{
    s16 oldCursor;

    switch (desk->roomStep) {
    case 0: /* the lines */
        if (pad & PAD_CIRCLE) {
            if (desk->menuCursor == ROOM_SETTING_DECIDE) {
                if (gRoomPassOn != 0 && gRoomPassword[0] == 0) {
                    Menu_PushNoticeOnce(LOBBY_NOTICE_PASSWORD_EMPTY);
                } else {
                    desk->roomStep = 0;
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                    return 1;
                }
            } else {
                if (desk->menuCursor == ROOM_SETTING_PASSWORD) {
                    desk->roomStep++;
                    gFieldControl = 0;
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                    LobbyMenu_Init();
                    LobbyMenu_SetDraw(OfficialDesk_Draw, OfficialDesk_Draw2, 0, 0);
                    LobbyMenu_KbdEntry(gRoomPassword);
                }
                if (desk->menuCursor != ROOM_SETTING_MEMBERS && desk->menuCursor != ROOM_SETTING_PASS_ON) {
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                }
            }
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (desk->menuCursor == ROOM_SETTING_PASS_ON) {
                gRoomPassOn ^= 1;
                if (gRoomPassOn == 0) {
                    memset(gRoomPassword, 0, sizeof(gRoomPassword));
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            } else if (desk->menuCursor == ROOM_SETTING_MEMBERS) {
                gQuestMinMembers = Menu_WrapLeftRight(gQuestMinMembers, ROOM_MIN_MEMBERS_CHOICES);
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            oldCursor = desk->menuCursor;
            desk->menuCursor = Menu_WrapUpDown(oldCursor, ROOM_SETTINGS);
            if ((s8)oldCursor != desk->menuCursor && desk->menuCursor == ROOM_SETTING_PASSWORD && gRoomPassOn == 0) {
                desk->menuCursor += desk->menuCursor - (s8)oldCursor;
            }
        }
        break;
    case 1: /* the password's keyboard, until it is closed */
        if (LobbyMenu_KbdEntry(gRoomPassword) == 1) {
            desk->roomStep = 0;
            LobbyMenu_Init();
            gFieldControl = 1;
        }
        break;
    }
    return 0;
}

extern LobbyMenu *gLobbyMenu;
void LobbyText_Open(s32);
s32 Quest_PostStep(MsgQuestPost *req);
/*
 * The room settings, then the posting: once decided, fills the posting
 * (the quest, no wanted fee, this desk, the area), opens the sending text
 * and runs Quest_PostStep. Returns MENU_DONE once posted, MENU_UNDONE when
 * the posting failed (the quest is dropped), else MENU_RUNNING.
 */
s32 OfficialDesk_Post(OfficialDeskMenu *menu, u16 pad)
{
    switch (menu->subStep) {
    case 0:
        if (OfficialDesk_RoomSettings(menu, pad) == 1) {
            menu->subStep++;
            LobbyText_Open(LOBBY_TEXT_SENDING);
            menu->req.quest = menu->quest->id;
            menu->req.money = 0;
            menu->req.desk = QUEST_POST_DESK_OFFICIAL;
            menu->req.area = gScene.area;
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (Quest_PostStep(&menu->req)) {
        case MENU_DONE:
            menu->subStep = 0;
            return MENU_DONE;
        case MENU_UNDONE:
            menu->subStep = 0;
            menu->quest = NULL;
            return MENU_UNDONE;
        }
        break;
    }
    return MENU_RUNNING;
}

s32 Quest_CheckOrderTerms(u8 id, u8 flag);
void LobbyDialog_Grow(void);
/*
 * Checks that the chosen quest can be taken: no quest taken already, the
 * quest's order terms met (else a dialog, closed with circle), its fee at
 * hand. Returns MENU_DONE when all pass, MENU_CANCEL after a refusal
 * (the quest dropped), else MENU_RUNNING.
 */
s32 OfficialDesk_CheckOrder(OfficialDeskMenu *desk, u16 pad)
{
    switch (desk->subStep) {
    case 0:
        if (gLobby->questTaken != 0) {
            desk->subStep++;
            Talk_Say(0xF, 0); /* a quest is already taken */
            Talk_Update();
        } else if (Quest_CheckOrderTerms(desk->quest->terms, 0) == 0) {
            desk->subStep = 2;
            LobbyText_Open(LOBBY_TEXT_ORDER_TERMS);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
        } else if (desk->quest->fee > gSave.money) {
            desk->subStep++;
            Talk_Say(8, 0); /* not enough money */
            Talk_Update();
        } else {
            return MENU_DONE;
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case MENU_DONE:
            desk->subStep = 0;
            desk->quest = NULL;
            return MENU_CANCEL;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            desk->subStep = 0;
            desk->quest = NULL;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_CANCEL;
        }
        break;
    }
    return MENU_RUNNING;
}

/* The quest list of the row under the cursor (a row of
 * OFFICIAL_ROW_NONE gives the raid's list). */
static u16 *OfficialDesk_RowList(OfficialDeskMenu *desk)
{
    switch (desk->row[desk->cursor]) {
    case OFFICIAL_ROW_EVENTS:
        return desk->list[OFFICIAL_ROW_EVENTS];
    case OFFICIAL_ROW_GATE:
        return desk->list[OFFICIAL_ROW_GATE];
    case OFFICIAL_ROW_RAID:
    default:
        return desk->list[OFFICIAL_ROW_RAID];
    }
}

extern u16 gTalkLine;
extern char *gTalkLineText;
QuestInfo *Quest_GetDiscInfo(u16 id);
char *Talk_LineText(u16 id);
char *Text_FillDollar(char *arg0, s32 arg1);
s32 Menu_WrapUpDownSe(s32 cur, s32 n, s32 se);
u16 *OfficialDesk_RowList(OfficialDeskMenu *m);
/*
 * The quest on the sheet: up and down move in the row's list, left and
 * right turn the sheet's page. Circle takes the quest and the clerk asks to
 * confirm it with its fee filled in, or says a quest is already taken; a
 * no is answered and the list comes back. Returns MENU_DONE on a yes,
 * MENU_CANCEL on cross, else MENU_RUNNING.
 */
s32 OfficialDesk_ChooseQuest(OfficialDeskMenu *desk, u16 pad)
{
    u16 *list = OfficialDesk_RowList(desk);

    switch (desk->subStep) {
    case 0:
        if (pad & PAD_CIRCLE) {
            desk->quest = Quest_GetDiscInfo(list[desk->listCursor]);
            if (gLobby->questTaken != 0) {
                desk->subStep = 3;
                desk->quest = NULL;
                Talk_Say(0xF, 0); /* a quest is already taken */
            } else if (desk->quest->fee != 0) {
                Talk_Say(6, 0); /* the confirmation, with the fee */
                gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), desk->quest->fee);
                desk->subStep++;
            } else {
                Talk_Say(5, 0); /* the confirmation, without a fee */
                gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), desk->quest->fee);
                desk->subStep++;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            desk->quest = NULL;
            LobbySe_Play(LOBBY_SE_WINDOW_OUT);
            return MENU_CANCEL;
        } else {
            desk->listCursor = Menu_WrapUpDownSe(desk->listCursor, desk->listCount, LOBBY_SE_WINDOW_OUT);
            desk->sheetPage = Menu_WrapLeftRight(desk->sheetPage, QUEST_SHEET_PAGES);
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case MENU_DONE:
            desk->subStep = 0;
            return MENU_DONE;
        case MENU_CANCEL:
            Talk_Say(0x10, 0); /* declined */
            desk->subStep++;
            break;
        }
        break;
    case 2:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            desk->subStep = 0;
            desk->quest = NULL;
            break;
        }
        break;
    case 3:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            desk->subStep = 0;
            LobbySe_Play(LOBBY_SE_CANCEL);
            break;
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * The row: up and down move over the three rows; circle on a row with
 * quests makes the clerk speak, counts the row's quests and resets the
 * list's cursor and the sheet's page (a row of OFFICIAL_ROW_NONE is
 * refused). Returns MENU_DONE after the clerk's words, MENU_CANCEL on
 * cross, else MENU_RUNNING.
 */
s32 OfficialDesk_ChooseRow(OfficialDeskMenu *desk, u16 pad)
{
    u16 *list;
    s32 i;

    switch (desk->subStep) {
    case 0:
        if (pad & PAD_CIRCLE) {
            list = OfficialDesk_RowList(desk);
            switch (desk->row[desk->cursor]) {
            case OFFICIAL_ROW_GATE:
                Talk_Say(3, 0); /* the clerk on the HR gate */
                break;
            default:
                Talk_Say(4, 0); /* the clerk on the other rows */
                break;
            case OFFICIAL_ROW_NONE:
                LobbySe_Play(LOBBY_SE_REFUSE);
                return MENU_RUNNING;
            }
            desk->subStep++;
            desk->sheetPage = 0;
            desk->listCursor = 0;
            desk->listCount = 0;
            for (i = 0; i < OFFICIAL_ROW_QUESTS; i++) {
                if (list[i] != 0) {
                    desk->listCount++;
                }
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else {
            desk->cursor = Menu_WrapUpDown(desk->cursor, OFFICIAL_ROWS);
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            desk->subStep = 0;
            return MENU_DONE;
        }
        break;
    }
    return MENU_RUNNING;
}

extern char *gOfficialEventTexts[];
void Menu_DrawDecoFrame(s16 x, s16 y0, s16 w0, s16 h, s8 kind, s8 b);
void Font_SetColor(s32 color);
void Font_SetSpacing(u8 a, u8 b);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
/* Draws the rows window at (x, y): its frame and the three rows' names
 * (the third by the raid row's kind: blank without a raid), the row under
 * the cursor with arrows. */
void OfficialDesk_DrawRows(s16 x, s16 y)
{
    OfficialDeskMenu *desk = (OfficialDeskMenu *)gMenuWork;
    char *label;
    s32 i;
    s16 labelX;

    Menu_DrawDecoFrame(x, y, 0x140, 0x168, 4, -1);
    Font_SetColor(0x807F7F7F);
    x = x + 0xA0;
    y = y + 0x64;
    Font_SetSpacing(0x18, 0x18);
    for (i = 0; i < OFFICIAL_ROWS; i++) {
        if (i < 2) {
            label = gOfficialEventTexts[i];
        } else {
            label = gOfficialEventTexts[desk->row[i]];
        }
        labelX = Text_CenterX(label, x, 0x18);
        if (i == desk->cursor) {
            Menu_DrawArrowsRev(labelX, y, 0x18, label, 0xFFB57510);
            Text_DrawShadowRgb(labelX, y, 0xFF101010, 0xFF106060, label);
        } else {
            Font_SetColor(0xFF101010);
            Font_SetPos(labelX, y);
            Text_PrintfColored(label);
        }
        y += 0x46;
    }
}


char *Quest_GetText(u16 id, s32 k);
char *strcat(char *d, const char *s);
extern char gOfficialDeskTextFmt[];
extern char *gQuestKindNames[];

/* Page QUEST_SHEET_OVERVIEW of a must-order quest's sheet
 * (QUEST_FLAG_MUST_ORDER): the name, texts 1-3, the stars, the order
 * condition's name and, at the left, the labels. */
void Quest_DrawMustOrderOverview(s16 x, s16 y, QuestInfo *quest)
{
    s32 i;
    char stars[0x40];
    s16 top = y;
    s32 id = quest->id;

    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(gOfficialDeskTextFmt, Quest_GetText(id, 0));
    Font_SetPos(x, y += 0x3C);
    Text_PrintfColored(gOfficialDeskTextFmt, Quest_GetText(id, 1));
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(gOfficialDeskTextFmt, Quest_GetText(id, 2));
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(gOfficialDeskTextFmt, Quest_GetText(id, 3));
    Font_SetPos(x + 0x5A, y += 0x1C);
    stars[0] = 0;
    for (i = 0; i < quest->stars; i++) {
        strcat(stars, gLobbyMarks[LOBBY_MARK_STAR]);
    }
    Text_PrintfColored(gOfficialDeskTextFmt, stars);
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestKindNames[quest->terms]);
    Font_SetColor(0x80301010);
    x = x - 0xA;
    y = top + 0x14;
    y += 0x2A;
    Font_SetPos(x, y);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_MUST_ORDER]);
    Font_SetPos(x, y += 0x60);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_DIFFICULTY]); /* the stars' label */
    Font_SetPos(x, y + 0x2A);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_SPECIAL_COND]); /* the order condition's label */
}

s32 sprintf(char *buf, const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);

/* Page QUEST_SHEET_TERMS of a must-order quest's sheet: the name, the time
 * limit in minutes, the fee and the reward (printed, then widened to
 * full-width characters), texts 4 and 5 and, at the left, the labels. */
void Quest_DrawMustOrderTerms(s16 x, s16 y, QuestInfo *quest)
{
    s16 top = y;
    s32 id = quest->id;
    char wide[0x40];
    char narrow[0x40];

    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(gOfficialDeskTextFmt, Quest_GetText(id, 0));
    Font_SetPos(x + 0x7E, y += 0x2A);
    sprintf(narrow, "%d%s", quest->timeLimit / 60 / 30, gLobbyWords[LOBBY_WORD_MINUTES]); /* frames to minutes; the minutes' unit */
    Sjis_Widen(narrow, wide);
    Text_PrintfColored(gOfficialDeskTextFmt, wide);
    Font_SetPos(x + 0x7E, y += 0x18);
    sprintf(narrow, "%dz", quest->fee);
    Sjis_Widen(narrow, wide);
    Text_PrintfColored(gOfficialDeskTextFmt, wide);
    Font_SetPos(x + 0x7E, y += 0x18);
    sprintf(narrow, "%dz", quest->reward);
    Sjis_Widen(narrow, wide);
    Text_PrintfColored(gOfficialDeskTextFmt, wide);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(gOfficialDeskTextFmt, Quest_GetText(id, 4));
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(gOfficialDeskTextFmt, Quest_GetText(id, 5));
    Font_SetColor(0x80301010);
    x = x - 0xA;
    y = top + 0x14;
    y += 0x2A;
    Font_SetPos(x, y);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_TIME_LIMIT]); /* the time limit's label */
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_CONTRACT_FEE]);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_REWARD]);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_CLEAR_COND]);
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_FAIL_COND]);
}

void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 a, s32 b);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl);
void Quest_DrawOverview(s16 x, s16 y, QuestInfo *q);
void Quest_DrawTerms(s16 x, s16 y, QuestInfo *q);
void Quest_DrawDetails(s16 x, s16 y, QuestInfo *q);
void Quest_DrawClearMark(s16 x, s16 y);
s32 Save_IsQuestCleared(u16 id);
u16 Map_Group(u16 id);
extern s8 gMapGroupMapIds[][2];
extern u16 gQuestStampRand;

/*
 * Draws the quest sheet at (x, y): the title bar, the money, the map strip
 * of the row's quests (the one under the cursor negated), the page number
 * while choosing, the page (the must-order pages for a must-order quest),
 * the cleared mark, and the stamp once the quest is taken. The quest drawn
 * is the one taken, else the one under the cursor.
 */
void OfficialDesk_DrawSheet(OfficialDeskMenu *desk, s16 x, s16 y)
{
    u16 *list = OfficialDesk_RowList(desk);
    QuestInfo *sheetQuest = Quest_GetDiscInfo(list[desk->listCursor]);
    s16 mapGroups[OFFICIAL_ROW_QUESTS];
    s32 i;

    if (desk->quest != NULL) {
        sheetQuest = desk->quest;
    }
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFFB0B000, gQuestSheetTexts[QUEST_SHEET_TEXT_OFFICIAL_QUEST], 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    memset(mapGroups, 0, sizeof(mapGroups));
    for (i = 0; i < OFFICIAL_ROW_QUESTS; i++) {
        if (list[i] == 0) {
            break;
        }
        mapGroups[i] = Map_Group(Quest_GetDiscInfo(list[i])->map);
        if (i == desk->listCursor) {
            mapGroups[i] = -mapGroups[i];
        }
    }
    Menu_DrawQuestSheet(x, y, 0x140, 0x168, mapGroups, desk->listCursor, gMapGroupMapIds[Map_Group(sheetQuest->map)][0], -1);
    Font_SetColor(0x80101010);
    if (desk->state == OFFICIAL_STEP_CHOOSE && desk->subStep == 0) {
        Menu_DrawPageNum(0x21C, y + 0xE, desk->sheetPage, QUEST_SHEET_PAGES, 1);
    }
    Font_SetSpacing(0x12, 0x12);
    if (sheetQuest->flags & QUEST_FLAG_MUST_ORDER) {
        switch (desk->sheetPage) {
        case QUEST_SHEET_OVERVIEW:
            Quest_DrawMustOrderOverview((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, sheetQuest);
            break;
        case QUEST_SHEET_TERMS:
            Quest_DrawMustOrderTerms((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, sheetQuest);
            break;
        case QUEST_SHEET_DETAILS:
            Quest_DrawDetails((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, sheetQuest);
            break;
        }
    } else {
        switch (desk->sheetPage) {
        case QUEST_SHEET_OVERVIEW:
            Quest_DrawOverview((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, sheetQuest);
            break;
        case QUEST_SHEET_TERMS:
            Quest_DrawTerms((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, sheetQuest);
            break;
        case QUEST_SHEET_DETAILS:
            Quest_DrawDetails((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, sheetQuest);
            break;
        }
    }
    if (Save_IsQuestCleared(sheetQuest->id)) {
        Quest_DrawClearMark(x + 0x28, y + 0xE);
    }
    if (desk->quest != NULL) {
        switch (desk->state) {
        case OFFICIAL_STEP_STAMP:
        case OFFICIAL_STEP_ACCEPTED:
        case OFFICIAL_STEP_LEAVE:
            Menu_DrawStamp(x + 0x28, y + 0xE, 0x140, 0x168, gQuestStampRand, desk->shake.timer);
            break;
        }
    }
}

void Menu_DrawArrows(s16 x, s16 y, s16 size, char *text, s32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);

/* Draws the room settings' sheet at (x, y): the least members, the
 * password on or off, the password in its box, and the decide line; the
 * line under the cursor has arrows. */
void OfficialDesk_DrawRoomSettings(OfficialDeskMenu *desk, s16 x, s16 y)
{
    s16 top = y;
    s32 valueX;

    Menu_DrawQuestSheet(x, y, 0x140, 0x168, NULL, 1, -1, -1);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    y += 0x28;
    valueX = x + 0xD8;
    Font_SetPos(valueX, y += 0x2A);
    Text_PrintfColored(gOfficialDeskTextFmt, gMemberCountTexts[gQuestMinMembers]);
    if (desk->menuCursor == ROOM_SETTING_MEMBERS) {
        Menu_DrawArrows(valueX, y, 0x12, gMemberCountTexts[gQuestMinMembers], 0xFF106060);
    }
    Font_SetPos(valueX, y += 0x2A);
    Text_PrintfColored(gNoneSetTexts[gRoomPassOn]);
    if (desk->menuCursor == ROOM_SETTING_PASS_ON) {
        Menu_DrawArrows(valueX, y, 0x12, gNoneSetTexts[gRoomPassOn], 0xFF106060);
    }
    Font_SetPos(Text_CenterX((char *)gRoomPassword, 0x1B8, 0x12), y += 0x2A);
    Text_PrintfColored(gOfficialDeskTextFmt, gRoomPassword);
    if (desk->menuCursor == ROOM_SETTING_PASSWORD) {
        Menu_DrawArrowsRev(x + 0x64, y, 0x12, "             " /* the password box's width */, 0xFF106060);
        Gfx_DrawRectFrame(x + 0x64, y - 1, 0x78, 0x14, 0xFF106060);
        Gfx_DrawRectFrame(x + 0x63, y - 2, 0x7A, 0x16, 0xFF106060);
    } else {
        Gfx_DrawRectFrame(x + 0x64, y - 1, 0x78, 0x14, 0xFF606060);
        Gfx_DrawRectFrame(x + 0x63, y - 2, 0x7A, 0x16, 0xFF606060);
    }
    Font_SetColor(0x80301010);
    x = x + 0x28;
    y = top + 0x28;
    Font_SetPos(Text_CenterX(gQuestSheetTexts[QUEST_SHEET_TEXT_RECRUIT], 0x1B8, 0x12), y);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_RECRUIT]); /* the title */
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_MEMBERS]);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_PASSWORD]); /* the password line's label */
    x = Text_CenterX(gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE], 0x1B8, 0x12);
    Font_SetPos(x, y += 0x78);
    Text_PrintfColored(gOfficialDeskTextFmt, gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE]);
    if (desk->menuCursor == ROOM_SETTING_DECIDE) {
        Menu_DrawArrowsRev(x, y, 0x12, gQuestSheetTexts[QUEST_SHEET_TEXT_DECIDE], 0xFF106060);
    }
}

void Text_SetSize(s32 a);
void Menu_DrawDecoBox(s16 x, s16 y, s16 w, s16 h, s8 kind);
void OfficialDesk_DrawSheet(OfficialDeskMenu *m, s16 x, s16 y);
void OfficialDesk_DrawRoomSettings(OfficialDeskMenu *m, s16 x, s16 y);
/* The desk's first draw: the rows window's empty box while it slides in
 * and during the row choice, the room settings before the posting starts,
 * nothing at the goodbye, else the quest sheet (shaken while the stamp
 * lands). */
void OfficialDesk_Draw(MenuTask *task)
{
    OfficialDeskMenu *desk = (OfficialDeskMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    switch (desk->state) {
    case OFFICIAL_STEP_OPEN:
    case OFFICIAL_STEP_ROWS:
        Menu_DrawDecoBox(desk->win.pos, 0x3A, 0x140, 0x168, 4);
        break;
    case OFFICIAL_STEP_ROWS_OUT:
    case OFFICIAL_STEP_CHOOSE:
    case OFFICIAL_STEP_CHECK:
    case OFFICIAL_STEP_STAMP:
    case OFFICIAL_STEP_ACCEPTED:
    case OFFICIAL_STEP_ROWS_BACK:
    default:
        OfficialDesk_DrawSheet(desk, desk->shake.dx + 0x118, desk->shake.dy + 0x3A);
        break;
    case OFFICIAL_STEP_POST:
        if (desk->subStep == 0) {
            OfficialDesk_DrawRoomSettings(desk, 0x118, 0x3A);
        }
        break;
    case OFFICIAL_STEP_LEAVE:
        break;
    }
}

void OfficialDesk_DrawRows(s16 x, s16 y);
/* The desk's second draw: the rows window at its sliding x while it
 * shows. */
void OfficialDesk_Draw2(MenuTask *task)
{
    OfficialDeskMenu *desk = (OfficialDeskMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    switch (desk->state) {
    case OFFICIAL_STEP_OPEN:
    case OFFICIAL_STEP_ROWS:
    case OFFICIAL_STEP_ROWS_OUT:
    case OFFICIAL_STEP_ROWS_BACK:
        OfficialDesk_DrawRows(desk->win.pos, 0x3A);
        break;
    }
}
