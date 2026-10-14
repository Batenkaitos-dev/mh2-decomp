#ifndef CHREDIT_H
#define CHREDIT_H

#include "common.h"

/* The character being created (sub_main's gChrEdit, 0x60 bytes; set up by select_entry, drawn by
 * select_parts.c's panels; select_chara.c's pages edit it). */
typedef struct ChrEdit {
    u8 unk00;
    u8 slot;                /* 0x01: the character slot (gSysSave's, 0x37F8 bytes each; sub_main's card menus choose it) */
    u8 panel;               /* 0x02: the chosen panel (0 name, 1 type, 2 face, 3 hair, 4 colour, 5 voice; gCharaPartNames) */
    u8 unk3;                /* 0x03 */
    u8 gender;              /* 0x04: 0 or 1: SaveData.gender and Player.gender (Chara_MakeSave), the voice bank (Sound_LoadVoiceBank); a row of gCharaStepConds and of the panels' condition labels */
    u8 face;                /* 0x05 */
    u8 voice;               /* 0x06: 15 voices (VOICE page) */
    u8 hair;                /* 0x07: 18 hair styles (HAIR page) */
    u32 color;              /* 0x08: 0xRRGGBB */
    f32 camTarget[3];       /* 0x0C: the camera's target */
    f32 camPos[3];          /* 0x18: the camera's position */
    s8 name[0x12];          /* 0x24 */
    u16 unk36;
    u16 timer;              /* 0x38: the pages' frame counter (to 0x3C, from 150 ...; the help's button blinks by timer / 15) */
    u8 unk3A;               /* 0x3A */
    u8 sex;                 /* 0x3B */
    u8 sound;               /* 0x3C: the next of the 12 sounds the VOICE page plays (Sound_PlaySeFlat(2, sound)) */
    u8 soundWait;           /* 0x3D: frames before the next sound may play (5, counted down) */
    u16 unk3E;              /* 0x3E */
    u16 unk40;              /* 0x40 */
    u8 unk42;
    s8 unk43;               /* 0x43 */
    u8 unk44[3];
    u8 type;                /* 0x47: the TYPE panel's choice (TYPE01 to 04: Parts_DrawTypes) */
    u16 flags;              /* 0x48 */
    u16 unk4A;              /* 0x4A */
    u16 unk4C;              /* 0x4C */
    u16 unk4E;              /* 0x4E: pad bits */
    u8 cond[8];             /* 0x50: the FACE/HAIR/VOICE pages' filter conditions (3, 3 and 2 rows) */
    s32 mask;               /* 0x58 */
    u8 unk5C;
    u8 cursor;              /* 0x5D: the FACE/HAIR/VOICE panels' row under the cursor (0: the number, 1 to 4: the rows) */
    s8 matchCount;          /* 0x5E: the faces, hair styles or voices that fit the conditions ("NN(MM)") */
    u8 noMatch;             /* 0x5F: nonzero: "nothing matches" is shown */
} ChrEdit;
typedef char ChrEdit_size_check[(sizeof(ChrEdit) == 0x60) ? 1 : -1];
extern ChrEdit gChrEdit;

#endif /* CHREDIT_H */
