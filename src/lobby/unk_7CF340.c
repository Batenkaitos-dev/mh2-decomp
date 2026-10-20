/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "net_save.h"
#include "sprite.h"
#include "net_mode.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "window.h"
#include "lobby/unk_7CF340.h"   /* the texts this file uses (tools/content_gen.py) */

void NetMode_CloseAll(void);
void Mem_Compact(void);
void Tex_FreeRange(s32 a, s32 b);
void FadeA_Start(s32);
void NetMode_DrawScreen(s32);
u8 FadeA_GetState(void);
void FadeA_Reset(void);
void CardTask_Begin(s32 i);


/* where the connection screen prints each of its fields (NetMode_PrintField) */
typedef struct FieldPos {
    s16 x;
    s16 y;
    u8 size;
    u8 pad5;
    s16 color;
} FieldPos;

extern FieldPos gNetFieldPos[];
extern ListWin gNetModeFrame;
extern s32 gCardMenuTex;
extern char gNetVerDigits[];
extern char D_008EDF11[];
s32 CardMenu_IsTexOn(void);
s32 Gs_GetState(u8 reg);
s32 Gs_SetState(u8, u32);
s32 Tex_Use(s32 n, u32 *handles);
s32 Sprite_Draw(Sprite *s);
struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *, s8 *title, s32 sel, u8 alpha);
void Lobby_RestoreWork(void);
void *memset(void *dst, s32 c, u32 n);
char *strcpy(char *dst, const char *src);
u32 strlen(const char *s);
char *strcat(char *dst, const char *src);
s32 sprintf(char *buf, const char *fmt, ...);
char *strncpy(void *, void *, u32);
void Sjis_Widen(char *src, char *dst);
void Font_SetColorIndex(s16 c);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16 x, s16 y);
void Font_Printf(const char *fmt, ...);

/* empty; its one caller (NetMode_RunSetup) passes 0 */
void NetMode_EmptyDrawHook(s32 unused) {

}

void NetMode_CallDrawScreen(s32 a0)
{
    NetMode_DrawScreen((s8)a0);
}

/* the screen's draw callback: a full-screen fade while the card flag is set
 * and a fade is running (gCardMenuTex), then the frame gNetModeFrame and the boxes */
void NetMode_Draw(DrawSlot *ds /* unused: the draw-slot walk passes it */)
{
    s32 r6C;
    s32 r5E;
    s32 r63;
    s32 r4;
    Sprite s;

    if (CardMenu_IsTexOn() == 1 && gCardMenuTex != 0) {
        r6C = Gs_GetState(0x6C);
        r5E = Gs_GetState(0x5E);
        r63 = Gs_GetState(0x63);
        r4 = Gs_GetState(4);
        Gs_SetState(0x6C, 0);
        Gs_SetState(0x5E, 0x32);
        Gs_SetState(0x63, 0);
        Tex_Use(1, (u32 *) &gCardMenuTex);
        Gs_SetState(4, gCardMenuTex);
        s.w = 0x280;
        s.u1 = 0xFF;
        s.h = 0x1C0;
        s.v1 = 0xFF;
        s.color = -1;
        s.x = 0;
        s.y = 0;
        s.u0 = 0;
        s.v0 = 0;
        Sprite_Draw(&s);
        Gs_SetState(4, r4);
        Gs_SetState(0x63, r63);
        Gs_SetState(0x5E, r5E);
        Gs_SetState(0x6C, r6C);
    }
    ListPanel_DrawAlpha(&gNetModeFrame, 0, -1, 0x80);
    NetMode_CallDrawScreen(0);
}

/* starts the screen: clears its counters and sets the draw callback */
/* Starts the connection screen: its draw slot (sub_main gNetMenuDrawHooks) draws NetMode_Draw. */
void NetMode_Begin(DrawSlot *ds)
{
    gNetMode.innerStep = 0;
    gNetMode.nA = 0;
    gNetMode.nB = 0;
    ds->draw = NetMode_Draw;
    Lobby_RestoreWork();
}

s32 NetMode_SeqABusy(void) {
    s32 state;

    state = FadeA_GetState() & 0xFF;
    switch (state) {                              /* irregular */
    case 0:
        return 0;
    case 1:
        return 1;
    default:
        return 0;
    }
}

void NetMode_SeqAReset(void) {
    FadeA_Reset();
}

s32 NetMode_SeqAStart(s32 unused0, s32 unused1, s64 arg2)
{
    if (((s64) (arg2 << 0x30) >> 0x30) == 1) {
        FadeA_Start(1);
    } else {
        FadeA_Start(2);
    }
    return 1;
}

void NetMode_ReleaseTextures(M2C_UNK unused0)
{
    s32 i;

    NetMode_CloseAll();
    Tex_FreeRange(0xA3, 9);
    Tex_FreeRange(2, 0xA);
    for (i = 0; i < 16; i++) {
        gNetMode.tex[i] = 0;
    }
    Mem_Compact();
}

void func_007CF620(void) {

}

/* empty; its callers (NetSetup_SaveNetFile, LobbyNetXfer_Run) pass 0 */
void func_007CF630(s32 unused) {

}

void NetMode_BeginCardTask(s32 i) {
    CardTask_Begin(i);
}

void func_007CF650(void) {

}

/* prints field `kind` of the connection screen at its place from
 * gNetFieldPos: the account id (1), the password as stars (2), a percentage
 * (5), an error code (6), the version (7-9) */
void NetMode_PrintField(s32 unused, s32 kind)
{
    char num[0x10];
    char ver[0x10];
    char buf[0x80];
    s16 n;
    s32 i;

    memset(buf, 0, 0x80);
    switch (kind) {
    case 0:
    case 3:
    case 4:
        break;
    case 1:
        strcpy(buf, gNetSaveData.mmbbLoginId);
        break;
    case 2:
        n = strlen(gNetSaveData.mmbbPassword);
        if (n != 0) {
            for (i = 0; i < n; i++) {
                buf[i] = '*';
            }
        }
        break;
    case 5:
        sprintf(num, "%3d", gNetMode.percent);
        Sjis_Widen(num, buf);
        strcat(buf, TXT(0x00847150) /* a full-width percent sign */);
        break;
    case 6:
        sprintf(buf, TXT(0x00847158) /* the error code, with its label */, gNetMode.err);
        break;
    case 7:
    case 8:
    case 9:
        memset(ver, 0, 0xA);
        strncpy(ver, gNetVerDigits, 1);
        strcat(ver, ".");
        strncpy(ver + 2, D_008EDF11, 3);
        Sjis_Widen(ver, buf);
        break;
    }
    Font_SetColorIndex(gNetFieldPos[kind].color);
    Font_SetSpacing(gNetFieldPos[kind].size, gNetFieldPos[kind].size);
    Font_SetPos(gNetFieldPos[kind].x, gNetFieldPos[kind].y);
    Font_Printf(buf);
}

void NetMode_CloseAll(void) {
    gNetMode.boxes = 0;
}

void NetMode_ResetToFrame(void) {
    gNetMode.boxes = 1;
}

/* unk_7CF340's .data (0x8197D0-0x819840) */
ListWin gNetModeFrame = {
#include "lobby/008197D0.inc"
};
FieldPos gNetFieldPos[10] = {
#include "lobby/008197F0.inc"
};
