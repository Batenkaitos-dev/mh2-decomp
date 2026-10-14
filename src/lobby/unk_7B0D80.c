/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "sprite.h"
#include "lobby_text.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "pad.h"
#include "lobby_state.h"
#include "menu_task.h"

/* prototypes */
void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), LobbyDialog_DrawWin 31 of 35 words, LobbyDialog_Draw 583 of 737 words differ */
s32 NetScreen_Step4(void);
void LobbyConn_Next(void);
void LobbyConn_Finish(void);
s32 LobbyNet_AbortSocket(void);
void Menu_SetYesNo(s8 arg0);
void LobbyDialog_Grow(void);
s16 LobbyDialog_GetGrow(void);
void LobbyDialog_SetMember(void *arg0, void *arg1);
void LobbyDialog_ClearMember(void);
static void LobbyHelp_Reset(void);
void LobbyHelp_SetBox(s16 arg0, s16 arg1, s16 arg2);
void LobbyDialog_ShowConnMsg(s32 arg0);
void LobbyMenu_DrawDialog(MenuTask *task);
void Overlay_LoadGmSub(void);
void func_8FC460(s32 sock, s32 count, s32 self); /* unit's own declaration: with the definition's u8 count and self, LobbyConn_StartQuest differs (2 of 18 words); the function is gm_sub QNetIf_Start (not named in lobby.txt: 0x8FC460 falls inside nethttp HttpReq_Send) */
extern u8 gNetPartySelf;
/* end of prototypes */

s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
extern LobbyMenu *gLobbyMenu;
extern char *gLobbyTitleTexts[];
extern char *gLobbyTitleText;
void Game_ResetSubsystems(void);
extern u8 gNetConnStep;
extern s8 D_008F8CB0;
s32 NetTcp_Abort(s32 sock);
s32 NetTcp_Delete(s32 *sock);
extern s32 gNetSock;                    /* the socket (sub_main) */
extern NetTcpStatus gNetTcpStatus;
extern s8 gLobbyDialogChoice;
extern u8 gMenuInputLock;
extern s16 gLobbyDialogGrow;
extern char gLobbyDialogMemberId[8];
extern char gLobbyDialogMemberName[0x11];
extern s32 gMsgWinTimer;
extern s32 gMsgWinSubTimer;
extern s16 gMsgWinX;
extern s16 gMsgWinY;
extern s16 gMsgWinW;
void Text_SetSize(s32);
void LobbyDialog_Draw(void);
void LobbyDialog_DrawMember(void);
void LobbyText_Open(s32 msg);
extern s32 gLobbyDialogMsg;
extern u8 gLobbyDialogOpen;

/* the connection (0x7AF6C0 on) */
extern s16 gNetStepTimeout;                  /* frames left before the step fails */
extern u8 gNetScreenStep;
extern s8 gNetDisconnectStep;
extern s8 gNetErrorCode;                   /* the quest server's result byte */
s32 LobbyNet_DisconnectWait(void);
u16 LobbyPad_Get(s32 a);
void Text_PrintCodes(s16 max, const char *fmt, ...);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
void Font_SetColor(s32 color);
extern s32 gLobbyDialogWaitText;
extern s32 gLobbyDialogCountdown;
extern s16 gLobbyDialogW;
extern s16 gLobbyDialogH;
extern u8 gLobbyDialogKind;
extern char *gMsgWinText;
extern char *gLobbyHelpTexts[];
extern s16 gNetConnTries;                  /* tries */
extern void (*gLobbyConnSteps[])(void);      /* the connection's steps */
void LobbyNet_GetPartyAddr(s32 *arg0, s16 *arg1);
s32 Net_Connect(s32 addr, u16 port, u16 unk6);
void NetTcp_SetOption(s32 sock);
s32 Game_IsOnline(void);
void LobbyMenu_DrawIdName(s16 y, char *id, char *name);
void NetMode_DrawBigText(char *str, s32 x, s32 y, s32 size);

s32 NetScreen_Step4(void) {
    Game_ResetSubsystems();
    return -3;
}

