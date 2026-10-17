/* lobby 0x71F8A0-0x722360: the village's menus (save records, quest info).
 * One of the game's units, apart from the browser's name lists and page
 * draws that follow it (unk_722360.c): each has its own copy of "%s"
 * (D_00838C60 here, D_00838E38 there), the two share no data and no calls,
 * and Arena_DrawMonsterInfo needs level 4 where Page_DrawFocus and
 * Browser_DrawOverlay (there) need level 3. */
#include "common.h"
#include "lobby_text.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_vobj.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "quest.h"

/* Built at optimization level 4 (Arena_DrawMonsterInfo matches only there). */
#pragma optimization_level 4

void *memset(void *dst, s32 c, u32 n);
extern u8 gMenuWork[];
void *memcpy(void *dst, void *src, u32 n);
void Text_SetSize(s32 size);
void Font_SetSpacing(u8 a, u8 b);

u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
s32 Lobby_HasQuest(void);
void Talk_Say(s32 msg, s8 b);
s32 Talk_Update(void);
void CamFocus_SetAside(s8 a);
void LobbySe_Play(s32 se);
s32 Save_TestEventFlag(s32 flag);
s32 ArenaCup_ChooseQuest(u16 pad);
s32 ArenaCup_Post(u16 pad);
s32 ArenaCup_MonsterInput(u16 pad);
s32 QuestDesk_Cancel(s8 *p, u16 pad);
void ArenaCup_Close(void);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
void LobbyText_Open(s32);
u16 Rand_Next(s32 stream);
extern u16 gQuestStampRand;
#include "lobby_state.h"
#include "lobby_menu.h"
extern LobbyRecs gLobbyRecs;
extern LobbyMenu *gLobbyMenu;

void ArenaCup_Update(void)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (m->step) {
    case 0:
        if (Lobby_HasQuest() == 1) {
            m->step = 3;
            Talk_Say(0xC, 1);
            Talk_Update();
            CamFocus_SetAside(1);
            m->unk52 = 1;
            LobbySe_Play(0x17);
        } else if (Save_TestEventFlag(0xC4) == 1) {
            m->step++;
            LobbySe_Play(8);
        } else {
            m->step = 10;
            Talk_Say(0xF, 0);
            Talk_Update();
        }
        break;
    case 1:
        switch (ArenaCup_ChooseQuest(pad)) {
        case 0:
            m->monster = 0;
            m->cursor = 0;
            switch (m->choice) {
            case 0:
                m->step++;
                LobbyText_Open(0x78);
                CamFocus_SetAside(1);
                WinSlide_Step(&m->win, 0x280, 0x104);
                LobbySe_Play(8);
                break;
            case 1:
                m->step = 7;
                WinSlide_Step(&m->win, 0x280, 0x104);
                LobbySe_Play(0x17);
                CamFocus_SetAside(1);
                break;
            case 2:
                m->step = 5;
                WinSlide_Step(&m->win, 0x280, 0x140);
                CamFocus_SetAside(1);
                LobbySe_Play(0x17);
                break;
            }
            break;
        case 3:
            m->step = 4;
            Talk_Say(1, 0);
            break;
        case 1:
            break;
        }
        break;
    case 2:
        switch (ArenaCup_Post(pad)) {
        case 0:
            m->step = 4;
            Talk_Say(0xB, 0);
            gQuestStampRand = Rand_Next(1);
            break;
        case 3:
        case 1:
            m->step = 4;
            Talk_Say(0x11, 0);
            break;
        }
        break;
    case 3:
        switch (QuestDesk_Cancel(m->cancel, pad)) {
        case 0:
            ArenaCup_Close();
            break;
        case 3:
            if (Save_TestEventFlag(0xC4) == 1) {
                m->step = 1;
                CamFocus_SetAside(0);
            } else {
                m->step = 4;
                Talk_Say(1, 0);
                CamFocus_SetAside(0);
            }
            break;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
            ArenaCup_Close();
            break;
        }
        break;
    case 5:
        switch (WinSlide_Step(&m->win, 0x280, 0x140)) {
        case 0:
            m->step++;
            break;
        }
        break;
    case 6:
        if (pad & 0x40) {
            m->step = 1;
            CamFocus_SetAside(0);
            LobbySe_Play(4);
        }
        break;
    case 7:
        switch (WinSlide_Step(&m->win, 0x280, 0x104)) {
        case 0:
            m->step++;
            break;
        }
        break;
    case 8:
        switch (ArenaCup_MonsterInput(pad)) {
        case 0:
            m->step = 1;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case 9:
        switch (Talk_Update()) {
        case 0:
            m->step = 1;
            break;
        }
        break;
    case 10:
        switch (Talk_Update()) {
        case 0:
            m->step = 4;
            Talk_Say(1, 0);
            break;
        }
        break;
    case 11:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->step = 4;
            Talk_Say(1, 0);
            LobbySe_Play(8);
        }
        break;
    }
}

