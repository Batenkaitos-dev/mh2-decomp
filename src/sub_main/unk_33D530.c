/*
 * The task system's slot functions (include/task.h): gTaskSlots holds
 * TASK_SLOT_COUNT slots, each a state, a timer and a function that
 * Task_RunFrame (unk_33B080.c) runs once a frame with the slot as its
 * argument. The functions here start, hold and end the slots' tasks, and
 * start the two tasks the overlays hand over to each other through:
 * select's Boot_Task, which runs the boot screens or goes on to gm_sub, and
 * gm_sub's GmSub_Task, which ends by asking Task_RunFrame for one of the
 * lobby's tasks (gSysSettings.taskRequest).
 */
#include "common.h"
#include "syssettings.h"
#include "task.h"

extern TaskSlot gTaskSlots[];
extern u8 gBootMode;
/* select's Boot_Task: the lobby has another function at this address, so
 * sub_main names it by address */
void D_6B5230(TaskSlot *t);
void GmSub_Task(TaskSlot *t);

void *memset(void *p, s32 c, u32 n);

/* Clears slot `slot` and starts func in it from the next frame. */
void TaskSlot_Set(void *func, s16 slot)
{
    TaskSlot *task = &gTaskSlots[slot];

    memset(task, 0, sizeof(TaskSlot));
    task->state = TASK_NEW;
    task->func = func;
}

/* Starts gm_sub's task in its slot, at its first state. */
void Task_StartGmSub(void)
{
    TaskSlot *task = &gTaskSlots[TASK_SLOT_GMSUB];

    memset(task, 0, sizeof(TaskSlot));
    task->state = TASK_NEW;
    task->func = GmSub_Task;
    task->mode = 1;
}

void TaskSlot_End(TaskSlot *task)
{
    task->state = TASK_FREE;
}

/* Keeps slot `slot`'s task but stops running it: gm_sub holds its own slot
 * while a select screen it started runs. */
void TaskSlot_Hold(s16 slot)
{
    gTaskSlots[slot].state = TASK_HELD;
}

/* Clears slot `slot` and runs func in it at once: in this frame's pass if
 * the slot is still to come. */
void TaskSlot_Prepare(void *func, s16 slot)
{
    TaskSlot *task = &gTaskSlots[slot];

    memset(task, 0, sizeof(TaskSlot));
    task->state = TASK_RUN;
    task->func = func;
}

TaskSlot *TaskSlot_Get(s16 slot)
{
    return &gTaskSlots[slot];
}

/* Ends gm_sub's task and asks for the lobby's NetScene_Run. */
void Task_EndGmSubForNetScene(void)
{
    gSysSettings.taskRequest = TASK_REQUEST_NET_SCENE;
    TaskSlot_End(&gTaskSlots[TASK_SLOT_GMSUB]);
}

/* Ends gm_sub's task and asks for the lobby's NetSetupTask_Run. */
void Task_EndGmSubForNetSetup(void)
{
    gSysSettings.taskRequest = TASK_REQUEST_NET_SETUP;
    TaskSlot_End(&gTaskSlots[TASK_SLOT_GMSUB]);
}

/* The four functions below start select's Boot_Task, each with a boot
 * mode. */

/* Boot_Task goes straight on to gm_sub, once. */
void Task_StartBootToGmSub(void)
{
    TaskSlot_Set(D_6B5230, TASK_SLOT_BOOT);
    gBootMode = BOOT_MODE_GMSUB;
    gSysSettings.bootFrom = BOOT_FROM_TASK;
}

/* Boot_Task goes straight on to gm_sub, at this boot and the later ones. */
void Task_StartBootToGmSubKeep(void)
{
    TaskSlot_Set(D_6B5230, TASK_SLOT_BOOT);
    gBootMode = BOOT_MODE_GMSUB_KEEP;
    gSysSettings.bootFrom = BOOT_FROM_TASK;
}

/* Boot_Task keeps the boot mode it has. */
void Task_StartBoot(void)
{
    TaskSlot_Set(D_6B5230, TASK_SLOT_BOOT);
    gSysSettings.bootFrom = BOOT_FROM_TASK;
}

/* Boot_Task runs the boot screens. */
void Task_StartBootScreens(void)
{
    TaskSlot_Set(D_6B5230, TASK_SLOT_BOOT);
    gBootMode = BOOT_MODE_SCREENS;
    gSysSettings.bootFrom = BOOT_FROM_TASK;
}
