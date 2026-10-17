/*
 * HUD and frame helpers:
 * - icons
 * - text wrapping (Shift-JIS)
 * - play-time strings
 * - the per-frame callback lists
 * - the draw order of a frame
 * - pad input (with the demo replay)
 * - screen captures
 * - keyframe interpolation
 */
#include "common.h"
#include "syssettings.h"
#include "player.h"
#include "pad.h"
#include "sprite.h"
#include "prim.h"
#include "scene.h"
#include "model.h"
#include "sub_main/unk_36BE70.h"   /* the texts this file uses (tools/content_gen.py) */
#include "render_state.h"
#include "overlay.h"
#pragma optimization_level 4

/* Keyframes, ended by a time of -1: the value is used as is from there. */
typedef struct Key1 {
    f32 t;
    f32 v;
} Key1;

typedef struct Key2 {
    f32 t;
    f32 v[2];
} Key2;

typedef struct Key3 {
    f32 t;
    f32 v[3];
} Key3;

/* The owner of a keyframe track: its clock at +0x15C. */
typedef struct KeyClock {
    u8 pad[0x15C];
    f32 time;
} KeyClock;

typedef void (*Callback)(void);


/* The owner of the bones that a tree node can copy (by index & 0x3F). */
typedef struct BoneRoot {
    u8 pad[0xD0];
    EmBone *bones[64];            /* 0xD0 */
} BoneRoot;

extern s16 *gBoneMtxSlotMaps[];       /* per table: a bone index per node, 0 none, bit 6 relative */

/* A textured rectangle for Sprite_Draw (0x14 bytes). */

extern Sprite gBlackLineSprite __attribute__((aligned(16)));

extern u8 D_006ADF7A[];         /* per port: replaying input */
extern u16 *D_006ADF7C[];       /* per port: the replay cursor {buttons, frames} */
extern u16 D_006ADF84[];        /* per port: frames left for the current entry */

extern char D_005BA990[];
extern char D_005BA9B0[];

extern s32 D_005BA9D0;
extern s32 gCallbackCount;
extern u8 gCallbackFlags[64];
extern Callback gCallbacks[64];
extern s32 gLayerCallbackCount;
extern u8 gLayerCallbackLayers[64];
extern Callback gLayerCallbackFns[64];
extern u8 gEquipPreview[];
extern s32 gFrameDrawing;
extern u8 gScreenCaptureOk;
extern u8 *gReadBuf;

void Hud_DrawButtonIcon(s32 id, s32 x, s32 y, s32 depth, u32 color, f32 scale);
struct TexRect;
void TexRect_DrawRotated(struct TexRect *r, s32 angle);
s32 Text_DrawWrapped(s16 x, s16 y, s16 lineh, char *s, s16 width);
s32 sprintf(char *dst, char *fmt, ...);     /* sprintf */
void *memset(void *dst, s32 c, u32 n);
/* unit's own declaration: with the definition's void (s32 id) (main/main.c), Scene_LoadOverlay: 1 of 2 words differ */
void LoadOverlay(u8 id);
void Tex_Prepare(s32 a, s32 b);
void Tex_Bind(s32 a);
void Gfx_SetTexFilter(s32 a);
void Gfx_SetBlend(s32 a, s32 b);
s32 Gs_SetState(u8 reg, u32 v);
/* unit's own declaration: with the definition's void (s16 x, s16 y, s16 w, s16 h, s32 color) (sub_main/unk_363550.c), Gfx_DrawBlackLine: 17 of 37 words differ */
void Gfx_DrawLine(s32 a, s32 b, s32 c, s32 d, u32 color);
void Callbacks_Clear(void);
void LayerCallbacks_Clear(void);
void FadeA_Draw(void);
void Loading_DrawLetters(void);
u16 Pad_ReadButtons(s32 port);
void Pad_CopyWords(s32 port);
void Font_SetHalfWidth(s32 a);
void AreaSky_Draw(void);
void StageObj_DrawKind4(void);
void LayerCallbacks_Run(u8 layer);
void LightSet_Apply(s32 a);
void ObjAttach_DrawAll(void);
void Task_RunList(u32 *list);
void Obj_DrawAll(void);
void StageObj_DrawAll(void);
void Sky_Draw(void);
void Area_DrawGlowProps(void);
void ScreenCapture_Take(void);
void Pause_CaptureScreen(void);
void Prim_DrawAll(void);
void Text_DrawPage(s32 layer);
void Text_DrawIcons(s32 layer);
void EquipPreview_Draw(void);
void FadeB_Draw(void);
void Text_Disable(void);
void Gfx_ResetRenderStates(s32 a);
void Text_Reset(void);
void Pause_DrawCapture(void);
s32 VramSlot_FreeAll(void);
void FlushCache(s32 op);
s32 Dma_WaitIdle(void);
s32 ScreenTex_Upload(void);
s32 Sprite_Draw(Sprite *s);
void Player_DrawLocal(Player *p, s32 a);
void Player_Draw(Player *p, s32 a);
void CharMake_DrawHunter(u8 *p, s32 a);
void ScreenTex_Init(void *buf);
s32 ScreenTex_Capture(s32 a);
s32 Game_IsOnline(void);
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_Copy3x3(f32 *m, f32 *a);
void Mtx_Mul3x3(f32 *m, f32 *a, f32 *b);
void Mtx_Identity(f32 *m);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);

