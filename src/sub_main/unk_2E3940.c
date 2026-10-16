/*
 * The start of sub_main: its entry and main loop, system settings, the
 * memory layout and version string, motion frame tests, the text objects
 * and the glyph cache (gFontCache), the model and texture code, matrices,
 * animation curves and math (0x2E3940-0x2EF020; the bones are unk_2EF020.c,
 * another unit: Player_UpdateMotion declares Bone_PosePart and Bone_BlendPart with the
 * float first and u16 ids, which their definitions don't match with). One unit: Font_CreateHandles uses v1 for its temporaries because the
 * static Font_ClearSlots leaves v0 alone, so everything from Font_ClearSlots to
 * Font_CreateHandles is one file; the .data of the old unk_2E3940.c ends where
 * unk_2E6F40.c's starts, and every function matches at level 4.
 */
#include "common.h"
#include "syswork.h"
#include "syssettings.h"
#include "ram_areas.h"
#include "em.h"
#include "model.h"
#include "motion.h"
#include "player.h"
#include "image.h"
#include "dma.h"
#include "texture.h"
#include "padio.h"
#include "syssave.h"
#include "pad.h"

/* The game compiled this unit at optimization level 4 (Vec_RotateX needs it;
 * every other matched function matches either way). */
#pragma optimization_level 4

/* A string to draw with the glyph cache (Text_DrawRun). */
typedef struct TextRun {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    f32 size;                   /* 0x04: the glyph size (the cache's +0x54) */
    u8 advance;                 /* 0x08: pen step per glyph */
    u8 charH;                   /* 0x09: the glyph height (a line on a newline) */
    u8 pal;                     /* 0x0A: a palette of the cache */
    u8 isGlyphs;                /* 0x0B: glyphs holds glyph indices, else a Shift-JIS string */
    s32 clut;                   /* 0x0C */
    s16 *glyphs;                /* 0x10: glyph indices ended by a negative one, or the string */
} TextRun;

/* The glyph cache of the text system (gFontCache). */
typedef struct FontCache {
    s32 font;                   /* 0x0000: the index in gFontDescs */
    s32 unk04;                  /* 0x0004 */
    s32 width;                  /* 0x0008: glyph pixels */
    s32 height;                 /* 0x000C */
    s32 unk10;                  /* 0x0010 */
    s32 unk14;                  /* 0x0014 */
    s32 unk18;                  /* 0x0018 */
    u32 glyphs;                 /* 0x001C: glyphs of the font */
    s32 glyphBytes;             /* 0x0020: bytes per glyph at bits */
    s32 (*palInfo)[4];          /* 0x0024: per palette, 4 words for Font_SetPalette */
    s32 palCount;               /* 0x0028 */
    s32 texCount;               /* 0x002C: pages */
    s32 unk30;                  /* 0x0030 */
    s32 unk34;                  /* 0x0034 */
    s32 unk38;                  /* 0x0038 */
    s32 unk3C;                  /* 0x003C: bytes from D_004B1940 to the end of text */
    s32 loaded;                 /* 0x0040: bits is loaded */
    s32 defAdvance;             /* 0x0044: the font's advance (TextObj_Reset) */
    s32 defCharH;               /* 0x0048: the font cell height (TextObj_Reset) */
    u8 *bits;                   /* 0x004C: the font glyphs, 2 bits a pixel */
    s16 runX;                   /* 0x0050: the position of new runs (Font_SetPos) */
    s16 runY;                   /* 0x0052 */
    f32 size;                   /* 0x0054: of new runs (10.0 by default) */
    s16 x;                      /* 0x0058: the pen */
    s16 y;                      /* 0x005A */
    u8 advance;                 /* 0x005C: of new runs (Font_SetSpacing) */
    u8 charH;                   /* 0x005D: the glyph height of new runs (Font_SetSpacing) */
    u8 unk5E;                   /* 0x005E */
    u8 unk5F;                   /* 0x005F */
    u8 *palPixels;              /* 0x0060: the palettes, 0x40 bytes each */
    s32 runCount[7];            /* 0x0064: per page, its text runs */
    s32 runPrev[7];             /* 0x0080: runCount of the frame before */
    TextRun *runs[7];    /* 0x009C: per page */
    u32 textLen;                /* 0x00B8: bytes used in text (at most 0x8000) */
    char *text;                 /* 0x00BC: the runs' strings */
    s32 halfWidth;              /* 0x00C0: one-byte characters advance half a glyph (else 2/3) */
    s32 runPage;                /* 0x00C4: the page of new runs (Font_SelectPage) */
    s32 page;                   /* 0x00C8: the page being filled */
    s32 pageDone[7];            /* 0x00CC: per page, drawn this frame */
    s32 max;                    /* 0x00E8: slots */
    s32 perPage;                /* 0x00EC: slots per page (texture) */
    f32 scale;                  /* 0x00F0: the display width / 640 */
    s32 clutNew;                /* 0x00F4: of new runs */
    s32 palNew;                 /* 0x00F8: of new runs */
    s32 glyphMode;              /* 0x00FC: new runs take glyph indices, not a string */
    u8 pad100[0x104 - 0x100];
    s16 pageFirst[7];           /* 0x0104: per page, its first slot */
    s16 pageEnd[7];             /* 0x0112: per page, the slot after its last */
    u16 *pageGlyphs[7];         /* 0x0120: per page, the glyph of each slot */
    u16 slot[0x1E80];           /* 0x013C: per glyph (Font_GlyphIndex), its slot or 0xFFFF */
    s32 used;                   /* 0x3E3C: slots in use */
    u32 *tex;                   /* 0x3E40: per page, a texture handle */
    u32 *pal;                   /* 0x3E44: palette handles */
    s32 pulse;                  /* 0x3E48: frames of the colour pulse (Text_UpdatePulse) */
    s32 clut;                   /* 0x3E4C: for runs without a palette of the cache */
    u8 *pktBase;                /* 0x3E50 */
    u8 *pkt;                    /* 0x3E54: the next packet */
    s32 sprites;                /* 0x3E58 */
    u32 key;                    /* 0x3E5C: texture | palette << 16 of the packets */
    u32 keySet;                 /* 0x3E60: also the TEX1 filter bits */
} FontCache;

extern FontCache *gFontCache;

/* prototypes */
void Font_SetPalette(s32 slot, u32 handle, u32 c0, u32 c1, u32 c2, u32 c3);
s32 Gs_SetState(u8 reg, u32 value);
void Callbacks_Clear(void);
void Callbacks_Run(void);
void Game_ResetSubsystems(void);
void LoadOverlay(s32 id);
void Patch_Init(void);
void Sound_ResetEffects(void);
void Sound_ReverbOff(void);
void Text_Reset(void);
void SubMain_Init(void);
void SubMain_BeginFrame(void);
void SysSettings_Reset(void);
void SubMain_ResetSystems(void);
void Gs_SetTableColor(s32 a);
extern s32 gFlipCountdown; /* main */
extern s32 gDispWidth; /* main: display width (divided by 640 for the font scale) */
extern s32 gSysFrameMode; /* main */
s32 Table_GetWordAt8(u8 *base, s32 i);
void func_6B6700(void); /* select overlay: Option_SetDefaults (select.txt) */
void Pad_UpdatePorts(void);
void func_002E4560(void);
void Mem_InitLayout(void);
void Version_Build(void);
void SubMain_OpenLoading(void);
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
s32 Motion_InFrameRange(void *obj, s32 layer, f32 from, f32 to);
void TextObj_Reset(void);
static void Font_ClearSlots(void);
void Font_FlushSlots(void);
void Font_SetPal(s32 a);
void Font_SetHalfWidth(s32 a);
void Text_UpdatePulse(void);
struct EmBone;
s32 Bone_GetAnimMove(f32 *out, struct EmBone *b, f32 t0, f32 t1);
s32 UsbKb_RunInitStep(void);
s32 Net_Start(void);
void Kbd_ReadUsb(void);
void Kbd_InitState(s8);
void func_00319FE0(void);
void Brightness_Draw(void);
void Tex_LoadFile(s32 a, s32 b, s32 c);
void PlayerLook_ResetLoaded(void);
void TaskSlot_InitAll(void);
void Task_RunFrame(void);
void TaskSlot_Set(void *func, s16 slot);
void Phrase_ResetDefaults(void);
void Draw_BeginFrame(void);
void Draw_EndFrame(void);
void FreeList_InitAll(void);
void Text_Init(void);
void MemFile_Load(void);
void Game_ReturnToTitle(void);
s32 SubMain_ResetRequested(void);
void Loading_Stop(void);
void Loading_Open(u32 a, s32 b);
void Loading_Draw(s32);
void Save_ClearCopy(void);
s32 Sys_FlipFrame(s32 unused0);
void Sys_SetNextFlipAt(s32);
void Pad_Update(void);
void Sys_EmptyHook(void);
void Overlay_LoadGmSub(void);
void Stream_Server(void);
void * memset(void *p, s32 c, u32 n);
extern s32 gSubMainStep;
extern s32 gFontRunCount;
extern s8 gElderForecastHeard;
extern u8 gQuestFiles;
extern u8 gShortMailMsg;
extern u8 gMailBox;
extern u8 gChatBlockList;
extern s16 D_005CA910;
extern s8 gOnlineBonusState;
extern u8 gSoundLoading;
extern s32 gSaveBuf;
extern u16 D_006AE190;
extern u16 gPadDebugMode;
extern u8 D_6B5CF0;
void Font_DrawPage(s32 idx);
void Font_BeginPage(void);
void Text_DrawString(TextRun *r);
void Text_DrawRun(TextRun *r);
void Font_EndFrame(void);
f32 Math_Atan(f32 x);
asm f32 Math_Sqrtf(f32 x);
void Mem_Free(void *handle);
void * Mem_GetAddr(void *handle);
void Font_UploadGlyphs(void);
void Font_CachePage(void);
void Gfx_SetBlend(s32 a, s32 b);
f32 acosf(f32 x);
f32 atan2f(f32 y, f32 x);
f32 expf(f32 x);
f32 powf(f32 x, f32 y);
extern s32 gAnimCurveKeys;
extern s32 gAnimData;
/* end of prototypes */

/* unk_2E3940's .rodata (0x46A100-0x46A310) */
#pragma rodata on
char D_0046A100[0x8] = {
#include "sub_main/0046A100.inc"
};
char D_0046A108[0x8] = {
#include "sub_main/0046A108.inc"
};
char D_0046A110[0x8] = {
#include "sub_main/0046A110.inc"
};
char D_0046A118[0x8] = {
#include "sub_main/0046A118.inc"
};
char D_0046A120[0x8] = {
#include "sub_main/0046A120.inc"
};
char D_0046A128[0x8] = {
#include "sub_main/0046A128.inc"
};
char D_0046A130[0x8] = {
#include "sub_main/0046A130.inc"
};
char D_0046A138[0x8] = {
#include "sub_main/0046A138.inc"
};
char D_0046A140[0x8] = {
#include "sub_main/0046A140.inc"
};
char D_0046A148[0x8] = {
#include "sub_main/0046A148.inc"
};
char D_0046A150[0x8] = {
#include "sub_main/0046A150.inc"
};
char D_0046A158[0x8] = {
#include "sub_main/0046A158.inc"
};
char D_0046A160[0x8] = {
#include "sub_main/0046A160.inc"
};
char D_0046A168[0x8] = {
#include "sub_main/0046A168.inc"
};
char D_0046A170[0x8] = {
#include "sub_main/0046A170.inc"
};
char D_0046A178[0xC] = {
#include "sub_main/0046A178.inc"
};
char D_0046A190[0x20] = {
#include "sub_main/0046A190.inc"
};
char D_0046A1B0[0x18] = {
#include "sub_main/0046A1B0.inc"
};
char D_0046A1C8[0x8] = {
#include "sub_main/0046A1C8.inc"
};
char D_0046A1D0[0x10] = {
#include "sub_main/0046A1D0.inc"
};
char D_0046A1E0[0x20] = {
#include "sub_main/0046A1E0.inc"
};
char D_0046A200[272] = {
#include "sub_main/0046A200.inc"
};
#pragma rodata off

s32 SubMain_Step(void);
s32 Game_IsOnline(void);
s32 NetIf_IsConnected(void);
s32 NetIf_PollError(s32 a, s32 b);
extern u16 gFrameTick;
extern s32 *gNetDevCur;
extern s8 gNetLanMode;

extern s32 gSysExiting;
/* The resident overlay's main loop. */
void sub_main_entry(void)
{
    gSubMainStep = 0;
    for (;;) {
        for (;;) {
            if (!(gSysExiting != 0) != 0) {
                break;
            }
            gFlipCountdown = 1;
            Sys_FlipFrame(0);
        }
        SubMain_Step();
        if (Game_IsOnline() != 1) {
            continue;
        }
        if (NetIf_IsConnected() == 0) {
            continue;
        }
        if (gFrameTick & 0x1F) {
            continue;
        }
        NetIf_PollError(*gNetDevCur, gNetLanMode == 1);
    }
}

s32 SubMain_Step(void)
{
    switch (gSubMainStep) {
    case 0:
        SubMain_Init();
        Sys_EmptyHook();
        TaskSlot_Set(&D_6B5CF0, 0);
        gSubMainStep += 1;
        break;
    case 1:
        SubMain_BeginFrame();
        Sys_SetNextFlipAt(1);
        if (SubMain_ResetRequested() != 0) {
            gSubMainStep += 1;
        } else {
            Task_RunFrame();
            Callbacks_Run();
            func_002E4560();
        }
        Sys_FlipFrame(0);
        break;
    case 2:
        Game_ReturnToTitle();
        gSubMainStep -= 1;
        Sys_FlipFrame(0);
        break;
    }
    return 0;
}

void SubMain_Init(void)
{
    Net_Start();
    do {
    } while (UsbKb_RunInitStep() != 0);
    Sys_SetNextFlipAt(1);
    Mem_InitLayout();
    Gs_SetTableColor(0);
    TaskSlot_InitAll();
    Overlay_LoadGmSub();
    SubMain_OpenLoading();
    PlayerLook_ResetLoaded();
    Text_Init();
    Sound_ReverbOff();
    MemFile_Load();
    memset(&gShortMailMsg, 0, 0x92U);
    memset(&gMailBox, 0, 0x490U);
    memset(&gQuestFiles, 0, 0x900U);
    memset(&gChatBlockList, 0, 0x1E0U);
    D_005CA910 = 0;
    gElderForecastHeard = 0;
    gOnlineBonusState = 0;
    Version_Build();
    LoadOverlay(2);
    SubMain_ResetSystems();
    Kbd_InitState((s32) gSysSettings.language);
    Loading_Stop();
}

void SubMain_BeginFrame(void)
{
    gSysSettings.unkBF = 0;
    if (gPadDebugMode != 0) {
        gFlipCountdown = 1;
    }
    Sound_ResetEffects();
    Font_FlushSlots();
    Text_UpdatePulse();
    Pad_Update();
    D_006AE190 = gPadDebugMode;
    Pad_UpdatePorts();
    func_00319FE0();
    Kbd_ReadUsb();
    Text_Reset();
    FreeList_InitAll();
    Callbacks_Clear();
    Draw_BeginFrame();
}

extern s32 gLoadingScreenOn;
void SubMain_EndFrame(void)
{
    if (gLoadingScreenOn != 0) {
        Loading_Draw(1);
    }
    if (gSubMainStep != 0) {
        Brightness_Draw();
    }
    Draw_EndFrame();
    Stream_Server();
}

void Sound_ApplyOutputMode(s32 mode);
void Stream_UpdateMaxVol(s32);
void Display_SetOffset(s32 x, s32 y);

/* Resets the system settings (gSysSettings) and applies the options. */
void SysSettings_Reset(void)
{
    SysSettings *sys;

    gSysSettings.unk1A = 0;
    sys = &gSysSettings;
    gSysSettings.language = 0;
    gSysSettings.pal = 0;
    if (gSysSettings.pal == 0) {
        sys->fps = 59.94f;
    } else {
        sys->fps = 50.0f;
    }
    sys->unk22 = 1;
    sys->busy = 0;
    sys->unk38 = 0;
    sys->unk39 = 5;
    sys->unk3A = 0;
    sys->unk3B = 0;
    sys->soundMode = gSysSave.options.soundMode;
    sys->musicVolume = gSysSave.options.musicVolume;
    sys->seVolume = gSysSave.options.seVolume;
    sys->brightness = gSysSave.options.brightness;
    if (sys->brightness < 0x38 || sys->brightness > 0xA4) {
        sys->brightness = 0x80;
        gSysSave.options.brightness = 0x80;
    }
    sys->unk24 = 0;
    sys->unk14 = 1;
    sys->unk06 = 0;
    sys->cameraPreset = 0xFF;
    sys->unk05 = 0;
    sys->unk31 = 1;
    Sound_ApplyOutputMode(sys->soundMode);
    Stream_UpdateMaxVol(0);
    sys->bootMode = 0;
    sys->taskRequest = 0;
    Display_SetOffset(gSysSave.options.screenX, gSysSave.options.screenY);
    sys->unk42 = 0;
}

void SubMain_ResetSystems(void)
{
    func_6B6700();
    Save_ClearCopy();
    Patch_Init();
    Phrase_ResetDefaults();
    SysSettings_Reset();
    Game_ResetSubsystems();
}

extern u8 gGsColorTable[];

/* Register 0x14 (Gs_SetState) = colour i of the RGB table gGsColorTable, alpha 0xFF. */
void Gs_SetTableColor(s32 i)
{
    u8 *c = &gGsColorTable[i * 3];
    u8 r;
    u8 g;
    u32 v;

    r = *c++;
    g = *c++;
    v = (r << 16) | 0xFF000000;
    v |= g << 8;
    Gs_SetState(0x14, *c | v);
}

extern PadInput *gPadInputPtr;
/* main's pointer to the pads as read (0x24BBE8; main.txt has no name for it yet) */

/* Per-pad update of pad i, emptied in this build: only the test of its state byte
 * (== 1) is left. Static: Pad_UpdatePorts keeps its counter in a2 across it. */
static void Pad_UpdatePort(s32 i)
{
    PadInput *pad = &gPadInputPtr[i];

    if (pad->state == 1) {
        return;
    }
    return;
}

/* Updates both pads (Pad_UpdatePort), then PadState_UpdateAll. */
void PadState_UpdateAll(void);

void Pad_UpdatePorts(void)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        Pad_UpdatePort(i);
    }
    PadState_UpdateAll();
}

u16 Pad_StickDeadZone(u32 x)
{
    if (x < 0x2D) {
        x = 0;
    }
    return x;
}

/* One pad's buttons in the game's layout (gPadState[i], 0x22 bytes). */
extern PadState gPadState[];

/* Updates gPadState[i] from pad i as read: the buttons in the game's bit order, the stick
 * directions, angles (0x10000 a turn) and magnitudes, the press/release edges and the
 * auto-repeat (after 13 frames held, then every 3). */
void PadState_Update(s32 port)
{
    PadInput *pad;
    u32 held;
    PadState *padState;
    u16 bits;

    padState = &gPadState[port];
    padState->onPrev = padState->on;
    pad = &gPadInputPtr[port];
    if (pad->state == 1) {
        padState->on = (pad->kind & 3) != 0;
    } else {
        padState->on = 0;
    }
    padState->unk20 = 0;
    if (gPadInputPtr[port].state == 1) {
        padState->prevHeld = padState->held;
        padState->dirPrev = padState->dir;
        held = gPadInputPtr[port].held;
        bits = 0;
        if (held & PADIO_UP) bits |= PAD_UP;
        if (held & PADIO_DOWN) bits |= PAD_DOWN;
        if (held & PADIO_LEFT) bits |= PAD_LEFT;
        if (held & PADIO_RIGHT) bits |= PAD_RIGHT;
        if (held & PADIO_SQUARE) bits |= PAD_SQUARE;
        if (held & PADIO_TRIANGLE) bits |= PAD_TRIANGLE;
        if (held & PADIO_R1) bits |= PAD_R1;
        if (held & PADIO_CROSS) bits |= PAD_CROSS;
        if (held & PADIO_CIRCLE) bits |= PAD_CIRCLE;
        if (held & PADIO_R2) bits |= PAD_R2;
        if (held & PADIO_L1) bits |= PAD_L1;
        if (held & PADIO_L2) bits |= PAD_L2;
        if (held & PADIO_L3) bits |= PAD_L3;
        if (held & PADIO_R3) bits |= PAD_R3;
        if (held & PADIO_START) bits |= PAD_START;
        if (held & PADIO_SELECT) bits |= PAD_SELECT;
        padState->held = bits;
        bits = 0;
        if (gPadInputPtr[port].caps & 0x30) {
            if (held & (PADIO_UP << PAD_LSTICK_SHIFT)) bits |= PAD_UP;
            if (held & (PADIO_DOWN << PAD_LSTICK_SHIFT)) bits |= PAD_DOWN;
            if (held & (PADIO_LEFT << PAD_LSTICK_SHIFT)) bits |= PAD_LEFT;
            if (held & (PADIO_RIGHT << PAD_LSTICK_SHIFT)) bits |= PAD_RIGHT;
            padState->degL = (gPadInputPtr[port].stickL.deg << 16) / 360;
            padState->magL = Pad_StickDeadZone(gPadInputPtr[port].stickL.mag);
        }
        if (gPadInputPtr[port].caps & 0x50) {
            if (held & (PADIO_UP << PAD_RSTICK_SHIFT)) bits |= PAD_RSTICK_UP;
            if (held & (PADIO_DOWN << PAD_RSTICK_SHIFT)) bits |= PAD_RSTICK_DOWN;
            if (held & (PADIO_LEFT << PAD_RSTICK_SHIFT)) bits |= PAD_RSTICK_LEFT;
            if (held & (PADIO_RIGHT << PAD_RSTICK_SHIFT)) bits |= PAD_RSTICK_RIGHT;
            padState->degR = (gPadInputPtr[port].stickR.deg << 16) / 360;
            padState->magR = Pad_StickDeadZone(gPadInputPtr[port].stickR.mag);
        }
        padState->dir = bits;
    } else {
        padState->held = 0;
        padState->dir = 0;
        padState->degR = 0;
        padState->degL = 0;
        padState->magR = 0;
        padState->magL = 0;
    }
    padState->pressed = ~padState->prevHeld & padState->held;
    padState->released = padState->prevHeld & ~padState->held;
    padState->dirPressed = ~padState->dirPrev & padState->dir;
    padState->dirReleased = padState->dirPrev & ~padState->dir;
    padState->repeat = 0;
    if (padState->prevHeld == padState->held) {
        padState->repeatCnt++;
        if (padState->repeatCnt > 10 && padState->repeatCnt > 12) {
            padState->repeatCnt = 10;
            padState->repeat = padState->held;
        }
    } else {
        padState->repeatCnt = 0;
    }
}

void PadState_Update(s32 i);
extern s32 gPadDebugSwitch;
extern s32 gPadOddFrame;
extern u16 D_006AE0D6;
extern u16 D_006AE0D2;

/* Updates the four slots (PadState_Update); then, with gPadDebugSwitch set, a debug switch toggled
 * by button 0x8000 that sets gPadDebugMode (0 with 0x20, cycled 0-3 with 0x40, else 0x8000). */
void PadState_UpdateAll(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        PadState_Update(i);
    }
    if (gPadDebugSwitch == 0) {
        return;
    }
    if (D_006AE0D6 & 0x8000) {
        gPadOddFrame ^= 1;
    }
    if (gPadOddFrame != 0) {
        if (D_006AE0D6 & 0x20) {
            gPadDebugMode = 0;
        } else if (D_006AE0D2 & 0x40) {
            gPadDebugMode = (u8)(gPadDebugMode + 1) % 4;
        } else {
            gPadDebugMode = 0x8000;
        }
    } else {
        gPadOddFrame = 0;
        gPadDebugMode = 0;
    }
}

void func_002E4560(void)
{
}

void Mem_Fill(void *dst, u8 c, s32 n);
void Tex_ClearIdSlots(void);
extern u32 gTexHandles[];
extern s32 gDrawFrameDepth;
extern s16 gTexPrepLast;
extern s16 gTexPrepLastCount;
extern s16 gTexPrepFrameCount;
extern s32 gPadDebugSwitch;
extern s32 gPadOddFrame;

/* Clears the system settings (keeping +0xB8), lays out the memory areas and clears the
 * texture handles. */
void Mem_InitLayout(void)
{
    s32 keep = gSysSettings.gmSubAddr;
    s32 i;

    memset(&gSysSettings, 0, sizeof(SysSettings));
    gSysSettings.gmSubAddr = keep;
    gSysSettings.subMainLoaded = 1;
    gSysSettings.memBase = (u8 *)MEM_AREAS_ADDR;
    Mem_Fill(gSysSettings.memBase, 0, 0x55E000);
    gSysSettings.selWork = (u8 *)MEM_AREAS_ADDR;
    gSysSettings.unk64 = 0xAE8000;
    gSysSettings.unk68 = 0xAF5200;
    gSysSettings.bones = (void *)0xB09200;
    gSysSettings.models = (struct Model *)0xBD1200;
    gSysSettings.prims = 0xBD5200;
    gSysSettings.extraBuf = (u8 *)0xBDA200;
    gSysSettings.lobbyWorkBuf = 0xBEE200;
    gSysSettings.workArea = (void *)WORK_AREA_ADDR;
    gSysSettings.stageFile = 0xC22200;
    gSysSettings.camData = (u8 *)0xC2A200;
    gSysSettings.dataBuf = (u8 *)ANIM_AREA_ADDR;
    gSysSettings.loadBuf = (u8 *)LOAD_BUF_ADDR;
    gSysSettings.readBuf = (u8 *)0xF08000;
    gSysSettings.saveBuf = (struct SaveData *)0xD60200;
    gSysSettings.unkB4 = 0x27E00;
    gSysSettings.patchArea = 0xAA6780;
    gSysSettings.unkDC = 0x20040;
    gSysSettings.unk80 = gSysSettings.dataBuf;
    gSysSettings.modelPack = gSysSettings.dataBuf;
    gSysSettings.unk7C = gSysSettings.modelPack;
    for (i = 0; i < 371; i++) {
        gTexHandles[i] = 0;
    }
    Tex_ClearIdSlots();
    gDrawFrameDepth = 0;
    gTexPrepLast = -1;
    gTexPrepLastCount = -1;
    gTexPrepFrameCount = 0;
    gPadDebugSwitch = 0;
    gPadOddFrame = 0;
}

