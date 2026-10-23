#ifndef HOUSE_PROPS_H
#define HOUSE_PROPS_H

#include "common.h"
#include "sprite.h"

/*
 * The online house's props (HouseProp), a sub_main object with draw slots
 * of its own: on the first visit to the online house at house level 1 or 2,
 * HouseProp_Spawn raises the props, which stand, tip over and sink into the
 * floor. HouseProp_Draw draws them and HouseProp_Free frees them.
 */

/* the props of lobby AREA_ONLINE_HOUSE_1 and AREA_ONLINE_HOUSE_2 (a sub_main object, Obj_Alloc(0);
 * HouseProp_Run by state; a view of the Obj): they stand 120 frames, tip over (tilt to
 * -ANGLE_TURN / 4, faster each frame), raise effects at their places, then sink
 * (HouseProp_Draw draws them) */
typedef struct HouseProp {
    /* 0x00 */ u8 active;           /* Obj.active */
    /* 0x01 */ u8 visible;          /* Obj.visible: 1 while it stands or sinks */
    /* 0x02 */ u8 kind;             /* Obj.kind: 15 (HouseProp_Spawn) */
    /* 0x03 */ u8 type;             /* Obj.type */
    /* 0x04 */ u8 state;            /* Obj.state: HouseProp_Run's switch */
    /* 0x05 */ u8 phase;            /* Obj.var0: 0 standing, 1 tipping, 2 down */
    /* 0x06 */ u8 pad6[2];
    /* 0x08 */ s16 timer;           /* Obj.timer */
    /* 0x0A */ s16 tilt;            /* Obj.var3: ANGLE_TURN a turn */
    /* 0x0C */ u8 padC[8];
    /* 0x14 */ void *draw;          /* Obj.draw: NULL (it draws through its slot) */
    /* 0x18 */ u8 pad18[8];
    /* 0x20 */ f32 pos[3];          /* Obj.pos */
    /* 0x2C */ u8 pad2C[12];
    /* 0x38 */ DrawSlot *ds;        /* Obj.ds */
    /* 0x3C */ s16 slot;            /* Obj.slot */
    /* 0x3E */ u8 pad3E[2];
} HouseProp;

typedef char HouseProp_size_check[(sizeof(HouseProp) == 64) ? 1 : -1]; /* an Obj */

void HouseProp_Spawn(void);
void HouseProp_Run(HouseProp *prop);
void HouseProp_Init(HouseProp *prop);
void HouseProp_Update(HouseProp *prop);
void HouseProp_End(HouseProp *prop);
void HouseProp_Free(void *prop);
void HouseProp_Draw(DrawSlot *slot);

#endif
