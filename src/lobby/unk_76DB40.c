#include "common.h"
#include "lobby_text.h"
#include "scene.h"
#include "save.h"
#include "item.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "pad.h"
#include "quest.h"
#include "player.h"

void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern u8 gTownActionDelay;
extern u8 gMenuWork;
void Text_SetSize(s32 size);
void QuestCounter_DrawTeamChoice(s32 x, s32 y0);
QuestInfo *Quest_GetInfoAny(u16);
void Quest_DrawArenaTeams(s16 x, s16 y, NetUser *list, s8 room);
void Quest_DrawPartyList(s16 x, s32 y0, s32 k);
void Font_SetColor(u32); /* unit's own declaration: u32 (the definition s32): QuestCounter_DrawRecruitInfo changes by 14 words with s32 */
void *memset(void *p, s32 c, u32 n);
void Mem_Fill(void *dst, u8 c, s32 n);
u8 *Talk_FindScript(u16 id);
void LobbyMenu_Init(void);
void QuestCounter_Draw(void *);
void QuestCounter_Draw2(void *arg0);
void LobbySe_Play(s32 se);
extern u8 gTalkNpcName;
extern u8 *gTalkScriptRec;
extern s16 gTalkScript;
extern s32 gTalkLine;
extern s8 gLobbyLevel2Index;
s32 func_355A70(void);
s32 Game_IsOnline(void);
void QuestCounter_DrawRecruit(WinSlide *, s16);
void QuestCounter_DrawQuestWindow(s16 x, s16 y);

u8 BoxMenu_ListStyles(u8 *out)
{
    SaveData *save = &gSave;
    u16 mask = 0;
    u32 i;
    u8 n;

    if (func_355A70() == 1) {
        mask = save->rateFlags;
    }
    if ((Game_IsOnline() != 1 || func_355A70() != 1) && save->styleBits != 0) {
        mask |= (u16)(1 << save->styleFlag);
    }
    for (i = 0, n = 0; i < 12; i++) {
        if (mask & (1 << i)) {
            *out++ = i;
            n++;
        }
    }
    return n;
}

void QuestCounter_Open(void)
{
    memset(&gMenuWork, 0, 0xF00);
    gLobbyLevel2Index = 0;
    gTownMenuOpen = 1;
    gTownMode = 1;
    gTalkLine = 0;
    gFieldControl = 0;
    Mem_Fill(&gTalkNpcName, 0, 0x34);
    gTalkScriptRec = Talk_FindScript(0x63);
    gTalkScript = 0x63;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(QuestCounter_Draw, QuestCounter_Draw2, NULL, NULL);
    if (gLobby->questTaken == 0) {
        memset(gLobby->unk1542, 0, 0x20);
    }
    LobbySe_Play(8);
}

void QuestCounter_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = 0;
    gFieldControl = 1;
    gTownActionDelay = 0x14;
    LobbyMenu_SetDraw(NULL, NULL, NULL, NULL);
}

void Talk_Say(s32 arg0, s8 arg1);
void LobbyText_Open(s32);
s32 Menu_PushNoticeOnce(s32 arg0);
s32 QuestCounter_OpenWindows(s32 x, s32 y);
s32 WinSlide_Step(WinSlide *, s32, s32); /* unit's own declaration: with the definition's s32 (WinSlide *s, s16 from, s16 to) (lobby/unk_7A7560.c), QuestCounter_OpenWindows 139 of 158 words differ */
s32 QuestCounter_CanJoin(s16 idx);
s32 Talk_Update(void);
void QuestCounter_Close(void);
u16 LobbyPad_Get(s32);
void Chat_SetWindowMode(s8 a, s8 b);
s32 LobbyLevel2_FetchList(void);
struct Player;
s32 Equip_GunHasNoAmmo(struct Player *pl, struct SaveData *save);
s32 QuestCounter_ChooseQuest(u16 arg0);
s32 LobbyStep_Level2Entry(u16 id, NetUser *users);
s32 QuestCounter_ViewRoom(u16 arg0);
char *Talk_LineText(u16 id);
char *Text_FillDollar(char *fmt, s32 n);
s32 QuestCounter_CheckQuest(void);
s32 Talk_YesNoBox(s8 flags);
s32 QuestCounter_KbdInput(void *task);
s32 Quest_IsDownloadId(s32);
s8 Quest_DownloadedSlot(u16 id);
s32 LobbyStep_Download(s8 slot, u16 id); /* unit's own declaration: with the definition's s32 (s32 slot, s32 id) (lobby/unk_7B3140.c), QuestCounter_Step 238 of 655 words differ */
QuestInfo *Quest_CurrentInfo(void);
s32 QuestCounter_ChooseKind(u16 arg0);
s32 LobbyLevel2_EnterWithPass(s32 v);
void QuestDesk_Accept(u16 id, s16 money, s32 pay);
s32 LobbyStep_6501(void);
extern LobbyMenu *gLobbyMenu;
extern LobbyRecs gLobbyRecs;
extern u16 gLobbyQuestId;
extern s8 gQuestMinMembers;
extern char *gTalkLineText;


