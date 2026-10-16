#include "common.h"
#include "lobby_state.h"

#include "scene.h"
#include "player.h"
#include "save.h"
#include "lobby_globals.h"
#include "quest.h"
#include "lobby_menu.h"
u16 Rand_Next(s32);


/* The work area of the village's menus (0xF00 bytes at 0x8BDC20), as these
 * menus use it. */
typedef struct {
    u8 unk0[7];
    s8 postStep;    /* 0x07: Quest_PostStep's (QuestPostHead) */
    u8 unk8[4];
    Msg650B req;    /* 0x0C: the posting (Quest_PostStep) */
    QuestInfo *quest; /* 0x24: Quest_GetInfoAny(gLobbyQuestId) */
    WinSlide win;   /* 0x28: the window (WinSlide_Step) */
    u8 unk30[8];
    WinShake shake;     /* 0x38: Win_Shake */
    WinShake shake2;    /* 0x40: Win_Shake */
    s8 unk48[2];        /* 0x48: QuestDesk_Cancel's state */
    s16 items[5];   /* 0x4A */
    s16 count;      /* 0x54 */
    u8 unk56[0x60 - 0x56];
    s16 unk60;      /* 0x60: a cursor */
    s16 unk62;      /* 0x62: its row count */
    s16 unk64;      /* 0x64 */
    s16 unk66;
    s16 unk68;      /* 0x68 */
    s8 unk6A;
    s8 state;       /* 0x6B */
    s8 unk6C;       /* 0x6C: a second state */
    u8 unk6D;
    s16 cursor;     /* 0x6E */
    s16 list;       /* 0x70: the list shown */
    s16 listCount;  /* 0x72 */
    u8 unk74[3];
    u8 unk77;       /* 0x77 */
    u16 lists[5][12]; /* 0x78: quest ids per list, 0 ends a list */
} VMenu;

extern s16 gQuestMapGroups[];
extern u8 gMenuWork[];
extern u16 gLobbyQuestId;
extern LobbyMenu *gLobbyMenu;
extern NetUser gNetUsers[];
void *memcpy(void *dst, void *src, u32 n);
s32 MapGroup_IsOpen(u16 id);
s32 QuestDesk_RoomSettings(u16 pad);
void HunterCard_Build(void *out);
void Money_PayPending(u8 silent);
s32 Lobby_HasQuest(void);
void Talk_Say(s32 a, s8 b);
QuestInfo *Quest_GetInfoAny(u16 id);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 Menu_WrapUpDownSe(s32 cur, s32 n, s32 se);
void LobbyText_Open(s32 id);
s32 Quest_PostStep(Msg650B *req);
s32 Talk_Update(void);
s32 Game_IsOnline(void);
void CamFocus_SetAside(s8 a);
s32 LobbyStep_6501(void);
s32 Quest_CheckOrderTerms(u8 id, u8 flag);
void LobbyDialog_Grow(void);
extern s8 gQuestMinMembers;
extern s8 gRoomPassOn;
extern s8 gRoomPassword[9];
s32 Menu_PushNoticeOnce(s32);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
s32 LobbyMenu_KbdEntry(void *opts);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
void *memset(void *dst, s32 c, u32 n);
void QuestDesk_Draw(u8 *task);
void QuestDesk_Draw2(u8 *task);
extern u16 gTalkLine;
extern char *gTalkLineText;
extern s8 gTalkMode;
QuestInfo *Quest_GetDiscInfo(u16 id);
s32 QuestDesk_IsWanted(u8 *p);
s16 Wanted_GetValue(void);
char *Talk_LineText(u16 id);
char *Text_FillDollar(char *fmt, s32 n);
char *Text_FillDollar2(s32 arg0);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);

s16 QuestDesk_CountOpenGroups(void *m)
{
    s16 i;
    s16 n = 0;
    s16 *p = gQuestMapGroups;

    for (i = 0; i < 7; i++, p++) {
        if (MapGroup_IsOpen(*p) == 1) {
            if (*p == 2 && (gScene.area == 0x86 || gScene.area == 0x89)) {
                continue;
            }
            if (*p != 0xD) {
                n++;
            }
        }
    }
    return n;
}