typedef struct Delims {
    char s[5];
} Delims;
char *strtok(char *s, char *delim);
char *strcpy(char *dst, const char *src);
s32 strcmp(char *a, char *b);
s32 sprintf(char *buf, char *fmt, ...);
void scePrintf(char *fmt, ...);
s32 strlen(char *s);
extern char D_00402220[];       /* the build date, "Mmm dd yyyy" */
extern char D_00402230[];       /* the build time, "hh:mm:ss" */
extern char *gMonthNames[12];    /* the month names */
extern Delims gVersionDelims;       /* the separators */
extern char D_0046A160[];
extern char D_0046A168[];
extern char D_0046A170[];
extern char D_0046A178[];
extern char D_0046A190[];
extern char D_0046A1B0[];
extern char D_0046A1C8[];
extern char D_0046A1D0[];
extern s8 gVersionString[];

/* Builds the version string gVersionString (yy mm dd hh mm) from the build date and time, and
 * prints it. */
void Version_Build(void)
{
    Delims delim;
    char year[8];
    char month[8];
    char day[8];
    char hour[8];
    char min[8];
    char yy[8];
    char mo[8];
    char dd[8];
    char hh[8];
    char mi[8];
    char date[0x28];
    char time[0x20];
    s32 j;
    s32 i;

    delim = gVersionDelims;
    strcpy(date, D_00402220);
    strcpy(time, D_00402230);
    strcpy(month, strtok(date, delim.s));
    strcpy(day, strtok(NULL, delim.s));
    strcpy(year, strtok(NULL, delim.s));
    strcpy(yy, year + 2);
    for (i = 0; i < 12; i++) {
        if (strcmp(gMonthNames[i], month) == 0) {
            sprintf(mo, D_0046A160, i + 1);
            break;
        }
    }
    if (strlen(day) == 1) {
        sprintf(dd, D_0046A168, day);
    } else {
        sprintf(dd, D_0046A170, day);
    }
    strcpy(hour, strtok(time, delim.s));
    strcpy(min, strtok(NULL, delim.s));
    sprintf(hh, D_0046A170, hour);
    sprintf(mi, D_0046A170, min);
    sprintf((char *)gVersionString, D_0046A178, yy, mo, dd, hh, mi);
    scePrintf(D_0046A190);
    scePrintf(D_0046A1B0);
    for (j = 0; j < 10; j++) {
        scePrintf(D_0046A1C8, gVersionString[j]);
    }
    scePrintf(D_0046A1D0);
    scePrintf(D_0046A190);
}

void SubMain_OpenLoading(void)
{
    gSoundLoading = 1;
    Tex_LoadFile(5, 0x155, 0);
    Loading_Open((u32) gSaveBuf, 1);
    Sys_FlipFrame(0);
    Sys_FlipFrame(0);
}

typedef struct AnimSet {
    u16 flags;
    s16 count;
    f32 start;
    f32 end;
    f32 loop;
    s32 anims;
    s32 base;
    s32 size;
    u8 pad1C[4];
} AnimSet;
extern s32 gCommonMotionGroupFirst[];
extern void *gMotions[];
s32 SysHeap_GetMark(void *mark);
void SysHeap_Release(void *mark);
s32 Anim_LoadSet(AnimSet *out, u8 *buf, void *f);
void *Skeleton_CopyBlock14(u8 *obj);
u8 *Table_GetEntry(u8 *t, s32 n);

void Anim_SetupCommon(void)
{
    s32 mark[2];
    AnimSet set;
    s32 base;
    u8 *buf;
    s32 n;
    s32 g;
    s32 j;
    s32 k;
    u8 *e;
    s32 *starts;
    void **ents;

    ents = gMotions;
    starts = gCommonMotionGroupFirst;
    buf = gSysSettings.loadBuf;
    k = 0;
    for (g = 0, base = 0; g < 10; g++, base += 100) {
        starts[g] = k;
        n = Table_GetWordAt8(buf, g);
        for (j = 0; j < n; j++, k++) {
            e = Table_GetEntry(buf, j + base);
            if (e != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, e);
                e = Skeleton_CopyBlock14((u8 *)&set);
                SysHeap_Release(&mark);
                ents[k] = e;
            } else {
                ents[k] = NULL;
            }
        }
    }
}

extern s32 gPlayerMotionGroupFirst[][8]; /* the first set index of each group, per set */

/*
 * As Anim_SetupCommon for the six groups of set idx: loads each animation set of the table
 * in gLoadBuf into gMotions[710 + idx * 300 ...], the first index of each group into
 * gPlayerMotionGroupFirst[idx].
 */
void Anim_SetupPlayer(s32 idx)
{
    s32 base;
    s32 g;
    s32 j;
    s32 k;
    s32 *first;
    s32 n;
    s32 mark[2];
    u8 *e;
    AnimSet set;
    void **out;
    u8 *buf;

    out = &gMotions[idx * 300 + 710];
    buf = gSysSettings.loadBuf;
    k = 0;
    g = 0;
    first = gPlayerMotionGroupFirst[idx];
    for (base = 0; g < 6; g++, first++, base += 100) {
        *first = k;
        n = Table_GetWordAt8(buf, g);
        for (j = 0; j < n; j++, out++, k++) {
            e = Table_GetEntry(buf, j + base);
            if (e != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, e);
                e = Skeleton_CopyBlock14((u8 *)&set);
                SysHeap_Release(&mark);
                *out = e;
            } else {
                *out = NULL;
            }
        }
    }
}

extern s32 gEmMotionGroupFirst[][8]; /* as gPlayerMotionGroupFirst, for these sets */
u8 EmKind_MotionGroupCount(s16 kind);

/*
 * As Anim_SetupPlayer for the 2 * EmKind_MotionGroupCount(kind) groups of set idx, into
 * gMotions[1910 + idx * 600 ...] and gEmMotionGroupFirst[idx]. The copy p has its own variable,
 * and base = g * 100 is set after the store (MWCC strength-reduces it as in the game).
 */
void Anim_SetupEm(s32 idx, s32 kind)
{
    s32 mark[2];
    AnimSet set;
    u8 *buf;
    s32 k;
    s32 n;
    u8 *e;
    void **out;
    void *p;
    s32 base;
    s32 g;
    s32 j;
    s32 groups;

    out = &gMotions[idx * 600 + 1910];
    buf = gSysSettings.loadBuf;
    groups = EmKind_MotionGroupCount(kind) * 2;
    k = 0;
    for (g = 0; g < groups; g++) {
        gEmMotionGroupFirst[idx][g] = k;
        base = g * 100;
        n = Table_GetWordAt8(buf, g);
        for (j = 0; j < n; j++, out++, k++) {
            e = Table_GetEntry(buf, j + base);
            if (e != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, e);
                p = Skeleton_CopyBlock14((u8 *)&set);
                SysHeap_Release(&mark);
                *out = p;
            } else {
                *out = NULL;
            }
        }
    }
}

/* As Anim_SetupPlayer for the first two groups of the sets Anim_SetupEm loads (into
 * gMotions[1910 + idx * 600 ...] and gEmMotionGroupFirst[idx]). */
void Anim_SetupEmSlot(s32 idx)
{
    s32 base;
    s32 g;
    s32 j;
    s32 k;
    s32 *first;
    s32 n;
    s32 mark[2];
    u8 *e;
    AnimSet set;
    void **out;
    u8 *buf;

    out = &gMotions[idx * 600 + 1910];
    buf = gSysSettings.loadBuf;
    k = 0;
    g = 0;
    first = gEmMotionGroupFirst[idx];
    for (base = 0; g < 2; g++, first++, base += 100) {
        *first = k;
        n = Table_GetWordAt8(buf, g);
        for (j = 0; j < n; j++, out++, k++) {
            e = Table_GetEntry(buf, j + base);
            if (e != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, e);
                e = Skeleton_CopyBlock14((u8 *)&set);
                SysHeap_Release(&mark);
                *out = e;
            } else {
                *out = NULL;
            }
        }
    }
}

/*
 * Entry n of table t: groups of 100 (group n / 100: its count at +0, the offset of its
 * offsets at +4), each an offset from t (-1: none); NULL when there is none. The copy b
 * of t, made first, is the result (v0); t itself walks to the offsets.
 */
u8 *Table_GetEntry(u8 *t, s32 n)
{
    u8 *b = t;
    s32 *g;
    s32 i;
    s32 off;

    g = (s32 *)(t + (n / 100) * 8);
    i = n % 100;
    if (i >= g[0]) {
        return NULL;
    }
    t = (u8 *)((s32 *)t + g[1] / 4);
    off = ((s32 *)t)[i];
    if (off == -1) {
        return NULL;
    }
    b += off;
    return b;
}

s32 Table_GetWordAt8(u8 *base, s32 i)
{
    return *(s32 *)(base + i * 8);
}

void Bone_GetAnimMoveFrom(f32 fparg0, f32 fparg1, f32 *out, EmBone *b)
{
    if (fparg0 < 0.0f) {
        Bone_GetAnimMove(out, b, 0.0f, fparg1 - fparg0);
        return;
    }
    Bone_GetAnimMove(out, b, fparg0, fparg1);
}

/* dst = a * s + b * t (3-vectors). */
void Vec_Blend(f32 *dst, f32 *a, f32 *b, f32 s, f32 t)
{
    dst[0] = a[0] * s + b[0] * t;
    dst[1] = a[1] * s + b[1] * t;
    dst[2] = a[2] * s + b[2] * t;
}

f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
f32 *Mtx_RotateVec(f32 *m, f32 *in, f32 *out);
u32 Player_TestFlag(void *obj, u32 flag);
void Player_ClearFlag(void *obj, u32 flag);

/* Moves p by v turned and scaled by p's rotation and scale (not while flag 0x20000 is set);
 * with flag 0x4000000 set, only clears it. */
void Player_MoveLocal(Player *p, f32 *v)
{
    f32 d[4];
    f32 m[16];
    f32 s[16];

    if (Player_TestFlag(p, 0x4000000)) {
        Player_ClearFlag(p, 0x4000000);
        return;
    }
    Mtx_FromAnglesXYZ(p->rot, m);
    Mtx_Scale(s, p->scale[0], p->scale[1], p->scale[2]);
    Mtx_Mul(m, m, s);
    Mtx_RotateVec(m, v, d);
    if (!Player_TestFlag(p, 0x20000)) {
        p->pos[0] += d[0];
        p->pos[1] += d[1];
        p->pos[2] += d[2];
    }
}

/* unit's own declaration: with the definition's s32 (EmBone *b, s32 h, s32 id) (sub_main/unk_2EF020.c), Player_UpdateMotion: 1 of 311 words differ */
s32 Bone_SetPartAnim(struct EmBone *b, s32 h, u16 id);
void Debug_Message(char *unused0, s32 unused1, s32 unused2, s32 unused3);
extern char D_0046A1E0[];
f32 Motion_GetLength(s32 h);
s32 Anim_GetLoopTime(s32 h, f32 *out);

/* Starts motion id +0x29C[part] on the player's layer part at frame `frame`,
 * directly (n == 0) or blended over |n| + 1 frames (n < 0: backwards). */
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part)
{
    u16 id;
    Model *md;
    void **e;
    s32 r;

    id = p->motionId[part];
    md = p->model;
    r = id % 1000;
    if (p->isEm == 0 || (p->unk01E != 0 && p->unk01F[0] != 0)) {
        if (id >= 1000) {
            e = &gMotions[p->index * 300 + 710];
            e += gPlayerMotionGroupFirst[p->index][r / 100];
        } else {
            e = gMotions;
            e += gCommonMotionGroupFirst[r / 100];
        }
    } else {
        if (id >= 1000) {
            e = &gMotions[p->unk30F[0] * 600 + 1910];
            e += gEmMotionGroupFirst[p->unk30F[0]][r / 100];
        } else {
            Debug_Message(D_0046A1E0, (s16)id, 0, 0);
        }
    }
    if (p->isEm == 0 || p->unk01E != 0) {
        e += r % 100;
    } else {
        e = (void **)((s32 *)e + r % 100);
    }
    if (n != 0) {
        if (n < 0) {
            p->motion[part].blend = -1;
            n = -n;
        } else {
            p->motion[part].blend = 1;
        }
        Bone_SetPartAnim(md->partBones2[0], *(s32 *)e, (u16)part);
        p->motion[part].blendFrameCount = Motion_GetLength(*(s32 *)e);
        p->motion[part].blendLoop = Anim_GetLoopTime(*(s32 *)e, &p->motion[part].blendLoopFrame);
        p->motion[part].blendFrame = frame;
        p->motion[part].blendFrames = n + 1;
        p->motion[part].blendT = p->motion[part].blendStep = 1.0f / (n + 1);
        p->motion[part].blendMotion = (s32 *)e;
    } else {
        Bone_SetPartAnim(md->partBones[0], *(s32 *)e, (u16)part);
        p->motion[part].frameCount = Motion_GetLength(*(s32 *)e);
        p->motion[part].loop = Anim_GetLoopTime(*(s32 *)e, &p->motion[part].loopFrame);
        p->motion[part].frame = frame;
        p->motion[part].blendFrames = 0;
        p->motion[part].blend = 0;
        p->motion[part].blendMotion = (s32 *)e;
    }
    p->motion[part].playing = 1;
    if (part == 0) {
        p->motion[0].move[0] = 0.0f;
        p->motion[0].move[1] = 0.0f;
        p->motion[0].move[2] = 0.0f;
    }
}

/* Sets the animation of part `part` of the player's model from its id in
 * +0x670 (thousands: the set: 0 common (gCommonMotionGroupFirst), else the weapon's
 * (+0xC, gPlayerMotionGroupFirst) or, for the other players' kind, +0x30F's (gEmMotionGroupFirst);
 * hundreds: the group; the rest: the entry). */
void Player_SetPartAnim(Player *p, s32 part)
{
    u16 id;
    Model *md;
    void **e;
    s32 r;

    id = p->partAnimMotion[part];
    md = p->model;
    r = id % 1000;
    if (p->isEm == 0 || (p->unk01E != 0 && p->unk01F[0] != 0)) {
        if (id >= 1000) {
            e = &gMotions[p->index * 300 + 710];
            e += gPlayerMotionGroupFirst[p->index][r / 100];
        } else {
            e = gMotions;
            e += gCommonMotionGroupFirst[r / 100];
        }
    } else {
        if (id >= 1000) {
            e = &gMotions[p->unk30F[0] * 600 + 1910];
            e += gEmMotionGroupFirst[p->unk30F[0]][r / 100];
        } else {
            Debug_Message(D_0046A1E0, (s16)id, 0, 0);
        }
    }
    if (p->isEm == 0 || p->unk01E != 0) {
        e += r % 100;
    } else {
        e = (void **)((s32 *)e + r % 100);
    }
    Bone_SetPartAnim(md->partBones2[0], *(s32 *)e, (u16)part);
}

/* The definitions (unk_2EF020.c) take the float first too (the same registers either way). */
s32 Bone_PosePart(f32 t, struct EmBone *b, u16 id); /* unit's own declaration: the definition takes s32 id; Player_UpdateMotion is 22 words off with it */
s32 Bone_BlendPart(f32 wa, f32 wb, struct EmBone *a, struct EmBone *b, u16 id); /* unit's own declaration: as Bone_PosePart */
f32 Math_Hermite(f32 t, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);

/* Advances the player's motion layers: poses each active layer, blends a layer into
 * its next motion (dir != 0) with the root motion of layer 0, loops or stops a layer at its
 * end (1 when one ended) and blends the part overrides (unk66C, weight unk678 %). */
s32 Player_UpdateMotion(Player *p)
{
    Model *md = p->model;
    EmBone *bones = md->bones;
    u16 i;
    s32 done = 0;
    f32 g;
    f32 h;
    f32 t;
    f32 d[4];
    f32 a[4];
    f32 b[4];

    for (i = 0; i < 4; i++) {
        if (i < p->motionLayerCount) {
            if (p->motion[i].blend != 0) {
                Bone_PosePart(p->motion[i].frame, md->partBones[0], i);
                Bone_PosePart(p->motion[i].blendFrame, md->partBones2[0], i);
                Bone_GetAnimMoveFrom(p->motion[0].frame, p->motion[0].frame + p->motion[0].step, a, md->partBones[0]->child);
                h = Math_Hermite(p->motion[i].blendT, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
                g = 1.0f - h;
                Bone_BlendPart(g, h, md->partBones[0], md->partBones2[0], i);
                if (i == 0) {
                    Bone_GetAnimMoveFrom(p->motion[0].blendFrame, p->motion[0].blendFrame + p->motion[0].step, b, md->partBones2[0]->child);
                    Vec_Blend(d, a, b, g, h);
                    if (p->motion[i].blend > 0) {
                        Player_MoveLocal(p, d);
                    }
                    bones[1].local[12] = 0.0f;
                    bones[1].local[13] = 0.0f;
                    bones[1].local[14] = 0.0f;
                }
                p->motion[i].blendT += p->motion[i].blendStep;
                if (--p->motion[i].blendFrames <= 0.0f) {
                    p->motion[i].blend = 0;
                    Bone_SetPartAnim(md->partBones[0], *p->motion[i].blendMotion, i);
                    p->motion[i].frameCount = p->motion[i].blendFrameCount;
                    p->motion[i].loop = p->motion[i].blendLoop;
                    p->motion[i].loopFrame = p->motion[i].blendLoopFrame;
                    p->motion[i].prevFrame = p->motion[i].blendFrame;
                    p->motion[i].frame = p->motion[i].blendFrame + p->motion[i].step;
                    p->motion[i].blendFrames = 0;
                }
            } else {
                Bone_PosePart(p->motion[i].frame, md->partBones[0], i);
                if (p->partAnimOn[i] != 0) {
                    Player_SetPartAnim(p, i);
                    Bone_PosePart(p->motion[i].frame, md->partBones2[0], i);
                    h = (f32)(u32)p->partAnimWeight[i] / 100.0f;
                    Bone_BlendPart(1.0f - h, h, md->partBones[0], md->partBones2[0], i);
                }
                if (i == 0) {
                    Bone_GetAnimMoveFrom(p->motion[i].frame - p->motion[i].step, p->motion[i].frame, d, md->partBones[0]->child);
                    Player_MoveLocal(p, d);
                    d[0] = bones[1].local[12] - p->motion[0].move[0];
                    d[1] = bones[1].local[13] - p->motion[0].move[1];
                    d[2] = bones[1].local[14] - p->motion[0].move[2];
                    p->motion[0].move[0] = bones[1].local[12];
                    p->motion[0].move[1] = bones[1].local[13];
                    p->motion[0].move[2] = bones[1].local[14];
                    bones[1].local[12] = 0.0f;
                    bones[1].local[13] = 0.0f;
                    bones[1].local[14] = 0.0f;
                }
                p->motion[i].prevFrame = p->motion[i].frame;
                p->motion[i].frame += p->motion[i].step;
                if (!(p->motion[i].frame <= p->motion[i].frameCount)) {
                    if (p->motion[i].loop != 0) {
                        t = p->motion[i].loopFrame;
                        if (!(t < 1.0f)) {
                            t -= 1.0f;
                        }
                        Bone_PosePart(t, md->partBones[0], i);
                        if (i == 0) {
                            p->motion[i].move[0] = bones[1].local[12];
                            p->motion[i].move[1] = bones[1].local[13];
                            p->motion[i].move[2] = bones[1].local[14];
                        }
                        Bone_PosePart(p->motion[i].frameCount, md->partBones[0], i);
                        p->motion[i].frame = p->motion[i].loopFrame;
                        if (i == 0) {
                            bones[1].local[12] = 0.0f;
                            bones[1].local[13] = 0.0f;
                            bones[1].local[14] = 0.0f;
                        }
                        p->motion[i].playing = 0;
                        done = 1;
                    } else {
                        done = 1;
                        p->motion[i].frame = p->motion[i].frameCount;
                        p->motion[i].playing = 0;
                    }
                    if (p->partAnimOn[i] != 0) {
                        Bone_PosePart(p->motion[i].frame, md->partBones[0], i);
                        Player_SetPartAnim(p, i);
                        Bone_PosePart(p->motion[i].frame, md->partBones2[0], i);
                        h = (f32)(u32)p->partAnimWeight[i] / 100.0f;
                        Bone_BlendPart(1.0f - h, h, md->partBones[0], md->partBones2[0], i);
                    }
                } else {
                    p->motion[i].playing = 1;
                }
            }
        }
    }
    return done;
}

/* The motion layers of an animated object (a player or a monster: both from +0x154). */
typedef struct MotionObj {
    u8 unk000[0x154];
    MotionLayer motion[4];      /* 0x154 */
} MotionObj;

/* 1 when the motion of layer passes frame during this step (frame to frame + speed,
 * wrapping to the loop frame at the end of a looping motion); 0 while motions are held. */
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame)
{
    Player *p = obj;
    MotionObj *o = (MotionObj *)p;
    s32 wrapped = 0;
    f32 next;
    f32 cur;
    f32 end;

    if (o->motion[0].blend != 0) {
        return 0;
    }
    cur = o->motion[layer].frame;
    end = o->motion[layer].frameCount;
    next = cur + o->motion[layer].step;
    if (!(next <= end)) {
        if (o->motion[layer].loop) {
            wrapped = 1;
            next = o->motion[layer].loopFrame + (next - end);
        } else {
            next = end;
        }
    }
    if (wrapped == 0) {
        if (!(frame < cur) && frame < next) {
            return 1;
        }
    } else {
        if (!(frame < cur) && next < end) {
            return 1;
        }
    }
    return 0;
}

s32 Player_PassedFrameScaled(void *obj, f32 frame, s32 layer)
{
    Player *p = obj;
    f32 k;

    k = p->resist[3];
    if (k != 1.0f && frame == 2.0f) {
        frame *= k;
    }
    return Motion_PassedFrame(p, layer, frame);
}

/* 1 once the motion of layer has reached frame (0 while motions are held). */
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame)
{
    MotionObj *o = obj;

    if (o->motion[0].blend != 0) {
        return 0;
    }
    if (frame <= o->motion[layer].frame) {
        return 1;
    }
    return 0;
}

/* Motion_ReachedFrame for the monsters. */
s32 EmMotion_ReachedFrame(void *em, s32 layer, f32 frame)
{
    return Motion_ReachedFrame(em, layer, frame);
}

/* 1 while the motion of layer is at or past from but before to. */
s32 Motion_InFrameRange(void *obj, s32 layer, f32 from, f32 to)
{
    if (Motion_ReachedFrame(obj, layer, from) != 0 && Motion_ReachedFrame(obj, layer, to) == 0) {
        return 1;
    }
    return 0;
}

/* Motion_InFrameRange for the monsters. */
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to)
{
    return Motion_InFrameRange(em, layer, from, to);
}

/* A font of gFontDescs (0x3C bytes). */
typedef struct FontDesc {
    s32 unk00;
    s32 width;
    s32 height;
    s32 unk0C;
    s32 unk10;
    s32 unk14;
    s32 palInfo;
    s32 palCount;
    s32 texCount;
    s32 glyphs;
    s32 glyphBytes;
    s32 bitsSize;
    s32 unk30;
    s32 unk34;
    s32 unk38;
} FontDesc;

extern FontDesc gFontDescs[];
extern u8 D_004B1940[];
s32 LoadCompressedFile(u8 *dst, s32 id);
void Font_ResetState(void);
void Font_CreateHandles(void);

/* Sets up the font cache (gFontCache) in D_004B1940: the font descriptor's fields, the page glyph
 * lists, the texture and palette tables, the glyph bits and the text runs; loads the glyph bits
 * (file 0x1712) once. Round 21: bits rounded up in two steps (in one, every temporary moves). */
void Font_Init(s32 unused0)
{
    FontDesc *d;
    s32 bits;
    s32 i;
    u8 *base;
    u8 *p;

    base = D_004B1940;

    p = (u8 *)(((u32)base + 0x3F) & ~0x3F);
    gFontCache = (FontCache *)p;
    memset(gFontCache, 0, sizeof(FontCache));
    p += 0x3E80;
    gFontCache->font = 0;
    d = gFontDescs;
    d += gFontCache->font;
    gFontCache->unk04 = d->unk00;
    gFontCache->width = d->width;
    gFontCache->height = d->height;
    gFontCache->unk10 = d->unk0C;
    gFontCache->unk14 = d->unk10;
    gFontCache->unk18 = d->unk14;
    gFontCache->glyphs = d->glyphs;
    gFontCache->glyphBytes = d->glyphBytes;
    gFontCache->palInfo = (void *)d->palInfo;
    gFontCache->palCount = d->palCount;
    gFontCache->texCount = d->texCount;
    gFontCache->unk30 = d->unk30;
    gFontCache->unk34 = d->unk34;
    gFontCache->unk38 = d->unk38;
    bits = d->bitsSize + 0x3F;
    bits &= ~0x3F;
    gFontCache->perPage = (0x100U / gFontCache->width) * (0x100U / gFontCache->height);
    gFontCache->max = gFontCache->perPage * gFontCache->texCount;
    for (i = 0; i < 7; i++) {
        gFontCache->pageGlyphs[i] = (u16 *)p;
        p += gFontCache->max * 2;
    }
    p = (u8 *)(((u32)p + 0x3F) & ~0x3F);
    gFontCache->tex = (u32 *)p;
    p += gFontCache->texCount * 4;
    gFontCache->pal = (u32 *)p;
    p += gFontCache->texCount * 4;
    p = (u8 *)(((u32)p + 0x3F) & ~0x3F);
    gFontCache->palPixels = p;
    p += ((gFontCache->palCount << 6) + 0x3F) & ~0x3F;
    gFontCache->bits = p;
    p += bits;
    gFontCache->runs[0] = (TextRun *)p;
    gFontCache->runs[1] = (TextRun *)(p + 0xA00);
    gFontCache->runs[2] = (TextRun *)(p + 0x1400);
    gFontCache->runs[3] = (TextRun *)(p + 0x1E00);
    gFontCache->runs[4] = (TextRun *)(p + 0x2800);
    gFontCache->runs[5] = (TextRun *)(p + 0x3200);
    gFontCache->runs[6] = (TextRun *)(p + 0x3C00);
    gFontCache->text = (char *)(p + 0x4600);
    gFontCache->defAdvance = gFontCache->width;
    gFontCache->defCharH = gFontCache->height;
    gFontCache->unk3C = (s32)p + 0x4600 + 0x8000 - (s32)base;
    Font_CreateHandles();
    if (gFontCache->loaded == 0) {
        LoadCompressedFile(gFontCache->bits, 0x1712);
        gFontCache->loaded = 1;
    }
    gFontCache->scale = (f32)gDispWidth / 640.0f;
    Font_ResetState();
}