/* The quest counter's steps (+0x4AE of the menu work area gMenuWork): fetch
 * the quest list, pick a quest (QuestCounter_ChooseQuest), enter it on the server,
 * check the members (QuestCounter_ViewRoom/QuestCounter_CheckQuest), download it, then the
 * password (QuestCounter_ChooseKind) and LobbyLevel2_EnterWithPass; 0x11 waits for
 * the circle button and goes back to +0x4B1. */
void QuestCounter_Step(void)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    PostedQuest *q = &gPostedQuests[w->quest];
    LevelEntry *r = &gLobbyLevel2Entries[w->quest];
    u16 pad = LobbyPad_Get(0);
    u16 slot;
    s8 k;
    u8 *info;

    Chat_SetWindowMode(1, 2);
    switch (w->step) {
    case 0:
        (w->step)++;
        LobbyText_Open(0x78);
        gFieldControl = 0;
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_FetchList()) {
        case 0:
            (w->step)++;
            Talk_Say(0, 0);
            if (Equip_GunHasNoAmmo(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave) == 1) {
                Menu_PushNoticeOnce(0x13);
            }
            gFieldControl = 1;
            QuestCounter_OpenWindows(0x280, 0xF0);
            LobbySe_Play(0x2C);
            break;
        case 1:
            w->step = 0x11;
            w->returnStep = 0x10;
            gFieldControl = 0;
            break;
        }
        break;
    case 2:
        switch (QuestCounter_OpenWindows(0x280, 0xF0)) {
        case 0:
            (w->step)++;
            break;
        }
        break;
    case 3:
        switch (QuestCounter_ChooseQuest(pad)) {
        case 0:
            (w->step)++;
            w->page = 0;
            gFieldControl = 0;
            LobbyText_Open(0x78);
            break;
        case 3:
            w->step = 0x10;
            break;
        case 1:
            w->step = 0x11;
            w->returnStep = 0x10;
            gFieldControl = 0;
            LobbyText_Open(0x62);
            break;
        }
        break;
    case 4:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_Level2Entry(w->quest, w->members)) {
        case 0:
            (w->step)++;
            w->questId = q->quest;
            gFieldControl = 1;
            WinSlide_Step(&w->questWin, 0x280, 0x118);
            LobbySe_Play(0x17);
            break;
        }
        break;
    case 5:
        switch (WinSlide_Step(&w->questWin, 0x280, 0x118)) {
        case 0:
            (w->step)++;
            break;
        }
        break;
    case 6:
        switch (QuestCounter_ViewRoom(pad)) {
        case 0:
            (w->step)++;
            if (q->money > 0) {
                Talk_Say(9, 0);
                gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), q->money);
            } else {
                Talk_Say(1, 0);
            }
            gFieldControl = 0;
            break;
        case 3:
            w->step = 3;
            break;
        }
        break;
    case 7:
        if (QuestCounter_CanJoin(w->quest) == 0) {
            switch (gLobbyLevel2Entries[w->quest].state) {
            case 4:
                Menu_PushNoticeOnce(0xA);
                break;
            default:
                Menu_PushNoticeOnce(9);
                break;
            }
            w->step = 3;
            Talk_Say(0, 0);
            gFieldControl = 1;
            LobbySe_Play(4);
            break;
        }
        switch (Talk_Update()) {
        case 0:
            if (gLobby->questTaken != 0) {
                Talk_Say(7, 0);
                w->step = 0xF;
                gFieldControl = 1;
                break;
            }
            (w->step)++;
            gFieldControl = 0;
            break;
        case 3:
            w->step = 3;
            gFieldControl = 1;
            break;
        }
        break;
    case 8:
        if (QuestCounter_CheckQuest() == 0) {
            if (gLobbyLevel2Entries[w->quest].unk17 != 0) {
                (w->step)++;
                Talk_Say(2, 0);
                memset(gLobbyRecs.pass, 0, 0x10);
                gFieldControl = 0;
                break;
            }
            w->step = 0xA;
            Talk_Say(3, 0);
            LobbyText_Open(0x78);
            gLobbyQuestId = w->questId;
            k = r->maxUsers - 1;
            gFieldControl = 0;
            gQuestMinMembers = k;
            break;
        }
        gLobbyMenu->redraw = 1;
        w->step = 0x11;
        w->returnStep = 3;
        gFieldControl = 0;
        break;
    case 9:
        Talk_YesNoBox(1);
        switch (QuestCounter_KbdInput(gLobbyRecs.pass)) {
        case 0:
            (w->step)++;
            Talk_Say(3, 0);
            LobbyText_Open(0x78);
            gLobbyQuestId = w->questId;
            k = r->maxUsers - 1;
            gFieldControl = 0;
            gQuestMinMembers = k;
            break;
        case 3:
            w->step = 3;
            Talk_Say(0, 0);
            gFieldControl = 1;
            LobbySe_Play(4);
            break;
        }
        break;
    case 10:
        if (Quest_IsDownloadId((u16)w->questId) == 0) {
            if (q->desk != 3) {
                w->passKind = 0;
                w->step = 0xC;
                break;
            }
            if (r->maxUsers == 2) {
                if (*(s16 *)((u8 *)&gSave + 0x2932) == 0) {
                    LobbyText_Open(0x67);
                    w->step = 0x11;
                    w->returnStep = 0x10;
                    gFieldControl = 0;
                    break;
                }
                w->kind = 3;
                w->passKind = 3;
                w->step = 0xC;
                break;
            }
            (w->step)++;
            break;
        }
        gLobbyMenu->redraw = 1;
        slot = Quest_DownloadedSlot((u16)w->questId);
        switch (LobbyStep_Download(slot, (u16)w->questId)) {
        case 0:
            info = (u8 *)Quest_CurrentInfo();
            if (*(u16 *)(info + 0x2A) != w->questId) {
                LobbyText_Open(0x62);
                w->step = 0x11;
                w->returnStep = 3;
                gFieldControl = 0;
                break;
            }
            w->passKind = 0;
            w->step = 0xC;
            gFieldControl = 0;
            break;
        case 1:
            w->step = 0x11;
            w->returnStep = 3;
            gFieldControl = 0;
            break;
        }
        break;
    case 11:
        if (QuestCounter_CanJoin(w->quest) == 0) {
            switch (gLobbyLevel2Entries[w->quest].state) {
            case 4:
                Menu_PushNoticeOnce(0xA);
                break;
            default:
                Menu_PushNoticeOnce(9);
                break;
            }
            w->step = 3;
            w->subStep = 0;
            Talk_Say(0, 0);
            gFieldControl = 1;
            LobbySe_Play(4);
            break;
        }
        switch (QuestCounter_ChooseKind(pad)) {
        case 3:
            w->step = 6;
            break;
        case 0:
            (w->step)++;
            gFieldControl = 0;
            break;
        case 1:
            w->step = 0x11;
            w->returnStep = 0xB;
            gFieldControl = 0;
            break;
        }
        break;
    case 12:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_EnterWithPass(w->passKind)) {
        case 0:
            if (w->questId != q->quest) {
                w->step = 0x14;
                gFieldControl = 0;
                break;
            }
            (w->step)++;
            gFieldControl = 0;
            LobbyText_Open(0x61);
            gPlayerText.card.arenaRole = w->kind;
            QuestDesk_Accept((u16)w->questId, (s16)q->money, 0);
            break;
        case 1:
            w->step = 0x11;
            w->returnStep = 3;
            gFieldControl = 0;
            break;
        }
        break;
    case 20:
        gLobbyMenu->redraw = 1;
        if (LobbyStep_6501() == 1) {
            w->step = 0x11;
            w->returnStep = 0x10;
            LobbyText_Open(0x96);
        }
        break;
    case 13:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            (w->step)++;
            gFieldControl = 1;
            LobbySe_Play(0);
        }
        break;
    case 14:
        QuestCounter_Close();
        break;
    case 15:
        if (Talk_Update() != 0) {
            break;
        }
    case 16:
        QuestCounter_Close();
        break;
    case 17:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            w->step = w->returnStep;
            gFieldControl = 1;
            LobbySe_Play(8);
        }
        break;
    }
}

