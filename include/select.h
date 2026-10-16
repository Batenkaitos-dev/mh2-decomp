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

void Sel_DrawParts(SelPart *p, s32 n, u32 color);   /* select_title.c: draws n parts */

/* Character creation's work, at 0x1000 in the shared work area. */
typedef struct CharaWork {
    union { s8 state; s8 sideState[2]; }; /* 0x000: func_006BD050 runs the two side panels' slide-in states (0-7) in bytes 0 and 1 (as ((s8 *)c)[k]); the character selection's state (CharaSel_Leave sets 2) */
    u8 unk2[2];
    s32 unk4;               /* 0x004 */
    s16 slideOut; /* 0x008: 0: the panels slide in (Parts_SlideInPanels), 1: out (Parts_SlideOutPanels); Parts_Animate. The character selection's slot (CharaSel_DrawSlots, CharaSel_Leave) */
    u8 unkA[0x4E];
    s32 slide[7]; /* 0x058: per panel, its slide step (x offset 80 per step, func_006BDA00): counted down to 0 when sliding in, up to 10 when out (Parts_Animate); [0], [1] and [4] the side panels' timers and shared delay (Parts_Init, func_006BD050) */
    s32 sliding[7]; /* 0x074: per panel, 1 while it slides (Parts_Animate clears it) */
    u8 unk90[0x7C];
    f32 unk10C[6];          /* 0x10C: only CharaSel_Leave (the CharaSelWork view of 0x108) */
    u8 unk124[0x3C];
    s32 slideDelay[7]; /* 0x160: per panel, frames before it slides out (Parts_SlideOutPanels: 0, 2, 4...) */
    u8 unk17C[4];
    s32 blink; /* 0x180: 0-2, one step each 4 frames (Parts_Animate): the cursor's frame (Parts_DrawCursor, CharaSel_DrawSlots) */
    s8 bounce[10]; /* 0x184: per list entry, its bounce: 0 none, 1 out, 2 back (Parts_StartBounce, Parts_Animate); its marker is lit while nonzero (Parts_DrawMarker) */
    s8 bounceOfs[10]; /* 0x18E: per list entry, the bounce offset (to +-5) */
    s16 cursorY; /* 0x198: the cursor's y while it jumps (Parts_DrawCursor; Parts_StartPanel sets the start) */
    s16 cursorYEnd; /* 0x19A: where the jump ends (Parts_StartPanel) */
    u8 unk19C[2];
    u8 cursorJump; /* 0x19E: the cursor's jump: 0 none, 1 up (-0x1C a frame, 12 frames), 2 down (+0x24 to cursorYEnd); Parts_StartPanel sets 1 */
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
#define CHARA_HAIR_SPECIAL 0x11 /* the last hair style: offered only when gSysSave.unk458 is set; saved as style 0x13 */
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
    CHARA_ASK_NET_MESSAGE       /* 5: the card menu's message 0x30 */
};

/* Chara_SetCamera's views. */
enum CharaCamera {
    CHARA_CAM_BODY,             /* 0: the whole body, from the front left */
    CHARA_CAM_FACE,             /* 1: close on the head (bone 0x14) */
    CHARA_CAM_FRONT             /* 2: the body from the front */
};

#define CHARA_SLOT_COUNT 3      /* the characters on a memory card */
#define CHARA_SLOT_NONE 0xFF    /* ChrEdit.slot: none (Chara_MakeSave: write gSave itself) */
#define CHARA_GENDER_NONE 0xFF  /* ChrEdit.loadedGender: no body model loaded */
#define CHARA_NAME_LEN 0x12     /* ChrEdit.name and SaveData.name */

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

/* gm_sub's work (gGmWork, gm_main.c's GmWork) as the title menu's drawing
 * reads it (select_title.c). */
typedef struct TitleBg {
    u8 unk0[0x20];
    s32 unk20;              /* 0x20: gm_main sets 1; nonzero: the logo at its full size */
    s32 menuSel;            /* 0x24: the title menu's entry under the cursor */
    s32 sel;                /* 0x28: mode select's entry */
    u8 unk2C[0x10];
    s32 frame;              /* 0x3C: 0-3, +1 a frame (Title_StepBgAnim) */
    s32 count;              /* 0x40: +1 each 4 frames: animates the chosen entry's number */
    u8 unk44[0xC];
    s32 logoAnim;           /* 0x50: the logo's state: 0 at full size, 1-3 its scale steps (gTitleLogoScales) */
    u32 menuSelColor;       /* 0x54: the chosen entry's text colour */
    u32 menuColor;          /* 0x58: the other entries' */
    f32 logoScale;          /* 0x5C */
    s32 logoX;              /* 0x60: the logo's second part: position, angle and colour (Title_RandomView) */
    s32 logoY;              /* 0x64 */
    s32 logoAngle;          /* 0x68: 0x10000 a turn */
    s32 logoColor;          /* 0x6C */
} TitleBg;

