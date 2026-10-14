#ifndef CARD_H
#define CARD_H

#include "common.h"

/* The memory card task and its operations (unk_3DDBA0.c). */

/* The card task (gCardTask, 0xF4 bytes). */
typedef struct CardTask {
    s32 step;             /* 0x00: the task step (the CardTask_*Step functions switch on it) */
    s32 subStep;          /* 0x04: the step within a card call (Card_ReadFile, Card_WriteFile, Card_GetInfo ...) */
    s32 fileNo;           /* 0x08: the file of the save operation being written (op->files[it]; CardTask_SaveStep) */
    s32 entryNo;          /* 0x0C: the directory entries found (Card_GetDir, from syncResult); the entry index (Card_DeleteDir) */
    s32 portCount;        /* 0x10: the ports to scan, 2 (Card_Init; CardTask_NextSlot) */
    s32 port;             /* 0x14: the memory card port (sceMcOpen, sceMcGetDir ...; cardState[it], the changed bit) */
    s32 syncCmd;          /* 0x18: the finished command (sceMcSync; Card_Sync) */
    s32 syncResult;       /* 0x1C: the result of the finished command (sceMcSync; Card_Sync): < 0 an error, an fd after sceMcOpen */
    s32 retries;        /* 0x20 */
    s32 result;         /* 0x24: -1 while busy */
    s32 type;           /* 0x28: card type, 2: PS2 card */
    s32 free;           /* 0x2C: free clusters */
    s32 format;         /* 0x30: nonzero when formatted */
    s32 cardState[3];   /* 0x34: per port (Card_PortState) */
    s32 cardFree[3];    /* 0x40: per port */
    s32 changed;        /* 0x4C: ports whose card changed, one bit each */
    char dir[0x22];     /* 0x50: directory name */
    char name[0x42];    /* 0x72: file name */
    s32 fd;             /* 0xB4: open file */
    s32 unkB8;          /* 0xB8: flag of the current file */
    s32 size;           /* 0xBC */
    s32 unkC0;          /* 0xC0: delete a directory with its files */
    s32 unkC4;          /* 0xC4 */
    s32 unkC8;          /* 0xC8 */
    s32 state;          /* 0xCC: index in gCardSteps */
    u8 *buf;            /* 0xD0 */
    s32 op;             /* 0xD4: index in gCardOps */
    u8 padD8[0xE0 - 0xD8];
    s32 unkE0;          /* 0xE0 */
    s32 mask;           /* 0xE4: files still to write, one bit each */
    u8 padE8[0xF4 - 0xE8];
} CardTask;

/* A file of a card operation (16 bytes). */
typedef struct McFileDesc {
    s32 flag;           /* 0x0: nonzero when used */
    char *name;         /* 0x4 */
    u8 *ptr;            /* 0x8 */
    s32 size;           /* 0xC */
} McFileDesc;

/* A card operation (gCardOps): the save's titles and its files
 * (files[0] is icon.sys, 1-3 the icons, 4-8 the data files). */
typedef struct CardOp {
    char *title1;       /* 0x00 */
    char *title2;       /* 0x04 */
    McFileDesc files[9]; /* 0x08 */
    u8 pad98[0x9C - 0x98];
    s32 unk9C;          /* 0x9C: to CardTask.unkC8 */
    s32 blocks;         /* 0xA0: card blocks needed */
} CardOp;

/* A memory card date and time (sceMcStDateTime; D_006AF0C8). */
typedef struct McDateTime {
    u8 wday;            /* 0x0: set by McDate_SetWeekday */
    u8 sec;
    u8 min;
    u8 hour;
    u8 day;             /* 0x4 */
    u8 month;           /* 0x5 */
    u16 year;           /* 0x6 */
} McDateTime;

extern CardOp *gCardOps[];
extern void (*gCardSteps[])(CardTask *t);
extern CardTask gCardTask;
extern McDateTime D_006AF0C8;

#endif /* CARD_H */