s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 WinSlide_Step(WinSlide *, s32, s32); /* unit's own declaration: with the definition's s32 (WinSlide *s, s16 from, s16 to) (lobby/unk_7A7560.c), QuestCounter_OpenWindows 139 of 158 words differ */
void LobbyText_Open(s32);

s32 QuestCounter_ChooseKind(u16 pad)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    s32 btn;
    s32 n;

    switch (w->subStep) {
    case 0:
        w->kind = 0;
        (w->subStep)++;
        WinSlide_Step(&w->kindWin, -0xE6, 0x72);
        break;
    case 1:
        switch (WinSlide_Step(&w->kindWin, -0xE6, 0x72)) {
        case 0:
            (w->subStep)++;
            break;
        }
        break;
    case 2:
        btn = pad & 0xFFFF;
        if (btn & 0x20) {
            w->subStep = 0;
            n = w->cursor + 2;
            if (n == 3 && *(s16 *)((u8 *)&gSave + 0x2932) == 0) {
                LobbyText_Open(0x67);
                return 1;
            }
            w->kind = n;
            w->passKind = w->cursor + 2;
            LobbySe_Play(8);
            return 0;
        }
        if (btn & 0x40) {
            w->subStep = 0;
            LobbySe_Play(4);
            return 3;
        }
        w->cursor = Menu_WrapUpDown(w->cursor, 3);
        break;
    }
    return 2;
}

