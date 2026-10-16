#ifndef SYSSETTINGS_H
#define SYSSETTINGS_H
#include "common.h"

/*
 * 0x6ADF90 (sub_main's .bss, 0xF4 bytes): the system settings and the memory
 * layout, which the boot ELF's core reads too (gSysSettings in main.txt).
 * SysSettings_Reset (unk_2E3940.c) sets the settings, Mem_InitLayout the
 * memory areas (the addresses given are the ones it stores).
 *
 * Portability: sub_main (and the lobby) also reach several fields through
 * scalar symbols at their addresses (gFieldControl = +0x1B, gCameraPreset =
 * +0x2F, gLoadBuf = +0x58, gOnlineFlag = +0x10 ...): member accesses share
 * one base register between fields and separate symbols don't, so each form
 * stays where the game's code needs it. A port must alias them to these fields.
 */
typedef struct SysSettings {
    /* 0x00 */ u8 unk00[2];
    /* 0x02 */ u8 unk02;                /* cleared when the mode select or the character select ends */
    /* 0x03 */ u8 unk03;                /* set to 1 when gm_main, the character select or plsel ends */
    /* 0x04 */ u8 bootFrom;             /* where Boot_Task is entered from (Boot_SetMode): 0 the power on, 1 gm_main's exit, 2 a sub_main task (Task_StartBoot*) */
    /* 0x05 */ u8 unk05;
    /* 0x06 */ u8 unk06;
    /* 0x07 */ u8 unk07[4];
    /* 0x0B */ u8 netMenuShown;         /* 1 once Boot_Task opened the net menu (the first boot) */
    /* 0x0C */ u8 unk0C[0x10 - 0x0C];
    /* 0x10 */ u8 online;               /* the network mode (Game_IsOnline; gm_main's mode select sets it) */
    /* 0x11 */ u8 unk11;
    /* 0x12 */ u8 unk12;                /* NetMenu_Init clears it, 1 once the net menu is done and after a system reset */
    /* 0x13 */ u8 unk13;
    /* 0x14 */ u8 unk14;
    /* 0x15 */ u8 unk15[2];
    /* 0x17 */ u8 soundMode;            /* 0 mono, 1 stereo, 2 surround (option gSysSave) */
    /* 0x18 */ u8 unk18[2];
    /* 0x1A */ u8 unk1A;
    /* 0x1B */ u8 fieldControl;         /* gFieldControl: 1 while the hunter takes pad input in the field */
    /* 0x1C */ u8 unk1C[0x22 - 0x1C];
    /* 0x22 */ u8 unk22;
    /* 0x23 */ u8 unk23;
    /* 0x24 */ u8 unk24;
    /* 0x25 */ u8 unk25[0x2A - 0x25];
    /* 0x2A */ u16 boundTex;            /* the texture last bound (Tex_Bind; 0xFFFF after Gfx_ResetRenderStates) */
    /* 0x2C */ u8 blendA;               /* Gfx_SetBlend's cached first table index (render state 0x5E) */
    /* 0x2D */ u8 blendB;               /* ... and its second */
    /* 0x2E */ u8 state63;              /* Gfx_SetState63's cached index (render state 0x63) */
    /* 0x2F */ u8 cameraPreset;         /* gCameraPreset (0xFF after a reset) */
    /* 0x30 */ u8 blendOp;              /* Gfx_SetBlendOp's cached index (render state 0xD) */
    /* 0x31 */ u8 unk31;                /* Kbd_Draw's argument (unk_363550.c) */
    /* 0x32 */ u8 screenCaptureOk;      /* gScreenCaptureOk */
    /* 0x33 */ u8 clearCapture;         /* the quest end screen capture: 0x81 asked for (the quest ends: unk_3A1070.c, unk_39C290.c), 0x80 taken (ScreenCapture_Take, then cleared by the quest script), 0 none */
    /* 0x34 */ u8 unk34;
    /* 0x35 */ u8 soundLoading;         /* set while a sound bank is read at once */
    /* 0x36 */ u8 musicVolume;          /* option level 0..7 (D_006A9E51; also the movies' volume) */
    /* 0x37 */ u8 seVolume;             /* option level 0..7 (D_006A9E52) */
    /* 0x38 */ u8 unk38;
    /* 0x39 */ u8 unk39;
    /* 0x3A */ u8 unk3A;
    /* 0x3B */ u8 unk3B;
    /* 0x3C */ u8 busy;                 /* gSysBusy: 1 while a card save or a lobby network step runs */
    /* 0x3D */ u8 unk3D;                /* 0x80 after Gfx_ResetRenderStates */
    /* 0x3E */ u8 unk3E;                /* 1 after Gfx_ResetRenderStates */
    /* 0x3F */ u8 offline;          /* 1: the town runs offline (NetScene_StateOffline, NetScene_Run; LobbyScreen_EnterSeq clears it): Town_PlaceLocalPlayer, Town_StartAreaMusic, LobbyScreen_LoadBanks */
    /* 0x40 */ s16 unk40;               /* the lobby clears it with unk3F */
    /* 0x42 */ u8 unk42;
    /* 0x43 */ u8 useSavedEquip;    /* 1: the local players load their save (Quest_InitPlayers) even in quests with preset equipment (GmSub_StepLaunch mode 2; 0 in modes 0 and 1, Chara_CreateTask) */
    /* 0x44 */ u8 galleryPlay;          /* 1 while the gallery plays a scene (select_gallery.c sets it): the event flags read as clear (Save_TestEventFlag, D_006ADFD4); gm_main tests 2 */
    /* 0x45 */ u8 unk45;
    /* 0x46 */ u8 oldGameKind;          /* gOldGameKind: MH data found at boot (1 MH, 2 MHG, 0 none) */
    /* 0x47 */ u8 portalPage;           /* gPortalPage */
    /* 0x48 */ u8 pspCharaWait;         /* 1 while the PSP connect task waits for the character select (Chara_SelectTask clears it); func_008FC080 tests it */
    /* 0x49 */ u8 unk49;
    /* 0x4A */ u8 unk4A;                /* the lobby keeps a copy of gNetMode.slot here */
    /* 0x4B */ u8 gallerySel;           /* the gallery's selected entry (select_gallery.c stores the cursor - 2 and restores it) */
    /* 0x4C */ u8 galleryEntry;         /* the gallery entry a scene plays (its list value; | 0x80 for the second variant: select_gallery.c, the plsel gallery tasks) */
    /* 0x4D */ u8 unk4D;
    /* 0x4E */ u8 galleryKeep[2];       /* the gallery's two view values (its work's 0x1BC and 0x1C0), kept while a scene plays */
    /* 0x50 */ s8 language;             /* Kbd_InitState argument: the language from the boot arguments (Select_ParseLanguage) */
    /* 0x51 */ u8 pal;                  /* 1 for PAL (fps 50) */
    /* 0x52 */ u8 subMainLoaded;
    /* 0x53 */ u8 gmSubLoaded;          /* gm_sub (overlay 6) is in its window */
    /* 0x54 */ u8 *memBase;             /* 0xAC8000: the start of the memory areas below (Mem_InitLayout clears 0x55E000 bytes from it) */
    /* 0x58 */ u8 *loadBuf;             /* gLoadBuf, 0xD88000: overlay files and sound banks are read here */
    /* 0x5C */ u8 *readBuf;             /* gReadBuf, 0xF08000: LoadCompressedFile reads the compressed file here */
    /* 0x60 */ u8 *selWork;             /* 0xAC8000: the select screens work area (OptWork and PspWork at +0x100) */
    /* 0x64 */ s32 unk64;               /* 0xAE8000 */
    /* 0x68 */ s32 unk68;               /* 0xAF5200 */
    /* 0x6C */ void *bones;             /* gBones, 0xB09200: the 0x800 bone slots */
    /* 0x70 */ struct Model *models;    /* gModels, 0xBD1200: the 0x80 model headers */
    /* 0x74 */ u8 *dataBuf;             /* 0xC2C200: animation sets (Anim_LoadSet), model packs (Model_SetupWithPack) and sound banks (Sound_LoadBank) are loaded here */
    /* 0x78 */ u8 *modelPack;       /* = dataBuf (Mem_InitLayout): the model packs are read here (Load_AreaPack, 0x128000 bytes; Load_CommonModelPack, Load_AreaSubModelPack) and set up from it (Model_SetupWithPack, Model_SetupTexPack) */
    /* 0x7C */ u8 *unk7C;
    /* 0x80 */ u8 *unk80;
    /* 0x84 */ s32 prims;               /* gPrims, 0xBD5200: up to 256 prims of 0x50 bytes */
    /* 0x88 */ u8 *extraBuf;            /* 0xBDA200 (0x14000 bytes, cleared at a reset): the extra save file SaveExtra_CopyFile copies; sent at login (the request + 0x20) */
    /* 0x8C */ void *workArea;          /* 0xC0A200 (0x18000 bytes): the overlays' shared work area (gWorkArea) */
    /* 0x90 */ s32 lobbyWorkBuf;        /* 0xBEE200: the lobby's work buffer (gLobbyWorkBuf) */
    /* 0x94 */ u32 stageFile;           /* gStageFile, 0xC22200 */
    /* 0x98 */ u8 *camData;         /* 0xC2A200 (0x2000 bytes, cleared at a reset): the area's camera data (Area_LoadCamData; CamCtl_InitDefaultParams) */
    /* 0x9C */ u8 loaded[6];            /* the overlay in each window 2..7 (gOverlayWindow): the code
                                           indexes from 0x9A (Overlay_IsLoaded, Overlay_Load) */
    /* 0xA2 */ u8 unkA2[0xB0 - 0xA2];
    /* 0xB0 */ struct SaveData *saveBuf; /* 0xD60200: the save data (gSaveBuf; Emcmd_LoadPartition in gm_load.c) */
    /* 0xB4 */ s32 unkB4;               /* 0x27E00 */
    /* 0xB8 */ u32 gmSubAddr;           /* kept by Mem_InitLayout */
    /* 0xBC */ u8 bootMode;             /* gBootMode: Boot_Task's switch (select_boot.c); gm_main sets 1 */
    /* 0xBD */ u8 taskRequest;          /* a task for Task_RunFrame to start, then cleared: 1, 2 the lobby's online tasks (overlay 4), 3 Scene_QuestTask (overlay 3) */
    /* 0xBE */ u8 brightness;           /* the brightness option 0x38..0xA4, 0x80 neutral (D_006A9E5B; select_option.c prints it - 0x80; above 0x80 a white, below a black overlay, unk_3240E0.c) */
    /* 0xBF */ u8 unkBF;
    /* 0xC0 */ f32 fps;                 /* 59.94 (NTSC) or 50 (PAL) */
    /* 0xC4 */ s8 cardHasChara;         /* 1 once the memory card holds a character (loaded or saved), 0 when none; gm_main then starts at the character entry */
    /* 0xC5 */ s8 systemSaved;      /* 1 once the system data is saved (CardNew_SaveSystem, CardGame_SaveSystem, CardSave_SaveSystem, Chara_SelectTask); Card_IsFlagE055 tests it */
    /* 0xC6 */ u8 unkC6[2];
    /* 0xC8 */ u32 lbguildFree;         /* free memory after lbguild's data */
    /* 0xCC */ u32 lbguildFreeSize;
    /* 0xD0 */ u8 *memFile;             /* file 0x19EB, read through the memory file's seek/read (ACCESS's engine, 0x2FB0C0) */
    /* 0xD4 */ s32 memFileSize;
    /* 0xD8 */ s32 patchArea;           /* 0xAA6780: the end of lbguild's free memory; the patch and
                                           memory card work areas start here */
    /* 0xDC */ s32 unkDC;               /* 0x20040 */
    /* 0xE0 */ u8 unkE0;
    /* 0xE1 */ u8 plselLoaded;
    /* 0xE2 */ u8 loadAnimOn;           /* the loading letters are drawn (Loading_DrawLetters) */
    /* 0xE3 */ s8 loadAnimFrame;
    /* 0xE4 */ s8 loadAnimPhase;        /* 0..11: the step of the letters' bob */
    /* 0xE5 */ s8 unkE5;
    /* 0xE6 */ u8 unkE6[2];
    /* 0xE8 */ void *questData;         /* the current quest: its stage file header or guild list entry (Quest_SetCurrent) */
    /* 0xEC */ s16 questId;             /* the current quest's id (Quest_SetCurrent) */
    /* 0xEE */ u8 unkEE[2];
    /* 0xF0 */ s32 questFromList;       /* 1: questData is a guild list entry, 0: a stage file (Quest_SetCurrent; was the label D_006AE080) */
} SysSettings;                          /* 0xF4 bytes */

typedef char SysSettings_size_check[(sizeof(SysSettings) == 0xF4) ? 1 : -1];

extern SysSettings gSysSettings;

#endif /* SYSSETTINGS_H */