/*
 * Draws pad button icon id (a 24x24 cell of the icon sheet, 32 wide for 8 and 9)
 * at x, y scaled by scale. 0x16-0x18 are arrows drawn from cell 0x10 (flipped or
 * rotated), 0x14/0x15 use cells 0/1, 0x19 is a small 18x18 one. depth is unused.
 */
void Hud_DrawButtonIcon(s32 id, s32 x, s32 y, s32 depth, u32 color, f32 scale)
{
    Sprite s;
    f32 size;
    s32 kind;
    s16 i;

    kind = (s16)id;
    if (kind == 0x16 || kind == 0x17 || kind == 0x18) {
        id = 0x10;
    }
    i = id;
    if (i == 8 || i == 9) {
        size = 32.0f;
    } else {
        size = 24.0f;
    }
    if (i == 0x14) {
        id = 0;
    } else if (i == 0x15) {
        id = 1;
    }
    Gfx_SetTexFilter(0);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    i = id;
    s.x = x;
    s.y = y;
    s.w = size * scale;
    s.h = 24.0f * scale;
    s.color = color;
    if (i == 9) {
        s.u0 = 0xE0;
    } else {
        s.u0 = (i % 10) * 24;
    }
    s.v0 = (i / 10) * 24;
    s.u1 = s.u0 + (s16)size + 1;
    if (i == 0x12 || i == 0x11 || i == 0x13) {
        s.v1 = s.v0 + 24;
    } else {
        s.v1 = s.v0 + 25;
    }
    if (i == 0x19) {
        s.u0 = 3;
        s.v0 = 0x1B;
        s.u1 = s.u0 + 18;
        s.v1 = s.v0 + 18;
        s.w = s.h = 18.0f * scale;
        s.x++;
        s.y++;
    }
    switch ((s16)kind) {
    case 0x16: {
        s16 t = s.u1 - 1;
        s.u1 = s.u0;
        s.u0 = t;
        Sprite_Draw(&s);
        break;
    }
    case 0x17:
        s.u1--;
        TexRect_DrawRotated((struct TexRect *)&s, 0x4000);
        break;
    case 0x18:
        s.u1--;
        TexRect_DrawRotated((struct TexRect *)&s, 0xC000);
        break;
    default:
        Sprite_Draw(&s);
        break;
    }
    Gfx_SetTexFilter(1);
}

void Hud_DrawButtonIconWhite(s32 a, s32 b, s32 c, s32 d, f32 scale)
{
    Hud_DrawButtonIcon(a, b, c, d, -1, scale);
}

/* 1 if byte n of s is the first byte of a two-byte (Shift-JIS) character. */
s32 Text_IsDoubleByteAt(u8 *s, u32 n)
{
    u32 i;
    u32 end;
    u8 c;

    end = n;
    end++;
    i = 0;
    while (i < end) {
        c = *s;
        if ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFF)) {
            if (i == n) {
                return 1;
            }
            s += 2;
            i += 2;
        } else {
            s += 1;
            i += 1;
        }
    }
    return 0;
}

void Text_DrawWrapped33(s16 x, s16 y, s16 lineh, char *s)
{
    Text_DrawWrapped(x, y, lineh, s, 0x21);
}

s32 Str_Length(char *s);
char *strchr(char *s, s32 c);
void Font_SetPos(s16 x, s16 y);
void Text_Draw(char *s);
char *strcpy(char *dst, const char *src);