s32 LobbyLevel2_FetchList(void);
s32 LobbyLevel2_EnterFree(s32 v);
s32 LobbyLevel2_FetchRecords(void);
s32 LobbyLevel2_SendSettings(void);
s32 LobbyLevel2_Send6509(u32 v);
s32 LobbyLevel2_Send650B(Msg650B *d);
s32 LobbyStep_6501(void);
void LobbyDialog_OpenWait(void *text);
s32 ArenaCup_PostInput(u16 pad);
void QuestDesk_Accept(u16 id, s16 money, s32 pay);
extern u32 gQuestPostArg;
extern u16 gLobbyQuestId;
extern u8 D_6ADAD4[];

/* Raw addresses kept here: the named form changes the code. */
s32 ArenaCup_Post(u16 pad)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;

    switch (m->subStep) {
    case 0:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_FetchList()) {
        case 0:
            m->subStep++;
            break;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            m->subStep = 8;
            *(s8 *)&gFieldControl = 0;
            break;
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_EnterFree(1)) {
        case 0:
            m->subStep++;
            break;
        case 1:
            m->subStep = 8;
            *(s8 *)&gFieldControl = 0;
            break;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_FetchRecords()) {
        case 0:
            m->subStep++;
            LobbySe_Play(0x17);
            break;
        case 1:
            m->subStep = 8;
            *(s8 *)&gFieldControl = 0;
            break;
        }
        break;
    case 3:
        switch (WinSlide_Step(&m->win, 0x280, 0x104)) {
        case 0:
            m->subStep++;
            break;
        }
        break;
    case 4:
        switch (ArenaCup_PostInput(pad)) {
        case 0:
            m->subStep++;
            LobbyText_Open(0x78);
            m->req.quest = 0x61A9;
            m->req.money = 0;
            m->req.desk = 3;
            m->req.area = gScene.area;
            break;
        case 3:
            m->subStep = 9;
            LobbyText_Open(0x77);
            LobbySe_Play(4);
            break;
        }
        break;
    case 5:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_SendSettings()) {
        case 0:
            m->subStep++;
        case 1:
            break;
        }
        *(s8 *)&gFieldControl = 0;
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_Send650B(&m->req)) {
        case 0:
            m->subStep++;
            gQuestPostArg = (gQuestPostArg & 0xFFFF0000) | gLobbyQuestId;
            break;
        case 1:
            m->subStep = 8;
            *(s8 *)&gFieldControl = 0;
            break;
        }
        *(s8 *)&gFieldControl = 0;
        break;
    case 7:
        gLobbyMenu->redraw = 1;
        switch (LobbyLevel2_Send6509(gQuestPostArg)) {
        case 0:
            m->subStep = 0;
            gLobby->questPaid = 1;
            gPlayerText.card.arenaRole = 1;
            QuestDesk_Accept(0x61A9, 0, 1);
            memset(D_6ADAD4, 0, 0x5C);
            *(s8 *)&gFieldControl = 1;
            return 0;
        case 1:
            LobbyDialog_OpenWait(gLobby->errText);
            m->subStep = 8;
            *(s8 *)&gFieldControl = 0;
            break;
        }
        *(s8 *)&gFieldControl = 0;
        break;
    case 9:
        gLobbyMenu->redraw = 1;
        if (LobbyStep_6501() == 1) {
            if (gLobby->questPaid == 1) {
                gLobby->questPaid = 0;
            }
            m->subStep = 0;
            return 3;
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        if (pad & 0x20) {
            m->subStep = 0;
            LobbySe_Play(8);
            return 1;
        }
        break;
    }
    return 2;
}