static void Font_SelectPage(void);

/* Resets the text object gFontCache: the 7 layers' slots (as Font_ClearPage), the position,
 * the size (10.0), the colours from +0x44/+0x48; then the size index (Font_SelectPage). */
void TextObj_Reset(void)
{
    s32 i;

    for (i = 0; i < 7; i++) {
        gFontCache->runCount[i] = 0;
        gFontCache->runPrev[i] = 0;
        gFontCache->pageDone[i] = 0;
    }
    gFontCache->runX = 0;
    gFontCache->runY = 0;
    gFontCache->size = 10.0f;
    gFontCache->advance = gFontCache->defAdvance;
    gFontCache->charH = gFontCache->defCharH;
    gFontCache->textLen = 0;
    gFontCache->halfWidth = 1;
    Font_SelectPage();
}

void Font_ClearPage(s32 layer)
{
    s32 ofs;

    ofs = layer * 4;
    *(s32 *)((u8 *)(ofs + (s32)gFontCache) + 0x64) = 0;
    *(s32 *)((u8 *)(ofs + (s32)gFontCache) + 0x80) = 0;
    *(s32 *)((u8 *)(ofs + (s32)gFontCache) + 0xCC) = 0;
}

static void Font_ClearSlots(void)
{
    s32 i;
    u16 *s;

    gFontCache->used = 0;
    s = gFontCache->slot;
    for (i = 0; i < 0x1E80; i++) {
        *s++ = 0xFFFF;
    }
    for (i = 0; i < 7; i++) {
        gFontCache->pageFirst[i] = 0;
        gFontCache->pageEnd[i] = 0;
        gFontCache->pageDone[i] = 0;
    }
}

void Font_FlushSlots(void)
{
    Font_ClearSlots();
    gFontRunCount = 0;
}

void Font_ResetState(void)
{
    TextObj_Reset();
    Font_SetHalfWidth(1);
}

/* Sets the four colours of palette slot `slot`: in gFontCache's table and in the locked palette. */
void Font_SetPalette(s32 slot, u32 handle, u32 c0, u32 c1, u32 c2, u32 c3)
{
    u32 c[4];
    Image img;
    u32 *d;
    u32 *p;
    s32 i;

    if (handle == 0) {
        return;
    }
    c[0] = c0;
    c[1] = c1;
    c[2] = c2;
    c[3] = c3;
    flLockPalette(NULL, handle, &img, 2);
    d = (u32 *)(gFontCache->palPixels + slot * 64);
    p = (u32 *)img.pixels;
    for (i = 0; i < 4; i++) {
        *d++ = c[i];
        *p++ = c[i];
    }
    flUnlockPalette(handle);
}

void Font_SetSpacing(u8 a, u8 b)
{
    gFontCache->advance = (s8) a;
    gFontCache->charH = (s8) b;
}

static void Font_SelectPage(void)
{
    s32 page;

    page = (s32) (7.0f * (gFontCache->size / 1000.0f));
    if (page < 0) {
        page = 0;
    }
    if (page >= 7) {
        page = 6;
    }
    gFontCache->runPage = page;
}

void Font_SetPos(s16 x, s16 y)
{
    gFontCache->runX = x;
    gFontCache->runY = y;
    Font_SelectPage();
}

/* Static: Font_NewRun keeps a2 across the call. */
static void Font_NextPage(s32 unused0, s32 unused1, s32 unused2)
{
    s32 page;

    page = gFontCache->runPage + 1;
    if (page < 0) {
        page = 0;
    }
    if (page >= 7) {
        page = 6;
    }
    gFontCache->runPage = page;
}

void Font_SetSize(f32 size)
{
    gFontCache->size = size;
    Font_SelectPage();
}

void Font_SetColor(s32 color)
{
    Font_SetPal(0);
    gFontCache->clutNew = (s32) ((color & 0xFF00) | (((color & 0xFF) << 0x10) | ((u32) (color & 0xFF0000) >> 0x10)) | 0x80000000);
}

static void Font_SetColorRaw(s32 rgb)
{
    gFontCache->clutNew = (s32) ((rgb & 0xFF00) | (((rgb & 0xFF) << 0x10) | ((u32) (rgb & 0xFF0000) >> 0x10)) | 0x80000000);
}

/*
 * Sets the text colour: -1 the default (gTextPulseColor), -2 and -3 modes 2 and 3 with colour 0,
 * 0-15 colour c of gFontColors in mode 0. Font_SetColorRaw is static (only this file calls
 * it): MWCC then knows it leaves v0 alone and keeps v0 out of the index, as in the game.
 */
extern u32 gTextPulseColor;
extern u32 gFontColors[];

void Font_SetColorIndex(s16 c)
{
    s16 mode;

    if (c >= 0x10) {
        return;
    }
    switch (c) {
    case -1:
        Font_SetPal(0);
        Font_SetColor(gTextPulseColor);
        return;
    case -2:
        mode = 2;
        c = 0;
        break;
    case -3:
        mode = 3;
        c = 0;
        break;
    default:
        if (c >= 0x10) {
            return;
        }
        mode = 0;
        break;
    }
    Font_SetPal(mode);
    Font_SetColorRaw(gFontColors[c]);
}

void Font_SetPal(s32 a)
{
    gFontCache->palNew = a;
}

s32 Font_GetPal(void)
{
    return gFontCache->palNew;
}

u32 Font_GetColor(void)
{
    s32 clutNew;

    clutNew = gFontCache->clutNew;
    return ((clutNew & 0xFF) << 0x10) | ((clutNew & 0xFF00FF00) | ((u32) (clutNew & 0xFF0000) >> 0x10));
}

void Font_SetHalfWidth(s32 a)
{
    gFontCache->halfWidth = 1;
}

/* The glyph cell (20 bytes) for the current size of the text object gFontCache: its slot
 * (+0x64[size]) in the size's cell table (+0x9C[size]); while the slot holds 0x80 or
 * more, the next size is tried (Font_NextPage); NULL when the size stops changing. */
u8 *Font_NewRun(void)
{
    s32 *size;
    u8 *t;
    s32 k;
    s32 s;

    for (;;) {
        size = (s32 *)((u8 *)gFontCache + 0xC4);
        t = (u8 *)gFontCache + *size * 4;
        k = *(s32 *)(t + 0x64);
        if (k < 0x80) {
            break;
        }
        gFontRunCount++;
        s = *size;
        Font_NextPage((s32)size, k, s);
        if (s == gFontCache->runPage) {
            return NULL;
        }
    }
    return *(u8 **)(t + 0x9C) + k * 20;
}

/* Getters and setters of the glyph cache's fields; strlen. */
s32 Font_GetPage(void)
{
    return gFontCache->runPage;
}

s16 Font_GetX(void)
{
    return gFontCache->runX;
}

s16 Font_GetY(void)
{
    return gFontCache->runY;
}

s16 Font_GetAdvance(void)
{
    return gFontCache->advance;
}

s16 Font_GetCharH(void)
{
    return gFontCache->charH;
}

s32 Font_GetHalfWidth(void)
{
    return gFontCache->halfWidth;
}

s32 FontCache_GetUnk5E(void)
{
    return gFontCache->unk5E;
}

s32 FontCache_GetUnk5F(void)
{
    return (s16)gFontCache->unk5F;
}

void FontCache_SetUnk5E(u8 v)
{
    gFontCache->unk5E = v;
}

void FontCache_SetUnk5F(s32 v)
{
    gFontCache->unk5F = v;
}

s32 Str_Length(char *s)
{
    return strlen(s);
}

#include "ee_stdarg.h"
s32 vsprintf(char *buf, const char *fmt, va_list ap);

/* Adds a text run with the cache's current position, size, colours and
 * palette: a printf into the text buffer, or (glyph mode) a copy of the
 * glyph indices up to the negative one that ends them. */
void Font_Printf(char *fmt, ...)
{
    TextRun *r;
    va_list ap;
    s16 *s;
    s16 *d;

    r = (TextRun *)Font_NewRun();
    if (r != NULL) {
        gFontCache->runCount[gFontCache->runPage]++;
        r->x = gFontCache->runX;
        r->y = gFontCache->runY;
        r->size = gFontCache->size;
        r->advance = gFontCache->advance;
        r->charH = gFontCache->charH;
        r->clut = gFontCache->clutNew;
        r->pal = gFontCache->palNew;
        r->isGlyphs = gFontCache->glyphMode;
        if (gFontCache->glyphMode == 0) {
            r->glyphs = (s16 *)(gFontCache->text + gFontCache->textLen);
            va_start(ap, fmt);
            gFontCache->textLen += vsprintf((char *)r->glyphs, fmt, ap);
            if (gFontCache->textLen >= 0x8000) {
                gFontCache->textLen = 0x7FFF;
            }
            gFontCache->text[gFontCache->textLen] = 0;
            gFontCache->textLen++;
        } else {
            gFontCache->textLen = (gFontCache->textLen + 1) & ~1;
            r->glyphs = (s16 *)(gFontCache->text + gFontCache->textLen);
            d = r->glyphs;
            s = (s16 *)fmt;
            for (;;) {
                *d = *s;
                gFontCache->textLen += 2;
                if (*s < 0) {
                    break;
                }
                d++;
                s++;
            }
            gFontCache->text[gFontCache->textLen] = 0;
            gFontCache->textLen++;
        }
    }
}

/* Draws the text runs of page idx once a frame; all pages; the frame's end. */
void Font_DrawPage(s32 idx)
{
    TextRun *e;
    s32 k;

    if (idx < 7) {
        if (gFontCache->pageDone[idx] == 0 && gFontCache->runCount[idx] != 0) {
            gFontCache->page = idx;
            Font_BeginPage();
            e = gFontCache->runs[idx];
            for (k = 0; k < gFontCache->runCount[idx]; k++, e++) {
                if (e->isGlyphs == 0) {
                    Text_DrawString(e);
                } else {
                    Text_DrawRun(e);
                }
            }
            Font_EndFrame();
            gFontCache->pageDone[idx] = 1;
        }
    }
}

void Font_DrawAllPages(void)
{
    s32 i;

    for (i = 0; i < 7; i++) {
        Font_DrawPage(i);
    }
}

void Font_DrawAndClearPages(void)
{
    s32 i;

    for (i = 0; i < 7; i++) {
        Font_DrawPage(i);
        gFontCache->pageDone[i] = 0;
    }
    for (i = 0; i < 7; i++) {
        gFontCache->runPrev[i] = gFontCache->runCount[i];
        gFontCache->runCount[i] = 0;
    }
}

extern Image gFontImage;
extern Image gFontClutImage;

/* Creates the glyph cache texture handles (one per page) and palette handles. */
void Font_CreateHandles(void)
{
    s32 i;
    u32 *h;
    s32 (*info)[4];
    u8 *pixels;
    u32 *p;
    u32 j;
    u32 ph;

    h = gFontCache->tex;
    for (i = 0; i < gFontCache->texCount; i++) {
        *h++ = flCreateTextureHandle(&gFontImage, 4);
    }
    p = gFontCache->pal;
    info = gFontCache->palInfo;
    pixels = gFontCache->palPixels;
    for (j = 0; j < gFontCache->palCount; j++) {
        gFontClutImage.pixels = pixels;
        ph = flCreatePaletteHandle(&gFontClutImage, 4);
        *p++ = ph;
        Font_SetPalette(j, ph, (*info)[0], (*info)[1], (*info)[2], (*info)[3]);
        info++;
        pixels += 0x40;
    }
    Font_ClearSlots();
}

void Font_DrawGlyph(s32 glyph, TextRun *r);
s32 Font_GlyphIndex(u32 code);

s32 Sjis_FromAscii(u8 c);

/* Draws the Shift-JIS string of run r (newlines go down by r->charH). */
void Text_DrawString(TextRun *r)
{
    u8 *s = (u8 *)r->glyphs;
    u32 c;
    s32 narrow;
    s32 k;

    gFontCache->x = r->x;
    gFontCache->y = r->y;
    for (;;) {
        c = *s++;
        if (c == 0) {
            break;
        }
        if (c == '\n') {
            gFontCache->x = r->x;
            gFontCache->y += r->charH;
            continue;
        }
        if ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c < 0x100)) {
            if (*s == 0) {
                break;
            }
            c <<= 8;
            c |= *s++;
            narrow = 0;
        } else {
            c = Sjis_FromAscii(c);
            narrow = 1;
        }
        if (c != 0x8140) {
            k = Font_GlyphIndex(c);
            if (k >= 0) {
                Font_DrawGlyph(k, r);
            }
        }
        if (!narrow) {
            gFontCache->x += r->advance;
        } else if (gFontCache->halfWidth != 0) {
            gFontCache->x += r->advance / 2;
        } else {
            gFontCache->x += r->advance * 2 / 3;
        }
    }
}


/* Draws the glyphs of run r from its position, one advance apart (Font_DrawGlyph). */
void Text_DrawRun(TextRun *r)
{
    s16 *g = r->glyphs;
    s32 c;

    gFontCache->x = r->x;
    gFontCache->y = r->y;
    for (;;) {
        c = *g++;
        if (c < 0) {
            break;
        }
        if ((u32)c < gFontCache->glyphs) {
            Font_DrawGlyph(c, r);
        }
        gFontCache->x += r->advance;
    }
}

/* 2 for a Shift-JIS lead byte (0x80-0x9F, 0xE0-0xFF), 1 otherwise. */
s32 Sjis_CharBytes(u8 *s)
{
    s32 c = *s;

    if ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c < 0x100)) {
        return 2;
    }
    return 1;
}

/* The GS TEX0 register (Sony's SCE_GS_SET_TEX0 layout). */
#define GS_TEX0(tbp, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld)                     \
    ((u64)(tbp) | ((u64)(tbw) << 14) | ((u64)(psm) << 20) | ((u64)(tw) << 26) |                 \
     ((u64)(th) << 30) | ((u64)(tcc) << 34) | ((u64)(tfx) << 35) | ((u64)(cbp) << 37) |         \
     ((u64)(cpsm) << 51) | ((u64)(csm) << 55) | ((u64)(csa) << 56) | ((u64)(cld) << 61))

extern u32 gPacketEnd;
/*
 * Queues the GS registers (TEX0_1, TEX1_1) of glyph texture key (texture | palette << 16).
 * Static: only Font_DrawGlyph calls it, and keeps its values in t7-t9 across the call.
 */
static void Font_SetTexRegs(u32 key)
{
    u32 *p;
    TexRec *t;
    TexRec *c;

    t = &gTexRecs[(key & 0xFFFF) - 1];
    c = &gPalRecs[((key & 0xFFFF0000) >> 16) - 1];
    p = (u32 *)gFontCache->pkt;

    if (gPacketEnd - (u32)p < 0x40) {
        return;
    }
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0x50000003;
    ((u64 *)p)[2] = 0x1000000000008002;
    ((u64 *)p)[3] = 0xE;
    ((u64 *)p)[4] = GS_TEX0(t->vram, 4, t->psm, t->tw, t->th, 1, 0, c->vram, c->psm, 0, 0, 2);
    ((u64 *)p)[5] = 6;
    ((u64 *)p)[6] = ((u64)gFontCache->keySet << 5) | ((u64)gFontCache->keySet << 6);
    ((u64 *)p)[7] = 0x14;
    gFontCache->pkt += 0x40;
}

extern s32 gDispHeight; /* main */

/* Adds the sprite of the glyph in cache slot slot, w x h from the pen, to the font packets
 * (clut is the TEX0 palette word). Static, as Font_DrawGlyph (its only caller) needs: MWCC
 * then knows which registers it clobbers.
 * Round 30: the far texel corner in place (`tw = u + tw - 1; v += th;`: the game keeps
 * v + th in v's own register), the screen offsets added in place after the frame-mode
 * halving (`x0 += ox;` ...: the game's adds are in x0/x1/y0/y1's own registers), y0 a
 * copy of y scaled in place (`y0 = y; ... y0 *= 16;`).
 * Earlier rounds: scale read before x0, slot % perPage assigned back to slot (the game's
 * mfhi a0), x s16 (the game extends it again for x + w), y an int cast back to s16 for
 * y + h (the game extends y again after x1's conversion branch).
 */
static void Font_AddGlyphSprite(s32 slot, u32 clut, s16 w, s16 h)
{
    u32 *p;
    u32 u;
    u32 v;
    u32 x0;
    u32 x1;
    u32 y0;
    s32 ox;
    u32 y1;
    s32 oy;
    u32 tw;
    u32 th;
    s16 x;
    s32 y;
    f32 scale;

    if (gPacketEnd - (u32)gFontCache->pkt < 0x50) {
        return;
    }
    tw = gFontCache->width;
    th = gFontCache->height;
    gFontCache->sprites++;
    p = (u32 *)gFontCache->pkt;
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0x50000004;
    ((u64 *)p)[2] = 0x8001 | ((u64)0x64000000 << 32);
    ((u64 *)p)[3] = 0x535310;
    ((u64 *)p)[4] = 0x156;
    ((u64 *)p)[5] = clut;
    slot = slot % gFontCache->perPage;
    u = tw * (slot % (0x100 / tw));
    v = th * (slot / (0x100 / th));
    ((u64 *)p)[6] = (u32)(u * 16 + 8) | ((u64)(u32)(v * 16 + 8) << 16);
    ox = ((0x1000 - gDispWidth) / 2) * 16;
    oy = ((0x1000 - gDispHeight) / 2) * 16;
    x = gFontCache->x;
    scale = gFontCache->scale;
    x0 = 16.0f * (x * scale);
    y = gFontCache->y;
    x1 = 16.0f * (scale * (x + w));
    y0 = y;
    y1 = (s16)y + h;
    y0 *= 16;
    y1 *= 16;
    if (gSysFrameMode == 0) {
        y0 >>= 1;
        y1 >>= 1;
    }
    x0 += ox;
    x1 += ox;
    y0 += oy;
    y1 += oy;
    tw = u + tw - 1;
    v += th;
    ((u64 *)p)[7] = (u32)x0 | ((u64)(u32)y0 << 16) | ((u64)0xFFFFFFFF << 32);
    ((u64 *)p)[8] = (u32)(tw * 16 + 8) | ((u64)(u32)(v * 16 + 8) << 16);
    ((u64 *)p)[9] = (u32)x1 | ((u64)(u32)y1 << 16) | ((u64)0xFFFFFFFF << 32);
    gFontCache->pkt += 0x50;
}

/*
 * Draws glyph c of run r from the glyph cache: switches the texture and palette when
 * they change (Font_SetTexRegs), then the sprite (Font_AddGlyphSprite).
 * Its registers come from Font_AddGlyphSprite being static C defined before it: MWCC then
 * knows that it leaves v0 alone, keeps v0 live to the end and puts slot, clut and r in
 * t7-t9 as the game does. slot is an s32 (the game passes the lhu value without a mask).
 */
void Font_DrawGlyph(s32 glyph, TextRun *r)
{
    s32 clut;
    u32 key;
    s32 slot = gFontCache->slot[glyph];

    if (slot == 0xFFFF) {
        return;
    }
    if (r->pal < 0) {
        clut = gFontCache->clut;
    } else {
        clut = r->clut;
    }
    key = gFontCache->tex[slot / gFontCache->perPage];
    key |= gFontCache->pal[(u8)r->pal] << 16;
    if (key != gFontCache->key || gFontCache->keySet != 1) {
        gFontCache->key = key;
        gFontCache->keySet = 1;
        Font_SetTexRegs(key);
    }
    Font_AddGlyphSprite(slot, clut, r->advance, r->charH);
}

extern s32 gPacketCur;
void Font_BeginPage(void)
{
    s32 pkt;

    Gs_SetState(0x61, 0x01000000);
    Gs_SetState(0x60, 0);
    Gs_SetState(0x5F, 5);
    gSysSettings.blendA = 0xFF;
    gSysSettings.blendB = 0xFF;
    Gfx_SetBlend(4, 5);
    Gs_SetState(0x6C, 0);
    *(s32 *)((u8 *)gFontCache + 0x100) = -1;
    Font_CachePage();
    Font_UploadGlyphs();
    gFontCache->pktBase = (u8 *)((gPacketCur + 0xF) & ~0xF);
    gFontCache->pkt = gFontCache->pktBase;
    pkt = (s32)gFontCache->pkt;
    if ((u32) (*(s32 *)&gPacketEnd - pkt) >= 0x10U) {
        gFontCache->pkt = (u8 *)(pkt + 0x10);
        gFontCache->sprites = 0;
        gFontCache->key = 0;
        gFontCache->keySet = -1;
    }
}

void *Packet_Alloc(u32 size, s32 align);
s32 DmaQueue_AddChain(s32 mode, u32 entry, u32 *last, DmaQueue *q);
extern DmaQueue gDmaQueue;

/*
 * Ends the glyph cache's frame: closes its packet (a CNT tag over the glyphs, an END tag)
 * and queues it, resets two render states, then evicts the cache's textures and palettes.
 */
void Font_EndFrame(void)
{
    u32 *base;
    u32 *end;
    s32 i;
    u32 *h;
    u32 *p;
    s32 j;

    if (gFontCache->pktBase < gFontCache->pkt) {
        base = (u32 *)gFontCache->pktBase;
        base[0] = (((u32)(gFontCache->pkt - (u8 *)base) >> 4) - 1) | 0x10000000;
        base[1] = 0;
        base[2] = 0;
        base[3] = 0;
        end = (u32 *)gFontCache->pkt;
        end[0] = 0xF0000001;
        end[1] = 0;
        end[2] = 0;
        end[3] = 0;
        *(u128 *)&end[4] = 0;
        gFontCache->pkt += 0x20;
        Packet_Alloc(gFontCache->pkt - gFontCache->pktBase, 0x10);
        DmaQueue_AddChain(0, (u32)gFontCache->pktBase & 0x0FFFFFFF, end, &gDmaQueue);
    }
    Gs_SetState(0x5F, 4);
    Gs_SetState(0x6C, 1);
    for (i = 0, h = gFontCache->tex; i < gFontCache->texCount; i++, h++) {
        if ((u16)*h != 0) {
            Tex_Evict(&gTexRecs[(u16)*h - 1]);
        }
    }
    for (j = 0, p = gFontCache->pal; j < gFontCache->palCount; j++, p++) {
        if (*p != 0) {
            Tex_Evict(&gPalRecs[*p - 1]);
        }
    }
}

s32 Font_SjisToJis(u32 code);

/* The low byte of a JIS code. An inline helper: written as `jis & 0xFF` in place,
 * MWCC schedules that mask before the high byte's shift. */
static inline s32 Jis_Low(s32 c)
{
    return c & 0xFF;
}

/* The font glyph index of a Shift-JIS code (row * 94 + column of its JIS code), or -1
 * past the table (0x1E80 glyphs). */
s32 Font_GlyphIndex(u32 code)
{
    s32 jis = Font_SjisToJis(code);
    s32 i = ((jis >> 8) - 0x21) * 94 + (Jis_Low(jis) - 0x21);

    if (i >= 0x1E80) {
        i = -1;
    }
    return i;
}

/*
 * Shift JIS to JIS X 0208: the lead byte (0x81-0x9F, 0xE0-0xEF) gives the row pair, the
 * trail byte (0x40-0x7E, 0x80-0x9E, 0x9F-0xFC) the column and the odd/even row.
 * The second lead-byte case, the second trail-byte case and the sum are done in 64 bits
 * (daddiu, then dsll32/dsra32): h is the lead byte as a long, computed by the same
 * expression as hi (MWCC shares the andi, so h needs no masking or extension).
 */
s32 Font_SjisToJis(u32 c)
{
    s32 hi = (c >> 8) & 0xFF;
    long h = (c >> 8) & 0xFF;
    u32 lo = c & 0xFF;
    long l;

    if (hi >= 0x81 && hi < 0xA0) {
        hi -= 0x81;
    } else if (hi >= 0xE0 && hi < 0xF0) {
        hi = h - 0xC1;
    }
    l = lo;
    hi <<= 1;
    if (l >= 0x40 && l < 0x7F) {
        lo -= 0x40;
    } else if (l >= 0x80 && l < 0x9F) {
        lo = l - 0x41;
    } else if (l >= 0x9F && l < 0xFD) {
        lo -= 0x9F;
        hi++;
    }
    return (((long)(u32)hi + 1) << 8) + (long)lo + 0x2021;
}

s32 Dma_WaitIdle(void);
s32 Tex_Use(s32 n, u32 *handles);
void Font_Expand2To4(u8 *src, u8 *dst);

/* Draws the glyphs of the page being filled (its slots pageFirst..pageEnd) into the page
 * textures, then hands every texture and palette handle to Tex_Use. */