extern u8 gLbsConnectStep;
extern u8 D_008BFAF9;                   /* the menu state's unk39 (gLobbyMenuData + 0x39) */
u16 Pad_GetPressedRepeat(s32 port);
void LobbyStream_Stop(void);
void SeqA_Start(s32);
s32 SeqA_IsPlaying(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void Lobby_DrawBlackLine(void);
void LobbyDialog_DrawPending(MenuTask *task);
void NetMode_SeqAReset(void);
void FlushCache(s32 kind);
extern char gBrowserUrl[0x100];            /* the browser's URL (sub_main) */
extern char gPortalUrlB[];
extern s8 gBrowserTopMode;
char *strcpy(char *dst, const char *src);
void Browser_Init(u8 mode);
s32 Browser_Update(void);
void LobbyKddi_CloseBrowser(void);
void NetScene_ToPortalPage3(void);
s32 NetMode_SeqAStart(s32, s32, s64);
void Text_SetColorIndex(s16 i);
void LobbyMenu_QueueLayers(void);

/* the network screen step 5: the second portal page (the account's,
 * 0x6ADFD7 = 2) in the browser, then a pause, the result message (0xB or
 * 0xE) and a key press; -4 when confirmed, 0 otherwise */
s32 NetScreen_Step5(void)
{
    u16 pad = Pad_GetPressedRepeat(0);
    s32 r = 0;

    switch (gNetScreenStep) {
    case 0:
        gNetScreenStep++;
        LobbyStream_Stop();
        SeqA_Start(1);
        break;
    case 1:
        if (SeqA_IsPlaying() == 0) {
            if (gLbsConnectStep != 0) {
                gNetScreenStep++;
            } else {
                gNetScreenStep = 7;
            }
            LobbyMenu_SetDraw(0, Lobby_DrawBlackLine, LobbyDialog_DrawPending, 0);
            NetMode_SeqAReset();
            gNetDisconnectStep = 0;
        }
        break;
    case 2:
        gNetScreenStep++;
    case 3:
        gNetScreenStep++;
        FlushCache(0);
        memset(gBrowserUrl, 0, 0x100);
        gPortalPage = 2;
        gBrowserTopMode = 1;
        strcpy(gBrowserUrl, gPortalUrlB);
        Browser_Init(0);
        break;
    case 4:
        switch (Browser_Update()) {
        case 0:
            break;
        case -100:
            LobbyKddi_CloseBrowser();
            NetScene_ToPortalPage3();
            break;
        default:
            LobbyKddi_CloseBrowser();
            gNetScreenStep++;
            break;
        }
        break;
    case 5:
        gNetScreenStep++;
        break;
    case 6:
        gNetScreenStep++;
    case 7:
        if (LobbyNet_DisconnectWait() != 0) {
            gNetScreenStep++;
        }
        break;
    case 8:
        gNetScreenStep = 9;
        gNetStepTimeout = 8;
        NetMode_SeqAStart(0, 0xA, 0);
        if (gLbsConnectStep == 0) {
            LobbyText_Open(0xE);
        } else {
            LobbyText_Open(0xB);
        }
        D_008BFAF9 = 1;
        break;
    case 9:
        D_008BFAF9 = 1;
        gNetStepTimeout--;
        if (gNetStepTimeout < 0) {
            gNetScreenStep++;
        }
        break;
    case 10:
        D_008BFAF9 = 1;
        if (pad & 0x20) {
            r = -4;
            gLobbyNetError = 0;
            gPortalPage = 0;
        }
        Text_SetColorIndex(5);
        Font_SetSpacing(0x14, 0x14);
        break;
    }
    LobbyMenu_QueueLayers();
    return r;
}

/* Runs the connection's current step (gLobbyConnSteps[gNetConnStep]); returns 1 once
 * it is connected (step 6), -1 once it failed (step 7), else 0. */
s32 LobbyConn_Run(void)
{
    s32 r;

    if (gNetConnStep == 6) {
        r = 1;
    } else if (gNetConnStep == 7) {
        r = -1;
    } else {
        r = 0;
    }
    gLobbyConnSteps[gNetConnStep]();
    return r;
}

/* step 0: closes the old socket */
void LobbyConn_Close(void)
{
    gNetStepTimeout = 0;
    gNetConnTries = 0;
    gLobbyConnDone = 0;
    gNetConnStep++;
    NetTcp_Abort(gNetSock);
    NetTcp_Delete(&gNetSock);
    memset(&gNetTcpStatus, 0, 8);
}

/* step 1: opens a connection to the server 6916 named (LobbyNet_GetPartyAddr), four
 * tries at most, then 0x258 frames for step 2 */
void LobbyConn_Open(void)
{
    u16 port;
    s32 addr;

    if (--gNetStepTimeout <= 0) {
        gNetStepTimeout = 0;
        gNetConnTries++;
        if (gNetConnTries > 3) {
            gNetStepTimeout = 0;
            gNetConnStep = 7;
            gNetConnTries = 0;
            return;
        }
        LobbyNet_GetPartyAddr(&addr, (s16 *) &port);
        if ((gNetSock = Net_Connect(addr, port, 0)) < 0) {
            gNetStepTimeout = 0x258;
            return;
        }
        gNetConnStep++;
        gNetStepTimeout = 0x258;
    }
}

/* step 2: waits for the connection (TCP state 4); back to step 1 on a
 * failure or when the time is up */
void LobbyConn_WaitOpen(void)
{
    if (--gNetStepTimeout <= 0) {
        gNetConnStep--;
        gNetStepTimeout = 10;
        NetTcp_Abort(gNetSock);
        NetTcp_Delete(&gNetSock);
        return;
    }
    if (NetTcp_Status(gNetSock, &gNetTcpStatus) < 0) {
        gNetConnStep--;
        gNetStepTimeout += 10;
        NetTcp_Abort(gNetSock);
        NetTcp_Delete(&gNetSock);
        return;
    }
    if (gNetTcpStatus.state == 4) {
        gNetStepTimeout = 0;
        gNetConnTries = 0;
        gNetConnStep++;
    }
}

/* the next connection step: sends the socket's ... (NetTcp_SetOption), 0xE10 frames (a minute) for the answer */
void LobbyConn_SetOption(void)
{
    gNetConnStep++;
    NetTcp_SetOption(gNetSock);
    gNetStepTimeout = 0xE10;
}

extern u8 gLobbyRecvHdr[];
extern u8 gAccountId[];                   /* the account id (sub_main) */
s32 Lobby_RecvPacket(s32 sock, u8 *hdr, u8 *data, u32 max);
s32 Net_ChallengeReply(char *out, s8 *ch, u16 key);

/*
 * The connection's first exchange: waits (gNetStepTimeout frames at most) for the
 * server's 1031 and answers it with a 1031 reply of 10 bytes,
 * Net_ChallengeReply(account id, the echo), sent directly on the socket.
 * gNetConnStep (the connection step) goes on by one, or to 7 on a failure.
 */
void LobbyConn_Challenge1031(void)
{
    u8 buf[0x20];
    u8 reply[0x10];
    s32 len;

    if (gNetStepTimeout-- < 0) {
        gNetConnStep = 7;
        return;
    }
    switch (Lobby_RecvPacket(gNetSock, gLobbyRecvHdr, gLobbyRecvData, 0x1000)) {
    case 1:
        break;
    case 0:
        return;
    case -1:
        gNetConnStep = 7;
        return;
    }
    switch ((u16)((u16)(gLobbyRecvHdr[2] << 8) | gLobbyRecvHdr[3])) {
    case 0x1031:
        Net_ChallengeReply((char *) reply, (s8 *) gAccountId, (u16)((u16)(gLobbyRecvHdr[6] << 8) + gLobbyRecvHdr[7]));
        LobbyPkt_Begin(&gLobbyPkt, 0x1031);
        LobbyPkt_SetFlag(&gLobbyPkt, 2);
        gLobbyPkt.echo[0] = gLobbyRecvHdr[6];
        gLobbyPkt.echo[1] = gLobbyRecvHdr[7];
        LobbyPkt_PutBytes(&gLobbyPkt, reply, 10);
        LobbyPkt_End(&gLobbyPkt);
        memcpy(buf, &gLobbyPkt.magic, 12);
        memcpy(buf + 12, gLobbyPkt.data, gLobbyPkt.len);
        len = gLobbyPkt.len + 12;
        if (NetTcp_Send(gNetSock, buf, len) != len) {
            gNetConnStep = 7;
            return;
        }
        gNetConnStep++;
        break;
    }
}

void LobbyConn_Next(void) {
    gNetConnStep += 1;
}

void LobbyConn_StartQuest(void)
{
    gNetConnStep = 0;
    gGmSubLoaded = 0;
    Overlay_LoadGmSub();
    func_8FC460(gNetSock, gLobby->partyCount, gNetPartySelf);
}

void LobbyConn_Finish(void) {
    gNetConnStep = 0;
    D_008F8CB0 = 3;
    gLobbyConnDone = 1;
}

/* a step run through gNetConnStep: waits a frame, then until LobbyNet_DisconnectWait
 * (sub_main Net_DisconnectStep) is done or sub_main 0x6ADFD7 is set (which closes
 * the socket); 1 then, with sub_main 0x5CADC8's byte kept in gNetErrorCode */
s32 LobbyNet_DisconnectStep(void)
{
    s32 r;

    r = 0;
    switch (gNetConnStep) {
    case 0:
        gNetConnStep++;
        break;
    case 1:
        gNetStepTimeout = 1;
        gNetScreenStep = 0;
        gNetConnStep++;
        gNetDisconnectStep = 0;
        break;
    case 2:
        if (--gNetStepTimeout < 0) {
            gNetConnStep++;
        }
        break;
    case 3:
        if (gPortalPage != 0) {
            NetTcp_Abort(gNetSock);
            NetTcp_Delete(&gNetSock);
            r = 1;
            gNetErrorCode = gLobbyNetError;
            gLobbyNetError = 0;
        } else if (LobbyNet_DisconnectWait() != 0) {
            r = 1;
            gNetErrorCode = gLobbyNetError;
            gLobbyNetError = 0;
        }
        break;
    }
    return r;
}

s32 LobbyNet_AbortSocket(void) {
    NetTcp_Abort(gNetSock);
    NetTcp_Delete(&gNetSock);
    return 2;
}

void LobbyDialog_OpenWait(void *text)
{
    gLobbyDialogWaitText = (s32) text;
    gLobbyDialogKind = 1;
    gLobbyDialogW = 0x1F4;
    gLobbyDialogH = 0x17C;
    LobbyDialog_ClearMember();
}

void LobbyDialog_OpenCountdown(void *text, s32 countdown)
{
    gLobbyDialogCountdown = countdown;
    gLobbyDialogWaitText = (s32) text;
    gLobbyDialogKind = 3;
    gLobbyDialogW = 0x1F4;
    gLobbyDialogH = 0x17C;
    LobbyDialog_ClearMember();
}

void Menu_SetYesNo(s8 choice) {
    gLobbyDialogChoice = choice;
    gLobbyMenu->yesNo = choice;
}

s32 LobbyDialog_Clear(void)
{
    return (s32)memset(&gLobbyDialogMsg, 0, 0x3C);
}

void LobbyDialog_Grow(void) {
    gLobbyDialogGrow = 3;
    gMenuInputLock = 9;
}

s16 LobbyDialog_GetGrow(void) {
    return gLobbyDialogGrow;
}

void LobbyDialog_SetMember(void *id, void *name) {
    memcpy(&gLobbyDialogMemberId, id, 8);
    memcpy(&gLobbyDialogMemberName, name, 0x11);
}

void LobbyDialog_ClearMember(void) {
    memset(&gLobbyDialogMemberId, 0, 8);
    memset(&gLobbyDialogMemberName, 0, 0x11);
}

extern LobbyMsgDef gLobbyMsgTexts[];
extern char *gLobbyDialogText;
extern s16 D_008F8D48;
extern u16 gLobbyDialogBlink;
extern s16 gLobbyDialogX;
extern s16 gLobbyDialogY;
extern s8 gLobbyDialogLines;
extern s8 gLobbyDialogNoLine;
char *strchr(char *s, s32 c);      /* strchr */
s32 Text_DisplayLength(char *s);

/* opens the message box with message msg (gLobbyMsgTexts): its size from the
 * text's longest line and its lines (two more for a yes/no, kinds 4-6),
 * centred */
void LobbyText_Open(s32 msg)
{
    LobbyMsgDef *d;
    char *line;
    char *p;
    s32 k;
    s32 n;
    s32 w = 0;
    s32 len;
    s32 bw;
    s32 bh;

    gLobbyDialogMsg = msg;
    gLobbyDialogText = gLobbyMsgTexts[msg].text;
    d = &gLobbyMsgTexts[msg];
    if (d->flags & 0x80) {
        gLobbyDialogNoLine = 0;
    } else {
        gLobbyDialogNoLine = 1;
    }
    memset(gLobbyDialogMemberId, 0, 8);
    memset(gLobbyDialogMemberName, 0, 0x11);
    p = gLobbyDialogText;
    n = 0;
    k = d->flags & ~0x80;
    gLobbyDialogOpen = 1;
    gLobbyDialogKind = k;
    D_008F8D48 = 0;
    gLobbyDialogBlink = 0;
    line = p;
    if (*p != 0) {
        do {
            p = strchr(p, 10);
            n++;
            if (p != NULL) {
                len = p - line;
                line = p;
            } else {
                len = Text_DisplayLength(line);
            }
            if (len < 0) {
                len = -len;
            }
            if (w < len) {
                w = len;
            }
            if (p == NULL) {
                break;
            }
            p++;
        } while (*p != 0);
    }
    switch (gLobbyDialogKind) {
    case 4:
    case 6:
        gLobbyDialogChoice = 0;
    case 5:
        n += 2;
        break;
    }
    bw = w * 10 + 0x3C;
    gLobbyDialogLines = n;
    bh = n * 20 + 0x3C;
    gLobbyDialogX = 0x140 - bw / 2;
    gLobbyDialogY = 0xE0 - bh / 2;
    gLobbyDialogW = bw;
    gLobbyDialogH = bh;
}

void LobbyTitle_Set(s32 idx)
{
    gLobbyTitleText = gLobbyTitleTexts[idx];
}

static void LobbyHelp_Reset(void) {
    gMsgWinTimer = 0;
    gMsgWinSubTimer = 0;
    gMsgWinX = 0x39;
    gMsgWinY = 0x16C;
    gMsgWinW = 0x244;
}

void LobbyHelp_Set(s32 i)
{
    gMsgWinText = gLobbyHelpTexts[i];
    LobbyHelp_Reset();
}

void LobbyHelp_SetText(void *p)
{
    gMsgWinText = p;
    LobbyHelp_Reset();
}

void LobbyHelp_SetBox(s16 x, s16 y, s16 w) {
    gMsgWinX = x;
    gMsgWinY = y;
    gMsgWinW = w;
}

void LobbyDialog_DrawWin(s16 x, s16 y, s16 w, s16 h, s32 c2)
{
    if (c2 != 0) {
        Win_Draw(x, y, w, h, 0xBDB59F6C, c2, 2);
        return;
    }
    Win_Draw(x, y, w, h, 0xBDB59F6C, 0x80202020, 2);
}

typedef struct GradBox {
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    u32 col[4];
} GradBox;
struct PrimGradRect;
s32 Prim_FillGradRect(struct PrimGradRect *);
extern u32 gMsgBoxGradient[4];              /* the message box's gradient */
extern char *D_0080D6D8;
extern char *D_0080D754;
extern char *D_0080D704;
extern char *gNoSaveTexts[];
extern char *D_0080D70C;
extern char *gNetStatusTexts[];
void Gfx_DrawBlackLineFixed(void);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 a);
void Gfx_SetState63(s32 a);
s32 Gs_SetState(u8, u32);
void TagText_Print(char *text, f32 x, f32 y);
void Lobby_DrawPressButton(void);
void Gfx_DrawBlackLine(s32 a, s32 b, s32 c, s32 d);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
s16 Lobby_DrawSprite155(s16, s16, s32);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
s32 Lobby_Strlen(char *s);
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
/* gLobbyDialogKind read as a struct member (reloaded after stores) */
typedef struct LobbyMsgKind {
    u8 kind;
} LobbyMsgKind;
#define MSG_KIND (((LobbyMsgKind *)&gLobbyDialogKind)->kind)