/* Draws s from (x, y) in lines of at most width bytes (a double-byte
 * character is not split) or up to each '\n', at most 50 lines; returns the
 * line count. */
s32 Text_DrawWrapped(s16 x, s16 y, s16 lineh, char *str, s16 width)
{
    char buf[0x80];
    char *s;
    s32 n;
    s32 i;
    s32 len;
    char *nl;

    if (str == NULL) {
        return 0;
    }
    s = str;
    n = 0;
    y -= lineh;
    for (i = 0; i < 50; i++) {
        if (s == NULL) {
            break;
        }
        n++;
        len = Str_Length(s);
        strcpy(buf, s);
        if (width - 1 < len) {
            if (Text_IsDoubleByteAt((u8 *)buf, width - 1) == 0) {
                buf[width] = 0;
                if ((nl = strchr(buf, '\n')) != NULL) {
                    *nl = 0;
                    s = strchr(s, '\n') + 1;
                } else {
                    s += width;
                }
            } else {
                buf[width - 1] = 0;
                if ((nl = strchr(buf, '\n')) != NULL) {
                    *nl = 0;
                    s = strchr(s, '\n') + 1;
                } else {
                    s += width - 1;
                }
            }
            y += lineh;
            Font_SetPos(x, y);
            Text_Draw(buf);
        } else if ((nl = strchr(buf, '\n')) != NULL) {
            *nl = 0;
            s = strchr(s, '\n') + 1;
            y += lineh;
            Font_SetPos(x, y);
            Text_Draw(buf);
        } else {
            Font_SetPos(x, y + lineh);
            Text_Draw(buf);
            break;
        }
    }
    return n;
}

/* Seconds as minutes and seconds. */
char *Time_FormatMinSec(u32 t)
{
    u32 s = t % 60;

    sprintf(D_005BA990, TXT(0x00490A10) /* hours and minutes, with their full-width unit marks */, (t - s) / 60, s);
    return D_005BA990;
}

/* Seconds as minutes (within the hour) and seconds. */
char *Time_FormatMinSecInHour(u32 t)
{
    u32 s = t % 60;

    sprintf(D_005BA9B0, TXT(0x00490A20) /* minutes and seconds, with their full-width unit marks */, ((t - s) % 3600) / 60, s);
    return D_005BA9B0;
}

void Gfx_DrawBlackLineFixed(void)
{
    Sprite r = gBlackLineSprite;

    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    Gfx_DrawLine(r.x, r.y, r.w, r.h, 0xFF000000);
}

void Gfx_DrawBlackLine(s32 a, s32 b, s32 c, s32 d)
{
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    Gfx_DrawLine(a, b, c, d, 0xFF000000);
}

s32 Prim_FillGradRect(PrimGradRect *r);
f32 Math_Sin(f32 a);
extern u16 gFrameTick;

/*
 * A pulsing highlight bar centred on x, w wide, from y down h: two gradient halves that
 * fade from transparent at the ends to the colour at the centre, alpha 0xA0 + 48 sin(t).
 * (The game converts x and w to float and back.)
 */
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color)
{
    PrimGradRect r;
    f32 s;
    s32 half;

    Gs_SetState(0x60, 0);
    Gfx_SetBlend(4, 5);
    s = Math_Sin(9.58738e-05f * (u16)((gFrameTick & 0x3F) << 10));
    x = (f32)x;
    w = (f32)w;
    half = w / 2;
    r.x0 = x - half;
    r.y0 = y;
    r.x1 = x;
    r.y1 = r.y0 + h;
    r.color[0] = color & 0xFFFFFF;
    r.color[1] = r.color[0] | (((s8)(48.0f * s) + 0xA0) << 24);
    r.color[2] = r.color[0];
    r.color[3] = r.color[1];
    Prim_FillGradRect(&r);
    r.x0 = r.x1 + half;
    Prim_FillGradRect(&r);
    Gs_SetState(0x60, 0);
}


extern s32 gLoadLettersTex;          /* the letters texture handle */
typedef struct XY16 {
    s16 x, y;
} XY16;

extern s16 gLoadingLetterPos[];        /* per letter: texture u, v, then screen x, y */
extern f32 D_0043C9B0[];        /* 12 steps of the bob */
s32 Gs_GetState(u8 reg);
s32 Overlay_IsLoaded(s32 id);
void Tex_LoadCommon(void);
s32 Tex_Use(s32 n, u32 *handles);