void Font_UploadGlyphs(void)
{
    Image img;
    u32 handles[8];
    u32 tex;
    s32 i;
    s32 end;
    s32 cur;
    u16 *glyph;
    u32 perRow;
    u32 perCol;
    s32 n;

    perRow = 0x100 / (u32)gFontCache->width;
    perCol = 0x100 / (u32)gFontCache->height;
    end = gFontCache->pageEnd[gFontCache->page];
    i = gFontCache->pageFirst[gFontCache->page];
    glyph = gFontCache->pageGlyphs[gFontCache->page];
    cur = i / gFontCache->perPage;
    if (end - i != 0) {
        Dma_WaitIdle();
        tex = gFontCache->tex[cur];
        if (tex != 0) {
            flLockTexture(0, tex, &img, 2);
        }
        for (; i < end; i++) {
            if (cur != i / gFontCache->perPage) {
                if (tex != 0) {
                    flUnlockTexture(tex);
                }
                cur = i / gFontCache->perPage;
                tex = gFontCache->tex[cur];
                if (tex == 0) {
                    break;
                }
                flLockTexture(0, tex, &img, 2);
            }
            Font_Expand2To4(gFontCache->bits + *glyph++ * gFontCache->glyphBytes,
                            img.pixels + ((u32)(i % gFontCache->perPage) / perCol) * (gFontCache->height * 128)
                                + ((u32)gFontCache->width >> 1) * (i % perRow));
        }
        if (tex != 0) {
            flUnlockTexture(tex);
        }
    }
    n = 0;
    for (i = 0; i < gFontCache->texCount; i++) {
        handles[n++] = gFontCache->tex[i];
    }
    for (i = 0; i < gFontCache->palCount; i++) {
        handles[n++] = gFontCache->pal[i] << 16;
    }
    Tex_Use(n, handles);
}

/* The font's 2-bit pixels at src as 4-bit ones at dst (rows of 0x80 bytes). */
void Font_Expand2To4(u8 *src, u8 *dst)
{
    s32 row;
    s32 i;
    s32 h;
    s32 n;
    u16 *d;
    s32 b;

    h = gFontCache->height;
    n = (u32)((gFontCache->width + 3) & ~3) >> 2;
    for (row = 0; row < h; row++) {
        d = (u16 *)dst;
        for (i = 0; i < n; i++) {
            b = *src++;
            *d++ = ((b & 0xC0) >> 6) | (b & 0x30) | ((b & 0xC) << 6) | ((b & 3) << 12);
        }
        dst += 0x80;
    }
}

/* The largest end slot of the cache's seven pages (at least 0). Static: Font_CachePage keeps
 * gFontCache in a3 across the call. */
static s16 Font_MaxPageEnd(void)
{
    s32 i;
    s16 max = 0;

    for (i = 0; i < 7; i++) {
        if (gFontCache->pageEnd[i] > max) {
            max = gFontCache->pageEnd[i];
        }
    }
    return max;
}

s32 Font_CacheGlyph(u16 c);

/* Starts a new page of the glyph cache and gives every glyph of its text runs a slot. */
void Font_CachePage(void)
{
    FontCache *f;
    s16 end;
    TextRun *r;
    s32 i;
    s32 n;
    s16 *g;
    u8 *s;
    u32 c;
    s32 k;

    f = gFontCache;
    end = Font_MaxPageEnd();
    f->pageEnd[f->page] = end;
    f->pageFirst[f->page] = end;
    n = gFontCache->runCount[gFontCache->page];
    r = gFontCache->runs[gFontCache->page];
    for (i = 0; i < n; i++, r++) {
        if (r->isGlyphs != 0) {
            g = r->glyphs;
            for (;;) {
                k = *g++;
                if (k < 0) {
                    break;
                }
                Font_CacheGlyph(k);
            }
        } else {
            s = (u8 *)r->glyphs;
            for (;;) {
                c = *s++;
                if (c == 0) {
                    break;
                }
                if (c == '\n') {
                    continue;
                }
                if ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c < 0x100)) {
                    if (*s == 0) {
                        break;
                    }
                    c <<= 8;
                    c |= *s++;
                } else {
                    c = Sjis_FromAscii(c);
                }
                if (c == 0x8140) {
                    continue;
                }
                k = Font_GlyphIndex(c);
                if (k >= 0 && k < 0x1E80) {
                    Font_CacheGlyph(k);
                }
            }
        }
    }
}

/* Gives glyph c a slot of the cache (on the current page) unless it has one; 0 when full. */
s32 Font_CacheGlyph(u16 c)
{
    if (gFontCache->slot[c] != 0xFFFF) {
        return 1;
    }
    if (gFontCache->used >= gFontCache->max) {
        return 0;
    }
    gFontCache->slot[c] = gFontCache->used;
    gFontCache->pageGlyphs[gFontCache->page][gFontCache->pageEnd[gFontCache->page] - gFontCache->pageFirst[gFontCache->page]] = c;
    gFontCache->used++;
    gFontCache->pageEnd[gFontCache->page]++;
    return 1;
}

s32 Sjis_FromAscii(u8 c)
{
    s32 temp_v1;

    temp_v1 = c & 0xFF;
    if (temp_v1 < 0x21) {
        return 0x8140;
    }
    if (temp_v1 < 0x60) {
        return temp_v1 + 0x7FFF + 0x520;
    }
    if (temp_v1 < 0x80) {
        return temp_v1 + 0x7FFF + 0x521;
    }
    if (temp_v1 == 0xA5) {
        return 0x85A3;
    }
    return temp_v1 + 0x7FFF + 0x4FF;
}

extern u8 *gSjisAsciiTable;

/* dst = src with its one-byte characters widened to two-byte Shift-JIS ones (the table at
 * gSjisAsciiTable); two-byte characters are copied. */
void Sjis_Widen(char *from, char *to)
{
    u8 *src = (u8 *)from;
    u8 *dst = (u8 *)to;
    s32 c;
    u8 *t;

    while ((c = *src) != 0) {
        if ((c >= 0x80 && c <= 0x9F) || (c >= 0xE0 && c < 0x100)) {
            dst[0] = c;
            dst[1] = src[1];
            src += 2;
            dst += 2;
        } else {
            t = gSjisAsciiTable + c * 2;
            src++;
            dst[0] = t[0];
            dst[1] = t[1];
            dst += 2;
        }
    }
    *dst = 0;
}

extern char D_00402490[];
char *strncpy(char *dst, const char *src, u32 n);

/* dst = the last three decimal digits of n as two-byte Shift-JIS digits (D_00402490), no
 * terminator. */
void Sjis_PutDigits3(char *dst, s32 n)
{
    s32 i;
    s32 d;

    for (i = 0, d = 1000; i < 3; i++) {
        n %= d;
        d /= 10;
        strncpy(dst, &D_00402490[n / d * 2], 2);
        dst += 2;
    }
}

f32 Math_Sin(f32 x);
extern u32 gTextPulseColor;

/* gTextPulseColor = a colour pulsing with a sine over 32 frames (around 0x50, 0xE4, 0xF7). */
void Text_UpdatePulse(void)
{
    f32 s = Math_Sin(3.1415927f / 32768.0f * (u16)((gFontCache->pulse & 0x1F) << 11));
    s8 r = 64.0f * s;
    s8 g = 20.0f * s;
    s8 b = 7.0f * s;

    gTextPulseColor = 0xFF000000 | (u8)(r + 0x50) << 16 | (u8)(g + 0xE4) << 8 | (u8)(b + 0xF7);
    gFontCache->pulse++;
}

/* gRsMaterials (main): a ModelMat per register 0x3A-0x59 (include/model.h). */

extern ModelEntry gModelPool[];
extern ModelMat gRsMaterials[];

void *Mem_Alloc(u32 size, s32 flag);
void Mem_Fill(void *dst, u8 c, s32 n);
/* unit's own declaration: with the definition's void (void *dst, void *src, s32 n) (main/unk_17FE20.c), Model_Create: 4 of 101 words differ */
void Mem_Copy(void *dst, void *src, u32 n);
s32 Model_Setup(ModelEntry *e, MeshDesc *hdr);
s32 Model_FreeHandle(u32 h);

extern s32 gRenderFlags2;
/*
 * A new model from the file header hdr (a copy of the file unless flags has 4): its
 * handle (pool index + 1), or 0 when it can't be set up.
 */
s32 Model_Create(MeshDesc *hdr, u32 flags)
{
    u32 i;
    ModelEntry *e;
    u8 *dst;

    for (i = 0; i < 0x180; i++) {
        if (gModelPool[i].used == 0) {
            break;
        }
    }
    e = &gModelPool[i];
    Mem_Fill(e, 0, sizeof(ModelEntry));
    e->used = hdr->flags;
    e->unk08 = hdr->format;
    e->flags = flags;
    e->unk50 = 0;
    e->unk54 = 0;
    if (!(flags & 4)) {
        e->size = 0x30;
        e->size += (hdr->stripsSize + 0xF) & ~0xF;
        e->size += (hdr->vertsSize + 0xF) & ~0xF;
        e->file = Mem_Alloc(e->size, 3);
        dst = Mem_GetAddr(e->file);
        Mem_Copy(dst, hdr, 0x24);
        dst += 0x30;
        Mem_Copy(dst, hdr->strips, hdr->stripsSize);
        dst += (hdr->stripsSize + 0xF) & ~0xF;
        Mem_Copy(dst, hdr->verts, hdr->vertsSize);
    }
    e->frame = gRenderFlags2;
    if (Model_Setup(e, hdr) == 0) {
        Model_FreeHandle(i + 1);
        return 0;
    }
    return i + 1;
}

void Vu1_SelectProgram(ModelEntry *e, MeshDesc *hdr);
static void Model_CollectTextures(ModelEntry *e, s32 unused);
void Model_SetupEntry(MeshDesc *hdr, ModelEntry *e, s32 arg2);
void Model_BuildVifPackets(MeshDesc *hdr, ModelEntry *e);
void Model_BuildPartPackets(ModelEntry *e, s32 n, s32 prog, s32 dst, u8 *mat);

extern s32 gModelCount;
/*
 * Sets up model e from its file header hdr: its VU1 program, packets, textures and parts;
 * 0 when no program fits. Counts the models (0x1E8B78, main).
 */
s32 Model_Setup(ModelEntry *e, MeshDesc *hdr)
{
    Vu1_SelectProgram(e, hdr);
    if (e->prog != -1) {
        Model_SetupEntry(hdr, e, 0);
        Model_BuildVifPackets(hdr, e);
        Model_CollectTextures(e, 0);
        Model_BuildPartPackets(e, e->partCount, e->progInfo[5], e->unk24 + (s32)Mem_GetAddr(e->data), e->mat);
    } else {
        return 0;
    }
    gModelCount++;
    return 1;
}

/*
 * hdr = the 0x24-byte header of model file obj (its handle at +0x14), then hdr+8 = its
 * data (after 0x30 bytes) and hdr+0x10 = the data after that (the size at +0x1C, 16-aligned).
 */
void Model_ReadHeader(ModelEntry *e, MeshDesc *hdr)
{
    MeshDesc *base;
    u8 *data;

    data = Mem_GetAddr(e->file);
    base = (MeshDesc *)data;
    data += 0x30;
    Mem_Copy(hdr, base, 0x24);
    hdr->strips = (u16 *)data;
    data += (base->stripsSize + 0xF) & ~0xF;
    hdr->verts = data;
}

extern s32 gModelPktSteps[];
extern s32 gVu1ProgLayouts[][2];

void Model_SetupEntry(MeshDesc *hdr, ModelEntry *e, s32 unused)
{
    s8 used[0x20];
    s32 i;
    s32 total;
    s32 off;
    s32 *info;
    u16 len;
    s32 step;
    s32 sz;
    s32 limit;
    s32 lastA;
    s32 curA;
    s32 cnt;
    u32 flags;
    s32 lastB;
    s32 curB;
    s32 count;
    u16 *p;

    total = 0;
    info = e->progInfo;
    off = 0;
    Mem_Fill(used, 0, 0x20);
    count = hdr->count;
    lastA = -1;
    p = hdr->strips;
    curA = 0;
    lastB = lastA;
    curB = 0;
    step = 0;
    cnt = 0;
    e->unk40 = 0;
    e->unk3C = 0;
    e->unk44 = 0;
    limit = info[8] + 0x20;
    for (i = 0; i < count; i++) {
        len = *p & 0x3FFF;
        e->unk40 += len;
        e->unk3C += len - 2;
        flags = hdr->flags;
        p++;
        if (flags & 0x100000) {
            curA = *p;
            p++;
        }
        if (flags & 0x200000) {
            curB = *p;
            p++;
        }
        if (flags & 0x1000) {
            p += len;
        }
        if (lastA != curA) {
            if (cnt != 0) {
                off += step;
                cnt = 0;
                total += off;
            }
            used[curA] = 1;
            lastA = curA;
            lastB = curB;
            off = 0x30;
            step = 0x20;
            e->unk44++;
        } else if (lastB != curB) {
            if (cnt != 0) {
                off += step;
                if ((u32)(off + limit) >= 0x100000) {
                    total += off;
                    off = 0x10;
                }
                cnt = 0;
            }
            lastB = curB;
            off += 0x20;
            step = 0x20;
        }
        if (len > 0) {
            sz = gVu1ProgLayouts[info[6]][0];
            do {
                if ((u32)(step + sz) >= (u32)info[8]) {
                    off += step;
                    if ((u32)(off + limit) >= 0x100000) {
                        total += off;
                        off = 0x10;
                    }
                    off += 0x20;
                    step = 0x20;
                    cnt = 0;
                }
                len--;
                cnt++;
                step += sz;
            } while (len != 0);
        }
    }
    off += step;
    total += off;
    e->unk18 = total;
    e->partCount = 0;
    for (i = 0; i < 0x20; i++) {
        if (used[i] != 0) {
            e->mat[e->partCount++] = i;
        }
    }
    e->unk30 = (e->partCount * 4 + 0xF) & ~0xF;
    e->pktSize = (e->partCount * gModelPktSteps[info[5]] + 0xF) & ~0xF;
    e->pktSize += 0x20;
    e->unk2C = (e->unk18 + 0xF) & ~0xF;
    e->unk24 = e->unk2C + ((e->unk30 + 0xF) & ~0xF);
    count = ((total + 0xF) & ~0xF);
    count += ((e->unk30 + 0xF) & ~0xF);
    count += ((e->pktSize + 0xF) & ~0xF);
    e->data = Mem_Alloc(count, 3);
    e->dataAddr = Mem_GetAddr(e->data);
}

/* Vertex formats 0x25, 0x35 and 0x135. */
typedef struct Vtx25 {
    f32 pos[3];
    f32 nrm[3];
    u32 col;
} Vtx25;

typedef struct Vtx35 {
    f32 pos[3];
    f32 nrm[3];
    u32 col;
    f32 uv[2];
} Vtx35;

typedef struct Vtx135 {
    f32 pos[3];
    f32 nrm[3];
    u32 col;
    f32 uv[2];
    u32 bones;                  /* 0x24: four bone indexes, the first in the top byte */
    u32 weights;                /* 0x28: their weights */
} Vtx135;

/* progInfo[10]: the bone matrices. */
typedef struct BoneInfo {
    s32 *base;
    s32 off;
    s32 size;
} BoneInfo;

/* The GIF tag fields per program (progInfo[7]): PRIM, REGS and NREG. */
typedef struct GifInfo {
    u32 prim;
    u32 regs;
    u32 nreg;
} GifInfo;

extern GifInfo gVu1GifInfos[];
f32 Gs_AlphaFromByte(u32 a, s32 mode);
s32 Debug_Printf(const char *fmt, ...);

#define GIF_TAG(nloop, prim, nreg) ((u64)(nloop) | ((u64)1 << 15) | ((u64)1 << 46) | ((u64)(prim) << 47) | ((u64)(nreg) << 60))

extern f32 gDispTexScaleV;
extern f32 gDispTexScaleU;
/*
 * Builds model e's VIF packets from its file header hdr (flPS2ConvClayData, from its debug
 * string): per material a DMA chain of VIF unpacks of the strips' vertices (position,
 * normal, colour, texture coordinates and bone weights, as the program's bits ask), each
 * batch with its GIF tag. Batches are cut where the program's buffer (progInfo[8]) would
 * overflow, and a DMA cnt tag every 1 MB. offs (at data + unk2C) gets each material's chain.
 */
void Model_BuildVifPackets(MeshDesc *h, ModelEntry *e)
{
    s32 *info = e->progInfo;
    s32 bits = gVu1ProgLayouts[info[6]][1];
    s32 a[4] = {0};
    s32 b[4] = {0};
    u32 starts[0x20];
    u32 base;
    u32 qv;
    u32 tag;
    s32 cnt;
    s32 lastA;
    s32 curA;
    s32 lastB;
    s32 curB;
    u32 step;
    u32 sz;
    s32 count;
    u32 raw;
    s32 *offs;
    u32 prim;
    u32 regs;
    u32 nreg;
    s32 toggle;
    u32 limit;
    s32 i;
    s32 j;
    s32 k;
    u32 q;
    u16 *p;
    u32 *t;
    u32 *w;
    f32 *wt;
    u32 *r;
    Vtx25 *v25;
    Vtx35 *v35;
    Vtx135 *v135;

    for (i = 0; i < 0x20; i++) {
        starts[i] = 0;
    }
    prim = gVu1GifInfos[info[7]].prim;
    regs = gVu1GifInfos[info[7]].regs;
    nreg = gVu1GifInfos[info[7]].nreg;
    if ((e->frame & 0x18000) == 0x8000) {
        prim |= 0x20;
    }
    base = (u32)Mem_GetAddr(e->data);
    q = base;
    p = h->strips;
    count = h->count;
    limit = info[8] + 0x20;
    lastA = -1;
    lastB = lastA;
    curA = 0;
    curB = 0;
    cnt = 0;
    offs = (s32 *)(base + e->unk2C);
    for (i = 0; i < count; i++) {
        raw = *p;
        toggle = 0;
        p++;
        if (h->flags & 0x100000) {
            curA = *p;
            p++;
        }
        if (h->flags & 0x200000) {
            curB = *p;
            p++;
        }
        if (lastA != curA) {
            if (cnt != 0) {
                qv = (u32)Vif_UnpackV4((u128 *)qv, step >> 4, 0);
                switch (lastB) {
                case 0:
                    ((u32 *)qv)[0] = 1;
                    break;
                case 1:
                    ((u32 *)qv)[0] = 2;
                    break;
                }
                ((u32 *)qv)[1] = ((nreg << 4) * (cnt + 2) >> 4) + 1;
                ((u64 *)qv)[1] = 0;
                ((u64 *)qv)[2] = GIF_TAG(cnt, prim, nreg);
                ((u32 *)qv)[6] = regs;
                ((u32 *)qv)[7] = 0;
                q = (u32)Vif_MsCnt((u128 *)q);
                if (tag == starts[lastA]) {
                    DmaTag_Ret((u128 *)tag, ((q - tag) >> 4) - 1, 0, 0);
                    offs[lastA] = tag - base;
                } else {
                    t = (u32 *)starts[lastA];
                    do {
                        switch (t[0] & 0x70000000) {
                        case 0x10000000:
                        case 0x20000000:
                            t = (u32 *)((u8 *)t + ((t[0] & 0xFFFF) + 1) * 16);
                            break;
                        case 0x60000000:
                            DmaTag_Next((u128 *)t, t[0] & 0xFFFF, tag, 0, 0);
                            t = NULL;
                            break;
                        }
                    } while (t != NULL);
                    DmaTag_Ret((u128 *)tag, ((q - tag) >> 4) - 1, 0, 0);
                }
                cnt = 0;
            }
            tag = q;
            lastA = curA;
            lastB = curB;
            if (starts[curA] == 0) {
                starts[curA] = q;
            }
            qv = q + 0x10;
            q += 0x40;
            step = 0x20;
        } else if (lastB != curB) {
            if (cnt != 0) {
                w = (u32 *)Vif_UnpackV4((u128 *)qv, step >> 4, 0);
                switch (lastB) {
                case 0:
                    w[0] = 1;
                    break;
                case 1:
                    w[0] = 2;
                    break;
                }
                w[1] = ((nreg << 4) * (cnt + 2) >> 4) + 1;
                ((u64 *)w)[1] = 0;
                ((u64 *)w)[2] = GIF_TAG(cnt, prim, nreg);
                w[6] = regs;
                w[7] = 0;
                q = (u32)Vif_MsCnt((u128 *)q);
                if (q - tag + limit >= 0x100000) {
                    DmaTag_Cnt((u128 *)tag, ((q - tag) >> 4) - 1, 0, 0);
                    tag = q;
                    q += 0x10;
                }
                cnt = 0;
            }
            qv = q;
            q += 0x30;
            lastB = curB;
            step = 0x20;
        }
        for (j = 0; j < (raw & 0x3FFF); j++) {
            sz = gVu1ProgLayouts[info[6]][0];
            if (step + sz >= (u32)info[8]) {
                w = (u32 *)Vif_UnpackV4((u128 *)qv, step >> 4, 0);
                switch (lastB) {
                case 0:
                    w[0] = 1;
                    break;
                case 1:
                    w[0] = 2;
                    break;
                }
                w[1] = ((nreg << 4) * (cnt + 2) >> 4) + 1;
                ((u64 *)w)[1] = 0;
                ((u64 *)w)[2] = GIF_TAG(cnt, prim, nreg);
                w[6] = regs;
                w[7] = 0;
                q = (u32)Vif_MsCnt((u128 *)q);
                if (q - tag + limit >= 0x100000) {
                    DmaTag_Cnt((u128 *)tag, ((q - tag) >> 4) - 1, 0, 0);
                    tag = q;
                    q += 0x10;
                }
                qv = q;
                step = 0x20;
                q += 0x30;
                cnt = 0;
            }
            step += sz;
            cnt++;
            switch (h->format) {
            case 0x25:
                if (h->flags & 0x1000) {
                    v25 = &((Vtx25 *)h->verts)[*p];
                    p++;
                } else {
                    v25++;
                }
                if (bits & 1) {
                    ((f32 *)q)[0] = v25->pos[0];
                    ((f32 *)q)[1] = v25->pos[1];
                    ((f32 *)q)[2] = v25->pos[2];
                    if (j < 2) {
                        ((u32 *)q)[3] = 0x8000;
                    } else {
                        ((u32 *)q)[3] = 0;
                    }
                    q += 0x10;
                }
                if (bits & 2) {
                    ((f32 *)q)[0] = v25->nrm[0];
                    ((f32 *)q)[1] = v25->nrm[1];
                    ((f32 *)q)[2] = v25->nrm[2];
                    if (toggle == 0) {
                        ((f32 *)q)[3] = 1.0f;
                    } else {
                        ((f32 *)q)[3] = -1.0f;
                    }
                    q += 0x10;
                }
                if (bits & 4) {
                    if (!(e->frame & 0xF00)) {
                        ((f32 *)q)[0] = (v25->col >> 16) & 0xFF;
                        ((f32 *)q)[1] = (v25->col >> 8) & 0xFF;
                        ((f32 *)q)[2] = v25->col & 0xFF;
                    } else {
                        ((f32 *)q)[0] = Gs_AlphaFromByte((v25->col >> 16) & 0xFF, 1);
                        ((f32 *)q)[1] = Gs_AlphaFromByte((v25->col >> 8) & 0xFF, 1);
                        ((f32 *)q)[2] = Gs_AlphaFromByte(v25->col & 0xFF, 1);
                    }
                    ((f32 *)q)[3] = Gs_AlphaFromByte((v25->col >> 24) & 0xFF, 0);
                    q += 0x10;
                }
                break;
            case 0x35:
                if (h->flags & 0x1000) {
                    v35 = &((Vtx35 *)h->verts)[*p];
                    p++;
                } else {
                    v35++;
                }
                if (bits & 1) {
                    ((f32 *)q)[0] = v35->pos[0];
                    ((f32 *)q)[1] = v35->pos[1];
                    ((f32 *)q)[2] = v35->pos[2];
                    if (j < 2) {
                        ((u32 *)q)[3] = 0x8000;
                    } else {
                        ((u32 *)q)[3] = 0;
                    }
                    q += 0x10;
                }
                if (bits & 2) {
                    ((f32 *)q)[0] = v35->nrm[0];
                    ((f32 *)q)[1] = v35->nrm[1];
                    ((f32 *)q)[2] = v35->nrm[2];
                    if (toggle == 0) {
                        ((f32 *)q)[3] = 1.0f;
                    } else {
                        ((f32 *)q)[3] = -1.0f;
                    }
                    q += 0x10;
                }
                if (bits & 4) {
                    if (!(e->frame & 0xF00)) {
                        ((f32 *)q)[0] = Gs_AlphaFromByte((v35->col >> 16) & 0xFF, 0);
                        ((f32 *)q)[1] = Gs_AlphaFromByte((v35->col >> 8) & 0xFF, 0);
                        ((f32 *)q)[2] = Gs_AlphaFromByte(v35->col & 0xFF, 0);
                    } else {
                        ((f32 *)q)[0] = Gs_AlphaFromByte((v35->col >> 16) & 0xFF, 1);
                        ((f32 *)q)[1] = Gs_AlphaFromByte((v35->col >> 8) & 0xFF, 1);
                        ((f32 *)q)[2] = Gs_AlphaFromByte(v35->col & 0xFF, 1);
                    }
                    ((f32 *)q)[3] = Gs_AlphaFromByte((v35->col >> 24) & 0xFF, 0);
                    q += 0x10;
                }
                if (bits & 8) {
                    r = (u32 *)q;
                    ((f32 *)q)[0] = v35->uv[0];
                    ((f32 *)q)[1] = v35->uv[1];
                    ((f32 *)q)[2] = 1.0f;
                    ((u32 *)q)[3] = 0;
                    q += 0x10;
                    if (e->flags & 0x20) {
                        ((f32 *)r)[0] = ((f32 *)r)[0] * gDispTexScaleU;
                        ((f32 *)r)[1] = ((f32 *)r)[1] * gDispTexScaleV;
                    }
                }
                break;
            case 0x135:
                if (h->flags & 0x1000) {
                    v135 = &((Vtx135 *)h->verts)[*p];
                    p++;
                } else {
                    v135++;
                }
                if (bits & 1) {
                    ((f32 *)q)[0] = v135->pos[0];
                    ((f32 *)q)[1] = v135->pos[1];
                    ((f32 *)q)[2] = v135->pos[2];
                    if (j < 2) {
                        ((u32 *)q)[3] = 0x8000;
                    } else {
                        ((u32 *)q)[3] = 0;
                    }
                    q += 0x10;
                }
                if (bits & 2) {
                    ((f32 *)q)[0] = v135->nrm[0];
                    ((f32 *)q)[1] = v135->nrm[1];
                    ((f32 *)q)[2] = v135->nrm[2];
                    if (toggle == 0) {
                        ((f32 *)q)[3] = 1.0f;
                    } else {
                        ((f32 *)q)[3] = -1.0f;
                    }
                    q += 0x10;
                }
                if (bits & 4) {
                    if (!(e->frame & 0xF00)) {
                        ((f32 *)q)[0] = Gs_AlphaFromByte((v135->col >> 16) & 0xFF, 0);
                        ((f32 *)q)[1] = Gs_AlphaFromByte((v135->col >> 8) & 0xFF, 0);
                        ((f32 *)q)[2] = Gs_AlphaFromByte(v135->col & 0xFF, 0);
                    } else {
                        ((f32 *)q)[0] = Gs_AlphaFromByte((v135->col >> 16) & 0xFF, 1);
                        ((f32 *)q)[1] = Gs_AlphaFromByte((v135->col >> 8) & 0xFF, 1);
                        ((f32 *)q)[2] = Gs_AlphaFromByte(v135->col & 0xFF, 1);
                    }
                    ((f32 *)q)[3] = Gs_AlphaFromByte((v135->col >> 24) & 0xFF, 0);
                    q += 0x10;
                }
                if (bits & 8) {
                    r = (u32 *)q;
                    ((f32 *)q)[0] = v135->uv[0];
                    ((f32 *)q)[1] = v135->uv[1];
                    ((f32 *)q)[2] = 1.0f;
                    ((u32 *)q)[3] = 0;
                    q += 0x10;
                    if (e->flags & 0x20) {
                        ((f32 *)r)[0] = ((f32 *)r)[0] * gDispTexScaleU;
                        ((f32 *)r)[1] = ((f32 *)r)[1] * gDispTexScaleV;
                    }
                }
                if ((bits & 0x10) && (bits & 0x20)) {
                    r = (u32 *)q;
                    wt = (f32 *)(q + 0x10);
                    q += 0x20;
                    a[0] = (v135->bones >> 24) & 0xFF;
                    a[1] = (v135->bones >> 16) & 0xFF;
                    a[2] = (v135->bones >> 8) & 0xFF;
                    a[3] = v135->bones & 0xFF;
                    b[0] = (v135->weights >> 24) & 0xFF;
                    b[1] = (v135->weights >> 16) & 0xFF;
                    b[2] = (v135->weights >> 8) & 0xFF;
                    b[3] = v135->weights & 0xFF;
                    *(u128 *)r = 0;
                    *(u128 *)wt = 0;
                    switch (lastB) {
                    case 0:
                        r[0] = (u32)&((BoneInfo *)info[10])->base[a[0]];
                        e->unk54 |= 1 << a[0];
                        r[1] = (u32)&((BoneInfo *)info[10])->base[((BoneInfo *)info[10])->off] + a[0] * ((BoneInfo *)info[10])->size;
                        wt[0] = 100.0f;
                        break;
                    case 1:
                        for (k = 0; k < 4; k++) {
                            r[k] = (u32)&((BoneInfo *)info[10])->base[a[k]];
                            e->unk54 |= 1 << a[k];
                            wt[k] = b[k] / 255.0f;
                        }
                        break;
                    }
                }
                break;
            }
            toggle ^= 1;
        }
    }
    if (cnt != 0) {
        w = (u32 *)Vif_UnpackV4((u128 *)qv, step >> 4, 0);
        switch (lastB) {
        case 0:
            w[0] = 1;
            break;
        case 1:
            w[0] = 2;
            break;
        }
        w[1] = ((nreg << 4) * (cnt + 2) >> 4) + 1;
        ((u64 *)w)[1] = 0;
        ((u64 *)w)[2] = GIF_TAG(cnt, prim, nreg);
        w[6] = regs;
        w[7] = 0;
        q = (u32)Vif_MsCnt((u128 *)q);
    }
    if (tag == starts[lastA]) {
        DmaTag_Ret((u128 *)tag, ((q - tag) >> 4) - 1, 0, 0);
        offs[lastA] = tag - base;
    } else {
        t = (u32 *)starts[lastA];
        do {
            switch (t[0] & 0x70000000) {
            case 0x10000000:
            case 0x20000000:
                t = (u32 *)((u8 *)t + ((t[0] & 0xFFFF) + 1) * 16);
                break;
            case 0x60000000:
                DmaTag_Next((u128 *)t, t[0] & 0xFFFF, tag, 0, 0);
                t = NULL;
                break;
            }
        } while (t != NULL);
        DmaTag_Ret((u128 *)tag, ((q - tag) >> 4) - 1, 0, 0);
    }
    if (e->unk18 != q - base) {
        Debug_Printf("A = %x B = %x, %x @flPS2ConvClayData", base, q, e->unk18);
    }
}