QuestInfo *Quest_GetInfoAny(u16 id);
s32 Quest_CheckOrderTerms(u8 id, u8 flag);
s32 Menu_WrapUpDown(s32 cur, s32 n);
s32 Menu_PushNoticeOnce(s32);
void LobbyDialog_Grow(void);
s32 LobbyTime_IsPm(void);
s32 ArenaCup_ChooseQuest(u16 pad)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;
    QuestInfo *q = Quest_GetInfoAny(0x61A9);

    if (pad & 0x20) {
        switch (m->choice) {
        case 0:
            if (LobbyTime_IsPm() == 0) {
                Talk_Say(0x12, 0);
                m->step = 9;
                LobbySe_Play(8);
                return 1;
            }
            if (gLobby->questTaken != 0) {
                Menu_PushNoticeOnce(1);
                LobbySe_Play(9);
                break;
            }
            if (m->monsterCount == 0) {
                Talk_Say(8, 0);
                m->step = 9;
                LobbySe_Play(8);
                return 1;
            }
            if (Quest_CheckOrderTerms(q->terms, 0) == 0) {
                m->step = 0xB;
                LobbyText_Open(0x92);
                LobbyDialog_Grow();
                gLobbyMenu->redraw = 1;
                return 1;
            }
            return 0;
        default:
            return 0;
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else {
        m->choice = Menu_WrapUpDown(m->choice, 3);
    }
    return 2;
}

s32 ArenaCup_QuestTaken(void);
void Menu_SetYesNo(s8 a);
s32 Menu_YesNoInput(void);
void ArenaCup_ReleaseMonster(void);
s32 ArenaCup_RenameMonster(u16 pad, s32 arg1);

s32 ArenaCup_MonsterInput(u16 pad)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;
    s8 state = m->subStep;

    switch (state) {
    case 0:
        if (m->monsterCount == 0) {
            if (pad & 0x40) {
                LobbySe_Play(4);
                return 0;
            }
            break;
        }
        if (pad & 0x40) {
            LobbySe_Play(4);
            return 0;
        }
        if (pad & 0x20) {
            if (gSave.arenaEms[m->monster].emKind != 0) {
                m->subStep = state + 1;
                m->cursor = 0;
                WinSlide_Step(&m->entryWin, 0x280, 0x104);
                LobbySe_Play(0x17);
            }
        } else {
            m->monster = Menu_WrapUpDown(m->monster, m->monsterCount);
        }
        break;
    case 1:
        switch (WinSlide_Step(&m->entryWin, 0x280, 0x104)) {
        case 0:
            m->subStep = 3;
            break;
        }
        break;
    case 2:
        switch (WinSlide_Step(&m->entryWin, 0x104, 0x280)) {
        case 0:
            m->subStep = 0;
            break;
        }
        break;
    case 3:
        if (pad & 0x40) {
            WinSlide_Step(&m->entryWin, 0x104, 0x280);
            m->subStep = 2;
            LobbySe_Play(4);
        } else if (pad & 0x20) {
            switch (m->cursor) {
            case 0:
                m->subStep = 4;
                WinSlide_Step(&m->monsterWin, 0x280, 0x11C);
                LobbySe_Play(0x17);
                break;
            case 1:
                m->subStep = 7;
                LobbySe_Play(8);
                break;
            case 2:
                if (ArenaCup_QuestTaken() == 1) {
                    LobbySe_Play(9);
                    Menu_PushNoticeOnce(0x22);
                } else {
                    m->subStep = 8;
                    LobbyText_Open(0x8C);
                    LobbyDialog_Grow();
                    Menu_SetYesNo(1);
                    LobbySe_Play(8);
                    gFieldControl = 0;
                }
                break;
            }
        } else {
            m->cursor = Menu_WrapUpDown(m->cursor, 3);
        }
        break;
    case 4:
        switch (WinSlide_Step(&m->monsterWin, 0x280, 0x11C)) {
        case 0:
            m->subStep = 6;
            break;
        }
        break;
    case 5:
        switch (WinSlide_Step(&m->monsterWin, 0x11C, 0x280)) {
        case 0:
            m->subStep = 3;
            break;
        }
        break;
    case 6:
        if (pad & 0x40) {
            WinSlide_Step(&m->monsterWin, 0x11C, 0x280);
            m->subStep = 5;
            LobbySe_Play(4);
        }
        break;
    case 7:
        switch (ArenaCup_RenameMonster(pad, (s32)gSave.arenaEms[m->monster].name)) {
        case 0:
            m->subStep = 3;
            break;
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case 0:
            ArenaCup_ReleaseMonster();
            m->subStep = 0;
            if (m->monster >= m->monsterCount) {
                if (--m->monster < 0) {
                    m->monster = 0;
                }
            }
            gFieldControl = 1;
            break;
        case 3:
            m->subStep = 3;
            gFieldControl = 1;
            break;
        }
        break;
    }
    return 2;
}

u8 Kbd_UsbNewKey(void);
void Kbd_SetPos(s32 n, f32 f);
void Chat_OpenKbd(s32 flags, s32 b, u16 c, void *d);
s8 Kbd_RunFiltered(char *text, s16 held, s16 pressed);
void Kbd_Cancel(void);

s32 ArenaCup_RenameMonster(u16 pad, s32 name)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;

    Kbd_UsbNewKey();
    switch (m->renameStep) {
    case 0:
        (m->renameStep)++;
        Kbd_SetPos(0x3A, 80.0f);
        Chat_OpenKbd(0, 6, 8, (void *)name);
        gFieldControl = 0;
        break;
    case 1:
        if (Kbd_RunFiltered((char *)name, *(s16 *)&gPadState[0].held, *(s16 *)&gPadState[0].pressed) != 0) {
            (m->renameStep)++;
        }
        break;
    case 2:
        Kbd_Cancel();
        m->renameStep = 0;
        gFieldControl = 1;
        return 0;
    }
    return 2;
}