QuestInfo *Quest_FindDownloaded(u16 id);
s32 QuestCounter_CanJoin(s16 idx);
s32 Quest_IsDownloadId(s32);
extern u16 gLobbyLevel2Count;

s32 QuestCounter_ChooseQuest(u16 pad)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    s32 btn = pad & 0xFFFF;
    PostedQuest *e = &gPostedQuests[w->quest];

    if (btn & 0x20) {
        if (QuestCounter_CanJoin(w->quest) == 0) {
            LobbySe_Play(9);
            return 2;
        }
        if (Quest_IsDownloadId(e->quest) == 1 && (gLobby->dlQuestsReady == 0 || Quest_FindDownloaded(e->quest) == NULL)) {
            LobbySe_Play(9);
            return 1;
        }
        gLobbyLevel2Index = w->quest;
        w->questId = e->quest;
        switch (e->desk) {
        case 3:
            w->pages = 2;
            break;
        default:
            w->pages = 4;
            w->passKind = 0;
            break;
        }
        LobbySe_Play(0);
        return 0;
    }
    if (btn & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    w->quest = Menu_WrapUpDown(w->quest, gLobbyLevel2Count);
    return 2;
}

s32 QuestCounter_CanJoin(s16 idx);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);
s32 Menu_PushNoticeOnce(s32 arg0);

s32 QuestCounter_ViewRoom(u16 pad)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    LevelEntry *r = &gLobbyLevel2Entries[w->quest];
    s32 btn;

    if (QuestCounter_CanJoin(w->quest) == 0) {
        switch (r->state) {
        case 4:
            Menu_PushNoticeOnce(10);
            break;
        default:
            Menu_PushNoticeOnce(9);
            break;
        }
        LobbySe_Play(4);
        return 3;
    }
    btn = pad & 0xFFFF;
    if (btn & 0x20) {
        gLobbyLevel2Index = w->quest;
        LobbySe_Play(0);
        return 0;
    }
    if (btn & 0x40) {
        w->step = 0x10;
        LobbySe_Play(4);
        return 3;
    }
    w->page = Menu_WrapLeftRight(w->page, w->pages);
    return 2;
}

s32 Quest_CheckOrderTerms(u8 id, u8 flag);
void Talk_Say(s32 arg0, s8 arg1);
s32 LobbyTime_IsPm(void);

/* Can the selected quest of the board be taken? 1 (with a message) when not:
 * a kind-3 quest offline or without its item, a downloaded quest that is
 * not there, the rank (Quest_CheckOrderTerms), the fee. */
s32 QuestCounter_CheckQuest(void)
{
    PostedQuest *e = &gPostedQuests[((QuestCounterMenu *)&gMenuWork)->quest];
    QuestInfo *q;

    if (e->desk == 3) {
        if (LobbyTime_IsPm() == 0) {
            LobbyText_Open(0x91);
            return 1;
        }
        if (gLobbyRecs.rec[2].cur == 0 && *(s16 *)((u8 *)&gSave + 0x2932) == 0) {
            LobbyText_Open(0x66);
            return 1;
        }
    }
    q = Quest_GetInfoAny(e->quest);
    if (Quest_IsDownloadId(e->quest) == 1 && (gLobby->dlQuestsReady == 0 || q->id != e->quest)) {
        Talk_Say(4, 0);
        return 1;
    }
    if (Quest_CheckOrderTerms(q->terms, 1) == 0) {
        if (e->desk == 3) {
            LobbyText_Open(0x92);
        } else {
            LobbyText_Open(0x65);
        }
        return 1;
    }
    if (e->money > gSave.money) {
        LobbyText_Open(0x68);
        return 1;
    }
    return 0;
}

u16 Pad_GetPressedRepeat(s32 pad);
void * LobbyLevel2_Entry(s32 arg0);
s8 Kbd_Run(char *dst, s16 held, s16 pressed);
void Kbd_Cancel(void);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
u8 Kbd_UsbNewKey(void);

s32 QuestCounter_KbdInput(void *task)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    LevelEntry *q;
    s32 step;

    Pad_GetPressedRepeat(0);
    Kbd_UsbNewKey();
    q = LobbyLevel2_Entry(w->quest);
    switch (step = w->subStep) {
    case 0:
        w->subStep = step + 1;
        Kbd_SetPos(0x3A, 80.0f);
        Chat_OpenKbd(0, 6, 8, task);
        gFieldControl = 0;
        break;
    case 1:
        if ((s8)Kbd_Run(task, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            (w->subStep)++;
            break;
        }
        if (QuestCounter_CanJoin(w->quest) == 0) {
            switch (q->state) {
            case 4:
                Menu_PushNoticeOnce(0xA);
                break;
            default:
                Menu_PushNoticeOnce(9);
                break;
            }
            Kbd_Cancel();
            w->subStep = 0;
            gFieldControl = 1;
            return 3;
        }
        break;
    case 2:
        Kbd_Cancel();
        w->subStep = 0;
        gFieldControl = 1;
        return 0;
    }
    return 2;
}

