/*
 * The start of sub_main: its entry and main loop (sub_main_entry,
 * SubMain_Step, SubMain_BeginFrame, SubMain_EndFrame), the system
 * settings, the GS colour table, the pads' frame state (pad.h), the memory
 * layout (ram_areas.h), the version string and the first loading screen.
 * The pads are read here once a frame, by the main loop; their state
 * belongs to the frame, so they share the file.
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
#include "card.h"
#include "kbd.h"
#include "net_tcp.h"
#include "quest.h"
#include "stream.h"
#include "sys_main.h"
#include "chat_phrases.h"
#include "display.h"
#include "display_util.h"
#include "file_load.h"
#include "font.h"
#include "frame.h"
#include "game_util.h"
#include "loading_screen.h"
#include "main.h"
#include "mem_util.h"
#include "net_devices.h"
#include "packet_mode.h"
#include "rand.h"
#include "screen_fade.h"
#include "usb_kbd.h"
#include "bgm.h"
#include "tex_files.h"

/* The game compiled this unit at optimization level 4 (Vec_RotateX needs it;
 * every other matched function matches either way). */
#pragma optimization_level 4

extern s32 gFlipCountdown; /* main */
void func_6B6700(void); /* select overlay: Option_SetDefaults (select.txt) */
void * memset(void *p, s32 c, u32 n);
extern s32 gSubMainStep;
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
extern s32 gAnimData;
/* end of prototypes */

/* This file's read-only data. */
#pragma rodata on
char gMonthNames_0[8] = {
#include "sub_main/0046A100.inc"
};
char gMonthNames_1[8] = {
#include "sub_main/0046A108.inc"
};
char gMonthNames_2[8] = {
#include "sub_main/0046A110.inc"
};
char gMonthNames_3[8] = {
#include "sub_main/0046A118.inc"
};
char gMonthNames_4[8] = {
#include "sub_main/0046A120.inc"
};
char gMonthNames_5[8] = {
#include "sub_main/0046A128.inc"
};
char gMonthNames_6[8] = {
#include "sub_main/0046A130.inc"
};
char gMonthNames_7[8] = {
#include "sub_main/0046A138.inc"
};
char gMonthNames_8[8] = {
#include "sub_main/0046A140.inc"
};
char gMonthNames_9[8] = {
#include "sub_main/0046A148.inc"
};
char gMonthNames_10[8] = {
#include "sub_main/0046A150.inc"
};
char gMonthNames_11[8] = {
#include "sub_main/0046A158.inc"
};
char gVerFmtMonth[8] = {
#include "sub_main/0046A160.inc"
};
char gVerFmtDay1[8] = {
#include "sub_main/0046A168.inc"
};
char gVerFmtField[8] = {
#include "sub_main/0046A170.inc"
};
char gVerFmtVersion[12] = {
#include "sub_main/0046A178.inc"
};
char gVerRule[32] = {
#include "sub_main/0046A190.inc"
};
char gVerTitle[24] = {
#include "sub_main/0046A1B0.inc"
};
char gVerFmtChar[8] = {
#include "sub_main/0046A1C8.inc"
};
char gVerLineEnd[16] = {
#include "sub_main/0046A1D0.inc"
};
#pragma rodata off

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
    memset(&gShortMailMsg, 0, 146U);
    memset(&gMailBox, 0, 1168U);
    memset(&gQuestFiles, 0, 2304U);
    memset(&gChatBlockList, 0, 480U);
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
    if (sys->brightness < 56 || sys->brightness > 164) {
        sys->brightness = 128;
        gSysSave.options.brightness = 128;
    }
    sys->unk24 = 0;
    sys->unk14 = 1;
    sys->unk06 = 0;
    sys->cameraPreset = 255;
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

/* The background colour (RS_BG_COLOR) = colour i of the RGB table gGsColorTable, alpha 255. */
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

/* Reads both controller ports, then updates their PadStates. */
void Pad_UpdatePorts(void)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        Pad_UpdatePort(i);
    }
    PadState_UpdateAll();
}

/* A stick's tilt with the dead zone applied: below 45 it counts as 0. */
u16 Pad_StickDeadZone(u32 x)
{
    if (x < 45) {
        x = 0;
    }
    return x;
}

/* One pad's buttons in the game's layout (gPadState[i], 34 bytes). */
extern PadState gPadState[];

/* Updates gPadState[i] from pad i as read: the buttons in the game's bit order, the stick
 * directions, angles (ANGLE_TURN to a turn) and magnitudes, the press/release edges and the
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

extern s32 gPadDebugSwitch;
extern s32 gPadDebugOn;
/* Updates the four slots (PadState_Update); then, with gPadDebugSwitch set, a debug switch on
 * the second pad: START toggles it, and while it is on, gPadDebugMode is 0 on CIRCLE, cycles
 * 0-3 while CROSS is held, and is 32768 otherwise. */
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
            gPadDebugMode = 32768;
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

extern u32 gTexHandles[];
extern s32 gDrawFrameDepth;
extern s16 gTexPrepLast;
extern s16 gTexPrepLastCount;
extern s16 gTexPrepFrameCount;

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
    char date[40];
    char time[32];
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
    Tex_LoadFile(5, 341, 0);
    Loading_Open((u32) gSaveBuf, 1);
    Sys_FlipFrame(0);
    Sys_FlipFrame(0);
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
char gBuildDate[16] = {
#include "sub_main/00402220.inc"
};
char gBuildTime[16] = {
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
