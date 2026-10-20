/*
 * The start of sub_main: its entry and main loop
 * (sub_main_entry, SubMain_Step), the system settings, the memory layout
 * (ram_areas.h) and the version string; the pads' frame state (pad.h); the
 * motion loaders and the player's motion layers (motion.h); the text runs
 * and the glyph cache (TextRun, FontCache, gFontCache); the draw models
 * (ModelEntry and MeshDesc, model.h: Model_Create, Model_Draw and the VIF
 * packet builders); then the animation curves, matrices, vectors,
 * quaternions and maths.
 *
 * The bones are another unit: Player_UpdateMotion declares
 * Bone_PosePart and Bone_BlendPart with the float first and u16 ids, which
 * their definitions don't match. Everything from Font_ClearSlots to
 * Font_CreateHandles has to be one unit: Font_CreateHandles uses v1 for its
 * temporaries because the static Font_ClearSlots leaves v0 alone.
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
#include "objhead.h"
#include "overlay.h"
#include "render_state.h"
#include "task.h"
#include "hw_regs.h"
#include "text.h"

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

/* The glyph cache's sizes. */
#define FONT_PAGES 7                /* the text pages, drawn in order (a run's page comes from its size: Font_SelectPage) */
#define FONT_PAGE_RUNS 0x80         /* text runs per page (0xA00 bytes of TextRun each) */
#define FONT_TEXT_SIZE 0x8000       /* bytes of the runs' strings, all pages together */
#define FONT_GLYPH_CODES 0x1E80     /* glyph indices (Font_GlyphIndex): FontCache.slot's length */
#define FONT_SLOT_NONE 0xFFFF       /* FontCache.slot: the glyph is not cached */
#define FONT_PAL_SIZE 0x40          /* bytes per palette in FontCache.palPixels */
#define FONT_FILE_ID 0x1712         /* the compressed file of the glyph bits (LoadCompressedFile) */


/* The glyph cache of the text system (gFontCache), at the start of
 * gFontArena. The fields the comments call "copied" come from the font's
 * FontDesc at Font_Init and nothing reads them. */
typedef struct FontCache {
    s32 font;                   /* 0x0000: the index in gFontDescs */
    s32 unk04;                  /* 0x0004: copied (FontDesc.unk00) */
    s32 width;                  /* 0x0008: glyph pixels */
    s32 height;                 /* 0x000C */
    s32 unk10;                  /* 0x0010: copied (FontDesc.unk0C) */
    s32 unk14;                  /* 0x0014: copied (FontDesc.unk10) */
    s32 unk18;                  /* 0x0018: copied (FontDesc.unk14) */
    u32 glyphs;                 /* 0x001C: glyphs of the font */
    s32 glyphBytes;             /* 0x0020: bytes per glyph at bits */
    s32 (*palInfo)[4];          /* 0x0024: per palette, 4 words for Font_SetPalette */
    s32 palCount;               /* 0x0028 */
    s32 texCount;               /* 0x002C: pages */
    s32 unk30;                  /* 0x0030: copied (FontDesc.unk30) */
    s32 unk34;                  /* 0x0034: copied (FontDesc.unk34) */
    s32 unk38;                  /* 0x0038: copied (FontDesc.unk38) */
    s32 arenaUsed;              /* 0x003C: bytes of gFontArena in use, to the end of text; nothing reads it */
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
    u8 lineGapOn;               /* 0x005E: Text_PrintCodes adds lineGap to each new line (Text_PrintfStyled) */
    u8 lineGap;                 /* 0x005F: pixels */
    u8 *palPixels;              /* 0x0060: the palettes, FONT_PAL_SIZE bytes each */
    s32 runCount[FONT_PAGES];   /* 0x0064: per page, its text runs */
    s32 runPrev[FONT_PAGES];    /* 0x0080: runCount of the frame before */
    TextRun *runs[FONT_PAGES];  /* 0x009C: per page, FONT_PAGE_RUNS runs */
    u32 textLen;                /* 0x00B8: bytes used in text (at most 0x8000) */
    char *text;                 /* 0x00BC: the runs' strings */
    s32 halfWidth;              /* 0x00C0: one-byte characters advance half a glyph (else 2/3) */
    s32 runPage;                /* 0x00C4: the page of new runs (Font_SelectPage) */
    s32 page;                   /* 0x00C8: the page being filled */
    s32 pageDone[FONT_PAGES];   /* 0x00CC: per page, drawn this frame */
    s32 max;                    /* 0x00E8: slots */
    s32 perPage;                /* 0x00EC: slots per page (texture) */
    f32 scale;                  /* 0x00F0: the display width / 640 */
    s32 clutNew;                /* 0x00F4: of new runs */
    s32 palNew;                 /* 0x00F8: of new runs */
    s32 glyphMode;              /* 0x00FC: new runs take glyph indices, not a string */
    s32 unk100;                 /* 0x0100: set to -1 by Font_BeginPage; nothing reads it */
    s16 pageFirst[FONT_PAGES];  /* 0x0104: per page, its first slot */
    s16 pageEnd[FONT_PAGES];    /* 0x0112: per page, the slot after its last */
    u16 *pageGlyphs[FONT_PAGES]; /* 0x0120: per page, the glyph of each slot */
    u16 slot[FONT_GLYPH_CODES]; /* 0x013C: per glyph (Font_GlyphIndex), its slot or FONT_SLOT_NONE */
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
s32 MotionPack_GroupCount(u8 *base, s32 i);
void func_6B6700(void); /* select overlay: Option_SetDefaults (select.txt) */
void Pad_UpdatePorts(void);
void SubMain_EmptyFrameHook(void);
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
void Kbd_ClearPolled(void);
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
extern s16 gQuestReturnArea;
extern s8 gPoogieErrand;
extern u8 gSoundLoading;
extern s32 gSaveBuf;
extern u16 gPadDebugModeLast;
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

/* This file's read-only data. */
#pragma rodata on
char gMonthNames_0[0x8] = {
#include "sub_main/0046A100.inc"
};
char gMonthNames_1[0x8] = {
#include "sub_main/0046A108.inc"
};
char gMonthNames_2[0x8] = {
#include "sub_main/0046A110.inc"
};
char gMonthNames_3[0x8] = {
#include "sub_main/0046A118.inc"
};
char gMonthNames_4[0x8] = {
#include "sub_main/0046A120.inc"
};
char gMonthNames_5[0x8] = {
#include "sub_main/0046A128.inc"
};
char gMonthNames_6[0x8] = {
#include "sub_main/0046A130.inc"
};
char gMonthNames_7[0x8] = {
#include "sub_main/0046A138.inc"
};
char gMonthNames_8[0x8] = {
#include "sub_main/0046A140.inc"
};
char gMonthNames_9[0x8] = {
#include "sub_main/0046A148.inc"
};
char gMonthNames_10[0x8] = {
#include "sub_main/0046A150.inc"
};
char gMonthNames_11[0x8] = {
#include "sub_main/0046A158.inc"
};
char gVerFmtMonth[0x8] = {
#include "sub_main/0046A160.inc"
};
char gVerFmtDay1[0x8] = {
#include "sub_main/0046A168.inc"
};
char gVerFmtField[0x8] = {
#include "sub_main/0046A170.inc"
};
char gVerFmtVersion[0xC] = {
#include "sub_main/0046A178.inc"
};
char gVerRule[0x20] = {
#include "sub_main/0046A190.inc"
};
char gVerTitle[0x18] = {
#include "sub_main/0046A1B0.inc"
};
char gVerFmtChar[0x8] = {
#include "sub_main/0046A1C8.inc"
};
char gVerLineEnd[0x10] = {
#include "sub_main/0046A1D0.inc"
};
char gMotionIdBadFmt[0x20] = {
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
/* The resident overlay's main loop, which never returns: one SubMain_Step per frame
 * (only frame flips while gSysExiting is set); online, every 32 frames, polls the
 * network interface for an error. */
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

/* One frame of sub_main: the first sets everything up and starts select's Boot_Task
 * (through its address: select is not linked with sub_main) in task slot 0; then each
 * runs the tasks and the frame callbacks, or, when a reset is asked for, returns to the
 * title screen. Returns 0. */
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
            SubMain_EmptyFrameHook();
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

/* The set-up of the first frame: the network, the USB keyboard, the memory layout, the
 * tasks, gm_sub and select, the text and sound systems, the mail and quest buffers. */
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
    gQuestReturnArea = 0;
    gElderForecastHeard = 0;
    gPoogieErrand = 0;
    Version_Build();
    LoadOverlay(OVERLAY_SELECT);
    SubMain_ResetSystems();
    Kbd_InitState((s32) gSysSettings.language);
    Loading_Stop();
}

/* The start of each frame: reads the pads and the USB keyboard and clears the frame's
 * text, free lists and callbacks before the tasks run. */
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
    gPadDebugModeLast = gPadDebugMode;
    Pad_UpdatePorts();
    Kbd_ClearPolled();
    Kbd_ReadUsb();
    Text_Reset();
    FreeList_InitAll();
    Callbacks_Clear();
    Draw_BeginFrame();
}

extern s32 gLoadingScreenOn;
/* The end of each frame's drawing: the loading screen, the brightness screen (after the
 * first frame), then the stream server. */
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
    sys->bootMode = BOOT_MODE_SCREENS;
    sys->taskRequest = TASK_REQUEST_NONE;
    Display_SetOffset(gSysSave.options.screenX, gSysSave.options.screenY);
    sys->unk42 = 0;
}

/* Resets the game's systems for a new start: the save copy, the patches, the chat
 * phrases, the system settings and the rest (Game_ResetSubsystems). */
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

/* The background colour (RS_BG_COLOR) = colour i of the RGB table gGsColorTable, alpha 0xFF. */
void Gs_SetTableColor(s32 i)
{
    u8 *rgb = &gGsColorTable[i * 3];
    u8 red;
    u8 green;
    u32 color;

    red = *rgb++;
    green = *rgb++;
    color = (red << 16) | 0xFF000000;
    color |= green << 8;
    Gs_SetState(RS_BG_COLOR, *rgb | color);
}

/* main's pointer to the pads as read. */
extern PadInput *gPadInputPtr;

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

/* Reads both controller ports, then updates their PadStates. */
void Pad_UpdatePorts(void)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        Pad_UpdatePort(i);
    }
    PadState_UpdateAll();
}

/* A stick's tilt with the dead zone applied: below 0x2D it counts as 0. */
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
extern s32 gPadDebugOn;
/* Updates the four slots (PadState_Update); then, with gPadDebugSwitch set, a debug switch on
 * the second pad: START toggles it, and while it is on, gPadDebugMode is 0 on CIRCLE, cycles
 * 0-3 while CROSS is held, and is 0x8000 otherwise. */
void PadState_UpdateAll(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        PadState_Update(i);
    }
    if (gPadDebugSwitch == 0) {
        return;
    }
    if (gPadState[1].pressed & PAD_START) {
        gPadDebugOn ^= 1;
    }
    if (gPadDebugOn != 0) {
        if (gPadState[1].pressed & PAD_CIRCLE) {
            gPadDebugMode = 0;
        } else if (gPadState[1].held & PAD_CROSS) {
            gPadDebugMode = (u8)(gPadDebugMode + 1) % 4;
        } else {
            gPadDebugMode = 0x8000;
        }
    } else {
        gPadDebugOn = 0;
        gPadDebugMode = 0;
    }
}

/* Empty: SubMain_Step calls it after each frame's tasks and callbacks. */
void SubMain_EmptyFrameHook(void)
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
extern s32 gPadDebugOn;

/* Clears the system settings (keeping gmSubAddr), lays out the memory areas
 * (ram_areas.h) and clears the texture handles. */
void Mem_InitLayout(void)
{
    s32 keep = gSysSettings.gmSubAddr;
    s32 i;

    memset(&gSysSettings, 0, sizeof(SysSettings));
    gSysSettings.gmSubAddr = keep;
    gSysSettings.subMainLoaded = 1;
    gSysSettings.memBase = (u8 *)MEM_AREAS_ADDR;
    Mem_Fill(gSysSettings.memBase, 0, MEM_AREAS_SIZE);
    gSysSettings.selWork = (u8 *)MEM_AREAS_ADDR;
    gSysSettings.unk64 = 0xAE8000;
    gSysSettings.unk68 = 0xAF5200;
    gSysSettings.bones = (void *)BONE_AREA_ADDR;
    gSysSettings.models = (struct Model *)MODEL_AREA_ADDR;
    gSysSettings.prims = PRIM_AREA_ADDR;
    gSysSettings.extraBuf = (u8 *)EXTRA_BUF_ADDR;
    gSysSettings.lobbyWorkBuf = LOBBY_WORK_ADDR;
    gSysSettings.workArea = (void *)WORK_AREA_ADDR;
    gSysSettings.stageFile = STAGE_FILE_ADDR;
    gSysSettings.camData = (u8 *)CAM_DATA_ADDR;
    gSysSettings.dataBuf = (u8 *)ANIM_AREA_ADDR;
    gSysSettings.loadBuf = (u8 *)LOAD_BUF_ADDR;
    gSysSettings.readBuf = (u8 *)READ_BUF_ADDR;
    gSysSettings.saveBuf = (struct SaveData *)SAVE_BUF_ADDR;
    gSysSettings.unkB4 = 0x27E00;
    gSysSettings.patchArea = (s32)PATCH_DATA;
    gSysSettings.unkDC = 0x20040;
    gSysSettings.unk80 = gSysSettings.dataBuf;
    gSysSettings.modelPack = gSysSettings.dataBuf;
    gSysSettings.unk7C = gSysSettings.modelPack;
    for (i = 0; i < TEX_HANDLE_COUNT; i++) {
        gTexHandles[i] = 0;
    }
    Tex_ClearIdSlots();
    gDrawFrameDepth = 0;
    gTexPrepLast = -1;
    gTexPrepLastCount = -1;
    gTexPrepFrameCount = 0;
    gPadDebugSwitch = 0;
    gPadDebugOn = 0;
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
extern char gBuildDate[];       /* the build date, as __DATE__ */
extern char gBuildTime[];       /* the build time, as __TIME__ */
extern char *gMonthNames[12];    /* the month names */
extern Delims gVersionDelims;       /* the separators */
extern char gVerFmtMonth[];
extern char gVerFmtDay1[];
extern char gVerFmtField[];
extern char gVerFmtVersion[];
extern char gVerRule[];
extern char gVerTitle[];
extern char gVerFmtChar[];
extern char gVerLineEnd[];
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
    char yearField[8];
    char monthField[8];
    char dayField[8];
    char hourField[8];
    char minField[8];
    char date[0x28];
    char time[0x20];
    s32 j;
    s32 i;

    delim = gVersionDelims;
    strcpy(date, gBuildDate);
    strcpy(time, gBuildTime);
    strcpy(month, strtok(date, delim.s));
    strcpy(day, strtok(NULL, delim.s));
    strcpy(year, strtok(NULL, delim.s));
    strcpy(yearField, year + 2);
    for (i = 0; i < 12; i++) {
        if (strcmp(gMonthNames[i], month) == 0) {
            sprintf(monthField, gVerFmtMonth, i + 1);
            break;
        }
    }
    if (strlen(day) == 1) {
        sprintf(dayField, gVerFmtDay1, day);
    } else {
        sprintf(dayField, gVerFmtField, day);
    }
    strcpy(hour, strtok(time, delim.s));
    strcpy(min, strtok(NULL, delim.s));
    sprintf(hourField, gVerFmtField, hour);
    sprintf(minField, gVerFmtField, min);
    sprintf((char *)gVersionString, gVerFmtVersion, yearField, monthField, dayField, hourField, minField);
    scePrintf(gVerRule);
    scePrintf(gVerTitle);
    for (j = 0; j < 10; j++) {
        scePrintf(gVerFmtChar, gVersionString[j]);
    }
    scePrintf(gVerLineEnd);
    scePrintf(gVerRule);
}

/* Loads the loading screen's texture file and opens the loading screen (in the save
 * buffer), then flips two frames. */
void SubMain_OpenLoading(void)
{
    gSoundLoading = 1;
    Tex_LoadFile(5, 0x155, 0);
    Loading_Open((u32) gSaveBuf, 1);
    Sys_FlipFrame(0);
    Sys_FlipFrame(0);
}

extern s32 gCommonMotionGroupFirst[];
extern void *gMotions[];
s32 SysHeap_GetMark(void *mark);
void SysHeap_Release(void *mark);
s32 Anim_LoadSet(AnimSet *out, u8 *buf, void *f);
void *AnimSet_CopyToHandle(u8 *obj);
u8 *MotionPack_GetEntry(u8 *t, s32 n);

/*
 * Loads the common motion pack (in gSysSettings.loadBuf; MOTIONS_COMMON_GROUPS groups)
 * into gMotions from 0, and the first index of each group into gCommonMotionGroupFirst.
 * Each animation file is converted in gSysSettings.dataBuf, under a system-heap mark
 * released after it, and copied into a memory handle of its own (NULL for no entry).
 */
void Anim_SetupCommon(void)
{
    s32 mark[2];
    AnimSet set;
    s32 groupBase;
    u8 *buf;
    s32 count;
    s32 group;
    s32 j;
    s32 motionIndex;
    u8 *entry;
    s32 *groupFirst;
    void **motions;

    motions = gMotions;
    groupFirst = gCommonMotionGroupFirst;
    buf = gSysSettings.loadBuf;
    motionIndex = 0;
    for (group = 0, groupBase = 0; group < MOTIONS_COMMON_GROUPS; group++, groupBase += MOTION_GROUP_SIZE) {
        groupFirst[group] = motionIndex;
        count = MotionPack_GroupCount(buf, group);
        for (j = 0; j < count; j++, motionIndex++) {
            entry = MotionPack_GetEntry(buf, j + groupBase);
            if (entry != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, entry);
                entry = AnimSet_CopyToHandle((u8 *)&set);
                SysHeap_Release(&mark);
                motions[motionIndex] = entry;
            } else {
                motions[motionIndex] = NULL;
            }
        }
    }
}

extern s32 gPlayerMotionGroupFirst[][8]; /* the first set index of each group, per set */

/*
 * As Anim_SetupCommon for the six groups of set slot: loads each animation set of the table
 * in gLoadBuf into gMotions[MOTIONS_PLAYER_FIRST + slot * MOTIONS_PER_PLAYER ...], the first index of each group into
 * gPlayerMotionGroupFirst[slot].
 */