/* draws the connection dialog of kind gLobbyDialogKind (1 waiting, 2 a
 * progress bar, 3 a countdown, 4-8 the text gLobbyDialogText with a choice
 * gLobbyDialogChoice; 11 none): the box grows while gLobbyDialogGrow counts down.
 */
void LobbyDialog_Draw(void)
{
    s16 w;
    s16 h;
    s16 x;
    s16 y;
    char *s;
    char *nl;
    GradBox b;
    char buf[0x80];

    if (MSG_KIND == 11) {
        return;
    }
    if (++gLobbyDialogBlink >= 0x28) {
        gLobbyDialogBlink = 0;
    }
    if (MSG_KIND == 1) {
        Gfx_DrawBlackLineFixed();
    } else if (gLobbyDialogNoLine == 0 || MSG_KIND == 10) {
        Gfx_DrawBlackLineFixed();
        if (MSG_KIND == 10) {
            return;
        }
    }
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    Gfx_SetState63(1);
    Gs_SetState(0x60, 0);
    b.col[0] = gMsgBoxGradient[0];
    b.col[1] = gMsgBoxGradient[1];
    b.col[2] = gMsgBoxGradient[2];
    b.col[3] = gMsgBoxGradient[3];
    switch (MSG_KIND) {
    case 7:
    case 8:
        {
            s16 bx;
            s16 by;
            s16 bw;
            s16 bh;

            if (gLobbyDialogGrow >= 0) {
                bw = gLobbyDialogW - 0.15f * gLobbyDialogW * gLobbyDialogGrow;
                bh = gLobbyDialogH - 0.15f * gLobbyDialogH * gLobbyDialogGrow;
            } else {
                bw = gLobbyDialogW;
                bh = gLobbyDialogH;
            }
            bx = 0x140 - bw / 2;
            by = 0xE0 - bh / 2;
            b.x0 = bx - 4;
            b.y0 = by - 4;
            b.x1 = bx + bw + 4;
            b.y1 = by + bh + 4;
            Prim_FillGradRect((struct PrimGradRect *)&b);
            Win_Draw(bx, by, bw, bh, 0xBDB59F6C, 0x404020, 7);
        }
        break;
    case 2:
        break;
    default:
        if (gLobbyDialogGrow >= 0) {
            w = gLobbyDialogW - 0.15f * gLobbyDialogW * gLobbyDialogGrow;
            h = gLobbyDialogH - 0.15f * gLobbyDialogH * gLobbyDialogGrow;
        } else {
            w = gLobbyDialogW;
            h = gLobbyDialogH;
        }
        x = 0x140 - w / 2;
        y = 0xE0 - h / 2;
        b.x0 = x - 4;
        b.y0 = y - 4;
        b.x1 = x + w + 4;
        b.y1 = y + h + 4;
        Prim_FillGradRect((struct PrimGradRect *)&b);
        Win_Draw(x, y, w, h, 0xBDB59F6C, 0x404020, 1);
        if (MSG_KIND == 9 && gLobbyDialogBlink >= 0x15) {
            return;
        }
        break;
    }
    if (gLobbyDialogGrow >= 0) {
        gLobbyDialogGrow--;
        return;
    }
    Font_SetSpacing(0x14, 0x14);
    switch (MSG_KIND) {
    case 3:
        TagText_Print((char *)gLobbyDialogWaitText, 100.0f, 60.0f);
        Font_SetColor(0x807F7F28);
        Font_SetSpacing(0x14, 0x14);
        if (--gLobbyDialogCountdown < 0) {
            gLobbyDialogCountdown = 0;
            Lobby_DrawPressButton();
            return;
        }
        Font_SetPos(0x1F4, 0x40);
        Text_PrintfColored("%2d", gLobbyDialogCountdown / 30 + 1);
        return;
    case 1:
        TagText_Print((char *)gLobbyDialogWaitText, 100.0f, 60.0f);
        if (gLobby->leave6307 == 0 && gLobby->leave640C == 0 && gLobby->leave6505 == 0) {
            Lobby_DrawPressButton();
        }
        break;
    case 2:
        Gfx_DrawBlackLine(0x46, 0x22, 0x1F4, 0x17C);
        if (gLobbyMenu->pages != 0) {
            Menu_DrawPageNum(0x19A, 0x176, gLobbyMenu->page, gLobbyMenu->pages, 0);
            TagText_Print((char *)gLobbyDialogWaitText, 120.0f, 60.0f);
        } else {
            s = D_0080D6D8;
            Text_DrawShadow(Text_CenterX(s, 0x140, 0x14), 0xA0, 1, 4, s);
        }
        if (gFrameTick & 0x10) {
            Lobby_DrawSprite155(0x78, 0x174, 1);
        }
        Font_SetPos(0x96, 0x178);
        Font_SetColor(0x807F7F7F);
        Font_SetSpacing(0x12, 0x12);
        Text_PrintfColored(D_0080D754);
        break;
    default:
        y = gLobbyDialogY + 0x1E;
        s = gLobbyDialogText;
        while (*s != 0) {
            memcpy(buf, s, 0x80);
            nl = strchr(buf, '\n');
            if (nl != NULL) {
                *nl = 0;
            }
            {
                s32 cx = Text_CenterX(buf, 0x140, 0x14);

                Font_SetPos(cx, y);
            }
            Text_Printf(buf);
            if (nl == NULL) {
                break;
            }
            y += 0x14;
            s += Lobby_Strlen(buf) + 1;
        }
        switch (MSG_KIND) {
        case 4:
            if (gLobbyDialogChoice == 0) {
                Font_SetColor(0x807F5230);
                y += 0x28;
                Font_SetPos(0xFA, y);
                Text_PrintfColored("%s", D_0080D704);
                Font_SetColor(0x80404040);
                Font_SetPos(0x154, y);
                Text_PrintfColored("%s", gNoSaveTexts[0]);
                Menu_DrawGlowBar(0x10E, y - 2, 0x50, 0x18, 0xB0008000);
            } else {
                Font_SetColor(0x80404040);
                y += 0x28;
                Font_SetPos(0xFA, y);
                Text_PrintfColored("%s", D_0080D704);
                Font_SetColor(0x807F5230);
                Font_SetPos(0x154, y);
                Text_PrintfColored("%s", gNoSaveTexts[0]);
                Menu_DrawGlowBar(0x172, y - 2, 0x64, 0x18, 0xB0008000);
            }
            break;
        case 5:
            Font_SetColor(0x807F5230);
            y += 0x28;
            Font_SetPos(0x12C, y);
            Text_PrintfColored("%s", D_0080D704);
            Menu_DrawGlowBar(0x140, y - 2, 0x50, 0x18, 0xB0008000);
            break;
        case 7:
            y += 0x28;
            if (gLobbyDialogChoice == 0) {
                Menu_DrawGlowBar(0x140, y - 0xA2, 0xF0, 0x18, 0xB0008000);
            } else {
                Menu_DrawGlowBar(0x140, y - 0x7A, 0xF0, 0x18, 0xB0008000);
            }
            break;
        case 8:
            y += 0x28;
            Menu_DrawGlowBar(0x140, y - (10 - gLobbyDialogChoice * 2) * 20 - 2, 0xF0, 0x18, 0xB0008000);
            break;
        case 6:
            if (gLobbyDialogChoice == 0) {
                Font_SetColor(0x807F5230);
                y += 0x28;
                Font_SetPos(0xC4, y);
                Text_PrintfColored("%s", D_0080D70C);
                Font_SetColor(0x80404040);
                Font_SetPos(0x154, y);
                Text_PrintfColored("%s", gNetStatusTexts[0]);
                Menu_DrawGlowBar(0xF6, y - 2, 0x8C, 0x18, 0xB0008000);
            } else {
                Font_SetColor(0x80404040);
                y += 0x28;
                Font_SetPos(0xC4, y);
                Text_PrintfColored("%s", D_0080D70C);
                Font_SetColor(0x807F5230);
                Font_SetPos(0x154, y);
                Text_PrintfColored("%s", gNetStatusTexts[0]);
                Menu_DrawGlowBar(0x1A1, y - 2, 0xC2, 0x18, 0xB0008000);
            }
            break;
        }
        break;
    }
}

