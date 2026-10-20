/*
 * The task system (include/task.h): gTaskSlots holds TASK_SLOT_COUNT
 * slots, each a state, a timer and a function that Task_RunFrame runs
 * once a frame with the slot as its argument; TaskSlot_InitAll and
 * TaskSlot_ClearFirstNine clear them. Task_RunFrame also starts the task
 * gSysSettings.taskRequest asks for, after loading its overlay. The
 * other functions here start, hold and end the slots' tasks, and
 * start the two tasks the overlays hand over to each other through:
 * select's Boot_Task, which runs the boot screens or goes on to gm_sub, and
 * gm_sub's GmSub_Task, which ends by asking Task_RunFrame for one of the
 * lobby's tasks (gSysSettings.taskRequest).
 */
#include "common.h"
#include "syssettings.h"
#include "task.h"
#include "overlay.h"

extern TaskSlot gTaskSlots[];
extern u8 gBootMode;
/* select's Boot_Task: the lobby has another function at this address, so
 * sub_main names it by address */
void D_6B5230(TaskSlot *t);
void GmSub_Task(TaskSlot *t);

void *memset(void *p, s32 c, u32 n);

/* Clears every task slot. */
void TaskSlot_InitAll(void)
{
    memset(gTaskSlots, 0, TASK_SLOT_COUNT * sizeof(TaskSlot));
}

/* Clears task slots 0-8 (TASK_FREE), leaving the others running: the return to the title
 * calls it before select starts. */
void TaskSlot_ClearFirstNine(void)
{
    s32 i;
    TaskSlot *task = gTaskSlots;

    for (i = 0; i < 9; i++) {
        memset(task, 0, sizeof(TaskSlot));
        task++;
    }
}

extern u16 gFrameTick;
extern s16 gPrimCount;
extern u16 gPadDebugMode;
/* The tasks Task_RunFrame starts in the other overlays. select's Boot_Task
 * goes by its address: sub_main's link is given the names of the lobby's,
 * game's, gm_sub's and plsel's code, but not select's. */
void NetScene_Run(TaskSlot *t);
void NetSetupTask_Run(void *t);
void D_6B5230(TaskSlot *t);         /* select's Boot_Task */
u16 Rand_Next(s32 stream);
void Callbacks_Clear(void);
void LoadOverlay(s32 id);
void TaskSlot_Set(void *func, s16 slot);
void Scene_QuestTask(u8 *obj);

/* The task slots' frame step: steps each slot's state (TaskSlot.state) and runs the tasks in
 * TASK_RUN. While gPadDebugMode or gSysSettings.unk07[0] is set, slots 2-11 are held: only
 * slots 0, 1 and 12-15 run. Then it starts the task gSysSettings.taskRequest asks for, after
 * loading its overlay. */
void Task_RunFrame(void)
{
    TaskSlot *task;
    s32 i;
    s32 busy;

    gFrameTick++;
    Rand_Next(1);
    busy = gPadDebugMode != 0;
    if (!busy) {
        Callbacks_Clear();
        gPrimCount = 0;
    }
    if (gSysSettings.unk07[0] != 0) {
        busy = 1;
    }
    for (i = 0, task = gTaskSlots; i < TASK_SLOT_COUNT; i++, task++) {
        if (!busy || i <= 1 || i > 11) {
            switch (task->state) {
            case TASK_FREE:
                break;
            case TASK_STARTING:
                task->state = TASK_RUN;
                break;
            case TASK_NEW:
                task->state = TASK_RUN;
                break;
            case TASK_RUN:
                task->func(task);
                break;
            case TASK_HELD:
                break;
            case TASK_RESTART:
                task->timer = 0;
                task->state = TASK_STARTING;
                break;
            case TASK_WAIT:
                task->timer--;
                if (task->timer < 0) {
                    task->timer = 0;
                } else if (task->timer == 0) {
                    task->state = TASK_STARTING;
                }
                break;
            }
        }
    }
    switch (gSysSettings.taskRequest) {
    case TASK_REQUEST_NONE:
        return;
    case TASK_REQUEST_NET_SCENE:
        LoadOverlay(OVERLAY_LOBBY);
        TaskSlot_Set(NetScene_Run, TASK_SLOT_SCENE);
        gSysSettings.taskRequest = TASK_REQUEST_NONE;
        return;
    case TASK_REQUEST_NET_SETUP:
        LoadOverlay(OVERLAY_LOBBY);
        TaskSlot_Set(NetSetupTask_Run, TASK_SLOT_SCENE);
        gSysSettings.taskRequest = TASK_REQUEST_NONE;
        return;
    case TASK_REQUEST_QUEST_SCENE:
        LoadOverlay(OVERLAY_GAME);
        TaskSlot_Set(Scene_QuestTask, TASK_SLOT_SCENE);
        gSysSettings.taskRequest = TASK_REQUEST_NONE;
        return;
    case TASK_REQUEST_BOOT:
        LoadOverlay(OVERLAY_SELECT);
        TaskSlot_Set(D_6B5230, TASK_SLOT_BOOT);
        gSysSettings.taskRequest = TASK_REQUEST_NONE;
        return;
    }
}

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
