#ifndef SELECT_H
#define SELECT_H

/* The select overlay (title, menus, character creation): types and the
 * functions of main/sub_main its units call. */

#include "common.h"
#include "sprite.h"            /* Sprite (Sprite_Draw) */
#include "task.h"              /* TaskSlot */

/* A piece of a screen: texture, flag, texture rectangle, position and size
 * (tables of them in the units' .data; Sel_DrawParts draws them). */
typedef struct SelPart {
    u8 tex;                 /* 0x00 */
    u8 flag;                /* 0x01 */
    s16 u0, v0, u1, v1;     /* 0x02 */
    s16 x, y, w, h;         /* 0x0A */
} SelPart;

void Title_ResetBgAnim(void);
void Title_StepBgAnim(void);
void Title_RandomView(void);
void Title_Draw(void);
void Sel_DrawParts(SelPart *p, s32 n, u32 color);   /* select_title.c: draws n parts */
void Sel_DrawPartsRotated(SelPart *part, s32 n, u32 color, s32 angle);
void Title_DrawLogo(void);
void Title_DrawMenu(void *unused);
void ModeSel_DrawScreen(void);
void Sel_DrawBackground(u8 gray);
void Sel_DrawHeader(s16 kind);
void ModeSel_StartSlide(void);
s32 ModeSel_IsIdle(void);
void ModeSel_UpdateSlide(void);
void ModeSel_DrawExtra(void);
void ModeSel_DrawButton(s32 *item);
void ModeSel_DrawButtons(s32 sel, s32 frame);
void ModeSel_Draw(s32 sel, s32 frame);
void CharaSel_Start(void);
void CharaSel_Leave(void);
void CharaSel_Update(void);
s32 CharaSel_IsIdle(void);
s32 CharaSel_IsDone(void);
s32 CharaSel_IsShown(void);
void CharaSel_DrawSlots(void);
void CharaSel_Draw(void);
void Sel_LoadCommonTextures(void);
void Sel_FreeTextures(void);

/* Character creation's work, at 0x1000 in the shared work area. */
typedef struct CharaWork {
    union { s8 state; s8 sideState[2]; }; /* 0x000: Parts_DrawSides runs the two side panels' slide-in states (0-7) in bytes 0 and 1 (as ((s8 *)c)[k]); the character selection's state (CharaSel_Leave sets 2) */
    u8 unk2[2];
    s32 unk4;               /* 0x004 */
    s16 slideOut; /* 0x008: 0: the panels slide in (Parts_SlideInPanels), 1: out (Parts_SlideOutPanels); Parts_Animate. The character selection's slot (CharaSel_DrawSlots, CharaSel_Leave) */
    u8 unkA[0x4E];
    s32 slide[7]; /* 0x058: per panel, its slide step (x offset 80 per step, Parts_DrawPanels): counted down to 0 when sliding in, up to 10 when out (Parts_Animate); [0], [1] and [4] the side panels' timers and shared delay (Parts_Init, Parts_DrawSides) */
    s32 sliding[7]; /* 0x074: per panel, 1 while it slides (Parts_Animate clears it) */
    u8 unk90[0x7C];
    f32 unk10C[6];          /* 0x10C: only CharaSel_Leave (the CharaSelWork view of offset 0x108) */
    u8 unk124[0x3C];
    s32 slideDelay[7]; /* 0x160: per panel, frames before it slides out (Parts_SlideOutPanels: 0, 2, 4...) */
    u8 unk17C[4];
    s32 blink; /* 0x180: 0-2, one step each 4 frames (Parts_Animate): the cursor's frame (Parts_DrawCursor, CharaSel_DrawSlots) */
    s8 bounce[10]; /* 0x184: per list entry, its bounce: 0 none, 1 out, 2 back (Parts_StartBounce, Parts_Animate); its marker is lit while nonzero (Parts_DrawMarker) */
    s8 bounceOfs[10]; /* 0x18E: per list entry, the bounce offset (to +-5) */
    s16 cursorY; /* 0x198: the cursor's y while it jumps (Parts_DrawCursor; Parts_StartPanel sets the start) */
    s16 cursorYEnd; /* 0x19A: where the jump ends (Parts_StartPanel) */
    u8 unk19C[2];
    u8 cursorJump; /* 0x19E: the cursor's jump: 0 none, 1 up (-28 a frame, 12 frames), 2 down (+36 to cursorYEnd); Parts_StartPanel sets 1 */
    u8 unk19F;
    s16 cursorJumpFrame; /* 0x1A0: frames of the jump up */
    u8 unk1A2[2];
} CharaWork;

/* Character creation's steps (Chara_CreateTask, its task's mode;
 * Parts_DrawPanels and Chara_DrawPages draw by them). */
enum CharaCreateStep {
    CHARA_CREATE_INIT,          /* 0: loads the textures, resets gChrEdit */
    CHARA_CREATE_LOAD,          /* 1: waits for the sound banks, then fades in */
    CHARA_CREATE_FADE_IN,       /* 2: waits for the fade */
    CHARA_CREATE_GENDER,        /* 3: the gender choice */
    CHARA_CREATE_PANELS,        /* 4: the list of panels (CharaPanel) */
    CHARA_CREATE_EDIT,          /* 5: the chosen panel's page */
    CHARA_CREATE_CONFIRM,       /* 6: the name check and the confirmation */
    CHARA_CREATE_SAVE,          /* 7: the memory card's save menu */
    CHARA_CREATE_DONE,          /* 8: the character speaks, 60 frames */
    CHARA_CREATE_FADE_OUT,      /* 9: waits for the fade */
    CHARA_CREATE_END,           /* 10: ends the task and starts the game */
    CHARA_CREATE_ASK_QUIT,      /* 11: asks whether to leave for the menu */
    CHARA_CREATE_QUIT,          /* 12: fades out, back to gm_sub's menu */
    CHARA_CREATE_ASK_GENDER     /* 13: asks whether to go back to the gender choice */
};

/* Character selection's steps (Chara_SelectTask, its task's mode). */
enum CharaSelectStep {
    CHARA_SELECT_INIT,          /* 0: loads the area, opens the card's load menu */
    CHARA_SELECT_CARD,          /* 1: the load menu; then sets up the saved characters */
    CHARA_SELECT_LOAD,          /* 2: waits for the sound banks */
    CHARA_SELECT_CHOOSE,        /* 3: left and right pick a character */
    CHARA_SELECT_CONFIRM,       /* 4: the yes/no question */
    CHARA_SELECT_MESSAGE,       /* 5: the card menu's message, until a press */
    CHARA_SELECT_DONE,          /* 6: the character speaks, 65 frames */
    CHARA_SELECT_FADE_OUT,      /* 7: waits for the fade */
    CHARA_SELECT_END            /* 8: after 60 frames, starts the game */
};