void LobbyDialog_DrawMember(void)
{
    if (Game_IsOnline() != 0 && gLobbyDialogGrow < 0 && gLobbyDialogMemberId[0] != 0) {
        LobbyMenu_DrawIdName(0x98, gLobbyDialogMemberName, gLobbyDialogMemberId);
    }
}

void LobbyTitle_Draw(void)
{
    NetMode_DrawBigText(gLobbyTitleText, -1, 0x1C, 0x22);
}

/* draws the message window (Win_Draw) and gMsgWinText's text, one more
 * character every frame (gMsgWinTimer, -1 for all of it; a button press shows
 * the rest) */
void LobbyMsg_Draw(void)
{
    u16 pad;

    pad = LobbyPad_Get(0);
    Win_Draw((s16)(gMsgWinX - 0x10), (s16)(gMsgWinY - 8), gMsgWinW, 0x43, 0xBDB59F6C, 0x70402020, 0);
    if (gMsgWinTimer >= 0) {
        if (++gMsgWinSubTimer > 0) {
            gMsgWinSubTimer = 0;
            if (++gMsgWinTimer > 0x100) {
                gMsgWinTimer = -1;
            }
        }
    }
    if (pad & 0x100) {
        gMsgWinTimer = -1;
    }
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(0x807F7F7F);
    if (gMsgWinText != NULL) {
        Font_SetPos(gMsgWinX, gMsgWinY);
        Text_PrintCodes(gMsgWinTimer, gMsgWinText);
    }
}