void ArenaCup_ReleaseMonster(void)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;

    memset(&gSave.arenaEms[m->monster], 0, 0x1C);
    switch (m->monster) {
    case 0:
        memcpy(&gSave.arenaEms[0], &gSave.arenaEms[1], 0x1C);
    case 1:
        memcpy(&gSave.arenaEms[1], &gSave.arenaEms[2], 0x1C);
        memset(&gSave.arenaEms[2], 0, 0x1C);
        break;
    }
    if (m->monsterCount > 0) {
        (m->monsterCount)--;
    }
}

s32 ArenaCup_IsKindOpen(s8 kind)
{
    u16 flags = *(u16 *)((s32)(gLobbyWorkBuf + 0x6EF4) + 0x11DE);

    switch (kind) {
    case 0:
    case 5:
        return 1;
    case 1:
        if (gLobbyRecs.usePass != 0) {
            return 1;
        }
        break;
    case 2:
        if (flags & 2) {
            return 1;
        }
        break;
    case 3:
        if (flags & 4) {
            return 1;
        }
        break;
    case 4:
        if (flags & 8) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 LobbyMenu_KbdEntry(void *p);
s32 Menu_WrapLeftRight(s32 cur, s32 n);
extern s8 gQuestMinMembers;
extern u8 D_008EA0A4[];
extern u8 gLobbyLevel2Sel;
s32 ArenaCup_PostInput(u16 pad)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;
    u8 *p = NULL;   /* also the retry count of the cursor loops (one register in the game) */

    switch (m->postStep) {
    case 0:
        if (pad & 0x20) {
            switch (m->cursor) {
            case 5:
                if (gLobbyRecs.usePass != 0 && gLobbyRecs.pass[0] == 0) {
                    Menu_PushNoticeOnce(2);
                    break;
                }
                if (ArenaCup_IsKindOpen(3) != 0) {
                    if (gLobbyRecs.rec[2].cur == 0) {
                        gLobbyRecs.rec[0].cur = 1;
                    } else {
                        gLobbyRecs.rec[0].cur = 3;
                    }
                } else {
                    gLobbyRecs.rec[0].cur = 1;
                }
                gQuestMinMembers = gLobbyRecs.rec[0].cur;
                *(s16 *)(D_008EA0A4 + gLobbyLevel2Sel * 0x164) = gQuestMinMembers;
                LobbySe_Play(8);
                return 0;
            case 1:
                m->postStep++;
                LobbySe_Play(8);
                break;
            }
        } else if (pad & 0x40) {
            return 3;
        } else if (pad & 0xC00) {
            switch (m->cursor) {
            case 0:
                gLobbyRecs.usePass ^= 1;
                if (gLobbyRecs.usePass == 0) {
                    memset(gLobbyRecs.pass, 0, 9);
                }
                LobbySe_Play(1);
                break;
            case 2:
                p = (u8 *)&gLobbyRecs.rec[1];
                break;
            case 3:
                p = (u8 *)&gLobbyRecs.rec[2];
                break;
            case 4:
                p = (u8 *)&gLobbyRecs.rec[3];
                break;
            }
            if (p != NULL) {
                p[0x45] = Menu_WrapLeftRight(p[0x45], p[0x42]);
            }
        } else if (pad & 0x1000) {
            do {
                if (++m->cursor >= 6) {
                    m->cursor = 0;
                }
                if (ArenaCup_IsKindOpen(m->cursor) != 0) {
                    break;
                }
                p++;
            } while ((s32)p < 6);
            LobbySe_Play(1);
        } else if (pad & 0x2000) {
            do {
                if (--m->cursor < 0) {
                    m->cursor = 5;
                }
                if (ArenaCup_IsKindOpen(m->cursor) != 0) {
                    break;
                }
                p++;
            } while ((s32)p < 6);
            LobbySe_Play(1);
        }
        break;
    case 1:
        if (LobbyMenu_KbdEntry(gLobbyRecs.pass) == 1) {
            m->postStep = 0;
        }
        break;
    }
    return 2;
}

void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 a);
void Menu_DrawArrows(s16 x, s16 y, s16 size, char *s, s32 color);
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *s, s32 color);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 col);
extern u8 D_0080C5EC[];
extern char *gArenaEntryTexts[];
void Text_PrintfColored(const char *fmt, ...);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
extern char *D_0080C5D4;
extern char *D_0080C074;
extern char *gNoneSetTexts[];
extern char *gCondDecideTexts[];
extern char D_00838C60[];
extern char D_00838C70[];

