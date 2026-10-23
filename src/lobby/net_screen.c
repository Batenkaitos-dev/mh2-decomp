/*
 * The network screen's small services: its draw callback, the fade, the
 * texture release, the field printer and the box resets.
 *
 * The card menu's online save hands its draw slot to the network screen
 * with NetMode_Begin, which hooks NetMode_Draw into it: the card menu's
 * backdrop (gCardMenuTex) while that menu shows it, then the screen's frame
 * gNetModeFrame and NetMode_DrawScreen's boxes. NetMode_FadeStart,
 * NetMode_FadeRunning and NetMode_FadeReset wrap the screen fade (FadeA_*).
 * NetMode_PrintField prints one of the screen's fields (NET_FIELD_*: the
 * login id, the password as stars, the percentage, the error code, the
 * version) at its place in gNetFieldPos; the message queues call it. The
 * empty NetMode_Empty*Hook functions are called by the set-up's steps and
 * do nothing.
 */
#include "common.h"
#include "net_save.h"
#include "sprite.h"
#include "net_mode.h"
#include "lobby_globals.h"
#include "window.h"
#include "render_state.h"
#include "scene.h"
#include "lobby/net_screen.h"   /* the texts this file uses (tools/content_gen.py) */
#include "card.h"
#include "texture.h"
#include "font.h"
#include "lobby_util.h"
#include "mem_util.h"
#include "screen_fade.h"
#include "prim.h"


/* Where the network screen prints each of its fields (NetMode_PrintField),
 * by NET_FIELD_*. */
typedef struct FieldPos {
    s16 x;
    s16 y;
    u8 spacing;                 /* the letters' spacing, across and down (Font_SetSpacing) */
    u8 pad5;
    s16 color;                  /* Font_SetColorIndex */
} FieldPos;

extern FieldPos gNetFieldPos[];
extern ListWin gNetModeFrame;
extern s32 gCardMenuTex;
extern char gNetVerDigits[];
extern char gNetVerDigits_1[];
struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *, s8 *title, s32 sel, u8 alpha);
void *memset(void *dst, s32 c, u32 n);
char *strcpy(char *dst, const char *src);
u32 strlen(const char *s);
char *strcat(char *dst, const char *src);
s32 sprintf(char *buf, const char *fmt, ...);
char *strncpy(void *, void *, u32);
void Font_Printf(const char *fmt, ...);

/* Does nothing. NetMode_RunSetup calls it with 0 each frame unless
 * gNetMode.noDraw has NET_NO_DRAW_HOOK. */
void NetMode_EmptyDrawHook(s32 unused)
{

}

/* NetMode_DrawScreen; withText 1 also draws the text layer's page 4. */
void NetMode_CallDrawScreen(s32 withText)
{
    NetMode_DrawScreen((s8)withText);
}

/* The network screen's draw callback (NetMode_Begin hooks it into the card
 * menu's draw slot): while the card menu shows its backdrop (CardMenu_IsTexOn
 * and gCardMenuTex loaded), the backdrop over the whole screen; then the
 * frame gNetModeFrame and the screen's boxes. The draw slot isn't read. */
void NetMode_Draw(DrawSlot *ds)
{
    s32 oldZwrite;
    s32 oldBlend;
    s32 oldFilter;
    s32 oldTex;
    Sprite s;

    if (CardMenu_IsTexOn() == 1 && gCardMenuTex != 0) {
        oldZwrite = Gs_GetState(RS_ZWRITE);
        oldBlend = Gs_GetState(RS_BLEND);
        oldFilter = Gs_GetState(RS_TEX_FILTER);
        oldTex = Gs_GetState(RS_TEXTURE);
        Gs_SetState(RS_ZWRITE, 0);
        Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
        Gs_SetState(RS_TEX_FILTER, 0);
        Tex_Use(1, (u32 *) &gCardMenuTex);
        Gs_SetState(RS_TEXTURE, gCardMenuTex);
        s.w = 640;
        s.u1 = 255;
        s.h = 448;
        s.v1 = 255;
        s.color = -1;
        s.x = 0;
        s.y = 0;
        s.u0 = 0;
        s.v0 = 0;
        Sprite_Draw(&s);
        Gs_SetState(RS_TEXTURE, oldTex);
        Gs_SetState(RS_TEX_FILTER, oldFilter);
        Gs_SetState(RS_BLEND, oldBlend);
        Gs_SetState(RS_ZWRITE, oldZwrite);
    }
    ListPanel_DrawAlpha(&gNetModeFrame, NULL, -1, 128);
    NetMode_CallDrawScreen(0);
}

/* Starts the network screen in the card menu's draw slot ds (the online
 * save's end): clears the step and both message queues, hooks NetMode_Draw
 * and restores the lobby's work area (Lobby_RestoreWork). */