/* Character creation's panels (ChrEdit.panel), in the list's order. */
enum CharaPanel {
    CHARA_PANEL_NAME,           /* 0: the name, through the software keyboard */
    CHARA_PANEL_TYPE,           /* 1: the type, 4 of them */
    CHARA_PANEL_FACE,           /* 2 */
    CHARA_PANEL_HAIR,           /* 3 */
    CHARA_PANEL_COLOR,          /* 4: the hair colour */
    CHARA_PANEL_VOICE,          /* 5 */
    CHARA_PANEL_FINISH          /* 6: ends the creation */
};

/* The parts Chara_FilterParts filters (their conditions are ChrEdit.cond
 * 0-2, 3-5 and 6-7). */
enum CharaPartKind {
    CHARA_PARTS_FACE,
    CHARA_PARTS_HAIR,
    CHARA_PARTS_VOICE
};
#define CHARA_FACE_COUNT  27
#define CHARA_HAIR_COUNT  18
#define CHARA_VOICE_COUNT 15
#define CHARA_HAIR_SPECIAL 0x11 /* the last hair style: offered only when gSysSave.bonusUnlocked is set; saved as style 19 */
#define CHARA_COND_ANY 0        /* a condition that any part fits */
#define CHARA_COND_NONE 99      /* in the condition tables: no value (the random look uses CHARA_COND_ANY instead) */

/* The colour editor's rows (ChrEdit.choice): red, green, blue, then the
 * presets (gPresetColors, 16) and the row that confirms. */
enum CharaColorRow {
    CHARA_COLOR_ROW_RED,
    CHARA_COLOR_ROW_GREEN,
    CHARA_COLOR_ROW_BLUE,
    CHARA_COLOR_ROW_PRESETS,
    CHARA_COLOR_ROW_DONE
};
#define CHARA_PRESET_COLOR_COUNT 16

/* The answers of a yes/no question (ChrEdit.choice). */
#define CHARA_CHOICE_YES 0
#define CHARA_CHOICE_NO  1

/* Chara_DrawQuestion's questions. */
enum CharaQuestion {
    CHARA_ASK_CREATE,           /* 0: confirm the finished character (or the name's error) */
    CHARA_ASK_SAVE,             /* 1: confirm saving it */
    CHARA_ASK_SELECT,           /* 2: confirm the selected character */
    CHARA_ASK_QUIT,             /* 3: leave for the menu */
    CHARA_ASK_GENDER,           /* 4: go back to the gender choice */
    CHARA_ASK_NET_MESSAGE       /* 5: the card menu's message 48 */
};

/* Chara_SetCamera's views. */
enum CharaCamera {
    CHARA_CAM_BODY,             /* 0: the whole body, from the front left */
    CHARA_CAM_FACE,             /* 1: close on the head (bone 20) */
    CHARA_CAM_FRONT             /* 2: the body from the front */
};

#define CHARA_SLOT_COUNT 3      /* the characters on a memory card */
#define CHARA_SLOT_NONE 0xFF    /* ChrEdit.slot: none (Chara_MakeSave: write gSave itself) */
#define CHARA_GENDER_NONE 0xFF  /* ChrEdit.loadedGender: no body model loaded */

/* A record of the name check's list (gNgWords, select_ngword.c): up to 14
 * characters of a word, then its length. A longer word goes on in the next
 * records, whose len is -1. The list ends at a record whose text starts
 * with 0. */
#define NGWORD_PART_LEN 14
typedef struct NgWordRec {
    s8 text[NGWORD_PART_LEN];   /* 0x00 */
    s8 unkE;
    s8 len;                     /* 0x0F: the word's length, or -1 in a continuation */
} NgWordRec;

/* gPartsPanels's records: a panel of character creation (Parts_DrawPanels). */
typedef struct PartsRec {
    s16 x, y, w, h;         /* 0x00 */
    s32 color;              /* 0x08 */
    u16 part;               /* 0x0C: its SelPart in gPartsCursorParts */
    s16 tx, ty;             /* 0x0E: the label */
    s16 tx2, ty2;           /* 0x12: the value */
    s16 unk16;
} PartsRec;

/* The colours of the face, hair and voice panels' rows (select_parts.c). */
#define PARTS_COLOR_SELECTED  0xFF32DA5D    /* the row under the cursor */
#define PARTS_COLOR_TEXT      0xFF2A2120    /* the other rows */
#define PARTS_COLOR_MARKER    0xFF342B2A    /* an arrow marker, unlit */
#define PARTS_COLOR_LIT       0xFFFB7103    /* an arrow marker while its entry bounces */
#define PARTS_COLOR_NO_MATCH  0xFFFFC000    /* the values when no part fits the conditions */

/* The options' work (select_option.c, gOptWork), at 0x100 in
 * gSysSettings.selWork. The options have a main page (sound and the
 * equipment view, with links to the other pages), a controls page
 * (vibration, chat size, aim and camera), the screen position and the
 * brightness; the network options share the work. */
typedef struct OptWork {
    s32 state;              /* 0x00: 0-1 fade in, 2 input, 3-4 fade out, then the next page */
    s32 step;               /* 0x04: Option_Exit's step; the network pages' step */
    s32 confirmStep;        /* 0x08: the confirmation's: 0 choosing, 1 waiting to start the setup guide */
    u8 unkC[4];
    s32 timer;              /* 0x10: frames (Option_Exit's wait, the confirmation's delay) */
    u8 unk14[0xC];
    s32 page;               /* 0x20: the page shown */
    s32 nextPage;           /* 0x24: the page after the fade */
    s32 cursor;             /* 0x28: the row under the cursor (the main page, the network page) */
    s32 subCursor;          /* 0x2C: the controls page's row; the confirmation's choice (0 yes, 1 no) */
    u8 unk30[8];
    s32 textSize;           /* 0x38: the rows' letter size */
    s32 textSize2;          /* 0x3C: set to the same, never read */
    s32 rowX;               /* 0x40 */
    s32 rowY;               /* 0x44: the first row's y */
    s32 rowStep;            /* 0x48: the rows' spacing */
    f32 selScale;           /* 0x4C: the row under the cursor is drawn this much larger */
    s32 valueX;             /* 0x50: where the values' column starts */
    s32 textColor;          /* 0x54 */
    u32 selColor;           /* 0x58: the row under the cursor */
    u32 defaultsColor;      /* 0x5C: the defaults row while the settings differ from them */
    u32 dimColor;           /* 0x60: ... while they don't (the row is skipped) */
    s32 helpX;              /* 0x64: the help line's position */
    s32 helpY;              /* 0x68 */
    s32 screenPosMoved;     /* 0x6C: the pad directions the picture moved this frame */
    s32 mainDiffers;        /* 0x70: the main page's settings differ from the defaults */
    s32 controlsDiffer;     /* 0x74: the controls page's */
    s32 changed;            /* 0x78: the settings differ from those at the start (to be saved) */
    s32 netDiffers;         /* 0x7C: the network settings differ from the defaults */
    s8 savedOptions[0x10];  /* 0x80: gSysSave.options at the start */
    s8 savedNetOptions[0x10]; /* 0x90: gSysSave.netOptions at the start */
    s32 arrowsShown;        /* 0xA0: the arrows of the row under the cursor are drawn */
    s16 arrowBounce;        /* 0xA4: 1 the right arrow bounces (the value went up), 2 the left */
    s16 arrowLeftX;         /* 0xA6 */
    s16 arrowRightX;        /* 0xA8 */
    s16 arrowLeftOfs;       /* 0xAA: the bounce's offsets, 0-4 */
    s16 arrowRightOfs;      /* 0xAC */
    s16 arrowY;             /* 0xAE */
} OptWork;

