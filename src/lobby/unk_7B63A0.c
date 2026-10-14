#include "common.h"
#include "lobby_menu.h"
#include "sprite.h"
#include "em.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "player.h"
void Font_SetSpacing(u8 a, u8 b);

void EquipPreview_Show(u8 on);
void StageModels_Free(void);
s32 LobbyReq_6501(void (*onReply)(NetMsg, NetMsg *));
void LobbyStep_6501Done(NetMsg m, NetMsg *pm);
s32 LobbyText_Update(void);
void HunterCard_Build(void *out);
extern NetUser gNetUsers[10];
u16 LobbyPad_Get(s32 a);
void Menu_SetYesNo(s8 a);
s32 Menu_YesNoInput(void);
void SeqA_Start(s32 a);
u8 SeqA_GetState(void);

extern MailEntry gShortMailMsg;
s32 LobbyReq_6704(void *id, char *s, void (*onReply)(NetMsg, NetMsg *));
void LobbySe_Play(s32 se);
void LobbyText_Open(s32 msg);
void LobbyDialog_Grow(void);
void LobbyStep_6704Done(NetMsg m, NetMsg *pm);
void *memcpy(void *dst, const void *src, u32 n);

typedef struct TextBox { u8 pad[0x18]; s32 size; } TextBox;
void Lobby_Disconnect(s32);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
s32 LobbyReq_6504(s32 arg0, void (*onReply)(NetMsg, NetMsg *));
void LobbyStep_6504LeaveDone(NetMsg m, NetMsg *pm);
extern s8 gIdMenuResult;
extern s8 gSrvMenuChoice;
extern void (*gIdMenuScreens[])(void);
extern void (*gSrvMenuPages[])(void);      /* the server menu's pages */
void IdMenu_Draw(void);
void *memset(void *dst, s32 c, u32 n);
char *strcpy(char *dst, const char *src);
s32 sprintf(char *dst, const char *fmt, ...);  /* sprintf */
void Sjis_Widen(char *src, char *dst);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
extern char *D_0080C07C;
extern char *D_0080D61C;
extern char *gMemberInfoTexts[];
void IdMenu_Redraw(TextBox *t);
void LobbyTitle_Set(s32 a);
void LobbyHelp_Set(s32 a);
s32 memcmp(void *a, void *b, s32 n);
void Text_SetSize(s32 size);
void LobbyDialog_Draw(void);
s16 LobbyDialog_GetGrow(void);
void LobbyMenu_DrawIdName(s16 y, char *id, char *name);
s32 LobbySrv_Index(char *s);
s32 LoginWave_Update(void);
void LobbyMenu_QueueLayers(void);

s32 LobbyNet_GetErrText(char *a);
void Money_PayPending(u8 silent);
void LobbyDialog_OpenWait(void *text);
void LobbyMenu_Init(void);
void LobbyScreen_ClearRequests(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
s32 Lobby_GetPlace3928(void);
void LobbyScreen_Close(void);
void LobbyRoom_BeginClose(u8 arg0);
void LobbyVillage_BeginLeave(void);
void LobbyTown_BeginLeave(void);
void CamFocus_End(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern LobbyMenu *gLobbyMenu;

/* leaves level 2 (6501), then clears the player's quest flags and resends
 * the text; 1 when done */
s32 LobbyStep_6501(void)
{
    u8 *st;
    LobbyState *p;
    u8 step;

    p = gLobby;
    st = &p->subStep;
    step = *st;
    switch (step) {
    case 0:
        *st = (u8)(step + 1);
        EquipPreview_Show(1);
        StageModels_Free();
        LobbyMenu_SetDraw(0, 0, NULL, 0);
    case 1:
        gLobby->subStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x2C;
        LobbyReq_6501(LobbyStep_6501Done);
        break;
    case 2:
        LobbyMenu_TickTimer();
        break;
    case 3:
        *st = 0;
        gPlayers[gScene.localPlayer].questStatus = 0;
        gPlayerText.card.arenaRole = 0;
        LobbyText_Update();
        HunterCard_Build(gLobby->members[gScene.localPlayer].text);
        HunterCard_Build(gNetUsers[gScene.localPlayer].text);
        return 1;
    }
    return 0;
}

/* 6504(0)'s reply (leaving a quest: the flags go) */
void LobbyStep_6501Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 0x2C) {
            *s = 0;
            if (m.kind == 0) {
                gLobby->questTaken = 0;
                gLobby->questPaid = 0;
                gLobby->subStep++;
            } else {
                gLobby->questTaken = 0;
                gLobby->questPaid = 0;
                Lobby_Disconnect(5);
            }
        }
    }
}