/* A 0..255 alpha as the GS's 0..128 (mode 0) or as 0..1 (mode 1). */
f32 Gs_AlphaFromByte(u32 a, s32 mode)
{
    f32 r = 0.0f;

    if (mode == 0) {
        if (a == 0xFF) {
            r = 128.0f;
        } else if (a != 0) {
            a >>= 1;
            if (a == 0) {
                a = 1;
            }
            r = a;
        }
    } else {
        r = a / 255.0f;
        if (r > 1.0f) {
            r = 1.0f;
        }
        if (r < 0.0f) {
            r = 0.0f;
        }
    }
    return r;
}

void Model_BuildPartPacket(u8 *part, u32 *dst, s32 prog, s32 mat);
extern s32 gModelPktSteps[];

/* The DMA packets of the n parts mat[] of model e (Model_BuildPartPacket, a step of gModelPktSteps[prog]
 * bytes each) from dst, closed by an END tag. */
void Model_BuildPartPackets(ModelEntry *e, s32 n, s32 prog, s32 dst, u8 *mat)
{
    s32 i;
    s32 step;
    s32 *parts;
    u8 *base;

    step = gModelPktSteps[prog];
    base = e->dataAddr;
    parts = (s32 *)(base + e->unk2C);
    for (i = 0; i < n; i++) {
        Model_BuildPartPacket(base + parts[mat[i]], (u32 *)dst, prog, mat[i]);
        dst += step;
    }
    DmaTag_End((u128 *)dst, 1, 1, 0);
    ((u32 *)dst)[2] = 0;
    ((u32 *)dst)[3] = 0;
    ((u32 *)dst)[4] = 0;
    ((u32 *)dst)[5] = 0;
    ((u32 *)dst)[6] = 0;
    ((u32 *)dst)[7] = 0;
}

void Gs_SetTexRegs(u32 tex, u64 *texa, u64 *tex1, u64 *tex0, u64 *clamp, u64 *mip1, u64 *mip2);
void Gs_SetTexRegs2(u32 tex, u64 *texa, u64 *tex1, u64 *tex0, u64 *clamp, u64 *mip1, u64 *mip2,
                   u32 tex2, u64 *tex1b, u64 *tex0b, u64 *clampb, u64 *mip1b, u64 *mip2b);

extern u32 D_0020E134;
/* The DMA packet of one model part at dst for VU1 program kind prog: a CALL of the part's
 * data, an UNPACK of the header, a GIF tag, the texture registers of material mat
 * (Gs_SetTexRegs, or fixed ones for kinds 1 and 4), the material's colours and an MSCNT. */
void Model_BuildPartPacket(u8 *part, u32 *dst, s32 prog, s32 mat)
{
    ModelMat *m = &gRsMaterials[mat];
    u8 *p;
    s32 lod;
    s32 on;

    switch (prog) {
    case 0:
        p = (u8 *)dst;
        DmaTag_Call((u128 *)p, 0xC, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(p + 0x10), 0xA, 0);
        *(u128 *)(p + 0x20) = 0;
        *(u64 *)(p + 0x30) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(p + 0x38) = 0xE;
        *(u32 *)(p + 0x3C) = 0;
        Gs_SetTexRegs(m->tex, (u64 *)(p + 0x40), (u64 *)(p + 0x50), (u64 *)(p + 0x60),
                      (u64 *)(p + 0x70), (u64 *)(p + 0x80), (u64 *)(p + 0x90));
        *(f32 *)(p + 0xA0) = m->rgb[0];
        *(f32 *)(p + 0xA4) = m->rgb[1];
        *(f32 *)(p + 0xA8) = m->rgb[2];
        *(f32 *)(p + 0xAC) = 128.0f * m->alpha;
        *(f32 *)(p + 0xB0) = m->unk24[0];
        *(f32 *)(p + 0xB4) = m->unk24[1];
        *(f32 *)(p + 0xB8) = m->unk24[2];
        *(f32 *)(p + 0xBC) = m->unk24[3];
        Vif_MsCnt((u128 *)(p + 0xC0));
        break;
    case 1:
        p = (u8 *)dst;
        DmaTag_Call((u128 *)p, 0xC, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(p + 0x10), 0xA, 0);
        *(u128 *)(p + 0x20) = 0;
        *(u64 *)(p + 0x30) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(p + 0x38) = 0xE;
        *(u32 *)(p + 0x3C) = 0;
        *(u64 *)(p + 0x40) = 0;
        *(u64 *)(p + 0x48) = 0x3B;
        *(u64 *)(p + 0x50) = 0;
        *(u64 *)(p + 0x58) = 0x14;
        *(u64 *)(p + 0x60) = 0;
        *(u64 *)(p + 0x68) = 6;
        *(u64 *)(p + 0x70) = 0;
        *(u64 *)(p + 0x78) = 8;
        *(u64 *)(p + 0x80) = 0;
        *(u64 *)(p + 0x88) = 0x34;
        *(u64 *)(p + 0x90) = 0;
        *(u64 *)(p + 0x98) = 0x36;
        *(f32 *)(p + 0xA0) = m->rgb[0];
        *(f32 *)(p + 0xA4) = m->rgb[1];
        *(f32 *)(p + 0xA8) = m->rgb[2];
        *(f32 *)(p + 0xAC) = 128.0f * m->alpha;
        *(f32 *)(p + 0xB0) = m->unk24[0];
        *(f32 *)(p + 0xB4) = m->unk24[1];
        *(f32 *)(p + 0xB8) = m->unk24[2];
        *(f32 *)(p + 0xBC) = m->unk24[3];
        Vif_MsCnt((u128 *)(p + 0xC0));
        break;
    case 2:
        p = (u8 *)dst;
        DmaTag_Call((u128 *)p, 0xE, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(p + 0x10), 0xC, 0);
        *(u128 *)(p + 0x20) = 0;
        *(u64 *)(p + 0x30) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(p + 0x38) = 0xE;
        *(u32 *)(p + 0x3C) = 0;
        Gs_SetTexRegs(m->tex, (u64 *)(p + 0x40), (u64 *)(p + 0x50), (u64 *)(p + 0x60),
                      (u64 *)(p + 0x70), (u64 *)(p + 0x80), (u64 *)(p + 0x90));
        *(f32 *)(p + 0xA0) = m->rgb[0];
        *(f32 *)(p + 0xA4) = m->rgb[1];
        *(f32 *)(p + 0xA8) = m->rgb[2];
        *(f32 *)(p + 0xAC) = 128.0f * m->alpha;
        *(f32 *)(p + 0xB0) = m->unk24[0];
        *(f32 *)(p + 0xB4) = m->unk24[1];
        *(f32 *)(p + 0xB8) = m->unk24[2];
        *(f32 *)(p + 0xBC) = m->unk24[3];
        *(f32 *)(p + 0xC0) = m->unk14[0];
        *(f32 *)(p + 0xC4) = m->unk14[1];
        *(f32 *)(p + 0xC8) = m->unk14[2];
        *(f32 *)(p + 0xCC) = m->unk14[3];
        switch (m->size) {
        case 0:
        case 1:
        case 2:
            on = lod = 1;
            break;
        case 4:
            lod = 2;
            on = 1;
            break;
        case 8:
            lod = 3;
            on = 1;
            break;
        case 0x10:
            lod = 4;
            on = 1;
            break;
        case 0x20:
            lod = 5;
            on = 1;
            break;
        case 0x40:
            lod = 6;
            on = 1;
            break;
        case 0x80:
            lod = 7;
            on = 1;
            break;
        case 0x100:
            lod = 8;
            on = 1;
            break;
        default:
            lod = m->size - 1;
            on = 0;
            break;
        }
        *(s32 *)(p + 0xD0) = lod;
        *(s32 *)(p + 0xD4) = on;
        *(s32 *)(p + 0xD8) = 0;
        *(s32 *)(p + 0xDC) = 0;
        Vif_MsCnt((u128 *)(p + 0xE0));
        break;
    case 3:
        p = (u8 *)dst;
        DmaTag_Call((u128 *)p, 0xA, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(p + 0x10), 8, 0);
        *(u128 *)(p + 0x20) = 0;
        *(u64 *)(p + 0x30) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(p + 0x38) = 0xE;
        *(u32 *)(p + 0x3C) = 0;
        Gs_SetTexRegs(m->tex, (u64 *)(p + 0x40), (u64 *)(p + 0x50), (u64 *)(p + 0x60),
                      (u64 *)(p + 0x70), (u64 *)(p + 0x80), (u64 *)(p + 0x90));
        Vif_MsCnt((u128 *)(p + 0xA0));
        break;
    case 4:
        p = (u8 *)dst;
        DmaTag_Call((u128 *)p, 0xB, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(p + 0x10), 9, 0);
        *(u128 *)(p + 0x20) = 0;
        *(u64 *)(p + 0x30) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(p + 0x38) = 0xE;
        *(u32 *)(p + 0x3C) = 0;
        *(u64 *)(p + 0x40) = 0;
        *(u64 *)(p + 0x48) = 0x3B;
        *(u64 *)(p + 0x50) = 0;
        *(u64 *)(p + 0x58) = 0x14;
        *(u64 *)(p + 0x60) = 0;
        *(u64 *)(p + 0x68) = 6;
        *(u64 *)(p + 0x70) = 0;
        *(u64 *)(p + 0x78) = 8;
        *(u64 *)(p + 0x80) = 0;
        *(u64 *)(p + 0x88) = 0x34;
        *(u64 *)(p + 0x90) = 0;
        *(u64 *)(p + 0x98) = 0x36;
        *(u128 *)(p + 0xA0) = 0;
        Vif_MsCnt((u128 *)(p + 0xB0));
        break;
    case 5:
        p = (u8 *)dst;
        DmaTag_Call((u128 *)p, 0x10, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(p + 0x10), 0xE, 0);
        *(u128 *)(p + 0x20) = 0;
        *(u64 *)(p + 0x30) = 0x800B | ((u64)0x10000000 << 32);
        *(u32 *)(p + 0x38) = 0xE;
        *(u32 *)(p + 0x3C) = 0;
        Gs_SetTexRegs2(m->tex, (u64 *)(p + 0x40), (u64 *)(p + 0x50), (u64 *)(p + 0x60),
                       (u64 *)(p + 0x70), (u64 *)(p + 0x80), (u64 *)(p + 0x90),
                       D_0020E134, (u64 *)(p + 0xA0), (u64 *)(p + 0xB0),
                       (u64 *)(p + 0xC0), (u64 *)(p + 0xD0), (u64 *)(p + 0xE0));
        *(f32 *)(p + 0xF0) = m->rgb[0];
        *(f32 *)(p + 0xF4) = m->rgb[1];
        *(f32 *)(p + 0xF8) = m->rgb[2];
        *(f32 *)(p + 0xFC) = 128.0f * m->alpha;
        Vif_MsCnt((u128 *)(p + 0x100));
        break;
    }
}

void Gs_SetTexRegs(u32 tex, u64 *texa, u64 *tex1, u64 *tex0, u64 *clamp, u64 *mip1, u64 *mip2);
void Gs_SetTexRegs2(u32 tex, u64 *texa, u64 *tex1, u64 *tex0, u64 *clamp, u64 *mip1, u64 *mip2,
                   u32 tex2, u64 *tex1b, u64 *tex0b, u64 *clampb, u64 *mip1b, u64 *mip2b);

/* The GS texture registers in the packets of n parts from pkt (textures tex[]), for VU1
 * program kinds 0, 2, 3 (one context) and 5 (two). */
void Model_SetTexRegs(s32 n, s32 prog, u8 *pkt, s32 *tex)
{
    s32 step = gModelPktSteps[prog];
    s32 i;

    switch (prog) {
    case 0:
        for (i = 0; i < n; i++) {
            Gs_SetTexRegs(*tex, (u64 *)(pkt + 0x40), (u64 *)(pkt + 0x50), (u64 *)(pkt + 0x60),
                          (u64 *)(pkt + 0x70), (u64 *)(pkt + 0x80), (u64 *)(pkt + 0x90));
            pkt += step;
            tex++;
        }
        break;
    case 2:
        for (i = 0; i < n; i++) {
            Gs_SetTexRegs(*tex, (u64 *)(pkt + 0x40), (u64 *)(pkt + 0x50), (u64 *)(pkt + 0x60),
                          (u64 *)(pkt + 0x70), (u64 *)(pkt + 0x80), (u64 *)(pkt + 0x90));
            pkt += step;
            tex++;
        }
        break;
    case 3:
        for (i = 0; i < n; i++) {
            Gs_SetTexRegs(*tex, (u64 *)(pkt + 0x40), (u64 *)(pkt + 0x50), (u64 *)(pkt + 0x60),
                          (u64 *)(pkt + 0x70), (u64 *)(pkt + 0x80), (u64 *)(pkt + 0x90));
            pkt += step;
            tex++;
        }
        break;
    case 5:
        for (i = 0; i < n; i++) {
            Gs_SetTexRegs2(*tex, (u64 *)(pkt + 0x40), (u64 *)(pkt + 0x50), (u64 *)(pkt + 0x60),
                          (u64 *)(pkt + 0x70), (u64 *)(pkt + 0x80), (u64 *)(pkt + 0x90),
                          D_0020E134, (u64 *)(pkt + 0xA0), (u64 *)(pkt + 0xB0),
                          (u64 *)(pkt + 0xC0), (u64 *)(pkt + 0xD0), (u64 *)(pkt + 0xE0));
            pkt += step;
            tex++;
        }
        break;
    case 1:
    case 4:
        break;
    }
}

s32 Gs_BuildTexRegs(u32 handle, u64 *texa, u64 *tex1, u64 *tex0, u64 *clamp, u64 *mip1, u64 *mip2, u32 flags);

extern u32 gRenderFlags;
/* The GS texture registers (A+D pairs: TEXA, TEX1_1, TEX0_1, CLAMP_1, MIPTBP1_1, MIPTBP2_1)
 * for texture tex (Gs_BuildTexRegs, main), or cleared when tex is 0. */
void Gs_SetTexRegs(u32 tex, u64 *texa, u64 *tex1, u64 *tex0, u64 *clamp, u64 *mip1, u64 *mip2)
{
    if (tex != 0) {
        Gs_BuildTexRegs(tex, texa, tex1, tex0, clamp, mip1, mip2, gRenderFlags);
        texa[1] = 0x3B;
        tex1[1] = 0x14;
        tex0[1] = 0x06;
        clamp[1] = 0x08;
        mip1[1] = 0x34;
        mip2[1] = 0x36;
    } else {
        texa[0] = 0;
        texa[1] = 0x3B;
        tex1[0] = 0;
        tex1[1] = 0x14;
        tex0[0] = 0;
        tex0[1] = 0x06;
        clamp[0] = 0;
        clamp[1] = 0x08;
        mip1[0] = 0;
        mip1[1] = 0x34;
        mip2[0] = 0;
        mip2[1] = 0x36;
    }
}

/* As Gs_SetTexRegs for both drawing contexts: tex for context 1, tex2 for context 2 (whose
 * MIPTBP registers get 0x34 and 0x36 when it has a texture, 0x35 and 0x37 when not). */
void Gs_SetTexRegs2(u32 tex, u64 *texa, u64 *tex1, u64 *tex0, u64 *clamp, u64 *mip1, u64 *mip2,
                   u32 tex2, u64 *tex1b, u64 *tex0b, u64 *clampb, u64 *mip1b, u64 *mip2b)
{
    if (tex != 0) {
        Gs_BuildTexRegs(tex, texa, tex1, tex0, clamp, mip1, mip2, gRenderFlags);
        texa[1] = 0x3B;
        tex1[1] = 0x14;
        tex0[1] = 0x06;
        clamp[1] = 0x08;
        mip1[1] = 0x34;
        mip2[1] = 0x36;
    } else {
        texa[0] = 0;
        texa[1] = 0x3B;
        tex1[0] = 0;
        tex1[1] = 0x14;
        tex0[0] = 0;
        tex0[1] = 0x06;
        clamp[0] = 0;
        clamp[1] = 0x08;
        mip1[0] = 0;
        mip1[1] = 0x34;
        mip2[0] = 0;
        mip2[1] = 0x36;
    }
    if (tex2 != 0) {
        Gs_BuildTexRegs(tex2, texa, tex1b, tex0b, clampb, mip1b, mip2b, gRenderFlags);
        texa[1] = 0x3B;
        tex1b[1] = 0x15;
        tex0b[1] = 0x07;
        clampb[1] = 0x09;
        mip1b[1] = 0x34;
        mip2b[1] = 0x36;
    } else {
        texa[0] = 0;
        texa[1] = 0x3B;
        tex1b[0] = 0;
        tex1b[1] = 0x15;
        tex0b[0] = 0;
        tex0b[1] = 0x07;
        clampb[0] = 0;
        clampb[1] = 0x09;
        mip1b[0] = 0;
        mip1b[1] = 0x35;
        mip2b[0] = 0;
        mip2b[1] = 0x37;
    }
}

/* The textures of e: partTex[i] = the texture of part i's material, tex[] the distinct ones. */
static void Model_CollectTextures(ModelEntry *e, s32 unused)
{
    s32 i;
    s32 j;
    ModelMat *m;

    e->texCount = 0;
    for (i = 0; i < e->partCount; i++) {
        m = &gRsMaterials[e->mat[i]];
        e->partTex[i] = 0;
        if (m->tex != 0) {
            e->partTex[i] = m->tex;
            for (j = 0; j < e->texCount; j++) {
                if (e->tex[j] == m->tex) {
                    goto next;
                }
            }
            e->tex[e->texCount++] = m->tex;
        }
    next:;
    }
}

void Model_Relocate(ModelEntry *e);

/* Model_Relocate for every model in use that has its data. */
void Model_RelocateAll(void)
{
    s32 i;
    ModelEntry *e;

    i = 0;
    e = gModelPool;
    do {
        if (e->used != 0 && e->data != NULL) {
            Model_Relocate(e);
        }
        i++;
        e++;
    } while (i < 0x180);
}

/*
 * After model e's data moved (its handle compacted): the new address, each part's DMA
 * chain start, and the NEXT tags inside the chains relocated.
 */
void Model_Relocate(ModelEntry *e)
{
    u8 *old = e->dataAddr;
    u8 *base = Mem_GetAddr(e->data);
    u32 i;
    s32 *ofs;
    u8 *p;
    u8 *tag;
    u32 w;
    u8 *t;

    e->dataAddr = base;
    ofs = (s32 *)(base + e->unk2C);
    p = base + e->unk24;
    for (i = 0; i < e->partCount; i++, ofs++) {
        tag = base + *ofs;
        *(u8 **)(p + 4) = tag;
        t = tag;
        p += (*(u16 *)p + 1) * 16;
        do {
            w = *(u32 *)t;
            switch (w & 0x70000000) {
            case 0x10000000:
                t += ((w & 0xFFFF) + 1) * 16;
                break;
            case 0x20000000:
                t = *(u8 **)(t + 4) = *(u8 **)(t + 4) - (s32)old + (s32)base;
                break;
            case 0x60000000:
                tag = NULL;
                break;
            }
        } while (tag != NULL);
    }
}

s32 Dma_FlushQueue(void);
void Mem_Compact(void);
s32 Tex_MakeResident(s32 n, u32 *handles);
void Mem_CopyQwords(void *src, void *dst, u32 qwc);
void Model_SetTexRegs(s32 n, s32 prog, u8 *pkt, s32 *partTex);

/*
 * Draws model h: sets its data up again when the frame changed, builds or copies its
 * packets, makes its textures resident, runs its VU1 program's packet builder
 * (progInfo[4]) and queues the chains (with a CALL of the program when it changed).
 */
