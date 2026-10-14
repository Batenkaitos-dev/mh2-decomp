#ifndef HUD_H
#define HUD_H

#include "common.h"
#include "grid.h"
#include "combine.h"

/*
 * The HUD and in-game menu state (gHudState, pointing to gHudStateBuf; Hud_Init fills it
 * for a quest): the local player, the health bar, the pouch, supply box, combine and paged
 * list menus (unk_33D7D0.c, unk_33E230.c, unk_33E620.c). Several menus use the same bytes.
 */

struct Player;

/* A recipe the combine list found for an item (Combine_ListRecipesFor): the
 * recipe and its place in the indexes (Combine_FindRecipeIndex). */
typedef struct RecipeHit {
    Recipe *recipe;     /* 0x00 */
    s16 info[4];        /* 0x04 */
} RecipeHit;

typedef struct HudState {
    s8 unk00;                   /* 0x000 */
    u8 unk001[0x004 - 0x001];
    u8 phase;                   /* 0x004 */
    u8 mode;                    /* 0x005: 1: the pouch-drop state (FieldMenu_Update) */
    u8 supply;                  /* 0x006: the supply box is open (Player.supplyBoxOpen) */
    u8 dropState;               /* 0x007: the pouch-full drop dialog state (PouchDrop_Update), 0: none */
    struct Player *player;      /* 0x008: the local player (Hud_Init) */
    s16 dpadRepeatDir;          /* 0x00C: the D-pad direction being repeated */
    s8 dpadRepeatTimer;         /* 0x00E: frames to the next repeat: 8, then 3 */
    u8 yesNo;                   /* 0x00F: the yes/no choice (0 yes) */
    s16 timer[4];               /* 0x010: per player (TIMER in unk_33E620.c) */
    u8 unk018[4];               /* 0x018 */
    u8 statusIcon[4];           /* 0x01C: per player: the status icon shown (0..14, cycles through the set bits of the mask; SEL in unk_33E620.c) */
    u8 unk020[0x022 - 0x020];
    u8 deliveryMenu;            /* 0x022: Player.deliveryMenu, copied by ItemHud_Update */
    u8 deliveryOpen;            /* 0x023: Player.deliveryOpen, copied each frame (ItemHud_Update) */
    s32 timeLeft;               /* 0x024: Quest_GetTime(0) (Hud_DrawClock) */
    s32 timeTotal;              /* 0x028: Quest_GetTime(1) (Hud_DrawClock) */
    s16 hp;                     /* 0x02C: the health shown, moves one a frame toward Player.hp */
    s16 hpRecoverable;          /* 0x02E */
    s8 camEventFade;            /* 0x030: 4 while no event view; counts down to 0 while one runs (Hud_DrawCamEvent draws while < 4) */
    u8 camEventBlink;           /* 0x031: blink phase at camEventFade 0 (& 0x1F) */
    s8 camEvent;                /* 0x032: the CamAim_GetState result, -1: no event view */
    s8 camEventStart;           /* 0x033: 1 on the frames the event view starts (camEventFade still 4) */
    f32 invScaleX;              /* 0x034: 1 / the display mode's x scale (gMapScales) */
    f32 invScaleY;              /* 0x038: 1 / its y scale */
    s16 pingTimer[4];           /* 0x03C: per player: frames since the map ping, -1: none */
    s16 pingHold;               /* 0x044: frames the ping button must be held (10) */
    u8 mapZoomed;               /* 0x046: 1: the zoomed map (Map_DrawZoomed) */
    u8 glowTimer;               /* 0x047: frames of the glow effect (0x1A) */
    u8 menuPhase;               /* 0x048: the field menu: 0 the list, 1 a page (FieldMenu_Update) */
    u8 fieldMenuCount;          /* 0x049: the field menu entries (7 or 8) */
    u8 fieldMenuCursor;         /* 0x04A: the field menu entry (FieldMenu_Update tables) */
    u8 listCursor;              /* 0x04B: the cursor of the status, info and chat menus */
    u8 infoSet;                 /* 0x04C: the info menu set (0-3: table gInfoMenuCounts/gInfoMenuEntries) */
    u8 menuStep;                /* 0x04D: step of the status, info and chat menus */
    u8 pageCursor;              /* 0x04E: the page (left/right) of the quest, status and equipment pages */
    u8 infoEntry;               /* 0x04F: the chosen info entry (from gInfoMenuEntries) */
    u8 state;                   /* 0x050: pouch menu: 0 pick a slot, 1 pick an action, 2 do it, 3 offered, 4 closing */
    u8 slot;                    /* 0x051: the selected pouch slot */
    u8 action;                  /* 0x052: 0 drop, 1 give, 2 move */
    u8 step;                    /* 0x053: giving: 0 the count, 1 the player */
    s8 target;                  /* 0x054: the player given to */
    u8 count;                   /* 0x055: the count given (Menu_AdjustCount) */
    u8 atMax;                   /* 0x056 */
    u8 dropChoice;              /* 0x057: the drop dialog yes/no (0 yes) */
    s16 dropHelpLine;           /* 0x058: the help line of the drop dialog: 0x14/0x15/0x16 or item + 0x18 */
    u16 dropItem;               /* 0x05A: the item that did not fit, 0: none */
    u8 dropNotice;              /* 0x05C: the notice id of the item-get notice, 0xFF: not posted */
    u8 dropBlink;               /* 0x05D: blink counter of the drop dialog */
    s8 dropCount;               /* 0x05E: how many of dropItem */
    s8 pouchBarShown;           /* 0x05F: 1 when Hud_DrawPouchBar drew this frame */
    s8 blink60;                 /* 0x060 */
    s8 dir61;                   /* 0x061 */
    u16 pouchCursor;            /* 0x062 */
    s8 blink64;                 /* 0x064 */
    s8 dir65;                   /* 0x065 */
    u16 ammoSlot;               /* 0x066 */
    s16 supplyMsgTimer;         /* 0x068: frames of the supply box flash (0x1F) */
    u8 supplyFlashSlot;         /* 0x06A: the supply box cursor flashed (Player +0x81B) */
    u8 supplyMoveFrames;        /* 0x06B: an item taken: its frames */
    u8 supplyMoveTimer;         /* 0x06C: its frames left */
    u8 supplyMoveSlot;          /* 0x06D: its supply box slot */
    u16 supplyMoveItem;         /* 0x06E: the item */
    Recipe *combineRecipe;      /* 0x070: the combine menu recipe (CombineWork.recipe) */
    u16 combineItems[3];        /* 0x074: the combine menu's ingredients */
    u8 combineSlots[3];         /* 0x07A: the pouch slots of combineItems, 0xFF: none (CombineWork.slots) */
    u8 combineThree;            /* 0x07D: 1 for a three-ingredient combine (CombineWork.three) */
    u8 unk07E[0x080 - 0x07E];
    s16 combineResult;          /* 0x080: the recipe result item, -1: none */
    u8 unk082[0x084 - 0x082];
    s16 unk84;                  /* 0x084: ItemGet_Reserve's item (CombineMenu_Open sets -1) */
    u8 unk086[0x088 - 0x086];
    s16 unk88;                  /* 0x088 */
    u8 combineConfirm;          /* 0x08A: the combine confirm cursor (CombineWork.unk8A) */
    u8 unk08B[0x08C - 0x08B];
    u8 fadeActive;              /* 0x08C: 1 while a menu fade runs */
    u8 fadeMode;                /* 0x08D: the Hud_FadeStart mode */
    s16 fadeFrame;              /* 0x08E: frames since Hud_FadeStart (from 1) */
    u8 equipCursor;             /* 0x090: the equipment status page */
    u8 equipCursorMax;          /* 0x091: its page count (Equip_LevelMark) */
    u8 hasQuestInfo;            /* 0x092: QuestMenu_CheckInfo: 0 online without a quest, else 1 */
    u8 questMenuOffline;        /* 0x093: 1 offline: picks the quest menu tables (gQuestMenuCursorMax/gQuestMenuKinds) */
    s8 unk94;                   /* 0x094 */
    s8 noteMonster;             /* 0x095: the monster shown in the monster notes, -1: none */
    s8 noteHelpHide;            /* 0x096: the monster notes: bit 0x80 hides the help line (button 0x200 toggles it; MonsterList_Open sets 0x80) */
    u8 phraseStep;              /* 0x097: the phrase menu step */
    u8 hidden;                  /* 0x098 */
    u8 optionKind;              /* 0x099: the option menu set (0-4, by Game_IsOnline) */
    u8 optionCount;             /* 0x09A: the option menu entries (gOptionMenuCounts[optionKind]) */
    u8 cockpitOff;              /* 0x09B: the cockpit (HUD) display option: 1 hidden */
    u8 onlineHud;               /* 0x09C: 1 in the online town (Hud_Init picks the B draw layers) */
    s8 optAimReverse;           /* 0x09D: the option menu copy of gScene.aimReverse (D_006A9E54) */
    s8 optCamFollow;            /* 0x09E: the option menu copy of gSysSave.options.camFollow */
    s8 optCamControl;           /* 0x09F: the option menu copy of gSysSave.options.camControl */
    s8 optVibration;            /* 0x0A0: the option menu copy of gSysSave.options.vibration */
    s8 optChatSize;             /* 0x0A1: the option menu copy of gSysSave.options.chatSize */
    u8 optChatHideNames;        /* 0x0A2: the option menu copy of gChatHideNames */
    s8 optChatBeep;             /* 0x0A3: the option menu copy of gSysSave.options.chatBeep */
    u8 dropShowSlot;            /* 0x0A4: 1: the drop dialog shows the pouch slot item */
    u8 combineTaken;            /* 0x0A5: ingredients taken out of the pouch (CombineWork.unkA5) */
    u8 sorting;                 /* 0x0A6: the pouch menu's sort prompt is open */
    u8 dest;                    /* 0x0A7: moving: the slot moved to */
    u8 cancel;                  /* 0x0A8 */
    u8 camReqStarted;           /* 0x0A9: CamReq_Started() of the last update */
    u8 unkAA;                   /* 0x0AA */
    u8 unk0AB[0x0AC - 0x0AB];
    u16 clockHand;              /* 0x0AC: the clock hand shown (16-bit turn), catches up 0x16C a frame */
    u16 dpadRepeat;             /* 0x0AE: the D-pad presses of this frame with key repeat (Menu_ReadDpad) */
    u16 dpadPrev;               /* 0x0B0: the D-pad held last frame (Menu_ReadDpad) */
    u8 maplessArea;             /* 0x0B2: compared with Map_IsMaplessArea of a map icon area (unk_33E230.c) */
    u8 unkB3;                   /* 0x0B3 */
    u8 unk0B4[0x0B5 - 0x0B4];
    u8 cursorMax;               /* 0x0B5: the entry or page count for listCursor / pageCursor */
    u8 questMenuStep;           /* 0x0B6: the quest menu step */
    u8 popupOpen;               /* 0x0B7: nonzero while a message popup takes the input (MsgEvent_PopupInput, ItemHud_Update): Hud_InputBlocked offline, Hud_DrawLayer3 skips */
    s32 gsSave6C;               /* 0x0B8: Gs state 0x6C saved by Hud_SaveGsState */
    s32 gsSave6D;               /* 0x0BC: Gs state 0x6D saved by Hud_SaveGsState */
    s16 hpBlink;                /* 0x0C0: phase of the health bar colour pulse (16-bit turn) */
    s16 staminaBlink;           /* 0x0C2: phase of the stamina bar colour pulse */
    Grid grid;                  /* 0x0C4: the combine list's cursor (CombineList_Setup; grid.unk0 is
                                   Combine_ListRecipesFor's flags, grid.row the item listItems[row]) */
    RecipeHit sets[3];          /* 0x0D0: the recipes found for the item (Combine_ListRecipesFor) */
    u16 listItems[5];           /* 0x0F4 */
    u8 set;                     /* 0x0FE */
    u8 setCount;                /* 0x0FF */
    u8 kbdOpen;                 /* 0x100: 1 while the chat input line keyboard is open (Chat_InputLine; the comment saved in gLoadBuf) */
    u8 unk101[0x108 - 0x101];
} HudState;

typedef char HudState_size_check[(sizeof(HudState) == 0x108) ? 1 : -1];

extern HudState gHudStateBuf;  /* the state (0x108 bytes, cleared by Hud_Reset) */
extern HudState *gHudState; /* points to gHudStateBuf */

#endif /* HUD_H */