/* The boot's attract loop (BootWork.mode): the opening movie, the title
 * screen, the still picture, then the movie again (select_boot.c). */
enum BootScreen {
    BOOT_MOVIE = 0,
    BOOT_TITLE = 1,
    BOOT_PICTURE = 2
};

/* Boot_Update's states (BootWork.state). */
enum BootState {
    BOOT_START = 0,         /* fade in, then run */
    BOOT_RUN = 1,           /* the attract loop */
    BOOT_LEAVE = 2          /* skipped: GmSub_Task takes over */
};

/* The boot's work (select_boot.c, gBootWork), at the start of the shared
 * work area. */
typedef struct BootWork {
    u8 state;               /* 0x00: enum BootState */
    u8 step;                /* 0x01: the step within the screen */
    u8 unk2;                /* 0x02: only cleared, at the movie's start */
    u8 unk3;
    s32 movieFrames;        /* 0x04: frames since the movie started (counted, never read) */
    s32 blinkTimer;         /* 0x08: +1 a frame; the start prompt shows while bit 0x20 is set */
    u8 unkC[0x14];
    s32 waitFrames;         /* 0x20: frames left in the step */
    u8 unk24[0x10];
    s32 skipped;            /* 0x34: 1 once a button was pressed: the boot leaves for the game */
    s32 mode;               /* 0x38: the screen shown, enum BootScreen */
} BootWork;

/* A sprite record with its texture (gCharaSelSlotIcons: the save slots'
 * frames; gCharaSelEntryIcons: the character selection's pictures). */
typedef struct SelIcon {
    s32 tex;                /* 0x00: index into gTexHandles, or into the screen's textures */
    s16 x, y;               /* 0x04: offsets */
    s16 w, h;               /* 0x08 */
    s16 u0, v0, u1, v1;     /* 0x0C */
    u32 color;              /* 0x14 */
    u16 angle;              /* 0x18: 0 unrotated */
    u16 unk1A;
    f32 scale;              /* 0x1C */
} SelIcon;

/* GmSub_Task's own modes (its TaskSlot.mode). */
enum GmSubTaskMode {
    GMSUB_TASK_INIT = 0,        /* clears gGmWork, or goes back to the gallery after a gallery scene */
    GMSUB_TASK_RUN = 1,         /* the title screen: GmWork.mode's phases */
    GMSUB_TASK_LAUNCH = 3,      /* mode select chose: GmSub_StartGame ends the task into the lobby */
    GMSUB_TASK_EXIT = 4         /* GmSub_ExitToBoot: back to select's Boot_Task (no code seen sets it) */
};

/* GmWork.mode: the title screen's phase (GmSub_Task, GMSUB_TASK_RUN). */
enum GmSubMode {
    GMSUB_MODE_SETUP = 0,       /* GmSub_Setup */
    GMSUB_MODE_MENU = 1,        /* the title menu and mode select (GmSub_UpdateMenu) */
    GMSUB_MODE_START_CHARA = 2, /* the character entries: start their select task, or the game once mode select chose */
    GMSUB_MODE_START_ENTRY = 3  /* the other entries: start the entry's select task */
};

/* GmWork.step: GmSub_UpdateMenu's steps. */
enum GmSubStep {
    GMSUB_STEP_INIT = 0,                /* sets the menu up (GmSub_InitMenu) and fades in */
    GMSUB_STEP_FADE_IN = 1,             /* the fade-in; then the logo, the title menu or mode select */
    GMSUB_STEP_LOGO = 2,                /* the logo's animation (GmWork.logoAnim up to GMSUB_LOGO_ANIM_END) */
    GMSUB_STEP_MENU = 3,                /* the title menu */
    GMSUB_STEP_CHOSEN = 4,              /* an entry chosen: after the fade-out, GmWork.mode starts it */
    GMSUB_STEP_MODE_SELECT = 5,         /* mode select: online or offline */
    GMSUB_STEP_MODE_CANCEL = 6,         /* mode select cancelled: back to the title menu after the fade */
    GMSUB_STEP_BACK = 10,               /* back from options, network options or the PSP link: fades in */
    GMSUB_STEP_RESET = 11,              /* back from the character screens or the gallery: reloads, fades in */
    GMSUB_STEP_REOPEN_MODE_SELECT = 12, /* GmSub_OpenModeSelect's step, then as GMSUB_STEP_RESET (no other code sets it) */
    GMSUB_STEP_QUIT = 20                /* the title menu cancelled: the boot screens after the fade */
};

/* The title menu's entries (GmWork.menuSel, menuChoice): each starts one of
 * select's tasks in TASK_SLOT_MENU (the PSP link in TASK_SLOT_SCENE). */
enum GmSubMenuEntry {
    GMSUB_MENU_NEW_CHARA = 0,   /* Chara_CreateTask */
    GMSUB_MENU_LOAD_CHARA = 1,  /* Chara_SelectTask, then mode select */
    GMSUB_MENU_GALLERY = 2,     /* Gallery_Task */
    GMSUB_MENU_OPTIONS = 3,     /* Option_Task */
    GMSUB_MENU_NET_OPTIONS = 4, /* NetOption_Task */
    GMSUB_MENU_PSP_LINK = 5,    /* Psp_ConnectTask */
    GMSUB_MENU_COUNT = 6
};

/* Mode select's entries (GmWork.sel, modeChoice; SaveData.modeSelChoice):
 * GmSub_StartGame sets gSysSettings.online and useSavedEquip from them. */
#define GMSUB_MODE_SEL_ONLINE 0
#define GMSUB_MODE_SEL_OFFLINE 1
#define GMSUB_MODE_SEL_SAVED_EQUIP 2    /* offline with the save's equipment: handled, but the cursor reaches only the first two */
#define GMSUB_MODE_SEL_COUNT 2

/* The title screen's stream (Stream_Play's file id, on channel 0). */
#define GMSUB_TITLE_STREAM 0x2F

/* GmWork.logoAnim's last value: the logo's animation is over (GMSUB_STEP_LOGO). */
#define GMSUB_LOGO_ANIM_END 4

/* gm_sub's work area (gGmWork, 176 bytes in gSysSettings.workArea): the
 * title menu's and mode select's state. gm_main.c runs the menus
 * (GmSub_Task, GmSub_UpdateMenu); select_title.c draws them from it. */