/* Draws the loading letters: ten sprites with a shadow, each bobbing on the wave
 * D_0043C9B0 one step after the other. */
void Loading_DrawLetters(void)
{
    SysSettings *la;
    s32 s6c;
    s32 s63;
    s32 s5e;
    Sprite spr;
    s32 i;
    s32 ph;
    s16 *t;
    s16 x;
    s16 y;

    la = &gSysSettings;
    if (gSysSettings.loadAnimOn != 0) {
        if (gLoadLettersTex == 0) {
            if (Overlay_IsLoaded(OVERLAY_GAME) != 1) {
                Tex_LoadCommon();
            }
            la->loadAnimOn = 0;
        } else {
            la->loadAnimFrame++;
            if (la->loadAnimFrame & 1) {
                la->loadAnimPhase++;
            }
            if (la->loadAnimPhase > 11) {
                la->loadAnimPhase -= 12;
            }
            s6c = Gs_GetState(RS_ZWRITE);
            s63 = Gs_GetState(0x63);
            s5e = Gs_GetState(RS_BLEND);
            Gs_SetState(RS_ZWRITE, 0);
            Tex_Use(1, (u32 *) &gLoadLettersTex);
            Gs_SetState(4, gLoadLettersTex);
            Gs_SetState(0x63, 0);
            Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
            spr.w = 0x1C;
            spr.h = 0x20;
            t = gLoadingLetterPos;
            ph = la->loadAnimPhase;
            for (i = 0; i < 10; i++) {
                spr.u0 = *t++;
                spr.v0 = *t++;
                spr.u1 = spr.u0 + 0x1F;
                spr.v1 = spr.v0 + 0x1F;
                x = *t++;
                y = (f32)*t++ + 4.0f * D_0043C9B0[ph];
                spr.color = 0xFF000000;
                spr.x = x + 2;
                spr.y = y + 2;
                spr.w = 0x1C;
                spr.h = 0x20;
                Sprite_Draw(&spr);
                spr.color = -1;
                spr.x = x;
                spr.y = y;
                spr.w = 0x1C;
                spr.h = 0x20;
                Sprite_Draw(&spr);
                if (i == 2) {
                    ph += 2;
                } else {
                    ph += 1;
                }
                if (ph > 11) {
                    ph -= 12;
                }
            }
            Gs_SetState(RS_ZWRITE, s6c);
            Gs_SetState(0x63, s63);
            Gs_SetState(RS_BLEND, s5e);
            la->loadAnimOn = 0;
        }
    }
}

void Loading_Request(void)
{
    gSysSettings.loadAnimOn = 1;
}

/* Clears both callback lists. */
void Callbacks_Clear(void)
{
    s32 i;

    gCallbackCount = 0;
    for (i = 0; i < 64; i++) {
        gCallbacks[i] = NULL;
        gCallbackFlags[i] = 0;
    }
    LayerCallbacks_Clear();
}

/* Adds a per-frame callback (once). */
void Callbacks_Add(Callback fn)
{
    s32 i;

    for (i = 0; i < gCallbackCount; i++) {
        if (gCallbacks[i] == fn) {
            return;
        }
    }
    if (D_005BA9D0 != 0) {
        Callbacks_Clear();
    }
    if (gCallbackCount != 64) {
        gCallbacks[gCallbackCount] = fn;
        gCallbackFlags[gCallbackCount] = 0;
        gCallbackCount++;
    }
}

/* Runs the per-frame callbacks. */
void Callbacks_Run(void)
{
    Callback *fn;
    s32 n = gCallbackCount;

    if (n != 0) {
        fn = gCallbacks;
        for (; n != 0; n--, fn++) {
            (*fn)();
        }
    }
    FadeA_Draw();
    Loading_DrawLetters();
}

void Scene_LoadOverlay(s32 id)
{
    LoadOverlay(id);
}

void func_0036CCE0(void)
{
}

void EquipPreview_Clear(void)
{
    memset(gEquipPreview, 0, 0xB0);
}

/* Moves this frame's buttons to the previous ones and reads the pads. */
/* FAKEMATCH: the second port's cur words go through pointers set up first (b1, s1) while the prev
 * stores stay absolute; the game may have had an inline helper per port. */
