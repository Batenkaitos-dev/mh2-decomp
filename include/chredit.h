#ifndef CHREDIT_H
#define CHREDIT_H

#include "common.h"

/* The character being created (sub_main's gChrEdit, 0x60 bytes; set up by select_entry, drawn by
 * select_parts.c's panels; select_chara.c's pages edit it). */
typedef struct ChrEdit {
    u8 unk00;
    u8 slot;                /* 0x01: the character slot (gSysSave's, 0x37F8 bytes each; sub_main's card menus choose it) */
    u8 panel;               /* 0x02: the chosen panel (0 name, 1 type, 2 face, 3 hair, 4 colour, 5 voice; gCharaPartNames) */
    u8 choice;              /* 0x03: the answer under the cursor of a yes/no question (0 yes, 1 no); the colour editor's row (CHARA_COLOR_ROW_*) */
    u8 gender;              /* 0x04: 0 or 1: SaveData.gender and Player.gender (Chara_MakeSave), the voice bank (Sound_LoadVoiceBank); a row of gCharaStepConds and of the panels' condition labels */
    u8 face;                /* 0x05 */
    u8 voice;               /* 0x06: 15 voices (the voice page) */
    u8 hair;                /* 0x07: 18 hair styles (the hair page) */
    u32 color;              /* 0x08: 0xRRGGBB */
    f32 camTarget[3];       /* 0x0C: the camera's target */
    f32 camPos[3];          /* 0x18: the camera's position */
    s8 name[0x12];          /* 0x24 */
    u16 frameCount;         /* 0x36: +1 a frame while the creation or selection task runs (not read) */
    u16 timer;              /* 0x38: the pages' frame counter (to 0x3C, from 150 ...; the help's button blinks by timer / 15) */
    u8 presetColor;         /* 0x3A: the preset colour under the cursor (gPresetColors, 16) */
    u8 sex;                 /* 0x3B: not the sex: the save menu's result after creation, 1 saved, 0 not (Chara_CreateTask); picks the message after it (Chara_DrawSaveMessage) */
    u8 sound;               /* 0x3C: the next of the 12 sounds the voice page plays (Sound_PlaySeFlat(2, sound)) */
    u8 soundWait;           /* 0x3D: frames before the next sound may play (5, counted down) */
    u16 padHoldRepeat;      /* 0x3E: the buttons newly pressed, and those held unchanged for 10 frames (the colour sliders' fast step) */
    u16 holdFrames;         /* 0x40: frames the held buttons have not changed, to 10 */
    u8 equipView;           /* 0x42: gSysSave.options.equipView, copied when the selection starts (Chara_EquipViewOn) */
    s8 inputDelay;          /* 0x43: frames left during which the pad is ignored (Chara_CreateTask) */
    u8 unk44[3];
    u8 type;                /* 0x47: the type panel's choice, 0-3 (Parts_DrawTypes) */
    u16 flags;              /* 0x48: the buttons newly pressed this frame (gPadState[0].pressed), 0 while input is ignored */
    u16 padPrevHeld;        /* 0x4A: gPadState[0].prevHeld */
    u16 padHeld;            /* 0x4C: gPadState[0].held */
    u16 padRepeat;          /* 0x4E: gPadState[0].repeat */
    u8 cond[8];             /* 0x50: the face, hair and voice pages' filter conditions (3, 3 and 2 rows; 0 any) */
    s32 mask;               /* 0x58: the faces, hair styles or voices that fit the conditions, a bit each (Chara_FilterParts) */
    u8 loadedGender;        /* 0x5C: the gender whose body model is loaded, 0xFF none */
    u8 cursor;              /* 0x5D: the face, hair and voice panels' row under the cursor (0: the number, then the conditions, the last: confirm) */
    s8 matchCount;          /* 0x5E: the count of mask's bits */
    u8 noMatch;             /* 0x5F: nonzero while the no-match message is shown */
} ChrEdit;
typedef char ChrEdit_size_check[(sizeof(ChrEdit) == 0x60) ? 1 : -1];
extern ChrEdit gChrEdit;

#endif /* CHREDIT_H */