s32 QuestCounter_CanJoin(s16 idx)
{
    LevelEntry *r = &gLobbyLevel2Entries[idx];
    PostedQuest *q = &gPostedQuests[idx];

    if (r->state != 3 || r->users >= r->maxUsers) {
        return 0;
    }
    return q->area == gScene.area;
}


/* Opens the quest board's five windows (+0x480, 8 bytes each) one after
 * another (+0x4AF the step, +0x4AC a delay) and runs the open ones.
 * Returns 2 while any is moving, 0 once all are done. */
s32 QuestCounter_OpenWindows(s32 x, s32 y)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    s16 busy = 0;
    s16 i;

    for (i = 0; i < 5; i++) {
        if (w->board[i].state != 0) {
            WinSlide_Step(&w->board[i], x, y);
            busy = 1;
        }
    }
    switch (w->subStep) {
    case 0:
        w->delay = 0;
        WinSlide_Step(w->board, x, y);
        w->board[1].pos = w->board[2].pos = w->board[3].pos = w->board[4].pos = x;
        busy = 1;
        (w->subStep)++;
        break;
    case 1:
        if (++w->delay > 0) {
            (w->subStep)++;
            w->delay = 0;
            WinSlide_Step(&w->board[1], x, y);
            busy = 1;
        }
        break;
    case 2:
        if (++w->delay > 0) {
            (w->subStep)++;
            w->delay = 0;
            WinSlide_Step(&w->board[2], x, y);
            busy = 1;
        }
        break;
    case 3:
        if (++w->delay > 0) {
            (w->subStep)++;
            w->delay = 0;
            WinSlide_Step(&w->board[3], x, y);
            busy = 1;
        }
        break;
    case 4:
        if (++w->delay > 0) {
            (w->subStep)++;
            w->delay = 0;
            WinSlide_Step(&w->board[4], x, y);
            busy = 1;
        }
        break;
    }
    if (busy == 0) {
        w->subStep = 0;
        return 0;
    }
    return 2;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_76DB40", D_0083B428);

void Text_PrintfColored(const char *, ...);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
s32 sprintf(char *, const char *, ...);
void Sjis_Widen(char *, char *);
extern char D_0083B428[];
extern char *D_0080C684[];
extern char *gArenaQuestTexts[];
extern char *gQuestHallNameTexts[];
extern char *D_0080C19C[];
extern char *gQuestTakerTexts[];
extern char *D_0080C1B4;
extern char *gWantedFeeTexts[];
char *strcpy(char *, const char *);
void Text_Printf(const char *fmt, ...);

/* Level 4 for this function (the file's other functions match at both 3 and
 * 4: an assumption about the unit's build). The second half draws from x + 0x40. */
#pragma optimization_level 4
s32 QuestCounter_DrawRecruitInfo(s16 x, s16 y, s16 i)
{
    PostedQuest *q;
    LevelEntry *r;
    u32 c2;
    u32 ret;
    u32 c1;
    char *s;
    s32 ty;
    char tmp[0x40];
    char buf[0x40];

    q = &gPostedQuests[i];
    r = &gLobbyLevel2Entries[i];
    Font_SetSpacing(0x12, 0x12);
    if (q->area == gScene.area) {
        c1 = 0x80101010;
        c2 = 0x80301010;
        ret = 0xFFE0D8C8;
    } else {
        c1 = 0x80101010;
        c2 = 0x80401017;
        ret = 0xFF808080;
    }
    Font_SetColor(c1);
    ty = y + 0x1B;
    Font_SetPos(x + 0x24, ty);
    Text_PrintfColored(D_0080C684[i]);
    c1 = 0x80101010;
    if (r->state != 3 || q->quest == 0) {
        switch (r->state) {
        case 0:
        case 1:
        case 7:
        case 8:
            s = gArenaQuestTexts[1];
            break;
        case 2:
            s = gArenaQuestTexts[3];
            break;
        case 4:
            s = gArenaQuestTexts[4];
            c1 = 0x807F5230;
            break;
        case 5:
            s = gArenaQuestTexts[5];
            break;
        case 6:
            s = gArenaQuestTexts[6];
            break;
        default:
            s = NULL;
            break;
        }
        if (s != NULL) {
            Font_SetColor(c1);
            Font_SetPos(Text_CenterX(s, x + 0xBE, 0x12), ty);
            Text_PrintfColored(s);
        }
        return 0xFF808080;
    }
    if (q->money != 0) {
        y += 8;
    } else {
        y += 0x11;
    }
    x += 0x40;
    Font_SetPos(x, y);
    switch (q->area) {
    case 0x86:
        s = gQuestHallNameTexts[0];
        break;
    case 0x87:
        s = gQuestHallNameTexts[1];
        break;
    case 0x88:
    default:
        s = gQuestHallNameTexts[2];
        break;
    }
    Text_Printf(D_0083B428, s, D_0080C19C[q->desk]);
    Font_SetColor(c1);
    memset(tmp, 0, 0x11);
    if (gChatHideNames) {
        strcpy(tmp, (char *)q + 7);
    } else {
        strcpy(tmp, (char *)q + 0xE);
    }
    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(D_0083B428, gQuestTakerTexts[0], tmp);
    Font_SetPos(x + 0xA2, y);
    sprintf(tmp, D_0080C1B4, r->users, r->maxUsers);
    Sjis_Widen(tmp, buf);
    Text_PrintfColored(buf);
    if (q->money != 0) {
        Font_SetColor(c2);
        Font_SetPos(x, y + 0x14);
        Text_PrintfColored(gWantedFeeTexts[0], q->money);
    }
    return ret;
}
#pragma optimization_level reset