s32 LobbyVillage_IsLeaving(void);
s32 LobbyTown_IsLeaving(void);
void Sjis_Widen(char *src, char *dst);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
extern char *D_0080D61C;
extern char *gMemberInfoTexts[];
extern s32 gTownMode;

/* a user's id and name (full width) at y after their labels, underlined */
void LobbyMenu_DrawIdName(s16 y, char *id, char *name)
{
    char buf[0x40];

    if (LobbyVillage_IsLeaving() == 1 || LobbyTown_IsLeaving() == 1 || gTownMode == 0x16) {
        return;
    }
    if (gLobby->phase != 5) {
        Font_SetSpacing(0x14, 0x14);
        Sjis_Widen(id, buf);
        Text_DrawShadow(0xEE, y, 1, 4, buf);
        Sjis_Widen(name, buf);
        Text_DrawShadow(0xEE, y + 0x1E, 1, 4, buf);
        Text_DrawShadow(0xB2, y, 1, 4, D_0080D61C);
        Text_DrawShadow(0xB2, y + 0x1E, 1, 4, gMemberInfoTexts[0]);
        Gfx_DrawRectFrame(0xB2, y + 0x16, 0x11C, 1, 0xBDB59F6C);
        Gfx_DrawRectFrame(0xB2, y + 0x34, 0x11C, 1, 0xBDB59F6C);
    }
}

