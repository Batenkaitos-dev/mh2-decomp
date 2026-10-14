#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "quest.h"
#include "lobby_menu.h"
#pragma optimization_level 4
void Menu_DrawLabelBar(s16, s16, s16, u32, char *, u8, u8);

extern u8 gMenuWork[];

void LobbySe_Play(s32 se);
u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
s32 Win_Shake(WinShake *s);
s32 OfficialDesk_TopMenu(SlotMenu *m, u16 pad);
s32 OfficialDesk_ChooseQuest(SlotMenu *m, u16 pad);
s32 OfficialDesk_CheckFee(SlotMenu *m, u16 pad);
s32 OfficialDesk_Post(SlotMenu *m, u16 pad);
void OfficialDesk_Close(void);
void CamFocus_SetAside(s8 a);
void Talk_Say(s32 a, s8 b);
s32 Talk_Update(void);
u16 Rand_Next(s32 stream);
void QuestRoom_ResetSettings(void);
void QuestDesk_Accept(u16 id, s16 money, s32 pay);
extern u16 gQuestStampRand;
extern u16 gLobbyQuestId;
extern u8 gQuestIsOfficial;
extern s8 gQuestMinMembers;

void OfficialDesk_Update(void)
{
    SlotMenu *m = (SlotMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (m->state) {
    case 0:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 1:
        switch (OfficialDesk_TopMenu(m, pad)) {
        case 0:
            m->state++;
            WinSlide_Step(&m->win, 0x118, 0x280);
            LobbySe_Play(0x17);
            break;
        case 3:
            m->state = 9;
            CamFocus_SetAside(0);
            Talk_Say(1, 0);
            break;
        }
        break;
    case 2:
        switch (WinSlide_Step(&m->win, 0x118, 0x280)) {
        case 0:
            m->state++;
            break;
        }
        break;
    case 3:
        switch (OfficialDesk_ChooseQuest(m, pad)) {
        case 0:
            m->state++;
            gQuestStampRand = Rand_Next(1);
            break;
        case 3:
            m->state = 8;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 4:
        switch (OfficialDesk_CheckFee(m, pad)) {
        case 0:
            m->state++;
            m->menuCursor = 0;
            gLobbyQuestId = m->quest->id;
            QuestRoom_ResetSettings();
            break;
        case 3:
            m->state = 8;
            WinSlide_Step(&m->win, 0x280, 0x118);
            break;
        }
        break;
    case 5:
        switch (OfficialDesk_Post(m, pad)) {
        case 0:
            m->state++;
            Win_Shake(&m->shake);
            break;
        case 1:
            OfficialDesk_Close();
            break;
        }
        gFieldControl = 0;
        break;
    case 6:
        gFieldControl = 0;
        switch (Win_Shake(&m->shake)) {
        case 0:
            m->state++;
            if (gQuestMinMembers == 0) {
                Talk_Say(0xA, 0);
            } else {
                Talk_Say(0xB, 0);
            }
            QuestDesk_Accept(m->quest->id, 0, 1);
            gQuestIsOfficial = 1;
            gFieldControl = 1;
            break;
        }
        break;
    case 7:
        switch (Talk_Update()) {
        case 0:
            OfficialDesk_Close();
            break;
        }
        break;
    case 8:
        switch (WinSlide_Step(&m->win, 0x280, 0x118)) {
        case 0:
            m->state = 1;
            break;
        }
        break;
    case 9:
        switch (Talk_Update()) {
        case 0:
        case 3:
            OfficialDesk_Close();
            break;
        }
        break;
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_6C9640", D_00826148);

extern s8 gQuestMinMembers;
extern s8 gRoomPassOn;
extern s8 gRoomPassword[9];
s32 LobbyMenu_KbdEntry(void *opts);
s32 Menu_PushNoticeOnce(s32);
void LobbySe_Play(s32 se);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void OfficialDesk_Draw(void *task);
void OfficialDesk_Draw2(void *task);
void *memset(void *dst, s32 c, u32 n);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 OfficialDesk_RoomSettings(SlotMenu *m, u16 pad)
{
    s16 old;

    switch (m->unk82) {
    case 0:
        if (pad & 0x20) {
            if (m->menuCursor == 3) {
                if (gRoomPassOn != 0 && gRoomPassword[0] == 0) {
                    Menu_PushNoticeOnce(2);
                } else {
                    m->unk82 = 0;
                    LobbySe_Play(8);
                    return 1;
                }
            } else {
                if (m->menuCursor == 2) {
                    m->unk82++;
                    gFieldControl = 0;
                    LobbySe_Play(8);
                    LobbyMenu_Init();
                    LobbyMenu_SetDraw(OfficialDesk_Draw, OfficialDesk_Draw2, 0, 0);
                    LobbyMenu_KbdEntry(gRoomPassword);
                }
                if (m->menuCursor != 0 && m->menuCursor != 1) {
                    LobbySe_Play(8);
                }
            }
        } else if (pad & 0xC00) {
            if (m->menuCursor == 1) {
                gRoomPassOn ^= 1;
                if (gRoomPassOn == 0) {
                    memset(gRoomPassword, 0, 9);
                }
                LobbySe_Play(1);
            } else if (m->menuCursor == 0) {
                gQuestMinMembers = Menu_WrapLeftRight(gQuestMinMembers, 4);
                LobbySe_Play(1);
            }
        } else {
            old = m->menuCursor;
            m->menuCursor = Menu_WrapUpDown(old, 4);
            if ((s8)old != m->menuCursor && m->menuCursor == 2 && gRoomPassOn == 0) {
                m->menuCursor += m->menuCursor - (s8)old;
            }
        }
        break;
    case 1:
        if (LobbyMenu_KbdEntry(gRoomPassword) == 1) {
            m->unk82 = 0;
            LobbyMenu_Init();
            gFieldControl = 1;
        }
        break;
    }
    return 0;
}

extern LobbyMenu *gLobbyMenu;
s32 OfficialDesk_RoomSettings(SlotMenu *m, u16 pad);
void LobbyText_Open(s32);
s32 Quest_PostStep(Msg650B *req);
s32 OfficialDesk_Post(SlotMenu *m, u16 pad)
{
    switch (m->subStep) {
    case 0:
        if (OfficialDesk_RoomSettings(m, pad) == 1) {
            m->subStep++;
            LobbyText_Open(0x78);
            m->req.quest = m->quest->id;
            m->req.money = 0;
            m->req.desk = 1;
            m->req.area = gScene.area;
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (Quest_PostStep(&m->req)) {
        case 0:
            m->subStep = 0;
            return 0;
        case 1:
            m->subStep = 0;
            m->quest = NULL;
            return 1;
        }
        break;
    }
    return 2;
}

void Talk_Say(s32 a, s8 b);
s32 Talk_Update(void);
s32 Quest_CheckOrderTerms(u8 id, u8 flag);
void LobbyDialog_Grow(void);
s32 OfficialDesk_CheckFee(SlotMenu *m, u16 pad)
{
    switch (m->subStep) {
    case 0:
        if (gLobby->questTaken != 0) {
            m->subStep++;
            Talk_Say(0xF, 0);
            Talk_Update();
        } else if (Quest_CheckOrderTerms(m->quest->terms, 0) == 0) {
            m->subStep = 2;
            LobbyText_Open(0x64);
            LobbyDialog_Grow();
            gLobbyMenu->redraw = 1;
        } else if (m->quest->fee > gSave.money) {
            m->subStep++;
            Talk_Say(8, 0);
            Talk_Update();
        } else {
            return 0;
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case 0:
            m->subStep = 0;
            m->quest = NULL;
            return 3;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->subStep = 0;
            m->quest = NULL;
            LobbySe_Play(8);
            return 3;
        }
        break;
    }
    return 2;
}

static u16 *OfficialDesk_RowList(SlotMenu *m)
{
    switch (m->row[m->cursor]) {
    case 0:
        return m->list[0];
    case 1:
        return m->list[1];
    case 2:
    default:
        return m->list[2];
    }
}

extern u16 gTalkLine;
extern char *gTalkLineText;
QuestInfo *Quest_GetDiscInfo(u16 id);
char *Talk_LineText(u16 id);
char *Text_FillDollar(char *arg0, s32 arg1);
s32 Menu_WrapUpDownSe(s32 v, s32 a, M2C_UNK b);
u16 *OfficialDesk_RowList(SlotMenu *m);
s32 OfficialDesk_ChooseQuest(SlotMenu *m, u16 pad)
{
    u16 *list = OfficialDesk_RowList(m);

    switch (m->subStep) {
    case 0:
        if (pad & 0x20) {
            m->quest = Quest_GetDiscInfo(list[m->listCursor]);
            if (gLobby->questTaken != 0) {
                m->subStep = 3;
                m->quest = NULL;
                Talk_Say(0xF, 0);
            } else if (m->quest->fee != 0) {
                Talk_Say(6, 0);
                gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), m->quest->fee);
                m->subStep++;
            } else {
                Talk_Say(5, 0);
                gTalkLineText = Text_FillDollar(Talk_LineText(gTalkLine), m->quest->fee);
                m->subStep++;
            }
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            m->quest = NULL;
            LobbySe_Play(0x17);
            return 3;
        } else {
            m->listCursor = Menu_WrapUpDownSe(m->listCursor, m->unk7A, 0x17);
            m->unk7C = Menu_WrapLeftRight(m->unk7C, 3);
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case 0:
            m->subStep = 0;
            return 0;
        case 3:
            Talk_Say(0x10, 0);
            m->subStep++;
            break;
        }
        break;
    case 2:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->subStep = 0;
            m->quest = NULL;
            break;
        }
        break;
    case 3:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->subStep = 0;
            LobbySe_Play(4);
            break;
        }
        break;
    }
    return 2;
}

s32 OfficialDesk_TopMenu(SlotMenu *m, u16 pad)
{
    u16 *list;
    s32 i;

    switch (m->subStep) {
    case 0:
        if (pad & 0x20) {
            list = OfficialDesk_RowList(m);
            switch (m->row[m->cursor]) {
            case 1:
                Talk_Say(3, 0);
                break;
            default:
                Talk_Say(4, 0);
                break;
            case 3:
                LobbySe_Play(9);
                return 2;
            }
            m->subStep++;
            m->unk7C = 0;
            m->listCursor = 0;
            m->unk7A = 0;
            for (i = 0; i < 9; i++) {
                if (list[i] != 0) {
                    m->unk7A++;
                }
            }
            LobbySe_Play(8);
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            return 3;
        } else {
            m->cursor = Menu_WrapUpDown(m->cursor, 3);
        }
        break;
    case 1:
        switch (Talk_Update()) {
        case 0:
        case 3:
            m->subStep = 0;
            return 0;
        }
        break;
    }
    return 2;
}

extern char *gOfficialEventTexts[];
void Menu_DrawDecoFrame(s16 x, s16 y0, s16 w0, s16 h, s8 kind, s8 b);
void Font_SetColor(s32 color);
void Font_SetSpacing(u8 a, u8 b);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
void OfficialDesk_DrawRows(s16 x, s16 y)
{
    SlotMenu *m = (SlotMenu *)gMenuWork;
    char *s;
    s32 i;
    s16 cx;

    Menu_DrawDecoFrame(x, y, 0x140, 0x168, 4, -1);
    Font_SetColor(0x807F7F7F);
    x = x + 0xA0;
    y = y + 0x64;
    Font_SetSpacing(0x18, 0x18);
    for (i = 0; i < 3; i++) {
        if (i < 2) {
            s = gOfficialEventTexts[i];
        } else {
            s = gOfficialEventTexts[m->row[i]];
        }
        cx = Text_CenterX(s, x, 0x18);
        if (i == m->cursor) {
            Menu_DrawArrowsRev(cx, y, 0x18, s, 0xFFB57510);
            Text_DrawShadowRgb(cx, y, 0xFF101010, 0xFF106060, s);
        } else {
            Font_SetColor(0xFF101010);
            Font_SetPos(cx, y);
            Text_PrintfColored(s);
        }
        y += 0x46;
    }
}


char *Quest_GetText(u16 id, s32 k);
char *strcat(char *d, const char *s);
extern char D_00826148[];
extern char *gStarMarkTexts[];
extern char *D_0080CD54;
extern char *D_0080CD64;
extern char *D_0080CD8C;
extern char *gQuestKindNames[];

void Quest_DrawSummary(s16 x, s16 y, QuestInfo *q)
{
    s32 i;
    char buf[0x40];
    s16 y0 = y;
    s32 id = q->id;

    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(D_00826148, Quest_GetText(id, 0));
    Font_SetPos(x, y += 0x3C);
    Text_PrintfColored(D_00826148, Quest_GetText(id, 1));
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(D_00826148, Quest_GetText(id, 2));
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(D_00826148, Quest_GetText(id, 3));
    Font_SetPos(x + 0x5A, y += 0x1C);
    buf[0] = 0;
    for (i = 0; i < q->stars; i++) {
        strcat(buf, gStarMarkTexts[0]);
    }
    Text_PrintfColored(D_00826148, buf);
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(D_00826148, gQuestKindNames[q->terms]);
    Font_SetColor(0x80301010);
    x = x - 0xA;
    y = y0 + 0x14;
    y += 0x2A;
    Font_SetPos(x, y);
    Text_PrintfColored(D_00826148, D_0080CD8C);
    Font_SetPos(x, y += 0x60);
    Text_PrintfColored(D_00826148, D_0080CD64);
    Font_SetPos(x, y + 0x2A);
    Text_PrintfColored(D_00826148, D_0080CD54);
}

s32 sprintf(char *buf, const char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
extern char *D_0080C08C;
extern char *gContractFeeTexts[];
extern char *D_0080CD5C;
extern char *gClearCondTexts[];
extern char *D_0080CD6C;
extern char *gRewardLabelTexts[];

void Quest_DrawContract(s16 x, s16 y, QuestInfo *q)
{
    s16 y0 = y;
    s32 id = q->id;
    char buf[0x40];
    char tmp[0x40];

    Font_SetPos(x, y += 0x14);
    Text_PrintfColored(D_00826148, Quest_GetText(id, 0));
    Font_SetPos(x + 0x7E, y += 0x2A);
    sprintf(tmp, "%d%s", q->timeLimit / 60 / 30, D_0080C08C);
    Sjis_Widen(tmp, buf);
    Text_PrintfColored(D_00826148, buf);
    Font_SetPos(x + 0x7E, y += 0x18);
    sprintf(tmp, "%dz", q->fee);
    Sjis_Widen(tmp, buf);
    Text_PrintfColored(D_00826148, buf);
    Font_SetPos(x + 0x7E, y += 0x18);
    sprintf(tmp, "%dz", q->reward);
    Sjis_Widen(tmp, buf);
    Text_PrintfColored(D_00826148, buf);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(D_00826148, Quest_GetText(id, 4));
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(D_00826148, Quest_GetText(id, 5));
    Font_SetColor(0x80301010);
    x = x - 0xA;
    y = y0 + 0x14;
    y += 0x2A;
    Font_SetPos(x, y);
    Text_PrintfColored(D_00826148, D_0080CD5C);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(D_00826148, gContractFeeTexts[0]);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(D_00826148, gRewardLabelTexts[0]);
    Font_SetPos(x, y += 0x18);
    Text_PrintfColored(D_00826148, gClearCondTexts[0]);
    Font_SetPos(x, y + 0x3C);
    Text_PrintfColored(D_00826148, D_0080CD6C);
}

void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 a, s32 b);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl);
void Quest_DrawOverview(s16 x, s16 y, QuestInfo *q);
void Quest_DrawTerms(s16 x, s16 y, QuestInfo *q);
void Quest_DrawDetails(s16 x, s16 y, QuestInfo *q);
void Quest_DrawClearMark(s16 x, s16 y);
s32 Save_TestListBit(u16 id);
u16 Area_MapGroup(u16 id);
extern s8 gMapGroupMapIds[][2];
extern u16 gQuestStampRand;
extern char *D_0080CD24;