typedef struct GmWork {
    s32 mode;               /* 0x00: enum GmSubMode */
    s32 step;               /* 0x04: enum GmSubStep (GmSub_UpdateMenu) */
    u8 unk08[0x20 - 0x08];
    s32 logoFull;           /* 0x20: gm_main sets 1; nonzero: the logo at its full size */
    s32 menuSel;            /* 0x24: the title menu's entry under the cursor (enum GmSubMenuEntry) */
    s32 sel;                /* 0x28: mode select's entry under the cursor (GMSUB_MODE_SEL_*, ModeSel_Start) */
    s32 menuChoice;         /* 0x2C: the title menu's entry chosen (menuSel), -1: none */
    s32 modeChoice;         /* 0x30: mode select's entry chosen (sel), -1: none; >= 0 starts it (GmSub_Task) */
    void (*frameCallback)(void); /* 0x34: added each frame (Callbacks_Add) unless mode select draws: Title_DrawLogo */
    void (*layerCallback)(void); /* 0x38: the screen's drawing, on layer 1 (LayerCallbacks_Add): Title_Draw or ModeSel_DrawScreen */
    s32 frame;              /* 0x3C: 0-3, +1 a frame (Title_StepBgAnim) */
    s32 count;              /* 0x40: +1 each 4 frames: animates the chosen entry's number */
    u8 unk44[0x4C - 0x44];
    s32 unk4C;              /* 0x4C: only cleared (gm_sub_entry) */
    s32 logoAnim;           /* 0x50: the logo's animation, +1 a frame from 0 to GMSUB_LOGO_ANIM_END: 1 and 2 pick the smaller scales of gTitleLogoScales, the rest the full size (Title_DrawLogo) */
    u32 menuSelColor;       /* 0x54: the chosen entry's text colour */
    u32 menuColor;          /* 0x58: the other entries' */
    f32 logoScale;          /* 0x5C */
    s32 logoX;              /* 0x60: the logo's second part: position, angle and colour (Title_RandomView) */
    s32 logoY;              /* 0x64 */
    s32 logoAngle;          /* 0x68: ANGLE_TURN a turn */
    s32 logoColor;          /* 0x6C */
    u8 unk70[0xB0 - 0x70];
} GmWork;
typedef char GmWork_size_check[(sizeof(GmWork) == 0xB0) ? 1 : -1];

/* An entry of the character selection or of mode select: its two pictures
 * (255: none), indices into gCharaSelEntryIcons or gModeSelIcons. */
typedef struct SelItem {
    s32 kind;               /* 0x00: the first picture; 255: the entry is empty */
    s32 pic2;               /* 0x04: the second picture */
} SelItem;

typedef struct CharaSelPos {
    f32 x, y;
} CharaSelPos;
void CharaSel_DrawEntry(s32 *item, CharaSelPos *pos);

/* The character selection's work, at 0x1000 in the shared work area (the
 * memory of CharaWork, which character creation uses). Its entries slide
 * in from the right and out to the left. */
typedef struct CharaSelWork {
    s8 state;               /* 0x000: 0 sliding in, 1 choosing, 2 sliding out, 3 done */
    s8 slidDone;            /* 0x001: entries that finished sliding */
    u8 unk2[2];
    s32 shown;              /* 0x004: 1 once the entries are in (CharaSel_IsShown) */
    s16 slot;               /* 0x008: the save slot under the cursor (from gChrEdit.slot) */
    u8 unkA[0xE];
    s32 tex[16];            /* 0x018: texture handles */
    s32 delay[5];           /* 0x058: per entry, frames before it slides */
    u8 unk6C[8];
    s32 slideState[5];      /* 0x074: per entry: 1 waiting, 2 in, 3 in place, 4 waiting, 5 out, 6 gone */
    u8 unk88[8];
    SelItem items[5];       /* 0x090 */
    CharaSelPos pos[5];     /* 0x0B8 */
    CharaSelPos target[5];  /* 0x0E0: where the slide in stops */
    CharaSelPos speed[5];   /* 0x108: the slide's step a frame */
    s32 order[5];           /* 0x130: the drawing order */
    s32 count;              /* 0x144 */
    s32 entryY[3];          /* 0x148: the save slots' entries: y, by the slot under the cursor */
    f32 entryScale[3];      /* 0x154: ... their scale (1: the chosen one, full size) */
    u8 unk160[0x1C];
    s32 frame;              /* 0x17C: +1 a frame while choosing */
    s32 blink;              /* 0x180: 0-2, a step each 4 frames: the chosen slot's frame */
    u8 unk184[0x20];
} CharaSelWork;

/* Mode select's work, at 0x1000 in the shared work area: two buttons that
 * slide out and back in, swapping sides, when the choice changes. */
typedef struct ModeWork {
    s8 state;               /* 0x000 */
    u8 unk1[3];
    s16 sel;                /* 0x004: the mode chosen */
    u8 unk6[6];
    s32 tex[16];            /* 0x00C: texture handles */
    u8 unk4C[0x14];
    s32 slideState[5];      /* 0x060: per item ([1] and [2] the buttons): 1 in place, 2 sliding out, 3 sliding back */
    SelItem items[5];       /* 0x074 */
    s32 order[5];           /* 0x09C: the drawing order */
    s32 count;              /* 0x0B0 */
    f32 buttonPos[2][2];    /* 0x0B4: the two buttons' x, y */
    f32 buttonHomeX[2];     /* 0x0C4: where each button's slide back stops */
    f32 piecePos[8][2];     /* 0x0CC: the buttons' pieces' offsets (gModeSelLayoutB) */
    f32 slideSpeed[2];      /* 0x10C: the buttons' slide speeds */
    u32 slidePhase[2];      /* 0x114: ... their phases (ANGLE_TURN a turn) */
    u32 slidePhaseStep[2];  /* 0x11C: ... and phase steps */
    s32 layout;             /* 0x124: which button is on the left (gModeSelLayoutA/B's row) */
    s32 shownLayout;        /* 0x128: the layout drawn */
} ModeWork;

/* A mode select picture (gModeSelIcons, 20 bytes each). */
typedef struct ModeIcon {
    s32 tex;                /* 0x00: index into the mode's textures */
    s16 u0, v0, u1, v1;     /* 0x04 */
    u32 color;              /* 0x0C */
    u16 angle;              /* 0x10: 0 unrotated */
    u16 unk12;
} ModeIcon;

/* The gallery's work (select_gallery.c), at 0x1000 in the shared work
 * area: the screen and its step, the six moving pictures of the menu, the
 * three categories' lists and the entry page. */