void Anim_SetupPlayer(s32 slot)
{
    s32 groupBase;
    s32 group;
    s32 j;
    s32 motionIndex;
    s32 *groupFirst;
    s32 count;
    s32 mark[2];
    u8 *entry;
    AnimSet set;
    void **out;
    u8 *buf;

    out = &gMotions[slot * MOTIONS_PER_PLAYER + MOTIONS_PLAYER_FIRST];
    buf = gSysSettings.loadBuf;
    motionIndex = 0;
    group = 0;
    groupFirst = gPlayerMotionGroupFirst[slot];
    for (groupBase = 0; group < MOTIONS_PLAYER_GROUPS; group++, groupFirst++, groupBase += MOTION_GROUP_SIZE) {
        *groupFirst = motionIndex;
        count = MotionPack_GroupCount(buf, group);
        for (j = 0; j < count; j++, out++, motionIndex++) {
            entry = MotionPack_GetEntry(buf, j + groupBase);
            if (entry != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, entry);
                entry = AnimSet_CopyToHandle((u8 *)&set);
                SysHeap_Release(&mark);
                *out = entry;
            } else {
                *out = NULL;
            }
        }
    }
}

extern s32 gEmMotionGroupFirst[][8]; /* as gPlayerMotionGroupFirst, for these sets */
u8 EmKind_MotionGroupCount(s16 kind);

/*
 * As Anim_SetupPlayer for the 2 * EmKind_MotionGroupCount(kind) groups of set slot, into
 * gMotions[MOTIONS_EM_FIRST + slot * MOTIONS_PER_EM ...] and gEmMotionGroupFirst[slot]. The copy has its own
 * variable, and groupBase is set after the store, for the match (MWCC strength-reduces it).
 */
void Anim_SetupEm(s32 slot, s32 kind)
{
    s32 mark[2];
    AnimSet set;
    u8 *buf;
    s32 motionIndex;
    s32 count;
    u8 *entry;
    void **out;
    void *copy;
    s32 groupBase;
    s32 group;
    s32 j;
    s32 groups;

    out = &gMotions[slot * MOTIONS_PER_EM + MOTIONS_EM_FIRST];
    buf = gSysSettings.loadBuf;
    groups = EmKind_MotionGroupCount(kind) * 2;
    motionIndex = 0;
    for (group = 0; group < groups; group++) {
        gEmMotionGroupFirst[slot][group] = motionIndex;
        groupBase = group * MOTION_GROUP_SIZE;
        count = MotionPack_GroupCount(buf, group);
        for (j = 0; j < count; j++, out++, motionIndex++) {
            entry = MotionPack_GetEntry(buf, j + groupBase);
            if (entry != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, entry);
                copy = AnimSet_CopyToHandle((u8 *)&set);
                SysHeap_Release(&mark);
                *out = copy;
            } else {
                *out = NULL;
            }
        }
    }
}

/* As Anim_SetupPlayer for the first two groups of the sets Anim_SetupEm loads (into
 * gMotions[MOTIONS_EM_FIRST + slot * MOTIONS_PER_EM ...] and gEmMotionGroupFirst[slot]). */
void Anim_SetupEmSlot(s32 slot)
{
    s32 groupBase;
    s32 group;
    s32 j;
    s32 motionIndex;
    s32 *groupFirst;
    s32 count;
    s32 mark[2];
    u8 *entry;
    AnimSet set;
    void **out;
    u8 *buf;

    out = &gMotions[slot * MOTIONS_PER_EM + MOTIONS_EM_FIRST];
    buf = gSysSettings.loadBuf;
    motionIndex = 0;
    group = 0;
    groupFirst = gEmMotionGroupFirst[slot];
    for (groupBase = 0; group < 2; group++, groupFirst++, groupBase += MOTION_GROUP_SIZE) {
        *groupFirst = motionIndex;
        count = MotionPack_GroupCount(buf, group);
        for (j = 0; j < count; j++, out++, motionIndex++) {
            entry = MotionPack_GetEntry(buf, j + groupBase);
            if (entry != NULL) {
                SysHeap_GetMark(&mark);
                Anim_LoadSet(&set, gSysSettings.dataBuf, entry);
                entry = AnimSet_CopyToHandle((u8 *)&set);
                SysHeap_Release(&mark);
                *out = entry;
            } else {
                *out = NULL;
            }
        }
    }
}

/*
 * Entry index of a motion pack (a loaded file of animation files): the entries come in
 * groups of MOTION_GROUP_SIZE (group index / 100: its count at +0, the offset of its
 * offsets at +4), each an offset from the pack (-1: none); NULL when there is none.
 * The copy `start` of the pack, made first, gives the result (for the match: v0);
 * pack itself walks to the offsets.
 */
u8 *MotionPack_GetEntry(u8 *pack, s32 index)
{
    u8 *start = pack;
    s32 *group;
    s32 inGroup;
    s32 offset;

    group = (s32 *)(pack + (index / MOTION_GROUP_SIZE) * 8);
    inGroup = index % MOTION_GROUP_SIZE;
    if (inGroup >= group[0]) {
        return NULL;
    }
    pack = (u8 *)((s32 *)pack + group[1] / 4);
    offset = ((s32 *)pack)[inGroup];
    if (offset == -1) {
        return NULL;
    }
    start += offset;
    return start;
}

/* The number of entries of a group of a motion pack. */
s32 MotionPack_GroupCount(u8 *pack, s32 group)
{
    return *(s32 *)(pack + group * 8);
}

/* The root motion of bone's animation from fromFrame to toFrame (Bone_GetAnimMove);
 * a negative fromFrame counts as 0, keeping the length. */
void Bone_GetAnimMoveFrom(f32 fromFrame, f32 toFrame, f32 *out, EmBone *bone)
{
    if (fromFrame < 0.0f) {
        Bone_GetAnimMove(out, bone, 0.0f, toFrame - fromFrame);
        return;
    }
    Bone_GetAnimMove(out, bone, fromFrame, toFrame);
}

/* dst = first * weightFirst + second * weightSecond (3-vectors). */
void Vec_Blend(f32 *dst, f32 *first, f32 *second, f32 weightFirst, f32 weightSecond)
{
    dst[0] = first[0] * weightFirst + second[0] * weightSecond;
    dst[1] = first[1] * weightFirst + second[1] * weightSecond;
    dst[2] = first[2] * weightFirst + second[2] * weightSecond;
}

f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
f32 *Mtx_RotateVec(f32 *m, f32 *in, f32 *out);
u32 Player_TestFlag(void *obj, u32 flag);
void Player_ClearFlag(void *obj, u32 flag);

/* Moves the player by move, turned and scaled by its rotation and scale (not while
 * PLF_NO_ROOT_MOVE is set); with flag 0x4000000 set, it only clears that flag (a move
 * skipped once). */
void Player_MoveLocal(Player *pl, f32 *move)
{
    f32 delta[4];
    f32 mtx[16];
    f32 scaleMtx[16];

    if (Player_TestFlag(pl, 0x4000000)) {
        Player_ClearFlag(pl, 0x4000000);
        return;
    }
    Mtx_FromAnglesXYZ(pl->rot, mtx);
    Mtx_Scale(scaleMtx, pl->scale[0], pl->scale[1], pl->scale[2]);
    Mtx_Mul(mtx, mtx, scaleMtx);
    Mtx_RotateVec(mtx, move, delta);
    if (!Player_TestFlag(pl, PLF_NO_ROOT_MOVE)) {
        pl->pos[0] += delta[0];
        pl->pos[1] += delta[1];
        pl->pos[2] += delta[2];
    }
}

/* unit's own declaration: declared as this file's callers use it; with the definition's s32 (EmBone *b, s32 h, s32 id), Player_UpdateMotion compiles differently */
s32 Bone_SetPartAnim(struct EmBone *b, s32 h, u16 id);
void Debug_Message(char *unused0, s32 unused1, s32 unused2, s32 unused3);
extern char gMotionIdBadFmt[];
f32 Motion_GetLength(s32 h);
s32 Anim_GetLoopTime(s32 h, f32 *out);

/* Starts motion motionId[part] on the player's layer `part` at frame `frame`, directly
 * (blendLen == 0) or blended over |blendLen| + 1 frames (blendLen < 0: without the root
 * motion). The id is read as in Player_SetPartAnim. */
void Player_BeginLayerMotion(Player *pl, s32 frame, s32 blendLen, s32 part)
{
    u16 id;
    Model *plModel;
    void **slot;
    s32 inSet;

    id = pl->motionId[part];
    plModel = pl->model;
    inSet = id % MOTION_ID_SET;
    if (pl->isEm == 0 || (pl->ownTask != 0 && pl->playerMotions != 0)) {
        if (id >= MOTION_ID_SET) {
            slot = &gMotions[pl->index * MOTIONS_PER_PLAYER + MOTIONS_PLAYER_FIRST];
            slot += gPlayerMotionGroupFirst[pl->index][inSet / MOTION_GROUP_SIZE];
        } else {
            slot = gMotions;
            slot += gCommonMotionGroupFirst[inSet / MOTION_GROUP_SIZE];
        }
    } else {
        if (id >= MOTION_ID_SET) {
            slot = &gMotions[pl->emModelSlot * MOTIONS_PER_EM + MOTIONS_EM_FIRST];
            slot += gEmMotionGroupFirst[pl->emModelSlot][inSet / MOTION_GROUP_SIZE];
        } else {
            Debug_Message(gMotionIdBadFmt, (s16)id, 0, 0);
        }
    }
    if (pl->isEm == 0 || pl->ownTask != 0) {
        slot += inSet % MOTION_GROUP_SIZE;
    } else {
        slot = (void **)((s32 *)slot + inSet % MOTION_GROUP_SIZE);
    }
    if (blendLen != 0) {
        if (blendLen < 0) {
            pl->motion[part].blend = -1;
            blendLen = -blendLen;
        } else {
            pl->motion[part].blend = 1;
        }
        Bone_SetPartAnim(plModel->partBones2[0], *(s32 *)slot, (u16)part);
        pl->motion[part].blendFrameCount = Motion_GetLength(*(s32 *)slot);
        pl->motion[part].blendLoop = Anim_GetLoopTime(*(s32 *)slot, &pl->motion[part].blendLoopFrame);
        pl->motion[part].blendFrame = frame;
        pl->motion[part].blendFrames = blendLen + 1;
        pl->motion[part].blendT = pl->motion[part].blendStep = 1.0f / (blendLen + 1);
        pl->motion[part].blendMotion = (s32 *)slot;
    } else {
        Bone_SetPartAnim(plModel->partBones[0], *(s32 *)slot, (u16)part);
        pl->motion[part].frameCount = Motion_GetLength(*(s32 *)slot);
        pl->motion[part].loop = Anim_GetLoopTime(*(s32 *)slot, &pl->motion[part].loopFrame);
        pl->motion[part].frame = frame;
        pl->motion[part].blendFrames = 0;
        pl->motion[part].blend = 0;
        pl->motion[part].blendMotion = (s32 *)slot;
    }
    pl->motion[part].playing = 1;
    if (part == 0) {
        pl->motion[0].move[0] = 0.0f;
        pl->motion[0].move[1] = 0.0f;
        pl->motion[0].move[2] = 0.0f;
    }
}

/* Sets the animation of part `part` of the player's model from its id in partAnimMotion
 * (MOTION_ID_SET: the set: 0 the common set (gCommonMotionGroupFirst), else the player's
 * own (gPlayerMotionGroupFirst) or, for an object that isn't a player, the monster set of
 * its unk30F (gEmMotionGroupFirst); then MOTION_GROUP_SIZE: the group; the rest: the
 * entry). */
void Player_SetPartAnim(Player *pl, s32 part)
{
    u16 id;
    Model *plModel;
    void **slot;
    s32 inSet;

    id = pl->partAnimMotion[part];
    plModel = pl->model;
    inSet = id % MOTION_ID_SET;
    if (pl->isEm == 0 || (pl->ownTask != 0 && pl->playerMotions != 0)) {
        if (id >= MOTION_ID_SET) {
            slot = &gMotions[pl->index * MOTIONS_PER_PLAYER + MOTIONS_PLAYER_FIRST];
            slot += gPlayerMotionGroupFirst[pl->index][inSet / MOTION_GROUP_SIZE];
        } else {
            slot = gMotions;
            slot += gCommonMotionGroupFirst[inSet / MOTION_GROUP_SIZE];
        }
    } else {
        if (id >= MOTION_ID_SET) {
            slot = &gMotions[pl->emModelSlot * MOTIONS_PER_EM + MOTIONS_EM_FIRST];
            slot += gEmMotionGroupFirst[pl->emModelSlot][inSet / MOTION_GROUP_SIZE];
        } else {
            Debug_Message(gMotionIdBadFmt, (s16)id, 0, 0);
        }
    }
    if (pl->isEm == 0 || pl->ownTask != 0) {
        slot += inSet % MOTION_GROUP_SIZE;
    } else {
        slot = (void **)((s32 *)slot + inSet % MOTION_GROUP_SIZE);
    }
    Bone_SetPartAnim(plModel->partBones2[0], *(s32 *)slot, (u16)part);
}

/* The definitions take the float first too (the same registers either way). */
s32 Bone_PosePart(f32 t, struct EmBone *b, u16 id); /* unit's own declaration: the definition takes s32 id; Player_UpdateMotion is 22 words off with it */
s32 Bone_BlendPart(f32 wa, f32 wb, struct EmBone *a, struct EmBone *b, u16 id); /* unit's own declaration: as Bone_PosePart */
f32 Math_Hermite(f32 t, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);

/* Advances the player's motion layers: poses each active layer, blends a layer into its
 * next motion (blend != 0) with the root motion of layer 0, moves the player by layer 0's
 * root motion (Player_MoveLocal), loops or stops a layer at its end, and blends the part
 * overrides (partAnimOn, weight partAnimWeight %). Returns 1 when a layer ended. */