void LobbyDialog_ShowConnMsg(s32 arg0) {
    switch (arg0) {                                 /* irregular */
    case 0:
        break;
    case 1:
        if (gLobbyDialogOpen != 0) {
            if (gLobbyDialogMsg != 2) {
                goto block_9;
            }
        } else {
block_9:
            LobbyText_Open(2);
        }
        break;
    case 2:
        if (gLobbyDialogOpen != 0) {
            if (gLobbyDialogMsg != 0) {
                goto block_13;
            }
        } else {
block_13:
            LobbyText_Open(0);
        }
        break;
    case 3:
        if ((gLobbyDialogOpen == 0) || (gLobbyDialogMsg != 1)) {
            LobbyText_Open(1);
        }
        break;
    }
    LobbyDialog_Draw();
}

s32 Lobby_ReceiveAll(s32 sock);
u16 LobbyPad_Read(s8 port);
s32 Lobby_IsSysBusy(void);
void Lobby_ResetIdleTimer(s32 *timer);
void Lobby_Disconnect(s32);
s32 NetIf_GetError(void);
extern u8 gConnCheckState;
void LobbyTime_Tick(void);
extern s8 gFriendListOpen;
extern u16 gFriendCheckTimers[5];
extern s8 gFriendStatus[];                 /* 5 rows of 7 (the loop clears 10) */
extern u16 gAreaSearchTimer;
void Chat_ClearBeep(void);
extern void (*gLobbyScreens[])(void);      /* the lobby screens (LobbyState.phase) */
extern s32 gLobbyActive;