typedef struct GalWork {
    s8 state;               /* 0x000: GalState */
    s8 step;                /* 0x001: the state's step */
    u8 unk2[0xA];
    s32 texHandles[16];     /* 0x00C: the common textures' handles (gTexHandles 163-178) */
    s32 settleTimer;        /* 0x04C: frames since a layout started; the category names move at 8 or 10 (Gallery_Settled) */
    s32 moveDelay[6];       /* 0x050: per picture, frames before it moves */
    s32 category;           /* 0x068: the category under the cursor, then shown */
    s32 categoryCount;      /* 0x06C: 3 */
    s32 moveState[6];       /* 0x070: per picture: 1 at rest, 2 waiting (moveDelay), 3 moving */
    s32 pictures[6][2];     /* 0x088: the six pictures' texture ids (255: none) */
    s32 drawOrder[6];       /* 0x0B8: the pictures in drawing order */
    f32 targetPos[6][2];    /* 0x0D0: the pictures' target positions */
    f32 speed[6][2];        /* 0x100: their speeds */
    f32 pos[6][2];          /* 0x130: their positions */
    u32 movePhase[6];       /* 0x160: their moves' phases (an angle up to a quarter turn: the speed eases by its sine) */
    s32 pictureCount;       /* 0x178: the pictures to draw (6) */
    s32 categoryPos[3][2];  /* 0x17C: the category names' positions */
    u8 unk194[0xC];
    s8 animTimer;           /* 0x1A0: +1 a frame on the menus */
    s8 animFrame;           /* 0x1A1: 0-2, one step each 4 frames of animTimer */
    s8 panelSlide;          /* 0x1A2: the list panel's slide: 0 none, 1 in, 2 out (Gallery_StartPanel) */
    u8 unk1A3;
    s16 panelOffset;        /* 0x1A4: its offset while it slides */
    u16 panelPhase;         /* 0x1A6: its slide's phase, to ANGLE_TURN / 4 */
    s32 iconTimer;          /* 0x1A8: +1 a frame on the category menu */
    s32 iconFrame;          /* 0x1AC: 0-2, one step each 4 frames: the column of the pictures' animation (Gallery_DrawPicture) */
    s32 entryCounts[3];     /* 0x1B0: per category, the entries listed (Gallery_BuildList) */
    s32 listTop;            /* 0x1BC: the first entry shown in the list */
    s32 listRow;            /* 0x1C0: the row of the cursor, 0-3 */
    s32 listShown;          /* 0x1C4: the list is drawn */
    u32 cursorPhase;        /* 0x1C8: the cursor's opening, an angle to ANGLE_TURN / 4 */
    s32 pageShown;          /* 0x1CC: the entry page is drawn */
    s32 fadeFrames;         /* 0x1D0: frames left of the black fade (Gallery_DrawFade) */
    s32 upArrowOfs;         /* 0x1D4: the up arrow's bounce offset */
    s32 downArrowOfs;       /* 0x1D8: the down arrow's */
    s32 upArrowBounce;      /* 0x1DC: the up arrow's bounce: 1 out, 2 back */
    s32 downArrowBounce;    /* 0x1E0: the down arrow's */
    char names[32][0x100];  /* 0x1E4: the listed entries' names */
    s8 isNew[32];           /* 0x21E4: ... and whether they are new */
    u8 categoryNew[4];      /* 0x2204: per category: something new to see */
    s32 selected;           /* 0x2208: the entry chosen, an index into entries */
    u8 entries[0x36];       /* 0x220C: the entries listed, by category (255: locked) */
    u8 entryCount;          /* 0x2242: how many */
    u8 unk2243;
    s32 night;              /* 0x2244: the time of day asked for an area: 0 day, 1 night */
    s32 askTime;            /* 0x2248: 1 while the day-or-night question is shown */
} GalWork;

/* GalWork.state: the gallery's screens. */
enum GalState {
    GAL_OPEN,                   /* 0: fades in */
    GAL_CATEGORIES,             /* 1: the category menu */
    GAL_LIST_MONSTERS,          /* 2-4: a category's list (category + 2) */
    GAL_LIST_WORLD,
    GAL_LIST_BONUS,
    GAL_ENDING,                 /* 5: the ending sequence (plsel's) */
    GAL_MOVIE,                  /* 6: a movie */
    GAL_NET_MENU,               /* 7: the card menu, on cancel */
    GAL_FADE_OUT,               /* 8 */
    GAL_DONE                    /* 9: back to gm_sub */
};

/* GalEntry.kind: what an entry opens. Seen: kinds 0 and 1 run plsel's
 * Galex_QuestTask (kind 0 is listed under the monsters, kind 1 under the
 * world), kind 2 its Galex_Task, kind 3 a movie, kind 4 the ending. */
enum GalEntryKind {
    GAL_KIND_MONSTER_QUEST,     /* 0: a monster's quest */
    GAL_KIND_WORLD_QUEST,       /* 1: a quest of the world category */
    GAL_KIND_AREA_TOUR,         /* 2: an area's tour */
    GAL_KIND_MOVIE,             /* 3: a movie (gGalleryMovieIds) */
    GAL_KIND_ENDING             /* 4: the ending */
};

#define GAL_ENTRY_COUNT 54      /* gGalleryEntries */
#define GAL_CATEGORY_COUNT 3    /* monsters, the world, bonus */
#define GAL_PICTURE_COUNT 6     /* the menu's moving pictures */
#define GAL_TEX_COUNT 0x37      /* the gallery's own textures (the galex partition) */
#define GAL_TEX_FIRST 0x2B      /* their first handle in gTexHandles */
#define GAL_TEX_LOCKED 0x61     /* the picture of a locked entry's page */
#define GAL_LIST_ROWS 4         /* the list's rows on screen */
/* gSysSettings.galleryEntry: the entry being played (its low 7 bits) and, for
 * an entry that asks, the night bit (plsel's Galex tasks read it). */
#define GAL_ENTRY_MASK 0x7F
#define GAL_ENTRY_NIGHT 0x80

/* gGalleryEntries's records: the gallery's entries. */
typedef struct GalEntry {
    char *title;            /* 0x00 */
    char **lines;           /* 0x04: the page's four description lines */
    u8 picture;             /* 0x08: the page's picture, GAL_TEX_FIRST + picture in gTexHandles */
    u8 kind;                /* 0x09: GalEntryKind */
    u8 kindIndex;           /* 0x0A: its number among the entries of its kind (a movie's index in gGalleryMovieIds) */
    u8 needsUnlock;         /* 0x0B: listed only once its event flag (the entry number) is set */
} GalEntry;

/* gGalleryArrowSprites's records: the arrows beside a category. */
typedef struct GalIcon {
    s32 tex;                /* 0x00: index into gTexHandles */
    s32 unk4;               /* not read by the gallery's code */
    s16 w, h;               /* 0x08 */
    s16 u0, v0, u1, v1;     /* 0x0C */
    u32 color;              /* 0x14 */
    s32 unk18;              /* not read */
    f32 unk1C;              /* 0x1C: not read */
} GalIcon;

