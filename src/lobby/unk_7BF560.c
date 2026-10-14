#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "pad.h"
#include "lobby_menu.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

s32 LobbyNet_GetErrText(char *);
extern u16 gLobbyLevel0Count;
s32 NetList0_GetCount(u16 *n);
s32 NetList0_GetByte(u16 id, u8 *out);
s32 NetList0_GetUsers(u16 id, u16 *a, u16 *b);
s32 NetList0_GetName(u16 id, char *dst);
s32 NetList0_GetText(u16 id, char *dst);
s32 NetList0_GetMaxUsers(u16 id, u16 *out);
s32 NetList0_GetValue10(u16 id, u16 *a);
s32 NetList0_GetKind(u16 id, u8 *out);

void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
void Lobby_Disconnect(s32);
s32 NetTask2_Start(s32 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *));
void LobbyStep_Level0ListDone(NetMsg m, NetMsg *pm);

/* unit's own declaration: with the definition's u32 (s32 arg0) (lobby/unk_7C8FA0.c), LevelList_SlotColor: 13 of 13 words differ */
u32 LobbyLogin_LitColor(u8 slot);
extern u8 gMenuWork[];

void LobbyTitle_Set(s32);
void ChatLog_Clear(void);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void LoginWave_Clear(void);
void LoginWave_Mark(s32);
u8 SeqA_GetState(void);
void SeqA_Start(s32);
void Bgm_PlayWithVolume(s32, s32);
typedef struct TextBox { u8 pad[0x18]; s32 size; } TextBox;
void LevelList_DrawInfo(TextBox *t);
void LevelList_DrawWindow(TextBox *t);
void LevelList_DrawText(void);
void Text_SetSize(s32 size);
void Font_SetSpacing(u8 a, u8 b);
void LobbyLogin_DrawMenu(void);
void LobbyLoc_DrawPanel(s16 x, s16 y);
void LobbySrv_DrawNamePanel(s16 x, s16 y);
void LevelList_DrawEntries(s16 x, s16 y);
void LevelList_DrawHelp(void);
void LobbyMsg_Draw(void);
s32 Lobby_StrLength(char *);
void Gfx_DrawPulseGradient(s16 x, s16 y, s16 w, s16 h, u32 color, u8 dir);
s16 Lobby_DrawSprite155(s16, s16, s32);
void Font_SetPos(s16, s16);
void Text_SetColor(s16 color);
void Text_PrintfColored(char *s, ...);
extern char *D_0080C0CC;               /* the help line's text */
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void LobbyUi_DrawIcon(s16 x, s16 y, s16 size, u32 color, s8 kind);
u32 LevelList_SlotColor(u8 slot);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void Menu_DrawCursorIcon(s16 x, s16 y, s16 w, u32 colour);
void Font_SetColor(s32 color);
extern char *D_0080C16C;
extern char *gAreaNameLabelTexts[];
extern char *gHeadcountTexts[];
extern LobbyMenu *gLobbyMenu;
extern u8 gLobbyLevel0Sel;
extern s8 gLobbyLevel1Sel;
extern s8 gLobbyLevel2Sel;

extern LevelEntry gLobbyLevel0Entries[];      /* level 0's entries (sub_main copies) */

void LevelList_Init(void);
s32 LobbyStep_Level0List(void);
void LevelList_Group(LevelList *m);
s32 LevelList_WaitAnim(void);
s32 LevelList_Input(void);
void LobbyHelp_SetText(void *p);
void LobbyHelp_SetBox(s16 x, s16 y, s16 w);
void LobbySe_Play(s32 se);
void LobbyText_Open(s32 msg);
u16 Rand_Next(s32 stream);
void QuitMenu_Open(void);
s32 LobbyLevel_SelId(s32 level);
s32 LobbyLogin_EnterEntry(s32 a, s32 b);
void LobbyVillage_Begin1(void);
void Stream_FadeOut(s32, s32);
void LobbyDialog_OpenWait(void *arg0);
u16 Pad_GetCursorButtons(s32 pad);
s32 QuitMenu_Update(u16 pad);
s32 LoginWave_Update(void);

/* the level-2 list screen's steps (LobbyState.phaseStep): set up, fetch the
 * entries (LobbyStep_Level0List) and group them, wait, the input (LevelList_Input),
 * the entering animation, entering the chosen entry (its error text: step
 * 8), the menu (6), and the two messages (7, 8) closed with pad 0x20 */
