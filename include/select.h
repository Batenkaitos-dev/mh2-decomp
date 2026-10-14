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
    s16 slideOut; /* 0x008: 0: the panels slide in (Chara_StartTabAnim), 1: out (Chara_StartTabAnim2); Parts_Animate. The character selection's slot (CharaSel_DrawSlots, CharaSel_Leave) */
    u8 unkA[0x4E];
    s32 slide[7]; /* 0x058: per panel, its slide step (x offset 80 per step, func_006BDA00): counted down to 0 when sliding in, up to 10 when out (Parts_Animate); [0], [1] and [4] the side panels' timers and shared delay (Parts_Init, func_006BD050) */
    s32 sliding[7]; /* 0x074: per panel, 1 while it slides (Parts_Animate clears it) */
    u8 unk90[0x7C];
    f32 unk10C[6];          /* 0x10C: only CharaSel_Leave (the CharaSelWork view of 0x108) */
    u8 unk124[0x3C];
    s32 slideDelay[7]; /* 0x160: per panel, frames before it slides out (Chara_StartTabAnim2: 0, 2, 4...) */
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

/* The work area the units share, *(SelWork **)0x6AE01C (sub_main's). */
typedef struct SelWork {
    u8 unk0[0x1000];        /* the boot sequence's (BootWork, select_boot.c) */
    CharaWork chara;        /* 0x1000 */
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
void SeqA_Start(s32);
void Font_SetSpacing(u8 a, u8 b);                   /* text size */
void Font_SetPos(s16, s16);                  /* text position */
void Font_SetColor(s32 color);                       /* text colour */
void Font_Printf(const char *fmt, ...);                    /* prints a string */
void Text_DrawShadow2(s16, s16, s32, s32, char *);  /* draws a message */

#include "chredit.h"            /* ChrEdit, gChrEdit */

#endif /* SELECT_H */