s32 Model_Draw(u32 h, s32 flags)
{
    ModelEntry *e = &gModelPool[h - 1];
    MeshDesc hdr;
    u8 *top;
    u32 *q;
    u32 *last;
    u8 *end;
    u8 *pkt;
    u8 *data = e->dataAddr;

    if (gSysWork.noDraw != 0) {
        return 1;
    }
    if (!(e->flags & 4)) {
        if (e->frame != gRenderFlags2) {
            Dma_FlushQueue();
            e->frame = gRenderFlags2;
            if (e->data != NULL) {
                Mem_Free(e->data);
            }
            Model_ReadHeader(e, &hdr);
            if (Model_Setup(e, &hdr) == 0) {
                return 0;
            }
            Mem_Compact();
            pkt = e->dataAddr + e->unk24;
        } else if (e->flags & 2) {
            pkt = data + e->unk24;
        } else {
            pkt = Packet_Alloc(e->pktSize, 0x10);
            Model_BuildPartPackets(e, e->partCount, e->progInfo[5], (s32)pkt, e->mat);
            Model_CollectTextures(e, 0);
        }
        if (e->texCount != 0) {
            Tex_MakeResident(e->texCount, (u32 *)e->tex);
            Model_SetTexRegs(e->partCount, e->progInfo[5], pkt, e->partTex);
        }
    } else if (e->flags & 2) {
        pkt = Packet_Alloc(e->pktSize, 0x10);
        Mem_CopyQwords(data + e->unk24, pkt, (u32)e->pktSize >> 4);
        if (e->texCount != 0) {
            Tex_MakeResident(e->texCount, (u32 *)e->tex);
            Model_SetTexRegs(e->partCount, e->progInfo[5], pkt, e->partTex);
        }
    } else {
        pkt = Packet_Alloc(e->pktSize, 0x10);
        Model_CollectTextures(e, 0);
        if (e->texCount != 0) {
            Tex_MakeResident(e->texCount, (u32 *)e->tex);
        }
        Model_BuildPartPackets(e, e->partCount, e->progInfo[5], (s32)pkt, e->mat);
    }
    end = pkt + e->partCount * gModelPktSteps[e->progInfo[5]];
    gSysWork.vu1Size = (u32)(e->progInfo[3] - e->progInfo[2]) >> 3;
    top = ((u8 *(*)(ModelEntry *, u32, u8 *))e->progInfo[4])(e, (u32)(e->progInfo[1] - e->progInfo[2]) >> 3, pkt);
    if (gSysWork.vu1Prog != e->prog) {
        gSysWork.vu1Prog = e->prog;
        q = Packet_Alloc(0x40, 0x10);
        q[0] = 0x50000001;
        q[1] = e->progInfo[0];
        *(u64 *)&q[2] = 0;
        last = q + 8;
        last[0] = 0xF0000001;
        last[1] = 0;
        *(u64 *)&last[2] = 0;
        *(u128 *)&q[12] = 0;
        *(u128 *)&q[4] = 0;
        DmaQueue_AddChain(0, (u32)q & 0x0FFFFFFF, last, &gDmaQueue);
    }
    DmaQueue_AddChain(0, ((u32)top & 0x0FFFFFFF) | 0x50000000, (u32 *)end, &gDmaQueue);
    return 1;
}

/* Frees model h (its handles and pool entry); 0 when h is not a model in use. */
s32 Model_FreeHandle(u32 h)
{
    ModelEntry *e = &gModelPool[h - 1];

    if (h == 0) {
        return 0;
    }
    if (h > 0x180) {
        return 0;
    }
    if (e->used == 0) {
        return 0;
    }
    Dma_FlushQueue();
    e->used = 0;
    if (e->file != NULL) {
        Mem_Free(e->file);
    }
    if (e->data != NULL) {
        Mem_Free(e->data);
    }
    Mem_Fill(e, 0, sizeof(ModelEntry));
    gModelCount--;
    return 1;
}

void Anim_SetCurveKeys(u8 *keys)
{
    gAnimCurveKeys = (s32) keys;
}

f32 Anim_EvalLinearF(AnimCurve *c, s16 *cur, f32 t);
f32 Anim_EvalCubicF(AnimCurve *c, s16 *cur, f32 t);
f32 Anim_EvalMixedF(AnimCurve *c, s16 *cur, f32 t);
f32 Anim_EvalLinearS(AnimCurve *c, s16 *cur, f32 t);
f32 Anim_EvalCubicS(AnimCurve *c, s16 *cur, f32 t);
f32 Anim_EvalMixedS(AnimCurve *c, s16 *cur, f32 t);
s16 Anim_FindKeysF2(AnimCurve *c, KeyF2 **pa, KeyF2 **pb, s16 cur, f32 t);
s16 Anim_FindKeysF4(AnimCurve *c, KeyF4 **pa, KeyF4 **pb, s16 cur, f32 t);
s16 Anim_FindKeysFM(AnimCurve *c, KeyFM **pa, KeyFM **pb, s16 cur, f32 t);
s16 Anim_FindKeysS2(AnimCurve *c, KeyS2 **pa, KeyS2 **pb, s16 cur, f32 t);
s16 Anim_FindKeysS4(AnimCurve *c, KeyS4 **pa, KeyS4 **pb, s16 cur, f32 t);
s16 Anim_FindKeysSM(AnimCurve *c, KeySM **pa, KeySM **pb, s16 cur, f32 t);
f32 Anim_LerpKeys(f32 x, f32 y0, f32 x0, f32 y1, f32 x1);
f32 Anim_CubicKeys(f32 x, f32 y0, f32 x0, f32 y1, f32 x1, f32 d0, f32 d1);

/* The value of curve c at time t (*cur: the key found last time, a hint kept by the caller). */
f32 Anim_EvalCurve(f32 t, AnimCurve *c, s16 *cur)
{
    f32 v;

    switch (c->type) {
    case ANIM_KEY_F32_LINEAR:
        v = Anim_EvalLinearF(c, cur, t);
        break;
    case ANIM_KEY_F32_CUBIC:
        v = Anim_EvalCubicF(c, cur, t);
        break;
    case ANIM_KEY_F32_MIXED:
        v = Anim_EvalMixedF(c, cur, t);
        break;
    case ANIM_KEY_S16_LINEAR:
        v = Anim_EvalLinearS(c, cur, t);
        break;
    case ANIM_KEY_S16_CUBIC:
        v = Anim_EvalCubicS(c, cur, t);
        break;
    case ANIM_KEY_S16_MIXED:
        v = Anim_EvalMixedS(c, cur, t);
        break;
    }
    return v;
}

/* Format 0x21: linear between f32 keys. */
f32 Anim_EvalLinearF(AnimCurve *c, s16 *cur, f32 t)
{
    KeyF2 *a = NULL;
    KeyF2 *b = NULL;

    *cur = Anim_FindKeysF2(c, &a, &b, *cur, t);
    if (b == NULL) {
        return a->v;
    }
    return Anim_LerpKeys(t, a->v, a->t, b->v, b->t);
}

/* Format 0x22: cubic between f32 keys. */
f32 Anim_EvalCubicF(AnimCurve *c, s16 *cur, f32 t)
{
    KeyF4 *a = NULL;
    KeyF4 *b = NULL;

    *cur = Anim_FindKeysF4(c, &a, &b, *cur, t);
    if (b == NULL) {
        return a->v;
    }
    return Anim_CubicKeys(t, a->v, a->t, a->out, b->v, b->t, b->in);
}

/* Format 0x23: f32 keys, linear (0x10000) or cubic (0x20000) by the first key of the pair. */
f32 Anim_EvalMixedF(AnimCurve *c, s16 *cur, f32 t)
{
    KeyFM *a = NULL;
    KeyFM *b = NULL;

    *cur = Anim_FindKeysFM(c, &a, &b, *cur, t);
    if (b == NULL) {
        return a->v;
    }
    switch (a->mode) {
    case ANIM_KEY_MODE_LINEAR:
        return Anim_LerpKeys(t, a->v, a->t, b->v, b->t);
    case ANIM_KEY_MODE_CUBIC:
        return Anim_CubicKeys(t, a->v, a->t, a->out, b->v, b->t, b->in);
    }
    return 0.0f;
}

/* Format 0x11: linear between s16 keys. */
f32 Anim_EvalLinearS(AnimCurve *c, s16 *cur, f32 t)
{
    KeyS2 *a = NULL;
    KeyS2 *b = NULL;

    *cur = Anim_FindKeysS2(c, &a, &b, *cur, t);
    if (b == NULL) {
        return a->v;
    }
    return Anim_LerpKeys(t, a->v, a->t, b->v, b->t);
}

/* Format 0x12: cubic between s16 keys. */
f32 Anim_EvalCubicS(AnimCurve *c, s16 *cur, f32 t)
{
    KeyS4 *a = NULL;
    KeyS4 *b = NULL;

    *cur = Anim_FindKeysS4(c, &a, &b, *cur, t);
    if (b == NULL) {
        return a->v;
    }
    return Anim_CubicKeys(t, a->v, a->t, a->out, b->v, b->t, b->in);
}

/* Format 0x13: s16 keys, linear or cubic by the first key of the pair. */
f32 Anim_EvalMixedS(AnimCurve *c, s16 *cur, f32 t)
{
    KeySM *a = NULL;
    KeySM *b = NULL;

    *cur = Anim_FindKeysSM(c, &a, &b, *cur, t);
    if (b == NULL) {
        return a->v;
    }
    switch (a->mode) {
    case ANIM_KEY_MODE_LINEAR:
        return Anim_LerpKeys(t, a->v, a->t, b->v, b->t);
    case ANIM_KEY_MODE_CUBIC:
        return Anim_CubicKeys(t, a->v, a->t, a->out, b->v, b->t, b->in);
    }
    return 0.0f;
}

/*
 * The keys of c around time t, searched from key cur: *pa and *pb = the keys before and
 * after t (*pb NULL at a key, before the first or after the last); returns *pa's index.
 * The five below do the same for the other key formats.
 */
s16 Anim_FindKeysF2(AnimCurve *c, KeyF2 **pa, KeyF2 **pb, s16 cur, f32 t)
{
    KeyF2 *k = (KeyF2 *)(c->keys + gAnimCurveKeys);

    *pa = NULL;
    *pb = NULL;
    if (t <= k[0].t || c->count == 1) {
        *pa = k;
        *pb = NULL;
        return 0;
    }
    if (!(t < k[c->count - 1].t)) {
        *pa = &k[c->count - 1];
        *pb = NULL;
        return c->count - 1;
    }
    if (cur < 0) {
        cur = 0;
    } else if (cur >= c->count) {
        cur = c->count - 1;
    }
    k += cur;
    for (;;) {
        if (k->t == t) {
            *pa = k;
            *pb = NULL;
            return cur;
        }
        if (k->t < t && t < k[1].t) {
            *pa = k;
            *pb = k + 1;
            return cur;
        }
        if (t < k->t) {
            k--;
            cur--;
        } else {
            k++;
            cur++;
        }
    }
}

s16 Anim_FindKeysF4(AnimCurve *c, KeyF4 **pa, KeyF4 **pb, s16 cur, f32 t)
{
    KeyF4 *k = (KeyF4 *)(c->keys + gAnimCurveKeys);

    *pa = NULL;
    *pb = NULL;
    if (t <= k[0].t || c->count == 1) {
        *pa = k;
        *pb = NULL;
        return 0;
    }
    if (!(t < k[c->count - 1].t)) {
        *pa = &k[c->count - 1];
        *pb = NULL;
        return c->count - 1;
    }
    if (cur < 0) {
        cur = 0;
    } else if (cur >= c->count) {
        cur = c->count - 1;
    }
    k += cur;
    for (;;) {
        if (k->t == t) {
            *pa = k;
            *pb = NULL;
            return cur;
        }
        if (k->t < t && t < k[1].t) {
            *pa = k;
            *pb = k + 1;
            return cur;
        }
        if (t < k->t) {
            k--;
            cur--;
        } else {
            k++;
            cur++;
        }
    }
}

s16 Anim_FindKeysFM(AnimCurve *c, KeyFM **pa, KeyFM **pb, s16 cur, f32 t)
{
    KeyFM *k = (KeyFM *)(c->keys + gAnimCurveKeys);

    *pa = NULL;
    *pb = NULL;
    if (t <= k[0].t || c->count == 1) {
        *pa = k;
        *pb = NULL;
        return 0;
    }
    if (!(t < k[c->count - 1].t)) {
        *pa = &k[c->count - 1];
        *pb = NULL;
        return c->count - 1;
    }
    if (cur < 0) {
        cur = 0;
    } else if (cur >= c->count) {
        cur = c->count - 1;
    }
    k += cur;
    for (;;) {
        if (k->t == t) {
            *pa = k;
            *pb = NULL;
            return cur;
        }
        if (k->t < t && t < k[1].t) {
            *pa = k;
            *pb = k + 1;
            return cur;
        }
        if (t < k->t) {
            k--;
            cur--;
        } else {
            k++;
            cur++;
        }
    }
}

s16 Anim_FindKeysS2(AnimCurve *c, KeyS2 **pa, KeyS2 **pb, s16 cur, f32 t)
{
    KeyS2 *k = (KeyS2 *)(c->keys + gAnimCurveKeys);

    *pa = NULL;
    *pb = NULL;
    if (t <= k[0].t || c->count == 1) {
        *pa = k;
        *pb = NULL;
        return 0;
    }
    if (!(t < k[c->count - 1].t)) {
        *pa = &k[c->count - 1];
        *pb = NULL;
        return c->count - 1;
    }
    if (cur < 0) {
        cur = 0;
    } else if (cur >= c->count) {
        cur = c->count - 1;
    }
    k += cur;
    for (;;) {
        if (k->t == t) {
            *pa = k;
            *pb = NULL;
            return cur;
        }
        if (k->t < t && t < k[1].t) {
            *pa = k;
            *pb = k + 1;
            return cur;
        }
        if (t < k->t) {
            k--;
            cur--;
        } else {
            k++;
            cur++;
        }
    }
}

s16 Anim_FindKeysS4(AnimCurve *c, KeyS4 **pa, KeyS4 **pb, s16 cur, f32 t)
{
    KeyS4 *k = (KeyS4 *)(c->keys + gAnimCurveKeys);

    *pa = NULL;
    *pb = NULL;
    if (t <= k[0].t || c->count == 1) {
        *pa = k;
        *pb = NULL;
        return 0;
    }
    if (!(t < k[c->count - 1].t)) {
        *pa = &k[c->count - 1];
        *pb = NULL;
        return c->count - 1;
    }
    if (cur < 0) {
        cur = 0;
    } else if (cur >= c->count) {
        cur = c->count - 1;
    }
    k += cur;
    for (;;) {
        if (k->t == t) {
            *pa = k;
            *pb = NULL;
            return cur;
        }
        if (k->t < t && t < k[1].t) {
            *pa = k;
            *pb = k + 1;
            return cur;
        }
        if (t < k->t) {
            k--;
            cur--;
        } else {
            k++;
            cur++;
        }
    }
}

s16 Anim_FindKeysSM(AnimCurve *c, KeySM **pa, KeySM **pb, s16 cur, f32 t)
{
    KeySM *k = (KeySM *)(c->keys + gAnimCurveKeys);

    *pa = NULL;
    *pb = NULL;
    if (t <= k[0].t || c->count == 1) {
        *pa = k;
        *pb = NULL;
        return 0;
    }
    if (!(t < k[c->count - 1].t)) {
        *pa = &k[c->count - 1];
        *pb = NULL;
        return c->count - 1;
    }
    if (cur < 0) {
        cur = 0;
    } else if (cur >= c->count) {
        cur = c->count - 1;
    }
    k += cur;
    for (;;) {
        if (k->t == t) {
            *pa = k;
            *pb = NULL;
            return cur;
        }
        if (k->t < t && t < k[1].t) {
            *pa = k;
            *pb = k + 1;
            return cur;
        }
        if (t < k->t) {
            k--;
            cur--;
        } else {
            k++;
            cur++;
        }
    }
}
/* The line through (x0, y0) and (x1, y1) at x. */
asm f32 Anim_LerpKeys(f32 x, f32 y0, f32 x0, f32 y1, f32 x1)
{
    .set noreorder
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf3
    mfc1 t0, f14
    nop
    qmtc2.ni t0, vf4
    mfc1 t0, f16
    nop
    qmtc2.ni t0, vf5
    mfc1 t0, f13
    nop
    qmtc2.ni t0, vf6
    mfc1 t0, f15
    nop
    qmtc2.ni t0, vf7
    vsub.x vf8, vf5, vf4
    vdiv Q, vf0w, vf8x
    vsub.x vf9, vf7, vf6
    vmul.x vf10, vf5, vf6
    vmul.x vf11, vf4, vf7
    vsub.x vf10, vf10, vf11
    vwaitq
    vmulq.x vf9, vf9, Q
    vmulq.x vf10, vf10, Q
    vmul.x vf4, vf9, vf3
    vadd.x vf4, vf4, vf10
    qmfc2.ni t0, vf4
    mtc1 t0, f0
    jr ra
    nop
}
/* A cubic curve between two keys, at x (from the key values and two slopes). */
asm f32 Anim_CubicKeys(f32 x, f32 y0, f32 x0, f32 y1, f32 x1, f32 d0, f32 d1)
{
    .set noreorder
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf3
    mfc1 t0, f14
    nop
    qmtc2.ni t0, vf4
    mfc1 t0, f17
    nop
    qmtc2.ni t0, vf5
    mfc1 t0, f13
    nop
    qmtc2.ni t0, vf6
    mfc1 t0, f16
    nop
    qmtc2.ni t0, vf7
    mfc1 t0, f15
    nop
    qmtc2.ni t0, vf8
    mfc1 t0, f18
    nop
    qmtc2.ni t0, vf2
    vsub.x vf9, vf3, vf4
    vsub.x vf10, vf5, vf4
    vdiv Q, vf0w, vf10x
    vwaitq
    vaddq.x vf10, vf0, Q
    vmul.x vf11, vf10, vf10
    vmul.x vf12, vf9, vf9
    vmul.x vf13, vf12, vf10
    vmul.x vf14, vf12, vf9
    vmul.x vf14, vf14, vf11
    vaddw.x vf1, vf0, vf0w
    vadd.x vf1, vf1, vf1
    vmul.x vf15, vf1, vf14
    vmul.x vf15, vf15, vf10
    vaddw.x vf1, vf1, vf0w
    vmul.x vf16, vf1, vf12
    vmul.x vf16, vf16, vf11
    vsub.x vf17, vf15, vf16
    vaddw.x vf17, vf17, vf0w
    vmul.x vf17, vf6, vf17
    vsub.x vf18, vf16, vf15
    vmul.x vf18, vf7, vf18
    vadd.x vf17, vf17, vf18
    vsub.x vf18, vf14, vf13
    vsub.x vf18, vf18, vf13
    vadd.x vf18, vf18, vf9
    vmul.x vf18, vf8, vf18
    vadd.x vf17, vf17, vf18
    vsub.x vf18, vf14, vf13
    vmul.x vf18, vf2, vf18
    vadd.x vf17, vf17, vf18
    qmfc2.ni t0, vf17
    mtc1 t0, f0
    jr ra
    nop
}
extern f32 gSinVu0Coefs[];
extern f32 gSinVu0Poly[];
/* The sqrt(1 - s * s) half of Math_SinCos (Math_Sin calls it). */
asm f32 Math_SinVu0(f32 x)
{
    .set noreorder
    /* Handwritten function */
    lui v0, (0xC0490FDB >> 16)
    ori v0, v0, (0xC0490FDB & 0xFFFF)
    mtc1 v0, f0
    nop
    c.le.s f12, f0
    bc1t L002EC308
    lui v0, (0x40490FDB >> 16)
    ori v0, v0, (0x40490FDB & 0xFFFF)
    mtc1 v0, f0
    nop
    c.lt.s f12, f0
    bc1t L002EC358
    nop
    nop
L002EC308:
    la v0, gSinVu0Coefs
    mfc1 t0, f12
    lqc2 vf6, 0x0(v0)
    qmtc2.ni t0, vf4
    slt t0, zero, t0
    ctc2.ni t0, vi2
    vmulx.x vf7, vf4, vf6x
    vftoi0.x vf7, vf7
    vmtir vi1, vf7x
    viaddi vi3, vi0, -0x2
    viadd vi1, vi1, vi2
    viand vi1, vi1, vi3
    vmfir.x vf7, vi1
    vitof0.x vf7, vf7
    vmuly.x vf7, vf7, vf6y
    vsub.x vf7, vf4, vf7
    qmfc2.ni t0, vf7
    mtc1 t0, f12
    nop
L002EC358:
    la v1, gSinVu0Poly
    mtc1 zero, f0
    nop
    c.lt.s f12, f0
    lui at, (0x3FC90FDB >> 16)
    ori v0, at, (0x3FC90FDB & 0xFFFF)
    bc1f L002EC390
    mtc1 v0, f0
    vnop
    add.s f12, f0, f12
    j L002EC398
    addi a3, zero, 0x1 /* handwritten instruction */
    vnop
L002EC390:
    sub.s f12, f0, f12
    addu a3, zero, zero
L002EC398:
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf6
    vmulx.x vf7, vf6, vf6x
    lqc2 vf5, 0x0(v1)
    vmulx.xyzw vf5, vf5, vf6x
    vmulaw.x ACC, vf6, vf0w
    vmulx.x vf8, vf7, vf7x
    vsub.zw vf6, vf0, vf0
    vmulx.x vf9, vf7, vf8x
    vmulx.x vf10, vf8, vf8x
    vmaddaw.x ACC, vf7, vf5w
    vmaddaz.x ACC, vf8, vf5z
    vmadday.x ACC, vf9, vf5y
    vmaddx.x vf6, vf10, vf5x
    vmul.x vf7, vf6, vf6
    vsubx.w vf7, vf0, vf7x
    vsqrt Q, vf7w
    bnez a3, L002EC3F8
    vwaitq
    vnop
    b L002EC400
    vaddq.x vf6, vf0, Q
    vnop
L002EC3F8:
    vsubq.x vf6, vf0, Q
    nop
L002EC400:
    qmfc2.ni t0, vf6
    mtc1 t0, f0
    jr ra
    nop
}
/* The polynomial half of Math_SinCos. */
asm f32 Math_CosVu0(f32 x)
{
    .set noreorder
    lui v0, (0xC0490FDB >> 16)
    ori v0, v0, (0xC0490FDB & 0xFFFF)
    mtc1 v0, f0
    nop
    c.le.s f12, f0
    bc1t L002EC448
    lui v0, (0x40490FDB >> 16)
    ori v0, v0, (0x40490FDB & 0xFFFF)
    mtc1 v0, f0
    nop
    c.lt.s f12, f0
    bc1t L002EC498
    nop
    nop
L002EC448:
    la v0, gSinVu0Coefs
    mfc1 t0, f12
    lqc2 vf6, 0x0(v0)
    qmtc2.ni t0, vf4
    slt t0, zero, t0
    ctc2.ni t0, vi2
    vmulx.x vf7, vf4, vf6x
    vftoi0.x vf7, vf7
    vmtir vi1, vf7x
    viaddi vi3, vi0, -0x2
    viadd vi1, vi1, vi2
    viand vi1, vi1, vi3
    vmfir.x vf7, vi1
    vitof0.x vf7, vf7
    vmuly.x vf7, vf7, vf6y
    vsub.x vf7, vf4, vf7
    qmfc2.ni t0, vf7
    mtc1 t0, f12
    nop
L002EC498:
    la v1, gSinVu0Poly
    mtc1 zero, f0
    nop
    c.lt.s f12, f0
    lui at, (0x3FC90FDB >> 16)
    ori v0, at, (0x3FC90FDB & 0xFFFF)
    bc1f L002EC4D0
    mtc1 v0, f0
    vnop
    j L002EC4D8
    add.s f12, f0, f12
    vnop
    nop
L002EC4D0:
    sub.s f12, f0, f12
    nop
L002EC4D8:
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf6
    vmulx.x vf7, vf6, vf6x
    lqc2 vf5, 0x0(v1)
    vmulx.xyzw vf5, vf5, vf6x
    vmulaw.x ACC, vf6, vf0w
    vmulx.x vf8, vf7, vf7x
    vsub.yzw vf6, vf0, vf0
    vmulx.x vf9, vf7, vf8x
    vmulx.x vf10, vf8, vf8x
    vmaddaw.x ACC, vf7, vf5w
    vmaddaz.x ACC, vf8, vf5z
    vmadday.x ACC, vf9, vf5y
    vmaddx.x vf6, vf10, vf5x
    vnop
    qmfc2.ni t1, vf6
    mtc1 t1, f0
    jr ra
    nop
}
/* out[0] = sin a, out[1] = cos a (VU0 polynomial). Static: Math_GetSinCos
 * keeps values in t1 and a2 across the call, which MWCC only does for a
 * static callee it has compiled. */
static asm void Math_SinCos(f32 *out, f32 a)
{
    .set noreorder
    /* Handwritten function */
    lui v1, (0xC0490FDB >> 16)
    ori v1, v1, (0xC0490FDB & 0xFFFF)
    mtc1 v1, f0
    nop
    c.le.s f12, f0
    bc1t L002EC568
    lui v1, (0x40490FDB >> 16)
    ori v1, v1, (0x40490FDB & 0xFFFF)
    mtc1 v1, f0
    nop
    c.lt.s f12, f0
    bc1t L002EC5B8
    nop
    nop
L002EC568:
    la v1, gSinVu0Coefs
    mfc1 t0, f12
    lqc2 vf6, 0x0(v1)
    qmtc2.ni t0, vf4
    slt t0, zero, t0
    ctc2.ni t0, vi2
    vmulx.x vf7, vf4, vf6x
    vftoi0.x vf7, vf7
    vmtir vi1, vf7x
    viaddi vi3, vi0, -0x2
    viadd vi1, vi1, vi2
    viand vi1, vi1, vi3
    vmfir.x vf7, vi1
    vitof0.x vf7, vf7
    vmuly.x vf7, vf7, vf6y
    vsub.x vf7, vf4, vf7
    qmfc2.ni t0, vf7
    mtc1 t0, f12
    nop
L002EC5B8:
    la a1, gSinVu0Poly
    mtc1 zero, f0
    nop
    c.lt.s f12, f0
    lui at, (0x3FC90FDB >> 16)
    ori v1, at, (0x3FC90FDB & 0xFFFF)
    bc1f L002EC5F0
    mtc1 v1, f0
    vnop
    add.s f12, f0, f12
    j L002EC5F8
    addi a3, zero, 0x1 /* handwritten instruction */
    vnop
L002EC5F0:
    sub.s f12, f0, f12
    addu a3, zero, zero
L002EC5F8:
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf6
    vmulx.x vf7, vf6, vf6x
    lqc2 vf5, 0x0(a1)
    vmulx.xyzw vf5, vf5, vf6x
    vmulaw.x ACC, vf6, vf0w
    vmulx.x vf8, vf7, vf7x
    vsub.zw vf6, vf0, vf0
    vmulx.x vf9, vf7, vf8x
    vmulx.x vf10, vf8, vf8x
    vmaddaw.x ACC, vf7, vf5w
    vmaddaz.x ACC, vf8, vf5z
    vmadday.x ACC, vf9, vf5y
    vmaddx.x vf6, vf10, vf5x
    vmul.x vf7, vf6, vf6
    vaddx.y vf6, vf0, vf6x
    vsubx.w vf7, vf0, vf7x
    vsqrt Q, vf7w
    bnez a3, L002EC660
    vwaitq
    vnop
    b L002EC668
    vaddq.x vf6, vf0, Q
    vnop
    nop
L002EC660:
    vsubq.x vf6, vf0, Q
    nop
L002EC668:
    jr ra
    sqc2 vf6, 0x0(a0)
}