void LevelList_Run(void)
{
    LevelList *m = (LevelList *)gMenuWork;

    switch (gLobby->phaseStep) {
    case 0:
        gLobby->phaseStep++;
        gLobby->seqStep = 0;
        LevelList_Init();
        break;
    case 1:
        switch (LobbyStep_Level0List()) {
        case 0:
            gLobby->phaseStep++;
            gLobby->mode = 0;
            LobbyHelp_SetText((u8 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20) + 0x27C);
            LobbyHelp_SetBox(0x24, 0x16A, 0x154);
            LevelList_Group(m);
            LobbySe_Play(0x16);
            m->wait = 6;
            break;
        case 1:
            Lobby_Disconnect(5);
            break;
        }
        break;
    case 2:
        switch (LevelList_WaitAnim()) {
        case 0:
            gLobby->phaseStep++;
            m->wait = 0;
            break;
        }
        break;
    case 3:
        switch (LevelList_Input()) {
        case 0:
            gLobby->phaseStep++;
            m->shake = 5;
            m->unkE = Rand_Next(1);
            LobbySe_Play(0x18);
            LobbyText_Open(0x10);
            break;
        case 1:
            gLobbyMenu->redraw = 1;
            gLobby->phaseStep = 7;
            break;
        case 3:
            gLobby->phaseStep = 6;
            QuitMenu_Open();
            break;
        }
        break;
    case 4:
        switch (m->shake) {
        case 3:
            m->shakeX = -2;
            m->shakeY = 0;
            break;
        case 2:
            m->shakeX = 0;
            m->shakeY = 1;
            break;
        case 1:
            m->shakeX = 0;
            m->shakeY = 0;
            break;
        }
        if (--m->shake <= 0) {
            m->shakeX = 0;
            m->shakeY = 0;
            gLobby->phaseStep++;
        }
        break;
    case 5:
        switch (LobbyLogin_EnterEntry(LobbyLevel_SelId(0), 0)) {
        case 0:
            LobbyVillage_Begin1();
            LobbyMenu_SetDraw(0, 0, 0, 0);
            Stream_FadeOut(0, 0xF);
            break;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            gLobby->phaseStep = 8;
            gLobbyMenu->redraw = 1;
            break;
        }
        break;
    case 6:
        switch (QuitMenu_Update(Pad_GetCursorButtons(0) & ~0x8000)) {
        case 3:
            gLobby->phaseStep = 3;
            break;
        }
        break;
    case 7:
        gLobbyMenu->redraw = 1;
        if (gPadState[0].pressed & 0x20) {
            gLobby->phaseStep = 3;
            LobbySe_Play(8);
            if (gLobby->groupJoin != 0) {
                memset(gLobby, 0, 0x1C);
            }
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        if (gPadState[0].pressed & 0x20) {
            gLobby->phaseStep = 0;
            LobbyText_Open(0x1B);
            LobbySe_Play(8);
            if (gLobby->groupJoin != 0) {
                memset(gLobby, 0, 0x1C);
            }
        }
        break;
    default:
        Lobby_Disconnect(5);
        break;
    }
    if (gLobby->phaseStep != 3) {
        gLobbyMenu->redraw = 1;
    }
    LoginWave_Update();
}

void LevelList_Init(void)
{
    s32 i;

    if (SeqA_GetState() & 0xFF) {
        SeqA_Start(2);
    }
    ChatLog_Clear();
    gLobbyLevel0Sel = 0;
    gLobbyLevel1Sel = 0;
    gLobbyLevel2Sel = 0;
    memset(&gLobbyLevel0Entries, 0, 0x22C4);
    Bgm_PlayWithVolume(0x124, 0x1E);
    for (i = 0; i < 25; i++) {
        gLobbyLevel0Entries[i].state = 0;
    }
    LobbyMenu_Init();
    gLobby->connected = 1;
    gLobbyMenu->promptStep = 0;
    LobbyMenu_SetDraw(LevelList_DrawInfo, LevelList_DrawWindow, 0, 0);
    LobbyTitle_Set(3);
    gLobby->unk393F = 0;
    LoginWave_Clear();
    LoginWave_Mark(1);
}


/* groups the second level's entries by kind (a new group after 7 of one
 * kind), and finds the group and index of the entry the player is in
 * (LobbyState.unk14; none: its state cleared) */
void LevelList_Group(LevelList *m)
{
    s32 k;
    u16 cnt;
    s32 i;
    s32 *c;
    s32 j;
    s32 n;

    m->n = 0;
    memset(m->count, 0, sizeof(m->count));
    memset(m->g, 0, sizeof(m->g));
    cnt = gLobbyLevel0Count;
    for (i = 0; i < cnt; i++) {
        k = gLobbyLevel0Entries[i].kind - 1;
        c = &m->count[k];
        if (*c == 0 || *c == 7) {
            m->g[m->n].kind = k;
            m->n++;
        }
        (*c)++;
        m->g[m->n - 1].ids[m->g[m->n - 1].n] = gLobbyLevel0Entries[i].id;
        m->g[m->n - 1].n++;
    }
    if (gLobby->groupJoin != 0) {
        if (gLobby->area == 0) {
            gLobby->groupJoin = 0;
            memset(gLobby, 0, 0x1C);
            return;
        }
        for (j = 0; j < m->n; j++) {
            for (n = 0; n < m->g[j].n; n++) {
                if (m->g[j].ids[n] == gLobby->area) {
                    m->sel = j;
                    m->sub = n;
                }
            }
        }
    }
}

/* waits gMenuWork's counter (LevelList.wait) out, then done */
s32 LevelList_WaitAnim(void)
{
    LevelList *c = (LevelList *)gMenuWork;

    switch (gLobby->mode) {
    case 0:
        if (--c->wait < 0) {
            gLobby->mode++;
        }
        break;
    case 1:
        c->wait = 0;
        gLobby->mode = 0;
        return 0;
    }
    return 2;
}

u16 Pad_GetCursorButtons(s32 pad);
s32 Lobby_CanEnterByRank(s32 i, s32 again);
void LobbyText_Open(s32 msg);
void LobbySe_Play(s32 se);
void FriendList_Open(s32 arg);
s32 FriendList_Update(u16 pad);
s32 Pad_StepWrap(s32 cur, s32 n, u16 pad);
s32 Menu_WrapUpDownPad(s32 cur, s32 n, u16 pad);
void LobbyHelp_SetText(void *p);
void LobbyHelp_SetBox(s16 x, s16 y, s16 w);

/* the level-2 list's input: the entry the player is in, or the one chosen
 * (pad 0x20; rank checked: 1 refused, 0 a kind-3 entry, gLobbyLevel0Sel),
 * 0x40 back (3), 0x200 the menu (step 1), the cursor; 2 while it runs. */
s32 LevelList_Input(void)
{
    LevelList *m = (LevelList *)gMenuWork;
    s16 sel;
    u16 pad;
    u8 kind;
    s32 j;
    s32 i;
    s32 id;

    pad = Pad_GetCursorButtons(0);
    sel = m->sel;
    kind = m->g[sel].kind;
    switch (gLobby->mode) {
    case 0:
        if ((pad & 0x20) || gLobby->groupJoin != 0) {
            if (gLobby->groupJoin != 0) {
                for (i = 0; i < m->n; i++) {
                    for (j = 0; j < m->g[i].n; j++) {
                        if (m->g[i].ids[j] == gLobby->area) {
                            m->sel = i;
                            m->sub = j;
                        }
                    }
                }
                if (gLobby->land == 0) {
                    gLobby->groupJoin = 0;
                    memset(gLobby, 0, 0x1C);
                }
            }
            if (!Lobby_CanEnterByRank(m->g[m->sel].ids[m->sub], 0)) {
                LobbyText_Open(0xF);
                LobbySe_Play(9);
                return 1;
            }
            id = m->g[m->sel].ids[m->sub] - 1;
            if (gLobbyLevel0Entries[id].state == 3) {
                gLobbyLevel0Sel = id + 1;
                LobbySe_Play(8);
                return 0;
            }
            LobbyText_Open(0x14);
            LobbySe_Play(8);
            return 1;
        }
        if (pad & 0x40) {
            if (D_5C34F8 > 1) {
                LobbySe_Play(4);
                return 3;
            }
        } else if (pad & 0x200) {
            gLobby->mode++;
            FriendList_Open(0);
            LobbySe_Play(8);
        } else if (pad & 0xC00) {
            m->sel = Pad_StepWrap(sel, m->n, pad);
            if (sel != m->sel) {
                LobbySe_Play(0x3C);
                m->sub = 0;
            }
        } else {
            m->sub = Menu_WrapUpDownPad(m->sub, m->g[sel].n, pad);
        }
        if (kind != m->g[m->sel].kind) {
            LobbyHelp_SetText((u8 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20) + (m->g[m->sel].kind << 8) + 0x27C);
            LobbyHelp_SetBox(0x24, 0x16A, 0x154);
        }
        break;
    case 1:
        switch (FriendList_Update(pad)) {
        case 4:
            LobbySe_Play(4);
        case 3:
            gLobby->mode = 0;
        }
        if (gLobby->groupJoin != 0) {
            gLobby->mode = 0;
            for (i = 0; i < m->n; i++) {
                for (j = 0; j < m->g[i].n; j++) {
                    if (m->g[i].ids[j] == gLobby->area) {
                        m->sel = i;
                        m->sub = j;
                    }
                }
            }
            LobbyHelp_SetText((u8 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x20) + (m->g[m->sel].kind << 8) + 0x27C);
        }
        break;
    }
    return 2;
}

u32 LevelList_SlotColor(u8 slot)
{
    return (LobbyLogin_LitColor(slot - 1) & 0xFFFFFF) | 0x40000000;
}

/* the list's window at x, y: the chosen entry's icon (LobbyUi_DrawIcon), the
 * page (Menu_DrawPageNum), the headings, and a line per entry of the
 * player's group (its ids index sub_main 0x5C81D0's entries: the name at
 * +0x1F, the counts at +6 and +8), the cursor on the entry m->sub. The
 * positions x + 0x14 and y + 0x14 stay ints, narrowed at each call. */
void LevelList_DrawEntries(s16 x, s16 y)
{
    LevelList *m = (LevelList *)gMenuWork;
    LevelEntry *e;
    s32 ty;
    s32 tx2;
    u32 col;
    s16 blink;
    s32 i;
    s16 ly = y;

    Icon_Draw(x, y, 0x10E, 0x130, -1, 0);
    e = &gLobbyLevel0Entries[m->g[m->sel].ids[m->sub] - 1];
    col = LevelList_SlotColor(e->kind);
    LobbyUi_DrawIcon((s16)(x + 0x87), (s16)(y + 0x98), 0x70, col, 2);
    switch (gLobby->phaseStep) {
    case 4:
    case 5:
        Menu_DrawStamp(x, y, 0x10E, 0x130, (u16)m->unkE, m->shake);
        break;
    }
    ty = y + 0x14;
    tx2 = (s16)(x + 0x14) + 0xB4;
    Menu_DrawPageNum(tx2, ty, m->sel, m->n, 1);
    Font_SetPos(x + 0x14, ty);
    Text_PrintfColored("%s", D_0080C16C);
    Font_SetColor(0xFF202060);
    ty = y + 0x28;
    Font_SetPos(x + 0x14, ty);
    Text_PrintfColored("%s", gAreaNameLabelTexts[0]);
    Font_SetPos(tx2, ty);
    Text_PrintfColored(gHeadcountTexts[0]);
    Menu_DrawDashLine(x + 0x14, (s16)ty + 0x10, 0xFA, 0xFF000000);
    ly += 0x2C;
    Font_SetColor(0x80101010);
    for (i = 0; i < m->g[m->sel].n; i++) {
        e = &gLobbyLevel0Entries[m->g[m->sel].ids[i] - 1];
        ly += 0x14;
        Font_SetPos(x + 0x14, ly);
        if (i == m->sub) {
            if (gFrameTick & 0x10) {
                blink = 0;
            } else {
                blink = 6;
            }
            Icon_Draw(x + 0x14, ly + 0xE, 0xE2, 8, 0x99106060, 0x6F);
            Menu_DrawCursorIcon((s16)(x + 0x14) - 0x18 - blink, ly - 2, 0x18, 0xFF20C0C0);
        }
        Text_PrintfColored("%s", e->name);
        Font_SetPos((s16)(x + 0x14) + 0xAA, ly);
        Text_PrintfColored("%3d/%3d", e->users, e->maxUsers);
    }
}

/* draws the panel of LobbyLoc_DrawPanel (LobbyLoc_Text's text) sliding in with
 * the counter LevelList.wait (x = 0x14 - 0x30 * it, 0x1A once it is
 * out), unless the screen is closing */
void LevelList_DrawInfo(TextBox *t)
{
    s16 x;
    u8 s;

    Text_SetSize(t->size);
    Font_SetSpacing(0x12, 0x12);
    LobbyLogin_DrawMenu();
    if (((LevelList *)gMenuWork)->wait < 0) {
        x = 0x1A;
    } else {
        x = 0x14 - ((LevelList *)gMenuWork)->wait * 0x30;
    }
    s = gLobby->phaseStep;
    if (s == 3 && gLobby->mode == 1) {
        return;
    }
    if (s == 0 || s == 1) {
        return;
    }
    LobbyLoc_DrawPanel(x, 0x44);
}

/* the same for the panel of LobbySrv_DrawNamePanel (LobbySrv_MyName's text; x = 0x28
 * - 0x33 * the counter, 0x2E once it is out), then LevelList_DrawText */
void LevelList_DrawWindow(TextBox *t)
{
    LevelList *c = (LevelList *)gMenuWork;
    s16 x;
    u8 s;

    s = gLobby->phaseStep;
    if (s == 3 && gLobby->mode == 1) {
        return;
    }
    if (s == 0 || s == 1) {
        return;
    }
    Text_SetSize(t->size);
    Font_SetSpacing(0x12, 0x12);
    if (c->wait < 0) {
        x = 0x2E;
    } else {
        x = 0x28 - c->wait * 0x33;
    }
    LobbySrv_DrawNamePanel(x, 0x62);
    LevelList_DrawText();
}

/* the help line at the bottom: a bar (Gfx_DrawPulseGradient) from the text's left
 * edge (0x238 - 9 * its length, Lobby_StrLength) to the right of the screen,
 * a mark (Lobby_DrawSprite155) and the text D_0080C0CC */
void LevelList_DrawHelp(void)
{
    s16 x;
    f32 fx;

    x = 0x238 - Lobby_StrLength(D_0080C0CC) * 9;
    fx = x;
    Gfx_DrawPulseGradient(fx, 0x18E, 640.0f - fx, 0x16, 0x80006000, 1);
    Lobby_DrawSprite155(x + 4, 0x18E, 3);
    Font_SetPos(x + 0x26, 0x190);
    Font_SetSpacing(0x12, 0x12);
    Text_SetColor(0);
    Text_PrintfColored(D_0080C0CC);
}

/* the list's text (LevelList_DrawEntries) sliding in with the counter at
 * LevelList.wait (x = 0x1E - 0x32 * it, 0x24 once it is out, plus the
 * shake); the help line (LevelList_DrawHelp) while nothing is going on;
 * the message window (LobbyMsg_Draw) from mode 3 on */
void LevelList_DrawText(void)
{
    LevelList *c = (LevelList *)gMenuWork;
    s16 x;

    Font_SetSpacing(0x12, 0x12);
    if (c->wait < 0) {
        x = 0x24;
    } else {
        x = 0x1E - c->wait * 0x32;
    }
    LevelList_DrawEntries(x + c->shakeX, c->shakeY + 0x80);
    if (gLobby->groupJoin == 0 && gLobbyLevel0Sel == 0) {
        LevelList_DrawHelp();
    }
    if (gLobby->phaseStep >= 3) {
        LobbyMsg_Draw();
    }
}

/* task 2: level 0's list (flags 0x40F) */
s32 LobbyStep_Level0List(void)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        gLobby->seqStep = (s16)(seqStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0xC;
        NetTask2_Start(0, 0x40F, LobbyStep_Level0ListDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    case 99:
        gLobby->seqStep = 0;
        return 1;
    default:
        Lobby_Disconnect(5);
        break;
    }
    return 2;
}

/* task 2's end: level 0's entries into sub_main's copies (gLobbyLevel0Entries, 0x164 bytes each) */
void LobbyStep_Level0ListDone(NetMsg m, NetMsg *pm)
{
    u16 i;
    LevelEntry *e;
    s32 id;
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 0xC && m.kind != 2) {
            *s = 0;
            if (m.kind == 0) {
                gLobby->seqStep++;
                NetList0_GetCount(&gLobbyLevel0Count);
                i = 0;
                if (i < gLobbyLevel0Count) {
                    e = gLobbyLevel0Entries;
                    do {
                        id = i + 1;
                        e->id = id;
                        NetList0_GetByte(id, &e->state);
                        NetList0_GetUsers(id, &e->users, &e->questUsers);
                        NetList0_GetName(id, e->name);
                        NetList0_GetText(id, e->text);
                        NetList0_GetMaxUsers(id, &e->maxUsers);
                        NetList0_GetValue10(id, &e->unk14);
                        NetList0_GetKind(id, &e->kind);
                        i++;
                        e++;
                    } while (i < gLobbyLevel0Count);
                }
            } else {
                gLobby->seqStep = 99;
                LobbyNet_GetErrText(gLobby->errText);
            }
        }
    }
}
