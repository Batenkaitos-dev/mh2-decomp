/*
 * A table of 32-byte task slots at gTaskSlots: a state halfword and a
 * function pointer (+4). Slot 3 is started with a function of the loaded
 * overlay (0x6B5230), slot 2 with 0x8FAFD0. Semantics not yet known.
 */
#include "common.h"
#include "syssettings.h"
#include "task.h"

extern TaskSlot gTaskSlots[];
extern u8 gBootMode;
/* task functions in the loaded overlays */
void D_6B5230(TaskSlot *t); /* select Boot_Task */
void D_8FAFD0(TaskSlot *t); /* gm_sub GmSub_Task */

void *memset(void *p, s32 c, u32 n);

void TaskSlot_Set(void *func, s16 slot)
{
    TaskSlot *t = &gTaskSlots[slot];

    memset(t, 0, sizeof(TaskSlot));
    t->state = 0xC;
    t->func = func;
}

void Task_SetGmSub(void)
{
    TaskSlot *t = &gTaskSlots[2];

    memset(t, 0, sizeof(TaskSlot));
    t->state = 0xC;
    t->func = D_8FAFD0;
    t->mode = 1;
}

void TaskSlot_End(TaskSlot *t)
{
    t->state = 0;
}

void TaskSlot_Start(s16 slot)
{
    gTaskSlots[slot].state = 1;
}

void TaskSlot_Prepare(void *func, s16 slot)
{
    TaskSlot *t = &gTaskSlots[slot];

    memset(t, 0, sizeof(TaskSlot));
    t->state = 8;
    t->func = func;
}

TaskSlot *TaskSlot_Get(s16 slot)
{
    return &gTaskSlots[slot];
}

void TaskSlot2_EndCode1(void)
{
    gSysSettings.taskRequest = 1;
    TaskSlot_End(&gTaskSlots[2]);
}

void TaskSlot2_EndCode2(void)
{
    gSysSettings.taskRequest = 2;
    TaskSlot_End(&gTaskSlots[2]);
}

void Task_SetBootMode1(void)
{
    TaskSlot_Set(D_6B5230, 3);
    gBootMode = 1;
    gSysSettings.bootFrom = 2;
}

void Task_SetBootMode2(void)
{
    TaskSlot_Set(D_6B5230, 3);
    gBootMode = 2;
    gSysSettings.bootFrom = 2;
}

void Task_SetBoot(void)
{
    TaskSlot_Set(D_6B5230, 3);
    gSysSettings.bootFrom = 2;
}

void Task_SetBootMode0(void)
{
    TaskSlot_Set(D_6B5230, 3);
    gBootMode = 0;
    gSysSettings.bootFrom = 2;
}
