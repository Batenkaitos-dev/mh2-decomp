#ifndef LOBBY_GLOBALS_H
#define LOBBY_GLOBALS_H

/*
 * sub_main globals the lobby used to read through raw address casts
 * (`*(u8 *)0x6ADCEC`). Fields of the scene state (gScene, scene.h), the
 * save data (gSave, save.h) and the pads (gPadState, pad.h) are named as
 * members (the files include those headers; a file that views gSave as
 * `u8 gSave[]` uses byte offsets); sub_main's scalars by their sub_main.txt
 * names; the rest are defined in config/symbols/lobby.txt (section "sub_main
 * globals named for the lobby") under their addresses unless named. Declared
 * with the files' own declaration or the type most accesses use; other
 * accesses cast the address (`*(s8 *)&gFieldControl`).
 * Where a name changed a function's code (MWCC shares one `lui` between raw
 * addresses in a 64 KB page, and assumes a named global is not aliased by
 * stores through pointers), the function keeps the raw cast.
 */

#include "common.h"

/* sub_main.txt names */
extern u16 gFrameTick;
extern u8 gPortalPage;
extern s16 gHelpLine;
extern u8 gHelpShow;
extern u8 *gLoadBuf;
extern u8 gCameraPreset;
extern s32 gNetSock;

/* config/symbols/lobby.txt */
extern void *gPadPtr;
extern s32 gNetSetupTexPack;
extern char *gInfoMenuLabels[]; /* sub_main unk_37B570.c: the info menu labels (the town info prints [0]) */
extern s32 gActiveSkillsTitle;
extern char *gMemberCardNumFmt;
extern char *gSkillDetailTitle;
extern char *gSkillDetailTitleDim;
extern s8 *gStartText;
extern char *gPageFmt;
extern s32 gPouchFullTakeText;
extern char *gPriceLegendTexts[];
extern char *gItemNames[];
extern f32 gLobbyDayFrac;
extern u16 D_581AFC;
extern s8 gNetMsgState;
extern s8 D_59D661;
extern s8 D_59D662;
extern s16 D_59D664;
extern s16 D_59D666;
extern s16 D_59D668;
extern u8 gNetProgress;
extern s8 D_59D66E;
extern u8 *gKbd;
extern s16 D_5BA5E0;
extern u32 D_5BAE0C;
extern s8 gChatHidden;
extern u8 gChatHideNames;
extern u8 gChatToAll;
extern s8 gHelpPage;
extern u8 gHelpColumn;
extern s8 gPauseMenuReq;
extern u16 gChatWinTimer;
extern u8 gChatWinOpen;
extern u8 gChatWindowSel;
extern u8 gChatSendTo;
extern u8 gChatWinState;
extern u8 gChatWinHold;
extern s32 gRewardMoney;
extern s32 gQuestQNetReadyArg;
extern u32 gSrvQNetPeerTimeout;
extern u32 gSrvQNetMaxStall;
extern s8 gNetSetupResult;
extern u8 gElderForecastHeard;
extern u16 gQuestDeskShown[][12];
extern u16 D_5BFA68;
extern u16 gQuestOfferId;
extern u8 gQuestAcceptDay;
extern u8 gLobbyLastPm;
extern u8 gLobbyLastSeason;
extern u8 gMemberNamesOffSaved;
extern u8 gChatTextColor;
extern u8 D_5C34F0;
extern u8 D_5C34F8;
extern s8 gPortalExit;
extern s8 gProxyMode;
extern u16 D_5CA910;
extern u8 D_5CAA40;
extern u8 gServerTriedCount;
extern u8 gLobbyConnDone;
extern s8 gLobbyNetError;
extern s8 D_5D46A1;
extern u8 D_5D4704;
extern s8 D_5D4705;
extern u8 gOnlineBonusState;
extern u8 gSeasonFlags;
extern u8 D_5D4924;
extern u32 D_5D4928;
extern s32 gQuestTimeLeft;
extern s32 gQuestReward;
extern struct QuestInfo *gQuestInfo;
extern f32 gCamDir;
extern u32 gQuestExtra;
extern s16 D_6ADADE;
extern s16 D_6ADAFA;
extern u8 D_6ADB0C[];
extern s16 D_6ADB14;
extern s16 D_6ADB4A;
extern s8 gOnlineFlag;
extern u8 gFieldControl; /* suggested name gFieldControl: 1 = the hunter takes pad input in the field (Field_ControlEnabled); the lobby clears it while a menu or talk runs */
extern u8 gSoundLoading;
extern s8 gSysBusy;
extern s8 gGmSubLoaded;
extern u8 *gMemAreas;
extern u8 *gObjBufBlocks;            /* the object buffer: 0x100 blocks of 0x200 bytes (StageObj_Alloc, sub_main unk_363550.c) */
extern struct ModelMatSet *gMatSets;     /* 0x180 material sets of 0x8C bytes (sub_main unk_339EC0.c) */
extern struct Obj50 *gMaterials;     /* 0x400 materials of 0x50 bytes (sub_main unk_339EC0.c) */
extern u8 *gLobbyWorkBuf;
extern void *gSaveBuf;
extern u8 gBootMode;
extern f32 gFps;
extern u8 *gLbguildFree;
extern s32 gCardMenuTex;
extern s32 gLoadLettersTex;
extern s32 D_6AE3F8;
extern s32 gOvlTex;
extern s32 D_6AEFA8;

/*
 * The online town's lists at 0x8BECA0 (gTownQuestLists): per list three entries
 * of three nonzero s16 (func_006AF410 checks them); areas 0x86 and 0x88 use
 * the first and the second set of three lists (LobbyWorld_GetSeason picks one).
 */
typedef struct TownEntry {
    /* 0x0 */ s16 v[3];
    /* 0x6 */ s16 unk6;
} TownEntry;

/* gTownQuestLists (0x8BECA0) and sub_main's copy gTownQuestListsCopy (unk_7B91F0.c
 * keeps the lists from the copy when the day and the list are the same). The
 * lobby also lends it to 6884 as four 0x100-byte reply buffers (unk_7C1540.c),
 * past its 0xD0 bytes. */
typedef struct TownQuestLists {
    /* 0x00 */ u8 unk0[4];
    /* 0x04 */ TownEntry a[3][3];   /* area 0x86 */
    /* 0x4C */ TownEntry b[3][3];   /* area 0x88 */
    /* 0x94 */ u16 quests[2][3][5]; /* quest ids, 5 per list: [0] below hunter rank 41, [1] from 41 on (ElderForecast_Load) */
} TownQuestLists;
typedef char TownQuestLists_size_check[(sizeof(TownQuestLists) == 0xD0) ? 1 : -1];

extern TownQuestLists gTownQuestLists;
extern TownQuestLists gTownQuestListsCopy;


/* A message of the table gLobbyMsgTexts (unk_7A1120.c, 152 of them; LobbyText_Open in
 * unk_7B0D80.c): its kind and flags (0x80: no ...), its text. */
typedef struct LobbyMsgDef {
    /* 0x0 */ u16 flags;
    /* 0x2 */ u16 pad2;
    /* 0x4 */ char *text;
} LobbyMsgDef;

typedef char LobbyMsgDef_size_check[(sizeof(LobbyMsgDef) == 0x8) ? 1 : -1];

#endif