/* Round 29: matched with the column x in its own s16 col (declared before cx and tx)
 * and the row y set by the chained assignment ty = y0 = y + 0x18 (cy = y0 then reuses
 * ty's conversion, the game's move s4,s2). */
void ArenaCup_DrawForm(s16 x, s16 y)
{
    s16 ty;
    s32 sx;
    s32 y0;
    s16 cy;
    LobbyRec *p;
    s16 lx;
    s32 ax;
    ArenaCupMenu *m;
    s16 col;
    s32 cx;
    s32 tx;

    m = (ArenaCupMenu *)gMenuWork;
    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x104, 0x12, 0xE6, 0xFFB0B000, (char *)(*(s32 *)D_0080C5EC), 2, 1);
    Icon_Draw(x, y, 0x154, 0x10E, 0xFFE0D8C8, 0);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80301010);
    ty = y0 = y + 0x18;
    sx = x;
    cx = sx + 0xAA;
    Font_SetPos(Text_CenterX(gArenaEntryTexts[0], cx, 0x12), ty);
    Text_PrintfColored(gArenaEntryTexts[0]);
    cy = y0;
    col = sx + 0xAE;
    Font_SetColor(0x80301010);
    tx = sx + 0x24;
    Font_SetPos(tx, cy += 0x1E);
    Text_PrintfColored("%-10s%s", D_0080C5D4, D_0080C074);
    Font_SetColor(0x80101010);
    Font_SetPos(col, cy);
    Text_PrintfColored(gNoneSetTexts[gLobbyRecs.usePass]);
    if (m->cursor == 0) {
        Menu_DrawArrows(col, cy, 0x12, gNoneSetTexts[gLobbyRecs.usePass], 0xFF106060);
    }
    Font_SetPos(Text_CenterX(gLobbyRecs.pass, cx, 0x12), cy += 0x1E);
    Text_PrintfColored(D_00838C60, gLobbyRecs.pass);
    if (m->cursor == 1) {
        ax = sx + 0x64;
        Menu_DrawArrowsRev(ax, cy, 0x12, D_00838C70, 0xFF106060);
        Gfx_DrawRectFrame(ax, (s16)cy - 1, 0x8C, 0x14, 0xFF106060);
        Gfx_DrawRectFrame(sx + 0x63, (s16)cy - 2, 0x8E, 0x16, 0xFF106060);
    } else {
        Gfx_DrawRectFrame(sx + 0x64, (s16)cy - 1, 0x8C, 0x14, 0xFF606060);
        Gfx_DrawRectFrame(sx + 0x63, (s16)cy - 2, 0x8E, 0x16, 0xFF606060);
    }
    if (ArenaCup_IsKindOpen(2) != 0) {
        p = &gLobbyRecs.rec[1];
        Font_SetColor(0x80301010);
        Font_SetPos(tx, cy += 0x28);
        Text_PrintfColored("%-10s%s", p->name, D_0080C074);
        Font_SetColor(0x80101010);
        Font_SetPos(col, cy);
        Text_PrintfColored(p->subName[p->cur]);
        if (m->cursor == 2) {
            Menu_DrawArrows(col, cy, 0x12, p->subName[p->cur], 0xFF106060);
        }
    }
    if (ArenaCup_IsKindOpen(3) != 0) {
        p = &gLobbyRecs.rec[2];
        Font_SetColor(0x80301010);
        Font_SetPos(tx, cy += 0x1E);
        Text_PrintfColored("%-10s%s", p->name, D_0080C074);
        Font_SetColor(0x80101010);
        Font_SetPos(col, cy);
        Text_PrintfColored(p->subName[p->cur]);
        if (m->cursor == 3) {
            Menu_DrawArrows(col, cy, 0x12, p->subName[p->cur], 0xFF106060);
        }
    }
    if (ArenaCup_IsKindOpen(4) != 0) {
        p = &gLobbyRecs.rec[3];
        Font_SetColor(0x80301010);
        Font_SetPos(tx, cy += 0x1E);
        Text_PrintfColored("%-10s%s", p->name, D_0080C074);
        Font_SetColor(0x80101010);
        Font_SetPos(col, cy);
        Text_PrintfColored(p->subName[p->cur]);
        if (m->cursor == 4) {
            Menu_DrawArrows(col, cy, 0x12, p->subName[p->cur], 0xFF106060);
        }
    }
    Font_SetColor(0x80301010);
    lx = Text_CenterX(gCondDecideTexts[0], cx, 0x12);
    Font_SetPos(lx, cy += 0x32);
    Text_PrintfColored(D_00838C60, gCondDecideTexts[0]);
    if (m->cursor == 5) {
        Menu_DrawArrowsRev(lx, cy, 0x12, gCondDecideTexts[0], 0xFF106060);
    }
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_71F8A0", D_00838C60);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_71F8A0", D_00838C70);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_71F8A0", D_00838C80);