/* the online lobby's frame: receives, runs the idle and connection
 * timeouts (Lobby_Disconnect: 4 idle, 7 connection lost, 6 and 8 the
 * server's end), the table timers, then the current screen; returns
 * gLobbyActive (1 after a timeout) */
s32 Lobby_Tick(void)
{
    s32 pad = Pad_GetPressedRepeat(0);
    s32 j;
    s32 i;

    i = gSysBusy;
    if (i == 0 && Lobby_ReceiveAll(gNetSock) == 1) {
        gLobby->recvTimer = 0xE10;
    }
    gLobby->recvTimer--;
    if (gMenuInputLock > 0) {
        gMenuInputLock--;
    }
    gLobby->buttons = LobbyPad_Read(0);
    if (gLobby->phase != 5) {
        if (Lobby_IsSysBusy() == 0 && gLobby->connected != 0) {
            if (pad != 0 || (gKbd)[0x668] != 0 || (gPadState[0].dir & 0x3C3C)) {
                Lobby_ResetIdleTimer(&gLobby->idleTimer);
            }
            if (--gLobby->idleTimer < 0) {
                Lobby_Disconnect(4);
                return 1;
            }
            if (NetIf_GetError() != 0 || gConnCheckState != 0) {
                Lobby_Disconnect(7);
                return 1;
            }
            if (gLobby->recvTimer < 0) {
                Lobby_Disconnect(7);
                return 1;
            }
            if (gLobby->serverClose == 1) {
                Lobby_Disconnect(6);
                return 1;
            }
            if (gLobby->serverClose == 2) {
                Lobby_Disconnect(8);
                return 1;
            }
        } else if (gLobby->linkState == 1 && gConnCheckState == 0) {
            if (pad != 0 || (gKbd)[0x668] != 0) {
                Lobby_ResetIdleTimer(&gLobby->idleTimer);
            }
            if (gLobby->recvTimer < 0) {
                gConnCheckState = 1;
            } else if (--gLobby->idleTimer < 0) {
                gConnCheckState = 1;
            } else if (gLobby->serverClose == 1 || gLobby->serverClose == 2) {
                gConnCheckState = 1;
            } else if (NetIf_GetError() != 0) {
                gConnCheckState = 1;
            }
        }
    }
    LobbyTime_Tick();
    gLobbyMenu->redraw = 0;
    gLobbyMenu->hideLayers = 0;
    Text_SetSize(6);
    if (gFriendListOpen == 0) {
        for (i = 0; i < 5; i++) {
            if (0 < gFriendCheckTimers[i]) {
                gFriendCheckTimers[i]--;
            } else if (gFriendCheckTimers[i] == 0) {
                for (j = 0; j < 10; j++) {
                    if (j + i * 10 < 0x33) {
                        gFriendStatus[i * 7 + j] = -1;
                    }
                }
                gFriendCheckTimers[i] = 0xFFFF;
            }
        }
    }
    if (gAreaSearchTimer != 0) {
        gAreaSearchTimer--;
    }
    Chat_ClearBeep();
    gLobbyScreens[gLobby->phase]();
    if (gLobbyMenu->hideLayers == 0) {
        LobbyMenu_QueueLayers();
    }
    return gLobbyActive;
}