extern Mtx44Copy gMtxSlots[];

/* The camera's matrices. */
extern f32 gMtxProj[16];
extern f32 gViewportMtx[16];
extern f32 gMtxProjClip[16];
extern f32 gMtxProjScreen[16];
extern f32 gMtxScreen[16];
extern f32 gMtxCamProj[16];
extern f32 gMtxClip[16];

/*
 * main's render state (the viewport), named in main.txt
 * (gSysFrameMode and gDispWidth are declared at the top of the file).
 */
extern f32 gViewClipH;
extern f32 gViewClipW;
extern f32 gViewCenterY;
extern f32 gViewCenterX;
extern f32 gViewH;
extern f32 gViewW;
extern f32 gViewY;
extern f32 gViewX;
extern f32 gSysZMax;
extern s32 gDispHeight;

void Mtx_Perspective(f32 *m, f32 n, f32 f, f32 fov, f32 aspect);
void Camera_StoreProjection(f32 *m);
void Camera_SetViewport(f32 *m, s32 x, s32 y, s32 w, s32 h, f32 n, f32 f);
void Camera_SetViewportMtx(s32 x, s32 y, s32 w, s32 h, f32 n, f32 f);
void Mtx_Copy(f32 *dst, f32 *src);
s32 Gs_SetScissor(s32 x, s32 y, s32 w, s32 h, s32 ctx);
void Math_GetSinCos(f32 *s, f32 *c, f32 a);
asm void Mtx_Identity(f32 *m);
asm void Mtx_Mul(f32 *dst, f32 *a, f32 *b);

/* Matrix slot i = a perspective projection (Mtx_Perspective, Camera_StoreProjection), then the camera matrices. */
void Camera_SetPerspective(s32 i, f32 n, f32 f, f32 fov, f32 aspect)
{
    f32 *m = gMtxSlots[i].m[0];

    Mtx_Perspective(m, n, f, fov, aspect);
    Camera_StoreProjection(m);
    Mtx_Mul(gMtxProjClip, gMtxCamProj, gMtxClip);
    Mtx_Mul(gMtxProjScreen, gMtxCamProj, gMtxScreen);
}

/* m = a perspective projection: near n, far f, vertical field of view fov, aspect ratio. */
void Mtx_Perspective(f32 *m, f32 n, f32 f, f32 fov, f32 aspect)
{
    f32 c;
    f32 s;

    fov *= 0.5f;
    Math_GetSinCos(&s, &c, fov);
    Mtx_Identity(m);
    m[0] = c / s / aspect;
    m[5] = c / s;
    m[10] = -(n + f) / (n - f);
    m[11] = -1.0f;
    m[14] = -(2.0f * n * f) / (n - f);
    m[15] = 0.0f;
}

/* The x and y scale of m (a projection) for the viewport; the result in gViewportMtx. */
/* Reads main's render-state globals by name (config/symbols/main.txt): with raw
 * addresses MWCC can't move their loads above the matrix stores. */
void Camera_StoreProjection(f32 *m)
{
    Mtx_Copy(gViewportMtx, m);
    gViewportMtx[0] = 2.0f * gViewportMtx[0] / (gViewClipW / (0.5f * gViewW));
    gViewportMtx[5] = 2.0f * gViewportMtx[5] / (gViewClipH / (0.5f * gViewH));
}

/* Matrix slot i = an orthographic screen matrix for the viewport (x, y, w, h), then the camera
 * matrices and the GS viewport (Gs_SetScissor; y and h halved while gSysFrameMode is 0). */
void Camera_SetOrtho(s32 i, s32 x, s32 y, s32 w, s32 h, f32 n, f32 f)
{
    Camera_SetViewport(gMtxSlots[i].m[0], x, y, w, h, n, f);
    Camera_StoreProjection(gMtxCamProj);
    Mtx_Mul(gMtxProjClip, gMtxCamProj, gMtxClip);
    Mtx_Mul(gMtxProjScreen, gMtxCamProj, gMtxScreen);
    if (gSysFrameMode == 0) {
        y /= 2;
        h /= 2;
    }
    Gs_SetScissor(x, y, w, h, 2);
}

/*
 * The viewport (x, y, w, h; depth n to f) into gMtxScreen and the screen matrix m, and
 * the viewport globals of main; then Camera_SetViewportMtx.
 */
/* Reads main's render-state globals by name (config/symbols/main.txt): with raw
 * addresses MWCC can't move their loads above the matrix stores. */
void Camera_SetViewport(f32 *m, s32 x, s32 y, s32 w, s32 h, f32 n, f32 f)
{
    f32 hw;
    f32 hh;
    f32 d;

    hw = w / 2;
    hh = h / 2;
    Mtx_Identity(gMtxScreen);
    gMtxScreen[0] = hw;
    gMtxScreen[5] = -hh;
    d = f - n;
    gMtxScreen[10] = d;
    gMtxScreen[12] = x + hw;
    gMtxScreen[13] = y + hh;
    gMtxScreen[14] = n;
    gMtxScreen[15] = 1.0f;
    if (gSysFrameMode == 0) {
        y /= 2;
        h /= 2;
    }
    hw = w / 2;
    hh = h / 2;
    Mtx_Identity(m);
    m[0] = hw;
    m[5] = -hh;
    m[10] = d;
    m[12] = x + hw;
    m[13] = y + hh;
    m[15] = 1.0f;
    m[10] *= 0.5f * -gSysZMax;
    m[12] += 2048 - (gDispWidth >> 1);
    m[13] += 2048 - (gDispHeight >> 1);
    m[14] = 0.5f * gSysZMax;
    gViewCenterX = m[12];
    gViewCenterY = m[13];
    gViewX = x;
    gViewY = y;
    gViewW = w;
    gViewH = h;
    gViewClipW = 2048.0f - gViewCenterX;
    if (gViewClipW < 0.0f) {
        gViewClipW *= -1.0f;
    }
    gViewClipW = 2048.0f - gViewClipW;
    gViewClipH = 2048.0f - gViewCenterY;
    if (gViewClipH < 0.0f) {
        gViewClipH *= -1.0f;
    }
    gViewClipH = 2048.0f - gViewClipH;
    Camera_SetViewportMtx(0, 0, gViewClipW, gViewClipH, n, f);
}

/* gMtxProj = the viewport matrix for a w x h screen (depth n to f). */
/* Reads main's render-state globals by name (config/symbols/main.txt): with raw
 * addresses MWCC can't move their loads above the matrix stores. */
void Camera_SetViewportMtx(s32 x, s32 y, s32 w, s32 h, f32 n, f32 f)
{
    f32 hw;
    f32 hh;

    Mtx_Identity(gMtxProj);
    hw = w / 2;
    hh = h / 2;
    gMtxProj[0] = hw;
    gMtxProj[5] = -hh;
    gMtxProj[15] = 1.0f;
    gMtxProj[10] = f - n;
    gMtxProj[10] *= 0.5f * -gSysZMax;
    gMtxProj[14] = 0.5f * gSysZMax;
    gMtxProj[12] = gViewCenterX;
    gMtxProj[13] = gViewCenterY;
}
extern u8 gMtxIdentity[];
/* m = the identity matrix (the copy at 0x20D7D0). */
asm void Mtx_Identity(f32 *m)
{
    .set noreorder
    la v1, gMtxIdentity
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    lqc2 vf7, 0x30(v1)
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    sqc2 vf6, 0x20(a0)
    jr ra
    sqc2 vf7, 0x30(a0)
}
/* The 3x3 part of m becomes the identity; the translation stays. */
asm void Mtx_ClearRotation(f32 *m)
{
    .set noreorder
    la v1, gMtxIdentity
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    lqc2 vf8, 0x0(a0)
    lqc2 vf9, 0x10(a0)
    lqc2 vf10, 0x20(a0)
    vadd.xyz vf8, vf0, vf4
    vadd.xyz vf9, vf0, vf5
    vadd.xyz vf10, vf0, vf6
    sqc2 vf8, 0x0(a0)
    sqc2 vf9, 0x10(a0)
    jr ra
    sqc2 vf10, 0x20(a0)
}
/* m = a scale matrix. */
asm void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z)
{
    .set noreorder
    la v1, gMtxIdentity
    mfc1 a2, f12
    mfc1 a3, f13
    mfc1 t0, f14
    lqc2 vf4, 0x0(v1)
    qmtc2.ni a2, vf8
    qmtc2.ni a3, vf9
    qmtc2.ni t0, vf10
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    lqc2 vf7, 0x30(v1)
    vaddx.x vf4, vf0, vf8x
    vaddx.y vf5, vf0, vf9x
    vaddx.z vf6, vf0, vf10x
    sqc2 vf7, 0x30(a0)
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    jr ra
    sqc2 vf6, 0x20(a0)
}
/* The 3x3 part of m becomes a scale; the translation stays. */
asm void Mtx_SetScale(f32 *m, f32 x, f32 y, f32 z)
{
    .set noreorder
    la v1, gMtxIdentity
    mfc1 a2, f12
    mfc1 a3, f13
    mfc1 t0, f14
    lqc2 vf4, 0x0(v1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    qmtc2.ni t0, vf22
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    lqc2 vf8, 0x0(a0)
    lqc2 vf9, 0x10(a0)
    lqc2 vf10, 0x20(a0)
    vaddx.x vf4, vf0, vf20x
    vaddx.y vf5, vf0, vf21x
    vaddx.z vf6, vf0, vf22x
    vaddx.w vf4, vf8, vf0x
    vaddx.w vf5, vf9, vf0x
    vaddx.w vf6, vf10, vf0x
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    jr ra
    sqc2 vf6, 0x20(a0)
}
/* m = a translation matrix. */
asm void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z)
{
    .set noreorder
    la v1, gMtxIdentity
    mfc1 a2, f12
    mfc1 a3, f13
    mfc1 t0, f14
    lqc2 vf7, 0x30(v1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    qmtc2.ni t0, vf22
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    vaddx.x vf7, vf0, vf20x
    vaddx.y vf7, vf0, vf21x
    vaddx.z vf7, vf0, vf22x
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    sqc2 vf6, 0x20(a0)
    jr ra
    sqc2 vf7, 0x30(a0)
}

void Mtx_SetScale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);

/* The 3x3 part of m = m * scale(x, y, z). */
void Mtx_ScaleBy(f32 *m, f32 x, f32 y, f32 z)
{
    f32 tmp[16];

    Mtx_SetScale(tmp, x, y, z);
    Mtx_Concat3x3(m, tmp);
}

void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z)
{
    m[12] = x;
    m[13] = y;
    m[14] = z;
}

void Mtx_AddTrans(f32 *m, f32 *v)
{
    m[12] = (m[12] + v[0]);
    m[13] = (m[13] + v[1]);
    m[14] = (m[14] + v[2]);
}