void Font_SetSpacing(u8 a, u8 b);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
s32 QuestCounter_DrawRecruitInfo(s16 x, s16 y, s16 i);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 colour, s32 spr, u8 a);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
extern char *D_0080C18C;
extern u16 gFrameTick;

/* The quest board's list: a panel per quest (its colour by
 * QuestCounter_DrawRecruitInfo), a mark on those with +0x17 set, and blinking arrows
 * beside the one under the cursor (+0x4B6 of the menu work area). */
void QuestCounter_DrawRecruit(WinSlide *pos, s16 y)
{
    QuestCounterMenu *m = (QuestCounterMenu *)&gMenuWork;
    s16 i;
    s32 c;
    s16 x;
    s16 d;

    Font_SetSpacing(0x14, 0x14);
    Menu_DrawLabelBar(0xF0, 0x12, 0xA0, 0xFFB0B000, D_0080C18C, 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    for (i = 0; i < gLobbyLevel2Count; i++, pos++) {
        x = pos->pos;
        c = QuestCounter_DrawRecruitInfo(x, y, i);
        Icon_DrawMode(x, y, 0x17C, 0x48, c, 0, i % 4);
        if (gLobbyLevel2Entries[i].unk17 != 0) {
            Icon_Draw(x + 0x14, y + 4, 0x1A, 0x16, -1, 0x47);
        }
        if (i == m->quest) {
            if (gFrameTick & 0x10) {
                d = 10;
            } else {
                d = 0;
            }
            Icon_Draw(x - 10 + d, y + 0x14, 0x20, 0x20, 0xFFA0A040, 0x29);
            Icon_Draw(x + 0x166 - d, y + 0x14, 0x20, 0x20, 0xFFA0A040, 0x28);
        }
        y += 0x4E;
    }
}

/* unit's own declaration: with the definition's u8 (EquipItem *e) (sub_main/unk_3B4C80.c), Quest_DrawArenaTeams: 4 of 528 words differ, Quest_DrawMembers: 1 of 105 words differ */
s32 Equip_GetClass(EquipItem *e);
char *Quest_GetText(u16 id, s32 k);
void Text_PrintfColored(const char *, ...);
void Icon_DrawKind48(s16, s16, s32, s32, u8, s32); /* unit's own declaration: with the definition's void (s32 a, s32 b, s32 c, s32 d, s16 e, s8 f) (lobby/unk_78F070.c), Quest_DrawMembers 58 of 105 words, Quest_DrawArenaTeams 432 of 528 words differ */
s32 sprintf(char *, const char *, ...);
void Font_SetPos(s16, s16);
void Sjis_Widen(char *, char *);
extern char D_0083B428[];
extern char *D_0080D61C;
extern char *gMemberInfoTexts[];

/* The quest's title and the four members (+0x11C each in the menu work
 * area: the weapon class icon, the name and the hunter rank). y steps once
 * inside a call's arguments, as the game's code does. */
void Quest_DrawMembers(s16 x, s16 y, QuestInfo *quest)
{
    char tmp[0x40];
    char buf[0x60];
    NetUser *p;
    s32 i;

    p = ((QuestCounterMenu *)&gMenuWork)->members;
    Font_SetPos(x, y += 0x14);
    Text_PrintfColored("%s", Quest_GetText(quest->id, 0));
    y += 0x32;
    Font_SetColor(0x80101010);
    x += 0x32;
    for (i = 0; i < 4; i++) {
        if (p->id[0] != 0) {
            Icon_DrawKind48(x - 0x36, y + 2, 0x28, -1, Equip_GetClass(&((HunterCard *)p->text)->weapon), 1);
            sprintf(tmp, D_0083B428, D_0080D61C, p->name);
            Sjis_Widen(tmp, buf);
            Font_SetPos(x, y);
            Text_PrintfColored(buf);
            y += 0x14;
            sprintf(tmp, D_0083B428, gMemberInfoTexts[0], p->id);
            Sjis_Widen(tmp, buf);
            Font_SetPos(x, y);
            Text_PrintfColored(buf);
        }
        y += 0x1A;
        p++;
    }
}

void Quest_DrawOverview(s16, s16, QuestInfo *);
void Quest_DrawTerms(s16, s16, QuestInfo *);
void Quest_DrawDetails(s16, s16, QuestInfo *);
void Quest_DrawMembers(s16, s16, QuestInfo *);

void QuestCounter_DrawQuestPage(s16 x, s16 y)
{
    QuestCounterMenu *w = (QuestCounterMenu *)&gMenuWork;
    QuestInfo *q;

    q = Quest_GetInfoAny(gPostedQuests[w->quest].quest);
    Font_SetColor(0x80101010);
    switch (w->page) {
    case 0:
        Quest_DrawOverview(x + 0x28, y, q);
        break;
    case 1:
        Quest_DrawTerms(x + 0x28, y, q);
        break;
    case 2:
        Quest_DrawDetails(x + 0x28, y, q);
        break;
    case 3:
        Quest_DrawMembers(x + 0x28, y, q);
        break;
    }
}

u8 Lobby_FindMember(u8 *id);
extern char *gArenaTournamentTexts[];
extern char *D_0080CCDC;
extern char *gArenaTeamTexts[];
extern char *D_0080CCE4;
extern NetUser gNetUsers[];

/* The four members of list grouped by their gNetUsers entry's card's
 * arenaRole (1 and 2 under D_0080CCDC, 3 and 4 under gArenaTeamTexts), D_0080CCE4 for
 * an empty group. Each loop has its own pointer; n = 0 before the call in
 * the second half. */
void Quest_DrawArenaTeams(s16 x, s16 y, NetUser *list, s8 room)
{
    s16 ty;
    char tmp[0x40];
    s16 tx;
    NetUser *p2;
    NetUser *e;
    NetUser *p4;
    char *s;
    s16 i;
    NetUser *p3;
    char buf[0x60];
    s8 m;
    NetUser *p1;
    s16 n;
    HunterCard *h;

    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(gArenaTournamentTexts[0], x + 0xA0, 0x12), y += 0x14);
    Text_PrintfColored(gArenaTournamentTexts[0]);
    Font_SetPos(Text_CenterX(D_0080CCDC, x + 0xA0, 0x12), y += 0x28);
    Text_PrintfColored(D_0080CCDC);
    y += 0x1E;
    tx = x + 0x5A;
    Font_SetColor(0x80101010);
    for (n = 0, i = 0, p1 = list; i < 4; i++, p1++) {
        if (p1->id[0] != 0) {
            m = Lobby_FindMember((u8 *)p1->id);
            e = &gNetUsers[m];
            h = (HunterCard *)e->text;
            if (((HunterCard *)e->text)->arenaRole == 1) {
                ty = y;
                Icon_DrawKind48(tx - 0x36, ty + 2, 0x28, -1, Equip_GetClass(&h->weapon), 1);
                sprintf(tmp, D_0083B428, D_0080D61C, p1->name);
                Sjis_Widen(tmp, buf);
                Font_SetPos(tx, y);
                Text_PrintfColored(buf);
                sprintf(tmp, D_0083B428, gMemberInfoTexts[0], p1->id);
                Sjis_Widen(tmp, buf);
                Font_SetPos(tx, ty + 0x14);
                Text_PrintfColored(buf);
                y += 0x2E;
                n++;
            }
        }
    }
    for (n = 0, i = 0, p2 = list; i < 4; i++, p2++) {
        if (p2->id[0] != 0) {
            m = Lobby_FindMember((u8 *)p2->id);
            e = &gNetUsers[m];
            h = (HunterCard *)e->text;
            if (((HunterCard *)e->text)->arenaRole == 2) {
                ty = y;
                Icon_DrawKind48(tx - 0x36, ty + 2, 0x28, -1, Equip_GetClass(&h->weapon), 1);
                sprintf(tmp, D_0083B428, D_0080D61C, p2->name);
                Sjis_Widen(tmp, buf);
                Font_SetPos(tx, y);
                Text_PrintfColored(buf);
                sprintf(tmp, D_0083B428, gMemberInfoTexts[0], p2->id);
                Sjis_Widen(tmp, buf);
                Font_SetPos(tx, ty + 0x14);
                Text_PrintfColored(buf);
                y += 0x2E;
                n++;
            }
        }
    }
    if (n == 0 && gLobbyLevel2Entries[room].maxUsers >= 3) {
        s = D_0080CCE4;
        Font_SetColor(0x80101050);
        Font_SetPos(Text_CenterX(s, x + 0xA0, 0x12), y);
        Text_PrintfColored(s);
    }
    y = 0xF4;
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(gArenaTeamTexts[0], x + 0xA0, 0x12), y += 0x1E);
    Text_PrintfColored(gArenaTeamTexts[0]);
    y += 0x1E;
    tx = x + 0x5A;
    n = 0;
    Font_SetColor(0x80101010);
    for (i = 0, p3 = list; i < 4; i++, p3++) {
        if (p3->id[0] != 0) {
            m = Lobby_FindMember((u8 *)p3->id);
            e = &gNetUsers[m];
            h = (HunterCard *)e->text;
            if (((HunterCard *)e->text)->arenaRole == 3) {
                ty = y;
                Icon_DrawKind48(tx - 0x36, ty + 2, 0x28, -1, Equip_GetClass(&h->weapon), 1);
                sprintf(tmp, D_0083B428, D_0080D61C, p3->name);
                Sjis_Widen(tmp, buf);
                Font_SetPos(tx, y);
                Text_PrintfColored(buf);
                sprintf(tmp, D_0083B428, gMemberInfoTexts[0], p3->id);
                Sjis_Widen(tmp, buf);
                Font_SetPos(tx, ty + 0x14);
                Text_PrintfColored(buf);
                y += 0x2E;
                n++;
            }
        }
    }
    if (n == 0) {
        s = D_0080CCE4;
        Font_SetColor(0x80101050);
        Font_SetPos(Text_CenterX(s, x + 0xA0, 0x12), y);
        Text_PrintfColored(s);
        y += 0x2E;
    }
    n = 0;
    Font_SetColor(0x80101010);
    for (i = 0, p4 = list; i < 4; i++, p4++) {
        if (p4->id[0] != 0) {
            m = Lobby_FindMember((u8 *)p4->id);
            e = &gNetUsers[m];
            h = (HunterCard *)e->text;
            if (((HunterCard *)e->text)->arenaRole == 4) {
                ty = y;
                Icon_DrawKind48(tx - 0x36, ty + 2, 0x28, -1, Equip_GetClass(&h->weapon), 1);
                sprintf(tmp, D_0083B428, D_0080D61C, p4->name);
                Sjis_Widen(tmp, buf);
                Font_SetPos(tx, y);
                Text_PrintfColored(buf);
                sprintf(tmp, D_0083B428, gMemberInfoTexts[0], p4->id);
                Sjis_Widen(tmp, buf);
                Font_SetPos(tx, ty + 0x14);
                Text_PrintfColored(buf);
                y += 0x2E;
                n++;
            }
        }
    }
    if (n == 0 && gLobbyLevel2Entries[room].maxUsers >= 3) {
        s = D_0080CCE4;
        Font_SetColor(0x80101050);
        Font_SetPos(Text_CenterX(s, x + 0xA0, 0x12), y);
        Text_PrintfColored(s);
    }
}