/* The gallery's drawn records (Gallery_DrawRecords): the pictures'
 * layouts (gGalleryLayout), the list's frame, cursor and scroll bar. */
typedef struct GalRec {
    s32 tex;                /* 0x00: > 100: gTexHandles, else the gallery's own */
    s16 x, y, w, h;         /* 0x04 */
    s16 u0, v0, u1, v1;     /* 0x0C */
    u32 color;              /* 0x14 */
    u16 angle;              /* 0x18 */
    u8 pad1A[2];
} GalRec;

/* A filled screen rectangle (Prim_FillRect). */
typedef struct GalRect {
    s16 x, y, w, h;
    u32 color;
} GalRect;

/* The work area the units share, gSysSettings.workArea (sub_main's
 * gWorkArea): the boot's work at its start, then one screen's work at
 * offset 0x1000. */
typedef struct SelWork {
    u8 unk0[0x1000];        /* the boot's (BootWork, select_boot.c) */
    union {
        CharaWork chara;        /* 0x1000: character creation */
        CharaSelWork charaSel;  /* 0x1000: the character selection */
        ModeWork modeSel;       /* 0x1000: mode select */
        GalWork gallery;        /* 0x1000: the gallery (select_gallery.c) */
    };
} SelWork;

/* main */
s32 Sprite_Draw(Sprite *s);          /* draws a sprite */

/* sub_main: select a texture for the sprites (Gfx_SetTexFilter(1);
 * Tex_Prepare(1, tex); Tex_Bind(tex)) */
void Gfx_SetTexFilter(s32);
void Tex_Bind(s32);

/* sub_main's task slots (TaskSlot, include/task.h): a task function gets
 * its slot, with the task's state in TaskSlot.mode and its step in .step. */

void *memset(void *, s32, u32);
void *memcpy(void *, const void *, u32);

/* sub_main */
void Font_Printf(const char *fmt, ...);                    /* prints a string */

#include "chredit.h"            /* ChrEdit, gChrEdit */
#include "player.h"

/* ---- The PSP link (select_psp.c) ---- */

/* A frame of an object's animation (gPspObjAnims points to each set's
 * first frame). */
typedef struct PspAnim {
    u8 unk0;                /* 0x00: not read */
    s8 loopPoint;           /* 0x01: 1 at a loop point (type 0 counts its loops there; types 2 and 6 may jump) */
    s8 next;                /* 0x02: the next frame (< 0: the end) */
    s8 se;                  /* 0x03: a sound (1 + its number), 0 none */
    s16 wait;               /* 0x04: the frame's time, in frames */
    s16 ox;                 /* 0x06: drawing offset */
    s16 oy;                 /* 0x08 */
    s16 dx;                 /* 0x0A: the step applied at the frame's start */
    s16 dy;                 /* 0x0C */
    s16 tex;                /* 0x0E: the file (PspWork.tex) */
    s16 cell;               /* 0x10 */
    u8 pad12[2];
    s16 *cells;             /* 0x14: u, v, texture w, h, screen w, h */
} PspAnim;

/* An animated object of the link's show (32 in PspWork). */
typedef struct PspObj {
    s32 active;             /* 0x00 */
    s32 wait;               /* 0x04: frames left of the current frame */
    s32 count;              /* 0x08: type 0: its loops; sound 10: its steps */
    s32 flip;               /* 0x0C: mirrored, walking right to left */
    s32 frame;              /* 0x10: the index of the current frame in anim */
    s32 x;                  /* 0x14 */
    s32 y;                  /* 0x18 */
    s32 unk1C;              /* 0x1C: only cleared (Psp_SetAnim) */
    s32 unk20;              /* 0x20: only cleared */
    s32 endX;               /* 0x24: past it the object ends (gPspObjParams) */
    s32 type;               /* 0x28: PSP_OBJ_* (its walk and gPspObjAnims set) */
    PspAnim *anim;          /* 0x2C */
} PspObj;

/* A part an actor draws (gPspActorAnims, 16 bytes each). */
typedef struct PspPart {
    s16 tex;                /* 0x0: the file (PspWork.tex) */
    s8 kind;                /* 0x2: 0 a filled rectangle, else a sprite */
    s8 blend;               /* 0x3: 0: the texture filtered */
    u32 color;              /* 0x4 */
    s16 u;                  /* 0x8: texture rectangle */
    s16 v;                  /* 0xA */
    s16 uw;                 /* 0xC */
    s16 vh;                 /* 0xE */
} PspPart;

/* An actor of the link's screens (32 in PspWork; PSP_ACTOR_* below). */
typedef struct PspActor {
    s32 exists;             /* 0x00: set up by Psp_InitActors; not drawn while 0 */
    s32 visible;            /* 0x04: in the frame's draw list */
    s32 kind;               /* 0x08: a frame's index in gPspFrameRects; a flying screen's direction (0 left) */
    s8 state;               /* 0x0C: PSP_ACTOR_INIT ... */
    u8 unkD[0x20 - 0xD];
    f32 speed;              /* 0x20: added to x each frame */
    u8 unk24[0x30 - 0x24];
    f32 x;                  /* 0x30 */
    f32 y;                  /* 0x34 */
    s32 flipX;              /* 0x38: mirrored horizontally */
    s32 flipY;              /* 0x3C: mirrored vertically */
    s16 w;                  /* 0x40 */
    s16 h;                  /* 0x42 */
    u8 unk44[4];
    s32 large;              /* 0x48: an icon screen drawn larger (Psp_MarkIcons) */
    f32 scaleX;             /* 0x4C */
    f32 scaleY;             /* 0x50 */
    s32 dim;                /* 0x54: drawn at half brightness */
    PspPart *part;          /* 0x58 */
    s32 relative;           /* 0x5C: placed relative to its parent */
    struct PspActor *parent; /* 0x60: always actor 1 */
} PspActor;

/* A filled rectangle (Prim_FillRect). */
typedef struct PspRect {
    s16 x0, y0, x1, y1;     /* 0x0 */
    u32 color;              /* 0x8 */
} PspRect;

/* A textured triangle (Prim_DrawTexTri, main). */
typedef struct PspTri {
    s16 x0, y0, x1, y1, x2, y2;     /* 0x00 */
    u32 color;                      /* 0x0C */
    s16 u0, v0, u1, v1, u2, v2;     /* 0x10 */
} PspTri;

/* A file's place in the link's archive (Psp_LoadFiles). */
typedef struct PspFileEnt {
    s32 offset;
    s32 size;
} PspFileEnt;