s32 QuestDesk_Start(void);
s32 QuestDesk_TopMenu(u16 pad);
s32 QuestDesk_ChooseList(u16 pad);
s32 QuestDesk_ChooseQuest(u16 pad);
s32 QuestDesk_CheckFee(u16 pad);
s32 QuestDesk_Post(u16 pad);
s32 QuestDesk_Cancel(s8 *st, u16 pad);
s32 QuestDesk_WaitBack(u16 pad);
s32 QuestDesk_ScrollInfo(u16 pad);
u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
void LobbySe_Play(s32 se);
s32 Win_Shake(WinShake *s);
void QuestDesk_SayBye(void);
void QuestDesk_BuildLists(void);
void QuestRoom_ResetSettings(void);
void QuestDesk_Close(void);
void QuestDesk_Accept(u16 id, s16 money, s32 pay);
extern s16 gQuestStampRand;

/* The quest counter's steps (m->unk6A): its menus return 0 (chosen) or 3
 * (cancelled); each test of a call's result is a switch (one-case ones
 * too), as the game's branch layout shows. */
void QuestDesk_Update(void)
{
    VMenu *m = (VMenu *)gMenuWork;
    u16 pad;
    s16 item;

    pad = LobbyPad_Get(0);
    Chat_SetWindowMode(1, 2);
    switch (m->unk6A) {
    case 0:
        if (QuestDesk_Start() == 1) {
            m->unk6A++;
            WinSlide_Step(&m->win, 0x280, 0x118);
            CamFocus_SetAside(1);
            LobbySe_Play(0x16);
            break;
        }
        m->unk6A = 0xC;
        m->quest = Quest_GetInfoAny(gLobbyQuestId);
        m->unk68 = gRewardMoney - m->quest->fee;
        Talk_Say(0xC, 1);
        Talk_Update();
        CamFocus_SetAside(1);
        m->unk77 = 1;
        LobbySe_Play(0x17);
        break;
    case 1:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->unk6A++;
            break;
        }
        break;
    case 2:
        switch (QuestDesk_TopMenu(pad)) {
        case 0:
            item = m->items[m->cursor];
            switch (item) {
            case 4:
            case 3:
                m->unk6A++;
                m->list = 0;
                m->unk60 = 0;
                QuestDesk_BuildLists();
                break;
            case 2:
                QuestDesk_BuildLists();
            case 0:
            case 1:
                m->unk6A = 4;
                m->unk60 = 0;
                WinSlide_Step(&m->win, 0x118, 0x280);
                break;
            }
            break;
        case 3:
            m->unk6A = 0xA;
            QuestDesk_SayBye();
            break;
        }
        break;
    case 3:
        switch (QuestDesk_ChooseList(pad)) {
        case 0:
            m->unk6A++;
            m->unk60 = 0;
            WinSlide_Step(&m->win, 0x118, 0x280);
            break;
        case 3:
            m->unk6A = 2;
            break;
        case 1:
            m->unk6A = 0xA;
            QuestDesk_SayBye();
            break;
        }
        break;
    case 4:
        switch (WinSlide_Step(&m->win, 0x118, 0x280)) {
        case 0:
            switch (m->items[m->cursor]) {
            case 4:
            case 3:
                m->unk6A++;
                break;
            case 0:
                m->unk6A = 0xD;
                break;
            case 1:
                m->unk6A = 0xE;
                break;
            case 2:
                m->unk6A++;
                break;
            }
            break;
        }
        break;
    case 5:
        switch (QuestDesk_ChooseQuest(pad)) {
        case 0:
            m->unk6A++;
            gQuestStampRand = Rand_Next(1);
            break;
        case 3:
            m->unk6A = 0xF;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 6:
        switch (QuestDesk_CheckFee(pad)) {
        case 0:
            if (Game_IsOnline() == 0) {
                if (QuestDesk_IsWanted((u8 *) m->quest) == 1) {
                    Win_Shake(&m->shake2);
                    m->unk6A = 8;
                } else {
                    Win_Shake(&m->shake);
                    m->unk6A = 9;
                }
                break;
            }
            m->unk6A++;
            *(s16 *)((u8 *)m + 0x198) = m->unk60;
            m->unk60 = 0;
            QuestRoom_ResetSettings();
            LobbySe_Play(0x17);
            break;
        case 3:
            m->unk6A = 0xF;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 7:
        switch (QuestDesk_Post(pad)) {
        case 0:
            if (QuestDesk_IsWanted((u8 *) m->quest) == 1) {
                m->unk6A++;
                Win_Shake(&m->shake2);
            } else {
                m->unk6A = 9;
                Win_Shake(&m->shake);
            }
            m->unk60 = *(s16 *)((u8 *)m + 0x198);
            break;
        case 1:
            m->unk6A = 0xB;
            break;
        }
        break;
    case 8:
        gFieldControl = 0;
        switch (Win_Shake(&m->shake2)) {
        case 0:
            m->unk6A++;
            Win_Shake(&m->shake);
            gFieldControl = 1;
            break;
        }
        break;
    case 9:
        switch (Win_Shake(&m->shake)) {
        case 0:
            m->unk6A++;
            if (Game_IsOnline() == 0) {
                Talk_Say(0xA, 0);
            } else if (gQuestMinMembers == 0) {
                Talk_Say(0xA, 0);
            } else {
                Talk_Say(0xB, 0);
            }
            QuestDesk_Accept(m->quest->id, m->unk68, 1);
            break;
        }
        break;
    case 10:
        switch (Talk_Update()) {
        case 0:
            m->unk6A = 0xB;
            break;
        }
        break;
    case 11:
        QuestDesk_Close();
        break;
    case 12:
        switch (QuestDesk_Cancel(m->unk48, pad)) {
        case 0:
            m->unk6A = 0xB;
            CamFocus_SetAside(0);
            break;
        case 3:
            QuestDesk_SayBye();
            m->unk6A = 0xA;
            break;
        }
        break;
    case 13:
        switch (QuestDesk_ScrollInfo(pad)) {
        case 0:
            m->unk6A = 0xF;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 14:
        switch (QuestDesk_WaitBack(pad)) {
        case 0:
            m->unk6A = 0xF;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 15:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            switch (m->items[m->cursor]) {
            case 4:
            case 3:
                m->unk6A = 3;
                break;
            default:
                m->unk6A = 2;
                break;
            }
            break;
        }
        break;
    }
}

extern u8 gQuestIsOfficial;
extern u8 gMemberNamesOff;
void Quest_ClearVersusDefeat(void);
void Quest_SetInfo(QuestInfo *q);
void NetQuest_SetStage(u16 id);
s32 QuestFlags_GetCategory(u32 *arg0);
void Save_AddMoney(s32 n);
s32 LobbyText_Update(void);
s32 LobbyWorld_GetSeason(void);
s32 LobbyTime_IsPm(void);
s32 LobbyWorld_GetSeasonDay(void);

void QuestDesk_Accept(u16 id, s16 money, s32 pay)
{
    QuestInfo *q = Quest_GetInfoAny(id);
    s32 flags;

    Quest_ClearVersusDefeat();
    gRewardMoney = money;
    if (Game_IsOnline() == 1) {
        D_5CA910 = gScene.area;
    }
    gQuestIsOfficial = 0;
    gLobbyQuestId = id;
    gLobby->questTaken = 1;
    Quest_SetInfo(q);
    NetQuest_SetStage(id);
    gScene.questId = id;
    LobbySe_Play(0xA);
    flags = QuestFlags_GetCategory((u32 *) q);
    if ((u8)pay) {
        gRewardMoney += q->fee;
        gLobby->questPaid = 1;
        gScene.questPaid = gLobby->questPaid;
        gPlayers[gScene.localPlayer].questStatus = flags | 0x80;
    } else {
        gPlayers[gScene.localPlayer].questStatus = flags | 0x40;
    }
    if (gRewardMoney != 0) {
        Save_AddMoney(-gRewardMoney);
    }
    HunterCard_Build(gLobby->members[gScene.localPlayer].text);
    HunterCard_Build(gNetUsers[gScene.localPlayer].text);
    gNextQuest = q->flags;
    D_6ADB4A = q->map;
    if (Game_IsOnline() == 1) {
        LobbyText_Update();
        gMemberNamesOff = 0;
        if (*(u16 *)(gLobbyWorkBuf + 0x7A94) != 0) {
            gQuestQNetReadyArg = *(u16 *)(gLobbyWorkBuf + 0x7A94);
        }
    } else {
        gQuestOfferId = 0;
        gLobbyLastSeason = LobbyWorld_GetSeason();
        gLobbyLastPm = LobbyTime_IsPm();
        gQuestAcceptDay = LobbyWorld_GetSeasonDay();
    }
}

void QuestDesk_Drop(void)
{
    gLobby->questTaken = 0;
    gPlayers[gScene.localPlayer].questStatus = 0;
    HunterCard_Build(gLobby->members[gScene.localPlayer].text);
    memcpy(gNetUsers[gScene.localPlayer].text, gLobby->members[gScene.localPlayer].text, 0x100);
    gQuestOfferId = 0;
    if (gRewardMoney > 0) {
        Money_PayPending(0);
    } else {
        LobbySe_Play(4);
    }
    gLobby->questPaid = 0;
}

s32 QuestDesk_Start(void)
{
    VMenu *m = (VMenu *)gMenuWork;

    if (Lobby_HasQuest() == 0) {
        Talk_Say(0, 0);
        return 1;
    }
    m->quest = Quest_GetInfoAny(gLobbyQuestId);
    Talk_Say(0xC, 1);
    return 0;
}

s32 QuestDesk_TopMenu(u16 pad)
{
    VMenu *m = (VMenu *)gMenuWork;

    if (pad & 0x20) {
        switch (m->items[m->cursor]) {
        case 0:
            m->unk62 = QuestDesk_CountOpenGroups(m);
            if (m->unk62 == 0) {
                LobbySe_Play(9);
            } else {
                LobbySe_Play(0x17);
                return 0;
            }
            break;
        case 2:
            switch (gScene.area) {
            case 0x88:
                if (D_5BFA68 == 0) {
                    LobbySe_Play(9);
                } else {
                    LobbySe_Play(0x17);
                    return 0;
                }
                break;
            default:
                if (gQuestDeskShown[0][0] == 0) {
                    LobbySe_Play(9);
                } else {
                    LobbySe_Play(0x17);
                    return 0;
                }
                break;
            }
            break;
        case 1:
            LobbySe_Play(0x17);
            return 0;
        default:
            LobbySe_Play(8);
            return 0;
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else {
        m->cursor = Menu_WrapUpDown(m->cursor, m->count);
    }
    return 2;
}

s32 QuestDesk_ChooseList(u16 pad)
{
    VMenu *m = (VMenu *)gMenuWork;

    switch (m->state) {
    case 0:
        if (pad & 0x20) {
            if (m->lists[m->list][0] == 0) {
                m->state = 2;
                Talk_Say(9, 0);
            } else {
                m->state++;
                Talk_Say(2, 0);
            }
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            return 3;
        } else {
            m->list = Menu_WrapUpDown(m->list, m->listCount);
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case 0:
            m->state = 0;
            LobbySe_Play(0x17);
            return 0;
        }
        break;
    case 2:
        switch (Talk_Update()) {
        case 0:
            m->state = 0;
            break;
        case 3:
            m->state = 0;
            return 1;
        }
        break;
    }
    return 2;
}

s32 QuestDesk_ChooseQuest(u16 pad)
{
    VMenu *m = (VMenu *)gMenuWork;
    char *s;
    u16 n;

    switch (m->state) {
    case 0:
        n = 0;
        while (m->lists[m->list][n] != 0) {
            n++;
            if (n >= 12) {
                break;
            }
        }
        m->unk77 = 0;
        if (pad & 0x20) {
            m->unk68 = 0;
            m->quest = Quest_GetDiscInfo(m->lists[m->list][m->unk60]);
            gLobbyQuestId = m->quest->id;
            if (m->quest->fee != 0) {
                if (QuestDesk_IsWanted((u8 *) m->quest) == 1) {
                    Talk_Say(7, 0);
                    s = Talk_LineText(gTalkLine);
                    m->unk68 = Wanted_GetValue();
                    Text_FillDollar(s, m->unk68);
                    gTalkLineText = Text_FillDollar2(m->quest->fee);
                    WinSlide_Step(&m->win, 0x280, 0x124);
                    m->state++;
                    LobbySe_Play(0x17);
                } else {
                    Talk_Say(6, 0);
                    gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), m->quest->fee);
                    m->state = 2;
                    LobbySe_Play(8);
                }
            } else {
                Talk_Say(5, 0);
                gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), m->quest->fee);
                m->state = 2;
                LobbySe_Play(8);
            }
            m->unk77 = 1;
        } else if (pad & 0x40) {
            m->quest = NULL;
            LobbySe_Play(0x17);
            return 3;
        } else {
            m->unk60 = Menu_WrapUpDownSe(m->unk60, n, 0x17);
            m->unk64 = Menu_WrapLeftRight(m->unk64, 3);
        }
        break;
    case 1:
        switch (WinSlide_Step(&m->win, 0x280, 0x124)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 2:
        switch (Talk_Update()) {
        case 0:
            if (QuestDesk_IsWanted((u8 *) m->quest) == 1) {
                m->state++;
                WinSlide_Step(&m->win, 0x124, 0x280);
                LobbySe_Play(0x2F);
            } else {
                m->state = 0;
                LobbySe_Play(8);
                return 0;
            }
            break;
        case 3:
            if (QuestDesk_IsWanted((u8 *) m->quest) == 1) {
                m->state++;
                WinSlide_Step(&m->win, 0x124, 0x280);
            } else {
                Talk_Say(0x10, 0);
                m->state = 4;
            }
            break;
        }
        break;
    case 3:
        switch (WinSlide_Step(&m->win, 0x124, 0x280)) {
        case 0:
            if (gTalkMode == 1) {
                m->state++;
                Talk_Say(0x10, 0);
            } else {
                m->state = 0;
                return 0;
            }
            break;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->state = 0;
            m->quest = NULL;
            break;
        }
        break;
    }
    return 2;
}

s32 QuestDesk_CheckFee(u16 pad)
{
    VMenu *m = (VMenu *)gMenuWork;

    switch (m->state) {
    case 0:
        if (Quest_CheckOrderTerms(m->quest->terms, 0) == 0) {
            m->state = 2;
            LobbyText_Open(0x64);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
        } else if (m->quest->fee + m->unk68 > gSave.money) {
            m->state++;
            Talk_Say(8, 0);
            Talk_Update();
        } else {
            return 0;
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case 0:
            m->state = 0;
            m->quest = NULL;
            return 3;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->state = 0;
            m->quest = NULL;
            LobbySe_Play(8);
            return 3;
        }
        break;
    }
    return 2;
}

s32 QuestDesk_RoomSettings(u16 pad)
{
    VMenu *m = (VMenu *)gMenuWork;
    s32 old;

    switch (m->unk6C) {
    case 0:
        if (pad & 0x20) {
            if (m->unk60 == 3) {
                if (gRoomPassOn != 0 && gRoomPassword[0] == 0) {
                    Menu_PushNoticeOnce(2);
                } else {
                    m->unk6C = 0;
                    LobbySe_Play(8);
                    return 1;
                }
            } else if (m->unk60 == 2) {
                m->unk6C++;
                gFieldControl = 0;
                LobbySe_Play(8);
                LobbyMenu_Init();
                LobbyMenu_SetDraw(QuestDesk_Draw, QuestDesk_Draw2, 0, 0);
                LobbyMenu_KbdEntry(gRoomPassword);
            }
        } else if (pad & 0xC00) {
            if (m->unk60 == 1) {
                gRoomPassOn ^= 1;
                if (gRoomPassOn == 0) {
                    memset(gRoomPassword, 0, 9);
                }
                LobbySe_Play(1);
            } else if (m->unk60 == 0) {
                gQuestMinMembers = Menu_WrapLeftRight(gQuestMinMembers, 4);
            }
        } else {
            old = m->unk60;
            m->unk60 = Menu_WrapUpDown(old, 4);
            if ((s8)old != m->unk60 && m->unk60 == 2 && gRoomPassOn == 0) {
                m->unk60 += m->unk60 - (s8)old;
            }
        }
        break;
    case 1:
        if (LobbyMenu_KbdEntry(gRoomPassword) == 1) {
            m->unk6C = 0;
            LobbyMenu_Init();
            gFieldControl = 1;
        }
        break;
    }
    return 0;
}

/* Raw addresses kept here: the named form changes the code. */
s32 QuestDesk_Post(u16 pad)
{
    VMenu *m = (VMenu *)gMenuWork;

    switch (m->state) {
    case 0:
        if (QuestDesk_RoomSettings(pad) == 1) {
            m->state++;
            LobbyText_Open(0x78);
            m->req.quest = m->quest->id;
            m->req.money = m->unk68;
            m->req.desk = 0;
            m->req.area = gScene.area;
        }
        *(s8 *)&gFieldControl = 0;
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (Quest_PostStep(&m->req)) {
        case 0:
            m->state = 0;
            *(s8 *)&gFieldControl = 0;
            return 0;
        case 1:
            m->state = 0;
            m->quest = NULL;
            *(s8 *)&gFieldControl = 0;
            return 1;
        }
        break;
    }
    *(s8 *)&gFieldControl = 0;
    return 2;
}

s32 QuestDesk_Cancel(s8 *st, u16 pad)
{
    switch (st[0]) {
    case 0:
        switch (Talk_Update()) {
        case 0:
            if (gLobby->questPaid != 0 && Game_IsOnline() == 1) {
                st[0]++;
                Talk_Say(0xD, 1);
                break;
            }
            if (Game_IsOnline() == 0) {
                st[0] = 3;
                Talk_Say(0xE, 0);
                CamFocus_SetAside(0);
                st[1] = 1;
            } else {
                st[0] = 2;
                LobbyText_Open(0x77);
            }
            break;
        case 3:
            st[0] = 0;
            CamFocus_SetAside(0);
            st[1] = 1;
            return 3;
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case 0:
            st[0]++;
            LobbyText_Open(0x77);
            break;
        case 3:
            st[0] = 0;
            return 3;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (LobbyStep_6501() == 1) {
            Talk_Say(0xE, 0);
            st[1] = 1;
            st[0]++;
            CamFocus_SetAside(0);
        }
        break;
    case 3:
        switch (Talk_Update()) {
        case 0:
            st[0] = 0;
            QuestDesk_Drop();
            return 0;
        }
        break;
    }
    return 2;
}

s32 QuestDesk_WaitBack(u16 pad)
{
    if (pad & 0x40) {
        LobbySe_Play(0x17);
        return 0;
    }
    return 2;
}

s32 QuestDesk_ScrollInfo(u16 pad)
{
    VMenu *m = (VMenu *)gMenuWork;

    if (pad & 0x40) {
        LobbySe_Play(0x17);
        return 0;
    }
    m->unk60 = Menu_WrapUpDownSe(m->unk60, m->unk62, 0x17);
    return 2;
}
