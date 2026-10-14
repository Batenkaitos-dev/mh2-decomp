#ifndef TASK_H
#define TASK_H

#include "common.h"

/*
 * A task slot of sub_main's table gTaskSlots (16 slots of 0x20 bytes):
 * TaskSlot_Set starts one with a function, which then runs with the slot as
 * its argument every frame (Task_RunFrame). The bytes from +8 on are the
 * task's own: every task keeps its state at +8 and the state's step at +9
 * (plsel_entry, the select tasks, GmSub_Task, the lobby's NetScene_Run,
 * plsel's galex tasks); the rest is named by the tasks that use it.
 */
typedef struct TaskSlot {
    s16 state;                  /* 0x00: 0 free; Task_RunFrame runs func in 8 (4 and 0xC go to 8, 0x10 waits) */
    s16 timer;                  /* 0x02: sub_main's state 0x10 counts it down */
    void (*func)(struct TaskSlot *t); /* 0x04: the task function, run in state 8 */
    u8 mode;                    /* 0x08: the task's state */
    u8 step;                    /* 0x09: the step within mode */
    u8 flag;                    /* 0x0A: the task's own: select boot: the pad is ready; plsel: the yes/no cursor, then the leaving steps */
    u8 count;                   /* 0x0B: the task's own: plsel: a blink and wait counter; gm_sub: GmSub_StepExit's step */
    s16 timeLeft;               /* 0x0C: plsel: frames left to choose online (3600; shown in seconds); select boot: a splash's frames */
    s16 splash;                 /* 0x0E: select boot: the splash screen's texture (Select_DrawSplash) */
    u8 unk10[0x14 - 0x10];
    u8 unk14;                   /* 0x14: plsel and the options clear it at their start */
    u8 unk15;                   /* 0x15: plsel: gSysSettings.unk45 at the choice's start; the options clear it */
    u8 unk16[0x20 - 0x16];
} TaskSlot;

typedef char TaskSlot_size_check[(sizeof(TaskSlot) == 0x20) ? 1 : -1];

#endif