void LobbyMenu_QueueLayers(void)
{
    MenuTask *t;

    t = gLobbyMenu->layer[0];
    if ((t != NULL) && (t->draw != NULL)) {
        OT_AddAt(gDrawListText0b, t, 0, 1);
    }
    t = gLobbyMenu->layer[1];
    if ((t != NULL) && (t->draw != NULL)) {
        OT_AddAt(gDrawListText1, t, 0, 1);
    }
    t = gLobbyMenu->layer[2];
    if ((t != NULL) && (t->draw != NULL)) {
        OT_AddAt(gDrawListText2, t, 0, 1);
    }
    t = gLobbyMenu->layer[3];
    if ((t != NULL) && (t->draw != NULL)) {
        OT_AddAt(gDrawOtText3, t, 0, 0x10);
    }
    t = gLobbyMenu->layer[4];
    if ((t != NULL) && (t->draw != NULL)) {
        OT_AddAt(gDrawListText5, t, 0, 1);
    }
}

void LobbyMenu_DrawDialog(MenuTask *task) {
    u8 temp_a0;

    Text_SetSize(task->textSize);
    temp_a0 = gLobbyMenu->redraw;
    if (temp_a0 == 1) {
        LobbyDialog_Draw();
        LobbyDialog_DrawMember();
    }
}

/* unk_7B0D80's .data (0x819400-0x819450) */
void LevelList_Run(void);
void LobbyEnter_Run(void);
void LobbyEntry_Run(void);
void LobbyExit_Run(void);
void LobbyLevel1_ToVillage(void);
void LobbyRoom_Run(void);
void LobbyRoom_RunClose(void);
void LobbyTown_Run(void);
void (*gLobbyConnSteps[8])(void) = {
    LobbyConn_Close, LobbyConn_Open, LobbyConn_WaitOpen, LobbyConn_SetOption,
    LobbyConn_Challenge1031, LobbyConn_Next, LobbyConn_StartQuest, LobbyConn_Finish,
};
u32 gMsgBoxGradient[4] = {
#include "lobby/00819420.inc"
};
void (*gLobbyScreens[8])(void) = {
    LobbyEntry_Run, LevelList_Run, LobbyLevel1_ToVillage, LobbyTown_Run,
    LobbyRoom_Run, LobbyExit_Run, LobbyRoom_RunClose, LobbyEnter_Run,
};