extern u8 D_0080C5F4[];
extern char *gMonsterRecordTexts[];
extern char *D_0080C5FC;
extern char *gWinCountTexts[];
extern char *D_0080C604;
extern char *gBattleTimeTexts[];
extern char D_00838C80[];
extern char D_00838C60[];
extern char gMyName[];
void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 a);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
char *Time_FormatMinSec(u32 t);
s32 sprintf(char *buf, char *fmt, ...);

/* The parameter y, dead after `cy = y`, is reused for the second column's x (the game keeps
 * both in s2), and the title bar's text is read as a word of D_0080C5F4 (with a char * the
 * constant 2 was scheduled before the colour). */
void ArenaCup_DrawRecord(s16 x, s16 y)
{
    s16 ty;
    char buf[0x30];
    s32 lx;
    s16 cy = y;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x140, 0x12, 0xE6, 0xFFB0B000, (char *)(*(s32 *)D_0080C5F4), 2, 1);
    Icon_Draw(x, y, 0x12C, 0xB4, 0xFFE0D8C8, 0);
    Font_SetSpacing(0x14, 0x14);
    Font_SetColor(0x80101010);
    lx = x;
    Font_SetPos(lx + 0x96, cy += 0x40);
    Text_PrintfColored(D_00838C80, gSave.versusWins + gSave.versusLosses);
    Font_SetPos(lx + 0x96, cy += 0x16);
    Text_PrintfColored(D_00838C80, gSave.versusWins);
    Font_SetPos(lx + 0x96, cy += 0x16);
    Text_PrintfColored(D_00838C80, gSave.versusLosses);
    Font_SetPos(lx + 0x96, cy + 0x16);
    Text_PrintfColored(D_00838C60, Time_FormatMinSec(gSave.versusMinutes));
    Font_SetColor(0x80301010);
    cy = y;
    ty = cy + 0x18;
    y = lx + 0x28;
    sprintf(buf, gMonsterRecordTexts[0], gMyName);
    Font_SetPos(Text_CenterX(buf, lx + 0x96, 0x14), ty);
    Text_PrintfColored(buf);
    Font_SetPos(y, cy += 0x40);
    Text_PrintfColored(D_0080C5FC);
    Font_SetPos(y, cy += 0x16);
    Text_PrintfColored(gWinCountTexts[0]);
    Font_SetPos(y, cy += 0x16);
    Text_PrintfColored(D_0080C604);
    Font_SetPos(y, cy + 0x16);
    Text_PrintfColored(gBattleTimeTexts[0]);
}

extern char *D_0080C60C;
extern char *gMonsterListTexts[];
extern char *D_0080C614;
extern char *D_0080C618;
extern char *D_0080C61C;
extern char *D_0080C624;
extern char *gArenaEmCondNames[];
extern char *gMonsterNames[];
extern char gMyName[];
extern char D_00838C60[];
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 a);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 color, u32 color2);

void SaveRecords_DrawPanel(s16 x, s16 y, s8 rec, s8 mode)
{
    ArenaEm *e;
    s16 sx;
    s16 ty;
    s16 i;

    Icon_Draw(x, y, 0x168, 0xA0, 0xFFE0D8C8, 0);
    Font_SetSpacing(0x12, 0x12);
    if (mode == 0) {
        Font_SetColor(0x80101050);
        Font_SetPos(Text_CenterX(D_0080C624, (s16)x + 0xB4, 0x12), (s16)y + 0x3E);
        Text_PrintfColored(D_0080C624);
        return;
    }
    Font_SetColor(0x80301010);
    ty = (s16)y + 0x14;
    sx = x + 0;
    Font_SetPos(Text_CenterX(D_0080C60C, sx + 0xB4, 0x12), ty);
    Text_PrintfColored(D_0080C60C, gMyName);
    ty += 0x1E;
    Font_SetColor(0x80101050);
    Font_SetPos(sx + 0x1E, ty);
    Text_PrintfColored(gMonsterListTexts[0]);
    Font_SetPos(sx + 0x5A, ty);
    Text_PrintfColored(D_0080C614);
    Font_SetPos(sx + 0xAA, ty);
    Text_PrintfColored(D_0080C618);
    Font_SetColor(0x80101010);
    for (i = 0; i < 3; i++) {
        ty += 0x16;
        e = &gSave.arenaEms[i];
        if (e->emKind != 0) {
            Font_SetPos(sx + 0x16, ty);
            Text_PrintfColored(gArenaEmCondNames[(s8)e->condition]);
            Font_SetPos(sx + 0x5A, ty);
            if ((s8)e->name[0] == 0) {
                Text_PrintfColored(D_0080C61C);
            } else {
                Text_PrintfColored(D_00838C60, e->name);
            }
            Font_SetPos(sx + 0xAA, ty);
            Text_PrintfColored(D_00838C60, gMonsterNames[e->emKind]);
        }
        if (rec == i && mode != 2) {
            Menu_DrawCursorRow(sx + 0xA, ty - 2, 0x12C, 0xA, 0xC0106060, 0xFF106060);
        }
    }
}