s32 Player_UpdateMotion(Player *pl)
{
    Model *plModel = pl->model;
    EmBone *bones = plModel->bones;
    u16 i;
    s32 done = 0;
    f32 weightFrom;
    f32 weightTo;
    f32 loopStart;
    f32 rootMove[4];
    f32 moveFrom[4];
    f32 moveTo[4];

    for (i = 0; i < 4; i++) {
        if (i < pl->motionLayerCount) {
            if (pl->motion[i].blend != 0) {
                Bone_PosePart(pl->motion[i].frame, plModel->partBones[0], i);
                Bone_PosePart(pl->motion[i].blendFrame, plModel->partBones2[0], i);
                Bone_GetAnimMoveFrom(pl->motion[0].frame, pl->motion[0].frame + pl->motion[0].step, moveFrom, plModel->partBones[0]->child);
                weightTo = Math_Hermite(pl->motion[i].blendT, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
                weightFrom = 1.0f - weightTo;
                Bone_BlendPart(weightFrom, weightTo, plModel->partBones[0], plModel->partBones2[0], i);
                if (i == 0) {
                    Bone_GetAnimMoveFrom(pl->motion[0].blendFrame, pl->motion[0].blendFrame + pl->motion[0].step, moveTo, plModel->partBones2[0]->child);
                    Vec_Blend(rootMove, moveFrom, moveTo, weightFrom, weightTo);
                    if (pl->motion[i].blend > 0) {
                        Player_MoveLocal(pl, rootMove);
                    }
                    bones[1].local[12] = 0.0f;
                    bones[1].local[13] = 0.0f;
                    bones[1].local[14] = 0.0f;
                }
                pl->motion[i].blendT += pl->motion[i].blendStep;
                if (--pl->motion[i].blendFrames <= 0.0f) {
                    pl->motion[i].blend = 0;
                    Bone_SetPartAnim(plModel->partBones[0], *pl->motion[i].blendMotion, i);
                    pl->motion[i].frameCount = pl->motion[i].blendFrameCount;
                    pl->motion[i].loop = pl->motion[i].blendLoop;
                    pl->motion[i].loopFrame = pl->motion[i].blendLoopFrame;
                    pl->motion[i].prevFrame = pl->motion[i].blendFrame;
                    pl->motion[i].frame = pl->motion[i].blendFrame + pl->motion[i].step;
                    pl->motion[i].blendFrames = 0;
                }
            } else {
                Bone_PosePart(pl->motion[i].frame, plModel->partBones[0], i);
                if (pl->partAnimOn[i] != 0) {
                    Player_SetPartAnim(pl, i);
                    Bone_PosePart(pl->motion[i].frame, plModel->partBones2[0], i);
                    weightTo = (f32)(u32)pl->partAnimWeight[i] / 100.0f;
                    Bone_BlendPart(1.0f - weightTo, weightTo, plModel->partBones[0], plModel->partBones2[0], i);
                }
                if (i == 0) {
                    Bone_GetAnimMoveFrom(pl->motion[i].frame - pl->motion[i].step, pl->motion[i].frame, rootMove, plModel->partBones[0]->child);
                    Player_MoveLocal(pl, rootMove);
                    rootMove[0] = bones[1].local[12] - pl->motion[0].move[0];
                    rootMove[1] = bones[1].local[13] - pl->motion[0].move[1];
                    rootMove[2] = bones[1].local[14] - pl->motion[0].move[2];
                    pl->motion[0].move[0] = bones[1].local[12];
                    pl->motion[0].move[1] = bones[1].local[13];
                    pl->motion[0].move[2] = bones[1].local[14];
                    bones[1].local[12] = 0.0f;
                    bones[1].local[13] = 0.0f;
                    bones[1].local[14] = 0.0f;
                }
                pl->motion[i].prevFrame = pl->motion[i].frame;
                pl->motion[i].frame += pl->motion[i].step;
                if (!(pl->motion[i].frame <= pl->motion[i].frameCount)) {
                    if (pl->motion[i].loop != 0) {
                        loopStart = pl->motion[i].loopFrame;
                        if (!(loopStart < 1.0f)) {
                            loopStart -= 1.0f;
                        }
                        Bone_PosePart(loopStart, plModel->partBones[0], i);
                        if (i == 0) {
                            pl->motion[i].move[0] = bones[1].local[12];
                            pl->motion[i].move[1] = bones[1].local[13];
                            pl->motion[i].move[2] = bones[1].local[14];
                        }
                        Bone_PosePart(pl->motion[i].frameCount, plModel->partBones[0], i);
                        pl->motion[i].frame = pl->motion[i].loopFrame;
                        if (i == 0) {
                            bones[1].local[12] = 0.0f;
                            bones[1].local[13] = 0.0f;
                            bones[1].local[14] = 0.0f;
                        }
                        pl->motion[i].playing = 0;
                        done = 1;
                    } else {
                        done = 1;
                        pl->motion[i].frame = pl->motion[i].frameCount;
                        pl->motion[i].playing = 0;
                    }
                    if (pl->partAnimOn[i] != 0) {
                        Bone_PosePart(pl->motion[i].frame, plModel->partBones[0], i);
                        Player_SetPartAnim(pl, i);
                        Bone_PosePart(pl->motion[i].frame, plModel->partBones2[0], i);
                        weightTo = (f32)(u32)pl->partAnimWeight[i] / 100.0f;
                        Bone_BlendPart(1.0f - weightTo, weightTo, plModel->partBones[0], plModel->partBones2[0], i);
                    }
                } else {
                    pl->motion[i].playing = 1;
                }
            }
        }
    }
    return done;
}


/* 1 when the motion of layer passes frame during this step (frame to frame + speed,
 * wrapping to the loop frame at the end of a looping motion); 0 while motions are held. */
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame)
{
    Player *pl = obj;
    ObjHead *head = (ObjHead *)pl;
    s32 wrapped = 0;
    f32 next;
    f32 cur;
    f32 end;

    if (head->motion[0].blend != 0) {
        return 0;
    }
    cur = head->motion[layer].frame;
    end = head->motion[layer].frameCount;
    next = cur + head->motion[layer].step;
    if (!(next <= end)) {
        if (head->motion[layer].loop) {
            wrapped = 1;
            next = head->motion[layer].loopFrame + (next - end);
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

/* Motion_PassedFrame for a monster; frame 2 is scaled by the monster's motion speed
 * when that isn't 1. */
s32 EmMotion_PassedFrameScaled(void *obj, f32 frame, s32 layer)
{
    Em *em = obj;
    f32 speed;

    speed = em->motionSpeed;
    if (speed != 1.0f && frame == 2.0f) {
        frame *= speed;
    }
    return Motion_PassedFrame(em, layer, frame);
}

/* 1 once the motion of layer has reached frame (0 while motions are held). */
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame)
{
    ObjHead *head = obj;

    if (head->motion[0].blend != 0) {
        return 0;
    }
    if (frame <= head->motion[layer].frame) {
        return 1;
    }
    return 0;
}

/* Motion_ReachedFrame for the monsters. */
s32 EmMotion_ReachedFrame(void *em, s32 layer, f32 frame)
{
    return Motion_ReachedFrame(em, layer, frame);
}

/* 1 while the motion of layer is at or past `from` but before `until`. */
s32 Motion_InFrameRange(void *obj, s32 layer, f32 from, f32 until)
{
    if (Motion_ReachedFrame(obj, layer, from) != 0 && Motion_ReachedFrame(obj, layer, until) == 0) {
        return 1;
    }
    return 0;
}

/* Motion_InFrameRange for the monsters. */
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 until)
{
    return Motion_InFrameRange(em, layer, from, until);
}

/* A font of gFontDescs (0x3C bytes). Font_Init copies it into the cache;
 * the unk fields are copied there and nothing reads them. */
typedef struct FontDesc {
    s32 unk00;
    s32 width;
    s32 height;
    s32 unk0C;
    s32 unk10;
    s32 unk14;
    s32 palInfo;                /* the address of the palettes' colours, 4 words each */
    s32 palCount;
    s32 texCount;               /* pages of the cache (textures of 256 x 256) */
    s32 glyphs;                 /* glyphs of the font */
    s32 glyphBytes;             /* bytes per glyph, 2 bits a pixel */
    s32 bitsSize;               /* bytes of the glyph bits */
    s32 unk30;
    s32 unk34;
    s32 unk38;
} FontDesc;

extern FontDesc gFontDescs[];
extern u8 gFontArena[];
s32 LoadCompressedFile(u8 *dst, s32 id);
void Font_ResetState(void);
void Font_CreateHandles(void);

/* Sets up the glyph cache (gFontCache) in gFontArena: the font descriptor's fields, the page
 * glyph lists, the texture and palette tables, the glyph bits and the text runs; loads the
 * glyph bits (FONT_FILE_ID) once. The bits' size is rounded up in two statements: in one,
 * every temporary takes another register. */
void Font_Init(s32 unused0)
{
    FontDesc *desc;
    s32 bits;
    s32 i;
    u8 *base;
    u8 *cursor;

    base = gFontArena;

    cursor = (u8 *)(((u32)base + 0x3F) & ~0x3F);
    gFontCache = (FontCache *)cursor;
    memset(gFontCache, 0, sizeof(FontCache));
    cursor += 0x3E80;
    gFontCache->font = 0;
    desc = gFontDescs;
    desc += gFontCache->font;
    gFontCache->unk04 = desc->unk00;
    gFontCache->width = desc->width;
    gFontCache->height = desc->height;
    gFontCache->unk10 = desc->unk0C;
    gFontCache->unk14 = desc->unk10;
    gFontCache->unk18 = desc->unk14;
    gFontCache->glyphs = desc->glyphs;
    gFontCache->glyphBytes = desc->glyphBytes;
    gFontCache->palInfo = (void *)desc->palInfo;
    gFontCache->palCount = desc->palCount;
    gFontCache->texCount = desc->texCount;
    gFontCache->unk30 = desc->unk30;
    gFontCache->unk34 = desc->unk34;
    gFontCache->unk38 = desc->unk38;
    bits = desc->bitsSize + 0x3F;
    bits &= ~0x3F;
    gFontCache->perPage = (0x100U / gFontCache->width) * (0x100U / gFontCache->height);
    gFontCache->max = gFontCache->perPage * gFontCache->texCount;
    for (i = 0; i < FONT_PAGES; i++) {
        gFontCache->pageGlyphs[i] = (u16 *)cursor;
        cursor += gFontCache->max * 2;
    }
    cursor = (u8 *)(((u32)cursor + 0x3F) & ~0x3F);
    gFontCache->tex = (u32 *)cursor;
    cursor += gFontCache->texCount * 4;
    gFontCache->pal = (u32 *)cursor;
    cursor += gFontCache->texCount * 4;
    cursor = (u8 *)(((u32)cursor + 0x3F) & ~0x3F);
    gFontCache->palPixels = cursor;
    cursor += ((gFontCache->palCount << 6) + 0x3F) & ~0x3F;
    gFontCache->bits = cursor;
    cursor += bits;
    gFontCache->runs[0] = (TextRun *)cursor;
    gFontCache->runs[1] = (TextRun *)(cursor + 0xA00);
    gFontCache->runs[2] = (TextRun *)(cursor + 0x1400);
    gFontCache->runs[3] = (TextRun *)(cursor + 0x1E00);
    gFontCache->runs[4] = (TextRun *)(cursor + 0x2800);
    gFontCache->runs[5] = (TextRun *)(cursor + 0x3200);
    gFontCache->runs[6] = (TextRun *)(cursor + 0x3C00);
    gFontCache->text = (char *)(cursor + 0x4600);
    gFontCache->defAdvance = gFontCache->width;
    gFontCache->defCharH = gFontCache->height;
    gFontCache->arenaUsed = (s32)cursor + 0x4600 + FONT_TEXT_SIZE - (s32)base;
    Font_CreateHandles();
    if (gFontCache->loaded == 0) {
        LoadCompressedFile(gFontCache->bits, FONT_FILE_ID);
        gFontCache->loaded = 1;
    }
    gFontCache->scale = (f32)gDispWidth / 640.0f;
    Font_ResetState();
}

static void Font_SelectPage(void);

/* Resets the glyph cache's text state: every page's runs (as Font_ClearPage), the position,
 * the size (10.0), the font's advance and height, the text buffer; then the page of new
 * runs (Font_SelectPage). */
void TextObj_Reset(void)
{
    s32 i;

    for (i = 0; i < FONT_PAGES; i++) {
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

/* Clears page `page`'s runs, this frame's and the last, and its drawn flag. */
void Font_ClearPage(s32 page)
{
    gFontCache->runCount[page] = 0;
    gFontCache->runPrev[page] = 0;
    gFontCache->pageDone[page] = 0;
}

/* Empties the glyph cache: every glyph's slot (FONT_SLOT_NONE) and every page's range. */
static void Font_ClearSlots(void)
{
    s32 i;
    u16 *slotEntry;

    gFontCache->used = 0;
    slotEntry = gFontCache->slot;
    for (i = 0; i < FONT_GLYPH_CODES; i++) {
        *slotEntry++ = FONT_SLOT_NONE;
    }
    for (i = 0; i < FONT_PAGES; i++) {
        gFontCache->pageFirst[i] = 0;
        gFontCache->pageEnd[i] = 0;
        gFontCache->pageDone[i] = 0;
    }
}

/* Empties the glyph cache and clears the count of runs that found their page full. */
void Font_FlushSlots(void)
{
    Font_ClearSlots();
    gFontRunCount = 0;
}

/* Resets the text state (TextObj_Reset), one-byte characters half wide. */
void Font_ResetState(void)
{
    TextObj_Reset();
    Font_SetHalfWidth(1);
}

/* Sets the four colours of palette slot `slot`: in gFontCache's table and in the locked palette. */
void Font_SetPalette(s32 slot, u32 handle, u32 colorA, u32 colorB, u32 colorC, u32 colorD)
{
    u32 colors[4];
    Image img;
    u32 *cacheDst;
    u32 *paletteDst;
    s32 i;

    if (handle == 0) {
        return;
    }
    colors[0] = colorA;
    colors[1] = colorB;
    colors[2] = colorC;
    colors[3] = colorD;
    flLockPalette(NULL, handle, &img, 2);
    cacheDst = (u32 *)(gFontCache->palPixels + slot * FONT_PAL_SIZE);
    paletteDst = (u32 *)img.pixels;
    for (i = 0; i < 4; i++) {
        *cacheDst++ = colors[i];
        *paletteDst++ = colors[i];
    }
    flUnlockPalette(handle);
}

/* The advance and the line height of new runs, in pixels. */
void Font_SetSpacing(u8 penStep, u8 lineStep)
{
    gFontCache->advance = (s8) penStep;
    gFontCache->charH = (s8) lineStep;
}

/* The page of the next text run, from the glyph size (size / 1000 of the 7 pages). */
static void Font_SelectPage(void)
{
    s32 page;

    page = (s32) (7.0f * (gFontCache->size / 1000.0f));
    if (page < 0) {
        page = 0;
    }
    if (page >= FONT_PAGES) {
        page = FONT_PAGES - 1;
    }
    gFontCache->runPage = page;
}

/* The next text run's position; also picks its page. */
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
    if (page >= FONT_PAGES) {
        page = FONT_PAGES - 1;
    }
    gFontCache->runPage = page;
}

/* The glyph size of new runs; it also picks their page (Font_SelectPage). */
void Font_SetSize(f32 size)
{
    gFontCache->size = size;
    Font_SelectPage();
}

/* The colour of new runs, 0xRRGGBB, with palette mode 0: kept in the GS's order (red in
 * the low byte) with alpha 0x80, which the GS reads as opaque. */
void Font_SetColor(s32 color)
{
    Font_SetPal(0);
    gFontCache->clutNew = (s32) ((color & 0xFF00) | (((color & 0xFF) << 0x10) | ((u32) (color & 0xFF0000) >> 0x10)) | 0x80000000);
}

/* As Font_SetColor, without setting the palette mode. */
static void Font_SetColorRaw(s32 rgb)
{
    gFontCache->clutNew = (s32) ((rgb & 0xFF00) | (((rgb & 0xFF) << 0x10) | ((u32) (rgb & 0xFF0000) >> 0x10)) | 0x80000000);
}

extern u32 gTextPulseColor;
extern u32 gFontColors[];

/*
 * Sets the text colour: -1 the pulsing one (gTextPulseColor), -2 and -3 palette modes 2
 * and 3 with colour 0, 0-15 that colour of gFontColors in mode 0. Font_SetColorRaw is
 * static (only this file calls it): MWCC then knows it leaves v0 alone and keeps v0 out
 * of the index, as in the game.
 */
void Font_SetColorIndex(s16 index)
{
    s16 mode;

    if (index >= 0x10) {
        return;
    }
    switch (index) {
    case -1:
        Font_SetPal(0);
        Font_SetColor(gTextPulseColor);
        return;
    case -2:
        mode = 2;
        index = 0;
        break;
    case -3:
        mode = 3;
        index = 0;
        break;
    default:
        if (index >= 0x10) {
            return;
        }
        mode = 0;
        break;
    }
    Font_SetPal(mode);
    Font_SetColorRaw(gFontColors[index]);
}

/* The palette mode of new runs. */
void Font_SetPal(s32 palette)
{
    gFontCache->palNew = palette;
}

s32 Font_GetPal(void)
{
    return gFontCache->palNew;
}

/* The colour of new runs, 0xAARRGGBB (Font_SetColor's order). */
u32 Font_GetColor(void)
{
    s32 clutNew;

    clutNew = gFontCache->clutNew;
    return ((clutNew & 0xFF) << 0x10) | ((clutNew & 0xFF00FF00) | ((u32) (clutNew & 0xFF0000) >> 0x10));
}

/* One-byte characters advance half a glyph; the argument is ignored. */
void Font_SetHalfWidth(s32 unused)
{
    gFontCache->halfWidth = 1;
}

/* A free text run on the page of new runs. While that page is full, the run moves to the
 * next page (Font_NextPage) and gFontRunCount counts it; NULL when the last page is full.
 * The caller counts the run in runCount. Font_NextPage's arguments keep the page's
 * address and the counts in the registers the game uses. */
TextRun *Font_NewRun(void)
{
    s32 *page;
    s32 runIndex;
    s32 oldPage;

    for (;;) {
        page = &gFontCache->runPage;
        runIndex = gFontCache->runCount[*page];
        if (runIndex < FONT_PAGE_RUNS) {
            break;
        }
        gFontRunCount++;
        oldPage = *page;
        Font_NextPage((s32)page, runIndex, oldPage);
        if (oldPage == gFontCache->runPage) {
            return NULL;
        }
    }
    return &gFontCache->runs[*page][runIndex];
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

s32 Font_IsLineGapOn(void)
{
    return gFontCache->lineGapOn;
}

s32 Font_GetLineGap(void)
{
    return (s16)gFontCache->lineGap;
}

void Font_SetLineGapOn(u8 value)
{
    gFontCache->lineGapOn = value;
}

void Font_SetLineGap(s32 value)
{
    gFontCache->lineGap = value;
}

s32 Str_Length(char *str)
{
    return strlen(str);
}

#include "ee_stdarg.h"
s32 vsprintf(char *buf, const char *fmt, va_list ap);

/* Adds a text run with the cache's current position, size, colours and
 * palette: a printf into the text buffer, or (glyph mode) a copy of the
 * glyph indices up to the negative one that ends them. */
void Font_Printf(char *fmt, ...)
{
    TextRun *run;
    va_list args;
    s16 *src;
    s16 *dst;

    run = Font_NewRun();
    if (run != NULL) {
        gFontCache->runCount[gFontCache->runPage]++;
        run->x = gFontCache->runX;
        run->y = gFontCache->runY;
        run->size = gFontCache->size;
        run->advance = gFontCache->advance;
        run->charH = gFontCache->charH;
        run->clut = gFontCache->clutNew;
        run->pal = gFontCache->palNew;
        run->isGlyphs = gFontCache->glyphMode;
        if (gFontCache->glyphMode == 0) {
            run->glyphs = (s16 *)(gFontCache->text + gFontCache->textLen);
            va_start(args, fmt);
            gFontCache->textLen += vsprintf((char *)run->glyphs, fmt, args);
            if (gFontCache->textLen >= FONT_TEXT_SIZE) {
                gFontCache->textLen = FONT_TEXT_SIZE - 1;
            }
            gFontCache->text[gFontCache->textLen] = 0;
            gFontCache->textLen++;
        } else {
            gFontCache->textLen = (gFontCache->textLen + 1) & ~1;
            run->glyphs = (s16 *)(gFontCache->text + gFontCache->textLen);
            dst = run->glyphs;
            src = (s16 *)fmt;
            for (;;) {
                *dst = *src;
                gFontCache->textLen += 2;
                if (*src < 0) {
                    break;
                }
                dst++;
                src++;
            }
            gFontCache->text[gFontCache->textLen] = 0;
            gFontCache->textLen++;
        }
    }
}

/* Draws the text runs of page idx once a frame; all pages; the frame's end. */
void Font_DrawPage(s32 idx)
{
    TextRun *run;
    s32 k;

    if (idx < FONT_PAGES) {
        if (gFontCache->pageDone[idx] == 0 && gFontCache->runCount[idx] != 0) {
            gFontCache->page = idx;
            Font_BeginPage();
            run = gFontCache->runs[idx];
            for (k = 0; k < gFontCache->runCount[idx]; k++, run++) {
                if (run->isGlyphs == 0) {
                    Text_DrawString(run);
                } else {
                    Text_DrawRun(run);
                }
            }
            Font_EndFrame();
            gFontCache->pageDone[idx] = 1;
        }
    }
}

/* Draws the text runs of all 7 pages. */
void Font_DrawAllPages(void)
{
    s32 i;

    for (i = 0; i < FONT_PAGES; i++) {
        Font_DrawPage(i);
    }
}

/* Draws the text runs of all 7 pages, then starts the next frame's runs. */
void Font_DrawAndClearPages(void)
{
    s32 i;

    for (i = 0; i < FONT_PAGES; i++) {
        Font_DrawPage(i);
        gFontCache->pageDone[i] = 0;
    }
    for (i = 0; i < FONT_PAGES; i++) {
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
    u32 *texHandle;
    s32 (*info)[4];
    u8 *pixels;
    u32 *palHandle;
    u32 palIndex;
    u32 handle;

    texHandle = gFontCache->tex;
    for (i = 0; i < gFontCache->texCount; i++) {
        *texHandle++ = flCreateTextureHandle(&gFontImage, 4);
    }
    palHandle = gFontCache->pal;
    info = gFontCache->palInfo;
    pixels = gFontCache->palPixels;
    for (palIndex = 0; palIndex < gFontCache->palCount; palIndex++) {
        gFontClutImage.pixels = pixels;
        handle = flCreatePaletteHandle(&gFontClutImage, 4);
        *palHandle++ = handle;
        Font_SetPalette(palIndex, handle, (*info)[0], (*info)[1], (*info)[2], (*info)[3]);
        info++;
        pixels += FONT_PAL_SIZE;
    }
    Font_ClearSlots();
}

void Font_DrawGlyph(s32 glyph, TextRun *r);
s32 Font_GlyphIndex(u32 code);

s32 Sjis_FromAscii(u8 c);

/* Draws the Shift-JIS string of run (newlines go down by run->charH). */
void Text_DrawString(TextRun *run)
{
    u8 *str = (u8 *)run->glyphs;
    u32 code;
    s32 narrow;
    s32 k;

    gFontCache->x = run->x;
    gFontCache->y = run->y;
    for (;;) {
        code = *str++;
        if (code == 0) {
            break;
        }
        if (code == '\n') {
            gFontCache->x = run->x;
            gFontCache->y += run->charH;
            continue;
        }
        if (SJIS_IS_LEAD(code)) {
            if (*str == 0) {
                break;
            }
            code <<= 8;
            code |= *str++;
            narrow = 0;
        } else {
            code = Sjis_FromAscii(code);
            narrow = 1;
        }
        if (code != SJIS_SPACE) {
            k = Font_GlyphIndex(code);
            if (k >= 0) {
                Font_DrawGlyph(k, run);
            }
        }
        if (!narrow) {
            gFontCache->x += run->advance;
        } else if (gFontCache->halfWidth != 0) {
            gFontCache->x += run->advance / 2;
        } else {
            gFontCache->x += run->advance * 2 / 3;
        }
    }
}


/* Draws the glyphs of run from its position, one advance apart (Font_DrawGlyph). */
void Text_DrawRun(TextRun *run)
{
    s16 *glyph = run->glyphs;
    s32 code;

    gFontCache->x = run->x;
    gFontCache->y = run->y;
    for (;;) {
        code = *glyph++;
        if (code < 0) {
            break;
        }
        if ((u32)code < gFontCache->glyphs) {
            Font_DrawGlyph(code, run);
        }
        gFontCache->x += run->advance;
    }
}

/* 2 for a Shift-JIS lead byte (0x80-0x9F, 0xE0-0xFF), 1 otherwise. */
s32 Sjis_CharBytes(u8 *str)
{
    s32 lead = *str;

    if (SJIS_IS_LEAD(lead)) {
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
    u32 *out;
    TexRec *tex;
    TexRec *clut;

    tex = &gTexRecs[(key & 0xFFFF) - 1];
    clut = &gPalRecs[((key & 0xFFFF0000) >> 16) - 1];
    out = (u32 *)gFontCache->pkt;

    if (gPacketEnd - (u32)out < 0x40) {
        return;
    }
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    out[3] = 0x50000003;
    ((u64 *)out)[2] = 0x1000000000008002;
    ((u64 *)out)[3] = 0xE;
    ((u64 *)out)[4] = GS_TEX0(tex->vram, 4, tex->psm, tex->tw, tex->th, 1, 0, clut->vram, clut->psm, 0, 0, 2);
    ((u64 *)out)[5] = GS_REG_TEX0_1;
    ((u64 *)out)[6] = ((u64)gFontCache->keySet << 5) | ((u64)gFontCache->keySet << 6);
    ((u64 *)out)[7] = GS_REG_TEX1_1;
    gFontCache->pkt += 0x40;
}

extern s32 gDispHeight; /* main */

/* Adds the sprite of the glyph in cache slot `slot`, glyphW x glyphH from the pen, to the
 * font packets (clut is the TEX0 palette word); the cells are cellW x cellH in 256-texel
 * pages. Static, as Font_DrawGlyph (its only caller) needs: MWCC then knows which
 * registers it clobbers.
 * The order and the types are the game's register use, kept for the match: the far texel
 * corner computed in place (`cellW = texU + cellW - 1; texV += cellH;`), the screen
 * offsets added in place after the frame-mode halving, top a copy of y scaled in place;
 * scale read before left, slot % perPage assigned back to slot, x an s16 (the game
 * extends it again for x + glyphW), y an int cast back to s16 for y + glyphH.
 */
static void Font_AddGlyphSprite(s32 slot, u32 clut, s16 glyphW, s16 glyphH)
{
    u32 *out;
    u32 texU;
    u32 texV;
    u32 left;
    u32 right;
    u32 top;
    s32 originX;
    u32 bottom;
    s32 originY;
    u32 cellW;
    u32 cellH;
    s16 x;
    s32 y;
    f32 scale;

    if (gPacketEnd - (u32)gFontCache->pkt < 0x50) {
        return;
    }
    cellW = gFontCache->width;
    cellH = gFontCache->height;
    gFontCache->sprites++;
    out = (u32 *)gFontCache->pkt;
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    out[3] = 0x50000004;
    ((u64 *)out)[2] = 0x8001 | ((u64)0x64000000 << 32);
    ((u64 *)out)[3] = 0x535310;
    ((u64 *)out)[4] = 0x156;
    ((u64 *)out)[5] = clut;
    slot = slot % gFontCache->perPage;
    texU = cellW * (slot % (0x100 / cellW));
    texV = cellH * (slot / (0x100 / cellH));
    ((u64 *)out)[6] = (u32)(texU * 16 + 8) | ((u64)(u32)(texV * 16 + 8) << 16);
    originX = ((0x1000 - gDispWidth) / 2) * 16;
    originY = ((0x1000 - gDispHeight) / 2) * 16;
    x = gFontCache->x;
    scale = gFontCache->scale;
    left = 16.0f * (x * scale);
    y = gFontCache->y;
    right = 16.0f * (scale * (x + glyphW));
    top = y;
    bottom = (s16)y + glyphH;
    top *= 16;
    bottom *= 16;
    if (gSysFrameMode == 0) {
        top >>= 1;
        bottom >>= 1;
    }
    left += originX;
    right += originX;
    top += originY;
    bottom += originY;
    cellW = texU + cellW - 1;
    texV += cellH;
    ((u64 *)out)[7] = (u32)left | ((u64)(u32)top << 16) | ((u64)0xFFFFFFFF << 32);
    ((u64 *)out)[8] = (u32)(cellW * 16 + 8) | ((u64)(u32)(texV * 16 + 8) << 16);
    ((u64 *)out)[9] = (u32)right | ((u64)(u32)bottom << 16) | ((u64)0xFFFFFFFF << 32);
    gFontCache->pkt += 0x50;
}

/*
 * Draws glyph `glyph` of run from the glyph cache: switches the texture and palette when
 * they change (Font_SetTexRegs), then the sprite (Font_AddGlyphSprite).
 * Its registers come from Font_AddGlyphSprite being static C defined before it: MWCC then
 * knows that it leaves v0 alone, keeps v0 live to the end and puts slot, clut and run in
 * t7-t9 as the game does. slot is an s32 (the game passes the lhu value without a mask).
 */
void Font_DrawGlyph(s32 glyph, TextRun *run)
{
    s32 clut;
    u32 key;
    s32 slot = gFontCache->slot[glyph];

    if (slot == FONT_SLOT_NONE) {
        return;
    }
    if (run->pal < 0) {
        clut = gFontCache->clut;
    } else {
        clut = run->clut;
    }
    key = gFontCache->tex[slot / gFontCache->perPage];
    key |= gFontCache->pal[(u8)run->pal] << 16;
    if (key != gFontCache->key || gFontCache->keySet != 1) {
        gFontCache->key = key;
        gFontCache->keySet = 1;
        Font_SetTexRegs(key);
    }
    Font_AddGlyphSprite(slot, clut, run->advance, run->charH);
}

extern s32 gPacketCur;
/* Starts drawing a page of text: the render states for glyphs (alpha test, no depth
 * write, blend 4, 5), the page's glyphs cached and uploaded (Font_CachePage,
 * Font_UploadGlyphs), and a new packet in the packet buffer, its first quadword kept
 * for the DMA tag Font_EndFrame writes. */
void Font_BeginPage(void)
{
    s32 pkt;

    Gs_SetState(0x61, 0x01000000);
    Gs_SetState(RS_ALPHA_REF, 0);
    Gs_SetState(RS_ALPHA_TEST, RS_CMP_NOTEQUAL);
    gSysSettings.blendA = 0xFF;
    gSysSettings.blendB = 0xFF;
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Gs_SetState(RS_ZWRITE, 0);
    gFontCache->unk100 = -1;
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
    u32 *texHandle;
    u32 *palHandle;
    s32 j;

    if (gFontCache->pktBase < gFontCache->pkt) {
        base = (u32 *)gFontCache->pktBase;
        base[0] = (((u32)(gFontCache->pkt - (u8 *)base) >> 4) - 1) | DMA_TAG_CNT;
        base[1] = 0;
        base[2] = 0;
        base[3] = 0;
        end = (u32 *)gFontCache->pkt;
        end[0] = DMA_TAG_IRQ | DMA_TAG_END | 1;
        end[1] = 0;
        end[2] = 0;
        end[3] = 0;
        *(u128 *)&end[4] = 0;
        gFontCache->pkt += 0x20;
        Packet_Alloc(gFontCache->pkt - gFontCache->pktBase, 0x10);
        DmaQueue_AddChain(0, (u32)gFontCache->pktBase & 0x0FFFFFFF, end, &gDmaQueue);
    }
    Gs_SetState(RS_ALPHA_TEST, RS_CMP_GREATER);
    Gs_SetState(RS_ZWRITE, 1);
    for (i = 0, texHandle = gFontCache->tex; i < gFontCache->texCount; i++, texHandle++) {
        if ((u16)*texHandle != 0) {
            Tex_Evict(&gTexRecs[(u16)*texHandle - 1]);
        }
    }
    for (j = 0, palHandle = gFontCache->pal; j < gFontCache->palCount; j++, palHandle++) {
        if (*palHandle != 0) {
            Tex_Evict(&gPalRecs[*palHandle - 1]);
        }
    }
}

s32 Font_SjisToJis(u32 code);

/* The low byte of a JIS code. An inline helper: written as `jis & 0xFF` in place,
 * MWCC schedules that mask before the high byte's shift. */
static inline s32 Jis_Low(s32 jis)
{
    return jis & 0xFF;
}

/* The font glyph index of a Shift-JIS code (row * 94 + column of its JIS code), or -1
 * past the table (FONT_GLYPH_CODES glyphs). */
s32 Font_GlyphIndex(u32 code)
{
    s32 jis = Font_SjisToJis(code);
    s32 i = ((jis >> 8) - 0x21) * 94 + (Jis_Low(jis) - 0x21);

    if (i >= FONT_GLYPH_CODES) {
        i = -1;
    }
    return i;
}

/*
 * Shift JIS to JIS X 0208: the first byte (0x81-0x9F, 0xE0-0xEF) gives the row pair, the
 * trail byte (0x40-0x7E, 0x80-0x9E, 0x9F-0xFC) the column and the odd/even row.
 * The second lead-byte case, the second trail-byte case and the sum are done in 64 bits
 * (daddiu, then dsll32/dsra32): `lead` is the first byte as a long, computed by the same
 * expression as jisHigh (MWCC shares the andi, so lead needs no masking or extension).
 */
s32 Font_SjisToJis(u32 sjis)
{
    s32 jisHigh = (sjis >> 8) & 0xFF;
    long lead = (sjis >> 8) & 0xFF;
    u32 jisLow = sjis & 0xFF;
    long trail;

    if (jisHigh >= 0x81 && jisHigh < 0xA0) {
        jisHigh -= 0x81;
    } else if (jisHigh >= 0xE0 && jisHigh < 0xF0) {
        jisHigh = lead - 0xC1;
    }
    trail = jisLow;
    jisHigh <<= 1;
    if (trail >= 0x40 && trail < 0x7F) {
        jisLow -= 0x40;
    } else if (trail >= 0x80 && trail < 0x9F) {
        jisLow = trail - 0x41;
    } else if (trail >= 0x9F && trail < 0xFD) {
        jisLow -= 0x9F;
        jisHigh++;
    }
    return (((long)(u32)jisHigh + 1) << 8) + (long)jisLow + 0x2021;
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
    s32 rows;
    s32 rowBytes;
    u16 *out;
    s32 packed;

    rows = gFontCache->height;
    rowBytes = (u32)((gFontCache->width + 3) & ~3) >> 2;
    for (row = 0; row < rows; row++) {
        out = (u16 *)dst;
        for (i = 0; i < rowBytes; i++) {
            packed = *src++;
            *out++ = ((packed & 0xC0) >> 6) | (packed & 0x30) | ((packed & 0xC) << 6) | ((packed & 3) << 12);
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

    for (i = 0; i < FONT_PAGES; i++) {
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
    FontCache *cache;
    s16 end;
    TextRun *run;
    s32 i;
    s32 n;
    s16 *glyph;
    u8 *str;
    u32 code;
    s32 k;

    cache = gFontCache;
    end = Font_MaxPageEnd();
    cache->pageEnd[cache->page] = end;
    cache->pageFirst[cache->page] = end;
    n = gFontCache->runCount[gFontCache->page];
    run = gFontCache->runs[gFontCache->page];
    for (i = 0; i < n; i++, run++) {
        if (run->isGlyphs != 0) {
            glyph = run->glyphs;
            for (;;) {
                k = *glyph++;
                if (k < 0) {
                    break;
                }
                Font_CacheGlyph(k);
            }
        } else {
            str = (u8 *)run->glyphs;
            for (;;) {
                code = *str++;
                if (code == 0) {
                    break;
                }
                if (code == '\n') {
                    continue;
                }
                if (SJIS_IS_LEAD(code)) {
                    if (*str == 0) {
                        break;
                    }
                    code <<= 8;
                    code |= *str++;
                } else {
                    code = Sjis_FromAscii(code);
                }
                if (code == SJIS_SPACE) {
                    continue;
                }
                k = Font_GlyphIndex(code);
                if (k >= 0 && k < FONT_GLYPH_CODES) {
                    Font_CacheGlyph(k);
                }
            }
        }
    }
}

/* Gives glyph code a slot of the cache (on the current page) unless it has one; 0 when full. */
s32 Font_CacheGlyph(u16 code)
{
    if (gFontCache->slot[code] != FONT_SLOT_NONE) {
        return 1;
    }
    if (gFontCache->used >= gFontCache->max) {
        return 0;
    }
    gFontCache->slot[code] = gFontCache->used;
    gFontCache->pageGlyphs[gFontCache->page][gFontCache->pageEnd[gFontCache->page] - gFontCache->pageFirst[gFontCache->page]] = code;
    gFontCache->used++;
    gFontCache->pageEnd[gFontCache->page]++;
    return 1;
}

/* The two-byte code of the glyph the font draws for a one-byte character: the full-width
 * space below 0x21, else a glyph of the font's row 0x85 (the trail byte skips 0x7F, as
 * Shift-JIS does; 0xA5 is the exception). */
s32 Sjis_FromAscii(u8 ascii)
{
    s32 code;

    code = ascii & 0xFF;
    if (code < 0x21) {
        return SJIS_SPACE;
    }
    if (code < 0x60) {
        return code + 0x7FFF + 0x520;
    }
    if (code < 0x80) {
        return code + 0x7FFF + 0x521;
    }
    if (code == 0xA5) {
        return 0x85A3;
    }
    return code + 0x7FFF + 0x4FF;
}

extern u8 *gSjisAsciiTable;

/* out = text with its one-byte characters widened to two-byte Shift-JIS ones (the table at
 * gSjisAsciiTable); two-byte characters are copied. */
void Sjis_Widen(char *text, char *out)
{
    u8 *src = (u8 *)text;
    u8 *dst = (u8 *)out;
    s32 lead;
    u8 *wideChar;

    while ((lead = *src) != 0) {
        if (SJIS_IS_LEAD(lead)) {
            dst[0] = lead;
            dst[1] = src[1];
            src += 2;
            dst += 2;
        } else {
            wideChar = gSjisAsciiTable + lead * 2;
            src++;
            dst[0] = wideChar[0];
            dst[1] = wideChar[1];
            dst += 2;
        }
    }
    *dst = 0;
}

extern char gSjisDigits[];
char *strncpy(char *dst, const char *src, u32 n);

/* dst = the last three decimal digits of n as two-byte Shift-JIS digits (gSjisDigits), no
 * terminator. */
void Sjis_PutDigits3(char *dst, s32 n)
{
    s32 i;
    s32 digit;

    for (i = 0, digit = 1000; i < 3; i++) {
        n %= digit;
        digit /= 10;
        strncpy(dst, &gSjisDigits[n / digit * 2], 2);
        dst += 2;
    }
}

f32 Math_Sin(f32 x);
extern u32 gTextPulseColor;

/* gTextPulseColor = a colour pulsing with a sine over 32 frames (around 0x50, 0xE4, 0xF7). */
void Text_UpdatePulse(void)
{
    f32 wave = Math_Sin(3.1415927f / 32768.0f * (u16)((gFontCache->pulse & 0x1F) << 11));
    s8 red = 64.0f * wave;
    s8 green = 20.0f * wave;
    s8 blue = 7.0f * wave;

    gTextPulseColor = 0xFF000000 | (u8)(red + 0x50) << 16 | (u8)(green + 0xE4) << 8 | (u8)(blue + 0xF7);
    gFontCache->pulse++;
}

/* gRsMaterials (main): a ModelMat per register 0x3A-0x59 (include/model.h). */

extern ModelEntry gModelPool[];
extern ModelMat gRsMaterials[];

void *Mem_Alloc(u32 size, s32 flag);
void Mem_Fill(void *dst, u8 c, s32 n);
/* unit's own declaration: declared as this file's callers use it; with the definition's void (void *dst, void *src, s32 n), Model_Create compiles differently */
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
    ModelEntry *entry;
    u8 *dst;

    for (i = 0; i < DRAW_MODEL_COUNT; i++) {
        if (gModelPool[i].used == 0) {
            break;
        }
    }
    entry = &gModelPool[i];
    Mem_Fill(entry, 0, sizeof(ModelEntry));
    entry->used = hdr->flags;
    entry->vtxFormat = hdr->format;
    entry->flags = flags;
    entry->unk50 = 0;
    entry->boneMask = 0;
    if (!(flags & MODEL_NO_COPY)) {
        entry->size = 0x30;
        entry->size += (hdr->stripsSize + 0xF) & ~0xF;
        entry->size += (hdr->vertsSize + 0xF) & ~0xF;
        entry->file = Mem_Alloc(entry->size, 3);
        dst = Mem_GetAddr(entry->file);
        Mem_Copy(dst, hdr, 0x24);
        dst += 0x30;
        Mem_Copy(dst, hdr->strips, hdr->stripsSize);
        dst += (hdr->stripsSize + 0xF) & ~0xF;
        Mem_Copy(dst, hdr->verts, hdr->vertsSize);
    }
    entry->renderFlags = gRenderFlags2;
    if (Model_Setup(entry, hdr) == 0) {
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
 * Sets up model entry from its mesh hdr: picks the VU1 program, sizes and allocates its
 * data, builds the VIF and part packets and collects the textures. Returns 1, or 0 when no
 * program fits. Counts the models (gModelCount).
 */
s32 Model_Setup(ModelEntry *entry, MeshDesc *hdr)
{
    Vu1_SelectProgram(entry, hdr);
    if (entry->prog != -1) {
        Model_SetupEntry(hdr, entry, 0);
        Model_BuildVifPackets(hdr, entry);
        Model_CollectTextures(entry, 0);
        Model_BuildPartPackets(entry, entry->partCount, entry->progInfo->pktKind, entry->pktOfs + (s32)Mem_GetAddr(entry->data), entry->mat);
    } else {
        return 0;
    }
    gModelCount++;
    return 1;
}

/*
 * Rebuilds the mesh hdr from model entry's copy of the file (Model_Create): its 0x24-byte
 * header, with strips and verts pointed into the copy (the strips at +0x30, the vertices
 * after them, 16-aligned).
 */
void Model_ReadHeader(ModelEntry *entry, MeshDesc *hdr)
{
    MeshDesc *base;
    u8 *data;

    data = Mem_GetAddr(entry->file);
    base = (MeshDesc *)data;
    data += 0x30;
    Mem_Copy(hdr, base, 0x24);
    hdr->strips = (u16 *)data;
    data += (base->stripsSize + 0xF) & ~0xF;
    hdr->verts = data;
}

extern s32 gModelPktSteps[];
extern s32 gVu1ProgLayouts[][2];

/*
 * Walks the strips of mesh hdr to size model entry's VIF data (batches cut at the program's
 * buffer (its descriptor's bufSize) and a new DMA CNT tag every 1 MB); counts the strips' vertices,
 * triangles and material runs, marks the materials used as the model's parts, and allocates
 * the data block (the VIF data, the chain table and the part packets).
 */
void Model_SetupEntry(MeshDesc *hdr, ModelEntry *entry, s32 unused)
{
    s8 used[0x20];
    s32 i;
    s32 total;
    s32 off;
    Vu1ProgInfo *info;
    u16 len;
    s32 step;
    s32 vtxBytes;
    s32 limit;
    s32 lastA;
    s32 curA;
    s32 cnt;
    u32 flags;
    s32 lastB;
    s32 curB;
    s32 count;
    u16 *stream;

    total = 0;
    info = entry->progInfo;
    off = 0;
    Mem_Fill(used, 0, 0x20);
    count = hdr->count;
    lastA = -1;
    stream = hdr->strips;
    curA = 0;
    lastB = lastA;
    curB = 0;
    step = 0;
    cnt = 0;
    entry->stripVtxCount = 0;
    entry->triCount = 0;
    entry->matRunCount = 0;
    limit = info->bufSize + 0x20;
    for (i = 0; i < count; i++) {
        len = *stream & MESH_STRIP_LEN;
        entry->stripVtxCount += len;
        entry->triCount += len - 2;
        flags = hdr->flags;
        stream++;
        if (flags & MESH_STRIP_MAT) {
            curA = *stream;
            stream++;
        }
        if (flags & MESH_STRIP_SKIN) {
            curB = *stream;
            stream++;
        }
        if (flags & MESH_INDEXED) {
            stream += len;
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
            entry->matRunCount++;
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
            vtxBytes = gVu1ProgLayouts[info->layout][0];
            do {
                if ((u32)(step + vtxBytes) >= (u32)info->bufSize) {
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
                step += vtxBytes;
            } while (len != 0);
        }
    }
    off += step;
    total += off;
    entry->vifSize = total;
    entry->partCount = 0;
    for (i = 0; i < 0x20; i++) {
        if (used[i] != 0) {
            entry->mat[entry->partCount++] = i;
        }
    }
    entry->chainTblSize = (entry->partCount * 4 + 0xF) & ~0xF;
    entry->pktSize = (entry->partCount * gModelPktSteps[info->pktKind] + 0xF) & ~0xF;
    entry->pktSize += 0x20;
    entry->chainTblOfs = (entry->vifSize + 0xF) & ~0xF;
    entry->pktOfs = entry->chainTblOfs + ((entry->chainTblSize + 0xF) & ~0xF);
    count = ((total + 0xF) & ~0xF);
    count += ((entry->chainTblSize + 0xF) & ~0xF);
    count += ((entry->pktSize + 0xF) & ~0xF);
    entry->data = Mem_Alloc(count, 3);
    entry->dataAddr = Mem_GetAddr(entry->data);
}

/* The GIF tag fields per program (Vu1ProgInfo.gifInfo): PRIM, REGS and NREG. */
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
 * batch with its GIF tag. Batches are cut where the program's buffer (its descriptor's bufSize) would
 * overflow, and a DMA CNT tag every 1 MB. offs (at data + chainTblOfs) gets each material's
 * chain.
 */
void Model_BuildVifPackets(MeshDesc *mesh, ModelEntry *entry)
{
    Vu1ProgInfo *info = entry->progInfo;
    s32 bits = gVu1ProgLayouts[info->layout][1];
    s32 boneIdx[4] = {0};
    s32 boneWt[4] = {0};
    u32 starts[0x20];
    u32 base;
    u32 unpackPos;
    u32 tag;
    s32 cnt;
    s32 lastA;
    s32 curA;
    s32 lastB;
    s32 curB;
    u32 step;
    u32 vtxBytes;
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
    u32 out;
    u16 *stream;
    u32 *walk;
    u32 *w;
    f32 *weightOut;
    u32 *lastQw;
    MeshVtxCol *colVtx;
    MeshVtxTex *texVtx;
    MeshVtxSkin *skinVtx;

    for (i = 0; i < 0x20; i++) {
        starts[i] = 0;
    }
    prim = gVu1GifInfos[info->gifInfo].prim;
    regs = gVu1GifInfos[info->gifInfo].regs;
    nreg = gVu1GifInfos[info->gifInfo].nreg;
    if ((entry->renderFlags & 0x18000) == 0x8000) {
        prim |= 0x20;
    }
    base = (u32)Mem_GetAddr(entry->data);
    out = base;
    stream = mesh->strips;
    count = mesh->count;
    limit = info->bufSize + 0x20;
    lastA = -1;
    lastB = lastA;
    curA = 0;
    curB = 0;
    cnt = 0;
    offs = (s32 *)(base + entry->chainTblOfs);
    for (i = 0; i < count; i++) {
        raw = *stream;
        toggle = 0;
        stream++;
        if (mesh->flags & MESH_STRIP_MAT) {
            curA = *stream;
            stream++;
        }
        if (mesh->flags & MESH_STRIP_SKIN) {
            curB = *stream;
            stream++;
        }
        if (lastA != curA) {
            if (cnt != 0) {
                unpackPos = (u32)Vif_UnpackV4((u128 *)unpackPos, step >> 4, 0);
                switch (lastB) {
                case 0:
                    ((u32 *)unpackPos)[0] = 1;
                    break;
                case 1:
                    ((u32 *)unpackPos)[0] = 2;
                    break;
                }
                ((u32 *)unpackPos)[1] = ((nreg << 4) * (cnt + 2) >> 4) + 1;
                ((u64 *)unpackPos)[1] = 0;
                ((u64 *)unpackPos)[2] = GIF_TAG(cnt, prim, nreg);
                ((u32 *)unpackPos)[6] = regs;
                ((u32 *)unpackPos)[7] = 0;
                out = (u32)Vif_MsCnt((u128 *)out);
                if (tag == starts[lastA]) {
                    DmaTag_Ret((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
                    offs[lastA] = tag - base;
                } else {
                    walk = (u32 *)starts[lastA];
                    do {
                        switch (walk[0] & DMA_TAG_ID_MASK) {
                        case DMA_TAG_CNT:
                        case DMA_TAG_NEXT:
                            walk = (u32 *)((u8 *)walk + ((walk[0] & 0xFFFF) + 1) * 16);
                            break;
                        case DMA_TAG_RET:
                            DmaTag_Next((u128 *)walk, walk[0] & 0xFFFF, tag, 0, 0);
                            walk = NULL;
                            break;
                        }
                    } while (walk != NULL);
                    DmaTag_Ret((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
                }
                cnt = 0;
            }
            tag = out;
            lastA = curA;
            lastB = curB;
            if (starts[curA] == 0) {
                starts[curA] = out;
            }
            unpackPos = out + 0x10;
            out += 0x40;
            step = 0x20;
        } else if (lastB != curB) {
            if (cnt != 0) {
                w = (u32 *)Vif_UnpackV4((u128 *)unpackPos, step >> 4, 0);
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
                out = (u32)Vif_MsCnt((u128 *)out);
                if (out - tag + limit >= 0x100000) {
                    DmaTag_Cnt((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
                    tag = out;
                    out += 0x10;
                }
                cnt = 0;
            }
            unpackPos = out;
            out += 0x30;
            lastB = curB;
            step = 0x20;
        }
        for (j = 0; j < (raw & MESH_STRIP_LEN); j++) {
            vtxBytes = gVu1ProgLayouts[info->layout][0];
            if (step + vtxBytes >= (u32)info->bufSize) {
                w = (u32 *)Vif_UnpackV4((u128 *)unpackPos, step >> 4, 0);
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
                out = (u32)Vif_MsCnt((u128 *)out);
                if (out - tag + limit >= 0x100000) {
                    DmaTag_Cnt((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
                    tag = out;
                    out += 0x10;
                }
                unpackPos = out;
                step = 0x20;
                out += 0x30;
                cnt = 0;
            }
            step += vtxBytes;
            cnt++;
            switch (mesh->format) {
            case 0x25:
                if (mesh->flags & MESH_INDEXED) {
                    colVtx = &((MeshVtxCol *)mesh->verts)[*stream];
                    stream++;
                } else {
                    colVtx++;
                }
                if (bits & 1) {
                    ((f32 *)out)[0] = colVtx->pos[0];
                    ((f32 *)out)[1] = colVtx->pos[1];
                    ((f32 *)out)[2] = colVtx->pos[2];
                    if (j < 2) {
                        ((u32 *)out)[3] = 0x8000;
                    } else {
                        ((u32 *)out)[3] = 0;
                    }
                    out += 0x10;
                }
                if (bits & 2) {
                    ((f32 *)out)[0] = colVtx->nrm[0];
                    ((f32 *)out)[1] = colVtx->nrm[1];
                    ((f32 *)out)[2] = colVtx->nrm[2];
                    if (toggle == 0) {
                        ((f32 *)out)[3] = 1.0f;
                    } else {
                        ((f32 *)out)[3] = -1.0f;
                    }
                    out += 0x10;
                }
                if (bits & 4) {
                    if (!(entry->renderFlags & 0xF00)) {
                        ((f32 *)out)[0] = (colVtx->col >> 16) & 0xFF;
                        ((f32 *)out)[1] = (colVtx->col >> 8) & 0xFF;
                        ((f32 *)out)[2] = colVtx->col & 0xFF;
                    } else {
                        ((f32 *)out)[0] = Gs_AlphaFromByte((colVtx->col >> 16) & 0xFF, 1);
                        ((f32 *)out)[1] = Gs_AlphaFromByte((colVtx->col >> 8) & 0xFF, 1);
                        ((f32 *)out)[2] = Gs_AlphaFromByte(colVtx->col & 0xFF, 1);
                    }
                    ((f32 *)out)[3] = Gs_AlphaFromByte((colVtx->col >> 24) & 0xFF, 0);
                    out += 0x10;
                }
                break;
            case 0x35:
                if (mesh->flags & MESH_INDEXED) {
                    texVtx = &((MeshVtxTex *)mesh->verts)[*stream];
                    stream++;
                } else {
                    texVtx++;
                }
                if (bits & 1) {
                    ((f32 *)out)[0] = texVtx->pos[0];
                    ((f32 *)out)[1] = texVtx->pos[1];
                    ((f32 *)out)[2] = texVtx->pos[2];
                    if (j < 2) {
                        ((u32 *)out)[3] = 0x8000;
                    } else {
                        ((u32 *)out)[3] = 0;
                    }
                    out += 0x10;
                }
                if (bits & 2) {
                    ((f32 *)out)[0] = texVtx->nrm[0];
                    ((f32 *)out)[1] = texVtx->nrm[1];
                    ((f32 *)out)[2] = texVtx->nrm[2];
                    if (toggle == 0) {
                        ((f32 *)out)[3] = 1.0f;
                    } else {
                        ((f32 *)out)[3] = -1.0f;
                    }
                    out += 0x10;
                }
                if (bits & 4) {
                    if (!(entry->renderFlags & 0xF00)) {
                        ((f32 *)out)[0] = Gs_AlphaFromByte((texVtx->col >> 16) & 0xFF, 0);
                        ((f32 *)out)[1] = Gs_AlphaFromByte((texVtx->col >> 8) & 0xFF, 0);
                        ((f32 *)out)[2] = Gs_AlphaFromByte(texVtx->col & 0xFF, 0);
                    } else {
                        ((f32 *)out)[0] = Gs_AlphaFromByte((texVtx->col >> 16) & 0xFF, 1);
                        ((f32 *)out)[1] = Gs_AlphaFromByte((texVtx->col >> 8) & 0xFF, 1);
                        ((f32 *)out)[2] = Gs_AlphaFromByte(texVtx->col & 0xFF, 1);
                    }
                    ((f32 *)out)[3] = Gs_AlphaFromByte((texVtx->col >> 24) & 0xFF, 0);
                    out += 0x10;
                }
                if (bits & 8) {
                    lastQw = (u32 *)out;
                    ((f32 *)out)[0] = texVtx->uv[0];
                    ((f32 *)out)[1] = texVtx->uv[1];
                    ((f32 *)out)[2] = 1.0f;
                    ((u32 *)out)[3] = 0;
                    out += 0x10;
                    if (entry->flags & MODEL_SCALE_UV) {
                        ((f32 *)lastQw)[0] = ((f32 *)lastQw)[0] * gDispTexScaleU;
                        ((f32 *)lastQw)[1] = ((f32 *)lastQw)[1] * gDispTexScaleV;
                    }
                }
                break;
            case 0x135:
                if (mesh->flags & MESH_INDEXED) {
                    skinVtx = &((MeshVtxSkin *)mesh->verts)[*stream];
                    stream++;
                } else {
                    skinVtx++;
                }
                if (bits & 1) {
                    ((f32 *)out)[0] = skinVtx->pos[0];
                    ((f32 *)out)[1] = skinVtx->pos[1];
                    ((f32 *)out)[2] = skinVtx->pos[2];
                    if (j < 2) {
                        ((u32 *)out)[3] = 0x8000;
                    } else {
                        ((u32 *)out)[3] = 0;
                    }
                    out += 0x10;
                }
                if (bits & 2) {
                    ((f32 *)out)[0] = skinVtx->nrm[0];
                    ((f32 *)out)[1] = skinVtx->nrm[1];
                    ((f32 *)out)[2] = skinVtx->nrm[2];
                    if (toggle == 0) {
                        ((f32 *)out)[3] = 1.0f;
                    } else {
                        ((f32 *)out)[3] = -1.0f;
                    }
                    out += 0x10;
                }
                if (bits & 4) {
                    if (!(entry->renderFlags & 0xF00)) {
                        ((f32 *)out)[0] = Gs_AlphaFromByte((skinVtx->col >> 16) & 0xFF, 0);
                        ((f32 *)out)[1] = Gs_AlphaFromByte((skinVtx->col >> 8) & 0xFF, 0);
                        ((f32 *)out)[2] = Gs_AlphaFromByte(skinVtx->col & 0xFF, 0);
                    } else {
                        ((f32 *)out)[0] = Gs_AlphaFromByte((skinVtx->col >> 16) & 0xFF, 1);
                        ((f32 *)out)[1] = Gs_AlphaFromByte((skinVtx->col >> 8) & 0xFF, 1);
                        ((f32 *)out)[2] = Gs_AlphaFromByte(skinVtx->col & 0xFF, 1);
                    }
                    ((f32 *)out)[3] = Gs_AlphaFromByte((skinVtx->col >> 24) & 0xFF, 0);
                    out += 0x10;
                }
                if (bits & 8) {
                    lastQw = (u32 *)out;
                    ((f32 *)out)[0] = skinVtx->uv[0];
                    ((f32 *)out)[1] = skinVtx->uv[1];
                    ((f32 *)out)[2] = 1.0f;
                    ((u32 *)out)[3] = 0;
                    out += 0x10;
                    if (entry->flags & MODEL_SCALE_UV) {
                        ((f32 *)lastQw)[0] = ((f32 *)lastQw)[0] * gDispTexScaleU;
                        ((f32 *)lastQw)[1] = ((f32 *)lastQw)[1] * gDispTexScaleV;
                    }
                }
                if ((bits & 0x10) && (bits & 0x20)) {
                    lastQw = (u32 *)out;
                    weightOut = (f32 *)(out + 0x10);
                    out += 0x20;
                    boneIdx[0] = (skinVtx->bones >> 24) & 0xFF;
                    boneIdx[1] = (skinVtx->bones >> 16) & 0xFF;
                    boneIdx[2] = (skinVtx->bones >> 8) & 0xFF;
                    boneIdx[3] = skinVtx->bones & 0xFF;
                    boneWt[0] = (skinVtx->weights >> 24) & 0xFF;
                    boneWt[1] = (skinVtx->weights >> 16) & 0xFF;
                    boneWt[2] = (skinVtx->weights >> 8) & 0xFF;
                    boneWt[3] = skinVtx->weights & 0xFF;
                    *(u128 *)lastQw = 0;
                    *(u128 *)weightOut = 0;
                    switch (lastB) {
                    case 0:
                        lastQw[0] = (u32)&(info->bones)->base[boneIdx[0]];
                        entry->boneMask |= 1 << boneIdx[0];
                        lastQw[1] = (u32)&(info->bones)->base[(info->bones)->off] + boneIdx[0] * (info->bones)->size;
                        weightOut[0] = 100.0f;
                        break;
                    case 1:
                        for (k = 0; k < 4; k++) {
                            lastQw[k] = (u32)&(info->bones)->base[boneIdx[k]];
                            entry->boneMask |= 1 << boneIdx[k];
                            weightOut[k] = boneWt[k] / 255.0f;
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
        w = (u32 *)Vif_UnpackV4((u128 *)unpackPos, step >> 4, 0);
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
        out = (u32)Vif_MsCnt((u128 *)out);
    }
    if (tag == starts[lastA]) {
        DmaTag_Ret((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
        offs[lastA] = tag - base;
    } else {
        walk = (u32 *)starts[lastA];
        do {
            switch (walk[0] & DMA_TAG_ID_MASK) {
            case DMA_TAG_CNT:
            case DMA_TAG_NEXT:
                walk = (u32 *)((u8 *)walk + ((walk[0] & 0xFFFF) + 1) * 16);
                break;
            case DMA_TAG_RET:
                DmaTag_Next((u128 *)walk, walk[0] & 0xFFFF, tag, 0, 0);
                walk = NULL;
                break;
            }
        } while (walk != NULL);
        DmaTag_Ret((u128 *)tag, ((out - tag) >> 4) - 1, 0, 0);
    }
    if (entry->vifSize != out - base) {
        Debug_Printf("A = %x B = %x, %x @flPS2ConvClayData", base, out, entry->vifSize);
    }
}

/* A 0..255 alpha as the GS's 0..128 (mode 0) or as 0..1 (mode 1). */
f32 Gs_AlphaFromByte(u32 byteVal, s32 mode)
{
    f32 result = 0.0f;

    if (mode == 0) {
        if (byteVal == 0xFF) {
            result = 128.0f;
        } else if (byteVal != 0) {
            byteVal >>= 1;
            if (byteVal == 0) {
                byteVal = 1;
            }
            result = byteVal;
        }
    } else {
        result = byteVal / 255.0f;
        if (result > 1.0f) {
            result = 1.0f;
        }
        if (result < 0.0f) {
            result = 0.0f;
        }
    }
    return result;
}

void Model_BuildPartPacket(u8 *part, u32 *dst, s32 prog, s32 mat);
extern s32 gModelPktSteps[];

/* The DMA packets of the n parts mat[] of model e (Model_BuildPartPacket, a step of gModelPktSteps[prog]
 * bytes each) from dst, closed by an END tag. */
void Model_BuildPartPackets(ModelEntry *entry, s32 n, s32 prog, s32 dst, u8 *mat)
{
    s32 i;
    s32 step;
    s32 *parts;
    u8 *base;

    step = gModelPktSteps[prog];
    base = entry->dataAddr;
    parts = (s32 *)(base + entry->chainTblOfs);
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
    ModelMat *material = &gRsMaterials[mat];
    u8 *pkt;
    s32 lod;
    s32 isPow2;

    switch (prog) {
    case 0:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 0xC, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 0x10), 0xA, 0);
        *(u128 *)(pkt + 0x20) = 0;
        *(u64 *)(pkt + 0x30) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 0x38) = 0xE;
        *(u32 *)(pkt + 0x3C) = 0;
        Gs_SetTexRegs(material->tex, (u64 *)(pkt + 0x40), (u64 *)(pkt + 0x50), (u64 *)(pkt + 0x60),
                      (u64 *)(pkt + 0x70), (u64 *)(pkt + 0x80), (u64 *)(pkt + 0x90));
        *(f32 *)(pkt + 0xA0) = material->rgb[0];
        *(f32 *)(pkt + 0xA4) = material->rgb[1];
        *(f32 *)(pkt + 0xA8) = material->rgb[2];
        *(f32 *)(pkt + 0xAC) = 128.0f * material->alpha;
        *(f32 *)(pkt + 0xB0) = material->unk24[0];
        *(f32 *)(pkt + 0xB4) = material->unk24[1];
        *(f32 *)(pkt + 0xB8) = material->unk24[2];
        *(f32 *)(pkt + 0xBC) = material->unk24[3];
        Vif_MsCnt((u128 *)(pkt + 0xC0));
        break;
    case 1:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 0xC, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 0x10), 0xA, 0);
        *(u128 *)(pkt + 0x20) = 0;
        *(u64 *)(pkt + 0x30) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 0x38) = 0xE;
        *(u32 *)(pkt + 0x3C) = 0;
        *(u64 *)(pkt + 0x40) = 0;
        *(u64 *)(pkt + 0x48) = 0x3B;
        *(u64 *)(pkt + 0x50) = 0;
        *(u64 *)(pkt + 0x58) = 0x14;
        *(u64 *)(pkt + 0x60) = 0;
        *(u64 *)(pkt + 0x68) = 6;
        *(u64 *)(pkt + 0x70) = 0;
        *(u64 *)(pkt + 0x78) = 8;
        *(u64 *)(pkt + 0x80) = 0;
        *(u64 *)(pkt + 0x88) = 0x34;
        *(u64 *)(pkt + 0x90) = 0;
        *(u64 *)(pkt + 0x98) = 0x36;
        *(f32 *)(pkt + 0xA0) = material->rgb[0];
        *(f32 *)(pkt + 0xA4) = material->rgb[1];
        *(f32 *)(pkt + 0xA8) = material->rgb[2];
        *(f32 *)(pkt + 0xAC) = 128.0f * material->alpha;
        *(f32 *)(pkt + 0xB0) = material->unk24[0];
        *(f32 *)(pkt + 0xB4) = material->unk24[1];
        *(f32 *)(pkt + 0xB8) = material->unk24[2];
        *(f32 *)(pkt + 0xBC) = material->unk24[3];
        Vif_MsCnt((u128 *)(pkt + 0xC0));
        break;
    case 2:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 0xE, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 0x10), 0xC, 0);
        *(u128 *)(pkt + 0x20) = 0;
        *(u64 *)(pkt + 0x30) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 0x38) = 0xE;
        *(u32 *)(pkt + 0x3C) = 0;
        Gs_SetTexRegs(material->tex, (u64 *)(pkt + 0x40), (u64 *)(pkt + 0x50), (u64 *)(pkt + 0x60),
                      (u64 *)(pkt + 0x70), (u64 *)(pkt + 0x80), (u64 *)(pkt + 0x90));
        *(f32 *)(pkt + 0xA0) = material->rgb[0];
        *(f32 *)(pkt + 0xA4) = material->rgb[1];
        *(f32 *)(pkt + 0xA8) = material->rgb[2];
        *(f32 *)(pkt + 0xAC) = 128.0f * material->alpha;
        *(f32 *)(pkt + 0xB0) = material->unk24[0];
        *(f32 *)(pkt + 0xB4) = material->unk24[1];
        *(f32 *)(pkt + 0xB8) = material->unk24[2];
        *(f32 *)(pkt + 0xBC) = material->unk24[3];
        *(f32 *)(pkt + 0xC0) = material->unk14[0];
        *(f32 *)(pkt + 0xC4) = material->unk14[1];
        *(f32 *)(pkt + 0xC8) = material->unk14[2];
        *(f32 *)(pkt + 0xCC) = material->unk14[3];
        switch (material->size) {
        case 0:
        case 1:
        case 2:
            isPow2 = lod = 1;
            break;
        case 4:
            lod = 2;
            isPow2 = 1;
            break;
        case 8:
            lod = 3;
            isPow2 = 1;
            break;
        case 0x10:
            lod = 4;
            isPow2 = 1;
            break;
        case 0x20:
            lod = 5;
            isPow2 = 1;
            break;
        case 0x40:
            lod = 6;
            isPow2 = 1;
            break;
        case 0x80:
            lod = 7;
            isPow2 = 1;
            break;
        case 0x100:
            lod = 8;
            isPow2 = 1;
            break;
        default:
            lod = material->size - 1;
            isPow2 = 0;
            break;
        }
        *(s32 *)(pkt + 0xD0) = lod;
        *(s32 *)(pkt + 0xD4) = isPow2;
        *(s32 *)(pkt + 0xD8) = 0;
        *(s32 *)(pkt + 0xDC) = 0;
        Vif_MsCnt((u128 *)(pkt + 0xE0));
        break;
    case 3:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 0xA, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 0x10), 8, 0);
        *(u128 *)(pkt + 0x20) = 0;
        *(u64 *)(pkt + 0x30) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 0x38) = 0xE;
        *(u32 *)(pkt + 0x3C) = 0;
        Gs_SetTexRegs(material->tex, (u64 *)(pkt + 0x40), (u64 *)(pkt + 0x50), (u64 *)(pkt + 0x60),
                      (u64 *)(pkt + 0x70), (u64 *)(pkt + 0x80), (u64 *)(pkt + 0x90));
        Vif_MsCnt((u128 *)(pkt + 0xA0));
        break;
    case 4:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 0xB, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 0x10), 9, 0);
        *(u128 *)(pkt + 0x20) = 0;
        *(u64 *)(pkt + 0x30) = 0x8006 | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 0x38) = 0xE;
        *(u32 *)(pkt + 0x3C) = 0;
        *(u64 *)(pkt + 0x40) = 0;
        *(u64 *)(pkt + 0x48) = 0x3B;
        *(u64 *)(pkt + 0x50) = 0;
        *(u64 *)(pkt + 0x58) = 0x14;
        *(u64 *)(pkt + 0x60) = 0;
        *(u64 *)(pkt + 0x68) = 6;
        *(u64 *)(pkt + 0x70) = 0;
        *(u64 *)(pkt + 0x78) = 8;
        *(u64 *)(pkt + 0x80) = 0;
        *(u64 *)(pkt + 0x88) = 0x34;
        *(u64 *)(pkt + 0x90) = 0;
        *(u64 *)(pkt + 0x98) = 0x36;
        *(u128 *)(pkt + 0xA0) = 0;
        Vif_MsCnt((u128 *)(pkt + 0xB0));
        break;
    case 5:
        pkt = (u8 *)dst;
        DmaTag_Call((u128 *)pkt, 0x10, (u32)part, 0, 0);
        Vif_UnpackV4((u128 *)(pkt + 0x10), 0xE, 0);
        *(u128 *)(pkt + 0x20) = 0;
        *(u64 *)(pkt + 0x30) = 0x800B | ((u64)0x10000000 << 32);
        *(u32 *)(pkt + 0x38) = 0xE;
        *(u32 *)(pkt + 0x3C) = 0;
        Gs_SetTexRegs2(material->tex, (u64 *)(pkt + 0x40), (u64 *)(pkt + 0x50), (u64 *)(pkt + 0x60),
                       (u64 *)(pkt + 0x70), (u64 *)(pkt + 0x80), (u64 *)(pkt + 0x90),
                       D_0020E134, (u64 *)(pkt + 0xA0), (u64 *)(pkt + 0xB0),
                       (u64 *)(pkt + 0xC0), (u64 *)(pkt + 0xD0), (u64 *)(pkt + 0xE0));
        *(f32 *)(pkt + 0xF0) = material->rgb[0];
        *(f32 *)(pkt + 0xF4) = material->rgb[1];
        *(f32 *)(pkt + 0xF8) = material->rgb[2];
        *(f32 *)(pkt + 0xFC) = 128.0f * material->alpha;
        Vif_MsCnt((u128 *)(pkt + 0x100));
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
void Gs_SetTexRegs(u32 tex, u64 *texa, u64 *tex1Reg, u64 *tex0Reg, u64 *clamp, u64 *miptbp1Reg, u64 *miptbp2Reg)
{
    if (tex != 0) {
        Gs_BuildTexRegs(tex, texa, tex1Reg, tex0Reg, clamp, miptbp1Reg, miptbp2Reg, gRenderFlags);
        texa[1] = GS_REG_TEXA;
        tex1Reg[1] = GS_REG_TEX1_1;
        tex0Reg[1] = GS_REG_TEX0_1;
        clamp[1] = GS_REG_CLAMP_1;
        miptbp1Reg[1] = GS_REG_MIPTBP1_1;
        miptbp2Reg[1] = GS_REG_MIPTBP2_1;
    } else {
        texa[0] = 0;
        texa[1] = GS_REG_TEXA;
        tex1Reg[0] = 0;
        tex1Reg[1] = GS_REG_TEX1_1;
        tex0Reg[0] = 0;
        tex0Reg[1] = GS_REG_TEX0_1;
        clamp[0] = 0;
        clamp[1] = GS_REG_CLAMP_1;
        miptbp1Reg[0] = 0;
        miptbp1Reg[1] = GS_REG_MIPTBP1_1;
        miptbp2Reg[0] = 0;
        miptbp2Reg[1] = GS_REG_MIPTBP2_1;
    }
}

/* As Gs_SetTexRegs for both drawing contexts: tex for context 1, tex2 for context 2 (whose
 * MIPTBP registers get 0x34 and 0x36 when it has a texture, 0x35 and 0x37 when not). */
void Gs_SetTexRegs2(u32 tex, u64 *texa, u64 *tex1Reg, u64 *tex0Reg, u64 *clamp, u64 *miptbp1Reg, u64 *miptbp2Reg,
                   u32 tex2, u64 *tex1Reg2, u64 *tex0Reg2, u64 *clamp2, u64 *miptbp1Reg2, u64 *miptbp2Reg2)
{
    if (tex != 0) {
        Gs_BuildTexRegs(tex, texa, tex1Reg, tex0Reg, clamp, miptbp1Reg, miptbp2Reg, gRenderFlags);
        texa[1] = GS_REG_TEXA;
        tex1Reg[1] = GS_REG_TEX1_1;
        tex0Reg[1] = GS_REG_TEX0_1;
        clamp[1] = GS_REG_CLAMP_1;
        miptbp1Reg[1] = GS_REG_MIPTBP1_1;
        miptbp2Reg[1] = GS_REG_MIPTBP2_1;
    } else {
        texa[0] = 0;
        texa[1] = GS_REG_TEXA;
        tex1Reg[0] = 0;
        tex1Reg[1] = GS_REG_TEX1_1;
        tex0Reg[0] = 0;
        tex0Reg[1] = GS_REG_TEX0_1;
        clamp[0] = 0;
        clamp[1] = GS_REG_CLAMP_1;
        miptbp1Reg[0] = 0;
        miptbp1Reg[1] = GS_REG_MIPTBP1_1;
        miptbp2Reg[0] = 0;
        miptbp2Reg[1] = GS_REG_MIPTBP2_1;
    }
    if (tex2 != 0) {
        Gs_BuildTexRegs(tex2, texa, tex1Reg2, tex0Reg2, clamp2, miptbp1Reg2, miptbp2Reg2, gRenderFlags);
        texa[1] = GS_REG_TEXA;
        tex1Reg2[1] = GS_REG_TEX1_2;
        tex0Reg2[1] = GS_REG_TEX0_2;
        clamp2[1] = GS_REG_CLAMP_2;
        miptbp1Reg2[1] = GS_REG_MIPTBP1_1;
        miptbp2Reg2[1] = GS_REG_MIPTBP2_1;
    } else {
        texa[0] = 0;
        texa[1] = GS_REG_TEXA;
        tex1Reg2[0] = 0;
        tex1Reg2[1] = GS_REG_TEX1_2;
        tex0Reg2[0] = 0;
        tex0Reg2[1] = GS_REG_TEX0_2;
        clamp2[0] = 0;
        clamp2[1] = GS_REG_CLAMP_2;
        miptbp1Reg2[0] = 0;
        miptbp1Reg2[1] = GS_REG_MIPTBP1_2;
        miptbp2Reg2[0] = 0;
        miptbp2Reg2[1] = GS_REG_MIPTBP2_2;
    }
}

/* The textures of e: partTex[i] = the texture of part i's material, tex[] the distinct ones. */
static void Model_CollectTextures(ModelEntry *entry, s32 unused)
{
    s32 i;
    s32 j;
    ModelMat *material;

    entry->texCount = 0;
    for (i = 0; i < entry->partCount; i++) {
        material = &gRsMaterials[entry->mat[i]];
        entry->partTex[i] = 0;
        if (material->tex != 0) {
            entry->partTex[i] = material->tex;
            for (j = 0; j < entry->texCount; j++) {
                if (entry->tex[j] == material->tex) {
                    goto next;
                }
            }
            entry->tex[entry->texCount++] = material->tex;
        }
    next:;
    }
}

void Model_Relocate(ModelEntry *e);

/* Model_Relocate for every model in use that has its data. */
void Model_RelocateAll(void)
{
    s32 i;
    ModelEntry *entry;

    i = 0;
    entry = gModelPool;
    do {
        if (entry->used != 0 && entry->data != NULL) {
            Model_Relocate(entry);
        }
        i++;
        entry++;
    } while (i < 0x180);
}

/*
 * After model e's data moved (its handle compacted): the new address, each part's DMA
 * chain start, and the NEXT tags inside the chains relocated.
 */
void Model_Relocate(ModelEntry *entry)
{
    u8 *old = entry->dataAddr;
    u8 *base = Mem_GetAddr(entry->data);
    u32 i;
    s32 *ofs;
    u8 *partPkt;
    u8 *tag;
    u32 w;
    u8 *walk;

    entry->dataAddr = base;
    ofs = (s32 *)(base + entry->chainTblOfs);
    partPkt = base + entry->pktOfs;
    for (i = 0; i < entry->partCount; i++, ofs++) {
        tag = base + *ofs;
        *(u8 **)(partPkt + 4) = tag;
        walk = tag;
        partPkt += (*(u16 *)partPkt + 1) * 16;
        do {
            w = *(u32 *)walk;
            switch (w & DMA_TAG_ID_MASK) {
            case DMA_TAG_CNT:
                walk += ((w & 0xFFFF) + 1) * 16;
                break;
            case DMA_TAG_NEXT:
                walk = *(u8 **)(walk + 4) = *(u8 **)(walk + 4) - (s32)old + (s32)base;
                break;
            case DMA_TAG_RET:
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
 * (the descriptor's build) and queues the chains (with a CALL of the program when it changed).
 */
s32 Model_Draw(u32 handle, s32 flags)
{
    ModelEntry *entry = &gModelPool[handle - 1];
    MeshDesc hdr;
    u8 *top;
    u32 *progCall;
    u32 *last;
    u8 *end;
    u8 *pkt;
    u8 *data = entry->dataAddr;

    if (gSysWork.noDraw != 0) {
        return 1;
    }
    if (!(entry->flags & MODEL_NO_COPY)) {
        if (entry->renderFlags != gRenderFlags2) {
            Dma_FlushQueue();
            entry->renderFlags = gRenderFlags2;
            if (entry->data != NULL) {
                Mem_Free(entry->data);
            }
            Model_ReadHeader(entry, &hdr);
            if (Model_Setup(entry, &hdr) == 0) {
                return 0;
            }
            Mem_Compact();
            pkt = entry->dataAddr + entry->pktOfs;
        } else if (entry->flags & MODEL_PREBUILT) {
            pkt = data + entry->pktOfs;
        } else {
            pkt = Packet_Alloc(entry->pktSize, 0x10);
            Model_BuildPartPackets(entry, entry->partCount, entry->progInfo->pktKind, (s32)pkt, entry->mat);
            Model_CollectTextures(entry, 0);
        }
        if (entry->texCount != 0) {
            Tex_MakeResident(entry->texCount, (u32 *)entry->tex);
            Model_SetTexRegs(entry->partCount, entry->progInfo->pktKind, pkt, entry->partTex);
        }
    } else if (entry->flags & MODEL_PREBUILT) {
        pkt = Packet_Alloc(entry->pktSize, 0x10);
        Mem_CopyQwords(data + entry->pktOfs, pkt, (u32)entry->pktSize >> 4);
        if (entry->texCount != 0) {
            Tex_MakeResident(entry->texCount, (u32 *)entry->tex);
            Model_SetTexRegs(entry->partCount, entry->progInfo->pktKind, pkt, entry->partTex);
        }
    } else {
        pkt = Packet_Alloc(entry->pktSize, 0x10);
        Model_CollectTextures(entry, 0);
        if (entry->texCount != 0) {
            Tex_MakeResident(entry->texCount, (u32 *)entry->tex);
        }
        Model_BuildPartPackets(entry, entry->partCount, entry->progInfo->pktKind, (s32)pkt, entry->mat);
    }
    end = pkt + entry->partCount * gModelPktSteps[entry->progInfo->pktKind];
    gSysWork.vu1Size = (u32)(entry->progInfo->codeEnd - entry->progInfo->codeStart) >> 3;
    top = entry->progInfo->build(entry, (u32)(entry->progInfo->start - entry->progInfo->codeStart) >> 3, pkt);
    if (gSysWork.vu1Prog != entry->prog) {
        gSysWork.vu1Prog = entry->prog;
        progCall = Packet_Alloc(0x40, 0x10);
        progCall[0] = DMA_TAG_CALL | 1;
        progCall[1] = entry->progInfo->upload;
        *(u64 *)&progCall[2] = 0;
        last = progCall + 8;
        last[0] = DMA_TAG_IRQ | DMA_TAG_END | 1;
        last[1] = 0;
        *(u64 *)&last[2] = 0;
        *(u128 *)&progCall[12] = 0;
        *(u128 *)&progCall[4] = 0;
        DmaQueue_AddChain(0, (u32)progCall & 0x0FFFFFFF, last, &gDmaQueue);
    }
    DmaQueue_AddChain(0, ((u32)top & 0x0FFFFFFF) | 0x50000000, (u32 *)end, &gDmaQueue);
    return 1;
}

/* Frees model h (its handles and pool entry); 0 when h is not a model in use. */
s32 Model_FreeHandle(u32 handle)
{
    ModelEntry *entry = &gModelPool[handle - 1];

    if (handle == 0) {
        return 0;
    }
    if (handle > 0x180) {
        return 0;
    }
    if (entry->used == 0) {
        return 0;
    }
    Dma_FlushQueue();
    entry->used = 0;
    if (entry->file != NULL) {
        Mem_Free(entry->file);
    }
    if (entry->data != NULL) {
        Mem_Free(entry->data);
    }
    Mem_Fill(entry, 0, sizeof(ModelEntry));
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

/* The value of curve at time (*hint: the key found last time, kept by the caller). */
f32 Anim_EvalCurve(f32 time, AnimCurve *curve, s16 *hint)
{
    f32 value;

    switch (curve->type) {
    case ANIM_KEY_F32_LINEAR:
        value = Anim_EvalLinearF(curve, hint, time);
        break;
    case ANIM_KEY_F32_CUBIC:
        value = Anim_EvalCubicF(curve, hint, time);
        break;
    case ANIM_KEY_F32_MIXED:
        value = Anim_EvalMixedF(curve, hint, time);
        break;
    case ANIM_KEY_S16_LINEAR:
        value = Anim_EvalLinearS(curve, hint, time);
        break;
    case ANIM_KEY_S16_CUBIC:
        value = Anim_EvalCubicS(curve, hint, time);
        break;
    case ANIM_KEY_S16_MIXED:
        value = Anim_EvalMixedS(curve, hint, time);
        break;
    }
    return value;
}

/* Format 0x21: linear between f32 keys. */
f32 Anim_EvalLinearF(AnimCurve *curve, s16 *hint, f32 time)
{
    KeyF2 *prevKey = NULL;
    KeyF2 *nextKey = NULL;

    *hint = Anim_FindKeysF2(curve, &prevKey, &nextKey, *hint, time);
    if (nextKey == NULL) {
        return prevKey->v;
    }
    return Anim_LerpKeys(time, prevKey->v, prevKey->t, nextKey->v, nextKey->t);
}

/* Format 0x22: cubic between f32 keys. */
f32 Anim_EvalCubicF(AnimCurve *curve, s16 *hint, f32 time)
{
    KeyF4 *prevKey = NULL;
    KeyF4 *nextKey = NULL;

    *hint = Anim_FindKeysF4(curve, &prevKey, &nextKey, *hint, time);
    if (nextKey == NULL) {
        return prevKey->v;
    }
    return Anim_CubicKeys(time, prevKey->v, prevKey->t, prevKey->out, nextKey->v, nextKey->t, nextKey->in);
}

/* Format 0x23: f32 keys, linear (0x10000) or cubic (0x20000) by the first key of the pair. */
f32 Anim_EvalMixedF(AnimCurve *curve, s16 *hint, f32 time)
{
    KeyFM *prevKey = NULL;
    KeyFM *nextKey = NULL;

    *hint = Anim_FindKeysFM(curve, &prevKey, &nextKey, *hint, time);
    if (nextKey == NULL) {
        return prevKey->v;
    }
    switch (prevKey->mode) {
    case ANIM_KEY_MODE_LINEAR:
        return Anim_LerpKeys(time, prevKey->v, prevKey->t, nextKey->v, nextKey->t);
    case ANIM_KEY_MODE_CUBIC:
        return Anim_CubicKeys(time, prevKey->v, prevKey->t, prevKey->out, nextKey->v, nextKey->t, nextKey->in);
    }
    return 0.0f;
}

/* Format 0x11: linear between s16 keys. */
f32 Anim_EvalLinearS(AnimCurve *curve, s16 *hint, f32 time)
{
    KeyS2 *prevKey = NULL;
    KeyS2 *nextKey = NULL;

    *hint = Anim_FindKeysS2(curve, &prevKey, &nextKey, *hint, time);
    if (nextKey == NULL) {
        return prevKey->v;
    }
    return Anim_LerpKeys(time, prevKey->v, prevKey->t, nextKey->v, nextKey->t);
}

/* Format 0x12: cubic between s16 keys. */
f32 Anim_EvalCubicS(AnimCurve *curve, s16 *hint, f32 time)
{
    KeyS4 *prevKey = NULL;
    KeyS4 *nextKey = NULL;

    *hint = Anim_FindKeysS4(curve, &prevKey, &nextKey, *hint, time);
    if (nextKey == NULL) {
        return prevKey->v;
    }
    return Anim_CubicKeys(time, prevKey->v, prevKey->t, prevKey->out, nextKey->v, nextKey->t, nextKey->in);
}

/* Format 0x13: s16 keys, linear or cubic by the first key of the pair. */
f32 Anim_EvalMixedS(AnimCurve *curve, s16 *hint, f32 time)
{
    KeySM *prevKey = NULL;
    KeySM *nextKey = NULL;

    *hint = Anim_FindKeysSM(curve, &prevKey, &nextKey, *hint, time);
    if (nextKey == NULL) {
        return prevKey->v;
    }
    switch (prevKey->mode) {
    case ANIM_KEY_MODE_LINEAR:
        return Anim_LerpKeys(time, prevKey->v, prevKey->t, nextKey->v, nextKey->t);
    case ANIM_KEY_MODE_CUBIC:
        return Anim_CubicKeys(time, prevKey->v, prevKey->t, prevKey->out, nextKey->v, nextKey->t, nextKey->in);
    }
    return 0.0f;
}

/*
 * The keys of curve around time, searched from key hint: *prevKeyOut and *nextKeyOut = the
 * keys before and after time (*nextKeyOut NULL at a key, before the first or after the
 * last); returns *prevKeyOut's index.
 * The five below do the same for the other key formats.
 */
s16 Anim_FindKeysF2(AnimCurve *curve, KeyF2 **prevKeyOut, KeyF2 **nextKeyOut, s16 hint, f32 time)
{
    KeyF2 *key = (KeyF2 *)(curve->keys + gAnimCurveKeys);

    *prevKeyOut = NULL;
    *nextKeyOut = NULL;
    if (time <= key[0].t || curve->count == 1) {
        *prevKeyOut = key;
        *nextKeyOut = NULL;
        return 0;
    }
    if (!(time < key[curve->count - 1].t)) {
        *prevKeyOut = &key[curve->count - 1];
        *nextKeyOut = NULL;
        return curve->count - 1;
    }
    if (hint < 0) {
        hint = 0;
    } else if (hint >= curve->count) {
        hint = curve->count - 1;
    }
    key += hint;
    for (;;) {
        if (key->t == time) {
            *prevKeyOut = key;
            *nextKeyOut = NULL;
            return hint;
        }
        if (key->t < time && time < key[1].t) {
            *prevKeyOut = key;
            *nextKeyOut = key + 1;
            return hint;
        }
        if (time < key->t) {
            key--;
            hint--;
        } else {
            key++;
            hint++;
        }
    }
}

/* Anim_FindKeysF2 for KeyF4 keys. */
s16 Anim_FindKeysF4(AnimCurve *curve, KeyF4 **prevKeyOut, KeyF4 **nextKeyOut, s16 hint, f32 time)
{
    KeyF4 *key = (KeyF4 *)(curve->keys + gAnimCurveKeys);

    *prevKeyOut = NULL;
    *nextKeyOut = NULL;
    if (time <= key[0].t || curve->count == 1) {
        *prevKeyOut = key;
        *nextKeyOut = NULL;
        return 0;
    }
    if (!(time < key[curve->count - 1].t)) {
        *prevKeyOut = &key[curve->count - 1];
        *nextKeyOut = NULL;
        return curve->count - 1;
    }
    if (hint < 0) {
        hint = 0;
    } else if (hint >= curve->count) {
        hint = curve->count - 1;
    }
    key += hint;
    for (;;) {
        if (key->t == time) {
            *prevKeyOut = key;
            *nextKeyOut = NULL;
            return hint;
        }
        if (key->t < time && time < key[1].t) {
            *prevKeyOut = key;
            *nextKeyOut = key + 1;
            return hint;
        }
        if (time < key->t) {
            key--;
            hint--;
        } else {
            key++;
            hint++;
        }
    }
}

/* Anim_FindKeysF2 for KeyFM keys. */
s16 Anim_FindKeysFM(AnimCurve *curve, KeyFM **prevKeyOut, KeyFM **nextKeyOut, s16 hint, f32 time)
{
    KeyFM *key = (KeyFM *)(curve->keys + gAnimCurveKeys);

    *prevKeyOut = NULL;
    *nextKeyOut = NULL;
    if (time <= key[0].t || curve->count == 1) {
        *prevKeyOut = key;
        *nextKeyOut = NULL;
        return 0;
    }
    if (!(time < key[curve->count - 1].t)) {
        *prevKeyOut = &key[curve->count - 1];
        *nextKeyOut = NULL;
        return curve->count - 1;
    }
    if (hint < 0) {
        hint = 0;
    } else if (hint >= curve->count) {
        hint = curve->count - 1;
    }
    key += hint;
    for (;;) {
        if (key->t == time) {
            *prevKeyOut = key;
            *nextKeyOut = NULL;
            return hint;
        }
        if (key->t < time && time < key[1].t) {
            *prevKeyOut = key;
            *nextKeyOut = key + 1;
            return hint;
        }
        if (time < key->t) {
            key--;
            hint--;
        } else {
            key++;
            hint++;
        }
    }
}

/* Anim_FindKeysF2 for KeyS2 keys. */
s16 Anim_FindKeysS2(AnimCurve *curve, KeyS2 **prevKeyOut, KeyS2 **nextKeyOut, s16 hint, f32 time)
{
    KeyS2 *key = (KeyS2 *)(curve->keys + gAnimCurveKeys);

    *prevKeyOut = NULL;
    *nextKeyOut = NULL;
    if (time <= key[0].t || curve->count == 1) {
        *prevKeyOut = key;
        *nextKeyOut = NULL;
        return 0;
    }
    if (!(time < key[curve->count - 1].t)) {
        *prevKeyOut = &key[curve->count - 1];
        *nextKeyOut = NULL;
        return curve->count - 1;
    }
    if (hint < 0) {
        hint = 0;
    } else if (hint >= curve->count) {
        hint = curve->count - 1;
    }
    key += hint;
    for (;;) {
        if (key->t == time) {
            *prevKeyOut = key;
            *nextKeyOut = NULL;
            return hint;
        }
        if (key->t < time && time < key[1].t) {
            *prevKeyOut = key;
            *nextKeyOut = key + 1;
            return hint;
        }
        if (time < key->t) {
            key--;
            hint--;
        } else {
            key++;
            hint++;
        }
    }
}

/* Anim_FindKeysF2 for KeyS4 keys. */
s16 Anim_FindKeysS4(AnimCurve *curve, KeyS4 **prevKeyOut, KeyS4 **nextKeyOut, s16 hint, f32 time)
{
    KeyS4 *key = (KeyS4 *)(curve->keys + gAnimCurveKeys);

    *prevKeyOut = NULL;
    *nextKeyOut = NULL;
    if (time <= key[0].t || curve->count == 1) {
        *prevKeyOut = key;
        *nextKeyOut = NULL;
        return 0;
    }
    if (!(time < key[curve->count - 1].t)) {
        *prevKeyOut = &key[curve->count - 1];
        *nextKeyOut = NULL;
        return curve->count - 1;
    }
    if (hint < 0) {
        hint = 0;
    } else if (hint >= curve->count) {
        hint = curve->count - 1;
    }
    key += hint;
    for (;;) {
        if (key->t == time) {
            *prevKeyOut = key;
            *nextKeyOut = NULL;
            return hint;
        }
        if (key->t < time && time < key[1].t) {
            *prevKeyOut = key;
            *nextKeyOut = key + 1;
            return hint;
        }
        if (time < key->t) {
            key--;
            hint--;
        } else {
            key++;
            hint++;
        }
    }
}

/* Anim_FindKeysF2 for KeySM keys. */
s16 Anim_FindKeysSM(AnimCurve *curve, KeySM **prevKeyOut, KeySM **nextKeyOut, s16 hint, f32 time)
{
    KeySM *key = (KeySM *)(curve->keys + gAnimCurveKeys);

    *prevKeyOut = NULL;
    *nextKeyOut = NULL;
    if (time <= key[0].t || curve->count == 1) {
        *prevKeyOut = key;
        *nextKeyOut = NULL;
        return 0;
    }
    if (!(time < key[curve->count - 1].t)) {
        *prevKeyOut = &key[curve->count - 1];
        *nextKeyOut = NULL;
        return curve->count - 1;
    }
    if (hint < 0) {
        hint = 0;
    } else if (hint >= curve->count) {
        hint = curve->count - 1;
    }
    key += hint;
    for (;;) {
        if (key->t == time) {
            *prevKeyOut = key;
            *nextKeyOut = NULL;
            return hint;
        }
        if (key->t < time && time < key[1].t) {
            *prevKeyOut = key;
            *nextKeyOut = key + 1;
            return hint;
        }
        if (time < key->t) {
            key--;
            hint--;
        } else {
            key++;
            hint++;
        }
    }
}
/* The value at time on the line through (prevTime, prevValue) and (nextTime, nextValue). */
asm f32 Anim_LerpKeys(f32 time, f32 prevValue, f32 prevTime, f32 nextValue, f32 nextTime)
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
/* The value at time on the cubic between two keys (their values and times, the first key's
 * out slope and the second key's in slope). */
asm f32 Anim_CubicKeys(f32 time, f32 prevValue, f32 prevTime, f32 prevOutSlope, f32 nextValue, f32 nextTime, f32 nextInSlope)
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
static asm void Math_SinCos(f32 *out, f32 angle)
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

extern MtxSlot gMtxSlots[];

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

void Mtx_Perspective(f32 *m, f32 farZ, f32 nearZ, f32 fov, f32 aspect);
void Camera_StoreProjection(f32 *m);
void Camera_SetViewport(f32 *m, s32 x, s32 y, s32 w, s32 h, f32 n, f32 f);
void Camera_SetViewportMtx(s32 x, s32 y, s32 w, s32 h, f32 n, f32 f);
void Mtx_Copy(f32 *dst, f32 *src);
s32 Gs_SetScissor(s32 x, s32 y, s32 w, s32 h, s32 ctx);
void Math_GetSinCos(f32 *s, f32 *c, f32 a);
asm void Mtx_Identity(f32 *m);
asm void Mtx_Mul(f32 *dst, f32 *a, f32 *b);

/* Matrix slot `slot` = a perspective projection (Mtx_Perspective, Camera_StoreProjection), then
 * the camera matrices. */
void Camera_SetPerspective(s32 slot, f32 farZ, f32 nearZ, f32 fov, f32 aspect)
{
    f32 *mtx = gMtxSlots[slot].m[0];

    Mtx_Perspective(mtx, farZ, nearZ, fov, aspect);
    Camera_StoreProjection(mtx);
    Mtx_Mul(gMtxProjClip, gMtxCamProj, gMtxClip);
    Mtx_Mul(gMtxProjScreen, gMtxCamProj, gMtxScreen);
}

/* mtx = a perspective projection: depth nearZ to farZ, vertical field of view fov (radians),
 * aspect ratio aspect. The far depth comes first: the callers pass Camera.farZ there, and
 * the depth terms are only right that way round. */
void Mtx_Perspective(f32 *mtx, f32 farZ, f32 nearZ, f32 fov, f32 aspect)
{
    f32 cosHalf;
    f32 sinHalf;

    fov *= 0.5f;
    Math_GetSinCos(&sinHalf, &cosHalf, fov);
    Mtx_Identity(mtx);
    mtx[0] = cosHalf / sinHalf / aspect;
    mtx[5] = cosHalf / sinHalf;
    mtx[10] = -(farZ + nearZ) / (farZ - nearZ);
    mtx[11] = -1.0f;
    mtx[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
    mtx[15] = 0.0f;
}

/* gViewportMtx = proj (a projection) with its x and y scale fitted to the viewport.
 * This function and the two viewport ones below read main's render-state globals by
 * name: as raw addresses, MWCC can't move their loads above the matrix stores. */
void Camera_StoreProjection(f32 *proj)
{
    Mtx_Copy(gViewportMtx, proj);
    gViewportMtx[0] = 2.0f * gViewportMtx[0] / (gViewClipW / (0.5f * gViewW));
    gViewportMtx[5] = 2.0f * gViewportMtx[5] / (gViewClipH / (0.5f * gViewH));
}

/* Matrix slot `slot` = an orthographic screen matrix for the viewport (x, y, width, height),
 * then the camera matrices and the GS scissor (y and height halved while gSysFrameMode is
 * 0: a field, half the lines). */
void Camera_SetOrtho(s32 slot, s32 x, s32 y, s32 width, s32 height, f32 nearZ, f32 farZ)
{
    Camera_SetViewport(gMtxSlots[slot].m[0], x, y, width, height, nearZ, farZ);
    Camera_StoreProjection(gMtxCamProj);
    Mtx_Mul(gMtxProjClip, gMtxCamProj, gMtxClip);
    Mtx_Mul(gMtxProjScreen, gMtxCamProj, gMtxScreen);
    if (gSysFrameMode == 0) {
        y /= 2;
        height /= 2;
    }
    Gs_SetScissor(x, y, width, height, 2);
}

/*
 * The viewport (x, y, width, height; depth nearZ to farZ) into gMtxScreen and the screen
 * matrix `screen` (centred on the GS's 2048, 2048), and the viewport globals of main;
 * then Camera_SetViewportMtx.
 */
void Camera_SetViewport(f32 *screen, s32 x, s32 y, s32 width, s32 height, f32 nearZ, f32 farZ)
{
    f32 halfW;
    f32 halfH;
    f32 depth;

    halfW = width / 2;
    halfH = height / 2;
    Mtx_Identity(gMtxScreen);
    gMtxScreen[0] = halfW;
    gMtxScreen[5] = -halfH;
    depth = farZ - nearZ;
    gMtxScreen[10] = depth;
    gMtxScreen[12] = x + halfW;
    gMtxScreen[13] = y + halfH;
    gMtxScreen[14] = nearZ;
    gMtxScreen[15] = 1.0f;
    if (gSysFrameMode == 0) {
        y /= 2;
        height /= 2;
    }
    halfW = width / 2;
    halfH = height / 2;
    Mtx_Identity(screen);
    screen[0] = halfW;
    screen[5] = -halfH;
    screen[10] = depth;
    screen[12] = x + halfW;
    screen[13] = y + halfH;
    screen[15] = 1.0f;
    screen[10] *= 0.5f * -gSysZMax;
    screen[12] += 2048 - (gDispWidth >> 1);
    screen[13] += 2048 - (gDispHeight >> 1);
    screen[14] = 0.5f * gSysZMax;
    gViewCenterX = screen[12];
    gViewCenterY = screen[13];
    gViewX = x;
    gViewY = y;
    gViewW = width;
    gViewH = height;
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
    Camera_SetViewportMtx(0, 0, gViewClipW, gViewClipH, nearZ, farZ);
}

/* gMtxProj = the viewport matrix for a width x height screen (depth nearZ to farZ). */
void Camera_SetViewportMtx(s32 x, s32 y, s32 width, s32 height, f32 nearZ, f32 farZ)
{
    f32 halfW;
    f32 halfH;

    Mtx_Identity(gMtxProj);
    halfW = width / 2;
    halfH = height / 2;
    gMtxProj[0] = halfW;
    gMtxProj[5] = -halfH;
    gMtxProj[15] = 1.0f;
    gMtxProj[10] = farZ - nearZ;
    gMtxProj[10] *= 0.5f * -gSysZMax;
    gMtxProj[14] = 0.5f * gSysZMax;
    gMtxProj[12] = gViewCenterX;
    gMtxProj[13] = gViewCenterY;
}
extern u8 gMtxIdentity[];
/* mtx = the identity matrix (copied from gMtxIdentity). */
asm void Mtx_Identity(f32 *mtx)
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
/* The 3x3 part of mtx becomes the identity; the translation stays. */
asm void Mtx_ClearRotation(f32 *mtx)
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
/* mtx = a scale matrix. */
asm void Mtx_Scale(f32 *mtx, f32 x, f32 y, f32 z)
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
/* The 3x3 part of mtx becomes a scale; the translation stays. */
asm void Mtx_SetScale(f32 *mtx, f32 x, f32 y, f32 z)
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
/* mtx = a translation matrix. */
asm void Mtx_Translation(f32 *mtx, f32 x, f32 y, f32 z)
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

/* The 3x3 part of mtx = mtx * scale(x, y, z). */
void Mtx_ScaleBy(f32 *mtx, f32 x, f32 y, f32 z)
{
    f32 scale[16];

    Mtx_SetScale(scale, x, y, z);
    Mtx_Concat3x3(mtx, scale);
}

/* mtx's translation = (x, y, z). */
void Mtx_SetTrans(f32 *mtx, f32 x, f32 y, f32 z)
{
    mtx[12] = x;
    mtx[13] = y;
    mtx[14] = z;
}

/* mtx's translation += offset. */
void Mtx_AddTrans(f32 *mtx, f32 *offset)
{
    mtx[12] = (mtx[12] + offset[0]);
    mtx[13] = (mtx[13] + offset[1]);
    mtx[14] = (mtx[14] + offset[2]);
}

/* out = mtx's translation. */
void Mtx_GetTrans(f32 *out, f32 *mtx)
{
    out[0] = mtx[12];
    out[1] = mtx[13];
    out[2] = mtx[14];
}
/* mtx = mtx * (rotation by angle about x). */
asm void Mtx_RotateX(f32 *mtx, f32 angle)
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
/* mtx = mtx * (rotation by angle about y). */
asm void Mtx_RotateY(f32 *mtx, f32 angle)
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
/* mtx = mtx * (rotation by angle about z). */
asm void Mtx_RotateZ(f32 *mtx, f32 angle)
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
/* The rotation part of mtx = rotations about x, then y, then z. */
asm void Mtx_SetRotXYZ(f32 *mtx, f32 x, f32 y, f32 z)
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
/* The rotation part of mtx = rotations about z, then y, then x. */
asm void Mtx_SetRotZYX(f32 *mtx, f32 x, f32 y, f32 z)
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
/* mtx rotated about x, then y, then z. */
asm void Mtx_RotateXYZ(f32 *mtx, f32 x, f32 y, f32 z)
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
/* mtx rotated about x, then z, then y. */
asm void Mtx_RotateXZY(f32 *mtx, f32 x, f32 y, f32 z)
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
/* mtx rotated about y, then x, then z. */
asm void Mtx_RotateYXZ(f32 *mtx, f32 x, f32 y, f32 z)
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
/* mtx rotated about z, then x, then y. */
asm void Mtx_RotateZXY(f32 *mtx, f32 x, f32 y, f32 z)
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

/* mtx = the view matrix of a camera at eye looking at target, with up as the up direction. */
void Mtx_LookAt(f32 *mtx, f32 *eye, f32 *target, f32 *up)
{
    f32 side[3];
    f32 upAxis[3];
    f32 back[3];
    Vec3Copy upDir;

    back[0] = eye[0] - target[0];
    back[1] = eye[1] - target[1];
    back[2] = eye[2] - target[2];
    upDir = *(Vec3Copy *)up;
    Vec_Normalize(back);
    Vec_Normalize((f32 *)&upDir);
    Vec_Cross(side, (f32 *)&upDir, back);
    Vec_Normalize(side);
    Vec_Cross(upAxis, back, side);
    mtx[3] = 0.0f;
    mtx[7] = 0.0f;
    mtx[11] = 0.0f;
    mtx[15] = 1.0f;
    mtx[0] = side[0];
    mtx[4] = side[1];
    mtx[8] = side[2];
    mtx[1] = upAxis[0];
    mtx[5] = upAxis[1];
    mtx[9] = upAxis[2];
    mtx[2] = back[0];
    mtx[6] = back[1];
    mtx[10] = back[2];
    mtx[12] = -Vec_Dot(eye, side);
    mtx[13] = -Vec_Dot(eye, upAxis);
    mtx[14] = -Vec_Dot(eye, back);
}
/* Transposes the 3x3 part of mtx. */
asm void Mtx_Transpose3x3(f32 *mtx)
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
/* dst = lhs * rhs (4x4, row vectors). */
asm void Mtx_Mul(f32 *dst, f32 *lhs, f32 *rhs)
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
/* mtx = mtx * other (4x4). */
asm void Mtx_Concat(f32 *mtx, f32 *other)
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
/* The 3x3 part of dst = lhs * rhs (3x3); the rest of dst stays. */
asm void Mtx_Mul3x3(f32 *dst, f32 *lhs, f32 *rhs)
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
/* The 3x3 part of mtx = mtx * other (3x3). */
asm void Mtx_Concat3x3(f32 *mtx, f32 *other)
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

/* The 3x3 part of mtx = src's. */
void Mtx_CopyRot(f32 *mtx, f32 *src)
{
    mtx[0] = src[0];
    mtx[1] = src[1];
    mtx[2] = src[2];
    mtx[4] = src[4];
    mtx[5] = src[5];
    mtx[6] = src[6];
    mtx[8] = src[8];
    mtx[9] = src[9];
    mtx[10] = src[10];
}
/* dst = first * weightFirst + second * weightSecond, the rotation made orthonormal again
 * (rows 0 and 1, then their cross products). */
asm void Mtx_Blend(f32 *dst, f32 *first, f32 *second, f32 weightFirst, f32 weightSecond)
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
/* out = vec * (the 3x3 part of mtx). */
asm void Vec_Rotate(f32 *out, f32 *vec, f32 *mtx)
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
/* vec = vec * (the 3x3 part of mtx). */
asm void Vec_RotateSelf(f32 *vec, f32 *mtx)
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
/* out = vec * mtx (4 components). */
asm void Vec4_Transform(f32 *out, f32 *vec, f32 *mtx)
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
/* vec = vec * mtx (4 components). */
asm void Vec4_TransformSelf(f32 *vec, f32 *mtx)
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
/* out = (vec, 1) * mtx divided by its w. */
asm void Vec_Project(f32 *out, f32 *vec, f32 *mtx)
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

/* vec rotated by angle about the x axis. */
void Vec_RotateX(f32 *vec, f32 angle)
{
    f32 sinA;
    f32 cosA;
    Vec3Copy orig;

    orig = *(Vec3Copy *)vec;
    Math_GetSinCos(&sinA, &cosA, angle);
    vec[1] = cosA * orig.y - sinA * orig.z;
    vec[2] = sinA * orig.y + cosA * orig.z;
}

/* vec rotated by angle about the y axis. */
void Vec_RotateY(f32 *vec, f32 angle)
{
    f32 sinA;
    f32 cosA;
    Vec3Copy orig;

    orig = *(Vec3Copy *)vec;
    Math_GetSinCos(&sinA, &cosA, angle);
    vec[0] = cosA * orig.x + sinA * orig.z;
    vec[2] = -sinA * orig.x + cosA * orig.z;
}

/* vec rotated by angle about the z axis. */
void Vec_RotateZ(f32 *vec, f32 angle)
{
    f32 sinA;
    f32 cosA;
    Vec3Copy orig;

    orig = *(Vec3Copy *)vec;
    Math_GetSinCos(&sinA, &cosA, angle);
    vec[0] = cosA * orig.x - sinA * orig.y;
    vec[1] = sinA * orig.x + cosA * orig.y;
}
/* |vec|. */
asm f32 Vec_Length(f32 *vec)
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
/* |from - to|. */
asm f32 Vec_Distance(f32 *from, f32 *to)
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
/* vec = vec / |vec|. */
asm void Vec_Normalize(f32 *vec)
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
/* lhs . rhs (3 components). */
asm f32 Vec_Dot(f32 *lhs, f32 *rhs)
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
/* out = lhs x rhs. */
asm void Vec_Cross(f32 *out, f32 *lhs, f32 *rhs)
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
    f32 rad = 3.1415927f * (0.0054931641f * angle) / 180.0f;

    if (rad > 3.1415927f) {
        rad += -6.2831855f;
    }
    return rad;
}

/* mtx into slot `slot` of main's matrix table gMtxSlots. */
void MtxSlot_Store(s32 slot, MtxSlot *mtx)
{
    gMtxSlots[slot] = *mtx;
}

/* Slot `slot` of main's matrix table gMtxSlots into mtx. */
void MtxSlot_Load(MtxSlot *mtx, s32 slot)
{
    *mtx = gMtxSlots[slot];
}

extern f32 gMtxView[];
extern f32 gMtxProjScreen[];
void Mtx_Concat(f32 *m, f32 *a);
void Vec_Project(f32 *out, f32 *v, f32 *m);

/* out = point projected by gMtxSlots[0] * gMtxView * gMtxProjScreen. */
void Camera_ProjectPoint(f32 *out, f32 *point)
{
    f32 mtx[16];

    Mtx_Mul(mtx, (f32 *)gMtxSlots, gMtxView);
    Mtx_Concat(mtx, gMtxProjScreen);
    Vec_Project(out, point, mtx);
}

/* |x|. */
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

/* *sinOut = sin angle, *cosOut = cos angle. */
void Math_GetSinCos(f32 *sinOut, f32 *cosOut, f32 angle)
{
    f32 sinCos[4];

    Math_SinCos(sinCos, angle);
    *sinOut = sinCos[0];
    *cosOut = sinCos[1];
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

/* The rotation matrix of quaternion quat (x, y, z, w), scaled by 2 / |quat|^2 (0 for a
 * zero quat). */
void Quat_ToMatrix(f32 *quat, f32 (*mtx)[4])
{
    f32 normSq = quat[0] * quat[0] + quat[1] * quat[1] + quat[2] * quat[2] + quat[3] * quat[3];
    f32 scale;
    f32 xx, yy, zz, xy, xz, yz, wx, wy, wz;

    scale = (normSq > 0.0f) ? 2.0f / normSq : 0.0f;
    xx = scale * (quat[0] * quat[0]);
    xy = scale * (quat[0] * quat[1]);
    xz = scale * (quat[0] * quat[2]);
    wx = scale * (quat[3] * quat[0]);
    yy = scale * (quat[1] * quat[1]);
    yz = scale * (quat[1] * quat[2]);
    wy = scale * (quat[3] * quat[1]);
    wz = scale * (quat[3] * quat[2]);
    zz = scale * (quat[2] * quat[2]);
    mtx[0][0] = 1.0f - (yy + zz);
    mtx[0][1] = xy + wz;
    mtx[0][2] = xz - wy;
    mtx[1][0] = xy - wz;
    mtx[1][1] = 1.0f - (xx + zz);
    mtx[1][2] = yz + wx;
    mtx[2][0] = xz + wy;
    mtx[2][1] = yz - wx;
    mtx[2][2] = 1.0f - (xx + yy);
    mtx[0][3] = mtx[1][3] = mtx[2][3] = 0.0f;
    mtx[3][0] = mtx[3][1] = mtx[3][2] = 0.0f;
    mtx[3][3] = 1.0f;
}

/* quat (its axis in quat[0..2]) = the rotation by angle about that axis. */
void Quat_FromAxisAngle(f32 *quat, f32 angle)
{
    f32 len;
    f32 halfSin;

    angle *= 0.5f;
    len = Math_Sqrtf(quat[0] * quat[0] + quat[1] * quat[1] + quat[2] * quat[2]);
    quat[3] = 0.0f;
    quat[0] /= len;
    quat[1] /= len;
    quat[2] /= len;
    halfSin = Math_Sin(angle);
    quat[3] = Math_Cos(angle);
    quat[0] *= halfSin;
    quat[1] *= halfSin;
    quat[2] *= halfSin;
}

/* quat = the rotation by angle about axis. */
void Quat_Set(f32 *axis, f32 *quat, f32 angle)
{
    quat[3] = 0.0f;
    quat[0] = axis[0];
    quat[1] = axis[1];
    quat[2] = axis[2];
    Quat_FromAxisAngle(quat, angle);
}

/*
 * Whether the sphere at center of the given radius is in the view volume `planes` (four
 * side planes through the eye at planes[0], [4], [8], [12]; the depth range at planes[16],
 * [17]); out = center in view space (mtx, the view matrix).
 */
s32 Frustum_TestSphere(f32 *center, f32 *out, f32 *mtx, f32 *planes, f32 radius)
{
    out[2] = mtx[2] * center[0] + mtx[6] * center[1] + mtx[10] * center[2] + mtx[14];
    if (out[2] - radius > planes[16]) {
        return 0;
    }
    if (out[2] + radius < planes[17]) {
        return 0;
    }
    out[0] = mtx[0] * center[0] + mtx[4] * center[1] + mtx[8] * center[2] + mtx[12];
    if (out[0] * planes[0] + out[2] * planes[2] > radius) {
        return 0;
    }
    if (out[0] * planes[4] + out[2] * planes[6] > radius) {
        return 0;
    }
    out[1] = mtx[1] * center[0] + mtx[5] * center[1] + mtx[9] * center[2] + mtx[13];
    if (out[1] * planes[9] + out[2] * planes[10] > radius) {
        return 0;
    }
    if (out[1] * planes[13] + out[2] * planes[14] > radius) {
        return 0;
    }
    return 1;
}

/* Makes the animation data in memory handle `handle` current (its keys too). */
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
    void *handle = Mem_Alloc(*(s32 *)(src + 0xC), 4);
    u8 *dst = Mem_GetAddr(handle);
    u32 i;
    u8 *from = src;

    for (i = 0; i < *(u32 *)(src + 0xC); i++) {
        *dst++ = *from++;
    }
    return handle;
}

/* A copy of a converted animation set (set->base, whose first object is the set itself,
 * with the block's size; copied by words) in a new memory handle, which it returns. The
 * set is read as bytes, like Skeleton_CopyData's. */
void *AnimSet_CopyToHandle(u8 *set)
{
    u8 *src = *(u8 **)(set + 0x14);
    void *handle = Mem_Alloc(*(s32 *)(src + 0x18), 4);
    u32 *dst = Mem_GetAddr(handle);
    u32 i;
    u32 *from = (u32 *)src;

    for (i = 0; i < *(u32 *)(src + 0x18) / 4; i++) {
        *dst++ = *from++;
    }
    return handle;
}

/* Frees a memory handle; returns 1. Mem_FreeRet1 is the same. */
s32 Model_FreeBlock(void *block)
{
    Mem_Free(block);
    return 1;
}

/* Frees a memory handle (a motion of gMotions); returns 1. */
s32 Mem_FreeRet1(void *block)
{
    Mem_Free(block);
    return 1;
}

/* This file's initialised data. */
extern char gMonthNames_0[];
extern char gMonthNames_1[];
extern char gMonthNames_2[];
extern char gMonthNames_3[];
extern char gMonthNames_4[];
extern char gMonthNames_5[];
extern char gMonthNames_6[];
extern char gMonthNames_7[];
extern char gMonthNames_8[];
extern char gMonthNames_9[];
extern char gMonthNames_10[];
extern char gMonthNames_11[];
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
char gBuildDate[0x10] = {
#include "sub_main/00402220.inc"
};
char gBuildTime[0x10] = {
#include "sub_main/00402230.inc"
};
char *gMonthNames[12] = {
    gMonthNames_0, gMonthNames_1, gMonthNames_2, gMonthNames_3,
    gMonthNames_4, gMonthNames_5, gMonthNames_6, gMonthNames_7,
    gMonthNames_8, gMonthNames_9, gMonthNames_10, gMonthNames_11,
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
char gSjisDigits[0x20] = {
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