s32 ArenaCup_IsKindOpen(s8);
extern char *gArenaTournamentTexts[];
extern char *D_0080C074;

/* gLobbyRecs read as rows of 0x14A8 bytes from its start (row i is rec[i] less 0x55 bytes: the
 * name at +0x56, the value chosen at +0x9A, the values' texts at +0xBD): Quest_DrawPartyList
 * reads it so (with &gLobbyRecs.rec[i] MWCC keeps one more register). */
typedef struct LobbyRecRow {
    /* 0x000 */ u8 unk0[0x56];
    /* 0x056 */ char name[0x41];
    /* 0x097 */ u8 m;
    /* 0x098 */ u8 unk98[2];
    /* 0x09A */ u8 cur;
    /* 0x09B */ u8 unk9B[0xBD - 0x9B];
    /* 0x0BD */ char subName[0x20][0x41];
    /* 0x8DD */ u8 unk8DD[0x14A8 - 0x8DD];
} LobbyRecRow;
typedef char LobbyRecRow_size_check[(sizeof(LobbyRecRow) == 0x14A8) ? 1 : -1];

/* The list of the gLobbyRecs.n records of gLobbyRecs (their names and the values chosen) under the
 * title gArenaTournamentTexts; the fourth only when ArenaCup_IsKindOpen(4). */
void Quest_DrawPartyList(s16 x, s32 y0, s32 k)
{
    s16 y = y0;
    s16 i;
    LobbyRecRow *p;

    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80301010);
    Font_SetPos(Text_CenterX(gArenaTournamentTexts[0], x + 0xA0, 0x12), y += 0x14);
    Text_PrintfColored(gArenaTournamentTexts[0]);
    y += 0x14;
    for (i = 0; i < gLobbyRecs.n; i++) {
        p = &((LobbyRecRow *)&gLobbyRecs)[i];
        if (i == 3) {
            if (ArenaCup_IsKindOpen(4) == 0) {
                continue;
            }
        } else if (i == 3) { /* never true: the game's test */
            if (ArenaCup_IsKindOpen(3) == 0) {
                continue;
            }
        }
        Font_SetColor(0x80301010);
        Font_SetPos(x + 0x28, y += 0x14);
        Text_PrintfColored("%10s%s", p->name, D_0080C074);
        Font_SetColor(0x80101010);
        Font_SetPos((s16)(x + 0x28) + 0x6C, y);
        Text_PrintfColored(p->subName[p->cur]);
    }
}


/* the padding at the end of the file's .rodata */
const u8 D_0083B468[8] = { 0 };