void OfficialDesk_DrawQuest(SlotMenu *m, s16 x, s16 y)
{
    u16 *list = OfficialDesk_RowList(m);
    QuestInfo *q = (QuestInfo *)Quest_GetDiscInfo(list[m->listCursor]);
    s16 offs[9];
    s32 i;

    if (m->quest != NULL) {
        q = m->quest;
    }
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x118, 0x12, 0xB4, 0xFFB0B000, D_0080CD24, 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    memset(offs, 0, sizeof(offs));
    for (i = 0; i < 9; i++) {
        if (list[i] == 0) {
            break;
        }
        offs[i] = Area_MapGroup(((QuestInfo *)Quest_GetDiscInfo(list[i]))->map);
        if (i == m->listCursor) {
            offs[i] = -offs[i];
        }
    }
    Menu_DrawQuestSheet(x, y, 0x140, 0x168, offs, m->listCursor, gMapGroupMapIds[Area_MapGroup(q->map)][0], -1);
    Font_SetColor(0x80101010);
    if (m->state == 3 && m->subStep == 0) {
        Menu_DrawPageNum(0x21C, y + 0xE, m->unk7C, 3, 1);
    }
    Font_SetSpacing(0x12, 0x12);
    if (q->flags & 0x400000) {
        switch (m->unk7C) {
        case 0:
            Quest_DrawSummary((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, q);
            break;
        case 1:
            Quest_DrawContract((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, q);
            break;
        case 2:
            Quest_DrawDetails((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, q);
            break;
        }
    } else {
        switch (m->unk7C) {
        case 0:
            Quest_DrawOverview((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, q);
            break;
        case 1:
            Quest_DrawTerms((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, q);
            break;
        case 2:
            Quest_DrawDetails((s16)(x + 0x28) + 10, (s16)(y + 0xE) + 4, q);
            break;
        }
    }
    if (Save_TestListBit(q->id)) {
        Quest_DrawClearMark(x + 0x28, y + 0xE);
    }
    if (m->quest != NULL) {
        switch (m->state) {
        case 6:
        case 7:
        case 9:
            Menu_DrawStamp(x + 0x28, y + 0xE, 0x140, 0x168, gQuestStampRand, m->shake.timer);
            break;
        }
    }
}

void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 a, s32 b);
void Menu_DrawArrows(s16 x, s16 y, s16 size, char *text, s32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
extern char *gMemberCountTexts[];
extern char *gNoneSetTexts[];
extern char *D_0080CD94;
extern char *gPlayerCountTexts[];
extern char *D_0080CD9C;
extern char *gCondDecideTexts[];

void OfficialDesk_DrawRoom(SlotMenu *m, s16 x, s16 y)
{
    s16 y0 = y;
    s32 cx;

    Menu_DrawQuestSheet(x, y, 0x140, 0x168, NULL, 1, -1, -1);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    y += 0x28;
    cx = x + 0xD8;
    Font_SetPos(cx, y += 0x2A);
    Text_PrintfColored(D_00826148, gMemberCountTexts[gQuestMinMembers]);
    if (m->menuCursor == 0) {
        Menu_DrawArrows(cx, y, 0x12, gMemberCountTexts[gQuestMinMembers], 0xFF106060);
    }
    Font_SetPos(cx, y += 0x2A);
    Text_PrintfColored(gNoneSetTexts[gRoomPassOn]);
    if (m->menuCursor == 1) {
        Menu_DrawArrows(cx, y, 0x12, gNoneSetTexts[gRoomPassOn], 0xFF106060);
    }
    Font_SetPos(Text_CenterX((char *)gRoomPassword, 0x1B8, 0x12), y += 0x2A);
    Text_PrintfColored(D_00826148, gRoomPassword);
    if (m->menuCursor == 2) {
        Menu_DrawArrowsRev(x + 0x64, y, 0x12, "             ", 0xFF106060);
        Gfx_DrawRectFrame(x + 0x64, y - 1, 0x78, 0x14, 0xFF106060);
        Gfx_DrawRectFrame(x + 0x63, y - 2, 0x7A, 0x16, 0xFF106060);
    } else {
        Gfx_DrawRectFrame(x + 0x64, y - 1, 0x78, 0x14, 0xFF606060);
        Gfx_DrawRectFrame(x + 0x63, y - 2, 0x7A, 0x16, 0xFF606060);
    }
    Font_SetColor(0x80301010);
    x = x + 0x28;
    y = y0 + 0x28;
    Font_SetPos(Text_CenterX(D_0080CD94, 0x1B8, 0x12), y);
    Text_PrintfColored(D_00826148, D_0080CD94);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(D_00826148, gPlayerCountTexts[0]);
    Font_SetPos(x, y += 0x2A);
    Text_PrintfColored(D_00826148, D_0080CD9C);
    x = Text_CenterX(gCondDecideTexts[0], 0x1B8, 0x12);
    Font_SetPos(x, y += 0x78);
    Text_PrintfColored(D_00826148, gCondDecideTexts[0]);
    if (m->menuCursor == 3) {
        Menu_DrawArrowsRev(x, y, 0x12, gCondDecideTexts[0], 0xFF106060);
    }
}

void Text_SetSize(s32 a);
void Menu_DrawDecoBox(s16 x, s16 y, s16 w, s16 h, s8 kind);
void OfficialDesk_DrawQuest(SlotMenu *m, s16 x, s16 y);
void OfficialDesk_DrawRoom(SlotMenu *m, s16 x, s16 y);
void OfficialDesk_Draw(void *task)
{
    SlotMenu *m = (SlotMenu *)gMenuWork;

    Text_SetSize(*(s32 *)((u8 *)task + 0x18));
    switch (m->state) {
    case 0:
    case 1:
        Menu_DrawDecoBox(m->win.pos, 0x3A, 0x140, 0x168, 4);
        break;
    case 2:
    case 3:
    case 4:
    case 6:
    case 7:
    case 8:
    default:
        OfficialDesk_DrawQuest(m, m->shake.dx + 0x118, m->shake.dy + 0x3A);
        break;
    case 5:
        if (m->subStep == 0) {
            OfficialDesk_DrawRoom(m, 0x118, 0x3A);
        }
        break;
    case 9:
        break;
    }
}

void OfficialDesk_DrawRows(s16 x, s16 y);
void OfficialDesk_Draw2(void *task)
{
    SlotMenu *m = (SlotMenu *)gMenuWork;

    Text_SetSize(*(s32 *)((u8 *)task + 0x18));
    switch (m->state) {
    case 0:
    case 1:
    case 2:
    case 8:
        OfficialDesk_DrawRows(m->win.pos, 0x3A);
        break;
    }
}

typedef struct DojoMenu {
    /* 0x00 */ WinSlide win;
    /* 0x08 */ u8 unk8[0x5C];
    /* 0x64 */ s32 task;
    /* 0x68 */ s32 unk68;
    /* 0x6C */ s32 unk6C;
    /* 0x70 */ u16 list[12];
    /* 0x88 */ u16 rows[5];
    /* 0x92 */ s16 row;
    /* 0x94 */ s16 cat;
    /* 0x96 */ s16 unk96;
    /* 0x98 */ s16 cats;
    /* 0x9A */ s16 nrows;
    /* 0x9C */ u8 unk9C[0x12];
    /* 0xAE */ s8 unkAE;
} DojoMenu;
extern s32 gTownMode;
extern s32 gTownMenuOpen;
void DojoMenu_DrawPanels(void *task);
void DojoMenu_Draw(void *task);
s32 LobbyWorld_GetSeason(void);
s32 LobbyTime_IsPm(void);
s32 Game_IsOnline(void);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
void CamFocus_SetAside(s8 a);
s32 Cond_Check(u16 id);
struct OfferTask;
s16 OfferMenu_BuildList(struct OfferTask *t, s32 b);
void DojoMenu_Open(u8 *task)
{
    DojoMenu *m = (DojoMenu *)gMenuWork;

    gTownMenuOpen = 1;
    gTownMode = 0x3F;
    memset(m, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(DojoMenu_DrawPanels, DojoMenu_Draw, 0, 0);
    Talk_Say(0, 0);
    m->unk68 = LobbyWorld_GetSeason();
    m->unk6C = LobbyTime_IsPm();
    m->task = (s32) task;
    if (Game_IsOnline() == 1) {
        m->unkAE = 2;
        m->cat = 0;
        m->rows[0] = 0;
        WinSlide_Step(&m->win, 0x280, 0x104);
        CamFocus_SetAside(1);
        LobbySe_Play(0x16);
    } else {
        m->nrows = 0;
        if (Cond_Check(10) == 1) {
            m->rows[m->nrows] = 0;
            m->nrows++;
        }
        m->rows[m->nrows] = 1;
        m->nrows++;
        if (OfferMenu_BuildList((struct OfferTask *) task, 0) != 0) {
            m->rows[m->nrows] = 2;
            m->nrows++;
        }
    }
}