void Pad_UpdateInput(void)
{
    s32 i;
    PadPairS *b;
    PadPairS *b1 = &gPadInput.buttons[1];
    PadPair *s1 = &gPadInput.stickDir[1];

    gPadInput.stickAngle[0].left = 0;
    gPadInput.stickAngle[0].right = 0;
    gPadInput.stickTilt[0].left = 0;
    gPadInput.stickTilt[0].right = 0;
    gPadInput.buttons[0].prev = gPadInput.buttons[0].cur;
    gPadInput.stickDir[0].prev = gPadInput.stickDir[0].cur;
    gPadInput.buttons[0].cur = 0;
    gPadInput.stickDir[0].cur = 0;
    gPadInput.buttons[1].prev = b1->cur;
    b1->cur = 0;
    gPadInput.stickDir[1].prev = s1->cur;
    s1->cur = 0;
    gPadInput.stickAngle[1].left = 0;
    gPadInput.stickAngle[1].right = 0;
    gPadInput.stickTilt[1].left = 0;
    gPadInput.stickTilt[1].right = 0;
    if (gScene.padRead != 0) {
        b = gPadInput.buttons;
        for (i = 0; i < 2; i++, b++) {
            b->cur = Pad_ReadButtons(i);
            Pad_CopyWords(i);
        }
    }
}

/* The buttons of a port: the pad's, or the next replayed ones. */
u16 Pad_ReadButtons(s32 port)
{
    u16 *e;
    u8 *rep = &D_006ADF7A[port];
    u16 *left;
    u16 **cur;
    s32 b;
    u16 k;

    if (*rep == 0) {
        b = gPadState[gScene.padIndex[port]].held;
        if (gScene.heldMask != 0) {
            gScene.heldMask &= b;
            b = (u16)(b & (u16)~gScene.heldMask);
        }
        return b;
    }
    cur = &D_006ADF7C[port];
    e = *cur;
    left = &D_006ADF84[port];
    if (--*left == 0) {
        if (*++e == 0xFFFF) {
            *rep = 0;
            return 0;
        }
        *left = *e;
        e++;
        *cur = e;
    }
    k = *e;
    if ((gPlayers[port].padSwapLR & 1) && (k & 0xC00)) {
        k ^= 0xC00;
    }
    return k;
}

/* Copies a pad's words into gPadInput. */
void Pad_CopyWords(s32 port)
{
    gPadInput.stickDir[port].cur = gPadState[port].dir;
    gPadInput.stickAngle[port].left = gPadState[port].degL;
    gPadInput.stickAngle[port].right = gPadState[port].degR;
    gPadInput.stickTilt[port].left = gPadState[port].magL;
    gPadInput.stickTilt[port].right = gPadState[port].magR;
}

/* Runs the callbacks of one layer. */
void LayerCallbacks_Run(u8 layer)
{
    Callback *fn;
    s32 n;
    u8 *l;

    fn = gLayerCallbackFns;
    n = gLayerCallbackCount;
    if (n != 0) {
        l = gLayerCallbackLayers;
        do {
            if (layer == *l) {
                (*fn)();
            }
            n--;
            fn++;
            l++;
        } while (n != 0);
    }
}

void LayerCallbacks_Clear(void)
{
    s32 i;

    gLayerCallbackCount = 0;
    for (i = 0; i < 64; i++) {
        gLayerCallbackFns[i] = NULL;
        gLayerCallbackLayers[i] = 0;
    }
}

/* Adds a callback to a layer. */
void LayerCallbacks_Add(Callback fn, u8 layer)
{
    if (gLayerCallbackCount != 64) {
        gLayerCallbackFns[gLayerCallbackCount] = fn;
        gLayerCallbackLayers[gLayerCallbackCount] = layer;
        gLayerCallbackCount++;
    }
}

