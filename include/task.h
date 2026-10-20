#ifndef TASK_H
#define TASK_H

#include "common.h"

/*
 * The task system: sub_main's table gTaskSlots holds TASK_SLOT_COUNT slots
 * of 0x20 bytes. TaskSlot_Set starts a task with a function, which then
 * runs with its slot as its argument once a frame (Task_RunFrame). The
 * bytes from +8 on are the task's own: every task keeps
 * its state at +8 and the state's step at +9 (plsel_entry, the select
 * tasks, GmSub_Task, the lobby's NetScene_Run, plsel's galex tasks); the
 * rest is named by the tasks that use it.
 */

/* The slots, as the callers of TaskSlot_Set use them. */
#define TASK_SLOT_COUNT 16
#define TASK_SLOT_GMSUB 2   /* gm_sub's GmSub_Task (Task_StartGmSub) */
#define TASK_SLOT_BOOT 3    /* select's Boot_Task (Task_RestartBoot and the rest) */
#define TASK_SLOT_MENU 4    /* the select screens gm_sub starts (Gallery_Task, Chara_CreateTask, ...) */
#define TASK_SLOT_SCENE 5   /* the scene: Scene_QuestTask, the lobby's NetScene_Run and NetSetupTask_Run */
#define TASK_SLOT_PLSEL 6   /* plsel's tasks (plsel_entry, Galex_Task, Plsel_SelectTask) */

/*
 * TaskSlot.state: Task_RunFrame's switch. While the game is paused (the
 * pad's debug mode, or gSysSettings.unk07[0]) only slots 0, 1 and 12 to 15
 * are stepped.
 */
enum TaskState {
    TASK_FREE = 0,      /* the slot is free: nothing runs */
    TASK_HELD = 1,      /* kept but not run (TaskSlot_Hold); no code seen moves it on */
    TASK_RESTART = 2,   /* the timer is cleared, then TASK_STARTING (no code seen sets it) */
    TASK_STARTING = 4,  /* becomes TASK_RUN the next frame */
    TASK_RUN = 8,       /* the function runs every frame */
    TASK_NEW = 0xC,     /* set by TaskSlot_Set; becomes TASK_RUN the next frame */
    TASK_WAIT = 0x10    /* the timer counts down, then TASK_STARTING (no code seen sets it) */
};

/*
 * gSysSettings.taskRequest: a task Task_RunFrame starts after the frame's
 * tasks have run, loading its overlay first; then the request is cleared.
 */
enum TaskRequest {
    TASK_REQUEST_NONE = 0,
    TASK_REQUEST_NET_SCENE = 1,   /* the lobby (overlay 4): NetScene_Run in TASK_SLOT_SCENE */
    TASK_REQUEST_NET_SETUP = 2,   /* the lobby (overlay 4): NetSetupTask_Run in TASK_SLOT_SCENE */
    TASK_REQUEST_QUEST_SCENE = 3, /* overlay 3: Scene_QuestTask in TASK_SLOT_SCENE */
    TASK_REQUEST_BOOT = 4         /* select (overlay 2): Boot_Task in TASK_SLOT_BOOT */
};

/* gSysSettings.bootMode (gBootMode): where select's Boot_Task goes on to. */
enum BootMode {
    BOOT_MODE_SCREENS = 0,  /* the boot screens (Boot_SetMode) */
    BOOT_MODE_GMSUB = 1,    /* straight to gm_sub's task; Boot_Task then clears the mode */
    BOOT_MODE_GMSUB_KEEP = 2 /* straight to gm_sub's task, at every boot until the mode changes */
};

/*
 * gSysSettings.bootFrom: where Boot_Task is entered from (select_boot.c
 * switches on it): 0 the power on, 1 gm_main's exit, 2 a sub_main task
 * function (Task_RestartBoot and the rest).
 */
#define BOOT_FROM_POWER_ON 0
#define BOOT_FROM_GMSUB 1
#define BOOT_FROM_TASK 2

typedef struct TaskSlot {
    s16 state;                  /* 0x00: an enum TaskState */
    s16 timer;                  /* 0x02: TASK_WAIT counts it down */
    void (*func)(struct TaskSlot *t); /* 0x04: the task function, run in TASK_RUN */
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