void NetMode_Begin(DrawSlot *ds)
{
    gNetMode.innerStep = 0;
    gNetMode.nA = 0;
    gNetMode.nB = 0;
    ds->draw = NetMode_Draw;
    Lobby_RestoreWork();
}

/* 1 while the screen fade runs (FADE_STATE_RUNNING), else 0. */
s32 NetMode_FadeRunning(void)
{
    s32 state;

    state = FadeA_GetState() & 0xFF;
    switch (state) {
    case FADE_STATE_IDLE:
        return 0;
    case FADE_STATE_RUNNING:
        return 1;
    default:
        return 0;
    }
}

void NetMode_FadeReset(void)
{
    FadeA_Reset();
}

/* Starts the screen fade: to black when fadeOut is 1, else back from black.
 * The fade's length is the key's own (16 frames); the callers' frame
 * counts aren't read. Returns 1. */
s32 NetMode_FadeStart(s32 unused, s32 frames, s16 fadeOut)
{
    if (fadeOut == 1) {
        FadeA_Start(FADE_OUT_BLACK);
    } else {
        FadeA_Start(FADE_IN_BLACK);
    }
    return 1;
}

/* Closes every box and frees the network screen's textures: nine slots
 * from NET_TEX_SLOT (NetMode_LoadTextures frees NET_TEX_SLOTS) and
 * sub_main's lobby set; clears gNetMode.tex and compacts the heap. */
void NetMode_ReleaseTextures(s32 unused)
{
    s32 i;

    NetMode_CloseAll();
    Tex_FreeRange(NET_TEX_SLOT, 9);
    Tex_FreeRange(NET_LOBBY_TEX_SLOT, NET_LOBBY_TEX_SLOTS);
    for (i = 0; i < NET_TEX_COUNT; i++) {
        gNetMode.tex[i] = 0;
    }
    Mem_Compact();
}

/* Does nothing: NetMode_Init, the disconnect and the patch's install call it
 * after the textures are released. */
void NetMode_EmptyResetHook(void)
{

}

/* Does nothing: the patch's steps call it with 0 before NetMode_FadeReset. */
void NetMode_EmptyFadeHook(s32 unused)
{

}

/* Starts the card task in its mode (CardTask_Begin). */
void NetMode_BeginCardTask(s32 mode)
{
    CardTask_Begin(mode);
}

/* Does nothing: NetMode_Init calls it. */
void NetMode_EmptyInitHook(void)
{

}

/* Prints field `field` (NET_FIELD_*) of the network screen at its place
 * and in its colour from gNetFieldPos: the login id, the password as stars,
 * the percentage, the error code or the version. Fields 0, 3 and 4 print
 * nothing. */
void NetMode_PrintField(s32 unused, s32 field)
{
    char num[16];
    char ver[16];
    char buf[128];
    s16 n;
    s32 i;

    memset(buf, 0, sizeof(buf));
    switch (field) {
    case 0:
    case 3:
    case 4:
        break;
    case NET_FIELD_LOGIN_ID:
        strcpy(buf, gNetSaveData.mmbbLoginId);
        break;
    case NET_FIELD_PASSWORD:
        n = strlen(gNetSaveData.mmbbPassword);
        if (n != 0) {
            for (i = 0; i < n; i++) {
                buf[i] = '*';
            }
        }
        break;
    case NET_FIELD_PERCENT:
        sprintf(num, "%3d", gNetMode.percent);
        Sjis_Widen(num, buf);
        strcat(buf, TXT(0x00847150) /* a full-width percent sign */);
        break;
    case NET_FIELD_ERROR_CODE:
        sprintf(buf, TXT(0x00847158) /* the error code, with its label */, gNetMode.err);
        break;
    case NET_FIELD_VERSION:
    case 8:
    case 9:
        /* the version's digits a.bcd, widened */
        memset(ver, 0, 10);
        strncpy(ver, gNetVerDigits, 1);
        strcat(ver, ".");
        strncpy(ver + 2, gNetVerDigits_1, 3);
        Sjis_Widen(ver, buf);
        break;
    }
    Font_SetColorIndex(gNetFieldPos[field].color);
    Font_SetSpacing(gNetFieldPos[field].spacing, gNetFieldPos[field].spacing);
    Font_SetPos(gNetFieldPos[field].x, gNetFieldPos[field].y);
    Font_Printf(buf);
}

/* Closes every box of the network screen. */
void NetMode_CloseAll(void)
{
    gNetMode.boxes = 0;
}

/* Leaves only the frame shown. */
void NetMode_ResetToFrame(void)
{
    gNetMode.boxes = NET_BOX_FRAME;
}

/* The file's .data: the frame, then the fields' places */
ListWin gNetModeFrame = {
#include "lobby/008197D0.inc"
};
FieldPos gNetFieldPos[10] = {
#include "lobby/008197F0.inc"
};