/* Draws a frame, in order. */
void Frame_Draw(void)
{
    gFrameDrawing = 1;
    Gfx_SetTexFilter(0);
    Gfx_SetBlend(4, 5);
    Gs_SetState(0x60, 0);
    Gs_SetState(RS_ZWRITE, 1);
    Font_SetHalfWidth(1);
    AreaSky_Draw();
    StageObj_DrawKind4();
    LayerCallbacks_Run(0);
    LightSet_Apply(1);
    ObjAttach_DrawAll();
    Task_RunList(gDrawOt32);
    Task_RunList(gDrawOt8);
    Task_RunList(gDrawOtSorted);
    LayerCallbacks_Run(1);
    LightSet_Apply(1);
    Obj_DrawAll();
    StageObj_DrawAll();
    Task_RunList(gDrawListStage);
    Sky_Draw();
    Task_RunList(gDrawListSky);
    Area_DrawGlowProps();
    if (gSysSettings.clearCapture == 0x81) {
        ScreenCapture_Take();
    }
    Pause_CaptureScreen();
    Gs_SetState(RS_ZTEST, RS_CMP_ALWAYS);
    Prim_DrawAll();
    Gs_SetState(RS_ZTEST, RS_CMP_LEQUAL);
    Task_RunList(gDrawListText6);
    Gfx_SetTexFilter(1);
    Text_DrawPage(6);
    Text_DrawIcons(6);
    Task_RunList(gDrawListText0a);
    Task_RunList(gDrawListText0b);
    Gfx_SetTexFilter(1);
    Text_DrawPage(0);
    Text_DrawIcons(0);
    Task_RunList(gDrawListText1);
    Gfx_SetTexFilter(1);
    Text_DrawPage(1);
    Text_DrawIcons(1);
    EquipPreview_Draw();
    Task_RunList(gDrawListText2);
    Gfx_SetTexFilter(1);
    Text_DrawPage(2);
    Text_DrawIcons(2);
    Task_RunList(gDrawListText4);
    Gfx_SetTexFilter(1);
    Text_DrawPage(4);
    Text_DrawIcons(4);
    Task_RunList(gDrawOtText3);
    FadeB_Draw();
    Gfx_SetTexFilter(1);
    Text_DrawPage(3);
    Text_DrawIcons(3);
    Task_RunList(gDrawListText5);
    Gfx_SetTexFilter(1);
    Text_DrawPage(5);
    Text_DrawIcons(5);
    Text_Disable();
    Gfx_ResetRenderStates(1);
    gFrameDrawing = 0;
    Text_Reset();
}

/* Draws a frame over a captured screen. */
void Frame_DrawOverCapture(void)
{
    Pause_DrawCapture();
    gFrameDrawing = 1;
    Gfx_SetTexFilter(0);
    Gfx_SetBlend(4, 5);
    Gs_SetState(0x60, 0);
    Gs_SetState(RS_ZWRITE, 1);
    Gs_SetState(RS_ZTEST, RS_CMP_ALWAYS);
    Prim_DrawAll();
    Gs_SetState(RS_ZTEST, RS_CMP_LEQUAL);
    Task_RunList(gDrawListText6);
    Gfx_SetTexFilter(1);
    Text_DrawPage(6);
    Text_DrawIcons(6);
    Task_RunList(gDrawListText0a);
    Task_RunList(gDrawListText0b);
    Gfx_SetTexFilter(1);
    Text_DrawPage(0);
    Text_DrawIcons(0);
    Task_RunList(gDrawListText1);
    Gfx_SetTexFilter(1);
    Text_DrawPage(1);
    Text_DrawIcons(1);
    Task_RunList(gDrawListText2);
    Gfx_SetTexFilter(1);
    Text_DrawPage(2);
    Text_DrawIcons(2);
    Task_RunList(gDrawListText4);
    Gfx_SetTexFilter(1);
    Text_DrawPage(4);
    Text_DrawIcons(4);
    Task_RunList(gDrawOtText3);
    Gfx_SetTexFilter(1);
    Text_DrawPage(3);
    Text_DrawIcons(3);
    Task_RunList(gDrawListText5);
    EquipPreview_Draw();
    Gfx_SetTexFilter(1);
    Text_DrawPage(5);
    Text_DrawIcons(5);
    Text_Disable();
    Gfx_ResetRenderStates(1);
    gFrameDrawing = 0;
    Text_Reset();
    Pause_CaptureScreen();
}

void DrawCb_LocalPlayer(DrawSlot *r)
{
    Player *p = &gPlayers[r->player];

    if (p->active == 0 || p->visible == 0) {
        return;
    }
    Player_DrawLocal(p, 0);
}

void DrawCb_Player(DrawSlot *r)
{
    Player *p = &gPlayers[r->player];

    if (p->active == 0 || p->visible == 0) {
        return;
    }
    Player_Draw(p, 0);
}

void DrawCb_CharMake(DrawSlot *r)
{
    Player *p = &gPlayers[r->player];

    if (p->active == 0 || p->visible == 0) {
        return;
    }
    CharMake_DrawHunter((u8 *)p, 0);
}

