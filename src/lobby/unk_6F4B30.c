#include "common.h"
#include "lobby_state.h"
#include "scene.h"
#include "lobby_globals.h"
#include "lobby_net.h"

/* Built at optimization level 4: MoveMenu_DrawAreas was 19 words off at 4
 * (before its FAKEMATCH form) against 80 at 3; the rest match at both. */
#pragma optimization_level 4

typedef struct SprDef {
    u8 unk0[4];
    s16 u0;
    s16 v0;
    s16 u1;
    s16 v1;
} SprDef;
typedef struct IdGroup {
    s8 count;
    s8 kind;
    u8 pad2[2];
    s32 ids[7];
} IdGroup;
typedef struct EntryMenu {
    s16 row;
    s16 nrows;
    s16 col;
    u16 area;           /* 0x06: the area it was opened in (AreaMenu_Open) */
    s16 t8;
    s16 tA;
    s16 tC;
    s16 tE;
    u8 pad10[0x10];
    IdGroup groups[20];
    s8 unk2A0;
    s8 state;
    s16 unk2A2;
    s16 timer;
    u8 pad2A6[2];
    s16 yesNo;          /* 0x2A8: the yes/no cursor (Menu_WrapUpDown, Menu_SetYesNo) */
} EntryMenu;
extern SprDef gIconSprites[];
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Text_SetSize(s32);
void Font_SetSpacing(u8 a, u8 b);
extern u8 gMenuWork;
void LobbyLoc_DrawPanel(s32, s32); /* unit's own declaration: the K&R definition's narrow parameters arrive promoted, as this prototype says (a narrow prototype is incompatible with it) */
void Text_PrintfColored(const char *, ...);
char *LobbySrv_MyName(void);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
extern char D_00838298[];
void MoveMenu_DrawAreaPage(u8 *o);
void MoveMenu_DrawAreas(s16 x, s16 y);
void LobbyMsg_Draw(void);
s32 LobbyLoc_Has(void);
char *LobbyLoc_Text(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s16 gHouseCloseTimer;
s32 Menu_PushNoticeOnce(s32);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void LobbyMenu_Init(void);
void *memset(void *dst, s32 c, u32 n);
extern s8 gMenuInputLock;
extern u8 gLobbyLevel0Sel;
struct LevelList;
void LevelList_Group(struct LevelList *m);
void LobbySe_Play(s32 se);
void LobbyHelp_SetText(void *p);
void LobbyHelp_SetBox(s16 arg0, s16 arg1, s16 arg2);

void Stream_Stop(s32 ch);
s32 QuitMenu_Update(u16 pad);
void MoveMenu_Close(void);
void MoveMenu_OpenEntries(void);
s32 MoveMenu_EntryInput(u16 pad);
u16 LobbyPad_Get(s32 a);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 Menu_YesNoInput(void);
void Menu_SetYesNo(u8 a); /* unit's own declaration: with the definition's void (s8 choice) (lobby/unk_7B0D80.c), MoveMenu_Update 7 of 161 words differ */
void LobbyDialog_Grow(void);
void LobbyText_Open(s32);
void Lobby_Disconnect(s32 a);
void LobbyVillage_Begin2(void);
void SeqA_Start(s32 a);
extern LobbyMenu *gLobbyMenu;
/* unk_6F4B30's .rodata (0x838298-0x8382B0) */
#pragma rodata on
char D_00838298[0x8] = {
#include "lobby/00838298.inc"
};
/* MoveMenu_DrawAreas's row format (in its assembly file while it was
 * NON_MATCHING) */
char D_008382A0[0x10] = {
#include "lobby/008382A0.inc"
};
#pragma rodata off

void MoveMenu_Update(void)
{
    EntryMenu *m = (EntryMenu *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);
    s16 old;

    switch (m->unk2A0) {
    case 0:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            switch (m->yesNo) {
            case 1:
                LobbyText_Open(0x3F);
                LobbyDialog_Grow();
                Menu_SetYesNo(1);
                m->unk2A0 = 2;
                LobbySe_Play(8);
                break;
            case 0:
                m->unk2A0 = 1;
                MoveMenu_OpenEntries();
                break;
            default:
                MoveMenu_Close();
                LobbySe_Play(8);
                break;
            }
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            MoveMenu_Close();
        } else {
            old = m->yesNo;
            m->yesNo = Menu_WrapUpDown(old, 2);
            if (old != m->yesNo) {
                Menu_SetYesNo(m->yesNo);
            }
        }
        break;
    case 1:
        switch (MoveMenu_EntryInput(pad)) {
        case 0:
            LobbyVillage_Begin2();
            LobbyMenu_SetDraw(0, 0, 0, 0);
            break;
        case 3:
            LobbyText_Open(0x84);
            LobbyDialog_Grow();
            Menu_SetYesNo(m->yesNo);
            m->unk2A0 = 0;
            break;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case 0:
            Lobby_Disconnect(0);
            SeqA_Start(0xA);
            Stream_Stop(0);
            Stream_Stop(1);
            break;
        case 3:
            LobbyText_Open(0x84);
            LobbyDialog_Grow();
            Menu_SetYesNo(m->yesNo);
            m->unk2A0 = 0;
            break;
        }
        break;
    case 3:
        switch (QuitMenu_Update(pad)) {
        case 3:
            LobbyText_Open(0x84);
            LobbyDialog_Grow();
            Menu_SetYesNo(m->yesNo);
            m->unk2A0 = 0;
            break;
        }
        break;
    }
}