/* sends 6504 (0) */
s32 LobbyStep_6504Leave(void)
{
    u8 subStep;

    subStep = gLobby->subStep;
    switch (subStep) {
    case 0:
        gLobby->subStep = (u8)(subStep + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x30;
        LobbyReq_6504(0, LobbyStep_6504LeaveDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->subStep = 0;
        return 0;
    case 3:
        gLobby->subStep = 0;
        return 1;
    }
    return 2;
}

void LobbyStep_6504LeaveDone(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 0x30) {
            *s = 0;
            if (m.kind == 0) {
                gLobby->subStep++;
                gLobby->entered6504 = 0;
            } else {
                LobbyNet_GetErrText(gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobby->subStep = 3;
            }
        }
    }
}

s32 Lobby_HandleLeaveRequest(void)
{
    s8 temp_v1;
    u8 phase;
    u8 questTaken;

    if (gLobby->connected != 0) {
        if (gLobby->playersListed != 0) {
            LobbyScreen_Close();
            return 1;
        }
        temp_v1 = gLobby->unk3963;
        if (temp_v1 == 1) {
            LobbyRoom_BeginClose(0);
            LobbyDialog_OpenWait(gLobby->errText);
            return 1;
        }
        if (temp_v1 == 2) {
            LobbyRoom_BeginClose(1);
            return 1;
        }
        phase = gLobby->phase;
        if (phase == 2) {
            if (gLobby->leave6307 == 1) {
                LobbyVillage_BeginLeave();
                return 1;
            }
            goto block_23;
        }
        if ((phase == 3) && (Lobby_GetPlace3928() != 0)) {
            questTaken = gLobby->questTaken;
            if (questTaken == 0) {
                if (gLobby->leave640C == 1) {
                    if (questTaken != 0) {
                        Money_PayPending(1U);
                    }
                    LobbyTown_BeginLeave();
                    return 1;
                }
                goto block_23;
            }
            if (gLobby->leave6505 == 1) {
                if (gLobby->questPaid != 0) {
                    gLobby->questPaid = 0U;
                }
                Money_PayPending(1U);
                CamFocus_End();
                LobbyDialog_OpenWait(gLobby->errText);
                gFieldControl = 0;
                gTownMode = 0x16;
                gTownMenuOpen = 1;
                gLobby->awaitReply = 0;
                gLobby->subStep = 0;
                LobbyMenu_Init();
                LobbyMenu_SetDraw(0, 0, NULL, 0);
                LobbyScreen_ClearRequests();
                gLobbyMenu->redraw = 1;
                return 1;
            }
            goto block_23;
        }
        goto block_23;
    }
block_23:
    return 0;
}

/* sends 6704 (the mail msg: its addressee's id and text; msg is kept in gShortMailMsg,
 * sub_main 0x5C04A0) */
s32 LobbyStep_6704(MailEntry *msg)
{
    s16 seqStep;

    seqStep = gLobby->seqStep;
    switch (seqStep) {
    case 0:
        memcpy(&gShortMailMsg, msg, sizeof(MailEntry));
        LobbyMenu_StartTimer();
        gLobby->awaitReply = 0x34;
        LobbyReq_6704(msg->id, msg->text, LobbyStep_6704Done);
        gLobby->seqStep++;
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return 0;
    case 99:
        gLobby->seqStep = 0;
        LobbyDialog_OpenWait(gLobby->errText);
        return 1;
    default:
        Lobby_Disconnect(5);
        break;
    }
    return 2;
}

void LobbyStep_6704Done(NetMsg m, NetMsg *pm)
{
    u8 *s;
    u8 *p = (u8 *)gLobby;

    if (p[0x3936] != 5) {
        s = p + 0x3945;
        if (*s == 52) {
            *s = 0;
            if (m.kind == 0) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = 99;
                LobbyNet_GetErrText((char *)(u8 *)gLobby + 0x3608);
            }
        }
    }
}

s32 IdMenu_Run(void)
{
    gIdMenuResult = -1;
    gIdMenuScreens[gLobbyMenu->screen]();
    IdMenu_Draw();
    return gIdMenuResult;
}