/* Psp_ConnectTask's modes (TaskSlot.mode). */
enum PspMode {
    PSP_MODE_START,             /* 0: starts the character selection */
    PSP_MODE_WAIT_CHARA,        /* 1: waits for its end, loads the sounds and textures */
    PSP_MODE_INIT,              /* 2: Psp_Init, the fade in */
    PSP_MODE_PROMPT,            /* 3: asks to connect: confirm starts the thread, cancel leaves */
    PSP_MODE_CONNECT,           /* 4: runs PspWork.step */
    PSP_MODE_EXIT_TO_MENU,      /* 5: after the fade: back to gm_sub's menu */
    PSP_MODE_FADE_DONE,         /* 6: after the fade, on to PSP_MODE_EXIT */
    PSP_MODE_NET_MENU,          /* 7: the card menu (not entered by this file) */
    PSP_MODE_EXIT,              /* 8: leaves for gm_sub, keeping the character */
    PSP_MODE_CANCELLED          /* 9: the cancelled message, then PSP_MODE_EXIT_TO_MENU */
};

/* PspWork.step, in PSP_MODE_CONNECT. Steps 2 and 3 only pass on. */
enum PspStep {
    PSP_STEP_WAIT_PSP = 0,      /* polls the device until a PSP answers */
    PSP_STEP_EXCHANGE = 1,      /* waits for the thread's exchange */
    PSP_STEP_DONE = 4,          /* shuts the link down, picks the features */
    PSP_STEP_ERROR = 5,         /* an error or cancel message; confirm goes back to the prompt */
    PSP_STEP_RESULT = 10,       /* what came: nothing leaves, else the show */
    PSP_STEP_SHOW = 11          /* runs PspWork.subStep */
};

/* PspWork.subStep, in PSP_STEP_SHOW. */
enum PspShowStep {
    PSP_SHOW_SPAWN,             /* 0: spawns the queued objects, one each 90 frames */
    PSP_SHOW_WAIT,              /* 1: until the objects are gone (or cancel) */
    PSP_SHOW_ICONS              /* 2: the icons and their cursor */
};

/* gPspState: the link thread's state (Psp_Thread). */
enum PspThreadState {
    PSP_THREAD_IDLE,            /* 0 */
    PSP_THREAD_EXCHANGE,        /* 1: gets the device path, opens it, sends sendBuf, reads recvBuf */
    PSP_THREAD_DONE,            /* 2 */
    PSP_THREAD_STOP,            /* 3: set by Psp_Shutdown */
    PSP_THREAD_CLOSE,           /* 4: closes the device, then idle (not set by this file) */
    PSP_THREAD_ERROR            /* 5 */
};

/* PspWork.textStep: the lines Psp_DrawTexts shows (gPspStepTexts). */
#define PSP_TEXT_NONE 0
#define PSP_TEXT_CONNECTING 2
#define PSP_TEXT_EXCHANGED 4
#define PSP_TEXT_PROMPT 6
#define PSP_TEXT_LINK_ERROR 7
#define PSP_TEXT_EXCHANGE_CANCELLED 8
#define PSP_TEXT_SHOW 9
#define PSP_TEXT_ICONS 10
#define PSP_TEXT_CANCELLED 13   /* reads past gPspStepTexts into gPspLines (said in Psp_DrawTexts) */

/* PspActor.state. */
enum PspActorState {
    PSP_ACTOR_INIT,             /* 0: takes its rectangle */
    PSP_ACTOR_WAIT,             /* 1 */
    PSP_ACTOR_RUN,              /* 2: slides, flies or dims */
    PSP_ACTOR_DONE              /* 3 */
};

/* The device's commands (sceDevctl): 'UPC' and a number. */
#define PSP_DEVCTL_PRESENT 0x55504301   /* whether a PSP is there (into PspWork.pspPresent) */
#define PSP_DEVCTL_GET_PATH 0x55504302  /* the path to open */

#define PSP_ACTOR_COUNT 32
#define PSP_ACTORS_USED 22      /* PspWork.actorCount */
#define PSP_OBJ_COUNT 32
#define PSP_ICON_COUNT 6
#define PSP_FILE_COUNT 13
#define PSP_TEX_FIRST 0xA3      /* the files' first handle in gTexHandles */
#define PSP_BUF_SIZE 0x40       /* each exchange buffer */

/* The link's work: 5264 bytes at offset 0x100 in the select work area
 * (gSysSettings.selWork; gPspWork). */
typedef struct PspWork {
    s32 step;               /* 0x000: PspStep, while the task's mode is PSP_MODE_CONNECT */
    s32 subStep;            /* 0x004: PspShowStep, in PSP_STEP_SHOW */
    u8 unk8[8];
    s32 modId;              /* 0x010: the USB module's id (-1 none) */
    u8 unk14[8];
    s32 fd;                 /* 0x01C: the device, open (-1 closed) */
    s32 pspPresent;         /* 0x020: the PSP's answer to the first devctl (0: none yet) */
    s32 *recvBuf;           /* 0x024: PSP_BUF_SIZE bytes read from the PSP; word 0: the features */
    s32 *sendBuf;           /* 0x028: PSP_BUF_SIZE bytes sent: the items unlocked (Psp_InitItemMask) */
    s32 *spareBuf;          /* 0x02C: a third buffer, only cleared */
    s32 timer;              /* 0x030: frames left before the next action */
    u8 unk34[0x40 - 0x34];
    s32 offerMask;          /* 0x040: the features offered this time (bits) */
    s32 unk44;              /* 0x044: only cleared */
    u8 unk48[0x54 - 0x48];
    s32 headerKind;         /* 0x054: Sel_DrawHeader's kind (0: none) */
    s32 textStep;           /* 0x058: the text lines shown (gPspStepTexts) */
    u8 unk5C[0xA4 - 0x5C];
    u8 featureOn[16];       /* 0x0A4: per feature: offered */
    u8 menuObjType[16];     /* 0x0B4: per menu: the object type its show spawns (0 none) */
    u8 spawnQueue[16];      /* 0x0C4: a copy of menuObjType, emptied by the show */
    s32 spawnLeft;          /* 0x0D4: the queue's entries left */
    s8 iconEntry[6];        /* 0x0D8: per icon: the entry of its list found */
    s8 iconFound[6];        /* 0x0DE: per icon: one found */
    s8 cursorIcon;          /* 0x0E4: the icon under the cursor (-1 none) */
    s8 cameFrom2;           /* 0x0E5: the icon the cursor left for the tall icon 2 */
    s8 cameFrom4;           /* 0x0E6: the same for the tall icon 4 */
    s8 cursorCol;           /* 0x0E7: its column in gPspCursorGrid */
    s8 cursorRow;           /* 0x0E8: its row */
    u8 unkE9[0xFC - 0xE9];
    s32 hideFrames;         /* 0x0FC: the window frames are drawn while 0 (never set) */
    s32 tex[PSP_FILE_COUNT]; /* 0x100: the loaded files' texture handles */
    s32 sceneState;         /* 0x134: 0 the opening, 1 the icons */
    PspActor actors[PSP_ACTOR_COUNT]; /* 0x138 */
    s32 actorCount;         /* 0xDB8: actors updated */
    PspActor *drawList[PSP_ACTOR_COUNT]; /* 0xDBC: this frame's visible actors */
    PspObj objs[PSP_OBJ_COUNT]; /* 0xE3C */
    s32 swingAmp[6];        /* 0x143C: per icon: its swing's amplitude, decaying to 0 */
    u16 swingPhase[6];      /* 0x1454 */
    u16 swingSpeed[6];      /* 0x1460: added to the phase each frame */
    u16 swingDecay[6];      /* 0x146C: taken from the amplitude each time the phase wraps */
    s16 centerX[6];         /* 0x1478: the icon screens' centres */
    s16 centerY[6];         /* 0x1484 */
} PspWork;