/* An entry of the character selection or of mode select: its two pictures
 * (0xFF: none), indices into gCharaSelEntryIcons or gModeSelIcons. */
typedef struct SelItem {
    s32 kind;               /* 0x00: the first picture; 0xFF: the entry is empty */
    s32 pic2;               /* 0x04: the second picture */
} SelItem;

typedef struct CharaSelPos {
    f32 x, y;
} CharaSelPos;

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
    u32 slidePhase[2];      /* 0x114: ... their phases (0x10000 a turn) */
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
    s32 texHandles[16];     /* 0x00C: the common textures' handles (gTexHandles 0xA3-0xB2) */
    s32 settleTimer;        /* 0x04C: frames since a layout started; the category names move at 8 or 10 (Gallery_Settled) */
    s32 moveDelay[6];       /* 0x050: per picture, frames before it moves */
    s32 category;           /* 0x068: the category under the cursor, then shown */
    s32 categoryCount;      /* 0x06C: 3 */
    s32 moveState[6];       /* 0x070: per picture: 1 at rest, 2 waiting (moveDelay), 3 moving */
    s32 pictures[6][2];     /* 0x088: the six pictures' texture ids (0xFF: none) */
    s32 drawOrder[6];       /* 0x0B8: the pictures in drawing order */
    f32 targetPos[6][2];    /* 0x0D0: the pictures' target positions */
    f32 speed[6][2];        /* 0x100: their speeds */
    f32 pos[6][2];          /* 0x130: their positions */
    u32 movePhase[6];       /* 0x160: their moves' phases (an angle to 0x4000: the speed eases by its sine) */
    s32 pictureCount;       /* 0x178: the pictures to draw (6) */
    s32 categoryPos[3][2];  /* 0x17C: the category names' positions */
    u8 unk194[0xC];
    s8 animTimer;           /* 0x1A0: +1 a frame on the menus */
    s8 animFrame;           /* 0x1A1: 0-2, one step each 4 frames of animTimer */
    s8 panelSlide;          /* 0x1A2: the list panel's slide: 0 none, 1 in, 2 out (Gallery_StartPanel) */
    u8 unk1A3;
    s16 panelOffset;        /* 0x1A4: its offset while it slides */
    u16 panelPhase;         /* 0x1A6: its slide's phase, to 0x4000 */
    s32 iconTimer;          /* 0x1A8: +1 a frame on the category menu */
    s32 iconFrame;          /* 0x1AC: 0-2, one step each 4 frames: the column of the pictures' animation (Gallery_DrawPicture) */
    s32 entryCounts[3];     /* 0x1B0: per category, the entries listed (Gallery_BuildList) */
    s32 listTop;            /* 0x1BC: the first entry shown in the list */
    s32 listRow;            /* 0x1C0: the row of the cursor, 0-3 */
    s32 listShown;          /* 0x1C4: the list is drawn */
    u32 cursorPhase;        /* 0x1C8: the cursor's opening, an angle to 0x4000 */
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
    u8 entries[0x36];       /* 0x220C: the entries listed, by category (0xFF: locked) */
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

/* The work area the units share, gSysSettings.workArea (sub_main's
 * gWorkArea): the boot's work at its start, then one screen's work at
 * 0x1000. */
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
s32 Gs_SetState(u8, u32);          /* (0x6C, 0) before drawing, (0x6C, 1) after */
s32 Sprite_Draw(Sprite *s);          /* draws a sprite */
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);  /* a sound effect */

/* sub_main: select a texture for the sprites (Gfx_SetState63(1);
 * Tex_Prepare(1, tex); Tex_Bind(tex)) */
void Gfx_SetState63(s32);
void Tex_Prepare(s32, s32);
void Tex_Bind(s32);

/* sub_main's task slots (TaskSlot, include/task.h): a task function gets
 * its slot; +8 is the task's state, +9 the state's step. */

void *memset(void *, s32, u32);
void *memcpy(void *, void *, u32);

/* sub_main */
void TaskSlot_Set(void *func, s16 slot);     /* starts a task in a slot */
void TaskSlot_End(TaskSlot *t);              /* ends a task */
TaskSlot *TaskSlot_Get(s16 slot);            /* a slot */
void FadeA_Start(s32);
void Font_SetSpacing(u8 a, u8 b);                   /* text size */
void Font_SetPos(s16, s16);                  /* text position */
void Font_SetColor(s32 color);                       /* text colour */
void Font_Printf(const char *fmt, ...);                    /* prints a string */
void Text_DrawShadow2(s16, s16, s32, s32, char *);  /* draws a message */

#include "chredit.h"            /* ChrEdit, gChrEdit */

#endif /* SELECT_H */