void IdMenu_Start(void)
{
    s8 i;
    s32 off;

    LobbyMenu_SetDraw(0, 0, IdMenu_Redraw, 0);
    gLobbyMenu->screen++;
    LobbyTitle_Set(4);
    LobbyHelp_Set(1);
    gLobbyMenu->cursor = 0;
    for (i = 0, off = 0; i < 3; i++, off += 0x11) {
        if (memcmp(gLobby->charaNames + off, (u8 *)gSave.name, 0x10) == 0) {
            gLobbyMenu->cursor = i;
            return;
        }
    }
}

/* menu page 1 (gLobbyMenu's menu): up and down choose one of the three
 * texts (+0x35), circle goes on, cross asks whether to quit (message 0x51).
 * The step is reached through a pointer taken first (its address is numbered
 * before the gLobbyMenu load), and case 0 tests an s32 copy of the buttons
 * (the game re-extends them there: its second andi). */
void IdMenu_Choose(void)
{
    u16 pad;
    s32 b;
    u8 *sp;

    pad = LobbyPad_Get(0);
    sp = &gLobbyMenu->step;
    switch (*sp) {
    case 0:
        b = pad;
        if (b & 0x2000) {
            if (gLobbyMenu->cursor == 0) {
                gLobbyMenu->cursor = 2;
            } else {
                gLobbyMenu->cursor--;
            }
            LobbySe_Play(1);
        } else if (b & 0x1000) {
            if (++gLobbyMenu->cursor > 2) {
                gLobbyMenu->cursor = 0;
            }
            LobbySe_Play(1);
        } else if (b & 0x20) {
            *sp += 1;
            gLobbyMenu->timer = 0x28;
            LobbySe_Play(0);
        } else if (b & 0x40) {
            *sp = 2;
            LobbyText_Open(0x51);
            Menu_SetYesNo(1);
            gLobbyMenu->redraw = 1;
            LobbySe_Play(4);
        }
        break;
    case 1:
        if (gLobby->charaIds[gLobbyMenu->cursor][0] == 0) {
            LobbyText_Open(8);
        } else {
            LobbyText_Open(7);
        }
        LobbyDialog_Grow();
        gLobbyMenu->screen++;
        gLobbyMenu->step = 0;
        gLobbyMenu->unk30 = 0;
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (Menu_YesNoInput()) {
        case 0:
            gIdMenuResult = -2;
            break;
        case 3:
            gLobbyMenu->step = 0;
            break;
        }
        break;
    }
}

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
            case 0:
                SeqA_Start(1);
                gLobbyMenu->step++;
                break;
            case 3:
                gLobbyMenu->screen = 1;
                gLobbyMenu->step = 0;
                LobbyTitle_Set(4);
                LobbyHelp_Set(1);
                break;
            }
        }
        break;
    case 2:
        if ((u8)SeqA_GetState() != 1) {
            gIdMenuResult = 0;
        }
        break;
    }
}

/* a menu line: id idx of LobbyState.unk26 (D_0080C07C when it is empty) at
 * x + 0x3C after the label gMemberInfoTexts, highlighted (colour 4) on the
 * cursor's line */
void IdMenu_DrawId(s16 x, s16 y, u8 cur, u8 idx, u8 unused)
{
    char buf[0x1E];
    s8 c;
    LobbyState *p;

    memset(buf, 0, 0x1E);
    p = gLobby;
    if (p->charaIds[idx][0] == 0) {
        strcpy(buf, D_0080C07C);
    } else {
        Sjis_Widen(p->charaIds[idx], buf);
    }
    Font_SetSpacing(0x14, 0x14);
    c = (cur != idx) ? 0 : 4;
    Text_DrawShadow(x + 0x3C, y, 1, c, buf);
    Text_DrawShadow(x, y, 1, c, gMemberInfoTexts[0]);
}

/* the same for the 0x11-byte texts at LobbyState + 0x46 after the label
 * D_0080D61C (an empty one printed as D_0080C07C, in the small font). The
 * compare as ints narrows idx again (the index's narrowed copy isn't
 * reused) */