/* Draws the captured screen (gReadBuf), setting its alpha once. */
void ScreenCapture_Draw(void)
{
    u32 *w;
    s32 i;
    Sprite r;

    if (gScreenCaptureOk != 0) {
        if ((gScreenCaptureOk & 0x7F) != 2) {
            VramSlot_FreeAll();
            w = (u32 *)gReadBuf;
            if (!(gScreenCaptureOk & 0x80)) {
                FlushCache(0);
                Dma_WaitIdle();
                for (i = 0; i < 0x38000; i++) {
                    *w = (*w & 0xFFFFFF) | 0x80000000;
                    w++;
                }
                gScreenCaptureOk |= 0x80;
            }
        }
        if (ScreenTex_Upload() != 0) {
            Gfx_SetTexFilter(0);
            Gfx_SetBlend(4, 5);
            Gs_SetState(RS_ZWRITE, 0);
            r.x = 0;
            r.y = 0;
            r.w = 0x280;
            r.h = 0x1C0;
            r.color = 0xA0FFFFFF;
            r.u0 = 0;
            r.v0 = 0;
            r.u1 = 0x1FF;
            r.v1 = 0x1BF;
            Sprite_Draw(&r);
            Gfx_SetTexFilter(1);
            Gs_SetState(RS_ZWRITE, 1);
        }
    }
}

void ScreenCapture_Take(void)
{
    ScreenTex_Init(gReadBuf);
    if (ScreenTex_Capture(0) != 0) {
        gSysSettings.clearCapture = 0x80;
        gScreenCaptureOk = 1;
    }
}

void Pause_CaptureScreen(void)
{
    if (Game_IsOnline() != 1 && gScene.pauseState == 1) {
        ScreenTex_Init(gReadBuf);
        if (ScreenTex_Capture(0) == 0) {
            gScene.pauseState = 0;
        } else {
            gScene.pauseState = 2;
        }
    }
}

void Pause_DrawCapture(void)
{
    u32 *w;
    s32 i;
    Sprite r;

    VramSlot_FreeAll();
    if (gScene.pauseState == 2) {
        FlushCache(0);
        Dma_WaitIdle();
        w = (u32 *)gReadBuf;
        for (i = 0; i < 0x38000; i++) {
            *w = (*w & 0xFFFFFF) | 0x80000000;
            w++;
        }
        gScene.pauseState = 3;
    }
    if (ScreenTex_Upload() != 0) {
        Gfx_SetTexFilter(0);
        Gfx_SetBlend(4, 5);
        Gs_SetState(RS_ZWRITE, 0);
        r.x = 0;
        r.y = -1;
        r.w = 0x280;
        r.h = 0x1C0;
        r.color = 0x66FFFFFF;
        r.u0 = 0;
        r.v0 = 0;
        r.u1 = 0x200;
        r.v1 = 0x1C0;
        Sprite_Draw(&r);
        Gs_SetState(RS_ZWRITE, 1);
    }
}

/* Sets the bone matrices of a tree (render states 0x1A + index). */
void Bone_SetMatrices(EmBone *b, BoneRoot *r, s16 tbl, s32 a3)
{
    f32 out[16];
    f32 m[16];
    s16 k = gBoneMtxSlotMaps[tbl][b->index];

    if (k > 0) {
        if (k < 0x40) {
            Mtx_Copy(m, r->bones[k & 0x3F]->local);
        } else {
            EmBone **pb;

            Mtx_Copy(m, b->world);
            pb = (EmBone **)(((k & 0x3F) << 2) + (u32)r + 0xD0);
            Mtx_Copy3x3(m, (*pb)->local);
            Mtx_Mul3x3(m, b->local, (*pb)->local);
        }
    } else {
        Mtx_Identity(m);
        m[12] = b->world[12];
        m[13] = b->world[13];
        m[14] = b->world[14];
    }
    Mtx_Mul(out, b->skin, m);
    Gs_SetState(b->index + 0x1A, (u32)out);
    if (b->child != NULL) {
        Bone_SetMatrices(b->child, r, tbl, a3);
    }
    if (b->sibling != NULL) {
        Bone_SetMatrices(b->sibling, r, tbl, a3);
    }
}

