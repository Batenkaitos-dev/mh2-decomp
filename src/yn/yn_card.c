/*
 * yn: the first Capcom unit of the overlay (0xAC96D0-0xAC9B90), after
 * Sony's netcnfif library: memory card slot checks, and the work pointer
 * gYnWork that the rest of the overlay uses. The file owns its .rodata
 * (the network settings directory's name, 0xAD2770) and .bss
 * (0xAD5B40-0xAD9B50). The checks run one frame at a time through
 * sub_main's memory card task (gCardTask, the MC_* names in yn.h).
 */
#include "common.h"
#include "m2c_macros.h"
#include "yn.h"

void Stream_Reset(void);
void CardTask_Begin(s32 mode);
void CardTask_Update(void);
void CardTask_StartCheck(void);
void CardTask_StartList(s32 a, char *b, u8 *c);
s32 CardTask_Result(void);
s32 YnCard_MapError(s32 err);

/* .bss, declared from the highest address down (MWCC emits them last-declared
 * first). */
YnWork *gYnWork;
u8 gYnCardDirBuf[0x4000];

/* Clears a card check's states and puts sub_main's card task in mode 2. */
void YnCard_Reset(YnCardCheck *check)
{
    check->state = 0;
    check->loadState = 0;
    check->slot0Done = 0;
    CardTask_Begin(2);
}

/*
 * Checks both memory card slots, one frame at a time: once the card task is
 * free, starts a check of slot 0, then slot 1. Returns -1 while it runs,
 * then the slots found as bits (slot + 1): okSlots for card types 1 and 2,
 * slots3 for type 3.
 */
s32 YnCard_CheckSlots(YnCardCheck *check)
{
    s32 i;

    switch (check->state) {
    case 0:
        if (MC_BUSY != 0) {
            break;
        }
        check->state++;
        check->slot0Done = 0;
        check->okSlots = 0;
        check->slots3 = 0;
        CardTask_Begin(2);
        MC_SLOT = 0;
        MC_TYPE[0] = -1;
        MC_TYPE[1] = -1;
        CardTask_StartCheck();
    case 1:
        CardTask_Update();
        if (MC_TYPE[0] != -1 && check->slot0Done == 0) {
            check->slot0Done++;
            MC_SLOT = 1;
        }
        if (MC_TYPE[0] == -1 || MC_TYPE[1] == -1) {
            break;
        }
        for (i = 0; i < 2; i++) {
            if (MC_TYPE[i] == 1 || MC_TYPE[i] == 2) {
                check->okSlots |= (u8)(1 << (i + 1));
            }
            if (MC_TYPE[i] == 3) {
                check->slots3 |= (u8)(1 << (i + 1));
            }
        }
        check->state = 0;
        CardTask_Begin(2);
        return check->slots3 | check->okSlots;
    }
    return -1;
}

/*
 * YnCard_CheckSlots for the slot gYnWork has chosen only. Its card type is
 * read through gCardTaskTypes, which the match needs (gCardTask.type
 * compiles differently).
 */
s32 YnCard_CheckCurrentSlot(YnCardCheck *check)
{
    s32 slot;
    s32 *type;

    switch (check->state) {
    case 0:
        if (MC_BUSY != 0) {
            break;
        }
        check->state++;
        check->slot0Done = 0;
        check->okSlots = 0;
        check->slots3 = 0;
        CardTask_Begin(2);
        MC_SLOT = gYnWork->slot;
        gCardTaskTypes[gYnWork->slot] = -1;
        CardTask_StartCheck();
    case 1:
        CardTask_Update();
        slot = gYnWork->slot;
        type = &gCardTaskTypes[slot];
        if (*type == -1) {
            break;
        }
        if (*type == 1 || *type == 2) {
            check->okSlots |= (u8)(1 << (slot + 1));
        }
        if (*type == 3) {
            check->slots3 |= (u8)(1 << (slot + 1));
        }
        check->state = 0;
        CardTask_Begin(2);
        return check->slots3 | check->okSlots;
    }
    return -1;
}

/* Sets the card slot of later list requests. */
void YnCard_SetSlot(YnCardCheck *check, u8 slot)
{
    check->slot = slot;
}

/* Returns the card slot set by YnCard_SetSlot. */
s32 YnCard_GetSlot(YnCardCheck *check)
{
    return check->slot;
}

/*
 * Lists the network settings directory of the chosen slot's card into
 * gYnCardDirBuf, one frame at a time. Returns -2 while it runs, -15 for a
 * slot past the second, then -1 when listed or YnCard_MapError's code.
 */
s32 YnCard_ListNetcnfDir(YnCardCheck *check)
{
    s32 ret;

    switch (check->loadState) {
    case 0:
        if (check->slot >= 2) {
            return -15;
        }
        if (MC_BUSY != 0) {
            break;
        }
        check->loadState++;
        CardTask_Begin(2);
        CardTask_StartList(check->slot, "BWNETCNF", gYnCardDirBuf);
    case 1:
        CardTask_Update();
        ret = CardTask_Result();
        if (ret == -1) {
            break;
        }
        check->loadState = 0;
        if (ret < 0) {
            ret = YnCard_MapError(ret);
        } else {
            ret = -1;
        }
        return ret;
    }
    return -2;
}

/*
 * Maps the card task's error codes to yn's: 0 -> -1, -1 -> -2,
 * -0x100 -> -3, -0xFF -> -4, -0xFE -> -5, -0xFD -> -7, -0xFC -> -6, any
 * other -> -3.
 */
s32 YnCard_MapError(s32 err)
{
    switch (err) {
    case 0:
        return -1;
    case -1:
        return -2;
    case -256:
        return -3;
    case -255:
        return -4;
    case -254:
        return -5;
    case -253:
        return -7;
    case -252:
        return -6;
    default:
        return -3;
    }
}

/* Resets sub_main's streams. */
void Yn_ResetStreams(void)
{
    Stream_Reset();
}