void IdMenu_DrawText(s16 x, s16 y, u8 cur, u8 idx, u8 unused)
{
    char buf[0x1E];
    s8 c;

    memset(buf, 0, 0x1E);
    if (((char *)gLobby->charaNames)[idx * 0x11] == 0) {
        Font_SetSpacing(0x14, 0x14);
        sprintf(buf, D_0080C07C);
    } else {
        Font_SetSpacing(0x18, 0x14);
        strcpy(buf, (char *)&gLobby->charaNames[idx * 0x11]);
    }
    c = ((s32)cur != (s32)idx) ? 0 : 4;
    Text_DrawShadow(x + 0x3C, y, 1, c, buf);
    Font_SetSpacing(0x14, 0x14);
    Text_DrawShadow(x, y, 1, c, D_0080D61C);
}

void Icon_DrawPreset(s32 a);
void Win_Draw(s32 x, s32 y, s32 w, s32 h, s32 color, s32 color2, s16 kind);
extern char *gCharaNameLabelTexts[];
u32 Color_Pulse(void);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i, u8 mode);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 col);
void LobbyMsg_Draw(void);
void LobbyTitle_Draw(void);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 a);
s32 Sprite_Draw(Sprite *s);
void Font_SetPos(s16 x, s16 y);
void Text_Printf(char *fmt, ...);

/* the id menu's screen: the player's id box, the three ids with the
 * cursor, the message window, and the error text with its icon */
void IdMenu_Draw(void)
{
    Sprite spr;
    s32 i;
    s16 y;

    LobbyPad_Get(0);
    Icon_DrawPreset(2);
    Font_SetSpacing(0x14, 0x14);
    Win_Draw(0x1E, 0x44, 0x10E, 0x42, 0xBDB59F6C, 0x80505000, 2);
    Text_DrawShadow(0x2A, 0x4F, 1, 5, gCharaNameLabelTexts[0]);
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
    LobbyMsg_Draw();
    LobbyTitle_Draw();
    if (gLobby->errText[0] != 0) {
        Icon_Draw(0x14A, 0x44, 0x122, 0x112, 0xFFC0C0C0, 0);
        Tex_Prepare(1, 0x147);
        Tex_Bind(0x147);
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

/* draws the menu's texts again when asked (unk39): the message screen and,
 * on screen 2 with no message box, the id under the cursor */
void IdMenu_Redraw(TextBox *t)
{
    Text_SetSize(t->size);
    if (gLobbyMenu->redraw != 0) {
        LobbyDialog_Draw();
        if (gLobbyMenu->screen == 2 && LobbyDialog_GetGrow() == -1) {
            if (gLobby->charaIds[gLobbyMenu->cursor][0] != 0) {
                LobbyMenu_DrawIdName(0x86, (char *) (u8 *)gSave.name, (char *) gLobby->charaIds[gLobbyMenu->cursor]);
            }
        }
        gLobbyMenu->redraw = 0;
    }
}

/* the server choice's result: the chosen server's index (LobbySrv_Index)
 * when LobbyState.unk0 is set, else runs the menu's page (SrvMenu_Start ...
 * SrvMenu_Quit, gSrvMenuPages[screen]) and returns its choice (gSrvMenuChoice, -1
 * while none). The pages take no parameter: the address of unk1 that the
 * original leaves in a0 is the if-branch's argument, scheduled into the
 * branch's delay slot. */
s32 SrvMenu_Run(void)
{
    LobbyState *p = gLobby;

    if (p->groupJoin != 0) {
        return LobbySrv_Index(p->groupId);
    }
    gSrvMenuChoice = -1;
    gSrvMenuPages[gLobbyMenu->screen]();
    LoginWave_Update();
    LobbyMenu_QueueLayers();
    return gSrvMenuChoice;
}

/* unk_7B63A0's .data (0x819450-0x819480) */
void IdMenu_Choose(void);
void IdMenu_Quit(void);
void SrvMenu_Choose(void);
void SrvMenu_Intro(void);
void SrvMenu_Quit(void);
void SrvMenu_SlideIn(void);
void SrvMenu_Start(void);
void (*gIdMenuScreens[4])(void) = {
    IdMenu_Start, IdMenu_Choose, IdMenu_Quit, NULL,
};
void (*gSrvMenuPages[8])(void) = {
    SrvMenu_Start, SrvMenu_SlideIn, SrvMenu_Choose, SrvMenu_Intro,
    SrvMenu_Quit, NULL, NULL, NULL,
};