void Chara_SetupEditPlayer(Player *pl, s32 mode, s32 slot);
void Chara_SetupCardPlayer(Player *pl, s32 mode, s16 slot);
void Chara_PlayCancelSe(void);
void Chara_DrawNoMatch(ChrEdit *edit);
void Chara_DrawQuestion(ChrEdit *edit, s16 mode);
void Chara_SetCamera(Player *pl, s32 mode, s32 hold);
void Chara_CreateTask(TaskSlot *task);
void Chara_DrawEquipment(ChrEdit *unused, s32 mode);
void Chara_SelectTask(TaskSlot *task);
void Chara_Frame(TaskSlot *task);
s32 Chara_CheckName(s8 *name);
void Boot_Task(TaskSlot *task);
void Boot_Update(TaskSlot *task);
void Boot_ToTitle(void);
void Boot_DrawTitleScreen(s32 alpha);
void Boot_UpdateMovie(void);
void Boot_UpdateScreens(void);
void Select_BeginTaskFrame(void);
void Select_LoadSoundBanks(void);
s32 Select_CheckPad(void);
s32 Select_IsPadReady(void);
void Select_DrawPadWarning(void);
void Select_DrawContentWarning(void);
void Select_DrawSplash(void);
s32 Select_ApplyBootArgs(void);
void Option_Task(TaskSlot *task);
s32 Option_MainInput(u16 trig, u16 rep);
void Option_DrawMainPage(void);
void Option_ControlInput(u16 trig, u16 rep);
void Option_DrawControlPage(void);
void Option_ScreenPosInput(u16 trig, u16 rep);
void Option_DrawScreenPosPage(void);
void Option_BrightnessInput(u16 trig, u16 rep);
void Option_Update(TaskSlot *task);
void Option_Start(TaskSlot *task, u16 net);
void Option_Exit(void *task, s32 net);
void NetOption_Task(TaskSlot *task);
void NetOption_Update(TaskSlot *task, s32 unused);
s32 NetOption_Input(u16 trig, u16 rep);
void NetOption_DrawPage(void);
void NetOption_ConfirmInput(u16 pad, u16 rep);
void NetOption_DrawNetMenuPage(void);
void NetOption_DrawConfirm(void);
void Option_DrawBrightnessStrip(void);
void Gallery_LoadSounds(void);
void Gallery_Task(TaskSlot *task);
void Gallery_LoadTextures(void);
void Gallery_Init(TaskSlot *task);
s32 Gallery_Update(TaskSlot *task);
void Gallery_UpdateNewFlags(void);
s32 Gallery_BuildList(s32 cat);
void Gallery_Exit(TaskSlot *task);
void Gallery_GetTextures(GalWork *gal);
void Gallery_Draw(void);
void Gallery_DrawFade(void);
void Gallery_DrawCategories(GalWork *gal);
void Gallery_DrawScrollBar(GalWork *gal);
void Gallery_DrawList(GalWork *gal);
void Gallery_DrawEntryPage(GalWork *gal);
void Gallery_DrawPicture(s32 *ids, f32 *pos);
void Gallery_DrawRecords(GalRec *rec, s32 n);
void Gallery_UpdatePictures(void);
void Gallery_SetLayout(s32 k);
void Gallery_SetLayoutBack(s32 unused);
s32 Gallery_PictureMoving(s32 i);
s32 Gallery_Settled(void);
void Gallery_FillNames(GalWork *gal);
s32 Gallery_ListInput(GalWork *gal, u16 pad);
void Gallery_PlayDecideSe(void);
void Gallery_PlayCancelSe(void);
void Gallery_PlayPageSe(void);
void Gallery_DrawArrow(s32 k);
void Gallery_StartPanel(GalWork *gal, s32 back);
void Gallery_ResetPanel(GalWork *gal);
void Psp_ConnectTask(TaskSlot *task);
void Psp_Init(void);
void Psp_Release(void);
void Psp_InitItemMask(void);
void Psp_StartThread(PspWork *work);
void Psp_Shutdown(PspWork *work);
void Psp_SelectFeatures(void);
void Psp_LoadFiles(s32 *out);
void Psp_FreeTextures(void);
void Psp_InitActors(void);
void Psp_MarkIcons(void);
void Psp_ResetSceneActors(void);
void Psp_UpdateObj(PspObj *obj);
void Psp_DrawObject(PspObj *obj);
s32 Psp_AnyObjectActive(void);
void Psp_SpawnObject(s32 type, s32 flip);
void Psp_SpawnChild(PspObj *src, s32 type, s32 x, s32 y);
void Psp_UpdateActor(s32 i, PspActor *actor);
void Psp_UpdateScene(void);
void Psp_DrawActor(PspActor *actor);
void Psp_DrawSwingActor(PspActor *actor, s32 icon);
void Psp_DrawSwingFrame(PspActor *actor, s32 icon);
void Psp_Draw(void);
void Psp_DrawTexts(void);
void Psp_Thread(void *arg);
s32 Psp_IsItemUnlocked(u16 id);
extern char *gNetOptValueNames_0[4];
extern char *gNetOptValueNames_1[4];

s32 NgWord_Check(s8 *name);
void NgWord_Normalize(s8 *dst, s8 *src, s32 n);
s32 NgWord_MatchAt(s8 *text);
s32 NgWord_StartsWithListed(s8 *name, NgWordRec *list);
s32 NgWord_ReadRecord(NgWordRec *rec, s8 *out, s32 max);
void Parts_Init(void);
s32 Parts_DrawSides(TaskSlot *task);
void Parts_SlideInPanels(u8 sel);
void Parts_SlideOutPanels(u8 sel);
void Parts_StartBounce(u8 entry);
void Parts_ResetBounces(void);
void Parts_StartPanel(s16 toPanel);
void Parts_Animate(void);
s32 Parts_DrawPanels(TaskSlot *task);
void Parts_DrawRotated(s16 x, s16 y, s16 width, s16 height, s32 deg);
void Parts_DrawMarker(s16 x, s16 y, s16 width, s16 height, s32 angle, s32 color, u8 entry);
void Parts_DrawTypes(s16 x, s16 y, s32 sel);
void Parts_DrawFacePanel(s16 x, s16 y, s32 sel);
void Parts_DrawHairPanel(s16 x, s16 y, s32 sel);
void Parts_DrawVoicePanel(s16 x, s16 y, s32 sel);

#endif /* SELECT_H */