void MoveMenu_OpenEntries(void)
{
    EntryMenu *m = (EntryMenu *)&gMenuWork;
    s16 g;
    s16 i;

    gMenuInputLock = 0xA;
    LevelList_Group((struct LevelList *)m);
    m->unk2A2 = 0x35;
    m->timer = 6;
    LobbySe_Play(0x16);
    for (g = 0; g < m->nrows; g++) {
        for (i = 0; i < m->groups[g].count; i++) {
            if (m->groups[g].ids[i] == gLobbyLevel0Sel) {
                m->row = g;
                m->col = i;
            }
        }
    }
    LobbyHelp_SetText((u8 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20) + (m->groups[m->row].kind << 8) + 0x27C);
    LobbyHelp_SetBox(0x140, 0x16A, 0x140);
}

s32 Lobby_CanEnterByRank(s32 i, s32 again);
u16 Rand_Next(s32 stream);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
void SeqB_Start(s32);
u8 SeqB_GetState(void);
s32 LobbyLogin_EnterEntry(s32 a, s32 b);
void ChatLog_Clear(void);
void LobbyDialog_OpenWait(void *text);
s32 MoveMenu_EntryInput(u16 pad)
{
    EntryMenu *m = (EntryMenu *)&gMenuWork;
    s32 id = m->groups[m->row].ids[m->col];
    u8 kind = m->groups[m->row].kind;
    s16 old;

    switch (m->state) {
    case 0:
        if (--m->timer < 0) {
            m->state++;
        }
        if (m->state < 0 || m->state > 99) {} /* compiled-out check */
        break;
    case 1:
        m->timer = 0;
        if (pad & 0x20) {
            if (Lobby_CanEnterByRank(id, 0) == 0) {
                m->state = 99;
                LobbyDialog_Grow();
                LobbySe_Play(9);
            } else if (gLobbyLevel0Sel == id) {
                m->state = 99;
                LobbyText_Open(0x18);
                LobbyDialog_Grow();
                LobbySe_Play(9);
            } else {
                m->state++;
                gLobby->seqStep = 0;
                m->t8 = 5;
                m->tE = Rand_Next(1);
                LobbySe_Play(0x18);
                LobbyText_Open(0x11);
                LobbySe_Play(0);
            }
        } else if (pad & 0x40) {
            LobbySe_Play(4);
            m->state = 0;
            return 3;
        } else if (pad & 0xC00) {
            old = m->row;
            m->row = Menu_WrapLeftRight(old, m->nrows);
            if (old != m->row) {
                m->col = 0;
            }
        } else {
            m->col = Menu_WrapUpDown(m->col, m->groups[m->row].count);
        }
        if (kind != m->groups[m->row].kind) {
            LobbyHelp_SetText((u8 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20) + (m->groups[m->row].kind << 8) + 0x27C);
            LobbyHelp_SetBox(0x140, 0x16A, 0x140);
        }
        break;
    case 2:
        switch (m->t8) {
        case 3:
            m->tA = -2;
            m->tC = 0;
            break;
        case 2:
            m->tA = 0;
            m->tC = 1;
            break;
        case 1:
            m->tA = 0;
            m->tC = 0;
            break;
        }
        if (--m->t8 <= 0) {
            m->tA = 0;
            m->tC = 0;
            m->state++;
            SeqB_Start(1);
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        if (SeqB_GetState() != 1) {
            switch (LobbyLogin_EnterEntry(id, 0)) {
            case 0:
                m->state = 0;
                ChatLog_Clear();
                return 0;
            case 1:
                m->state = 99;
                LobbyDialog_OpenWait(gLobby->errText);
                LobbyDialog_Grow();
                SeqB_Start(2);
                break;
            }
        }
        break;
    case 99:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            LobbySe_Play(8);
            m->state = 1;
        }
        break;
    }
    return 2;
}

