/*
 * yn: the first Capcom unit of the overlay (0xAC96D0-0xAC9B90), after
 * Sony's netcnfif library: memory card slot checks, and the work pointer
 * gYnWork that the rest of the overlay uses. The file owns its .rodata
 * ("BWNETCNF", 0xAD2770) and .bss (0xAD5B40-0xAD9B50).
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

void YnCard_Reset(YnCardCheck *c) {
    c->state = 0;
    c->loadState = 0;
    c->slot0Done = 0;
    CardTask_Begin(2);
}

s32 YnCard_CheckSlots(YnCardCheck *c) {
    s32 i;

    switch (c->state) {
    case 0:
        if (MC_BUSY != 0) {
            break;
        }
        c->state++;
        c->slot0Done = 0;
        c->okSlots = 0;
        c->slots3 = 0;
        CardTask_Begin(2);
        MC_SLOT = 0;
        MC_TYPE[0] = -1;
        MC_TYPE[1] = -1;
        CardTask_StartCheck();
    case 1:
        CardTask_Update();
        if (MC_TYPE[0] != -1 && c->slot0Done == 0) {
            c->slot0Done++;
            MC_SLOT = 1;
        }
        if (MC_TYPE[0] == -1 || MC_TYPE[1] == -1) {
            break;
        }
        for (i = 0; i < 2; i++) {
            if (MC_TYPE[i] == 1 || MC_TYPE[i] == 2) {
                c->okSlots |= (u8)(1 << (i + 1));
            }
            if (MC_TYPE[i] == 3) {
                c->slots3 |= (u8)(1 << (i + 1));
            }
        }
        c->state = 0;
        CardTask_Begin(2);
        return c->slots3 | c->okSlots;
    }
    return -1;
}

s32 YnCard_CheckCurrentSlot(YnCardCheck *c) {
    s32 slot;
    s32 *type;

    switch (c->state) {
    case 0:
        if (MC_BUSY != 0) {
            break;
        }
        c->state++;
        c->slot0Done = 0;
        c->okSlots = 0;
        c->slots3 = 0;
        CardTask_Begin(2);
        MC_SLOT = gYnWork->slot;
        D_6AF134[gYnWork->slot] = -1;
        CardTask_StartCheck();
    case 1:
        CardTask_Update();
        slot = gYnWork->slot;
        type = &D_6AF134[slot];
        if (*type == -1) {
            break;
        }
        if (*type == 1 || *type == 2) {
            c->okSlots |= (u8)(1 << (slot + 1));
        }
        if (*type == 3) {
            c->slots3 |= (u8)(1 << (slot + 1));
        }
        c->state = 0;
        CardTask_Begin(2);
        return c->slots3 | c->okSlots;
    }
    return -1;
}

void YnCard_SetSlot(YnCardCheck *c, u8 slot) {
    c->slot = slot;
}

s32 YnCard_GetSlot(YnCardCheck *c) {
    return c->slot;
}

s32 YnCard_ListNetcnfDir(YnCardCheck *c) {
    s32 ret;

    switch (c->loadState) {
    case 0:
        if (c->slot >= 2) {
            return -15;
        }
        if (MC_BUSY != 0) {
            break;
        }
        c->loadState++;
        CardTask_Begin(2);
        CardTask_StartList(c->slot, "BWNETCNF", gYnCardDirBuf);
    case 1:
        CardTask_Update();
        ret = CardTask_Result();
        if (ret == -1) {
            break;
        }
        c->loadState = 0;
        if (ret < 0) {
            ret = YnCard_MapError(ret);
        } else {
            ret = -1;
        }
        return ret;
    }
    return -2;
}

s32 YnCard_MapError(s32 err) {
    switch (err) {                                 /* irregular */
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

void Yn_ResetStreams(void) {
    Stream_Reset();
}