/* As Bone_SetMatrices, with the parent matrix passed down. */
void Bone_SetMatricesRel(EmBone *b, BoneRoot *r, s16 tbl, s32 a3, f32 *parent)
{
    f32 out[16];
    f32 m[16];
    s16 k = gBoneMtxSlotMaps[tbl][b->index];

    if (k > 0) {
        if (k < 0x40) {
            Mtx_Copy(m, r->bones[k & 0x3F]->local);
        } else {
            Mtx_Mul(m, b->local, parent);
        }
    } else {
        Mtx_Identity(m);
        m[12] = b->world[12];
        m[13] = b->world[13];
        m[14] = b->world[14];
    }
    Mtx_Mul(out, b->skin, m);
    Gs_SetState(b->index + 0x1A, (u32)out);
    if (b->child != NULL) {
        Bone_SetMatricesRel(b->child, r, tbl, a3, m);
    }
    if (b->sibling != NULL) {
        Bone_SetMatricesRel(b->sibling, r, tbl, a3, parent);
    }
}

f32 Key1_Eval(KeyClock *c, Key1 *k)
{
    for (;; k++) {
        if (k->t == -1.0f) {
            return k->v;
        }
        if (Motion_ReachedFrame(c, 0, k->t) != 0 && Motion_ReachedFrame(c, 0, k[1].t) == 0) {
            return k->v + (k[1].v - k->v) * ((c->time - k->t) / (k[1].t - k->t));
        }
    }
}

void Key3_Eval(KeyClock *c, Key3 *k, f32 *out)
{
    for (;; k++) {
        if (k->t == -1.0f) {
            out[0] = k->v[0];
            out[1] = k->v[1];
            out[2] = k->v[2];
            return;
        }
        if (Motion_ReachedFrame(c, 0, k->t) != 0 && Motion_ReachedFrame(c, 0, k[1].t) == 0) {
            out[0] = k->v[0] + (k[1].v[0] - k->v[0]) * ((c->time - k->t) / (k[1].t - k->t));
            out[1] = k->v[1] + (k[1].v[1] - k->v[1]) * ((c->time - k->t) / (k[1].t - k->t));
            out[2] = k->v[2] + (k[1].v[2] - k->v[2]) * ((c->time - k->t) / (k[1].t - k->t));
            return;
        }
    }
}

void Key2_Eval(KeyClock *c, Key2 *k, f32 *a, f32 *b)
{
    for (;; k++) {
        if (k->t == -1.0f) {
            *a = k->v[0];
            *b = k->v[1];
            return;
        }
        if (Motion_ReachedFrame(c, 0, k->t) != 0 && Motion_ReachedFrame(c, 0, k[1].t) == 0) {
            *a = k->v[0] + (k[1].v[0] - k->v[0]) * ((c->time - k->t) / (k[1].t - k->t));
            *b = k->v[1] + (k[1].v[1] - k->v[1]) * ((c->time - k->t) / (k[1].t - k->t));
            return;
        }
    }
}

#pragma optimization_level 3
void Key3_EvalByTimer(Player *p, Key3 *k, f32 *out)
{
    f32 x = 10.0f - p->swordGaugeTimer;
    f32 f;
    f32 g;

    for (;; k++) {
        if (k->t == -1.0f) {
            out[0] = k->v[0];
            out[1] = k->v[1];
            out[2] = k->v[2];
            return;
        }
        if (!(x < k->t) && x < k[1].t) {
            /* Each channel checks its key interval after the ratio, an empty check compiled out
             * (docs/mwcc-matching.md, "Statements scheduled as if a check were still there"): without
             * them out[1] and out[2] take other registers. */
            f = (x - k->t) / (k[1].t - k->t);
            if (k[1].t == k->t || x < k->t) { /* compiled-out check */
            }
            g = k[1].v[0] - k->v[0];
            out[0] = k->v[0] + g * f;
            f = (x - k->t) / (k[1].t - k->t);
            if (k[1].t == k->t || x < k->t) { /* compiled-out check */
            }
            g = k[1].v[1] - k->v[1];
            out[1] = k->v[1] + g * f;
            f = (x - k->t) / (k[1].t - k->t);
            if (k[1].t == k->t || x < k->t) { /* compiled-out check */
            }
            g = k[1].v[2] - k->v[2];
            out[2] = k->v[2] + g * f;
            return;
        }
    }
}
#pragma optimization_level reset

/* unk_36BE70's .data (0x43C990-0x43CA30) */
Sprite gBlackLineSprite = {
#include "sub_main/0043C990.inc"
};
f32 D_0043C9B0[12] = {
#include "sub_main/0043C9B0.inc"
};
s16 gLoadingLetterPos[40] = {
#include "sub_main/0043C9E0.inc"
};