/* w: unused (LobbyLoc_DrawPanel passes the window width) */
void LobbyLoc_DrawDimIcon(s16 x, s16 y, s16 w)
{
    SprDef *s = &gIconSprites[0x1B];
    s16 sw = s->u1 - s->u0;
    s16 sh = s->v1 - s->v0;

    Icon_Draw(x + 0xC, y + 0x10, sw * 2, sh * 2, 0xA0987222, 0x1B);
}

/* K&R definition: MoveMenu_Draw calls it through the (s32, s32) declaration above and the
 * body uses x and y as s16 without narrowing them (an s16 prototype: MoveMenu_Draw differs;
 * s32 parameters narrowed in the body: this function differs). */
void LobbyLoc_DrawPanel(x, y)
s16 x;
s16 y;
{
    if (LobbyLoc_Has() == 1) {
        Icon_Draw(x, y, 0x10E, 0x130, -1, 1);
    } else {
        Icon_Draw(x, y, 0x10E, 0x130, 0xFF407287, 1);
        LobbyLoc_DrawDimIcon(x, y, 0x10E);
        return;
    }
    Font_SetColor(0x80101010);
    Font_SetPos(x + 0x14, y + 0xA);
    Text_PrintfColored(D_00838298, LobbyLoc_Text());
}

void LobbySrv_DrawNamePanel(s16 x, s16 y)
{
    Icon_Draw(x, y, 0x10E, 0x130, -1, 1);
    Font_SetColor(0x80101010);
    Font_SetPos(x + 0x14, y + 0xA);
    Text_PrintfColored(D_00838298, LobbySrv_MyName());
}

extern LevelEntry gLobbyLevel0Entries[];
u32 LevelList_SlotColor(u8 slot);
void LobbyUi_DrawIcon(s16 x, s16 y, s16 size, u32 color, s8 kind);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void Menu_DrawCursorIcon(s16 x, s16 y, s16 w, u32 color);
extern char *D_0080C16C;
extern char *gAreaNameLabelTexts[];
extern char *gHeadcountTexts[];

/* the entry list draw (the EntryMenu rows and their gLobbyLevel0Entries records).
 * FAKEMATCH: a second pointer w to gMenuWork for the row of the loop bound;
 * with m the loop reuses m->row * 32 from the bottom test where the original
 * reloads m->row at the loop top (permuter run 5 found a copy of m). */