extern char *gMonsterStatusTexts[];
extern s32 D_0080C628;
extern char *gMonsterActionTexts[];
void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);

void ArenaCup_DrawMonsterCheck(s16 x, s16 y)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x104, 0x12, 0xE6, 0xFFB0B000, gMonsterStatusTexts[0], 2, 1);
    if (m->monsterCount == 0) {
        SaveRecords_DrawPanel(x, y, (s8)m->monster, 0);
        return;
    }
    if (m->subStep != 0) {
        SaveRecords_DrawPanel(x, y, (s8)m->monster, 2);
    } else {
        SaveRecords_DrawPanel(x, y, (s8)m->monster, 1);
    }
    switch (m->subStep) {
    case 0:
    case 2:
        Win_DrawText(0x104, 0x152, 0x168, 0x50, 0xBDB59F6C, 0x80202020, (char *) D_0080C628);
        break;
    case 1:
    case 3:
        Win_DrawText(0x104, 0x152, 0x168, 0x50, 0xBDB59F6C, 0x80202020, gMonsterActionTexts[0]);
        break;
    }
}

extern char *gArenaEntryTexts[];
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 a);
void Font_SetColor(s32 color);
void Font_SetPos(s16 x, s16 y);
void Text_PrintfColored(const char *fmt, ...);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 color, u32 color2);

void ArenaCup_DrawEntryForm(s16 x, s16 y)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;
    s16 ty = y;
    s16 tx;
    s16 i;
    s32 lx;

    Icon_Draw(x, y, 0x168, 0x64, 0xFFE0D8C8, 2);
    Font_SetSpacing(0x14, 0x14);
    Font_SetColor(0x80101010);
    ty += 0x14;
    tx = x + 0xB4;
    for (i = 0; i < 3; i++) {
        lx = Text_CenterX(gArenaEntryTexts[i + 23], tx, 0x14);
        Font_SetPos(lx, ty);
        Text_PrintfColored(gArenaEntryTexts[i + 23]);
        if (m->cursor == i) {
            Menu_DrawCursorRow(x + 0x64, ty - 2, 0xAA, 0xA, 0xC0106060, 0xFF106060);
        }
        ty += 0x16;
    }
}

extern char *D_0080C63C;
extern char *gArenaEmTameNames[];
extern char *gArenaEmRankNames[];
extern char D_00838C80[];
extern char *D_0080C074;
f32 Monster_SizeFromPercent(u8 kind, u16 x);
s8 SaveRecord_KindIndex_Unk(u8 *r);
char *Time_FormatMinSec(u32 t);

/* Its varargs float is converted with MWCC's fptodp (main 0x111A30). */
void Arena_DrawMonsterInfo(s16 x, s16 y, ArenaEm *r)
{
    s16 ty = y;
    s16 lx;
    s16 i;

    Font_SetSpacing(0x16, 0x16);
    Icon_Draw(x, y, 0x140, 0x10E, 0xFFE0D8C8, 1);
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x80101010);
    Font_SetPos(x + 0x82, ty += 0x3C);
    if ((s8)r->name[0] == 0) {
        Text_PrintfColored(D_0080C61C);
    } else {
        Text_PrintfColored(D_00838C60, r->name);
    }
    Font_SetPos(x + 0x82, ty += 0x14);
    Text_PrintfColored(D_00838C60, gMonsterNames[r->emKind]);
    Font_SetPos(x + 0x82, ty += 0x14);
    Text_PrintfColored("%6.1fcm", Monster_SizeFromPercent(r->emKind, r->sizePct));
    Font_SetPos(x + 0x82, ty += 0x14);
    Text_PrintfColored(gArenaEmTameNames[r->gaugeLevel]);
    Font_SetPos(x + 0x82, ty += 0x14);
    Text_PrintfColored(D_00838C60, gArenaEmCondNames[(s8)r->condition]);
    Font_SetPos(x + 0x82, ty += 0x14);
    Text_PrintfColored(D_00838C60, gArenaEmRankNames[SaveRecord_KindIndex_Unk((u8 *)r)]);
    Font_SetPos(x + 0x82, ty += 0x14);
    Text_PrintfColored(D_00838C60, Time_FormatMinSec(r->minutes));
    Font_SetPos(x + 0x82, ty += 0x14);
    Text_PrintfColored(D_00838C80, r->wins);
    Font_SetPos(x + 0x82, ty + 0x14);
    Text_PrintfColored(D_00838C80, r->losses);
    Font_SetColor(0x80301010);
    lx = x + 0x24;
    Font_SetPos(Text_CenterX(D_0080C63C, x + 0xA0, 0x14), y + 0x18);
    Text_PrintfColored(D_0080C63C);
    ty = y + 0x3C;
    for (i = 0x1C; i <= 0x24; i++) {
        Font_SetPos(lx, ty);
        Text_PrintfColored("%-8s%s", gArenaEntryTexts[i], D_0080C074);
        ty += 0x14;
    }
}