void Mtx_GetTrans(f32 *out, f32 *m)
{
    out[0] = m[12];
    out[1] = m[13];
    out[2] = m[14];
}
/* m = m * (rotation about x). */
asm void Mtx_RotateX(f32 *m, f32 a)
{
    .set noreorder
    addiu sp, sp, -0x20
    daddu t1, a0, zero
    sd ra, 0x0(sp)
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x20
}
/* m = m * (rotation about y). */
asm void Mtx_RotateY(f32 *m, f32 a)
{
    .set noreorder
    addiu sp, sp, -0x20
    daddu t1, a0, zero
    sd ra, 0x0(sp)
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x20
}
/* m = m * (rotation about z). */
asm void Mtx_RotateZ(f32 *m, f32 a)
{
    .set noreorder
    addiu sp, sp, -0x20
    daddu t1, a0, zero
    sd ra, 0x0(sp)
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x20
}
/* The rotation part of m = rotations about x, then y, then z. */
asm void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x60
    sd ra, 0x20(sp)
    sq s0, 0x10(sp)
    swc1 f22, 0x8(sp)
    daddu s0, a0, zero
    swc1 f21, 0x4(sp)
    swc1 f20, 0x0(sp)
    mov.s f22, f12
    mov.s f21, f13
    jal Mtx_ClearRotation
    mov.s f20, f14
    mov.s f12, f22
    jal Math_SinCos
    addiu a0, sp, 0x50
    lwc1 f1, 0x50(sp)
    lwc1 f0, 0x54(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    mov.s f12, f21
    jal Math_SinCos
    addiu a0, sp, 0x40
    lwc1 f1, 0x40(sp)
    lwc1 f0, 0x44(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    mov.s f12, f20
    jal Math_SinCos
    addiu a0, sp, 0x30
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    ld ra, 0x20(sp)
    lwc1 f22, 0x8(sp)
    lq s0, 0x10(sp)
    lwc1 f21, 0x4(sp)
    lwc1 f20, 0x0(sp)
    jr ra
    addiu sp, sp, 0x60
}
/* The rotation part of m = rotations about z, then y, then x. */
asm void Mtx_SetRotZYX(f32 *m, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x60
    sd ra, 0x20(sp)
    sq s0, 0x10(sp)
    swc1 f22, 0x8(sp)
    daddu s0, a0, zero
    swc1 f21, 0x4(sp)
    swc1 f20, 0x0(sp)
    mov.s f22, f12
    mov.s f21, f13
    jal Mtx_ClearRotation
    mov.s f20, f14
    mov.s f12, f20
    jal Math_SinCos
    addiu a0, sp, 0x50
    lwc1 f1, 0x50(sp)
    lwc1 f0, 0x54(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    mov.s f12, f21
    jal Math_SinCos
    addiu a0, sp, 0x40
    lwc1 f1, 0x40(sp)
    lwc1 f0, 0x44(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    mov.s f12, f22
    jal Math_SinCos
    addiu a0, sp, 0x30
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    ld ra, 0x20(sp)
    lwc1 f22, 0x8(sp)
    lq s0, 0x10(sp)
    lwc1 f21, 0x4(sp)
    lwc1 f20, 0x0(sp)
    jr ra
    addiu sp, sp, 0x60
}
/* m rotated about x, then y, then z. */
asm void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x40
    daddu t1, a0, zero
    sd ra, 0x0(sp)
    jal Math_SinCos
    addiu a0, sp, 0x30
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f13
    jal Math_SinCos
    addiu a0, sp, 0x20
    lwc1 f1, 0x20(sp)
    lwc1 f0, 0x24(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f14
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x40
}
/* m rotated about x, then z, then y. */
asm void Mtx_RotateXZY(f32 *m, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x40
    daddu t1, a0, zero
    sd ra, 0x0(sp)
    jal Math_SinCos
    addiu a0, sp, 0x30
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f14
    jal Math_SinCos
    addiu a0, sp, 0x20
    lwc1 f1, 0x20(sp)
    lwc1 f0, 0x24(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f13
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x40
}
/* m rotated about y, then x, then z. */
asm void Mtx_RotateYXZ(f32 *m, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x40
    daddu t1, a0, zero
    mov.s f2, f12
    sd ra, 0x0(sp)
    addiu a0, sp, 0x30
    jal Math_SinCos
    mov.s f12, f13
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f2
    jal Math_SinCos
    addiu a0, sp, 0x20
    lwc1 f1, 0x20(sp)
    lwc1 f0, 0x24(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f14
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x40
}
/* m rotated about z, then x, then y. */
asm void Mtx_RotateZXY(f32 *m, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x40
    daddu t1, a0, zero
    mov.s f2, f12
    sd ra, 0x0(sp)
    addiu a0, sp, 0x30
    jal Math_SinCos
    mov.s f12, f14
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f2
    jal Math_SinCos
    addiu a0, sp, 0x20
    lwc1 f1, 0x20(sp)
    lwc1 f0, 0x24(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f13
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x40
}

typedef struct Vec3Copy {
    f32 x, y, z;
} Vec3Copy;

asm void Vec_Normalize(f32 *v);
asm f32 Vec_Dot(f32 *a, f32 *b);
asm void Vec_Cross(f32 *out, f32 *a, f32 *b);

/* m = the view matrix of a camera at eye looking at at, with up as the up direction. */
void Mtx_LookAt(f32 *m, f32 *eye, f32 *at, f32 *up)
{
    f32 x[3];
    f32 y[3];
    f32 z[3];
    Vec3Copy u;

    z[0] = eye[0] - at[0];
    z[1] = eye[1] - at[1];
    z[2] = eye[2] - at[2];
    u = *(Vec3Copy *)up;
    Vec_Normalize(z);
    Vec_Normalize((f32 *)&u);
    Vec_Cross(x, (f32 *)&u, z);
    Vec_Normalize(x);
    Vec_Cross(y, z, x);
    m[3] = 0.0f;
    m[7] = 0.0f;
    m[11] = 0.0f;
    m[15] = 1.0f;
    m[0] = x[0];
    m[4] = x[1];
    m[8] = x[2];
    m[1] = y[0];
    m[5] = y[1];
    m[9] = y[2];
    m[2] = z[0];
    m[6] = z[1];
    m[10] = z[2];
    m[12] = -Vec_Dot(eye, x);
    m[13] = -Vec_Dot(eye, y);
    m[14] = -Vec_Dot(eye, z);
}
/* Transposes the 3x3 part of m. */
asm void Mtx_Transpose3x3(f32 *m)
{
    .set noreorder
    lqc2 vf8, 0x0(a0)
    lqc2 vf9, 0x10(a0)
    lqc2 vf10, 0x20(a0)
    vaddx.x vf4, vf0, vf8x
    vaddy.x vf5, vf0, vf8y
    vaddz.x vf6, vf0, vf8z
    vaddx.y vf4, vf0, vf9x
    vaddy.y vf5, vf0, vf9y
    vaddz.y vf6, vf0, vf9z
    vaddx.z vf4, vf0, vf10x
    vaddy.z vf5, vf0, vf10y
    vaddz.z vf6, vf0, vf10z
    vaddx.w vf4, vf8, vf0x
    vaddx.w vf5, vf9, vf0x
    vaddx.w vf6, vf10, vf0x
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    jr ra
    sqc2 vf6, 0x20(a0)
}
/* dst = a * b (4x4, row vectors). */
asm void Mtx_Mul(f32 *dst, f32 *a, f32 *b)
{
    .set noreorder
    lqc2 vf4, 0x0(a2)
    lqc2 vf5, 0x10(a2)
    lqc2 vf6, 0x20(a2)
    lqc2 vf7, 0x30(a2)
    lqc2 vf8, 0x0(a1)
    lqc2 vf9, 0x10(a1)
    lqc2 vf10, 0x20(a1)
    lqc2 vf11, 0x30(a1)
    vmulax.xyzw ACC, vf4, vf8x
    vmadday.xyzw ACC, vf5, vf8y
    vmaddaz.xyzw ACC, vf6, vf8z
    vmaddw.xyzw vf8, vf7, vf8w
    vmulax.xyzw ACC, vf4, vf9x
    vmadday.xyzw ACC, vf5, vf9y
    vmaddaz.xyzw ACC, vf6, vf9z
    vmaddw.xyzw vf9, vf7, vf9w
    vmulax.xyzw ACC, vf4, vf10x
    vmadday.xyzw ACC, vf5, vf10y
    vmaddaz.xyzw ACC, vf6, vf10z
    vmaddw.xyzw vf10, vf7, vf10w
    vmulax.xyzw ACC, vf4, vf11x
    vmadday.xyzw ACC, vf5, vf11y
    vmaddaz.xyzw ACC, vf6, vf11z
    vmaddw.xyzw vf11, vf7, vf11w
    sqc2 vf8, 0x0(a0)
    sqc2 vf9, 0x10(a0)
    sqc2 vf10, 0x20(a0)
    sqc2 vf11, 0x30(a0)
    jr ra
    nop
}
/* m = m * a (4x4). */
asm void Mtx_Concat(f32 *m, f32 *a)
{
    .set noreorder
    lqc2 vf4, 0x0(a1)
    lqc2 vf5, 0x10(a1)
    lqc2 vf6, 0x20(a1)
    lqc2 vf7, 0x30(a1)
    lqc2 vf8, 0x0(a0)
    lqc2 vf9, 0x10(a0)
    lqc2 vf10, 0x20(a0)
    lqc2 vf11, 0x30(a0)
    vmulax.xyzw ACC, vf4, vf8x
    vmadday.xyzw ACC, vf5, vf8y
    vmaddaz.xyzw ACC, vf6, vf8z
    vmaddw.xyzw vf8, vf7, vf8w
    vmulax.xyzw ACC, vf4, vf9x
    vmadday.xyzw ACC, vf5, vf9y
    vmaddaz.xyzw ACC, vf6, vf9z
    vmaddw.xyzw vf9, vf7, vf9w
    vmulax.xyzw ACC, vf4, vf10x
    vmadday.xyzw ACC, vf5, vf10y
    vmaddaz.xyzw ACC, vf6, vf10z
    vmaddw.xyzw vf10, vf7, vf10w
    vmulax.xyzw ACC, vf4, vf11x
    vmadday.xyzw ACC, vf5, vf11y
    vmaddaz.xyzw ACC, vf6, vf11z
    vmaddw.xyzw vf11, vf7, vf11w
    sqc2 vf8, 0x0(a0)
    sqc2 vf9, 0x10(a0)
    sqc2 vf10, 0x20(a0)
    sqc2 vf11, 0x30(a0)
    jr ra
    nop
}
/* The 3x3 part of dst = a * b (3x3); the rest of dst stays. */
asm void Mtx_Mul3x3(f32 *dst, f32 *a, f32 *b)
{
    .set noreorder
    lqc2 vf4, 0x0(a2)
    lqc2 vf5, 0x10(a2)
    lqc2 vf6, 0x20(a2)
    lqc2 vf8, 0x0(a1)
    lqc2 vf9, 0x10(a1)
    lqc2 vf10, 0x20(a1)
    lqc2 vf11, 0x0(a0)
    lqc2 vf12, 0x10(a0)
    lqc2 vf13, 0x20(a0)
    vmulax.xyz ACC, vf4, vf8x
    vmadday.xyz ACC, vf5, vf8y
    vmaddz.xyz vf11, vf6, vf8z
    vmulax.xyz ACC, vf4, vf9x
    vmadday.xyz ACC, vf5, vf9y
    vmaddz.xyz vf12, vf6, vf9z
    vmulax.xyz ACC, vf4, vf10x
    vmadday.xyz ACC, vf5, vf10y
    vmaddz.xyz vf13, vf6, vf10z
    sqc2 vf11, 0x0(a0)
    sqc2 vf12, 0x10(a0)
    sqc2 vf13, 0x20(a0)
    jr ra
    nop
}
/* The 3x3 part of m = m * a (3x3). */
asm void Mtx_Concat3x3(f32 *m, f32 *a)
{
    .set noreorder
    lqc2 vf4, 0x0(a1)
    lqc2 vf5, 0x10(a1)
    lqc2 vf6, 0x20(a1)
    lqc2 vf8, 0x0(a0)
    lqc2 vf9, 0x10(a0)
    lqc2 vf10, 0x20(a0)
    vmulax.xyz ACC, vf4, vf8x
    vmadday.xyz ACC, vf5, vf8y
    vmaddz.xyz vf8, vf6, vf8z
    vmulax.xyz ACC, vf4, vf9x
    vmadday.xyz ACC, vf5, vf9y
    vmaddz.xyz vf9, vf6, vf9z
    vmulax.xyz ACC, vf4, vf10x
    vmadday.xyz ACC, vf5, vf10y
    vmaddz.xyz vf10, vf6, vf10z
    sqc2 vf8, 0x0(a0)
    sqc2 vf9, 0x10(a0)
    sqc2 vf10, 0x20(a0)
    jr ra
    nop
}
/* Inverse of a rotation, scale and translation matrix (transpose / scale^2). */
asm void Mtx_Inverse(f32 *dst, f32 *src)
{
    .set noreorder
    la v1, gMtxIdentity
    lqc2 vf8, 0x0(a1)
    lqc2 vf9, 0x10(a1)
    lqc2 vf10, 0x20(a1)
    lqc2 vf11, 0x30(a1)
    vmul.xyz vf1, vf8, vf8
    vaddy.x vf1, vf1, vf1y
    vaddz.x vf1, vf1, vf1z
    vrsqrt Q, vf0w, vf1x
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    lqc2 vf7, 0x30(v1)
    vmul.xyz vf2, vf9, vf9
    vaddy.x vf2, vf2, vf2y
    vaddz.x vf2, vf2, vf2z
    vmul.xyz vf3, vf10, vf10
    vaddy.x vf3, vf3, vf3y
    vaddz.x vf3, vf3, vf3z
    vsub.xyz vf21, vf0, vf11
    vwaitq
    vaddq.x vf20, vf0, Q
    vrsqrt Q, vf0w, vf2x
    vmulx.x vf4, vf20, vf8x
    vmuly.x vf5, vf20, vf8y
    vmulz.x vf6, vf20, vf8z
    vwaitq
    vaddq.y vf20, vf0, Q
    vrsqrt Q, vf0w, vf3x
    vmulx.y vf4, vf20, vf9x
    vmuly.y vf5, vf20, vf9y
    vmulz.y vf6, vf20, vf9z
    vwaitq
    vaddq.z vf20, vf0, Q
    vmulx.z vf4, vf20, vf10x
    vmuly.z vf5, vf20, vf10y
    vmulz.z vf6, vf20, vf10z
    vmulax.xyz ACC, vf4, vf21x
    vmadday.xyz ACC, vf5, vf21y
    vmaddz.xyz vf7, vf6, vf21z
    vmul.xyz vf4, vf4, vf20
    vmul.xyz vf5, vf5, vf20
    vmul.xyz vf6, vf6, vf20
    vmul.xyz vf7, vf7, vf20
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    sqc2 vf6, 0x20(a0)
    jr ra
    sqc2 vf7, 0x30(a0)
}

/* dst = src (a 4x4 matrix, 16-byte aligned; hand-written, with lq/sq). */
asm void Mtx_Copy(f32 *dst, f32 *src)
{
    .set noreorder
    lq a2, 0x0(a1)
    lq a3, 0x10(a1)
    lq t0, 0x20(a1)
    lq t1, 0x30(a1)
    sq a2, 0x0(a0)
    sq a3, 0x10(a0)
    sq t0, 0x20(a0)
    jr ra
    sq t1, 0x30(a0)
}

void Mtx_CopyRot(f32 *m, f32 *src)
{
    m[0] = src[0];
    m[1] = src[1];
    m[2] = src[2];
    m[4] = src[4];
    m[5] = src[5];
    m[6] = src[6];
    m[8] = src[8];
    m[9] = src[9];
    m[10] = src[10];
}
/* a * wa + b * wb, the rotation made orthonormal again (rows 0 and 1, then their cross products). */
asm void Mtx_Blend(f32 *dst, f32 *a, f32 *b, f32 wa, f32 wb)
{
    .set noreorder
    vmr32.xyzw vf6, vf0
    vmove.xyzw vf7, vf0
    vmr32.xyzw vf5, vf6
    vmr32.xyzw vf4, vf5
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf8
    mfc1 t0, f13
    nop
    qmtc2.ni t0, vf9
    lqc2 vf10, 0x0(a1)
    lqc2 vf11, 0x0(a2)
    lqc2 vf12, 0x10(a1)
    lqc2 vf13, 0x10(a2)
    lqc2 vf14, 0x30(a1)
    lqc2 vf15, 0x30(a2)
    vmulax.xyz ACC, vf10, vf8x
    vmaddx.xyz vf10, vf11, vf9x
    vmulax.xyz ACC, vf12, vf8x
    vmaddx.xyz vf12, vf13, vf9x
    vmulax.xyz ACC, vf14, vf8x
    vmaddx.xyz vf7, vf15, vf9x
    vmul.xyz vf4, vf10, vf10
    vaddy.x vf4, vf4, vf4y
    vaddz.x vf4, vf4, vf4z
    vrsqrt Q, vf0w, vf4x
    vwaitq
    vmulq.xyz vf4, vf10, Q
    vmul.xyz vf5, vf12, vf12
    vaddy.x vf5, vf5, vf5y
    vaddz.x vf5, vf5, vf5z
    vrsqrt Q, vf0w, vf5x
    vwaitq
    vmulq.xyz vf5, vf12, Q
    vopmula.xyz ACC, vf4, vf5
    vopmsub.xyz vf6, vf5, vf4
    vmul.xyz vf8, vf6, vf6
    vaddy.x vf8, vf8, vf8y
    vaddz.x vf8, vf8, vf8z
    vrsqrt Q, vf0w, vf8x
    vwaitq
    vmulq.xyz vf6, vf6, Q
    vopmula.xyz ACC, vf6, vf4
    vopmsub.xyz vf5, vf4, vf6
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    sqc2 vf6, 0x20(a0)
    jr ra
    sqc2 vf7, 0x30(a0)
}
/* out = v * (the 3x3 part of m). */
asm void Vec_Rotate(f32 *out, f32 *v, f32 *m)
{
    .set noreorder
    lwc1 f2, 0x0(a1)
    addiu sp, sp, -0x10
    lwc1 f1, 0x4(a1)
    addiu v1, sp, 0x0
    lwc1 f0, 0x8(a1)
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(a2)
    lqc2 vf6, 0x10(a2)
    lqc2 vf7, 0x20(a2)
    vmulax.xyz ACC, vf5, vf4x
    vmadday.xyz ACC, vf6, vf4y
    vmaddz.xyz vf4, vf7, vf4z
    sqc2 vf4, 0x0(v1)
    addiu v1, sp, 0x0
    lwc1 f2, 0x0(v1)
    lwc1 f1, 0x4(v1)
    lwc1 f0, 0x8(v1)
    swc1 f2, 0x0(a0)
    swc1 f1, 0x4(a0)
    swc1 f0, 0x8(a0)
    jr ra
    addiu sp, sp, 0x10
}
/* v = v * (the 3x3 part of m). */
asm void Vec_RotateSelf(f32 *v, f32 *m)
{
    .set noreorder
    lwc1 f2, 0x0(a0)
    addiu sp, sp, -0x10
    lwc1 f1, 0x4(a0)
    addiu v1, sp, 0x0
    lwc1 f0, 0x8(a0)
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(a1)
    lqc2 vf6, 0x10(a1)
    lqc2 vf7, 0x20(a1)
    vmulax.xyz ACC, vf5, vf4x
    vmadday.xyz ACC, vf6, vf4y
    vmaddz.xyz vf4, vf7, vf4z
    sqc2 vf4, 0x0(v1)
    addiu v1, sp, 0x0
    lwc1 f2, 0x0(v1)
    lwc1 f1, 0x4(v1)
    lwc1 f0, 0x8(v1)
    swc1 f2, 0x0(a0)
    swc1 f1, 0x4(a0)
    swc1 f0, 0x8(a0)
    jr ra
    addiu sp, sp, 0x10
}
/* out = v * m (4 components). */
asm void Vec4_Transform(f32 *out, f32 *v, f32 *m)
{
    .set noreorder
    lwc1 f3, 0x0(a1)
    addiu sp, sp, -0x10
    lwc1 f2, 0x4(a1)
    addiu v1, sp, 0x0
    lwc1 f1, 0x8(a1)
    lwc1 f0, 0xC(a1)
    swc1 f3, 0x0(v1)
    swc1 f2, 0x4(v1)
    swc1 f1, 0x8(v1)
    swc1 f0, 0xC(v1)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(a2)
    lqc2 vf6, 0x10(a2)
    lqc2 vf7, 0x20(a2)
    lqc2 vf8, 0x30(a2)
    vmulax.xyzw ACC, vf5, vf4x
    vmadday.xyzw ACC, vf6, vf4y
    vmaddaz.xyzw ACC, vf7, vf4z
    vmaddw.xyzw vf4, vf8, vf4w
    sqc2 vf4, 0x0(v1)
    addiu v1, sp, 0x0
    lwc1 f3, 0x0(v1)
    lwc1 f2, 0x4(v1)
    lwc1 f1, 0x8(v1)
    lwc1 f0, 0xC(v1)
    swc1 f3, 0x0(a0)
    swc1 f2, 0x4(a0)
    swc1 f1, 0x8(a0)
    swc1 f0, 0xC(a0)
    jr ra
    addiu sp, sp, 0x10
}
/* v = v * m (4 components). */
asm void Vec4_TransformSelf(f32 *v, f32 *m)
{
    .set noreorder
    lwc1 f3, 0x0(a0)
    addiu sp, sp, -0x10
    lwc1 f2, 0x4(a0)
    addiu v1, sp, 0x0
    lwc1 f1, 0x8(a0)
    lwc1 f0, 0xC(a0)
    swc1 f3, 0x0(v1)
    swc1 f2, 0x4(v1)
    swc1 f1, 0x8(v1)
    swc1 f0, 0xC(v1)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(a1)
    lqc2 vf6, 0x10(a1)
    lqc2 vf7, 0x20(a1)
    lqc2 vf8, 0x30(a1)
    vmulax.xyzw ACC, vf5, vf4x
    vmadday.xyzw ACC, vf6, vf4y
    vmaddaz.xyzw ACC, vf7, vf4z
    vmaddw.xyzw vf4, vf8, vf4w
    sqc2 vf4, 0x0(v1)
    addiu v1, sp, 0x0
    lwc1 f3, 0x0(v1)
    lwc1 f2, 0x4(v1)
    lwc1 f1, 0x8(v1)
    lwc1 f0, 0xC(v1)
    swc1 f3, 0x0(a0)
    swc1 f2, 0x4(a0)
    swc1 f1, 0x8(a0)
    swc1 f0, 0xC(a0)
    jr ra
    addiu sp, sp, 0x10
}
/* out = (v, 1) * m divided by its w. */
asm void Vec_Project(f32 *out, f32 *v, f32 *m)
{
    .set noreorder
    lwc1 f0, 0x0(a1)
    addiu sp, sp, -0x10
    lui v1, (0x3F800000 >> 16)
    addiu a3, sp, 0x0
    swc1 f0, 0x0(sp)
    lwc1 f0, 0x4(a1)
    swc1 f0, 0x4(sp)
    lwc1 f0, 0x8(a1)
    swc1 f0, 0x8(sp)
    sw v1, 0xC(sp)
    lqc2 vf4, 0x0(a3)
    lqc2 vf5, 0x0(a2)
    lqc2 vf6, 0x10(a2)
    lqc2 vf7, 0x20(a2)
    lqc2 vf8, 0x30(a2)
    vmulax.xyzw ACC, vf5, vf4x
    vmadday.xyzw ACC, vf6, vf4y
    vmaddaz.xyzw ACC, vf7, vf4z
    vmaddw.xyzw vf4, vf8, vf4w
    vdiv Q, vf0w, vf4w
    vwaitq
    vmulq.xyz vf4, vf4, Q
    sqc2 vf4, 0x0(a3)
    addiu v1, sp, 0x0
    lwc1 f3, 0x0(v1)
    lwc1 f2, 0x4(v1)
    lwc1 f1, 0x8(v1)
    lwc1 f0, 0xC(v1)
    swc1 f3, 0x0(a0)
    swc1 f2, 0x4(a0)
    swc1 f1, 0x8(a0)
    swc1 f0, 0xC(a0)
    jr ra
    addiu sp, sp, 0x10
}

/* v rotated by a about the x axis. */
void Vec_RotateX(f32 *v, f32 a)
{
    f32 s;
    f32 c;
    Vec3Copy t;

    t = *(Vec3Copy *)v;
    Math_GetSinCos(&s, &c, a);
    v[1] = c * t.y - s * t.z;
    v[2] = s * t.y + c * t.z;
}

/* v rotated by a about the y axis. */
void Vec_RotateY(f32 *v, f32 a)
{
    f32 s;
    f32 c;
    Vec3Copy t;

    t = *(Vec3Copy *)v;
    Math_GetSinCos(&s, &c, a);
    v[0] = c * t.x + s * t.z;
    v[2] = -s * t.x + c * t.z;
}

/* v rotated by a about the z axis. */
void Vec_RotateZ(f32 *v, f32 a)
{
    f32 s;
    f32 c;
    Vec3Copy t;

    t = *(Vec3Copy *)v;
    Math_GetSinCos(&s, &c, a);
    v[0] = c * t.x - s * t.y;
    v[1] = s * t.x + c * t.y;
}
/* |v|. */
asm f32 Vec_Length(f32 *v)
{
    .set noreorder
    lwc1 f2, 0x0(a0)
    addiu sp, sp, -0x10
    lwc1 f1, 0x4(a0)
    addiu v0, sp, 0x0
    lwc1 f0, 0x8(a0)
    swc1 f2, 0x0(v0)
    swc1 f1, 0x4(v0)
    swc1 f0, 0x8(v0)
    lqc2 vf4, 0x0(v0)
    vmul.xyz vf4, vf4, vf4
    vaddy.x vf4, vf4, vf4y
    vaddz.x vf4, vf4, vf4z
    vsqrt Q, vf4x
    vwaitq
    vaddq.x vf4, vf0, Q
    qmfc2.ni v0, vf4
    mtc1 v0, f0
    jr ra
    addiu sp, sp, 0x10
}
/* |a - b|. */
asm f32 Vec_Distance(f32 *a, f32 *b)
{
    .set noreorder
    lwc1 f2, 0x0(a0)
    addiu sp, sp, -0x20
    lwc1 f1, 0x4(a0)
    addiu v1, sp, 0x10
    lwc1 f0, 0x8(a0)
    addiu v0, sp, 0x0
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lwc1 f2, 0x0(a1)
    lwc1 f1, 0x4(a1)
    lwc1 f0, 0x8(a1)
    swc1 f2, 0x0(v0)
    swc1 f1, 0x4(v0)
    swc1 f0, 0x8(v0)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(v0)
    vsub.xyz vf4, vf4, vf5
    vmul.xyz vf4, vf4, vf4
    vaddy.x vf4, vf4, vf4y
    vaddz.x vf4, vf4, vf4z
    vsqrt Q, vf4x
    vwaitq
    vaddq.x vf4, vf0, Q
    qmfc2.ni v1, vf4
    mtc1 v1, f0
    jr ra
    addiu sp, sp, 0x20
}
/* v = v / |v|. */
asm void Vec_Normalize(f32 *v)
{
    .set noreorder
    lwc1 f2, 0x0(a0)
    addiu sp, sp, -0x10
    lwc1 f1, 0x4(a0)
    addiu v1, sp, 0x0
    lwc1 f0, 0x8(a0)
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lqc2 vf4, 0x0(v1)
    vmul.xyz vf5, vf4, vf4
    vaddy.x vf5, vf5, vf5y
    vaddz.x vf5, vf5, vf5z
    vrsqrt Q, vf0w, vf5x
    vwaitq
    vmulq.xyz vf4, vf4, Q
    sqc2 vf4, 0x0(v1)
    addiu v1, sp, 0x0
    lwc1 f2, 0x0(v1)
    lwc1 f1, 0x4(v1)
    lwc1 f0, 0x8(v1)
    swc1 f2, 0x0(a0)
    swc1 f1, 0x4(a0)
    swc1 f0, 0x8(a0)
    jr ra
    addiu sp, sp, 0x10
}
/* a . b (3 components). */
asm f32 Vec_Dot(f32 *a, f32 *b)
{
    .set noreorder
    lwc1 f2, 0x0(a0)
    addiu sp, sp, -0x20
    lwc1 f1, 0x4(a0)
    addiu v1, sp, 0x10
    lwc1 f0, 0x8(a0)
    addiu v0, sp, 0x0
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lwc1 f2, 0x0(a1)
    lwc1 f1, 0x4(a1)
    lwc1 f0, 0x8(a1)
    swc1 f2, 0x0(v0)
    swc1 f1, 0x4(v0)
    swc1 f0, 0x8(v0)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(v0)
    vmul.xyz vf4, vf4, vf5
    vaddy.x vf4, vf4, vf4y
    vaddz.x vf4, vf4, vf4z
    qmfc2.ni v1, vf4
    mtc1 v1, f0
    jr ra
    addiu sp, sp, 0x20
}
/* out = a x b. */
asm void Vec_Cross(f32 *out, f32 *a, f32 *b)
{
    .set noreorder
    lwc1 f2, 0x0(a1)
    addiu sp, sp, -0x20
    lwc1 f1, 0x4(a1)
    addiu a3, sp, 0x10
    lwc1 f0, 0x8(a1)
    addiu v1, sp, 0x0
    swc1 f2, 0x0(a3)
    swc1 f1, 0x4(a3)
    swc1 f0, 0x8(a3)
    lwc1 f2, 0x0(a2)
    lwc1 f1, 0x4(a2)
    lwc1 f0, 0x8(a2)
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lqc2 vf4, 0x0(a3)
    lqc2 vf5, 0x0(v1)
    vopmula.xyz ACC, vf4, vf5
    vopmsub.xyz vf6, vf5, vf4
    sqc2 vf6, 0x0(a3)
    addiu v1, sp, 0x10
    lwc1 f2, 0x0(v1)
    lwc1 f1, 0x4(v1)
    lwc1 f0, 0x8(v1)
    swc1 f2, 0x0(a0)
    swc1 f1, 0x4(a0)
    swc1 f0, 0x8(a0)
    jr ra
    addiu sp, sp, 0x20
}

void Vec_Copy(f32 *dst, f32 *src)
{
    *(Vec3Copy *)dst = *(Vec3Copy *)src;
}

/* Radians to a 16-bit angle (0x10000 a turn, in [0, 0x10000)). */
u32 Math_RadToAngle(f32 rad)
{
    f32 deg = (180.0f * rad) / 3.1415927f;

    if (deg < 0.0f) {
        deg += 360.0f;
    }
    return (u32)(182.04445f * deg);
}

/* A 16-bit angle (0x10000 a turn) in radians, in (-pi, pi]. */
f32 Math_Angle16ToRad(u16 angle)
{
    f32 r = 3.1415927f * (0.0054931641f * angle) / 180.0f;

    if (r > 3.1415927f) {
        r += -6.2831855f;
    }
    return r;
}

/* A matrix into slot i of the table at 0x20D830 (main). */
void MtxSlot_Store(s32 i, Mtx44Copy *m)
{
    gMtxSlots[i] = *m;
}

/* Slot i of the matrix table at 0x20D830 (main) into m. */
void MtxSlot_Load(Mtx44Copy *m, s32 i)
{
    *m = gMtxSlots[i];
}

extern f32 gMtxView[];
extern f32 gMtxProjScreen[];
void Mtx_Concat(f32 *m, f32 *a);
void Vec_Project(f32 *out, f32 *v, f32 *m);

/* out = v projected by the matrix of slot 0 of gMtxSlots times gMtxView times gMtxProjScreen. */
void Camera_ProjectPoint(f32 *out, f32 *v)
{
    f32 m[16];

    Mtx_Mul(m, (f32 *)gMtxSlots, gMtxView);
    Mtx_Concat(m, gMtxProjScreen);
    Vec_Project(out, v, m);
}

f32 Math_Absf(f32 x)
{
    f32 ret;

    ret = x;
    if (ret < 0.0f) {
        ret = -ret;
    }
    return ret;
}

f32 Math_Acos(f32 x)
{
    return acosf(x);
}

/* asin x = atan(x / sqrt(1 - x * x)). */
f32 Math_Asin(f32 x)
{
    return Math_Atan(x / Math_Sqrtf(1.0f + (-x * x)));
}

f32 atanf(f32 x);

f32 Math_Atan(f32 x)
{
    return atanf(x);
}

/* atan2(y, x). */
f32 Math_Atan2(f32 y, f32 x)
{
    return atan2f(y, x);
}

f32 ceilf(f32 x);

f32 Math_Ceil(f32 x)
{
    return ceilf(x);
}

/* e to the x. */
f32 Math_Exp(f32 x)
{
    return expf(x);
}

f32 Math_SinVu0(f32 x);

f32 Math_Sin(f32 x)
{
    return Math_SinVu0(x);
}

f32 Math_CosVu0(f32 x);

f32 Math_Cos(f32 x)
{
    return Math_CosVu0(x);
}

/* *s = sin a, *c = cos a. */
void Math_GetSinCos(f32 *s, f32 *c, f32 a)
{
    f32 sc[4];

    Math_SinCos(sc, a);
    *s = sc[0];
    *c = sc[1];
}

f32 floorf(f32 x);

f32 Math_Floor(f32 x)
{
    return floorf(x);
}

/* x to the y. */
f32 Math_Pow(f32 x, f32 y)
{
    return powf(x, y);
}
/* sqrt x (VU0). */
asm f32 Math_Sqrtf(f32 x)
{
    .set noreorder
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf4
    vsqrt Q, vf4x
    vwaitq
    vaddq.x vf4, vf0, Q
    qmfc2.ni t0, vf4
    mtc1 t0, f0
    jr ra
    nop
}

/* The rotation matrix of quaternion q (x, y, z, w), scaled by 2 / |q|^2 (0 for a zero q). */
void Quat_ToMatrix(f32 *q, f32 (*m)[4])
{
    f32 n = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
    f32 s;
    f32 xx, yy, zz, xy, xz, yz, wx, wy, wz;

    s = (n > 0.0f) ? 2.0f / n : 0.0f;
    xx = s * (q[0] * q[0]);
    xy = s * (q[0] * q[1]);
    xz = s * (q[0] * q[2]);
    wx = s * (q[3] * q[0]);
    yy = s * (q[1] * q[1]);
    yz = s * (q[1] * q[2]);
    wy = s * (q[3] * q[1]);
    wz = s * (q[3] * q[2]);
    zz = s * (q[2] * q[2]);
    m[0][0] = 1.0f - (yy + zz);
    m[0][1] = xy + wz;
    m[0][2] = xz - wy;
    m[1][0] = xy - wz;
    m[1][1] = 1.0f - (xx + zz);
    m[1][2] = yz + wx;
    m[2][0] = xz + wy;
    m[2][1] = yz - wx;
    m[2][2] = 1.0f - (xx + yy);
    m[0][3] = m[1][3] = m[2][3] = 0.0f;
    m[3][0] = m[3][1] = m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

/* q (its axis in q[0..2]) = the rotation by angle about that axis, as a quaternion. */
void Quat_FromAxisAngle(f32 *q, f32 angle)
{
    f32 len;
    f32 s;

    angle *= 0.5f;
    len = Math_Sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2]);
    q[3] = 0.0f;
    q[0] /= len;
    q[1] /= len;
    q[2] /= len;
    s = Math_Sin(angle);
    q[3] = Math_Cos(angle);
    q[0] *= s;
    q[1] *= s;
    q[2] *= s;
}

void Quat_Set(f32 *axis, f32 *q, f32 angle)
{
    q[3] = 0.0f;
    q[0] = axis[0];
    q[1] = axis[1];
    q[2] = axis[2];
    Quat_FromAxisAngle(q, angle);
}

/*
 * Whether the sphere at p of radius r is in the view volume fr (four side planes through
 * the eye at fr[0], fr[4], fr[8], fr[12]; the depth range at fr[16], fr[17]); out = p in
 * view space (m).
 */
s32 Frustum_TestSphere(f32 *p, f32 *out, f32 *m, f32 *fr, f32 r)
{
    out[2] = m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14];
    if (out[2] - r > fr[16]) {
        return 0;
    }
    if (out[2] + r < fr[17]) {
        return 0;
    }
    out[0] = m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12];
    if (out[0] * fr[0] + out[2] * fr[2] > r) {
        return 0;
    }
    if (out[0] * fr[4] + out[2] * fr[6] > r) {
        return 0;
    }
    out[1] = m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13];
    if (out[1] * fr[9] + out[2] * fr[10] > r) {
        return 0;
    }
    if (out[1] * fr[13] + out[2] * fr[14] > r) {
        return 0;
    }
    return 1;
}

void Anim_SetData(s32 handle)
{
    gAnimData = (s32) (Mem_GetAddr((void *) handle));
    Anim_SetCurveKeys((u8 *) gAnimData);
}

/* A copy of a built skeleton (the block skel's header points at, its size
 * bytes; model.h's SkelHeader) in a new memory handle, which it returns
 * (Model_InitSkeleton). The header's block (+8) and size (+0xC) are read
 * as bytes: read through SkelHeader, the code differs. */
void *Skeleton_CopyData(u8 *skel)
{
    u8 *src = *(u8 **)(skel + 8);
    void *h = Mem_Alloc(*(s32 *)(src + 0xC), 4);
    u8 *dst = Mem_GetAddr(h);
    u32 i;
    u8 *s = src;

    for (i = 0; i < *(u32 *)(src + 0xC); i++) {
        *dst++ = *s++;
    }
    return h;
}

/* A copy of the block at +0x14 of obj (its size at +0x18, copied by words) in a new handle. */
void *Skeleton_CopyBlock14(u8 *obj)
{
    u8 *src = *(u8 **)(obj + 0x14);
    void *h = Mem_Alloc(*(s32 *)(src + 0x18), 4);
    u32 *dst = Mem_GetAddr(h);
    u32 i;
    u32 *s = (u32 *)src;

    for (i = 0; i < *(u32 *)(src + 0x18) / 4; i++) {
        *dst++ = *s++;
    }
    return h;
}

s32 Model_FreeBlock(void *p)
{
    Mem_Free(p);
    return 1;
}

s32 Mem_FreeRet1(void *p)
{
    Mem_Free(p);
    return 1;
}

/* unk_2E3940's .data (0x4021C0-0x402570) */
extern char D_0046A100[];
extern char D_0046A108[];
extern char D_0046A110[];
extern char D_0046A118[];
extern char D_0046A120[];
extern char D_0046A128[];
extern char D_0046A130[];
extern char D_0046A138[];
extern char D_0046A140[];
extern char D_0046A148[];
extern char D_0046A150[];
extern char D_0046A158[];
extern char D_0046A200[];
#pragma explicit_zero_data on
u32 D_004021C0[16] = {
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
};
#pragma explicit_zero_data reset
u8 gGsColorTable[32] = {
#include "sub_main/00402200.inc"
};
char D_00402220[0x10] = {
#include "sub_main/00402220.inc"
};
char D_00402230[0x10] = {
#include "sub_main/00402230.inc"
};
char *gMonthNames[12] = {
    D_0046A100, D_0046A108, D_0046A110, D_0046A118,
    D_0046A120, D_0046A128, D_0046A130, D_0046A138,
    D_0046A140, D_0046A148, D_0046A150, D_0046A158,
};
Delims gVersionDelims = {
#include "sub_main/00402270.inc"
};
u32 gFontDescs_0[16] = {
#include "sub_main/00402280.inc"
};
FontDesc gFontDescs[4] = {
#include "sub_main/004022C0.inc"
};
u32 gFontColors[16] = {
#include "sub_main/004023B0.inc"
};
Image gFontImage = {
#include "sub_main/004023F0.inc"
};
Image gFontClutImage = {
#include "sub_main/00402440.inc"
};
u8 *gSjisAsciiTable = (u8 *)D_0046A200;
char D_00402490[0x20] = {
#include "sub_main/00402490.inc"
};
s32 gModelPktSteps[8] = {
#include "sub_main/004024B0.inc"
};
s32 gVu1ProgLayouts[12][2] = {
#include "sub_main/004024D0.inc"
};
GifInfo gVu1GifInfos[2] = {
#include "sub_main/00402530.inc"
};
f32 gSinVu0Coefs[4] = {
#include "sub_main/00402550.inc"
};
f32 gSinVu0Poly[4] = {
#include "sub_main/00402560.inc"
};