void MoveMenu_DrawAreas(s16 x, s16 y)
{
    EntryMenu *m = (EntryMenu *)&gMenuWork;
    EntryMenu *w = (EntryMenu *)&gMenuWork;
    LevelEntry *r;
    s16 ty = y;
    s16 d;
    s16 i;
    u32 c;

    Icon_Draw(x, y, 0x10E, 0x130, -1, 0);
    r = &gLobbyLevel0Entries[m->groups[m->row].ids[0] - 1];
    c = LevelList_SlotColor(r->kind);
    LobbyUi_DrawIcon(x + 0x87, y + 0x98, 0x70, c, 2);
    if (m->unk2A0 == 1 && (m->state == 2 || m->state == 3)) {
        Menu_DrawStamp(x, y, 0x10E, 0x130, (u16)m->tE, m->t8);
    }
    x += 0x14;
    Menu_DrawPageNum(x + 0xB4, y + 0xA, m->row, m->nrows, 1);
    Font_SetColor(0x80101010);
    Font_SetPos(x, y + 0xA);
    Text_PrintfColored(D_00838298, D_0080C16C);
    Font_SetColor(0xFF202060);
    y += 0x1E;
    Font_SetPos(x, y);
    Text_PrintfColored(D_00838298, gAreaNameLabelTexts[0]);
    Font_SetPos(x + 0xB4, y);
    Text_PrintfColored(gHeadcountTexts[0]);
    Menu_DrawDashLine(x, y + 0x10, 0xFA, 0xFF000000);
    ty += 0x22;
    for (i = 0; i < m->groups[w->row].count; i++) {
        r = &gLobbyLevel0Entries[m->groups[m->row].ids[i] - 1];
        ty += 0x14;
        Font_SetPos(x, ty);
        if (i == m->col) {
            if (gFrameTick & 0x10) {
                d = 0;
            } else {
                d = 6;
            }
            Icon_Draw(x, ty + 0xE, 0xE2, 8, 0x99106060, 0x6F);
            Menu_DrawCursorIcon(x - 0x1A - d, ty - 2, 0x18, 0xFF20C0C0);
        }
        if (gLobbyLevel0Sel == m->groups[m->row].ids[i]) {
            Font_SetColor(0x80404040);
        } else {
            Font_SetColor(0x80101010);
        }
        Text_PrintfColored(D_00838298, r->name);
        Font_SetPos(x + 0xAA, ty);
        Text_PrintfColored(D_008382A0, r->users, r->maxUsers);
    }
}

void MoveMenu_Draw(u8 *o)
{
    EntryMenu *m = (EntryMenu *)&gMenuWork;
    s32 i;

    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    i = m->timer;
    switch (m->unk2A0) {
    case 1:
        LobbyLoc_DrawPanel(i < 0 ? 0x13A : (s16)(m->unk2A2 * i + 0x140), 0x36);
        break;
    }
}

void MoveMenu_Draw2(u8 *o)
{
    EntryMenu *m = (EntryMenu *)&gMenuWork;
    s16 x;

    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    if (m->timer < 0) {
        x = 0x14E;
    } else {
        x = m->unk2A2 * m->timer + 0x154;
    }
    switch (m->unk2A0) {
    case 1:
        LobbySrv_DrawNamePanel(x, 0x54);
        break;
    }
    MoveMenu_DrawAreaPage(o);
}

void MoveMenu_DrawAreaPage(u8 *o)
{
    EntryMenu *m = (EntryMenu *)&gMenuWork;
    s16 x;

    Text_SetSize(*(s32 *)(o + 0x18));
    Font_SetSpacing(0x12, 0x12);
    if (m->timer < 0) {
        x = 0x144;
    } else {
        x = m->unk2A2 * m->timer + 0x14A;
    }
    switch (m->unk2A0) {
    case 1:
        MoveMenu_DrawAreas(x + m->tA, m->tC + 0x72);
        break;
    }
    if (m->unk2A0 == 1 && m->state == 1) {
        LobbyMsg_Draw();
    }
}

void AreaMenu_Open(u16 id)
{
    EntryMenu *m = (EntryMenu *)&gMenuWork;
    u8 k;

    if (gLobby->questTaken == 1) {
        Menu_PushNoticeOnce(1);
        return;
    }
    if (gTownMode != 0x2B) {
        k = gScene.localPlayer;
        gLobby->memberSlot[k] = k + 0x30;
    }
    gHouseCloseTimer = 0;
    gTownMode = 0x3A;
    gTownMenuOpen = 1;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    memset(&gMenuWork, 0, 0xF00);
    LobbyMenu_Init();
    m->area = gScene.area;
    m->t8 = id;
}