void Arena_DrawMonsterInfo(s16 x, s16 y, ArenaEm *r);
void ArenaCup_DrawMonster(s16 a, s16 b)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;

    Arena_DrawMonsterInfo(a, b, &gSave.arenaEms[m->monster]);
}

void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 h, s16 *offs, s16 sel, s8 kind, s32 b);
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 h, u16 seed, s16 lvl);
void Quest_DrawClearMark(s16 x, s16 y);
void Quest_DrawArenaTeams(s16 x, s16 y, NetUser *list, s8 room);
void Quest_DrawOverview(s16 x, s16 y, QuestInfo *q);
u16 Map_Group(u16 id);
s32 Save_IsQuestCleared(u16 id);
extern s8 gMapGroupMapIds[];

void Quest_DrawInfoPanel(ArenaCupMenu *m, s16 x, s16 y)
{
    QuestInfo *q = Quest_GetInfoAny(gLobbyQuestId);
    s16 offs[3];
    s8 c;

    if (m->cancel[1] != 1 && q != NULL) {
    Font_SetColor(0x80101010);
    Font_SetSpacing(0x12, 0x12);
    x += 0x28;
    y += 5;
    if (q->flags & 0x10) {
        Menu_DrawQuestSheet(x - 0x14, y, 0x140, 0x168, NULL, 0, -1, -1);
        Quest_DrawArenaTeams(x - 0x14, y + 6, gLobby->players, gLobbyLevel2Sel - 1);
    } else {
        memset(offs, 0, 6);
        c = gMapGroupMapIds[Map_Group(q->map) * 2];
        Menu_DrawQuestSheet(x - 0x28, y - 10, 0x140, 0x168, offs, 0, c, -1);
        Quest_DrawOverview(x, y, q);
    }
    if (Save_IsQuestCleared(q->id) != 0) {
        Quest_DrawClearMark(x, y);
    }
    Menu_DrawStamp(x, y, 0x140, 0x168, gQuestStampRand, 0);
    }
}

extern u8 D_0080C5EC[];
void Menu_DrawLabelList(s16 x, s8 **text, s16 n, s16 cur, u32 color);
void ArenaCup_DrawForm(s16 x, s16 y);
void ArenaCup_DrawRecord(s16 x, s16 y);
void ArenaCup_DrawMonsterCheck(s16 x, s16 y);
void ArenaCup_DrawEntryForm(s16 x, s16 y);
void Quest_DrawInfoPanel(ArenaCupMenu *m, s16 x, s16 y);

void ArenaCup_Draw(MenuTask *t)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;

    Text_SetSize(t->textSize);
    Font_SetSpacing(0x12, 0x12);
    switch (m->step) {
    case 0:
        break;
    case 1:
        Menu_DrawLabelList(0x190, (s8 **)D_0080C5EC, 3, m->choice, 0xFFB0B000);
        break;
    case 2:
        switch (m->subStep) {
        case 3:
        case 4:
            ArenaCup_DrawForm(m->win.pos, 0x3A);
            break;
        }
        break;
    case 5:
    case 6:
        ArenaCup_DrawRecord(m->win.pos, 0x3A);
        break;
    case 7:
    case 8:
        ArenaCup_DrawMonsterCheck(m->win.pos, 0x3A);
        switch (m->subStep) {
        case 0:
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            ArenaCup_DrawEntryForm(m->entryWin.pos, 0xE0);
            break;
        }
        break;
    case 3:
        Quest_DrawInfoPanel(m, 0x104, 0x30);
        break;
    }
}

void ArenaCup_Draw2(MenuTask *t)
{
    ArenaCupMenu *m = (ArenaCupMenu *)gMenuWork;

    Text_SetSize(t->textSize);
    Font_SetSpacing(0x12, 0x12);
    switch (m->step) {
    case 8:
        switch (m->subStep) {
        case 4:
        case 5:
        case 6:
            ArenaCup_DrawMonster(m->monsterWin.pos, 0x4E);
            break;
        }
        break;
    }
}
